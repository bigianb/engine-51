#pragma once

//=========================================================================
//enum_type is the enumeration type you want to pass in, it is the same type the table uses
//data_type is the data type of the Enum which may or may not be the same as the enum_Type
//=========================================================================

template <class data_type, class enum_type>
bool SMP_UTIL_IsEnumVar(prop_query& rPropQuery, const char* pLabel, data_type& rEnumData, enum_table<enum_type>& rEnumTable)
{
    if (rPropQuery.IsVar(pLabel)) {
        if (rPropQuery.IsRead()) {
            if (rEnumTable.DoesValueExist((enum_type)rEnumData)) {
                rPropQuery.SetVarEnum(rEnumTable.GetString(((enum_type)rEnumData)));
            } else {
                rPropQuery.SetVarEnum("INVALID");
            }
        } else {
            enum_type TmpVal;

            if (rEnumTable.GetValue(rPropQuery.GetVarEnum(), TmpVal)) {
                rEnumData = (data_type)TmpVal;
            }
        }

        return true;
    }
    return false;
}

//=========================================================================
// templated string_type class must be one of the simple_string variants, or
// support the Get, Set and MaxLen functions...
//=========================================================================

template <class string_type>
bool SMP_UTIL_IsExternalVar(prop_query& rPropQuery, const char* pLabel, string_type& rString, bool& IsRead)
{
    IsRead = false;

    if (rPropQuery.IsVar(pLabel)) {
        IsRead = rPropQuery.IsRead();

        if (IsRead) {
            rPropQuery.SetVarExternal(rString.Get(), rString.MaxLen());
        } else {
            if (rPropQuery.GetVarExternal()[0]) {
                rString.Set(rPropQuery.GetVarExternal());
            }
        }

        return true;
    }

    return false;
}
