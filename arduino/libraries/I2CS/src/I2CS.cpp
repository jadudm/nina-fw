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

I2CSClass::I2CSClass(int i2cPortNum, int sclPin, int sdaPin, int i2cAddr, int readyPin) :
  _i2cPortNum((i2c_port_t)i2cPortNum),
  _sclPin((gpio_num_t)sclPin),
  _sdaPin((gpio_num_t)sdaPin),
  _i2cAddr(i2cAddr),
  _readyPin(readyPin)
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

  // Set up the ready pin.
  pinMode(_readyPin, OUTPUT);
  set_ready_state(true);
  // _readySemaphore = xSemaphoreCreateCounting(1, 0);


  // WARNING MCJ 20190627
  // The SPIS class returns 1. In the UNIX world, this is an error.
  // I'm assuming that 1 is being returned to be a notional TRUE
  // value. However, that is not the convention in the C/UNIX 
  // land of antiquity. For now, I'll follow the convention of SPIS.c/h,
  // but it makes me uncomfortable.
  return 1;
}

// State machine for reading a request packet from the 
// Adafruit SPI/I2C library.
int I2CSClass::read_packet(uint8_t buffer[]) {
  int ptr = 0;

  // Read in the start command, 0x0E
  i2c_slave_read_buffer(_i2cPortNum, &buffer[ptr], 1, portMAX_DELAY);
  ptr += 1;

  // Read in the command itself 
  i2c_slave_read_buffer(_i2cPortNum, &buffer[ptr], 1, portMAX_DELAY);
  ptr += 1;

  // Read in the number of parameters
  i2c_slave_read_buffer(_i2cPortNum, &buffer[ptr], 1, portMAX_DELAY);
  ptr += 1;
  
  // Now, read in those parameters.
  for (int ndx = 0 ; ndx < buffer[2] ; ndx++) {
    // Read in the length of this parameters
    uint8_t param_length;
    i2c_slave_read_buffer(_i2cPortNum, &param_length, 1, portMAX_DELAY);
    buffer[ptr] = param_length;
    ptr += 1;

    for (int param_ndx = 0 ; param_ndx < param_length ; param_ndx++) {
      i2c_slave_read_buffer(_i2cPortNum, &buffer[ptr], 1, portMAX_DELAY);
      ptr += 1;
    }    
  }

  // Read the end byte.
  i2c_slave_read_buffer(_i2cPortNum, &buffer[ptr], 1, portMAX_DELAY);
  
  // Return 0 if everything is good.
  if ((buffer[0] == 0xE0) && (buffer[ptr] == 0xEE)) {
    return 0;
  } else {
    return 1;
  }
}

void I2CSClass::set_ready_state(bool is_ready) {
  if (is_ready) {
    // ets_printf("_readyPin is ready LOW\n");
    digitalWrite(_readyPin, LOW);  
  } else {
    // ets_printf("_readyPin is not ready HIGH\n");
    digitalWrite(_readyPin, HIGH);  
  }
  
}

int I2CSClass::transfer(uint8_t out[], uint8_t in[], size_t len)
{
  
  // If 'in' is NULL, then we're doing an output.
  if (in == NULL) {
    if (_debug) ets_printf("OUTPUT I2C BUFFER len[%d]\n", len);  
    // Send the bytes.
    // ESP-IDF: http://bit.ly/2xiJvpK
    // FIXME MCJ 20190627
    // How long until we time out?
    // How about one tick, which is... 10ms?
    // https://esp32.com/viewtopic.php?f=2&t=2377
    // Delay timing
    // 1 is 10ms
    // 10 is 100ms
    // 100 is 1000ms
    // int result =   time.sleep(1)
    // i2c_reset_tx_fifo(_i2cPortNum);

    i2c_reset_rx_fifo(_i2cPortNum);
    i2c_reset_tx_fifo(_i2cPortNum);
    vTaskDelay(10);
    i2c_slave_write_buffer(_i2cPortNum, out, len, 100);
    set_ready_state(true);


    
  } else if (out == NULL) {

    // Just before writing this response, clear the buffers.
    set_ready_state(true);
    int err = read_packet(in);
    set_ready_state(false);
    //i2c_reset_tx_fifo(_i2cPortNum);

    if (_debug && !err) {
      for (int ndx = 0; ndx < 8 ; ndx++) {
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
// Pin 14/A6 on the Huzzah32 is module pin 17.
I2CSClass I2CS(I2C_NUM_0, GPIO_NUM_22, GPIO_NUM_23, 0x2A, GPIO_NUM_14);

