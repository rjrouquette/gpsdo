//
// Created by robert on 5/24/22.
//

void sendBatt(uint8_t *frame) {
    // variable bindings
    uint8_t buffer[512];
    int dlen = 0;

    sendResults(frame, buffer, dlen);
}
