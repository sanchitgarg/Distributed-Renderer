#pragma once

#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/string_cast.hpp>
#include "glm/glm.hpp"
#include "utilities.h"
#include "sceneStructs.h"
#include "Mesh.h"

using namespace std;

class Scene {
private:
    ifstream fp_in;
    int loadMaterial(string materialid);
    int loadGeom(string objectid);
    int loadCamera();

public:
    Scene(string filename);
	~Scene();

    std::vector<Geom> geoms;
    std::vector<Material> materials;
    std::vector<Mesh> meshes;
	std::vector<MeshGeom> meshGeoms;
    RenderState state;

    void configureCamera();
    void findLights();
	void configureMeshes();
	void getImage();
};
