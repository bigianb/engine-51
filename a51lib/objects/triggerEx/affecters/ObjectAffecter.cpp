#include "ObjectAffecter.h"
#include "../../../Property.h"
#include "../../../objectManager/ObjectManager.h"
#include "../TriggerExObject.h"
#include "../../../GlobalDictionary.h"
#include "../../../xfiles/xfs.h"
#include "../../../xfiles/x_plus.h"

std::string object_affecter::m_Description;

object_affecter::object_affecter()
    : m_GuidVarName(-1)
    , m_GuidVarHandle(HNULL)
    , m_ObjectGuid(NULL_GUID)
    , m_GuidCode(OBJECT_CODE_CHECK_BY_STATIC_GUID)
{
}

//=============================================================================

void object_affecter::OnEnumProp(prop_enum& rPropList, const char* pPropName, uint32_t Flags)
{
    rPropList.PropEnumInt(xfs("%sGCode", pPropName), "", PROP_TYPE_DONT_SHOW);

    rPropList.PropEnumEnum(xfs("%s Selector", pPropName), "Static Guid\0Variable Guid\0", "Are we checking a set guid or a global variable guid.", PROP_TYPE_DONT_SAVE | PROP_TYPE_MUST_ENUM);

    if (m_GuidCode == OBJECT_CODE_CHECK_BY_STATIC_GUID) {
        rPropList.PropEnumGuid(xfs("%s Guid", pPropName), "The GUID of the object to act upon.", PROP_TYPE_MUST_ENUM | Flags);
    }

    if (m_GuidCode == OBJECT_CODE_CHECK_BY_GLOBAL_GUID) {
        rPropList.PropEnumExternal(xfs("%s Var", pPropName), "global\0global_guid",
                                   "Name of the Global Guid Variable that you wish to affect.", PROP_TYPE_MUST_ENUM | Flags);
    }
}

//=============================================================================

bool object_affecter::OnProperty(prop_query& rPropQuery, const char* pPropName)
{
    if (rPropQuery.VarInt(xfs("%sGCode", pPropName), m_GuidCode)) {
        return true;
    }

    if (rPropQuery.VarGUID(xfs("%s Guid", pPropName), m_ObjectGuid)) {
        if (!rPropQuery.IsRead()) {
            m_GuidCode = OBJECT_CODE_CHECK_BY_STATIC_GUID;
        }
        return true;
    }

    if (rPropQuery.IsVar(xfs("%s Var", pPropName))) {
        if (rPropQuery.IsRead()) {
            if (m_GuidVarName >= 0) {
                rPropQuery.SetVarExternal(g_StringMgr.GetString(m_GuidVarName));
            } else {
                rPropQuery.SetVarExternal("", 256);
            }
            return true;
        } else {
            m_GuidCode = OBJECT_CODE_CHECK_BY_GLOBAL_GUID;
            if (strlen(rPropQuery.GetVarExternal()) > 0) {
                m_GuidVarName = g_StringMgr.Add(rPropQuery.GetVarExternal());
                if (m_GuidVarName == -1) {
                    m_GuidVarHandle = HNULL;
                } else if (!g_VarMgr.GetGuidHandle(g_StringMgr.GetString(m_GuidVarName), &m_GuidVarHandle)) {
                    m_GuidVarHandle = HNULL;
                }
                return true;
            }
        }
    }

    if (rPropQuery.IsVar(xfs("%s Selector", pPropName))) {

        if (rPropQuery.IsRead()) {
            switch (m_GuidCode) {
            case OBJECT_CODE_CHECK_BY_STATIC_GUID:
                rPropQuery.SetVarEnum("Static Guid");
                break;
            case OBJECT_CODE_CHECK_BY_GLOBAL_GUID:
                rPropQuery.SetVarEnum("Variable Guid");
                break;
            default:
                assert(0);
                break;
            }

            return (true);
        } else {
            const char* pString = rPropQuery.GetVarEnum();
            if (x_stricmp(pString, "Static Guid") == 0) { m_GuidCode = OBJECT_CODE_CHECK_BY_STATIC_GUID; }
            if (x_stricmp(pString, "Variable Guid") == 0) { m_GuidCode = OBJECT_CODE_CHECK_BY_GLOBAL_GUID; }

            return (true);
        }
    }

    return false;
}

//=============================================================================

void object_affecter::SetStaticGuid(guid Guid)
{
    m_GuidCode = OBJECT_CODE_CHECK_BY_STATIC_GUID;
    m_ObjectGuid = Guid;
}

//=============================================================================

guid object_affecter::GetGuid(void)
{
    switch (m_GuidCode) {
    case OBJECT_CODE_CHECK_BY_STATIC_GUID:
        return m_ObjectGuid;
        break;
    case OBJECT_CODE_CHECK_BY_GLOBAL_GUID:
        if (m_GuidVarHandle != HNULL) {
            return g_VarMgr.GetGuid(m_GuidVarHandle);
        }
        break;
    default:
        assert(0);
        break;
    }

    return 0;
}

//=============================================================================

Object* object_affecter::GetObjectPtr(ObjectManager* objectManager)
{
    Object* pObject = nullptr;
    guid    ObjectGuid = GetGuid();
    if (ObjectGuid != 0) {
        pObject = objectManager->GetObjectByGuid(ObjectGuid);
    }

    return pObject;
}

//=============================================================================

const char* object_affecter::GetObjectInfo(ObjectManager* objectManager)
{
    m_Description = "???";

    switch (m_GuidCode) {
    case OBJECT_CODE_CHECK_BY_STATIC_GUID:
        if (m_ObjectGuid != 0) {
            Object* pObject = objectManager->GetObjectByGuid(m_ObjectGuid);
            if (pObject) {
                m_Description = pObject->GetTypeDesc().GetTypeName();
            }
        }
        break;
    case OBJECT_CODE_CHECK_BY_GLOBAL_GUID:
        if ((m_GuidVarHandle != HNULL) && (m_GuidVarName != -1)) {
            m_Description = xfs("[%s]", g_StringMgr.GetString(m_GuidVarName).c_str());
        } else {
            m_Description = "Variable Object";
        }
        break;
    default:
        assert(0);
        break;
    }

    return m_Description.c_str();
}
