//
// Created by robert on 5/20/22.
//

#pragma once

#include <cstdint>

#define DHCP_PORT_SRV (67)
#define DHCP_PORT_CLI (68)

void DHCP_init();
void DHCP_renew();
uint32_t DHCP_expires();

const char* DHCP_hostname();

void DHCP_ntpAddr(uint32_t **addr, int *count);
