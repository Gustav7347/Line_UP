#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

inline constexpr float GROUND_Y = 0.0f;
inline constexpr float FLIGHT_HALF_WIDTH = 500.0f;
inline constexpr float RUNWAY_START_Z = 0.0f;
inline constexpr float DESTINATION_Z = 2000.0f;

inline const glm::vec3 CLOUD_REGION_MIN(-FLIGHT_HALF_WIDTH, 100.0f, RUNWAY_START_Z);
inline const glm::vec3 CLOUD_REGION_MAX(FLIGHT_HALF_WIDTH, 200.0f, DESTINATION_Z);
inline constexpr float SKY_R = 0.5f, SKY_G = 0.7f, SKY_B = 0.9f;

void InitMap();
void DrawMap(const glm::mat4& viewProj, GLuint transformLoc);
void DrawSky(const glm::mat4& viewProj, GLuint transformLoc);
