#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "Window.h"
#include "SkyBox.hpp"

#include <iostream>

//window
int glWindowWidth = 800;
int glWindowHeight = 600;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

//constants
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

//vectors
std::vector<const GLchar*> faces;

//matrices
glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

//light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;
GLuint lightDirLoc;
GLuint lightColorLoc;

//camera
gps::Camera myCamera(
	glm::vec3(0.0f, 6.0f, 5.5f),
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.1f;

bool pressedKeys[1024];
float angleY = 0.0f;
GLfloat lightAngle;

//models
gps::Model3D farm;
gps::Model3D lightCube;
gps::Model3D screenQuad;
gps::Model3D racoon;
gps::Model3D scarecrow;
gps::Model3D tree;

//shaders
gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;
gps::Shader skyboxShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;

bool showDepthMap;
bool fullScreen = false;

//skyBox
gps::SkyBox mySkyBox;

//mouse
bool mouse = true;
float lastX, lastY;
float yaw = -90.0f, pitch;

//fog
int initFog = 0;
GLint initFogLocation;
GLfloat initFogDensity = 0.005f;

//camera animation
bool startCameraPreview = false;
float previewCameraAngle;

//scale tree
GLfloat treeScale = 0.0f;
GLfloat scale = 0.01f;

//rotate scarecrow
GLfloat scarecrowRotation = 0.0f;

//move racoon
float moveRacoonX;
float move = 0.01f;

//spotlight
int initSpotLight;
float spotLight;
float spotLight1;

glm::vec3 spotLightDirection;
glm::vec3 spotLightPosition;

void cameraPreviewFunction() {
	if (startCameraPreview) {
		previewCameraAngle += 0.3f;
		myCamera.scenePreview(previewCameraAngle);
	}
}

GLenum glCheckError_(const char* file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	// get the size
	glfwGetFramebufferSize(window, &width, &height);
	glfwGetFramebufferSize(window, &retina_width, &retina_height);
	// compute the matrix and send it to the shader
	myCustomShader.useShaderProgram();
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
	GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	// set Viewport transform
	glViewport(0, 0, width, height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	if (mouse) {
		lastX = xpos;
		lastY = ypos;
		mouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	xoffset *= 0.15f;
	yoffset *= 0.15f;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	myCamera.rotate(pitch, yaw);
	view = myCamera.getViewMatrix();
	myCustomShader.useShaderProgram();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::inverseTranspose(view * model))));
}

void processMovement()
{
	//start preview
	if (pressedKeys[GLFW_KEY_Z]) {
		startCameraPreview = true;
	}

	//stop preview
	if (pressedKeys[GLFW_KEY_X]) {
		startCameraPreview = false;
	}

	//line view
	if (pressedKeys[GLFW_KEY_1]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	//point view
	if (pressedKeys[GLFW_KEY_2]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}

	//normal view
	if (pressedKeys[GLFW_KEY_3]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	//smooth view
	if (pressedKeys[GLFW_KEY_4]) {
		glEnable(GL_MULTISAMPLE);
	}

	if (pressedKeys[GLFW_KEY_5]) {
		glDisable(GL_MULTISAMPLE);
	}

	//rotate scene
	if (pressedKeys[GLFW_KEY_Q]) {
		angleY -= 1.0f;
	}

	if (pressedKeys[GLFW_KEY_E]) {
		angleY += 1.0f;
	}

	//move light
	if (pressedKeys[GLFW_KEY_K]) {
		lightAngle -= 1.0f;
	}

	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle += 1.0f;
	}

	//start spotlight
	if (pressedKeys[GLFW_KEY_O]) {
		myCustomShader.useShaderProgram();
		initSpotLight = 1;
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "initSpotLight"), initSpotLight);
	}

	//stop spotlight
	if (pressedKeys[GLFW_KEY_P]) {
		myCustomShader.useShaderProgram();
		initSpotLight = 0;
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "initSpotLight"), initSpotLight);
	}

	//move camera
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
	}

	//full screen
	if (pressedKeys[GLFW_KEY_F]) {
		if (fullScreen)
		{
			glfwSetWindowMonitor(glWindow, nullptr, 100, 100, glWindowWidth, glWindowHeight, GLFW_DONT_CARE);
			windowResizeCallback;
		}
		else
		{
			glfwSetWindowMonitor(glWindow, glfwGetPrimaryMonitor(), 0, 0, 1400, 800 , GLFW_DONT_CARE);
			windowResizeCallback;
		}
		fullScreen = !fullScreen;
	}

	//print camera's coordinates
	if (pressedKeys[GLFW_KEY_I]) {
		printf("Camera position: x = %f   y = %f   z = %f \n", myCamera.cameraPosition.x, myCamera.cameraPosition.y, myCamera.cameraPosition.z);
		printf("Camera direction: x = %f   y = %f   z = %f \n", myCamera.cameraFrontDirection.x, myCamera.cameraFrontDirection.y, myCamera.cameraFrontDirection.z);
	}

	//start fog
	if (pressedKeys[GLFW_KEY_R]) {
		myCustomShader.useShaderProgram();
		initFog = 1;
		initFogLocation = glGetUniformLocation(myCustomShader.shaderProgram, "initFog");
		glUniform1i(initFogLocation, initFog);
	}

	//stop fog
	if (pressedKeys[GLFW_KEY_T]) {
		myCustomShader.useShaderProgram();
		initFog = 0;
		initFogLocation = glGetUniformLocation(myCustomShader.shaderProgram, "initFog");
		glUniform1i(initFogLocation, initFog);
	}

	// increase the intensity of fog
	if (pressedKeys[GLFW_KEY_Y]) {
		initFogDensity = glm::min(initFogDensity + 0.0001f, 1.0f);
	}

	// decrease the intensity of fog
	if (pressedKeys[GLFW_KEY_U]) {
		initFogDensity = glm::max(initFogDensity - 0.0001f, 0.0f);
	}
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwMakeContextCurrent(glWindow);
	glfwSwapInterval(1);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);
	return true;
}

void initOpenGLState()
{
	glClearColor(0.3, 0.3, 0.3, 1.0);
	glViewport(0, 0, retina_width, retina_height);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
	glEnable(GL_FRAMEBUFFER_SRGB);
}

void initObjects() {
	farm.LoadModel(
		"objects/Scene.obj");

	lightCube.LoadModel(
		"objects/cube/cube.obj");

	screenQuad.LoadModel(
		"objects/quad/quad.obj");

	racoon.LoadModel(
		"objects/racoon/racoon.obj", 
		"objects/racoon/");

	scarecrow.LoadModel(
		"objects/scarecrow/scarecrow.obj", 
		"objects/scarecrow/");

	tree.LoadModel(
		"objects/tree/tree.obj", 
		"objects/tree/");
}

void initShaders() {
	myCustomShader.loadShader(
		"shaders/shaderStart.vert", 
		"shaders/shaderStart.frag");

	lightShader.loadShader(
		"shaders/lightCube.vert",
		"shaders/lightCube.frag");

	screenQuadShader.loadShader(
		"shaders/screenQuad.vert", 
		"shaders/screenQuad.frag");

	depthMapShader.loadShader(
		"shaders/depthMapShader.vert", 
		"shaders/depthMapShader.frag");

	skyboxShader.loadShader(
		"shaders/skyboxShader.vert",
		"shaders/skyboxShader.frag");
}

void initSkyBox() {
	faces.push_back("skybox/posx(1).jpg"); //right
	faces.push_back("skybox/negx(1).jpg"); //left
	faces.push_back("skybox/posy(1).jpg"); //top
	faces.push_back("skybox/negy(1).jpg"); //bottom
	faces.push_back("skybox/posz(1).jpg"); //back
	faces.push_back("skybox/negz(1).jpg"); //front
}

void loadSkyBox() {
	mySkyBox.Load(faces);
	skyboxShader.useShaderProgram();
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	projection = glm::perspective(glm::radians(45.0f), (float)glWindowWidth / (float)glWindowHeight, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void initUniforms() {
	myCustomShader.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(1.0f, 1.0f, 0.0f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	// spotlight
	spotLight = glm::cos(glm::radians(10.0f));
	spotLight1 = glm::cos(glm::radians(20.0f));
	spotLightDirection = glm::vec3(0, 0, -1);
	spotLightPosition = glm::vec3(0.0f, 1.0f, 0.0f);

	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight"), spotLight);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotLight1"), spotLight1);
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "spotLightDirection"), 1, glm::value_ptr(spotLightDirection));
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "spotLightPosition"), 1, glm::value_ptr(spotLightPosition));

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void initFBO() {
	glGenFramebuffers(1, &shadowMapFBO);
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	float borderColor[] = { 1.0f,1.0f,1.0f,1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	const GLfloat near_plane = 0.1f, far_plane = 200.0f;
	glm::mat4 lightProjection = glm::ortho(-100.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
	glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
	glm::mat4 lightView = glm::lookAt(lightDirTr, myCamera.cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
	return lightProjection * lightView;
}

void drawObjects(gps::Shader shader, bool depthPass) {
	shader.useShaderProgram();

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 modelCopy = model;
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	farm.Draw(shader);

	model = glm::translate(model, glm::vec3(6.25f, 1.44f, -12.48f));
	model = glm::scale(model, glm::vec3(treeScale, treeScale, treeScale));
	model = glm::translate(model, glm::vec3(-6.25f, -1.44f, 12.48f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	tree.Draw(shader);
	if (treeScale >= 2.0f) {
		scale = -0.01f;
	}
	if (treeScale <= 0.5f) {
		scale = 0.01f;
	}
	treeScale += scale;

	//rotate scarecrow
	model = glm::translate(modelCopy, glm::vec3(10.29, 0, 13.808));
	model = glm::rotate(model, scarecrowRotation, glm::vec3(0, 1, 0));
	model = glm::translate(model, glm::vec3(-10.29, 0, -13.808));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	scarecrow.Draw(shader);
	scarecrowRotation += 0.01f;

	//moveRacoon
	model = glm::translate(modelCopy, glm::vec3(moveRacoonX, 0, moveRacoonX));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	if (moveRacoonX >= 10.0f)
		move = -0.01f;
	if (moveRacoonX <= 0.0f)
		move = 0.01f;
	moveRacoonX += move;
	racoon.Draw(shader);
}

void renderScene() {
	//camera preview
	cameraPreviewFunction();

	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawObjects(depthMapShader, true);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (showDepthMap) {
		glViewport(0, 0, retina_width, retina_height);
		glClear(GL_COLOR_BUFFER_BIT);
		screenQuadShader.useShaderProgram();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		glDisable(GL_DEPTH_TEST);
		screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);
	}
	else {
		glViewport(0, 0, retina_width, retina_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		myCustomShader.useShaderProgram();
		glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
		glViewport(0, 0, retina_width, retina_height);
		myCustomShader.useShaderProgram();

		view = myCamera.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "initFogDensity"), initFogDensity);
		glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

		drawObjects(myCustomShader, false);

		lightShader.useShaderProgram();
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		model = lightRotation;
		model = glm::translate(model, glm::vec3(0.0f, 20.0f, 0.0f) );
		model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
		lightCube.Draw(lightShader);
	}
	mySkyBox.Draw(skyboxShader, view, projection);
}

void cleanup() {
	glDeleteTextures(1, &depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);
	glfwTerminate();
}

int main(int argc, const char* argv[]) {

	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}

	initOpenGLState();
	initObjects();
	initShaders();
	initUniforms();
	initFBO();
	initSkyBox();
	loadSkyBox();

	glCheckError();

	while (!glfwWindowShouldClose(glWindow)) {
		processMovement();
		renderScene();

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	cleanup();
	return 0;
}
