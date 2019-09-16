//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/MiscStuffs.H"
#include  "Common/FormatNumber.H"
#include  "Processing/RawProcessing.H"

// -----------------------------------------------------------------------------
//	==================  TSrcDetailsIterationHandler  ====================
// -----------------------------------------------------------------------------

TSrcDetailsIterationHandler::TSrcDetailsIterationHandler(bool verbose_mode)
{
	m_verbose_mode = verbose_mode;

	m_curr_dir_name[0] = 0;
	m_scanned_files_in_subdir = 0;
	m_subdir_header_printed = FALSE;

	m_curr_file_info.Init(strg_disk, rscn_c_cpp);
	m_sys_generated_file = FALSE;
	m_header_file = FALSE;
	m_resource_file = FALSE;
	m_css_file = FALSE;

	m_cnt_details = 0;

	m_file_header_printed = FALSE;
	m_file_headers_count = 0;

	m_subdirs_with_files = m_scanned_files_count = 0;

	m_total_files_size = 0;
	m_total_lines_count = m_code_lines_count = 0;
	m_cnt_files_with_details = m_cnt_details_in_all_files = 0;
}

void TSrcDetailsIterationHandler::ProcessSubtree(TGenericConsoleEx &cons, const wchar_t *directory_name, bool recurse_subdirs, const wchar_t **list_of_file_name_exts, bool  wait_for_user_resp, TBatchParsingSummary *pSumry)
{
	assert(TPathHelper::IsEmptyPath(directory_name) == FALSE);
	wprintf(L"\r\n Processing: %s\r\n------------------------------------------------------------\r\n", directory_name);
	MiscSupp::DumpListOfExtensions(list_of_file_name_exts);

	// Instantiate the parsing driver with the dummy parser and the dummy mid lev cbk handler.
	TGenericParser dummyParser;
	TMidLevScanCbkHandler midLevHandler(&cons);
	TParsingDriver driver(dummyParser, midLevHandler);

	m_owner_driver = &driver;
	driver.ProcessSubtreeFilesRawData(*this, directory_name, recurse_subdirs, list_of_file_name_exts, wait_for_user_resp, pSumry);
	m_owner_driver = NULL;
}

void TSrcDetailsIterationHandler::ProcessDirectoryName(const wchar_t *subdir_name)
{
	wcscpy(m_curr_dir_name, subdir_name);
	m_scanned_files_in_subdir = 0;
	m_subdir_header_printed = FALSE;
}

void TSrcDetailsIterationHandler::ProcessDataFile(TBasicFileInfo &basic_info)
{
	// Store info about the file.
	m_curr_file_info.ReleaseLinesInfo();
	m_curr_file_info.info = basic_info;
	m_sys_generated_file = FALSE;
	m_header_file = FALSE;
	m_resource_file = FALSE;
	m_css_file = FALSE;
	m_cnt_details = 0;
	m_file_header_printed = FALSE;

	if (TGenericDetailsChecker::IsNoDependenciesMsg(basic_info.file_body, basic_info.file_len) == TRUE)
	{
		// This is a Visual Studio auto generated file. It does not need a disclaimer.
		m_sys_generated_file = TRUE;
	}

	wchar_t *curr_ext = wcsrchr(basic_info.file_name, L'.');
	if (curr_ext != NULL && (wcscmp(curr_ext, L".h") == 0 || wcscmp(curr_ext, L".H") == 0))
	{
		m_header_file = TRUE;
	}
	else if (curr_ext != NULL && (wcscmp(curr_ext, L".rc") == 0 || wcscmp(curr_ext, L".RC") == 0))
	{
		m_resource_file = TRUE;
	}
	else if (curr_ext != NULL && (wcscmp(curr_ext, L".css") == 0 || wcscmp(curr_ext, L".CSS") == 0))
	{
		m_css_file = TRUE;
	}

	// Print the verbose directory header and update the count of directories.
	if (m_scanned_files_in_subdir == 0)
	{
		if (m_verbose_mode == TRUE)
			PrintSubdirectoryHeader();
		m_subdirs_with_files++;
	}

	// Print the verbose file header.
	if (m_verbose_mode == TRUE)
		PrintFileNameHeader();

	// Count the number of lines in the file.
	int cnt_lines = 0;
	if (m_curr_file_info.info.file_len > 0)
	{
		cnt_lines++;
		wchar_t *pnt_ch = m_curr_file_info.info.file_body+m_curr_file_info.info.file_len;
		while (--pnt_ch >= m_curr_file_info.info.file_body)
			if (*pnt_ch == L'\n')
				cnt_lines++;
	}

	// Pass current file to the raw file data checkers.
	for (TListIter<TFileDataDetailsChecker> iter1(m_src_details_checkers); iter1; ++iter1)
		iter1.CurrItem().ProcessFileData(basic_info);

	// Prepare processing in the lexema checkers.
	for (TListIter<TLexemaDetailsChecker> iter2(m_lex_details_checkers); iter2; ++iter2)
		iter2.CurrItem().StartNewFileProcessing();

	// Process the file using the C/C++ raw scanner.
	int code_lines = 0;
	bool non_empty_lex_flag = FALSE;
	TStringsDict disct;
	TRawScanner scanner(&m_curr_file_info, &disct);
	TLexema prev_lex;
	for(;;)
	{
		TLexema lex;
		scanner.GetLex(lex);

		// Do the stnd processing.
		if (lex.type >= ltx_number && lex.type <= ltx_name || lex.type == ltx_error)
		{
			non_empty_lex_flag = TRUE;
		}
		else if (lex.type == ltx_eol)
		{
			if (non_empty_lex_flag == TRUE)
				code_lines++;
			non_empty_lex_flag = FALSE;
		}

		// Pass current lexema to the lexema data checkers.
		for (TListIter<TLexemaDetailsChecker> iter3(m_lex_details_checkers); iter3; ++iter3)
			iter3.CurrItem().ProcessLexema(prev_lex, lex);

		prev_lex = lex;
		if (lex.type == ltx_eof)
			break;
	}

	// Close processing in the lexema checkers.
	for (TListIter<TLexemaDetailsChecker> iter4(m_lex_details_checkers); iter4; ++iter4)
		iter4.CurrItem().FinalizeFileProcessing();

	// Update general statistics.
	m_scanned_files_in_subdir++;
	m_scanned_files_count++;
	m_total_files_size += m_curr_file_info.info.file_len;
	m_total_lines_count += cnt_lines;
	m_code_lines_count += code_lines;
}

void TSrcDetailsIterationHandler::PrintSummary(TGenericConsoleEx &cons)
{
	// Setup the console infrastructure.
	TGenericParser dummyParser;
	TMidLevScanCbkHandler midLevHandler(&cons);
	TParsingDriver driver(dummyParser, midLevHandler);
	m_owner_driver = &driver;

	// Print summary about the scanned files.
	wprintf(L"\r\n\r\n Raw Summary\r\n--------------------------------------------------\r\n");
	wprintf(L" Number of subdirs with files: \t%8ld.\r\n", m_subdirs_with_files);
	wprintf(L" Number of files scanned: \t%8ld.\r\n", m_scanned_files_count);
	wprintf(L" Total files length: \t\t%8ld.\r\n", m_total_files_size);
	wprintf(L" Total number of lines:\t\t%8ld.\r\n", m_total_lines_count);
	wprintf(L" Number of code lines: \t\t%8ld.\r\n", m_code_lines_count);

	// Print summary about the discovered details.
	wprintf(L"\r\n\r\n Details Summary\r\n--------------------------------------------------\r\n");
	if (m_src_details_checkers.IsEmpty() == TRUE && m_lex_details_checkers.IsEmpty() == TRUE)
	{
		// There are no details because there are no checkers to find them.
		wprintf(L" The details checkers are missing.\r\n\r\n");
	}
	else
	{
		 if (m_cnt_details_in_all_files <= 0)
		 {
			// The details are missing.
			wprintf(L" There are no discovered details.\r\n\r\n");
		}
		else
		{
			//
			// The checkers and details are both present.
			//
			int cnt_counts = 0;
			wchar_t buff_details_name[160];

			for (TListIter<TFileDataDetailsChecker> iter1(m_src_details_checkers); iter1; ++iter1)
			{
				TFileDataDetailsChecker &checker1 = iter1.CurrItem();
				for (int idetail1=0; idetail1<checker1.NumDetailTypes(); ++idetail1)
				{
					int cnt1 = checker1.GetNumDetails(idetail1);
					if (cnt1 > 0)
					{
						swprintf(buff_details_name, 160, L"%s:", checker1.GetDetailName(idetail1));
						wprintf(L" %-34s   %8ld.\r\n", buff_details_name, cnt1);
						cnt_counts++;
					}
				}
			}

			for (TListIter<TLexemaDetailsChecker> iter2(m_lex_details_checkers); iter2; ++iter2)
			{
				TLexemaDetailsChecker &checker2 = iter2.CurrItem();
				for (int idetail2=0; idetail2<checker2.NumDetailTypes(); ++idetail2)
				{
					int cnt2 = checker2.GetNumDetails(idetail2);
					if (cnt2 > 0)
					{
						swprintf(buff_details_name, 160, L"%s:", checker2.GetDetailName(idetail2));
						wprintf(L" %-34s   %8ld.\r\n", buff_details_name, cnt2);
						cnt_counts++;
					}
				}
			}

			if (cnt_counts == 0)
				wprintf(L" ERROR COLLECTING COUNT OF DISCOVERED DETAILS\r\n");

			wprintf(L"--------------------------------------------------\r\n");
			wprintf(L" Total files with details: %4d.\r\n", m_cnt_files_with_details);
			wprintf(L" Total details discovered: %4d.\r\n\r\n", m_cnt_details_in_all_files);
		}

		// Print custom reports. They may be non empty even if there are no discovered details.
		for (TListIter<TGenericDetailsChecker> iter3(m_src_details_checkers); iter3; ++iter3)
			iter3.CurrItem().PrintCustomReport();
		for (TListIter<TGenericDetailsChecker> iter4(m_lex_details_checkers); iter4; ++iter4)
			iter4.CurrItem().PrintCustomReport();
	}

	wprintf(L"\r\n");
	m_owner_driver = NULL;
}

void TSrcDetailsIterationHandler::PrintSubdirectoryHeader()
{
	if (m_subdir_header_printed == FALSE)
	{
		wprintf(L"\r\n %s\r\n-------------------------------------------------------\r\n", m_curr_dir_name);
		m_subdir_header_printed = TRUE;
	}
}

void TSrcDetailsIterationHandler::PrintFileNameHeader()
{
	if (m_file_header_printed == FALSE)
	{
		wprintf(L"%6d. FileName: %s\r\n", ++m_file_headers_count, m_curr_file_info.info.file_name);
		m_file_header_printed = TRUE;
	}
}

void TSrcDetailsIterationHandler::ReportDetail(long file_offset, const wchar_t *format, ...)
{
	PrintSubdirectoryHeader();
	PrintFileNameHeader();

	va_list vargs;
	va_start(vargs, format);
	wchar_t buffer[1024];
	vswprintf(buffer, 1024, format, vargs);
	va_end(vargs);

	wprintf(L"         Line: %5d: %s\r\n", GetLineNumber(file_offset), buffer);

	ReportSilentDetail();
}

void TSrcDetailsIterationHandler::ReportDetail(PrEventHeader &origin, const wchar_t *format, ...)
{
	PrintSubdirectoryHeader();
	PrintFileNameHeader();

	va_list vargs;
	va_start(vargs, format);
	wchar_t buffer[1024];
	vswprintf(buffer, 1024, format, vargs);
	va_end(vargs);

	wprintf(L"         Line: %5d, %s\r\n", GetLineNumber(origin.src_area.AreaBeg()), buffer);

	ReportSilentDetail();
}

void TSrcDetailsIterationHandler::ReportSilentDetail()
{
	if (m_cnt_details == 0)
		m_cnt_files_with_details++;

	m_cnt_details++;
	m_cnt_details_in_all_files++;
}

int TSrcDetailsIterationHandler::GetLineNumber(long file_offset)
{
	if (m_curr_file_info.BuildLinesInfo() == FALSE)
		return(-1);

	return(m_curr_file_info.GetLineNumber(file_offset));
}

// -----------------------------------------------------------------------------
//	===================  TMiscFileDetailsChecker  =====================
// -----------------------------------------------------------------------------

TMiscFileDetailsChecker::TMiscFileDetailsChecker(TSrcDetailsIterationHandler &handler, bool notify_on_spaces_after_tabs)
		: TFileDataDetailsChecker(handler)
{
	m_notify_on_spaces_after_tabs = notify_on_spaces_after_tabs;

	m_cnt_tabs_after_spc = 0;
	m_cnt_spcs_after_tab = 0;

	m_cnt_spcs_before_eol = 0;
	m_cnt_tabs_before_eol = 0;
	m_cnt_spcs_line_beg = 0;
}

const wchar_t *TMiscFileDetailsChecker::GetDetailName(int idetail)
{
	switch (idetail)
	{
		case 0:	return(L"Tabs after spaces");
		case 1:	return(L"Spaces after tabs");

		case 2:	return(L"Spaces before EOL");
		case 3:	return(L"Tabs before EOL");
		case 4:	return(L"Spaces at the line beg");

		default:
			return(L"BogusDetailIndex");
	}
}

int TMiscFileDetailsChecker::GetNumDetails(int idetail)
{
	switch (idetail)
	{
		case 0:	return(m_cnt_tabs_after_spc);
		case 1:	return(m_cnt_spcs_after_tab);

		case 2:	return(m_cnt_spcs_before_eol);
		case 3:	return(m_cnt_tabs_before_eol);
		case 4:	return(m_cnt_spcs_line_beg);

		default:
			return(0);
	}
}

void TMiscFileDetailsChecker::ProcessFileData(TBasicFileInfo &basic_info)
{
	for (int inx=0; inx<basic_info.file_len; ++inx)
	{
		wchar_t ch = basic_info.file_body[inx];

		if (ch == L' ' && inx < basic_info.file_len-1)
		{
			// Space in the middle of the file.
			wchar_t ch_after = basic_info.file_body[inx+1];
			if (ch_after == L'\t')
			{
				m_iteration_handler.ReportDetail(inx, L"Tab after space.");
				m_cnt_tabs_after_spc++;
			}
		}

		if (ch == L'\t' && m_notify_on_spaces_after_tabs == TRUE && inx < basic_info.file_len-1)
		{
			// Tab in the middle of the file.
			wchar_t ch_after = basic_info.file_body[inx+1];
			if (ch_after == L' ')
			{
				m_iteration_handler.ReportDetail(inx, L"Space after tab.");
				m_cnt_spcs_after_tab++;
			}
		}

		if (ch == L'\r' && inx > 0)
		{
			// End of line in the middle of the file.
			wchar_t ch_before = basic_info.file_body[inx-1];
			if (ch_before == L' ' && IsRcFile() == FALSE)
			{
				m_iteration_handler.ReportDetail(inx, L"Space before EOL.");
				m_cnt_spcs_before_eol++;
			}
			else if (ch_before == L'\t')
			{
				m_iteration_handler.ReportDetail(inx, L"Tab before EOL.");
				m_cnt_tabs_before_eol++;
			}
		}

		if (ch == L'\n' && IsRcFile() == FALSE && inx < basic_info.file_len-1)
		{
			// End of line in the middle of the file.
			wchar_t ch_after = basic_info.file_body[inx+1];
			if (ch_after == L' ')
			{
				m_iteration_handler.ReportDetail(inx+1, L"Space in the beg of the line.");
				m_cnt_spcs_line_beg++;
			}
		}
	}
}

// -------------------------------------------------------------------------------
//	===================  TSrcFileDisclaimerChecker  =====================
// -------------------------------------------------------------------------------

const wchar_t *g_Disclaimer1 =
{
	L"//\r\n"
	L"//      Kirill Kobelev, Moscow-Paris-Sammamish.\r\n"
	L"//  -------------------------------------------------\r\n"
	L"//   All rights reserved. Commercial use without written permission prohibited.\r\n"
	L"//\r\n"
};

const wchar_t *g_Disclaimer2 =
{
	L"/**/\r\n"
	L"/**    www.CodeSturctureAnalysis.com. All rights reserved.    **/\r\n"
	L"/**/\r\n"
};

TSrcFileDisclaimerChecker::TSrcFileDisclaimerChecker(TSrcDetailsIterationHandler &handler)
		: TFileDataDetailsChecker(handler)
{

	m_cnt_missing_disclaimers = 0;
}

void TSrcFileDisclaimerChecker::ProcessFileData(TBasicFileInfo &basic_info)
{
	if (IsCssFile() == TRUE)
	{
		CheckDisclaimerPresence(basic_info, g_Disclaimer2, TRUE, TRUE);
	}
	else if (IsRcFile() == FALSE && IsSystemGeneratedFile() == FALSE)
	{
		// The file with app ids for the Cpp grammar is a header file but it does not need a disclaimer.
		if (wcsstr(basic_info.file_name, L"CppGrammarAppIds") == NULL)
			CheckDisclaimerPresence(basic_info, g_Disclaimer1, TRUE, TRUE);
	}

}
void TSrcFileDisclaimerChecker::CheckDisclaimerPresence(TBasicFileInfo &basic_info, const wchar_t *xpct_disclaimer, bool allow_comments_continuation, bool req_trailing_empty_line)
{
	int dclmr_len = wcslen(xpct_disclaimer);
	if (basic_info.file_len < dclmr_len)
	{
		// The current file is too short.
		m_iteration_handler.ReportDetail(0, L"The file is too short to contain the requied disclaimer.");
		m_cnt_missing_disclaimers++;
		return;
	}

	// Compare contents of the file and the required disclaimer char by char.
	int ln_pos = 1, ch_pos = 1;
	for (int inx=0; inx<dclmr_len; ++inx)
	{
		wchar_t ch = basic_info.file_body[inx];
		if (ch != xpct_disclaimer[inx])
		{
			wchar_t ctx1[40], ctx2[40];
			m_iteration_handler.ReportDetail(0, L"The disclaimer is missing. Check the line %d, pos %d. Instead of %s there should be %s.",
					ln_pos, ch_pos, PickContext(ctx1, basic_info.file_body+inx, basic_info.file_len-inx), PickContext(ctx2, xpct_disclaimer+inx, dclmr_len-inx));
			m_cnt_missing_disclaimers++;
			return;
		}

		ch_pos++;
		if (ch == L'\n')
		{
			ln_pos++;
			ch_pos = 1;
		}
	}

	// The body of the disclaimer is present.
	wchar_t *ptr_cont = basic_info.file_body+dclmr_len;
	long cont_len = basic_info.file_len-dclmr_len;
	if (allow_comments_continuation == TRUE && cont_len >= 2)
	{
		if (ptr_cont[0] == L'/' && ptr_cont[1] == L'/')
		{
			// The disclaimer is followed by the C++ comment. Look for the end of this comment.
			while (cont_len >= 2 && ptr_cont[0] == L'/' && ptr_cont[1] == L'/')
			{
				int line_len = cont_len;
				for (int ich=0; ich<cont_len; ++ich)
				{
					if (ptr_cont[ich] == L'\n')
					{
						line_len = ich+1;
						break;
					}
				}

				ptr_cont += line_len;
				cont_len -= line_len;
			}
		}
		else if (ptr_cont[0] == L'/' && ptr_cont[1] == L'*')
		{
			// The disclaimer is followed by the C style comment.
			while (cont_len >= 2 && ptr_cont[0] == L'/' && ptr_cont[1] == L'*')
			{
				// Look for the length of this comment.
				int comment_len = cont_len;
				for (int ich=0; ich<cont_len-1; ++ich)
				{
					if (ptr_cont[ich] == L'*' && ptr_cont[ich+1] == L'/')
					{
						comment_len = ich+2;
						break;
					}
				}

				ptr_cont += comment_len;
				cont_len -= comment_len;

				// Skip the end of the line if it is present after the comment.
				if (cont_len >= 2 && ptr_cont[0] == L'\r' && ptr_cont[1] == L'\n')
				{
					ptr_cont += 2;
					cont_len -= 2;
				}
			}
		}
	}

	if (req_trailing_empty_line == TRUE)
	{
		// Params require an empty line after the disclaimer.
		if (cont_len < 2 || ptr_cont[0] != L'\r' || ptr_cont[1] != L'\n')
		{
			m_iteration_handler.ReportDetail(0, L"Empty line is missing after the disclaimer.");
			m_cnt_missing_disclaimers++;
			return;
		}
	}
}

const wchar_t *TSrcFileDisclaimerChecker::PickContext(wchar_t *buffer, const wchar_t *data, int data_len)
{
	if (data_len <= 0)
	{
		wcscpy(buffer, L"<Empty context>");
	}
	else if (data[0] == L'\r' || data[0] == L'\r')
	{
		wcscpy(buffer, L"<End of line>");
	}
	else if (data[0] < L' ' && data[0] != L'\t')
	{
		wcscpy(buffer, L"<Non printable chars>");
	}
	else
	{
		int len_ctx = 8;

		buffer[0] = L'\"';
		for (int inx=0; ((inx < len_ctx) && (inx < data_len)); ++inx)
		{
			if (data[inx] == L'\r' || data[inx] == L'\r' || (data[inx] < L' ' && data[inx] != L'\t'))
			{
				// End of context in the middle of the data.
				buffer[inx+1] = L'\"';
				buffer[inx+2] = 0;
				return(buffer);
			}

			// Copy in the char.
			buffer[inx+1] = data[inx];
		}

		if (data_len <= len_ctx)
			buffer[data_len+1] = 0;
		else wcscpy(buffer+len_ctx+1, L"...");
		wcscat(buffer, L"\"");
	}

	return(buffer);
}

// ----------------------------------------------------------------------------
//	================  TKeywordSeparDetailsChecker  ===================
// ----------------------------------------------------------------------------

TKeywordSeparDetailsChecker::TKeywordSeparDetailsChecker(TSrcDetailsIterationHandler &handler)
		: TLexemaDetailsChecker(handler)
{
	m_ignore_lex_cnt = 0;
	m_cnt_events = 0;
	m_cnt_error_lex = 0;
	m_cnt_bad_comments = 0;

	StartNewFileProcessing();
}

const wchar_t *TKeywordSeparDetailsChecker::GetDetailName(int idetail)
{
	switch (idetail)
	{
		case 0:	return(L"Keyword separators");
		case 1:	return(L"Error lexemas");
		case 2:	return(L"Badly formatted comments");

		default:
			return(L"BogusDetailIndex");
	}
}

int TKeywordSeparDetailsChecker::GetNumDetails(int idetail)
{
	switch (idetail)
	{
		case 0:	return(m_cnt_events);
		case 1:	return(m_cnt_error_lex);
		case 2:	return(m_cnt_bad_comments);

		default:
			return(0);
	}
}

void TKeywordSeparDetailsChecker::StartNewFileProcessing()
{
	m_for_lexema_flag = FALSE;
	m_for_header_pars_cnt = 0;
	m_ifndef_mentioned = FALSE;
	m_define_mentioned = FALSE;
}

void TKeywordSeparDetailsChecker::ProcessLexema(TLexema &prev_lex, TLexema &lex)
{
	if (IsRcFile() == TRUE)
		return;

	if (m_ignore_lex_cnt > 0)
	{
		// Some construct requested to ignore some number of lexemas.
		m_ignore_lex_cnt--;
		return;
	}

	if (lex.type == ltx_comment && (lex.comment_type == lct_cpp_wholeline || lex.comment_type == lct_cpp_endofline))
	{
		// Ensure that double slashes are followed by the space symbol.
		TSourceArea &area = lex.origin.src_area;
		if (area.area_len > 2 && IsSpaceChar(area.AreaBegPtr()[2]) == FALSE && area.AreaBegPtr()[2] != L'-' && IsNoDependenciesMsg(area.AreaBegPtr(), area.area_len) == FALSE)
		{
			m_iteration_handler.ReportDetail(lex.origin, L"There is no space after the double slashes in the C++ style comment.");
			m_cnt_bad_comments++;
		}
	}

	if (lex.type == ltx_keyword)
	{
		ProcessKeyword(lex);
	}
	else if (prev_lex.IsKeyword(mpr_define) == TRUE && lex.IsName(L"STRICT") == TRUE)
	{
		if (prev_lex.origin.src_area.AreaEnd()+4 != lex.origin.src_area.AreaBeg())
		{
			m_iteration_handler.ReportDetail(lex.origin, L"Wrong number of separators in the \"#define STRICT\" statement.");
			m_cnt_events++;
		}
	}
	else if (lex.type == ltx_error)
	{
		m_iteration_handler.ReportDetail(lex.origin, L"Error lexema.");
		m_cnt_error_lex++;
	}

	if (m_for_header_pars_cnt > 0)
	{
		if (lex.IsKeyword(opr_lpar) == TRUE)
			m_for_header_pars_cnt++;
		if (lex.IsKeyword(opr_rpar) == TRUE)
			m_for_header_pars_cnt--;
	}

	if (m_for_lexema_flag == TRUE && lex.IsKeyword(clg_for) == FALSE)
	{
		if (lex.IsKeyword(opr_lpar) == TRUE)
			m_for_header_pars_cnt = 1;
		if (lex.IsEol() == FALSE)
			m_for_lexema_flag = FALSE;
	}
}

void TKeywordSeparDetailsChecker::ProcessKeyword(TLexema &lex)
{
	//
	// Check for keywords that require non standard processing.
	//

	if (lex.IsKeyword(mpr_define) == TRUE && IsSystemGeneratedFile() == FALSE)
	{
		if (m_define_mentioned == FALSE && IsHeaderFile() == TRUE)
		{
			CheckSeparatorsAfter(lex, L"DEFINE", TRUE, 1);
			m_define_mentioned = TRUE;
		}
	}
	else if (lex.IsKeyword(mpr_ifndef) == TRUE && IsHeaderFile() == TRUE && IsSystemGeneratedFile() == FALSE)
	{
		if (m_ifndef_mentioned == FALSE)
		{
			CheckSeparatorsAfter(lex, L"IFNDEF", TRUE, 1);
			m_ifndef_mentioned = TRUE;
		}
		else
		{
			CheckSeparatorsAfter(lex, L"IFNDEF", FALSE, 3);
		}
	}
	else if (lex.IsKeyword(mpr_include) == TRUE)
	{
		CheckSeparatorsAfter(lex, L"INCLUDE", FALSE, 2);
	}
	else if (lex.IsKeyword(opr_comma) == TRUE)
	{
		if (lex.origin.src_area.AreaBeg() > 0)
		{
			wchar_t prev_ch = FileBodyPtr()[lex.origin.src_area.AreaBeg()-1];
			if (prev_ch == L' ' || prev_ch == L'\t' || prev_ch == L'\n')
			{
				m_iteration_handler.ReportDetail(lex.origin, L"Space in front of the comma.");
				m_cnt_events++;
			}
		}

		if (lex.origin.src_area.AreaEnd() < FileLength())
		{
			wchar_t next_ch = FileBodyPtr()[lex.origin.src_area.AreaEnd()];
			if (next_ch != L' ' && next_ch != L'\t' && next_ch != L'\r')
			{
				m_iteration_handler.ReportDetail(lex.origin, L"There is no space after the comma.");
				m_cnt_events++;
			}
		}
	}
	else if (lex.IsKeyword(spr_semicol) == TRUE && m_for_header_pars_cnt == 0)
	{
		if (lex.origin.src_area.AreaEnd() < FileLength())
		{
			wchar_t next_ch = FileBodyPtr()[lex.origin.src_area.AreaEnd()];
			if (next_ch != L' ' && next_ch != L'\t' && next_ch != L'\r')
			{
				m_iteration_handler.ReportDetail(lex.origin, L"There is no space after the semicolon.");
				m_cnt_events++;
			}
		}
	}

	//
	// Check for spaces around the major delimiters and operators.
	//

	else if (lex.IsKeyword(spr_lcurvbr) == TRUE)
		CheckSpacesOnBothSides(lex, L"{", TRUE, FALSE);
	else if (lex.IsKeyword(spr_rcurvbr) == TRUE)
		CheckSpacesOnBothSides(lex, L"}", FALSE, TRUE);

	else if (lex.IsKeyword(opr_question) == TRUE)
		CheckSpacesOnBothSides(lex, L"?");

	else if (lex.IsKeyword(opr_le) == TRUE)
		CheckSpacesOnBothSides(lex, L"<=");
	else if (lex.IsKeyword(opr_ge) == TRUE)
		CheckSpacesOnBothSides(lex, L">=");
	else if (lex.IsKeyword(opr_eq) == TRUE)
		CheckSpacesOnBothSides(lex, L"==");
	else if (lex.IsKeyword(opr_ne) == TRUE)
		CheckSpacesOnBothSides(lex, L"!=");

	else if (lex.IsKeyword(opr_bitlsh) == TRUE)
		CheckSpacesOnBothSides(lex, L"<<");
	else if (lex.IsKeyword(opr_bitrsh) == TRUE)
		CheckSpacesOnBothSides(lex, L">>");

	else if (lex.IsKeyword(opr_bitxor) == TRUE)
		CheckSpacesOnBothSides(lex, L"^");
	else if (lex.IsKeyword(opr_bitor) == TRUE)
		CheckSpacesOnBothSides(lex, L"|");
	else if (lex.IsKeyword(opr_logand) == TRUE)
		CheckSpacesOnBothSides(lex, L"&&");
	else if (lex.IsKeyword(opr_logor) == TRUE)
		CheckSpacesOnBothSides(lex, L"||");

	else if (lex.IsKeyword(opr_assign) == TRUE && m_for_header_pars_cnt == 0)
		CheckSpacesOnBothSides(lex, L"=", FALSE, TRUE);

	else if (lex.IsKeyword(opr_mul_asgn) == TRUE)
		CheckSpacesOnBothSides(lex, L"*=");
	else if (lex.IsKeyword(opr_div_asgn) == TRUE)
		CheckSpacesOnBothSides(lex, L"/=");
	else if (lex.IsKeyword(opr_rmnd_asgn) == TRUE)
		CheckSpacesOnBothSides(lex, L"%=");
	else if (lex.IsKeyword(opr_plus_asgn) == TRUE)
		CheckSpacesOnBothSides(lex, L"+=");
	else if (lex.IsKeyword(opr_minus_asgn) == TRUE)
		CheckSpacesOnBothSides(lex, L"-=");
	else if (lex.IsKeyword(opr_lsh_asgn) == TRUE)
		CheckSpacesOnBothSides(lex, L">>=");
	else if (lex.IsKeyword(opr_rsh_asgn) == TRUE)
		CheckSpacesOnBothSides(lex, L"<<=");
	else if (lex.IsKeyword(opr_btand_asgn) == TRUE)
		CheckSpacesOnBothSides(lex, L"&=");
	else if (lex.IsKeyword(opr_btor_asgn) == TRUE)
		CheckSpacesOnBothSides(lex, L"|=");
	else if (lex.IsKeyword(opr_btxor_asgn) == TRUE)
		CheckSpacesOnBothSides(lex, L"^=");

	else if (lex.IsKeyword(opr_dot) == TRUE)
			CheckNoSpacesOnBothSides(lex, L".");
	else if (lex.IsKeyword(opr_dotstar) == TRUE)
			CheckNoSpacesOnBothSides(lex, L".*");
	else if (lex.IsKeyword(opr_arrow) == TRUE)
			CheckNoSpacesOnBothSides(lex, L"->");
	else if (lex.IsKeyword(opr_arrowstar) == TRUE)
			CheckNoSpacesOnBothSides(lex, L"->*");

	//
	// Check for spaces after the major statements.
	//

	else if (lex.IsKeyword(clg_if) == TRUE)
	{
		CheckSpaceAtTheEnd(lex, L"IF");
	}
	else if (lex.IsKeyword(clg_for) == TRUE)
	{
		// Check for the special case of the "for(;;)" statement.
		if (lex.origin.src_area.AreaEnd() <= FileLength()-4)
		{
			wchar_t *data_after = FileBodyPtr()+lex.origin.src_area.AreaEnd();
			if (data_after[0] == L'(' && data_after[1] == L';' && data_after[2] == L';' && data_after[3] == L')')
			{
				m_ignore_lex_cnt = 4;
				return;
			}
		}

		m_for_lexema_flag = TRUE;
		CheckSpaceAtTheEnd(lex, L"FOR");
	}
	else if (lex.IsKeyword(clg_while) == TRUE)
	{
		CheckSpaceAtTheEnd(lex, L"WHILE");
	}
	else if (lex.IsKeyword(clg_switch) == TRUE)
	{
		CheckSpaceAtTheEnd(lex, L"SWITCH");
	}
	else if (lex.IsKeyword(clg_case) == TRUE)
	{
		CheckSpaceAtTheEnd(lex, L"CASE");
	}
}

void TKeywordSeparDetailsChecker::CheckSpaceAtTheEnd(TLexema &lex, const wchar_t *lexema_name)
{
	if (lex.origin.src_area.AreaEnd() >= FileLength())
		return;

	// These keywords should be followed by one space.
	wchar_t next_ch1 = FileBodyPtr()[lex.origin.src_area.AreaEnd()];
	if (next_ch1 != L' ')
	{
		m_iteration_handler.ReportDetail(lex.origin, L"There is no space after the %s statement.", lexema_name);
		m_cnt_events++;
		return;
	}

	if (lex.origin.src_area.AreaEnd()+1 >= FileLength())
		return;

	// The character after the space should not be a space.
	wchar_t next_ch2 = FileBodyPtr()[lex.origin.src_area.AreaEnd()+1];
	if (next_ch2 == L' ' || next_ch2 == L'\t' || next_ch2 == L'\r' || next_ch2 == L'\n')
	{
		m_iteration_handler.ReportDetail(lex.origin, L"Too many spaces after the %s statement.", lexema_name);
		m_cnt_events++;
		return;
	}
}

void TKeywordSeparDetailsChecker::CheckSpacesOnBothSides(TLexema &lex, const wchar_t *lexema_name, bool lcrw_mode, bool rcrw_mode)
{
	if (lex.origin.src_area.AreaBeg() > 0)
	{
		wchar_t prev_ch = FileBodyPtr()[lex.origin.src_area.AreaBeg()-1];
		if (prev_ch != L' ' && prev_ch != L'\t' && prev_ch != L'\n')
		{
			m_iteration_handler.ReportDetail(lex.origin, L"Space in front of the \"%s\" is missing.", lexema_name);
			m_cnt_events++;
		}
	}

	if (lex.origin.src_area.AreaEnd() < FileLength())
	{
		wchar_t next_ch = FileBodyPtr()[lex.origin.src_area.AreaEnd()];
		if (next_ch != L' ' && next_ch != L'\r' && (lcrw_mode == FALSE || next_ch != L'\t') && (rcrw_mode == FALSE || (next_ch != L';' && next_ch != L',' && next_ch != L'\t')))
		{
			m_iteration_handler.ReportDetail(lex.origin, L"Space after the \"%s\" is missing.", lexema_name);
			m_cnt_events++;
		}
	}
}

void TKeywordSeparDetailsChecker::CheckNoSpacesOnBothSides(TLexema &lex, const wchar_t *lexema_name)
{
	if (lex.origin.src_area.AreaBeg() > 0)
	{
		wchar_t prev_ch = FileBodyPtr()[lex.origin.src_area.AreaBeg()-1];
		if ((prev_ch == L' ' || prev_ch == L'\t') && IsCssFile() == FALSE)
		{
			m_iteration_handler.ReportDetail(lex.origin, L"Space in front of the \"%s\".", lexema_name);
			m_cnt_events++;
		}
	}

	if (lex.origin.src_area.AreaEnd() < FileLength())
	{
		wchar_t next_ch = FileBodyPtr()[lex.origin.src_area.AreaEnd()];
		if (next_ch == L' ' || next_ch == L'\t')
		{
			m_iteration_handler.ReportDetail(lex.origin, L"Space after the \"%s\".", lexema_name);
			m_cnt_events++;
		}
	}
}

void TKeywordSeparDetailsChecker::CheckSeparatorsAfter(TLexema &lex, const wchar_t *stmt_name, bool want_tab_sep, int num_separs)
{
	if (lex.origin.src_area.AreaEnd()+num_separs+1 >= FileLength())
	{
		m_iteration_handler.ReportDetail(lex.origin, L"Statement %s is too close to the end of the file.", stmt_name);
		m_cnt_events++;
		return;
	}

	wchar_t *data_after = FileBodyPtr()+lex.origin.src_area.AreaEnd();
	for (int inx=0; inx<num_separs; ++inx)
	{
		if (data_after[inx] != ((want_tab_sep == TRUE) ? L'\t' : L' '))
		{
			m_iteration_handler.ReportDetail(lex.origin, L"Only %d out of %d separator%s %s present after the statement %s.", inx, num_separs, PluralSuffix(inx), GetPresentTenseVerbForNumber(inx), stmt_name);
			m_cnt_events++;
			return;
		}
	}

	wchar_t ch_after = data_after[num_separs];
	if (ch_after == L' ' || ch_after == L'\t' || ch_after == L'\r' || ch_after == L'\n')
	{
		m_iteration_handler.ReportDetail(lex.origin, L"Sequence of separators after the statement %s is too long.", stmt_name);
		m_cnt_events++;
		return;
	}
}

// -----------------------------------------------------------------------------
//	=================  TCommentedCodeChecker  =====================
// -----------------------------------------------------------------------------

void TCommentedCodeChecker::ProcessLexema(TLexema &prev_lex, TLexema &lex)
{
	if (lex.type != ltx_comment || lex.comment_type == lct_csh_trislash)
		return;

	// Current lexema is either C or C++ comment.
	TSourceArea area = lex.origin.src_area;
	area.area_beg += 2;
	area.area_len -= 2;
	if (lex.comment_type == lct_c_style)
		area.area_len -= 2;

	assert(area.area_len >= 0);
	TStringsDict disct;
	TRawScanner scanner(FileInfo(), &disct);
	scanner.Seek(area.area_beg, area.area_len, TRUE, FALSE);

	int num_keywords = 0;
	double fragment_score = 0;
	int num_lexemas = 0;
	for(;;)
	{
		TLexema lex;
		scanner.GetLex(lex);
		if (lex.type == ltx_eof)
			break;

		if (lex.IsKeyword(opr_minus) == TRUE || lex.IsKeyword(opr_minusminus) == TRUE)
		{
			if (SkipSequence(scanner, lex.origin.src_area.AreaBeg()) != lex.origin.src_area.area_len)
				continue;
		}
		if (lex.IsKeyword(opr_assign) == TRUE || lex.IsKeyword(opr_eq) == TRUE)
		{
			if (SkipSequence(scanner, lex.origin.src_area.AreaBeg()) != lex.origin.src_area.area_len)
				continue;
		}

		num_lexemas++;
		bool lex_keyword = FALSE;
		bool lex_lo_pri_keyword = FALSE;
		if (lex.type == ltx_keyword)
		{
			if (lex.IsKeyword(opr_comma) == FALSE && lex.IsKeyword(opr_dot) == FALSE && lex.IsKeyword(clg_if) == FALSE && lex.IsKeyword(clg_default) == FALSE)
			{
				if (lex.IsKeyword(opr_new) == FALSE && lex.IsKeyword(opr_delete) == FALSE && lex.IsKeyword(clg_short) == FALSE && lex.IsKeyword(clg_long) == FALSE)
				{
					if (lex.IsKeyword(clg_for) == FALSE && lex.IsKeyword(clg_do) == FALSE && lex.IsKeyword(clg_case) == FALSE)
					{
						lex_keyword = TRUE;
						num_keywords++;

						lex_lo_pri_keyword = FALSE;
						lex_lo_pri_keyword |= lex.IsKeyword(opr_minus) || lex.IsKeyword(opr_minusminus) || lex.IsKeyword(opr_div);
						lex_lo_pri_keyword |= lex.IsKeyword(opr_lpar) || lex.IsKeyword(opr_rpar);
						lex_lo_pri_keyword |= lex.IsKeyword(opr_lt) || lex.IsKeyword(opr_gt);
						lex_lo_pri_keyword |= lex.IsKeyword(clg_char) || lex.IsKeyword(clg_signed) || lex.IsKeyword(clg_unsigned);
						lex_lo_pri_keyword |= lex.IsKeyword(cpp_this) || lex.IsKeyword(cpp_class);
					}
				}
			}
		}

		if (lex.type == ltx_number)
		{
			fragment_score += 1;
		}
		else if (lex.type == ltx_charconst || lex.type == ltx_string)
		{
			fragment_score += 1;
		}
		else if (lex_keyword == TRUE && lex_lo_pri_keyword == TRUE)
		{
			fragment_score += 0.5;
		}
		else if (lex_keyword == TRUE && lex_lo_pri_keyword == FALSE)
		{
			fragment_score += 1;
		}
	}

	if (num_lexemas >= 3 && num_keywords > 0)
	{
		double density = fragment_score/num_lexemas;
		if (density >= m_min_density)
		{
			m_iteration_handler.ReportDetail(lex.origin, L"Commented code fragment. Lexemas: %d, Keywords: %d, Score: %.1f, Density: %.3f.",
										num_lexemas, num_keywords, fragment_score, density);
			m_cnt_events++;
		}
	}
}

long TCommentedCodeChecker::SkipSequence(TRawScanner &scanner, long seq_beg_offs)
{
	TRawScanSeekInfo seek_info;
	scanner.GetCurrPos(seek_info);
	long scan_area_end = scanner.GetScanAreaEnd();

	assert(seq_beg_offs < scan_area_end);
	wchar_t seq_char = FileBodyPtr()[seq_beg_offs];
	seek_info.offs = seq_beg_offs;
	while (seek_info.offs < scan_area_end && FileBodyPtr()[seek_info.offs] == seq_char)
		seek_info.offs++;

	scanner.SeekTo(seek_info);
	return(seek_info.offs-seq_beg_offs);
}

// -----------------------------------------------------------------------------
//	==================  TWin32ApiStyleChecker  ======================
// -----------------------------------------------------------------------------

//
// List of Win32 APIs that are used in the project. All these function names should be preceeded with
// the double colon to highlight that they belong to the system space and not to the project.
//
const wchar_t *g_UsedWin32Api[] =
{
	L"GetCurrentProcessId",		L"DebugBreak",

	L"CreateEvent",				L"SetEvent",					L"ResetEvent",
	L"CreateThread",				L"GetExitCodeThread",			L"GetLastError",				L"GetCurrentProcess",
	L"InitializeCriticalSection",		L"DeleteCriticalSection",			L"EnterCriticalSection",			L"LeaveCriticalSection",
	L"Sleep",						L"GetTickCount",				L"GetSystemTimeAsFileTime",
	L"WaitForSingleObject",			L"CloseHandle",				L"UuidCreate",

	L"CreateFileW",				L"SetFilePointer",				L"SetEndOfFile",				L"DeleteFileW",
	L"CreateDirectoryW",			L"GetCurrentDirectoryW",
	L"FindFirstFileW",				L"FindNextFileW",				L"FindClose",
	L"ReadFile",					L"WriteFile",					L"GetFileAttributesW",			L"GetFileAttributesExW",
	L"GetFileInformationByHandle",	L"CopyFileW",

	L"CreateFileMapping",			L"MapViewOfFile",				L"GetMappedFileNameW",		L"UnmapViewOfFile",
	L"GetFullPathNameW",			L"QueryDosDeviceW",			L"GetLogicalDriveStringsW",

	L"GetDateFormatW",			L"GetTimeFormatW",			L"FileTimeToSystemTime",		L"FileTimeToLocalFileTime",

	L"GetFocus",					L"SetFocus",					L"GetParent",					L"SetParent",
	L"GetClientRect",				L"GetWindowRect",				L"SetWindowPos",
	L"IsWindow",					L"IsWindowEnabled",			L"IsWindowVisible",			L"IsIconic",
	L"OpenIcon",
	L"ShowWindow",				L"DrawMenuBar",				L"ScreenToClient",
	L"EnableWindow",				L"InvalidateRect",				L"UpdateWindow",				L"SetForegroundWindow",
	L"MessageBoxW",				L"DialogBoxParam",			L"DialogBoxIndirectParam",		L"CreateDialogParam",
	L"CreateWindowW",			L"DestroyWindow",				L"EndDialog",					L"CreateDialogIndirectParam",
	L"SetWindowTextW",			L"GetDlgItem",				L"DefWindowProc",				L"GetClassNameW",
	L"RegisterClassW",				L"RegisterClassExW",

	L"SetTimer",					L"KillTimer",
	L"GetWindowLong",				L"GetWindowLongPtr",			L"SetWindowLong",				L"SetWindowLongPtr",
	L"IsDialogMessage",			L"GetDlgCtrlID",				L"GetDlgItemTextW",			L"SetDlgItemTextW",
	L"GetMessage",				L"PeekMessage",				L"SendMessage",				L"PostMessage",
	L"TranslateMessage",			L"DispatchMessage",			L"SendDlgItemMessage",

	L"Gdiplus",
	L"GetDC",					L"CreateCompatibleDC",		L"ReleaseDC",					L"DeleteDC",
	L"SaveDC",					L"RestoreDC",					L"MulDiv",
	L"BeginPaint",					L"EndPaint",					L"ScrollWindowEx",				L"GetSystemMetrics",
	L"GetScrollInfo",				L"SetScrollInfo",				L"GetDeviceCaps",

	L"CreateSolidBrush",			L"CreateHatchBrush",			L"CreateBrushIndirect",			L"LoadBitmap",
	L"LoadIcon",
	L"CreateFontW",				L"GetTextMetrics",				L"GetTextFaceW",
	L"CreateBitmap",				L"CreateCompatibleBitmap",
	L"CreatePen",					L"GetObject",					L"SelectObject",				L"DeleteObject",
	L"SetPixel",					L"GetPixel",					L"FillRect",					L"IntersectClipRect",
	L"MoveToEx",					L"LineTo",					L"BitBlt",						L"Ellipse",
	L"DrawTextW",				L"SetTextColor",				L"GetTextExtentPoint32W",
	L"SetBkColor",				L"SetBkMode",				L"SetGraphicsMode",			L"SetWorldTransform",

	L"GetSysColor",				L"GetSysColorBrush",
	L"GetMonitorInfo",				L"MonitorFromRect",
	L"GetConsoleWindow",			L"GetConsoleScreenBufferInfo",	L"SetConsoleTextAttribute",		L"GetStdHandle",
	L"SetConsoleTitleW",

	L"LoadCursor",				L"SetCursor",					L"SetCapture",				L"ReleaseCapture",
	L"TrackMouseEvent",
	L"CreatePopupMenu",			L"TrackPopupMenu",			L"DestroyMenu",				L"GetMenu",
	L"GetSubMenu",				L"InsertMenuW",				L"SetMenuItemInfo",

	L"GlobalAlloc",					L"GlobalLock",					L"GlobalUnlock",				L"GlobalFree",
	L"OpenClipboard",				L"SetClipboardData",			L"EmptyClipboard",				L"CloseClipboard",
	L"GetOpenFileNameW",			L"GetSaveFileNameW",			L"GetModuleFileNameW",		L"GetModuleHandle",
	L"ChooseColor",

	L"SHBrowseForFolder",			L"SHGetPathFromIDList",
	L"DefSubclassProc",			L"SetWindowSubclass",

	L"GetWindowThreadProcessId",	L"GetCurrentThreadId",			L"PostThreadMessage",

	NULL,
};

TWin32ApiStyleChecker::TWin32ApiStyleChecker(TSrcDetailsIterationHandler &handler)
		: TLexemaDetailsChecker(handler)
{
	m_prev_prev_lex_name = FALSE;
	m_cnt_bad_uses = 0;

	// Copy list of all known APIs into the hash table dict.
	const wchar_t **api = g_UsedWin32Api;
	while (*api != NULL)
	{
		// Ensure that the list of APIs does not have duplications.
		const wchar_t *old_ptr = m_known_api.FindString(*api, wcslen(*api));
		assert(old_ptr == NULL);

		// Add the finction name.
		const wchar_t *registered_ptr = m_known_api.RegisterStr(*api);
		assert(registered_ptr != NULL);
		api++;
	}
}

void TWin32ApiStyleChecker::ProcessLexema(TLexema &prev_lex, TLexema &lex)
{
	if (lex.type == ltx_name)
	{
		// Current lexema is name.
		const wchar_t *known_api_ptr = m_known_api.FindString(lex.str_value.m_body, lex.str_value.m_len);
		if (prev_lex.IsKeyword(opr_dblcolon) == FALSE)
		{
			// Current name is not preceeded with the double colon.
			if (known_api_ptr != NULL)
			{
				m_iteration_handler.ReportDetail(lex.origin, L"Win32 API %s(...) is not preceeded with the double colon.", known_api_ptr);
				m_cnt_bad_uses++;
			}
			else
			{
				const wchar_t *potential_api_ptr = m_noticed_api.FindString(lex.str_value.m_body, lex.str_value.m_len);
				if (potential_api_ptr != NULL)
				{
					m_iteration_handler.ReportDetail(lex.origin, L"Potential Win32 API %s is not preceeded with the double colon.", potential_api_ptr);
					m_cnt_bad_uses++;
				}
			}
		}
		else
		{
			// Double colon is present in front of the name. Check one more lexema ahead.
			if (m_prev_prev_lex_name == FALSE && known_api_ptr == NULL)
			{
				if (m_noticed_api.RegisterStr(lex.str_value.m_body, lex.str_value.m_len) == NULL)
				{
					m_iteration_handler.ReportDetail(lex.origin, L"Error adding potential Win32 API to the dictionary.");
					m_cnt_bad_uses++;
				}
			}
		}
	}

	m_prev_prev_lex_name = (prev_lex.type == ltx_name || prev_lex.IsKeyword(opr_gt) == TRUE) ? TRUE : FALSE;
}

void TWin32ApiStyleChecker::PrintCustomReport()
{
	if (m_noticed_api.NumItems() == 0)
		return;

	// The pattern that looks like a Win32 API is present.
	wprintf(L"\r\n");
	TraceWithAttr(YellowAttr, L" Win32 API detected");
	TraceWithAttr(YellowAttr, L"--------------------------------------------------");
	TraceWithAttr(YellowAttr, L" Number of functions: \t%8ld.", m_noticed_api.NumItems());
	wprintf(L"\r\n");

	int cnt = 0;
	for (m_noticed_api.StartIteration(); m_noticed_api.CheckIteration() == TRUE; m_noticed_api.StepIteration())
	{
		TraceWithAttr(YellowAttr, L"  %4d. %s\r\n", ++cnt, m_noticed_api.CurrIteratedItem()->Contents());
	}

	wprintf(L"\r\n");
}


