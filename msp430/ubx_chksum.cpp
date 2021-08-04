#include <cstdint>
#include <iostream>

// GNSS measurement rate
const uint8_t UBX_CFG_RATE[] = {
    0xB5, 0x62,             // sync
    0x06, 0x08, 0x06, 0x00, // header (class, id, length)
    0x32, 0x00,             // measuremnt interval (ms) 0x03E8 = 50
    0x14, 0x00,             // 20 measurements per fix
    0x01, 0x00,             // align to GPS time
    0x5B, 0x32              // Checksum
};

// PPS configuration
const uint8_t UBX_CFG_TP5[] = {
    0xB5, 0x62,             // sync
    0x06, 0x31, 0x20, 0x00, // header (class, id, length)
    0x00,                   // TIMEPULSE 0
    0x00,                   // Version 0
    0x00, 0x00,             // Reserved
    0x00, 0x00,             // antenna delay (ns)
    0x00, 0x00,             // RF group delay (ns)
    0x40, 0x42, 0x0F, 0x00, // PPS period us (no-lock) 0xF4240 = 1M
    0x40, 0x42, 0x0F, 0x00, // PPS period us (gps-lock) 0xF4240 = 1M
    0x40, 0xD4, 0x03, 0x00, // PPS len us (no-lock) 0x30D40 = 200k
    0xA0, 0x86, 0x01, 0x00, // PPS len us (gps-lock) 0x186A0 = 100k
    0x00, 0x00, 0x00, 0x00, // PPS edge delay
    0xF7, 0x00, 0x00, 0x00, // Flags
    0xAE, 0xB5              // Checksum
};

void checksum(const uint8_t *packet, size_t len) {
    uint8_t CK_A = 0;
    uint8_t CK_B = 0;
    for(size_t i = 2; i < len - 2; i++) {
        CK_A += packet[i];
        CK_B += CK_A;
    }
    
    std::cout << "CK_A: 0x" << std::hex << (CK_A & 0xFFu) << ((CK_A == packet[len-2])?" [PASS]":" [FAIL]") << std::endl;
    std::cout << "CK_B: 0x" << std::hex << (CK_B & 0xFFu) << ((CK_B == packet[len-1])?" [PASS]":" [FAIL]") << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "UBX_CFG_RATE" << std::endl;
    checksum(UBX_CFG_RATE, sizeof(UBX_CFG_RATE));

    std::cout << "UBX_CFG_TP5" << std::endl;
    checksum(UBX_CFG_TP5, sizeof(UBX_CFG_TP5));

    return 0;
}
