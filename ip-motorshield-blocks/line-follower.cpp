#include "line-follower.h"
#include "config.h"
#include "robot.h"
#include "shared-globals.h"
#include <Arduino.h>

float calculatePIDstep(PIDController &pid, float measurement, float executionFrequency)
{
    pidError = (int)(pid.target - measurement);

    // integral windup prevention
    const float maximumOutput = PWM_MAX_VALUE;
    float integralLimit = maximumOutput / pid.ki;
    float dt = 1.0f / executionFrequency; // normally 40 Hz, so dt = 0.025s
    pid.integral += pidError * dt;
    pid.integral = constrain(pid.integral, -integralLimit, integralLimit);

    // integral reset to prevent overshooting after long build-up when approaching the line from far away
    bool isCrossingLine = (pidError > 0) != (pid.prevError > 0);
    if (isCrossingLine)
    {
        pid.integral = 0;
    }

    // Derivative term (indiferent if based on pidError or measurement, since target is constant (=0))
    // (pidError - previousError) / dt;
    // == (-measurement - (-prevMeasurement)) / dt;  if target is constant
    // == -(measurement - prevMeasurement) / dt;
    float derivative = (pidError - pid.prevError) / dt;

    float control = pid.kp * pidError + pid.ki * pid.integral + pid.kd * derivative;

    pid.prevError = pidError;

    return control;
}

void updateLineStatus()
{
    for (uint8_t i = 0; i < QTR_SENSOR_COUNT; i++)
    {
        if (qtrSensorValues[i] > lineThreshold)
        {
            qtrSensorStatus[i] = 1;
        }
        else
        {
            qtrSensorStatus[i] = 0;
        }
    }

    if (qtrSensorStatus[0] == 1 && qtrSensorStatus[4] == 1 && (qtrSensorStatus[1] == 0 || qtrSensorStatus[2] == 0 || qtrSensorStatus[3] == 0))
    {
        lineStatus = STATION;
    }
    else if (qtrSensorStatus[0] == 1 && qtrSensorStatus[1] == 1 && qtrSensorStatus[2] == 1 && qtrSensorStatus[3] == 1 && qtrSensorStatus[4] == 1)
    {
        lineStatus = STOP;
    }
    else if (qtrSensorStatus[0] == 0 /*&& qtrSensorStatus[1] == 0*/ && qtrSensorStatus[2] == 1 /*&& qtrSensorStatus[3] == 0*/ && qtrSensorStatus[4] == 0)//TEST there could be some optimisation to improve line tracking consistency
    {
        lineStatus = NORMAL;
    }
    else {
        lineStatus = ROTATION;
    }

}
