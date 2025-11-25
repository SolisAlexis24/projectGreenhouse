/**
 *************************************
 * @file: PIDControl.c
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */
#include "PIDControl.h"

void setPIDDesiredValue(PIDController *pidC, float desiredVal){
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
	if(pidC == NULL){
		return 0;
	}

	// Tiempo actual en segundos
	float now = (float)xTaskGetTickCount() * (float)portTICK_PERIOD_MS / 250.0f;
	float dt = now - pidC->lastTime;
	if (dt <= 0.0f){
		dt = 0.001f;
	}

	float error = pidC->desiredVal - inputVal;

	pidC->IntegralVal += error * dt;

	float derivativeVal = (error - pidC->prevError) / dt;

	float outVal = pidC->Kp * error + pidC->Ki * pidC->IntegralVal + pidC->Kd * derivativeVal;

	if(outVal > pidC->maxOutput) 
		outVal = pidC->maxOutput;
	else if (outVal < pidC->minOutput) 
		outVal = pidC->minOutput;

	// Actualizar memoria
	pidC->prevError = error;
	pidC->lastTime = now;

	return outVal;
}

