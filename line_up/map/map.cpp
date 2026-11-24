#include "map.h"
#include <cstddef>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

struct MapVertex {
    glm::vec3 position;
    glm::vec3 color;
};

static GLuint mapVAO, mapVBO;
static GLsizei triangleVertexCount, wallVertexCount;

static void AddStrip(std::vector<MapVertex>& vertices, float minX, float maxX,
    float minZ, float maxZ, float y, const glm::vec3& color) {
    const MapVertex a{{minX, y, minZ}, color};
    const MapVertex b{{minX, y, maxZ}, color};
    const MapVertex c{{maxX, y, maxZ}, color};
    const MapVertex d{{maxX, y, minZ}, color};
    vertices.insert(vertices.end(), {a, b, c, a, c, d});
}

void InitMap() {
    std::vector<MapVertex> vertices;
    constexpr float runwayHalfWidth = 14.0f;
    constexpr float markingY = GROUND_Y + 0.04f;
    const glm::vec3 asphalt(0.20f, 0.22f, 0.25f);
    const glm::vec3 white(0.95f, 0.95f, 0.93f);
    const glm::vec3 destination(0.95f, 0.68f, 0.18f);

    AddStrip(vertices, -runwayHalfWidth, runwayHalfWidth, RUNWAY_START_Z, DESTINATION_Z, GROUND_Y, asphalt);
    AddStrip(vertices, -13.0f, -12.5f, RUNWAY_START_Z, DESTINATION_Z, markingY, white);
    AddStrip(vertices, 12.5f, 13.0f, RUNWAY_START_Z, DESTINATION_Z, markingY, white);
    for (float z = RUNWAY_START_Z + 8.0f; z < DESTINATION_Z - 24.0f; z += 20.0f) {
        AddStrip(vertices, -0.35f, 0.35f, z, z + 8.0f, markingY, white);
    }
    AddStrip(vertices, -12.0f, 12.0f, DESTINATION_Z - 24.0f, DESTINATION_Z, markingY, destination);
    triangleVertexCount = static_cast<GLsizei>(vertices.size());

    // 비행 범위의 좌우 경계를 활주로 구간에 선으로 표시한다.
    const glm::vec3 wallColor(0.72f, 0.37f, 0.16f);
    constexpr float wallGuideHeight = 240.0f;
    for (float x : {-FLIGHT_HALF_WIDTH, FLIGHT_HALF_WIDTH}) {
        for (float y : {0.0f, 80.0f, 160.0f, wallGuideHeight}) {
            vertices.push_back({{x, y, RUNWAY_START_Z}, wallColor});
            vertices.push_back({{x, y, DESTINATION_Z}, wallColor});
        }
        for (float z = RUNWAY_START_Z; z <= DESTINATION_Z; z += 100.0f) {
            vertices.push_back({{x, GROUND_Y, z}, wallColor});
            vertices.push_back({{x, wallGuideHeight, z}, wallColor});
        }
    }
    wallVertexCount = static_cast<GLsizei>(vertices.size()) - triangleVertexCount;

    glGenVertexArrays(1, &mapVAO);
    glBindVertexArray(mapVAO);
    glGenBuffers(1, &mapVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mapVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(MapVertex), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MapVertex), reinterpret_cast<const void*>(offsetof(MapVertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MapVertex), reinterpret_cast<const void*>(offsetof(MapVertex, color)));
    glEnableVertexAttribArray(1);
}

void DrawMap(const glm::mat4& viewProj, GLuint transformLoc) {
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(viewProj));
    glBindVertexArray(mapVAO);
    glDrawArrays(GL_TRIANGLES, 0, triangleVertexCount);
    glDrawArrays(GL_LINES, triangleVertexCount, wallVertexCount);
}
