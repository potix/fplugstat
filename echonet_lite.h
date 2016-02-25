#ifndef ECHONET_LITE_H
#define ECHONET_LITE_H

#define MAX_ENL_FRAME 65535

struct enl_request_frame_info {
        unsigned char buffer[MAX_ENL_FRAME];
        size_t frame_len;
        unsigned char opc;
};
typedef struct enl_request_frame_info enl_request_frame_info_t;

struct enl_response_frame_info {
        unsigned char buffer[MAX_ENL_FRAME];
        size_t frame_len;
        unsigned char opc;
        unsigned char epc_ready;
};
typedef struct enl_response_frame_info enl_response_frame_info_t;

int enl_request_frame_init(enl_request_frame_info_t *enl_request_frame_info, unsigned char ehd2, unsigned char seojcg,
    unsigned char seojcc, unsigned char seojic, unsigned char deojcg, unsigned char deojcc, unsigned char deojic, unsigned char esv);
int enl_request_frame_add(enl_request_frame_info_t *enl_request_frame_info, unsigned char epc, unsigned char pdc, unsigned char *edt);
int enl_request_frame_get(enl_request_frame_info_t *enl_request_frame_info, unsigned char **frame, size_t *frame_len, unsigned short *tid);

int enl_response_frame_init(enl_response_frame_info_t *enl_response_frame_info, unsigned char **buffer, size_t *buffer_len);
int enl_response_frame_add(enl_response_frame_info_t *enl_response_frame_info, unsigned char **buffer, size_t *buffer_len);
int enl_response_frame_get_tid(enl_response_frame_info_t *enl_response_frame_info, unsigned short *tid);
int enl_response_frame_get_seoj(enl_response_frame_info_t *enl_response_frame_info, unsigned char *seojcg, unsigned char *seojcc, unsigned char *seojic);
int enl_response_frame_get_deoj(enl_response_frame_info_t *enl_response_frame_info, unsigned char *deojcg, unsigned char *deojcc, unsigned char *deojic);
int enl_response_frame_get_esv(enl_response_frame_info_t *enl_response_frame_info, unsigned char *esv);
int enl_response_frame_get_opc(enl_response_frame_info_t *enl_response_frame_info, unsigned char *opc);
int enl_response_frame_get_data(enl_response_frame_info_t *enl_response_frame_info,
    unsigned char opc, unsigned char *epc, unsigned char *pdc, unsigned char **edt);

#endif
