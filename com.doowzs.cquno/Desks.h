#pragma once

#include <time.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <tchar.h>  
#include <regex> 
#include <thread>

#include "classes.h"
using namespace std;

Desk::Desk() {
	srand((unsigned)time(NULL));

	//先将第一张牌放入，否则会造成除0错误
	this->cards.push_back(cardDest[0]);
	for (int i = 1; i < 108; i++) {
		this->cards.insert(this->cards.begin()+(rand()%this->cards.size()),cardDest[i]);
	}

	this->state = STATE_WAIT;
	this->lastPlayIndex = -1;				//当前谁出得牌
	this->currentPlayIndex = -1;			//该谁出牌

	vector<wstring> lastCard;				//上位玩家的牌
	this->lastWDFPlayer = 0;
	this->damageCount = 0;

	this->whoIsWinner = -1;
	this->turn = 0;
	this->isClockwise = 1;
	this->lastTime = 0;						//最后一次发牌时间，记录用
}

void Desk::at(int64_t playNum)
{
	this->msg << L"[CQ:at,qq=" << playNum << L"]";
}

int Desk::getPlayer(int64_t number) {
	for (unsigned i = 0; i < players.size(); i++) {
		if (players[i]->number == number) {
			return i;
		}
	}
	return -1;
}

int Desk::getWatcher(int64_t number) {
	for (unsigned i = 0; i < watchers.size(); i++) {
		if (watchers[i]->number == number) {
			return i;
		}
	}
	return -1;
}

void Desk::listPlayers(int type)
{
	bool hasWin = ((type >> 1) & 1) == 1;

	if (this->players.size() < 2) {
		this->msg << L"游戏尚未开始，玩家列表：";
		this->msg << ENDL;
	}
	else {
		this->msg << L"出牌次数：" << this->turn;
		this->msg << ENDL;
	}

	if (hasWin) {
		vector<int> scores;
		int winScore = 0;
		for (unsigned i = 0; i < this->players.size(); i++) {
			scores.push_back(0);
			if (i != this->whoIsWinner) {
				for (unsigned j = 0; j < this->players[i]->card.size(); j++) {
					if (Util::findFlag(this->players[i]->card[j]) >= 100) {		//黑牌
						scores[i] -= 50;
						continue;
					}
					if (Util::findFlag(this->players[i]->card[j]) % 25 >= 19) {	//功能牌
						scores[i] -= 20;
						continue;
					}
					scores[i] -= (Util::findFlag(this->players[i]->card[j]) % 25 + 1) / 2;	//数字牌
				}
			}
			winScore += -scores[i];			//赢家获得正分
		}
		scores[whoIsWinner] = winScore;

		for (unsigned i = 0; i < this->players.size(); i++) {
			this->msg << i + 1 << L"：";
			this->at(this->players[i]->number);
			this->msg << L"[" << (i==this->whoIsWinner? L"胜利" : L"失败")
				<< scores[i] << L"分]" << ENDL;
			Admin::addScore(this->players[i]->number, scores[i]);
		}
		return;
	}
	else {
		for (unsigned i = 0; i < this->players.size(); i++) {
			this->msg << i + 1 << L"：";
			this->at(this->players[i]->number);
			this->msg << L"：" << this->players[i]->card.size();
			this->msg << ENDL;
		}
		return;
	}
}

void Desk::sendMsg(bool subType)
{
	wstring tmp = this->msg.str();
	if (tmp.empty()) {
		return;
	}
	int length = tmp.length();
	if (tmp[length - 2] == '\r' && tmp[length - 1] == '\n') {
		tmp = tmp.substr(0, length - 2);
	}
	if (subType) {
		Util::sendGroupMsg(this->number, Util::wstring2string(tmp).data());
	}
	else {
		Util::sendDiscussMsg(this->number, Util::wstring2string(tmp).data());
	}

	this->msg.str(L"");
}

void Desk::sendPlayerMsg()
{
	for (unsigned i = 0; i < this->players.size(); i++) {
		players[i]->sendMsg();
	}
}

void Desk::sendWatcherMsg()
{
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watchers[i]->sendMsg();
	}
}

void Desk::shuffle() {
	srand((unsigned)time(NULL));
	for (unsigned i = 0; i < this->cards.size(); i++) {
		swap(this->cards[i], this->cards[rand() % this->cards.size()]);
	}
}

void Desk::play(int64_t playNum, wstring msg)
{
	int playIndex = this->getPlayer(playNum);
	int length = msg.length();

	//由于有抢牌的可能性，这里不判断是否是当前玩家，而是留到下一个函数判断
	if (playIndex == -1
		|| (!(this->state == STATE_GAMING && this->turn > 0)
			&& !(this->state == STATE_READYTOGO && this->turn == 0))
		|| length <= 2) {	//消息长度必须大于2（出+牌）
		return;
	}

	vector<wstring> msglist;

	//获取牌面信息
	for (int i = 1; i < length - 1; i++) {
		wstring color = msg.substr(i, 1);
		i++;
		wstring face = msg.substr(i, 1);
		if (face == L"跳") {
			face = L"跳过";
			i++;
		}
		else if (face == L"+" || face == L"＋") {
			face = L"+2";
			i++;
		}
		else if (face == L"反") {
			face = L"反转";
			i++;
		}
		
		msglist.push_back(color + face);
	}

	this->play(msglist, playIndex);
}

void Desk::play(vector<wstring> list, int playIndex)
{
	//完全一致，抢牌（第一次出牌肯定不能实现）
	if (this->turn > 0 && list[0] == this->lastCard[0]) {
		this->currentPlayIndex = playIndex;
	}

	//抢牌不成功或非当前玩家，终止执行
	if (playIndex != this->currentPlayIndex) {
		return;
	}

	Player *player = this->players[playIndex];
	vector<wstring> mycardTmp(player->card);

	int cardCount = list.size();

	for (int i = 0; i < cardCount; i++) {
		if (Util::findAndRemove(mycardTmp, list[i]) == -1) {
			this->at(this->players[currentPlayIndex]->number);
			this->msg << ENDL;
			this->msg << L"真丢人，你就没有你要出的牌，会不会玩？";
			return;
		}
	}

	//判断出牌是否合法
	bool isLegalPlay = false;

	this->lastWDFPlayer = 0;
	this->isLegalWDF = !this->ableToPlay_NoWDF(this->players[playIndex]->number);

	if ((this->turn == 0													//首次出牌
		|| list[0].substr(0, 1) == this->lastCard[0].substr(0, 1)		//相同颜色
		|| list[0].substr(1, 1) == this->lastCard[0].substr(1, 1)		//相同数字
		|| list[0].substr(0, 2) == L"变色"								//出黑牌
		|| list[0].substr(0, 2) == L"+4"
		|| (this->lastCard[0].substr(0, 1) == L"Z"
			&& list[0].substr(0, 1) == this->lastCard[0].substr(1, 1)))	//变色后出牌
		&& (cardCount == 1 ||
		(cardCount == 2 && list[0] == list[1]))) {
		isLegalPlay = true;
	}

	if (this->damageCount > 0
		&& (list[0].substr(0, 1) != L"+" && list[0].substr(1, 1) != L"跳"
			&& list[0].substr(1, 1) != L"+" && list[0].substr(1, 1) != L"反")) {
		isLegalPlay = false;
	}
	
	if (isLegalPlay) {
		if (this->turn == 0) {
			this->state = STATE_GAMING;
		}

		//防止提示后出牌出现bug
		this->warningSent = false;

		player->card = mycardTmp;
		this->lastCard = list;
		this->lastPlayIndex = this->currentPlayIndex;
		this->turn++;

		//出的牌随机塞入牌库
		srand((unsigned)time(NULL));
		for (unsigned i = 0; i < this->lastCard.size(); i++) {
			this->cards.insert(this->cards.begin()+rand()%this->cards.size(),this->lastCard[i]);
		}
		this->shuffle();

		if (mycardTmp.size() == 0) {//赢了。
			this->whoIsWinner = playIndex;
			this->sendWatchingMsg_Over();

			this->msg << L"---------------";
			this->msg << ENDL;
			this->msg << L"分数结算：";
			this->msg << ENDL;
			this->listPlayers(3);

			casino.gameOver(this->number);
			return;
		}

		player->listCards();

		if (player->card.size() == 1) {
			this->msg << L"UNO！";
			this->msg << ENDL;
			this->at(player->number);
			this->msg << L"仅剩下" << player->card.size() << L"张牌！";
			this->msg << ENDL;
			this->msg << L"---------------";
			this->msg << ENDL;
		}

		this->at(this->players[currentPlayIndex]->number);
		this->msg << L"打出";
		for (unsigned m = 0; m < this->lastCard.size(); m++) {
			this->msg << L"[" << this->lastCard.at(m) << L"]";
		}
		this->msg << ENDL;

		//观战播报，必须先转发战况再设置下一位玩家，否则玩家信息错误
		this->sendWatchingMsg_Play();

		this->msg << L"---------------";
		this->msg << ENDL;
		this->msg << L"剩余手牌数：";
		this->msg << ENDL;
		for (unsigned i = 0; i < this->players.size(); i++) {
			this->msg << i + 1 << L"：";
			this->at(this->players[i]->number);
			this->msg << L"：" << static_cast<int>(this->players[i]->card.size());
			this->msg << ENDL;
		}

		if (list[0] == L"变色") {
			this->msg << L"---------------" << ENDL;
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"请选择颜色。";
			this->lastCard[0] = L"X" + this->lastCard[0].substr(1,1);
			return;
		}

		if (list[0] == L"+4") {
			this->lastWDFPlayer = this->players[this->currentPlayIndex]->number;

			this->msg << L"---------------" << ENDL;
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"请选择颜色。";
			this->lastCard[0] = L"Y" + this->lastCard[0].substr(1, 1);
			return;
		}
		
		if (list[0].substr(1, 2) == L"+2") {
			this->damageCount += 2;
			this->msg << L"累计：" << this->damageCount << ENDL;
			this->setNextPlayerIndex(false, true);
			return;
		}
		
		if (list[0].substr(1, 2) == L"跳过") {
			if (this->damageCount == 0) {
				this->setNextPlayerIndex(true, false);
			}
			else {
				this->setNextPlayerIndex(false, true);
			}
			return;
		}

		if (list[0].substr(1, 2) == L"反转") {
			if (this->damageCount == 0) {
				if (this->players.size() == 2) {
					this->setNextPlayerIndex(true, false);
					return;
				}
				else {
					this->isClockwise = 0 - this->isClockwise;
				}
			}
			else {
				this->isClockwise = 0 - this->isClockwise;
				this->setNextPlayerIndex(false, true);
			}
		}

		//打出0：所有人按顺序交换手牌，有问题，待重写
		if (Admin::isSevenOEnabled() && list[0].substr(1, 1) == L"0") {
		}
		//打出7：与指定玩家交换手牌
		if (Admin::isSevenOEnabled() && list[0].substr(1, 1) == L"7") {
		}

		this->setNextPlayerIndex(false, false);
	}
	else {
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << ENDL;
		this->msg << L"傻逼网友，打的什么几把玩意！学会出牌再打！";
		this->msg << ENDL;
	}
}

void Desk::changeColor(int64_t playNum, wstring color) {
	//非法操作，退出
	if (playNum != this->players[this->currentPlayIndex]->number) {
		return;
	}

	this->at(this->players[currentPlayIndex]->number);
	this->msg << L"选择" << color << L"色。" << ENDL;
	this->lastCard[0] = this->lastCard[0].substr(0, 1) + color;

	//先转发再设置下一个玩家
	this->sendWatchingMsg_ChangeColor();

	if (this->lastCard[0].substr(0, 1) == L"X") {
		this->lastCard[0] = L"Z" + color;
		this->setNextPlayerIndex(false, false);
		return;
	}
	if (this->lastCard[0].substr(0, 1) == L"Y") {
		this->damageCount += 4;
		this->msg << L"累计" << this->damageCount << ENDL;
		this->setNextPlayerIndex(false, true);
		return;
	}
}

void Desk::surrender(int64_t playNum)
{
	int index = this->getPlayer(playNum);
	if (index == -1 || this->players[index]->isSurrender) {
		return;
	}
	if (this->state != STATE_GAMING) {
		this->at(playNum);
		this->msg << ENDL;
		this->msg << L"当前游戏状态无法弃牌！游戏开始后方可弃牌。";
		return;
	}

	//防止提示后弃牌出现bug
	this->warningSent = false;

	Player *player = this->players[index];

	player->isSurrender = true;

	int alivePlayerCount = 0;
	for (size_t i = 0; i < this->players.size(); i++) {
		if (!players[i]->isSurrender) {
			alivePlayerCount++;
			this->whoIsWinner = i;
		}
	}
	if (alivePlayerCount > 1) {
		this->whoIsWinner = -1;
	}

	if (this->whoIsWinner >= 0) {
		this->sendWatchingMsg_Over();

		this->msg << L"游戏结束，";
		this->at(this->players[this->whoIsWinner]->number);
		this->msg << L"赢了。" << ENDL;

		this->msg << L"---------------";
		this->msg << ENDL;
		this->msg << L"分数结算：";
		this->msg << ENDL;
		this->listPlayers(3);

		casino.gameOver(this->number);
		return;
	}


	if (this->currentPlayIndex == index) {
		this->at(playNum);
		this->msg << L"弃牌，";

		this->setNextPlayerIndex(false, false);
	}
	else {
		this->at(playNum);
		this->msg << L"弃牌。";
		this->msg << ENDL;
	}

	//观战播报
	this->sendWatchingMsg_Surrender(playNum);
}

void Desk::getPlayerInfo(int64_t playNum)
{
	this->msg << Admin::readDataType() << L" " << Admin::readVersion();// << L" CST";
	this->msg << ENDL;
	this->at(playNum);
	this->msg << L"：";
	//this->msg << ENDL;
	this->msg << L"积分";
	this->readSendScore(playNum);
	this->msg << ENDL;
}

int64_t Desk::readScore(int64_t playNum) {
	int64_t hasScore = Admin::readScore(playNum);
	hasScore -= 500000000;
	return hasScore;
}

void Desk::readSendScore(int64_t playNum) {
	int64_t hasScore = Admin::readScore(playNum);
	hasScore -= 500000000;
	if (hasScore >= 0) {
		this->msg << hasScore;
	}
	else {
		this->msg << L"-" << -hasScore;
	}
}

void Desks::gameOver(int64_t number)
{
	int index = casino.getDesk(number);
	if (index == -1) {
		return;
	}

	//销毁挂机检测程序
	casino.desks[index]->state = STATE_OVER;
	casino.desks[index]->counter->join();

	vector<Desk*>::iterator it = casino.desks.begin() + index;
	casino.desks.erase(it);
}

void Desk::setNextPlayerIndex(bool skipNext, bool getCards)
{
	//记录时间
	time_t rawtime;
	this->lastTime = time(&rawtime);

	this->currentPlayIndex = (this->currentPlayIndex + 1*this->isClockwise 
		+ this->players.size()) % this->players.size();

	this->msg << L"---------------" << ENDL;
	this->msg << L"[" << this->lastCard[0] << L"]";

	//如果下一个该出牌的玩家被跳过，则重新set下一位玩家
	if (skipNext) {
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"跳过。" << ENDL;
		this->setNextPlayerIndex(false, false);
		return;
	}
	//如果下一个玩家弃牌，直接跳过
	if (this->players[this->currentPlayIndex]->isSurrender) {
		this->setNextPlayerIndex(false, false);
		return;
	}
	//如果叠加摸牌
	if (this->damageCount > 0) {
		if (this->lastWDFPlayer > 0) {
			//上一个人+4
			this->msg << L"现在轮到";
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"，你可以质疑或者摸牌！" << ENDL;
			return;
		}
		else {
			if (!this->ableToDamage(this->players[this->currentPlayIndex]->number)) {
				//普通叠加，无牌
				this->msg << L"现在轮到";
				this->at(this->players[this->currentPlayIndex]->number);
				this->msg << L"，没有可以出的牌，系统自动摸牌！" << ENDL;
				this->drawCards(this->players[this->currentPlayIndex]->number, this->damageCount);
				return;
			}
			else {
				//普通叠加，有牌
				this->msg << L"现在轮到";
				this->at(this->players[this->currentPlayIndex]->number);
				this->msg << L"，你可以叠加或转移或摸牌！" << ENDL;
				return;
			}
		}
	}
	//如果下一个该出牌的玩家将要出牌，且没有能出的牌，则自动摸牌。
	if (this->damageCount == 0 && !this->ableToPlay(this->players[this->currentPlayIndex]->number)) {
		this->msg << L"现在轮到";
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"，没有可以出的牌，系统自动摸牌！" << ENDL;
		this->drawCards(this->players[this->currentPlayIndex]->number, 0);
		return;
	}

	this->msg << L"现在轮到";
	this->at(this->players[this->currentPlayIndex]->number);
	this->msg << L"。";
	this->msg << ENDL;
}

bool Desk::ableToPlay(int64_t playNum) {
	int index = this->getPlayer(playNum);

	if (this->turn == 0) return true;

	for (unsigned i = 0; i < this->players[index]->card.size(); i++) {
		if (this->players[index]->card[i].substr(0, 1) == this->lastCard[0].substr(0, 1)			//相同颜色
			|| this->players[index]->card[i].substr(1, 1) == this->lastCard[0].substr(1, 1)			//相同数字
			|| this->players[index]->card[i].substr(0, 2) == L"变色"									//出黑牌
			|| this->players[index]->card[i].substr(0, 2) == L"+4"
			|| (this->lastCard[0].substr(0, 1) == L"Z"
				&& this->players[index]->card[i].substr(0, 1) == this->lastCard[0].substr(1, 1))) {	//变色后出牌
			return true;
		}
	}

	return false;
}

bool Desk::ableToPlay_NoWDF(int64_t playNum) {
	int index = this->getPlayer(playNum);

	if (this->turn == 0) return true;

	for (unsigned i = 0; i < this->players[index]->card.size(); i++) {
		if (this->players[index]->card[i].substr(0, 1) == this->lastCard[0].substr(0, 1)			//相同颜色
			|| this->players[index]->card[i].substr(1, 1) == this->lastCard[0].substr(1, 1)			//相同数字
			|| this->players[index]->card[i].substr(0, 2) == L"变色"									//出黑牌
			|| (this->lastCard[0].substr(0, 1) == L"Z"
				&& this->players[index]->card[i].substr(0, 1) == this->lastCard[0].substr(1, 1))) {	//变色后出牌
			return true;
		}
	}

	return false;
}

bool Desk::ableToDamage(int64_t playNum) {
	int index = this->getPlayer(playNum);

	if (this->turn == 0) return true;

	for (unsigned i = 0; i < this->players[index]->card.size(); i++) {
		if ((this->players[index]->card[i].substr(0, 1) == L"+")
			|| (this->lastCard[0].substr(0, 1) == L"Y"
				|| this->players[index]->card[i].substr(0, 1) == this->lastCard[0].substr(0, 1))
				&&	 (this->players[index]->card[i].substr(1, 1) == L"+"
					|| this->players[index]->card[i].substr(1, 1) == L"跳"
					|| this->players[index]->card[i].substr(1, 1) == L"反")) {
			return true;
		}
	}

	return false;
}

void Desk::drawCards(int64_t playNum, int amount) {
	int index = this->getPlayer(playNum);

	//非自己的回合不能摸牌
	if (index != this->currentPlayIndex) return;
	
	int count = 0; 

	if (amount == 0) {
		wstring lastGot;	//记录最后一张牌，一定是可以打出的
		if (Admin::isCrazyDrawEnabled()) {
			//摸到爽
			while (!this->ableToPlay(playNum)) {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				if (this->cards.empty()) {
					this->msg << L"牌库空了，游戏结束。" << ENDL;
					casino.gameOver(this->number);
				}

				count++;
				lastGot = this->cards.front();
				this->players[index]->card.push_back(lastGot);
				this->cards.erase(this->cards.begin());

				sort(this->players[index]->card.begin(), this->players[index]->card.end(), Util::compareCard);

				for (unsigned m = 0; m < this->players[index]->card.size(); m++) {
					this->players[index]->msg << L"[" << this->players[index]->card.at(m) << L"]";
				}
				this->players[index]->msg << ENDL;
				this->sendPlayerMsg();
			}
			this->at(playNum);
			this->msg << L"共摸了" << count << L"张，手牌总数"
				<< this->players[index]->card.size() << L"张。" << ENDL;
			this->sendPlayerMsg();
		}
		else {
			//不摸到爽的情况下，只摸一张牌，如果不能出就跳过
			if (this->cards.empty()) {
				this->msg << L"牌库空了，游戏结束。" << ENDL;
				casino.gameOver(this->number);
			}

			count++;
			lastGot = this->cards.front();
			this->players[index]->card.push_back(lastGot);
			this->cards.erase(this->cards.begin());

			sort(this->players[index]->card.begin(), this->players[index]->card.end(), Util::compareCard);

			for (unsigned m = 0; m < this->players[index]->card.size(); m++) {
				this->players[index]->msg << L"[" << this->players[index]->card.at(m) << L"]";
			}
			this->players[index]->msg << ENDL;
			this->sendPlayerMsg();

			if (ableToPlay(playNum)) {
				this->at(playNum);
				this->msg << L"共摸了" << count << L"张，手牌总数"
					<< this->players[index]->card.size() << L"张。" << ENDL;
				this->sendPlayerMsg();
			}
			else {
				this->at(playNum);
				this->msg << L"共摸了" << count << L"张，手牌总数"
					<< this->players[index]->card.size() << L"张，无法出牌，跳过。" << ENDL;
				this->sendPlayerMsg();
				this->setNextPlayerIndex(false, false);
			}
		}
	}
	else {
		for (int i = 0; i < amount; i++) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));

			if (this->cards.empty()) {
				this->msg << L"牌库空了，游戏结束。" << ENDL;
				casino.gameOver(this->number);
			}

			count++;
			this->players[index]->card.push_back(this->cards.front());
			this->cards.erase(this->cards.begin());

			sort(this->players[index]->card.begin(), this->players[index]->card.end(), Util::compareCard);

			for (unsigned m = 0; m < this->players[index]->card.size(); m++) {
				this->players[index]->msg << L"[" << this->players[index]->card.at(m) << L"]";
			}
			this->players[index]->msg << ENDL;
			this->sendPlayerMsg();
		}
		this->at(playNum);
		this->msg << L"共摸了" << count << L"张，手牌总数"
			<< this->players[index]->card.size() << L"张。" << ENDL;
		this->sendPlayerMsg();

		//处理+4的信息
		if (this->lastCard[0].substr(0, 1) == L"Y") {
			this->lastCard[0] = L"Z" + this->lastCard[0].substr(1, 1);
		}

		//只有伤害摸牌才会自动到下一个人
		if (this->damageCount > 0) {
			this->damageCount = 0;
			this->setNextPlayerIndex(false, false);
		}
	}
}

void Desk::deal() {
	unsigned i, k, j;
	for (i = k = 0; i < this->players.size(); i++) {
		Player *player = players[i];
		//清空存储，避免bug
		players[i]->card.clear();

		//UNO初始牌为7张
		for (j = 0; j < 7; j++) {
			player->card.push_back(this->cards.front());
			this->cards.erase(this->cards.begin());
		}

		sort(player->card.begin(), player->card.end(), Util::compareCard);

		player->msg << L"当前额外规则" << ENDL
			<< L"自摸：" << (Admin::isFreeDrawEnabled() ? L"启用" : L"禁用") << ENDL
			<< L"摸到爽：" << (Admin::isCrazyDrawEnabled() ? L"启用" : L"禁用") << ENDL
			<< L"0-7换牌：" << (Admin::isSevenOEnabled() ? L"启用" : L"禁用") << ENDL
			<< L"伤害叠加：" << (Admin::isRegressiveEnabled() ? L"启用" : L"禁用") << ENDL
			<< L"同种牌抢牌：" << (Admin::isJumpInEnabled() ? L"启用" : L"禁用") << ENDL;
		this->sendPlayerMsg();

		for (unsigned m = 0; m < player->card.size(); m++) {
			player->msg << L"[" << player->card.at(m) << L"]";
		}

		player->msg << ENDL;
		this->sendPlayerMsg();
	}
}

int Desks::getDesk(int64_t deskNum) {

	for (unsigned i = 0; i < this->desks.size(); i++) {
		if (this->desks[i]->number == deskNum) {
			return i;
		}
	}

	return -1;
}

void Desk::join(int64_t playNum)
{

	int playIndex = this->getPlayer(playNum);

	if (playIndex != -1) {
		this->at(playNum);
		this->msg << ENDL;
		this->msg << L"你已经加入游戏！";
		this->msg << ENDL;
		return;
	}

	if (this->players.size() > 10) {
		this->at(playNum);
		this->msg << ENDL;
		this->msg << L"很遗憾，人数已满！";
		this->msg << ENDL;
		this->msg << L"但你可以[加入观战]！";
		this->msg << ENDL;
		return;
	}

	Player *player = new Player;
	player->number = playNum;
	this->players.push_back(player);

	this->at(playNum);
	this->msg << ENDL;
	this->msg << L"加入成功，已有玩家" << this->players.size() << L"个，分别是：";
	this->msg << ENDL;
	for (unsigned i = 0; i < this->players.size(); i++) {
		this->msg << i + 1 << L"：";
		this->at(this->players[i]->number);
		this->msg << L"，积分";
		this->readSendScore(this->players[i]->number);
		this->msg << ENDL;
	}

	if (this->players.size() > 1) {
		this->msg << ENDL;
		this->msg << L"玩家数足够，";
		this->msg << L"请输入[启动]或[GO]来启动游戏。";
		this->msg << ENDL;
	}
}

void Desk::exit(int64_t number)
{
	if (this->state == STATE_WAIT) {
		int index = this->getPlayer(number);
		if (index != -1) {
			vector<Player*>::iterator it = this->players.begin() + index;
			this->players.erase(it);
			this->msg << L"退出成功，剩余玩家" << this->players.size() << L"个";
			if (this->players.size() > 0) {
				this->msg << L"，分别是：";
				this->msg << ENDL;
				for (unsigned i = 0; i < this->players.size(); i++) {
					this->msg << i + 1 << L"：";
					this->at(this->players[i]->number);
					this->msg << L"，积分";
					this->readSendScore(this->players[i]->number);
					this->msg << ENDL;
				}
			}
			else {
				this->msg << L"。";
			}
		}

	}
	else {
		this->msg << L"游戏已经开始不能退出！";
		this->msg << ENDL;
		this->msg << L"但你可以使用[弃牌]来放弃游戏！";
	}
}

void Desk::joinWatching(int64_t playNum) {
	int playIndex = this->getPlayer(playNum);
	int watchIndex = this->getWatcher(playNum);

	//非弃牌玩家不能观战
	if (playIndex != -1 && !players[playIndex]->isSurrender) {
		this->at(playNum);
		this->msg << ENDL;
		this->msg << L"你已经加入游戏，请不要作弊！";
		this->msg << ENDL;
		return;
	}
	if (watchIndex != -1) {
		return;
	}
	if (this->players.size() < 2) {
		this->at(playNum);
		this->msg << ENDL;
		this->msg << L"游戏未开始或人数不足，当前无法加入观战模式。";
		this->msg << ENDL;
		return;
	}

	Watcher *watcher = new Watcher;
	watcher->number = playNum;
	this->watchers.push_back(watcher);

	sendWatchingMsg_Join(playNum);
}

void Desk::exitWatching(int64_t playNum) {
	int index = this->getWatcher(playNum);
	if (index != -1) {
		vector<Watcher*>::iterator it = this->watchers.begin() + index;
		this->watchers.erase(it);
	}
}

void Desk::sendWatchingMsg_Join(int64_t joinNum) {
	int index = getWatcher(joinNum);
	Watcher *watcher = watchers[index];

	watcher->msg << L"加入观战模式成功。";
	watcher->msg << ENDL;

	watcher->msg << L"---------------";
	watcher->msg << ENDL;
	watcher->msg << L"当前手牌信息：";
	watcher->msg << ENDL;
	for (unsigned j = 0; j < this->players.size(); j++) {
		watcher->msg << j + 1 << L"：";
		watcher->at(this->players[j]->number);
		watcher->msg << ENDL;

		for (unsigned m = 0; m < players[j]->card.size(); m++) {
			watcher->msg << L"[" << players[j]->card.at(m) << L"]";
		}

		watcher->msg << ENDL;
	}
}

void Desk::sendWatchingMsg_Start() {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->msg << L"---------------";
		watcher->msg << ENDL;
		watcher->msg << L"初始手牌：";
		watcher->msg << ENDL;
		for (unsigned j = 0; j < this->players.size(); j++) {
			watcher->msg << j + 1 << L"：";
			watcher->at(this->players[j]->number);
			watcher->msg << ENDL;

			for (unsigned m = 0; m < players[j]->card.size(); m++) {
				watcher->msg << L"[" << players[j]->card.at(m) << L"]";
			}

			watcher->msg << ENDL;
		}
		watcher->msg << ENDL;
	}
}

void Desk::sendWatchingMsg_Play() {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->at(this->players[currentPlayIndex]->number);
		watcher->msg << L"打出" << this->lastCardType;
		for (unsigned m = 0; m < this->lastCard.size(); m++) {
			watcher->msg << L"[" << this->lastCard.at(m) << L"]";
		}
		watcher->msg << ENDL;

		watcher->msg << L"---------------";
		watcher->msg << ENDL;
		watcher->msg << L"当前剩余手牌：";
		watcher->msg << ENDL;
		for (unsigned j = 0; j < this->players.size(); j++) {
			watcher->msg << j + 1 << L"：";
			watcher->at(this->players[j]->number);
			watcher->msg << ENDL;

			for (unsigned m = 0; m < players[j]->card.size(); m++) {
				watcher->msg << L"[" << players[j]->card.at(m) << L"]";
			}

			watcher->msg << ENDL;
		}
		watcher->msg << ENDL;
		watcher->msg << L"现在轮到";
		watcher->at(this->players[this->currentPlayIndex]->number);
		watcher->msg << L"。";
		watcher->msg << ENDL;
	}
}

void Desk::sendWatchingMsg_ChangeColor() {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->at(this->players[currentPlayIndex]->number);
		watcher->msg << L"选择" << this->lastCard[0].substr(1, 1) << L"色，" << ENDL;

		watcher->msg << L"现在轮到";
		watcher->at(this->players[this->currentPlayIndex]->number);
		watcher->msg << L"。";
		watcher->msg << ENDL;
	}
}

void Desk::sendWatchingMsg_Surrender(int64_t playNum) {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->at(playNum);
		watcher->msg << L"弃牌，";

		watcher->msg << L"现在轮到";
		watcher->at(this->players[this->currentPlayIndex]->number);
		watcher->msg << L"。";
		watcher->msg << ENDL;
	}
}

void Desk::sendWatchingMsg_Over() {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->msg << L"斗地主游戏结束，";
		watcher->msg << (this->whoIsWinner == 1 ? L"地主赢了" : L"农民赢了");
		watcher->msg << ENDL;
		watcher->msg << L"退出观战模式。";
		watcher->msg << ENDL;
	}
}

Desk* Desks::getOrCreateDesk(int64_t deskNum) {

	Desk *desk = NULL;
	int deskIndex = getDesk(deskNum);
	if (deskIndex == -1) {				//没有桌子
		desk = new Desk;
		desk->number = deskNum;
		desks.push_back(desk);
	}
	else {
		desk = casino.desks[deskIndex];
	}

	return desk;
}

void Desk::startGame() {
	if (this->players.size() >= 2 && this->players.size() <= 10 && this->state == STATE_WAIT) {
		this->state = STATE_START;

		//启动挂机检测程序
		time_t rawtime;
		this->lastTime = time(&rawtime);
		this->counter = new thread(&Desk::checkAFK, this);

		this->msg << L"游戏开始，桌号：" << this->number << L"。";
		this->msg << ENDL;
		this->msg << L"游戏挂机检测时间：" << CONFIG_TIME_GAME << L"秒";
		this->msg << ENDL;

		this->msg << L"---------------";
		this->msg << ENDL;

		this->shuffle();

		this->deal();

		this->listPlayers(1);

		this->state = STATE_READYTOGO;

		srand((unsigned)time(NULL));
		this->currentPlayIndex = rand() % this->players.size();
		this->msg << L"请";
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"先出牌。";
	}
	else {
		if (this->state >= STATE_START) {
			//启动游戏往往会多次发送出发提示，此处不再做无效提示
			return;
		}
		else {
			this->msg << L"没有足够的玩家。";
			this->msg << ENDL;
			for (unsigned i = 0; i < this->players.size(); i++) {
				this->msg << i + 1 << L"：";
				this->at(this->players[i]->number);
				this->msg << L"，积分";
				this->readSendScore(this->players[i]->number);
				this->msg << ENDL;
			}
		}
	}
}

void Desk::listCardsOnDesk(Player* player)
{
	for (unsigned m = 0; m < player->card.size(); m++) {
		this->msg << L"[" << player->card.at(m) << L"]";
	}
}

void Desk::checkAFK() {
	time_t rawtime;
	int64_t timeNow = time(&rawtime);

	while (this->state < STATE_READYTOGO) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	this->warningSent = false;

	while (this->state == STATE_READYTOGO || this->state == STATE_GAMING) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		timeNow = time(&rawtime);

		if (timeNow - this->lastTime > CONFIG_TIME_GAME) {
			this->warningSent = false;

			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"挂机，惩罚2张牌。";
			this->msg << ENDL;
			this->msg << L"---------------";
			this->msg << ENDL;
			this->damageCount += 2;

			//摸牌后会自动过到下一位玩家
			this->drawCards(this->players[this->currentPlayIndex]->number, this->damageCount);

			this->sendMsg(this->subType);
			this->msg.str(L"");
		}
		else if (!this->warningSent && timeNow - this->lastTime > CONFIG_TIME_GAME - CONFIG_TIME_WARNING) {
			this->warningSent = true;
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"时间剩余" << CONFIG_TIME_WARNING << L"秒。";
			this->msg << ENDL;

			this->sendMsg(this->subType);
			this->msg.str(L"");
		}
	}
}

void Desk::WDFHandle(int64_t playNum, int64_t WDFNum, bool notSuccess) {
	Player* WDFIndex = this->players[this->getPlayer(WDFNum)];
	this->at(WDFNum);
	this->msg << L"的手牌为";
	this->listCardsOnDesk(WDFIndex);
	this->msg << ENDL;

	if (!notSuccess) {
		this->at(playNum);
		this->msg << L"举报成功" << ENDL;
		this->drawCards(WDFNum, this->damageCount);
		return;
	}
	else {
		this->at(playNum);
		this->msg << L"举报失败" << ENDL;
		this->damageCount += 2;
		this->drawCards(WDFNum, this->damageCount);
		return;
	}
}