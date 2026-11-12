#include "render.h"
#include "airplane.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 이 파일 안에서만 쓰는 변수 (static)
static GLuint shaderProgramID;
static GLuint axisVAO, axisVBO[2]; // 좌표축용
static int axisVertexCount = 0;

// 좌표축 길이 (원점에서 양쪽으로)
static const float AXIS_LEN = 5.0f;

// --- 카메라 조작 변수 ---
static bool  isPerspective = true;  // P: 원근/직각 투영 전환
static float cameraAngleY = 45.0f;  // ←/→: 카메라 Y축 회전
static float cameraRadius = 8.0f;   // +/-: 줌 인/아웃

// 셰이더
static std::string filetobuf(const char* file) {
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open()) {
        std::cerr << "셰이더 파일을 열 수 없음: " << file << std::endl;
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
}

void InitShader() {
    std::string vStr = filetobuf("glsl/airplane_vertex.glsl");
    std::string fStr = filetobuf("glsl/airplane_fragment.glsl");
    const char* vSrc = vStr.c_str();
    const char* fSrc = fStr.c_str();

    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vSrc, NULL);
    glCompileShader(vShader);

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fSrc, NULL);
    glCompileShader(fShader);

    shaderProgramID = glCreateProgram();
    glAttachShader(shaderProgramID, vShader);
    glAttachShader(shaderProgramID, fShader);
    glLinkProgram(shaderProgramID);

    glDeleteShader(vShader);
    glDeleteShader(fShader);
}

// 좌표축
// 선분 하나(정점 2개)를 버퍼에 추가
static void addLine(std::vector<float>& pos, std::vector<float>& col,
    glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    pos.insert(pos.end(), { a.x, a.y, a.z, b.x, b.y, b.z });
    col.insert(col.end(), { c.r, c.g, c.b, c.r, c.g, c.b });
}

// 축 하나 생성
//  - 양(+)의 방향: 밝은 색 실선 + 끝에 화살촉
//  - 음(-)의 방향: 어두운 색 점선
static void buildAxis(std::vector<float>& pos, std::vector<float>& col,
    glm::vec3 dir, glm::vec3 side, glm::vec3 color) {
    glm::vec3 bright = color;
    glm::vec3 dark = color * 0.35f;

    // + 방향 실선
    addLine(pos, col, glm::vec3(0.0f), dir * AXIS_LEN, bright);

    // + 끝 화살촉 (두 방향으로 V자 2쌍 → 어느 각도에서 봐도 보이게)
    glm::vec3 tip = dir * AXIS_LEN;
    glm::vec3 back = dir * (AXIS_LEN - 0.4f);
    glm::vec3 side2 = glm::cross(dir, side);
    addLine(pos, col, tip, back + side * 0.2f, bright);
    addLine(pos, col, tip, back - side * 0.2f, bright);
    addLine(pos, col, tip, back + side2 * 0.2f, bright);
    addLine(pos, col, tip, back - side2 * 0.2f, bright);

    // - 방향 점선 (0.2 그리고 0.15 띄우기)
    for (float t = 0.0f; t < AXIS_LEN; t += 0.35f) {
        float t2 = t + 0.2f;
        if (t2 > AXIS_LEN) t2 = AXIS_LEN;
        addLine(pos, col, -dir * t, -dir * t2, dark);
    }
}

void InitAxis() {
    std::vector<float> pos, col;
    buildAxis(pos, col, glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(1.0f, 0.0f, 0.0f)); // X: 빨강
    buildAxis(pos, col, glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), glm::vec3(0.0f, 1.0f, 0.0f)); // Y: 초록
    buildAxis(pos, col, glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0.0f, 0.0f, 1.0f)); // Z: 파랑
    axisVertexCount = (int)(pos.size() / 3);

    glGenVertexArrays(1, &axisVAO);
    glBindVertexArray(axisVAO);
    glGenBuffers(2, axisVBO);

    glBindBuffer(GL_ARRAY_BUFFER, axisVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(float), pos.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, axisVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, col.size() * sizeof(float), col.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);
}

// 키 입력
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    switch (key) {
    case GLFW_KEY_Q: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    case GLFW_KEY_P: isPerspective = !isPerspective; break;
    case GLFW_KEY_R:
        isPerspective = true;
        cameraAngleY = 45.0f;
        cameraRadius = 8.0f;
        break;
    case GLFW_KEY_EQUAL:
        cameraRadius -= 0.5f;
        if (cameraRadius < 2.0f) cameraRadius = 2.0f;
        break;
    case GLFW_KEY_MINUS:
        cameraRadius += 0.5f;
        if (cameraRadius > 30.0f) cameraRadius = 30.0f;
        break;
    case GLFW_KEY_LEFT:  cameraAngleY -= 3.0f; break;
    case GLFW_KEY_RIGHT: cameraAngleY += 3.0f; break;
    }
}

// 장면 그리기
void DrawScene() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.7f, 0.9f, 1.0f); // 하늘색 배경
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    GLuint transformLoc = glGetUniformLocation(shaderProgramID, "transform");

    // --- 카메라: 원점을 바라보며 Y축 기준으로 공전 ---
    float radY = glm::radians(cameraAngleY);
    float pitch = glm::radians(30.0f);
    glm::vec3 cameraPos = glm::vec3(
        cameraRadius * sin(radY) * cos(pitch),
        cameraRadius * sin(pitch),
        cameraRadius * cos(radY) * cos(pitch));
    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection;
    if (isPerspective) {
        projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    }
    else {
        float s = cameraRadius * 0.4f;
        projection = glm::ortho(-s, s, -s, s, 0.1f, 100.0f);
    }

    glm::mat4 viewProj = projection * view;

    // --- 좌표축 (모델 변환 없음) ---
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(viewProj));
    glBindVertexArray(axisVAO);
    glDrawArrays(GL_LINES, 0, axisVertexCount);

    // --- 비행기(직육면체) ---
    DrawAirplane(viewProj, transformLoc);
}
