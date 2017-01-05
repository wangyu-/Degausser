#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <3ds.h>
#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"
#define GLYPH_HEADER_FILE_ONLY
#include "glyph.cpp"

#include "comm.h"
#include "bbp_process.h"

u8 buffer[524288];
jbMgr jbMgr;
Bbp bbp;

// SANITY CHECKS
typedef char test_item[sizeof(_JbMgrItem) == 312 ? 1 : -1];
typedef char test_jbmgr[sizeof(jbMgr) == 1154408 ? 1 : -1];
typedef char test_jbmgr[sizeof(PackHeader) == 68 ? 1 : -1];

// archive-related stuff

u32 extdata_archive_lowpathdata[3] = {MEDIATYPE_SD, 0xa0b, 0};
FS_Archive extdata_archive = {ARCHIVE_EXTDATA, {PATH_BINARY, 0xC, &extdata_archive_lowpathdata}};
FS_Archive sdmc_archive = {ARCHIVE_SDMC, {PATH_ASCII, 1, ""}};



Result ReadJbMgr()
{
	Handle handle;
	u64 filesize;
	u32 size2;
	
	TRY(FSUSER_OpenFile(&handle, extdata_archive, fsMakePath(PATH_ASCII, "/jb/mgr.bin"), FS_OPEN_READ, 0), "Unable to open /jb/mgr.bin for reading");
	TRY(FSFILE_GetSize(handle, &filesize), "Unable to obtain jbMgr filesize");
	TRY(filesize != 1155072, "Unexpected jbMgr filesize");
	TRY(FSFILE_Read(handle, NULL, filesize - 4, &size2, 4), "Unable to read uncompressed jbMgr filesize");
	TRY(size2 > sizeof(buffer), "Uncompressed jbMgr is too big for buffer!");
	TRY(FSFILE_Read(handle, NULL, 0, &buffer, size2), "Unable to read jbMgr");
	FSFILE_Close(handle);

	TRY(gz_decompress((u8 *)&jbMgr, sizeof(jbMgr), buffer, size2), "Unable to decompress jbMgr");
	
	myprintf("Successfully loaded extdata://00000a0b/jb/mgr.bin!\n");
	return 0;
}

Result WriteJbMgr()
{
	u32 compLen = sizeof(buffer);
	TRY(gz_compress(buffer, &compLen, (u8 *)&jbMgr, sizeof(jbMgr)), "Unable to compress jbMgr");
	
	const char* paths[2] = {"/jb/mgr.bin", "/jb/mgr_.bin"};
	for (int i = 0; i < 2; i++)
	{
		Handle handle;
		TRY(FSUSER_OpenFile(&handle, extdata_archive, fsMakePath(PATH_ASCII, paths[i]), FS_OPEN_WRITE, 0), "Unable to write to jbMgr");
		FSFILE_Write(handle, NULL, 0, buffer, compLen, FS_WRITE_FLUSH);
		FSFILE_Write(handle, NULL, 1155068, &compLen, 4, FS_WRITE_FLUSH);
		FSFILE_Close(handle);
	}
	
	// NOT YET IMPLEMENTED
	return 0;
}

int FindSongID(u32 id)
{
	// inefficient code to find the first slot with ID = id
	for (int i = 0; i < 3700; i++)
	{
		if (jbMgr.Items[i].ID == id) return i;
	}
	return -1;
}


// Find the earliest unused customID (0x8000????) in jbMgr
u32 GetEarliestCustomID()
{
	// very inefficient code ahead
	bool used[3700] = {};
	for (int i = 0; i < 3700; i++)
	{
		u32 id = jbMgr.Items[i].ID;
		if ((id >> 16) == 0x8000) used[id & 0xFFFF] = true;
	}
	for (int i = 0; i < 3700; i++)
	{
		if (!used[i]) return 0x80000000 | i;
	}
	return -1;
}


Result DumpAllPacks()
{
	int exportCount = 0;
	
	// create bbpdump folder
	FSUSER_CreateDirectory(sdmc_archive, fsMakePath(PATH_ASCII, "/bbpdump"), 0);
	
	// some operations on jbMgr
	for (int i = 0; i < 3700; i++)
	{
		if (jbMgr.Items[i].ID == (u32)-1) continue; // id != -1 for the item to be valid
		if (!(jbMgr.Items[i].Flags & 1)) continue; // bit0 == 1 to be valid
		if (!(jbMgr.Items[i].Flags & 2)) continue; // bit1 == 1 for pack to be stored on SD
		
		// make a copy with fresh changes
		_JbMgrItem* item = (_JbMgrItem*)buffer;
		*item = jbMgr.Items[i];
		memset(item->Scores, 0, 50);
		item->Singer = -1;
		item->Icon = 0;
		item->Flags &= 0x7FDFFF;
		
		Handle handle;
		u64 filesize;
		
		// read pack
		char packPath[32];
		sprintf(packPath, "/jb/gak/%08lx/pack", item->ID);
		myprintf("* %08lx (", item->ID);
		print(item->Title);
		print(")");
		//myprintf("TEST %s\n", packPath);
		TRYCONT(FSUSER_OpenFile(&handle, extdata_archive, fsMakePath(PATH_ASCII, packPath), FS_OPEN_READ, 0), "Unable to open pack file");
		TRYCONT(FSFILE_GetSize(handle, &filesize), "Unable to obtain pack file size");
		TRYCONT(filesize > sizeof(buffer) - 312, "Size of pack file is unexpectedlly large");
		TRYCONT(FSFILE_Read(handle, NULL, 0, buffer + 312, filesize), "Unable to read pack file");
		FSFILE_Close(handle);
		
		// dump contents of buffer to "/bbpdump/<title> (<author>).bbp"
		unsigned char tmp_id[10];
		sprintf((char*)tmp_id,"%08lx-",item->ID);
		u16 tmp_id2[10];
		for(int j=0;j<10;j++)
			tmp_id2[j]=tmp_id[j];
		u16 bbpPath[128];
		ConcatUTF16(bbpPath, false, u"/bbpdump/", tmp_id2,item->Title, u" (", item->Author, u").bbp", NULL);
		TRYCONT(FSUSER_OpenFile(&handle, sdmc_archive, fsMakePath(PATH_UTF16, bbpPath), FS_OPEN_CREATE | FS_OPEN_WRITE, 0), "Unable to create bbp file");
		TRYCONT(FSFILE_Write(handle, NULL, 0, buffer, 312 + filesize, FS_WRITE_FLUSH | FS_WRITE_UPDATE_TIME), "Unable to write bbp file header");
		FSFILE_Close(handle);
		
		printRight("...SUCCESS!");
		exportCount++;
	}

	myprintf("Exported %u bbp files to sdmc://bbpdata/.\n", exportCount);
	return 0;
}

Result ImportPacks(int c)
{
	print("fuck");
	Handle dirHandle;
	if(c==0)
	{
	TRY(FSUSER_OpenDirectory(&dirHandle, sdmc_archive, fsMakePath(PATH_ASCII, "/bbpimport")), "Cannot find bbpimport directory");
	}
	else
	{
	TRY(FSUSER_OpenDirectory(&dirHandle, sdmc_archive, fsMakePath(PATH_ASCII, "/bbpimportc")), "Cannot find bbpimportc directory");
	}
	int fileCount = 0;
	
	FS_DirectoryEntry entry;
	u32 entriesRead = 0;
	while (!FSDIR_Read(dirHandle, &entriesRead, 1, &entry) && entriesRead)
	{
		print("good so far ");
		if (entry.attributes & FS_ATTRIBUTE_DIRECTORY) continue; // skip folders
		if (strcmp(entry.shortExt, "BBP")) continue; // only read *.bbp
		//myprintf("* %8s (%5llu B)... ", entry.shortName, entry.fileSize);
		print("* ");
		print(entry.name);
		
		print("\n");
		Handle handle;
		u16 bbpPath[300];
		if(c==0)
		{
			ConcatUTF16(bbpPath, false, u"/bbpimport/", entry.name, NULL);
		}
		else
		{
			ConcatUTF16(bbpPath, false, u"/bbpimportc/", entry.name, NULL);
		}
		if (FSUSER_OpenFile(&handle, sdmc_archive, fsMakePath(PATH_UTF16, bbpPath), FS_OPEN_READ, 0))
		{
			printRight("...unable to open");
		}
		else if (entry.fileSize > 131072)
		{
			printRight("...file too large");
			FSFILE_Close(handle);
		}
		else
		{
			int size=entry.fileSize;
			FSFILE_Read(handle, NULL, 0, buffer, entry.fileSize);
			FSFILE_Close(handle);
			
			_JbMgrItem* item;
			if(c==0)
			{
			item = (_JbMgrItem*)buffer;
			if ((item->ID >> 16) == 0x8000)
			{
				// custom ID, do weird stuff
				// TODO: MUCH LATER
				// 1) decompress part1 to buffer + 131072
				// 2) copy part2 to buffer + 262044
				// 3) modify decompressed part1
				// 4) recompress part1
				/*
				PackHeader* header = (PackHeader*)(buffer + 312);
				
				if (header->Version != 0x20001 || header->Parts[0].Used != 1 || header->Parts[2].Used || header->Parts[3].Used || header->Parts[0].Offset != 68)
				{
					myprintf("parsing error\n");
				}
				else if (gz_decompress(buffer + 312 + 68, header->Parts[0].
				{
					// 1)
					gz_decomp
				}
				*/
				printRight("...customID error");
				continue;
			}
			else if (FindSongID(item->ID) != -1)
			{
				printRight("...already exists");
				continue;
			}
			}
			else //c=1;
			{
				int id=GetEarliestCustomID();
				if(id==-1)
				{
					//full
					printRight("...customID full");
					continue;
				}
				myprintf("\n");
				myprintf("\n");
				bbp.init(buffer, entry.fileSize);
				bbp.raw_to_bbp();
				bbp.set_id(id);
				bbp.bbp_to_raw();
				memcpy(buffer,bbp.get_raw(),bbp.raw_size());
				size=bbp.raw_size();
				item = (_JbMgrItem*)buffer;
			}
			//int index = FindEmptySlot();
			int index = FindSongID(-1);
			if (index == -1)
			{
				// THERE IS NO EMPTY SLOT!!!
				printRight("...no empty slot");
				continue;
			}

			char packPath[32];
			sprintf(packPath, "/jb/gak/%08lx", item->ID);
			FSUSER_CreateDirectory(extdata_archive, fsMakePath(PATH_ASCII, packPath), 0);
			strcat(packPath, "/pack");

			FS_Path fsPath = fsMakePath(PATH_ASCII, packPath);
			FSUSER_CreateFile(extdata_archive, fsPath, 0, size - 312);
			FSUSER_OpenFile(&handle, extdata_archive, fsPath, FS_OPEN_WRITE, 0);
			FSFILE_Write(handle, NULL, 0, buffer + 312, size - 312, FS_WRITE_FLUSH);
			FSFILE_Close(handle);

			// TODO: CHECK FIRST THAT PACK WAS SUCCESSFULLY CREATED!?
			jbMgr.Items[index] = *item;

			printRight("...SUCCESS!");
			fileCount++;
		}
	}
	
	FSDIR_Close(dirHandle);
	
	if (fileCount)
	{
		myprintf("Committing changes to /jb/mgr.bin...\n");
		TRY(WriteJbMgr(), "ERROR: Could not modify /jb/mgr.bin\n");
		myprintf("Imported %u bbp files from sdmc://bbpimport/.\n", fileCount);
	}
	else
	{
		myprintf("No changes were made to the extdata.\n");
	}
	
	return 0;
}


Result DeletePacks()
{
	Handle dirHandle;
	TRY(FSUSER_OpenDirectory(&dirHandle, sdmc_archive, fsMakePath(PATH_ASCII, "/bbpdelete")), "Cannot find bbpimport directory");
	int fileCount = 0;

	FS_DirectoryEntry entry;
	u32 entriesRead = 0;
	while (!FSDIR_Read(dirHandle, &entriesRead, 1, &entry) && entriesRead)
	{
		if (entry.attributes & FS_ATTRIBUTE_DIRECTORY) continue; // skip folders
		if (strcmp(entry.shortExt, "BBP")) continue; // only read *.bbp
		//myprintf("* %8s (%5llu B)... ", entry.shortName, entry.fileSize);
		print("* ");
		print(entry.name);

		Handle handle;
		u16 bbpPath[300];
		ConcatUTF16(bbpPath, false, u"/bbpdelete/", entry.name, NULL);
		if (FSUSER_OpenFile(&handle, sdmc_archive, fsMakePath(PATH_UTF16, bbpPath), FS_OPEN_READ, 0))
		{
			printRight("...unable to open");
		}
		else if (entry.fileSize > 131072)
		{
			printRight("...file too large");
			FSFILE_Close(handle);
		}
		else
		{
			FSFILE_Read(handle, NULL, 0, buffer, entry.fileSize);
			FSFILE_Close(handle);

			_JbMgrItem* item = (_JbMgrItem*)buffer;
			if ((item->ID >> 16) == 0x8000)
			{
				myprintf("%lx",item->ID);
				myprintf("<%lx>",(item->ID >> 16) );
				//printRight(" skiped custom song");
				//continue;
			}
			int cnt=0;
			for (int i = 0; i < 3700; i++)//not really very slow.....
			{
				if (jbMgr.Items[i].ID == (u32)-1) continue; // id != -1 for the item to be valid
				if (!(jbMgr.Items[i].Flags & 1)) continue; // bit0 == 1 to be valid
				if (!(jbMgr.Items[i].Flags & 2)) continue; // bit1 == 1 for pack to be stored on SD

				if (jbMgr.Items[i].ID==item->ID)
				{
					memset((void*)(jbMgr.Items+i),0,sizeof(jbMgr.Items[i]));
					jbMgr.Items[i].ID=-1;
					fileCount++;
					char packPath[32];
					sprintf(packPath, "/jb/gak/%08lx", item->ID);
					if(FSUSER_DeleteDirectoryRecursively(extdata_archive, fsMakePath(PATH_ASCII, packPath)))
					{
						printRight("delete dir ");
						printRight(packPath);
						printRight(" fail!");
					}
					cnt++;
					printRight("...SUCCESS!");
				}

			}
			if(cnt==0) printRight("...not found!");
		}
	}

	FSDIR_Close(dirHandle);

	if (fileCount)
	{
		myprintf("Committing changes to /jb/mgr.bin...\n");
		TRY(WriteJbMgr(), "ERROR: Could not modify /jb/mgr.bin\n");
		myprintf("Idelete %u bbp files in sdmc://bbpimport/.\n", fileCount);
	}
	else
	{
		myprintf("No changes were made to the extdata.\n");
	}

	return 0;
}

bool initialised = false;
void ShowInstructions()
{
	myprintf("\n");
	if (initialised)
	{
		myprintf("Press X to dump all BBP files(to /bbpdump/).\n");
		myprintf("Press Y to import all BBP files(from /bbpimport/).\n");
		myprintf("Press A to import all BBP files as custom(from /bbpimportc/).\n");
		myprintf("Press SELECT to delete BBP files(according to /bbpdelete/).\n");
	}
	myprintf("Press START to exit.\n\n");
}


int main()
{
	// Initialize services
	glyphInit();
	//gfxInitDefault();
	//consoleInit(GFX_BOTTOM, NULL);
	
	//FSUSER_OpenArchive(&sdmc_archive); ImportPacks(); while(true);
	
	myprintf("== degausser3ds v2.2a new==\n");
	myprintf("Loading Daigasso! Band Bros P extdata into memory...\n");
	if (FSUSER_OpenArchive(&extdata_archive)) myprintf("ERROR: Unable to open DBBP extdata.\n");
	else if (FSUSER_OpenArchive(&sdmc_archive)) myprintf("ERROR: Unable to open SDMC archive.\n");
	else if (ReadJbMgr()) myprintf("ERROR: Unable to fully process /jb/mgr.bin\n");
	else initialised = true;

	ShowInstructions();
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxFlushBuffers();
		gfxSwapBuffers();
		hidScanInput();
		if (hidKeysDown() & KEY_X)
		{
			if (!initialised) continue;
			myprintf("Dumping bbp files to sdmc://bbpdata/:\n");
			DumpAllPacks();
			ShowInstructions();
		}
		else if (hidKeysDown() & KEY_Y)
		{
			if (!initialised) continue;
			myprintf("Importing bbp files from sdmc://bbpimport/:\n");
			ImportPacks(0);
			ShowInstructions();
		}
		else if (hidKeysDown() & KEY_A)
		{
			if (!initialised) continue;
			myprintf("Importing bbp files from sdmc://bbpimportc/:\n");
			ImportPacks(1);
			ShowInstructions();
		}
		else if (hidKeysDown() & KEY_SELECT)
		{
			if (!initialised) continue;
			myprintf("deleting:\n");
			DeletePacks();
			ShowInstructions();
		}

		else if (hidKeysDown() & KEY_START) break;
	}
	
	// Close archives
	FSUSER_CloseArchive(&sdmc_archive);
	FSUSER_CloseArchive(&extdata_archive);
	
	glyphExit();
	return 0;
}
