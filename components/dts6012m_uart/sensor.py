"""
DTS6012M UART Distance Sensor Custom Component for ESPHome

This component provides support for the DTS6012M ultrasonic distance sensor
via UART communication. It handles the sensor protocol and provides distance
measurements in meters.

Configuration example:

sensor:
  - platform: dts6012m_uart
    name: "Distance Sensor"
    id: distance_sensor
    update_interval: 60s

uart:
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 9600
  data_bits: 8
  parity: NONE
  stop_bits: 1

@version 1.0.0
@date 2025
@dlsnet

@license MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_DISTANCE,
    STATE_CLASS_MEASUREMENT,
    UNIT_METER,
    ICON_ARROW_EXPAND_VERTICAL,
)

# Component dependencies and auto-loading
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["uart"]

# Namespace declaration
dts6012m_uart_ns = cg.esphome_ns.namespace("dts6012m_uart")
DTS6012MUartSensor = dts6012m_uart_ns.class_(
    "DTS6012MUartSensor", 
    sensor.Sensor, 
    cg.PollingComponent, 
    uart.UARTDevice
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        DTS6012MUartSensor,
        unit_of_measurement=UNIT_METER,
        icon=ICON_ARROW_EXPAND_VERTICAL,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_DISTANCE,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

def validate_uart_config(config):
    """
    Validate UART configuration for DTS6012M sensor compatibility.
    
    The DTS6012M sensor requires specific UART settings:
    - Baud rate: 9600
    - Data bits: 8
    - Parity: None
    - Stop bits: 1
    
    Args:
        config: The configuration dictionary to validate
        
    Returns:
        Validated configuration
        
    Raises:
        cv.Invalid: If UART configuration is incompatible with the sensor
    """
    uart_config = config[uart.CONF_UART]
    
    # Validate baud rate (DTS6012M typically uses 9600)
    if uart_config[uart.CONF_BAUD_RATE] != 9600:
        raise cv.Invalid("DTS6012M sensor requires 9600 baud rate")
    
    # Validate data bits (should be 8)
    if uart_config.get(uart.CONF_DATA_BITS, 8) != 8:
        raise cv.Invalid("DTS6012M sensor requires 8 data bits")
    
    # Validate parity (should be none)
    if uart_config.get(uart.CONF_PARITY, "NONE") != "NONE":
        raise cv.Invalid("DTS6012M sensor requires no parity")
    
    # Validate stop bits (should be 1)
    if uart_config.get(uart.CONF_STOP_BITS, 1) != 1:
        raise cv.Invalid("DTS6012M sensor requires 1 stop bit")
    
    return config

# Apply configuration validation
FINAL_CONFIG_SCHEMA = cv.All(CONFIG_SCHEMA, validate_uart_config)

async def to_code(config):
    """
    Generate C++ code for the DTS6012M sensor component.
    
    This function is called by ESPHome to generate the C++ code
    for the sensor component based on the YAML configuration.
    
    Args:
        config: The configuration dictionary from YAML
    """
    # Create sensor variable
    var = cg.new_Pvariable(config[CONF_ID])
    
    # Register as a component
    await cg.register_component(var, config)
    
    # Register as a sensor
    await sensor.register_sensor(var, config)
    
    # Register UART device
    await uart.register_uart_device(var, config)