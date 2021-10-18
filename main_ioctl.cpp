/****************************************************************************

    Модуль main_ioctl.cpp

    Программа управления драйвером с виртуальной файловой системой.

    Маткин Илья Александрович               12.06.2020

****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <dbt.h>
#include <tchar.h>

#include "install_driver.h"

#include "ioctl.h"


//----------------------------------------



BOOL BroadcastDeviceChange (CHAR diskLetter, BOOL bRemoved) {

DWORD receipients;
DWORD deviceEvent;
DEV_BROADCAST_VOLUME params;


    if (!isalpha(diskLetter)) {
        return FALSE;
        }

    receipients = BSM_APPLICATIONS | BSM_ALLDESKTOPS;

    deviceEvent = bRemoved ? DBT_DEVICEREMOVECOMPLETE : DBT_DEVICEARRIVAL;

    ZeroMemory(&params, sizeof(params));
    params.dbcv_size = sizeof(params);
    params.dbcv_devicetype = DBT_DEVTYP_VOLUME;
    params.dbcv_reserved = 0;
    params.dbcv_unitmask = (1 << (toupper(diskLetter) - 'A'));
    params.dbcv_flags = 0;

    if (BroadcastSystemMessage (BSF_NOHANG | BSF_FORCEIFHUNG | BSF_NOTIMEOUTIFNOTHUNG, 
        &receipients, WM_DEVICECHANGE, deviceEvent, (LPARAM)&params) <= 0) {
        return FALSE;
        }

    return TRUE;
}



void PrintDosDevices (void) {

DWORD drives = GetLogicalDrives();
unsigned int i;


    for (i = 0; i <= TEXT('Z') - TEXT('A'); ++i) {
        if (drives & (1 << i)) {
            TCHAR letter = TEXT('A') + i;
            TCHAR fsName[100];
            TCHAR volumeName[100];
            TCHAR rootName[100];
            DWORD volumeSize;
            DWORD maxComponentLength;
            DWORD fsFlags;
            PTCHAR driveTypesNames[] = {TEXT("DRIVE_UNKNOWN"), TEXT("DRIVE_NO_ROOT_DIR"), TEXT("DRIVE_REMOVABLE"), TEXT("DRIVE_FIXED"), TEXT("DRIVE_REMOTE"), TEXT("DRIVE_CDROM"), TEXT("DRIVE_RAMDISK")};
            TCHAR targetPath[100];

            //_tprintf (TEXT("%c"), letter);

            _stprintf (rootName, TEXT("%c:"), letter);
            _tprintf (TEXT("%s"), rootName);
            if (QueryDosDevice (rootName, targetPath, sizeof(targetPath))) {
                _tprintf (TEXT(" -> %s"), targetPath);
                }

            _stprintf (rootName, TEXT("%c:\\"), letter);
            if (GetVolumeInformation (rootName, 
                volumeName, sizeof(volumeName),
                &volumeSize, &maxComponentLength, &fsFlags,
                fsName, sizeof(fsName))) {

                _tprintf (TEXT(" %s %s %s"), volumeName, fsName,
                    driveTypesNames[GetDriveType (rootName)]);
                }
            printf ("\n");
            }
        }

    return;
}



int main (int argc, char *argv[]) {

char buf[2*MAX_PATH];
char in [100];          // входной буфер
char out [100];         // выходной буфер
DWORD BytesReturned;
char *driverName;
char *driverPath;

    if (argc < 3) {
        printf ("usage: %s driver_name driver_path\n", argv[0]);
        return 0;
        }

    driverName = argv[1];
    driverPath = argv[2];

    PrintDosDevices();

    printf ("Load %s: %s\n", driverName, driverPath);

    // установка и запуск драйвера
    if (!InstallDriver (driverName, driverPath)) {
        printf ("Error install driver\n");
        return 0;
        }
    if (!StartDriver (driverName)) {
        RemoveDriver (driverName);
        printf ("Error start driver\n");
        return 0;
        }

    // открываем устройство драйвера
    HANDLE file = CreateFileA("T:\\",
                   GENERIC_READ | GENERIC_WRITE, 
                   FILE_SHARE_READ | FILE_SHARE_WRITE, 
                   NULL, 
                   OPEN_EXISTING, 
                   FILE_ATTRIBUTE_NORMAL, 
                   (HANDLE) NULL);

    // посылаем всем (в том числе проводнику) оконное сообщение о добавлении нового диска
    BroadcastDeviceChange ('T', FALSE);
    PrintDosDevices();
    
    getchar();

    // посылаем всем оконное сообщение об удалении диска
    BroadcastDeviceChange ('T', TRUE);

    CloseHandle(file);

    // останавливаем и выгружаем драйвер
    if (!StopDriver (driverName)) {
        printf ("Error stop driver\n");
        return 0;
        }
    if (!RemoveDriver (driverName)) {
        printf ("Error remove driver\n");
        return 0;
        }

    return 0;
}


//----------------------------------------

//----------------------------------------
