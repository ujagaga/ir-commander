#ifndef CONFIG_H
#define CONFIG_H

/* Release: increase FIRMWARE_VERSION, run tools/build.sh, commit config.h together with build/ir-commander.ino.bin.
 * After each WiFi connection the device compares this number on GitHub and updates itself when it is higher. */
#define FIRMWARE_VERSION        11
#define OTA_GITHUB_REPOSITORY   "ujagaga/ir-commander"
#define OTA_GITHUB_BRANCH       "main"
#define OTA_FIRMWARE_PATH       "build/ir-commander.ino.bin"

#define PASSWORD                "ircommander"   // Config AP password (min 8 chars)
#define AP_NAME_PREFIX          "IR_Commander_" // Will be appended by device MAC
#define HOSTNAME                "ir-commander"  // Default hostname until changed in Settings

#define AP_MODE_TIMEOUT_S       120 // While in AP only mode, retry STA connection after this period.
#define STA_CONNECT_TIMEOUT_S   30  // If STA does not connect within this period, fall back to AP only and retry after AP_MODE_TIMEOUT_S.

/* IR LED anode is connected to all these pins, each via its own resistor (~220R).
 * Cathode goes to GND. All pins are switched at the same instant, so the currents add up.
 * Use only GPIO 0..15 (GPIO16/D0 is not on the same port register).
 * Avoid D3(0), D4(2), D8(15) as they affect boot. D1=5, D2=4, D5=14, D6=12, D7=13 */
#define IR_PINS                 {5, 4, 14, 12, 13}
#define IR_PIN_MAIN             5   // One of IR_PINS. Passed to the IR library.

/* Defaults until changed on the Settings page (stored in EEPROM).
 * TV code is an index into the TV power list in ir_codes.h; it changes if that file is regenerated. */
#define TV_CODE_DEFAULT         235       // VOX 32DIS471B: RC5 0x4C 12 bits
#define AC_PROTOCOL_DEFAULT     "COOLIX"  // Midea RG70A/BGEF remote

/* Other TV buttons. VOX 32DIS471B: standard Philips RC5 commands on address 1 (12 bits). */
#define TV_BUTTONS_PROTOCOL     "RC5"
#define TV_BUTTONS_BITS         12
#define TV_VOLUME_UP_CODE       0x050     // command 16
#define TV_VOLUME_DOWN_CODE     0x051     // command 17
#define TV_MUTE_CODE            0x04D     // command 13

#define WIFI_PASS_EEPROM_ADDR   (0)
#define WIFI_PASS_SIZE          (64)  // WPA2 passphrase up to 63 chars + terminator
#define SSID_EEPROM_ADDR        (WIFI_PASS_EEPROM_ADDR + WIFI_PASS_SIZE)
#define SSID_SIZE               (33)  // SSID up to 32 chars + terminator
#define OTA_TARGET_EEPROM_ADDR  (SSID_EEPROM_ADDR + SSID_SIZE)
#define OTA_TARGET_SIZE         (4)   // uint32_t version being installed
#define TV_CODE_EEPROM_ADDR     (OTA_TARGET_EEPROM_ADDR + OTA_TARGET_SIZE)
#define TV_CODE_SIZE            (2)   // uint16_t index into TV power list
#define AC_PROTOCOL_EEPROM_ADDR (TV_CODE_EEPROM_ADDR + TV_CODE_SIZE)
#define AC_PROTOCOL_SIZE        (1)   // decode_type_t as uint8_t
/* Preserve the original 104-byte layout when migrating existing devices. */
#define EEPROM_LEGACY_SIZE      (AC_PROTOCOL_EEPROM_ADDR + AC_PROTOCOL_SIZE)
#define HOSTNAME_MARKER_ADDR    EEPROM_LEGACY_SIZE // Layout marker and home visibility flags
#define HOSTNAME_EEPROM_ADDR    (HOSTNAME_MARKER_ADDR + 4)
#define HOSTNAME_SIZE           (32) // 31 characters + terminator (ESP8266 hostname limit)
#define EEPROM_SIZE             (HOSTNAME_EEPROM_ADDR + HOSTNAME_SIZE)

#endif
