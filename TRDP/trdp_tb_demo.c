/**
 * \file trdp_tb_demo.c
 * \brief 
 * \date 28.10.2014
 * 
 * \details 
 * 
 * ========================================================================== 
 *  Licence   : Duagon Software Licence (see file 'licence.txt')
 *
 * --------------------------------------------------------------------------
 *
 *  (C) COPYRIGHT, Duagon AG, CH-8953 Dietikon, Switzerland
 *  All Rights Reserved.
 *
 * ==========================================================================
 */


/* ==================================================================================
 * Includes
 * ================================================================================*/
#include <tau_xml.h>
#include <tau_marshall.h>
#include <trdp_if.h>
#include <trdp_types.h>
#include <vos_thread.h>

#include "os_def.h"

/* ==================================================================================
 * Definitions (typedef, constants, macros)
 * ================================================================================*/
typedef struct DS1001 {
        DG_U8 a;
        DG_U8 b;
        DG_U16 c;
        DG_U32 d;
        DG_U64 e;
} DS1001;

/* ==================================================================================
 * Local/Global variables definitions
 * ================================================================================*/
static TRDP_XML_DOC_HANDLE_T xml_handle;
static TRDP_MEM_CONFIG_T mem_cfg;
static TRDP_DBG_CONFIG_T dbg_cfg;
static DG_U32 nb_com_par;
static TRDP_COM_PAR_T *p_com_par;
static DG_U32 nb_if_cfg;
static TRDP_IF_CONFIG_T *p_if_cfg;

static TRDP_PROCESS_CONFIG_T process_cfg;
static TRDP_PD_CONFIG_T pd_cfg;
static TRDP_MD_CONFIG_T md_cfg;
static DG_U32 nb_exchg_par;
static TRDP_EXCHG_PAR_T *p_exchg_par;

static DG_U32 nb_com_id;
static TRDP_COMID_DSID_MAP_T *p_com_id_ds_map;
static DG_U32 nb_dataset;
static apTRDP_DATASET_T ap_dataset;

static TRDP_MARSHALL_CONFIG_T marshall_cfg;
static TRDP_APP_SESSION_T session_handle;

static TRDP_MD_INFO_T rx_callback_msg;
static char recv_buffer[1000];
static UINT32 recv_length;
static UINT32 is_msg_received;

/* ==================================================================================
 * Local function declarations
 * ================================================================================*/

static void print_log (void *pRefCon, VOS_LOG_T category, const CHAR8 *pTime,
                const CHAR8 *pFile, UINT16 line, const CHAR8 *pMsgStr);

static DG_S32 trdp_pd_demo(
    DG_S32 *trdp_err_no);

static DG_S32 trdp_md_demo(DG_S32 *trdp_err_no);

static void md_callback (void *ref, TRDP_APP_SESSION_T apph, const TRDP_MD_INFO_T *msg, UINT8 *data, UINT32 size);

static void wait_for_msg();
/* ==================================================================================
 * Local function implementations
 * ================================================================================*/

static void wait_for_msg()
{
    while(0==is_msg_received)
    {
        tlc_process(session_handle, NULL, NULL);
        Sleep(100);
    }
    is_msg_received = 0;
}

static void print_log (void *pRefCon, VOS_LOG_T category, const CHAR8 *pTime,
                const CHAR8 *pFile, UINT16 line, const CHAR8 *pMsgStr)
{
    static const char *cat[] = { "ERR", "WAR", "INF", "DBG" };

    const char *file = strrchr(pFile, '/');
    if(category < 0)
    {
        fprintf(stderr, "%s %s:%d %s", cat[category], file ? file + 1 : pFile, line, pMsgStr);
    }
}

static DG_S32 trdp_pd_demo(DG_S32 *trdp_err_no)
{
    DG_S32 ret_val = TRDP_NO_ERR;

    TRDP_PUB_T pub_handle;
    TRDP_SUB_T sub_handle;
    TRDP_PD_INFO_T pd_info;

    DG_U32 i;
    DG_U32 size = sizeof(DS1001);
    DS1001 data_put = { 0, 0, 0, 0, 0};
    DS1001 data_get = { 0, 0, 0, 0, 0};
    DG_U32 com_id;
    DG_U32 com_id_idx =0;

    com_id = 1001;
    pd_info.comId = 1001;

    for(i=0; i<nb_exchg_par; i++)
    {
        if((p_exchg_par[i].comId == com_id) && (p_exchg_par[i].datasetId == com_id)) // com_id = dataset_id in xml file for this example
        {
            com_id_idx = i;
        }
    }

    /* publish and subscribe telegrams */
    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tlp_publish(session_handle, &pub_handle, com_id, 0, 0, 0, vos_dottedIP(*(p_exchg_par[com_id_idx].pDest[0].pUriHost)),
                p_exchg_par[com_id_idx].pPdPar->cycle, p_exchg_par[com_id_idx].pPdPar->redundant, p_exchg_par[com_id_idx].pPdPar->flags,
                &pd_cfg.sendParam, NULL, size);
    }
    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tlp_subscribe(session_handle, &sub_handle, &data_get, NULL, com_id, 0, 0, vos_dottedIP(*(p_exchg_par[com_id_idx].pSrc[0].pUriHost1)),
                0, TRDP_FLAGS_DEFAULT, pd_cfg.timeout, pd_cfg.toBehavior);
    }

    while(0!=tlp_get(session_handle, sub_handle, &pd_info,(DG_U8*)&data_get, &size))
    {
        printf(".");
        Sleep(100);
    }
    printf("\n");
    /* run main loop */
    for(i = 0;i < 100;i++)
    {
        data_put.b++;
        data_put.d++;
        size = sizeof(DS1001);
        printf("Put dataset %u, %u, %u, %u, %u\n", data_put.a, data_put.b, data_put.c, data_put.d, (DG_U32)data_put.e);
        ret_val = tlp_put(session_handle, pub_handle, (DG_U8*)&data_put, size);
        printf("RETURN: %d\n", ret_val);

        tlc_process(session_handle, NULL, NULL);

        ret_val = tlp_get(session_handle, sub_handle, &pd_info,(DG_U8*)&data_get, &size);
        printf("Got dataset %u, %u, %u, %u, %u\n", data_put.a, data_put.b, data_put.c, data_put.d, (DG_U32)data_put.e);
        printf("RETURN: %d\n", ret_val);
        printf("\n");
        Sleep(100);
    }
    /* cleanup */
    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tlp_unpublish(session_handle, pub_handle);
    }
    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tlp_unsubscribe(session_handle, sub_handle);
    }
    return ret_val;
}

static DG_S32 trdp_md_demo(DG_S32 *trdp_err_no)
{
    TRDP_LIS_T list_handle;
    DG_S32 ret_val = TRDP_NO_ERR;
    DG_CHAR8 md_buffer[1000];
    DG_U32 md_size = sizeof(md_buffer);
    printf("md_size: %d\n", md_size);

    DG_U32 user_ref = 0;
    DG_U32 com_id;

    TRDP_IP_ADDR_T src_ip = vos_dottedIP("192.168.1.97");
    TRDP_IP_ADDR_T dest_ip = vos_dottedIP("192.168.1.20");
    TRDP_URI_USER_T src_uri;
    TRDP_URI_USER_T dest_uri;
    strcpy(src_uri,"test@duagon");
    strcpy(dest_uri,"duagon@duagon");

    TRDP_UUID_T session_id;

    TRDP_FLAGS_T flags = TRDP_FLAGS_CALLBACK;

    /* Add listenere for all com ids originating from the destination */
    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tlm_addListener(session_handle, &list_handle, &user_ref, NULL, 0, 0, 0, src_ip, flags, src_uri);
        printf("msg_info.user_ref=%d\n", user_ref);
        printf("\n");
    }

    printf("\nREPLIER\n\n");

    while(TRUE)
    {
        wait_for_msg();

        if(rx_callback_msg.msgType == TRDP_MSG_MN)
        {
            printf("Received message with com id %u: %s\n", rx_callback_msg.comId, recv_buffer);
        }
        else if(rx_callback_msg.msgType == TRDP_MSG_MR)
        {
            printf("Received message with com id %u: %s\n", rx_callback_msg.comId, recv_buffer);
            strcpy(md_buffer, "Request received, Testbox requires a confirm!");
            md_size = strlen(md_buffer)+1;
            tlm_replyQuery(session_handle, (const TRDP_UUID_T *)&rx_callback_msg.sessionId, rx_callback_msg.comId+1, 0, 5000000, NULL, (DG_U8*)md_buffer, md_size);
        }
        else if(rx_callback_msg.msgType == TRDP_MSG_MC)
        {
            printf("Received confirm\n");
            break;
        }
        else
        {
            printf("ERROR UNEXPECTED MESSAGE RECEIVED\n");
            break;
        }
        Sleep(100);
    }
    printf("\nCALLER\n\n");
    flags = TRDP_FLAGS_DEFAULT;

    /* Send notify */
    if(TRDP_NO_ERR == ret_val)
    {
        user_ref++;
        com_id = 3000;
        strcpy(md_buffer, "You are about to receive a message from the Testbox.");
        md_size = strlen(md_buffer)+1;
        ret_val = tlm_notify(session_handle, &user_ref, NULL, com_id, 0, 0, src_ip, dest_ip, flags, NULL, (DG_U8*)md_buffer, md_size, src_uri, dest_uri);
        printf("notify: %d\n", ret_val);
        printf("\n");
    }

    tlc_process(session_handle, NULL, NULL);
    Sleep(1000);

    /* Request - Reply - Confirm */
    if(TRDP_NO_ERR == ret_val)
    {
        com_id = 4000;
        vos_getUuid(session_id);
        strcpy(md_buffer, "Testbox Requesting a Reply!");
        md_size = strlen(md_buffer)+1;
        ret_val = tlm_request(session_handle, &user_ref, NULL, &session_id, com_id, 0, 0, src_ip, dest_ip, flags, 1, 5000000, 0, NULL, (DG_U8*)md_buffer, md_size, src_uri, dest_uri);
        printf("request: %d\n", ret_val);
        printf("\n");
    }

    tlc_process(session_handle, NULL, NULL);

    if(TRDP_NO_ERR == ret_val)
    {
        is_msg_received = 0;
        wait_for_msg();

        if(TRDP_NO_ERR != *trdp_err_no)
        {
            printf("ERROR\n");
        }
        else if((rx_callback_msg.comId / 1000) != (com_id / 1000))
        {
            printf("ERROR RECEIVED COM ID %u DOES NOT MATCH REPLY\n", rx_callback_msg.comId);
            ret_val = TRDP_PARAM_ERR;
        }
        else if(0!=memcmp(session_id, rx_callback_msg.sessionId,sizeof(TRDP_UUID_T)))
        {
            printf("ERROR RECEIVED SESSION ID DOES NOT MATCH\n");
            ret_val = TRDP_PARAM_ERR;
        }
        else if(rx_callback_msg.msgType == TRDP_MSG_MQ)
        {
            printf("Received message with com_id %d: %s\n", rx_callback_msg.comId, md_buffer);
            ret_val = tlm_confirm(session_handle, (const TRDP_UUID_T *)&session_id, 0, NULL);
            tlc_process(session_handle, NULL, NULL);
        }
        else
        {
            printf("ERROR UNEXPECTED MESSAGE RECEIVED\n");
        }

    }


    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tlm_delListener(session_handle, list_handle);
        printf("\n");
    }
    return ret_val;
}

static void md_callback (void *ref, TRDP_APP_SESSION_T apph, const TRDP_MD_INFO_T *msg, UINT8 *data, UINT32 size)
{
    memcpy(&rx_callback_msg, msg, sizeof(TRDP_MD_INFO_T));
    recv_length = size;
    memcpy(recv_buffer, data, recv_length);
    is_msg_received = 1;
}


/* ==================================================================================
 * Global function implementations
 * ================================================================================*/
/*
int main(void)
{
    DG_S32 trdp_err_no = 0;
    DG_S32 ret_val = 0;

    recv_length = sizeof(recv_buffer);
    is_msg_received = 0;


    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tau_prepareXmlDoc("./trdp_tb_config.xml", &xml_handle);
    }
    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tau_readXmlDeviceConfig(&xml_handle, &mem_cfg, &dbg_cfg, &nb_com_par, &p_com_par, &nb_if_cfg, &p_if_cfg);
    }
    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tau_readXmlInterfaceConfig(&xml_handle, p_if_cfg[0].ifName, &process_cfg, &pd_cfg, &md_cfg, &nb_exchg_par, &p_exchg_par);
    }
    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tau_readXmlDatasetConfig(&xml_handle, &nb_com_id, &p_com_id_ds_map, &nb_dataset, &ap_dataset );
    }


    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tau_initMarshall(&marshall_cfg.pRefCon, nb_com_id, p_com_id_ds_map, nb_dataset, ap_dataset);
        marshall_cfg.pfCbMarshall = tau_marshall;
        marshall_cfg.pfCbUnmarshall = tau_unmarshall;
    }

    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tlc_init(print_log, &mem_cfg);
    }
    if(TRDP_NO_ERR == ret_val)
    {
        md_cfg.pfCbFunction      = md_callback;
        ret_val = tlc_openSession(&session_handle, p_if_cfg[0].hostIp, p_if_cfg[0].leaderIp, &marshall_cfg, &pd_cfg, &md_cfg, &process_cfg);
    }

    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = trdp_pd_demo(&trdp_err_no);
    }

    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = trdp_md_demo(&trdp_err_no);
    }

    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tlc_closeSession(session_handle);
    }
    if(TRDP_NO_ERR == ret_val)
    {
        ret_val = tlc_terminate();
    }

    tau_freeTelegrams(nb_exchg_par, p_exchg_par);
    tau_freeXmlDoc(&xml_handle);


    printf("Result: %d\n", trdp_err_no);
    return 0;
}
*/
