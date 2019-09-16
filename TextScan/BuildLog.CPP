//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "TextScan/BuildLog.H"

//----------------------------------------------------------------------------
//  =================  TBuildLogCommandLine  =======================
//----------------------------------------------------------------------------

void TBuildLogCommandLine::AddSrcFile(TBuildLogSrcFile &info, TBuildLogInfo &errs_log)
{
	// The src file info does not contain memory resources. Because of that it is not necessary
	// to check if they were passed into the array or not.
	if (files.AppendItem(info) == FALSE)
		errs_log.AddScanningError(blec_oom_srcfile_info, info.fname_area);
}

void TBuildLogCommandLine::SetSrcFileState(int file_inx, bool new_stt)
{
	assert(file_inx >= 0 && file_inx < files.NumItems());

	if (new_stt != files[file_inx].bProcess)
	{
		files[file_inx].bProcess = new_stt;
		if (proc_mode == sbtm_marked)
		{
			if (new_stt == TRUE)
				chld_active++;
			else chld_active--;
		}
	}
}

//--------------------------------------------------------------------------
//  =================  TBuildLogSubdirInfo  ========================
//--------------------------------------------------------------------------

void TBuildLogSubdirInfo::ReleaseData()
{
	// Release resources of the array members.
	for (int i=0; i<cmdls.NumItems(); ++i)
		cmdls[i].ReleaseData();

	// Now release the array itself.
	cmdls.ReleaseData();
}

void TBuildLogSubdirInfo::AddCmdl(TBuildLogCommandLine &info, TBuildLogInfo &errs_log)
{
	if (cmdls.AppendItem(info) == TRUE)
	{
		// Resources that were owned by the param are now owned by the array.
		info.Clear();
	}
	else
	{
		// Resources that are owned by the param are still in the parameter.
		info.ReleaseData();
		errs_log.AddScanningError(blec_oom_cmdline_info, info.cmdl_area);
		errs_log.mem_error = TRUE;
	}
}

void TBuildLogSubdirInfo::SetCommandLineState(int cmdl_inx, TBuildLogSubtrProcMode new_mode)
{
}

void TBuildLogSubdirInfo::SetSrcFileState(int cmdl_inx, int file_inx, bool new_stt)
{
	assert(cmdl_inx >= 0 && cmdl_inx < cmdls.NumItems());

	if (proc_mode == sbtm_marked)
		chld_active -= cmdls[cmdl_inx].chld_active;

	cmdls[cmdl_inx].SetSrcFileState(file_inx, new_stt);

	if (proc_mode == sbtm_marked)
		chld_active += cmdls[cmdl_inx].chld_active;
}

int __cdecl TBuildLogSubdirInfo::CompareInfos(const TBuildLogSubdirInfo *p1, const TBuildLogSubdirInfo *p2)
{
	// Compare directory names using the length of a shorter dir name.
	int min_len = __min(p1->subdir_area.area_len, p2->subdir_area.area_len);
	wchar_t *buff = p1->subdir_area.file_info->info.file_body;
	int r1 = wcsncmp(buff+p1->subdir_area.area_beg, buff+p2->subdir_area.area_beg, min_len);
	if (r1 != 0)
		return(r1);

	// One directory name is a prefix of another directory name. Push directory with a shorter name up.
	return(p1->subdir_area.area_len-p2->subdir_area.area_len);
}

//----------------------------------------------------------------------------
//  ================== TBuildLogScanError  =========================
//----------------------------------------------------------------------------

wchar_t *TBuildLogScanError::SprintfError(wchar_t *buff, long buff_len)
{
	assert(buff_len >= 80);
	assert(area.file_info->CheckLinesInfo() == TRUE);

	long line_num = area.file_info->GetLineNumber(area.area_beg);
	long char_pos = area.file_info->GetCharPos(area.area_beg);
	swprintf(buff, buff_len, L"Error %d: line %d, char %d, area_len %d. %s.",
			err_code, line_num, char_pos, area.area_len, TBuildLogParser::GetErrorText(err_code));

	return(buff);
}

//----------------------------------------------------------------------------
//  =====================  TBuildLogInfo  =========================
//----------------------------------------------------------------------------

void TBuildLogInfo::ReleaseData()
{
	// Release the body of the logfile.
	file_info.ReleaseBasicFileInfo();
	file_info.ReleaseLinesInfo();
	file_info.Clear();

	// Release the derived data structures.
	for (int i=0; i<subdirs.NumItems(); ++i)
		subdirs[i].ReleaseData();

	subdirs.ReleaseData();
	errors.ReleaseData();
	mem_error = FALSE;
}

int TBuildLogInfo::NumCmdlsTotal()
{
	// Simply loop over the subdirectories.
	int num_cmdls = 0;
	for (int i1=0; i1<subdirs.NumItems(); ++i1)
	{
		num_cmdls += subdirs[i1].cmdls.NumItems();
	}

	return(num_cmdls);
}

int TBuildLogInfo::NumFilesTotal()
{
	// Loop over subdirectories and command lines.
	int num_files = 0;
	for (int i1=0; i1<subdirs.NumItems(); ++i1)
	{
		for (int i2=0; i2<subdirs[i1].cmdls.NumItems(); ++i2)
		{
			num_files += subdirs[i1].cmdls[i2].files.NumItems();
		}
	}

	return(num_files);
}

void TBuildLogInfo::SetBuildLogState(TBuildLogSubtrProcMode new_mode)
{
}

void TBuildLogInfo::SetSubdirInfoState(int subdir_inx, TBuildLogSubtrProcMode new_mode)
{
}

void TBuildLogInfo::SetCommandLineState(int subdir_inx, int cmdl_inx, TBuildLogSubtrProcMode new_mode)
{
}

void TBuildLogInfo::SetSourceFileState(int subdir_inx, int cmdl_inx, int file_inx, bool new_stt)
{
	assert(subdir_inx >= 0 && subdir_inx < subdirs.NumItems());

	if (proc_mode == sbtm_marked)
		chld_active -= subdirs[subdir_inx].chld_active;

	subdirs[subdir_inx].SetSrcFileState(cmdl_inx, file_inx, new_stt);

	if (proc_mode == sbtm_marked)
		chld_active += subdirs[subdir_inx].chld_active;
}

void TBuildLogInfo::AddSubdirInfo(TBuildLogSubdirInfo &info)
{
	if (subdirs.AppendItem(info) == TRUE)
	{
		// Resources that were owned by the param are now owned by the array.
		info.Clear();
	}
	else
	{
		// Resources that are owned by the param are still in the parameter.
		info.ReleaseData();
		AddScanningError(blec_oom_subdir_info, info.subdir_area);
		mem_error = TRUE;
	}
}

void TBuildLogInfo::AddScanningError(int err, TSourceArea &area)
{
	TBuildLogScanError err_info = { err, area };
	if (errors.AppendItem(err_info) == FALSE)
	{
		// Appending the error info failed. It is unlikely that adding new error about that will succeeed.
		// Just set the flag.
		mem_error = TRUE;
	}
}

//----------------------------------------------------------------------------
//  ======================  TBuildLogParser  =======================
//----------------------------------------------------------------------------

TBuildLogParser::TBuildLogParser(TGenericConsole *pConsole, TParseEnvInfo **known_envs)
{
	m_cons = pConsole;
	m_envs = known_envs;
	m_info = NULL;
}

TBuildLogParser::~TBuildLogParser()
{
	m_stt.DeleteAll();
}

void TBuildLogParser::Parse(TBuildLogInfo &info_buff, const wchar_t *build_log_fname, const wchar_t **apps_filter, const wchar_t *subdirs_filter)
{
	long subdirs_filter_len = (subdirs_filter != NULL) ? (long)wcslen(subdirs_filter) : 0;

	// Clear the prev contents of the log file info if any.
	m_info = &info_buff;
	m_info->ReleaseData();
	TFileInfo &fi = m_info->file_info;

	// Clear the prev dir names if any.
	m_stt.DeleteAll();

	// Step 1. Load the log file into memory.
	TLoadSaveResult res = TFileBodyHelper::LoadAsciiFileBody(fi.info, build_log_fname);
	if (res != ldres_success)
	{
		wchar_t message_buff[160+2*MAX_PATH];
		swprintf(message_buff, 160+2*MAX_PATH, L"Error loading the build log: %s\r\n\r\nReason: %s.             ",
				build_log_fname, TFileBodyHelper::GetLoadSaveResultText(res));

		m_cons->ShowRunTimeError(message_buff);
		if (res == ldres_out_of_memory)
			m_info->mem_error = TRUE;
		return;
	}

	// Create lines info for this file.
	if (fi.BuildLinesInfo() == FALSE)
	{
		m_cons->ShowRunTimeError(L"Error building the lines info for the build log.");
		m_info->mem_error = TRUE;
		return;
	}

	// The file was successfully loaded.
	long num_lines = fi.NumLines();
	assert(num_lines > 0);

	// Step 2. Loop over the lines of the file.
	TSimpleLineInfo *li = fi.GetLineInfoFromFileOffset(0);
	for (; num_lines > 0; li++, num_lines--)
	{
		if (li->line_len == 0)
			continue;

		// The current line is not empty.
		wchar_t *ptr = fi.info.file_body+li->line_offs;
		long len = li->line_len;

		// Check if it has a CPU id prefix.
		int cpu_num = 0;
		while (len > 0 && ptr[0] >= L'0' && ptr[0] <= L'9')
		{
			cpu_num = cpu_num*10 + ptr[0] - L'0';
			ptr++;
			len--;
		}

		if (cpu_num == 0 || len == 0 || ptr[0] != L'>')
		{
			// There is no CPU id prefix.
			continue;
		}

		// Consume the '>' character.
		ptr++;
		len--;

		// Look for or create descriptor for the curr cpu_num.
		TBuildLogSubdirInfoItem lookup_item(cpu_num);
		TBuildLogSubdirInfoItem *cpu_stt = m_stt.FindRecord(&lookup_item);
		if (cpu_stt == NULL)
		{
			// This cpu id was not used before.
			cpu_stt = new TBuildLogSubdirInfoItem(cpu_num);
			if (cpu_stt == NULL)
			{
				// No chance. Ignore this line.
				TSourceArea err_area = { &fi, li->line_offs, li->line_len-len };
				m_info->AddScanningError(blec_oom_cpu_num_info, err_area);
				continue;
			}

			m_stt.AddRecord(cpu_stt);
		}

		// Prefill the area that can be used as a command line.
		TSourceArea cmdl_area = { &fi, li->line_offs+li->line_len-len, len };
		wchar_t *word_beg;
		long word_len;
		if (PickNextWord(ptr, len, word_beg, word_len) == FALSE)
			continue;

		// Check for well known messages.
		if (wcsncmp(word_beg, L"BUILDMSG:", word_len) == 0)
		{
			if (PickNextWord(ptr, len, word_beg, word_len) == TRUE && wcsncmp(word_beg, L"Processing", word_len) == 0)
			{
				// This is start of the new directory processing.
				wchar_t *dn_beg; long dn_len;
				if (PickNextWord(ptr, len, dn_beg, dn_len) == TRUE)
				{
					if (cpu_stt->m_ignore_dir == FALSE && cpu_stt->m_info.subdir_area.file_info != NULL && cpu_stt->m_info.cmdls.NumItems() > 0)
					{
						// Save info about the prev directory into the log.
						m_info->AddSubdirInfo(cpu_stt->m_info);
					}

					cpu_stt->m_ignore_dir = FALSE;
					if (subdirs_filter != NULL)
					{
						// Check if the current subdir belongs to the passed tree root.
						if (dn_len < subdirs_filter_len || wcsncmp(dn_beg, subdirs_filter, subdirs_filter_len) != 0)
						{
							// The subdir has not passed the filter.
							cpu_stt->m_ignore_dir = TRUE;
						}
					}

					// Fill in the slot.
					cpu_stt->m_info.subdir_area.file_info = &fi;
					cpu_stt->m_info.subdir_area.area_beg = (long)(dn_beg-fi.info.file_body);
					cpu_stt->m_info.subdir_area.area_len = dn_len;
					cpu_stt->m_info.ReleaseData();
				}
				else
				{
					// The directory name is missing.
					AddScanningError(blec_no_dir_name, word_beg, word_len);
				}
			}
		}

		// Check for known application names.
		TParseEnvInfo *penv = NULL;
		if (m_envs == NULL)
		{
			// Use the std list of known apps.
			penv = TParseEnvInfo::FindParseEnvForAppName(word_beg, word_len);
		}
		else
		{
			// Use list of apps that was provided by the caller.
			TParseEnvInfo **ppenv = m_envs;
			while (*ppenv != NULL)
			{
				if (wcsncmp(word_beg, (*ppenv)->app_name, word_len) == 0)
				{
					// The env for the current app is found.
					penv = *ppenv;
					break;
				}
			}
		}

		if (penv != NULL && apps_filter != NULL)
		{
			// Caller provided additional list of app names.
			wchar_t **ppapp = (wchar_t**)apps_filter;
			while (*ppapp != NULL)
			{
				if (wcsncmp(word_beg, *ppapp, word_len) == 0)
					break;

				ppapp++;
			}

			if (*ppapp == NULL)
			{
				// Name of the app from the command line is not present in the list of apps that was provided by the caller.
				penv = NULL;
			}
		}

		// Check the results.
		if (penv == NULL || cpu_stt->m_ignore_dir == TRUE)
		{
			// Proceed to the next line.
			continue;
		}

		// Ensure that the name of the current directory is already set.
		if (cpu_stt->m_info.subdir_area.file_info == NULL)
		{
			m_info->AddScanningError(blec_dir_name_not_set, cmdl_area);
			continue;
		}

		// Instantiate ther command line descriptor to collect info about the names of the files.
		TBuildLogCommandLine cmdl;
		cmdl.parse_env = penv;
		cmdl.cmdl_area = cmdl_area;

		// Instantiate the file descriptor with a body that will start right after the name of the application and
		// last till the end of the command line.
		TFileInfo cmdlf;
		cmdlf.Clear();
		cmdlf.info.file_name = L"cmdl";
		cmdlf.info.file_body = fi.info.file_body+cmdl_area.area_beg+word_len;
		cmdlf.info.file_len = cmdl_area.area_len-word_len;

		// Look for the file names in the command line.
		TCmdlRawScanner cmdl_raw_parser(penv->cmdl_ks_chars, penv->cmdl_keys_dict, &cmdlf);
		TCmdlLexema lex;
		while (cmdl_raw_parser.GetLex(lex) != cmdl_eof)
		{
			if (lex.type == cmdl_pos_param)
			{
				// The file name is discovered.
				TBuildLogSrcFile srcf_info;
				srcf_info.bProcess = TRUE;
				srcf_info.fname_area.file_info = &fi;
				srcf_info.fname_area.area_beg = lex.value_origin.src_area.area_beg + cmdl_area.area_beg;
				srcf_info.fname_area.area_len = lex.value_origin.src_area.area_len;
				cmdl.AddSrcFile(srcf_info, *m_info);
			}
			else if (lex.type == cmdl_error)
			{
				// Low level scanning error.
				lex.value_origin.src_area.file_info = &fi;
				lex.value_origin.src_area.area_beg += cmdl_area.area_beg;
				m_info->AddScanningError(blec_cmdl_raw_error, lex.value_origin.src_area);
			}
		}

#ifdef ALLOW_CMDLS_WITHOUT_FILES
		//
		// Allow command lines without files. One of the cases when such cmd line is used is creation
		// of the precompiled header.
		//
#else
		if (cmdl.files.NumItems() == 0)
		{
			// There are no file names on the command line.
			m_info->AddScanningError(blec_cmdl_no_files, cmdl_area);
			continue;
		}
#endif

		// Add command line to the current directory info.
		cpu_stt->m_info.AddCmdl(cmdl, *m_info);
	}

	// Step 3. The build log is processed. Flush the m_stt hash table.
	for (m_stt.StartIteration(); m_stt.CheckIteration(); m_stt.StepIteration())
	{
		TBuildLogSubdirInfoItem *item = m_stt.CurrIteratedItem();
		if (item->m_ignore_dir == FALSE && item->m_info.subdir_area.file_info != NULL && item->m_info.cmdls.NumItems() > 0)
		{
			// Current CPU has non empty data.
			m_info->AddSubdirInfo(item->m_info);
		}
	}

	// Step 4. Sort the subdirectory names in the log.
	m_info->subdirs.QuickSort(TBuildLogSubdirInfo::CompareInfos);
}

wchar_t *TBuildLogParser::GetErrorText(int err_no)
{
	switch (err_no)
	{
		case blec_oom_srcfile_info:		return(L"Erorr adding source file info");
		case blec_oom_cmdline_info:	return(L"Erorr adding command line info");
		case blec_oom_subdir_info:		return(L"Erorr adding subdirectory info");
		case blec_oom_cpu_num_info:	return(L"Out of memory while creating the CPU info");
		case blec_no_dir_name:		return(L"The directory name is missing");
		case blec_dir_name_not_set:	return(L"The directory name is missing before (above) the command line");
		case blec_cmdl_raw_error:		return(L"Low level command line scanning error");
		case blec_cmdl_no_files:		return(L"Command line does not contain the source files");
	}

	return(L"Bogus error code");
}

bool TBuildLogParser::PickNextWord(wchar_t *&line_ptr, long &line_len, wchar_t *&word_beg, long &word_len)
{
	// Skip the spaces if any.
	while (line_len > 0)
	{
		if (IsBuildLogSpace(line_ptr[0]) == FALSE)
			break;

		line_ptr++;
		line_len--;
	}

	// Use the current place as the beginning of the word.
	word_beg = line_ptr;
	word_len = 0;

	// Look for the end of the word.
	while (line_len > 0)
	{
		if (IsBuildLogSpace(line_ptr[0]) == TRUE)
			break;

		line_ptr++;
		line_len--;
		word_len++;
	}

	// If the word is not empty, then return TRUE.
	return(word_len > 0);
}

void TBuildLogParser::AddScanningError(int err_no, wchar_t *area_beg, long area_len)
{
	wchar_t *buff = m_info->file_info.info.file_body;
	assert(area_beg >= buff && area_beg+area_len < buff+m_info->file_info.info.file_len);

	TSourceArea area = { &(m_info->file_info), (long)(area_beg-buff), area_len };
	m_info->AddScanningError(err_no, area);
}


