#include "netdev.h"

static netdev_t *devices[MAX_NETDEV];
static int netdev_count = 0;
static netdev_t *default_ndev = NULL; 

void netdev_register(netdev_t* dev)
{
    if(netdev_count >= MAX_NETDEV) return;

    devices[netdev_count++] = dev;
    if(default_ndev == NULL) default_ndev = dev;

    if(dev->init) dev->init(dev);
}

netdev_t* netdev_get_default()
{
    return default_ndev;
}

void netdev_send(uint8_t* frame, size_t len)
{
    if(default_ndev && default_ndev->send) default_ndev->send(default_ndev, frame, len);
}

void netdev_poll()
{
    for(int i = 0 ; i < netdev_count ; i++)
    {
        if(devices[i]->poll) devices[i]->poll(devices[i]);
    }
}