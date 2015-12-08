#pragma once

#include <cstdio>
#include <cuda.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cmath>
#include <thrust/execution_policy.h>
#include <thrust/random.h>
#include <thrust/remove.h>

#include "scene.h"
#include "interactions.h"

#define DI 0
#define DOF 0
#define SHOW_TIMING 0
#define ERRORCHECK 1

#define FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define checkCUDAError(msg) checkCUDAErrorFn(msg, FILENAME, __LINE__)

class CUDARenderer{
	public:
		CUDARenderer();
		~CUDARenderer();
		void pathtraceInit(Scene* scene, int rendererNo_, int totalRenderer_);
		void pathtraceFree();
		void pathtrace(uchar4 *pbo, int frame, int iteration);
		int getPixelCount();

	private:
		bool active;

		Scene* hst_scene;
		glm::vec3 *dev_image;
		Camera *dev_camera;
		Geom *dev_geoms;
		int* dev_geoms_count;
		MeshGeom *dev_meshes;
		int *dev_meshes_count;
		Material *dev_materials;
		RenderState *dev_state;
		RayState *dev_rays_begin;
		RayState *dev_rays_end;
		int *dev_light_indices;
		int *dev_light_count;

		int rendererNo;
		int totalRenderer;
		int pixelcount;
		
		void copyMeshes();
};

void checkCUDAErrorFn(const char *msg, const char *file, int line);

__host__ __device__ thrust::default_random_engine makeSeededRandomEngine(int iter, int index, int depth);

__global__ void sendImageToPBO(uchar4* pbo, glm::ivec2 resolution, int iter, glm::vec3* image,
	int rendererNo, int totalRenderer);

__global__ void kernGetRayDirections(Camera * camera, RayState* rays, int iter, int rendererNo, 
	int totalRenderer);

__global__ void kernJitterDOF(Camera * camera, RayState* rays, int iter, int rendererNo, int totalRenderer);

__global__ void kernTracePath(Camera * camera, RayState *ray, Geom * geoms, int *geomCount,
	MeshGeom *meshGeoms, int *meshCount, int* lightIndices, int *lightCount, Material* materials,
	glm::vec3* image, int iter, int currDepth, int rayCount);

__global__ void kernDirectLightPath(Camera * camera, RayState *ray, Geom * geoms, int * lightIndices, 
	int* lightCount, glm::vec3* image, int iter, int currDepth, int rayCount);

__global__ void kernWritePixels(Camera * camera, RayState *ray, glm::vec3* image, int rayCount);

__global__ void kernRussianRoullete(Camera * camera, RayState *ray, glm::vec3* image, int iter,
	int rayCount);

struct isDead
{
	__host__ __device__ bool  operator()(const RayState r)
	{
		return (!r.isAlive);
	}
};


