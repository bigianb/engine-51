#include "Object.h"
#include "../objectManager/ObjectManager.h"
#include <cassert>

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

void Object::UpdateRenderableLinks()
{
    // TODO: Implement this function to update the renderable links.
}

void Object::OnActivate(bool Flag)
{
    if (Flag) {
        // You can't set an object to have logic if the type desc doesn't have that bit
        // assert(GetTypeDesc().GetAttrBits() & ATTR_NEEDS_LOGIC_TIME);
        m_AttrBits |= ATTR_NEEDS_LOGIC_TIME;
    } else {
        m_AttrBits &= ~ATTR_NEEDS_LOGIC_TIME;
    }
}

void Object::OnMove(const Vector3& NewPos)
{
    assert(NewPos.IsValid());

    assert(std::abs(NewPos.GetX()) <= 1000000.0f);
    assert(std::abs(NewPos.GetY()) <= 1000000.0f);
    assert(std::abs(NewPos.GetZ()) <= 1000000.0f);

    m_FlagBits |= FLAG_DIRTY_TRANSLATION;
    m_L2W.SetTranslation(NewPos);
    UpdateTransform(); //    Being called in OnSpatialUpdate
    OnSpatialUpdate();
}

void Object::OnMoveRel(const Vector3& DeltaPos)
{
    OnMove(GetPosition() + DeltaPos);
}

void Object::OnTransform(const Matrix4& L2W)
{
    // Mark to recompute the W2L
    m_FlagBits |= FLAG_DIRTY_TRANSFORM;
    m_L2W = L2W;
    UpdateTransform();
    OnSpatialUpdate();
}

void Object::OnColCheck()
{
    /* IJB
    if( g_CollisionMgr.IsUsingHighPoly() )
    {
        // This is because of the editor. In reality this should not be here
        g_CollisionMgr.StartApply( GetGuid() );
        g_CollisionMgr.ApplySphere( GetPosition(), 10 );
        g_CollisionMgr.EndApply();
    }
    else
    {
        g_CollisionMgr.StartApply( GetGuid() ) ;
        g_CollisionMgr.ApplyAABBox( GetBBox() ) ;
        g_CollisionMgr.EndApply() ;
    }
        */
}

void Object::OnEnumProp(prop_enum& List)
{
    List.PropEnumString("Base", "This is the base class for all the objects.", PROP_TYPE_HEADER);
    List.PropEnumString("Base\\Name", "Name of object", 0);
    List.PropEnumBool("Base\\Hidden", "true - Object is not rendered in the editor", PROP_TYPE_DONT_SAVE);
    List.PropEnumBool("Base\\Selectable", "true - Object is selectable in the editor", PROP_TYPE_DONT_SAVE);
    List.PropEnumVector3("Base\\Position", "Position of the object in world space", 0);
    List.PropEnumRotation("Base\\Rotation", "Rotation of the object in world space", 0);
    List.PropEnumBBox("Base\\WorldBBox", "BBox of the object in world space", PROP_TYPE_READ_ONLY);
    List.PropEnumBBox("Base\\LocalBBox", "BBox of the object in local space", PROP_TYPE_READ_ONLY);
    List.PropEnumGuid("Base\\GUID", "Unique Identifier of the object", PROP_TYPE_READ_ONLY);
    List.PropEnumBool("Base\\Permeable", "Object is permeable", PROP_TYPE_READ_ONLY);
    List.PropEnumBool("Base\\DisableProjShadows", "Object cannot receive projector shadows (i.e. artist-placed shadows)", 0);
    List.PropEnumBool("Base\\CastShadows", "Object can cast dynamic shadows", 0);
    List.PropEnumBool("Base\\ReceiveShadows", "Object can receive dynamic shadows", 0);
    List.PropEnumInt("Base\\ZoneInfo", "HIDDEN - 8 bits for zone1, 8 bits for zone2", PROP_TYPE_DONT_SHOW);
    List.PropEnumInt("Base\\Attrs", "HIDDEN - attr bits for copying", PROP_TYPE_DONT_SHOW | PROP_TYPE_DONT_SAVE | PROP_TYPE_DONT_EXPORT);
    List.PropEnumInt("Base\\Flags", "HIDDEN - flag bits for copying", PROP_TYPE_DONT_SHOW | PROP_TYPE_DONT_SAVE | PROP_TYPE_DONT_EXPORT);

    List.PropEnumBool("Base\\Collision", "These are the collision flags for all objects.", PROP_TYPE_HEADER);
    List.PropEnumBool("Base\\Collision\\Collision On", "Turn ALL collision On/Off for object", PROP_TYPE_MUST_ENUM);
    if ((GetAttrBits() & ATTR_COLLIDABLE) == ATTR_COLLIDABLE) {
        List.PropEnumBool("Base\\Collision\\Block Player", "Block Player", 0);
        List.PropEnumBool("Base\\Collision\\Block Character", "Block Character", 0);
        List.PropEnumBool("Base\\Collision\\Block Ragdoll", "Block Ragdoll", 0);
        List.PropEnumBool("Base\\Collision\\Block Small Projectile", "Block Small Projectile", 0);
        List.PropEnumBool("Base\\Collision\\Block Large Projectile", "Block Large Projectile", 0);
        List.PropEnumBool("Base\\Collision\\Block Character LOS", "Block Character LOS", 0);
        List.PropEnumBool("Base\\Collision\\Block Player LOS", "Block Player LOS", 0);
        List.PropEnumBool("Base\\Collision\\Block Pain LOS", "Block Pain LOS", 0);
        List.PropEnumBool("Base\\Collision\\Block Small Debris", "Block Small Debris", 0);
    }
}

bool Object::HandleAttribute(prop_query& I, const char* PropName, uint32_t AttrBit)
{
    if (I.IsVar(PropName)) {
        if (I.IsRead()) {
            I.SetVarBool((GetAttrBits() & AttrBit) ? (true) : (false));
        } else {
            if (I.GetVarBool()) {
                TurnAttrBitsOn(AttrBit);
            } else {
                TurnAttrBitsOff(AttrBit);
            }
        }

        return true;
    }

    return false;
}

//==============================================================================

bool Object::OnProperty(prop_query& I)
{
    //
    // All properties should begin with Base!
    //
    if (!I.IsBasePath("Base")) {
        return false;
    }

    if (HandleAttribute(I, "Base\\DisableProjShadows", ATTR_DISABLE_PROJ_SHADOWS)) {
        return true;
    }

    if (HandleAttribute(I, "Base\\CastShadows", ATTR_CAST_SHADOWS)) {
        return true;
    }

    if (HandleAttribute(I, "Base\\ReceiveShadows", ATTR_RECEIVE_SHADOWS)) {
        return true;
    }

    if (I.IsBasePath("Base\\Collision")) {
        if (HandleAttribute(I, "Base\\Collision\\Collision On", ATTR_COLLIDABLE)) {
            return true;
        }
        if (HandleAttribute(I, "Base\\Collision\\Block Player", ATTR_BLOCKS_PLAYER)) {
            return true;
        }
        if (HandleAttribute(I, "Base\\Collision\\Block Character", ATTR_BLOCKS_CHARACTER)) {
            return true;
        }
        if (HandleAttribute(I, "Base\\Collision\\Block Ragdoll", ATTR_BLOCKS_RAGDOLL)) {
            return true;
        }
        if (HandleAttribute(I, "Base\\Collision\\Block Small Projectile", ATTR_BLOCKS_SMALL_PROJECTILES)) {
            return true;
        }
        if (HandleAttribute(I, "Base\\Collision\\Block Large Projectile", ATTR_BLOCKS_LARGE_PROJECTILES)) {
            return true;
        }
        if (HandleAttribute(I, "Base\\Collision\\Block Character LOS", ATTR_BLOCKS_CHARACTER_LOS)) {
            return true;
        }
        if (HandleAttribute(I, "Base\\Collision\\Block Player LOS", ATTR_BLOCKS_PLAYER_LOS)) {
            return true;
        }
        if (HandleAttribute(I, "Base\\Collision\\Block Pain LOS", ATTR_BLOCKS_PAIN_LOS)) {
            return true;
        }
        if (HandleAttribute(I, "Base\\Collision\\Block Small Debris", ATTR_BLOCKS_SMALL_DEBRIS)) {
            return true;
        }

        return false;
    }

    if (I.IsVar("Base")) {
        assert(I.IsRead());
        I.SetVarString(GetTypeDesc().GetTypeName(), 256);
    } else if (I.IsVar("Base\\Name")) {
        if (I.IsRead()) {
            I.SetVarString(GetTypeDesc().GetTypeName(), MAX_OBJECT_NAME_LENGTH);
        }
    }

    else if (I.IsVar("Base\\Position")) {
        if (I.IsRead()) {
            I.SetVarVector3(GetPosition());
        } else {
            OnMove(I.GetVarVector3());
        }
    } else if (I.IsVar("Base\\Rotation")) {
        if (I.IsRead()) {
            I.SetVarRotation(GetL2W().GetRotation());
        } else {
            Matrix4 L2W;
            L2W.Setup(Vector3(1, 1, 1), I.GetVarRotation(), GetPosition());
            OnTransform(L2W);
        }
    } else if (I.IsVar("Base\\WorldBBox")) {
        if (I.IsRead()) {
            I.SetVarBBox(GetBBox());
        } else {
            assert(0);
        }
    } else if (I.IsVar("Base\\LocalBBox")) {
        if (I.IsRead()) {
            I.SetVarBBox(GetLocalBBox());
        } else {
            assert(0);
        }
    } else if (I.IsVar("Base\\Permeable")) {
        if (I.IsRead()) {
            I.SetVarBool(m_AttrBits & ATTR_COLLISION_PERMEABLE);
        } else {
            assert(0);
        }
    } else if (I.VarGUID("Base\\GUID", m_Guid)) {
        // .. Nothing to do
    } else if (I.IsVar("Base\\Attrs")) {
        if (I.IsRead()) {
            I.SetVarInt((int)GetAttrBits());
        } else {
            SetAttrBits((uint32_t)I.GetVarInt());
        }
    } else if (I.VarInt("Base\\Flags", (int&)m_FlagBits)) {
        // .. Nothing to do
    } else if (I.IsVar("Base\\ZoneInfo")) {
        if (I.IsRead()) {
            I.SetVarInt(m_ZoneInfo);
        } else {
            int Zone = I.GetVarInt();
            SetZone1(Zone & 0xff);
            SetZone2((Zone >> 8) & 0xff);
        }
    } else {
        // could not find the property here
        return false;
    }

    return true;
}

Geom* Object::GetGeomPtr()
{
    render_inst* pRenderInst = GetRenderInstPtr();
    if (pRenderInst) {
        return pRenderInst->GetGeom();
    } else {
        return NULL;
    }
}

std::string Object::GetGeomName()
{
    render_inst* pRenderInst = GetRenderInstPtr();
    if (pRenderInst) {
        return pRenderInst->GetGeomName();
    } else {
        return "NULL";
    }
}

AnimGroup* Object::GetAnimGroupPtr()
{
    AnimGroup::handle* pAnimGroupHandle = GetAnimGroupHandlePtr();
    if (pAnimGroupHandle) {
        return pAnimGroupHandle->getPointer();
    } else {
        return nullptr;
    }
}

std::string Object::GetAnimGroupName()
{
    AnimGroup::handle* pAnimGroupHandle = GetAnimGroupHandlePtr();
    if( pAnimGroupHandle )
        return pAnimGroupHandle->getName();
    else
    return "NULL";
}

const char* Object::GetLogicalName()
{
    return GetTypeDesc().GetTypeName();
}

void Object::SetNewZoneInfo(uint16_t ZoneInfo)
{
    // TODO: Implement this function to set the new zone information.
    /*
    // remove any current zone link information
    g_ObjMgr.RemoveSlotFromZone(m_SlotID);
    if (GetAttrBits() & Object::ATTR_RENDERABLE) {
        g_ObjMgr.RemoveSlotFromRenderable(m_SlotID);
    }

    // set in the new zone and add the zone links back in
    m_ZoneInfo = ZoneInfo;
    g_ObjMgr.AddSlotToZone(m_SlotID);
    if (GetAttrBits() & Object::ATTR_RENDERABLE) {
        g_ObjMgr.AddSlotToRenderable(m_SlotID);
    }
        */
}

void Object::UpdateTransform()
{
    if (!(m_FlagBits & FLAG_DIRTY_TRANSFORM)) {
        return;
    }

    // Compute the new bbox
    m_WorldBBox = GetLocalBBox();
    m_WorldBBox.Transform(m_L2W);

    // Update the dirty bits
    m_FlagBits &= ~FLAG_DIRTY_TRANSFORM;
}

void Object::OnSpatialUpdate()
{
    assert(!(GetAttrBits() & ATTR_DESTROY));
    if (IsLoading() == false) {
        objectManager->RemoveFromSpatialDBase(m_SlotID);
        UpdateTransform();

        if (GetAttrBits() & ATTR_SPACIAL_ENTRY) {
            objectManager->AddToSpatialDBase(m_SlotID);
        }
    }
}

int Object::GetAttachPointIDByName(const char* pName) const
{
    if (strcasecmp(pName, "BaseObject") == 0) {
        return 0;
    }

    return -1;
}

std::string Object::GetAttachPointNameByID(int iAttachPt) const
{
    if (iAttachPt == 0) {
        return "BaseObject";
    }

    return "INVALID";
}

void Object::OnAttachedMove(int iAttachPt, const Matrix4& L2W)
{
    if (iAttachPt != 0) {
        return;
    }

    OnTransform(L2W);
}

bool Object::GetAttachPointData(int      iAttachPt,
                                Matrix4& L2W,
                                uint32_t Flags)
{
    if (iAttachPt == 0) {
        L2W = GetL2W();
        return true;
    }
    return false;
}

void Object::OnTriggerTransform(const Matrix4& L2W)
{
    OnTransform(L2W);
}

void Object::OnLoad(text_in& TextIn)
{
    LoadStart();

    // Okay we can savefly start loading now
    prop_interface::OnLoad(TextIn);

    LoadEnd();
}
//==============================================================================

void Object::OnPaste(const std::vector<prop_container>& Container)
{
    LoadStart();

    // Okay we can savefly start loading now
    prop_interface::OnPaste(Container);

    LoadEnd();
}

void Object::EnumAttachPoints(std::string& String) const
{
    String = "BaseObject~";
}

Object::type Object::GetType() const
{
    return GetTypeDesc().GetType();
}
