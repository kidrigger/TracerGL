#include "Camera.h"

Camera::Camera(glm::vec3 lookFrom, glm::vec3 lookAt, glm::vec3 up, float vfov, float aspect, float aperture, float focal_length) : focal_distance(focal_length) {
	float theta = glm::radians(vfov);
	float half_height = tan(theta * 0.5f);
	float half_width = aspect * half_height;

	lens_radius = aperture * 0.5f;
	if (focal_distance < 0.0f) {
		focal_distance = glm::distance(lookFrom, lookAt);
	}

	origin = lookFrom;

	glm::vec3 w = glm::normalize(lookFrom - lookAt);
	glm::vec3 u = glm::normalize(glm::cross(up, w));
	glm::vec3 v = glm::cross(w, u);

	lower_left_corner = origin - (half_width * u + half_height * v + w) * focal_distance;
	horz = 2.0f * half_width * focal_distance * u;
	vert = 2.0f * half_height * focal_distance * v;
}