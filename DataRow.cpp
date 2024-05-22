#include "pch.h"
#include "DataRow.h"

namespace Utility
{
	DataRow::DataRow(CDataRowPool* pool)
	{
		this->_pool = pool;
		partDataSent = 0;
	}

	void DataRow::SetPool(CDataRowPool* pool)
	{
		this->_pool = pool;
	}

	void DataRow::recycle()
	{
		if (_pool != NULL)
		{
			partDataSent = 0;

			DataRowPtr datarow(this);
			_pool->Recycle(datarow);
		}
		else
		{
			delete this;
		}
	}

	//data row pool

	CDataRowPool::CDataRowPool()
	{

	}

	CDataRowPool::~CDataRowPool()
	{
		CThreadRecMutex::Lock guard(_mutex);

		DataRowQueue::iterator iter = _pool.begin();
		while (iter != _pool.end())
		{
			(*iter)->SetPool(NULL);
			iter++;
		}

		_pool.clear();
	}

	DataRowPtr CDataRowPool::CreateDataRow()
	{
		DataRowPtr dataRow;

		CThreadRecMutex::Lock guard(_mutex);

		if (_pool.size() == 0)
		{
			dataRow = new DataRow(this);
		}
		else
		{
			dataRow = _pool.front();
			_pool.pop_front();
		}

		return dataRow;
	}

	void CDataRowPool::Recycle(DataRowPtr dataRow)
	{
		CThreadRecMutex::Lock guard(_mutex);

		_pool.push_back(dataRow);

	}
}