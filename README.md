# Custom ESPHome DTS6012M UART Distance Sensor Component
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A custom ESPHome component for the DTS6012M ultrasonic distance sensor using UART communication. This component provides accurate distance measurements with robust error handling and efficient data processing.

## Sensor Specifications

- **Measurement Range**: 20mm - 6000mm
- **Accuracy**: ±1% of reading
- **Resolution**: 1mm
- **Output**: UART serial communication
- **Baud Rate**: 9600 bps
- **Voltage**: 3.3V-5V DC

## Installation

### Method 1: Manual Installation

1. Create a `components` directory in your ESPHome configuration folder
2. Copy these files to `components/dts6012m_uart/`:
   - `dts6012m_uart.h`
   - `dts6012m_uart.cpp` 
   - `sensor.py`
   - '__init__.py'

### Method 2: External Components (Recommended)

Add to your ESPHome YAML:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/dlsnet/DTS6012M-Custom-ESPHome-Component
    refresh: 1d
```

## Configuration

### Basic Example

```yaml
# Example configuration for DTS6012M sensor
uart:
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 9600
  data_bits: 8
  parity: NONE
  stop_bits: 1

sensor:
  - platform: dts6012m_uart
    name: "Distance Sensor"
    id: distance_sensor
    update_interval: 60s
    filters:
      - delta: 0.01  # Only trigger on changes > 1cm
      - throttle: 1s  # Limit update rate
```

## Wiring Diagram

### ESP32/ESP8266 Connection

```
DTS6012M Sensor    ESP32/ESP8266
┌─────────────┐    ┌─────────────┐
│             │    │             │
│ VCC, Laser  ├───►│   3.3V      │
│             │    │             │
│ GND, PIN 5  ├───►│   GND       │
│             │    │             │
│   TX        ├───►│   RX Pin    │
│             │    │             │
│   RX        ├───►│   TX Pin    │
│             │    │             │
└─────────────┘    └─────────────┘
```

**Pin Connections:**
- **VCC, Laser**: 3.3V 
- **GND, PIN 5**: Ground
- **TX** → **ESP RX**: Sensor transmit to ESP receive
- **RX** → **ESP TX**: Sensor receive to ESP transmit



## Troubleshooting

### Common Issues

1. **No Data Received**
   - Check wiring (TX/RX swapped?)
   - Verify baud rate is 9600
   - Ensure sensor is powered properly

2. **CRC Errors**
   - Check for electrical noise
   - Verify stable power supply
   - Ensure proper grounding

3. **Inconsistent Readings**
   - Avoid mounting near air vents or moving objects
   - Ensure clean, solid surface for mounting
   - Check for obstacles in sensor field



## License

This project is licensed under the MIT License - see the LICENSE file for details.


## Changelog

### v1.0.0
- Initial release
- Basic UART communication
- CRC validation
- Change-based publishing
- Error recovery mechanisms

---

**Disclaimer**: This component is not officially affiliated with the sensor manufacturer. Use at your own risk and always verify sensor readings in critical applications.
