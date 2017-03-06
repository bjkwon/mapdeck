#include "wxx_wincore.h" // Win32++ 8.2. This must be placed prior to <windows.h> to avoid including winsock.h

#include <windows.h>
#include <commctrl.h>  // includes the common control header
#include "audfret.h"
#include "resource.h"
#include "audstr.h" // for str2array

#include "MapDeckDlg.h"
#include "WavdeckDlg.h"
#include "msgCrack_dancer.h"

#include <map>
#include <vector>
#include <process.h>

#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

#define PIPE_RETURN_MSG_LEN		MAX_PATH * 16

#define FLYPORT_PRESENTERSERVER		1025
#define FLYPORT_PRESENTER_STATUS_SERVER		1026

#define MAXPENDING		5	

#define MAX_WORDS_PIPEMSG	32
#define ID_STATUSBAR	1000

extern CMapDeckDlg hDDlg;
extern CWavDeckDlg hWavDeck;

extern char pipeNameStr[256];
extern char PipeReturnMsg[4096];
extern char remotePC[256];
extern char remoteIPA_STATUS[256];

void pipeThread (PVOID dummy);

extern CMapDeckDlg hDDlg;

#define WAVILSTFILE	"wavfiles"


//SetCtlFont sets the font and position of the control with correct size
#define SetCtlFont(ID,FONT,XPOS,YPOS){\
	::SendMessage(GetDlgItem(ID), WM_SETFONT,(WPARAM)efont[FONT],1); \
	GetDlgItemText(ID, buf, sizeof(buf));\
	CFont _fnt(efont[FONT]);\
	dc.SelectObject(_fnt);\
	sz = dc.GetTextExtentPoint32(buf, strlen(buf));\
	::MoveWindow( GetDlgItem(ID), XPOS, YPOS, sz.cx, sz.cy, 0);}

//For radio controls, the space for two more characters is needed to claim the space for the radio button
#define SetRadioCtlFont(ID,FONT,XPOS,YPOS) {\
	::SendMessage(GetDlgItem(ID), WM_SETFONT,(WPARAM)efont[FONT],1); \
	GetDlgItemText(ID, buf, sizeof(buf)); strcat(buf,"XX");\
	CFont _fnt(efont[FONT]);\
	dc.SelectObject(_fnt);\
	sz = dc.GetTextExtentPoint32(buf, strlen(buf));\
	if (XPOS>=0 && YPOS>=0)	::MoveWindow( GetDlgItem(ID), XPOS, YPOS, sz.cx, sz.cy, 0);\
}


#define IDD_TAB_QUALITY 100
#define IDD_TAB_EQUALIZER 101
#define IDD_TAB_FREQRANGE 102
