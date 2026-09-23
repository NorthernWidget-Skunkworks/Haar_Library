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

`getString()` triggers one reading of both chips through the [NW-Device-Specification](https://github.com/NorthernWidget/NW-Device-Specification) handshake (`updateMeasurements()`), waits for the device's reading counter, and prints pressure [hPa = mBar], relative humidity [%], and the LPS35HW and SHT31 temperatures [°C], with `-9999.00` where a reading failed. The getters return the stored reading (`getPressure()`, `getHumidity()`, `getTemperature(RH_Sense)` or `getTemperature(Pres_Sense)`); pass `true` to take a fresh one first. Faults, for sketches that want them: `faulted(chip)`, `anyFault()`, `faultChip()`, `faultKind()`, `printFault(Serial)`, `faultNote()` (one word, e.g. `SHT31Checksum`, for a logger's note column). `begin()` refuses a device that is not Schema 1, not a Haar, or below firmware patch `HAAR_FW_MIN_PATCH`; `beginFailure()` says which. The default address is `0x48` (Schema 1); firmware before Schema 1 answered at `0x42` and is not supported by this version. Requires the [NW_Core](https://github.com/NorthernWidget/NW_Core) library.

**Full API reference:** https://docs.northernwidget.com/Haar_Library/
