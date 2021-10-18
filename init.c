#include <ntifs.h>
#include <ntddk.h>
#include <wdm.h>
#include <windef.h>
#include <fltKernel.h>

#include "ioctl.h"
#include "ntstring.h"
#include "ufs.h"



extern char *PsGetProcessImageFileName (PEPROCESS proc);

//*************************************************************
// макросы и глобальные переменные


#define	SYM_LINK_NAME   L"\\Global??\\T:"
#define DEVICE_NAME     L"\\Device\\DDriver"


UNICODE_STRING  DevName;
UNICODE_STRING	SymLinkName;

char *glWriteBuffer;
ULONG glSizeWriteBuffer;


#define FILE_NAME_1 L"dir"
#define DIR_NAME    FILE_NAME_1
#define FILE_NAME_2 L"file2"
#define FILE_NAME   FILE_NAME_2
#define FILES_COUNT 2

#define VOLUME_LABEL L"UFS_VOLUME"
#define FS_NAME     L"UFS"


//*************************************************************
// предварительное объявление функций


NTSTATUS DriverEntry (IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING registryPath);
VOID DriverUnload (IN PDRIVER_OBJECT pDriverObject);
NTSTATUS CompleteIrp (PIRP pIrp, NTSTATUS status, ULONG info);
NTSTATUS DispatchCreate (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DispatchClose (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DispatchRead (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DispatchWrite (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DispatchQueryInformation (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DispatchSetInformation (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DispatchControl (IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp);
NTSTATUS DispatchDirectoryControl (IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp);
NTSTATUS DispatchIrp (IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp);
NTSTATUS DispatchVolumeQueryInformation (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS DispatchQuerySecurity (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

NTSTATUS DispathCreateDirectory2(ParseNameEntry* names);

void GetSystemTime(PTIME_FIELDS time);

NTSTATUS CompleteIrp (PIRP Irp, NTSTATUS status,ULONG Info);


//*************************************************************
// описание функций


// функция инициализации драйвера
NTSTATUS DriverEntry (IN PDRIVER_OBJECT pDriverObject,IN PUNICODE_STRING RegistryPath){
		
PDEVICE_OBJECT  pDeviceObject;				// указатель на объект устройство
NTSTATUS		status = STATUS_SUCCESS;	// статус завершения функции

    pDriverObject->MajorFunction [IRP_MJ_CREATE                  ] = DispatchCreate;
    pDriverObject->MajorFunction [IRP_MJ_CLOSE                   ] = DispatchClose;
    pDriverObject->MajorFunction [IRP_MJ_READ                    ] = DispatchRead;
    pDriverObject->MajorFunction [IRP_MJ_WRITE                   ] = DispatchWrite;
    pDriverObject->MajorFunction [IRP_MJ_QUERY_INFORMATION       ] = DispatchQueryInformation;
    pDriverObject->MajorFunction [IRP_MJ_SET_INFORMATION         ] = DispatchSetInformation;
    pDriverObject->MajorFunction [IRP_MJ_QUERY_EA                ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_SET_EA                  ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_FLUSH_BUFFERS           ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_QUERY_VOLUME_INFORMATION] = DispatchVolumeQueryInformation;
    pDriverObject->MajorFunction [IRP_MJ_SET_VOLUME_INFORMATION  ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_DIRECTORY_CONTROL       ] = DispatchDirectoryControl;
    pDriverObject->MajorFunction [IRP_MJ_FILE_SYSTEM_CONTROL     ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL          ] = DispatchControl;
    pDriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_SHUTDOWN                ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_LOCK_CONTROL            ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_CLEANUP                 ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_CREATE_MAILSLOT         ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_QUERY_SECURITY          ] = DispatchQuerySecurity;
    pDriverObject->MajorFunction [IRP_MJ_SET_SECURITY            ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_POWER                   ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_SYSTEM_CONTROL          ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_DEVICE_CHANGE           ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_QUERY_QUOTA             ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_SET_QUOTA               ] = DispatchIrp;
    pDriverObject->MajorFunction [IRP_MJ_PNP                     ] = DispatchIrp;

    pDriverObject->DriverUnload = DriverUnload;

#if DBG
    DbgPrint ("Load driver %wZ",&pDriverObject->DriverName);
#endif


    RtlInitUnicodeString (&DevName, DEVICE_NAME);
    RtlInitUnicodeString (&SymLinkName, SYM_LINK_NAME);

    // создание устройства
	status = IoCreateDevice (pDriverObject,	// указатель на объект драйвера
                            0,				// размер области дополнительной памяти устройства
                            &DevName,		// имя устройства
                            FILE_DEVICE_FILE_SYSTEM,	// идентификатор типа устройства
                            0,				// дополнительная информация об устройстве
                            FALSE,			// без эксклюзивного доступа
                            &pDeviceObject); // адрес для сохранения указателя на объект устройства
    if (!NT_SUCCESS(status)) {
        return status;
        }

    pDeviceObject->Flags |= DO_BUFFERED_IO;

#if DBG
    DbgPrint ("Create device %ws", DEVICE_NAME);
#endif

    // создание символьной ссылки
	status = IoCreateSymbolicLink (&SymLinkName, &DevName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice (pDeviceObject);
        return status;
        }

#if DBG
    DbgPrint ("Create symbolic link %ws", SYM_LINK_NAME);
#endif


    // Инициализация корня UFS
    ExInitializePagedLookasideList(&glPagedFileList, NULL, NULL, 0, sizeof(OpenFileEntry), ' LFO', 0);
    ExInitializePagedLookasideList(&glPagedDirList, NULL, NULL, 0, sizeof(OpenDirEntry), ' LFO', 0);
    ExInitializePagedLookasideList(&glPagedParseEntry, NULL, NULL, 0, sizeof(ParseNameEntry), ' LFO', 0);

    InitializeListHead(&glUniformFileSystem.dir);
    InitializeListHead(&glUniformFileSystem.file);
    RtlInitUnicodeString(&glUniformFileSystem.name, FS_NAME);
    glUniformFileSystem.drank = RANK_UFS;
    glUniformFileSystem.frank = RANK_UFS;
    glUniformFileSystem.link.Blink = NULL;
    glUniformFileSystem.link.Flink = NULL;
    GetSystemTime(&glUniformFileSystem.creationTime);
    GetSystemTime(&glUniformFileSystem.changeTime);

    return status;
}

//--------------------

// функция выгрузки драйвера
VOID DriverUnload (IN PDRIVER_OBJECT pDriverObject){

    // удаление символьной ссылки и объекта устройства
    IoDeleteSymbolicLink(&SymLinkName);
    IoDeleteDevice( pDriverObject->DeviceObject);


    //FreeUFS(&glUniformFileSystem);

    // удаляем резервный список
    ExDeletePagedLookasideList(&glPagedFileList);
    ExDeletePagedLookasideList(&glPagedDirList);
    ExDeletePagedLookasideList(&glPagedParseEntry);

    if(glWriteBuffer)
        ExFreePool(glWriteBuffer);

#if DBG
    DbgPrint("Driver unload");
#endif

    return;		
}

//--------------------

//
// Функция обработки запроса на открытие устройства драйвера
//
NTSTATUS DispatchCreate (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

NTSTATUS status = STATUS_SUCCESS;
PIO_STACK_LOCATION pIrpStack;
ULONG info = FILE_OPENED;
ULONG options;
ULONG disposition;
PWCHAR pFileName;
UNICODE_STRING copyFileName;
PWCHAR copyFileNameBuffer;
ParseNameEntry parseNames;


    pIrpStack = IoGetCurrentIrpStackLocation (pIrp);
    options = pIrpStack->Parameters.Create.Options;
    disposition = options >> 24;

    // Возможные значения для флагов pIrpStack->Parameters.Create.Options
    // FILE_DIRECTORY_FILE      открываемый файл должен быть директорией
    // FILE_NON_DIRECTORY_FILE  открываемый файл должен быть обычным файлом

    // Возможные значения для запрашиваемого действия (disposition)
    // FILE_SUPERSEDE заменить существующий файл
    // FILE_OPEN    открыть существующий файл
    // FILE_CREATE  создать несуществующий файл
    // FILE_OPEN_IF всегда открывать (при необходимости создать)
    // FILE_OVERWRITE   переписывать существующий файл
    // FILE_OVERWRITE_IF всегда создавать (если существует - переписать)

    // В зависимости от запрашиваемого действия и результата обработки
    // в info надо возвращать
    // FILE_SUPERSEDED файл заменен
    // FILE_OPENED  файл открыт
    // FILE_CREATED файл создан
    // FILE_OVERWRITTEN файл переписан
    // FILE_EXISTS  файл существует
    // FILE_DOES_NOT_EXIST  файл не существует

    copyFileName.Length = pIrpStack->FileObject->FileName.Length + 2;
    copyFileName.MaximumLength = copyFileName.Length;
    copyFileNameBuffer = ExAllocatePool (PagedPool, copyFileName.Length);
    if (!copyFileNameBuffer) {
        return CompleteIrp (pIrp, STATUS_MEMORY_NOT_ALLOCATED, 0);
        }
    copyFileName.Buffer = copyFileNameBuffer;

    RtlZeroMemory (copyFileName.Buffer, copyFileName.Length);

    RtlCopyUnicodeString (&copyFileName, &pIrpStack->FileObject->FileName);

    pFileName = wcsrchr (copyFileNameBuffer, L'\\');
    if (!pFileName) {
        status = STATUS_ACCESS_DENIED;
        // путь не содержит \ (такого быть не должно)
        pFileName = copyFileNameBuffer;
    }
    else {
        if (pFileName == copyFileNameBuffer) {
            // передано имя \...
            if (!_wcsicmp(pFileName, L"\\")) {
                // передано имя \ 
            }
            else {
                // передано имя \...
                ++pFileName;
            }
        }
        else {
            if (!_wcsicmp(pFileName, L"\\")) {
                // передано имя вида .....\ (т.е. заканчивается на \)
                // тогда ищем предыдущий \ 
                *pFileName = 0;
                pFileName = wcsrchr(copyFileNameBuffer, L'\\');
                if (!pFileName) {
                    // такого быть не должно
                    status = STATUS_ACCESS_DENIED;
                    pFileName = copyFileNameBuffer;
                }
                else {
                    ++pFileName;
                }
            }
            else {
                // передано имя вида ....\.....
                // тогда имя это следующий символ после последнего найденного \ 
                ++pFileName;
            }
        }
    }

    //DbgPrint("11111111 %p", ParseName(&pIrpStack->FileObject->FileName, &glPagedParseEntry));
    // если выставлен флаг директории, то должно быть имя директории
    if (FlagOn(options, FILE_DIRECTORY_FILE) && FlagOn(disposition, FILE_CREATE) && !status) {

        if (pIrpStack->FileObject->FileName.Length > 2) {
            if (IsValidStringName(&pIrpStack->FileObject->FileName)) {
                //DbgPrint("BOOOOOOOOOOL %d", IsValidStringName(&pIrpStack->FileObject->FileName));

                ParseName(&pIrpStack->FileObject->FileName, &parseNames);

                //DbgPrint("!!!!!!!!!!!!!! %wZ", );
                    //NTSTATUS FLTAPI FltParseFileName(
                    //    PCUNICODE_STRING FileName,
                    //    PUNICODE_STRING  Extension,
                    //    PUNICODE_STRING  Stream,
                    //    PUNICODE_STRING  FinalComponent
                    //);

                status = DispathCreateDirectory2(&parseNames);

                PrintParseName(&parseNames);
                FreeParseEntry(&parseNames);
            }
            else {
                status = STATUS_INVALID_EA_NAME;
            }
        }
        else status = STATUS_ACCESS_DENIED;
    }
    // 
    //if (!FlagOn (options, FILE_DIRECTORY_FILE) && _wcsnicmp (pFileName, FILE_NAME, wcslen(FILE_NAME))) {
    //    status = STATUS_FILE_IS_A_DIRECTORY;
    //    }


    DbgPrint("%s %s%s %s %S %wZ %08X %08X\n",
        PsGetProcessImageFileName (PsGetCurrentProcess()),
        FlagOn (disposition,FILE_CREATE) ? "Create" : "",
        FlagOn (disposition,FILE_OPEN) ? "Open" : "",
        FlagOn (options, FILE_DIRECTORY_FILE) ? "directory" : "file",
        pFileName, &pIrpStack->FileObject->FileName, options, status);


    ExFreePool (copyFileNameBuffer);

    return CompleteIrp (pIrp, status, info);
}


//--------------------

//
// Функция обработки запроса на закрытие устройства драйвера
//
NTSTATUS DispatchClose (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

NTSTATUS status = STATUS_SUCCESS;
PIO_STACK_LOCATION pIrpStack;
ULONG info = 0;

    pIrpStack = IoGetCurrentIrpStackLocation (pIrp);


    DbgPrint("Close %wZ\n", &pIrpStack->FileObject->FileName);

    return CompleteIrp (pIrp, status, info);
}


//--------------------

//
// Функция обработки запроса на чтение
//
NTSTATUS DispatchRead(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;
    ULONG info = 0;
    ULONG i;
    PCHAR outputBuffer;
    OpenDirEntry* dHeadEntry;
    OpenFileEntry* fHeadEntry;
    OpenFileEntry* fEntry;
    ParseNameEntry parseName;
    ParseNameEntry* fileName;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    // В стеке IRP доступны следующие параметры
    // pIrpStack->Parameters.Read.Length
    // pIrpStack->Parameters.Read.Key
    // pIrpStack->Parameters.Read.ByteOffset

    DbgPrint("Read %d bytes to offset %d\n", pIrpStack->Parameters.Read.Length, *((int*)&pIrpStack->Parameters.Read.ByteOffset));

    if (pDeviceObject->Flags & DO_BUFFERED_IO) {
        outputBuffer = (char*)pIrp->AssociatedIrp.SystemBuffer;
    }
    else {
        outputBuffer = (char*)pIrp->UserBuffer;
    }
    //DbgPrint ("Output buffer: %08X\n", outputBuffer);

    if (pIrpStack->FileObject->CurrentByteOffset.QuadPart) {
        return CompleteIrp(pIrp, STATUS_END_OF_FILE, 0);
    }

    if (pIrpStack->FileObject->FileName.Length > 2) {
        if (IsValidStringName(&pIrpStack->FileObject->FileName)) {
            //dHeadEntry = IsFoundDirEntry(&glUniformFileSystem, &pIrpStack->FileObject->FileName);
            status = ParseName(&pIrpStack->FileObject->FileName, &parseName);
            if (NT_SUCCESS(status)) {

                fileName = (ParseNameEntry*)CONTAINING_RECORD(parseName.link.Flink, ParseNameEntry, link);
                dHeadEntry = IsFoundDirEntry2(&glUniformFileSystem, &parseName);
                PrintParseName(&parseName);

                if (dHeadEntry->frank == RANK_UFS) {
                    DbgPrint("[!] File not found");
                    status = STATUS_FILE_INVALID;
                }
                else {
                    fHeadEntry = CONTAINING_RECORD(&dHeadEntry->file, OpenFileEntry, link);
                    fEntry = IsFoundFileEntry(fHeadEntry, &fileName->name);
                    if (fEntry == NULL) {
                        DbgPrint("[!] File not found");
                        status = STATUS_FILE_INVALID;
                    }
                    else {
                        // размер возвращаемых данных
                        if (pIrpStack->Parameters.Read.Length > fEntry->size.QuadPart) {
                            info = fEntry->size.QuadPart;
                        }
                        else {
                            info = pIrpStack->Parameters.Read.Length;
                        }
                        RtlCopyMemory(outputBuffer, fEntry->buffer, info);
                        // изменяем текущее файловое смещение
                        pIrpStack->FileObject->CurrentByteOffset.QuadPart = info;
                    }
                }
                FreeParseEntry(&parseName);
            }
            else {
                DbgPrint("[!] Error parse name");
            }
        }
        else {
            DbgPrint("[!] Error name file");
            status = STATUS_FILE_INVALID;
        }
    }
    else {
        DbgPrint("[!] Error name file");
       status = STATUS_FILE_INVALID;
    }

    return CompleteIrp(pIrp, status, info);
}



//--------------------

//
// Функция обработки запроса на запись
//
NTSTATUS DispatchWrite(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;
    ULONG info = 0;
    PCHAR inputBuffer;
    PWCHAR copyFileName;
    OpenDirEntry* dHeadEntry;
    OpenDirEntry* dHeadEntryTmp;
    OpenFileEntry* fHeadEntry;
    OpenFileEntry* fEntry = NULL;
    ParseNameEntry parseNames;
    ParseNameEntry* fileName;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    // pIrpStack->Parameters.Write.Length
    // pIrpStack->Parameters.Write.Key
    // pIrpStack->Parameters.Write.ByteOffset

    DbgPrint("Write %d bytes to offset %d\n", pIrpStack->Parameters.Write.Length, *((int*)&pIrpStack->Parameters.Write.ByteOffset));

    if (pDeviceObject->Flags & DO_BUFFERED_IO) {
        inputBuffer = (char*)pIrp->AssociatedIrp.SystemBuffer;
    }
    else {
        inputBuffer = (char*)pIrp->UserBuffer;
    }
    //DbgPrint ("Input buffer: %08X\n", inputBuffer);

    if (pIrpStack->FileObject->CurrentByteOffset.QuadPart) {
        return CompleteIrp(pIrp, STATUS_END_OF_FILE, 0);
    }

    if (pIrpStack->FileObject->FileName.Length > 2) {
        if (IsValidStringName(&pIrpStack->FileObject->FileName)) {
            //dHeadEntry = IsFoundDirEntry(&glUniformFileSystem, &pIrpStack->FileObject->FileName);
            ParseName(&pIrpStack->FileObject->FileName, &parseNames);
            PrintParseName(&parseNames);

            dHeadEntry = IsFoundDirEntry2(&glUniformFileSystem, &parseNames);

            fileName = (ParseNameEntry*)CONTAINING_RECORD(parseNames.link.Flink, ParseNameEntry, link);

            if (fileName->name.Length == dHeadEntry->name.Length) {
                if (fileName->name.Length == RtlCompareMemory(fileName->name.Buffer, dHeadEntry->name.Buffer, fileName->name.Length)) {
                    FreeParseEntry(&parseNames);
                    DbgPrint("[!] This name has already been selected for directory");
                    return CompleteIrp(pIrp, STATUS_ACCESS_DENIED, 0);
                }
            }

            DbgPrint("FILENAEM WRITE %d %wZ", fileName->name.Length, &fileName->name);
            DbgPrint("DHEADENTRY %d %wZ", dHeadEntry->name.Length, &dHeadEntry->name);

            if (dHeadEntry->frank == RANK_UFS) {
                InitializeListHead(&dHeadEntry->file);
            }
            else {
                fHeadEntry = CONTAINING_RECORD(&dHeadEntry->file, OpenFileEntry, link);
                //fEntry = IsFoundFileEntry(fHeadEntry, &pIrpStack->FileObject->FileName);
                fEntry = IsFoundFileEntry(fHeadEntry, &fileName->name);
            }

            if (fEntry == NULL) {
                fEntry = (OpenFileEntry*)ExAllocateFromPagedLookasideList(&glPagedFileList);
                InsertTailList(&dHeadEntry->file, &fEntry->link);



                //fEntry->name.Length = pIrpStack->FileObject->FileName.Length + 2;
                //fEntry->name.MaximumLength = fEntry->name.Length;
                //copyFileName = ExAllocatePool(PagedPool, fEntry->name.Length);
                //if (!copyFileName) {
                //    return CompleteIrp(pIrp, STATUS_MEMORY_NOT_ALLOCATED, 0);
                //}
                //fEntry->name.Buffer = copyFileName;

                fEntry->name.Length = fileName->name.Length + 2;
                fEntry->name.MaximumLength = fEntry->name.Length;
                copyFileName = ExAllocatePool(PagedPool, fEntry->name.Length);
                if (!copyFileName) {
                    return CompleteIrp(pIrp, STATUS_MEMORY_NOT_ALLOCATED, 0);
                }
                fEntry->name.Buffer = copyFileName;

                RtlZeroMemory(fEntry->name.Buffer, fEntry->name.Length);


                //RtlCopyUnicodeString(&fEntry->name, &pIrpStack->FileObject->FileName);
                RtlCopyUnicodeString(&fEntry->name, &fileName->name);
                GetSystemTime(&fEntry->creationTime);
                GetSystemTime(&fEntry->changeTime);
                dHeadEntry->frank++;

            }
            else {
                dHeadEntry->size.QuadPart = dHeadEntry->size.QuadPart - fEntry->size.QuadPart;
                GetSystemTime(&fEntry->changeTime);
                ExFreePool(fEntry->buffer);
            }
            info = pIrpStack->Parameters.Write.Length;
            fEntry->size.QuadPart = info;
            fEntry->buffer = (PCHAR)ExAllocatePool(PagedPool, info);
            RtlCopyMemory(fEntry->buffer, inputBuffer, info);
            //strcpy(entry->buffer, inputBuffer);
            dHeadEntry->size.QuadPart = fEntry->size.QuadPart + dHeadEntry->size.QuadPart;


            FreeParseEntry(&parseNames);
        }
        else {
            DbgPrint("[!] Error name file");
            return CompleteIrp(pIrp, STATUS_FILE_INVALID, 0);
        }
    }
    else {
        DbgPrint("[!] Error name file");
        return CompleteIrp(pIrp, STATUS_FILE_INVALID, 0);
    }



    DbgPrint("%s\n", inputBuffer);

    // изменяем текущее файловое смещение
    pIrpStack->FileObject->CurrentByteOffset.QuadPart = pIrpStack->Parameters.Write.Length;


    PrintUFS(&glUniformFileSystem);
    return CompleteIrp(pIrp, status, info);
}


//--------------------

//
// Функция обработки запроса информации о файле
//
NTSTATUS DispatchQueryInformation (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

NTSTATUS status = STATUS_SUCCESS;
PIO_STACK_LOCATION pIrpStack;
ULONG info = 0;
PCHAR outputBuffer;
	
    pIrpStack = IoGetCurrentIrpStackLocation (pIrp);
    // pIrpStack->Parameters.QueryFile.Length
    // pIrpStack->Parameters.QueryFile.FileInformationClass

    if (pDeviceObject->Flags & DO_BUFFERED_IO) {
        outputBuffer = (char*) pIrp->AssociatedIrp.SystemBuffer;
        }
    else {
        outputBuffer = (char*) pIrp->UserBuffer;
        }

    DbgPrint ("Query information device %wZ %d %d\n",
        &pIrpStack->FileObject->FileName,
        pIrpStack->Parameters.QueryFile.FileInformationClass,
        pIrpStack->Parameters.QueryFile.Length);

    // в зависимости от типа запроса заполняем
    // буфер как соответствующую структуру
    switch (pIrpStack->Parameters.QueryFile.FileInformationClass) {

        case FileBasicInformation:
            {
            TIME_FIELDS timeFields;
            LARGE_INTEGER timeStamp;
            FILE_BASIC_INFORMATION *fbi;

            //DbgPrint("!!!!!!!!!!!!! %wZ", &pIrpStack->FileObject->FileName);

            if (pIrpStack->Parameters.QueryFile.Length < sizeof(FILE_BASIC_INFORMATION)) {
                status = STATUS_BUFFER_OVERFLOW;
                break;
                }

            timeFields.Year = 2020;
            timeFields.Month = 06;
            timeFields.Day = 13;
            timeFields.Hour = 1;
            timeFields.Minute = 1;
            timeFields.Second = 0;
            timeFields.Milliseconds = 0;
            timeFields.Weekday = 0;
            RtlTimeFieldsToTime (&timeFields, &timeStamp);

            info = sizeof(FILE_BASIC_INFORMATION);

            fbi = (FILE_BASIC_INFORMATION*) outputBuffer;
            fbi->ChangeTime = timeStamp;
            fbi->CreationTime = timeStamp;
            fbi->LastAccessTime = timeStamp;
            fbi->LastWriteTime = timeStamp;
            fbi->FileAttributes = FILE_ATTRIBUTE_NORMAL;    // для директории надо вернуть FILE_ATTRIBUTE_DIRECTORY
            }
            break;

        case FileStandardInformation:
            {
            FILE_STANDARD_INFORMATION *fsi;

            if (pIrpStack->Parameters.QueryFile.Length < sizeof(FILE_STANDARD_INFORMATION)) {
                status = STATUS_BUFFER_OVERFLOW;
                break;
                }

            info = sizeof(FILE_STANDARD_INFORMATION);
            fsi = (FILE_STANDARD_INFORMATION*) outputBuffer;
            fsi->AllocationSize.QuadPart = 100;
            fsi->EndOfFile.QuadPart = 100;
            fsi->NumberOfLinks = 1;
            fsi->Directory = FALSE;     // для директории должно быть TRUE
            fsi->DeletePending = FALSE;
            }
            break;

        case FileNameInformation:
            {
            PFILE_NAME_INFORMATION fni = (PFILE_NAME_INFORMATION)outputBuffer;
            ULONG fileNameLength = pIrpStack->FileObject->FileName.Length;

            info = sizeof(FILE_NAME_INFORMATION) + fileNameLength;

            if (pIrpStack->Parameters.QueryFile.Length < sizeof(FILE_NAME_INFORMATION) + fileNameLength) {
                status = STATUS_BUFFER_OVERFLOW;
                break;
                }

            fni->FileNameLength = fileNameLength;
            memcpy (fni->FileName, pIrpStack->FileObject->FileName.Buffer, fileNameLength);
            }
            break;

        default:
            status = STATUS_UNSUCCESSFUL;
            break;
        }

    return CompleteIrp (pIrp, status, info);
}


//--------------------

//
// Функция обработки запроса установки информации о файле
//
NTSTATUS DispatchSetInformation (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

NTSTATUS status = STATUS_SUCCESS;
PIO_STACK_LOCATION pIrpStack;
ULONG info = 0;
	
    pIrpStack = IoGetCurrentIrpStackLocation (pIrp);
    // pIrpStack->Parameters.SetFile.Length
    // pIrpStack->Parameters.SetFile.FileInformationClass
    // pIrpStack->Parameters.SetFile.FileObject


    return CompleteIrp (pIrp, status, info);
}


//--------------------

// основная функция обработки всех ioctl-запросов
NTSTATUS DispatchControl (IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp) {

NTSTATUS status = STATUS_SUCCESS;
PIO_STACK_LOCATION pIrpStack;
ULONG	Info = 0;
ULONG   inlen;          // размер входного буфера
ULONG   outlen;         // размер выходного буфера
ULONG   len;
unsigned char *in;      // входной буфер
unsigned char *out;     // выходной буфер
ULONG   ioctl;          // ioctl-код
char*   buf;            // вспомогательный буфер
ULONG   i;              // счётчик цикла
	
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    // получаем ioctl-код
    ioctl = pIrpStack->Parameters.DeviceIoControl.IoControlCode;

    // размер входного буфера
    inlen = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;

    // размер выходного буфера
    outlen = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    // проверяем тип ввода/вывода, ассоциированный с ioctl-кодом
    if( (ioctl & 0x00000003) == METHOD_BUFFERED ){
        // если буферизованный
        // то системный буфер является и входным и выходным
        out = in = pIrp->AssociatedIrp.SystemBuffer;
        }
    else{
        // иначе получаем указатели из соответствующих полей IPR
        in = pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
        out = pIrp->UserBuffer;
        }

    switch(ioctl){

        default:
            status = STATUS_INVALID_PARAMETER;
            break;
        
        }


    return CompleteIrp (pIrp, status, Info);
}


//--------------------

ULONG ProduceFileBothDirectoryInformationFile(
    OpenFileEntry* filename,
    ULONG fileIndex,
    ULONG attributes,
    PFILE_BOTH_DIR_INFORMATION dirInfo,
    ULONG outputBufferSize) {

    ULONG minSize = sizeof(FILE_BOTH_DIR_INFORMATION) + filename->name.Length;
    TIME_FIELDS timeFields;
    LARGE_INTEGER timeStamp;

    if (outputBufferSize < minSize) {
        return 0;
    }

    timeFields.Year = 2020;
    timeFields.Month = 01;
    timeFields.Day = 01;
    timeFields.Hour = 1;
    timeFields.Minute = 1;
    timeFields.Second = 0;
    timeFields.Milliseconds = 0;
    timeFields.Weekday = 0;
    RtlTimeFieldsToTime(&timeFields, &timeStamp);

    dirInfo->FileIndex = fileIndex;
    RtlTimeFieldsToTime(&filename->creationTime, &timeStamp);
    dirInfo->CreationTime = timeStamp;
    dirInfo->LastAccessTime = timeStamp;
    dirInfo->LastWriteTime = timeStamp;

    RtlTimeFieldsToTime(&filename->changeTime, &timeStamp);
    dirInfo->ChangeTime = timeStamp;
    dirInfo->FileAttributes = attributes;
    dirInfo->EndOfFile = filename->size;
    dirInfo->AllocationSize = filename->size;
    dirInfo->FileNameLength = filename->name.Length;
    wcscpy(dirInfo->ShortName, filename->name.Buffer);
    wcscpy(dirInfo->FileName, filename->name.Buffer);

    dirInfo->NextEntryOffset = minSize;

    return minSize;
}

ULONG ProduceFileBothDirectoryInformationDir(
    OpenDirEntry* filename,
    ULONG fileIndex,
    ULONG attributes,
    PFILE_BOTH_DIR_INFORMATION dirInfo,
    ULONG outputBufferSize) {

    ULONG minSize = sizeof(FILE_BOTH_DIR_INFORMATION) + filename->name.Length;
    TIME_FIELDS timeFields;
    LARGE_INTEGER timeStamp;

    if (outputBufferSize < minSize) {
        return 0;
    }

    timeFields.Year = 2020;
    timeFields.Month = 01;
    timeFields.Day = 01;
    timeFields.Hour = 1;
    timeFields.Minute = 1;
    timeFields.Second = 0;
    timeFields.Milliseconds = 0;
    timeFields.Weekday = 0;
    RtlTimeFieldsToTime(&timeFields, &timeStamp);

    dirInfo->FileIndex = fileIndex;
    RtlTimeFieldsToTime(&filename->creationTime, &timeStamp);
    dirInfo->CreationTime = timeStamp;
    dirInfo->LastAccessTime = timeStamp;
    dirInfo->LastWriteTime = timeStamp;

    RtlTimeFieldsToTime(&filename->changeTime, &timeStamp);
    dirInfo->ChangeTime = timeStamp;
    dirInfo->FileAttributes = attributes;
    dirInfo->EndOfFile = filename->size;
    dirInfo->AllocationSize = filename->size;
    dirInfo->FileNameLength = filename->name.Length;
    wcscpy(dirInfo->ShortName, filename->name.Buffer);
    wcscpy(dirInfo->FileName, filename->name.Buffer);

    dirInfo->NextEntryOffset = minSize;

    return minSize;
}


NTSTATUS ProcessingQueryDirectory (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

NTSTATUS status = STATUS_SUCCESS;
PIO_STACK_LOCATION pIrpStack;
ULONG info = 0;
PUCHAR outputBuffer;
ULONG outputBufferSize;
ULONG count = 0;
PFILE_OBJECT pFileObject;

ParseNameEntry parseNames;
ParseNameEntry pParseNames;
OpenDirEntry* dHeadEntry;
OpenDirEntry* dHeadEntryFor;
OpenFileEntry* fHeadEntryFor;
OpenDirEntry* dEntry;
OpenFileEntry* fEntry;
PLIST_ENTRY link;

PFILE_BOTH_DIR_INFORMATION dirInfo;
ULONG offset = 0;
ULONG ind = 0;
ULONG outSize = 0;
BOOLEAN flag = TRUE;


    pIrpStack = IoGetCurrentIrpStackLocation (pIrp);
    outputBufferSize = pIrpStack->Parameters.QueryDirectory.Length;
    pFileObject = pIrpStack->FileObject;

    if (pDeviceObject->Flags & DO_BUFFERED_IO) {
        outputBuffer = (char*) pIrp->AssociatedIrp.SystemBuffer;
        }
    else {
        outputBuffer = (char*) pIrp->UserBuffer;
        }
    outputBufferSize = pIrpStack->Parameters.QueryDirectory.Length;
    // Допустимые параметры запроса
    // pIrpStack->QueryDirectory.Length
    // pIrpStack->QueryDirectory.FileName
    // pIrpStack->QueryDirectory.FileInformationClass
    // pIrpStack->QueryDirectory.FileIndex

    // для IRP_MN_QUERY_DIRECTORY допустимые флаги в pIrp->Flags
    // SL_RESTART_SCAN      вернуть записи, начиная с первой (иначе продолжить с того места,
    //                      на котором остановились на предыдущем запросе с этим FILE_OBJECT)
    // SL_RETURN_SINGLE_ENTRY   вернуть только одну запись
    // SL_INDEX_SPECIFIED   выводить записи с указанного индекса (pIrpStack->FileIndex)

    DbgPrint ("Directory control %wZ (%wZ) %d %d %d %d %d %d\n", 
        &pFileObject->FileName,
        pIrpStack->Parameters.QueryDirectory.FileName,
        pIrpStack->Parameters.QueryDirectory.FileInformationClass,
        pIrpStack->Parameters.QueryDirectory.FileIndex,
        !!FlagOn (pIrpStack->Flags, SL_RESTART_SCAN),
        !!FlagOn (pIrpStack->Flags, SL_RETURN_SINGLE_ENTRY),
        !!FlagOn (pIrpStack->Flags, SL_INDEX_SPECIFIED),
        outputBufferSize);

    //if (FlagOn (pIrp->Flags, SL_RESTART_SCAN)) {
    //    pFileObject->CurrentByteOffset.QuadPart = 0;
    //    }

    if (pFileObject->CurrentByteOffset.QuadPart >= 1) { //FILES_COUNT
        return CompleteIrp(pIrp, STATUS_NO_MORE_FILES, 0);
    }

    switch (pIrpStack->Parameters.QueryDirectory.FileInformationClass) {
    case FileBothDirectoryInformation:
        if (FlagOn(pIrpStack->Flags, SL_RETURN_SINGLE_ENTRY)) { // <-------------------------
            DbgPrint("ONE");
            dHeadEntry = &glUniformFileSystem;
            if (pFileObject->FileName.Length > 2) {
                status = ParseName(&pFileObject->FileName, &parseNames);
                if (!NT_SUCCESS(status)) {
                    DbgPrint("[!] Error parse names");
                    return CompleteIrp(pIrp, status, info);
                }
                PrintParseName(&parseNames);
                dHeadEntry = IsFoundDirEntry2(&glUniformFileSystem, &parseNames);
            }
            dirInfo = (PFILE_BOTH_DIR_INFORMATION)outputBuffer;
            RtlZeroMemory(dirInfo, outputBufferSize);
            
            
            DbgPrint("HEAD DIR ENTRY ---> %wZ", &dHeadEntry->name);

            if(pIrpStack->Parameters.QueryDirectory.FileName &&
                pIrpStack->Parameters.QueryDirectory.FileName->Buffer &&
                (RtlCompareMemory(pIrpStack->Parameters.QueryDirectory.FileName->Buffer, L"*", sizeof(WCHAR)) != sizeof(WCHAR))) {
                if (dHeadEntry->drank > RANK_UFS) {
                    DbgPrint("---->1");
                    for (link = dHeadEntry->dir.Flink; link != &dHeadEntry->dir; link = link->Flink) {
                        DbgPrint("---->2");
                        dEntry = CONTAINING_RECORD(link, OpenDirEntry, link);
                        if (dEntry->name.Length == pIrpStack->Parameters.QueryDirectory.FileName->Length) {
                            if (dEntry->name.Length == RtlCompareMemory(dEntry->name.Buffer, pIrpStack->Parameters.QueryDirectory.FileName->Buffer, dEntry->name.Length)) {
                                info = ProduceFileBothDirectoryInformationDir(
                                    dEntry,
                                    0,
                                    FILE_ATTRIBUTE_DIRECTORY,
                                    dirInfo,
                                    outputBufferSize
                                );
                                dirInfo->NextEntryOffset = 0;
                                return CompleteIrp(pIrp, status, info);
                            }
                        }
                    }
                }
                if (dHeadEntry->frank > RANK_UFS) {
                    fHeadEntryFor = CONTAINING_RECORD(&dHeadEntry->file, OpenFileEntry, link);
                    fEntry = IsFoundFileEntry(fHeadEntryFor, pIrpStack->Parameters.QueryDirectory.FileName);
                    if (fEntry) {
                        info = ProduceFileBothDirectoryInformationFile(
                            fEntry,
                            0,
                            FILE_ATTRIBUTE_NORMAL,
                            dirInfo,
                            outputBufferSize
                        );
                        dirInfo->NextEntryOffset = 0;
                        return CompleteIrp(pIrp, status, info);
                    }
                    else if (dHeadEntry->drank == RANK_UFS) {
                        status = STATUS_NO_SUCH_FILE;
                    }
                }
            }
            else {

                if (dHeadEntry->drank > RANK_UFS) {
                    dEntry = CONTAINING_RECORD(dHeadEntry->dir.Flink, OpenDirEntry, link);
                    info = ProduceFileBothDirectoryInformationDir(
                        dEntry,
                        0,
                        FILE_ATTRIBUTE_DIRECTORY,
                        dirInfo,
                        outputBufferSize
                    );
                }
                else if (dHeadEntry->frank > RANK_UFS) {
                    fEntry = CONTAINING_RECORD(dHeadEntry->file.Flink, OpenFileEntry, link);
                    info = ProduceFileBothDirectoryInformationFile(
                        fEntry,
                        0,
                        FILE_ATTRIBUTE_NORMAL,
                        dirInfo,
                        outputBufferSize
                    );
                }
            }
            
            dirInfo->NextEntryOffset = 0;

        }
        else if (FlagOn(pIrpStack->Flags, SL_RESTART_SCAN)) { // <---------------------------
            DbgPrint("TWO");
        }
        else if (FlagOn(pIrpStack->Flags, SL_INDEX_SPECIFIED)) { // <---------------------------
            DbgPrint("THREE");
        }
        else {
            DbgPrint("OUT");
            dHeadEntry = &glUniformFileSystem;
            if (pFileObject->FileName.Length > 2) {
                status = ParseName(&pFileObject->FileName, &parseNames);
                if (!NT_SUCCESS(status)) {
                    DbgPrint("[!] Error parse names");
                    return CompleteIrp(pIrp, status, info);
                }

                PrintParseName(&parseNames);
                dHeadEntry = IsFoundDirEntry2(&glUniformFileSystem, &parseNames);
            }

            dirInfo = (PFILE_BOTH_DIR_INFORMATION)outputBuffer;
            RtlZeroMemory(dirInfo, outputBufferSize);


            if (dHeadEntry->drank > RANK_UFS) {
                dHeadEntryFor = (OpenDirEntry*)CONTAINING_RECORD(&dHeadEntry->dir, OpenDirEntry, link);
                for (link = dHeadEntryFor->link.Flink; link != &dHeadEntryFor->link; link = link->Flink) {
                    if (ind > 0) {
                        dEntry = (OpenDirEntry*)CONTAINING_RECORD(link, OpenDirEntry, link);
                        DbgPrint("[*] DIR  SELECT %p %wZ", link, &dEntry->name);
                        offset = ProduceFileBothDirectoryInformationDir(
                            dEntry,
                            ind++,
                            FILE_ATTRIBUTE_DIRECTORY,
                            dirInfo,
                            outputBufferSize - info
                        );
                        info += offset;
                        if (link->Flink != &dHeadEntryFor->link) {
                            dirInfo = (PFILE_BOTH_DIR_INFORMATION)((PUCHAR)dirInfo + offset);
                        }
                    }
                    else ind++;
                }
                if (dHeadEntry->frank > RANK_UFS) {
                    dirInfo = (PFILE_BOTH_DIR_INFORMATION)((PUCHAR)dirInfo + offset);
                }
            }
            if (dHeadEntry->frank > RANK_UFS) {
                fHeadEntryFor = (OpenFileEntry*)CONTAINING_RECORD(&dHeadEntry->file, OpenFileEntry, link);
                for (link = fHeadEntryFor->link.Flink; link != &fHeadEntryFor->link; link = link->Flink) {
                    if (ind > 0) {
                        fEntry = (OpenFileEntry*)CONTAINING_RECORD(link, OpenFileEntry, link);
                        DbgPrint("[*] FILE SELECT %p %wZ", link, &fEntry->name);
                        offset = ProduceFileBothDirectoryInformationFile(
                            fEntry, //file
                            ind++,
                            FILE_ATTRIBUTE_NORMAL,
                            dirInfo,
                            outputBufferSize - info
                        );
                        info += offset;
                        if (link->Flink != &fHeadEntryFor->link) {
                            dirInfo = (PFILE_BOTH_DIR_INFORMATION)((PUCHAR)dirInfo + offset);
                        }
                    }
                    else ind++;
                }

            }
            dirInfo->NextEntryOffset = 0;

            pFileObject->CurrentByteOffset.QuadPart++;
        }
        if (!info) {
            status = STATUS_NO_SUCH_FILE;
            //status = STATUS_BUFFER_OVERFLOW;
            //info = outputBufferSize + 10;
        }
        //else {
        //    pFileObject->CurrentByteOffset.QuadPart += count - 1;
        //}

        if (pFileObject->FileName.Length > 2) {
            FreeParseEntry(&parseNames);
        }
        DbgPrint("[*] Send bytes %d\n", info);
        break;
    }


    return CompleteIrp (pIrp, status, info);
}


NTSTATUS DispatchDirectoryControl (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

PIO_STACK_LOCATION pIrpStack;

    pIrpStack = IoGetCurrentIrpStackLocation (pIrp);
    
    // Допустимые значения для pIrpStack->MinorFunction
    // IRP_MN_QUERY_DIRECTORY   запрос информации о каталоге
    // IRP_MN_NOTIFY_CHANGE_DIRECTORY   запрос информации об изменениях

    switch (pIrpStack->MinorFunction) {
        case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
            return CompleteIrp (pIrp, STATUS_INVALID_PARAMETER, 0);

        case IRP_MN_QUERY_DIRECTORY:
            return ProcessingQueryDirectory (pDeviceObject, pIrp);

        default:
            return CompleteIrp (pIrp, STATUS_INVALID_PARAMETER, 0);
        }

}


//--------------------


NTSTATUS DispatchVolumeQueryInformation (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {
NTSTATUS status = STATUS_SUCCESS;
PIO_STACK_LOCATION pIrpStack;
ULONG info = 0;
PUCHAR outputBuffer;
ULONG outputBufferSize;
PFILE_OBJECT pFileObject;

    pIrpStack = IoGetCurrentIrpStackLocation (pIrp);
    if (pDeviceObject->Flags & DO_BUFFERED_IO) {
        // если для устройства определён буферизованный ввод/вывод,
        // то записываем данные в системный буфер
        outputBuffer = (char*) pIrp->AssociatedIrp.SystemBuffer;
        }
    else {
        // иначе непосредственно в пользовательский буфер
        outputBuffer = (char*) pIrp->UserBuffer;
        }
    outputBufferSize = pIrpStack->Parameters.QueryVolume.Length;
    pFileObject = pIrpStack->FileObject;

    DbgPrint ("Volume query %wZ %d %d %d\n", 
        &pFileObject->FileName,
        pIrpStack->MinorFunction,
        pIrpStack->Parameters.QueryVolume.FsInformationClass,
        outputBufferSize);


    switch (pIrpStack->Parameters.QueryVolume.FsInformationClass) {
        case FileFsVolumeInformation:
            {
            PFILE_FS_VOLUME_INFORMATION pVolumeInfo = (PFILE_FS_VOLUME_INFORMATION)outputBuffer;
            ULONG volumeLabelLength = sizeof(WCHAR) * wcslen (VOLUME_LABEL);
            ULONG minSize = sizeof(FILE_FS_VOLUME_INFORMATION) + volumeLabelLength;
            
            info = minSize;

            if (outputBufferSize < minSize) {
                status = STATUS_BUFFER_OVERFLOW;
                break;
                }

            pVolumeInfo->VolumeCreationTime.QuadPart = 1000000;
            pVolumeInfo->VolumeSerialNumber = 1234;
            pVolumeInfo->VolumeLabelLength = volumeLabelLength;
            pVolumeInfo->SupportsObjects = FALSE;
            wcscpy (pVolumeInfo->VolumeLabel, VOLUME_LABEL);
            }
            break;

        case FileFsAttributeInformation:
            {
            PFILE_FS_ATTRIBUTE_INFORMATION pVolumeInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)outputBuffer;
            ULONG fileSystemNameLength = sizeof(WCHAR) * wcslen (FS_NAME);
            ULONG minSize = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + fileSystemNameLength;
            
            info = minSize;

            if (outputBufferSize < minSize) {
                status = STATUS_BUFFER_OVERFLOW;
                break;
                }

            pVolumeInfo->FileSystemAttributes = 0;
            pVolumeInfo->MaximumComponentNameLength = 100;  // максимальная длина одного имени в пути
            pVolumeInfo->FileSystemNameLength = fileSystemNameLength;
            wcscpy (pVolumeInfo->FileSystemName, FS_NAME);
            
            }
            break;

        case FileFsFullSizeInformation:
            {
            PFILE_FS_FULL_SIZE_INFORMATION pVolumeInfo = (PFILE_FS_FULL_SIZE_INFORMATION)outputBuffer;
            ULONG minSize = sizeof(FILE_FS_FULL_SIZE_INFORMATION);
            
            info = minSize;

            if (outputBufferSize < minSize) {
                status = STATUS_BUFFER_OVERFLOW;
                break;
                }

            pVolumeInfo->TotalAllocationUnits.QuadPart = 100;
            pVolumeInfo->CallerAvailableAllocationUnits.QuadPart = 50;
            pVolumeInfo->ActualAvailableAllocationUnits.QuadPart = 50;
            pVolumeInfo->SectorsPerAllocationUnit = 8;
            pVolumeInfo->BytesPerSector = 512;
            
            }
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            break;
        }

    return CompleteIrp (pIrp, status, info);
}


//--------------------


NTSTATUS DispatchQuerySecurity (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {

    return CompleteIrp (pIrp, STATUS_UNSUCCESSFUL, 0);
}

//--------------------

NTSTATUS DispatchIrp (IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp) {

NTSTATUS status = STATUS_SUCCESS;
PIO_STACK_LOCATION pIrpStack;
ULONG info = 0;
UCHAR majorFunction;
char *majorFunctionsNames[] = {
    "IRP_MJ_CREATE",
    "IRP_MJ_CREATE_NAMED_PIPE",
    "IRP_MJ_CLOSE",
    "IRP_MJ_READ",
    "IRP_MJ_WRITE",
    "IRP_MJ_QUERY_INFORMATION",
    "IRP_MJ_SET_INFORMATION",
    "IRP_MJ_QUERY_EA",
    "IRP_MJ_SET_EA",
    "IRP_MJ_FLUSH_BUFFERS",
    "IRP_MJ_QUERY_VOLUME_INFORMATION",
    "IRP_MJ_SET_VOLUME_INFORMATION",
    "IRP_MJ_DIRECTORY_CONTROL",
    "IRP_MJ_FILE_SYSTEM_CONTROL",
    "IRP_MJ_DEVICE_CONTROL",
    "IRP_MJ_INTERNAL_DEVICE_CONTROL",
    "IRP_MJ_SHUTDOWN",
    "IRP_MJ_LOCK_CONTROL",
    "IRP_MJ_CLEANUP",
    "IRP_MJ_CREATE_MAILSLOT",
    "IRP_MJ_QUERY_SECURITY",
    "IRP_MJ_SET_SECURITY",
    "IRP_MJ_POWER",
    "IRP_MJ_SYSTEM_CONTROL",
    "IRP_MJ_DEVICE_CHANGE",
    "IRP_MJ_QUERY_QUOTA",
    "IRP_MJ_SET_QUOTA",
    "IRP_MJ_PNP"};


    pIrpStack = IoGetCurrentIrpStackLocation (pIrp);
    majorFunction = pIrpStack->MajorFunction;


    DbgPrint ("IPR %s(%d)\n", majorFunctionsNames[majorFunction], majorFunction);

    return CompleteIrp (pIrp, status, info); // Завершение IRP
}


//--------------------

// функция завершения операции с IRP-пакетом
NTSTATUS CompleteIrp (PIRP Irp, NTSTATUS status,ULONG Info) {

    Irp->IoStatus.Status = status;		// статус завершении операции
    Irp->IoStatus.Information = Info;	// количество возращаемой информации
    IoCompleteRequest (Irp, IO_NO_INCREMENT);	// завершение операции ввода-вывода
    return status;
}

//--------------------


void GetSystemTime(PTIME_FIELDS time) {

    LARGE_INTEGER SystemTime;
    LARGE_INTEGER LocalTime;

    KeQuerySystemTime(&SystemTime);
    ExSystemTimeToLocalTime(&SystemTime, &LocalTime);
    RtlTimeToTimeFields(&LocalTime, time);

    return;
}


//--------------------

//--------------------

NTSTATUS DispathCreateDirectory2(ParseNameEntry* names) {

    NTSTATUS status = STATUS_SUCCESS;
    PWCHAR copyDirName;
    ParseNameEntry* nEntry;
    OpenDirEntry* dEntry;
    OpenFileEntry* fEntry;
    OpenDirEntry* dHeadEntry = IsFoundDirEntry2(&glUniformFileSystem, names);

    nEntry = (ParseNameEntry*)CONTAINING_RECORD(names->link.Flink, ParseNameEntry, link);
    if (dHeadEntry) {
        if (dHeadEntry->frank > RANK_UFS) {
            fEntry = CONTAINING_RECORD(&dHeadEntry->file, OpenFileEntry, link);
            fEntry = IsFoundFileEntry(fEntry, &nEntry->name);
            if (fEntry) {
                return STATUS_FILE_IS_A_DIRECTORY;
            }

        }
        if (dHeadEntry->name.Length == nEntry->name.Length) {
            if (dHeadEntry->name.Length == RtlCompareMemory(dHeadEntry->name.Buffer, nEntry->name.Buffer, dHeadEntry->name.Length)) {
                return STATUS_ACCESS_DENIED;
            }
        }

        //DbgPrint("DHEADENTRY ---- %wZ", &dHeadEntry->name);

        if (dHeadEntry->drank == RANK_UFS) {
            InitializeListHead(&dHeadEntry->dir);
        }
        dEntry = (OpenDirEntry*)ExAllocateFromPagedLookasideList(&glPagedDirList);

        dEntry->name.Length = nEntry->name.Length;
        dEntry->name.MaximumLength = dEntry->name.Length;
        copyDirName = ExAllocatePool(PagedPool, dEntry->name.Length);
        if (!copyDirName) {
            return STATUS_MEMORY_NOT_ALLOCATED;
        }
        dEntry->name.Buffer = copyDirName;

        RtlZeroMemory(dEntry->name.Buffer, dEntry->name.Length);
        RtlCopyUnicodeString(&dEntry->name, &nEntry->name);


        GetSystemTime(&dEntry->creationTime);
        GetSystemTime(&dEntry->changeTime);
        dEntry->drank = RANK_UFS;
        dEntry->frank = RANK_UFS;
        dEntry->size.QuadPart = 0;
        dHeadEntry->drank++;

        //DbgPrint("HEad %wZ", &dHeadEntry->name);
        InsertTailList(&dHeadEntry->dir, &dEntry->link);

        DbgPrint("[*] Successful create directory %wZ", &dEntry->name);
        PrintUFS(&glUniformFileSystem);
    }
    else {
        DbgPrint("[!] Error create dir %wZ", &nEntry->name);
        status = STATUS_ACCESS_DENIED;
    }
    return status;
}

//--------------------
