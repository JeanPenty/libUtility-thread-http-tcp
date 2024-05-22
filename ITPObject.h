#pragma once

#include "Base.h"
#include "TPTypedef.h"
#include "ITPListener.h"
#include "DataRow.h"
#include "BaseMutex.h"


namespace Utility
{
	typedef std::queue<DataRowPtr> DataRowList;
#ifdef WIN32
	typedef std::unordered_map<int, DataRowList*> DataRowJitt;
#endif

#ifdef WIN32
	//��clientid��Ϊ��ֵ
	typedef std::unordered_map<unsigned int, client_list*> CONN_MAP;
#endif

	class JP_UTIL_API ITPObject
	{
	public:
		//engineId Ψһ��ʾITPObject, ��һ���������ж���һITPObjectʱ�����engineId��ֵ
		ITPObject(ITPListener* instance, int engineId = 0);
		virtual ~ITPObject();

		void SetListener(ITPListener* listener);
		virtual int Heartbeat() = 0;

	public:
		//���õײ��׽��ֻ�������С��type��ʾ�Ƿ��ͻ��������ǽ��ջ�����
		int SetSocketBufferSize(TPType type, int size);

		//��ȡ�ײ��׽��ֻ�������С��type��ʾ�Ƿ��ͻ��������ǽ��ջ�����
		int GetSocketBufferSize(TPType type);

		//���ô�������selectʱ�ĳ�ʱֵ���������ֵ��Ĭ����sec=0;usec=10;
		//�������Ϊ0���ʾΪ��ѭ״̬
		int SetSelectTimeout(long sec, long usec);

		//����ͬһ���������������������ô���㻺�����ĺ�������ͬʱʹ��
		//���ô������ջ�������С���������ɴ�����Լ�ά����Ӧ�ò��ָ����С
		int SetTPRecvBuffSize(int size);

		//�������ý��ջ������������趨Ӧ�ó����Լ��Ľ��ջ�����������㽫����ֱ�ӽ����ڸ����С�
		//����Ļ�����Ҫ�ϲ����й����²㲻�������
		int SetTPRecvBuffer(char* buff, int size);

		//ʹ��nagle�㷨��1Ϊ�򿪣�0Ϊ�ر�
		int SetNodelayFlag(int flag);

	public:
		//������ʼ���������������
		static void Startup(void);
		static void Cleanup(void);

	protected:
		long GetNewConnectId();

		unsigned int	_ip;		// IP,PORT ���������ֽ���
		int				_socket;
		unsigned short	_port;

	protected:
		ITPListener* _listener;
		CBaseMutex* _mutex;
		bool					m_bInnerMutex;

		int					_engineId;
		int					_nodelay;

		int				_recvBuffSize;
		int				_sendBufferSize;

		char* _buffer;
		int				_tpRecvBuffSize;
		bool			m_bLocalBuffer;

		long			_newConnectId;

		struct timeval	_timeout;

		CDataRowPool	 _dataRowPool;
		DataRowPtr createDataRow();
	};
}


