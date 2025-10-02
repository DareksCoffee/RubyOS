#ifndef __NETDEV_H
#define __NETDEV_H

#include <stddef.h>
#include <stdint.h>

// Hard codding it to 4 maximum entries
#define MAX_NETDEV          4


typedef struct netdev 
{
    char name[16];
    uint8_t mac[6];

    void (*init)(struct netdev *dev);
    void (*send)(struct netdev *dev, uint8_t *frame, size_t len);
    void (*poll)(struct netdev *dev);
} netdev_t;

#endif

void netdev_register(netdev_t *dev);
netdev_t* netdev_get_default(); 
void netdev_send(uint8_t* frame, size_t len);
void netdev_poll();