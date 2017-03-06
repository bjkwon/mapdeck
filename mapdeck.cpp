#include "MapDeck.h"
// #include "consts.h"

char pipeNameStr[256];
char PipeReturnMsg[4096];
char remotePC[256];
char remoteIPA_STATUS[256];


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
	int sss(0);
	//if (umsg==WM_HSCROLL)
	//{
	//	FILE *fp=fopen("OnLog","at");
	//	fprintf(fp, "WM_HSCROLL: code=%d, pos=%d\n", (UINT)(LOWORD(wParam)), (int)(short)HIWORD(wParam) );
	//	fclose(fp);
	//}
	//if (umsg==WM_INITDIALOG)
	//{
	//	FILE *fp=fopen("OnLog","at");
	//	fprintf(fp, "TCN_SELCHANGE=0x%x\nTRBN_THUMBPOSCHANGING=0x%x\nNM_RELEASEDCAPTURE=0x%x\nNM_CUSTOMDRAW=0x%x\n", TCN_SELCHANGE, TRBN_THUMBPOSCHANGING, NM_RELEASEDCAPTURE, NM_CUSTOMDRAW );
	//	fclose(fp);
	//}
		
	switch (umsg)
	{
	chHANDLE_DLGMSG (hDlg, WM_INITDIALOG, hDDlg.OnInitDialog);
	chHANDLE_DLGMSG (hDlg, WM_VSCROLL, hDDlg.OnVScroll);
	chHANDLE_DLGMSG (hDlg, WM_TIMER, hDDlg.OnTimer);
//	chHANDLE_DLGMSG (hDlg, WM_CTLCOLORSTATIC, hDDlg.OnCtlColorStatic);
	chHANDLE_DLGMSG (hDlg, WM_COMMAND, hDDlg.OnCommand);
	chHANDLE_DLGMSG (hDlg, WM_SIZE, hDDlg.OnSize);
	chHANDLE_DLGMSG (hDlg, WM_CLOSE, hDDlg.OnClose);
	chHANDLE_DLGMSG (hDlg, WM_PAINT, hDDlg.OnPaint);
	chHANDLE_DLGMSG (hDlg, WM_MOUSEMOVE, hDDlg.OnMouseMove);
	chHANDLE_DLGMSG (hDlg, WM_LBUTTONDOWN, hDDlg.OnLButtonDown);
	chHANDLE_DLGMSG (hDlg, WM_LBUTTONUP, hDDlg.OnLButtonUp);
	chHANDLE_DLGMSG (hDlg, WM_MOUSEWHEEL, hDDlg.OnMouseWheel);

	
//	chHANDLE_DLGMSG (hDlg, WM_KEYDOWN, hDDlg.OnKey);
	//chHANDLE_DLGMSG (hDlg, WM_FL_ARRIVED, hDDlg.OnFlyArrived);
	//chHANDLE_DLGMSG (hDlg, WM_FL_CLOSE, hDDlg.OnFlyClosed);
	//chHANDLE_DLGMSG (hDlg, WM_FL_CONNECTED, hDDlg.OnFlyConnected);

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
//	chHANDLE_DLGMSG (hDlg, WM_TIMER, hWavDeck.OnTimer);
//	chHANDLE_DLGMSG (hDlg, WM_CTLCOLORSTATIC, hWavDeck.OnCtlColorStatic);
	chHANDLE_DLGMSG (hDlg, WM_COMMAND, hWavDeck.OnCommand);
	chHANDLE_DLGMSG (hDlg, WM_SIZE, hWavDeck.OnSize);
	chHANDLE_DLGMSG (hDlg, WM_CLOSE, hWavDeck.OnClose);
//	chHANDLE_DLGMSG (hDlg, WM_KEYDOWN, hWavDeck.OnKey);

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