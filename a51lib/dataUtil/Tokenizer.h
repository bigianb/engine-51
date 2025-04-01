#pragma once

#include "../Colour.h"
#include "../VectorMath.h"

#define TOKEN_STRING_SIZE 1024
#define TOKEN_DELIMITER_STR ",{}()<>;"

class token_stream
{
public:
    enum type
    {
        TOKEN_NONE,
        TOKEN_NUMBER,
        TOKEN_DELIMITER,
        TOKEN_SYMBOL,
        TOKEN_STRING,
        TOKEN_EOF = 0x7FFFFFFF,
    };

public:
    token_stream();
    ~token_stream();

    void OpenText(const char* pTextString);
    void CloseText();
    bool IsEOF() const;

    char* GetDelimeter();
    void  SetDelimeter(const char* pStr);

    // Move through tokens in file
    void  Rewind();
    type  Read(int NTokens = 1);
    float ReadFloat();
    int   ReadInt();
    int   ReadHex();
    char* ReadSymbol();
    char* ReadString();
    char* ReadLine();
    char* ReadToSymbol(char Sym);
    float ReadF32FromString();
    int   ReadS32FromString();
    bool  ReadBoolFromString();

    void SkipToNextLine();

    // Complex requests
    bool  Find(const char* TokenStr, bool FromBeginning = false);
    int   GetCursor();
    void  SetCursor(int Pos);
    int   GetLineNumber() { return m_LineNumber; }
    char* GetFilename() { return m_Filename; }

    // Interrogate about current token
    type  Type() { return m_Type; }
    float Float() { return m_Float; }
    int   Int() { return m_Int; }
    char  Delimiter() { return m_Delimiter; }
    char* String() { return m_String; }
    bool  IsFloat() { return m_IsFloat; }

protected:
    void SkipWhitespace();
    char CHAR(int FilePos);

protected:
    int   m_FileSize;
    char* m_FileBuffer;
    int   m_FilePos;
    int   m_LineNumber;

    char    m_Filename[64];
    char    m_DelimiterStr[16];
    uint8_t m_IsCharNumber[256];

    type  m_Type;
    char  m_String[TOKEN_STRING_SIZE];
    float m_Float;
    int   m_Int;
    bool  m_IsFloat;
    char  m_Delimiter;

    bool  m_bBuffered;
    int   m_CurBufferStart;
    int   m_CurBufferEnd;
    char* m_CurBuffer;
    int   m_StartPosition;
};
