# 04-Moire_disc-DC_motor-fw

Firmware for the single-DC-motor version of the Moiré disc. One brushed DC
motor turns the disc through a Pololu MAX14870 driver, and a ring of 9
NeoPixels lights it from behind. You control everything from a smartphone. The
disc runs its own Wi-Fi network, and a web page served from it has the motor
and light controls.

It runs on a **Seeed XIAO ESP32-C3** or a **Seeed XIAO ESP32-C6**. Both use the
same source code and pins.

This is a rework of [04-Moire disc](../04-Moire%20disc). That version drove two
stepper motors and took commands from a custom knob controller over ESP-NOW.

---

## Features

- **Wi-Fi access point + phone web page**: no app to install and no internet
  needed. The phone joins the `MoireDisc` network and the control page opens.
- **Motor control**:
  - *Manual*: a speed slider from −100 % to +100 %, with a centre detent at 0,
    plus a **Reverse** button.
  - *Sweep*: the speed follows a slow sine wave between a low and a high speed,
    with an adjustable cycle time. Set the low speed below zero to make the
    disc rock back and forth.
  - A big **Stop** button.
  - The speed changes gradually (soft ramp), and the direction pin only switches
    while the motor is stopped. This protects the motor, gearbox and power
    supply.
- **Light effects** (switching between effects crossfades; nothing flashes):
  - *Gradient*: a colour gradient that turns around the ring. You set the
    colour, spread and rotation. This replaces the old COLOR mode.
  - *Rainbow*: one full rainbow turning around the ring, with the colours
    slowly drifting. This is the old standalone look.
  - *Solid*: one colour on every LED.
  - *Aurora* (new): soft colour clouds that drift around the ring. It is made
    from two hue waves travelling in opposite directions and a gentle
    brightness wave. Every period is 7 s or longer, and the brightness never
    drops below about 35 %.
  - *Off*.
  - **Turn with the motor**: the ring's rotation follows the motor's real speed
    and direction, so the light pattern turns along with the disc.
- **Settings are saved** in flash, 2 s after the last change. After a power
  cycle the disc starts with whatever was set last, so it runs on its own
  without a phone. The first boot uses slow motor speed and the Rainbow effect.
- **Several phones** can be connected at once. The page polls every 1.5 s, so
  every phone shows the same state.

---

## Hardware

| Part | Notes |
|---|---|
| Seeed XIAO ESP32-C3 **or** XIAO ESP32-C6 | Firmware builds for both |
| Pololu MAX14870 single brushed DC motor driver carrier | Sign-magnitude control through DIR + PWM, motor supply 4.5–36 V |
| Brushed DC (gear) motor | Drives the disc |
| 9 × WS2812B / NeoPixel LEDs | GRB, 800 kHz |
| Power supply | <!-- TODO: motor supply voltage, and how the 5 V rail is made --> |

### Pinout

| XIAO pin | C3 GPIO | C6 GPIO | Connects to |
|---|---|---|---|
| **D1** | GPIO3 | GPIO1 | MAX14870 **PWM** |
| **D2** | GPIO4 | GPIO2 | MAX14870 **DIR** |
| **D6** | GPIO21 | GPIO16 | LED ring **DIN** (through a series resistor) |
| 5V | – | – | LED ring 5 V (and XIAO supply) |
| 3V3 | – | – | – |
| GND | – | – | common ground: MAX14870 GND, LED GND, supply GND |

MAX14870 pins that are not driven by the firmware:

- **EN** (active low): must be low for the driver to run. Tie it to GND, or
  leave it open if your carrier pulls it low on the board (check the Pololu
  documentation for your revision).
- **FAULT**: open-drain output, not used.

> **Note: D6 is UART0 TX on both chips.** The firmware sends `Serial` over
> native USB (`ARDUINO_USB_CDC_ON_BOOT=1` in `platformio.ini`), so nothing is
> printed on D6 at run time. The ROM bootloader still prints its boot log on
> that pin for a moment after reset. The first LED can briefly show a random
> colour at power-up before the firmware takes over. This is harmless.

---

## Building and flashing

The project uses [PlatformIO](https://platformio.org/). The `pioarduino`
platform provides Arduino core 3.x, which the ESP32-C6 needs.

```sh
# XIAO ESP32-C3
pio run -e xiao_c3 -t upload

# XIAO ESP32-C6
pio run -e xiao_c6 -t upload

# serial monitor (115200)
pio device monitor
```

In VS Code you can pick the environment in the PlatformIO status bar instead.

The only library dependency is Adafruit NeoPixel. `WiFi`, `WebServer`,
`DNSServer` and `Preferences` ship with the ESP32 Arduino core.

---

## Using it

1. Power the disc. It starts with the last saved settings.
2. On the phone, join the Wi-Fi network **`MoireDisc`**. The password is
   **`moiredisc`**.
3. Most phones open the control page on their own, as a "sign in to network"
   page. If yours doesn't, open a browser and go to **http://192.168.4.1**
   (any other http address redirects there too).
4. The phone may warn that the network has no internet. Choose to stay
   connected.

The status dot in the top right turns green and shows how many phones are
connected. **Reset to defaults** at the bottom of the page restores the
first-boot settings.

### HTTP API

The page uses a small API, which you can also call from a script:

| Request | Effect |
|---|---|
| `GET /api/state` | JSON of all settings plus `target`/`actual` motor speed, `clients`, `uptime` |
| `GET /api/set?key=value&…` | Changes settings (clamped to valid ranges) and returns the new state |
| `GET /api/reset` | Restores the defaults |

Keys: `motorMode` (0 manual, 1 sweep), `speed`, `sweepMin`, `sweepMax`
(−100…100), `sweepPeriod` (4…120 s), `effect` (0 gradient, 1 rainbow, 2 solid,
3 aurora, 4 off), `hue` (0…359), `spread` (0…100), `rotation` (−100…100),
`brightness` (0…100), `syncMotor` (0/1).

Example: `http://192.168.4.1/api/set?speed=-40&effect=3`

---

## Configuration

All tunable constants live in [include/config.h](include/config.h):

| Constant | Default | What it does |
|---|---|---|
| `AP_SSID`, `AP_PASSWORD`, `AP_CHANNEL` | `MoireDisc`, `moiredisc`, 1 | Wi-Fi network. Use `""` as the password for an open network |
| `MOTOR_PWM_HZ` | 20 kHz | PWM frequency. It is above hearing, so the motor doesn't whine |
| `MOTOR_MIN_DUTY_PCT` | 12 % | Duty the motor gets at 1 % speed. Raise it if the motor stalls at low speed |
| `MOTOR_RAMP_PCT_PER_S` | 50 %/s | How fast the speed may change. A full reversal takes 4 s |
| `MOTOR_DIR_INVERT` | `false` | Flip this if "positive" turns the disc the wrong way |
| `NUM_PIXELS` | 9 | LED count |
| `RING_ROT_MAX_LAPS_PER_S` | 0.5 | Ring rotation at a full rotation slider |
| `RING_SYNC_LAPS_PER_S` | 0.5 | Ring rotation at full motor speed with "Turn with the motor" on |
| `RAINBOW_HUE_PERIOD_S` | 30 s | How long the Rainbow effect's colours take to drift through a full cycle |

The first-boot defaults are in `DEFAULTS` in
[src/settings.cpp](src/settings.cpp). If you change the layout of the
`Settings` struct, bump `SETTINGS_VERSION` in the same file so an old saved
copy isn't read back as garbage.

### Source layout

```
include/config.h     pins, limits, Wi-Fi credentials
include/web_page.h   the phone page (HTML/CSS/JS in one string)
src/main.cpp         setup + 50 Hz main tick
src/motor.cpp        PWM/DIR output, ramp, sweep
src/leds.cpp         LED effects and crossfade
src/settings.cpp     settings struct, saving to flash
src/web.cpp          access point, captive-portal DNS, HTTP routes
```

---

## Differences from the stepper version

| | Stepper version | This version |
|---|---|---|
| Motors | 2 steppers, STEP/DIR through LEDC | 1 brushed DC motor, PWM/DIR (MAX14870) |
| Control | Custom ESP-NOW knob controller | Smartphone web page over the disc's own Wi-Fi AP |
| LEDs | 14 on D10 | 9 on D6 |
| Modes | SPEED / COLOR / AUTO, cycled by a button | Motor mode and light effect are chosen separately |
| Without a controller | Fixed standalone program | Keeps running the last saved settings |
| Boards | XIAO ESP32-C6 | XIAO ESP32-C3 or ESP32-C6 |

---

## Custom perfboard PCB layout

The electronics sit on a hand-wired perfboard, with the XIAO and the MAX14870
carrier plugged into female headers so either one can be swapped.

![Perfboard layout](docs/perfboard_layout.png)

<!-- TODO: save the layout picture as docs/perfboard_layout.png -->

### Components on the board

| Ref | Part | Purpose |
|---|---|---|
| U1 | Seeed XIAO ESP32-C3 / C6 (on headers) | Controller, Wi-Fi AP |
| U2 | Pololu MAX14870 carrier (on headers) | Motor H-bridge |
| J1 | Power input connector | Motor supply <!-- TODO: voltage --> |
| J2 | 2-pin screw terminal | Motor (MAX14870 M1 / M2) |
| J3 | 3-pin header | LED ring: 5 V, GND, DIN |
| R1 | ~330 Ω resistor | Series resistor in the LED data line, close to J3 |
| C1 | ~470–1000 µF electrolytic | Bulk capacitor across the LED 5 V / GND |
| <!-- TODO --> | <!-- e.g. 5 V buck converter --> | <!-- 5 V rail for the XIAO and LEDs --> |

### Connections

- **XIAO D1 → MAX14870 PWM**, **XIAO D2 → MAX14870 DIR**.
- **MAX14870 EN → GND** (unless the carrier already pulls it low).
- **XIAO D6 → R1 → J3 DIN.**
- **Motor supply → MAX14870 VIN / GND.** The motor current flows only through
  the MAX14870 and J2. Keep those traces short and thick, and away from the
  LED data line.
- **5 V rail → XIAO 5V pin and J3 5 V**, with C1 right at J3.
- **One common ground** for the supply, MAX14870, XIAO and LED ring.

### Notes

- Don't power the XIAO from USB and the 5 V rail at the same time unless your
  5 V source can take back-feed. <!-- TODO: note how this board handles it -->
- 9 LEDs at full white draw about 0.55 A from 5 V. The firmware's default
  brightness is 25 %.
- WS2812B LEDs at 5 V usually accept the XIAO's 3.3 V data signal. If the first
  LED flickers, add a level shifter (e.g. 74AHCT125) or run the first LED from
  a slightly lower voltage.
