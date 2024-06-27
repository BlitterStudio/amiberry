// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

#ifdef _WIN32

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CAPSInit();
		break;

	case DLL_PROCESS_DETACH:
		CAPSExit();
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

#endif // _WIN32
