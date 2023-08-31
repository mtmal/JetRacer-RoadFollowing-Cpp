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
  mRacer(),
  mOled(std::stoi(mConfig.at("oledAddress"), nullptr, 0), std::stoi(mConfig.at("oledMaxWait"))),
  mCamera(camera),
  mDataSaver(config),
  mState(RC),
  mPreviousState(RC),
  mGamepad(),
  mGamepadDrive(std::stoi(mConfig.at("steeringAxis")), std::stoi(mConfig.at("throttleAxis"))),
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

}

StateMachine::~StateMachine()
{
    stop();
    sem_destroy(&mSemaphore);
}

bool StateMachine::initialise()
{
    if (!mRacer.initialise(mConfig.at("jetracerDevice").c_str()))
    {
        puts("Failed to initialise jetracer");
        return false;
    }

    if (!mOled.initialise((mConfig.at("jetracerDevice").c_str())))
    {
        puts("Failed to initialise OLED display.");
        return false;
    }

    if (!mGamepad.initialise(mConfig.at("gamepadDevice").c_str()))
    {
        puts("Failed to initialise gamepad");
        return false;
    }

    if (!mGamepad.startEventThread())
    {
        puts("Failed to start gamepad thread");
        return false;
    }
    
    return true;
}

void StateMachine::stop()
{
    mDataSaver.stop();
    mGamepad.stopEventThread();
    mGamepad.unregisterFrom(this);
    mGamepad.unregisterFrom(&mGamepadDrive);
    mRacer.unregisterFrom(&mGamepadDrive);
    mRacer.setThrottle(0.0f);
    mCamera->stopCamera();
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

void StateMachine::processRcState()
{

}

void StateMachine::processMlState()
{

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
            case RC_IMAGES:
            case ML:
                puts("Pausing camera");
                mCamera->pause();
                break;
            default:
                break;
        }
    }
    else
    {
        switch (mPreviousState)
        {
            case RC_IMAGES:
            case ML:
                puts("Resuming camera");
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
                puts("Stopping camera");
                if (mCamera->isRunning())
                {
                    mCamera->stopCamera();
                }
                puts("Unregistering data saver");
                static_cast<GenericListener<DriveCommands>&>(mDataSaver).unregisterFrom(&mGamepadDrive);
                static_cast<GenericListener<CameraData>&>(mDataSaver).unregisterFrom(mCamera);
                break;
            case RC_IMAGES:
                puts("Registering data saver");
                static_cast<GenericListener<DriveCommands>&>(mDataSaver).registerTo(&mGamepadDrive);
                static_cast<GenericListener<CameraData>&>(mDataSaver).registerTo(mCamera);
                puts("Starting camera");
                if (!mCamera->isRunning())
                {
                    startCamera();
                }
                break;
            case ML:
                puts("Unregistering data saver");
                static_cast<GenericListener<DriveCommands>&>(mDataSaver).unregisterFrom(&mGamepadDrive);
                static_cast<GenericListener<CameraData>&>(mDataSaver).unregisterFrom(mCamera);
                puts("Starting camera");
                if (!mCamera->isRunning())
                {
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