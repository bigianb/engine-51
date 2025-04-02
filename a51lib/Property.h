#pragma once

#include <vector>

class text_in;
class prop_enum;
class prop_container;
class prop_query;

class prop_interface
{
public:
    virtual ~prop_interface() {}

    virtual void OnEnumProp(prop_enum& List) = 0;
    virtual bool OnProperty(prop_query& I) = 0;

    virtual void OnLoad(text_in& TextIn);
    virtual void OnLoad(const char* FileName);
    virtual void OnCopy(std::vector<prop_container>& Container);
    virtual void OnPaste(const std::vector<prop_container>& Container);
};
