#ifndef __NewGameDialog__
#define __NewGameDialog__

#include "stdafx.h"

/**
@file
Subclass of TNewGameDialog, which is generated by wxFormBuilder.
*/

#include "GUI.h"

/** Implementing TNewGameDialog */
class NewGameDialog : public TNewGameDialog
{
protected:    
    virtual void doCancel( wxCommandEvent& event );
    virtual void doOK( wxCommandEvent& event );
    virtual void doInit( wxInitDialogEvent& event );
    virtual void doHandicapUpdate( wxSpinEvent& event );
    virtual void doRadioBox( wxCommandEvent& event );
    virtual void doLevel( wxCommandEvent& event );
    virtual void doChangeEngine( wxCommandEvent& event );
    virtual void doChangeRule( wxCommandEvent& event );
    
public:
    /** Constructor */
    NewGameDialog( wxWindow* parent );                
    
    /* getters/setters */
    float getKomi();
    int getHandicap();
    int getBoardsize();
    int getSimulations();
    int getPlayerColor();
    int getTimeControl();
    int getByoControl();
    bool getNetsEnabled();
    static int simulationsToVisitLimit(int visits);

    void checkNetsEnabled();
    void checkHandicapRange();
};

#endif // __NewGameDialog__
