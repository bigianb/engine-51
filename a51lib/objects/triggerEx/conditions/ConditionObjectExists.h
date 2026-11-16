#pragma once

#include "../TriggerExConditionals.h"
#include "../affecters/ObjectAffecter.h"

class condition_object_exists : public conditional_ex_base
{
public:
    condition_object_exists(conditional_affecter* pParent);

    virtual conditional_ex_types GetType() override { return GetTypeStatic(); }
    static conditional_ex_types  GetTypeStatic() { return TYPE_CONDITION_OBJECT_EXISTS; }
    virtual const char*          GetTypeInfo() override { return "Check whether or not a specified object exists."; }
    std::string  GetDescription() override;

    virtual bool Execute(guid TriggerGuid) override;
    virtual void OnEnumProp(prop_enum& rList) override;
    virtual bool OnProperty(prop_query& rPropQuery) override;

protected:
    enum check_codes
    {
        INVALID_CHECK_CODES = -1,
        CODE_DOES_OBJECT_EXIST,
        CODE_DOES_OBJECT_NOT_EXIST,
        CHECK_CODES_END
    };

    int             m_CheckCode; // Used to determine what type of conditional check to perform
    object_affecter m_Affecter;
};
