#include "stdafx.h"
#include "config.h"
#include "App.h"
#include "MainFrame.h"
#ifdef WIN32
#include <DbgHelp.h>
#include <Shlwapi.h>
#endif

#ifdef WIN32
bool IsWindowsVistaOrHigher() {
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    return osvi.dwMajorVersion >= 6;
}

typedef BOOL (WINAPI *SetProcDPICall)(void);
#endif

IMPLEMENT_APP(MyApp);

#if defined(WIN32) && defined(USE_MINIDUMPS)
extern BOOL WriteMiniDumpHelper(HANDLE hDump, LPEXCEPTION_POINTERS param);
static LONG __stdcall ExceptFilterProc(LPEXCEPTION_POINTERS param) {
    if (IsDebuggerPresent()) {
        return UnhandledExceptionFilter(param);
    } else {
        TCHAR lpTempPathBuffer[MAX_PATH];
        TCHAR comboPathBuffer[MAX_PATH];
        GetTempPath(MAX_PATH, lpTempPathBuffer);
        PathCombine(comboPathBuffer, lpTempPathBuffer, L"leela.dmp");
        HANDLE hFile = CreateFile(
            comboPathBuffer,
            GENERIC_WRITE,          // open for writing
            0,                      // do not share
            NULL,                   // default security
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,  // normal file
            NULL);                  // no attr. template
        if (hFile != INVALID_HANDLE_VALUE) {
            WriteMiniDumpHelper(hFile, param);
            CloseHandle(hFile);
        }
        TerminateProcess(GetCurrentProcess(), 0);
        return 0;// never reached
    }
}
#endif

bool MyApp::OnInit()
{
#if defined(WIN32) && defined(USE_MINIDUMPS)
    SetUnhandledExceptionFilter(ExceptFilterProc);
#endif

    wxConfig * config = new wxConfig(wxT("LeelaI18N"), wxT("Sjeng.Org"));
    wxConfig::Set(config);

    wxLocale locale;
    locale.AddCatalogLookupPathPrefix(_T("catalogs"));
    if (wxConfig::Get()->ReadBool(wxT("japaneseEnabled"), true)) {
        locale.Init(wxLANGUAGE_JAPANESE, wxLOCALE_DONT_LOAD_DEFAULT);
    } else {
        locale.Init(wxLANGUAGE_ENGLISH, wxLOCALE_DONT_LOAD_DEFAULT);
    }
    locale.AddCatalog(_T("messages"));
    locale.AddCatalog(_T("wxstd"));

    wxImage::AddHandler(new wxPNGHandler());
#ifdef WIN32
    bool dpiScale = wxConfig::Get()->Read(wxT("dpiscaleEnabled"), (long)0);
    if (!dpiScale) {
        if (IsWindowsVistaOrHigher()) {
            HINSTANCE dllHandle = LoadLibrary(wxT("user32.dll"));
            if (dllHandle) {
                SetProcDPICall procDPI = (SetProcDPICall)GetProcAddress(
                    dllHandle, "SetProcessDPIAware");
                if (procDPI) {
                    procDPI();
                }
                FreeLibrary(dllHandle);
            }
        }
    }
#endif

    MainFrame* frame = new MainFrame(NULL, _("Leela"));

    frame->Show();

#ifdef WIN32
    SetTopWindow(frame);
#endif

    if (argc > 1) {
        long movenum = 999;
        if (argc > 2) {
            wxString moveNumString(argv[2]);
            moveNumString.ToLong(&movenum);
        }
        wxString sgfString(argv[1]);
        frame->loadSGF(sgfString, movenum);
    } else {
        wxCommandEvent evt;
        frame->doNewRatedGame(evt);
    }

    return true;
}
