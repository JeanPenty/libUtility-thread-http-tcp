#pragma once
#if defined(WIN32)
#include <unordered_map>
#endif


#define BUF_SIZE 64*1024

#define TP_NORMAL_RET						0

#define TP_ERROR_BASE						-1
#define TP_ERROR_UNSUPPORT					(TP_ERROR_BASE - 100)
#define TP_ERROR_BAD_CONNECTION				(TP_ERROR_BASE - 101)
#define TP_ERROR_SET_NOBLOCKING_FAILED		(TP_ERROR_BASE - 102)
#define TP_ERROR_MAX_SENDQUEUE_LENGTH		(TP_ERROR_BASE - 103)
#define TP_ERROR_CONNECTFAILED						(TP_ERROR_BASE - 104)


#ifndef SOCKET_ERROR
#define SOCKET_ERROR		-1
#endif

typedef struct
{
	unsigned int	ip;//ÍøÂç×Ö½ÚÐò
	unsigned short	port;//ÍøÂç×Ö½ÚÐò
	unsigned short	online;		//0:using 1:unusing
	int				socket;
	unsigned int	id;
	unsigned int	timemark;
}client_list;

typedef enum
{
	TP_SEND = 1,
	TP_RECEIVE
} TPType;