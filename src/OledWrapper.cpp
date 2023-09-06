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

#include <cerrno>
#include <ctime>
#include <ScopedLock.h>
extern "C" {
#include <GUI_Paint.h>
}
#include "OledWrapper.h"

OledWrapper::OledWrapper(const uint8_t deviceAddress, const uint8_t maxSleepTime)
: GenericThread<OledWrapper>(),
  mOled(deviceAddress),
  mImages{},
  mSleepTime(maxSleepTime)
{
    // prepare all images beforehand
    Paint_SelectImage(mImages[static_cast<uint8_t>(E_State::RC)]);
    OLED_0in91::drawText("Remote    Controlled", 10, 0, &Font16, mImages[static_cast<uint8_t>(E_State::RC)]);
    Paint_SelectImage(mImages[static_cast<uint8_t>(E_State::RC_IMAGES)]);
    OLED_0in91::drawText("Acquiring Images", 10, 0, &Font16, mImages[static_cast<uint8_t>(E_State::RC_IMAGES)]);
    Paint_SelectImage(mImages[static_cast<uint8_t>(E_State::ML)]);
    OLED_0in91::drawText("Road      Following", 10, 0, &Font16, mImages[static_cast<uint8_t>(E_State::ML)]);
}

OledWrapper::~OledWrapper()
{
    stopThread(true);
    mOled.clear();
}

bool OledWrapper::initialise(const std::string& device)
{
    if (mOled.initialise(device))
    {
        mOled.clear();
        return startThread();
    }
    return false;
}

void OledWrapper::selectImage(const E_State state)
{
    if (state != E_State::UNUSED)
    {
        ScopedLock lock(mMutex);
        mOled.display(mImages[static_cast<uint8_t>(state)]);
        sem_post(&mSemaphore);
    }
}

void* OledWrapper::threadBody()
{
    int ret;
    struct timespec ts;
    while (isRunning())
    {
        // if there was an error the thread could have been canceled, or some other error may have happened
        if (0 == sem_wait(&mSemaphore))
        {
            // wait for specified time. Timeouts are OK. If semaphore was posted, we need to sleep again
            do
            {
                clock_gettime(CLOCK_REALTIME, &ts);
                ts.tv_sec += mSleepTime;
                ret = sem_timedwait(&mSemaphore, &ts);
            } while (0 == ret);

            // if we timed out, we need to clear the OLED display
            if (-1 == ret && errno == ETIMEDOUT)
            {
                ScopedLock lock(mMutex);
                mOled.clear();
            }
        }
    }
    return nullptr;
}
