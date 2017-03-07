#include "WndDlg0.h"
#include "FileDlg.h" // from common/include

#include <map>
#include <string>


enum SETCOMMAND
{
	MAX,
	RATE,
	ONOFF,
	FB,
	Q,
	BL,
	EQ,
	TC,
	DUMMY,
} ;


class CMapDeckDlg : public CWndDlg
{
public:
	char iniFile[256];
	char mapdir[256];
	int lastTrackPosT, lastTrackPosC;

	CFileDlg fileDlg;
	int nBands;
	map<string,HFONT> efont; 
	HWND hStatusbar;
	string mapfname[4];
	char subj[64];
	unsigned char savedslot[4]; // if saved, the slot is non-zero.
	unsigned char selected[22]; // 2 for selected, 1 for unselected, 0 for disabled (permanently excluded) 

	string wavpath;

	CRect rtChan[22];
	__int8 clickedRect;
	CRect rtSlider[3]; // the rectangle for slider and slider rail
	bool mouseOn; //2
	bool colDir[3]; // false--full Blue fixed. RG adjusted. True--Full Red, BG adjusted
	BYTE rectID; //2.          1, 2, or 3 for corresponding rect, returns 0 otherwise.
	POINT lastpt; //2
	DWORD boxColor[3]; //2
	int eqgains[3]; //current equalizer gain settings

	int leftEdge, secGroupTop ;
	CRect rtStimrate; // rect for Stimulation rate (aka seasons)
	CRect rteq; // rect for equalizer 
	CRect rtFT; // rect for Frequency Table (AKA Personality)
	CRect rtMAX; // rect for MAXIMA (aka Density)
	CRect rtChansel;
	CRect rtCamera;
	CRect rtColorPal;

	DWORD backColor;

	void ToggleSel(__int8 k);

	void RetrieveSettings(int id);
	string EnumSavedSettings(int id, int &res);

	void InitDraw(HDC hdc);
	int CountSelected();
	void ShowSubjInWindowTitle();
	CSize SetControlPosFont(CDC dc, int ID, const char* FONT, int XPOS, int YPOS);
	void DrawStimrate(CDC dc);
	void DrawChansel(CDC dc);
	void DrawFTMAX(CDC dc, CRect rt, int radiobutton0);
	int GetMAPfilenames(CString settingsfile, char *estr);
	int Getsubj(string fname, char *errstr);

	int ReadINI_UpdateScreen(string fname, char *estr);
	int UpdateINI(string fname, char *estr);
	void initTab();
	void OnPaint();
	void OnPaintEqualizer(CDC dc);
	VOID OnDrawPNG(HDC hdc);
	void OnMouseMove(BOOL fDoubleClick, int x, int y);
	void OnLButtonDown(BOOL fDoubleClick, int x, int y);
	void OnLButtonUp(BOOL fDoubleClick, int x, int y);
	void OnMouseWheel(int xPos, int yPos, int zDelta, UINT fwKeys);

	void EQUpdate();
	void SetPresenter(SETCOMMAND command, string argstr);
	int GetCheckedRadioButton(int ID1, int ID2);

	int CurrentTabPage();
	int MakeString4SetCommand(vector<string> &out);
	void pipecallshow(const char *buf, char *PipeReturnMsg, char *errstr);

	CMapDeckDlg();
	virtual ~CMapDeckDlg();

	void AdjustIDC_SUBJ();

	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	void OnVScroll(HWND hwndCtl, UINT code, int pos);
	void OnTimer(UINT id);
	void OnFlyConnected(char *hostname, char* ipa);
	void OnFlyClosed();
	void OnFlyArrived(WORD command, WORD len, void* inBuffer);
//	HBRUSH OnCtlColorStatic(HDC hdc, HWND hCtrl, int);
	void OnSize(UINT state, int cx, int cy);
	void OnCommand(int idc, HWND hwndCtl, UINT event);
	void OnClose();
	void OnDestroy();
	DWORD ColorFromEqual(char c);
	int letter2gain(char c);
	void SaveMAP(int savedSpot);
//	void OnNotify(int idcc, NMHDR *pnm);
	LRESULT ProcessCustomDraw (NMHDR *lParam);

	//	void OnTimer(HWND hwnd, UINT id);
	map<int, int> maprate;
	int currentRateID;
	int defaultRate;
	string lastopensettingfile;
	vector<string> savedslotname;

};