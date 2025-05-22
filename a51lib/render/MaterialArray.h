#pragma once
#include "Render.h"
#include <cassert>

#ifndef RENDER_PRIVATE
#error "This file is for internal use by the rendering system. Please don't include it!"
#endif

//=============================================================================
// PRIVATE PRIVATE PRIVATE
//
// This class is VERY similar to a standard xharray, except that while the
// xharray has a direct mapping between the handle and the data, ours will
// use in indirect mapping through indices. This will be better for sorting,
// and allow us to still store xhandles for the normal render sort key. Also
// this guarantees that we'll be accessing materials linearly for rendering.
// This class should only be used by the rendering system and the game should
// go through the interface provided in Render.hpp
//=============================================================================

class ResourceManager;
class material_array
{
public:
         material_array (ResourceManager* rm);
        ~material_array ();

    void        Sort                ();
    int         GetCount            () const;
    void        Clear               ();
    void        GrowListBy          ( int       nNodes  );
    material&   Add                 ( xhandle&  hHandle );
    material&   operator[]          ( int       Index   );
    material&   operator()          ( xhandle   hHandle );
    void        DeleteByHandle      ( xhandle   hHandle );
    int         GetIndexByHandle    ( xhandle   hHandle );
    xhandle     GetHandleByIndex    ( int       Index   );
    bool       SanityCheck         () const;
    void        Update              ( float       DeltaTime );


protected:
    struct destructor                         
    {  
        material Item; 
        inline ~destructor() {} 
    };     

    struct node_info
    {
        material    Item;
        xhandle     Handle;
    };

    friend  int NodeCompareFn( const void* pA, const void* pB );

    bool       m_Sorted;       // is this guy sorted?
    int         m_Capacity;
    int         m_nNodes;
    node_info*  m_pNodes;       // indexes have a direct mapping into this array
    int*        m_pIndices;     // handles have a direct mapping into this array

    ResourceManager* resourceManager;
};

//=============================================================================

inline int material_array::GetCount() const
{
    return m_nNodes;
}

//=============================================================================

inline material& material_array::operator[]( int Index )
{
    assert( (Index>=0) && (Index<m_nNodes) );
    return m_pNodes[Index].Item;
}

//=============================================================================

inline material& material_array::operator()( xhandle hHandle )
{
    assert( (hHandle.Handle>=0) && (hHandle.Handle<m_Capacity) );
    return operator[](m_pIndices[hHandle.Handle]);
}

//=============================================================================

inline int material_array::GetIndexByHandle( xhandle hHandle )
{
    assert( (hHandle.Handle>=0) && (hHandle.Handle<m_Capacity) );
    return m_pIndices[hHandle.Handle];
}

//=============================================================================

inline xhandle material_array::GetHandleByIndex( int Index )
{
    assert( (Index>=0) && (Index<m_nNodes) );
    return m_pNodes[Index].Handle;
}
