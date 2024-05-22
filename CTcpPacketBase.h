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
	//���
	virtual int Serialize();
	//���
	virtual int Deserialize(char* data, unsigned int len, unsigned int& useLen, bool& bComplete);

	//����ʱʹ��
	char* GetPacket();
	unsigned int GetPacketLen() { return m_packetLen; }

	TcpPacketHeader* GetHeader() { return &m_header; }
	//����
	std::string& GetBody() { return m_body; }
	void SetBody(const std::string& body);

	std::string GetPacketID();//��������

	TcpPacketBasePtr CreateResponse();

protected:
	//���������
	char m_serialBuf[SIZE_SERIALIZEBUFFER];
	unsigned int m_packetLen;
	std::vector<char> m_largePacket;
	//���ʱ�ѽ����ĳ���
	kfint32 m_parseLen;

	//��ͷ
	TcpPacketHeader m_header;
	//����json��
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