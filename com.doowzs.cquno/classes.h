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
	L"绿0", L"绿1", L"绿1", L"绿2", L"绿2", L"绿3", L"绿3", L"绿4", L"绿4", L"绿5",
	L"绿5", L"绿6", L"绿6", L"绿7", L"绿7", L"绿8", L"绿8", L"绿9", L"绿9", L"绿跳过",
	L"绿跳过", L"绿+2", L"绿+2", L"绿反转", L"绿反转",
	L"黄0", L"黄1", L"黄1", L"黄2", L"黄2", L"黄3", L"黄3", L"黄4", L"黄4", L"黄5",
	L"黄5", L"黄6", L"黄6", L"黄7", L"黄7", L"黄8", L"黄8", L"黄9", L"黄9", L"黄跳过",
	L"黄跳过", L"黄+2", L"黄+2", L"黄反转", L"黄反转",
	L"红0", L"红1", L"红1", L"红2", L"红2", L"红3", L"红3", L"红4", L"红4", L"红5",
	L"红5", L"红6", L"红6", L"红7", L"红7", L"红8", L"红8", L"红9", L"红9", L"红跳过",
	L"红跳过", L"红+2", L"红+2", L"红反转", L"红反转",
	L"蓝0", L"蓝1", L"蓝1", L"蓝2", L"蓝2", L"蓝3", L"蓝3", L"蓝4", L"蓝4", L"蓝5",
	L"蓝5", L"蓝6", L"蓝6", L"蓝7", L"蓝7", L"蓝8", L"蓝8", L"蓝9", L"蓝9", L"蓝跳过",
	L"蓝跳过", L"蓝+2", L"蓝+2", L"蓝反转", L"蓝反转",
	L"变色", L"变色", L"变色", L"变色", L"+4", L"+4", L"+4", L"+4"
};

/*
static const wstring flagColor[5] = {L"绿", L"黄", L"红", L"蓝", L"黑"};
static const wstring flagIndex[14] = { 
	L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7",
	L"8", L"9", L"跳过", L"+2", L"反转", L"W"
};
*/

static const wstring flag[108] = {
	L"绿0", L"绿1", L"绿1", L"绿2", L"绿2", L"绿3", L"绿3", L"绿4", L"绿4", L"绿5",
	L"绿5", L"绿6", L"绿6", L"绿7", L"绿7", L"绿8", L"绿8", L"绿9", L"绿9", L"绿跳过",
	L"绿跳过", L"绿+2", L"绿+2", L"绿反转", L"绿反转",
	L"黄0", L"黄1", L"黄1", L"黄2", L"黄2", L"黄3", L"黄3", L"黄4", L"黄4", L"黄5",
	L"黄5", L"黄6", L"黄6", L"黄7", L"黄7", L"黄8", L"黄8", L"黄9", L"黄9", L"黄跳过",
	L"黄跳过", L"黄+2", L"黄+2", L"黄反转", L"黄反转",
	L"红0", L"红1", L"红1", L"红2", L"红2", L"红3", L"红3", L"红4", L"红4", L"红5",
	L"红5", L"红6", L"红6", L"红7", L"红7", L"红8", L"红8", L"红9", L"红9", L"红跳过",
	L"红跳过", L"红+2", L"红+2", L"红反转", L"红反转",
	L"蓝0", L"蓝1", L"蓝1", L"蓝2", L"蓝2", L"蓝3", L"蓝3", L"蓝4", L"蓝4", L"蓝5",
	L"蓝5", L"蓝6", L"蓝6", L"蓝7", L"蓝7", L"蓝8", L"蓝8", L"蓝9", L"蓝9", L"蓝跳过",
	L"蓝跳过", L"蓝+2", L"蓝+2", L"蓝反转", L"蓝反转",
	L"变色", L"变色", L"变色", L"变色", L"+4", L"+4", L"+4", L"+4"
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

static const wregex allotReg(L"设置积分(\\d+)=(\\d+)");
static const wregex allotReg2(L"设置积分(\\d+)=-(\\d+)");
static const wregex getInfoReg(L"查询积分(\\d+)");
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
	static bool isDrawToDieEnabled();
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

//部分关于player的函数定义在Desks中
class Player
{
public:
	Player();
	wstringstream msg;
	int64_t number;
	vector<wstring> card;
	bool isReady;
	bool isOpenCard;//明牌状态
	bool isSurrender;//投降状态
	void sendMsg();
	void listCards();
};

//部分关于watcher的函数定义在Desks中
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
	int isClockwise;			//存储当前出牌顺序是顺时针（1）还是逆时针（-1）
	int64_t lastTime;
	vector<wstring> cards;
	int64_t number;				//桌号
	vector<Player*> players;	//玩家队列
	vector<Watcher*> watchers;	//观察者队列
	thread *counter;			//倒计时器

	int whoIsWinner;
	int state;					//当前游戏进行阶段
	int lastPlayIndex;			//当前谁出得牌
	int currentPlayIndex;		//该谁出牌
	bool warningSent;			//倒计时警告消息已发送

	bool subType;				//存储消息类型

	vector<wstring> lastCard;	//上位玩家的牌
	wstring lastCardType;		//上位玩家得牌类
	vector<int> *lastWeights;	//上位玩家的牌的权重

	wstringstream msg;

	void join(int64_t playNum);
	void startGame();
	void exit(int64_t playNum);
	void commandList();

	void shuffle();				//洗牌
	void deal();				//发牌

	void play(int64_t playNum, wstring msg);
	void play(vector<wstring> list, int playIndex);	//出牌
	void changeColor(int64_t playNum, wstring color);	//变色
	void getcard(int64_t playNum);					//摸牌
	void surrender(int64_t playNum);				//投降

	void getPlayerInfo(int64_t playNum);
	void getScore(int64_t playNum);

	static int64_t readScore(int64_t playerNum);
	void readSendScore(int64_t playNum);

	void at(int64_t playNum); 
	int getPlayer(int64_t number);					//按qq号获得玩家得索引
	void setNextPlayerIndex(bool skipNext);			//设置下个出牌得玩家索引
	bool ableToPlay(int64_t playNum);				//判断能否出牌，不能出牌则摸牌
	void drawCards(int64_t playNum, int amount);	//摸牌
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
	int getWatcher(int64_t number);						//按qq号获得玩家得索引

	void checkAFK();
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