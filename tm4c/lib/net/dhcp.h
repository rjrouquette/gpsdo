//
// Created by robert on 5/20/22.
//

#ifndef GPSDO_DHCP_H
#define GPSDO_DHCP_H

#define DHCP_PORT_SRV (67)
#define DHCP_PORT_CLI (68)

void DHCP_init();
void DHCP_run();
void DHCP_renew();

const char * DHCP_hostname();

void DHCP_ntpAddr(uint32_t **addr, int *count);

#endif //GPSDO_DHCP_H
