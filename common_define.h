#ifndef COMMON_DEFINE_H
#define COMMON_DEFINE_H

#ifndef DEFAULT_EVENT_PRIORITY
#define DEFAULT_EVENT_PRIORITY 50
#endif
#ifndef MAX_LINE_BUFFER
#define MAX_LINE_BUFFER 2048
#endif
#ifndef MAX_TCP_LISTEN
#define MAX_TCP_LISTEN 8
#endif


/* XXXX configarable */
#ifndef READ_TIMEOUT
#define READ_TIMEOUT 60
#endif
#ifndef WRITE_TIMEOUT
#define WRITE_TIMEOUT 60
#endif
#ifndef SERVER_HOST
#define SERVER_HOST "localhost"
#endif
#ifndef SERVER_PORT
#define SERVER_PORT "50000"
#endif

#define MAJOR_VERSION 0
#define MINOR_VERSION 1
#endif
