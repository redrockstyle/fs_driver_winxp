#include "ntstring.h"


//
// �������� ������, �������������� ����, �� UNICODE-������.
// ������ ��� SZ-������ ������ ���� ����������� � ���������� �������.
//
PCHAR GetSZFromUnicodeString(PUNICODE_STRING pUniStr) {

    ANSI_STRING ansiStr;
    char* str;
    USHORT length;
    NTSTATUS status;

    // �������� ������ ANSI �� UNICODE
    status = RtlUnicodeStringToAnsiString(&ansiStr, pUniStr, TRUE);

    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    length = ansiStr.Length;

    // �������� ������ ��� SZ-������
    // ������ ������ ���� ����������� ������� ExFreePool (str);
    str = (char*)ExAllocatePool(PagedPool, length + 1);
    if (!str)
        return NULL;

    RtlCopyMemory(str, ansiStr.Buffer, length);
    //strncpy(str,ansistr.Buffer,len);
    str[length] = 0;

    // ����������� �����, ���������� ��� �������� ANSI ������
    RtlFreeAnsiString(&ansiStr);

    return str;
}


//--------------------

//
// �������� UNICODE ������, ��������������� ���� �� SZ-������.
//
PWCH AnsiToUnicode(char* str) {

    ANSI_STRING ansiStr;
    UNICODE_STRING uniStr;
    USHORT length;

    RtlInitAnsiString(&ansiStr, str);
    length = RtlAnsiStringToUnicodeSize(&ansiStr);
    uniStr.Buffer = (PWCH)ExAllocatePool(PagedPool, length);
    uniStr.MaximumLength = length;
    RtlAnsiStringToUnicodeString(&uniStr, &ansiStr, FALSE);

    return uniStr.Buffer;
}


//--------------------


//
// �������� ��������� ������ � ���� SZ-������
//
PCHAR GetCurrentTimeString() {

    PCHAR pszTime;
    SIZE_T cchTime = 20;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER LocalTime;
    TIME_FIELDS  timeFiled;
    NTSTATUS status;

    LPCSTR pszFormat = "%d-%02d-%02d %02d:%02d:%02d";
    pszTime = (char*)ExAllocatePool(PagedPool, cchTime);

    KeQuerySystemTime(&SystemTime);
    ExSystemTimeToLocalTime(&SystemTime, &LocalTime);
    RtlTimeToTimeFields(&LocalTime, &timeFiled);

    status = RtlStringCchPrintfA((NTSTRSAFE_PSTR)pszTime, cchTime, pszFormat,
        timeFiled.Year,
        timeFiled.Month,
        timeFiled.Day,
        timeFiled.Hour,
        timeFiled.Minute,
        timeFiled.Second
    );

    //DbgPrint("STATUS %p", status);
    //DbgPrint("GET TIME SYSTEM %s", pszTime);

    return pszTime;
}

// \/:*?"<>|

BOOLEAN IsValidStringName(PUNICODE_STRING str) {

    WCHAR* p = str->Buffer;
    //WCHAR* offset = str->Buffer;

    //DbgPrint("IsValidStringName \\ / : * ? \" < > | %wZ",   str);


    if (*p == L'\\') {
        while (p != str->Buffer + (str->Length / sizeof(WCHAR))) {
            if (*p == L'/' || *p == L':' || *p == L'\"' ||
                *p == L'<' || *p == L'>' || *p == L'|' ||
                *p == L'?' || *p == L'*') {
                return FALSE;
            }
            p++;
        }
    }
    else return FALSE;


    return TRUE;
}
