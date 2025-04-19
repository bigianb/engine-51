
#include "PolyCache.h"
#include "CollisionMgr.h"
#include "../objects/Object.h"
#include "../objectManager/ObjectManager.h"

#define POLYCACHE_CELL_SIZE (400.0f)
#define POLYCACHE_ONE_OVER_CELL_SIZE (1.0f / POLYCACHE_CELL_SIZE)

int g_n8Clusters = POLYCACHE_MAX_8_CLUSTERS;
int g_n16Clusters = POLYCACHE_MAX_16_CLUSTERS;
int g_n32Clusters = POLYCACHE_MAX_32_CLUSTERS;

//==============================================================================

Vector3    s_InvalidateMarker[128];
int        s_nInvalidateMarkers = 0;

#define DO_LOGGING (0)

guid POLYCACHE_INVALIDATE_GUID[32] = {0};
int  POLYCACHE_N_INVALIDATE_GUIDS = 0;

poly_cache::poly_cache(ObjectManager* om)
{
    objectManager = om;
    assert(POLYCACHE_MAX_CELLS > 2);

    // Initialize the cluster numbers.
    m_n8Clusters = g_n8Clusters;
    m_n16Clusters = g_n16Clusters;
    m_n32Clusters = g_n32Clusters;
    m_nSumClusters = m_n8Clusters + m_n16Clusters + m_n32Clusters;
    m_nClusterHashSize = m_nSumClusters * 2 + 1;

    // Initialize the cluster buffers.
    m_Cluster_8 = NULL;
    m_Cluster_16 = NULL;
    m_Cluster_32 = NULL;
    m_ClusterHash = NULL;

    Init(m_n8Clusters, m_n16Clusters, m_n32Clusters);
}

//==============================================================================

poly_cache::~poly_cache(void)
{
    Clear();
}

//==============================================================================

void poly_cache::Clear(void)
{
    int i;

    // Clear hash table
    for (i = 0; i < POLYCACHE_HASH_SIZE; i++) {
        m_Hash[i] = NULL;
    }

    // Clear cells
    for (i = 0; i < POLYCACHE_MAX_CELLS; i++) {
        m_Cell[i].pHashNext = NULL;
        m_Cell[i].iHash = -1;
        m_Cell[i].nClusters = 0;
        m_Cell[i].ppCluster = m_Cell[i].CLUSTERPTR;
    }

    // Setup MRU chain
    m_pMRU = &m_Cell[0];
    m_pLRU = &m_Cell[0];
    for (i = 1; i < POLYCACHE_MAX_CELLS; i++) {
        m_Cell[i].pMRUNext = NULL;
        m_Cell[i].pMRUPrev = m_pLRU;
        m_pLRU->pMRUNext = &m_Cell[i];
        m_pLRU = &m_Cell[i];
    }

    m_pGatherCell = NULL;
    m_bInsideGather = false;

    // Clear cluster hash
    memset(m_ClusterHash, 0, m_nClusterHashSize * sizeof(cluster*));

    InitClusters();

    m_Sequence = 1;
}

void poly_cache::Update(void)
{
    bool bInit = ((m_n8Clusters != g_n8Clusters) ||
                   (m_n16Clusters != g_n16Clusters) ||
                   (m_n32Clusters != g_n32Clusters));
    if (bInit) {
        Init(g_n8Clusters, g_n16Clusters, g_n32Clusters);
    }
}

//==============================================================================

void poly_cache::Init(int n8Clusters, int n16Clusters, int n32Clusters)
{
    // Has something changed?
    bool bClear = ((m_n8Clusters != n8Clusters) ||
                    (m_n16Clusters != n16Clusters) ||
                    (m_n32Clusters != n32Clusters));

    // Only if there was a change...
    if (bClear) {
        // Change 8 cluster?
        if (m_Cluster_8 && (m_n8Clusters != n8Clusters)) {
            m_n8Clusters = n8Clusters;
            free(m_Cluster_8);
            m_Cluster_8 = NULL;
        }

        // Change 16 cluster?
        if (m_Cluster_16 && (m_n16Clusters != n16Clusters)) {
            m_n16Clusters = n16Clusters;
            free(m_Cluster_16);
            m_Cluster_16 = NULL;
        }

        // Change 32 cluster?
        if (m_Cluster_32 && (m_n32Clusters != n32Clusters)) {
            m_n32Clusters = n32Clusters;
            free(m_Cluster_32);
            m_Cluster_32 = NULL;
        }

        // Always nuke the hash table.
        if (m_ClusterHash) {
            free(m_ClusterHash);
            m_ClusterHash = NULL;
        }

        // Always recalculate these if something changed...
        m_nSumClusters = m_n8Clusters + m_n16Clusters + m_n32Clusters;
        m_nClusterHashSize = m_nSumClusters * 2 + 1;
    }

    // Allocate memory.
    if (m_Cluster_8 == NULL) {
        m_Cluster_8 = (cluster_8*)malloc(m_n8Clusters * sizeof(cluster_8));
    }
    if (m_Cluster_16 == NULL) {
        m_Cluster_16 = (cluster_16*)malloc(m_n16Clusters * sizeof(cluster_16));
    }
    if (m_Cluster_32 == NULL) {
        m_Cluster_32 = (cluster_32*)malloc(m_n32Clusters * sizeof(cluster_32));
    }
    if (m_ClusterHash == NULL) {
        m_ClusterHash = (cluster**)malloc(m_nClusterHashSize * sizeof(cluster*));
    }

    Clear();
}

//==============================================================================

void poly_cache::Kill(void)
{
    Clear();
}

//==============================================================================

bool poly_cache::VerifyInFreeList(cluster* pCL)
{
    int           iB = pCL->iBank;
    cluster_bank& B = m_ClusterBank[iB];
    cluster*      pC = B.pFirstFreeCluster;

    // Loop through free list looking for pCL
    while (pC != NULL) {
        if (pC == pCL) {
            return true;
        }

        pC = pC->pNext;
    }

    return false;
}

poly_cache::cell* poly_cache::AcquireCell(int X, int Y, int Z)
{
    cell* pCell = LookupCell(X, Y, Z);

    if (pCell) {
        // Move to front of MRU if not already
        if (m_pMRU != pCell) {
            // Move cell to front of MRU
            // Remove from MRU list
            if (pCell->pMRUNext) {
                pCell->pMRUNext->pMRUPrev = pCell->pMRUPrev;
            } else {
                m_pLRU = pCell->pMRUPrev;
            }

            if (pCell->pMRUPrev) {
                pCell->pMRUPrev->pMRUNext = pCell->pMRUNext;
            } else {
                m_pMRU = pCell->pMRUNext;
            }

            // Add to MRU end
            m_pMRU->pMRUPrev = pCell;
            pCell->pMRUPrev = NULL;
            pCell->pMRUNext = m_pMRU;
            m_pMRU = pCell;
        }
    } else {
        pCell = BuildNewCell(X, Y, Z);
    }

    return pCell;
}

//==============================================================================

void poly_cache::InvalidateCell(int X, int Y, int Z)
{

    cell* pCell = LookupCell(X, Y, Z);

    if (pCell) {
        DestroyCell(pCell);
    }
}

//==============================================================================

void poly_cache::InvalidateCells(const BBox& bb, const guid& Guid)
{

    BBox IBBox = bb;
    IBBox.Inflate(1, 1, 1);

    int MinX, MinY, MinZ;
    int MaxX, MaxY, MaxZ;

    GetCellRegion(IBBox, MinX, MinY, MinZ, MaxX, MaxY, MaxZ);

    int nCells = (MaxX - MinX) * (MaxY - MinY) * (MaxZ - MinZ);

    if (POLYCACHE_N_INVALIDATE_GUIDS < 32) {
        POLYCACHE_INVALIDATE_GUID[POLYCACHE_N_INVALIDATE_GUIDS++] = Guid;
    }

    if (nCells > 1000) {
        
        // Run through list looking for cells in range!
        cell* pCell = m_pLRU;
        while (pCell) {
            cell* pNextCell = pCell->pMRUPrev;
            if ((pCell->iHash != -1) &&
                (pCell->X >= MinX) && (pCell->X <= MaxX) &&
                (pCell->Y >= MinY) && (pCell->Y <= MaxY) &&
                (pCell->Z >= MinZ) && (pCell->Z <= MaxZ)) {
                DestroyCell(pCell);
            }
            pCell = pNextCell;
        }

        return;
    } else {
        for (int X = MinX; X <= MaxX; X++) {
            for (int Y = MinY; Y <= MaxY; Y++) {
                for (int Z = MinZ; Z <= MaxZ; Z++) {
                    cell* pCell = LookupCell(X, Y, Z);

                    if (pCell) {
                        DestroyCell(pCell);
                    }
                }
            }
        }
    }
}

//==============================================================================

void poly_cache::CacheCells(const BBox& bb)
{
    BBox IBBox = bb;
    IBBox.Inflate(1, 1, 1);

    int MinX, MinY, MinZ;
    int MaxX, MaxY, MaxZ;

    GetCellRegion(IBBox, MinX, MinY, MinZ, MaxX, MaxY, MaxZ);

    for (int X = MinX; X <= MaxX; X++) {
        for (int Y = MinY; Y <= MaxY; Y++) {
            for (int Z = MinZ; Z <= MaxZ; Z++) {
                AcquireCell(X, Y, Z);
            }
        }
    }
}

//==============================================================================

void poly_cache::GetCellBBox(int X, int Y, int Z, BBox& bb)
{
    bb.min.set((float)X, (float)Y, (float)Z);
    bb.min *= POLYCACHE_CELL_SIZE;
    bb.max = bb.min;
    bb.max += Vector3(POLYCACHE_CELL_SIZE, POLYCACHE_CELL_SIZE, POLYCACHE_CELL_SIZE);
}

//==============================================================================

void poly_cache::GetCellBBox(const cell& Cell, BBox& bb)
{
    GetCellBBox(Cell.X, Cell.Y, Cell.Z, bb);
}

//==============================================================================

void poly_cache::DestroyCell(cell* pCell)
{

    //
    // Free the clusters
    //
    {
        // Check clusters for stomp

        for (int iC = 0; iC < pCell->nClusters; iC++) {
            cluster* pCL = pCell->ppCluster[iC];
            assert(pCL);

            pCL->nReferences--;
            if (pCL->nReferences == 0) {
                FreeCluster(pCell->ppCluster[iC]);
            }

            pCell->ppCluster[iC] = NULL;
        }

        // Free cluster ptr array if not internal
        if (pCell->ppCluster != pCell->CLUSTERPTR) {
            free(pCell->ppCluster);
            pCell->ppCluster = pCell->CLUSTERPTR;
        }
    }

    // Find head of this chain and find previous cell in hash chain
    // for easy removal
    cell* pPrevC = NULL;
    cell* pC = m_Hash[pCell->iHash];
    while (pC) {
        if (pC == pCell) {
            break;
        }
        pPrevC = pC;
        pC = pC->pHashNext;
    }
    assert(pC); // Could not find chain in hash table!?!

    // Remove pCell from hash table
    if (pPrevC) {
        pPrevC->pHashNext = pCell->pHashNext;
    } else {
        m_Hash[pCell->iHash] = pCell->pHashNext;
    }

    // Remove from MRU list
    if (pCell->pMRUNext) {
        pCell->pMRUNext->pMRUPrev = pCell->pMRUPrev;
    } else {
        m_pLRU = pCell->pMRUPrev;
    }

    if (pCell->pMRUPrev) {
        pCell->pMRUPrev->pMRUNext = pCell->pMRUNext;
    } else {
        m_pMRU = pCell->pMRUNext;
    }

    // Add to LRU end
    m_pLRU->pMRUNext = pCell;
    pCell->pMRUPrev = m_pLRU;
    pCell->pMRUNext = NULL;
    m_pLRU = pCell;

    pCell->iHash = -1;
    pCell->pHashNext = NULL;
    pCell->X = 0;
    pCell->Y = 0;
    pCell->Z = 0;
    pCell->nClusters = 0;
}

//==============================================================================

void poly_cache::GetCellRegion(const BBox& aBBox,
                               int& MinX, int& MinY, int& MinZ,
                               int& MaxX, int& MaxY, int& MaxZ)
{

    // Clip bbox to objmgr limits
    BBox bb = aBBox;
    BBox LBBox = objectManager->GetSafeBBox();

    bb.min.Max(LBBox.min);
    bb.max.Min(LBBox.max);

    MinX = (int)((bb.min.x * POLYCACHE_ONE_OVER_CELL_SIZE) + 4096.0f) - 4096;
    MinY = (int)((bb.min.y * POLYCACHE_ONE_OVER_CELL_SIZE) + 4096.0f) - 4096;
    MinZ = (int)((bb.min.z * POLYCACHE_ONE_OVER_CELL_SIZE) + 4096.0f) - 4096;
    MaxX = (int)((bb.max.x * POLYCACHE_ONE_OVER_CELL_SIZE) + 4096.0f) - 4096;
    MaxY = (int)((bb.max.y * POLYCACHE_ONE_OVER_CELL_SIZE) + 4096.0f) - 4096;
    MaxZ = (int)((bb.max.z * POLYCACHE_ONE_OVER_CELL_SIZE) + 4096.0f) - 4096;
}

//==============================================================================

void poly_cache::InvalidateAllCells(void)
{

    while (m_pMRU->iHash != -1) {
        DestroyCell(m_pMRU);
    }
}

//==============================================================================

poly_cache::cell* poly_cache::BuildNewCell(int X, int Y, int Z)
{

    //
    // Allocate a new cell
    //
    if (m_pLRU->iHash != -1) {
        // Look for a cell with the fewest number of clusters
        // on the LRU end of the list
        int   I = 16;
        cell* pCell = m_pLRU;
        cell* pBestCell = NULL;
        int   BestScore = INT_MAX;
        while (pCell) {
            if (--I == 0) {
                break;
            }

            if (pCell->nClusters < BestScore) {
                BestScore = pCell->nClusters;
                pBestCell = pCell;
                if (BestScore == 0) {
                    break;
                }
            }
        }
        assert(pBestCell);

        DestroyCell(pBestCell);
    }
    assert(m_pLRU->iHash == -1);

    //
    // Pull from the LRU end of the list
    //
    cell& C = *m_pLRU;
    m_pLRU = C.pMRUPrev;
    assert(m_pLRU);
    m_pLRU->pMRUNext = NULL;

    //
    // At this point the cell does not exist in the MRU/LRU list
    //

    //
    // Fill out basics
    //
    {
        C.iHash = ComputeHash(X, Y, Z);
        ;
        C.X = X;
        C.Y = Y;
        C.Z = Z;
        C.pHashNext = m_Hash[C.iHash];
        m_Hash[C.iHash] = &C;
        C.nClusters = 0;
    }

    //
    // Compute bbox containing cell and inflate for safety
    //
    BBox CellBBox;
    GetCellBBox(X, Y, Z, CellBBox);
    CellBBox.Inflate(1, 1, 1);

    //
    // Gather clusters into cell
    //
    GatherClustersIntoCell(C, CellBBox);

    //
    // Insert cell at head of MRU list
    //
    C.pMRUPrev = NULL;
    C.pMRUNext = m_pMRU;
    m_pMRU->pMRUPrev = &C;
    m_pMRU = &C;

    // Finish timing and return cell

    return &C;
}

//==============================================================================

void poly_cache::InitClusters(void)
{
    int i;
    //
    // Allocate the '8' bank
    //

    m_ClusterBank[0].nClusters = POLYCACHE_MAX_8_CLUSTERS;
    m_ClusterBank[0].nPoints = 8;
    m_ClusterBank[0].nNormals = 8;
    m_ClusterBank[0].nQuads = 8;
    m_ClusterBank[0].iBank = 0;
    m_ClusterBank[0].nFree = POLYCACHE_MAX_8_CLUSTERS;
    m_ClusterBank[0].pFirstFreeCluster = (cluster*)m_Cluster_8;
    m_ClusterBank[0].pCluster = (cluster*)m_Cluster_8;
    m_ClusterBank[0].ClusterSize = sizeof(cluster_8);
    for (i = 0; i < POLYCACHE_MAX_8_CLUSTERS; i++) {
        m_Cluster_8[i].pNext = (cluster*)(&m_Cluster_8[i + 1]);
        m_Cluster_8[i].iBank = 0;
        m_Cluster_8[i].pPoint = m_Cluster_8[i].POINT;
        m_Cluster_8[i].pNormal = m_Cluster_8[i].NORMAL;
        m_Cluster_8[i].pQuad = m_Cluster_8[i].QUAD;
        m_Cluster_8[i].pBounds = m_Cluster_8[i].BOUNDS;
        m_Cluster_8[i].nReferences = 0;

    }
    m_Cluster_8[POLYCACHE_MAX_8_CLUSTERS - 1].pNext = NULL;

    //
    // Allocate the '16' bank
    //

    m_ClusterBank[1].nClusters = POLYCACHE_MAX_16_CLUSTERS;
    m_ClusterBank[1].nPoints = 16;
    m_ClusterBank[1].nNormals = 16;
    m_ClusterBank[1].nQuads = 16;
    m_ClusterBank[1].iBank = 1;
    m_ClusterBank[1].nFree = POLYCACHE_MAX_16_CLUSTERS;
    m_ClusterBank[1].pFirstFreeCluster = (cluster*)m_Cluster_16;
    m_ClusterBank[1].pCluster = (cluster*)m_Cluster_16;
    m_ClusterBank[1].ClusterSize = sizeof(cluster_16);
    for (i = 0; i < POLYCACHE_MAX_16_CLUSTERS; i++) {
        m_Cluster_16[i].pNext = (cluster*)(&m_Cluster_16[i + 1]);
        m_Cluster_16[i].iBank = 1;
        m_Cluster_16[i].pPoint = m_Cluster_16[i].POINT;
        m_Cluster_16[i].pNormal = m_Cluster_16[i].NORMAL;
        m_Cluster_16[i].pQuad = m_Cluster_16[i].QUAD;
        m_Cluster_16[i].pBounds = m_Cluster_16[i].BOUNDS;
        m_Cluster_16[i].nReferences = 0;
    }
    m_Cluster_16[POLYCACHE_MAX_16_CLUSTERS - 1].pNext = NULL;

    //
    // Allocate the '32' bank
    //

    m_ClusterBank[2].nClusters = POLYCACHE_MAX_32_CLUSTERS;
    m_ClusterBank[2].nPoints = 32;
    m_ClusterBank[2].nNormals = 32;
    m_ClusterBank[2].nQuads = 32;
    m_ClusterBank[2].iBank = 2;
    m_ClusterBank[2].nFree = POLYCACHE_MAX_32_CLUSTERS;
    m_ClusterBank[2].pFirstFreeCluster = (cluster*)m_Cluster_32;
    m_ClusterBank[2].pCluster = (cluster*)m_Cluster_32;
    m_ClusterBank[2].ClusterSize = sizeof(cluster_32);
    for (i = 0; i < POLYCACHE_MAX_32_CLUSTERS; i++) {
        m_Cluster_32[i].pNext = (cluster*)(&m_Cluster_32[i + 1]);
        m_Cluster_32[i].iBank = 2;
        m_Cluster_32[i].pPoint = m_Cluster_32[i].POINT;
        m_Cluster_32[i].pNormal = m_Cluster_32[i].NORMAL;
        m_Cluster_32[i].pQuad = m_Cluster_32[i].QUAD;
        m_Cluster_32[i].pBounds = m_Cluster_32[i].BOUNDS;
        m_Cluster_32[i].nReferences = 0;
    }
    m_Cluster_32[POLYCACHE_MAX_32_CLUSTERS - 1].pNext = NULL;
}

//==============================================================================

poly_cache::cluster* poly_cache::FindCluster(guid Guid, int iCollDataCluster)
{
    int      iHash = ComputeClusterHash(Guid, iCollDataCluster);
    cluster* pCL = m_ClusterHash[iHash];
    while (pCL != NULL) {
        if ((pCL->Guid == Guid) && (pCL->iCollDataCluster == iCollDataCluster)) {
            assert(pCL->iCollDataCluster < 1024);
            return pCL;
        }

        pCL = pCL->pNext;
    }

    return NULL;
}

//==============================================================================

poly_cache::cluster* poly_cache::AllocCluster(int  nPoints,
                                              int  nNormals,
                                              int  nQuads,
                                              guid Guid,
                                              int  iCollDataCluster)
{
    // Find bank best suited to inputs
    cluster_bank* pCB = NULL;
    int           i;
    for (i = 0; i < POLYCACHE_NUM_CLUSTER_BANKS; i++) {
        if ((m_ClusterBank[i].nPoints >= nPoints) &&
            (m_ClusterBank[i].nNormals >= nNormals) &&
            (m_ClusterBank[i].nQuads >= nQuads)) {
            pCB = &m_ClusterBank[i];
            break;
        }
    }
    // There was no bank that could fit the request!!!
    assert(pCB);

    // Check if there are any clusters available?
    cluster* pCL = NULL;
    while (1) {
        // Is there a cluster available?
        if (pCB->pFirstFreeCluster) {
            pCL = pCB->pFirstFreeCluster;
            assert(pCL->nReferences == 0);
            pCB->pFirstFreeCluster = pCL->pNext;
            pCL->pNext = NULL;
            pCB->nFree--;
            break;
        }

        //
        // There was no cluster available so run up LRU and nuke
        // the first cell that has one!
        //
        int   I = 0;
        int   FirstVictimI = -1;
        int   BestScore = INT_MIN;
        cell* pBestVictim = NULL;
        cell* pVictim = m_pLRU;
        while (pVictim) {
            // If we've checked a number of cells past first victim then kill first victim
            if (pBestVictim && ((I - FirstVictimI) > 8)) {
                break;
            }

            assert(pVictim != m_pGatherCell);

            int nCorrectComplete = 0;
            int nIncorrectComplete = 0;
            int nCorrectIncomplete = 0;
            int nIncorrectIncomplete = 0;

            //
            // We'll nuke the cell only if it has a cluster of
            // the bank type we are interested in.
            //
            int i;
            for (i = 0; i < pVictim->nClusters; i++) {
                cluster& CL = *pVictim->ppCluster[i];

                if (CL.nReferences == 1) {
                    if ((int)CL.iBank == pCB->iBank) {
                        nCorrectComplete++;
                    } else {
                        nIncorrectComplete++;
                    }
                } else {
                    if ((int)CL.iBank == pCB->iBank) {
                        nCorrectIncomplete++;
                    } else {
                        nIncorrectIncomplete++;
                    }
                }
            }

            // Compute score for this cell
            if (nCorrectComplete || nCorrectIncomplete) {
                if (pBestVictim == NULL) {
                    FirstVictimI = I;
                }

                int Score = nCorrectComplete * (+1000) +
                            nCorrectIncomplete * (+100) +
                            nIncorrectComplete * (-10) +
                            nIncorrectIncomplete * (-1);

                if (Score > BestScore) {
                    BestScore = Score;
                    pBestVictim = pVictim;
                }
            }

            // Remember best score

            // Go to next possible victim
            pVictim = pVictim->pMRUPrev;
            I++;
        }

        assert(pBestVictim && (pBestVictim->iHash != -1));

        //LOG_MESSAGE("POLYCACHE","BestVictim: %d\n",BestScore);
        DestroyCell(pBestVictim);
    }
    assert(pCL);
    assert(pCL->nReferences == 0);

    // Fill out basic info
    pCL->Guid = Guid;
    pCL->iCollDataCluster = iCollDataCluster;
    pCL->nNormals = nNormals;
    pCL->nPoints = nPoints;
    pCL->nQuads = nQuads;
    pCL->nReferences = 1;
    pCL->nHits = 0;
    pCL->Sequence = 0;

    //
    // Add cluster to hash
    //
    {
        int iHash = ComputeClusterHash(Guid, iCollDataCluster);
        pCL->pNext = m_ClusterHash[iHash];
        m_ClusterHash[iHash] = pCL;
    }

    return pCL;
}

//==============================================================================

void poly_cache::FreeCluster(cluster* pCluster)
{

    //
    // Decrement references
    //
    assert(pCluster->nReferences == 0);

    //
    // Remove cluster from hash
    //
    {
        int      iHash = ComputeClusterHash(pCluster->Guid, pCluster->iCollDataCluster);
        cluster* pPrevC = NULL;
        cluster* pC = m_ClusterHash[iHash];
        while (pC) {
            if (pC == pCluster) {
                break;
            }
            pPrevC = pC;
            pC = pC->pNext;
        }
        assert(pC); // Could not find chain in hash table!?!

        // Remove pCell from hash table
        if (pPrevC) {
            pPrevC->pNext = pC->pNext;
        } else {
            assert(m_ClusterHash[iHash] == pC);
            m_ClusterHash[iHash] = pC->pNext;
        }

        pC->pNext = NULL;
    }

    // Get ref to cluster's bank
    assert(pCluster->iBank < POLYCACHE_NUM_CLUSTER_BANKS);
    cluster_bank& CB = m_ClusterBank[pCluster->iBank];

    // Put cluster bank into free list
    pCluster->pNext = CB.pFirstFreeCluster;
    CB.pFirstFreeCluster = pCluster;
    CB.nFree++;
    pCluster->nReferences = 0;
    pCluster->nHits = 0;
    pCluster->Guid = 0;
    pCluster->iCollDataCluster = 1024;
}

//==============================================================================

int poly_cache::ComputeClusterHash(guid Guid, int iCollDataCluster)
{
    //return ((uint32_t)((/*(Guid>>32) ^*/ (Guid)) ^ iCollDataCluster)) % POLYCACHE_CLUSTER_HASH_SIZE;
    return ((uint32_t)(((uint32_t)(Guid >> 16)) ^ iCollDataCluster)) % POLYCACHE_CLUSTER_HASH_SIZE;
}

//==============================================================================

void poly_cache::GatherClustersIntoCell(cell& Cell, const BBox& CellBBox)
{

    // Be sure there isn't a nested gather
    assert(m_pGatherCell == NULL);

    //
    // Setup necessary structures to start accepting Apply's
    //
    m_GatherBBox = CellBBox;
    m_pGatherCell = &Cell;
    m_nGatherClusters = 0;
    m_nGatherClustersAlreadyCached = 0;

    //
    // Run through playsurfaces
    //
    /* TODO: Uncomment this code when the playsurface manager is available
    if (1) {
        g_PlaySurfaceMgr.CollectSurfaces(m_GatherBBox, Object::ATTR_COLLIDABLE, Object::ATTR_COLLISION_PERMEABLE);
        playsurface_mgr::surface* pSurface = g_PlaySurfaceMgr.GetNextSurface();
        while (pSurface != NULL) {
            playsurface_mgr::surface* pS = pSurface;
            guid                      SurfaceGuid = g_PlaySurfaceMgr.GetPlaySurfaceGuid();
            pSurface = g_PlaySurfaceMgr.GetNextSurface();

            rigid_geom* pGeom = (rigid_geom*)render::GetGeom(pS->RenderInst);

            RigidGeom_GatherToPolyCache(SurfaceGuid, pS->WorldBBox, (uint64_t)-1, &pS->L2W, pGeom);
        }
    }
*/
    //
    // Run through other types of objects derived from play_surface
    //
    if (1) {
        objectManager->SelectBBox(Object::ATTR_COLLIDABLE, m_GatherBBox, Object::TYPE_ALL_TYPES, Object::ATTR_COLLISION_PERMEABLE);

        for (slot_id aID = objectManager->StartLoop(); aID != SLOT_NULL; aID = objectManager->GetNextResult(aID)) {
            Object* pObject = objectManager->GetObjectBySlot(aID);
            if (pObject) {
                pObject->OnPolyCacheGather();
            }
        }

        objectManager->EndLoop();
    }

    //
    // Copy list of clusters into cell
    //
    m_pGatherCell->nClusters = m_nGatherClusters;
    if (m_nGatherClusters > POLYCACHE_MAX_CLUSTER_PTRS_PER_CELL) {
        //LOG_MESSAGE("POLYCACHE","Allocating cell cluster array %d %d",POLYCACHE_MAX_CLUSTER_PTRS_PER_CELL,m_nGatherClusters);
        m_pGatherCell->ppCluster = (cluster**)malloc(sizeof(cluster*) * m_nGatherClusters);
        assert(m_pGatherCell->ppCluster);
    } else {
        m_pGatherCell->ppCluster = m_pGatherCell->CLUSTERPTR;
    }

    // Copy cluster ptrs into cell and increase references to clusters
    for (int i = 0; i < m_nGatherClusters; i++) {
        m_pGatherCell->ppCluster[i] = m_GatherClusterList[i];
    }

    //
    // Shut off possibility of applying additional primitives
    //
    m_pGatherCell = NULL;
}

//==============================================================================

const BBox& poly_cache::GetGatherBBox()
{
    return m_GatherBBox;
}

//==============================================================================

void poly_cache::GatherCluster(const CollisionData& CollData,
                               const Matrix4*        pL2W,
                               uint64_t                 MeshMask,
                               guid                  Guid)
{
    assert(pL2W);
    if (!pL2W) {
        return;
    }

    Object* pObj = objectManager->GetObjectByGuid(Guid);
    if (!pObj) {
        return;
    }

    // Get object type and attrbits
    Object::type ObjectType = pObj->GetType();
    uint32_t          AttrBits = pObj->GetAttrBits();

    // Loop through the low_clusters
    for (int iCL = 0; iCL < CollData.numLowClusters; iCL++) {
        CollisionData::LowCluster& CL = CollData.lowClusters[iCL];

        uint64_t Bit = 1 << CL.iMesh;
        if (!(MeshMask & Bit)) {
            continue;
        }

        // Transform local bbox into world
        BBox WorldBBox = CL.bbox;
        WorldBBox.Transform(pL2W[CL.iBone]);

        // Check if we want this cluster
        if (WorldBBox.Intersect(m_GatherBBox)) {
            cluster* pCL = NULL;

            // Check if cluster has already been cached
            if (!(pCL = FindCluster(Guid, iCL))) {
                // Allocate a cluster to cache into
                pCL = AllocCluster(CL.nPoints, CL.nNormals, CL.nQuads, Guid, iCL);
                assert(pCL);

                // Most of the basic info was filled out in the Alloc()
                pCL->ObjectType = ObjectType;
                pCL->AttrBits = AttrBits;
                pCL->iBone = CL.iBone;
                pCL->PrimKey = (((uint32_t)iCL) << 16) | ((uint32_t)CL.iBone);

                // Transform points
                {
                    //-------------------------------------------------------
                    const Matrix4& M = pL2W[CL.iBone];
                    Vector3*       pS = CollData.lowVectors + CL.iVectorOffset;
                    Vector3*       pD = pCL->pPoint;
                    int            N = pCL->nPoints;

                    //-------------------------------------------------------
                    BBox bb;
                    bb.Clear();
                    while (N--) {
                        *pD = M.Transform(*pS);
                        bb += *pD;
                        pD++;
                        pS++;
                    }
                    bb.Inflate(1.0f, 1.0f, 1.0f);
                    pCL->m_BBox = bb;
                }

                // Transform normals
                {
                    Vector3* pS = CollData.lowVectors + CL.iVectorOffset + CL.nPoints;
                    Vector3* pD = pCL->pNormal;
                    int      N = pCL->nNormals;

                    const Matrix4& M = pL2W[CL.iBone];
                    while (N--) {
                        *pD = M.RotateVector(*pS);
                        pD++;
                        pS++;
                    }
                    //#endif
                }

                // Copy over quad info and generate other information
                for (int i = 0; i < CL.nQuads; i++) {
                    CollisionData::LowQuad& QD = CollData.lowQuads[CL.iQuadOffset + i];
                    cluster::quad&            CQD = pCL->pQuad[i];

                    CQD.iP[0] = QD.iP[0];
                    CQD.iP[1] = QD.iP[1];
                    CQD.iP[2] = QD.iP[2];
                    CQD.iP[3] = QD.iP[3];
                    CQD.iN = QD.iN;

                    // Clear the bbox and build it
                    BBox& bb = *((BBox*)&(pCL->pBounds[i]));
                    //-------------------------------------------------------
                    bb.Clear();
                    bb += pCL->pPoint[CQD.iP[0]];
                    bb += pCL->pPoint[CQD.iP[1]];
                    bb += pCL->pPoint[CQD.iP[2]];
                    bb += pCL->pPoint[CQD.iP[3]];
                    bb.Inflate(1, 1, 1);

                    // Clear flags
                    pCL->pBounds[i].Flags = 0;

                    // Remember if this is a quad
                    if (pCL->pPoint[CQD.iP[0]] != pCL->pPoint[CQD.iP[3]]) {
                        pCL->pBounds[i].Flags |= BOUNDS_IS_QUAD;
                    }

                    // Compute D value of plane
                    Vector4 N = pCL->pNormal[CQD.iN];
                    N.w = 0;
                    pCL->pBounds[i].PlaneD = -N.Dot(pCL->pPoint[CQD.iP[0]]);

                    // Set other normal flags
                    if (N.x > +0.999f) {
                        pCL->pBounds[i].Flags |= BOUNDS_X_POS;
                    }
                    if (N.x < -0.999f) {
                        pCL->pBounds[i].Flags |= BOUNDS_X_NEG;
                    }
                    if (N.y > +0.999f) {
                        pCL->pBounds[i].Flags |= BOUNDS_Y_POS;
                    }
                    if (N.y < -0.999f) {
                        pCL->pBounds[i].Flags |= BOUNDS_Y_NEG;
                    }
                    if (N.z > +0.999f) {
                        pCL->pBounds[i].Flags |= BOUNDS_Z_POS;
                    }
                    if (N.z < -0.999f) {
                        pCL->pBounds[i].Flags |= BOUNDS_Z_NEG;
                    }
                }
            } else {
                m_nGatherClustersAlreadyCached++;
                pCL->nReferences++;
            }

            //
            // Add cluster to list for this cell!
            //
            assert(m_nGatherClusters < 128);
            assert(pCL->iCollDataCluster < 1024);
            m_GatherClusterList[m_nGatherClusters] = pCL;
            m_nGatherClusters++;
        }
    }
}

//==============================================================================

void poly_cache::BuildClusterList(const BBox&  aBBox,
                                  Object::type ThisType,
                                  uint32_t          TheseAttributes,
                                  uint32_t          NotTheseAttributes,
                                  const guid*  pIgnoreList,
                                  int          nIgnores)
{
    BBox bb = aBBox;
    bb.Inflate(1, 1, 1);

    m_nClusters = 0;

    IncrementSequence();

    int MinX, MinY, MinZ;
    int MaxX, MaxY, MaxZ;
    GetCellRegion(bb, MinX, MinY, MinZ, MaxX, MaxY, MaxZ);

    for (int X = MinX; X <= MaxX; X++) {
        for (int Y = MinY; Y <= MaxY; Y++) {
            for (int Z = MinZ; Z <= MaxZ; Z++) {
                cell* pCell = AcquireCell(X, Y, Z);

                // Loop through clusters in cell and add to list
                for (int i = 0; i < pCell->nClusters; i++) {
                    cluster* pCL = pCell->ppCluster[i];

                    if (pCL->Sequence == m_Sequence) {
                        continue;
                    }
                    pCL->Sequence = m_Sequence;

                    if ((pCL->AttrBits & TheseAttributes) == 0) {
                        continue;
                    }

                    if ((pCL->AttrBits & NotTheseAttributes) != 0) {
                        continue;
                    }

                    // Make sure that it matches the type
                    if ((ThisType != Object::TYPE_ALL_TYPES) && (pCL->ObjectType != ThisType)) {
                        continue;
                    }

                    if (bb.Intersect(pCL->m_BBox)) {
                        // Check if cluster is already listed
                        {
                            // Check if it's in the ignore list
                            int k = 0;
                            for (k = 0; k < nIgnores; k++) {
                                if (pCL->Guid == pIgnoreList[k]) {
                                    break;
                                }
                            }

                            if (k == nIgnores) {
                                assert(m_nClusters < POLYCACHE_MAX_CLUSTERS_IN_LIST);
                                if (m_nClusters < POLYCACHE_MAX_CLUSTERS_IN_LIST) {
                                    m_ClusterList[m_nClusters] = pCL;
                                    m_nClusters++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

//==============================================================================

void poly_cache::BeginRayClusterWalk(const Vector3& RayStart,
                                     const Vector3& RayEnd,
                                     Object::type   ThisType,
                                     uint32_t            TheseAttributes,
                                     uint32_t            NotTheseAttributes,
                                     const guid*    pIgnoreList,
                                     int            nIgnores)
{
    // Clear cluster array.
    m_nClusters = 0;
    m_iNextCluster = 0;

    // Setup initial ray grid walking information
    m_GridWalker.Setup(RayStart, RayEnd, Vector3(0, 0, 0), POLYCACHE_CELL_SIZE);

    // Copy
    m_RayStart = RayStart;
    m_RayEnd = RayEnd;
    m_ThisType = ThisType;
    m_TheseAttributes = TheseAttributes;
    m_NotTheseAttributes = NotTheseAttributes;
    m_pIgnoreList = pIgnoreList;
    m_nIgnores = nIgnores;

    IncrementSequence();
}

//==============================================================================

bool poly_cache::StepRayClusterWalker()
{
    m_nClusters = 0;
    m_iNextCluster = 0;

    while (m_nClusters == 0) {
        // Pull out cell we are currently in
        int X, Y, Z;
        m_GridWalker.GetCell(X, Y, Z);

        // Check for any clusters in this cell
        {
            cell* pCell = AcquireCell(X, Y, Z);

            // Loop through clusters in cell and add to list
            for (int i = 0; i < pCell->nClusters; i++) {
                cluster* pCL = pCell->ppCluster[i];

                // Use sequence number
                if (pCL->Sequence == m_Sequence) {
                    continue;
                }
                pCL->Sequence = m_Sequence;

                if ((pCL->AttrBits & m_TheseAttributes) == 0) {
                    continue;
                }

                if ((pCL->AttrBits & m_NotTheseAttributes) != 0) {
                    continue;
                }

                // Make sure that it matches the type
                if ((m_ThisType != Object::TYPE_ALL_TYPES) && (pCL->ObjectType != m_ThisType)) {
                    continue;
                }

                {
                    // Check if it's in the ignore list

                    int k = 0;
                    for (k = 0; k < m_nIgnores; k++) {
                        if (pCL->Guid == m_pIgnoreList[k]) {
                            break;
                        }
                    }

                    if (k == m_nIgnores) {
                        assert(m_nClusters < POLYCACHE_MAX_CLUSTERS_IN_LIST);
                        if (m_nClusters < POLYCACHE_MAX_CLUSTERS_IN_LIST) {
                            m_ClusterList[m_nClusters] = pCL;
                            m_nClusters++;
                        }
                    }
                }
            }
        }

        // Step the grid walker
        if (!m_GridWalker.Step()) {
            return false;
        }
    }

    return true;
}

//==============================================================================

void poly_cache::ClearSequence(void)
{
    // Clear sequence for all clusters
    m_Sequence = 1;

    // Loop through banks
    for (int iB = 0; iB < POLYCACHE_NUM_CLUSTER_BANKS; iB++) {
        cluster_bank& B = m_ClusterBank[iB];

        // Loop through all the clusters
        for (int i = 0; i < B.nClusters; i++) {
            cluster& CL = *((cluster*)(((uint8_t*)(B.pCluster)) + (i * B.ClusterSize)));
            CL.Sequence = 0;
        }
    }
}
