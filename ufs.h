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
    LARGE_INTEGER size;             // суммарный размер вложенных файлов и вложенных директорий
    TIME_FIELDS creationTime;
    TIME_FIELDS changeTime;

    ULONG drank;                    // степень вложенности (инициализируется RANK_UFS в корне UFS)
    LIST_ENTRY dir;                 // голова списка вложенных директорий
    ULONG frank;
    LIST_ENTRY file;                // голова списка вложенных файлов

    LIST_ENTRY link;                // ссылки на каталоги одного уровня (поля ссылок инициализируются NULL в корне UFS)
} OpenDirEntry;

typedef struct _ParseNameEntry {
    UNICODE_STRING name;
    LIST_ENTRY link;
} ParseNameEntry;

OpenDirEntry glUniformFileSystem;   // корень файловой системы
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