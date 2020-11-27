#ifndef AutoPilotModule_h
#define AutoPilotModule_h

#include <Arduino.h>
#include "SensorModule.h"
#include "DrivingModule.h"
#include "CommandModule.h"

#define SIDE_SENSOR_PADDING 3
#define SIDE_SENSOR_COLLISION_THRESHOLD 4 + SIDE_SENSOR_PADDING
#define SIDE_SENSOR_COLLISION_WARNING_THRESHOLD 7 + SIDE_SENSOR_PADDING
#define SIDE_SENSOR_COLLISION_WARNING_THRESHOLD 7 + SIDE_SENSOR_PADDING
#define FRONT_SENSOR_COLLISION_THRESHOLD 7
#define SIDE_SENSOR_CLEAR_THRESHOLD 20 + SIDE_SENSOR_PADDING
#define CENTER_TOLERANCE 2

class AutoPilotModule
{
public:
    AutoPilotModule(HardwareSerial *stream,
                    DrivingModule *drivingModule,
                    CommandModule *commandModule);

    void handle(SensorResult *sensorResult);

private:
    HardwareSerial *_stream;
    DrivingModule *_drivingModule;
    CommandModule *_commandModule;

    bool isCentered(SensorResult *sensorResult);
    bool spaceAhead(SensorResult *sensorResult);
    bool isTrapped(SensorResult *sensorResult);
};

#endif