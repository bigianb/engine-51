#pragma once

#define CheckCollisions fn_CheckCollisions

inline bool collision_mgr::IsInIgnoreList(guid Guid)
{
    for (int i = 0; i < m_nIgnoredObjects; i++) {
        if (Guid == m_IgnoreList[i]) {
            return true;
        }
    }
    return false;
}

inline guid collision_mgr::GetMovingObjGuid() const
{
    return m_MovingObjGuid;
}

inline void collision_mgr::SetMaxCollisions(int nMaxCollisions)
{
    assert(nMaxCollisions > 0);

    if (m_nMaxCollisions > MAX_COLLISION_MGR_COLLISIONS) {
        nMaxCollisions = MAX_COLLISION_MGR_COLLISIONS;
    }

    m_nMaxCollisions = nMaxCollisions;
}

inline const BBox& collision_mgr::GetDynamicBBox() const
{
    return m_DynamicBBoxes[m_ContextInfo.Context];
}

inline const collision_mgr::dynamic_cylinder& collision_mgr::GetDynamicCylinder() const
{
    return m_CylinderInfo[m_ContextInfo.Context];
}

inline void collision_mgr::AddToIgnoreList(guid Guid)
{
    assert(m_nIgnoredObjects < MAX_IGNORED_OBJECTS);
    m_IgnoreList[m_nIgnoredObjects] = Guid;
    m_nIgnoredObjects++;
}

inline void collision_mgr::ClearIgnoreList(void)
{
    m_nIgnoredObjects = 0;
}

inline const collision_mgr::dynamic_ray& collision_mgr::GetDynamicRay() const
{
    return m_RayInfo[0];
}

inline void collision_mgr::CollectPermeables()
{
    m_bCollectPermeable = true;
}

inline int collision_mgr::GetNPermeables()
{
    return m_nPermeables;
}

inline guid collision_mgr::GetPermeableGuid(int Index)
{
    assert((Index >= 0) && (Index < m_nPermeables));
    return m_Permeable[Index];
}

inline int CompareCollisions(const void* C1, const void* C2)
{
    assert(C1 != NULL);
    assert(C2 != NULL);

    float T1 = ((collision_mgr::collision*)(C1))->T;
    float T2 = ((collision_mgr::collision*)(C2))->T;

    return (T1 > T2) ? 1 : ((T1 < T2) ? -1 : 0);
}

inline void collision_mgr::SortCollisions()
{
    if (m_nCollisions > 1) {
        assert(false); // TODO: fix this
        //x_qsort(m_Collisions, m_nCollisions, sizeof(collision_mgr::collision), CompareCollisions);
    }
}

inline void collision_mgr::StartApply(guid Guid)
{
    //
    // Object is colliding against self
    //
    assert((Guid == 0) || (Guid != m_MovingObjGuid));

    assert(!m_bApplyStarted); // EndApply() wasn't called
    m_bApplyStarted = true;
    m_ContextInfo.Context = 0;
    m_ContextInfo.Guid = Guid;

    // transforms do nothing
    m_ContextInfo.L2W.Identity();
    m_ContextInfo.W2L.Identity();
}

inline void collision_mgr::EndApply()
{
    assert(m_bApplyStarted); // StartApply() wasn't called
    m_bApplyStarted = false;
    m_ContextInfo.Context = 0;
    m_ContextInfo.Guid = 0;
}

inline void collision_mgr::UseLowPoly()
{
    m_bUseLowPoly = true;
}

inline void collision_mgr::StopOnFirstCollisionFound()
{
    m_bStopOnFirstCollisionFound = true;
}

inline void collision_mgr::IgnoreGlass()
{
    m_bIgnoreGlass = true;
}

inline void collision_mgr::DoNotRemoveDuplicateGuids()
{
    m_bRemoveDuplicateGuids = false;
}

inline void collision_mgr::UsePolyCache()
{
    m_bUsePolyCache = true;
}
