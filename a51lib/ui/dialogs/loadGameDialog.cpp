#include "loadGameDialog.h"
#include <cassert>

#define MAX_TEXTURES 5
#define FINAL_FADE_OUT_TIME 1.0f
#define SLIDESHOW_IMAGE_WIDTH 512
#define SLIDESHOW_IMAGE_HEIGHT 512

#define TEXT_IMAGE_WIDTH 512
#define TEXT_IMAGE_HEIGHT 64

#define TEXT_X_POSITION -50
#define TEXT_Y_POSITION 313

#define HIGHLIGHT_SCALE 1.2f

#define SHADOW_SCALE_BEGIN_TIME 0.0f
#define SHADOW_SCALE_END_TIME 1.3333333f
#define SHADOW_SCALE_BEGIN 8.6666666f
#define SHADOW_SCALE_END 1.0f
#define SHADOW_SLIDE_BEGIN_TIME 1.3333333f
#define SHADOW_SLIDE_END_TIME 5.8f
#define SHADOW_SLIDE_BEGIN_X 0
#define SHADOW_SLIDE_END_X -64
#define SHADOW_SLIDE_BEGIN_Y 0
#define SHADOW_SLIDE_END_Y 20
#define SHADOW_FADEIN_BEGIN_TIME 5.8f
#define SHADOW_FADEIN_END_TIME 6.8f

Colour g_DropShadowTweak(187, 255, 187, 255);

//=========================================================================
// defines for the light shaft animation
//=========================================================================

#define LIGHT_SHAFT_PIXEL_WIDTH 64.0f   // Source pixel width of light shafts
#define LIGHT_SHAFT_HORIZ_SCALE 0.5f    // Horizontal shaft spread amount
#define LIGHT_SHAFT_VERT_SCALE 12.0f    // Vertical shaft spread amount
#define LIGHT_SHAFT_ALPHA 0.15f         // Alpha of each layer
#define LIGHT_SHAFT_NUM_PASSES 15       // # of passes
#define LIGHT_SHAFT_FOG_ROT_SPEED R_5   // Speed of rotation
#define LIGHT_SHAFT_FOG_ALPHA 0.3f      // Alpha of each layer
#define LIGHT_SHAFT_FOG_SIZE (1024.0f)  // Max size of fog layer
#define LIGHT_SHAFT_FOG_ZOOM_SPEED 0.2f // Speed of zoom
#define LIGHT_SHAFT_NUM_FOG_PASSES 15   // # of passes

#define LIGHT_SHAFT_FOG_NAME "UI_LoadScreen_Fog.xbmp"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MINMAX(a, v, b) (MAX((a), MIN((v), (b))))

namespace ui
{
    enum controls
    {
        IDC_LEVEL_NAME,
    };

    static ControlTemplate controls[] =
        {
            {IDC_LEVEL_NAME, "IDS_NULL", "text", 0, 308, 480, 30, 0, 0, 1, 1, Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE}};

    static DialogTemplate thisDialogTemplate =
        {
            "IDS_LOAD_GAME",
            1, 9,
            sizeof(controls) / sizeof(ControlTemplate),
            &controls[0],
            0};

    static Dialog* dlg_factory(User* user, Manager* manager, DialogTemplate* dialogTemplate, const IntRect& position, Window* parent, int flags, void* userData)
    {
        LoadGameDialog* dialog = new LoadGameDialog;
        dialog->create(user, manager, dialogTemplate, position, parent, flags);

        return dialog;
    }

    void LoadGameDialog::registerDialog(Manager* manager)
    {
        manager->registerDialogClass("load game", &thisDialogTemplate, &dlg_factory);
    }

    LoadGameDialog::LoadGameDialog(void)
    {
    }

    //=========================================================================

    LoadGameDialog::~LoadGameDialog(void)
    {
        destroy();
    }

    //=========================================================================

    bool LoadGameDialog::create(User*           user,
                                Manager*        manager,
                                DialogTemplate* dialogTemplate,
                                const IntRect&  position,
                                Window*         parent,
                                int             flags)
    {
        bool success = Dialog::create(user, manager, dialogTemplate, position, parent, flags);

        state = DialogState::Active;

        // initialize the slide data
        m_VoiceID = 0;
        m_nSlides = 0;
        m_nTextures = 0;
        m_StartTextAnim = 0;
        m_NameText = L"";

        // Initialize the state management data
        m_SlideshowState = STATE_IDLE;
        m_LoadingComplete = false;
        m_FinalFadeoutStarted = false;
        m_ElapsedTime = 0.0f;
        m_FadeTimeElapsed = 0.0f;

        // initialize the light shaft effect data
        m_FogLoaded = false;
        m_Position = -0.5f;
        m_HorizSpeed = 0.15f;
        m_FogAngle = R_0;
        m_FogZoom = 0.0f;
        m_LightShaftArea.left = 0;
        m_LightShaftArea.right = 1;
        m_LightShaftArea.top = 0;
        m_LightShaftArea.bottom = 1;

        return success;
    }

    //=========================================================================

    void LoadGameDialog::destroy()
    {
        Dialog::destroy();

        // kill screen wipe
        //g_UiMgr->ResetScreenWipe();

        // kill the light shaft effect
        if (m_FogLoaded) {
            m_FogLoaded = false;

            //m_FogBMP.Kill();
        }
    }

    //=========================================================================

    void LoadGameDialog::render(Renderer& renderer, int ox, int oy)
    {
        // do nothing if the slideshow hasn't started yet
        if ((m_SlideshowState == STATE_IDLE) || (m_SlideshowState == STATE_SETUP)) {
            return;
        }

        // render a big black quad so that any text and/or screens has
        // something to blend with
        //platform_FillScreen(XCOLOR_BLACK);

        // figure out the alpha values based on the slideshow state
        int   i;
        int   SlideAlphas[NUM_SLIDES];
        int   NameAlpha = 0;
        float ShadowOffsetX = SHADOW_SLIDE_BEGIN_X;
        float ShadowOffsetY = SHADOW_SLIDE_BEGIN_Y;
        float ShadowVertScale = SHADOW_SCALE_END;
        float ShadowAlpha = 0.0f;
        memset(SlideAlphas, 0, sizeof(int) * NUM_SLIDES);
        if (m_SlideshowState == STATE_SLIDESHOW) {
            float CurrTime = m_ElapsedTime;

            for (i = 0; i < m_nSlides; i++) {
                // figure out the alpha value for this slide based on where it is
                // in comparison to the audio
                if (CurrTime < m_Slides[i].StartFadeIn) {
                    // nothing to display, and alpha is already set to zero
                } else if ((CurrTime >= m_Slides[i].StartFadeIn) && (CurrTime < m_Slides[i].EndFadeIn)) {
                    // this slide is fading in, figure out where it is in the fade
                    float FadeDuration = m_Slides[i].EndFadeIn - m_Slides[i].StartFadeIn;
                    if (FadeDuration > 0.0f) {
                        SlideAlphas[i] = (int)(255.0f * (CurrTime - m_Slides[i].StartFadeIn) / FadeDuration);
                        SlideAlphas[i] = MINMAX(0, SlideAlphas[i], 255);
                    }
                } else if ((CurrTime >= m_Slides[i].EndFadeIn) && (CurrTime < m_Slides[i].StartFadeOut)) {
                    // This slide is fully faded in and hasn't started fading out yet
                    SlideAlphas[i] = 255;
                } else if ((CurrTime >= m_Slides[i].StartFadeOut) && (CurrTime < m_Slides[i].EndFadeOut)) {
                    // This slide is fading out. Figure out how far along in its fade it should be.
                    float FadeDuration = m_Slides[i].EndFadeOut - m_Slides[i].StartFadeOut;
                    if (FadeDuration > 0.0f) {
                        SlideAlphas[i] = 255 - (int)(255.0f * (CurrTime - m_Slides[i].StartFadeOut) / FadeDuration);
                        SlideAlphas[i] = MINMAX(0, SlideAlphas[i], 255);
                    }
                }
            }

            // figure out the drop shadow offset
            if (CurrTime > m_StartTextAnim) {
                float TextAnimTime = CurrTime - m_StartTextAnim;
                if (TextAnimTime < SHADOW_SCALE_END_TIME) {
                    float T = (TextAnimTime - SHADOW_SCALE_BEGIN_TIME) / (SHADOW_SCALE_END_TIME - SHADOW_SCALE_BEGIN_TIME);
                    ShadowVertScale = SHADOW_SCALE_BEGIN + T * (SHADOW_SCALE_END - SHADOW_SCALE_BEGIN);
                    ShadowAlpha = T;
                    NameAlpha = (int)(255.0f * T);
                    NameAlpha = MINMAX(0, NameAlpha, 255);
                } else if (TextAnimTime < SHADOW_SLIDE_END_TIME) {
                    float T = (TextAnimTime - SHADOW_SLIDE_BEGIN_TIME) / (SHADOW_SLIDE_END_TIME - SHADOW_SLIDE_BEGIN_TIME);
                    ShadowAlpha = 1.0f - T;
                    ShadowOffsetX = SHADOW_SLIDE_BEGIN_X + T * (SHADOW_SLIDE_END_X - SHADOW_SLIDE_BEGIN_X);
                    ShadowOffsetY = SHADOW_SLIDE_BEGIN_Y + T * (SHADOW_SLIDE_END_Y - SHADOW_SLIDE_BEGIN_Y);
                    NameAlpha = 255;
                } else if (TextAnimTime < SHADOW_FADEIN_END_TIME) {
                    float T = (TextAnimTime - SHADOW_FADEIN_BEGIN_TIME) / (SHADOW_FADEIN_END_TIME - SHADOW_FADEIN_BEGIN_TIME);
                    ShadowAlpha = T;
                    NameAlpha = 255;
                    ShadowVertScale = HIGHLIGHT_SCALE;
                } else {
                    ShadowAlpha = 1.0f;
                    NameAlpha = 255;
                    ShadowVertScale = HIGHLIGHT_SCALE;
                }
            }
        } else {
            // We must be in the loading state or we never got proper data
            // set up. So just render the name full on with a highlight
            // scrolling across to show that we haven't crashed.
            NameAlpha = 255;
            ShadowOffsetX = SHADOW_SLIDE_BEGIN_X;
            ShadowOffsetY = SHADOW_SLIDE_BEGIN_Y;
            ShadowVertScale = HIGHLIGHT_SCALE;
            ShadowAlpha = 1.0f;
        }

        for (i = 0; i < m_nSlides; i++) {
            if (SlideAlphas[i] > 0) {
                Colour C(m_Slides[i].SlideColor.r,
                         m_Slides[i].SlideColor.g,
                         m_Slides[i].SlideColor.b,
                         SlideAlphas[i]);

                if (m_Slides[i].HasImage) {
                    //               platform_RenderSlide(i, C);
                } else {
                    //               platform_FillScreen(C);
                }
            }
        }

        if (NameAlpha > 0) {
            /**
            // figure out which font we're using
            int FontIndex = g_UiMgr->FindFont("loadscr");
            ASSERT(FontIndex >= 0);

            // create a drop shadow to be placed under the text
            CreateDropShadow(FontIndex);

            // clear the level name buffer in preparation for what we're about to do
            platform_ClearBuffer(BUFFER_LEVEL_NAME);
            platform_ClearBuffer(BUFFER_DROP_SHADOW_2);

            if (m_SlideshowState == STATE_LOADING) {
                // render the drop shadow to the name texture
                vector2 Center = vector2(TEXT_IMAGE_WIDTH / 2, TEXT_IMAGE_HEIGHT / 2);
                vector2 UL = Center - vector2(TEXT_IMAGE_WIDTH / 2, ShadowVertScale * TEXT_IMAGE_HEIGHT / 2);
                u8      ShadA = (u8)(255.0f * ShadowAlpha);
                platform_SetSrcBuffer(BUFFER_DROP_SHADOW_1);
                platform_SetDstBuffer(BUFFER_LEVEL_NAME);
                platform_DrawSprite(UL,
                                    vector2(TEXT_IMAGE_WIDTH, ShadowVertScale * TEXT_IMAGE_HEIGHT),
                                    vector2(0.0f, 0.0f), vector2(1.0f, 1.0f),
                                    xcolor(g_DropShadowTweak.R, g_DropShadowTweak.G, g_DropShadowTweak.B, ShadA),
                                    true);

                // render the text to the name texture
                irect Rect;
                Rect.l = 0;
                Rect.r = TEXT_IMAGE_WIDTH - 16; // leave 16 so there is room to blur
                Rect.t = 0;
                Rect.b = TEXT_IMAGE_HEIGHT - 16; // leave 16 so there is room to blur
                platform_SetDstBuffer(BUFFER_LEVEL_NAME);
                g_UiMgr->RenderText(FontIndex, Rect, ui_font::h_right | ui_font::v_bottom, XCOLOR_WHITE, m_NameText);

                // render the light shaft effect
                RenderLightShaftEffect();
            } else {
                // render the drop shadow to the screen
                vector2 Center = vector2(TEXT_X_POSITION + ShadowOffsetX, TEXT_Y_POSITION + ShadowOffsetY) +
                                 vector2(TEXT_IMAGE_WIDTH / 2, TEXT_IMAGE_HEIGHT / 2);
                vector2 UL = Center - vector2(TEXT_IMAGE_WIDTH / 2, ShadowVertScale * TEXT_IMAGE_HEIGHT / 2);
                u8      ShadA = (u8)(255.0f * ShadowAlpha);
                platform_SetSrcBuffer(BUFFER_DROP_SHADOW_1);
                platform_SetDstBuffer(BUFFER_SCREEN);
                platform_DrawSprite(UL, vector2(TEXT_IMAGE_WIDTH, TEXT_IMAGE_HEIGHT * ShadowVertScale),
                                    vector2(0.0f, 0.0f), vector2(1.0f, 1.0f),
                                    xcolor(g_DropShadowTweak.R, g_DropShadowTweak.G, g_DropShadowTweak.B, ShadA),
                                    true);

                // render the text to the name texture
                irect Rect;
                Rect.l = 0;
                Rect.r = TEXT_IMAGE_WIDTH - 16; // leave 16 so there is room to blur
                Rect.t = 0;
                Rect.b = TEXT_IMAGE_HEIGHT - 16; // leave 16 so there is room to blur
                platform_SetDstBuffer(BUFFER_LEVEL_NAME);
                g_UiMgr->RenderText(FontIndex, Rect, ui_font::h_right | ui_font::v_bottom, XCOLOR_WHITE, m_NameText);

                // render the text to the screen
                platform_SetSrcBuffer(BUFFER_LEVEL_NAME);
                platform_SetDstBuffer(BUFFER_SCREEN);
                UL.Set(TEXT_X_POSITION, TEXT_Y_POSITION);
                platform_DrawSprite(UL,
                                    vector2(TEXT_IMAGE_WIDTH, TEXT_IMAGE_HEIGHT),
                                    vector2(0.0f, 0.0f),
                                    vector2(1.0f, 1.0f),
                                    xcolor(255, 255, 255, NameAlpha),
                                    true);
            }
                                    */
        }

        if (m_FinalFadeoutStarted) {
            float T = m_FadeTimeElapsed / FINAL_FADE_OUT_TIME;
            T = MINMAX(0.0f, T, 1.0f);
            uint8_t A = (uint8_t)(255.0f * T);

            //platform_FillScreen(xcolor(0, 0, 0, A));
        }
    }

    void LoadGameDialog::onPadSelect(Window* win)
    {
        // skip to the loading state if we're not already there
        if (m_SlideshowState == STATE_SLIDESHOW) {
            // kill the audio
            //if (g_AudioMgr.IsValidVoiceId(m_VoiceID)) {
            //    g_AudioMgr.Release(m_VoiceID, 0.0f);
            // }

            // go into the the loading state
            m_SlideshowState = STATE_LOADING;
            m_ElapsedTime = 0.0f;
        }
    }

    //=========================================================================

    void LoadGameDialog::onUpdate(float DeltaTime)
    {
        // handle the various state updates
        if (m_SlideshowState == STATE_SLIDESHOW) {
            /*
            if (!g_AudioMgr.IsValidVoiceId(m_VoiceID)) {
                // If we don't have a valid voice ID, then we must increment
                // the elapsed time manually.
                m_ElapsedTime += DeltaTime;
            } else {
                // otherwise get the elapsed time from the audio
                m_ElapsedTime = g_AudioMgr.GetCurrentPlayTime(m_VoiceID);
            }
*/
            // Have we finished loading and are we at least far enough in the
            // name animation that we can start fading the screen out with
            // it still looking nice?
            if (m_LoadingComplete &&
                !m_FinalFadeoutStarted &&
                m_ElapsedTime > (m_StartTextAnim + SHADOW_SLIDE_BEGIN_TIME)) {
                m_FinalFadeoutStarted = true;
            }

            // we can transition to the loading state, iff
            // a) We've passed the point where the shadow has faded back
            //    in in preparation for the loading state, AND
            // b) the last slide has completely faded out
            int   i;
            float EndTime = m_StartTextAnim + SHADOW_FADEIN_END_TIME;
            for (i = 0; i < m_nSlides; i++) {
                EndTime = MAX(EndTime, m_Slides[i].EndFadeOut);
            }

            // Transition to the "loading" state?
            if (m_ElapsedTime > EndTime) {
                m_SlideshowState = STATE_LOADING;
                m_ElapsedTime = 0.0f;
            }
        } else if (m_SlideshowState == STATE_LOADING) {
            // just update the elapsed time
            m_ElapsedTime += DeltaTime;

            // Have we finished loading? If so, then start fading out the screen
            // so we can get into the game!
            if (m_LoadingComplete && !m_FinalFadeoutStarted) {
                m_FinalFadeoutStarted = true;
            }

            // And update the light shaft effect
            UpdateLightShaftEffect(DeltaTime);
        }

        // Now update the final screen fade-out (we'll keep whatever animation
        // that happens to be going, but fade the whole thing out so we get a nice
        // smooth transition regardless of what we were doing before).
        if (m_FinalFadeoutStarted) {
            m_FadeTimeElapsed += DeltaTime;
            // We're tacking on a little extra time to allow the fade to reach
            // total black
            if (m_FadeTimeElapsed > (FINAL_FADE_OUT_TIME + 0.11f)) {
                state = DialogState::Exit;
            }
        }
    }

    //=========================================================================

    void LoadGameDialog::StartLoadingProcess(void)
    {
        // advance the state
        assert(m_SlideshowState == STATE_IDLE);
        m_SlideshowState = STATE_SETUP;

        // do platform-specific initialization
        //platform_Init();
        /*
                // figure out the name text
                m_NameText.Clear();
                m_NameText += g_ActiveConfig.GetLevelName();

                int i;
                for (i = 0; i < m_NameText.GetLength(); i++) {
                    if (m_NameText[i] == '\\') {
                        // HACK HACK - We shouldn't need to do this, but the font doesn't have a backslash
                        // right now, and this will work around the problem so we don't assert.
                        m_NameText[i] = '/';
                    }
                }

                // Load the light shaft fog image
                m_FogLoaded = m_FogBMP.Load(xfs("%s\\%s", g_RscMgr.GetRootDirectory(),
                                                LIGHT_SHAFT_FOG_NAME));

                // figure out the area of the screen that we should perform the light
                // shaft effect on
                int FontIndex = g_UiMgr->FindFont("loadscr");

                IntRect Rect;
                getUIManger()->textSize(FontIndex, Rect, m_NameText, m_NameText.GetLength());
                m_LightShaftArea.right = TEXT_X_POSITION + TEXT_IMAGE_WIDTH;
                m_LightShaftArea.left = m_LightShaftArea.right - (Rect.right - Rect.left) - 32; // -32 pads for blurring
                m_LightShaftArea.top = TEXT_Y_POSITION;
                m_LightShaftArea.bottom = TEXT_Y_POSITION + TEXT_IMAGE_HEIGHT;
        */
    }

    //=========================================================================

    void LoadGameDialog::SetTextAnimInfo(float StartTextAnim)
    {
        assert(m_SlideshowState == STATE_SETUP);
        m_StartTextAnim = StartTextAnim;
    }

    //=========================================================================

    void LoadGameDialog::SetVoiceID(int VoiceID)
    {
        assert(m_SlideshowState == STATE_SETUP);
        m_VoiceID = VoiceID;
    }

    //=========================================================================

    void LoadGameDialog::SetNSlides(int nSlides)
    {
        assert(m_SlideshowState == STATE_SETUP);
        m_nSlides = nSlides;

        int i;
        for (i = 0; i < m_nSlides; i++) {
            m_Slides[i].StartFadeIn = 0.0f;
            m_Slides[i].EndFadeIn = 1.0f;
            m_Slides[i].StartFadeOut = 2.0f;
            m_Slides[i].EndFadeOut = 3.0f;
            m_Slides[i].SlideColor = COLOR_WHITE;
            m_Slides[i].HasImage = false;
        }
    }

    //=========================================================================

    void LoadGameDialog::SetSlideInfo(int         Index,
                                      const char* pTextureName,
                                      float       StartFadeIn,
                                      float       EndFadeIn,
                                      float       StartFadeOut,
                                      float       EndFadeOut,
                                      Colour      SlideColor)
    {
        assert(m_SlideshowState == STATE_SETUP);
        /*
                assert((Index >= 0) && (Index < m_nSlides));
                m_Slides[Index].StartFadeIn = StartFadeIn;
                m_Slides[Index].EndFadeIn = EndFadeIn;
                m_Slides[Index].StartFadeOut = StartFadeOut;
                m_Slides[Index].EndFadeOut = EndFadeOut;
                m_Slides[Index].SlideColor = SlideColor;
                if (pTextureName && stricmp(pTextureName, "<NULL>")) {
                    m_Slides[Index].HasImage = true;
                    platform_LoadSlide(Index, m_nTextures, pTextureName);
                    m_nTextures++;
                } else {
                    m_Slides[Index].HasImage = false;
                }
        */
    }

    //=========================================================================

    void LoadGameDialog::StartSlideshow(void)
    {
        // advance the state
        assert(m_SlideshowState == STATE_SETUP);
        m_SlideshowState = STATE_SLIDESHOW;
        m_ElapsedTime = 0.0f;

        // if we have no slides, skip to the loading state
        if (m_nSlides == 0) {
            m_SlideshowState = STATE_LOADING;
        }
    }

    //=========================================================================

    void LoadGameDialog::LoadingComplete(void)
    {
        m_LoadingComplete = true;

        if (m_SlideshowState == STATE_IDLE) {
            state = DialogState::Exit;
        }
    }

    //=========================================================================

    void LoadGameDialog::UpdateLightShaftEffect(float DeltaTime)
    {
        // Update rotation
        m_FogAngle += DeltaTime * LIGHT_SHAFT_FOG_ROT_SPEED;

        // Update pos
        m_Position += m_HorizSpeed * DeltaTime;
        if (m_Position > 1.5f) {
            m_Position = 1.5f;
            m_HorizSpeed = -m_HorizSpeed;
        } else if (m_Position < -0.5f) {
            m_Position = -0.5f;
            m_HorizSpeed = -m_HorizSpeed;
        }

        // Update zoom
        m_FogZoom -= LIGHT_SHAFT_FOG_ZOOM_SPEED * DeltaTime;
        if (m_FogZoom < 0.0f) {
            m_FogZoom += 1.0f;
        }
    }

    //=========================================================================

    void LoadGameDialog::CreateDropShadow(int FontIndex)
    {
        int i;
        /*
                // clear the buffers to start
                platform_ClearBuffer(BUFFER_LEVEL_NAME);
                platform_ClearBuffer(BUFFER_DROP_SHADOW_1);
                platform_ClearBuffer(BUFFER_DROP_SHADOW_2);

                // render the text to the level name buffer
                irect Rect;
                Rect.l = 0;
                Rect.r = TEXT_IMAGE_WIDTH - 16; // leave 16 so there is room to blur
                Rect.t = 0;
                Rect.b = TEXT_IMAGE_HEIGHT - 16; // leave 16 so there is room to blur
                platform_SetDstBuffer(BUFFER_LEVEL_NAME);
                g_UiMgr->RenderText(FontIndex, Rect, ui_font::h_right | ui_font::v_bottom, XCOLOR_WHITE, m_NameText);

                // now shrink the image as the first step in blurring
                platform_SetSrcBuffer(BUFFER_LEVEL_NAME);
                platform_SetDstBuffer(BUFFER_DROP_SHADOW_1);
                platform_DrawSprite(vector2(-0.5f, -0.5f),
                                    vector2(TEXT_IMAGE_WIDTH / 2, TEXT_IMAGE_HEIGHT / 2),
                                    vector2(0.0f, 0.0f),
                                    vector2(1.0f, 1.0f),
                                    XCOLOR_WHITE,
                                    true);

                // now blur the image horizontally
                platform_SetSrcBuffer(BUFFER_DROP_SHADOW_1);
                platform_SetDstBuffer(BUFFER_DROP_SHADOW_2);
                static int HorzAlphas[] = {16, 48, 64, 70, 64, 48, 16};
                static int HorzOffsets[] = {-3, -2, -1, 0, 1, 2, 3};
                for (i = 0; i < 7; i++) {
                    platform_DrawSprite(vector2(HorzOffsets[i] - 0.5f, -0.5f),
                                        vector2(TEXT_IMAGE_WIDTH / 2, TEXT_IMAGE_HEIGHT / 2),
                                        vector2(0.0f, 0.0f), vector2(1.0f, 1.0f),
                                        xcolor(255, 255, 255, HorzAlphas[i]),
                                        true);
                }

                // now blur the image vertically
                platform_ClearBuffer(BUFFER_DROP_SHADOW_1);
                platform_SetSrcBuffer(BUFFER_DROP_SHADOW_2);
                platform_SetDstBuffer(BUFFER_DROP_SHADOW_1);
                static int VertAlphas[] = {16, 48, 64, 70, 64, 48, 16};
                static int VertOffsets[] = {-3, -2, -1, 0, 1, 2, 3};
                for (i = 0; i < 7; i++) {
                    platform_DrawSprite(vector2(-0.5f, VertOffsets[i] - 0.5f),
                                        vector2(TEXT_IMAGE_WIDTH / 2, TEXT_IMAGE_HEIGHT / 2),
                                        vector2(0.0f, 0.0f), vector2(1.0f, 1.0f),
                                        xcolor(255, 255, 255, VertAlphas[i]),
                                        true);
                }
            */
    }

    //==============================================================================

    void LoadGameDialog::RenderLightShaftEffect(void)
    {
        // we can't do the effect if we don't have a proper fog texture
        if (!m_FogLoaded) {
            return;
        }
        /*
                // Compute the section we'll send light through
                float     w = (float)(m_LightShaftArea.right - m_LightShaftArea.left);
                float     h = (float)(m_LightShaftArea.bottom - m_LightShaftArea.top);
                float     c = m_Position * (float)(w - LIGHT_SHAFT_PIXEL_WIDTH);
                Vector2 FogCenter(m_LightShaftArea.left + c + LIGHT_SHAFT_PIXEL_WIDTH * 0.5f,
                                  m_LightShaftArea.top + h * 0.5f);

                // Render rotating, zooming fog into alpha channel. Each layer gets bigger and more transparent
                platform_BeginFogRender();
                int i;
                for (i = 0; i < LIGHT_SHAFT_NUM_FOG_PASSES; i++) {
                    // Lookup shaft # with zoom
                    float nShaft = i + (m_FogZoom * (float)LIGHT_SHAFT_NUM_FOG_PASSES);
                    while (nShaft >= (float)LIGHT_SHAFT_NUM_FOG_PASSES) {
                        nShaft -= (float)LIGHT_SHAFT_NUM_FOG_PASSES;
                    }

                    // Compute color
                    float T = nShaft / (float)LIGHT_SHAFT_NUM_FOG_PASSES;
                    float InvT = 1.0f - T;
                    uint8_t  Col = (uint8_t)(InvT * LIGHT_SHAFT_FOG_ALPHA * 255.0f);

                    // Compute rotation speed and radius
                    float Speed = T;
                    float R = LIGHT_SHAFT_FOG_SIZE * Speed;
                    float U = 0.0f;

                    // Flip direction?
                    if (i & 1) {
                        Speed = -Speed;
                        U = 1.0f;
                    }

                    // Draw the fog layer
                    platform_DrawFogSprite(FogCenter,
                                           vector2(R, R),
                                           vector2(U, 0.0f),
                                           vector2(1.0f - U, 1.0f),
                                           xcolor(Col, Col, Col, Col),
                                           (nShaft * R_5) + (m_FogAngle * Speed));
                }
                platform_EndFogRender();

                // now just draw the main image over the screen, but don't clobber the
                // alpha channel at all
                platform_SetSrcBuffer(BUFFER_LEVEL_NAME);
                platform_SetDstBuffer(BUFFER_SCREEN, true, false);
                Vector2 UL(TEXT_X_POSITION, TEXT_Y_POSITION);
                platform_DrawSprite(UL,
                                    Vector2(TEXT_IMAGE_WIDTH, TEXT_IMAGE_HEIGHT),
                                    vector2(0.0f, 0.0f),
                                    vector2(1.0f, 1.0f),
                                    COLOR_WHITE,
                                    true);

                // Compute section of light shaft to draw
                float X0 = (float)m_LightShaftArea.l + m_Position * (w - (float)LIGHT_SHAFT_PIXEL_WIDTH);
                float X1 = X0 + (float)LIGHT_SHAFT_PIXEL_WIDTH;
                float U0 = (X0 - TEXT_X_POSITION) / (float)TEXT_IMAGE_WIDTH;
                float U1 = (X1 - TEXT_X_POSITION) / (float)TEXT_IMAGE_WIDTH;

                // Render light shafts by rendering layers of scaled up text (each layer more transparent)
                // To remove abrupt horizontal lines, the layers are rendered with 3 quads, with the left/right
                // quads having a completely transparent edge.
                platform_BeginShaftRender();
                for (i = 0; i < LIGHT_SHAFT_NUM_PASSES; i++) {
                    // Render single light shaft ( multiply by alpha in frame buffer)
                    float    T = (float)i / (float)LIGHT_SHAFT_NUM_PASSES;
                    float    InvT = 1.0f - T;
                    uint8_t     A = (uint8_t)(255.0f * LIGHT_SHAFT_ALPHA * InvT);
                    float    WO = w * (T * LIGHT_SHAFT_HORIZ_SCALE);
                    float    HO = h * (T * LIGHT_SHAFT_VERT_SCALE);
                    Colour MidCol(A, A, A, A);
                    Colour EdgeCol(0, 0, 0, 0);

                    // Compute rect values
                    float u0 = U0;
                    float u1 = U1;
                    float v0 = 0.0f;
                    float v1 = 1.0f;
                    float x0 = (X0 - WO);
                    float y0 = (float)m_LightShaftArea.t - HO;
                    float x1 = (X1 + WO);
                    float y1 = (float)m_LightShaftArea.t + (h + HO);

                    // Compute interior edge values
                    float F = 0.2f;
                    float xi0 = x0 + (F * (x1 - x0));
                    float ui0 = u0 + (F * (u1 - u0));
                    float xi1 = x0 + ((1.0f - F) * (x1 - x0));
                    float ui1 = u0 + ((1.0f - F) * (u1 - u0));

                    Colour  Colors[4];
                    Vector2 UVs[4];
                    Vector2 Corners[4];

                    // Draw left quad (fades from transparent -> opaque)
                    Colors[0] = EdgeCol;
                    UVs[0].Set(u0, v0);
                    Corners[0].set(x0, y0);
                    Colors[1] = MidCol;
                    UVs[1].Set(ui0, v0);
                    Corners[1].Set(xi0, y0);
                    Colors[2] = MidCol;
                    UVs[2].Set(ui0, v1);
                    Corners[2].Set(xi0, y1);
                    Colors[3] = EdgeCol;
                    UVs[3].Set(u0, v1);
                    Corners[3].Set(x0, y1);
                    platform_DrawShaftQuad(Corners, UVs, Colors);

                    // Draw mid (all opaque)
                    Colors[0] = MidCol;
                    UVs[0].Set(ui0, v0);
                    Corners[0].Set(xi0, y0);
                    Colors[1] = MidCol;
                    UVs[1].Set(ui1, v0);
                    Corners[1].Set(xi1, y0);
                    Colors[2] = MidCol;
                    UVs[2].Set(ui1, v1);
                    Corners[2].Set(xi1, y1);
                    Colors[3] = MidCol;
                    UVs[3].Set(ui0, v1);
                    Corners[3].Set(xi0, y1);
                    platform_DrawShaftQuad(Corners, UVs, Colors);

                    // Draw right quad (fades from opaque -> transparent)
                    Colors[0] = MidCol;
                    UVs[0].Set(ui1, v0);
                    Corners[0].Set(xi1, y0);
                    Colors[1] = EdgeCol;
                    UVs[1].Set(u1, v0);
                    Corners[1].Set(x1, y0);
                    Colors[2] = EdgeCol;
                    UVs[2].Set(u1, v1);
                    Corners[2].Set(x1, y1);
                    Colors[3] = MidCol;
                    UVs[3].Set(ui1, v1);
                    Corners[3].Set(xi1, y1);
                    platform_DrawShaftQuad(Corners, UVs, Colors);
                }
                platform_EndShaftRender();
        */
    }

    void LoadGameDialog::platform_LoadSlide(int         Index,
                                            int         TextureIndex,
                                            const char* pTextureName)
    {
        /*
                // Other targets we don't really care about giving up the memory.
                // Just load it as though it were any other image.
                bool Success = m_Slides[Index].BMP.Load(xfs("%s\\%s", g_RscMgr.GetRootDirectory(), pTextureName));
                if (Success) {
                    vram_Register(m_Slides[Index].BMP);
                } else {
                    m_Slides[Index].HasImage = false;
                }
        */
    }

    //==============================================================================

    void LoadGameDialog::platform_FillScreen(Colour C)
    {

        // IntRect Rect(0, 0, g_PhysW, g_PhysH);
        // draw_Rect(Rect, C, false);
    }

    //=========================================================================

    void LoadGameDialog::platform_RenderSlide(int SlideIndex, Colour C)
    {
        /*
                // start up the drawing mode
                draw_EnableBilinear();
                draw_Begin(DRAW_SPRITES, DRAW_USE_ALPHA | DRAW_TEXTURED | DRAW_2D | DRAW_NO_ZBUFFER | DRAW_NO_ZWRITE | DRAW_BLEND_ADD);

                // activate the sprite texture
                draw_SetTexture(m_Slides[SlideIndex].BMP);

                // draw the sprite
                draw_Sprite(vector3(0.0f, 0.0f, 0.0f),
                            vector2((float)g_PhysW, (float)g_PhysH),
                            C);

                // finished
                draw_End();
                */
    }

    //==============================================================================

    void LoadGameDialog::platform_GetBufferInfo(vram_buffer BufferID,
                                                int&        MemOffset,
                                                int&        BufferW,
                                                int&        BufferH)
    {
        switch (BufferID) {
        default:
            assert(false);
            break;
        case BUFFER_SCREEN:
            MemOffset = 0;
            //BufferW = g_PhysW;
            //BufferH = g_PhysH;
            break;
        case BUFFER_LEVEL_NAME:
            MemOffset = 0;
            BufferW = TEXT_IMAGE_WIDTH;
            BufferH = TEXT_IMAGE_HEIGHT;
            break;
        case BUFFER_DROP_SHADOW_1:
            MemOffset = TEXT_IMAGE_WIDTH * TEXT_IMAGE_HEIGHT * 4;
            BufferW = TEXT_IMAGE_WIDTH / 2;
            BufferH = TEXT_IMAGE_HEIGHT / 2;
            break;
        case BUFFER_DROP_SHADOW_2:
            MemOffset = TEXT_IMAGE_WIDTH * TEXT_IMAGE_HEIGHT * 4 +
                        (TEXT_IMAGE_WIDTH / 2) * (TEXT_IMAGE_HEIGHT / 2) * 4;
            BufferW = TEXT_IMAGE_WIDTH / 2;
            BufferH = TEXT_IMAGE_HEIGHT / 2;
            break;
        }
    }

    //=============================================================================

    void LoadGameDialog::platform_SetSrcBuffer(vram_buffer BufferID)
    {
        assert(BufferID != BUFFER_SCREEN);

        // figure out the buffer info
        int BufferW;
        int BufferH;
        int MemOffset;
        platform_GetBufferInfo(BufferID, MemOffset, BufferW, BufferH);
        /*
                // Get base of tiled memory
                uint8_t* BaseTiledPtr = (uint8_t*)g_TextureFactory.GetTiledPool().GetBase();
                BaseTiledPtr += MemOffset;

                // alias some other texture buffer memory from the system
                texture_factory::handle Handle = g_TextureFactory.Alias(
                    BufferW * 4,
                    (u32)BufferW,
                    (u32)BufferH,
                    D3DFMT_A8R8G8B8,
                    BaseTiledPtr,
                    texture_factory::ALIAS_FROM_SCRATCH);

                assert(Handle);
                g_Texture.Set(0, Handle);
        */
    }

    //==============================================================================

    void LoadGameDialog::platform_SetDstBuffer(vram_buffer BufferID,
                                               bool        EnableRGBChannel,
                                               bool        EnableAlphaChannel)
    {
        /*
        if (BufferID == BUFFER_SCREEN) {
            g_pPipeline->SetRenderTarget(pipeline_mgr::kLAST, -1);
            m_BufferW = g_PhysW;
            m_BufferH = g_PhysH;
        } else {
            // figure out the buffer info
            int MemOffset;
            platform_GetBufferInfo(BufferID, MemOffset, m_BufferW, m_BufferH);

            // Get base of tiled memory
            u8* BaseTiledPtr = (u8*)g_TextureFactory.GetTiledPool().GetBase();
            BaseTiledPtr += MemOffset;

            // alias some other texture buffer memory from the system
            texture_factory::handle Handle = g_TextureFactory.Alias(
                m_BufferW * 4,
                (u32)m_BufferW,
                (u32)m_BufferH,
                D3DFMT_A8R8G8B8,
                BaseTiledPtr,
                texture_factory::ALIAS_FROM_SCRATCH);
            ASSERT(Handle);
            IDirect3DSurface8* Surface;
            if (Handle) {
                Handle->GetSurfaceLevel(0, &Surface);
            }
            g_RenderTarget.Set(Surface, NULL);
            Surface->Release();
        }

        // set up the color write mask
        m_ColorWriteMask = 0;
        if (EnableRGBChannel) {
            m_ColorWriteMask |= D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE;
        }
        if (EnableAlphaChannel) {
            m_ColorWriteMask |= D3DCOLORWRITEENABLE_ALPHA;
        }
            */
    }

    //==============================================================================

    void LoadGameDialog::platform_ClearBuffer(vram_buffer BufferID, bool EnableRGBChannel, bool EnableAlphaChannel)
    {
        /*
        platform_SetDstBuffer(BufferID, EnableRGBChannel, EnableAlphaChannel);
        u32 Flags = 0;
        if (EnableRGBChannel) {
            Flags |= D3DCLEAR_TARGET_R | D3DCLEAR_TARGET_G | D3DCLEAR_TARGET_B;
        }
        if (EnableAlphaChannel) {
            Flags |= D3DCLEAR_TARGET_A;
        }
        g_pd3dDevice->Clear(0, 0, Flags, 0, 0.0f, 0);
        */
    }

    //=========================================================================

    void LoadGameDialog::platform_DrawSprite(const Vector2& UpperLeft,
                                             const Vector2& Size,
                                             const Vector2& UV0,
                                             const Vector2& UV1,
                                             Colour         C,
                                             bool           Additive)
    {
        /*
        u32 DrawFlags = DRAW_2D | DRAW_USE_ALPHA | DRAW_TEXTURED | DRAW_NO_ZBUFFER | DRAW_NO_ZWRITE;
        if (Additive) {
            DrawFlags |= DRAW_BLEND_ADD;
        }
        draw_Begin(DRAW_SPRITES, DrawFlags);
        draw_SpriteImmediate(UpperLeft,
                             Size,
                             UV0,
                             UV1,
                             C);
        draw_End();
        */
    }

    //=========================================================================

    void LoadGameDialog::platform_BeginFogRender()
    {

        // make sure the screen is cleared to start
        platform_ClearBuffer(BUFFER_SCREEN, false, true);

        // render to the screen, but mask out everything except alpha
        platform_SetDstBuffer(BUFFER_SCREEN, false, true);
        /*
                // begin drawing
                draw_Begin(DRAW_SPRITES, DRAW_USE_ALPHA |
                                             DRAW_TEXTURED |
                                             DRAW_2D |
                                             DRAW_NO_ZBUFFER |
                                             DRAW_NO_ZWRITE |
                                             DRAW_BLEND_ADD |
                                             DRAW_XBOX_NO_BEGIN);
                g_RenderState.Set(D3DRS_COLORWRITEENABLE, m_ColorWriteMask);
                draw_Begin(DRAW_SPRITES, DRAW_KEEP_STATES);
                draw_SetTexture(m_FogBMP);
        */
    }

    //=========================================================================

    void LoadGameDialog::platform_EndFogRender()
    {
        // end drawing
        //draw_End();
    }

    //=========================================================================

    void LoadGameDialog::platform_DrawFogSprite(const Vector2& SpriteCenter,
                                                const Vector2& WH,
                                                const Vector2& UV0,
                                                const Vector2& UV1,
                                                Colour         C,
                                                Radian         Rotation)
    {
        /*
        draw_SpriteUV(Vector3(SpriteCenter.X, SpriteCenter.Y, 0.0f),
                      WH,
                      UV0,
                      UV1,
                      C,
                      Rotation);
                      */
    }

    //=========================================================================

    void LoadGameDialog::platform_BeginShaftRender()
    {
        // set the level name as our texture, and the screen as our render
        // target, but mask out writing to the alpha channel
        platform_SetDstBuffer(BUFFER_SCREEN, true, false);
        platform_SetSrcBuffer(BUFFER_LEVEL_NAME);
        /*
                draw_Begin(DRAW_QUADS, DRAW_CULL_NONE |
                                           DRAW_2D |
                                           DRAW_NO_ZBUFFER |
                                           DRAW_TEXTURED |
                                           DRAW_USE_ALPHA |
                                           DRAW_BLEND_ADD |
                                           DRAW_U_CLAMP |
                                           DRAW_V_CLAMP |
                                           DRAW_XBOX_NO_BEGIN);

                g_RenderState.Set(D3DRS_ALPHABLENDENABLE, true);
                g_RenderState.Set(D3DRS_BLENDOP, D3DBLENDOP_ADD);
                g_RenderState.Set(D3DRS_SRCBLEND, D3DBLEND_DESTALPHA);
                g_RenderState.Set(D3DRS_DESTBLEND, D3DBLEND_ONE);
                g_RenderState.Set(D3DRS_COLORWRITEENABLE, m_ColorWriteMask);
                g_TextureStageState.Set(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
                g_TextureStageState.Set(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                g_TextureStageState.Set(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
                g_TextureStageState.Set(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
                g_TextureStageState.Set(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                g_TextureStageState.Set(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);

                draw_Begin(DRAW_QUADS, DRAW_KEEP_STATES);
        */
    }

    //=========================================================================

    void LoadGameDialog::platform_EndShaftRender()
    {
        //draw_End();
    }

    //=========================================================================

    void LoadGameDialog::platform_DrawShaftQuad(const Vector2* pCorners,
                                                const Vector2* pUVs,
                                                const Colour*  pColors)
    {
        /*
                draw_Color(pColors[0]);
                draw_UV(pUVs[0]);
                draw_Vertex(pCorners[0].X, pCorners[0].Y, 0.0f);
                draw_Color(pColors[1]);
                draw_UV(pUVs[1]);
                draw_Vertex(pCorners[1].X, pCorners[1].Y, 0.0f);
                draw_Color(pColors[2]);
                draw_UV(pUVs[2]);
                draw_Vertex(pCorners[2].X, pCorners[2].Y, 0.0f);
                draw_Color(pColors[3]);
                draw_UV(pUVs[3]);
                draw_Vertex(pCorners[3].X, pCorners[3].Y, 0.0f);
                */
    }

}
