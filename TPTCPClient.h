#pragma once

#include "ITPObject.h"

namespace Utility
{
	class JP_UTIL_API TPTCPClient : public ITPObject
	{
	public:
		TPTCPClient(ITPListener* tcpclientapp, int engineId = 0);
		TPTCPClient(ITPListener* tcpclientapp, CBaseMutex* mutex, int engineId = 0);
		virtual ~TPTCPClient();

	public:
		int Connect(const char* ip, int port);
		int Send(unsigned int sendid, char* pBuf, unsigned int iBufLen);
		int Heartbeat(void);
		int Close(void);
		int SetMaxDataQueueLength(int maxDataQueueLength);

	protected:
		int _SendInside(char* pBuf, unsigned int iBufLen);
		int _CloseInside();

	protected:
		int	_maxDataQueueLength;
		DataRowList		_queue;
	};
}


