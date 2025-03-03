#pragma once

#include "../UIDialog.h"
#include "../UIManager.h"
#include "../UIText.h"
#include "../UIBitmap.h"

namespace ui
{
    /*
        Waits for the user to press start.
        The movie 'StartScreen' plays in the background and the logo bitmap is hidden.
        After a given time, the movie changes to 'attract', the logo bitmap is shown and the
        text is changed to 'demo mode'.
    */
    class PressStartDialog : public Dialog
    {
    public:
        PressStartDialog();
        ~PressStartDialog();

        static void registerDialog(Manager*);

        bool create(User*           user,
                    Manager*        manager,
                    DialogTemplate* dialogTemplate,
                    const IntRect&  position,
                    Window*         parent,
                    int             flags);

        void render(Renderer& renderer, int ox = 0, int oy = 0) override;

        void onUpdate(float deltaTime) override;

        void onPadSelect() override;

    protected:
        Text* text;
        BitmapControl* logoBitmap;
    };
}
