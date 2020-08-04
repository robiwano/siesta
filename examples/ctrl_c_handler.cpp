#include "ctrl_c_handler.h"

static bool was_signalled = false;

bool ctrlc::signalled() { return was_signalled; }

#ifdef _WIN32

#include <objbase.h>
#include <windows.h>

BOOL WINAPI HandlerRoutine(_In_ DWORD dwCtrlType)
{
    switch (dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
        was_signalled = true;
        return TRUE;
    default:
        // Pass signal on to the next handler
        return FALSE;
    }
}

void ctrlc::set_signal_handler()
{
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);
}

#else
#include <signal.h>

void intHandler(int) { was_signalled = true; }

void ctrlc::set_signal_handler()
{
    signal(SIGINT, intHandler);
    signal(SIGSTOP, intHandler);
    signal(SIGTERM, intHandler);
}

#endif
