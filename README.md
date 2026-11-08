# No-AD-WIFI

A lightweight ESP32-based networking experiment designed to interfere with the loading of advertisement services by introducing network latency.

## Hardware Requirements

- ESP32 development board
- USB cable with data-transfer support

## Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software/)
- ESP32 board package for Arduino IDE
- An active Wi-Fi network

## How It Works

No-AD-WIFI uses an ESP32 to interact with a configured Wi-Fi network and experiment with network latency.

The project aims to introduce significant delays to network communication, potentially preventing certain advertisement services from loading successfully or causing their requests to time out.

**Note:** Actual results depend on network conditions, device configuration, and how individual advertisement services handle connection delays. The project does not guarantee universal ad blocking.

## Installation & Setup

Follow these steps to get No-AD-WIFI running on your ESP32.

### Step 1 — Install Arduino IDE

Download and install the official Arduino IDE:

[Download Arduino IDE](https://www.arduino.cc/en/software/)

### Step 2 — Install the ESP32 Board Package

Configure Arduino IDE to support ESP32 development by installing the appropriate ESP32 board package.

### Step 3 — Connect Your ESP32

Connect your ESP32 development board to your computer using a Micro-USB or USB cable that supports data transfer.

### Step 4 — Load the Source Code

Open the `slow.ino` file from this repository in Arduino IDE.

### Step 5 — Configure Wi-Fi Credentials

Update the Wi-Fi configuration in the source code with your network credentials:

- **SSID:** Your Wi-Fi network name.
- **Password:** Your Wi-Fi network password.

Keep your credentials private and avoid publishing them in public repositories.

### Step 6 — Upload the Sketch

Select the appropriate ESP32 board and serial port in Arduino IDE, then compile and upload the sketch to your device.

Once uploaded, the ESP32 will execute the configured program.

## Repository Structure

The repository is intentionally kept minimal to make the project easy to understand, navigate, and modify.

```text
No-AD-WIFI/
│
├── LICENSE
│
├── README.md
│
└── slow.ino
```

### File Breakdown

| File | Description |
|---|---|
| `LICENSE` | MIT License governing the use, modification, and distribution of the software. |
| `README.md` | Project documentation, including requirements, installation, configuration, and usage instructions. |
| `slow.ino` | Main Arduino sketch containing the ESP32 implementation. |

## License

This project is licensed under the **MIT License**.

You are free to fork, modify, distribute, and rebuild the project in accordance with the license terms.

See [`LICENSE`](LICENSE) for the complete license text.

---

**No-AD-WIFI** — Simple by design. Open by nature.

**Software developed and maintained by: [scriptsnsenses-sys](https://github.com/scriptsnsenses-sys)**
