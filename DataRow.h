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
		unsigned int sequence;//唯一标示此DataRow	
		int partDataSent;//0：完整发送 1:部分发送		
		int connId;//服务器使用时用来唯一标示连接的客户端；客户端使用时固定为0
		int socket;//发送此DataRow使用的socket接口

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


