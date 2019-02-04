#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum class MaterialType : uint32_t {
	LAMBERTIAN = 0x00000001u,
	METALLIC = 0x00000002u,
	DIELECTRIC = 0x00000003u
};


struct Sphere {
	float position[3];
	float radius;
	float color[3];
	float param;
	uint32_t type;
	uint32_t __padding[3];

	Sphere(const glm::vec3& pos, float radius, const glm::vec3& col, float param, MaterialType type) :
			radius(radius), 
			param(param), 
			type(static_cast<uint32_t>(type)) {
		position[0] = pos.x;
		position[1] = pos.y;
		position[2] = pos.z;
		color[0] = col.r;
		color[1] = col.g;
		color[2] = col.b;
	}
};
