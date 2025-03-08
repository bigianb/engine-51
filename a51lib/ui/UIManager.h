#pragma once

#include <string>
#include <vector>
#include <map>

#include "../VectorMath.h"
#include "UIDialog.h"
#include "../resourceManager/ResourceManager.h"
#include "../Bitmap.h"

class Engine;
class Renderer;
class ResourceManager;

namespace ui
{

    static const int INPUT_MAX_CONTROLLER_COUNT = 2;

    class Window;
    class Dialog;
    class Font;

    class ButtonInputData
    {
    public:
        bool  state;
        float analogScaler;
        float analogEngage;
        float analogDisengage;
        float repeatDelay;
        float repeatInterval;
        float repeatTimer;
        int   nPresses;
        int   nRepeats;
        int   nReleases;

    public:
        ButtonInputData(void)
        {
            state = 0;
            nPresses = 0;
            nRepeats = 0;
            nReleases = 0;
            repeatDelay = 0.200f;
            repeatInterval = 0.060f;
            analogScaler = 1.0f;
            analogEngage = 0.5f;
            analogDisengage = 0.3f;
        };

        ~ButtonInputData(void) {};

        void clear(void)
        {
            state = 0;
            nPresses = 0;
            nRepeats = 0;
            nReleases = 0;
        };

        void setupRepeat(float Delay, float Interval)
        {
            repeatDelay = Delay;
            repeatInterval = Interval;
        };
        void setupAnalog(float s, float e, float d)
        {
            analogScaler = s;
            analogEngage = e;
            analogDisengage = d;
        };
    };

    class User
    {
    public:
        bool        enabled;
        int         controllerID;
        IntRect     bounds;
        int         data;
        int         height;
        Window*     lastWindowUnderCursor;
        std::string background;
        int         highlightElement;

        bool            cursorVisible;
        bool            mouseActive;
        int             cursorX;
        int             cursorY;
        int             lastCursorX;
        int             lastCursorY;
        ButtonInputData buttonLB;
        ButtonInputData buttonMB;
        ButtonInputData buttonRB;

        ButtonInputData dpadUp[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData dpadDown[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData dpadLeft[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData dpadRight[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData padSelect[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData padBack[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData padDelete[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData padHelp[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData padActivate[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData padShoulderL[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData padShoulderR[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData padShoulderL2[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData padShoulderR2[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData lStickUp[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData lStickDown[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData lStickLeft[INPUT_MAX_CONTROLLER_COUNT];
        ButtonInputData lStickRight[INPUT_MAX_CONTROLLER_COUNT];

        std::vector<Dialog*> dialogStack;
    };

    struct Element
    {
        Element(ResourceManager* rm)
            : bitmap(rm)
        {
        }

        std::string            name;
        ResourceHandle<Bitmap> bitmap;
        int                    nStates;
        int                    cx;
        int                    cy;
        std::vector<IntRect>   r;
    };

    struct ControlTemplate
    {
        int         controlID;
        const char* stringID;
        const char* clazz;
        int         x, y, w, h;
        int         nx, ny, nw, nh;
        int         flags;
    };

    struct DialogTemplate
    {
        const char*      stringID;
        int              navW, navH;
        int              nControls;
        ControlTemplate* controls;
        int              focusControl;
    };

    class Manager;
    typedef Dialog* (*DialogFactoryFn)(User* user, Manager* manager, DialogTemplate* dialogTemplate, const IntRect& position, Window* parent, int flags);

    struct DialogClass
    {
        std::string     className;
        DialogFactoryFn factoryFn;
        DialogTemplate* dialogTemplate;
    };

    typedef Window* (*WindowFactoryFn)(User* user, Manager* manager, const IntRect& position, Window* parent, int flags);

    struct WindowClass
    {
        std::string     className;
        WindowFactoryFn factoryFn;
    };

    class Manager
    {
    public:
        enum ControlState
        {
            CS_NORMAL = 0,
            CS_HIGHLIGHT,
            CS_SELECTED,
            CS_HIGHLIGHT_SELECTED,
            CS_DISABLED
        };

        Manager()
            : width(1)
            , height(1)
            , scaleX(1.0)
            , scaleY(1.0)
            , enableUserInput(false)
            , uiLastSelectController(0)
            , ativeController(0)
            , endDialogCount(0)
        {
        }
        ~Manager();
        void init(Renderer& renderer, ResourceManager*);
        void setRes();

        void update(float deltaTime);
        bool processInput(Engine*, float deltaTime);
        bool processInput(Engine*, float deltaTime, User* user);
        void render(Renderer& renderer);

        User*   createUser(int controllerID, const IntRect& bounds);
        void    enableUser(User* user, bool state);
        Dialog* openDialog(std::string className, IntRect position, Window* parent, int flags);
        void    endDialog(bool resetCursor = false);
        Window* createWin(User* user, const char* ClassName, const IntRect& position, Window* parent, int flags);

        int loadElement(ResourceManager* rm, const char* name, const char* pathName, int nStates, int cx, int cy);

        Bitmap* loadBitmap(const char* name, const char* pathName);
        void    unloadBitmap(const char* name);

        Bitmap* loadBackground(const char* name, const char* pathName);
        void    unloadBackground(const char* name);
        void    setUserBackground(const char* name);

        bool registerWinClass(const char* className, WindowFactoryFn factoryFunction);
        bool registerDialogClass(const char* className, DialogTemplate* dialogTemplate, DialogFactoryFn factoryFunction);

        float getScaleX() const { return scaleX; }
        float getScaleY() const { return scaleY; }

        int findElement(const char* name) const;

        User* getUserId() const { return userId; }

        std::wstring lookupString(std::string tablename, int id);
        std::wstring lookupString(std::string tablename, const char* id);

        void loadFont(std::string name, std::string filename);

        Window* getWindowAtXY(User* user, int x, int y);

        bool isScreenScaling(void) { return m_isScaling; }
        void setScreenScaling(bool val) { m_isScaling = val; }
        void getScreenSize(IntRect& size) { size = m_CurrScreenSize; }
        void setScreenSize(const IntRect& size);
        bool isScreenOn(void) { return (m_ScreenIsOn); }
        void setScreenOn(bool state) { m_ScreenIsOn = state; }

        // highlight
        void initScreenHighlight();
        void setScreenHighlight(const IntRect& pos);
        void renderScreenHighlight(Renderer& renderer);

        // Glow bar
        void initGlowBar();
        void renderGlowBar(Renderer&);
        void updateGlowBar(float deltaTime);
        void getGlowBarPos(IntRect& pos) { pos = m_GlowPos; }

        // render helpers
        void renderText(Renderer& renderer, std::string fontName, const IntRect& pos, int flags, Colour textColor, std::wstring text, bool ignoreEmbeddedColor = true, bool useGradient = true, float flareAmount = 0.0) const;
        void renderBackground(Renderer& renderer, std::string name);
        void renderElement(Renderer& renderer, int iElement, const IntRect& position, int state, const Colour& color = COLOR_WHITE, bool isAdditive = false) const;

    private:
        void updateButton(ui::ButtonInputData& button, bool state, float deltaTime);

        bool enableUserInput;

        bool enableBackground;

        IntRect m_CurrScreenSize;
        bool    m_ScreenIsOn;

        // highlight
        int     m_ScreenHighlightID;
        int     m_ScreenGlowID;
        IntRect m_ScreenHighlightPos;
        bool    m_ScreenHighlightEnabled;
        int     m_HighlightAlpha;
        bool    m_HighlightFadeUp;
        bool    m_CycleFadeUp;

        int     m_GlowID;
        int     m_GlowStartX;
        int     m_GlowEndX;
        float   m_GlowSpeed;
        IntRect m_GlowPos;
        IntRect m_GlowTrail[8];
        bool    m_GlowOnTop;

        // maybe better in the engine object
        int uiLastSelectController;

        int ativeController;

        int endDialogCount;

        // Window pixel size, set in init.
        int width;
        int height;

        bool m_isScaling;

        float scaleX;
        float scaleY;

        ResourceManager* resourceManager;

        std::vector<User*> users;

        std::map<std::string, DialogClass*> dialogClasses;
        std::map<std::string, WindowClass*> windowClasses;

        std::vector<Element*>        elements;
        std::map<std::string, Font*> fontMap;

        std::map<std::string, ResourceHandle<Bitmap>> bitmaps;
        std::map<std::string, ResourceHandle<Bitmap>> backgrounds;

        User* userId;
    };
}
