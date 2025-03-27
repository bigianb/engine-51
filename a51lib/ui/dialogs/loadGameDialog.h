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

        void render(Renderer& renderer, int ox = 0, int oy = 0) override;

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
