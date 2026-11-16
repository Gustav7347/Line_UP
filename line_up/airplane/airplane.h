#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

// 비행기 관련 함수
void InitAirplane();                                            // VAO/VBO 생성
void DrawAirplane(const glm::mat4& viewProj, GLuint transformLoc); // 그리기
void StartAirplaneEngine();
void UpdateAirplane(float deltaTime, bool ascend, bool descend,
    bool turnLeft, bool turnRight, bool rollLeft, bool rollRight);
glm::vec3 GetAirplanePosition();
glm::vec3 GetAirplaneForward();
glm::vec3 GetAirplaneUp();
