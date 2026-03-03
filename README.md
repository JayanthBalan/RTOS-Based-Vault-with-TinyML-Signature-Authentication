# SignVault — Biometric Signature Password Vault

A hardware-based password vault secured by handwritten signature recognition. Instead of a PIN or master password, you sign your name on a touchscreen — the system verifies it using a TinyML pipeline (DTW + KNN) running on an ESP32, and only unlocks the vault if the signature matches. Passwords are XOR-encrypted and stored in onboard flash.

---

## How It Works

The system is split across two ESP32 microcontrollers communicating over I2C:

**Master ESP32** handles everything user-facing:
- Drives a 2.4" ILI9341 TFT touchscreen (240×320, parallel interface)
- Presents the UI: start screen → access screen → signature capture
- Resamples the raw touch data to a fixed 150-point sequence
- Transmits the processed signature to the slave over I2C
- Waits for a classification result and updates the display accordingly
- Manages the password vault (SPIFFS storage, XOR encryption)

**Slave ESP32** handles the ML backend:
- Receives signature data chunks over I2C (address 0x08)
- Computes Dynamic Time Warping (DTW) distances between the new signature and every stored reference
- Extracts a 5-feature statistical vector from those distances (min, max, mean, std dev, median)
- Classifies using K-Nearest Neighbors (K=3) via the Arduino_KNN library
- Responds `y1` (genuine) or `n1` (forgery) back to master
- Runs FreeRTOS with 7 concurrent tasks across both cores

```
[User signs on TFT] → [Master resamples → sends over I2C] → [Slave: DTW → KNN → classify]
                                                                        ↓
[Vault unlocked / denied] ← [Master receives y1 / n1] ←────────────────┘
```

---

## Hardware

| Component | Interface | Notes |
|---|---|---|
| ESP32 (Master) | — | NodeMCU-32S |
| ESP32 (Slave) | — | NodeMCU-32S |
| ILI9341 TFT (2.4") | Parallel (8-bit) | MCUFRIEND shield-style |
| Resistive touchscreen | Analog | XP=27, XM=15, YP=4, YM=14 |
| MicroSD module | SPI (CS=5) | Stores signature training data |
| I2C bus | SDA/SCL | Master→Slave, address 0x08 |

**Touch calibration (portrait):**
```
x = map(p.x, LEFT=116, RT=936, 0, 240)
y = map(p.y, TOP=964, BOT=139, 0, 320)
```

---

## Project Structure

```
SignVault/
├── Master/
│   ├── main.cpp           # Setup, loop, signature resampling
│   ├── lcd_sigman.cpp/h   # TFT UI screens and touch signature reader
│   ├── comm_ops.cpp/h     # I2C master: tx_comm(), rx_comm()
│   ├── vault_ops.cpp/h    # SPIFFS storage, XOR encrypt/decrypt
│   ├── type_dat.h         # sigpoints struct definition
│   └── platformio.ini     # Adafruit GFX, TouchScreen, MCUFRIEND_kbv
│
└── Slave/
    ├── main.cpp           # FreeRTOS task setup, serial command handling
    ├── comm_ops.cpp/h     # I2C slave: interrupt-driven rx, tx
    ├── knn_dtw_ops.cpp/h  # DTW distance, feature extraction, KNN classify
    ├── sd_ops.cpp/h       # SD read/write, training tasks, dataset trim
    ├── type_dat.h         # Shared sigpoints struct
    └── platformio.ini     # Arduino_KNN, -O3 optimizations
```

---

## ML Pipeline Detail

### Signature Capture
The touchscreen samples X, Y, and Z (pressure) coordinates during signing. Raw point counts vary per signature, so the master resamples every capture to exactly 150 points using linear interpolation before transmission.

### DTW Distance
On the slave, each incoming signature is compared against every stored reference signature using DTW. DTW handles natural timing variations in handwriting — someone might sign faster or slower each time, and DTW warps the time axis to find the best alignment before measuring distance. The implementation uses a memory-efficient two-row rolling matrix.

Normalized DTW:
```
distance = raw_dtw / (n + m)
```

### Feature Extraction
The vector of DTW distances against all stored references gets compressed into 5 scalar features:
- `features[0]` — minimum distance (closest match)
- `features[1]` — maximum distance
- `features[2]` — mean
- `features[3]` — standard deviation
- `features[4]` — median

### KNN Classification
These 5 features are passed to a KNN classifier (K=3, confidence threshold=0.5). Training patterns come from both genuine (`signat1.bin`) and forgery (`signat0.bin`) classes. The first 4 signatures ever entered are auto-accepted to bootstrap the training dataset.

---

## FreeRTOS Task Layout (Slave)

| Task | Core | Priority | Stack | Role |
|---|---|---|---|---|
| `wait_task` | 1 | 1 | 4000 | Holds mutex until full signature received |
| `read_dat` | 1 | 2 | 12000 | Reads signature data from SD card |
| `vec_process` | 0 | 2 | 15000 | Orchestrates training + classification |
| `train_proc_1` | 0 | 3 | 12000 | Computes DTW patterns for genuine class |
| `train_proc_2` | 1 | 3 | 12000 | Computes DTW patterns for forgery class |
| `class_task1` | 0 | 3 | 8000 | DTW computation for classification (half) |
| `class_task2` | 1 | 3 | 8000 | DTW computation for classification (half) |

Three FreeRTOS queues pass data between tasks (`sig_transfer_Q`, `sig_transfer_Q1`, `sig_transfer_Q2`). A mutex (`sign_lock`) and counting semaphores (`train_fin`, `dtw_fin`) coordinate training completion and classification readiness.

---

## Password Storage

Passwords are stored in SPIFFS (`/Password_Archive.bin`) on the master ESP32. Up to 50 entries are supported (`MAX_STORAGE = 50`). Before writing, all entries are XOR-encrypted with a fixed key. Decryption requires two additional key characters (`key1`, `key2`) provided at runtime, making the stored blob useless without both the physical device and the key components.

> **Note:** XOR encryption provides obfuscation rather than strong cryptographic security. It protects against casual file inspection but is not suitable against a dedicated attacker with physical access to flash.

---

## Serial Commands (Slave)

After boot, the slave waits for a single character over UART (115200 baud) before starting task execution. This is used during the enrollment phase:

| Command | Action |
|---|---|
| `p` | Label the last received signature as **genuine** and append to `signat1.bin` |
| `f` | Label the last received signature as **forgery** and append to `signat0.bin` |
| `t` | Run a **performance test** (accuracy, precision, recall, FAR, FRR, F1) on stored data |
| `x` | **Clear** all stored signature data and restart |

---

## Build & Flash

Both ESP32s use PlatformIO. Flash each from its respective directory.

**Master** (`platformio.ini`):
```ini
[env:nodemcu-32s]
platform = espressif32
board = nodemcu-32s
framework = arduino
lib_deps =
    adafruit/Adafruit GFX Library@^1.12.3
    adafruit/Adafruit TouchScreen@^1.1.6
    prenticedavid/MCUFRIEND_kbv@^3.1.0-Beta
```

**Slave** (`platformio.ini`):
```ini
[env:nodemcu-32s]
platform = espressif32
board = nodemcu-32s
framework = arduino
monitor_speed = 115200
build_flags = -O3 -ffast-math -funroll-loops
lib_deps = arduino-libraries/Arduino_KNN@^0.1.0
```

The slave uses `-O3 -ffast-math -funroll-loops` to squeeze performance out of the DTW computation, which runs against a growing dataset of stored signatures on every classification.

---

## Enrollment Flow

1. Flash both ESP32s
2. Power up the system — slave waits for serial input
3. Have the owner sign on the TFT several times
4. After each transmission, send `p` over serial to the slave to label it genuine
5. Optionally enroll forgery examples by having someone else sign and sending `f`
6. Send `t` to run a performance check and verify classifier metrics
7. The system is now operational — subsequent signature attempts are classified automatically

The dataset is kept lean by `dataset_trim()`, which removes the oldest entry whenever the per-class count exceeds `MAX_CNT = 3`. This keeps DTW computation time bounded as usage grows.

---

## Performance Metrics

The `performance_check()` function does a basic train/test split on stored data (even indices → train, odd → test) and prints:

- Accuracy, Precision, Recall, F1 Score
- **FAR** (False Accept Rate) — how often a forgery is accepted
- **FRR** (False Reject Rate) — how often a genuine signature is rejected

These are reported over UART after sending the `t` command.

---

## Known Limitations

- I2C at standard speed with 150 points × 8 bytes/point = 1200 bytes. Chunk retries are handled but large transmission windows add latency.
- XOR encryption is not cryptographically strong.
- KNN classification quality depends heavily on enrollment data diversity. More genuine/forgery samples improve accuracy.
- The auto-accept policy for the first 4 signatures means initial enrollment has no verification — physical security matters during setup.

---

## Dependencies

| Library | Target | Version |
|---|---|---|
| Adafruit GFX | Master | ^1.12.3 |
| Adafruit TouchScreen | Master | ^1.1.6 |
| MCUFRIEND_kbv | Master | ^3.1.0-Beta |
| Arduino_KNN | Slave | ^0.1.0 |
| FreeRTOS | Slave | Built into ESP-IDF |
| SPIFFS | Master | Built into Arduino ESP32 |
| SD | Slave | Built into Arduino ESP32 |
