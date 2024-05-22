#pragma once

class CTcpPacketBase;//前向声明

typedef enum
{
	Pdu_Error_Parse_Failed = 1, //解析失败
	Pdu_Error_Buf_Full,			//缓冲满
}EnumPduError;

class ITcpPacketListener
{
public:
	virtual ~ITcpPacketListener() {}
public:
	//数据包到达
	virtual int onPacket(int engineId, int connId, CTcpPacketBase* pdu) = 0;
	//连接关闭,可能是对端关闭，也可能是连接断开
	virtual int onClose(int engineId, int connId) { return 0; }
	//具体错误
	virtual int onError(int engineId, int connId, EnumPduError error) { return 0; }
};

