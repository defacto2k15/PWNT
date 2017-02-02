#pragma once

#include "stdafx.h"
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include "Common.h"
#include <iostream>
#include "IoCompletitionPortServer.h"

#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("aaMySynchronizationBarrierService")

SERVICE_STATUS          gSvcStatus; 
SERVICE_STATUS_HANDLE   gSvcStatusHandle; 
HANDLE                  ghSvcStopEvent = NULL;

VOID SvcInstall(void);
VOID SvcUninstall(void);
VOID SvcStart(void);
VOID WINAPI SvcCtrlHandler( DWORD ); 
VOID WINAPI SvcMain( DWORD, LPTSTR * ); 

VOID ReportSvcStatus( DWORD, DWORD, DWORD );
VOID SvcInit( DWORD, LPTSTR * ); 
VOID SvcReportEvent( LPTSTR );


void serviceAppMain(int argc, char ** argv) 
{ 
    // If command-line parameter is "install", install the service. 
    // Otherwise, the service is probably being started by the SCM.
	if( argc == 2 &&  strcmp(argv[1], "-install") == 0)
    {
        SvcInstall();
        return;
    } else 	if( argc == 2 &&  strcmp(argv[1], "-uninstall") == 0) {
        SvcUninstall();
        return;
    } else 	if( argc == 2 &&  strcmp(argv[1], "-start") == 0) {
        SvcStart();
        return;
    } else {
		std::cout << "Usage: -install : install service " << std::endl;
		std::cout << "Usage: -uninstall : uninstall service " << std::endl;
		std::cout << "Usage: -interactive : interactive testing app " << std::endl;
		std::cout << "Usage: -start :start service " << std::endl;
	}

	
    SERVICE_TABLE_ENTRY DispatchTable[] = 
    { 
        { SVCNAME, (LPSERVICE_MAIN_FUNCTION) SvcMain }, 
        { NULL, NULL } 
    }; 
 
    // This call returns when the service has stopped. 
    // The process should simply terminate when the call returns.

    if (!StartServiceCtrlDispatcher( DispatchTable )) 
    { 
        SvcReportEvent(TEXT("StartServiceCtrlDispatcher")); 
    } 
} 

void SvcInstall()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    TCHAR szPath[MAX_PATH];

	if( check( GetModuleFileName( NULL, szPath, MAX_PATH) != NULL, "Service: GetModuleFileName failed" )) {
		return;
	}

    // Get a handle to the SCM database. 
 
    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 
 
	if(check(schSCManager != NULL, "Service: OpenSCManager failed ") ){
		return;
	}

    // Create the service

    schService = CreateService( 
        schSCManager,              // SCM database 
        SVCNAME,                   // name of service 
        SVCNAME,                   // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_DEMAND_START,      // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        szPath,                    // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 
 
	if( check(schService != NULL, "Service: schService failed") ){
		CloseServiceHandle(schSCManager);
		return;
	}

    std::cout << "Service installed successfully" << std::endl;

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

// service entry point
VOID WINAPI SvcMain( DWORD dwArgc, LPTSTR *lpszArgv )
{
    // Register the handler function for the service

    gSvcStatusHandle = RegisterServiceCtrlHandler( 
        SVCNAME, 
        SvcCtrlHandler);

	if( check( gSvcStatusHandle != NULL, "Service :gSvcStatusHandle")){
		SvcReportEvent(TEXT("RegisterServiceCtrlHandler")); 
	}


    // These SERVICE_STATUS members remain as set here

    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
    gSvcStatus.dwServiceSpecificExitCode = 0;    

    // Report initial status to the SCM

    ReportSvcStatus( SERVICE_START_PENDING, NO_ERROR, 3000 );

    // Perform service-specific initialization and work.

    SvcInit( dwArgc, lpszArgv );
}

// initialize service
VOID SvcInit( DWORD dwArgc, LPTSTR *lpszArgv)
{
    // Create an event. The control handler function, SvcCtrlHandler,
    // signals this event when it receives the stop control code.

    ghSvcStopEvent = CreateEvent(
                         NULL,    // default security attributes
                         TRUE,    // manual reset event
                         FALSE,   // not signaled
                         NULL);   // no name

    if ( ghSvcStopEvent == NULL)
    {
        ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
        return;
    }

    // Report running status when initialization is complete.

    ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );

	SvcReportEvent(TEXT("Starting pipe thread"));
	std::thread pipeServer( [=](){ startIocpServer();});
	pipeServer.detach();

    while(1)
    {
        // Check whether to stop the service.

        WaitForSingleObject(ghSvcStopEvent, INFINITE);

        ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
        return;
    }
}


VOID ReportSvcStatus( DWORD dwCurrentState,
                      DWORD dwWin32ExitCode,
                      DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    // Fill in the SERVICE_STATUS structure.

    gSvcStatus.dwCurrentState = dwCurrentState;
    gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
    gSvcStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
        gSvcStatus.dwControlsAccepted = 0;
    else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if ( (dwCurrentState == SERVICE_RUNNING) ||
           (dwCurrentState == SERVICE_STOPPED) )
        gSvcStatus.dwCheckPoint = 0;
    else gSvcStatus.dwCheckPoint = dwCheckPoint++;

    // Report the status of the service to the SCM.
    SetServiceStatus( gSvcStatusHandle, &gSvcStatus );
}

//   Called by SCM whenever a control code is sent to the service
//   using the ControlService function.
VOID WINAPI SvcCtrlHandler( DWORD dwCtrl )
{
   // Handle the requested control code. 
   switch(dwCtrl) 
   {  
      case SERVICE_CONTROL_STOP: 
         ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

         // Signal the service to stop.

         SetEvent(ghSvcStopEvent);
         ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);
         
         return;
 
      case SERVICE_CONTROL_INTERROGATE: 
         break; 
 
      default: 
         break;
   } 
   
}

//   Logs messages to the event log
VOID SvcReportEvent(LPTSTR szFunction) 
{ 
    HANDLE hEventSource;
    LPCTSTR lpszStrings[2];
    TCHAR Buffer[80];

    hEventSource = RegisterEventSource(NULL, SVCNAME);

    if( NULL != hEventSource )
    {
        StringCchPrintf(Buffer, 80, TEXT("%s reported with %d"), szFunction, GetLastError());

        lpszStrings[0] = SVCNAME;
        lpszStrings[1] = Buffer;

        ReportEvent(hEventSource,        // event log handle
                    EVENTLOG_SUCCESS, // event type
                    0,                   // event category
                    0,           // event identifier
                    NULL,                // no security identifier
                    2,                   // size of lpszStrings array
                    0,                   // no binary data
                    lpszStrings,         // array of strings
                    NULL);               // no binary data

        DeregisterEventSource(hEventSource);
    }
}

//   Stop and remove the service from the local service control 
//   manager database.
void SvcUninstall()
{
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS ssSvcStatus = {};

	auto pszServiceName = SVCNAME;

    // Open the local default service control manager database
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if(check(schSCManager != NULL, "Service: UninstallService OpenSCManager failed ") ){
		return;
	}

    // Open the service with delete, stop, and query status permissions
    schService = OpenService(schSCManager, pszServiceName, SERVICE_STOP | 
        SERVICE_QUERY_STATUS | DELETE);
	if( check(schService != NULL , "Service: UninstallService OpenService failed ")){
		CloseServiceHandle(schSCManager);
		CloseServiceHandle(schService);
		return;
	}


    // Try to stop the service
    if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
    {
		std::cout << " Stopping service ";
        Sleep(1000);

        while (QueryServiceStatus(schService, &ssSvcStatus))
        {
            if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
            {
                std::cout << ".";
                Sleep(1000);
            }
            else break;
        }

        if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
        {
			std::cout << " Service is stopped" << std::endl;
        }
        else
        {
			std::cout << " Service failed to stop" << std::endl;
        }
    }

    // Now remove the service by calling DeleteService.
	if( check(DeleteService(schService)," Service: DeleteService failed")){
		CloseServiceHandle(schSCManager);
		CloseServiceHandle(schService);
		return;
	}

	std::cout << "Service was removed " << std::endl;

	CloseServiceHandle(schSCManager);
	CloseServiceHandle(schService);
}

void SvcStart(){
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;

	auto pszServiceName = SVCNAME;

    // Open the local default service control manager database
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if(check(schSCManager != NULL, "Service: UninstallService OpenSCManager failed ") ){
		return;
	}

    // Open the service with delete, stop, and query status permissions
    schService = OpenService(schSCManager, pszServiceName, SERVICE_STOP | 
        SERVICE_QUERY_STATUS | DELETE | SERVICE_START);
	if( check(schService != NULL , "Service: UninstallService OpenService failed ")){
		CloseServiceHandle(schSCManager);
		CloseServiceHandle(schService);
		return;
	}

	auto res = StartService( schService, 0, NULL );
	if( check(res, "Service: starting failed ")){
		return;
	}
	std::cout << "Service: Started!" << std::endl;
}