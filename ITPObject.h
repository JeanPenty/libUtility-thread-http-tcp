#pragma once

#include "Base.h"
#include "TPTypedef.h"
#include "ITPListener.h"
#include "DataRow.h"
#include "BaseMutex.h"


namespace Utility
{
	typedef std::queue<DataRowPtr> DataRowList;
#ifdef WIN32
	typedef std::unordered_map<int, DataRowList*> DataRowJitt;
#endif

#ifdef WIN32
	//以clientid作为键值
	typedef std::unordered_map<unsigned int, client_list*> CONN_MAP;
#endif

	class JP_UTIL_API ITPObject
	{
	public:
		//engineId 唯一标示ITPObject, 当一个程序中有多于一ITPObject时，需给engineId赋值
		ITPObject(ITPListener* instance, int engineId = 0);
		virtual ~ITPObject();

		void SetListener(ITPListener* listener);
		virtual int Heartbeat() = 0;

	public:
		//设置底层套接字缓冲区大小，type表示是发送缓冲区还是接收缓冲区
		int SetSocketBufferSize(TPType type, int size);

		//获取底层套接字缓冲区大小，type表示是发送缓冲区还是接收缓冲区
		int GetSocketBufferSize(TPType type);

		//设置传输层调用select时的超时值。如果不赋值则默认设sec=0;usec=10;
		//如果设置为0则表示为轮循状态
		int SetSelectTimeout(long sec, long usec);

		//对于同一个传输对象，下面的两个设置传输层缓冲区的函数不可同时使用
		//设置传输层接收缓冲区大小，缓冲区由传输层自己维护，应用层仅指定大小
		int SetTPRecvBuffSize(int size);

		//用于设置接收缓冲区，可以设定应用程序自己的接收缓冲区，传输层将数据直接接收在该区中。
		//传入的缓冲区要上层自行管理，下层不负责管理。
		int SetTPRecvBuffer(char* buff, int size);

		//使能nagle算法。1为打开，0为关闭
		int SetNodelayFlag(int flag);

	public:
		//传输层初始化，在主程序入口
		static void Startup(void);
		static void Cleanup(void);

	protected:
		long GetNewConnectId();

		unsigned int	_ip;		// IP,PORT 都是网络字节序
		int				_socket;
		unsigned short	_port;

	protected:
		ITPListener* _listener;
		CBaseMutex* _mutex;
		bool					m_bInnerMutex;

		int					_engineId;
		int					_nodelay;

		int				_recvBuffSize;
		int				_sendBufferSize;

		char* _buffer;
		int				_tpRecvBuffSize;
		bool			m_bLocalBuffer;

		long			_newConnectId;

		struct timeval	_timeout;

		CDataRowPool	 _dataRowPool;
		DataRowPtr createDataRow();
	};
}


