#include "CUDARenderer.h"

CUDARenderer::CUDARenderer(){
	active = false;
}

CUDARenderer::~CUDARenderer(){
	if (active)
		pathtraceFree();
};

void CUDARenderer::pathtraceInit(Scene* scene, int rendererNo_, int totalRenderer_)
{
	active = true;

	hst_scene = scene;
	const Camera &cam = hst_scene->state.camera;

	rendererNo = rendererNo_;
	totalRenderer = totalRenderer_;

	pixelcount = (cam.resolution.x * cam.resolution.y) / totalRenderer;
	if (cam.resolution.x * cam.resolution.y % totalRenderer > rendererNo)
		pixelcount++;
		
	std::cout << "Pixel to be rendered: " << pixelcount << std::endl;

	//2D Pixel array to store image color
	cudaMalloc(&dev_image, pixelcount * sizeof(glm::vec3));
	cudaMemset(dev_image, 0, pixelcount * sizeof(glm::vec3));

	//Copy Camera
	cudaMalloc((void**)&dev_camera, sizeof(Camera));
	cudaMemcpy(dev_camera, &hst_scene->state.camera, sizeof(Camera), cudaMemcpyHostToDevice);

	//Copy geometry count
	int geom_count = hst_scene->geoms.size();
	cudaMalloc((void**)&dev_geoms_count, sizeof(int));
	cudaMemcpy(dev_geoms_count, &geom_count, sizeof(int), cudaMemcpyHostToDevice);

	//Copy geometry
	cudaMalloc((void**)&dev_geoms, geom_count * sizeof(Geom));
	cudaMemcpy(dev_geoms, hst_scene->geoms.data(), geom_count * sizeof(Geom), cudaMemcpyHostToDevice);

	copyMeshes();

	//Copy material
	cudaMalloc((void**)&dev_materials, hst_scene->materials.size() * sizeof(Material));
	cudaMemcpy(dev_materials, hst_scene->materials.data(), hst_scene->materials.size() * sizeof(Material), cudaMemcpyHostToDevice);

	//Copy state
	cudaMalloc((void**)&dev_state, sizeof(RenderState));
	cudaMemcpy(dev_state, &hst_scene->state, sizeof(RenderState), cudaMemcpyHostToDevice);

	//Allocate memory for rays
	cudaMalloc((void**)&dev_rays_begin, pixelcount * sizeof(RayState));
	//    cudaMalloc((void**)&dev_rays_end, sizeof(RayState));

	//Copy Light Indices
	cudaMalloc((void**)&dev_light_indices, hst_scene->state.lightIndices.size() * sizeof(int));
	cudaMemcpy(dev_light_indices, hst_scene->state.lightIndices.data(), hst_scene->state.lightIndices.size() * sizeof(int), cudaMemcpyHostToDevice);

	//Copy Light Count
	int lightCount = hst_scene->state.lightIndices.size();
	cudaMalloc((void**)&dev_light_count, sizeof(int));
	cudaMemcpy(dev_light_count, &lightCount, sizeof(int), cudaMemcpyHostToDevice);

	checkCUDAError("pathtraceInit");
}

void CUDARenderer::pathtraceFree(){
	cudaFree(dev_image);
	// TODO: clean up the above static variables

	cudaFree(dev_camera);
	cudaFree(dev_geoms);
	cudaFree(dev_geoms_count);
	cudaFree(dev_meshes);
	cudaFree(dev_meshes_count);
	cudaFree(dev_materials);
	cudaFree(dev_state);
	cudaFree(dev_rays_begin);
	//    cudaFree(dev_rays_end);
	cudaFree(dev_light_indices);
	cudaFree(dev_light_count);

	active = false;
}


void CUDARenderer::pathtrace(uchar4 *pbo, int frame, int iter) {
	const int traceDepth = hst_scene->state.traceDepth;
	const Camera &cam = hst_scene->state.camera;

	int blockSize = 64;
	int blocksPerGrid = glm::ceil(pixelcount / (float)blockSize);

	///////////////////////////////////////////////////////////////////////////

	// Recap:
	// * Initialize array of path rays (using rays that come out of the camera)
	//   * You can pass the Camera object to that kernel.
	// * For each depth:
	//   * Compute one new (ray, color) pair along each path (using scatterRay).
	//     Note that many rays will terminate by hitting a light or hitting
	//     nothing at all. You'll have to decide how to represent your path rays
	//     and how you'll mark terminated rays.
	//   * Add all of the terminated rays' results into the appropriate pixels.
	//   * Stream compact away all of the terminated paths.
	//     You may use your implementation or `thrust::remove_if` or its
	//     cousins.
	// * Finally, handle all of the paths that still haven't terminated.
	//   (Easy way is to make them black or background-colored.)

	// TODO: perform one iteration of path tracing

	//Setup initial rays
	kernGetRayDirections << < blocksPerGrid, blockSize >> >(dev_camera, dev_rays_begin, iter, 
		rendererNo, totalRenderer);
	checkCUDAError("kernGetRayDirections");

	//Jitter rays as per Depth of field
	if (DOF)
	{
		kernJitterDOF << <blocksPerGrid, blockSize >> >(dev_camera, dev_rays_begin, iter,
			rendererNo, totalRenderer);
		checkCUDAError("kernJitterDOF");
	}

	dev_rays_end = dev_rays_begin + pixelcount;
	int rayCount = pixelcount;
	int numBlocks, numThreads = 128;

	for (int i = 0; (i<traceDepth && rayCount > 0); ++i)	//For Path Tracing
		//for (int i = 0; i<1; ++i)							//For DI
	{
		//Calculate new number of blocks
		numBlocks = (rayCount + numThreads - 1) / numThreads;

		//    	cudaEvent_t start, stop;
		//    	cudaEventCreate(&start);
		//    	cudaEventCreate(&stop);
		//    	cudaEventRecord(start);

		//Take one step, should make dead rays as false
		//std::cout << i << " tracedepth: " << traceDepth << " / raycount: " << rayCount << std::endl;
		kernTracePath << <numBlocks, numThreads >> >(dev_camera, dev_rays_begin, dev_geoms, dev_geoms_count, dev_meshes, dev_meshes_count, dev_light_indices, dev_light_count, dev_materials, dev_image, iter, i, rayCount);
		checkCUDAError("kernTracePath");

		//If currDepth is > 2, play russian roullete
		if (i > 2)
		{
			kernRussianRoullete << <numBlocks, numThreads >> >(dev_camera, dev_rays_begin, dev_image, iter, rayCount);
			checkCUDAError("kernRussianRoullete");
		}

		// Compact rays, dev_rays_end points to the new end
		dev_rays_end = thrust::remove_if(thrust::device, dev_rays_begin, dev_rays_end, isDead());
		rayCount = dev_rays_end - dev_rays_begin;

		//    	cudaEventRecord(stop);
		//    	cudaEventSynchronize(stop);
		//    	float milliseconds = 0;
		//    	cudaEventElapsedTime(&milliseconds, start, stop);
		//    	if(SHOW_TIMING)
		//    		std::cout<</*"Iter : "<<iter<<" Depth : "<<i<<" Total time in milliseconds : "<<*/milliseconds<<std::endl;
	}

	//std::cout << i << std::endl;
	if (rayCount > 0)
	{
		kernWritePixels << <numBlocks, numThreads >> >(dev_camera, dev_rays_begin, dev_image, rayCount);
		checkCUDAError("kernWritePixels");
	}

	//Direct Illumination
	if (DI && rayCount > 0)
	{
		kernDirectLightPath << <numBlocks, numThreads >> >(dev_camera, dev_rays_begin, dev_geoms, dev_light_indices, dev_light_count, dev_image, iter, traceDepth, rayCount);
		checkCUDAError("kernDirectLightPath");
	}

	// Send results to OpenGL buffer for rendering
	sendImageToPBO << <blocksPerGrid, blockSize >> >(pbo, cam.resolution, iter, dev_image, rendererNo, totalRenderer);
	checkCUDAError("sendImageToPBO");

	// Retrieve image from GPU
	cudaMemcpy(hst_scene->state.image.data(), dev_image,
		pixelcount * sizeof(glm::vec3), cudaMemcpyDeviceToHost);

	checkCUDAError("pathtrace");
}

int CUDARenderer::getPixelCount(){
	return pixelcount;
}

void CUDARenderer::copyMeshes()
{
	//Copy meshes count
	int mesh_count = hst_scene->meshGeoms.size();

	MeshGeom *allMeshes = new MeshGeom[mesh_count];

	cudaMalloc((void**)&dev_meshes_count, sizeof(int));
	cudaMemcpy(dev_meshes_count, &mesh_count, sizeof(int), cudaMemcpyHostToDevice);

	for (int i = 0; i < mesh_count; ++i)
	{
		/*meshes[i] = hst_scene->meshGeoms[i];
		meshes[i].numVertices = hst_scene->meshGeoms[i].numVertices;*/
		MeshGeom meshes;

		meshes = hst_scene->meshGeoms[i];
		meshes.numVertices = hst_scene->meshGeoms[i].numVertices;

		glm::vec3 *triangles, *normals;

		cudaMalloc(&triangles, meshes.numVertices * sizeof(glm::vec3));
		cudaMalloc(&normals, meshes.numVertices * sizeof(glm::vec3));

		cudaMemcpy(triangles, hst_scene->meshGeoms[i].triangles, meshes.numVertices * sizeof(glm::vec3), cudaMemcpyHostToDevice);
		cudaMemcpy(normals, hst_scene->meshGeoms[i].normals, meshes.numVertices * sizeof(glm::vec3), cudaMemcpyHostToDevice);

		meshes.normals = normals;
		meshes.triangles = triangles;

		allMeshes[i] = meshes;
		/*meshes[i].normals = normals;
		meshes[i].triangles = triangles;*/
	}

	cudaMalloc((void**)&dev_meshes, mesh_count * sizeof(MeshGeom));
	cudaMemcpy(dev_meshes, allMeshes, mesh_count * sizeof(MeshGeom), cudaMemcpyHostToDevice);
}

void checkCUDAErrorFn(const char *msg, const char *file, int line) {
	cudaError_t err = cudaGetLastError();
	if (cudaSuccess == err) {
		return;
	}

	fprintf(stderr, "CUDA error");
	if (file) {
		fprintf(stderr, " (%s:%d)", file, line);
	}
	fprintf(stderr, ": %s: %s\n", msg, cudaGetErrorString(err));
	exit(EXIT_FAILURE);
}

__host__ __device__ thrust::default_random_engine makeSeededRandomEngine(int iter, int index, int depth) {
	int h = utilhash((1 << 31) | (depth << 22) | iter) ^ utilhash(index);
	return thrust::default_random_engine(h);
}

//Kernel that writes the image to the OpenGL PBO directly.
__global__ void sendImageToPBO(uchar4* pbo, glm::ivec2 resolution, int iter, glm::vec3* image,
	int rendererNo, int totalRenderer) 
{
	int ptr = (blockIdx.x * blockDim.x) + threadIdx.x;
	int index = rendererNo + ptr * totalRenderer;

	if (index < resolution.x * resolution.y) {
		glm::vec3 pix = image[ptr];

		glm::ivec3 color;
		color.x = glm::clamp((int)(pix.x / iter * 255.0), 0, 255);
		color.y = glm::clamp((int)(pix.y / iter * 255.0), 0, 255);
		color.z = glm::clamp((int)(pix.z / iter * 255.0), 0, 255);

		// Each thread writes one pixel location in the texture (textel)
		pbo[index].w = 0;
		pbo[index].x = color.x;
		pbo[index].y = color.y;
		pbo[index].z = color.z;
	}
}

//Kernel function that gets all the ray directions
__global__ void kernGetRayDirections(Camera * camera, RayState* rays, int iter, 
	int rendererNo, int totalRenderer)
{
	int ptr = (blockIdx.x * blockDim.x) + threadIdx.x;
	int index = rendererNo + ptr * totalRenderer;

	if (index < camera->resolution.x * camera->resolution.y)
	{
		//TODO : Tweak the random variable here if the image looks fuzzy
		thrust::default_random_engine rng = makeSeededRandomEngine(iter, index, 0);
		thrust::uniform_real_distribution<float> u01(0, 0.005);

		//Find the ray direction
		int p = index / camera->resolution.y;
		float sy = float(p) / ((float)(camera->resolution.y) - 1.0f);

		p = index - (p * camera->resolution.y);
		float sx = float(p) / ((float)(camera->resolution.x) - 1.0f);

		glm::vec3 rayDir = (camera->M - (2.0f*sx - 1.0f + u01(rng)) * camera->H - (2.0f*sy - 1.0f + u01(rng)) * camera->V);
		//		glm::vec3 rayDir = (camera->M - (2.0f*sx - 1.0f) * camera->H - (2.0f*sy - 1.0f) * camera->V);

		rayDir -= camera->position;
		rayDir = glm::normalize(rayDir);

		rays[ptr].ray.direction = rayDir;
		rays[ptr].ray.origin = camera->position;
		rays[ptr].isAlive = true;
		rays[ptr].rayColor = glm::vec3(0);
		rays[ptr].pixelIndex = ptr;
		rays[ptr].rayThroughPut = 1.0f;

		//		printf("%d %d : %f %f %f\n", x, y, rayDir.x, rayDir.y, rayDir.z);
	}
}

//Kernel function that generates the Depth of field jitter
__global__ void kernJitterDOF(Camera * camera, RayState* rays, int iter,
	int rendererNo, int totalRenderer)
{
	int ptr = (blockIdx.x * blockDim.x) + threadIdx.x;
	int index = rendererNo + ptr * totalRenderer;

	if (index < camera->resolution.x * camera->resolution.y)
	{
		Ray &r = rays[ptr].ray;

		glm::vec3 intersectionPoint, normal;

		sphereIntersectionTest(camera->camSphere, r, intersectionPoint, normal);//, outside);

		thrust::default_random_engine rng = makeSeededRandomEngine(iter, index, 0);
		thrust::uniform_real_distribution<float> u01(-0.5, 0.5);

		r.origin += glm::vec3(camera->aperture * u01(rng), camera->aperture * u01(rng), 0);
		r.direction = glm::normalize(intersectionPoint - r.origin);
	}
}


//Kernel function that performs one iteration of tracing the path.
__global__ void kernTracePath(Camera * camera, RayState *ray, Geom * geoms, int *geomCount, MeshGeom *meshGeoms, int *meshCount, int* lightIndices, int *lightCount, Material* materials, glm::vec3* image, int iter, int currDepth, int rayCount)
{
	int index = (blockIdx.x * blockDim.x) + threadIdx.x;

	if (index < rayCount)
	{
		if (ray[index].isAlive)
		{
			glm::vec3 intersectionPoint = glm::vec3(0), normal = glm::vec3(0);
			float min_t = FLT_MAX, t;
			RayState &r = ray[index];
			int nearestIndex = -1;
			glm::vec3 nearestIntersectionPoint = glm::vec3(0), nearestNormal = glm::vec3(0);
			bool outside = false;

			//Find geometry intersection
			for (int i = 0; i<(*geomCount); ++i)
			{
				if (geoms[i].type == CUBE)
				{
					t = boxIntersectionTest(geoms[i], r.ray, intersectionPoint, normal, outside);
				}

				else if (geoms[i].type == SPHERE)
				{
					t = sphereIntersectionTest(geoms[i], r.ray, intersectionPoint, normal, outside);
				}

				else if (geoms[i].type == MESH)
				{
					t = meshIntersectionTest(geoms[i], meshGeoms[geoms[i].meshid], r.ray, intersectionPoint, normal, outside);
				}

				if (t > 0 && t < min_t)
				{
					min_t = t;
					nearestIntersectionPoint = intersectionPoint;
					nearestIndex = i;
					nearestNormal = normal;
				}
			}

			//If the nearest index remains unchanged, means no intersection and we can kill the ray.
			if (nearestIndex == -1)
			{
				r.isAlive = false;

				//Write the accumulated color for that pixel.
				image[r.pixelIndex] += r.rayColor;
			}

			//else find the material color
			else
			{
				//If light source
				if (materials[geoms[nearestIndex].materialid].emittance >= 1)
				{
					//Light source, end ray here
					r.isAlive = false;

					//If this is the primary ray, write the light color
					if (currDepth == 0)
					{
						image[r.pixelIndex] += materials[geoms[nearestIndex].materialid].emittance
							* materials[geoms[nearestIndex].materialid].color;
					}
					//Else write the accumulated color
					else
					{
						image[r.pixelIndex] += (r.rayColor);
					}
				}

				else
				{
					thrust::default_random_engine rng = makeSeededRandomEngine(iter, index, currDepth);

					getRayColor(camera->position,
						r,
						nearestIntersectionPoint,
						nearestNormal,
						outside,
						materials,
						rng,
						geoms,
						nearestIndex,
						geomCount,
						meshGeoms,
						meshCount,
						lightIndices,
						lightCount);


					/*scatterRay(camera->position,
					r,
					nearestIntersectionPoint,
					nearestNormal,
					materials[geoms[nearestIndex].materialid],
					rng,
					geoms,
					nearestIndex,
					lightIndices,
					lightCount);*/

					//TODO: Remove next line for path tracing
					//image[r.pixelIndex] += r.rayColor;
				}
			}
		}
	}
}

__global__ void kernDirectLightPath(Camera * camera, RayState *ray, Geom * geoms, int * lightIndices, int* lightCount, glm::vec3* image, int iter, int currDepth, int rayCount)
{
	int index = (blockIdx.x * blockDim.x) + threadIdx.x;

	if (index < rayCount)
	{
		if (ray[index].isAlive)
		{
			glm::vec3 intersectionPoint, normal;
			float t;

			RayState &r = ray[index];
			int i;
			//bool outside;
			thrust::default_random_engine rng = makeSeededRandomEngine(iter, index, currDepth);

			glm::vec3 pointOnLight = getRandomPointOnLight(geoms, lightIndices, lightCount, rng, i);

			r.ray.direction = glm::normalize(pointOnLight - r.ray.origin);
			t = sphereIntersectionTest(geoms[i], r.ray, intersectionPoint, normal);

			if (t > 0)
			{
				//Intersection with light, write the color
				image[r.pixelIndex] += r.rayColor;

				/*image[r.pixelIndex] += (r.rayColor
				* materials[geoms[i].materialid].emittance
				* materials[geoms[i].materialid].color);*/
			}
		}
	}
}


__global__ void kernWritePixels(Camera * camera, RayState *ray, glm::vec3* image, int rayCount)
{
	int index = (blockIdx.x * blockDim.x) + threadIdx.x;

	if (index < rayCount)
	{
		if (ray[index].isAlive)
		{
			image[ray[index].pixelIndex] += ray[index].rayColor;
		}
	}
}

__global__ void kernRussianRoullete(Camera * camera, RayState *ray, glm::vec3* image, int iter, int rayCount)
{
	int index = (blockIdx.x * blockDim.x) + threadIdx.x;

	if (index < rayCount)
	{
		if (ray[index].isAlive)
		{
			thrust::default_random_engine rng = makeSeededRandomEngine(iter, index, 0);
			thrust::uniform_real_distribution<float> u01(0, 1.0f);

			if (ray[index].rayThroughPut < u01(rng))
			{
				ray[index].isAlive = false;
				image[ray[index].pixelIndex] += ray[index].rayColor;
			}
		}
	}
}
