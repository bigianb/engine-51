#pragma once

#include <string>
#include "../../../Property.h"
#include "../../../xfiles/xHandle.h"

class Object;
class ObjectManager;

class object_affecter
{
public:
    enum guid_codes
    {
        OBJECT_GUID_CODES = -1,
        OBJECT_CODE_CHECK_BY_STATIC_GUID,
        OBJECT_CODE_CHECK_BY_GLOBAL_GUID,
        OBJECT_GUID_CODES_END
    };

    object_affecter();

    virtual void OnEnumProp(prop_enum& rList, const char* pPropName, uint32_t Flags = 0);
    virtual bool OnProperty(prop_query& rPropQuery, const char* pPropName);

    Object*     GetObjectPtr(ObjectManager* objectManager);
    const char* GetObjectInfo(ObjectManager* objectManager);

    inline guid_codes GetObjectCode(void) { return (guid_codes)m_GuidCode; }
    void              SetStaticGuid(guid Guid);

    guid           GetGuid(void);
    inline int     GetGuidVarName(void) { return m_GuidVarName; }
    inline xhandle GetGuidVarHandle(void) { return m_GuidVarHandle; }

protected:
    int               m_GuidVarName;   // Name of the variable
    xhandle           m_GuidVarHandle; // Global variable handle..
    guid              m_ObjectGuid;    // Object ID to see if an object exist...
    int               m_GuidCode;      // Used to determine what type of conditional check to perform
    static std::string m_Description;
};
