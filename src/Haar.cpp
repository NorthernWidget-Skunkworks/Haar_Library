#include "Haar.h"

Haar::Haar()
{
}

bool Haar::begin(uint8_t ADR_)
{
	//Page 0 gates: Schema 1, the name "Haar", firmware patch >= HAAR_FW_MIN_PATCH.
	return _dev.begin(ADR_, "Haar", HAAR_FW_MIN_PATCH);
}

float Haar::getPressure(bool update) //Get pressure in mBar
{
	if(update) updateMeasurements(); //Only call for updated value if requested
	return _pressure;
}

float Haar::getHumidity(bool update)  //Return humidity in % (realtive)
{
	if(update) updateMeasurements(); //Only call for updated value if requested
	return _humidity;
}

float Haar::getTemperature(Sensor device, bool update)  //Return temp in C
{
	if(update) updateMeasurements(); //Only call for updated value if requested
	return (device == Pres_Sense) ? _tempPres : _tempRH;
}

bool Haar::sleep(bool state)
{
	return false; //FIX! Firmware does not act on the Control sleep bit yet
}

bool Haar::updateMeasurements(bool block)
{
	dataRequested = false;
	if(!block) {
		dataRequested = _dev.requestReading(0x03); //both chips: bit 0 SHT31, bit 1 LPS35HW
		return dataRequested;
	}
	_pressure = _humidity = _tempRH = _tempPres = NW_ERROR;
	if(!_dev.takeReading(0x03)) return false;
	return readData() && !_dev.anyFault();
}

bool Haar::readData()
{
	//Block 1 and Block 2 are consecutive (0x28-0x35): one read.
	uint8_t d[14];
	_pressure = _humidity = _tempRH = _tempPres = NW_ERROR;
	if(!_dev.readBytes(NW_REG_DATA, d, 14)) return false;
	if(!_dev.faulted(0)) { //SHT31: temperature int16 0.01 C, humidity uint16 0.01 %RH
		_tempRH = float((int16_t)(d[0] | (d[1] << 8))) / 100.0;
		_humidity = float((uint16_t)(d[2] | (d[3] << 8))) / 100.0;
	}
	if(!_dev.faulted(1)) { //LPS35HW: pressure uint32 0.01 hPa, temperature int16 0.01 C
		uint32_t p = (uint32_t)d[8] | ((uint32_t)d[9] << 8) | ((uint32_t)d[10] << 16) | ((uint32_t)d[11] << 24);
		_pressure = float(p) / 100.0;
		_tempPres = float((int16_t)(d[12] | (d[13] << 8))) / 100.0;
	}
	return true;
}

String Haar::getHeader()
{
	return "Pressure Atmos [mBar], Humidity [%], Temp Pres [C], Temp RH [C],";
}

String Haar::getString()
{
	if(dataRequested) {  //If new data is already en-route
		dataRequested = false;
		_pressure = _humidity = _tempRH = _tempPres = NW_ERROR;
		if(_dev.waitReading() && _dev.captureReading()) readData(); //Else NW_ERROR: it never came
	}
	else(updateMeasurements(true)); //Else, block for new conversion
	return String(getPressure()) + "," + String(getHumidity()) + "," \
					+ String(getTemperature(Pres_Sense)) + "," \
					+ String(getTemperature(RH_Sense)) + ",";
}

bool Haar::newData()  // Checks for updated data
{
	if(dataRequested) { //A non-blocking request is out: capture it when the counter has moved
		if(!_dev.newReading()) return false;
		_dev.captureReading();
		readData();
		dataRequested = false;
		return true;
	}
	return _dev.ready();
}

bool    Haar::faulted(uint8_t chip) { return _dev.faulted(chip); }
bool    Haar::anyFault()            { return _dev.anyFault(); }
uint8_t Haar::faultChip()           { return _dev.faultChip(); }
uint8_t Haar::faultKind()           { return _dev.faultKind(); }
String  Haar::beginFailure()        { return _dev.beginFailure(); }
uint8_t Haar::getHardwareMajor()    { return _dev.hardwareMajor(); }
uint8_t Haar::getHardwareMinor()    { return _dev.hardwareMinor(); }
uint8_t Haar::getFirmwareVersion()  { return _dev.firmwareVersion(); }

size_t Haar::printFault(Print& out)
{
	//The chip names are Haar's own; the kind names are universal (NW_Fault).
	static const char* const chips[] = {"SHT31", "LPS35HW"};
	uint8_t chip = faultChip(), kind = faultKind();
	if(kind == 0) return out.print("none");
	size_t n = 0;
	if(chip == 7) n += out.print("unit");
	else if(chip < 2) n += out.print(chips[chip]);
	else { n += out.print("chip "); n += out.print(chip); }
	n += out.print(": ");
	return n + _dev.fault().printKind(out);
}

String Haar::faultNote()
{
	//One word for a data-table note: the chip, then the kind ("SHT31Checksum").
	static const char* const chips[] = {"SHT31", "LPS35HW"};
	uint8_t chip = faultChip();
	String w;
	if(chip == 7) w = F("Unit");
	else if(chip < 2) w = chips[chip];
	else { w = F("Chip"); w += String(chip); }
	w += _dev.fault().kindWord();
	return w;
}
