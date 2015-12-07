#include "interactions.h"

// ---------------------------------------------------------------------------------------
//						Common Sampling functions
// ---------------------------------------------------------------------------------------

//Function to find a random point in the cosine weighted hemispherical direction
__host__ __device__
glm::vec3 calculateRandomDirectionInHemisphere(
glm::vec3 normal, thrust::default_random_engine &rng) {
	thrust::uniform_real_distribution<float> u01(0, 1);

	float up = sqrt(u01(rng)); // cos(theta)
	float over = sqrt(1 - up * up); // sin(theta)
	float around = u01(rng) * TWO_PI;

	// Find a direction that is not the normal based off of whether or not the
	// normal's components are all equal to sqrt(1/3) or whether or not at
	// least one component is less than sqrt(1/3). Learned this trick from
	// Peter Kutz.

	glm::vec3 directionNotNormal;
	if (abs(normal.x) < SQRT_OF_ONE_THIRD) {
		directionNotNormal = glm::vec3(1, 0, 0);
	}
	else if (abs(normal.y) < SQRT_OF_ONE_THIRD) {
		directionNotNormal = glm::vec3(0, 1, 0);
	}
	else {
		directionNotNormal = glm::vec3(0, 0, 1);
	}

	// Use not-normal direction to generate two perpendicular directions
	glm::vec3 perpendicularDirection1 =
		glm::normalize(glm::cross(normal, directionNotNormal));
	glm::vec3 perpendicularDirection2 =
		glm::normalize(glm::cross(normal, perpendicularDirection1));

	return up * normal
		+ cos(around) * over * perpendicularDirection1
		+ sin(around) * over * perpendicularDirection2;
}

//Taken from CIS 560 code
//Get a random point on a cube
__host__ __device__
glm::vec3 getRandomPointOnCubeLight(Geom &box, thrust::default_random_engine &rng)
{
	glm::vec3 dim = box.scale;//, glm::vec4(1,1,1,1.0f));

	float side1 = dim[0] * dim[1];		// x-y
	float side2 = dim[1] * dim[2];		// y-z
	float side3 = dim[0] * dim[2];		// x-z
	float totalArea = 1.0f / (2.0f * (side1 + side2 + side3));

	// pick random face weighted by surface area
	thrust::uniform_real_distribution<float> u01(0, 1);
	thrust::uniform_real_distribution<float> u02(-0.5, 0.5);
	float r = u01(rng);
	// pick 2 random components for the point in the range (-0.5, 0.5)
	float c1 = u02(rng);
	float c2 = u02(rng);

	glm::vec4 point;
	if (r < side1 / totalArea) {
		// x-y front
		point = glm::vec4(c1, c2, 0.5f, 1);
	}
	else if (r < (side1 * 2) * totalArea) {
		// x-y back
		point = glm::vec4(c1, c2, -0.5f, 1);
	}
	else if (r < (side1 * 2 + side2) * totalArea) {
		// y-z front
		point = glm::vec4(0.5f, c1, c2, 1);
	}
	else if (r < (side1 * 2 + side2 * 2) * totalArea) {
		// y-z back
		point = glm::vec4(-0.5f, c1, c2, 1);
	}
	else if (r < (side1 * 2 + side2 * 2 + side3) * totalArea) {
		// x-z front
		point = glm::vec4(c1, 0.5f, c2, 1);
	}
	else {
		// x-z back
		point = glm::vec4(c1, -0.5f, c2, 1);
	}

	//	return glm::vec3(point);
	return multiplyMV(box.transform, point);
}

//Taken from CIS 560 code
//Get a random point on a sphere
__host__ __device__
glm::vec3 getRandomPointOnSphereLight(Geom &sphere, thrust::default_random_engine &rng)
{
	thrust::uniform_real_distribution<float> u01(0, 1);
	float u = u01(rng);
	float v = u01(rng);

	float theta = 2.0f * PI * u;
	float phi = acos(2.0f * v - 1.0f);

	glm::vec4 point;
	point[0] = sin(phi) * cos(theta);
	point[1] = sin(phi) * sin(theta);
	point[2] = cos(phi);
	point[3] = 1.0;

	return multiplyMV(sphere.transform, point);
}

// ---------------------------------------------------------------------------------------
//						FUNCTIONS RELATED TO LIGHT CALCULATIONS
// ---------------------------------------------------------------------------------------

//Select random light
//TODO : Change the function to bias light selection maybe based on intensity or size
__host__ __device__
void selectRandomLight(int *lightIndices, int *lightCount,
thrust::default_random_engine &rng, int& i)
{
	thrust::uniform_real_distribution<float> u01(0, *lightCount - 0.001);
	i = lightIndices[(int)u01(rng)];
}

//Get a random point on a random light
__host__ __device__
glm::vec3 getRandomPointOnLight(Geom *g, int *lightIndices, int *lightCount, thrust::default_random_engine &rng, int& i)
{
	selectRandomLight(lightIndices, lightCount, rng, i);

	switch (g[i].type)
	{
	case CUBE:
		return getRandomPointOnCubeLight(g[i], rng);

	case MESH:		//TODO: treat mesh light like a sphere for now, change later
	case SPHERE:
		return getRandomPointOnSphereLight(g[i], rng);
	default:
		break;
	}

	return glm::vec3(0);
}


//LIGHT CALCULATIONS
//Function to calculate the light term in the rendering equation
__host__ __device__
glm::vec3 getLightTerm(Geom *g,		// Pointer to all geometries
Material *m,
int& i		//Light geometry index
)
{
	// Calculate : Diffused Color * Light Intensity
	// If we have a participating media, then we need to change this
	return m[g[i].materialid].color * m[g[i].materialid].emittance;
}

//Function to see if the intersection point can reach a randomly sampled light
// If it reaches the light, then the intersect and normal arguments will give you the light intersection light 
// point and the normal at that point
__host__ __device__
bool hitRandomLightPoint(glm::vec3 &intersect, glm::vec3 &normal, int &lightIndex,
Geom *g, int &geomIndex, int *geomCount,
MeshGeom *meshGeoms, int *meshCount,
int *lightIndices, int *lightCount,
thrust::default_random_engine &rng		// random engine
)
{

	intersect += normal * 0.001f;	//Ofset point

	//Get random point on light
	glm::vec3 lightPoint = getRandomPointOnLight(g, lightIndices, lightCount, rng, lightIndex);

	//Create the ray tp sampled point
	Ray rayToLight;
	rayToLight.origin = intersect;
	rayToLight.direction = glm::normalize(lightPoint - intersect);

	//Create variables for intersection
	glm::vec3 intersectionPoint = glm::vec3(0), intersectNormal = glm::vec3(0);
	float min_t = FLT_MAX, t;
	int nearestIndex = -1;


	for (int i = 0; i<(*geomCount); ++i)
	{
		if (g[i].type == CUBE)
		{
			t = boxIntersectionTest(g[i], rayToLight, intersectionPoint, intersectNormal);//, outside);
		}

		else if (g[i].type == SPHERE)
		{
			t = sphereIntersectionTest(g[i], rayToLight, intersectionPoint, intersectNormal);//, outside);
		}

		else if (g[i].type == MESH)
		{
			//t = sphereIntersectionTest(geoms[i], r.ray, intersectionPoint, normal);//, outside);
			t = meshIntersectionTest(g[i], meshGeoms[g[i].meshid], rayToLight, intersectionPoint, intersectNormal);//, outside);
		}

		if (t > 0 && t < min_t)//&& !outside)
		{
			min_t = t;
			intersect = intersectionPoint;
			nearestIndex = i;
			normal = intersectNormal;
		}
	}

	return (nearestIndex == lightIndex);
}

__host__ __device__
float getLightArea(Geom *g, int *lightIndices, int &lightIndex)
{
	Geom &l = g[lightIndices[lightIndex]];

	switch (l.type)
	{
	case CUBE:
		return (2.0f * (l.scale.x * l.scale.y +
			l.scale.y * l.scale.z +
			l.scale.x * l.scale.z));

		//Reference http://www.web-formulas.com/Math_Formulas/Geometry_Surface_of_Ellipsoid.aspx
		//			Knud Thomsen’s Formula for area of a ellipsoid
		//			Area approx = 4 * PI * ((ab + bc + ca)/3) ^ p
		//					a = radius1 ^ p
		//					b = radius2 ^ p
		//					c = radius3 ^ p
		//				Using p = 1.6075
		//					1/p = 0.62
	case SPHERE:
	{
		float a = powf(l.scale.x, 1.6075);
		float b = powf(l.scale.y, 1.6075);
		float c = powf(l.scale.z, 1.6075);

		return (FOUR_PI * powf((a*b + b*c + c*a) * ONE_OVER_THREE, 0.62));
	}

	default:
		return 0.f;
	}
}

__host__ __device__
bool hitSelectedLight(glm::vec3 &lightIntersect, glm::vec3 &lightNormal, glm::vec3 &wi,
Geom * g, int *geomCount, MeshGeom *meshGeoms,
int * lightIndices, int &lightIndex)
{
	Ray r;
	r.origin = lightIntersect + EPSILON * wi;
	r.direction = wi;

	float lightT = -1.0f;
	Geom &geom = g[lightIndices[lightIndex]];

	switch (geom.type)
	{
	case CUBE:
		lightT = boxIntersectionTest(geom, r, lightIntersect, lightNormal);//, outside);
		break;

	case MESH:
		lightT = meshIntersectionTest(geom, meshGeoms[geom.meshid], r, lightIntersect, lightNormal);//, outside);
		break;

	case SPHERE:
		lightT = sphereIntersectionTest(geom, r, lightIntersect, lightNormal);//, outside);
		break;
	}

	//If t > 0 means we can reach the light
	if (lightT > 0.f)
	{
		glm::vec3 intersectionPoint, normal;
		//Now check if the path is clear i.e. light is the closest intersection
		float t = -1.0f;

		for (int i = 0; i < (*geomCount); ++i)
		{
			if (i != lightIndices[lightIndex])
			{
				switch (geom.type)
				{
				case CUBE:
					t = boxIntersectionTest(geom, r, intersectionPoint, normal);//, outside);
					break;

				case MESH:
					t = meshIntersectionTest(geom, meshGeoms[geom.meshid], r, intersectionPoint, normal);//, outside);
					break;

				case SPHERE:
					t = sphereIntersectionTest(geom, r, intersectionPoint, normal);//, outside);
					break;
				}

				//if new intersection is nearer to the ray origin, means like cannot be reached
				if (t > 0 && t < lightT)//&& !outside)
				{
					return false;
				}
			}
		}

		//If we can reach the light and that is our closest intersection point
		//	Then return true
		return true;
	}

	return false;
}




// ---------------------------------------------------------------------------------------
//						BxDF Calculation Functions
// ---------------------------------------------------------------------------------------

//Helper function to evaluate Fresnel reflection formula for dielectric materials
__host__ __device__
float FrDiel(float &cosi, float &cost, float &ei, float &et)
{
	float etat(et);
	float etai(ei);

	float Rparl = ((etat * cosi) - (etai * cost)) /
		((etat * cosi) + (etai * cost));

	float Rperp = ((etai * cosi) - (etat * cost)) /
		((etai * cosi) + (etat * cost));

	return (Rparl * Rparl + Rperp * Rperp) / 2.0f;
}

//Function to evaluate the fresnel term for refraction
__host__ __device__
float evaluateFresnel(Material &m,
glm::vec3 &wi, glm::vec3 &wo, bool &outside,
glm::vec3 &normal)
{
	float cosi = glm::dot(wo, normal);
	cosi = glm::clamp(cosi, -1.f, 1.f);

	float absCosi = glm::abs(cosi);

	float ei, et;

	if (outside)
	{
		ei = 1.0f;
		et = m.indexOfRefraction;
	}

	else
	{
		ei = m.indexOfRefraction;
		et = 1.0f;
	}

	float sint = (ei / et) * sqrtf(std::max(0.0f, 1.0f - cosi * cosi));

	if (sint >= 1.f)
	{
		//TIR
		return 1.0f;
	}

	else
	{
		//Else refracted so get the color
		float cost = sqrtf(max(0.0f, 1.f - sint*sint));

		//Using dielectric materials
		return FrDiel(absCosi, cost, ei, et);
	}
}

//Function to calculate the BxDF term (color) used by the Monte Carlo estimator
__host__ __device__
glm::vec3 getBxDFTerm(Material &m,
glm::vec3 &wi, glm::vec3 &wo, bool &outside,
glm::vec3 &normal)
{
	//REFRACTIVE MATERIAL
	if (m.hasRefractive)
	{
		//PBRT : Pg 444
		//	Assuming transmission scale factor T = material color

		//glm::vec3 F = evaluateFresnel(m, wi, wo, outside, normal);
		//float F = evaluateFresnel(m, wi, wo, outside, normal);
		//F = glm::clamp(F, 0.0f, 1.0f);
		//
		//if (outside)
		//{
		//	//return (m.indexOfRefraction * m.indexOfRefraction) * (glm::vec3(1.f) - F) 
		//	//					* m.color / glm::abs(glm::dot(wi, normal));
		//	return (m.indexOfRefraction * m.indexOfRefraction) * (1.f - F)
		//			* m.color / glm::abs(glm::dot(wi, normal));
		//
		//	//return F * m.color / glm::abs(glm::dot(wi, normal));
		//
		//}
		//else
		//{
		//	//return (1.0f / (m.indexOfRefraction * m.indexOfRefraction)) * (glm::vec3(1.f) - F)
		//	//	* m.color / glm::abs(glm::dot(wi, normal));
		//	return (1.0f / (m.indexOfRefraction * m.indexOfRefraction)) * (1.f - F)
		//			* m.color / glm::abs(glm::dot(wi, normal));
		//
		//	//return F * m.color / glm::abs(glm::dot(wi, normal));
		//}
		//	
		return 1.0f * m.color / glm::abs(glm::dot(wo, normal));
	}

	//REFLECTIVE MATERIAL
	else if (m.hasReflective)
	{
		//Fresnel (Specular) term is 1.0 for perfect mirror
		//PBRT : Pg 441
		return 1.0f * m.color / glm::abs(glm::dot(wi, normal));
	}

	//DIFFUSED MATERIAL
	else if (m.hasReflective == 0 && m.hasRefractive == 0)
	{
		//Calculate : (Diffused Color * base color * texture color) / PI
		//		As the color gets distributed equally around the hemisphere
		//PBRT : Pg 447
		return m.color * ONE_OVER_PI;
	}

	return glm::vec3(0);
}

//Function to calculate the reflection direction based on the BxDF
__host__ __device__
glm::vec3 getBxDFDirection(Material &m,
glm::vec3 &normal, thrust::default_random_engine &rng,
glm::vec3 &rayDirection, glm::vec3 &intersect, bool &outside,
Geom *g, int geomIndex
)
{

	//REFRACTIVE MATERIAL
	if (m.hasRefractive)
	{
		//Create a ray
		Ray r; r.direction = rayDirection;

		//Refract
		if (outside)
		{
			r.direction = (glm::refract(r.direction, normal, m.indexOfRefraction));
		}
		else
		{
			r.direction = (glm::refract(r.direction, normal, 1.0f / m.indexOfRefraction));
		}

		r.origin = intersect + 100.0f * EPSILON * r.direction;

		//// find intersection with the same object
		//if (g[geomIndex].type == SPHERE)
		//{
		//	sphereIntersectionTest(g[geomIndex], r, intersect, normal);
		//}

		//else if (g[geomIndex].type == CUBE)
		//{
		//	boxIntersectionTest(g[geomIndex], r, intersect, normal);
		//}

		////Refract it again
		//r.direction = (glm::refract(r.direction, normal, m.indexOfRefraction));
		//r.origin = intersect + EPSILON * r.direction;

		//Set the new intersection as the offset position
		intersect = r.origin;

		//return the new direction
		return r.direction;
	}

	//REFLECTIVE MATERIAL
	else if (m.hasReflective)
	{
		return glm::normalize(glm::reflect(rayDirection, normal));
	}

	//DIFFUSED MATERIAL
	else if (m.hasReflective == 0 && m.hasRefractive == 0)
	{
		//cosine weighted hemisphere direction
		return glm::normalize(calculateRandomDirectionInHemisphere(normal, rng));
	}
}

//Function to calculate the PDF based on the BxDF
__host__ __device__
float calculateBxDFPDF(glm::vec3 &normal, glm::vec3 &wi, Material &m)
{
	//REFRACTIVE MATERIAL
	if (m.hasRefractive)
	{
		return 1.0f;
	}

	//REFLECTIVE MATERIAL
	else if (m.hasReflective)
	{
		return 1.0f;
	}

	//DIFFUSED MATERIAL
	else if (m.hasReflective == 0 && m.hasRefractive == 0)
	{
		//TODO : CHECK
		return glm::abs(glm::dot(normal, wi)) * ONE_OVER_PI;
	}
}



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
int *lightCount)
{
	//This function solves the monte carlo light equation estimator
	// Lo(p,wo) = LE(p,wo) + Summation (f(p,wo,wi)Li(p,wi) V(p’,p) absdot(wi, N) dwi)
	// Output radiance = emitted radiance 
	//					+ sum over samples( BRDF * LIGHT Imp Sampling * Light obstruction (0 or 1) * cos (angle between light normal and ray from light) )

	//Variables for light sampling
	glm::vec3 lightNormal = normal;
	glm::vec3 lightIntersection = intersect;
	int lightIndex = 0;

	Material &material = m[g[geomIndex].materialid];

	//---------LIS calculation---------------
	//Variable for LIS
	glm::vec3 LIS(0.f);
	float PDFLight;

	if (hitRandomLightPoint(lightIntersection, lightNormal, lightIndex, g, geomIndex, geomCount, meshGeoms, meshCount, lightIndices, lightCount, rng))
	{
		//If we hit a light, then we can reach the light so do LIS calculations

		//Find the absdot(wi, N) term
		glm::vec3 wi = glm::normalize(intersect - lightIntersection);
		float cosTheta = glm::abs(glm::dot(normal, wi));

		if (cosTheta > EPSILON)
		{
			//calculate sampled light area
			float lightArea = getLightArea(g, lightIndices, lightIndex);

			//calculate light PDF
			PDFLight = glm::distance2(lightIntersection, intersect) / (lightArea * cosTheta);

			if (PDFLight > EPSILON)
			{
				// LIS = LightTerm * BxDFTerm * absdot(wi, N) / PDFLight
				LIS = (getLightTerm(g, m, lightIndex) * getBxDFTerm(material, wi, ray.ray.direction, outside, normal) * cosTheta) / PDFLight;
			}
		}
	}


	//---------BIS calculation---------------
	//BIS variables
	glm::vec3 BIS(0.f);
	float PDFBxDF;
	float cosTheta = 0.0;
	glm::vec3 BxDFTerm;

	//Find an wi direction based on input direction and BxDF
	glm::vec3 wi = getBxDFDirection(material, normal, rng, ray.ray.direction, intersect, outside, g, geomIndex);

	//Calculate the BxDF PDF
	PDFBxDF = calculateBxDFPDF(normal, wi, material);

	if (PDFBxDF > EPSILON)
	{
		lightIntersection = intersect;
		lightNormal = normal;

		//CosTheta is the absdot of the surface normal and the wi
		cosTheta = glm::abs(glm::dot(normal, wi));

		//Get the BxDF term
		BxDFTerm = getBxDFTerm(material, wi, ray.ray.direction, outside, normal);

		//See if this ray hits the light used in LIS
		if (hitSelectedLight(lightIntersection, lightNormal, wi, g, geomCount, meshGeoms, lightIndices, lightIndex))
		{
			BIS = (getLightTerm(g, m, lightIndex) * BxDFTerm * cosTheta) / PDFBxDF;
		}
	}

	//Set Ray Color using power heuristics
	PDFLight *= PDFLight;
	float PDFBxDF2 = PDFBxDF * PDFBxDF;
	ray.rayColor += (((PDFLight * LIS) + (PDFBxDF2 * BIS)) / (PDFBxDF2 + PDFLight)) * ray.rayThroughPut;

	//Calculate new throughput which is the max of rgb of costheta(BRDF) * BxDFTerm / BxDF PDF
	ray.rayThroughPut *= cosTheta * glm::compMax(BxDFTerm) / PDFBxDF;

	//New ray direction is the wi selected
	ray.ray.direction = wi;

	//New ray origin is the intersection point plus some offset in the new direction
	ray.ray.origin = intersect + wi * EPSILON;

	//ray.rayColor = BIS;
	//ray.rayColor = LIS;
}


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
int *lightCount)
{

	Ray &r = ray.ray;

	if (m.emittance > 0 && m.emittance < 1)
	{
		//Glowing material
		ray.rayColor *= (m.color) / m.emittance;

		//Do SSS + Fresnel split using russian roulette
		thrust::uniform_real_distribution<float> u01(0, 1);

		float split = u01(rng);

		if (split > 0.5)
		{
			//Do SSS
			r.direction = glm::normalize(calculateRandomDirectionInHemisphere(normal, rng));

			if (split > 0.75)
			{
				//Do Sub Surface Scattering

				//Intersect the ray with the geometry again to get a point on the geom
				Ray newR;
				newR.origin = getPointOnRay(r, m.hasTranslucence);
				newR.direction = glm::normalize(g[geomIndex].translation - newR.origin);

				glm::vec3 newIntersect, newNormal;

				if (g[geomIndex].type == SPHERE)
				{
					sphereIntersectionTest(g[geomIndex], newR, newIntersect, newNormal);
				}

				else if (g[geomIndex].type == CUBE)
				{
					boxIntersectionTest(g[geomIndex], newR, newIntersect, newNormal);
				}

				r.direction = glm::normalize(calculateRandomDirectionInHemisphere(newNormal, rng));
				r.origin = newIntersect + EPSILON * r.direction;
			}

			else
			{
				//Do diffused
				r.origin = intersect + EPSILON * r.direction;
			}
		}

		else
		{
			//Do refraction to get refracted ray dir
			glm::vec3 transmittedDir = (glm::refract(r.direction, normal, 1.0f / m.indexOfRefraction));

			float cos_t = glm::dot(transmittedDir, -normal),
				cos_i = glm::dot(-r.direction, normal);

			float r_parallel = (m.indexOfRefraction * cos_i - cos_t) / (m.indexOfRefraction * cos_i + cos_t),
				r_perpendicular = (cos_i - m.indexOfRefraction * cos_t) / (cos_i + m.indexOfRefraction * cos_t);

			if (split < 0.25f * (r_parallel * r_parallel + r_perpendicular * r_perpendicular))
			{
				//do reflection

				ray.rayColor *= (m.color);
				r.direction = (glm::reflect(r.direction, normal));
				r.origin = intersect + EPSILON * r.direction;
			}

			else
			{
				//Do refraction
				ray.rayColor *= (m.color);
				r.direction = transmittedDir;
				r.origin = intersect + EPSILON * r.direction;

				//Intersect with the object again
				//float t;
				//bool outside;
				if (g[geomIndex].type == SPHERE)
				{
					/*t = */sphereIntersectionTest(g[geomIndex], r, intersect, normal);//, outside);
				}

				else if (g[geomIndex].type == CUBE)
				{
					/*t = */boxIntersectionTest(g[geomIndex], r, intersect, normal);//, outside);
				}

				r.direction = (glm::refract(r.direction, normal, m.indexOfRefraction));
				r.origin = intersect + EPSILON * r.direction;
			}
		}
	}

	else if (m.hasTranslucence > 0)
	{
		ray.rayColor *= (m.color) * 2.0f; // multiply by 2 as we take a 50% between
		// diffused and SSS

		//Sub Surface Scattering
		//Do random splitting between diffused and sub surface for a better result
		thrust::uniform_real_distribution<float> u01(0, 1);

		r.direction = glm::normalize(calculateRandomDirectionInHemisphere(normal, rng));

		if (u01(rng) > 0.5)
		{
			//Do Sub Surface Scattering

			//Intersect the ray with the geometry again to get a point on the geom
			Ray newR;
			newR.origin = getPointOnRay(r, m.hasTranslucence);
			newR.direction = glm::normalize(g[geomIndex].translation - newR.origin);

			glm::vec3 newIntersect, newNormal;

			if (g[geomIndex].type == SPHERE)
			{
				sphereIntersectionTest(g[geomIndex], newR, newIntersect, newNormal);
			}

			else if (g[geomIndex].type == CUBE)
			{
				boxIntersectionTest(g[geomIndex], newR, newIntersect, newNormal);
			}

			r.direction = glm::normalize(calculateRandomDirectionInHemisphere(newNormal, rng));
			r.origin = newIntersect + EPSILON * r.direction;
		}

		else
		{
			//Do diffused
			r.origin = intersect + EPSILON * r.direction;
		}
	}

	else if (m.hasReflective == 0 && m.hasRefractive == 0)
	{
		//Diffused material
		thrust::uniform_real_distribution<float> u01(0, 1);
		if (m.specular.exponent > 0 && u01(rng) < 0.2f)
		{
			//Do specular reflection
			int i;
			glm::vec3 lightVector = glm::normalize(getRandomPointOnLight(g, lightIndices, lightCount, rng, i) - intersect);
			glm::vec3 camVector = glm::normalize(camPosition - intersect);

			float specTerm = glm::dot(normal, glm::normalize(lightVector + camVector));
			specTerm = powf(specTerm, m.specular.exponent);

			ray.rayColor *= (m.specular.color * specTerm);

			//r.direction = glm::reflect(r.direction, normal);
			r.origin = intersect + EPSILON * r.direction;
		}

		else
		{
			//Do perfect diffused
			ray.rayColor *= (m.color);
			r.direction = glm::normalize(calculateRandomDirectionInHemisphere(normal, rng));
			r.origin = intersect + EPSILON * r.direction;
		}
	}

	else if (m.hasReflective > 0 && m.hasRefractive > 0)
	{
		//Do frenels reflection
		//REFERENCE : PBRT Page 435
		thrust::uniform_real_distribution<float> u01(0, 1);

		//Do refraction to get refracted ray dir
		glm::vec3 transmittedDir = (glm::refract(r.direction, normal, 1.0f / m.indexOfRefraction));

		float cos_t = glm::dot(transmittedDir, -normal),
			cos_i = glm::dot(-r.direction, normal);

		float r_parallel = (m.indexOfRefraction * cos_i - cos_t) / (m.indexOfRefraction * cos_i + cos_t),
			r_perpendicular = (cos_i - m.indexOfRefraction * cos_t) / (cos_i + m.indexOfRefraction * cos_t);

		if (u01(rng) < 0.5f * (r_parallel * r_parallel + r_perpendicular * r_perpendicular))
		{
			//do reflection

			ray.rayColor *= (m.color);
			r.direction = (glm::reflect(r.direction, normal));
			r.origin = intersect + EPSILON * r.direction;
		}

		else
		{
			//Do refraction
			ray.rayColor *= (m.color);
			r.direction = transmittedDir;
			r.origin = intersect + EPSILON * r.direction;

			//Intersect with the object again
			//float t;
			//bool outside;
			if (g[geomIndex].type == SPHERE)
			{
				/*t = */sphereIntersectionTest(g[geomIndex], r, intersect, normal);//, outside);
			}

			else if (g[geomIndex].type == CUBE)
			{
				/*t = */boxIntersectionTest(g[geomIndex], r, intersect, normal);//, outside);
			}

			r.direction = (glm::refract(r.direction, normal, m.indexOfRefraction));
			r.origin = intersect + EPSILON * r.direction;
		}

	}

	else if (m.hasReflective > 0)
	{
		//Reflective surface
		ray.rayColor *= m.color;
		r.direction = (glm::reflect(r.direction, normal));
		r.origin = intersect + EPSILON * r.direction;
	}

	else if (m.hasRefractive > 0)
	{
		//Refractive surface
		ray.rayColor *= m.color;
		r.direction = (glm::refract(r.direction, normal, 1.0f / m.indexOfRefraction));
		r.origin = intersect + EPSILON * r.direction;

		//Intersect with the object again
		//		float t;
		//		bool outside;
		if (g[geomIndex].type == SPHERE)
		{
			/*t = */sphereIntersectionTest(g[geomIndex], r, intersect, normal);//, outside);
		}

		else if (g[geomIndex].type == CUBE)
		{
			/*t = */boxIntersectionTest(g[geomIndex], r, intersect, normal);//, outside);
		}

		//		if (t > 0)
		{
			r.direction = (glm::refract(r.direction, normal, m.indexOfRefraction));
			r.origin = intersect + EPSILON * r.direction;
		}
	}
}

