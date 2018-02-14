#pragma once

#include <time.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <tchar.h>  
#include <regex> 

#include "classes.h"
using namespace std;

void Util::testMsg(bool subType, int64_t desknum, int64_t playNum, const char * str) {
	int index = casino.desks[0]->currentPlayIndex;
	casino.game(subType, desknum, playNum + index, str);
}


void Util::sendGroupMsg(int64_t groupid, const char *msg) {
#ifdef _DEBUG  
	string aa = msg;
	cout << "Ⱥ����" << aa << endl;
#else
	CQ_sendGroupMsg(Util::AC, groupid, msg);
#endif
}


void Util::sendDiscussMsg(int64_t groupid, const char *msg) {
#ifdef _DEBUG  
	string aa = msg;
	cout << "Ⱥ����" << aa << endl;
#else
	CQ_sendDiscussMsg(Util::AC, groupid, msg);
#endif
}

void Util::sendPrivateMsg(int64_t number, const char* msg) {
#ifdef _DEBUG  
	string aa = msg;
	cout << "˽��" << number << "��" << aa << endl;
#else
	CQ_sendPrivateMsg(Util::AC, number, msg);
#endif
}


//��stringת����wstring  
wstring Util::string2wstring(string str)
{
	wstring result;
	//��ȡ��������С��������ռ䣬��������С���ַ�����  
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
	TCHAR* buffer = new TCHAR[len + 1];
	//���ֽڱ���ת���ɿ��ֽڱ���  
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
	buffer[len] = '\0';             //����ַ�����β  
									//ɾ��������������ֵ  
	result.append(buffer);
	delete[] buffer;
	return result;
}

void Util::mkdir()
{
	CreateDirectory(CONFIG_DIR.c_str(), NULL);
}

//��wstringת����string  
string Util::wstring2string(wstring wstr)
{
	string result;
	//��ȡ��������С��������ռ䣬��������С�°��ֽڼ����  
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
	char* buffer = new char[len + 1];
	//���ֽڱ���ת���ɶ��ֽڱ���  
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
	buffer[len] = '\0';
	//ɾ��������������ֵ  
	result.append(buffer);
	delete[] buffer;
	return result;
}

int Util::findAndRemove(vector<wstring> &dest, wstring str) {
	for (unsigned i = 0; i < dest.size(); i++) {
		if (dest.at(i) == str) {
			vector<wstring>::iterator it = dest.begin() + i;
			dest.erase(it);
			return i;
		}
	}
	return -1;
}

int Util::find(vector<wstring> &dest, wstring str) {

	for (unsigned i = 0; i < dest.size(); i++) {
		if (dest.at(i) == str) {
			return i;
		}
	}
	return -1;
}

int Util::findFlag(wstring str)
{
	for (int i = 0; i < 108; i++) {
		if (flag[i] == str) {
			return i;
		}
	}
	return -1;

}

int Util::desc(int a, int b)
{
	return a > b;
}

int Util::asc(int a, int b)
{
	return a < b;
}

bool Util::compareCard(const wstring &carda, const wstring &cardb)
{
	return findFlag(carda) < findFlag(cardb);
}

void Util::trim(wstring &s)
{

	int index = 0;
	if (!s.empty())
	{
		while ((index = s.find(' ', index)) != wstring::npos)
		{
			s.erase(index, 1);
		}
	}

}

void Util::toUpper(wstring &str) {
	transform(str.begin(), str.end(), str.begin(), ::toupper);
}

void Util::setAC(int32_t ac)
{
	Util::AC = ac;
}

void Util::strcat_tm(char* result, rsize_t size, struct tm now_time) {
	char tmp[20] = { 0 };

	tmp[0] = '0' + (now_time.tm_year % 100) / 10;
	tmp[1] = '0' + (now_time.tm_year % 100) % 10;
	tmp[2] = '0' + (now_time.tm_mon + 1) / 10;
	tmp[3] = '0' + (now_time.tm_mon + 1) % 10;
	tmp[4] = '0' + (now_time.tm_mday) / 10;
	tmp[5] = '0' + (now_time.tm_mday) % 10;
	tmp[6] = '0' + (now_time.tm_hour) / 10;
	tmp[7] = '0' + (now_time.tm_hour) % 10;
	tmp[8] = '0' + (now_time.tm_min) / 10;
	tmp[9] = '0' + (now_time.tm_min) % 10;

	//wstring msg = L"�Һ�΢����ǰʱ�䣺" + Util::string2wstring(tmp);
	//Util::sendPrivateMsg(Admin::readAdmin(), Util::wstring2string(msg).data());

	strcat_s(result, size, tmp);
	return;
}

