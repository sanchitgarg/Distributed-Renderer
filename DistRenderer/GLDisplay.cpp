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
	
	windowInit();
	glInit();
	pixels = new GLubyte[WIDTH * HEIGHT * 3]();

	//do it once to clear the scene
	draw();
}

void GLDisplay::setPixelColor(int px, int py, int r, int g, int b){
	if (px < 0 || py < 0 || px >= WIDTH || py >= HEIGHT){
		std::cout << "px or py is out of range." << std::endl;
		exit(EXIT_FAILURE);
	}

	int offset = py * (WIDTH * 3) + px * 3;
	pixels[offset] = r;
	pixels[offset + 1] = g;
	pixels[offset + 2] = b;
}

void GLDisplay::update(){
	glfwPollEvents();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
}

void GLDisplay::draw(){
	glClearColor(0.0, 1.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	glfwSwapBuffers(window);
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

//TODO:: add local shader program implementation.
void GLDisplay::initProgram(){
	std::string VSsource_RayTrace =
		"	attribute vec4 Position; \n"
		"	attribute vec2 Texcoords; \n"
		"	varying vec2 v_Texcoords; \n"
		"	\n"
		"	void main(void){ \n"
		"		v_Texcoords = Texcoords; \n"
		"		gl_Position = Position; \n"
		"	}";

	std::string FSSource_RayTrace =
		"	varying vec2 v_Texcoords; \n"
		"	\n"
		"	uniform sampler2D u_Image; \n"
		"	\n"
		"	void main(void){ \n"
		"		gl_FragColor = texture2D(u_Image, v_Texcoords); \n"
		"	}";

	GLuint program_rayTrace = initShaderProgram(VSsource_RayTrace, FSSource_RayTrace);
	glUseProgram(program_rayTrace);

	GLuint vao_rayTrace;
	glGenVertexArrays(1, &vao_rayTrace);
	glBindVertexArray(vao_rayTrace);

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
	GLuint posLocation = glGetAttribLocation(program_rayTrace, "Position");
	GLuint texLocation = glGetAttribLocation(program_rayTrace, "Texcoords");

	glGenBuffers(3, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(posLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posLocation);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
	glVertexAttribPointer(texLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(texLocation);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//init texture
	glGenTextures(1, &displayImage);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, displayImage);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}

GLuint GLDisplay::initShaderProgram(std::string VSsource, std::string FSSource){
	GLuint program = glCreateProgram();

	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	GLint vslen = VSsource.length();
	const char* vs = &VSsource[0];
	glShaderSource(vertex, 1, &vs, &vslen);
	glCompileShader(vertex);

	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
	GLint fslen = FSSource.length();
	const char* fs = &FSSource[0];
	glShaderSource(fragment, 1, &fs, &fslen);
	glCompileShader(fragment);

	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);

	return program;
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