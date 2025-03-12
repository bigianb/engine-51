#include "UIManager.h"

#include <iostream>
#include <algorithm>

#include "../system/Engine.h"
#include "../system/Renderer.h"
#include "../VectorMath.h"
#include "dialogs/esrbDialog.h"
#include "dialogs/mainMenuDialog.h"
#include "dialogs/pressStartDialog.h"
#include "dialogs/startGameDialog.h"
#include "dialogs/profileSelectDialog.h"
#include "UIButton.h"
#include "UIText.h"
#include "UIFont.h"

ui::Manager::~Manager()
{
    // TODO: delete dialog class map entries.
}

void ui::Manager::init(Renderer& renderer, ResourceManager* rm)
{
    resourceManager = rm;

    renderer.getRes(width, height);
    IntRect r(0, 0, width, height);
    setScreenSize(r);
    userId = createUser(-1, r);
    enableUser(userId, false);

    registerWinClass("text", &ui::Text::factory);
    registerWinClass("bitmap", &ui::BitmapControl::factory);
    registerWinClass("button", &ui::Button::factory);

    EsrbDialog::registerDialog(this);
    PressStartDialog::registerDialog(this);
    StartGameDialog::registerDialog(this);
    MainMenuDialog::registerDialog(this);
    ProfileSelectDialog::registerDialog(this);

    loadElement(rm, "frame", "UI_frame1.XBMP", 2, 3, 3);
    loadElement(rm, "frame2", "UI_frame2.XBMP", 1, 3, 3);
    loadElement(rm, "glow", "UI_barglow.XBMP", 1, 1, 1);
    loadElement(rm, "highlight", "UI_highlight.XBMP", 1, 3, 3);
    loadElement(rm, "screenglow", "UI_screenglow.XBMP", 1, 3, 3);

    loadElement(rm, "button", "UI_uibutton.XBMP", 5, 3, 1);

    loadElement(rm, "button_check", "UI_checkbox.XBMP", 5, 1, 1);

    loadElement(rm, "sb_frame", "UI_frame2.XBMP", 1, 3, 3);
    loadElement(rm, "sb_arrowdown", "UI_downarrow.XBMP", 5, 1, 1);
    loadElement(rm, "sb_arrowup", "UI_uparrow.XBMP", 5, 1, 1);
    loadElement(rm, "sb_container", "UI_container.XBMP", 5, 3, 3);
    loadElement(rm, "sb_thumb", "UI_thumb.XBMP", 5, 1, 3);

    loadElement(rm, "slider_bar", "UI_slidercontainer.XBMP", 1, 3, 1);
    loadElement(rm, "slider_thumb", "UI_sliderthumb.XBMP", 5, 1, 1);

    loadElement(rm, "button_combo1", "UI_combobox.XBMP", 5, 3, 1);
    loadElement(rm, "button_combo2", "UI_combobox_128.XBMP", 1, 3, 1);

    loadElement(rm, "button_edit", "UI_editbox.XBMP", 5, 3, 1);

    rm->loadStringTable("ui", "ENG_ui_strings.stringbin");
    rm->loadStringTable("scan", "ENG_character_scan_strings.stringbin");
    rm->loadStringTable("lore_ingame", "ENG_ingame_lore_strings.stringbin");

    loadFont("large", "UI_A51FontLarge");
    loadFont("small", "UI_A51FontLegal");
    loadFont("hudnum", "UI_A51FontHUD");
    loadFont("loadscr", "UI_A51FontLoadscr");

    Button::setTextColorNormal(Colour(126, 220, 60, 255));
    Button::setTextColorHightlight(Colour(255, 252, 204, 255));
    Button::setTextColorDisabled(COLOR_GREY);
    Button::setTextColorShadow(Colour(0, 0, 0, 0));

    //Dialog::setTextColorNormal       (Colour(255,252,204,255));
    //Dialog::setTextColorShadow       (COLOR_BLACK);

    enableUserInput = true;
    enableBackground = false;
    initScreenHighlight();
    initGlowBar();
}

void ui::Manager::loadFont(std::string name, std::string filename)
{
    if (fontMap.contains(name)) {
        return;
    }

    ResourceHandle<Font> fontResource(resourceManager);
    fontResource.setName(filename + ".FONT");
    Font* font = fontResource.getPointer();

    ResourceHandle<Bitmap> fontBitmapResource(resourceManager);
    fontBitmapResource.setName(filename + ".XBMP");
    Bitmap* fontBitmap = fontBitmapResource.getPointer();

    font->bitmap = fontBitmap;

    fontMap[name] = font;
}

int ui::Manager::loadElement(ResourceManager* rm, const char* name, const char* pathName, int nStates, int cx, int cy)
{
    std::vector<int> x;
    std::vector<int> y;

    int id = findElement(name);
    if (id != -1) {
        return id;
    }

    Element* element = new Element(rm);

    element->name = name;
    element->nStates = nStates;
    element->cx = cx;
    element->cy = cy;

    element->bitmap.setName(pathName);
    Bitmap* bitmap = element->bitmap.getPointer();

    if ((nStates > 0) && (cx > 0) && (cy > 0)) {
        Colour RegColor = bitmap->getPixelColor(0, 0);

        // Find the registration markers
        x.reserve(cx + 1);
        y.reserve((cy * nStates) + 1);
        for (int i = 0; i < bitmap->getWidth(); i++) {
            if (bitmap->getPixelColor(i, 0) == RegColor) {
                x.push_back(i);
            }
        }
        for (int i = 0; i < bitmap->getHeight(); i++) {
            if (bitmap->getPixelColor(0, i) == RegColor) {
                y.push_back(i);
            }
        }
        // Setup the rectangles
        element->r.reserve(cx * cy * nStates);
        for (int iy = 0; iy < (cy * nStates); iy++) {
            for (int ix = 0; ix < cx; ix++) {
                element->r.push_back(IntRect(x[ix] + 1, y[iy] + 1, (x[ix + 1]), (y[iy + 1])));
            }
        }
    }

    elements.push_back(element);
    return elements.size() - 1;
}

Bitmap* ui::Manager::loadBitmap(const char* name, const char* pathName)
{
    if (!bitmaps.contains(name)) {
        bitmaps[name] = ResourceHandle<Bitmap>(resourceManager);
        ResourceHandle<Bitmap>& bitmapResource = bitmaps[name];
        bitmapResource.setName(pathName);
    }
    return bitmaps[name].getPointer();
}

void ui::Manager::unloadBitmap(const char* name)
{
    if (bitmaps.contains(name)) {
        ResourceHandle<Bitmap>& bitmapResource = bitmaps[name];
        bitmapResource.destroy();
        bitmaps.erase(name);
    }
}

Bitmap* ui::Manager::loadBackground(const char* name, const char* pathName)
{
    if (!backgrounds.contains(name)) {
        backgrounds[name] = ResourceHandle<Bitmap>(resourceManager);
        ResourceHandle<Bitmap>& bitmapResource = backgrounds[name];
        bitmapResource.setName(pathName);
    }
    return backgrounds[name].getPointer();
}

void ui::Manager::unloadBackground(const char* name)
{
    if (backgrounds.contains(name)) {
        ResourceHandle<Bitmap>& bitmapResource = backgrounds[name];
        bitmapResource.destroy();
        backgrounds.erase(name);
    }
}

void ui::Manager::setUserBackground(const char* name)
{
    enableBackground = true;
    userId->background = name;
}

ui::User* ui::Manager::createUser(int controllerID, const IntRect& bounds)
{
    User* pUser = new User;
    if (pUser) {
        int i;

        // Fill out the struct
        pUser->enabled = true;
        pUser->controllerID = controllerID;
        pUser->bounds = bounds;
        pUser->data = 0;
        pUser->height = 0;
        pUser->lastWindowUnderCursor = nullptr;
        //pUser->highlightElement = FindElement("highlight");
        //ASSERT(pUser->iHighlightElement);

        pUser->cursorVisible = true;
        pUser->mouseActive = false;
        pUser->cursorX = bounds.getWidth() / 2 + bounds.left;
        pUser->cursorY = bounds.getHeight() / 2 + bounds.top;
        pUser->lastCursorX = bounds.getWidth() / 2 + bounds.left;
        pUser->lastCursorY = bounds.getHeight() / 2 + bounds.top;

        // Set Analog Scalers
        for (i = 0; i < INPUT_MAX_CONTROLLER_COUNT; i++) {
            pUser->dpadUp[i].setupRepeat(0.200f, 0.060f);
            pUser->dpadDown[i].setupRepeat(0.200f, 0.060f);
            pUser->dpadLeft[i].setupRepeat(0.200f, 0.060f);
            pUser->dpadRight[i].setupRepeat(0.200f, 0.060f);
            pUser->padSelect[i].setupRepeat(0.200f, 0.060f);
            pUser->padBack[i].setupRepeat(0.200f, 0.060f);
            pUser->padHelp[i].setupRepeat(0.200f, 0.060f);
            pUser->padActivate[i].setupRepeat(0.200f, 0.060f);
            pUser->padShoulderL[i].setupRepeat(0.200f, 0.060f);
            pUser->padShoulderR[i].setupRepeat(0.200f, 0.060f);
            pUser->padShoulderL2[i].setupRepeat(0.200f, 0.060f);
            pUser->padShoulderR2[i].setupRepeat(0.200f, 0.060f);
            pUser->lStickUp[i].setupRepeat(40.600f, 10.600f);
            pUser->lStickDown[i].setupRepeat(40.600f, 10.600f);
            pUser->lStickLeft[i].setupRepeat(40.600f, 10.600f);
            pUser->lStickRight[i].setupRepeat(40.600f, 10.600f);
            pUser->lStickUp[i].setupAnalog(1.0f, 0.15f, 0.2f);
            pUser->lStickDown[i].setupAnalog(-1.0f, 0.15f, 0.2f);
            pUser->lStickLeft[i].setupAnalog(-1.0f, 0.15f, 0.2f);
            pUser->lStickRight[i].setupAnalog(1.0f, 0.15f, 0.2f);
        }
        static const int MAX_DIALOGS_EVER = 20;
        pUser->dialogStack.reserve(MAX_DIALOGS_EVER);

        // Add an entry to the users list
        users.push_back(pUser);
    }

    return pUser;
}

bool ui::Manager::registerWinClass(const char* className, WindowFactoryFn factoryFunction)
{
    bool success = false;
    if (!windowClasses.contains(className)) {
        WindowClass* clazz = new WindowClass();
        clazz->className = className;
        clazz->factoryFn = factoryFunction;
        windowClasses[className] = clazz;
        success = true;
    }
    return success;
}

bool ui::Manager::registerDialogClass(const char* className, ui::DialogTemplate* dialogTemplate, ui::DialogFactoryFn factoryFunction)
{
    bool success = false;
    if (!dialogClasses.contains(className)) {
        DialogClass* clazz = new DialogClass();
        clazz->className = className;
        clazz->dialogTemplate = dialogTemplate;
        clazz->factoryFn = factoryFunction;
        dialogClasses[className] = clazz;
        success = true;
    }

    return success;
}

void ui::Manager::enableUser(ui::User* user, bool state)
{
    user->enabled = state;
}

ui::Dialog* ui::Manager::openDialog(std::string className, IntRect position, ui::Window* parent, int flags)
{
    if (!dialogClasses.contains(className)) {
        std::cerr << "*** ERROR: No registration for dialog class " << className << std::endl;
        return nullptr;
    }
    const DialogClass* dlgClass = dialogClasses[className];

    if (flags & Window::WF_DLG_CENTER) {
        /* TODO: Implement
        IntRect b = GetUserBounds( UserID );

        if (b.GetWidth() > position.GetWidth())
        {
            b.Translate( -b.l, -b.t );
            position.Translate( b.l + (b.GetWidth ()-position.GetWidth ())/2 - position.l,
                            b.t + (b.GetHeight()-position.GetHeight())/2 - position.t );
        }
        */
    }
    if (!(flags & Window::WF_USE_ABSOLUTE)) {
        int dlgWidth = (int)(position.getWidth() * scaleX);
        position.left = (width - dlgWidth) / 2;
        position.right = position.left + dlgWidth;

        int dlgHeight = (int)(position.getHeight() * scaleY);
        position.top = (int)(position.top * scaleY);
        position.bottom = position.top + dlgHeight;
    }
    IntRect createPosition = position;

    Dialog* pDialog = dlgClass->factoryFn(userId, this, dlgClass->dialogTemplate, position, parent, flags);

    pDialog->setCreatePosition(createPosition);

    // If this is not a TAB dialog page
    if (!(pDialog->getFlags() & Window::WF_TAB)) {
        // Add to the Dialog Stack
        if (parent == nullptr) {
            userId->dialogStack.push_back(pDialog);
        }

        if (!(flags & Window::WF_NO_ACTIVATE) && (pDialog->children.size() > 0)) {
            pDialog->gotoControl(0);
        }
    }
    return pDialog;
}

void ui::Manager::endDialog(bool resetCursor)
{
    while (!userId->dialogStack.empty()) {
        Dialog* dlg = userId->dialogStack.back();
        if (resetCursor) {
            userId->cursorX = dlg->oldCursorX;
            userId->cursorY = dlg->oldCursorY;
        }

        // Clear LastWindow under cursor if it was part of this dialog
        if (userId->lastWindowUnderCursor) {
            if ((userId->lastWindowUnderCursor == dlg) ||
                (userId->lastWindowUnderCursor->isChildOf(dlg))) {
                userId->lastWindowUnderCursor = nullptr;
            }
        }

        delete dlg;
        userId->dialogStack.pop_back();
    }
}

ui::Window* ui::Manager::createWin(ui::User* user, const char* className, const IntRect& position, ui::Window* parent, int flags)
{
    if (!windowClasses.contains(className)) {
        std::cerr << "*** ERROR: No registration for window class " << className << std::endl;
        return nullptr;
    }
    const WindowClass* windowClass = windowClasses[className];
    return windowClass->factoryFn(user, this, position, parent, flags);
}

int ui::Manager::findElement(const char* elementName) const
{
    for (int i = 0; i < elements.size(); ++i) {
        if (elements[i]->name == elementName) {
            return i;
        }
    }
    return -1;
}

void ui::Manager::setRes()
{
    scaleX = 1.0;
    scaleY = 1.0;
}

bool ui::Manager::processInput(Engine* engine, float deltaTime)
{
    bool result = true;
    for (User* user : users) {
        if (user->enabled) {
            result &= processInput(engine, deltaTime, user);
        }
    }
    return result;
}

void ui::Manager::updateButton(ui::ButtonInputData& button, bool state, float deltaTime)
{
    // Clear number of presses, repeats and releases
    button.nPresses = 0;
    button.nRepeats = 0;
    button.nReleases = 0;

    if (enableUserInput) {
        // Check for press
        if (!button.state && state) {
            button.nPresses++;
            button.repeatTimer = button.repeatDelay;
        }

        // Check for repeat
        if (button.state && state) {
            // If repeat interval is 0 then repeat is disabled
            if (button.repeatInterval > 0.0f) {
                button.repeatTimer -= deltaTime;
                while (button.repeatTimer < 0.0f) {
                    button.nRepeats++;
                    button.repeatTimer += button.repeatInterval;
                }
            }
        }

        // Check for release
        if (button.state && !state) {
            button.nReleases++;
        }
    } else {
        state = 0;
    }

    // Set new state
    button.state = state;
}

ui::Window* ui::Manager::getWindowAtXY(User* user, int x, int y)
{
    Window* window = nullptr;

    // Check if anything on dialog stack
    if (user->dialogStack.size() > 0) {
        int i = user->dialogStack.size() - 1;

        // Yes search from topmost dialog back
        while ((window == nullptr) && (i >= 0)) {
            Dialog* dialog = user->dialogStack[i];
            dialog->screenToLocal(x, y);
            window = dialog->getWindowAtXY(x, y);

            // Don't select a disabled window
            if (window && (window->getFlags() & ui::Window::WF_DISABLED)) {
                window = dialog;
            }

            // If modal then exit, otherwise step back to next dialog
            if (dialog->getFlags() & ui::Window::WF_INPUTMODAL) {
                if (window == nullptr) {
                    window = dialog;
                }
                break;
            } else {
                i--;
            }
        }
    }
    return window;
}

bool ui::Manager::processInput(Engine* engine, float deltaTime, User* user)
{
    bool iterate = false;
    int  iterateCount = 0;
    int  startController = 0;
    int  endController = 0;

    do {
        if (iterate) {
            iterateCount++;
            iterate = false;
        }

        user->cursorX += engine->input_GetValue(InputGadget::INPUT_MOUSE_X_REL);
        user->cursorY += engine->input_GetValue(InputGadget::INPUT_MOUSE_Y_REL);

        user->cursorX = std::max(user->cursorX, 0);
        user->cursorX = std::min(user->cursorX, (user->bounds.getWidth() - 1));
        user->cursorY = std::max(user->cursorY, 0);
        user->cursorY = std::min(user->cursorY, (user->bounds.getHeight() - 1));

        Window* windowUnderCursor = getWindowAtXY(user, user->cursorX, user->cursorY);

        // Has window under cursor changed?
        if (windowUnderCursor != user->lastWindowUnderCursor) {
            // Call exit function if there was a window under the cursor
            if (user->lastWindowUnderCursor) {
                user->lastWindowUnderCursor->onCursorExit();
            }

            // Set new window under cursor and call enter function
            user->lastWindowUnderCursor = windowUnderCursor;
            if (user->lastWindowUnderCursor) {
                user->lastWindowUnderCursor->onCursorEnter();
            }
        }

        for (int i = startController; i <= endController; i++) {

            updateButton(user->dpadUp[i], engine->input_IsPressed(InputGadget::INPUT_PS2_BTN_L_UP, i), deltaTime);
            updateButton(user->dpadDown[i], engine->input_IsPressed(InputGadget::INPUT_PS2_BTN_L_DOWN, i), deltaTime);
            updateButton(user->dpadLeft[i], engine->input_IsPressed(InputGadget::INPUT_PS2_BTN_L_LEFT, i), deltaTime);
            updateButton(user->dpadRight[i], engine->input_IsPressed(InputGadget::INPUT_PS2_BTN_L_RIGHT, i), deltaTime);

            updateButton(user->padSelect[i], engine->input_WasPressed(InputGadget::INPUT_PS2_BTN_CROSS, i), deltaTime);
            updateButton(user->padBack[i], engine->input_WasPressed(InputGadget::INPUT_PS2_BTN_TRIANGLE, i), deltaTime);
            updateButton(user->padDelete[i], engine->input_WasPressed(InputGadget::INPUT_PS2_BTN_SQUARE, i), deltaTime);
            updateButton(user->padActivate[i], engine->input_WasPressed(InputGadget::INPUT_PS2_BTN_CIRCLE, i), deltaTime);
            updateButton(user->padShoulderL[i], engine->input_WasPressed(InputGadget::INPUT_PS2_BTN_L1, i), deltaTime);
            updateButton(user->padShoulderR[i], engine->input_WasPressed(InputGadget::INPUT_PS2_BTN_R1, i), deltaTime);
            updateButton(user->padShoulderL2[i], engine->input_WasPressed(InputGadget::INPUT_PS2_BTN_L2, i), deltaTime);
            updateButton(user->padShoulderR2[i], engine->input_WasPressed(InputGadget::INPUT_PS2_BTN_R2, i), deltaTime);
            updateButton(user->padHelp[i], engine->input_WasPressed(InputGadget::INPUT_PS2_BTN_START, i), deltaTime);

            updateButton(user->lStickUp[i], engine->input_GetValue(InputGadget::INPUT_PS2_STICK_LEFT_Y, i), deltaTime);
            updateButton(user->lStickDown[i], engine->input_GetValue(InputGadget::INPUT_PS2_STICK_LEFT_Y, i), deltaTime);
            updateButton(user->lStickLeft[i], engine->input_GetValue(InputGadget::INPUT_PS2_STICK_LEFT_X, i), deltaTime);
            updateButton(user->lStickRight[i], engine->input_GetValue(InputGadget::INPUT_PS2_STICK_LEFT_X, i), deltaTime);

            // Keep index of last controller that pressed a select button so we can hack
            // the controller number into the players controller for 1 player games
            if (user->padSelect[i].nPresses > 0) {
                uiLastSelectController = i;
            }
        }

        // Update mouse buttons
        updateButton(user->buttonLB, engine->input_IsPressed(InputGadget::INPUT_MOUSE_BTN_L), deltaTime);
        updateButton(user->buttonMB, engine->input_IsPressed(InputGadget::INPUT_MOUSE_BTN_C), deltaTime);
        updateButton(user->buttonRB, engine->input_IsPressed(InputGadget::INPUT_MOUSE_BTN_R), deltaTime);

        // Only do this if there is a target window
        if (windowUnderCursor) {
            // Issue window calls for mouse
            if ((user->lastCursorX != user->cursorX) || (user->lastCursorY != user->cursorY)) {
                windowUnderCursor->onCursorMove(user->cursorX, user->cursorY);
            }
            if (user->buttonLB.nPresses) {
                windowUnderCursor->onLBDown(windowUnderCursor);
            }
            if (user->buttonLB.nReleases) {
                windowUnderCursor->onLBUp();
            }
            if (user->buttonMB.nPresses) {
                windowUnderCursor->onMBDown();
            }
            if (user->buttonMB.nReleases) {
                windowUnderCursor->onMBUp();
            }
            if (user->buttonRB.nPresses) {
                windowUnderCursor->onRBDown();
            }
            if (user->buttonRB.nReleases) {
                windowUnderCursor->onRBUp();
            }

            // Sum up button presses
            int pdpadUp = 0;
            int pdpadDown = 0;
            int pdpadLeft = 0;
            int pdpadRight = 0;
            int rdpadUp = 0;
            int rdpadDown = 0;
            int rdpadLeft = 0;
            int rdpadRight = 0;
            int tdpadUp = 0;
            int tdpadDown = 0;
            int tdpadLeft = 0;
            int tdpadRight = 0;
            int PadSelect = 0;
            int PadBack = 0;
            int PadDelete = 0;
            int PadActivate = 0;
            int PadShoulderL = 0;
            int PadShoulderR = 0;
            int PadShoulderL2 = 0;
            int PadShoulderR2 = 0;
            int PadHelp = 0;
            {
                for (int i = startController; i <= endController; i++) {
                    // set active controller
                    ativeController = i;

                    // check input for each controller
                    pdpadUp = user->dpadUp[i].nPresses;
                    pdpadDown = user->dpadDown[i].nPresses;
                    pdpadLeft = user->dpadLeft[i].nPresses;
                    pdpadRight = user->dpadRight[i].nPresses;
                    rdpadUp = user->dpadUp[i].nRepeats;
                    rdpadDown = user->dpadDown[i].nRepeats;
                    rdpadLeft = user->dpadLeft[i].nRepeats;
                    rdpadRight = user->dpadRight[i].nRepeats;
                    tdpadUp = user->dpadUp[i].nPresses + user->dpadUp[i].nRepeats;
                    tdpadDown = user->dpadDown[i].nPresses + user->dpadDown[i].nRepeats;
                    tdpadLeft = user->dpadLeft[i].nPresses + user->dpadLeft[i].nRepeats;
                    tdpadRight = user->dpadRight[i].nPresses + user->dpadRight[i].nRepeats;
                    PadSelect = user->padSelect[i].nPresses;
                    PadBack = user->padBack[i].nPresses;
                    PadDelete = user->padDelete[i].nPresses;
                    PadActivate = user->padActivate[i].nPresses;
                    PadShoulderL = user->padShoulderL[i].nPresses + user->padShoulderL[i].nRepeats;
                    PadShoulderR = user->padShoulderR[i].nPresses + user->padShoulderR[i].nRepeats;
                    PadShoulderL2 = user->padShoulderL2[i].nPresses + user->padShoulderL2[i].nRepeats;
                    PadShoulderR2 = user->padShoulderR2[i].nPresses + user->padShoulderR2[i].nRepeats;

                    PadHelp = user->padHelp[i].nPresses;

                    pdpadUp += user->lStickUp[i].nPresses;
                    pdpadDown += user->lStickDown[i].nPresses;
                    pdpadLeft += user->lStickLeft[i].nPresses;
                    pdpadRight += user->lStickRight[i].nPresses;
                    rdpadUp += user->lStickUp[i].nRepeats;
                    rdpadDown += user->lStickDown[i].nRepeats;
                    rdpadLeft += user->lStickLeft[i].nRepeats;
                    rdpadRight += user->lStickRight[i].nRepeats;
                    tdpadUp += user->lStickUp[i].nPresses + user->lStickUp[i].nRepeats;
                    tdpadDown += user->lStickDown[i].nPresses + user->lStickDown[i].nRepeats;
                    tdpadLeft += user->lStickLeft[i].nPresses + user->lStickLeft[i].nRepeats;
                    tdpadRight += user->lStickRight[i].nPresses + user->lStickRight[i].nRepeats;

                    // send commands for each controller
                    endDialogCount = 0;
                    // Issue window calls for pad navigation
                    if (tdpadUp) {
                        iterate = true;
                        windowUnderCursor->onPadNavigate(Window::NavigateDir::NAV_UP, pdpadUp, rdpadUp, false, true);
                    }

                    if (tdpadDown) {
                        iterate = true;
                        windowUnderCursor->onPadNavigate(Window::NavigateDir::NAV_DOWN, pdpadDown, rdpadDown, false, true);
                    }

                    if (tdpadLeft) {
                        iterate = true;
                        windowUnderCursor->onPadNavigate(Window::NavigateDir::NAV_LEFT, pdpadLeft, rdpadLeft);
                    }

                    if (tdpadRight) {
                        iterate = true;
                        windowUnderCursor->onPadNavigate(Window::NavigateDir::NAV_RIGHT, pdpadRight, rdpadRight);
                    }

                    // Issue window calls for pad select / back / help
                    if (!iterate && PadSelect && !endDialogCount) {
                        iterate = true;
                        windowUnderCursor->onPadSelect(windowUnderCursor);
                    }

                    if (!iterate && PadBack && !endDialogCount) {
                        iterate = true;
                        windowUnderCursor->onPadBack();
                    }

                    if (!iterate && PadDelete && !endDialogCount) {
                        iterate = true;
                        windowUnderCursor->onPadDelete();
                    }

                    if (!iterate && PadActivate && !endDialogCount) {
                        iterate = true;
                        windowUnderCursor->onPadActivate();
                    }

                    if (!iterate && PadHelp && !endDialogCount) {
                        iterate = true;
                        windowUnderCursor->onPadHelp();
                    }

                    // Issue window calls for pad shoulders
                    if (PadShoulderL && !endDialogCount) {
                        windowUnderCursor->onPadShoulder(-1);
                    } else if (PadShoulderR && !endDialogCount) {
                        windowUnderCursor->onPadShoulder(1);
                    };

                    if (PadShoulderL2 && !endDialogCount) {
                        windowUnderCursor->onPadShoulder2(-1);
                    } else if (PadShoulderR2 && !endDialogCount) {
                        windowUnderCursor->onPadShoulder2(1);
                    }

                    endDialogCount = 0;
                }
            }
        }

        // Save Last Cursor Position for next time around
        user->lastCursorX = user->cursorX;
        user->lastCursorY = user->cursorY;

        // Clear deltaTime in case of next iteration
        deltaTime = 0.0f;

    } while (iterate && !iterateCount);

    // Do Global inputs

    if (engine->input_IsPressed(InputGadget::INPUT_MSG_EXIT)) {
        return false;
    }

    // Return TRUE if not exiting
    return true;
}

void ui::Manager::update(float deltaTime)
{
    /*
    if( highlightFadeUp )
    {
        if( ++highlightAlpha == 32)
        {
            highlightFadeUp = false;
        }
    }
    else
    {
        if( --highlightAlpha == 0 )
        {
            highlightFadeUp = true;
        }
    }
    */
    //UpdateScreenWipe(deltaTime);
    //UpdateRefreshBar(deltaTime);

    for (User* user : users) {
        if (user->enabled) {
            {
                // Update all Dialogs on Stack
                for (Dialog* dlg : user->dialogStack) {
                    dlg->onUpdate(deltaTime);
                }
            }
        }
    }
}

void ui::Manager::render(Renderer& renderer)
{
    for (User* user : users) {
        if (user->enabled) {
            renderBackground(renderer, user->background);
            int dlgNo = user->dialogStack.size() - 1;
            // Find top modal dialog
            while (dlgNo > 0 && !user->dialogStack[dlgNo]->isRenderModel()) {
                --dlgNo;
            }
            if (dlgNo < 0) {
                dlgNo = 0;
            }
            while (dlgNo < user->dialogStack.size()) {
                user->dialogStack[dlgNo++]->render(renderer, user->bounds.left, user->bounds.top);
            }
        }
    }
}

std::wstring ui::Manager::lookupString(std::string tablename, int id)
{
    return resourceManager->lookupString(tablename, id);
}

std::wstring ui::Manager::lookupString(std::string tablename, const char* id)
{
    return resourceManager->lookupString(tablename, id);
}

void ui::Manager::renderText(Renderer& renderer, std::string fontName, const IntRect& pos, int flags, Colour textColor, std::wstring text, bool ignoreEmbeddedColor, bool useGradient, float flareAmount) const
{
    if (!fontMap.contains(fontName)) {
        return;
    }
    const Font* font = fontMap.at(fontName);
    font->renderText(renderer, pos, flags, textColor, text.c_str(), ignoreEmbeddedColor, useGradient, flareAmount);
}

void ui::Manager::renderBackground(Renderer& renderer, std::string name)
{
    IntRect r(0, 0, width, height);
    if (!enableBackground) {
        renderer.renderRect(r, COLOR_BLACK, false);
        return;
    }
    if (!backgrounds.contains(name)) {
        return;
    }

    const Bitmap* bitmap = backgrounds[name].getPointer();
    if (bitmap == nullptr) {
        return;
    }

    renderer.drawBegin(Renderer::Primitive::DRAW_TRIANGLES, DRAW_2D | DRAW_TEXTURED | DRAW_NO_ZBUFFER | DRAW_UV_CLAMP);
    renderer.setTexture(backgrounds[name].getPointer());
    renderer.drawVertex(r.left, r.top, 0.0f, 0, 0);
    renderer.drawVertex(r.right, r.top, 0.0f, 1.0, 0);
    renderer.drawVertex(r.left, r.bottom, 0.0f, 0, 1.0);

    renderer.drawVertex(r.left, r.bottom, 0.0f, 0, 1.0);
    renderer.drawVertex(r.right, r.bottom, 0.0f, 1.0, 1.0);
    renderer.drawVertex(r.right, r.top, 0.0f, 1.0, 0);

    renderer.drawEnd();
}

void ui::Manager::renderElement(Renderer& renderer, int iElement, const IntRect& Position, int State, const Colour& Color, bool IsAdditive) const
{
    if (iElement < 0 || iElement >= elements.size()) {
        return;
    }
    const Element* element = elements[iElement];
    Bitmap*        bitmap = element->bitmap.getPointer();
    if (bitmap == nullptr) {
        return;
    }

    /*
    if( IsAdditive )
    {
        renderer.SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
        renderer.SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
    }
    */

    renderer.drawBegin(Renderer::Primitive::DRAW_TRIANGLES, DRAW_2D | DRAW_TEXTURED | DRAW_USE_ALPHA | DRAW_NO_ZBUFFER);
    renderer.setTexture(bitmap);
    int     ie = State * (element->cx * element->cy);
    Vector2 p(0.0, Position.top);
    Vector2 wh;
    for (int iy = 0; iy < element->cy; iy++) {
        // Reset x position
        p.x = Position.left;

        // Set Height
        if ((element->cy == 3) && (iy == 1)) {
            wh.y = Position.getHeight() - (element->r[2 * element->cx].getHeight() + element->r[0].getHeight());
        } else {
            wh.y = element->r[ie].getHeight();
        }

        // Loop on x
        for (int ix = 0; ix < element->cx; ix++) {
            Vector2 uv0, uv1;
            uv0.x = (element->r[ie].left + 0.0f) / bitmap->getWidth();
            uv0.y = (element->r[ie].top + 0.0f) / bitmap->getHeight();
            uv1.x = (element->r[ie].right - 0.0f) / bitmap->getWidth();
            uv1.y = (element->r[ie].bottom - 0.0f) / bitmap->getHeight();

            if ((element->cx == 3) && (ix == 1)) {
                wh.x = Position.getWidth() - (element->r[2].getWidth() + element->r[0].getWidth());
            } else {
                wh.x = element->r[ie].getWidth();
            }
            renderer.drawSpriteUV(Vector3(p.x, p.y, 0.0f), wh, uv0, uv1, Color);
            p.x += wh.x;
            ie++;
        }
        p.y += wh.y;
    }
    renderer.drawEnd();
}

void ui::Manager::setScreenSize(const IntRect& size)
{
    m_CurrScreenSize = size;
    m_GlowStartX = m_CurrScreenSize.left + 46;
    m_GlowEndX = m_CurrScreenSize.right - 46 - 16;
}

void ui::Manager::initScreenHighlight()
{
    m_ScreenHighlightID = findElement("highlight");
    m_ScreenGlowID = findElement("screenglow");
    m_HighlightAlpha = 0;
    m_ScreenHighlightEnabled = false;
    m_HighlightFadeUp = true;
    m_CycleFadeUp = true;
}

void ui::Manager::setScreenHighlight(const IntRect& pos)
{
    m_ScreenHighlightPos.top = m_CurrScreenSize.top + pos.top - 12;
    m_ScreenHighlightPos.bottom = m_CurrScreenSize.top + pos.bottom + 12;
    m_ScreenHighlightPos.left = m_CurrScreenSize.left + 22;
    m_ScreenHighlightPos.right = m_CurrScreenSize.right - 22;

    m_ScreenHighlightEnabled = true;
}

void ui::Manager::renderScreenHighlight(Renderer& renderer)
{
    int val = 64 + (m_HighlightAlpha * 1);
    renderElement(renderer, m_ScreenHighlightID, m_ScreenHighlightPos, 0, Colour(val, val, val, val), true);
}

void ui::Manager::renderScreenGlow(Renderer& renderer)
{
    if (!m_ScreenHighlightEnabled) {
        return;
    }

    if (m_isScaling) {
        return;
    }

    // render the highlight glow
    IntRect pos = m_ScreenHighlightPos;
    pos.left -= 4;
    pos.right += 4;
    int val = 128 + (m_HighlightAlpha * 2);
    renderElement(renderer, m_ScreenGlowID, pos, 0, Colour(val, val, val, val), true);
}

void ui::Manager::initGlowBar()
{
    m_GlowID = findElement("glow");

    m_GlowStartX = m_CurrScreenSize.left + 46;
    m_GlowEndX = m_CurrScreenSize.right - 46 - 16;

    m_GlowPos.left = m_GlowStartX;
    m_GlowPos.top = m_CurrScreenSize.top;
    m_GlowPos.right = 16;
    m_GlowPos.bottom = 7;

    m_GlowSpeed = 120;
    m_GlowOnTop = true;

    // initialize trail
    for (int i = 0; i < 8; i++) {
        m_GlowTrail[i].left = -1;
    }
}

void ui::Manager::renderGlowBar(Renderer& renderer)
{
    for (int i = 0; i < 8; i++) {
        if (m_GlowTrail[i].left != -1) {
            uint8_t val = 255 - (i * 32);
            renderElement(renderer, m_GlowID, m_GlowTrail[i], 0, Colour(val, val, val, val), true);
        }
    }
}

void ui::Manager::updateGlowBar(float deltaTime)
{
    deltaTime = deltaTime;

    if (m_GlowOnTop) {
        m_GlowPos.left += (int)((m_GlowSpeed * deltaTime) + 0.5f);

        if (m_GlowPos.left > m_GlowEndX) {
            m_GlowPos.left = m_GlowEndX;
            m_GlowPos.top = m_CurrScreenSize.bottom - 7;
            m_GlowOnTop = false;
        } else if (m_GlowPos.left < m_GlowStartX) {
            m_GlowPos.left = m_GlowStartX;

            for (int i = 0; i < 8; i++) {
                if (m_GlowTrail[i].left < m_GlowStartX) {
                    m_GlowTrail[i].left = -1;
                }
            }
        }
    } else {
        m_GlowPos.left -= (int)((m_GlowSpeed * deltaTime) + 0.5f);

        if (m_GlowPos.left < m_GlowStartX) {
            m_GlowPos.left = m_GlowStartX;
            m_GlowPos.top = m_CurrScreenSize.top;
            m_GlowOnTop = true;
        } else if (m_GlowPos.left > m_GlowEndX) {
            m_GlowPos.left = m_GlowEndX;

            for (int i = 0; i < 8; i++) {
                if (m_GlowTrail[i].left > m_GlowEndX) {
                    m_GlowTrail[i].left = -1;
                }
            }
        }
    }

    for (int i = 7; i > 0; i--) {
        m_GlowTrail[i] = m_GlowTrail[i - 1];
    }
    m_GlowTrail[0] = m_GlowPos;
}

void ui::Manager::setCursorVisible(User* user, bool state)
{
    user->cursorVisible = state;
}

bool ui::Manager::getCursorVisible(User* user) const
{
    return user->cursorVisible;
}

void ui::Manager::setCursorPos(User* user, int x, int y)
{
    user->cursorX = x;
    user->cursorY = y;

    auto* pWin = getWindowAtXY(user, x, y);

    // Has window under cursor changed?
    if (pWin != user->lastWindowUnderCursor) {
        // Call exit function if there was a window under the cursor
        if (user->lastWindowUnderCursor) {
            user->lastWindowUnderCursor->onCursorExit();
        }

        // Set new window under cursor and call enter function
        user->lastWindowUnderCursor = pWin;
        if (user->lastWindowUnderCursor) {
            user->lastWindowUnderCursor->onCursorEnter();
        }
    }
}

void ui::Manager::getCursorPos(User* user, int& x, int& y) const
{
    x = user->cursorX;
    y = user->cursorY;
}
