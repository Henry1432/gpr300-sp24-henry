#include <hb/Framebuffer.h>
#include <ew/procGen.h>
#include <glm/gtc/type_ptr.hpp>

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();
void Update();
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

struct PointLight {
	glm::vec3 position;
	float radius;
	glm::vec3 color;
};

float blurScale = 0.0f;
int kernalSet = 0;

hb::Framebuffer shadowBuffer; 
hb::Framebuffer gBuffer;
glm::vec3 setLightPos = glm::vec3(0.0f, 1.0f, 1.0f);
float bias = 0.005;
int shadScale = 9;
GLFWwindow* window;
ew::Transform monkeyTransform;

int main() {
	window = initWindow("Assignment 0", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	//model setup
	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader pp = ew::Shader("assets/pp.vert", "assets/pp.frag");
	ew::Shader shadowShader = ew::Shader("assets/depth.vert", "assets/depth.frag");
	ew::Shader gBufferShader = ew::Shader("assets/geometryPass.vert", "assets/geometryPass.frag");
	ew::Shader deferredLit = ew::Shader("assets/deferredLit.vert", "assets/deferredLit.frag");
	ew::Shader lightOrbShader = ew::Shader("assets/lightOrb.vert", "assets/lightOrb.frag");
	
	gBuffer = hb::createGBuffer(screenWidth, screenHeight);
	hb::Framebuffer fb = hb::createFramebuffer(screenWidth, screenHeight, GL_RGB16F);
	shadowBuffer = hb::createDepthMap(1024, 720);

	light.position = glm::vec3(0.0f, 3.0f, 3.0f);
	light.target = glm::vec3(0, 0, 0);
	light.nearPlane = 0.5;
	light.farPlane = 15.0f;
	light.aspectRatio = (float)1024/1024;
	light.orthographic = true;
	light.orthoHeight = 4;
	
	const int numPointLights = 64;
	PointLight pointLights[numPointLights];

	for (int i = 0; i < numPointLights; i++)
	{
		pointLights[i].position = glm::vec3(i, 3, i);
		pointLights[i].radius = 5.0f;
		float color = ((float)i)/ numPointLights;
		pointLights[i].color = glm::vec3(1, color, color);
	}

	ew::Model monkeyModel = ew::Model("assets/suzanne.obj"); 
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

	ew::Mesh planeMesh = ew::Mesh(ew::createPlane(100, 100, 5));
	ew::Mesh sphereMesh = ew::Mesh(ew::createSphere(1.0f, 8));
	ew::Transform planeTransform;
	planeTransform.position = glm::vec3(45,-1,45);

	while (!glfwWindowShouldClose(window)) {
		Update();

		glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);
		glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthBuffer);
		glViewport(0, 0, shadowBuffer.width, shadowBuffer.height);
		glClear(GL_DEPTH_BUFFER_BIT);

		shadowShader.use();

		shadowShader.setMat4("_ViewProjection", light.projectionMatrix() * light.viewMatrix());

		/*shadowShader.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();*/
		shadowShader.setMat4("_Model", monkeyTransform.modelMatrix());
		monkeyModel.draw();



		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.fbo);
		glViewport(0, 0, gBuffer.width, gBuffer.height);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		gBufferShader.use();
		glBindTextureUnit(0, brickTexture);
		gBufferShader.setInt("_MainTex", 0);
		gBufferShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());

		gBufferShader.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();
		gBufferShader.setMat4("_Model", monkeyTransform.modelMatrix());
		monkeyModel.draw();

		glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
		glViewport(0, 0, fb.width, fb.height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindTextureUnit(0, gBuffer.colorBuffer[0]);
		glBindTextureUnit(1, gBuffer.colorBuffer[1]);
		glBindTextureUnit(2, gBuffer.colorBuffer[2]);
		glBindTextureUnit(3, shadowBuffer.depthBuffer); //For shadow mapping

		deferredLit.use();

		deferredLit.setVec3("_EyePos", camera.position);
		deferredLit.setFloat("_Material.Ka", material.Ka);
		deferredLit.setFloat("_Material.Kd", material.Kd);
		deferredLit.setFloat("_Material.Ks", material.Ks);
		deferredLit.setFloat("_Material.Shininess", material.Shininess);
		for (int i = 0; i < numPointLights; i++) {
			//Creates prefix "_PointLights[0]." etc
			std::string prefix = "pointLights[" + std::to_string(i) + "].";
			deferredLit.setVec3(prefix + "position", pointLights[i].position);
			deferredLit.setVec3(prefix + "color", pointLights[i].color);
			deferredLit.setFloat(prefix + "radius", pointLights[i].radius);
		}

		glBindVertexArray(dummyVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);


		//Blit gBuffer depth to same framebuffer as fullscreen quad
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.fbo); //Read from gBuffer 
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.fbo); //Write to current fbo
		glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		
		//Draw all light orbs
		lightOrbShader.use();
		lightOrbShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		for (int i = 0; i < numPointLights; i++)
		{
			glm::mat4 m = glm::mat4(1.0f);
			m = glm::translate(m, pointLights[i].position);
			m = glm::scale(m, glm::vec3(0.2f)); //Whatever radius you want
			
			lightOrbShader.setMat4("_Model", m);
			lightOrbShader.setVec3("_Color", pointLights[i].color);
			sphereMesh.draw();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb.width, fb.height);
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

void Update()
{
	glfwPollEvents();
	glClearColor(0.6f, 0.8f, 0.92f, 1.0f);

	float time = (float)glfwGetTime();
	deltaTime = time - prevFrameTime;
	prevFrameTime = time;


	cameraController.move(window, &camera, deltaTime);

	//rotate
	monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

	light.position = setLightPos * 3.0f;
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
	if (ImGui::CollapsingHeader("Lighting")) { 
		float *lightPos[3] = {&(float)setLightPos.x, &(float)setLightPos.y, &(float)setLightPos.z};
		ImGui::SliderFloat3("LightPosition", *lightPos, -1, 1);
		ImGui::SliderFloat("Bias", &bias, 0.0, 0.05);
		ImGui::SliderInt("ShadeScale", &shadScale, 1.0f, 25.0f);
	}
	//Add more camera settings here!
	ImGui::End();
	ImGui::Begin("GBuffers"); {
		ImVec2 texSize = ImVec2(gBuffer.width / 4, gBuffer.height / 4);
		for (size_t i = 0; i < 3; i++)
		{
			ImGui::Image((ImTextureID)gBuffer.colorBuffer[i], texSize, ImVec2(0, 1), ImVec2(1, 0));
		}
		ImGui::Image((ImTextureID)gBuffer.depthBuffer, texSize, ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();
	}

	
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



