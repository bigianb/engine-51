#pragma once

#include "../UIDialog.h"
#include "../UIManager.h"
#include "../UIText.h"
#include "../../VectorMath.h"

namespace ui
{
    class LoadGameDialog : public Dialog
    {
    public:
        LoadGameDialog();
        ~LoadGameDialog();

        enum slideshow_state
        {
            STATE_IDLE = 0,  // we haven't started doing anything yet
            STATE_SETUP,     // we are setting up
            STATE_SLIDESHOW, // we are doing a slideshow
            STATE_LOADING,   // the slideshow has finished, and now we're just waiting
        };

        static void registerDialog(Manager*);

        bool create(User*           user,
                    Manager*        manager,
                    DialogTemplate* dialogTemplate,
                    const IntRect&  position,
                    Window*         parent,
                    int             flags);

        void destroy() override;

        void render(Renderer& renderer, int ox = 0, int oy = 0) override;

        void onPadSelect(Window* win) override;

        void onUpdate(float deltaTime) override;

        //  Functions for doing the initial slide show setup
        void StartLoadingProcess();
        void SetTextAnimInfo(float StartTextAnim);
        void SetNSlides(int nSlides);
        void SetSlideInfo(int         Index,
                          const char* pTextureName,
                          float       StartFadeIn,
                          float       EndFadeIn,
                          float       StartFadeOut,
                          float       EndFadeOut,
                          Colour      SlideColor);
        void SetVoiceID(int VoiceID);
        void StartSlideshow();
        void LoadingComplete();

        //  Query functions for the rest of the world to use
        slideshow_state GetSlideShowState() { return m_SlideshowState; }

    protected:
        //  Internal helper functions for the effects rendering
        void UpdateLightShaftEffect(float DeltaTime);
        void CreateDropShadow(int FontIndex);
        void RenderLightShaftEffect();

        enum vram_buffer
        {
            BUFFER_SCREEN = 0,
            BUFFER_LEVEL_NAME,      // buffer for using a source texture for the name      
            BUFFER_DROP_SHADOW_1,   // buffer for doing the drop shadow creation (1/2 resolution)
            BUFFER_DROP_SHADOW_2,   // buffer for doing the drop shadow creation (1/2 resolution)
            BUFFER_COUNT
        };

        void platform_Init();
        void platform_Destroy();
        void platform_LoadSlide(int         Index,
                                int         TextureIndex,
                                const char* pTextureName);
        void platform_FillScreen(Colour C);
        void platform_RenderSlide(int    SlideIndex,
                                  Colour C);
        void platform_GetBufferInfo(vram_buffer BufferID,
                                    int&        MemOffset,
                                    int&        BufferW,
                                    int&        BufferH);
        void platform_SetSrcBuffer(vram_buffer BufferID);
        void platform_SetDstBuffer(vram_buffer BufferID,
                                   bool        EnableRGBChannel = true,
                                   bool        EnableAlphaChannel = true);
        void platform_ClearBuffer(vram_buffer BufferID,
                                  bool        EnableRGBChannel = true,
                                  bool        EnableAlphaChannel = true);
        void platform_DrawSprite(const Vector2& UpperLeft,
                                 const Vector2& Size,
                                 const Vector2& UV0,
                                 const Vector2& UV1,
                                 Colour         C,
                                 bool           Additive);
        void platform_BeginFogRender(void);
        void platform_EndFogRender(void);
        void platform_DrawFogSprite(const Vector2& SpriteCenter,
                                    const Vector2& WH,
                                    const Vector2& UV0,
                                    const Vector2& UV1,
                                    Colour         C,
                                    Radian         Rotation);
        void platform_BeginShaftRender(void);
        void platform_EndShaftRender(void);
        void platform_DrawShaftQuad(const Vector2* pCorners,
                                    const Vector2* pUVs,
                                    const Colour*  pColors);

        //  Slide data
        enum
        {
            NUM_SLIDES = 16
        };
        struct slide_info
        {
            float  StartFadeIn;
            float  EndFadeIn;
            float  StartFadeOut;
            float  EndFadeOut;
            Colour SlideColor;
            bool   HasImage;
        };

        int          m_VoiceID;
        int          m_nSlides;
        int          m_nTextures;
        slide_info   m_Slides[NUM_SLIDES];
        float        m_StartTextAnim;
        std::wstring m_NameText;

        //  State management data
        slideshow_state m_SlideshowState;
        bool            m_LoadingComplete;
        bool            m_FinalFadeoutStarted;
        float           m_ElapsedTime;
        float           m_FadeTimeElapsed;

        // data for the light shaft effect
        Bitmap* m_FogBMP; // Fog texture
        bool    m_FogLoaded;
        float   m_Position;       // Current position of light shafts
        float   m_HorizSpeed;     // Speed of light shafts
        Radian  m_FogAngle;       // Current fog rotation
        float   m_FogZoom;        // Current fog zoom
        IntRect m_LightShaftArea; // Area of screen we'll be doing effect on
    };
}
