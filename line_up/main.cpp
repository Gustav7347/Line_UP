#include <GL/glew.h>
#include <GL/glfw3.h>

#include "render.h"
#include "airplane.h"
#include "title.h"

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 각자 모니터 해상도에 맞춰 전체화면으로 생성
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    glfwWindowHint(GLFW_STENCIL_BITS, 8); // 원형 미니맵에 스텐실 버퍼 사용

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Line UP!", monitor, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    InitShader();
    InitAirplane();
    InitAxis();
    InitTitle(); // 타이틀 이미지 로드
    glfwSetKeyCallback(window, key_callback);

    double previousTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        if (IsTitleActive()) DrawTitle(); // Enter 누르기 전: 타이틀 화면
        else DrawScene();                 // Enter 누른 후: 게임 화면
        glfwPollEvents();
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;

        UpdateAirplane(deltaTime,
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);

        DrawScene();
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
