#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum class MaterialType : uint32_t {
	LAMBERTIAN	= 0x00000001u,
	METALLIC	= 0x00000002u,
	DIELECTRIC	= 0x00000003u,
	ISOTROPIC	= 0x00000004u
};

enum class ShapeType : uint32_t {
	SPHERE		= 0x00000001u,
	CUBOID		= 0x00000002u,
	RECT		= 0x00000003u,
	ISOTROPIC	= 0xF0000000u,
	ISO_SPHERE	= SPHERE | ISOTROPIC,
	ISO_CUBOID	= CUBOID | ISOTROPIC
};

struct Shape {
	float A[3];
	uint32_t shape_type;
	float B[3];
	float rotation;
	float params[4];
	float C[3];
	float param;
	float D[3];
	uint32_t mat_type;
};

struct Sphere {
	float position[3];
	uint32_t shape_type;
	float radius[3];
	uint32_t __padd1[1];
	float dense;
	float __padd2[3];
	float color[3];
	float param;
	float emit[3];
	uint32_t material_type;

	Sphere(const glm::vec3& pos, float rad, const glm::vec3& col, float param, MaterialType type = MaterialType::LAMBERTIAN) : Sphere(pos, rad, col, glm::vec3{ 0.0f }, param, type) {
	}

	Sphere(const glm::vec3& pos, float rad, const glm::vec3& col, const glm::vec3& emissive, float param, MaterialType type = MaterialType::LAMBERTIAN) :
		param(param),
		material_type(static_cast<uint32_t>(type)),
		shape_type(static_cast<uint32_t>(ShapeType::SPHERE)) {
		position[0] = pos.x;
		position[1] = pos.y;
		position[2] = pos.z;
		radius[0] = rad;
		radius[1] = rad;
		radius[2] = rad;
		color[0] = col.r;
		color[1] = col.g;
		color[2] = col.b;
		emit[0] = emissive.r;
		emit[1] = emissive.g;
		emit[2] = emissive.b;
	}

	operator Shape() {
		return *(Shape*)this;
	}
};

struct Cuboid {
	float position[3];
	uint32_t shape_type;
	float diagonal[3];
	uint32_t rotation;
	float dense;
	float __padd2[3];
	float color[3];
	float param;
	float emit[3];
	uint32_t material_type;

	Cuboid(const glm::vec3& pos, const glm::vec3& dia, const glm::vec3& col, float param, MaterialType type) : Cuboid(pos, dia, 0, col, glm::vec3{ 0.0f }, param, type) {

	}

	Cuboid(const glm::vec3& pos, const glm::vec3& dia, float rot, const glm::vec3& col, float param, MaterialType type) : Cuboid(pos, dia, rot, col, glm::vec3{ 0.0f }, param, type) {

	}
	Cuboid(const glm::vec3& pos, const glm::vec3& dia, float rot, const glm::vec3& col, const glm::vec3& emissive, float param, MaterialType type) :
		rotation((uint32_t)(glm::radians(rot+360)*1000.0)),
		param(param),
		material_type(static_cast<uint32_t>(type)),
		shape_type(static_cast<uint32_t>(ShapeType::CUBOID)) {
		position[0] = pos.x;
		position[1] = pos.y;
		position[2] = pos.z;
		diagonal[0] = dia.x;
		diagonal[1] = dia.y;
		diagonal[2] = dia.z;
		color[0] = col.r;
		color[1] = col.g;
		color[2] = col.b;
		emit[0] = emissive.r;
		emit[1] = emissive.g;
		emit[2] = emissive.b;
	}

	operator Shape() {
		return *(Shape*)this;
	}
};

struct Rect {
	float position[3];
	uint32_t shape_type;
	float diagonal[3];
	uint32_t axis;
	float __padd2[4];
	float color[3];
	float param;
	float emit[3];
	uint32_t mat_type;

	Rect(const glm::vec3& pos, const glm::vec3& dia, const glm::vec3& col, float param, MaterialType type, float normal_dir = 1.0f) : Rect(pos, dia, col, glm::vec3{ 0.0f }, param, type, normal_dir) {

	}
	Rect(const glm::vec3& pos, const glm::vec3& dia, const glm::vec3& col, const glm::vec3& emissive, float param, MaterialType type, float normal_dir) :
		param(param),
		mat_type(static_cast<uint32_t>(type)),
		shape_type(static_cast<uint32_t>(ShapeType::RECT)) {
		position[0] = pos.x;
		position[1] = pos.y;
		position[2] = pos.z;
		diagonal[0] = dia.x;
		diagonal[1] = dia.y;
		diagonal[2] = dia.z;

		if (dia.x*dia.x < 0.00000001f) {
			axis = 0u;
		} else if (dia.y*dia.y < 0.00000001f) {
			axis = 1u;
		} else {
			axis = 2u;
		}

		diagonal[axis] = normal_dir;

		color[0] = col.r;
		color[1] = col.g;
		color[2] = col.b;
		emit[0] = emissive.r;
		emit[1] = emissive.g;
		emit[2] = emissive.b;

	}

	operator Shape() {
		return *(Shape*)this;
	}
};

template <class T> 
struct Volume : public T {
public:
	template <typename... Ts>
	Volume(float density, Ts... args) : T(std::forward<Ts>(args)...) {
		T::dense = density;
		T::shape_type = static_cast<uint32_t>(ShapeType::ISOTROPIC) | static_cast<uint32_t>(T::shape_type);
	}

	operator Shape() {
		return *(Shape*)this;
	}
};