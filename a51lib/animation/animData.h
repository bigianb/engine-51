#pragma once
#include <stdlib.h>
#include <cstdint>
#include <string>
#include <iostream>
#include <vector>

#include "../resourceManager/ResourceManager.h"

#include "../VectorMath.h"

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

#define STREAM_FLAG_MASKED 1

#define ANIM_DATA_FLAG_LOOPING (1 << 0)
#define ANIM_DATA_FLAG_HAS_MASKS (1 << 1)
#define ANIM_DATA_FLAG_ACCUM_HORIZ_MOTION (1 << 2)
#define ANIM_DATA_FLAG_ACCUM_VERT_MOTION (1 << 3)
#define ANIM_DATA_FLAG_ACCUM_YAW_MOTION (1 << 4)
#define ANIM_DATA_FLAG_GRAVITY (1 << 5)
#define ANIM_DATA_FLAG_WORLD_COLLISION (1 << 6)
#define ANIM_DATA_FLAG_CHAIN_CYCLES_INTEGER (1 << 7)
#define ANIM_DATA_FLAG_BLEND_FRAMES (1 << 8)
#define ANIM_DATA_FLAG_BLEND_LOOP (1 << 9)

class DataReader;
class AnimGroup;
class event_data;
class event_data_format;
struct anim_event;

struct AnimBone
{
    Matrix4 bindMatrixInv;
    Vector3 localTranslation;
    Vector3 bindTranslation;

    int16_t     iBone;
    int16_t     iParent;
    int16_t     nChildren;
    std::string name;
};

class AnimKey
{
public:
    void Interpolate(const AnimKey& Key0, const AnimKey& Key1, float T);
    void Identity(void);
    void Setup(Matrix4& M);

    Quaternion rotation;
    Vector3    translation;
};

inline void AnimKey::Interpolate(const AnimKey& Key0, const AnimKey& Key1, float T)
{
    rotation = Blend(Key0.rotation, Key1.rotation, T);
    translation = Key0.translation + T * (Key1.translation - Key0.translation);
}

class AnimKeys
{
public:
    // Only returns bone streams
    bool IsBoneMasked(const AnimGroup& AnimGroup, int iBone) const;
    void GetRawKeys(const AnimGroup& AnimGroup, int iFrame, AnimKey* pKey) const;
    void GetInterpKeys(const AnimGroup& AnimGroup, float Frame, AnimKey* pKey) const;
    void GetInterpKeys(const AnimGroup& AnimGroup, float Frame, AnimKey* pKey, int nBones) const;

    // Can return bone or prop streams
    void GetRawKey(const AnimGroup& AnimGroup, int iFrame, int iStream, AnimKey& Key) const;
    void GetInterpKey(const AnimGroup& AnimGroup, float Frame, int iStream, AnimKey& Key) const;

    short nFrames;
    short nBones;
    short nProps;
    short nKeyBlocks;
    short iKeyBlock;
};

#define EVENT_MAX_INTS 5
#define EVENT_MAX_FLOATS 8
#define EVENT_MAX_POINTS 2
#define EVENT_MAX_BOOLS 8
#define EVENT_MAX_STRINGS 5
#define EVENT_MAX_COLORS 4
#define EVENT_MAX_STRING_LENGTH 32

class event_data_format
{
public:
    event_data_format();
    int  GetNInts() const;
    int  GetNFloats() const;
    int  GetNPoints() const;
    int  GetNBools() const;
    int  GetNStrings() const;
    int  CountSetBits(int NumBitsBefore, int NumBitsPossible) const;
    void SetInt(int Idx);
    void SetFloat(int Idx);
    void SetPoint(int Idx);
    void SetBool(int Idx);
    void SetString(int Idx);

    uint32_t m_Flags;
};

class event_data
{
public:
    event_data();
    void SetType(const char* Type);
    void SetName(const char* Name);
    void StoreInt(int Idx, int Value);
    void StoreFloat(int Idx, float Value);
    void StorePoint(int Idx, const Vector3& Value);
    void StoreBool(int Idx, bool Value);
    void StoreString(int Idx, const char* String);

    int nInts() const { return m_DataFormat.GetNInts(); }
    int nFloats() const { return m_DataFormat.GetNFloats(); }
    int nPoints() const { return m_DataFormat.GetNPoints(); }
    int nBools() const { return m_DataFormat.GetNBools(); }
    int nStrings() const { return m_DataFormat.GetNStrings(); }

    const int*     Ints() const { return m_Ints; }
    const float*   Floats() const { return m_Floats; }
    const Vector3* Points() const { return m_Points; }
    const bool*    Bools() const { return m_Bools; }
    const char**   Strings() const { return (const char**)m_Strings; }
    const char*    String(int i) const { return (const char*)m_Strings[i]; }

    const char* GetType() const { return m_Type; }
    const char* GetName() const { return m_Name; }

    event_data_format GetDataFormat() const { return m_DataFormat; }
    void              SetDataFormat(event_data_format DataFormat) { m_DataFormat = DataFormat; }

    void SwitchEndian();

private:
    Vector3           m_Points[EVENT_MAX_POINTS];
    event_data_format m_DataFormat;
    int               m_Ints[EVENT_MAX_INTS];
    float             m_Floats[EVENT_MAX_FLOATS];
    bool              m_Bools[EVENT_MAX_BOOLS];
    char              m_Type[EVENT_MAX_STRING_LENGTH + 1];
    char              m_Name[EVENT_MAX_STRING_LENGTH + 1];
    char              m_Strings[EVENT_MAX_STRINGS][EVENT_MAX_STRING_LENGTH + 1];
};

enum event_types
{
    EVENT_TYPE_OLD_EVENT,
    EVENT_TYPE_DO_NOT_USE,
    EVENT_HOT_POINT,
    EVENT_TYPE_AUDIO,
    EVENT_TYPE_PARTICLE,
    EVENT_TYPE_GENERIC,
    EVENT_TYPE_INTENSITY,
    EVENT_TYPE_WORLD_COLLISION,
    EVENT_TYPE_GRAVITY,
    EVENT_TYPE_WEAPON,
    EVENT_TYPE_PAIN,
    EVENT_TYPE_DEBRIS,
    EVENT_TYPE_SET_MESH,
    EVENT_TYPE_SWAP_MESH,
    EVENT_TYPE_FADE_GEOMETRY,
    EVENT_TYPE_SWAP_TEXTURE,
    EVENT_TYPE_CAMERA_FOV,
    NUM_EVENT_TYPES
};

extern char g_EventTypes[NUM_EVENT_TYPES][EVENT_MAX_STRING_LENGTH + 1];

class AnimInfo
{
public:
    int   GetNAnims() const { return nAnims; }
    float GetAnimsWeight() const { return animsWeight; }

    const char* GetName() const { return name.c_str(); }
    float       GetWeight() const { return weight; }

    float GetBlendTime() const { return blendTime; }
    int   GetNFrames() const { return nFrames; }
    int   GetLoopFrame() const { return iLoopFrame; }
    int   GetEndFrameOffset() const { return endFrameOffset; }

    // Key
    void GetRawKey(int iFrame, int iBone, AnimKey& Key) const;
    void GetInterpKey(float Frame, int iBone, AnimKey& Key) const;
    void GetRawKeys(int iFrame, AnimKey* pKey) const;
    void GetInterpKeys(float Frame, AnimKey* pKey) const;
    void GetInterpKeys(float Frame, AnimKey* pKey, int nBones) const;

    // Misc
    Radian      GetTotalMoveDir() const { return (float)totalMoveDir * (R_360 / 65535.0f); }
    Radian      GetTotalYaw() const { return (float)totalYaw * (R_360 / 65535.0f); }
    Radian      GetHandleAngle() const { return (float)handleAngle * (R_360 / 65535.0f); }
    Vector3     GetTotalTranslation() const { return totalTranslation; }
    const BBox& GetBBox() const { return bbox; }
    int         GetFPS() const { return fps; }
    float       GetSpeed() const;
    float       GetYawRate() const;

    // Flags
    bool DoesLoop() const { return ((flags & ANIM_DATA_FLAG_LOOPING) != 0); }
    bool HasMasks() const { return ((flags & ANIM_DATA_FLAG_HAS_MASKS) != 0); }
    bool AccumHorizMotion() const { return ((flags & ANIM_DATA_FLAG_ACCUM_HORIZ_MOTION) != 0); }
    bool AccumVertMotion() const { return ((flags & ANIM_DATA_FLAG_ACCUM_VERT_MOTION) != 0); }
    bool AccumYawMotion() const { return ((flags & ANIM_DATA_FLAG_ACCUM_YAW_MOTION) != 0); }
    bool Gravity() const { return ((flags & ANIM_DATA_FLAG_GRAVITY) != 0); }
    bool WorldCollision() const { return ((flags & ANIM_DATA_FLAG_WORLD_COLLISION) != 0); }
    bool ChainCyclesInteger() const { return ((flags & ANIM_DATA_FLAG_CHAIN_CYCLES_INTEGER) != 0); }
    bool BlendFrames() const { return ((flags & ANIM_DATA_FLAG_BLEND_FRAMES) != 0); }
    bool BlendLoop() const { return ((flags & ANIM_DATA_FLAG_BLEND_LOOP) != 0); }
    bool IsBoneMasked(int iBone) const { return animKeys.IsBoneMasked(*parentGroup, iBone); }

    // Chain
    int GetChainFramesMin() const { return nChainFramesMin; }
    int GetChainFramesMax() const { return nChainFramesMax; }
    int GetChainAnim() const { return iChainAnim; }
    int GetChainFrame() const { return iChainFrame; }

    // Animated bone info
    int GetAnimBoneMinIndex() const { return iAnimBoneMin; }
    int GetAnimBoneMaxIndex() const { return iAnimBoneMax; }

    // Props
    int  GetNProps() const { return nProps; }
    int  GetPropChannel(const char* pChannelName) const;
    void GetPropRawKey(int iChannel, int iFrame, AnimKey& Key) const;
    void GetPropInterpKey(int iChannel, float Frame, AnimKey& Key) const;
    int  GetPropParentBoneIndex(int iChannel) const
    {
        return -1; // IJB parentGroup->m_pProp[m_iProp + iChannel].m_iBone;
    }

    // Events
    int         GetNEvents() const { return nEvents; }
    anim_event& GetEvent(int eventIdx) const;
    bool        IsEventActive(int eventIdx, float Frame) const;
    bool        IsEventActive(int eventIdx, float CurrFrame, float PrevFrame) const;
    bool        IsEventTypeActive(int Type, float Frame) const;
    bool        IsEventTypeActive(int Type, float CurrFrame, float PrevFrame) const;
    void        SetEventData(int eventIdx, const event_data& ED);
    float       FindLipSyncEventStartFrame() const;

public:
    AnimGroup* parentGroup;

    Vector3     totalTranslation; // Total movement
    BBox        bbox;             // BBox of all verts pushed thu anim
    int         nAnims;
    float       animsWeight;
    std::string name;
    float       weight;
    float       blendTime;

    short nChainFramesMin;
    short nChainFramesMax;
    short iChainAnim;
    short iChainFrame;

    short iAnimBoneMin;
    short iAnimBoneMax;

    short nFrames;
    short iLoopFrame;
    short endFrameOffset;

    short nEvents;
    short iEvent;

    short nProps;
    short iProp;

    uint16_t handleAngle;
    uint16_t totalYaw;
    uint16_t totalMoveDir;

    uint16_t flags;
    uint16_t fps;

    AnimKeys animKeys;
};

struct anim_event
{
    //-------------------------------------------------------------------------
public:
    anim_event();
    ~anim_event();

    //-------------------------------------------------------------------------
public:
    //
    // Old event members (will be removed)
    //
    /*

    s16             m_Type;
    s16             m_iBone;
    s16             m_iFrame0;
    s16             m_iFrame1;
    float             m_Radius;
    Vector3         m_Offset;
    */

public:
    //
    // These are all indices into the flags (INT_BLAH = X would be the Xth int)
    //
    enum int_idxs
    {
        // Standard ints
        INT_IDX_START_FRAME = 0,
        INT_IDX_END_FRAME,
        INT_IDX_BONE,

        INT_IDX_FIRST_CUSTOM,

        // Old style events
        INT_IDX_OLD_TYPE = INT_IDX_FIRST_CUSTOM,

        // Effect events
        INT_IDX_EFFECT_GUID_INDEX = INT_IDX_FIRST_CUSTOM,

        // Looped audio events
        INT_IDX_AUDIO_DATA = INT_IDX_FIRST_CUSTOM,

        // Looped audio events
        INT_IDX_WEAPON_DATA = INT_IDX_FIRST_CUSTOM,

        // Pain events
        INT_IDX_PAIN_TYPE = INT_IDX_FIRST_CUSTOM,

        // Fade Geometry events
        INT_IDX_FADE_DIRECTION = INT_IDX_FIRST_CUSTOM,
    };

    enum float_idxs
    {
        // Standard floats
        FLOAT_IDX_RADIUS = 0,
        FLOAT_IDX_FIRST_CUSTOM,

        // Intensity events.
        FLOAT_IDX_CONTROLLER_INTENSITY = FLOAT_IDX_FIRST_CUSTOM,
        FLOAT_IDX_CONTROLLER_DURATION,
        FLOAT_IDX_CAMERA_SHAKE_TIME,
        FLOAT_IDX_CAMERA_SHAKE_AMOUNT,
        FLOAT_IDX_CAMERA_SHAKE_SPEED,
        FLOAT_IDX_CAMERA_INTENSITY,
        FLOAT_IDX_BLUR_INTENSITY,
        FLOAT_IDX_BLUR_DURATION,

        FLOAT_IDX_DEBRIS_MIN_VELOCITY = FLOAT_IDX_FIRST_CUSTOM,
        FLOAT_IDX_DEBRIS_MAX_VELOCITY,
        FLOAT_IDX_DEBRIS_LIFE,

        // geometry fading events
        FLOAT_IDX_GEOMETRY_FADE_TIME = FLOAT_IDX_FIRST_CUSTOM,

        FLOAT_IDX_CAMERA_FOV = FLOAT_IDX_FIRST_CUSTOM,
        FLOAT_IDX_CAMERA_FOV_TIME,
    };

    enum point_idxs
    {
        // Standard points
        POINT_IDX_OFFSET = 0,
        POINT_IDX_ROTATION,
        POINT_IDX_FIRST_CUSTOM,
    };

    enum bool_idxs
    {
        BOOL_IDX_FIRST_CUSTOM = 0,

        BOOL_IDX_WORLD_COLLISION = BOOL_IDX_FIRST_CUSTOM,

        BOOL_IDX_GRAVITY = BOOL_IDX_FIRST_CUSTOM,

        BOOL_IDX_PARTICLE_EVENT_ACTIVE = BOOL_IDX_FIRST_CUSTOM,
        BOOL_IDX_PARTICLE_DONOT_APPLY_TRANSFORM,

        BOOL_IDX_DEBRIS_BOUNCE = BOOL_IDX_FIRST_CUSTOM,
    };

    enum string_idxs
    {
        STRING_IDX_FIRST_CUSTOM = 0,

        // Effects strings
        STRING_IDX_EFFECT_RESOURCE = STRING_IDX_FIRST_CUSTOM,

        // Projectile strings
        STRING_IDX_PARTICLE_TYPE = STRING_IDX_FIRST_CUSTOM,

        // One-Shot Audio strings
        STRING_IDX_AUDIO_SOUND_ID = STRING_IDX_FIRST_CUSTOM,
        STRING_IDX_AUDIO_LOCATION,
        STRING_IDX_AUDIO_TYPE,

        // HotPoint
        STRING_IDX_HOTPOINT_TYPE = STRING_IDX_FIRST_CUSTOM,

        // Generic strings
        STRING_IDX_GENERIC_TYPE = STRING_IDX_FIRST_CUSTOM,

        // Generic strings
        STRING_IDX_PAIN_TYPE = STRING_IDX_FIRST_CUSTOM,

        // Rigid instance.
        STRING_IDX_DEBRIS_TYPE = STRING_IDX_FIRST_CUSTOM,

        // Set Mesh
        STRING_IDX_SET_MESH = STRING_IDX_FIRST_CUSTOM,
        STRING_IDX_SET_MESH_ON_OR_OFF,

        // Swap Mesh
        STRING_IDX_SWAP_MESH_ON = STRING_IDX_FIRST_CUSTOM,
        STRING_IDX_SWAP_MESH_OFF,

        // Swap Virtual Texture
        STRING_IDX_SET_TEXTURE = STRING_IDX_FIRST_CUSTOM,
        STRING_IDX_USE_TEXTURE,

    };

    static void    Init();
    void           SetData(const event_data& Data);
    event_data     GetData() const;
    const char*    GetType() const;
    const char*    GetName() const;
    int            StartFrame() const { return GetInt(INT_IDX_START_FRAME); }
    int            EndFrame() const { return GetInt(INT_IDX_END_FRAME); }
    int            GetNInts() const { return m_DataFormat.GetNInts(); }
    int            GetNFloats() const { return m_DataFormat.GetNFloats(); }
    int            GetNPoints() const { return m_DataFormat.GetNPoints(); }
    int            GetNBools() const { return m_DataFormat.GetNBools(); }
    int            GetNStrings() const { return m_DataFormat.GetNStrings(); }
    uint32_t       GetDataFlags() const { return m_DataFormat.m_Flags; }
    const uint8_t* GetDataBuffer() const;

    // in these methods, "Idx" refers to the index
    // within that specific type, so GetString(4)
    // gets the 4th string
    int         GetInt(int Idx) const;
    float       GetFloat(int Idx) const;
    Vector3     GetPoint(int Idx) const;
    bool        GetBool(int Idx) const;
    const char* GetString(int Idx) const;

    //static const uint8_t*	GetStreamBuffer     () { return m_pEventByteStream->GetBuffer(); }
    //static const uint8_t*  GetTypeNameBuffer   () { return m_pEventTypeNameStrings->GetBuffer(); }
    //static int          GetStreamLength     () { return m_pEventByteStream->GetLength(); }
    //static int          GetTypeNameLength   () { return m_pEventTypeNameStrings->GetLength(); }

    void SwitchEndian();

    static void ResetByteStreams();

private:
    int m_ByteStreamDataOffset;
    int m_TypeOffset;
    int m_NameOffset;

    event_data_format m_DataFormat;

    static const char* GetTypeNameString(int Offset);
    static int         SaveTypeNameString(const char* String);

    //static xbytestream* m_pEventByteStream;
    //static xbytestream* m_pEventTypeNameStrings;
};

//=========================================================================
// CLASS ANIM_PROP
//=========================================================================

struct anim_prop
{
    //-------------------------------------------------------------------------
public:
    anim_prop();
    ~anim_prop();

    //-------------------------------------------------------------------------
public:
    char m_Type[32];
    int  m_iBone;
};

class AnimKeyStream
{
public:
    void     setOffset(uint32_t);
    uint32_t getOffset() const;

    //  2 bits for the scale format = 4 formats
    //  2 bits for the rot   format = 4 formats
    //  2 bits for the trans format = 4 formats
    //  8 bits for bone flag bits   = 8 custom flags per bone per anim
    // 18 bits for the offset to the scale data = 0-262143 range
    //-----
    // 32

    uint32_t Offset;

public:
    void grabKey(const uint8_t* data, int totalFrames, int frame, AnimKey& Key) const;
    void GetInterpKey(uint8_t* pData, int nFrames, int iFrame, float T, AnimKey& Key) const;
    uint32_t GetFlags() const;
};

class AnimKeyBlock
{
public:
    AnimKeyBlock()
    {
        next = nullptr;
        prev = nullptr;
        stream = nullptr;
        factoredCompressedData = nullptr;
    }

    ~AnimKeyBlock()
    {
        if (stream != nullptr) {
            free(stream);
            stream = nullptr;
        }
    }

    void grabKey(int frame, int streamIdx, AnimKey& Key) const;

    AnimKeyBlock*  next;
    AnimKeyBlock*  prev;
    AnimKeyStream* stream; // Points to decompressed data if available
    uint32_t       checksum;
    uint8_t*       factoredCompressedData;
    int            compressedDataOffset; // Offset into compressed data for this key set
    int            nFrames;
    int            decompressedDataSize;
};

/**
 * An AnimGroup contains a number of animations for a single skeleton,
 * each defined by an AnimInfo object.
 * Each AnimInfo points to a list of AnimKeyBlock objects.
 */
class AnimGroup
{
public:
    bool readFile(uint8_t* fileData, int len);
    void describe(std::ostream& ss);

    int GetNAnims() const { return numAnims; }

    int GetAnimIndex(const char* pAnimName) const;
    int GetRandomAnimIndex(const char* pAnimName, int iSkipAnim = -1) const;
    int GetRandomAnimIndex(int iStartAnim, int iSkipAnim = -1) const;

    const AnimInfo& GetAnimInfo(int iAnim) const { return anims[iAnim]; }
    void            GetL2W(const Matrix4& L2W, float Frame, int iAnim, Matrix4* pBoneL2W) const;

    // Skeleton related calls
    const AnimBone& GetBone(int idx) const { return bones[idx]; }
    int             GetNBones() const { return numBones; }
    int             GetBoneIndex(const char* pBoneName, bool FindAnywhere = false) const;
    int             GetBoneParent(int iBone) const { return bones[iBone].iParent; }
    void            ComputeBoneL2W(int iBone, const Matrix4& L2W, AnimKey* pKey, Matrix4& BoneL2W) const;
    void            ComputeBonesL2W(const Matrix4& L2W, AnimKey* pKey, int nBones, Matrix4* BoneL2W, bool bApplyTheBindPose = true) const;
    const Matrix4&  GetBoneBindInvMatrix(int iBone) const { return bones[iBone].bindMatrixInv; }
    Vector3         GetEventPos(int iBone, const Vector3& Offset, AnimKey* pKey) const;
    const BBox&     GetBBox() const { return bbox; }
    Radian3         GetEventRot(int iBone, const Vector3& Offset, AnimKey* pKey) const;

    void getRawKey(int animNo, int frame, int iStream, AnimKey& key) const;

private:
    void readBone(DataReader& reader, AnimBone& bone);
    void readAnim(DataReader& reader, AnimInfo& info);
    void readKeyBlock(DataReader& reader, AnimKeyBlock& keyBlock, int compressedDataStartOffset);

    void describeAnimKeys(std::ostream& ss, int animNo);

public:
    BBox        bbox;
    std::string filename;
    int         version;

    int totalNFrames;
    int totalNKeys;

    int numBones;
    int numAnims;
    int numProps;
    int numEvents;
    int numKeyBlocks;

    std::vector<anim_event>   events;
    std::vector<AnimBone>     bones;
    std::vector<AnimInfo>     anims;
    std::vector<AnimKeyBlock> keyBlocks;

    typedef ResourceHandle<AnimGroup> handle;
};

inline anim_event& AnimInfo::GetEvent(int eventIdx) const
{
    return parentGroup->events[iEvent + eventIdx];
}
