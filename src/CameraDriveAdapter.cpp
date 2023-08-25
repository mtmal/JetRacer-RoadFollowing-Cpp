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

#include "CameraDriveAdapter.h"

CameraDriveAdapter::CameraDriveAdapter(const std::string& pathToModel, const cv::Size& imageSize, const bool isMono)
: GenericListener<CameraData>(),
  GenericTalker<DriveCommands>()
{
    mTorchInference.initialise(pathToModel, isMono ? imageSize.width : imageSize.width * 2, imageSize.height, isMono ? 3 : 1);
}

CameraDriveAdapter::~CameraDriveAdapter()
{
}

void CameraDriveAdapter::update(const CameraData& camData)
{
    if (camData.mImage.size() == 1)
    {
        mTorchInference.processImage(camData.mImage[0].createMatHeader(), mOutput).flatten();
    }
    else
    {
        cv::Mat img(camData.mImage[0].rows, camData.mImage[0].cols + camData.mImage[1].cols, CV_8UC1);
        camData.mImage[0].createMatHeader().copyTo(img(cv::Rect(0, 0, camData.mImage[0].cols, camData.mImage[0].rows)));
        camData.mImage[1].createMatHeader().copyTo(img(cv::Rect(camData.mImage[0].cols, 0, camData.mImage[1].cols, camData.mImage[1].rows)));
        mTorchInference.processGreyImage(img, mOutput).flatten();
    }
    notifyListeners(DriveCommands(mOutput[0].item().toFloat(), mOutput[1].item().toFloat()));
}