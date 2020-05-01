# Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`class `[`Haar`](#classHaar) | Library for the rugged temperature, pressure, relative-humidity sensor.

# class `Haar` 

Library for the rugged temperature, pressure, relative-humidity sensor.

Project [Haar](#classHaar), named for the cold fog off the north sea, is a pressure, temperature, and relative-humidity sensor that can withstand full submersion. Bobby Schulz designed it after the consistently-near-0C temperatures and 100% relative humidity of Chimborazo Volcano, Ecuador, claimed the lives of many brave but misguided BME-280 units, who then catistrophically bricked their I2C buses and those of their associated data loggers, thus taking down a large fraction of our hydromet network.

*May their sad silicon and copper souls join the chorus of the stars.*

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  `[`Haar`](#classHaar_1a0da6a368ea9f434647065dbc356350cf)`()` | Instantiate the [Haar](#classHaar) sensor class.
`public uint8_t `[`begin`](#classHaar_1a80119761d396fae3f7f36dc5ef0f5d63)`(uint8_t ADR_)` | Begin communications with the [Haar](#classHaar) sensor.
`public float `[`GetPressure`](#classHaar_1a0f01c6a94097c9da103157d605a2640e)`(bool Update)` | Return the currently stored pressure [in mBar].
`public float `[`GetHumidity`](#classHaar_1ae70cc679d30a06bdd911e52db6346618)`(bool Update)` | Return the currently stored relative humidity [%].
`public float `[`GetTemperature`](#classHaar_1a5e4c7cff226975a2ddade7cb63be3ba3)`(Sensor Device,bool Update)` | Return the currently stored relative humidity [%].
`public uint8_t `[`Sleep`](#classHaar_1ac4646f6c4b5532b1d4304a761eb77ad8)`(bool State)` | Enable or disable device sleep mode THIS FUNCTION CURRENTLY DOES NOTHING!
`public uint8_t `[`RequestSample`](#classHaar_1aff15e81cba7f13897ae38c8dfe21d73e)`(bool Block)` | Return a new sample of all of the data.
`public bool `[`NewData`](#classHaar_1a1a99e2cb4d3a53a170bb9a96fac73440)`()` | Checks for updated data. Returns `true` if new data are available; otherwise returns `false`
`public String `[`GetString`](#classHaar_1a58fc9627f957e39e8481837e8616d52a)`()` | The most important function for the user! Returns all data as a comma-separated string: "P,RH,T(P),T[RH],".
`public String `[`GetHeader`](#classHaar_1a7069008fd8febc964d330a9313d5d79f)`()` | Returns a header: "Pressure Atmos [mBar], Humidity [%], Temp Pres [C], Temp RH [C],".

## Members

#### `public  `[`Haar`](#classHaar_1a0da6a368ea9f434647065dbc356350cf)`()` 

Instantiate the [Haar](#classHaar) sensor class.

#### `public uint8_t `[`begin`](#classHaar_1a80119761d396fae3f7f36dc5ef0f5d63)`(uint8_t ADR_)` 

Begin communications with the [Haar](#classHaar) sensor.

#### Parameters
* `ADR_` I2C address. Defaults to 0x40. DOES NOT ACTUALLY SEEM TO BE USED! 0x40 FOR EVERYTHING, REGARDLESS

#### `public float `[`GetPressure`](#classHaar_1a0f01c6a94097c9da103157d605a2640e)`(bool Update)` 

Return the currently stored pressure [in mBar].

#### Parameters
* `Update` Read and store a new pressure value before sending? By default is false; simply returns the already-available pressure value.

#### `public float `[`GetHumidity`](#classHaar_1ae70cc679d30a06bdd911e52db6346618)`(bool Update)` 

Return the currently stored relative humidity [%].

#### Parameters
* `Update` Read and store a new pressure value before sending? By default is false; simply returns the already-available RH value.

#### `public float `[`GetTemperature`](#classHaar_1a5e4c7cff226975a2ddade7cb63be3ba3)`(Sensor Device,bool Update)` 

Return the currently stored relative humidity [%].

#### Parameters
* `Device` Selects which sensor is used to measure the temperature. Input can be `RH_Sense`, which corresponds to a 0, or `Pres_Sense`, which corresponds to a 1. By default, this is RH_Sense, as this sensor has a better internal temperature sensor. 

* `Update` Read and store a new pressure value before sending? By default is false; simply returns the already-available RH value.

#### `public uint8_t `[`Sleep`](#classHaar_1ac4646f6c4b5532b1d4304a761eb77ad8)`(bool State)` 

Enable or disable device sleep mode THIS FUNCTION CURRENTLY DOES NOTHING!

#### Parameters
* `State` Can be `ON` (1) or `OFF` (0). By default, this command sets "Sleep" to ON, sending the device into a low-power sleep mode.

#### `public uint8_t `[`RequestSample`](#classHaar_1aff15e81cba7f13897ae38c8dfe21d73e)`(bool Block)` 

Return a new sample of all of the data.

#### Parameters
* `Block` if `true`, wait for data to be returned. Defaults to `false`.

#### `public bool `[`NewData`](#classHaar_1a1a99e2cb4d3a53a170bb9a96fac73440)`()` 

Checks for updated data. Returns `true` if new data are available; otherwise returns `false`

#### Parameters
* `Block` if `true`, wait for data to be returned. Defaults to `false`.

#### `public String `[`GetString`](#classHaar_1a58fc9627f957e39e8481837e8616d52a)`()` 

The most important function for the user! Returns all data as a comma-separated string: "P,RH,T(P),T[RH],".

This string is: <PRESSURE>,<RELATIVE_HUMIDITY, <TEMPERATURE-FROM-PRESSURE-SENSOR>,<TEMPERATURE-FROM-RH-SENSOR>,

#### `public String `[`GetHeader`](#classHaar_1a7069008fd8febc964d330a9313d5d79f)`()` 

Returns a header: "Pressure Atmos [mBar], Humidity [%], Temp Pres [C], Temp RH [C],".

Generated by [Moxygen](https://sourcey.com/moxygen)