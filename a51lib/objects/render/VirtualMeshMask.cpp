
#include "VirtualMeshMask.h"
#include "../../Property.h"
#include "../../render/Geom.h"


virtual_mesh_mask::virtual_mesh_mask(  ) :
    VMeshMask   ( 0xffffffff )
{
}

//==============================================================================

void virtual_mesh_mask::OnEnumProp( prop_enum& List, Geom* pGeom )
{    
    List.PropEnumHeader( "VMeshList", "Properties related to a virtual mesh mask", 0 );
    int HeaderId = List.PushPath( "VMeshList\\" );

    // add the mask property
    List.PropEnumInt( "Mask", "The final mask that the game uses", PROP_TYPE_DONT_SHOW | PROP_TYPE_DONT_SAVE );

    List.PopPath( HeaderId );
}

//==============================================================================

bool virtual_mesh_mask::OnProperty( prop_query& I, Geom* pGeom )
{
    // early out
    if( !I.IsBasePath( "VMeshList" ) )
    {
        return false;
    }

    int HeaderId = I.PushPath( "VMeshList\\" );

    I.PopPath( HeaderId );
    return false;
}

//==============================================================================

void virtual_mesh_mask::SyncVMeshes( Geom* pGeom )
{
}
