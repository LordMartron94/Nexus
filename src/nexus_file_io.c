/* Created by LordMartron on 20/08/2025. */

#include <nexus/nexus.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static nexus_i32 nexus_file_seek(FILE *stream, nexus_i64 offset, nexus_i32 origin) {
#if defined(_WIN32) || defined(_WIN64)
    return _fseeki64(stream, offset, origin);
#else
    return fseeko(stream, (off_t)offset, origin);
#endif
}

/**
 * Reads up to readBufferSize bytes starting at (origin, offset) into readBuffer.
 *
 * Parameters:
 *   filePath        : path to file
 *   offset          : byte offset relative to origin (can be > 2GB)
 *   origin          : SEEK_SET, SEEK_CUR, or SEEK_END
 *   readBuffer      : destination buffer
 *   readBufferSize  : capacity of destination buffer in bytes
 *   outBytesRead    : [out] actual number of bytes read (can be < readBufferSize)
 *   errorBuffer     : destination for human-readable error message
 *   errorBufferSize : size of errorBuffer
 */
void nexus_file_read_at(
    const char *filePath, const nexus_i64 offset, const nexus_i32 origin,
    void *readBuffer,
    const size_t readBufferSize,
    size_t *outNumberBytesRead,
    char *errorBuffer,
    const nexus_u16 errorBufferSize
) {
    if (!filePath || !readBuffer || readBufferSize == 0) {
        nexus_string_message_copy(errorBuffer, errorBufferSize, "Invalid arguments.");
        return;
    }

    if (outNumberBytesRead) *outNumberBytesRead = 0;

    FILE *fileStream = NULL;
#if defined(_MSC_VER)
    errno_t openError = fopen_s(&fileStream, filePath, "rb");
    if (openError != 0 || !fileStream) {
        char systemMessage[NEXUS_ERROR_MESSAGE_MAX];
        if (strerror_s(systemMessage, sizeof systemMessage, openError) == 0) {
            nexus_string_message_copy(errorBuffer, errorBufferSize, systemMessage);
        } else {
            nexus_string_message_copy(errorBuffer, errorBufferSize, "Cannot open file.");
        }
        return;
    }
#else
    fileStream = fopen(filePath, "rb");
    if (!fileStream) {
        nexus_string_message_copy(errorBuffer, errorBufferSize, strerror(errno));
        return;
    }
#endif

    // Seek to desired start
    if (nexus_file_seek(fileStream, offset, origin) != 0) {
#if defined(_MSC_VER)
        char systemMessage[NEXUS_ERROR_MESSAGE_MAX];
        if (strerror_s(systemMessage, sizeof systemMessage, errno) == 0) {
            nexus_string_message_copy(errorBuffer, errorBufferSize, systemMessage);
        } else {
            nexus_string_message_copy(errorBuffer, errorBufferSize, "Seek failed.");
        }
#else
        enigma_message_copy(errorBuffer, errorBufferSize, strerror(errno));
#endif
        fclose(fileStream);
        return;
    }

    // Read up to capacity
    const size_t amountRead = fread(readBuffer, 1u, readBufferSize, fileStream);

    if (amountRead == 0) {
        if (ferror(fileStream)) {
#if defined(_MSC_VER)
            char systemMessage[NEXUS_ERROR_MESSAGE_MAX];
            if (strerror_s(systemMessage, sizeof systemMessage, errno) == 0) {
                nexus_string_message_copy(errorBuffer, errorBufferSize, systemMessage);
            } else {
                nexus_string_message_copy(errorBuffer, errorBufferSize, "Read error.");
            }
#else
            nexus_string_message_copy(errorBuffer, errorBufferSize, strerror(errno));
#endif
            fclose(fileStream);
            return;
        }
    }

    if (outNumberBytesRead) *outNumberBytesRead = amountRead;

    fclose(fileStream);
}
