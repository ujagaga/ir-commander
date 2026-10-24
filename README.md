# IR Commander

WiFi IR remote for an ESP8266 (Wemos/LOLIN D1 mini). It turns a TV on/off,
changes its volume and controls an air conditioner from a web page or by plain HTTP GET requests on the
local network. It updates its own firmware from this GitHub repository.

Currently configured for:

- **TV:** VOX 32DIS471B (RC5 address 1: power `0x4C`, volume up `0x50`,
  volume down `0x51`, mute `0x4D`)
- **AC:** Midea with RG70A/BGEF remote (`COOLIX` protocol)

The TV power code and the AC protocol can be changed from the web page without
rebuilding. The volume and mute codes are fixed in `config.h`
(`TV_VOLUME_UP_CODE`, `TV_VOLUME_DOWN_CODE`, `TV_MUTE_CODE`).

## Hardware

One IR LED driven from five GPIO pins at once. Each pin goes through its own
resistor to the LED, so the currents add up while every pin stays under the
ESP8266's 12 mA limit.

| D1 mini pin | GPIO | Connection |
|---|---|---|
| D1 | 5 | 220 Ω → LED anode (long leg) |
| D2 | 4 | 220 Ω → LED anode |
| D5 | 14 | 220 Ω → LED anode |
| D6 | 12 | 220 Ω → LED anode |
| D7 | 13 | 220 Ω → LED anode |
| GND | | LED cathode (short leg, flat side) |

About 9 mA per pin, roughly 45 mA in total. All pins switch in the same instant:
the firmware overrides `digitalWrite` for `IR_PIN_MAIN` and writes all IR pins
with one register write (see `ir_control.cpp`). Pins are set in `config.h`
(`IR_PINS`). Avoid D0, D3, D4 and D8.

Optional DS18B20 room temperature sensor (AC device):

| D1 mini pin | GPIO | Connection |
|---|---|---|
| D3 | 0 | DS18B20 data (middle leg); 4.7 kΩ to 3V3 recommended |
| 3V3 | | DS18B20 VDD |
| GND | | DS18B20 GND |

The D1 mini already has a 10 kΩ pull-up on D3, which is enough for a short
wire. The sensor keeps the line high at boot, so D3 (which must be high at
boot) is safe. It is detected once at boot; without it the room temperature
is not shown. The pin is `TEMP_PIN` in `config.h`.

Notes:

- Use a 940 nm IR LED if possible. An 850 nm LED (e.g. from a surveillance
  camera) works, but with less range.
- Aim matters. The AC receiver is a small window on the indoor unit; the AC
  only reacts when the LED points at it.
- For more range, drive the LED through a transistor (e.g. BC547/2N2222) from
  the 5 V pin and set a single pin in `IR_PINS`.
- To check that the LED emits, look at it through a phone's front camera while
  holding the TV button. Many rear cameras filter IR.
- The onboard LED (D4) is on while a firmware update downloads.

## Building and flashing

Requires `arduino-cli` with the ESP8266 core 3.1.2. Install the core and
libraries (IRremoteESP8266, ESP_EEPROM, OneWire, DallasTemperature) with:

```
tools/install_dependencies.sh
```

Then:

```
tools/build.sh        # builds into build/ (board esp8266:esp8266:d1_mini)
tools/upload_usb.sh   # flashes build/ir-commander.ino.bin over /dev/ttyUSB0
```

The VS Code buttons in `.vscode/settings.json` run the same scripts and open the
serial monitor (115200 baud).

After flashing over USB, press the reset button once. The first software
restart after serial programming can hang otherwise.

## WiFi setup

1. On first start (no WiFi saved) the device starts an access point
   `IR_Commander_<MAC>`, password `ircommander`.
2. Connect to it and open `http://192.168.4.1`.
3. Go to **Settings → Configure WiFi**, pick your network, enter the password
   and save. The device restarts and joins your network.
4. Open `http://ir-commander.local`, or the IP address your router assigned
   (shown on the Settings page and in the serial log). Some Android browsers
   don't resolve `.local` names. WiFi power saving is disabled on the device
   (about 70 mA instead of ~20 mA) because in sleep it misses the multicast
   mDNS queries, which made `.local` lookups unreliable. On Linux computers
   with WiFi power saving on, lookups can still fail; turn it off there too
   (NetworkManager: `wifi.powersave = 2`) or add the device to `/etc/hosts`.

Connection behaviour:

- With saved WiFi it connects at boot. The access point stays up until the
  connection works and nobody is connected to it.
- If the connection fails within 30 s, it falls back to the access point only
  and retries every 120 s.
- If the connection drops later, it reconnects the same way.

To change the network later, use **Settings → Configure WiFi**. Saving an
empty SSID switches to access point only mode.

## Web interface

The mobile-friendly interface switches between **Home** and **Settings**. All
styles and scripts are served by the device, with no external dependencies.
Controls stack on phones and sit side by side on larger screens. HTML is streamed from
flash in small chunks. SVG icons load separately from content-versioned
`/icons/` URLs and are cached by the browser. Only the selected AC mode's icon
is requested; changing mode updates its image URL. HTML is not cached, so saved
settings and new icon URLs are picked up when the page is reloaded.

Icon assets and their routes live in `ui.h`. When changing an SVG, update
its URL hash to the first 12 characters of its SHA-256 digest and update the
references in `ui.h`. Run `python3 tests/check_web_icons.py` to verify
asset hashes, SVG validity, and page references.

### Home

- **TV buttons:** power, Vol −, Mute and Vol +. A tap sends the command;
  holding a button keeps sending it until release (volume keeps changing, and
  holding power is useful for checking the LED with a phone camera).
  Volume and mute use the fixed RC5 codes from `config.h`, so they only work
  for this TV (or another TV using standard RC5 on address 1).
- **Air conditioner.** Pick mode (Cool, Heat, Auto, Dry, Fan), temperature
  with the − / + buttons (17–30 °C) and fan speed (Auto, Low, Medium, High),
  then press **On / Apply** or **Off**. AC remotes send the complete state with every press, so the AC goes
  straight to the chosen settings. The last sent mode, temperature and fan
  speed are shown again when the page is reloaded (kept in RAM, reset to
  Cool / 24 °C / Auto on reboot). A spinning circle around the temperature
  shows that the last command sent was **On / Apply**. Command feedback confirms sending, not the
  actual device state; infrared does not report AC power back.
  With a DS18B20 connected, the room temperature is shown under the target
  temperature value and refreshed every 30 s (dash until the first reading).

### Settings

- **Home page controls.** Choose **Show TV controls** and **Show AC controls**,
  then press **Save home page controls**. The choices are saved in EEPROM for
  all clients and applied the next time Home is opened or refreshed; no restart
  is needed. Both are shown by default. Hidden controls remain available through
  the HTTP API, and Settings stays accessible even when both are hidden.

- **TV code.** Dropdown with 311 TV power codes from the Flipper Zero
  universal remote list. The saved one is selected.
  - **Scan** sends the next code every 2 s. Point the LED at the TV (TV on)
    and press **Stop** when it turns off.
  - **Prev / Test / Next** send single codes to confirm the right one. It may
    be the code shown or the one before.
- **AC protocol.** Dropdown with all AC protocols the IRremoteESP8266 library
  can generate (~80 brands). The saved one is selected.
  - **Scan**: with the AC running, sends OFF in the next protocol every 3 s.
    Stop when the AC turns off.
  - **Test ON / Test OFF** send the selected protocol (cool, 24 °C, fan auto).
- **Save TV code and AC protocol** stores both to EEPROM. Until saved, the
  defaults from `config.h` are used (`TV_CODE_DEFAULT`, `AC_PROTOCOL_DEFAULT`).
- **Advanced: test any code** expands to send a code typed by hand (protocol name, hex code,
  optional bit count).
- **System**: WiFi configuration, manual firmware update check, connection and
  firmware status.
- **Hostname** (under System): enter a name such as `living-room`, then press
  **Save hostname and restart**. After restart, open `http://living-room.local`
  or the device IP. Use 1–31 letters, numbers or hyphens, starting and ending
  with a letter or number. Names are saved in lowercase in EEPROM; leave the
  field blank to restore `HOSTNAME` from `config.h`. Existing WiFi credentials
  and TV/AC settings are preserved when upgrading to this EEPROM layout.

## HTTP API

Control requests are plain GET, without authentication (meant for a trusted
local network). Responses are `text/plain`. Hostname changes use
`POST /hostname` with a form field named `hostname`; successful saves return
an HTML reconnect page and restart the device.
`POST /visibility` accepts form fields `tv` and `ac` (each `0` or `1`) to save
Home visibility without restarting.

| Request | Action |
|---|---|
| `/tv?cmd=power` | TV power (saved TV code) |
| `/tv?cmd=volume_up` | TV volume up |
| `/tv?cmd=volume_down` | TV volume down |
| `/tv?cmd=mute` | TV mute |
| `/ac?power=1&mode=cool&temp=24&fan=auto` | Send AC state with the saved protocol |
| `/ac?power=0` | Turn the AC off |
| `/ac?...&protocol=COOLIX` | Same, with a specific protocol |
| `/acprotocols` | Supported AC protocols, separated by `\|` |
| `/tvcodes` | TV power code list, one description per line (line number = index) |
| `/scan?i=235` | Send TV code number 235 from the list |
| `/save?tv=235&ac=COOLIX` | Store TV code index and AC protocol |
| `/send?protocol=NEC&code=0x20DF10EF&bits=32&repeat=0` | Send any code (`bits` and `repeat` optional) |
| `/temperature` | Room temperature in °C, empty until the first reading; 404 without a DS18B20 |
| `/update` | Check GitHub for a newer firmware now |

`mode`: `cool`, `heat`, `auto`, `dry`, `fan`. `fan`: `auto`, `low`, `medium`,
`high`. `temp` is in °C.

Example from a shell or a home automation system:

```
curl "http://ir-commander.local/tv?cmd=power"
curl "http://ir-commander.local/tv?cmd=volume_up"
curl "http://ir-commander.local/ac?power=1&mode=cool&temp=23&fan=low"
```

## Firmware updates from GitHub

After each WiFi connection, and when **Check for update** is pressed, the
device:

1. Gets the latest commit of `ujagaga/ir-commander`, branch `main`.
2. Reads `FIRMWARE_VERSION` from `config.h` in that commit.
3. If that number is higher than the running version, downloads
   `build/ir-commander.ino.bin` from the same commit, installs it and restarts.

Failed checks are retried up to 3 times, one minute apart. Before restarting it
stores the version it expects. If it boots and that version is not running
(e.g. `config.h` was bumped but the binary was built from older code), that
version is marked invalid and skipped, so the device can't get stuck in an
update loop. Progress is printed on the serial port.

HTTPS certificates are **not** verified, so anyone able to intercept the
device's internet traffic could serve it different firmware. The repository
must be public.

To release a new version:

1. Increase `FIRMWARE_VERSION` in `config.h`.
2. Run `tools/build.sh`.
3. Commit `config.h` together with `build/ir-commander.ino.bin` (the only build
   output tracked by git) and push to `main`.

## TV code list

`ir_codes.h` is generated from the Flipper Zero universal remote file `tv.ir`
by `tools/gen_ir_codes.py`. Usage instructions are at the top of the script.
Regenerating it can change the order of codes, and so the meaning of the saved
TV code index. Re-select the TV code on the Settings page afterwards.

## Source files

| File | Purpose |
|---|---|
| `ir-commander.ino` | Setup and main loop |
| `config.h` | Pins, defaults, version, OTA source, EEPROM layout |
| `ir_control.cpp/.h` | IR sending: TV codes, AC protocols, multi-pin LED drive, stored settings |
| `ir_codes.h` | Generated TV power code list |
| `http_server.cpp/.h` | HTTP API, routing and page streaming |
| `ui.h` | Static HTML, CSS, JavaScript, page fragments and SVG assets |
| `wifi_connection.cpp/.h` | WiFi station/access point handling, mDNS |
| `temp_sensor.cpp/.h` | Optional DS18B20 room temperature |
| `ota.cpp/.h` | Firmware update from GitHub |
| `firmware_version.cpp/.h` | Parses `FIRMWARE_VERSION` from the downloaded `config.h` |
| `tools/` | Build, upload, dependency install and code list generator scripts |
