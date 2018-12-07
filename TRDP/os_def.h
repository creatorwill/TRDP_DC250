/* ==========================================================================
 *
 *  File      : OS_DEF.H
 *
 *  Purpose   : Host definitions for WIN32 hosts
 *
 *  Licence   : Duagon Software Licence (see file 'licence.txt')
 *
 * --------------------------------------------------------------------------
 *
 *  (C) COPYRIGHT, Duagon GmbH, CH-8953 Dietikon, Switzerland
 *  All Rights Reserved.
 *
 * ==========================================================================
 */

#ifndef OS_DEF_H
#define OS_DEF_H

/* ==========================================================================
 *
 *  General Constants and Types
 *
 * ==========================================================================
 */

/* Following defines may be used to configure the driver to your needs */
#define HAVE_MUTEX      1    /* Fast exclusion of mutex library and
                                functionality */
#define HAVE_PRINTF     1    /* Fast exclusion of printf libraries and
                                functionality */
#define HAVE_TIMER      1    /* Fast exclusion of timer library and
                                functionality */
#define HAVE_FILE		1	 /* Fast exclusion of file library */

/* ==========================================================================
 *
 *  Include Files
 *
 * ==========================================================================
 */
#include <basetsd.h>   /* data types    */
#include <string.h>    /* strlen        */

#if (1 == HAVE_TIMER)
#include <time.h>      /* time          */
#endif

#if (1 == HAVE_MUTEX)
#include "pthread.h"   /* mutex         */
#endif

#if (1 == HAVE_FILE)
#include <stdio.h>     /* files         */
#endif

/* ==========================================================================
 *
 *  DLL handling
 *
 * ==========================================================================
 */
#ifdef CREATE_DLL
#define DLL __declspec(dllexport)
#else
#define DLL 
#endif

/* ==========================================================================
 *
 *  Data Types
 *
 * ==========================================================================
 */

/* --------------------------------------------------------------------------
 *  Data types with less than 8-bits
 * --------------------------------------------------------------------------
 */
typedef unsigned char       DG_BOOL;

/* --------------------------------------------------------------------------
 *  8-bit data types
 * --------------------------------------------------------------------------
 */
typedef unsigned char       DG_U8;
typedef signed char         DG_S8;
typedef char                DG_CHAR8;

/* --------------------------------------------------------------------------
 *  16-bit data types
 * --------------------------------------------------------------------------
 */
typedef unsigned short      DG_U16;
typedef short               DG_S16;

/* --------------------------------------------------------------------------
 *  32-bit data types
 * --------------------------------------------------------------------------
 */
typedef unsigned int        DG_U32;
typedef int                 DG_S32;

typedef float               DG_REAL32;

/* --------------------------------------------------------------------------
 *  64-bit data types
 * --------------------------------------------------------------------------
 */
typedef UINT64              DG_U64;
typedef INT64               DG_S64;


/* This type is used for protocol mutexes and can be accessed by the osl
 * functions.
 */
#if (1 == HAVE_MUTEX)
typedef pthread_mutex_t     OSL_MUTEX;
#else
typedef DG_U32            OSL_MUTEX;
#endif

/* Please adapt to File handle type (only needed for certain drivers!) */
#if (1 == HAVE_FILE)
typedef FILE        OSL_FILE;
#else
typedef DG_S32    OSL_FILE;
#endif

/* Please adapt this Type to the System Timeout Struct */
#if (1 == HAVE_TIMER)
typedef UINT64      OSL_TIMER;
#else
typedef DG_U32    OSL_TIMER;
#endif

/* ==========================================================================
 *
 *  Macros
 *
 * ==========================================================================
 */

/* --------------------------------------------------------------------------
 *  Macros for declaration of variables and procedures.
 *  NOTE:
 *  extern "C" is used in mixed C/C++ headers to force C linkage on an
 *  external definition.
 * --------------------------------------------------------------------------
 */

#define DG_DECL_PUBLIC  extern
#define DG_DECL_LOCAL   static

#define DG_DECL_CONST      const
#ifndef NULL
 #define NULL (void*)0
#endif

#ifndef TRUE
 #define TRUE (0==0)
#endif

#ifndef FALSE
 #define FALSE !TRUE
#endif
 

/* --------------------------------------------------------------------------
 *  Must be adapted to little and big endian (set '1' if big endian host)
 * --------------------------------------------------------------------------
 */
#define DG_HOST_SWAP 0

 
/* --------------------------------------------------------------------------
 *  REGISTER SIZE CONFIGURATION
 *
 *  This datatype is dependant of the architecture and the host configuration.
 *  It has to match with the STEPCFG value in the HCR register (interface ISA
 *  configuration). Possible values: DG_U8, DG_U16, DG_U32
 * --------------------------------------------------------------------------
 */
typedef DG_U8                    REG_SIZE;


/* ----------------------------------------------------------------------- */
#define osl_printf printf

#endif /* #ifndef OS_DEF_H*/
