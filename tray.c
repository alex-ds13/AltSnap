/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2020                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct _NOTIFYICONDATAA tray;
int tray_added = 0;
int hide = 0;
int UseZones = 0;

char *iconstr[] = {
    "tray_off",
    "tray_on",
    "tray_sus"
};
char *traystr[] = {
    APP_NAMEA" (Off)",
    APP_NAMEA" (On)",
    APP_NAMEA"...",
};

/////////////////////////////////////////////////////////////////////////////
static int InitTray()
{
    ScrollLockState = GetPrivateProfileInt(L"Input", L"ScrollLockState", 0, inipath);

    // Create icondata
    tray.cbSize = sizeof(tray);
    tray.uID = 0;
    tray.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP;
    tray.hWnd = g_hwnd;
    tray.uCallbackMessage = WM_TRAY;
    // Balloon tooltip
    tray.uTimeout = 10000;
    strcpy(tray.szInfoTitle, APP_NAMEA);
    tray.dwInfoFlags = NIIF_USER;

    // Register TaskbarCreated so we can re-add the tray icon if (when) explorer.exe crashes
    WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");
    LOG("Register TaskbarCreated message: %X\n", WM_TASKBARCREATED);

    return 0;
}
/////////////////////////////////////////////////////////////////////////////
static int UpdateTray()
{
    int Index = !!ENABLED();
    if (Index && (ScrollLockState&1))
        Index += !( !(GetKeyState(VK_SCROLL)&1) ^ !(ScrollLockState&2) );

    strcpy(tray.szTip, traystr[Index]);
    tray.hIcon = LoadIconA(g_hinst, iconstr[Index]);

    // Only add or modify if not hidden or if balloon will be displayed
    if (!hide || tray.uFlags&NIF_INFO) {
        // Try a few times, sleep 100 ms between each attempt
        int i=0;
        LOG("Updating tray icon\n");
        while (!Shell_NotifyIconA(tray_added? NIM_MODIFY: NIM_ADD, &tray) ) {
            LOG("Failed in try No. %d\n", i);
            if (i > 3) {
                LOG("Failed all atempts!!\n");
                return 1;
            }
            Sleep(100);
            i++;
        }
        // Success
        tray_added = 1;
    }
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
static int RemoveTray()
{
    if (!tray_added)
        return 1;

    if (!Shell_NotifyIconA(NIM_DELETE, &tray))
        return 1;

    // Success
    tray_added = 0;
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
// Zones functions
static wchar_t *RectToStr(RECT *rc, wchar_t *rectstr)
{
    wchar_t txt[16];
    UCHAR i;
    long *RC = (long *)rc;
    rectstr[0] = '\0';
    for(i = 0; i < 4; i++) {
        wcscat(rectstr, _itow(RC[i], txt, 10));
        wcscat(rectstr, L",");
    }
    return rectstr;
}
// Save a rect as a string in a Zone<num> entry in the inifile
static void SaveZone(RECT *rc, unsigned num)
{
    wchar_t txt[128], name[32];
//    LOG("Saving %d\n", num);
//    LOG("%S\n", ZidxToZonestr(num, name))
//    LOG("%S\n", RectToStr(rc, txt))
    WritePrivateProfileString(L"Zones", ZidxToZonestr(num, name), RectToStr(rc, txt), inipath);
}
static void ClearAllZones()
{
    int i;
    wchar_t txt[128], name[32];
    for (i = 0; i < 32; i++) {
        ZidxToZonestr(i, name);
        if (GetPrivateProfileString(L"Zones", name, L"", txt, ARR_SZ(txt), inipath)) {
            WritePrivateProfileString(L"Zones", name, L"", inipath);
        }
    }
}
// Call with lParam = 1 to reset NZones
BOOL CALLBACK SaveTestWindow(HWND hwnd, LPARAM lParam)
{
    static unsigned NZones;
    if (lParam) { // Reset number of Zones
        NZones = 0;
        return FALSE;
    }

    wchar_t classn[256];
    RECT rc;
    if (IsWindowVisible(hwnd)
    && GetClassName(hwnd, classn, sizeof(classn))
    && !wcscmp(classn, APP_NAME"-Test")
    && GetWindowRectL(hwnd, &rc)) {
        SaveZone(&rc, NZones++);
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
    return TRUE;
}

static void SaveCurrentLayout()
{
    ClearAllZones();
    SaveTestWindow(NULL, 1);
    EnumDesktopWindows(NULL, SaveTestWindow, 0);
}

/////////////////////////////////////////////////////////////////////////////
static void ShowContextMenu(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU menu = CreatePopupMenu();

    InsertMenu(menu, -1, MF_BYPOSITION, SWM_TOGGLE, (ENABLED()?l10n->menu_disable:l10n->menu_enable));
    InsertMenu(menu, -1, MF_BYPOSITION, SWM_HIDE, l10n->menu_hide);
    if(VISTA)
        InsertMenu(menu, -1, elevated?MF_BYPOSITION|MF_GRAYED:MF_BYPOSITION
                 , SWM_ELEVATE, (elevated? l10n->general_elevated: l10n->general_elevate));

    InsertMenu(menu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
    InsertMenu(menu, -1, MF_BYPOSITION, SWM_CONFIG, l10n->menu_config);
    InsertMenu(menu, -1, MF_BYPOSITION, SWM_ABOUT, l10n->menu_about);

    if (UseZones) { // Zones section
        InsertMenu(menu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
        InsertMenu(menu, -1, MF_BYPOSITION, SWM_TESTWIN,  l10n->advanced_testwindow);
        InsertMenu(menu, -1, FindWindow(APP_NAME"-test", NULL)? MF_BYPOSITION:MF_BYPOSITION|MF_GRAYED
                  , SWM_SAVEZONES, l10n->menu_savezones);
    }

    InsertMenu(menu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);

    InsertMenu(menu, -1, MF_BYPOSITION, SWM_EXIT, l10n->menu_exit);

    // Track menu
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(menu);
}
