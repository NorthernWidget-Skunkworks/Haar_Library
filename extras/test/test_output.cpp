// Output-regression test for Haar_Library: compiles src/Haar.cpp against the
// NW_Core stubs and prints getHeader()/getString()/getters for fixed register
// images. run.sh diffs the result against baseline.txt.
#include "Arduino.h"
#include "Wire.h"
TwoWire Wire;
#include "../../src/Haar.cpp"

static uint8_t crc8(const uint8_t* d, uint8_t n) {           // CRC-8/SMBUS, as NW-Provision writes it
  uint8_t c = 0; for (uint8_t i = 0; i < n; i++) { c ^= d[i]; for (int b = 0; b < 8; b++) c = (c & 0x80) ? (c << 1) ^ 0x07 : (c << 1); }
  return c;
}

// Physical values used throughout: 1013.25 hPa, 55.00 %RH, 21.37 C (SHT31), 22.15 C (LPS35HW).
// Schema 1 register image: Page 0 as NW-Provision writes it (with the firmware's
// patch at 0x0A), Page 1 with a complete reading (Haar appendix: SHT31 temp int16
// 0.01 C at 0x28, humidity uint16 0.01 %RH at 0x2A, LPS35HW pressure uint32
// 0.01 hPa at 0x30, temperature int16 0.01 C at 0x34).
static void loadImage(uint32_t pres, uint16_t rh, int16_t tSHT, int16_t tLPS, uint8_t fwPatch = 1, uint8_t schema = 0x01) {
  uint8_t* r = Wire.image; memset(r, 0, sizeof(Wire.image));
  r[0x00] = schema; memcpy(r + 0x01, "Haar", 4);
  r[0x08] = 0; r[0x09] = 1; r[0x0A] = fwPatch;                       // HW 0.1, FW patch
  r[0x10] = 0x48; r[0x11] = 0x01; r[0x12] = 0; r[0x13] = 7; r[0x14] = 0; r[0x15] = 42;
  r[0x1D] = 0x4E; r[0x1E] = crc8(r, 0x1E); r[0x1F] = 0x48;
  r[0x20] = 0x01;                                                   // ready
  r[0x21] = 0x06;                                                   // both chips selected
  r[0x22] = 1; r[0x23] = 0;                                         // reading counter = 1
  r[0x26] = 0x00; r[0x27] = 0x00;
  r[0x28] = tSHT & 0xFF; r[0x29] = (tSHT >> 8) & 0xFF;
  r[0x2A] = rh & 0xFF;   r[0x2B] = (rh >> 8) & 0xFF;
  for (int i = 0; i < 4; i++) r[0x30 + i] = (pres >> (8 * i)) & 0xFF;
  r[0x34] = tLPS & 0xFF; r[0x35] = (tLPS >> 8) & 0xFF;
}

// Emulate the Schema 1 firmware's response to a control write: a trigger
// completes a reading at once (counter +1, ready set, trigger and sleep bits
// cleared, fault byte cleared). A per-test hook can vary the data.
static std::function<void(TwoWire&)> onReading;
static void installFirmwareEmulation() {
  Wire.onWrite = [](TwoWire& w, uint8_t reg, uint8_t val) {
    if (reg != 0x21) return;
    w.image[0x27] = 0;
    if (!(val & 0x01)) return;
    w.image[0x21] = val & 0x7E;
    if (onReading) onReading(w);
    uint16_t c = w.image[0x22] | (w.image[0x23] << 8); c++;
    w.image[0x22] = c & 0xFF; w.image[0x23] = c >> 8;
    w.image[0x20] |= 0x01;
  };
}

// Print into a fixed buffer: the in-memory Print destination from the design.
class BufferPrint : public Print {
  char* _buf; size_t _cap, _len = 0;
  public:
  BufferPrint(char* buf, size_t cap) : _buf(buf), _cap(cap) { _buf[0] = 0; }
  size_t write(uint8_t c) override { if (_len + 1 >= _cap) return 0; _buf[_len++] = c; _buf[_len] = 0; return 1; }
};

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
    bool ok = s.updateMeasurements(); BufferPrint bp(pb, sizeof pb); s.printFault(bp);
    printf("[SHT31 checksum] update=%d faulted(0)=%d faulted(1)=%d any=%d chip=%u kind=%u text='%s' note='%s'\n",
           ok, s.faulted(0), s.faulted(1), s.anyFault(), s.faultChip(), s.faultKind(), pb, s.faultNote().c_str());
    printf("[SHT31 checksum] string: %s\n", s.getString().c_str());
    onReading = [](TwoWire& w) { w.image[0x20] = 0x85; w.image[0x27] = 0x22; };
    String row = s.getString();   // evaluated before the note: printf argument order is unspecified
    printf("[LPS35HW timeout] string: %s note='%s'\n", row.c_str(), s.faultNote().c_str());
    onReading = [](TwoWire& w) { w.image[0x20] = 0x01; w.image[0x27] = 0xE6; };
    ok = s.updateMeasurements(); BufferPrint bp2(pb, sizeof pb); s.printFault(bp2);
    printf("[unit reset] update=%d any=%d chip=%u kind=%u text='%s' note='%s'\n", ok, s.anyFault(), s.faultChip(), s.faultKind(), pb, s.faultNote().c_str());
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

  fprintf(stderr, "bus transactions total: %u\n", Wire.transactions);   // metric, not output
  return 0;
}
