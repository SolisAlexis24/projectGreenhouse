/**
 *************************************
 * @file: PIDControl.c
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */
#include "PIDControl.h"

void setDesiredValue(PIDController *pidC, float desiredVal){
	if(NULL == pidC){
		return;
	}
	pidC->desiredVal = desiredVal;
}

void setPIDGains(PIDController *pidC, float Kp, float Ki, float Kd){
	if(NULL == pidC){
		return;
	}
	pidC->Kp = Kp;
	pidC->Ki = Ki;
	pidC->Kd = Kd;
}

void setPIDMaxAndMinVals(PIDController *pidC, float min ,float max){
	if(NULL == pidC){
		return;
	}
	pidC->maxOutput = max;
	pidC->minOutput = min;
}


float computePIDOutput(PIDController *pidC, float inputVal){
	if(NULL == pidC){
		return 0;
	}
	float error = pidC->desiredVal - inputVal;
	pidC->IntegralVal += error;
	float derivativeVal = error - pidC->prevError;
	float outVal = pidC->Kp * error + pidC->Ki * pidC->IntegralVal + pidC->Kd * derivativeVal;
	if(outVal > pidC->maxOutput)
		outVal = pidC->maxOutput;
	else if (outVal < pidC->minOutput)
		outVal = pidC->minOutput;

	pidC->prevError = error;
	return outVal;
}
