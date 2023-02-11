#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include "resource.h"

#pragma pack(push, 1)
struct DLGTEMPLATEEX_PART1 {
	WORD      dlgVer;
	WORD      signature;
	DWORD     helpID;
	DWORD     exStyle;
	DWORD     style;
	WORD      cDlgItems;
	short     x;
	short     y;
	short     cx;
	short     cy;
};
#pragma pack(pop)

BYTE* AdvanceThrough_sz_Or_Ord(BYTE* pData)
{
	WORD* pWArr = (WORD*)pData;
	if (*pWArr == 0x0000)
	{
		pWArr++;
	}
	else if (*pWArr == 0xFFFF)
	{
		pWArr++;
		pWArr++;
	}
	else
	{
		WCHAR z;
		do
		{
			z = *pWArr;
			pWArr++;
		} while (z != 0);
	}
	return (BYTE*)pWArr;
}

BYTE* AdvanceThrough_String(BYTE* pData, LPWSTR pOutStr)
{
	WCHAR* pWStr = (WCHAR*)pData;
	WCHAR z;
	do
	{
		z = *pWStr;
		pWStr++;
	} while (z != 0);
	if (pOutStr)
	{
		int nLn = pWStr - (WCHAR*)pData;
		CopyMemory(pOutStr, pData, nLn * sizeof(WCHAR));
	}
	return (BYTE*)pWStr;
}

HFONT GetFontFromDialogTemplate(LPCTSTR lpszResourceID)
{
	HFONT hFont = NULL;
	HRSRC hResource = FindResource(0, lpszResourceID, RT_DIALOG);
	if (hResource)
	{
		HGLOBAL hDialogTemplate = LoadResource(GetModuleHandle(0), hResource);
		if (hDialogTemplate)
		{
			LPCDLGTEMPLATE lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(hDialogTemplate);
			DWORD dwszDialogTemplate = SizeofResource(GetModuleHandle(0), hResource);
			if (lpDialogTemplate && dwszDialogTemplate)
			{
				LPCDLGTEMPLATE lpDialogTemplateToUse = lpDialogTemplate;
				DLGTEMPLATEEX_PART1* pDTX1 = (DLGTEMPLATEEX_PART1*)lpDialogTemplate;
				if (pDTX1->signature == 0xFFFF && pDTX1->dlgVer == 1)
				{
					//Now get thru variable length elements
					BYTE* pData = (BYTE*)(pDTX1 + 1);

					//sz_Or_Ord menu;
					pData = AdvanceThrough_sz_Or_Ord(pData);

					//sz_Or_Ord windowClass;
					pData = AdvanceThrough_sz_Or_Ord(pData);

					//title
					WCHAR strTitle[1024];
					pData = AdvanceThrough_String(pData, strTitle);

					//Now pointsize of the font
					//This member is present only if the style member specifies DS_SETFONT or DS_SHELLFONT.
					if (pDTX1->style & (DS_SETFONT | DS_SHELLFONT))
					{
						//Font size in pts
						BYTE* pPtr_FontSize = pData;
						WORD ptFontSize = *(WORD*)pData;
						pData += sizeof(WORD);

						//WORD wFontWeight = *(WORD*)pData;
						pData += sizeof(WORD);

						//BYTE italic = *(BYTE*)pData;
						pData += sizeof(BYTE);

						//BYTE charset = *(BYTE*)pData;
						pData += sizeof(BYTE);

						//Font face name
						WCHAR strFontFaceName[LF_FACESIZE];
						//BYTE* pPtr_FontFaceName = pData;
						pData = AdvanceThrough_String(pData, strFontFaceName);

						if (lstrcmpW(strFontFaceName, L"MS Shell Dlg") == 0)
						{
							lstrcpy(strFontFaceName, TEXT("MS Shell Dlg 2"));
						}

						hFont = CreateFontW(-MulDiv(ptFontSize, 96, 72), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, strFontFaceName);
					}
				}
			}
		}
	}
	return hFont;
}

BOOL GetActualDialogBaseUnits(HWND hWnd, SIZE *baseUnit)
{
	RECT rect = { 4, 8, 0, 0 };
	BOOL result = MapDialogRect(hWnd, &rect);
	if (result)
	{
		baseUnit->cx = rect.left;
		baseUnit->cy = rect.top;
	}
	return result;
}

BOOL GetActualDialogBaseUnits2(HWND hWnd, SIZE *baseUnit)
{
	HDC hdc = GetDC(hWnd);
	HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
	TEXTMETRIC tm = { 0 };
	GetTextMetrics(hdc, &tm);
	baseUnit->cy = (int)(tm.tmHeight);
	SIZE size = { 0 };
	GetTextExtentPoint32(hdc, TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"), 52, &size);
	SelectObject(hdc, hOldFont);
	baseUnit->cx = (int)((size.cx / 26 + 1) / 2);
	ReleaseDC(hWnd, hdc);
	return TRUE;
}

BOOL GetActualDialogBaseUnits3(HWND hWnd, SIZE *baseUnit, LPCTSTR lpszResourceID)
{
	HDC hdc = GetDC(hWnd);
	HFONT hFont = (HFONT)GetFontFromDialogTemplate(lpszResourceID);
	HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
	TEXTMETRIC tm = { 0 };
	GetTextMetrics(hdc, &tm);
	baseUnit->cy = (int)(tm.tmHeight);
	SIZE size = { 0 };
	GetTextExtentPoint32(hdc, TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"), 52, &size);
	SelectObject(hdc, hOldFont);
	baseUnit->cx = (int)((size.cx / 26 + 1) / 2);
	ReleaseDC(hWnd, hdc);
	DeleteObject(hFont);
	return TRUE;
}

INT_PTR CALLBACK DialogProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		SIZE size1 = { 0 }, size2 = { 0 }, size3 = { 0 };
		GetActualDialogBaseUnits(hWnd, &size1);
		GetActualDialogBaseUnits2(hWnd, &size2);
		GetActualDialogBaseUnits3(hWnd, &size3, MAKEINTRESOURCE(IDD_DIALOG1));
		TCHAR szText[1024];
		wsprintf(szText, TEXT("width = %d\r\nheight = %d\r\n\r\nwidth = %d\r\nheight = %d\r\n\r\nwidth = %d\r\nheight = %d"),
			size1.cx, size1.cy, size2.cx, size2.cy, size3.cx, size3.cy);
		MessageBox(hWnd, szText, TEXT("確認"), 0);
	}
	return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hWnd, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DialogProc);
	return 0;
}
