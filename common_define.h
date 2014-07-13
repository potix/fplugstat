#ifndef COMMON_DEFINE_H
#define COMMON_DEFINE_H

#ifndef MAX_DEV_ADDRLEN
#define MAX_DEV_ADDRLEN 32
#endif

#ifndef MAX_DEV_NAMELEN
#define MAX_DEV_NAMELEN 32
#endif

#ifndef DEFAULT_CNTRL_ADDR
#define DEFAULT_CNTRL_ADDR "localhost"
#endif

#ifndef DEFAULT_CNTRL_PORT
#define DEFAULT_CNTRL_PORT "58000"
#endif

#ifndef MAX_CNTRL_REQ_BUFF
#define MAX_CNTRL_REQ_BUFF 65535
#endif

#ifndef MAX_CNTRL_RES_BUFF
#define MAX_CNTRL_RES_BUFF 65535
#endif

#ifndef MAX_CNTRL_LINE_BUFFER
#define MAX_CNTRL_LINE_BUFFER 2048
#endif

#ifndef DEFAULT_EVENT_PRIORITY
#define DEFAULT_EVENT_PRIORITY 50
#endif

#ifndef MAX_TCP_LISTEN
#define MAX_TCP_LISTEN 8
#endif

#ifndef READ_TIMEOUT
#define READ_TIMEOUT 60
#endif

#ifndef WRITE_TIMEOUT
#define WRITE_TIMEOUT 60
#endif

#define MAX_FPLUG_DEVICE 7

#define TYPE_DEV_NAME 1
#define TYPE_DEV_ADDR 2

#define MAJOR_VERSION 0
#define MINOR_VERSION 1

#endif
