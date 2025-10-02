/**
 * @file dts6012m_uart.cpp
 * @brief ESPHome custom component implementation for DTS6012M UART distance sensor
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

#include "dts6012m_uart.h"
#include <cstring>

namespace esphome {
namespace dts6012m_uart {

static const char *const TAG = "dts6012m_uart";

// Frame structure constants
constexpr uint8_t FRAME_HEADER[] = {0xA5, 0x03, 0x20, 0x01};
constexpr size_t MIN_FRAME_LENGTH = 7;
constexpr size_t HEADER_LENGTH = 4;
constexpr size_t DATA_LENGTH_POS = 5;
constexpr size_t DISTANCE_DATA_POS = 13;
constexpr size_t CRC_LENGTH = 2;

// Communication timing constants
constexpr uint32_t COMMUNICATION_TIMEOUT_MS = 10000;  // 10 seconds
constexpr size_t MAX_BYTES_PER_LOOP = 32;             // Prevent loop blocking
constexpr float DISTANCE_CHANGE_THRESHOLD = 0.01f;    // 10mm change threshold

void DTS6012MUartSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up DTS6012M UART Sensor");
  reset_sensor();
  delay(1000);  // Allow sensor to stabilize
  send_start_command_();
  measurement_started_ = true;
  last_communication_time_ = millis();
}

void DTS6012MUartSensor::update() {
  uint32_t now = millis();
  
  if (!measurement_started_) {
    // Initial start command if not yet started
    ESP_LOGD(TAG, "Sending initial start command");
    send_start_command_();
    measurement_started_ = true;
    last_communication_time_ = now;
  } else if (now - last_communication_time_ > COMMUNICATION_TIMEOUT_MS) {
    // Resend start command if no communication for timeout period
    ESP_LOGW(TAG, "No communication for %d ms, resending start command", COMMUNICATION_TIMEOUT_MS);
    send_start_command_();
    last_communication_time_ = now;
  }
}

void DTS6012MUartSensor::loop() {
  bool data_received = false;
  
  // Process incoming UART data with limit to prevent blocking
  for (size_t i = 0; i < MAX_BYTES_PER_LOOP && this->available(); i++) {
    uint8_t byte;
    if (!this->read_byte(&byte)) {
      break;
    }
    
    data_received = true;

    // Add byte to buffer if space available
    if (buffer_index_ < sizeof(buffer_)) {
      buffer_[buffer_index_++] = byte;
    } else {
      ESP_LOGE(TAG, "Buffer overflow, resetting buffer");
      buffer_index_ = 0;
      continue;
    }

    // Check if we have at least the minimum frame header
    if (buffer_index_ >= MIN_FRAME_LENGTH) {
      // Look for frame header pattern
      if (buffer_[0] == FRAME_HEADER[0] && 
          buffer_[1] == FRAME_HEADER[1] && 
          buffer_[2] == FRAME_HEADER[2] && 
          buffer_[3] == FRAME_HEADER[3]) {
        
        // Extract data length from bytes 5-6 (big-endian)
        uint16_t data_length = (buffer_[DATA_LENGTH_POS] << 8) | buffer_[DATA_LENGTH_POS + 1];
        
        // Validate data length to prevent buffer overflows
        if (data_length > 32) {
          ESP_LOGW(TAG, "Invalid large data length: %d, discarding frame", data_length);
          // Remove only the first byte and continue processing
          memmove(buffer_, buffer_ + 1, --buffer_index_);
          continue;
        }
        
        // Calculate total frame length: header(7) + data + CRC(2)
        size_t total_frame_length = 7 + data_length + CRC_LENGTH;
        
        // Check if frame fits in our buffer
        if (total_frame_length > sizeof(buffer_)) {
          ESP_LOGE(TAG, "Frame too large: %d bytes, resetting buffer", total_frame_length);
          buffer_index_ = 0;
          continue;
        }
        
        // Check if we have complete frame
        if (buffer_index_ >= total_frame_length) {
          ESP_LOGD(TAG, "Complete frame received, length: %d", total_frame_length);
          
          if (parse_data_frame_(buffer_, total_frame_length)) {
            // Frame parsed successfully, update communication timestamp
            last_communication_time_ = millis();
            
            // Remove processed frame from buffer
            if (buffer_index_ > total_frame_length) {
              // More data in buffer, shift remaining data to front
              memmove(buffer_, buffer_ + total_frame_length, buffer_index_ - total_frame_length);
              buffer_index_ -= total_frame_length;
            } else {
              // No more data in buffer, reset index
              buffer_index_ = 0;
            }
          } else {
            // Frame parsing failed, discard just the first byte and continue
            memmove(buffer_, buffer_ + 1, --buffer_index_);
          }
        }
      } else {
        // Header not found at current position, shift buffer by one byte
        memmove(buffer_, buffer_ + 1, --buffer_index_);
      }
    }
  }
  
  // Update communication timestamp if we received any data in this loop
  if (data_received) {
    last_communication_time_ = millis();
  }
}

void DTS6012MUartSensor::send_start_command_() {
  // Start measurement command for DTS6012M sensor
  uint8_t command[] = {0xA5, 0x03, 0x20, 0x01, 0x00, 0x00, 0x00, 0x02, 0x6E};
  
  // Clear any pending data from UART buffer
  while (this->available()) {
    uint8_t dummy;
    this->read_byte(&dummy);
  }
  
  // Send command and wait for transmission to complete
  this->write_array(command, sizeof(command));
  this->flush();
  
  ESP_LOGI(TAG, "Start command sent");
  
  // Log command in hex format for debugging
  if (esp_log_level_get(TAG) >= ESP_LOG_DEBUG) {
    std::string hex_str;
    char hex[4];
    for (size_t i = 0; i < sizeof(command); i++) {
      sprintf(hex, "%02X ", command[i]);
      hex_str += hex;
    }
    ESP_LOGD(TAG, "Command bytes: %s", hex_str.c_str());
  }
}

void DTS6012MUartSensor::reset_sensor() {
  buffer_index_ = 0;
  last_distance_ = -1;
  measurement_started_ = false;
  last_communication_time_ = 0;
  
  // Clear any pending UART data
  while (this->available()) {
    uint8_t dummy;
    this->read_byte(&dummy);
  }
  
  ESP_LOGD(TAG, "Sensor reset complete");
}

uint16_t DTS6012MUartSensor::calculate_crc16_(const uint8_t *data, size_t length) {
  // Modbus CRC-16 calculation
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc = crc >> 1;
      }
    }
  }
  return crc;
}

bool DTS6012MUartSensor::parse_data_frame_(const uint8_t *data, size_t len) {
  // Validate minimum frame length
  if (len < 9) {
    ESP_LOGE(TAG, "Frame too short: %d bytes", len);
    return false;
  }
  
  // Verify CRC (excluding the CRC bytes themselves)
  uint16_t calculated_crc = calculate_crc16_(data, len - CRC_LENGTH);
  uint16_t received_crc = (data[len - 2] << 8) | data[len - 1];
  
  if (calculated_crc != received_crc) {
    ESP_LOGE(TAG, "CRC mismatch: calculated 0x%04X, received 0x%04X", calculated_crc, received_crc);
    return false;
  }
  
  // Extract data length from frame
  uint16_t data_length = (data[DATA_LENGTH_POS] << 8) | data[DATA_LENGTH_POS + 1];
  
  // Validate we have enough data for distance measurement
  if (data_length < 14) {
    ESP_LOGW(TAG, "Short data length: %d bytes, skipping", data_length);
    return true;  // Valid frame but insufficient data
  }
  
  // Extract distance (bytes 13-14, little-endian format)
  uint16_t distance_mm = (data[DISTANCE_DATA_POS + 1] << 8) | data[DISTANCE_DATA_POS];
  float distance_m = distance_mm / 1000.0f;
  
  // Handle no target detected case (0xFFFF)
  if (distance_mm == 0xFFFF) {
    if (last_distance_ != NAN) {
      ESP_LOGI(TAG, "No valid target detected");
      this->publish_state(NAN);
      last_distance_ = NAN;
    }
    return true;
  }
  
  // Check if this is a significant change from last reading
  if (last_distance_ < 0 || fabs(distance_m - last_distance_) >= DISTANCE_CHANGE_THRESHOLD) {
    ESP_LOGI(TAG, "Distance: %d mm (%.3f m)", distance_mm, distance_m);
    this->publish_state(distance_m);
    last_distance_ = distance_m;
  } else {
    ESP_LOGD(TAG, "Distance: %d mm (%.3f m) - no significant change", distance_mm, distance_m);
  }
  
  return true;
}

void DTS6012MUartSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "DTS6012M UART Sensor:");
  LOG_SENSOR("  ", "Distance", this);
  ESP_LOGCONFIG(TAG, "  Buffer size: %d bytes", sizeof(buffer_));
  ESP_LOGCONFIG(TAG, "  Measurement started: %s", measurement_started_ ? "Yes" : "No");
  ESP_LOGCONFIG(TAG, "  Communication timeout: %d ms", COMMUNICATION_TIMEOUT_MS);
  ESP_LOGCONFIG(TAG, "  Distance threshold: %.3f m", DISTANCE_CHANGE_THRESHOLD);
}

}  // namespace dts6012m_uart
}  // namespace esphome