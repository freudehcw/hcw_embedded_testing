# Theremin auf dem Raspberry Pi 5 — Embedded Unit Testing


```
mkdir build && cd build
cmake ..
make                      # baut App + Tests
ctest                     # bzw. ./test_theremin ./test_ultrasonic ./test_buzzer
make static_analysis      # cppcheck
make coverage             # Tests + Coverage-Report (schlägt fehl unter 95 %)
```

cppcheck = cppcheck --enable=all --std=c99 src/*.c

## Architektur — warum das testbar ist

Der Trick beim Testen von Hardware-Code ist eine **Hardware-Abstraktions-
schicht (HAL)** als Naht. Die Treiber sprechen nur die HAL an; im Test ersetzt
CMock die HAL durch generierte Mocks, sodass kein echter Pi nötig ist.

```
            getestet (Mocks)                  echte HW (nur Ziel-Build)
  ┌───────────────────────────────┐    ┌──────────────────────────────┐
  │ theremin.c   (reine Logik)     │    │ hal_gpio_pi.c  (/dev/gpiochip)│
  │ ultrasonic.c ─┐                │    │ hal_time_pi.c  (clock/nanosl.)│
  │ buzzer.c    ──┤                │    └──────────────┬───────────────┘
  └───────────────┤                │                   │
                  ▼  hal_gpio.h / hal_time.h  ◄─────────┘
                  (die Naht: hier mockt CMock)
```

| Modul           | getestet | Coverage | Beschreibung                              |
|-----------------|:--------:|:--------:|-------------------------------------------|
| `theremin.c`    |    ✓     |  100 %   | reine Logik: Abstand → Frequenz, Tonleiter|
| `ultrasonic.c`  |    ✓     |  100 %   | HC-SR04-Treiber (nutzt HAL → Mocks)       |
| `buzzer.c`      |    ✓     |  100 %   | KY-006-Treiber, Rechteckwelle (HAL → Mocks)|
| `hal_gpio_pi.c` |    —     |    —     | echtes GPIO (Linux gpiochip), nur Ziel    |
| `hal_time_pi.c` |    —     |    —     | echtes Timing, nur Ziel                   |
| `main.c`        |    —     |    —     | Komposition + Endlosschleife, nur Ziel    |

Die drei HW-/Glue-Dateien sind die untestbare Hardware-Grenze — genau das,
wofür die Mocks einspringen. Sie werden bewusst aus der Coverage gefiltert
(siehe `--filter` im `coverage`-Target).

## Tests & Mocks (Unity + CMock)

CMock liest die HAL-Header und erzeugt vor dem Test-Build automatisch
`mock_hal_gpio.*` und `mock_hal_time.*` (siehe `cmock.yml` und das
`generate_mocks`-Target in `CMakeLists.txt`). Damit lassen sich exakte
GPIO-/Zeit-Sequenzen vorgeben, z. B. ein kompletter HC-SR04-Ping:

```c
expect_trigger_pulse();                         /* 10 us Trigger      */
hal_time_now_us_ExpectAndReturn(1000u);         /* Startzeit          */
hal_gpio_read_ExpectAndReturn(ECHO, GPIO_HIGH); /* steigende Flanke   */
hal_time_now_us_ExpectAndReturn(1010u);         /* echo_start         */
hal_gpio_read_ExpectAndReturn(ECHO, GPIO_LOW);  /* fallende Flanke    */
hal_time_now_us_ExpectAndReturn(1510u);         /* echo_end → 500 us  */
TEST_ASSERT_EQUAL_INT(500, ultrasonic_measure_us(&s));
```

`setUp()` ruft `mock_*_Init()`, `tearDown()` `mock_*_Verify()` —
nicht erfüllte Erwartungen lassen den Test fehlschlagen.

## Statische Analyse (cppcheck)

```
cppcheck --enable=all --std=c99 src/*.c          # alle Quellen zusammen
# oder im Build:  make static_analysis
```

**Wichtig — alle Dateien gemeinsam prüfen, nicht einzeln.** Prüfst du nur *eine*
Datei (`cppcheck --enable=all --std=c99 ultrasonic.c`), meldet cppcheck *jede*
öffentliche Funktion als `unusedFunction`, weil der Aufrufer in einer anderen
Datei steht — cppcheck sieht ihn bei Einzeldatei-Analyse schlicht nicht. Das ist
eine Eigenheit der Single-File-Analyse, kein echter Fehler. Über das ganze
`src/`-Verzeichnis (bzw. `src/*.c`) verschwinden diese Meldungen.

Die internen Rechen-Helfer (Echo-µs↔mm, Halbperiode, Zyklenzahl) sind bewusst
`static` und werden über die öffentlichen Funktionen mitgetestet — dadurch gibt
es auch keine `staticFunction`-Hinweise.

Übrig bleiben nur `information`-Zeilen, dass cppcheck Standard-Header wie
`<stdint.h>` nicht findet ("Include file ... not found"). cppcheck sagt selbst,
dass diese für ein korrektes Ergebnis nicht nötig sind; sie sind keine Warnungen
über deinen Code, sondern genau die "fehlenden libs". Zusätzlich gibt cppcheck am
Ende eine Eigeninfo `nofile:0:0: ... Active checkers: X/Y [checkersReport]` aus --
das ist cppchecks Selbstauskunft (wie viele Prüfer liefen) und betrifft den Code
nicht; sie erscheint in jedem `--enable=all`-Lauf.

Das `static_analysis`-Target blendet die Infozeilen aus und schlägt bei jeder
echten style/warning/error-Meldung fehl.

## Coverage

`make coverage` führt alle Tests aus und erzeugt mit `gcovr` einen Bericht
unter `build/coverage/coverage.html`. Das Target erzwingt **≥ 95 %**
(`--fail-under-line 95`); aktuell sind alle drei Module bei 100 %.

## Cross-Compile auf den Raspberry Pi 5

Der echte GPIO-Zugriff nutzt die Linux-`gpiochip`-Zeichengeräte-Schnittstelle
(`<linux/gpio.h>`) — **keine externe Bibliothek** (kein libgpiod) nötig. Damit
genügt der Standard-Cross-Compiler:

```
sudo apt install gcc-aarch64-linux-gnu

cmake -S . -B build-pi -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64.cmake
cmake --build build-pi --target theremin

scp build-pi/theremin pi@raspberrypi:~
ssh pi@raspberrypi 'sudo ./theremin'      # GPIO braucht Rechte
```

> **gpiochip-Name:** Auf dem Pi 5 heißt der Header-Chip je nach Kernel
> `gpiochip0` oder `gpiochip4`. Prüfe mit `gpiodetect`. Standard ist
> `/dev/gpiochip0`; abweichend per
> `cmake ... -DCMAKE_C_FLAGS='-DHAL_GPIOCHIP=\"/dev/gpiochip4\"'`.

## Verkabelung (Pi 5, 40-Pin-Header)

```
  SR04  VCC  -> Pin 1  (3.3V)
  SR04  GND  -> Pin 6  (GND)
  SR04  Trig -> Pin 11 (GPIO 17)
  SR04  Echo -> Pin 13 (GPIO 27)   
  KY-006 VCC    -> Pin 2  (5V)
  KY-006 GND    -> Pin 9  (GND)
  KY-006 Signal -> Pin 15 (GPIO 22)
```



## Projektstruktur

```
theremin_pi/
├── CMakeLists.txt            Build, Mocks, Tests, coverage, static_analysis
├── cmock.yml                 CMock-Konfiguration
├── cmake/
│   └── toolchain-aarch64.cmake   Cross-Compile-Toolchain für Pi 5
├── include/                  HAL-Header (Mock-Naht) + Treiber-Header
├── src/                      Treiber (getestet) + echte HAL + main (Ziel)
├── test/                     Unity-Tests (test_theremin/ultrasonic/buzzer)
└── vendor/
    ├── unity/                Unity 2.6 + auto/ (Ruby-Helfer)
    └── cmock/                CMock (Ruby-Generator + C-Runtime)
```

## Voraussetzungen (Host)

```
sudo apt install cmake gcc ruby cppcheck gcovr gcc-aarch64-linux-gnu
```

Unity und CMock sind im Repo (`vendor/`) enthalten — nichts weiter nötig.
