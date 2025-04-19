

#include "SMemMatrixCache.h"
#include "../xfiles/scratchMem.h"
#include <cassert>

smem_matrix_cache::smem_matrix_cache()
{
    // Clear cache
    m_bDirty = false;      // TRUE if data is valid
    m_ID = -1;             // Allocation ID
    m_nMatrices = 0;       // Current number of matrices allocated
    m_pMatrices = nullptr; // Allocated matrices
}

//=========================================================================

// Sets dirty status
void smem_matrix_cache::SetDirty(bool bStatus)
{
    // Keep status
    m_bDirty = bStatus;

    // If setting status to valid, then the allocation must be valid!
    if (m_bDirty == false) {
        assert(IsAllocValid()); // "Smem toggle occured during a main thread function - another thread (frontend) must still be running!") ;
    }
}

//=========================================================================

// Returns dirty status
bool smem_matrix_cache::IsDirty() const
{
    // Allocation invalid?
    if (IsAllocValid() == false) {
        return true;
    }

    // Is data dirty?
    return (m_bDirty);
}

//=========================================================================

// Returns true if matrix data allocated and not dirty
bool smem_matrix_cache::IsValid(int nMatrices) const
{
    // Is data dirty?
    if (m_bDirty == true) {
        return false;
    }

    // Is allocation bad?
    if (IsAllocValid(nMatrices) == false) {
        return false;
    }

    // All is good
    return true;
}

//=========================================================================

// Returns current matrix allocation (doesn't care if it's dirty of not)
Matrix4* smem_matrix_cache::GetMatrices(int nMatrices)
{
    // Is current allocation okay?
    if (IsAllocValid(nMatrices)) {
        return m_pMatrices;
    }

    // Allocate new cache and flag data is dirty
    m_ID = smem_GetActiveID();
    assert(nMatrices > 0);
    m_nMatrices = nMatrices;
    m_pMatrices = (Matrix4*)smem_BufferAlloc(nMatrices * sizeof(Matrix4));
    assert(m_pMatrices);
    m_bDirty = true;

    return m_pMatrices;
}
// Returns true if matrix allocation is valid
bool smem_matrix_cache::IsAllocValid(int nMatrices) const
{
    // Allocation must be present
    if (m_pMatrices == nullptr) {
        return false;
    }

    // Allocation must be big enough
    if (m_nMatrices < nMatrices) {
        return false;
    }

    // Allocation must be on the correct smem frame
    if (m_ID != smem_GetActiveID()) {
        return false;
    }

    // All is good
    return true;
}
