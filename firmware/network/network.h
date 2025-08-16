#ifndef NETWORK_H_
#define NETWORK_H_

typedef enum {
    NET_EVT_CONNECTED,
    NET_EVT_DISCONNECTED,
} net_evt_t;

typedef void (*net_evt_cb_t)(net_evt_t evt, void *ctx);

int network_init(void);

void network_cb_reg(net_evt_cb_t cb);

#endif /*NETWORK_H_*/