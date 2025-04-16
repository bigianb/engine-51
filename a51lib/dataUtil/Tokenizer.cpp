#include "Tokenizer.h"
#include <cstring>
#include <cassert>
#include <cmath>

static const int BUFFER_SIZE = 1024 * 32;

#define CHAR(s) m_FileBuffer[(s)]

token_stream::token_stream()
{
    m_FileBuffer = nullptr;
    m_FileSize = 0;
    m_LineNumber = 1;
    m_CurBuffer = nullptr;
    m_bBuffered = false;
    m_CurBufferStart = -1;
    m_CurBufferEnd = -1;

    strcpy(m_DelimiterStr, TOKEN_DELIMITER_STR);

    // Setup number chars
    int  i;
    char IStr[] = "0123456789-+";
    char FStr[] = "Ee.#QNABIFD";
    for (i = 0; i < 256; i++) {
        m_IsCharNumber[i] = 0;
    }
    i = 0;
    while (IStr[i]) {
        m_IsCharNumber[IStr[i]] = 1;
        i++;
    }
    i = 0;
    while (FStr[i]) {
        m_IsCharNumber[FStr[i]] = 2;
        i++;
    }
}

token_stream::~token_stream()
{
    if (m_FileBuffer) {
        free(m_FileBuffer);
        m_FileBuffer = nullptr;
    }
    if (m_CurBuffer) {
        free(m_CurBuffer);
        m_CurBuffer = nullptr;
    }
    m_bBuffered = false;
    m_CurBufferStart = -1;
    m_CurBufferEnd = -1;
}

bool token_stream::IsEOF() const
{
    return m_FilePos >= m_FileSize;
}

//==============================================================================

char* token_stream::GetDelimeter()
{
    return m_DelimiterStr;
}

//==============================================================================

void token_stream::SetDelimeter(const char* pStr)
{
    assert(strlen(pStr) >= 2);
    strcpy(m_DelimiterStr, pStr);
}

//==============================================================================

void token_stream::Rewind()
{
    m_FilePos = 0;
    m_LineNumber = 1;
    m_Type = TOKEN_NONE;
    m_Delimiter = ' ';
    m_Float = 0.0f;
    m_Int = 0;
    m_String[0] = 0;
}

//==============================================================================

void token_stream::SetCursor(int Pos)
{
    assert((Pos >= 0) && (Pos < m_FilePos));
    m_FilePos = Pos;
    m_LineNumber = 1;
    m_Type = TOKEN_NONE;
    m_Delimiter = ' ';
    m_Float = 0.0f;
    m_Int = 0;
    m_String[0] = 0;
}

//==============================================================================

int token_stream::GetCursor()
{
    return m_FilePos;
}

//==============================================================================

bool token_stream::Find(const char* TokenStr, bool FromBeginning)
{
    if (FromBeginning) {
        Rewind();
    }

    Read();

    while (m_Type != TOKEN_EOF) {
        // TODO: should be case insensitive
        if (strcmp(m_String, TokenStr) == 0) {
            return true;
        }
        Read();
    }

    return false;
}

//==============================================================================

void token_stream::SkipToNextLine()
{
    while (1) {
        if (m_FilePos >= m_FileSize) {
            return;
        }

        // Move forward to end of line
        {
            while ((CHAR(m_FilePos) != '\n') && (m_FilePos < m_FileSize)) {
                m_FilePos++;
            }

            if (CHAR(m_FilePos) == '\n') {
                m_LineNumber++;
                m_FilePos++;
                return;
            }
        }
    }
}

//==============================================================================

void token_stream::SkipWhitespace()
{
    while (1) {
        if (m_FilePos >= m_FileSize) {
            return;
        }

        // Skip whitespace and count EOLNs
        if (CHAR(m_FilePos) <= 32) {
            while ((m_FilePos < m_FileSize) && (CHAR(m_FilePos) <= 32)) {
                if (CHAR(m_FilePos) == '\n') {
                    m_LineNumber++;
                }
                m_FilePos++;
            }

            continue;
        }

        // Watch for line comment
        if ((CHAR(m_FilePos + 0) == '/') && (CHAR(m_FilePos + 1) == '/')) {
            m_FilePos += 2;

            while ((CHAR(m_FilePos) != '\n') && (m_FilePos < m_FileSize)) {
                m_FilePos++;
            }

            if (CHAR(m_FilePos) == '\n') {
                m_LineNumber++;
                m_FilePos++;
            }

            continue;
        }

        // Watch for block comment
        if ((CHAR(m_FilePos + 0) == '/') && (CHAR(m_FilePos + 1) == '*')) {
            m_FilePos += 2;

            while (m_FilePos <= m_FileSize - 1) {
                if ((CHAR(m_FilePos + 0) == '*') && (CHAR(m_FilePos + 1) == '/')) {
                    m_FilePos += 2;
                    break;
                }

                if (CHAR(m_FilePos) == '\n') {
                    m_LineNumber++;
                }

                m_FilePos++;
            }

            continue;
        }

        // No whitespace found
        return;
    }
}

//==============================================================================

token_stream::type token_stream::Read(int NTokens)
{
    while (NTokens--) {
        int  i, j;
        char ch;

        // Clear the current settings
        m_Type = TOKEN_NONE;
        m_Delimiter = ' ';
        m_Float = 0.0f;
        m_Int = 0;
        m_IsFloat = false;
        m_String[0] = 0;

        assert(m_FilePos <= m_FileSize);

        // Skip whitespace and comments
        SkipWhitespace();

        // Watch for end of file
        if (m_FilePos >= m_FileSize) {
            m_Type = TOKEN_EOF;
            continue;
        }

        // Look for number first since we load a lot of these
        ch = CHAR(m_FilePos);
        if (((ch >= '0') && (ch <= '9')) || (ch == '-') || (ch == '+')) {
            // Copy number into string buffer
            i = 0;
            u_int8_t floatTracker = 0;
            while (1) {
                ch = CHAR(m_FilePos);
                if (!m_IsCharNumber[ch]) {
                    break;
                }
                m_String[i] = ch;
                floatTracker |= m_IsCharNumber[ch];
                m_FilePos++;
                i++;
                assert(i < TOKEN_STRING_SIZE - 1);
            }
            floatTracker >>= 1;
            m_String[i] = 0;

            m_IsFloat = floatTracker != 0;

            // Generate float version
            if (m_IsFloat) {
                m_Float = atof(m_String);
                m_Int = (int)m_Float;
                m_Type = TOKEN_NUMBER;
            } else {
                m_Int = atoi(m_String);
                m_Float = (float)m_Int;
                m_Type = TOKEN_NUMBER;
            }

            continue;
        }

        // Watch for string
        if (CHAR(m_FilePos) == '"') {
            m_FilePos++;
            i = 0;
            while (CHAR(m_FilePos) != '"') {
                // Check for illegal ending of a string
                assert((m_FilePos < m_FileSize) && "EOF in quote");
                assert((i < TOKEN_STRING_SIZE - 1) && "Quote too long");
                assert((CHAR(m_FilePos) != '\n') && "EOLN in quote");

                m_String[i] = CHAR(m_FilePos);
                i++;
                m_FilePos++;
                assert(i < TOKEN_STRING_SIZE - 1);
            }

            m_FilePos++;
            m_String[i] = 0;
            m_Type = TOKEN_STRING;

            continue;
        }

        // Look for delimiter
        ch = CHAR(m_FilePos);
        i = 0;
        while (m_DelimiterStr[i] && (ch != m_DelimiterStr[i])) {
            i++;
        }
        if (m_DelimiterStr[i]) {
            m_FilePos++;
            m_Type = TOKEN_DELIMITER;
            m_Delimiter = ch;
            m_String[0] = ch;
            m_String[1] = 0;
            continue;
        }

        // Treat this as a raw symbol
        {
            i = 0;
            while (m_FilePos < m_FileSize) {
                j = 0;
                if (i == TOKEN_STRING_SIZE - 1) {
                    break;
                }
                if (CHAR(m_FilePos) <= 32) {
                    break;
                }
                while (m_DelimiterStr[j] && (CHAR(m_FilePos) != m_DelimiterStr[j])) {
                    j++;
                }
                if (m_DelimiterStr[j]) {
                    break;
                }

                m_String[i] = (char)CHAR(m_FilePos);
                i++;
                m_FilePos++;
            }

            m_String[i] = 0;
            m_Type = TOKEN_SYMBOL;
            continue;
        }
    }

    return m_Type;
}

//==============================================================================

float token_stream::ReadFloat()
{
    SkipWhitespace();

    char ch = CHAR(m_FilePos);
    assert(((ch >= '0') && (ch <= '9')) || (ch == '-') || (ch == '+'));

    // Copy number into string buffer
    int i = 0;
    while (1) {
        ch = CHAR(m_FilePos);
        if (!m_IsCharNumber[ch]) {
            break;
        }
        m_String[i] = ch;
        m_FilePos++;
        i++;
        assert(i < TOKEN_STRING_SIZE - 1);
    }

    // Generate float version
    m_String[i] = 0;
    m_Float = atof(m_String);
    m_Int = (int)m_Float;
    m_Type = TOKEN_NUMBER;
    m_IsFloat = true;

    return m_Float;
}

//==============================================================================

float token_stream::ReadF32FromString()
{
    ReadString();

    // Transform string into a float
    m_Float = atof(m_String);
    m_Int = (int)m_Float;
    m_Type = TOKEN_NUMBER;
    m_IsFloat = true;

    return m_Float;
}

//==============================================================================

int token_stream::ReadS32FromString()
{
    ReadString();

    // Transform string into a int
    m_Int = atoi(m_String);
    m_Float = (float)m_Int;
    m_Type = TOKEN_NUMBER;
    m_IsFloat = false;

    return m_Int;
}

//==============================================================================

bool token_stream::ReadBoolFromString()
{
    ReadString();

    // Transform string into an bool
    m_Int = atoi(m_String);
    m_Float = (float)m_Int;
    m_Type = TOKEN_NUMBER;
    m_IsFloat = false;

    return (m_Int) ? (true) : (false);
}

//==============================================================================

int token_stream::ReadInt()
{
    SkipWhitespace();

    char ch = CHAR(m_FilePos);
    assert(((ch >= '0') && (ch <= '9')) || (ch == '-') || (ch == '+'));

    // Copy number into string buffer
    int i = 0;
    while (1) {
        ch = CHAR(m_FilePos);
        if (!m_IsCharNumber[ch]) {
            break;
        }
        m_String[i] = ch;
        m_FilePos++;
        i++;
        assert(i < TOKEN_STRING_SIZE - 1);
    }

    // Generate int version
    m_String[i] = 0;
    m_Int = atoi(m_String);
    m_Float = (float)m_Int;
    m_Type = TOKEN_NUMBER;
    m_IsFloat = false;

    //x_DebugMsg("Read integer: %d\n", m_Int);
    return m_Int;
}

//==============================================================================

int token_stream::ReadHex()
{
    SkipWhitespace();

    char ch = CHAR(m_FilePos);
    m_FilePos++;
    assert(ch == '0');
    ch = CHAR(m_FilePos);
    m_FilePos++;
    assert(ch == 'x');

    // Copy number into string buffer
    int Number = 0;
    int i = 2;
    m_String[0] = '0';
    m_String[1] = 'x';
    while (1) {
        break;
        // TODO:
        /*
        ch = toupper(CHAR(m_FilePos));
        if (ishex(ch)) {
            int Digit = ch - '0';
            if (Digit >= 10) {
                Digit -= 'A' - '0' - 10;
            }
            Number *= 16;
            Number += Digit;
            m_FilePos++;
            assert(i < TOKEN_STRING_SIZE - 1);
            m_String[i++] = ch;
        } else {
            break;
        }
            */
    }

    // Generate int version
    m_String[i] = 0;
    m_Int = Number;
    m_Float = (float)m_Int;
    m_Type = TOKEN_NUMBER;
    m_IsFloat = false;

    return m_Int;
}

//==============================================================================

char* token_stream::ReadSymbol()
{
    int i, j;

    SkipWhitespace();

    i = 0;
    while (m_FilePos < m_FileSize) {
        j = 0;
        if (i == TOKEN_STRING_SIZE - 1) {
            break;
        }
        if (CHAR(m_FilePos) <= 32) {
            break;
        }
        while (m_DelimiterStr[j] && (CHAR(m_FilePos) != m_DelimiterStr[j])) {
            j++;
        }
        if (m_DelimiterStr[j]) {
            break;
        }

        m_String[i] = (char)CHAR(m_FilePos);
        i++;
        m_FilePos++;
        assert(i < TOKEN_STRING_SIZE - 1);
    }

    m_String[i] = 0;
    m_Type = TOKEN_SYMBOL;
    return m_String;
}

//==============================================================================

char* token_stream::ReadString()
{
    SkipWhitespace();

    assert(CHAR(m_FilePos) == '"');

    m_FilePos++;
    int i = 0;
    while (CHAR(m_FilePos) != '"') {
        // Check for illegal ending of a string
        assert((m_FilePos < m_FileSize) && "EOF in quote");
        assert((i < TOKEN_STRING_SIZE - 1) && "Quote too long");
        assert((CHAR(m_FilePos) != '\n') && "EOLN in quote");

        m_String[i] = CHAR(m_FilePos);
        i++;
        m_FilePos++;
        assert(i < TOKEN_STRING_SIZE - 1);
    }

    m_FilePos++;
    m_String[i] = 0;
    m_Type = TOKEN_STRING;
    return m_String;
}

//==============================================================================

char* token_stream::ReadLine()
{
    // Enf of file not reached or string buffer full?
    int i = 0;
    while ((i < (TOKEN_STRING_SIZE - 1)) && (m_FilePos < m_FileSize)) {
        // Get char
        char C = CHAR(m_FilePos++);

        // End of line reached?
        if ((C == '\n') || (C == 13)) {
            break;
        }

        // Add to string
        m_String[i++] = C;
    }

    // Terminate the string
    m_String[i] = 0;
    m_Type = TOKEN_SYMBOL;

    // Skip end of line feeds
    while (m_FilePos < m_FileSize) {
        // Get char
        char C = CHAR(m_FilePos++);

        // End of line reached?
        if ((C != '\n') || (C != 13)) {
            break;
        }
    }

    return m_String;
}

char* token_stream::ReadToSymbol(char Sym)
{
    int i = 0;
    while (CHAR(m_FilePos) != Sym) {
        // Check for illegal ending of a string
        assert((m_FilePos < m_FileSize) && "EOF in quote");
        assert((i < TOKEN_STRING_SIZE - 1) && "Quote too long");

        m_String[i] = CHAR(m_FilePos);
        i++;
        m_FilePos++;
    }

    m_FilePos++;
    m_String[i] = 0;
    m_Type = TOKEN_STRING;
    return m_String;
}

void token_stream::OpenText(const char* pTextString)
{
    assert(pTextString);
    // Clear class
    m_FileSize = 0;
    m_FilePos = 0;
    m_LineNumber = 1;
    m_Type = TOKEN_NONE;

    strcpy(m_Filename, "<internal string>");

    m_bBuffered = false;
    m_FileBuffer = (char*)pTextString;

    // Find how large the file is
    m_FileSize = strlen(pTextString) + 1;

    Rewind();

    m_StartPosition = 0;
}

void token_stream::CloseText()
{
    assert(m_bBuffered == false);
    assert(m_CurBuffer == nullptr);

    m_FileBuffer = nullptr;
}
