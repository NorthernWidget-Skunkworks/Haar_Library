
#ifndef HAAR_h
#define HAAR_h

#include "Arduino.h"
#include <Wire.h>

enum Sensor {
	RH_Sense = 0,
	Pres_Sense = 1
};

#define CTRL1 0x00
#define TEMP_PRES 0x02
#define TEMP_RH 0x04
#define PRES_REG 0x06
#define RH_REG 0x09

#define ON 1
#define OFF 0

class Haar
{
	public:
		Haar();
		uint8_t begin(uint8_t ADR_ = 0x40); //use default address
		float GetPressure(bool Update = false);
		float GetHumidity(bool Update = false);
		float GetTemperature(Sensor Device = RH_Sense, bool Update = false); //By default use temp from RH sensor
		uint8_t Sleep(bool State = ON); //Set device into sleep mode 
		uint8_t RequestSample(bool Block = false); //Default to non-blocking
		bool NewData();
		String GetString();
		String GetHeader();
	private:
		uint8_t ADR = 0x40; //Default global sensor address 
		int16_t GetWord(uint8_t Reg);
		unsigned long TimeoutGlobal = 500; //Timeout value, ms //FIX??
		bool DataRequested = false; //Flag for keeping track of data requests and data retrevals 
};

#endif