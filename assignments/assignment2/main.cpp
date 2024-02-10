#include <hb/Framebuffer.h>
#include <ew/procGen.h>
#include <glm/gtc/type_ptr.hpp>

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI(); 
void resetCamera(ew::Camera* camera, ew::CameraController* controller);

//Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

//cam setup
ew::Camera camera;
ew::Camera light;
ew::CameraController cameraController;

struct Material {
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
}material;

float blurScale = 0.0f;
int kernalSet = 0;

hb::Framebuffer shadowBuffer;

int main() {
	GLFWwindow* window = initWindow("Assignment 0", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	//model setup
	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader pp = ew::Shader("assets/pp.vert", "assets/pp.frag");
	ew::Shader shadowShader = ew::Shader("assets/depth.vert", "assets/depth.frag");

	hb::Framebuffer fb = hb::createFramebuffer(screenWidth, screenHeight, GL_RGB16F);
	shadowBuffer = hb::createDepthMap(1024, 1024);

	light.position = glm::vec3(0.0f, 3.0f, 1.0f);
	light.target = glm::vec3(0, 0, 0);
	light.nearPlane = 0.1;
	light.farPlane = 20.0f;
	light.aspectRatio = (float)1024/1024;
	light.orthographic = true;
	light.orthoHeight = 4;
	

	ew::Model monkeyModel = ew::Model("assets/suzanne.obj"); 
	ew::Transform monkeyTransform;
	//Handles to OpenGL object are unsigned integers
	GLuint brickTexture = ew::loadTexture("assets/brick_color.jpg");

	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); //Look at the center of the scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f; //Vertical field of view, in degrees

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); //Back face culling
	glEnable(GL_DEPTH_TEST);//Depth testing
	
	unsigned int dummyVAO;
	glCreateVertexArrays(1, &dummyVAO);

	ew::Mesh planeMesh = ew::Mesh(ew::createPlane(10, 10, 5));
	ew::Transform planeTransform;
	planeTransform.position = glm::vec3(0,-1,0);

	while (!glfwWindowShouldClose(window)) {
		glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);
		glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthBuffer);
		glViewport(0, 0, shadowBuffer.width, shadowBuffer.height);
		glClear(GL_DEPTH_BUFFER_BIT);

		shadowShader.use();

		shadowShader.setMat4("_ViewProjection", light.projectionMatrix() * light.viewMatrix());
		
		shadowShader.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();
		shadowShader.setMat4("_Model", monkeyTransform.modelMatrix());
		monkeyModel.draw();

		glfwPollEvents();
		glClearColor(0.6f, 0.8f, 0.92f, 1.0f);

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;


		cameraController.move(window, &camera, deltaTime);

		//rotate
		monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

		glBindTextureUnit(0, brickTexture);
		glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
		glViewport(0, 0, fb.width, fb.height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Make "_MainTex" sampler2D sample from the 2D texture bound to unit 0
		shader.use(); 
		shader.setFloat("_Material.Ka", material.Ka);
		shader.setFloat("_Material.Kd", material.Kd);
		shader.setFloat("_Material.Ks", material.Ks);
		shader.setFloat("_Material.Shininess", material.Shininess);


		shader.setInt("_MainTex", 0);
		shader.setVec3("_EyePos", camera.position);
		shader.setMat4("_Model", monkeyTransform.modelMatrix());
		shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());

		monkeyModel.draw(); //Draws monkey model using current shader

		shader.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		pp.use();
		pp.setFloat("blurScale", blurScale);
		pp.setInt("kernalSet", kernalSet);

		glBindTextureUnit(0, fb.colorBuffer[0]);
		glBindVertexArray(dummyVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		drawUI();

		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}

void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	if (ImGui::Button("Reset Camera")) {
		resetCamera(&camera, &cameraController);
	}
	if (ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
	}
	if (ImGui::CollapsingHeader("Post Processing")) {
		ImGui::SliderFloat("BlurScale", &blurScale, 0.0f, 10.0f);
		ImGui::SliderInt("KernalSet", &kernalSet, 0.0f, 3.0f);
	}
	//Add more camera settings here!
	ImGui::End();
	
	ImGui::Begin("Shadow Map");
	//Using a Child allow to fill all the space of the window.
	ImGui::BeginChild("Shadow Map");
	//Stretch image to be window size
	ImVec2 windowSize = ImGui::GetWindowSize();
	//Invert 0-1 V to flip vertically for ImGui display
	//shadowMap is the texture2D handle
	ImGui::Image((ImTextureID)shadowBuffer.depthBuffer, windowSize, ImVec2(0, 1), ImVec2(1, 0));
	ImGui::EndChild();
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;
}

/// <summary>
/// Initializes GLFW, GLAD, and IMGUI
/// </summary>
/// <param name="title">Window title</param>
/// <param name="width">Window width</param>
/// <param name="height">Window height</param>
/// <returns>Returns window handle on success or null on fail</returns>
GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}

void resetCamera(ew::Camera* camera, ew::CameraController* controller)
{
	camera->position = glm::vec3(0, 0, 5.0f);
	camera->target = glm::vec3(0);
	controller->yaw = controller->pitch = 0;
}



