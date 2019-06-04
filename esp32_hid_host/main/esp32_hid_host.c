/*
 * Copyright (C) 2017 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */

#define __BTSTACK_FILE__ "esp32_hid_host.c"

/* EXAMPLE_START(hid_host_demo): HID Host Demo
 *
 * @text This example implements an HID Host for the ESP32. For now, it connects to a fixed Xbox One Controller, queries the HID SDP
 * record and opens the HID Control + Interrupt channels
 */

#include <inttypes.h>
#include <stdio.h>
#include <math.h>

#include "btstack_config.h"
#include "btstack.h"
#include "driver/ledc.h"
#include "esp_err.h"

#define MAX_ATTRIBUTE_VALUE_SIZE 300
#define HUNDRED 100
// ### Xbox One Controller
// Address
#define MAC_ADDRESS "5C-BA-37-FE-E0-03"
// Controls
#define JOYSTICK_FULL 65535
#define TRIGGER_FULL 1023
#define DPAD_UP 1
#define DPAD_RIGHT 3
#define DPAD_DOWN 5
#define DPAD_LEFT 7
#define BUTTON_A 1
#define BUTTON_B 2
#define BUTTON_X 4
#define BUTTON_Y 8
#define BUTTON_LEFT 16
#define BUTTON_RIGHT 32
#define BUTTON_BACK 64
#define BUTTON_START 128
#define LEFT_STICK_PUSH 1
#define RIGHT_STICK_PUSH 2
// Bluetooth packets
#define JOYSTICK_PACKET_SIZE 4 // 2 bytes for x and y
#define TRIGGER_PACKET_SIZE 2 // just one direction
// PWM
#define PWM_FREQ 62 // Hz
#define MOTOR_PWM_CHANNEL_1 LEDC_CHANNEL_1
#define MOTOR_PWM_CHANNEL_2 LEDC_CHANNEL_2
#define MOTOR_PWM_CHANNEL_3 LEDC_CHANNEL_3
#define MOTOR_PWM_TIMER LEDC_TIMER_1
#define MOTOR_PWM_BIT_NUM LEDC_TIMER_10_BIT
// GPIO
#define PWM1_PIN GPIO_NUM_19
#define PWM2_PIN GPIO_NUM_21
#define PWM3_PIN GPIO_NUM_18

// SDP
static uint8_t            hid_descriptor[MAX_ATTRIBUTE_VALUE_SIZE];
static uint16_t           hid_descriptor_len;

static uint16_t           hid_control_psm;
static uint16_t           hid_interrupt_psm;

static uint8_t            attribute_value[MAX_ATTRIBUTE_VALUE_SIZE];
static const unsigned int attribute_value_buffer_size = MAX_ATTRIBUTE_VALUE_SIZE;

// L2CAP
static uint16_t           l2cap_hid_control_cid;
static uint16_t           l2cap_hid_interrupt_cid;

// Xbox One Controller
static const char * remote_addr_string = MAC_ADDRESS;

static ledc_channel_config_t pwm1;
static ledc_channel_config_t pwm2;
static ledc_channel_config_t pwm3;

static bd_addr_t remote_addr;

static btstack_packet_callback_registration_t hci_event_callback_registration;


/* @section Main application configuration
 *
 * @text In the application configuration, L2CAP is initialized 
 */
static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void handle_sdp_client_query_result(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void check_controller_joystick_left_move(uint16_t left_joy_x, uint16_t left_joy_y);
static void check_controller_joystick_right_move(uint16_t right_joy_x, uint16_t right_joy_y);
static void check_controller_trigger_left(uint16_t left_trigger_pos);
static void check_controller_trigger_right(uint16_t right_trigger_pos);
static float calc_speed_motor(uint16_t value);
static void check_controller_dpad(uint8_t packet15);
static void check_controller_button(uint8_t packet16);
static void check_controller_joystick_push(uint8_t packet17);
static int fill_joystick_data(uint8_t joystick_packets[], uint8_t *packet);
static int fill_trigger_data(uint8_t trigger_packets[], uint8_t *packet);
static void handle_controller_interrupts(uint8_t *packet, uint16_t size);
static void motor_pwm_init(void);
static void pwm1_duty_set(float perc);
static void pwm2_duty_set(float perc);
static void pwm3_duty_set(float perc);

static void hid_host_setup(void){
    // Initialize L2CAP 
    l2cap_init();

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // Disable stdout buffering
    setbuf(stdout, NULL);
}

/* @section SDP parser callback 
 * 
 * @text The SDP parsers retrieves the BNEP PAN UUID as explained in  
 * Section [on SDP BNEP Query example](#sec:sdpbnepqueryExample}.
 */
static void handle_sdp_client_query_result(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    des_iterator_t attribute_list_it;
    des_iterator_t additional_des_it;
    des_iterator_t prot_it;
    uint8_t       *des_element;
    uint8_t       *element;
    uint32_t       uuid;
    uint8_t        status;

    switch (hci_event_packet_get_type(packet)){
        case SDP_EVENT_QUERY_ATTRIBUTE_VALUE:
            if (sdp_event_query_attribute_byte_get_attribute_length(packet) <= attribute_value_buffer_size) {
                attribute_value[sdp_event_query_attribute_byte_get_data_offset(packet)] = sdp_event_query_attribute_byte_get_data(packet);
                if ((uint16_t)(sdp_event_query_attribute_byte_get_data_offset(packet)+1) == sdp_event_query_attribute_byte_get_attribute_length(packet)) {
                    switch(sdp_event_query_attribute_byte_get_attribute_id(packet)) {
                        case BLUETOOTH_ATTRIBUTE_PROTOCOL_DESCRIPTOR_LIST:
                            for (des_iterator_init(&attribute_list_it, attribute_value); des_iterator_has_more(&attribute_list_it); des_iterator_next(&attribute_list_it)) {                                    
                                if (des_iterator_get_type(&attribute_list_it) != DE_DES) continue;
                                des_element = des_iterator_get_element(&attribute_list_it);
                                des_iterator_init(&prot_it, des_element);
                                element = des_iterator_get_element(&prot_it);
                                if (!element) continue;
                                if (de_get_element_type(element) != DE_UUID) continue;
                                uuid = de_get_uuid32(element);
                                des_iterator_next(&prot_it);
                                switch (uuid){
                                    case BLUETOOTH_PROTOCOL_L2CAP:
                                        if (!des_iterator_has_more(&prot_it)) continue;
                                        de_element_get_uint16(des_iterator_get_element(&prot_it), &hid_control_psm);
                                        printf("HID Control PSM: 0x%04x\n", (int) hid_control_psm);
                                        break;
                                    default:
                                        break;
                                }
                            }
                            break;
                        case BLUETOOTH_ATTRIBUTE_ADDITIONAL_PROTOCOL_DESCRIPTOR_LISTS:
                            for (des_iterator_init(&attribute_list_it, attribute_value); des_iterator_has_more(&attribute_list_it); des_iterator_next(&attribute_list_it)) {                                    
                                if (des_iterator_get_type(&attribute_list_it) != DE_DES) continue;
                                des_element = des_iterator_get_element(&attribute_list_it);
                                for (des_iterator_init(&additional_des_it, des_element); des_iterator_has_more(&additional_des_it); des_iterator_next(&additional_des_it)) {                                    
                                    if (des_iterator_get_type(&additional_des_it) != DE_DES) continue;
                                    des_element = des_iterator_get_element(&additional_des_it);
                                    des_iterator_init(&prot_it, des_element);
                                    element = des_iterator_get_element(&prot_it);
                                    if (!element) continue;
                                    if (de_get_element_type(element) != DE_UUID) continue;
                                    uuid = de_get_uuid32(element);
                                    des_iterator_next(&prot_it);
                                    switch (uuid){
                                        case BLUETOOTH_PROTOCOL_L2CAP:
                                            if (!des_iterator_has_more(&prot_it)) continue;
                                            de_element_get_uint16(des_iterator_get_element(&prot_it), &hid_interrupt_psm);
                                            printf("HID Interrupt PSM: 0x%04x\n", (int) hid_interrupt_psm);
                                            break;
                                        default:
                                            break;
                                    }
                                }
                            }
                            break;
                        case BLUETOOTH_ATTRIBUTE_HID_DESCRIPTOR_LIST:
                            for (des_iterator_init(&attribute_list_it, attribute_value); des_iterator_has_more(&attribute_list_it); des_iterator_next(&attribute_list_it)) {
                                if (des_iterator_get_type(&attribute_list_it) != DE_DES) continue;
                                des_element = des_iterator_get_element(&attribute_list_it);
                                for (des_iterator_init(&additional_des_it, des_element); des_iterator_has_more(&additional_des_it); des_iterator_next(&additional_des_it)) {                                    
                                    if (des_iterator_get_type(&additional_des_it) != DE_STRING) continue;
                                    element = des_iterator_get_element(&additional_des_it);
                                    const uint8_t * descriptor = de_get_string(element);
                                    hid_descriptor_len = de_get_data_size(element);
                                    memcpy(hid_descriptor, descriptor, hid_descriptor_len);
                                    printf("HID Descriptor:\n");
                                    printf_hexdump(hid_descriptor, hid_descriptor_len);
                                }
                            }                        
                            break;
                        default:
                            break;
                    }
                }
            } else {
                fprintf(stderr, "SDP attribute value buffer size exceeded: available %d, required %d\n", attribute_value_buffer_size, sdp_event_query_attribute_byte_get_attribute_length(packet));
            }
            break;
            
        case SDP_EVENT_QUERY_COMPLETE:
            if (!hid_control_psm) {
                printf("HID Control PSM missing\n");
                break;
            }
            if (!hid_interrupt_psm) {
                printf("HID Interrupt PSM missing\n");
                break;
            }
            printf("Setup HID\n");
            status = l2cap_create_channel(packet_handler, remote_addr, hid_control_psm, 48, &l2cap_hid_control_cid);
            if (status){
                printf("Connecting to HID Control failed: 0x%02x\n", status);
            }
            break;
    }
}

/*
 * @section Packet Handler
 * 
 * @text The packet handler responds to various HCI Events.
 */
static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    /* LISTING_PAUSE */
    uint8_t   event;
    bd_addr_t event_addr;
    uint8_t   status;
    uint16_t  l2cap_cid;

    /* LISTING_RESUME */
    switch (packet_type) {
		case HCI_EVENT_PACKET:
            event = hci_event_packet_get_type(packet);
            switch (event) {            
                /* @text When BTSTACK_EVENT_STATE with state HCI_STATE_WORKING
                 * is received and the example is started in client mode, the remote SDP HID query is started.
                 */
                case BTSTACK_EVENT_STATE:
                    if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING){
                        printf("Start SDP HID query for remote HID Device.\n");
                        sdp_client_query_uuid16(&handle_sdp_client_query_result, remote_addr, BLUETOOTH_SERVICE_CLASS_HUMAN_INTERFACE_DEVICE_SERVICE);
                    }
                    break;

                /* LISTING_PAUSE */
                case HCI_EVENT_PIN_CODE_REQUEST:
					// inform about pin code request
                    printf("Pin code request - using '0000'\n");
                    hci_event_pin_code_request_get_bd_addr(packet, event_addr);
                    gap_pin_code_response(event_addr, "0000");
					break;

                case HCI_EVENT_USER_CONFIRMATION_REQUEST:
                    // inform about user confirmation request
                    printf("SSP User Confirmation Request with numeric value '%"PRIu32"'\n", little_endian_read_32(packet, 8));
                    printf("SSP User Confirmation Auto accept\n");
                    break;

                /* LISTING_RESUME */

                case L2CAP_EVENT_CHANNEL_OPENED: 
                    status = packet[2];
                    if (status){
                        printf("L2CAP Connection failed: 0x%02x\n", status);
                        break;
                    }
                    l2cap_cid  = little_endian_read_16(packet, 13);
                    if (!l2cap_cid) break;
                    if (l2cap_cid == l2cap_hid_control_cid){
                        status = l2cap_create_channel(packet_handler, remote_addr, hid_interrupt_psm, 48, &l2cap_hid_interrupt_cid);
                        if (status){
                            printf("Connecting to HID Control failed: 0x%02x\n", status);
                            break;
                        }
                    }                        
                    if (l2cap_cid == l2cap_hid_interrupt_cid){
                        printf("HID Connection established\n");
                    }
                    break;
                default:
                    break;
            }
            break;
        case L2CAP_DATA_PACKET:
            if (channel == l2cap_hid_interrupt_cid){
                handle_controller_interrupts(packet, size);
            } else if (channel == l2cap_hid_control_cid){
                printf("HID Control: ");
                printf_hexdump(packet, size);
            } else {
                break;
            }
        default:
            break;
    }
}

/* handles left Joystick rotation */
/* 100 in y ist unten 0 ist oben
 0 in x ist links 100 ist rechts*/
static void check_controller_joystick_left_move(uint16_t left_joy_x, uint16_t left_joy_y) {
    printf("LJoy_x: %d%\n",((left_joy_x * HUNDRED) / JOYSTICK_FULL));
    printf("LJoy_y: %d%\n",((left_joy_y * HUNDRED) / JOYSTICK_FULL));
    // ...
}

/* handles right Joystick rotation */
static void check_controller_joystick_right_move(uint16_t right_joy_x, uint16_t right_joy_y) {
    printf("RJoy_x: %d%\n",((right_joy_x * HUNDRED) / JOYSTICK_FULL));
    printf("RJoy_y: %d%\n",((right_joy_y * HUNDRED) / JOYSTICK_FULL));
    // ...
}

/* handles left Trigger (LT) position */
static void check_controller_trigger_left(uint16_t left_trigger_pos) {
    printf("LT: %d%\n",left_trigger_pos);
    pwm1_duty_set(calc_speed_motor(left_trigger_pos));
    // ...
}

/* handles right Trigger (RT) position */
static void check_controller_trigger_right(uint16_t right_trigger_pos) {
    printf("RT: %d%\n",right_trigger_pos);
    //pwm1_duty_set(calc_speed_motor(left_trigger_pos));
    // ...
}

static float calc_speed_motor(uint16_t value) {
    return (0.124121 * value + 60.1);
}

/* handles directional-pad (Dpad) button presses */
static void check_controller_dpad(uint8_t packet15) {
    if(packet15 == DPAD_UP) {
        printf("dpad Up pressed\n");
    }
    if(packet15 == DPAD_RIGHT) {
        printf("dpad Right pressed\n");
    }
    if(packet15 == DPAD_DOWN) {
        printf("dpad Down pressed\n");
    }
    if(packet15 == DPAD_LEFT) {
        printf("dpad Left pressed\n");
    }
}

/* handles action and setting button presses */
static void check_controller_button(uint8_t packet16) {
    if(packet16 & BUTTON_A) {
        printf("button A pressed\n");
    }
    if(packet16 & BUTTON_B) {
        printf("button B pressed\n");
    }
    if(packet16 & BUTTON_X) {
        printf("button X pressed\n");
    }
    if(packet16 & BUTTON_Y) {
        printf("button Y pressed\n");
    }
    if(packet16 & BUTTON_LEFT) {
        printf("button left pressed\n");
    }
    if(packet16 & BUTTON_RIGHT) {
        printf("button right pressed\n");
    }
    if(packet16 & BUTTON_BACK) {
        printf("button back pressed\n");
    }
    if(packet16 & BUTTON_START) {
        printf("button start pressed\n");
    }
}

/* handles Joystick button push */
static void check_controller_joystick_push(uint8_t packet17) {
    if(packet17 & LEFT_STICK_PUSH) {
        printf("Left Joystick pushed\n");
    }
    if(packet17 & RIGHT_STICK_PUSH) {
        printf("Right Joystick pushed\n");
    }
}

/*
 * fills joystick data array with the respcetive bt packets
 * @return 1 if data in array changed
 */
static int fill_joystick_data(uint8_t joystick_packets[], uint8_t *packet) {
    uint32_t i, changed;
    for(i = 0, changed = 0; i < JOYSTICK_PACKET_SIZE; i++) {
        if(joystick_packets[i] != *packet) {
            joystick_packets[i] = *packet++;
            changed = 1;
        } else {
            packet++;
        }
    }
    return changed;
}

/*
 * fills trigger data array with the respcetive bt packets
 * @return 1 if data in array changed
 */
static int fill_trigger_data(uint8_t trigger_packets[], uint8_t *packet) {
    uint32_t i, changed;
    for(i = 0, changed = 0; i < TRIGGER_PACKET_SIZE; i++) {
        if(trigger_packets[i] != *packet) {
            trigger_packets[i] = *packet++;
            changed = 1;
        } else {
            packet++;
        }
    }
    return changed;
}

/* handles the controller interrupts */
static void handle_controller_interrupts(uint8_t *packet, uint16_t size) {
    static uint8_t joystick_left_packets[JOYSTICK_PACKET_SIZE], joystick_right_packets[JOYSTICK_PACKET_SIZE];
    static uint8_t trigger_left_packets[TRIGGER_PACKET_SIZE], trigger_right_packets[TRIGGER_PACKET_SIZE];
    // skip unimportant packets
    packet += 2;
    // joystick packets
    {
        // Left
        if(fill_joystick_data(joystick_left_packets, packet)) {
            uint16_t left_joy_x = joystick_left_packets[1] << 8 | joystick_left_packets[0];
            uint16_t left_joy_y = joystick_left_packets[3] << 8 | joystick_left_packets[2];
            check_controller_joystick_left_move(left_joy_x, left_joy_y);
        }
        packet += JOYSTICK_PACKET_SIZE;
        // Right
        if(fill_joystick_data(joystick_right_packets, packet)) {
            uint16_t right_joy_x = joystick_right_packets[1] << 8 | joystick_right_packets[0];
            uint16_t right_joy_y = joystick_right_packets[3] << 8 | joystick_right_packets[2];
            check_controller_joystick_right_move(right_joy_x, right_joy_y);
        }
        packet += JOYSTICK_PACKET_SIZE;
    }
    // trigger packets
    {
        // Left
        if(fill_trigger_data(trigger_left_packets, packet)) {
            uint16_t left_trigger_pos = trigger_left_packets[1] << 8 | trigger_left_packets[0];
            check_controller_trigger_left(left_trigger_pos);
        }
        packet += TRIGGER_PACKET_SIZE;
        // Right
        if(fill_trigger_data(trigger_right_packets, packet)) {
            uint16_t right_trigger_pos = trigger_right_packets[1] << 8 | trigger_right_packets[0];
            check_controller_trigger_right(right_trigger_pos);
        }
        packet += TRIGGER_PACKET_SIZE;
    }
    // push buttons
    check_controller_dpad(*packet++);
    check_controller_button(*packet++);
    check_controller_joystick_push(*packet);
}

/* Initializes all three PWM Signals  */
static void motor_pwm_init(void)
{
    {
        pwm1.gpio_num = PWM1_PIN;
        pwm1.speed_mode = LEDC_HIGH_SPEED_MODE;
        pwm1.channel = MOTOR_PWM_CHANNEL_1;
        pwm1.intr_type = LEDC_INTR_DISABLE;
        pwm1.timer_sel = MOTOR_PWM_TIMER;
        pwm1.duty = 200; // 20%
        printf("pwm1 initialized");
        
        pwm2.gpio_num = PWM2_PIN;
        pwm2.speed_mode = LEDC_HIGH_SPEED_MODE;
        pwm2.channel = MOTOR_PWM_CHANNEL_2;
        pwm2.intr_type = LEDC_INTR_DISABLE;
        pwm2.timer_sel = MOTOR_PWM_TIMER;
        pwm2.duty = 500; // 50%
        printf("pwm2 initialized");
        
        pwm3.gpio_num = PWM3_PIN;
        pwm3.speed_mode = LEDC_HIGH_SPEED_MODE;
        pwm3.channel = MOTOR_PWM_CHANNEL_3;
        pwm3.intr_type = LEDC_INTR_DISABLE;
        pwm3.timer_sel = MOTOR_PWM_TIMER;
        pwm3.duty = 800; // 80%
        printf("pwm3 initialized");
    }
    ledc_timer_config_t ledc_timer = {0};
    ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_timer.bit_num = MOTOR_PWM_BIT_NUM;
    ledc_timer.timer_num = MOTOR_PWM_TIMER;
    ledc_timer.freq_hz = PWM_FREQ; // freq -> 62 Hz
    
    ESP_ERROR_CHECK( ledc_channel_config(&pwm1) );
    ESP_ERROR_CHECK( ledc_channel_config(&pwm2) );
    ESP_ERROR_CHECK( ledc_channel_config(&pwm3) );
    ESP_ERROR_CHECK( ledc_timer_config(&ledc_timer) );
}

/* Sets the dutycicle of PWM1 */
static void pwm1_duty_set(float perc) {
    pwm1.duty = perc;
    ESP_ERROR_CHECK( ledc_channel_config(&pwm1) );
}

/* Sets the dutycicle of PWM2 */
static void pwm2_duty_set(float perc) {
    pwm2.duty = perc;
    ESP_ERROR_CHECK( ledc_channel_config(&pwm2) );
}

/* Sets the dutycicle of PWM3 */
static void pwm3_duty_set(float perc) {
    pwm3.duty = perc;
    ESP_ERROR_CHECK( ledc_channel_config(&pwm3) );
}

int btstack_main(int argc, const char * argv[]);
int btstack_main(int argc, const char * argv[]){

    (void)argc;
    (void)argv;
    
    // init
    motor_pwm_init();
    hid_host_setup();

    // parse human readable Bluetooth address
    sscanf_bd_addr(remote_addr_string, remote_addr);

    // Turn on the device 
    hci_power_control(HCI_POWER_ON);
    return 0;
}

