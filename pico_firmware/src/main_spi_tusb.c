#include <stdio.h>
#include <string.h>
#include <hardware/vreg.h>
#include "pico/stdlib.h"

#include "hardware/gpio.h"

#include "lib/usb_phy/usb_common.h"
#include "lib/messages/messages.h"

#include "lib/rp2040/rp2040_usb.h"

uint8_t data[1000];

_Noreturn int main() {
    vreg_set_voltage(VREG_VOLTAGE_1_30);
    set_sys_clock_khz(290400, true);
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    bool test_master = true;

    if(is_master() ^ test_master) {
        //stdio_init_all();
        sleep_ms(2500); //To establish console in VM
    }

    messages_config();

    if(is_master()) {
        // master

        uint8_t len = 0;
        if(test_master) {

            dcd_init_new(0);
            dcd_int_enable_new(0);
            /*uint8_t setup[8] = {0x80, 0x6, 0x0, 0x1, 0x0, 0x0, 0x40, 0x0};
            spi_send_blocking(setup, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);
            spi_receive_blocking(bugger);
            gpio_put(PICO_DEFAULT_LED_PIN,0);
            spi_send_blocking(setup, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);
            spi_receive_blocking(bugger);*/
        }
        else {
            sleep_ms(500);
            uint8_t setup[8] = {0x80, 0x6, 0x0, 0x1, 0x0, 0x0, 0x40, 0x0}; //wLength = 64
            spi_send_blocking(setup, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);

            while(!(get_flag() & USB_DATA))
                spi_receive_blocking(data);
            sleep_ms(500);

            uint8_t setup2[8] = {0x80, 0x6, 0x0, 0x2, 0x0, 0x0, 0x2c, 0x0};
            spi_send_blocking(setup2, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);

            while(!(get_flag() & USB_DATA))
                spi_receive_blocking(data);
            sleep_ms(500);

            uint8_t config[8] = {0x0, 0x9, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0};
            spi_send_blocking(config, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);
        }
        while(true) {
            len = 0;//spi_receive_blocking(data);
            if(len > 0) {
                //dcd_edpt_xfer_new(0, 0x80, data, len);
                // Send received USB data to host
                //gpio_put(PICO_DEFAULT_LED_PIN,0);
            }
        }
    }
    else {
        // slave
        if(true || !test_master) {
            hcd_init(0);
            hcd_int_enable(0);


            //spi_receive_blocking(data);
            //define_setup_packet(data);
            //uint8_t setup[8] = {0x80, 0x6, 0x0, 0x1, 0x0, 0x0, 0x40, 0x0};
            //define_setup_packet(setup);
            //uint8_t setup2[8] = {0x80, 0x6, 0x0, 0x2, 0x0, 0x0, 0x2c, 0x0};
            //hcd_setup_send(0, 0, setup2);
            //{0x80, 0x6, 0x0, 0x2, 0x0, 0x0, 0x2c, 0x0};
            //{0x80, 0x6, 0x0, 0x1, 0x0, 0x0, 0x40, 0x0}; //wLength = 64
            //define_setup_packet(setup);
            slavework();
        }

        uint8_t arr[18] = {0x12, 0x01, 0x10, 0x02, 0x00, 0x00, 0x00, 0x40,
                           0x81, 0x07, 0x81, 0x55, 0x00, 0x01, 0x01, 0x02,
                           0x03, 0x01};
        uint8_t cfg[32] = {0x09, 0x02, 0x20, 0x00,0x01, 0x01, 0x00, 0x80,
        0x70, 0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06,
        0x50, 0x00, 0x07, 0x05, 0x81, 0x02, 0x40, 0x00,
        0x00, 0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00};
        int time = 0;
        while(true) {

            //spi_receive_blocking(data);

            /*if(!(get_flag() & SETUP_DATA)) continue;
            if(time==1 || time == 0)
            {
                spi_send_blocking(arr, 18, USB_DATA | DEBUG_PRINT_AS_HEX);
                time++;
            } else {
                spi_send_blocking(cfg, 32, USB_DATA | DEBUG_PRINT_AS_HEX);
            }*/
            //printf("flag: %d", get_flag());
            /*if(get_flag() & SETUP_DATA) {
                gpio_put(PICO_DEFAULT_LED_PIN,0);

                //sleep_ms(5);
                //spi_send_blocking(arr, 18, USB_DATA);
            }*/
            /*uint8_t len = spi_receive_blocking(data);
            //spi_send_string("test");
            //spi_send_blocking(data, len, DEBUG_PRINT_AS_HEX);
            if(!test_master) {
                if(get_flag() & USB_DATA) {
                    hcd_edpt_xfer(0, 0x00, 0x80, data, len); //0x80?

                } else if (get_flag() & SETUP_DATA) {
                    level = 0;
                    hcd_setup_send(0, 0, data);
                    //spi_send_string("test");
                }
            }*/
           /*
            if(len > 0) {
                // Send received USB data to host
                gpio_put(PICO_DEFAULT_LED_PIN,0);
                if(!test_master) {
                    hcd_edpt_xfer(0, 0x00, 0x00, data, len); //Send second setup
                }
                uint8_t arr[18] = {0x12, 0x01, 0x10, 0x02, 0x00, 0x00, 0x00, 0x40,
                                   0x81, 0x07, 0x81, 0x55, 0x00, 0x01, 0x01, 0x02,
                                   0x03, 0x01};

                //spi_send_blocking(arr, 18, USB_DATA | DEBUG_PRINT_AS_HEX);
            }*/
        }

    }

}

/*
0x09 0x02 0x20 0x00 0x01 0x01 0x00 0x80
0x70 0x09 0x04 0x00 0x00 0x02 0x08 0x06
0x50 0x00 0x07 0x05 0x81 0x02 0x40 0x00
0x00 0x07 0x05 0x02 0x02 0x40 0x00 0x00
 */