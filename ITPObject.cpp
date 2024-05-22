#include "pch.h"
#include "ITPObject.h"

namespace Utility
{
	ITPObject::ITPObject(ITPListener* instance, int engineId /* = 0 */)
	{
		this->_listener = instance;

		_buffer = new char[BUF_SIZE];
		m_bLocalBuffer = true;
		_tpRecvBuffSize = BUF_SIZE;

		_socket = INVALID_SOCKET;
		_ip = INADDR_ANY;
		_port = 0;

		_engineId = engineId;

		_recvBuffSize = 64 * 1024; //16 * 1024;
		_sendBufferSize = 64 * 1024; //16 * 1024;

		_timeout.tv_usec = 1;
		_timeout.tv_sec = 0;

		_nodelay = 0;

		_newConnectId = 0;
	}

	ITPObject::~ITPObject()
	{
		if (m_bLocalBuffer && _buffer != NULL)
		{
			delete[]_buffer;
			_buffer = NULL;
		}
	}

	void ITPObject::Startup(void)
	{
#ifdef WIN32
		WORD version_requested = MAKEWORD(2, 2);
		WSADATA wsa_data;
		int error = WSAStartup(version_requested, &wsa_data);
#endif
	}

	void ITPObject::Cleanup(void)
	{
#ifdef WIN32
		WSACleanup();
#endif
	}

	void ITPObject::SetListener(ITPListener* listener)
	{
		assert(listener != NULL);
		this->_listener = listener;
	}

	int ITPObject::SetSocketBufferSize(TPType type, int size)
	{
		LockPtrT<CBaseMutex> guard(_mutex);

		int ret = 0;

		if (size >= 0)
		{
			switch (type)
			{
			case TP_SEND:
				_sendBufferSize = size;
				break;
			case TP_RECEIVE:
				_recvBuffSize = size;
				break;
			default:
				ret = -1;
			}
		}
		else
		{
			ret = -2;
		}

		return ret;
	}

	int ITPObject::GetSocketBufferSize(TPType type)
	{
		switch (type)
		{
		case TP_SEND:
			return _sendBufferSize;
		case TP_RECEIVE:
			return _recvBuffSize;
		default:
			return -1;
		}
	}

	int ITPObject::SetSelectTimeout(long sec, long usec)
	{
		_mutex->lock();
		int ret = 0;

		if (sec >= 0 && usec >= 0)
		{
			_timeout.tv_usec = usec;
			_timeout.tv_sec = sec;
		}
		else
		{
			ret = -1;
		}

		_mutex->unlock();
		return ret;
	}

	int ITPObject::SetTPRecvBuffSize(int size)
	{
		_mutex->lock();

		int ret = 0;

		if (size > 0 && size < 1024 * 1024)
		{
			_tpRecvBuffSize = size;
			char* p = new char[_tpRecvBuffSize];

			if (NULL != p)
			{
				delete _buffer;
				_buffer = p;
				ret = 0;
			}
			else
			{
				ret = -2;
			}
		}
		else
		{
			ret = -1;
		}

		_mutex->unlock();
		return ret;
	}

	int ITPObject::SetTPRecvBuffer(char* buff, int size)
	{
		_mutex->lock();

		if (m_bLocalBuffer)
		{
			delete _buffer;
			m_bLocalBuffer = false;
		}


		_buffer = buff;
		_tpRecvBuffSize = size;

		_mutex->unlock();

		return 0;
	}

	DataRowPtr ITPObject::createDataRow()
	{
		return _dataRowPool.CreateDataRow();
	}

	int ITPObject::SetNodelayFlag(int flag)
	{
		_mutex->lock();
		_nodelay = flag;
		_mutex->unlock();
		return 0;
	}

	long ITPObject::GetNewConnectId()
	{
#ifdef WIN32
		return InterlockedIncrement(&_newConnectId);
#endif
	}
}