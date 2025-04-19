
protected:

        bool       InitEffect      ( int& Index, const char* pName );
        void        KillEffect      ( int  Index );
        void        CreateEffect    ( int  Index, fx_def* pEffectDef );
        
        void        BindHandle      (       fx_handle& Handle, int Index );
        void        UnbindHandle    (       fx_handle& Handle );
                                            
        void        RestartEffect   (       fx_handle& Handle );
        void        AdvanceLogic    (       fx_handle& Handle, float DeltaTime );
        void        Render          ( const fx_handle& Handle );

        bool       IsFinished      ( const fx_handle& Handle );
        bool       IsInstanced     ( const fx_handle& Handle );
        bool       Validate        ( const fx_handle& Handle );

        void        SetScale        (       fx_handle& Handle, const Vector3& Scale       );
        void        SetRotation     (       fx_handle& Handle, const Radian3& Rotation    );
        void        SetTranslation  (       fx_handle& Handle, const Vector3& Translation );
        void        SetColor        (       fx_handle& Handle, const Colour&  Color       );
        Colour      GetColor        (       fx_handle& Handle                             );
                                            
        void        SetSuspended    (       fx_handle& Handle, bool Suspended );

const   BBox&       GetBounds       ( const fx_handle& Handle );

//------------------------------------------------------------------------------
protected:

        fx_effect_base*         m_pEffect   [ MAX_EFFECT_INSTANCES   ];
        fx_def*                 m_pEffectDef[ MAX_EFFECT_DEFINITIONS ];

static  fx_load_bitmap_fn*      m_pLoadBitmapFn;
static  fx_unload_bitmap_fn*    m_pUnloadBitmapFn;
static  fx_resolve_bitmap_fn*   m_pResolveBitmapFn;

static  int                     m_NCtrlTypes;
static  fx_ctrl_type            m_CtrlType[ MAX_CTRL_TYPES ];
                                
static  int                     m_NElementTypes;
static  fx_element_type         m_ElementType[ MAX_ELEMENT_TYPES ];

static  int                     m_SpriteBudget;
static  int                     m_SpritesThisFrame;

friend  fx_handle;
friend  fx_effect;
friend  fx_effect_base;
friend  fx_ctrl_reg;
friend  fx_element_reg;

//------------------------------------------------------------------------------
public:
        
static  void AddSpritesThisFrame( int Count )  { m_SpritesThisFrame += Count;  }
static  int  GetSpritesThisFrame()       { return( m_SpritesThisFrame ); }
static  int  GetSpriteBudget    ()       { return( m_SpriteBudget     ); }

