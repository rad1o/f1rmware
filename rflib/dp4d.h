#ifndef _DP4D_H
#define _DP4D_H

struct dp4d_pkg_s {
    uint8_t version;
    uint8_t button;
    uint16_t x;
    uint16_t y;
    uint32_t id;
} __attribute__((packed));
typedef struct dp4d_pkg_s dp4d_pkg;

struct dp4d_client_s {
    uint32_t id;
    uint32_t lifetime;
};
typedef struct dp4d_client_s dp4d_client;

#endif // _DP4D_H
