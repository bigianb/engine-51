#pragma once

#define HNULL -1

struct xhandle
{
    int Handle;

    inline xhandle                  (   )      {}
    inline xhandle                  ( int I )      { Handle = I;             }
    inline operator const int       (  ) const { return Handle;          }
    inline bool          IsNonNull (  ) const { return Handle != HNULL; }
    inline bool          IsNull    (  ) const { return Handle == HNULL; }    
};
