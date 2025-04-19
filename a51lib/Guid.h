#pragma once
#include <cstdint>

typedef uint64_t guid;

#define NULL_GUID 0

guid guid_New();
void guid_Init();
void guid_Kill();

class GuidLookup
{
public:
    GuidLookup();
    ~GuidLookup();

    void SetCapacity(int NGuids, bool CanGrow);

    void Add(guid GUID, uint64_t Data);
    void Add(guid GUID, int Data);
    void Add(guid GUID, void* Data);

    bool Find(guid GUID);
    bool Find(guid GUID, uint32_t Data);
    bool Find(guid GUID, int& Data);
    bool Find(guid GUID, void*& Data);

    int GetIndex(guid GUID);

    int GetNGUIDS() { return m_nNodes; }
    int GetNBytes();
    int GetCapacity() { return m_nNodesAllocated; }

    void Del(guid GUID);
    void Clear();

protected:
    struct node
    {
        guid     GUID;
        uint64_t Data;
        int      Next;
        int      Prev;
    };

    int   m_nNodes;
    int   m_nNodesAllocated;
    node* m_pNode;
    int   m_FirstFreeNode;
    int   m_nHashEntries;
    int*  m_pHashEntry;
    bool  m_CanGrow;

    void Resize(int NGuids);
    int  AllocNode(void);
    void DeallocNode(int Node);
    int  GetHash(guid GUID)
    {
        return (int)(GUID % m_nHashEntries);
    }
};

inline int GuidLookup::GetIndex(guid GUID)
{
    if (m_nHashEntries == 0) {
        return -1;
    }

    // Compute first node in hash table
    int I = m_pHashEntry[GetHash(GUID)];

    // Check linked list for match
    while (I != -1) {
        if (m_pNode[I].GUID == GUID) {
            return I;
        }
        I = m_pNode[I].Next;
    }

    // Not in list
    return -1;
}
