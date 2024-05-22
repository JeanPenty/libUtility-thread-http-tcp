#pragma once
class CCurlClient
{
public:
	CCurlClient(void);
	~CCurlClient(void);
public:
	static bool Initialize();
	static void UnInitialize();
};
