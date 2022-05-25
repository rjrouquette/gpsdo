
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
uint64_t toFixedPoint(uint32_t sec, uint32_t nano);

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
    struct timespec ts = {};

    bool hasRxTimestamp = false;
    int64_t prevOffset = 0;
    for(;;) {
        socklen_t sl = sizeof(struct sockaddr_in);
        ssize_t nread = recvfrom(fdSocket, &packetNtp, sizeof(packetNtp), 0, (struct sockaddr *) &sa, &sl);
        if (nread != sizeof(packetNtp))
            continue;

        // timestamp first response
        if(packetNtp.txTime[0] == 0) {
            clock_gettime(CLOCK_REALTIME, &ts);
            hasRxTimestamp = true;
            continue;
        }
        if(!hasRxTimestamp)
            continue;
        hasRxTimestamp = false;

        // display final response
        log("tx1: %12u.%09u", packetNtp.origTime[0], packetNtp.origTime[1]);
        log("rx1: %12u.%09u", packetNtp.rxTime[0], packetNtp.rxTime[1]);
        log("ref: %12u.%09u", packetNtp.refTime[0], packetNtp.refTime[1]);
        log("tx2: %12u.%09u", packetNtp.txTime[0], packetNtp.txTime[1]);
        log("rx2: %12lu.%09lu", ts.tv_sec, ts.tv_nsec);

        // compute delays
        auto tx1 = toFixedPoint(packetNtp.origTime[0], packetNtp.origTime[1]);
        auto rx1 = toFixedPoint(packetNtp.rxTime[0], packetNtp.rxTime[1]);
        auto tx2 = toFixedPoint(packetNtp.txTime[0], packetNtp.txTime[1]);
        auto rx2 = toFixedPoint(ts.tv_sec, ts.tv_nsec);

        int64_t delaySrv = tx2 - rx1;
        int64_t delayCli = rx2 - tx1;
        int64_t delayXfr = delayCli - delaySrv;

        log("offset 1:  %+.9f", ((double)(int32_t)(tx1 - rx1))/(1l<<32));
        log("offset 2:  %+.9f", ((double)(int32_t)(rx2 - tx2))/(1l<<32));

        log("delay client:  %+.9f", ((double)delayCli)/(1l<<32));
        log("delay server:  %+.9f", ((double)delaySrv)/(1l<<32));
        log("delay transit: %+.9f", ((double)delayXfr)/(1l<<32));

//        int64_t offset = (int32_t) (rx2 - tx2);
//        offset -= delayXfr / 2;
        int64_t offset = ((rx2 - tx2) + (tx1 - rx1)) / 2;
        log("time offset: %+.9f", ((double)offset)/(1l<<32));
        log("delta offset: %+.9f", ((double)(offset - prevOffset))/(1l<<32));
        prevOffset = offset;
    }
}

uint64_t toFixedPoint(uint32_t sec, uint32_t nano) {
    uint64_t result;
    ((uint32_t *) &result)[0] = 0;
    ((uint32_t *) &result)[1] = nano;
    result /= 1000000000;
    ((uint32_t *) &result)[1] = sec;
    return result;
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
