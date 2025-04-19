#pragma once

#include "../VectorMath.h"

#define PS2_PIXEL_SCALE     0.849f
#define PS2_VIEWPORT_WIDTH  512
#define PS2_VIEWPORT_HEIGHT 448


#define DEFAULT_PIXEL_SCALE     1.0f

class view
{   

//------------------------------------------------------------------------------
//  Local Types
//------------------------------------------------------------------------------

public:
    // Coordinate systems
    enum system { WORLD, VIEW };

    // Visiblity results
    enum visible
    {
        VISIBLE_NONE,     // Totally outside of volume
        VISIBLE_FULL,     // Totally inside of volume
        VISIBLE_PARTIAL   // Partially visible in volume (clipping maybe needed)
    } ;


//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------

public:
                                
                view            (  );
                view            ( const view& View );
               ~view            (  );
                
void            SetViewport     ( int X0, int Y0, int X1, int Y1 );
void            SetXFOV         ( Radian XFOV );
void            SetYFOV         ( Radian YFOV );
void            SetPixelScale   ( float PixelScale = DEFAULT_PIXEL_SCALE );
void            SetZLimits      ( float ZNear, float ZFar );
                
void            SetPosition     ( const Vector3& Position );
void            SetRotation     ( const Radian3& Rotation );
void            SetRotation     ( const Quaternion& Quat );
void            SetV2W          ( const Matrix4& V2W      );
                
void            Translate       ( const Vector3& Translation, system System = WORLD );
void            RotateX         ( Radian Angle, system System = WORLD );
void            RotateY         ( Radian Angle, system System = WORLD );
void            RotateZ         ( Radian Angle, system System = WORLD );

//------------------------------------------------------------------------------

void            AddScissorRect  ( const IntRect& Rc );
void            ClearScissors   ();

//------------------------------------------------------------------------------

void            GetViewport     ( int& X0, int& Y0, int& X1, int& Y1 ) const;
void            GetViewport     ( rect& r ) const;

void            GetPixel        ( float ParamX, float ParamY, int& X, int& Y ) const;
void            GetZLimits      ( float& ZNear, float& ZFar ) const;
void            GetPitchYaw     ( Radian& Pitch, Radian& Yaw ) const;
                
Radian          GetXFOV         () const;
Radian          GetYFOV         () const;
float             GetPixelScale   () const;
                
Vector3         GetPosition     () const;

const Matrix4&  GetW2V          () const;
const Matrix4&  GetV2W          () const;
const Matrix4&  GetV2C          () const;
const Matrix4&  GetC2S          () const;
const Matrix4&  GetV2S          () const;
const Matrix4&  GetW2C          () const;
const Matrix4&  GetW2S          () const;

void            GetV2C          ( int ClipSize, Matrix4& V2C ) const;
void            GetW2C          ( int ClipSize, Matrix4& W2C ) const;
void            GetC2S          ( int ClipSize, Matrix4& C2S ) const;
void            GetC2W          ( int ClipSize, Matrix4& C2W ) const;
void            GetC2W          (               Matrix4& C2W ) const;

Vector3         GetViewX        () const;     // "Camera Left"
Vector3         GetViewY        () const;     // "Camera Up"
Vector3         GetViewZ        () const;     // "Camera Line of Sight"

float             GetScreenDist   () const;

//------------------------------------------------------------------------------

void            LookAtPoint     ( const Vector3& Point, 
                                        system   System = WORLD );
                
void            LookAtPoint     ( const Vector3& FromPoint, 
                                  const Vector3& ToPoint, 
                                        system   System = WORLD );

void            OrbitPoint      ( const Vector3& Point, 
                                        float      Distance,
                                        Radian   Pitch,
                                        Radian   Yaw );

//------------------------------------------------------------------------------
//  These functions build planes from the frustum taking into account the 
//  viewport size and field of view.  All the planes face inward.  A point in 
//  the view frustum will be inside all the planes.
//------------------------------------------------------------------------------

void            GetViewPlanes   ( plane& Top,
                                  plane& Bottom,
                                  plane& Left,
                                  plane& Right,
                                  system System = WORLD ) const;
                
void            GetViewPlanes   ( plane& Top,
                                  plane& Bottom,
                                  plane& Left,
                                  plane& Right,
                                  plane& Near,
                                  plane& Far,
                                  system System = WORLD ) const;

//------------------------------------------------------------------------------
//  This version of the plane function returns the internally cached planes,
//  so it is very quick.  The order of the planes is rigged to improve culling
//  operations.  Essentially, the planes are ordered so that, statistically, 
//  "stuff" can be culled by one of the first few planes thus avoiding the need
//  to test every plane.
//
//  0=Left  1=Right  2=Bottom  3=Top  4=Near  5=Far
//------------------------------------------------------------------------------

const plane*    GetViewPlanes   ( system System = WORLD ) const;

const int*      GetViewPlaneMinBBoxIndices( system System = WORLD ) const;
const int*      GetViewPlaneMaxBBoxIndices( system System = WORLD ) const;

void            GetMinMaxZ  ( const BBox& BBox, float& MinZ, float& MaxZ ) const;

//------------------------------------------------------------------------------
//  This version of the plane function allows you to get the planes which pass
//  through a region of the viewport.  The X/Y coordinates specify a rectangle
//  with the familiar SCREEN coordinates where +X=Right and +Y=Down.  The (0,0)
//  point is NOT the top left.  Rather, it is where the line of sight passes
//  through.  Note that the region you request can be smaller OR larger than the
//  view's viewport or even larger than the entire screen.
//
//  For example, say you want to get the planes of a region which is W pixels 
//  wide and H pixels tall centered about the line of sight.  (Note that the 
//  position of the line of sight on the screen and the current viewport 
//  settings are not relevant.)  You would use values (-W/2, -H/2, W/2, H/2).
//------------------------------------------------------------------------------

void            GetViewPlanes   ( float X0, float Y0, float X1, float Y1,
                                  plane& Top,
                                  plane& Bottom,
                                  plane& Left,
                                  plane& Right,
                                  plane& Near,
                                  plane& Far,
                                  system System = WORLD ) const;
                
//------------------------------------------------------------------------------

void            GetProjection   ( float& XP0, float& XP1, float& YP0, float& YP1 ) const;
//------------------------------------------------------------------------------

bool           PointInView     ( const Vector3& Point, 
                                        system   System = WORLD ) const;



//------------------------------------------------------------------------------
//  These are quick implementations, and can give false positives in certain
//  situations.  The return values are 0="outside of view", 1="completely within
//  view", and 2="partially within view".  Note that you can treat the return
//  value as a boolean for the question "Visible?".
//------------------------------------------------------------------------------

int             SphereInView    ( const Vector3& Center,
                                        float      Radius,
                                        system   System = WORLD ) const;

int             BBoxInView      ( const  BBox&   bbox,
                                  uint32_t&           CheckPlaneMask,
                                  system         System = WORLD ) const;
                
int             BBoxInView      ( const BBox&    bbox,
                                        system   System = WORLD ) const;

bool           SphereInCone    ( const Vector3& Center,
                                        float      Radius ) const;

bool           SphereInConeAngle( const Vector3& Center,
                                         float      Radius,
                                         float      tanAngle ) const;

//------------------------------------------------------------------------------

Vector3         ConvertW2V      ( const Vector3& Point ) const;
Vector3         ConvertV2W      ( const Vector3& Point ) const;
                
Vector3         PointToScreen   ( const Vector3& Point,
                                        system   System = WORLD ) const;
                
Vector3         RayFromScreen   (       float      ScreenX,
                                        float      ScreenY,
                                        system   System = WORLD ) const;
              
                
float             CalcScreenSize  ( const Vector3& Position,
                                        float      WorldRadius,
                                        system   System = WORLD ) const;

//------------------------------------------------------------------------------
//  Fields
//------------------------------------------------------------------------------

protected:

        //
        // Required fields.
        //

        Vector3     m_WorldPos;         // Position    of camera in World (V2W)
        Matrix4     m_WorldOrient;      // Orientation of camera in World (V2W)
                                        
        int         m_ViewportX0;       // Corners of 2D viewpoint on screen
        int         m_ViewportY0;       
        int         m_ViewportX1;       
        int         m_ViewportY1;       
                                        
        Radian      m_XFOV;             // Field of view in X
        float         m_PixelScale;       // Says how the hardware scales pixels (not all pixels are square!)
                                        
        float         m_ZNear;            // Near plane in Z
        float         m_ZFar;             // Far  plane in Z


        //
        // Derived fields with cached data.
        //
        
mutable uint32_t         m_Dirty;            // Dirty flags for derived fields.
                                        
mutable Matrix4     m_W2V;              // Transforms World -> View
mutable Matrix4     m_V2W;              // Transforms View -> World
mutable Matrix4     m_V2C;
mutable Matrix4     m_C2S;
mutable Matrix4     m_V2S;
mutable Matrix4     m_W2C;
mutable Matrix4     m_W2S;

mutable float         m_ScaledScreenDistX;    // Dist where 3D units = screen units
mutable float         m_ScaledScreenDistY;    // same as X, except adjusted for non-square pixels
mutable Radian      m_YFOV;                 // Field of view in Y

// The planes are ordered for faster culling: L0,R1,B2,T3,N4,F5
mutable plane       m_WorldSpacePlane[6];
mutable plane       m_ViewSpacePlane[6];
mutable int         m_WorldPlaneMinIndex[6*3];
mutable int         m_WorldPlaneMaxIndex[6*3];
mutable int         m_ViewPlaneMinIndex[6*3];
mutable int         m_ViewPlaneMaxIndex[6*3];
mutable plane       m_ZPlane;
mutable int         m_ZPlaneMinI[3];
mutable int         m_ZPlaneMaxI[3];
mutable Vector3     m_ConeAxis;
mutable float         m_ConeSlope;
        
mutable float         m_ProjectX[2];      // ScreenX = ProjectX[0] + ProjectX[1]*(ViewX/ViewZ)
mutable float         m_ProjectY[2];      // ScreenY = ProjectY[0] + ProjectY[1]*(ViewY/ViewZ)     

mutable IntRect       m_ScissorRects[4];
mutable int         m_nScissors;
mutable int         m_iScissor;


//------------------------------------------------------------------------------
//  Internal Functions
//------------------------------------------------------------------------------

protected:

    void    UpdateW2V       () const;
    void    UpdateV2W       () const;
                            
    void    UpdateV2C       () const;
    void    UpdateC2S       () const;
    void    UpdateV2S       () const;
    void    UpdateW2C       () const;
    void    UpdateW2S       () const;
                            
    void    UpdateYFOV      () const;
    void    UpdatePlanes    () const;
    void    UpdateEdges     () const;
    void    UpdateScreenDist() const;
    void    UpdateProjection() const;
};

extern int g_ClipSize;
