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

class GLDisplay{
	public:
		GLDisplay();
		void setPixelColor(int index, int r, int g, int b);
		bool update();

		void saveImage(std::string text);
		void checkGLError(std::string mark);

	private:
		bool modified;
		GLubyte* pixels;
		GLuint displayImage;

		bool init();
		void initTextures();
		void initVAO();
		GLuint initShader();

		GLuint positionLocation = 0;
		GLuint texcoordsLocation = 1;
		
	//static variables-functions: basically everything related to GLFW
		static bool windowActive;
		static bool initialized;
		static GLFWwindow* window;
		static glm::vec3 cam_pos;
		static float yaw;
		static float pitch;
		static float roll;

		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void cursorCallback(GLFWwindow* window, double xpos, double ypos);
};