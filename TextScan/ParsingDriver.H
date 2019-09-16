//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	TextScan_ParsingDriver_H
#define	TextScan_ParsingDriver_H

#ifndef   TextScan_MidScan_H
#include  "TextScan/MidScan.H"
#endif
#ifndef   TextScan_BuildLog_H
#include  "TextScan/BuildLog.H"
#endif

class TGenericParser;

//
// The parser console should not be necessarily supported by every generic parser.
// This console is primarily used by the grammar based parsers. The parser console is
// defined as part of the TGenericParser interface primarily for the code simplification.
//
enum TGenericParserConsoleEvent
{
	pcev_none,						// No event. This value is used only for variables that keep the latest event
									// value when it is necessary to store the fact that there is no event.
	pcev_parsing_startup,			// Parsing has just started. The source file is already loaded but nothing is done yet.
									// When the data param is TRUE, this means that parsing started, otherwise this
									// means that grammar is either missing or not compatible with the current parser.
	pcev_parsing_complete,			// This event is sent at the end of parsing. Event is sent both on successful
									// and uncussessful (critical eror) termination.
	pcev_symbol_pickup,			// When this event is sent, new symbol object is already added to the right
									// hand side of the internal input queue of the grammar based parser.
	pcev_symbol_update,			// Application layer decided to modify symbol value of the picked
									// symbol. This should primarily happen during the error recovery.
	pcev_symbol_discard,			// Symbol at the right hand side of the input symbols queue is about
									// to be discarded.
	pcev_action_pickup,				// The action was just picked up from the analysis table. Nothing is done
									// with this action yet. Data parameter contains the action value.
	pcev_action_resolution,			// Action was taken from the conflict resolution vector. Nothing is done
									// with this action yet. Data parameter contains the action value.
	pcev_action_update,				// Application layer decided to modify the parsing action. Most often this will
									// happen during dynamic conflicts resolution. Data param is the action value.
	pcev_action_shift,				// Left had side symbol of the input queue is about to be shifted into the stack.
									// Data parameter is the new state of the parser.
	pcev_action_resolve,			// The grammar rule is about to be applied. The data parameter is an action value
									// that contains an index of the rule of the current grammar that is about to be applied.
	pcev_enter_nesting,				// This event is sent while processing the conflict action when it is already clear
									// that nested grammar is available.
	pcev_exit_nesting,				// This event is sent just before exiting the nested parsing. Data parameter is
									// the rule index.
	pcev_errec_sym_added,			// A fake symbol is added into input queue to recover from the syntax error.
									// The data param contains an index of the symbol in the queue.
	pcev_errec_sym_updated,		// Symbol is modified in the input queue to recover from the syntax error.
									// The data param contains an index of the symbol in the input queue.
	pcev_errec_sym_removed,		// Symbol is removed from the input queue to recover from the syntax error.
									// The data param contains an index of the symbol in the queue.
	pcev_grpars_cfct_pickup,		// This is special event that is never sent by the grammar based parser itself.
									// It is used inside the parsing debugger only.
	pcev_num_items,
};

#define PCEV_START_STOP ((1 << pcev_parsing_startup) | (1 << pcev_parsing_complete))

//
// This union has only aux meaning. It is not used for making any decisions during
// the parsing process. It is passed as a third param to the ProcessEvent() method.
//
union TParsingActionOriginInfo
{
	struct
	{
		WORD	initial_action;			// Parsing action as it was taken either from the parsing table or from
										// the conflict resolution vector.
		WORD	conflict_action;			// Index of the conflict in the form of the parsing action or zero.
	};

	DWORD		dw_action_origin;
};

//
// The parser console is not expected to create logs of notifications for future analysis. Its major purpose
// is displaying the current state of the parser and changes in its current state.
//
class TGenericParserConsole
{
public:

	TGenericParserConsole(TGenericParser *parser = NULL) { m_parser = parser; }
					// Default ctor.

	virtual void ProcessEvent(TGenericParserConsoleEvent evt_code, WORD &evt_data, DWORD event_data_ex) = 0;
					// Note that the second params is a reference. This means that in some cases this method will
					// modify the passed evt_data value. The third param is relevant only for pcev_action_shift and
					// pcev_action_resolve event codes. In all other cases the passed value is 0xFFFFFFFF.

	virtual void RecycleParserConsoleData(TListItem *data) = 0;
					// In many cases the derived class will recycle its data inside of processing the parser console
					// events and clear the appropriate field in the data structure of its parser.
					// This method is called when the parser needs to recycle the symbol descriptor and the parser
					// console data field there is not NULL.
protected:

	TGenericParser	*m_parser;
};

//
//  The viewer of the names table is an optional feature. It is primarily used in the VisualParser.
//
enum TParserNamesTableViewerType
{
	pntvc_none,			// The parser has no names table viewer or maybe it does not any names table at all.
	pntvc_cpp,			// The C/C++ names table viewer.
};

//
//  Generic parser defines prototype for the "lexemas loop handler".
//
//  Generic parser itself implements the most simple procedure that sends all lexemas from the source file
//  to the callback handler. Applications should overwrite virtual methods of this class and implement more
//  complex processing as it is appropriate.
//
class TGenericParser
{
public:

	TGenericParser(TMidLevScanner *scanner = NULL)
				{ m_parser_console = NULL; m_scanner = scanner; m_critical_error_code = 0; m_critical_error_descr = NULL; }

	virtual WORD	GetParserIdentificationCode() const { return(0); }
					// This method provides limited capability for checking the parser type.

	virtual TParserNamesTableViewerType GetNamesTableViewerCode() const { return(pntvc_none); }
					// This method should provide response based on the nature of the derived class. Return value pntvc_none
					// means that parser either does not have any names table or there is no viewer for this table.

	virtual bool	SetupParserConsole(TGenericParserConsole *console) { return(FALSE); }
					// When theurn value is FALSE, this means that parser does not suport functionality of the parser console
					// and hense it will never the passed callback in theure.

	virtual void	SetupScanner(TMidLevScanner *pScanner) { m_scanner = pScanner; }

	virtual void	ResetParser() { m_critical_error_code = 0; m_critical_error_descr = NULL; }

	void			RaiseCriticalError(PrEventHeader *ctx, int err_code, const wchar_t *short_static_description, const wchar_t *detailed_error_message = NULL);
	virtual void	RaiseCriticalErrorNotification() { }
					// Note that the first method is not virtual.

	TGenericParserConsole		*ParserConsole() { return(m_parser_console); }
	TMidLevScanner			*Scanner() { return(m_scanner); }

	inline  bool				CheckForCriticalError() const { return(m_critical_error_code != 0); }
	inline  int					GetCriticalErrorCode() const { return(m_critical_error_code); }
	inline  const wchar_t		*GetCriticalErrorDescription() const { return(m_critical_error_descr); }

	virtual bool Parse();
				// This is the major API of the generic parser. This method is expected to be overriden by the derived class
				// and it should contain a loop over all lexemas in the source.
protected:

	TGenericParserConsole		*m_parser_console;
								// Parser console should be used as a parsing debugger. The current state of the parsing
								// process should be displayed on the scanner console that is a field of the TMidLevScanner.

	TMidLevScanner			*m_scanner;
								// The source of the input lexemas.

	int						m_critical_error_code;
	wchar_t					*m_critical_error_descr;
								// Info about the critical error is stored in the data fields of the object to allow checking it by entities
								// like a parsing debugger. It is better to store this information separately from the TMidLev callback
								// handler. Critical error desription is expected to be a short static string. These fields are declared
								// as private to enforce using the RaiseCriticalError() method.

	enum { CRITICAL_ERROR_DTLS_LEN = 256 };

	wchar_t					m_critical_error_details[CRITICAL_ERROR_DTLS_LEN];
								// After raising the critical error derived class can prepare this details message that will be dispayed
								// in the debugger if one is attached. In some cases it might be reasonable to make this message
								// identical to the error message of the critical error that is reported to the MidLev callback handler,
								// in some cases not. This is why this field is not set in the RaiseCriticalError() method.
};

class TParsingDriver;

class TFileDataIterationHandler
{
public:		//
			//  This class is a visitor for the method TParsingDriver::ProcessSubtreeFilesRawData().
			//  This method iterates files. It loads them into the memory and calls this callback handler.
			//  The nature of processing can be anything but most likely it will be a raw scanning.
			//

		TFileDataIterationHandler() { m_owner_driver = NULL; }

	TGenericConsoleEx		&Console();
							// Unfortunately this simple method cannot be implemented inline.

	virtual void	ProcessDirectoryName(const wchar_t *subdir_name) { }
					// Passed pointer points into the internal buffer of the parsing driver. Derived class should
					// not modify this name. Contents of this buffer will be no longer valid after the return from
					// this method.

	virtual void	ProcessDataFile(TBasicFileInfo &basic_info) { }
					// Derived class can take data from the passed structure and NULL the pointers there.

			//
			//  If a derived class wants to abort the iteration, it should raise the abort flag in the generic console.
			//

protected:

	TParsingDriver			*m_owner_driver;
								// Parsing driver will always set a non NULL pointer in this field before calling
								// the ProcessDirectoryName() or ProcessDataFile() methods.
	friend class TParsingDriver;
};

struct TBatchParsingSummary
{
	ID						batch_id;

	ULONG					files_processed;
	ULONG					dirs_processed;

	ULONG					num_errors;
	ULONG					num_warnings;

	ULONG					run_time_errors;				// Copy of the count of the console errors.
	bool						abort_flag;						// Copy of the console abort flag.

	TDateTime				batch_duration;					// This field may contain duration of several batches, if processing of
															// subdirectories was started with init_batch_start_time == FALSE.

	void Clear() { memset(this, 0, sizeof(TBatchParsingSummary)); }
};

// This struct is used when params of the ProcessSourceFile() method are passed between different threads.
struct TProcessSourceFileInfo
{
	ID						batch_id;
	THighLevelLangType		lang_type;

	wchar_t					*src_fname;
	wchar_t					*base_dir;
	wchar_t					*cmdl_format;
	TParseEnvInfo				*cmdl_env_info;					// This field should be not NULL only if the cmdl_format is not NULL.
															// Otherwise this field is not used.
	TDriveSubstInfo			*drive_substs_array;
	bool						allow_interm_paths;
	bool						allow_non_substituted_paths;
	bool						wait_for_user_resp;

	inline void Clear() { memset(this, 0, sizeof(TProcessSourceFileInfo)); }
};

// This struct is used when params of the ProcessCommandLine() method are passed between different threads.
struct TProcessCommandLineInfo
{
	ID						batch_id;
	THighLevelLangType		lang_type;

	wchar_t					*cmdl_fname;
	wchar_t					*cmdl_contents;
	int						mj_src_prm;
	TParseEnvInfo				*cmdl_env_info;
	wchar_t					*base_dir;

	TDriveSubstInfo			*drive_substs_array;
	bool						allow_interm_paths;
	bool						allow_non_substituted_paths;
	bool						wait_for_user_resp;

	TSourceAreaInfo			*cmdl_origin;

	inline void Clear() { memset(this, 0, sizeof(TProcessCommandLineInfo)); }
};

//
//  The parsing driver puts together several pieces of the parsing infrastructure.
//
class TParsingDriver
{
public:

		TParsingDriver(TGenericParser &parser,
				TMidLevScanCbkHandler &mid_lev_cbk_handler,
				TMidLevScannerOptions *initial_scanner_opts = NULL,
				TMidLevScanCmdlCbkHandler *cmdl_cbk_handler = NULL,
				TMidLevPrepOptions *initial_prep_opts = NULL);
		~TParsingDriver();

	TGenericParser	&Parser() { return(m_parser); }
						// This is a read accessor for the parser. The parser cannot be changed during the life
						// of the parsing driver.

	TMidLevScanner	&Scanner() { return(m_scanner); }
						// Read accessor for the mid lev scanner. The scanner is a data field inside the driver.

	void		SetupScannerOptions(TMidLevScannerOptions *initial_scanner_opts = NULL, TMidLevPrepOptions *initial_prep_opts = NULL);
				// The parsing options are passed to the mid lev scanner each time when the new parsing session is started.
				// This means that the scanning options, that are stored in the driver, can be updated before or after any call
				// to any of the ProcessXys() methods.

	bool		ProcessSourceFile(ID batch_id,					// It is ok to pass 0 here.
				THighLevelLangType lang_type,						// The lang type should not be equal to lang_none.
				const wchar_t *src_fname,							// Short or full name of the file to process.
				const wchar_t *base_dir = NULL,					// If the param value is NULL, then the current dir of the process is used.
				const wchar_t *cmdl_format = NULL,					// This param should either contain format with one "%s" parameter or it should be NULL.
				TParseEnvInfo *cmdl_env_info = NULL,				// This param should be not NULL only if the cmdl_format is not NULL.
				TDriveSubstInfo *drive_substs_array = NULL,
				bool  allow_interm_paths = FALSE,
				bool  allow_non_substituted_paths = FALSE,
				bool  wait_for_user_resp = FALSE,
				bool *parsing_sess_started = NULL);

	bool		ProcessSourceFile(TProcessSourceFileInfo &info, bool *parsing_sess_started = NULL);

				//
				// These methods parse the src file without using a command line or they use dynamicaly generated
				// cmdl that is generated using a simple format string. When the command line is not used, the app
				// can customize the parsing process (ex: set the inc paths) only from inside the StartPrepPhase(...)
				// method.
				//
				// Return value shows if an error has happened before calling the parser (the return value is FALSE)
				// or the generic parser WAS called and it did some work (the return value TRUE).
				//

	bool		ProcessCommandLine(ID batch_id,					// It is ok to pass 0 here.
				THighLevelLangType lang_type,
				const wchar_t *cmdl_fname,
				const wchar_t *cmdl_contents,
				int mj_src_prm,										// The pos params numbering starts from 0. The index zero is used for the name of the app.
																	// This means that the first name of the source file on the cmd line has index equal to 1.
				TParseEnvInfo *cmdl_env_info,
				const wchar_t *base_dir = NULL,
				TDriveSubstInfo *drive_substs_array = NULL,
				bool  allow_interm_paths = FALSE,
				bool  allow_non_substituted_paths = FALSE,
				bool  wait_for_user_resp = FALSE,
				TSourceAreaInfo *cmdl_origin = NULL,
				bool *parsing_sess_started = NULL);

	bool		ProcessCommandLine(TProcessCommandLineInfo &info, bool *parsing_sess_started = NULL);

				//
				// Command line can have either the name or contents or both. If the command line has contents,
				// the driver uses the strg_temp to create a file descriptor. If the name is missing, the driver assigns
				// a name to the commnad line based on the parsing id that is being opened. If command line body
				// is missing, the driver attempts to open a disk file using the passed name.
				//
				// Return value shows if an error has happened before calling the parser (the return value is FALSE)
				// or the generic parser WAS called and it did some work (the return value TRUE).
				//

	void		ProcessSubtreeFilesRawData(TFileDataIterationHandler &handler,
				const wchar_t *directory_name,
				bool recurse_subdirs,
				const wchar_t **list_of_fname_exts_to_process,			// An empty string enables processing file names without extensions.
																	// Otherwise every extension string should start with the dot and it may
																	// contain an asterisk at its end. Extensions are case insensitive.
																	// Passing NULL pointer as a parameter instructs to process all files.
				bool  wait_for_user_resp = FALSE,
				TBatchParsingSummary *pSumry = NULL);

				//
				// Method loads the file data into the memory and passes this data to the callback handler.
				//

	void		ProcessSubtreeFilesWithParsing(ID batch_id,		// Passing -1 will create a new batch.
				THighLevelLangType lang_type,
				const wchar_t *directory_name,
				bool recurse_subdirs,
				const wchar_t **list_of_fname_exts_to_process,			// An empty string enables processing file names without extensions.
																	// Otherwise every extension string should start with the dot and it may
																	// contain an asterisk at its end. The extensions are case insensitive.
																	// Passing NULL pointer instructs to process all files in the directory.
				const wchar_t *cmdl_format = NULL,						// The passed string should be either NULL or it should be a format string
																	// with only one "%s" formatting parameter.
				TParseEnvInfo *cmdl_env_info = NULL,					// This param should be not NULL only if the cmdl_format is not NULL.
				TDriveSubstInfo *drive_substs_array = NULL,
				bool  allow_interm_paths = FALSE,
				bool  allow_non_substituted_paths = FALSE,
				bool  wait_for_user_resp = FALSE,
				TBatchParsingSummary *pSumry = NULL);

				//
				// Method parses all files in the directories subtree if this is requested.
				//
				// The list of extensions is an array of pointers to extensions. The array itself should be terminated with
				// a NULL pointer. All extensions should include dots and be simple strings like ".cxx", ".h*". An asterisk
				// can be present only at the end of extension. Empty string matches to file names without extensions.
				//

	void		ProcessBuildLog(ID batch_id,							// Passing -1 will create a new batch.
				THighLevelLangType lang_type,
				TBuildLogInfo &build_log,
				TDriveSubstInfo *drive_substs_array = NULL,
				bool  allow_interm_paths = FALSE,
				bool  allow_non_substituted_paths = FALSE,
				bool  wait_for_user_resp = FALSE,
				TBatchParsingSummary *pSumry = NULL);

				//
				// Method processes all files that are marked for processing in the build log parameter. This function
				// is not changing any directory names. They are used exactly as they are present in the log.
				//

	void		UpdateBatchDuration(bool force_update);
				// This method is declared public to allow implementation of several batch sessions
				// using the same instance of the parsing driver.

protected:

	enum { FILE_NAME_EXT_LEN = 12 };

	struct TFileNameExtInfo
	{
		wchar_t		ext_text[FILE_NAME_EXT_LEN];
		int			solid_part_len;
						//
						// Three types of records are possible depending on the value of the solid_part_len field.
						//
						//		solid_part_len  <  0		--	Empty record. This record is ignored during the iteration.
						//		solid_part_len == 0		--	Record describes extension without an asterisk at the end.
						//		solid_part_len  >  0		--	Length of the extension text in front of the trailing asterisk.
						//
						// Empty records are needed to distinguish empty filter from the filter that allows processing
						// only file names without extensions and nothing else.
						//
	};

	typedef TStructsArray<TFileNameExtInfo, 16, 16> TFileNameExtInfosArray;

	class TFileSystemFilesIterator : public TDirectoriesSubtreeIterator
	{
	public:		//
				// This class is a wrapper above the TDirectoriesSubtreeIterator that adds
				// functionality of the file name extension based filtering of the files.
				//

		TFileSystemFilesIterator(TParsingDriver *owner) : m_owner(*owner) { m_allow_no_ext_fnames = FALSE; m_summary.Clear(); }

		virtual void ProcessSubdirectory(const wchar_t *subdir_name)
		{
			m_summary.dirs_processed++;
		}

		bool BuildFileNameExtensionsFilter(const wchar_t **list_of_fname_exts_to_process)
		{
			m_exts_data.Clear();
			m_allow_no_ext_fnames = FALSE;
			if (list_of_fname_exts_to_process == NULL)
				return(TRUE);

			// The filter is not empty. Validity of this filter strings is already verified. In particular it was ensured
			// that each string is either empty or it starts with a dow and that it has reasonable length.
			TGenericConsole &cons = m_owner.m_scanner.Console();
			while (*list_of_fname_exts_to_process != NULL)
			{
				if (m_exts_data.ReserveSpace() == FALSE)
				{
					cons.ShowRunTimeError(L"Low on memory while setting up the list of file name extensions to process.");
					return(FALSE);
				}

				// The length of the ext string is already verified.
				TFileNameExtInfo record;
				wcscpy(record.ext_text, *list_of_fname_exts_to_process);
				_wcslwr(record.ext_text);

				if (record.ext_text[0] == 0)
				{
					// This record instructs to process files without extensions.
					record.solid_part_len = -1;
					m_allow_no_ext_fnames = TRUE;
				}
				else
				{
					wchar_t *pastr = wcschr(record.ext_text, L'*');
					record.solid_part_len = (pastr != NULL) ? (int)(INT_PTR)(pastr-record.ext_text) : 0;
				}

				m_exts_data.AppendItem(record);
				list_of_fname_exts_to_process++;
			}

			// Success.
			return(TRUE);
		}

		bool CheckFileNameExtension(const wchar_t *file_name)
		{
			if (m_exts_data.NumItems() <= 0)
			{
				// The filter is empty. Accept the file name.
				return(TRUE);
			}

			// The filter is not empty. Pick up the file name extension.
			wchar_t *curr_ext = wcsrchr((wchar_t*)file_name, L'.');
			if (curr_ext == NULL)
			{
				// File names without extensions have explicit property that controls them.
				return(m_allow_no_ext_fnames);
			}

			// Passed file name has extension and the list of extensions is available. Copy extension into
			// the internal buffer and make it lower case before comparing it with the elements of the filter.
			wchar_t ext_buff[FILE_NAME_EXT_LEN];
			wcsncpy(ext_buff, curr_ext, FILE_NAME_EXT_LEN);
			ext_buff[FILE_NAME_EXT_LEN-1] = 0;
			_wcslwr(ext_buff);

			TFileNameExtInfo *pe = m_exts_data.DataPtr();
			for (int cnt=0; cnt<m_exts_data.NumItems(); ++cnt, ++pe)
			{
				if (pe->solid_part_len == 0)
				{
					// This is a record without an asterisk. The extensions should match exactly.
					if (wcscmp(pe->ext_text, ext_buff) == 0)
						return(TRUE);
				}
				else if (pe->solid_part_len > 0)
				{
					// This is a record with an asterisk at the end. Only the prefix of the extension should match.
					if (wcsncmp(pe->ext_text, ext_buff, pe->solid_part_len) == 0)
						return(TRUE);
				}
			}

			// Passed file name has not passed the filter.
			return(FALSE);
		}

		void CheckProcessAbortFlag()
		{
			// If the interactive user or the app layer wants to abort the iteration, they should raise the abort flag
			// in the generic console object. This should be reflected on the console labels and the base class should
			// be notified about this.
			if (m_owner.m_scanner.Console().GetAbortFlag() == TRUE)
			{
				m_owner.ShowAbortingMessage();
				m_abort_iteration = TRUE;
			}
		}

	protected:

		TParsingDriver				&m_owner;

		TFileNameExtInfosArray			m_exts_data;
		bool							m_allow_no_ext_fnames;

		TBatchParsingSummary			m_summary;

		friend class TParsingDriver;
	};

	class TParsingProcessingHelper : public TFileSystemFilesIterator
	{
	public:

		TParsingProcessingHelper(TParsingDriver *owner,
									ID bid, THighLevelLangType langt,
									wchar_t *cmdl_fmt, TParseEnvInfo *cmdl_env,
									TDriveSubstInfo *substs, bool ainp, bool ansp)
				: TFileSystemFilesIterator(owner)
		{
			batch_id = bid; lang_type = langt;
			cmdl_format = cmdl_fmt; cmdl_env_info = cmdl_env;
			drive_substs = substs; allow_interm_paths = ainp;
			allow_non_substed_paths = ansp;
		}

		virtual void ProcessFileShortName(const wchar_t *subdir_name, const wchar_t *short_file_name)
		{
			if (CheckFileNameExtension(short_file_name) == TRUE)
			{
				// Either the extensions filter is empty or the current file name has passed the filter.
				m_owner.ProcessSourceFile(batch_id, lang_type, short_file_name, subdir_name,
								cmdl_format, cmdl_env_info, drive_substs, allow_interm_paths,
								allow_non_substed_paths, FALSE);
				m_owner.ShowParsingSessionResults(m_summary);
			}

			CheckProcessAbortFlag();
		}

	protected:

		ID							batch_id;
		THighLevelLangType			lang_type;

		wchar_t						*cmdl_format;
		TParseEnvInfo					*cmdl_env_info;

		TDriveSubstInfo				*drive_substs;
		bool							allow_interm_paths;
		bool							allow_non_substed_paths;
	};

	class TRawDataProcessingHelper : public TFileSystemFilesIterator
	{
	public:
			TRawDataProcessingHelper(TParsingDriver *owner, TFileDataIterationHandler &handler) : TFileSystemFilesIterator(owner), m_handler(handler) { }

		virtual void ProcessSubdirectory(const wchar_t *subdir_name)
		{
			// No preparation is needed. Simply call the handler.
			m_handler.ProcessDirectoryName(subdir_name);

			TFileSystemFilesIterator::ProcessSubdirectory(subdir_name);
			CheckProcessAbortFlag();
		}

		virtual void ProcessFileFullName(const wchar_t *file_name)
		{
			if (CheckFileNameExtension(file_name) == TRUE)
			{
				// Load the file data from the disk. Use the passed name exactly as-is and allow empty files loading.
				TBasicFileInfo basic_info;
				TLoadSaveResult lds_res = TFileBodyHelper::LoadAsciiFileBody(basic_info, file_name, NULL, FALSE, TRUE);
				if (lds_res != ldres_success)
				{
					// Loading failed.
					wchar_t buffer[480];
					swprintf(buffer, 480, L"Error loading the data file.\r\n\r\nFile name: \t%s                      \r\nProblem: \t%s.\r\n\r\n"
									L"Do you want to skip this file and continue?", file_name, TFileBodyHelper::GetLoadSaveResultText(lds_res, TRUE));

					if (m_owner.m_scanner.Console().ShowRunTimeError(buffer, MB_YESNO) == IDNO)
					{
						m_owner.ShowAbortingMessage();
						m_owner.m_scanner.Console().SetAbortFlag(TRUE);
					}
				}
				else
				{
					// The file data was picked up. Call the app level callback.
					m_handler.ProcessDataFile(basic_info);
					basic_info.FreeBasicFileInfo();
				}
			}

			CheckProcessAbortFlag();
		}

	protected:

		TFileDataIterationHandler		&m_handler;
	};

protected:

	TGenericParser			&m_parser;
	TMidLevScannerOptions		m_scanner_opts;
	TMidLevPrepOptions		m_prep_opts;

	TMidLevScanner			m_scanner;

	bool						m_batch_mode;
	TDateTime				m_batch_start;
	TDateTime				m_batch_elapsed_displayed;

	bool						m_aborting_displayed;
								// This field prevents displaying the same message on the console several times.
private:

	bool		PrepareBatchProcessing(const wchar_t *dir_name, const wchar_t **list_of_fname_exts, TBatchParsingSummary *pSumry);
	void		DoBatchProcessing(ID batch_id, TFileSystemFilesIterator &iter, const wchar_t *dir_name, bool recurse_subdirs, bool  wait_for_user_resp, TBatchParsingSummary *pSumry);

	void		ShowParsingSessionResults(TBatchParsingSummary &summary);
	void		ShowAbortingMessage();
	void		WaitContinue(const wchar_t *message = NULL);
};

#endif	// TextScan_ParsingDriver_H


