#pragma once

#include "ITPObject.h"

namespace Utility
{
	class JP_UTIL_API TPTCPServer : public ITPObject
	{
	public:
		TPTCPServer(ITPListener* tcpserverapp, int engineId = 0);
		TPTCPServer(ITPListener* listener, CBaseMutex* mutex, int engineId = 0);
		virtual ~TPTCPServer();

	public:
		int Listen(char* ip, int port);
		int Send(int connId, unsigned int sendid, char* pBuf, unsigned int iBufLen);
		int Heartbeat();
		int CloseClient(int id);
		int Close();

		int SetMaxDataQueueLength(int maxDataQueueLength);
	protected:
		int __SendInside(int connId, char* pBuf, unsigned int iBufLen);
		int __CloseInside();

	protected:
		void	__AddPendingCloseClient(int id);
		int		__DealPendingCloseClients();
		void	__PendingCloseClient(int id);
		std::deque<int> _pendingCloseClients;
		CThreadRecMutex _pendingCloseClientsMutex;

	protected:
		int	_maxDataQueueLength;
		CONN_MAP		_clients;
		DataRowJitt		_queueJitt;
	};
}


