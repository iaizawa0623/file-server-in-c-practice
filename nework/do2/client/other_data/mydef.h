
/* mydef.h */
#ifndef _POSIX_C_SOURCE
#define	_POSIX_C_SOURCE 199309L
#endif	/* _POSIX_C_SOURCE */

#ifndef MYDEF_H
#define MYDEF_H

#include <stdint.h>
#define MAX_CLIT	3
#define PORT_NUM	5000
#define DATA_SIZ	256 * 1024
#define SELECT_SEC	3
#define SELECT_USEC	0
#define	ALARM_SEC	1
#define	LARGER(a,b)	(a > b ? a : b)
/* head部分の型 */
typedef struct {
	uint32_t length;
	uint32_t sequence;
} head_t;
#define HEAD_SIZ sizeof(head_t)
/* 送受信するパケットの型 */
typedef struct {
	head_t	head;
	uint8_t	data[DATA_SIZ];
} pack_t;

#endif	/* MYDEF_H */
