#pragma once

#include "Base.h"
#include "SmartPtr.h"
#include "ThreadRecMutex.h"


namespace Utility
{
	class CDataRowPool;
	class JP_UTIL_API DataRow : public CSharedObject
	{
		friend class CDataRowPool;
	public:
		char* data;
		int len;
		unsigned int sequence;//Ψһ��ʾ��DataRow	
		int partDataSent;//0���������� 1:���ַ���		
		int connId;//������ʹ��ʱ����Ψһ��ʾ���ӵĿͻ��ˣ��ͻ���ʹ��ʱ�̶�Ϊ0
		int socket;//���ʹ�DataRowʹ�õ�socket�ӿ�

	public:
		virtual void recycle();

	protected:
		DataRow(CDataRowPool* pool);
		void SetPool(CDataRowPool* pool);
		CDataRowPool* _pool;
	};

	typedef SmartPtr<DataRow> DataRowPtr;
	typedef std::deque<DataRowPtr> DataRowQueue;

	class  JP_UTIL_API CDataRowPool
	{
		friend class DataRow;
	public:
		CDataRowPool();
		~CDataRowPool();

		DataRowPtr CreateDataRow();

	private:
		void	Recycle(DataRowPtr dataRow);

	private:
		DataRowQueue _pool;
		CThreadRecMutex _mutex;
	};
}


