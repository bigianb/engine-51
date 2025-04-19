#pragma once

#include "xHandle.h"
#include <cassert>

template <class T>
class xharray
{
public:
    xharray();
    ~xharray();
    int GetCount() const;

    int  GetCapacity() const;
    void Clear(bool bReorder = false);
    T&   Add();
    T&   Add(xhandle& hHandle);

    T&       operator[](int Index);
    const T& operator[](int Index) const;
    T&       operator()(xhandle hHandle);
    const T& operator()(xhandle hHandle) const;

    void    DeleteByIndex(int Index);
    void    DeleteByHandle(xhandle hHandle);
    xhandle GetHandleByIndex(int Index) const;
    int     GetIndexByHandle(xhandle hHandle) const;

    int  CalcGrowth();
    void GrowListBy(int nNodes);

    const xharray<T>& operator=(const xharray<T>& Array);

protected:
    int      m_Capacity;
    int      m_nNodes;
    xhandle* m_pHandle;
    T*       m_pList;
};

template <class T>
inline int xharray<T>::CalcGrowth()
{
    return std::max(100, (m_Capacity / 2));
}

template <class T>
inline xharray<T>::xharray()
{
    m_Capacity = 0;
    m_nNodes = 0;
    m_pHandle = nullptr;
    m_pList = nullptr;
}

template <class T>
inline xharray<T>::~xharray()
{
    Clear(false);

    delete[] m_pHandle;
    delete[] m_pList;
}

template <class T>
inline const xharray<T>& xharray<T>::operator=(const xharray<T>& Array)
{
    //
    // delete all the nodes
    //
    Clear(false);

    delete[] m_pHandle;
    delete[] m_pList;
    m_pHandle = nullptr;
    m_pList = nullptr;

    //
    // Reset all the variables
    //
    m_Capacity = 0;
    m_nNodes = 0;

    //
    // Allocate the buffers
    //
    GrowListBy(Array.GetCount());

    //
    // Copy all the nodes
    //
    for (int i = 0; i < Array.GetCount(); i++) {
        Add() = Array[i];
    }

    return *this;
}

template <class T>
void xharray<T>::GrowListBy(int nNodes)
{
    xhandle* pNewHandle;
    T*       pNewList;
    int      i;

    assert(nNodes > 0);

    //
    // Increase the capacity
    //
    m_Capacity += nNodes;

    //
    // Allocate the new arrays
    //
    pNewList = new T[m_Capacity];
    assert(pNewList);

    pNewHandle = new xhandle[m_Capacity];
    assert(pNewHandle);

    //
    // Copy all the previous nodes to the new arrays
    //
    for (i = 0; i < m_nNodes; i++) {
        pNewHandle[i] = m_pHandle[i];
        pNewList[i] = m_pList[i];
    }

    //
    // Fill in the rest of the hash entries
    //
    for (i = m_nNodes; i < m_Capacity; i++) {
        pNewHandle[i].Handle = i;
    }

    //
    // Update the class with the new lists
    //
    delete[] m_pHandle;
    delete[] m_pList;

    m_pHandle = pNewHandle;
    m_pList = pNewList;
}

template <class T>
inline T& xharray<T>::operator[](int Index)
{
    assert(Index >= 0);
    assert(Index < m_nNodes);

    return m_pList[m_pHandle[Index].Handle];
}

template <class T>
inline const T& xharray<T>::operator[](int Index) const
{
    assert(Index >= 0);
    assert(Index < m_nNodes);

    return m_pList[m_pHandle[Index].Handle];
}

template <class T>
inline T& xharray<T>::operator()(xhandle hHandle)
{
    assert(hHandle >= 0);
    assert(hHandle < m_Capacity);

    return m_pList[hHandle.Handle];
}

template <class T>
inline const T& xharray<T>::operator()(xhandle hHandle) const
{
    assert(hHandle >= 0);
    assert(hHandle < m_Capacity);

    return m_pList[hHandle.Handle];
}

template <class T>
inline int xharray<T>::GetCount() const
{
    return m_nNodes;
}

template <class T>
inline int xharray<T>::GetCapacity() const
{
    return m_Capacity;
}

template <class T>
inline xhandle xharray<T>::GetHandleByIndex(int Index) const
{
    assert(Index >= 0);
    assert(Index < m_nNodes);
    assert(m_pHandle[Index] != HNULL);

    return m_pHandle[Index];
}

template <class T>
inline int xharray<T>::GetIndexByHandle(xhandle hHandle) const
{
    assert(hHandle != HNULL);

    for (int i = 0; i < m_nNodes; i++) {
        if (m_pHandle[i] == hHandle) {
            return i;
        }
    }

    assert(0 && "Not hash entry contain that ID");
    return 0;
}

template <class T>
inline void xharray<T>::DeleteByIndex(int Index)
{
    assert(Index >= 0);
    assert(Index < m_nNodes);

    xhandle HandleID = m_pHandle[Index];

    m_nNodes--;

    // Copy the last node into the deleted node. We don't care if it is the same.
    m_pHandle[Index] = m_pHandle[m_nNodes];
    m_pHandle[m_nNodes] = HandleID;
}

template <class T>
inline void xharray<T>::DeleteByHandle(xhandle hHandle)
{
    assert(hHandle != HNULL);
    DeleteByIndex(GetIndexByHandle(hHandle));
}

template <class T>
inline T& xharray<T>::Add(xhandle& hHandle)
{
    //
    // Grow if need it
    //
    if (m_nNodes >= m_Capacity) {
        GrowListBy(CalcGrowth());
    }

    hHandle = m_pHandle[m_nNodes];
    m_nNodes++;

    assert(hHandle != HNULL);

    // return the node
    return m_pList[hHandle.Handle];
}

template <class T>
inline T& xharray<T>::Add()
{
    xhandle hHandle;
    return Add(hHandle);
}

template <class T>
inline void xharray<T>::Clear(bool bReorder)
{
    int i;

    // reorder if the user request it
    if (bReorder) {
        for (i = 0; i < m_Capacity; i++) {
            m_pHandle[i].Handle = i;
        }
    }

    m_Capacity = 0;
    m_nNodes = 0;

    delete[] m_pHandle;
    delete[] m_pList;
    m_pHandle = nullptr;
    m_pList = nullptr;
}
