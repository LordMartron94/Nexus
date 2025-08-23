/* Created by LordMartron on 20/08/2025. */

#include <nexus/nexus.h>
#include <stdio.h>

double nexus_bytes_byte_to_kilobytes_convert(const nexus_u64 bytes) {
  return (double)bytes / 1024.0;
}

double nexus_bytes_byte_to_kilobits_convert(const nexus_u64 bytes) {
  return (double)bytes / 1000.0;
}

void nexus_bytes_byte_array_hex_print(const unsigned char *byteArray, const nexus_u64 arraySize, FILE *out) {
  for (nexus_u64 i = 0; i < arraySize; ++i) {
    fprintf(out, "%02x", (unsigned)byteArray[i]);
  }
}