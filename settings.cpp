 #include "MapDeck.h"

HANDLE mtgetsubj;
void SetControlPosFont(HWND hDlg, HDC hdc, int ID, HFONT _fnt, int XPOS, int YPOS);
map<string, string> savedsubjlist;

extern FILE*fp;
#define PRINT(STR) {fp=fopen("record","at"); fprintf(fp,STR); fclose(fp);}

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

int SaveMAPfnames(HWND hDlg, const char* fullfname)
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
	return 1;
}

int ReadMAPfnames(HWND hDlg, char * errstr)
{
	char mappath[256], buf0[256], buf[256];
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

string checksavedfile(const char* mappath, const char* path, const char* savedfname)
{
	int k, res;
	bool remote(false);
	char buf[256], buf2[256], errStr[256];
	string mapfile, subj, subj0, out;
	vector<string> mapheader;
	mapheader.push_back("MAPFILENAME500");
	mapheader.push_back("MAPFILENAME900");
	mapheader.push_back("MAPFILENAME1200");
	mapheader.push_back("MAPFILENAME1800");
	for (k=0; k<4; k++)
	{
		FulfillFile(buf, path, savedfname);
		res = ReadINI (errStr, buf, mapheader[k].c_str(), mapfile);
		FulfillFile(buf2, mappath, mapfile.c_str());
		res = ReadINI (errStr, buf2, "SUBJECT ID", subj);
		sprintf(buf, "GET SUBJ %s", buf2);
		remote = true;
		sendsocketWthread (GETSUBJ, 0, buf);
	}
	if (!remote)
		if (k==4) out = subj;
	return out;
}

void OnGetSubj(const char *favedfile, const char *mapfile, const char *subjstr)
{
	DWORD res;
	static map<string, string> faved_subj;
	static int count(1);
	static string lastname;
	bool faved_exist(false);

	for (map<string,string>::iterator it=faved_subj.begin(); it!=faved_subj.end() && !faved_exist; it++)
		if (it->first==string(favedfile)) 
			faved_exist=true;
	if (!faved_exist) faved_subj[favedfile] = subjstr;
try {
	for (map<string,string>::iterator it=faved_subj.begin(); it!=faved_subj.end(); it++)
	{
		if (it->first==string(favedfile))
			if (it->second!=string(subjstr))
			{ count = 1; throw string("Subj info in .ace files shown in ") + string(favedfile) + " not consistent"; }
	}

	if (count==4 || count==1)
		lastname = favedfile;
	if (count==4)
	{
		savedsubjlist[favedfile]=subjstr;
		count=1;
	}
	else
	{
		assert( (lastname==favedfile));
		count++;
	}
	res = SetEvent(mtgetsubj);
}
catch(string estr)
{
	MessageBox(NULL, estr.c_str(), "", 0);
	res = SetEvent(mtgetsubj);
}
}

BOOL CALLBACK SettingsDlgProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	char errstr[256], inibuf[256], buf[256], buf2[256];
	int k, id, res, nMax, code;
	string name, tp;
	WIN32_FIND_DATA data;
	char fullfname[MAX_PATH];
	DWORD res1;
	HANDLE hFind ;
	static int listcounter(0), timercounter(0);
	static vector<string> savedfiles;
	switch (umsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_REMOTEPC, remotePC);
		SetDlgItemText(hDlg, IDC_MAPDIR, hDDlg.mapdir);

		strcpy(buf, "*saved.ini");
		FulfillFile(buf2, hDDlg.AppPath, buf);
		hFind = FindFirstFile(buf2, &data);
		if (hFind!=INVALID_HANDLE_VALUE )
		{
		vector<string> mapfname;
		do {
		char buf2[256];
		int k(0);
			FulfillFile(inibuf, hDDlg.AppPath, data.cFileName);
			listcounter++;
			mapfname = hDDlg.GetMAPfilenames(inibuf, errstr);
			for (vector<string>::iterator it=mapfname.begin(); it!=mapfname.end(); it++)
			{
				sprintf(buf, "GET SUBJ %s", it->c_str());
				sprintf(buf2, "k=%d,%s..waiting\n",k, buf); PRINT(buf2)
				res1 = WaitForSingleObject(mtgetsubj, INFINITE);
				sprintf(buf2, "k=%d, calling sendsocketWthread\n",k++, buf, it->c_str()); PRINT(buf2)
				strcat(buf, "\t");
				strcat(buf, inibuf);
				sendsocketWthread (GETSUBJ, 0, buf);
			}
		} while (res = FindNextFile(hFind, &data));
		}
		//we don't know when the saved ini list is ready to show in the dlg box, so run the timer and check.
		if (listcounter) SetTimer(hDlg, 1000, 50, NULL);
		break;

	case WM_TIMER:
		if (timercounter++>50)
		{
			KillTimer(hDlg, LOWORD(wParam));
			MessageBox(hDlg, "Connection to Processor PC Lost or Not Established.", "",0);
		}
		else if (listcounter==savedsubjlist.size())
		{
			KillTimer(hDlg, LOWORD(wParam));
			if (listcounter)
			{
				for (map<string,string> ::iterator it=savedsubjlist.begin(); it!=savedsubjlist.end(); it++)
				{
					res = SendDlgItemMessage(hDlg, IDC_SUBJID, CB_ADDSTRING, 0, (LPARAM)it->second.c_str());
					savedfiles.push_back(it->first);
				}
				if ((id=SendDlgItemMessage(hDlg, IDC_SUBJID, CB_FINDSTRING, (WPARAM)-1, (LPARAM)hDDlg.subj))!=-1)
				{
					id = SendDlgItemMessage(hDlg, IDC_SUBJID, CB_SETCURSEL, id, 0);
					SendMessage(hDlg, WM_COMMAND, MAKELONG(IDC_SUBJID, CBN_SELCHANGE), NULL);
				}
			}
		}
		break;

	case WM_COMMAND:
		switch((id=LOWORD(wParam)))
		{
		case IDC_SUBJID:
			if ((code=HIWORD(wParam))==CBN_SELCHANGE)
			{
				id = SendDlgItemMessage(hDlg, IDC_SUBJID, CB_GETCURSEL, 0, 0);
				SendDlgItemMessage(hDlg, IDC_SUBJID, CB_GETLBTEXT, id, (LPARAM)buf);
				SetDlgItemText(hDlg, IDC_SAVED_FILENAME, savedfiles[id].c_str());

				vector<string> mapheader;
				mapheader.push_back("MAPFILENAME500");
				mapheader.push_back("MAPFILENAME900");
				mapheader.push_back("MAPFILENAME1200");
				mapheader.push_back("MAPFILENAME1800");
				vector<int> idc;
				idc.push_back(IDC_500);
				idc.push_back(IDC_900);
				idc.push_back(IDC_1200);
				idc.push_back(IDC_1800);
				string mapfile;
				for (k=0; k<4; k++)
				{
					res = ReadINI (errstr, savedfiles[id].c_str(), mapheader[k].c_str(), mapfile);
					SetDlgItemText(hDlg, idc[k], mapfile.c_str());
				}
			}

			break;

		case IDC_500:
		case IDC_900:
		case IDC_1200:
		case IDC_1800:
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
			SaveMAPfnames(hDlg, fullfname);
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
			listcounter=0;
			savedfiles.clear();
			EndDialog(hDlg, LOWORD(wParam));
			break;
		}
		return 1;
	}
	return 0;
}

