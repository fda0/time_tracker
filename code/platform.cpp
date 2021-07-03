#if Def_Windows
#include "win32_platform.cpp"
#elif Def_Linux
#include "linux_platform.cpp"
#else
#error "platform unsupported"
#endif

