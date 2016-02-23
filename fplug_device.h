#ifndef FPLUG_DEVICE_H
#define FPLUG_DEVICE_H

#define MAX_BLUETOOTH_DEVICE 8

struct bluetooth_device {
        char device_name[32];
        char device_address[32]; 
        struct sockaddr_rc saddr;
        int sd;
        int connected;
};
typedef struct bluetooth_device bluetooth_device_t;

struct fplug_device {
        bluetooth_device_t bluetooth_device[MAX_BLUETOOTH_DEVICE];
        int available_count;
};
typedef struct fplug_device fplug_device_t;

/*
 * fplugデバイスインスタンスの作成
 */
int fplug_device_create(fplug_device_t *fplug_device, config_t *config);
/*
 * fplugデバイスに接続する
 */
int fplug_device_connect(fplug_device_t *fplug_device);
/*
 * fplugデバイスインスタンスの削除
 */
int fplug_device_destroy(fplug_device_t *fplug_device);

#endif
