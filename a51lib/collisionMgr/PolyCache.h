#pragma once

#include "../VectorMath.h"
#include "../Guid.h"
#include "../objects/Object.h"
#include "GridWalker.h"
#include "../render/CollisionVolume.h"

#define POLYCACHE_MAX_CELLS 800
#define POLYCACHE_HASH_SIZE 2053
#define POLYCACHE_MAX_8_CLUSTERS 1024
#define POLYCACHE_MAX_16_CLUSTERS 512
#define POLYCACHE_MAX_32_CLUSTERS 512
#define POLYCACHE_NUM_CLUSTER_BANKS 3
#define POLYCACHE_MAX_CLUSTER_PTRS_PER_CELL 32
#define POLYCACHE_MAX_CLUSTERS_IN_LIST 1024

#define POLYCACHE_MAX_CLUSTERS (POLYCACHE_MAX_8_CLUSTERS + POLYCACHE_MAX_16_CLUSTERS + POLYCACHE_MAX_32_CLUSTERS)
#define POLYCACHE_CLUSTER_HASH_SIZE ((POLYCACHE_MAX_CLUSTERS * 2) + 1)
#define POLYCACHE_MAX_SEQUENCE (65535)

class ObjectManager;

class poly_cache
{
    //------------------------------------------------------------------------------
    //  Public structures
    //------------------------------------------------------------------------------
public:
    struct cluster
    {

#define BOUNDS_X_POS (1 << 0)
#define BOUNDS_X_NEG (1 << 1)
#define BOUNDS_Y_POS (1 << 2)
#define BOUNDS_Y_NEG (1 << 3)
#define BOUNDS_Z_POS (1 << 4)
#define BOUNDS_Z_NEG (1 << 5)
#define BOUNDS_IS_QUAD (1 << 6)

        struct bounds
        {
            Vector3  BBoxMin;
            Vector3  BBoxMax;
            uint32_t Flags;
            float    PlaneD;
        };

        struct quad
        {
            uint8_t iP[4];
            uint8_t iN;
        };

        BBox     m_BBox;
        guid     Guid;
        uint32_t AttrBits;
        uint32_t PrimKey;

        Object::type ObjectType;
        uint8_t      nPoints;
        uint8_t      nNormals;
        uint8_t      nQuads;
        uint8_t      iBank;
        uint8_t      iBone;
        uint8_t      nReferences;
        uint8_t      nHits;
        uint16_t     iCollDataCluster;
        uint16_t     Sequence;

        cluster* pNext;
        Vector3* pPoint;
        Vector3* pNormal;
        quad*    pQuad;
        bounds*  pBounds;
    };

    struct cell
    {
        cell*     pHashNext;
        cell*     pMRUNext;
        cell*     pMRUPrev;
        int16_t   X, Y, Z;
        int16_t   iHash;
        int16_t   nClusters;
        cluster** ppCluster;

        cluster* CLUSTERPTR[POLYCACHE_MAX_CLUSTER_PTRS_PER_CELL];
    };

    struct cluster_32 : public cluster
    {
        Vector3 POINT[32];
        Vector3 NORMAL[32];
        quad    QUAD[32];
        bounds  BOUNDS[32];
    };

    struct cluster_16 : public cluster
    {
        Vector3 POINT[16];
        Vector3 NORMAL[16];
        quad    QUAD[16];
        bounds  BOUNDS[16];
    };

    struct cluster_8 : public cluster
    {
        Vector3 POINT[8];
        Vector3 NORMAL[8];
        quad    QUAD[8];
        bounds  BOUNDS[8];
    };

    struct cluster_bank
    {
        int      nQuads;
        int      nPoints;
        int      nNormals;
        int      nClusters;
        int      nFree;
        int      iBank;
        cluster* pFirstFreeCluster;
        cluster* pCluster;
        int      ClusterSize;
        int      PointOffset;
        int      NormalOffset;
        int      QuadOffset;
        int      BoundsOffset;
    };

    //------------------------------------------------------------------------------
    //  Public Functions
    //------------------------------------------------------------------------------
public:
    poly_cache(ObjectManager* pObjMgr = nullptr);
    ~poly_cache();

    void setObjectManager(ObjectManager* pObjMgr) { objectManager = pObjMgr; }

    void Init(int n8Clusters,
              int n16Clusters,
              int n32Clusters);

    void Kill(void);
    void Update(void);
    void ClearStats(void);

    void SanityCheck(void);
    void Render(void);

    cell* AcquireCell(int X, int Y, int Z);
    void  InvalidateCell(int X, int Y, int Z);
    void  InvalidateCells(const BBox& bb, const guid& Guid);
    void  InvalidateAllCells(void);
    void  CacheCells(const BBox& bb);

    void GetCellBBox(int X, int Y, int Z, BBox& bb);
    void GetCellBBox(const cell& Cell, BBox& bb);

    void GetCellRegion(const BBox& bb,
                       int& MinX, int& MinY, int& MinZ,
                       int& MaxX, int& MaxY, int& MaxZ);

    //
    // Used by objects when poly_cache is gathering triangles
    //
    const BBox& GetGatherBBox(void);

    void GatherCluster(const CollisionData& CollData,
                       const Matrix4*       pL2W,
                       uint64_t             MeshMask,
                       guid                 Guid);

    //------------------------------------------------------------------------------

public:
    // This routine and members are for collecting all the clusters
    // intersected by a bbox
    void BuildClusterList(const BBox&  bb,
                          Object::type ThisType,
                          uint32_t     TheseAttributes,
                          uint32_t     NotTheseAttributes,
                          const guid*  pIgnoreList = NULL,
                          int          nIgnores = 0);

    cluster* m_ClusterList[POLYCACHE_MAX_CLUSTERS_IN_LIST];
    int      m_nClusters;

public:
    // These routines are for walking a ray through the polycache
    void BeginRayClusterWalk(const Vector3& RayStart,
                             const Vector3& RayEnd,
                             Object::type   ThisType,
                             uint32_t       TheseAttributes,
                             uint32_t       NotTheseAttributes,
                             const guid*    pIgnoreList = NULL,
                             int            nIgnores = 0);

    cluster* GetNextClusterFromRayWalk();

private:
    bool         StepRayClusterWalker();
    Object::type m_ThisType;
    uint32_t     m_TheseAttributes;
    uint32_t     m_NotTheseAttributes;
    const guid*  m_pIgnoreList;
    int          m_nIgnores;
    grid_walker  m_GridWalker;
    int          m_iNextCluster;
    Vector3      m_RayStart;
    Vector3      m_RayEnd;

    //------------------------------------------------------------------------------
    //  Private Functions
    //------------------------------------------------------------------------------
private:
    void Clear(void);
    int  ComputeHash(int X, int Y, int Z);

    cell* LookupCell(int X, int Y, int Z);
    cell* BuildNewCell(int X, int Y, int Z);
    void  DestroyCell(cell* pCell);

    void GatherClustersIntoCell(cell& Cell, const BBox& CellBBox);

    void     InitClusters(void);
    cluster* AllocCluster(int nPoints, int nNormals, int nQuads, guid Guid, int iCollDataCluster);
    cluster* FindCluster(guid Guid, int iCollDataCluster);
    void     FreeCluster(cluster* pCluster);
    int      ComputeClusterHash(guid Guid, int iCollDataCluster);
    void     DrawCluster(cluster* pCL, float* Intensity);
    void     DrawClusterNormals(cluster* pCL);

    void SanityCheckFreeClusterList(void);
    bool VerifyInFreeList(cluster* pCL);

    void IncrementSequence(void);
    void ClearSequence(void);

    //------------------------------------------------------------------------------
    //  Public Data
    //------------------------------------------------------------------------------
public:
    cell m_Cell[POLYCACHE_MAX_CELLS];

private:
    cluster_8*  m_Cluster_8;
    cluster_16* m_Cluster_16;
    cluster_32* m_Cluster_32;
    cluster**   m_ClusterHash;

    cluster_bank m_ClusterBank[POLYCACHE_NUM_CLUSTER_BANKS];

    cell* m_pMRU;
    cell* m_pLRU;
    cell* m_Hash[POLYCACHE_HASH_SIZE];

    BBox  m_GatherBBox;
    cell* m_pGatherCell;
    bool  m_bInsideGather;

    cluster* m_GatherClusterList[128];
    int      m_nGatherClusters;
    int      m_nGatherClustersAlreadyCached;
    uint16_t m_Sequence;

    int m_n8Clusters;
    int m_n16Clusters;
    int m_n32Clusters;
    int m_nSumClusters;
    int m_nClusterHashSize;

    ObjectManager* objectManager;
};

//==============================================================================

inline int GetBoneIndexFromPolyCachePrimKey(uint32_t PrimKey)
{
    return PrimKey & 0xFF;
}

//==============================================================================

inline poly_cache::cluster* poly_cache::GetNextClusterFromRayWalk()
{
    if (m_iNextCluster < m_nClusters) {
        poly_cache::cluster* pCluster = m_ClusterList[m_iNextCluster];

        // Increment to next cluster
        m_iNextCluster++;

        // Return cluster address
        return pCluster;
    } else {
        // Be sure there are clusters in the cache
        while (m_iNextCluster == m_nClusters) {
            // Step ray through grid and collect clusters.  If we hit the end of the ray
            // then return NULL
            if (StepRayClusterWalker() == false) {
                return NULL;
            }
        }

        m_iNextCluster = 1;
        return m_ClusterList[0];
    }
}

//==============================================================================

inline void poly_cache::IncrementSequence()
{
    if (m_Sequence < POLYCACHE_MAX_SEQUENCE) {
        m_Sequence++;
    } else {
        ClearSequence();
    }
}

//==============================================================================

inline int poly_cache::ComputeHash(int X, int Y, int Z)
{
    int H = ((uint32_t)((((X << 10) + Y) << 10) + Z)) % POLYCACHE_HASH_SIZE;
    return H;
}

//==============================================================================

inline poly_cache::cell* poly_cache::LookupCell(int X, int Y, int Z)
{
    int H = ComputeHash(X, Y, Z);

    cell* pC = m_Hash[H];
    while (pC) {
        if ((pC->X == X) && (pC->Y == Y) && (pC->Z == Z)) {
            break;
        }

        pC = pC->pHashNext;
    }

    return pC;
}

extern poly_cache g_PolyCache;
