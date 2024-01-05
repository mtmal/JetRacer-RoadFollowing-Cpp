# JetRacer-RoadFollowing-Cpp
C++ implementation of a JetRacer road following example.

## Requirements
* OpenCV
* PyTorch (LibTorch comes installed with it)
* cmake 3.25
* Torch Tensort RT (have a look at my [jetcard fork](https://github.com/mtmal/jetcard/tree/jetpack_4.6.4))

## Building
```
$ mkdir build
$ cd build
$ cmake .. -DTorch_DIR=/usr/local/lib/python3.6/dist-packages/torch/share/cmake/Torch -DTorchTensorRT_DIR=/usr/local/lib/python3.6/dist-packages/torch_tensorrt
$ make -j 2
```
Note that you either need to pass a path to Torch folder as in the above example, or you need to export your own environment variable, or add it to your path.

Additionally, Tensor RT loads libraries dynamically at runtime, so it is important to add them to your LD library path:
```
$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/python3.6/dist-packages/torch/lib
```

## Running
Execute the following using provided model:
```
$ ./JetRacer_RoadFollowing -p ../drive_road_following_model_cpp.pt -f 10 -m 4
```
To exit, simply press ctrl+c in the terminal. The application handles SIGINT to gracefully stop JetRacer.
