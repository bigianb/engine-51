#include "Projector.h"
#include "../VectorMath.h"
#include "../view/View.h"
//#include "Parsing\TextIn.hpp"

#include "../collisionMgr/CollisionMgr.h"

//#include "Render\Render.hpp"

static struct projector_obj_desc : public object_desc
{
    projector_obj_desc(  ) : object_desc( 
            Object::TYPE_PROJECTOR, 
            "Projector", 
            "RENDER",

            Object::ATTR_RENDERABLE,

            FLAGS_GENERIC_EDITOR_CREATE | FLAGS_IS_DYNAMIC) {}

    virtual Object* Create( ObjectManager* om, collision_mgr* ) { return new projector_obj(om); }

} s_ProjectorObj_Desc;

const object_desc& projector_obj::GetTypeDesc(  ) const
{
    return s_ProjectorObj_Desc;
}

const object_desc& projector_obj::GetObjectType(  )
{
    return s_ProjectorObj_Desc;
}

projector_obj::projector_obj( ObjectManager* om ) : Object(om),
    m_bIsDynamic    ( false   ),
    m_bIsShadow     ( true    ),
    m_bIsActive     ( true    ),
    m_bIsFlashlight ( false   ),
    m_FOV           ( R_30    ),
    m_Length        ( 1000.0f ),
    m_hTexture      (         )
{
}

projector_obj::~projector_obj(  )
{
}


void projector_obj::OnRender(  )
{
    if ( IsActive() )
    {
        if( IsShadow() )
            render::SetShadowProjection( GetL2W(), GetFOV(), GetLength(), GetTexture() );
        else
            render::SetTextureProjection( GetL2W(), GetFOV(), GetLength(), GetTexture() );
    }
}

BBox projector_obj::GetLocalBBox( void ) const
{
    BBox Box;

    // this will keep the bbox around the sphere
    Box.Set( Vector3( -40.0f, -40.0f, -40.0f ), Vector3( 40.0f, 40.0f, 40.0f ) );

    // get the viewport dimensions
    int W, H;
    if( m_hTexture.getPointer() )
    {
        // get the viewport dimensions from the texture
        texture* pTex = m_hTexture.getPointer();
        W = pTex->m_Bitmap.getWidth();
        H = pTex->m_Bitmap.getHeight();
    }
    else
    {
        // Assume it's a square texture.
        W = 32;
        H = 32;
    }

    // this will keep the bbox around the frustum
    view    ProjView;
    Vector3 P[5];
    int     X0,X1,Y0,Y1;
    Matrix4 L2W = GetL2W();
    L2W.SetTranslation(Vector3(0.0f, 0.0f, 0.0f));
    ProjView.SetV2W(L2W);
    ProjView.SetXFOV(m_FOV);
    ProjView.SetPixelScale(1.0f);
    ProjView.SetViewport(0, 0, W, H);
    ProjView.SetZLimits(1.0f, GetLength()); // not really important
    ProjView.GetViewport(X0,Y0,X1,Y1);
    P[0] = ProjView.GetPosition();
    P[1] = ProjView.RayFromScreen( (float)X0, (float)Y0, view::VIEW );
    P[2] = ProjView.RayFromScreen( (float)X0, (float)Y1, view::VIEW );
    P[3] = ProjView.RayFromScreen( (float)X1, (float)Y1, view::VIEW );
    P[4] = ProjView.RayFromScreen( (float)X1, (float)Y0, view::VIEW );
    // Normalize so that Z is Dist
    for( int i=1; i<=4; i++ )
    {
        P[i] *= GetLength() / P[i].GetZ();
    }
    Box.AddVerts( P, 5 );

    return Box;
}

//=========================================================================

void projector_obj::OnEnumProp( prop_enum& List )
{
    Object::OnEnumProp( List );

    List.PropEnumHeader  ( "Projector", "Projector Properties", 0 );
    List.PropEnumBool    ( "Projector\\Dynamic",   "Can this guy move?", 0 );
    List.PropEnumBool    ( "Projector\\Shadow",    "Is this a shadow projector or light projector?", 0 );
    List.PropEnumAngle   ( "Projector\\FOV",       "FOV for the projector", 0 );
    List.PropEnumFloat   ( "Projector\\Length",    "Max distance the projector can extend.", 0 );
    List.PropEnumExternal( "Projector\\Texture",   "Resource\0xbmp\0", "Resource File", PROP_TYPE_MUST_ENUM );
}

bool projector_obj::OnProperty( prop_query& I )
{
    if( Object::OnProperty( I ) )
    {
    }
    else if( I.IsVar( "Projector\\Texture" ) )
    {
        // External
        if( I.IsRead() )
        {
            I.SetVarExternal( m_hTexture.getName().c_str(), RESOURCE_NAME_SIZE );
        }
        else
        {
            // Get the FileName
            std::string String = I.GetVarExternal();
            if( !String.empty() )
            {
                m_hTexture.setName( String );

                // Force the bbox to be recomputed
                OnMove( GetPosition() );
            }
        }
        return( true );
    }
    else if( I.VarBool("Projector\\Dynamic", m_bIsDynamic) )
    {
    }
    else if( I.VarBool("Projector\\Shadow", m_bIsShadow) )
    {
    }
    else if( I.VarAngle("Projector\\FOV", m_FOV, R_15, R_179) )
    {
        if( !I.IsRead() )
        {
            // Force the bbox to be recomputed
            OnMove( GetPosition() );
        }
    }
    else if( I.VarFloat("Projector\\Length", m_Length, 2.0f, 10000.0f) )
    {
        if( !I.IsRead() )
        {
            // Force the bbox to be recomputed
            OnMove( GetPosition() );
        }
    }
    else
    {   
        return false;
    }

    return true;
}

