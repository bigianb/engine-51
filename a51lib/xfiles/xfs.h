#pragma once

class xfs
{
public:
    xfs(const char* pFormatString, ...);
    ~xfs();
    operator const char*();
    xfs(const xfs& XFS);

private:
    xfs();
    const xfs& operator=(const xfs& XFS);
    char*      m_pString;
};
