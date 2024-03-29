#include "AutoPilotModule.h"
#include "Debug.h"

AutoPilotModule::AutoPilotModule(Stream *stream,
                                 IDrivingModule *drivingModule,
                                 IBumperModule *bumperModule,
                                 ISensorModule *sensorModule,
                                 SensorResult* sensorResult)
{
    _stream = stream;
    _drivingModule = drivingModule;
    _bumperModule = bumperModule;
    _sensorModule = sensorModule;
    _sensorResult = sensorResult;
}

bool AutoPilotModule::enabled()
{
    return _state != State::DISABLED;
}

void AutoPilotModule::enable()
{
    _state = State::RESETTING;
}

void AutoPilotModule::disable()
{
    _state = State::DISABLED;
}

void AutoPilotModule::handle()
{
    switch (_state)
    {
        case State::DISABLED:
            handleDisabled();
            return;
        case State::RESETTING:
            handleResetting();
            return;
        case State::SCANNING:
            handleScanning();
            return;
        case State::DECIDING:
            handleDeciding();
            return;
        case State::REQUESTING:
            handleRequesting();
            return;
    }
}

void AutoPilotModule::handleDisabled()
{
    if (hasCollided())
    {
        _drivingModule->stop();
    }
}

void AutoPilotModule::handleResetting()
{
    _drivingModule->stop();
    _sensorModule->reset();
    _state = State::SCANNING;
}

void AutoPilotModule::handleScanning()
{
    if (!_sensorModule->scanOnce(_sensorResult))
    {
        return;
    }
    
    if (isTrapped())
    {
        _state = State::RESETTING;
        return;
    }

    _state = State::DECIDING;
}

void AutoPilotModule::handleDeciding()
{
    if (hasCollided())
    {
        _state = State::RESETTING;
        return;
    }

    if (outOfDate())
    {
        _state = State::RESETTING;
        return;
    }

    _state = State::REQUESTING;

    if (!spaceAhead())
    {
        _drivingModule->setSpeed(TURNING_SPEED);
        if (_sensorResult->Right.Distance > _sensorResult->Left.Distance)
        {
            _drivingModule->turnRight();
            return;
        }
        if (_sensorResult->Left.Distance > _sensorResult->Right.Distance)
        {
            _drivingModule->turnLeft();
            return;
        }
        // if equal, bias for right
        _drivingModule->turnRight();
        return;
    }

    if (_sensorResult->Left.Distance < SIDE_SENSOR_COLLISION_THRESHOLD)
    {
        _drivingModule->setSpeed(TURNING_SPEED);
        _drivingModule->turnRight();
        return;
    }
    if (_sensorResult->Right.Distance < SIDE_SENSOR_COLLISION_THRESHOLD)
    {
        _drivingModule->setSpeed(TURNING_SPEED);
        _drivingModule->turnLeft();
        return;
    }

    if (_sensorResult->Front.Distance > FRONT_SENSOR_COLLISION_WARNING_THRESHOLD)
    {
        _drivingModule->setSpeed(CLEAR_SPEED);
    }
    else if (_sensorResult->Front.Distance > FRONT_SENSOR_COLLISION_THRESHOLD)
    {
        _drivingModule->setSpeed(WARNING_SPEED);
    }

    if (_sensorResult->Left.Distance < SIDE_SENSOR_COLLISION_WARNING_THRESHOLD)
    {
        _drivingModule->bearRight(DIR_FORWARD);
        return;
    }
    if (_sensorResult->Right.Distance < SIDE_SENSOR_COLLISION_WARNING_THRESHOLD)
    {
        _drivingModule->bearLeft(DIR_FORWARD);
        return;
    }

    if (isCentered())
    {
        _drivingModule->moveForward();
        return;
    }

    if (isOneSideClear())
    {
        _drivingModule->moveForward();
        return;
    }

    if (_sensorResult->Left.Distance < _sensorResult->Right.Distance)
    {
        _drivingModule->bearRight(DIR_FORWARD);
        return;
    }

    _drivingModule->bearLeft(DIR_FORWARD);
}

void AutoPilotModule::handleRequesting()
{
    if (hasCollided())
    {
        _state = State::RESETTING;
        return;
    }

    if (!_sensorModule->request(_sensorResult, Direction::FRONT))
    {
        return;
    }

    _state = State::DECIDING;
}

void AutoPilotModule::handle2()
{
    if (!enabled())
    {
        return;
    }

    _sensorModule->scan(_sensorResult);
    
    if (outOfDate())
    {
        debugPrintln(F("Sensor result is out of date"));
        _drivingModule->stop();
        return;
    }

    if (isTrapped())
    {
        _drivingModule->stop();
        return;
    }

    if (!spaceAhead())
    {
        if (_sensorResult->Right.Distance > _sensorResult->Left.Distance)
        {
            _drivingModule->turnRight();
            return;
        }
        if (_sensorResult->Left.Distance > _sensorResult->Right.Distance)
        {
            _drivingModule->turnLeft();
            return;
        }
        // if equal, bias for right
        _drivingModule->turnRight();
        return;
    }

    if (_sensorResult->Left.Distance < SIDE_SENSOR_COLLISION_THRESHOLD)
    {
        _drivingModule->turnRight();
        return;
    }
    if (_sensorResult->Right.Distance < SIDE_SENSOR_COLLISION_THRESHOLD)
    {
        _drivingModule->turnLeft();
        return;
    }

    if (_sensorResult->Left.Distance < SIDE_SENSOR_COLLISION_WARNING_THRESHOLD)
    {
        _drivingModule->bearRight(DIR_FORWARD);
        return;
    }
    if (_sensorResult->Right.Distance < SIDE_SENSOR_COLLISION_WARNING_THRESHOLD)
    {
        _drivingModule->bearLeft(DIR_FORWARD);
        return;
    }

    if (isCentered())
    {
        _drivingModule->moveForward();
        return;
    }

    if (isOneSideClear())
    {
        _drivingModule->moveForward();
        return;
    }

    if (_sensorResult->Left.Distance < _sensorResult->Right.Distance)
    {
        _drivingModule->bearRight(DIR_FORWARD);
        return;
    }

    _drivingModule->bearLeft(DIR_FORWARD);
    return;
}

bool AutoPilotModule::outOfDate()
{
    unsigned long now = millis();
    unsigned long frontSensorResultAge = now - _sensorResult->Front.Timestamp;

    if (frontSensorResultAge > MAX_FRONT_SENSOR_RESULT_AGE)
    {
        debugPrintf("** Sensor result is out of date\n");
        debugPrintf("** Result: %lu\n", _sensorResult->Front.Timestamp);
        debugPrintf("** Now: %lu\n", now);
        debugPrintf("** Max Age: %hu\n", MAX_FRONT_SENSOR_RESULT_AGE);
        debugPrintf("** Age: %lu\n", frontSensorResultAge);
        return true;
    }

    unsigned long leftSensorResultAge = now - _sensorResult->Left.Timestamp;
    if (leftSensorResultAge > MAX_SIDE_SENSOR_RESULT_AGE)
    {
        return true;
    }

    unsigned long rightSensorResultAge = now - _sensorResult->Right.Timestamp;
    if (rightSensorResultAge > MAX_SIDE_SENSOR_RESULT_AGE)
    {
        return true;
    }

    return false;
}

bool AutoPilotModule::isCentered()
{
    if ((_sensorResult->Left.Distance - CENTER_TOLERANCE) < _sensorResult->Right.Distance && _sensorResult->Right.Distance < (_sensorResult->Left.Distance + CENTER_TOLERANCE))
    {
        return true;
    }
    if ((_sensorResult->Right.Distance - CENTER_TOLERANCE) < _sensorResult->Left.Distance && _sensorResult->Left.Distance < (_sensorResult->Right.Distance + CENTER_TOLERANCE))
    {
        return true;
    }
    return false;
}

bool AutoPilotModule::isOneSideClear()
{
    if (_sensorResult->Left.Distance > SIDE_SENSOR_CLEAR_THRESHOLD)
    {
        return true;
    }
    if (_sensorResult->Right.Distance > SIDE_SENSOR_CLEAR_THRESHOLD)
    {
        return true;
    }
    return false;
}

bool AutoPilotModule::spaceAhead()
{
    return _sensorResult->Front.Distance > FRONT_SENSOR_COLLISION_THRESHOLD;
}

bool AutoPilotModule::isTrapped()
{
    if (_sensorResult->Right.Distance > SIDE_SENSOR_COLLISION_THRESHOLD)
    {
        return false;
    }
    if (_sensorResult->Left.Distance > SIDE_SENSOR_COLLISION_THRESHOLD)
    {
        return false;
    }
    return true;
}

bool AutoPilotModule::hasCollided()
{
    return _bumperModule->hasCollided(5) != Sides::NONE;
}