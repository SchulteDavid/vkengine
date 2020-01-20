#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H


class InputHandler
{
    public:
        InputHandler();
        virtual ~InputHandler();

        virtual void onKeyboard(int key, int scancode, int action, int mods);
        virtual void onMouseButton(int button, int action, int mods);
        virtual void onMouseMotion(double xpos, double ypos, double dx, double dy);

    protected:

    private:



};

#endif // INPUTHANDLER_H
