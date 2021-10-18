#include <ntddk.h>
#include "ufs.h"


void PrintUFS(OpenDirEntry* glUFS) {

    PLIST_ENTRY link;
    OpenDirEntry* dEntry;

    KdPrint(("[outdir]~~~~~~~~~~~~~~~~~~~~~~~~~~~>"));
    KdPrint(("[outdir] Rank D:%d F:%d", glUFS->drank, glUFS->frank));
    KdPrint(("[outdir] Directory name %wZ", &glUFS->name));
    KdPrint(("[outdir] Size directory %I64d", glUFS->size.QuadPart));
    PRINTTIME("[outdir] Creation time", glUFS->creationTime);
    PRINTTIME("[outdir] Change time", glUFS->changeTime);

    PrintFileList(glUFS);

    if (glUFS->drank > RANK_UFS) {
        for (link = glUFS->dir.Flink; link != &glUFS->dir; link = link->Flink) {
            dEntry = CONTAINING_RECORD(link, OpenDirEntry, link);
            PrintUFS(dEntry);
        }
    }

    return;
}

void PrintFileList(OpenDirEntry* dEntry) {

    if (dEntry->frank > RANK_UFS) {

        PLIST_ENTRY link;
        OpenFileEntry* fEntry;

        for (link = dEntry->file.Flink; link != &dEntry->file; link = link->Flink) {

            fEntry = CONTAINING_RECORD(link, OpenFileEntry, link);

            KdPrint(("[outfile] Name file %wZ", &fEntry->name));
            //KdPrint(("SIZE %I64d", entry->fileSize));
            KdPrint(("[outfile] Size %I64d", fEntry->size.QuadPart));
            KdPrint(("[outfile] Buffer %s", fEntry->buffer));
            PRINTTIME("[outfile] Creation time", fEntry->creationTime);
            PRINTTIME("[outfile] Change time", fEntry->changeTime);
        }
    }

    return;
}

BOOLEAN IsFoundFileUFS(OpenDirEntry* glUFS, PUNICODE_STRING name) {

    OpenDirEntry* dHeadEntry;
    OpenFileEntry* fHeadEntry;
    OpenFileEntry* fEntry;

    dHeadEntry = IsFoundDirEntry(glUFS, name);

    if (dHeadEntry->frank > RANK_UFS) {
        fHeadEntry = CONTAINING_RECORD(&dHeadEntry->file, OpenFileEntry, link);
        fEntry = IsFoundFileEntry(fHeadEntry, name);
        if (fEntry) {
            return TRUE;
        }
    }


    return FALSE;
}


OpenFileEntry* IsFoundFileEntry(OpenFileEntry* fEntry, PUNICODE_STRING filename) {

    PLIST_ENTRY link;
    OpenFileEntry* entry;

    for (link = fEntry->link.Flink; link != &fEntry->link; link = link->Flink) {
        entry = CONTAINING_RECORD(link, OpenFileEntry, link);
        //KdPrint(("[*] Strlen name found %d %d", filename->Length, entry->name.Length));
        if (entry->name.Length == filename->Length) {
            if (entry->name.Length == RtlCompareMemory(entry->name.Buffer, filename->Buffer, entry->name.Length)) {
                return entry;
            }
        }
    }

    return NULL;
}


OpenDirEntry* IsFoundDirEntry(OpenDirEntry* glUFS, PUNICODE_STRING filename) {

    PLIST_ENTRY link;
    OpenDirEntry* entry;

    if (glUFS->drank > RANK_UFS) {
        for (link = glUFS->dir.Flink; link != &glUFS->dir; link = link->Flink) {
            entry = CONTAINING_RECORD(link, OpenDirEntry, link);
            //DbgPrint("!!!!!!!!!!! %wZ %d (%wZ) %d", &entry->name, entry->name.Length, filename, filename->Length);
            
            if (filename->Length == entry->name.Length) {
                if (entry->name.Length ==
                    RtlCompareMemory(entry->name.Buffer, filename->Buffer, entry->name.Length)) {
                    if (entry->drank > RANK_UFS) return IsFoundDirEntry(entry, filename);
                    else NULL;
                }
            }
            
        }
    }
    return glUFS;
}


OpenDirEntry* IsFoundDirEntry2(OpenDirEntry* glUFS, ParseNameEntry* names) {

    PLIST_ENTRY dLink;
    PLIST_ENTRY nLink;
    OpenDirEntry* dEntry = glUFS;
    OpenDirEntry* dHeadEntry = glUFS;
    OpenDirEntry* dOutEntry = glUFS;
    ParseNameEntry* nEntry;

    if (!IsListEmpty(&names->link)) {

        nLink = names->link.Blink;

        if (glUFS->drank > RANK_UFS) {
            nLink = names->link.Blink;
            dLink = glUFS->dir.Flink;
            while (dLink != &dHeadEntry->dir && nLink != &names->link) {
                dEntry = CONTAINING_RECORD(dLink, OpenDirEntry, link);
                nEntry = CONTAINING_RECORD(nLink, ParseNameEntry, link);
                //DbgPrint("[NAME PARSE] %wZ", &entry->name);

                //DbgPrint("111111111111 NAME %d %wZ", nEntry->name.Length, &nEntry->name);
                //DbgPrint("111111111111 DIRUFS %d %wZ", dEntry->name.Length, &dEntry->name);

                if (dEntry->name.Length == nEntry->name.Length) {
                    //DbgPrint("!!!!");
                    if (RtlCompareMemory(dEntry->name.Buffer, nEntry->name.Buffer, dEntry->name.Length) == dEntry->name.Length) {
                        //DbgPrint("????");

                        dOutEntry = dEntry;
                        nLink = nLink->Blink;
                        if (dEntry->drank > RANK_UFS) {
                            dHeadEntry = dEntry;
                            dLink = dEntry->dir.Flink;
                        }
                        else {
                            //DbgPrint("%p %p %p %p %p", &names->link, nLink, nLink->Flink, nLink->Blink, nLink->Blink->Blink);
                            return dEntry;
                        }
                    }
                    else {
                        dLink = dLink->Flink;
                    }
                }
                else {
                    dLink = dLink->Flink;
                }
            }
        }
        //if (nLink->Blink != &names->link) {
        //    return NULL;
        //}
        return dOutEntry;
    }
    else {
        return NULL;
    }
}


void FreeUFS(OpenDirEntry* glUFS) {

    PLIST_ENTRY pLink;
    OpenFileEntry* fEntry;
    OpenDirEntry* dEntry;
    
    if (glUFS->frank > RANK_UFS) {
        while (!IsListEmpty(&glUFS->file)) {
            pLink = RemoveHeadList(&glUFS->file);
            fEntry = CONTAINING_RECORD(pLink, OpenFileEntry, link);
            ExFreePool(fEntry->buffer);
            //RtlFreeAnsiString(&fEntry->name);
            ExFreeToPagedLookasideList(&glPagedFileList, fEntry);
        }
    }
    if (glUFS->drank > RANK_UFS) {
        while (!IsListEmpty(&glUFS->dir)) {
            pLink = RemoveHeadList(&glUFS->dir);
            dEntry = CONTAINING_RECORD(pLink, OpenDirEntry, link);
            if (dEntry->drank > RANK_UFS) {
                FreeUFS(dEntry);
                dEntry->drank--;
            }
            ExFreePool(&dEntry->name.Buffer);
            //RtlFreeAnsiString(&dEntry->name);
            ExFreeToPagedLookasideList(&glPagedDirList, dEntry);
        }
    }


    return;
}

// \/:*?"<>|

NTSTATUS ParseName(IN PUNICODE_STRING path, OUT ParseNameEntry* names) {

    WCHAR* p;
    WCHAR* offset = NULL;
    PWCHAR buff;
    SIZE_T flag = 0;
    BOOLEAN last = FALSE;
    SIZE_T i = 0;
    ParseNameEntry* entry;

    if (path->Buffer && *(path->Buffer)) {
        InitializeListHead(&names->link);
        p = path->Buffer;

        //DbgPrint("9999999999 %wZ", path);
        while ((p < path->Buffer + (path->Length / sizeof(WCHAR))) && i < 256) {
            if (*p == L'\\') {
                flag++;
                if (flag == 2) {
                    entry = (ParseNameEntry*)ExAllocateFromPagedLookasideList(&glPagedParseEntry);
                    InsertHeadList(&names->link, &entry->link);

                    entry->name.Length = (p - offset) * sizeof(WCHAR);
                    entry->name.MaximumLength = entry->name.Length;
                    buff = ExAllocatePool(PagedPool, entry->name.Length);
                    if (!buff) {
                        return STATUS_MEMORY_NOT_ALLOCATED;
                    }
                    entry->name.Buffer = buff;
                    RtlCopyMemory(entry->name.Buffer, offset, entry->name.Length);

                    last = TRUE;
                    flag = 1;
                }
                offset = p + 1;
            }
            p++;
            i++;
        }

        if (offset != p) { // if path  offset-> \...\ <-p (==offset?)
            entry = (ParseNameEntry*)ExAllocateFromPagedLookasideList(&glPagedParseEntry);
            InsertHeadList(&names->link, &entry->link);

            entry->name.Length = (p - offset) * sizeof(WCHAR);
            entry->name.MaximumLength = entry->name.Length;
            buff = ExAllocatePool(PagedPool, entry->name.Length);
            if (!buff) {
                return STATUS_MEMORY_NOT_ALLOCATED;
            }
            entry->name.Buffer = buff;
            RtlCopyMemory(entry->name.Buffer, offset, entry->name.Length);
        }

        //PrintParseName(names);
    }
    else {
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

void PrintParseName(ParseNameEntry* names) {

    PLIST_ENTRY link;

    for (link = names->link.Blink; link != &names->link; link = link->Blink) {
        ParseNameEntry* entry = CONTAINING_RECORD(link, ParseNameEntry, link);
        DbgPrint("[NAME PARSE] %d %wZ", entry->name.Length, &entry->name);
    }

    return;
}

SIZE_T CountList(PLIST_ENTRY list) {

    SIZE_T count = 0;
    PLIST_ENTRY link;

    if (!IsListEmpty(list)) {
        for (link = list->Flink; link != list; link = link->Flink) {
            count++;
        }
    }

    return count;
}

void FreeParseEntry(IN ParseNameEntry* names) {


    while (!IsListEmpty(&names->link)) {
        PLIST_ENTRY pLink = RemoveHeadList(&names->link);
        ParseNameEntry* entry = CONTAINING_RECORD(pLink, ParseNameEntry, link);
        //ExFreePool(&entry->name);
        RtlFreeUnicodeString(&entry->name);
        ExFreeToPagedLookasideList(&glPagedParseEntry, entry);
    }


    return;
}