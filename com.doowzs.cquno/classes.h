#pragma once

#include <time.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <tchar.h>  
#include <regex> 
#include <thread>

#include "cqp.h"
#include "stringapiset.h"
#include "WinBase.h"

using namespace std;

static const wstring cardDest[108] = {
	L"��0", L"��1", L"��1", L"��2", L"��2", L"��3", L"��3", L"��4", L"��4", L"��5",
	L"��5", L"��6", L"��6", L"��7", L"��7", L"��8", L"��8", L"��9", L"��9", L"������",
	L"������", L"��+2", L"��+2", L"�̷�ת", L"�̷�ת",
	L"��0", L"��1", L"��1", L"��2", L"��2", L"��3", L"��3", L"��4", L"��4", L"��5",
	L"��5", L"��6", L"��6", L"��7", L"��7", L"��8", L"��8", L"��9", L"��9", L"������",
	L"������", L"��+2", L"��+2", L"�Ʒ�ת", L"�Ʒ�ת",
	L"��0", L"��1", L"��1", L"��2", L"��2", L"��3", L"��3", L"��4", L"��4", L"��5",
	L"��5", L"��6", L"��6", L"��7", L"��7", L"��8", L"��8", L"��9", L"��9", L"������",
	L"������", L"��+2", L"��+2", L"�췴ת", L"�췴ת",
	L"��0", L"��1", L"��1", L"��2", L"��2", L"��3", L"��3", L"��4", L"��4", L"��5",
	L"��5", L"��6", L"��6", L"��7", L"��7", L"��8", L"��8", L"��9", L"��9", L"������",
	L"������", L"��+2", L"��+2", L"����ת", L"����ת",
	L"��ɫ", L"��ɫ", L"��ɫ", L"��ɫ", L"+4", L"+4", L"+4", L"+4"
};

/*
static const wstring flagColor[5] = {L"��", L"��", L"��", L"��", L"��"};
static const wstring flagIndex[14] = { 
	L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7",
	L"8", L"9", L"����", L"+2", L"��ת", L"W"
};
*/

static const wstring flag[108] = {
	L"��0", L"��1", L"��1", L"��2", L"��2", L"��3", L"��3", L"��4", L"��4", L"��5",
	L"��5", L"��6", L"��6", L"��7", L"��7", L"��8", L"��8", L"��9", L"��9", L"������",
	L"������", L"��+2", L"��+2", L"�̷�ת", L"�̷�ת",
	L"��0", L"��1", L"��1", L"��2", L"��2", L"��3", L"��3", L"��4", L"��4", L"��5",
	L"��5", L"��6", L"��6", L"��7", L"��7", L"��8", L"��8", L"��9", L"��9", L"������",
	L"������", L"��+2", L"��+2", L"�Ʒ�ת", L"�Ʒ�ת",
	L"��0", L"��1", L"��1", L"��2", L"��2", L"��3", L"��3", L"��4", L"��4", L"��5",
	L"��5", L"��6", L"��6", L"��7", L"��7", L"��8", L"��8", L"��9", L"��9", L"������",
	L"������", L"��+2", L"��+2", L"�췴ת", L"�췴ת",
	L"��0", L"��1", L"��1", L"��2", L"��2", L"��3", L"��3", L"��4", L"��4", L"��5",
	L"��5", L"��6", L"��6", L"��7", L"��7", L"��8", L"��8", L"��9", L"��9", L"������",
	L"������", L"��+2", L"��+2", L"����ת", L"����ת",
	L"��ɫ", L"��ɫ", L"��ɫ", L"��ɫ", L"+4", L"+4", L"+4", L"+4"
};

static const wstring ENDL = L"\r\n";

static const int STATE_WAIT = 0;
static const int STATE_START = 1;
static const int STATE_READYTOGO = 2;
static const int STATE_GAMING = 3;
static const int STATE_OVER = 4;

static const wstring CONFIG_PATH = L".\\app\\com.doowzs.cquno\\config.ini";
static const wstring DATA_PATH = L".\\app\\com.doowzs.cquno\\data.ini";
static const wstring CONFIG_DIR = L".\\app\\com.doowzs.cquno\\";

static const int CONFIG_TIME_GAME = 60;
static const int CONFIG_TIME_WARNING = 15;
static const wstring CONFIG_VERSION = L"0.0.0";

static const wregex allotReg(L"���û���(\\d+)=(\\d+)");
static const wregex allotReg2(L"���û���(\\d+)=-(\\d+)");
static const wregex getInfoReg(L"��ѯ����(\\d+)");
static const wregex numberReg(L"\\d+");

class Util {
public:
	static int AC;

	static void testMsg(bool subType, int64_t playNum, int64_t desknum, const char * str);
	static void sendGroupMsg(int64_t groupid, const char *msg);
	static void sendDiscussMsg(int64_t groupid, const char *msg);
	static void sendPrivateMsg(int64_t groupid, const char *msg);
	static int findAndRemove(vector<wstring> &dest, wstring str);
	static int find(vector<wstring> &dest, wstring str);
	static int findFlag(wstring str);
	static int desc(int a, int b);
	static int asc(int a, int b);
	static bool compareCard(const wstring &carda, const wstring &cardb);
	static void trim(wstring &s);
	static void toUpper(wstring &str);
	static void setAC(int32_t ac);
	static string wstring2string(wstring wstr);
	static wstring string2wstring(string str);
	static void strcat_tm(char* result, rsize_t size, tm now_time);
	static void mkdir();
};

class Admin {
public:
	static wstring readString();
	static int64_t readAdmin();
	static int64_t readScore(int64_t playerNum);
	static bool isAdmin(int64_t playNum);
	static bool writeAdmin(int64_t playerNum);
	static bool addScore(int64_t playerNum, int64_t score);

	static int64_t readVersion();
	static bool writeVersion();
	static wstring readDataType();
	static bool writeDataType();
	static bool backupData(int64_t playNum);

	//��ȡ����Ĺ���
	static bool isFreeDrawEnabled();
	static bool isCrazyDrawEnabled();
	static bool isRegressiveEnabled();
	static bool isSevenOEnabled();
	static bool isJumpInEnabled();

	static bool IAmAdmin(int64_t playerNum);
	static bool resetGame(int64_t playNum);
	static void getPlayerInfo(int64_t playNum);
	static bool allotScoreTo(wstring msg, int64_t playNum);
	static bool allotScoreTo2(wstring msg, int64_t playNum);
	static bool gameOver(wstring msg, int64_t playNum);

private:
	static bool writeScore(int64_t playerNum, int64_t score);
};

//���ֹ���player�ĺ���������Desks��
class Player
{
public:
	Player();
	wstringstream msg;
	int64_t number;
	vector<wstring> card;
	bool isReady;
	bool isOpenCard;//����״̬
	bool isSurrender;//Ͷ��״̬
	void sendMsg();
	void listCards();
};

//���ֹ���watcher�ĺ���������Desks��
class Watcher
{
public:
	Watcher();
	wstringstream msg;
	int64_t number;
	void sendMsg();
	void at(int64_t playNum);
};

class Desk {
public:
	Desk();
	int turn;
	int isClockwise;			//�洢��ǰ����˳����˳ʱ�루1��������ʱ�루-1��
	int damageCount;
	bool isLegalWDF;
	int64_t lastWDFPlayer;

	int64_t lastTime;
	vector<wstring> cards;
	int64_t number;				//����
	vector<Player*> players;	//��Ҷ���
	vector<Watcher*> watchers;	//�۲��߶���
	thread *counter;			//����ʱ��

	int whoIsWinner;
	int state;					//��ǰ��Ϸ���н׶�
	int lastPlayIndex;			//��ǰ˭������
	int currentPlayIndex;		//��˭����
	bool warningSent;			//����ʱ������Ϣ�ѷ���

	bool subType;				//�洢��Ϣ����

	vector<wstring> lastCard;	//��λ��ҵ���
	wstring lastCardType;		//��λ��ҵ�����
	vector<int> *lastWeights;	//��λ��ҵ��Ƶ�Ȩ��

	wstringstream msg;

	void join(int64_t playNum);
	void startGame();
	void exit(int64_t playNum);
	void commandList();

	void shuffle();				//ϴ��
	void deal();				//����

	void play(int64_t playNum, wstring msg);
	void play(vector<wstring> list, int playIndex);	//����
	void changeColor(int64_t playNum, wstring color);	//��ɫ
	void getcard(int64_t playNum);					//����
	void surrender(int64_t playNum);				//Ͷ��

	void getPlayerInfo(int64_t playNum);
	void getScore(int64_t playNum);

	static int64_t readScore(int64_t playerNum);
	void readSendScore(int64_t playNum);

	void at(int64_t playNum); 
	int getPlayer(int64_t number);					//��qq�Ż����ҵ�����
	void setNextPlayerIndex(bool skipNext, bool getCards);	//�����¸����Ƶ��������
	bool ableToPlay(int64_t playNum);				//�ж��ܷ���ƣ����ܳ���������
	bool ableToPlay_NoWDF(int64_t playNum);			//�жϳ���+4�ܷ����
	bool ableToDamage(int64_t playNum);				//�ж��ܷ������˺�
	void drawCards(int64_t playNum, int amount);	//����
	void listPlayers(int type);
	bool isCanWin(int cardCount, vector<int> *Weights, wstring type);
	void sendMsg(bool subType);
	void sendPlayerMsg();
	void sendWatcherMsg();
	void listCardsOnDesk(Player* player);

	void joinWatching(int64_t playNum);
	void exitWatching(int64_t playNum);
	void sendWatchingMsg_Join(int64_t joinNum);
	void sendWatchingMsg_Start();
	void sendWatchingMsg_Play();
	void sendWatchingMsg_ChangeColor();
	void sendWatchingMsg_Surrender(int64_t playNum);
	void sendWatchingMsg_Over();
	int getWatcher(int64_t number);						//��qq�Ż����ҵ�����

	void checkAFK();
	void WDFHandle(int64_t playNum, int64_t WDFNum, bool notSuccess);
};

class Desks {
public:
	vector<Desk*> desks;
	Desk * getOrCreateDesk(int64_t deskNum);
	static bool game(bool subType, int64_t deskNum, int64_t playNum, const char *msg);
	static bool game(int64_t playNum, const char *msg);
	int getDesk(int64_t deskNum);
	void gameOver(int64_t deskNum);
	void listDesks();
};

static Desks casino;