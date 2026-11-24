#include "camera.h"
#include "airplane.h"

#include <iostream>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

// 이 파일 안에서만 쓰는 변수 (static)
static ViewMode viewMode = ViewMode::ThirdPerson;

// --- 3인칭 카메라 조작 변수 ---
static bool  isPerspective = true;  // P: 원근/직각 투영 전환 (3인칭에서만)
static float orbitAngle = 0.0f;     // ←/→: 비행기 주위 회전 (0 = 비행기 바로 뒤)
static const float DEFAULT_RADIUS = 20.0f; // 3인칭 기본 거리 (클수록 카메라가 뒤로)
static float cameraRadius = DEFAULT_RADIUS; // +/-: 비행기와의 거리
static const float ORBIT_PITCH = 15.0f; // 비행기 뒤에서 하늘과 활주로가 함께 보이는 각도
static constexpr float WORLD_VIEW_DISTANCE = 3200.0f;

// --- 1인칭: 조종석 위치 (비행기 로컬 좌표) ---
// 비행기 모델 길이 15 기준: 동체 윗면 바로 위, 기수 쪽에 눈 위치를 둠
// (예전 직육면체용 (0, 0.7, 0.6)은 동체 안쪽이라 화면이 가려짐)
static const glm::vec3 COCKPIT_EYE = glm::vec3(0.0f, 2.1f, 6.0f);

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
    glm::vec3 planePosition = GetAirplanePosition();
    glm::vec3 forward = GetAirplaneForward();

    if (mode == ViewMode::FirstPerson) {
        glm::vec3 up = GetAirplaneUp();
        glm::vec3 right = glm::normalize(glm::cross(up, forward));
        // W/S 기울기(피치)만큼 비행기 오른쪽 축을 기준으로 시선도 같이 들고 숙임
        glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), glm::radians(-GetAirplanePitch()), right);
        forward = glm::vec3(pitchRot * glm::vec4(forward, 0.0f));
        up = glm::vec3(pitchRot * glm::vec4(up, 0.0f));
        glm::vec3 eye = planePosition
            + right * COCKPIT_EYE.x
            + up * COCKPIT_EYE.y
            + forward * COCKPIT_EYE.z;
        return glm::lookAt(eye, eye + forward, up);
    }

    // 3인칭: 비행기의 위쪽(up)을 기준으로 카메라를 둠
    // Q/E로 비행기가 좌우로 기울면(롤) up도 같이 기울어서 카메라도 함께 회전함
    float pitch = glm::radians(ORBIT_PITCH);
    float orbit = glm::radians(orbitAngle);
    glm::vec3 up = GetAirplaneUp();
    glm::vec3 right = glm::normalize(glm::cross(up, forward));
    glm::vec3 horizontal = -forward * std::cos(orbit) - right * std::sin(orbit);
    glm::vec3 offset = cameraRadius * (
        horizontal * std::cos(pitch) + up * std::sin(pitch));
    return glm::lookAt(planePosition + offset, planePosition, up);
}

glm::mat4 GetProjectionMatrixFor(ViewMode mode, float aspect) {
    // 1인칭은 항상 원근 투영, near를 작게 해서 기수가 잘리지 않게 함
    if (mode == ViewMode::FirstPerson)
        return glm::perspective(glm::radians(60.0f), aspect, 0.05f, WORLD_VIEW_DISTANCE);

    if (isPerspective)
        return glm::perspective(glm::radians(55.0f), aspect, 0.1f, WORLD_VIEW_DISTANCE);

    float s = cameraRadius * 0.4f;
    return glm::ortho(-s * aspect, s * aspect, -s, s, 0.1f, WORLD_VIEW_DISTANCE);
}

// 현재 시점 모드 기준
glm::mat4 GetViewMatrix() { return GetViewMatrixFor(viewMode); }
glm::mat4 GetProjectionMatrix(float aspect) { return GetProjectionMatrixFor(viewMode, aspect); }

glm::vec3 GetPlanePosition() { return GetAirplanePosition(); }
