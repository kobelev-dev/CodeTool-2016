//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	Processing_RawProcessing_H
#define	Processing_RawProcessing_H

#ifndef   TextScan_ParsingDriver_H
#include  "TextScan/ParsingDriver.H"
#endif

// ------------------- Section 1 -------------------------

class TSrcDetailsIterationHandler : public TFileDataIterationHandler
{
public:

	TSrcDetailsIterationHandler(bool verbose_mode = FALSE);
	~TSrcDetailsIterationHandler() { m_curr_file_info.ReleaseLinesInfo(); }

	void				ProcessSubtree(TGenericConsoleEx &cons, const wchar_t *directory_name, bool recurse_subdirs,
										const wchar_t **list_of_file_name_exts, bool  wait_for_user_resp, TBatchParsingSummary *pSumry = NULL);

	virtual void		ProcessDirectoryName(const wchar_t *subdir_name);
	virtual void		ProcessDataFile(TBasicFileInfo &basic_info);

	void				PrintSummary(TGenericConsoleEx &cons);

	void				PrintSubdirectoryHeader();
	void				PrintFileNameHeader();

	void				ReportDetail(long file_offset, const wchar_t *format, ...);
	void				ReportDetail(PrEventHeader &origin, const wchar_t *format, ...);
	void				ReportSilentDetail();

	int				GetLineNumber(long file_offset);

protected:

	bool				m_verbose_mode;
						// When the verbose mode is TRUE, the file header is printed for every scanned file.
						// Otherwise it is printed only when some detail is discovered in the file.

	TList				m_src_details_checkers;				// List of TFileDataDetailsChecker.
	TList				m_lex_details_checkers;				// List of TLexemaDetailsChecker.

	wchar_t			m_curr_dir_name[4*MAX_PATH];
	int				m_scanned_files_in_subdir;			// Number of files that have passed the extensions filter.
	bool				m_subdir_header_printed;

	TFileInfo			m_curr_file_info;
	bool				m_sys_generated_file;				// This is a Visual Studio auto generated file.
	bool				m_header_file;						// This is either ".h" or ".H" file.
	bool				m_resource_file;					// Current file contains Windows resources.
	bool				m_css_file;							// Current file contains the Cascaded Style Sheet.

	int				m_cnt_details;

	bool				m_file_header_printed;
	int				m_file_headers_count;				// In the non verbose mode this is the same to m_cnt_files_with_details.

	int				m_subdirs_with_files;				// Number of subdirs that are containing files that passed the extensions filter.
	int				m_scanned_files_count;				// Number of files that passed the extensions filter.

	int				m_total_files_size;
	int				m_total_lines_count;
	int				m_code_lines_count;

	int				m_cnt_files_with_details;
	int				m_cnt_details_in_all_files;

	friend class TGenericDetailsChecker;
	friend class TFileDataDetailsChecker;
	friend class TLexemaDetailsChecker;
};

class TGenericDetailsChecker : public TListItem
{
public:

	TGenericDetailsChecker(TSrcDetailsIterationHandler &handler) : m_iteration_handler(handler) { }

	virtual int				NumDetailTypes() = 0;
	virtual const wchar_t	*GetDetailName(int idetail) = 0;
	virtual int				GetNumDetails(int idetail) = 0;

	virtual void			PrintCustomReport() { }

	enum
	{
		DHM = TGenericConsole::DefHighlightMode,
		YellowAttr =  (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY),
	};

	void TraceWithAttr(WORD message_higlight_mode, const wchar_t *format, ...)
	{
		// Generate the UNICODE string.
		wchar_t message_buff[2048];
		va_list vargs;
		va_start(vargs, format);
		vswprintf(message_buff, 2048, format, vargs);
		va_end(vargs);

		// Pass it to the console.
		m_iteration_handler.Console().HandleTrace(message_buff, message_higlight_mode);
	}

	bool			IsSystemGeneratedFile() const { return(m_iteration_handler.m_sys_generated_file); }
	bool			IsHeaderFile() const { return(m_iteration_handler.m_header_file); }
	bool			IsRcFile() const { return(m_iteration_handler.m_resource_file); }
	bool			IsCssFile() const { return(m_iteration_handler.m_css_file); }

	TFileInfo		*FileInfo() const { return(&m_iteration_handler.m_curr_file_info); }
	long			FileLength() const { return(m_iteration_handler.m_curr_file_info.info.file_len); }
	wchar_t		*FileBodyPtr() const { return(m_iteration_handler.m_curr_file_info.info.file_body); }

	static bool IsNoDependenciesMsg(const wchar_t *msg, long msg_len) { return(msg_len >= 21 && wcsncmp(msg, L"//{{NO_DEPENDENCIES}}", 21) == 0); }

protected:

	TSrcDetailsIterationHandler		&m_iteration_handler;
};

class TFileDataDetailsChecker : public TGenericDetailsChecker
{
public:

	TFileDataDetailsChecker(TSrcDetailsIterationHandler &handler)
				: TGenericDetailsChecker(handler) { handler.m_src_details_checkers.AppendItem(this); }

	virtual void		ProcessFileData(TBasicFileInfo &basic_info) = 0;
};

class TLexemaDetailsChecker : public TGenericDetailsChecker
{
public:

	TLexemaDetailsChecker(TSrcDetailsIterationHandler &handler)
				: TGenericDetailsChecker(handler) { handler.m_lex_details_checkers.AppendItem(this); }

	virtual void		StartNewFileProcessing() { }
	virtual void		FinalizeFileProcessing() { }

	virtual void		ProcessLexema(TLexema &prev_lex, TLexema &lex) = 0;
						// Note that the prev_lex is empty when the first lexema of the file is processed.

	static bool IsSpaceChar(wchar_t ch) { return(ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n'); }
};

// ------------------- Section 2 -------------------------

class TMiscFileDetailsChecker : public TFileDataDetailsChecker
{
public:

	TMiscFileDetailsChecker(TSrcDetailsIterationHandler &handler, bool notify_on_spaces_after_tabs);

	virtual int				NumDetailTypes() { return(5); }
	virtual const wchar_t	*GetDetailName(int idetail);
	virtual int				GetNumDetails(int idetail);

	virtual void			ProcessFileData(TBasicFileInfo &basic_info);

protected:

	bool					m_notify_on_spaces_after_tabs;

	int					m_cnt_tabs_after_spc;
	int					m_cnt_spcs_after_tab;

	int					m_cnt_spcs_before_eol;
	int					m_cnt_tabs_before_eol;
	int					m_cnt_spcs_line_beg;
};

class TSrcFileDisclaimerChecker : public TFileDataDetailsChecker
{
public:

	TSrcFileDisclaimerChecker(TSrcDetailsIterationHandler &handler);

	virtual int				NumDetailTypes() { return(1); }
	virtual const wchar_t	*GetDetailName(int idetail) { return(L"Missing disclaimers"); }
	virtual int				GetNumDetails(int idetail) { return(m_cnt_missing_disclaimers); }

	virtual void			ProcessFileData(TBasicFileInfo &basic_info);

	void					CheckDisclaimerPresence(TBasicFileInfo &basic_info, const wchar_t *xpct_disclaimer, bool allow_comments_continuation, bool req_trailing_empty_line);
	const wchar_t			*PickContext(wchar_t *buffer, const wchar_t *data, int data_len);

protected:

	int					m_cnt_missing_disclaimers;
};

class TKeywordSeparDetailsChecker : public TLexemaDetailsChecker
{
public:

	TKeywordSeparDetailsChecker(TSrcDetailsIterationHandler &handler);

	virtual int				NumDetailTypes() { return(3); }
	virtual const wchar_t	*GetDetailName(int idetail);
	virtual int				GetNumDetails(int idetail);

	virtual void			StartNewFileProcessing();
	virtual void			ProcessLexema(TLexema &prev_lex, TLexema &lex);

protected:

	void		ProcessKeyword(TLexema &lex);
	void		CheckSpaceAtTheEnd(TLexema &lex, const wchar_t *lexema_name);
	void		CheckSpacesOnBothSides(TLexema &lex, const wchar_t *lexema_name, bool lcrw_mode = FALSE, bool rcrw_mode = FALSE);
	void		CheckNoSpacesOnBothSides(TLexema &lex, const wchar_t *lexema_name);
	void		CheckSeparatorsAfter(TLexema &lex, const wchar_t *stmt_name, bool want_tab_sep, int num_separs);

protected:

	int					m_ignore_lex_cnt;

	int					m_cnt_events;
	int					m_cnt_error_lex;
	int					m_cnt_bad_comments;

	bool					m_for_lexema_flag;				// This flag indicates that the previous lexema was "for".
	int					m_for_header_pars_cnt;

	bool					m_ifndef_mentioned;
	bool					m_define_mentioned;
};

class TCommentedCodeChecker : public TLexemaDetailsChecker
{
public:

	TCommentedCodeChecker(TSrcDetailsIterationHandler &handler, double min_density = 0.5)
			: TLexemaDetailsChecker(handler) { m_min_density = min_density; m_cnt_events = 0; }

	virtual int				NumDetailTypes() { return(1); }
	virtual const wchar_t	*GetDetailName(int idetail) { return(L"Commented code fragments"); }
	virtual int				GetNumDetails(int idetail) { return(m_cnt_events); }

	virtual void			ProcessLexema(TLexema &prev_lex, TLexema &lex);

protected:

	long		SkipSequence(TRawScanner &scanner, long seq_beg_offs);

protected:

	double				m_min_density;

	int					m_cnt_events;
							// Number of discovered commented code fragments.
};

//
// This checker ensures that all Win32 API finctions are preceeded with "::".
//
class TWin32ApiStyleChecker : public TLexemaDetailsChecker
{
public:

	TWin32ApiStyleChecker(TSrcDetailsIterationHandler &handler);

	virtual int				NumDetailTypes() { return(1); }
	virtual const wchar_t	*GetDetailName(int idetail) { return(L"Improper Win32 API uses"); }
	virtual int				GetNumDetails(int idetail) { return(m_cnt_bad_uses); }

	virtual void			ProcessLexema(TLexema &prev_lex, TLexema &lex);
	virtual void			PrintCustomReport();

protected:

	bool				m_prev_prev_lex_name;
	int				m_cnt_bad_uses;

	TStringsDict		m_known_api;
	TStringsDict		m_noticed_api;

};

#endif	// Processing_RawProcessing_H


