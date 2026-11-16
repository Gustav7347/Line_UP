#pragma once
#include <GL/glew.h>
#include <GL/glfw3.h>

// 하늘색 배경 (메인 화면과 뷰포트 안쪽 공통)
static const float SKY_R = 0.5f, SKY_G = 0.7f, SKY_B = 0.9f;

// 오른쪽 아래 사각형 뷰포트: 1인칭일 때 3인칭으로 본 비행기 (검은 테두리)
void DrawThirdPersonViewport(int width, int height, GLuint transformLoc);

// 오른쪽 위 원형 미니맵: XZ 평면을 위에서 내려다본 시점, 비행기가 항상 중심
void DrawMinimap(int width, int height, GLuint transformLoc);
