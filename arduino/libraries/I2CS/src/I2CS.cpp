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
#include "driver/i2c.h"

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
  if (_debug) ets_printf("I2C In the begin()ning.\n");

  int check = i2c_param_config(_i2cPortNum, &conf);
  if (_debug) ets_printf("i2c_param_config == %d\n", check);

  if (check == ESP_OK && _debug) {
    ets_printf("I2C param config checks out.\n");
  } else if (check == ESP_ERR_INVALID_ARG && _debug) {
    ets_printf("I2C param config FAIL.\n");
  }

  check = i2c_driver_install(
      _i2cPortNum,
      conf.mode,
      _i2c_rx_buff_len,
      _i2c_tx_buff_len,
      0
  );

  if (check == ESP_OK && _debug) {
    ets_printf("I2C driver installed OK!\n");
  } else if (check == ESP_ERR_INVALID_ARG && _debug) {
    ets_printf("I2C driver install param error.\n");
  } else if (check == ESP_FAIL && _debug) {
    ets_printf("I2C driver install fail.\n");
  }
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
  uint8_t byte = 0;

  i2c_reset_rx_fifo(_i2cPortNum);
  i2c_reset_tx_fifo(_i2cPortNum);

  // If 'in' is NULL, then we're doing an output.
  if (in == NULL) {
    if (_debug) ets_printf("OUTPUT I2C BUFFER len[%d]\n", len);  
    // Send the bytes.
    // ESP-IDF: http://bit.ly/2xiJvpK
    // FIXME MCJ 20190627
    // How long until we time out?
    // How about one tick, which is... 10ms?
    // https://esp32.com/viewtopic.php?f=2&t=2377
    result = i2c_slave_write_buffer(_i2cPortNum, out, len, 50);
  } else if (out == NULL) {
    // if (_debug) ets_printf("INPUT I2C BUFFER len[%d]\n", len);
    // Read the length
    // 1 is 10ms
    // 10 is 100ms
    // 100 is 1000ms
    result = i2c_slave_read_buffer(_i2cPortNum, &byte, 1, portMAX_DELAY);
    // ets_printf("\n%d\n", byte);
    // Read the bytes
    result = i2c_slave_read_buffer(_i2cPortNum, in, byte, portMAX_DELAY);
    
    if (_debug) {
      ets_printf("I2C IN RESULT: %d\n\t", result);
      for (int ndx = 0; ndx < len ; ndx++) {
        ets_printf("%02x ", in[ndx]);
      }
      ets_printf("\n\n");
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
// SCL on the Huzzah32 is GPIO22
// SDA on the Huzzah32 is GPIO23
I2CSClass I2CS(I2C_NUM_0, GPIO_NUM_22, GPIO_NUM_23, 0x2A);

