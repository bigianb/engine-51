#include "animData.h"
#include "../dataUtil/Bitstream.h"

#include <cassert>

#define MAX_KEYS_PER_BLOCK_SHIFT (5)
#define MAX_KEYS_PER_BLOCK (1 << MAX_KEYS_PER_BLOCK_SHIFT)
#define MAX_KEYS_PER_BLOCK_MASK ((1 << MAX_KEYS_PER_BLOCK_SHIFT) - 1)

#define STREAM_SCL_NBITS (2)  // 4 scale formats
#define STREAM_ROT_NBITS (2)  // 4 rot formats
#define STREAM_TRS_NBITS (2)  // 4 trans formats
#define STREAM_FLG_NBITS (8)  // 8 stream flags
#define STREAM_OFT_NBITS (18) // 256k offset

#define STREAM_SCL_SHIFT (32 - STREAM_SCL_NBITS)
#define STREAM_ROT_SHIFT (STREAM_SCL_SHIFT - STREAM_ROT_NBITS)
#define STREAM_TRS_SHIFT (STREAM_ROT_SHIFT - STREAM_TRS_NBITS)
#define STREAM_FLG_SHIFT (STREAM_TRS_SHIFT - STREAM_FLG_NBITS)
#define STREAM_OFT_SHIFT (STREAM_FLG_SHIFT - STREAM_OFT_NBITS)

#define STREAM_SCL_MASK ((1 << STREAM_SCL_NBITS) - 1)
#define STREAM_ROT_MASK ((1 << STREAM_ROT_NBITS) - 1)
#define STREAM_TRS_MASK ((1 << STREAM_TRS_NBITS) - 1)
#define STREAM_FLG_MASK ((1 << STREAM_FLG_NBITS) - 1)
#define STREAM_OFT_MASK ((1 << STREAM_OFT_NBITS) - 1)

typedef void decomp_fn(Bitstream& BS, const AnimData& AG, int iStream, int nFrames, uint8_t*& pData);

//=========================================================================

void Decomp_Const(Bitstream&, const AnimData&, int, int, uint8_t*&)
{
    // nothing to do
}

void DeltaDecompress(Bitstream& BS, float* pSample, int Stride, int nSamples, float Prec)
{
    const float oneOverPrec = 1.0f / Prec;

    int32_t firstSample;
    BS.readVariableLenS32(firstSample);
    int32_t minDelta;
    BS.readVariableLenS32(minDelta);
    uint32_t nDeltaBits;
    BS.readU32(nDeltaBits, 5);

    // Create first sample
    pSample[0] = firstSample * oneOverPrec;

    // Create other samples
    for (int i = 1; i < nSamples; i++) {
        uint32_t D;
        BS.readU32(D, nDeltaBits);
        pSample[i * Stride] = pSample[(i - 1) * Stride] + (((int32_t)D + minDelta) * oneOverPrec);
    }
}

void DecompS_Delta(Bitstream& BS, const AnimData&, int, int nFrames, uint8_t*& pData)
{
    DeltaDecompress(BS, &((Vector3p*)pData)->x, 3, nFrames, 128.0f);
    DeltaDecompress(BS, &((Vector3p*)pData)->y, 3, nFrames, 128.0f);
    DeltaDecompress(BS, &((Vector3p*)pData)->z, 3, nFrames, 128.0f);
    pData += sizeof(Vector3p) * nFrames;
}

void DecompT_Delta(Bitstream& BS, const AnimData&, int, int nFrames, uint8_t*& pData)
{
    DeltaDecompress(BS, &((Vector3p*)pData)->x, 3, nFrames, 128.0f);
    DeltaDecompress(BS, &((Vector3p*)pData)->y, 3, nFrames, 128.0f);
    DeltaDecompress(BS, &((Vector3p*)pData)->z, 3, nFrames, 128.0f);
    pData += sizeof(Vector3p) * nFrames;
}

void DeltaDecompressQ(Bitstream& BS, uint16_t* pSample, int stride, int nSamples, float Prec)
{
    const float oneOverPrec = 1.0f / Prec;

    int32_t  firstSample;
    int32_t  minDelta;
    uint32_t nDeltaBits;
    BS.readVariableLenS32(firstSample);
    BS.readVariableLenS32(minDelta);
    BS.readU32(nDeltaBits, 5);

    // Create first sample
    float s = firstSample * oneOverPrec;
    assert((s >= -1.0f) && (s <= 1.0f));
    *pSample = (uint16_t)((s + 1.0f) * 0.5f * 65535.0f);
    pSample += stride;
    float prevSample = s;
    // Create other samples
    for (int i = 1; i < nSamples; i++) {
        uint32_t d;
        BS.readU32(d, nDeltaBits);

        s = prevSample + (((int)d + minDelta) * oneOverPrec);
        assert((s >= -1.0f) && (s <= 1.0f));
        *pSample = (uint16_t)((s + 1.0f) * 0.5f * 65535.0f);
        prevSample = s;

        pSample += stride;
    }
}

void DecompQ_Delta(Bitstream& BS, const AnimData&, int, int nFrames, uint8_t*& pData)
{
    DeltaDecompressQ(BS, &((uint16_t*)pData)[0], 4, nFrames, 2048.0f);
    DeltaDecompressQ(BS, &((uint16_t*)pData)[1], 4, nFrames, 2048.0f);
    DeltaDecompressQ(BS, &((uint16_t*)pData)[2], 4, nFrames, 2048.0f);
    DeltaDecompressQ(BS, &((uint16_t*)pData)[3], 4, nFrames, 2048.0f);

    pData += 8 * nFrames;
}

void DecompQ_Delta2(Bitstream& BS, const AnimData&, int, int nFrames, uint8_t*& pData)
{
    DeltaDecompressQ(BS, &((uint16_t*)pData)[0], 4, nFrames, (float)(1 << 14));
    DeltaDecompressQ(BS, &((uint16_t*)pData)[1], 4, nFrames, (float)(1 << 14));
    DeltaDecompressQ(BS, &((uint16_t*)pData)[2], 4, nFrames, (float)(1 << 14));
    DeltaDecompressQ(BS, &((uint16_t*)pData)[3], 4, nFrames, (float)(1 << 14));

    pData += 8 * nFrames;
}
void DecompQ_Delta3(Bitstream& BS, const AnimData&, int, int nFrames, uint8_t*& pData)
{
    DeltaDecompressQ(BS, &((uint16_t*)pData)[0], 4, nFrames, (float)(1 << 16));
    DeltaDecompressQ(BS, &((uint16_t*)pData)[1], 4, nFrames, (float)(1 << 16));
    DeltaDecompressQ(BS, &((uint16_t*)pData)[2], 4, nFrames, (float)(1 << 16));
    DeltaDecompressQ(BS, &((uint16_t*)pData)[3], 4, nFrames, (float)(1 << 16));

    pData += 8 * nFrames;
}

void DecompS_Single(Bitstream& BS, const AnimData&, int, int, uint8_t*& pData)
{
    int32_t iX, iY, iZ;
    BS.readVariableLenS32(iX);
    BS.readVariableLenS32(iY);
    BS.readVariableLenS32(iZ);

    Vector3p v;
    v.x = iX / 128.0f;
    v.y = iY / 128.0f;
    v.z = iZ / 128.0f;

    *((Vector3p*)pData) = v;
    pData += sizeof(Vector3p);
}

void DecompQ_Single(Bitstream& BS, const AnimData&, int, int, uint8_t*& pData)
{
    int32_t iX, iY, iZ, iW;
    BS.readVariableLenS32(iX);
    BS.readVariableLenS32(iY);
    BS.readVariableLenS32(iZ);
    BS.readVariableLenS32(iW);

    Quaternion q;
    q.x = iX / 2048.0f;
    q.y = iY / 2048.0f;
    q.z = iZ / 2048.0f;
    q.w = iW / 2048.0f;

    *((Quaternion*)pData) = q;
    pData += sizeof(Quaternion);
}
void DecompT_Single(Bitstream& BS, const AnimData&, int, int, uint8_t*& pData)
{
    int32_t iX, iY, iZ;
    BS.readVariableLenS32(iX);
    BS.readVariableLenS32(iY);
    BS.readVariableLenS32(iZ);

    Vector3p v;
    v.x = iX / 128.0f;
    v.y = iY / 128.0f;
    v.z = iZ / 128.0f;

    *((Vector3p*)pData) = v;
    pData += sizeof(Vector3p);
}

void DecompV_32(Bitstream& BS, const AnimData&, int, int nFrames, uint8_t*& pData)
{
    // Reads x, y, z, w but only writes x, y, z
    float* pf = (float*)pData;
    for (int i = 0; i < nFrames; i++) {
        Vector3 v;
        BS.readVector(v);

        *pf++ = v.x;
        *pf++ = v.y;
        *pf++ = v.z;
        pData += 12;
    }
}

void DecompQ_32(Bitstream& BS, const AnimData&, int, int nFrames, uint8_t*& pData)
{
    Quaternion* pq = (Quaternion*)pData;

    for (int i = 0; i < nFrames; i++) {
        Quaternion q;
        BS.readQuaternion(q);
        *pq++ = q;
    }
    pData += nFrames * sizeof(Quaternion);
}

void DecompT_LocalTrans(Bitstream&, const AnimData& AG, int iStream, int, uint8_t*& pData)
{
    assert(iStream < AG.bones.size());
    const Vector3& v = AG.bones.at(iStream).localTranslation;
    float*         pf = (float*)pData;
    *pf++ = v.x;
    *pf++ = v.y;
    *pf++ = v.z;
    pData += 12;
}

//=========================================================================

decomp_fn* s_ScaleDecompFnptr[] =
    {
        Decomp_Const,
        DecompS_Single,
        DecompS_Delta,
        DecompV_32,
};

//=========================================================================

decomp_fn* s_RotationDecompFnptr[] =
    {
        Decomp_Const,
        DecompQ_Single,
        DecompQ_Delta,
        DecompQ_32,
        DecompQ_Delta2,
        DecompQ_Delta3,
};

//=========================================================================

decomp_fn* s_TranslationDecompFnptr[] =
    {
        Decomp_Const,
        DecompT_LocalTrans,
        DecompT_Single,
        DecompT_Delta,
        DecompV_32,
};

void RLEDecompress(Bitstream& BS, uint32_t* pData, int& nSamples)
{
    uint32_t nBitsPerCount;
    uint32_t nBitsPerSample;
    uint32_t C;
    uint32_t V;

    //
    // Unpack bitcounts from bitstream
    //
    BS.readRangedU32(nBitsPerCount, 3, 8);
    BS.readRangedU32(nBitsPerSample, 0, 32);

    //
    // Loop until all samples are decompressed
    //
    nSamples = 0;
    while (1) {
        // Read Count
        BS.readU32(C, nBitsPerCount);

        // Increment total samples
        nSamples += C;

        // If we hit terminator break out
        if (C == 0) {
            break;
        }

        // Read Value and duplicate
        BS.readU32(V, nBitsPerSample);
        while (C--) {
            *pData++ = V;
        }
    }
}

void RLEDecompressOffsetInfo(Bitstream& BS, AnimKeyStream* stream, uint32_t Mask, uint32_t Shift)
{
    uint32_t nBitsPerCount;
    uint32_t nBitsPerSample;
    uint32_t C;
    uint32_t V;

    //
    // Unpack bitcounts from bitstream
    //
    BS.readRangedU32(nBitsPerCount, 3, 8);
    BS.readRangedU32(nBitsPerSample, 0, 32);

    //
    // Loop until all samples are decompressed
    //
    while (1) {
        // Read Count
        BS.readU32(C, nBitsPerCount);

        // If we hit terminator break out
        if (C == 0) {
            break;
        }

        // Read Value and duplicate
        BS.readU32(V, nBitsPerSample);

        while (C--) {
            stream->Offset &= ~(Mask << Shift);
            stream->Offset |= (V & Mask) << Shift;
            stream++;
        }
    }
}

void AnimationDecompress(const AnimData& animData, const uint8_t* compressedData, AnimKeyStream* stream, int DecompressedSize)
{
    Bitstream BS;
    BS.init((uint8_t*)compressedData, 1024 * 1024);

    uint32_t headerCursor;
    BS.readU32(headerCursor);
    uint32_t keyDataCursor = BS.getCursor();

    BS.setCursor(headerCursor);

    uint32_t TotalStreams;
    uint32_t nFrames;
    BS.readU32(TotalStreams, 10);
    BS.readRangedU32(nFrames, 2, MAX_KEYS_PER_BLOCK + 1);

    // Decompress rle info into stream structures
    RLEDecompressOffsetInfo(BS, stream, STREAM_SCL_MASK, STREAM_SCL_SHIFT);
    RLEDecompressOffsetInfo(BS, stream, STREAM_ROT_MASK, STREAM_ROT_SHIFT);
    RLEDecompressOffsetInfo(BS, stream, STREAM_TRS_MASK, STREAM_TRS_SHIFT);
    RLEDecompressOffsetInfo(BS, stream, STREAM_FLG_MASK, STREAM_FLG_SHIFT);

    //
    // Decompress iDecompressor
    //
    int       nStreams = 0;
    uint32_t* pSDIndex = (uint32_t*)malloc(sizeof(uint32_t) * TotalStreams);
    uint32_t* pRDIndex = (uint32_t*)malloc(sizeof(uint32_t) * TotalStreams);
    uint32_t* pTDIndex = (uint32_t*)malloc(sizeof(uint32_t) * TotalStreams);
    RLEDecompress(BS, pSDIndex, nStreams);
    RLEDecompress(BS, pRDIndex, nStreams);
    RLEDecompress(BS, pTDIndex, nStreams);

    //
    // Jump back to compressed key data
    //
    BS.setCursor(keyDataCursor);

    //
    // Initialize ptr to the destination keyframe data
    //
    uint8_t* pData = (uint8_t*)stream + sizeof(AnimKeyStream) * TotalStreams;

    //
    // Read in the individual streams
    //
    for (uint32_t i = 0; i < TotalStreams; i++) {
        // Be sure we haven't overun the decompressed area
        assert(pData <= ((uint8_t*)stream + DecompressedSize));

        // Compute offset for this stream
        stream[i].Offset = (pData - (uint8_t*)stream);

        //
        // Call the decompressors for S,R,T
        // These functions advance the pData pointer
        //
        s_ScaleDecompFnptr[pSDIndex[i]](BS, animData, i, nFrames, pData);
        s_RotationDecompFnptr[pRDIndex[i]](BS, animData, i, nFrames, pData);
        s_TranslationDecompFnptr[pTDIndex[i]](BS, animData, i, nFrames, pData);
    }

    //
    // Verify we decompressed into the size we expected
    //
    assert(pData == ((uint8_t*)stream + DecompressedSize));

    free(pSDIndex);
    free(pRDIndex);
    free(pTDIndex);
}
