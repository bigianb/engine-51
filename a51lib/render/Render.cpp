
#include "Render.h"
#include "../xfiles/xHandleArray.h"
#include "../view/View.h"
#include "../xfiles/scratchMem.h"

#define RENDER_PRIVATE
#include "MaterialArray.h"
#undef RENDER_PRIVATE

#include "../system/Renderer.h"

#include <cassert>

struct distortion_info
{
    Radian3  NormalRot; // rotation for perturbing the normals
    uint32_t MatIndex;  // index of the original material, or -1 if we are
                        // completely overriding the material settings
};

//=============================================================================

#define ENABLE_RENDER_XTIMERS 0

enum
{
    ORDER_OPAQUE = 0,
    ORDER_GLOWING = 1,
    ORDER_ALPHA_GLOWING = 2,
    ORDER_ALPHA = 3,
    // everything aver this point will be in the custom render
    ORDER_CUSTOM_START = 4,
    ORDER_FORCED_LAST = 4,
    ORDER_ZPRIME = 5,
    ORDER_FADING_ALPHA = 6,
    ORDER_DISTORTION = 7
};

//=============================================================================

struct sort_struct
{
    uint32_t SortKey;
    int      iRenderInst;
};

//=============================================================================

struct private_instance
{
    Geom*     pGeom;
    geom_type Type;

#ifdef TARGET_PC
    xarray<xhandle> RigidDList;
    bool            IsLit;
#endif // TARGET_PC
};

//=============================================================================

struct private_geom
{
    // simple struct for registered geoms...no information is really needed,
    // but we'll put a pointer back in for sanity checking later on
    Geom* pGeom;

#ifdef TARGET_PC
    xarray<xhandle> SkinDList;
#endif // TARGET_PC
};

//=============================================================================

struct texture_projection
{
    texture_projection() : Texture(nullptr){}

    Matrix4         L2W;
    Radian          FOV;
    float           Length;
    texture::handle Texture;
};

color_info::usage color_info::m_Usage = color_info::kUse16;

//=============================================================================

// constants
// VERY IMPORTANT NOTE: README README README README!!!!!
//    Some of these max numbers will get used by the sort keys, so if they need
//    to increase, make sure the sort key still has enough bits to deal with it.

static const int kHashTableSize = 769; // 1543;   // keep this a prime number for best hashing results.
static const int kMaxRegisteredGeoms = 512;
static const int kMaxRegisteredInstances = 10000;
static const int kMaxRegisteredMaterials = 640; // NOTE: Don't go over what the sort key can handle!
static const int kMaxTexAnims = 2048;
static const int kMaxTexAnimInstances = 1024;
static const int kMaxRegisteredTexAnims = 1024;
static const int kMaxDistortedInstances = 16;
static const int kMaxRenderedInstances = 32768;

// arrays for rendering everything
static int                          s_LoHashMark; // below this needs sorting
static int                          s_HiHashMark; // above this is a duplicate key (no need to sort)
static int16_t                      s_HashTable[kMaxRenderedInstances];
static std::vector<sort_struct>     s_lSortData;
static xharray<private_geom>*        s_lRegisteredGeoms = nullptr;
static xharray<private_instance>*    s_lRegisteredInst = nullptr;
static material_array*              s_lRegisteredMaterials = nullptr;
static std::vector<render_instance> s_lRenderInst;
static std::vector<distortion_info> s_lDistortionInfo;

// sanity check data
static bool s_InRawBegin = false;
static bool s_InRenderBegin = false;
static bool s_InShadowBegin = false;
static bool s_InCustomBegin = false;

// misc. data
static cubemap* s_pCurrCubeMap = NULL;
static float    s_PulseTime;
static int      s_CustomStart;

// Filter light data
static int    s_bFilterLight = false;
static Colour s_FilterLightColor(Colour(30, 0, 0, 255));

// texture and shadow projection
static texture_projection s_TextureProjection;
static texture_projection s_ShadowProjections[render::MAX_SHADOW_PROJECTORS];
static bool               s_bDoTextureProjection;
static int                s_nShadowProjections;
static view               s_TextureProjectionView;
static view               s_ShadowProjectionViews[render::MAX_SHADOW_PROJECTORS];
static Matrix4            s_TextureProjectionMatrix;
static Matrix4            s_ShadowProjectionMatrices[render::MAX_SHADOW_PROJECTORS];

// projected shadows that are generated dynamically
static int s_nDynamicShadows;

//=============================================================================

inline bool IsAlphaMaterial(material_type Type)
{
    switch (Type) {
    default:
    case Material_Diff:
    case Material_Diff_PerPixelEnv:
    case Material_Diff_PerPixelIllum:
        return false;

    case Material_Alpha:
    case Material_Alpha_PerPolyEnv:
    case Material_Alpha_PerPixelIllum:
    case Material_Alpha_PerPolyIllum:
    case Material_Distortion:
    case Material_Distortion_PerPolyEnv:
        return true;
    }
}

static uint32_t           HashFn(uint32_t SortKey);
static render_instance&   AddToHashHybrid(uint32_t SortKey);
static xhandle            FindMaterial(material& Material);
static void               RegisterMaterials(Geom&, ResourceManager*);
static void               UnregisterMaterials(Geom&);
static void               RegisterGeom(Geom&, ResourceManager*);
static uint32_t           GetRenderOrder(material_type Type);
static void               ComputeBaseSortKeys(Geom&, geom_type Type);
static void               UnregisterGeom(Geom&);
static void               RegisterRigidGeom(RigidGeom&, ResourceManager* rm);
static void               UnregisterRigidGeom(RigidGeom&);
static void               RegisterSkinGeom(RigidGeom&, ResourceManager* rm);
static void               UnregisterSkinGeom(RigidGeom&);
static render::hgeom_inst AddPrivateInstance(Geom&, geom_type Type);
static void               RemovePrivateInstance(render::hgeom_inst hInst);
static int                InstanceCompareFn(const void* p1, const void* p2);
static void               GetUVOffset(uint8_t&  UOffset,
                                      uint8_t&  VOffset,
                                      Geom*     pGeom,
                                      material& Mat);
static bool               IntersectsView(const view& V, const BBox&);
static bool               IntersectsProjTexture(const BBox&);
static bool               IntersectsShadowTexture(int Index, const BBox&);
static uint32_t           CollectProjectionInfo(uint32_t RenderFlags, const BBox&);
static bool               CanHaveProjTexture(const Geom::Material& Mat);

static void CalcVMatOffsets(int*        VMatOffsets,
                            const Geom* pGeom,
                            uint32_t    VTextureMask);

static uint32_t HashFn(uint32_t SortKey)
{
    return SortKey % kHashTableSize;
}

//=============================================================================

static render_instance& AddToHashHybrid(uint32_t SortKey)
{
    assert(s_LoHashMark < s_HiHashMark);

    uint32_t HashIndex = HashFn(SortKey);
    if (s_HashTable[HashIndex] == -1) {
        // if there is no hash entry, just add it
        render_instance& AddInst = s_lRenderInst[s_LoHashMark];
        s_HashTable[HashIndex] = s_LoHashMark;
        AddInst.SortKey.Bits = SortKey;
        AddInst.Next = -1;
        AddInst.Brother = -1;
        AddInst.pLighting = NULL;
        sort_struct& SortInst = s_lSortData[s_LoHashMark];
        SortInst.iRenderInst = s_LoHashMark;
        SortInst.SortKey = SortKey;
        s_LoHashMark++;
        return AddInst;
    } else {
        // loop through the linked list of hash collisions
        int CurrLink = s_HashTable[HashIndex];
        while (1) {
            render_instance& TestInst = s_lRenderInst[CurrLink];
            if (TestInst.SortKey.Bits == SortKey) {
                // this sort key isn't unique, add it to the end of the array
                // so that it won't be considered for sorting, and link it as a
                // "brother" of the unique instance
                render_instance& AddInst = s_lRenderInst[s_HiHashMark];
                AddInst.SortKey.Bits = SortKey;
                AddInst.Next = -1;
                AddInst.Brother = TestInst.Brother;
                AddInst.pLighting = NULL;
                TestInst.Brother = s_HiHashMark;
                s_HiHashMark--;
                return AddInst;
            }

            if (TestInst.Next == -1) {
                // the key wasn't found, so add it to the end of our linked list of
                // hash collisions
                render_instance& AddInst = s_lRenderInst[s_LoHashMark];
                AddInst.SortKey.Bits = SortKey;
                AddInst.Next = -1;
                AddInst.Brother = -1;
                AddInst.pLighting = NULL;
                TestInst.Next = s_LoHashMark;
                sort_struct& SortInst = s_lSortData[s_LoHashMark];
                SortInst.iRenderInst = s_LoHashMark;
                SortInst.SortKey = SortKey;
                s_LoHashMark++;
                return AddInst;
            }

            CurrLink = TestInst.Next;
        }
    }
}

//=============================================================================

static xhandle FindMaterial(material& mat)
{
    for (int i = 0; i < s_lRegisteredMaterials->GetCount(); i++) {
        material& M = (*s_lRegisteredMaterials)[i];

        if (M == mat) {
            return s_lRegisteredMaterials->GetHandleByIndex(i);
        }
    }

    return HNULL;
}

//=============================================================================

static void RegisterMaterials(Geom& geom, ResourceManager* rm)
{
    // Register all the materials in the geom
    for (int iMat = 0; iMat < geom.numMaterials; iMat++) {
        material Mat(rm);

        // get the next material used by the geom
        Geom::Material& GeomMat = geom.materials[iMat];

        // set the material type
        Mat.m_Type = GeomMat.type;

        // set the flags
        Mat.m_Flags = GeomMat.flags;

        // set the detail scale
        Mat.m_DetailScale = GeomMat.detailScale;

        // set the fixed alpha
        Mat.m_FixedAlpha = GeomMat.fixedAlpha;

        // copy across the uvanim data
        Mat.m_UVAnim.CurrentFrame = 0.0f;
        Mat.m_UVAnim.iKey = GeomMat.uvAnim.iKey;
        Mat.m_UVAnim.iFrame = 0;
        Mat.m_UVAnim.Dir = 1;
        Mat.m_UVAnim.Type = GeomMat.uvAnim.type;
        Mat.m_UVAnim.nFrames = GeomMat.uvAnim.nKeys;
        Mat.m_UVAnim.FPS = GeomMat.uvAnim.fps;
        Mat.m_UVAnim.StartFrame = GeomMat.uvAnim.startFrame;

        // sanity checks
        if (((Mat.m_Type == Material_Diff_PerPixelEnv) ||
             (Mat.m_Type == Material_Alpha_PerPolyEnv)) &&
            !(Mat.m_Flags & Geom::Material::FLAG_ENV_CUBE_MAP)) {
            if (!(GeomMat.flags & Geom::Material::FLAG_HAS_ENV_MAP)) {
                throw("Environment mapped material without an env texture!");
            }
        }

        // copy the texture info over
        int iDiffuse = GeomMat.iTexture;
        int iEnvironment = iDiffuse + GeomMat.nVirtualMats;
        int iDetail = iEnvironment +
                      ((GeomMat.flags & Geom::Material::FLAG_HAS_ENV_MAP) ? 1 : 0);

        // now for virtual textures, each different bitmap choice will become a
        // unique material
        for (int iVMat = 0; iVMat < GeomMat.nVirtualMats; iVMat++) {
            // set the diffuse map for this bitmap choice
            Mat.m_DiffuseMap.setName(geom.getTextureFilename(iDiffuse + iVMat));

            // set the env map for this bitmap choice
            if (GeomMat.flags & Geom::Material::FLAG_HAS_ENV_MAP) {
                Mat.m_EnvironmentMap.setName(geom.getTextureFilename(iEnvironment));
            } else {
                Mat.m_EnvironmentMap.setName("");
            }

            // set the detail map for this bitmap choice
            if (GeomMat.flags & Geom::Material::FLAG_HAS_DETAIL_MAP) {
                Mat.m_DetailMap.setName(geom.getTextureFilename(iDetail));
            } else {
                Mat.m_DetailMap.setName("");
            }

            // check if we already have this material registered
            xhandle Handle = FindMaterial(Mat);

            // if this is a new material, then add it
            if (Handle == HNULL) {
                // If you hit this assert, this means we are causing a realloc
                // which will fragment memory, and we are possibly going over
                // the max number of materials that will fit within the sort
                // key. This could cause material corruptions.
                assert(s_lRegisteredMaterials->GetCount() < kMaxRegisteredMaterials);
                material& NewMat = s_lRegisteredMaterials->Add(Handle);
                NewMat = Mat;
            }

            // finally, we can add a ref to this material, and let the geometry know its
            // material handle
            material& FinalMat = (*s_lRegisteredMaterials)(Handle);
            FinalMat.AddRef();

            // let the geometry know where its registered material can be found
            geom.virtualMaterials[GeomMat.iVirtualMat + iVMat].MatHandle = Handle;

            // and let the platform do any initialization that it needs to
            //platform_RegisterMaterial(FinalMat);
        }
    }
}

//=============================================================================

static void UnregisterMaterials(Geom& geom)
{
    // unregister any of the materials
    for (int iMat = 0; iMat < geom.numMaterials; iMat++) {
        Geom::Material& GeomMat = geom.materials[iMat];

        for (int iVMat = 0; iVMat < GeomMat.nVirtualMats; iVMat++) {
            xhandle   Handle = geom.virtualMaterials[GeomMat.iVirtualMat + iVMat].MatHandle;
            material& Mat = (*s_lRegisteredMaterials)(Handle);
            Mat.Release();
            if (Mat.GetRefCount() == 0) {
                s_lRegisteredMaterials->DeleteByHandle(Handle);
            }
        }
    }
}

//=============================================================================

static void RegisterGeom(Geom& geom, ResourceManager* rm)
{
    // register the geometry
    assert(geom.m_hGeom == HNULL);
    assert(geom.GetRefCount() == 0);
    assert(s_lRegisteredGeoms->GetCount() < kMaxRegisteredGeoms);
    private_geom& RegGeom = s_lRegisteredGeoms->Add(geom.m_hGeom);

    // this pointer isn't really needed, but will be nice for sanity checking
    // later on
    RegGeom.pGeom = &geom;

    // register the materials this geometry uses
    RegisterMaterials(geom, rm);
}

//=============================================================================

static uint32_t GetRenderOrder(material_type Type)
{
    switch (Type) {
    default:
        assert(false /* "Unknown material type." */);
    case Material_Diff:
    case Material_Diff_PerPixelEnv:
        return ORDER_OPAQUE;
    case Material_Diff_PerPixelIllum:
        return ORDER_GLOWING;
    case Material_Alpha_PerPixelIllum:
    case Material_Alpha_PerPolyIllum:
        return ORDER_ALPHA_GLOWING;
    case Material_Alpha:
    case Material_Alpha_PerPolyEnv:
        return ORDER_ALPHA;
    case Material_Distortion:
    case Material_Distortion_PerPolyEnv:
        return ORDER_DISTORTION;
    }
}

//=============================================================================

static void ComputeBaseSortKeys(Geom& geom, geom_type Type)
{
    int iSubMesh;
    assert(geom.m_hGeom.IsNonNull());
    for (iSubMesh = 0; iSubMesh < geom.numSubMeshes; iSubMesh++) {
        Geom::Submesh*  pSubMesh = &geom.subMeshes[iSubMesh];
        Geom::Material* pMaterial = &geom.materials[pSubMesh->iMaterial];
        int             TypeBit = (Type == TYPE_RIGID) ? 0 : 1;

        // range safety check for the sort key
        assert((geom.m_hGeom >= 0) && (geom.m_hGeom < kMaxRegisteredGeoms));
        assert((iSubMesh >= 0) && (iSubMesh < 256));

        // build the sort key
        sortkey BaseSortKey;
        BaseSortKey.Bits = 0;
        BaseSortKey.GeomSubMesh = iSubMesh;
        BaseSortKey.GeomHandle = geom.m_hGeom;
        BaseSortKey.GeomType = TypeBit;
        BaseSortKey.RenderOrder = GetRenderOrder((material_type)pMaterial->type);
        pSubMesh->baseSortKey = BaseSortKey.Bits;
    }
}

//=============================================================================

static void UnregisterGeom(Geom& geom)
{
    assert(geom.GetRefCount() == 0);

    // unregister the materials
    UnregisterMaterials(geom);

    // unregister the geom
    assert((*s_lRegisteredGeoms)(geom.m_hGeom).pGeom == &geom);
    s_lRegisteredGeoms->DeleteByHandle(geom.m_hGeom);
    geom.m_hGeom = HNULL;
}

//=============================================================================

static void RegisterRigidGeom(RigidGeom& geom, ResourceManager* rm)
{
    RegisterGeom(geom, rm);
    ComputeBaseSortKeys(geom, TYPE_RIGID);
    //platform_RegisterRigidGeom(geom);
}

//=============================================================================

static void UnregisterRigidGeom(RigidGeom& geom)
{
    //platform_UnregisterRigidGeom(geom);
    UnregisterGeom(geom);
}

//=============================================================================

static void RegisterSkinGeom(SkinGeom& geom, ResourceManager* rm)
{
    RegisterGeom(geom, rm);
    ComputeBaseSortKeys(geom, TYPE_SKIN);
    //platform_RegisterSkinGeom(geom);
}

//=============================================================================

static void UnregisterSkinGeom(SkinGeom& geom)
{
    //platform_UnregisterSkinGeom(geom);
    UnregisterGeom(geom);
}

//=============================================================================

static render::hgeom_inst AddPrivateInstance(Geom& geom, geom_type Type)
{
    // increment the geom's ref count
    geom.AddRef();

    // add the instance
    render::hgeom_inst Handle;
    assert(s_lRegisteredInst->GetCount() < kMaxRegisteredInstances);
    private_instance& Inst = s_lRegisteredInst->Add(Handle);
    Inst.pGeom = &geom;
    Inst.Type = Type;

    // return the new handle
    return Handle;
}

//=============================================================================

static void RemovePrivateInstance(render::hgeom_inst hInst)
{
    // decrement the geom's ref count
    private_instance& Inst = (*s_lRegisteredInst)(hInst);
    Inst.pGeom->Release();

    // delete the instance
    s_lRegisteredInst->DeleteByHandle(hInst);
}

//=============================================================================

static int InstanceCompareFn(const void* p1, const void* p2)
{
    sort_struct* Inst1 = (sort_struct*)p1;
    sort_struct* Inst2 = (sort_struct*)p2;

    if (Inst1->SortKey > Inst2->SortKey) {
        return 1;
    }
    if (Inst1->SortKey < Inst2->SortKey) {
        return -1;
    }

    return 0;
}

//=============================================================================

static void GetUVOffset(uint8_t& UOffset, uint8_t& VOffset, Geom* pGeom, material& Mat)
{
    if (Mat.m_UVAnim.nFrames == 0) {
        UOffset = VOffset = 0;
        return;
    }

    int iKey = Mat.m_UVAnim.iKey + Mat.m_UVAnim.iFrame;
    UOffset = pGeom->uvKeys[iKey].offsetU;
    VOffset = pGeom->uvKeys[iKey].offsetV;
}

//=============================================================================

static bool IntersectsView(const view& V, const BBox& bb)
{
    return (V.BBoxInView(bb) != view::VISIBLE_NONE);
}

//=============================================================================

static bool IntersectsProjTexture(const BBox& bb)
{
    return IntersectsView(s_TextureProjectionView, bb);
}

//=============================================================================

static bool IntersectsShadowTexture(int Index, const BBox& bb)
{
    return IntersectsView(s_ShadowProjectionViews[Index], bb);
}

//=============================================================================

static uint32_t CollectProjectionInfo(uint32_t RenderFlags, const BBox& bb)
{
    uint32_t RetFlags = 0;

    // do we even have a texture projection we can use?
    if (!s_bDoTextureProjection && !s_nShadowProjections) {
        return RetFlags;
    }

    // check the spotlight
    if (s_bDoTextureProjection &&
        !(RenderFlags & render::DISABLE_SPOTLIGHT) &&
        IntersectsProjTexture(bb)) {
        RetFlags |= render::INSTFLAG_SPOTLIGHT;
    }

    // check any projected shadows
    if (s_nShadowProjections &&
        !(RenderFlags & render::DISABLE_PROJ_SHADOWS) &&
        IntersectsShadowTexture(0, bb)) {
        RetFlags |= render::INSTFLAG_PROJ_SHADOW_1;
    }

    if ((s_nShadowProjections > 1) &&
        !(RenderFlags & render::DISABLE_PROJ_SHADOWS) &&
        IntersectsShadowTexture(1, bb)) {
        RetFlags |= render::INSTFLAG_PROJ_SHADOW_2;
    }

    return RetFlags;
}

//=============================================================================

static bool CanHaveProjTexture(const Geom::Material& Mat)
{
    // Opaque materials can always accept projected textures.
    if (!IsAlphaMaterial((material_type)Mat.type)) {
        return true;
    }

    // They must have the zfill flag on to accept a projected texture.
    if (!(Mat.flags & Geom::Material::FLAG_FORCE_ZFILL)) {
        return false;
    }

    // Now it just comes down to whether or not they are subtractive.
    // Subtractive materials don't like doing flashlights at all, because
    // the flashlight area is rendered with a regular diffuse blend.
    if (Mat.flags & Geom::Material::FLAG_IS_SUBTRACTIVE) {
        return false;
    }

    // Yes, this alpha material can receive a projected texture.
    return true;
}

static void CalcVMatOffsets(int* VMatOffsets, const Geom* pGeom, uint32_t VTextureMask)
{
    memset(VMatOffsets, 0, sizeof(int) * 32);

    int i, j;
    for (i = 0; i < pGeom->numVirtualTextures; i++) {
        int Offset = VTextureMask & 0xf;
        VTextureMask >>= 4;

        Geom::VirtualTexture& VTexture = pGeom->virtualTextures[i];
        for (j = 0; j < pGeom->numMaterials; j++) {
            assert(j < 32);
            if (VTexture.materialMask & (1 << j)) {
                Offset = std::clamp(Offset, 0, pGeom->materials[j].nVirtualMats - 1);
                VMatOffsets[j] = Offset;
            }
        }
    }
}

int render::GetHardwareBufferSize(void)
{
    return 80;
}

//=============================================================================

// Bit ugly but not as bad as a global.
static Renderer* platformRenderer = nullptr;

void render::Init(ResourceManager* rm, Renderer* renderer)
{
    platformRenderer = renderer;
    
    s_PulseTime = 0.0f;

    s_lRegisteredGeoms = new xharray<private_geom>(rm);
    s_lRegisteredGeoms->Clear();
    s_lRegisteredGeoms->GrowListBy(kMaxRegisteredGeoms);

    s_lRegisteredInst = new xharray<private_instance>(rm);
    s_lRegisteredInst->Clear();
    s_lRegisteredInst->GrowListBy(kMaxRegisteredInstances);

    s_lRegisteredMaterials = new material_array(rm);
    s_lRegisteredMaterials->Clear();
    s_lRegisteredMaterials->GrowListBy(kMaxRegisteredMaterials);

    s_lRenderInst.clear();
    s_lRenderInst.resize(kMaxRenderedInstances);
    
    std::cout << s_lRenderInst.size() << std::endl;

   // s_lDistortionInfo.Clear();
   // s_lDistortionInfo.SetCapacity(kMaxDistortedInstances);
   // s_lDistortionInfo.SetLocked(true);
   // s_lSortData.Clear();
   // s_lSortData.SetCapacity(kMaxRenderedInstances);
   // s_lSortData.SetCount(kMaxRenderedInstances);

    //platform_Init();
}

//=============================================================================

void render::Kill(void)
{
    //platform_Kill();

    assert(s_lRegisteredGeoms->GetCount() == 0);
    assert(s_lRegisteredInst->GetCount() == 0);
    assert(s_lRegisteredMaterials->GetCount() == 0);
    s_lRegisteredGeoms->Clear();
    s_lRegisteredInst->Clear();
    s_lRegisteredMaterials->Clear();
    s_lRenderInst.clear();
  //  s_lSortData.Clear();
}

//=============================================================================

void render::Update(float DeltaTime)
{
    s_PulseTime += DeltaTime;

    // update all uv animations
    s_lRegisteredMaterials->Update(DeltaTime);
}

//=============================================================================

void render::StartRawDataMode()
{
    assert(!s_InRenderBegin && !s_InShadowBegin && !s_InRawBegin);
    s_InRawBegin = true;

    //platform_StartRawDataMode();
}

//=============================================================================

void render::EndRawDataMode()
{
    //platform_EndRawDataMode();

    assert(s_InRawBegin);
    s_InRawBegin = false;
}

//=============================================================================

void render::RenderRawStrips(int             nVerts,
                             const Matrix4&  L2W,
                             const Vector4*  pPos,
                             const int16_t*  pUV,
                             const uint32_t* pColor)
{
    assert(s_InRawBegin);
    //platform_RenderRawStrips(nVerts, L2W, pPos, pUV, pColor);
}

//=============================================================================

void render::Render3dSprites(int             nSprites,
                             float           UniScale,
                             const Matrix4*  pL2W,
                             const Vector4*  pPositions,
                             const Vector2*  pRotScales,
                             const uint32_t* pColors)
{
    assert(s_InRawBegin);

    //platform_Render3dSprites(nSprites, UniScale, pL2W, pPositions, pRotScales, pColors);
}

//=============================================================================

void render::RenderHeatHazeSprites(int             nSprites,
                                   float           UniScale,
                                   const Matrix4*  pL2W,
                                   const Vector4*  pPositions,
                                   const Vector2*  pRotScales,
                                   const uint32_t* pColors)
{
    assert(s_InRawBegin);

    //platform_RenderHeatHazeSprites(nSprites, UniScale, pL2W, pPositions, pRotScales, pColors);
}

//=============================================================================

void render::RenderVelocitySprites(int             nSprites,
                                   float           UniScale,
                                   const Matrix4*  pL2W,
                                   const Matrix4*  pVelMatrix,
                                   const Vector4*  pPositions,
                                   const Vector4*  pVelocities,
                                   const uint32_t* pColors)
{
    assert(s_InRawBegin);

    //platform_RenderVelocitySprites(nSprites, UniScale, pL2W, pVelMatrix, pPositions, pVelocities, pColors);
}

//=============================================================================

void render::SetDiffuseMaterial(const Bitmap& bitmap, int BlendMode, bool ZTestEnabled)
{
    assert(s_InRawBegin);
    //platform_SetDiffuseMaterial(bitmap, BlendMode, ZTestEnabled);
}

//=============================================================================

void render::SetGlowMaterial(const Bitmap& bitmap, int BlendMode, bool ZTestEnabled)
{
    assert(s_InRawBegin);
    //platform_SetGlowMaterial(bitmap, BlendMode, ZTestEnabled);
}

//=============================================================================

void render::SetEnvMapMaterial(const Bitmap& bitmap, int BlendMode, bool ZTestEnabled)
{
    assert(s_InRawBegin);
    //platform_SetEnvMapMaterial(bitmap, BlendMode, ZTestEnabled);
}

//=============================================================================

void render::SetDistortionMaterial(int BlendMode, bool ZTestEnabled)
{
    assert(s_InRawBegin);
    //platform_SetDistortionMaterial(BlendMode, ZTestEnabled);
}

//=============================================================================

render::hgeom_inst render::RegisterRigidInstance(RigidGeom& geom, ResourceManager* rm)
{
    assert(!s_InRenderBegin && !s_InShadowBegin && !s_InRawBegin);

    // register the geom if that hasn't been done yet
    if (geom.GetRefCount() == 0) {
        RegisterRigidGeom(geom, rm);
    }

    // safety check
    assert((*s_lRegisteredGeoms)(geom.m_hGeom).pGeom == &geom);

    // add the instance
    render::hgeom_inst Handle = AddPrivateInstance(geom, TYPE_RIGID);
    //platform_RegisterRigidInstance(geom, Handle);

    return Handle;
}

//=============================================================================

void render::UnregisterRigidInstance(hgeom_inst hInst)
{
    assert(!s_InRenderBegin && !s_InShadowBegin && !s_InRawBegin);

    // do we need to unregister the geom?
    bool bUnregisterGeom = false;
    ;
    private_instance& Inst = (*s_lRegisteredInst)(hInst);
    assert(Inst.Type == TYPE_RIGID);
    if (Inst.pGeom->GetRefCount() == 1) {
        bUnregisterGeom = true;
    }

    // unregister the instance
    //platform_UnregisterRigidInstance(hInst);
    RemovePrivateInstance(hInst);

    // unregister the geom
    if (bUnregisterGeom) {
        UnregisterRigidGeom(*((RigidGeom*)Inst.pGeom));
    }
}

//=============================================================================

render::hgeom_inst render::RegisterSkinInstance(SkinGeom& geom, ResourceManager* rm)
{
    assert(!s_InRenderBegin && !s_InShadowBegin && !s_InRawBegin);

    // register the geom if that hasn't been done yet
    if (geom.GetRefCount() == 0) {
        RegisterSkinGeom(geom, rm);
    }

    // safety check
    assert((*s_lRegisteredGeoms)(geom.m_hGeom).pGeom == &geom);

    // add the instance
    render::hgeom_inst Handle = AddPrivateInstance(geom, TYPE_SKIN);
    //platform_RegisterSkinInstance(geom, Handle);

    return Handle;
}

//=============================================================================

void render::UnregisterSkinInstance(hgeom_inst hInst)
{
    assert(!s_InRenderBegin && !s_InShadowBegin && !s_InRawBegin);

    // do we need to unregister the geom?
    bool bUnregisterGeom = false;
    ;
    private_instance& Inst = (*s_lRegisteredInst)(hInst);
    assert(Inst.Type == TYPE_SKIN);
    if (Inst.pGeom->GetRefCount() == 1) {
        bUnregisterGeom = true;
    }

    // unregister the instance
    //platform_UnregisterSkinInstance(hInst);
    RemovePrivateInstance(hInst);

    // unregister the geom
    if (bUnregisterGeom) {
        UnregisterSkinGeom(*((SkinGeom*)Inst.pGeom));
    }
}

//=============================================================================

const Geom* render::GetGeom(hgeom_inst hInst)
{
    if (hInst.IsNull()) {
        return NULL;
    }

    private_instance& Inst = (*s_lRegisteredInst)(hInst);
    return Inst.pGeom;
}

//=============================================================================

void render::SetTextureProjection(const Matrix4&         L2W,
                                  Radian                 FOV,
                                  float                  Length,
                                  const texture::handle& Texture)
{
    /*
    if (Texture.getPointer() != nullptr) {
        // Test if the projection is too perpendicular to the view, and turn it
        // off if it is. This can cause too many artifacts where the projection
        // would be clipped (we can't do clipping for performance reasons). We
        // may still see the occasional artifact, but hopefully this will keep it
        // to an absolute minimum.
        const view*    pView = eng_GetView();
        const Matrix4& V2W = pView->GetV2W();
        Vector3        V0, V1, ViewDir, ProjDir;
        V2W.GetColumns(V0, V1, ViewDir);
        L2W.GetColumns(V0, V1, ProjDir);
        if (ViewDir.Dot(ProjDir) > 0.8660f) {
            s_bDoTextureProjection = true;
            s_TextureProjection.L2W = L2W;
            s_TextureProjection.FOV = FOV;
            s_TextureProjection.Length = Length;
            s_TextureProjection.Texture = Texture;

            platform_SetProjectedTexture(Texture);
            platform_ComputeProjTextureMatrix(s_TextureProjectionMatrix, s_TextureProjectionView, s_TextureProjection);
            platform_SetTextureProjection(s_TextureProjection);
        }
    }
        */
}

//=============================================================================

void render::SetShadowProjection(const Matrix4&         L2W,
                                 Radian                 FOV,
                                 float                  Length,
                                 const texture::handle& Texture)
{
    if ((Texture.getPointer() != NULL) && (s_nShadowProjections < 2)) {
        s_ShadowProjections[s_nShadowProjections].L2W = L2W;
        s_ShadowProjections[s_nShadowProjections].FOV = FOV;
        s_ShadowProjections[s_nShadowProjections].Length = Length;
        s_ShadowProjections[s_nShadowProjections].Texture = Texture;
/*
        platform_SetProjectedShadowTexture(s_nShadowProjections, Texture);
        platform_ComputeProjShadowMatrix(s_ShadowProjectionMatrices[s_nShadowProjections],
                                         s_ShadowProjectionViews[s_nShadowProjections],
                                         s_ShadowProjections[s_nShadowProjections]);
*/
        s_nShadowProjections++;
    }
}

//=============================================================================

void render::SetCustomFogPalette(const texture::handle& Texture, bool ImmediateSwitch, int PaletteIndex)
{
    //platform_SetCustomFogPalette(Texture, ImmediateSwitch, PaletteIndex);
}

//=============================================================================

Colour render::GetFogValue(const Vector3& WorldPos, int PaletteIndex)
{
    return COLOR_GREY; //platform_GetFogValue(WorldPos, PaletteIndex);
}

//=============================================================================

void render::BeginNormalRender()
{
    // clear out the distorted instance list
    s_lDistortionInfo.clear();

    // sort the materials
    s_lRegisteredMaterials->Sort();

    // safety check
    assert(!s_InRenderBegin && !s_InShadowBegin && !s_InRawBegin);
    s_InRenderBegin = true;

    // clear out the list of render instances
    s_LoHashMark = 0;
    s_HiHashMark = kMaxRenderedInstances - 1;
    memset(s_HashTable, 0xff, sizeof(int16_t) * kMaxRenderedInstances);

    // clear out texture and shadow projections
    s_bDoTextureProjection = false;
    s_nShadowProjections = 0;
    //texture::handle Handle;
    //platform_SetProjectedTexture(Handle);

    //platform_BeginNormalRender();
}

void render::EndNormalRender()
{
    // mark that we have no distortion or alpha meshes to render during the custom phase
    s_CustomStart = s_LoHashMark;

    // safety check
    assert(s_InRenderBegin);
    s_InRenderBegin = false;

    // bail out early if there are no instances to render
    if (s_LoHashMark == 0) {
        return;
    }

    // set up the "cube" environment texture
    //platform_CreateEnvTexture();

    // set the projected texture matrices
    //platform_SetTextureProjectionMatrix(s_TextureProjectionMatrix);
    //platform_SetShadowProjectionMatrix(0, s_ShadowProjectionMatrices[0]);
    //platform_SetShadowProjectionMatrix(1, s_ShadowProjectionMatrices[1]);
    // render the light map from the same

    // let the platform-specific shaders get initialized (whether its d3d vert/pixel
    // shaders, vu0/vu1 microcode, or gamecube passes)
    //platform_BeginShaders();

    // sort the render instances (by material and sort key)
    qsort(s_lSortData.data(), s_LoHashMark, sizeof(sort_struct), InstanceCompareFn);
    {
        // loop through all of the render instances and render those bad boys
        sortkey CurrentSortData;
        CurrentSortData.Bits = 0xffffffff;
        Geom*     pCurrentGeom = nullptr;
        geom_type CurrentType = TYPE_UNKNOWN;
        for (int iUniqueInst = 0; iUniqueInst < s_LoHashMark; iUniqueInst++) {
            render_instance& Inst = s_lRenderInst[s_lSortData[iUniqueInst].iRenderInst];

            // If this is the start of the custom distortion and alpha instances,
            // the bail out of this loop. They'll come later
            if (Inst.SortKey.RenderOrder >= ORDER_CUSTOM_START) {
                // distortion meshes have to be done separately
                s_CustomStart = iUniqueInst;
                break;
            }

            // activate the material if necessary
            if (CurrentSortData.MatIndex != Inst.SortKey.MatIndex) {
                if (pCurrentGeom != nullptr) {
                    assert(CurrentType != TYPE_UNKNOWN);
                    if (CurrentType == TYPE_RIGID) {
                        //platform_EndRigidGeom();
                    } else {
                       // platform_EndSkinGeom();
                    }
                    pCurrentGeom = nullptr;
                }

                material& Mat = (*s_lRegisteredMaterials)[Inst.SortKey.MatIndex];
                assert((Mat.m_Type != Material_Distortion) &&
                       (Mat.m_Type != Material_Distortion_PerPolyEnv));

               // platform_ActivateMaterial(Mat);

                CurrentSortData.MatIndex = Inst.SortKey.MatIndex;
            }

            // start a new instance batch if necessary (geometry sorting should already
            // be built into the sort key)
            if ((Inst.SortKey.GeomType == 0) &&
                ((pCurrentGeom != Inst.Data.Rigid.pGeom) || (CurrentSortData.GeomSubMesh != Inst.SortKey.GeomSubMesh))) {
                if (pCurrentGeom != nullptr) {
                    assert(CurrentType != TYPE_UNKNOWN);
                    if (CurrentType == TYPE_RIGID) {
                //        platform_EndRigidGeom();
                    } else {
                //        platform_EndSkinGeom();
                    }
                }

                pCurrentGeom = Inst.Data.Rigid.pGeom;
                CurrentType = TYPE_RIGID;
                CurrentSortData.GeomSubMesh = Inst.SortKey.GeomSubMesh;
                platformRenderer->BeginRigidGeom(pCurrentGeom, Inst.SortKey.GeomSubMesh);
            } else if (Inst.SortKey.GeomType &&
                       ((pCurrentGeom != Inst.Data.Skin.pGeom) || (CurrentSortData.GeomSubMesh != Inst.SortKey.GeomSubMesh))) {
                if (pCurrentGeom != nullptr) {
                    assert(CurrentType != TYPE_UNKNOWN);
                    if (CurrentType == TYPE_RIGID) {
              //          platform_EndRigidGeom();
                    } else {
             //           platform_EndSkinGeom();
                    }
                }

                pCurrentGeom = Inst.Data.Skin.pGeom;
                CurrentType = TYPE_SKIN;
                CurrentSortData.GeomSubMesh = Inst.SortKey.GeomSubMesh;
           //     platform_BeginSkinGeom(pCurrentGeom, Inst.SortKey.GeomSubMesh);
            }

            // let the platform run its render code on the instances
            int iInstToRender = s_lSortData[iUniqueInst].iRenderInst;
            if (Inst.SortKey.GeomType == 0) {
                while (iInstToRender != -1) {

                    assert(s_lRenderInst[iInstToRender].SortKey.Bits == Inst.SortKey.Bits);
          //          platform_RenderRigidInstance(s_lRenderInst[iInstToRender]);
                    iInstToRender = s_lRenderInst[iInstToRender].Brother;
                };
            } else {
                while (iInstToRender != -1) {

                    assert(s_lRenderInst[iInstToRender].SortKey.Bits == Inst.SortKey.Bits);
          //          platform_RenderSkinInstance(s_lRenderInst[iInstToRender]);
                    iInstToRender = s_lRenderInst[iInstToRender].Brother;
                };
            }
        }

        // finish up any pending tasks
        if (pCurrentGeom != nullptr) {
            assert(CurrentType != TYPE_UNKNOWN);
            if (CurrentType == TYPE_RIGID) {
          //      platform_EndRigidGeom();
            } else {
          //      platform_EndSkinGeom();
            }
        }
    }

    // let the microcode or whatever finish up
   // platform_EndShaders();
   // platform_EndNormalRender();
}

//=============================================================================

void render::BeginCustomRender(void)
{
    // safety check
    //assert(eng_InBeginEnd());
    assert(!s_InRenderBegin && !s_InShadowBegin && !s_InRawBegin && !s_InCustomBegin);
    s_InCustomBegin = true;
}

//=============================================================================

void render::EndCustomRender()
{
    // safety check
   // assert(eng_InBeginEnd());
    assert(s_InCustomBegin);
    s_InCustomBegin = false;

    // bail out early if there are no distorted meshes to render
    if (s_CustomStart == s_LoHashMark) {
        return;
    }

    // let the platform-specific shaders get initialized (whether its d3d vert/pixel
    // shaders, vu0/vu1 microcode, or gamecube passes)
    //platform_BeginShaders();

    // loop through all of the render instances and render those bad boys
    // now handle the distorted meshes
    sortkey CurrentSortData;
    CurrentSortData.Bits = 0xffffffff;
    CurrentSortData.RenderOrder = ORDER_OPAQUE;
    Geom*     pCurrentGeom = nullptr;
    geom_type CurrentType = TYPE_UNKNOWN;

    for (int iUniqueInst = s_CustomStart; iUniqueInst < s_LoHashMark; iUniqueInst++) {
        render_instance& Inst = s_lRenderInst[s_lSortData[iUniqueInst].iRenderInst];

        assert(Inst.SortKey.RenderOrder >= ORDER_CUSTOM_START);

        // set up a z-prime material or distortion map for the first time if necessary
        if (CurrentSortData.RenderOrder != Inst.SortKey.RenderOrder) {
            if (pCurrentGeom != nullptr) {
                assert(CurrentType != TYPE_UNKNOWN);
                if (CurrentType == TYPE_RIGID) {
          //          platform_EndRigidGeom();
                } else {
          //          platform_EndSkinGeom();
                }
                pCurrentGeom = nullptr;
            }

            CurrentSortData.MatIndex = 0x3ff;
            CurrentSortData.RenderOrder = Inst.SortKey.RenderOrder;

            if (CurrentSortData.RenderOrder == ORDER_ZPRIME) {
          //      platform_ActivateZPrimeMaterial();
            } else if (CurrentSortData.RenderOrder == ORDER_DISTORTION) {
           //     platform_BeginDistortion();
            }
        }

        // the distortion and fading alpha materials need to get set properly
        if (((CurrentSortData.RenderOrder == ORDER_FORCED_LAST) ||
             (CurrentSortData.RenderOrder == ORDER_FADING_ALPHA)) &&
            (CurrentSortData.MatIndex != Inst.SortKey.MatIndex)) {
            if (pCurrentGeom != nullptr) {
                assert(CurrentType != TYPE_UNKNOWN);
                if (CurrentType == TYPE_RIGID) {
          //          platform_EndRigidGeom();
                } else {
           //         platform_EndSkinGeom();
                }
                pCurrentGeom = nullptr;
            }

            material& Mat = (*s_lRegisteredMaterials)[Inst.SortKey.MatIndex];
        //    platform_ActivateMaterial(Mat);
            CurrentSortData.MatIndex = Inst.SortKey.MatIndex;
        } else if ((CurrentSortData.RenderOrder == ORDER_DISTORTION) &&
                   (CurrentSortData.MatIndex != Inst.SortKey.MatIndex)) {
            if (pCurrentGeom != nullptr) {
                assert(CurrentType != TYPE_UNKNOWN);
                if (CurrentType == TYPE_RIGID) {
        //            platform_EndRigidGeom();
                } else {
        //            platform_EndSkinGeom();
                }
                pCurrentGeom = nullptr;
            }

            if (Inst.OverrideMat) {
                const distortion_info& DistortInfo = s_lDistortionInfo[(int)Inst.SortKey.MatIndex];
                if (DistortInfo.MatIndex != 0xffffffff) {
                    material& Mat = (*s_lRegisteredMaterials)[DistortInfo.MatIndex];
        //            platform_ActivateDistortionMaterial(&Mat, DistortInfo.NormalRot);
                } else {
        //            platform_ActivateDistortionMaterial(NULL, DistortInfo.NormalRot);
                }
            } else {
                material& Mat = (*s_lRegisteredMaterials)[Inst.SortKey.MatIndex];
        //        platform_ActivateMaterial(Mat);
            }

            CurrentSortData.MatIndex = Inst.SortKey.MatIndex;
        }

        // start a new instance batch if necessary (geometry sorting should already
        // be built into the sort key)
        if ((Inst.SortKey.GeomType == 0) &&
            ((pCurrentGeom != Inst.Data.Rigid.pGeom) || (CurrentSortData.GeomSubMesh != Inst.SortKey.GeomSubMesh))) {
            if (pCurrentGeom != nullptr) {
                assert(CurrentType != TYPE_UNKNOWN);
                if (CurrentType == TYPE_RIGID) {
        //            platform_EndRigidGeom();
                } else {
         //           platform_EndSkinGeom();
                }
            }

            pCurrentGeom = Inst.Data.Rigid.pGeom;
            CurrentType = TYPE_RIGID;
            CurrentSortData.GeomSubMesh = Inst.SortKey.GeomSubMesh;
     //       platform_BeginRigidGeom(pCurrentGeom, Inst.SortKey.GeomSubMesh);
        } else if (Inst.SortKey.GeomType &&
                   ((pCurrentGeom != Inst.Data.Skin.pGeom) || (CurrentSortData.GeomSubMesh != Inst.SortKey.GeomSubMesh))) {
            if (pCurrentGeom != nullptr) {
                assert(CurrentType != TYPE_UNKNOWN);
                if (CurrentType == TYPE_RIGID) {
     //               platform_EndRigidGeom();
                } else {
     //               platform_EndSkinGeom();
                }
            }

            pCurrentGeom = Inst.Data.Skin.pGeom;
            CurrentType = TYPE_SKIN;
            CurrentSortData.GeomSubMesh = Inst.SortKey.GeomSubMesh;
    //        platform_BeginSkinGeom(pCurrentGeom, Inst.SortKey.GeomSubMesh);
        }

        // let the platform run its render code on the instances
        int iInstToRender = s_lSortData[iUniqueInst].iRenderInst;
        if (Inst.SortKey.GeomType == 0) {
            while (iInstToRender != -1) {
                assert(s_lRenderInst[iInstToRender].SortKey.Bits == Inst.SortKey.Bits);
    //            platform_RenderRigidInstance(s_lRenderInst[iInstToRender]);
                iInstToRender = s_lRenderInst[iInstToRender].Brother;
            };
        } else {
            while (iInstToRender != -1) {
                assert(s_lRenderInst[iInstToRender].SortKey.Bits == Inst.SortKey.Bits);
    //            platform_RenderSkinInstance(s_lRenderInst[iInstToRender]);
                iInstToRender = s_lRenderInst[iInstToRender].Brother;
            };
        }
    }

    // finish up any pending tasks
    if (pCurrentGeom != nullptr) {
        assert(CurrentType != TYPE_UNKNOWN);
        if (CurrentType == TYPE_RIGID) {
    //        platform_EndRigidGeom();
        } else {
    //        platform_EndSkinGeom();
        }
    }

    // end distortion
    if (CurrentSortData.RenderOrder == ORDER_DISTORTION) {
 //       platform_EndDistortion();
    }

    // let the microcode or whatever finish up
  //  platform_EndShaders();
}

//=============================================================================

void render::ResetAfterException()
{
    s_InRenderBegin = false;
    s_InShadowBegin = false;
    s_InRawBegin = false;
}

//=============================================================================

void render::AddRigidInstanceSimple(hgeom_inst     hInst,
                                    const void*    pCol,
                                    const Matrix4* pL2W,
                                    const BBox&    WorldBBox,
                                    uint32_t       Flags)
{
    // safety check
    assert(s_InRenderBegin);
    //assert(pL2W->IsValid());
    // grab the useful pointers out
    private_instance& RegisteredInst = (*s_lRegisteredInst)(hInst);
    assert(RegisteredInst.Type == TYPE_RIGID);
    RigidGeom* pGeom = (RigidGeom*)RegisteredInst.pGeom;

    // calculate lighting
    void* pLighting = nullptr;
    if (Flags & DO_SIMPLE_LIGHTING) {
        pLighting = nullptr; //platform_CalculateRigidLighting(*pL2W, WorldBBox);
        if (pLighting) {
            Flags |= INSTFLAG_DYNAMICLIGHT;
        }
    }

    // collect texture projections
    uint32_t ProjFlags = CollectProjectionInfo(Flags, WorldBBox);

    // Use filter light?
    if ((s_bFilterLight) && ((Flags & DISABLE_FILTERLIGHT) == 0)) {
        Flags |= INSTFLAG_FILTERLIGHT;
    }

    // add each of the submeshes to the render list
    for (int iMesh = 0; iMesh < pGeom->numMeshes; iMesh++) {
        Geom::Mesh& Mesh = pGeom->meshes[iMesh];
        for (int iSubMesh = Mesh.iSubMesh; iSubMesh < Mesh.iSubMesh + Mesh.nSubMeshes; iSubMesh++) {
            Geom::Submesh& SubMesh = pGeom->subMeshes[iSubMesh];

            // get the material handle info
            Geom::Material& Material = pGeom->materials[SubMesh.iMaterial];
            xhandle         hMat = pGeom->virtualMaterials[Material.iVirtualMat].MatHandle;
            assert((hMat >= 0) && (hMat < kMaxRegisteredMaterials));

            const uint16_t* pInstCol = (uint16_t*)pCol;

            // figure out the sort key
            sortkey SortKey;
            SortKey.Bits = SubMesh.baseSortKey;
            SortKey.MatIndex = s_lRegisteredMaterials->GetIndexByHandle(hMat);

            // fill in the basic render instance info
            render_instance& Inst = AddToHashHybrid(SortKey.Bits);
            Inst.Flags = Flags;
            Inst.OverrideMat = false;
            Inst.Alpha = 255;

            // get scrolling uv information
            GetUVOffset(Inst.UOffset, Inst.VOffset, pGeom, (*s_lRegisteredMaterials)(hMat));

            // fill in the rigid geom instance info
            Inst.Data.Rigid.pGeom = pGeom;
            Inst.Data.Rigid.pL2W = pL2W;
            Inst.Data.Rigid.pColInfo = pInstCol;

            // fill in the lighting
            Inst.pLighting = pLighting;

            // do texture projections
            if ((Inst.SortKey.RenderOrder < ORDER_ZPRIME) &&
                CanHaveProjTexture(Material)) {
                Inst.Flags |= ProjFlags;
            }
        }
    }
}

//=============================================================================

void render::AddRigidInstance(hgeom_inst     hInst,
                              const void*    pCol,
                              const Matrix4* pL2W,
                              uint64_t       Mask,
                              uint32_t       Flags,
                              int            Alpha)
{
    // safety check
    assert(s_InRenderBegin);
    //assert(pL2W->IsValid());

    // grab the useful pointers out
    private_instance& RegisteredInst = (*s_lRegisteredInst)(hInst);
    assert(RegisteredInst.Type == TYPE_RIGID);
    RigidGeom* pGeom = (RigidGeom*)RegisteredInst.pGeom;

    // calculate lighting
    BBox WorldBBox(pGeom->bbox);
    WorldBBox.Transform(*pL2W);
    void* pLighting = nullptr; //platform_CalculateRigidLighting(*pL2W, WorldBBox);
    if (pLighting) {
        Flags |= INSTFLAG_DYNAMICLIGHT;
    }

    // collect texture projections
    uint32_t ProjFlags = CollectProjectionInfo(Flags, WorldBBox);

    // Use filter light?
    if ((s_bFilterLight) && ((Flags & DISABLE_FILTERLIGHT) == 0)) {
        Flags |= INSTFLAG_FILTERLIGHT;
    }

    // fading alpha?
    if (Alpha != 255) {
        Flags |= INSTFLAG_FADING_ALPHA;
    }

    // add the meshes and submeshes to the render list
    int         iMesh = 0;
    Geom::Mesh* pMesh = pGeom->meshes;
    Geom::Mesh* pEndMesh = pMesh + pGeom->numMeshes;
    while (pMesh < pEndMesh) {
        // skip this mesh?
        if ((Mask & 1) == 0) {
            pMesh++;
            iMesh++;
            Mask >>= 1;
            continue;
        }

        // add each of the submeshes to the render list
        for (int iSubMesh = pMesh->iSubMesh;
             iSubMesh < pMesh->iSubMesh + pMesh->nSubMeshes;
             iSubMesh++) {
            Geom::Submesh&  SubMesh = pGeom->subMeshes[iSubMesh];
            Geom::Material& Material = pGeom->materials[SubMesh.iMaterial];

            // get the material handle info
            xhandle hMat = pGeom->virtualMaterials[Material.iVirtualMat].MatHandle;

            // range safety check for the sort key
            assert((pGeom->m_hGeom >= 0) && (pGeom->m_hGeom < kMaxRegisteredGeoms));
            assert((hMat >= 0) && (hMat < kMaxRegisteredMaterials));
            assert((iSubMesh >= 0) && (iSubMesh < 256));

            int iBone = pGeom->system.pPC[SubMesh.iDList].iBone;

            const uint16_t* pInstCol = (uint16_t*)pCol;

            // build the sort key
            sortkey SortKey;
            SortKey.Bits = 0;
            SortKey.GeomSubMesh = iSubMesh;
            SortKey.GeomHandle = pGeom->m_hGeom;
            SortKey.GeomType = 0;
            SortKey.MatIndex = s_lRegisteredMaterials->GetIndexByHandle(hMat);
            SortKey.RenderOrder = GetRenderOrder((material_type)Material.type);
            if ((Flags & render::FADING_ALPHA) && (SortKey.RenderOrder < ORDER_FADING_ALPHA)) {
                SortKey.RenderOrder = ORDER_FADING_ALPHA;
            }

            // make a copy of the l2w in smem that we can ref to
            Matrix4* pMat = (Matrix4*)smem_BufferAlloc(sizeof(Matrix4));
            {
                *pMat = *(pL2W + iBone);
                //assert(pMat->IsValid());
            }

            // fill in the basic render instance info
            render_instance& Inst = AddToHashHybrid(SortKey.Bits);
            Inst.SortKey = SortKey;
            Inst.Flags = Flags;
            Inst.OverrideMat = false;
            Inst.Alpha = Alpha;

            // get scrolling uv information
            GetUVOffset(Inst.UOffset, Inst.VOffset, pGeom, (*s_lRegisteredMaterials)(hMat));

            // fill in the rigid geom instance info
            Inst.Data.Rigid.pGeom = pGeom;
            Inst.Data.Rigid.pL2W = pMat;
            Inst.Data.Rigid.pColInfo = pInstCol;

            // fill in the lighting
            Inst.pLighting = pLighting;

            // do texture projections
            if ((Inst.SortKey.RenderOrder < ORDER_ZPRIME) &&
                CanHaveProjTexture(Material)) {
                Inst.Flags |= ProjFlags;
            }

#ifdef TARGET_PC
            Inst.hDList = RegisteredInst.RigidDList[(int)SubMesh.iDList];
#endif // TARGET_PC

            // handle fading geometry
            if (Flags & render::FADING_ALPHA) {
                // we need to render twice to prime the z-buffer for alpha geometry
                SortKey.GeomSubMesh = iSubMesh;
                SortKey.GeomHandle = pGeom->m_hGeom;
                SortKey.GeomType = 0;
                SortKey.MatIndex = 0x3ff;
                SortKey.RenderOrder = ORDER_ZPRIME;
                render_instance& ZPrimeInst = AddToHashHybrid(SortKey.Bits);
                ZPrimeInst.Flags = Inst.Flags & render::CLIPPED;
                ZPrimeInst.pLighting = pLighting;
                ZPrimeInst.Data = Inst.Data;
                ZPrimeInst.UOffset = Inst.UOffset;
                ZPrimeInst.VOffset = Inst.VOffset;
                ZPrimeInst.Alpha = 0x80;
                ZPrimeInst.OverrideMat = 1;
#ifdef TARGET_PC
                ZPrimeInst.hDList = Inst.hDList;
#endif
            }
        }

        // next mesh
        iMesh++;
        pMesh++;
        Mask >>= 1;
    }
}

//=============================================================================

void render::AddRigidInstance(hgeom_inst     hInst,
                              const void*    pCol,
                              const Matrix4* pL2W,
                              uint64_t       Mask,
                              uint32_t       VTextureMask,
                              uint32_t       Flags,
                              int            Alpha)
{
    // safety check
    assert(s_InRenderBegin);
    //assert(pL2W->IsValid());

    // grab the useful pointers out
    private_instance& RegisteredInst = (*s_lRegisteredInst)(hInst);
    assert(RegisteredInst.Type == TYPE_RIGID);
    RigidGeom* pGeom = (RigidGeom*)RegisteredInst.pGeom;

    // calculate lighting
    BBox WorldBBox(pGeom->bbox);
    WorldBBox.Transform(*pL2W);
    void* pLighting = nullptr; //platform_CalculateRigidLighting(*pL2W, WorldBBox);
    if (pLighting) {
        Flags |= INSTFLAG_DYNAMICLIGHT;
    }

    // collect texture projections
    uint32_t ProjFlags = CollectProjectionInfo(Flags, WorldBBox);

    // Use filter light?
    if ((s_bFilterLight) && ((Flags & DISABLE_FILTERLIGHT) == 0)) {
        Flags |= INSTFLAG_FILTERLIGHT;
    }

    // fading alpha?
    if (Alpha != 255) {
        Flags |= INSTFLAG_FADING_ALPHA;
    }

    // calculate the virtual mesh offsets
    int VMatOffsets[32];
    CalcVMatOffsets(VMatOffsets, pGeom, VTextureMask);

    // add the meshes and submeshes to the render list
    int         iMesh = 0;
    Geom::Mesh* pMesh = pGeom->meshes;
    Geom::Mesh* pEndMesh = pMesh + pGeom->numMeshes;
    while (pMesh < pEndMesh) {
        // skip this mesh?
        if ((Mask & 1) == 0) {
            pMesh++;
            iMesh++;
            Mask >>= 1;
            continue;
        }

        // add each of the submeshes to the render list
        for (int iSubMesh = pMesh->iSubMesh;
             iSubMesh < pMesh->iSubMesh + pMesh->nSubMeshes;
             iSubMesh++) {
            Geom::Submesh&  SubMesh = pGeom->subMeshes[iSubMesh];
            Geom::Material& Material = pGeom->materials[SubMesh.iMaterial];

            // get the material handle info
            xhandle hMat = pGeom->virtualMaterials[Material.iVirtualMat + VMatOffsets[SubMesh.iMaterial]].MatHandle;

            // range safety check for the sort key
            assert((pGeom->m_hGeom >= 0) && (pGeom->m_hGeom < kMaxRegisteredGeoms));
            assert((hMat >= 0) && (hMat < kMaxRegisteredMaterials));
            assert((iSubMesh >= 0) && (iSubMesh < 256));

            // figure out the bone we should render with
            int iBone = pGeom->system.pPC[SubMesh.iDList].iBone;

            // set the color pointer
            const uint16_t* pInstCol = (uint16_t*)pCol;

            // build the sort key
            sortkey SortKey;
            SortKey.Bits = 0;
            SortKey.GeomSubMesh = iSubMesh;
            SortKey.GeomHandle = pGeom->m_hGeom;
            SortKey.GeomType = 0;
            SortKey.MatIndex = s_lRegisteredMaterials->GetIndexByHandle(hMat);
            SortKey.RenderOrder = GetRenderOrder((material_type)Material.type);
            if ((Flags & render::FADING_ALPHA) && (SortKey.RenderOrder < ORDER_FADING_ALPHA)) {
                SortKey.RenderOrder = ORDER_FADING_ALPHA;
            }

            // make a copy of the l2w in smem that we can ref to
            Matrix4* pMat = (Matrix4*)smem_BufferAlloc(sizeof(Matrix4));
            {
                *pMat = *(pL2W + iBone);
                //assert(pMat->IsValid());
            }

            // fill in the basic render instance info
            render_instance& Inst = AddToHashHybrid(SortKey.Bits);
            Inst.SortKey = SortKey;
            Inst.Flags = Flags;
            Inst.OverrideMat = false;
            Inst.Alpha = Alpha;

            // get scrolling uv information
            GetUVOffset(Inst.UOffset, Inst.VOffset, pGeom, (*s_lRegisteredMaterials)(hMat));

            // fill in the rigid geom instance info
            Inst.Data.Rigid.pGeom = pGeom;
            Inst.Data.Rigid.pL2W = pMat;
            Inst.Data.Rigid.pColInfo = pInstCol;

            // fill in the lighting
            Inst.pLighting = pLighting;

            // do texture projections
            if ((Inst.SortKey.RenderOrder < ORDER_ZPRIME) &&
                CanHaveProjTexture(Material)) {
                Inst.Flags |= ProjFlags;
            }

#ifdef TARGET_PC
            Inst.hDList = RegisteredInst.RigidDList[(int)SubMesh.iDList];
#endif // TARGET_PC

            // handle fading geometry
            if (Flags & render::FADING_ALPHA) {
                // we need to render twice to prime the z-buffer for alpha geometry
                SortKey.GeomSubMesh = iSubMesh;
                SortKey.GeomHandle = pGeom->m_hGeom;
                SortKey.GeomType = 0;
                SortKey.MatIndex = 0x3ff;
                SortKey.RenderOrder = ORDER_ZPRIME;
                render_instance& ZPrimeInst = AddToHashHybrid(SortKey.Bits);
                ZPrimeInst.Flags = Inst.Flags & render::CLIPPED;
                ZPrimeInst.pLighting = pLighting;
                ZPrimeInst.Data = Inst.Data;
                ZPrimeInst.UOffset = Inst.UOffset;
                ZPrimeInst.VOffset = Inst.VOffset;
                ZPrimeInst.Alpha = 0x80;
                ZPrimeInst.OverrideMat = 1;
#ifdef TARGET_PC
                ZPrimeInst.hDList = Inst.hDList;
#endif
            }
        }

        // next mesh
        iMesh++;
        pMesh++;
        Mask >>= 1;
    }
}

//=============================================================================

void render::AddSkinInstance(hgeom_inst     hInst,
                             const Matrix4* pBone,
                             uint64_t       Mask,
                             uint32_t       VTextureMask,
                             uint32_t       Flags,
                             const Colour&  Ambient)
{
    // safety check
    assert(s_InRenderBegin);

    // grab the useful pointers out
    private_instance& RegisteredInst = (*s_lRegisteredInst)(hInst);
    assert(RegisteredInst.Type == TYPE_SKIN);
    SkinGeom* pGeom = (SkinGeom*)RegisteredInst.pGeom;

    // calculate lighting
    void* pLighting = nullptr; //platform_CalculateSkinLighting(Flags, pBone[0], pGeom->bbox, Ambient);
    Flags |= INSTFLAG_DYNAMICLIGHT;

    // collect texture projections
    BBox WorldBBox(pGeom->bbox);
    WorldBBox.Transform(pBone[0]);
    uint32_t ProjFlags = CollectProjectionInfo(Flags, WorldBBox);

    // calculate the virtual mesh offsets
    int VMatOffsets[32];
    CalcVMatOffsets(VMatOffsets, pGeom, VTextureMask);

    // add the meshes and submeshes to the render list
    for (int iMesh = 0; iMesh < pGeom->numMeshes; iMesh++) {
        // skip this mesh?
        if ((Mask & ((uint64_t)1 << iMesh)) == 0) {
            continue;
        }

        // add each of the submeshes to the render list
        Geom::Mesh& Mesh = pGeom->meshes[iMesh];
        for (int iSubMesh = Mesh.iSubMesh;
             iSubMesh < Mesh.iSubMesh + Mesh.nSubMeshes;
             iSubMesh++) {
            Geom::Submesh&  SubMesh = pGeom->subMeshes[iSubMesh];
            Geom::Material& Material = pGeom->materials[SubMesh.iMaterial];

            // get the material handle info
            xhandle hMat = pGeom->virtualMaterials[Material.iVirtualMat + VMatOffsets[SubMesh.iMaterial]].MatHandle;

            // range safety check for the sort key
            assert((pGeom->m_hGeom >= 0) && (pGeom->m_hGeom < kMaxRegisteredGeoms));
            assert((hMat >= 0) && (hMat < kMaxRegisteredMaterials));
            assert((iSubMesh >= 0) && (iSubMesh < 256));

            // build the sort key
            sortkey SortKey;
            SortKey.Bits = 0;
            SortKey.GeomSubMesh = iSubMesh;
            SortKey.GeomHandle = pGeom->m_hGeom;
            SortKey.GeomType = 1;
            SortKey.MatIndex = s_lRegisteredMaterials->GetIndexByHandle(hMat);
            SortKey.RenderOrder = GetRenderOrder((material_type)Material.type);
            if ((Flags & render::FADING_ALPHA) && (SortKey.RenderOrder < ORDER_FADING_ALPHA)) {
                SortKey.RenderOrder = ORDER_FADING_ALPHA;
            }
            if ((Flags & render::GLOWING) && (SortKey.RenderOrder < ORDER_GLOWING)) {
                SortKey.RenderOrder = ORDER_GLOWING;
            }
            if ((Flags & render::FORCE_LAST) && (SortKey.RenderOrder < ORDER_FORCED_LAST)) {
                SortKey.RenderOrder = ORDER_FORCED_LAST;
            }

            // fill in the basic render instance info
            render_instance& Inst = AddToHashHybrid(SortKey.Bits);
            Inst.SortKey = SortKey;
            Inst.Flags = Flags;
            Inst.OverrideMat = false;

            // get scrolling uv information
            GetUVOffset(Inst.UOffset, Inst.VOffset, pGeom, (*s_lRegisteredMaterials)(hMat));

            // fill in the alpha
            Inst.Alpha = Ambient.a;

            // fill in the skin geom instance info
            Inst.Data.Skin.pGeom = pGeom;
            Inst.Data.Skin.pBones = pBone;
            Inst.Data.Skin.Pad = 0;

            // fill in the lighting
            Inst.pLighting = pLighting;

            // do texture projections
            if ((Inst.SortKey.RenderOrder < ORDER_ZPRIME) &&
                CanHaveProjTexture(Material)) {
                Inst.Flags |= ProjFlags;
            }

#ifdef TARGET_PC
            private_geom& PrivateGeom = (*s_lRegisteredGeoms)(pGeom->m_hGeom);
            Inst.hDList = PrivateGeom.SkinDList[(int)SubMesh.iDList];
#endif

            // handle fading geometry
            if (Flags & render::FADING_ALPHA) {
                // we need to render twice to prime the z-buffer for alpha geometry
                SortKey.GeomSubMesh = iSubMesh;
                SortKey.GeomHandle = pGeom->m_hGeom;
                SortKey.GeomType = 1;
                SortKey.MatIndex = 0x3ff;
                SortKey.RenderOrder = ORDER_ZPRIME;
                render_instance& ZPrimeInst = AddToHashHybrid(SortKey.Bits);
                ZPrimeInst.Flags = Inst.Flags & render::CLIPPED;
                ZPrimeInst.pLighting = pLighting;
                ZPrimeInst.Data = Inst.Data;
                ZPrimeInst.UOffset = Inst.UOffset;
                ZPrimeInst.VOffset = Inst.VOffset;
                ZPrimeInst.Alpha = 0x80;
                ZPrimeInst.OverrideMat = 1;
#ifdef TARGET_PC
                ZPrimeInst.hDList = Inst.hDList;
#endif
            }
        }
    }
}

void render::AddSkinInstanceDistorted(hgeom_inst     hInst,
                                      const Matrix4* pBone,
                                      uint64_t       Mask,
                                      uint32_t       Flags,
                                      const Radian3& NormalRot,
                                      Colour         Ambient)
{
    // safety check
    assert(s_InRenderBegin);

    // grab the useful pointers out
    private_instance& RegisteredInst = (*s_lRegisteredInst)(hInst);
    assert(RegisteredInst.Type == TYPE_SKIN);
    SkinGeom* pGeom = (SkinGeom*)RegisteredInst.pGeom;

    // calculate lighting
    // TODO: Ignore dynamic lights for "cloaked" objects
    void* pLighting = nullptr; //platform_CalculateSkinLighting(Flags, pBone[0], pGeom->m_BBox, Ambient);
    //    void* pLighting = platform_CalculateDistortionLighting( pBone[0], pGeom->m_BBox, Ambient );
    Flags |= INSTFLAG_DYNAMICLIGHT;

    // generate a default distortion info struct for handling this guy
    int              DefaultInfoIndex = s_lDistortionInfo.size();
    s_lDistortionInfo.emplace_back();
    distortion_info& DefaultInfo = s_lDistortionInfo[DefaultInfoIndex];
    DefaultInfo.MatIndex = 0xffffffff;
    DefaultInfo.NormalRot = NormalRot;

    // add the meshes and submeshes to the render list
    for (int iMesh = 0; iMesh < pGeom->numMeshes; iMesh++) {
        // skip this mesh?
        if ((Mask & ((uint64_t)1 << iMesh)) == 0) {
            continue;
        }

        // add each of the submeshes to the render list
        Geom::Mesh& Mesh = pGeom->meshes[iMesh];
        for (int iSubMesh = Mesh.iSubMesh;
             iSubMesh < Mesh.iSubMesh + Mesh.nSubMeshes;
             iSubMesh++) {
            Geom::Submesh&  SubMesh = pGeom->subMeshes[iSubMesh];
            Geom::Material& Material = pGeom->materials[SubMesh.iMaterial];

            // get the material handle info
            xhandle hMat = pGeom->virtualMaterials[Material.iVirtualMat].MatHandle;

            // range safety check for the sort key
            assert((pGeom->m_hGeom >= 0) && (pGeom->m_hGeom < kMaxRegisteredGeoms));
            assert((hMat >= 0) && (hMat < kMaxRegisteredMaterials));
            assert((iSubMesh >= 0) && (iSubMesh < 256));

            // are we overriding a non-distortion material, or using a distortion material
            // already?!?
            if ((Material.type == Material_Distortion) ||
                (Material.type == Material_Distortion_PerPolyEnv)) {
                // NOTE: Since the default info is shared by the entire mesh, this
                // will break any cases where we've mixed distortion materials within
                // a single piece of geometry. This shouldn't ever happen, but if
                // it does, then we need a DefaultInfo per material.
                DefaultInfo.MatIndex = s_lRegisteredMaterials->GetIndexByHandle(hMat);
            }

            // build the sort key
            sortkey SortKey;
            SortKey.Bits = 0;
            SortKey.GeomSubMesh = iSubMesh;
            SortKey.GeomHandle = pGeom->m_hGeom;
            SortKey.GeomType = 1;
            SortKey.MatIndex = DefaultInfoIndex;
            SortKey.RenderOrder = ORDER_DISTORTION;

            // fill in the basic render instance info
            render_instance& Inst = AddToHashHybrid(SortKey.Bits);
            Inst.SortKey = SortKey;
            Inst.Flags = Flags;
            Inst.OverrideMat = true;

            // get scrolling uv information
            GetUVOffset(Inst.UOffset, Inst.VOffset, pGeom, (*s_lRegisteredMaterials)(hMat));

            // fill in the alpha
            Inst.Alpha = Ambient.a;

            // fill in the skin geom instance info
            Inst.Data.Skin.pGeom = pGeom;
            Inst.Data.Skin.pBones = pBone;
            Inst.Data.Skin.Pad = 0;

            // fill in the lighting
            Inst.pLighting = pLighting;

#ifdef TARGET_PC
            private_geom& PrivateGeom = (*s_lRegisteredGeoms)(pGeom->m_hGeom);
            Inst.hDList = PrivateGeom.SkinDList[(int)SubMesh.iDList];
#endif
        }
    }
}

//=============================================================================

void render::BeginMidPostEffects(void)
{
    // DS: Do we really need separate functions for the "mid" post-effects?
    // Perhaps for the x-box...need to check with Bryon...
    //platform_BeginPostEffects();
}

//=============================================================================

void render::EndMidPostEffects(void)
{
    // DS: Do we really need separate functions for the "mid" post-effects?
    // Perhaps for the x-box...need to check with Bryon...
    //platform_EndPostEffects();
}

//=============================================================================

void render::BeginPostEffects(void)
{
    //platform_BeginPostEffects();
}

//=============================================================================

void render::ApplySelfIllumGlows(float MotionBlurIntensity, int GlowCutoff)
{
    //platform_ApplySelfIllumGlows(MotionBlurIntensity, GlowCutoff);
}

//=============================================================================

void render::ZFogFilter(render::post_falloff_fn Fn, Colour Color, float Param1, float Param2)
{
    //platform_ZFogFilter(Fn, Color, Param1, Param2);
}

//=============================================================================

void render::ZFogFilter(render::post_falloff_fn Fn, int PaletteIndex)
{
    //platform_ZFogFilter(Fn, PaletteIndex);
}

//=============================================================================

void render::AddScreenWarp(const Vector3& WorldPos, float Radius, float WarpAmount)
{
    //platform_AddScreenWarp(WorldPos, Radius, WarpAmount);
}

//=============================================================================

void render::MotionBlur(float Intensity)
{
    //platform_MotionBlur(Intensity);
}

//=============================================================================

void render::MipFilter(int                     nFilters,
                       float                   Offset,
                       render::post_falloff_fn Fn,
                       Colour                  Color,
                       float                   Param1,
                       float                   Param2,
                       int                     PaletteIndex)
{
   // platform_MipFilter(nFilters, Offset, Fn, Color, Param1, Param2, PaletteIndex);
}

//=============================================================================

void render::MipFilter(int                     nFilters,
                       float                   Offset,
                       render::post_falloff_fn Fn,
                       const texture::handle&  Texture,
                       int                     PaletteIndex)
{
    //platform_MipFilter(nFilters, Offset, Fn, Texture, PaletteIndex);
}

//=============================================================================

void render::MultScreen(Colour MultColor, post_screen_blend FinalBlend)
{
    //platform_MultScreen(MultColor, FinalBlend);
}

//=============================================================================

void render::RadialBlur(float Zoom, Radian Angle, float AlphaSub, float AlphaScale)
{
   // platform_RadialBlur(Zoom, Angle, AlphaSub, AlphaScale);
}

//=============================================================================

void render::NoiseFilter(Colour Color)
{
    //platform_NoiseFilter(Color);
}

//=============================================================================

void render::ScreenFade(Colour Color)
{
   // platform_ScreenFade(Color);
}

//=============================================================================

void render::EndPostEffects()
{
   // platform_EndPostEffects();
}

material& render::GetMaterial(hgeom_inst hInst, int iSubMesh)
{
    // grab the useful pointers out
    private_instance& RegisteredInst = (*s_lRegisteredInst)(hInst);

    Geom* pGeom = RegisteredInst.pGeom;
    assert(pGeom);

    // get the internal registered material from the geometry material
    Geom::Submesh&  SubMesh = pGeom->subMeshes[iSubMesh];
    Geom::Material& Material = pGeom->materials[SubMesh.iMaterial];
    xhandle         hMat = pGeom->virtualMaterials[Material.iVirtualMat].MatHandle;
    assert((hMat >= 0) && (hMat < kMaxRegisteredMaterials));

    return (*s_lRegisteredMaterials)(hMat);
}

//=============================================================================

texture* render::GetVTexture(const Geom* pGeom,
                             int         iMaterial,
                             int         VTextureMask)
{
    // assume no offset to start with
    int VMatOffset = 0;

    // find any virtual textures that might affect this material, and if so
    // the material offset will come directly from the mask
    int i;
    for (i = 0; i < pGeom->numVirtualTextures; i++) {
        // grab the associated bits for this vtexture from the texture mask
        int Offset = VTextureMask & 0xf;
        VTextureMask >>= 4;

        // does this virtual texture affect this material?
        const Geom::VirtualTexture& VTexture = pGeom->virtualTextures[i];
        if (VTexture.materialMask & (1 << iMaterial)) {
            VMatOffset = Offset;
            break;
        }
    }

    // now, using the offset, get the texture (requires a good bit of
    // redirection, but all comes out in the end)
    const Geom::Material&         GeomMat = pGeom->materials[iMaterial];
    const Geom::VirtualMaterial& GeomVMat = pGeom->virtualMaterials[GeomMat.iVirtualMat + VMatOffset];
    xhandle                       hMat = GeomVMat.MatHandle;
    material&                     Mat = (*s_lRegisteredMaterials)(hMat);

    return Mat.m_DiffuseMap.getPointer();
}

//=============================================================================

void render::SetAreaCubeMap(const cubemap::handle& CubeMap)
{
    s_pCurrCubeMap = CubeMap.getPointer();
}

//=============================================================================
// Filter lighting functions
//=============================================================================

void render::EnableFilterLight(bool bEnable)
{
    s_bFilterLight = bEnable;
}

//=============================================================================

bool render::IsFilterLightEnabled()
{
    return s_bFilterLight;
}

//=============================================================================

void render::SetFilterLightColor(Colour Color)
{
    s_FilterLightColor = Color;
}

//=============================================================================

Colour render::GetFilterLightColor()
{
    return s_FilterLightColor;
}

void render::BeginShadowCreation()
{
    assert(!s_InRenderBegin && !s_InShadowBegin && !s_InRawBegin);

    s_InShadowBegin = true;

    // clear out the list of render instances
    s_LoHashMark = 0;
    s_HiHashMark = kMaxRenderedInstances - 1;
    memset(s_HashTable, 0xff, sizeof(int16_t) * kMaxRenderedInstances);

    // clear out any current projectors
    //platform_ClearShadowProjectorList();
    s_nDynamicShadows = 0;
}

//=============================================================================

void render::EndShadowCreation()
{
    // safety check
    //assert(eng_InBeginEnd());
    assert(s_InShadowBegin);
    s_InShadowBegin = false;

    // let the platform-specific shaders get initialized (whether its d3d vert/pixel
    // shaders, vu0/vu1 microcode, or gamecube passes)
    //platform_BeginShadowShaders();

    qsort(s_lSortData.data(), s_LoHashMark, sizeof(sort_struct), InstanceCompareFn);

    // we start by casting shadows
    //platform_StartShadowCast();

    // loop through all of the render instances and render those bad boys
    Geom*        pCurrentGeom = NULL;
    geom_type    CurrentType = TYPE_UNKNOWN;
    shad_sortkey CurrentSortData;
    CurrentSortData.Bits = 0xffffffff;
    int iUniqueInst;
    for (iUniqueInst = 0; iUniqueInst < s_LoHashMark; iUniqueInst++) {
        render_instance& Inst = s_lRenderInst[s_lSortData[iUniqueInst].iRenderInst];

        // break out of this loop if it's time to receive
        if (Inst.ShadSortKey.ShadType) {
            break;
        }

        // TODO: Handle material swaps...shadows will eventually need to work
        // with punch-through

        // start a new instance batch if necessary (geometry sorting should already
        // be built into the sort key)
        if ((Inst.ShadSortKey.GeomType == 0) &&
            ((pCurrentGeom != Inst.Data.Rigid.pGeom) || (CurrentSortData.GeomSubMesh != Inst.ShadSortKey.GeomSubMesh))) {
            if (pCurrentGeom != NULL) {
                assert(CurrentType != TYPE_UNKNOWN);
                if (CurrentType == TYPE_RIGID) {
           //         platform_EndShadowCastRigid();
                } else {
           //         platform_EndShadowCastSkin();
                }
            }

            pCurrentGeom = Inst.Data.Rigid.pGeom;
            CurrentType = TYPE_RIGID;
            CurrentSortData.GeomSubMesh = Inst.ShadSortKey.GeomSubMesh;
        //    platform_BeginShadowCastRigid(pCurrentGeom, Inst.ShadSortKey.GeomSubMesh);
        } else if (Inst.ShadSortKey.GeomType &&
                   ((pCurrentGeom != Inst.Data.Skin.pGeom) || (CurrentSortData.GeomSubMesh != Inst.ShadSortKey.GeomSubMesh))) {
            if (pCurrentGeom != NULL) {
                assert(CurrentType != TYPE_UNKNOWN);
                if (CurrentType == TYPE_RIGID) {
       //             platform_EndShadowCastRigid();
                } else {
       //             platform_EndShadowCastSkin();
                }
            }

            pCurrentGeom = Inst.Data.Skin.pGeom;
            CurrentType = TYPE_SKIN;
            CurrentSortData.GeomSubMesh = Inst.ShadSortKey.GeomSubMesh;
       //     platform_BeginShadowCastSkin(pCurrentGeom, Inst.ShadSortKey.GeomSubMesh);
        }

        // let the platform run its render code on the instances
        int iInstToRender = s_lSortData[iUniqueInst].iRenderInst;
        if (Inst.ShadSortKey.GeomType == 0) {
            while (iInstToRender != -1) {
                assert(s_lRenderInst[iInstToRender].ShadSortKey.Bits == Inst.ShadSortKey.Bits);
      //          platform_RenderShadowCastRigid(s_lRenderInst[iInstToRender]);
                iInstToRender = s_lRenderInst[iInstToRender].Brother;
            };
        } else {
            while (iInstToRender != -1) {
                assert(s_lRenderInst[iInstToRender].ShadSortKey.Bits == Inst.ShadSortKey.Bits);
        //        platform_RenderShadowCastSkin(s_lRenderInst[iInstToRender], Inst.ShadSortKey.ProjectorIndex);
                iInstToRender = s_lRenderInst[iInstToRender].Brother;
            };
        }
    }

    // finish any pending tasks
    if (pCurrentGeom != NULL) {
        assert(CurrentType != TYPE_UNKNOWN);
        if (CurrentType == TYPE_RIGID) {
      //      platform_EndShadowCastRigid();
        } else {
      //      platform_EndShadowCastSkin();
        }
    }

    // done casting, time to receive
   // platform_EndShadowCast();

   // platform_StartShadowReceive();

    pCurrentGeom = NULL;
    CurrentType = TYPE_UNKNOWN;
    CurrentSortData.Bits = 0xffffffff;
    for (; iUniqueInst < s_LoHashMark; iUniqueInst++) {
        render_instance& Inst = s_lRenderInst[s_lSortData[iUniqueInst].iRenderInst];

        // sanity check
        assert(Inst.ShadSortKey.ShadType);

        // TODO: Handle material swaps...shadows will eventually need to work
        // with punch-through

        // start a new instance batch if necessary (geometry sorting should already
        // be built into the sort key)
        if ((Inst.ShadSortKey.GeomType == 0) &&
            ((pCurrentGeom != Inst.Data.Rigid.pGeom) || (CurrentSortData.GeomSubMesh != Inst.ShadSortKey.GeomSubMesh))) {
            if (pCurrentGeom != NULL) {
                assert(CurrentType != TYPE_UNKNOWN);
                if (CurrentType == TYPE_RIGID) {
     //               platform_EndShadowReceiveRigid();
                } else {
     //               platform_EndShadowReceiveSkin();
                }
            }

            pCurrentGeom = Inst.Data.Rigid.pGeom;
            CurrentType = TYPE_RIGID;
            CurrentSortData.GeomSubMesh = Inst.ShadSortKey.GeomSubMesh;
     //       platform_BeginShadowReceiveRigid(pCurrentGeom, Inst.ShadSortKey.GeomSubMesh);
        } else if (Inst.ShadSortKey.GeomType &&
                   ((pCurrentGeom != Inst.Data.Skin.pGeom) || (CurrentSortData.GeomSubMesh != Inst.ShadSortKey.GeomSubMesh))) {
            if (pCurrentGeom != NULL) {
                assert(CurrentType != TYPE_UNKNOWN);
                if (CurrentType == TYPE_RIGID) {
      //              platform_EndShadowReceiveRigid();
                } else {
      //              platform_EndShadowReceiveSkin();
                }
            }

            pCurrentGeom = Inst.Data.Skin.pGeom;
            CurrentType = TYPE_SKIN;
            CurrentSortData.GeomSubMesh = Inst.ShadSortKey.GeomSubMesh;
     //       platform_BeginShadowReceiveSkin(pCurrentGeom, Inst.ShadSortKey.GeomSubMesh);
        }

        // let the platform run its render code on the instances
        int iInstToRender = s_lSortData[iUniqueInst].iRenderInst;
        if (Inst.ShadSortKey.GeomType == 0) {
            while (iInstToRender != -1) {
                assert(s_lRenderInst[iInstToRender].ShadSortKey.Bits == Inst.ShadSortKey.Bits);
      //          platform_RenderShadowReceiveRigid(s_lRenderInst[iInstToRender], Inst.ShadSortKey.ProjectorIndex);
                iInstToRender = s_lRenderInst[iInstToRender].Brother;
            };
        } else {
            while (iInstToRender != -1) {
                assert(s_lRenderInst[iInstToRender].ShadSortKey.Bits == Inst.ShadSortKey.Bits);
      //          platform_RenderShadowReceiveSkin(s_lRenderInst[iInstToRender]);
                iInstToRender = s_lRenderInst[iInstToRender].Brother;
            };
        }
    }

    // finish up any pending tasks
    if (pCurrentGeom != NULL) {
        assert(CurrentType != TYPE_UNKNOWN);
        if (CurrentType == TYPE_RIGID) {
    //        platform_EndShadowReceiveRigid();
        } else {
    //        platform_EndShadowReceiveSkin();
        }
    }

    // completely done
  //  platform_EndShadowReceive();
  //  platform_EndShadowShaders();
}

//=============================================================================

void render::AddPointShadowProjection(const Matrix4& L2W,
                                      Radian         FOV,
                                      float          NearZ,
                                      float          FarZ)
{
    assert(s_InShadowBegin);
  //  platform_AddPointShadowProjection(L2W, FOV, NearZ, FarZ);
    s_nDynamicShadows++;
}

//=============================================================================

void render::AddDirShadowProjection(const Matrix4& L2W,
                                    float          Width,
                                    float          Height,
                                    float          NearZ,
                                    float          FarZ)
{
    assert(s_InShadowBegin);
  //  platform_AddDirShadowProjection(L2W, Width, Height, NearZ, FarZ);
    s_nDynamicShadows++;
}

//=============================================================================

void render::AddRigidCasterSimple(render::hgeom_inst hInst,
                                  const Matrix4*     pL2W, // will be DMA ref'd to!
                                  uint64_t           ProjMask)
{
}

//=============================================================================

void render::AddRigidCaster(render::hgeom_inst hInst,
                            const Matrix4*     pL2W,
                            uint64_t           Mask,
                            uint64_t           ProjMask)
{
}

//=============================================================================

void render::AddSkinCaster(render::hgeom_inst hInst,
                           const Matrix4*     pBone,
                           uint64_t           Mask,
                           uint64_t           ProjMask)
{
    // grab the useful pointers out
    private_instance& RegisteredInst = (*s_lRegisteredInst)(hInst);
    assert(RegisteredInst.Type == TYPE_SKIN);
    SkinGeom* pGeom = (SkinGeom*)RegisteredInst.pGeom;

    // for each of the projectors, add the meshes and submeshes to the render list
    for (int iProj = 0; iProj < s_nDynamicShadows; iProj++) {
        if ((ProjMask & (1 << iProj)) == 0) {
            continue;
        }

        for (int iMesh = 0; iMesh < pGeom->numMeshes; iMesh++) {
            // skip this mesh?
            if ((Mask & ((uint64_t)1 << iMesh)) == 0) {
                continue;
            }

            // add each of the submeshes to the render list
            Geom::Mesh& Mesh = pGeom->meshes[iMesh];
            for (int iSubMesh = Mesh.iSubMesh;
                 iSubMesh < Mesh.iSubMesh + Mesh.nSubMeshes;
                 iSubMesh++) {
                // range safety check for the sort key
                assert((pGeom->m_hGeom >= 0) && (pGeom->m_hGeom < kMaxRegisteredGeoms));
                assert((iSubMesh >= 0) && (iSubMesh < 256));

                // don't let alpha cast shadows
                Geom::Submesh&  SubMesh = pGeom->subMeshes[iSubMesh];
                Geom::Material& Material = pGeom->materials[SubMesh.iMaterial];
                if (IsAlphaMaterial((material_type)Material.type)) {
                    continue;
                }

                // build the sort key
                shad_sortkey SortKey;
                SortKey.Bits = 0;
                SortKey.ProjectorIndex = iProj;
                SortKey.GeomSubMesh = iSubMesh;
                SortKey.GeomHandle = pGeom->m_hGeom;
                SortKey.GeomType = 1;
                SortKey.ShadType = 0;

                // fill in the basic render instance info
                render_instance& Inst = AddToHashHybrid(SortKey.Bits);
                Inst.ShadSortKey = SortKey;
                Inst.Flags = 0;
                Inst.UOffset = 0;
                Inst.VOffset = 0;
                Inst.OverrideMat = false;

                // fill in the skin geom instance info
                Inst.Data.Skin.pGeom = pGeom;
                Inst.Data.Skin.pBones = pBone;
                Inst.Data.Skin.Pad = 0;
            }
        }
    }
}
//=============================================================================

void render::AddRigidReceiverSimple(render::hgeom_inst hInst,
                                    const Matrix4*     pL2W, // will be DMA ref'd to!
                                    uint32_t           Flags,
                                    uint64_t           ProjMask)
{
    // safety check
    assert(s_InShadowBegin);
    //assert(pL2W->IsValid());

    // grab the useful pointers out
    private_instance& RegisteredInst = (*s_lRegisteredInst)(hInst);
    assert(RegisteredInst.Type == TYPE_RIGID);
    RigidGeom* pGeom = (RigidGeom*)RegisteredInst.pGeom;

    // for each shadow cast, add each of the submeshes to the render list
    for (int iProj = 0; iProj < s_nDynamicShadows; iProj++) {
        if ((ProjMask & (1 << iProj)) == 0) {
            continue;
        }

        for (int iSubMesh = 0; iSubMesh < pGeom->numSubMeshes; iSubMesh++) {
            // don't let alpha receive shadows
            Geom::Submesh&  SubMesh = pGeom->subMeshes[iSubMesh];
            Geom::Material& Material = pGeom->materials[SubMesh.iMaterial];
            if (IsAlphaMaterial((material_type)Material.type) && !(Material.flags & Geom::Material::FLAG_FORCE_ZFILL)) {
                continue;
            }

            // don't let punch-through receive shadows
            if (Material.flags & Geom::Material::FLAG_IS_PUNCH_THRU) {
                continue;
            }

            // build the sort key
            shad_sortkey SortKey;
            SortKey.Bits = 0;
            SortKey.ProjectorIndex = iProj;
            SortKey.GeomSubMesh = iSubMesh;
            SortKey.GeomHandle = pGeom->m_hGeom;
            SortKey.GeomType = 0;
            SortKey.ShadType = 1;

            // fill in the basic render instance info
            render_instance& Inst = AddToHashHybrid(SortKey.Bits);
            Inst.Flags = Flags;
            Inst.UOffset = 0;
            Inst.VOffset = 0;
            Inst.OverrideMat = false;

            // fill in the rigid geom info
            Inst.Data.Rigid.pGeom = pGeom;
            Inst.Data.Rigid.pL2W = pL2W;

#ifdef TARGET_PC
            Inst.hDList = RegisteredInst.RigidDList[(int)SubMesh.iDList];
#endif
        }
    }
}
//=============================================================================

void render::AddRigidReceiver(render::hgeom_inst hInst,
                              const Matrix4*     pL2W,
                              uint64_t           Mask,
                              uint32_t           Flags,
                              uint64_t           ProjMask)
{
}

//=============================================================================

void render::AddSkinReceiver(render::hgeom_inst hInst,
                             const Matrix4*     pBone,
                             uint64_t           Mask,
                             uint32_t           Flags,
                             uint64_t           ProjMask)
{
}

