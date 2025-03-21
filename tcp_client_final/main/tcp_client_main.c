/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_pdm.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_log.h"
// #include "esp_wifi.h"

int tcp_send(const uint16_t *mes, int size);
void i2s_example_pdm_rx_task(void *args);
void send_task(void *args);
// static IRAM_ATTR bool i2s_rx_queue_overflow_callback(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx);

// int send_buffer();
int doubleBuffer = 0;

uint16_t *r_buf1, *r_buf2, *c_buf;

#define EXAMPLE_PDM_RX_CLK_IO GPIO_NUM_5  // I2S PDM RX clock io number
#define EXAMPLE_PDM_RX_DIN1_IO GPIO_NUM_1 // I2S PDM RX data line1 in io number
#define EXAMPLE_PDM_RX_DIN2_IO GPIO_NUM_2 // I2S PDM RX data line2 in io number
#define EXAMPLE_PDM_RX_DIN3_IO GPIO_NUM_6 // I2S PDM RX data line2 in io number
#define EXAMPLE_PDM_RX_DIN4_IO GPIO_NUM_7 // I2S PDM RX data line2 in io number
#define TEST_PIN GPIO_NUM_8               // TEST PIN

#define EXAMPLE_PDM_RX_FREQ_HZ 48000 // I2S PDM RX frequency
#define VPU_NUM 2                   // number of GPIO connected
#define CHANNEL_NUM 1                // MONO for 1, STERO for 2

#define slot_num ((VPU_NUM * CHANNEL_NUM == 1) ? (2) : (VPU_NUM * CHANNEL_NUM))
// #define slot_num 2

#define my_dma_desc_num 8 * slot_num // fill 5 times before sending out. 10 for big desktop, 8 for windows laptop.
#define my_dma_size 4092
#define my_dma_frame_num (4092 / slot_num)
// #define read_buff_size my_dma_desc_num *my_dma_size / 7.2
// frame length 20460
#define read_buff_size my_dma_size * 5

#define GAIN 0 // left shift bits

int64_t current_timestamp;
int64_t previous_timestamp = 0;
int64_t elapsed_time = 0;
int64_t did_copy = 0;
int64_t start_time = 0;
int64_t elapsed_time_minutes = 0;

uint16_t counter_id_wifi = 0;

SemaphoreHandle_t xSemaphore_buffer = NULL;
// SemaphoreHandle_t xSemaphore_callback = NULL;

TaskHandle_t rxHandle = NULL;
TaskHandle_t sendHandle = NULL;
i2s_chan_handle_t rx_chan;

// #define ECHO_TEST_TXD (19)
// #define ECHO_TEST_RXD (20)
// #define ECHO_TEST_RTS UART_PIN_NO_CHANGE
// #define ECHO_TEST_CTS UART_PIN_NO_CHANGE

// #define ECHO_UART_PORT_NUM 0
// #define ECHO_UART_BAUD_RATE 115200

static const char *TAG = "channel";

int bytes_filled = 0;
size_t r_bytes;

void send_task(void *args)
{
    int signal_num = VPU_NUM * CHANNEL_NUM;
    uint16_t delimiter[] = {0xde, 0xad, 0xbe, 0xef, 0};
    uint16_t end[] = {0x00, 0x00};
    uint16_t *s_buf;
    uint16_t *signal_buffer[slot_num];
    for (int i = 0; i < slot_num; i++)
        signal_buffer[i] = (uint16_t *)calloc(read_buff_size / slot_num, sizeof(uint16_t));

    int error;
    while (1)
    {
        if (xSemaphoreTake(xSemaphore_buffer, portMAX_DELAY))
        {
            /* Calculate elapsed time */
            elapsed_time_minutes = (esp_timer_get_time() - start_time) / (60 * 1000 * 1000);
            s_buf = (doubleBuffer ? r_buf2 : r_buf1);
            int index[slot_num];
            memset(index, 0, slot_num * sizeof(int));
            int VPU_counter = 0;
            for (int i = 0; i < read_buff_size; i++)
            {
                signal_buffer[VPU_counter][index[VPU_counter]] = (uint16_t)s_buf[i] << GAIN;
                index[VPU_counter]++;
                VPU_counter++;
                if (VPU_counter == slot_num)
                    VPU_counter = 0;
            }
            // printf("Read Task: i2s read DIN1 size of %d with buffer %d\n-----------------------------------\n",index[0],(doubleBuffer?2:1));
            // printf("start:[0] %x [1] %x [2] %x [3] %x\n[4] %x [5] %x [6] %x [7] %x\n\n",
            // signal_buffer[0][0], signal_buffer[0][1], signal_buffer[0][2], signal_buffer[0][3], signal_buffer[0][ 4 ], signal_buffer[0][5], signal_buffer[0][6], signal_buffer[0][7]);

            // printf("Read Task: i2s read DIN2 size of %d with buffer %d\n-----------------------------------\n",index[1],(doubleBuffer?2:1));
            // printf("start:[0] %x [1] %x [2] %x [3] %x\n[4] %x [5] %x [6] %x [7] %x\n\n",
            // signal_buffer[1][0], signal_buffer[1][1], signal_buffer[1][2], signal_buffer[1][3], signal_buffer[1][ 4 ], signal_buffer[1][5], signal_buffer[1][6], signal_buffer[1][7]);
            ESP_LOGI(TAG, "timestamp: %lld", elapsed_time / 1000);

            // calculating the battery time
            // ESP_LOGI(TAG, "Elapsed time: %lld minute(s)", elapsed_time_minutes);

            if (signal_num == 1)
            {
                error = tcp_send(delimiter, 5);
                while (error < 0)
                    error = tcp_send(delimiter, 5);
                // error = tcp_send(counter_id_wifi, 1);
                // while (error < 0)
                //     error = tcp_send(counter_id_wifi, 1);

                error = tcp_send(signal_buffer[0], read_buff_size / slot_num);
                while (error < 0)
                    error = tcp_send(signal_buffer[0], read_buff_size / slot_num);
                end[1] = 0x01;
                error = tcp_send(end, 2);
                while (error < 0)
                    error = tcp_send(end, 2);
                delimiter[4]++;
                if (delimiter[4] >= read_buff_size)
                    delimiter[4] = 0;
            }
            else
            {
                for (int i = 0; i < slot_num; i++)
                {
                    error = tcp_send(delimiter, 5);
                    while (error < 0)
                        error = tcp_send(delimiter, 5);
                    error = tcp_send(signal_buffer[i], read_buff_size / slot_num);
                    while (error < 0)
                        error = tcp_send(signal_buffer[i], read_buff_size / slot_num);

                    // if not constrain the channel order, only set the end[1] = 0x01 is enough.
                    if (i == slot_num - 1)
                        end[1] = 0x01;
                    error = tcp_send(end, 2);
                    while (error < 0)
                        error = tcp_send(end, 2);
                    end[0] = i;
                    // ESP_LOGI(TAG, "End[0]: %u, End[1]: %u, slot_num: %u", end[0], end[1], slot_num);
                    // if constrain the channel order, send the slot_number as the index of the channel number.
                    delimiter[4]++;
                    if (delimiter[4] >= read_buff_size)
                        delimiter[4] = 0;
                }
            }
        }
        ///////////////////////////////////////////////////////////////////
    }

    vTaskDelete(NULL);
}

static i2s_chan_handle_t i2s_example_init_pdm_rx()
{
    i2s_chan_handle_t rx_chan;
    i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    rx_chan_cfg.dma_frame_num = my_dma_frame_num;
    rx_chan_cfg.dma_desc_num = my_dma_desc_num;
    ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, NULL, &rx_chan));

    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(EXAMPLE_PDM_RX_FREQ_HZ),
        /* The data bit-width of PDM mode is fixed to 16 */
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = EXAMPLE_PDM_RX_CLK_IO,
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };
    // clk rate = sample rate * 64
    if (CHANNEL_NUM == 2)
        pdm_rx_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;

    switch (VPU_NUM)
    {

    case (1):
        // 1 DIN 1 CHANNEL total 1 VPU
        pdm_rx_cfg.gpio_cfg.din = EXAMPLE_PDM_RX_DIN1_IO;
        pdm_rx_cfg.slot_cfg.slot_mask = I2S_PDM_SLOT_LEFT;
        // 1 DIN 2 CHANNEL total 2 VPU
        // if (CHANNEL_NUM ==2)
        pdm_rx_cfg.slot_cfg.slot_mask = I2S_PDM_SLOT_BOTH;
        break;
    case (2):
        // 2 DIN 1 CHANNEL total 2 VPU
        pdm_rx_cfg.slot_cfg.slot_mask = (I2S_PDM_RX_LINE1_SLOT_LEFT | I2S_PDM_RX_LINE0_SLOT_LEFT);
        // 2 DIN 2 CHANNEL total 4 VPU
        if (CHANNEL_NUM == 2)
            pdm_rx_cfg.slot_cfg.slot_mask = (I2S_PDM_RX_LINE1_SLOT_LEFT | I2S_PDM_RX_LINE0_SLOT_LEFT | I2S_PDM_RX_LINE1_SLOT_RIGHT | I2S_PDM_RX_LINE0_SLOT_RIGHT);

        pdm_rx_cfg.gpio_cfg.dins[0] = EXAMPLE_PDM_RX_DIN1_IO;
        pdm_rx_cfg.gpio_cfg.dins[1] = EXAMPLE_PDM_RX_DIN2_IO;

        break;
    case (3):
        // 3 DIN 1 CHANNEL total 3 VPU
        pdm_rx_cfg.slot_cfg.slot_mask = (I2S_PDM_RX_LINE2_SLOT_LEFT | I2S_PDM_RX_LINE1_SLOT_LEFT | I2S_PDM_RX_LINE0_SLOT_LEFT);
        // 3 DIN 2 CHANNEL total 6 VPU
        if (CHANNEL_NUM == 2)
            pdm_rx_cfg.slot_cfg.slot_mask = (I2S_PDM_RX_LINE2_SLOT_LEFT | I2S_PDM_RX_LINE1_SLOT_LEFT | I2S_PDM_RX_LINE0_SLOT_LEFT | I2S_PDM_RX_LINE2_SLOT_RIGHT | I2S_PDM_RX_LINE1_SLOT_RIGHT | I2S_PDM_RX_LINE0_SLOT_RIGHT);

        pdm_rx_cfg.gpio_cfg.dins[0] = EXAMPLE_PDM_RX_DIN1_IO;
        pdm_rx_cfg.gpio_cfg.dins[1] = EXAMPLE_PDM_RX_DIN2_IO;
        pdm_rx_cfg.gpio_cfg.dins[2] = EXAMPLE_PDM_RX_DIN3_IO;
        break;
    case (4):
        // 4 DIN 1 CHANNEL total 4 VPU
        pdm_rx_cfg.slot_cfg.slot_mask = (I2S_PDM_RX_LINE3_SLOT_LEFT | I2S_PDM_RX_LINE2_SLOT_LEFT | I2S_PDM_RX_LINE1_SLOT_LEFT | I2S_PDM_RX_LINE0_SLOT_LEFT);
        // 4 DIN 2 CHANNEL total 8 VPU
        if (CHANNEL_NUM == 2)
            pdm_rx_cfg.slot_cfg.slot_mask = (I2S_PDM_RX_LINE3_SLOT_LEFT | I2S_PDM_RX_LINE2_SLOT_LEFT | I2S_PDM_RX_LINE1_SLOT_LEFT | I2S_PDM_RX_LINE0_SLOT_LEFT | I2S_PDM_RX_LINE3_SLOT_RIGHT | I2S_PDM_RX_LINE2_SLOT_RIGHT | I2S_PDM_RX_LINE1_SLOT_RIGHT | I2S_PDM_RX_LINE0_SLOT_RIGHT);
        pdm_rx_cfg.gpio_cfg.dins[0] = EXAMPLE_PDM_RX_DIN1_IO;
        pdm_rx_cfg.gpio_cfg.dins[1] = EXAMPLE_PDM_RX_DIN2_IO;
        pdm_rx_cfg.gpio_cfg.dins[2] = EXAMPLE_PDM_RX_DIN3_IO;
        pdm_rx_cfg.gpio_cfg.dins[3] = EXAMPLE_PDM_RX_DIN4_IO;
        break;
    default:
        pdm_rx_cfg.gpio_cfg.din = EXAMPLE_PDM_RX_DIN1_IO;
        pdm_rx_cfg.slot_cfg.slot_mask = I2S_PDM_SLOT_LEFT;
        break;
    }
    // CLK_FREQUNCY = SAMPLE_RATE * 64 for I2S_PDM_DSR_8S
    // CLK_FREQUNCY = SAMPLE_RATE * 128 for I2S_PDM_DSR_16S

    // pdm_rx_cfg.clk_cfg.dn_sample_mode = I2S_PDM_DSR_16S;
    pdm_rx_cfg.clk_cfg.dn_sample_mode = I2S_PDM_DSR_8S;

    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_chan, &pdm_rx_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
    return rx_chan;
}

void i2s_example_pdm_rx_task(void *args)
{

    i2s_chan_handle_t rx_chan = i2s_example_init_pdm_rx();
    // i2s_chan_handle_t rx_chan2 = i2s_example_init_pdm_rx(2);

    size_t r_bytes = 0;

    while (1)
    {

        /* Read i2s data */
        if (i2s_channel_read(rx_chan, c_buf, read_buff_size * 2, &r_bytes, 1000) == ESP_OK)
        {
            gpio_set_level(TEST_PIN, 1);
            doubleBuffer = 1 - doubleBuffer;
            c_buf = (doubleBuffer ? r_buf1 : r_buf2);
            // send data until success
            did_copy = 1;
            xSemaphoreGive(xSemaphore_buffer);
            gpio_set_level(TEST_PIN, 0);
        }
        else
        {
        }
        if (did_copy == 1)
        {
            elapsed_time = current_timestamp - previous_timestamp; // Time elapsed in microseconds
            previous_timestamp = current_timestamp;
            current_timestamp = esp_timer_get_time();
            did_copy = 0;
        }
    }
    free(r_buf1);
    free(r_buf2);

    vTaskDelete(NULL);
}

void app_main(void)
{
    /* Start the timer */
    start_time = esp_timer_get_time();

    gpio_reset_pin(TEST_PIN);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(TEST_PIN, GPIO_MODE_OUTPUT);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    r_buf1 = (uint16_t *)calloc(read_buff_size, sizeof(uint16_t));
    r_buf2 = (uint16_t *)calloc(read_buff_size, sizeof(uint16_t));
    assert(r_buf1);
    assert(r_buf2);

    c_buf = r_buf1;

    xSemaphore_buffer = xSemaphoreCreateBinary();

    // esp_wifi_set_max_tx_power(*power);
    // receive task have higher priority than send task
    xTaskCreatePinnedToCore(i2s_example_pdm_rx_task, "i2s_example_pdm_rx_task", 4096, NULL, 5, &rxHandle, 1);
    xTaskCreatePinnedToCore(send_task, "send_task", 4096, NULL, 3, &sendHandle, 0);
}
