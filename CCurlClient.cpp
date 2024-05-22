#include "pch.h"
#include "CCurlClient.h"

CCurlClient::CCurlClient(void)
{
	//
}

CCurlClient::~CCurlClient(void)
{
	//
}

bool CCurlClient::Initialize()
{
	if (curl_global_init(CURL_GLOBAL_WIN32) != CURLE_OK)
	{
		return false;
	}
	return true;
}

void CCurlClient::UnInitialize()
{
	curl_global_cleanup();
}