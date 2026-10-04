#pragma once

#include <windows.h>

#ifdef CreateProcess
#undef CreateProcess
#endif
#ifdef CreateMutex
#undef CreateMutex
#endif
#ifdef MoveFile
#undef MoveFile
#endif
#ifdef GetUserName
#undef GetUserName
#endif
#ifdef ReadFile
#undef ReadFile
#endif
#ifdef WriteFile
#undef WriteFile
#endif