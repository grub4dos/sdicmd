// SPDX-License-Identifier: Unlicense
#include <stdio.h>
#include <windows.h>
#include "sdi.h"

#define SDI_MAX_TOC 8

static BOOL
SdiRead(HANDLE Fd, UINT64 Offset, PVOID pBuf, DWORD Length)
{
	LARGE_INTEGER li;
	li.QuadPart = Offset;
	li.LowPart = SetFilePointer(Fd, li.LowPart, &li.HighPart, FILE_BEGIN);
	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return FALSE;
	return ReadFile(Fd, pBuf, Length, &Length, NULL);
}

static BOOL
SdiWrite(HANDLE Fd, UINT64 Offset, CONST PVOID pBuf, DWORD Length)
{
	LARGE_INTEGER li;
	li.QuadPart = Offset;
	li.LowPart = SetFilePointer(Fd, li.LowPart, &li.HighPart, FILE_BEGIN);
	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return FALSE;
	return WriteFile(Fd, pBuf, Length, &Length, NULL);
}

static UINT64
SdiChecksum(CONST PVOID pBuf)
{
	CONST PUINT8 pSdi = pBuf;
	UINT8* ptr;
	UINT64 ret = 0;
	for (ptr = pSdi; ptr < pSdi + sizeof(SDIHDR); ptr++)
		ret += *ptr;
	return (ret % 256);
}

static UINT8 SdiBlobBoot[8] = SDI_TOC_BLOB_BOOT;
static UINT8 SdiBlobLoad[8] = SDI_TOC_BLOB_LOAD;
static UINT8 SdiBlobDisk[8] = SDI_TOC_BLOB_DISK;
static UINT8 SdiBlobPart[8] = SDI_TOC_BLOB_PART;
static UINT8 SdiBlobWim[8] = SDI_TOC_BLOB_WIM;

static LPCSTR
SdiTocBlobType(SDITOC* pToc)
{
	if (memcmp(pToc->BlobType, SdiBlobBoot, 8) == 0)
		return "Boot";
	else if (memcmp(pToc->BlobType, SdiBlobLoad, 8) == 0)
		return "Loader";
	else if (memcmp(pToc->BlobType, SdiBlobDisk, 8) == 0)
		return "Disk";
	else if (memcmp(pToc->BlobType, SdiBlobPart, 8) == 0)
		return "Partition";
	else if (memcmp(pToc->BlobType, SdiBlobWim, 8) == 0)
		return "WIM";
	else
		return "Unknown";
}

static UINT64
SdiFind(HANDLE Fd)
{
	UINT i;
	UINT64 Offset = 0;
	SDIHDR SdiHeader = { 0 };
	UINT64 Checksum = 0;
	UINT64 Alignment = 0;
	SDITOC SdiToc = { 0 };
	if (!SdiRead(Fd, 0, &SdiHeader, sizeof(SDIHDR)))
		return 0;
	if (memcmp(SdiHeader.Magic, SDI_MAGIC, 8) != 0)
		return 0;
	for (i = 0; i < 8; i++)
		printf("%c", SdiHeader.Magic[i]);
	printf("\n");
	Alignment = SdiHeader.PageAlignment << SDI_PAGE_SHIFT;
	printf("BLOB alignment: %llu (%llu pages)\n", Alignment, SdiHeader.PageAlignment);
	Checksum = SdiChecksum(&SdiHeader);
	printf("Checksum: %llu (%llu, %s)\n", SdiHeader.Checksum, Checksum, Checksum == 0 ? "OK" : "FAIL");
	for (i = 0; i < SDI_MAX_TOC; i++)
	{
		UINT64 Padding = 0;
		if (!SdiRead(Fd, 0x400 + i * sizeof(SDITOC), &SdiToc, sizeof(SdiToc)))
			return 0;
		if (SdiToc.BlobType[0] == 0)
			break;
		printf("BLOB %u: %s\n", i, SdiTocBlobType(&SdiToc));
		printf("  ID: %llu\n", SdiToc.BaseType);
		printf("  Attribute: %llu\n", SdiToc.Attr);
		printf("  Offset: %llu\n", SdiToc.Offset);
		if (SdiToc.Size & (Alignment - 1))
			Padding = Alignment - (SdiToc.Size & (Alignment - 1));
		printf("  Size: %llu (+%llu)\n", SdiToc.Size, Padding);
		if (memcmp(SdiToc.BlobType, SdiBlobPart, 8) == 0)
			Offset = SdiToc.Offset + 512;
	}
	return Offset;
}

static int
SdiHelp(void)
{
	printf("sdicmd OPTIONS SDI_FILE\n");
	printf("OPTIONS:\n");
	printf("  -h      Print help text.\n");
	printf("  -c=TEXT Set SDI cmd.\n");
	printf("  -x=X    Print cmd from drive.\n");
	return 0;
}

static int
SdiDrv(CHAR Letter)
{
	int ErrCode = 0;
	CHAR SdiBuf[512];
	HANDLE hDrive = INVALID_HANDLE_VALUE;
	char PhyPath[] = "\\\\.\\A:";
	snprintf(PhyPath, sizeof(PhyPath), "\\\\.\\%C:", Letter);
	hDrive = CreateFileA(PhyPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (!hDrive || hDrive == INVALID_HANDLE_VALUE)
	{
		printf("error open drive\n");
		return 1;
	}
	if (!SdiRead(hDrive, 512, SdiBuf, sizeof(SdiBuf)))
	{
		printf("error drive read\n");
		ErrCode = 1;
		goto fail;
	}
	if (memcmp(SdiBuf, "FUCK", 4) != 0)
	{
		printf("error invalid magic\n");
		ErrCode = 2;
		goto fail;
	}
	SdiBuf[sizeof(SdiBuf) - 1] = 0;
	printf("%s\n", SdiBuf + 0x10);
fail:
	CloseHandle(hDrive);
	return ErrCode;
}

int main(int argc, char* argv[])
{
	int i;
	char* SdiPath = NULL;
	char* SdiCmd = NULL;
	UINT64 SdiOfs = 0;
	HANDLE SdiFile = INVALID_HANDLE_VALUE;
	CHAR SdiBuf[512];
	for (i = 0; i < argc; i++)
	{
		if (i == 0)
			continue;
		else if (_stricmp(argv[i], "-h") == 0)
			return SdiHelp();
		else if (_strnicmp(argv[i], "-c=", 3) == 0)
			SdiCmd = &argv[i][3];
		else if (_strnicmp(argv[i], "-x=", 3) == 0)
			return SdiDrv(argv[i][3]);
		else
			SdiPath = argv[i];
	}
	if (!SdiPath)
		return SdiHelp();
	SdiFile = CreateFileA(SdiPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (!SdiFile || SdiFile == INVALID_HANDLE_VALUE)
	{
		printf("open %s failed\n", SdiPath);
		return 1;
	}
	SdiOfs = SdiFind(SdiFile);
	if (!SdiOfs)
	{
		printf("Partition image not found.\n");
		goto fail;
	}
	printf("Cmd offset: %llu\n", SdiOfs);
	if (!SdiRead(SdiFile, SdiOfs, SdiBuf, sizeof(SdiBuf)))
	{
		printf("SDI read error\n");
		goto fail;
	}
	SdiBuf[sizeof(SdiBuf) - 1] = 0;
	if (!SdiCmd)
	{
		if (memcmp(SdiBuf, "FUCK", 4) == 0)
			printf("Cmd: %s\n", SdiBuf + 0x10);
		goto fail;
	}
	ZeroMemory(SdiBuf, sizeof(SdiBuf));
	memcpy(SdiBuf, "FUCK", 4);
	snprintf(SdiBuf + 0x10, sizeof(SdiBuf) - 0x10, "%s", SdiCmd);
	SdiWrite(SdiFile, SdiOfs, SdiBuf, sizeof(SdiBuf));

fail:
	CloseHandle(SdiFile);
	return 0;
}
