#ifndef CONFIG_H
#define CONFIG_H

/* Release: increase FIRMWARE_VERSION, run tools/build.sh, commit config.h together with build/ir-commander.ino.bin.
 * After each WiFi connection the device compares this number on GitHub and updates itself when it is higher. */
#define FIRMWARE_VERSION        1
#define OTA_GITHUB_REPOSITORY   "ujagaga/ir-commander"
#define OTA_GITHUB_BRANCH       "main"
#define OTA_FIRMWARE_PATH       "build/ir-commander.ino.bin"

#define PASSWORD                "ircommander"   // Config AP password (min 8 chars)
#define AP_NAME_PREFIX          "IR_Commander_" // Will be appended by device MAC
#define HOSTNAME                "ir-commander"  // http://ir-commander.local

#define AP_MODE_TIMEOUT_S       120 // While in AP only mode, retry STA connection after this period.
#define STA_CONNECT_TIMEOUT_S   30  // If STA does not connect within this period, fall back to AP only and retry after AP_MODE_TIMEOUT_S.

/* IR LED anode is connected to all these pins, each via its own resistor (~220R).
 * Cathode goes to GND. All pins are switched at the same instant, so the currents add up.
 * Use only GPIO 0..15 (GPIO16/D0 is not on the same port register).
 * Avoid D3(0), D4(2), D8(15) as they affect boot. D1=5, D2=4, D5=14, D6=12, D7=13 */
#define IR_PINS                 {5, 4, 14, 12, 13}
#define IR_PIN_MAIN             5   // One of IR_PINS. Passed to the IR library.

/* TV power toggle code. Unknown for VOX 32DIS471B, so this is a guess (RC5 TV standby: address 0, command 12).
 * Find the right one using /send?protocol=NEC&code=0x...&bits=32 and put it here. */
#define TV_POWER_PROTOCOL       "RC5"
#define TV_POWER_CODE           0x00C
#define TV_POWER_BITS           12

#define WIFI_PASS_EEPROM_ADDR   (0)
#define WIFI_PASS_SIZE          (64)  // WPA2 passphrase up to 63 chars + terminator
#define SSID_EEPROM_ADDR        (WIFI_PASS_EEPROM_ADDR + WIFI_PASS_SIZE)
#define SSID_SIZE               (33)  // SSID up to 32 chars + terminator
#define OTA_TARGET_EEPROM_ADDR  (SSID_EEPROM_ADDR + SSID_SIZE)
#define OTA_TARGET_SIZE         (4)   // uint32_t version being installed
#define EEPROM_SIZE             (WIFI_PASS_SIZE + SSID_SIZE + OTA_TARGET_SIZE)

#endif
