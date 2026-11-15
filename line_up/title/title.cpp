#include "title.h"

#define STB_IMAGE_IMPLEMENTATION // stb_image 구현부는 프로젝트 전체에서 이 파일 한 곳에만
#include "stb_image.h"

#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>

// ------------------------------------------------------------
// 이미지 경로 (프로젝트 폴더 = 실행 작업 폴더 기준, glsl/ 폴더와 같은 위치)
// ------------------------------------------------------------
static const char* PATH_BG = "title.png";           // 1920x1080 배경
static const char* PATH_LOGO = "title_logo.png";      // 947x236 로고
static const char* PATH_BUTTON = "title_button.png";    // 724x72 PRESS ENTER
static const char* PATH_LIGHTNING = "title_lightning.png"; // 4x2 = 8프레임
static const char* PATH_RAIN = "title_rain.png";      // 4x3 = 12프레임 (우박)

// 모든 좌표는 1920x1080 기준 화면(왼쪽 위가 (0,0)인 픽셀)
static const float REF_W = 1920.0f, REF_H = 1080.0f;

static const float LOGO_X = 487.0f, LOGO_Y = 698.0f;   // 로고 원래 위치
static const float LOGO_BOB = 8.0f;                    // 위아래 흔들림 폭(px)
static const float LOGO_PERIOD = 1.8f;                 // 흔들림 주기(초)

static const float BUTTON_X = 570.0f, BUTTON_Y = 943.0f; // PRESS ENTER 위치
static const float BUTTON_BLINK = 0.45f;                 // 깜빡임 간격(초)

static const int   RAIN_COLS = 4, RAIN_ROWS = 3, RAIN_FRAMES = 12;
static const int   LIGHT_COLS = 4, LIGHT_ROWS = 2, LIGHT_FRAMES = 8;
static const float FRAME_TIME = 0.05f;                 // 스프라이트 프레임당 시간(초)
static const float LIGHT_WAIT_MIN = 2.0f, LIGHT_WAIT_MAX = 6.0f; // 번개 사이 대기(초)

// ------------------------------------------------------------
// 내부 상태
// ------------------------------------------------------------
struct TitleTex { GLuint id = 0; int w = 0, h = 0; };

static TitleTex texBg, texLogo, texButton, texLightning, texRain;
static GLuint titleProgram = 0, titleVAO = 0, titleVBO = 0;
static GLint locRect = -1, locUV = -1, locTex = -1;

static bool  titleActive = true;
static double titleStartTime = 0.0;
static double lightningStart = -1.0;   // 재생 시작 시각 (-1 = 대기 중)
static double lightningNext = 0.0;     // 다음 번개 시각
static std::mt19937 rng{ std::random_device{}() };

// ------------------------------------------------------------
// 셰이더 (타이틀 전용 2D 셰이더, 파일 없이 코드에 포함)
// ------------------------------------------------------------
static const char* TITLE_VS = R"(
#version 330 core
layout (location = 0) in vec2 aCorner; // (0,0)=왼쪽 위, (1,1)=오른쪽 아래
uniform vec4 uRect; // NDC 기준 x(왼쪽), y(위), 너비, 높이
uniform vec4 uUV;   // u0, v0, 너비, 높이 (스프라이트 시트에서 한 프레임 영역)
out vec2 vUV;
void main() {
    gl_Position = vec4(uRect.x + aCorner.x * uRect.z, uRect.y - aCorner.y * uRect.w, 0.0, 1.0);
    vUV = uUV.xy + aCorner * uUV.zw;
}
)";

static const char* TITLE_FS = R"(
#version 330 core
in vec2 vUV;
uniform sampler2D uTex;
out vec4 FragColor;
void main() {
    FragColor = texture(uTex, vUV);
}
)";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s, 512, NULL, log);
        std::cerr << "타이틀 셰이더 컴파일 실패: " << log << std::endl;
    }
    return s;
}

// ------------------------------------------------------------
// 텍스처 로드 (stb_image)
// ------------------------------------------------------------
static TitleTex loadTexture(const char* path) {
    TitleTex t;
    int ch;
    // 이미지 첫 줄 = 화면 위쪽으로 그대로 사용하므로 뒤집지 않음 (UV를 위→아래로 계산)
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(path, &t.w, &t.h, &ch, 4); // 항상 RGBA로 읽기
    if (!data) {
        std::cerr << "타이틀 이미지를 열 수 없음: " << path << " (" << stbi_failure_reason() << ")" << std::endl;
        return t;
    }

    GLint maxSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    if (t.w > maxSize || t.h > maxSize)
        std::cerr << "경고: " << path << " 크기(" << t.w << "x" << t.h << ")가 GPU 최대 텍스처 크기(" << maxSize << ")보다 큼" << std::endl;

    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, t.w, t.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    return t;
}

static double randomWait() {
    std::uniform_real_distribution<double> d(LIGHT_WAIT_MIN, LIGHT_WAIT_MAX);
    return d(rng);
}

void InitTitle() {
    // 셰이더
    GLuint vs = compileShader(GL_VERTEX_SHADER, TITLE_VS);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, TITLE_FS);
    titleProgram = glCreateProgram();
    glAttachShader(titleProgram, vs);
    glAttachShader(titleProgram, fs);
    glLinkProgram(titleProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
    locRect = glGetUniformLocation(titleProgram, "uRect");
    locUV = glGetUniformLocation(titleProgram, "uUV");
    locTex = glGetUniformLocation(titleProgram, "uTex");

    // 사각형 하나 (삼각형 2개, 모서리 좌표만 넣고 위치/크기는 uniform으로)
    float corners[] = {
        0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,
        0.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f,
    };
    glGenVertexArrays(1, &titleVAO);
    glBindVertexArray(titleVAO);
    glGenBuffers(1, &titleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, titleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // 이미지 (스프라이트 시트는 커서 Debug 빌드에선 몇 초 걸릴 수 있음)
    texBg = loadTexture(PATH_BG);
    texLightning = loadTexture(PATH_LIGHTNING);
    texRain = loadTexture(PATH_RAIN);
    texLogo = loadTexture(PATH_LOGO);
    texButton = loadTexture(PATH_BUTTON);

    // 로딩이 끝난 시점부터 애니메이션 시간 시작
    titleStartTime = glfwGetTime();
    lightningStart = -1.0;
    lightningNext = randomWait();
}

bool IsTitleActive() { return titleActive; }

void TitleHandleKey(GLFWwindow* window, int key) {
    switch (key) {
    case GLFW_KEY_ENTER:
    case GLFW_KEY_KP_ENTER:
        titleActive = false; // 다음 프레임부터 DrawScene()
        break;
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    }
}

// ------------------------------------------------------------
// 그리기
// ------------------------------------------------------------
// 1920x1080 기준 화면을 실제 창에 꽉 차게(비율 유지, 넘치는 부분은 잘림) 맞추는 값
static float fitScale, fitOffX, fitOffY;
static int screenW, screenH;

// 기준 좌표(px)의 사각형을 그림. frame >= 0 이면 스프라이트 시트의 해당 칸만 사용
static void drawQuad(const TitleTex& tex, float x, float y, float w, float h,
    int cols = 1, int rows = 1, int frame = 0) {
    if (tex.id == 0) return; // 로드 실패한 이미지는 건너뜀

    // 기준 px → 실제 화면 px → NDC
    float sx = fitOffX + x * fitScale;
    float sy = fitOffY + y * fitScale;
    float sw = w * fitScale;
    float sh = h * fitScale;
    float ndcX = sx / screenW * 2.0f - 1.0f;
    float ndcY = 1.0f - sy / screenH * 2.0f;
    float ndcW = sw / screenW * 2.0f;
    float ndcH = sh / screenH * 2.0f;

    float du = 1.0f / cols, dv = 1.0f / rows;
    float u0 = (frame % cols) * du;
    float v0 = (frame / cols) * dv;

    glUniform4f(locRect, ndcX, ndcY, ndcW, ndcH);
    glUniform4f(locUV, u0, v0, du, dv);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void DrawTitle() {
    glfwGetFramebufferSize(glfwGetCurrentContext(), &screenW, &screenH);
    if (screenW <= 0 || screenH <= 0) return; // 최소화 상태

    fitScale = std::max(screenW / REF_W, screenH / REF_H);
    fitOffX = (screenW - REF_W * fitScale) * 0.5f;
    fitOffY = (screenH - REF_H * fitScale) * 0.5f;

    glViewport(0, 0, screenW, screenH);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(titleProgram);
    glUniform1i(locTex, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(titleVAO);

    double t = glfwGetTime() - titleStartTime;

    // 1. 배경
    drawQuad(texBg, 0, 0, REF_W, REF_H);

    // 2. 번개: 대기 → 8프레임 1회 재생 → 2~6초 랜덤 대기 → 반복
    if (lightningStart < 0.0 && t >= lightningNext)
        lightningStart = t;
    if (lightningStart >= 0.0) {
        int frame = (int)((t - lightningStart) / FRAME_TIME);
        if (frame < LIGHT_FRAMES) {
            drawQuad(texLightning, 0, 0, REF_W, REF_H, LIGHT_COLS, LIGHT_ROWS, frame);
        }
        else {
            lightningStart = -1.0;
            lightningNext = t + randomWait();
        }
    }

    // 3. 우박: 12프레임 반복
    int rainFrame = (int)(t / FRAME_TIME) % RAIN_FRAMES;
    drawQuad(texRain, 0, 0, REF_W, REF_H, RAIN_COLS, RAIN_ROWS, rainFrame);

    // 4. 로고: 위아래로 살짝 둥실둥실
    const float PI = 3.14159265f;
    float logoY = LOGO_Y + LOGO_BOB * std::sin(2.0f * PI * (float)t / LOGO_PERIOD);
    drawQuad(texLogo, LOGO_X, logoY, (float)texLogo.w, (float)texLogo.h);

    // 5. PRESS ENTER: 깜빡임
    if (((int)(t / BUTTON_BLINK)) % 2 == 0)
        drawQuad(texButton, BUTTON_X, BUTTON_Y, (float)texButton.w, (float)texButton.h);

    // 게임 화면(DrawScene)에 영향 없도록 상태 정리
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}
