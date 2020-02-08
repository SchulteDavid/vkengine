#ifndef PLAYERCONTROLER_H
#define PLAYERCONTROLER_H

#include "inputhandler.h"
#include "render/camera.h"
#include "render/util/vkutil.h"

class PlayerControler : public InputHandler {
    public:
        PlayerControler(Camera * camera, const vkutil::VulkanState & state);
        virtual ~PlayerControler();

        void onMouseMotion(double xpos, double ypos, double dx, double dy);
        void onKeyboard(int key, int scancode, int action, int mods);
        void onMouseButton(int button, int action, int mods);

    protected:

    private:

        const vkutil::VulkanState & state;

        Camera * camera;
        bool hasCursor;

        double phi;
        double theta;
        double radius;

};

#endif // PLAYERCONTROLER_H
