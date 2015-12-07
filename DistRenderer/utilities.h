#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#include <ctime>
#include "glm/glm.hpp"
#include <algorithm>
#include <istream>
#include <ostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#define PI                3.1415926535897932384626422832795028841971f
#define ONE_OVER_PI       0.3183098861837906715377676381983307306405f
#define TWO_PI            6.2831853071795864769252867665590057683943f
#define FOUR_PI           12.566370614359172953850569133118011536788f
#define SQRT_OF_ONE_THIRD 0.5773502691896257645091487805019574556476f
#define ONE_OVER_THREE	  0.3333333333333333333333333333333333333333f
#define EPSILON           0.00001f

namespace utilityCore {
    extern float clamp(float f, float min, float max);
    extern bool replaceString(std::string& str, const std::string& from, const std::string& to);
    extern glm::vec3 clampRGB(glm::vec3 color);
    extern bool epsilonCheck(float a, float b);
    extern std::vector<std::string> tokenizeString(std::string str);
    extern glm::mat4 buildTransformationMatrix(glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale);
    extern std::string convertIntToString(int number);
    extern std::istream& safeGetline(std::istream& is, std::string& t); //Thanks to http://stackoverflow.com/a/6089413
    extern void printVec3(glm::vec3 v);

	extern std::string currentTimeString();
	extern void errorCallback(int error, const char* description);
}
