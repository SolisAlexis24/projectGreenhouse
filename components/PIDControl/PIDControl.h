/**
 *************************************
 * @file: PIDControl.h
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */
#pragma once
#include <unistd.h>


typedef struct{
	float Kp;
	float Ki;
	float Kd;

	float desiredVal;
	float IntegralVal;
	float prevError;

	float maxOutput;
	float minOutput;
}PIDController;

void setPIDDesiredValue(PIDController *pidC, float desiredVal);

void setPIDGains(PIDController *pidC, float Kp, float Ki, float Kd);

void setPIDMaxAndMinVals(PIDController *pidC, float min ,float max);

float computePIDOutput(PIDController *pidC, float inputVal);

