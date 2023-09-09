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

#include <string>
#include <DriveCommands.h>
#include <GenericListener.h>
#include <GenericThread.h>

struct CameraData;
class Configuration;

/**
 * Dedicated class for saving images with associated steering and throttle to a file.
 * Images path are built like that: [steering]_[throttle]_[uid].jpg
 * Because the thread waits on semaphore, it needs to be cancelled when stopping.
 */
class DataSaver : public GenericListener<CameraData>,
                  public GenericListener<DriveCommands>,
                  public GenericThread<DataSaver>
{
public:
    /**
     * Constructor that initialises path to where images should be saved and pre-allocates image buffer.
     *  @param config the main configuration.
     */
    explicit DataSaver(const Configuration& config);

    /**
     * Destructor, stops the thread.
     */
    virtual ~DataSaver();

    /**
     * Copies the @p cameraData and notifies the semaphore to resume separate thread to save the data to a file.
     *  @param cameraData the latest camera data received from either mono or stereo camera.
     */
    void update(const CameraData& cameraData) override;

    /**
     * Saves the @p driveCommands to be encoded into the image path. 
     *  @param driveCommands the latest drive command.
     */
    void update(const DriveCommands& driveCommands) override;

    /**
     * Overrides/shadows the baseclass function to enable creation of folder where data is to be saved.
     *  @return true if the thread was successfully started.
     */ 
    bool startThread();

    /**
     * The main body of the data saver thread.
     *  @return nullptr.
     */
    void* threadBody();

private:
    /** Unique ID that is given to each image. */
    unsigned long mUid;
    /** Path to folder where data is to be saved. */
    std::string mFolderName;
    /** The latest image received from the camera talker. */
    cv::Mat mImage;
    /** The latest drive command received from the talker. */
    DriveCommands mDriveCommands;
};
