// Output-regression test for Haar_Library: compiles src/Haar.cpp against the
// NW_Core stubs and prints getHeader()/getString()/getters for fixed register
// images. run.sh diffs the result against baseline.txt.
#include "Arduino.h"
#include "Wire.h"
TwoWire Wire;
#include "../../src/Haar.cpp"

#include "NW_TestSupport.h"

// Physical values used throughout: 1013.25 hPa, 55.00 %RH, 21.37 C (SHT31), 22.15 C (LPS35HW).
// Schema 1 register image: Page 0 as NW-Provision writes it (with the firmware's
// patch at 0x0A), Page 1 with a complete reading (Haar appendix: SHT31 temp int16
// 0.01 C at 0x28, humidity uint16 0.01 %RH at 0x2A, LPS35HW pressure uint32
// 0.01 hPa at 0x30, temperature int16 0.01 C at 0x34).
static void loadImage(uint32_t pres, uint16_t rh, int16_t tSHT, int16_t tLPS, uint8_t fwPatch = 1, uint8_t schema = 0x01) {
  uint8_t* r = Wire.image;
  nwLoadPage0(r, "Haar", 0x48, 1, fwPatch, schema);                 // Page 0 and Block 0, HW 0.1
  r[0x28] = tSHT & 0xFF; r[0x29] = (tSHT >> 8) & 0xFF;
  r[0x2A] = rh & 0xFF;   r[0x2B] = (rh >> 8) & 0xFF;
  for (int i = 0; i < 4; i++) r[0x30 + i] = (pres >> (8 * i)) & 0xFF;
  r[0x34] = tLPS & 0xFF; r[0x35] = (tLPS >> 8) & 0xFF;
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
  Wire.deviceAddress = 0x48;
  installFirmwareEmulation();

  // 1. A complete reading: the firmware now serves 0.01-unit values, so 55.00 %RH and 21.37 C exactly.
  loadImage(101325, 5500, 2137, 2215);
  { Haar s; bool ok = s.begin(); printf("begin=%d\n", ok); report("complete reading", s); }

  // 2. Negative temperatures: SHT31 -20.97 C; LPS35HW -12.34 C; 985.50 hPa; 100 %RH.
  loadImage(98550, 10000, -2097, -1234);
  { Haar s; s.begin(); report("negative temperatures", s); }

  // 3. Device absent: no acknowledge at the address.
  loadImage(101325, 5500, 2137, 2215); Wire.present = false;
  { Haar s; bool ok = s.begin(); printf("begin=%d\n", ok); report("device absent", s); }
  Wire.present = true;

  // 4. Device present but the reading never completes (no firmware response to the trigger).
  loadImage(101325, 5500, 2137, 2215); Wire.image[0x20] = 0x00; Wire.onWrite = nullptr;
  { Haar s; s.begin(); report("never ready", s); }
  installFirmwareEmulation();

  // 5. begin() gates: wrong name, wrong schema, firmware too old, and the versions it reports.
  loadImage(101325, 5500, 2137, 2215); Wire.image[0x01] = 'X';
  { Haar s; bool ok = s.begin(); printf("[wrong name] begin=%d failure=%s\n", ok, s.beginFailure().c_str()); }
  loadImage(101325, 5500, 2137, 2215, 1, 0x00);
  { Haar s; bool ok = s.begin(); printf("[schema 0x00] begin=%d failure=%s\n", ok, s.beginFailure().c_str()); }
  loadImage(101325, 5500, 2137, 2215, 0);
  { Haar s; bool ok = s.begin(); printf("[fw patch 0 < min %d] begin=%d fw=%u failure=%s\n", HAAR_FW_MIN_PATCH, ok, s.getFirmwareVersion(), s.beginFailure().c_str()); }
  loadImage(101325, 5500, 2137, 2215);
  { Haar s; bool ok = s.begin(); printf("[versions] begin=%d hw=%u.%u fw=%u failure=%s\n", ok, s.getHardwareMajor(), s.getHardwareMinor(), s.getFirmwareVersion(), s.beginFailure().c_str()); }

  // 6. Faults: the SHT31 fails its checksum (status bit 1, pan-fault, latched 0x03); the
  //    LPS35HW values survive. Then an LPS35HW timeout (0x22), then a unit reset with a clean status.
  loadImage(101325, 5500, 2137, 2215);
  { Haar s; s.begin(); char pb[48];
    onReading = [](TwoWire& w) { w.image[0x20] = 0x83; w.image[0x27] = 0x03; };
    bool ok = s.updateMeasurements(); BufferPrint bp(pb, sizeof pb); s.printReport(bp);
    printf("[SHT31 checksum] update=%d faulted(0)=%d faulted(1)=%d any=%d chip=%u kind=%u text='%s' note='%s'\n",
           ok, s.faulted(0), s.faulted(1), s.anyFault(), s.reportChip(), s.reportKind(), pb, s.reportNote().c_str());
    printf("[SHT31 checksum] string: %s\n", s.getString().c_str());
    onReading = [](TwoWire& w) { w.image[0x20] = 0x85; w.image[0x27] = 0x22; };
    String row = s.getString();   // evaluated before the note: printf argument order is unspecified
    printf("[LPS35HW timeout] string: %s note='%s'\n", row.c_str(), s.reportNote().c_str());
    onReading = [](TwoWire& w) { w.image[0x20] = 0x01; w.image[0x27] = 0xE6; };
    ok = s.updateMeasurements(); BufferPrint bp2(pb, sizeof pb); s.printReport(bp2);
    printf("[unit reset] update=%d any=%d chip=%u kind=%u text='%s' note='%s'\n", ok, s.anyFault(), s.reportChip(), s.reportKind(), pb, s.reportNote().c_str());
    onReading = nullptr; }

  // 7. Non-blocking path: request, poll newData() (captures), getters; then a request followed by getString().
  loadImage(101325, 5500, 2137, 2215);
  { Haar s; s.begin(); int k = 0;
    onReading = [&](TwoWire& w) { k++; uint32_t p = 101325 + 10 * k; for (int i = 0; i < 4; i++) w.image[0x30 + i] = (p >> (8 * i)) & 0xFF; };
    bool req = s.updateMeasurements(false); bool nd = s.newData();
    printf("[non-blocking] request=%d newData=%d pressure=%.2f (stale getter untouched by the request)\n", req, nd, s.getPressure());
    req = s.updateMeasurements(false); printf("[non-blocking] then getString: %s\n", s.getString().c_str());
    unsigned t0 = Wire.transactions; s.getString(); printf("[cost] requestFrom calls for one getString(): %u\n", Wire.transactions - t0);
    onReading = nullptr; }

  // 8. N readings with statistics: humidity steps through five values, pressure through three;
  //    the batch word reaches the device; getString() grows its columns.
  loadImage(101325, 5500, 2137, 2215);
  { Haar s; s.begin(); int k = 0;
    onReading = [&](TwoWire& w) { k++;
      int16_t rh = 5480 + 10 * (k % 5); w.image[0x2A] = rh & 0xFF; w.image[0x2B] = (rh >> 8) & 0xFF;
      uint32_t p = 101300 + 25 * (k % 3); for (int i = 0; i < 4; i++) w.image[0x30 + i] = (p >> (8 * i)) & 0xFF; };
    printf("[N] setHumidityReadings(5)=%u setPressureReadings(3)=%u setHumidityReadings(99)=%u\n",
           s.setHumidityReadings(5), s.setPressureReadings(3), s.setHumidityReadings(99));
    s.setHumidityReadings(5); s.setHumidityStats(true); s.setPressureStats(true);
    lastRequest = 0; unsigned t0 = Wire.transactions; bool ok = s.updateMeasurements();
    printf("[N=5,3] update=%d humidityCount=%u pressureCount=%u lastRequest=%u requestFrom=%u\n",
           ok, s.getHumidityCount(), s.getPressureCount(), lastRequest, Wire.transactions - t0);
    printf("[N=5,3] humidity mean=%.4f std=%.4f sterr=%.4f median=%.4f | pressure mean=%.4f std=%.4f median=%.4f | tRH mean=%.4f std=%.4f | tPres mean=%.4f\n",
           s.getHumidityMean(), s.getHumidityStd(), s.getHumiditySterr(), s.getHumidityMedian(),
           s.getPressureMean(), s.getPressureStd(), s.getPressureMedian(), s.getTemperatureMean(RH_Sense), s.getTemperatureStd(RH_Sense), s.getTemperatureMean(Pres_Sense));
    printf("[N=5,3] header: %s\n", s.getHeader().c_str());
    printf("[N=5,3] string: %s\n", s.getString().c_str());
    ok = s.updateMeasurements(Haar::LPS35HW);
    printf("[LPS35HW only] update=%d humidityCount=%u pressureCount=%u humidity=%.4f\n", ok, s.getHumidityCount(), s.getPressureCount(), s.getHumidity());
    onReading = nullptr; }

  // 9. Reading interface: header, three logged readings of ALL, then SHT31 alone; the
  //    batch word for the run reaches the device.
  loadImage(101325, 5500, 2137, 2215);
  { Haar s; s.begin(); int k = 0; char pb[96];
    onReading = [&](TwoWire& w) { k++; uint32_t p = 101300 + 5 * k; for (int i = 0; i < 4; i++) w.image[0x30 + i] = (p >> (8 * i)) & 0xFF; };
    lastRequest = 0; s.beginReadings(Haar::ALL, 3);
    BufferPrint bh(pb, sizeof pb); s.printHeader(bh); printf("[run ALL] header: %s lastRequest=%u\n", pb, lastRequest);
    for (int i = 0; i < 3; i++) { BufferPrint bp(pb, sizeof pb); size_t n = s.logReading(bp); printf("[run ALL] row %d (%zu bytes): %s\n", i, n, pb); }
    s.endReadings();
    printf("[run ALL] pressure count=%u mean=%.4f median=%.4f\n", s.getPressureCount(), s.getPressureMean(), s.getPressureMedian());
    s.beginReadings(Haar::SHT31);
    BufferPrint bh2(pb, sizeof pb); s.printHeader(bh2); printf("[run SHT31] header: %s\n", pb);
    BufferPrint bp2(pb, sizeof pb); s.logReading(bp2); s.endReadings(); printf("[run SHT31] row: %s\n", pb);
    onReading = nullptr; }

  // 10. A dead LPS35HW (no acknowledge on the first reading) stops its batch of 10.
  loadImage(101325, 5500, 2137, 2215);
  { Haar s; s.begin(); int k = 0;
    onReading = [&](TwoWire& w) { k++; w.image[0x20] = 0x85; w.image[0x27] = 0x21; };
    s.setPressureReadings(10); bool ok = s.updateMeasurements(Haar::LPS35HW);
    printf("[dead LPS35HW] N=10: update=%d readings taken=%d pressureCount=%u pressure=%.2f note='%s'\n", ok, k, s.getPressureCount(), s.getPressure(), s.reportNote().c_str());
    onReading = nullptr; }

  fprintf(stderr, "bus transactions total: %u\n", Wire.transactions);   // metric, not output
  return 0;
}
