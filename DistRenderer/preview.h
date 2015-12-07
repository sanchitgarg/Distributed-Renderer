#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

#pragma comment(lib, "glew32.lib")

#include "image.h"
#include "scene.h"
#include "glslUtility.hpp"

class Viewer{
	public:
		Viewer(Scene *scene_);
		~Viewer();

		bool init();
		void update(int iteration);
		void saveImage(std::string startTime, int iteration);
		GLuint getPBO();

	private:
		void initTextures();
		void initVAO();
		void initPBO();
		GLuint initShader();

		void initCuda();
		void cleanupCuda();

		Scene* scene;
		RenderState *renderState;
		int width;
		int height;

		GLuint positionLocation = 0;
		GLuint texcoordsLocation = 1;
		GLuint pbo;
		GLuint displayImage;

		static bool windowActive;
		static bool initialized;
		static GLFWwindow *window;
};