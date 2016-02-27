#ifndef FPLUG_DEVICE_H
#define FPLUG_DEVICE_H

#include "stat_store.h"

typedef struct fplug_device fplug_device_t;

/*
 * fplugデバイスインスタンスの作成
 */
int fplug_device_create(fplug_device_t **fplug_device, config_t *config, struct event_base *event_base);
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
int fplug_device_active_device_foreach(fplug_device_t *fplug_device,
     void (*foreach_cb)(const char *device_name, const char *device_address, void *cb_arg), void *cb_arg);
/*
 * fplugデバイスのアドレスからstat_storeを取得する 
 */
int fplug_device_get_stat_store(fplug_device_t *fplug_device,
     stat_store_t **stat_store, const char *device_address);

/*
 * fplugデバイスの初期化
 */
int fplug_device_reset(fplug_device_t *fplug_device, const char *device_address);
	

/*
 * fplugデバイスの日時設定
 */
int fplug_device_set_datetime(fplug_device_t *fplug_device, const char *device_address);

/*
 * fplugデバイスの電力の積算値取得 時間毎
 */
int fplug_device_get_hourly_power_total(fplug_device_t *fplug_device, const char *device_address, struct tm *start_tm,
     void (*foreach_cb)(unsigned char result, double watt, void *cb_arg), void *cb_arg);

/*
 * fplugデバイスの電力以外の情報取得 時間毎
 */
int fplug_device_get_hourly_other(fplug_device_t *fplug_device, const char *device_address, struct tm *start_tm,
    void (*foreach_cb)(unsigned char result, double temperature, unsigned int humidity, unsigned int illuminance, void *cb_arg), void *cb_arg);

#endif
