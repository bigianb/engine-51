
#include "PlaySurface.h"
#include "../dataUtil/TextIn.h"

#include "../collisionMgr/CollisionMgr.h"
#include "../collisionMgr/PolyCache.h"
#include "../RigidGeomCollision.h"
#include "../render/Render.h"
//#include "Debris\Debris_mgr.hpp"

bool ShowCollision = false;

class play_surface_desc : public object_desc
{
public:
    play_surface_desc()
        : object_desc(
              Object::TYPE_PLAY_SURFACE,
              "Play Surface",
              "PLAY SURFACE",
              Object::ATTR_COLLIDABLE |
                  Object::ATTR_BLOCKS_ALL_PROJECTILES |
                  Object::ATTR_BLOCKS_ALL_ACTORS |
                  Object::ATTR_BLOCKS_RAGDOLL |
                  Object::ATTR_BLOCKS_CHARACTER_LOS |
                  Object::ATTR_BLOCKS_PLAYER_LOS |
                  Object::ATTR_BLOCKS_PAIN_LOS |
                  Object::ATTR_BLOCKS_SMALL_DEBRIS |
                  Object::ATTR_RENDERABLE |
                  Object::ATTR_RECEIVE_SHADOWS |
                  Object::ATTR_EDITOR_TEMP_OBJECT |
                  Object::ATTR_SPACIAL_ENTRY,

              FLAGS_GENERIC_EDITOR_CREATE |
                  FLAGS_NO_ICON |
                  FLAGS_BURN_VERTEX_LIGHTING)
    {
    }

    Object* Create(ObjectManager* om, collision_mgr* cm)
    {
        return new play_surface(om);
    }

    //-------------------------------------------------------------------------

    virtual const char* QuickResourceName() const
    {
        return "RigidGeom";
    }

    //-------------------------------------------------------------------------

    virtual const char* QuickResourcePropertyName() const
    {
        return "RenderInst\\File";
    }
};

static play_surface_desc s_PlaySurface_Desc;

const object_desc& play_surface::GetTypeDesc() const
{
    return s_PlaySurface_Desc;
}

const object_desc& play_surface::GetObjectType()
{
    return s_PlaySurface_Desc;
}

play_surface::play_surface(ObjectManager* om) : Object(om)
{
}

play_surface::~play_surface()
{
}

void play_surface::OnImport(text_in& TextIn)
{
}

BBox play_surface::GetLocalBBox() const
{
    RigidGeom* pRigidGeom = m_Inst.GetRigidGeom();
    if (pRigidGeom == nullptr) {
        BBox bb;
        bb.Set(Vector3(-200, -200, -200), Vector3(200, 200, 200));
        return bb;
    }

    // The geometry bbox should incorporate both the collision and render
    // geometry (which doesn't always match up). Unfortunately, since we use
    // the same bbox for both operations, we have to go with the bigger one.
    return pRigidGeom->bbox;
}

//=============================================================================

void play_surface::OnRender()
{
    RigidGeom* pRigidGeom = m_Inst.GetRigidGeom();

    if (pRigidGeom) {
        uint32_t Flags = (GetFlagBits() & Object::FLAG_CHECK_PLANES) ? render::CLIPPED : 0;
        if (GetAttrBits() & Object::ATTR_DISABLE_PROJ_SHADOWS) {
            Flags |= render::DISABLE_PROJ_SHADOWS;
        }

        if (pRigidGeom->numBones > 1) {
            assert(false);
            //ASSERTS(0, xfs("Play surface can't use multi-bone geometry (%s)", m_Inst.GetRigidGeomName()));
        } else {
            m_Inst.Render(&GetL2W(), Flags | GetRenderMode());
        }
    }
}

//=============================================================================

void play_surface::DoColCheck(const Matrix4* pBone)
{
    RigidGeom* pRigidGeom = m_Inst.GetRigidGeom();

    RigidGeom_ApplyCollision(GetGuid(),
                             GetBBox(),
                             m_Inst.GetLODMask(0xFFFF),
                             pBone,
                             pRigidGeom);
}

//=============================================================================

void play_surface::OnColCheck()
{
    DoColCheck(&GetL2W());
}

//=============================================================================

void play_surface::OnPolyCacheGather()
{
    RigidGeom_GatherToPolyCache(GetGuid(),
                                GetBBox(),
                                m_Inst.GetLODMask(0xFFFF),
                                &GetL2W(),
                                m_Inst.GetRigidGeom());
}

bool play_surface::GetColDetails(int Key, detail_tri& Tri)
{
    if (Key == -1) {
        return false;
    }

    RigidGeom* pRigidGeom = m_Inst.GetRigidGeom();
    if (!pRigidGeom) {
        return false;
    }

    if (!pRigidGeom->collision.numHighClusters) {
        return false;
    }

    // From RigidGeomCollision
    return RigidGeom_GetColDetails(pRigidGeom,
                                   &GetL2W(),
                                   m_Inst.GetColorTable(),
                                   Key,
                                   Tri);
}

//=============================================================================

const Matrix4* play_surface::GetBoneL2Ws(void)
{
    // Just 1 bone in a play surface
    return &GetL2W();
}

//=============================================================================

void play_surface::OnEnumProp(prop_enum& List)
{
    Object::OnEnumProp(List);
    m_Inst.OnEnumProp(List);
}

//=============================================================================

bool play_surface::OnProperty(prop_query& I)
{
    // HACK: fix this later!
    SetFlagBits(GetFlagBits() | FLAG_DIRTY_TRANSLATION);

    if (Object::OnProperty(I)) {
        return (true);
    }

    if (m_Inst.OnProperty(I)) {
        return (true);
    }

    return (false);
}

//=============================================================================

void play_surface::OnKill()
{
    //
    // Be sure polycache knows this object just got nuked
    //
    g_PolyCache.InvalidateCells(GetBBox(), GetGuid());

    Object::OnKill();
}


void play_surface::OnTransform(const Matrix4& L2W)
{
    Object::OnTransform(L2W);
}
