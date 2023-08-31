cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0079 NEW)

# set the project name
project(JetRacer_RoadFollowing)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
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

# add the I2C submodule
add_subdirectory(I2C)
include_directories(I2C/src)

# set the custom jetracer utils path to prevent issues with building the same submodule by other submodules
set(CUSTOM_JETRACER_UTILS JetracerUtils)
set(CUSTOM_I2C I2C)

# add the camera submodule
add_subdirectory(CSI_Camera)
include_directories(CSI_Camera/src)

# add the jetracer submodule
add_subdirectory(JetRacer)
include_directories(JetRacer/src)

# add the OLED submodule
add_subdirectory(OLED-0.91in-Cpp)
include_directories(OLED-0.91in-Cpp/src)
include_directories(OLED-0.91in-Cpp/src/Fonts)
include_directories(OLED-0.91in-Cpp/src/GUI)

# add the torch inference submodule
add_subdirectory(TorchInference)
include_directories(TorchInference/src)

# add the source code
include_directories(src)

# build the road following app
add_executable(JetRacer_RoadFollowing src/CameraDriveAdapter.cpp src/main.cpp)
target_link_libraries(JetRacer_RoadFollowing JetracerUtils CSI_Camera JetRacer I2C TorchInference ${TORCH_LIBRARIES} ${OpenCV_LIBRARIES})

# build config parser test
add_executable(test_configParser tests/test_configParser.cpp src/Configuration.cpp)


add_executable(test_racer src/CameraDriveAdapter.cpp src/StateMachine.cpp src/Configuration.cpp src/DataSaver.cpp src/OledWrapper.cpp src/jetracer_main.cpp)
target_link_libraries(test_racer JetracerUtils CSI_Camera JetRacer I2C TorchInference OLED-0.91in ${TORCH_LIBRARIES} ${OpenCV_LIBRARIES} -lstdc++fs)