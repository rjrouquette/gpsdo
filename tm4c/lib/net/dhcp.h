//
// Created by robert on 5/20/22.
//

#ifndef GPSDO_DHCP_H
#define GPSDO_DHCP_H

#ifdef __cplusplus
extern "C" {
#else
#define static_assert _Static_assert
#endif

#define DHCP_PORT_SRV (67)
#define DHCP_PORT_CLI (68)

void DHCP_init();
void DHCP_renew();
uint32_t DHCP_expires();

const char * DHCP_hostname();

void DHCP_ntpAddr(uint32_t **addr, int *count);

#ifdef __cplusplus
}
#endif

#endif //GPSDO_DHCP_H
