/**
 * \file trdp_host_demo.c
 * \brief 
 * \date 13.10.2014
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
#include "dg_trdp.h"
#include "os_def.h"
#include "dg_error.h"

#include <windows.h>

/* ==================================================================================
 * Definitions (typedef, constants, macros)
 * ================================================================================*/
#define MAX_LINE_LENGTH 16

typedef struct DS1001 {
        DG_U8 a;
        DG_U8 b;
        DG_U16 c;
        DG_U32 d;
        DG_U64 e;
} DS1001;

typedef enum { false, true } bool;
/* ==================================================================================
 * Local/Global variables definitions
 * ================================================================================*/

/* ==================================================================================
 * Local function declarations
 * ================================================================================*/
static DG_S32 trdp_pd_demo(DG_S32 *trdp_err_no);

static DG_S32 trdp_md_demo(DG_S32 *trdp_err_no);
/* ==================================================================================
 * Local function implementations
 * ================================================================================*/
static DG_S32 trdp_pd_demo(
    DG_S32 *trdp_err_no)
{
    DG_S32 ret_val = DG_TRDP_NO_ERR;
    DG_U32 size = sizeof(DS1001);
    DS1001 data_put = { 0, 0, 0, 0, 0};
    DS1001 data_get = { 0, 0, 0, 0, 0};
    DG_U32 com_id;
    DG_U32 i;
    TRDP_PD_INFO pd_info;
    com_id = 1001;
    pd_info.com_id = com_id;

    if(DG_TRDP_NO_ERR == ret_val)
    {
        ret_val = trdp_pd_publish(com_id, (DG_U8*)&data_put, size, trdp_err_no, NULL);
        printf("RETURN: %d, ERROR: %d\n", ret_val, *trdp_err_no);
        printf("\n");
    }
    if(DG_TRDP_NO_ERR == ret_val)
    {
        ret_val = trdp_pd_subscribe(com_id, trdp_err_no, NULL);
        printf("RETURN: %d, ERROR: %d\n", ret_val, *trdp_err_no);
        printf("\n");
    }

    while(0 != trdp_pd_get(pd_info.com_id, &pd_info.src_ip, &pd_info.dest_ip, &pd_info.seq_count, &pd_info.msg_type, (DG_U8*)&data_get, &size,
            trdp_err_no, NULL))
    {
        printf(".");
        Sleep(100);
    }
    printf("\n");

    for(i = 0;i < 100;i++)
    {
        data_put.a++;
        data_put.c++;
        data_put.e++;
        size = sizeof(DS1001);
        printf("Put dataset %d, %d, %d, %u, %u\n", data_put.a, data_put.b, data_put.c, data_put.d, (DG_U32)data_put.e);
        printf("RETURN: %d, ERROR: %d\n", ret_val, *trdp_err_no);
        ret_val = trdp_pd_put(com_id, (DG_U8*)&data_put, size, trdp_err_no, NULL);

        ret_val = trdp_pd_get(pd_info.com_id, &pd_info.src_ip, &pd_info.dest_ip, &pd_info.seq_count, &pd_info.msg_type, (DG_U8*)&data_get, &size,
                trdp_err_no, NULL);
        printf("Got dataset %d, %d, %d, %u, %u\n", data_get.a, data_get.b, data_get.c, data_get.d, (DG_U32)data_get.e);
        printf("RETURN: %d, ERROR: %d\n", ret_val, *trdp_err_no);
        printf("\n");

        Sleep(100);
    }

    ret_val = trdp_pd_unpublish(com_id, trdp_err_no, NULL);
    printf("RETURN: %d, ERROR: %d\n", ret_val, *trdp_err_no);
    printf("\n");

    ret_val = trdp_pd_unsubscribe(com_id, trdp_err_no, NULL);
    printf("RETURN: %d, ERROR: %d\n", ret_val, *trdp_err_no);

    return ret_val;
}

static DG_S32 trdp_md_demo(DG_S32 *trdp_err_no)
{
    TRDP_MSG_INFO msg_info;
    TRDP_MSG_INFO received_msg;
    DG_S32 ret_val = DG_TRDP_NO_ERR;
    DG_CHAR8 md_buffer[1000];
    strcpy(md_buffer, "Duagon Device Requesting Backup - Please Reply!");
    DG_U32 md_size = sizeof(md_buffer);
    printf("md_size: %d\n", md_size);

    DG_U32 list_size = 10;
    DG_U32 list_info[list_size];

    DG_U32 user_status = 12;

    msg_info.com_id = 0;
    msg_info.src_ip = 0xC0A80114;
    msg_info.dest_ip = 0xC0A80161;
    strcpy(msg_info.src_uri,"duagon@duagon");
    strcpy(msg_info.dest_uri,"test@duagon");

    msg_info.flags = DG_TRDP_FLAGS_CALLBACK;

    /* Add listenere for all com ids originating from the destination */
    if(DG_TRDP_NO_ERR == ret_val)
    {
        ret_val = trdp_md_add_listener(&msg_info.user_ref, msg_info.com_id, msg_info.src_ip, msg_info.src_uri, msg_info.flags, trdp_err_no, NULL);
        received_msg.user_ref = msg_info.user_ref;
        printf("msg_info.user_ref=%d\n", msg_info.user_ref);
        printf("\n");
    }

    printf("\nCALLER\n\n");
    /* Send notify */

    if(DG_TRDP_NO_ERR == ret_val)
    {
        msg_info.com_id = 1000;
        strcpy(md_buffer, "You are about to receive a message from the duagon device");
        md_size = strlen(md_buffer)+1;
        ret_val = trdp_md_notify(msg_info.user_ref, msg_info.com_id, msg_info.src_ip, msg_info.dest_ip, msg_info.src_uri,
                msg_info.dest_uri, msg_info.flags, (DG_U8*)md_buffer, md_size, trdp_err_no, NULL);
        printf("notify: %d, %d\n", ret_val, *trdp_err_no);
        printf("\n");
    }

    Sleep(1000);

    /* Request - Reply - Confirm */
    if(DG_TRDP_NO_ERR == ret_val)
    {
        msg_info.com_id = 2000;
        strcpy(md_buffer, "Duagon Device Requesting a Reply!");
        md_size = strlen(md_buffer)+1;
        ret_val = trdp_md_request(msg_info.user_ref, msg_info.com_id, msg_info.src_ip, msg_info.dest_ip, msg_info.src_uri,
                msg_info.dest_uri, msg_info.flags, 1, 0, msg_info.session_id, (DG_U8*)md_buffer, md_size, trdp_err_no, NULL);
        printf("request: %d, %d\n", ret_val, *trdp_err_no);
        printf("\n");
    }

    if(DG_TRDP_NO_ERR == ret_val)
    {

        while(ret_val != 1)
        {
            list_size = sizeof(list_info);
            ret_val = trdp_md_poll_msgs(list_info, &list_size, trdp_err_no, NULL);
            printf(".");
            //printf("poll: %d, %d, %d, %d\n", list_info[0], list_size, ret_val, *trdp_err_no);
            received_msg.user_ref=list_info[0];
            Sleep(100);
        }
        printf("\n");
        md_size = sizeof(md_buffer);
        ret_val = trdp_md_get(&received_msg.user_ref, &received_msg.com_id, &received_msg.msg_type, &received_msg.src_ip, &received_msg.dest_ip,
                received_msg.src_uri, received_msg.dest_uri, received_msg.session_id, (DG_U8*)md_buffer, &md_size, trdp_err_no, NULL);
        printf("get: %d, %d\n", ret_val, *trdp_err_no);
        if(DG_TRDP_NO_ERR != *trdp_err_no)
        {
            printf("ERROR\n");
        }
        else if((received_msg.com_id / 1000) != (msg_info.com_id / 1000))
        {
            printf("ERROR RECEIVED COM ID %u DOES NOT MATCH REPLY\n", received_msg.com_id);
            ret_val = DG_TRDP_PARAM_ERR;
        }
        else if(0!=memcmp(msg_info.session_id,received_msg.session_id,sizeof(DG_TRDP_UUID_T)))
        {
            printf("ERROR RECEIVED SESSION ID DOES NOT MATCH\n");
            ret_val = DG_TRDP_PARAM_ERR;
        }
        else if(received_msg.msg_type == DG_TRDP_MSG_MQ)
        {
            printf("Received message with com_id %d: %s\n", received_msg.com_id, md_buffer);
            ret_val = trdp_md_confirm(received_msg.session_id, user_status, trdp_err_no, NULL);
        }
    }

    printf("\nREPLIER\n\n");
    if(DG_TRDP_NO_ERR == ret_val)
    {
        while(TRUE)
        {
            ret_val = 0;
            while(ret_val != 1)
            {
                list_size = sizeof(list_info);
                ret_val = trdp_md_poll_msgs(list_info, &list_size, trdp_err_no, NULL);
                printf(".");
                //printf("poll: %d, %d, %d, %d\n", list_info[0], list_size, ret_val, *trdp_err_no);
                received_msg.user_ref=list_info[0];
                Sleep(100);
            }
            printf("\n");
            md_size = sizeof(md_buffer);
            ret_val = trdp_md_get(&received_msg.user_ref, &received_msg.com_id, &received_msg.msg_type, &received_msg.src_ip, &received_msg.dest_ip, received_msg.src_uri,
                received_msg.dest_uri, received_msg.session_id, (DG_U8*)md_buffer, &md_size, trdp_err_no, NULL);
            //printf("get: %d, %d\n", ret_val, *trdp_err_no);
            if(DG_TRDP_NO_ERR != *trdp_err_no)
            {
                printf("RET_VAL: %d, ERROR: %d\n", ret_val, *trdp_err_no);
            }

            if(received_msg.msg_type == DG_TRDP_MSG_MN)
            {
                printf("Received message with com id %u: %s\n", received_msg.com_id, md_buffer);
            }
            else if(received_msg.msg_type == DG_TRDP_MSG_MR)
            {
                printf("Received message with com id %u: %s\n", received_msg.com_id, md_buffer);
                strcpy(md_buffer, "Request received, duagon device requires a confirm!");
                md_size = strlen(md_buffer)+1;
                trdp_md_reply_query(received_msg.session_id,0,received_msg.com_id+1,(DG_U8*)md_buffer, md_size, trdp_err_no, NULL);
            }
            else if(received_msg.msg_type == DG_TRDP_MSG_MC)
            {
                printf("Received confirm\n");
                break;
            }
            else
            {
                printf("ERROR UNEXPECTED MESSAGE RECEIVED\n");
                break;
            }

        }
    }

    trdp_md_del_listener(msg_info.user_ref, trdp_err_no, NULL);
    printf("delete listener %d\n",msg_info.user_ref);
    return ret_val;
}

/* ==================================================================================
 * Global function implementations
 * ================================================================================*/
int main(void)
{
    DG_S32 trdp_err_no = 0;
    DG_S32 ret_val = 0;

    DG_S32 nb_pub_pd = 0;
    DG_S32 nb_sub_pd = 0;
    DG_S32 nb_list_md = 0;

    printf("Initializing TRDP\n");
    while ( (trdp_init(&trdp_err_no, NULL) == -1))
    {
        switch ( trdp_err_no )
        {
        case DG_ERROR_OSL_NOT_READY:
            osl_printf ( "REGISTERS not ready\n");
            break;

        case DG_ERROR_EMPTY_BLOCK:
            osl_printf ( "PROTOCOL not ready\n");
            break;

        case DG_ERROR_OSL_INIT:
            osl_printf ( "I/O access initialization failed\n");
        #ifdef _WIN32
            osl_printf ( "is SYS device running? (GiveIO or OLS)\n" );
        #endif
            return 0;

        default:
            osl_printf ( "trdp_init failed with error: %d\n", trdp_err_no);
            return 0;
        }
    }
    ret_val = trdp_get_status(&nb_pub_pd, &nb_sub_pd, &nb_list_md, &trdp_err_no, NULL);

    if(ret_val == DG_TRDP_NO_ERR)
    {
        ret_val = trdp_pd_demo(&trdp_err_no);
    }

    if(ret_val == DG_TRDP_NO_ERR)
    {
        ret_val = trdp_md_demo(&trdp_err_no);
    }

    printf("Result: %d\n", trdp_err_no);

    return ret_val;
}