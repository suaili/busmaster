#pragma once

#include "afxcmn.h"

class CS19File
{
public:
	CS19File(void);
	CS19File(CString FilePathName);
	~CS19File(void);

private:
	UCHAR BinBlockCount;
	CString FileHeader;
	UINT BinStartAddr[10];
	UINT BinBlockLengh[10];
	UCHAR BinBlockData[10][102400];
	bool ParseFinish;

public:
	void CS19File::ParseFile(CString fileName);

	UCHAR CS19File::GetBlockCount(void);
	CString CS19File::GetFileHeader(void);
	UCHAR* CS19File::GetBlockData(UCHAR BlockIndex);
	UINT CS19File::GetBlockLength(UCHAR BlockIndex);
	UINT CS19File::GetBlockStartAddr(UCHAR BlockIndex);
	bool CS19File::IsParseFinish(void);
};

