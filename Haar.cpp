#include "Haar.h"

Haar::Haar()
{
}

uint8_t Haar::begin(uint8_t ADR_)
{
	ADR = ADR_;
	Wire.begin();
	RequestSample();
}

float Haar::GetPressure(bool Update) //Get pressure in mBar 
{
	if(Update) RequestSample(); //Only call for updated value if requested
	uint32_t Val = 0; //Val for getting/calculating pressure value
	uint32_t Temp = 0; //DEBUG

	for(int i = 0; i < 3; i++) {
		Wire.beginTransmission(ADR);
		Wire.write(PRES_REG + i);
		Wire.endTransmission();
		Wire.requestFrom(ADR, 1);
		Temp = Wire.read(); //DEBUG!
		Val = (Temp << 8*i) | Val; //DEBUG!
	}
	DataRequested = false; //Clear flag on data retreval 
  	return (Val / 4096.0);
}

float Haar::GetHumidity(bool Update)  //Return humidity in % (realtive)
{
	if(Update) RequestSample(); //Only call for updated value if requested
	float Val = 0; //Val for getting/calculating RH value
	Val = (uint16_t(GetWord(RH_REG)));
	Val = (100.0*Val)/65535.0;  //Convert to RH
	DataRequested = false; //Clear flag on data retreval 
	return Val;
}

float Haar::GetTemperature(Sensor Device, bool Update)  //Return temp in C
{
	if(Update) RequestSample(); //Only call for updated value if requested
	float Val = 0; //Val for getting/calculating temp value
	if(Device == Pres_Sense) {
		Val = GetWord(TEMP_PRES);
		Val = Val/100.0; //Convert to C
	}

	else if (Device == RH_Sense) {
		Val = GetWord(TEMP_RH);
		Val = ((Val*175.0)/65535.0) - 45; //Convert to C
	}
	DataRequested = false; //Clear flag on data retreval 
	return Val;
}

uint8_t Haar::Sleep(bool State)
{
	//Add sleep function!
}

uint8_t Haar::RequestSample(bool Block)  //FIX! Allow for read of status register in order to not overwrite state bits
{
	DataRequested = true; //Set flag 
	Wire.beginTransmission(0x40);
	Wire.write(0x00);
	Wire.write(0x01); //Trigger conversion
	uint8_t Error = Wire.endTransmission();  //Return I2C status
	if(Block) { //Only block if triggered
		unsigned long Timeout = millis(); //Get timeout value
		while(!NewData() && (millis() - Timeout < TimeoutGlobal)) { //Wait for new data to be returned
			delay(1);
		}
		return Error; 
	}
	else return Error;
}

String Haar::GetHeader()
{
	return "Pressure Atmos [mBar], Humidity [%], Temp Pres [C], Temp RH [C],";
}

String Haar::GetString()
{
	if(DataRequested) {  //If new data is already en-route
		unsigned long Timeout = millis(); //Get timeout value
		while(!NewData() && (millis() - Timeout < TimeoutGlobal)) { //Wait for new data to be returned
			delay(1);
		}
	}
	else(RequestSample(true)); //Else, Block for new conversion
	// delay(100); //DEBUG!
	return String(GetPressure()) + "," + String(GetHumidity()) + "," + String(GetTemperature(Pres_Sense)) + "," + String(GetTemperature(RH_Sense)) + ",";
}

bool Haar::NewData()  //Checks for updated data
{
	unsigned long Timeout = millis(); //Get timeout value
	Wire.beginTransmission(0x40);
	Wire.write(0x00);
	Wire.endTransmission();
	Wire.requestFrom(0x40, 1);
	while(Wire.available() < 1 && (millis() - Timeout < TimeoutGlobal)) { //Wait for value to be returned //FIX! add timeout/remove
		delay(1);
	}
	uint8_t Val = Wire.read();  //DEBUG!
	bool State = false;
	// bool State = ~(Val & 0x01);
	if(Val & 0x01 == 1) State = false;  //FIX! Make cleaner
	else State = true;
	// Serial.println(State); //DEBUG!
	return (State); //Return inverse of bit 0, true when bit has been cleared, false when waiting for new conversion
}

int16_t Haar::GetWord(uint8_t Reg)  //Returns word, read from Reg position
{
	uint16_t Val = 0; //Val to be read from device
	Wire.beginTransmission(ADR);
	Wire.write(Reg);
	Wire.endTransmission();

	Wire.requestFrom(ADR, 1);  //Request word
//	while(Wire.available() < 2); //Wait //FIX! Add timeout
	Val = Wire.read();
//  Serial.println(Val, HEX);

  Wire.beginTransmission(ADR);
  Wire.write(Reg + 1);
  Wire.endTransmission();
  Wire.requestFrom(ADR, 1);  //Request word
	Val = Val | (Wire.read() << 8);  //Concatonate 16 bits 
//  Val = Wire.read() | (Val << 8);  //Concatonate 16 bits //DEBUG!
	return Val; 
}