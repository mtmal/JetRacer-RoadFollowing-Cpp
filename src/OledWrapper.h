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

#include <atomic>
#include <pthread.h>
#include <semaphore.h>
#include <OLED_0in91.h>
#include "E_State.h"

/**
 * A class that wrapps OLED library with additional functionality. Basically, it has pre-drawn image,
 * which will never be displayed for longer than configurable amount of seconds. This is to ensure
 * that the OLED is not on for too long.
 */
class OledWrapper
{
public:
    /**
     * Initialises all variables, prepares images, and clears the display.
     *  @param deviceAddress the address of the OLED display in the I2C bus.
     *  @param maxSleepTime the maximum duration in seconds how long display should be on after the last update.
     */
    OledWrapper(const uint8_t deviceAddress = 0x3c, const uint8_t maxSleepTime = 5);

    /**
     * Basic destructor. 
     */
    virtual ~OledWrapper();

    /**
     * Initialises the OLED display and starts a thread that clears the display after specified time since the last update.
     *  @param device the path to the I2C bus on which OLED is wired.
     */
    inline bool initialise(const std::string& device)
    {
        return mOled.initialise(device) && (0 == pthread_create(&mThread, nullptr, OledWrapper::startThread, this));
    }

    /**
     * Based on provided state, displays appropriate image. These images are just text descriptions of the state.
     *  @param state a main state for which image should be displayed.
     */
    void selectImage(const E_State state);

    /**
     * The main body of the thread that waits until OLED display can be cleared.
     */
    void waitingThread();

private:
    /**
     * Starts the waiting thread.
     *  @param instance an instance of this class.
     *  @return nullptr 
     */
    static void* startThread(void* instance);

    /** The OLED class. */
    OLED_0in91 mOled;
    /** Images for each state. */
    uint8_t mImages[static_cast<uint8_t>(E_State::UNUSED)][IMAGE_SIZE];
    /** Duration in seconds when the OLED should be cleared after the last update. */
    uint8_t mSleepTime;
    /** Flag indicating whether a thead should be running. */
    std::atomic<bool> mRun;
    /** The thread itself. */
    pthread_t mThread;
    /** Mutex for controlling access to the @p mOled */
    pthread_mutex_t mMutex;
    /** Semaphore used for timed wait. */
    sem_t mSemaphore;
};
