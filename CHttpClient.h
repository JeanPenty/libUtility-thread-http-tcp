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
	* @POST�ı����ݣ��ı�������'\0'��β
	* @param strUrl �������,�����Url��ַ,��:http://www.baidu.com
	* @param strPost �������,Post������(�ַ�����ʽ)
	* @param strResponse �������,���ص��ı�����
	* @return �����Ƿ�Post�ɹ�
	*/
	int Post(const std::string& strUrl, const std::string& strPost, std::string& strResponse);

	/**
	* @brief HTTP GET����
	* @param strUrl �������,�����Url��ַ,��:http://www.baidu.com
	* @param strResponse �������,���ص��ı�����
	* @return �����Ƿ�Post�ɹ�
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
	//���ϲ��û�����ResetRequestField�ֶ����m_mapRequestField������
	//ÿ��ִ��Get��Postʱ���������ϴε�m_mapRequestField
	std::map<std::string, std::string> m_mapRequestField;

	long m_lStatusCode;
	//ÿ��ִ��Get��Post֮ǰ�ײ���Զ����m_mapResponseField
	std::map<std::string, std::string> m_mapResponseField;
	bool m_bUseProxy;
	std::string m_strProxy;

};

