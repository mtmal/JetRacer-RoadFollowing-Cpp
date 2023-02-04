cmake_minimum_required(VERSION 3.12)

# set the project name
project(JetRacer_RoadFollowing)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-fPIC -g -pedantic -Wall -Wextra")

# get OpenCV
find_package(OpenCV 4.5 REQUIRED)
include_directories(${OpenCV_INCLUDE_DIR})

# get Torch
find_package(Torch REQUIRED)
include_directories(${TORCH_INCLUDE_DIR})

# make sure we have the necessary submodule
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

# add the camera submodule
add_subdirectory(CSI_Camera)
include_directories(CSI_Camera/src)

# add the jetracer submodule with nested I2C
add_subdirectory(JetRacer)
include_directories(JetRacer/src)
include_directories(JetRacer/I2C/src)

# add the torch inference submodule
add_subdirectory(TorchInference)
include_directories(TorchInference/src)

# add the source code
include_directories(src)

# Build the calibration tool
add_executable(JetRacer_RoadFollowing src/main.cpp)
target_link_libraries(JetRacer_RoadFollowing CSI_Camera JetRacer I2C TorchInference ${TORCH_LIBRARIES} ${OpenCV_LIBRARIES})
