
#include "animData.h"

event_data_format::event_data_format()
{
    m_Flags = 0;
}

//=========================================================================

int event_data_format::CountSetBits(int NumBitsBefore, int NumBitsPossible) const
{
    int      i;
    int      Count = 0;
    uint32_t Mask = 1 << NumBitsBefore;
    for (i = 0; i < NumBitsPossible; ++i) {
        if (m_Flags & Mask) {
            ++Count;
        }

        Mask <<= 1;
    }

    return Count;
}

//=========================================================================

void event_data_format::SetInt(int Idx)
{
    uint32_t Mask = 1;
    Mask <<= Idx;
    m_Flags |= Mask;
}

//=========================================================================

void event_data_format::SetFloat(int Idx)
{
    uint32_t Mask = 1;
    Mask <<= EVENT_MAX_INTS + Idx;
    m_Flags |= Mask;
}

//=========================================================================

void event_data_format::SetPoint(int Idx)
{
    uint32_t Mask = 1;
    Mask <<= EVENT_MAX_INTS + EVENT_MAX_FLOATS + Idx;
    m_Flags |= Mask;
}

//=========================================================================

void event_data_format::SetBool(int Idx)
{
    uint32_t Mask = 1;
    Mask <<= EVENT_MAX_INTS + EVENT_MAX_FLOATS + EVENT_MAX_POINTS + Idx;
    m_Flags |= Mask;
}

//=========================================================================

void event_data_format::SetString(int Idx)
{
    uint32_t Mask = 1;
    Mask <<= EVENT_MAX_INTS + EVENT_MAX_FLOATS + EVENT_MAX_POINTS + EVENT_MAX_BOOLS + Idx;
    m_Flags |= Mask;
}

event_data::event_data()
{
    m_Type[0] = '\0';
    m_Name[0] = '\0';
}

//=========================================================================

void event_data::SetType(const char* Type)
{
    strncpy(m_Type, Type, EVENT_MAX_STRING_LENGTH);
}

//=========================================================================

void event_data::SetName(const char* Name)
{
    strncpy(m_Name, Name, EVENT_MAX_STRING_LENGTH);
}

//=========================================================================

void event_data::StoreInt(int Idx, int Value)
{
    m_Ints[Idx] = Value;
    m_DataFormat.SetInt(Idx);
}

//=========================================================================

void event_data::StoreFloat(int Idx, float Value)
{
    m_Floats[Idx] = Value;
    m_DataFormat.SetFloat(Idx);
}

//=========================================================================

void event_data::StorePoint(int Idx, const Vector3& Value)
{
    m_Points[Idx] = Value;
    m_DataFormat.SetPoint(Idx);
}

//=========================================================================

void event_data::StoreBool(int Idx, bool Value)
{
    m_Bools[Idx] = Value;
    m_DataFormat.SetBool(Idx);
}

//=========================================================================

void event_data::StoreString(int Idx, const char* String)
{
    if (NULL == String) {
        return;
    }
    strncpy(m_Strings[Idx], String, EVENT_MAX_STRING_LENGTH);
    m_DataFormat.SetString(Idx);
}

int PaddedStringLength(const char* String);
int StringPadding(const char* String);

anim_event::anim_event()
{
    m_ByteStreamDataOffset = 0;
    m_TypeOffset = 0;
    m_NameOffset = 0;
}

//=========================================================================
anim_event::~anim_event()
{
}

//=========================================================================
void anim_event::Init()
{
    //if ( NULL == m_pEventByteStream ) m_pEventByteStream = new xbytestream;
    //if ( NULL == m_pEventTypeNameStrings ) m_pEventTypeNameStrings = new xbytestream;
}

//=========================================================================
void anim_event::SetData(const event_data& Data)
{
    int      i;
    uint32_t Mask = 1;
    m_DataFormat.m_Flags = 0;

    if (Data.GetType()) {
        m_TypeOffset = SaveTypeNameString(Data.GetType());
    }

    if (Data.GetName()) {
        m_NameOffset = SaveTypeNameString(Data.GetName());
    }

    //
    // Set flags
    //
    for (i = 0; i < EVENT_MAX_INTS; ++i) {
        if (i < Data.nInts()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }
    for (i = 0; i < EVENT_MAX_FLOATS; ++i) {
        if (i < Data.nFloats()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }
    for (i = 0; i < EVENT_MAX_POINTS; ++i) {
        if (i < Data.nPoints()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }
    for (i = 0; i < EVENT_MAX_BOOLS; ++i) {
        if (i < Data.nBools()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }
    for (i = 0; i < EVENT_MAX_STRINGS; ++i) {
        if (i < Data.nStrings()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }

    //
    // Store all the data
    //
    /*
    m_ByteStreamDataOffset = m_pEventByteStream->GetLength();

    m_pEventByteStream->Append( (uint8_t*)(Data.Ints()), sizeof( int ) * Data.nInts() );
    m_pEventByteStream->Append( (uint8_t*)(Data.Floats()), sizeof( float ) * Data.nFloats() );
    m_pEventByteStream->Append( (uint8_t*)(Data.Points()), sizeof( Vector3 ) * Data.nPoints() );
    m_pEventByteStream->Append( (uint8_t*)(Data.Bools()), sizeof( bool ) * Data.nBools() );

    for ( i = 0; i < Data.nStrings(); ++i )
    {
        const int Length = strlen( Data.String( i ) );
        m_pEventByteStream->Append( (uint8_t*)(Data.String(i)), sizeof( char ) * Length );
        m_pEventByteStream->Append( '\0' );

        // dword align...
        m_pEventByteStream->Append( (uint8_t*)"@@@@", StringPadding( Data.String( i ) ) );
    }
        */
}

//=========================================================================

event_data anim_event::GetData() const
{
    event_data Data;
    int        i;

    Data.SetType(GetType());
    Data.SetName(GetName());

    for (i = 0; i < GetNInts(); ++i) {
        Data.StoreInt(i, GetInt(i));
    }

    for (i = 0; i < GetNFloats(); ++i) {
        Data.StoreFloat(i, GetFloat(i));
    }

    for (i = 0; i < GetNPoints(); ++i) {
        Data.StorePoint(i, GetPoint(i));
    }

    for (i = 0; i < GetNBools(); ++i) {
        Data.StoreBool(i, GetBool(i));
    }

    for (i = 0; i < GetNStrings(); ++i) {
        Data.StoreString(i, GetString(i));
    }

    return Data;
}

//=========================================================================

const char* anim_event::GetType() const
{
    return GetTypeNameString(m_TypeOffset);
}

//=========================================================================

const char* anim_event::GetName() const
{
    return GetTypeNameString(m_NameOffset);
}

//=========================================================================

inline const uint8_t* anim_event::GetDataBuffer() const
{
    return nullptr; //(uint8_t*)(m_pEventByteStream->GetBuffer() + m_ByteStreamDataOffset);
}

//=========================================================================

inline int event_data_format::GetNInts() const
{
    return CountSetBits(0, EVENT_MAX_INTS);
}

//=========================================================================

inline int event_data_format::GetNFloats() const
{
    return CountSetBits(EVENT_MAX_INTS, EVENT_MAX_FLOATS);
}

//=========================================================================

inline int event_data_format::GetNPoints() const
{
    return CountSetBits((EVENT_MAX_INTS + EVENT_MAX_FLOATS), EVENT_MAX_POINTS);
}

//=========================================================================

inline int event_data_format::GetNBools() const
{
    return CountSetBits((EVENT_MAX_INTS + EVENT_MAX_FLOATS + EVENT_MAX_POINTS), EVENT_MAX_BOOLS);
}

//=========================================================================

inline int event_data_format::GetNStrings() const
{
    return CountSetBits((EVENT_MAX_INTS + EVENT_MAX_FLOATS + EVENT_MAX_POINTS + EVENT_MAX_BOOLS), EVENT_MAX_STRINGS);
}

//=========================================================================

int anim_event::GetInt(int Idx) const
{
    const int Offset = Idx * sizeof(int);

    return *(int*)(GetDataBuffer() + Offset);
}

//=========================================================================

float anim_event::GetFloat(int Idx) const
{
    const int Offset = (GetNInts() * sizeof(int)) + (Idx * sizeof(float));

    return *(float*)(GetDataBuffer() + Offset);
}

//=========================================================================

Vector3 anim_event::GetPoint(int Idx) const
{
    const int Offset = (GetNInts() * sizeof(int)) + (GetNFloats() * sizeof(float)) + (Idx * sizeof(Vector3));

    return *(Vector3*)(GetDataBuffer() + Offset);

    //Vector3p* pVec = (Vector3p*)(GetDataBuffer() + Offset);
    //return Vector3(*pVec);
}

//=========================================================================

bool anim_event::GetBool(int Idx) const
{
    const int Offset = (GetNInts() * sizeof(int)) + (GetNFloats() * sizeof(float)) + (GetNPoints() * sizeof(Vector3)) + (Idx * sizeof(bool));

    return *(bool*)(GetDataBuffer() + Offset);
}

//=========================================================================
const char* anim_event::GetString(int Idx) const
{
    // First, find the offset to the first string
    const int Offset = (GetNInts() * sizeof(int)) + (GetNFloats() * sizeof(float)) + (GetNPoints() * sizeof(Vector3)) + (GetNBools() * sizeof(bool));

    char* pString = (char*)(GetDataBuffer() + Offset);

    // Count up to the correct string
    int i;
    for (i = 0; i < Idx; ++i) {
        //Get to the end of the string
        while (*pString != '\0') {
            ++pString;
        }
        ++pString;

        // Get past the padding
        while (*pString == '@') {
            ++pString;
        }
    }

    return pString;
}

//=========================================================================

const char* anim_event::GetTypeNameString(int Offset)
{
    //ASSERT( Offset < m_pEventTypeNameStrings->GetLength() );
    return ""; //((char*)(m_pEventTypeNameStrings->GetBuffer() + Offset));
}

//=========================================================================

int anim_event::SaveTypeNameString(const char* String)
{
    //ASSERT( String );
    if (nullptr == String) {
        return 0;
    }
    /*
        // First, see if we have the string saved yet
        char* CurString = (char*)m_pEventTypeNameStrings->GetBuffer();
        int   Offset = 0;

        while (Offset < m_pEventTypeNameStrings->GetLength()) {
            //ASSERT( CurString );
            if (NULL == CurString) {
                // problem -- m_EventStrings is corrupted
                return -1;
            }

            if (strcmp(CurString, String) == 0) {
                // found it
                return Offset;
            }

            const int PaddedLength = PaddedStringLength(CurString);
            Offset += PaddedLength;
            CurString += PaddedLength * sizeof(char);
        }

        // We didn't find it, add it
        Offset = m_pEventTypeNameStrings->GetLength();
        const int Length = strlen(String);
        m_pEventTypeNameStrings->Append((uint8_t*)String, Length);
        m_pEventTypeNameStrings->Append((uint8_t)'\0');

        // dword align...
        m_pEventTypeNameStrings->Append((uint8_t*)"@@@@", StringPadding(String));

        return Offset;
        */
    return 0;
}

//=========================================================================

void anim_event::ResetByteStreams()
{
    /*
    if (m_pEventByteStream) {
        delete m_pEventByteStream;
    }
    if (m_pEventTypeNameStrings) {
        delete m_pEventTypeNameStrings;
    }
    m_pEventByteStream = nullptr;
    m_pEventTypeNameStrings = nullptr;
    */
}

//=========================================================================
// returns strlen() + null terminator + padding ===> which is total mem used
int PaddedStringLength(const char* String)
{
    //ASSERT(String);
    return strlen(String) + 1 + StringPadding(String);
}

//=========================================================================

int StringPadding(const char* String)
{
    return (4 - ((strlen(String) + 1) % 4));
}
