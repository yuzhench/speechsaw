/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "sdkconfig.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h> // struct addrinfo
#include <arpa/inet.h>
#include "esp_netif.h"
#include "esp_log.h"
#if defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
#include "addr_from_stdin.h"
#endif
#define IP "192.168.8.196"

#define PORT 8888

static const char *TAG = "tcp connection";
static int sock = -1;

int tcp_send(const uint16_t *mes, int size)
{
    printf("sending %d data", size);
    size = size * 2;
    char host_ip[] = IP;
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;

    int err;

    if (sock < 0)
    {
        // Create socket if it doesn't exist
        struct sockaddr_in dest_addr;
        inet_pton(addr_family, host_ip, &dest_addr.sin_addr);
        dest_addr.sin_family = addr_family;
        dest_addr.sin_port = htons(PORT);

        sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            return -1;
        }

        // Disable Nagle Algorithm
        int flag = 1;
        int result = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
        if (result < 0)
        {
            ESP_LOGE(TAG, "Unable to set socket option: errno %d", errno);
            close(sock);
            return -1;
        }

        // ESP_LOGI(TAG, "Socket created");
        ESP_LOGI(TAG, "Socket created, Nagle Algorithm disabled");

        // Set timeout for connect()
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        // Attempt to connect
        inet_pton(addr_family, host_ip, &dest_addr.sin_addr);
        dest_addr.sin_family = addr_family;
        dest_addr.sin_port = htons(PORT);

        err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0)
        {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            shutdown(sock, SHUT_RDWR);
            close(sock);
            sock = -1;
            return -2;
        }
        ESP_LOGI(TAG, "Successfully connected");
    }
    // Send data
    // ESP_LOGI(TAG,"size:%d",size);
    int sent = 0;
    int sent_size = 0;

    while (sent < size)
    {
        sent_size = send(sock, mes + sent, size - sent, 0);
        sent += sent_size;
        ESP_LOGI(TAG, "%d/%d bytes sent:sent size is %d", sent, size, sent_size);
        if (sent < 0)
        {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            shutdown(sock, SHUT_RDWR);
            close(sock);
            sock = -1;
            return -3;
        }
    }

    return 1;
}
