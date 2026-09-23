# Haar_Library

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.4572354.svg)](https://doi.org/10.5281/zenodo.4572354)

Library for the rugged temperature, pressure, relative-humidity sensor.

Project [Haar](https://github.com/NorthernWidget/Project-Haar), named for the cold fog off the north sea, is a pressure, temperature, and relative-humidity sensor that can withstand full submersion. Bobby Schulz designed it after the consistently-near-0C temperatures and 100% relative humidity of Chimborazo Volcano, Ecuador, claimed the lives of many brave but misguided BME-280 units, who then catistrophically bricked their I2C buses and those of their associated data loggers, thus taking down a large fraction of our hydromet network.

*May their silicon and copper souls join the chorus of the stars.*

**Installation:** included in [NorthernWidget-libraries](https://github.com/NorthernWidget/NorthernWidget-libraries).

```cpp
#include <Haar.h>

Haar sensor;

void setup() {
    Serial.begin(9600);
    if (!sensor.begin()) {
        Serial.println("Haar not found. Check wiring.");
        while (1);
    }
    Serial.println(sensor.getHeader());
}

void loop() {
    Serial.println(sensor.getString());
    delay(1000);
}
```

See [examples/](examples/) for this demo and a serial-output variant.

`getString()` takes one reading of both chips through the [NW-Device-Specification](https://github.com/NorthernWidget/NW-Device-Specification) handshake (`updateMeasurements()`): it triggers the device, waits for its reading counter, and prints pressure [hPa = mBar], relative humidity [%], and the LPS35HW and SHT31 temperatures [°C], with `-9999.00` where a reading failed. The getters return the stored reading: `getPressure()`, `getHumidity()`, and `getTemperature(RH_Sense)` or `getTemperature(Pres_Sense)`. Pass `true` to take a fresh one first.

One `getString()` can take several readings of each chip. `setHumidityReadings(n)` sets how many the SHT31 takes, for humidity and its temperature. `setPressureReadings(n)` does the same for the LPS35HW, for pressure and its temperature. Each chip stores up to `HAAR_HUMIDITY_CAPACITY` or `HAAR_PRESSURE_CAPACITY` readings: 16 by default, and you can define a larger one before the include. The values printed are then the means. For the spread, `getHumidityMean()`, `getHumidityStd()`, `getHumiditySterr()` and `getHumidityMedian()` read the stored readings. The same four exist for pressure and for `getTemperature…(RH_Sense)` or `(Pres_Sense)`, and `getHumidityCount()` and `getPressureCount()` say how many readings each chip holds. `setHumidityStats(true)` or `setPressureStats(true)` adds std and sterr columns to `getHeader()` and `getString()`. To read one chip alone, call `updateMeasurements(Haar::SHT31)` or `updateMeasurements(Haar::LPS35HW)`. `updateMeasurements(false)` only requests a reading, which `newData()` captures later. To log one row per reading to a file, call `beginReadings(component, n)`, `printHeader(out)`, then `logReading(out)` n times and `endReadings()`. Here `out` is any `Print`, such as an SdFat `File` or `Serial`.

Faults are there if you want them. `faulted(chip)` takes 0 for the SHT31 and 1 for the LPS35HW. `anyFault()`, `reportChip()` and `reportKind()` summarise the last reading. `printReport(Serial)` prints the report as text, and `reportNote()` gives it as one word, such as `SHT31ChecksumFailed`, for a logger's note column. `begin()` refuses a device that is not Schema 1, is not a Haar, or runs firmware older than patch `HAAR_FW_MIN_PATCH`. `beginFailure()` tells you which. The default address is `0x48` (Schema 1). Firmware before Schema 1 answered at `0x42` and will not pass `begin()`. `printStatus(out)` prints one status line for a logger's status file (name, serial, versions, the last report as code and note, Pages 0 to 2 in hex), for the logger to write with its timestamp whenever `reportKind()` is not zero. Pages renumbered 2026-09-23 (spec 4c3b18d): calibration is Page 1 at 0x20, data Page 2 at 0x40. This version needs the [NW_Core](https://github.com/NorthernWidget/NW_Core) library.

**Full API reference:** https://docs.northernwidget.com/Haar_Library/
