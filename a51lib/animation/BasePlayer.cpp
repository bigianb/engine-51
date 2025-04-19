
#include "BasePlayer.h"
#include "../render/Geom.h"

AnimKey* base_player::GetMixBuffer( mix_buffer MixBuffer )
{
    // Use main memory
    static AnimKey s_MixBuffer[MAX_ANIM_BONES * MIX_BUFFER_COUNT];
    return &s_MixBuffer[MAX_ANIM_BONES * MixBuffer];
}
