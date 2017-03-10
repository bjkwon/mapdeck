//#include "WndDlg0.h"
//#include <windows.h>
//#include <commctrl.h>  // includes the common control header
//#include "audfret.h"
//#include "resource.h"
//#include "audstr.h" // for str2array
//#include "msgCrack_dancer.h"
//
#include "mapdeck.h"
#include "audfret.h"
#include "wavdeckDlg.h"

#define ID_STATUSBAR 1000
#define ID_PLAYSINGLE 1340

void SetControlPosFont(HWND hDlg, HDC hdc, int ID, HFONT _fnt, int XPOS, int YPOS)
{
	char buf[256];
	SIZE sz;
	SendMessage(GetDlgItem(hDlg, ID), WM_SETFONT,(WPARAM)_fnt,1); 
	GetDlgItemText(hDlg, ID, buf, sizeof(buf));
	SelectObject(hdc, _fnt);
	GetTextExtentPoint32(hdc, buf, strlen(buf), &sz);
	if (XPOS>=0 && YPOS>=0)	MoveWindow( GetDlgItem(hDlg, ID), XPOS, YPOS, sz.cx, sz.cy, 1);
}

void QuickSocketThread (PVOID var)
{
	string name, returned;
	char str4pipe[256];
	strcpy(str4pipe, (char*)var);
	int res = TransSocket (remotePC, FLYPORT_PRESENTERSERVER, str4pipe, PipeReturnMsg, sizeof(PipeReturnMsg));
	if (res<=0)
	{ // TransSocket error

	}
	else
	{
		if (strncmp(PipeReturnMsg,"SUCCESS", 6)) // if not success, show on the screen; otherwise, do nothing
		{
			returned = "Error -- ";
			returned += strcat(str4pipe, "\n");
			MessageBox(hWavDeck.hDlg, returned.c_str(), PipeReturnMsg, 0);
		}
		else // success
		{

		}
	}

}

BOOL CALLBACK QSubmitDlgProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	static HFONT efont1;
	static int chosen(-1);
	LOGFONT      lf;
	HDC	hdc;
	CFileDlg fileDlg;
	int id;
	static string str4pipe;
	string name, returned;
	static string lastfile;
	switch (umsg)
	{
	case WM_INITDIALOG:
		for (int k(0); k<5; k++)
		{
			HANDLE hc = LoadImage(hWavDeck.hInst,MAKEINTRESOURCE(IDB_BITMAP3+k), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);
			::SendMessage(GetDlgItem(hDlg, IDC_BUTTON1+k),BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,(LPARAM)hc);
		}
		ShowWindow(GetDlgItem(hDlg, IDC_RATINGS), SW_HIDE);
		EnableDlgItem(hDlg, IDC_SUBMIT, 0);
		hdc = GetDC(hDlg);
		efont1 = CreateFont((lf.lfHeight=-MulDiv(15, GetDeviceCaps(hdc, LOGPIXELSY), 72)), 0,0,0, FW_NORMAL, FALSE, FALSE, 0,
				ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Berlin Sans FB");
		SetControlPosFont(hDlg, hdc, IDC_STATIC_INST, efont1, 10, 10);
		SetControlPosFont(hDlg, hdc, IDC_RATINGS, efont1, -1, -1);
		SetControlPosFont(hDlg, hdc, IDC_SUBMIT, efont1, -1, -1);
		ReleaseDC(NULL, hdc);
		return 1;
		
	case WM_COMMAND:
		switch((id=LOWORD(wParam)))
		{
		case IDC_BUTTON1:
		case IDC_BUTTON2:
		case IDC_BUTTON3:
		case IDC_BUTTON4:
		case IDC_BUTTON5:
			ShowWindow(GetDlgItem(hDlg, IDC_RATINGS), SW_SHOW);
			chosen = id-IDC_BUTTON1+1;
			SetDlgItemInt(hDlg, IDC_RATINGS, chosen, 0);
			EnableDlgItem(hDlg, IDC_SUBMIT, 1);
			break;
		case IDC_SUBMIT:
			sformat(str4pipe, 64, "SET RATINGS %d", chosen);
			_beginthread (QuickSocketThread, 0, (void*)str4pipe.c_str());
			DeleteObject (efont1);
			EndDialog(hDlg, 1);
			break;

		case IDCANCEL:
			DeleteObject (efont1);
			EndDialog(hDlg, 0);
			break;
		}
		return 1;
	default:
		return 0;
	}
}


CWavDeckDlg::CWavDeckDlg()
:playID(0), lastBeginID(-1)
{

}

CWavDeckDlg::~CWavDeckDlg()
{

}

BOOL CWavDeckDlg::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	int nSoundSamples;
	char buf[256];
 	char fullmoduleName[256], drive[16], dir[256], ext[8], errstr[256];
	CWndDlg::OnInitDialog(hwndFocus, lParam);

	strcpy(iniFile, (char*)lParam);

	int sbarWidth[]={300,-1};

	hStatusbar = CreateWindow (STATUSCLASSNAME, "", WS_CHILD|WS_VISIBLE|WS_BORDER|SBS_SIZEGRIP,
		0, 0, 0, 0, hDlg, (HMENU)ID_STATUSBAR, hInst, NULL);
	SendDlgItemMessage (ID_STATUSBAR, SB_SETPARTS, 2, (LPARAM)sbarWidth);

	int tar[4];
	CRect rt;
	string strRead;
	if (sscanfINI(errstr, iniFile, "NUMBER OF SOUND SAMPLES FOR PLAY", "%d", &nSoundSamples)<=0)
		nSoundSamples = 5; // DEFAULT...
	SetDlgItemInt(IDC_NUMSAMPLES, nSoundSamples);
	if (ReadINI (errstr, iniFile, "WAVDECK POS", strRead)>=0 && str2array (tar, 4, strRead.c_str(), " ")==4)
	{
		rt.left = tar[0];
		rt.top = tar[1];
		rt.right = tar[2] + tar[0];
		rt.bottom = tar[3] + tar[1];
		MoveWindow(rt);
	}

 	GetModuleFileName(GetModuleHandle(NULL), fullmoduleName, MAX_PATH);
 	_splitpath(fullmoduleName, drive, dir, AppName, ext);
	
	LvItem.mask=LVIF_TEXT;   // Text Style
	LvItem.cchTextMax = 256; // Max size of test
	LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
	hList = GetDlgItem(IDC_LIST);
	hLog = GetDlgItem(IDC_LOG);
	::SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT);
	int width[]={490,20};
	LvCol.cx=width[0];
	LoadString(hInst, IDS_STRING157, buf, sizeof(buf));
	LvCol.pszText=buf;
	::SendMessage(hList, LVM_INSERTCOLUMN, 0,(LPARAM)&LvCol); 
	
	sprintf(wavlistFile, "%swavlist.%s.ini", AppPath, remotePC);
	ReadWavlistUpdateScreen(wavlistFile, errstr);
	SendDlgItemMessage (ID_STATUSBAR, SB_SETTEXT, 1, (LPARAM)"....");
	return 1;
}

void CWavDeckDlg::OnFlyClosed()
{
	SendDlgItemMessage (ID_STATUSBAR, SB_SETTEXT, 1, (LPARAM)"off");
}

void CWavDeckDlg::OnFlyConnected(char *hostname, char* ipa)
{
	SendDlgItemMessage (ID_STATUSBAR, SB_SETTEXT, 0, (LPARAM)hostname);
	SendDlgItemMessage (ID_STATUSBAR, SB_SETTEXT, 1, (LPARAM)ipa);
}

void CWavDeckDlg::OnClose()
{
	char errstr[256];
	CString posStr;
	CRect rt;
	GetWindowRect(&rt);
	posStr.Format("%d %d %d %d", rt.left, rt.top, rt.Width(), rt.Height());
	printfINI (errstr, iniFile, "WAVDECK POS", "%s", posStr.c_str());

	int nSoundSamples = GetDlgItemInt(IDC_NUMSAMPLES);
	printfINI(errstr, iniFile, "NUMBER OF SOUND SAMPLES FOR PLAY", "%d", nSoundSamples);

	UpdateINI(wavlistFile, errstr);
//	OnDestroy();
}

void CWavDeckDlg::OnDestroy()
{
	DestroyWindow();
	PostQuitMessage(0);
}

enum ERRTYPE
{
	ERR_SOCKET=1,
	ERR_SOCKET2,
	ERR_PREPARE,
	ERR_PRESENT,
};

void CWavDeckDlg::OnStreamingDone()
{
	int nLines = ListView_GetItemCount(hList);

	for (int row(lastBeginID); row<=playID; row++)
		ListView_SetItemState (hList, row,  0, 0x000F);
	if (!continuePlay) 
	{
		EnableDlgItem(hDlg, IDC_LIST, 1);
		EnableDlgItem(hDlg, IDC_PLAY3, 1);
		EnableDlgItem(hDlg, IDC_PLAY4, 1);
		ListView_SetItemState (hList, playID,  LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
		return;
	}
	if (++playID>=nLines)	playID = 0;

	int res =SetForegroundWindow (hDlg);
	res = ListView_EnsureVisible(hList, playID, FALSE);

	ListView_SetItemState (hList, playID,  LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
	int numSamples = GetDlgItemInt(IDC_NUMSAMPLES);
	if (playCount < numSamples) 
	{
		PlayLine(playID);
		playCount++;
	}
	else
	{ // when all samples are finished playing
		playCount=1;
		DialogBox(hInst, MAKEINTRESOURCE(IDD_QRATING), hDlg, QSubmitDlgProc);
		EnableDlgItem(hDlg, IDC_LIST, 1);
		EnableDlgItem(hDlg, IDC_PLAY3, 1);
		EnableDlgItem(hDlg, IDC_PLAY4, 1);
	}
}

void CWavDeckDlg::PlayLine(int lineID)
{
	static char wavpath[MAX_PATH];
	char fname[MAX_PATH]={};
	char PipeReturnMsg[2048], buf[2048];
	char cde[32], dir[256], fn[256], et[256], buffer[256];
	ListView_GetItemText(hList, lineID, 0, fname, sizeof(fname));
	if (strlen(fname)==0) return;
	_splitpath (fname, cde, dir, fn, et);
	if (cde[0]==0 && dir[0]==0)
		sprintf(buffer, "%s%s", wavpath, fname), strcpy(fname, buffer);
	else
		sprintf(wavpath, "%s%s", cde, dir);
	sprintf(buf, "PREPARE %s 0", fname);
	PipeReturnMsg[0]=0;
	try {
		ListView_SetSelectionMark (hList, playID-1);
		EditPrintf(hLog, "%s (outgoing) %s\n", remotePC, buf);
		int res = TransSocket (remotePC, FLYPORT_PRESENTERSERVER, buf, PipeReturnMsg, sizeof(PipeReturnMsg));
		if (res<=0)			throw ERR_SOCKET;
		else
		{
			if (strncmp(PipeReturnMsg,"SUCCESS", 6)) // if not success, show on the screen; otherwise, do nothing
				throw ERR_PREPARE;
			EditPrintf(hLog, "(incoming) %s\n", PipeReturnMsg);
			EditPrintf(hLog, "%s (outgoing) PRESENT\n", remotePC);
			res = TransSocket (remotePC, FLYPORT_PRESENTERSERVER, "PRESENT", PipeReturnMsg, sizeof(PipeReturnMsg));
			if (res<=0)			throw ERR_SOCKET2;
			EditPrintf(hLog, "%s (incoming) %s\n", remotePC, PipeReturnMsg);
		}
	}
	catch (ERRTYPE e) {
		EnableDlgItem(hDlg, IDC_LIST, 1);
		EnableDlgItem(hDlg, IDC_PLAY3, 1);
		EnableDlgItem(hDlg, IDC_PLAY4, 1);
		switch(e)
		{
		case ERR_PREPARE:
			EditPrintf(hLog, "(incoming) %s\n", PipeReturnMsg);
			MessageBox(PipeReturnMsg, "Mapdeck");
			break;
		case ERR_SOCKET:
		case ERR_SOCKET2:
		CString emsg, emsg_help ("socketerror(prepare)\n, socketerror(present)\n, prepareerror\n, prsentererr");
		emsg.Format("code=%d", e);
		MessageBox(emsg_help, emsg);
			break;
		}
	}
}

void CWavDeckDlg::OnCommand(int idc, HWND hwndCtl, UINT ebent) 
{
	int k(0), res(1);
	int width;
	RECT wndrt;
	char fullfname[MAX_PATH], fname[MAX_PATH], errstr[MAX_PATH];
	static char buffer[256];
	switch(idc)
	{
	case IDC_OPEN:
		fileDlg.InitFileDlg(hDlg, hInst, "");
		fullfname[0]=fname[0]=0;
		if (fileDlg.FileOpenDlg(fullfname, fname, "Wav file (*.WAV)\0*.wav\0", "wav"))
		{
			LvItem.pszText=fullfname;
			LvItem.iItem=ListView_GetItemCount(hList)+1;
			SendDlgItemMessage(IDC_LIST,LVM_INSERTITEM,0,(LPARAM)&LvItem);
		}
		else
		{
			if (GetLastError()!=0) GetLastErrorStr(errstr), MessageBox (errstr, "File Open dialog box error");
		}
		break;

	case IDC_PLAY3:
	case IDC_PLAY4:
		continuePlay = true;
		strcpy(buffer, "SET RATINGS");
		_beginthread (QuickSocketThread, 0, (void*)buffer);
	case ID_PLAYSINGLE:
		playCount = 1;
		beginID = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
		if (beginID==-1) beginID=0;
		if (idc==IDC_PLAY4)		
		{
			ListView_SetItemState (hList, beginID,  0, 0x000F);
			playID = lastBeginID;
		}
		else if (idc==IDC_PLAY3)			
		{
			lastBeginID = beginID;
			playID = beginID;
		}
		else
			playID = beginID;
		EnableDlgItem(hDlg, IDC_LIST, 0);
		EnableDlgItem(hDlg, IDC_PLAY3, 0);
		EnableDlgItem(hDlg, IDC_PLAY4, 0);
		ListView_SetItemState (hList, playID,  LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
		PlayLine(playID);
		if (idc==ID_PLAYSINGLE)
		{
			EnableDlgItem(hDlg, IDC_LIST, 1);
			EnableDlgItem(hDlg, IDC_PLAY3, 1);
			EnableDlgItem(hDlg, IDC_PLAY4, 1);
		}
		break;
		
	case IDC_VIEWCOMM:
		GetWindowRect(&wndrt);
		width = wndrt.right - wndrt.left;
		if (width > 600 ) width /=2, wndrt.right = wndrt.left + width;
		else			width *=2, wndrt.right = wndrt.left + width;
		MoveWindow(wndrt.left,wndrt.top, width, wndrt.bottom-wndrt.top, 1);
		break;

	case IDC_STOP:
		playCount = 1;
		continuePlay = false;
		//EnableDlgItem(hDlg, IDC_LIST, 1);
		//EnableDlgItem(hDlg, IDC_PLAY3, 1);
		//EnableDlgItem(hDlg, IDC_PLAY4, 1);
		break;
	case IDCANCEL:
		OnDestroy();
		break;
	}
}
/*
void CWavDeckDlg::OnFlyArrived(WORD command, WORD len, void* inBuffer)
{
	string buf;
	int iPos;
	switch (command)
	{
	case FL_PREPARE:
		buf = (char*)inBuffer;
		EditPrintf(hLog, (buf+"\n").c_str());
		if (buf.find("SUCCESS")!=string::npos)
		{
			FER(flySendText (FL_PRESENT, ""));
			EditPrintf(hLog, "PRESENT\n");
		}
		break;
	
	case FL_PRESENT:
		buf = (char*)inBuffer;
		EditPrintf(hLog, (buf+"\n").c_str());
		break;
	case FL_STREAM:
		buf = (char*)inBuffer;
		EditPrintf(hLog, (buf+"\n").c_str());
		if (buf=="STIM:START")
//			ListView_SetSelectionMark (hList, playID-1);
		{
			ListView_SetItemState (hList, playID, LVIS_FOCUSED, 1);
		}
		else if (buf=="STIM:END")
		{
			iPos = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
			ListView_SetItemState (hList, iPos, 0, LVNI_SELECTED);
			playID++;
			if ( (playID-lastBeginID) < 5  && playID<ListView_GetItemCount (hList)-2 )
				OnCommand(IDC_PLAY3, GetDlgItem(IDC_PLAY3), 8767);
			else
			{
				ListView_SetItemState (hList, playID,  LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
			}
		}
		break;
	}
}
*/

void CWavDeckDlg::OnSize(UINT state, int cx, int cy)
{

}


#define WAVLISTTAG "WAV FILES---ONE FILE PER LINE. INCLUDE PATH OR THE LAST PATH IS USED"

int CWavDeckDlg::ReadWavlistUpdateScreen(const char* fname, char *estr)
{
	char buf[256], errStr[256];
	char cde[256], dir[256], fn[256], et[256];
	string filewpath(fname);
	cde[0]=dir[0]=fn[0]=et[0]=0;
	_splitpath (fname, cde, dir, fn, et);
	if (cde[0]==0 && dir[0]==0)
		filewpath = string(AppPath) + fname;
	string strRead;
	int res = ReadINI (errStr, filewpath.c_str(), WAVLISTTAG, strRead);
	if (res>=0)
	{
		vector<string> lines;
		int res2 = str2vect(lines, strRead.c_str(), "\r\n");
		for (size_t k(0); k<lines.size(); k++)
		{
			strcpy(buf, lines[k].c_str());
			LvItem.pszText = buf;
			LvItem.iItem=k;
			SendDlgItemMessage(IDC_LIST,LVM_INSERTITEM,0,(LPARAM)&LvItem);
		}
	}
	else if (res==AUD_ERR_FILE_NOT_FOUND||res==AUD_ERR_HEADING_NOT_FOUND)
	{
		if (!printfINI (errStr, fname, WAVLISTTAG, "")) {strcpy(estr, errStr); return 0;}
	}
	else
	{	strcpy(estr, errStr); return 0; }
	return 1;
}
	
int CWavDeckDlg::UpdateINI(const char* fname, char *estr)
{
	char errStr[256], buf[MAX_PATH];
	char cde[256], dir[256], fn[256], et[256];
	string filewpath(fname);
	_splitpath (fname, cde, dir, fn, et);
	if (cde[0]==0 && dir[0]==0)
		filewpath = string(AppPath) + fname;
	string longstr;
	int k(0);
	strcpy(buf,"xasdf");
	while(buf[0])
	{
		buf[0]=0;
		ListView_GetItemText(hList, k++, 0, buf, sizeof(buf));
		if (strlen(buf)==0)
			break;
		longstr += buf  + string("\r\n");
	}
	if (!printfINI (errStr, filewpath.c_str(), WAVLISTTAG, "%s", longstr.c_str())) {strcpy(estr, errStr); return 0;}
	return 1;
}
void CWavDeckDlg::OnNotify(int idcc, NMHDR *pnm)
{
	int res(0), iSel;
	char buf1[256], buf2[256];
	NMITEMACTIVATE *pnmitem = (NMITEMACTIVATE*)pnm;
	LPNMLVKEYDOWN lvnkeydown = (LPNMLVKEYDOWN)pnm;
	switch(pnm->code)
	{
		case NM_CUSTOMDRAW:
            ::SetWindowLong(hDlg, DWL_MSGRESULT, (LONG)ProcessCustomDraw(pnm));
            return;
		case NM_CLICK:
			//Beep(1000,200);
			break;
		case NM_DBLCLK:
			continuePlay = false;
			OnCommand(ID_PLAYSINGLE, NULL, 0);
			break;
		case LVN_ITEMCHANGED:
			break;
		case LVN_KEYDOWN:
			iSel = ListView_GetSelectionMark(lvnkeydown->hdr.hwndFrom);
			ListView_GetItemText(lvnkeydown->hdr.hwndFrom, iSel, 0, buf1, sizeof(buf1));
			if (strlen(buf1)>0) 
			{
				switch(lvnkeydown->wVKey)
				{
				case VK_SPACE:
					break;
				case VK_DELETE:
					sprintf(buf2, "Delete? %s", buf1);
					if (MessageBox(buf2, "Adjust Play List", MB_YESNO)==IDYES)
						ListView_DeleteItem(lvnkeydown->hdr.hwndFrom, iSel);
					break;
				}
			}

			break;

	}
}

LRESULT CWavDeckDlg::ProcessCustomDraw (NMHDR *lParam)
{
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	switch(lplvcd->nmcd.dwDrawStage) 
	{
	case CDDS_PREPAINT :
		return CDRF_NOTIFYITEMDRAW;
            
	case CDDS_ITEMPREPAINT: //Before an item is drawn
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: //Before a subitem is drawn
		switch(lplvcd->iSubItem)
		{
		case 0:
			lplvcd->clrText   = RGB(255,255,0);
			lplvcd->clrTextBk = RGB(0,0,0);
			return CDRF_NEWFONT;
		}
	}
	return CDRF_DODEFAULT;
}

