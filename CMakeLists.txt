cmake_minimum_required(VERSION 3.30)
include(cmake/dependencies.cmake)

# Find the PluginSDK package for Grand Theft Auto: San Andreas
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(PluginSDK REQUIRED COMPONENTS gtasa)

# Check for GTA_SA_DIR environment variable for deployment
if(NOT DEFINED ENV{GTA_SA_DIR})
    message(FATAL_ERROR "Environment variable GTA_SA_DIR is not set. This is required for debugging and deployment.")
else()
    set(GTA_SA_DIR "$ENV{GTA_SA_DIR}" CACHE PATH "Path to GTA:SA installation directory")
endif()

project(
	pricel
	VERSION 0.1
	LANGUAGES CXX
)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(
		FATAL_ERROR
		"In-source builds are not allowed. Please create a separate build directory."
	)
endif()

# Set global properties
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the MSVC runtime library for all targets in the project
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

file(GLOB_RECURSE sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.[ch]"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.[ch]pp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.[ch]xx"
)

add_library(
	${PROJECT_NAME}
	SHARED
	${sources}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "${PROJECT_NAME}.IM"
    SUFFIX ".asi"
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    spdlog::spdlog
    nlohmann_json::nlohmann_json
	PluginSDK::gtasa
	sampapi
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3 
        /sdl-
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        /IGNORE:4199 # LNK4199: /DELAYLOAD specified with no delays
        /IGNORE:4221 # LNK4221: This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library
    )
endif()

target_include_directories(
  ${PROJECT_NAME} PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:src>
) 