#pragma once

#include "../UIDialog.h"
#include "../UIManager.h"
#include "../UIText.h"

namespace ui
{
    class StartGameDialog : public Dialog
    {
    public:
        StartGameDialog();
        ~StartGameDialog();

        static void registerDialog(Manager*);

        bool create(User*           user,
                    Manager*        manager,
                    DialogTemplate* dialogTemplate,
                    const IntRect&  position,
                    Window*         parent,
                    int             flags);

        void render(Renderer& renderer, int ox = 0, int oy = 0) override;

        void onUpdate(float deltaTime) override;

    protected:
        Text* text;
        bool  startLoading;
    };
}
