#include "ObjectManager.h"
#include "../spatialDBase/SpatialDBase.h"
#include "../Guid.h"
#include "../zoneManager/ZoneManager.h"
#include <cassert>
#include <cstring>
#include <iostream>

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

object_desc::object_desc(Object::type Type,
                         const char*  pTypeName,
                         const char*  pEditorGroupName,
                         uint32_t     CommonAttrBit,
                         uint32_t     TypeBits)
{
    assert(Type > 0);
    assert(Type < Object::TYPE_END_OF_LIST);
    assert(pTypeName);

    m_Type = Type;
    m_pTypeName = pTypeName;
    m_pEditorGroupName = pEditorGroupName;
    m_CommonAttrBits = CommonAttrBit;
    m_bRender = true;
    m_bLogic = true;
    m_bDisplayLocation = false;
    m_bTrapOnCreate = false;
    m_bTrapOnDestroy = false;
    m_bRenderHiCollision = false;
    m_bRenderLoCollision = false;

    m_ObjectCount = 0;
    m_Flags = TypeBits;
}

void object_desc::OnEnumProp(prop_enum& List)
{
    List.PropEnumHeader("ObjectDesc", "This is the object description information of an object", 0);
    List.PropEnumString("ObjectDesc\\TypeName", "This is the name of the type been descrive", PROP_TYPE_READ_ONLY);
    List.PropEnumInt("ObjectDesc\\TypeID", "This is the ID of the type been descrive", PROP_TYPE_READ_ONLY);
    List.PropEnumInt("ObjectDesc\\Count", "The total count of this type of object", PROP_TYPE_READ_ONLY);

    if (m_CommonAttrBits & Object::ATTR_RENDERABLE) {
        List.PropEnumBool("ObjectDesc\\Render", "This tells whether all the object of this type should render or not.", 0);
    }

    if (m_CommonAttrBits & (Object::ATTR_NEEDS_LOGIC_TIME)) {
        List.PropEnumBool("ObjectDesc\\Logic", "This tells whether all the object of this type should run logic or not.", 0);
    }
}

//==============================================================================

bool object_desc::OnProperty(prop_query& I)
{
    int Type = (int)m_Type;
    if (I.VarString("ObjectDesc\\TypeName", (char*)m_pTypeName, 256)) {
        // You can only read this guy
        assert(I.IsRead());
    } else if (I.VarInt("ObjectDesc\\TypeID", Type)) {
    } else if (I.VarBool("ObjectDesc\\Render", m_bRender)) {
    } else if (I.VarBool("ObjectDesc\\Logic", m_bLogic)) {
    } else if (I.VarInt("ObjectDesc\\Count", m_ObjectCount)) {
    } else {
        return false;
    }

    return true;
}

ObjectManager::ObjectManager()
{
    m_InLoop = false;
}

ObjectManager::~ObjectManager()
{
}

void ObjectManager::Init(ObjectRegistrarInterface* objectRegistrar, spatial_dbase* sdb, collision_mgr* cm, ResourceManager* rm)
{
    spatialDatabase = sdb;
    collisionMgr = cm;
    resourceManager = rm;
    objectDescriptors.clear();
    objectRegistrar->RegisterObjects(objectDescriptors);

    m_FirstSearchResult = MAX_OBJECTS;

    for (int i = 0; i < (int)Object::TYPE_END_OF_LIST; i++) {
        m_ObjectType[i].pDesc = nullptr;
    }

    for (int i = 0; i < (int)Object::TYPE_END_OF_LIST; i++) {
        int ObjType = i;
        for (const object_desc* desc : objectDescriptors) {
            if (desc->m_Type == (Object::type)ObjType) {
                //desc->m_pNextGroupType = m_ObjectType[i].pDesc;
                m_ObjectType[i].pDesc = desc;
            }
        }

        // Set the rest of the info
        m_ObjectType[i].FirstType = SLOT_NULL;
        m_ObjectType[i].FirstVis = SLOT_NULL;
        m_ObjectType[i].InstanceCount = 0;
    }

    // Hook all Slots into free list
    m_FirstFreeSlot = SLOT_NULL;

    // just walk through the whole array and set everything up
    for (int i = 0; i < MAX_OBJECTS; i++) {
        m_ObjectSlot[i].pObject = NULL;
        m_ObjectSlot[i].Next = m_FirstFreeSlot;
        m_ObjectSlot[i].Prev = SLOT_NULL;
        m_ObjectSlot[i].NextSearch = SLOT_NULL;
        m_ObjectSlot[i].NextVis = SLOT_NULL;
        m_ObjectSlot[i].NextRenderable[0] = SLOT_NULL;
        m_ObjectSlot[i].PrevRenderable[0] = SLOT_NULL;
        m_ObjectSlot[i].NextRenderable[1] = SLOT_NULL;
        m_ObjectSlot[i].PrevRenderable[1] = SLOT_NULL;
        m_ObjectSlot[i].NextZone[0] = SLOT_NULL;
        m_ObjectSlot[i].PrevZone[0] = SLOT_NULL;
        m_ObjectSlot[i].NextZone[1] = SLOT_NULL;
        m_ObjectSlot[i].PrevZone[1] = SLOT_NULL;

        for (int j = 0; j < 8; j++) {
            m_ObjectLink[((link_id)i) * 8 + j].SpacialCellID = SPATIAL_CELL_NULL;
            m_ObjectLink[((link_id)i) * 8 + j].Next = LINK_NULL;
            m_ObjectLink[((link_id)i) * 8 + j].Prev = LINK_NULL;
        }

        if (m_FirstFreeSlot != SLOT_NULL) {
            m_ObjectSlot[m_FirstFreeSlot].Prev = i;
        }

        m_FirstFreeSlot = i;
    }

    // walk through zones and set them up
    for (int i = 0; i < 256; i++) {
        m_ObjectZone[i].FirstZone[0] = SLOT_NULL;
        m_ObjectZone[i].FirstZone[1] = SLOT_NULL;
        m_ObjectZone[i].FirstRenderable[0] = SLOT_NULL;
        m_ObjectZone[i].FirstRenderable[1] = SLOT_NULL;
    }

    //
    // Register all the objects into the system
    //
    /*
    {
        for (object_desc* pNext = object_desc::s_pHead; pNext != NULL; pNext = pNext->m_pNext) {
            g_RegGameMgrs.AddManager(xfs("Object Manager\\%s", pNext->m_pTypeName), pNext);
        }
    }
        */
}

void ObjectManager::Kill()
{
    //Clear();
}

guid ObjectManager::CreateObject(const char* objectTypeName)
{
    if (objectTypeName != nullptr) {
        for (const object_desc* desc : objectDescriptors) {
            if (strcasecmp(desc->GetTypeName(), objectTypeName) == 0) {
                return CreateObject(*desc);
            }
        }
    }
    std::cout << "ERROR: ObjectManager::CreateObject: Unknown object type " << objectTypeName << std::endl;
    //assert(false);
    return guid(0);
}

void ObjectManager::CreateObject(const char* objectTypeName, guid Guid)
{
    if (objectTypeName != nullptr) {
        for (const object_desc* desc : objectDescriptors) {
            if (strcasecmp(desc->GetTypeName(), objectTypeName) == 0) {
                return CreateObject(*desc, Guid);
            }
        }
    }
    std::cout << "ERROR: ObjectManager::CreateObject: Unknown object type " << objectTypeName << std::endl;
}

void ObjectManager::ReserveGuid(guid Guid)
{
    m_ReservedGuid = Guid;
}

guid ObjectManager::CreateObject(const object_desc& desc)
{
    guid Guid;
    if (m_ReservedGuid) {
        Guid = m_ReservedGuid;
        m_ReservedGuid = 0;
    } else {
        Guid = guid_New();
    }
    CreateObject(desc, Guid);
    return Guid;
}

const object_desc* ObjectManager::GetDescFromName(const char* objectTypeName)
{
    if (objectTypeName != nullptr) {
        for (const object_desc* desc : objectDescriptors) {
            if (strcasecmp(desc->GetTypeName(), objectTypeName) == 0) {
                return desc;
            }
        }
    }

    //assert(false);
    return nullptr;
}

slot_id ObjectManager::AllocSlot()
{
    // Find slot for object
    assert(m_FirstFreeSlot != SLOT_NULL);

    slot_id Slot = m_FirstFreeSlot;

    assert(m_FirstFreeSlot != m_ObjectSlot[Slot].Next);

    m_FirstFreeSlot = m_ObjectSlot[Slot].Next;
    m_ObjectSlot[Slot].Next = SLOT_NULL;
    m_ObjectSlot[Slot].Prev = SLOT_NULL;
    m_ObjectSlot[Slot].NextRenderable[0] = SLOT_NULL;
    m_ObjectSlot[Slot].PrevRenderable[0] = SLOT_NULL;
    m_ObjectSlot[Slot].NextRenderable[1] = SLOT_NULL;
    m_ObjectSlot[Slot].PrevRenderable[1] = SLOT_NULL;
    m_ObjectSlot[Slot].NextZone[0] = SLOT_NULL;
    m_ObjectSlot[Slot].PrevZone[0] = SLOT_NULL;
    m_ObjectSlot[Slot].NextZone[1] = SLOT_NULL;
    m_ObjectSlot[Slot].PrevZone[1] = SLOT_NULL;
    if (m_FirstFreeSlot != SLOT_NULL) {
        m_ObjectSlot[m_FirstFreeSlot].Prev = SLOT_NULL;
    }

    return Slot;
}

Object* ObjectManager::CreateObject(const object_desc& Desc, slot_id Slot)
{
    Object* pObject;
    int     Type;

    assert(Slot != SLOT_NULL && Slot < MAX_OBJECTS);

    Type = Desc.GetType();

    //
    // Add slot to type list
    //
    if (m_ObjectType[Type].FirstType != SLOT_NULL) {
        m_ObjectSlot[m_ObjectType[Type].FirstType].Prev = Slot;
    }

    m_ObjectSlot[Slot].Next = m_ObjectType[Type].FirstType;
    m_ObjectSlot[Slot].Prev = SLOT_NULL;
    m_ObjectSlot[Slot].NextRenderable[0] = SLOT_NULL;
    m_ObjectSlot[Slot].PrevRenderable[0] = SLOT_NULL;
    m_ObjectSlot[Slot].NextRenderable[1] = SLOT_NULL;
    m_ObjectSlot[Slot].PrevRenderable[1] = SLOT_NULL;
    m_ObjectSlot[Slot].NextZone[0] = SLOT_NULL;
    m_ObjectSlot[Slot].PrevZone[0] = SLOT_NULL;
    m_ObjectSlot[Slot].NextZone[1] = SLOT_NULL;
    m_ObjectSlot[Slot].PrevZone[1] = SLOT_NULL;
    m_ObjectType[Type].FirstType = Slot;
    m_ObjectType[Type].InstanceCount++;

    //
    // Find the right object description to create the object
    //
    {
        assert(m_ObjectType[Type].pDesc);
        object_desc& SaveDesc = const_cast<object_desc&>(Desc);
        pObject = SaveDesc.Create(this, collisionMgr, resourceManager);
        SaveDesc.AddObjectCount(1);
        assert(pObject);

        // Inser the object in its slot
        m_ObjectSlot[Slot].pObject = pObject;
    }

    //
    // Clear links
    //
    for (int i = 0; i < 8; i++) {
        m_ObjectLink[((link_id)Slot) * 8 + i].SpacialCellID = SPATIAL_CELL_NULL;
        m_ObjectLink[((link_id)Slot) * 8 + i].Next = LINK_NULL;
        m_ObjectLink[((link_id)Slot) * 8 + i].Prev = LINK_NULL;
    }

    // Init base values in object
    // TODO: This has to go
    pObject->m_SlotID = Slot;

    //
    // Add the slot to the zone list
    //
    AddSlotToZone(Slot);
    if (pObject->GetAttrBits() & Object::ATTR_RENDERABLE) {
        AddSlotToRenderable(Slot);
    }

    return pObject;
}

void ObjectManager::CreateObject(const object_desc& desc, guid Guid)
{
    assert(Guid != 0);
    slot_id Slot = AllocSlot();
    Object* pObject = CreateObject(desc, Slot);

    // Add the slot in the hash table
    if (m_GuidLookup.GetCapacity() == 0) {
        m_GuidLookup.SetCapacity(MAX_OBJECTS, false);
    }

    m_GuidLookup.Add(Guid, (int)Slot);

    // Initialize a few things for the object
    pObject->m_Guid = Guid;
    pObject->SetAttrBits(desc.m_CommonAttrBits);

    // Notify the object that he need to initialize
    pObject->OnInit();

    // Insert the object into the data base if we have to
    if (pObject->GetAttrBits() & Object::ATTR_SPACIAL_ENTRY) {
        AddToSpatialDBase(Slot);
    }
}

Object* ObjectManager::GetObjectByGuid(guid Guid)
{
    if ((Guid & 0xffffffff) == 0xffffffff) {
        // this is a play surface
        assert(false);
        // IJB
        //proxy_playsurface& Proxy = proxy_playsurface::GetSafeType(*m_pProxyPlaySurface);
        //Proxy.SetSurface(Guid);
        //return m_pProxyPlaySurface;
        return nullptr;
    } else {
        int Slot;
        if (m_GuidLookup.Find(Guid, Slot)) {
            return m_ObjectSlot[Slot].pObject;
        } else {
            return nullptr;
        }
    }
}

slot_id ObjectManager::GetNext(slot_id SlotID)
{
    //
    // Keep going to the next for as long as there is deleted objects
    //
    while (m_ObjectSlot[SlotID].Next != SLOT_NULL) {
        assert(GetObjectBySlot(SlotID));
        Object* pObject = GetObjectBySlot(m_ObjectSlot[SlotID].Next);
        if (!(pObject->GetAttrBits() & Object::ATTR_DESTROY)) {
            break;
        }
        SlotID = m_ObjectSlot[SlotID].Next;
    }

    assert(GetObjectBySlot(SlotID));
    return m_ObjectSlot[SlotID].Next;
}

const object_desc* ObjectManager::GetDescFromName(const char* objectTypeName) const
{
    for (const object_desc* desc : objectDescriptors) {
        if (strcasecmp(desc->GetTypeName(), objectTypeName) == 0) {
            return desc;
        }
    }

    return nullptr;
}

void ObjectManager::ChangeObjectGuid(guid CurrentGuid, guid NewGuid)
{
    // Get object info
    slot_id Slot = GetSlotFromGuid(CurrentGuid);
    Object* pObject = GetObjectBySlot(Slot);

    // Update?
    if (pObject) {
        // Update guid lookup
        m_GuidLookup.Del(CurrentGuid);
        m_GuidLookup.Add(NewGuid, Slot);

        // Update object
        pObject->m_Guid = NewGuid;

        // Make sure we worked!
        assert(GetObjectByGuid(NewGuid) == pObject);
    }
}

slot_id ObjectManager::GetSlotFromGuid(guid Guid)
{
    int Slot;

    if (m_GuidLookup.Find(Guid, Slot)) {
        return (slot_id)Slot;
    }

    return SLOT_NULL;
}

void ObjectManager::AddSlotToZone(slot_id SlotID)
{
    assert((m_ObjectSlot[SlotID].NextZone[0] == SLOT_NULL) &&
           (m_ObjectSlot[SlotID].PrevZone[0] == SLOT_NULL));
    assert((m_ObjectSlot[SlotID].NextZone[1] == SLOT_NULL) &&
           (m_ObjectSlot[SlotID].PrevZone[1] == SLOT_NULL));

    Object*  pObject = m_ObjectSlot[SlotID].pObject;
    uint16_t Zone1 = pObject->GetZone1();
    uint16_t Zone2 = pObject->GetZone2();

    m_ObjectSlot[SlotID].NextZone[0] = m_ObjectZone[Zone1].FirstZone[0];
    m_ObjectSlot[SlotID].PrevZone[0] = SLOT_NULL;
    if (m_ObjectZone[Zone1].FirstZone[0] != SLOT_NULL) {
        m_ObjectSlot[m_ObjectZone[Zone1].FirstZone[0]].PrevZone[0] = SlotID;
    }
    m_ObjectZone[Zone1].FirstZone[0] = SlotID;

    if (Zone2 != 0) {
        m_ObjectSlot[SlotID].NextZone[1] = m_ObjectZone[Zone2].FirstZone[1];
        m_ObjectSlot[SlotID].PrevZone[1] = SLOT_NULL;
        if (m_ObjectZone[Zone2].FirstZone[1] != SLOT_NULL) {
            m_ObjectSlot[m_ObjectZone[Zone2].FirstZone[1]].PrevZone[1] = SlotID;
        }
        m_ObjectZone[Zone2].FirstZone[1] = SlotID;
    }
}

void ObjectManager::RemoveSlotFromZone(slot_id SlotID)
{
    Object*  pObject = m_ObjectSlot[SlotID].pObject;
    uint16_t Zone1 = pObject->GetZone1();
    uint16_t Zone2 = pObject->GetZone2();

    slot_id Next = m_ObjectSlot[SlotID].NextZone[0];
    slot_id Prev = m_ObjectSlot[SlotID].PrevZone[0];
    if (Next != SLOT_NULL) {
        m_ObjectSlot[Next].PrevZone[0] = m_ObjectSlot[SlotID].PrevZone[0];
    }
    if (Prev != SLOT_NULL) {
        m_ObjectSlot[Prev].NextZone[0] = m_ObjectSlot[SlotID].NextZone[0];
    }
    if (SlotID == m_ObjectZone[Zone1].FirstZone[0]) {
        m_ObjectZone[Zone1].FirstZone[0] = m_ObjectSlot[SlotID].NextZone[0];
    }

    Next = m_ObjectSlot[SlotID].NextZone[1];
    Prev = m_ObjectSlot[SlotID].PrevZone[1];
    if (Next != SLOT_NULL) {
        m_ObjectSlot[Next].PrevZone[1] = m_ObjectSlot[SlotID].PrevZone[1];
    }
    if (Prev != SLOT_NULL) {
        m_ObjectSlot[Prev].NextZone[1] = m_ObjectSlot[SlotID].NextZone[1];
    }
    if (SlotID == m_ObjectZone[Zone2].FirstZone[1]) {
        m_ObjectZone[Zone2].FirstZone[1] = m_ObjectSlot[SlotID].NextZone[1];
    }

    m_ObjectSlot[SlotID].NextZone[0] = SLOT_NULL;
    m_ObjectSlot[SlotID].PrevZone[0] = SLOT_NULL;
    m_ObjectSlot[SlotID].NextZone[1] = SLOT_NULL;
    m_ObjectSlot[SlotID].PrevZone[1] = SLOT_NULL;
}

void ObjectManager::AddSlotToRenderable(slot_id SlotID)
{
    assert((m_ObjectSlot[SlotID].NextRenderable[0] == SLOT_NULL) &&
           (m_ObjectSlot[SlotID].PrevRenderable[0] == SLOT_NULL));
    assert((m_ObjectSlot[SlotID].NextRenderable[1] == SLOT_NULL) &&
           (m_ObjectSlot[SlotID].PrevRenderable[1] == SLOT_NULL));

    Object*  pObject = m_ObjectSlot[SlotID].pObject;
    uint16_t Zone1 = pObject->GetZone1();
    uint16_t Zone2 = pObject->GetZone2();

    m_ObjectSlot[SlotID].NextRenderable[0] = m_ObjectZone[Zone1].FirstRenderable[0];
    m_ObjectSlot[SlotID].PrevRenderable[0] = SLOT_NULL;
    if (m_ObjectZone[Zone1].FirstRenderable[0] != SLOT_NULL) {
        m_ObjectSlot[m_ObjectZone[Zone1].FirstRenderable[0]].PrevRenderable[0] = SlotID;
    }
    m_ObjectZone[Zone1].FirstRenderable[0] = SlotID;

    if (Zone2 != 0) {
        m_ObjectSlot[SlotID].NextRenderable[1] = m_ObjectZone[Zone2].FirstRenderable[1];
        m_ObjectSlot[SlotID].PrevRenderable[1] = SLOT_NULL;
        if (m_ObjectZone[Zone2].FirstRenderable[1] != SLOT_NULL) {
            m_ObjectSlot[m_ObjectZone[Zone2].FirstRenderable[1]].PrevRenderable[1] = SlotID;
        }
        m_ObjectZone[Zone2].FirstRenderable[1] = SlotID;
    }
}

void ObjectManager::RemoveSlotFromRenderable(slot_id SlotID)
{
    Object*  pObject = m_ObjectSlot[SlotID].pObject;
    uint16_t Zone1 = pObject->GetZone1();
    uint16_t Zone2 = pObject->GetZone2();

    slot_id Next = m_ObjectSlot[SlotID].NextRenderable[0];
    slot_id Prev = m_ObjectSlot[SlotID].PrevRenderable[0];
    if (Next != SLOT_NULL) {
        m_ObjectSlot[Next].PrevRenderable[0] = m_ObjectSlot[SlotID].PrevRenderable[0];
    }
    if (Prev != SLOT_NULL) {
        m_ObjectSlot[Prev].NextRenderable[0] = m_ObjectSlot[SlotID].NextRenderable[0];
    }
    if (SlotID == m_ObjectZone[Zone1].FirstRenderable[0]) {
        m_ObjectZone[Zone1].FirstRenderable[0] = m_ObjectSlot[SlotID].NextRenderable[0];
    }

    Next = m_ObjectSlot[SlotID].NextRenderable[1];
    Prev = m_ObjectSlot[SlotID].PrevRenderable[1];
    if (Next != SLOT_NULL) {
        m_ObjectSlot[Next].PrevRenderable[1] = m_ObjectSlot[SlotID].PrevRenderable[1];
    }
    if (Prev != SLOT_NULL) {
        m_ObjectSlot[Prev].NextRenderable[1] = m_ObjectSlot[SlotID].NextRenderable[1];
    }
    if (SlotID == m_ObjectZone[Zone2].FirstRenderable[1]) {
        m_ObjectZone[Zone2].FirstRenderable[1] = m_ObjectSlot[SlotID].NextRenderable[1];
    }

    m_ObjectSlot[SlotID].NextRenderable[0] = SLOT_NULL;
    m_ObjectSlot[SlotID].PrevRenderable[0] = SLOT_NULL;
    m_ObjectSlot[SlotID].NextRenderable[1] = SLOT_NULL;
    m_ObjectSlot[SlotID].PrevRenderable[1] = SLOT_NULL;
}

void ObjectManager::DestroyObjectEx(guid ObjectGuid, bool bRemoveNow)
{
    slot_id Slot = GetSlotFromGuid(ObjectGuid);

    // If the object does not exist, exit gracefully.
    if (SLOT_NULL == Slot) {
        return;
    }

    // Trap if we are watching for destroys of this object type
    const object_desc& Desc = *m_ObjectType[m_ObjectSlot[Slot].pObject->GetType()].pDesc;

    m_ObjectType[m_ObjectSlot[Slot].pObject->GetType()].InstanceCount--;

    // Check type loop cursors
    RemoveFromSpatialDBase(Slot);

    // Tell object to remove
    m_ObjectSlot[Slot].pObject->OnKill();

    // Remove from type list
    if (m_ObjectType[m_ObjectSlot[Slot].pObject->GetType()].FirstType == Slot) {
        m_ObjectType[m_ObjectSlot[Slot].pObject->GetType()].FirstType = m_ObjectSlot[Slot].Next;
    }

    if (m_ObjectSlot[Slot].Prev != SLOT_NULL) {
        m_ObjectSlot[m_ObjectSlot[Slot].Prev].Next = m_ObjectSlot[Slot].Next;
    }

    if (m_ObjectSlot[Slot].Next != SLOT_NULL) {
        m_ObjectSlot[m_ObjectSlot[Slot].Next].Prev = m_ObjectSlot[Slot].Prev;
    }

    // Mark this object as it dead
    m_ObjectSlot[Slot].pObject->SetAttrBits(Object::ATTR_DESTROY);

    // Remove GUID from lookup
    m_GuidLookup.Del(m_ObjectSlot[Slot].pObject->GetGuid());

    if (bRemoveNow) {
        UnlinkAndFreeObject(Slot);
    } else {
        // Added in the list of objects that need to be deleted
        m_DeleteObject.push_back(Slot);
    }
}

void ObjectManager::UnlinkAndFreeObject(slot_id Slot)
{
    // Remove from zone lists
    RemoveSlotFromZone(Slot);
    RemoveSlotFromRenderable(Slot);

    // Remove the obejct from memory
    m_ObjectSlot[Slot].pObject->GetTypeDesc().AddObjectCount(-1);
    delete m_ObjectSlot[Slot].pObject;

    // Put slot in free list
    m_ObjectSlot[Slot].Next = m_FirstFreeSlot;
    m_ObjectSlot[Slot].Prev = SLOT_NULL;

    if (m_FirstFreeSlot != SLOT_NULL) {
        m_ObjectSlot[m_FirstFreeSlot].Prev = Slot;
    }

    m_FirstFreeSlot = Slot;

    // Clear node
    m_ObjectSlot[Slot].pObject = nullptr;
}

void ObjectManager::RemoveFromSpatialDBase(guid Guid)
{
    RemoveFromSpatialDBase(GetSlotFromGuid(Guid));
}

void ObjectManager::RemoveFromSpatialDBase(slot_id SlotID)
{
    obj_cell_link* pLink = GetCellLinks();

    // Remove ourselves
    for (int i = 0; i < 8; i++) {
        // Get index to this link
        link_id LI = ((link_id)SlotID) * 8 + i;

        // Check if unused
        if (pLink[LI].SpacialCellID == SPATIAL_CELL_NULL) {
            break;
        }

        // Get access to cell
        spatial_cell& Cell = spatialDatabase->GetCell(pLink[LI].SpacialCellID);

        // Remove from linked list
        if (Cell.FirstObjectLink[0] == LI) {
            Cell.FirstObjectLink[0] = pLink[LI].Next;
        }

        if (pLink[LI].Next != LINK_NULL) {
            pLink[pLink[LI].Next].Prev = pLink[LI].Prev;
        }

        if (pLink[LI].Prev != LINK_NULL) {
            pLink[pLink[LI].Prev].Next = pLink[LI].Next;
        }

        // Check if list is empty
        if (Cell.FirstObjectLink[0] == LINK_NULL) {
            Cell.OccFlags &= (~(1 << 0));
            //            x_DebugMsg("Removed  SlotID  -- %d  --- CellID  ---  %d\n",SlotID, pLink[LI].SpacialCellID );
            spatialDatabase->UpdateCell(Cell);
        }

        pLink[LI].SpacialCellID = SPATIAL_CELL_NULL;
        pLink[LI].Next = LINK_NULL;
        pLink[LI].Prev = LINK_NULL;
    }
}

void ObjectManager::AddToSpatialDBase(guid Guid)
{
    AddToSpatialDBase(GetSlotFromGuid(Guid));
}

void ObjectManager::AddToSpatialDBase(slot_id SlotID)
{
    // Update spatial database
    int i;
    int X, Y, Z;
    int MinX, MinY, MinZ;
    int MaxX, MaxY, MaxZ;

    // Get cell regions affected
    Object* pObject = GetObjectBySlot(SlotID);
    assert(pObject);
    BBox ObjBBox(pObject->GetBBox());

    int Level = spatialDatabase->GetBBoxLevel(ObjBBox);

    spatialDatabase->GetCellRegion(ObjBBox, Level, MinX, MinY, MinZ, MaxX, MaxY, MaxZ);

    obj_cell_link* pLink = GetCellLinks();

    // Add ourselves
    i = 0;
    for (X = MinX; X <= MaxX; X++) {
        for (Y = MinY; Y <= MaxY; Y++) {
            for (Z = MinZ; Z <= MaxZ; Z++) {
                // Get index to link
                link_id LI = ((link_id)SlotID) * 8 + i;
                assert(i < 8);

                // Get access to cell
                pLink[LI].SpacialCellID = spatialDatabase->GetCellIndex(X, Y, Z, Level, true);

                spatial_cell& Cell = spatialDatabase->GetCell(pLink[LI].SpacialCellID);

                // Add link to list
                pLink[LI].Prev = LINK_NULL;
                assert(LI != Cell.FirstObjectLink[0]);

                pLink[LI].Next = Cell.FirstObjectLink[0];

                if (Cell.FirstObjectLink[0] != LINK_NULL) {
                    pLink[Cell.FirstObjectLink[0]].Prev = LI;
                }

                Cell.FirstObjectLink[0] = LI;

                // Turn on occupied flag
                Cell.OccFlags |= (1 << 0);
                i++;
            }
        }
    }
}

void ObjectManager::ResetSearchResult()
{
    assert(!m_InLoop);
    m_FirstSearchResult = SLOT_NULL;

    //
    // Update the Sequence
    //
    m_Sequence++;
    if (m_Sequence == 0) {
        for (int i = 0; i < MAX_OBJECTS; i++) {
            m_ObjectSlot[i].Sequence = 0;
        }
        m_Sequence++;
    }
}

//==============================================================================
//  FIXME:  SrcSet is not currently used.  Objects should e selected from
//          the resulting list from this object.  All objects should be used
//          if -1 is used for the srcset since that is the number that will
//          be returned by the GetFirstSearchResult call
//==============================================================================

void ObjectManager::SelectBBox(uint32_t Attribute, const BBox& bb, Object::type Type, uint32_t NotTheseAttributes)
{
    //
    // update the sequence
    //
    ResetSearchResult();

    //
    // Ask the spatial data base to collect cells that have object in there
    //
    spatialDatabase->TraverseCells(bb);

    //
    // Loop through all visible nodes that contain objects
    //

    uint16_t CI = spatialDatabase->GetFirstInSearch();

    while (CI != SPATIAL_CELL_NULL) {
        spatial_cell& Cell = spatialDatabase->GetCell(CI);

        // Check if there are any objects in cell
        if (Cell.OccFlags) {
            int Channel;
            for (Channel = 0; Channel < NUM_SPATIAL_CHANNELS; Channel++) {
                link_id LI = Cell.FirstObjectLink[Channel];

                for (; LI != LINK_NULL; LI = m_ObjectLink[LI].Next) {

                    // Get object index
                    slot_id I = LI / 8;

                    if (m_ObjectSlot[I].Sequence == m_Sequence) {
                        continue;
                    }
                    m_ObjectSlot[I].Sequence = m_Sequence;

                    Object* pObject = m_ObjectSlot[I].pObject;

                    // This is object match the attributes?
                    if ((pObject->GetAttrBits() & Attribute) == 0) {
                        continue;
                    }

                    if ((pObject->GetAttrBits() & NotTheseAttributes) != 0) {
                        continue;
                    }

                    // Make sure that it matches the type
                    if (Type != Object::TYPE_ALL_TYPES) {
                        if ((pObject->GetType() != Type)) {
                            continue;
                        }
                    }

                    if (!bb.Intersect(pObject->GetBBox())) {
                        continue;
                    }

                    m_ObjectSlot[I].Sequence = m_Sequence;
                    m_ObjectSlot[I].NextSearch = m_FirstSearchResult;
                    m_FirstSearchResult = I;
                }
            }
        }

        // Get the next cell
        CI = spatialDatabase->GetNextInSearch(CI);
    }
}

void ObjectManager::SelectRay(uint32_t Attribute, const Vector3& RayStart, const Vector3& RayEnd, Object::type Type, uint32_t notTheseAttributes)
{
    //
    // update the sequence
    //
    ResetSearchResult();

    //
    // Ask the spacial data base to collect cells that have object in there
    //
    spatialDatabase->TraverseCells(RayStart, RayEnd);

    //
    // Loop through all nodes that contain objects
    //

    uint16_t CI = spatialDatabase->GetFirstInSearch();

    while (CI != SPATIAL_CELL_NULL) {
        spatial_cell& Cell = spatialDatabase->GetCell(CI);

        // Check if there are any objects in cell
        if (Cell.OccFlags) {
            int Channel;
            for (Channel = 0; Channel < NUM_SPATIAL_CHANNELS; Channel++) {
                link_id LI = Cell.FirstObjectLink[Channel];

                for (; LI != LINK_NULL; LI = m_ObjectLink[LI].Next) {
                    // Get object index
                    slot_id I = LI / 8;

                    if (m_ObjectSlot[I].Sequence == m_Sequence) {
                        continue;
                    }
                    m_ObjectSlot[I].Sequence = m_Sequence;

                    Object* pObject = m_ObjectSlot[I].pObject;

                    // This is object match the attributes?
                    if ((pObject->GetAttrBits() & Attribute) == 0) {
                        continue;
                    }

                    if ((pObject->GetAttrBits() & notTheseAttributes) != 0) {
                        continue;
                    }

                    // Make sure that it matches the type
                    if (Type != Object::TYPE_ALL_TYPES) {
                        if ((pObject->GetType() != Type)) {
                            continue;
                        }
                    }

                    float T;
                    if (!pObject->GetBBox().Intersect(T, RayStart, RayEnd)) {
                        continue;
                    }

                    // Add it to the list
                    m_ObjectSlot[I].Sequence = m_Sequence;
                    m_ObjectSlot[I].NextSearch = m_FirstSearchResult;
                    m_FirstSearchResult = I;
                }
            }
        }

        // Get the next cell
        CI = spatialDatabase->GetNextInSearch(CI);
    }
}

void ObjectManager::Render(bool bDoPortalWalk, const view& PortalView, uint8_t StartZone)
{
    Render3dObjects(bDoPortalWalk, PortalView, StartZone);

    // Render HUD
    Render2dObjects();

    // Render screen fades after the 2d stuff so that the hud will also fade
    // eng_Begin( "Screen Fade" );
    // g_PostEffectMgr.RenderScreenFade();
    // eng_End();
}

void ObjectManager::Render3dObjects(bool bDoPortalWalk, const view& PortalView, uint8_t StartZone)
{
    if (g_ZoneMgr.GetPortalCount() == 0) {
        bDoPortalWalk = false;
    }
    Render3dPrep(bDoPortalWalk, PortalView, StartZone);

    // Handle the normal rendering
    if (eng_Begin("3d Objects")) {
        render::BeginNormalRender();
        RenderNormalObjects();

        if (g_bRenderPlaysurfaces) {
            RenderPlaySurfaces();
        }

        g_LightMgr.EndLightCollection();
        // Check if any types need their collision rendered
        
        render::EndNormalRender();
        eng_End();
    }

    RenderSpecialObjects();
}
