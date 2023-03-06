#pragma once
#include "framework.h"
#include <GL/glew.h>
#include <GL/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GL/glfw3native.h>
#include <timeapi.h>

bool gl33_InitWindow(GLFWwindow** glfwnWin, int width, int height, const char* title, HWND hParent);
float gl33_SwapBuffers(GLFWwindow* fwWin, bool calcFPS);
int text_CreateTextureCircle(GLFWwindow* glfww);