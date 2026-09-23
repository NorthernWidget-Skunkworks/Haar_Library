
#ifndef HAAR_h
#define HAAR_h

#include "Arduino.h"
#include <Wire.h>
#include <NW_Core.h>   // NW_Core: NW_Device (Schema 1 protocol), NW_Report

/// Lowest firmware patch (Page 0 byte 0x0A) this library accepts: patch 1
/// brought Schema 1 (Page 0, Block 0 handshake, data at 0x48 in 0.01 units).
#define HAAR_FW_MIN_PATCH 1

// Build identity: this library's version (held equal to library.properties by
// NW-Tests/version_check.py) and its build commit, set by the NW-Build wrapper from
// git and blank in an Arduino IDE build. Both go into a logger's status file.
#define HAAR_LIBRARY_VERSION "1.0.0"
#ifndef HAAR_LIBRARY_COMMIT
#define HAAR_LIBRARY_COMMIT ""
#endif

// Readings per updateMeasurements() are kept in static arrays of this
// capacity (one per chip group; no heap); set<Field>Readings(n) clamps to it.
// Override before the include to trade RAM for a longer batch.
#ifndef HAAR_HUMIDITY_CAPACITY
  #define HAAR_HUMIDITY_CAPACITY 16   // SHT31: humidity and its temperature
#endif
#ifndef HAAR_PRESSURE_CAPACITY
  #define HAAR_PRESSURE_CAPACITY 16   // LPS35HW: pressure and its temperature
#endif

enum Sensor {
	RH_Sense = 0,
	Pres_Sense = 1
};

#define CTRL1 0x41     // Schema 1 Page 2 Block 0: Control (trigger, chip select, sleep)
#define TEMP_RH 0x48   // Schema 1 Page 2 Block 1: SHT31 temperature, int16, 0.01 °C
#define RH_REG 0x4A    // Schema 1 Page 2 Block 1: humidity, uint16, 0.01 %RH
#define PRES_REG 0x50  // Schema 1 Page 2 Block 2: LPS35HW pressure, uint32, 0.01 hPa
#define TEMP_PRES 0x54 // Schema 1 Page 2 Block 2: LPS35HW temperature, int16, 0.01 °C

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
class Haar : public NW_Sensor
{
	public:
	  /** @brief Default I2C address: NW-Device-Specification Schema 1 'H' (0x48; was 0x42). */
		static constexpr uint8_t DEFAULT_ADDRESS = 0x48;
	  /** @brief Chip groups a reading can cover (the spec's chip table: 0 SHT31, 1 LPS35HW). */
		enum Component : uint8_t {
			SHT31   = 0x01,  ///< humidity and the SHT31's own temperature (RH_Sense)
			LPS35HW = 0x02,  ///< pressure and the LPS35HW's own temperature (Pres_Sense)
			ALL     = 0x03
		};
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
	   * @brief Take the configured number of readings of the selected chips
	   * and store them for the getters and the statistics.
	   * @details Each reading triggers the device and waits for its reading
	   * counter to advance (NW-Device-Specification handshake); N > 1 is
	   * declared to the device as a batch first. The single-value getters
	   * return the mean of the readings taken; a chip the device reports
	   * faulted leaves its values at NW_ERROR (-9999). With one reading of
	   * ALL, both chips are read in a single transaction.
	   * @param component Haar::ALL (default), Haar::SHT31 or Haar::LPS35HW.
	   * @return true if every selected chip gave at least one valid reading
	   */
		bool updateMeasurements(uint8_t component = ALL);
	  /**
	   * @brief Take a new sample of all of the data, blocking or not.
	   * @param Block: `true` = updateMeasurements(ALL). `false` = only request
	   * one reading of both chips; newData() captures it once it is there.
	   * @return true if the reading (or the request) succeeded.
	   */
		bool updateMeasurements(bool block);
	  /** @brief Take ONE reading of the SHT31 (humidity and its temperature) and append it to the readings. */
		bool updateHumidity();
	  /** @brief Take ONE reading of the LPS35HW (pressure and its temperature) and append it to the readings. */
		bool updatePressure();
	  /**
	   * @brief Set how many SHT31 readings updateMeasurements() takes
	   * (statistics are computed over them). Clamped to HAAR_HUMIDITY_CAPACITY.
	   * @return The number actually set.
	   */
		uint16_t setHumidityReadings(uint16_t n);
	  /** @brief Set how many LPS35HW readings updateMeasurements() takes. Clamped to HAAR_PRESSURE_CAPACITY. */
		uint16_t setPressureReadings(uint16_t n);
	  /** @brief Enable or disable humidity and SHT31-temperature std and sterr columns in getString()/getHeader(). */
		void setHumidityStats(bool enable);
	  /** @brief Enable or disable pressure and LPS35HW-temperature std and sterr columns in getString()/getHeader(). */
		void setPressureStats(bool enable);
	  /** @brief Number of valid SHT31 readings stored by the last updateMeasurements(). */
		uint16_t getHumidityCount();
	  /** @brief Number of valid LPS35HW readings stored by the last updateMeasurements(). */
		uint16_t getPressureCount();

		// --- Statistics getters ---
		// Computed two-pass in 32-bit float over the readings stored by the last
		// updateMeasurements() (NW_Readings). Adequate for N up to the array
		// capacities; at N in the thousands the sum of squared deviations would
		// want double precision, which the AVR lacks.
		/** @brief Pressure mean [mBar] over the stored readings (NW_ERROR when none). */
		float getPressureMean();
		/** @brief Pressure standard deviation [mBar]. */
		float getPressureStd();
		/** @brief Pressure standard error [mBar]. */
		float getPressureSterr();
		/** @brief Pressure median [mBar] (mean of the middle pair for even N). */
		float getPressureMedian();
		/** @brief Humidity mean [%] over the stored readings. */
		float getHumidityMean();
		/** @brief Humidity standard deviation [%]. */
		float getHumidityStd();
		/** @brief Humidity standard error [%]. */
		float getHumiditySterr();
		/** @brief Humidity median [%]. */
		float getHumidityMedian();
		/** @brief Temperature mean [C] of the given sensor over the stored readings. */
		float getTemperatureMean(Sensor Device = RH_Sense);
		/** @brief Temperature standard deviation [C] of the given sensor. */
		float getTemperatureStd(Sensor Device = RH_Sense);
		/** @brief Temperature standard error [C] of the given sensor. */
		float getTemperatureSterr(Sensor Device = RH_Sense);
		/** @brief Temperature median [C] of the given sensor. */
		float getTemperatureMedian(Sensor Device = RH_Sense);

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
	   * with std and sterr columns after a value when its chip group's
	   * statistics are enabled and more than one reading is configured.
	   */
		String getHeader();

		// --- Reading interface (NW standard) ---
		/**
		 * @brief Print the header matching printReading(): column names with
		 * units, each followed by a comma, for the chips selected by
		 * beginReadings(). No statistics columns: one reading has none.
		 * @param out Any Print destination (SdFat File, Serial, ...).
		 * @return Bytes written.
		 */
		size_t printHeader(Print& out);
		/**
		 * @brief Print the stored reading of the selected chips, each value
		 * followed by a comma, in getString()'s order: pressure [mBar],
		 * humidity [%], LPS35HW temperature [C], SHT31 temperature [C]. Does
		 * not acquire: call updateMeasurements() first, or use logReading().
		 * @return Bytes written.
		 */
		size_t printReading(Print& out);
		/**
		 * @brief Take ONE reading of the selected chips and print it: the
		 * one-reading primitive for collecting many readings to a file.
		 * @return Bytes written.
		 */
		size_t logReading(Print& out);
		/**
		 * @brief Begin a run of readings, selecting which chips they cover.
		 * @param component Haar::ALL, Haar::SHT31 or Haar::LPS35HW.
		 * @param n How many readings the run will take (the number of
		 * logReading() calls to follow); with n > 1 the device is told in
		 * advance (readings-requested word). Nothing on Haar is powered per
		 * batch, so the word only satisfies the protocol.
		 */
		void beginReadings(uint8_t component = ALL, uint16_t n = 0);
		/** @brief End a run of readings. */
		void endReadings();

		// --- Faults (status byte, live; Report register, latched) ---
		/** @brief True if the given chip (0 = SHT31, 1 = LPS35HW) was faulted in the last reading. */
		bool faulted(uint8_t chip);
		/** @brief True if any chip was faulted in the last reading (status pan-fault bit). */
		bool anyFault();
		/** @brief Chip index of the report (0 SHT31, 1 LPS35HW, 7 the unit); meaningful when reportKind() != 0. */
		uint8_t reportChip();
		/** @brief Kind of the report, per the spec's table (1 not answering, 2 timed out, 3 checksum failed, 6 restarted since configured, ...). */
		uint8_t reportKind();
		/** @brief Print the report as text, e.g. "SHT31: checksum failed"; "none" when there is no fault. */
		size_t printReport(Print& out);
		/** @brief The report as one word for a note column: "SHT31ChecksumFailed", "LPS35HWTimeout", "UnitRestarted"; "UnitNone" when none. */
		String reportNote();
		/** @brief Print one status line for a logger's status file: name, serial, versions, the last report, Pages 0-2 in hex; no newline, not answering. */
		size_t printStatus(Print& out, bool boot = false) override;
		// --- NW_Sensor: the logger's view (Margay::watch) ---
		const char* name() const override { return "Haar"; }
		bool reportIsFault() override;
		uint8_t bootReportKind() override;
		void clearBootReport() override;
		/** @brief Why the last begin() refused, as one word: "NotAnswering", "NotSchema1", "WrongName", "OldFirmware"; "None" after success. */
		String beginFailure();
		uint8_t getHardwareMajor();
		uint8_t getHardwareMinor();
		uint8_t getFirmwareVersion();

	private:
		NW_Device _dev;
		float _pressure = NW_ERROR; //Mean of the last updateMeasurements() [hPa = mBar]
		float _humidity = NW_ERROR; //[%RH]
		float _tempRH = NW_ERROR; //SHT31 [C]
		float _tempPres = NW_ERROR; //LPS35HW [C]
		// Readings as the device serves them (raw register units), one array per
		// field; statistics come from these and are scaled on the way out.
		NW_Readings<int16_t, HAAR_HUMIDITY_CAPACITY> _tempRHReadings;   //0.01 C
		NW_Readings<int16_t, HAAR_HUMIDITY_CAPACITY> _humidityReadings; //0.01 %RH
		NW_Readings<int32_t, HAAR_PRESSURE_CAPACITY> _pressureReadings; //0.01 hPa
		NW_Readings<int16_t, HAAR_PRESSURE_CAPACITY> _tempPresReadings; //0.01 C
		NW_ReadingsConfig _humidityCfg; //Readings per updateMeasurements() and stats columns, SHT31 group
		NW_ReadingsConfig _pressureCfg; //LPS35HW group
		uint8_t _component = ALL; //Selection of the current beginReadings() run
		bool readSHT31(uint8_t* d);   //Append one served SHT31 reading (4 bytes from 0x48) unless faulted
		bool readLPS35HW(uint8_t* d); //Append one served LPS35HW reading (6 bytes from 0x50) unless faulted
		bool readData();              //One 14-byte read of both chips, appended
		void summarise(uint8_t component); //Means into the single-value fields, NW_ERROR when no reading
		bool dataRequested = false; //Flag for keeping track of data requests and
		                            // data retrevals
};

#endif
