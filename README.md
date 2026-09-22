# Haar_Library

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.4572354.svg)](https://doi.org/10.5281/zenodo.4572354)

Library for the rugged temperature, pressure, relative-humidity sensor.

Project [Haar](https://github.com/NorthernWidget-Skunkworks/Project-Haar), named for the cold fog off the north sea, is a pressure, temperature, and relative-humidity sensor that can withstand full submersion. Bobby Schulz designed it after the consistently-near-0C temperatures and 100% relative humidity of Chimborazo Volcano, Ecuador, claimed the lives of many brave but misguided BME-280 units, who then catistrophically bricked their I2C buses and those of their associated data loggers, thus taking down a large fraction of our hydromet network.

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

**Full API reference:** https://docs.northernwidget.com/Haar_Library/
