//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/DebugLogger.H"
#include  "TextScan/MidScan.H"
#include  "TextScan/ParsingDriver.H"

// -------------------------------------------------------------------
//   ==================  TGenericParser  ====================
// -------------------------------------------------------------------

void TGenericParser::RaiseCriticalError(PrEventHeader *ctx, int err_code, const wchar_t *short_static_description, const wchar_t *detailed_error_message)
{
	assert(short_static_description != NULL && short_static_description[0] != 0);

	// For now a single critical error model is used.
	assert(m_critical_error_code == 0);

	// Save the parameter values for future use.
	m_critical_error_code = err_code;
	m_critical_error_descr = (wchar_t*)short_static_description;

	// Report this error to the mid lev callback.
	if (detailed_error_message == NULL)
		detailed_error_message = short_static_description;
	m_scanner->AddErrorWarning(ctx, 0, lerrc_syntax_err, err_code, detailed_error_message);

	// Send notification to the derived-derived (double derived) class.
	RaiseCriticalErrorNotification();
}

bool TGenericParser::Parse()
{
	// The scanner should be already set.
	assert(m_scanner != NULL);

	// Give all lexemas to the callback handler including the final eof.
	TLexema lex;
	while (m_critical_error_code == 0)
	{
		m_scanner->Scan(lex);
		m_scanner->CbkHandler().MidLevLexemaNotification(&lex);

		if (lex.type == ltx_eof)
			break;
	}

	// The parsing is successful when there are no critical errors and there are no syntax errors.
	return(m_critical_error_code == 0 && m_scanner->GetErrorsCount() == 0);
}

// -------------------------------------------------------------------
//   ===============  TFileDataIterationHandler  =================
// -------------------------------------------------------------------

TGenericConsoleEx &TFileDataIterationHandler::Console()
{
	return(m_owner_driver->Scanner().Console());
}

// ------------------------------------------------------------------
//   ==================  TParsingDriver  ====================
// ------------------------------------------------------------------

TParsingDriver::TParsingDriver(TGenericParser &parser,
					TMidLevScanCbkHandler &cbkHandler,
					TMidLevScannerOptions *initialScannerOpts,
					TMidLevScanCmdlCbkHandler *cmdlCbkHandler,
					TMidLevPrepOptions *initialPrepOpts)
		: m_parser(parser), m_scanner(&cbkHandler, cmdlCbkHandler)
{
	// Init the parser.
	m_parser.SetupScanner(&m_scanner);

	//
	// Copy in or set the default state of the scanning options.
	//

	if (initialScannerOpts != NULL)
		m_scanner_opts = *initialScannerOpts;
	else m_scanner_opts.Init();

	if (initialPrepOpts != NULL)
		m_prep_opts = *initialPrepOpts;
	else m_prep_opts.Clear();

	m_batch_mode = FALSE;
	m_batch_start = CurrDateTime();
	m_batch_elapsed_displayed = 0;
	m_aborting_displayed = FALSE;
}

TParsingDriver::~TParsingDriver()
{
	// Ctor of this class has set pointers to the local scanner into the passed parser.
	m_parser.SetupScanner(NULL);
}

void TParsingDriver::SetupScannerOptions(TMidLevScannerOptions *initial_scanner_opts, TMidLevPrepOptions *initial_prep_opts)
{
	if (initial_scanner_opts != NULL)
		m_scanner_opts = *initial_scanner_opts;
	if (initial_prep_opts != NULL)
		m_prep_opts = *initial_prep_opts;
}

bool TParsingDriver::ProcessSourceFile(DWORD batch_id,
						THighLevelLangType lang_type,
						const wchar_t *src_fname,
						const wchar_t *base_dir,
						const wchar_t *cmdl_format,			// Format should be NULL or it should contain format with only one "%s" parameter.
						TParseEnvInfo *cmdl_env_info,			// This param should be not NULL only if cmdl_format is not NULL.
						TDriveSubstInfo *drive_substs,
						bool  allow_interm_paths,
						bool  allow_non_substed_paths,
						bool  wait_for_user_resp,
						bool *parsing_sess_started)
{
	if (parsing_sess_started != NULL)
		*parsing_sess_started = FALSE;

	if (m_batch_mode == FALSE)
		m_aborting_displayed = FALSE;

	// Open a new parsing session. This will also reset the previous state of the scanner if any.
	if (m_scanner.BeginParsingSession(batch_id, base_dir, drive_substs, allow_interm_paths, allow_non_substed_paths) == FALSE)
	{
		// MidLev callback should display an error on the console.
		return(FALSE);
	}

	// The parsing session has managed to start.
	if (cmdl_format != NULL)
	{
		// User requested a complex procedure.
		assert(cmdl_env_info != NULL);

		// Esure that the passed format contains only one "%s" param specifier.
		const wchar_t *ptr_prc = wcschr(cmdl_format, L'%');
		assert(ptr_prc != NULL && ptr_prc[1] == L's' && wcschr(ptr_prc+1, L'%') == NULL);

		// Generate the command line and create the file descriptor for it.
		wchar_t cmdl_buff[2048];
		swprintf(cmdl_buff, 2048, cmdl_format, src_fname);
		TFileInfoItem *cmdl_finfo = m_scanner.CreateFileInfo(NULL, strg_temp, cmdl_env_info->cmdl_rscn_type, L"@cmdl", cmdl_buff);
		if (cmdl_finfo == NULL)
		{
			// A console error should be already displayed.
			m_scanner.CloseParsingSession();
			return(FALSE);
		}

		// Start parsing from the generated command line. Set param index of the major src file to one.
		TCommandLineTaskInfo cmdl_task_info;
		cmdl_task_info.Init(*cmdl_env_info, 1, lang_type);
		if (m_scanner.BeginPrepPhase(cmdl_finfo, cmdl_task_info, &m_prep_opts, &m_scanner_opts) == FALSE)
		{
			// A console error should be already displayed.
			m_scanner.CloseParsingSession();
			return(FALSE);
		}
	}
	else
	{
		// Load the major source file into memory and place it into the cache.
		TRawScanningType rscn = (lang_type == lang_csh) ? rscn_csh : rscn_c_cpp;
		TFileInfoItem *major_src = m_scanner.CreateFileInfo(NULL, strg_disk, rscn, src_fname);
		if (major_src == NULL)
		{
			// A console error should be already displayed.
			m_scanner.CloseParsingSession();
			return(FALSE);
		}

		// Set this file as the root of the lexema sources.
		if (m_scanner.BeginMainPhase(lang_type, major_src, NULL, NULL, &m_scanner_opts) == FALSE)
		{
			// A console error should be already displayed.
			m_scanner.CloseParsingSession();
			return(FALSE);
		}
	}

	if (parsing_sess_started != NULL)
		*parsing_sess_started = TRUE;

	// Do the processing. Its nature depends on the nature of the parser that was passed to the ctor of this object.
	m_parser.Parse();

	// Show the final message.
	if (wait_for_user_resp == TRUE)
	{
		WaitContinue((m_scanner.Console().GetAbortFlag() == TRUE) ? L"Parsing was aborted." : L"Parsing complete.");
	}

	// Finally close the session without discarding the disk files cache.
	ULONG num_errors = m_scanner.GetErrorsCount();
	m_scanner.CloseParsingSession(FALSE);

	// All major parsing steps have succeeded.
	return(num_errors == 0);
}

bool TParsingDriver::ProcessSourceFile(TProcessSourceFileInfo &info, bool *parsing_sess_started)
{
	// Simply pass all fields of the struct as params.
	return(ProcessSourceFile(info.batch_id, info.lang_type,
				info.src_fname, info.base_dir,
				info.cmdl_format, info.cmdl_env_info,
				info.drive_substs_array,
				info.allow_interm_paths,
				info.allow_non_substituted_paths,
				info.wait_for_user_resp,
				parsing_sess_started));
}

bool TParsingDriver::ProcessCommandLine(DWORD batch_id,
						THighLevelLangType lang_type,
						const wchar_t *cmdl_fname,
						const wchar_t *cmdl_contents,
						int   mj_src_prm,							// Pos params numbering starts from 0. Index zero is used for the name of the app.
																// This means that the first name of the source file on the cmd has index equal to 1.
						TParseEnvInfo *cmdl_env_info,
						const wchar_t *base_dir,
						TDriveSubstInfo *drive_substs,
						bool  allow_interm_paths,
						bool  allow_non_substed_paths,
						bool  wait_for_user_resp,
						TSourceAreaInfo *cmdl_origin,
						bool *parsing_sess_started)
{
	assert(cmdl_fname != NULL || cmdl_contents != NULL);

	if (parsing_sess_started != NULL)
		*parsing_sess_started = FALSE;

	if (m_batch_mode == FALSE)
		m_aborting_displayed = FALSE;

	// Open a new parsing session. This will also reset the previous state of the scanner if any.
	if (m_scanner.BeginParsingSession(batch_id, base_dir, drive_substs, allow_interm_paths, allow_non_substed_paths) == FALSE)
	{
		// MidLev callback should display an error on the console.
		return(FALSE);
	}

	// Create TFileInfo object for the command line.
	TFileInfoItem *cmdl_finfo = NULL;
	if (cmdl_contents != NULL)
	{
		// Create a temp file descriptor.
		wchar_t fname_buff[80];
		const wchar_t *fname = (cmdl_fname != NULL) ? cmdl_fname : fname_buff;
		if (cmdl_fname == NULL)
			swprintf(fname_buff, 80, L"@cmdl-%lu", m_scanner.CbkHandler().GetParsingId());
		cmdl_finfo = m_scanner.CreateFileInfo(NULL, strg_temp, cmdl_env_info->cmdl_rscn_type, fname, cmdl_contents);
	}
	else
	{
		// Load the regular disk file.
		cmdl_finfo = m_scanner.CreateFileInfo(NULL, strg_disk, cmdl_env_info->cmdl_rscn_type, cmdl_fname);
	}

	// Check the file registering results.
	if (cmdl_finfo == NULL)
	{
		// A console error should be already displayed.
		return(FALSE);
	}

	// Begin the preparation phase.
	TCommandLineTaskInfo cmdl_task_info;
	cmdl_task_info.Init(*cmdl_env_info, mj_src_prm, lang_type);
	if (cmdl_origin != NULL)
		cmdl_task_info.cmdl_origin = *cmdl_origin;

	if (m_scanner.BeginPrepPhase(cmdl_finfo, cmdl_task_info, &m_prep_opts, &m_scanner_opts) == FALSE)
	{
		// A console error should be already displayed.
		m_scanner.CloseParsingSession();
		return(FALSE);
	}

	if (parsing_sess_started != NULL)
		*parsing_sess_started = TRUE;

	// Do the processing. Its nature depends on the nature of the parser that was passed to the ctor of this object.
	m_parser.Parse();

	// Show the final message.
	if (wait_for_user_resp == TRUE)
	{
		WaitContinue((m_scanner.Console().GetAbortFlag() == TRUE) ? L"Parsing was aborted." : L"Parsing complete.");
	}

	// Finally close the session without discarding the disk files cache.
	ULONG num_errors = m_scanner.GetErrorsCount();
	m_scanner.CloseParsingSession(FALSE);

	// All major parsing steps have succeeded.
	return(num_errors == 0);
}

bool TParsingDriver::ProcessCommandLine(TProcessCommandLineInfo &info, bool *parsing_sess_started)
{
	// Simply pass all fields of the struct as parameters.
	return(ProcessCommandLine(info.batch_id, info.lang_type,
				info.cmdl_fname, info.cmdl_contents, info.mj_src_prm,
				info.cmdl_env_info, info.base_dir, info.drive_substs_array,
				info.allow_interm_paths, info.allow_non_substituted_paths,
				info.wait_for_user_resp, info.cmdl_origin,
				parsing_sess_started));
}

void TParsingDriver::ProcessSubtreeFilesRawData(TFileDataIterationHandler &handler,
										const wchar_t *directory_name, bool recurse_subdirs, const wchar_t **list_of_fname_exts,
										bool  wait_for_user_resp, TBatchParsingSummary *pSumry)
{
	if (PrepareBatchProcessing(directory_name, list_of_fname_exts, pSumry) == FALSE)
	{
		// An error is already displayed or reported.
		return;
	}

	// Prepare iterator and setup the extensions info.
	TRawDataProcessingHelper iter(this, handler);
	if (iter.BuildFileNameExtensionsFilter(list_of_fname_exts) == FALSE)
	{
		// An error is already displayed.
		return;
	}

	// Call the worker method.
	DoBatchProcessing(0, iter, directory_name, recurse_subdirs, wait_for_user_resp, pSumry);
}

void TParsingDriver::ProcessSubtreeFilesWithParsing(ID batch_id,			// Passing -1 will create a new batch.
							THighLevelLangType lang_type,
							const wchar_t *dir_name, bool recurse_subdirs, const wchar_t **list_of_fname_exts,
							const wchar_t *cmdl_format, TParseEnvInfo *cmdl_env_info,
							TDriveSubstInfo *drive_substs, bool allow_interm_paths, bool allow_non_substed_paths,
							bool wait_for_user_resp, TBatchParsingSummary *pSumry)
{
	if (PrepareBatchProcessing(dir_name, list_of_fname_exts, pSumry) == FALSE)
	{
		// An error is already displayed.
		return;
	}

	if (batch_id == ((DWORD)-1))
	{
		// Allocate a new batch id with the help of the mid lev callback.
		if (m_scanner.CbkHandler().AllocateBatchId(batch_id) == FALSE)
		{
			// An error should be already displayed.
			return;
		}
	}

	// Prepare iterator for the recursive directory scanning.
	TParsingProcessingHelper iter(this, batch_id, lang_type, (wchar_t*)cmdl_format, cmdl_env_info,
							drive_substs, allow_interm_paths, allow_non_substed_paths);

	// Setup the extensions info.
	if (iter.BuildFileNameExtensionsFilter(list_of_fname_exts) == FALSE)
	{
		// An error is already displayed or reported.
		return;
	}

	// Call the worker method.
	DoBatchProcessing(batch_id, iter, dir_name, recurse_subdirs, wait_for_user_resp, pSumry);
}

void TParsingDriver::ProcessBuildLog(ID batch_id,								// Passing -1 will create a new batch.
						THighLevelLangType lang_type, TBuildLogInfo &build_log,
						TDriveSubstInfo *drive_substs,
						bool allow_interm_paths, bool allow_non_substed_paths,
						bool wait_for_user_resp, TBatchParsingSummary *pSumry)
{
	TGenericConsoleEx &cons = m_scanner.Console();

	m_aborting_displayed = FALSE;
	TBatchParsingSummary summary;
	summary.Clear();
	if (pSumry != NULL)
		pSumry->Clear();

	if (batch_id == ((DWORD)-1))
	{
		// Allocate a new batch id with the help of the midlev callback.
		m_batch_start = CurrDateTime();
		if (m_scanner.CbkHandler().AllocateBatchId(batch_id) == FALSE)
		{
			// An error should be already displayed.
			return;
		}
	}

	//
	// Run the main loop over all marked files in the log.
	//
	TFileNameBuffer base_dir;
	TTextBuffer<4096> cmdl_contents;
	m_batch_mode = TRUE;

	// Level 1: Iterate the subdirectories.
	int nDirs = build_log.NumSubdirs();
	for (int idir=0; idir<nDirs; ++idir)
	{
		TBuildLogSubdirInfo &sdi = build_log.Subdir(idir);
		base_dir.ClearBuffer();
		base_dir.Append(sdi.subdir_area.file_info->info.file_body+sdi.subdir_area.area_beg, sdi.subdir_area.area_len);
		if (base_dir.GetXpndError() == TRUE)
		{
			cons.ShowRunTimeError(L"ProcessBuildLog: Low on memory while preparing the subdirectory name.");
			return;
		}

		// Level 2: Iterate the command lines.
		int nCmdls = sdi.cmdls.NumItems();
		for (int ic=0; ic<nCmdls; ++ic)
		{
			TBuildLogCommandLine &cmdli = sdi.cmdls[ic];
			TSourceAreaInfo cmdl_area_info;
			cmdl_area_info.Init(cmdli.cmdl_area);
			cmdl_contents.ClearBuffer();
			cmdl_contents.Append(cmdli.cmdl_area.file_info->info.file_body+cmdli.cmdl_area.area_beg, cmdli.cmdl_area.area_len);

			if (cmdl_contents.GetXpndError() == TRUE)
			{
				cons.ShowRunTimeError(L"ProcessBuildLog: Low on memory while preparing the command line contents.");
				return;
			}

			// Level 3: Iterate the files on the command line.
			int nFiles = cmdli.files.NumItems();
			if (nFiles == 0)
			{
				// Process command line without the major source file at all.
				ProcessCommandLine(batch_id, lang_type, NULL, cmdl_contents,
									0, cmdli.parse_env, base_dir,
									drive_substs, allow_interm_paths, allow_non_substed_paths,
									FALSE, &cmdl_area_info);
			}
			else
			{
				// Iterate the non empty list of files.
				for (int ifl=0; ifl<nFiles; ++ifl)
				{
					ProcessCommandLine(batch_id, lang_type, NULL, cmdl_contents,
										ifl+1, cmdli.parse_env, base_dir,
										drive_substs, allow_interm_paths, allow_non_substed_paths,
										FALSE, &cmdl_area_info);
					ShowParsingSessionResults(summary);
				}
			}
		}
	}

	// The processing is complete.
	m_batch_mode = FALSE;
	UpdateBatchDuration(TRUE);

	// Wait for the user response if needed.
	if (wait_for_user_resp == TRUE)
	{
		bool abort_state = cons.GetAbortFlag();
		if (abort_state == TRUE)
			cons.SetAbortFlag(FALSE);

		WaitContinue((abort_state == TRUE) ? L"Build log processing was aborted." : L"The build log processing complete.");
		if (abort_state == TRUE)
			cons.SetAbortFlag(TRUE);
	}

	// Give out the summary.
	if (pSumry != NULL)
	{
		pSumry->batch_id = batch_id;
		pSumry->run_time_errors = cons.GetErrorsCount();
		pSumry->abort_flag = cons.GetAbortFlag();
		pSumry->batch_duration = CurrDateTime()-m_batch_start;
	}
}

void TParsingDriver::UpdateBatchDuration(bool force_update)
{
	TDateTime curr_elapsed = CurrDateTime()-m_batch_start;
	TDateTime elaps_diff = curr_elapsed-m_batch_elapsed_displayed;
	if ((elaps_diff & ~0x7FFFFF) != 0 || force_update == TRUE)
	{
		// It is high time to update the elapsed time field.
		wchar_t buffer[80];
		m_batch_elapsed_displayed = curr_elapsed;
		m_scanner.Console().SetConsoleLabel(sclb_batch_elapsed_time, FormatDuration(curr_elapsed, buffer, 80));
	}
}

bool TParsingDriver::PrepareBatchProcessing(const wchar_t *dir_name, const wchar_t **list_of_fname_exts, TBatchParsingSummary *pSumry)
{
	TGenericConsoleEx &cons = m_scanner.Console();
	cons.ResetErrorsCount();
	cons.SetAbortFlag(FALSE);
	m_aborting_displayed = FALSE;

	if (pSumry != NULL)
		pSumry->Clear();

	// Ensure that the bypassed name is a directory.
	if (TPathHelper::IsPathADir(dir_name) == FALSE)
	{
		cons.ShowRunTimeError(L"TParsingDriver: The bypassed path is not a valid directory name.");
		return(FALSE);
	}

	// Check the passed list of extensions.
	if (list_of_fname_exts != NULL)
	{
		// The extensions filter is not empty.
		while (*list_of_fname_exts != NULL)
		{
			if (**list_of_fname_exts != 0)
			{
				if (**list_of_fname_exts != L'.')
				{
					cons.ShowRunTimeError(L"TParsingDriver: The extension is not starting with the dot. The input data is bogus.");
					return(FALSE);
				}
				else if (wcslen(*list_of_fname_exts) >= FILE_NAME_EXT_LEN)
				{
					cons.ShowRunTimeError(L"TParsingDriver: The file name extension is too long.");
					return(FALSE);
				}
			}

			list_of_fname_exts++;
		}
	}

	// Success.
	m_batch_start = CurrDateTime();
	return(TRUE);
}

void TParsingDriver::DoBatchProcessing(ID batch_id, TFileSystemFilesIterator &iter, const wchar_t *dir_name, bool recurse_subdirs, bool  wait_for_user_resp, TBatchParsingSummary *pSumry)
{
	// Initialize the batch related fields on the console.
	TGenericConsoleEx &cons = m_scanner.Console();
	cons.SetConsoleLabel(sclb_run_time_errors, L"none");
	cons.SetConsoleLabel(sclb_batch_files_processed, L"0");
	cons.SetConsoleLabel(sclb_batch_dirs_processed, L"0");
	cons.SetConsoleLabel(sclb_batch_num_errors, L"none");
	cons.SetConsoleLabel(sclb_batch_num_warns, L"none");
	UpdateBatchDuration(TRUE);

	// Call the processing.
	m_batch_mode = TRUE;
	bool main_res = iter.Iterate(dir_name, recurse_subdirs);		// <--- This is real processing function.
	m_batch_mode = FALSE;
	UpdateBatchDuration(TRUE);

	if (main_res == FALSE)
	{
		wchar_t buffer[480];
		if (iter.IterationResuilt() == ERROR_NOT_ENOUGH_MEMORY)
			wcscpy(buffer, L"Low on memory while iterating files.\r\n\r\nSome of the files/subdirectories were not processed.");
		else swprintf(buffer, 480, L"Win32 iteration error: %lu.", iter.IterationResuilt());
		cons.ShowRunTimeError(buffer);
	}

	// Wait for the user response if needed.
	if (wait_for_user_resp == TRUE)
	{
		bool abort_state = cons.GetAbortFlag();
		if (abort_state == TRUE)
			cons.SetAbortFlag(FALSE);

		WaitContinue((abort_state == TRUE) ? L"Batch processing was aborted." : L"Batch processing complete.");
		if (abort_state == TRUE)
			cons.SetAbortFlag(TRUE);
	}

	// Give out the summary.
	if (pSumry != NULL)
	{
		*pSumry = iter.m_summary;
		pSumry->batch_id = batch_id;
		pSumry->run_time_errors = cons.GetErrorsCount();
		pSumry->abort_flag = cons.GetAbortFlag();
		pSumry->batch_duration = CurrDateTime()-m_batch_start;
	}
}

void TParsingDriver::ShowParsingSessionResults(TBatchParsingSummary &summary)
{
	// Update the status panel after processing the file.
	m_scanner.ShowConsoleValue(sclb_batch_files_processed, ++summary.files_processed);
	TMidLevParsingSummary sumry;
	m_scanner.GetParsingSummary(sumry);

	if (sumry.num_errors > 0)
	{
		summary.num_errors += sumry.num_errors;
		m_scanner.ShowConsoleValue(sclb_batch_num_errors, summary.num_errors);
	}

	if (sumry.num_warnings > 0)
	{
		summary.num_warnings += sumry.num_warnings;
		m_scanner.ShowConsoleValue(sclb_batch_num_warns, summary.num_warnings);
	}

	UpdateBatchDuration(FALSE);
}

void TParsingDriver::ShowAbortingMessage()
{
	if (m_aborting_displayed == FALSE)
	{
		m_scanner.Console().SetMajorStatus(L"Aborting ...");
		m_aborting_displayed = TRUE;
	}
}

void TParsingDriver::WaitContinue(const wchar_t *message)
{
	m_scanner.Console().ShowRunTimeError(message, TGenericConsole::CONS_BTN_OK, FALSE);
}


