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
		dataRequested = _dev.requestReading(ALL); //both chips: bit 0 SHT31, bit 1 LPS35HW
		return dataRequested;
	}
	return updateMeasurements((uint8_t)ALL);
}

bool Haar::updateMeasurements(uint8_t component)
{
	bool doSHT = component & SHT31, doLPS = component & LPS35HW;
	if(doSHT) { _tempRHReadings.reset(); _humidityReadings.reset(); }
	if(doLPS) { _pressureReadings.reset(); _tempPresReadings.reset(); }
	if(doSHT && doLPS && _humidityCfg.n <= 1 && _pressureCfg.n <= 1) {
		//One reading of everything: both chips in one trigger, one 14-byte read.
		_dev.resetBatch();
		if(_dev.takeReading(ALL)) readData();
	}
	else {
		//Per chip group: N readings each, appended to the arrays; a chip that
		//reports absent (no acknowledge / not initialised) stops its batch.
		if(doSHT) _dev.takeReadings(SHT31, _humidityCfg.n, [this] { return updateHumidity(); });
		if(doLPS) _dev.takeReadings(LPS35HW, _pressureCfg.n, [this] { return updatePressure(); });
	}
	summarise(component);
	bool ok = true;
	if(doSHT) ok = ok && _humidityReadings.count() > 0;
	if(doLPS) ok = ok && _pressureReadings.count() > 0;
	return ok;
}

bool Haar::updateHumidity()
{
	uint8_t d[4];
	if(!_dev.takeReading(SHT31) || !_dev.readData(TEMP_RH, d, 4)) return false;
	return readSHT31(d);
}

bool Haar::updatePressure()
{
	uint8_t d[6];
	if(!_dev.takeReading(LPS35HW) || !_dev.readData(PRES_REG, d, 6)) return false;
	return readLPS35HW(d);
}

bool Haar::readSHT31(uint8_t* d)
{
	if(_dev.faulted(0)) return false; //SHT31: temperature int16 0.01 C, humidity uint16 0.01 %RH
	_tempRHReadings.append((int16_t)(d[0] | (d[1] << 8)));
	_humidityReadings.append((int16_t)(d[2] | (d[3] << 8)));
	return true;
}

bool Haar::readLPS35HW(uint8_t* d)
{
	if(_dev.faulted(1)) return false; //LPS35HW: pressure uint32 0.01 hPa, temperature int16 0.01 C
	_pressureReadings.append((int32_t)((uint32_t)d[0] | ((uint32_t)d[1] << 8) | ((uint32_t)d[2] << 16) | ((uint32_t)d[3] << 24)));
	_tempPresReadings.append((int16_t)(d[4] | (d[5] << 8)));
	return true;
}

bool Haar::readData()
{
	//Block 1 and Block 2 are consecutive (0x28-0x35): one read.
	uint8_t d[14];
	if(!_dev.readData(NW_REG_DATA, d, 14)) return false;
	readSHT31(d);
	readLPS35HW(d + 8);
	return true;
}

void Haar::summarise(uint8_t component)
{
	//Means over the readings taken, scaled from the register units; NW_ERROR when none.
	if(component & SHT31) {
		_tempRH   = nwScaled(_tempRHReadings.mean(),   100.0);
		_humidity = nwScaled(_humidityReadings.mean(), 100.0);
	}
	if(component & LPS35HW) {
		_pressure = nwScaled(_pressureReadings.mean(), 100.0);
		_tempPres = nwScaled(_tempPresReadings.mean(), 100.0);
	}
}

uint16_t Haar::setHumidityReadings(uint16_t n) { return _humidityCfg.set(n, HAAR_HUMIDITY_CAPACITY); }
uint16_t Haar::setPressureReadings(uint16_t n) { return _pressureCfg.set(n, HAAR_PRESSURE_CAPACITY); }
void     Haar::setHumidityStats(bool enable)   { _humidityCfg.stats = enable; }
void     Haar::setPressureStats(bool enable)   { _pressureCfg.stats = enable; }
uint16_t Haar::getHumidityCount()            { return _humidityReadings.count(); }
uint16_t Haar::getPressureCount()            { return _pressureReadings.count(); }

//Statistics are computed from the arrays each call (NW_Readings), in the
//register units (0.01 hPa, 0.01 %RH, 0.01 C), then scaled. NW_ERROR when empty.
static float scaled(float v) { return nwScaled(v, 100.0); }
float Haar::getPressureMean()   { return scaled(_pressureReadings.mean()); }
float Haar::getPressureStd()    { return scaled(_pressureReadings.std()); }
float Haar::getPressureSterr()  { return scaled(_pressureReadings.sterr()); }
float Haar::getPressureMedian() { return scaled(_pressureReadings.median()); }
float Haar::getHumidityMean()   { return scaled(_humidityReadings.mean()); }
float Haar::getHumidityStd()    { return scaled(_humidityReadings.std()); }
float Haar::getHumiditySterr()  { return scaled(_humidityReadings.sterr()); }
float Haar::getHumidityMedian() { return scaled(_humidityReadings.median()); }
float Haar::getTemperatureMean(Sensor device)   { return scaled(device == Pres_Sense ? _tempPresReadings.mean()   : _tempRHReadings.mean()); }
float Haar::getTemperatureStd(Sensor device)    { return scaled(device == Pres_Sense ? _tempPresReadings.std()    : _tempRHReadings.std()); }
float Haar::getTemperatureSterr(Sensor device)  { return scaled(device == Pres_Sense ? _tempPresReadings.sterr()  : _tempRHReadings.sterr()); }
float Haar::getTemperatureMedian(Sensor device) { return scaled(device == Pres_Sense ? _tempPresReadings.median() : _tempRHReadings.median()); }

String Haar::getHeader()
{
	bool sh = _humidityCfg.columns(), sp = _pressureCfg.columns();
	String h = "Pressure Atmos [mBar], ";
	if(sp) h += "Pressure Atmos std [mBar], Pressure Atmos sterr [mBar], ";
	h += "Humidity [%], ";
	if(sh) h += "Humidity std [%], Humidity sterr [%], ";
	h += "Temp Pres [C], ";
	if(sp) h += "Temp Pres std [C], Temp Pres sterr [C], ";
	h += "Temp RH [C],";
	if(sh) h += " Temp RH std [C], Temp RH sterr [C],";
	return h;
}

//The reading interface: one reading per logReading(), printed as it is taken.
void Haar::beginReadings(uint8_t component, uint16_t n)
{
	_component = component;
	if(component & SHT31) { _tempRHReadings.reset(); _humidityReadings.reset(); }
	if(component & LPS35HW) { _pressureReadings.reset(); _tempPresReadings.reset(); }
	_dev.beginBatch(n);
}

void Haar::endReadings()
{
	//No cleanup required currently
}

size_t Haar::printHeader(Print& out)
{
	size_t n = 0;
	if(_component & LPS35HW) n += out.print("Pressure Atmos [mBar],");
	if(_component & SHT31) n += out.print("Humidity [%],");
	if(_component & LPS35HW) n += out.print("Temp Pres [C],");
	if(_component & SHT31) n += out.print("Temp RH [C],");
	return n;
}

size_t Haar::printReading(Print& out)
{
	size_t n = 0;
	if(_component & LPS35HW) { n += out.print(_pressure); n += out.print(','); }
	if(_component & SHT31) { n += out.print(_humidity); n += out.print(','); }
	if(_component & LPS35HW) { n += out.print(_tempPres); n += out.print(','); }
	if(_component & SHT31) { n += out.print(_tempRH); n += out.print(','); }
	return n;
}

size_t Haar::logReading(Print& out)
{
	//One acquisition per chip group selected, then the values just taken.
	if(_component & SHT31) {
		_humidity = _tempRH = NW_ERROR;
		if(updateHumidity()) { _humidity = _humidityReadings.last() / 100.0; _tempRH = _tempRHReadings.last() / 100.0; }
	}
	if(_component & LPS35HW) {
		_pressure = _tempPres = NW_ERROR;
		if(updatePressure()) { _pressure = _pressureReadings.last() / 100.0; _tempPres = _tempPresReadings.last() / 100.0; }
	}
	return printReading(out);
}

String Haar::getString()
{
	if(dataRequested) {  //If new data is already en-route
		dataRequested = false;
		_pressure = _humidity = _tempRH = _tempPres = NW_ERROR;
		_tempRHReadings.reset(); _humidityReadings.reset(); _pressureReadings.reset(); _tempPresReadings.reset();
		if(_dev.waitReading() && _dev.captureReading()) readData(); //Else NW_ERROR: it never came
		summarise(ALL);
	}
	else(updateMeasurements(true)); //Else, block for new conversion
	bool sh = _humidityCfg.columns(), sp = _pressureCfg.columns();
	String s = String(getPressure()) + ",";
	if(sp) s += String(getPressureStd()) + "," + String(getPressureSterr()) + ",";
	s += String(getHumidity()) + ",";
	if(sh) s += String(getHumidityStd()) + "," + String(getHumiditySterr()) + ",";
	s += String(getTemperature(Pres_Sense)) + ",";
	if(sp) s += String(getTemperatureStd(Pres_Sense)) + "," + String(getTemperatureSterr(Pres_Sense)) + ",";
	s += String(getTemperature(RH_Sense)) + ",";
	if(sh) s += String(getTemperatureStd(RH_Sense)) + "," + String(getTemperatureSterr(RH_Sense)) + ",";
	return s;
}

bool Haar::newData()  // Checks for updated data
{
	if(dataRequested) { //A non-blocking request is out: capture it when the counter has moved
		if(!_dev.newReading()) return false;
		_dev.captureReading();
		_tempRHReadings.reset(); _humidityReadings.reset(); _pressureReadings.reset(); _tempPresReadings.reset();
		readData();
		summarise(ALL);
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
	//The chip names are Haar's own (the spec's chip table); NW_Fault prints the rest.
	static const char* const chips[] = {"SHT31", "LPS35HW"};
	return _dev.fault().print(out, chips, 2);
}

String Haar::faultNote()
{
	//One word for a data-table note: the chip, then the kind ("SHT31Checksum").
	static const char* const chips[] = {"SHT31", "LPS35HW"};
	return _dev.fault().note(chips, 2);
}
