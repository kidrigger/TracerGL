#include <iostream>
#include "glad/glad.h"

#include "GLFW/glfw3.h"

#include "Shader.h"
#include "Camera.h"
#include "ShaderStructs.h"
#include <random>
 
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

int main() {

	GLFWwindow* window = nullptr;

	const int WIDTH = 1920;
	const int HEIGHT = 1080;

	if (!glfwInit()) {
		std::cerr << "ERR::GLFW::INIT_FAIL" << std::endl;
		std::cin.ignore();
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

	GLFWmonitor* monitor = nullptr;
	monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	if (mode->height != HEIGHT || mode->width != WIDTH) {
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

	std::default_random_engine rng;
	std::uniform_int_distribution<uint32_t> distr;
	// SSBO for rng
	std::vector<GLuint> init_rng;
	init_rng.resize(WIDTH * HEIGHT);
	for (GLuint& i : init_rng) {
		i = distr(rng);
	}
	compshdr.use();
	GLuint SSBO_rng;
	glGenBuffers(1, &SSBO_rng);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, SSBO_rng);
	glBufferData(GL_SHADER_STORAGE_BUFFER, init_rng.size() * sizeof(GLuint), init_rng.data(), GL_DYNAMIC_DRAW);

	// SSBO for the spheres.
	std::vector<Sphere> obj{ { {0, 0, 0}	, 0.5f	, {0.1f, 0.2f, 0.5f}, 1.0f, MaterialType::LAMBERTIAN},
							{ {0,-100.5,0}	, 100.0f, {0.9f, 0.9f, 0.9f}, 0.0f, MaterialType::LAMBERTIAN},
							{ {1,0,0}		, 0.5f	, {0.8f, 0.6f, 0.2f}, 0.5f, MaterialType::METALLIC	},
							{ {-1,0,0}		, 0.5f	, {1.0f, 1.0f, 1.0f}, 1.5f, MaterialType::DIELECTRIC},
							{ {-1,0,0}		, -0.48f, {1.0f, 1.0f, 1.0f}, 1.5f, MaterialType::DIELECTRIC} };


	const float rif = 0.5f / (float)INT_MAX;
	int index = 1302;
	for (int i = -5; i < 6; i++) {
		for (int j = -5; j < 6; j++) {
			if (i*i < 4 && j*j < 1) continue;
			if (init_rng[index++]*rif > 0.1)
				obj.emplace_back(glm::vec3{ i + init_rng[index++]*rif*0.5f - 0.25f, -0.25f, j + init_rng[index++] *rif*0.5f - 0.25f }, 0.25f, glm::vec3{ 0.8f *init_rng[index++] * rif,0.8f*init_rng[index++] *rif,0.8f*init_rng[index++]*rif }, init_rng[index++]*rif*0.7f + 0.3f, (init_rng[index++] % 2) ? MaterialType::LAMBERTIAN : MaterialType::METALLIC);
			else
				obj.emplace_back(glm::vec3{ i + init_rng[index++]*rif*0.5f - 0.25f, -0.25f, j + init_rng[index++] *rif*0.5f - 0.25f }, 0.25f, glm::vec3{ 1.0f }, 1.4f, MaterialType::DIELECTRIC);
		}
	} 

	compshdr.use();
	GLuint SSBO_objects;
	glGenBuffers(1, &SSBO_objects);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, SSBO_objects);
	glBufferData(GL_SHADER_STORAGE_BUFFER, obj.size() * sizeof(Sphere), obj.data(), GL_STATIC_DRAW);

	// Camera for the system.
	Camera cam({ -4,3,4 }, { 0,0,0}, { 0,1,0 }, 30.0f, (float)WIDTH / (float)HEIGHT, 0.1f);
	cam.Bind(compshdr);

	// Variables.
	int iteration = 0;
	bool wait_after_quit = false;
	double start = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {

		// Compute shader dispatch.
		{
			compshdr.use();
			compshdr.setInt("iteration",iteration++);
			compshdr.setFloat("time", (float)glfwGetTime());
			glDispatchCompute((GLuint)TEX_W, (GLuint)TEX_H, 1);
		}

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		{
			glClear(GL_COLOR_BUFFER_BIT);
			drawshdr.use();
			glBindVertexArray(VAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_output);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_SPACE) || glfwGetKey(window, GLFW_KEY_ESCAPE) || glfwGetKey(window, GLFW_KEY_ENTER)) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			if (glfwGetKey(window, GLFW_KEY_ENTER)) {
				wait_after_quit = true;
			}
		}
		glfwSwapBuffers(window);
		std::cout << "Number of iterations: " << iteration << " FPS: " << iteration/(glfwGetTime() - start) << "\t\r";
		
	}
	std::cout << std::endl;


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