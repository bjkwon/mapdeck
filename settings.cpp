 #include "MapDeck.h"

void SetControlPosFont(HWND hDlg, HDC hdc, int ID, HFONT _fnt, int XPOS, int YPOS);

int ValidatePath(char* filewpath, char* errstr)
{
	return 1;
	WIN32_FIND_DATA ls;
	HANDLE res = FindFirstFile(filewpath, &ls);
	if (res==INVALID_HANDLE_VALUE)	{strcpy(errstr,"Invalid path or file name"); return 0;}
	errstr[0]=0;
	return 1;
}

int GetCheckedRadioButton(HWND hDlg, int ID1, int ID2)
{
	for (int k(ID1); k<=ID2; k++) 
		if (SendDlgItemMessage(hDlg, k, BM_GETCHECK, 0, 0) == BST_CHECKED)
			return k-ID1;
	return -1;
}

int OpenSettings(HWND hDlg, const char* fullfname)
{
	char errstr[256];
	int res;
	if (hDDlg.GetMAPfilenames(fullfname, errstr)==0)
	{
		MessageBox(hDlg, errstr, "", 0);
		return 0;
	}
	hDDlg.Getsubj("", errstr);

	SetDlgItemText(hDlg, IDC_500, hDDlg.mapfname[0].c_str());
	SetDlgItemText(hDlg, IDC_900, hDDlg.mapfname[1].c_str());
	SetDlgItemText(hDlg, IDC_1200, hDDlg.mapfname[2].c_str());
	SetDlgItemText(hDlg, IDC_1800, hDDlg.mapfname[3].c_str());
	if (sscanfINI(errstr, fullfname, "DEFAULT RATE", "%d", &hDDlg.defaultRate)>0)
		res = hDDlg.maprate[hDDlg.defaultRate];
	else
		res = 1, hDDlg.defaultRate=900;
	for (int k(0); k<4; k++)
	{
		if (k==res) SendDlgItemMessage(hDlg, IDC_RATE1+k, BM_SETCHECK, BST_CHECKED, 0);
		else		SendDlgItemMessage(hDlg, IDC_RATE1+k, BM_SETCHECK, BST_UNCHECKED, 0);
	}
	SetDlgItemText(hDlg, IDC_SUBJID, hDDlg.subj);
	return 1;
}

int SaveMAPfnames_DefRate(HWND hDlg, const char* fullfname)
{
	char buf[256], errstr[256];
	GetDlgItemText(hDlg, IDC_500, buf, sizeof(buf));
	if (!printfINI (errstr, fullfname, "MAPFILENAME500", "%s", buf)) {MessageBox(hDlg, errstr, "Settings saving error", 0); return 0;}
	GetDlgItemText(hDlg, IDC_900, buf, sizeof(buf));
	if (!printfINI (errstr, fullfname, "MAPFILENAME900", "%s", buf)) {MessageBox(hDlg, errstr, "Settings saving error", 0); return 0;}
	GetDlgItemText(hDlg, IDC_1200, buf, sizeof(buf));
	if (!printfINI (errstr, fullfname, "MAPFILENAME1200", "%s", buf)) {MessageBox(hDlg, errstr, "Settings saving error", 0); return 0;}
	GetDlgItemText(hDlg, IDC_1800, buf, sizeof(buf));
	if (!printfINI (errstr, fullfname, "MAPFILENAME1800", "%s", buf)) {MessageBox(hDlg, errstr, "Settings saving error", 0); return 0;}

	int res = GetCheckedRadioButton(hDlg, IDC_RATE1, IDC_RATE4);
	int rate;
	for (map<int,int>::iterator it = hDDlg.maprate.begin(); it!=hDDlg.maprate.end(); it++)
	{
		if (it->second == res)
			rate = it->first, it = hDDlg.maprate.end(), it--;
	}
	if (!printfINI (errstr, fullfname, "DEFAULT RATE", "%d", rate)) {MessageBox(hDlg, errstr, "Settings saving error", 0); return 0;}

	return 1;
}

int ReadMAPfnames(HWND hDlg, char * errstr)
{
	char mappath[256], buf0[256], buf[256], fullname[256];
	try {
		GetDlgItemText(hDlg, IDC_MAPDIR, mappath, sizeof(mappath));
		for (int k(0); k<4; k++)
		{
			GetDlgItemText(hDlg, IDC_500+k, buf, sizeof(buf));
			FulfillFile(buf0, mappath, buf);
			if (strlen(buf0)>0)
			{
				if(!ValidatePath(buf0, errstr)) throw buf;
				hDDlg.mapfname[k] = buf0;
				if (k==3) 
				{
					if (!hDDlg.Getsubj(hDDlg.mapfname[k], errstr)) throw errstr;
					SetDlgItemText(hDlg, IDC_SUBJID, hDDlg.subj);
				}
			}
		}

		return 1;
	}
	catch (char *estr) {
		strcpy(errstr, estr);
		return 0;
	}
}

BOOL CALLBACK SettingsDlgProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	char errstr[256], buf[256];
	CFileDlg fileDlg;
	int id, res, nMax;
	string name, tp;
	char fname[MAX_PATH], fullfname[MAX_PATH];
	static string lastfile;
	switch (umsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_REMOTEPC, remotePC);
		SetDlgItemText(hDlg, IDC_MAPDIR, hDDlg.mapdir);
		//res = strlen(hDDlg.mapdir);
		//tp =  hDDlg.mapfname[0].substr(res);
		//SetDlgItemText(hDlg, IDC_500, tp.c_str());
		if (hDDlg.mapfname[0].size()>0) SetDlgItemText(hDlg, IDC_500, hDDlg.mapfname[0].substr(strlen(hDDlg.mapdir)).c_str());
		if (hDDlg.mapfname[1].size()>0)  SetDlgItemText(hDlg, IDC_900, hDDlg.mapfname[1].substr(strlen(hDDlg.mapdir)).c_str());
		if (hDDlg.mapfname[2].size()>0)  SetDlgItemText(hDlg, IDC_1200, hDDlg.mapfname[2].substr(strlen(hDDlg.mapdir)).c_str());
		if (hDDlg.mapfname[3].size()>0)  SetDlgItemText(hDlg, IDC_1800, hDDlg.mapfname[3].substr(strlen(hDDlg.mapdir)).c_str());
		SendDlgItemMessage(hDlg, IDC_RATE1+hDDlg.maprate[hDDlg.defaultRate], BM_SETCHECK, BST_CHECKED, 0);
		SetDlgItemText(hDlg, IDC_SUBJID, hDDlg.subj);
		return 1;

	case WM_COMMAND:
		switch((id=LOWORD(wParam)))
		{
		case IDC_500:
		case IDC_900:
		case IDC_1200:
		case IDC_1800:
//			if (HIWORD(wParam)==EN_KILLFOCUS)
//				if (!ReadMAPfnames(hDlg, errstr)) MessageBox(hDlg, errstr, "Invalid MAP file.", 0); 
			break;

		case IDOK:
			GetDlgItemText(hDlg, IDC_REMOTEPC, remotePC, sizeof(remotePC));
			if (strlen(remotePC)==0) 
			{
				MessageBox(hDlg, "Enter PC name for Processor", "MapDeck--Settings", 0);
				return 1;
			}
			if (!ReadMAPfnames(hDlg, errstr)) 
			{
				sprintf(buf, "Error in ReadMAPfnames--%s", errstr);
				MessageBox(hDlg, buf, "MapDeck--Settings", 0); 
				EndDialog(hDlg, -1);
				break;
			}
			sprintf(fullfname, "%s%s.%s.saved.ini", hDDlg.AppPath, hDDlg.AppName, hDDlg.subj);
			SaveMAPfnames_DefRate(hDlg, fullfname);
			hDDlg.OnCommand(IDC_500+hDDlg.maprate[hDDlg.defaultRate], NULL, 0);
			SetDlgItemText(hDDlg.hDlg, IDC_SUBJ, hDDlg.subj);

			hDDlg.Getsubj("", errstr); // why necessary?
			SetDlgItemText(hDDlg.hDlg, IDC_SUBJ, hDDlg.subj); //why necessary?
			hDDlg.lastopensettingfile = fullfname;
			hDDlg.AdjustIDC_SUBJ();
			for (int k(0); k<22; k++) //To show channel selection when the map is first open
				InvalidateRect(hDDlg.hDlg, hDDlg.rtChan[k], 0);


			//Peek the MAP file to get MAX
			res = sscanfINI(errstr, hDDlg.mapfname[hDDlg.maprate[hDDlg.defaultRate]].c_str(), "NUMBER OF MAXIMA", "%d", &nMax);
			if (res>0)
			{
				for (int k(0);k<3;k++) SendDlgItemMessage(hDDlg.hDlg, IDC_MAX8+k, BM_SETCHECK, BST_UNCHECKED, 0);
				switch(nMax)
				{
				case 8:
					SendDlgItemMessage(hDDlg.hDlg, IDC_MAX8, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 10:
					SendDlgItemMessage(hDDlg.hDlg, IDC_MAX8+1, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 12:
					SendDlgItemMessage(hDDlg.hDlg, IDC_MAX8+2, BM_SETCHECK, BST_CHECKED, 0);
					break;
				default:
					SendDlgItemMessage(hDDlg.hDlg, IDC_MAX8, BM_SETCHECK, BST_CHECKED, 0);
				}
			}

			// Open the saved file for this subj ID if it exists
			for (int k(0);k<4; k++)
			{
				name = hDDlg.EnumSavedSettings(k+1, res);
				if (res>0) 
				{
					EnableDlgItem(hDDlg.hDlg, IDC_SAVED1+k, 1);
					SetDlgItemText(hDDlg.hDlg, IDC_SAVED1+k, name.c_str());
					hDDlg.savedslot[k] = 1;
				}
				else						
					EnableDlgItem(hDDlg.hDlg, IDC_SAVED1+k, 0);
			}
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			break;

		case IDC_OPEN:
			fullfname[0]=0;
			fileDlg.InitFileDlg(hDlg, hDDlg.hInst, hDDlg.AppPath);
//			sprintf(fullfname, "%s.%s.waved", hDDlg.AppPath, hDDlg.subj);
			if (fileDlg.FileOpenDlg(fullfname, fname, "settings file (*.SAVED)\0*.saved\0", "saved settings"))
			{
				if (OpenSettings(hDlg, fullfname)) lastfile = fullfname;
			}
			else
			{
				if (GetLastError()!=0) GetLastErrorStr(errstr), MessageBox (hDlg, errstr, "Fileopen dialog box error", 0);
			}
			break;
		}
		return 1;
	}
	return 0;
}

