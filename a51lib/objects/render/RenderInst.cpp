
#include "RenderInst.h"

render_inst::render_inst()
{
    m_hInst.Handle = HNULL;
    m_Alpha = 255;

    m_FadeTime = 0.0f;
    m_FadeTimeElapsed = 0.0f;
    m_FadeDirection = 0;
}

//============================================================================+=

render_inst::~render_inst(void)
{
}

//=============================================================================

void render_inst::StartFade(int8_t Direction, float TimeToFade)
{
    // handle fading in
    if (Direction == 1) {
        // full alpha right away?
        if ((m_Alpha == 255) || (TimeToFade == 0.0f)) {
            m_Alpha = 255;
            m_FadeDirection = 0;
            m_FadeTime = 0.0f;
        } else {
            // calculate how far we should be faded already
            float T = (float)m_Alpha / 255.0f;
            m_FadeTime = TimeToFade;
            m_FadeTimeElapsed = T * TimeToFade;
            m_FadeDirection = 1;
        }
    }
    // handle fading out
    else if (Direction == -1) {
        // zero alpha right away?
        if ((m_Alpha == 0) || (TimeToFade == 0.0f)) {
            m_Alpha = 0;
            m_FadeDirection = 0;
            m_FadeTime = 0.0f;
        } else {
            // calculate how far we should be faded already
            float T = 1.0f - ((float)m_Alpha / 255.0f);
            m_FadeTime = TimeToFade;
            m_FadeTimeElapsed = T * TimeToFade;
            m_FadeDirection = -1;
        }
    }
}

//=============================================================================

void render_inst::OnAdvanceLogic(float DeltaTime)
{
    if (m_FadeDirection == -1) {
        m_FadeTimeElapsed += DeltaTime;
        if (m_FadeTimeElapsed >= m_FadeTime) {
            m_Alpha = 0;
            m_FadeTime = 0.0f;
            m_FadeTimeElapsed = 0.0f;
            m_FadeDirection = 0;
        } else {
            assert(m_FadeTime > 0.0f);
            m_Alpha = (uint8_t)(255.0f * (1.0f - (m_FadeTimeElapsed / m_FadeTime)));
        }
    } else if (m_FadeDirection == 1) {
        m_FadeTimeElapsed += DeltaTime;
        if (m_FadeTimeElapsed >= m_FadeTime) {
            m_Alpha = 255;
            m_FadeTime = 0.0f;
            m_FadeTimeElapsed = 0.0f;
            m_FadeDirection = 0;
        } else {
            assert(m_FadeTimeElapsed > 0.0f);
            m_Alpha = (uint8_t)(255.0f * (m_FadeTimeElapsed / m_FadeTime));
        }
    }
}

//=============================================================================

BBox& render_inst::GetBBox() const
{
    static BBox bb;
    bb.Clear();

    Geom* pGeom = GetGeom();

    if (pGeom) {
        return (pGeom->bbox);
    } else {
        return (bb);
    }
}

//=============================================================================

uint64_t render_inst::GetLODMask(const Matrix4& L2W)
{
    Geom*       pGeom = GetGeom();
    /*
    // Compute screen size of object
    const view* pView = eng_GetView();
    
    float       ScreenSize = pView->CalcScreenSize(L2W.GetTranslation(), pGeom->bbox.GetRadius());
*/
    float ScreenSize = 0.0f;
    // It is possible that when you get some cinematics putting weird camera angles
    // in place, the bbox position can technically be behind the camera. This is
    // because the view is at eye point, and the actor position is usually at
    // his feet. Get in close, tilt the camera up a bit, and voila. To correct
    // for this, fix up a screen size of zero returned from the view by
    // max'ing it out.
    if (ScreenSize <= 0.0f) {
        ScreenSize = 65000.0f;
    }

    // Get correct LOD Mask based on screen size
    return pGeom->GetLODMask(m_VMeshMask, (uint16_t)ScreenSize); // instead of -1, make a mask!!!!
}

//=============================================================================

int render_inst::GetNActiveBones(const uint64_t& LODMask) const
{
    // Lookup geom
    Geom* pGeom = GetGeom();
    if (!pGeom) {
        return 0;
    }

    // Count bones used by geometry and tell the animation player
    //( bones in sorted into hierarchical order so we can just keep the max )
    int16_t nActiveBones = 0;
    for (int i = 0; i < pGeom->numMeshes; i++) {
        // Is this mesh being used?
        if (LODMask & ((uint64_t)1 << i)) {
            // Update max count
            nActiveBones = std::max(nActiveBones, pGeom->meshes[i].nBones);
        }
    }

    return nActiveBones;
}

//=============================================================================

void render_inst::OnEnumProp(prop_enum& List)
{
    // enumerate the vmesh list
    int HeaderId = List.PushPath("RenderInst\\");
    m_VMeshMask.OnEnumProp(List, GetGeom());
    List.PopPath(HeaderId);
}

//=============================================================================

bool render_inst::OnProperty(prop_query& I)
{
    Geom* pGeom = GetGeom();

    if (!I.IsBasePath("RenderInst\\")) {
        return false;
    }

    // handle the vmesh list
    int HeaderId = I.PushPath("RenderInst\\");
    if (m_VMeshMask.OnProperty(I, pGeom)) {
    } else {
        I.PopPath(HeaderId);
        return false;
    }

    I.PopPath(HeaderId);
    return (true);
}
