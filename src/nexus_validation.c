/* Created by LordMartron on 23/08/2025. */

#include <nexus/nexus.h>

nexus_u8 nexus_validation_crc8_update(nexus_u8 crc, const nexus_u8 byte) {
  crc ^= byte;
  for (int i = 0; i < 8; ++i) {
    if (crc & 0x80u) crc = (nexus_u8)((crc << 1) ^ 0x07u);
    else             crc = (nexus_u8)(crc << 1);
  }
  return crc;
}
