#include "Property.h"

#include <vector>
#include <cassert>

extern const char* const  k_EnumEndStringConst    = "LAST_ENUM_STRING_90210";
extern const int    k_MaxEnumInTable        = 10000;
char         g_EnumStringOut[255]    = {0};

prop_query              g_PropQuery;
std::vector<prop_container>  g_PropContainer;
prop_enum               g_PropEnum;

void prop_interface::OnLoad(text_in& TextIn)
{
    // TODO: implement me
    assert(false);
}

void prop_interface::OnLoad(const char* FileName)
{
    // TODO: implement me
    assert(false);
}

void prop_interface::OnCopy(std::vector<prop_container>& Container)
{
    // TODO: implement me
    assert(false);
}

void prop_interface::OnPaste(const std::vector<prop_container>& Container)
{
    // TODO: implement me
    assert(false);
}

prop_query::prop_query(void)
{
    m_pData = NULL;
    m_RootPath[0] = 0;
    m_RootLength = 0;
}

PropertyType prop_query::GetQueryType(void) const
{
    return ClearType(m_Type);
}
PropertyType prop_query::ClearType(unsigned int Type) const
{
    return (PropertyType)(Type & PROP_TYPE_BASIC_MASK);
}
PropertyType prop_query::ClearType(PropertyType Type) const
{
    return (PropertyType)(((unsigned int)Type) & PROP_TYPE_BASIC_MASK);
}

prop_query& prop_query::RQueryFloat(const char* pName, float& Data)
{
    GenericQuery(true, PROP_TYPE_FLOAT, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryInt(const char* pName, int& Data)
{
    GenericQuery(true, PROP_TYPE_INT, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryBool(const char* pName, bool& Data)
{
    GenericQuery(true, PROP_TYPE_BOOL, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryGUID(const char* pName, guid& Data)
{
    GenericQuery(true, PROP_TYPE_GUID, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryVector2(const char* pName, Vector2& Data)
{
    GenericQuery(true, PROP_TYPE_VECTOR2, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryVector3(const char* pName, Vector3& Data)
{
    GenericQuery(true, PROP_TYPE_VECTOR3, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryBBox(const char* pName, BBox& Data)
{
    GenericQuery(true, PROP_TYPE_BBOX, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryAngle(const char* pName, Radian& Data)
{
    GenericQuery(true, PROP_TYPE_ANGLE, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryRotation(const char* pName, Radian3& Data)
{
    GenericQuery(true, PROP_TYPE_ROTATION, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryColor(const char* pName, Colour& Data)
{
    GenericQuery(true, PROP_TYPE_COLOR, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryString(const char* pName, char* Data)
{
    RStringQuery(PROP_TYPE_STRING, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryEnum(const char* pName, char* Data)
{
    RStringQuery(PROP_TYPE_ENUM, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryExternal(const char* pName, char* Data)
{
    RStringQuery(PROP_TYPE_EXTERNAL, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryFileName(const char* pName, char* Data)
{
    RStringQuery(PROP_TYPE_FILENAME, pName, Data);
    return *this;
}
prop_query& prop_query::RQueryButton(const char* pName, char* Data)
{
    RStringQuery(PROP_TYPE_BUTTON, pName, Data);
    return *this;
}

prop_query& prop_query::WQueryFloat(const char* pName, const float& Data)
{
    GenericQuery(false, PROP_TYPE_FLOAT, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryInt(const char* pName, const int& Data)
{
    GenericQuery(false, PROP_TYPE_INT, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryBool(const char* pName, const bool& Data)
{
    GenericQuery(false, PROP_TYPE_BOOL, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryGUID(const char* pName, const guid& Data)
{
    GenericQuery(false, PROP_TYPE_GUID, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryVector2(const char* pName, const Vector2& Data)
{
    GenericQuery(false, PROP_TYPE_VECTOR2, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryVector3(const char* pName, const Vector3& Data)
{
    GenericQuery(false, PROP_TYPE_VECTOR3, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryBBox(const char* pName, const BBox& Data)
{
    GenericQuery(false, PROP_TYPE_BBOX, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryAngle(const char* pName, const Radian& Data)
{
    GenericQuery(false, PROP_TYPE_ANGLE, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryRotation(const char* pName, const Radian3& Data)
{
    GenericQuery(false, PROP_TYPE_ROTATION, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryColor(const char* pName, const Colour& Data)
{
    GenericQuery(false, PROP_TYPE_COLOR, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryString(const char* pName, const char* Data)
{
    WStringQuery(PROP_TYPE_STRING, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryEnum(const char* pName, const char* Data)
{
    WStringQuery(PROP_TYPE_ENUM, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryExternal(const char* pName, const char* Data)
{
    WStringQuery(PROP_TYPE_EXTERNAL, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryFileName(const char* pName, const char* Data)
{
    WStringQuery(PROP_TYPE_FILENAME, pName, Data);
    return *this;
}
prop_query& prop_query::WQueryButton(const char* pName, const char* Data)
{
    WStringQuery(PROP_TYPE_BUTTON, pName, Data);
    return *this;
}

bool prop_query::VarFloat(const char* pPropName, float& Data, float Min, float Max)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVar(PROP_TYPE_FLOAT, Data, Min, Max);
    m_MinFloat = Min;
    m_MaxFloat = Max;
    return true;
}
bool prop_query::VarInt(const char* pPropName, int& Data, int Min, int Max)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVar(PROP_TYPE_INT, Data, Min, Max);
    m_MinInt = Min;
    m_MaxInt = Max;
    return true;
}
bool prop_query::VarAngle(const char* pPropName, Radian& Data, Radian Min, Radian Max)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVar(PROP_TYPE_ANGLE, Data, Min, Max);
    m_MinFloat = Min;
    m_MaxFloat = Max;
    return true;
}
bool prop_query::VarBool(const char* pPropName, bool& Data)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVar(PROP_TYPE_BOOL, Data);
    return true;
}
bool prop_query::VarGUID(const char* pPropName, guid& Data)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVar(PROP_TYPE_GUID, Data);
    return true;
}
bool prop_query::VarVector2(const char* pPropName, Vector2& Data)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVar(PROP_TYPE_VECTOR2, Data);
    return true;
}
bool prop_query::VarVector3(const char* pPropName, Vector3& Data)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVar(PROP_TYPE_VECTOR3, Data);
    return true;
}
bool prop_query::VarBBox(const char* pPropName, BBox& Data)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVar(PROP_TYPE_BBOX, Data);
    return true;
}
bool prop_query::VarRotation(const char* pPropName, Radian3& Data)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVar(PROP_TYPE_ROTATION, Data);
    return true;
}
bool prop_query::VarColor(const char* pPropName, Colour& Data)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVar(PROP_TYPE_COLOR, Data);
    return true;
}
bool prop_query::VarString(const char* pPropName, char* Data, int MaxStrLen)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVarString(Data, MaxStrLen);
    return true;
}
bool prop_query::VarEnum(const char* pPropName, char* Data)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVarString(Data, 256);
    return true;
}
bool prop_query::VarExternal(const char* pPropName, char* Data, int MaxStrLen)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVarString(Data, MaxStrLen);
    return true;
}
bool prop_query::VarFileName(const char* pPropName, char* Data, int MaxStrLen)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVarString(Data, MaxStrLen);
    return true;
}
bool prop_query::VarButton(const char* pPropName, char* Data)
{
    if (IsVar(pPropName) == false) {
        return false;
    }
    GenericVarString(Data, 256);
    return true;
}

float prop_query::GetVarFloat(float MinVal, float MaxVal)
{
    float Data;
    GetGenericVar(PROP_TYPE_FLOAT, Data, MinVal, MaxVal);
    m_MinFloat = MinVal;
    m_MaxFloat = MaxVal;
    return Data;
}
int prop_query::GetVarInt(int MinVal, int MaxVal)
{
    int Data;
    GetGenericVar(PROP_TYPE_INT, Data, MinVal, MaxVal);
    m_MinInt = MinVal;
    m_MaxInt = MaxVal;
    return Data;
}
Radian prop_query::GetVarAngle(Radian MinVal, Radian MaxVal)
{
    Radian Data;
    GetGenericVar(PROP_TYPE_ANGLE, Data, MinVal, MaxVal);
    m_MinFloat = MinVal;
    m_MaxFloat = MaxVal;
    return Data;
}
bool prop_query::GetVarBool(void)
{
    bool Data;
    GetGenericVar(PROP_TYPE_BOOL, Data);
    return Data;
}
guid prop_query::GetVarGUID(void)
{
    guid Data;
    GetGenericVar(PROP_TYPE_GUID, Data);
    return Data;
}
Vector2 prop_query::GetVarVector2(void)
{
    Vector2 Data;
    GetGenericVar(PROP_TYPE_VECTOR2, Data);
    return Data;
}
Vector3 prop_query::GetVarVector3(void)
{
    Vector3 Data;
    GetGenericVar(PROP_TYPE_VECTOR3, Data);
    return Data;
}
BBox prop_query::GetVarBBox(void)
{
    BBox Data;
    GetGenericVar(PROP_TYPE_BBOX, Data);
    return Data;
}
Radian3 prop_query::GetVarRotation(void)
{
    Radian3 Data;
    GetGenericVar(PROP_TYPE_ROTATION, Data);
    return Data;
}
Colour prop_query::GetVarColor(void)
{
    Colour Data;
    GetGenericVar(PROP_TYPE_COLOR, Data);
    return Data;
}
const char* prop_query::GetVarFileName(void)
{
    assert(PROP_TYPE_FILENAME == GetQueryType());
    return (char*)m_pData;
}
const char* prop_query::GetVarString(void)
{
    assert(PROP_TYPE_STRING == GetQueryType());
    return (char*)m_pData;
}
const char* prop_query::GetVarEnum(void)
{
    assert(PROP_TYPE_ENUM == GetQueryType());
    return (char*)m_pData;
}
const char* prop_query::GetVarExternal(void)
{
    assert(PROP_TYPE_EXTERNAL == GetQueryType());
    return (char*)m_pData;
}

void prop_query::SetVarFloat(float Data)
{
    SetGenericVar(PROP_TYPE_FLOAT, Data);
}
void prop_query::SetVarInt(int Data)
{
    SetGenericVar(PROP_TYPE_INT, Data);
}
void prop_query::SetVarBool(bool Data)
{
    SetGenericVar(PROP_TYPE_BOOL, Data);
}
void prop_query::SetVarGUID(guid Data)
{
    SetGenericVar(PROP_TYPE_GUID, Data);
}
void prop_query::SetVarVector2(const Vector2& Data)
{
    SetGenericVar(PROP_TYPE_VECTOR2, Data);
}
void prop_query::SetVarVector3(const Vector3& Data)
{
    SetGenericVar(PROP_TYPE_VECTOR3, Data);
}
void prop_query::SetVarBBox(const BBox& Data)
{
    SetGenericVar(PROP_TYPE_BBOX, Data);
}
void prop_query::SetVarAngle(Radian Data)
{
    SetGenericVar(PROP_TYPE_ANGLE, Data);
}
void prop_query::SetVarRotation(const Radian3& Data)
{
    SetGenericVar(PROP_TYPE_ROTATION, Data);
}
void prop_query::SetVarColor(Colour Data)
{
    SetGenericVar(PROP_TYPE_COLOR, Data);
}
void prop_query::SetVarFileName(const char* pData, int MaxStrLen)
{
    assert(PROP_TYPE_FILENAME == GetQueryType());
    strsavecpy((char*)m_pData, pData, MaxStrLen);
    m_MaxStringLength = MaxStrLen;
}
void prop_query::SetVarString(const char* pData, int MaxStrLen)
{
    assert(PROP_TYPE_STRING == GetQueryType());
    strsavecpy((char*)m_pData, pData, MaxStrLen);
    m_MaxStringLength = MaxStrLen;
}
void prop_query::SetVarButton(const char* pData)
{
    assert(PROP_TYPE_BUTTON == GetQueryType());
    strsavecpy((char*)m_pData, pData, 256);
    m_MaxStringLength = 256;
}
void prop_query::SetVarEnum(const char* pData)
{
    assert(PROP_TYPE_ENUM == GetQueryType());
    strsavecpy((char*)m_pData, pData, 256);
    m_MaxStringLength = 256;
}
void prop_query::SetVarExternal(const char* pData, int MaxStrLen)
{
    assert(PROP_TYPE_EXTERNAL == GetQueryType());
    strsavecpy((char*)m_pData, pData, MaxStrLen);
    m_MaxStringLength = MaxStrLen;
}

int prop_query::PushPath(const char* pRootPath)
{
    int i;
    int OldID = m_RootLength;

    for (i = 0; pRootPath[i]; i++) {
        // Can we push it or not?
        // TODO: We can only push a path if this property also has that path
        //       so for right now we will fail to push it but restoring the length.
        //       In reality should return a -1 or something like that.
        if (m_PropName[m_RootLength + i] != pRootPath[i]) {
            m_RootLength = OldID;
            return OldID;
        }

        m_RootPath[m_RootLength + i] = pRootPath[i];
    }

    assert(i < 128);
    m_RootLength += i;
    m_RootPath[m_RootLength] = 0;

    return OldID;
}

//=========================================================================

void prop_query::PopPath(int iPath)
{
    m_RootLength = iPath;
    m_RootPath[m_RootLength] = 0;
}

//=========================================================================

bool prop_query::IsVar(const char* pString)
{
    // Super fast string compare
    for (int i = m_PropNameLen; i >= m_RootLength; --i) {
        if (pString[i - m_RootLength] != m_PropName[i]) {
            return false;
        }
    }

    return true;
}

//=========================================================================

bool prop_query::IsSimilarPath(const char* pPath)
{
    return stristr(&m_PropName[m_RootLength], pPath) != NULL;
}

//=========================================================================

bool prop_query::IsBasePath(const char* pPath)
{
    const char* pStr1 = &m_PropName[m_RootLength];
    const char* pStr2 = pPath;
    assert(pStr1 && pStr2);

    int C1, C2;

    do {
        C1 = (int)(*(pStr1++));
        if ((C1 >= 'A') && (C1 <= 'Z')) {
            C1 -= ('A' - 'a');
        }

        C2 = (int)(*(pStr2++));
        if ((C2 >= 'A') && (C2 <= 'Z')) {
            C2 -= ('A' - 'a');
        }

    } while (C1 && C2 && (C1 == C2));

    // If we made it to the end of pStr2 then we are good.
    if (C2 == 0) {
        return true;
    }

    return false;
}

//=========================================================================

bool prop_query::IsRead(void)
{
    return m_bRead;
}

//=========================================================================

int prop_query::GetIndex(int Number)
{
    assert(Number >= 0);
    if (Number >= m_nIndices) {
        return 0;
    }
    return m_Index[Number];
}

//=========================================================================

prop_query& prop_query::RQuery(prop_container& Container)
{
    ParseString(Container.GetName());
    m_bRead = true;
    m_Type = Container.GetType();
    m_DataSize = Container.GetDataSize();
    m_pData = Container.GetRawData();
    m_RootPath[0] = 0;
    m_RootLength = 0;

    return *this;
}

//=========================================================================

prop_query& prop_query::RQuery(const char* pName, prop_container& Container)
{
    Container.SetName(pName);
    return RQuery(Container);
}

//=========================================================================

prop_query& prop_query::WQuery(const prop_container& Container)
{
    ParseString(Container.GetName());
    m_bRead = false;
    m_Type = Container.GetType();
    m_DataSize = Container.GetDataSize();
    m_pData = (void*)Container.GetRawData();
    m_RootPath[0] = 0;
    m_RootLength = 0;

    return *this;
}

void prop_query::ParseString(const char* pString)
{
    int i, j;

    m_nIndices = 0;

    // Find the length of the string
    for (j = i = 0; pString[j]; i++, j++) {
        uint8_t C = pString[j];
        m_PropName[i] = C;

        // Decode array entry. Take the number out as well
        if (C == '[') {
            int Total = 0;

            // Decode the number
            while ((C = pString[++j]) != ']') {
                assert((C >= '0') && (C <= '9'));

                // Accumulate digit.
                Total = (10 * Total) + (C - '0');
            }

            // Roll back j
            j--;

            // Set the number
            assert(m_nIndices < 16);
            assert(i != j); // This usually happens if the string pass has no number in the [12] such []
            m_Index[m_nIndices++] = Total;
        }
    }

    // make the end char
    m_PropName[i] = 0;
    m_PropNameLen = i;
}

int prop_container::GetDataSize() const
{
    switch (m_Type & PROP_TYPE_BASIC_MASK) {
    case PROP_TYPE_FLOAT:
        return sizeof(float);
    case PROP_TYPE_VECTOR2:
        return sizeof(Vector2);
    case PROP_TYPE_VECTOR3:
        return sizeof(Vector3);
    case PROP_TYPE_INT:
        return sizeof(int);
    case PROP_TYPE_BOOL:
        return sizeof(bool);
    case PROP_TYPE_ROTATION:
        return sizeof(Radian3);
    case PROP_TYPE_ANGLE:
        return sizeof(Radian);
    case PROP_TYPE_BBOX:
        return sizeof(BBox);
    case PROP_TYPE_GUID:
        return sizeof(guid);
        //    case PROP_TYPE_TRANSFORM:   return sizeof(matrix4);
    case PROP_TYPE_COLOR:
        return sizeof(Colour);
    case PROP_TYPE_FILENAME:
        return strlen(m_Data);
    case PROP_TYPE_STRING:
        return strlen(m_Data);
    case PROP_TYPE_ENUM:
        return strlen(m_Data);
    case PROP_TYPE_BUTTON:
        return strlen(m_Data);
    case PROP_TYPE_EXTERNAL:
        return strlen(m_Data);
    default:
        assert(0);
        break;
    }

    return 0;
}
