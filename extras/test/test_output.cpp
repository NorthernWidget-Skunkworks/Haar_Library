// Output-regression test for Haar_Library: compiles src/Haar.cpp against the
// NW_Core stubs and prints getHeader()/getString()/getters for fixed register
// images. run.sh diffs the result against baseline.txt.
#include "Arduino.h"
#include "Wire.h"
TwoWire Wire;
#include "../../src/Haar.cpp"

// Physical values used throughout: 1013.25 hPa, 55.00 %RH, 21.37 C (SHT31), 22.15 C (LPS35HW).
// Legacy register image (firmware before Schema 1): trigger at 0x00 bit 0,
// raw chip counts at 0x02-0x0A, little-endian.
static void loadLegacyImage(uint32_t presRaw, uint16_t shtTempRaw, uint16_t rhRaw, int16_t lpsTemp) {
  uint8_t* r = Wire.image; memset(r, 0, sizeof(Wire.image));
  r[0x02] = lpsTemp & 0xFF;    r[0x03] = (lpsTemp >> 8) & 0xFF;
  r[0x04] = shtTempRaw & 0xFF; r[0x05] = (shtTempRaw >> 8) & 0xFF;
  r[0x06] = presRaw & 0xFF; r[0x07] = (presRaw >> 8) & 0xFF; r[0x08] = (presRaw >> 16) & 0xFF;
  r[0x09] = rhRaw & 0xFF;      r[0x0A] = (rhRaw >> 8) & 0xFF;
}

// Legacy firmware emulation: a write of 1 to 0x00 completes the conversion at once (bit cleared).
static void installLegacyFirmware() {
  Wire.onWrite = [](TwoWire& w, uint8_t reg, uint8_t val) { if (reg == 0x00 && (val & 0x01)) w.image[0x00] = val & 0xFE; };
}

static void report(const char* name, Haar& s) {
  printf("[%s]\n", name);
  printf("header: %s\n", s.getHeader().c_str());
  printf("string: %s\n", s.getString().c_str());
  printf("getters: pressure=%.4f humidity=%.4f tPres=%.4f tRH=%.4f default=%.4f newData=%d\n",
         s.getPressure(), s.getHumidity(), s.getTemperature(Pres_Sense), s.getTemperature(RH_Sense), s.getTemperature(), s.newData());
  printf("update: updateMeasurements(true)=%u pressure(update)=%.4f\n", s.updateMeasurements(true), s.getPressure(true));
}

int main() {
  Wire.deviceAddress = 0x42;
  installLegacyFirmware();

  // 1. A complete reading. Raw counts chosen so the physical values above come out exactly
  //    where the scaling allows: SHT31 temp 24856 -> 21.3737 C; RH 36044 -> 54.9996 %.
  loadLegacyImage(4150272, 24856, 36044, 2215);
  { Haar s; bool ok = s.begin(); printf("begin=%d\n", ok); report("complete reading", s); }

  // 2. Negative temperatures: SHT31 raw 9000 -> -20.9670 C; LPS35HW -1234 -> -12.34 C; 985.50 hPa; 100 %RH.
  loadLegacyImage(4036608, 9000, 65535, -1234);
  { Haar s; s.begin(); report("negative temperatures", s); }

  // 3. Device absent: no acknowledge at the address.
  loadLegacyImage(4150272, 24856, 36044, 2215); Wire.present = false;
  { Haar s; bool ok = s.begin(); printf("begin=%d\n", ok); report("device absent", s); }
  Wire.present = true;

  // 4. Device present but the conversion never completes (trigger bit stays set).
  loadLegacyImage(4150272, 24856, 36044, 2215); Wire.onWrite = nullptr;
  { Haar s; s.begin(); report("never ready", s); }
  installLegacyFirmware();

  fprintf(stderr, "bus transactions total: %u\n", Wire.transactions);   // metric, not output
  return 0;
}
