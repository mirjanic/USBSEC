//
// Created by vlamir on 11/2/21.
//

#ifndef PICO_FIRMWARE_MESSAGES_H
#define PICO_FIRMWARE_MESSAGES_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include <stdio.h>
#include <string.h>

#define GPIO_MASTER_SELECT_PIN 0
#define GPIO_SLAVE_IRQ_PIN 14
#define SPI_BAUDRATE (int)(6*1000*1000) // 8MHz is too much at 144MHz clock

typedef enum {
    USB_DATA = 0x1,
    SETUP_DATA = 0x2,
    DEBUG_PRINT_AS_STRING = 0x4,
    DEBUG_PRINT_AS_HEX = 0x8
} msg_type;

typedef enum {
    SPI_ROLE_MASTER,
    SPI_ROLE_SLAVE
} spi_role;

void messages_config(void);
void spi_send_blocking(const uint8_t *data, uint8_t len, uint8_t flag);
uint8_t spi_receive_blocking(uint8_t *data);
void spi_send_string(char *data);
spi_role get_role(void);
void spi_send_async(const uint8_t *data, uint8_t len, uint8_t flag);
uint8_t get_flag();

#endif //PICO_FIRMWARE_MESSAGES_H
