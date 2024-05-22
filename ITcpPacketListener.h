#pragma once

class CTcpPacketBase;//ǰ������

typedef enum
{
	Pdu_Error_Parse_Failed = 1, //����ʧ��
	Pdu_Error_Buf_Full,			//������
}EnumPduError;

class ITcpPacketListener
{
public:
	virtual ~ITcpPacketListener() {}
public:
	//���ݰ�����
	virtual int onPacket(int engineId, int connId, CTcpPacketBase* pdu) = 0;
	//���ӹر�,�����ǶԶ˹رգ�Ҳ���������ӶϿ�
	virtual int onClose(int engineId, int connId) { return 0; }
	//�������
	virtual int onError(int engineId, int connId, EnumPduError error) { return 0; }
};

