// SPDX-License-Identifier: Unlicense

#ifndef SDI_HEADER
#define SDI_HEADER 1

#include <windows.h>

#pragma pack(1)

#define SDI_MAGIC "$SDI0001"
#define SDI_ALIGN 4096

#define SDI_PAGE_SHIFT 12

#define MDB_TYPE_UNSPECIFIED	0
#define MDB_TYPE_RAM			1
#define MDB_TYPE_ROM			2

typedef struct
{
	UINT8  Magic[8];
	UINT64 MDBType;
	UINT64 BootCodeOffset;
	UINT64 BootCodeSize;
	UINT64 VendorID;
	UINT64 DeviceID;
	GUID   DeviceModel;
	UINT64 DeviceRole;
	UINT64 Reserved1;
	GUID   RuntimeGUID;
	UINT64 RuntimeOEMRev;
	UINT64 Reserved2;
	UINT64 PageAlignment;
	UINT64 Reserved3[48];
	UINT64 Checksum;
} SDIHDR;

#define SDI_TOC_BLOB_BOOT	{ 'B', 'O', 'O', 'T', 0, 0, 0, 0 }
#define SDI_TOC_BLOB_LOAD	{ 'L', 'O', 'A', 'D', 0, 0, 0, 0 }
#define SDI_TOC_BLOB_PART	{ 'P', 'A', 'R', 'T', 0, 0, 0, 0 }
#define SDI_TOC_BLOB_DISK	{ 'D', 'I', 'S', 'K', 0, 0, 0, 0 }
#define SDI_TOC_BLOB_WIM	{ 'W', 'I', 'M', 0, 0, 0, 0, 0 }

#define SDI_TOC_BASE_NONE		0x00
#define SDI_TOC_BASE_BIG_FAT16	0x06
#define SDI_TOC_BASE_NTFS_EXFAT	0x07
#define SDI_TOC_BASE_CHS_FAT32	0x0b
#define SDI_TOC_BASE_LBA_FAT32	0x0c

typedef struct
{
	UINT8  BlobType[8];
	UINT64 Attr;
	UINT64 Offset;
	UINT64 Size;
	UINT64 BaseType;
	UINT64 Reserved[3];
} SDITOC;

#pragma pack()

#endif
