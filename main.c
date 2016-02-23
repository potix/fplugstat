#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <event2/event.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "common_macros.h"
#include "common_define.h"
#include "fplug_device.h"
#include "config.h"
#include "logger.h"
#include "http.h"

#ifndef DEFAULT_CONFIG_FILE
#define DEFAULT_CONFIG_FILE FPLUGSTAT_PATH "/fplugstatd.conf"
#endif

struct fplugstatd {
	const char *config_file;
	int foreground;
        struct event_base *event_base;
	fplug_devicies_t fplug_devicies;
	config_t *config;
        sigset_t sig_block_mask;
        struct event *sig_term_event;
        struct event *sig_int_event;
	http_server_t *http_server;
};
typedef struct fplugstatd fplugstatd_t;

static int initialize_fplugstatd(fplugstatd_t *fplugstatd);
static int parse_command_arguments(int argc, char *argv[], fplugstatd_t *fplugstatd);
static void usage(char *command);
static void terminate(int fd, short event, void *args);

int
main(
    int argc,
    char *argv[])
{
	int error = 0;
	fplugstatd_t fplugstatd;
	char facility[LOG_FACILITY_LEN];
	char serverity[LOG_SERVERITY_LEN];
	int dev_cnt;
	char dev_section[CONFIG_LINE_BUF];

	// 構造体初期化
	if (initialize_fplugstatd(&fplugstatd)) {
		fprintf(stderr, "failed in initialize");
		return 1;
	}

	// コマンドのパース
	if (parse_command_arguments(argc, argv, &fplugstatd)) {
		usage(argv[0]);
		return 1;
	}

	// デーモン化
        if (!fplugstatd.foreground) {
                if (daemon(1,1)) {
                        fprintf(stderr, "failed in daemonaize");
                        return 1;
                }
                setsid();
        }

	// コンフィグコンテキストの生成
	if (config_create(&fplugstatd.config, fplugstatd.config_file)) {
		fprintf(stderr, "failed in create config");
		return 1;
	}

	// コンフィグの読み込み
	if (config_load(fplugstatd.config)) {
		fprintf(stderr, "failed in load config");
		return 1;
	}

	if (config_get_string(fplugstatd.config, facility, sizeof(facility), "global", "syslogFacility",  "daemon", sizeof(facility) - 1)) {
		fprintf(stderr, "failed in get facility from config");
		return 1;
	}

	if (config_get_string(fplugstatd.config, serverity, sizeof(serverity), "global", "syslogSeverity",  "warning", sizeof(serverity) - 1)) {
		fprintf(stderr, "failed in get serverity from config");
		return 1;
	}

	if (logger_open(argv[0], LOG_PID, facility)) {
		fprintf(stderr, "failed in open logger");
		return 1;
	}

	if (logger_filter(serverity)) {
		fprintf(stderr, "failed in set logger filter");
		return 1;
	}

	// bluetoothデバイス初期化
        if (fplug_device_create(&fplugstatd.fplug_device, fplugstatd.config)) {
                fprintf(stderr, "failed in initialize of fplug devicies");
		return 1;
        }

	// statistics poolの作成
	if (stat_pool_create(&fplugstatd.stat_pool, fplugstatd.event_base, fplugstatd.config)) {
		LOG(LOG_ERR, "failed in create stat pool");
		return 1;
	}

	// device poller作成
	if (device_poller_create(&fplugstatd.device_polling, fplugstatd.event_base, fplugstatd.config, &fplugstatd.fplug_devicies, &fplugstatd.stat_pool)) {
		LOG(LOG_ERR, "failed in create device poller");
		return 1;
	}

	// http server作成
	if (http_server_create(&fplugstatd.http_server, fplugstatd.event_base, fplugstatd.config, &fplugstatd.stat_pool)) {
		LOG(LOG_ERR, "failed in create http server");
		return 1;
	}


	// シグナルマスク
	sigemptyset(&fplugstatd.sig_block_mask);
	pthread_sigmask(SIG_BLOCK, &fplugstatd.sig_block_mask, NULL);

	// int, termシグナル受信時の処理イベント
	evsignal_add(fplugstatd.sig_term_event, NULL);
        evsignal_add(fplugstatd.sig_int_event, NULL);


	// bluetoothに接続
	if (connect_bluetooth_devicies(&fplugstatd.fplug_devicies)) {
		LOG(LOG_ERR, "failed in connect to bluetooth");
		return 1;
	}

	/* device pollerの開始 */
	if (device_poller_start(fplugstatd.device_poller) {
		LOG(LOG_ERR, "failed in start device poller");
		return 1;
	}

	/* http serverの開始 */
	if (http_server_start(fplugstatd.http_server)) {
		LOG(LOG_ERR, "failed in start http server");
		return 1;
	}






	
/*
	sigaddset(&fplugstatd.sigmask, SIGPIPE);
	pthread_sigmask(SIG_SETMASK, &fplugstatd.sigmask, NULL);
	signal_set(&fplugstatd.sigterm_event, SIGTERM, terminate, &fplugstatd);
	event_base_set(fplugstatd.event_base, &fplugstatd.sigterm_event);
	signal_add(&fplugstatd.sigterm_event, NULL);
	signal_set(&fplugstatd.sigint_event, SIGINT, terminate, &fplugstatd);
	event_base_set(fplugstatd.event_base, &fplugstatd.sigint_event);
	signal_add(&fplugstatd.sigint_event, NULL);
	for (i = 0; i < fplugstatd.device_count; i++) {
		if (connect_bluetooth(&fplugstatd.fplug_device[i])) {
			continue;
		}
		fplugstatd.available_count += 1;
	}
	if (fplugstatd.available_count == 0) {
		fprintf(stderr, "no there available device.");
		error = 1;
		goto last;
	}


	
	

	fplugstatd.timer_tv.tv_sec = 5;
	fplugstatd.timer_tv.tv_usec = 0;
	//evtimer_set(&fplugstatd.timer_event, statistics_main, &fplugstatd);
	event_set(&fplugstatd.timer_event, -1, EV_PERSIST, statistics_main, &fplugstatd);
	event_base_set(fplugstatd.event_base, &fplugstatd.timer_event);
	evtimer_add(&fplugstatd.timer_event, &fplugstatd.timer_tv);



	if (event_base_dispatch(fplugstatd.event_base) == -1) {
		fprintf(stderr, "failed in event base dispatch");
		error = 1;
		goto last;
	}
last:
	for (i = 0; i < fplugstatd.device_count; i++) {
		close_bluetooth(&fplugstatd.fplug_device[i]);
	}



*/




	// イベントループ
	LOG(LOG_DEBUG, "start event loop");
	event_base_dispatch(fplugstatd.event_base);
	LOG(LOG_DEBUG, "finish event loop");
	
	// 解放処理
	if (fplugstatd.sig_term_event) {
		event_free(fplugstatd.sig_term_event);
	}
	if (fplugstatd.sig_int_event) {
		event_free(fplugstatd.sig_int_event);
	}
	if (fplugstatd.http_server) {
		http_server_destroy(fplugstatd.http_server);
	}
	if (fplugstatd.device_poller) {
		device_poller_destroy(fplugstatd.device_poller);
	}
	if (fplugstatd.stat_pool) {
		stat_pool_destroy(fplugstatd.stat_pool);
	}
	if (fplugstatd.fplug_device) {
		fplug_device_destroy(&fplugstatd.fplug_devicies);
	}
	if (fplugstatd.config) {
		config_destroy(fplugstatd.config);
	}
        if (fplugstatd.event_base) {
                event_base_free(fplugstatd.event_base);
        }
	logger_close();

	return error;
}

static int
initialize_fplugstatd(
    fplugstatd_t *fplugstatd)
{
	ASSERT(fplugstatd != NULL);

	memset(fplugstatd, 0, sizeof(fplugstatd_t));
	fplugstatd->config_file = DEFAULT_CONFIG_FILE;
	fplugstatd->event_base = event_base_new();
        if ((fplugstatd->sig_term_event = evsignal_new(fplugstatd->event_base, SIGTERM, terminate, fplugstatd)) == NULL) {
		LOG(LOG_ERR, "failed in create event of SIGTERM");
		return 1;
	}
	if ((fplugstatd->sig_int_event = evsignal_new(fplugstatd->event_base, SIGINT, terminate, fplugstatd)) == NULL) {
		LOG(LOG_ERR, "failed in create event of SIGINT");
		return 1;
	}
	if (initialize_fplug_devicies(&fplugstatd->fplug_devicies)) {
		fprintf(stderr, "failed in initialize of fplug devicies");
	}

	return 0;
}

static int
parse_command_arguments(
    int argc,
    char *argv[],
    fplugstatd_t *fplugstatd)
{
	int opt;

	ASSERT(argc != 0);
	ASSERT(argv != NULL);
	ASSERT(fplugstatd != NULL);

	while ((opt = getopt(argc, argv, "c:F")) != -1) {
		switch (opt) {
		case 'c':
			fplugstatd->config_file = optarg;
			break;
		case 'F':
			fplugstatd->foreground = 1;
			break;
		default:
			return 1;
		}
	}

	return 0;
}

static void
usage(
    char *command)
{
	ASSERT(command != NULL);

	fprintf(
	    stderr,
	    "Usage: %s [-c <config_file_path] [-F]\n",
	    command);
}


static void
terminate(
    int fd,
    short event,
    void *args)
{
	fplugstatd_t *fplugstatd = args;

	ASSERT(fplugstatd != NULL);

	http_server_stop(fplugstatd->http_server);
	device_poller_stop(fplugstatd->device_poller);
	evsignal_del(fplugstatd->sig_term_event);
	evsignal_del(fplugstatd->sig_int_event);
}
