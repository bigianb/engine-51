
#include "TextIn.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <cassert>

char text_in::s_Error[512];

text_in::text_in()
{
    m_pData = nullptr;
    m_nDataEntriesAllocated = 0;

    m_Record.nFieldsAllocated = 0;
    m_Record.Field = nullptr;

    m_nFieldMatchesAllocated = 0;
    m_FieldMatch = nullptr;
}

//=========================================================================

text_in::~text_in()
{
    delete[] m_pData;
    free(m_Record.Field);
    free(m_FieldMatch);
}

//=========================================================================

bool text_in::ReportError(const char* pStr, ...)
{
    char    Buff[512];
    va_list Args;
    va_start(Args, pStr);

    vsnprintf(Buff, 512, pStr, Args);

    if (m_Record.Name[0]) {
        snprintf(s_Error, 512, "%s(%d) : error : In Record %s, %s\n",
                m_Tokenizer.GetFilename(),
                m_Tokenizer.GetLineNumber(),
                GetHeaderName(),
                Buff);
    } else {
        snprintf(s_Error, 512, "%s(%d) : error : %s\n",
                m_Tokenizer.GetFilename(),
                m_Tokenizer.GetLineNumber(),
                Buff);
    }

    std::cout << s_Error << std::endl;;

    return false;
}

//=========================================================================

bool text_in::ReadAllFields()
{
    token_stream::type Type;

    // Reset the current field count to zero
    m_nValidFields = 0;
    m_Record.nFields = 0;

    //
    // First lets make sure that we start with the right delimeter
    //
    Type = m_Tokenizer.Read();
    if (Type != token_stream::TOKEN_DELIMITER) {
        return false;
    }

    if (m_Tokenizer.Delimiter() != '{') {
        return ReportError("Puntuation missing. Expecting { found %c", m_Tokenizer.Delimiter());
    }

    //
    // Now we are ready to read fields
    //
    while (1) {
        // Update number of fields allocated
        if (m_Record.nFields >= m_Record.nFieldsAllocated) {
            m_Record.nFieldsAllocated = std::max(32, (m_Record.nFields + 1) + (m_Record.nFields / 2));
            m_Record.Field = (field*)realloc(m_Record.Field, sizeof(field) * m_Record.nFieldsAllocated);
            assert(m_Record.Field);
        }
        field& Field = m_Record.Field[m_Record.nFields];

        //
        // First must find a string
        //
        Type = m_Tokenizer.Read();
        if (Type != token_stream::TOKEN_SYMBOL) {
            // Are we done reading fields?
            if (Type == token_stream::TOKEN_DELIMITER) {
                if (m_Tokenizer.Delimiter() == '}') {
                    break;
                }
            }

            return ReportError("Expecting to read a string for the name of the field but found something else");
        }

        // Read the name of the field
        strncpy(Field.Name, m_Tokenizer.String(), TEXTFILE_MAX_STRLENGTH - 1);

        //
        // Now we must find a delimeter
        //
        Type = m_Tokenizer.Read();
        if (Type != token_stream::TOKEN_DELIMITER) {
            return ReportError("Puntuation missing. Expecting : found somthing else");
        }

        if (m_Tokenizer.Delimiter() != ':') {
            return ReportError("Puntuation missing. Expecting : found %c", m_Tokenizer.Delimiter());
        }

        //
        // Now we should read a string
        //
        Type = m_Tokenizer.Read();
        if (Type != token_stream::TOKEN_SYMBOL) {
            return ReportError("Expecting the type of the field but found something else");
        }

        //
        // We must interpret the string into types
        //
        {
            char* pStr = m_Tokenizer.String();

            assert(pStr);
            Field.nTypes = 0;
            Field.ID = -1;

            while (*pStr) {
                assert(Field.nTypes < TEXTFILE_MAX_TYPES);
                switch (*pStr) {
                case 'd':
                case 'D':
                    Field.Type[Field.nTypes++] = TYPE_INTEGER;
                    break;

                case 'f':
                case 'F':
                    Field.Type[Field.nTypes++] = TYPE_FLOAT;
                    break;

                case 'S':
                case 's':
                    Field.Type[Field.nTypes++] = TYPE_STRING;
                    break;

                case 'G':
                case 'g':
                    Field.Type[Field.nTypes++] = TYPE_GUID;
                    break;

                default:
                    return ReportError("Unkown type %c from %s field", *pStr, Field.Name);
                }

                // Get the next character
                pStr++;
            }
            assert(Field.nTypes < TEXTFILE_MAX_TYPES);
        }

        //
        // Okay we are done with this field
        //
        m_Record.nFields++;
    }

    // DONE
    return true;
}

//=========================================================================

bool text_in::ReadHeader()
{
    token_stream::type Type;

    // Clear the record name
    m_Record.Name[0] = 0;
    m_Record.Count = -1;
    m_RecordLineNumber = 0;
    m_nFieldMatches = 0;

    //
    // Read Delimiters untill we find the right one
    //
    while (1) {
        Type = m_Tokenizer.Read();

        if (Type == token_stream::TOKEN_EOF ||
            Type == token_stream::TOKEN_NONE) {
            return false;
        }

        if (Type == token_stream::TOKEN_DELIMITER) {
            if (m_Tokenizer.Delimiter() == '[') {
                break;
            }
        }
    }

    //
    // Now it should be fallow by a string
    //
    Type = m_Tokenizer.Read();
    if (Type != token_stream::TOKEN_SYMBOL) {
        return ReportError("Expecting a string found something else");
    }

    strcpy(m_Record.Name, m_Tokenizer.String());

    //
    // Now if must be fallow by a delimeter
    //
    Type = m_Tokenizer.Read();
    if (Type != token_stream::TOKEN_DELIMITER) {
        return ReportError("Expecting a ] or : But found something else");
    }

    // If it is the end of the block we are then done
    if (m_Tokenizer.Delimiter() == ']') {
        return ReadAllFields();
    }

    // We don't know that is going on so just return
    if (m_Tokenizer.Delimiter() != ':') {
        return ReportError("Expecting a ] or : But found %c", m_Tokenizer.Delimiter());
    }

    //
    // Must be a integer at this point
    //
    Type = m_Tokenizer.Read();
    if (Type != token_stream::TOKEN_NUMBER) {
        return ReportError("After : specting an integer but found something else");
    }

    // Get our count
    m_Record.Count = m_Tokenizer.Int();

    //
    // Now it must be a nother delimerter
    //
    Type = m_Tokenizer.Read();
    if (Type != token_stream::TOKEN_DELIMITER) {
        return ReportError("Expecting ] But stead found something else");
    }

    //
    // Now start reading fields
    //
    return ReadAllFields();
}

//=========================================================================
#define TEXTFILE_MAX_FLOATS 16
#define TEXTFILE_MAX_INTS 16
#define TEXTFILE_MAX_STRINGS 4
#define TEXTFILE_MAX_GUIDS 1

bool text_in::ReadFields()
{
    if (m_Record.nFields > m_nDataEntriesAllocated) {
        delete[] m_pData;
        m_nDataEntriesAllocated = m_Record.nFields;
        m_pData = new data[m_nDataEntriesAllocated];
    }
    for (int f = 0; f < m_Record.nFields; f++) {
        field& Field = m_Record.Field[f];
        data&  Data = m_pData[f];

        Data.nFloats = 0;
        Data.nIntegers = 0;
        Data.nStrings = 0;
        Data.nGuids = 0;

        for (int i = 0; i < Field.nTypes; i++) {
            token_stream::type Type = m_Tokenizer.Read();

            // Read the
            switch (Field.Type[i]) {
            case TYPE_FLOAT:
            {
                assert(Data.nFloats < TEXTFILE_MAX_FLOATS);
                if (Type != token_stream::TOKEN_NUMBER && m_Tokenizer.IsFloat() == true) {
                    return ReportError("Expecting a FLOAT but found something else");
                }

                Data.Float[Data.nFloats++] = m_Tokenizer.Float();

                break;
            }
            case TYPE_INTEGER:
            {
                assert(Data.nIntegers < TEXTFILE_MAX_INTS);
                if (Type != token_stream::TOKEN_NUMBER && m_Tokenizer.IsFloat() == false) {
                    return ReportError("Expecting a INTEGER but found something else");
                }

                Data.Integer[Data.nIntegers++] = m_Tokenizer.Int();

                break;
            }
            case TYPE_STRING:
            {
                assert(Data.nStrings < TEXTFILE_MAX_STRINGS);
                if (Type != token_stream::TOKEN_STRING) {
                    return ReportError("Expecting a STRING but found something else");
                }

                strcpy(Data.String[Data.nStrings++], m_Tokenizer.String());

                break;
            }

            case TYPE_GUID:
            {
                assert(Data.nGuids < TEXTFILE_MAX_GUIDS);
                if (Type != token_stream::TOKEN_STRING) {
                    return ReportError("Expecting a GUID but found something else");
                }

                guid GUID;

                //
                // Take from guid.cpp
                //
                /* TODO
                {
                    GUID.Guid = 0;
                    const char* pGUID = m_Tokenizer.String();

                    while (*pGUID) {
                        char c = *pGUID;
                        pGUID++;

                        if (c == ':') {
                            continue;
                        }
                        uint32_t v = 0;
                        if (c > '9') {
                            v = (c - 'A') + 10;
                        } else {
                            v = (c - '0');
                        }

                        GUID.Guid <<= 4;
                        GUID.Guid |= (v & 0xF);
                    }
                }*/

                Data.Guid[Data.nGuids++] = GUID;

                break;
            }

            default:
                assert(false);
            }
        }
    }

    // Increase the line number
    m_RecordLineNumber++;
    m_RecordTypeNumber = 0;

    return true;
}

//=========================================================================

int text_in::Stricmp(const char* pStr1, const char* pStr2, int Count)
{
    char Buf[256];
    strncpy(Buf, pStr2, Count);
    Buf[Count] = 0;
    // TODO return x_stricmp(pStr1, Buf);
    return strcmp(pStr1, Buf);
}

//=========================================================================

bool text_in::GetField(const char* pFieldName, ...)
{

    //
    // Verify that all is good in the first line
    //
    if (m_RecordLineNumber == 1) {
        int   Index = -1;
        int   j;
        const char* pType = strstr(pFieldName, ":");

        if (pType == nullptr) {
            return ReportError("User forgot to put the type in %s field", pFieldName);
        }

        //
        // Make sure that we have that field
        //
        {
            int Length = (int)(pType - pFieldName);
            for (j = 0; j < m_Record.nFields; j++) {
                if (Stricmp(m_Record.Field[j].Name, pFieldName, Length) == 0) {
                    if (m_Record.Field[j].ID != -1) {
                        return ReportError("User register the same type (%s) twice", pFieldName);
                    }

                    // Set the user ID
                    m_Record.Field[j].ID = 0;
                    Index = j;

                    // There was found and set
                    m_nValidFields++;
                    break;
                }
            }

            if (j == m_Record.nFields) {
                return ReportError("The field %s was not found in the file", pFieldName);
            }
        }
        assert(Index >= 0);

        //
        // Check the types against what we know form the file
        //
        {
            int    nValidTypes = 0;
            field& Field = m_Record.Field[Index];

            // Skip the :
            pType++;

            while (pType[nValidTypes]) {
                type Type;
                switch (pType[nValidTypes]) {
                case 'd':
                case 'D':
                    Type = TYPE_INTEGER;
                    break;

                case 'f':
                case 'F':
                    Type = TYPE_FLOAT;
                    break;

                case 'S':
                case 's':
                    Type = TYPE_STRING;
                    break;

                case 'G':
                case 'g':
                    Type = TYPE_GUID;
                    break;

                default:
                    return ReportError("Unkown type %c from %s field", pType[nValidTypes], Field.Name);
                }

                if (Type != Field.Type[nValidTypes++]) {
                    return ReportError("For %s field the types don't match.", Field.Name);
                }
            }

            if (nValidTypes != Field.nTypes) {
                return ReportError("For field %s the types are different from the user specify one.", Field.Name);
            }

            // Resize array if needed
            if (m_RecordTypeNumber >= m_nFieldMatchesAllocated) {
                m_nFieldMatchesAllocated = std::max(32, (m_RecordTypeNumber + 1) + (m_RecordTypeNumber / 2));
                m_FieldMatch = (field_match*)realloc(m_FieldMatch, sizeof(field_match) * m_nFieldMatchesAllocated);
                assert(m_FieldMatch);
            }
            //
            // Set the new type
            //
            m_FieldMatch[m_RecordTypeNumber].UserString = pFieldName;
            m_FieldMatch[m_RecordTypeNumber].Index = Index;
        }
    }

    // SB:
    // Crash fix - if we have past the last field, then this field is not present so found bail elegantly
    if (m_RecordTypeNumber == m_Record.nFields) {
        return false;
    }

    //
    // Now give the user the data
    //
    if (m_FieldMatch[m_RecordTypeNumber].UserString == pFieldName) {
        data&  Data = m_pData[m_FieldMatch[m_RecordTypeNumber].Index];
        field& Field = m_Record.Field[m_FieldMatch[m_RecordTypeNumber].Index];
        // Make sure we're giving the correct data.
        /* TODO: 
        if (xstring(pFieldName).Find(xstring(Field.Name)) == -1) {
            return false;
        }
*/
        va_list Args;
        va_start(Args, pFieldName);

        int I = 0, F = 0, S = 0, G = 0;

        for (int i = 0; i < Field.nTypes; i++) {
            switch (Field.Type[i]) {
            case TYPE_INTEGER:
            {
                int* p = (va_arg(Args, int*));
                assert(p);
                *p = Data.Integer[I++];
                break;
            }
            case TYPE_FLOAT:
            {
                float* p = (va_arg(Args, float*));
                assert(p);
                *p = Data.Float[F++];
                break;
            }
            case TYPE_STRING:
            {
                char* p = (va_arg(Args, char*));
                assert(p);
                strcpy(p, Data.String[S++]);
                break;
            }
            case TYPE_GUID:
            {
                guid* p = (va_arg(Args, guid*));
                assert(p);
                *p = Data.Guid[G++];
                break;
            }
            case TYPE_NULL:
                break;
            }
        }
    } else {
        // Must do a manual search for the field.
        // Right now we don't handle this case.
        return false;
    }

    m_RecordTypeNumber++;

    return true;
}

//=========================================================================

bool text_in::SkipToNextHeader()
{
    int i, n;

    // Determine whether we have a count or not
    // if not then we assume that there is only one line
    if (m_Record.Count == -1) {
        n = 1;
    } else {
        n = m_Record.Count;
    }

    // Read all the lines
    for (i = 0; i < n; i++) {
        ReadFields();
    }

    return true;
}

//=========================================================================

bool text_in::GetVector3(const char* pName, Vector3& V)
{
    char buf[64];
    snprintf(buf, 64, "%s:fff", pName);
    return GetField(buf, &V.x, &V.y, &V.z);
}

//=========================================================================

bool text_in::GetColor(const char* pName, Colour& C)
{
    char buf[64];
    snprintf(buf, 64, "%s:dddd", pName);

    int  R, G, B, A;
    bool Result = GetField(buf, &R, &G, &B, &A);
    C.r = (uint8_t)(R & 0xFF);
    C.g = (uint8_t)(G & 0xFF);
    C.b = (uint8_t)(B & 0xFF);
    C.a = (uint8_t)(A & 0xFF);
    return Result;
}

//=========================================================================

bool text_in::GetF32(const char* pName, float& F)
{
    char buf[64];
    snprintf(buf, 64, "%s:f", pName);
    return GetField(buf, &F);
}

//=========================================================================

bool text_in::GetS32(const char* pName, int& I)
{
    char buf[64];
    snprintf(buf, 64, "%s:d", pName);
    return GetField(buf, &I);
}

//=========================================================================

bool text_in::GetString(const char* pName, char* pStr)
{
    char buf[64];
    snprintf(buf, 64, "%s:s", pName);
    return GetField(buf, pStr);
}

//=========================================================================

bool text_in::GetBBox(const char* pName, BBox& BBox)
{
    char buf[64];
    snprintf(buf, 64, "%s:ffffff", pName);
    return GetField(buf,
                    &BBox.min.x, &BBox.min.y, &BBox.min.z,
                    &BBox.max.x, &BBox.max.y, &BBox.max.z);
}

//=========================================================================

bool text_in::GetRadian3(const char* pName, Radian3& Orient)
{
    char buf[64];
    snprintf(buf, 64, "%s:fff", pName);
    float  P, Y, R;
    bool Result = GetField(buf, &P, &Y, &R);

    Orient.pitch = DEG_TO_RAD(P);
    Orient.yaw = DEG_TO_RAD(Y);
    Orient.roll = DEG_TO_RAD(R);

    return Result;
}

//=========================================================================

bool text_in::GetQuaternion(const char* pName, Quaternion& Q)
{
    char buf[64];
    snprintf(buf, 64, "%s:ffff", pName);
    return GetField(buf, &Q.x, &Q.y, &Q.z, &Q.w);
}

//=========================================================================

bool text_in::GetBool(const char* pName, bool& Bool)
{
    char buf[64];
    snprintf(buf, 64, "%s:d", pName);
    return GetField(buf, &Bool);
}

//=========================================================================

bool text_in::GetGuid(const char* pName, guid& Guid)
{
    char buf[64];
    snprintf(buf, 64, "%s:g", pName);
    return GetField(buf, &Guid);
}
