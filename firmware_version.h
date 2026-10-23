#ifndef FIRMWARE_VERSION_H
#define FIRMWARE_VERSION_H

#include <stdint.h>

// Accept exactly one positive decimal integer definition, never a quoted version.
bool parseFirmwareVersion(const char *config, uint32_t &version);

#endif
