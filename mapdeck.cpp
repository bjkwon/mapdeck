#include "MapDeck.h"
// #include "consts.h"

char pipeNameStr[256];
char PipeReturnMsg[4096];
char remotePC[256];
char remoteIPA_STATUS[256];

HMODULE hInst;
bool demoOnly;
HANDLE hEvent;
HANDLE mt;
HANDLE mtComing;

CMapDeckDlg hDDlg;
CWavDeckDlg hWavDeck;

FILE*fp;
#define PRINT(STR) {fp=fopen("record","at"); fprintf(fp,STR); fclose(fp);}

class CWinPP : public CWinApp 
{
public:
	CWinPP() {};
	virtual ~CWinPP() {};
};	

BOOL CALLBACK QuickProc(HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	switch (umsg)
	{
	case WM_INITDIALOG:
		hquick = hDlg;
		SetDlgItemText(hDlg, IDC_PCNAME, remotePC);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam)==IDOK)
			EndDialog(hDlg, FALSE);
		if (LOWORD(wParam)==IDCANCEL)
			EndDialog(hDlg, TRUE);
		break;
	case WM_SIZE:
		break;
	//case WM_DESTROY:
	//	hquick = NULL;
	//	break;
	default:
		return FALSE;
	}
	return 1;
}

void pipeThread (PVOID dummy) // This is a status receiving thread
{
	vector<string> parsedMsg;
	bool veryfirst(true);
	int res, len, nArg;
	char errstr[MAX_PATH], inBuf[MAX_PATH*4], inBuf2[MAX_PATH];

	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(FLYPORT_PRESENTER_STATUS_SERVER);
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sa.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind (sock, (SOCKADDR *)&sa, sizeof(sa))==-1)
	{
		GetLastErrorStr(WSAGetLastError(), inBuf2);
		sprintf(errstr, "%s(%d)", inBuf2, WSAGetLastError());
		MessageBox (hDDlg.hDlg, errstr, "socketThread", MB_OK);		
		return;	
	}
	while (1)
	{
		res=listen (sock, MAXPENDING);
		len = sizeof(sa);
		SOCKET acceptedSock = accept(sock, (SOCKADDR *)&sa, &len);
		strcpy(remoteIPA_STATUS, inet_ntoa (sa.sin_addr));
		res = recv(acceptedSock, inBuf, sizeof(inBuf), 0);
		inBuf[res]='\0';
		if (res<=0)
		{
			//do error handling
		}
		EditPrintf (hWavDeck.hLog, "%s (incoming) %s\r\n", remoteIPA_STATUS, inBuf);
		nArg = str2vect (parsedMsg, inBuf, " ");
		res = send(acceptedSock, "OK", 3, 0); // acknowledging 
		if (parsedMsg[0]=="STREAMING:END") 
			hWavDeck.OnStreamingDone();
	}
}

BOOL CALLBACK MapDeckDlgProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	switch (umsg)
	{
	chHANDLE_DLGMSG (hDlg, WM_INITDIALOG, hDDlg.OnInitDialog);
	chHANDLE_DLGMSG (hDlg, WM_VSCROLL, hDDlg.OnVScroll);
	chHANDLE_DLGMSG (hDlg, WM_TIMER, hDDlg.OnTimer);
	chHANDLE_DLGMSG (hDlg, WM_COMMAND, hDDlg.OnCommand);
	chHANDLE_DLGMSG (hDlg, WM_SIZE, hDDlg.OnSize);
	chHANDLE_DLGMSG (hDlg, WM_CLOSE, hDDlg.OnClose);
	chHANDLE_DLGMSG (hDlg, WM_PAINT, hDDlg.OnPaint);
	chHANDLE_DLGMSG (hDlg, WM_MOUSEMOVE, hDDlg.OnMouseMove);
	chHANDLE_DLGMSG (hDlg, WM_LBUTTONDOWN, hDDlg.OnLButtonDown);
	chHANDLE_DLGMSG (hDlg, WM_LBUTTONUP, hDDlg.OnLButtonUp);
	chHANDLE_DLGMSG (hDlg, WM_MOUSEWHEEL, hDDlg.OnMouseWheel);

	case WM__TRANSOCKET_DONE:
		hDDlg.OnSocketComing(0, (int)wParam, (char*)lParam);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK wavDeckDlgProc (HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	switch (umsg)
	{
	chHANDLE_DLGMSG (hDlg, WM_INITDIALOG, hWavDeck.OnInitDialog);
	chHANDLE_DLGMSG (hDlg, WM_COMMAND, hWavDeck.OnCommand);
	chHANDLE_DLGMSG (hDlg, WM_SIZE, hWavDeck.OnSize);
	chHANDLE_DLGMSG (hDlg, WM_CLOSE, hWavDeck.OnClose);

	case WM_NOTIFY: //This cannot be through msg cracker... 
		hWavDeck.OnNotify((int)(wParam), (NMHDR *)lParam);
		break;
	
	default:
		return FALSE;
	}
	return TRUE;
}


int  WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)
{
	CWinPP cbase;
	MSG         msg ;

	// Don't forget to add comctl32.lib
	INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwICC = ICC_TAB_CLASSES;
    InitCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    BOOL bRet = InitCommonControlsEx(&InitCtrls);
	WSADATA wsdat;
	WSAStartup(MAKEWORD(2,0), &wsdat);

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	hInst = hInstance;
	demoOnly = false;
	mt = CreateEvent(NULL, FALSE, 1, NULL);
	mtComing = CreateEvent(NULL, FALSE, 1, NULL);
	mtgetsubj = CreateEvent(NULL, FALSE, 1, NULL);
	
	hDDlg.hDlg = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAPDECK), NULL, (DLGPROC)MapDeckDlgProc);
	hWavDeck.hDlg = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_WAVDECK), hDDlg.hDlg, (DLGPROC)wavDeckDlgProc, (LPARAM)hDDlg.iniFile);

	HANDLE h = LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, 0);
	SetClassLongPtr (hDDlg.hDlg, GCLP_HICON, (LONG)(LONG_PTR)h);
	HACCEL hAccel = LoadAccelerators (hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	ShowWindow(hDDlg.hDlg, iCmdShow);
	UpdateWindow (hDDlg.hDlg);
	while (GetMessage (&msg, NULL, 0, 0))
	{ 
		int res(0);
		HWND h = GetParent(msg.hwnd);
		if (h==hWavDeck.hDlg || hDDlg.hDlg)
			res = TranslateAccelerator(hDDlg.hDlg, hAccel, &msg);
		if ( !res && !IsDialogMessage (hDDlg.hDlg, &msg) )
		{
			TranslateMessage (&msg) ;
			DispatchMessage (&msg) ;
		}
	}
	GdiplusShutdown(gdiplusToken);
	return (int)msg.wParam ;
}

void TransactSocketThread (PVOID var)
{ // This gets called the first time---to attempt connection to Dancer


	string name, returned;
	int res;
	DWORD res1;
	char str4pipe[256], bigbox[8192];
	__int16 code;
	code = *(__int16*)var;
	int id = *(int*)((char*)var+sizeof(code));
	strcpy(str4pipe, (char*)var+sizeof(code)+sizeof(int));

	char buf2[256];
	sprintf(buf2,"(TransactSocketThread) code=%d, msg=%s\n", code, str4pipe);
	PRINT(buf2)

	if (demoOnly)
		res = SetEvent(mt), SetEvent(mtgetsubj);
	else
	{
		while (!demoOnly && (res = TransSocket (remotePC, FLYPORT_PRESENTERSERVER, str4pipe, PipeReturnMsg, sizeof(PipeReturnMsg)))<0 )
			demoOnly = (res = DialogBox(hInst, MAKEINTRESOURCE(IDD_WAITING), NULL, (DLGPROC)QuickProc))==1; 
		strcpy(bigbox, str4pipe);
		memcpy(bigbox+strlen(str4pipe)+1, PipeReturnMsg, strlen(PipeReturnMsg)+1);
		res1 = WaitForSingleObject(mtComing, INFINITE);
		hDDlg.OnSocketComing(strncmp(PipeReturnMsg,"SUCCESS", 6)==0, code, bigbox);
		res1 = SetEvent(mtComing);
	}
}

void sendsocketWthread(CODE code, HWND hDlg, const char* msg2send)
{ 
	/* WaitForSingleObject should be called outside of sendsocketWthread
	   because if it is called here, local variable buf is not protected from a racing condition
	   5/6/2017 bjkwon
	*/
	// hDlg is not necessary, just keep it now in case I want it later. 
	// function signature and  strcpy(str4pipe, .... ) line in TransactSocketThread be updated  
	// 5/6/2017 bjk
	static char buf[4096];
	memcpy(buf, &code, sizeof(code));
	int id = GetCurrentThreadId ();
	memcpy(buf+sizeof(code), &id, sizeof(id));
	strcpy(buf+sizeof(code)+sizeof(id), msg2send);
	char buf2[4096];
	sprintf(buf2,"(sendsocketWthread) code=%d, msg=%s\n", code, msg2send);
	PRINT(buf2)
	_beginthread (TransactSocketThread, 0, buf);
}