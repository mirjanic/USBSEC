// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/6/21.
//

#ifndef PICO_FIRMWARE_USB_EVENT_HANDLERS_H
#define PICO_FIRMWARE_USB_EVENT_HANDLERS_H

#include <pico/stdlib.h>
#include <string.h>
#include "../messages/messages.h"
#include "tusb_common.h"
#include "tusb_types.h"
#include <hardware/structs/usb.h>

typedef struct hw_endpoint {
    // Is this a valid struct
    bool configured;

    // Transfer direction (i.e. IN is rx for host but tx for device)
    // allows us to common up transfer functions
    bool rx;

    uint8_t ep_addr;
    uint8_t next_pid;

    // Endpoint control register
    io_rw_32 *endpoint_control;

    // Buffer control register
    io_rw_32 *buffer_control;

    // Buffer pointer in usb dpram
    uint8_t *hw_data_buf;

    // Current transfer information
    bool active;
    uint16_t remaining_len;
    uint16_t xferred_len;

    // User buffer in main memory
    uint8_t *user_buf;

    // Data needed from EP descriptor
    uint16_t wMaxPacketSize;

    // Interrupt, bulk, etc
    uint8_t transfer_type;

    // Only needed for host
    uint8_t dev_addr;

    // If interrupt endpoint
    uint8_t interrupt_num;

    // Partial transfer
    bool partial;
} hw_endpoint_t;

void define_setup_packet(uint8_t *setup);

// Helper to send device attach event
void hcd_event_device_attach();

// Helper to send device removal event
void hcd_event_device_remove();

void hcd_event_xfer_complete(uint8_t dev_addr, uint8_t ep_addr, uint32_t xferred_bytes, int result, bool in_isr);

void dcd_event_xfer_complete_new(uint8_t rhport, uint8_t ep_addr, uint32_t xferred_bytes, uint8_t result, bool in_isr);

void dcd_event_setup_received_new(uint8_t rhport, uint8_t const *setup, bool in_isr);

extern void hcd_setup_send(uint8_t rhport, uint8_t dev_addr, uint8_t const setup_packet[8]);

void device_event_bus_reset(void);

bool dcd_edpt_open_new(uint8_t rhport, tusb_desc_endpoint_t const *desc_edpt);

extern hw_endpoint_t *get_dev_ep(uint8_t dev_addr, uint8_t ep_addr);

bool hcd_init(uint8_t rhport);

bool hcd_edpt_open(tusb_desc_endpoint_t const *ep_desc, uint8_t dev_addr);

// Call to send data
void hw_endpoint_xfer_start(struct hw_endpoint *ep, uint8_t *buffer /* user_buf*/, uint16_t total_len);

bool hcd_edpt_xfer(uint8_t rhport, uint8_t dev_addr, uint8_t ep_addr, uint8_t *buffer, uint16_t buflen);

bool dcd_edpt_xfer_new(uint8_t rhport, uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes);

void dcd_init_new(uint8_t rhport);

void dcd_edpt0_status_complete(uint8_t rhport, tusb_control_request_t const *request);

void slavework(void);

void dcd_edpt_xfer_partial(uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes, uint16_t flag);

void handle_spi_slave_event(void);

void change_epx_packetsize(uint8_t bMaxPacketSize0);

#endif //PICO_FIRMWARE_USB_EVENT_HANDLERS_H
