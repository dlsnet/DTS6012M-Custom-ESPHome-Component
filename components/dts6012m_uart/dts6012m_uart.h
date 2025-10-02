/**
 * @file dts6012m_uart.h
 * @brief ESPHome custom component for DTS6012M UART distance sensor
 * @version 1.0.0
 * @date 2025
 * @dlsnet
 * 
 * @license MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#ifndef DTS6012M_UART_H
#define DTS6012M_UART_H

#include "esphome.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace dts6012m_uart {

/**
 * @class DTS6012MUartSensor
 * @brief ESPHome component for DTS6012M UART distance sensor
 * 
 * This component interfaces with the DTS6012M ultrasonic distance sensor
 * over UART. It handles the sensor's communication protocol, data parsing,
 * and provides distance measurements in meters.
 * 
 * Features:
 * - Automatic start command transmission
 * - CRC validation for data integrity
 * - Change-based publishing to reduce unnecessary updates
 * - Robust buffer management and error recovery
 */
class DTS6012MUartSensor : public PollingComponent, public sensor::Sensor, public uart::UARTDevice {
 public:
  /// @brief Component setup - called once during initialization
  void setup() override;
  
  /// @brief Component update - called periodically based on polling interval
  void update() override;
  
  /// @brief Main loop - handles incoming UART data
  void loop() override;
  
  /// @brief Dump component configuration for debugging
  void dump_config() override;
  
  /// @brief Reset sensor state and clear buffers
  void reset_sensor();

 private:
  /// @brief Send start measurement command to sensor
  void send_start_command_();
  
  /// @brief Parse received data frame and extract distance
  /// @param data Pointer to frame data
  /// @param len Length of frame data
  /// @return true if frame was parsed successfully, false otherwise
  bool parse_data_frame_(const uint8_t *data, size_t len);
  
  /// @brief Calculate CRC16 for data validation
  /// @param data Data buffer to calculate CRC for
  /// @param length Length of data buffer
  /// @return Calculated CRC16 value
  uint16_t calculate_crc16_(const uint8_t *data, size_t length);
  
  // Member variables
  uint8_t buffer_[64];           ///< Circular buffer for incoming UART data
  size_t buffer_index_ = 0;      ///< Current buffer write position
  bool measurement_started_ = false;  ///< Track if measurement has been initiated
  uint32_t last_communication_time_ = 0;  ///< Timestamp of last communication (send/receive)
  float last_distance_ = -1;     ///< Last published distance value for change detection
};

}  // namespace dts6012m_uart
}  // namespace esphome

#endif  // DTS6012M_UART_H