#include "../core/global.h"
#include "../core/commandloop.h"
#include "../core/config_parser.h"
#include "../core/fileutils.h"
#include "../core/timer.h"
#include "../core/datetime.h"
#include "../core/makedir.h"
#include "../dataio/sgf.h"
#include "../search/asyncbot.h"
#include "../search/patternbonustable.h"
#include "../program/setup.h"
#include "../program/playutils.h"
#include "../program/play.h"
#include "../tests/tests.h"
#include "../command/commandline.h"
#include "../command/gtpKata.h"
#ifdef NO_GIT_REVISION
#define GIT_REVISION "<omitted>"
#else
#include "../program/gitinfo.h"
#endif
#include <wx/translation.h>

using namespace std;

static const vector<string> knownCommands = {
  //Basic GTP commands
  "protocol_version",
  "name",
  "version",
  "known_command",
  "list_commands",
  "quit",

  //GTP extension - specify "boardsize X:Y" or "boardsize X Y" for non-square sizes
  //rectangular_boardsize is an alias for boardsize, intended to make it more evident that we have such support
  "boardsize",
  "rectangular_boardsize",

  "clear_board",
  "set_position",
  "komi",
  //GTP extension - get KataGo's current komi setting
  "get_komi",
  "play",
  "undo",

  //GTP extension - specify rules
  "kata-get-rules",
  "kata-set-rule",
  "kata-set-rules",

  //Get or change a few limited params dynamically
  "kata-get-param",
  "kata-set-param",
  "kata-list-params",
  "kgs-rules",

  "genmove",
  "genmove_debug", //Prints additional info to stderr
  "search_debug", //Prints additional info to stderr, doesn't actually make the move

  //Clears neural net cached evaluations and bot search tree, allows fresh randomization
  "clear_cache",

  "showboard",
  "fixed_handicap",
  "place_free_handicap",
  "set_free_handicap",

  "time_settings",
  "kgs-time_settings",
  "time_left",
  //KataGo extensions for time settings
  "kata-list_time_settings",
  "kata-time_settings",

  "final_score",
  "final_status_list",

  "loadsgf",
  "printsgf",

  //GTP extensions for board analysis
  "genmove_analyze",
  "lz-genmove_analyze",
  "kata-genmove_analyze",
  "analyze",
  "lz-analyze",
  "kata-analyze",

  //Display raw neural net evaluations
  "kata-raw-nn",

  //Misc other stuff
  "cputime",
  "gomill-cpu_time",
  "kata-benchmark",

  //Some debug commands
  "kata-debug-print-tc",

  //Stop any ongoing ponder or analyze
  "stop",
};

static bool tryParseLoc(const string& s, const Board& b, Loc& loc) {
  return Location::tryOfString(s,b,loc);
}

//Filter out all double newlines, since double newline terminates GTP command responses
static string filterDoubleNewlines(const string& s) {
  string filtered;
  for(int i = 0; i<s.length(); i++) {
    if(i > 0 && s[i-1] == '\n' && s[i] == '\n')
      continue;
    filtered += s[i];
  }
  return filtered;
}

static bool timeIsValid(const double& time) {
  if(isnan(time) || time < 0.0 || time > TimeControls::MAX_USER_INPUT_TIME)
    return false;
  return true;
}
static bool timeIsValidAllowNegative(const double& time) {
  if(isnan(time) || time < -TimeControls::MAX_USER_INPUT_TIME || time > TimeControls::MAX_USER_INPUT_TIME)
    return false;
  return true;
}

static double parseTime(const vector<string>& args, int argIdx, const string& description) {
  double time = 0.0;
  if(args.size() <= argIdx || !Global::tryStringToDouble(args[argIdx],time))
    throw StringError("Expected float for " + description + " as argument " + Global::intToString(argIdx));
  if(!timeIsValid(time))
    throw StringError(description + " is an invalid value: " + args[argIdx]);
  return time;
}
static double parseTimeAllowNegative(const vector<string>& args, int argIdx, const string& description) {
  double time = 0.0;
  if(args.size() <= argIdx || !Global::tryStringToDouble(args[argIdx],time))
    throw StringError("Expected float for " + description + " as argument " + Global::intToString(argIdx));
  if(!timeIsValidAllowNegative(time))
    throw StringError(description + " is an invalid value: " + args[argIdx]);
  return time;
}
static int parseByoYomiStones(const vector<string>& args, int argIdx) {
  int byoYomiStones = 0;
  if(args.size() <= argIdx || !Global::tryStringToInt(args[argIdx],byoYomiStones))
    throw StringError("Expected int for byo-yomi overtime stones as argument " + Global::intToString(argIdx));
  if(byoYomiStones < 0 || byoYomiStones > 1000000)
    throw StringError("byo-yomi overtime stones is an invalid value: " + args[argIdx]);
  return byoYomiStones;
}
static int parseByoYomiPeriods(const vector<string>& args, int argIdx) {
  int byoYomiPeriods = 0;
  if(args.size() <= argIdx || !Global::tryStringToInt(args[argIdx],byoYomiPeriods))
    throw StringError("Expected int for byo-yomi overtime periods as argument " + Global::intToString(argIdx));
  if(byoYomiPeriods < 0 || byoYomiPeriods > 1000000)
    throw StringError("byo-yomi overtime periods is an invalid value: " + args[argIdx]);
  return byoYomiPeriods;
}

//Assumes that stones are worth 15 points area and 14 points territory, and that 7 komi is fair
static double initialBlackAdvantage(const BoardHistory& hist) {
  BoardHistory histCopy = hist;
  histCopy.setAssumeMultipleStartingBlackMovesAreHandicap(true);
  int handicapStones = histCopy.computeNumHandicapStones();
  if(handicapStones <= 1)
    return 7.0 - hist.rules.komi;

  //Subtract one since white gets the first move afterward
  int extraBlackStones = handicapStones - 1;
  double stoneValue = hist.rules.scoringRule == Rules::SCORING_AREA ? 15.0 : 14.0;
  double whiteHandicapBonus = 0.0;
  if(hist.rules.whiteHandicapBonusRule == Rules::WHB_N)
    whiteHandicapBonus += handicapStones;
  else if(hist.rules.whiteHandicapBonusRule == Rules::WHB_N_MINUS_ONE)
    whiteHandicapBonus += handicapStones-1;

  return stoneValue * extraBlackStones + (7.0 - hist.rules.komi - whiteHandicapBonus);
}

static double getBoardSizeScaling(const Board& board) {
  return pow(19.0 * 19.0 / (double)(board.x_size * board.y_size), 0.75);
}
static double getPointsThresholdForHandicapGame(double boardSizeScaling) {
  return std::max(4.0 / boardSizeScaling, 2.0);
}

static bool noWhiteStonesOnBoard(const Board& board) {
  for(int y = 0; y < board.y_size; y++) {
    for(int x = 0; x < board.x_size; x++) {
      Loc loc = Location::getLoc(x,y,board.x_size);
      if(board.colors[loc] == P_WHITE)
        return false;
    }
  }
  return true;
}

static void updateDynamicPDAHelper(
  const Board& board, const BoardHistory& hist,
  const double dynamicPlayoutDoublingAdvantageCapPerOppLead,
  const vector<double>& recentWinLossValues,
  double& desiredDynamicPDAForWhite
) {
  (void)board;
  if(dynamicPlayoutDoublingAdvantageCapPerOppLead <= 0.0) {
    desiredDynamicPDAForWhite = 0.0;
  }
  else {
    double boardSizeScaling = getBoardSizeScaling(board);
    double pdaScalingStartPoints = getPointsThresholdForHandicapGame(boardSizeScaling);
    double initialBlackAdvantageInPoints = initialBlackAdvantage(hist);
    Player disadvantagedPla = initialBlackAdvantageInPoints >= 0 ? P_WHITE : P_BLACK;
    double initialAdvantageInPoints = std::fabs(initialBlackAdvantageInPoints);
    if(initialAdvantageInPoints < pdaScalingStartPoints || board.x_size <= 7 || board.y_size <= 7) {
      desiredDynamicPDAForWhite = 0.0;
    }
    else {
      double desiredDynamicPDAForDisadvantagedPla =
        (disadvantagedPla == P_WHITE) ? desiredDynamicPDAForWhite : -desiredDynamicPDAForWhite;

      //What increment to adjust desiredPDA at.
      //Power of 2 to avoid any rounding issues.
      const double increment = 0.125;

      //Hard cap of 2.75 in this parameter, since more extreme values start to reach into values without good training.
      //Scale mildly with board size - small board a given point lead counts as "more".
      double pdaCap = std::min(
        2.75,
        dynamicPlayoutDoublingAdvantageCapPerOppLead *
        (initialAdvantageInPoints - pdaScalingStartPoints) * boardSizeScaling
      );
      pdaCap = round(pdaCap / increment) * increment;

      //No history, or literally no white stones on board? Then this is a new game or a newly set position
      if(recentWinLossValues.size() <= 0 || noWhiteStonesOnBoard(board)) {
        //Just use the cap
        desiredDynamicPDAForDisadvantagedPla = pdaCap;
      }
      else {
        double winLossValue = recentWinLossValues[recentWinLossValues.size()-1];
        //Convert to perspective of disadvantagedPla
        if(disadvantagedPla == P_BLACK)
          winLossValue = -winLossValue;

        //Keep winLossValue between 5% and 25%, subject to available caps.
        if(winLossValue < -0.9)
          desiredDynamicPDAForDisadvantagedPla = desiredDynamicPDAForDisadvantagedPla + 0.125;
        else if(winLossValue > -0.5)
          desiredDynamicPDAForDisadvantagedPla = desiredDynamicPDAForDisadvantagedPla - 0.125;

        desiredDynamicPDAForDisadvantagedPla = std::max(desiredDynamicPDAForDisadvantagedPla, 0.0);
        desiredDynamicPDAForDisadvantagedPla = std::min(desiredDynamicPDAForDisadvantagedPla, pdaCap);
      }

      desiredDynamicPDAForWhite = (disadvantagedPla == P_WHITE) ? desiredDynamicPDAForDisadvantagedPla : -desiredDynamicPDAForDisadvantagedPla;
    }
  }
}

static bool shouldResign(
  const Board& board,
  const BoardHistory& hist,
  Player pla,
  const vector<double>& recentWinLossValues,
  double lead,
  const double resignThreshold,
  const int resignConsecTurns,
  const double resignMinScoreDifference
) {
  double initialBlackAdvantageInPoints = initialBlackAdvantage(hist);

  int minTurnForResignation = 0;
  double noResignationWhenWhiteScoreAbove = board.x_size * board.y_size;
  if(initialBlackAdvantageInPoints > 0.9 && pla == P_WHITE) {
    //Play at least some moves no matter what
    minTurnForResignation = 1 + board.x_size * board.y_size / 5;

    //In a handicap game, also only resign if the lead difference is well behind schedule assuming
    //that we're supposed to catch up over many moves.
    double numTurnsToCatchUp = 0.60 * board.x_size * board.y_size - minTurnForResignation;
    double numTurnsSpent = (double)(hist.moveHistory.size()) - minTurnForResignation;
    if(numTurnsToCatchUp <= 1.0)
      numTurnsToCatchUp = 1.0;
    if(numTurnsSpent <= 0.0)
      numTurnsSpent = 0.0;
    if(numTurnsSpent > numTurnsToCatchUp)
      numTurnsSpent = numTurnsToCatchUp;

    double resignScore = -initialBlackAdvantageInPoints * ((numTurnsToCatchUp - numTurnsSpent) / numTurnsToCatchUp);
    resignScore -= 5.0; //Always require at least a 5 point buffer
    resignScore -= initialBlackAdvantageInPoints * 0.15; //And also require a 15% of the initial handicap

    noResignationWhenWhiteScoreAbove = resignScore;
  }

  if(hist.moveHistory.size() < minTurnForResignation)
    return false;
  if(pla == P_WHITE && lead > noResignationWhenWhiteScoreAbove)
    return false;
  if(resignConsecTurns > recentWinLossValues.size())
    return false;
  //Don't resign close games.
  if((pla == P_WHITE && lead > -resignMinScoreDifference) || (pla == P_BLACK && lead < resignMinScoreDifference))
    return false;

  for(int i = 0; i<resignConsecTurns; i++) {
    double winLossValue = recentWinLossValues[recentWinLossValues.size()-1-i];
    Player resignPlayerThisTurn = C_EMPTY;
    if(winLossValue < resignThreshold)
      resignPlayerThisTurn = P_WHITE;
    else if(winLossValue > -resignThreshold)
      resignPlayerThisTurn = P_BLACK;

    if(resignPlayerThisTurn != pla)
      return false;
  }

  return true;
}

GTPEngine::GTPEngine(
  const string& modelFile, SearchParams initialParams, Rules initialRules,
  bool assumeMultiBlackHandicap, bool prevtEncore,
  double dynamicPDACapPerOppLead, double staticPDA, bool staticPDAPrecedence,
  double normAvoidRepeatedPatternUtility, double hcapAvoidRepeatedPatternUtility,
  bool avoidDagger,
  double genmoveWRN, double analysisWRN,
  bool genmoveAntiMir, bool analysisAntiMir,
  Player persp, int pvLen,
  std::unique_ptr<PatternBonusTable>&& pbTable
)
  :nnModelFile(modelFile),
   assumeMultipleStartingBlackMovesAreHandicap(assumeMultiBlackHandicap),
   analysisPVLen(pvLen),
   preventEncore(prevtEncore),
   dynamicPlayoutDoublingAdvantageCapPerOppLead(dynamicPDACapPerOppLead),
   staticPlayoutDoublingAdvantage(staticPDA),
   staticPDATakesPrecedence(staticPDAPrecedence),
   normalAvoidRepeatedPatternUtility(normAvoidRepeatedPatternUtility),
   handicapAvoidRepeatedPatternUtility(hcapAvoidRepeatedPatternUtility),
   genmoveWideRootNoise(genmoveWRN),
   analysisWideRootNoise(analysisWRN),
   genmoveAntiMirror(genmoveAntiMir),
   analysisAntiMirror(analysisAntiMir),
   nnEval(NULL),
   bot(NULL),
   currentRules(initialRules),
   params(initialParams),
   bTimeControls(),
   wTimeControls(),
   initialBoard(),
   initialPla(P_BLACK),
   moveHistory(),
   recentWinLossValues(),
   lastSearchFactor(1.0),
   desiredDynamicPDAForWhite(0.0),
   avoidMYTDaggerHack(avoidDagger),
   patternBonusTable(std::move(pbTable)),
   perspective(persp),
   genmoveTimeSum(0.0)
{
}

GTPEngine::~GTPEngine() {
  stopAndWait();
  delete bot;
  delete nnEval;
}

void GTPEngine::stopAndWait() {
  bot->stopAndWait();
}

Rules GTPEngine::getCurrentRules() {
  return currentRules;
}

void GTPEngine::clearStatsForNewGame() {
  //Currently nothing
}

//Specify -1 for the sizes for a default
void GTPEngine::setOrResetBoardSize(ConfigParser& cfg, Logger& logger, Rand& seedRand, int boardXSize, int boardYSize, bool loggingToStderr) {
  if(nnEval != NULL && boardXSize == nnEval->getNNXLen() && boardYSize == nnEval->getNNYLen())
    return;
  if(nnEval != NULL) {
    assert(bot != NULL);
    bot->stopAndWait();
    delete bot;
    delete nnEval;
    bot = NULL;
    nnEval = NULL;
    logger.write("Cleaned up old neural net and bot");
  }

  bool wasDefault = false;
  if(boardXSize == -1 || boardYSize == -1) {
    boardXSize = Board::DEFAULT_LEN;
    boardYSize = Board::DEFAULT_LEN;
    wasDefault = true;
  }

  const int maxConcurrentEvals = params.numThreads * 2 + 16; // * 2 + 16 just to give plenty of headroom
  const int expectedConcurrentEvals = params.numThreads;
  const int defaultMaxBatchSize = std::max(8,((params.numThreads+3)/4)*4);
  const bool defaultRequireExactNNLen = true;
  const string expectedSha256 = "";
  nnEval = Setup::initializeNNEvaluator(
    nnModelFile,nnModelFile,expectedSha256,cfg,logger,seedRand,maxConcurrentEvals,expectedConcurrentEvals,
    boardXSize,boardYSize,defaultMaxBatchSize,defaultRequireExactNNLen,
    Setup::SETUP_FOR_GTP
  );
  logger.write("Loaded neural net with nnXLen " + Global::intToString(nnEval->getNNXLen()) + " nnYLen " + Global::intToString(nnEval->getNNYLen()));

  {
    bool rulesWereSupported;
    nnEval->getSupportedRules(currentRules,rulesWereSupported);
    if(!rulesWereSupported) {
      throw StringError("Rules " + currentRules.toJsonStringNoKomi() + " from config file " + cfg.getFileName() + " are NOT supported by neural net");
    }
  }

  //On default setup, also override board size to whatever the neural net was initialized with
  //So that if the net was initalized smaller, we don't fail with a big board
  if(wasDefault) {
    boardXSize = nnEval->getNNXLen();
    boardYSize = nnEval->getNNYLen();
  }
  logger.write("Initializing board with boardXSize " + Global::intToString(boardXSize) + " boardYSize " + Global::intToString(boardYSize));
  if(!loggingToStderr)
    cerr << ("Initializing board with boardXSize " + Global::intToString(boardXSize) + " boardYSize " + Global::intToString(boardYSize)) << endl;

  string searchRandSeed;
  if(cfg.contains("searchRandSeed"))
    searchRandSeed = cfg.getString("searchRandSeed");
  else
    searchRandSeed = Global::uint64ToString(seedRand.nextUInt64());

  bot = new AsyncBot(params, nnEval, &logger, searchRandSeed);
  bot->setCopyOfExternalPatternBonusTable(patternBonusTable);

  Board board(boardXSize,boardYSize);
  Player pla = P_BLACK;
  BoardHistory hist(board,pla,currentRules,0);
  vector<Move> newMoveHistory;
  setPositionAndRules(pla,board,hist,board,pla,newMoveHistory);
  clearStatsForNewGame();
}

void GTPEngine::setPositionAndRules(Player pla, const Board& board, const BoardHistory& h, const Board& newInitialBoard, Player newInitialPla, const vector<Move> newMoveHistory) {
  BoardHistory hist(h);
  //Ensure we always have this value correct
  hist.setAssumeMultipleStartingBlackMovesAreHandicap(assumeMultipleStartingBlackMovesAreHandicap);
   currentRules = hist.rules;
  bot->setPosition(pla,board,hist);
  initialBoard = newInitialBoard;
  initialPla = newInitialPla;
  moveHistory = newMoveHistory;
  recentWinLossValues.clear();
  updateDynamicPDA();
}

void GTPEngine::clearBoard() {
  assert(bot->getRootHist().rules == currentRules);
  int newXSize = bot->getRootBoard().x_size;
  int newYSize = bot->getRootBoard().y_size;
  Board board(newXSize,newYSize);
  Player pla = P_BLACK;
  BoardHistory hist(board,pla,currentRules,0);
  vector<Move> newMoveHistory;
  setPositionAndRules(pla,board,hist,board,pla,newMoveHistory);
  clearStatsForNewGame();
}

bool GTPEngine::setPosition(const vector<Move>& initialStones) {
  assert(bot->getRootHist().rules == currentRules);
  int newXSize = bot->getRootBoard().x_size;
  int newYSize = bot->getRootBoard().y_size;
  Board board(newXSize,newYSize);
  for(int i = 0; i<initialStones.size(); i++) {
    if(!board.isOnBoard(initialStones[i].loc) || board.colors[initialStones[i].loc] != C_EMPTY) {
      return false;
    }
    bool suc = board.setStone(initialStones[i].loc, initialStones[i].pla);
    if(!suc) {
      return false;
    }
  }

  //Make sure nothing died along the way
  for(int i = 0; i<initialStones.size(); i++) {
    if(board.colors[initialStones[i].loc] != initialStones[i].pla) {
      return false;
    }
  }
  Player pla = P_BLACK;
  BoardHistory hist(board,pla,currentRules,0);
  hist.setInitialTurnNumber(board.numStonesOnBoard()); //Heuristic to guess at what turn this is
  vector<Move> newMoveHistory;
  setPositionAndRules(pla,board,hist,board,pla,newMoveHistory);
  clearStatsForNewGame();
  return true;
}

void GTPEngine::updateKomiIfNew(float newKomi) {
  bot->setKomiIfNew(newKomi);
  currentRules.komi = newKomi;
}

void GTPEngine::setStaticPlayoutDoublingAdvantage(double d) {
  staticPlayoutDoublingAdvantage = d;
  staticPDATakesPrecedence = true;
}
void GTPEngine::setAnalysisWideRootNoise(double x) {
  analysisWideRootNoise = x;
}
void GTPEngine::setRootPolicyTemperature(double x) {
  params.rootPolicyTemperature = x;
  bot->setParams(params);
  bot->clearSearch();
}
void GTPEngine::setNumSearchThreads(int numThreads) {
  params.numThreads = numThreads;
  bot->setParams(params);
  bot->clearSearch();
}

void GTPEngine::updateDynamicPDA() {
  updateDynamicPDAHelper(
    bot->getRootBoard(),bot->getRootHist(),
    dynamicPlayoutDoublingAdvantageCapPerOppLead,
    recentWinLossValues,
    desiredDynamicPDAForWhite
  );
}

bool GTPEngine::play(Loc loc, Player pla) {
  assert(bot->getRootHist().rules == currentRules);
  bool suc = bot->makeMove(loc,pla,preventEncore);
  if(suc)
    moveHistory.push_back(Move(loc,pla));
  return suc;
}

bool GTPEngine::undo() {
  if(moveHistory.size() <= 0)
    return false;
  assert(bot->getRootHist().rules == currentRules);

  vector<Move> moveHistoryCopy = moveHistory;

  Board undoneBoard = initialBoard;
  BoardHistory undoneHist(undoneBoard,initialPla,currentRules,0);
  undoneHist.setInitialTurnNumber(bot->getRootHist().initialTurnNumber);
  vector<Move> emptyMoveHistory;
  setPositionAndRules(initialPla,undoneBoard,undoneHist,initialBoard,initialPla,emptyMoveHistory);

  for(int i = 0; i<moveHistoryCopy.size()-1; i++) {
    Loc moveLoc = moveHistoryCopy[i].loc;
    Player movePla = moveHistoryCopy[i].pla;
    bool suc = play(moveLoc,movePla);
    assert(suc);
    (void)suc; //Avoid warning when asserts are off
  }
  return true;
}

bool GTPEngine::setRulesNotIncludingKomi(Rules newRules, string& error) {
  assert(nnEval != NULL);
  assert(bot->getRootHist().rules == currentRules);
  newRules.komi = currentRules.komi;

  bool rulesWereSupported;
  nnEval->getSupportedRules(newRules,rulesWereSupported);
  if(!rulesWereSupported) {
    error = "Rules " + newRules.toJsonStringNoKomi() + " are not supported by this neural net version";
    return false;
  }

  vector<Move> moveHistoryCopy = moveHistory;

  Board board = initialBoard;
  BoardHistory hist(board,initialPla,newRules,0);
  hist.setInitialTurnNumber(bot->getRootHist().initialTurnNumber);
  vector<Move> emptyMoveHistory;
  setPositionAndRules(initialPla,board,hist,initialBoard,initialPla,emptyMoveHistory);

  for(int i = 0; i<moveHistoryCopy.size(); i++) {
    Loc moveLoc = moveHistoryCopy[i].loc;
    Player movePla = moveHistoryCopy[i].pla;
    bool suc = play(moveLoc,movePla);

    //Because internally we use a highly tolerant test, we don't expect this to actually trigger
    //even if a rules change did make some earlier moves illegal. But this check simply futureproofs
    //things in case we ever do
    if(!suc) {
      error = "Could not make the rules change, some earlier moves in the game would now become illegal.";
      return false;
    }
  }
  return true;
}

void GTPEngine::ponder() {
  bot->ponder(lastSearchFactor);
}

void GTPEngine::filterZeroVisitMoves(const AnalyzeArgs& args, vector<AnalysisData> buf) {
  //Avoid printing moves that have 0 visits, unless we need them
  //These should already be sorted so that 0-visit moves only appear at the end.
  int keptMoves = 0;
  for(int i = 0; i<buf.size(); i++) {
    if(buf[i].numVisits > 0 || keptMoves < args.minMoves)
      buf[keptMoves++] = buf[i];
  }
  buf.resize(keptMoves);
}

std::function<void(const Search* search)> GTPEngine::getAnalyzeCallback(Player pla, AnalyzeArgs args) {
  std::function<void(const Search* search)> callback;
  //lz-analyze
  if(args.lz && !args.kata) {
    //Avoid capturing anything by reference except [this], since this will potentially be used
    //asynchronously and called after we return
    callback = [args,pla,this](const Search* search) {
      vector<AnalysisData> buf;
      bool duplicateForSymmetries = true;
      search->getAnalysisData(buf,args.minMoves,false,analysisPVLen,duplicateForSymmetries);
      filterZeroVisitMoves(args,buf);
      if(buf.size() > args.maxMoves)
        buf.resize(args.maxMoves);
      if(buf.size() <= 0)
        return;

      const Board board = search->getRootBoard();
      for(int i = 0; i<buf.size(); i++) {
        if(i > 0)
          cout << " ";
        const AnalysisData& data = buf[i];
        double winrate = 0.5 * (1.0 + data.winLossValue);
        double lcb = PlayUtils::getHackedLCBForWinrate(search,data,pla);
        if(perspective == P_BLACK || (perspective != P_BLACK && perspective != P_WHITE && pla == P_BLACK)) {
          winrate = 1.0-winrate;
          lcb = 1.0 - lcb;
        }
        cout << "info";
        cout << " move " << Location::toString(data.move,board);
        cout << " visits " << data.numVisits;
        cout << " winrate " << round(winrate * 10000.0);
        cout << " prior " << round(data.policyPrior * 10000.0);
        cout << " lcb " << round(lcb * 10000.0);
        cout << " order " << data.order;
        cout << " pv ";
        if(preventEncore && data.pvContainsPass())
          data.writePVUpToPhaseEnd(cout,board,search->getRootHist(),search->getRootPla());
        else
          data.writePV(cout,board);
        if(args.showPVVisits) {
          cout << " pvVisits ";
          if(preventEncore && data.pvContainsPass())
            data.writePVVisitsUpToPhaseEnd(cout,board,search->getRootHist(),search->getRootPla());
          else
            data.writePVVisits(cout);
        }
        if(args.showPVEdgeVisits) {
          cout << " pvEdgeVisits ";
          if(preventEncore && data.pvContainsPass())
            data.writePVEdgeVisitsUpToPhaseEnd(cout,board,search->getRootHist(),search->getRootPla());
          else
            data.writePVEdgeVisits(cout);
        }
      }
      cout << endl;
    };
  }
  //kata-analyze, analyze (sabaki)
  else {
    callback = [args,pla,this](const Search* search) {
      vector<AnalysisData> buf;
      bool duplicateForSymmetries = true;
      search->getAnalysisData(buf,args.minMoves,false,analysisPVLen,duplicateForSymmetries);
      filterZeroVisitMoves(args,buf);
      if(buf.size() > args.maxMoves)
        buf.resize(args.maxMoves);
      if(buf.size() <= 0)
        return;

      vector<double> ownership, ownershipStdev;
      if(args.showOwnershipStdev) {
        tuple<vector<double>,vector<double>> ownershipAverageAndStdev;
        ownershipAverageAndStdev = search->getAverageAndStandardDeviationTreeOwnership();
        ownership = std::get<0>(ownershipAverageAndStdev);
        ownershipStdev = std::get<1>(ownershipAverageAndStdev);
      }
      else if(args.showOwnership) {
        ownership = search->getAverageTreeOwnership();
      }

      ostringstream out;
      if(!args.kata) {
        //Hack for sabaki - ensure always showing decimal point. Also causes output to be more verbose with trailing zeros,
        //unfortunately, despite doing not improving the precision of the values.
        out << std::showpoint;
      }

      const Board board = search->getRootBoard();
      for(int i = 0; i<buf.size(); i++) {
        if(i > 0)
          out << " ";
        const AnalysisData& data = buf[i];
        double winrate = 0.5 * (1.0 + data.winLossValue);
        double utility = data.utility;
        //We still hack the LCB for consistency with LZ-analyze
        double lcb = PlayUtils::getHackedLCBForWinrate(search,data,pla);
        ///But now we also offer the proper LCB that KataGo actually uses.
        double utilityLcb = data.lcb;
        double scoreMean = data.scoreMean;
        double lead = data.lead;
        if(perspective == P_BLACK || (perspective != P_BLACK && perspective != P_WHITE && pla == P_BLACK)) {
          winrate = 1.0-winrate;
          lcb = 1.0 - lcb;
          utility = -utility;
          scoreMean = -scoreMean;
          lead = -lead;
          utilityLcb = -utilityLcb;
        }
        out << "info";
        out << " move " << Location::toString(data.move,board);
        out << " visits " << data.numVisits;
        out << " utility " << utility;
        out << " winrate " << winrate;
        // We report lead for scoreMean here so that a bunch of legacy tools that use KataGo use lead instead, which
        // is usually a better field for user applications. We report scoreMean instead as scoreSelfplay
        out << " scoreMean " << lead;
        out << " scoreStdev " << data.scoreStdev;
        out << " scoreLead " << lead;
        out << " scoreSelfplay " << scoreMean;
        out << " prior " << data.policyPrior;
        out << " lcb " << lcb;
        out << " utilityLcb " << utilityLcb;
        if(data.isSymmetryOf != Board::NULL_LOC)
          out << " isSymmetryOf " << Location::toString(data.isSymmetryOf,board);
        out << " order " << data.order;
        out << " pv ";
        if(preventEncore && data.pvContainsPass())
          data.writePVUpToPhaseEnd(out,board,search->getRootHist(),search->getRootPla());
        else
          data.writePV(out,board);
        if(args.showPVVisits) {
          out << " pvVisits ";
          if(preventEncore && data.pvContainsPass())
            data.writePVVisitsUpToPhaseEnd(out,board,search->getRootHist(),search->getRootPla());
          else
            data.writePVVisits(out);
        }
        if(args.showPVEdgeVisits) {
          out << " pvEdgeVisits ";
          if(preventEncore && data.pvContainsPass())
            data.writePVEdgeVisitsUpToPhaseEnd(out,board,search->getRootHist(),search->getRootPla());
          else
            data.writePVEdgeVisits(out);
        }
        vector<double> movesOwnership, movesOwnershipStdev;
        if(args.showMovesOwnershipStdev) {
          tuple<vector<double>,vector<double>> movesOwnershipAverageAndStdev;
          movesOwnershipAverageAndStdev = search->getAverageAndStandardDeviationTreeOwnership(perspective,data.node,data.symmetry);
          movesOwnership = std::get<0>(movesOwnershipAverageAndStdev);
          movesOwnershipStdev = std::get<1>(movesOwnershipAverageAndStdev);

        }
        else if(args.showMovesOwnership) {
          movesOwnership = search->getAverageTreeOwnership(perspective,data.node,data.symmetry);
        }
        if(args.showMovesOwnership) {
          out << " ";
          out << "movesOwnership";
          int nnXLen = search->nnXLen;
          for(int y = 0; y<board.y_size; y++) {
            for(int x = 0; x<board.x_size; x++) {
              int pos = NNPos::xyToPos(x,y,nnXLen);
              out << " " << movesOwnership[pos]; // perspective already handled by getAverageAndStandardDeviationTreeOwnership
            }
          }
        }
        if(args.showMovesOwnershipStdev) {
          out << " ";
          out << "movesOwnershipStdev";
          int nnXLen = search->nnXLen;
          for(int y = 0; y<board.y_size; y++) {
            for(int x = 0; x<board.x_size; x++) {
              int pos = NNPos::xyToPos(x,y,nnXLen);
              out << " " << movesOwnershipStdev[pos];
            }
          }
        }
      }

      if(args.showOwnership) {
        out << " ";
        out << "ownership";
        int nnXLen = search->nnXLen;
        for(int y = 0; y<board.y_size; y++) {
          for(int x = 0; x<board.x_size; x++) {
            int pos = NNPos::xyToPos(x,y,nnXLen);
            if(perspective == P_BLACK || (perspective != P_BLACK && perspective != P_WHITE && pla == P_BLACK))
              out << " " << -ownership[pos];
            else
              out << " " << ownership[pos];
          }
        }
      }

      if(args.showOwnershipStdev) {
        out << " ";
        out << "ownershipStdev";
        int nnXLen = search->nnXLen;
        for(int y = 0; y<board.y_size; y++) {
          for(int x = 0; x<board.x_size; x++) {
            int pos = NNPos::xyToPos(x,y,nnXLen);
            out << " " << ownershipStdev[pos];
          }
        }
      }

      cout << out.str() << endl;
    };
  }
  return callback;
}

void GTPEngine::genMove(
  Player pla,
  Logger& logger, double searchFactorWhenWinningThreshold, double searchFactorWhenWinning,
  enabled_t cleanupBeforePass, enabled_t friendlyPass, bool ogsChatToStderr,
  bool allowResignation, double resignThreshold, int resignConsecTurns, double resignMinScoreDifference,
  bool logSearchInfo, bool debug, bool playChosenMove,
  string& response, bool& responseIsError, bool& maybeStartPondering,
  double& winRate, double& scoreLead,
  AnalyzeArgs args
) {
  ClockTimer timer;

  response = "";
  responseIsError = false;
  maybeStartPondering = false;

  nnEval->clearStats();
  TimeControls tc = pla == P_BLACK ? bTimeControls : wTimeControls;

  //Update dynamic PDA given whatever the most recent values are, if we're using dynamic
  updateDynamicPDA();
  //Make sure we have the right parameters, in case someone ran analysis in the meantime.
  if(staticPDATakesPrecedence) {
    if(params.playoutDoublingAdvantage != staticPlayoutDoublingAdvantage) {
      params.playoutDoublingAdvantage = staticPlayoutDoublingAdvantage;
      bot->setParams(params);
    }
  }
  else {
    double desiredDynamicPDA =
      (params.playoutDoublingAdvantagePla == P_WHITE) ? desiredDynamicPDAForWhite :
      (params.playoutDoublingAdvantagePla == P_BLACK) ? -desiredDynamicPDAForWhite :
      (params.playoutDoublingAdvantagePla == C_EMPTY && pla == P_WHITE) ? desiredDynamicPDAForWhite :
      (params.playoutDoublingAdvantagePla == C_EMPTY && pla == P_BLACK) ? -desiredDynamicPDAForWhite :
      (assert(false),0.0);

    if(params.playoutDoublingAdvantage != desiredDynamicPDA) {
      params.playoutDoublingAdvantage = desiredDynamicPDA;
      bot->setParams(params);
    }
  }
  Player avoidMYTDaggerHackPla = avoidMYTDaggerHack ? pla : C_EMPTY;
  if(params.avoidMYTDaggerHackPla != avoidMYTDaggerHackPla) {
    params.avoidMYTDaggerHackPla = avoidMYTDaggerHackPla;
    bot->setParams(params);
  }
  if(params.wideRootNoise != genmoveWideRootNoise) {
    params.wideRootNoise = genmoveWideRootNoise;
    bot->setParams(params);
  }
  if(params.antiMirror != genmoveAntiMirror) {
    params.antiMirror = genmoveAntiMirror;
    bot->setParams(params);
  }

  {
    double avoidRepeatedPatternUtility = normalAvoidRepeatedPatternUtility;
    if(!args.analyzing) {
      double initialOppAdvantage = initialBlackAdvantage(bot->getRootHist()) * (pla == P_WHITE ? 1 : -1);
      if(initialOppAdvantage > getPointsThresholdForHandicapGame(getBoardSizeScaling(bot->getRootBoard())))
        avoidRepeatedPatternUtility = handicapAvoidRepeatedPatternUtility;
    }
    if(params.avoidRepeatedPatternUtility != avoidRepeatedPatternUtility) {
      params.avoidRepeatedPatternUtility = avoidRepeatedPatternUtility;
      bot->setParams(params);
    }
  }

  //Play faster when winning
  double searchFactor = PlayUtils::getSearchFactor(searchFactorWhenWinningThreshold,searchFactorWhenWinning,params,recentWinLossValues,pla);
  lastSearchFactor = searchFactor;

  Loc moveLoc;
  bot->setAvoidMoveUntilByLoc(args.avoidMoveUntilByLocBlack,args.avoidMoveUntilByLocWhite);
  if(args.analyzing) {
    std::function<void(const Search* search)> callback = getAnalyzeCallback(pla,args);
    if(args.showOwnership || args.showOwnershipStdev || args.showMovesOwnership || args.showMovesOwnershipStdev)
      bot->setAlwaysIncludeOwnerMap(true);
    else
      bot->setAlwaysIncludeOwnerMap(false);
    moveLoc = bot->genMoveSynchronousAnalyze(pla, tc, searchFactor, args.secondsPerReport, callback);
    //Make sure callback happens at least once
    callback(bot->getSearch());
  }
  else {
    moveLoc = bot->genMoveSynchronous(pla,tc,searchFactor);
  }

  bool isLegal = bot->isLegalStrict(moveLoc,pla);
  if(moveLoc == Board::NULL_LOC || !isLegal) {
    responseIsError = true;
    response = "genmove returned null location or illegal move";
    ostringstream sout;
    sout << "genmove null location or illegal move!?!" << "\n";
    sout << bot->getRootBoard() << "\n";
    sout << "Pla: " << PlayerIO::playerToString(pla) << "\n";
    sout << "MoveLoc: " << Location::toString(moveLoc,bot->getRootBoard()) << "\n";
    logger.write(sout.str());
    genmoveTimeSum += timer.getSeconds();
    return;
  }

  ReportedSearchValues values;
  double winLossValue;
  double lead;
  {
    values = bot->getSearch()->getRootValuesRequireSuccess();
    winLossValue = values.winLossValue;
    lead = values.lead;
  }

  //Record data for resignation or adjusting handicap behavior ------------------------
  recentWinLossValues.push_back(winLossValue);

  //Decide whether we should resign---------------------
  bool resigned = allowResignation && shouldResign(
    bot->getRootBoard(),bot->getRootHist(),pla,recentWinLossValues,lead,
    resignThreshold,resignConsecTurns,resignMinScoreDifference
  );


  //Snapshot the time NOW - all meaningful play-related computation time is done, the rest is just
  //output of various things.
  double timeTaken = timer.getSeconds();
  genmoveTimeSum += timeTaken;

  //Chatting and logging ----------------------------

  //if(ogsChatToStderr) {
    int64_t visits = bot->getSearch()->getRootVisits();
    double winrate = 0.5 * (1.0 + (values.winValue - values.lossValue));
    double leadForPrinting = lead;
    //Print winrate from desired perspective
    if(perspective == P_BLACK || (perspective != P_BLACK && perspective != P_WHITE && pla == P_BLACK)) {
      winrate = 1.0 - winrate;
      leadForPrinting = -leadForPrinting;
    }
  if(ogsChatToStderr) {
    cerr << "CHAT:"
         << "Visits " << visits
         << " Winrate " << Global::strprintf("%.2f%%", winrate * 100.0)
         << " ScoreLead " << Global::strprintf("%.1f", leadForPrinting)
         << " ScoreStdev " << Global::strprintf("%.1f", values.expectedScoreStdev);
    if(params.playoutDoublingAdvantage != 0.0) {
      cerr << Global::strprintf(
        " (PDA %.2f)",
        bot->getSearch()->getRootPla() == getOpp(params.playoutDoublingAdvantagePla) ?
        -params.playoutDoublingAdvantage : params.playoutDoublingAdvantage);
    }
    cerr << " PV ";
    bot->getSearch()->printPVForMove(cerr,bot->getSearch()->rootNode, moveLoc, analysisPVLen);
    cerr << endl;
  }

  if(logSearchInfo) {
    ostringstream sout;
    PlayUtils::printGenmoveLog(sout,bot,nnEval,moveLoc,timeTaken,perspective);
    logger.write(sout.str());
  }
  if(debug) {
    PlayUtils::printGenmoveLog(cerr,bot,nnEval,moveLoc,timeTaken,perspective);
  }

  //Hacks--------------------------------------------------
  //At least one of these hacks will use the bot to search stuff and clears its tree, so we apply them AFTER
  //all relevant logging and stuff.

  //Implement friendly pass - in area scoring rules other than tromp-taylor, maybe pass once there are no points
  //left to gain.
  int64_t numVisitsForFriendlyPass = 8 + std::min((int64_t)1000, std::min(params.maxVisits, params.maxPlayouts) / 10);
  moveLoc = PlayUtils::maybeFriendlyPass(cleanupBeforePass, friendlyPass, pla, moveLoc, bot->getSearchStopAndWait(), numVisitsForFriendlyPass);

  //Implement cleanupBeforePass hack - if the bot wants to pass, instead cleanup if there is something to clean
  //and we are in a ruleset where this is necessary or the user has configured it.
  moveLoc = PlayUtils::maybeCleanupBeforePass(cleanupBeforePass, friendlyPass, pla, moveLoc, bot);

  //Actual reporting of chosen move---------------------
  if(resigned)
    response = "resign";
  else
    response = Location::toString(moveLoc,bot->getRootBoard());

  if(!resigned && moveLoc != Board::NULL_LOC && isLegal && playChosenMove) {
    bool suc = bot->makeMove(moveLoc,pla,preventEncore);
    if(suc)
      moveHistory.push_back(Move(moveLoc,pla));
    assert(suc);
    (void)suc; //Avoid warning when asserts are off

    maybeStartPondering = true;
  }

  if(args.analyzing) {
    response = "play " + response;
  }

  winRate = winrate;
  scoreLead = leadForPrinting;

  return;
}

void GTPEngine::clearCache() {
  bot->clearSearch();
  nnEval->clearCache();
}

void GTPEngine::placeFixedHandicap(int n, string& response, bool& responseIsError) {
  int xSize = bot->getRootBoard().x_size;
  int ySize = bot->getRootBoard().y_size;
  Board board(xSize,ySize);
  try {
    PlayUtils::placeFixedHandicap(board,n);
  }
  catch(const StringError& e) {
    responseIsError = true;
    response = string(e.what()) + ", try place_free_handicap";
    return;
  }
  assert(bot->getRootHist().rules == currentRules);

  Player pla = P_BLACK;
  BoardHistory hist(board,pla,currentRules,0);

  //Also switch the initial player, expecting white should be next.
  hist.clear(board,P_WHITE,currentRules,0);
  hist.setAssumeMultipleStartingBlackMovesAreHandicap(assumeMultipleStartingBlackMovesAreHandicap);
  hist.setInitialTurnNumber(board.numStonesOnBoard()); //Should give more accurate temperaure and time control behavior
  pla = P_WHITE;

  response = "";
  for(int y = 0; y<board.y_size; y++) {
    for(int x = 0; x<board.x_size; x++) {
      Loc loc = Location::getLoc(x,y,board.x_size);
      if(board.colors[loc] != C_EMPTY) {
        response += " " + Location::toString(loc,board);
      }
    }
  }
  response = Global::trim(response);
  (void)responseIsError;

  vector<Move> newMoveHistory;
  setPositionAndRules(pla,board,hist,board,pla,newMoveHistory);
  clearStatsForNewGame();
}

void GTPEngine::placeFreeHandicap(int n, string& response, bool& responseIsError, Rand& rand) {
  stopAndWait();

  //If asked to place more, we just go ahead and only place up to 30, or a quarter of the board
  int xSize = bot->getRootBoard().x_size;
  int ySize = bot->getRootBoard().y_size;
  int maxHandicap = xSize*ySize / 4;
  if(maxHandicap > 30)
    maxHandicap = 30;
  if(n > maxHandicap)
    n = maxHandicap;

  assert(bot->getRootHist().rules == currentRules);

  Board board(xSize,ySize);
  Player pla = P_BLACK;
  BoardHistory hist(board,pla,currentRules,0);
  double extraBlackTemperature = 0.25;
  PlayUtils::playExtraBlack(bot->getSearchStopAndWait(), n, board, hist, extraBlackTemperature, rand);
  //Also switch the initial player, expecting white should be next.
  hist.clear(board,P_WHITE,currentRules,0);
  hist.setAssumeMultipleStartingBlackMovesAreHandicap(assumeMultipleStartingBlackMovesAreHandicap);
  hist.setInitialTurnNumber(board.numStonesOnBoard()); //Should give more accurate temperaure and time control behavior
  pla = P_WHITE;

  response = "";
  for(int y = 0; y<board.y_size; y++) {
    for(int x = 0; x<board.x_size; x++) {
      Loc loc = Location::getLoc(x,y,board.x_size);
      if(board.colors[loc] != C_EMPTY) {
        response += " " + Location::toString(loc,board);
      }
    }
  }
  response = Global::trim(response);
  (void)responseIsError;

  vector<Move> newMoveHistory;
  setPositionAndRules(pla,board,hist,board,pla,newMoveHistory);
  clearStatsForNewGame();
}

void GTPEngine::analyze(Player pla, AnalyzeArgs args) {
  assert(args.analyzing);
  //Analysis should ALWAYS be with the static value to prevent random hard-to-predict changes
  //for users.
  if(params.playoutDoublingAdvantage != staticPlayoutDoublingAdvantage) {
    params.playoutDoublingAdvantage = staticPlayoutDoublingAdvantage;
    bot->setParams(params);
  }
  if(params.avoidMYTDaggerHackPla != C_EMPTY) {
    params.avoidMYTDaggerHackPla = C_EMPTY;
    bot->setParams(params);
  }
  //Also wide root, if desired
  if(params.wideRootNoise != analysisWideRootNoise) {
    params.wideRootNoise = analysisWideRootNoise;
    bot->setParams(params);
  }
  if(params.antiMirror != analysisAntiMirror) {
    params.antiMirror = analysisAntiMirror;
    bot->setParams(params);
  }

  std::function<void(const Search* search)> callback = getAnalyzeCallback(pla,args);
  bot->setAvoidMoveUntilByLoc(args.avoidMoveUntilByLocBlack,args.avoidMoveUntilByLocWhite);
  if(args.showOwnership || args.showOwnershipStdev || args.showMovesOwnership || args.showMovesOwnershipStdev)
    bot->setAlwaysIncludeOwnerMap(true);
  else
    bot->setAlwaysIncludeOwnerMap(false);

  double searchFactor = 1e40; //go basically forever
  bot->analyzeAsync(pla, searchFactor, args.secondsPerReport, callback);
}

void GTPEngine::computeAnticipatedWinnerAndScore(Player& winner, double& finalWhiteMinusBlackScore) {
  stopAndWait();

  //No playoutDoublingAdvantage to avoid bias
  //Also never assume the game will end abruptly due to pass
  {
    SearchParams tmpParams = params;
    tmpParams.playoutDoublingAdvantage = 0.0;
    tmpParams.conservativePass = true;
    bot->setParams(tmpParams);
  }

  //Make absolutely sure we can restore the bot's old state
  const Player oldPla = bot->getRootPla();
  const Board oldBoard = bot->getRootBoard();
  const BoardHistory oldHist = bot->getRootHist();

  Board board = bot->getRootBoard();
  BoardHistory hist = bot->getRootHist();
  Player pla = bot->getRootPla();

  //Tromp-taylorish scoring, or finished territory game scoring (including noresult)
  if(hist.isGameFinished && (
       (hist.rules.scoringRule == Rules::SCORING_AREA && !hist.rules.friendlyPassOk) ||
       (hist.rules.scoringRule == Rules::SCORING_TERRITORY)
     )
  ) {
    //For GTP purposes, we treat noResult as a draw since there is no provision for anything else.
    winner = hist.winner;
    finalWhiteMinusBlackScore = hist.finalWhiteMinusBlackScore;
  }
  //Human-friendly score or incomplete game score estimation
  else {
    int64_t numVisits = std::max(50, params.numThreads * 10);
    //Try computing the lead for white
    double lead = PlayUtils::computeLead(bot->getSearchStopAndWait(),NULL,board,hist,pla,numVisits,OtherGameProperties());

    //Round lead to nearest integer or half-integer
    if(hist.rules.gameResultWillBeInteger())
      lead = round(lead);
    else
      lead = round(lead+0.5)-0.5;

    finalWhiteMinusBlackScore = lead;
    winner = lead > 0 ? P_WHITE : lead < 0 ? P_BLACK : C_EMPTY;
  }

  //Restore
  bot->setPosition(oldPla,oldBoard,oldHist);
  bot->setParams(params);
}

vector<bool> GTPEngine::computeAnticipatedStatuses() {
  stopAndWait();

  //Make absolutely sure we can restore the bot's old state
  const Player oldPla = bot->getRootPla();
  const Board oldBoard = bot->getRootBoard();
  const BoardHistory oldHist = bot->getRootHist();

  Board board = bot->getRootBoard();
  BoardHistory hist = bot->getRootHist();
  Player pla = bot->getRootPla();

  int64_t numVisits = std::max(100, params.numThreads * 20);
  vector<bool> isAlive;
  //Tromp-taylorish statuses, or finished territory game statuses (including noresult)
  if(hist.isGameFinished && (
       (hist.rules.scoringRule == Rules::SCORING_AREA && !hist.rules.friendlyPassOk) ||
       (hist.rules.scoringRule == Rules::SCORING_TERRITORY)
     )
  )
    isAlive = PlayUtils::computeAnticipatedStatusesSimple(board,hist);
  //Human-friendly statuses or incomplete game status estimation
  else {
    vector<double> ownershipsBuf;
    isAlive = PlayUtils::computeAnticipatedStatusesWithOwnership(bot->getSearchStopAndWait(),board,hist,pla,numVisits,ownershipsBuf);
  }

  //Restore
  bot->setPosition(oldPla,oldBoard,oldHist);

  return isAlive;
}

string GTPEngine::rawNN(int whichSymmetry) {
  if(nnEval == NULL)
    return "";
  ostringstream out;

  for(int symmetry = 0; symmetry < SymmetryHelpers::NUM_SYMMETRIES; symmetry++) {
    if(whichSymmetry == NNInputs::SYMMETRY_ALL || whichSymmetry == symmetry) {
      Board board = bot->getRootBoard();
      BoardHistory hist = bot->getRootHist();
      Player nextPla = bot->getRootPla();

      MiscNNInputParams nnInputParams;
      nnInputParams.playoutDoublingAdvantage =
        (params.playoutDoublingAdvantagePla == C_EMPTY || params.playoutDoublingAdvantagePla == nextPla) ?
        staticPlayoutDoublingAdvantage : -staticPlayoutDoublingAdvantage;
      nnInputParams.symmetry = symmetry;
      NNResultBuf buf;
      bool skipCache = true;
      bool includeOwnerMap = true;
      nnEval->evaluate(board,hist,nextPla,nnInputParams,buf,skipCache,includeOwnerMap);

      NNOutput* nnOutput = buf.result.get();
      out << "symmetry " << symmetry << endl;
      out << "whiteWin " << Global::strprintf("%.6f",nnOutput->whiteWinProb) << endl;
      out << "whiteLoss " << Global::strprintf("%.6f",nnOutput->whiteLossProb) << endl;
      out << "noResult " << Global::strprintf("%.6f",nnOutput->whiteNoResultProb) << endl;
      out << "whiteLead " << Global::strprintf("%.3f",nnOutput->whiteLead) << endl;
      out << "whiteScoreSelfplay " << Global::strprintf("%.3f",nnOutput->whiteScoreMean) << endl;
      out << "whiteScoreSelfplaySq " << Global::strprintf("%.3f",nnOutput->whiteScoreMeanSq) << endl;
      out << "varTimeLeft " << Global::strprintf("%.3f",nnOutput->varTimeLeft) << endl;
      out << "shorttermWinlossError " << Global::strprintf("%.3f",nnOutput->shorttermWinlossError) << endl;
      out << "shorttermScoreError " << Global::strprintf("%.3f",nnOutput->shorttermScoreError) << endl;

      out << "policy" << endl;
      for(int y = 0; y<board.y_size; y++) {
        for(int x = 0; x<board.x_size; x++) {
          int pos = NNPos::xyToPos(x,y,nnOutput->nnXLen);
          float prob = nnOutput->policyProbs[pos];
          if(prob < 0)
            out << "    NAN ";
          else
            out << Global::strprintf("%8.6f ", prob);
        }
        out << endl;
      }
      out << "policyPass ";
      {
        int pos = NNPos::locToPos(Board::PASS_LOC,board.x_size,nnOutput->nnXLen,nnOutput->nnYLen);
        float prob = nnOutput->policyProbs[pos];
        if(prob < 0)
          out << "    NAN "; // Probably shouldn't ever happen for pass unles the rules change, but we handle it anyways
        else
          out << Global::strprintf("%8.6f ", prob);
        out << endl;
      }

      out << "whiteOwnership" << endl;
      for(int y = 0; y<board.y_size; y++) {
        for(int x = 0; x<board.x_size; x++) {
          int pos = NNPos::xyToPos(x,y,nnOutput->nnXLen);
          float whiteOwn = nnOutput->whiteOwnerMap[pos];
          out << Global::strprintf("%9.7f ", whiteOwn);
        }
        out << endl;
      }
      out << endl;
    }
  }

  return Global::trim(out.str());
}

SearchParams GTPEngine::getParams() {
  return params;
}

void GTPEngine::setParams(SearchParams p) {
  params = p;
  bot->setParams(params);
}

std::vector<std::pair<float, int> > GTPEngine::get_policy() {
  std::vector<std::pair<float, int> > result;

  if(nnEval == NULL)
    return result;

  bool first = true;
  for(int symmetry = 0; symmetry < SymmetryHelpers::NUM_SYMMETRIES; symmetry++) {
    Board board = bot->getRootBoard();
    BoardHistory hist = bot->getRootHist();
    Player nextPla = bot->getRootPla();

    MiscNNInputParams nnInputParams;
    nnInputParams.playoutDoublingAdvantage =
      (params.playoutDoublingAdvantagePla == C_EMPTY || params.playoutDoublingAdvantagePla == nextPla) ?
      staticPlayoutDoublingAdvantage : -staticPlayoutDoublingAdvantage;
    nnInputParams.symmetry = symmetry;
    NNResultBuf buf;
    bool skipCache = true;
    bool includeOwnerMap = false;
    nnEval->evaluate(board,hist,nextPla,nnInputParams,buf,skipCache,includeOwnerMap);

    NNOutput* nnOutput = buf.result.get();
    int result_i = 0;
    for(int y = 0; y<board.y_size; y++) {
      for(int x = 0; x<board.x_size; x++) {
        int pos = NNPos::xyToPos(x,y,nnOutput->nnXLen);
        float prob = nnOutput->policyProbs[pos];
        if(first)
          result.emplace_back(prob / SymmetryHelpers::NUM_SYMMETRIES, pos);
        else
          result[result_i].first += prob / SymmetryHelpers::NUM_SYMMETRIES;
        result_i++;
      }
    }
    first = false;
  }
  return result;
}

std::vector<float> GTPEngine::get_owner() {
  std::vector<float> result;
  if(nnEval == NULL)
    return result;

  bool first = true;
  for(int symmetry = 0; symmetry < SymmetryHelpers::NUM_SYMMETRIES; symmetry++) {
    Board board = bot->getRootBoard();
    BoardHistory hist = bot->getRootHist();
    Player nextPla = bot->getRootPla();

    MiscNNInputParams nnInputParams;
    nnInputParams.playoutDoublingAdvantage =
      (params.playoutDoublingAdvantagePla == C_EMPTY || params.playoutDoublingAdvantagePla == nextPla) ?
      staticPlayoutDoublingAdvantage : -staticPlayoutDoublingAdvantage;
    nnInputParams.symmetry = symmetry;
    NNResultBuf buf;
    bool skipCache = false;
    bool includeOwnerMap = true;
    nnEval->evaluate(board,hist,nextPla,nnInputParams,buf,skipCache,includeOwnerMap);

    NNOutput* nnOutput = buf.result.get();
    int result_i = 0;
    for(int y = 0; y<board.y_size; y++) {
      for(int x = 0; x<board.x_size; x++) {
        int pos = NNPos::xyToPos(x,y,nnOutput->nnXLen);
        float whiteOwn = nnOutput->whiteOwnerMap[pos];
        if(first)
          result.emplace_back(whiteOwn / SymmetryHelpers::NUM_SYMMETRIES);
        else
          result[result_i] += whiteOwn / SymmetryHelpers::NUM_SYMMETRIES;
        result_i++;
      }
    }
    first = false;
  }
  return result;
}

//User should pre-fill pla with a default value, as it will not get filled in if the parsed command doesn't specify
static GTPEngine::AnalyzeArgs parseAnalyzeCommand(
  const string& command,
  const vector<string>& pieces,
  Player& pla,
  bool& parseFailed,
  GTPEngine* engine
) {
  int numArgsParsed = 0;

  bool isLZ = (command == "lz-analyze" || command == "lz-genmove_analyze");
  bool isKata = (command == "kata-analyze" || command == "kata-genmove_analyze");
  double lzAnalyzeInterval = TimeControls::UNLIMITED_TIME_DEFAULT;
  int minMoves = 0;
  int maxMoves = 10000000;
  bool showOwnership = false;
  bool showOwnershipStdev = false;
  bool showMovesOwnership = false;
  bool showMovesOwnershipStdev = false;
  bool showPVVisits = false;
  bool showPVEdgeVisits = false;
  vector<int> avoidMoveUntilByLocBlack;
  vector<int> avoidMoveUntilByLocWhite;
  bool gotAvoidMovesBlack = false;
  bool gotAllowMovesBlack = false;
  bool gotAvoidMovesWhite = false;
  bool gotAllowMovesWhite = false;

  parseFailed = false;

  //Format:
  //lz-analyze [optional player] [optional interval float] <keys and values>
  //Keys and values consists of zero or more of:

  //interval <float interval in centiseconds>
  //avoid <player> <comma-separated moves> <until movenum>
  //minmoves <int min number of moves to show>
  //maxmoves <int max number of moves to show>
  //ownership <bool whether to show ownership or not>
  //ownershipStdev <bool whether to show ownershipStdev or not>
  //pvVisits <bool whether to show pvVisits or not>
  //pvEdgeVisits <bool whether to show pvEdgeVisits or not>

  //Parse optional player
  if(pieces.size() > numArgsParsed && PlayerIO::tryParsePlayer(pieces[numArgsParsed],pla))
    numArgsParsed += 1;

  //Parse optional interval float
  if(pieces.size() > numArgsParsed &&
     Global::tryStringToDouble(pieces[numArgsParsed],lzAnalyzeInterval) &&
     !isnan(lzAnalyzeInterval) && lzAnalyzeInterval >= 0 && lzAnalyzeInterval < TimeControls::MAX_USER_INPUT_TIME)
    numArgsParsed += 1;

  //Now loop and handle all key value pairs
  while(pieces.size() > numArgsParsed) {
    const string& key = pieces[numArgsParsed];
    numArgsParsed += 1;
    //Make sure we have a value. If not, then we fail.
    if(pieces.size() <= numArgsParsed) {
      parseFailed = true;
      break;
    }

    const string& value = pieces[numArgsParsed];
    numArgsParsed += 1;

    if(key == "interval" && Global::tryStringToDouble(value,lzAnalyzeInterval) &&
       !isnan(lzAnalyzeInterval) && lzAnalyzeInterval >= 0 && lzAnalyzeInterval < TimeControls::MAX_USER_INPUT_TIME) {
      continue;
    }
    else if(key == "avoid" || key == "allow") {
      //Parse two more arguments
      if(pieces.size() < numArgsParsed+2) {
        parseFailed = true;
        break;
      }
      const string& movesStr = pieces[numArgsParsed];
      numArgsParsed += 1;
      const string& untilDepthStr = pieces[numArgsParsed];
      numArgsParsed += 1;

      int untilDepth = -1;
      if(!Global::tryStringToInt(untilDepthStr,untilDepth) || untilDepth < 1) {
        parseFailed = true;
        break;
      }
      Player avoidPla = C_EMPTY;
      if(!PlayerIO::tryParsePlayer(value,avoidPla)) {
        parseFailed = true;
        break;
      }
      vector<Loc> parsedLocs;
      vector<string> locPieces = Global::split(movesStr,',');
      for(size_t i = 0; i<locPieces.size(); i++) {
        string s = Global::trim(locPieces[i]);
        if(s.size() <= 0)
          continue;
        Loc loc;
        if(!tryParseLoc(s,engine->bot->getRootBoard(),loc)) {
          parseFailed = true;
          break;
        }
        parsedLocs.push_back(loc);
      }
      if(parseFailed)
        break;

      //Make sure the same analyze command can't specify both avoid and allow, and allow at most one allow.
      vector<int>& avoidMoveUntilByLoc = avoidPla == P_BLACK ? avoidMoveUntilByLocBlack : avoidMoveUntilByLocWhite;
      bool& gotAvoidMoves = avoidPla == P_BLACK ? gotAvoidMovesBlack : gotAvoidMovesWhite;
      bool& gotAllowMoves = avoidPla == P_BLACK ? gotAllowMovesBlack : gotAllowMovesWhite;
      if((key == "allow" && gotAvoidMoves) || (key == "allow" && gotAllowMoves) || (key == "avoid" && gotAllowMoves)) {
        parseFailed = true;
        break;
      }
      avoidMoveUntilByLoc.resize(Board::MAX_ARR_SIZE);
      if(key == "allow") {
        std::fill(avoidMoveUntilByLoc.begin(),avoidMoveUntilByLoc.end(),untilDepth);
        for(Loc loc: parsedLocs) {
          avoidMoveUntilByLoc[loc] = 0;
        }
      }
      else {
        for(Loc loc: parsedLocs) {
          avoidMoveUntilByLoc[loc] = untilDepth;
        }
      }
      gotAvoidMoves |= (key == "avoid");
      gotAllowMoves |= (key == "allow");

      continue;
    }
    else if(key == "minmoves" && Global::tryStringToInt(value,minMoves) &&
            minMoves >= 0 && minMoves < 1000000000) {
      continue;
    }
    else if(key == "maxmoves" && Global::tryStringToInt(value,maxMoves) &&
            maxMoves >= 0 && maxMoves < 1000000000) {
      continue;
    }
    else if(isKata && key == "ownership" && Global::tryStringToBool(value,showOwnership)) {
      continue;
    }
    else if(isKata && key == "ownershipStdev" && Global::tryStringToBool(value,showOwnershipStdev)) {
      continue;
    }
    else if(isKata && key == "movesOwnership" && Global::tryStringToBool(value,showMovesOwnership)) {
      continue;
    }
    else if(isKata && key == "movesOwnershipStdev" && Global::tryStringToBool(value,showMovesOwnershipStdev)) {
      continue;
    }
    else if(isKata && key == "pvVisits" && Global::tryStringToBool(value,showPVVisits)) {
      continue;
    }
    else if(isKata && key == "pvEdgeVisits" && Global::tryStringToBool(value,showPVEdgeVisits)) {
      continue;
    }

    parseFailed = true;
    break;
  }

  GTPEngine::AnalyzeArgs args = GTPEngine::AnalyzeArgs();
  args.analyzing = true;
  args.lz = isLZ;
  args.kata = isKata;
  //Convert from centiseconds to seconds
  args.secondsPerReport = lzAnalyzeInterval * 0.01;
  args.minMoves = minMoves;
  args.maxMoves = maxMoves;
  args.showOwnership = showOwnership;
  args.showOwnershipStdev = showOwnershipStdev;
  args.showMovesOwnership = showMovesOwnership;
  args.showMovesOwnershipStdev = showMovesOwnershipStdev;
  args.showPVVisits = showPVVisits;
  args.showPVEdgeVisits = showPVEdgeVisits;
  args.avoidMoveUntilByLocBlack = avoidMoveUntilByLocBlack;
  args.avoidMoveUntilByLocWhite = avoidMoveUntilByLocWhite;
  return args;
}

GTPKata::GTPKata(const vector<string>& args) {
  engine = nullptr;
  Board::initHash();
  ScoreValue::initTables();
  //Rand seedRand;

  //ConfigParser cfg;
  //string nnModelFile;
  //string overrideVersion;
  KataGoCommandLine cmd("Run KataGo main GTP engine for playing games or casual analysis.");
  try {
    cmd.addConfigFileArg(KataGoCommandLine::defaultGtpConfigFileName(), "gtp_example.cfg");
    //cmd.addModelFileArg();
    cmd.setShortUsageArgLimit();
    cmd.addOverrideConfigArg();

    TCLAP::ValueArg<string> overrideVersionArg("", "override-version", "Force KataGo to say a certain value in response to gtp version command", false, string(), "VERSION");
    cmd.add(overrideVersionArg);
    cmd.parseArgs(args);
    //nnModelFile = cmd.getModelFile();
    overrideVersion = overrideVersionArg.getValue();

    cmd.getConfig(cfg);
  }
  catch (TCLAP::ArgException& e) {
    cerr << "Error: " << e.error() << " for argument " << e.argId() << endl;
    throw StringError(_("Argument error when starting KataGo"));
    //return 1;
  }

  if (cfg.contains("modelFile")) {
    nnModelFile = cfg.getString("modelFile");
    FILE* fp = fopen(nnModelFile.c_str(), "r");
    if (fp == NULL) {
      throw StringError(_("Key 'modelFile'(") + nnModelFile + _(") dose not exist"));
    } else {
      fclose(fp);
    }
  } else {
    throw StringError(_("Key 'modelFile' not specified in config file ") + KataGoCommandLine::defaultGtpConfigFileName());
  }

  //Logger logger;
  if (cfg.contains("logFile") && cfg.contains("logDir"))
    throw StringError(_("Cannot specify both logFile and logDir in config"));
  else if (cfg.contains("logFile"))
    logger.addFile(cfg.getString("logFile"));
  else if (cfg.contains("logDir")) {
    MakeDir::make(cfg.getString("logDir"));
    Rand rand;
    logger.addFile(cfg.getString("logDir") + "/" + DateTime::getCompactDateTimeString() + "-" + Global::uint32ToHexString(rand.nextUInt()) + ".log");
  }

  /*const bool */logAllGTPCommunication = cfg.getBool("logAllGTPCommunication");
  /*const bool */logSearchInfo = cfg.getBool("logSearchInfo");
  /*bool */loggingToStderr = false;

  /*const bool */logTimeStamp = cfg.contains("logTimeStamp") ? cfg.getBool("logTimeStamp") : true;
  if (!logTimeStamp)
    logger.setLogTime(false);

  /*bool */startupPrintMessageToStderr = true;
  if (cfg.contains("startupPrintMessageToStderr"))
    startupPrintMessageToStderr = cfg.getBool("startupPrintMessageToStderr");

  if (cfg.contains("logToStderr") && cfg.getBool("logToStderr")) {
    loggingToStderr = true;
    logger.setLogToStderr(true);
  }

  logger.write("GTP Engine starting...");
  logger.write(Version::getKataGoVersionForHelp());
  //Also check loggingToStderr so that we don't duplicate the message from the log file
  if (startupPrintMessageToStderr && !loggingToStderr) {
    cerr << Version::getKataGoVersionForHelp() << endl;
  }

  //Defaults to 7.5 komi, gtp will generally override this
  /*const bool */loadKomiFromCfg = false;
  /*Rules */initialRules = Setup::loadSingleRules(cfg, loadKomiFromCfg);
  logger.write("Using " + initialRules.toStringNoKomiMaybeNice() + " rules initially, unless GTP/GUI overrides this");
  if (startupPrintMessageToStderr && !loggingToStderr) {
    cerr << "Using " + initialRules.toStringNoKomiMaybeNice() + " rules initially, unless GTP/GUI overrides this" << endl;
  }
  /*bool */isForcingKomi = false;
  /*float */forcedKomi = 0;
  if (cfg.contains("ignoreGTPAndForceKomi")) {
    isForcingKomi = true;
    forcedKomi = cfg.getFloat("ignoreGTPAndForceKomi", Rules::MIN_USER_KOMI, Rules::MAX_USER_KOMI);
    initialRules.komi = forcedKomi;
  }

  /*SearchParams */initialParams = Setup::loadSingleParams(cfg, Setup::SETUP_FOR_GTP);
  logger.write("Using " + Global::intToString(initialParams.numThreads) + " CPU thread(s) for search");
  //Set a default for conservativePass that differs from matches or selfplay
  if (!cfg.contains("conservativePass") && !cfg.contains("conservativePass0"))
    initialParams.conservativePass = true;
  if (!cfg.contains("fillDameBeforePass") && !cfg.contains("fillDameBeforePass0"))
    initialParams.fillDameBeforePass = true;

  /*const bool */ponderingEnabled = cfg.getBool("ponderingEnabled");

  /*const enabled_t */cleanupBeforePass = cfg.contains("cleanupBeforePass") ? cfg.getEnabled("cleanupBeforePass") : enabled_t::Auto;
  /*const enabled_t */friendlyPass = cfg.contains("friendlyPass") ? cfg.getEnabled("friendlyPass") : enabled_t::Auto;
  if (cleanupBeforePass == enabled_t::True && friendlyPass == enabled_t::True)
    throw StringError(_("Cannot specify both cleanupBeforePass = true and friendlyPass = true at the same time"));

  /*const bool */allowResignation = cfg.contains("allowResignation") ? cfg.getBool("allowResignation") : false;
  /*const double */resignThreshold = cfg.contains("allowResignation") ? cfg.getDouble("resignThreshold", -1.0, 0.0) : -1.0; //Threshold on [-1,1], regardless of winLossUtilityFactor
  /*const int */resignConsecTurns = cfg.contains("resignConsecTurns") ? cfg.getInt("resignConsecTurns", 1, 100) : 3;
  /*const double */resignMinScoreDifference = cfg.contains("resignMinScoreDifference") ? cfg.getDouble("resignMinScoreDifference", 0.0, 1000.0) : -1e10;

  Setup::initializeSession(cfg);

  /*const double */searchFactorWhenWinning = cfg.contains("searchFactorWhenWinning") ? cfg.getDouble("searchFactorWhenWinning", 0.01, 1.0) : 1.0;
  /*const double */searchFactorWhenWinningThreshold = cfg.contains("searchFactorWhenWinningThreshold") ? cfg.getDouble("searchFactorWhenWinningThreshold", 0.0, 1.0) : 1.0;
  /*const bool */ogsChatToStderr = cfg.contains("ogsChatToStderr") ? cfg.getBool("ogsChatToStderr") : false;
  /*const int */analysisPVLen = cfg.contains("analysisPVLen") ? cfg.getInt("analysisPVLen", 1, 1000) : 13;
  /*const bool */assumeMultipleStartingBlackMovesAreHandicap =
    cfg.contains("assumeMultipleStartingBlackMovesAreHandicap") ? cfg.getBool("assumeMultipleStartingBlackMovesAreHandicap") : true;
  /*const bool */preventEncore = cfg.contains("preventCleanupPhase") ? cfg.getBool("preventCleanupPhase") : true;
  /*const double */dynamicPlayoutDoublingAdvantageCapPerOppLead =
    cfg.contains("dynamicPlayoutDoublingAdvantageCapPerOppLead") ? cfg.getDouble("dynamicPlayoutDoublingAdvantageCapPerOppLead", 0.0, 0.5) : 0.045;
  /*double */staticPlayoutDoublingAdvantage = initialParams.playoutDoublingAdvantage;
  /*const bool */staticPDATakesPrecedence = cfg.contains("playoutDoublingAdvantage") && !cfg.contains("dynamicPlayoutDoublingAdvantageCapPerOppLead");
  /*const bool */avoidMYTDaggerHack = cfg.contains("avoidMYTDaggerHack") ? cfg.getBool("avoidMYTDaggerHack") : false;
  /*const double */normalAvoidRepeatedPatternUtility = initialParams.avoidRepeatedPatternUtility;
  /*const double */handicapAvoidRepeatedPatternUtility = (cfg.contains("avoidRepeatedPatternUtility") || cfg.contains("avoidRepeatedPatternUtility0")) ?
    initialParams.avoidRepeatedPatternUtility : 0.005;

  /*int */defaultBoardXSize = -1;
  /*int */defaultBoardYSize = -1;
  Setup::loadDefaultBoardXYSize(cfg, logger, defaultBoardXSize, defaultBoardYSize);

  /*const bool */forDeterministicTesting =
    cfg.contains("forDeterministicTesting") ? cfg.getBool("forDeterministicTesting") : false;

  if (forDeterministicTesting)
    seedRand.init("forDeterministicTesting");

  /*const double */genmoveWideRootNoise = initialParams.wideRootNoise;
  /*const double */analysisWideRootNoise =
    cfg.contains("analysisWideRootNoise") ? cfg.getDouble("analysisWideRootNoise", 0.0, 5.0) : Setup::DEFAULT_ANALYSIS_WIDE_ROOT_NOISE;
  /*const bool */analysisAntiMirror = initialParams.antiMirror;
  /*const bool */genmoveAntiMirror =
    cfg.contains("genmoveAntiMirror") ? cfg.getBool("genmoveAntiMirror") : cfg.contains("antiMirror") ? cfg.getBool("antiMirror") : true;

  /*std::unique_ptr<PatternBonusTable> */patternBonusTable = nullptr;
  {
    std::vector<std::unique_ptr<PatternBonusTable>> tables = Setup::loadAvoidSgfPatternBonusTables(cfg, logger);
    assert(tables.size() == 1);
    patternBonusTable = std::move(tables[0]);
  }

  /*Player */perspective = Setup::parseReportAnalysisWinrates(cfg, C_EMPTY);

  engine = new GTPEngine(
    nnModelFile, initialParams, initialRules,
    assumeMultipleStartingBlackMovesAreHandicap, preventEncore,
    dynamicPlayoutDoublingAdvantageCapPerOppLead,
    staticPlayoutDoublingAdvantage, staticPDATakesPrecedence,
    normalAvoidRepeatedPatternUtility, handicapAvoidRepeatedPatternUtility,
    avoidMYTDaggerHack,
    genmoveWideRootNoise, analysisWideRootNoise,
    genmoveAntiMirror, analysisAntiMirror,
    perspective, analysisPVLen,
    std::move(patternBonusTable)
  );
  engine->setOrResetBoardSize(cfg, logger, seedRand, defaultBoardXSize, defaultBoardYSize, loggingToStderr);

  //If nobody specified any time limit in any way, then assume a relatively fast time control
  if (!cfg.contains("maxPlayouts") && !cfg.contains("maxVisits") && !cfg.contains("maxTime")) {
    double mainTime = 1.0;
    double byoYomiTime = 5.0;
    int byoYomiPeriods = 5;
    TimeControls tc = TimeControls::canadianOrByoYomiTime(mainTime, byoYomiTime, byoYomiPeriods, 1);
    engine->bTimeControls = tc;
    engine->wTimeControls = tc;
  }

  //Check for unused config keys
  cfg.warnUnusedKeys(cerr, &logger);

  logger.write("Loaded config " + cfg.getFileName());
  logger.write("Loaded model " + nnModelFile);
  cmd.logOverrides(logger);
  logger.write("Model name: " + (engine->nnEval == NULL ? string() : engine->nnEval->getInternalModelName()));
  logger.write("GTP ready, beginning main protocol loop");
  //Also check loggingToStderr so that we don't duplicate the message from the log file
  if (startupPrintMessageToStderr && !loggingToStderr) {
    cerr << "Loaded config " << cfg.getFileName() << endl;
    cerr << "Loaded model " << nnModelFile << endl;
    cerr << "Model name: " + (engine->nnEval == NULL ? string() : engine->nnEval->getInternalModelName()) << endl;
    cerr << "GTP ready, beginning main protocol loop" << endl;
  }
}

GTPKata::~GTPKata() {
    if (engine) {
        delete engine;
        engine = nullptr;
    }
    NeuralNet::globalCleanup();
    ScoreValue::freeTables();
}

void GTPKata::boardsize(int boardsizex, int boardsizey) {
  engine->setOrResetBoardSize(cfg, logger, seedRand, boardsizex, boardsizey, loggingToStderr);
}

void GTPKata::clear_board() {
  engine->clearBoard();
}

void GTPKata::komi(float komi) {
  if(isForcingKomi)
    engine->updateKomiIfNew(forcedKomi);
  else
    engine->updateKomiIfNew(komi);
}

string GTPKata::gen_move(Player pla, double& winRate, double& scoreLead) {
  bool debug = false;
  bool playChosenMove = true;
  bool responseIsError = false;
  string response;

  engine->genMove(
    pla,
    logger,searchFactorWhenWinningThreshold,searchFactorWhenWinning,
    cleanupBeforePass,friendlyPass,ogsChatToStderr,
    allowResignation,resignThreshold,resignConsecTurns,resignMinScoreDifference,
    logSearchInfo,debug,playChosenMove,
    response,responseIsError,maybeStartPondering,
    winRate,scoreLead,
    GTPEngine::AnalyzeArgs()
  );
  if(logAllGTPCommunication)
    logger.write(response);
  return response;
}

void GTPKata::ponder() {
  if(maybeStartPondering)
    engine->ponder();
}

void GTPKata::play(int cellX, int cellY) {
  Player pla = engine->bot->getRootPla();
  int ySize = engine->bot->getRootBoard().y_size;
  cellY = -1 * (cellY - (ySize - 1));
  Loc loc = ((cellY + 1) * (ySize + 1)) + (cellX + 1);
  engine->play(loc,pla);
  maybeStartPondering = true;
}

void GTPKata::undo() {
  engine->undo();
}

void GTPKata::set_free_handicap(const std::vector<int> & move_handi) {
  int xSize = engine->bot->getRootBoard().x_size;
  int ySize = engine->bot->getRootBoard().y_size;
  Board board(xSize,ySize);
  for(auto itr = move_handi.begin(); itr != move_handi.end(); ++itr) {
    int handi_move = *itr;
    int x = (handi_move % (xSize + 2)) - 1;
    int y = (handi_move / (ySize + 2)) - 1;
    y = -1 * (y - (ySize - 1));
    handi_move = (x + 1) + (y + 1) * (ySize + 1);
    board.setStone(handi_move,P_BLACK);
  }
  Player pla = P_WHITE;
  BoardHistory hist(board,pla, engine->getCurrentRules(),0);
  hist.setInitialTurnNumber(board.numStonesOnBoard());
  vector<Move> newMoveHistory;
  engine->setPositionAndRules(pla,board,hist,board,pla,newMoveHistory);
}

std::vector<std::pair<float, int> > GTPKata::get_policy() {
  return engine->get_policy();
}

std::vector<float> GTPKata::get_owner() {
  return engine->get_owner();
}

string Version::getKataGoVersion() {
  return string("1.11.0");
}

string Version::getKataGoVersionForHelp() {
  return string("KataGo v1.11.0");
}

string Version::getGitRevision() {
  return string(GIT_REVISION);
}

string Version::getKataGoVersionFullInfo() {
  ostringstream out;
  out << Version::getKataGoVersionForHelp() << endl;
  out << "Git revision: " << Version::getGitRevision() << endl;
  out << "Compile Time: " << __DATE__ << " " << __TIME__ << endl;
#if defined(USE_CUDA_BACKEND)
  out << "Using CUDA backend" << endl;
#if defined(CUDA_TARGET_VERSION)
#define STRINGIFY(x) #x
#define STRINGIFY2(x) STRINGIFY(x)
  out << "Compiled with CUDA version " << STRINGIFY2(CUDA_TARGET_VERSION) << endl;
#endif
#elif defined(USE_TENSORRT_BACKEND)
  out << "Using TensorRT backend" << endl;
#elif defined(USE_OPENCL_BACKEND)
  out << "Using OpenCL backend" << endl;
#elif defined(USE_EIGEN_BACKEND)
  out << "Using Eigen(CPU) backend" << endl;
#else
  out << "Using dummy backend" << endl;
#endif

#if defined(USE_AVX2)
  out << "Compiled with AVX2 and FMA instructions" << endl;
#endif
#if defined(COMPILE_MAX_BOARD_LEN)
  out << "Compiled to allow boards of size up to " << COMPILE_MAX_BOARD_LEN << endl;
#endif
#if defined(BUILD_DISTRIBUTED)
  out << "Compiled to support contributing to online distributed selfplay" << endl;
#endif

  return out.str();
}
