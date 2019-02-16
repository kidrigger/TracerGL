#include <iostream>
#include "glad/glad.h"

#include "GLFW/glfw3.h"

#include "Shader.h"
#include "Camera.h"
#include "ShaderStructs.h"
#include <random>
#include <iomanip>
#include <stbi/stb_image.h>

extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

const float rad2deg = 180.0f/ 3.1415926535f;

int main() {

	GLFWwindow* window = nullptr;

	const int  WIDTH      = 1920;
	const int  HEIGHT     = 1080;
	const int CHUNKS_X	  = 2;
	const int CHUNKS_Y	  = 2;
	const bool FULLSCREEN = true;
	const bool SKYBOX_ACTIVE = false;

	if (!glfwInit()) {
		std::cerr << "ERR::GLFW::INIT_FAIL" << std::endl;
		std::cin.ignore(); 
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	if (!monitor) {
		std::cerr << "ERR::GLFW::NO_MONITOR" << std::endl;
		glfwTerminate();
		std::cin.ignore();
		return 1;
	}
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	if (!FULLSCREEN || mode->height != HEIGHT || mode->width != WIDTH) {
		monitor = nullptr;
	}

	window = glfwCreateWindow(WIDTH, HEIGHT, "TracerGL", monitor, nullptr);
	if (!window) {
		std::cerr << "ERR::GLFW::WNDW_CREATE_FAIL" << std::endl;
		glfwTerminate();
		std::cin.ignore();
		return 1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "ERR::GLAD::GL_LOADER_FAIL" << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		std::cin.ignore();
		return 1;
	}

	std::cout << glGetString(GL_VENDOR) << ' ' << glGetString(GL_RENDERER) << std::endl;

	// Getting the work-group details from the GPU
	int work_grp_cnt[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
	std::cout << "Max global work group sizes <" << work_grp_cnt[0] << ',' << work_grp_cnt[1] << ',' << work_grp_cnt[2] << '>' << std::endl;

	int work_grp_size[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
	std::cout << "Max local work group sizes <" << work_grp_size[0] << ',' << work_grp_size[1] << ',' << work_grp_size[2] << '>' << std::endl;

	int work_grp_inv;
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	std::cout << "Max invocations: " << work_grp_inv << std::endl;

	// Creating the shaders
	Shader<ShaderType::COMPUTE> compshdr("raycompute.comp");
	Shader<ShaderType::RENDER> drawshdr("vDraw.vert", "fDraw.frag");

	// Generating the render quad. Just a simple quad.
	float quad[] = {
		-1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f
	};

	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Creating the texture for the output from the raytracer.
	const int TEX_W = WIDTH, TEX_H = HEIGHT;
	GLuint tex_output;
	glGenTextures(1, &tex_output);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_output);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEX_W, TEX_H, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	// TextureCube Skybox
	GLuint tex_sky;
	glGenTextures(1, &tex_sky);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex_sky);
	{
		std::string face_arr[] = {
			"right.jpg",
			"left.jpg",
			"top.jpg",
			"bottom.jpg",
			"front.jpg",
			"back.jpg"
		};
		int w, h, nrChannels;
		for (int i = 0; i < 6; i++) {
			uint8_t* data = stbi_load(("skybox/" + face_arr[i]).c_str(), &w, &h, &nrChannels, 0);
			if (!data) {
				std::cout << "HELL::FROZE" << std::endl;
			}
			if (nrChannels == 3)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			if (nrChannels == 4)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, 0, GL_RGBA8, w, h);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindImageTexture(3, tex_sky, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
	}

	// SSBO for rng
	std::default_random_engine rng;
	std::uniform_int_distribution<uint32_t> distr;
	std::vector<GLuint> init_rng(WIDTH * HEIGHT);
	/*for (GLuint& i : init_rng) {
		i = distr(rng);
	}*/
	for (int i = 0; i < WIDTH; i++) {
		for (int j = 0; j < HEIGHT; j++) {
			init_rng[j*WIDTH + i] = distr(rng);
		}
	}


	compshdr.use();
	GLuint SSBO_rng;
	glGenBuffers(1, &SSBO_rng);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, SSBO_rng);
	glBufferData(GL_SHADER_STORAGE_BUFFER, init_rng.size() * sizeof(GLuint), init_rng.data(), GL_DYNAMIC_DRAW);

	// SSBO for the spheres.
	//std::vector<Shape> obj{
	//	(Sphere({-0.176776f, 0, -0.176776f}, 0.25f, {0.1f, 0.2f, 0.5f}, {4.0f, 4.0f, 4.0f}, 1.0f, MaterialType::LAMBERTIAN)),
	//	(Cuboid({0, 0, 0}, {0.5f, 0.5f, 0.5f}, {0.9f, 0.9f, 0.9f}, 0.1f, MaterialType::LAMBERTIAN)),
	//	(Rect({-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.1f, 0.8f, 0.1f}, 0.0f, MaterialType::LAMBERTIAN))
	//	/*,
	//	{ {0,-100.5,0}	, 100.0f, {0.9f, 0.9f, 0.9f}, 0.0f, MaterialType::LAMBERTIAN},
	//	{ {1,0,0}		,	0.5f, {0.8f, 0.6f, 0.2f}, 0.5f, MaterialType::METALLIC	},
	//	{ {-1,0,0}		,	0.5f, {1.0f, 1.0f, 1.0f}, 1.5f, MaterialType::DIELECTRIC},
	//	{ {-1,0,0}		, -0.48f, {1.0f, 1.0f, 1.0f}, 1.5f, MaterialType::DIELECTRIC} */
	//};

	std::vector<Shape> obj{
		(Rect(glm::vec3{-3,-3,-2}, glm::vec3(6,6,0), glm::vec3(0.73f), 1.0f, MaterialType::LAMBERTIAN)),
		(Rect(glm::vec3(-3,-3,-2), glm::vec3(0,6,6), glm::vec3(0.65f, 0.05f, 0.05f), 1.0f, MaterialType::LAMBERTIAN)),
		(Rect(glm::vec3(3,-3,-2), glm::vec3(0,6,6), glm::vec3(0.12f, 0.45f, 0.15f), 1.0f, MaterialType::LAMBERTIAN, -1.0f)),
		(Rect(glm::vec3(-3,-3,-2), glm::vec3(6,0,6), glm::vec3(0.8f, 0.8f, 0.8f), 1.0f, MaterialType::LAMBERTIAN, 1.0f)),
		(Rect(glm::vec3(-3, 3,-2), glm::vec3(6,0,6), glm::vec3(0.8f, 0.8f, 0.8f), 1.0f, MaterialType::LAMBERTIAN, -1.0f)),
		(Rect(glm::vec3(-2.0f, 2.99f,-1.0f), glm::vec3(4,0,4), glm::vec3(0.8f, 0.8f, 0.8f), glm::vec3(2.0f), 1.0f, MaterialType::LAMBERTIAN, -1.0f)),
		(Cuboid(glm::vec3(-2.5, -3, -1), glm::vec3(2,4,2), 20.0f, glm::vec3(0.8f), 0.1f, MaterialType::LAMBERTIAN)),
		(Volume<Cuboid>(0.4f, glm::vec3(0.0f, -2.99, 1), glm::vec3(2,2,2), -30.0f, glm::vec3(1.0f), 0.0f, MaterialType::ISOTROPIC)),
		(Sphere(glm::vec3(-2.5f, 1.5f, -1.0f), 0.5f, glm::vec3(1.0f), 1.5f, MaterialType::DIELECTRIC)),
		(Volume<Sphere>(0.7f, glm::vec3(-2.5f, 1.5f, -1.0f), 0.49f, glm::vec3(0.1f, 0.1f, 0.9f), 0.1f, MaterialType::ISOTROPIC))
	};

	/*const float rif = 0.5f / (float)INT_MAX;
	int index = 1302;
	for (int i = -5; i < 6; i++) {
		for (int j = -5; j < 6; j++) {
			if (i*i < 4 && j*j < 1) continue;
			if (init_rng[index++]*rif > 0.1)
				obj.emplace_back(glm::vec3{ i + init_rng[index++]*rif*0.5f - 0.25f, -0.25f, j + init_rng[index++] *rif*0.5f - 0.25f }, 0.25f, glm::vec3{ 0.8f *init_rng[index++] * rif,0.8f*init_rng[index++] *rif,0.8f*init_rng[index++]*rif }, init_rng[index++]*rif*0.7f + 0.3f, (init_rng[index++] % 2) ? MaterialType::LAMBERTIAN : MaterialType::METALLIC);
			else
				obj.emplace_back(glm::vec3{ i + init_rng[index++]*rif*0.5f - 0.25f, -0.25f, j + init_rng[index++] *rif*0.5f - 0.25f }, 0.25f, glm::vec3{ 1.0f }, 1.4f, MaterialType::DIELECTRIC);
		}
	} */

	compshdr.use();
	GLuint SSBO_objects;
	glGenBuffers(1, &SSBO_objects);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, SSBO_objects);
	glBufferData(GL_SHADER_STORAGE_BUFFER, obj.size() * sizeof(Sphere), obj.data(), GL_STATIC_DRAW);

	// Camera for the system.
	//Camera cam({ -4,3,4 }, { 0,0,0}, { 0,1,0 }, 30.0f, (float)WIDTH / (float)HEIGHT, 0.1f);
	Camera cam({ 0,0,16 }, { 0,0,0 }, { 0,1,0 }, 30.0f, (float)WIDTH / (float)HEIGHT);
	cam.Bind(compshdr);

	// Variables.
	int iteration = 0;
	bool wait_after_quit = false;
	double start = glfwGetTime();
	const int CHUNK_W = TEX_W / CHUNKS_X;
	const int CHUNK_H = TEX_H / CHUNKS_Y;
	const int N_CHUNKS = CHUNKS_X * CHUNKS_Y;
	compshdr.setBool("skybox_active", SKYBOX_ACTIVE);
	
	// Chunks must divide the screen properly.
	if (TEX_H%CHUNKS_Y != 0 || TEX_W % CHUNKS_X != 0) {
		std::cout << "ERR::TEX_CHUNKS_INDIVISIBLE" << std::endl;
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	double prev = start;\
		try {
		while (!glfwWindowShouldClose(window)) {

			// Compute shader dispatch.
			{
				int chunk_x, chunk_y;
				chunk_x = iteration % CHUNKS_X;
				chunk_y = (iteration / CHUNKS_X) % CHUNKS_Y;
				compshdr.use();
				compshdr.setInt("iteration", (iteration++ / N_CHUNKS));
				compshdr.setFloat("time", (float)glfwGetTime());
				compshdr.setVector("chunk", glm::ivec2(chunk_x*CHUNK_W, chunk_y * CHUNK_H));
				glDispatchCompute((GLuint)(TEX_W / CHUNKS_X), (GLuint)(TEX_H / CHUNKS_Y), 1);
			}

			glfwPollEvents();
			if (glfwGetKey(window, GLFW_KEY_SPACE) || glfwGetKey(window, GLFW_KEY_ESCAPE) || glfwGetKey(window, GLFW_KEY_ENTER)) {
				glfwSetWindowShouldClose(window, GLFW_TRUE);
				if (glfwGetKey(window, GLFW_KEY_ENTER)) {
					wait_after_quit = true;
				}
			}

			// Rendering Code
			{
				glClear(GL_COLOR_BUFFER_BIT);
				drawshdr.use();
				glBindVertexArray(VAO);
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}
			glfwSwapBuffers(window);

			if (iteration%N_CHUNKS == 0) {

				// Book keeping 
				double time = glfwGetTime();
				std::cout << "Number of iterations: " << std::setw(4) << iteration / N_CHUNKS << " FPS: " << std::setw(7) << iteration / (N_CHUNKS * (time - start)) << " Delta: " << std::setw(7) << time - prev << "\t\t\t\r";
				prev = time;
			}
		}
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	std::cout << std::endl;

	// Cleanup
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &SSBO_rng);
	glDeleteBuffers(1, &SSBO_objects);
	glDeleteTextures(1, &tex_output);
	glDeleteVertexArrays(1, &VAO);

	glfwDestroyWindow(window);
	glfwTerminate();

	if (wait_after_quit) {
		std::cin.ignore();
	}

	return 0;
}