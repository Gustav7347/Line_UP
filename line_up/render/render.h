#pragma once
#include <GL/glew.h>
#include <GL/glfw3.h>

// 셰이더, 좌표축, 카메라, 장면 그리기 담당
void InitShader();                 // glsl 파일 읽어서 셰이더 프로그램 생성
void InitAxis();                   // X/Y/Z 좌표축 VAO/VBO 생성
void DrawScene();                  // 매 프레임 장면 그리기
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
