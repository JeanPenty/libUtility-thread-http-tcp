#include "pch.h"
#include "CTcpPacketTloss.h"

CTcpPacketTloss::CTcpPacketTloss(std::string strClientID)
	: CTcpPacketRequest()
	, m_clientID(strClientID)
{
}

CTcpPacketTloss::~CTcpPacketTloss()
{
}

int CTcpPacketTloss::Serialize()
{
	//�ȹ������
	std::string strBody = "{\"cmd\":\"tloss\"}";
	SetBody(strBody);

	//���޸İ�ͷ
	memcpy(m_header.clientid, m_clientID.c_str(), TCP_CLIENTID_LEN);

	//����uuid
	UUID uuid;
	UuidCreate(&uuid);
	unsigned char* str;
	UuidToStringA(&uuid, &str);
	std::string strUUID = reinterpret_cast<char*>(str);
	RpcStringFreeA(&str);

	std::string strPackID = strUUID;
	memcpy(m_header.packetid, strPackID.c_str(), TCP_PACKETID_LEN);

	return CTcpPacketBase::Serialize();
}
