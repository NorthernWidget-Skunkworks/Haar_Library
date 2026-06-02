#include <Haar.h>

Haar sensor;

void setup() {
    Serial.begin(9600);
    Serial.println("Haar temperature, pressure, and humidity sensor");
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
