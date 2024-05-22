#pragma once

namespace Utility
{
	class ITPListener
	{
	public:
		//发送时数据放在队列中,当心跳heartbeat时才实际发送,发送成功时将之前
		//的ID确认给调用者,用于清除数据区指针.发送直接删除指针会引起错误.
		//* 注意：onSendDataAck中不允许再调用任何传输层的方法，比如Send等，
		//* 否则会引起死锁和传输层队列错误。
		//sendLen == 0表示发送成功， !=0表示部分发送成功（此时应用层应对网络状况有所察觉）
		virtual int onSendDataAck(int engineId, int connId, unsigned int sendid, int sendLen) = 0;
		//收到数据
		virtual int onData(int engineId, int connId, const char* data, int len) = 0;
		//对方关闭连接,上层在此回调中处理未发送的所有数据
		virtual int onClose(int engineId, int connId) = 0;

		//服务器端专用
		//返回值为0表示接受此连接，返回值为1表示拒绝接受
		virtual int onConnect(int engineId, int connId, const char* ip, int port) = 0;
	};
}

