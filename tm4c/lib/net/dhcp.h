//
// Created by robert on 5/20/22.
//

#ifndef GPSDO_DHCP_H
#define GPSDO_DHCP_H

#define DHCP_PORT_SRV (67)
#define DHCP_PORT_CLI (68)

void DHCP_poll();
void DHCP_renew();
void DHCP_release();

// handler for server responses
void DHCP_process(uint8_t *frame, int flen);

#endif //GPSDO_DHCP_H
