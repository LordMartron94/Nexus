/* Created by LordMartron on 20/08/2025. */

#include <nexus/nexus.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "internal/nexus_internal.h"

/* =========================
   Platform & error helpers
   ========================= */

static nexus_i32 nexus__seek64(FILE *stream, nexus_i64 offset, nexus_i32 origin) {
#if defined(_WIN32) || defined(_WIN64)
    return _fseeki64(stream, offset, origin);
#else
    return fseeko(stream, (off_t)offset, origin);
#endif
}

static nexus_i64 nexus__tell64(FILE *stream) {
#if defined(_WIN32) || defined(_WIN64)
    return _ftelli64(stream);
#else
    off_t pos = ftello(stream);
    return (nexus_i64)pos;
#endif
}

static NEXUS_BOOL nexus__file_size_get(FILE *stream, nexus_u64 *out_size) {
    if (!stream || !out_size) return NEXUS_FALSE;

    const nexus_i64 save = nexus__tell64(stream);
    if (save < 0) return NEXUS_FALSE;

    if (nexus__seek64(stream, 0, SEEK_END) != 0) return NEXUS_FALSE;
    const nexus_i64 end = nexus__tell64(stream);

    if (nexus__seek64(stream, save, SEEK_SET) != 0) return NEXUS_FALSE;
    if (end < 0) return NEXUS_FALSE;

    *out_size = (nexus_u64)end;
    return NEXUS_TRUE;
}

/* Clamp request to remaining file; optionally return remaining bytes. */
static nexus_u64 nexus__clamp_readable(const NEXUS_FILE_INFORMATION_CHANDLE handle,
                                       nexus_u64 request,
                                       nexus_u64 *out_remaining_file)
{
    nexus_u64 size = 0;
    if (!nexus__file_size_get(handle->stream, &size)) {
        if (out_remaining_file) *out_remaining_file = (nexus_u64)-1;
        return request;
    }

    if (handle->index >= size) {
        if (out_remaining_file) *out_remaining_file = 0;
        return 0;
    }

    const nexus_u64 remain = size - handle->index;
    if (out_remaining_file) *out_remaining_file = remain;
    if (request > remain) request = remain;
    return request;
}

/* =========================
   Core read routine
   ========================= */

typedef struct {
  NEXUS_BOOL peek;    /* if true, restore stream POS; do not advance index */
  NEXUS_BOOL advance; /* if true, advance index by bytes read/consumed     */
} nexus__read_flags;

// ReSharper disable once CppParameterMayBeConst
static NEXUS_BOOL nexus__read_core(NEXUS_FILE_INFORMATION_HANDLE handle,
                                   const nexus_u64 byteAmount,
                                   void *destination,
                                   const nexus_u64 destinationSize,
                                   nexus_u64 *outBytesRead,
                                   char *errorBuffer,
                                   const nexus_u64 errorBufferSize,
                                   const nexus__read_flags flags)
{
    nexus_string_buffer_reset(errorBuffer, errorBufferSize);
    if (outBytesRead) *outBytesRead = 0;

    if (!handle || !handle->stream) {
        nexus_string_message_copy(errorBuffer, errorBufferSize, "Invalid handle.");
        return NEXUS_FALSE;
    }

    /* Compute to_read respecting destination capacity (if any) and file size. */
    nexus_u64 toRead = byteAmount;
    if (destination && destinationSize > 0 && toRead > destinationSize) {
        toRead = destinationSize;
    }

    toRead = nexus__clamp_readable(handle, toRead, NULL);
    if (toRead == 0) {
        return NEXUS_TRUE; /* EOF or zero request is not an error */
    }

    /* Save current physical position to restore if peeking. */
    const nexus_i64 save_pos = nexus__tell64(handle->stream);
    if (save_pos < 0) {
        nexus_string_message_copy(errorBuffer, errorBufferSize, "Tell failed.");
        return NEXUS_FALSE;
    }

    /* Seek to logical index before reading/skipping. */
    if (nexus__seek64(handle->stream, (nexus_i64)handle->index, SEEK_SET) != 0) {
        nexus_errors_error_number_format(errorBuffer, errorBufferSize, "Seek failed.", errno);
        return NEXUS_FALSE;
    }

    nexus_u64 got = 0;

    if (destination && destinationSize > 0) {
        got = fread(destination, 1u, toRead, handle->stream);
    } else {
        if (nexus__seek64(handle->stream, (nexus_i64)toRead, SEEK_CUR) != 0) {
            nexus_errors_error_number_format(errorBuffer, errorBufferSize, "Skip failed.", errno);
            return NEXUS_FALSE;
        }
        got = toRead;
    }

    if (ferror(handle->stream)) {
        nexus_errors_error_number_format(errorBuffer, errorBufferSize, "Read error.", errno);
        (void)nexus__seek64(handle->stream, save_pos, SEEK_SET);
        return NEXUS_FALSE;
    }

    if (flags.peek == NEXUS_TRUE) {
        /* Restore physical position; logical index untouched for peek. */
        (void)nexus__seek64(handle->stream, save_pos, SEEK_SET);
    } else if (flags.advance == NEXUS_TRUE) {
        handle->index += got;
    }

    if (outBytesRead) *outBytesRead = got;
    return NEXUS_TRUE;
}

/* ----- PUBLIC API --------------------------------------------------------- */

NEXUS_BOOL nexus_file_information_open(const char *filePath,
                                       NEXUS_FILE_INFORMATION_HANDLE *outHandle,
                                       char *errorBuffer,
                                       const nexus_u64 errorBufferSize) {
  nexus_string_buffer_reset(errorBuffer, errorBufferSize);
  if (!filePath || !outHandle) {
    nexus_string_message_copy(errorBuffer, errorBufferSize,
                              "Invalid arguments.");
    return NEXUS_FALSE;
  }

  *outHandle = NULL;

  FILE *fileStream = NULL;
#if defined(_MSC_VER)
  const errno_t openError = fopen_s(&fileStream, filePath, "rb");
  if (openError != 0 || !fileStream) {
    nexus_errors_error_number_format(errorBuffer, errorBufferSize,
                                     "Cannot open file.", openError);
    return NEXUS_FALSE;
  }
#else
  fileStream = fopen(filePath, "rb");
  if (!fileStream) {
    nexus_errors_error_number_format(errorBuffer, errorBufferSize,
                                     "Cannot open file.", errno);
    return NEXUS_FALSE;
  }
#endif

  NEXUS_FILE_INFORMATION *info = NEXUS_ALLOC(sizeof *info);
  if (!info) {
    if (fileStream)
      fclose(fileStream);
    nexus_string_message_copy(errorBuffer, errorBufferSize, "Out of memory.");
    return NEXUS_FALSE;
  }
  info->stream = fileStream;
  info->index = 0u;

  *outHandle = info;
  return NEXUS_TRUE;
}

// ReSharper disable once CppParameterMayBeConst
void nexus_file_information_close(NEXUS_FILE_INFORMATION_HANDLE handle) {
    if (!handle) return;
    if (handle->stream) fclose(handle->stream);
    NEXUS_FREE(handle);
}

NEXUS_BOOL nexus_file_reader_peek(const NEXUS_FILE_INFORMATION_CHANDLE handle,
                                  const nexus_u64 byteAmount, void *destination,
                                  const nexus_u64 destinationSize,
                                  nexus_u64 *outBytesRead, char *errorBuffer,
                                  const nexus_u64 errorBufferSize) {
  const nexus__read_flags f = {NEXUS_TRUE, NEXUS_FALSE};
  return nexus__read_core((NEXUS_FILE_INFORMATION_HANDLE)handle, byteAmount,
                          destination, destinationSize, outBytesRead,
                          errorBuffer, errorBufferSize, f);
}

// ReSharper disable once CppParameterMayBeConst
NEXUS_BOOL nexus_file_reader_consume(NEXUS_FILE_INFORMATION_HANDLE handle,
                                     const nexus_u64 byteAmount,
                                     void *destination,
                                     const nexus_u64 destinationSize,
                                     nexus_u64 *outBytesRead,
                                     char *errorBuffer,
                                     const nexus_u64 errorBufferSize)
{
    const nexus__read_flags f = { NEXUS_FALSE, NEXUS_TRUE };
    return nexus__read_core(handle, byteAmount, destination, destinationSize,
                            outBytesRead, errorBuffer, errorBufferSize, f);
}

nexus_u64 nexus_file_reader_index_get(const NEXUS_FILE_INFORMATION_CHANDLE handle)
{
    if (!handle) return 0u;
    return handle->index;
}