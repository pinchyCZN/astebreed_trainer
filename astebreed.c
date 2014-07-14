#include <windows.h>
#include <tlhelp32.h>
#include "resource.h"
HWND ghwindow=0;

int get_train1(char **buf,int *len,int *address){

	__asm{
		lea eax,END
		lea ebx,START
		sub eax,ebx
		mov ebx,len
		mov [ebx],eax
		mov	eax,buf
		lea ebx,START
		mov	[eax],ebx
		mov eax,address
		mov	dword ptr[eax],0x004002B0
		jmp END
START:
		fld dword ptr [esp+0x10]				//original code 
		fstp st(0)							//pop the float 
		mov dword ptr [esp+0x10], 0x42C80000	//move 100.00 into health
		mov eax,0x0043CC62
		jmp eax
END:
	}
	return TRUE;
};

int get_train2(char **buf,int *len,int *address){

	__asm{
		lea eax,END
		lea ebx,START
		sub eax,ebx
		mov ebx,len
		mov [ebx],eax
		mov	eax,buf
		lea ebx,START
		mov	[eax],ebx
		mov eax,address
		mov	dword ptr[eax],0x0043CC5B
		jmp END
START:
		mov eax,0x004002B0
		jmp eax
END:
	}
	return TRUE;
};

int get_train3(char **buf,int *len,int *address)
{
	__asm{
		lea eax,END
		lea ebx,START
		sub eax,ebx
		mov ebx,len
		mov [ebx],eax
		mov	eax,buf
		lea ebx,START
		mov	[eax],ebx
		mov eax,address
		mov	dword ptr[eax],0x00400300
		jmp END
START:
		fst dword ptr [ecx-0x30]    // Original Code 
		push 00
		fstp st(0)  // Pop the float 
		mov dword ptr [ecx-0x38], 0x44160000 // Move 600.0 into energy
		mov eax,0x004DD810
		jmp eax
END:
	}
	return TRUE;
}
int get_train4(char **buf,int *len,int *address)
{
	__asm{
		lea eax,END
		lea ebx,START
		sub eax,ebx
		mov ebx,len
		mov [ebx],eax
		mov	eax,buf
		lea ebx,START
		mov	[eax],ebx
		mov eax,address
		mov	dword ptr[eax],0x004DD808
		jmp END
START:
		mov eax,0x00400300
		jmp eax
END:
	}
	return TRUE;
}


int find_process(char *exename,int *pid)
{
	HANDLE snapshot;
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	
	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if(snapshot!=INVALID_HANDLE_VALUE){
		if(Process32First(snapshot, &entry) == TRUE){
			while(Process32Next(snapshot, &entry) == TRUE){
				if(stricmp(entry.szExeFile, exename) == 0){
					*pid=entry.th32ProcessID;
					break;
				}
			}
		}
		CloseHandle(snapshot);
	}
	return 0;
}
int read_process(int phandle,int address,char *data,int len)
{
	DWORD read_len=0;
	ReadProcessMemory(phandle,(void *)address,data,len,&read_len);
	return read_len!=0;
}


int write_process(int phandle,int address,char *data,int len)
{
	DWORD write_len=0,old=0;
	VirtualProtectEx(phandle,address,len,PAGE_EXECUTE_READWRITE,&old);
	WriteProcessMemory(phandle,(void *)address,data,len,&write_len);
	return write_len!=0;
}
int verify_game(HANDLE phandle)
{
	int result=FALSE;
	char data[20];
	memset(data,0,sizeof(data));
	if(read_process(phandle,0x0043CC47,data,11)){
		static char test[11]={0xD9,0x5C,0x24,0x10,0xD9,0x44,0x24,0x10,0xD9,0x5E,0x10};
		result=memcmp(data,test,11)==0;
	}
	return result;
}
int mod_game(int rewrite)
{
	char *data=0;
	int len=0;
	int address=0;
	static int pid=0;
	static int done=FALSE;
	if(rewrite){
		done=FALSE;
		pid=0;
	}
	if(done)
		return TRUE;
	if(pid==0)
		find_process("astebreed.exe",&pid);
	if(pid==0){
		SetDlgItemText(ghwindow, IDC_STATUS, "error: cannot find game process!");
	}
	else{
		static HANDLE phandle=0;
		if(phandle==0){
			phandle=OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		}
		if(phandle==0){
			SetDlgItemText(ghwindow, IDC_STATUS, "error: cannot open game process!");
		}
		else{
			if(!done){
				if(!verify_game(phandle)){
					SetDlgItemText(ghwindow, IDC_STATUS, "unable to verify version!");
				}
				else{
					get_train1(&data,&len,&address);
					write_process(phandle,address,data,len);
					get_train2(&data,&len,&address);
					write_process(phandle,address,data,len);
					get_train3(&data,&len,&address);
					write_process(phandle,address,data,len);
					get_train4(&data,&len,&address);
					write_process(phandle,address,data,len);
					SetDlgItemText(ghwindow, IDC_STATUS, "game memory patched");
				}
				CloseHandle(phandle);
				phandle=0;
				done=TRUE;
			}
		}
	}
	return TRUE;
}

LRESULT CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg){
	case WM_INITDIALOG:
		ghwindow=hwnd;
		SetDlgItemText(hwnd,IDC_TRAINER_INFO,"Astebreed 2.00 trainer\r\n"
			"inf health\r\n"
			"inf energy");
		SetTimer(hwnd,0,1000,NULL);
		mod_game(TRUE);
		return TRUE;
	case WM_CLOSE:
		EndDialog(hwnd,0);
		break;
	case WM_TIMER:
		mod_game(FALSE);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		case IDOK:
			SetDlgItemText(ghwindow, IDC_STATUS, "");
			mod_game(TRUE);
			break;
		}
		break;

	}
	return FALSE;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOG),NULL,dialog_proc);
	return 0;
}



