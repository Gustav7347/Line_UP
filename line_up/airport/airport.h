#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

// 공항 (출발지 + 목적지)
// 육면체 VAO 하나를 위치/크기/색만 바꿔 여러 번 그려서 공항을 만듦
//  - 활주로: 갓길, 시작 표시, 활주로 번호(36/18), 접지/조준점 표시, 중앙 점선, 가장자리등, 진입등
//  - 유도로: 평행 유도로 + 연결로 3개, 노란 중심선, 정지선, 파란 유도로등
//  - 계류장/터미널: 탑승교 3개, 주기 유도선, 주기된 여객기 2대
//  - 관제탑, 격납고 2동, 연료 탱크, 풍향계

// 출발 공항 기준점 = 활주로 시작 끝(중앙선 위), 활주로 윗면 높이
// 활주로는 이 점에서 +Z(비행기 앞쪽) 방향으로 뻗어 있음
// 비행기 박스 바닥(y = -0.5)이 활주로 위에 닿고, 원점의 비행기가
// 활주로 번호 "36" 바로 앞(시작점에서 48 들어간 곳)에 놓이도록 맞춤
const glm::vec3 START_AIRPORT_POS = glm::vec3(0.0f, -0.5f, -48.0f);

// 목적지 공항 기준점: 출발 공항과 같은 모양, 활주로 시작 끝이 Z = 2000
// 비행기가 +Z 로 날아와서 "36" 쪽 끝으로 내려앉아 +Z 방향으로 활주하게 됨
const glm::vec3 DEST_AIRPORT_POS = glm::vec3(0.0f, -0.5f, 2000.0f);

void InitAirport();                                                // VAO/VBO 생성
void DrawAirport(const glm::mat4& viewProj, GLuint transformLoc);  // 출발 + 목적지 공항 그리기

// 공항에 서 있는 것과 같은 여객기 모델 그리기 (InitAirport() 이후에 사용)
//  mvp    : viewProj * 비행기 모델 행렬. 원점 = 바퀴 바닥 중앙, 기수 = +Z, 위 = +Y
//  length : 기수~꼬리 길이 (날개 폭도 같은 길이, 높이는 길이의 약 0.36배)
void DrawAirlinerModel(const glm::mat4& mvp, GLuint transformLoc, float length);
