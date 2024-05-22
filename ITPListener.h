#pragma once

namespace Utility
{
	class ITPListener
	{
	public:
		//����ʱ���ݷ��ڶ�����,������heartbeatʱ��ʵ�ʷ���,���ͳɹ�ʱ��֮ǰ
		//��IDȷ�ϸ�������,�������������ָ��.����ֱ��ɾ��ָ����������.
		//* ע�⣺onSendDataAck�в������ٵ����κδ����ķ���������Send�ȣ�
		//* ��������������ʹ������д���
		//sendLen == 0��ʾ���ͳɹ��� !=0��ʾ���ַ��ͳɹ�����ʱӦ�ò�Ӧ������״�����������
		virtual int onSendDataAck(int engineId, int connId, unsigned int sendid, int sendLen) = 0;
		//�յ�����
		virtual int onData(int engineId, int connId, const char* data, int len) = 0;
		//�Է��ر�����,�ϲ��ڴ˻ص��д���δ���͵���������
		virtual int onClose(int engineId, int connId) = 0;

		//��������ר��
		//����ֵΪ0��ʾ���ܴ����ӣ�����ֵΪ1��ʾ�ܾ�����
		virtual int onConnect(int engineId, int connId, const char* ip, int port) = 0;
	};
}

