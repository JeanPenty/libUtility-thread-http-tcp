#pragma once

#include "ITcpPacketListener.h"
#include "CTcpPacketBase.h"

class CTcpPacketParser
{
public:
	CTcpPacketParser(ITcpPacketListener* listener, int engineId = 0);
	~CTcpPacketParser(void);

	void parse(char* data, int len);
	void reset();

private:
	void parseInside(char* data, int len);

private:
	int					m_engineId;
	ITcpPacketListener* m_listener;

	TcpPacketBasePtr   m_ptrPartPacket;
};

