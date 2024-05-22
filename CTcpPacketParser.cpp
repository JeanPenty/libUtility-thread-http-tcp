#include "pch.h"
#include "CTcpPacketParser.h"

CTcpPacketParser::CTcpPacketParser(ITcpPacketListener* listener, int engineId)
	: m_engineId(engineId)
	, m_listener(listener)
	, m_ptrPartPacket(NULL)
{
}

CTcpPacketParser::~CTcpPacketParser(void)
{
}

void CTcpPacketParser::parse(char* data, int len)
{
	char* curData = data;
	int remainLen = len;
	if (m_ptrPartPacket != NULL)
	{
		unsigned int useLen = 0;
		bool bComplete = false;
		m_ptrPartPacket->Deserialize(curData, remainLen, useLen, bComplete);
		curData += useLen;
		remainLen -= useLen;
		if (bComplete)
		{
			m_listener->onPacket(m_engineId, 0, m_ptrPartPacket.get());
			m_ptrPartPacket = NULL;
		}

		if (remainLen > 0)
		{
			parseInside(curData, remainLen);
		}

	}
	else
	{
		parseInside(curData, remainLen);
	}
}

void CTcpPacketParser::parseInside(char* data, int len)
{
	char* curData = data;
	int remainLen = len;
	TcpPacketBasePtr   ptrPacket;

	while (remainLen > 0)
	{
		unsigned int useLen = 0;
		bool bComplete = false;
		ptrPacket = new CTcpPacketBase();
		ptrPacket->Deserialize(curData, remainLen, useLen, bComplete);
		curData += useLen;
		remainLen -= useLen;

		if (bComplete)
		{
			m_listener->onPacket(m_engineId, 0, ptrPacket.get());

		}
		else
		{
			m_ptrPartPacket = ptrPacket;
		}

		ptrPacket = NULL;
	}
}

void CTcpPacketParser::reset()
{
	m_ptrPartPacket = NULL;
}
