#include "Guid.h"
#include "stdlib.h"

#include <cassert>

GuidLookup::GuidLookup()
{
    m_nNodes = 0;
    m_nNodesAllocated = 0;
    m_pNode = nullptr;
    m_FirstFreeNode = -1;
    m_nHashEntries = 0;
    m_pHashEntry = nullptr;
    m_CanGrow = true;
}

GuidLookup::~GuidLookup()
{
    free(m_pNode);
    free(m_pHashEntry);

    m_nNodes = 0;
    m_nNodesAllocated = 0;
    m_pNode = nullptr;
    m_FirstFreeNode = -1;
    m_nHashEntries = 0;
    m_pHashEntry = nullptr;
}

void GuidLookup::SetCapacity(int NGuids, bool CanGrow)
{
    m_CanGrow = true;
    Resize(NGuids);
    m_CanGrow = CanGrow;
}

void GuidLookup::Clear()
{
    free(m_pNode);
    free(m_pHashEntry);

    m_nNodes = 0;
    m_nNodesAllocated = 0;
    m_pNode = nullptr;
    m_FirstFreeNode = -1;
    m_nHashEntries = 0;
    m_pHashEntry = nullptr;
}

// List of closest prime number above 1000*N (1>=N<=256)
static int PrimeList[] =
    {
        1009,
        2003,
        3001,
        4001,
        5003,
        6007,
        7001,
        8009,
        9001,
        10007,
        11003,
        12007,
        13001,
        14009,
        15013,
        16001,
        17011,
        18013,
        19001,
        20011,
        21001,
        22003,
        23003,
        24001,
        25013,
        26003,
        27011,
        28001,
        29009,
        30011,
        31013,
        32003,
        33013,
        34019,
        35023,
        36007,
        37003,
        38011,
        39019,
        40009,
        41011,
        42013,
        43003,
        44017,
        45007,
        46021,
        47017,
        48017,
        49003,
        50021,
        51001,
        52009,
        53003,
        54001,
        55001,
        56003,
        57037,
        58013,
        59009,
        60013,
        61001,
        62003,
        63029,
        64007,
        65003,
        66029,
        67003,
        68023,
        69001,
        70001,
        71011,
        72019,
        73009,
        74017,
        75011,
        76001,
        77003,
        78007,
        79031,
        80021,
        81001,
        82003,
        83003,
        84011,
        85009,
        86011,
        87011,
        88001,
        89003,
        90001,
        91009,
        92003,
        93001,
        94007,
        95003,
        96001,
        97001,
        98009,
        99013,
        100003,
        101009,
        102001,
        103001,
        104003,
        105019,
        106013,
        107021,
        108007,
        109001,
        110017,
        111029,
        112019,
        113011,
        114001,
        115001,
        116009,
        117017,
        118033,
        119027,
        120011,
        121001,
        122011,
        123001,
        124001,
        125003,
        126001,
        127031,
        128021,
        129001,
        130003,
        131009,
        132001,
        133013,
        134033,
        135007,
        136013,
        137029,
        138007,
        139021,
        140009,
        141023,
        142007,
        143053,
        144013,
        145007,
        146009,
        147011,
        148013,
        149011,
        150001,
        151007,
        152003,
        153001,
        154001,
        155003,
        156007,
        157007,
        158003,
        159013,
        160001,
        161009,
        162007,
        163003,
        164011,
        165001,
        166013,
        167009,
        168013,
        169003,
        170003,
        171007,
        172001,
        173021,
        174007,
        175003,
        176017,
        177007,
        178001,
        179021,
        180001,
        181001,
        182009,
        183023,
        184003,
        185021,
        186007,
        187003,
        188011,
        189011,
        190027,
        191021,
        192007,
        193003,
        194003,
        195023,
        196003,
        197003,
        198013,
        199021,
        200003,
        201007,
        202001,
        203011,
        204007,
        205019,
        206009,
        207013,
        208001,
        209021,
        210011,
        211007,
        212029,
        213019,
        214003,
        215051,
        216023,
        217001,
        218003,
        219001,
        220009,
        221021,
        222007,
        223007,
        224011,
        225023,
        226001,
        227011,
        228013,
        229003,
        230003,
        231001,
        232003,
        233021,
        234007,
        235003,
        236017,
        237011,
        238001,
        239017,
        240007,
        241013,
        242009,
        243011,
        244003,
        245023,
        246011,
        247001,
        248021,
        249017,
        250007,
        251003,
        252001,
        253003,
        254003,
        255007,
        256019,
};

void GuidLookup::Resize(int aNGuids)
{
    // Is this table allowed to grow?
    assert(m_CanGrow);

    // Only allow the system to grow
    if (aNGuids <= m_nNodesAllocated) {
        return;
    }

    int NNodes = aNGuids;

    // Decide on prime number of hash table entries
    int NHashEntries = ((NNodes / 2) * 3) / 1000;
    if (NHashEntries > 256) {
        NHashEntries = 256;
    }
    if (NHashEntries == 0) {
        NHashEntries = 1;
    }
    NHashEntries = PrimeList[NHashEntries - 1];

    // Realloc nodes
    if (m_pNode == NULL) {
        m_pNode = (node*)malloc(NNodes * sizeof(node));
        assert(m_pNode);
    } else {
        m_pNode = (node*)realloc(m_pNode, NNodes * sizeof(node));
        assert(m_pNode);
    }

    // Clear new nodes and add to free list
    for (int i = m_nNodesAllocated; i < NNodes; i++) {
        m_pNode[i].GUID = 0;
        m_pNode[i].Data = 0;
        m_pNode[i].Next = m_FirstFreeNode;
        m_pNode[i].Prev = -1;
        if (m_FirstFreeNode != -1) {
            m_pNode[m_FirstFreeNode].Prev = i;
        }
        m_FirstFreeNode = i;
    }

    // Setup new number of nodes
    m_nNodesAllocated = NNodes;

    // Free current hash table to free up memory manager
    free(m_pHashEntry);

    // Set new hash size and allocate new table
    m_nHashEntries = NHashEntries;
    m_pHashEntry = (int*)malloc(m_nHashEntries * sizeof(int));

    assert(m_pHashEntry);

    // Clear hash table entries
    for (int i = 0; i < NHashEntries; i++) {
        m_pHashEntry[i] = -1;
    }

    // Insert current nodes into new hash table
    for (int i = 0; i < m_nNodesAllocated; i++) {
        // Check if node is in use
        if (m_pNode[i].GUID > 0) {
            // Compute hash entry index
            int I = GetHash(m_pNode[i].GUID);

            // Connect to linked list
            m_pNode[i].Next = m_pHashEntry[I];
            m_pNode[i].Prev = -1;
            if (m_pHashEntry[I] != -1) {
                m_pNode[m_pHashEntry[I]].Prev = i;
            }
            m_pHashEntry[I] = i;
        }
    }
}

void GuidLookup::Add(guid GUID, uint64_t Data)
{
    int I = AllocNode();
    assert(I != -1);
    m_pNode[I].Data = Data;
    m_pNode[I].GUID = GUID;

    // Compute hash entry index
    int HI = GetHash(m_pNode[I].GUID);

    // Connect to linked list
    m_pNode[I].Next = m_pHashEntry[HI];
    m_pNode[I].Prev = -1;
    if (m_pHashEntry[HI] != -1) {
        m_pNode[m_pHashEntry[HI]].Prev = I;
    }
    m_pHashEntry[HI] = I;
}

void GuidLookup::Add(guid GUID, int Data)
{
    Add(GUID, (uint64_t)Data);
}

void GuidLookup::Add(guid GUID, void* Data)
{
    Add(GUID, (uint64_t)Data);
}

bool GuidLookup::Find(guid GUID)
{
    int I = GetIndex(GUID);
    return (I != -1);
}

bool GuidLookup::Find(guid GUID, uint32_t Data)
{
    int I = GetIndex(GUID);
    if (I != -1) {
        Data = (uint32_t)m_pNode[I].Data;
        return true;
    }

    return false;
}

bool GuidLookup::Find(guid GUID, int& Data)
{
    int I = GetIndex(GUID);
    if (I != -1) {
        Data = (int)m_pNode[I].Data;
        return true;
    }

    return false;
}

bool GuidLookup::Find(guid GUID, void*& Data)
{
    int I = GetIndex(GUID);
    if (I != -1) {
        Data = (void*)m_pNode[I].Data;
        return true;
    }

    return false;
}

int GuidLookup::AllocNode()
{
    if (m_FirstFreeNode == -1) {
        Resize(m_nNodesAllocated + 128);
    }

    // Get index of new node
    int I = m_FirstFreeNode;
    assert((I >= 0) && (I < m_nNodesAllocated));

    // Adjust free list
    m_FirstFreeNode = m_pNode[I].Next;
    if (m_FirstFreeNode != -1) {
        m_pNode[m_FirstFreeNode].Prev = -1;
    }

    // Init new node
    m_pNode[I].Data = 0;
    m_pNode[I].GUID = 0;
    m_pNode[I].Next = -1;
    m_pNode[I].Prev = -1;

    m_nNodes++;

    return I;
}

void GuidLookup::DeallocNode(int Node)
{
    // Decide hash table index
    int I = GetHash(m_pNode[Node].GUID);

    // Remove from list
    if (m_pNode[Node].Prev != -1) {
        m_pNode[m_pNode[Node].Prev].Next = m_pNode[Node].Next;
    }
    if (m_pNode[Node].Next != -1) {
        m_pNode[m_pNode[Node].Next].Prev = m_pNode[Node].Prev;
    }

    // Remove from head
    if (m_pHashEntry[I] == Node) {
        m_pHashEntry[I] = m_pNode[Node].Next;
    }

    // Add into free list
    m_pNode[Node].GUID = 0;
    m_pNode[Node].Data = 0;
    m_pNode[Node].Next = m_FirstFreeNode;
    m_pNode[Node].Prev = -1;
    if (m_FirstFreeNode != -1) {
        m_pNode[m_FirstFreeNode].Prev = Node;
    }
    m_FirstFreeNode = Node;

    m_nNodes--;
}

void GuidLookup::Del(guid GUID)
{
    int I = GetIndex(GUID);
    assert(I != -1);

    DeallocNode(I);
}

static bool s_Inited = false;
static guid s_Sequence; // Current sequence number

void guid_Init()
{
    // Be sure we are initialized only once
    if (s_Inited == true) {
        return;
    }
    s_Inited = true;

    // Start with 1 so we never create a null guid by mistake
    s_Sequence = 1;
}

void guid_Kill()
{
    assert(s_Inited == true);
    s_Inited = false;
}

guid guid_New()
{
    assert(s_Inited == true);

    return s_Sequence++;
}
