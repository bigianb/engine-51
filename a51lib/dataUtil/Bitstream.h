#pragma once
#include <cstdint>
#include "../Colour.h"
#include "../VectorMath.h"

class Bitstream
{
public:
    Bitstream();
    ~Bitstream();

    void init(int32_t DataSize);
    void init(const uint8_t* pData, int32_t DataSize);
    void kill();
    void setOwnsData(bool OwnsData);
    void grow();
    void setMaxGrowSize(int32_t MaxGrowSize);

    int32_t  getNBytes() const;
    int32_t  getNBits() const;
    int32_t  getNBytesUsed() const;
    int32_t  getNBytesFree() const;
    int32_t  getNBitsUsed() const;
    int32_t  getNBitsFree() const;
    uint8_t* getDataPtr() const;
    bool     isFull() const;
    void     clear();

    int32_t getCursor() const;
    void    setCursor(int32_t BitIndex);
    int32_t getCursorRemaining() const;

    // Jumps cursor to aligned position.
    // Remember we are talking about bits...not bytes so
    // (2) = nibble alignment because (2^2) ==  4 bits == 0.5 bytes
    // (3) = uint8_t   alignment because (2^3) ==  8 bits == 1 uint8_t
    // (4) = uint16_t    alignment because (2^4) == 16 bits == 2 bytes
    void alignCursor(int32_t PowerOfTwo);

    // Integer Helpers
    void writeU64(uint64_t Value, int32_t NBits = 64);
    void writeS32(int32_t Value, int32_t NBits = 32);
    void writeU32(uint32_t Value, int32_t NBits = 32);
    void writeS16(int16_t Value, int32_t NBits = 16);
    void writeU16(uint16_t Value, int32_t NBits = 16);
    void writeRangedS32(int32_t Value, int32_t Min, int32_t Max);
    void writeRangedU32(uint32_t Value, int32_t Min, int32_t Max);
    void writeVariableLenS32(int32_t Value);
    void writeVariableLenU32(uint32_t Value);

    void readU64(uint64_t& Value, int32_t NBits = 64) const;
    void readS32(int32_t& Value, int32_t NBits = 32) const;
    void readU32(uint32_t& Value, int32_t NBits = 32) const;
    void readS16(int16_t& Value, int32_t NBits = 16) const;
    void readU16(uint16_t& Value, int32_t NBits = 16) const;
    void readRangedS32(int32_t& Value, int32_t Min, int32_t Max) const;
    void readRangedU32(uint32_t& Value, int32_t Min, int32_t Max) const;
    void readVariableLenS32(int32_t& Value) const;
    void readVariableLenU32(uint32_t& Value) const;

    // Float helpers
    void writeF32(float Value);
    void writeRangedF32(float Value, int32_t NBits, float Min, float Max);
    void writeVariableLenF32(float Value);

    void readF32(float& Value) const;
    void readRangedF32(float& Value, int32_t NBits, float Min, float Max) const;
    void readVariableLenF32(float& Value) const;

    static void truncateRangedF32(float& Value, int32_t NBits, float Min, float Max);
    static void truncateRangedVector(Vector3& N, int32_t NBits, float Min, float Max);

    // Vectors
    void writeVector(const Vector3& N);
    void writeRangedVector(const Vector3& N, int32_t NBits, float Min, float Max);
    void writeVariableLenVector(const Vector3& N);
    void writeUnitVector(const Vector3& N, int32_t TotalBits);

    void readVector(Vector3& N) const;
    void readRangedVector(Vector3& N, int32_t NBits, float Min, float Max) const;
    void readVariableLenVector(Vector3& N) const;
    void readUnitVector(Vector3& N, int32_t TotalBits) const;

    // Quaternion
    void writeQuaternion(const Quaternion& Q);
    void readQuaternion(Quaternion& Q) const;
    /*
        // Radian3
        void writeRadian3(const radian3& Radian);
        void writeRangedRadian3(const radian3& Radian, int32_t NBits);

        void readRadian3(radian3& Radian) const;
        void readRangedRadian3(radian3& Radian, int32_t NBits) const;
    */
    // Colour
    void writeColor(Colour colour);
    void readColor(Colour& colour) const;

    // String
    void writeString(const char* pBuf);
    void readString(char* pBuf, int32_t MaxLength = 0) const;

    // Wide string
    //void writeWString(const wchar* pBuf);
    //void readWString(wchar* pBuf, int32_t MaxLength = 0) const;

    // Handles a full matrix... full precision
    void writeMatrix4(const Matrix4& M);
    void readMatrix4(Matrix4& M) const;

    // Raw bits
    void writeBits(const void* pData, int32_t NBits);
    void readBits(void* pData, int32_t NBits) const;
    bool writeFlag(bool Value);
    bool readFlag() const;
    bool readFlag(bool& Flag) const;

    // Marker support
    void writeMarker();
    void readMarker() const;

    // Overwrite control
    bool overwrite() const;
    void clearOverwrite();

    // Section support
    bool openSection(bool Flag = true);
    bool closeSection();

private:
    uint8_t* m_Data;
    int32_t  m_DataSize;
    int32_t  m_DataSizeInBits;
    int32_t  m_HighestBitWritten;
    bool     m_bOwnsData;
    int32_t  m_MaxGrowSize;

    mutable int32_t m_Cursor;
    int32_t         m_SectionCursor;
    bool            m_bOverwrite;

    void     writeRawBits(const void* pData, int32_t NBits);
    void     readRawBits(void* pData, int32_t NBits) const;
    void     writeRaw32(uint32_t Value, int32_t NBits);
    uint32_t readRaw32(int32_t NBits) const;
};

inline void Bitstream::writeS32(int32_t Value, int32_t NBits)
{
    writeRaw32(Value, NBits);
}

inline void Bitstream::writeU32(uint32_t Value, int32_t NBits)
{
    writeRaw32(Value, NBits);
}

inline void Bitstream::writeS16(int16_t Value, int32_t NBits)
{
    writeRaw32(Value, NBits);
}

inline void Bitstream::writeU16(uint16_t Value, int32_t NBits)
{
    writeRaw32(Value, NBits);
}

inline void Bitstream::writeMarker()
{
#if defined(X_DEBUG)
    writeU32(0xDEADBEEF);
#endif
}

inline void Bitstream::readU32(uint32_t& Value, int32_t NBits) const
{
    Value = readRaw32(NBits);
}

inline void Bitstream::readU16(uint16_t& Value, int32_t NBits) const
{
    Value = readRaw32(NBits);
}

inline void Bitstream::readS16(int16_t& Value, int32_t NBits) const
{
    Value = readRaw32(NBits);

    if (NBits == 16) {
        return;
    }

    // extend sign bit
    if (Value & (1 << (NBits - 1))) {
        Value |= (0xFFFF & (~((1 << NBits) - 1)));
    }
}

inline void Bitstream::readS32(int32_t& Value, int32_t NBits) const
{
    Value = readRaw32(NBits);

    if (NBits == 32) {
        return;
    }

    // extend sign bit
    if (Value & (1 << (NBits - 1))) {
        Value |= (0xFFFFFFFF & (~((1 << NBits) - 1)));
    }
}
