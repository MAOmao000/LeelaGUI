#include "stdafx.h"
#include "TBoardPanel.h"
#include "MainFrame.h"
#include "SGFTree.h"
#include "MCOTable.h"
#include "Network.h"
#ifndef WIN32
#include "img/blackstone.xpm"
#include "img/whitestone.xpm"
#include "img/wood.xpm"
#endif

wxDEFINE_EVENT(wxEVT_DISPLAY_MAINLINE, wxCommandEvent);

BEGIN_EVENT_TABLE(TBoardPanel, wxPanel)
    EVT_KEY_DOWN(TBoardPanel::doKeyDown)
    EVT_PAINT(TBoardPanel::doPaint)
    EVT_ERASE_BACKGROUND(TBoardPanel::doErase)
    EVT_LEFT_DOWN(TBoardPanel::doLeftMouse)
END_EVENT_TABLE()

TBoardPanel::TBoardPanel(wxWindow *parent, wxWindowID winid, const wxPoint& pos,
                         const wxSize& size, long style,const wxString& name)
    : wxPanel(parent, winid, pos, size, style, name) {

    Bind(wxEVT_DISPLAY_MAINLINE, &TBoardPanel::doDisplayMainline, this);
    Bind(wxEVT_BESTMOVES_UPDATE, &TBoardPanel::doBestMovesUpdate, this);

    SetBackgroundStyle(wxBG_STYLE_CUSTOM);

    wxBitmap woodTile = wxBITMAP(IDB_BITMAPWOOD);
    int srcWidth = woodTile.GetWidth();
    int srcHeight = woodTile.GetHeight();

    // generate all orientations
    wxImage tileNorm = woodTile.ConvertToImage();
    wxImage tileHoriz = tileNorm.Mirror(true);
    wxImage tileVertic = tileNorm.Mirror(false);
    wxImage tileInvert = tileNorm.Mirror(true).Mirror(false);
    
    wxBitmap tileN(tileNorm);    
    wxBitmap tileH(tileHoriz);
    wxBitmap tileV(tileVertic);
    wxBitmap tileI(tileInvert);   

    // 2 x 2 tile
    m_tileFull.Create(srcWidth * 2, srcHeight * 2);
    wxMemoryDC dcTile;
    wxMemoryDC dcSrc;

    dcTile.SelectObject(m_tileFull);    
    dcSrc.SelectObject(tileN);    
    dcTile.Blit(0, 0, srcWidth, srcHeight, &dcSrc, 0, 0, wxCOPY, false);
    dcSrc.SelectObject(tileH);
    dcTile.Blit(srcWidth, 0, srcWidth, srcHeight, &dcSrc, 0, 0, wxCOPY, false);
    dcSrc.SelectObject(tileV);
    dcTile.Blit(0, srcHeight, srcWidth, srcHeight, &dcSrc, 0, 0, wxCOPY, false);
    dcSrc.SelectObject(tileI);
    dcTile.Blit(srcWidth, srcHeight, srcWidth, srcHeight, &dcSrc, 0, 0, wxCOPY, false);

    dcTile.SelectObject(wxNullBitmap);
    dcSrc.SelectObject(wxNullBitmap);

    // stones
    wxBitmap wstone = wxBITMAP(IDB_WHITESTONE);
    wstone.SetMask(new wxMask(wstone, *wxRED));

    wxBitmap bstone = wxBITMAP(IDB_BLACKSTONE);
    bstone.SetMask(new wxMask(bstone, *wxWHITE));

    m_whiteStone = wstone.ConvertToImage();
    m_blackStone = bstone.ConvertToImage();

    m_cellDim = 0;

    m_showMoyo = false;
    m_showOwner = false;
    m_showTerritory = false;
    m_showProbabilities = false;
    m_showBestMoves = false;
    m_stateLock = false;
    m_State = NULL;
    m_playerColor = FastBoard::BLACK;
    m_Hatch.resize(FastBoard::MAXSQ);
    m_PV.resize(FastBoard::MAXSQ);
    m_Probabilities.resize(FastBoard::MAXSQ);
    m_DisplayedStateHash = 0;
    m_use_KataGo = false;
    m_KataGo_engine_type = GTP::NONE;

    clearViz();
}

void TBoardPanel::setState(GameState * state) {
    m_State = state;
}

void TBoardPanel::setPlayerColor(int color) {
    m_playerColor = color;
}

void TBoardPanel::doPaint(wxPaintEvent& event) {
    if (m_State == NULL) {
        wxLogDebug(_("Paint on empty state"));
        return;
    }

    wxAutoBufferedPaintDC dc(this);
    PrepareDC(dc);

    wxSize sz = GetClientSize();
    wxLogDebug(_("width: %d height: %d"), sz.GetWidth(), sz.GetHeight());
    int boardSize = m_State->board.get_boardsize();

    int minDim = std::min(sz.GetWidth(), sz.GetHeight());
    int cellDim = std::max(1, minDim / ((boardSize - 1) + 2));
    m_cellDim = cellDim;

    wxLogDebug(_("cell size: %d"), cellDim);

    // Tiled background
    int tileW = m_tileFull.GetWidth();
    int tileH = m_tileFull.GetHeight();
    int xc = (sz.GetWidth() / tileW) + 1;
    int yc = (sz.GetHeight() / tileH) + 1;

    for (int y = 0; y < yc; y++) {
        for (int x = 0; x < xc; x++) {
            dc.DrawBitmap(m_tileFull, x * tileW, y * tileH);
        }
    }

    //if (m_State == NULL) {
    //    wxLogDebug(_("Paint on empty state"));
    //    return;
    //}

    wxPen penThick(*wxBLACK, 2, wxPENSTYLE_SOLID);
    wxPen penThin(*wxBLACK, 1, wxPENSTYLE_SOLID);
    wxPen penEmpty(*wxBLACK, 0, wxPENSTYLE_TRANSPARENT);

    wxGraphicsContext *mgc = wxGraphicsContext::Create(dc);

    if (m_showProbabilities || m_showBestMoves) {
        if (m_showProbabilities) {
            doProbabilities();
        }
        // Background image bitmap
        wxImage backgroundImg(cellDim * (boardSize + 2), cellDim * (boardSize + 2));
        size_t alphaSize = cellDim * (boardSize + 2) * cellDim  * (boardSize + 2);
        unsigned char* alphaBuff = (unsigned char*)calloc(alphaSize, 1);
        backgroundImg.SetAlpha(alphaBuff);

        wxGraphicsContext *gc = wxGraphicsContext::Create(backgroundImg);
        gc->SetPen(penEmpty);
        wxBrush brush(wxColour(255, 255, 255, wxALPHA_TRANSPARENT));
        gc->SetBrush(brush);
        gc->DrawRectangle(0, 0, (boardSize + 2) * cellDim, (boardSize + 2) * cellDim);

        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {
                // engine board is inverted vertically
                int vtx = m_State->board.get_vertex(x, boardSize - y - 1);

                int xoff = (x + 1) * cellDim;
                int yoff = (y + 1) * cellDim;

                float val = m_Probabilities[vtx];
                if (val > 0.005f) {
                    int red, green, blue;
                    float frac = 1.0f;
                    if (m_MaxProbability > 0.0f) {
                        frac = val / m_MaxProbability;
                    }
                    frac = std::max(0.0f, std::min(1.0f, frac));
                    if (val > m_MaxProbability / 4.0f) {
                        red = 255;
                        green = 255 * (1.0f - frac);
                        blue =  0;
                        frac = 0.75f + frac / 4.0f;
                    } else {
                        // 0.0 blue -- 0.25 yellow -- 1.0 red
                        red = 255 * (frac * 2.0f);
                        green = 255 * (frac * 2.0f);
                        blue = 255 - (255.0f * (frac * 2.0f));
                        frac = 0.25f + frac * 3.0f;
                    }

                    wxBrush brush_w(wxColour(red, green, blue, (unsigned char)(255.0f * frac)));
                    gc->SetBrush(brush_w);
                    if (val > 0.20f || cellDim < 9) {
                        gc->DrawRectangle(xoff, yoff, cellDim, cellDim);
                    } else if (val > 0.05f) {
                        gc->DrawRectangle(xoff + 2, yoff + 2, cellDim - 4, cellDim - 4);
                    } else {
                        gc->DrawRectangle(xoff + 4, yoff + 4, cellDim - 8, cellDim - 8);
                    }
                }
            }
        }

        gc->Flush();
        delete gc;
        wxImage blurMap = backgroundImg.Blur(cellDim / 2.5f);
        wxGraphicsBitmap bg = mgc->CreateBitmapFromImage(blurMap);
        mgc->DrawBitmap(bg, -cellDim / 2, -cellDim / 2,
                        backgroundImg.GetWidth(), backgroundImg.GetHeight());
    }

    delete mgc;

    // stones
    wxBrush bbrush(*wxBLACK, wxBRUSHSTYLE_SOLID);
    wxBrush rbrush(*wxRED, wxBRUSHSTYLE_SOLID);
    int stoneSize = ((cellDim * 19)/ 40);
    int stoneDiam = std::max(1, ((cellDim * 19 * 2)/ 40));

    // bitmaps
    wxBitmap wstone = wxBitmap(m_whiteStone.Scale(stoneDiam, stoneDiam, wxIMAGE_QUALITY_HIGH));
    wxBitmap bstone = wxBitmap(m_blackStone.Scale(stoneDiam, stoneDiam, wxIMAGE_QUALITY_HIGH));

    // emtpy fill
    wxBrush ebrush(*wxBLACK, wxBRUSHSTYLE_TRANSPARENT);

    // board outline
    dc.SetBrush(ebrush);
    dc.SetPen(penThick);
    dc.DrawRectangle(cellDim, cellDim, 
                     cellDim * (boardSize - 1), cellDim * (boardSize - 1));                

    // board lines            
    for (int y = 0; y < boardSize - 1; y++) {
        for (int x = 0; x < boardSize - 1; x++) {
            int xoff = cellDim + x * cellDim;
            int yoff = cellDim + y * cellDim;
            dc.SetPen(penThin);
            dc.SetBrush(ebrush);
            dc.DrawRectangle(xoff - 1, yoff - 1, cellDim + 1, cellDim + 1);
            if (m_State->board.starpoint(boardSize, x, y)) {
                dc.SetPen(penEmpty);
                dc.SetBrush(bbrush);
#ifdef WIN32
                dc.DrawCircle(xoff, yoff, std::max(1, (stoneSize/5)+1));
#else
                dc.DrawCircle(xoff - 1, yoff - 1, std::max(1, (stoneSize/5)+1));
#endif
            }
        }
    }  
    
    // board coordinates
    int coordFontSize = cellDim / 3/*4*/;
    int coordFontSize0 = (int)(cellDim / 4.5f);
    int pvFontSize = (int)(cellDim / 2.5f);
    wxFont cfont(coordFontSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    wxFont pvfont(pvFontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    dc.SetFont(cfont);
    dc.SetBackgroundMode(wxTRANSPARENT);
    dc.SetTextForeground(*wxBLACK);
    dc.SetTextBackground(*wxWHITE);
    for (int y = 0; y < boardSize; y++) {
        int x = 0;
        int xoff = cellDim + (x - 1) * cellDim;
        int yoff = cellDim + y  * cellDim;   
        xoff = xoff + cellDim / 2 - coordFontSize0;  
        yoff = yoff - coordFontSize0; 
        int xoff2 = xoff + cellDim * boardSize;
        wxString text;
        text.Printf("%2d", boardSize - y);
        dc.DrawText(text, xoff, yoff);
        dc.DrawText(text, xoff2, yoff);
    }   
    for (int x = 0; x < boardSize; x++) {
        int y = 0;
        int yoff = cellDim + (y - 1) * cellDim;
        int xoff = cellDim + x  * cellDim;   
        yoff = yoff + cellDim / 2 - coordFontSize0;  
        xoff = xoff - coordFontSize; 
        int yoff2 = yoff + cellDim * boardSize;
        wxString text;
        if (x < 25) {
            if (x < 8) {
                text.Printf(" %c", 'A'/*'a'*/ + x);
            } else {
                text.Printf(" %c", 'A'/*'a'*/ + x  + 1);
            }
        } else {
            int tmx = x - 25;
            if (tmx < 8) {
                text.Printf(" %c", 'A' + tmx);
            } else {
                text.Printf(" %c", 'A' + tmx  + 1);
            }
        }
        dc.DrawText(text, xoff, yoff);
        dc.DrawText(text, xoff, yoff2);
    }      
    
    // moyo/territory/influence
    if (!m_showTerritory) {
        if (m_showMoyo) {
            doMoyo();
        } else if (m_showOwner) {
            doOwner();
        }
    }

    if (m_showMoyo || m_showTerritory) {
        wxBrush bmbrush(*wxBLACK, wxBRUSHSTYLE_CROSSDIAG_HATCH);
        wxBrush wmbrush(*wxWHITE, wxBRUSHSTYLE_CROSSDIAG_HATCH);

        dc.SetPen(penEmpty);

        for (int y = 0; y < boardSize; y++) {
            for (int x = 0; x < boardSize; x++) {           
                // engine board is inverted vertically
                int vtx = m_State->board.get_vertex(x, boardSize - y - 1);
                
                int xoff = (cellDim + x * cellDim) - 1;
                int yoff = (cellDim + y * cellDim) - 1;
                
                int xxoff = (xoff - (stoneDiam/2));
                int yyoff = (yoff - (stoneDiam/2));
                
                if (m_Hatch[vtx] == FastBoard::BLACK) {                               
                    dc.SetBrush(bmbrush);
                    dc.DrawRoundedRectangle(xxoff, yyoff, cellDim, cellDim, 0);
                } else if (m_Hatch[vtx] == FastBoard::WHITE) {                                
                    dc.SetBrush(wmbrush);
                    dc.DrawRoundedRectangle(xxoff, yyoff, cellDim, cellDim, 0);
                }
            }
        }    
    }

    // stones
    dc.SetBrush(rbrush);
    wxPen wDeadPenThick(*wxBLACK, 2, wxPENSTYLE_SOLID);
    wxPen bDeadPenThick(*wxWHITE, 2, wxPENSTYLE_SOLID);
    dc.SetFont(pvfont);

    wxBrush bmbrush(wxColour(128, 128, 128));
    wxBrush wmbrush(wxColour(255, 255, 0));

    // PV coloring
    bool btm = m_analysisColor == FastBoard::BLACK;

    for (int y = 0; y < boardSize; y++) {
        for (int x = 0; x < boardSize; x++) {
            // engine board is inverted vertically
            int vtx = m_State->board.get_vertex(x, boardSize - y - 1);
            int cell = m_State->board.get_square(vtx);

            int xoff = (cellDim + x * cellDim) - 1;
            int yoff = (cellDim + y * cellDim) - 1;

            int xxoff = (xoff - (stoneDiam/2));
            int yyoff = (yoff - (stoneDiam/2));

            dc.SetPen(penThin);
            if (cell == FastBoard::BLACK
                || (cell == FastBoard::EMPTY
                   && ((btm && m_PV[vtx] > 0 && (m_PV[vtx] % 2 == 1))
                   || (!btm && m_PV[vtx] > 0 && (m_PV[vtx] % 2 == 0))))) {
                dc.DrawBitmap(bstone, xxoff, yyoff, true);
            } else if (cell == FastBoard::WHITE
                       || (cell == FastBoard::EMPTY
                       && ((!btm && m_PV[vtx] > 0 && (m_PV[vtx] % 2 == 1))
                           || (btm && m_PV[vtx] > 0 && (m_PV[vtx] % 2 == 0))))) {
                dc.DrawBitmap(wstone, xxoff, yyoff, true);
            } // else if (m_showOwner) {
            if (m_showOwner) {
                float ratio = 2.0f * fabs(0.5f - m_Owner[vtx]);
                int miniDiam = (stoneDiam * ratio) - 1;
                if (miniDiam >= 3) {
                    miniDiam = (stoneDiam * ratio * 0.8f) - 1;
                    int xxxoff = (xoff - (miniDiam/2));
                    int yyyoff = (yoff - (miniDiam/2));

                    if (m_Owner[vtx] > 0.5f) {
                        dc.SetBrush(bmbrush);
                        dc.DrawRoundedRectangle(xxxoff, yyyoff, miniDiam, miniDiam, 0);
                    } else {
                        dc.SetBrush(wmbrush);
                        dc.DrawRoundedRectangle(xxxoff, yyyoff, miniDiam, miniDiam, 0);
                    }
                    dc.SetBrush(rbrush);
                }
            }

            // Part of the mainline
            if (m_PV[vtx] > 0) {
                wxString text;
                text.Printf("%d", m_PV[vtx]);
                wxRect rect = dc.GetTextExtent(text);
                int cx = xoff - (rect.GetWidth() / 2);
                int cy = yoff - (rect.GetHeight() / 2);
                /*dc.SetTextForeground(*wxWHITE);
                dc.DrawText(text, cx-1, cy-1);
                dc.DrawText(text, cx+1, cy+1);
                dc.DrawText(text, cx-1, cy+1);
                dc.DrawText(text, cx+1, cy-1);*/
                if ((btm && m_PV[vtx] % 2 == 1)
                    || (!btm && m_PV[vtx] % 2 == 0)) {
                    dc.SetTextForeground(*wxWHITE);
                } else {
                    dc.SetTextForeground(*wxBLACK);
                }
                dc.DrawText(text, cx, cy);
            }

            if (cell != FastBoard::EMPTY) {
                if (m_State->get_last_move() == vtx) {
                    dc.DrawCircle(xoff, yoff, stoneSize/3);
                }
            }

            int bxoff = (xoff - (cellDim/2));
            int byoff = (yoff - (cellDim/2));
            
            if (m_showTerritory) {
                // dead stone
                if ((cell == FastBoard::BLACK && m_Hatch[vtx] == FastBoard::WHITE)
                  ||(cell == FastBoard::WHITE && m_Hatch[vtx] == FastBoard::BLACK)) {
                    if (cell == FastBoard::BLACK) {
                        dc.SetPen(bDeadPenThick);  
                    } else {
                        dc.SetPen(wDeadPenThick);  
                    }
                    dc.DrawLine(bxoff, yyoff, bxoff+cellDim, byoff+cellDim);
                    dc.DrawLine(bxoff+cellDim, byoff, bxoff, byoff+cellDim);
                }
            }
        }
    }

    dc.SetBrush(wxNullBrush);
    dc.SetPen(wxNullPen);
    dc.SetFont(wxNullFont);
}

void TBoardPanel::doErase(wxEraseEvent& event) {    
    //event.Skip();
}

void TBoardPanel::doLeftMouse(wxMouseEvent& event) {
    if (m_State == NULL) {
        wxLogDebug(_("Click on empty board"));
        return;
    }
    
    int startX = event.GetX();
    int startY = event.GetY();
    
    wxLogDebug(_("Left down at %d %d"), startX, startY);
    
    if (m_State->get_last_move() == FastBoard::RESIGN) {
        wxLogDebug(_("Game has been resigned"));
        return;
    }
    
    if (m_stateLock) {
        wxLogDebug(_("Click on locked state"));
        return;
    } 
    
    if (m_State->get_to_move() == m_playerColor) {
        int boardSize = m_State->board.get_boardsize();
        
        int corrX = startX - (m_cellDim / 2);
        int corrY = startY - (m_cellDim / 2);
     
        int cellX = corrX / m_cellDim;
        int cellY = corrY / m_cellDim;
        
        if (cellX >= boardSize) cellX = boardSize - 1;
        if (cellY >= boardSize) cellY = boardSize - 1;
        
        // engine board is inverted vertically
        cellY = boardSize - cellY - 1;
     
        int vtx = m_State->board.get_vertex(cellX, cellY);
        
        if (m_State->legal_move(vtx)) {
            wxCommandEvent event_w(wxEVT_NEW_MOVE, GetId());
            event_w.SetEventObject(this);
            if (m_use_KataGo && m_KataGo_engine_type == GTP::GTP_INTERFACE) {
                wxString msg;
                if (m_State->get_to_move() == FastBoard::BLACK) {
                    msg = wxString("play b " + m_State->move_to_text(vtx));
                } else {
                    msg = wxString("play w " + m_State->move_to_text(vtx));
                }
                event_w.SetString(msg);
            }
            ::wxPostEvent(GetEventHandler(), event_w);

            m_State->play_move(vtx);
            
            Refresh();
        }
        
    } else {
        wxLogDebug(_("It's not your move!"));
    }
    
    event.Skip();
}

void TBoardPanel::setShowTerritory(bool val) {
    m_showTerritory = val;
    
    Refresh();
}

void TBoardPanel::setShowOwner(bool val) {
    m_showOwner = val;
    
    Refresh();
}

void TBoardPanel::setShowMoyo(bool val) {
    m_showMoyo = val;
    
    Refresh();
}

bool TBoardPanel::getShowOwner() {
    return m_showOwner;
}

bool TBoardPanel::getShowMoyo() {
    return m_showMoyo;
}

void TBoardPanel::setShowProbabilities(bool val) {
    m_showProbabilities = val;
    m_DisplayedStateHash = 0;

    Refresh();
}

bool TBoardPanel::getShowProbabilities() {
    return m_showProbabilities;
}

void TBoardPanel::setShowBestMoves(bool val) {
    m_showBestMoves = val;
    m_DisplayedStateHash = 0;

    Refresh();
}

bool TBoardPanel::getShowBestMoves() {
    return m_showBestMoves;
}

void TBoardPanel::doMoyo() {
    std::vector<int> moyo = m_State->board.moyo();

    m_Hatch.resize(FastBoard::MAXSQ);
    std::fill(m_Hatch.begin(), m_Hatch.end(), FastBoard::EMPTY);

    for (size_t i = 0; i < moyo.size(); i++) {
        if (moyo[i] > 0) {
            m_Hatch[i] = FastBoard::BLACK;
        } else if (moyo[i] < 0) {
            m_Hatch[i] = FastBoard::WHITE;
        }
    }
}

void TBoardPanel::doOwner() {
    int boardsize = m_State->board.get_boardsize();

    if (m_use_KataGo) {
        if (m_State->m_owner.size() > 0) {
            m_Owner.clear();
            for (auto itr = m_State->m_owner.begin(); itr != m_State->m_owner.end(); ++itr) {
                m_Owner.emplace_back(*itr);
            }
            return;
        }
        m_Owner.resize(FastBoard::MAXSQ);
        std::fill(m_Owner.begin(), m_Owner.end(), 0.5f);
        return;
    }

    if (!MCOwnerTable::get_MCO()->is_primed()) {
        Playout::mc_owner(*m_State);
    }

    m_Owner.resize(FastBoard::MAXSQ);
    std::fill(m_Owner.begin(), m_Owner.end(), 0.5f);

    for (int x = 0; x < boardsize; x++) {
        for (int y = 0; y < boardsize; y++) {
            int vertex = m_State->board.get_vertex(x, y);
            m_Owner[vertex] =
                MCOwnerTable::get_MCO()->get_blackown(FastBoard::BLACK, vertex);
        }
    }
}

void TBoardPanel::doTerritory(bool disputing) {
    std::vector<int> map = m_State->final_score_map(disputing ? false : true);

    m_Hatch.resize(FastBoard::MAXSQ);
    std::fill(m_Hatch.begin(), m_Hatch.end(), FastBoard::EMPTY);

    for (size_t i = 0; i < map.size(); i++) {
        if (map[i] == FastBoard::BLACK) {
            m_Hatch[i] = FastBoard::BLACK;
        } else if (map[i] == FastBoard::WHITE) {
            m_Hatch[i] = FastBoard::WHITE;
        }
    }
}

void TBoardPanel::lockState() {
    m_stateLock = true;
}

void TBoardPanel::unlockState() {
    m_stateLock = false;
}

void TBoardPanel::doKeyDown(wxKeyEvent& event) {
    auto keycode = event.GetKeyCode();
    if (keycode == WXK_LEFT || keycode == WXK_RIGHT) {
        static_cast<MainFrame*>(GetParent())->doKeyDown(event);
    } else {
        event.Skip();
    }
}

void TBoardPanel::doDisplayMainline(wxCommandEvent& event) {
    if (m_State == NULL) {
        return;
    }
    
    wxString pv = event.GetString();
    int toMove = event.GetInt();

    // Clear past PV
    std::fill(m_PV.begin(), m_PV.end(), 0);
    int pv_move_counter = 0;
    m_analysisColor = toMove;

    wxStringTokenizer tokenizer(pv);
    while (tokenizer.HasMoreTokens()) {
        pv_move_counter++;

        wxString move = tokenizer.GetNextToken();
        int vertex = m_State->board.text_to_move(move.ToStdString());
        if (vertex >= 0 && vertex < (int)m_PV.size()) {
            if (m_PV[vertex] == 0) {
                m_PV[vertex] = pv_move_counter;
            }
        }
    }

    Refresh();
}

void TBoardPanel::doProbabilities() {
    if (m_State->board.get_hash() != m_DisplayedStateHash) {
        if (m_use_KataGo) {
            if (m_State->m_policy.size() > 0) {
                m_Probabilities.clear();
                for (auto itr = m_State->m_policy.begin(); itr != m_State->m_policy.end(); ++itr) {
                    m_Probabilities.emplace_back(*itr);
                }
                m_MaxProbability = m_State->m_policy[0];
                m_DisplayedStateHash = m_State->board.get_hash();
                return;
            }
            std::fill(m_Probabilities.begin(), m_Probabilities.end(), 0.0f);
            m_MaxProbability = 0.0f;
            return;
        }
        std::fill(m_Probabilities.begin(), m_Probabilities.end(), 0.0f);
        m_MaxProbability = 0.0f;

        auto vec = Network::get_Network()->get_scored_moves(
            m_State, Network::Ensemble::AVERAGE_ALL);

        for (const auto& pair : vec) {
            m_Probabilities[pair.second] = pair.first;
            if (pair.first > m_MaxProbability) {
                m_MaxProbability = pair.first;
            }
        }
        m_DisplayedStateHash = m_State->board.get_hash();
    }
}

void TBoardPanel::clearViz() {
    std::fill(m_PV.begin(), m_PV.end(), 0);
    std::fill(m_Hatch.begin(), m_Hatch.end(), FastBoard::EMPTY);
    std::fill(m_Owner.begin(), m_Owner.end(), 0.5f);
    std::fill(m_Probabilities.begin(), m_Probabilities.end(), 0.0f);
    m_DisplayedStateHash = 0;
    m_MaxProbability = 0.0f;
}

void TBoardPanel::doBestMovesUpdate(wxCommandEvent& event) {
    if (m_State == NULL) {
        return;
    }
    
    void* rawdataptr = event.GetClientData();
    if (!rawdataptr) return;

    // Take ownership of the data
    auto data = reinterpret_cast<std::vector<std::pair<std::string, float>>*>(rawdataptr);
    std::unique_ptr<std::remove_pointer<decltype(data)>::type> dataptr(data);

    if (m_showBestMoves) {
        std::fill(m_Probabilities.begin(), m_Probabilities.end(), 0.0f);
        m_MaxProbability = 0.0f;

        for (const auto& pair : *data) {
            std::string move = pair.first;
            int vertex = m_State->board.text_to_move(move);
            if (vertex >= 0 && vertex < (int)m_Probabilities.size()) {
                m_Probabilities[vertex] = pair.second;
                if (pair.second > m_MaxProbability) {
                    m_MaxProbability = pair.second;
                }
            }
        }
        m_DisplayedStateHash = m_State->board.get_hash();
        Refresh();
    }
}

void TBoardPanel::setUseKataGo(bool use_KataGo) {
    m_use_KataGo = use_KataGo;
}

void TBoardPanel::setKataGoEngineType(int engine_type) {
    m_KataGo_engine_type = engine_type;
}