#include "pch.h"
#include "TPTCPClient.h"

namespace Utility
{
	TPTCPClient::TPTCPClient(ITPListener* tcpclientapp, int engineId) :ITPObject(tcpclientapp, engineId)
	{
		_mutex = new CNullMutex();
		m_bInnerMutex = true;
		_maxDataQueueLength = 0;
	}

	TPTCPClient::TPTCPClient(ITPListener* tcpclientapp, CBaseMutex* mutex, int engineId)
		:ITPObject(tcpclientapp, engineId)
	{
		if (mutex != NULL)
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

	TPTCPClient::~TPTCPClient()
	{
		if (m_bInnerMutex)
		{
			delete _mutex;
			_mutex = NULL;
		}
	}

	int TPTCPClient::Connect(const char* ip, int port)
	{
		_mutex->lock();

		_ip = inet_addr(ip);
		_port = htons(port);

		if (INVALID_SOCKET == _socket)
		{
			_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		}

		struct sockaddr_in remote_addr;
		memset(&remote_addr, 0, sizeof(remote_addr));

		remote_addr.sin_family = AF_INET;
		remote_addr.sin_addr.s_addr = _ip;
		remote_addr.sin_port = _port;

		//��socket����Ϊ������
#ifdef WIN32
		unsigned long l = 1;
		int n = ioctlsocket(_socket, FIONBIO, &l);
		if (n != 0)
		{
			int errcode = WSAGetLastError();
			_mutex->unlock();
			//return errcode;
			return TP_ERROR_SET_NOBLOCKING_FAILED;
		}

#endif


		if (_nodelay == 1)
		{
			const int optval = 1;
#ifdef WIN32
			if (setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval)) < 0)
#endif
			{
				printf("error setsockopt nodelay");
			}
		}

		if (_recvBuffSize > 0)
			setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (char*)&_recvBuffSize, sizeof(_recvBuffSize));
		if (_sendBufferSize > 0)
			setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, (char*)&_sendBufferSize, sizeof(_sendBufferSize));

		//connect
		int ret = connect(_socket, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr));
		if (SOCKET_ERROR == ret)
		{
			fd_set wds;
			FD_ZERO(&wds);
			FD_SET(_socket, &wds);

			timeval timeo;
			timeo.tv_sec = _timeout.tv_sec;
			timeo.tv_usec = _timeout.tv_usec;

			int iRet = select(_socket + 1, NULL, &wds, NULL, &timeo);

			if (iRet > 0 && FD_ISSET(_socket, &wds))
			{
				int error = -1;
				int llen = sizeof(int);
#ifdef WIN32
				getsockopt(_socket, SOL_SOCKET, SO_ERROR, (char*)&error, &llen);
#endif

				if (error == 0)
					ret = TP_NORMAL_RET;
				else
				{
					_CloseInside();
					ret = TP_ERROR_CONNECTFAILED;
				}

				_mutex->unlock();
				return ret;
			}
			else
			{
				_CloseInside();
				_mutex->unlock();
				return TP_ERROR_CONNECTFAILED;
			}
		}

		_mutex->unlock();

		return TP_NORMAL_RET;
	}

	int TPTCPClient::Close()
	{
		_mutex->lock();

		int iRet = _CloseInside();

		_mutex->unlock();
		return iRet;
	}

	int TPTCPClient::_CloseInside()
	{
		_mutex->lock();

		int iRet = 0;

		if (_socket != INVALID_SOCKET)
		{
			iRet = closesocket(_socket);
			_socket = INVALID_SOCKET;
		}

		while (_queue.size() != 0)
		{
			_queue.pop();
		}

		_mutex->unlock();

		return iRet;
	}

	int TPTCPClient::Heartbeat()
	{
		int retValue = TP_NORMAL_RET;
		_mutex->lock();

		if (INVALID_SOCKET == _socket)
		{
			_mutex->unlock();
			return TP_ERROR_BASE;
		}

		int iRevLen = 0;
		int queueSize = 0;

		fd_set readfds;
		fd_set writefds;

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_SET(_socket, &readfds);

		if (_queue.size() > 0)
			FD_SET(_socket, &writefds);

		struct timeval timeo;
		timeo.tv_sec = _timeout.tv_sec;
		timeo.tv_usec = _timeout.tv_usec;

		int fdnums = select(_socket + 1, &readfds, &writefds, NULL, &timeo);

		if (fdnums > 0)
		{
			//������¼�
			if (fdnums > 0 && FD_ISSET(_socket, &readfds))
			{
				fdnums--;	//����������¼���ȥ

				iRevLen = recv(_socket, _buffer, _tpRecvBuffSize, 0);
				if (iRevLen <= 0)
				{
					int nNetError = WSAGetLastError();
					std::ostringstream os;
					os << "[TPTCPClient]�Զ������ر�or�����쳣 recv return = " << iRevLen << " Error = " << nNetError;
					//print log

					//����֪ͨ�ϲ㻹���ȹر���Ҫ���о�,��Ϊ���ܻ�Ӱ���ϲ�Ӧ�ó���.
					if (_listener != NULL)
					{
						_mutex->unlock();
						_listener->onClose(_engineId, 0);
						_mutex->lock();
					}
					_CloseInside();

					retValue = TP_NORMAL_RET;
					goto _out;
				}
				else
				{
					if (_listener != NULL)
					{
						//_mutex->unlock();
						this->_listener->onData(_engineId, 0, _buffer, iRevLen);
						//_mutex->lock();
					}
				}
			}

			//����д�¼�
			if (fdnums > 0 && FD_ISSET(_socket, &writefds))
			{
				int stop = _queue.size();
				for (int i = 0; i < stop; i++)
				{
					DataRowPtr data = _queue.front();

					int iSend = _SendInside(data->data, data->len);
					if (iSend < 0)
					{
						//����Ӧ���Ѷϣ����ﲻ�ô����´�heartbeat���⵽���ߡ�
						break;
					}
					else
					{
						if (iSend < data->len)
						{
							//	_sendStatistic++;
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
								while (_queue.size() != 0)
								{
									_queue.pop();
								}
							}
							break;
						}
						else
						{
							//������ɣ��ص�֪ͨ�ϲ�
							if (_listener != NULL)
							{
								//_mutex->unlock();
								_listener->onSendDataAck(_engineId, data->connId, data->sequence, 0);
								//_mutex->lock();
							}
						}
					}

					_queue.pop();
				}
			}
		}
		else if (fdnums < 0) //�д������������ԭ���зǳ��࣬��Ҫ��Ӧ�ò������δ���
		{
			printf("client select error\n");
			retValue = TP_ERROR_BASE;
		}
		else if (fdnums == 0) //���¼�
		{
			retValue = TP_NORMAL_RET;
		}

		//����Ƿ���Ҫ�ص��ڲ�״̬
		queueSize = _queue.size();
		{
			//�˴��������Լ���֪ͨ�ͻ��������״̬�Ļص�
		}

	_out:
		_mutex->unlock();

		return retValue;
	}

	int TPTCPClient::SetMaxDataQueueLength(int maxDataQueueLength)
	{
		_maxDataQueueLength = maxDataQueueLength;
		return 0;
	}

	int TPTCPClient::Send(unsigned int sendid, char* pBuf, unsigned int iBufLen)
	{
		int ret = TP_NORMAL_RET;

		_mutex->lock();
		if (_maxDataQueueLength > 0)
		{
			//����Ƿ��ѵ�����󻺳峤��
			if (_queue.size() >= _maxDataQueueLength)
			{
				_mutex->unlock();
				return TP_ERROR_MAX_SENDQUEUE_LENGTH;
			}
		}

		DataRowPtr row = createDataRow();

		row->partDataSent = 0;
		row->connId = 0;
		row->data = pBuf;
		row->len = iBufLen;
		row->socket = _socket;
		row->sequence = sendid;

		_queue.push(row);

		_mutex->unlock();

		return ret;
	}

	int TPTCPClient::_SendInside(char* pBuf, unsigned int iBufLen)
	{
		if (INVALID_SOCKET == _socket)
		{
			printf("socket invalid\n");
			return TP_ERROR_BASE;
		}

		if ((0 == iBufLen) || (NULL == pBuf))
		{
			return TP_NORMAL_RET;
		}

		int iSend = send(_socket, pBuf, iBufLen, 0);

		return iSend;
	}
}