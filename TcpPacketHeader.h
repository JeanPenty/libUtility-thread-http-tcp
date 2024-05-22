//TCP消息头定义
#pragma once

//big-edian顺序
#define TCP_PACKETTYPE_REQUEST 0x0100
#define TCP_PACKETTYPE_RESPONSE 0x0200

#define TCP_PROTOCOL_JSONOBJ 0x0100
#define TCP_PROTOCOL_JSONARR 0x0200

#define TCP_STATE_OK "ok"
#define TCP_STATE_NO "no"


//消息头中的跨字节类型使用big-endian
#define TCP_VERSION_LEN 4
#define TCP_CLIENTID_LEN 36
#define TCP_PACKETID_LEN 36
#define TCP_STATE_LEN 2

#pragma pack(push)
#pragma pack(1)
typedef struct _tagTcpPacketHeader
{
	kfint32 headlen;
	char version[TCP_VERSION_LEN];
	char clientid[TCP_CLIENTID_LEN];
	char packetid[TCP_PACKETID_LEN];
	int16 packettype;
	short protocol;
	char state[TCP_STATE_LEN];
	kfint32 bodylen;
} TcpPacketHeader;
#pragma pack(pop)

const int TcpPacketHeaderLen = sizeof(TcpPacketHeader);
