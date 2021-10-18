#ifndef _UFS_H
#define _UFS_H


#define RANK_UFS        0x00000000
#define PRINTTIME(str,tf)  DbgPrint("%s %02d.%02d.%04d  %02d:%02d:%02d",\
        str,tf.Day,tf.Month,tf.Year,tf.Hour,tf.Minute,tf.Second);

#include <ntddk.h>



PAGED_LOOKASIDE_LIST glPagedFileList;
PAGED_LOOKASIDE_LIST glPagedDirList;
PAGED_LOOKASIDE_LIST glPagedParseEntry;

typedef struct _OpenFileEntry {
    UNICODE_STRING name;
    LARGE_INTEGER size;
    PCHAR buffer;
    TIME_FIELDS creationTime;
    TIME_FIELDS changeTime;

    LIST_ENTRY link;
} OpenFileEntry;

typedef struct _OpenDirEntry {
    UNICODE_STRING name;
    LARGE_INTEGER size;             // ��������� ������ ��������� ������ � ��������� ����������
    TIME_FIELDS creationTime;
    TIME_FIELDS changeTime;

    ULONG drank;                    // ������� ����������� (���������������� RANK_UFS � ����� UFS)
    LIST_ENTRY dir;                 // ������ ������ ��������� ����������
    ULONG frank;
    LIST_ENTRY file;                // ������ ������ ��������� ������

    LIST_ENTRY link;                // ������ �� �������� ������ ������ (���� ������ ���������������� NULL � ����� UFS)
} OpenDirEntry;

typedef struct _ParseNameEntry {
    UNICODE_STRING name;
    LIST_ENTRY link;
} ParseNameEntry;

OpenDirEntry glUniformFileSystem;   // ������ �������� �������
LIST_ENTRY glParseEntry;


void PrintUFS(OpenDirEntry* glUFS);
void PrintFileList(OpenDirEntry* dEntry);

void FreeUFS(OpenDirEntry* glUFS);
OpenFileEntry* IsFoundFileEntry(OpenFileEntry* fEntry, PUNICODE_STRING filename);
OpenDirEntry* IsFoundDirEntry(OpenDirEntry* glUFS, PUNICODE_STRING filename);
OpenDirEntry* IsFoundDirEntry2(OpenDirEntry* glUFS, ParseNameEntry* names);
BOOLEAN IsFoundFileUFS(OpenDirEntry* glUFS, PUNICODE_STRING name);
NTSTATUS ParseName(IN PUNICODE_STRING path, OUT ParseNameEntry* names);
void PrintParseName(ParseNameEntry* names);
void FreeParseEntry(IN ParseNameEntry* nemas);
SIZE_T CountList(PLIST_ENTRY names);


#endif // !_UFS_H