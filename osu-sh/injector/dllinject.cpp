#include "stdafx.h"
#include <stdio.h>
#include <TlHelp32.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <Psapi.h>
using namespace std;

extern "C" __declspec(dllexport) int init();

string dllDir;

int getProc() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(hSnapshot) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if(Process32First(hSnapshot,&pe32)) {
            do {
				
			   if (wcscmp(pe32.szExeFile, L"osu!.exe")==0)
				   return pe32.th32ProcessID;
            } while(Process32Next(hSnapshot,&pe32));
         }
         CloseHandle(hSnapshot);
    }
	return -1;
}

//reads
bool readSettings() {
	ifstream f;
	string line;
	f.open("settings.ini");
	if(f.is_open()) {
		for(int i = 0; f.good(); i++ ){			
			getline(f, line);
			switch(i) {
			case 0: 
				dllDir = line;
				cout << "dllDir: " << dllDir << endl;
				break;
			}
		}
		f.close();
		return true;
	} else {
		cerr << "Could not find settings.ini" << endl;
		return false;
	}
}
int main()
{
	return init();
}
void copySettings(string loc)
{
	ifstream f1("settings.ini", fstream::binary);
	ofstream f2(loc.c_str(), fstream::trunc|fstream::binary);
	f2 << f1.rdbuf();
	f1.close();
	f2.close();
}
int init()
{
	STARTUPINFOA lpStartupInfo = {sizeof(STARTUPINFOA)};
	PROCESS_INFORMATION lpProcessInfo={0};
	memset(&lpProcessInfo, 0, sizeof(lpProcessInfo));
	memset(&lpProcessInfo, 0, sizeof(lpProcessInfo));
	if(!readSettings()) {
		//getchar();
	}
	HANDLE hProcess;
	int id;
	id = getProc();
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE,id);
	if (hProcess == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "cannot open that pid\n");
		getchar();
		return 1;
	}
	char* file = new char[256];
	DWORD out = GetModuleFileNameExA(hProcess, NULL, file, 256);
	const char * path = dllDir.c_str();
	string s = string(file);
	s = s.substr(0, strlen(s.c_str()) - 9);
	s = s + string("\\settings.cfg");
	cout << "exe; " << s.c_str() << endl;
	copySettings(s);
	PVOID mem = VirtualAllocEx(hProcess, NULL, strlen(path) + 1, MEM_COMMIT, PAGE_READWRITE);
	HMODULE sh;
	if (mem == NULL)
	{
		fprintf(stderr, "can't allocate memory in that pid\n");
		CloseHandle(hProcess);
		getchar();
		return 2;
	}
	
	if (WriteProcessMemory(hProcess, mem, (void*)path, strlen(path) + 1, NULL) == 0)
	{
		fprintf(stderr, "can't write to memory in that pid\n");
		VirtualFreeEx(hProcess, mem, strlen(path) + 1, MEM_RELEASE);
		CloseHandle(hProcess);
		getchar();
		return 3;
	}
	cout << "load addr" << (DWORD)GetProcAddress(GetModuleHandleA("KERNEL32.DLL"),"LoadLibraryA") << endl;
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) GetProcAddress(GetModuleHandleA("KERNEL32.DLL"),"LoadLibraryA"), mem, 0, NULL);
	if (hThread == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "can't create a thread in that pid\n");
		VirtualFreeEx(hProcess, mem, strlen(path) + 1, MEM_RELEASE);
		CloseHandle(hProcess);
		getchar();
		return 4;
	}

	WaitForSingleObject(hThread, INFINITE);
	HMODULE hLibrary = NULL;
	if (!GetExitCodeThread(hThread, (LPDWORD)&hLibrary))
	{
		printf("can't get exit code for thread GetLastError() = %i.\n", GetLastError());
		CloseHandle(hThread);
		VirtualFreeEx(hProcess, mem, strlen(path) + 1, MEM_RELEASE);
		CloseHandle(hProcess);
		getchar();
		return 5;
	}
	CloseHandle(hThread);	
	VirtualFreeEx(hProcess, mem, strlen(path) + 1, MEM_RELEASE);
	if (hLibrary == NULL)
	{
		hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) GetProcAddress(GetModuleHandleA("KERNEL32.DLL"),"GetLastError"), 0, 0, NULL);
		if (hThread == INVALID_HANDLE_VALUE)
		{
			fprintf(stderr, "LoadLibraryA returned NULL and can't get last error.\n");
			CloseHandle(hProcess);
			getchar();
			return 6;
		}

		WaitForSingleObject(hThread, INFINITE);
		DWORD error;
		GetExitCodeThread(hThread, &error);

		CloseHandle(hThread);

		printf("LoadLibrary return NULL, GetLastError() is %i\n", error);
		CloseHandle(hProcess);
		getchar();
		return 7;
	}
	
	CloseHandle(hProcess);
	return 0;
}
