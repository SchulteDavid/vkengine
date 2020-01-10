#include <iostream>
#include <memory>

#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>

#include "render/util/vkutil.h"
#include "render/window.h"
#include "render/viewport.h"

int main(int argc, char ** argv) {


    std::shared_ptr<Window> window(new Window());

    std::shared_ptr<Viewport> view(new Viewport(window));


    while (!glfwWindowShouldClose(window->getGlfwWindow())) {

    }

    return 0;

}
