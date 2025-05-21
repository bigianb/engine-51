
#include "animData.h"

#include "../dataUtil/Bitstream.h"

//
// Based on anim_key_formats
//
//=========================================================================
//                                            C    S  16   32
static int s_ScaleFormatOverhead[] = {0, 12, 24, 0};
static int s_ScaleFormatSize[] = {0, 0, 2, 12};
static int s_RotationFormatOverhead[] = {0, 16, 0, 0};
static int s_RotationFormatSize[] = {0, 0, 8, 16};

//=========================================================================

static AnimKeyBlock* s_pMRUBlock = nullptr;
static AnimKeyBlock* s_pLRUBlock = nullptr;
static int           s_nBlocksDecompressed = 0;
static int           s_nBlockBytesDecompressed = 0;
static int           s_MaxAllowedDecompressedBytes = 300 * 1024;

//=========================================================================
/*
int     AnimKeyStream::s_SF;
int     AnimKeyStream::s_RF;
int     AnimKeyStream::s_TF;
int     AnimKeyStream::s_SO;
int     AnimKeyStream::s_RO;
int     AnimKeyStream::s_TO;
uint8_t*   AnimKeyStream::s_pData;
*/
//=========================================================================

extern void AnimationDecompress(const AnimGroup& AG,
                                const uint8_t*   pCompressedData,
                                AnimKeyStream*   pStream,
                                int              DecompressedSize);

//=========================================================================

void anim_SetMaxAllowedDecompressedBytes(int NBytes)
{
    s_MaxAllowedDecompressedBytes = NBytes;
}

void AnimKey::Identity()
{
    translation.Zero();
    rotation.identity();
}

//=========================================================================

void AnimKey::Setup(Matrix4& M)
{
    // Fill out 3x3 rotations.
    float tx = 2.0f * rotation.x; // 2x
    float ty = 2.0f * rotation.y; // 2y
    float tz = 2.0f * rotation.z; // 2z
    float txw = tx * rotation.w;  // 2x * w
    float tyw = ty * rotation.w;  // 2y * w
    float tzw = tz * rotation.w;  // 2z * w
    float txx = tx * rotation.x;  // 2x * x
    float tyx = ty * rotation.x;  // 2y * x
    float tzx = tz * rotation.x;  // 2z * x
    float tyy = ty * rotation.y;  // 2y * y
    float tzy = tz * rotation.y;  // 2z * y
    float tzz = tz * rotation.z;  // 2z * z
    M(0, 0) = 1.0f - (tyy + tzz);
    M(0, 1) = tyx + tzw;
    M(0, 2) = tzx - tyw;
    M(1, 0) = tyx - tzw;
    M(1, 1) = 1.0f - (txx + tzz);
    M(1, 2) = tzy + txw;
    M(2, 0) = tzx + tyw;
    M(2, 1) = tzy - txw;
    M(2, 2) = 1.0f - (txx + tyy);

    // Fill out translation
    M.SetTranslation(translation);

    // Fill out last column
    M(0, 3) = 0.0f;
    M(1, 3) = 0.0f;
    M(2, 3) = 0.0f;
    M(3, 3) = 1.0f;
}

/*
void AnimKeyStream::GetOffsetsAndFormats   (  int  nFrames,
                                                int& SO,
                                                int& RO,
                                                int& TO,
                                                anim_key_format& SF,
                                                anim_key_format& RF,
                                                anim_key_format& TF ) const
{
    SF = (anim_key_format)((Offset >> STREAM_SCL_SHIFT) & STREAM_SCL_MASK);
    RF = (anim_key_format)((Offset >> STREAM_ROT_SHIFT) & STREAM_ROT_MASK);
    TF = (anim_key_format)((Offset >> STREAM_TRS_SHIFT) & STREAM_TRS_MASK);
    SO = (Offset >> STREAM_OFT_SHIFT) & STREAM_OFT_MASK;
    RO = SO + s_ScaleFormatOverhead[SF]     + s_ScaleFormatSize[SF]*nFrames;
    TO = RO + s_RotationFormatOverhead[RF]  + s_RotationFormatSize[RF]*nFrames;
}
*/
//=========================================================================
/*
inline void AnimKeyStream::GrabKey( int iFrame, AnimKey& Key )
{
    // Decompress rotation
    {
        if( s_RF == PRECISION_16 )
        {
            // I'm using temp variables to tell the compiler that pR doesn't
            // point to Key.rotation so it can do the math out of order.
            u16* pR = &((u16*)(s_pData + s_RO))[ iFrame<<2 ];
            float TempX = ((float)pR[0] * (2.0f / 65535.0f)) - 1.0f;
            float TempY = ((float)pR[1] * (2.0f / 65535.0f)) - 1.0f;
            float TempZ = ((float)pR[2] * (2.0f / 65535.0f)) - 1.0f;
            float TempW = ((float)pR[3] * (2.0f / 65535.0f)) - 1.0f;
            Key.rotation.x = TempX;
            Key.rotation.y = TempY;
            Key.rotation.z = TempZ;
            Key.rotation.w = TempW;
        }
        else
        if( s_RF == CONSTANT_VALUE )
        {
            Key.rotation.Identity();
        }
        else
        if( s_RF == SINGLE_VALUE )
        {
            Key.rotation = ((quaternion*)(s_pData + s_RO))[ 0 ];
        }
        else
        if( s_RF == PRECISION_32 )
        {
            Key.rotation = ((quaternion*)(s_pData + s_RO))[ iFrame ];
        }
        else
        {
            assert( false );
        }
    }

    // Decompress translation
    {
        if( s_TF == CONSTANT_VALUE )
        {
            Key.Translation.Set(0.0f,0.0f,0.0f);// = vector3(0,0,0);
        }
        else
        if( s_TF == SINGLE_VALUE )
        {
            Key.Translation = ((vector3p*)(s_pData + s_TO))[ 0 ];
        }
        else
        if( s_TF == PRECISION_32 )
        {
            Key.Translation = ((vector3p*)(s_pData + s_TO))[ iFrame ];
        }
        else
        {
            assert(false);
        }
    }
}
*/
//=========================================================================
/*
void AnimKeyStream::GetRawKey( uint8_t* pData, int nFrames, int iFrame, AnimKey& Key )
{
    s_SF = (Offset >> STREAM_SCL_SHIFT) & STREAM_SCL_MASK;
    s_RF = (Offset >> STREAM_ROT_SHIFT) & STREAM_ROT_MASK;
    s_TF = (Offset >> STREAM_TRS_SHIFT) & STREAM_TRS_MASK;
    s_SO = (Offset >> STREAM_OFT_SHIFT) & STREAM_OFT_MASK;
    s_RO = s_SO + s_ScaleFormatOverhead[s_SF]     + s_ScaleFormatSize[s_SF]*nFrames;
    s_TO = s_RO + s_RotationFormatOverhead[s_RF]  + s_RotationFormatSize[s_RF]*nFrames;
    s_pData = pData;

    GrabKey( iFrame, Key );
}
*/
//=========================================================================
/*
void AnimKeyStream::GetInterpKey( uint8_t* pData, int nFrames, int iFrame, float T, AnimKey& Key )
{
    assert( iFrame < nFrames-1 );

    s_pData = pData;
    s_SF = (Offset >> STREAM_SCL_SHIFT) & STREAM_SCL_MASK;
    s_RF = (Offset >> STREAM_ROT_SHIFT) & STREAM_ROT_MASK;
    s_TF = (Offset >> STREAM_TRS_SHIFT) & STREAM_TRS_MASK;
    s_SO = (Offset >> STREAM_OFT_SHIFT) & STREAM_OFT_MASK;
    s_RO = s_SO + s_ScaleFormatOverhead[s_SF]     + s_ScaleFormatSize[s_SF]*nFrames;
    s_TO = s_RO + s_RotationFormatOverhead[s_RF]  + s_RotationFormatSize[s_RF]*nFrames;
    AnimKey K0;
    AnimKey K1;

    GrabKey( iFrame+0, K0 );
    GrabKey( iFrame+1, K1 );

    Key.Interpolate( K0, K1, T );
}
*/
//=========================================================================
/*
u32 AnimKeyStream::GetFlags( void ) const
{
    return (Offset >> STREAM_FLG_SHIFT) & STREAM_FLG_MASK;
}
*/
//=========================================================================
/*
void AnimKeyStream::SetFlags( u32 Flags )
{
    // Clear current flags
    Offset &= ~(STREAM_FLG_MASK<<STREAM_FLG_SHIFT);

    // Write new flags
    Offset |= (Flags<<STREAM_FLG_SHIFT);
}
*/
//=========================================================================
/*
void AnimKeyStream::SetFormats( anim_key_format SF, anim_key_format RF, anim_key_format TF )
{
    // Clear current formats
    Offset &= ~(STREAM_SCL_MASK<<STREAM_SCL_SHIFT);
    Offset &= ~(STREAM_ROT_MASK<<STREAM_ROT_SHIFT);
    Offset &= ~(STREAM_TRS_MASK<<STREAM_TRS_SHIFT);

    // Write new formats
    Offset |= ((u32)SF<<STREAM_SCL_SHIFT);
    Offset |= ((u32)RF<<STREAM_ROT_SHIFT);
    Offset |= ((u32)TF<<STREAM_TRS_SHIFT);
}

//=========================================================================

void AnimKeyStream::SetOffset( int ScaleOffset )
{
    // Clear current offset
    Offset &= ~(STREAM_OFT_MASK<<STREAM_OFT_SHIFT);

    // Write new offset
    Offset |= (ScaleOffset<<STREAM_OFT_SHIFT);
}
*/

/*
void AnimKeyBlock::AttachToList(  )
{
    // Add to beginning of list
    pNext = s_pMRUBlock;
    pPrev = nullptr;
    if( s_pMRUBlock ) s_pMRUBlock->pPrev = this;
    if( s_pLRUBlock == nullptr ) s_pLRUBlock = this;
    s_pMRUBlock = this;
}

//=========================================================================

void AnimKeyBlock::DetachFromList(  )
{
    // Remove from list
    if( pNext ) pNext->pPrev = pPrev;
    if( pPrev ) pPrev->pNext = pNext;
    if( s_pMRUBlock == this ) s_pMRUBlock = pNext;
    if( s_pLRUBlock == this ) s_pLRUBlock = pPrev;
    pNext = nullptr;
    pPrev = nullptr;
}

//=========================================================================

AnimKeyStream* AnimKeyBlock::AcquireStreams( const AnimGroup& AG )
{
    MEMORY_OWNER( "ANIMATION CACHE" );
    // Be sure we can fit this decompressed data into the cache
    assert( DecompressedDataSize <= s_MaxAllowedDecompressedBytes );

    // Move block to beginning of list
    DetachFromList();
    AttachToList();

    // Check if we've already decompressed
    if( pStream )
        return pStream;

    // Limit the maximum number of decompressed bytes
    while( (s_nBlockBytesDecompressed+DecompressedDataSize) > s_MaxAllowedDecompressedBytes )
    {
        assert( s_pLRUBlock != this );
        s_pLRUBlock->ReleaseStreams();

        // Force us to hold at least one
        if( s_pMRUBlock == nullptr )
            break;
    }

    // Increment number of decompressed bytes
    s_nBlockBytesDecompressed += DecompressedDataSize;
    s_nBlocksDecompressed++;

    // Allocate destination of decompressed data
    pStream = (AnimKeyStream*)malloc( DecompressedDataSize );
    assert( pStream );

    xtimer Timer;
    Timer.Start();

    // Kick off decompression
    AnimationDecompress( AG,
                         //AG.GetCompressedDataPtr()+CompressedDataOffset,
                         pFactoredCompressedData,
                         pStream,
                         DecompressedDataSize );

    Timer.Stop();

    //x_DebugMsg("DecompressedBlock: (%1.3f ms) (%d bytes) (%d blocks) (%d bytes total) <%s>\n",
    //    Timer.ReadMs(), DecompressedDataSize, s_nBlocksDecompressed, s_nBlockBytesDecompressed, AG.GetFileName() );

    return pStream;
}

void AnimKeyBlock::ReleaseStreams(  )
{
    if( pStream == nullptr )
        return;

    // Detach from list
    DetachFromList();

    // Deallocate decompressed data
    x_free( pStream );
    pStream = nullptr;

    // Decrement number of decompressed bytes
    s_nBlockBytesDecompressed -= DecompressedDataSize;
    s_nBlocksDecompressed--;
}
*/

bool AnimKeys::IsBoneMasked(const AnimGroup& animGroup, int iBone) const
{
    assert((iBone >= 0) && (iBone < nBones));
    const AnimKeyBlock&  KeyBlock = animGroup.keyBlocks[iKeyBlock];
    AnimKeyStream* pStream = KeyBlock.stream;
    return (pStream[iBone].GetFlags() & STREAM_FLAG_MASKED) == STREAM_FLAG_MASKED;
}

//=========================================================================

void AnimKeys::GetRawKey(const AnimGroup& animGroup, int iFrame, int iStream, AnimKey& Key) const
{
    assert((iStream >= 0) && (iStream < (nBones + nProps)));

    int iBlock = iFrame >> MAX_KEYS_PER_BLOCK_SHIFT;
    int iBlockFrame = iFrame & MAX_KEYS_PER_BLOCK_MASK;
    if ((iBlock == nKeyBlocks) && (iBlockFrame == 0)) {
        iBlock--;
        iBlockFrame = MAX_KEYS_PER_BLOCK;
    }
    assert((iBlock >= 0) && (iBlock < nKeyBlocks));

    const AnimKeyBlock&  KeyBlock = animGroup.keyBlocks[iKeyBlock + iBlock];
    AnimKeyStream* pStream = KeyBlock.stream;

    pStream[iStream].grabKey((uint8_t*)pStream, KeyBlock.nFrames, iBlockFrame, Key);
}

//=========================================================================

void AnimKeys::GetInterpKey(const AnimGroup& animGroup, float Frame, int iStream, AnimKey& Key) const
{
    assert((iStream >= 0) && (iStream < (nBones + nProps)));

    int   iFrame = (int)Frame;
    float fFrac = Frame - (float)iFrame;
    int   iBlock = iFrame >> MAX_KEYS_PER_BLOCK_SHIFT;
    int   iBlockFrame = iFrame & MAX_KEYS_PER_BLOCK_MASK;
    if ((iBlock == nKeyBlocks) && (iBlockFrame == 0)) {
        iBlock--;
        iBlockFrame = MAX_KEYS_PER_BLOCK;
    }
    assert((iBlock >= 0) && (iBlock < nKeyBlocks));

    const AnimKeyBlock&  KeyBlock = animGroup.keyBlocks[iKeyBlock + iBlock];
    AnimKeyStream* pStream = KeyBlock.stream;

    pStream[iStream].GetInterpKey((uint8_t*)pStream, KeyBlock.nFrames, iBlockFrame, fFrac, Key);
}

//=========================================================================

void AnimKeys::GetRawKeys(const AnimGroup& animGroup, int iFrame, AnimKey* pKey) const
{
    int iBlock = iFrame >> MAX_KEYS_PER_BLOCK_SHIFT;
    int iBlockFrame = iFrame & MAX_KEYS_PER_BLOCK_MASK;
    if ((iBlock == nKeyBlocks) && (iBlockFrame == 0)) {
        iBlock--;
        iBlockFrame = MAX_KEYS_PER_BLOCK;
    }
    assert((iBlock >= 0) && (iBlock < nKeyBlocks));

    const AnimKeyBlock&  KeyBlock = animGroup.keyBlocks[iKeyBlock + iBlock];
    AnimKeyStream* pStream = KeyBlock.stream;

    for (int i = 0; i < nBones; i++) {
        pStream[i].grabKey((uint8_t*)pStream, KeyBlock.nFrames, iBlockFrame, pKey[i]);
    }
}

//=========================================================================

void AnimKeys::GetInterpKeys(const AnimGroup& animGroup, float Frame, AnimKey* pKey) const
{
    int   iFrame = (int)Frame;
    float fFrac = Frame - (float)iFrame;
    int   iBlock = iFrame >> MAX_KEYS_PER_BLOCK_SHIFT;
    int   iBlockFrame = iFrame & MAX_KEYS_PER_BLOCK_MASK;
    if ((iBlock == nKeyBlocks) && (iBlockFrame == 0)) {
        iBlock--;
        iBlockFrame = MAX_KEYS_PER_BLOCK;
    }
    assert((iBlock >= 0) && (iBlock < nKeyBlocks));

    const AnimKeyBlock&  KeyBlock = animGroup.keyBlocks[iKeyBlock + iBlock];
    AnimKeyStream* pStream = KeyBlock.stream;

    for (int i = 0; i < nBones; i++) {
        pStream[i].GetInterpKey((uint8_t*)pStream, KeyBlock.nFrames, iBlockFrame, fFrac, pKey[i]);
    }
}

//=========================================================================

void AnimKeys::GetInterpKeys(const AnimGroup& animGroup, float Frame, AnimKey* pKey, int nBones) const
{
    int   iFrame = (int)Frame;
    float fFrac = Frame - (float)iFrame;
    int   iBlock = iFrame >> MAX_KEYS_PER_BLOCK_SHIFT;
    int   iBlockFrame = iFrame & MAX_KEYS_PER_BLOCK_MASK;
    if ((iBlock == nKeyBlocks) && (iBlockFrame == 0)) {
        iBlock--;
        iBlockFrame = MAX_KEYS_PER_BLOCK;
    }
    assert((iBlock >= 0) && (iBlock < nKeyBlocks));

    const AnimKeyBlock&  KeyBlock = animGroup.keyBlocks[iKeyBlock + iBlock];
    AnimKeyStream* pStream = KeyBlock.stream;

    assert(nBones <= nBones);
    for (int i = 0; i < nBones; i++) {
        pStream[i].GetInterpKey((uint8_t*)pStream, KeyBlock.nFrames, iBlockFrame, fFrac, pKey[i]);
    }
}
