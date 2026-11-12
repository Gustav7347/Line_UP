#include "airplane.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 이 파일 안에서만 쓰는 변수 (static)
static GLuint cubeVAO, cubeVBO[2]; // 0: 위치, 1: 색상

// 직육면체 크기 (가로 X, 높이 Y, 깊이 Z)
static const glm::vec3 BOX_SIZE = glm::vec3(2.0f, 1.0f, 3.0f);

// 단위 육면체 (-0.5 ~ 0.5), 면당 삼각형 2개 = 6정점, 총 36정점
static const float cubeVertices[] = {
    // Front (+Z)
    -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
    // Back (-Z)
    -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    // Top (+Y)
    -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
    // Bottom (-Y)
    -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
    // Right (+X)
     0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,
    // Left (-X)
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f
};

// 조명이 없으므로 면마다 색을 다르게 해서 입체감이 보이도록 함
static const glm::vec3 faceColors[6] = {
    glm::vec3(0.9f, 0.3f, 0.3f), // Front  - 빨강
    glm::vec3(0.6f, 0.2f, 0.2f), // Back   - 어두운 빨강
    glm::vec3(0.3f, 0.8f, 0.3f), // Top    - 초록
    glm::vec3(0.2f, 0.5f, 0.2f), // Bottom - 어두운 초록
    glm::vec3(0.3f, 0.4f, 0.9f), // Right  - 파랑
    glm::vec3(0.2f, 0.25f, 0.6f) // Left   - 어두운 파랑
};

void InitAirplane() {
    // 정점 색상 데이터 (면마다 6정점 같은 색)
    float colors[36 * 3];
    for (int f = 0; f < 6; ++f) {
        for (int v = 0; v < 6; ++v) {
            int idx = (f * 6 + v) * 3;
            colors[idx + 0] = faceColors[f].r;
            colors[idx + 1] = faceColors[f].g;
            colors[idx + 2] = faceColors[f].b;
        }
    }

    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);
    glGenBuffers(2, cubeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);
}

void DrawAirplane(const glm::mat4& viewProj, GLuint transformLoc) {
    // 원점(0,0,0)에 직육면체 하나
    glm::mat4 model = glm::scale(glm::mat4(1.0f), BOX_SIZE);
    glm::mat4 mvp = viewProj * model;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}
