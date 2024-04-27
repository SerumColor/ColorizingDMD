#include "OGL_Immediate_2D.h"

float gl33_FPS; // le nombre de frames par seconde
DWORD gl33_lastTime = 0; // dernier timeGetTime() pour le calcul du FPS
DWORD gl33_nbHits = 0; // nombre de hits entre 2 secondes
bool gl33_init = false;

int text_CreateTextureCircle(GLFWwindow* glfww)
{
	glfwMakeContextCurrent(glfww);
	// si pwidth etpheight sont !=NULL ils renvoient la taille de l'image
	/* Generate texture */
	GLuint texid;
	UINT8 texel[64] = { 0,255,255,255,255,255,255,0,
						255,255,255,255,255,255,255,255,
						255,255,255,255,255,255,255,255,
						255,255,255,255,255,255,255,255,
						255,255,255,255,255,255,255,255,
						255,255,255,255,255,255,255,255,
						255,255,255,255,255,255,255,255,
						0,255,255,255,255,255,255,0 };

	//ZeroMemory(texel, 64);

	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);

	/* Setup texture filters */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_INTENSITY8, 8, 8, 0, GL_RED, GL_UNSIGNED_BYTE, texel);

	return texid;
}

bool gl33_InitWindow(GLFWwindow** glfwnWin, int width, int height, const char* title, HWND hParent)
{
	// Initializing GLFW
	if (!gl33_init)
	{
		if (!glfwInit())
		{
			return false;
		}
		gl33_init = true;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); 
	glfwWindowHint(GLFW_DECORATED, false);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); //We don't want the old OpenGL
	*glfwnWin = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!*glfwnWin)
	{
		return false;
	}
	glfwMakeContextCurrent(*glfwnWin);
	if (hParent)
	{
		HWND oglwin = glfwGetWin32Window(*glfwnWin);
		SetParent(oglwin, hParent);
		const LONG nNewStyle = (GetWindowLong(oglwin, GWL_STYLE) & ~WS_POPUP) | WS_CHILDWINDOW;
		SetWindowLong(oglwin, GWL_STYLE, nNewStyle);
	}

	while (glGetError() != GL_NO_ERROR);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		return false;
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	//glShadeModel(GL_SMOOTH);					// Enables Smooth Color Shading
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	return true;
}

float gl33_SwapBuffers(GLFWwindow* fwWin,bool calcFPS)
{
	if (calcFPS)
	{
		if (gl33_lastTime > 0)
		{
			DWORD acTime, elpsTime;
			acTime = timeGetTime();
			elpsTime = acTime - gl33_lastTime;
			if (elpsTime > 1000)
			{
				gl33_FPS = (float)((1.0f * gl33_nbHits) / (elpsTime / 1000.0f));
				gl33_lastTime = acTime;
				gl33_nbHits = 0;
			}
			gl33_nbHits++;
		}
		else
		{
			gl33_FPS = 0;
			gl33_lastTime = timeGetTime();
		}
	}
	glfwMakeContextCurrent(fwWin);
	glfwSwapBuffers(fwWin);
	glClear(GL_COLOR_BUFFER_BIT);
	return gl33_FPS;
}

