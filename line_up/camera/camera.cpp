#include "camera.h"

#include <iostream>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

// 비행기 기준 (지금 비행기는 원점에 고정, 앞쪽 = +Z, 위쪽 = +Y)
static const glm::vec3 PLANE_POS = glm::vec3(0.0f, 0.0f, 0.0f);
static const glm::vec3 PLANE_FORWARD = glm::vec3(0.0f, 0.0f, 1.0f);
static const glm::vec3 PLANE_UP = glm::vec3(0.0f, 1.0f, 0.0f);

// 이 파일 안에서만 쓰는 변수 (static)
static ViewMode viewMode = ViewMode::ThirdPerson;

// --- 3인칭 카메라 조작 변수 ---
static bool  isPerspective = true;  // P: 원근/직각 투영 전환 (3인칭에서만)
static float orbitAngle = 0.0f;     // ←/→: 비행기 주위 회전 (0 = 비행기 바로 뒤)
static const float DEFAULT_RADIUS = 20.0f; // 3인칭 기본 거리 (클수록 카메라가 뒤로)
static float cameraRadius = DEFAULT_RADIUS; // +/-: 비행기와의 거리
static const float ORBIT_PITCH = 25.0f; // 위에서 내려다보는 각도 (클수록 카메라가 높아짐)

// --- 1인칭: 조종석 위치 (비행기 로컬 좌표) ---
// 박스는 X ±1, Y ±0.5, Z ±1.5 → 윗면 바로 위, 앞쪽에 눈 위치를 둠
// 그러면 화면 아래쪽에 비행기 앞부분(기수)이 살짝 보여서 "타고 있는" 느낌이 남
static const glm::vec3 COCKPIT_EYE = glm::vec3(0.0f, 0.7f, 0.6f);

void ToggleViewMode() {
    viewMode = (viewMode == ViewMode::FirstPerson) ? ViewMode::ThirdPerson : ViewMode::FirstPerson;
    std::cout << (viewMode == ViewMode::FirstPerson ? "[시점] 1인칭 (조종석)" : "[시점] 3인칭") << std::endl;
}

ViewMode GetViewMode() { return viewMode; }

bool CameraHandleKey(int key) {
    switch (key) {
    case GLFW_KEY_V: ToggleViewMode(); return true;
    case GLFW_KEY_P: isPerspective = !isPerspective; return true;
    case GLFW_KEY_R:
        isPerspective = true;
        orbitAngle = 0.0f;
        cameraRadius = DEFAULT_RADIUS;
        return true;
    case GLFW_KEY_EQUAL:
        cameraRadius -= 0.5f;
        if (cameraRadius < 4.0f) cameraRadius = 4.0f;
        return true;
    case GLFW_KEY_MINUS:
        cameraRadius += 0.5f;
        if (cameraRadius > 30.0f) cameraRadius = 30.0f;
        return true;
    case GLFW_KEY_LEFT:  orbitAngle -= 3.0f; return true;
    case GLFW_KEY_RIGHT: orbitAngle += 3.0f; return true;
    }
    return false;
}

glm::mat4 GetViewMatrixFor(ViewMode mode) {
    if (mode == ViewMode::FirstPerson) {
        // 조종석에서 비행기 앞쪽을 바라봄
        glm::vec3 eye = PLANE_POS + COCKPIT_EYE;
        return glm::lookAt(eye, eye + PLANE_FORWARD, PLANE_UP);
    }

    // 3인칭: 비행기 뒤쪽(-Z) + 위쪽에서 비행기를 바라봄
    float yaw = glm::radians(180.0f + orbitAngle); // 180 = 뒤쪽
    float pitch = glm::radians(ORBIT_PITCH);
    glm::vec3 offset = glm::vec3(
        cameraRadius * sin(yaw) * cos(pitch),
        cameraRadius * sin(pitch),
        cameraRadius * cos(yaw) * cos(pitch));
    return glm::lookAt(PLANE_POS + offset, PLANE_POS, PLANE_UP);
}

glm::mat4 GetProjectionMatrixFor(ViewMode mode, float aspect) {
    // 1인칭은 항상 원근 투영, near를 작게 해서 기수가 잘리지 않게 함
    if (mode == ViewMode::FirstPerson)
        return glm::perspective(glm::radians(60.0f), aspect, 0.05f, 200.0f);

    if (isPerspective)
        return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 200.0f);

    float s = cameraRadius * 0.4f;
    return glm::ortho(-s * aspect, s * aspect, -s, s, 0.1f, 200.0f);
}

// 현재 시점 모드 기준
glm::mat4 GetViewMatrix() { return GetViewMatrixFor(viewMode); }
glm::mat4 GetProjectionMatrix(float aspect) { return GetProjectionMatrixFor(viewMode, aspect); }

glm::vec3 GetPlanePosition() { return PLANE_POS; }
