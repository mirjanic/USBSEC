//
// Created by vlamir on 11/6/21.
//

#include "usb_event_handlers.h"

static uint8_t bugger[1000];
bool cfg_set = false;

/*
 * Device Events
 */

void dcd_event_setup_received_new(uint8_t rhport, uint8_t const *setup, bool in_isr) {
    tusb_control_request_t *const req = (tusb_control_request_t *) setup;
    if (req->bRequest == 0x5 /*SET ADDRESS*/) {
        /*
         * If request is SET_ADDRESS we do not want to pass it on.
         * Instead, it gets handled here and slave keeps communicating to device on dev_addr 0
         */
        dcd_edpt_xfer_new(rhport, 0x80, NULL, 0); // ACK
        return;
    }

    /*
     * Forward setup packet to slave and get response from device
     */
    spi_send_blocking(setup, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);

    int len = spi_await(bugger, USB_DATA);

    /*
     * Hooks
     */
    if (setup[3] == 0x2 && setup[6] > 9) { // TODO Harden
        // Endpoints
        printf("doing endpoints.\n");
        int pos = 0;
        while (pos < len) {
            if (bugger[pos + 1] == 0x05) {
                const tusb_desc_endpoint_t *const edpt = (const tusb_desc_endpoint_t *const) &bugger[pos];
                dcd_edpt_open_new(rhport, edpt);
                // Start read
                printf("0x%x\n", edpt->bEndpointAddress);
                if (edpt->bEndpointAddress == 2) {
                    dcd_edpt_xfer_new(rhport, edpt->bEndpointAddress, bugger, 64);
                }
                spi_send_blocking((const uint8_t *) edpt, edpt->bLength, EDPT_OPEN); // TODO ONLY IF INTERRUPT
                insert_into_registry(edpt);
                spi_await(bugger, USB_DATA);
            }
            pos += bugger[pos];
        }
    }
    if (req->bRequest == 0x09 /* SET CONFIG */) {
        cfg_set = true;
        printf("Configuration confirmed.\n");
    }

    dcd_edpt_xfer_new(0, 0x80, bugger, len);
    dcd_edpt_xfer_new(rhport, 0x00, NULL, 0);
}

static bool needack = false;

void dcd_event_xfer_complete_new(uint8_t rhport, uint8_t ep_addr, uint32_t xferred_bytes, uint8_t result, bool in_isr) {

    if (((tusb_control_request_t *) usb_dpram->setup_packet)->bRequest == 0x5 /*SET ADDRESS*/) {
        //spi_send_blocking(&usb_dpram->setup_packet, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);
        //assert(spi_await(bugger, USB_DATA) == 0);
        printf("Setting address to %d, [%d] %d\n", ((const tusb_control_request_t *) usb_dpram->setup_packet)->wValue,
               ep_addr,
               xferred_bytes);
        dcd_edpt0_status_complete(0, (const tusb_control_request_t *) usb_dpram->setup_packet); // Update address
        return;
    }

    if (ep_addr == 0 || ep_addr == 0x80) return;


    printf("+-----\n|Completed transfer on %d\n+-----\n", ep_addr);
    printf("Bugger points to %p\n", bugger);
    if (xferred_bytes == 0 || result != 0) return;


    // TODO HARDEN THIS INTERACTION
    /*bugger[xferred_bytes] = 1;
    spi_send_blocking(bugger, xferred_bytes + 1, USB_DATA | DEBUG_PRINT_AS_HEX);
    spi_await(bugger, USB_DATA);
    memset(bugger, 0, 64);
    bugger[64] = 0;
    spi_send_blocking(bugger, 64 + 1, USB_DATA);
    int len = spi_await(bugger, USB_DATA);*/

    if (~ep_addr & 0x80) {
        if (needack) {
            uint8_t bkp[100];
            printf("sending ack\n");
            uint8_t ack = 1;
            spi_send_blocking(&ack, 1, USB_DATA | DEBUG_PRINT_AS_HEX);
            spi_await(bkp, GOING_IDLE);
            trigger_spi_irq();
        }
        bugger[xferred_bytes] = 1; //TODO
        spi_send_blocking(bugger, xferred_bytes + 1, USB_DATA | DEBUG_PRINT_AS_HEX);
        spi_await(bugger, GOING_IDLE);
        trigger_spi_irq();

        needack = !needack;
        int len = 0; // Todo remove cheese
        while (len != 13) {
            memset(bugger, 0, 64);
            bugger[64] = 0;
            printf("Poll %d [0x%x]\n", 0, 0x81);
            spi_send_blocking(bugger, 64 + 1, USB_DATA);

            printf("Waiting idle \n");
            len = spi_await(bugger, USB_DATA);
            dcd_edpt_xfer_new(rhport, 0x81, bugger, len);
            trigger_spi_irq();
        }

        dcd_edpt_xfer_new(rhport, ep_addr, bugger, 64);
    }

    /*const tusb_desc_endpoint_t *edpt = get_first_in_registry();
    for (int i = 0; i < 2; i++, edpt = get_next_in_registry(edpt)) {
        if (!(edpt->bEndpointAddress & 0x80)) {
            // Skip OUT endpoints as we want to get data into host.

            continue;
        }
        while (true) {
            memset(bugger, 0, 64);
            bugger[64] = i;
            printf("Poll %d [0x%x]\n", i, edpt->bEndpointAddress);
            spi_send_blocking(bugger, 64 + 1, USB_DATA);

            printf("Waiting idle \n");
            int len = spi_await(bugger, USB_DATA);
            dcd_edpt_xfer_new(rhport, edpt->bEndpointAddress, bugger, len);
            trigger_spi_irq();
            break;
        }
    }*/

/*
    bool gottem = false;
    while (!gottem) {
        gottem = true;

        printf("Endpoint iteration done \n");
        trigger_spi_irq();
        spi_send_blocking(NULL, 0, EVENTS);
        spi_receive_blocking(bugger);
        int count = bugger[0];
        while (count--) {
            gottem = false;
            int len = spi_receive_blocking(bugger);
            dcd_edpt_xfer_new(rhport, bugger[len - 1], bugger, len - 1);
        }
    }*/

}

void device_event_bus_reset() {
    printf("Resetting\n");
    spi_send_blocking(NULL, 0, RESET_USB);
}
