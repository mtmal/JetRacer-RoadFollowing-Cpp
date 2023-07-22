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

#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <CSI_Camera.h>
#include <NvidiaRacer.h>
#include <TorchInference.h>

/** Main semaphore to cleanly stop the application without busy waiting. */
sem_t SEM;

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
 * Handler for signals.
 *  @param signum the type of received signal.
 */
void handleSignals(int signum)
{
    if (signum == SIGINT)
    {
        sem_post(&SEM);
    }
}

class CameraListener : public IGenericListener<CameraData>
{
public:
    CameraListener(const int camId) : mCamId(camId)
    {
    }

    bool initialise(const std::string& pathToModel, const cv::Size& imageSize)
    {
        bool retVal = mRacer.initialise();
        if (retVal)
        {
            mRacer.setThrottleGain(0.5f);
            mTorchInference.initialise(pathToModel, imageSize.width, imageSize.height, 3);
        }
        return retVal;
    }

    void update(const CameraData& camData)
    {
        if (camData.mID == mCamId)
        {
            mTorchInference.processImage(camData.mImage.createMatHeader(), mOutTensor).flatten();
            mRacer.setSteering(mOutTensor[0].item().toFloat());
            mRacer.setThrottle(mOutTensor[1].item().toFloat());
        }
    }

    void stop()
    {
        mRacer.setThrottle(0);
    }

private:
    /** Camera ID. */
    uint8_t mCamId;
    /** Wrapper class to perform Torch inference. */
    TorchInference mTorchInference;
    /** Output data as a tensor. */
    torch::Tensor mOutTensor;
    /** Interface for the Jetracer. */
    NvidiaRacer mRacer;
};

/**
 * Runs the application.
 *  @param pathToModel path to JIT script model.
 *  @param imageSize the size to which images should be resized.
 *  @param framerate the framerate at which cameras should acquire images.
 *  @param camId the ID of the camera to start.
 *  @param mode the mode at which cameras should operate.
 */
void run(const std::string& pathToModel, const cv::Size& imageSize, const uint8_t framerate, const uint8_t mode, const uint8_t camId)
{
    /** The mono camera class. */
    CSI_Camera camera;
    /** Camera listener and JetRacer wrapper. */
    CameraListener listener(camId);
    /** Listener's ID. */
    int listID = camera.registerListener(listener);

    if (listener.initialise(pathToModel, imageSize))
    {
        if (camera.startCamera(imageSize, framerate, mode, camId, 2, true))
        {
            while (0 == sem_wait(&SEM))
            {
                // nothing to do, we are waiting on semaphore. While loop is to prevent
                // early application exit due to interrupts
                ;
            }
            camera.unregisterListener(listID);
            listener.stop();
        }
        else
        {
            puts("Failed to initialise camera and Jetracer.");
        }
    }
    else
    {
        puts("Failed to initalise camera listener.");
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

    /* Register the SIGINT handler (ctrl+c) */
    signal(SIGINT, handleSignals);
    /* Initialise the semaphore */
    sem_init(&SEM, 0, 0);

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
