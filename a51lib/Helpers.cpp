#include "Helpers.h"
#include <cctype>

char* strsavecpy(char* pDest, const char* pSrc, int Count)
{
    char* pStart = pDest;

    while (Count && (*pDest = *pSrc)) {
        pDest++;
        pSrc++;
        Count--;
    }

    if (!Count) {
        *(pDest - 1) = 0;
    }

    return (pStart);
}

char* stristr(const char* pMainStr, const char* pSubStr)
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

        while (*pS1 && *pS2 && !(tolower(*pS1) - tolower(*pS2))) {
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
