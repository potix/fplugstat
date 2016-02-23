#ifndef FPLUG_DEVICE_H
#define FPLUG_DEVICE_H

#define MAX_FPLUG_DEVICE 7

struct fplug_device {
        char device_name[32];
        char device_address[32]; 
        struct sockaddr_rc saddr;
        int sd;
        int connected;
};
typedef struct fplug_device fplug_device_t;

struct fplug_devicies {
        fplug_device_t fplug_device[MAX_FPLUG_DEVICE];
        int available_count;
};
typedef struct fplug_devicies fplug_devicies_t;

/*
 * fplugデバイス構造体を初期化する
 */
int initialize_fplug_devicies(fplug_devicies_t *fplug_devicies);
/*
 * fplugデバイスに接続する
 */
int connect_bluetooth_devicies(fplug_devicies_t *fplug_devicies);
/*
 * fplugデバイスから切断する
 */
int close_bluetooth_devicies(fplug_devicies_t *fplug_devicies);

#endif
