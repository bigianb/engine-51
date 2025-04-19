#pragma once

#include "../VectorMath.h"

// A simple scratch memory cache class
class smem_matrix_cache
{
public:
    smem_matrix_cache();

    // Sets dirty status
    void SetDirty(bool bStatus);

    // Returns dirty status
    bool IsDirty() const;

    // Returns TRUE if matrix data allocated and not dirty
    bool IsValid(int nMatrices) const;

    // Returns current matrix allocation (doesn't care if it's dirty of not)
    Matrix4* GetMatrices(int nMatrices = -1);

private:
    // Returns true if matrix allocation is valid
    bool IsAllocValid(int nMatrices = -1) const;

    // Private data
private:
    bool     m_bDirty;    // TRUE if matrices are valid
    int      m_ID;        // Allocation ID
    int      m_nMatrices; // Current number of matrices allocated
    Matrix4* m_pMatrices; // Allocated matrices
};
