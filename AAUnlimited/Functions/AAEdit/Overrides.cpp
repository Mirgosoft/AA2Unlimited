#include "Overrides.h"

#include <Windows.h>

#include "Files\Logger.h"
#include "General\Util.h"
#include "Functions\AAEdit\Globals.h"
#include "Functions\TextureImage.h"

namespace AAEdit {


/*********************************/
/* General Archive File Override */
/*********************************/

bool ArchiveOverrideRules(wchar_t* archive, wchar_t* file, DWORD* readBytes, BYTE** outBuffer) {
	size_t nConverted = 0;
	char strArchivePath[256];
	wcstombs_s(&nConverted, strArchivePath, 256, archive, 256);
	if (nConverted == 0) return false;
	char* strArchive = General::FindFileInPath(strArchivePath);

	char strFile[256];
	wcstombs_s(&nConverted, strFile, 256, file, 256);
	if (nConverted == 0) return false;

	const OverrideFile* match = g_cardData.GetArchiveOverrideFile(strArchive, strFile);
	if (match == NULL) return false;

	void* fileBuffer = IllusionMemAlloc(match->GetFileSize());
	match->WriteToBuffer((BYTE*)fileBuffer);
	*outBuffer = (BYTE*)fileBuffer;
	*readBytes = match->GetFileSize();
	return true;
}

/**************************/
/* Mesh-Texture overrides */
/**************************/
//for more information, look at the MemMods/AAEdit/MeshTexture.cpp

namespace {
	bool loc_currentlyOverriding = false;
	const TextureImage* loc_match = NULL;
	DWORD loc_oldNameLength;
	DWORD loc_oldFileLength;
	HANDLE loc_matchFile;
}

/*
	* List Step 1: Determine if we want to override this file, replace size if we want.
	* string is saved as DWORD size, then BYTE[size], where the array is not-ed with itself.
	* Function returns new size, or 0 if size shouldnt be changed
	*/
DWORD __stdcall MeshTextureListStart(BYTE* xxFileBuffer, DWORD offset) {
	loc_currentlyOverriding = false;
	BYTE* file = xxFileBuffer + offset;
	DWORD size = *(DWORD*)(file);
	file += 4;

	for (DWORD i = 0; i < size; i++) file[i] = ~file[i];
	loc_match = g_cardData.GetMeshOverrideTexture((char*)file);
	for (DWORD i = 0; i < size; i++) file[i] = ~file[i];

	if (loc_match != NULL) {
		loc_currentlyOverriding = true;
		loc_oldNameLength = size;
		return loc_match->GetFileName().size() + 1;
	}
	else {
		return 0;
	}
}

/*
	* List Step 2: if name is overriden, insert new name into buffer instead.
	* also, adjust iterator so we dont shoot past the file.
	*/
bool __stdcall MeshTextureListFill(BYTE* name, DWORD* xxReadOffset) {
	if (!loc_currentlyOverriding) return false;
	const std::string& matchname = loc_match->GetFileName();
	for (size_t i = 0; i < matchname.size(); i++) {
		name[i] = matchname[i];
	}
	name[matchname.size()] = '\0';
	DWORD difference = loc_oldNameLength - (matchname.size() + 1); //note that it incudes \0
	*xxReadOffset += difference;
	return true;
}
	
	
/*
	* Step 1. Determine if we want to override this file, replace size member if we want.
	* reminder:  //struct filelist {
	//  DWORD nFiles;
	//  struct {
	//   DWORD nameLength; //including \0
	//   char name[nameLength]; //"encrypted", the name is not-ed
	//	  DWORD unknown;
	//	  DWORD width;
	//	  DWORD height;
	//	  BYTE unknown[0x21-12]
	//	  DWORD filelength;
	//	  BYTE file[fileLength]; //the magic number of BMs might be 0
	//  } files[nFiles]
	//}
	*/
void __stdcall MeshTextureStart(BYTE* xxFile, DWORD offset) {
	loc_currentlyOverriding = false;

	BYTE* file = xxFile + offset;
	DWORD* ptrNameLength = (DWORD*)(file);
	DWORD nameLength = *ptrNameLength;
	file += 4;
	//"decrypt" the string name in place so we can compare better
	for (DWORD i = 0; i < nameLength; i++) file[i] = ~file[i];
	loc_match = g_cardData.GetMeshOverrideTexture((char*)file);

	//remember to encrypt again
	for (DWORD i = 0; i < nameLength; i++) file[i] = ~file[i];

	//if we found a file to override with, change length (thats our job here after all)
	if (loc_match != NULL) {
		const std::string& matchPath = loc_match->GetFileName();
		loc_matchFile = CreateFile(loc_match->GetFilePath().c_str(), FILE_READ_ACCESS, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (loc_matchFile != NULL && loc_matchFile != INVALID_HANDLE_VALUE) {
			//successfully opened file
			loc_currentlyOverriding = true;
			//manipulate name length
			loc_oldNameLength = *ptrNameLength;
			*ptrNameLength = matchPath.size() + 1; //plus nullterminator
		}

			
	}
}

/*
	* Step 2. If we want to replace the name, fill it here and return true.
	* else, return false.
	* ALSO, we need to adjust the xxReadOffset. because we changed the nameLength,
	* he will do offset += nameLength, which is wrong now (it should be offset += oldNameLength).
	* therefor, we will need to add (oldNameLength - nameLength)
	*/
bool __stdcall MeshTextureOverrideName(BYTE* buffer, DWORD* xxReadOffset) {
	if (!loc_currentlyOverriding) return false;
	//we cant replace the name. it will erase it later for some reason, or not find the texture.
	const std::string& matchPath = loc_match->GetFileName();
	for (unsigned int i = 0; i < matchPath.size(); i++) {
		buffer[i] = matchPath[i];
	}
	buffer[matchPath.size()] = '\0';
	//also, adjust his current offset
	DWORD difference = loc_oldNameLength - (matchPath.size()+1); //note that it incudes \0
	*xxReadOffset += difference;
	return true;
}

/*
	* Step 3 . change file size and dimensions. offset points to first unknown data
	*/
void __stdcall MeshTextureOverrideSize(BYTE* buffer, DWORD offset) {
	if (!loc_currentlyOverriding) return;
	//set width and height
	BYTE* file = buffer + offset + 4; //skip to width
	*(DWORD*)(file) = loc_match->GetWidth();
	file += 4;
	*(DWORD*)(file) = loc_match->GetHeight();
	file += 4 + 0x15;
	loc_oldFileLength = *(DWORD*)(file);
	*(DWORD*)(file) = loc_match->GetFileSize();
}

/*
	* Step 4. Fill the buffer with the selected image file.
	*/

bool __stdcall MeshTextureOverrideFile(BYTE* buffer, DWORD* xxReadOffset) {
	if (!loc_currentlyOverriding) return false;
	//just copy it all. the class has a method for that.
	loc_match->WriteToBuffer(buffer);
	//remember to change the xxFileOffsetm like above
	DWORD difference = loc_oldFileLength - loc_match->GetFileSize();
	*xxReadOffset += difference;
	return true;
}


}