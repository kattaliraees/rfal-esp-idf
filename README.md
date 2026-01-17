# RFAL for ESP-IDF

This is a port of the STMicroelectronics **RFAL (Radio Frequency Abstraction Layer)** to the **Espressif ESP-IDF** framework. It enables the use of ST25R series NFC readers (specifically **ST25R3911/B**) with ESP32 microcontrollers.

## Features

-   **Full RFAL Support**: Includes the complete ST RFAL library (v2.8.x).
-   **ESP-IDF Native Port**: 
    -   Uses **SPI Master** driver for high-speed communication.
    -   Uses **FreeRTOS** tasks and queues for non-blocking interrupt handling.
    -   Implements **Mutex-based** thread safety for concurrent access.
-   **High Performance**: Optimized SPI speed (5MHz) and interrupt handling.
-   **Easy to Use**: packaged as an IDF Component.

## Supported Hardware

-   **MCU**: ESP32, ESP32-S3, ESP32-C3, etc. (Any ESP-IDF supported chip with SPI).
-   **NFC Reader**: ST25R3911, ST25R3911B (X-NUCLEO-NFC05A1 or custom boards).

## Installation

### Using IDF Component Manager (Recommended)

Add the dependency to your `idf_component.yml`:

```yaml
dependencies:
  kattaliraees/rfal-esp-idf: "*"
```

### Manual

Clone this repository into your project's `components/` directory:

```bash
cd components
git clone https://github.com/kattaliraees/rfal-esp-idf.git rfal
```

## Wiring

Default configuration (can be changed in `platform.c` or via Kconfig in future updates):

| Pin Name | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **MOSI** | GPIO 36 | SPI MOSI |
| **MISO** | GPIO 37 | SPI MISO |
| **SCK** | GPIO 35 | SPI Clock |
| **CS/SS** | GPIO 0 | SPI Chip Select |
| **IRQ** | GPIO 38 | Interrupt Request (Rising Edge) |

*Note: Ensure your ESP32 pins are capable of 10MHz SPI if you increase the speed.*

## Example Usage

See the [examples/get_started](examples/get_started) directory for a complete example.

```c
#include "rfal_nfca.h"
#include "rfal_rf.h"

// Initialize
rfalInitialize();

// Poll for NFC-A tags
rfalNfcaListenDevice nfcaDevList[5];
uint8_t devCnt;
rfalNfcaPollerInitialize();
rfalNfcaPollerTechnologyDetection(RFAL_COMPLIANCE_MODE_NFC, &nfcaDevList, &devCnt);
```

## License

This project is licensed under the same terms as the original ST RFAL library (see bundled license files). Portions specifically for ESP-IDF implementation are provided under MIT/Apache-2.0.
