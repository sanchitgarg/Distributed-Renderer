#include "GLDisplay.h"

bool GLDisplay::initialized = false;
float GLDisplay::yaw = 0;
float GLDisplay::pitch = 0;
float GLDisplay::roll = 0;
glm::vec3 GLDisplay::cam_pos = glm::vec3(0, 0, 0);
GLFWwindow* GLDisplay::window = nullptr;

GLDisplay::GLDisplay(){
	if (initialized){
		std::cout << "Display can only be initialized once." << std::endl;
		exit(EXIT_FAILURE);
	}

	initialized = true;
	modified = false;
	
	windowInit();
	glInit();
	pixels = new GLubyte[WIDTH * HEIGHT * 3]();

	//do it once to clear the scene
	draw();
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

bool GLDisplay::update(){
	if (!modified) return false;

	glfwPollEvents();
	glBindTexture(GL_TEXTURE_2D, displayImage);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	modified = false;
	return true;
}

void GLDisplay::draw(){
	glClearColor(0.0, 1.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	glfwSwapBuffers(window);
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
void GLDisplay::windowInit(){
	glfwSetErrorCallback(errorCallBack);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Distributed Renderer", NULL, NULL);

	if (!window){
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, keyCallback);
	//glfwSetCursorPosCallback(window, cursorCallback);
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	//glfwSetCursorPos(window, WIDTH/2, HEIGHT/2);
}

void GLDisplay::glInit(){
	//set GL context
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK){
		exit(EXIT_FAILURE);
	}

	initProgram();
}
GLuint GLDisplay::initShader() {
	const char *attribLocations[] = { "Position", "Texcoords" };
	GLuint program = glslUtility::createDefaultProgram(attribLocations, 2);
	GLint location;

	//glUseProgram(program);
	if ((location = glGetUniformLocation(program, "u_image")) != -1) {
		glUniform1i(location, 0);
	}

	return program;
}

//TODO:: add local shader program implementation.
void GLDisplay::initProgram(){
	GLuint passthroughProgram = initShader();

	glUseProgram(passthroughProgram);
	glActiveTexture(GL_TEXTURE0);

	//set up VBOs
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

	GLuint vbo[3];
	GLuint posLocation = 0;
	GLuint texLocation = 1;

	glGenBuffers(3, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)posLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posLocation);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)texLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(texLocation);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//init texture
	glGenTextures(1, &displayImage);
	glBindTexture(GL_TEXTURE_2D, displayImage);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}

void GLDisplay::errorCallBack(int error, const char* description){
	fputs(description, stderr);
}

void GLDisplay::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		exit(0);
}

void GLDisplay::cursorCallback(GLFWwindow* window, double xpos, double ypos){
	//camera control
	yaw += (xpos - HALF_WIDTH) * YAW_SENSITIVITY;		//around local y
	pitch += (ypos - HALF_HEIGHT) * PITCH_SENSITIVITY;	//around local x

	//reset cursor to middle pos.
	//glfwSetCursorPos(window, HALF_WIDTH, HALF_HEIGHT);
}