
#include <arpa/inet.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <netinet/in.h>
#include <sysexits.h>
#include <thread>
#include <unistd.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

#define NTP_PORT (123)

struct PACKED PACKET_NTPv3 {
        union PACKED {
            struct PACKED {
                unsigned mode: 3;
                unsigned version: 3;
                unsigned status: 2;
            };
            uint8_t bits;
        } flags;
        uint8_t stratum;
        uint8_t poll;
        int8_t precision;
        uint32_t rootDelay;
        uint32_t rootDispersion;
        uint8_t refID[4];
        uint32_t refTime[2];
        uint32_t origTime[2];
        uint32_t rxTime[2];
        uint32_t txTime[2];
};
static_assert(sizeof(struct PACKET_NTPv3) == 48, "FRAME_NTPv3 must be 48 bytes");

int fdSocket;

void log(const char *format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
void workerRX();

int main(int argc, char **argv) {
    fdSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    static struct sockaddr_in cliAddr = {}, srvAddr = {};
    bzero(&cliAddr, sizeof(struct sockaddr_in));
    bzero(&srvAddr, sizeof(struct sockaddr_in));
    cliAddr.sin_family = AF_INET;

    srvAddr.sin_port = htons(NTP_PORT);
    inet_aton(argv[1], &srvAddr.sin_addr);

    if((bind(fdSocket, (struct sockaddr *) &cliAddr, sizeof(cliAddr))))
        return EX_IOERR;

    socklen_t slen = sizeof(cliAddr);
    if(getsockname(fdSocket, (struct sockaddr *) &cliAddr, &slen))
        return EX_OSERR;

    log("listening on %s:%d", inet_ntoa(cliAddr.sin_addr), cliAddr.sin_port);

    std::thread rxThread(workerRX);

    struct timespec ts = {};
    PACKET_NTPv3 packetNtp = {};
    packetNtp.flags.version = 0;
    packetNtp.flags.mode = 3;
    for(;;) {
        clock_gettime(CLOCK_REALTIME, &ts);
        packetNtp.txTime[0] = ts.tv_sec;
        packetNtp.txTime[1] = ts.tv_nsec;
        auto len = ::sendto(
                fdSocket,
                &packetNtp,
                sizeof(packetNtp),
                0,
                (sockaddr *) &srvAddr,
                sizeof(srvAddr)
        );
        log("sendto: %ld bytes", len);
        sleep(1);
    }

    close(fdSocket);
    return 0;
}

void workerRX() {
    struct sockaddr_in sa = {};
    PACKET_NTPv3 packetNtp = {};

    for(;;) {
        socklen_t sl = sizeof(struct sockaddr_in);
        ssize_t nread = recvfrom(fdSocket, &packetNtp, sizeof(packetNtp), 0, (struct sockaddr *) &sa, &sl);
        if (nread != sizeof(packetNtp))
            continue;

        log("recvfrom: %ld bytes", nread);
        log("orig: %u.%9u", packetNtp.origTime[0], packetNtp.origTime[1]);
        log("rx:   %u.%9u", packetNtp.rxTime[0], packetNtp.rxTime[1]);
        log("ref:  %u.%9u", packetNtp.refTime[0], packetNtp.refTime[1]);
        log("tx:   %u.%9u", packetNtp.txTime[0], packetNtp.txTime[1]);
    }
}


std::mutex mutexLog;
void log(const char *format, ...) {
    va_list argptr;
    va_start(argptr, format);

    std::unique_lock lock(mutexLog);
    vfprintf(stdout, format, argptr);
    fwrite("\n", 1, 1, stdout);
    fflush(stdout);
    lock.unlock();

    va_end(argptr);
}
