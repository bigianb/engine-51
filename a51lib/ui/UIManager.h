#pragma once

#include <string>
#include <vector>
#include <map>

#include "../VectorMath.h"
#include "UIDialog.h"
#include "../resourceManager/ResourceManager.h"
#include "../Bitmap.h"

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
        Window*     captureWindow;
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
        Manager()
            : width(1)
            , height(1)
            , scaleX(1.0)
            , scaleY(1.0)
        {
        }
        ~Manager();
        void init(Renderer& renderer, ResourceManager*);
        void setRes();

        void update(float deltaTime);
        void render(Renderer& renderer);

        User*   createUser(int controllerID, const IntRect& bounds);
        void    enableUser(User* user, bool state);
        Dialog* openDialog(std::string className, IntRect position, Window* parent, int flags);

        Window* createWin(User* user, const char* ClassName, const IntRect& position, Window* parent, int flags);

        int loadElement(ResourceManager* rm, const char* name, const char* pathName, int nStates, int cx, int cy);

        bool registerWinClass(const char* className, WindowFactoryFn factoryFunction);
        bool registerDialogClass(const char* className, DialogTemplate* dialogTemplate, DialogFactoryFn factoryFunction);

        float getScaleX() const { return scaleX; }
        float getScaleY() const { return scaleY; }

        int findElement(const char* name) const;

        User* getUserId() const { return userId; }

        std::wstring lookupString(std::string tablename, int id);
        std::wstring lookupString(std::string tablename, const char* id);

        void loadFont(std::string name, std::string filename);

    private:
        // Window pixel size, set in init.
        int width;
        int height;

        float scaleX;
        float scaleY;

        ResourceManager* resourceManager;

        std::vector<User*> users;

        std::map<std::string, DialogClass*> dialogClasses;
        std::map<std::string, WindowClass*> windowClasses;

        std::vector<Element*> elements;
        std::map<std::string, Font*> fontMap;

        User* userId;
    };
}
