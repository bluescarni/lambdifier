# NOTE: C++20 supported since CMake 3.12.0:
# https://cmake.org/cmake/help/v3.12/prop_tgt/CXX_STANDARD.html
cmake_minimum_required(VERSION 3.12.0)

# Set default build type to "Release".
# NOTE: this should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself (it does so for nmake).
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	FORCE)
endif()

project(lambdifier VERSION 0.0.1 LANGUAGES CXX C)

