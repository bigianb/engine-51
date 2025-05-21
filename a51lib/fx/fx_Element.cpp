
#include "fx_Mgr.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MINMAX(a, v, b) (MAX((a), MIN((v), (b))))

int DefaultElementMemoryFn(const fx_element_def& ElementDef)
{
    return (sizeof(fx_element));
}

void fx_element::Initialize(const fx_element_def* pElementDef,
                            float*                pInput)
{
    // Initialize the fields from arguments.
    m_pElementDef = pElementDef;
    m_pInput = pInput;

    // Install controller values OR default values.

    float* pWrite = (float*)(&m_Scale);
    float* pRead = (float*)(&m_pElementDef->Scale);
    int*   pOffset = (int*)(m_pElementDef->CtrlOffsets);

    for (int i = 0; i < 13; i++) {
        if (pOffset[i] == -1) {
            pWrite[i] = pRead[i];
        } else {
            pWrite[i] = m_pInput[pOffset[i]];
        }
    }

    // Color my world.
    m_BaseColor.set(255, 255, 255, 255);
    m_ColorDirty = true;

    // The bounds are local by default.  Particular elements can override.
    m_BBox.Clear();
    m_LocalDirty = true;
}

void fx_element::BaseLogic()
{
    int    i;
    float* pWrite = (float*)(&m_Scale);

    // Install controller values.
    for (i = 0; i < 13; i++) {
        if (m_pElementDef->CtrlOffsets[i] != -1) {
            if (i < 9) {
                m_LocalDirty = true;
            } else {
                m_ColorDirty = true;
            }

            pWrite[i] = m_pInput[m_pElementDef->CtrlOffsets[i]];
        }
    }
}

void fx_element::AdvanceLogic(const fx_effect_base* pEffect, float DeltaTime)
{
}

void fx_element::ApplyColor(void)
{
    float FloatColor[4];

    // Enforce the float color range.
    // (Smooth controlled color can get out of range.)
    FloatColor[0] = MINMAX(0.0f, m_FloatColor[0], 1.0f);
    FloatColor[1] = MINMAX(0.0f, m_FloatColor[1], 1.0f);
    FloatColor[2] = MINMAX(0.0f, m_FloatColor[2], 1.0f);
    FloatColor[3] = MINMAX(0.0f, m_FloatColor[3], 1.0f);

    // Installed combination of local element color and parent effect color.
    m_Color.r = (uint8_t)(m_BaseColor.r * FloatColor[0]);
    m_Color.g = (uint8_t)(m_BaseColor.g * FloatColor[1]);
    m_Color.b = (uint8_t)(m_BaseColor.b * FloatColor[2]);
    m_Color.a = (uint8_t)(m_BaseColor.a * FloatColor[3]);
}

void fx_element::Render(const fx_effect_base* pEffect) const
{
}

bool fx_element::IsFinished(const fx_effect_base* pEffect) const
{
    return ((pEffect->IsSuspended()) ||
            (pEffect->GetAge() >= m_pElementDef->TimeStop));
}

void fx_element::Reset()
{
    m_LocalDirty = true;
}
