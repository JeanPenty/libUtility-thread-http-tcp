#include "pch.h"
#include "TPTCPServer.h"

namespace Utility
{
	TPTCPServer::TPTCPServer(ITPListener* TCPserverapp, int engineId) : ITPObject(TCPserverapp, engineId)
	{
		_mutex = new CNullMutex();
		m_bInnerMutex = true;

		_maxDataQueueLength = 0;
	}

	TPTCPServer::TPTCPServer(ITPListener* TCPserverapp, CBaseMutex* mutex, int engineId)
		: ITPObject(TCPserverapp, engineId)
	{
		if (NULL != mutex)
		{
			_mutex = mutex;
			m_bInnerMutex = false;
		}
		else
		{
			_mutex = new CNullMutex();
			m_bInnerMutex = true;
		}

		_maxDataQueueLength = 0;
	}

	TPTCPServer::~TPTCPServer()
	{
		if (m_bInnerMutex)
		{
			delete _mutex;
			_mutex = NULL;
		}
	}

	int TPTCPServer::Listen(char* ip, int port)
	{
		_mutex->lock();

		if (ip == NULL)
		{
			_ip = INADDR_ANY;
		}
		else
		{
			_ip = inet_addr(ip);
		}

		_port = htons(port);

		if (INVALID_SOCKET == _socket)
		{
			_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		}

		struct sockaddr_in my_addr;
		const int yes = 1;

		//TODO: ������Ƿ���Ҫ����SO_REUSEADDR����

#ifdef WIN32
		unsigned long l = 1;
		int n = ioctlsocket(_socket, FIONBIO, &l);
		if (n != 0)
		{
			int errcode = WSAGetLastError();
			_mutex->unlock();
			return TP_ERROR_SET_NOBLOCKING_FAILED;
		}

#endif

		memset(&my_addr, 0, sizeof(my_addr));
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = _port;
		my_addr.sin_addr.s_addr = _ip;

		if ((INVALID_SOCKET == bind(_socket, (struct sockaddr*)&my_addr, sizeof(struct sockaddr))) ||
			INVALID_SOCKET == listen(_socket, 5)
			)
		{
			__CloseInside();
			_socket = INVALID_SOCKET;
		}

		if (_recvBuffSize > 0)
			setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (char*)&_recvBuffSize, sizeof(_recvBuffSize));
		if (_sendBufferSize > 0)
			setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, (char*)&_sendBufferSize, sizeof(_sendBufferSize));

		_mutex->unlock();
		return _socket;
	}

	void TPTCPServer::__AddPendingCloseClient(int id)
	{
		CThreadRecMutex::Lock guard(_pendingCloseClientsMutex);
		_pendingCloseClients.push_back(id);
	}

	int TPTCPServer::__DealPendingCloseClients()
	{
		std::deque<int> clients;

		{
			CThreadRecMutex::Lock guard(_pendingCloseClientsMutex);
			if (!_pendingCloseClients.empty())
			{
				clients = _pendingCloseClients;
				_pendingCloseClients.clear();
			}
			else
			{
				return 0;
			}
		}

		std::deque<int>::iterator iter = clients.begin();
		const std::deque<int>::const_iterator end = clients.end();
		int count = 0;
		for (; end != iter; ++iter, ++count)
		{
			__PendingCloseClient(*iter);
		}

		return count;
	}

	void TPTCPServer::__PendingCloseClient(int id)
	{
		_mutex->lock();

		int iRet = 0;

		//begin modify by gaowei,08-03-24
		client_list* bean = NULL;
		int sock = 0;
		CONN_MAP::iterator it = _clients.find(id);

		if (it != _clients.end())
		{
			bean = it->second;
			_clients.erase(id);

			if (NULL != bean)
			{
				sock = bean->socket;
				closesocket(bean->socket);
				delete bean;
			}
		}

		//�޸����ݻ������ݽṹ��QueueJitt��ɾ���ͻ��˵����ݾͷ����� 
		/* �ӷ���������������رտͻ��˵�����*/
		DataRowJitt::iterator itJitt = _queueJitt.find(sock);
		if (itJitt != _queueJitt.end())
		{
			DataRowList* ql = itJitt->second;
			if (ql != NULL)
			{
				int queueSize = ql->size();
				for (unsigned int i = 0; i < queueSize; i++)
				{
					DataRowPtr data = ql->front();
					ql->pop();
				}

				delete ql;
			}
			_queueJitt.erase(itJitt);
		}
		else
		{
			//	OutputDebugString("what? ? \n");
		}

		_mutex->unlock();
	}
	int TPTCPServer::CloseClient(int id)
	{
		__AddPendingCloseClient(id);
		return 0;
	}

	int TPTCPServer::Heartbeat()
	{
		int retValue = TP_NORMAL_RET;

		_mutex->lock();

		if (INVALID_SOCKET == _socket)
		{
			_mutex->unlock();
			return TP_ERROR_BASE;
		}

		__DealPendingCloseClients();

		int totalqueueSize = 0;

		fd_set readfds;
		fd_set writefds;
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);

		FD_SET(_socket, &readfds);

		int maxfd = _socket;

		for (CONN_MAP::iterator cit = _clients.begin(); cit != _clients.end(); cit++)
		{
			client_list* conn = cit->second;
			FD_SET(conn->socket, &readfds);
			if (conn->socket > maxfd)
			{
				maxfd = conn->socket;
			}
		}

		//ֻ����Ҫд��socket����set�У�Ϊ׼ȷ��ֵretValue����
		if (_queueJitt.size() > 0)
		{
			for (DataRowJitt::iterator it = _queueJitt.begin(); it != _queueJitt.end(); it++)
			{
				int queueSize = it->second->size();
				if (queueSize > 0)
				{
					totalqueueSize += queueSize;
					int sock = it->first;
					FD_SET(sock, &writefds);
					if (sock > maxfd)
						maxfd = sock;
				}
			}
		}

		timeval timeo;
		timeo.tv_sec = _timeout.tv_sec;
		timeo.tv_usec = _timeout.tv_usec;

		int fds = select(maxfd + 1, &readfds, &writefds, NULL, &timeo);

		if (fds > 0)
		{
			//����������
			if (fds > 0 && FD_ISSET(_socket, &readfds))
			{
				fds--;

				int iRet = 1;
				client_list* _dlg_coming = new client_list;
				struct sockaddr_in addr;
				int addr_len = sizeof(struct sockaddr_in);

#ifdef WIN32
				_dlg_coming->socket = accept(_socket, (struct sockaddr*)&addr, &addr_len);
#endif

				if (INVALID_SOCKET != _dlg_coming->socket)
				{
					_dlg_coming->online = 1;
					_dlg_coming->ip = addr.sin_addr.s_addr;
					_dlg_coming->port = addr.sin_port;
					_dlg_coming->id = GetNewConnectId();
					const char* ip = inet_ntoa(addr.sin_addr);

					int port = ntohs(addr.sin_port);
					if (_listener != NULL)
					{
						_mutex->unlock();
						iRet = this->_listener->onConnect(_engineId, _dlg_coming->id, ip, port);
						_mutex->lock();
					}

					if (0 == iRet) //Ӧ�ò���Ծ����Ƿ����������ӣ�onConnect�ķ�����0��ʾ���գ�1��ʾ�ܾ�
					{
						_clients[_dlg_coming->id] = _dlg_coming;

						const int optval = 1;

						if (_nodelay == 1)
						{
#ifdef WIN32
							if (setsockopt(_dlg_coming->socket, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval)) < 0) //�ر�nagel�㷨
#endif
							{
								printf("error setsockopt nodelay");
							}
						}

						if (_recvBuffSize > 0)
							setsockopt(_dlg_coming->socket, SOL_SOCKET, SO_RCVBUF, (char*)&_recvBuffSize, sizeof(_recvBuffSize));
						if (_sendBufferSize > 0)
							setsockopt(_dlg_coming->socket, SOL_SOCKET, SO_SNDBUF, (char*)&_sendBufferSize, sizeof(_sendBufferSize));

#ifdef WIN32
						unsigned long l = 1;
						int n = ioctlsocket(_dlg_coming->socket, FIONBIO, &l);
						if (n != 0)
						{
							int errcode = WSAGetLastError();
							printf("noblock set failed!\n");
							//printf();
						}
#endif
					}
					else
					{
						//printf("client error, refuse it!\n");
						closesocket(_dlg_coming->socket);
						delete _dlg_coming;
					}
				}
				else
				{
					printf("invalid dlg socket\n");
					delete _dlg_coming;
				}
			}

			//��ס�Զ��Ѿ��رյ�socket
			std::set<int> closedSockets;

			//���������
			int iRevLen = 0;

			if (fds > 0)
			{
				for (CONN_MAP::iterator it = _clients.begin(); it != _clients.end(); it++)
				{
					client_list* conn = it->second;

					if (conn == NULL)
						continue;

					int connId = conn->id;
					if (FD_ISSET(conn->socket, &readfds))//�������ж�online�Ƿ���Ч�����online��Ч���ܲ���socket,FD_ISSET�����        
					{
						fds--;
						iRevLen = recv(conn->socket, _buffer, _tpRecvBuffSize, 0);
						if (iRevLen <= 0)
						{
							closedSockets.insert(conn->socket);//����׼���رյ�socket
							if (_listener != NULL)
							{
								_mutex->unlock();
								_listener->onClose(_engineId, connId);//֪ͨӦ�ò�
								_mutex->lock();
							}
							CloseClient(connId);
							break;
						}
						else
						{
							if (_listener != NULL)
							{
								//_mutex->unlock();
								this->_listener->onData(_engineId, conn->id, _buffer, iRevLen);
								//_mutex->lock();
							}
						}
					}
				}
			}

			//����д����
			if (fds > 0)
			{
				DataRowPtr data;

				DataRowJitt::iterator it = _queueJitt.begin();
				while (it != _queueJitt.end())
				{
					DataRowList* ql = (it->second);
					if (NULL == ql)
					{
						//��Ӧ�ý���˷�֧
						_queueJitt.erase(it++);
						printf("what ? ? ? \n");
						continue;
					}

					int stop = ql->size();

					//�����stop��
					for (int i = 0; i < stop; i++)
					{
						data = NULL;
						if (ql->size() > 0)
						{
							data = ql->front();
							//������ָ�socket�Ѿ�����׼���رյ�socket
							//�򲻴�������socket������
							//��Ϊ���ܵ��ù�������ӵ�onClose,����onClose�ص��ã���Щ����һ��ᱻ����
							if (closedSockets.end() != closedSockets.find(data->socket))
								break;
							if (!FD_ISSET(data->socket, &writefds))
							{
								//�´μ�����
								break;
							}
						}
						else
						{
							break;
						}

						int iSend = __SendInside(data->connId, data->data, data->len);
						if (iSend < 0)
						{
							//����Ӧ���Ѷϣ����ﲻ�ô����´�heartbeat���⵽���ߡ�
							break;
						}
						else
						{
							if (iSend < data->len)
							{
								data->partDataSent = 1;
								//δȫ�����ͳɹ�
								//_mutex->unlock();
								int ret = _listener->onSendDataAck(_engineId, data->connId, data->sequence, iSend);
								//_mutex->lock();

								//���ݻص��ķ���ֵ����ͬ����
								if (0 == ret)
								{
									//����Ͷ������ֱ��ȫ�����ͳɹ�
									data->len = data->len - iSend;
									data->data = data->data + iSend;
								}
								else if (1 == ret)
								{
									//����ڲ�����
									int queueSize = ql->size();
									for (unsigned int i = 0; i < queueSize; i++)
									{
										data = ql->front();
										ql->pop();
									}
								}
								break;
							}
							else
							{
								totalqueueSize--;
								//������ɣ��ص�֪ͨ�ϲ�
								if (_listener != NULL)
								{
									//_mutex->unlock();
									_listener->onSendDataAck(_engineId, data->connId, data->sequence, 0);
									//_mutex->lock();
								}
							}
						}

						ql->pop();
					}

					it++;
				}
			}
		}
		else if (fds < 0)		//�����������
		{
			printf("TPTCPServer^_^select error\n");
			retValue = TP_ERROR_BASE;
		}
		else if (fds == 0) //���¼������
		{
			retValue = TP_NORMAL_RET;
		}

		//����Ƿ���Ҫ�ص��ڲ�״̬

	_out:
		_mutex->unlock();

		return retValue;
	}

	int TPTCPServer::Send(int connId, unsigned int sendid, char* pBuf, unsigned int iBufLen)
	{
		int ret = TP_NORMAL_RET;

		_mutex->lock();
		client_list* conn = NULL;
		CONN_MAP::iterator it = _clients.find(connId);
		if (it != _clients.end())
		{
			conn = it->second;
		}
		if (conn == NULL)
		{
			_mutex->unlock();
			return TP_ERROR_BAD_CONNECTION;
		}

		DataRowJitt::iterator itJitt = _queueJitt.find(conn->socket);
		if (itJitt == _queueJitt.end())
		{
			_queueJitt[conn->socket] = new DataRowList;
		}
		else
		{
			if (_maxDataQueueLength > 0)
			{
				//����Ƿ��ѵ�����󻺳峤��
				if (_queueJitt[conn->socket]->size() >= _maxDataQueueLength)
				{
					_mutex->unlock();
					return TP_ERROR_MAX_SENDQUEUE_LENGTH;
				}
			}
		}

		DataRowPtr row = createDataRow();

		row->partDataSent = 0;
		row->connId = connId;
		row->data = pBuf;
		row->len = iBufLen;
		row->socket = conn->socket;
		row->sequence = sendid;

		_queueJitt[row->socket]->push(row);
		_mutex->unlock();

		return ret;
	}

	inline int TPTCPServer::__SendInside(int connId, char* pBuf, unsigned int iBufLen)
	{
		client_list* bean = NULL;
		CONN_MAP::iterator it = _clients.find(connId);
		if (it != _clients.end())
		{
			bean = it->second;
		}
		if (NULL == bean || INVALID_SOCKET == bean->socket)
		{
			printf("TPTCPServer^_^socket invalid\n");
			//CloseClient(id);
			return TP_ERROR_BAD_CONNECTION;
		}

		if ((0 == iBufLen) || (NULL == pBuf))
		{
			return TP_NORMAL_RET;
		}

		int iSend = send(bean->socket, pBuf, iBufLen, 0);
		return iSend;
	}

	int TPTCPServer::__CloseInside()
	{
		int iRet = 0;

		for (CONN_MAP::iterator it = _clients.begin(); it != _clients.end(); it++)
		{
			client_list* bean = it->second;
			if (bean != NULL)
			{
				closesocket(bean->socket);
				delete bean;
			}
		}

		_clients.clear();

		if (_socket != INVALID_SOCKET)
		{
			iRet = closesocket(_socket);
			_socket = INVALID_SOCKET;
		}

		return iRet;
	}

	int TPTCPServer::Close()
	{
		_mutex->lock();
		int iRet = __CloseInside();

		DataRowJitt::iterator it = _queueJitt.begin();
		while (it != _queueJitt.end())
		{
			DataRowList* ql = it->second;
			if (ql != NULL)
			{
				int queueSize = ql->size();
				for (unsigned int i = 0; i < queueSize; i++)
				{
					DataRowPtr data = ql->front();
					ql->pop();
				}

				delete ql;
			}

			it++;
		}
		_queueJitt.clear();

		_mutex->unlock();
		return iRet;
	}

	int TPTCPServer::SetMaxDataQueueLength(int maxDataQueueLength)
	{
		_maxDataQueueLength = maxDataQueueLength;
		return 0;
	}
}