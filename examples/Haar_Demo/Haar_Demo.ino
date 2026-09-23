#include <Haar.h>

Haar sensor;

void setup() {
    Serial.begin(9600);
    Serial.println("Haar temperature, pressure, and humidity sensor");
    if (!sensor.begin()) {
        Serial.print("Haar not found: ");
        Serial.println(sensor.beginFailure());  // NotAnswering, NotSchema1, WrongName, OldFirmware
        while (1);
    }
    Serial.println(sensor.getHeader());
}

void loop() {
    Serial.println(sensor.getString());  // -9999.00 where a reading failed
    if (sensor.anyFault()) {
        sensor.printReport(Serial);  // e.g. "SHT31: checksum failed"
        Serial.println();
    }
    delay(1000);
}
