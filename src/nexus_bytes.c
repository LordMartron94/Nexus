/* Created by LordMartron on 20/08/2025. */

#include <nexus/nexus.h>

double nexus_bytes_byte_to_kilobytes_convert(const nexus_u64 bytes) {
  return (double)bytes / 1024.0;
}

double nexus_bytes_byte_to_kilobits_convert(const nexus_u64 bytes) {
  return (double)bytes / 1000.0;
}
