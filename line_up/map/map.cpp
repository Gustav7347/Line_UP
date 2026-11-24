#include "map.h"
#include <cmath>
#include <cstddef>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <random>
#include <vector>

struct MapVertex {
    glm::vec3 position;
    glm::vec3 color;
};

static GLuint mapVAO, mapVBO;
static GLsizei triangleVertexCount, wallVertexCount, skyVertexCount;
static const glm::vec3 SUN_POSITION(440.0f, 295.0f, DESTINATION_Z + 300.0f);

static void AddStrip(std::vector<MapVertex>& vertices, float minX, float maxX,
    float minZ, float maxZ, float y, const glm::vec3& color) {
    const MapVertex a{{minX, y, minZ}, color};
    const MapVertex b{{minX, y, maxZ}, color};
    const MapVertex c{{maxX, y, maxZ}, color};
    const MapVertex d{{maxX, y, minZ}, color};
    vertices.insert(vertices.end(), {a, b, c, a, c, d});
}

static void AddSun(std::vector<MapVertex>& vertices) {
    constexpr int segments = 48;
    const glm::vec3 skyColor(SKY_R, SKY_G, SKY_B);
    const glm::vec3 sunColor(1.0f, 0.94f, 0.64f);
    const glm::vec3 rimColor(1.0f, 0.83f, 0.40f);
    for (int i = 0; i < segments; ++i) {
        const float a = glm::two_pi<float>() * i / segments;
        const float b = glm::two_pi<float>() * (i + 1) / segments;
        const glm::vec3 dirA(std::cos(a), std::sin(a), 0.0f);
        const glm::vec3 dirB(std::cos(b), std::sin(b), 0.0f);
        const MapVertex innerA{SUN_POSITION + dirA * 42.0f, rimColor};
        const MapVertex innerB{SUN_POSITION + dirB * 42.0f, rimColor};
        const MapVertex outerA{SUN_POSITION + dirA * 115.0f, skyColor};
        const MapVertex outerB{SUN_POSITION + dirB * 115.0f, skyColor};
        vertices.insert(vertices.end(), {
            {SUN_POSITION, sunColor}, innerB, innerA,
            innerA, innerB, outerA, innerB, outerB, outerA
        });
    }
}

static void AddCloudPuff(std::vector<MapVertex>& vertices, const glm::vec3& center,
    const glm::vec3& size) {
    constexpr int rings = 8;
    constexpr int sectors = 12;
    const glm::vec3 lightDirection = glm::normalize(SUN_POSITION - center);
    const glm::vec3 shadow(0.75f, 0.81f, 0.89f);
    const glm::vec3 white(1.0f, 0.99f, 0.96f);
    auto vertex = [&](int ring, int sector) {
        const float latitude = glm::pi<float>() * ring / rings;
        const float longitude = glm::two_pi<float>() * sector / sectors;
        const glm::vec3 direction(std::sin(latitude) * std::cos(longitude),
            std::cos(latitude), std::sin(latitude) * std::sin(longitude));
        const float light = glm::clamp(0.58f + 0.22f * direction.y
            + 0.20f * glm::dot(direction, lightDirection), 0.0f, 1.0f);
        return MapVertex{center + direction * size, glm::mix(shadow, white, light)};
    };
    for (int ring = 0; ring < rings; ++ring) {
        for (int sector = 0; sector < sectors; ++sector) {
            const MapVertex a = vertex(ring, sector);
            const MapVertex b = vertex(ring + 1, sector);
            const MapVertex c = vertex(ring + 1, sector + 1);
            const MapVertex d = vertex(ring, sector + 1);
            vertices.insert(vertices.end(), {a, b, c, a, c, d});
        }
    }
}

static void AddClouds(std::vector<MapVertex>& vertices) {
    const glm::vec3 margin(90.0f, 26.0f, 140.0f);
    const glm::vec3 low = CLOUD_REGION_MIN + margin;
    const glm::vec3 high = CLOUD_REGION_MAX - margin;
    std::mt19937 random(7347);
    std::uniform_real_distribution<float> fraction(0.15f, 0.85f);
    // 구역마다 중심 위치를 조금씩 바꾸고 시작할 때 한 번만 만든다.
    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 4; ++col) {
            const glm::vec3 factor((col + fraction(random)) / 4.0f, fraction(random),
                (row + fraction(random)) / 6.0f);
            const glm::vec3 center = glm::mix(low, high, factor);
            AddCloudPuff(vertices, center, {43.0f, 17.0f, 32.0f});
            AddCloudPuff(vertices, center + glm::vec3(-36.0f, -5.0f, 4.0f), {33.0f, 12.0f, 26.0f});
            AddCloudPuff(vertices, center + glm::vec3(31.0f, -4.0f, 9.0f), {29.0f, 12.0f, 25.0f});
            AddCloudPuff(vertices, center + glm::vec3(8.0f, 7.0f, -12.0f), {26.0f, 18.0f, 23.0f});
        }
    }
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
    AddSun(vertices);
    AddClouds(vertices);
    skyVertexCount = static_cast<GLsizei>(vertices.size()) - triangleVertexCount - wallVertexCount;

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

void DrawSky(const glm::mat4& viewProj, GLuint transformLoc) {
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(viewProj));
    glBindVertexArray(mapVAO);
    glDrawArrays(GL_TRIANGLES, triangleVertexCount + wallVertexCount, skyVertexCount);
}
