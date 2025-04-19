
#include "VirtualTextureMask.h"
#include "../../Property.h"
#include "../../render/Geom.h"

#include <cassert>

virtual_texture_mask::virtual_texture_mask(  ) :
VTextureMask  ( 0x0 )
{
}

//==============================================================================

void virtual_texture_mask::OnEnumProp( prop_enum& List, Geom* pGeom )
{
    List.PropEnumHeader( "VTextureList", "Properties related to a virtual texture mask", 0 );
    int HeaderId = List.PushPath( "VTextureList\\" );

    // add the mask property
    List.PropEnumInt( "Mask", "The final mask that the game uses", PROP_TYPE_DONT_SHOW | PROP_TYPE_DONT_SAVE );

    List.PopPath( HeaderId );
}

//==============================================================================

bool virtual_texture_mask::OnProperty( prop_query& I, Geom* pGeom )
{
    (void)pGeom;

    // early out
    if( !I.IsBasePath( "VTextureList" ) )
    {
        return false;
    }

    int HeaderId = I.PushPath( "VTextureList\\" );


    if( I.VarInt( "Mask", (int&)VTextureMask ) )
    {
    }
    else
    {
        I.PopPath( HeaderId );
        return false;
    }

    I.PopPath( HeaderId );
    return true;
}

//==============================================================================

int virtual_texture_mask::FindFirstMat( Geom* pGeom, int iVTexture )
{
    assert( (iVTexture >= 0) && (iVTexture < pGeom->numVirtualTextures) );

    // find any material this vtexture operates on
    int iMaterial;
    for( iMaterial = 0; iMaterial < pGeom->numMaterials; iMaterial++ )
    {
        if( pGeom->virtualTextures[iVTexture].materialMask & (1<<iMaterial) )
        {
            return iMaterial;
        }
    }

    return -1;
}

std::string virtual_texture_mask::BuildEnumList( Geom* pGeom, int iVTexture )
{
    assert( (iVTexture >= 0) && (iVTexture < pGeom->numVirtualTextures) );

    std::string EnumList;

    int iMaterial = FindFirstMat( pGeom, iVTexture );

    // now add the textures this material uses, separating them with a '~'
    if( iMaterial < 0 )
    {
        EnumList += "~~";
    }
    else
    {
        Geom::Material& GeomMat = pGeom->materials[iMaterial];

        int iTexture;
        for( iTexture = GeomMat.iTexture; iTexture < GeomMat.iTexture + GeomMat.nTextures; iTexture++ )
        {
            EnumList += pGeom->GetTextureDesc( iTexture );
            EnumList += "~";
        }
    }

    // replace the '~' characters with terminators
    int i;
    for( i = 0; EnumList[i]; i++ )
    {
        if( EnumList[i] == '~' )
            EnumList[i] = 0;
    }

    return EnumList;
}

void virtual_texture_mask::SyncVTextures( Geom* pGeom )
{
 
}
