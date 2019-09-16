//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/FormatNumber.H"
#include  "Common/DateTimeHelper.H"
#include  "TextScan/GrammarAnalysisDriver.H"
#include  "Xml/XmlReader.H"
#include  "Xml/XmlWriter.H"

//--------------------------------------------------------------------------------------------
//  ======================  TGrammarConflictAnalysisDriver  =========================
//--------------------------------------------------------------------------------------------

wchar_t	TGrammarConflictAnalysisDriver::g_AnalysisDatabaseFileName[] = L"Analysis_Database.XML";

static GUID g_AnalysisDriverNullGuid = { 0 };

TGrammarConflictAnalysisDriver::TGrammarConflictAnalysisDriver(TGenericConsoleEx &console, DWORD tracing_mask, TGrammar *root_grammar)
		: TBasicGrammarAnalysisHelper(console, tracing_mask)
{
	HardAssert(root_grammar != NULL, L"The grammar to process is missing.");

	m_database_guid = g_AnalysisDriverNullGuid;
	m_save_grammar_tables = FALSE;
	m_grammar = root_grammar;
}

void TGrammarConflictAnalysisDriver::SetAnalysisDirectory(const wchar_t *analysis_database_directory)
{
	HardAssert(analysis_database_directory != NULL && analysis_database_directory[0] != 0, L"The analysis database directory is bogus.");
	m_database_directory.ClearBuffer();

	wchar_t full_fname_buff[2*MAX_PATH];
	DWORD res1 = ::GetFullPathNameW(analysis_database_directory, 2*MAX_PATH, full_fname_buff, NULL);
	HardAssert(res1 > 0, L"The database path is too long (1).");

	bool res2 = m_database_directory.Append(full_fname_buff);
	HardAssert(res2 == TRUE, L"The database path is too long (2).");
}

bool TGrammarConflictAnalysisDriver::DeleteAnalysisFiles()
{
	if (TPathHelper::IsPathADir(m_database_directory) == FALSE)
	{
		// There is no directory. This means that there are no files to delete.
		TraceMsgWithDateAttr(DHM, L"DeleteAnalysisFiles: The database directory is not exisintg.");
		return(TRUE);
	}

	// Prepare buffer with the dir name and with the back slash at the end.
	TFileNameBuffer fname_buff;
	bool res1 = fname_buff.Append(m_database_directory);
	HardAssert(res1 == TRUE, L"DeleteAnalysisFiles (1).");
	bool res2 = TPathHelper::EnsureTrailingSlash(fname_buff);
	HardAssert(res2 == TRUE, L"DeleteAnalysisFiles (2).");
	int dir_len = fname_buff.NumItems();

	// Iterate objects in the subdirectory.
	bool overall_result = TRUE;
	TDirectoryIterator iter;
	for (iter.StartIteration(m_database_directory, FALSE, TRUE); iter; ++iter)
	{
		// Check if this file should be deleted or not.
		bool delete_file = FALSE;
		if (wcscmp(iter.ShortFileName(), g_AnalysisDatabaseFileName) == 0)
		{
			delete_file = TRUE;
		}
		else if (wcsstr(iter.ShortFileName(), L"SavedActionAnalysisState") != NULL)
		{
			delete_file = TRUE;
		}
		else if (wcsstr(iter.ShortFileName(), L"gres_") != NULL)
		{
			delete_file = TRUE;
		}
		else if (wcsstr(iter.ShortFileName(), L"_conflicts_analysis") != NULL)
		{
			delete_file = TRUE;
		}

		// Do the job.
		if (delete_file == TRUE)
		{
			fname_buff.TruncateTo(dir_len);
			bool res3 = fname_buff.Append(iter.ShortFileName());
			HardAssert(res3 == TRUE, L"DeleteAnalysisFiles (3).");

			BOOL res = ::DeleteFileW(fname_buff);
			if (res == 0)
			{
				DWORD win32_err = ::GetLastError();
				TraceMsgWithDateAttr(YellowAttr, L"*********** Error deleting the \"%s\". Win32 error: %d.", fname_buff.DataPtr(), win32_err);
				overall_result = FALSE;
			}
		}
	}

	if (iter.IterationResuilt() != ERROR_SUCCESS)
	{
		TraceMsgWithDateAttr(YellowAttr, L"*********** Error iterating files in the analysis directory. Win32 error: %d.", iter.IterationResuilt());
		overall_result = FALSE;
	}

	return(overall_result);
}

void TGrammarConflictAnalysisDriver::DoConflictsProcessing(int cnt_tasks)
{
	// Startup tracing.
	TDateTime dt_start = CurrDateTime();
	TraceMsgWithDateAttr(DHM, L"GrammarConflictsAnalysisDriver: Startup, PID=%lu.", ::GetCurrentProcessId());
	TraceMsgWithDateAttr(DHM, L"=============================================================================");
	TraceMsgWithDateAttr(DHM, L"DatabaseSummary: NumGrammars=%d, CntConflictingActions=%d.", m_grammar->GetNumGrammars(), m_grammar->GetNumConflictingActionsHier());
	TraceMsgWithDateAttr(DHM, L"DatabaseDirectory: \"%s\".", m_database_directory.DataPtr());

	// Do the job.
	if (DoConflictsProcessingInternal(cnt_tasks) == FALSE)
	{
		TraceMsgWithDateAttr(RedAttr, L"************************************************************************");
		TraceMsgWithDateAttr(RedAttr, L"*");
		TraceMsgWithDateAttr(RedAttr, L"*           Processing failed.");
		TraceMsgWithDateAttr(RedAttr, L"*");
		TraceMsgWithDateAttr(RedAttr, L"************************************************************************");
	}

	// Show the duration.
	wchar_t buffer[80];
	TraceMsgWithDateAttr(DHM, L"*");
	TraceMsgWithDateAttr(DHM, L"* Worker process processing time: %s.", FormatDuration(CurrDateTime()-dt_start, buffer, 80, TRUE));
	TraceMsgWithDateAttr(DHM, L"*");

	// Close console.
	m_console.CloseConsole();
}

bool TGrammarConflictAnalysisDriver::DoConflictsProcessingInternal(int cnt_tasks)
{
	// Ceck if directory for the database file is existing or not.
	if (TPathHelper::IsPathADir(m_database_directory) == FALSE)
	{
		TraceMsgWithDateAttr(YellowAttr, L"****** THE DATABASE DIRECTORY IS NOT EXISTING: \"%s\".", m_database_directory.DataPtr());
		return(FALSE);
	}

	if (cnt_tasks <= 0)
		cnt_tasks = 0x7FFFFFFF;

	// Allocate the helper outside of the loop to reuse the iteration indexes.
	TGrammarConflictAnalysisHelper analysis_helper(m_console, m_tracing_mask);
	for (int itask=0; itask<cnt_tasks; ++itask)
	{
		if (m_console.GetAbortFlag() == TRUE)
		{
			TraceMsgWithDateAttr(YellowAttr, L"*********** Abort from console detected ***********");
			return(FALSE);
		}

		// Load database without preserving any action in the current grammar.
		int igrammar = -1, iconflict = -1, iaction = -1;
		HANDLE hFile = LoadDatabase(igrammar, iconflict, iaction);
		if (hFile == NULL)
		{
			// The loading has failed. An explanation is already reported.
			return(FALSE);
		}

		if (itask == 0)
		{
			// Open console during the first rotation of the loop AFTER loading the database.
			m_console.OpenConsole();
		}

		// The database was loaded and locked. Look for the action to process.
		__int64 num_steps_in_slct_action;
		TAnalysisDatabaseAndTaskStatus summary;
		TSelectActionRes sel_res = FindBestConflictingAction(igrammar, iconflict, iaction, num_steps_in_slct_action, summary);

		// Show overview of the current database state.
		wchar_t buffer[80];
		TraceMsgWithDateAttr(DHM, L"=======================================================================================");
		TraceMsgWithDateAttr(DHM, L"Summary: CntActions=%d, CntRunning=%d, CntInProblem=%d, CntTooDeepTruncs=%s.",
							m_grammar->GetNumConflictingActionsHier(), summary.m_cnt_running, summary.m_cnt_in_problem, FormatInt64(summary.m_cnt_too_deep_truncs, buffer, 80, fnms_dec_signed, L'_'));

		if (sel_res != sar_error_selecting && sel_res != sar_error_marking)
		{
			wchar_t buff40[40];
			m_console.SetConsoleLabel(TBasicGrammarAnalysisHelper::cons_label_num_scheduled_actions, _itow(summary.m_num_scheduled_actions, buff40, 10));
			if (summary.m_num_started_actions < summary.m_num_scheduled_actions)
			{
				m_console.SetConsoleLabel(TBasicGrammarAnalysisHelper::cons_label_num_started_actions, _itow(summary.m_num_started_actions, buff40, 10));
			}
			else if (summary.m_num_started_no_paths > 0)
			{
				m_console.SetConsoleLabel(TBasicGrammarAnalysisHelper::cons_label_num_no_paths_actions, _itow(summary.m_num_started_no_paths, buff40, 10));
			}
			else
			{
				// All actions are started and all actions have discovered paths.
				m_console.SetConsoleLabel(TBasicGrammarAnalysisHelper::cons_label_num_remaining_actions, _itow(summary.m_num_started_actions-summary.m_num_complete_actions, buff40, 10));
			}
		}

		// Check the results of picking up the action.
		if (sel_res == sar_no_action)
		{
			if (summary.m_cnt_too_deep_truncs > 0)
			{
				TraceMsgWithDateAttr(YellowAttr, L"*");
				TraceMsgWithDateAttr(YellowAttr, L"*********** Too deep path truncations: %s.", FormatInt64(summary.m_cnt_too_deep_truncs, buffer, 80, fnms_dec_signed, L'_'));
			}

			if (summary.m_cnt_partial_iter_aborts > 0)
			{
				TraceMsgWithDateAttr(YellowAttr, L"*");
				TraceMsgWithDateAttr(YellowAttr, L"*********** Partial iteration aborts: %s.", FormatInt64(summary.m_cnt_partial_iter_aborts, buffer, 80, fnms_dec_signed, L'_'));
			}

			if (summary.m_cnt_in_problem == 0 && summary.m_cnt_running == 0)
			{
				TraceMsgWithDateAttr(YellowAttr, L"*");
				TraceMsgWithDateAttr(YellowAttr, L"*********** Processing finished with SUCCESS ***********");
				TraceMsgWithDateAttr(YellowAttr, L"*");
			}
			else
			{
				TraceMsgWithDateAttr(DHM, L"*");

				if (summary.m_cnt_in_problem > 0)
					TraceMsgWithDateAttr(YellowAttr, L"*********** Processing for %d action%s failed.", summary.m_cnt_in_problem, PluralSuffix(summary.m_cnt_in_problem));
				if (summary.m_cnt_running > 0)
					TraceMsgWithDateAttr(YellowAttr, L"*********** %d action%s %s still running.", summary.m_cnt_running, PluralSuffix(summary.m_cnt_running), (wcslen(PluralSuffix(summary.m_cnt_running)) <= 0) ? L"is" : L"are");

				TraceMsgWithDateAttr(DHM, L"*");
				TraceMsgWithDateAttr(DHM, L"*********** There are no conflicting actions to process. Exiting.");
				TraceMsgWithDateAttr(DHM, L"*");
			}

			::CloseHandle(hFile);
			return(summary.m_cnt_in_problem == 0);
		}
		else if (sel_res == sar_error_selecting)
		{
			TraceMsgWithDateAttr(YellowAttr, L"*********** Error selecting the best conflicting action in the grammar to process.");
			::CloseHandle(hFile);
			return(FALSE);
		}
		else if (sel_res == sar_error_marking)
		{
			TraceMsgWithDateAttr(YellowAttr, L"*********** Attempt to mark the selected action in the grammar as running has failed.");
			::CloseHandle(hFile);
			return(FALSE);
		}

		// Some action to process was selected and this action was marked as busy in the grammar structure.
		assert(sel_res == sar_start_as_new || sel_res == sar_continue);

		TraceMsgWithDateAttr(ActionSelectionAttr, L"------------------------------------------------------------------------------");
		TraceMsgWithDateAttr(ActionSelectionAttr, L"ActionSelection: iGrammar=%d, iConflict=%d, iAction=%d, CntSteps=%s.", igrammar, iconflict, iaction, FormatInt64(num_steps_in_slct_action, buffer, 80, fnms_dec_signed, L'_'));
		TraceMsgWithDateAttr(ActionSelectionAttr, L"------------------------------------------------------------------------------");

		// Reset the helper and set the props that were picked up from the database into the helper.
		// Note that on each rotation of the loop the props might change. Although this is not recommended.
		analysis_helper.ResetMajorFields();
		analysis_helper.SetProps(m_analysis_props);

		// Check, if loading a saved state is needed.
		if (sel_res == sar_continue)
		{
			// Pick up the saved analysis state. The database thinks that it should be there.
			if (LoadSavedAnalysisState(analysis_helper, igrammar, iconflict, iaction, num_steps_in_slct_action) == FALSE)
			{
				// The loading failed. An explanation is already reported. Remove the "running" mark from the grammar in memory.
				bool res1 = MarkConflictingAction(igrammar, iconflict, iaction, TConflictingActionAnalysisResults::gres_timeout);
				HardAssert(res1 == TRUE, L"Grammar structure is corrupted (1).");
				::CloseHandle(hFile);
				return(FALSE);
			}
		}

		// Save and release the database. This method will store the "action busy" flag in the file and close the file handle.
		if (SaveDatabase(hFile) == FALSE)
		{
			// An explanation is already reported.
			return(FALSE);
		}

		// Do the long lasting calculation.
		if (sel_res == sar_start_as_new)
		{
			wchar_t buff40[40];
			m_console.SetConsoleLabel(TBasicGrammarAnalysisHelper::cons_label_num_started_actions, _itow(summary.m_num_started_actions+1, buff40, 10));
			analysis_helper.ProcessConflicts(m_grammar, igrammar, iconflict, iaction);
		}
		else
		{
			analysis_helper.ContinueCurrentProcessing();
		}

		// Load the database again to store the results of the latest calculation step. Instruct the loader to preserve
		// the results of analysis that are stored in the current conflicting action.
		hFile = LoadDatabase(igrammar, iconflict, iaction);
		if (hFile == NULL)
		{
			// The loading has failed. An explanation is already reported.
			return(FALSE);
		}

		// Save the calculation results. This will simply overwrite the old file.
		if (SaveCurrentAnalysisState(analysis_helper) == FALSE)
		{
			// An explanation is already reported. Exit without unlocking the current action.
			return(FALSE);
		}

		if (m_save_grammar_tables == TRUE)
		{
			// Store the results of calculation as a C++ source file.
			if (StoreCurrentGrammar() == FALSE)
			{
				// An explanation is already reported.
				return(FALSE);
			}
		}

		// Store the database. This will also reset the  the "action busy" flag in the file.
		if (SaveDatabase(hFile) == FALSE)
		{
			// An explanation is already reported.
			return(FALSE);
		}
	}

	// Final tracing.
	TraceMsgWithDateAttr(YellowAttr, L"*********** WORK COMPLETE. %d requested step%s %s executed. ***********", cnt_tasks, PluralSuffix(cnt_tasks), (cnt_tasks <= 1) ? L"was" :  L"were");
	return(TRUE);
}

void TGrammarConflictAnalysisDriver::PrepareDatabaseFileName(TFileNameBuffer &fname_buff)
{
	fname_buff.ClearBuffer();
	fname_buff.Append(m_database_directory);
	fname_buff.Append(L"\\");
	fname_buff.Append(g_AnalysisDatabaseFileName);
	HardAssert(fname_buff.GetXpndError() == FALSE, L"The database path is too long (3).");
}

void TGrammarConflictAnalysisDriver::PrepareSavedAnalysisStateFileName(TFileNameBuffer &fname_buff, int igrammar, int iconflict, int iaction,
																	TConflictingActionAnalysisResults::TGenResult gres, TFileNameBuffer &fname_buff_alt)
{
	fname_buff.ClearBuffer();
	fname_buff_alt.ClearBuffer();

	bool res1 = fname_buff.Append(m_database_directory);
	HardAssert(res1 == TRUE, L"The database path is too long (5).");

	wchar_t short_fname1[80], short_fname2[80];
	swprintf(short_fname1, 80, L"\\SavedActionAnalysisState_%02d_%04d_%d.xml", igrammar, iconflict, iaction);
	swprintf(short_fname2, 80, L"\\%s_%02d_%04d_%d.xml", TConflictingActionAnalysisResults::GetGenResultEnumName(gres), igrammar, iconflict, iaction);

	if (gres >= TConflictingActionAnalysisResults::gres_not_started && gres <= TConflictingActionAnalysisResults::gres_timeout)
	{
		// Prepare file name for storing the intermediate processing state.
		bool res2 = fname_buff.Append(short_fname1);
		HardAssert(res2 == TRUE, L"The SavedAnalysisState path is too long (6).");
	}
	else
	{
		// Prepare file name for storing the final result of processing.
		bool res3 = fname_buff.Append(short_fname2);
		HardAssert(res3 == TRUE, L"The SavedAnalysisState path is too long (7).");

		// Prepare an alternative file name to simplify deleting the old file.
		fname_buff_alt.Append(m_database_directory);
		fname_buff_alt.Append(short_fname1);
		HardAssert(fname_buff_alt.GetXpndError() == FALSE, L"The SavedAnalysisState path is too long (8).");
	}
}

HANDLE TGrammarConflictAnalysisDriver::LoadDatabase(int igrammar_to_preserve, int iconflict_to_preserve, int iaction_to_preserve)
{
	// When this method is called for the first time, the database guid is NULL.
	bool first_call_flag = memcmp(&m_database_guid, &g_AnalysisDriverNullGuid, sizeof(GUID)) == 0;

	// Prepare the database file name.
	TFileNameBuffer database_fname;
	PrepareDatabaseFileName(database_fname);

	// Check the file.
	if (TPathHelper::IsPathAFile(database_fname) == FALSE)
	{
		//
		// The database file is not existing.
		//

		if (first_call_flag == TRUE)
		{
			// Create a brand new GUID.
			DWORD retGuid = ::UuidCreate(&m_database_guid);
			HardAssert(retGuid == RPC_S_OK, L"Unable to create the database GUID.");

			// Let the derived class the specify the analysis task AND the analysis props.
			if (PrepareAnalysisTaskAndProps(m_analysis_task, m_analysis_props, TRUE) == FALSE)
			{
				TraceMsgWithDateAttr(YellowAttr, L"****** ERROR PREPARING THE ANALYSIS TASK AND PROPS.");
				return(NULL);
			}

			// Create a database file. Later on the file will be loaded and locked using the regular procedure.
			if (SaveDatabase(NULL) == FALSE)
			{
				// An error message is already emitted.
				return(NULL);
			}

			// Creation succeeded.
			first_call_flag = FALSE;
		}
		else
		{
			TraceMsgWithDateAttr(YellowAttr, L"****** ANALYSIS DATABASE DISAPPEARED.");
			return(NULL);
		}
	}

	TDateTime dt_start = CurrDateTime();
	TraceMsgWithDateAttr(DHM, L"Loading analysis database ....");

	// Load the file and lock it at the same time.
	HANDLE hFile = NULL;
	TBasicFileInfo database_data;
	int cnt_attempts = 0;
	for(;;)
	{
		DWORD win32_error;
		TLoadSaveResult lds_res = TFileBodyHelper::LoadAsciiFileBody(database_data, database_fname, NULL, TRUE, FALSE, TRUE, NULL, NULL, &hFile, &win32_error);
		if (lds_res == ldres_success)
		{
			// The file is loaded and locked.
			HardAssert(hFile != NULL && hFile != INVALID_HANDLE_VALUE, L"The database file handle is bogus.");
			break;
		}
		else if (lds_res == ldres_error_opening)
		{
			// It can be that other process is currently trying to open this file.
			if (++cnt_attempts < 200)
			{
				for (int cnt_sleeps=0; cnt_sleeps < ((win32_error == ERROR_SHARING_VIOLATION) ? 20 : 5); ++cnt_sleeps)
				{
					if (m_console.GetAbortFlag() == TRUE)
						break;

					// Sleep for a short time and recheck the console after that.
					::Sleep(100);
					m_console.ProcessWindowsMessages();
				}

				continue;
			}
			else
			{
				TraceMsgWithDateAttr(YellowAttr, L"******  LOADING THE DATABASE FAILED WITH ERROR OPENING THE FILE.");
				TraceMsgWithDateAttr(YellowAttr, L"******  Latest Win32 error: %d%s.", win32_error, (win32_error == ERROR_SHARING_VIOLATION) ? L" (ERROR_SHARING_VIOLATION)" : L"");
				return(NULL);
			}
		}

		// Loading the file has failed.
		TraceMsgWithDateAttr(YellowAttr, L"****** LOADING THE DATABASE FILE FAILED: %s.", TFileBodyHelper::GetLoadSaveResultText(lds_res));
		return(NULL);
	}

	// Allocate an XML stream for scanning the memory buffer.
	TXmlStream xmlSource(database_fname, database_data.file_body, database_data.file_len);
	TGrammarAnalysisDatabaseXmlReader readerHandler(m_database_guid, m_analysis_props, m_grammar, igrammar_to_preserve, iconflict_to_preserve, iaction_to_preserve);

	// Scan the data.
	bool xmlRes = readerHandler.ScanXmlSource(xmlSource, &m_console);
	database_data.FreeBasicFileInfo();

	if (xmlRes == FALSE)
	{
		// The reading has failed. This is a critical error.
		TraceMsgWithDateAttr(YellowAttr, L"****** Scanning the XML data of the main database failed.");
		::CloseHandle(hFile);
		return(NULL);
	}

	// The data in the file is ok.
	wchar_t buffer[80];
	TraceMsgWithDateAttr(DHM, L"Analysis database was successfully loaded. Elapsed time: %s.", FormatDuration(CurrDateTime()-dt_start, buffer, 80, TRUE));

	if (first_call_flag == TRUE)
	{
		// Pick up the processing task. The derived class may show a dialog here.
		if (PrepareAnalysisTaskAndProps(m_analysis_task, m_analysis_props, FALSE) == FALSE)
		{
			TraceMsgWithDateAttr(YellowAttr, L"****** ERROR PREPARING THE ANALYSIS TASK.");
			::CloseHandle(hFile);
			return(NULL);
		}
	}

	// Non NULL file handle indicates the success of loading the database.
	return(hFile);
}

bool TGrammarConflictAnalysisDriver::SaveDatabase(HANDLE hOpenedFile)
{
	TDateTime dt_start = CurrDateTime();
	TraceMsgWithDateAttr(DHM, L"Saving analysis database ....");

	// Prepare the database file name.
	TFileNameBuffer database_fname;
	PrepareDatabaseFileName(database_fname);

	// Prepare the writer.
	m_console.SetupAbortFlagHiding(TRUE);
	TXmlWriter writer(database_fname, &m_console);
	if (writer.PrepareDiskFile(hOpenedFile) == FALSE)
	{
		TraceMsgWithDateAttr(YellowAttr, L"****** UNABLE TO OPEN THE FILE FOR WRITING THE ANALYSIS DATABASE ******");
		m_console.SetupAbortFlagHiding(FALSE);
		return(FALSE);
	}

	// Do the writing.
	TGrammarAnalysisDatabaseXmlWriter::DumpGrammarAnalysisData(writer, 0, m_database_guid, m_analysis_props, *m_grammar);
	m_console.SetupAbortFlagHiding(FALSE);
	writer.WriteLine();
	writer.Close();

	// Check the results.
	if (writer.GetErrorsCount() > 0)
	{
		TraceMsgWithDateAttr(YellowAttr, L"****** WRITING THE DATABASE FAILED BECAUSE OF THE XML PROBLEMS ******");
		return(FALSE);
	}
	else if (writer.GetErrorFlag() == TRUE)
	{
		TraceMsgWithDateAttr(YellowAttr, L"****** WRITING THE DATABASE FAILED BECAUSE OF THE DISK PROBLEMS ******");
		return(FALSE);
	}

	// Success.
	wchar_t buffer[80];
	TraceMsgWithDateAttr(DHM, L"Analysis database was successfully saved. The elapsed time: %s.", FormatDuration(CurrDateTime()-dt_start, buffer, 80, TRUE));
	return(TRUE);
}

bool TGrammarConflictAnalysisDriver::LoadSavedAnalysisState(TGrammarConflictAnalysisHelper &data, int igrammar, int iconflict, int iaction, __int64 num_steps)
{
	// Prepare the file name.
	TFileNameBuffer fname_buff, fname_buff_alt;
	PrepareSavedAnalysisStateFileName(fname_buff, igrammar, iconflict, iaction, TConflictingActionAnalysisResults::gres_not_started, fname_buff_alt);

	// Allocate an XML reader.
	TXmlReader reader(xtags_SavedConflictingActionAnalysisState, L"SavedConflictingActionAnalysisState", &m_console);
	TXmlStream xmlSource(fname_buff);

	// Prepare the XML handler.
	GUID saved_state_database_guid;
	TSavedConflictingActionAnalysisStateXmlReader readerHandler;
	readerHandler.SetDestination(saved_state_database_guid, data, m_grammar, igrammar);

	// Scan the data.
	bool xmlRes = reader.Parse(xmlSource, &readerHandler);
	if (xmlRes == FALSE)
	{
		// The reading has failed. This is a critical error.
		TraceMsgWithDateAttr(YellowAttr, L"****** SCANNING THE SAVED ANALYSIS STATE XML DATA FAILED ******");
		return(FALSE);
	}

	// The scanning went fine. Check for additional high level conditions.
	if (memcmp(&saved_state_database_guid, &m_database_guid, sizeof(GUID)) != 0)
	{
		TraceMsgWithDateAttr(YellowAttr, L"****** The database GUIDs do not match.");
		return(FALSE);
	}
	else if (data.m_iconflict != iconflict)
	{
		TraceMsgWithDateAttr(YellowAttr, L"****** The picked grammar conflict index is bogus: %d.", data.m_iconflict);
		return(FALSE);
	}
	else if (data.m_iaction != iaction)
	{
		TraceMsgWithDateAttr(YellowAttr, L"****** The picked conflicting action index is bogus: %d.", data.m_iaction);
		return(FALSE);
	}
	else if (data.m_cnt_steps != num_steps)
	{
		wchar_t buffer1[80], buffer2[80];
		TraceMsgWithDateAttr(YellowAttr, L"****** The picked number of iteration steps (%s) does not match the expected value (%s).",
							FormatInt64(data.m_cnt_steps, buffer1, 80, fnms_dec_signed, L'_'), FormatInt64(num_steps, buffer2, 80, fnms_dec_signed, L'_'));
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TGrammarConflictAnalysisDriver::SaveCurrentAnalysisState(TGrammarConflictAnalysisHelper &data)
{
	assert(data.m_result != NULL);

	// Prepare the file name.
	TFileNameBuffer fname_buff, fname_buff_alt;
	PrepareSavedAnalysisStateFileName(fname_buff, data.m_igrammar, data.m_iconflict, data.m_iaction, data.m_result->m_generation_result, fname_buff_alt);

	// Prepare the writer.
	m_console.SetupAbortFlagHiding(TRUE);
	TXmlWriter writer(fname_buff, &m_console);
	if (writer.PrepareDiskFile() == FALSE)
	{
		TraceMsgWithDateAttr(YellowAttr, L"****** UNABLE TO OPEN THE FILE FOR WRITING THE ACTION ANALYSIS STATE ******");
		m_console.SetupAbortFlagHiding(FALSE);
		return(FALSE);
	}

	// Do the writing.
	TSavedConflictingActionAnalysisStateXmlWriter::DumpGrammarAnalysisState(writer, 0, m_database_guid, data);
	m_console.SetupAbortFlagHiding(FALSE);
	writer.WriteLine();
	writer.Close();

	// Check the results.
	if (writer.GetErrorsCount() > 0)
	{
		TraceMsgWithDateAttr(YellowAttr, L"****** WRITING ACTION ANALYSIS STATE FAILED BECAUSE OF THE XML PROBLEMS ******");
		return(FALSE);
	}
	else if (writer.GetErrorFlag() == TRUE)
	{
		TraceMsgWithDateAttr(YellowAttr, L"****** WRITING ACTION ANALYSIS STATE FAILED BECAUSE OF THE DISK PROBLEMS ******");
		return(FALSE);
	}

	if (fname_buff_alt.NumItems() > 0 && TPathHelper::IsPathAFile(fname_buff_alt) == TRUE)
	{
		// File with an old intermediate state is existing. Delete this file.
		BOOL res = ::DeleteFileW(fname_buff_alt);
		if (res == 0)
		{
			DWORD win32_err = ::GetLastError();
			TraceMsgWithDateAttr(YellowAttr, L"*********** Error deleting the \"%s\". Win32 error: %d.", fname_buff_alt.DataPtr(), win32_err);
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

bool TGrammarConflictAnalysisDriver::StoreCurrentGrammar()
{
	TDateTime dt_start = CurrDateTime();
	TraceMsgWithDateAttr(DHM, L"Writing grammar tables file ....");

	m_console.SetupAbortFlagHiding(TRUE);
	bool res = m_grammar->EmitTablesEx(&m_console, L"AnalysisDriver:", m_database_directory, L"_conflicts_analysis.cxx", NULL, FALSE);
	m_console.SetupAbortFlagHiding(FALSE);

	wchar_t buffer[80];
	TraceMsgWithDateAttr(DHM, L"Writing time: %s.", FormatDuration(CurrDateTime()-dt_start, buffer, 80, TRUE));
	return(res);
}

TGrammarConflictAnalysisDriver::TSelectActionRes TGrammarConflictAnalysisDriver::FindBestConflictingAction(int &igrammar, int &iconflict, int &iaction, __int64 &num_steps_in_selected_action,
																									TAnalysisDatabaseAndTaskStatus &summary)
{
	// Clear the output variables.
	num_steps_in_selected_action = 0;
	summary.Clear();

	__int64 min_steps = 0;
	int num_paths_at_min_steps = -1;
	int num_grms = m_grammar->GetNumGrammars();
	assert(num_grms > 0);

	if (m_analysis_task.m_only_igrammar >= 0)
	{
		// Iterate only one grammar.
		assert(igrammar < num_grms);
		if (FindBestConflictingActionInGrammar(m_analysis_task.m_only_igrammar, igrammar, iconflict, iaction, min_steps, num_paths_at_min_steps, summary) == FALSE)
			return(sar_error_selecting);
	}
	else
	{
		// Iterate all grammars in the hierarchy.
		for (int igrm=0; igrm<num_grms; ++igrm)
		{
			if (FindBestConflictingActionInGrammar(igrm, igrammar, iconflict, iaction, min_steps, num_paths_at_min_steps, summary) == FALSE)
				return(sar_error_selecting);
		}
	}

	if (min_steps == 0)
	{
		// There are no startable conflicting actions.
		return(sar_no_action);
	}
	else
	{
		// Something was picked up. Mark this conflicting action as "running".
		if (MarkConflictingAction(igrammar, iconflict, iaction, TConflictingActionAnalysisResults::gres_proc_running) == FALSE)
		{
			return(sar_error_marking);
		}

		if (min_steps  < 0)
		{
			// Start processing the selected action.
			return(sar_start_as_new);
		}
		else
		{
			// Continue processing the selected action.
			num_steps_in_selected_action = min_steps;
			return(sar_continue);
		}
	}
}

bool TGrammarConflictAnalysisDriver::FindBestConflictingActionInGrammar(int igrm_prm, int &igrammar, int &iconflict, int &iaction, __int64 &min_steps, int &num_paths_at_min_steps,
																			TAnalysisDatabaseAndTaskStatus &summary)
{
	TGrammar *grm = m_grammar->GetGrammarByIndex(igrm_prm);
	assert(grm != NULL);

	if (m_analysis_task.m_only_iconflict <= -3)
	{
		// Check conflicts that belong to one expected conflict.
		int ixpct = -(3+m_analysis_task.m_only_iconflict);
		assert(ixpct >= 0 && ixpct < grm->NumXpctConflicts());

		int icon = m_grammar->GetFirstGrammarConflictInx(ixpct);
		while (icon >= 0)
		{
			if (FindBestConflictingActionInConflict(grm, igrm_prm, icon, -1, igrammar, iconflict, iaction, min_steps, num_paths_at_min_steps, summary) == FALSE)
				return(FALSE);
			icon = m_grammar->GetNextGrammarConflictInx(ixpct, icon);
		}
	}
	else if (m_analysis_task.m_only_iconflict == -2)
	{
		// Check conflicts that belong to the explicit list of the grammar conflicts.
		for (int icon=0; icon<m_analysis_task.m_cfct_indexes_list.NumItems(); ++icon)
		{
			WORD cfct_inx = m_analysis_task.m_cfct_indexes_list[icon];
			assert(cfct_inx < grm->NumConflicts());

			if (FindBestConflictingActionInConflict(grm, igrm_prm, cfct_inx, -1, igrammar, iconflict, iaction, min_steps, num_paths_at_min_steps, summary) == FALSE)
				return(FALSE);
		}
	}
	else if (m_analysis_task.m_only_iconflict == -1)
	{
		// Check all conflicts in the current grammar if any.
		for (int icon=0; icon<grm->NumConflicts(); ++icon)
		{
			if (FindBestConflictingActionInConflict(grm, igrm_prm, icon, -1, igrammar, iconflict, iaction, min_steps, num_paths_at_min_steps, summary) == FALSE)
				return(FALSE);
		}
	}
	else
	{
		// Check only one grammar conflict.
		assert(m_analysis_task.m_only_iconflict < grm->NumConflicts());
		if (FindBestConflictingActionInConflict(grm, igrm_prm, m_analysis_task.m_only_iconflict, m_analysis_task.m_only_iaction, igrammar, iconflict, iaction, min_steps, num_paths_at_min_steps, summary) == FALSE)
			return(FALSE);
	}

	// All required conflicts were evaluated.
	return(TRUE);
}

bool TGrammarConflictAnalysisDriver::FindBestConflictingActionInConflict(TGrammar *grm, int igrm_prm, int icon_prm, int iact_prm, int &igrammar, int &iconflict, int &iaction,
																		 __int64 &min_steps, int &num_paths_at_min_steps, TAnalysisDatabaseAndTaskStatus &summary)
{
	TGrammarConflict &cfct = grm->conflicts[icon_prm];
	assert(cfct.NumActions() > 0);

	if (iact_prm >= 0)
	{
		// Ensure that all conflicting actions are allocated. Caller wants a specific action.
		if (cfct.analysis_results.NumItems() != cfct.NumActions())
		{
			// Analysis results are not allocated or the data in this grammar conflict is bogus.
			if (cfct.BuildAnalysisResultsStub() == FALSE)
				return(FALSE);
		}

		// Check only one conflicting action.
		assert(iact_prm < cfct.NumActions());
		EvaluateConflictingActionProps(cfct, igrm_prm, icon_prm, iact_prm, igrammar, iconflict, iaction, min_steps, num_paths_at_min_steps, summary);
	}
	else
	{
		// There is no restriction on the action index.
		if (cfct.analysis_results.NumItems() != cfct.NumActions())
		{
			// Analysis results are not allocated or the data in this grammar conflict is bogus.
			if (min_steps >= 0)
			{
				// Pick up the first action of this conflict because none of its actions are started. Selection of this
				// action is final but iteration should be continued to get full report on the busy and failed actions.
				min_steps = -1;
				num_paths_at_min_steps = 0;
				igrammar = igrm_prm;
				iconflict = icon_prm;
				iaction = 0;
			}

			summary.m_num_scheduled_actions += cfct.NumActions();
		}
		else
		{
			// Check all conflicting actions.
			for (int iact=0; iact<cfct.analysis_results.NumItems(); ++iact)
			{
				EvaluateConflictingActionProps(cfct, igrm_prm, icon_prm, iact, igrammar, iconflict, iaction, min_steps, num_paths_at_min_steps, summary);
			}
		}
	}

	// Success.
	return(TRUE);
}

void TGrammarConflictAnalysisDriver::EvaluateConflictingActionProps(TGrammarConflict &cfct, int igrm_prm, int icon_prm, int iact_prm, int &igrammar, int &iconflict, int &iaction,
																		__int64 &min_steps, int &num_paths_at_min_steps, TAnalysisDatabaseAndTaskStatus &summary)
{
	TConflictingActionAnalysisResults::TGenResult gres = cfct.analysis_results[iact_prm].m_generation_result;
	assert(gres < TConflictingActionAnalysisResults::gres_num_types);

	// Update the overall classification of actions.
	summary.m_num_scheduled_actions++;
	if (gres != TConflictingActionAnalysisResults::gres_not_started)
	{
		summary.m_num_started_actions++;
		if (cfct.analysis_results[iact_prm].m_derivation_paths.NumItems() == 0)
			summary.m_num_started_no_paths++;
	}

	if (gres >= TConflictingActionAnalysisResults::gres_ok)
		summary.m_num_complete_actions++;

	if (gres == TConflictingActionAnalysisResults::gres_not_started)
	{
		if (min_steps >= 0)
		{
			// Pick up the current action because its processing is not started. Selection of the action
			// is final but the iteration should be continued to get full report on the busy and failed actions.
			min_steps = -1;
			num_paths_at_min_steps = 0;
			igrammar = igrm_prm;
			iconflict = icon_prm;
			iaction = iact_prm;
		}
	}
	else if (gres == TConflictingActionAnalysisResults::gres_proc_running)
	{
		// This action cannot be selected in any way.
		summary.m_cnt_running++;
	}
	else if (gres == TConflictingActionAnalysisResults::gres_aborted_by_user || gres == TConflictingActionAnalysisResults::gres_timeout)
	{
		if (min_steps >= 0)
		{
			__int64 num_gen_steps = cfct.analysis_results[iact_prm].m_generation_steps;
			if (num_gen_steps <= 0)
			{
				// Processing was aborted at the very beginning. This is the same as if processing was not started at all.
				min_steps = -1;
				num_paths_at_min_steps = 0;
				igrammar = igrm_prm;
				iconflict = icon_prm;
				iaction = iact_prm;
			}
			else
			{
				int num_paths = cfct.analysis_results[iact_prm].m_derivation_paths.NumItems();
				if (num_paths == 0)
				{
					// The current action does not have any discovered paths. If an already selected action has paths,
					// then pick up the current action. Otherwise select action with the lowest number of processing steps.
					if (min_steps == 0 || num_paths_at_min_steps > 0 || num_gen_steps < min_steps)
					{
						// Pick up the current action of the current conflict because it has a better processing score.
						min_steps = num_gen_steps;
						num_paths_at_min_steps = num_paths;
						igrammar = igrm_prm;
						iconflict = icon_prm;
						iaction = iact_prm;
					}
				}
				else
				{
					// The current action has paths. Change the already selected action only if it also has paths and the current
					// action is less developed than the already selected action.
					if (min_steps == 0 || (num_paths_at_min_steps > 0 && num_gen_steps < min_steps))
					{
						// Pick up the current action of the current conflict because it has a better processing score.
						min_steps = num_gen_steps;
						num_paths_at_min_steps = num_paths;
						igrammar = igrm_prm;
						iconflict = icon_prm;
						iaction = iact_prm;
					}
				}
			}
		}
	}
	else if (gres != TConflictingActionAnalysisResults::gres_ok)
	{
		// Processing for this action is terminated with a problem.
		summary.m_cnt_in_problem++;
	}

	summary.m_cnt_too_deep_truncs += cfct.analysis_results[iact_prm].m_too_deep_truncations;
	summary.m_cnt_partial_iter_aborts += cfct.analysis_results[iact_prm].m_partial_iteration_aborts;
}

bool TGrammarConflictAnalysisDriver::MarkConflictingAction(int igrammar, int iconflict, int iaction, TConflictingActionAnalysisResults::TGenResult gres)
{
	// Pick up the right structure.
	TGrammar *grm = m_grammar->GetGrammarByIndex(igrammar);
	TGrammarConflict &conflict = grm->conflicts[iconflict];
	assert(conflict.NumActions() > 0);

	if (conflict.analysis_results.NumItems() != conflict.NumActions())
	{
		if (conflict.BuildAnalysisResultsStub() == FALSE)
		{
			return(FALSE);
		}
	}

	// Put the mark itself.
	conflict.analysis_results[iaction].m_generation_result = gres;
	return(TRUE);
}


