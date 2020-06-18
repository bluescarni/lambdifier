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

project(lambdifier VERSION 0.1.0 LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/yacma")

message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "lambdifier version: ${lambdifier_VERSION}")

# Run the YACMA compiler setup.
include(YACMACompilerLinkerSettings)

# The build options.
option(LAMBDIFIER_BUILD_TESTS "Build unit tests." OFF)

# Assemble the flags.
set(LAMBDIFIER_CXX_FLAGS_DEBUG ${YACMA_CXX_FLAGS} ${YACMA_CXX_FLAGS_DEBUG})
set(LAMBDIFIER_CXX_FLAGS_RELEASE ${YACMA_CXX_FLAGS})
if(YACMA_COMPILER_IS_MSVC)
  # On both cl and clang-cl, disable the idiotic minmax macros and enable the bigobj option.
  # Also, enable the WIN32_LEAN_AND_MEAN definition:
  # https://stackoverflow.com/questions/11040133/what-does-defining-win32-lean-and-mean-exclude-exactly
  list(APPEND LAMBDIFIER_CXX_FLAGS_DEBUG "-DNOMINMAX" "/bigobj" "-DWIN32_LEAN_AND_MEAN")
  list(APPEND LAMBDIFIER_CXX_FLAGS_RELEASE "-DNOMINMAX" "/bigobj" "-DWIN32_LEAN_AND_MEAN")
  if(YACMA_COMPILER_IS_CLANGXX)
    # clang-cl emits various warnings from GMP/MPFR, let's just silence them.
    # NOTE: at one point in the recent past, MSVC added an options similar to GCC's isystem:
    # https://blogs.msdn.microsoft.com/vcblog/2017/12/13/broken-warnings-theory/
    # We probably just need to wait for this to be picked up by CMake/clang-cl. Let's
    # revisit the issue in the future.
    list(APPEND _LAMBDIFIER_CLANG_CL_DISABLED_WARNINGS
        "-Wno-unused-variable"
        "-Wno-inconsistent-dllimport"
        "-Wno-unknown-pragmas"
        "-Wno-unused-parameter"
        "-Wno-sign-compare"
        "-Wno-deprecated-declarations"
        "-Wno-deprecated-dynamic-exception-spec"
        "-Wno-old-style-cast"
        "-Wno-sign-conversion"
        "-Wno-non-virtual-dtor"
        "-Wno-deprecated"
        "-Wno-shadow"
        "-Wno-shorten-64-to-32"
        "-Wno-reserved-id-macro"
        "-Wno-undef"
        "-Wno-c++98-compat-pedantic"
        "-Wno-documentation-unknown-command"
        "-Wno-zero-as-null-pointer-constant"
        "-Wno-language-extension-token"
        "-Wno-gnu-anonymous-struct"
        "-Wno-nested-anon-types"
        "-Wno-documentation"
        "-Wno-comma"
        "-Wno-nonportable-system-include-path"
        "-Wno-global-constructors"
        "-Wno-redundant-parens"
        "-Wno-exit-time-destructors"
        "-Wno-missing-noreturn"
        "-Wno-switch-enum"
        "-Wno-covered-switch-default"
        "-Wno-float-equal"
        "-Wno-double-promotion"
        "-Wno-microsoft-enum-value"
        "-Wno-missing-prototypes"
        "-Wno-implicit-fallthrough"
        "-Wno-format-nonliteral"
        "-Wno-cast-qual"
        "-Wno-disabled-macro-expansion"
        "-Wno-unused-private-field"
        "-Wno-unused-template"
        "-Wno-unused-macros"
        "-Wno-extra-semi-stmt"
        "-Wno-c++98-compat")
    list(APPEND LAMBDIFIER_CXX_FLAGS_DEBUG ${_LAMBDIFIER_CLANG_CL_DISABLED_WARNINGS})
    list(APPEND LAMBDIFIER_CXX_FLAGS_RELEASE ${_LAMBDIFIER_CLANG_CL_DISABLED_WARNINGS})
    unset(_LAMBDIFIER_CLANG_CL_DISABLED_WARNINGS)
  else()
    # Same as above, disable some cl warnings.
    list(APPEND LAMBDIFIER_CXX_FLAGS_DEBUG "/wd4459" "/wd4127")
    list(APPEND LAMBDIFIER_CXX_FLAGS_RELEASE "/wd4459" "/wd4127")
  endif()
  # Enable strict conformance mode, if supported.
  set(CMAKE_REQUIRED_QUIET TRUE)
  check_cxx_compiler_flag("/permissive-" _LAMBDIFIER_MSVC_SUPPORTS_STRICT_CONFORMANCE)
  unset(CMAKE_REQUIRED_QUIET)
  if(_LAMBDIFIER_MSVC_SUPPORTS_STRICT_CONFORMANCE)
    message(STATUS "The '/permissive-' flag is supported, enabling it.")
    list(APPEND LAMBDIFIER_CXX_FLAGS_DEBUG "/permissive-")
    list(APPEND LAMBDIFIER_CXX_FLAGS_RELEASE "/permissive-")
  endif()
  unset(_LAMBDIFIER_MSVC_SUPPORTS_STRICT_CONFORMANCE)
endif()

# List of source files.
set(LAMBDIFIER_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/llvm_state.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/expression.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/number.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/binary_operator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/variable.cpp"
)

# Setup of the lambdifier shared library.
add_library(lambdifier SHARED "${LAMBDIFIER_SRC_FILES}")
set_property(TARGET lambdifier PROPERTY VERSION "1.0")
set_property(TARGET lambdifier PROPERTY SOVERSION 1)
set_target_properties(lambdifier PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(lambdifier PROPERTIES VISIBILITY_INLINES_HIDDEN TRUE)

# Setup common to both the shared and static variants.
target_compile_options(lambdifier PRIVATE
    "$<$<CONFIG:Debug>:${LAMBDIFIER_CXX_FLAGS_DEBUG}>"
    "$<$<CONFIG:Release>:${LAMBDIFIER_CXX_FLAGS_RELEASE}>"
    "$<$<CONFIG:RelWithDebInfo>:${LAMBDIFIER_CXX_FLAGS_RELEASE}>"
    "$<$<CONFIG:MinSizeRel>:${LAMBDIFIER_CXX_FLAGS_RELEASE}>"
)
# Let's setup the target C++ standard, but only if the user did not provide it manually.
if(NOT CMAKE_CXX_STANDARD)
    set_property(TARGET lambdifier PROPERTY CXX_STANDARD 20)
endif()
set_property(TARGET lambdifier PROPERTY CXX_STANDARD_REQUIRED YES)
set_property(TARGET lambdifier PROPERTY CXX_EXTENSIONS NO)
target_include_directories(lambdifier PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>)

find_package(LLVM REQUIRED)

add_library(lambdifier::llvm_headers INTERFACE IMPORTED)
set_target_properties(lambdifier::llvm_headers PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LLVM_INCLUDE_DIRS}")

llvm_map_components_to_libnames(LAMBDIFIER_LLVM_LIBS core support instcombine scalaropts)
target_link_libraries(lambdifier PUBLIC lambdifier::llvm_headers ${LAMBDIFIER_LLVM_LIBS})

if(LAMBDIFIER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
