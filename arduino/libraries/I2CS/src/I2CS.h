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

#ifndef I2CS_H
#define I2CS_H

#include <driver/gpio.h>
#include <driver/i2c.h>
  
class I2CSClass {

  public:
    // i2cNum  -> Either I2C_NUM_0 or I2C_NUM_1
    // sclPin  -> 
    // sdaPin  ->
    // i2cAddr -> In the range 0x01 to 0x7F
    // Regarding GPIO pin numbers: http://bit.ly/31W5e57
    I2CSClass(int i2cNum, int sclPin, int sdaPin, int i2cAddr);

    int begin();
    int transfer(uint8_t out[], uint8_t in[], size_t len);

  private:
    // static void onSetupComplete(spi_slave_transaction_t*);
    void handleSetupComplete();

  private:
    int _i2cNum;
    int _sclPin;
    int _sdaPin;
    int _i2cAddr;
    // WARNING MCJ 20190627
    // Choosing buffer lengths arbitrarily. May wany to 
    // revisit these numbers. And, think about how they're declared/specified.
    int _i2c_rx_buff_len = 1024;
    int _i2c_tx_buff_len = 1024;

    intr_handle_t _csIntrHandle;

    SemaphoreHandle_t _readySemaphore;
};

extern I2CSClass I2CS;

#endif

