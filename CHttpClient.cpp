#include "pch.h"
#include "CHttpClient.h"

#include <sstream>


#define HTTPCONNECT_TIMEOUT 10
#define HTTPOPERATION_TIMEOUT 60

CHttpClient::CHttpClient(void)
	: m_lStatusCode(0)
	, m_bUseProxy(false)
{
}

CHttpClient::~CHttpClient(void)
{

}

void CHttpClient::__ClearLastResponse()
{
	m_mapResponseField.clear();
	m_lStatusCode = 0;
}

void CHttpClient::AddRequestField(const std::string& strFieldName, const std::string& strFieldValue)
{
	m_mapRequestField.insert(std::make_pair(strFieldName, strFieldValue));
}

void CHttpClient::AddRequestField(const std::string& strFieldName, int nFieldValue)
{
	char szValue[100];
	sprintf(szValue, "%d", nFieldValue);
	m_mapRequestField.insert(std::make_pair(strFieldName, szValue));
}

void CHttpClient::ResetRequestField()
{
	m_mapRequestField.clear();
}

size_t CHttpClient::OnRecvHeader(char* buffer, size_t size, size_t nitems, void* userdata)
{
	long status = 0;
	CHttpClient* pClient = (CHttpClient*)userdata;
	if (NULL != pClient)
	{
		pClient->__ParseHeader(buffer, size, nitems);
	}
	return size * nitems;
}

void CHttpClient::__ParseHeader(char* buffer, size_t size, size_t nitems)
{
	//这个版本的libcurl的消息头回调是一行一行回调的
	if (memcmp(buffer, "HTTP/", 5) == 0)
	{//首行
		char* pStart = buffer + 5;
		//找到第一个空格
		while (((pStart - buffer) < size * nitems) && (*pStart) != ' ')
			pStart++;

		while (((pStart - buffer) < size * nitems) && (*pStart) == ' ')
			pStart++;

		if (((pStart + 3 - buffer) < size * nitems))
		{
			char szStatusCode[4] = { 0 };
			memcpy(szStatusCode, pStart, 3);
			m_lStatusCode = atol(szStatusCode);
		}

	}
	else
	{//头域行
		if (m_lStatusCode >= 200 && m_lStatusCode < 300)
		{//只解析在2**范围内的响应头域字段
			char* pStart = buffer;
			while (((pStart - buffer) < size * nitems) && (*pStart) != ':')
				pStart++;
			if ((pStart - buffer) < size * nitems)
			{
				char szFiled[200] = { 0 };
				memcpy(szFiled, buffer, pStart - buffer);

				pStart++;//跳过冒号

				while (((pStart - buffer) < size * nitems) && (*pStart) == ' ')
					pStart++;

				if ((pStart - buffer) < size * nitems)
				{
					char szValue[1024] = { 0 };
					memcpy(szValue, pStart, (size * nitems - 2 - (pStart - buffer)));//2是结尾的\r\n

					m_mapResponseField.insert(std::make_pair(szFiled, szValue));
				}
			}
		}
	}
}


size_t CHttpClient::OnRecvData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
	std::string* str = dynamic_cast<std::string*>((std::string*)lpVoid);
	if (NULL == str || NULL == buffer)
	{
		return -1;
	}

	char* pData = (char*)buffer;
	str->append(pData, size * nmemb);
	return nmemb * size;
}

int CHttpClient::Post(const std::string& strUrl, const std::string& strPost, std::string& strResponse)
{
	//先清除上次的响应头域
	//__ClearLastResponse();

	CURLcode res;
	CURL* curl = curl_easy_init();
	if (NULL == curl)
	{
		return CURLE_FAILED_INIT;
	}
	//URL
	curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
	//POST
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPost.c_str());
	//回调
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &CHttpClient::OnRecvHeader);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*)this);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CHttpClient::OnRecvData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&strResponse);
	//超时
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, HTTPCONNECT_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTPOPERATION_TIMEOUT);
	//重定向
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1);
	curl_easy_setopt(curl, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);
	//代理
	if (m_bUseProxy)
	{
		curl_easy_setopt(curl, CURLOPT_PROXY, m_strProxy.c_str());
	}

	//自定义头域
	struct curl_slist* headers = NULL;
	if (!m_mapRequestField.empty())
	{
		std::string strField;
		std::map<std::string, std::string>::iterator iter = m_mapRequestField.begin();
		for (; iter != m_mapRequestField.end(); iter++)
		{
			strField = iter->first + ": " + iter->second;
			headers = curl_slist_append(headers, strField.c_str());
		}

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	}

	res = curl_easy_perform(curl);

	if (headers)
	{
		curl_slist_free_all(headers);
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &m_lStatusCode);
	curl_easy_cleanup(curl);
	return res;
}


int CHttpClient::Get(const std::string& strUrl, std::string& strResponse)
{
	//先清除上次的响应头域
	__ClearLastResponse();

	CURLcode res;
	CURL* curl = curl_easy_init();
	if (NULL == curl)
	{
		return CURLE_FAILED_INIT;
	}
	//URL
	curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
	//回调
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CHttpClient::OnRecvData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&strResponse);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &CHttpClient::OnRecvHeader);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*)this);

	//超时
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, HTTPCONNECT_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTPOPERATION_TIMEOUT);
	//重定向
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1);
	//代理
	if (m_bUseProxy)
	{
		curl_easy_setopt(curl, CURLOPT_PROXY, m_strProxy.c_str());
	}

	//自定义头域
	struct curl_slist* headers = NULL;
	if (!m_mapRequestField.empty())
	{
		std::string strField;
		std::map<std::string, std::string>::iterator iter = m_mapRequestField.begin();
		for (; iter != m_mapRequestField.end(); iter++)
		{
			strField = iter->first + ": " + iter->second;
			headers = curl_slist_append(headers, strField.c_str());
		}

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	}

	res = curl_easy_perform(curl);

	if (headers)
	{
		curl_slist_free_all(headers);
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &m_lStatusCode);
	curl_easy_cleanup(curl);

	return res;
}