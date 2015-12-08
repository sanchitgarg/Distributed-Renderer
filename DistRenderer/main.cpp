
#include <glm/gtx/transform.hpp>

#include "CUDARenderer.h"
#include "preview.h"

void mainLoop(CUDARenderer *renderer, Viewer* viewer, int maxIteration);

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("Usage: %s SCENEFILE.txt\n", argv[0]);
		return 1;
	}

	const char *sceneFile = argv[1];

	// Load scene file
	Scene scene(sceneFile);
	Viewer viewer(&scene);
	CUDARenderer renderer;
	renderer.pathtraceInit(&scene, 0, 10);

	int maxIteration = scene.state.iterations;

	mainLoop(&renderer, &viewer, maxIteration);

	return 0;
}

void mainLoop(CUDARenderer *renderer, Viewer* viewer, int maxIteration)
{
	for (int iter = 0; iter < maxIteration; iter++){
		uchar4 *pbo_dptr = NULL;

		if (viewer != nullptr)
			cudaGLMapBufferObject((void**)&pbo_dptr, viewer->getPBO());

		// execute the kernel
		int frame = 0;
		renderer->pathtrace(pbo_dptr, frame, iter);

		// unmap buffer object
		if (viewer != nullptr){
			cudaGLUnmapBufferObject(viewer->getPBO());
			viewer->update(iter);
		}

		//std::cout << "done iter " << iter << std::endl;
	}

	viewer->saveImage(utilityCore::currentTimeString(), maxIteration);
	renderer->pathtraceFree();
	cudaDeviceReset();
}