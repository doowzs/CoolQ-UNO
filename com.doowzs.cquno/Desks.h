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
	this->lastWDFPlayer = 0;
	this->damageCount = 0;

	this->whoIsWinner = -1;
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

	if (hasWin) {
		vector<int> scores;
		int winScore = 0;
		for (unsigned i = 0; i < this->players.size(); i++) {
			scores.push_back(0);
			if (i != this->whoIsWinner) {
				for (unsigned j = 0; j < this->players[i]->card.size(); j++) {
					if (Util::findFlag(this->players[i]->card[j]) >= 100) {		//����
						scores[i] -= 50;
						continue;
					}
					if (Util::findFlag(this->players[i]->card[j]) % 25 >= 19) {	//������
						scores[i] -= 20;
						continue;
					}
					scores[i] -= (Util::findFlag(this->players[i]->card[j]) % 25 + 1) / 2;	//������
				}
			}
			winScore += -scores[i];			//Ӯ�һ������
		}
		scores[whoIsWinner] = winScore;

		for (unsigned i = 0; i < this->players.size(); i++) {
			this->msg << i + 1 << L"��";
			this->at(this->players[i]->number);
			this->msg << L"[" << (i==this->whoIsWinner? L"ʤ��" : L"ʧ��")
				<< scores[i] << L"��]" << ENDL;
			Admin::addScore(this->players[i]->number, scores[i]);
		}
		return;
	}
	else {
		for (unsigned i = 0; i < this->players.size(); i++) {
			this->msg << i + 1 << L"��";
			this->at(this->players[i]->number);
			this->msg << L"��" << this->players[i]->card.size();
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

	//���������ƵĿ����ԣ����ﲻ�ж��Ƿ��ǵ�ǰ��ң�����������һ�������ж�
	if (playIndex == -1
		|| (!(this->state == STATE_GAMING && this->turn > 0)
			&& !(this->state == STATE_READYTOGO && this->turn == 0))
		|| length <= 2) {	//��Ϣ���ȱ������2����+�ƣ�
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
	}

	this->play(msglist, playIndex);
}

void Desk::play(vector<wstring> list, int playIndex)
{
	//��ȫһ�£����ƣ���һ�γ��ƿ϶�����ʵ�֣�
	if (this->turn > 0 && list[0] == this->lastCard[0]) {
		this->currentPlayIndex = playIndex;
	}

	//���Ʋ��ɹ���ǵ�ǰ��ң���ִֹ��
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
			this->msg << L"�涪�ˣ����û����Ҫ�����ƣ��᲻���棿";
			return;
		}
	}

	//�жϳ����Ƿ�Ϸ�
	bool isLegalPlay = false;

	this->lastWDFPlayer = 0;
	this->isLegalWDF = !this->ableToPlay_NoWDF(this->players[playIndex]->number);

	if ((this->turn == 0													//�״γ���
		|| list[0].substr(0, 1) == this->lastCard[0].substr(0, 1)		//��ͬ��ɫ
		|| list[0].substr(1, 1) == this->lastCard[0].substr(1, 1)		//��ͬ����
		|| list[0].substr(0, 2) == L"��ɫ"								//������
		|| list[0].substr(0, 2) == L"+4"
		|| (this->lastCard[0].substr(0, 1) == L"Z"
			&& list[0].substr(0, 1) == this->lastCard[0].substr(1, 1)))	//��ɫ�����
		&& (cardCount == 1 ||
		(cardCount == 2 && list[0] == list[1]))) {
		isLegalPlay = true;
	}

	if (this->damageCount > 0
		&& (list[0].substr(0, 1) != L"+" && list[0].substr(1, 1) != L"��"
			&& list[0].substr(1, 1) != L"+" && list[0].substr(1, 1) != L"��")) {
		isLegalPlay = false;
	}
	
	if (isLegalPlay) {
		if (this->turn == 0) {
			this->state = STATE_GAMING;
		}

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

		if (list[0] == L"��ɫ") {
			this->msg << L"---------------" << ENDL;
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"��ѡ����ɫ��";
			this->lastCard[0] = L"X" + this->lastCard[0].substr(1,1);
			return;
		}

		if (list[0] == L"+4") {
			this->lastWDFPlayer = this->players[this->currentPlayIndex]->number;

			this->msg << L"---------------" << ENDL;
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"��ѡ����ɫ��";
			this->lastCard[0] = L"Y" + this->lastCard[0].substr(1, 1);
			return;
		}
		
		if (list[0].substr(1, 2) == L"+2") {
			this->damageCount += 2;
			this->msg << L"�ۼƣ�" << this->damageCount << ENDL;
			this->setNextPlayerIndex(false, true);
			return;
		}
		
		if (list[0].substr(1, 2) == L"����") {
			if (this->damageCount == 0) {
				this->setNextPlayerIndex(true, false);
			}
			else {
				this->setNextPlayerIndex(false, true);
			}
			return;
		}

		if (list[0].substr(1, 2) == L"��ת") {
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

		//���0�������˰�˳�򽻻����ƣ������⣬����д
		if (Admin::isSevenOEnabled() && list[0].substr(1, 1) == L"0") {
		}
		//���7����ָ����ҽ�������
		if (Admin::isSevenOEnabled() && list[0].substr(1, 1) == L"7") {
		}

		this->setNextPlayerIndex(false, false);
	}
	else {
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << ENDL;
		this->msg << L"ɵ�����ѣ����ʲô�������⣡ѧ������ٴ�";
		this->msg << ENDL;
	}
}

void Desk::changeColor(int64_t playNum, wstring color) {
	//�Ƿ��������˳�
	if (playNum != this->players[this->currentPlayIndex]->number) {
		return;
	}

	this->at(this->players[currentPlayIndex]->number);
	this->msg << L"ѡ��" << color << L"ɫ��" << ENDL;
	this->lastCard[0] = this->lastCard[0].substr(0, 1) + color;

	//��ת����������һ�����
	this->sendWatchingMsg_ChangeColor();

	if (this->lastCard[0].substr(0, 1) == L"X") {
		this->lastCard[0] = L"Z" + color;
		this->setNextPlayerIndex(false, false);
		return;
	}
	if (this->lastCard[0].substr(0, 1) == L"Y") {
		this->damageCount += 4;
		this->msg << L"�ۼ�" << this->damageCount << ENDL;
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
		this->msg << L"��ǰ��Ϸ״̬�޷����ƣ���Ϸ��ʼ�󷽿����ơ�";
		return;
	}

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
		this->whoIsWinner = -1;
	}

	if (this->whoIsWinner >= 0) {
		this->sendWatchingMsg_Over();

		this->msg << L"��Ϸ������";
		this->at(this->players[this->whoIsWinner]->number);
		this->msg << L"Ӯ�ˡ�" << ENDL;

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

		this->setNextPlayerIndex(false, false);
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

void Desk::setNextPlayerIndex(bool skipNext, bool getCards)
{
	//��¼ʱ��
	time_t rawtime;
	this->lastTime = time(&rawtime);

	this->currentPlayIndex = (this->currentPlayIndex + 1*this->isClockwise 
		+ this->players.size()) % this->players.size();

	this->msg << L"---------------" << ENDL;
	this->msg << L"[" << this->lastCard[0] << L"]";

	//�����һ���ó��Ƶ���ұ�������������set��һλ���
	if (skipNext) {
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"������" << ENDL;
		this->setNextPlayerIndex(false, false);
		return;
	}
	//�����һ��������ƣ�ֱ������
	if (this->players[this->currentPlayIndex]->isSurrender) {
		this->setNextPlayerIndex(false, false);
		return;
	}
	//�����������
	if (this->damageCount > 0) {
		if (this->lastWDFPlayer > 0) {
			//��һ����+4
			this->msg << L"�����ֵ�";
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"����������ɻ������ƣ�" << ENDL;
			return;
		}
		else {
			if (!this->ableToDamage(this->players[this->currentPlayIndex]->number)) {
				//��ͨ���ӣ�����
				this->msg << L"�����ֵ�";
				this->at(this->players[this->currentPlayIndex]->number);
				this->msg << L"��û�п��Գ����ƣ�ϵͳ�Զ����ƣ�" << ENDL;
				this->drawCards(this->players[this->currentPlayIndex]->number, this->damageCount);
				return;
			}
			else {
				//��ͨ���ӣ�����
				this->msg << L"�����ֵ�";
				this->at(this->players[this->currentPlayIndex]->number);
				this->msg << L"������Ե��ӻ�ת�ƻ����ƣ�" << ENDL;
				return;
			}
		}
	}
	//�����һ���ó��Ƶ���ҽ�Ҫ���ƣ���û���ܳ����ƣ����Զ����ơ�
	if (this->damageCount == 0 && !this->ableToPlay(this->players[this->currentPlayIndex]->number)) {
		this->msg << L"�����ֵ�";
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"��û�п��Գ����ƣ�ϵͳ�Զ����ƣ�" << ENDL;
		this->drawCards(this->players[this->currentPlayIndex]->number, 0);
		return;
	}

	this->msg << L"�����ֵ�";
	this->at(this->players[this->currentPlayIndex]->number);
	this->msg << L"��";
	this->msg << ENDL;
}

bool Desk::ableToPlay(int64_t playNum) {
	int index = this->getPlayer(playNum);

	if (this->turn == 0) return true;

	for (unsigned i = 0; i < this->players[index]->card.size(); i++) {
		if (this->players[index]->card[i].substr(0, 1) == this->lastCard[0].substr(0, 1)			//��ͬ��ɫ
			|| this->players[index]->card[i].substr(1, 1) == this->lastCard[0].substr(1, 1)			//��ͬ����
			|| this->players[index]->card[i].substr(0, 2) == L"��ɫ"									//������
			|| this->players[index]->card[i].substr(0, 2) == L"+4"
			|| (this->lastCard[0].substr(0, 1) == L"Z"
				&& this->players[index]->card[i].substr(0, 1) == this->lastCard[0].substr(1, 1))) {	//��ɫ�����
			return true;
		}
	}

	return false;
}

bool Desk::ableToPlay_NoWDF(int64_t playNum) {
	int index = this->getPlayer(playNum);

	if (this->turn == 0) return true;

	for (unsigned i = 0; i < this->players[index]->card.size(); i++) {
		if (this->players[index]->card[i].substr(0, 1) == this->lastCard[0].substr(0, 1)			//��ͬ��ɫ
			|| this->players[index]->card[i].substr(1, 1) == this->lastCard[0].substr(1, 1)			//��ͬ����
			|| this->players[index]->card[i].substr(0, 2) == L"��ɫ"									//������
			|| (this->lastCard[0].substr(0, 1) == L"Z"
				&& this->players[index]->card[i].substr(0, 1) == this->lastCard[0].substr(1, 1))) {	//��ɫ�����
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
					|| this->players[index]->card[i].substr(1, 1) == L"��"
					|| this->players[index]->card[i].substr(1, 1) == L"��")) {
			return true;
		}
	}

	return false;
}

void Desk::drawCards(int64_t playNum, int amount) {
	int index = this->getPlayer(playNum);

	//���Լ��Ļغϲ�������
	if (index != this->currentPlayIndex) return;
	
	int count = 0; 

	if (amount == 0) {
		wstring lastGot;	//��¼���һ���ƣ�һ���ǿ��Դ����
		if (Admin::isCrazyDrawEnabled()) {
			//����ˬ
			while (!this->ableToPlay(playNum)) {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				if (this->cards.empty()) {
					this->msg << L"�ƿ���ˣ���Ϸ������" << ENDL;
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
			this->msg << L"������" << count << L"�ţ���������"
				<< this->players[index]->card.size() << L"�š�" << ENDL;
			this->sendPlayerMsg();
		}
		else {
			//������ˬ������£�ֻ��һ���ƣ�������ܳ�������
			if (this->cards.empty()) {
				this->msg << L"�ƿ���ˣ���Ϸ������" << ENDL;
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
				this->msg << L"������" << count << L"�ţ���������"
					<< this->players[index]->card.size() << L"�š�" << ENDL;
				this->sendPlayerMsg();
			}
			else {
				this->at(playNum);
				this->msg << L"������" << count << L"�ţ���������"
					<< this->players[index]->card.size() << L"�ţ��޷����ƣ�������" << ENDL;
				this->sendPlayerMsg();
				this->setNextPlayerIndex(false, false);
			}
		}
	}
	else {
		for (int i = 0; i < amount; i++) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));

			if (this->cards.empty()) {
				this->msg << L"�ƿ���ˣ���Ϸ������" << ENDL;
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
		this->msg << L"������" << count << L"�ţ���������"
			<< this->players[index]->card.size() << L"�š�" << ENDL;
		this->sendPlayerMsg();

		//����+4����Ϣ
		if (this->lastCard[0].substr(0, 1) == L"Y") {
			this->lastCard[0] = L"Z" + this->lastCard[0].substr(1, 1);
		}

		//ֻ���˺����ƲŻ��Զ�����һ����
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
		//��մ洢������bug
		players[i]->card.clear();

		//UNO��ʼ��Ϊ7��
		for (j = 0; j < 7; j++) {
			player->card.push_back(this->cards.front());
			this->cards.erase(this->cards.begin());
		}

		sort(player->card.begin(), player->card.end(), Util::compareCard);

		player->msg << L"��ǰ�������" << ENDL
			<< L"������" << (Admin::isFreeDrawEnabled() ? L"����" : L"����") << ENDL
			<< L"����ˬ��" << (Admin::isCrazyDrawEnabled() ? L"����" : L"����") << ENDL
			<< L"0-7���ƣ�" << (Admin::isSevenOEnabled() ? L"����" : L"����") << ENDL
			<< L"�˺����ӣ�" << (Admin::isRegressiveEnabled() ? L"����" : L"����") << ENDL
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

		this->shuffle();

		this->deal();

		this->listPlayers(1);

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
			this->msg << L"�һ����ͷ�2���ơ�";
			this->msg << ENDL;
			this->msg << L"---------------";
			this->msg << ENDL;
			this->damageCount += 2;

			//���ƺ���Զ�������һλ���
			this->drawCards(this->players[this->currentPlayIndex]->number, this->damageCount);

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

void Desk::WDFHandle(int64_t playNum, int64_t WDFNum, bool notSuccess) {
	Player* WDFIndex = this->players[this->getPlayer(WDFNum)];
	this->at(WDFNum);
	this->msg << L"������Ϊ";
	this->listCardsOnDesk(WDFIndex);
	this->msg << ENDL;

	if (!notSuccess) {
		this->at(playNum);
		this->msg << L"�ٱ��ɹ�" << ENDL;
		this->drawCards(WDFNum, this->damageCount);
		return;
	}
	else {
		this->at(playNum);
		this->msg << L"�ٱ�ʧ��" << ENDL;
		this->damageCount += 2;
		this->drawCards(WDFNum, this->damageCount);
		return;
	}
}