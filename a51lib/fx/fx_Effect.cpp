
#include "fx_Mgr.h"

inline int ALIGN_16(int x)
{
    return (x + 0x0f) & ~0x0f;
}

void fx_effect_base::Initialize(const fx_def* pEffectDef)
{
    m_Scale.set(1.0f, 1.0f, 1.0f);
    m_Rotation.Set(R_0, R_0, R_0);
    m_Translation.set(0.0f, 0.0f, 0.0f);
    m_Color.set(255, 255, 255, 255);

    m_pEffectDef = pEffectDef;
    m_Flags = pEffectDef->Flags;
    m_NReferences = 0;
    m_L2WDirty = true;
    m_EL2WDirty = true;
    m_ColorDirty = true;
    m_BBoxDirty = true;
    m_BBox.Clear();
}

void fx_effect_base::GetBitmaps(int Index, const Bitmap*& pDiffuseMap,
                                const Bitmap*& pAlphaMap) const
{
    assert(Index < m_pEffectDef->NBitmaps);

    pDiffuseMap = fx_mgr::m_pResolveBitmapFn(m_pEffectDef->pDiffuseMap[Index]);
    pAlphaMap = fx_mgr::m_pResolveBitmapFn(m_pEffectDef->pAlphaMap[Index]);
}

void fx_effect_base::Render()
{
    // Render each element.
    for (int i = 0; i < m_pEffectDef->NElements; i++) {
        fx_element*           pElement = m_pElement[i];
        const fx_element_def* pElementDef = m_pEffectDef->pElementDef[i];

        if (m_ColorDirty) {
            pElement->BaseColor(m_Color);
        }

        if (m_Flags & FX_SINGLETON) {
            pElement->BaseL2W();
        }

        if ((GetAge() >= pElementDef->TimeStart) &&
            (!pElement->IsFinished(this))) {
            pElement->BaseColor();
            pElement->Render(this);
        }
    }

    m_ColorDirty = false;

    //
    // Debug rendering.
    //

#ifdef DEBUG_FX
    if (!FXDebug.EffectReserved) {
        return;
    }

    if (FXDebug.EffectCenter) {
        draw_Marker(m_Translation, XCOLOR_RED);
    }

    if (FXDebug.EffectBounds) {
        draw_BBox(GetBounds(), XCOLOR_RED);
    }

    if (FXDebug.EffectAxis || FXDebug.EffectVolume) {
        draw_SetL2W(m_L2W);
        if (FXDebug.EffectAxis) {
            draw_Axis(100.0f);
        }
        if (FXDebug.EffectVolume) {
            draw_BBox(bbox(vector3(-0.5f, -0.5f, -0.5f),
                           vector3(0.5f, 0.5f, 0.5f)),
                      XCOLOR_BLUE);
        }
        draw_ClearL2W();
    }
#endif // DEBUG_FX
}

//  FX_EFFECT_CLONE FUNCTIONS

const BBox& fx_effect_clone::GetBounds() const
{
    if (m_BBoxDirty) {
        m_BBox = m_pEffect->GetBounds();
        m_BBox.Transform(GetL2W());

        m_BBoxDirty = false;
    }

    return (m_BBox);
}

float fx_effect_clone::GetAge() const
{
    return (m_pEffect->GetAge());
}

bool fx_effect_clone::IsSuspended() const
{
    return (m_pEffect->IsSuspended());
}

bool fx_effect_clone::IsFinished() const
{
    return (m_pEffect->IsFinished());
}

bool fx_effect_clone::IsInstanced(void) const
{
    return (true);
}

void fx_effect_clone::SetSuspended(bool Suspended)
{
    m_pEffect->SetSuspended(Suspended);
}

void fx_effect_clone::AdvanceLogic(float DeltaTime)
{
    m_BBoxDirty = true;

    if (!(m_pEffect->m_Flags & FX_MASTER_LOGIC)) {
        m_pEffect->m_EL2WDirty = true;
        m_pEffect->AdvanceLogic(DeltaTime);
        m_pEffect->m_Flags |= FX_MASTER_LOGIC;
    }
}

void fx_effect_clone::Restart(void)
{
    m_pEffect->Restart();
    m_BBoxDirty = true;
    m_BBox.Clear();
}

void fx_effect_clone::Initialize(fx_effect_base* pMasterEffect,
                                 const fx_def*   pEffectDef)
{
    // Initialize the base class.
    fx_effect_base::Initialize(pEffectDef);

    // Attach to the master copy.
    m_pElement = pMasterEffect->GetElementList();
    m_pEffect = pMasterEffect;
    pMasterEffect->AddReference();
}

//  FX_EFFECT FUNCTIONS

const BBox& fx_effect::GetBounds() const
{
    if (m_BBoxDirty) {
        int i;

        m_BBox.Clear();

        for (i = 0; i < m_pEffectDef->NElements; i++) {
            m_BBox += m_pElement[i]->GetBBox();
        }

        // Check the bounding box.  If it is empty, then add the effect anchor.
        if ((m_BBox.min.x > m_BBox.max.x) &&
            (m_BBox.min.y > m_BBox.max.y) &&
            (m_BBox.min.z > m_BBox.max.z)) {
            m_BBox += m_Translation;
        }

        m_BBoxDirty = false;
    }

    return (m_BBox);
}

float fx_effect::GetAge() const
{
    return (m_Age);
}

bool fx_effect::IsSuspended() const
{
    return (m_Suspended);
}

bool fx_effect::IsFinished() const
{
    return (m_Done);
}

bool fx_effect::IsInstanced() const
{
    return (false);
}

void fx_effect::SetSuspended(bool Suspended)
{
    if (!m_Suspended && Suspended) {
        m_Suspended = true;
    }

    if (m_Suspended && !Suspended) {
        if (m_Done) {
            Restart();
        } else {
            m_Suspended = false;
        }
    }
}

void fx_effect::Initialize(const fx_def* pEffectDef)
{
    int      i;
    uint8_t* pPointer;

    // Initialize the base class.
    fx_effect_base::Initialize(pEffectDef);

    //
    // Initialize the local data members.
    //

    m_Age = 0.0f;
    m_Done = false;
    m_Suspended = false;

    //
    // Initialize the staging area, controllers, and elements.
    //

    pPointer = (uint8_t*)(this + 1); // Point to addr AFTER this object.

    // Set the staging area pointer.
    m_pStagingArea = (float*)pPointer;       // Set staging area pointer.
    pPointer += (pEffectDef->NSAValues * 4); // Step over memory for staging area.

    // Setup the controllers.

    m_pCtrl = (fx_ctrl**)(pPointer);            // Set controller array pointer.
    pPointer += (pEffectDef->NControllers * 4); // Step over memory for ctrl ptr array.
    fx_ctrl* pCtrl = (fx_ctrl*)pPointer;        // Pointer to first controller instance.

    for (i = 0; i < pEffectDef->NControllers; i++) {
        // Locals to assist.
        int TypeIndex = pEffectDef->pCtrlDef[i]->TypeIndex;

        // Construct the appropriate type.
        fx_mgr::m_CtrlType[TypeIndex].pFactoryFn(pCtrl);

        // Enter address in controller array.
        m_pCtrl[i] = pCtrl;

        // Initialize and get values from initial evaluation.
        pCtrl->Initialize(pEffectDef->pCtrlDef[i], m_pStagingArea);
        pCtrl->Evaluate(pCtrl->ComputeLogicalTime());

        // Advance walking pointer.
        pCtrl++;
    }

    pPointer = (uint8_t*)pCtrl;

    // Setup the elements.

    m_pElement = (fx_element**)pPointer;
    pPointer += (pEffectDef->NElements * 4);

    // Need to align the pointer up to a 16 multiple offset.
    {
        int Offset = pPointer - (uint8_t*)this;
        pPointer = ((uint8_t*)this) + ALIGN_16(Offset);
    }

    for (i = 0; i < pEffectDef->NElements; i++) {
        // Locals to assist.
        int         TypeIndex = pEffectDef->pElementDef[i]->TypeIndex;
        fx_element* pElement = (fx_element*)pPointer;

        // Construct the appropriate type.
        fx_mgr::m_ElementType[TypeIndex].pFactoryFn(pElement);

        // Enter address in element array.
        m_pElement[i] = pElement;

        // Initialize the element.
        pElement->Initialize(pEffectDef->pElementDef[i], m_pStagingArea);

        // Advance the walking pointer.
        pPointer += fx_mgr::m_ElementType[TypeIndex].pMemoryFn(*pEffectDef->pElementDef[i]);

        // Need to align the pointer up to a 16 multiple offset.
        {
            int Offset = pPointer - (uint8_t*)this;
            pPointer = ((uint8_t*)this) + ALIGN_16(Offset);
        }
    }
}

void fx_effect::Restart(void)
{
    int i;

    m_Age = 0.0f;
    m_Done = false;
    m_Suspended = false;
    m_BBoxDirty = true;
    m_BBox.Clear();

    for (i = 0; i < m_pEffectDef->NControllers; i++) {
        m_pCtrl[i]->Initialize(m_pEffectDef->pCtrlDef[i], m_pStagingArea);
        m_pCtrl[i]->Evaluate(m_pCtrl[i]->ComputeLogicalTime());
    }

    for (i = 0; i < m_pEffectDef->NElements; i++) {
        m_pElement[i]->Reset();
    }
}

void fx_effect::AdvanceLogic(float DeltaTime)
{
    int i;
    int Finished = 0;

    if (m_Done) {
        return;
    }

    // Clear the bounding box.  It will get rebuilt during the logic.
    m_BBoxDirty = true;

    // Advance each controller.
    for (i = 0; i < m_pEffectDef->NControllers; i++) {
        m_pCtrl[i]->AdvanceLogic(DeltaTime);
    }

    // Advance each element.
    for (i = 0; i < m_pEffectDef->NElements; i++) {
        fx_element*           pElement = m_pElement[i];
        const fx_element_def* pElementDef = m_pEffectDef->pElementDef[i];

        if (GetAge() >= pElementDef->TimeStart) {
            if (!pElement->IsFinished(this)) {
                if (m_EL2WDirty) {
                    pElement->BaseL2W();
                }

                pElement->BaseLogic();
                pElement->AdvanceLogic(this, DeltaTime);
            } else {
                Finished++;
            }
        }
    }

    m_EL2WDirty = false;

    // Advance the effect's age.
    m_Age += DeltaTime;

    // All elements are finished?
    if (Finished == m_pEffectDef->NElements) {
        m_Done = true;
    }
}

//  FORCED CONSTRUCTION FUNCTIONS

#undef new

void fx_effect::ForceConstruct(void* pAddress)
{
    new (pAddress) fx_effect;
}

void fx_effect_clone::ForceConstruct(void* pAddress)
{
    new (pAddress) fx_effect_clone;
}
