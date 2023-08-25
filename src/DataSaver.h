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

#pragma once

#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <DriveCommands.h>
#include <GenericListener.h>

struct CameraData;
class Configuration;

class DataSaver : public GenericListener<CameraData>,
                  public GenericListener<DriveCommands>
{
public:
    explicit DataSaver(const Configuration& config);
    virtual ~DataSaver();

    void update(const CameraData& cameraData) override;
    void update(const DriveCommands& driveCommands) override;

    void stop();

    void threadBody();

private:
    static void* startThread(void* instance);

    std::atomic<bool> mRun;
    pthread_t mThread;
    unsigned long mUid;
    std::string mFolderName;
    cv::Mat mImage;
    DriveCommands mDriveCommands;
    pthread_mutex_t mLock;
    sem_t mSem;
};