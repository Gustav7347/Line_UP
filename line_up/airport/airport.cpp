#include "airport.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 이 파일 안에서만 쓰는 변수 (static)
static GLuint cubeVAO, cubeVBO; // 위치만 (색은 그릴 때마다 glVertexAttrib3f로 지정)

// ===== 배치 기준 =====
// 공항 기준점(START_AIRPORT_POS) = 활주로 시작 끝 중앙, 활주로는 +Z 로 뻗음
// 활주로 오른쪽(+X)에 유도로 → 계류장 → 터미널 순서로 배치
//
//   -X  잔디 | 활주로 | 잔디 | 유도로 | 계류장(비행기 주기) | 터미널   +X
//
// 단위: 비행기 박스(2 x 1 x 3) 기준

// --- 크기 ---
static const float RUNWAY_WIDTH = 30.0f;   // 활주로 폭 (X)
static const float RUNWAY_LENGTH = 400.0f;  // 활주로 길이 (Z)
static const float SHOULDER = 3.0f;    // 활주로 양옆 갓길 폭
static const float GROUND_SIZE = 700.0f;  // 잔디 바닥 한 변 (활주로 가운데 기준, 앞뒤로 150씩 여유)

static const float TAXI_X = 45.0f;  // 평행 유도로 중심 X
static const float TAXI_WIDTH = 14.0f;  // 유도로 폭
static const float APRON_X0 = 52.0f, APRON_X1 = 135.0f; // 계류장 X 범위
static const float APRON_Z0 = 60.0f, APRON_Z1 = 370.0f; // 계류장 Z 범위

// 유도로 ↔ 활주로 연결로 위치 (Z)
static const float CONNECTOR_Z[3] = { 30.0f, 200.0f, 370.0f };

// --- 높이: 겹치는 면이 깜빡이지 않게(z-fighting) 조금씩 다르게 ---
static const float GROUND_TOP = -0.3f;  // 잔디 윗면
static const float SHOULDER_TOP = -0.1f;  // 갓길 윗면
static const float PAVE_TOP = 0.0f;   // 활주로/유도로/계류장 윗면
static const float MARK_TOP = 0.08f;  // 표시선 윗면
static const float MARK_H = 0.1f;   // 표시선 두께
static const float SLAB = 1.0f;   // 바닥판 두께 (얇으면 멀리서 윗면/아랫면이 깜빡일 수 있어 두껍게, 아래는 잔디에 묻힘)

// --- 색상 ---
static const glm::vec3 COLOR_GROUND = glm::vec3(0.30f, 0.55f, 0.25f); // 잔디
static const glm::vec3 COLOR_RUNWAY = glm::vec3(0.28f, 0.28f, 0.30f); // 활주로 아스팔트
static const glm::vec3 COLOR_SHOULDER = glm::vec3(0.45f, 0.45f, 0.42f); // 갓길
static const glm::vec3 COLOR_TAXIWAY = glm::vec3(0.36f, 0.36f, 0.38f); // 유도로
static const glm::vec3 COLOR_APRON = glm::vec3(0.62f, 0.62f, 0.60f); // 계류장(콘크리트)
static const glm::vec3 COLOR_WHITE = glm::vec3(0.95f, 0.95f, 0.95f); // 활주로 표시선
static const glm::vec3 COLOR_YELLOW = glm::vec3(0.95f, 0.80f, 0.10f); // 유도로 표시선
static const glm::vec3 COLOR_TOWER = glm::vec3(0.85f, 0.85f, 0.82f); // 관제탑 기둥
static const glm::vec3 COLOR_GLASS = glm::vec3(0.35f, 0.65f, 0.85f); // 유리
static const glm::vec3 COLOR_ROOF = glm::vec3(0.25f, 0.25f, 0.28f); // 지붕/창틀
static const glm::vec3 COLOR_TERMINAL = glm::vec3(0.90f, 0.86f, 0.76f); // 터미널 벽
static const glm::vec3 COLOR_BRIDGE = glm::vec3(0.75f, 0.75f, 0.78f); // 탑승교
static const glm::vec3 COLOR_HANGAR = glm::vec3(0.55f, 0.60f, 0.62f); // 격납고
static const glm::vec3 COLOR_DOOR = glm::vec3(0.32f, 0.35f, 0.38f); // 격납고 문
static const glm::vec3 COLOR_TANK = glm::vec3(0.92f, 0.92f, 0.90f); // 연료 탱크
static const glm::vec3 COLOR_PLANE = glm::vec3(0.96f, 0.96f, 0.98f); // 주기된 여객기 동체
static const glm::vec3 COLOR_PLANE_TAIL = glm::vec3(0.15f, 0.35f, 0.75f); // 여객기 꼬리 날개
static const glm::vec3 COLOR_ENGINE = glm::vec3(0.55f, 0.55f, 0.60f); // 엔진
static const glm::vec3 COLOR_ORANGE = glm::vec3(1.00f, 0.50f, 0.10f); // 풍향계
static const glm::vec3 COLOR_POLE = glm::vec3(0.60f, 0.60f, 0.60f); // 기둥
static const glm::vec3 COLOR_LIGHT = glm::vec3(1.00f, 0.90f, 0.50f); // 활주로 가장자리등
static const glm::vec3 COLOR_LIGHT_START = glm::vec3(0.20f, 1.00f, 0.30f); // 활주로 시작 (초록)
static const glm::vec3 COLOR_LIGHT_END = glm::vec3(1.00f, 0.20f, 0.20f); // 활주로 끝 (빨강)
static const glm::vec3 COLOR_LIGHT_TAXI = glm::vec3(0.20f, 0.40f, 1.00f); // 유도로 등 (파랑)
static const glm::vec3 COLOR_LIGHT_APPR = glm::vec3(1.00f, 1.00f, 0.85f); // 진입등 (흰색)

// 단위 육면체 (-0.5 ~ 0.5), 면당 삼각형 2개 = 6정점, 총 36정점 (airplane.cpp와 같은 순서)
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

// 조명이 아직 없으므로 면마다 밝기를 다르게 곱해서 입체감을 냄 (Front, Back, Top, Bottom, Right, Left)
static const float faceShade[6] = { 0.85f, 0.70f, 1.00f, 0.50f, 0.80f, 0.65f };

void InitAirport() {
    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // 색상(location = 1)은 배열을 쓰지 않음 → glVertexAttrib3f 값이 모든 정점에 적용됨
    // 덕분에 셰이더를 바꾸지 않고 상자마다 색을 다르게 줄 수 있음
    glDisableVertexAttribArray(1);
}

// ===== 그리기 도우미 =====
// 매번 viewProj, transformLoc, 기준점을 넘기지 않도록 이 파일 안에서만 보관
static glm::mat4 gViewProj;
static GLuint    gLoc;
static glm::vec3 gBase;

// 상자 하나 그리기 (공통)
//  parentMVP : viewProj * (상자들이 놓일 좌표계의 모델 행렬)
//  bottom    : 그 좌표계에서 상자 "바닥 중앙" 위치
//  size      : 상자 크기 (X, Y, Z)
static void partBox(const glm::mat4& parentMVP, glm::vec3 bottom, glm::vec3 size, glm::vec3 color) {
    glm::vec3 center = bottom + glm::vec3(0.0f, size.y * 0.5f, 0.0f);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), center);
    model = glm::scale(model, size);
    glm::mat4 mvp = parentMVP * model;
    glUniformMatrix4fv(gLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    for (int f = 0; f < 6; ++f) {
        glm::vec3 c = color * faceShade[f];
        glVertexAttrib3f(1, c.r, c.g, c.b);
        glDrawArrays(GL_TRIANGLES, f * 6, 6);
    }
}

// 공항 좌표계(기준점 gBase)에 상자 하나 그리기
static void box(glm::vec3 bottom, glm::vec3 size, glm::vec3 color) {
    partBox(gViewProj, gBase + bottom, size, color);
}

// 바닥 표시선 (위에서 본 사각형). x, z = 중심, w = X 폭, l = Z 길이
static void mark(float x, float z, float w, float l, glm::vec3 color) {
    box(glm::vec3(x, MARK_TOP - MARK_H, z), glm::vec3(w, MARK_H, l), color);
}

// 작은 등 하나
static void lamp(float x, float z, float floorY, glm::vec3 color) {
    box(glm::vec3(x, floorY, z), glm::vec3(0.5f, 0.5f, 0.5f), color);
}

// ===== 활주로 번호 (7세그먼트 숫자) =====
//        a
//      f   b
//        g
//      e   c
//        d
static const unsigned char DIGIT_SEGMENTS[10] = {
    // gfedcba 비트
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

// 숫자 하나를 활주로 바닥에 그림
//  (cx, cz)   : 숫자 중심
//  rightSign  : 읽는 사람 기준 오른쪽이 +X면 +1, -X면 -1
//  upSign     : 숫자 위쪽이 +Z면 +1, -Z면 -1
static void digit(int d, float cx, float cz, float rightSign, float upSign) {
    const float w = 5.0f, h = 9.0f, t = 1.0f; // 숫자 폭, 높이, 획 두께
    struct Seg { float u, v, sw, sh; };       // 중심(u: 오른쪽, v: 위쪽), 크기
    const Seg segs[7] = {
        { 0.0f,            h / 2 - t / 2, w, t }, // a
        { w / 2 - t / 2,   h / 4,         t, h / 2 }, // b
        { w / 2 - t / 2,  -h / 4,         t, h / 2 }, // c
        { 0.0f,           -h / 2 + t / 2, w, t }, // d
        { -w / 2 + t / 2,  -h / 4,         t, h / 2 }, // e
        { -w / 2 + t / 2,   h / 4,         t, h / 2 }, // f
        { 0.0f,            0.0f,          w, t }  // g
    };
    for (int i = 0; i < 7; ++i) {
        if (!(DIGIT_SEGMENTS[d] & (1 << i))) continue;
        mark(cx + segs[i].u * rightSign, cz + segs[i].v * upSign, segs[i].sw, segs[i].sh, COLOR_WHITE);
    }
}

// ===== 활주로 =====
// 한쪽 끝의 표시선 (시작 끝과 반대쪽 끝에 똑같이 그림)
//  zAt(d) : 활주로 끝에서 d 만큼 안쪽의 Z 좌표
static void runwayEndMarkings(float endZ, float inward, int num1, int num2) {
    auto zAt = [&](float d) { return endZ + d * inward; };
    const float halfW = RUNWAY_WIDTH * 0.5f;

    // 시작 표시 (피아노 건반 모양 줄무늬 8개)
    for (int i = 0; i < 8; ++i) {
        float x = -halfW + 3.5f + i * ((RUNWAY_WIDTH - 7.0f) / 7.0f);
        mark(x, zAt(15.0f), 1.8f, 20.0f, COLOR_WHITE);
    }

    // 활주로 번호 (비행기가 들어오는 방향에서 읽힘)
    // 진입 방향 = inward. 그 방향을 보고 섰을 때 오른쪽 = -X * inward
    float rightSign = -inward;
    digit(num1, -3.5f * rightSign, zAt(37.0f), rightSign, inward);
    digit(num2, 3.5f * rightSign, zAt(37.0f), rightSign, inward);

    // 접지 구역 표시 (짧은 줄 3개씩 양쪽)
    for (int k = 0; k < 3; ++k) {
        float x = 5.0f + k * 1.8f;
        mark(x, zAt(65.0f), 1.0f, 12.0f, COLOR_WHITE);
        mark(-x, zAt(65.0f), 1.0f, 12.0f, COLOR_WHITE);
    }
    // 조준점 표시 (굵고 긴 사각형 2개)
    mark(7.0f, zAt(100.0f), 3.0f, 25.0f, COLOR_WHITE);
    mark(-7.0f, zAt(100.0f), 3.0f, 25.0f, COLOR_WHITE);
    // 접지 구역 표시 (짧은 줄 2개씩 양쪽)
    for (int k = 0; k < 2; ++k) {
        float x = 5.0f + k * 1.8f;
        mark(x, zAt(140.0f), 1.0f, 12.0f, COLOR_WHITE);
        mark(-x, zAt(140.0f), 1.0f, 12.0f, COLOR_WHITE);
    }
}

static void drawRunway() {
    const float halfW = RUNWAY_WIDTH * 0.5f;
    const float midZ = RUNWAY_LENGTH * 0.5f;

    // 아스팔트 + 양옆 갓길
    box(glm::vec3(0.0f, PAVE_TOP - SLAB, midZ), glm::vec3(RUNWAY_WIDTH, SLAB, RUNWAY_LENGTH), COLOR_RUNWAY);
    box(glm::vec3(-halfW - SHOULDER * 0.5f, SHOULDER_TOP - SLAB, midZ), glm::vec3(SHOULDER, SLAB, RUNWAY_LENGTH), COLOR_SHOULDER);
    box(glm::vec3(halfW + SHOULDER * 0.5f, SHOULDER_TOP - SLAB, midZ), glm::vec3(SHOULDER, SLAB, RUNWAY_LENGTH), COLOR_SHOULDER);

    // 양쪽 가장자리 실선
    mark(-halfW + 1.0f, midZ, 0.6f, RUNWAY_LENGTH - 4.0f, COLOR_WHITE);
    mark(halfW - 1.0f, midZ, 0.6f, RUNWAY_LENGTH - 4.0f, COLOR_WHITE);

    // 중앙 점선 (길이 12, 간격 10)  번호 표시가 끝나는 곳부터
    for (float z = 50.0f; z + 12.0f < RUNWAY_LENGTH - 50.0f; z += 22.0f)
        mark(0.0f, z + 6.0f, 0.8f, 12.0f, COLOR_WHITE);

    // 양 끝 표시: 시작 끝은 "36" (북쪽으로 이륙), 반대쪽 끝은 "18"
    runwayEndMarkings(0.0f, 1.0f, 3, 6);
    runwayEndMarkings(RUNWAY_LENGTH, -1.0f, 1, 8);

    // 가장자리 등 (20 간격, 시작 = 초록, 끝 = 빨강)
    float lightX = halfW + SHOULDER - 0.5f;
    for (float z = 0.0f; z <= RUNWAY_LENGTH; z += 20.0f) {
        glm::vec3 c = COLOR_LIGHT;
        if (z == 0.0f) c = COLOR_LIGHT_START;
        else if (z + 20.0f > RUNWAY_LENGTH) c = COLOR_LIGHT_END;
        lamp(-lightX, z, SHOULDER_TOP, c);
        lamp(lightX, z, SHOULDER_TOP, c);
    }
    // 시작/끝 가로줄 등
    for (float x = -halfW + 2.0f; x <= halfW - 2.0f; x += 4.0f) {
        lamp(x, -1.0f, GROUND_TOP, COLOR_LIGHT_START);
        lamp(x, RUNWAY_LENGTH + 1.0f, GROUND_TOP, COLOR_LIGHT_END);
    }

    // 진입등: 활주로 앞쪽(-Z) 잔디 위에 일렬로, 중간중간 가로 막대
    for (float d = 15.0f; d <= 90.0f; d += 15.0f) {
        box(glm::vec3(0.0f, GROUND_TOP, -d), glm::vec3(0.3f, 1.0f, 0.3f), COLOR_POLE);
        lamp(0.0f, -d, GROUND_TOP + 1.0f, COLOR_LIGHT_APPR);
        if (d == 30.0f || d == 60.0f) {
            for (float x = -6.0f; x <= 6.0f; x += 2.0f) {
                if (x == 0.0f) continue;
                box(glm::vec3(x, GROUND_TOP, -d), glm::vec3(0.3f, 1.0f, 0.3f), COLOR_POLE);
                lamp(x, -d, GROUND_TOP + 1.0f, COLOR_LIGHT_APPR);
            }
        }
    }
}

// ===== 유도로 =====
static void drawTaxiways() {
    const float halfW = RUNWAY_WIDTH * 0.5f;
    const float conX0 = halfW + SHOULDER;          // 연결로 시작 (갓길 바깥)
    const float conX1 = TAXI_X - TAXI_WIDTH * 0.5f; // 연결로 끝 (유도로 왼쪽 가장자리)
    const float taxiZ0 = CONNECTOR_Z[0] - TAXI_WIDTH * 0.5f;
    const float taxiZ1 = CONNECTOR_Z[2] + TAXI_WIDTH * 0.5f;

    // 평행 유도로
    box(glm::vec3(TAXI_X, PAVE_TOP - SLAB, (taxiZ0 + taxiZ1) * 0.5f),
        glm::vec3(TAXI_WIDTH, SLAB, taxiZ1 - taxiZ0), COLOR_TAXIWAY);
    // 노란 중심선
    mark(TAXI_X, (CONNECTOR_Z[0] + CONNECTOR_Z[2]) * 0.5f, 0.5f, CONNECTOR_Z[2] - CONNECTOR_Z[0], COLOR_YELLOW);

    for (int i = 0; i < 3; ++i) {
        float cz = CONNECTOR_Z[i];
        // 연결로
        box(glm::vec3((conX0 + conX1) * 0.5f, PAVE_TOP - SLAB, cz),
            glm::vec3(conX1 - conX0, SLAB, TAXI_WIDTH), COLOR_TAXIWAY);
        // 노란 중심선 (활주로 가장자리부터 유도로 중심까지)
        mark((halfW + 0.1f + TAXI_X) * 0.5f, cz, TAXI_X - (halfW + 0.1f), 0.5f, COLOR_YELLOW);

        // 활주로 진입 전 정지선 (실선 2줄 + 점선 2줄)
        float hx = conX0 + 4.0f;
        mark(hx, cz, 0.4f, TAXI_WIDTH - 1.0f, COLOR_YELLOW);
        mark(hx + 0.8f, cz, 0.4f, TAXI_WIDTH - 1.0f, COLOR_YELLOW);
        for (float z = -TAXI_WIDTH * 0.5f + 1.0f; z < TAXI_WIDTH * 0.5f - 1.0f; z += 2.0f) {
            mark(hx + 1.8f, cz + z + 0.5f, 0.4f, 1.0f, COLOR_YELLOW);
            mark(hx + 2.6f, cz + z + 0.5f, 0.4f, 1.0f, COLOR_YELLOW);
        }

        // 연결로 양옆 파란 등
        for (float x = conX0 + 2.0f; x < conX1; x += 6.0f) {
            lamp(x, cz - TAXI_WIDTH * 0.5f - 0.6f, GROUND_TOP, COLOR_LIGHT_TAXI);
            lamp(x, cz + TAXI_WIDTH * 0.5f + 0.6f, GROUND_TOP, COLOR_LIGHT_TAXI);
        }
    }

    // 평행 유도로 왼쪽 파란 등 (연결로 자리는 비움)
    for (float z = taxiZ0; z <= taxiZ1; z += 15.0f) {
        bool onConnector = false;
        for (int i = 0; i < 3; ++i)
            if (z > CONNECTOR_Z[i] - TAXI_WIDTH * 0.5f - 1.0f && z < CONNECTOR_Z[i] + TAXI_WIDTH * 0.5f + 1.0f)
                onConnector = true;
        if (!onConnector) lamp(conX1 - 0.6f, z, GROUND_TOP, COLOR_LIGHT_TAXI);
    }
}

// ===== 여객기 모델 =====
// 비행기 자신의 좌표계: 기수 = +Z, 위 = +Y, 바퀴 바닥 중앙 = 원점
// 길이 30 (Z -15 ~ 15), 날개 폭 30, 높이 약 10.7
// 공항에 서 있는 비행기와 플레이어 비행기가 같은 모양을 씀
static void airliner(const glm::mat4& mvp) {
    const float gearH = 1.2f; // 바퀴 높이

    // 바퀴 (앞 1개, 뒤 2개)
    partBox(mvp, glm::vec3(0.0f, 0.0f, 12.0f), glm::vec3(0.6f, gearH, 0.6f), COLOR_ROOF);
    partBox(mvp, glm::vec3(-2.5f, 0.0f, -1.0f), glm::vec3(0.8f, gearH, 1.0f), COLOR_ROOF);
    partBox(mvp, glm::vec3(2.5f, 0.0f, -1.0f), glm::vec3(0.8f, gearH, 1.0f), COLOR_ROOF);
    // 동체 + 기수 + 조종석 창
    partBox(mvp, glm::vec3(0.0f, gearH, -1.0f), glm::vec3(3.5f, 3.5f, 28.0f), COLOR_PLANE);
    partBox(mvp, glm::vec3(0.0f, gearH + 0.4f, 14.0f), glm::vec3(2.8f, 2.6f, 2.0f), COLOR_PLANE);
    partBox(mvp, glm::vec3(0.0f, gearH + 2.2f, 13.9f), glm::vec3(3.0f, 1.0f, 1.2f), COLOR_ROOF);
    // 주 날개
    partBox(mvp, glm::vec3(0.0f, gearH + 0.8f, -1.0f), glm::vec3(30.0f, 0.5f, 6.0f), COLOR_PLANE);
    // 엔진 (날개 아래)
    partBox(mvp, glm::vec3(-6.5f, gearH - 0.6f, 1.5f), glm::vec3(1.6f, 1.6f, 4.0f), COLOR_ENGINE);
    partBox(mvp, glm::vec3(6.5f, gearH - 0.6f, 1.5f), glm::vec3(1.6f, 1.6f, 4.0f), COLOR_ENGINE);
    // 꼬리: 수직 꼬리날개 + 수평 꼬리날개
    partBox(mvp, glm::vec3(0.0f, gearH + 3.5f, -12.5f), glm::vec3(0.5f, 6.0f, 4.0f), COLOR_PLANE_TAIL);
    partBox(mvp, glm::vec3(0.0f, gearH + 2.5f, -12.5f), glm::vec3(11.0f, 0.4f, 3.0f), COLOR_PLANE);
}

// 주기된 여객기 (장식, 코가 +X 쪽 터미널을 향함)
static void parkedPlane(float noseX, float z) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), gBase + glm::vec3(noseX - 15.0f, PAVE_TOP, z));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // 기수 +Z → +X
    airliner(gViewProj * model);
}

// 플레이어 비행기용 (airplane.cpp 에서 호출)
void DrawAirlinerModel(const glm::mat4& mvp, GLuint transformLoc, float length) {
    glBindVertexArray(cubeVAO);
    gLoc = transformLoc;
    float s = length / 30.0f; // 모델 원래 길이 30 기준으로 축소/확대
    airliner(mvp * glm::scale(glm::mat4(1.0f), glm::vec3(s)));
}

// ===== 계류장 + 터미널 =====
static void drawTerminalArea() {
    // 계류장 바닥
    box(glm::vec3((APRON_X0 + APRON_X1) * 0.5f, PAVE_TOP - SLAB, (APRON_Z0 + APRON_Z1) * 0.5f),
        glm::vec3(APRON_X1 - APRON_X0, SLAB, APRON_Z1 - APRON_Z0), COLOR_APRON);

    // 터미널 본관
    const float tx0 = APRON_X1, tx1 = APRON_X1 + 25.0f; // X 범위
    const float tz0 = 100.0f, tz1 = 260.0f;           // Z 범위
    const float tcx = (tx0 + tx1) * 0.5f, tcz = (tz0 + tz1) * 0.5f;
    box(glm::vec3(tcx, GROUND_TOP, tcz), glm::vec3(tx1 - tx0, 12.0f - GROUND_TOP, tz1 - tz0), COLOR_TERMINAL);
    // 계류장 쪽 유리벽 (띠 모양, 벽보다 살짝 앞으로)
    box(glm::vec3(tx0 - 0.15f, 4.0f, tcz), glm::vec3(0.3f, 5.0f, tz1 - tz0 - 4.0f), COLOR_GLASS);
    // 지붕 (처마가 살짝 나오게)
    box(glm::vec3(tcx - 1.0f, 12.0f, tcz), glm::vec3(tx1 - tx0 + 4.0f, 1.0f, tz1 - tz0 + 2.0f), COLOR_ROOF);
    // 가운데 높은 홀
    box(glm::vec3(tcx + 2.0f, 13.0f, tcz), glm::vec3(16.0f, 6.0f, 40.0f), COLOR_TERMINAL);
    box(glm::vec3(tcx + 2.0f - 8.0f - 0.15f, 14.0f, tcz), glm::vec3(0.3f, 4.0f, 36.0f), COLOR_GLASS);
    box(glm::vec3(tcx + 2.0f, 19.0f, tcz), glm::vec3(17.0f, 0.8f, 41.0f), COLOR_ROOF);

    // 게이트 3개: 탑승교 + 주기 유도선 + 비행기
    const float gateZ[3] = { 130.0f, 180.0f, 230.0f };
    for (int i = 0; i < 3; ++i) {
        float gz = gateZ[i];
        // 탑승교 (터미널에서 -X 로 뻗어 나옴, 기둥 위에 얹힘)
        box(glm::vec3(tx0 - 9.0f, 3.0f, gz + 4.0f), glm::vec3(18.0f, 3.0f, 3.0f), COLOR_BRIDGE);
        box(glm::vec3(tx0 - 15.0f, PAVE_TOP, gz + 4.0f), glm::vec3(1.0f, 3.0f, 1.0f), COLOR_POLE);
        // 노란 주기 유도선 (유도로 쪽에서 게이트까지) + 정지선
        mark((APRON_X0 + tx0 - 20.0f) * 0.5f, gz, tx0 - 20.0f - APRON_X0, 0.4f, COLOR_YELLOW);
        mark(tx0 - 20.0f, gz, 0.4f, 6.0f, COLOR_YELLOW);
        // 비행기는 게이트 1, 3 에만 (가운데는 빈 자리)
        if (i != 1) parkedPlane(tx0 - 20.0f, gz);
    }

    // 계류장 가장자리 노란 선 (유도로 쪽)
    mark(APRON_X0 + 1.0f, (APRON_Z0 + APRON_Z1) * 0.5f, 0.4f, APRON_Z1 - APRON_Z0 - 2.0f, COLOR_YELLOW);
}

// ===== 관제탑 =====
static void drawTower() {
    const glm::vec3 p = glm::vec3(95.0f, 0.0f, 30.0f); // 계류장 남쪽 잔디 위
    // 아래 사무동
    box(p + glm::vec3(0.0f, GROUND_TOP, 0.0f), glm::vec3(18.0f, 6.0f - GROUND_TOP, 12.0f), COLOR_TERMINAL);
    box(p + glm::vec3(0.0f, 6.0f, 0.0f), glm::vec3(19.0f, 0.6f, 13.0f), COLOR_ROOF);
    // 기둥
    box(p + glm::vec3(0.0f, 6.6f, 0.0f), glm::vec3(5.0f, 30.0f, 5.0f), COLOR_TOWER);
    // 관제실: 아래 받침 → 유리 → 지붕 (위로 갈수록 넓게)
    box(p + glm::vec3(0.0f, 36.6f, 0.0f), glm::vec3(9.0f, 1.2f, 9.0f), COLOR_TOWER);
    box(p + glm::vec3(0.0f, 37.8f, 0.0f), glm::vec3(11.0f, 0.6f, 11.0f), COLOR_ROOF);
    box(p + glm::vec3(0.0f, 38.4f, 0.0f), glm::vec3(10.6f, 4.0f, 10.6f), COLOR_GLASS);
    box(p + glm::vec3(0.0f, 42.4f, 0.0f), glm::vec3(12.0f, 0.8f, 12.0f), COLOR_ROOF);
    // 안테나 + 빨간 경고등
    box(p + glm::vec3(2.5f, 43.2f, 2.5f), glm::vec3(0.4f, 6.0f, 0.4f), COLOR_POLE);
    box(p + glm::vec3(2.5f, 49.2f, 2.5f), glm::vec3(0.8f, 0.8f, 0.8f), COLOR_LIGHT_END);
    box(p + glm::vec3(-2.5f, 43.2f, -2.5f), glm::vec3(2.5f, 1.2f, 2.5f), COLOR_TOWER); // 레이더 받침
}

// ===== 격납고 + 연료 탱크 =====
static void hangar(float cx, float cz) {
    const float w = 30.0f, d = 34.0f; // X 폭, Z 깊이
    box(glm::vec3(cx, PAVE_TOP, cz), glm::vec3(w, 12.0f, d), COLOR_HANGAR);
    // 둥근 지붕 느낌 (층마다 좁아지게 3단)
    box(glm::vec3(cx, 12.0f, cz), glm::vec3(w - 4.0f, 2.0f, d), COLOR_HANGAR);
    box(glm::vec3(cx, 14.0f, cz), glm::vec3(w - 10.0f, 1.5f, d), COLOR_HANGAR);
    box(glm::vec3(cx, 15.5f, cz), glm::vec3(w - 18.0f, 1.0f, d), COLOR_HANGAR);
    // 유도로 쪽(-X) 큰 문 + 문 가운데 이음선
    box(glm::vec3(cx - w * 0.5f - 0.1f, PAVE_TOP, cz), glm::vec3(0.2f, 10.0f, d - 6.0f), COLOR_DOOR);
    box(glm::vec3(cx - w * 0.5f - 0.25f, PAVE_TOP, cz), glm::vec3(0.1f, 10.0f, 0.4f), COLOR_ROOF);
}

static void drawHangars() {
    hangar(85.0f, 305.0f);
    hangar(85.0f, 345.0f);

    // 연료 탱크 (터미널 뒤쪽 잔디 위)
    for (int i = 0; i < 3; ++i) {
        float z = 300.0f + i * 14.0f;
        box(glm::vec3(150.0f, GROUND_TOP, z), glm::vec3(9.0f, 7.0f, 9.0f), COLOR_TANK);
        box(glm::vec3(150.0f, GROUND_TOP + 7.0f, z), glm::vec3(7.0f, 0.8f, 7.0f), COLOR_TANK);
    }
}

// ===== 풍향계 (활주로 왼쪽 잔디) =====
static void drawWindsock() {
    const float x = -RUNWAY_WIDTH * 0.5f - 15.0f, z = 70.0f;
    box(glm::vec3(x, GROUND_TOP, z), glm::vec3(0.3f, 7.0f, 0.3f), COLOR_POLE);
    // 바람 방향(+Z)으로 뻗은 주황/흰 줄무늬 자루
    for (int i = 0; i < 4; ++i) {
        float s = 1.2f - i * 0.15f;
        box(glm::vec3(x, 6.0f - s * 0.5f + 0.1f, z + 0.9f + i * 1.0f), glm::vec3(s, s, 1.0f),
            (i % 2 == 0) ? COLOR_ORANGE : COLOR_WHITE);
    }
}

// 공항 하나를 기준점(base)에 그림
static void drawAirportAt(const glm::mat4& viewProj, GLuint loc, glm::vec3 base) {
    gViewProj = viewProj;
    gLoc = loc;
    gBase = base;

    // 잔디 바닥 (활주로 중심 기준으로 넓게)
    box(glm::vec3(0.0f, GROUND_TOP - 5.0f, RUNWAY_LENGTH * 0.5f),
        glm::vec3(GROUND_SIZE, 5.0f, GROUND_SIZE), COLOR_GROUND);

    drawRunway();
    drawTaxiways();
    drawTerminalArea();
    drawTower();
    drawHangars();
    drawWindsock();
}

// 공항 전체(잔디 바닥 + 관제탑 높이)가 화면 밖이면 true
// 공항을 감싸는 상자의 8개 꼭짓점이 모두 같은 화면 경계 바깥에 있으면 안 보이는 것으로 판단
static bool isOutsideView(const glm::mat4& viewProj, glm::vec3 base) {
    const float half = GROUND_SIZE * 0.5f;
    const glm::vec3 lo = base + glm::vec3(-half, GROUND_TOP - 5.0f, RUNWAY_LENGTH * 0.5f - half);
    const glm::vec3 hi = base + glm::vec3(half, 55.0f, RUNWAY_LENGTH * 0.5f + half);

    int outside[6] = { 0, 0, 0, 0, 0, 0 }; // -x, +x, -y, +y, near, far 쪽 바깥 개수
    for (int i = 0; i < 8; ++i) {
        glm::vec4 p = viewProj * glm::vec4((i & 1) ? hi.x : lo.x, (i & 2) ? hi.y : lo.y, (i & 4) ? hi.z : lo.z, 1.0f);
        if (p.x < -p.w) outside[0]++;
        if (p.x > p.w) outside[1]++;
        if (p.y < -p.w) outside[2]++;
        if (p.y > p.w) outside[3]++;
        if (p.z < -p.w) outside[4]++;
        if (p.z > p.w) outside[5]++;
    }
    for (int k = 0; k < 6; ++k)
        if (outside[k] == 8) return true;
    return false;
}

void DrawAirport(const glm::mat4& viewProj, GLuint transformLoc) {
    glBindVertexArray(cubeVAO);

    // 출발 공항과 목적지 공항은 똑같은 모양, 위치만 다름
    // 멀어서 안 보이는 공항은 건너뜀 (상자 수백 개라 그리는 양을 줄이기 위해)
    const glm::vec3 airports[2] = { START_AIRPORT_POS, DEST_AIRPORT_POS };
    for (int i = 0; i < 2; ++i) {
        if (isOutsideView(viewProj, airports[i])) continue;
        drawAirportAt(viewProj, transformLoc, airports[i]);
    }
}
