#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "common_macros.h"
#include "common_define.h"
#include "logger.h"
#include "echonet_lite.h"

/*
 * echonet lite specification 
 * http://echonet.jp/wp/wp-content/uploads/pdf/General/Standard/ECHONET_lite_V1_12_jp/ECHONET-Lite_Ver.1.12_02.pdf
 */
struct enl_frame_hdr {
        unsigned char ehd1; /* always 0x10 */
        unsigned char ehd2; /* 0x81 or 0x82 */
        unsigned short tid; /* transaction id */
}__attribute__((__packed__));
typedef struct enl_frame_hdr enl_frame_hdr_t;

struct enl_frame_edata {
        unsigned char seojcg; /* source echonet lite object class group code */
        unsigned char seojcc; /* source echonet lite object class code */
        unsigned char seojic; /* source echonet lite object instance code */
        unsigned char deojcg; /* destination echonet lite object class group code */
        unsigned char deojcc; /* destination echonet lite object class code */
        unsigned char deojic; /* destination echonet lite object instance code */
        unsigned char esv; /* echonet lite service */
        unsigned char opc; /* operation property count */
	/* repeat epc pdc edt */
}__attribute__((__packed__));
typedef struct enl_frame_edata enl_frame_edata_t;

struct enl_frame {
	enl_frame_hdr_t ef_hdr;
	enl_frame_edata_t ef_edata;
}__attribute__((__packed__));
typedef struct enl_frame enl_frame_t;

#define ENL_REGULATION_FRAME_COMMON_LEN sizeof(enl_frame_t)
#define ENL_ANY_FRAME_COMMON_LEN sizeof(enl_frame_hdr_t)

enum enl_class_group_code {
	ENL_CG_SENSOR       = 0x00,
	ENL_CG_AIRCONDITION = 0x01,
	ENL_CG_HOUSING      = 0x02,
	ENL_CG_HOUSEWORK    = 0x03,
	ENL_CG_HEALTH       = 0x04,
	ENL_CG_MANAGEMENT   = 0x05,
	ENL_CG_AUDIOVISUAL  = 0x06,
	ENL_CG_PROFILE      = 0x0E,
	ENL_CG_USERDEFINED  = 0x0E,
};
typedef enum enl_class_group_code enl_class_group_code_t;

enum enl_profile_class_code {
	ENL_PROFILE_CC_NODEPROFILE = 0x0F
};
typedef enum enl_profile_class_code enl_profile_class_code_t;

/*
 * other class code specification
 * http://echonet.jp/wp/wp-content/uploads/pdf/General/Standard/Echonet/Version_3_21/SpecVer341_ap_b.pdf
 */

enum enl_service_code {
	ENL_SETI_SNA   = 0x50, /* プロパティ値書き込み要求不可応答 */
	ENL_SETC_SNA   = 0x51, /* プロパティ値書き込み要求不可応答 */
	ENL_GET_SNA    = 0x52, /* プロパティ値読み出し不可応答 */
	ENL_INF_SNA    = 0x53, /* プロパティ値通知不可応答   */
	ENL_SETGET_SNA = 0x5E, /* プロパティ値書き込み・読み出し不可応答 */

	ENL_SETI       = 0x60, /* プロパティ値書き込み要求（応答不要） */
	ENL_SETC       = 0x61, /* プロパティ値書き込み要求（応答要） */
	ENL_GET        = 0x62, /* プロパティ値読み出し要求 */
	ENL_INF_REQ    = 0x63, /* プロパティ値通知要求  */
	ENL_SETGET     = 0x6E, /* プロパティ値書き込み・読み出し要求  */

	ENL_SET_RES    = 0x71, /* プロパティ値書き込み応答 */
	ENL_GET_RES    = 0x72, /* プロパティ値読み出し応答 */
	ENL_INF        = 0x73, /* プロパティ値通知  */
	ENL_INFC       = 0x64, /* プロパティ値通知（応答要）   */
	ENL_INFC_RES   = 0x7A, /* プロパティ値通知応答  */
	ENL_SETGET_RES = 0x7E, /* プロパティ値書き込み・読み出し応答 */
};
typedef enum enl_service_code enl_service_code_t;

static int get_transaction_id(void);

unsigned short g_tid;

static int
get_transaction_id(void) {
	return g_tid++;
}

int
enl_request_frame_init(
    enl_request_frame_info_t *enl_request_frame_info,
    unsigned char seojcg, 
    unsigned char seojcc,
    unsigned char seojic,
    unsigned char deojcg,
    unsigned char deojcc,
    unsigned char deojic,
    unsigned char esv)
{
	enl_frame_t *ef;

	if (enl_request_frame_info == NULL) {
		errno = EINVAL;
		return 1;
	}

	// frameの初期化
	memset(enl_request_frame_info->buffer, 0, ENL_REGULATION_FRAME_COMMON_LEN);
	enl_request_frame_info->frame_len = ENL_REGULATION_FRAME_COMMON_LEN;
	enl_request_frame_info->opc = 0;
	ef = (enl_frame_t *)enl_request_frame_info->buffer;
	ef->ef_hdr.ehd1 = 0x10;
	ef->ef_hdr.ehd2 = 0x81;
	ef->ef_edata.seojcg = seojcg;
	ef->ef_edata.seojcc = seojcc;
	ef->ef_edata.seojic = seojic;
	ef->ef_edata.deojcg = deojcg;
	ef->ef_edata.deojcc = deojcc;
	ef->ef_edata.deojic = deojic;
	ef->ef_edata.esv = esv;

	return 0;
}

int
enl_request_frame_add(
    enl_request_frame_info_t *enl_request_frame_info,
    unsigned char epc,
    unsigned char pdc,
    unsigned char *edt)
{
	if (enl_request_frame_info == NULL ||
	    (pdc > 0 && edt == NULL)) {
		errno = EINVAL;
		return 1;
	}
	if (enl_request_frame_info->frame_len + 1 + 1 + pdc >= MAX_ENL_FRAME_LEN) {
		errno = ENOMEM;
		return 1;
	}
	enl_request_frame_info->buffer[enl_request_frame_info->frame_len] = epc;
	enl_request_frame_info->frame_len++;
	enl_request_frame_info->buffer[enl_request_frame_info->frame_len] = pdc;
	enl_request_frame_info->frame_len++;
	if (pdc > 0) {
		memcpy(&enl_request_frame_info->buffer[enl_request_frame_info->frame_len], edt, pdc);
		enl_request_frame_info->frame_len += pdc;
	}
	enl_request_frame_info->opc++;

	return 0;
}

int
enl_request_frame_get(
    enl_request_frame_info_t *enl_request_frame_info,
    unsigned char **frame,
    size_t *frame_len,
    unsigned short *tid)
{
	enl_frame_t *ef;

	if (enl_request_frame_info == NULL ||
	    frame == NULL ||
	    frame_len == NULL) {
		errno = EINVAL;
		return 1;
	}

	ef = (enl_frame_t *)enl_request_frame_info->buffer;
	ef->ef_hdr.tid = get_transaction_id();
	ef->ef_edata.opc = enl_request_frame_info->opc;
	*frame = enl_request_frame_info->buffer;
	*frame_len = enl_request_frame_info->frame_len;
	if (tid) {
		*tid = ef->ef_hdr.tid;
	}

	return 0;
}

int
enl_response_frame_init(
    enl_response_frame_info_t *enl_response_frame_info,
    unsigned char **buffer,
    size_t *buffer_len)
{

	if (enl_response_frame_info == NULL || 
	    buffer == NULL || 
	    buffer_len == NULL) {
		errno = EINVAL;
		return 1;
	}
	enl_response_frame_info->frame_len = ENL_REGULATION_FRAME_COMMON_LEN;
	enl_response_frame_info->epc_ready = 1;
	*buffer = enl_response_frame_info->buffer;
	*buffer_len = ENL_REGULATION_FRAME_COMMON_LEN;
	
	return 0;
}

int
enl_response_frame_add(
    enl_response_frame_info_t *enl_response_frame_info,
    unsigned char **buffer,
    size_t *buffer_len)
{
	enl_frame_t *ef;
	unsigned char latest_pdc;

	if (enl_response_frame_info == NULL || 
	    buffer == NULL || 
	    buffer_len == NULL ||
	    enl_response_frame_info->frame_len < ENL_REGULATION_FRAME_COMMON_LEN) {
		errno = EINVAL;
		return 1;
	}
	if (enl_response_frame_info->frame_len == ENL_REGULATION_FRAME_COMMON_LEN) {
		ef = (enl_frame_t *)enl_response_frame_info->buffer;
		enl_response_frame_info->opc = ef->ef_edata.opc;
	}
	if (enl_response_frame_info->opc != 0) {
		if (enl_response_frame_info->epc_ready) {
			*buffer = &enl_response_frame_info->buffer[enl_response_frame_info->frame_len];
			*buffer_len = 2; /* epc + pdc */
			enl_response_frame_info->frame_len += 2; /* epc + pdc */
			enl_response_frame_info->epc_ready = 0;
		} else {
			latest_pdc = enl_response_frame_info->buffer[enl_response_frame_info->frame_len - 1];
			*buffer = &enl_response_frame_info->buffer[enl_response_frame_info->frame_len];
			*buffer_len = latest_pdc;
			enl_response_frame_info->frame_len += latest_pdc;
			enl_response_frame_info->epc_ready = 1;
			enl_response_frame_info->opc--;
		}
	} else {
		*buffer = NULL;
		*buffer_len = 0;
	}

	return 0;
}

int
enl_response_frame_get_tid(
    enl_response_frame_info_t *enl_response_frame_info,
    unsigned short *tid) 
{
	enl_frame_t *ef;

	if (enl_response_frame_info == NULL || 
	    tid == NULL) {
		errno = EINVAL;
		return 1;
	}

	ef = (enl_frame_t *)enl_response_frame_info->buffer;
	*tid = ef->ef_hdr.tid;

	return 0;
}

int
enl_response_frame_get_seoj(
    enl_response_frame_info_t *enl_response_frame_info,
    unsigned char *seojcg,
    unsigned char *seojcc,
    unsigned char *seojic)
{
	enl_frame_t *ef;

	if (enl_response_frame_info == NULL || 
	    seojcg == NULL ||
	    seojcc == NULL ||
	    seojic == NULL) {
		errno = EINVAL;
		return 1;
	}

	ef = (enl_frame_t *)enl_response_frame_info->buffer;
	*seojcg = ef->ef_edata.seojcg;
	*seojcc = ef->ef_edata.seojcc;
	*seojic = ef->ef_edata.seojic;

	return 0;
}

int
enl_response_frame_get_deoj(
    enl_response_frame_info_t *enl_response_frame_info,
    unsigned char *deojcg,
    unsigned char *deojcc,
    unsigned char *deojic)
{
	enl_frame_t *ef;

	if (enl_response_frame_info == NULL || 
	    deojcg == NULL ||
	    deojcc == NULL ||
	    deojic == NULL) {
		errno = EINVAL;
		return 1;
	}

	ef = (enl_frame_t *)enl_response_frame_info->buffer;
	*deojcg = ef->ef_edata.deojcg;
	*deojcc = ef->ef_edata.deojcc;
	*deojic = ef->ef_edata.deojic;

	return 0;
}

int
enl_response_frame_get_esv(
    enl_response_frame_info_t *enl_response_frame_info,
    unsigned char *esv)
{
	enl_frame_t *ef;

	if (enl_response_frame_info == NULL || 
	    esv == NULL) {
		errno = EINVAL;
		return 1;
	}

	ef = (enl_frame_t *)enl_response_frame_info->buffer;
	*esv = ef->ef_edata.esv;

	return 0;
}

int
enl_response_frame_get_opc(
    enl_response_frame_info_t *enl_response_frame_info,
    unsigned char *opc)
{
	enl_frame_t *ef;

	if (enl_response_frame_info == NULL || 
	    opc == NULL) {
		errno = EINVAL;
		return 1;
	}

	ef = (enl_frame_t *)enl_response_frame_info->buffer;
	*opc = ef->ef_edata.opc;

	return 0;

}

int
enl_response_frame_get_data(
    enl_response_frame_info_t *enl_response_frame_info, 
    unsigned char idx,
    unsigned char *epc,
    unsigned char *pdc,
    unsigned char **edt_ptr)
{
	enl_frame_t *ef;
	unsigned char *handle_ptr;
	unsigned char latest_epc = 0;
	unsigned char latest_pdc = 0;
	unsigned char *latest_edt_ptr = NULL;
	int i;

	if (enl_response_frame_info == NULL || 
	    epc == NULL ||
	    pdc == NULL ||
	    edt_ptr == NULL ||
	    idx == 0) {
		errno = EINVAL;
		return 1;
	}

	ef = (enl_frame_t *)enl_response_frame_info->buffer;
	if (idx > ef->ef_edata.opc) {
		errno = ENOENT;
		return 1;
	}

	handle_ptr = &enl_response_frame_info->buffer[ENL_REGULATION_FRAME_COMMON_LEN];
	for (i = 0; i < idx; i++) {
		latest_epc = *handle_ptr;
		latest_pdc = *(handle_ptr + 1);
		latest_edt_ptr = handle_ptr + 2;
		handle_ptr += 2 /* epc + pdc */ + latest_pdc;
	}	
	*epc = latest_epc;
	*pdc = latest_pdc;
	*edt_ptr = latest_edt_ptr;

	return 0;
}

int
enl_request_any_frame_init(
    enl_request_any_frame_info_t *enl_request_any_frame_info,
    unsigned char *edata,
    size_t edata_len)
{
	enl_frame_hdr_t *ef_hdr;

	if (enl_request_any_frame_info == NULL) {
		errno = EINVAL;
		return 1;
	}

	// frameの初期化
	memset(enl_request_any_frame_info->buffer, 0, ENL_ANY_FRAME_COMMON_LEN);
	enl_request_any_frame_info->frame_len = ENL_ANY_FRAME_COMMON_LEN + edata_len;
	ef_hdr = (enl_frame_hdr_t *)enl_request_any_frame_info->buffer;
	ef_hdr->ehd1 = 0x10;
	ef_hdr->ehd2 = 0x82;
	if (edata_len > 0) {
		memcpy(&enl_request_any_frame_info->buffer[ENL_ANY_FRAME_COMMON_LEN], edata, edata_len);
	}

	return 0;
}

int
enl_request_any_frame_get(
    enl_request_any_frame_info_t *enl_request_any_frame_info,
    unsigned char **frame,
    size_t *frame_len, unsigned short *tid)
{
	enl_frame_hdr_t *ef_hdr;

	if (enl_request_any_frame_info == NULL ||
	    frame == NULL ||
	    frame_len == NULL) {
		errno = EINVAL;
		return 1;
	}

	ef_hdr = (enl_frame_hdr_t *)enl_request_any_frame_info->buffer;
	ef_hdr->tid = get_transaction_id();
	*frame = enl_request_any_frame_info->buffer;
	*frame_len = enl_request_any_frame_info->frame_len;
	if (tid) {
		*tid = ef_hdr->tid;
	}

	return 0;
}

/* 任意レスポンスフレーム処理 */
int
enl_response_any_frame_init(
    enl_response_any_frame_info_t *enl_response_any_frame_info,
    size_t edata_len,
    unsigned char **buffer,
    size_t *buffer_len)
{
	if (enl_response_any_frame_info == NULL || 
	    buffer == NULL || 
	    buffer_len == NULL) {
		errno = EINVAL;
		return 1;
	}
	if (ENL_ANY_FRAME_COMMON_LEN + edata_len > MAX_ENL_FRAME_LEN) {
		errno = ENOMEM;
		return 1;
	}

	enl_response_any_frame_info->frame_len = ENL_ANY_FRAME_COMMON_LEN + edata_len;
	enl_response_any_frame_info->edata_len = edata_len;
	*buffer = enl_response_any_frame_info->buffer;
	*buffer_len = enl_response_any_frame_info->frame_len;

	return 0;
}

int
enl_response_any_frame_get_tid(
    enl_response_any_frame_info_t *enl_response_any_frame_info,
    unsigned short *tid)
{
	enl_frame_hdr_t *ef_hdr;

	if (enl_response_any_frame_info == NULL || 
	    tid == NULL) {
		errno = EINVAL;
		return 1;
	}

	ef_hdr = (enl_frame_hdr_t *)enl_response_any_frame_info->buffer;
	*tid = ef_hdr->tid;

	return 0;
}

int
enl_response_any_frame_get_edata(
    enl_response_any_frame_info_t *enl_response_any_frame_info,
    unsigned char **edata_ptr,
    size_t *edata_len)
{
	if (enl_response_any_frame_info == NULL || 
	    edata_ptr == NULL) {
		errno = EINVAL;
		return 1;
	}

	*edata_ptr = &enl_response_any_frame_info->buffer[ENL_ANY_FRAME_COMMON_LEN];
	if (edata_len) {
		*edata_len = enl_response_any_frame_info->edata_len;
	}

	return 0;
}
