#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <cstdlib>
#include <cstring>
#include <vector>
using std::vector;
#include <string>
using std::string;
#include <limits>
using std::numeric_limits;
#include <glm/glm.hpp>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
namespace k10
{
	const int FIXED_FRAMES_PER_SECOND = 240;
	const std::chrono::duration<double> FIXED_SECONDS_PER_FRAME =
		std::chrono::duration<double>(1) / FIXED_FRAMES_PER_SECOND;
}
template<class T>
inline T clamp(T value, T min, T max)
{
	return value < min ? min :
		   value > max ? max :
		   value;
}
namespace k10
{
	vector<Uint8> readFile(string const& fileName);
}