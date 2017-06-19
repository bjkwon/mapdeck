 #include "MapDeck.h"
// #include "consts.h"

#define DELAY 1010

void OnGetSubj(const char *favedfile, const char *mapfile, const char *subjstr);


int whichRect(CPoint pt, CRect rt1, CRect rt2, CRect rt3)
{
	if (rt1.PtInRect(pt))	return 1;
	else if (rt2.PtInRect(pt))	return 2;
	else if (rt3.PtInRect(pt))	return 3;
	else return 0;
}

BOOL CALLBACK QuickProc(HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SettingsDlgProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);


BOOL CALLBACK RatingDlgProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	switch (umsg)
	{
	case WM_INITDIALOG:
		return 1;

	case WM_COMMAND:
		return 1;
	}
	return 0;
}

BOOL CALLBACK SaveMAPDlgProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	int id;
	char buf[128];
	static char *nameholder;
	switch (umsg)
	{
	case WM_INITDIALOG:
		for (int k(0); k<4; k++)
		{
			sprintf(buf, "(SLOT%d)%s", k+1, hDDlg.savedslotname[k].c_str());
			SendDlgItemMessage(hDlg, IDC_LIST_SAVESLOT, LB_ADDSTRING, NULL, (WPARAM)buf);
		}
		return 1;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			id = SendDlgItemMessage(hDlg, IDC_LIST_SAVESLOT, LB_GETCURSEL, 0, 0);
			if (id!=LB_ERR)
			{
				GetDlgItemText(hDlg, IDC_SAVENAME, buf, sizeof(buf));
				hDDlg.savedslotname[id] = buf;
				EndDialog(hDlg, id);
			}
			else
				MessageBox(hDlg, "Select SLOT (1-4) to save settings", "MapDeck", 0);
			break;

		case IDCANCEL:
			EndDialog(hDlg, -1);
			break;

		case IDC_LIST_SAVESLOT:
			id = SendDlgItemMessage(hDlg, IDC_LIST_SAVESLOT, LB_GETCURSEL, 0, 0);
			if (id>=0)
				SetDlgItemText(hDlg, IDC_SAVENAME, hDDlg.savedslotname[id].c_str());
			break;
		}
		break;

		return 1;
	}
	return 0;
}




BOOL CALLBACK MapDeckDlgProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);

CMapDeckDlg::CMapDeckDlg()
:rectID(0), mouseOn(0), nBands(22), lastTrackPosT(0), lastTrackPosC(0)
{
	savedslotname.push_back("");
	savedslotname.push_back("");
	savedslotname.push_back("");
	savedslotname.push_back("");
	for (int k(0); k<3; k++)
	{
		boxColor[k] = 0xFF000000;
		colDir[k] = 0;
	}
	maprate[500]=0;
	maprate[900]=1;
	maprate[1200]=2;
	maprate[1800]=3;

}

CMapDeckDlg::~CMapDeckDlg()
{
}

string fontname[]={"Verdana", "Elephant", "Book Antiqua", "Berlin Sans FB", "Arial", "Arial", "Arial Black", "Castellar", "Candara", "Lucida Console", "Arial", };
int fontsize[]={15, 18, 12, 20, 15, 12, 20, 16, 14, 16, 10,};

int CMapDeckDlg::GetCheckedRadioButton(int ID1, int ID2)
{
	for (int k(ID1); k<=ID2; k++) 
		if (SendDlgItemMessage(k, BM_GETCHECK, 0, 0) == BST_CHECKED)
			return k-ID1;
	return -1;
}

void CMapDeckDlg::AdjustIDC_SUBJ()
{
	HDC	hdc = GetDC(hDlg);
	char buf[256];
	CSize sz;
	CDC dc(hdc);
	SetCtlFont(IDC_SUBJ, "Elephant18", 0, 0)
	ReleaseDC(hDlg, hdc);
	CRect rt(CPoint(0,0), sz);
	InvalidateRect(rt);
}

BOOL CMapDeckDlg::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	int res ;
	HANDLE hc;
	CWndDlg::OnInitDialog(hwndFocus, lParam);
 	char errstr[256];
	char buf[256];

	_beginthread (pipeThread, 0, NULL);

	//setting up the font to use in tab pages
	LOGFONT      lf;
	HDC	hdc = GetDC(hDlg);
	for (int k(0); k<8; k++)
	{
		strcpy(lf.lfFaceName,fontname[k].c_str());
		efont[fontname[k]+itoa(fontsize[k], buf,10)] = CreateFont((lf.lfHeight=-MulDiv(fontsize[k], GetDeviceCaps(hdc, LOGPIXELSY), 72)), 0,0,0, FW_NORMAL, FALSE, FALSE, 0,
			ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, lf.lfFaceName);
	}
	int sbarWidth[]={350,-1};
	hStatusbar = CreateWindow (STATUSCLASSNAME, "", WS_CHILD|WS_VISIBLE|WS_BORDER|SBS_SIZEGRIP, 0, 0, 0, 0, hDlg, (HMENU)ID_STATUSBAR, hInst, NULL);
	res = SendDlgItemMessage (ID_STATUSBAR, SB_SETPARTS, 2, (LPARAM)sbarWidth);

	hc = LoadImage(hInst,MAKEINTRESOURCE(IDB_CAMERA), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);
	::SendMessage(GetDlgItem(IDC_CAMERA),BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,(LPARAM)hc);
	
	DWORD dw = sizeof(buf);
	GetComputerName(buf, &dw);
	sprintf (iniFile, "%s%s.%s.ini", AppPath, AppName, buf);
	InitDraw(iniFile, hdc);
	ReleaseDC(NULL, hdc);

	if (!ReadINI_UpdateScreen(iniFile, errstr))
	{
		// When last open settings are not available, make the screen arrngement blank except CONTRAST (set A) because that's not part of MAP---everything else will be set as a MAP is retrived.
//		SendDlgItemMessage(IDC_FREQTABLE_A, BM_SETCHECK, BST_CHECKED, 0);
	}
	else
	{
		MessageBox("ReadINI_UpdateScreen error","will exit",0);
		PostQuitMessage(0);
	}
	ShowSubjInWindowTitle();
	return 0;
}

void CMapDeckDlg::ShowSubjInWindowTitle()
{
 	char verStr[64], szbuffer[256];
	GetModuleFileName (GetModuleHandle(NULL), szbuffer, sizeof(szbuffer));
	getVersionString(szbuffer, verStr, sizeof(verStr));
	CString title, helptitle = LoadString(IDS_TITLE1);
	title = AppName;
	title += " ";
	title += verStr;
//	title +=  "     " + CString(subj) ;
	title += "        " + helptitle;
	SetWindowText (title.c_str());
}

void CMapDeckDlg::OnMouseWheel(int xPos, int yPos, int zDelta, UINT fwKeys)
{
	CRect rt, rt_hDlg;
	if (rtSlider[0].Width()>0 && rtSlider[1].Width()>0 && rtSlider[2].Width()>0)
	{
		GetWindowRect(rt_hDlg);
		xPos -= rt_hDlg.left;
		yPos -= (rt_hDlg.top+27);
		int newID = whichRect(CPoint(xPos,yPos),rtSlider[0],rtSlider[1],rtSlider[2]);

		if (newID>0 && newID<4)
		{
			eqgains[newID-1] += zDelta/120;
			eqgains[newID-1] = max(eqgains[newID-1],-2);
			eqgains[newID-1] = min(eqgains[newID-1],2);
			InvalidateRect(&rtSlider[newID-1], 1);
		}
		int ddd=5;
	}

}

void CMapDeckDlg::DrawStimrate(CDC dc)
{
	CBrush hBack;
	char buf[64];
	CSize sz;
	hBack.CreateSolidBrush(backColor);
	dc.SelectObject(hBack);
	dc.Rectangle(rtStimrate);

	int x0 = rtStimrate.Width()/30; 
	int step = rtStimrate.Width()/4; 
	// Not working... why? 11/2/2016
	//CFont fnt(efont["Candara14"]);
	//dc.SelectObject(fnt);
	//dc.TextOut(rtStimrate.left + step, rtStimrate.top + 20, subj, strlen(subj));
	for (int k(0); k<4; k++)
	{
		SetRadioCtlFont(k+IDC_500, "Arial10", -1, -1 )
		::MoveWindow( GetDlgItem(k+IDC_500), rtStimrate.left+x0+step*k, rtStimrate.top + rtStimrate.Height()/3, rtStimrate.Width()/6, sz.cy*2, 1);
	}
//	SetCtlFont(IDC_SUBJ, "Elephant18", rtStimrate.left, rtStimrate.top + rtStimrate.Height()/30)
	SetCtlFont(IDC_SUBJ, "Elephant18", 0, 0)
}

void CMapDeckDlg::OnTimer(UINT id)
{
	int res;
	string name;
	if (id==DELAY)
	{ // This does not work in OnInitDialog()
		KillTimer(id);
		for (int k(0);k<4; k++)
		{
			name = EnumSavedSettings(k+1, res);
			if (res>0)
			{
				EnableDlgItem(hDDlg.hDlg, IDC_SAVED1+k, 1);
				SetDlgItemText(IDC_SAVED1+k, name.c_str());
			}
			else						
				EnableDlgItem(hDDlg.hDlg, IDC_SAVED1+k, 0);
		}
	}
}


void CMapDeckDlg::DrawChansel(CDC dc)
{
	CRect rt(rtChansel);
	int wid(rt.Width()/(nBands+1)); 
	int heit(rt.Height()*2/7);
	CPoint pt1(wid/2, rt.Height()*5/18); // first chanel block, left: half of wid, top: 4 ninth of rect height
	pt1 += CPoint(rtChansel.left, rtChansel.top+rtChansel.Height()/5);
	CPoint pt2(pt1.x + wid, pt1.y+heit);
	rtChan[0].SetRect(pt1,pt2);
	for (int k(1); k<nBands; k++)
	{
		CPoint pt1(rtChan[0].left + k*wid, rtChan[0].top);
		CPoint pt2(rtChan[0].left + (k+1)*wid, rtChan[0].bottom);
		rtChan[k].SetRect(pt1, pt2);
	}
	int x0 = rt.Width()/20; 
	int step = rt.Width()/4; 
	int lefts[]={x0, x0+step*3/4, x0+step*9/4,};
	for (int k(0); k<3; k++)
	{
		char buf[64];
		CSize sz;
		SetRadioCtlFont(k+IDC_CHANSEL_ALL, "Arial15", rt.left+lefts[k], rtChansel.bottom - rtChansel.Height()/4)
	}
}

void CMapDeckDlg::DrawFTMAX(CDC dc, CRect rt, int radiobutton0)
{
	char buf[64];
	CSize sz;
	int x0 = rt.Width()/20; 
	int wid = rt.Width()/4; 
	int gap = (rt.Width() - 2*x0  - 3*wid)/2;
	for (int k(0); k<3; k++)
	{
		SetRadioCtlFont(radiobutton0+k, "Arial15", 0, 0 )
		::MoveWindow( GetDlgItem(radiobutton0+k), rt.left+x0+(wid+gap)*k, rt.top + rt.Height()/4, rt.Width()/4, rt.Height()/2, 1);
	}
}


void CMapDeckDlg::InitDraw(const char* inifile, HDC hdc)
{
	char buf[64];
	CSize sz, tpp;
	CString tp;
	CDC dc(hdc, hDlg);
	CRect rt_hDlg, rct;
	GetClientRect(hDlg, &rt_hDlg);
	int widDlg(rt_hDlg.Width()), heitDlg(rt_hDlg.Height());
	double stimrateH(.169), ftH(.171), maxH(.171), chanselH(.186), cameraH(.122);
	double gap1(0.017), gap2(.042), gap3(.014), gap4(.042), gap5(.034);
	double xgap1(.02), xgap2(.05), xgap22(.02), widstimrate(.96), widGroup(.4), xgap3(.04), wideq(.35), widchansel(.83);
	double eqaltop(gap1+stimrateH+gap2+gap2), equalH(ftH+gap3+maxH-gap2*3/2);

	int stimrateL((int)(xgap1*widDlg)), chanselL((int)(xgap2*widDlg)), ftL((int)(xgap22*widDlg)), eqL((int)((xgap22+widGroup+xgap3)*widDlg));
	int stimrateT((int)(gap1*heitDlg)), ftT((int)((gap1+stimrateH+gap2)*heitDlg)), maxT((int)((gap1+stimrateH+gap2+ftH+gap3)*heitDlg)), chanselT((int)((gap1+stimrateH+gap2+ftH+gap3+maxH+gap4)*heitDlg));
	int istimrateW((int)(widstimrate*widDlg)), istimrateH((int)(stimrateH*heitDlg));
	int iftW((int)(widGroup*widDlg)), iftH((int)(ftH*heitDlg));
	int iEqualizerT((int)(eqaltop*heitDlg));
	int iEqualizerW((int)(wideq*widDlg)), iEqualizerH((int)(equalH*heitDlg));
	int iChanselW((int)(widchansel*widDlg)), iChanselH((int)(chanselH*heitDlg));

	int cameraT = (int)(chanselT + iChanselH + gap4*heitDlg);
	int cameraL = (int)(ftL + xgap2*widDlg);
	int icameraW = (int)(istimrateW - xgap2*widDlg);
	int icameraH = (int)(cameraH*heitDlg);

	rtStimrate = CRect( CPoint(stimrateL, stimrateT), CSize(istimrateW, istimrateH) );
	rtFT = CRect( CPoint(ftL, ftT), CSize(iftW, iftH) );
	rtMAX = CRect( CPoint(ftL, maxT), CSize(iftW, iftH) );
	rtChansel = CRect( CPoint(chanselL, chanselT), CSize(iChanselW, iChanselH) );
	rtCamera = CRect( CPoint(cameraL, cameraT), CSize(icameraW, icameraH) );

	rteq = CRect( CPoint( eqL, iEqualizerT ), CSize( iEqualizerW, iEqualizerH) );
	for (int k(0); k<3; k++)
	{
		rtSlider[k] = rteq;
		rtSlider[k].left = rteq.left + rteq.Width()/3*k;
		rtSlider[k].right = rtSlider[k].left + rteq.Width()/3;
	}
	CRect rt2;
	GetClientRect(GetDlgItem(IDC_PIC_SLIDER1), &rt2);
	int heit2 = rt2.Height(); // dimension of the slider
	for (int k(0); k<3; k++)
	{
		int rr = rtSlider[k].CenterPoint().x;
		tp.LoadString(2*k+IDS_STRING158);
		SetDlgItemText(2*k+IDC_STEXT_EQUALIZER1, tp.c_str());
		tpp = SetControlPosFont(dc, 2*k+IDC_STEXT_EQUALIZER1, "Book Antiqua12", rtSlider[k].CenterPoint().x-25, rteq.bottom+heit2/8);
		tp.LoadString(2*k+1+IDS_STRING158);
		SetDlgItemText(2*k+1+IDC_STEXT_EQUALIZER1, tp.c_str());
		tpp = SetControlPosFont(dc, 2*k+1+IDC_STEXT_EQUALIZER1, "Book Antiqua12", rtSlider[k].CenterPoint().x-10, rteq.bottom+heit2/8+tpp.cy);
	}
	rtColorPal.SetRect(CPoint(rteq.right+(int)(.08*widDlg), rteq.top), CPoint(rteq.right+(int)(.11*widDlg), rteq.bottom));

	SetCtlFont(IDC_STATIC_GROUP1, "Castellar16", 0, 0);
	SetCtlFont(IDC_STATIC_GROUP2, "Castellar16", 0, 0);
	SetCtlFont(IDC_STATIC_GROUP3, "Castellar16", 0, 0);
	::MoveWindow( GetDlgItem(IDC_STATIC_GROUP2), ftL, ftT, iftW, iftH, 0);
	::MoveWindow( GetDlgItem(IDC_STATIC_GROUP3), ftL, maxT, iftW, iftH, 0);
	::MoveWindow( GetDlgItem(IDC_STATIC_GROUP1), ftL, chanselT, iChanselW, iChanselH, 0);

	::GetWindowRect(GetDlgItem(IDC_CAMERA), &rct);
	::MoveWindow( GetDlgItem(IDC_CAMERA), cameraL, cameraT, rct.Width(), rct.Height(), 0);
	//SAVED SETTINGS buttons here
	int savedsetting0 = cameraL + rct.Width()*5/2; 
	for (int k(0); k<4; k++)
	{
		::MoveWindow( GetDlgItem(IDC_SAVED1+k), savedsetting0 + k*(rct.Width()*13/10) , cameraT, rct.Width(), rct.Height(), 0);
		EnableDlgItem(hDlg, IDC_SAVED1+k, 0);
	}
	DrawStimrate(dc);
	DrawChansel(dc);
	DrawFTMAX(dc, rtFT, IDC_FREQTABLE_A);
	DrawFTMAX(dc, rtMAX, IDC_MAX8);

#ifdef _DEBUG
	::MoveWindow( GetDlgItem(IDC_STEXT_VAL1), rtChansel.right+rtChansel.Width()/20, rtChansel.top, 100, 30, 1);
#else
	::ShowWindow(GetDlgItem(IDC_STEXT_VAL1), SW_HIDE);
#endif

	SetControlPosFont(dc, IDC_TONEVOL, "Verdana15", rteq.left, rteq.top-1.5*gap2*heitDlg);
	int wasax = rtChansel.Width();
	int wasay = rtChansel.Height();
	SetControlPosFont(dc, IDC_FREQ, "Arial12", rtChan[5].left, rtChansel.top+wasay/4);

	CPen pen;

	SetControlPosFont(dc, IDC_LOW, "Arial12", rtChan[1].left, rtChansel.top+wasay*3/4);
	SetControlPosFont(dc, IDC_HIGH, "Arial12", rtChan[20].left, rtChansel.top+wasay*3/4);

	int res;

	HWND hSlider1 = ::CreateWindow (TRACKBAR_CLASS, "", WS_CHILD|WS_VISIBLE|TBS_AUTOTICKS|TBS_BOTH|TBS_VERT|TBS_FIXEDLENGTH, 
				rtColorPal.left-40, rtColorPal.top, 40, rtColorPal.Height(), hDlg, (HMENU)(IDC_SBAR_T), hInst, NULL);
	HWND hSlider2 = ::CreateWindow (TRACKBAR_CLASS, "", WS_CHILD|WS_VISIBLE|TBS_AUTOTICKS|TBS_BOTH|TBS_VERT|TBS_FIXEDLENGTH, 
				rtColorPal.right, rtColorPal.top, 40, rtColorPal.Height(), hDlg, (HMENU)(IDC_SBAR_C), hInst, NULL);


	char errStr[256];
	if (sscanfINI (errStr, inifile, "TC_ADJ_STEPS", "%d", &res)<=0) res = 5;
	res = min(max(res,5),10);

	::SendMessage (hSlider1, TBM_SETRANGE, TRUE, MAKELONG(-res, res));
	::SendMessage (hSlider1, TBM_SETPAGESIZE, 0, (LPARAM)1);
	::SendMessage (hSlider1, TBM_SETPOS, 0, -1);
	::SendMessage (hSlider2, TBM_SETRANGE, TRUE, MAKELONG(-res, res));
	::SendMessage (hSlider2, TBM_SETPAGESIZE, 0, (LPARAM)1);
	::SendMessage (hSlider2, TBM_SETPOS, 0, -1);
	res = ::ShowWindow (hSlider1, SW_SHOW);
	res = ::ShowWindow (hSlider2, SW_SHOW);
}

int CMapDeckDlg::CountSelected()
{
	int res(0);
	for (int k(0); k<22; k++)
		res += (int)selected[k];
	return res;
}

void CMapDeckDlg::OnVScroll(HWND hwndCtl, UINT code, int pos)
{
	if (code==SB_ENDSCROLL)		
		return;
	char buf[16];
//	sprintf(buf, "code=%d", code);
//	if (code!=-1)		SendDlgItemMessage (ID_STATUSBAR, SB_SETTEXT, 1, (LPARAM)buf);

	string tc;
	int id = GetDlgCtrlID(hwndCtl);
	if (id==IDC_SBAR_T) tc = "T ";
	else if (id==IDC_SBAR_C)	tc = "C ";

	if (pos==0) sprintf(buf, "0");
	else if (pos>0) sprintf(buf, "-%d", pos);
	else sprintf(buf, "+%d", -pos);
	tc += buf; 
	SetPresenter(TC, tc);

	if (id==IDC_SBAR_C)
	{
		::SendMessage (GetDlgItem(IDC_SBAR_T), TBM_SETPOS, 1, pos);
		OnVScroll(GetDlgItem(IDC_SBAR_T), -1, pos);
	}
}

void CMapDeckDlg::OnFlyClosed()
{
	SendDlgItemMessage (ID_STATUSBAR, SB_SETTEXT, 1, (LPARAM)"off");
}

void CMapDeckDlg::OnFlyConnected(char *hostname, char* ipa)
{
	SendDlgItemMessage (ID_STATUSBAR, SB_SETTEXT, 1, (LPARAM)ipa);
}

void CMapDeckDlg::OnClose()
{
	char errstr[256];
	UpdateINI(iniFile, errstr);
	hWavDeck.OnClose();
	OnDestroy();
}

void CMapDeckDlg::OnDestroy()
{
	DestroyWindow();
	PostQuitMessage(0);
}

int CMapDeckDlg::letter2gain(char c)
{
	switch(c)
	{
	case '2':
		return 2;
	case '1':
		return 1;
	case '0':
		return 0;
	case 'a':
		return -1;
	case 'b':
		return -2;
	default:
		return 0;
	}
}

DWORD CMapDeckDlg::ColorFromEqual(char c)
{
	switch(c)
	{
	case '2':
		return RGB(0xff, 0x3f, 0x3f);
	case '1':
		return RGB(0xff, 0xa1, 0xa1);
	case '0':
		return RGB(0xff, 0xff, 0xff);
	case 'a':
		return RGB(0x9d, 0x9d, 0xff);
	case 'b':
		return RGB(0x2e, 0x2e, 0xff);
	default:
		return 0;
	}
}

string CMapDeckDlg::EnumSavedSettings(int id, int &res)
{
	// Enumerate and check if it is in a valid format.
	char buf[64], errStr[256];
	string strRead, out;
	CString fname;
	vector<string> vec;
	fname.Format("%s%s.%s.saved.ini", AppPath, AppName, subj);
	sprintf(buf, "SAVED%d", id);
	try {
		if	(ReadINI (errStr, fname.c_str(), buf, strRead)<0) throw 1;
		if (str2vect(vec, strRead.c_str(), "\t")<2)	throw 2;
		savedslotname[id-1] = out = vec[0];
		strRead = vec[1];
		if (str2vect(vec, strRead.c_str(), " ")!=4) throw 3;
		if (sscanf(vec[0].c_str(), "%d", &currentRateID)==EOF) throw 4;
		if (currentRateID>3 || currentRateID<0) throw 5;
		if ((char)vec[1][0]<'A' ||(char)vec[1][0]>'C' ) throw 6;
		if ((char)vec[1][1]<'A' ||(char)vec[1][1]>'C' ) throw 7;
		for (int k(0); k<3; k++) 
		{
			char c = (char)vec[2][k];
			if (c!='b' && c!='a' && c!='0' && c!='1' && c!='2')  throw 8+k;
		}
		if (vec[3].size()!=22) throw 11;
		for (int k(0); k<22; k++) 
			if (vec[3][k]!='O' && vec[3][k]!='X') throw 12+k;
		res = 1;
		return out;
	}
	catch(int e) {
		if (e!=1)
		{
			sprintf(buf,"%d",e);
			MessageBox("EnumSavedSettings exception.",buf);
		}
		res = 0;
		return "";
	}




}

void CMapDeckDlg::RetrieveSettings(int id)
{
	char buf[64], errStr[256];
	CString fname;
	string strRead;
	vector<string> vec;
	int res;
	fname.Format("%s%s.%s.saved.ini", AppPath, AppName, subj);
	sprintf(buf, "SAVED%d", id);
	try {
		if (ReadINI (errStr, fname.c_str(), buf, strRead)<0) throw 1;
		str2vect(vec, strRead.c_str(), "\t");
		strRead = vec[1];
		str2vect(vec, strRead.c_str(), " ");
		sscanf(vec[0].c_str(), "%d", &currentRateID);
		for (int k=IDC_MAX8; k<=IDC_MAX12; k++)
		{
			int dd = (char)vec[1][1]-'A';
			if (k==IDC_MAX8+(char)vec[1][1]-'A')	
				SendDlgItemMessage(k, BM_SETCHECK, BST_CHECKED);
			else 
				SendDlgItemMessage(k, BM_SETCHECK, BST_UNCHECKED);
		}
		res = -GetCheckedRadioButton(IDC_MAX8, IDC_MAX12);
		SetPresenter(MAX, itoa(res, buf, 10));
		SendMessage(WM_COMMAND, MAKELONG(IDC_500+currentRateID,BN_CLICKED), 0);
		for (int k=IDC_FREQTABLE_A; k<IDC_MAX8; k++)
		{
			int dd = (char)vec[1][0]-'A';
			if (k==IDC_FREQTABLE_A+(char)vec[1][0]-'A')	
				SendDlgItemMessage(k, BM_SETCHECK, BST_CHECKED);
			else 
				SendDlgItemMessage(k, BM_SETCHECK, BST_UNCHECKED);
		}
		for (int k(0); k<3; k++) 
		{
			eqgains[k] = letter2gain((char)vec[2][k]);
			boxColor[k] = ColorFromEqual((char)vec[2][k]);
			InvalidateRect(&rtSlider[k], 0);
		}
		for (int k(0); k<22; k++) 
		{
			if ((char)vec[3][k]=='O') selected[k] = 2;
			else					selected[k] = 1;	// should this be 0 or 1?
			InvalidateRect(rtChan[k]);
		}
		EQUpdate();
		string argstr="";
		for (int k=0; k<nBands; k++)
		{
			if (selected[k]>1)	argstr += '1';
			else				argstr += '0'; 
		}
		SetPresenter(ONOFF, argstr);
		SetPresenter(FB, "");
	}
	catch(int e) {

	}
}

void CMapDeckDlg::SaveMAP(int spot)
{
	int res1, res2;
	char buf[64], errStr[256];
	CString outstr, shortout(""), chansel("");
	CString fname;
	fname.Format("%s%s.%s.saved.ini", AppPath, AppName, subj);
	res1 = GetCheckedRadioButton(IDC_FREQTABLE_A, IDC_FREQTABLE_C);
	res2 = GetCheckedRadioButton(IDC_MAX8, IDC_MAX12);
	for (int k(0); k<3; k++)
	{
		if (eqgains[k]>=0.)	sprintf(buf, "%d", (int)eqgains[k]);
		else	sprintf(buf, "%c", '`'-(int)eqgains[k]);
		shortout += buf;
	}
	for (int k(0); k<22; k++)
	{
		if (selected[k]==2) chansel += 'O';
		else				chansel += 'X';
	}
	outstr.Format("%s\t%d %c%c %s %s\n",  savedslotname[spot].c_str(), currentRateID, 'A'+res1, 'A'+res2, shortout.c_str(), chansel.c_str());
	sprintf(buf, "SAVED%d", spot+1);
	if (!printfINI (errStr, fname.c_str(), buf, "%s", outstr.c_str())) {MessageBox(errStr); return ;}
}

void CMapDeckDlg::OnCommand(int idc, HWND hwndCtl, UINT ebent) 
{
	static int cumid;
	int k(0), res(1);
	INT_PTR res1;
	char errstr[256], buf[2048]={};
	static char savedmapname[64];
	CRect rct;
	string pipestr, returned, argstr;
	vector<string> str4pipe;
	//FILE *fp=fopen("OnLog","at");
	//fprintf(fp, "OnCommand: idc=%d, ebent=%d\n", idc, ebent);
	//fclose(fp);
	switch(idc)
	{
	case ID_SETTINGS:
		DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGS), hDlg, SettingsDlgProc);
		break;
	case IDC_BUTTON1:
	case IDC_BUTTON2:
	case IDC_BUTTON3:
	case IDC_BUTTON4:
	case IDC_BUTTON5:
		sprintf(buf,"%d",idc-IDC_BUTTON1+1);
		SetDlgItemText(IDC_RATINGS, buf);
		break;

	case IDC_SAVED1:
	case IDC_SAVED2:
	case IDC_SAVED3:
	case IDC_SAVED4:
		RetrieveSettings(idc-IDC_SAVED1+1);
		break;

	case IDC_CAMERA:
		if ((res1 = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SAVEMAP), hDlg, SaveMAPDlgProc, (LPARAM)&savedslotname))>=0)
		{
			SaveMAP(res1);
			SetDlgItemText(IDC_SAVED1+res1, savedslotname[res1].c_str());
			for (int k(0); k<4; k++)
					if (savedslotname[k].size()>0)	EnableDlgItem(hDlg, IDC_SAVED1+k, 1);
					else				EnableDlgItem(hDlg, IDC_SAVED1+k, 0);
		}
		break;

	case IDC_500:
		backColor = RGB(127,106,0);
		break;
	case IDC_900:
		backColor = RGB(63,73,127);
		break;
	case IDC_1200:
		backColor = RGB(87,0,127);
		break;
	case IDC_1800:
		backColor = RGB(127,0,55);
		res = GetCheckedRadioButton(IDC_MAX8, IDC_MAX12);
		if (res>0) {
			MessageBox("For 1800, only MAXIMA A is allowed.", "For your information---");
			SendDlgItemMessage(IDC_MAX8, BM_SETCHECK, BST_CHECKED); 
			SendDlgItemMessage(IDC_MAX8+res, BM_SETCHECK, BST_UNCHECKED); 
			OnCommand(IDC_MAX8, 0, 0);
		}
		break;

	case IDC_CHANSEL_ALL:
		for (int k=0; k<nBands; k++)
			if (selected[k]>0) selected[k]=2;
		break;	
	case IDC_CHANSEL_EVEN:
		for (int k=0; k<nBands; k++)
		{
			if (selected[k]>0)	
			{
				if ((k/2)*2==k) selected[k]=2;
				else			selected[k]=1;
			}
		}
		break;	
	case IDC_CHANSEL_ODD:
		for (int k=0; k<nBands; k++)
		{
			if (selected[k]>0)	
			{
				if ((k/2)*2==k) selected[k]=1;
				else			selected[k]=2;
			}
		}
		break;

	}
	if (idc>=IDC_500 && idc<=IDC_1800)
	{ // When one of the rate buttons are pressed, it only retrieves T and C values, no other paramaters. 2/14/2017
		if (strlen(subj)==0) 
		{ 
			if (Getsubj("", errstr)==0)
				MessageBox("Can't retrive MAP or settings file.", errstr); 
			return;
		}
		
		// When a stimrate button (Stim Rate) is pressed.
		HDC	hdc = GetDC(hDlg);
		CDC dc(hdc, hDlg);
		CSize sz;
		//If it is -1, -1, MoveWindow is not called (i.e., existing position is kept)
		for (int k=IDC_500; k<=IDC_1800; k++)
			if (k==idc)	SetRadioCtlFont(k, "Elephant18", -1, -1)
			else	SetRadioCtlFont(k, "Arial10", -1, -1)
		ReleaseDC(NULL, hdc);

		argstr = "SET OPENTC ";
		argstr += mapfname[idc-IDC_500];
		SetPresenter(RATE, argstr);
		currentRateID = idc-IDC_500;
		InvalidateRect(rtStimrate);
	}
	else if (idc>=IDC_CHANSEL_ALL && idc<=IDC_CHANSEL_ODD)
	{
		argstr="";
		int nSelected(0);
		for (int k=0; k<nBands; k++)
		{
			if (selected[k]>1)	argstr += '1', nSelected++;
			else				argstr += '0'; 
		}
		SetPresenter(ONOFF, argstr);
		SetPresenter(FB, "");
		MessageBox("FBdone","",0);
		// For CIS (even or odd-numbered), numMax should be the same as numSelected
		if (idc==IDC_CHANSEL_EVEN || idc==IDC_CHANSEL_ODD)
		{
			SetPresenter(MAX, itoa(nSelected, buf, 10));
			//And temporarily disable (or hide) the Max buttons
			for (int k=IDC_MAX8; k<=IDC_MAX12; k++)
				::ShowWindow(GetDlgItem(k), SW_HIDE);
		}
		else // for ALL, put back MAX indicated in Max
		{
			res = -GetCheckedRadioButton(IDC_MAX8, IDC_MAX12);
			MessageBox(itoa(res, buf, 10),"",0);
			SetPresenter(MAX, itoa(res, buf, 10));
			//And SHOW back the Density buttons
			for (int k=IDC_MAX8; k<=IDC_MAX12; k++)
				::ShowWindow(GetDlgItem(k), SW_SHOW);
		}

		
		if (eqgains[0]!=0 || eqgains[1]!=0 || eqgains[2]!=0)
			EQUpdate();
		rct.SetRect(CPoint(rtChan[0].left, rtChan[0].top), CPoint(rtChan[nBands-1].right, rtChan[nBands-1].bottom));
		InvalidateRect(rct, 0);
	}
	else if (idc>=IDC_FREQTABLE_A && idc<=IDC_FREQTABLE_C)
		SetPresenter(FB, "");
	else if (idc>=IDC_MAX8 && idc<=IDC_MAX12)
	{
		res = -GetCheckedRadioButton(IDC_MAX8, IDC_MAX12);
		SetPresenter(MAX, itoa(res, buf, 10));
	}

}
void CMapDeckDlg::OnSize(UINT state, int cx, int cy)
{

}

int CMapDeckDlg::Getsubj(string fname, char *estr)
{//Get SUBJECT ID from mapfiles
	char errstr[256];
	vector<string> sorted;
	estr[0]=0;
	static int cumErrors(0);
	string returned, str4pipe = "GET SUBJ";
	try {
		if (fname.size()>0) str4pipe += " ", str4pipe += fname;
		EditPrintf(hWavDeck.hLog, "%s (outgoing) %s\n", remotePC, str4pipe.c_str());
		int res = TransSocket (remotePC, FLYPORT_PRESENTERSERVER, str4pipe.c_str(), PipeReturnMsg, sizeof(PipeReturnMsg));
		if (res<=0)
		{
			cumErrors++;
			if (cumErrors==3) 
			{
				::GetLastErrorStr(WSAGetLastError(), PipeReturnMsg);
				returned = "Error -- ";
				returned += str4pipe;
				MessageBox(PipeReturnMsg, returned.c_str(), 0);
			}
		}
		else 
		{
			if (strncmp(PipeReturnMsg,"SUCCESS", 6)) // if not success, show on the screen; otherwise, do nothing
			{
				returned = "Error -- ";
				returned += str4pipe + "\n";
				MessageBox(PipeReturnMsg, returned.c_str(), 0);
			}
			else // success, if SET OPENTC, get the electrode selection info
			{
				strcpy(subj, PipeReturnMsg+8);
				return 1;
			}
		}
		strcpy(estr, PipeReturnMsg);
		return 0;
	}
	catch (int e) {
		switch(e)
		{
		case 1:
			strcpy(estr, "mapfname empty");
			break;
		case 2:
			sprintf(estr, "Error reading [SUBJECT ID]--%s", errstr);
			break;
		case 3:
			strcpy(estr, "Subject ID not the same across MAP files.");
			break;
		}
		return 0;
	}
}

vector<string> CMapDeckDlg::GetMAPfilenames(CString favedfile, char *estr)
{
	//in: saved ini file
	//out: mapfname, vector of 4.

	char fname[256], errStr[256];
	char cde[32], dir[256], fn[256], et[256];
	string sss, str[4];
	vector<string> out;
	try {
		_splitpath (favedfile.c_str(), cde, dir, fn, et);
		if (cde[0]==0 && dir[0]==0)
			sprintf(fname, "%s%s", AppPath, favedfile.c_str()); 
		else
			strcpy(fname, favedfile.c_str()); 
		if (ReadINI (errStr, fname, "MAPFILENAME500", str[0])<0) throw "MAPFILENAME500";
		if (ReadINI (errStr, fname, "MAPFILENAME900", str[1])<0) throw "MAPFILENAME900";
		if (ReadINI (errStr, fname, "MAPFILENAME1200", str[2])<0) throw "MAPFILENAME1200";
		if (ReadINI (errStr, fname, "MAPFILENAME1800", str[3])<0) throw "MAPFILENAME1800";
		for (int k(0);k<4; k++) 	
		{
			sss = mapdir; sss += str[k]; 
			out.push_back(sss);
		}
	}
	catch (const char* errstr)	{
		sprintf(estr, "Item %s not found in MapDeck.ini", errstr);
		out.clear();
		return out;
	}
	estr[0]=0;
	return out;
}

int CMapDeckDlg::ReadINI_UpdateScreen(string fname, char *estr)
{
try {
	int buttonID, buttonID2;
	int res(0);
	char buf[256], buf2[256], errStr[256];
	buf[0]=0;
	string strRead, str;
	if (ReadINI (errStr, fname.c_str(), "remotePCNAME", strRead)>=0)
		strcpy(remotePC, strRead.c_str());
	else
	{
		INT_PTR ib = InputBox("Enter Processor PC name", "need this info--mapdeck", buf, sizeof(buf));
		if (ib==1 && strlen(buf)>0)
			strcpy(remotePC, buf);
		else
		{
			MessageBox("Program will exit.","need Processor PC name",0);
			return -1;
		}
	}
	int res1;
	size_t res2, ind(1010), res0(999);
	if ((res1=ReadINI (errStr, fname.c_str(), "MAP_DIRECTORY", strRead))>0)
	{
		vector<string> cut, cut2;
		res0 = str2vect(cut, strRead.c_str(), "\r\n");
		for (ind=0; ind<res0; ind++)
		{
			res2 = str2vect(cut2, cut[ind].c_str(), "|");
			if (res2!=2) throw 2;
			CString str1(remotePC), str2(cut2[0].c_str());
			str1.MakeLower(); str2.MakeLower();
			if (str1==str2) ind=res0+10, strcpy(mapdir, cut2[1].c_str());
		}
	}
	if (res1<=0 || ind==res0) // map directory not available for current remotepc 
	{
		buf[0]=0;
		INT_PTR res1 = InputBox("map directory not available in ini file", "Please enter the folder name for MAP files in the remote PC", buf, sizeof(buf));
		if (res1==1 && strlen(buf)>0)
			strcpy(mapdir, buf);
		else
		{
			if (res1!=1 || strlen(buf)==0)
				res1 = InputBox("map directory not available in ini file", "Please enter the folder name for MAP files in the remote PC", buf, sizeof(buf));
		}
		if (strlen(buf)>0) // update mapdec.(pcname).ini
		{
			sprintf(buf2, "%s|%s", remotePC, buf);
			if (ReadINI (errStr, fname.c_str(), "MAP_DIRECTORY", strRead)<0)	strRead="";
			else	strRead += "\n\r";
			strRead += buf2;
			if (!printfINI (errStr, fname.c_str(), "MAP_DIRECTORY", "%s", strRead.c_str())) throw -5;
		}
	}
	if (mapdir[strlen(mapdir)-1]!='\\') strcat(mapdir,"\\");
	if (ReadINI (errStr, fname.c_str(), "LASTSUBJ", strRead)>0)
	{

		CString saved_set_file;
		strcpy(subj, strRead.c_str());
		saved_set_file.Format("%s%s.%s.saved.ini", AppPath, AppName, subj);
		vector<string> out;
		out = GetMAPfilenames(saved_set_file, errStr);
		for (int k=0; k<4; k++) mapfname[k] = out[k];
		if (out.size()>0)
		{
			SetDlgItemText(IDC_SUBJ, subj);
			lastopensettingfile = strRead;
			int rate;
			// This is where SET OPENTC is first sent, so let's send SET INIT_MAPDECK here
			str = AppPath; str += AppRunName;
			getVersionString(str.c_str(), buf, sizeof(buf));
			sformat(str, "SET INIT_MAPDECK %s", buf);
			DWORD res1 = WaitForSingleObject(mt, INFINITE);
			sendsocketWthread(SETINIT, hDlg, str.c_str());
			//Now SET OPENTC
			if (sscanfINI (errStr, saved_set_file.c_str(), "RATE", "%d", &rate)>0)
				OnCommand(IDC_500+maprate[rate], NULL, 0);
			char c0, c[3];
			if (sscanfINI (errStr, saved_set_file.c_str(), "CONTRAST", "%c", &c0)>0)
			{
				for (int k(0);k<3;k++) SendDlgItemMessage(IDC_FREQTABLE_A+k, BM_SETCHECK, BST_UNCHECKED);
				SendDlgItemMessage(IDC_FREQTABLE_A+c0-'A', BM_SETCHECK, BST_CHECKED);
			}
			if (sscanfINI (errStr, saved_set_file.c_str(), "DENSITY", "%c", &c0)>0)
			{
				for (int k(0);k<3;k++) SendDlgItemMessage(IDC_MAX8+k, BM_SETCHECK, BST_UNCHECKED);
				SendDlgItemMessage(IDC_MAX8+c0-'A', BM_SETCHECK, BST_CHECKED);
			}
			if (sscanfINI (errStr, saved_set_file.c_str(), "EQUALIZER", "%c %c %c", &c[0], &c[1], &c[2])==3)
			{
				//turn the equalier screen according to eqgains
				for (int k(0); k<3; k++) 
				{
					eqgains[k] = letter2gain(c[k]);
					boxColor[k] = ColorFromEqual(c[k]);
					InvalidateRect(&rtSlider[k], 0);
				}
			
			}
			if (ReadINI (errStr, saved_set_file.c_str(), "CHANNEL SELECTION", strRead)>0)
			{
				//turn the channel selection on the screen according to eqgains
				for (int k(0); k<22; k++) 
				{
					if (strRead[k]=='O') selected[k] = 2;
					else					selected[k] = 1;	// should this be 0 or 1?
					InvalidateRect(rtChan[k]);
				}
				// and SET presenter
				string argstr="";
				for (int k=0; k<nBands; k++)
				{
					if (selected[k]>1)	argstr += '1';
					else				argstr += '0'; 
				}
				SetPresenter(ONOFF, argstr);
			}
			SetPresenter(FB, "");
			buttonID = GetCheckedRadioButton(IDC_CHANSEL_ALL, IDC_CHANSEL_ODD);
			if (buttonID<=0)
			{
				buttonID2 = -GetCheckedRadioButton(IDC_MAX8, IDC_MAX12);
				SetPresenter(MAX, itoa(buttonID2, buf, 10));
			}
			// SET presenter for equalizer and chan selection
			if (eqgains[0]!=0 || eqgains[1]!=0 || eqgains[2]!=0)
			EQUpdate();

			// Show saved buttons if they are available in saved_set_file
			//
			//
		}
		else
			res=0;
	}
	else 
		res=0;
	int tar[4];
	CRect rt;
	if (ReadINI (errStr, fname.c_str(), "MAPDECK POS", strRead)>=0 && str2array (tar, 4, strRead.c_str(), " ")==4)
	{
		rt.left = tar[0];
		rt.top = tar[1];
		rt.right = tar[2] + tar[0];
		rt.bottom = tar[3] + tar[1];
		MoveWindow(rt);
	}
	SetTimer(DELAY, 400, NULL);
	return res;
	}
	catch (int e)
	{
		return e;
	}
}
	
int CMapDeckDlg::UpdateINI(string fname, char *estr)
{
	char buf[64], errStr[256];
	string longstr;
	if (!printfINI (errStr, fname.c_str(), "remotePCNAME", "%s", remotePC)) {strcpy(estr, errStr); return 0;}
	CString posStr;
	CRect rt;
	GetWindowRect(&rt);
	posStr.Format("%d %d %d %d", rt.left, rt.top, rt.Width(), rt.Height());
	if (!printfINI (errStr, fname.c_str(), "MAPDECK POS", "%s", posStr.c_str())) {strcpy(estr, errStr); return 0;}
	if (!printfINI (errStr, fname.c_str(), "LASTSUBJ", "%s", subj)) {strcpy(estr, errStr); return 0;}
	CString savedfname;
	savedfname.Format("%s%s.%s.saved.ini", AppPath, AppName, subj);
	int rate;
	for (map<int,int>::iterator it = maprate.begin(); it!=maprate.end(); it++)
	{
		if (it->second == currentRateID)
			rate = it->first, it = maprate.end(), it--;
	}
	if (!printfINI (errStr, savedfname.c_str(), "RATE", "%d", rate)) {strcpy(estr, errStr); return 0;}
	int cho = GetCheckedRadioButton(IDC_FREQTABLE_A, IDC_FREQTABLE_C);
	if (!printfINI (errStr, savedfname.c_str(), "CONTRAST", "%c", 'A'+cho)) {strcpy(estr, errStr); return 0;}
	cho = GetCheckedRadioButton(IDC_MAX8, IDC_MAX12);
	if (!printfINI (errStr, savedfname.c_str(), "DENSITY", "%c", 'A'+cho)) {strcpy(estr, errStr); return 0;}
	longstr="";
	for (int k(0); k<3; k++)
	{
		if (eqgains[k]>=0.)	sprintf(buf, "%d", (int)eqgains[k]);
		else	sprintf(buf, "%c", '`'-(int)eqgains[k]);
		longstr += buf;
	}	
	if (!printfINI (errStr, savedfname.c_str(), "EQUALIZER", "%s", longstr.c_str())) {strcpy(estr, errStr); return 0;}
	longstr="";
	for (int k(0); k<22; k++) if (selected[k]==2) longstr += 'O'; else longstr += 'X';
	if (!printfINI (errStr, savedfname.c_str(), "CHANNEL SELECTION", "%s", longstr.c_str())) {strcpy(estr, errStr); return 0;}

	int res = ::SendMessage (GetDlgItem(IDC_SBAR_C), TBM_GETRANGEMAX, 0, 0);
	if (!printfINI (errStr, fname.c_str(), "TC_ADJ_STEPS", "%d", res)) {strcpy(estr, errStr); return 0;}
	return 1;
}

CSize CMapDeckDlg::SetControlPosFont(CDC dc, int ID, const char* FONT, int XPOS, int YPOS)
{
	char buf[256];
	CSize sz;
	::SendMessage(GetDlgItem(ID), WM_SETFONT,(WPARAM)efont[FONT],1); 
	GetDlgItemText(ID, buf, sizeof(buf));
	CFont _fnt(efont[FONT]);
	dc.SelectObject(_fnt);
	sz = dc.GetTextExtentPoint32(buf, strlen(buf));
	::MoveWindow( GetDlgItem(ID), XPOS, YPOS, sz.cx, sz.cy, 1);
	return sz;
}

void CMapDeckDlg::OnPaint()
{
	CRect rt_hDlg, rct;
	int res;
	GetClientRect(hDlg, &rt_hDlg);
	PAINTSTRUCT  ps;
	CRect ctrlrct;
	CPen hPen;
	char buf[256];
	HDC hdc = BeginPaint(hDlg, &ps);
	CDC dc(hdc, hDlg);
	int wid; // chan selection rectangle dimension
	CSize sz;

	CBrush hBack;
	hBack.CreateSolidBrush(backColor);
	dc.SelectObject(hBack);
	dc.Rectangle(rtStimrate);

	//Freq Chan Sel
	CBrush hBrSel, hBrSelNot, hBrExc;
	hBrSel.CreateSolidBrush (RGB(100,255,150));
	hBrSelNot.CreateSolidBrush (RGB(200,255,230));
	hBrExc.CreateSolidBrush (RGB(19,55,85));
	hPen.CreatePen(PS_DOT, 1, 0);
	dc.SelectObject(&hPen);
	res = CountSelected();
	for (int k(0); k<nBands; k++)
	{
		if (selected[k]==2) 			dc.SelectObject(hBrSel);
		else if (selected[k]==1) 		dc.SelectObject(hBrSelNot);
		else 							dc.SelectObject(hBrExc);
		dc.Rectangle(rtChan[k]);
	}
	hPen.CreatePen(PS_SOLID, 1, 0);
	dc.SelectObject(hPen);
//	HGDIOBJ tp = GetStockObject( NULL_BRUSH );
//	dc.SelectObject(&tp);
	dc.SelectObject(GetStockObject( NULL_BRUSH ));
	rct.SetRect(CPoint(rtChan[0].left, rtChan[0].top), CPoint(rtChan[nBands-1].right, rtChan[nBands-1].bottom));
	dc.Rectangle(rct);
	CFont fnt(efont["Candara14"]);
	dc.SelectObject(fnt);
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextAlign (TA_CENTER | TA_BOTTOM);
	wid = rtChan[1].left - rtChan[0].left;
	for (int k(0); k<nBands; k++)
		dc.TextOut(rtChan[0].left + (k*wid+(k+1)*wid)/2, rtChan[0].bottom-10, itoa(k+1, buf, 10), 2); 

	OnPaintEqualizer(dc);
	OnDrawPNG(hdc);
	ReleaseDC(hDlg, hdc);
	EndPaint(hDlg, &ps);
}


int col2ypos(DWORD col, int y1, int y2, int objheight)
{ // returns the top position of the object according to the color input
	int res;
	int height = y2-y1;
	double yrange = (height - objheight)/2;
	BYTE r = GetRValue(col);
	BYTE g = GetGValue(col);
	BYTE b = GetBValue(col);

	if (r==255) // upper range
	{
		double prop = (double)b/(double)255;
		res = y1 + (int)(yrange*prop);
	}
	else // b should be 255
	{
		double prop = (double)r/(double)255;
		res = y2 - (int)(yrange*prop) - objheight;
	}
	return res;
}



void CMapDeckDlg::OnPaintEqualizer(CDC dc)
{
	BYTE b0(0), b1(255), r0(255), r1(0);
	HBRUSH hPal;
	int iBegin, iEnd, iPalWidth;
	iBegin = rtColorPal.top;
	iEnd = iBegin+(rtColorPal.bottom-rtColorPal.top)/2;
	iPalWidth = rtColorPal.right-rtColorPal.left;
	for (int k=iBegin; k<=iEnd; k+=1)
	{
		double prop = (double)(k-iBegin) / (double)(iEnd-iBegin);
		CRect rt(CPoint(rtColorPal.left, k), CSize(iPalWidth, 2));
		BYTE col = (BYTE)( prop * (b1-b0));
		hPal = CreateSolidBrush(RGB(255, col, col));
		dc.FillRect(rt, hPal);
		DeleteObject(hPal);
		if (k>(rtColorPal.bottom-rtColorPal.top)/2-10)	col++;
	}
	iBegin = iEnd+1;
	iEnd = rtColorPal.bottom;
	for (int k=iBegin; k<=iEnd; k+=1)
	{
		double prop = (double)(k-iBegin) / (double)(iEnd-iBegin);
		CRect rt(CPoint(rtColorPal.left, k), CSize(iPalWidth, 2));
		BYTE col = 255-(BYTE)( prop * (r0-r1));
		hPal = CreateSolidBrush(RGB(col, col, 255));
		dc.FillRect(rt, hPal);
		DeleteObject(hPal);
	}

	int wid, heit, wid2, heit2;
	CSize sz;
	CRect rtSliderOutline;
	rtSliderOutline = rtSlider[0];	rtSliderOutline |= rtSlider[1]; 	rtSliderOutline |= rtSlider[2]; 
	rtSliderOutline.InflateRect(1,1);
	dc.Rectangle(rtSliderOutline);


	CRect rt, rt2;
	GetClientRect(GetDlgItem(IDC_PIC_SLIDER_RAIL1), &rt);
	wid = rt.Width(), heit = rt.Height(); // dimension of the slider rail
	heit *= 4; heit /= 5;
	GetClientRect(GetDlgItem(IDC_PIC_SLIDER1), &rt2);
	wid2 = rt2.Width(), heit2 = rt2.Height(); // dimension of the slider

	HBRUSH hBr[3];
	for (int k(0); k<3; k++)
	{
		BYTE hi2 = HIBYTE(HIWORD(boxColor[k]));
		if (hi2) boxColor[k] = RGB(255,255,255); // if uninitialized, white
		int r = GetRValue(boxColor[k]);
		int g = GetGValue(boxColor[k]);
		int b = GetBValue(boxColor[k]);
		r = (int)((double)r/128+.5)*127;
		g = (int)((double)g/128+.5)*127;
		b = (int)((double)b/128+.5)*127;
		hBr[k] = CreateSolidBrush(RGB(r,g,b));
		dc.FillRect(rtSlider[k], hBr[k]);

		CRect trt(rtSlider[k]);
		trt.DeflateRect((rtSlider[k].Width()-wid)/2, 0, (rtSlider[k].Width()-wid)/2, 0);// deflating the rect to make the new width wid
		MoveWindow( GetDlgItem(IDC_PIC_SLIDER_RAIL1+k), trt, 1);
		int ypos = col2ypos(boxColor[k], rtSlider[k].top, rtSlider[k].top+rtSlider[k].Height(), rt2.Height());
		::MoveWindow( GetDlgItem(IDC_PIC_SLIDER1+k), rtSlider[k].CenterPoint().x-wid2/2, ypos, rt2.Width(), rt2.Height(), 1);
		::ShowWindow( GetDlgItem(IDC_PIC_SLIDER1+k), SW_SHOW);
	}

}

//Bitmap.FromFile
//https://msdn.microsoft.com/en-us/library/vs/alm/ms536290(v=vs.85).aspx

VOID CMapDeckDlg::OnDrawPNG(HDC hdc)
{
   Graphics graphics(hdc);
//Image image(L"c:\\transparent-arrow-mapdeck.png");
//graphics.DrawImage(&image, 60, rteq.top-10);

Pen pen(Color(255, 0, 0, 0), 1);
Status stat = pen.SetEndCap(LineCapArrowAnchor);
int mid = (rtChansel.top+3*rtChan[6].top)/4 ;
stat = graphics.DrawLine(&pen, rtChan[8].left, mid, rtChan[10].left, mid);
}


void CMapDeckDlg::OnMouseMove(BOOL fDoubleClick, int x, int y)
{
//	FILE *tp=fopen("mousemove","at");
	if (!mouseOn) 		return;

try {

	HDC hdc = GetDC(hDlg);
	CDC dc(hdc, hDlg);

	static bool initialized(0);
	static int lastid, lastgridCal;
	int gridCal;
	int newID = whichRect(CPoint(x,y),rtSlider[0],rtSlider[1],rtSlider[2]);
	if (newID==0) 	{lastpt.x = -1; lastpt.y = -1; throw hdc;}
	if (newID!=rectID) /*reset*/ 
	{rectID=newID; lastpt.x = x; lastpt.y = y; throw hdc;}
	
	int id(rectID-1);
	int diff = y-lastpt.y;
	if (diff==0) throw hdc;
	diff *= 35; 	diff /= 10; // change this line to adjust sensitivity (y movement vs. how much the value changes in 255 scale)
	if (!initialized) // for the very first time, colDir[id] should be set according to its sign (positive/negative)
	{ 
		colDir[id]  = eqgains[id]>0;
		initialized=true;
	}
	int r = GetRValue(boxColor[id]);
	int g = GetGValue(boxColor[id]);
	int b = GetBValue(boxColor[id]);
	int r2(r), b2(b), g2(g);
	int next2b = colDir[id] ? g+diff : g-diff;
	//fprintf(tp, "last y=%d, (x,y)=(%d,%d) diff=%d, next2b=%d, colDir[id]=%d: r=%d,g=%d,b=%d\t\t\t", lastpt.y, x, y, diff, next2b, colDir[id], r,g,b);
	if (diff<0 && next2b>255) // moving up and if G value goes beyond 255
	{
		r = 255, b = g = 255 - (next2b-255);
		colDir[id] = true;
	}
	else if (diff>0 && next2b>255) // moving down and if G value goes beyond 255
	{
		b = 255, r = g = 255 - (next2b-255);
		colDir[id] = false;
	}
	else if (diff<0 && next2b<0) // moving up and if G value goes below 0
		r = 255, b = g = 0;
	else if (diff>0 && next2b<0) // moving down and if G value goes beyond 255
		b = 255, r = g = 0;
	else
	{
		if (colDir[id])
			g += diff, r = 255, b = g;
		else
			g -= diff, b = 255, r = g;
	}
	//fprintf(tp, "r=%d,g=%d,b=%d\n", r,g,b);
//	fclose(tp);
	boxColor[id] = RGB(r,g,b);
	lastpt.x = x; lastpt.y = y;
		
	int r3 = (int)((double)r/128+.5)*127;
	int g3 = (int)((double)g/128+.5)*127;
	int b3 = (int)((double)b/128+.5)*127;

	gridCal=2-g3/127;
	if (b3>=254)	gridCal *= -1;
	eqgains[id] = gridCal;
	SetDlgItemInt(IDC_STEXT_EQ1+id, gridCal, 1);

	::ShowWindow(GetDlgItem(IDC_STEXT_EQ1+id), SW_SHOW);
	SetControlPosFont(dc, IDC_STEXT_EQ1+id, "Arial Black20", rtSlider[id].CenterPoint().x, rtSlider[id].CenterPoint().y);

	InvalidateRect(&rtSlider[id], 0);
	// Redraw only when there's change of color
	if ( lastgridCal != gridCal)
		EQUpdate();
	lastgridCal = gridCal;

#ifdef _DEBUG
	char buf[32];
	sprintf(buf, "%d %d %d", r,g,b);
	SetDlgItemText(IDC_STEXT_VAL1, buf);
	SetControlPosFont(dc, IDC_STEXT_VAL1, "Arial10", rtChansel.right+rtChansel.Width()/40, rtChansel.top);
#endif
	if (!colDir[id]) g -= 255 ;
	else			g = 255-g;

	lastid = id;
}
catch(HDC hdc)
{
	ReleaseDC(hDlg, hdc);
}

}

void CMapDeckDlg::OnLButtonDown(BOOL fDoubleClick, int x, int y)
{
	__int8 k;
	CPoint pt(x,y);
	if (rteq.PtInRect(pt))
	{
		mouseOn=true;
		rectID = whichRect(CPoint(x,y),rtSlider[0],rtSlider[1],rtSlider[2]);
		lastpt.x = x;
		lastpt.y = y;
	}
	else if (rtChansel.PtInRect(pt))
	{
		for (k=0; k<nBands; k++)
		{
			if (selected[k]>0 && rtChan[k].PtInRect(pt))
			{
				clickedRect = k;
				k=nBands+100;
			}
		}
	}
}

void CMapDeckDlg::ToggleSel(__int8 k)
{
	if (selected[k]==2) selected[k]=1;
	else if (selected[k]==1) selected[k]=2;
	InvalidateRect(rtChan[k], 0);
}

void CMapDeckDlg::OnLButtonUp(BOOL fDoubleClick, int x, int y)
{
	__int8 k;
	bool allselected(true);
	string argstr;
	CPoint pt(x,y);

	//This should be outside if (rteq.PtInRect(pt)), or the resetting mouseOn cannot happene outside rteq
	for (k=0; k<3; k++)		::ShowWindow(GetDlgItem(IDC_STEXT_EQ1+k), SW_HIDE);
	rectID=0;
	mouseOn=false;
	lastpt.x = lastpt.y = -1;

	if (rteq.PtInRect(pt))
	{
		//for (k=0; k<3; k++)		::ShowWindow(GetDlgItem(IDC_STEXT_EQ1+k), SW_HIDE);
		//rectID=0;
		//mouseOn=false;
		//lastpt.x = lastpt.y = -1;
	}
	else if (rtChansel.PtInRect(pt))
	{
		for (k=0; k<nBands; k++)
		{
			if (selected[k]>0 && rtChan[k].PtInRect(CPoint(x,y)))
			{
				if (k==clickedRect) // select or unselected
				{	ToggleSel(k); k=nBands+100; }
			}
		}
		if (k==nBands+1) clickedRect=0xff;
		if (k==nBands+1) 
			clickedRect=0xff;
		argstr="";
		for (int k(0); k<nBands; k++)
		{
			if (hDDlg.selected[k]>1)	argstr += '1'; 
			else						argstr += '0', allselected = false; 
		}
		SetPresenter(ONOFF, argstr);
		SetPresenter(FB, "");
		if (eqgains[0]!=0 || eqgains[1]!=0 || eqgains[2]!=0)
			EQUpdate();
		if (allselected) 
			SendDlgItemMessage(IDC_CHANSEL_ALL, BM_SETCHECK, BST_CHECKED, 0);
		else
			SendDlgItemMessage(IDC_CHANSEL_ALL, BM_SETCHECK, BST_UNCHECKED, 0);
	}
}

void CMapDeckDlg::EQUpdate()
{
	string argstr, argstr2, argstr3;
	char buf[256];
	//For now, 3 channels for bass and treble for gain eq adjustment
	for (int k(0), count(0); count<3 && k<22; k++)
	{
		if (selected[k]>1) 
		{
			if (eqgains[0]>=0.)	sprintf(buf, "%d", (int)eqgains[0]);
			else	sprintf(buf, "%c", '`'-(int)eqgains[0]);
			argstr += buf;
			count++;
		}
		else
			argstr += '0';
	}
	for (int k(nBands-1), count(0); count<3 && k>=0; k--)
	{
		if (selected[k]>1) 
		{
			if (eqgains[2]>=0.)	sprintf(buf, "%d", (int)eqgains[2]);
			else	sprintf(buf, "%c", '`'-(int)eqgains[2]);
			argstr2 += buf;
			count++;
		}
		else
			argstr2 += '0';
	}
	for (size_t k=argstr.size()+1; k<=22-argstr2.size(); k++)
	{
		if (eqgains[1]>=0.)	sprintf(buf, "%d", (int)eqgains[1]);
		else	sprintf(buf, "%c", '`'-(int)eqgains[1]);
		argstr += buf;
	}
	for (size_t k(0); k<argstr2.size(); k++)
		argstr3 += argstr2[argstr2.size()-1-k];
	argstr += argstr3;
	SetPresenter(EQ, argstr);
}

void CMapDeckDlg::SetPresenter(SETCOMMAND command, string argstr)
{
	static int cumErrors(0);
	string str4pipe;
	int res;
	char buffer[512];
	switch(command)
	{
	case RATE:
		res = GetCheckedRadioButton(IDC_500, IDC_1800);
		if(argstr.length()==0)
		{ // this doesn't seem to be in use.
			argstr = mapfname[res];
			str4pipe = "SET OPENTC "; 
			str4pipe += argstr;
		}
		else
			str4pipe = argstr;
		break;

	case TC:
		str4pipe = "SET "; str4pipe += argstr;
		break;

	case EQ:
		str4pipe = "SET EQS "; str4pipe += argstr;
		break;

	case ONOFF:
		str4pipe = "SET ONOFF "; str4pipe += argstr;
		break;

	case FB:
		res = GetCheckedRadioButton(IDC_FREQTABLE_A, IDC_FREQTABLE_C);
		argstr = res == 0 ? "0" : ( (res==1) ? "4444" : "2222" ); 
		str4pipe = "SET FBTYPE "; str4pipe += argstr;
		break;

	case MAX:
		sscanf(argstr.c_str(), "%d", &res);
		res = abs(res);
		string argstrCopy(argstr);
		argstr = res == 0 ? "8" : ( (res==1) ? "10" : (res==2) ? "12" : argstrCopy); 
		str4pipe = "SET MAX "; str4pipe += argstr;
		break;
	}
	if (!demoOnly) 
	{
		DWORD res = WaitForSingleObject(mt, INFINITE);
		sendsocketWthread(SETPRESENTER, hDlg, str4pipe.c_str());
	}
}


void CMapDeckDlg::OnSocketComing(bool success, int code, char *msgbox)
{
	if (!success)
		Beep(1000,500);
	size_t res;
	vector<string> sorted;
	char sent[256];
	strcpy(sent, msgbox);
	char *pt, *returned = msgbox + strlen(sent)+1;
	switch(code)
	{
	case SETPRESENTER:
		res = str2vect(sorted, sent, " ");
		if (res>2 && sorted[1]=="OPEN")
		{
			size_t len = strlen("SUCCESS ");
			for (char* pt = returned + len ; pt<returned + len + 22; pt++)
			{
				int k = pt - (returned + len);
				if (*pt=='1')	selected[k]=2;
				else if (*pt=='0')	selected[k]=0;
			}
		}
		break;
	case GETSUBJ:
		res = str2vect(sorted, sent, " ");
		if (res>2 && sorted[1]=="SUBJ")
		{
			vector<string> out;
			size_t res = str2vect(out, sent, "\t");
			size_t len = strlen("SUCCESS ");
			pt = returned + len ;
			OnGetSubj(out[1].c_str(), out[0].c_str(), pt);
		}

		break;
	}
	res = SetEvent(mt);
//	res = ReleaseMutex(mt);
}


