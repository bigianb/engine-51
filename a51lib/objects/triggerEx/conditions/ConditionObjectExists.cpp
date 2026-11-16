#include "ConditionObjectExists.h"
#include "../../../Property.h"
#include "../../../xfiles/x_plus.h"
#include "../affecters/ConditionalAffecter.h"

condition_object_exists::condition_object_exists(conditional_affecter* pParent)
    : conditional_ex_base(pParent)
    , m_CheckCode(CODE_DOES_OBJECT_EXIST)
{
}

bool condition_object_exists::Execute(guid TriggerGuid)
{
    (void)TriggerGuid;

    Object* pObject = m_Affecter.GetObjectPtr(m_pParent->objectManager);
    switch (m_CheckCode) {
    case CODE_DOES_OBJECT_EXIST:
        if (pObject != nullptr) {
            return true;
        }
        return false;
        break;
    case CODE_DOES_OBJECT_NOT_EXIST:
        if (pObject == nullptr) {
            return true;
        }
        break;
    default:
        assert(0);
        break;
    }

    return false;
}

//=============================================================================

void condition_object_exists::OnEnumProp(prop_enum& rPropList)
{
    //guid specific fields
    m_Affecter.OnEnumProp(rPropList, "Object");

    rPropList.PropEnumInt("CheckCode", "", PROP_TYPE_DONT_SHOW);
    rPropList.PropEnumEnum("Operation", "Does Exist\0Does NOT exist\0", "Are we checking whether the object exists or does not exist.", PROP_TYPE_DONT_SAVE | PROP_TYPE_MUST_ENUM);

    conditional_ex_base::OnEnumProp(rPropList);
}

//=============================================================================

bool condition_object_exists::OnProperty(prop_query& rPropQuery)
{
    //guid specific fields
    if (m_Affecter.OnProperty(rPropQuery, "Object")) {
        return true;
    }

    if (conditional_ex_base::OnProperty(rPropQuery)) {
        return true;
    }

    if (rPropQuery.VarInt("CheckCode", m_CheckCode)) {
        return true;
    }

    if (rPropQuery.IsVar("Operation")) {

        if (rPropQuery.IsRead()) {
            switch (m_CheckCode) {
            case CODE_DOES_OBJECT_EXIST:
                rPropQuery.SetVarEnum("Does Exist");
                break;
            case CODE_DOES_OBJECT_NOT_EXIST:
                rPropQuery.SetVarEnum("Does NOT exist");
                break;
            default:
                assert(0);
                break;
            }

            return (true);
        } else {
            const char* pString = rPropQuery.GetVarEnum();
            if (x_stricmp(pString, "Does Exist") == 0) { m_CheckCode = CODE_DOES_OBJECT_EXIST; }
            if (x_stricmp(pString, "Does NOT exist") == 0) { m_CheckCode = CODE_DOES_OBJECT_NOT_EXIST; }

            return (true);
        }
    }

    return false;
}

std::string condition_object_exists::GetDescription()
{
    switch (m_CheckCode) {
    case CODE_DOES_OBJECT_EXIST:
        return std::string("If ") + m_Affecter.GetObjectInfo(m_pParent->objectManager) + " exists";
    case CODE_DOES_OBJECT_NOT_EXIST:
        return std::string("If ") + m_Affecter.GetObjectInfo(m_pParent->objectManager) + " does not exist";
    default:
        assert(0);
        break;
    }

    return "";
}
