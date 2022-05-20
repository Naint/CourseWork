#include "stdafx.h"
#include "fileman.h"
#include <vector>
#include <ShellAPI.h>
#include "inputbox.h"
#include <CommCtrl.h>
#include <Shlwapi.h>
#include "fileio.h"
#include <WindowsX.h>

#define MAX_LOADSTRING 100

class TPanel;

HINSTANCE hInst;								
TCHAR szTitle[MAX_LOADSTRING];					
TCHAR szWindowClass[MAX_LOADSTRING];			

HWND hWnd;

BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
void resizeWindow();

HPEN hSelPen;
HBRUSH SelBrush;

TPanel *active_panel;
TPanel *l_panel, *r_panel;

bool show_path = false, show_info = false;

TPanel* other_panel()
{
	if (active_panel == l_panel) return r_panel;
	return l_panel;
}

typedef std::vector<TFInfo*> TFInfoList;


int th = 16;

class TPanel
{
public:
	TFInfoList files, csel;
	TCHAR path[1000];

	void Clear ()
	{
		unsigned int i;

		for( i = 0; i < files.size(); i++ )
			delete files.at(i);

		files.clear();		
	}

	int selCount()
	{
		int i, c = 0;

		for( i = 0; i < files.size(); i++ )
			if (files.at(i)->selected)
				c++;

		return c;
	}

	void List()
	{
		Clear ();

		WIN32_FIND_DATA find;
		TFInfo *fi;

		TCHAR buf[10000];

		wsprintf( buf, L"%s\\*.*", path );

		HANDLE h = FindFirstFile( buf, &find );

		if (wcslen(path) > 3)
		{
			fi = new TFInfo();
			fi->typ = fRoot;
			wcscpy( fi->wname, L".." );

			files.push_back( fi );
		}

		while (true)
		{
			if ( (wcscmp( find.cFileName, L"." )!=0) &
				(wcscmp( find.cFileName, L".." )!=0) )
			{
				fi = new TFInfo;

				wcscpy( fi->wname, find.cFileName );

				if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					fi->typ = fFolder;
				else
					fi->typ = fFile;
				
				files.push_back( fi );
			}
				
			if (FindNextFile( h, &find ) == 0) break;
		}

		LoadImages();
	}

	void LoadImages()
	{
		for( int i = 0; i < files.size(); i++ )
		{
			TFInfo *fi = files.at(i);
			SHFILEINFO fileinfo;

			if (fi->typ == fRoot) continue;

			SHGetFileInfo( fi->wname, FILE_ATTRIBUTE_NORMAL, 
				&fileinfo, sizeof( fileinfo ), 
				SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON );

			fi->hicon = fileinfo.hIcon;
		}
	}

	int start; // top line of 
	int cursor;
	RECT rect;

	int lines_on_screen;

	void move_cursor( int delta )
	{
		if (files.size() == 0) return;

		// Move
		cursor += delta;

		if (cursor < 0) cursor = 0;
		if (cursor >= files.size())
			cursor = files.size()-1;
		
		fixStart();
	}

	void fixStart()
	{
		if (cursor < start)
			start = cursor;

		if (cursor > (start + lines_on_screen - 1))
			start = cursor - lines_on_screen + 1;

		// csel.clear();
		// csel.push_back( files[cursor] );
	}

	void Draw( HDC dc )
	{
		int c, y;
		RECT r2;
		TFInfo *fi;

		int rect_h_min = 0;

		FillRect( dc, &rect, (HBRUSH) (COLOR_WINDOW) );

		if (show_path) rect_h_min += 20;
		if (show_info) rect_h_min += 40;

		c = min ( (rect.bottom - rect.top - rect_h_min) / th, files.size() - start );
		lines_on_screen = (rect.bottom - rect.top) / th;
		
		y = rect.top;
		r2.left = rect.left;
		r2.right = rect.right;

		if (show_path) 
		{ // Выводим путь, если надо
			TextOut( dc, rect.left, y, path, wcslen( path ) );
			y += 20;
		}

		for( int i = 0; i < c; i++ )
		{
			fi = files.at(i+start);
			if (fi->selected)
			{
				r2.top = y;
				r2.bottom = y+th;
				
				FillRect( dc, &r2, SelBrush );
			}
			if  ( ((i + start) == cursor) & (this == active_panel) )
			{
				SelectObject( dc, hSelPen );

				if (fi->selected)
					SelectObject( dc, SelBrush );
				else
					SelectObject( dc,  (HBRUSH) (COLOR_WINDOW) );

				Rectangle( dc, rect.left, y, rect.right, y+th-1 );
			}

			SetBkMode( dc, TRANSPARENT );
			TextOut( dc, rect.left + 16, y, fi->wname, wcslen( fi->wname ) );
			if (fi->hicon != 0)
				DrawIconEx( dc, rect.left, y, fi->hicon, 0,0, 0, NULL, DI_NORMAL );
			y += th;
		}

		if (show_info)
		{
			fi = files.at(cursor);

			y = rect.bottom - 40;

			if (fi->typ == fRoot)
				TextOut( dc, rect.left, y, L"Корневой каталог", 16 );
			if (fi->typ == fFolder)
				TextOut( dc, rect.left, y, L"Папка", 5 );
			if (fi->typ == fFile)
			{
				TCHAR buf[1000];

				WIN32_FILE_ATTRIBUTE_DATA att;

				wsprintf( buf, L"%s\\%s", path, fi->wname );
				int s = FileSize( buf );

				GetFileAttributesEx( buf, GetFileExInfoStandard, &att );

				wsprintf( buf, L"Size: %d. Attr: %x", att.nFileSizeLow, att.dwFileAttributes );

				TextOut( dc, rect.left, y, buf, wcslen(buf) );
			}
		}
	}

	TPanel()
	{
		start = 0;
		cursor = 0;
	}

	~TPanel()
	{
		Clear ();
	}
};

void Redraw()
{
	// RedrawWindow( hWnd, NULL, NULL, RDW_INVALIDATE );
	InvalidateRect( hWnd, NULL, TRUE );
}

void InitPanels()
{
	hSelPen = CreatePen( PS_DOT, 1, RGB(0,0,0) );
	SelBrush = CreateSolidBrush( RGB(0,0,255) );

	l_panel = new TPanel ();

	l_panel->rect.left = 10;
	l_panel->rect.top = 40;
	l_panel->rect.right = 300;
	l_panel->rect.bottom = 300;

	GetCurrentDirectory( 1000, l_panel->path );

	l_panel->List();

	r_panel = new TPanel ();

	r_panel->rect.left = 310;
	r_panel->rect.top = l_panel->rect.top;
	r_panel->rect.right = 300;
	r_panel->rect.bottom = 300;

	GetCurrentDirectory( 1000, r_panel->path );

	r_panel->List();

	active_panel = l_panel;
}

HWND comboBox1, comboBox2;

TCHAR *types[] = { L"Unknown",  L"Invalid", L"Сьемный", L"Фиксированный", L"Сетевой", L"CD-ROM", L"RAM disk" };
TCHAR *drives[30];
int drive_c;
TCHAR drive_str[100];

int findDrive( wchar_t c )
{
	for( int i = 0; i < drive_c; i++ )
		if ( (drives[i] != NULL) && (drives[i][0] == c) )
			return i;
	return -1;
}

void ListDrives()
{
	TCHAR buf2[100];
	int i, l;
	UINT t;

	l = GetLogicalDriveStrings( 100, drive_str );

	SendMessage( comboBox1, (UINT) CB_DELETESTRING, NULL, NULL );
	SendMessage( comboBox2, (UINT) CB_DELETESTRING, NULL, NULL );

	ULARGE_INTEGER avail, total, free;
	drive_c = 0;

	for( i = 0; i < l; )	
	{
		drives[ drive_c++ ] = &drive_str[i];

		t = GetDriveType( &drive_str[i] );
		if (t < 0) t = 0;
		if (t > 6) t = 6;

		GetDiskFreeSpaceEx( &drive_str[i], &avail, &total, &free );
	
		float f_free = 1.0f * free.QuadPart / 1e9;
		wsprintf( buf2, L"%s    %s     %d gb free", &drive_str[i], types[t], (int) f_free ); 

		SendMessage( comboBox1, (UINT) CB_ADDSTRING, NULL, (LPARAM) buf2 );
		SendMessage( comboBox2, (UINT) CB_ADDSTRING, NULL, (LPARAM) buf2 );

		while ( (i < l) & (drive_str[i] != 0) ) i++;
		while ( (i < l) & (drive_str[i] == 0) ) i++;
	}
}

void initDiskCombobox()
{
	comboBox1 = CreateWindow( WC_COMBOBOX, TEXT(""), 
		CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
		10, 10, 300, 300, hWnd, NULL, hInst, NULL);

	comboBox2 = CreateWindow( WC_COMBOBOX, TEXT(""), 
		CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
		310, 10, 300, 300, hWnd, NULL, hInst, NULL);

	// SendMessage( comboBox1, (UINT) CB_ADDSTRING, NULL, (LPARAM) L"drop text" );

	ListDrives ();

	SendMessage( comboBox1, (UINT) CB_SETCURSEL, findDrive( l_panel->path[0] ), 0 );
	SendMessage( comboBox2, (UINT) CB_SETCURSEL, findDrive( r_panel->path[0] ), 0 );
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	MSG msg;
	HACCEL hAccelTable;

	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_FILEMAN, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	InitPanels ();

	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FILEMAN));

	initDiskCombobox();

	resizeWindow();
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	delete active_panel;


	return (int) msg.wParam;
}





BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Сохранить дескриптор экземпляра в глобальной переменной

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


void folderUp()
{
	int j;

	if ((j = wcslen( active_panel->path )) > 3)
	{
		j--;
		while ( (j > 3) & active_panel->path[j] != '\\' )
			j--;

		if (j > 3)
			active_panel->path[j] = 0;
		else
			active_panel->path[3] = 0;

	}
}

void CopyItem( TCHAR *root, TCHAR *to_path, TFInfo *fi )
{
	TCHAR from[1000], to[1000];

	if (fi->typ == fFile)
	{
		wsprintf( from, L"%s\\%s", root, fi->wname );
		wsprintf( to, L"%s\\%s", to_path, fi->wname );

		CopyFile( from, to, true );

		return;
	}

	TFInfo fi2;
	TCHAR buf[1000];
	WIN32_FIND_DATA find;

	if (fi->typ != fFolder) return;

	wsprintf( buf, L"%s\\%s", to_path, fi->wname );

	if (!PathFileExists(buf))
		CreateDirectory(buf, NULL);

	wsprintf( from, L"%s\\%s", root, fi->wname );
	wsprintf( to, L"%s\\%s", to_path, fi->wname );

	wsprintf( buf, L"%s\\%s\\*.*", root, fi->wname );
	HANDLE h = FindFirstFile( buf, &find );

	if (h == 0) return;

	while (true)
	{
		if ( (wcscmp( find.cFileName, L"." )!=0) &
			(wcscmp( find.cFileName, L".." )!=0) )
		{
			wcscpy( fi2.wname, find.cFileName );

			if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				fi2.typ = fFolder;
			else
				fi2.typ = fFile;
				
			CopyItem( from, to, &fi2 );
		}

		if (FindNextFile( h, &find ) == 0) break;
	}

	FindClose( h );	
}

void DelItem( TCHAR *root, TFInfo *fi )
{
	TCHAR buf[1000];

	if (fi->typ == fFile)
	{
		wsprintf( buf, L"%s\\%s", root, fi->wname );
		DeleteFile( buf );
		return;
	}

	TFInfo fi2;
	WIN32_FIND_DATA find;

	if (fi->typ != fFolder) return;
	
	wsprintf( buf, L"%s\\%s\\*.*", root, fi->wname );
	HANDLE h = FindFirstFile( buf, &find );

	if (h == 0) return;

	while (true)
	{
		if ( (wcscmp( find.cFileName, L"." )!=0) &
			(wcscmp( find.cFileName, L".." )!=0) )
		{
			wcscpy( fi2.wname, find.cFileName );

			if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				fi2.typ = fFolder;
			else
				fi2.typ = fFile;
				
			wsprintf( buf, L"%s\\%s", root, fi->wname );

			DelItem( buf, &fi2 );
		}

		if (FindNextFile( h, &find ) == 0) break;
	}

	FindClose( h );	

	wsprintf( buf, L"%s\\%s", root, fi->wname );
	RemoveDirectory( buf );
}

void resizeWindow()
{
	RECT r;

	GetWindowRect( hWnd, &r );
	int w = r.right - r.left, 
		h = r.bottom - r.top;

	l_panel->rect.right = (w-20) / 2;
	l_panel->rect.bottom = h - 80;

	r_panel->rect.left = l_panel->rect.right + 10;
	r_panel->rect.right = w - 10;
	r_panel->rect.bottom = h - 80;

	SetWindowPos( comboBox2, 0, r_panel->rect.left, 10, 0,0, SWP_NOSIZE );
}

void createFile( wchar_t *fname )
{
	char buf[1000];

	wcstombs( buf, fname, sizeof(buf) );

	FILE *f;

	fopen_s( &f, buf, "wt" );
	fclose(f);
}

bool in( int x, int y, const RECT *rect )
{
	return (rect->left <= x) & (x <= rect->right) &
		(rect->top <= y) & (y <= rect->bottom);
}

void doAction( TFInfo *fi )
{
	if (fi->typ == fFile)
		ShellExecute( 0, L"open", fi->wname, NULL, NULL, SW_SHOW );
	else if (fi->typ == fRoot)
	{ 
		folderUp();
		active_panel->List ();
	}
	else if (fi->typ = fFolder)
	{
		int j = wcslen( active_panel->path );
		active_panel->path[j] = '\\';
		wcscpy( &active_panel->path[j+1], fi->wname );

		active_panel->List ();
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int i, j;
	TFInfo *fi;
	TCHAR buf[1000], buf2[1000], buf3[1000];
	char buf4[1000];

	switch (message)
	{
	case WM_COMMAND:			
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmEvent)
		{
			case CBN_SELCHANGE:
			{
				int ItemIndex = SendMessage((HWND) lParam, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
				if (ItemIndex >= drive_c) break;

				if ((HWND) lParam == comboBox1)
				{
					wcscpy( l_panel->path, drives[ ItemIndex ] );			
					l_panel->List ();
				}
				else
				{
					wcscpy( r_panel->path, drives[ ItemIndex ] );			
					r_panel->List ();
				}

				Redraw ();

				SetFocus( hWnd );
				break;
			}
		}

		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_PATH:
			show_path = !show_path;
			Redraw ();
			break;
		case IDM_INFO:
			show_info = !show_info;
			Redraw ();
			break;
		case IDM_NEWFILE:
			InputBox( hWnd, L"Введите имя файла", buf );
			createFile( buf );

			ShellExecute( 0, L"open", buf, NULL, NULL, SW_SHOW );

			active_panel->List();
			Redraw();
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		{
			int mx, my;
			mx = GET_X_LPARAM(lParam);
			my = GET_Y_LPARAM(lParam);

			if (in( mx, my, &l_panel->rect))
				active_panel = l_panel;
			else if (in( mx, my, &r_panel->rect ))
				active_panel = r_panel;
			else break;

			int i = active_panel ->start + (my - active_panel ->rect.top) / th;
			if (i >= active_panel ->files.size())
				i = active_panel ->files.size()-1;

			active_panel ->cursor = i;
			if (message == WM_LBUTTONDBLCLK)
				doAction( active_panel->files[ active_panel->cursor ] );

			Redraw();
		}
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
			case VK_INSERT:
				if (active_panel->files.size() > 0)
				{
					i = active_panel->cursor;
						
					if (active_panel->files.at(i)->typ != fRoot)
					{
						active_panel->files.at(i)->selected = !active_panel->files.at(i)->selected;
					}
					else
						break;
				}
			case VK_UP:
				active_panel->move_cursor(-1);
				Redraw ();
				break;			
			case VK_DOWN:
				active_panel->move_cursor(1);

				Redraw ();
				break;		
			case VK_HOME:
				active_panel->cursor = 0;
				active_panel->fixStart();

				Redraw();
				break;
			case VK_END:
				active_panel->cursor = active_panel->files.size()-1;
				active_panel->fixStart();

				Redraw();
				break;

			case VK_TAB:
				if (active_panel == l_panel)
					active_panel = r_panel;
				else
					active_panel = l_panel;

				Redraw ();
				break;			
			case VK_F5:
				j = active_panel->selCount();

				if (j == 0)
				{
					i = active_panel->cursor;
					fi = active_panel->files.at(i);

					CopyItem( active_panel->path, other_panel()->path, fi );
				}
				else
				{
					for( i = 0; i < active_panel->files.size(); i++ )
					{
						fi = active_panel->files.at(i);
						if (!fi->selected) continue;

						CopyItem( active_panel->path, other_panel()->path, fi );
					}
				}

				other_panel()->List ();

				Redraw ();
				break;
			case VK_F2:
			case VK_F6:
				j = active_panel->selCount();
				if (j > 0)
				{
					MessageBox( hWnd, L"Нельзя переименовать больше одного файла", L"Ошибка", 0 );
					break;
				}

				i = active_panel->cursor;
				fi = active_panel->files.at(i);

				if (fi->typ == fRoot) break;

				wsprintf( buf2, L"%s\\%s", active_panel->path, fi->wname );
				if (wParam == VK_F2)
				{
					InputBox( hWnd, L"Введите новое имя", buf );
					wsprintf( buf3, L"%s\\%s", active_panel->path, buf );
				}
				else
					wsprintf( buf3, L"%s\\%s", other_panel()->path, fi->wname );

				i = _wrename( buf2, buf3 );

				active_panel->List();
				if (wParam == VK_F6)
					other_panel()->List();

				Redraw();
				break;
			case VK_F7:
				if (!InputBox( hWnd, L"Введите имя новой папки", buf2 )) break;

				wsprintf( buf, L"%s\\%s", active_panel->path, buf2 );

				CreateDirectory( buf, NULL );

				active_panel->List ();

				break;				
			case VK_DELETE:
			case VK_F8:
				i = active_panel->cursor;
				fi = active_panel->files.at(i);

				if (fi->typ == fRoot) break;
				
				j = active_panel->selCount();

				if (j > 0)
				{
					wsprintf( buf, L"Удалить %d файлов?", j );

					if (MessageBox( hWnd, buf, L"Вопрос", MB_YESNO) != IDYES) break;

					for( i = 0; i < active_panel->files.size(); i++ )
					{
						fi = active_panel->files.at(i);
						if (!fi->selected) continue;

						DelItem( active_panel->path, fi );
					}
				}
				else
				{
					wsprintf( buf, L"Удалить файл %s?", fi->wname );

					if (MessageBox( hWnd, buf, L"Вопрос", MB_YESNO) != IDYES) break;

					DelItem( active_panel->path, fi );
				}

				active_panel->List ();
				break;
			case VK_RETURN:
				i = active_panel->cursor;
				if (i < 0) break;

				doAction( active_panel->files.at(i) );

				Redraw ();

				break;
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		
		l_panel->Draw( hdc );
		r_panel->Draw( hdc );

		EndPaint(hWnd, &ps);
		break;
	case WM_SIZE:
		resizeWindow();		
		break;		
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// о программе
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)
		FALSE;
}
