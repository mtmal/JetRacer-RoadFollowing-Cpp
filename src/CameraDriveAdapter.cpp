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

#include <cstdio>
#include <ATen/ATen.h>
#include "CameraDriveAdapter.h"

namespace
{
/**
 * Converts bool to +1 (true) or -1 (false).
 *  @param value bool to convert
 *  @return +1 or -1 depending on @p value.
 */
inline int boolToInt(const bool value)
{
    return static_cast<int>(value) * 2 - 1;
}
} /* end of anonymous namespace */

CameraDriveAdapter::CameraDriveAdapter()
: GenericListener<CameraData>(),
  GenericTalker<DriveCommands>(),
  mTTA(false),
  mIsInitialised(false)
{
}

CameraDriveAdapter::~CameraDriveAdapter()
{
}

void CameraDriveAdapter::initialise(const std::string& pathToModel, const cv::Size& imageSize, const bool isMono, const bool tta)
{
    if (!isInitialised())
    {
        mTTA = tta;
        mTorchInference.initialise(pathToModel, imageSize.width, imageSize.height, isMono ? 3 : 1);
        mIsInitialised = true;
    }
}

void CameraDriveAdapter::update(const CameraData& camData)
{
    DriveCommands driveCommands;
    at::Tensor output;
    printf("Received image! \n");
    if (camData.mImage.size() == 1)
    {
        mTorchInference.processImage(mTTA, camData.mImage[0].createMatHeader(), output);
    }
    else
    {
        mTorchInference.processGreyImage(mTTA, camData.mImage[0].createMatHeader(), camData.mImage[1].createMatHeader(), output);
    }
    processResults(output, driveCommands);
    notifyListeners(driveCommands);
}

void CameraDriveAdapter::processResults(const at::Tensor& results, DriveCommands& driveCommands) const
{
    if (static_cast<int>(results.size(0)) > 0)
    {
        for (int i = 0; i < static_cast<int>(results.size(0)); ++i)
        {
            driveCommands.mSteering -= boolToInt(mTTA && (i >= (static_cast<int>(results.size(0)) / 2))) * results[i][0].item().toFloat();
            driveCommands.mThrottle += results[i][1].item().toFloat();
        }
        driveCommands.mSteering /= static_cast<int>(results.size(0));
        driveCommands.mThrottle /= static_cast<int>(results.size(0));
    }

}