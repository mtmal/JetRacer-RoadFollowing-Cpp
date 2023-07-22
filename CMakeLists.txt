cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0079 NEW)

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

# add the utils submodule
add_subdirectory(JetracerUtils)
include_directories(JetracerUtils/src)

# set the custom jetracer utils path to prevent issues with building the same submodule by other submodules
set(CUSTOM_JETRACER_UTILS JetracerUtils)

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

# Build the road following tool
add_executable(JetRacer_RoadFollowing src/main.cpp)
target_link_libraries(JetRacer_RoadFollowing JetracerUtils CSI_Camera JetRacer I2C TorchInference ${TORCH_LIBRARIES} ${OpenCV_LIBRARIES})

#add_executable(JetRacer_SaveImages src/saveImagesMain.cpp)
#target_link_libraries(JetRacer_SaveImages CSI_Camera JetRacer I2C ${OpenCV_LIBRARIES})
