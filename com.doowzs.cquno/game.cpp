#pragma once 

#include "stdafx.h"
#include "string"

#ifdef _DEBUG 
#else
#include "cqp.h"
#endif 

#include <time.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <tchar.h>  
#include <regex> 

#include "Util.h"
#include "Admin.h"
#include "Desks.h"
#include "Player.h"
#include "Watcher.h"

using namespace std;

int Util::AC = 0;

void Desk::commandList()
{
	this->msg
		<< L"û���ã�" << "\r\n";
	this->msg << ENDL;
}

bool Desks::game(bool subType, int64_t deskNum, int64_t playNum, const char* msgArray) {

	string tmp = msgArray;

	wstring msg = Util::string2wstring(tmp);
	Util::trim(msg);
	Util::toUpper(msg);

	Desk *desk = casino.getOrCreateDesk(deskNum);

	desk->subType = subType;

	if (playNum == 80000000) {
		return false;
	}
	else if ((desk->state == STATE_WAIT) && msg.find(L"UNO") == 0) {
		desk->join(playNum);
	}
	else if (desk->state >= STATE_READYTOGO
		&& (desk->turn > 0 && desk->lastWDFPlayer > 0
		&& (msg.find(L"����") == 0))) {
		desk->WDFHandle(playNum, desk->lastWDFPlayer, desk->isLegalWDF);
	}
	else if (desk->state >= STATE_READYTOGO
		&& (desk->turn > 0 && (desk->lastCard[0].substr(0, 1) == L"X"
			|| desk->lastCard[0].substr(0, 1) == L"Y"))			//��һ�غ�vectorΪ�ջᵼ���ڴ����
		&& (msg.find(L"��") == 0 || msg.find(L"��") == 0 || msg.find(L"��") == 0 || msg.find(L"��") == 0)) {
		desk->changeColor(playNum, msg.substr(0, 1));
	}
	else if (desk->state >= STATE_READYTOGO
		&& (msg.find(L"��") == 0 || msg.find(L"��") == 0 || msg.find(L"��") == 0 || msg.find(L"��") == 0
		|| msg.find(L"��") == 0 || msg.find(L"+") == 0 || msg.find(L"��") == 0)) {
		desk->play(playNum, L"��" + msg);						//�Զ���Ⲣ��ȫ
	}
	else if ((desk->state >= STATE_READYTOGO)
		&& (msg.find(L"��") == 0 || msg.find(L"��") == 0)) {		//�������ƽ׶�
		desk->play(playNum, msg);
	}
	else if ((desk->state >= STATE_READYTOGO) &&
		(msg.find(L"��") == 0)) {
		if (desk->damageCount == 0) {
			if (Admin::isFreeDrawEnabled()) {
				desk->drawCards(playNum, 1);
			}
			else {
				desk->msg << L"�������Լ�����ƿɳ��������ƣ�";
			}
		}
		else {
			desk->drawCards(playNum, desk->damageCount);
		}
	}
	else if (msg.find(L"����") == 0 || msg.find(L"����") == 0
		|| msg.find(L"������") == 0) {	//������Ϸ
		desk->exit(playNum);
	}
	else if (msg == L"����б�") {
		desk->listPlayers(1);
	}
	else if (msg.find(L"GO") == 0 || msg.find(L"����") == 0) {
		desk->startGame();
	}
	else if ((msg.find(L"����") == 0)
		&& desk->state >= STATE_READYTOGO) {
		desk->surrender(playNum);
	}
	else if (msg == L"������") {
		desk->msg << L"������û��(��)�أ���������֮��������ã�";
	}
	else if (msg == L"�ҵ���Ϣ") {
		desk->getPlayerInfo(playNum);
	}
	else if (msg.find(L"�����ս") == 0 || msg.find(L"��ս") == 0) {
		desk->joinWatching(playNum);
	}
	else if (msg.find(L"�˳���ս") == 0) {
		desk->exitWatching(playNum);
	}
	else if (msg.find(L"ǿ�ƽ���") == 0) {
		if (Admin::isAdmin(playNum)) {
			if (desk->state < STATE_READYTOGO) {
				return false;
			}
			else {
				desk->msg << L"����Աǿ�ƽ���������Ϸ��";
				desk->msg << ENDL;
				casino.gameOver(deskNum);
			}
		}
		else {
			desk->msg << L"��������ǹ���Ա��";
			desk->msg << ENDL;
		}
	}
	else {
		return false;
	}

	desk->sendMsg(subType);
	desk->sendPlayerMsg();
	desk->sendWatcherMsg();
	return true;
}

bool Desks::game(int64_t playNum, const char * msgArray)
{
	string tmp = msgArray;

	wstring msg = Util::string2wstring(tmp);
	Util::trim(msg);
	Util::toUpper(msg);


	bool result;
	if (msg == L"���ǹ���") {
		result = Admin::IAmAdmin(playNum);
	}
	else if (msg == L"���ö�����" || msg == L"��ʼ��������") {
		result = Admin::resetGame(playNum);
	}
	else if (msg.find(L"�ı�����") == 0) {
		result = Admin::writeDataType();
		Admin::getPlayerInfo(Admin::readAdmin());
	}
	else if (regex_match(msg, allotReg)) {
		result = Admin::allotScoreTo(msg, playNum);
	}
	else if (regex_match(msg, allotReg2)) {
		result = Admin::allotScoreTo2(msg, playNum);
	}
	else if (msg.find(L"������Ϸ") == 0 || msg.find(L"ֹͣ��Ϸ") == 0) {//������Ϸ
		result = Admin::gameOver(msg, playNum);
	}
	else if (msg == L"�ҵ���Ϣ") {
		Admin::getPlayerInfo(playNum);
		return false;
	}
	else if (regex_match(msg, getInfoReg)) {
		//��ѯָ����һ���
		int64_t playerNum;

		wsmatch mr;
		wstring::const_iterator src_it = msg.begin(); // ��ȡ��ʼλ��
		wstring::const_iterator src_end = msg.end(); // ��ȡ����λ��
		regex_search(src_it, src_end, mr, numberReg);
		wstringstream ss;
		ss << mr[0].str();
		ss >> playerNum;
		ss.str(L"");

		Admin::getPlayerInfo(playNum);
		return false;
	}
	else if (msg == L"��������") {
		result = Admin::backupData(playNum);
	}
	else {
		return false;
	}

	msg = result ? L"�����ɹ������Ĺ���Ա" : L"�ǳ���Ǹ������ʧ��";
	Util::sendPrivateMsg(playNum, Util::wstring2string(msg).data());
	return true;
}