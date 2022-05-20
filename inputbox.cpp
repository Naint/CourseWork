#include "stdafx.h"
#include "inputbox.h"
#include "Resource.h"

HGLOBAL inputBoxDlg = 0;
TCHAR dlg_ws[100];

LRESULT CALLBACK dlgProc(HWND hDlg, UINT Msg, 
                 WPARAM wParam, LPARAM lParam)
{
  switch (Msg)
  {
  case WM_INITDIALOG:
	  SetDlgItemText( hDlg, IDC_INPUTBOX_PROMPT, (TCHAR*) lParam );
	  return TRUE;
    case WM_CLOSE:
      EndDialog(hDlg, IDCANCEL);
      return TRUE;
	case WM_COMMAND:
		switch( LOWORD(wParam) )
		{
		case IDOK:			
			GetDlgItemText( hDlg, IDC_INPUTBOX_DLG_EDIT1, &dlg_ws[0], 100 );

			EndDialog(hDlg, IDOK);
			return TRUE;
		}
  }
  return 0;
}

HINSTANCE _hInst;

void initInputBox()
{
	HMODULE hMod = GetModuleHandle(0);
	_hInst = hMod;

	HRSRC hrsrc = FindResource( hMod, MAKEINTRESOURCE(IDD_INPUTBOX), RT_DIALOG );
 
	inputBoxDlg =  LoadResource( hMod, hrsrc );
}

bool InputBox( HWND hWnd, TCHAR *caption, TCHAR *buf )
{
	if (inputBoxDlg == 0)
		initInputBox ();

	INT_PTR r = DialogBoxIndirectParam(_hInst, (LPCDLGTEMPLATE) inputBoxDlg, 
		hWnd, (DLGPROC)dlgProc, (LPARAM) caption );

	wcscpy( buf, dlg_ws );

	return (r == IDOK);
}
