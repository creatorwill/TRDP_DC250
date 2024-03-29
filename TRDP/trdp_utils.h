/******************************************************************************/
/**
 * @file            trdp_utils.h
 *
 * @brief           Common utilities for TRDP communication
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id: trdp_utils.h 1353 2014-11-11 15:11:13Z ahweiss $
 *
 */


#ifndef TRDP_UTILS_H
#define TRDP_UTILS_H

/*******************************************************************************
 * INCLUDES
 */

#include <stdio.h>

#include "trdp_private.h"
#include "vos_utils.h"
#include "vos_sock.h"

/*******************************************************************************
 * DEFINES
 */

#define Swap32(val)     (UINT32)(((0xFF000000 & (UINT32)val) >> 24) | \
                                 ((0x00FF0000 & (UINT32)val) >> 8)  | \
                                 ((0x0000FF00 & (UINT32)val) << 8)  | \
                                 ((0x000000FF & (UINT32)val) << 24))

#ifdef B_ENDIAN
/* introduce byte swapping on big endian machines needed for CRC handling */
#define MAKE_LE(a)      Swap32(a)
#else
#ifdef L_ENDIAN
#define MAKE_LE(a)      (a)
#endif
#endif

#define TRDP_INVALID_SOCKET_INDEX  -1

/*******************************************************************************
 * TYPEDEFS
 */

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */


extern TRDP_LOG_T   gDebugLevel;

PD_ELE_T            *trdp_queueFindComId (
    PD_ELE_T    *pHead,
    UINT32      comId);

PD_ELE_T            *trdp_queueFindSubAddr (
    PD_ELE_T            *pHead,
    TRDP_ADDRESSES_T    *pAddr);

PD_ELE_T            *trdp_queueFindPubAddr (
    PD_ELE_T            *pHead,
    TRDP_ADDRESSES_T    *addr);

void    trdp_queueDelElement (
    PD_ELE_T    * *pHead,
    PD_ELE_T    *pDelete);

void    trdp_queueAppLast (
    PD_ELE_T    * *pHead,
    PD_ELE_T    *pNew);

void    trdp_queueInsFirst (
    PD_ELE_T    * *pHead,
    PD_ELE_T    *pNew);

#if MD_SUPPORT
MD_ELE_T    *trdp_MDqueueFindAddr (
    MD_ELE_T            *pHead,
    TRDP_ADDRESSES_T    *addr);

void        trdp_MDqueueDelElement (
    MD_ELE_T    * *ppHead,
    MD_ELE_T    *pDelete);

void        trdp_MDqueueAppLast (
    MD_ELE_T    * *pHead,
    MD_ELE_T    *pNew);

void        trdp_MDqueueInsFirst (
    MD_ELE_T    * *ppHead,
    MD_ELE_T    *pNew);
#endif

/*********************************************************************************************************************/
/** Handle the socket pool: Initialize it
 *
 *  @param[in]      iface          pointer to the socket pool
 */

void trdp_initSockets(
    TRDP_SOCKETS_T iface[]);


/**********************************************************************************************************************/
/** ???
 *
 *  @param[in]      appHandle          session handle
 */

void trdp_initUncompletedTCP (
    TRDP_APP_SESSION_T appHandle);

/**********************************************************************************************************************/
/** remove the sequence counter for the comID/source IP.
 *  The sequence counter should be reset if there was a packet time out.
 *  
 *
 *  @param[in]      pElement            subscription element
 *  @param[in]      srcIP               Source IP address
 *  @param[in]      msgType             message type
 *
 *  @retval         none
 */

void trdp_resetSequenceCounter(
                               PD_ELE_T*       pElement,
                               TRDP_IP_ADDR_T  srcIP,
                               TRDP_MSG_T      msgType);

/*********************************************************************************************************************/
/** Handle the socket pool: Request a socket from our socket pool
 *
 *  @param[in,out]  iface           socket pool
 *  @param[in]      port            port to use
 *  @param[in]      params          parameters to use
 *  @param[in]      srcIP           IP to bind to (0 = any address)
 *  @param[in]      mcGroup         MC group to join (0 = do not join)
 *  @param[in]      usage           type and port to bind to
 *  @param[in]      options         blocking/nonblocking
 *  @param[in]      rcvMostly       only used for receiving
 *  @param[out]     useSocket       socket to use, do not open a new one
 *  @param[out]     pIndex          returned index of socket pool
 *  @param[in]      cornerIp        only used for receiving
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_PARAM_ERR
 */

TRDP_ERR_T trdp_requestSocket(
    TRDP_SOCKETS_T iface[],
    UINT32 port,
    const TRDP_SEND_PARAM_T * params,
    TRDP_IP_ADDR_T srcIP,
    TRDP_IP_ADDR_T mcGroup,
    TRDP_SOCK_TYPE_T usage,
    TRDP_OPTION_T options,
    BOOL8 rcvMostly,
    INT32 useSocket,
    INT32                   * pIndex,
    TRDP_IP_ADDR_T cornerIp);

/*********************************************************************************************************************/
/** Handle the socket pool: Release a socket from our socket pool
 *
 *  @param[in,out]  iface           socket pool
 *  @param[in]      lIndex          index of socket to release
 *  @param[in]      connectTimeout  timeout value
 *  @param[in]      checkAll        release all TCP pending sockets
 *
 */

void trdp_releaseSocket(
    TRDP_SOCKETS_T iface[],
    INT32 lIndex,
    UINT32 connectTimeout,
    BOOL8 checkAll);


/*********************************************************************************************************************/
/** Get the packet size from the raw data size
 *
 *  @param[in]      dataSize            net data size
 *
 *  @retval         packet size         the size of the complete packet to
 *                                      be sent or received
 */

UINT32 trdp_packetSizePD (
    UINT32 dataSize);

/*********************************************************************************************************************/
/** Get the packet size from the raw data size
 *
 *  @param[in]      dataSize            net data size
 *
 *  @retval         packet size         the size of the complete packet to
 *                                      be sent or received
 */

UINT32 trdp_packetSizeMD (
    UINT32 dataSize);

/*********************************************************************************************************************/
/** Get the initial sequence counter for the comID/message type and subnet (source IP).
 *  If the comID/srcIP is not found elsewhere, return 0 -
 *  else return its current sequence number (the redundant packet needs the same seqNo)
 *
 *  Note: The standard demands that sequenceCounter is managed per comID/msgType at each publisher,
 *        but shall be the same for redundant telegrams (subnet/srcIP).
 *
 *  @param[in]      comId           comID to look for
 *  @param[in]      msgType         PD/MD type
 *  @param[in]      srcIpAddr       Source IP address
 *
 *  @retval            return the sequence number
 */

UINT32 trdp_getSeqCnt (
    UINT32          comID,
    TRDP_MSG_T      msgType,
    TRDP_IP_ADDR_T  srcIP);


/**********************************************************************************************************************/
/** check and update the sequence counter for the comID/source IP.
 *  If the comID/srcIP is not found, update it and return 0 -
 *  else if already received, return 1
 *  On memory error, return -1
 *
 *  @param[in]      pElement            subscription element
 *  @param[in]      sequenceCounter     sequence counter to check
 *  @param[in]      srcIP               Source IP address
 *  @param[in]      msgType             type of the message
 *
 *  @retval         0 - no duplicate
 *                  1 - duplicate sequence counter
 *                 -1 - memory error
 */

int trdp_checkSequenceCounter(
    PD_ELE_T*       pElement,
    UINT32          sequenceCounter,
    TRDP_IP_ADDR_T  srcIP,
    TRDP_MSG_T      msgType);


/**********************************************************************************************************************/
/** Check if listener URI is in addressing range of destination URI.
 *
 *  @param[in]      listUri       Null terminated listener URI string to compare
 *  @param[in]      destUri       Null terminated destination URI string to compare
 *
 *  @retval         FALSE - not in addressing range
 *  @retval         TRUE  - listener URI is in addressing range of destination URI
 */

BOOL8 trdp_isAddressed (
    const TRDP_URI_USER_T   listUri,
    const TRDP_URI_USER_T   destUri);

#endif
