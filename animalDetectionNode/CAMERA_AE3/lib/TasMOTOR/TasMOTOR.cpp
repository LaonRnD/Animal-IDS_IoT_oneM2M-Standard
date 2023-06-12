/**
* Copyright (c) 2018, OCEAN
* All rights reserved.
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
* 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
TasLED.cpp - Library for LED control
Created by Ilyeup Ahn in KETI Korea, March 06, 2018.
Released into the public domain.
*/

#include "TasMOTOR.h"

//ULN2003 Motor Driver Pins for esp32-cam
#define IN1 12
#define IN2 13
#define IN3 15
#define IN4 14

const uint16_t stepsPerRevolution = 256;//8 step // 1cycle : 2048;
Stepper camStepper(stepsPerRevolution, IN1, IN3, IN2, IN4); 
	
TasMOTOR::TasMOTOR()
{

}

void TasMOTOR::init() {
    angle = 0;    
}

void TasMOTOR::setSPEED(uint16_t speed) {
    
	camStepper.setSpeed(speed);	
}

void TasMOTOR::setMOTOR(uint8_t value) {
	if (value == 1) {
        camStepper.step(stepsPerRevolution); //clockwise rotate      
        if(angle>=8){
		  angle = 0;
	    }else{
		  angle += 1;
	    }    
    }
	else if(value == 2) {
        camStepper.step(-stepsPerRevolution); //counterclockwise  rotate       
        if(angle<=0){
		  angle = 0;
	    } else {
		  angle -= 1;
	    }   
    }
	else if(value == 3) {
		motor_step = 2048;//clockwies 360도 
        camStepper.step(motor_step); //counterclockwise  rotate       
        
	}
	else if(value == 4) {
        motor_step = -2048;//clockwies 360도 
        camStepper.step(motor_step); //counterclockwise  rotate    
    }

}

uint8_t TasMOTOR::getMOTOR() {
    return angle;
}
