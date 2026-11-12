#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

// 비행기 관련 함수
void InitAirplane();                                            // VAO/VBO 생성
void DrawAirplane(const glm::mat4& viewProj, GLuint transformLoc); // 그리기
