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

#include <CSI_Camera.h>
#include <CSI_StereoCamera.h>
#include "Configuration.h"
#include "StateMachine.h"

bool strToBool(const std::string& value)
{
    return value == "true" || value == "True" || value == "1";
}

void start(const Configuration& config)
{
    std::unique_ptr<ICameraTalker> camera;
    if (strToBool(config.at("isMono")))
    {
        camera = std::make_unique<CSI_Camera>();
    }
    else
    {
        cv::Size imageSize(std::stoi(config.at("width")), std::stoi(config.at("height")));
        camera = std::make_unique<CSI_StereoCamera>(imageSize);
        if (!static_cast<CSI_StereoCamera*>(camera.get())->loadCalibration(config.at("calibration")))
        {
            puts("Failed to load stereo camera calibration");
        }
    } 
    StateMachine sm(config, camera.get());

    if (sm.initialise())
    {
        while (0 != sem_wait(sm.getSem()))
        {
            ;
        }
    }
}

int main(int argc, char** argv)
{
    Configuration config;
    std::string path = (argc > 1) ? argv[1] : "../config/config.ini";

    if (config.loadConfiguration(path))
    {
        start(config);
    }
    else
    {
        printf("Failed to open file at path: %s \n", path.c_str());
        puts("Either provide a valid path as the first argument, or ensure that there is a valid file under config/config.ini.");
    }

    return 0;
}