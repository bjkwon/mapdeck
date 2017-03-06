#include "WndDlg0.h"
#include "FileDlg.h" // from common/include

#ifndef CWAVDECK
#define CWAVDECK

class CWavDeckDlg : public CWndDlg
{
public:
	char iniFile[256];
	char wavlistFile[256];
	CFileDlg fileDlg;
	HWND hList, hLog;
	LVCOLUMN LvCol; // Make Coluom struct for ListView
	LVITEM LvItem;  // ListView Item struct
	bool streaming;
	bool continuePlay;
	int playID;
	int beginID;
	int lastBeginID;
	int playCount;
	HWND hStatusbar;

	int ReadWavlistUpdateScreen(const char * fname, char *estr);
	int UpdateINI(const char* fname, char *estr);
	void pipecallshow(char *buf, char *PipeReturnMsg, char *errstr);
	int prepareplay(char *fname);

	CWavDeckDlg();
	virtual ~CWavDeckDlg();

	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	void OnFlyConnected(char *hostname, char* ipa);
	void OnFlyClosed();
	void OnFlyArrived(WORD command, WORD len, void* inBuffer);
//	HBRUSH OnCtlColorStatic(HDC hdc, HWND hCtrl, int);
	void OnSize(UINT state, int cx, int cy);
	void OnCommand(int idc, HWND hwndCtl, UINT event);
	void OnClose();
	void OnDestroy();
	void OnNotify(int idcc, NMHDR *pnm);
	LRESULT ProcessCustomDraw (NMHDR *lParam);

	//	void OnTimer(HWND hwnd, UINT id);
	void OnStreamingDone();
	void PlayLine(int lineID);

};

#endif CWAVDECK