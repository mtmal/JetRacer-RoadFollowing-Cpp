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
#include <NvidiaRacer.h>
#include "Configuration.h"
#include "StateMachine.h"

namespace
{
/** Define iterator to reduce typing, but avoid using auto which pollutes reading. */
typedef std::unordered_map<int, std::function<void(StateMachine&, const short)>>::iterator ActionsMapIterator;

const std::string stateToStr(const E_State state)
{
    switch (state)
    {
        case RC: return "RC";
        case RC_IMAGES: return "RC_IMAGES";
        case ML: return "ML";
        case UNUSED:
        default: return "UNUSED";
    }
}

bool strToBool(const std::string& value)
{
    return value == "true" || value == "True" || value == "1";
}
} // end of anonymouse namespace 

StateMachine::StateMachine(const Configuration& config, ICameraTalker* camera)
: GenericListener<GamepadEventData>(),
  GenericTalker<DriveCommands>(),
  mConfig(config),
  mRacer(-1),
  mOled(std::stoi(mConfig.at("oledAddress"), nullptr, 0), std::stoi(mConfig.at("oledMaxWait"))),
  mCamera(camera),
  mDataSaver(config),
  mState(RC),
  mPreviousState(RC),
  mGamepad(),
  mGamepadDrive(std::stoi(mConfig.at("steeringAxis")), std::stoi(mConfig.at("throttleAxis"))),
  mTorchDrive(),
  mRcOverride(false)
{
    sem_init(&mSemaphore, 0, 0);

    mRacer.setSteeringGain(std::stof(mConfig.at("steeringGain")));
    mRacer.setSteeringOffset(std::stof(mConfig.at("steeringOffset")));
    mRacer.setThrottleGain(std::stof(mConfig.at("throttleGain")));

    mAxisActions[std::stoi(mConfig.at("statePageAxis"))] = &StateMachine::processStatePageAxis;

    mButtonActions[std::stoi(mConfig.at("stopButton"))] = &StateMachine::processStopButton;
    mButtonActions[std::stoi(mConfig.at("stateConfButton"))] = &StateMachine::processStateConfButton;
    mButtonActions[std::stoi(mConfig.at("rcOverrideButton"))] = &StateMachine::processRcOverrideButton;

    mGamepad.registerTo(this);
    mGamepad.registerTo(&mGamepadDrive);
    mRacer.registerTo(&mGamepadDrive);
    mRacer.registerTo(&mTorchDrive);
}

StateMachine::~StateMachine()
{
    stop();
    sem_destroy(&mSemaphore);
}

bool StateMachine::initialise()
{
    if (mOled.initialise((mConfig.at("jetracerDevice").c_str())))
    {
        puts("OLED inititialised");
    }
    else
    {
        puts("Failed to initialise OLED display.");
        return false;
    }

    if (mRacer.initialise(mConfig.at("jetracerDevice").c_str()))
    {
        puts("Racer inititialised");
    }
    else
    {
        puts("Failed to initialise jetracer");
        return false;
    }

    if (mGamepad.initialise(mConfig.at("gamepadDevice").c_str()))
    {
        if (mGamepad.startThread())
        {
            puts("Gamepad inititialised");
        }
        else
        {
            puts("Failed to start gamepad thread");
            return false;
        }
    }
    else
    {
        puts("Failed to initialise gamepad");
        return false;
    }
    
    return true;
}

void StateMachine::stop()
{
    mDataSaver.stopThread(true);
    mGamepad.stopThread();
    mGamepad.unregisterFrom(this);
    mGamepad.unregisterFrom(&mGamepadDrive);
    mRacer.unregisterFrom(&mGamepadDrive);
    mRacer.unregisterFrom(&mTorchDrive);
    mCamera->stopCamera();
    mRacer.setThrottle(0.0f);
}

void StateMachine::update(const GamepadEventData& eventData)
{
    if (eventData.mIsAxis)
    {
        ActionsMapIterator it = mAxisActions.find(eventData.mNumber);
        if (it != mAxisActions.end())
        {
            (it->second)(*this, eventData.mValue);
        }
    }
    else
    {
        ActionsMapIterator it = mButtonActions.find(eventData.mNumber);
        if (it != mButtonActions.end())
        {
            (it->second)(*this, eventData.mValue);
        }
    }
}

void StateMachine::processStopButton(const short value)
{
    printf("processStopButton, value=%d \n", value);
    stop();
    sem_post(&mSemaphore);
}

void StateMachine::processRcOverrideButton(const short value)
{
    printf("processRcOverrideButton, value=%d, current state=%s \n", value, stateToStr(mState).c_str());
    mRcOverride = static_cast<bool>(value);
    if (mRcOverride)
    {
        mPreviousState = mState;
        switch (mState)
        {
            case ML:
                mTorchDrive.pause();
                [[fallthrough]];
            case RC_IMAGES:
                mCamera->pause();
                mRacer.update(DriveCommands(0.0f, 0.0f));
                break;
            default:
                break;
        }
    }
    else
    {
        switch (mPreviousState)
        {
            case ML:
                mTorchDrive.resume();
                [[fallthrough]];
            case RC_IMAGES:
                mCamera->resume();
                break;
            default:
                break;
        }
        mState = mPreviousState;
    }
}

void StateMachine::processStateConfButton(const short value)
{
    if (mRcOverride && mState != mPreviousState)
    {
        printf("processStateConfButton, value=%d, switching to state=%s \n", value, stateToStr(mState).c_str());
        switch (mState)
        {
            case RC:
                if (mCamera->isRunning())
                {
                    puts("Stopping camera");
                    mCamera->stopCamera();
                }
                if (mDataSaver.isRunning())
                {
                    puts("Stopping datasaver thread");
                    mDataSaver.stopThread(true);
                }
                puts("Unregistering data saver");
                static_cast<GenericListener<DriveCommands>&>(mDataSaver).unregisterFrom(&mGamepadDrive);
                static_cast<GenericListener<CameraData>&>(mDataSaver).unregisterFrom(mCamera);
                puts("Unregistering torch drive");
                static_cast<GenericListener<CameraData>&>(mTorchDrive).unregisterFrom(mCamera);
                break;
            case RC_IMAGES:
                puts("Unregistering torch drive");
                static_cast<GenericListener<CameraData>&>(mTorchDrive).unregisterFrom(mCamera);
                puts("Registering data saver");
                static_cast<GenericListener<DriveCommands>&>(mDataSaver).registerTo(&mGamepadDrive);
                static_cast<GenericListener<CameraData>&>(mDataSaver).registerTo(mCamera);
                if (!mCamera->isRunning())
                {
                    puts("Starting camera");
                    startCamera();
                }
                if (!mDataSaver.isRunning())
                {
                    puts("Starting datasaver thread");
                    mDataSaver.startThread();
                }
                break;
            case ML:
                if (mDataSaver.isRunning())
                {
                    puts("Stopping datasaver thread");
                    mDataSaver.stopThread(true);
                }
                if (!mTorchDrive.isInitialised())
                {
                    puts("Initilising torch inference");
                    mTorchDrive.initialise(mConfig.at("model"), 
                                           cv::Size(std::stoi(mConfig.at("width")), std::stoi(mConfig.at("height"))), 
                                           strToBool(mConfig.at("isMono")), 
                                           strToBool(mConfig.at("tta")));
                }
                puts("Unregistering data saver");
                static_cast<GenericListener<DriveCommands>&>(mDataSaver).unregisterFrom(&mGamepadDrive);
                static_cast<GenericListener<CameraData>&>(mDataSaver).unregisterFrom(mCamera);
                puts("Registering torch drive");
                static_cast<GenericListener<CameraData>&>(mTorchDrive).registerTo(mCamera);
                if (!mCamera->isRunning())
                {
                    puts("Starting camera");
                    startCamera();
                }
                break;
            default:
                break;
        }
        mPreviousState = mState;
    }
}

void StateMachine::processStatePageAxis(const short value)
{
    if (mRcOverride && value != 0)
    {
        int tempState = static_cast<int>(mState) + static_cast<int>(std::copysign(1, value));
        if (tempState < 0)
        {
            mState = static_cast<E_State>(static_cast<int>(UNUSED) - 1);
        }
        else if (tempState >= static_cast<int>(UNUSED))
        {
            mState = static_cast<E_State>(0);
        }
        else
        {
            mState = static_cast<E_State>(tempState);
        }
        printf("Going from state: %s to state: %s \n", stateToStr(mPreviousState).c_str(), stateToStr(mState).c_str());
        mOled.selectImage(mState);
    }
}

void StateMachine::startCamera()
{
    cv::Size imageSize(std::stoi(mConfig.at("width")), std::stoi(mConfig.at("height")));
    bool isMono = strToBool(mConfig.at("isMono"));
    std::vector<uint8_t> ids;
    if (isMono)
    {
        ids.push_back(std::stoi(mConfig.at("monoID")));
    }
    else
    {
        ids.push_back(0);
        ids.push_back(1);
    }

    mCamera->startCamera(imageSize, std::stoi(mConfig.at("framerate")), 0, ids, 2, isMono, !isMono);
}
