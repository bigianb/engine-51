

#include "fx_Mgr.h"

#include "../Bitmap.h"
#include "../xfiles/x_plus.h"
#include "../xfiles/xfs.h"

// #include "x_context.hpp"
// #include "e_Draw.hpp"
// #include "e_VRAM.hpp"
// #include "Entropy.hpp"

// #include "Auxiliary/Bitmap/aux_Bitmap.hpp"

fx_mgr FXMgr;

fx_load_bitmap_fn*    fx_mgr::m_pLoadBitmapFn = nullptr;
fx_unload_bitmap_fn*  fx_mgr::m_pUnloadBitmapFn = nullptr;
fx_resolve_bitmap_fn* fx_mgr::m_pResolveBitmapFn = nullptr;

int          fx_mgr::m_NCtrlTypes = 0;
fx_ctrl_type fx_mgr::m_CtrlType[MAX_CTRL_TYPES];

int             fx_mgr::m_NElementTypes = 0;
fx_element_type fx_mgr::m_ElementType[MAX_ELEMENT_TYPES];

int fx_mgr::m_SpriteBudget = INT_MAX;
int fx_mgr::m_SpritesThisFrame = 0;

//==============================================================================

static char s_LastDirectory[1024] = ".";
static BBox s_DefaultBounds;

bool DefaultLoad(const char* pBitmapName,
                 xhandle&    Handle)
{
    xfs XFS("%s%s", s_LastDirectory, pBitmapName);
/* IJB
    Bitmap* pBitmap = new Bitmap;

    if (pBitmap->Load((const char*)XFS)) {
        vram_Register( *pBitmap );
        Handle.Handle = (int)pBitmap;
        return (true);
    } else {
        delete pBitmap;
        return (false);
    }
*/
    return false;
}

void DefaultUnload(xhandle Handle)
{
}

//==============================================================================

const Bitmap* DefaultResolve(xhandle Handle)
{
    return reinterpret_cast<Bitmap*&>(Handle);
}

//==============================================================================
//  REGISTRATION FUNCTIONS
//==============================================================================

fx_ctrl_reg::fx_ctrl_reg(const char* pName, fx_ctrl_ctor_fn* pFactoryFn)
{
    strcpy(fx_mgr::m_CtrlType[fx_mgr::m_NCtrlTypes].Name, pName);

    for (int i = 0; i < fx_mgr::m_NCtrlTypes; i++) {
        // Cannot register the same name twice.
        assert(x_stricmp(pName, fx_mgr::m_CtrlType[i].Name));
    }

    fx_mgr::m_CtrlType[fx_mgr::m_NCtrlTypes].pFactoryFn = pFactoryFn;

    fx_mgr::m_NCtrlTypes += 1;
}

//==============================================================================

fx_element_reg::fx_element_reg(const char*           pName,
                               fx_element_ctor_fn*   pFactoryFn,
                               fx_element_memory_fn* pMemoryFn)
{
    strcpy(fx_mgr::m_ElementType[fx_mgr::m_NElementTypes].Name, pName);

    for (int i = 0; i < fx_mgr::m_NElementTypes; i++) {
        // Cannot register the same name twice.
        assert(x_stricmp(pName, fx_mgr::m_ElementType[i].Name));
    }

    fx_mgr::m_ElementType[fx_mgr::m_NElementTypes].pFactoryFn = pFactoryFn;
    fx_mgr::m_ElementType[fx_mgr::m_NElementTypes].pMemoryFn = pMemoryFn;

    fx_mgr::m_NElementTypes += 1;
}
// extern int fx_Plane;
// extern int fx_Sprite;
// extern int fx_Sphere;
// extern int fx_Cylinder;
// extern int fx_SPEmitter;
// extern int fx_ShockWave;
// extern int fx_LinearKeyCtrl;
// extern int fx_SmoothKeyCtrl;

fx_mgr::fx_mgr()
{
    int i;

    // No instances.
    for (i = 0; i < MAX_EFFECT_INSTANCES; i++) {
        m_pEffect[i] = nullptr;
    }

    // No definitions.
    for (i = 0; i < MAX_EFFECT_DEFINITIONS; i++) {
        m_pEffectDef[i] = nullptr;
    }

    m_pLoadBitmapFn = DefaultLoad;
    m_pUnloadBitmapFn = DefaultUnload;
    m_pResolveBitmapFn = DefaultResolve;

    s_DefaultBounds.Clear();

    // Damn linker!
    // fx_Plane = 0;
    // fx_Sprite = 0;
    // fx_Sphere = 0;
    // fx_Cylinder = 0;
    // fx_SPEmitter = 0;
    // fx_ShockWave = 0;
    // fx_LinearKeyCtrl = 0;
    // fx_SmoothKeyCtrl = 0;
}

//==============================================================================

fx_mgr::~fx_mgr()
{
}

//==============================================================================

void fx_mgr::BindHandle(fx_handle& Handle, int Index)
{
    UnbindHandle(Handle);

    if (m_pEffect[Index] != nullptr) {
        Handle.Index = Index;
        m_pEffect[Index]->AddReference();
        assert(m_pEffect[Index]->GetReferences() >= 0);
    } else {
        // Attempting to bind to an non-existant effect.
        assert(false);
    }
}

//==============================================================================

void fx_mgr::UnbindHandle(fx_handle& Handle)
{
    int Index = Handle.Index;

    if ((IN_RANGE(0, Index, MAX_EFFECT_INSTANCES - 1)) &&
        (m_pEffect[Index] != nullptr)) {
        m_pEffect[Index]->RemoveReference();
        assert(m_pEffect[Index]->GetReferences() >= 0);

        if (m_pEffect[Index]->GetReferences() == 0) {
            KillEffect(Index);
        }
    }

    Handle.Index = -1;
}

//==============================================================================

void fx_mgr::SetBitmapFns(fx_load_bitmap_fn*    pLoad,
                          fx_unload_bitmap_fn*  pUnload,
                          fx_resolve_bitmap_fn* pResolve)
{
    assert(pLoad != nullptr);
    assert(pUnload != nullptr);
    assert(pResolve != nullptr);

    m_pLoadBitmapFn = pLoad;
    m_pUnloadBitmapFn = pUnload;
    m_pResolveBitmapFn = pResolve;
}

//==============================================================================

void fx_mgr::SetSpriteBudget(int MaxSprites)
{
    m_SpriteBudget = MaxSprites;
}

//==============================================================================

int fx_mgr::GetSpriteCount(void)
{
    return (m_SpritesThisFrame);
}

//==============================================================================

void fx_mgr::EndOfFrame(void)
{
    int i;

    // Clear logic bits for master copies.
    for (i = 0; i < MAX_EFFECT_INSTANCES; i++) {
        // Is there an effect in this slot?
        if ((m_pEffect[i])) {
            // Get the flags
            uint32_t Flags = m_pEffect[i]->m_Flags;

            // Check for deferred deletion
            if (Flags & FX_DEFERRED_DELETE) {
                // Increment the counter and do the delete
                if (m_pEffect[i]->m_NReferences++ > 2) {
                    free(m_pEffect[i]);
                    m_pEffect[i] = nullptr;
                }
                continue;
            }

            // Clear the logic ran flag
            if ((Flags & FX_MASTER_COPY) &&
                (Flags & FX_MASTER_LOGIC)) {
                m_pEffect[i]->m_Flags &= ~FX_MASTER_LOGIC;
            }
        }
    }

    // Clear count of particles this frame.
    m_SpritesThisFrame = 0;
}

//==============================================================================

bool fx_mgr::LoadEffect(const char* pEffectName, const char* pFileName)
{
    /* IJB
        // Save off last directory.
        {
            char  Drive[ X_MAX_DRIVE ];
            char  Dir  [ X_MAX_DIR   ];

            x_splitpath( pFileName, Drive, Dir, nullptr, nullptr );
            x_makepath( s_LastDirectory, Drive, Dir, nullptr, nullptr );
        }

        bool Result;

        X_FILE* pFile = x_fopen( pFileName, "rb" );
        if( !pFile )
            return( false );

        Result = LoadEffect( pEffectName, pFile );
        x_fclose( pFile );
        return( Result );
        */
    return false;
}

/* IJB*
bool fx_mgr::LoadEffect( const char* pEffectName, X_FILE* pFile )
{
    CONTEXT( "fx_mgr::LoadEffect(char*,X_FILE*)" );
    MEMORY_OWNER( "EFFECT DATA" );

    int     Size;
    int     Read;
    int     i;
    int     Index;
    char*   pNameData;

    assert( pFile );

    //
    // Make sure that there are no other effects using the given EffectName
    // already loaded.  And, find a slot to put the effect in.
    //

    Index = -1;

    for( i = 0; i < MAX_EFFECT_DEFINITIONS; i++ )
    {
        if( m_pEffectDef[i] )
        {
            // If you get the following assertion failure, then you have
            // attempted to load two effects and give the same logical name.
            assert( x_stricmp( pEffectName, m_pEffectDef[i]->pEffectName ) );
        }
        else
        {
            // Got an index yet?
            if( Index == -1 )
                Index = i;
        }
    }

    //
    // Read the entire effect definition.
    //
    // The "base" size is given.  Increase that by the size of the name.
    // We are going to "append" the effect name onto the end of the data.
    //
    {
        int   Magic;
        char* pMagic = (char*)&Magic;
        pMagic[0] = 'F';
        pMagic[1] = 'X';
        pMagic[2] = '0';
        pMagic[3] = '2';

        // Read in the magic number.
        Read = x_fread( &Size, 4, 1, pFile );
        if( (Size != Magic) &&
            (Size != (int)ENDIAN_SWAP_32(Magic)) )
        {
            assert( false );    // File version mismatch.
            return( false );
        }

        // Read memory size (in u32s) of effect definition.
        Read = x_fread( &Size, 4, 1, pFile );
        assert( Read == 1 );

        // Endian swap.
        #ifdef TARGET_GCN
        Size = ENDIAN_SWAP_32( Size );
        #endif

        // Allocate memory.
        {
            MEMORY_OWNER( "EFFECT DEFINITION" );
            m_pEffectDef[Index] = (fx_def*)x_malloc( (Size * 4) + (x_strlen(pEffectName) + 1) );
            assert( m_pEffectDef[Index] );
            m_pEffectDef[Index]->TotalSize = Size;
        }

        // Read the effect definition data.
        Read = x_fread( ((int*)m_pEffectDef[Index])+1, 4, Size-1, pFile );
        assert( Read == (Size-1) );
        assert( Size == m_pEffectDef[Index]->TotalSize );

        // Endian fixup.
        #ifdef TARGET_GCN
        uint32_t* pData = (uint32_t*)m_pEffectDef[Index];
        for( i = 0; i < Size; i++ )
            pData[i] = ENDIAN_SWAP_32( pData[i] );
        #endif

        // Append the effect name.
        m_pEffectDef[Index]->pEffectName = ((char*)m_pEffectDef[Index]) + (Size * 4);
        x_strcpy( m_pEffectDef[Index]->pEffectName, pEffectName );
    }

    //
    // TO DO: React to magic number here.
    //
    {
    }

    //
    // TO DO: Endian fixup.
    //
    {
    }

    //
    // Pointer fixups for controllers, elements, and bitmaps, within effect
    // definition.
    //
    {
        int* pData;

        // Start data pointer immediately after the fixed portion of the effect
        // definition.
        pData = (int*)(m_pEffectDef[Index] + 1);

        // Set the pCtrlDef pointer.
        m_pEffectDef[Index]->pCtrlDef = (fx_ctrl_def**)(pData);
        pData += m_pEffectDef[Index]->NControllers;

        // Set the pElementDef pointer.
        m_pEffectDef[Index]->pElementDef = (fx_element_def**)(pData);
        pData += m_pEffectDef[Index]->NElements;

        // Set the pBmpDiffuse pointer.
        m_pEffectDef[Index]->pDiffuseMap = (xhandle*)(pData);
        pData += m_pEffectDef[Index]->NBitmaps;

        // Set the pBmpAlpha pointer.
        m_pEffectDef[Index]->pAlphaMap = (xhandle*)(pData);
        pData += m_pEffectDef[Index]->NBitmaps;

        // Populate the controller definition pointer array.
        for( i = 0; i < m_pEffectDef[Index]->NControllers; i++ )
        {
            m_pEffectDef[Index]->pCtrlDef[i] = (fx_ctrl_def*)pData;
            pData += ((fx_ctrl_def*)pData)->TotalSize;
        }

        // Populate the element definition pointer array.
        for( i = 0; i < m_pEffectDef[Index]->NElements; i++ )
        {
            m_pEffectDef[Index]->pElementDef[i] = (fx_element_def*)pData;
            pData += ((fx_element_def*)pData)->TotalSize;
        }

        // Sanity check.
        assert( (pData - (int*)m_pEffectDef[Index]) == Size );
    }

    //
    // Read in the string data which follows the numeric data.
    //
    {
        // Read memory size of string data at end of file.
        Read = x_fread( &Size, 4, 1, pFile );
        assert( Read == 1 );

        // Endian swap.
        #ifdef TARGET_GCN
        Size = ENDIAN_SWAP_32( Size );
        #endif

        // Allocate memory for the strings.
        pNameData = (char*)x_malloc( Size );
        assert( pNameData );

        // Read in the strings in a single block.
        Read = x_fread( pNameData, 1, Size, pFile );
        assert( Read == Size );
    }

    //
    // Attach the controllers and elements to their registered types.  Then load
    // the bitmaps.
    //
    {
        int   j;
        char* pNameCursor = pNameData;
        char* pCustomName;

        // For each controller, search for its registered type by name.
        for( i = 0; i < m_pEffectDef[Index]->NControllers; i++ )
        {
            for( j = 0; j < m_NCtrlTypes; j++ )
            {
            //  if( s_CtrlType[j].Name == pNameCursor )
                if( x_stricmp( m_CtrlType[j].Name, pNameCursor ) == 0 )
                {
                    m_pEffectDef[Index]->pCtrlDef[i]->TypeIndex = j;
                    break;
                }
            }
            assert( j < m_NCtrlTypes );
            while( *pNameCursor )
                pNameCursor++;
            pNameCursor++;  // Skip the '\0'.
        }

        // For each element, search for its registered type by name.
        for( i = 0; i < m_pEffectDef[Index]->NElements; i++ )
        {
            try_again:

            // Watch for custom element types.
            pCustomName = x_strchr( pNameCursor, '~' );

            // Search for the name in the registered element types.
            for( j = 0; j < m_NElementTypes; j++ )
            {
            //  if( s_ElementType[j].Name == pNameCursor )
                if( x_stricmp( m_ElementType[j].Name, pNameCursor ) == 0 )
                {
                    m_pEffectDef[Index]->pElementDef[i]->TypeIndex = j;
                    break;
                }
            }

            // If not type isn't found, and the type name has a '~' (which
            // indicates a "custom" type), then try again using just the
            // "base" type name.
            if( (j == m_NElementTypes) && (pCustomName) )
            {
                pNameCursor = pCustomName + 1;
                goto try_again;
            }

            assert( j < m_NElementTypes );
            while( *pNameCursor )
                pNameCursor++;
            pNameCursor++;  // Skip the '\0'.
        }

        // Load each bitmap.
        for( i = 0; i < m_pEffectDef[Index]->NBitmaps; i++ )
        {
            // Load diffuse texture.
            m_pLoadBitmapFn( pNameCursor, m_pEffectDef[Index]->pDiffuseMap[i] );

            // Skip the name.
            while( *pNameCursor )
                pNameCursor++;
            pNameCursor++;  // Skip the '\0'.

            // See if there is an alpha texture.
            if( *pNameCursor )
            {
                VERIFY( m_pLoadBitmapFn( pNameCursor, m_pEffectDef[Index]->pAlphaMap[i] ) );
            }
            else
            {
                m_pEffectDef[Index]->pAlphaMap[i] = HNULL;
            }

            // Skip the name.
            while( *pNameCursor )
                pNameCursor++;
            pNameCursor++;  // Skip the '\0'.
        }
    }

    //
    // Ditch the string data and clear the reference count.
    //
    {
        x_free( pNameData );
        m_pEffectDef[Index]->NInstances = 0;
    }

    //
    // Success!
    //
    return( true );
}
*/

bool fx_mgr::UnloadEffect(const char* pEffectName)
{
    int i, j;

    // Look for the effect.
    for (i = 0; i < MAX_EFFECT_DEFINITIONS; i++) {
        if ((m_pEffectDef[i]) &&
            (x_stricmp(pEffectName, m_pEffectDef[i]->pEffectName) == 0)) {
            break;
        }
    }

    // Found it?
    if (i == MAX_EFFECT_DEFINITIONS) {
        return (false);
    }

    // Is it in use?  If so, we can't proceed.
    if (m_pEffectDef[i]->NInstances > 0) {
        return (false);
    }

    // We are clear to get rid of it!
    for (j = 0; j < m_pEffectDef[i]->NBitmaps; j++) {
        m_pUnloadBitmapFn(m_pEffectDef[i]->pDiffuseMap[j]);
        if (m_pEffectDef[i]->pAlphaMap[j]) {
            m_pUnloadBitmapFn(m_pEffectDef[i]->pAlphaMap[j]);
        }

        m_pEffectDef[i]->pDiffuseMap[j] = HNULL;
        m_pEffectDef[i]->pAlphaMap[j] = HNULL;
    }
    free(m_pEffectDef[i]);
    m_pEffectDef[i] = nullptr;

    // Done!
    return (true);
}

//==============================================================================

void fx_mgr::CreateEffect(int Index, fx_def* pEffectDef)
{
    int        Size, i;
    fx_effect* pEffect = nullptr;

    //
    // Create the new instance.
    // Need to determine how much memory is needed for the effect.
    //

    // Start with base size of an effect.
    Size = sizeof(fx_effect);

    // Add in space for staging area.
    Size += (pEffectDef->NSAValues * sizeof(float));

    // Add in space for controllers and their pointers.
    Size += (pEffectDef->NControllers * (sizeof(fx_ctrl) + sizeof(fx_ctrl*)));

    // Add in space for element pointers
    Size += (pEffectDef->NElements * sizeof(fx_element*));

    // elements can have vectors and matrices, and so they must be aligned
    Size = (Size + 0x0f) & ~0x0f;

    // Add in space for elements and memory required by particular element types,
    // taking into account the required alignment restrictions.
    for (i = 0; i < pEffectDef->NElements; i++) {
        int                   ETypeIndex = pEffectDef->pElementDef[i]->TypeIndex;
        fx_element_memory_fn* pMemoryFn = m_ElementType[ETypeIndex].pMemoryFn;

        int MemSize = pMemoryFn(*(pEffectDef->pElementDef[i]));
        Size += ((MemSize + 0x0f) & ~0x0f);
    }

    //
    // Allocate the memory.
    //

    pEffect = (fx_effect*)malloc(Size);
    assert(pEffect);

    fx_effect::ForceConstruct(pEffect);

    //
    // Now start setting up the structures.
    //

    // Let the effect initialize itself from here on out.
    pEffect->Initialize(pEffectDef);

    // Enter the pointer in the table of effect instances.
    m_pEffect[Index] = pEffect;

    // Track the instances of the effect definition.
    pEffectDef->NInstances++;
}

//==============================================================================

bool fx_mgr::InitEffect(int& Index, const char* pName)
{
    if (!pName) {
        assert(false); //, "Null effect name pointer passed into fx-mgr::InitEffect" );
        return false;
    }

    int     i;
    fx_def* pEffectDef = nullptr;

    Index = -1;

    // Find the requested effect definition.
    for (i = 0; i < MAX_EFFECT_DEFINITIONS; i++) {
        if ((m_pEffectDef[i]) &&
            (x_stricmp(m_pEffectDef[i]->pEffectName, pName) == 0)) {
            pEffectDef = m_pEffectDef[i];
            break;
        }
    }
    if (!pEffectDef) {
        return (false);
    }

    // Find an available slot for the new instance.
    for (Index = 0; Index < MAX_EFFECT_INSTANCES; Index++) {
        if (m_pEffect[Index] == nullptr) {
            break;
        }
    }
    if (Index == MAX_EFFECT_INSTANCES) {
        return (false);
    }

    //
    // If this is a NON-SINGLETON effect, OR it is a SINGLETON but the master
    // copy hasn't been created yet, then we need to create a normal effect.
    //

    if (!(pEffectDef->Flags & FX_SINGLETON) || (pEffectDef->NInstances == 0)) {
        CreateEffect(Index, pEffectDef);

        // If we have now just created the master copy, then set its index and
        // tweak its flags (remove SINGLETON, add MASTER_COPY).
        if ((pEffectDef->Flags & FX_SINGLETON) &&
            (pEffectDef->NInstances == 1)) {
            pEffectDef->MasterCopy = Index;
            m_pEffect[Index]->m_Flags &= ~FX_SINGLETON;
            m_pEffect[Index]->m_Flags |= FX_MASTER_COPY;

            // Clear the index.  We need to find another slot for the clone.
            Index = -1;
        }
    }

    //
    // If we are creating a singleton effect... well, let's get on with it!
    //
    if (pEffectDef->Flags & FX_SINGLETON) {
        fx_effect_base*  pEffect = m_pEffect[pEffectDef->MasterCopy];
        fx_effect_clone* pEffectClone = nullptr;

        // Find an available slot for the new clone instance.
        if (Index == -1) {
            for (Index = 0; Index < MAX_EFFECT_INSTANCES; Index++) {
                if (m_pEffect[Index] == nullptr) {
                    break;
                }
            }
            if (Index == MAX_EFFECT_INSTANCES) {
                return (false);
            }
        }

        // Allocate memory.
        pEffectClone = (fx_effect_clone*)malloc(sizeof(fx_effect_clone));
        assert(pEffectClone);

        fx_effect_clone::ForceConstruct(pEffectClone);

        // Initialize the clone.
        pEffectClone->Initialize(pEffect, pEffectDef);

        // Add the clone to the internal list.
        m_pEffect[Index] = pEffectClone;

        // Track the instances (including clones) of the effect definition.
        pEffectDef->NInstances++;
    }

    //
    // Success!
    //

    return (true);
}

//==============================================================================

void fx_mgr::KillEffect(int Index)
{
    assert(IN_RANGE(0, Index, MAX_EFFECT_INSTANCES - 1));
    assert(m_pEffect[Index]);
    assert(m_pEffect[Index]->GetReferences() == 0);

    fx_effect_base* pEffect = m_pEffect[Index];

    // There is one less instance of this effect definition.
    pEffect->m_pEffectDef->NInstances--;

    // Singleton?
    if (pEffect->m_Flags & FX_SINGLETON) {
        Index = pEffect->m_pEffectDef->MasterCopy;
        m_pEffect[Index]->RemoveReference();

        if (m_pEffect[Index]->GetReferences() == 0) {
            assert(m_pEffect[Index]->m_Flags & FX_MASTER_COPY);
            KillEffect(Index);
        }
    }

    pEffect->m_Flags |= FX_DEFERRED_DELETE;
    pEffect->m_NReferences = 0;
}

//==============================================================================

void fx_mgr::AdvanceLogic(fx_handle& Handle, float DeltaTime)
{
    if (Validate(Handle)) {
        m_pEffect[Handle.Index]->AdvanceLogic(DeltaTime);
    }
}

//==============================================================================

void fx_mgr::Render(const fx_handle& Handle)
{
    if (!Validate(Handle)) {
        return;
    }

    fx_effect_base* pEffect = m_pEffect[Handle.Index];

    if (pEffect->IsFinished()) {
        return;
    }

    pEffect->Render();
}

//==============================================================================

void fx_mgr::RestartEffect(fx_handle& Handle)
{
    if (Validate(Handle)) {
        m_pEffect[Handle.Index]->Restart();
    }
}

//==============================================================================

bool fx_mgr::IsFinished(const fx_handle& Handle)
{
    if (Validate(Handle)) {
        return (m_pEffect[Handle.Index]->IsFinished());
    } else {
        return (true);
    }
}

//==============================================================================

bool fx_mgr::IsInstanced(const fx_handle& Handle)
{
    if (Validate(Handle)) {
        return (m_pEffect[Handle.Index]->IsInstanced());
    } else {
        return (true);
    }
}

//==============================================================================

void fx_mgr::SetScale(fx_handle& Handle, const Vector3& Scale)
{
    if (Validate(Handle)) {
        m_pEffect[Handle.Index]->SetScale(Scale);
    }
}

//==============================================================================

void fx_mgr::SetRotation(fx_handle& Handle, const Radian3& Rotation)
{
    if (Validate(Handle)) {
        m_pEffect[Handle.Index]->SetRotation(Rotation);
    }
}

//==============================================================================

void fx_mgr::SetTranslation(fx_handle& Handle, const Vector3& Translation)
{
    if (Validate(Handle)) {
        m_pEffect[Handle.Index]->SetTranslation(Translation);
    }
}

Vector3 fx_mgr::GetTranslation(const fx_handle& Handle)
{
    if (Validate(Handle)) {
        return m_pEffect[Handle.Index]->GetTranslation();
    } else {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
}

//==============================================================================

void fx_mgr::SetColor(fx_handle& Handle, const Colour& Color)
{
    if (Validate(Handle)) {
        m_pEffect[Handle.Index]->SetColor(Color);
    }
}

//==============================================================================

Colour fx_mgr::GetColor(fx_handle& Handle)
{
    if (Validate(Handle)) {
        return m_pEffect[Handle.Index]->GetColor();
    }

    return Colour(0, 0, 0, 0);
}

//==============================================================================

const BBox& fx_mgr::GetBounds(const fx_handle& Handle)
{
    if (Validate(Handle)) {
        return (m_pEffect[Handle.Index]->GetBounds());
    } else {
        return (s_DefaultBounds);
    }
}

//==============================================================================

void fx_mgr::SetSuspended(fx_handle& Handle, bool Suspended)
{
    if (Validate(Handle)) {
        m_pEffect[Handle.Index]->SetSuspended(Suspended);
    }
}

bool fx_mgr::Validate(const fx_handle& Handle)
{
    if (IN_RANGE(0, Handle.Index, MAX_EFFECT_INSTANCES - 1) &&
        m_pEffect[Handle.Index] &&
        !(m_pEffect[Handle.Index]->m_Flags & FX_DEFERRED_DELETE)) {
        return (true);
    } else {
        Handle.Index = -1;
        return (false);
    }
}
