#pragma once

#include "../UIDialog.h"
#include "../UIManager.h"
#include "../UIText.h"

namespace ui
{
    class EsrbDialog : public Dialog
    {
    public:
        EsrbDialog();
        ~EsrbDialog();

        static void registerDialog(Manager*);

        bool create(User*           user,
                    Manager*        manager,
                    DialogTemplate* dialogTemplate,
                    const IntRect&  position,
                    Window*         parent,
                    int             flags);

        void onUpdate(float deltaTime) override;

        void onPadSelect(Window* ) override;

    protected:
        Text* text;
        float waitTime;
    };
}
