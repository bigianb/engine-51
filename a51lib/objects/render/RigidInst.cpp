
#include "RigidInst.h"
#include "../../resourceManager/ResourceManager.h"

rigid_inst::rigid_inst(ResourceManager* rm)
    : render_inst()
    , m_hRigidGeom(rm)
    , m_pRigidColor(nullptr)
    , m_nColors(0)
    , m_iColor(0)
{
}

rigid_inst::~rigid_inst()
{
    if (m_hInst.IsNonNull()) {
        render::UnregisterRigidInstance(m_hInst);
    }
}

int rigid_inst::GetNumColors() const
{
    return (m_nColors);
}

const void* rigid_inst::GetColorTable(void) const
{
    if (!m_pRigidColor) {
        return nullptr;
    }

    uint16_t* pCol = (uint16_t*)m_pRigidColor;
    return (pCol + m_iColor);
}

void rigid_inst::SetColorTable(const void* pColorTable, int iColor, int nColors)
{
    m_pRigidColor = pColorTable;
    m_iColor = iColor;
    m_nColors = nColors;
}


void rigid_inst::LoadColorTable(const char* pFileName, ResourceManager* rm)
{
    ResourceHandle<color_info> hRigidColor(rm);
    hRigidColor.setName(pFileName);

    color_info* pInfo = hRigidColor.getPointer();
    if (pInfo) {
        m_pRigidColor = *pInfo;
    } else {
        m_pRigidColor = nullptr;
    }
}

std::string rigid_inst::GetRigidGeomName() const
{
    return m_hRigidGeom.getName();
}

void rigid_inst::Render(const Matrix4* pL2W, uint32_t Flags, uint64_t Mask)
{
    if (m_Alpha == 0) {
        return;
    }

    if (m_Alpha != 255) {
        Flags |= render::FADING_ALPHA;
    }

    // Add a Rigid Instance
    render::AddRigidInstance(m_hInst,
                             GetColorTable(),
                             pL2W,
                             Mask,
                             Flags,
                             m_Alpha);
}

void rigid_inst::Render(const Matrix4* pL2W, uint32_t Flags, uint64_t Mask, uint8_t Alpha)
{
    if (Alpha == 0) {
        return;
    }

    if (Alpha != 255) {
        Flags |= render::FADING_ALPHA;
    }

    // Add a Rigid Instance
    render::AddRigidInstance(m_hInst,
                             GetColorTable(),
                             pL2W,
                             Mask,
                             Flags,
                             Alpha);
}

void rigid_inst::Render(const Matrix4* pL2W, uint32_t Flags, uint32_t VTextureMask, int Alpha)
{
    if (Alpha == 0) {
        return;
    }

    if (Alpha != 255) {
        Flags |= render::FADING_ALPHA;
    }

    // Add a Rigid Instance
    render::AddRigidInstance(m_hInst,
                             GetColorTable(),
                             pL2W,
                             GetLODMask(*pL2W),
                             VTextureMask,
                             Flags,
                             Alpha);
}

void rigid_inst::Render(const Matrix4* pL2W, uint32_t Flags)
{
    // Add a Rigid Instance
    Render(pL2W, Flags, GetLODMask(*pL2W));
}

void rigid_inst::OnEnumProp(prop_enum& List)
{
    // Important: The Header and External MUST be enumerated first!
    List.PropEnumHeader("RenderInst", "Render Instance", 0);
    List.PropEnumExternal("RenderInst\\File", "Resource\0rigidgeom", "Resource File", PROP_TYPE_MUST_ENUM);

    render_inst::OnEnumProp(List);

    List.PropEnumInt("RenderInst\\iColor", "iColor", PROP_TYPE_INT | PROP_TYPE_DONT_SHOW);
    List.PropEnumInt("RenderInst\\nColors", "nColors", PROP_TYPE_INT | PROP_TYPE_DONT_SHOW);
}

bool rigid_inst::OnProperty(prop_query& I)
{
    static uint32_t Count = 0;
    Count++;
    if (render_inst::OnProperty(I)) {
        return true;
    }

    // External
    if (I.IsVar("RenderInst\\File")) {
        if (I.IsRead()) {
            I.SetVarExternal(m_hRigidGeom.getName().c_str(), RESOURCE_NAME_SIZE);
        } else {
            // Get the FileName
            const char* pString = I.GetVarExternal();

            // Clear?
            if (strcmp(pString, "<null>") == 0) {
                SetUpRigidGeom("");
            } else if (pString[0]) {
                // Setup
                SetUpRigidGeom(pString);
            }

            m_VMeshMask.VMeshMask = 0xffffffff;
        }
        return true;
    }

    if (I.VarInt("RenderInst\\iColor", m_iColor)) {
        return true;
    }

    if (I.VarInt("RenderInst\\nColors", m_nColors)) {
        return true;
    }

    return false;
}

bool rigid_inst::SetUpRigidGeom(const char* pFileName)
{

    if (m_hInst.IsNonNull()) {
        render::UnregisterRigidInstance(m_hInst);
        m_hInst = HNULL;
    }

    m_hRigidGeom.setName(pFileName);
    RigidGeom* pRigidGeom = m_hRigidGeom.getPointer();

    if (pRigidGeom) {
        // Register the instance with the Render Manager
        m_hInst = render::RegisterRigidInstance(*pRigidGeom, m_hRigidGeom.resourceManager);
        return true;
    }

    return false;
}
