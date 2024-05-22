#pragma once

#include <string>
#include <map>

#include "CCurlClient.h"

class CHttpClient : public CCurlClient
{
public:
	CHttpClient(void);
	~CHttpClient(void);

public:
	/**
	* @POST文本数据，文本数据以'\0'结尾
	* @param strUrl 输入参数,请求的Url地址,如:http://www.baidu.com
	* @param strPost 输入参数,Post的内容(字符串格式)
	* @param strResponse 输出参数,返回的文本内容
	* @return 返回是否Post成功
	*/
	int Post(const std::string& strUrl, const std::string& strPost, std::string& strResponse);

	/**
	* @brief HTTP GET请求
	* @param strUrl 输入参数,请求的Url地址,如:http://www.baidu.com
	* @param strResponse 输出参数,返回的文本内容
	* @return 返回是否Post成功
	*/
	int Get(const std::string& strUrl, std::string& strResponse);

public:
	void	AddRequestField(const std::string& strFieldName, const std::string& strFieldValue);
	void	AddRequestField(const std::string& strFieldName, int nFieldValue);
	void	ResetRequestField();

public:
	static size_t OnRecvData(void* buffer, size_t size, size_t nmemb, void* lpVoid);
	static size_t OnRecvHeader(char* buffer, size_t size, size_t nitems, void* userdata);
private:
	void __ParseHeader(char* buffer, size_t size, size_t nitems);
	void __ClearLastResponse();

private:
	//需上层用户调用ResetRequestField手动清除m_mapRequestField，否则
	//每次执行Get或Post时都会重用上次的m_mapRequestField
	std::map<std::string, std::string> m_mapRequestField;

	long m_lStatusCode;
	//每次执行Get或Post之前底层会自动清除m_mapResponseField
	std::map<std::string, std::string> m_mapResponseField;
	bool m_bUseProxy;
	std::string m_strProxy;

};

