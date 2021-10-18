#ifndef _NTSTRING_H_
#define _NTSTRING_H_

#include <ntddk.h>
#include <ntstrsafe.h>

PCHAR GetSZFromUnicodeString(PUNICODE_STRING pUniStr);
PWCH AnsiToUnicode(char* str);
PCHAR GetCurrentTimeString();
//NTSTATUS GetParseName(IN PWCHAR buffer, OUT LIST_ENTRY parse);
BOOLEAN IsValidStringName(PUNICODE_STRING str);

#endif // !_NTSTRING_H_
