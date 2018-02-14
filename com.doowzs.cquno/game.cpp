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
		<< L"没做好！" << "\r\n";
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
		&& (desk->turn>0 && (desk->lastCard[0].substr(0, 1) == L"X"
			|| desk->lastCard[0].substr(0, 1) == L"Y"))			//第一回合vector为空会导致内存错误
		&& (msg.find(L"绿") == 0 || msg.find(L"黄") == 0 || msg.find(L"红") == 0 || msg.find(L"蓝") == 0)) {
		desk->changeColor(playNum, msg.substr(0, 1));
	}
	else if (desk->state >= STATE_READYTOGO
		&& (msg.find(L"绿") == 0 || msg.find(L"黄") == 0 || msg.find(L"红") == 0 || msg.find(L"蓝") == 0
		|| msg.find(L"变") == 0 || msg.find(L"+") == 0 || msg.find(L"＋") == 0)) {
		desk->play(playNum, L"出" + msg);						//自动检测并补全
	}
	else if ((desk->state >= STATE_READYTOGO) &&
		(msg.find(L"出") == 0 || msg.find(L"打") == 0)) {		//出牌阶段
		desk->play(playNum, msg);
	}
	else if ((desk->state >= STATE_READYTOGO) &&
		(msg.find(L"摸") == 0)) {
		desk->drawCards(playNum, 0);
	}
	else if (msg.find(L"退桌") == 0 || msg.find(L"下桌") == 0
		|| msg.find(L"不玩了") == 0) {	//结束游戏
		desk->exit(playNum);
	}
	else if (msg == L"玩家列表") {
		desk->listPlayers(1);
	}
	else if (msg.find(L"GO") == 0 || msg.find(L"启动") == 0) {
		desk->startGame();
	}
	else if ((msg.find(L"弃牌") == 0)
		&& desk->state >= STATE_READYTOGO) {
		desk->surrender(playNum);
	}
	else if (msg == L"记牌器") {
		desk->msg << L"记牌器没做(好)呢！估计有生之年可以做好！";
	}
	else if (msg == L"我的信息") {
		desk->getPlayerInfo(playNum);
	}
	else if (msg.find(L"加入观战") == 0 || msg.find(L"观战") == 0) {
		desk->joinWatching(playNum);
	}
	else if (msg.find(L"退出观战") == 0) {
		desk->exitWatching(playNum);
	}
	else if (msg.find(L"强制结束") == 0) {
		if (Admin::isAdmin(playNum)) {
			if (desk->state < STATE_READYTOGO) {
				return false;
			}
			else {
				desk->msg << L"管理员强制结束本桌游戏。";
				desk->msg << ENDL;
				casino.gameOver(deskNum);
			}
		}
		else {
			desk->msg << L"你根本不是管理员！";
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
	if (msg == L"我是管理") {
		result = Admin::IAmAdmin(playNum);
	}
	else if (msg == L"重置斗地主" || msg == L"初始化斗地主") {
		result = Admin::resetGame(playNum);
	}
	else if (msg.find(L"改变数据") == 0) {
		result = Admin::writeDataType();
		Admin::getPlayerInfo(Admin::readAdmin());
	}
	else if (regex_match(msg, allotReg)) {
		result = Admin::allotScoreTo(msg, playNum);
	}
	else if (regex_match(msg, allotReg2)) {
		result = Admin::allotScoreTo2(msg, playNum);
	}
	else if (msg.find(L"结束游戏") == 0 || msg.find(L"停止游戏") == 0) {//结束游戏
		result = Admin::gameOver(msg, playNum);
	}
	else if (msg == L"我的信息") {
		Admin::getPlayerInfo(playNum);
		return false;
	}
	else if (regex_match(msg, getInfoReg)) {
		//查询指定玩家积分
		int64_t playerNum;

		wsmatch mr;
		wstring::const_iterator src_it = msg.begin(); // 获取起始位置
		wstring::const_iterator src_end = msg.end(); // 获取结束位置
		regex_search(src_it, src_end, mr, numberReg);
		wstringstream ss;
		ss << mr[0].str();
		ss >> playerNum;
		ss.str(L"");

		Admin::getPlayerInfo(playNum);
		return false;
	}
	else if (msg == L"备份数据") {
		result = Admin::backupData(playNum);
	}
	else {
		return false;
	}

	msg = result ? L"操作成功，尊贵的管理员" : L"非常抱歉，操作失败";
	Util::sendPrivateMsg(playNum, Util::wstring2string(msg).data());
	return true;
}