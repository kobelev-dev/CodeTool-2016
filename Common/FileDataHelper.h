//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	Common_FileDataHelper_H
#define	Common_FileDataHelper_H

#ifndef   Common_Buffers_H
#include  "Common/Buffers.H"
#endif
#ifndef   Common_DateTimeHelper_H
#include  "Common/DateTimeHelper.H"
#endif

//
// The file name and the file body are typically pointers to the heap allocations. They may be owned,
// and they may not be owned by the TBasicFileInfo structure. This structure should be primarily used
// for the text files because the length of the file body is expressed in WCHARS, not in bytes.
//
struct TBasicFileInfo
{
	wchar_t			*file_name;			// This field is not NULL when the struct is inited.
	wchar_t			*file_body;			// The body of the file is NOT NULL terminated.

	long				file_len;				// The length is expressed in the number of WIDE chars.
	TDateTime		file_date;
	DWORD			file_crc;

public:

	void		InitBasicFileInfo();

	void		FreeBasicFileInfo();
				// This method should be used with care because it expects that the file name and the body
				// of the file are owned by the structure and reside in the heap.

	static bool	Compare(const TBasicFileInfo *info1, const TBasicFileInfo *info2);
					// The return value is TRUE when the passed structures describe the same file. This means
					// that the names of the files, their lengths and the CRCs of their bodies match exactly.
};

//
//  Result of loading file from the disk into memory or the result of saving in memory file to the disk.
//
enum TLoadSaveResult
{
	ldres_success,
	ldres_fname_missing,
	ldres_file_empty,
	ldres_error_opening,
	ldres_out_of_memory,
	ldres_error_reading,
	ldres_error_writing,
	ldres_error_closing,
	ldres_num_results,
};

class TFileBodyHelper
{
public:

	//
	//  Function loads the file and keeps its crlf structure intact. After loading the file body contains exactly the same
	//  characters that were present in the disk file except for the non ASCII chars (>=128) that are replaced with
	//  the question marks ('?'). This function adds '\0' to the end of the file body and this char is NOT included into
	//  length of the file.
	//
	//		fname_on_disk		use_real_disk_fname
	//   ----------------------------------------------------------------------------------
	//		     NULL					FALSE			File is opened as "fname" and this name is placed into the
	//												passed TBasicFileInfo structure without any changes.
	//		     NULL					TRUE			Param "fname" is resolved using the ::GetFullPathName function.
	//												After that the file is opened using that name and that name is stored
	//												into the TBasicFileInfo structure.
	//		Non null value				   x				The file is opened with the name exactly as it is in "fname_on_disk"
	//												param and "fname" is stored into the TBasicFileInfo structure.
	//
	//	After return from the function param "info" contains meaningful data only when the return value
	//	is "ldres_success".
	//
	static TLoadSaveResult LoadAsciiFileBody(TBasicFileInfo &info,			// Structure to fill. New data overwrites the old contents regardless of what was there before.
							const wchar_t *fname,							// Name of the file is duplicated into heap.
							const wchar_t *fname_on_disk = NULL,
							bool  use_real_disk_fname = TRUE,
							bool  allow_empty_files_loading = TRUE,
							bool  warn_on_big_values = FALSE,				// Big values are chars >= 128.
							int  *p_num_non_ascii_warnings = NULL,
							long *p_first_non_ascii_char_index = NULL,
							HANDLE *p_opened_file_handle = NULL,			// Valid non NULL file handle is returned only in ldres_success case.
							DWORD *p_win32_error = NULL);				// Win32 error is returned in ldres_error_opening and ldres_error_reading cases.

	//
	//  Function always duplicates the name of the file and the body of the file into the heap memory.
	//  The only one possible error is the ldres_out_of_memory.
	//
	static TLoadSaveResult SetupInMemFileBody(TBasicFileInfo &info,		// Structure to fill.
							const wchar_t *fname,							// Name of the file is duplicated.
							const wchar_t *fbody,							// Body of the file is duplicated. Although it can be NULL.
							long file_len = -1,							// Length of the file in WCHARs.
							TDateTime fdate = 0);

	//
	//  Function converts the file body to ASCII and saves it to the disk.
	//
	static TLoadSaveResult SaveAsciiFileBody(const wchar_t *fname,			// Name of the file.
							wchar_t *fbody,								// Body of the file. Function will do conversion to ASCII right in this buffer.
							long file_len = -1);							// Length of file in WCHARs.

	//
	//  Function returns a piece of the sentence, like: "unable to open the file".
	//
	static const wchar_t *GetLoadSaveResultText(TLoadSaveResult err, bool want_first_cap = FALSE);

	//
	//  Function calculates CRC32 based on the Ethernet polinomial.
	//
	static DWORD CalcCrc32(const void *data, long data_len_in_bytes);

	//
	//	Simple function that drops the high byte of the UNICODE symbols.
	//
	static void  ConvertToAsciiInPlace(wchar_t *buffer);
};

//
//  Helper structure for handling translation to/from the file offset and the line number.
//
struct TSimpleLineInfo
{
	long				line_offs;			// Offset to the beg of the line from the beginning of the file data in WCHARs.
	long				line_len;			// Length of the line in WCHARs. The line is NOT null terminated.

	inline long LineBeg() { return(line_offs); }
	inline long LineEnd() { return(line_offs+line_len); }

	static TSimpleLineInfo	*BuildLinesInfoFromData(const wchar_t *fdata, long fdata_len, long fdata_beg_src_offs, long &linfo_len, long &longest_line_len);
							// This function looks for '\n' characters and treats them as the line delimiters. The '\n' char is the only one
							// line delimiter. In other words the number of lines in the passed data is the number of '\n' characters plus
							// one. The passed fdata should be always not NULL. Although it is ok to build the lines info for empty files,
							// i.e. when the fdata_len == 0. In this case the line info array will contain one element that has two zeroes
							// in its fields.

	static void			FreeLinesInfo(TSimpleLineInfo **pinfo);
							// Finction frees the lines info and clears the passed pointer.

	static TSimpleLineInfo	*FindLineInfo(TSimpleLineInfo *lines_info, long lines_info_len, long file_offs);
							// When the lines info is missing or when the passed file_offs is out of the file, the return value is NULL.
};

//
//  This iterator iterates the lines of the file body.
//
struct TFileSpaceIterationInfo
{
	TFileSpaceIterationInfo(long abeg, long alen, TSimpleLineInfo *li_data, long li_len) { area_beg = abeg; area_len = alen; line_inx = -1; linfo_data = li_data; linfo_len = li_len; }
			// Right after creation the iterator does not contain informantion about the current piece of the file.
			// StepIteration() should be called to retrieve the first line area if any.

	bool		StepIteration();
				// Note that the iteration step may return empty area at the beginning or at the end of the line.

public:

	long			char_beg, num_chars;		// Character offsets inside the line. These 2 fields are the result of the iteration
											// step. They have meaning only if the latest iteration step has returned TRUE.
	long			line_inx;					// Index of the current line in the linfo_data array. This field is used as the state
											// of iteration.
private:

	long					area_beg, area_len;			// When the iteration is finished, the length of the area becomes negative.

	TSimpleLineInfo		*linfo_data;
	long					linfo_len;

	friend class TDecoratedFileViewLineInfo;
	friend class TDecoratedFileViewScreenItem;
	friend class THtmlDecoratedFile;
};

class TLocalFilesDirectory
{
public:

	TLocalFilesDirectory(TBasicFileInfo &data) : m_file_data(data) { m_next_file = m_files_list; m_files_list = this; }
		// Instances of this class are expected to be created only in the static memory.
		// The passed parameter should be in the static memory only.

	static TBasicFileInfo	*FindFileInfo(const wchar_t *full_file_name);

protected:

	TBasicFileInfo					&m_file_data;
	TLocalFilesDirectory			*m_next_file;

	static TLocalFilesDirectory		*m_files_list;
									// This is a single linked list of all files that belong to the directory.
};

class TDestinationFile;

class TPathHelper
{
public:

	static bool	IsEmptyPath(const wchar_t *path) { return(path == NULL || path[0] == 0); }

	static bool	IsAbsolutePath(const wchar_t *path);
	static bool	IsIntermediatePath(const wchar_t *path);
	static bool	IsRelativePath(const wchar_t *path);
					//
					// These methods determine the type of the passed path by simply inspecting the path. They do not check
					// anything on the disk. Examples:
					//
					//     absolute:		drive:\dir\name.ext
					//					\\box\dir\name.ext
					//     intermediate:		drive:dir\name.ext		(curr dir is referecced)
					//					\dir\name.ext			(curr drive is referecced)
					//     relative:			dir\name.ext
					//
					// In other words the intermediate path either refers to the current directory on the directly specified drive
					// or it refers to something starting from the root directory of the current drive.
					//

	static bool	IsPathADir(const wchar_t *path);
	static bool	IsPathAFile(const wchar_t *path);
					//
					// These methods check the passed path using the OS. Methods pass passed paths to OS without any changes.
					// Some examples:
					//
					//		"c:"			--  DIR			"\\as6"		-- not a dir
					//		"c:\"		--  DIR				"\\as6\"		-- not a dir
					//		"c:\temp"	--  DIR				"\\as6\i386"	--   DIR
					//		"c:\temp\"	--  DIR			"\\as6\i386\"	--   DIR
					//		"\"			--  DIR
					//

	static void	ConvertToBackSlashesInPlace(wchar_t *path);
	static void	RemoveTrailingSlashInPlace(wchar_t *path);

	static void	ConvertToBackSlashes(TFileNameBuffer &buffer);
	static void	RemoveTrailingSlash(TFileNameBuffer &buffer);
	static bool	EnsureTrailingSlash(TFileNameBuffer &buffer);

	static void	RemoveExtensionAndDot(TFileNameBuffer &buffer);
					// This method should be used when the app needs to change the filename extension.

	static bool	GetDirectoryName(TFileNameBuffer &buffer, const wchar_t *file_name);
	static bool	ExtractShortName(TFileNameBuffer &buffer, const wchar_t *file_name, bool remove_extension_and_dot = FALSE);
					// Second parameter is expected to be a name of the file, not a directory. Ret value is TRUE when there
					// is no memory error and non empty dir name or short name was placed into the buffer. Note that short
					// name from the file name like "\a\b\" will be empty.

	static const wchar_t *GetShortNamePtr(const wchar_t *file_name);
						// Method returns pointer to the part of the name that stays after the last L'\\' or L'/' which ever will occur
						// to be the last. If none of the delimiters is present, the pointer to the original file name is returned.

	static const wchar_t *GetExtensionPtr(const wchar_t *file_name);
						// This method always returns a non NULL pointer. When the passed file name does not have any extension,
						// the returnned pointer points to the trailing NULL of the file name.

	static bool	ConstructPath(TFileNameBuffer &buffer, const wchar_t *base_dir, const wchar_t *relative_path, bool relative_path_is_file = TRUE);
					// If second param is NULL, this means that name of the root is already in the buffer. Otherwise the buffer
					// is cleared and name of the root is put there. Last param is expected to be pure relative. Result is placed
					// into the buffer. In case of error contents of the buffer is not valid.

	static bool	GetExeDirectoryName(wchar_t *buffer, int buffer_len);
					// Method picks up the name of exe file of the process and then strips the short name of the executable.
					// The trailing backslash remains in the buffer.

	static bool	PrepareDestDirectory(wchar_t *err_msg_buffer, int err_msg_buffer_len, TFileNameBuffer &dest_buffer, const wchar_t *subdir_name, const wchar_t *reference_file_name, const wchar_t *reference_file_purpose);
					//
					//  This method prepares name of directory that will be used to place some generated file into it. In case if
					//  the name is successfully generated and this directory is not existing, method tries to create this directory.
					//  There are 2 possible use scenarios:
					//
					//		--	The subdir name is emty or it is a relative path. In this case destination file will be placed
					//			either in the same directory as the reference file, or to the subdirectoty where the reference
					//			file is located.
					//		--	The subdir name is an absolute or an intermediate path. In this case destination directory is
					//			simply the subdir itself and no conversions are made.
					//

	static bool	PrepareDestFile(wchar_t *err_msg_buffer, int err_msg_buffer_len, TDestinationFile &rprt, const wchar_t *dir_name, const wchar_t *file_name_prefix, const wchar_t *file_name_proto, const wchar_t *file_name_suffix, const wchar_t *destination_file_purpose);
					//
					//  Method prepares the name of the disk file and open the passed report with this name. The name of the report file
					//  is the short name of the passed file name proto without an extension and with added prefix and suffix. It is expected
					//  that the passed suffix contains an extension for the dest file. If the passed file name suffix is null or empty, method
					//  adds the default ".cxx" suffix.
					//

	static bool	CloseDestFile(wchar_t *err_msg_buffer, int err_msg_buffer_len, TDestinationFile &rprt);
					//
					//  Method closes the passed file and prepares an error message if needed.
					//

	static DWORD		CreateSubDirectory(const wchar_t *directory_name);
										
	static DWORD		DeleteAllFilesFromDirectory(const wchar_t *directory_name);
						// This method stops processing after the first error. The return value is a WIn32 error code.

protected:

	static wchar_t *GetLastBackSlashPtr(const wchar_t *buffer);
};

//
//  Iteration of the contents of a single directory.
//
class TDirectoryIterator
{
public:
			TDirectoryIterator();
			~TDirectoryIterator() { if (m_search_handle != INVALID_HANDLE_VALUE) ::FindClose(m_search_handle); }

	void		StartIteration(const wchar_t *directory_name, bool want_subdirectories, bool want_files);
				// The name of the directory should be not empty and it should NOT contain a backslash at the end.

	void		StartIteration(TFileNameBuffer &directory_name_buffer, bool want_subdirectories, bool want_files);
				// This method expects that the name of the directory is already in the buffer. This method temorarily
				// appends "\\*.*" to the name of directory. After return from the method the state of the buffer is
				// restored back.

	operator bool() const		{ return(m_search_handle != INVALID_HANDLE_VALUE); }
	void	operator ++ ()			{ while (StepIteration() == TRUE) if (CheckCurrentFileInfo() == TRUE) break; }

public:

		//
		//  Current state of the iterator. This iterator does not support file masks.
		//

	DWORD			FileAttributes() const		{ return(m_file_info.dwFileAttributes); }
	bool				IsSubdirectory() const		{ return((m_file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0); }

	const wchar_t		*ShortFileName() const		{ return(m_file_info.cFileName); }
	long				FileLength() const			{ return(m_file_info.nFileSizeLow); }
	TDateTime		FileDate() const				{ return(MakeDateTime(m_file_info.ftLastWriteTime)); }
						// These methods should be called only when iteration is active.

	DWORD	IterationResuilt() const { return(m_last_error); }
				// This is Win32 error if any. Note that fname buffer expansion error is converted into ERROR_NOT_ENOUGH_MEMORY.
				// In typical scenario this method should be called when iteration is finished to check if everything went fine or not.
protected:

	bool		StepIteration();
	bool		CheckCurrentFileInfo();

protected:

	bool						m_include_subdirectories;
	bool						m_include_files;

	WIN32_FIND_DATAW		m_file_info;
	HANDLE					m_search_handle;

	DWORD					m_last_error;
};

class TDirectoriesSubtreeIterator
{
public:	//
		//	This iterator uses the concept of callback handlers that are defined as virtual methods.
		//

	TDirectoriesSubtreeIterator() { m_abort_iteration = FALSE; m_last_error = ERROR_SUCCESS; }

	bool		Iterate(const wchar_t *subdir_name, bool process_subdirectories = TRUE, bool want_callback_for_root_dir_name = TRUE);
				// Return value is FALSE in case of a Win32 error or a memory error. When the app layer aborts iteration,
				// this is not considered as an iteration error. Virtual callback methods are called from inside of this method.

	virtual void	ProcessSubdirectory(const wchar_t *subdir_name) { }
	virtual void	ProcessFileShortName(const wchar_t *subdir_name, const wchar_t *short_file_name) { }
	virtual void	ProcessFileFullName(const wchar_t *full_file_name) { }
					// Derived classes should not modify names, that are passed to them as parameters.

	DWORD	IterationResuilt() const { return(m_last_error); }
				// This method returns the Win32 error if any. Note that the file name buffer expansion error is converted
				// into the ERROR_NOT_ENOUGH_MEMORY. In typical scenario this method should be called when the iteration
				// is finished to check if everything went fine or not.
protected:

	void		IterateObjectsInternal(bool process_subdirectories);
				// This is recursive worker function.

	TFileNameBuffer		m_curr_name_buffer;
	bool					m_abort_iteration;
	DWORD				m_last_error;
};

enum TDestinationFileState
{
	dstfs_nodata = 1,		// This is state of the dest file right after creation or when the in memory
							// stream was created, filled, closed the and its body was extracted.
	dstfs_writing,
	dstfs_writing_err,
	dstfs_closed,
	dstfs_closed_err,
};

class TDestinationFile
{
public:
			TDestinationFile(const wchar_t *name = NULL);
			~TDestinationFile();

	void		SetFileName(const wchar_t *fname) { wcsncpy(m_file_name, fname, FNAME_BUFFER_LEN); m_file_name[FNAME_BUFFER_LEN-1] = 0; }

	bool		PrepareDiskFile(HANDLE opened_file = NULL, bool convert_to_ascii = TRUE);
	bool		PrepareInMemoryStream();
				// In case of success both methods transfer object into the "opened" state and reset the error flag.

	void		Write(const wchar_t *data, long data_len_in_wchars = -1);
	void		WriteLine(const wchar_t *data = L"", long data_len_in_wchars = -1);
	void		WriteFmt(const wchar_t *format, ...);
	void		WriteFmtLine(const wchar_t *format, ...);

	bool		IsWritingState() { return((m_stt == dstfs_writing || m_stt == dstfs_writing_err) ? TRUE : FALSE); }
	bool		Close();
				// Return value is TRUE if there were no writing errors and the file was closed successfully.
				// Otherwise the return value is FALSE.

	bool		DeleteClosedDiskFile();
	bool		WipeDiskFile();

	bool		GetBasicFileInfo(TBasicFileInfo &info, bool dup_body = FALSE, TDestinationFileState *pStt = NULL);
				// Basic file info can be retrieved only when the state is dstfs_closed. When the state of the object is
				// dstfs_closed and ret value is still FALSE, this means that there was memory error while allocating
				// file name or file body. File name and body are always in the heap. They should be freed
				// with ReleaseBasicFileInfo(). File CRC is always zero.

	const wchar_t		*FileName() const		{ return(m_file_name);   }
	long				GetCurrLen() const		{ return(m_file_len);    }
	long				GetNumLines() const	{ return(m_num_lines); }
	bool				GetErrorFlag() const	{ return(((m_stt & 1) != 0) ? TRUE : FALSE); }

	wchar_t			*GetBodyPtr()			{ if (m_mem_body == NULL) return(NULL); m_mem_body[m_file_len] = 0; return(m_mem_body); }
						// This method should be used to distinguish between the disk file and the in memory buffer.

	wchar_t		*ExtractMemBody() { wchar_t  *pbody = GetBodyPtr(); m_mem_body = NULL; return(pbody); }
					// This method safely returns NULL if the object represents the disk file.

	void			TruncateTo(long new_len, long new_num_lines = 0);
					// Caller is responsible for passing correct line number if this number is important.

	void			SetWritingError() { if (m_stt != dstfs_nodata) m_stt = (TDestinationFileState)(m_stt | 1); }
					// Sometimes it is needed to fail writing the file for reasons that are not realted to disk I/O.
					// This method allows doing this.

	enum { FNAME_BUFFER_LEN = 2*MAX_PATH };

protected:

	void			WriteToDiskFile(BYTE *data, long data_len);

	void			GetExactFileName();
					// Method retrieves file name from the current file handle.
protected:

	enum { MESSAGE_BUFF_LEN = 2048 };
	enum { DISK_FILE_BUFF_LEN = 8192 };

	TDestinationFileState		m_stt;

	wchar_t					m_file_name[FNAME_BUFFER_LEN];
	long						m_file_len;				// Size of the stream in WCHARs.
													// Trick: for closed disk files, this is size of the file in bytes.
	TDateTime				m_file_date;
	long						m_num_lines;			// This is number of '\n' chars in the stream.

	HANDLE					m_file_handle;
	bool						m_ascii_mode;

	wchar_t					*m_mem_body;			// This field can be used to distinguish between the disk file and the in memory stream.
													// If allocated, the buffer is always at least one WCHAR longer than the contained data.
	long						m_mem_body_len;		// Length of the allocated buffer.

	DWORD					m_win32_error;

	wchar_t					m_message_buff[MESSAGE_BUFF_LEN];

	BYTE					m_disk_file_buffer[DISK_FILE_BUFF_LEN];
	int						m_disk_file_buff_data_len;

	friend class TPathHelper;
};

//
//  Dumping text file as a Cpp source file for including it into the TLocalFilesDirectory.
//
class TFileDumpHelper
{
public:

	static bool	DumpAsSourceFile(wchar_t *err_msg_buffer, int err_msg_buffer_len, TBasicFileInfo &info, const wchar_t *subdir_name, const wchar_t *reference_file_name, const wchar_t *reference_file_purpose,
									const wchar_t *file_name_prefix, const wchar_t *file_name_suffix_with_extension, const wchar_t *destination_file_purpose, const wchar_t *structs_and_arrays_names_prefix,
									wchar_t *rprt_fname_buff = NULL, int rprt_fname_buff_len = -1);
protected:

	static void	AddStdHeader(TDestinationFile &rprt);
	static void	AddStdFooter(TDestinationFile &rprt);

	static void	AddFileBodyNameExtern(TDestinationFile &rprt, const wchar_t *file_struct_name, bool last_extern = FALSE);
	static void	AddFileHeader(TDestinationFile &rprt, const wchar_t *file_struct_name, TBasicFileInfo &info);
	static void	AddLocalFilesDirectoryItem(TDestinationFile &rprt, const wchar_t *file_struct_name, bool last_directory_item = FALSE);
	static void	AddFileBody(TDestinationFile &rprt, const wchar_t *file_struct_name, const wchar_t *file_body_data, long file_data_len_in_wchars);
};

#endif	// Common_FileDataHelper_H


