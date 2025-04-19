#pragma once

#define HNULL -1

struct xhandle
{
    int Handle;

    inline xhandle                  ( void  )      {}
    inline xhandle                  ( int I )      { Handle = I;             }
    inline operator const int       ( void ) const { return Handle;          }
    inline bool          IsNonNull ( void ) const { return Handle != HNULL; }
    inline bool          IsNull    ( void ) const { return Handle == HNULL; }    
};
