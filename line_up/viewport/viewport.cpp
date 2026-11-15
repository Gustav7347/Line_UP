#include "viewport.h"
#include "render.h"
#include "camera.h"

#include <vector>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 뷰포트 공통 설정
// 미니 뷰포트 설정 (1인칭일 때 오른쪽 아래에 3인칭 화면)
static const float MINI_SIZE_RATIO = 0.3f;  // 화면 크기 대비 비율
static const float MINI_MARGIN_CM = 1.0f;  // 화면 가장자리와의 간격
static const float MINI_BORDER_CM = 0.1f;  // 검은 테두리 두께 (미니 뷰포트, 원형 미니맵 공통)

// cm를 픽셀로 바꿈 (모니터 실제 크기 기준, 못 읽으면 96 DPI로 가정)
static int CmToPixels(float cm, int framebufferWidth) {
    float pxPerMm = 96.0f / 25.4f;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor) {
        int widthMM = 0, heightMM = 0;
        glfwGetMonitorPhysicalSize(monitor, &widthMM, &heightMM);
        if (widthMM > 0) pxPerMm = (float)framebufferWidth / (float)widthMM;
    }
    return (int)(cm * 10.0f * pxPerMm + 0.5f);
}


// --- 오른쪽 아래 사각형 뷰포트: 1인칭일 때 3인칭으로 본 비행기 ---
void DrawThirdPersonViewport(int width, int height, GLuint transformLoc) {
    int border = CmToPixels(MINI_BORDER_CM, width);
    int margin = CmToPixels(MINI_MARGIN_CM, width);

    // 안쪽(실제 그림) 크기: 화면 비율 그대로 축소
    int innerW = (int)(width * MINI_SIZE_RATIO);
    int innerH = (int)(height * MINI_SIZE_RATIO);
    int innerX = width - margin - border - innerW; // 오른쪽 아래 (OpenGL은 왼쪽 아래가 원점)
    int innerY = margin + border;

    glEnable(GL_SCISSOR_TEST);

    // 1) 테두리: 안쪽보다 border만큼 큰 영역을 검은색으로 칠함
    glScissor(innerX - border, innerY - border, innerW + border * 2, innerH + border * 2);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 2) 안쪽: 하늘색 + 깊이 초기화 후 3인칭으로 다시 그림
    glScissor(innerX, innerY, innerW, innerH);
    glClearColor(SKY_R, SKY_G, SKY_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_SCISSOR_TEST);

    glViewport(innerX, innerY, innerW, innerH);
    float miniAspect = (float)innerW / (float)innerH;
    glm::mat4 miniViewProj = GetProjectionMatrixFor(ViewMode::ThirdPerson, miniAspect)
        * GetViewMatrixFor(ViewMode::ThirdPerson);
    DrawWorld(miniViewProj, transformLoc);

    glViewport(0, 0, width, height); // 원래대로
}

// --- 원형 미니맵 (오른쪽 위): XZ 평면을 위에서 내려다본 시점, 비행기가 항상 중심 ---
static const float MAP_SIZE_RATIO = 0.3f;   // 원 지름 = 화면 높이 * 비율
static const float MAP_RANGE = 15.0f;  // 원 반지름 안에 보이는 월드 거리
static const int   CIRCLE_SEGMENTS = 64;
static GLuint circleVAO = 0, circleVBO[2];

// 원(삼각형 팬)을 만든 VAO. 위치는 -1~1 (NDC), 색은 나중에 상수 속성으로 지정
static void InitCircle() {
    std::vector<float> pos, col;
    pos.insert(pos.end(), { 0.0f, 0.0f, 0.0f });
    for (int i = 0; i <= CIRCLE_SEGMENTS; ++i) {
        float a = 2.0f * 3.14159265f * i / CIRCLE_SEGMENTS;
        pos.insert(pos.end(), { cosf(a), sinf(a), 0.0f });
    }
    glGenVertexArrays(1, &circleVAO);
    glBindVertexArray(circleVAO);
    glGenBuffers(1, circleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(float), pos.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    // 색상(속성 1)은 배열을 쓰지 않고 glVertexAttrib3f로 한 색을 줌
    glDisableVertexAttribArray(1);
}

// 현재 뷰포트 안에 반지름 r(NDC)짜리 원을 한 색으로 그림
static void DrawCircle(GLuint transformLoc, float r, float cr, float cg, float cb) {
    glm::mat4 m = glm::scale(glm::mat4(1.0f), glm::vec3(r, r, 1.0f));
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(m));
    glBindVertexArray(circleVAO);
    glVertexAttrib3f(1, cr, cg, cb);
    glDrawArrays(GL_TRIANGLE_FAN, 0, CIRCLE_SEGMENTS + 2);
}

void DrawMinimap(int width, int height, GLuint transformLoc) {
    if (circleVAO == 0) InitCircle();

    int border = CmToPixels(MINI_BORDER_CM, width);
    int margin = CmToPixels(MINI_MARGIN_CM, width);
    int innerD = (int)(height * MAP_SIZE_RATIO);   // 안쪽 원 지름
    int outerD = innerD + border * 2;              // 테두리 포함 지름
    int x = width - margin - outerD;               // 오른쪽 위
    int y = height - margin - outerD;
    float innerR = (float)innerD / (float)outerD;  // 정사각형 뷰포트 안에서 안쪽 원 반지름 (NDC)

    glViewport(x, y, outerD, outerD);

    // 이 정사각형 영역의 깊이/스텐실만 초기화 (색은 그대로 → 원 바깥은 메인 화면이 보임)
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, outerD, outerD);
    glClearStencil(0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    // 1) 검은 테두리 원 + 하늘색 안쪽 원. 안쪽 원 자리에 스텐실 = 1 기록
    glDisable(GL_DEPTH_TEST);
    DrawCircle(transformLoc, 1.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0xFF);
    DrawCircle(transformLoc, innerR, SKY_R, SKY_G, SKY_B);

    // 2) 스텐실 = 1인 곳(안쪽 원)에만 위에서 내려다본 장면을 그림
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilMask(0x00);
    glEnable(GL_DEPTH_TEST);

    glm::vec3 center = GetPlanePosition();
    // 위(+Y)에서 아래로 내려다봄. 화면 위쪽 = 비행기 앞쪽(+Z)
    glm::mat4 view = glm::lookAt(center + glm::vec3(0.0f, 50.0f, 0.0f), center, glm::vec3(0.0f, 0.0f, 1.0f));
    // 정사각형 뷰포트 전체가 ±(MAP_RANGE / innerR) → 안쪽 원 반지름이 MAP_RANGE
    float s = MAP_RANGE / innerR;
    glm::mat4 proj = glm::ortho(-s, s, -s, s, 0.1f, 100.0f);
    DrawWorld(proj * view, transformLoc);

    glStencilMask(0xFF);
    glDisable(GL_STENCIL_TEST);
    glViewport(0, 0, width, height); // 원래대로
}
