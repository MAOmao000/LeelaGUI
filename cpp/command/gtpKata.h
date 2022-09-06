#ifndef GTPKATA_H_INCLUDED
#define GTPKATA_H_INCLUDED

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
//#include "../main.h"
#ifdef NO_GIT_REVISION
#define GIT_REVISION "<omitted>"
#else
#include "../program/gitinfo.h"
#endif

using namespace std;

namespace Version {
    std::string getKataGoVersion();
    std::string getKataGoVersionForHelp();
    std::string getKataGoVersionFullInfo();
    std::string getGitRevision();
    std::string getGitRevisionWithBackend();
}

struct GTPEngine
{
  GTPEngine(const GTPEngine&) = delete;
  GTPEngine& operator=(const GTPEngine&) = delete;

  const std::string nnModelFile;
  const bool assumeMultipleStartingBlackMovesAreHandicap;
  const int analysisPVLen;
  const bool preventEncore;

  const double dynamicPlayoutDoublingAdvantageCapPerOppLead;
  double staticPlayoutDoublingAdvantage;
  bool staticPDATakesPrecedence;
  double normalAvoidRepeatedPatternUtility;
  double handicapAvoidRepeatedPatternUtility;

  double genmoveWideRootNoise;
  double analysisWideRootNoise;
  bool genmoveAntiMirror;
  bool analysisAntiMirror;

  NNEvaluator* nnEval;
  AsyncBot* bot;
  Rules currentRules; //Should always be the same as the rules in bot, if bot is not NULL.

  //Stores the params we want to be using during genmoves or analysis
  SearchParams params;

  TimeControls bTimeControls;
  TimeControls wTimeControls;

  //This move history doesn't get cleared upon consecutive moves by the same side, and is used
  //for undo, whereas the one in search does.
  Board initialBoard;
  Player initialPla;
  std::vector<Move> moveHistory;

  std::vector<double> recentWinLossValues;
  double lastSearchFactor;
  double desiredDynamicPDAForWhite;
  bool avoidMYTDaggerHack;
  std::unique_ptr<PatternBonusTable> patternBonusTable;

  Player perspective;

  double genmoveTimeSum;

  GTPEngine(
    const std::string& modelFile, SearchParams initialParams, Rules initialRules,
    bool assumeMultiBlackHandicap, bool prevtEncore,
    double dynamicPDACapPerOppLead, double staticPDA, bool staticPDAPrecedence,
    double normAvoidRepeatedPatternUtility, double hcapAvoidRepeatedPatternUtility,
    bool avoidDagger,
    double genmoveWRN, double analysisWRN,
    bool genmoveAntiMir, bool analysisAntiMir,
    Player persp, int pvLen,
    std::unique_ptr<PatternBonusTable>&& pbTable
  );
  ~GTPEngine();
  void stopAndWait();
  Rules getCurrentRules();
  void clearStatsForNewGame();
  void setOrResetBoardSize(ConfigParser& cfg, Logger& logger, Rand& seedRand, int boardXSize, int boardYSize, bool loggingToStderr);
  void setPositionAndRules(Player pla, const Board& board, const BoardHistory& h, const Board& newInitialBoard, Player newInitialPla, const std::vector<Move> newMoveHistory);
  void clearBoard();
  bool setPosition(const std::vector<Move>& initialStones);
  void updateKomiIfNew(float newKomi);
  void setStaticPlayoutDoublingAdvantage(double d);
  void setAnalysisWideRootNoise(double x);
  void setRootPolicyTemperature(double x);
  void setNumSearchThreads(int numThreads);
  void updateDynamicPDA();
  bool play(Loc loc, Player pla);
  bool undo();
  bool setRulesNotIncludingKomi(Rules newRules, std::string& error);
  void ponder();

  struct AnalyzeArgs {
    bool analyzing = false;
    bool lz = false;
    bool kata = false;
    int minMoves = 0;
    int maxMoves = 10000000;
    bool showOwnership = false;
    bool showOwnershipStdev = false;
    bool showMovesOwnership = false;
    bool showMovesOwnershipStdev = false;
    bool showPVVisits = false;
    bool showPVEdgeVisits = false;
    double secondsPerReport = TimeControls::UNLIMITED_TIME_DEFAULT;
    std::vector<int> avoidMoveUntilByLocBlack;
    std::vector<int> avoidMoveUntilByLocWhite;
  };

  void filterZeroVisitMoves(const AnalyzeArgs& args, std::vector<AnalysisData> buf);
  std::function<void(const Search* search)> getAnalyzeCallback(Player pla, AnalyzeArgs args);
  void genMove(
    Player pla,
    Logger& logger, double searchFactorWhenWinningThreshold, double searchFactorWhenWinning,
    enabled_t cleanupBeforePass, enabled_t friendlyPass, bool ogsChatToStderr,
    bool allowResignation, double resignThreshold, int resignConsecTurns, double resignMinScoreDifference,
    bool logSearchInfo, bool debug, bool playChosenMove,
    std::string& response, bool& responseIsError, bool& maybeStartPondering,
    double& winRate, double& scoreLead,
    AnalyzeArgs args
  );
  void clearCache();
  void placeFixedHandicap(int n, std::string& response, bool& responseIsError);
  void placeFreeHandicap(int n, std::string& response, bool& responseIsError, Rand& rand);
  void analyze(Player pla, AnalyzeArgs args);
  void computeAnticipatedWinnerAndScore(Player& winner, double& finalWhiteMinusBlackScore);
  std::vector<bool> computeAnticipatedStatuses();
  std::string rawNN(int whichSymmetry);
  std::vector<std::pair<float, int> > get_policy();
  SearchParams getParams();
  void setParams(SearchParams p);
  std::vector<float> get_owner();
};

class GTPKata {
public:
    GTPKata(
        const vector<string>& args,
        bool allowResignation,
        enabled_t cleanupBeforePass,
        float forcedKomi,
        enabled_t friendlyPass,
        bool isForcingKomi,
        bool logSearchInfo,
        bool loggingToStder,
        bool ogsChatToStderr,
        int resignConsecTurns,
        double resignMinScoreDifference,
        double resignThreshold,
        double searchFactorWhenWinning,
        double searchFactorWhenWinningThreshold,
        bool maybeStartPondering,
        bool logAllGTPCommunication
    );
    ~GTPKata();
    void boardsize(int boardsizex, int boardsizey);
    void clear_board();
    void komi(float komi);
    string gen_move(Player pla, double& winRate, double& scoreLead);
    void ponder();
    void play(int cellX, int cellY);
    void undo();
    void set_free_handicap(const std::vector<int>& move_handi);
    std::vector<std::pair<float, int> > get_policy();
    std::vector<float> get_owner();
private:
    bool allowResignation;
    enabled_t cleanupBeforePass;
    float forcedKomi;
    enabled_t friendlyPass;
    bool isForcingKomi;
    bool logSearchInfo;
    bool loggingToStderr;
    bool ogsChatToStderr;
    int resignConsecTurns;
    double resignMinScoreDifference;
    double resignThreshold;
    double searchFactorWhenWinning;
    double searchFactorWhenWinningThreshold;
    bool maybeStartPondering;
    bool logAllGTPCommunication;

    GTPEngine* engine;
    Rand seedRand;
    ConfigParser cfg;
    Logger logger;
    std::unique_ptr<PatternBonusTable> patternBonusTable;
    std::vector<std::unique_ptr<PatternBonusTable>> tables;
};

#endif
