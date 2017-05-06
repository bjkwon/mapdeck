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

CMapDeckDlg hDDlg;
CWavDeckDlg hWavDeck;

class CWinPP : public CWinApp 
{
public:
	CWinPP() {};
	virtual ~CWinPP() {};
};	

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
		hDDlg.OnSocketComing((int)wParam, (char*)lParam);
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

	demoOnly = false;
	
	hInst = hInstance;
	
	hDDlg.hDlg = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAPDECK), NULL, (DLGPROC)MapDeckDlgProc);
	hWavDeck.hDlg = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_WAVDECK), hDDlg.hDlg, (DLGPROC)wavDeckDlgProc, (LPARAM)hDDlg.iniFile);

	HANDLE h = LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, 0);
	SetClassLongPtr (hDDlg.hDlg, GCLP_HICON, (LONG)(LONG_PTR)h);
	HACCEL hAccel = LoadAccelerators (hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	ShowWindow(hDDlg.hDlg, iCmdShow);
	UpdateWindow (hDDlg.hDlg);


//	FILE *fp=fopen("msg.log","at");
//	fprintf(fp, "hDDlg.hDlg=0x%d, hWavDeck.hDlg=0x%d\n", hDDlg.hDlg, hWavDeck.hDlg);


	while (GetMessage (&msg, NULL, 0, 0))
	{ 
		int res(0);
		HWND h = GetParent(msg.hwnd);

	//	fprintf(fp, "msg.hwnd=0x%d, umsg=0x%04x\n", msg.hwnd, msg.message);

	/* This way the F10 key works even when the focus is on WavDeck*/
		if (h==hWavDeck.hDlg || hDDlg.hDlg)
			res = TranslateAccelerator(hDDlg.hDlg, hAccel, &msg);
	//	fprintf(fp, "GetParent(msg.hwnd) = 0x%d, TranslateAccelerator(GetParent(msg.hwnd)..)=%d\n", GetParent(msg.hwnd), res);
		if ( !res && !IsDialogMessage (hDDlg.hDlg, &msg) )
		{
			TranslateMessage (&msg) ;
			DispatchMessage (&msg) ;
		}
	}
//	fclose(fp);
	GdiplusShutdown(gdiplusToken);
	return (int)msg.wParam ;
}

void TransactSocketThread (PVOID var)
{ // This gets called the first time---to attempt connection to Dancer
	string name, returned;
	int res;
	char str4pipe[256], bigbox[8192];
	__int16 code;
	code = *(__int16*)var;
//	HWND hDlg = *(HWND*)((char*)var+sizeof(code));
	int id = *(int*)((char*)var+sizeof(code));
	strcpy(str4pipe, (char*)var+sizeof(code)+sizeof(int));

	if (!mt) 
		mt = CreateMutex(0,1,0);
	// Stays in the loop while TransSocket returns error
	while ((res = TransSocket (remotePC, FLYPORT_PRESENTERSERVER, str4pipe, PipeReturnMsg, sizeof(PipeReturnMsg)))<0 && !demoOnly )
		Sleep(150);
	if (hquick) EndDialog(hquick, 0);
	if (!demoOnly)
		if (code != INITIALIZE && strncmp(PipeReturnMsg,"SUCCESS", 6))
		// if not success, show on the screen
		{
			returned = "Error -- ";
			returned += strcat(str4pipe, "\n");
			MessageBox(hWavDeck.hDlg, returned.c_str(), PipeReturnMsg, 0);
		}
		else
		{
			if (code != INITIALIZE) 
			{
				strcpy(bigbox, str4pipe);
				memcpy(bigbox+strlen(str4pipe)+1, PipeReturnMsg, strlen(PipeReturnMsg)+1);
//				res = PostThreadMessage (id, WM__TRANSOCKET_DONE, (WPARAM)code, (LPARAM)bigbox);
	//			res = SendMessage (hDlg, WM__TRANSOCKET_DONE, (WPARAM)code, (LPARAM)bigbox);
				hDDlg.OnSocketComing(code, bigbox);
			}
		}
	res = ReleaseMutex(mt);
	res = CloseHandle(mt); mt=NULL;
}

void sendsocketWthread(CODE code, HWND hDlg, const char* msg2send)
{
	DWORD res = WaitForSingleObject(mt, INFINITE);
	char buf[4096];
	memcpy(buf, &code, sizeof(code));
	int id = GetCurrentThreadId ();
	memcpy(buf+sizeof(code), &id, sizeof(id));
//	memcpy(buf+sizeof(code), &hDlg, sizeof(hDlg));
//	strcpy(buf+sizeof(code)+sizeof(hDlg), msg2send);
	strcpy(buf+sizeof(code)+sizeof(id), msg2send);
	_beginthread (TransactSocketThread, 0, buf);
}