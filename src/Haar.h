
#ifndef HAAR_h
#define HAAR_h

#include "Arduino.h"
#include <Wire.h>
#include <NW_Core.h>   // NW_Core: NW_Device (Schema 1 protocol), NW_Fault

/// Lowest firmware patch (Page 0 byte 0x0A) this library accepts: patch 1
/// brought Schema 1 (Page 0, Block 0 handshake, data at 0x28 in 0.01 units).
#define HAAR_FW_MIN_PATCH 1

enum Sensor {
	RH_Sense = 0,
	Pres_Sense = 1
};

#define CTRL1 0x21     // Schema 1 Page 1 Block 0: Control (trigger, chip select, sleep)
#define TEMP_RH 0x28   // Schema 1 Page 1 Block 1: SHT31 temperature, int16, 0.01 °C
#define RH_REG 0x2A    // Schema 1 Page 1 Block 1: humidity, uint16, 0.01 %RH
#define PRES_REG 0x30  // Schema 1 Page 1 Block 2: LPS35HW pressure, uint32, 0.01 hPa
#define TEMP_PRES 0x34 // Schema 1 Page 1 Block 2: LPS35HW temperature, int16, 0.01 °C

#define ON 1
#define OFF 0

/**
 * @class Haar
 * @brief Library for the rugged temperature, pressure, relative-humidity sensor
 * @details Project Haar, named for the cold fog off the north sea, is a
 * pressure, temperature, and relative-humidity sensor that can withstand
 * full submersion. Bobby Schulz designed it after the consistently-near-0C
 * temperatures and 100% relative humidity of Chimborazo Volcano, Ecuador,
 * claimed the lives of many brave but misguided BME-280 units, who then
 * catistrophically bricked their I2C buses and those of their associated
 * data loggers, thus taking down a large fraction of our hydromet network.
 *
 * *May their silicon and copper souls join the chorus of the stars.*
 *
 * \verbatim [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.4572354.svg)](https://doi.org/10.5281/zenodo.4572354) \endverbatim
 */
class Haar
{
	public:
	  /** @brief Default I2C address: NW-Device-Specification Schema 1 'H' (0x48; was 0x42). */
		static constexpr uint8_t DEFAULT_ADDRESS = 0x48;
	  /**
	   * @brief Instantiate the Haar sensor class
	   */
		Haar();

	  /**
	   * @brief Begin communications with the Haar sensor.
	   * @details Refuses the device unless Page 0 says Schema 1, the name
	   * "Haar", and a firmware patch of at least HAAR_FW_MIN_PATCH;
	   * beginFailure() says which gate refused. Takes no reading.
	   * @param[in] ADR_: I2C address. Defaults to DEFAULT_ADDRESS.
	   * @return True if the device answered and passed the three gates.
	   */
		bool begin(uint8_t ADR_ = DEFAULT_ADDRESS);

	  /**
	   * @brief Return the currently stored pressure [in mBar]
	   * @param[in] update: Read and store a new pressure value before sending?
	   * By default is false; simply returns the already-available pressure value.
	   */
		float getPressure(bool update = false);

	  /**
	   * @brief Return the currently stored relative humidity [%]
	   * @param[in] update: Read and store a new pressure value before sending?
	   * By default is false; simply returns the already-available RH value.
	   */
		float getHumidity(bool update = false);

	  /**
	   * @brief Return the currently stored Temperature [degrees C]
	   * @param[in] Device: Selects which sensor is used to measure the
	   * temperature. Input can be `RH_Sense`, which corresponds to a 0, or
	   * `Pres_Sense`, which corresponds to a 1. By default, this is RH_Sense,
	   * as this sensor has a better internal temperature sensor.
	   * @param[in] update: Read and store a new pressure value before sending?
	   * By default is false; simply returns the already-available temperature
     * value.
	   */
		float getTemperature(Sensor Device = RH_Sense, bool update = false);

	  /**
	   * @brief Enable or disable device sleep mode.
	   * THIS FUNCTION CURRENTLY DOES NOTHING and returns false: the firmware
	   * accepts the Control sleep bit but does not act on it yet.
	   * @param State: Can be `ON` (1) or `OFF` (0). By default, this command sets
	   * "Sleep" to ON, sending the device into a low-power sleep mode.
	   * @return False (not yet implemented).
	   */
		bool sleep(bool state = ON);

	  /**
	   * @brief Take a new sample of all of the data.
	   * @details Triggers both chips through the NW-Device-Specification
	   * handshake. Blocking: waits for the device's reading counter, then
	   * reads all four values in one transaction and stores them. Non-blocking:
	   * only requests the reading; newData() captures it once it is there.
	   * @param Block: if `true` (the default), wait for the reading.
	   * @return true if the device answered and no chip faulted (blocking), or
	   * if the request was written (non-blocking).
	   */
		bool updateMeasurements(bool block = true);

	  /**
	   * @brief Checks for updated data. Returns `true` if new data are available;
	   * otherwise returns `false`.
	   * @details After a non-blocking updateMeasurements(false), returns true once
	   * the device's reading counter has advanced and stores the values as it
	   * does so; otherwise reports the device's ready bit.
	   */
		bool newData();

	  /**
	   * @brief The most important function for the user! Returns all data as a
	   * comma-separated string: "P,RH,T(P),T[RH],".
	   * @details This string is: PRESSURE,RELATIVE_HUMIDITY,
	   * TEMPERATURE_FROM_PRESSURE_SENSOR,TEMPERATURE_FROM_RH_SENSOR,
	   */
		String getString();

	  /**
	   * @brief Returns a header:
     * "Pressure Atmos [mBar], Humidity [%], Temp Pres [C], Temp RH [C],"
	   */
		String getHeader();

		// --- Faults (status byte, live; fault byte, latched) ---
		/** @brief True if the given chip (0 = SHT31, 1 = LPS35HW) was faulted in the last reading. */
		bool faulted(uint8_t chip);
		/** @brief True if any chip was faulted in the last reading (status pan-fault bit). */
		bool anyFault();
		/** @brief Chip index of the latched fault (0 SHT31, 1 LPS35HW, 7 the unit); meaningful when faultKind() != 0. */
		uint8_t faultChip();
		/** @brief Kind of the latched fault, per the spec's table (1 no acknowledge, 2 timeout, 3 checksum, 6 reset since configured, ...). */
		uint8_t faultKind();
		/** @brief Print the latched fault as text, e.g. "SHT31: checksum"; "none" when there is no fault. */
		size_t printFault(Print& out);
		/** @brief The latched fault as one word for a note column: "SHT31Checksum", "LPS35HWTimeout", "UnitReset"; "UnitNone" when none. */
		String faultNote();
		/** @brief Why the last begin() refused, as one word: "NoACK", "NotSchema1", "WrongName", "OldFirmware"; "None" after success. */
		String beginFailure();
		uint8_t getHardwareMajor();
		uint8_t getHardwareMinor();
		uint8_t getFirmwareVersion();

	private:
		NW_Device _dev;
		bool readData(); //Read 0x28-0x35 into the stored values, NW_ERROR for a faulted chip
		float _pressure = NW_ERROR; //Stored by updateMeasurements() [hPa = mBar]
		float _humidity = NW_ERROR; //[%RH]
		float _tempRH = NW_ERROR; //SHT31 [C]
		float _tempPres = NW_ERROR; //LPS35HW [C]
		bool dataRequested = false; //Flag for keeping track of data requests and
		                            // data retrevals
};

#endif
