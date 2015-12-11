#pragma once

#include <sstream> 
#include "image.h"
#include "msg.pb.h"
#include "Helper.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "glslUtility.hpp"
#include "utilities.h"
#include "PacketManager.h"

class GLDisplay{
	public:
		GLDisplay(PacketManager* pMgr_);
		void setPixelColor(int index, int r, int g, int b);
		bool update();

		void saveImage(std::string text);
		Message::CAM_MOVE* checkCameraMove();

	private:
		bool modified;
		GLubyte* pixels;
		GLuint displayImage;
		PacketManager* pMgr;

		bool init();
		void initTextures();
		void initVAO();
		GLuint initShader();
		void checkGLError(std::string mark);

		GLuint positionLocation = 0;
		GLuint texcoordsLocation = 1;
		
	//static variables-functions: basically everything related to GLFW
		static bool windowActive;
		static bool initialized;
		static GLFWwindow* window;
		static bool printimg;
		static bool camchanged;
		static float theta;
		static float phi;
		static glm::vec3 cammove;

		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void cursorCallback(GLFWwindow* window, double xpos, double ypos);
};