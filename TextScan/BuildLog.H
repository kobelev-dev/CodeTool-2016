//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	TextScan_BuildLog_H
#define	TextScan_BuildLog_H

#ifndef   Common_GenericConsole_H
#include  "Common/GenericConsole.H"
#endif
#ifndef   Common_HashTable_H
#include  "Common/HashTable.H"
#endif
#ifndef   TextScan_Globals_H
#include  "TextScan/Globals.H"
#endif

class TBuildLogInfo;

enum TBuildLogSubtrProcMode
{
	sbtm_none,				// Children will be not processed regardless of the props on the level below.
	sbtm_marked,			// Consider processing props at the lower level.
	sbtm_all,				// Process everything regargless of selection state on the lower levels.
};

struct TBuildLogTreeNode
{
	TBuildLogSubtrProcMode	proc_mode;
	int						chld_active;			// The number of leaves to process.
	int						chld_total;			// Total number of leaves.
};

// Source file does not have any properties. The parsing properties are present on the command line level.
struct TBuildLogSrcFile
{
	bool						bProcess;
	TSourceArea				fname_area;
};

typedef THeapBasedArray<TBuildLogSrcFile, 32> TBuildLogSrcFilesArray;

struct TBuildLogCommandLine : public TBuildLogTreeNode
{
	TParseEnvInfo				*parse_env;
	TSourceArea				cmdl_area;
	TBuildLogSrcFilesArray		files;

	void		Clear() { files.Clear(); }
	void		ReleaseData() { files.ReleaseData(); }

	void		AddSrcFile(TBuildLogSrcFile &info, TBuildLogInfo &errs_log);
	void		SetSrcFileState(int file_inx, bool new_stt);
};

typedef THeapBasedArray<TBuildLogCommandLine, 16> TBuildLogCommandLinesArray;

struct TBuildLogSubdirInfo : public TBuildLogTreeNode
{
	TSourceArea					subdir_area;
	TBuildLogCommandLinesArray	cmdls;

		~TBuildLogSubdirInfo() { ReleaseData(); }

	void		Clear() { subdir_area.Clear(); cmdls.Clear(); }
	void		ReleaseData();

	void		AddCmdl(TBuildLogCommandLine &info, TBuildLogInfo &errs_log);
	void		SetCommandLineState(int cmdl_inx, TBuildLogSubtrProcMode new_mode);
	void		SetSrcFileState(int cmdl_inx, int file_inx, bool new_stt);

	static int __cdecl CompareInfos(const TBuildLogSubdirInfo *p1, const TBuildLogSubdirInfo *p2);
};

typedef THeapBasedArray<TBuildLogSubdirInfo, 16> TBuildLogSubdirInfosArray;

enum TBuildLogScanErrorCode
{
	blec_none,
	blec_oom_srcfile_info,
	blec_oom_cmdline_info,
	blec_oom_subdir_info,
	blec_oom_cpu_num_info,
	blec_no_dir_name,
	blec_dir_name_not_set,
	blec_cmdl_raw_error,
	blec_cmdl_no_files,				// Disabled for now.
};

struct TBuildLogScanError
{
	int				err_code;
	TSourceArea		area;

	wchar_t	*SprintfError(wchar_t *buff, long buff_len);
};

typedef THeapBasedArray<TBuildLogScanError, 16> TBuildLogScanErrorsArray;

class TBuildLogInfo : public TBuildLogTreeNode
{
public:
			TBuildLogInfo()		{ file_info.Clear(); mem_error = FALSE; }
			~TBuildLogInfo()		{ ReleaseData(); }

	void		ReleaseData();

	int		NumSubdirs() const	{ return(subdirs.NumItems()); }
	int		NumErrors() const		{ return(errors.NumItems()); }
	bool		IsMemError() const		{ return(mem_error); }

	int		NumCmdlsTotal();
	int		NumFilesTotal();

	TFileInfo				&File()				{ return(file_info); }
	TBuildLogSubdirInfo	&Subdir(int inx)		{ return(subdirs[inx]); }
	TBuildLogScanError	&Error(int inx)		{ return(errors[inx]);  }

	void		SetBuildLogState(TBuildLogSubtrProcMode new_mode);
	void		SetSubdirInfoState(int subdir_inx, TBuildLogSubtrProcMode new_mode);
	void		SetCommandLineState(int subdir_inx, int cmdl_inx, TBuildLogSubtrProcMode new_mode);
	void		SetSourceFileState(int subdir_inx, int cmdl_inx, int file_inx, bool new_stt);

protected:

	void		AddSubdirInfo(TBuildLogSubdirInfo &info);
	void		AddScanningError(int err, TSourceArea &area);

protected:

	TFileInfo							file_info;				// This file does not belong to any database.
															// Its doc_id is zero.
	TBuildLogSubdirInfosArray			subdirs;
	TBuildLogScanErrorsArray			errors;
	bool								mem_error;

	friend struct TBuildLogCommandLine;
	friend struct TBuildLogSubdirInfo;
	friend class  TBuildLogParser;
};

// Info about the subdir as an item in the hash table.
class TBuildLogSubdirInfoItem : public THashTableItem<TBuildLogSubdirInfoItem>
{
public:

	TBuildLogSubdirInfoItem(int num) { m_cpu_num = num; m_ignore_dir = FALSE; m_info.Clear(); }

	//
	// These functions are required for the hash table to work.
	//
	virtual DWORD	GetHashValue() { return(m_cpu_num); }
	virtual bool		IsEqualTo(const TBuildLogSubdirInfoItem &item) { return(m_cpu_num == item.m_cpu_num); }

	int								m_cpu_num;
	bool								m_ignore_dir;			// This field supports subdirectories filtering.

	TBuildLogSubdirInfo				m_info;
};

// Hash table of the subdirectory infos.
class TBuildLogSubdirInfosList : public THashTable<TBuildLogSubdirInfosList, TBuildLogSubdirInfoItem, 0x80> { };

class TBuildLogParser
{
public:

		TBuildLogParser(TGenericConsole *pConsole, TParseEnvInfo **known_envs = NULL);
		~TBuildLogParser();

	void			Parse(TBuildLogInfo &info_buff, const wchar_t *build_log_fname,
						const wchar_t **apps_filter = NULL,			// List of the names of the applications that should be processed or NULL.
																// When the value of this param is NULL, all known apps are processed.
						const wchar_t *subdirs_filter = NULL);

	static wchar_t		*GetErrorText(int err_no);

protected:

	bool		PickNextWord(wchar_t *&line_ptr, long &line_len, wchar_t *&word_beg, long &word_len);
	void		AddScanningError(int err_no, wchar_t *area_beg, long area_len);

	static bool	IsBuildLogSpace(wchar_t ch) { return(ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n' || ch == 0); }

	TGenericConsole				*m_cons;
	TParseEnvInfo					**m_envs;
	TBuildLogInfo					*m_info;

	TBuildLogSubdirInfosList		m_stt;
									// Note: Hash table slot is treated as non empty when its field info.subdir_area.file_info is not NULL.
};

#endif // TextScan_BuildLog_H


