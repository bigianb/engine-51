
#pragma once

//=========================================================================
// This file reads text file descrive as falls:
//
// [ HeaderName : HeaderCount ]
// { FieldName:ddd  FieldName:fff FieldName:sss FieldName:fds }
//      1 2 3       1.0 2.0 3.0   "1" "2" "3"    1.0 2 "3"
// ...
//
// or also can look like this
//
// [ HeaderName ]
// { FieldName:ddd  FieldName:fff FieldName:sss FieldName:fds }
//      1 2 3       1.0 2.0 3.0   "1" "2" "3"    1.0 2 "3"
// ...
//
//=========================================================================

#include "Tokenizer.h"
#include <cstdint>
#include "../Colour.h"
#include "../Guid.h"
#include "../VectorMath.h"

class text_in
{
public:
    text_in();
    ~text_in(); // do not virtualise

    void OpenText(const char* pText);

    bool ReadHeader();
    bool SkipToNextHeader();

    bool ReadFields();
    bool GetField(const char* FieldName, ...);
    bool GetVector3(const char* pName, Vector3& V);
    bool GetColor(const char* pName, Colour& C);
    bool GetF32(const char* pName, float& F);
    bool GetS32(const char* pName, int& I);
    bool GetString(const char* pName, char* pStr);
    bool GetBBox(const char* pName, BBox& bbox);
    bool GetRadian3(const char* pName, Radian3& Orient);
    bool GetQuaternion(const char* pName, Quaternion& Q);
    bool GetBool(const char* pName, bool& Bool);
    bool GetGuid(const char* pName, guid& Guid);

    inline const char* GetHeaderName() const { return m_Record.Name; }
    inline int         GetHeaderCount() const { return m_Record.Count; }
    inline const char* GetError() const { return s_Error; }
    inline bool        IsEOF() const { return m_Tokenizer.IsEOF(); }
    inline const char* GetFileName() { return m_Tokenizer.GetFilename(); }

    inline int         GetFieldCount() const { return m_Record.nFields; }
    inline const char* GetFieldName(int nIndex) const { return m_Record.Field[nIndex].Name; }
    inline int         GetFieldTypeCount(int nIndex) const { return m_Record.Field[nIndex].nTypes; }
    void               GetFieldTypeStr(int nField, char* pString);
    bool               GetFieldAsString(const char* FieldName, char** pOutStr);

protected:
    enum type
    {
        TYPE_NULL,
        TYPE_FLOAT,
        TYPE_INTEGER,
        TYPE_STRING,
        TYPE_GUID
    };

#define TEXTFILE_MAX_FIELDS 32     // a:f b:f c:f d:f e:f f:f g:f h:f ...
#define TEXTFILE_MAX_TYPES 16      // MaxTypes:ffffffffffffffff
#define TEXTFILE_MAX_STRLENGTH 256 // "dssdfsdfsdsdfdsfsdfsfd ..."
#define TEXTFILE_MAX_FLOATS 16
#define TEXTFILE_MAX_INTS 16
#define TEXTFILE_MAX_STRINGS 4
#define TEXTFILE_MAX_GUIDS 1

    struct data
    {
        int FieldID;

        int nFloats;
        int nIntegers;
        int nStrings;
        int nGuids;

        float Float[TEXTFILE_MAX_FLOATS];
        int   Integer[TEXTFILE_MAX_INTS];
        char  String[TEXTFILE_MAX_STRINGS][TEXTFILE_MAX_STRLENGTH];
        guid  Guid[TEXTFILE_MAX_GUIDS];
    };

    struct field
    {
        char Name[TEXTFILE_MAX_STRLENGTH];
        int  ID;
        int  nTypes;
        type Type[TEXTFILE_MAX_TYPES];
    };

    struct record
    {
        char Name[TEXTFILE_MAX_STRLENGTH];
        int  Count;

        int    nFields;
        int    nFieldsAllocated;
        field* Field;
    };

    struct field_match
    {
        const char* UserString;
        int         Index;
    };

protected:
    bool ReadAllFields();
    bool ReportError(const char* pStr, ...);
    int  Stricmp(const char* pStr1, const char* pStr2, int Count);

protected:
    record       m_Record;
    token_stream m_Tokenizer;
    int          m_nValidFields;

    int          m_nFieldMatches;
    int          m_nFieldMatchesAllocated;
    field_match* m_FieldMatch;

    int m_RecordLineNumber;
    int m_RecordTypeNumber;

    data* m_pData;
    int   m_nDataEntriesAllocated;

    static char s_Error[512];

protected:
};
