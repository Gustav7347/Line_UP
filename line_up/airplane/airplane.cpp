#include "airplane.h"
#include "map.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 이 파일 안에서만 쓰는 변수 (static)
static GLuint cubeVAO, cubeVBO[2]; // 0: 위치, 1: 색상
// 직육면체 크기 (가로 X, 높이 Y, 깊이 Z)
static const glm::vec3 BOX_SIZE(2.0f, 1.0f, 3.0f);
static glm::vec3 airplanePosition(0.0f, GROUND_Y + BOX_SIZE.y * 0.5f, RUNWAY_START_Z + 3.0f);
static float headingAngle = 0.0f;
static float rollAngle = 0.0f;
static float flightSpeed = 0.0f;

static const float MAX_FLIGHT_SPEED = 80.0f;
static const float ACCELERATION = 20.0f;
static const float BRAKE_DECELERATION = 35.0f;
static const float TURN_SPEED = 90.0f;
static const float ROLL_SPEED = 180.0f;
static const float ALTITUDE_SPEED = 3.0f;

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

static glm::mat4 AirplaneRotation() {
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(headingAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    return glm::rotate(rotation, glm::radians(rollAngle), glm::vec3(0.0f, 0.0f, 1.0f));
}

void DrawAirplane(const glm::mat4& viewProj, GLuint transformLoc) {
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), airplanePosition);
    glm::mat4 model = translation * glm::scale(AirplaneRotation(), BOX_SIZE);
    glm::mat4 mvp = viewProj * model;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void UpdateAirplane(float deltaTime, bool ascend, bool descend,
    bool turnLeft, bool turnRight, bool rollLeft, bool rollRight,
    bool accelerate, bool brake) {
    headingAngle += (static_cast<int>(turnLeft) - static_cast<int>(turnRight)) * TURN_SPEED * deltaTime;
    rollAngle += (static_cast<int>(rollRight) - static_cast<int>(rollLeft)) * ROLL_SPEED * deltaTime;
    headingAngle = std::remainder(headingAngle, 360.0f);
    rollAngle = std::remainder(rollAngle, 360.0f);

    if (brake) flightSpeed = std::max(0.0f, flightSpeed - BRAKE_DECELERATION * deltaTime);
    else if (accelerate) flightSpeed = std::min(MAX_FLIGHT_SPEED, flightSpeed + ACCELERATION * deltaTime);

    // 회전한 직육면체의 월드 축 기준 반폭과 반높이
    glm::mat4 rotation = AirplaneRotation();
    glm::vec3 halfSize = BOX_SIZE * 0.5f;
    glm::vec3 extent = glm::abs(glm::vec3(rotation[0])) * halfSize.x
        + glm::abs(glm::vec3(rotation[1])) * halfSize.y
        + glm::abs(glm::vec3(rotation[2])) * halfSize.z;
    const float minX = -FLIGHT_HALF_WIDTH + extent.x;
    const float maxX = FLIGHT_HALF_WIDTH - extent.x;
    airplanePosition.x = std::clamp(airplanePosition.x, minX, maxX);

    glm::vec3 travel = GetAirplaneForward() * flightSpeed * deltaTime;
    const float nextX = airplanePosition.x + travel.x;
    if ((travel.x > 0.0f && nextX >= maxX) || (travel.x < 0.0f && nextX <= minX)) {
        const float wallX = travel.x > 0.0f ? maxX : minX;
        travel *= (wallX - airplanePosition.x) / travel.x;
        flightSpeed = 0.0f;
    }
    airplanePosition += travel;
    airplanePosition.x = std::clamp(airplanePosition.x, minX, maxX);
    const float climb = (static_cast<int>(ascend) - static_cast<int>(descend)) * ALTITUDE_SPEED * deltaTime;
    airplanePosition.y = std::max(GROUND_Y + extent.y, airplanePosition.y + climb);
}

glm::vec3 GetAirplanePosition() {
    return airplanePosition;
}

glm::vec3 GetAirplaneForward() {
    float headingRadians = glm::radians(headingAngle);
    return glm::vec3(std::sin(headingRadians), 0.0f, std::cos(headingRadians));
}

glm::vec3 GetAirplaneUp() {
    float headingRadians = glm::radians(headingAngle);
    float rollRadians = glm::radians(rollAngle);
    return glm::vec3(
        -std::sin(rollRadians) * std::cos(headingRadians),
        std::cos(rollRadians),
        std::sin(rollRadians) * std::sin(headingRadians));
}
