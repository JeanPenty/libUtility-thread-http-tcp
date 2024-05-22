#include "pch.h"
#include "CTcpPacketBase.h"


CTcpPacketBase::CTcpPacketBase(void)
{
	m_parseLen = 0;
	memset(&m_header, 0, TcpPacketHeaderLen);
	m_header.headlen = htonl(TcpPacketHeaderLen);
	m_header.version[0] = 0x01;
	// 	m_header.version[1] = 0x00;
	// 	m_header.version[2] = 0x00;
	// 	m_header.version[3] = 0x00;
	m_header.protocol = TCP_PROTOCOL_JSONOBJ;
	memcpy(m_header.state, TCP_STATE_OK, TCP_STATE_LEN);
	m_header.bodylen = 0;
	m_body = "";
}

CTcpPacketBase::~CTcpPacketBase(void)
{
	//
}

int CTcpPacketBase::Serialize()
{
	//调用打包函数时,m_header和m_strBody必须都已准备好
	long bodyLen = m_body.length();
	m_header.bodylen = htonl(bodyLen);
	m_packetLen = TcpPacketHeaderLen + bodyLen;

	if (m_packetLen <= SIZE_SERIALIZEBUFFER)
	{
		memcpy(m_serialBuf, &m_header, TcpPacketHeaderLen);

		if (bodyLen != 0)
		{
			memcpy(m_serialBuf + TcpPacketHeaderLen, m_body.c_str(), bodyLen);
		}
	}
	else
	{
		m_largePacket.resize(m_packetLen);
		assert(m_largePacket.size() == m_packetLen);
		memcpy(&m_largePacket[0], &m_header, TcpPacketHeaderLen);

		if (bodyLen != 0)
		{
			memcpy(&m_largePacket[TcpPacketHeaderLen], m_body.c_str(), bodyLen);
		}
	}

	return 0;
}

int CTcpPacketBase::Deserialize(char* data, unsigned int len, unsigned int& useLen, bool& bComplete)
{
	assert(data != NULL && len != 0);

	char* curData = data;
	unsigned int remainLen = len;
	if (m_parseLen < TcpPacketHeaderLen)
	{//先解析包头
		int copyLen = min(TcpPacketHeaderLen - m_parseLen, remainLen);
		memcpy(&m_header + m_parseLen, curData, copyLen);
		curData += copyLen;
		remainLen -= copyLen;
		m_parseLen += copyLen;
	}

	if (m_parseLen >= TcpPacketHeaderLen)
	{//已解析出了包头
		unsigned long bodyLen = /*m_header.bodylen;*/  ntohl(m_header.bodylen);
		if (0 == bodyLen)
		{
			bComplete = true;
		}
		else
		{
			if (remainLen > 0)
			{//再解析包体
				int copyLen = min(bodyLen - (m_parseLen - TcpPacketHeaderLen), remainLen);
				m_body.append(curData, copyLen);
				curData += copyLen;
				remainLen -= copyLen;
				m_parseLen += copyLen;

				if (m_parseLen == (TcpPacketHeaderLen + bodyLen))
				{//包数据接收完毕，为json串添加'\0'，便于以后解析
					m_body.append(1, '\0');
					bComplete = true;
				}
			}
		}
	}

	useLen = len - remainLen;

	return 0;
}

char* CTcpPacketBase::GetPacket()
{
	if (m_packetLen <= SIZE_SERIALIZEBUFFER)
	{
		return m_serialBuf;
	}
	else
	{
		return &m_largePacket[0];
	}
}

void CTcpPacketBase::SetBody(const std::string& body)
{
	m_body = body;
}

std::string CTcpPacketBase::GetPacketID()
{
	std::string strID;
	strID.append(m_header.packetid, TCP_PACKETID_LEN);
	return strID;
}

TcpPacketBasePtr CTcpPacketBase::CreateResponse()
{
	TcpPacketBasePtr ptr;
	if (TCP_PACKETTYPE_REQUEST == m_header.packettype)
	{
		CTcpPacketBase* pPack = new CTcpPacketBase();
		if (pPack != NULL)
		{
			TcpPacketHeader* pHeader = pPack->GetHeader();
			memcpy(pHeader, &m_header, TcpPacketHeaderLen);
			pHeader->packettype = TCP_PACKETTYPE_RESPONSE;
			memcpy(pHeader->state, TCP_STATE_OK, TCP_STATE_LEN);

			ptr = pPack;
		}
	}

	return ptr;
}


CTcpPacketResponse::CTcpPacketResponse()
	:CTcpPacketBase()
{
	m_header.packettype = TCP_PACKETTYPE_RESPONSE;
}

CTcpPacketResponse::~CTcpPacketResponse()
{
}

CTcpPacketRequest::CTcpPacketRequest()
	:CTcpPacketBase()
{
	m_header.packettype = TCP_PACKETTYPE_REQUEST;
}

CTcpPacketRequest::~CTcpPacketRequest()
{
}
