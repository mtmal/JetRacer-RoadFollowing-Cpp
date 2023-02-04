////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023 Mateusz Malinowski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <CSI_Camera.h>
#include <NvidiaRacer.h>
#include <TorchInference.h>

/**
 * Prints help information about this application.
 *  @param name the name of the executable.
 */
static void printHelp(const char* name)
{
    printf("Usage: %s [options] \n", name);
    printf("    -h, --help      -> prints this information \n");
    printf("    -p, --path      -> path to JIT model \n");
    printf("    -m, --mode      -> sets the specific camera mode, default: 0 \n");
    printf("    -f, --framerate -> sets the camera framerate in Hz, default: 10 \n");
    printf("    -c, --cols      -> sets the number of columns (width) in resized image, default: 224 \n");
    printf("    -r, --rows      -> sets the number of rows (height) in resized image, default: 224 \n");
    printf("    -i, --id        -> sets the ID of the camera to start, default: 0 \n");
    printf("\nExample: %s -c 224 -r 224 \n\n", name);
    printf("NOTE: if the application that uses nvargus to control cameras was killed without releasing the cameras,"
        " execute the following:\n\n"
        "$ sudo systemctl restart nvargus-daemon \n\n");
}

/**
 * Runs the application.
 *  @param pathToModel path to JIT script model.
 *  @param imageSize the size to which images should be resized.
 *  @param framerate the framerate at which cameras should acquire images.
 *  @param id the ID of the camera to start.
 *  @param mode the mode at which cameras should operate.
 */
void run(const std::string& pathToModel, const cv::Size& imageSize, const uint8_t framerate, const uint8_t mode, const uint8_t id)
{
    /** Wrapper class to perform Torch inference. */
    TorchInference torchInference(imageSize.width, imageSize.height);
    /** The mono camera class. */
    CSI_Camera camera(imageSize);
    /** Output data as a tensor. */
    torch::Tensor outTensor;
    /** Interface for the Jetracer. */
    NvidiaRacer racer;
    /** Buffer for acquired image. */
    cv::Mat img;
    /** Time when the system started. */
    long startTime = std::time(0);

    torchInference.initialise(pathToModel.c_str());
    if (racer.initialise())
    {
    	racer.setThrottleGain(0.5f);
        if (camera.startCamera(framerate, mode, id, 0))
        {
            while (std::time(0) - startTime < 10)
            {
                if (camera.getRawImage(img))
                {
                    torchInference.processImage(img, outTensor);
                    outTensor = outTensor.flatten();
                    racer.setSteering(outTensor[0].item().toFloat());
                    racer.setThrottle(outTensor[1].item().toFloat());
                }
                usleep(static_cast<__useconds_t>(1000000.0 / static_cast<double>(framerate)));
            }
            racer.setThrottle(0.0f);
            racer.setSteering(0.0f);
        }
        else
        {
            puts("Failed to open camera.");
        }
    }
    else
    {
    	puts("Failed to initialise Jetracer.");
    }
}

int main(int argc, char** argv)
{
    /** Flag indicating if the program was invoked only to provide help message. */
    bool requestedHelp = false;
    /** Path to JIT script model. */
    std::string pathToModel("");
    /** Image size for final images (after resize). */
    cv::Size imageSize(224, 224);
    /** Framerate at which CSI cameras should capture images. */
    uint8_t framerate = 10;
    /** The mode in which CSI cameras should operate. */
    uint8_t mode = 0;
    /** The ID of the CSI camera in case there are mutliple cameras. */
    uint8_t id = 0;

    for (int i = 1; i < argc; ++i)
    {
        if ((0 == strcmp(argv[i], "--path")) || (0 == strcmp(argv[i], "-p")))
        {
        	pathToModel = argv[i + 1];
        }
        else if ((0 == strcmp(argv[i], "--mode")) || (0 == strcmp(argv[i], "-m")))
        {
            mode = static_cast<uint8_t>(atoi(argv[i + 1]));
        }
        else if ((0 == strcmp(argv[i], "--framerate")) || (0 == strcmp(argv[i], "-f")))
        {
        	framerate = static_cast<uint8_t>(atoi(argv[i + 1]));
        }
        else if ((0 == strcmp(argv[i], "--cols")) || (0 == strcmp(argv[i], "-c")))
        {
        	imageSize.width = atoi(argv[i + 1]);
        }
        else if ((0 == strcmp(argv[i], "--rows")) || (0 == strcmp(argv[i], "-r")))
        {
        	imageSize.height = atoi(argv[i + 1]);
        }
        else if ((0 == strcmp(argv[i], "--id")) || (0 == strcmp(argv[i], "-i")))
        {
        	id = static_cast<uint8_t>(atoi(argv[i + 1]));
        }
        else if ((0 == strcmp(argv[i], "--help")) || (0 == strcmp(argv[i], "-h")))
        {
        	printHelp(argv[0]);
        	requestedHelp = true;
        }
        else
        {
            /* nothing to do in here */
        }
    }

    if (!requestedHelp)
    {
        if (pathToModel.empty())
        {
            puts("Missing path to a model, exiting...");
        }
        else
        {
            run(pathToModel, imageSize, framerate, mode, id);
        }
    }
    return 0;
}
