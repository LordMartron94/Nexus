/* Created by LordMartron on 21/08/2025. */

#include <nexus/nexus.h>

nexus_u8 nexus_bits_bit_in_byte_lsb_get(const unsigned char byte, const nexus_u8 bitIndex) {
  if (bitIndex >= CHAR_BIT) return 0u;

  return byte >> bitIndex & 1u;
}

nexus_u8 nexus_bits_bit_in_byte_msb_get(const unsigned char byte, const nexus_u8 bitIndex) {
  if (bitIndex >= CHAR_BIT) return 0u;

  return byte >> 7 - bitIndex & 1u;
}
