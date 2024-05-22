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

		//将socket设置为非阻塞
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
			//处理读事件
			if (fdnums > 0 && FD_ISSET(_socket, &readfds))
			{
				fdnums--;	//将处理掉的事件减去

				iRevLen = recv(_socket, _buffer, _tpRecvBuffSize, 0);
				if (iRevLen <= 0)
				{
					int nNetError = WSAGetLastError();
					std::ostringstream os;
					os << "[TPTCPClient]对端主动关闭or网络异常 recv return = " << iRevLen << " Error = " << nNetError;
					//print log

					//是先通知上层还是先关闭需要再研究,因为可能会影响上层应用程序.
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

			//处理写事件
			if (fdnums > 0 && FD_ISSET(_socket, &writefds))
			{
				int stop = _queue.size();
				for (int i = 0; i < stop; i++)
				{
					DataRowPtr data = _queue.front();

					int iSend = _SendInside(data->data, data->len);
					if (iSend < 0)
					{
						//连接应该已断，这里不用处理，下次heartbeat会检测到断线。
						break;
					}
					else
					{
						if (iSend < data->len)
						{
							//	_sendStatistic++;
							//未全部发送成功
							//_mutex->unlock();
							int ret = _listener->onSendDataAck(_engineId, data->connId, data->sequence, iSend);
							//_mutex->lock();

							//根据回调的返回值做不同处理
							if (0 == ret)
							{
								//继续投递请求，直至全部发送成功
								data->len = data->len - iSend;
								data->data = data->data + iSend;
							}
							else if (1 == ret)
							{
								//清空内部缓冲
								while (_queue.size() != 0)
								{
									_queue.pop();
								}
							}
							break;
						}
						else
						{
							//发送完成，回调通知上层
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
		else if (fdnums < 0) //有错误发生，错误的原因有非常多，需要由应用层决定如何处理
		{
			printf("client select error\n");
			retValue = TP_ERROR_BASE;
		}
		else if (fdnums == 0) //无事件
		{
			retValue = TP_NORMAL_RET;
		}

		//检查是否需要回调内部状态
		queueSize = _queue.size();
		{
			//此处将来可以加上通知客户缓冲队列状态的回调
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
			//检查是否已到达最大缓冲长度
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