#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>
#include <stddef.h>

// Raw byte chunk (pointer valid during PubSub callback only)
typedef struct {
    uint8_t *data;
    size_t len;
} raw_packet_t;

#endif
