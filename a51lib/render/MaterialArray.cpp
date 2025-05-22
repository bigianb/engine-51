
#include "Render.h"

#define RENDER_PRIVATE
#include "MaterialArray.h"
#undef RENDER_PRIVATE

#include <cassert>

//=============================================================================
// Implementation
//=============================================================================

material_array::material_array(ResourceManager* rm)
    : resourceManager(rm)
    , m_Sorted(false)
    , m_Capacity(false)
    , m_nNodes(0)
    , m_pNodes(nullptr)
    , m_pIndices(nullptr)
{
}

//=============================================================================

material_array::~material_array()
{
    Clear();
    if (m_pNodes) {
        free(m_pNodes);
    }
}

//=============================================================================

static int GetMaterialPriority(material_type Type)
{
    static const int8_t kMatPriorities[Material_NumTypes] =
        {
            1, // Material_Not_Used
            2, // Material_Diff
            5, // Material_Alpha
            2, // Material_Diff_PerPixelEnv
            3, // Material_Diff_PerPixelIllum
            5, // Material_Alpha_PerPolyEnv
            4, // Material_Alpha_PerPixelIllum
            4, // Material_Alpha_PerPolyIllum
            6, // Material_Distortion
            6, // Material_Distortion_PerPolyEnv
        };

    return kMatPriorities[Type];
}

//=============================================================================

int NodeCompareFn(const void* pA, const void* pB)
{
    material_array::node_info* pNodeA = (material_array::node_info*)pA;
    material_array::node_info* pNodeB = (material_array::node_info*)pB;
    material*                  pMatA = &pNodeA->Item;
    material*                  pMatB = &pNodeB->Item;
    material_type              TypeA = (material_type)pMatA->m_Type;
    material_type              TypeB = (material_type)pMatB->m_Type;

    if (GetMaterialPriority(TypeA) > GetMaterialPriority(TypeB)) {
        return 1;
    }
    if (GetMaterialPriority(TypeA) < GetMaterialPriority(TypeB)) {
        return -1;
    }
    if (pMatA->m_DiffuseMap.getPointer() > pMatB->m_DiffuseMap.getPointer()) {
        return 1;
    }
    if (pMatA->m_DiffuseMap.getPointer() < pMatB->m_DiffuseMap.getPointer()) {
        return -1;
    }
    if (pMatA->m_EnvironmentMap.getPointer() > pMatB->m_EnvironmentMap.getPointer()) {
        return 1;
    }
    if (pMatA->m_EnvironmentMap.getPointer() < pMatB->m_EnvironmentMap.getPointer()) {
        return -1;
    }
    if (pMatA->m_DetailMap.getPointer() > pMatB->m_DetailMap.getPointer()) {
        return 1;
    }
    if (pMatA->m_DetailMap.getPointer() < pMatB->m_DetailMap.getPointer()) {
        return -1;
    }

    return 0;
}

//=============================================================================

void material_array::Sort()
{
    if (m_Sorted) {
        return;
    }

    if (m_nNodes) {
        // sort the materials
        qsort(m_pNodes, m_nNodes, sizeof(node_info), NodeCompareFn);

        // remap the material indices
        for (int i = 0; i < m_nNodes; i++) {
            assert((m_pNodes[i].Handle.Handle >= 0) &&
                   (m_pNodes[i].Handle.Handle < m_Capacity));
            m_pIndices[m_pNodes[i].Handle] = i;
        }
    }

    // sanity check
    if (0) {
        for (int iNode = 0; iNode < m_nNodes - 1; iNode++) {
            assert(NodeCompareFn(&m_pNodes[iNode + 1], &m_pNodes[iNode]) >= 0);
        }
    }

    m_Sorted = true;
}

//=============================================================================

void material_array::Clear()
{
    int i;

    // destruct all of the objects
    for (i = 0; i < m_nNodes; i++) {
        ((destructor*)&m_pNodes[i].Item)->~destructor();
    }

    // clear all of the handle indices
    for (i = 0; i < m_Capacity; i++) {
        m_pIndices[i] = -1;
    }

    m_nNodes = 0;
    m_Sorted = false;
}

//=============================================================================

void material_array::GrowListBy(int nNodes)
{
    assert(nNodes > 0);

    // increase the capacity
    int NewCapacity = m_Capacity + nNodes;

    // allocate the new arrays
    node_info* pNewNodes = (node_info*)malloc((sizeof(node_info) +
                                               sizeof(int)) *
                                              NewCapacity);
    assert(pNewNodes);

    int* pNewIndices = (int*)(pNewNodes + NewCapacity);
    assert(pNewIndices);

    // copy all the previous nodes to the new arrays
    if (m_nNodes) {
        assert(m_pNodes);
        memcpy(pNewNodes, m_pNodes, sizeof(node_info) * m_nNodes);
    }
    if (m_Capacity) {
        assert(m_pIndices);
        memcpy(pNewIndices, m_pIndices, sizeof(int) * m_Capacity);
    }

    // set the new indices to -1
    for (int i = m_Capacity; i < NewCapacity; i++) {
        pNewIndices[i] = -1;
    }

    // update the arrays
    if (m_pNodes) {
        free(m_pNodes);
    }
    m_pNodes = pNewNodes;
    m_pIndices = pNewIndices;
    m_Capacity = NewCapacity;
}

//=============================================================================

material& material_array::Add(xhandle& hHandle)
{
    // do we need to grow?
    if (m_nNodes >= m_Capacity) {
        GrowListBy(std::max(m_Capacity / 2, 100));
    }

    // find the first available index
    int iHandle;
    for (iHandle = 0; iHandle < m_Capacity; iHandle++) {
        if (m_pIndices[iHandle] == -1) {
            break;
        }
    }
    assert(iHandle != m_Capacity);

    // fill in the index information
    m_pIndices[iHandle] = m_nNodes;
    hHandle.Handle = iHandle;

    // construct the node info
    new(&m_pNodes[m_nNodes].Item) material(resourceManager);
    m_pNodes[m_nNodes].Handle.Handle = iHandle;
    m_nNodes++;

    // we'll need to re-sort
    m_Sorted = false;

    return m_pNodes[m_pIndices[iHandle]].Item;
}

//=============================================================================

void material_array::DeleteByHandle(xhandle hHandle)
{
    int Index = GetIndexByHandle(hHandle);
    assert((Index >= 0) && (Index < m_nNodes));

    // call destructor
    ((destructor*)&m_pNodes[Index].Item)->~destructor();

    // clear out the index
    m_pIndices[hHandle.Handle] = -1;

    // copy the last node into the deleted node
    if (Index != (m_nNodes - 1)) {
        m_pNodes[Index].Item = m_pNodes[m_nNodes - 1].Item;
        m_pNodes[Index].Handle = m_pNodes[m_nNodes - 1].Handle;
        m_pIndices[m_pNodes[Index].Handle.Handle] = Index;
    }
    m_nNodes--;

    // flag that we'll need to sort again
    m_Sorted = false;
}

//=============================================================================

bool material_array::SanityCheck(void) const
{
    int i;

    for (i = 0; i < m_nNodes; i++) {
        assert(m_pIndices[m_pNodes[i].Handle.Handle] == i);
    }

    for (i = 0; i < m_Capacity; i++) {
        if (m_pIndices[i] != -1) {
            assert(m_pNodes[m_pIndices[i]].Handle.Handle == i);
        }
    }

    return true;
}

void material_array::Update(float DeltaTime)
{
    for (int i = 0; i < GetCount(); i++) {
        material&         Mat = operator[](i);
        material::uvanim& UVAnim = Mat.m_UVAnim;
        if (UVAnim.nFrames <= 1) {
            continue;
        }

        // Artists duplicate the first and last frame to be consistent
        // with the sketal animations (this is to make blending and
        // looped anims work). We really don't want to do this for
        // uv anims, so consider nFrames to be one smaller.
        int nFrames = UVAnim.nFrames - 1;

        // calculate the next frame
        UVAnim.CurrentFrame += DeltaTime * UVAnim.FPS * UVAnim.Dir;

        // determine the type of animation
        switch (UVAnim.Type) {
        case Geom::Material::UVanim::FIXED:
            UVAnim.Dir = 0;
            UVAnim.CurrentFrame = (float)UVAnim.StartFrame;
            break;
        case Geom::Material::UVanim::LOOPED:
            UVAnim.Dir = 1;
            if (UVAnim.CurrentFrame >= (float)nFrames) {
                UVAnim.CurrentFrame = (float)UVAnim.StartFrame;
            }
            break;
        case Geom::Material::UVanim::PINGPONG:
            if (UVAnim.Dir > 0) {
                UVAnim.Dir = 1;
                if (UVAnim.CurrentFrame >= (float)nFrames) {
                    UVAnim.CurrentFrame = (float)(nFrames - 1);
                    UVAnim.Dir = -UVAnim.Dir;
                }
            } else {
                UVAnim.Dir = -1;
                if (UVAnim.CurrentFrame <= 0.0f) {
                    UVAnim.CurrentFrame = 0.0f;
                    UVAnim.Dir = -UVAnim.Dir;
                }
            }
            break;
        case Geom::Material::UVanim::ONESHOT:
            UVAnim.Dir = 1;
            if (UVAnim.CurrentFrame >= (float)nFrames) {
                UVAnim.CurrentFrame = (float)(nFrames - 1);
                UVAnim.Dir = 0;
            }
            break;
        }

        UVAnim.iFrame = (int8_t)floor(UVAnim.CurrentFrame);
    }
}
