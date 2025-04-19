#include "x_plus.h"

int x_tolower(int c)
{
    if ((c >= 'A') && (c <= 'Z')) {
        c += ('a' - 'A');
    }
    return c;
}

char* x_stristr(const char* pMainStr, const char* pSubStr)
{
    char* pM = (char*)pMainStr;
    char* pS1;
    char* pS2;

    if (!*pSubStr) {
        return ((char*)pMainStr);
    }

    while (*pM) {
        pS1 = pM;
        pS2 = (char*)pSubStr;

        while (*pS1 && *pS2 && !(x_tolower(*pS1) - x_tolower(*pS2))) {
            pS1++;
            pS2++;
        }

        if (!*pS2) {
            return (pM);
        }

        pM++;
    }

    return nullptr;
}
