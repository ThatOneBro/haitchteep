#include "http_utils.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * Checks if a buffer ends with CRLF CRLF sequence (\r\n\r\n)
 * @param buffer Pointer to the buffer to check
 * @param length Length of the buffer
 * @return true if buffer ends with \r\n\r\n, false otherwise
 */
inline bool check_http_end(const char *buffer, size_t length)
{
    // Check if buffer is NULL or too short
    if (buffer == NULL || length < 4) {
        return false;
    }

    // Check last 4 characters
    return (buffer[length - 4] == '\r' && buffer[length - 3] == '\n' && buffer[length - 2] == '\r' && buffer[length - 1] == '\n');
}
