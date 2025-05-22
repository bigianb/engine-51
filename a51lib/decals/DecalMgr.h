#pragma once

#include "DecalDefinition.h"
#include "../render/Texture.h"
#include "../xfiles/xHandleArray.h"

struct RigidGeom;
class ResourceManager;

class decal_mgr
{
public:
    enum { MAX_VERTS_PER_DECAL = 32 }; // the maximum number of verts a decal may have (if it wraps around complex geometry)

    //==========================================================================
    // This structure contains everything a decal will need to render. At the
    // bottom level we'll use structures that are better suited to the hardware,
    // but this is good for the high-level interface.
    //==========================================================================
    struct decal_vert
    {
        enum { FLAG_DRAW_WIREFRAME = 0x0001,
               FLAG_DECAL_START    = 0x0002,
               FLAG_SKIP_TRIANGLE  = 0x8000 };
        
        Vector3 Pos;
        int     Flags;
        Vector2 UV;
    };

    //==========================================================================
    // Construction/destruction, initialization
    //==========================================================================
    // TODO IJB: default param is wrong but needed for global static instance.
            decal_mgr   (ResourceManager* = nullptr);
            ~decal_mgr  ();
    void    Init        ();
    void    Kill        ();

    //==========================================================================
    // load/unload functions
    //==========================================================================
    void    ClearDynamicQueue   ();
    void    ResetDynamicDecals  ();
    void    LoadStaticDecals    ( const char* FileName );
    void    UnloadStaticDecals  ();

    //==========================================================================
    // Methods for rendering decals
    //==========================================================================

    // This function is for rendering static decals in the editor. In the game,
    // static decals will be pre-compiled and this function won't be necessary.
    void    RenderStaticDecal       ( const decal_definition& Def,
                                      const decal_vert*       pVerts,
                                      int                     nVerts,
                                      const Matrix4&          L2W,
                                      bool                   Wireframe );

    void    OnRender                ();
    void    OnUpdate                ( float DeltaTime );

    //==========================================================================
    // Methods for plastering decals all over the world
    //==========================================================================
    
    // This function is meant to be used by the editor and will cast a ray to
    // determine if a decal can be stuck in the world, and if so, it will fill
    // in the transform matrix that will place that decal. The function returns
    // the # of verts in the created decal, or 0 if a decal couldn't be made.
    int     CreateStaticDecalData   ( const decal_definition& Def,
                                      const Vector3&          Start,
                                      const Vector3&          End,
                                      const Vector2&          Size,
                                      Radian                  Roll,
                                      decal_vert              Verts[MAX_VERTS_PER_DECAL],
                                      Matrix4&                L2W );

    // This function will cast a ray, and stick a decal where any collisions
    // have occured on playsurfaces. An example of this would be casting a ray
    // from the back of a guy's head after being shot, and making a blood splatter
    // three feet away.
    void    CreateDecalFromRayCast  ( const decal_definition& Def,
                                      const Vector3&          Start,
                                      const Vector3&          End,
                                      const Vector2&          Size,
                                      Radian                  Roll );
    void    CreateDecalFromRayCast  ( const decal_definition& Def,
                                      const Vector3&          Start,
                                      const Vector3&          End );

    // This function will take a point that was pre-computed (probably from a
    // collision), and create a decal there. An example of this would be when
    // a bullet impact occurs, you already know the point of collision on the
    // wall, so you don't need to re-cast that ray.
    void    CreateDecalAtPoint      ( const decal_definition& Def,
                                      const Vector3&          Point,
                                      const Vector3&          Normal,
                                      const Vector2&          Size,
                                      Radian                  Roll );
    void    CreateDecalAtPoint      ( const decal_definition& Def,
                                      const Vector3&          Point,
                                      const Vector3&          Normal );

    //
    // This is a fast path for bullet holes which take advantage of possible
    // shortcuts
    //
    void    CreateBulletHole        ( const decal_definition& Def,
                                      const Vector3&          Point,
                                      const plane&            Plane,
                                      const Vector3*          pTriPos );

    //==========================================================================
    // Methods for registering decal definitions
    //==========================================================================
    xhandle RegisterDefinition      ( decal_definition& Def );
    void    UnregisterDefinition    ( decal_definition& Def );

    //==========================================================================
    // Methods for the editor to export static decals
    //==========================================================================
    void    BeginStaticDecalExport  ( const char*         FileName );
    void    AddStaticDecalToExport  ( const char*         PackageName,
                                      int                 iGroup,
                                      int                 iDecalDef,
                                      int                 nVerts,
                                      const decal_vert    Verts[MAX_VERTS_PER_DECAL],
                                      const Matrix4&      L2W,
                                      uint16_t                 ZoneInfo );
    //void    EndStaticDecalExport    ( platform            PlatformType );

protected:

    //==========================================================================
    // Structures used for simplifying decal geometry
    //==========================================================================
    struct decal_edge
    {
        int16_t   P0;
        int16_t   P1;
        bool Added;
    };

    struct triangulate_link
    {
        int16_t     iPrev;
        int16_t     iNext;
        int16_t     iPrevConcave;
        int16_t     iNextConcave;
        bool   bConcave;
    };

    struct working_data
    {
        enum { MAX_WORKING_VERTS = 512 };
        enum
        {
            FLAG_POLY_ADDED  = 0x0001,
            FLAG_POLY_START  = 0x8000
        };

        // Raw data version
        int         nVerts;
        uint16_t         VertFlags[MAX_WORKING_VERTS];
        Vector3     ClippedVerts[MAX_WORKING_VERTS];
        
        // Indexed version
        int         nIndexedVerts;
        int16_t         RemapIndices[MAX_WORKING_VERTS];
        Vector3     IndexedVertPool[MAX_WORKING_VERTS];

        // edge data for poly reduction and retriangulation
        int         nFinalVerts;
        int         nEdges;
        decal_edge  EdgeList[MAX_WORKING_VERTS];
        int         nCoplanarPolyVerts;
        int16_t         CoplanarPolyVerts[MAX_WORKING_VERTS];
        uint16_t         FinalVertFlags[MAX_VERTS_PER_DECAL];
    };

    //==========================================================================
    // The following functions are for creating the different decal types and
    // are very high-level (although still considered protected)
    //==========================================================================
    int     CalcNoClipDecal         ( int               Flags,
                                      const Vector3&    Point,
                                      const Vector3&    Normal,
                                      const Vector2&    Size,
                                      Radian            Roll,
                                      decal_vert        Verts[MAX_VERTS_PER_DECAL],
                                      Matrix4&          L2W );
    int     CalcProjectedDecal      ( const Vector3&    Point,
                                      const Vector3&    SurfaceNormal,
                                      const Vector3&    NegIncomingRay,
                                      const Vector2&    Size,
                                      Radian            Roll,
                                      decal_vert        Verts[MAX_VERTS_PER_DECAL],
                                      Matrix4&          L2W );

    //==========================================================================
    // These functions are used to add the various geometry triangles to our
    // decal. The geometry polygons will get clipped against the projection
    //==========================================================================
    void    AddGeometryToDecal      ( const RigidGeom* pRigidGeom,
                                      const Matrix4&    GeomL2W,
                                      const Matrix4&    OrthoProjection,
                                      const Vector3&    ProjectionRay,
                                      working_data&     WorkingData     );
    void    CalcDecalVertsFromVolume( const BBox&       WorldBBox,
                                      const Matrix4&    OrthoProjection,
                                      const Vector3&    ProjectionRay,
                                      working_data&     WorkingData     );

    //==========================================================================
    // These functions are all used to reduce the number of polygons we have by
    // combing coplanar polygons and removing degenerate tris.
    //==========================================================================
    void    CreateIndexedVertPool   ( working_data&     WorkingData     );
    void    RemoveDegenerateTris    ( working_data&     WorkingData     );
    bool   CollectCoplanarEdges    ( working_data&     WorkingData,
                                      plane&            PolyPlane       );
    void    RemoveDuplicateEdges    ( working_data&     WorkingData     );
    bool   GetCoplanarPolyVerts    ( working_data&     WorkingData     );
    bool   IsCollinear             ( const Vector3&    StartPoint,
                                      const Vector3&    MidPoint,
                                      const Vector3&    EndPoint        );
    void    RedoWinding             ( working_data&     WorkingData,
                                      const plane&      PolyPlane       );
    void    BuildConvexStrip        ( working_data&     WorkingData     );
    bool   TriContainsVert         ( working_data&     WorkingData,
                                      int               P0,
                                      int               P1,
                                      int               P2,
                                      int               PointToTest     );
    bool   IsAnEar                 ( working_data&     WorkingData,
                                      triangulate_link  VertPool[MAX_VERTS_PER_DECAL],
                                      int               FirstConcave,
                                      int               VertToTest      );
    void    RemoveFromPool          ( triangulate_link  VertPool[MAX_VERTS_PER_DECAL],
                                      int&              FirstVert,
                                      int               VertToRemove    );
    void    RemoveFromConcave       ( triangulate_link  VertPool[MAX_VERTS_PER_DECAL],
                                      int&              FirstConcave,
                                      int               VertToRemove    );
    bool   IsConvex                ( working_data&     WorkingData,
                                      triangulate_link  VertPool[MAX_VERTS_PER_DECAL],
                                      int               VertToTest,
                                      const plane&      PolyPlane       );
    void    Triangulate             ( working_data&     WorkingData,
                                      const plane&      PolyPlane       );
    void    CombineCoplanarPolys    ( working_data&     WorkingData     );

    //==========================================================================
    // Friends for sorting
    //==========================================================================
    friend int DecalEdgeSortFn( const void* pA, const void* pB );

    //==========================================================================
    // The hardware-friendly data structures
    //==========================================================================
    struct position_data
    {
        //void    FileIO( fileio& File );

        Vector3p Pos;
        uint32_t      Flags;
    };

    struct uv_data
    {
        //void FileIO( fileio& File );

        int16_t     U;
        int16_t     V;
    };

    //==========================================================================
    // A file format that contains all static decals
    //==========================================================================
    struct static_data
    {
        enum { STATIC_DECAL_VERSION = 3 };

        struct package
        {
            //void FileIO( fileio& File );

            char    PackageName[256];
            int     iDefinition;
            int     nDefinitions;
        };

        struct definition
        {
            //void FileIO( fileio& File );

            int     iGroup;
            int     iDecalDef;
            int     iZoneInfo;
            int     nZones;
        };

        struct zone_info
        {
            //void FileIO( fileio& File );

            int     iVert;
            int     nVerts;
            uint32_t     Zone;
        };

                    static_data ();
        //            static_data ( fileio& File );
        //void        FileIO      ( fileio& File );

        int             Version;
        int             nPackages;
        package*        pPackage;
        int             nDefinitions;
        definition*     pDefinition;
        int             nZones;
        zone_info*      pZone;
        int             nVerts;
        position_data*  pPos;
        uv_data*        pUV;
        uint32_t*            pColor;
    };

    //==========================================================================
    // The runtime information for both dynamic and static decals.
    //==========================================================================
    struct registration_info
    {
        registration_info   (ResourceManager*);
        ~registration_info  ();
        void Kill();

        void AllocVertList          ( int nVerts );
        void GrowStaticVertListBy   ( int nVerts );
        int  GetAllocSize           ( int nVerts );
    
        int                 m_nVertsAllocated;
        int                 m_Start;
        int                 m_End;
        int                 m_Blank;
        position_data*      m_pPositions;
        uv_data*            m_pUVs;
        uint32_t*                m_pColors;
        float*                m_pElapsedTimes;
        ResourceHandle<texture>    m_Bitmap;
        uint16_t                 m_BlendMode;
        int                 m_Flags;
        float                 m_FadeoutTime;
        Colour              m_Color;

        int                 m_nStaticVertsAlloced;
        int                 m_nStaticVerts;
        position_data*      m_pStaticPositions;
        uv_data*            m_pStaticUVs;
        uint32_t*                m_pStaticColors;

        int                 m_StaticDataOffset;
    };

    enum { MAX_DECAL_RESOURCES = 128 };

    //==========================================================================
    // A queue for adding dynamic decals. Since this is an expensive operation,
    // we'll only let one get added per frame
    //==========================================================================
    struct queue_element
    {
        bool   Valid;  // whether or not this is a valid queue entry
        xhandle Handle; // handle to the registered definition;
        Vector2 Size;   // size of the decal to be added
        Radian  Roll;   // roll of the decal to be added
        Colour  Color;  // color of the decal to be added
        int     Flags;  // flags of the decal to be added
        Vector3 Point;  // point of collision
        Vector3 Normal; // surface normal
        Vector3 NegRay; // Negative incoming ray for projection
    };

    enum { DYNAMIC_QUEUE_SIZE = 8 };

    //==========================================================================
    // Internal functions for creating dynamic decals
    //==========================================================================
    int         CalcDecalVerts      ( int                     Flags,
                                      const Vector3&          Point,
                                      const Vector3&          SurfaceNormal,
                                      const Vector3&          NegIncomingRay,
                                      const Vector2&          Size,
                                      Radian                  Roll,
                                      decal_vert              Verts[MAX_VERTS_PER_DECAL],
                                      Matrix4&                L2W );
    int         GetDecalStart       ( xhandle                 RegInfoHandle,
                                      int                     nVerts );
    void        AddDecal            ( xhandle                 RegInfoHandle,
                                      int                     nVerts,
                                      decal_vert              DecalVerts[MAX_VERTS_PER_DECAL],
                                      const Matrix4&          L2W );
    void        AddClippedToQueue   ( const decal_definition& DecalDef,
                                      const Vector3&          Point,
                                      const Vector3&          Normal,
                                      const Vector3&          NegIncomingRay,
                                      const Vector2&          Size,
                                      Radian                  Roll );

    //==========================================================================
    // Internal functions for exporting static decals
    //==========================================================================
    //void        SetupExportVertBuffers  ( platform      PlatformType );

    //==========================================================================
    // Rendering functions
    //==========================================================================
    void        RenderVerts         ( int                nVerts,
                                      position_data*     pPos,
                                      uv_data*           pUV,
                                      uint32_t*               pColor );
    void        RenderEditorStatics ( registration_info& RegInfo,
                                      uint32_t                DrawFlags );
    void        RenderStaticDecals  ( registration_info& RegInfo );
    void        RenderDynamicDecals ( registration_info& RegInfo );

    //==========================================================================
    // Update functions
    //==========================================================================
    void        UpdateAlphaFade     ( float                DeltaTime,
                                      float                FadeTime,
                                      int                nVerts,
                                      uint32_t*               pColor,
                                      float*               pTimeElapsed );

    //==========================================================================
    // Finally, some data!
    //==========================================================================

    int                         m_DynamicQueueAddPos;
    int                         m_DynamicQueueReadPos;
    queue_element               m_DynamicQueue[DYNAMIC_QUEUE_SIZE];
    xharray<registration_info>  m_RegisteredDefs;
    static_data*                m_pStaticData;

    uv_data                     m_TriangleTemplateUV[3];
};

//==============================================================================

inline void decal_mgr::CreateDecalFromRayCast( const decal_definition& Def,
                                               const Vector3& Start,
                                               const Vector3& End )
{
    CreateDecalFromRayCast( Def, Start, End, Def.RandomSize(), Def.RandomRoll() );
}

//==============================================================================

inline void decal_mgr::CreateDecalAtPoint( const decal_definition& Def,
                                           const Vector3&          Point,
                                           const Vector3&          Normal )
{
    CreateDecalAtPoint( Def, Point, Normal, Def.RandomSize(), Def.RandomRoll() );
}

extern decal_mgr g_DecalMgr;
