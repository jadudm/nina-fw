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


I2CSClass::I2CSClass(int i2cNum, int sclPin, int sdaPin, int i2cAddr) :
  _i2cNum(i2cNum),
  _sclPin(sclPin),
  _sdaPin(sdaPin),
  _i2cAddr(i2cAddr)
{
  // Empty class. Nifty C++ thing.
}


int I2CSClass::begin()
{
  i2c_port_t i2c_slave_port = (i2c_port_t) _i2cNum;
 
  i2c_config_t conf;
  // NOTE MCJ 20190627
  // C++ type conversion around enums means I have to be more specific/careful
  // with declarations of GPIO pins. Even though these are "just" integers,
  // C++ is trying to make sure things come through correctly.
  // https://esp32.com/viewtopic.php?t=1591
  conf.sda_io_num = (gpio_num_t) _sdaPin;
  conf.scl_io_num = (gpio_num_t)  _sclPin;
  conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
  conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
  conf.mode = I2C_MODE_SLAVE;
  conf.slave.addr_10bit_en = 0;
  conf.slave.slave_addr = _i2cAddr;

  // FIXME MCJ 20190627
  // Check what these macros do, and decide if I'd rather 
  // do something else for error handling.
  ESP_ERROR_CHECK(i2c_param_config(i2c_slave_port, &conf));
  ESP_ERROR_CHECK(i2c_driver_install(
      i2c_slave_port,
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
  return 1;
}

/*
void I2CSClass::onSetupComplete(spi_slave_transaction_t*)
{
  
}
 */

void I2CSClass::handleSetupComplete()
{
  
}

/*
#define I2C_SLAVE_1_SCL         19 
#define I2C_SLAVE_1_SDA         18 
#define I2C_SLAVE_1_NUM         I2C_NUM_0
#define I2C_SLAVE_1_TX_BUF_LEN  (1024 * SLAVE_1_DATA_LENGTH)
#define I2C_SLAVE_1_RX_BUF_LEN  (1024 * SLAVE_1_DATA_LENGTH)
*/

// FIXME WARNING MCJ 20190627
// Need to choose a default I2C address.
// Need to decide if there will be a jumper or similar to allow
// choice of I2C address. If so, then this becomes dynamic
// at class init time based on a GPIO read of high/low. 
I2CSClass I2CS(I2C_NUM_0, GPIO_NUM_19, GPIO_NUM_18, 0x2A);

