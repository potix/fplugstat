#ifndef FPLUG_DEVICE_H
#define FPLUG_DEVICE_H

#define MAX_FPLUG_DEVICE 7

struct fplug_device {
        const char *device_address;
        struct sockaddr_rc saddr;
        int sd;
        int connected;
};
typedef struct fplug_device fplug_device_t;

struct fplug_devicies {
        fplug_device_t fplug_device[MAX_FPLUG_DEVICE];
        int device_count;
        int available_count;
};
typedef struct fplug_devicies fplug_devicies_t;

/*
 * fplugデバイス構造体を初期化する
 */
void
initialize_fplug_devicies(
    fplug_devicies_t *fplug_devicies);

#endif
