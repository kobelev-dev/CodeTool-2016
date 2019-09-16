//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <stdlib.h>
#include  <windows.h>
#include  <assert.h>
#include  <psapi.h>

#include  "Common/FileDataHelper.H"

//----------------------------------------------------------------------
//  ===================  TBasicFileInfo =======================
//----------------------------------------------------------------------

void TBasicFileInfo::InitBasicFileInfo()
{
	file_name = file_body = NULL;
}

void TBasicFileInfo::FreeBasicFileInfo()
{
	if (file_name != NULL)
	{
		free(file_name);
		file_name = NULL;
	}

	if (file_body != NULL)
	{
		free(file_body);
		file_body = NULL;
	}
}

bool TBasicFileInfo::Compare(const TBasicFileInfo *info1, const TBasicFileInfo *info2)
{
	if (info1 == NULL || info2 == NULL)
		return(FALSE);

	// Compare props that are easy to compare first.
	if (info1->file_name == NULL || info2->file_name == NULL)
		return(FALSE);
	else if (info1->file_len != info2->file_len || info1->file_crc != info2->file_crc)
		return(FALSE);

	// Compare the bodies of the file names.
	if (wcscmp(info1->file_name, info2->file_name) != 0)
		return(FALSE);

	// The files are the same.
	return(TRUE);
}

//------------------------------------------------------------------------
//  ===================  TFileBodyHelper =======================
//------------------------------------------------------------------------

static const wchar_t *g_FileBodyHelper_FileLoadSaveErrorTexts[2*ldres_num_results] =
{
	L"success",								L"Success",									// ldres_succes,
	L"the file name is empty",					L"The file name is empty",						// ldres_fname_missing,
	L"the file is empty",						L"The file is empty",							// ldres_file_empty,
	L"error opening the file",					L"Error opening the file",						// ldres_error_opening,
	L"not enough memory for loading the file",		L"Not enough memory for loading the file",			// ldres_out_of_memory,
	L"error reading the file body",				L"Error reading the file body",					// ldres_error_reading,
	L"error writing the file",						L"Error writing the file",							// ldres_error_writing,
	L"error closing the file",						L"Error closing the file",							// ldres_error_closing,
};

//
// The table is based on the Ethernet polinomial.
//
DWORD g_FileBodyHelper_Crc32Table[256] =
{
/*00*/	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
		0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
/*10*/	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
		0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
/*20*/	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
		0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
/*30*/	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
		0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
/*40*/	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
		0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
/*50*/	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
		0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
/*60*/	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
		0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
/*70*/	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
		0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
/*80*/	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
		0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
/*90*/	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
		0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
/*A0*/	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
		0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
/*B0*/	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
		0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
/*C0*/	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
		0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
/*D0*/	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
		0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
/*E0*/	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
		0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
/*F0*/	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
		0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

//
//  Load disk file into memory.
//
TLoadSaveResult TFileBodyHelper::LoadAsciiFileBody(TBasicFileInfo &info,
									const wchar_t *fname, const wchar_t *fname_on_disk, bool  use_real_disk_fname,
									bool  allow_empty_files_loading, bool  warn_on_big_values, int  *p_num_non_ascii_warnings,
									long *p_first_non_ascii_char_index, HANDLE *p_opened_file_handle, DWORD *p_win32_error)
{
	// Clear the output variables.
	memset(&info, 0, sizeof(TBasicFileInfo));
	if (p_opened_file_handle != NULL)
		*p_opened_file_handle = NULL;
	if (p_win32_error != NULL)
		*p_win32_error = 0;

	// Prepare the file access flags for simple reading.
	DWORD desired_access_rights = GENERIC_READ;
	DWORD share_mode = FILE_SHARE_WRITE | FILE_SHARE_READ;

	if (p_opened_file_handle != NULL)
	{
		// Change access rights to exclusive read-write mode.
		desired_access_rights = GENERIC_READ | GENERIC_WRITE;
		share_mode = 0;			// Do not share this file with anybody.
	}

	// Check the input parameters.
	if (fname == NULL || fname[0] == 0)
		return(ldres_fname_missing);
	if (fname_on_disk != NULL && fname_on_disk[0] == 0)
		return(ldres_fname_missing);

	// Prepare the file name if needed.
	wchar_t fname_buff[2*MAX_PATH];
	if (fname_on_disk == NULL && use_real_disk_fname == TRUE)
	{
		if (::GetFullPathNameW(fname, 2*MAX_PATH, fname_buff, NULL) == 0)
		{
			if (p_win32_error != NULL)
				*p_win32_error = ::GetLastError();
			return(ldres_error_opening);
		}

		// Use the retrieved name as the name of the file.
		fname = fname_buff;
	}

	// Open the file.
	HANDLE hFile = ::CreateFileW((fname_on_disk != NULL) ? fname_on_disk : fname,
							desired_access_rights, share_mode,
							NULL,								// Security attributes.
							OPEN_EXISTING,						// Do not create the new file.
							0,									// No additional misc attributes.
							NULL);								// Template.

	if (hFile == INVALID_HANDLE_VALUE)
	{
		if (p_win32_error != NULL)
			*p_win32_error = ::GetLastError();
		return(ldres_error_opening);
	}

	// Get the length of the file.
	BY_HANDLE_FILE_INFORMATION file_info;
	if (::GetFileInformationByHandle(hFile, &file_info) == FALSE || file_info.nFileSizeHigh != 0)
	{
		if (p_win32_error != NULL)
			*p_win32_error = ::GetLastError();
		::CloseHandle(hFile);
		return(ldres_error_opening);
	}

	long file_size = file_info.nFileSizeLow;
	if (file_size == 0 && allow_empty_files_loading == FALSE)
	{
		::CloseHandle(hFile);
		return(ldres_file_empty);
	}

	// Allocate memory for the file body and read the file in.
	wchar_t *pBody = (wchar_t*)malloc((file_size+1)*sizeof(wchar_t));
	if (pBody == NULL)
	{
		::CloseHandle(hFile);
		return(ldres_out_of_memory);
	}

	if (file_size > 0)
	{
		DWORD bytes_retrieved;
		if (::ReadFile(hFile, pBody, file_size, &bytes_retrieved, NULL) == FALSE || (long)bytes_retrieved != file_size)
		{
			if (p_win32_error != NULL)
				*p_win32_error = ::GetLastError();
			::CloseHandle(hFile);
			free(pBody);
			return(ldres_error_reading);
		}
	}

	// Duplicate the name of the file.
	info.file_name = (wchar_t*)malloc((wcslen(fname)+1)*sizeof(wchar_t));
	if (info.file_name == NULL)
	{
		::CloseHandle(hFile);
		free(pBody);
		return(ldres_out_of_memory);
	}

	// Allocation succeeded. Copy in the data.
	wcscpy(info.file_name, fname);

	// The loaded data is expected to be 8-bit ASCII data. Convert it into UNICODE.
	int  cnt_warnings = 0;
	long first_wrn_index = -1;
	unsigned char *char_data = (unsigned char*)pBody;
	for (long inx=file_size-1; inx >= 0; --inx)
	{
		unsigned char ch = char_data[inx];
		if (ch >= 128)
		{
			ch = L'?';
			cnt_warnings++;
			first_wrn_index = inx;
		}
		pBody[inx] = (wchar_t)ch;
	}

	// Place NULL character at the end.
	pBody[file_size] = 0;

	// Fill in the structure.
	info.file_body = pBody;
	info.file_len = file_size;
	info.file_date = MakeDateTime(file_info.ftLastWriteTime);
	info.file_crc = CalcCrc32(pBody, file_size*sizeof(wchar_t));

	// Return the details.
	if (p_num_non_ascii_warnings != NULL)
		*p_num_non_ascii_warnings = cnt_warnings;
	if (p_first_non_ascii_char_index != NULL)
		*p_first_non_ascii_char_index = first_wrn_index;

	if (p_opened_file_handle != NULL)
	{
		// Give out the handle.
		*p_opened_file_handle = hFile;
	}
	else
	{
		// Get rid of the handle.
		::CloseHandle(hFile);
	}

	return(ldres_success);
}

TLoadSaveResult TFileBodyHelper::SetupInMemFileBody(TBasicFileInfo &info, const wchar_t *fname, const wchar_t *fbody, long flen, TDateTime fdate)
{
	// Check the input parameters.
	if (fname == NULL || fname[0] == 0)
		return(ldres_fname_missing);

	memset(&info, 0, sizeof(TBasicFileInfo));

	// Duplicate the name of the file.
	info.file_name = (wchar_t*)malloc((wcslen(fname)+1)*sizeof(wchar_t));
	if (info.file_name == NULL)
		return(ldres_out_of_memory);

	wcscpy(info.file_name, fname);

	if (fbody != NULL)
	{
		// Duplicate the body of the file.
		if (flen < 0)
			flen = (long)wcslen(fbody);

		info.file_body = (wchar_t*)malloc((flen+1)*sizeof(wchar_t));
		if (info.file_body == NULL)
		{
			free(info.file_name);
			info.file_name = NULL;
			return(ldres_out_of_memory);
		}

		// Use memcpy to allow for NULL chars inside the file body.
		memcpy(info.file_body, fbody, flen*sizeof(wchar_t));
		info.file_body[flen] = 0;
	}
	else
	{
		// It is not necessary to do anything. Clear the passed length.
		flen = 0;
	}

	// Fill in the rest of the fields.
	info.file_len = flen;
	info.file_date = fdate;
	info.file_crc = (fbody != NULL) ? CalcCrc32(fbody, flen*sizeof(wchar_t)) : 0;
	return(ldres_success);
}

TLoadSaveResult TFileBodyHelper::SaveAsciiFileBody(const wchar_t *fname, wchar_t *fbody, long flen)
{
	// Check the input parameters.
	if (fname == NULL || fname[0] == 0)
		return(ldres_fname_missing);

	// Open the file using the passed name.
	HANDLE hFile = ::CreateFileW(fname,
						GENERIC_WRITE | STANDARD_RIGHTS_ALL,
						0,					// No sharing.
						NULL,				// Security attributes.
						OPEN_ALWAYS,
						0,
						NULL);				// Template.

	if (hFile == INVALID_HANDLE_VALUE)
		return(ldres_error_opening);

	::SetEndOfFile(hFile);

	// Convert the body of the file and write it to the disk.
	if (fbody != NULL)
	{
		if (flen < 0)
			flen = (long)wcslen(fbody);

		if (flen > 0)
		{
			// Convert to ASCII in place.
			wchar_t *src = fbody;
			char *dest = (char*)fbody;
			for (long cnt=flen; cnt>0; --cnt)
				*dest++ = (char)*src++;

			DWORD dwBytesWritten;
			if (::WriteFile(hFile, fbody, flen, &dwBytesWritten, NULL) == FALSE)
			{
				::CloseHandle(hFile);
				return(ldres_error_writing);
			}
		}
	}

	// Success.
	::CloseHandle(hFile);
	return(ldres_success);
}

const wchar_t *TFileBodyHelper::GetLoadSaveResultText(TLoadSaveResult err, bool want_first_cap)
{
	if (((int)err) < 0 || err >= ldres_num_results)
		return((want_first_cap == TRUE) ? L"Incorrect TLoadSaveResult value" : L"incorrect TLoadSaveResult value");

	int inx = 2*err;
	if (want_first_cap == TRUE)
		inx++;

	return(g_FileBodyHelper_FileLoadSaveErrorTexts[inx]);
}

DWORD TFileBodyHelper::CalcCrc32(const void *data, long data_len)
{
	BYTE *ptr = (BYTE*)data;
	DWORD crc = 0xFFFFFFFF;

	while (data_len-- > 0)
	{
		crc = (crc >> 8) ^ g_FileBodyHelper_Crc32Table[(crc & 0xFF) ^ *ptr++];
	}

	return(crc ^ 0xFFFFFFFF);
}

void TFileBodyHelper::ConvertToAsciiInPlace(wchar_t *buffer)
{
	wchar_t *src = buffer;
	char *dest = (char*)buffer;

	while (*src != 0)
		*dest++ = (char)*src++;

	*dest = 0;
}

//------------------------------------------------------------------------
//  ===================  TSimpleLineInfo =======================
//------------------------------------------------------------------------

TSimpleLineInfo *TSimpleLineInfo::BuildLinesInfoFromData(const wchar_t *fdata, long fdata_len, long fdata_beg_src_offs, long &linfo_len, long &longest_line_length)
{
	assert(fdata != NULL);
	if (fdata == NULL)
		return(NULL);

	if (fdata_len < 0)
		fdata_len = (long)wcslen(fdata);

	if (fdata_len == 0)
	{
		// This is a special case of an empty file. Create lines info that consists of one line only.
		TSimpleLineInfo *linfo = (TSimpleLineInfo*)malloc(sizeof(TSimpleLineInfo));
		if (linfo == NULL)
			return(NULL);

		linfo->line_offs = fdata_beg_src_offs;
		linfo->line_len = 0;
		linfo_len = 1;
		longest_line_length = 0;
		return(linfo);
	}

	// Allocate an initial buffer.
	linfo_len = 0;
	int linfo_alloc = 1000;
	TSimpleLineInfo *linfo = (TSimpleLineInfo*)malloc(linfo_alloc*sizeof(TSimpleLineInfo));
	if (linfo == NULL)
		return(NULL);

	int  inx = 0;
	long curr_line_offs = fdata_beg_src_offs;
	long longest_line_len = 0;
	while (fdata_len >= 0)
	{
		// Check if there is a room for at least one additional descriptor or not.
		if (inx >= linfo_alloc)
		{
			// Reallocate the array.
			TSimpleLineInfo *li_temp = (TSimpleLineInfo*)realloc(linfo, (linfo_alloc+4000)*sizeof(TSimpleLineInfo));
			if (li_temp == NULL)
			{
				free(linfo);
				return(NULL);
			}

			linfo = li_temp;
			linfo_alloc += 4000;
		}

		linfo[inx].line_offs = curr_line_offs;
		if (fdata_len == 0)
		{
			// This is an empty string after the crlf at the end of the file.
			linfo[inx++].line_len = 0;
			break;
		}

		// Find out where the current line ends.
		int len_lf = 0;
		int curr_line_len = fdata_len;
		for (int ch_inx=0; ch_inx<fdata_len; ++ch_inx)
		{
			if (fdata[ch_inx] == L'\n')
			{
				curr_line_len = ch_inx;
				len_lf = 1;
				break;
			}
			else if (fdata[ch_inx] == L'\r' && ch_inx < fdata_len-1 && fdata[ch_inx+1] == L'\n')
			{
				curr_line_len = ch_inx;
				len_lf = 2;
				break;
			}
		}

		if (curr_line_len > longest_line_len)
			longest_line_len = curr_line_len;

		if (curr_line_len == fdata_len)
		{
			// The end of the data is reached. There is no eol at the end of the file data.
			linfo[inx++].line_len = fdata_len;
			break;
		}

		// An end of line is found.
		linfo[inx++].line_len = curr_line_len;
		curr_line_len += len_lf;
		fdata += curr_line_len;
		fdata_len -= curr_line_len;
		curr_line_offs += curr_line_len;
	}

	// Give out the data.
	linfo_len = inx;
	longest_line_length = longest_line_len;
	return(linfo);
}

void TSimpleLineInfo::FreeLinesInfo(TSimpleLineInfo **pinfo)
{
	if (*pinfo != NULL)
	{
		free(*pinfo);
		*pinfo = NULL;
	}
}

TSimpleLineInfo *TSimpleLineInfo::FindLineInfo(TSimpleLineInfo *lines_info, long lines_info_len, long file_offs)
{
	if (lines_info_len <= 0)
	{
		// The input data is bogus.
		return(NULL);
	}
	else if (file_offs < lines_info[0].line_offs)
	{
		// The offset is too small or it is negative.
		return(NULL);
	}

	// Find the first line that is smaller than the given offset. Use the binary search for doing this.
	long lo_good = 0;
	long hi_bad = lines_info_len;
	while ((hi_bad - lo_good) > 1)
	{
		long inx_test = (hi_bad+lo_good)/2;
		if (file_offs >= lines_info[inx_test].line_offs)
			lo_good = inx_test;
		else hi_bad = inx_test;
	}

	// The result is in the lo_good variable.
	TSimpleLineInfo &res_info = lines_info[lo_good];
	if (file_offs > res_info.LineEnd())
	{
		// The offset is bigger than the length of the line.
		return(NULL);
	}

	// Full success.
	return(&res_info);
}

//-------------------------------------------------------------------------
//  =================  TFileSpaceIterationInfo  =====================
//-------------------------------------------------------------------------

bool TFileSpaceIterationInfo::StepIteration()
{
	if (area_len < 0)
	{
		// Iteration is already finished or the passed file area was empty.
		return(FALSE);
	}

	if (line_inx < 0)
	{
		assert(linfo_data != NULL && linfo_len > 0);

		// This is the start of iteration. Ensure that the passed area belongs to the file data.
		if (area_beg < linfo_data[0].LineBeg())
		{
			// Length of the area should be modified first.
			long adjusted_beg = linfo_data[0].line_offs;
			area_len -= adjusted_beg-area_beg;
			area_beg = adjusted_beg;

			if (area_len < 0)
			{
				// The area is outside of the file.
				return(FALSE);
			}
		}

		if (area_beg > linfo_data[linfo_len-1].LineEnd())
		{
			// The area is outside of the file.
			area_len = -1;
			return(FALSE);
		}

		// Find the first line of the intersection. Use the binary search for this.
		long lo_good = 0;
		long hi_bad = linfo_len;
		while ((hi_bad - lo_good) > 1)
		{
			long inx_test = (hi_bad+lo_good)/2;
			if (area_beg >= linfo_data[inx_test].line_offs)
				lo_good = inx_test;
			else hi_bad = inx_test;
		}

		// The result of the search is in the lo_good variable.
		line_inx = lo_good;
		char_beg = area_beg - linfo_data[lo_good].line_offs;
		long line_len = linfo_data[lo_good].line_len;
		if (char_beg <= line_len)
		{
			// An itersection with the curr line is not empty.
			num_chars = __min(line_len-char_beg, area_len);
			return(TRUE);
		}
	}

	// This is the next step of iteration or the passed area begins in the gap between two lines.
	line_inx++;
	if (line_inx >= linfo_len)
	{
		// An end of the file data is reached.
		area_len = -1;
		return(FALSE);
	}

	// Check, if there is any intersection with the current line.
	long new_beg = linfo_data[line_inx].line_offs;
	area_len -= new_beg-area_beg;
	if (area_len < 0)
		return(FALSE);

	// An intersection is non empty.
	area_beg = new_beg;
	char_beg = 0;
	num_chars = __min(linfo_data[line_inx].line_len, area_len);
	return(TRUE);
}

//-------------------------------------------------------------------------
//  ==================  TLocalFilesDirectory  ======================
//-------------------------------------------------------------------------

TLocalFilesDirectory *TLocalFilesDirectory::m_files_list = NULL;

TBasicFileInfo *TLocalFilesDirectory::FindFileInfo(const wchar_t *full_file_name)
{
	// Use plain iteration of the single linked list.
	TLocalFilesDirectory *info = m_files_list;
	while (info != NULL)
	{
		if (wcscmp(info->m_file_data.file_name, full_file_name) == 0)
			return(&(info->m_file_data));
		info = info->m_next_file;
	}

	// This file name is not known.
	return(NULL);
}

//-------------------------------------------------------------------------
//  ====================  TPathHelper  =========================
//-------------------------------------------------------------------------

bool TPathHelper::IsAbsolutePath(const wchar_t *path)
{
	// Method expects that the pointer itself is not NULL.
	if (path == NULL || path[0] == 0)
		return(FALSE);		// Bogus path.

	if (wcsstr(path, L":\\") != NULL || wcsstr(path, L":/") != NULL)
		return(TRUE);		// Full path on some local drive.

	// This type of check is still safe in case of the short strings.
	if ((path[0] == L'/' && path[1] == L'/') || (path[0] == L'\\' && path[1] == L'\\'))
		return(TRUE);		// Full path on some remote share.

	return(FALSE);
}

bool TPathHelper::IsIntermediatePath(const wchar_t *path)
{
	return(IsAbsolutePath(path) == FALSE && IsRelativePath(path) == FALSE);
}

bool TPathHelper::IsRelativePath(const wchar_t *path)
{
	// Method expects that the pointer itself is not NULL.
	if (path == NULL || path[0] == 0)
		return(FALSE);		// Bogus path.

	if (wcschr(path, L':') != NULL)
		return(FALSE);		// Path refers to some drive.

	if (path[0] == L'/' || path[0] == L'\\')
		return(FALSE);		// Path refers to the root directory.

	return(TRUE);
}

bool TPathHelper::IsPathADir(const wchar_t *path)
{
	DWORD attrs = ::GetFileAttributesW(path);
	if (attrs == INVALID_FILE_ATTRIBUTES)
	{
		// The path doesn't exist.
		return(FALSE);
	}

	// Path exists, check if it corresponds a file or a directory.
	if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
	{
		// This is a directory.
		return(TRUE);
	}

	// This is a file.
	return(FALSE);
}

bool TPathHelper::IsPathAFile(const wchar_t *path)
{
	DWORD attrs = ::GetFileAttributesW(path);
	if (attrs == INVALID_FILE_ATTRIBUTES)
	{
		// The path doesn't exist.
		return(FALSE);
	}

	// Path exists, check if it corresponds a file or a directory.
	if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
	{
		// This is a directory.
		return(FALSE);
	}

	// This is a file.
	return(TRUE);
}

void TPathHelper::ConvertToBackSlashesInPlace(wchar_t *path)
{
	// This method should crash on a NULL parameter.
	while (*path != 0)
	{
		if (*path == L'/')
			*path = L'\\';
		path++;
	}
}

void TPathHelper::	RemoveTrailingSlashInPlace(wchar_t *path)
{
	// This method should crash on a NULL parameter.
	if (wcscmp(path, L".\\") == 0 || wcscmp(path, L"./") == 0)
	{
		*path = 0;
		return;
	}

	int len = wcslen(path);
	if (len > 0)
	{
		// The path is non trivial.
		if (path[len-1] == L'\\' || path[len-1] == L'/')
		{
			path[len-1] = 0;
		}
	}
}

void TPathHelper::ConvertToBackSlashes(TFileNameBuffer &buffer)
{
	int len = buffer.NumItems();
	for (int inx=0; inx<len; ++inx)
	{
		if (buffer[inx] == L'/')
			buffer[inx] = L'\\';
	}
}

void TPathHelper::RemoveTrailingSlash(TFileNameBuffer &buffer)
{
	int len = buffer.NumItems();
	if (buffer.GetXpndError() == FALSE && len > 0)
	{
		if (buffer == L".\\" || buffer == L"./")
		{
			buffer.ClearBuffer();
		}
		else if (buffer[len-1] == L'\\' || buffer[len-1] == L'/')
		{
			buffer[len-1] = 0;
			buffer.IncNumItems(-1);
		}
	}
}

bool TPathHelper::EnsureTrailingSlash(TFileNameBuffer &buffer)
{
	int len = buffer.NumItems();
	if (buffer.GetXpndError() == FALSE && len > 0)
	{
		if (buffer[len-1] != L'\\' && buffer[len-1] != L'/')
		{
			buffer.Append(L'\\');
		}
	}

	return(buffer.GetXpndError() == FALSE);
}

void TPathHelper::RemoveExtensionAndDot(TFileNameBuffer &buffer)
{
	wchar_t *buff = buffer.DataPtr();
	wchar_t *pnt_dot = wcsrchr(buff, L'.');
	if (pnt_dot != NULL)
	{
		// The extension is present in the passed file name.
		int new_len = (int)(pnt_dot-buff);
		buffer[new_len] = 0;
		buffer.SetNumItems(new_len);
	}
}

bool TPathHelper::GetDirectoryName(TFileNameBuffer &buffer, const wchar_t *file_name)
{
	buffer.ClearBuffer();
	buffer.Append(file_name);
	if (buffer.GetXpndError() == TRUE)
		return(FALSE);

	wchar_t *buff = buffer.DataPtr();
	wchar_t *pnt_xsl = GetLastBackSlashPtr(buff);

	if (pnt_xsl == NULL || pnt_xsl == buff)
	{
		// The name is short or the passed path is not absolute.
		return(FALSE);
	}

	if (pnt_xsl == buff+1 && (buff[0] == L'/' || buff[0] == L'\\'))
	{
		// Passed name contains only the name of some remote box.
		return(FALSE);
	}

	int new_len = (int)(pnt_xsl-buff);
	buffer[new_len] = 0;
	buffer.SetNumItems(new_len);
	return(TRUE);
}

bool TPathHelper::ExtractShortName(TFileNameBuffer &buffer, const wchar_t *file_name, bool remove_extension_and_dot)
{
	buffer.ClearBuffer();

	wchar_t *pnt_xsl = GetLastBackSlashPtr(file_name);
	if (pnt_xsl == NULL)
		pnt_xsl = wcsrchr((wchar_t*)file_name, L':');

	if (pnt_xsl == NULL)
	{
		// Passed path looks to be already a short name.
		buffer.Append(file_name);
	}
	else
	{
		// Short name should start right after the delimiter.
		buffer.Append(pnt_xsl+1);
	}

	if (remove_extension_and_dot == TRUE)
		RemoveExtensionAndDot(buffer);

	if (buffer.GetXpndError() == TRUE)
		return(FALSE);

	return(buffer.NumItems() > 0);
}

const wchar_t *TPathHelper::GetShortNamePtr(const wchar_t *file_name)
{
	wchar_t *pnt_xsl = GetLastBackSlashPtr(file_name);
	if (pnt_xsl == NULL)
		pnt_xsl = wcsrchr((wchar_t*)file_name, L':');
	return((pnt_xsl != NULL) ? pnt_xsl+1 : file_name);
}

const wchar_t *TPathHelper::GetExtensionPtr(const wchar_t *file_name)
{
	wchar_t *curr_ext = wcsrchr((wchar_t*)file_name, L'.');
	return((curr_ext != NULL) ? curr_ext : file_name+wcslen(file_name));
}

bool TPathHelper::ConstructPath(TFileNameBuffer &buffer, const wchar_t *base_dir, const wchar_t *relative_path, bool relative_path_is_file)
{
	assert(base_dir != NULL);
	assert(relative_path != NULL);

	buffer.ClearBuffer();
	if (*base_dir == 0 || relative_path[0] == 0)
		return(FALSE);
	if (IsRelativePath(relative_path) == FALSE)
		return(FALSE);

	// Put base direcory into the buffer.
	buffer.Append(base_dir);
	RemoveTrailingSlash(buffer);
	if (buffer.GetXpndError() == TRUE)
		return(FALSE);

	// Process special prefixes in the third param.
	wchar_t *relative = (wchar_t*)relative_path;
	while (relative[0] == L'.')
	{
		if (relative[1] == 0)
		{
			// End of the path.
			return((relative_path_is_file == TRUE) ? FALSE : TRUE);
		}
		else if (relative[1] == L'/' || relative[1] == L'\\')
		{
			// Old style reference to the current dir.
			relative += 2;
		}
		else if (relative[1] == L'.' && (relative[2] == 0 || relative[2] == L'/' || relative[2] == L'\\'))
		{
			//
			// This is a step up in the directory structure.
			//

			// Consume prefix chars.
			if (relative[2] == 0)
				relative += 2;
			else relative += 3;

			int base_dir_len = buffer.NumItems();
			if (base_dir_len == 0)
				return(FALSE);

			wchar_t ch = buffer[base_dir_len-1];
			if (ch == L'\\' || ch == L'/' || ch == L':')
				return(FALSE);

			while (base_dir_len > 0)
			{
				ch = buffer[--base_dir_len];
				if (ch == L'\\' || ch == L'/' || ch == L':')
					break;
			}

			ch = buffer[0];
			if (base_dir_len == 0 || base_dir_len == 1 && (ch == L'\\' || ch == L'/'))
				return(FALSE);
			if (buffer[base_dir_len] == L':')
				base_dir_len++;

			// Accept new length of the base dir.
			buffer[base_dir_len] = 0;
			buffer.SetNumItems(base_dir_len);
		}
		else
		{
			// End of the special prefixes.
			break;
		}
	}

	// Check that relative path is still not empty.
	if (wcslen(relative) == 0)
		return((relative_path_is_file == TRUE) ? FALSE : TRUE);

	// Append the final piece of the relative path.
	buffer.Append(L"\\");
	buffer.Append(relative);
	RemoveTrailingSlash(buffer);

	// Ret value should be TRUE if there was no memory allocation error.
	return(buffer.GetXpndError() == FALSE);
}

bool TPathHelper::GetExeDirectoryName(wchar_t *buffer, int buffer_len)
{
	assert(buffer_len > 10);
	int nChrs = ::GetModuleFileNameW(NULL, buffer, buffer_len);
	if (nChrs == 0)
		return(FALSE);

	// In case of the buffer overflow the GetModuleFileName() will leave the buffer non NULL terminated.
	if (nChrs >= buffer_len)
		buffer[buffer_len-1] = 0;

	wchar_t *dir_end = wcsrchr(buffer, L'\\');
	if (dir_end == NULL)
		buffer[0] = 0;
	else dir_end[1] = 0;

	// Success.
	return(TRUE);
}

bool TPathHelper::PrepareDestDirectory(wchar_t *err_msg_buffer, int err_msg_buffer_len, TFileNameBuffer &dest_buffer, const wchar_t *subdir_name, const wchar_t *reference_file_name, const wchar_t *reference_file_purpose)
{
	assert(TPathHelper::IsEmptyPath(reference_file_purpose) == FALSE);

	dest_buffer.ClearBuffer();
	if (TPathHelper::IsEmptyPath(subdir_name) == TRUE || TPathHelper::IsRelativePath(subdir_name) == TRUE)
	{
		// Start from directory of the reference file.
		if (TPathHelper::IsEmptyPath(reference_file_name) == TRUE)
		{
			swprintf(err_msg_buffer, err_msg_buffer_len, L"Name of the %s is empty. Unable to pick up the directory name.", reference_file_purpose);
			return(FALSE);
		}

		if (TPathHelper::GetDirectoryName(dest_buffer, reference_file_name) == FALSE)
		{
			swprintf(err_msg_buffer, err_msg_buffer_len, L"Error getting directory name from the %s name: \"%s\".", reference_file_purpose, reference_file_name);
			return(FALSE);
		}

		if (TPathHelper::IsEmptyPath(subdir_name) == FALSE)
		{
			TFileNameBuffer buffer;
			if (TPathHelper::ConstructPath(buffer, dest_buffer, subdir_name, FALSE) == FALSE)
			{
				swprintf(err_msg_buffer, err_msg_buffer_len, L"Error constructing directory name from \"%s\" and \"%s\".", dest_buffer.DataPtr(), subdir_name);
				return(FALSE);
			}

			// Constuction succeeded. Copy the data back.
			dest_buffer.TakeContentsFrom(buffer);
		}
	}
	else
	{
		// Use the passed name of the directory as-is.
		if (dest_buffer.Append(subdir_name) == FALSE)
		{
			swprintf(err_msg_buffer, err_msg_buffer_len, L"Passed subdirectory name for the generated files is too long.");
			return(FALSE);
		}
	}

	// Check/create the destination directory.
	if (TPathHelper::IsPathADir(dest_buffer) == FALSE && ::CreateDirectoryW(dest_buffer, NULL) == FALSE)
	{
		DWORD err = ::GetLastError();
		swprintf(err_msg_buffer, err_msg_buffer_len, L"Error creating destination directory: \"%s\". Win32 error: %d.", dest_buffer.DataPtr(), err);
		return(FALSE);
	}

	// Name is prepared and directory is existing.
	return(TRUE);
}

bool TPathHelper::PrepareDestFile(wchar_t *err_msg_buffer, int err_msg_buffer_len, TDestinationFile &rprt, const wchar_t *dir_name, const wchar_t *file_name_prefix, const wchar_t *file_name_proto, const wchar_t *file_name_suffix, const wchar_t *destination_file_purpose)
{
	assert(TPathHelper::IsEmptyPath(destination_file_purpose) == FALSE);

	// Pick up the short name of the original source file.
	TFileNameBuffer short_fname;
	if (TPathHelper::ExtractShortName(short_fname, file_name_proto) == FALSE)
	{
		swprintf(err_msg_buffer, err_msg_buffer_len, L"Error getting the short name from: \"%s\".", file_name_proto);
		return(FALSE);
	}

	// Construct the short name for the destination file.
	TPathHelper::RemoveExtensionAndDot(short_fname);
	if (TPathHelper::IsEmptyPath(file_name_prefix) == FALSE)
		short_fname.Insert(0, file_name_prefix);

	if (TPathHelper::IsEmptyPath(file_name_suffix) == FALSE)
		short_fname.Append(file_name_suffix);
	else short_fname.Append(L".cxx");

	if (short_fname.GetXpndError() == TRUE)
	{
		swprintf(err_msg_buffer, err_msg_buffer_len, L"Error constucting the short name for the %s.", destination_file_purpose);
		return(FALSE);
	}

	// Combine all parts of the path together.
	TFileNameBuffer rprt_fname_buffer;
	rprt_fname_buffer.Append(dir_name);
	rprt_fname_buffer.Append(L'\\');
	rprt_fname_buffer.Append(short_fname);

	if (rprt_fname_buffer.GetXpndError() == TRUE)
	{
		swprintf(err_msg_buffer, err_msg_buffer_len, L"Error constucting the path for the %s.", destination_file_purpose);
		return(FALSE);
	}

	// The file name is prepared. Open the file.
	rprt.SetFileName(rprt_fname_buffer);
	if (rprt.PrepareDiskFile() == FALSE)
	{
		swprintf(err_msg_buffer, err_msg_buffer_len, L"Error opening for writing the %s: \"%s\".", destination_file_purpose, rprt.FileName());
		return(FALSE);
	}

	// The disk file is opened.
	return(TRUE);
}

bool TPathHelper::CloseDestFile(wchar_t *err_msg_buffer, int err_msg_buffer_len, TDestinationFile &rprt)
{
	err_msg_buffer[0] = 0;
	if (rprt.m_stt == dstfs_writing_err)
	{
		swprintf(err_msg_buffer, err_msg_buffer_len, L"Error buffering data or writing to the \"%s\".", rprt.FileName());
	}

	bool res = rprt.Close();
	if (err_msg_buffer[0] == 0 && res == FALSE)
	{
		swprintf(err_msg_buffer, err_msg_buffer_len, L"Error closing the \"%s\".", rprt.FileName());
	}

	return(res);
}

DWORD TPathHelper::CreateSubDirectory(const wchar_t *directory_name)
{
	// Use simple approach for now.
	return(ERROR_SUCCESS);
}

DWORD TPathHelper::DeleteAllFilesFromDirectory(const wchar_t *directory_name)
{
	TFileNameBuffer fname_buff;
	if (fname_buff.Append(directory_name) == FALSE)
		return(ERROR_NOT_ENOUGH_MEMORY);
	else if (TPathHelper::EnsureTrailingSlash(fname_buff) == FALSE)
		return(ERROR_NOT_ENOUGH_MEMORY);

	int dir_len = fname_buff.NumItems();

	TDirectoryIterator iter;
	for (iter.StartIteration(directory_name, FALSE, TRUE); iter; ++iter)
	{
		fname_buff.TruncateTo(dir_len);
		if (fname_buff.Append(iter.ShortFileName()) == FALSE)
			return(ERROR_NOT_ENOUGH_MEMORY);

		BOOL res = ::DeleteFileW(fname_buff);
		if (res == 0)
		{
			DWORD win32_err = ::GetLastError();
			break;
		}
	}

	// Return the result whatever it is.
	return(iter.IterationResuilt());
}

wchar_t *TPathHelper::GetLastBackSlashPtr(const wchar_t *buffer)
{
	wchar_t *pnt_xsl = wcsrchr((wchar_t*)buffer, L'\\');
	wchar_t *pnt_xsl_forw = wcsrchr((wchar_t*)buffer, L'/');

	if (pnt_xsl == NULL)
	{
		// Back slashes are missing. Use info on the forward slashes instead.
		pnt_xsl = pnt_xsl_forw;
	}
	else if (pnt_xsl_forw != NULL)
	{
		// Both types of delimiters are present.
		if (pnt_xsl_forw > pnt_xsl)
			pnt_xsl = pnt_xsl_forw;
	}

	return(pnt_xsl);
}

//-------------------------------------------------------------------------
//  ===================  TDirectoryIterator  ======================
//-------------------------------------------------------------------------

TDirectoryIterator::TDirectoryIterator()
{
	m_include_subdirectories = m_include_files = FALSE;
	m_search_handle = INVALID_HANDLE_VALUE;
	m_last_error = ERROR_SUCCESS;
}

void TDirectoryIterator::StartIteration(const wchar_t *directory_name, bool want_subdirectories, bool want_files)
{
	assert(TPathHelper::IsEmptyPath(directory_name) == FALSE);

	if (m_search_handle != INVALID_HANDLE_VALUE)
	{
		::FindClose(m_search_handle);
		m_search_handle = INVALID_HANDLE_VALUE;
	}

	TFileNameBuffer dir_name_buff;
	if (dir_name_buff.Append(directory_name) == TRUE)
	{
		StartIteration(dir_name_buff, want_subdirectories, want_files);
	}
	else
	{
		// This should be a rare situation. Nevertheless report this.
		m_last_error = ERROR_NOT_ENOUGH_MEMORY;
	}
}

void TDirectoryIterator::StartIteration(TFileNameBuffer &directory_name_buffer, bool want_subdirectories, bool want_files)
{
	assert(directory_name_buffer.NumItems() > 0);

	if (m_search_handle != INVALID_HANDLE_VALUE)
	{
		::FindClose(m_search_handle);
		m_search_handle = INVALID_HANDLE_VALUE;
	}

	m_include_subdirectories = want_subdirectories;
	m_include_files = want_files;
	m_last_error = ERROR_SUCCESS;

	// Prepare directory name with the wild cards.
	int inital_len = directory_name_buffer.NumItems();
	if (directory_name_buffer.Append(L"\\*.*") == FALSE)
	{
		// Iteration failed to start. Although this should be extremely rare situation.
		m_last_error = ERROR_NOT_ENOUGH_MEMORY;
		return;
	}

	// Start the iteration.
	m_search_handle = ::FindFirstFileW(directory_name_buffer, &m_file_info);
	if (m_search_handle == INVALID_HANDLE_VALUE)
	{
		// Iteration failed to start. This cannot be an empty directory case because the low level iteration
		// should always return at least useless "." and ".." elements.
		m_last_error = ::GetLastError();
	}
	else
	{
		// Win32 iterator has returned something. Navigate to the next appropriate record.
		while (CheckCurrentFileInfo() == FALSE)
		{
			if (StepIteration() == FALSE)
				break;
		}
	}

	// Restore the original state of directory name.
	directory_name_buffer.TruncateTo(inital_len);
}

bool TDirectoryIterator::StepIteration()
{
	if (m_search_handle == INVALID_HANDLE_VALUE)
		return(FALSE);

	if (::FindNextFileW(m_search_handle, &m_file_info) == 0)
	{
		m_last_error = ::GetLastError();
		::FindClose(m_search_handle);
		m_search_handle = INVALID_HANDLE_VALUE;

		if (m_last_error == ERROR_NO_MORE_FILES)
			m_last_error = ERROR_SUCCESS;

		// Iteration is complete.
		return(FALSE);
	}

	// Next record is retrieved.
	return(TRUE);
}

bool TDirectoryIterator::CheckCurrentFileInfo()
{
	assert(m_search_handle != INVALID_HANDLE_VALUE);
	DWORD attr_dir_flag = m_file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

	if (attr_dir_flag != 0 && (wcscmp(m_file_info.cFileName, L".") == 0 || wcscmp(m_file_info.cFileName, L"..") == 0))
		return(FALSE);

	if (attr_dir_flag != 0 && m_include_subdirectories == FALSE)
		return(FALSE);
	else if (attr_dir_flag == 0 && m_include_files == FALSE)
		return(FALSE);

	// Directory record seems to be fine.
	return(TRUE);
}

//-------------------------------------------------------------------------
//  =================  TDirectoriesSubtreeIterator  ==================
//-------------------------------------------------------------------------

bool TDirectoriesSubtreeIterator::Iterate(const wchar_t *subdir_name, bool process_subdirectories, bool want_callback_for_root_dir_name)
{
	// Passed directory name should not be empty.
	assert(TPathHelper::IsEmptyPath(subdir_name) == FALSE);

	m_abort_iteration = FALSE;
	m_last_error = ERROR_SUCCESS;

	if (m_curr_name_buffer.Append(subdir_name) == FALSE)
	{
		// This should be a rare situation. Nevertheless report this.
		m_last_error = ERROR_NOT_ENOUGH_MEMORY;
		return(FALSE);
	}

	if (want_callback_for_root_dir_name == TRUE)
	{
		// Call the virtual method.
		ProcessSubdirectory(m_curr_name_buffer);
	}

	// Call the recursive worker method.
	IterateObjectsInternal(process_subdirectories);
	return(m_last_error == ERROR_SUCCESS);
}

void TDirectoriesSubtreeIterator::IterateObjectsInternal(bool process_subdirectories)
{
	assert(m_curr_name_buffer.NumItems() > 0);
	if (m_curr_name_buffer.Append(L"\\") == FALSE)
	{
		m_last_error = ERROR_NOT_ENOUGH_MEMORY;
		return;
	}

	TDirectoryIterator iter;
	int inital_len = m_curr_name_buffer.NumItems();

	// Iterate files of the current directory as the first pass.
	for (iter.StartIteration(m_curr_name_buffer, FALSE, TRUE); iter; ++iter)
	{
		// Call the first worker method for file.
		ProcessFileShortName(m_curr_name_buffer, iter.ShortFileName());

		// Prepare the full name of the file.
		if (m_curr_name_buffer.Append(iter.ShortFileName()) == FALSE)
		{
			m_last_error = ERROR_NOT_ENOUGH_MEMORY;
			break;
		}

		// Call the second worker method for file.
		ProcessFileFullName(m_curr_name_buffer);

		// Restore the name of the current directory back.
		m_curr_name_buffer.TruncateTo(inital_len);

		// Check for the app level abort from the virtual methods.
		if (m_abort_iteration == TRUE)
			break;
	}

	if (process_subdirectories == TRUE && m_abort_iteration == FALSE && m_last_error == ERROR_SUCCESS)
	{
		// Do the second pass that will iterate the subdirectories.
		for (iter.StartIteration(m_curr_name_buffer, TRUE, FALSE); iter; ++iter)
		{
			// Prepare the full name of the subdirectory.
			if (m_curr_name_buffer.Append(iter.ShortFileName()) == FALSE)
			{
				m_last_error = ERROR_NOT_ENOUGH_MEMORY;
				break;
			}

			// Call the virtual method.
			ProcessSubdirectory(m_curr_name_buffer);

			// Iterate contents of the subdirectory.
			IterateObjectsInternal(TRUE);
			if (m_last_error != ERROR_SUCCESS)
				break;

			// Restore the name of the current directory back.
			m_curr_name_buffer.TruncateTo(inital_len);

			// Check for the app level abort from the virtual method.
			if (m_abort_iteration == TRUE)
				break;
		}
	}

	// Pick up the low level result for the current layer.
	if (m_last_error == ERROR_SUCCESS)
		m_last_error = iter.IterationResuilt();

	// Restore the name of the current directory once again.
	m_curr_name_buffer.TruncateTo(inital_len-1);
}

//-------------------------------------------------------------------------
//  ====================  TDestinationFile  =======================
//-------------------------------------------------------------------------

TDestinationFile::TDestinationFile(const wchar_t *name)
{
	m_stt = dstfs_nodata;
	SetFileName((name != NULL) ? name : L"");

	m_file_date = 0;
	m_file_handle = INVALID_HANDLE_VALUE;
	m_mem_body = NULL;

	m_win32_error = 0;
	m_disk_file_buff_data_len = 0;
}

TDestinationFile::~TDestinationFile()
{
	if (m_file_handle != INVALID_HANDLE_VALUE)
	{
		if (m_disk_file_buff_data_len > 0)
		{
			// Dump remaining contents of the buffer.
			DWORD dwBytesWritten;
			::WriteFile(m_file_handle, m_disk_file_buffer, m_disk_file_buff_data_len, &dwBytesWritten, NULL);
		}

		::CloseHandle(m_file_handle);
	}

	if (m_mem_body != NULL)
		free(m_mem_body);
}

bool TDestinationFile::PrepareDiskFile(HANDLE opened_file, bool convert_to_ascii)
{
	if (IsWritingState() == TRUE)
		return(FALSE);

	if (opened_file == NULL && m_file_name[0] == 0)
	{
		m_stt = dstfs_nodata;
		return(FALSE);
	}

	assert(m_file_handle == INVALID_HANDLE_VALUE);

	m_mem_body_len = 0;
	if (m_mem_body != NULL)
	{
		free(m_mem_body);
		m_mem_body = NULL;
	}

	m_num_lines = 0;
	m_ascii_mode  = convert_to_ascii;

	if (opened_file == NULL)
	{
		// Open the file.
		m_file_handle = ::CreateFileW(m_file_name,
								GENERIC_WRITE | GENERIC_READ | STANDARD_RIGHTS_ALL,
								0,					// No sharing.
								NULL,				// Security attributes.
								OPEN_ALWAYS,		// Open existing or create new file.
								0,					// No ext misc flags.
								NULL);				// Template.

		m_win32_error = ::GetLastError();

		if (m_file_handle == INVALID_HANDLE_VALUE)
		{
			m_stt = dstfs_nodata;
			return(FALSE);
		}
	}
	else
	{
		// Use the passed handle.
		m_file_handle = opened_file;
		m_file_name[0] = 0;
		::SetFilePointer(m_file_handle, 0, NULL, FILE_BEGIN);
	}

	m_file_len = 0;
	::SetEndOfFile(m_file_handle);
	m_stt = dstfs_writing;
	return(TRUE);
}

bool TDestinationFile::PrepareInMemoryStream()
{
	if (IsWritingState() == TRUE)
		return(FALSE);

	assert(m_mem_body == NULL);

	m_ascii_mode = FALSE;
	if (m_file_handle != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_file_handle);
		m_file_handle = INVALID_HANDLE_VALUE;
	}

	m_num_lines = 0;

	if (m_mem_body == NULL)
	{
		m_mem_body_len = 0x10000;
		m_mem_body = (wchar_t*)malloc(m_mem_body_len*sizeof(wchar_t));
		if (m_mem_body == NULL)
		{
			// This state of the object is also an error state.
			m_stt = dstfs_nodata;
			return(FALSE);
		}
	}

	m_file_len = 0;
	m_stt = dstfs_writing;
	return(TRUE);
}

void TDestinationFile::Write(const wchar_t *data, long data_len)
{
	//
	// This is low level method. All other writing methods come here.
	//
	if (IsWritingState() == FALSE || data_len == 0)
		return;

	if (data_len < 0)
		data_len = (long)wcslen(data);

	// Update the lines count.
	for (int i=0; i<data_len; ++i)
	{
		if (data[i] == L'\n')
		m_num_lines++;
	}

	if (m_mem_body != NULL)
	{
		// This is an in memory stream.
		if (m_file_len+data_len >= m_mem_body_len)
		{
			// Increase size of the body.
			long new_len = m_file_len+data_len+0x10000;
			wchar_t *new_body = (wchar_t*)realloc(m_mem_body, new_len*sizeof(wchar_t));
			if (new_body == NULL)
			{
				m_stt = dstfs_writing_err;
				return;
			}
			m_mem_body = new_body;
			m_mem_body_len = new_len;
		}

		// There is enough space in the buffer.
		memcpy(m_mem_body+m_file_len, data, data_len*sizeof(wchar_t));
		m_file_len += data_len;
		return;
	}

	// This is a disk file.
	if (m_ascii_mode == TRUE)
	{
		char *buffer = (char*)m_message_buff;
		if (data_len > MESSAGE_BUFF_LEN)
		{
			buffer = (char*)malloc(data_len);
			if (buffer == NULL)
			{
				m_stt = dstfs_writing_err;
				return;
			}
		}

		// Convert the data from Unicode to ASCII. Function ConvertToAsciiInPlace() cannot be used
		// here because this code copies the data into the internal buffer during conversion.
		for (long inx=0; inx<data_len; ++inx)
			buffer[inx] = (char)data[inx];

		// Pass data to the data buffering layer.
		WriteToDiskFile((BYTE*)buffer, data_len);

		if (buffer != (char*)m_message_buff)
			free(buffer);
	}
	else
	{
		// Pass data to the data buffering layer.
		WriteToDiskFile((BYTE*)data, data_len*sizeof(wchar_t));
	}

	// Advance the file length in case of the disk file.
	m_file_len += data_len;
}

void TDestinationFile::WriteLine(const wchar_t *data, long data_len)
{
	Write(data, data_len);
	Write(L"\r\n", 2);
}

void TDestinationFile::WriteFmt(const wchar_t *format, ...)
{
	if (IsWritingState() == FALSE)
		return;

	// Generate UNICODE string from the format and params.
	va_list vargs;
	va_start(vargs, format);
	int nch_written = vswprintf(m_message_buff, MESSAGE_BUFF_LEN, format, vargs);
	va_end(vargs);
	if (nch_written < 0 || nch_written == MESSAGE_BUFF_LEN)
	{
		m_stt = dstfs_writing_err;
	}

	// Send the string to the file.
	Write(m_message_buff, nch_written);
}

void TDestinationFile::WriteFmtLine(const wchar_t *format, ...)
{
	if (IsWritingState() == FALSE)
		return;

	// Generate UNICODE string from format and params.
	va_list vargs;
	va_start(vargs, format);
	int nch_written = vswprintf(m_message_buff, MESSAGE_BUFF_LEN, format, vargs);
	va_end(vargs);
	if (nch_written < 0 || nch_written == MESSAGE_BUFF_LEN)
	{
		m_stt = dstfs_writing_err;
	}

	// Send the string to the file.
	WriteLine(m_message_buff, nch_written);
}

bool TDestinationFile::Close()
{
	if (IsWritingState() == FALSE)
		return(FALSE);

	if (m_mem_body != NULL)
	{
		// This is in memory stream.
		m_file_date = CurrDateTime();
	}
	else
	{
		// This is a disk file.
		if (m_disk_file_buff_data_len > 0)
		{
			// Dump current contents of the buffer.
			DWORD dwBytesWritten;
			if (::WriteFile(m_file_handle, m_disk_file_buffer, m_disk_file_buff_data_len, &dwBytesWritten, NULL) == FALSE)
				m_stt = dstfs_writing_err;

			m_disk_file_buff_data_len = 0;
		}

		BY_HANDLE_FILE_INFORMATION file_info;
		if (::GetFileInformationByHandle(m_file_handle, &file_info) == 0)
		{
			m_file_len = 0;
			m_file_date = CurrDateTime();
			m_stt = dstfs_writing_err;
		}
		else
		{
			m_file_len = file_info.nFileSizeLow;
			m_file_date = MakeDateTime(file_info.ftLastWriteTime);
		}

		// Refresh the file name in the fname buffer.
		GetExactFileName();

		if (::CloseHandle(m_file_handle) == 0)
			m_stt = dstfs_writing_err;

		m_file_handle = INVALID_HANDLE_VALUE;
	}

	m_stt = (m_stt == dstfs_writing) ? dstfs_closed : dstfs_closed_err;
	return((m_stt == dstfs_closed) ? TRUE : FALSE);
}

bool TDestinationFile::DeleteClosedDiskFile()
{
	if (m_stt != dstfs_closed)
	{
		// State is not correct for this operation.
		return(FALSE);
	}

	if (::DeleteFileW(m_file_name) == FALSE)
	{
		m_win32_error = ::GetLastError();
		m_stt = dstfs_closed_err;
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TDestinationFile::WipeDiskFile()
{
	if (m_stt == dstfs_closed)
	{
		// Simple case.
		return(DeleteClosedDiskFile());
	}

	// Create non empty disk file with the current file name and delete it.
	PrepareDiskFile();
	Write(L"...", 3);
	Close();
	return(DeleteClosedDiskFile());
}

bool TDestinationFile::GetBasicFileInfo(TBasicFileInfo &info, bool dup_body, TDestinationFileState *pStt)
{
	if (pStt != NULL)
		*pStt = m_stt;
	if (m_stt != dstfs_closed && m_stt != dstfs_closed_err)
		return(FALSE);

	TLoadSaveResult res = ldres_success;
	if (m_mem_body != NULL)
	{
		// This is an in memory stream.
		if (dup_body == TRUE)
		{
			res = TFileBodyHelper::SetupInMemFileBody(info, m_file_name, m_mem_body, m_file_len, m_file_date);
		}
		else
		{
			// Fill in the structure directly.
			memset(&info, 0, sizeof(TBasicFileInfo));

			// Duplicate the name of the file.
			info.file_name = (wchar_t*)malloc((wcslen(m_file_name)+1)*sizeof(wchar_t));
			if (info.file_name == NULL)
				return(FALSE);

			wcscpy(info.file_name, m_file_name);

			info.file_body = m_mem_body;
			info.file_len = m_file_len;
			info.file_date = m_file_date;

			// Kill contents of the object.
			m_mem_body = NULL;
			m_stt = dstfs_nodata;
		}
	}
	else
	{
		// This is a disk file.
		res = TFileBodyHelper::SetupInMemFileBody(info, m_file_name, NULL, m_file_len, m_file_date);
	}

	return((res == ldres_success) ? TRUE : FALSE);
}

void TDestinationFile::TruncateTo(long new_len, long new_num_lines)
{
	if (m_stt != dstfs_writing)
		return;

	if (new_len < 0 || new_len > m_file_len)
	{
		// Bogus value of the new length.
		m_stt = dstfs_writing_err;
		return;
	}

	if (new_len == m_file_len)
	{
		// Simply update the lines count.
		m_num_lines = new_num_lines;
		return;
	}

	if (m_mem_body != NULL)
	{
		// This is in memory stream.
		// Value of the new length is already checked.
		m_file_len = new_len;
		m_num_lines = new_num_lines;
	}
	else
	{
		// Update length of the file on disk.
		::SetFilePointer(m_file_handle, (m_ascii_mode == TRUE) ? new_len : new_len*sizeof(wchar_t), NULL, FILE_BEGIN);
		DWORD err = ::GetLastError();
		if (err != ERROR_SUCCESS && err != ERROR_ALREADY_EXISTS)
		{
			m_stt = dstfs_writing_err;
		}
		else
		{
			if (::SetEndOfFile(m_file_handle) == FALSE)
			{
				m_stt = dstfs_writing_err;
			}
			else
			{
				// Both actions succeeded.
				m_file_len = new_len;
				m_num_lines = new_num_lines;
			}
		}
	}
}

void TDestinationFile::WriteToDiskFile(BYTE *data, long data_len)
{
	if (data_len < (DISK_FILE_BUFF_LEN-m_disk_file_buff_data_len))
	{
		// Size of the passed data is not big. It fits into the free space in the buffer.
		memcpy(m_disk_file_buffer+m_disk_file_buff_data_len, data, data_len);
		m_disk_file_buff_data_len += data_len;
	}
	else if (data_len < DISK_FILE_BUFF_LEN)
	{
		// The size of the data is intermediate.
		assert(m_disk_file_buff_data_len > 0);

		// Top up the buffer.
		long free_space_size = (DISK_FILE_BUFF_LEN-m_disk_file_buff_data_len);
		memcpy(m_disk_file_buffer+m_disk_file_buff_data_len, data, free_space_size);
		data += free_space_size;
		data_len -= free_space_size;

		// Dump the buffer.
		DWORD dwBytesWritten;
		if (::WriteFile(m_file_handle, m_disk_file_buffer, DISK_FILE_BUFF_LEN, &dwBytesWritten, NULL) == FALSE)
			m_stt = dstfs_writing_err;

		// Process the rest of the data if any.
		if (data_len > 0)
			memcpy(m_disk_file_buffer, data, data_len);

		m_disk_file_buff_data_len = data_len;
	}
	else
	{
		// Caller wants to write big amount of data.
		if (m_disk_file_buff_data_len > 0)
		{
			// Dump current contents of the buffer.
			DWORD dwBytesWritten;
			if (::WriteFile(m_file_handle, m_disk_file_buffer, m_disk_file_buff_data_len, &dwBytesWritten, NULL) == FALSE)
				m_stt = dstfs_writing_err;
		}

		while (data_len >= DISK_FILE_BUFF_LEN)
		{
			// Write data directly from the buffer of the upper layer.
			DWORD dwBytesWritten;
			if (::WriteFile(m_file_handle, data, DISK_FILE_BUFF_LEN, &dwBytesWritten, NULL) == FALSE)
				m_stt = dstfs_writing_err;

			data += DISK_FILE_BUFF_LEN;
			data_len -= DISK_FILE_BUFF_LEN;
		}

		// Place rest of the data into the buffer if any.
		if (data_len > 0)
			memcpy(m_disk_file_buffer, data, data_len);

		m_disk_file_buff_data_len = data_len;
	}
}

void TDestinationFile::GetExactFileName()
{
	if (m_file_len == 0)
	{
		// File with zero length cannot be mapped.
		return;
	}

	// Create the file mapping object.
	HANDLE hFileMap = ::CreateFileMapping(m_file_handle, NULL, PAGE_READONLY, 0, 1, NULL);
	if (hFileMap == NULL)
	{
		// Some error happened. Moist likely file handle has no read access.
		DWORD err = ::GetLastError();
		return;
	}

	// Create memory mapping to get the file name.
	void *pMemArea = ::MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);
	if (pMemArea != NULL)
	{
		wchar_t fname_buffer[MAX_PATH];
		if (::GetMappedFileNameW(::GetCurrentProcess(), pMemArea, fname_buffer, MAX_PATH-1) != 0)
		{
			// Convert device name into drive letters. Pick up list of disk drives in the system.
			bool fn_ready = FALSE;
			wchar_t drive_names[MAX_PATH];
			if (::GetLogicalDriveStringsW(MAX_PATH-1, drive_names) != 0)
			{
				// Iterate all retrieved drives.
				int drv_name_len = 0;
				for (wchar_t *drive=drive_names; *drive != 0; drive += drv_name_len)
				{
					// Remove root directory char from the drive name.
					drv_name_len = (int)wcslen(drive)+1;
					assert(drv_name_len >= 2);

					if (drive[drv_name_len-2] == L'\\' || drive[drv_name_len-2] == L'/')
						drive[drv_name_len-2] = 0;

					// Pick up device name for the current drive.
					wchar_t device_name[MAX_PATH];
					if (::QueryDosDeviceW(drive, device_name, MAX_PATH) != 0)
					{
						int dev_name_len = (int)wcslen(device_name);
						if (dev_name_len < MAX_PATH && _wcsnicmp(fname_buffer, device_name, dev_name_len) == 0)
						{
							// File belongs to the current drive.
							swprintf(m_file_name, FNAME_BUFFER_LEN, L"%s%s", drive, fname_buffer+dev_name_len);
							fn_ready = TRUE;
							break;
						}
					}
				}
			}

			if (fn_ready == FALSE)
			{
				// None of the drives matched. Use file name that contains the device name.
				wcscpy(m_file_name, fname_buffer);
			}
		}

		::UnmapViewOfFile(pMemArea);
	}

	::CloseHandle(hFileMap);
}

//----------------------------------------------------------------------
//  ==================  TFileDumpHelper  ======================
//----------------------------------------------------------------------

bool TFileDumpHelper::DumpAsSourceFile(wchar_t *err_msg_buffer, int err_msg_buffer_len, TBasicFileInfo &info, const wchar_t *subdir_name, const wchar_t *reference_file_name, const wchar_t *reference_file_purpose,
										const wchar_t *file_name_prefix, const wchar_t *file_name_suffix, const wchar_t *destination_file_purpose, const wchar_t *structs_and_arrays_names_prefix, wchar_t *rprt_fname_buff, int rprt_fname_buff_len)
{
	TFileNameBuffer dest_directory_name;
	if (TPathHelper::PrepareDestDirectory(err_msg_buffer, err_msg_buffer_len, dest_directory_name, subdir_name, reference_file_name, reference_file_purpose) == FALSE)
		return(FALSE);

	TDestinationFile rprt;
	if (TPathHelper::PrepareDestFile(err_msg_buffer, err_msg_buffer_len, rprt, dest_directory_name, file_name_prefix, info.file_name, file_name_suffix, destination_file_purpose) == FALSE)
		return(FALSE);

	TFileNameBuffer struct_name;
	if (TPathHelper::ExtractShortName(struct_name, info.file_name) == FALSE || struct_name.Insert(0, structs_and_arrays_names_prefix) == FALSE)
	{
		swprintf(err_msg_buffer, err_msg_buffer_len, L"Error preparing file info struct name for: \"%s\".", info.file_name);
		return(FALSE);
	}

	// Do the job.
	TPathHelper::RemoveExtensionAndDot(struct_name);
	TFileDumpHelper::AddStdHeader(rprt);
	TFileDumpHelper::AddFileBodyNameExtern(rprt, struct_name, TRUE);
	TFileDumpHelper::AddFileHeader(rprt, struct_name, info);
	TFileDumpHelper::AddLocalFilesDirectoryItem(rprt, struct_name, TRUE);
	TFileDumpHelper::AddFileBody(rprt, struct_name, info.file_body, info.file_len);
	TFileDumpHelper::AddStdFooter(rprt);
	rprt.Close();

	// Check the overall result.
	if (rprt.GetErrorFlag() == TRUE)
	{
		swprintf(err_msg_buffer, err_msg_buffer_len, L"Error emitting source file code to: \"%s\".", rprt.FileName());
		return(FALSE);
	}

	// Success.
	if (rprt_fname_buff != NULL && rprt_fname_buff_len > 0)
		swprintf(rprt_fname_buff, rprt_fname_buff_len, L"%s", rprt.FileName());
	return(TRUE);
}

void TFileDumpHelper::AddStdHeader(TDestinationFile &rprt)
{
	// Header of the file.
	rprt.WriteLine(L"#include    <stdio.h>");
	rprt.WriteLine(L"#include    <windows.h>");
	rprt.WriteLine(L"");
	rprt.WriteLine(L"#include    \"Common/FileDataHelper.h\"");
	rprt.WriteLine(L"");
}

void TFileDumpHelper::AddStdFooter(TDestinationFile &rprt)
{
	rprt.WriteLine();
}

void TFileDumpHelper::AddFileBodyNameExtern(TDestinationFile &rprt, const wchar_t *file_struct_name, bool last_extern)
{
	assert(file_struct_name != NULL && file_struct_name[0] != 0);

	rprt.WriteFmtLine(L"extern wchar_t %s_file_body[];", file_struct_name);
	if (last_extern == TRUE)
		rprt.WriteLine();
}

void TFileDumpHelper::AddFileHeader(TDestinationFile &rprt, const wchar_t *file_struct_name, TBasicFileInfo &info)
{
	assert(file_struct_name != NULL && file_struct_name[0] != 0);
	assert(info.file_name != NULL && info.file_name[0] != 0);

	rprt.WriteFmtLine(L"struct TBasicFileInfo %s = ", file_struct_name);
	rprt.WriteFmtLine(L"{");

	rprt.Write(L"\tL\"");
	int fname_len = (int)wcslen(info.file_name);
	for (int ii=0; ii<fname_len; ++ii)
	{
		if (info.file_name[ii] == L'\\' || info.file_name[ii] == L'/')
			rprt.Write(L"\\\\");
		else rprt.WriteFmt(L"%c", info.file_name[ii]);
	}

	rprt.WriteLine(L"\",");
	if (info.file_body != NULL)
		rprt.WriteFmtLine(L"\t%s_file_body,", file_struct_name);
	else rprt.WriteFmtLine(L"\tNULL,");

	wchar_t loc_date_buff[80];
	rprt.WriteFmtLine(L"\t%ld,  // File body length in WCHARs.", info.file_len);
	rprt.WriteFmtLine(L"\t%I64d,  // File date (%s).", info.file_date, FormatDateTime(info.file_date, loc_date_buff, 80, FALSE));
	rprt.WriteFmtLine(L"\t0x%08X,  // File CRC.", info.file_crc);

	rprt.WriteFmtLine(L"};");
	rprt.WriteFmtLine(L"");
}

void TFileDumpHelper::AddLocalFilesDirectoryItem(TDestinationFile &rprt, const wchar_t *file_struct_name, bool last_directory_item)
{
	assert(file_struct_name != NULL && file_struct_name[0] != 0);

	rprt.WriteFmtLine(L"static TLocalFilesDirectory %s_directory_item(%s);", file_struct_name, file_struct_name);
	if (last_directory_item == TRUE)
		rprt.WriteLine();
}

void TFileDumpHelper::AddFileBody(TDestinationFile &rprt, const wchar_t *file_struct_name, const wchar_t *file_body_data, long file_len_in_wchars)
{
	assert(file_struct_name != NULL && file_struct_name[0] != 0);

	rprt.WriteFmtLine(L"wchar_t %s_file_body[] = ", file_struct_name);
	rprt.WriteFmtLine(L"{");

	if (file_body_data != NULL)
	{
		if (file_len_in_wchars < 0)
			file_len_in_wchars = (long)wcslen(file_body_data);

		int cnt_lines = 1;
		while (file_len_in_wchars > 0)
		{
			// Find out where the current line ends.
			int line_len = file_len_in_wchars;
			for (int ch_inx=0; ch_inx<file_len_in_wchars; ++ch_inx)
			{
				if (file_body_data[ch_inx] == L'\n')
				{
					line_len = ch_inx+1;
					break;
				}
				else if (file_body_data[ch_inx] == L'\r' && ch_inx < file_len_in_wchars-1 && file_body_data[ch_inx+1] == L'\n')
				{
					line_len = ch_inx+2;
					break;
				}
			}

			// End of line is found or the this is last line in the file.
			rprt.WriteFmt(L"  /* %06d */  L\"", cnt_lines++);

			assert(line_len > 0);
			for (int ii=0; ii<line_len; ++ii)
			{
				switch (file_body_data[ii])
				{
					case L'\t':	rprt.Write(L"\\t");	break;
					case L'\r':	rprt.Write(L"\\r");	break;
					case L'\n':	rprt.Write(L"\\n");	break;
					case L'\'':	rprt.Write(L"\\\'");	break;
					case L'\"':	rprt.Write(L"\\\"");	break;
					case L'\\':	rprt.Write(L"\\\\");	break;

					default:
						rprt.WriteFmt(L"%c", file_body_data[ii]);
						break;
				}
			}

			rprt.WriteLine(L"\"");

			// Shift to the next line.
			file_body_data += line_len;
			file_len_in_wchars -= line_len;
		}
	}

	rprt.WriteFmtLine(L"};");
	rprt.WriteFmtLine(L"");
}


