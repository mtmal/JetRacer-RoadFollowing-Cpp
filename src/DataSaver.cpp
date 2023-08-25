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

#include <cmath>
#include <filesystem>
#include <opencv2/imgcodecs.hpp>
#include <ScopedLock.h>
#include <CameraData.h>
#include <GenericTalker.h>
#include "Configuration.h"
#include "DataSaver.h"


namespace
{
/**
 *  @return the time of the system in seconds.
 */
double getTime()
{
    struct timespec timeStruct;
    clock_gettime(CLOCK_REALTIME, &timeStruct);
    return static_cast<double>(timeStruct.tv_sec) + static_cast<double>(timeStruct.tv_nsec / 1000000) / 1000;
}

bool strToBool(const std::string& value)
{
    return value == "true" || value == "True" || value == "1";
}

} // end of anonymouse namespace


// DataSaver::DataSaver(const std::string& mainFolder, const int height, const int width, const int type)
DataSaver::DataSaver(const Configuration& config)
: GenericListener<CameraData>(),
  GenericListener<DriveCommands>(),
  mRun(true),
  mThread(0),
  mUid(0),
  mFolderName(),
  mImage()
{
    pthread_mutex_init(&mLock, nullptr);
    sem_init(&mSem, 0, 0);

    if (strToBool(config.at("isMono")))
    {
        mImage = cv::Mat(std::stod(config.at("height")), std::stod(config.at("width")), CV_8UC3);
        mFolderName = ("./mono/" + std::to_string(getTime()));
    }
    else
    {
        mImage = cv::Mat(std::stod(config.at("height")), std::stod(config.at("width")) * 2, CV_8UC1);
        mFolderName = ("./stereo/" + std::to_string(getTime()));
    }
    std::filesystem::create_directories(mFolderName);
    pthread_create(&mThread, nullptr, DataSaver::startThread, this);
}

DataSaver::~DataSaver()
{
    stop();
    pthread_mutex_destroy(&mLock);
    sem_destroy(&mSem);
}

void DataSaver::update(const CameraData& cameraData)
{
    ScopedLock lock(mLock);
    if (cameraData.mImage.size() == 1)
    {
        cameraData.mImage[0].createMatHeader().copyTo(mImage);
    }
    else
    {
        cameraData.mImage[0].createMatHeader().copyTo(mImage(cv::Rect(0, 0, cameraData.mImage[0].cols, cameraData.mImage[0].rows)));
        cameraData.mImage[1].createMatHeader().copyTo(mImage(cv::Rect(cameraData.mImage[0].cols, 0, cameraData.mImage[1].cols, cameraData.mImage[1].rows)));
    }
    sem_post(&mSem);
}

void DataSaver::update(const DriveCommands& driveCommands)
{
    ScopedLock lock(mLock);
    mDriveCommands = driveCommands;
}

void DataSaver::stop()
{
    mRun = false;
    if (mThread > 0)
    {
        pthread_cancel(mThread);
        pthread_join(mThread, nullptr);
        mThread = 0;
    }
}

void DataSaver::threadBody()
{
    int bytesWritten;
    char path[256] = {0};
    
    while (mRun.load(std::memory_order_relaxed))
    {
        if (0 == sem_wait(&mSem))
        {
            ScopedLock lock(mLock);
            if (fabs(mDriveCommands.mThrottle) > 1e-6f)
            {
                bytesWritten = snprintf(path, 256, "%s/%f_%f_%lu.jpg", mFolderName.c_str(), mDriveCommands.mSteering, mDriveCommands.mThrottle, mUid++);
                memset(path + bytesWritten, '\0', 256 - bytesWritten);
                imwrite(path, mImage);
                printf("Image saved at: %s \n", path);
            }
        }
    }
}

void* DataSaver::startThread(void* instance)
{
    DataSaver* ds = static_cast<DataSaver*>(instance);
    ds->threadBody();
    return nullptr;
}