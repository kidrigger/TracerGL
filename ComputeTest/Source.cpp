#include <iostream>
#include "glad/glad.h"

#include "GLFW/glfw3.h"

#include "Shader.h"

int main() {

	GLFWwindow* window = nullptr;

	const int WIDTH = 1024;
	const int HEIGHT = 512;

	if (!glfwInit()) {
		std::cerr << "ERR::GLFW::INIT_FAIL" << std::endl;
		std::cin.ignore();
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "N-Body", nullptr, nullptr);
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

	glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

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

	Shader<ShaderType::COMPUTE> compshdr("raycompute.comp");
	Shader<ShaderType::RENDER> drawshdr("vDraw.vert", "fDraw.frag");

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

	while (!glfwWindowShouldClose(window)) {

		{
			compshdr.use();
			/*compshdr.setVector("leftBottom", glm::vec3{ -1.0f, -1.0f, 0.0f });
			compshdr.setVector("rightTop", glm::vec3{ 1.0f, 1.0f, 0.0f });*/
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
		}
		glfwSwapBuffers(window);
		
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	std::cin.ignore();

	return 0;
}