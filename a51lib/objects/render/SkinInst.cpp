#include "SkinInst.h"

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

/*
static struct skin_loader : public rsc_loader
{
    //-------------------------------------------------------------------------
    
    skin_loader( void ) : rsc_loader( "SKIN GEOM", ".skingeom" ) {}

    //-------------------------------------------------------------------------
    
    virtual void* PreLoad ( X_FILE* FP )
    {
        MEMORY_OWNER( "SKIN GEOM DATA" );
        fileio File;
        return( File.PreLoad( FP ) );
    }

    //-------------------------------------------------------------------------
    
    virtual void* Resolve ( void* pData ) 
    {
        fileio     File;
        SkinGeom* pSkinGeom = NULL;

        File.Resolved( (fileio::resolve*)pData, pSkinGeom );

        return( pSkinGeom );
    }

    //-------------------------------------------------------------------------
    
    virtual void Unload( void* pData )
    {
        SkinGeom* pSkinGeom = (SkinGeom*)pData;
        ASSERT( pSkinGeom );

        #ifdef TARGET_XBOX
        xbox_Unregister( pSkinGeom );
        #endif

        delete pSkinGeom;
    }

} s_Skin_Geom_Loader;
*/

skin_inst::skin_inst( ) :
    render_inst(),
    m_MinAmbient(0,0,0,255),
    m_OtherAmbientAmount(0.4f)
{
}

//=============================================================================

skin_inst::~skin_inst(  )
{
    if ( m_hInst.IsNonNull() )
    {
        render::UnregisterSkinInstance( m_hInst );
    }
}

//=============================================================================

std::string skin_inst::GetSkinGeomName() const
{
    return( m_hSkinGeom.getName() );
}

//=============================================================================

void skin_inst::Render( const Matrix4* pL2W,
                        const Matrix4* pBone,
                        int            nBone,
                        uint32_t            Flags,
                        uint64_t            LODMask,
                        const Colour&  Ambient )
{
    // calculate ambient
    Vector3 AmbientVec( Ambient.r*m_OtherAmbientAmount,
                        Ambient.g*m_OtherAmbientAmount,
                        Ambient.b*m_OtherAmbientAmount );
    AmbientVec += Vector3( (float)m_MinAmbient.r, (float)m_MinAmbient.g, (float)m_MinAmbient.b );
    AmbientVec.Min(255.0f);

    int Alpha = (int)(Ambient.a * m_Alpha) / 255;

    // Add a Skin Instance
    render::AddSkinInstance( m_hInst,
                             pBone,
                             LODMask,
                             m_VTextureMask,
                             Flags,
                             Colour( (uint8_t)AmbientVec.GetX(), (uint8_t)AmbientVec.GetY(), (uint8_t)AmbientVec.GetZ(), (uint8_t)Alpha ) );
}

//=============================================================================

void skin_inst::RenderDistortion( const Matrix4* pL2W,
                                  const Matrix4* pBone,
                                  int            nBone,
                                  uint32_t            Flags,
                                  uint64_t            LODMask,
                                  const Radian3& NormalRot,
                                  const Colour&  Ambient )
{
    (void)nBone;
    (void)pL2W;

    // Add a Skin Instance
    render::AddSkinInstanceDistorted( m_hInst, pBone, LODMask, Flags, NormalRot, Ambient );
}

//=============================================================================

void skin_inst::RenderShadowCast( const Matrix4* pL2W,
                                  const Matrix4* pBone,
                                  int            nBone,
                                  uint32_t            Flags,
                                  uint64_t            LODMask,
                                  uint64_t            ProjMask )
{

    // add the shadow
    render::AddSkinCaster( m_hInst,
                           pBone,
                           LODMask,
                           ProjMask );
}

//=============================================================================

void skin_inst::OnEnumProp( prop_enum& List )
{
    // Important: The Header and External MUST be enumerated first!
    List.PropEnumHeader  ( "RenderInst", "Render Instance", 0 );
    List.PropEnumExternal( "RenderInst\\File", "Resource\0skingeom", "Resource File", PROP_TYPE_MUST_ENUM );
    List.PropEnumColor   ( "RenderInst\\MinAmbient",   "Minimum amount of ambient this instance will receive.", 0 );
    List.PropEnumFloat   ( "RenderInst\\OtherAmbient", "Percentage of floor color that will influence ambient (between 0 and 1).", 0 );

    // enumerate the vtexture list
    int HeaderId = List.PushPath( "RenderInst\\" );
    m_VTextureMask.OnEnumProp( List, GetGeom() );
    List.PopPath( HeaderId );

    render_inst::OnEnumProp( List );
}

//=============================================================================

bool skin_inst::OnProperty( prop_query& I )
{
    if( render_inst::OnProperty( I ) )
        return( true );

     // External
    if( I.IsVar( "RenderInst\\File" ) )
    {
        if( I.IsRead() )
        {
            I.SetVarExternal( m_hSkinGeom.getName().c_str(), RESOURCE_NAME_SIZE );
        }
        else
        {
            // Get the FileName
            const char* pString = I.GetVarExternal();
            assert( pString );
            
            // Clear?
            if( strcmp( pString, "<null>" ) == 0 )
            {
                SetUpSkinGeom( "" );
            }
            else if( pString[0] )
            {
                // Setup
                SetUpSkinGeom( pString );
            }

            m_VMeshMask.VMeshMask       = 0xffffffff;
            m_VTextureMask.VTextureMask = 0;
        }
        return( true );
    }

    if ( I.VarColor( "RenderInst\\MinAmbient", m_MinAmbient ) )
    {
        return true;
    }

    if ( I.VarFloat( "RenderInst\\OtherAmbient", m_OtherAmbientAmount ) )
    {
        return true;
    }

    // handle the vtexture list
    int HeaderId = I.PushPath( "RenderInst\\" );
    if( m_VTextureMask.OnProperty( I, GetGeom() ) )
    {
        I.PopPath( HeaderId );
        return true;
    }
    I.PopPath( HeaderId );

    return( false );
}

//=============================================================================

void skin_inst::SetUpSkinGeom( std::string fileName)
{
    if ( m_hInst.IsNonNull() )
    {
        render::UnregisterSkinInstance( m_hInst );
        m_hInst = HNULL;
    }

    m_hSkinGeom.setName( fileName );
    SkinGeom* pSkinGeom = m_hSkinGeom.getPointer();
    if( pSkinGeom )
    {
        // Register the instance with the Render Manager
        m_hInst = render::RegisterSkinInstance( *pSkinGeom );
    }
}

//=============================================================================

const skin_inst& skin_inst::operator = ( const skin_inst& Skin )
{
    m_hSkinGeom = Skin.m_hSkinGeom;
    SetUpSkinGeom( m_hSkinGeom.getName() );

    m_VMeshMask    = Skin.m_VMeshMask;
    m_VTextureMask = Skin.m_VTextureMask;

    return *this;
}

void skin_inst::SetVirtualTexture( const char* VTextureName, const char* DiffuseTextureDesc )
{
    Geom* pGeom = GetGeom();
    if( !pGeom )
        return;

    // get a reference to the virtual texture we are talking about
    int VTextureIndex = pGeom->GetVTextureIndex( VTextureName );
    if( VTextureIndex < 0 )
        return;
    Geom::VirtualTexture& GeomVTex = pGeom->virtualTextures[VTextureIndex];

    // Figure out any material that this vtexture effects...this is a bit
    // nasty, but necessary due to the way vtextures are structured by the
    // geometry system.
    int i;
    for( i = 0; i < pGeom->numMaterials; i++ )
    {
        if( GeomVTex.materialMask & (1<<i) )
            break;
    }
    if( i == pGeom->numMaterials )
        return;
    Geom::Material& GeomMat = pGeom->materials[i];

    // now from the material, see if we can find a texture description that matches
    // the diffuse texture description
    uint32_t Mask = 0x0f<<(VTextureIndex*4);
    for( i = GeomMat.iTexture; i < (GeomMat.iTexture+GeomMat.nTextures); i++ )
    {
        if( !strcasecmp( DiffuseTextureDesc, pGeom->GetTextureDesc( i ).c_str() ) )
        {
            m_VTextureMask.VTextureMask &= ~Mask;
            m_VTextureMask.VTextureMask |= (i-GeomMat.iTexture)<<(VTextureIndex*4);
            break;
        }
    }
}

void skin_inst::SetVirtualTexture( int VTexture )
{
    m_VTextureMask.VTextureMask = VTexture;
}
