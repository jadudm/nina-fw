/*
  This file is an addition to the Arduino NINA firmware.
  Copyright (c) 2019 Matthew Jadud <matt@jadud.com>. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <string.h>
#include "wiring_digital.h"
#include "WInterrupts.h"
// Includes drivers for I2C operation on the ESP32.
#include "I2CS.h"

SemaphoreHandle_t print_mux = NULL;

I2CSClass::I2CSClass(int i2cPortNum, int sclPin, int sdaPin, int i2cAddr) :
  _i2cPortNum((i2c_port_t)i2cPortNum),
  _sclPin((gpio_num_t)sclPin),
  _sdaPin((gpio_num_t)sdaPin),
  _i2cAddr(i2cAddr)
{
  // Empty class. Nifty C++ thing.
}

int I2CSClass::begin()
{ 
  i2c_config_t conf;
  // NOTE MCJ 20190627
  // C++ type conversion around enums means I have to be more specific/careful
  // with declarations of GPIO pins. Even though these are "just" integers,
  // C++ is trying to make sure things come through correctly.
  // https://esp32.com/viewtopic.php?t=1591
  conf.sda_io_num = _sdaPin;
  conf.scl_io_num = _sclPin;
  conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
  conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
  conf.mode = I2C_MODE_SLAVE;
  conf.slave.addr_10bit_en = 0;
  conf.slave.slave_addr = _i2cAddr;

  // FIXME MCJ 20190627
  // Check what these macros do, and decide if I'd rather 
  // do something else for error handling.
  ESP_ERROR_CHECK(i2c_param_config(_i2cPortNum, &conf));
  ESP_ERROR_CHECK(i2c_driver_install(
      _i2cPortNum,
      conf.mode,
      _i2c_rx_buff_len,
      _i2c_tx_buff_len,
      0
  ));

  // WARNING MCJ 20190627
  // The SPIS class returns 1. In the UNIX world, this is an error.
  // I'm assuming that 1 is being returned to be a notional TRUE
  // value. However, that is not the convention in the C/UNIX 
  // land of antiquity. For now, I'll follow the convention of SPIS.c/h,
  // but it makes me uncomfortable.
  return 1;
}

int I2CSClass::transfer(uint8_t out[], uint8_t in[], size_t len)
{
  int result = -1;

  // If 'in' is NULL, then we're doing an output.
  if (in == NULL){
    // Send the bytes.
    // ESP-IDF: http://bit.ly/2xiJvpK
    result = i2c_slave_write_buffer(_i2cPortNum, out, len, portMAX_DELAY);
  } else if (out == NULL) {
    // Or, we're doing an input.
    result = i2c_slave_read_buffer(_i2cPortNum, in, len, portMAX_DELAY);
    
    if (_debug) {
      xSemaphoreTake(print_mux, portMAX_DELAY);
      printf("I2C IN RESULT: %d\n\t", result);
      for (int ndx = 0; ndx < len ; ndx++) {
        printf("%02x", in[ndx]);
      }
      printf("\n");
      xSemaphoreGive(print_mux);
    }
  }

  return 1;
}

// FIXME WARNING MCJ 20190627
// Need to choose a default I2C address.
// Need to decide if there will be a jumper or similar to allow
// choice of I2C address. If so, then this becomes dynamic
// at class init time based on a GPIO read of high/low. 

// I2CSClass(int i2cNum, int sclPin, int sdaPin, int i2cAddr);
I2CSClass I2CS(I2C_NUM_0, GPIO_NUM_19, GPIO_NUM_18, 0x2A);

