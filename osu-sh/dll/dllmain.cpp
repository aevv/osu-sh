#undef UNICODE
#define WIN32_LEAN_AND_MEAN //so winsock.h doesnt mess with winsock2 (not needed for this build?)
#include <stdio.h>
#include <sstream>
#include <string>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "mhook-lib\mhook.h"
using namespace std;

//typedefs of original winapi functions
typedef BOOL (WINAPI *pBASS_ChannelSetAttribute)(DWORD, DWORD, float);
typedef BOOL (WINAPI *pQueryPerformanceCounter)(LARGE_INTEGER*);
typedef DWORD (WINAPI *pTimeGetTime)(void);
typedef DWORD (WINAPI *pGetTickCount)(void);

//prototypes for our functions that are called instead
BOOL WINAPI MyBASS_ChannelSetAttribute(DWORD, DWORD, float);
BOOL WINAPI MyQueryPerformanceCounter(LARGE_INTEGER*);
DWORD WINAPI MyTimeGetTime(void);
DWORD WINAPI MyGetTickCount(void);

//variables to contain the original functions
pBASS_ChannelSetAttribute pOrigAttr = NULL;
pQueryPerformanceCounter pOrigQuery = NULL;
pTimeGetTime pOrigTime = NULL;
pGetTickCount pOrigTick = NULL;

BOOL basshook;
BOOL queryhook;
BOOL timehook;
BOOL tickhook;

//stuff for hack
DWORD baseTime, baseTicks;
LARGE_INTEGER basePerf = LARGE_INTEGER();
std::stringstream ss;
float speed = 0.75;
float realSpeed;
float shSpeed;
BOOL worked = TRUE;
BOOL doubletime = FALSE;
BOOL halftime = FALSE;
BOOL nomod = TRUE;
//stackoverflow easymodelol
 double string_to_double( const std::string& s )
 {
   std::istringstream i(s);
   double x;
   if (!(i >> x))
     return 0;
   return x;
 } 
 bool to_bool(std::string str) {
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    istringstream is(str);
    bool b;
    is >> boolalpha >> b;
    return b;
}
bool readSettings() {
	doubletime = FALSE;
	halftime = FALSE;
	nomod = TRUE;
	ifstream f;
	string line;
	//by this point working directory is osu!s dir
	f.open("settings.cfg", fstream::binary);
	if(f.is_open()) {
		for(int i = 0; f.good(); i++ ){			
			getline(f, line);
			switch(i) {
			case 1: 
				speed = string_to_double(line);
				break;
			case 2:
				doubletime = to_bool(line);
				break;
			case 3:
				if (doubletime == FALSE)
					halftime = to_bool(line);
				if (doubletime == FALSE && halftime == FALSE)
					nomod = TRUE;
				f.close();
				return true;
				break;
			}
		}
		if (doubletime == FALSE && halftime == FALSE)
			nomod = TRUE;
		f.close();
		return true;
	} else {
		cerr << "Could not find settings.cfg" << endl;
		return false;
	}
}

INT APIENTRY DllMain(HMODULE hDLL, DWORD Reason, LPVOID Reserved)
{	
	switch(Reason)
	{
	case DLL_PROCESS_ATTACH:
		//try to get function addresses + hook, exceptions here are uncatchable and cause the game to crash
		pOrigAttr = (pBASS_ChannelSetAttribute) GetProcAddress(GetModuleHandle("bass.dll"), "BASS_ChannelSetAttribute");
		basshook = Mhook_SetHook((PVOID*)&pOrigAttr, MyBASS_ChannelSetAttribute);	
		if(basshook==FALSE) {
			MessageBoxA(NULL,"Injection failed: bass hook", "Info", MB_ICONEXCLAMATION);
		}
		else {
			pOrigQuery = (pQueryPerformanceCounter)GetProcAddress(GetModuleHandle("kernel32.dll"), "QueryPerformanceCounter");
			queryhook = Mhook_SetHook((PVOID*)&pOrigQuery, MyQueryPerformanceCounter);
			pOrigTime = (pTimeGetTime)GetProcAddress(GetModuleHandle("winmm.dll"), "timeGetTime");
			timehook = Mhook_SetHook((PVOID*)&pOrigTime, MyTimeGetTime);
			pOrigTick = (pGetTickCount)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetTickCount");
			tickhook = Mhook_SetHook((PVOID*)&pOrigTick, MyGetTickCount);
			if (tickhook==FALSE) {
				MessageBoxA(NULL,"Injection failed: tick hook", "Info", MB_ICONEXCLAMATION);
			}
			else if (timehook==FALSE) {
				MessageBoxA(NULL,"Injection failed: time hook", "Info", MB_ICONEXCLAMATION);
			}
			else if (queryhook==FALSE) {
				MessageBoxA(NULL,"Injection failed: query hook", "Info", MB_ICONEXCLAMATION);
			}
			else
			{
				worked = TRUE;
			}
			if (worked == TRUE)
			{	
				baseTime = pOrigTime();
				baseTicks = pOrigTick();
				pOrigQuery(&basePerf);		
			}
			MessageBoxA(NULL,"Injection success", "Info", MB_ICONEXCLAMATION);
		}
		break;
	case DLL_PROCESS_DETACH:
		Mhook_Unhook((PVOID*)&pOrigAttr);
		Mhook_Unhook((PVOID*)&pOrigQuery);
		Mhook_Unhook((PVOID*)&pOrigTick);
		Mhook_Unhook((PVOID*)&pOrigTime);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}


BOOL WINAPI MyBASS_ChannelSetAttribute(DWORD handle, DWORD attrib, float value)
{
	if (attrib == 3 || attrib == 2) //maybe find the specific number for speed and only change that, one of these are volume
	{		
		return pOrigAttr(handle, attrib, value);
	}
	return pOrigAttr(handle, attrib, realSpeed);
}
BOOL read = FALSE;
BOOL WINAPI MyQueryPerformanceCounter(LARGE_INTEGER *count)
{
	ofstream o;
	o.open("settings.cfg", ios::app | ios::out);
	o.close();
	if (read == FALSE)
	{			
		read = TRUE;
		readSettings();	
		realSpeed = (speed * 100) - 100;
		double mult = 1;
		if (doubletime == TRUE)
		{
			mult = 1.5;
		}
		else if (halftime == TRUE)
		{
			mult = 0.75;
		}
		shSpeed = speed / mult;
	}
	pOrigQuery(count);
	if (worked == TRUE)
	{
		LARGE_INTEGER current;
		pOrigQuery(&current);
		LARGE_INTEGER change;
		change.QuadPart = current.QuadPart - basePerf.QuadPart;
		change.QuadPart = change.QuadPart * shSpeed;
		count->QuadPart = basePerf.QuadPart + change.QuadPart;
	}	
	return TRUE;
}
DWORD WINAPI MyTimeGetTime(void)
{
	if (worked == TRUE)
	{
		DWORD current = pOrigTime();
		DWORD change = current - baseTime;
		change = change * shSpeed;
		DWORD newTime = baseTime + change;
		if (newTime == 0)
		{
			return pOrigTime();
		}
		return newTime;
	}
	return pOrigTime();
}
DWORD WINAPI MyGetTickCount(void)
{
	if (worked == TRUE)
	{
		DWORD current = pOrigTick();
		DWORD change = current - baseTicks;
		change = change * shSpeed;
		DWORD newTicks = baseTicks + change;
		if (newTicks == 0)
		{
			return pOrigTick();
		}
		return newTicks;
	}
	return pOrigTick();
}