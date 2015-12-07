#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>

#include "sceneStructs.h"
#include "utilities.h"

__host__ __device__ unsigned int utilhash(unsigned int a);

__host__ __device__ glm::vec3 getPointOnRay(Ray r, float t);

__host__ __device__ glm::vec3 multiplyMV(glm::mat4 m, glm::vec4 v);

__host__ __device__ float boxIntersectionTest(Geom box, Ray r,
	glm::vec3 &intersectionPoint, glm::vec3 &normal, bool outside = false);

__host__ __device__ float sphereIntersectionTest(Geom sphere, Ray r,
	glm::vec3 &intersectionPoint, glm::vec3 &normal, bool outside = false);

__host__ __device__ float meshIntersectionTest(Geom g, MeshGeom &m, Ray r,
	glm::vec3 &intersectionPoint, glm::vec3 &normal, bool outside = false);