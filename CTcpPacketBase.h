#pragma once

#include "SmartPtr.h"
#include "TcpPacketHeader.h"

#define SIZE_SERIALIZEBUFFER 1024 //1K

class CTcpPacketBase;
typedef Utility::SmartPtr<CTcpPacketBase> TcpPacketBasePtr;

class CTcpPacketBase : public Utility::CSharedObject
{
public:
	CTcpPacketBase(void);
	virtual ~CTcpPacketBase(void);

public:
	//打包
	virtual int Serialize();
	//解包
	virtual int Deserialize(char* data, unsigned int len, unsigned int& useLen, bool& bComplete);

	//发送时使用
	char* GetPacket();
	unsigned int GetPacketLen() { return m_packetLen; }

	TcpPacketHeader* GetHeader() { return &m_header; }
	//包体
	std::string& GetBody() { return m_body; }
	void SetBody(const std::string& body);

	std::string GetPacketID();//辅助函数

	TcpPacketBasePtr CreateResponse();

protected:
	//打包缓冲区
	char m_serialBuf[SIZE_SERIALIZEBUFFER];
	unsigned int m_packetLen;
	std::vector<char> m_largePacket;
	//解包时已解析的长度
	kfint32 m_parseLen;

	//包头
	TcpPacketHeader m_header;
	//包体json串
	std::string m_body;

private:
	/* not allow copying and assignment. */
	CTcpPacketBase(const CTcpPacketBase&);
	CTcpPacketBase& operator= (const CTcpPacketBase&);
};

class CTcpPacketResponse : public CTcpPacketBase
{
public:
	CTcpPacketResponse(void);
	virtual ~CTcpPacketResponse(void);
};


typedef Utility::SmartPtr<CTcpPacketResponse> TcpPacketResponsePtr;

class CTcpPacketRequest : public CTcpPacketBase
{
public:
	CTcpPacketRequest(void);
	virtual ~CTcpPacketRequest(void);
};

typedef Utility::SmartPtr<CTcpPacketRequest> TcpPacketRequestPtr;