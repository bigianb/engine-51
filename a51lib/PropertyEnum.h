#pragma once

#include <vector>
#include <cassert>

extern const char* const k_EnumEndStringConst;
extern const int   k_MaxEnumInTable;
extern char        g_EnumStringOut[255];

template <class T>
class enum_pair
{
public:
    enum_pair(const char* pString, T EnumVal)
    {
        m_EnumVal = EnumVal;
        m_pString = pString;
    }

    T           m_EnumVal;
    const char* m_pString;
};

//=========================================================================
// enum_table : Templated enumeration table useful within the property system..
//  NOTE: The enum_table expects a table of enum_pairs with the last pair
//          be the terminating pair by convention with the String  g_EnumEndStringConst
//          and the EnumVal of the invalid value you wanted return on invalid results.
//=========================================================================

template <class T>
class enum_table
{
public:
    enum_table(enum_pair<T>* pTable);

    const char* BuildString() const;

    bool        GetValue(const char* pString, T& Value) const;
    const char* GetString(T Value) const;

    T&          operator[](int Index);
    T           GetValueFromIndex(int Index) const;
    const char* GetStringFromIndex(int Index) const;

    int  GetCount() const { return m_Count; }
    bool IsEmpty() const { return m_Count > 0; }

    bool DoesValueExist(T Value) const;

    int GetIndex(const char* pString) const;
    int GetIndex(T Value) const;

private:
    enum_pair<T>* m_pTable;
    int           m_Count;
};

//=============================================================================

template <class T>
enum_table<T>::enum_table(enum_pair<T>* pTable)
    : m_pTable(NULL)
    , m_Count(0)
{
    //Careful about the pTable, if it's allocated dynamically make sure its valid
    //for the lifetime of the enum_table...

    //assert( pTable );

    int i = 0;

    while (1) {
        if (i > k_MaxEnumInTable) {
            throw("Enumeration table has exceeded maximum table size limits.\n");
        }

        if (strcmp(pTable[i].m_pString, k_EnumEndStringConst) == 0) {
            i++;
            break;
        }

        i++;
    }

    m_pTable = pTable;
    m_Count = i;
}

//=============================================================================

template <class T>
const char* enum_table<T>::BuildString() const
{
    char* pRawString = g_EnumStringOut;
    int   MaxStrLen = 255;
    int   CurrentIndex = 0;

    for (int i = 0; i < m_Count - 1; i++) {
        if (CurrentIndex >= MaxStrLen) {
            assert(false); //throw("enum_table::BuildString, tried to build enum string greater than buffer capacity.\n");
        }

        strcpy((char*)&pRawString[CurrentIndex], m_pTable[i].m_pString);

        CurrentIndex += strlen(m_pTable[i].m_pString);

        pRawString[CurrentIndex] = 0;

        CurrentIndex++;
    }

    if (CurrentIndex >= MaxStrLen) {
        assert(false); //throw("enum_table::BuildString, tried to build enum string greater than buffer capacity.\n");
    }

    pRawString[CurrentIndex] = 0;

    return pRawString;
}

//=============================================================================

template <class T>
bool enum_table<T>::GetValue(const char* pString, T& Value) const
{
    int Index = GetIndex(pString);

    if (Index >= 0) {
        Value = m_pTable[Index].m_EnumVal;
        return true;
    }

    return false;
}

//=============================================================================

template <class T>
const char* enum_table<T>::GetString(T Value) const
{
    int Index = GetIndex(Value);

    assert(Index >= 0);

    if (Index < 0) {
        return NULL;
    }

    return m_pTable[Index].m_pString;
}

//=============================================================================

template <class T>
T enum_table<T>::GetValueFromIndex(int Index) const
{
    assert(Index >= 0 && Index < m_Count);

    return m_pTable[Index].m_EnumVal;
}

//=============================================================================

template <class T>
T& enum_table<T>::operator[](int Index)
{
    assert(Index >= 0 && Index < m_Count);

    return m_pTable[Index].m_EnumVal;
}

//=============================================================================

template <class T>
const char* enum_table<T>::GetStringFromIndex(int Index) const
{
    assert(Index >= 0 && Index < m_Count);

    return m_pTable[Index].m_pString;
}

//=============================================================================

template <class T>
bool enum_table<T>::DoesValueExist(T Value) const
{
    int Index = GetIndex(Value);

    return Index >= 0;
}

//=============================================================================

template <class T>
int enum_table<T>::GetIndex(const char* pString) const
{
    for (int i = 0; i < m_Count - 1; i++) {
        if (strcmp(pString, m_pTable[i].m_pString) == 0) {
            return i;
        }
    }

    return -1;
}

//=============================================================================

template <class T>
int enum_table<T>::GetIndex(T Value) const
{
    for (int i = 0; i < m_Count - 1; i++) {
        if (Value == m_pTable[i].m_EnumVal) {
            return i;
        }
    }

    return -1;
}

//=========================================================================
// simple_string : Templated simple string class with some common funtionality
//=========================================================================

template <class T>
class enum_list
{
public:
    void Add(const char* pString, T Value);
    void BuildString(char* pString) const;

    T           GetValue(const char* pString) const;
    const char* GetString(T Value) const;

    T&          operator[](int Index) { return (m_lEnum[Index].Value); }
    T           GetValueFromIndex(int Index) const { return (m_lEnum[Index].Value); }
    const char* GetStringFromIndex(int Index) const { return (m_lEnum[Index].pString); }

    int  GetCount() const { return (m_lEnum.GetCount()); }
    bool IsInitialized() const { return (m_lEnum.GetCount() > 0); }

    bool DoesValueExist(T Value) const;

private:
    struct node
    {
        const char* pString;
        T           Value;
    };

    std::vector<node> m_lEnum;
};

//=============================================================================

template <class T>
void enum_list<T>::Add(const char* pString, T Value)
{
    node& Node = m_lEnum.Append();
    Node.pString = pString;
    Node.Value = Value;
}

//=============================================================================

template <class T>
void enum_list<T>::BuildString(char* pString) const
{
    for (int i = 0; i < m_lEnum.GetCount(); i++) {
        node& Node = m_lEnum[i];

        int Len = strlen(Node.pString) + 1;
        memcpy(pString, Node.pString, Len);
        pString += Len;
    }

    pString[0] = '\0';
}

//=============================================================================

template <class T>
T enum_list<T>::GetValue(const char* pString) const
{
    for (int i = 0; i < m_lEnum.GetCount(); i++) {
        node& Node = m_lEnum[i];

        if (strcmp(Node.pString, pString) == 0) {
            return (Node.Value);
        }
    }

    assert(false); //x_throw( xfs( "String [%s] could not be found in enum", pString ) );

    return m_lEnum[0].Value;
}

//=============================================================================

template <class T>
const char* enum_list<T>::GetString(T Value) const
{
    for (int i = 0; i < m_lEnum.GetCount(); i++) {
        node& Node = m_lEnum[i];

        if (Node.Value == Value) {
            return (Node.pString);
        }
    }

    assert(false); //x_throw( xfs( "Value [%d] could not be found in enum", Value ) );
    return nullptr;
}

//=============================================================================

template <class T>
bool enum_list<T>::DoesValueExist(T Value) const
{
    for (int i = 0; i < m_lEnum.GetCount(); i++) {
        node& Node = m_lEnum[i];

        if (Node.Value == Value) {
            return true;
        }
    }

    return false;
}
