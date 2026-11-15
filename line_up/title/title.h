#pragma once
#include <GL/glew.h>
#include <GL/glfw3.h>

// 타이틀 화면 담당 (배경, 번개, 우박, 로고, PRESS ENTER)
void InitTitle();                                  // 타이틀 이미지/셰이더 로드 (glewInit 이후 한 번 호출)
void DrawTitle();                                  // 매 프레임 타이틀 화면 그리기
bool IsTitleActive();                              // 아직 타이틀 화면이면 true
void TitleHandleKey(GLFWwindow* window, int key);  // Enter: 게임 시작, Q: 종료
