#pragma once

#include "VectorMath.h"
#include "Guid.h"
#include "PropertyDefs.h"
#include "Helpers.h"
#include <cfloat>

#include <algorithm>
#include <cassert>
#include <vector>
#include <string>

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

// This class is use to read/write properties for an object class.
class prop_query
{
public:
    prop_query();
    PropertyType GetQueryType() const;

    prop_query& RQuery(prop_container& Container);
    prop_query& RQuery(const char* pName, prop_container& Container);
    prop_query& RQueryFloat(const char* pName, float& Data);
    prop_query& RQueryInt(const char* pName, int& Data);
    prop_query& RQueryBool(const char* pName, bool& Data);
    prop_query& RQueryGUID(const char* pName, guid& Data);
    prop_query& RQueryVector2(const char* pName, Vector2& Data);
    prop_query& RQueryVector3(const char* pName, Vector3& Data);
    prop_query& RQueryBBox(const char* pName, BBox& Data);
    prop_query& RQueryAngle(const char* pName, Radian& Data);
    prop_query& RQueryRotation(const char* pName, Radian3& Data);
    prop_query& RQueryColor(const char* pName, Colour& Data);
    prop_query& RQueryString(const char* pName, char* Data);
    prop_query& RQueryEnum(const char* pName, char* Data);
    prop_query& RQueryExternal(const char* pName, char* Data);
    prop_query& RQueryFileName(const char* pName, char* Data);
    prop_query& RQueryButton(const char* pName, char* Data);

    prop_query& WQuery(const prop_container& Container);
    prop_query& WQueryFloat(const char* pName, const float& Data);
    prop_query& WQueryInt(const char* pName, const int& Data);
    prop_query& WQueryBool(const char* pName, const bool& Data);
    prop_query& WQueryGUID(const char* pName, const guid& Data);
    prop_query& WQueryVector2(const char* pName, const Vector2& Data);
    prop_query& WQueryVector3(const char* pName, const Vector3& Data);
    prop_query& WQueryBBox(const char* pName, const BBox& Data);
    prop_query& WQueryAngle(const char* pName, const Radian& Data);
    prop_query& WQueryRotation(const char* pName, const Radian3& Data);
    prop_query& WQueryColor(const char* pName, const Colour& Data);
    prop_query& WQueryString(const char* pName, const char* pData);
    prop_query& WQueryEnum(const char* pName, const char* pData);
    prop_query& WQueryExternal(const char* pName, const char* pData);
    prop_query& WQueryFileName(const char* pName, const char* pData);
    prop_query& WQueryButton(const char* pName, const char* pData);

    //=========================================================================

    template <class T>
    inline void GenericQuery(bool bRead, PropertyType Type, const char* pName, T& Data)
    {
        ParseString(pName);
        m_bRead = bRead;
        m_Type = (uint32_t)Type;
        m_pData = (void*)&Data;
        m_DataSize = sizeof(T);
        m_RootPath[0] = 0;
        m_RootLength = 0;
    }

    int  PushPath(const char* pRoot);
    void PopPath(int iPath);

    // Generic Getters and setters
    bool VarFloat(const char* pPropName, float& Data, float MinVal = -FLT_MAX, float MaxVal = FLT_MAX);
    bool VarBool(const char* pPropName, bool& Data);
    bool VarInt(const char* pPropName, int& Data, int MinVal = INT_MIN, int MaxVal = INT_MAX);
    bool VarGUID(const char* pPropName, guid& Data);
    bool VarVector2(const char* pPropName, Vector2& Data);
    bool VarVector3(const char* pPropName, Vector3& Data);
    bool VarBBox(const char* pPropName, BBox& Data);
    bool VarAngle(const char* pPropName, Radian& Data, Radian MinVal = -FLT_MAX, Radian MaxVal = FLT_MAX);
    bool VarRotation(const char* pPropName, Radian3& Data);
    bool VarColor(const char* pPropName, Colour& Data);
    bool VarString(const char* pPropName, char* Data, int MaxStrLen);
    bool VarEnum(const char* pPropName, char* Data);
    bool VarExternal(const char* pPropName, char* Data, int MaxStrLen);
    bool VarFileName(const char* pPropName, char* Data, int MaxStrLen);
    bool VarButton(const char* pPropName, char* Data);

    // state
    bool IsVar(const char* pString);

    // Does Path contain pPath?
    bool IsSimilarPath(const char* pPath);

    // Does Path begin with pPath?
    bool IsBasePath(const char* pPath);

    bool IsEditor(void);
    bool IsRead(void);
    int  GetIndex(int Number);
    int  GetIndexCount(void) { return m_nIndices; }

    const char* GetName(void) { return m_PropName; }

    // Getters
    float       GetVarFloat(float MinVal = -FLT_MAX, float MaxVal = FLT_MAX);
    int         GetVarInt(int MinVal = INT_MIN, int MaxVal = INT_MAX);
    bool        GetVarBool(void);
    guid        GetVarGUID(void);
    Vector2     GetVarVector2(void);
    Vector3     GetVarVector3(void);
    BBox        GetVarBBox(void);
    Radian      GetVarAngle(Radian MinVal = -FLT_MAX, Radian MaxVal = FLT_MAX);
    Radian3     GetVarRotation(void);
    Colour      GetVarColor(void);
    const char* GetVarFileName(void);
    const char* GetVarString(void);
    const char* GetVarEnum(void);
    const char* GetVarExternal(void);

    // Setters
    void SetVarFloat(float Data);
    void SetVarInt(int Data);
    void SetVarBool(bool Data);
    void SetVarGUID(guid Data);
    void SetVarVector2(const Vector2& Data);
    void SetVarVector3(const Vector3& Data);
    void SetVarBBox(const BBox& Data);
    void SetVarAngle(Radian Data);
    void SetVarRotation(const Radian3& Data);
    void SetVarColor(Colour Data);
    void SetVarFileName(const char* Data, int MaxStrLen);
    void SetVarString(const char* Data, int MaxStrLen);
    void SetVarEnum(const char* Data);
    void SetVarExternal(const char* Data, int MaxStrLen);
    void SetVarButton(const char* Data);

    // Get ranges
    float GetMinFloat(void) const { return m_MinFloat; }
    float GetMaxFloat(void) const { return m_MaxFloat; }

    int GetMinInt(void) const { return m_MinInt; }
    int GetMaxInt(void) const { return m_MaxInt; }

    int GetMaxStringLength(void) const { return m_MaxStringLength; }

protected:
    PropertyType ClearType(uint32_t Type) const;
    PropertyType ClearType(PropertyType Type) const;

    void RStringQuery(PropertyType Type, const char* pName, char* pData)
    {
        ParseString(pName);
        m_bRead = true;
        m_Type = (uint32_t)Type;
        m_DataSize = 0;
        m_pData = pData;
        m_RootPath[0] = 0;
        m_RootLength = 0;
    }

    void WStringQuery(PropertyType Type, const char* pName, const char* pData)
    {
        ParseString(pName);
        m_RootPath[0] = 0;
        m_RootLength = 0;
        m_bRead = false;
        m_Type = (uint32_t)Type;
        m_DataSize = 0;
        m_pData = (void*)pData;
        m_RootPath[0] = 0;
        m_RootLength = 0;
    }

    template <class T>
    inline void GenericVar(PropertyType Type, T& Data)
    {
        assert(ClearType(Type) == GetQueryType());
        assert(m_DataSize == sizeof(T));
        (void)Type;
        if (m_bRead) {
            *((T*)m_pData) = Data;
        } else {
            Data = *((T*)m_pData);
        }
    }

    template <class T>
    inline void GenericVar(PropertyType Type, T& Data, T Min, T Max)
    {
        GenericVar(Type, Data);
        Data = std::max(Data, Min);
        Data = std::min(Data, Max);
    }

    template <class T>
    inline void GetGenericVar(PropertyType Type, T& Data) const
    {
        assert(ClearType(Type) == GetQueryType());
        assert(m_DataSize == sizeof(T));
        assert(m_bRead == false);
        (void)Type;
        Data = *((T*)m_pData);
    }

    template <class T>
    inline void GetGenericVar(PropertyType Type, T& Data, T Min, T Max) const
    {
        GetGenericVar(Type, Data);
        Data = std::max(Data, Min);
        Data = std::min(Data, Max);
    }

    template <class T>
    inline void SetGenericVar(PropertyType Type, const T& Data)
    {
        assert(ClearType(Type) == GetQueryType());
        assert(m_DataSize == sizeof(T));
        assert(m_bRead);
        (void)Type;
        *((T*)m_pData) = Data;
    }

    inline void GenericVarString(char* pData, int MaxStrLen)
    {
        assert(m_Type & PROP_TYPE_STRING);

        if (m_bRead) {
            strsavecpy(((char*)m_pData), pData, MaxStrLen);
        } else {
            strsavecpy(pData, ((char*)m_pData), MaxStrLen);
        }

        // Set an indicater on how long a string can be
        m_MaxStringLength = MaxStrLen;
    }

    void ParseString(const char* pString);

    bool m_bRead;
    int  m_PropNameLen;
    char m_PropName[128];
    char m_RootPath[128];
    int  m_RootLength;

    int      m_nIndices;
    int      m_Index[16];
    uint32_t m_Type;
    int      m_DataSize;
    void*    m_pData;

    int   m_MaxStringLength;
    int   m_MaxInt;
    int   m_MinInt;
    float m_MinFloat;
    float m_MaxFloat;
};

//=========================================================================
// prop_enum
//-------------------------------------------------------------------------
// This class is use to enumerate properties.
//=========================================================================
class prop_enum
{
public:
    class node
    {
    public:
        node();
        void Set(const char* pName, const char* pComment = "", uint32_t type = PROP_TYPE_NULL);

        const char* GetName() const;
        uint32_t    GetType() const;
        int         GetEnumCount() const;
        const char* GetEnumType(int Index) const;
        const char* GetComment() const;
        void        SetFlags(uint32_t Flags);

    protected:
        char        m_Name[128];
        std::string m_String;
        uint32_t    m_Type;
        const char* m_pComment;

        friend class prop_enum;
    };
    prop_enum();
    virtual ~prop_enum() {};

#define PropEnumHeader(a, b, c) _PropEnumHeader(a, c)
#define PropEnumBool(a, b, c) _PropEnumBool(a, c)
#define PropEnumInt(a, b, c) _PropEnumInt(a, c)
#define PropEnumFloat(a, b, c) _PropEnumFloat(a, c)
#define PropEnumVector2(a, b, c) _PropEnumVector2(a, c)
#define PropEnumVector3(a, b, c) _PropEnumVector3(a, c)
#define PropEnumRotation(a, b, c) _PropEnumRotation(a, c)
#define PropEnumAngle(a, b, c) _PropEnumAngle(a, c)
#define PropEnumBBox(a, b, c) _PropEnumBBox(a, c)
#define PropEnumGuid(a, b, c) _PropEnumGuid(a, c)
#define PropEnumColor(a, b, c) _PropEnumColor(a, c)
#define PropEnumString(a, b, c) _PropEnumString(a, c)
#define PropEnumEnum(a, b, c, d) _PropEnumEnum(a, b, d)
#define PropEnumButton(a, b, c) _PropEnumButton(a, c)
#define PropEnumFileName(a, b, c, d) _PropEnumFileName(a, b, d)
#define PropEnumExternal(a, b, c, d) _PropEnumExternal(a, b, d)

    virtual void _PropEnumHeader(const char* pName, uint32_t Flags);
    virtual void _PropEnumBool(const char* pName, uint32_t Flags);
    virtual void _PropEnumInt(const char* pName, uint32_t Flags);
    virtual void _PropEnumFloat(const char* pName, uint32_t Flags);
    virtual void _PropEnumVector2(const char* pName, uint32_t Flags);
    virtual void _PropEnumVector3(const char* pName, uint32_t Flags);
    virtual void _PropEnumRotation(const char* pName, uint32_t Flags);
    virtual void _PropEnumAngle(const char* pName, uint32_t Flags);
    virtual void _PropEnumBBox(const char* pName, uint32_t Flags);
    virtual void _PropEnumGuid(const char* pName, uint32_t Flags);
    virtual void _PropEnumColor(const char* pName, uint32_t Flags);
    virtual void _PropEnumString(const char* pName, uint32_t Flags);
    virtual void _PropEnumEnum(const char* pName, const char* pEnum, uint32_t Flags);
    virtual void _PropEnumButton(const char* pName, uint32_t Flags);
    virtual void _PropEnumFileName(const char* pName, const char* pExt, uint32_t Flags);
    virtual void _PropEnumExternal(const char* pName, const char* TypeInfo, uint32_t Flags);

    int   GetCount();
    node& operator[](int Index);
    void  Clear();

    int  PushPath(const char* pPath);
    void PopPath(int iPath);
    //void            SetCapacity     ( int Capacity ) { m_lList.SetCapacity( Capacity ); }

protected:
    const char* GetRootPath() { return m_RootPath; }

protected:
    std::vector<node> m_lList;
    char              m_RootPath[128];
    int               m_iRootPath;
};

class prop_enum_counter : public prop_enum
{
public:
    prop_enum_counter(void) { m_Count = 0; }
    virtual ~prop_enum_counter(void) {}
    int GetCount(void) { return m_Count; }

    virtual void _PropEnumHeader(const char* pName, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumBool(const char* pName, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumInt(const char* pName, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumFloat(const char* pName, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumVector2(const char* pName, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumVector3(const char* pName, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumRotation(const char* pName, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumAngle(const char* pName, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumBBox(const char* pName, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumGuid(const char* pName, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumColor(const char* pName, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumString(const char* pName, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumEnum(const char* pName, const char* pEnum, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumButton(const char* pName, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumFileName(const char* pName, const char* pExt, uint32_t Flags)
    {
        m_Count++;
    }
    virtual void _PropEnumExternal(const char* pName, const char* TypeInfo, uint32_t Flags)
    {
        m_Count++;
    }

protected:
    int m_Count;
};

// Keeps a copy of a property.
class prop_container
{
public:
    prop_container();

    //void InitPropEnum(const prop_enum::node& Enum);
    void InitFloat(const char* pName, const float& Data);
    void InitInt(const char* pName, const int& Data);
    void InitBool(const char* pName, const bool& Data);
    void InitGUID(const char* pName, const guid& Data);
    void InitVector2(const char* pName, const Vector2& Data);
    void InitVector3(const char* pName, const Vector3& Data);
    void InitBBox(const char* pName, const BBox& Data);
    void InitAngle(const char* pName, const Radian& Data);
    void InitRotation(const char* pName, const Radian3& Data);
    void InitColor(const char* pName, const Colour& Data);
    void InitString(const char* pName, const char* Data);
    void InitEnum(const char* pName, const char* Data);
    void InitExternal(const char* pName, const char* Data);
    void InitFileName(const char* pName, const char* Data);
    void InitButton(const char* pName, const char* Data);
    void InitGeneric(const char* pName, PropertyType Type, const void* pData);

    void GetFloat(float& Data) const;
    void GetInt(int& Data) const;
    void GetBool(bool& Data) const;
    void GetGUID(guid& Data) const;
    void GetVector2(Vector2& Data) const;
    void GetVector3(Vector3& Data) const;
    void GetBBox(BBox& Data) const;
    void GetAngle(Radian& Data) const;
    void GetRotation(Radian3& Data) const;
    void GetColor(Colour& Data) const;
    void GetString(char* Data) const;
    void GetEnum(char* Data) const;
    void GetExternal(char* Data) const;
    void GetGeneric(void* Data) const;
    void GetFileName(char* Data) const;
    void GetButton(char* Data) const;

    void SetFloat(const float& Data);
    void SetInt(const int& Data);
    void SetBool(const bool& Data);
    void SetGUID(const guid& Data);
    void SetVector2(const Vector2& Data);
    void SetVector3(const Vector3& Data);
    void SetBBox(const BBox& Data);
    void SetAngle(const Radian& Data);
    void SetRotation(const Radian3& Data);
    void SetColor(const Colour& Data);
    void SetString(const char* pString);
    void SetEnum(const char* pString);
    void SetExternal(const char* pString);
    void SetGeneric(const void* Data);
    void SetFileName(const char* Data);
    void SetButton(const char* Data);

    const char* GetName() const { return m_Name; }
    uint32_t    GetType() const { return m_Type; }
    uint32_t    GetTypeFlags() const { return m_Type & PROP_TYPE_BASIC_MASK; }
    int         GetDataSize() const;
    void*       GetRawData() const { return (void*)m_Data; }
    void*       GetRawData() { return m_Data; }
    void        SetName(const char* pName) { strcpy(m_Name, pName); }

protected:
    char     m_Name[128];
    uint32_t m_Type;
    char     m_Data[256];
};
