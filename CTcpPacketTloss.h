#pragma once

#include "CTcpPacketBase.h"

class CTcpPacketTloss : public CTcpPacketRequest
{
public:
	CTcpPacketTloss(std::string strClientID);
	virtual ~CTcpPacketTloss(void);

public:
	virtual int Serialize();

private:
	std::string m_clientID;
};

typedef Utility::SmartPtr<CTcpPacketTloss> CTcpPacketTlossPtr;
