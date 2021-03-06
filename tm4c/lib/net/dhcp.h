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

// handler for server responses
void DHCP_process(uint8_t *frame, int flen);

const char * DHCP_hostname();

#endif //GPSDO_DHCP_H
