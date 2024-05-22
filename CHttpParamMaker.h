#pragma once
#include <string>
#include <map>
#include <list>
#include <iterator>

/////////////////////////////////////////////////////////////////////////////
//构造参数字符串 
//说明：用于构造如aaa=123&bbb=321&ccc=888之类的字符串
class CHttpParamMaker
{
public:
	CHttpParamMaker() {}
	virtual ~CHttpParamMaker() {}

public:
	void	add_param(const std::string& strKey, const std::string& strValue);
	void	add_param(const std::string& strKey, int nValue);
	void	set_paramlines(const std::string& strLines);
	std::string  get_params();
protected:
private:
	typedef struct
	{
		std::string strKey;
		std::string strValue;
	}HttpGetMakerParam;
	std::list<HttpGetMakerParam>	m_params;
	std::string m_strParamLines;
};

inline void CHttpParamMaker::add_param(const std::string& strKey, const std::string& strValue)
{
	HttpGetMakerParam param;
	param.strKey = strKey;
	param.strValue = strValue;
	m_params.push_back(param);
}

inline void CHttpParamMaker::add_param(const std::string& strKey, int nValue)
{
	char szValue[100];
	sprintf(szValue, "%d", nValue);
	add_param(strKey, szValue);
}

inline void CHttpParamMaker::set_paramlines(const std::string& strParamLines)
{
	m_strParamLines = strParamLines;
}

inline std::string CHttpParamMaker::get_params()
{
	if (!m_strParamLines.empty())
		return m_strParamLines;

	char szParams[8192] = { 0 };
	std::list<HttpGetMakerParam>::iterator it;
	for (it = m_params.begin(); it != m_params.end(); it++)
	{
		HttpGetMakerParam param = *it;

		char szTmp[8192];
		sprintf(szTmp, "%s=%s&", param.strKey.c_str(), param.strValue.c_str());
		strcat(szParams, szTmp);
	}
	if (szParams[strlen(szParams) - 1] == '&')
		szParams[strlen(szParams) - 1] = 0;
	return szParams;
}


//解析参数
//说明：用于解析如aaa=123&bbb=321&ccc=888之类的字符串
class CHttpParamParser
{
public:
	CHttpParamParser(const char* szHttpParam = NULL, int nLen = 0)
	{
		if (szHttpParam && nLen > 0)
		{
			parse(szHttpParam, nLen);
		}
	}
	virtual ~CHttpParamParser() {}

public:
	bool parse(const char* szHttpParam, int nLen);
	std::string get_param(const char* szKey);
	int get_param_int(const char* szKey);

private:
	std::map<std::string, std::string> m_mapValues;
};

inline bool CHttpParamParser::parse(const char* szHttpParam, int nLen)
{
	//清理内存
	m_mapValues.clear();

	//复制到缓冲
	char* szBuffer = new char[4096 + nLen];
	memcpy(szBuffer, szHttpParam, nLen);
	szBuffer[nLen] = 0;
	if (szBuffer[strlen(szBuffer) - 1] != '&')
		strcat(szBuffer, "&");

	//处理参数
	char* szParam = szBuffer;
	while (1)
	{
		if (szParam[0] == '\0')
			break;

		if (!((szParam[0] >= 'a' && szParam[0] <= 'z') || (szParam[0] >= 'A' && szParam[0] <= 'Z')))
		{
			szParam++;
			continue;
		}

		char* szValue = strchr(szParam, '=');
		if (!szValue)
			break;

		char* szSplit = strchr(szParam, '&');
		if (!szSplit)
			break;

		std::string strKey;
		std::string strValue;

		char szTmp[1024];
		//提取key
		memset(szTmp, 0, sizeof(szTmp));
		memcpy(szTmp, szParam, szValue - szParam);
		strKey = szTmp;

		//提取value
		memset(szTmp, 0, sizeof(szTmp));
		memcpy(szTmp, szValue + 1, szSplit - szValue - 1);
		strValue = szTmp;

		//保存
		m_mapValues.insert(std::make_pair(strKey, strValue));
		//printf("parse http request, key:%s, value:%s \r\n", strKey.c_str(), strValue.c_str());

		//下一个参数
		szParam = szSplit + 1;
	}

	delete[]szBuffer;
	return true;
}

inline std::string CHttpParamParser::get_param(const char* szKey)
{
	std::map<std::string, std::string>::iterator it = m_mapValues.find(szKey);
	if (it != m_mapValues.end())
	{
		return it->second;
	}
	else
	{
		return "";
	}
}

inline int CHttpParamParser::get_param_int(const char* szKey)
{
	return atoi(get_param(szKey).c_str());
}

