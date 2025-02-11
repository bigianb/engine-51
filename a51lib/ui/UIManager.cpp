#include "UIManager.h"

#include <iostream>

#include "../system/Renderer.h"
#include "../VectorMath.h"
#include "dialogs/esrbDialog.h"
#include "UIText.h"

ui::Manager::~Manager()
{
    // TODO: delete dialog class map entries.
}

void ui::Manager::init(Renderer& renderer, ResourceManager* rm)
{
    renderer.getRes(width, height);
    IntRect r(0, 0, width, height);
    userId = createUser(-1, r);
    enableUser(userId, false);

    registerWinClass("text", &ui::Text::factory);

    EsrbDialog::registerDialog(this);

    loadElement(rm, "frame", "UI_frame1.xbmp", 2, 3, 3);
    loadElement(rm, "frame2", "UI_frame2.xbmp", 1, 3, 3);
    loadElement(rm, "glow", "UI_barglow.xbmp", 1, 1, 1);
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

    /*
    // TODO: figure out the registrations
    if ((nStates > 0) && (cx > 0) && (cy > 0)) {
        //RegColor = pBitmap->getPixelColor( 0, 0 );

        // Find the registration markers
        x.reserve(cx + 1);
        y.reserve((cy * nStates) + 1);
        for (int i = 0; i < pBitmap->GetWidth(); i++) {
            if (pBitmap->GetPixelColor(i, 0) == RegColor) {
                x.push_back(i);
            }
        }
        for (int i = 0; i < pBitmap->GetHeight(); i++) {
            if (pBitmap->GetPixelColor(0, i) == RegColor) {
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
    */

    elements.push_back(element);
    return elements.size() - 1;
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
        pUser->captureWindow = 0;
        pUser->lastWindowUnderCursor = 0;
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
        /*
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

void ui::Manager::update(float deltaTime)
{
}

void ui::Manager::render(Renderer& renderer)
{
    for (User* user : users) {
        if (user->enabled) {
            //RenderBackground( user->background );
            /*
                        // Find Topmost Render Modal Dialog
                        s32 j = pUser->DialogStack.GetCount()-1;
                        while( (j > 0) && !(pUser->DialogStack[j]->GetFlags() & ui_win::WF_RENDERMODAL) )
                            j--;

                        // Make sure we start with a legal dialog
                        if( j < 0 ) j = 0;

                        // Render all Dialogs from the Render Modal one
                        for( ; j<pUser->DialogStack.GetCount() ; j++ )
                        {
                            pUser->DialogStack[j]->render( renderer, user->bounds.left, user->bounds.top );
                        }
                        */
        }
    }
}
