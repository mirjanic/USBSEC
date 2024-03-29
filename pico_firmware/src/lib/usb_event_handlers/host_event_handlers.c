// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/9/21.
//

#include <malloc.h>
#include "usb_event_handlers.h"
#include "../debug/debug.h"

/*
 * Host events
 */

/// Setup packet to change device address
static const tusb_control_request_t CHG_ADDR_SETUP = {
        .bmRequestType = 0,
        .wValue = 7,
        .bRequest = 0x5 // SET ADDRESS
};

static tusb_control_request_t setup_packet;
static uint8_t bugger[1000];
static uint8_t level = 0;
static uint8_t dev_addr = 0;
static uint8_t debug_lock = false;

void define_setup_packet(uint8_t *setup) {
    memcpy(&setup_packet, setup, 8);
}

void slavework() {
    spi_message_t message;
    //if (debug_lock) return;
    if (dequeue_spi_message(&message)) {
        switch (message.e_flag & ~DEBUG_PRINT_AS_HEX) {

            case RESET_USB:
                /*
                 * Reset USB device
                 */
                send_string_message("RESET_USB");
                dev_addr = 0; // TODO is this needed?
                usb_hw->sie_ctrl |= USB_SIE_CTRL_RESET_BUS_BITS;
                break;

            case EDPT_OPEN:
                /*
                 * Open sent endpoint
                 */
                send_string_message("EDPT_OPEN");
                hcd_edpt_open((const tusb_desc_endpoint_t *) message.payload, dev_addr);
                break;

            case SETUP_DATA:
                /*
                 * Data is copied into setup
                 */
                send_string_message("SETUP_DATA");
                debug_lock = true;
                define_setup_packet(message.payload);
                level = 0;
                hcd_setup_send(0, dev_addr, (const uint8_t *) &setup_packet);
                break;

            case USB_DATA:
                /*
                 * Data is copied into buffer
                 */
                level = 3;
                send_string_message("USB_DATA");
                memcpy(bugger, message.payload, message.payload_length - 1);
                hcd_edpt_xfer(0,
                              dev_addr,
                              message.payload[message.payload_length - 1],
                              bugger,
                              message.payload_length - 1);
                break;

            case CHG_ADDR:
                /*
                 * Change device address to 7 (CHG_ADDR_SETUP.wValue)
                 */
                send_string_message("CHG_ADDR");
                debug_lock = true;
                define_setup_packet((uint8_t *) &CHG_ADDR_SETUP);
                level = 0;
                hcd_setup_send(0, dev_addr, (const uint8_t *) &setup_packet);
                dev_addr = CHG_ADDR_SETUP.wValue;
                break;

            case CHG_EPX_PACKETSIZE:
                /*
                 * Force packet size change for old devices which have packetsize < 64
                 * This is only needed for EPX as packet size will be set correctly for other endpoints
                 * at open time
                 */
                send_string_message("CHG_PCKSZ");
                runtime_assert(message.payload_length == 1);
                change_epx_packetsize(message.payload[0]);
                break;
            case 0:
                break;
            default:
                error("Invalid message from master!");
        }

        free(message.payload);
    }
}

void hcd_event_device_attach() {
    gpio_put(GPIO_SLAVE_DEVICE_ATTACHED_PIN, 1);
}

void hcd_event_device_remove() {
    gpio_put(GPIO_SLAVE_DEVICE_ATTACHED_PIN, 0);
}

static void send_event_to_master(uint16_t len, uint8_t ep_addr, uint16_t flag) {
    if (len == 0) {
        spi_message_t msg = {
                .payload_length = 1,
                .payload = &ep_addr,
                .e_flag = IS_PACKET | flag
        };
        enqueue_spi_message(&msg);
    } else {
        bugger[len] = ep_addr;
        spi_message_t msg = {
                .payload_length = len + 1,
                .payload = bugger,
                .e_flag = IS_PACKET | flag
        };
        enqueue_spi_message(&msg);
    }
}

void hcd_event_xfer_complete(uint8_t dev_addr_curr, uint8_t ep_addr, uint32_t xferred_bytes, int result, bool in_isr) {
    runtime_assert(result == 0 || result == 4);
    send_string_message("HCD XFER COMPLETE");

    if ((ep_addr & 0x7F) == 0) {
        /*
         * ep_addr is 0x00 or 0x80 and we are handling setup
         */
        if (level == 0) {
            /*
             * Setup packet is sent to device. Now need to read data.
             */
            runtime_assert(xferred_bytes == 0);
            runtime_assert(result == XFER_RESULT_SUCCESS);

            level = 1;
            uint16_t buglen = (setup_packet.bmRequestType_bit.direction == 0) ? 64 : setup_packet.wLength;
            hcd_edpt_xfer(0, dev_addr_curr, 0x80, bugger, buglen);
        } else if (level == 1) {
            /*
             * Setup response data received.
             */
            uint16_t last = 0;
            if (result == XFER_RESULT_SUCCESS) {
                level = 2;
                last = LAST_PACKET;
                debug_lock = false;
                if (setup_packet.bmRequestType_bit.direction == 1) {
                    hcd_edpt_xfer(0, dev_addr_curr, 0x00, NULL, 0); // Request ACK
                }
            } else {
                uint16_t buglen = (setup_packet.bmRequestType_bit.direction == 0) ? 64 : setup_packet.wLength;
                hcd_edpt_xfer(0, dev_addr_curr, 0x80, bugger, buglen);
            }
            send_event_to_master(xferred_bytes, ep_addr, last | SETUP_DATA);
        } else if (level == 2) {
            // Ack sent
        }
    } else {
        /*
         * Non-control transfer
         */
        send_string_message("AAHAAHAH");
        debug_lock = false;
        uint16_t new_flag = 0;
        if (result == 0) new_flag |= LAST_PACKET;
        if (result == 4) new_flag |= FIRST_PACKET;
        send_event_to_master(xferred_bytes, ep_addr, new_flag);
        //Problematic as xferred bytes is unbounded here. todo or is it?
    }
}