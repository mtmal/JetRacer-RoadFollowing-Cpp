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

#include <CameraData.h>
#include <DriveCommands.h>
#include <TorchInference.h>
#include <GenericListener.h>
#include <GenericTalker.h>


/**
 * Class that takes images, passes them through libtorch and notifies jetracer with drive commands.
 */
class CameraDriveAdapter : public GenericListener<CameraData>,
                           public GenericTalker<DriveCommands>
{
public:
    /**
     * Initialises the underhood TorchInference class.
     */
    CameraDriveAdapter();

    /**
     * Class destructor.
     */
    virtual ~CameraDriveAdapter();

    /**
     * Initialises the torch inference class.
     *  @param pathToModel path to JIT model for liborch to load.
     *  @param imageSize size of a single image. For stereo camera double width is taken.
     *  @param isMono true for a mono camera system.
     *  @param tta true to enable test time augmentations.
     */
    void initialise(const std::string& pathToModel, const cv::Size& imageSize, const bool isMono, const bool tta);

    /**
     * Receives camera images to predict drive commands for JetRacer.
     *  @param camData the camera data, can be from either mono or stereo camera.
     */
    void update(const CameraData& camData) override;

    /**
     *  @return true of the class was initialised.
     */
    inline bool isInitialised() const
    {
        return mIsInitialised;
    }

private:
    /**
     * Converts results into drive command.
     *  @param result the result of inference.
     *  @param driveCommands the drive commands to be issued.
     */
    void processResults(const at::Tensor& results, DriveCommands& driveCommands) const;

    /** Flag to indicate if test time augmentations should be applied. */
    bool mTTA;
    /** Flag to indicate if the class was initialised. */
    bool mIsInitialised;
    /** Wrapper class to perform Torch inference. */
    TorchInference mTorchInference;
};
