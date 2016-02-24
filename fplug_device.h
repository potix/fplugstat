#ifndef FPLUG_DEVICE_H
#define FPLUG_DEVICE_H

typedef struct fplug_device fplug_device_t;

/*
 * fplugデバイスインスタンスの作成
 */
int fplug_device_create(fplug_device_t **fplug_device, config_t *config);
/*
 * fplugデバイスに接続する
 */
int fplug_device_connect(fplug_device_t *fplug_device);
/*
 * fplugデバイスのポーリングを開始する
 */
int fplug_device_polling_start(fplug_device_t *fplug_device);
/*
 * fplugデバイスのポーリングを停止する
 */
int fplug_device_polling_stop(fplug_device_t *fplug_device);
/*
 * fplugデバイスインスタンスの削除
 */
int fplug_device_destroy(fplug_device_t *fplug_device);
/*
 * アクティブなfplugデバイスのアドレスと名前の情報をループ処理で取得する
 */
int fplug_device_active_device_foreach(fplug_device_t *fplug_device, void (*foreach_cb)(char *device_name, char *device_address, void *cb_arg), void *cb_arg);
/*
 * fplugデバイスのアドレスからstat_storeを取得する 
 */
int fplug_device_get_stat_store(fplug_device_t *fplug_device, stat_store_t *stat_store, char *device_address);

#endif
