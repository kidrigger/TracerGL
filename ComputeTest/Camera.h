#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"

class Camera
{
private:
	glm::vec3 lower_left_corner;
	glm::vec3 horz;
	glm::vec3 vert;
	glm::vec3 origin;
	float lens_radius;
	float focal_distance;
public:
	Camera(glm::vec3 lookFrom, glm::vec3 lookAt, glm::vec3 up, float vfov, float aspect, float aperture = 0, float focal_length = -1);

	void Bind(Shader<ShaderType::COMPUTE>& shdr) {
		shdr.use();
		shdr.setVector("cam.origin", origin);
		shdr.setVector("cam.lower_left", lower_left_corner);
		shdr.setVector("cam.horz", horz);
		shdr.setVector("cam.vert", vert);
		shdr.setFloat("cam.lens_radius", lens_radius);
	}

};

