// ChangeWallpaper.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


//glog
#define GLOG_NO_ABBREVIATED_SEVERITIES		// ERROR clash
#include "glog/logging.h"

// HTTP
#include <WinSock2.h>		// 放在windows.h 前
#include <WS2tcpip.h>
#include <urlmon.h>
#include <WinInet.h>

#include <windows.h>
#include <shlobj.h>

// iostream
#include <iostream>
using namespace std;

char* GetIpFromHost(char*);
int _tmain(int argc, _TCHAR* argv[])
{

	//char szPath[MAX_PATH] = { 0 };
	//SHGetSpecialFolderPathA(NULL, szPath, CSIDL_COMMON_APPDATA, FALSE);

	google::InitGoogleLogging("Wallpaper");
	google::LogToStderr();
	google::SetLogDestination(google::GLOG_INFO, "D:\\Project\\ChangeWallpaper\\LOG\\");
	
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
	{
		LOG(ERROR) << "WSAStartup failed" << endl;
		system("pause");
		return 1;
	}

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		LOG(ERROR) << "Error at socket(): " << WSAGetLastError() << endl;
		system("pause");
		return 1;
	}

	sockaddr_in addr;
	char* ipAddr = GetIpFromHost("bing.com");
	//LOG(INFO) << "ipAddr" << ipAddr << endl;
	int port = 80;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ipAddr);
	addr.sin_port = htons(port);
	
	int c = connect(s, (LPSOCKADDR)&addr, sizeof(addr));
	if (c == SOCKET_ERROR)
	{
		closesocket(s);
		LOG(ERROR) << "Unable to connect to server:" << WSAGetLastError() << endl;
		WSACleanup();
		system("pause");
		return 1;
	}
	LOG(INFO) << "Connect result: " << c << endl;
	
	
	char* header = "GET /HPImageArchive.aspx?format=xml&idx=0&n=1 HTTP/1.1\r\nHost: www.bing.com\r\nConnection: keep-alive\r\n\r\n";
	int snd = send(s, header, strlen(header), 0);
	if (snd == SOCKET_ERROR)
	{

		LOG(ERROR) << "Send failed:" << WSAGetLastError() << endl;
		closesocket(s);
		WSACleanup();
		return 1;
	}
	LOG(INFO) << "Send result: " << snd << endl;
	Sleep(100);
	int shtdwn = shutdown(s, SD_SEND);
	if (shtdwn == SOCKET_ERROR)
	{
		LOG(ERROR) << "Shutdown failed:" << WSAGetLastError() << endl;
		closesocket(s);
		WSACleanup();
		return 1;
	}
	LOG(INFO) << "shutdown result: " << shtdwn << endl;
	char recvBuf[BUFSIZ + 1];
	string strHtml;
	int r;

	do 
	{
	
		r = recv(s, recvBuf, BUFSIZ, 0);
		LOG(INFO) << "s:" << s << endl;
		if (r > 0)
		{
			recvBuf[r] = '\0';
			strHtml.append(recvBuf);
			cout << "strHtml append" << endl;
			LOG(INFO) << "strHTML append,r=" << r << endl;
		}
		else if (r == 0)
		{
			LOG(INFO) << "Connection closed" << endl;
		}
		else{
			LOG(INFO) << "recv failed: " << WSAGetLastError() << endl;
		}
		
	} while (r>0);

	closesocket(s);
	WSACleanup();

	string strUrl;
	int start = strHtml.find("<url>");
	int end = strHtml.find("</url>");

	if (start < end)
	{
		strUrl = "http://www.bing.com" + strHtml.substr(start + 5, end - start - 5);
		cout << strUrl << endl;
	}
	else{
		cout << "解析网页失败！" << endl;
		system("pause");
		return 1;
	}

	HRESULT hr;

	char file[MAX_PATH];

	cout << "正在下载..." << endl;
	hr = URLDownloadToCacheFileA(NULL, strUrl.c_str(), file, MAX_PATH, 0, NULL);
	if (SUCCEEDED(hr))
	{
		cout << "下载成功！" << endl;
		//cout << file << endl;
	}
	else
	{
		cout << "下载失败！" << endl;
		system("pause");
		return 1;
	}

	int fileLen = MultiByteToWideChar(CP_ACP, 0, file, -1, NULL, 0);
	wchar_t* wFile = new wchar_t[fileLen + 1];
	MultiByteToWideChar(CP_ACP, 0, file, -1, wFile, fileLen);
	wcout << wFile << endl;


	IActiveDesktop* pActiveDesktop;
	CoInitialize(NULL);
	hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_ALL, IID_IActiveDesktop, (LPVOID*)&pActiveDesktop);
	if (SUCCEEDED(hr))
	{
		hr = pActiveDesktop->SetWallpaper(wFile, 0);
		if (SUCCEEDED(hr)){
			hr = pActiveDesktop->ApplyChanges(AD_APPLY_ALL);
			if (SUCCEEDED(hr))
				cout << "设置成功！" << endl;
			else
				cout << "ApplyChanges Error: " << hr << endl;
		}
		else{
			cout << "SetWallpaper Error: " << hr << endl;
		}
		
	}
	else{
		cout << "CoCreateInstance Error: " << hr << endl;
	}
	pActiveDesktop->Release();
	CoUninitialize();

	system("pause");

	return 0;
}

char* GetIpFromHost(char* hostname)
{
	char* IP;
	struct addrinfo* result = NULL;
	struct sockaddr_in *sockaddr_ipv4;
	DWORD dwRetval = getaddrinfo(hostname, NULL, NULL, &result);
	if (dwRetval != 0)
	{
		LOG(ERROR) << "Get addrinfo failed error: %d" << dwRetval << endl;
		WSACleanup();

	}

	sockaddr_ipv4 = (sockaddr_in*)result->ai_addr;
	IP = inet_ntoa(sockaddr_ipv4->sin_addr);
	freeaddrinfo(result);
	return IP;
}