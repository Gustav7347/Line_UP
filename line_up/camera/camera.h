#pragma once
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>

// 시점 모드
enum class ViewMode {
    FirstPerson,   // 1인칭: 조종석에 앉아 앞을 보는 시점
    ThirdPerson    // 3인칭: 비행기 뒤·위에서 비행기를 바라보는 시점
};

void      ToggleViewMode();          // V키: 1인칭 <-> 3인칭
ViewMode  GetViewMode();

// 카메라 키 입력 처리 (P, R, +/-, ←/→). 처리했으면 true
bool      CameraHandleKey(int key);

// 비행기 위치/방향을 기준으로 매 프레임 계산
glm::mat4 GetViewMatrix();
glm::mat4 GetProjectionMatrix(float aspect);

// 특정 시점 모드로 계산 (1인칭일 때 오른쪽 아래 미니 뷰포트에 3인칭을 그릴 때 사용)
glm::mat4 GetViewMatrixFor(ViewMode mode);
glm::mat4 GetProjectionMatrixFor(ViewMode mode, float aspect);

// 비행기 위치 (미니맵 중심으로 사용)
glm::vec3 GetPlanePosition();
