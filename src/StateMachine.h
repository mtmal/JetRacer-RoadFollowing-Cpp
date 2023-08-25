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
#include "DataSaver.h"

class Configuration;

enum E_States
{
    RC          = 0, // remote controlled
    RC_IMAGES   = 1, // remote controlled with images being save with steering and throttle
    ML          = 2, // model controlled
    UNUSED      = 3  // mode value used for iterations
};

class StateMachine : public GenericListener<GamepadEventData>,
                     public GenericTalker<DriveCommands>
{
public:
    explicit StateMachine(const Configuration& config, ICameraTalker* camera);
    virtual ~StateMachine();

    bool initialise();

    void stop();

    void update(const GamepadEventData& eventData) override;

    inline sem_t* getSem()
    {
        return &mSemaphore;
    }

protected:
    void processRcState();
    void processMlState();

    void processStopButton(const short value);
    void processRcOverrideButton(const short value);
    void processStateConfButton(const short value);

    void processStatePageAxis(const short value);

private:
    void startCamera();

    const Configuration& mConfig;
    NvidiaRacer mRacer;

    ICameraTalker* mCamera;

    DataSaver mDataSaver;

    /** The current state of the state machine. */
    E_States mState;
    E_States mPreviousState;


	/** Gamepad class. */
	Gamepad mGamepad;
    GamepadDriveAdapter mGamepadDrive;

    bool mRcOverride;
    sem_t mSemaphore;

    std::unordered_map<int, std::function<void(StateMachine&, const short)>> mAxisActions;
    std::unordered_map<int, std::function<void(StateMachine&, const short)>> mButtonActions;

};
