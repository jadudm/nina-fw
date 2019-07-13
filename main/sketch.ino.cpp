/*
  This file is part of the Arduino NINA firmware.
  Copyright (c) 2018 Arduino SA. All rights reserved.

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

#include <esp32/rom/uart.h>

extern "C" {
  #include <driver/periph_ctrl.h>
  #include <driver/uart.h>
  #include <esp_bt.h>
  // MCJ 20190626 
  // For esp-idf 4.X, this driver is needed for the 
  // GPIO_PIN_MUX_REG macro.
  #include <driver/gpio.h>
}

#include <Arduino.h>

#include <SPIS.h>
#include <I2CS.h>
#include <WiFi.h>

#include "CommandHandler.h"

#define USE_I2C 1

// #define SPI_BUFFER_LEN SPI_MAX_DMA_LEN
#define SPI_BUFFER_LEN 32

int debug = 1;

uint8_t* commandBuffer;
uint8_t* responseBuffer;

// FIXME: Think about how much space for values array.
// uint8_t values[255];

void dumpBuffer(const char* label, uint8_t data[], int length) {
  ets_printf("%s: ", label);

  for (int i = 0; i < length; i++) {
    ets_printf("%02x", data[i]);
  }

  ets_printf("\r\n");
}

void setDebug(int d) {
  debug = d;

  // FIXME Was if(debug)
  if (1) {
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[1], 0);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[3], 0);

    const char* default_uart_dev = "/dev/uart/0";
    _GLOBAL_REENT->_stdin  = fopen(default_uart_dev, "r");
    _GLOBAL_REENT->_stdout = fopen(default_uart_dev, "w");
    _GLOBAL_REENT->_stderr = fopen(default_uart_dev, "w");

    uart_div_modify(CONFIG_CONSOLE_UART_NUM, (APB_CLK_FREQ << 4) / 115200);

    // uartAttach();
    ets_install_uart_printf();
    uart_tx_switch(CONFIG_CONSOLE_UART_NUM);

    ets_printf("*** DEBUG ON\n");
  } else {
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[1], PIN_FUNC_GPIO);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[3], PIN_FUNC_GPIO);

    _GLOBAL_REENT->_stdin  = (FILE*) &__sf_fake_stdin;
    _GLOBAL_REENT->_stdout = (FILE*) &__sf_fake_stdout;
    _GLOBAL_REENT->_stderr = (FILE*) &__sf_fake_stderr;

    ets_install_putc1(NULL);
    ets_install_putc2(NULL);
  }
}

// MCJ 20190628 REMOVED CODE
// The code for the Arduino had setup routines for 
// bluetooth... but, that code didn't get used.
// Furhter, the command handlers were initialized 
// in the WiFi setup routine... which didn't set up the
// WiFi. I've moved this code to setup(), where it
// belongs.


bool has_message = false;
void i2c_handler(void* parameter);

void setup() {
  setDebug(debug);

  // put SWD and SWCLK pins connected to SAMD as inputs
  // pinMode(15, INPUT);
  // pinMode(21, INPUT);
  
  if (debug) ets_printf("Initializing buffers\n");
  commandBuffer = (uint8_t*)heap_caps_malloc(SPI_BUFFER_LEN, MALLOC_CAP_DMA);
  responseBuffer = (uint8_t*)heap_caps_malloc(SPI_BUFFER_LEN, MALLOC_CAP_DMA);
  memset(commandBuffer, 0x00, SPI_BUFFER_LEN);
  memset(responseBuffer, 0x00, SPI_BUFFER_LEN);

  #ifdef USE_I2C
    if (debug)  ets_printf("*** I2CS\n");
    I2CS.begin();
  #else
    if (debug)  ets_printf("*** SPIS\n");
    SPIS.begin();
  #endif
  
  if (debug) ets_printf("Starting CommandHandler\n");
  CommandHandler.begin();

  if (debug) ets_printf("i2c_handler task creation...\n");
  // xTaskCreate(i2c_handler, 
  //             // task handler
  //             "i2c_handler",
  //             // stack size
  //             10000,
  //             // task parameter
  //             NULL,
  //             // priority
  //             1,
  //             // handle for tracking the created task
  //             NULL
  //             );
}

// MCJ 20190627
// I don't know if I want a compiler directive, but this is 
// one way to keep the main loop clean.
int transfer (uint8_t out[], uint8_t in[], size_t len) {
  int commandLength = 0;
  #ifdef USE_I2C
  commandLength = I2CS.transfer(out, in, len);
  #else
  commandLength = SPIS.transfer(NULL, commandBuffer, SPI_BUFFER_LEN);
  #endif
  return commandLength;
}

void loop() {
  I2CS.transfer(NULL, commandBuffer, 0);
  ets_printf(".");
  CommandHandler.handle(commandBuffer, responseBuffer);
  vTaskDelay(1);  
}