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

int debug = 0;

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
  xTaskCreate(i2c_handler, 
              // task handler
              "i2c_handler",
              // stack size
              10000,
              // task parameter
              NULL,
              // priority
              1,
              // handle for tracking the created task
              NULL
              );
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


#define ERROR_BYTE -1
#define ST_START 0x01
#define ST_HEADER 0x02
#define ST_LENGTH 0x03
#define ST_VALUE_LENGTHS 0x04
#define ST_VALUES 0x05
#define ST_CRC 0x06
#define ST_PROCESS 0x07
#define HEADER_B1 0xAD
#define HEADER_B2 0xAF
int state = ST_START;
#define NUM_POSSIBLE_PARAMS 255

// http://www.esp32learning.com/code/esp32-and-freertos-example-create-a-task.php
// https://www.hackster.io/Niket/tasks-parametertotasks-freertos-tutorial-5-b8a7b7
void i2c_handler (void* parameter) {
  uint8_t number_of_values = 0;
  uint8_t value_lengths[NUM_POSSIBLE_PARAMS];
  uint8_t value_counter = 0;
  uint8_t total_value_length = 0;
  uint8_t value_loc = 0;
  uint8_t write_location = 0;

  // FIXME FIXME: Until I rename, point to the same thing.
  uint8_t* values;
  values = commandBuffer;

  while (true) {
    // Returns -1 if error; otherwise, byte value.

    int b = I2CS.read_byte();

    switch (state) {
      // STATE: START
      // We are looking for the first header byte.
      case ST_START:
        switch (b) {
          case ERROR_BYTE:
            // pass
            break;
          case HEADER_B1:

            if (debug) ets_printf("HEADER B1\n");
            // Initialize the variables that carry state.
            number_of_values = 0;
            memset(&value_lengths, 0x00, NUM_POSSIBLE_PARAMS);
            // FIXME: Think about how much space for values array.
            //memset(&values, 0x00, 255);
            value_counter = 0;
            total_value_length = 0;
            value_loc = 0;
            write_location = 0;

            // Set the next state.
            state = ST_HEADER;
            break;
        }
        break;
      // STATE: HEADER
      // Now we are looking for the second header byte.
      case ST_HEADER:
        if (debug) ets_printf("HEADER\n");
        switch (b) {
          case HEADER_B2:
            state = ST_LENGTH;
            break;
          default:
            state = ST_START;
            break;
        }
        break;
      // In this state, we are going to read a single byte
      // that says how many values are being sent. A max of
      // 255 values can be packed together. 
      case ST_LENGTH:
        number_of_values = b;
        // The number of values is stored in the zeroth
        // possition, for handing off to the CommandHandler.
        values[write_location] = b;
        write_location += 1;

        if (debug) ets_printf("LENGTH: %d\n", b);
        
        if (number_of_values < NUM_POSSIBLE_PARAMS) {
          state = ST_VALUE_LENGTHS;
        } else {
          state = ST_START;
        }
        break;
      case ST_VALUE_LENGTHS:
        if (debug) ets_printf("VAL LENGTHS\n");
        value_lengths[value_counter] = b;
        // [0] Number of values
        // [1 ... ] The value lengths.
        values[write_location] = b;
        write_location += 1;
        value_counter++;
        if (debug) ets_printf("value_counter[%d] >= number_of_values[%d]\n", value_counter, number_of_values);
        if (value_counter >= number_of_values) {
          state = ST_VALUES;
          // Calculate how long all the values are.
          total_value_length = 0;
          for (int ndx = 0; ndx < number_of_values ; ndx++) {
            total_value_length += value_lengths[ndx];
          }
          // ets_printf("VAL LENGTHS total_value_length[%d]\n", total_value_length);
        } else { 
          state = ST_VALUE_LENGTHS;
        }
        break;
      case ST_VALUES:
        if (value_loc < total_value_length - 1) {
          // [0] Number of values
          // [1 ...] Value lengths
          values[write_location] = b;
          write_location += 1;
          value_loc++;
        } else {
          // Catch the last value.
          values[write_location] = b;
          write_location += 1;
          state = ST_CRC;
        }
        break;
      case ST_CRC:
        uint8_t crc = b;
        if (debug) ets_printf("CRC: %02x\n", crc);
        values[write_location] = crc;
        write_location += 1;

        if (debug) {
          ets_printf("VALUES READ:\n\t");
          for (int ndx = 0 ; ndx < write_location; ndx++) {
            ets_printf("%02x ", values[ndx]);
          }
          ets_printf("\n");
        }
        // This gets picked up outside the event handler.
        // It keeps us from overwriting the values[] array.
        state = ST_PROCESS;
        break; 
    }

    vTaskDelay(1);
  } 

  vTaskDelete(NULL);
}
// [       2 bytes] 0xAD 0xAF
// [        1 byte] Number of values (N)
// [       N bytes] The length of each value (M)
// [ E Ni*Mi bytes] Each value N of length M
// [        1 byte] CRC

void interpret_message() {
  ets_printf(".");
  CommandHandler.handle(commandBuffer, responseBuffer);
}

void loop() {
  if (state == ST_PROCESS) {
    interpret_message();
    state = ST_START;
  }
  vTaskDelay(1);
}
