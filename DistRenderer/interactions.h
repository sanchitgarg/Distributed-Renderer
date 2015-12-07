#pragma once

#include <cuda.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/norm.hpp>
#include <thrust/execution_policy.h>
#include <thrust/random.h>

#include "intersections.h"

// ---------------------------------------------------------------------------------------
//						Common Sampling functions
// ---------------------------------------------------------------------------------------

//Function to find a random point in the cosine weighted hemispherical direction
__host__ __device__
glm::vec3 calculateRandomDirectionInHemisphere(
glm::vec3 normal, thrust::default_random_engine &rng);

//Taken from CIS 560 code
//Get a random point on a cube
__host__ __device__
glm::vec3 getRandomPointOnCubeLight(Geom &box, thrust::default_random_engine &rng);

//Taken from CIS 560 code
//Get a random point on a sphere
__host__ __device__
glm::vec3 getRandomPointOnSphereLight(Geom &sphere, thrust::default_random_engine &rng);

// ---------------------------------------------------------------------------------------
//						FUNCTIONS RELATED TO LIGHT CALCULATIONS
// ---------------------------------------------------------------------------------------

//Select random light
//TODO : Change the function to bias light selection maybe based on intensity or size
__host__ __device__
void selectRandomLight(int *lightIndices, int *lightCount,
					thrust::default_random_engine &rng, int& i);

//Get a random point on a random light
__host__ __device__
glm::vec3 getRandomPointOnLight(Geom *g, int *lightIndices, int *lightCount, thrust::default_random_engine &rng, int& i);


//LIGHT CALCULATIONS
//Function to calculate the light term in the rendering equation
__host__ __device__
glm::vec3 getLightTerm(Geom *g,		// Pointer to all geometries
					Material *m,
					int& i		//Light geometry index
				);

//Function to see if the intersection point can reach a randomly sampled light
// If it reaches the light, then the intersect and normal arguments will give you the light intersection light 
// point and the normal at that point
__host__ __device__
bool hitRandomLightPoint(glm::vec3 &intersect, glm::vec3 &normal, int &lightIndex,
				Geom *g, int &geomIndex, int *geomCount,
				MeshGeom *meshGeoms, int *meshCount,
				int *lightIndices, int *lightCount,
				thrust::default_random_engine &rng		// random engine
				);

__host__ __device__
float getLightArea(Geom *g, int *lightIndices, int &lightIndex);

__host__ __device__
bool hitSelectedLight( glm::vec3 &lightIntersect, glm::vec3 &lightNormal, glm::vec3 &wi,
					Geom * g, int *geomCount, MeshGeom *meshGeoms,
					int * lightIndices, int &lightIndex);



// ---------------------------------------------------------------------------------------
//						BxDF Calculation Functions
// ---------------------------------------------------------------------------------------

//Helper function to evaluate Fresnel reflection formula for dielectric materials
__host__ __device__
float FrDiel(float &cosi, float &cost, float &ei, float &et);

//Function to evaluate the fresnel term for refraction
__host__ __device__
float evaluateFresnel(Material &m,
						glm::vec3 &wi, glm::vec3 &wo, bool &outside,
						glm::vec3 &normal);

//Function to calculate the BxDF term (color) used by the Monte Carlo estimator
__host__ __device__
glm::vec3 getBxDFTerm(Material &m,
						glm::vec3 &wi, glm::vec3 &wo, bool &outside,
						glm::vec3 &normal);

//Function to calculate the reflection direction based on the BxDF
__host__ __device__
glm::vec3 getBxDFDirection(Material &m,
							glm::vec3 &normal, thrust::default_random_engine &rng,
							glm::vec3 &rayDirection, glm::vec3 &intersect, bool &outside,
							Geom *g, int geomIndex
							);

//Function to calculate the PDF based on the BxDF
__host__ __device__
float calculateBxDFPDF(glm::vec3 &normal, glm::vec3 &wi, Material &m);



// ---------------------------------------------------------------------------------------
//						Color calculation functions
// ---------------------------------------------------------------------------------------

//Solve the Monte Carlo Rendering equation Estimator
__host__ __device__
void getRayColor(glm::vec3 &camPosition,
		RayState &ray,
		glm::vec3 &intersect,
		glm::vec3 &normal,
		bool &outside,
		Material *m,
		thrust::default_random_engine &rng,
		Geom *g,
		int geomIndex,
		int *geomCount,
		MeshGeom *meshGeoms,
		int *meshCount,
		int *lightIndices,
		int *lightCount);


//Earlier hacky color monte carlo estimator

/**
* Scatter a ray with some probabilities according to the material properties.
* For example, a diffuse surface scatters in a cosine-weighted hemisphere.
* A perfect specular surface scatters in the reflected ray direction.
* In order to apply multiple effects to one surface, probabilistically choose
* between them.
*
*The visual effect you want is to straight-up add the diffuse and specular
* components. You can do this in a few ways. This logic also applies to
* combining other types of materias (such as refractive).
* (NOT RECOMMENDED - converges slowly or badly especially for pure-diffuse
*   or pure-specular. In principle this correct, though.)
*   Always take a 50/50 split between a diffuse bounce and a specular bounce,
*   but multiply the result of either one by 1/0.5 to cancel the 0.5 chance
*   of it happening.
* - Pick the split based on the intensity of each color, and multiply each
*   branch result by the inverse of that branch's probability (same as above).

* This method applies its changes to the Ray parameter `ray` in place.
* It also modifies the color `color` of the ray in place.
*
* You may need to change the parameter list for your purposes!
*/
__host__ __device__
void scatterRay(
		glm::vec3 &camPosition,
		RayState &ray,
		glm::vec3 intersect,
		glm::vec3 normal,
		Material &m,
		thrust::default_random_engine &rng,
		Geom *g,
		int geomIndex,
		int *lightIndices,
		int *lightCount);
