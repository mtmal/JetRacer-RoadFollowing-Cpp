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

#include <functional>
#include <semaphore.h>
#include <Gamepad.h>
#include <GamepadDriveAdapter.h>
#include <ICameraTalker.h>
#include <NvidiaRacer.h>
#include "CameraDriveAdapter.h"
#include "DataSaver.h"
#include "OledWrapper.h"

class Configuration;

/**
 * The state machine that controls the operation of the Jetracer. States can be modified using gamepad.
 * There are three main states: remotely controlled (default), RC with image acquisition and saving to files,
 * and road following. There is additional temporary state which is entered that pauses image acquistion and
 * allows for a steady RC operation with cameras and other threads started. 
 */
class StateMachine : public GenericListener<GamepadEventData>,
                     public GenericTalker<DriveCommands>
{
public:
    /**
     * Basic constructor that initialises parameters, populates maps with gamepad axis and button actions, 
     * and registers listeners with talkers.
     *  @param config the main configuration.
     *  @param camera pointer to the camera, either mono or stereo.
     */
    StateMachine(const Configuration& config, ICameraTalker* camera);

    /**
     * Basic destructor that calls stop().
     */
    virtual ~StateMachine();

    /**
     * Initialises all classes and threads.
     *  @return true if the operation was successful.
     */
    bool initialise();

    /**
     * Stops all threads.
     */
    void stop();

    /**
     * Gamepad events for controlling the state machine.
     *  @param eventData gamepad event data.
     */
    void update(const GamepadEventData& eventData) override;

    /**
     *  @return the semaphore for the main thread to wait on it.
     */
    inline sem_t* getSem()
    {
        return &mSemaphore;
    }

protected:
    /**
     * Processes stop button event, i.e., stops all threads and exists the application.
     *  @param value the value of the button, 1 for pressed.
     */
    void processStopButton(const short value);

    /**
     * Processes RC override button event, i.e., pauses the camera and allows for RC to drive the racer.
     *  @param value the value of the button, 1 for pressed.
     */
    void processRcOverrideButton(const short value);

    /**
     * Processes the new state confirmation button event, i.e., the machine will enter selected state.
     *  @param value the value of the button, 1 for pressed.
     */
    void processStateConfButton(const short value);

    /**
     * Processes the state browsing axis event, i.e., loops over available states without entering them.
     *  @param value the value of the axis that determins the direction of states browsing.
     */
    void processStatePageAxis(const short value);

private:
    /**
     * Starts the camera. It checks whether to start mono or stereo camera based on the configuration.
     */
    void startCamera();

    /** The main configuration. */
    const Configuration& mConfig;
    /** The racer class. */
    NvidiaRacer mRacer;
    /** The OLED wrapper class. */
    OledWrapper mOled;
    /** Pointer to camera interface for either mono or stereo camera. */
    ICameraTalker* mCamera;
    /** The data saver class. */
    DataSaver mDataSaver;
    /** The current state of the state machine. */
    E_State mState;
    /** The previous state. */
    E_State mPreviousState;
	/** Gamepad class. */
	Gamepad mGamepad;
    /** An adapter class for converting gamepad inputs into drive commands. */
    GamepadDriveAdapter mGamepadDrive;
    /** An adapter class for converting images inputs into drive commands. */
    CameraDriveAdapter mTorchDrive;
    /** Flag indicating if the remote-controlled override state has been activated. */
    bool mRcOverride;
    /** Semaphore for pausing the main application thread. */
    sem_t mSemaphore;
    /** Map of actions for associated gamepad axis events. */
    std::unordered_map<int, std::function<void(StateMachine&, const short)>> mAxisActions;
    /** Map of actions for associated gamepad button events. */
    std::unordered_map<int, std::function<void(StateMachine&, const short)>> mButtonActions;
};
