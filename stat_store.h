#ifndef STAT_STORE_H
#define STAT_STORE_H

typedef struct stat_store stat_store_t;

enum stat_type {
	TEMPERATURE = 1,
	HUMIDITY    = 2,
	ILLUMINANCE = 3,
	RWATT       = 4,
};
typedef enum stat_type stat_type_t;

/*
 * stat storeを作成する
 */
int stat_store_create(stat_store_t **stat_store, config_t *config);
/*
 * stat storeを削除する
 */
int stat_store_destroy(stat_store_t *stat_store);
/*
 * stat_storeにstatを追加する
 */
int stat_store_stat_add(stat_store_t *stat_store, time_t stat_time, double temperature, unsigned int humidity, unsigned int illuminance, double rwatt);
/*
 * stat情報をループ処理で取得する
 */
int stat_store_stat_foreach(stat_store_t *stat_store, time_t start, time_t end,
    void (*foreach_cb)(time_t stat_time, double temperature, unsigned int humidity, unsigned intilluminance, double rwatt, void *cb_arg), void *cb_arg);

#endif
