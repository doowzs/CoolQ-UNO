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

	//�Ƚ���һ���Ʒ��룬�������ɳ�0����
	this->cards.push_back(cardDest[0]);
	for (int i = 1; i < 108; i++) {
		this->cards.insert(this->cards.begin()+(rand()%this->cards.size()),cardDest[i]);
	}

	this->state = STATE_WAIT;
	this->lastPlayIndex = -1;				//��ǰ˭������
	this->currentPlayIndex = -1;			//��˭����

	vector<wstring> lastCard;				//��λ��ҵ���
	//this->lastCardType = L"";				//��λ��ҵ�����

	this->whoIsWinner = 0;
	this->turn = 0;
	this->isClockwise = 1;
	this->lastTime = 0;						//���һ�η���ʱ�䣬��¼��
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
		this->msg << L"��Ϸ��δ��ʼ������б�";
		this->msg << ENDL;
	}
	else {
		this->msg << L"���ƴ�����" << this->turn;
		this->msg << ENDL;
	}

	for (unsigned i = 0; i < this->players.size(); i++) {
		this->msg << i + 1 << L"��";

		this->at(this->players[i]->number);
		this->msg << L"��û�����أ�" << this->turn;
		this->msg << ENDL;
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

	if (playIndex == -1 || playIndex != this->currentPlayIndex
		|| (!(this->state == STATE_GAMING && this->turn > 0)
			&& !(this->state == STATE_READYTOGO && this->turn == 0))
		|| length < 2) {
		return;
	}

	vector<wstring> msglist;

	//��ȡ������Ϣ
	for (int i = 1; i < length - 1; i++) {
		wstring color = msg.substr(i, 1);
		i++;
		wstring face = msg.substr(i, 1);
		if (face == L"��") {
			face = L"����";
			i++;
		}
		else if (face == L"+" || face == L"��") {
			face = L"+2";
			i++;
		}
		else if (face == L"��") {
			face = L"��ת";
			i++;
		}
		
		msglist.push_back(color + face);
		this->msg << L"ʶ������" << color + face;
		this->sendMsg(this->subType);
	}

	this->play(msglist, playIndex);
}

void Desk::play(vector<wstring> list, int playIndex)
{

	Player *player = this->players[playIndex];
	vector<wstring> mycardTmp(player->card);

	int cardCount = list.size();

	for (int i = 0; i < cardCount; i++) {
		if (Util::findAndRemove(mycardTmp, list[i]) == -1) {
			this->at(this->players[currentPlayIndex]->number);
			this->msg << ENDL;
			this->msg << L"�涪�ˣ����û����Ҫ�����ƣ��᲻���棿";
			return;
		}
	}

	//�жϳ����Ƿ�Ϸ�
	bool isLegalPlay = false;

	if (((this->turn == 0 || this->lastCard.empty())						//�״γ���
			|| list[0].substr(0, 1) == this->lastCard[0].substr(0, 1)		//��ͬ��ɫ
			|| list[0].substr(1, 1) == this->lastCard[0].substr(1, 1)		//��ͬ����
			|| list[0].substr(0, 2) == L"��ɫ"								//������
			|| list[0].substr(0, 2) == L"+4"
			|| ((this->lastCard[0].substr(0, 1) == L"X" || this->lastCard[0].substr(0, 1) == L"Y")
				&& list[0].substr(0, 1) == this->lastCard[0].substr(1, 1)))	//��ɫ�����
		&& (cardCount == 1 || 
			(cardCount == 2 && list[0] == list[1]))) {
		isLegalPlay = true;
	}
	
	if (isLegalPlay) {
		if (this->turn == 0) {
			this->state = STATE_GAMING;
		}

		//��¼����ʱ��
		time_t rawtime;
		this->lastTime = time(&rawtime);
		//��ֹ��ʾ����Ƴ���bug
		this->warningSent = false;

		player->card = mycardTmp;
		this->lastCard = list;
		this->lastPlayIndex = this->currentPlayIndex;
		this->turn++;

		//��������������ƿ�
		srand((unsigned)time(NULL));
		for (unsigned i = 0; i < this->lastCard.size(); i++) {
			this->cards.insert(this->cards.begin()+rand()%this->cards.size(),this->lastCard[i]);
		}
		this->shuffle();

		if (mycardTmp.size() == 0) {//Ӯ�ˡ�
			this->whoIsWinner = playIndex;
			this->sendWatchingMsg_Over();

			this->msg << L"---------------";
			this->msg << ENDL;
			this->msg << L"�������㣺";
			this->msg << ENDL;
			this->listPlayers(3);

			casino.gameOver(this->number);
			return;
		}

		player->listCards();

		if (player->card.size() == 1) {
			this->msg << L"UNO��";
			this->msg << ENDL;
			this->at(player->number);
			this->msg << L"��ʣ��" << player->card.size() << L"���ƣ�";
			this->msg << ENDL;
			this->msg << L"---------------";
			this->msg << ENDL;
		}

		this->at(this->players[currentPlayIndex]->number);
		this->msg << L"���";
		for (unsigned m = 0; m < this->lastCard.size(); m++) {
			this->msg << L"[" << this->lastCard.at(m) << L"]";
		}
		this->msg << ENDL;

		//��ս������������ת��ս����������һλ��ң����������Ϣ����
		this->sendWatchingMsg_Play();

		this->msg << L"---------------";
		this->msg << ENDL;
		this->msg << L"ʣ����������";
		this->msg << ENDL;
		for (unsigned i = 0; i < this->players.size(); i++) {
			this->msg << i + 1 << L"��";
			this->at(this->players[i]->number);
			this->msg << L"��" << static_cast<int>(this->players[i]->card.size());
			this->msg << ENDL;
		}
		this->msg << ENDL;

		if (list[0] == L"��ɫ") {
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"��ѡ����ɫ��";
			this->lastCard[0] = L"X" + this->lastCard[0].substr(1,1);
			return;
		}

		if (list[0] == L"+4") {
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"��ѡ����ɫ��";
			this->lastCard[0] = L"Y" + this->lastCard[0].substr(1, 1);
			return;
		}
		
		if (list[0].substr(1, 2) == L"+2") {
			this->setNextPlayerIndex(false);
			this->drawCards(this->players[this->currentPlayIndex]->number, 2);
			this->setNextPlayerIndex(false);
			return;
		}
		
		if (list[0].substr(1, 2) == L"����") {
			this->setNextPlayerIndex(true);
			return;
		}

		if (list[0].substr(1, 2) == L"��ת") {
			if (this->players.size() == 2) {
				this->setNextPlayerIndex(true);
				return;
			}
			else {
				this->isClockwise = 0 - this->isClockwise;
			}
		}

		//���0�������˰�˳�򽻻�����
		if (Admin::isSevenOEnabled() && list[0].substr(1, 1) == L"0") {
			for (unsigned i = 0; i < this->cards.size(); i++) {
				swap(this->players[i]->card, this->players[(i + 1) % this->players.size()]->card);
			}
		}

		this->setNextPlayerIndex(false);
	}
	else {
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << ENDL;
		this->msg << L"ɵ�����ѣ����ʲô�������⣡ѧ������ٴ�";
		this->msg << ENDL;
	}
}

void Desk::changeColor(int64_t playNum, wstring color) {
	this->at(this->players[currentPlayIndex]->number);
	this->msg << L"ѡ��" << color << L"ɫ��";
	this->lastCard[0] = this->lastCard[0].substr(0, 1) + color;

	//��ת����������һ�����
	this->sendWatchingMsg_ChangeColor();

	if (this->lastCard[0].substr(0, 1) == L"X") {
		this->setNextPlayerIndex(false);
	}
	else if (this->lastCard[0].substr(0, 1) == L"Y") {
		this->setNextPlayerIndex(false);
		this->drawCards(this->players[this->currentPlayIndex]->number, 4);
		this->setNextPlayerIndex(false);
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
		this->msg << L"��ǰ��Ϸ״̬�޷����ƣ���Ϸ��ʼ�󷽿����ơ�";
		return;
	}

	//��¼����ʱ��
	time_t rawtime;
	this->lastTime = time(&rawtime);
	//��ֹ��ʾ�����Ƴ���bug
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
		this->whoIsWinner = 0;
	}

	if (this->whoIsWinner > 0) {
		this->sendWatchingMsg_Over();

		this->msg << L"��Ϸ������";
		this->msg << ENDL;

		this->msg << L"---------------";
		this->msg << ENDL;
		this->msg << L"�������㣺";
		this->msg << ENDL;
		this->listPlayers(3);

		casino.gameOver(this->number);
		return;
	}


	if (this->currentPlayIndex == index) {
		this->at(playNum);
		this->msg << L"���ƣ�";

		this->setNextPlayerIndex(false);
	}
	else {
		this->at(playNum);
		this->msg << L"���ơ�";
		this->msg << ENDL;
	}

	//��ս����
	this->sendWatchingMsg_Surrender(playNum);
}

void Desk::getPlayerInfo(int64_t playNum)
{
	this->msg << Admin::readDataType() << L" " << Admin::readVersion();// << L" CST";
	this->msg << ENDL;
	this->at(playNum);
	this->msg << L"��";
	//this->msg << ENDL;
	this->msg << L"����";
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

	//���ٹһ�������
	casino.desks[index]->state = STATE_OVER;
	casino.desks[index]->counter->join();

	vector<Desk*>::iterator it = casino.desks.begin() + index;
	casino.desks.erase(it);
}

void Desk::setNextPlayerIndex(bool skipNext)
{
	this->currentPlayIndex = (this->currentPlayIndex + 1*this->isClockwise) % this->players.size();

	//�����һ���ó��Ƶ�������������ˣ�������set��һλ���
	if (skipNext || this->players[this->currentPlayIndex]->isSurrender) {
		this->setNextPlayerIndex(false);
	}

	//�����һ���ó��Ƶ���ҽ�Ҫ���ƣ���û���ܳ����ƣ����Զ����ơ�
	if (!skipNext && !this->ableToPlay(this->players[this->currentPlayIndex]->number)) {
		//this->drawCards(this->players[this->currentPlayIndex]->number, 0);
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"��û�п��Գ����ƣ������ƣ�" << ENDL;
	}

	this->msg << L"�����ֵ�";
	this->at(this->players[this->currentPlayIndex]->number);
	this->msg << L"��";
	this->msg << ENDL;
}

bool Desk::ableToPlay(int64_t playNum) {
	int index = this->getPlayer(playNum);

	if (this->turn == 0 || this->lastCard.empty()) return true;

	for (unsigned i = 0; i < this->players[index]->card.size(); i++) {
		if (this->players[index]->card[i].substr(0, 1) == this->lastCard[0].substr(0, 1)			//��ͬ��ɫ
			|| this->players[index]->card[i].substr(1, 1) == this->lastCard[0].substr(1, 1)			//��ͬ����
			|| this->players[index]->card[i].substr(0, 2) == L"��ɫ"														//������
			|| this->players[index]->card[i].substr(0, 2) == L"+4"
			|| ((this->lastCard[0].substr(0, 1) == L"X" || this->lastCard[0].substr(0, 1) == L"Y")
				&& this->players[index]->card[i].substr(0, 1) == this->lastCard[0].substr(1, 1))){//��ɫ�����
			return true;
		}
	}

	return false;
}

void Desk::drawCards(int64_t playNum, int amount) {
	int index = this->getPlayer(playNum);
	int count = 0;

	if (amount == 0) {
		while (!this->ableToPlay(playNum)) {
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
		this->msg << L"���ƽ�����������" << count << L"�ţ���������"
			<< this->players[index]->card.size() << L"�š�" << ENDL;
		this->sendPlayerMsg();
	}
	else {
		for (int i = 0; i < amount; i++) {
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
		this->msg << L"���ƽ�����������" << count << L"�ţ���������"
			<< this->players[index]->card.size() << L"�š�" << ENDL;
		this->sendPlayerMsg();
	}
}

void Desk::deal() {
	unsigned i, k, j;
	for (i = k = 0; i < this->players.size(); i++) {
		Player *player = players[i];
		//��մ洢������bug
		players[i]->card.clear();

		//UNO��ʼ��Ϊ7��
		for (j = 0; j < 7; j++) {
			player->card.push_back(this->cards.front());
			this->cards.erase(this->cards.begin());
		}

		sort(player->card.begin(), player->card.end(), Util::compareCard);

		player->msg << L"��ǰ�������" << ENDL
			<< L"����ˬ��" << (Admin::isDrawToDieEnabled() ? L"����" : L"����") << ENDL
			<< L"�˺����ӣ�" << (Admin::isRegressiveEnabled() ? L"����" : L"����") << ENDL
			<< L"0-7���ƣ�" << (Admin::isSevenOEnabled() ? L"����" : L"����") << ENDL
			<< L"ͬ�������ƣ�" << (Admin::isJumpInEnabled() ? L"����" : L"����") << ENDL;
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
		this->msg << L"���Ѿ�������Ϸ��";
		this->msg << ENDL;
		return;
	}

	if (this->players.size() > 10) {
		this->at(playNum);
		this->msg << ENDL;
		this->msg << L"���ź�������������";
		this->msg << ENDL;
		this->msg << L"�������[�����ս]��";
		this->msg << ENDL;
		return;
	}

	Player *player = new Player;
	player->number = playNum;
	this->players.push_back(player);

	this->at(playNum);
	this->msg << ENDL;
	this->msg << L"����ɹ����������" << this->players.size() << L"�����ֱ��ǣ�";
	this->msg << ENDL;
	for (unsigned i = 0; i < this->players.size(); i++) {
		this->msg << i + 1 << L"��";
		this->at(this->players[i]->number);
		this->msg << L"������";
		this->readSendScore(this->players[i]->number);
		this->msg << ENDL;
	}

	if (this->players.size() > 1) {
		this->msg << ENDL;
		this->msg << L"������㹻��";
		this->msg << L"������[����]��[GO]��������Ϸ��";
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
			this->msg << L"�˳��ɹ���ʣ�����" << this->players.size() << L"��";
			if (this->players.size() > 0) {
				this->msg << L"���ֱ��ǣ�";
				this->msg << ENDL;
				for (unsigned i = 0; i < this->players.size(); i++) {
					this->msg << i + 1 << L"��";
					this->at(this->players[i]->number);
					this->msg << L"������";
					this->readSendScore(this->players[i]->number);
					this->msg << ENDL;
				}
			}
			else {
				this->msg << L"��";
			}
		}

	}
	else {
		this->msg << L"��Ϸ�Ѿ���ʼ�����˳���";
		this->msg << ENDL;
		this->msg << L"�������ʹ��[����]��������Ϸ��";
	}
}

void Desk::joinWatching(int64_t playNum) {
	int playIndex = this->getPlayer(playNum);
	int watchIndex = this->getWatcher(playNum);

	//��������Ҳ��ܹ�ս
	if (playIndex != -1 && !players[playIndex]->isSurrender) {
		this->at(playNum);
		this->msg << ENDL;
		this->msg << L"���Ѿ�������Ϸ���벻Ҫ���ף�";
		this->msg << ENDL;
		return;
	}
	if (watchIndex != -1) {
		return;
	}
	if (this->players.size() < 2) {
		this->at(playNum);
		this->msg << ENDL;
		this->msg << L"��Ϸδ��ʼ���������㣬��ǰ�޷������սģʽ��";
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

	watcher->msg << L"�����սģʽ�ɹ���";
	watcher->msg << ENDL;

	watcher->msg << L"---------------";
	watcher->msg << ENDL;
	watcher->msg << L"��ǰ������Ϣ��";
	watcher->msg << ENDL;
	for (unsigned j = 0; j < this->players.size(); j++) {
		watcher->msg << j + 1 << L"��";
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
		watcher->msg << L"��ʼ���ƣ�";
		watcher->msg << ENDL;
		for (unsigned j = 0; j < this->players.size(); j++) {
			watcher->msg << j + 1 << L"��";
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
		watcher->msg << L"���" << this->lastCardType;
		for (unsigned m = 0; m < this->lastCard.size(); m++) {
			watcher->msg << L"[" << this->lastCard.at(m) << L"]";
		}
		watcher->msg << ENDL;

		watcher->msg << L"---------------";
		watcher->msg << ENDL;
		watcher->msg << L"��ǰʣ�����ƣ�";
		watcher->msg << ENDL;
		for (unsigned j = 0; j < this->players.size(); j++) {
			watcher->msg << j + 1 << L"��";
			watcher->at(this->players[j]->number);
			watcher->msg << ENDL;

			for (unsigned m = 0; m < players[j]->card.size(); m++) {
				watcher->msg << L"[" << players[j]->card.at(m) << L"]";
			}

			watcher->msg << ENDL;
		}
		watcher->msg << ENDL;
		watcher->msg << L"�����ֵ�";
		watcher->at(this->players[this->currentPlayIndex]->number);
		watcher->msg << L"��";
		watcher->msg << ENDL;
	}
}

void Desk::sendWatchingMsg_ChangeColor() {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->at(this->players[currentPlayIndex]->number);
		watcher->msg << L"ѡ��" << this->lastCard[0].substr(1, 1) << L"ɫ��" << ENDL;

		watcher->msg << L"�����ֵ�";
		watcher->at(this->players[this->currentPlayIndex]->number);
		watcher->msg << L"��";
		watcher->msg << ENDL;
	}
}

void Desk::sendWatchingMsg_Surrender(int64_t playNum) {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->at(playNum);
		watcher->msg << L"���ƣ�";

		watcher->msg << L"�����ֵ�";
		watcher->at(this->players[this->currentPlayIndex]->number);
		watcher->msg << L"��";
		watcher->msg << ENDL;
	}
}

void Desk::sendWatchingMsg_Over() {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->msg << L"��������Ϸ������";
		watcher->msg << (this->whoIsWinner == 1 ? L"����Ӯ��" : L"ũ��Ӯ��");
		watcher->msg << ENDL;
		watcher->msg << L"�˳���սģʽ��";
		watcher->msg << ENDL;
	}
}

Desk* Desks::getOrCreateDesk(int64_t deskNum) {

	Desk *desk = NULL;
	int deskIndex = getDesk(deskNum);
	if (deskIndex == -1) {				//û������
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

		//�����һ�������
		time_t rawtime;
		this->lastTime = time(&rawtime);
		this->counter = new thread(&Desk::checkAFK, this);

		this->msg << L"��Ϸ��ʼ�����ţ�" << this->number << L"��";
		this->msg << ENDL;
		this->msg << L"��Ϸ�һ����ʱ�䣺" << CONFIG_TIME_GAME << L"��";
		this->msg << ENDL;

		this->msg << L"---------------";
		this->msg << ENDL;

		this->listPlayers(1);

		this->shuffle();

		this->deal();

		this->state = STATE_READYTOGO;

		srand((unsigned)time(NULL));
		this->currentPlayIndex = rand() % this->players.size();
		this->msg << L"��";
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"�ȳ��ơ�";
	}
	else {
		if (this->state >= STATE_START) {
			//������Ϸ�������η��ͳ�����ʾ���˴���������Ч��ʾ
			return;
		}
		else {
			this->msg << L"û���㹻����ҡ�";
			this->msg << ENDL;
			for (unsigned i = 0; i < this->players.size(); i++) {
				this->msg << i + 1 << L"��";
				this->at(this->players[i]->number);
				this->msg << L"������";
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
			this->msg << L"�һ����ͷ�2����";
			this->msg << ENDL;
			this->drawCards(this->players[this->currentPlayIndex]->number, 2);

			this->msg << L"---------------";
			this->msg << ENDL;
			this->setNextPlayerIndex(false);
			this->lastTime = timeNow;

			this->sendMsg(this->subType);
			this->msg.str(L"");
		}
		else if (!this->warningSent && timeNow - this->lastTime > CONFIG_TIME_GAME - CONFIG_TIME_WARNING) {
			this->warningSent = true;
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"ʱ��ʣ��" << CONFIG_TIME_WARNING << L"�롣";
			this->msg << ENDL;

			this->sendMsg(this->subType);
			this->msg.str(L"");
		}
	}
}