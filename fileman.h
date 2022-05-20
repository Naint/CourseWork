#pragma once

#include "resource.h"

enum TFInfoType { fUnk, fRoot, fFile, fFolder };

class TFInfo
{
public:
	wchar_t wname[270];
	bool selected;	
	TFInfoType typ;
	HICON hicon;

	TFInfo()
	{
		typ = fUnk;
		selected = false;
		memset(wname,0,sizeof(wname));
		hicon = 0;
	}

	~TFInfo()
	{
		if (hicon != 0)
			DestroyIcon( hicon );
	}
};
