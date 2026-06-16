#pragma once

char*   x_stristr   ( const char* pMainStr, const char* pSubStr );

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#else
// needed for strcasecmp on linux
#include <strings.h>
#endif

#define x_stricmp strcasecmp
