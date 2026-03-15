#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>
#include <stddef.h>

// Raw DB protocol packet (pointer valid during PubSub callback only)
typedef struct {
    uint8_t *data;   // pointer to packet buffer
    size_t len;      // packet length in bytes
} db_packet_t;

#endif
