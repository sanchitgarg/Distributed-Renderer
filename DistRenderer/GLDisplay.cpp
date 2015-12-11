#include "GLDisplay.h"

bool GLDisplay::windowActive = false;
bool GLDisplay::initialized = false;
bool GLDisplay::printimg = 0;
bool GLDisplay::camchanged = 0;
float GLDisplay::theta = 0;
float GLDisplay::phi = 0;
glm::vec3 GLDisplay::cammove = glm::vec3(0, 0, 0);
GLFWwindow* GLDisplay::window = nullptr;

GLDisplay::GLDisplay(PacketManager* pMgr_){
	if (initialized){
		std::cout << "Display can only be initialized once." << std::endl;
		exit(EXIT_FAILURE);
	}

	pMgr = pMgr_;

	initialized = true;
	modified = false;

	positionLocation = 0;
	texcoordsLocation = 1;
	
	pixels = new GLubyte[WIDTH * HEIGHT * 3]();
	init();
}

void GLDisplay::setPixelColor(int index, int r, int g, int b){
	if (index < 0 || index >= WIDTH * HEIGHT){
		std::cout << "index is out of range : " << index << std::endl;
		//exit(EXIT_FAILURE);
		return;
	}

	int offset = index * 3;
	pixels[offset] = r;
	pixels[offset + 1] = g;
	pixels[offset + 2] = b;

	modified = true;

	//std::cout << r << " " << g << " " << b << std::endl;
	//std::cout << pixels[offset] << " " << pixels[offset + 1] << " " << pixels[offset + 2] << std::endl;
}

Message::CAM_MOVE* GLDisplay::checkCameraMove(){
	if (camchanged){
		Message::CAM_MOVE* cmsg = new Message::CAM_MOVE();
		cmsg->set_phi(phi);
		cmsg->set_theta(theta);
		cmsg->set_cammove_x(cammove.x);
		cmsg->set_cammove_y(cammove.y);
		cmsg->set_cammove_z(cammove.z);

		camchanged = false;
		return cmsg;
	}

	return nullptr;
}

bool GLDisplay::update(){
	if (!windowActive) return false;

	if (printimg)
		saveImage("FrontEnd");

	if (!glfwWindowShouldClose(window)) {
		checkGLError("pre-update");
		glfwPollEvents();

		if (!modified) return false;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
		checkGLError("update");

		glClearColor(0.0, 1.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
		glfwSwapBuffers(window);
		checkGLError("draw");

		modified = false;
		return true;
	}
	else {
		glfwDestroyWindow(window);
		glfwTerminate();
		windowActive = false;
	}

	return false;
}

void GLDisplay::saveImage(std::string title) {
	// output image file
	image img(WIDTH, HEIGHT);
	int pixelcount = WIDTH * HEIGHT;
	for (int ptr = 0; ptr < pixelcount; ptr++) {
		int offset = ptr * 3;
		int y = ptr / WIDTH;
		int x = ptr - (y * WIDTH);

		glm::ivec3 pix;
		pix.r = pixels[offset];
		pix.g = pixels[offset + 1];
		pix.b = pixels[offset + 2];

		//std::cout << pix.r << " " << pix.g << " " << pix.b << std::endl;
		img.setPixel(WIDTH - 1 - x, y, glm::vec3(pix) / 255.0f);
	}

	std::string filename = "Saved";
	filename = filename + "." + title;


	// CHECKITOUT
	img.savePNG(filename);
	//img.saveHDR(filename);  // Save a Radiance HDR file

}

bool GLDisplay::init(){
	glfwSetErrorCallback(utilityCore::errorCallback);

	if (!glfwInit()) {
		std::cout << "glfw not OK" << std::endl;
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(WIDTH, WIDTH, "Distributed Renderer - Front End Viewer", NULL, NULL);
	if (!window) {
		glfwTerminate();
		std::cout << "Window not OK" << std::endl;
		return false;
	}

	glfwMakeContextCurrent(window);
	windowActive = true;

	// Set up GL context
	glewExperimental = GL_TRUE;
	checkGLError("glewExperimental");
	if (glewInit() != GLEW_OK) {
		std::cout << "glew not OK" << std::endl;
		return false;
	}

	glfwSetKeyCallback(window, keyCallback);
	//glfwSetCursorPosCallback(window, cursorCallback);
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	//glfwSetCursorPos(window, WIDTH/2, HEIGHT/2);

	checkGLError("glfwSetKeyCallback");

	// Initialize other stuff
	initShader();
	checkGLError("passthroughProgram");

	initVAO();
	checkGLError("initVAO");

	initTextures();
	checkGLError("initTextures");

	return true;
}

void GLDisplay::initTextures() {
	glGenTextures(1, &displayImage);
	glBindTexture(GL_TEXTURE_2D, displayImage);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	checkGLError("initTextures");
}

void GLDisplay::initVAO(void) {
	GLfloat vertices[] = {
		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f,
		-1.0f, 1.0f,
	};

	GLfloat texcoords[] = {
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f
	};

	GLushort indices[] = { 0, 1, 3, 3, 1, 2 };

	GLuint vertexBufferObjID[3];
	glGenBuffers(3, vertexBufferObjID);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjID[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)positionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionLocation);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjID[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)texcoordsLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(texcoordsLocation);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexBufferObjID[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

GLuint GLDisplay::initShader() {
	const char *attribLocations[] = { "Position", "Texcoords" };
	GLuint program = glslUtility::createDefaultProgram(attribLocations, 2);
	checkGLError("createDefaultProgram");

	glUseProgram(program);

	GLint location = glGetUniformLocation(program, "u_image");
	checkGLError("glGetUniformLocation");
	if (location != -1) {
		glUniform1i(location, 0);
	}
	checkGLError("glUniform1i");

	glActiveTexture(GL_TEXTURE0);

	return program;
}

void GLDisplay::checkGLError(std::string mark){
	GLuint glErr = glGetError();
	if (glErr != GL_NO_ERROR)
	{
		std::cout << "GL Error Code: " << glErr <<
			" [" << mark << ":" << glewGetErrorString(glErr) << "]" << std::endl;
	}
}

void GLDisplay::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
			printimg = true;
			glfwSetWindowShouldClose(window, GL_TRUE);
			windowActive = false;
			break;
		case GLFW_KEY_SPACE:
			printimg = true;
			break;
		case GLFW_KEY_DOWN:  camchanged = true; theta = -0.1f; break;
		case GLFW_KEY_UP:    camchanged = true; theta = +0.1f; break;
		case GLFW_KEY_RIGHT: camchanged = true; phi = -0.1f; break;
		case GLFW_KEY_LEFT:  camchanged = true; phi = +0.1f; break;
		case GLFW_KEY_A:     camchanged = true; cammove -= glm::vec3(.1f, 0, 0); break;
		case GLFW_KEY_D:     camchanged = true; cammove += glm::vec3(.1f, 0, 0); break;
		case GLFW_KEY_W:     camchanged = true; cammove += glm::vec3(0, 0, .1f); break;
		case GLFW_KEY_S:     camchanged = true; cammove -= glm::vec3(0, 0, .1f); break;
		case GLFW_KEY_R:     camchanged = true; cammove += glm::vec3(0, .1f, 0); break;
		case GLFW_KEY_F:     camchanged = true; cammove -= glm::vec3(0, .1f, 0); break;
		}
	}
}