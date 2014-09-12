#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <syslog.h>
#include <event2/event.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "common_macros.h"
#include "common_define.h"
#include "fplug_device.h"
#include "config.h"
#include "logger.h"

#ifndef DEFAULT_CONFIG_FILE
//#define DEFAULT_CONFIG_FILE "/etc/fplugstatd.conf"
#define DEFAULT_CONFIG_FILE "/conf/fplugstatd.conf"
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
};
typedef struct fplugstatd fplugstatd_t;

static void initialize_fplugstatd(fplugstatd_t *fplugstatd);
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

	// 構造体初期化
	initialize_fplugstatd(&fplugstatd);

	// コマンドのパース
	if (parse_command_arguments(argc, argv, &fplugstatd)) {
		usage(argv[0]);
		return 1;
	}

	// デーモン化
        if (!fplugstatd.foreground) {
                if (daemon(1,1)) {
                        fprintf(stderr, "faile in daemonaize");
                        return 1;
                }
                setsid();
        }

	// コンフィグコンテキストの生成
	if (config_create(&fplugstatd.config, fplugstatd.config_file)) {
		fprintf(stderr, "faile in create config");
		return 1;
	}

	// コンフィグの読み込み
	if (config_load(fplugstatd.config)) {
		fprintf(stderr, "faile in load config");
		return 1;
	}

	if (config_get_string(fplugstatd.config, facility, sizeof(facility), "global", "syslogFacility",  "daemon", LOG_FACILITY_LEN - 1)) {
		fprintf(stderr, "faile in get facility from config");
		return 1;
	}

	if (config_get_string(fplugstatd.config, serverity, sizeof(serverity), "global", "syslogSeverity",  "warning", LOG_SERVERITY_LEN - 1)) {
		fprintf(stderr, "faile in get serverity from config");
		return 1;
	}

	if (logger_open(argv[0], LOG_PID, facility)) {
		fprintf(stderr, "faile in open logger");
		return 1;
	}

	if (logger_filter(serverity)) {
		fprintf(stderr, "faile in set logger filter");
		return 1;
	}

	// シグナルマスク
	sigemptyset(&fplugstatd.sig_block_mask);
	pthread_sigmask(SIG_BLOCK, &fplugstatd.sig_block_mask, NULL);

	// int, termシグナル受信時のの処理
        if ((fplugstatd.sig_term_event = evsignal_new(fplugstatd.event_base, SIGTERM, terminate, &fplugstatd)) == NULL) {
		LOG(LOG_ERR, "failed in create event of SIGTERM");
		return 1;
	};
	evsignal_add(fplugstatd.sig_term_event, NULL);

	if ((fplugstatd.sig_int_event = evsignal_new(fplugstatd.event_base, SIGINT, terminate, &fplugstatd)) == NULL) {
		LOG(LOG_ERR, "failed in create event of SIGINT");
		return 1;
	}
        evsignal_add(fplugstatd.sig_int_event, NULL);


	
	
	
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
	event_base_dispatch(fplugstatd.event_base);

	logger_close();

	// 解放処理
	if (fplugstatd.sig_term_event) {
		event_free(fplugstatd.sig_term_event);
	}
	if (fplugstatd.sig_int_event) {
		event_free(fplugstatd.sig_int_event);
	}
	if (fplugstatd.config) {
		if (config_destroy(fplugstatd.config)) {
			// XXX logging
		}
	}
	return error;
}

static void
initialize_fplugstatd(
    fplugstatd_t *fplugstatd)
{
	ASSERT(fplugstatd != NULL);

	memset(fplugstatd, 0, sizeof(fplugstatd_t));
	fplugstatd->config_file = DEFAULT_CONFIG_FILE;
	fplugstatd->event_base = event_base_new();
	initialize_fplug_devicies(&fplugstatd->fplug_devicies);
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

	evsignal_del(fplugstatd->sig_term_event);
	evsignal_del(fplugstatd->sig_int_event);
}
