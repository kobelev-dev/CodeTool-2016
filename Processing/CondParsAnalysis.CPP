//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Conditional compilation structure analysis.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "Common/FormatNumber.H"
#include  "TextScan/ParsingDriver.H"
#include  "LangView/HtmlGenSource.H"
#include  "Processing/CondParsAnalysis.H"

// -----------------------------------------------------------------------------
//	=================  TCpanHtmlSourceCodeFile  =====================
// -----------------------------------------------------------------------------

class TCpanHtmlSourceCodeFile : public THtmlSourceCodeFile
{
public:		//
			//	This class shows parsing events from 2 different parsing sessions on one decorated file.
			//

			TCpanHtmlSourceCodeFile(ID extra_pars_sess, const wchar_t *css_class = NULL, const wchar_t *css_style = NULL, THtmlCrLfMode crlf_mode = crlm_outline_object_frames)
				: THtmlSourceCodeFile(css_class, css_style, crlf_mode) { m_extra_pars_sess = extra_pars_sess; }

	virtual void SetupAdditionalDecors(TSyntaxHighlighter &hlpr)
	{
		if (m_extra_pars_sess == 0)
			return;

		// The id of the parsing session is passed. Pick up the session info.
		if (hlpr.MldbReader().GetCompilationInfo(m_extra_pars_sess) == FALSE)
			return;

		ID mj_src_enter_evt = hlpr.MldbReader().CompilationData().major_src_enter_evt;
		assert(mj_src_enter_evt != 0);

		// Add extra decorations to the current file.
		hlpr.SetupMidLevEventDecors(m_extra_pars_sess, mj_src_enter_evt, PSET_TYPE_BIT(pset_macro_library_change));
	}

protected:

	ID		m_extra_pars_sess;
};

// -----------------------------------------------------------------------------
//	====================  TCpanHelperBase  ========================
// -----------------------------------------------------------------------------

TCpanHelperBase::TCpanHelperBase(TGenericConsoleEx &cons)
		: THtmlReportHelper(cons)
{
	m_curr_reader = NULL;
	m_curr_fid = -1;

	m_enable_src_files_legal_censoring = TRUE;
	m_enable_src_files_bandwidth_saving = TRUE;

	m_num_framing_lines = 50;
	m_min_lines_to_remove = 200;
}

bool TCpanHelperBase::PrepareHelper(TMidLevRamLogger &mldb)
{
	// Pick up the database reader.
	wchar_t buffer[4*MAX_PATH];
	m_curr_reader = mldb.AllocateReaderObject(buffer);
	if (m_curr_reader == NULL)
	{
		m_helper_console.HandleTrace(L"TCpanHelperBase: Error allocating the database reader.", TGenericConsole::ErrorHighlightMode);
		return(FALSE);
	}

	// Allocate and init an array of descriptors for all disk files in the database.
	int num_files = m_curr_reader->GetNumDiskFiles(TRUE);
	if (m_src_proc_info.ReserveTotalBufferSpace(num_files) == FALSE)
	{
		m_helper_console.HandleTrace(L"TCpanHelperBase: Low on memory while preparing the source file descriptors.", TGenericConsole::ErrorHighlightMode);
		return(FALSE);
	}

	// Fill in the array with empty elements.
	TFileProcInfo info = { { 0, 0 } };
	for (int inx=0; inx<num_files; ++inx)
		m_src_proc_info.AppendItem(info);

	// Success.
	return(TRUE);
}

void TCpanHelperBase::CleanupHelper(TMidLevRamLogger &mldb)
{
	assert(m_curr_reader != NULL);

	// Release the reader.
	mldb.DisposeReaderObject(m_curr_reader);
	m_curr_reader = NULL;
}

void TCpanHelperBase::DumpSourceFiles(const wchar_t *reports_directory, bool delete_old_source_files, bool delete_processed_pars_sessions)
{
	assert(m_curr_reader != NULL);

	if (delete_old_source_files == TRUE)
	{
		wprintf(L"Deleting old source files ...\r\n");
		DWORD res = TPathHelper::DeleteAllFilesFromDirectory(reports_directory);
		if (res != ERROR_SUCCESS)
			wprintf(L"Error %d detected. Some of the files may not be deleted.\r\n", res);
	}

	wprintf(L"Generating source file listings ...\r\n");
	int cnt_dumped = 0;

	// Look for files that are marked for processing.
	for (int fid=TFileInfo::DiskFileIdsBase; fid<m_src_proc_info.NumItems(); ++fid)
	{
		if (m_src_proc_info[fid].m_pars_sessions[0] == 0)
			continue;

		// Current disk file was mentioned in the analysis.
		m_curr_fid = fid;
		wchar_t fname_buffer[80];

		swprintf(fname_buffer, 80, L"Src_%05ld_Name", fid);
		WriteHtmlReport(reports_directory, fname_buffer, TRUE, (HtmlBodyWriterMethod)&TCpanHelperBase::DumpSourceFileName);

		swprintf(fname_buffer, 80, L"Src_%05ld", fid);
		WriteHtmlReport(reports_directory, fname_buffer, TRUE, (HtmlBodyWriterMethod)&TCpanHelperBase::DumpSourceFileBody);

		if (delete_processed_pars_sessions == TRUE)
		{
			// Caller is asking to remove processed sessions. This reduces the amount of required memory.
			m_curr_reader->DeleteParsingSession(m_src_proc_info[fid].m_pars_sessions[0]);
			if (m_src_proc_info[fid].m_pars_sessions[1] != 0)
				m_curr_reader->DeleteParsingSession(m_src_proc_info[fid].m_pars_sessions[1]);
		}

		cnt_dumped++;
	}

	if (cnt_dumped == 0)
		m_helper_console.HandleTrace(L"None of the source files is marked for processing.", TGenericConsole::ErrorHighlightMode);
	else wprintf(L"Source files involved: %d\r\n", cnt_dumped);

	m_curr_fid = -1;
	wprintf(L"\r\n");
}

void TCpanHelperBase::DumpSourceFileName(THtmlDocument &rprt)
{
	assert(m_curr_reader != NULL);
	assert((int)m_curr_fid >= TFileInfo::DiskFileIdsBase && (int)m_curr_fid < m_src_proc_info.NumItems());

	// Pick up the name of the file from the database.
	bool file_res = m_curr_reader->GetFileInformation(0, m_curr_fid);
	assert(file_res == TRUE);

	// Emit simple body element. This will be just a name of the file, although possibly escaped.
	TSimpleHtmlElement src_file_name(NULL, m_curr_reader->FileData().info.file_name);
	rprt.EmitElement(src_file_name);
}

void TCpanHelperBase::DumpSourceFileBody(THtmlDocument &rprt)
{
	assert(m_curr_reader != NULL);
	assert((int)m_curr_fid >= TFileInfo::DiskFileIdsBase && (int)m_curr_fid < m_src_proc_info.NumItems());

	// Pick up the info about the session that was processing the requested file.
	ID pars_sess_id = m_src_proc_info[m_curr_fid].m_pars_sessions[0];
	bool sess_res = m_curr_reader->GetCompilationInfo(pars_sess_id);
	assert(sess_res == TRUE);

	THtmlSourceCodeStyle code_style(L"/Images/SrcView");
	code_style.PrepareStdStyles();

	// Instantiate and emit the decorated source file.
	TCpanHtmlSourceCodeFile src_file(m_src_proc_info[m_curr_fid].m_pars_sessions[1]);
	if (src_file.SetupSourceFile(m_helper_console, code_style, m_curr_reader, NULL, pars_sess_id, m_curr_reader->CompilationData().major_src_enter_evt) == TRUE)
	{
		// The file view is prepared. Look for the biggest gap between the guarded lines if any.
		long num_lines = src_file.NumLines();
		long biggest_gap = 0, biggest_gap_beg = -1;
		TStructsArray<long, 16, 256> &guarged_lines = m_src_proc_info[m_curr_fid].m_line_numbers;
		if (num_lines > 0 && guarged_lines.NumItems() > 0)
		{
			// The file is not empty and the list of the guarded lines is available.
			long prev_guarded = 0;
			for (int iline=0; iline<guarged_lines.NumItems(); ++iline)
			{
				 // Space in front of the first guarded line is not garded, but ignore this space if the first
				 // guarded line is relatively small to avoid removing part of the file head comments.
				if (iline != 0 || guarged_lines[iline] > 80)
				{
					long curr_gap = guarged_lines[iline]-prev_guarded-1;
					if (curr_gap > 0 && curr_gap > biggest_gap)
					{
						biggest_gap = curr_gap;
						biggest_gap_beg = prev_guarded+1;
					}
				}

				prev_guarded = guarged_lines[iline];
			}

			long final_gap = num_lines-prev_guarded;
			if (final_gap > 0 && final_gap > biggest_gap)
			{
				biggest_gap = final_gap;
				biggest_gap_beg = prev_guarded+1;
			}
		}

		// Apply one of the three possible emitting procedures.
		if (num_lines > 0 && m_enable_src_files_bandwidth_saving == TRUE && biggest_gap >= 2*m_num_framing_lines+m_min_lines_to_remove)
		{
			// Apply the bandwidth saving procedure. This will also satisfy the legal censoring if it is enabled.
			assert(guarged_lines.NumItems() > 0);

			long prev_guarded = 0, prev_dumped = 0;
			for (int iline=0; iline<guarged_lines.NumItems(); ++iline)
			{
				long curr_gap = guarged_lines[iline]-(prev_guarded+1);
				if (curr_gap >= 2*m_num_framing_lines+m_min_lines_to_remove)
					GenLinesRangeAndGap(rprt, src_file, prev_dumped, prev_guarded+1, curr_gap);
				prev_guarded = guarged_lines[iline];
			}

			long final_gap = num_lines-prev_guarded;
			if (final_gap >= 2*m_num_framing_lines+m_min_lines_to_remove)
				GenLinesRangeAndGap(rprt, src_file, prev_dumped, prev_guarded+1, final_gap);

			assert(prev_dumped != 0);
			if (prev_dumped < num_lines)
				src_file.EmitLinesRange(rprt, prev_dumped+1, num_lines-prev_dumped, FALSE, TRUE);
		}
		else if (num_lines > 0 && m_enable_src_files_legal_censoring == TRUE)
		{
			// At least something should be skipped. Preset the values to conseal the whole file.
			long skip_beg = 1, skip_end = num_lines;

			if (biggest_gap >= 14)
			{
				// A big gap is existing. Remove 10 lines from the middle of the gap.
				skip_beg = biggest_gap_beg+(biggest_gap-10)/2;
				skip_end = skip_beg+9;
			}
			else if (biggest_gap > 0)
			{
				// The biggest gap is relatively small. Remove half of the available gap.
				skip_beg = biggest_gap_beg+biggest_gap/4;
				skip_end = skip_beg+biggest_gap/2;
			}
			else if (num_lines >= 4)
			{
				// There are no gaps. Remove 2 lines from the middle of the file.
				skip_beg = num_lines/2;
				skip_end = skip_beg+1;
			}

			if (skip_beg > 1)
				src_file.EmitLinesRange(rprt, 1, skip_beg-1, TRUE, FALSE);

			GenSkippedLinesRange(rprt, src_file, skip_beg, skip_end);

			if (skip_end < num_lines)
				src_file.EmitLinesRange(rprt, skip_end+1, num_lines-skip_end, FALSE, TRUE);
		}
		else
		{
			// Emit the whole body of the file.
			rprt.EmitElement(src_file);
		}
	}
}

void TCpanHelperBase::GenSingleFileNameCell(THtmlDocument &rprt, const wchar_t *show_src_file_script_name, TCpanNameUseInfosArray &locations, int &iloc, int psess_inx, THtmlCrLfMode crlf_mode)
{
	assert(m_curr_reader != NULL);

	if (iloc >= locations.NumItems())
	{
		// The list of locations is already processed. Emit an empty cell.
		TSimpleHtmlElement empty_cell(L"td");
		rprt.EmitElement(empty_cell);
		return;
	}

	// Figure out the range of events that belong to the same parsing session.
	ID pars_sess_id = locations[iloc].evt_info.pars_id;
	int iloc_after = iloc;
	for(;;)
	{
		iloc_after++;
		if (iloc_after >= locations.NumItems() || locations[iloc_after].evt_info.pars_id != pars_sess_id)
			break;
	}

	// Open the cell element.
	TSimpleHtmlElement cell(L"td", NULL, 0, L"arp_file_name", crlf_mode);
	rprt.EmitElementBeg(cell);

	// Pick up the info about the major source file.
	ID mj_src_fid = 0;
	const wchar_t *mj_src_fname = L"{-Error-Getting-The-File-Name-}";
	if (m_curr_reader->GetCompilationInfo(pars_sess_id) == TRUE)
	{
		mj_src_fid = m_curr_reader->CompilationData().major_src_doc_id.file_id;
		mj_src_fname = m_curr_reader->CompilationData().major_src_file_name;
	}

	// Emit the name of the file element and the delimiter element.
	TSimpleHtmlElement file_name_element(NULL, mj_src_fname);
	TSimpleHtmlElement delim_element(NULL, ((iloc_after-iloc) > 1) ? L", lines " : L", line ");
	rprt.EmitElements(file_name_element, delim_element);

	// Mark this file for processing.
	m_src_proc_info[mj_src_fid].m_pars_sessions[psess_inx] = pars_sess_id;

	// Emit the list of the line numbers.
	for (int inx = iloc; inx<iloc_after; ++inx)
	{
		if (inx != iloc)
		{
			TSimpleHtmlElement comma(NULL, L", ");
			rprt.EmitElement(comma);
		}

		// Pick up the source area of the current event.
		long line_num = -1;
		TSourceLocation source_loc;
		if (m_curr_reader->GetSourceLocationInfo(source_loc, pars_sess_id, locations[inx].evt_info.evt_id) == TRUE)
			line_num = m_curr_reader->GetSourceAreaLineBeg(source_loc.src_area);

		wchar_t buff_link[80];
		swprintf(buff_link, 80, L"%s?fid=%ld&ln=%ld", show_src_file_script_name, mj_src_fid, line_num);

		// Emit a hyperlink with the line number as its body.
		wchar_t line_num_buff[40];
		THyperLinkHtmlElement line_num_element(buff_link, _itow(line_num, line_num_buff, 10), -1, L"arp_line_num", crlm_none);
		line_num_element.SetLinkTarget(L"CsaSrc");
		rprt.EmitElement(line_num_element);

		// Check emit the type of the use.
		DWORD use_type_cat = locations[inx].use_type;
		if (use_type_cat != 0)
		{
			wchar_t cat_inx_body[40];
			swprintf(cat_inx_body, 40, L"(%lu)", use_type_cat);
			TSimpleHtmlElement cat_inx(L"span", cat_inx_body, -1, L"arp_cpu_line", crlm_none);
			rprt.EmitElement(cat_inx);
		}

		if (line_num > 0)
		{
			// Add this line number to the list of important lines.
			int ibefore = m_src_proc_info[mj_src_fid].m_line_numbers.NumItems();
			for (int inx=0; inx<ibefore; ++inx)
			{
				if (line_num < m_src_proc_info[mj_src_fid].m_line_numbers[inx])
				{
					ibefore = inx;
					break;
				}
			}

			if (m_src_proc_info[mj_src_fid].m_line_numbers.InsertItem(ibefore, line_num) == FALSE)
				m_helper_console.HandleTrace(L"TCpanHelperBase: Low on memory while storying the line number.", TGenericConsole::ErrorHighlightMode);
		}
	}

	TSimpleHtmlElement dot(NULL, L".");
	rprt.EmitElement(dot);

	// Close the opened cell.
	rprt.EmitElementEnd(cell);

	// Give out the updated index.
	iloc = iloc_after;
}

void TCpanHelperBase::GenLinesRangeAndGap(THtmlDocument &rprt, THtmlSourceCodeFile &src_file, long &prev_dumped_line, long curr_gap_beg, long curr_gap_len)
{
	assert(curr_gap_beg > prev_dumped_line);
	long line_to_skip_beg = curr_gap_beg+m_num_framing_lines;
	long line_to_skip_end = curr_gap_beg+curr_gap_len-m_num_framing_lines;

	src_file.EmitLinesRange(rprt, prev_dumped_line+1, line_to_skip_beg-(prev_dumped_line+1), (prev_dumped_line == 0), FALSE);
	GenSkippedLinesRange(rprt, src_file, line_to_skip_beg, line_to_skip_end);
	prev_dumped_line = line_to_skip_end;
}

void TCpanHelperBase::GenSkippedLinesRange(THtmlDocument &rprt, THtmlSourceCodeFile &src_file, long skipped_beg, long skipped_end)
{
	assert(skipped_beg > 0 && skipped_beg <= skipped_end);

	wchar_t msg_buff[80];
	if (skipped_beg != skipped_end)
		swprintf(msg_buff, 80, L"Lines %d ... %d are skipped.", skipped_beg, skipped_end);
	else swprintf(msg_buff, 80, L"Line %d is skipped.", skipped_beg);

	src_file.EmitSpecialInsert(rprt, (skipped_beg > 1) ? skipped_beg-1 : skipped_beg, L"arp_skipped_lines", msg_buff);
}

// -----------------------------------------------------------------------------
//	================  TCpanDefnListsIntersectionHelper  ==================
// -----------------------------------------------------------------------------

void TCpanDefnListsIntersectionHelper::WriteHtmlReportBody(THtmlDocument &rprt)
{
	// Open the main table.
	TSimpleHtmlElement main_table(L"table");
	rprt.EmitElementBeg(main_table);

	// Add a header row with the explanation of the columns on top.
	WriteTableHeaderRow(rprt);

	int cnt1 = m_ds1.GetNumMacroNames();
	wprintf(L"Sorting the first database (%d item%s) ...\r\n", cnt1, PluralSuffix(cnt1));
	m_ds1.SortNamesAlphabetically();

	int cnt2 = m_ds2.GetNumMacroNames();
	wprintf(L"Sorting the second database (%d item%s) ...\r\n", cnt2, PluralSuffix(cnt2));
	m_ds2.SortNamesAlphabetically();

	// Both lists are sorted. Look for the intersection between them.
	wprintf(L"Saving the intersection to: %s ...\r\n\r\n", rprt.FileName());
	int cnt_defns = 0;
	TListIter<TCpanMacroNameObject> iter1(m_ds1.m_macro_names), iter2(m_ds2.m_macro_names);
	while (iter1 == TRUE && iter2 == TRUE)
	{
		int res = wcscmp(iter1.CurrItem().Name(), iter2.CurrItem().Name());
		if (res < 0)
		{
			++iter1;
		}
		else if (res == 0)
		{
			// Current name is present in both databases.
			if ((cnt_defns % 50) == 49)
			{
				TSimpleHtmlElement small_row(L"tr", NULL, 0, L"arp_tiny_row");
				rprt.EmitElement(small_row);
				WriteTableHeaderRow(rprt);
			}

			// Dump info about the current defn and shift both iterators.
			WriteDefnInfoRows(rprt, ++cnt_defns, iter1.CurrItem().Name(), iter1.CurrItem().m_definitions, iter2.CurrItem().m_definitions);
			++iter1;
			++iter2;
		}
		else
		{
			++iter2;
		}
	}

	if (cnt_defns == 0)
	{
		// Add an explanation row to the table to make the situation clear.
		WriteEmptyTableMessage(rprt, L"6", L"An intersection between the lists of definitions is empty.", TRUE);
		wprintf(L"The intersection is empty.\r\n\r\n");
	}
	else
	{
		// The HTML table is not empty.
		wprintf(L"Complete. Intersection length: %d\r\n\r\n", cnt_defns);
	}

	// Close the main table.
	rprt.EmitElementEnd(main_table);
}

void TCpanDefnListsIntersectionHelper::WriteTableHeaderRow(THtmlDocument &rprt)
{
	// Allocate simple headers for the first 4 columns.
	TAdvancedHtmlElement hdr_count(L"td", L"Cnt", -1, L"arp_table_header");
	TSimpleHtmlElement hdr_name(L"td", L"Name", -1, L"arp_table_header");
	TSimpleHtmlElement hdr_cnt1(L"td", L"Redefs in Db1", -1, L"arp_table_header");
	TSimpleHtmlElement hdr_cnt2(L"td", L"Redefs in Db2", -1, L"arp_table_header");
	hdr_count.SetCssStyle(L"text-align:right;");

	// The header of the fifth column should contain an invisible bitmap in its body.
	TArrayBasedHtmlContainer hdr_pad(L"td", L"arp_table_header", crlm_before_and_after_object);
	TImageHtmlElement empty_rect(L"/Images/MainFrame/TranspRect.GIF");
	hdr_pad.AppendElement(&empty_rect);

	// Last 2 columns have simple headers.
	TSimpleHtmlElement hdr_fnames1(L"td", L"Files with definitions in Db1", -1, L"arp_table_header");
	TSimpleHtmlElement hdr_fnames2(L"td", L"Files with definitions in Db2", -1, L"arp_table_header");

	// Allocate, assemble and emit the row object.
	TArrayBasedHtmlContainer header_row(L"tr");
	header_row.AppendElements(&hdr_count, &hdr_name, &hdr_cnt1, &hdr_cnt2, &hdr_pad, &hdr_fnames1, &hdr_fnames2);
	rprt.EmitElement(header_row);
}

void TCpanDefnListsIntersectionHelper::WriteDefnInfoRows(THtmlDocument &rprt, int cnt_defns, const wchar_t *defn_name, TCpanNameUseInfosArray &defn_locs1, TCpanNameUseInfosArray &defn_locs2)
{
	// Both lists of locations should be not empty.
	assert(defn_locs1.NumItems() > 0 && defn_locs2.NumItems() > 0);

	int isubrow = 0;
	int iloc1 = 0, iloc2 = 0;
	while (iloc1 < defn_locs1.NumItems() || iloc2 < defn_locs2.NumItems())
	{
		TSimpleHtmlElement intersection_row(L"tr");
		rprt.EmitElementBeg(intersection_row);

		if (isubrow == 0)
		{
			// Emit general info about the name.
			wchar_t buff_cnt[40], buff1[40], buff2[40];
			TSimpleHtmlElement cell_count(L"td", _itow(cnt_defns, buff_cnt, 10), -1, L"arp_objs_cnt");
			TSimpleHtmlElement cell_name(L"td", defn_name, -1, L"arp_object_name");
			TSimpleHtmlElement cell_cnt1(L"td", _itow(defn_locs1.NumItems(), buff1, 10), -1, L"arp_num_uses");
			TSimpleHtmlElement cell_cnt2(L"td", _itow(defn_locs2.NumItems(), buff2, 10), -1, L"arp_num_uses");
			rprt.EmitElements(cell_count, cell_name, cell_cnt1, cell_cnt2);
		}
		else
		{
			// Emit an empt cell to balance the name cells.
			TAdvancedHtmlElement empty_cell(L"td");
			empty_cell.SetFirstAppAttr(L"colspan", L"4");
			rprt.EmitElement(empty_cell);
		}

		// Emit an empty column to the set the subsequent columns apart.
		TSimpleHtmlElement cell_pad(L"td", NULL, -1, L"arp_padding_big");
		rprt.EmitElement(cell_pad);

		// Emit the file name and line numbers in that file.
		GenSingleFileNameCell(rprt, L"Src_Disect.php", defn_locs1, iloc1, 0);
		GenSingleFileNameCell(rprt, L"Src_Disect.php", defn_locs2, iloc2, 0);

		rprt.EmitElementEnd(intersection_row);
		isubrow++;
	}
}

// -----------------------------------------------------------------------------
//	===============  TCpanCondParamsClassificationHelper  ================
// -----------------------------------------------------------------------------

void TCpanCondParamsClassificationHelper::WriteHtmlReportBody(THtmlDocument &rprt)
{
	int cnt = m_ds.GetNumMacroNames();
	wprintf(L"Sorting the database (%d item%s) ...\r\n", cnt, PluralSuffix(cnt));
	m_ds.SortNamesAlphabetically();

	// Do the classification and dump the lists at the same time.
	wprintf(L"Writing the report ...\r\n");
	DumpFreeCondParsParams(rprt);
	DumpDoubleDefPreventerParams(rprt);
	DumpOtherNonLiteralConstParams(rprt);
	DumpLiteralConstParameters(rprt);
	wprintf(L"\r\n");
}

void TCpanCondParamsClassificationHelper::WriteSummaryReportBody(THtmlDocument &rprt)
{
	// This method expects that classification procedure is already carried out.
	TSimpleHtmlElement summary_table(L"table");
	rprt.EmitElementBeg(summary_table);

	// Allocate the header cell elements.
	TSimpleHtmlElement hdr_cat_name(L"td", L"Category name", -1, L"arp_table_header");
	TSimpleHtmlElement hdr_num_params(L"td", L"Num params", -1, L"arp_table_header");

	// Write the header row.
	TArrayBasedHtmlContainer header_row(L"tr");
	header_row.SetCssStyle(L"height:2em;");
	header_row.AppendElements(&hdr_cat_name, &hdr_num_params);
	rprt.EmitElement(header_row);

	int all_params = m_cnt_free_params + m_cnt_dbl_def_preventers + m_cnt_other_params + m_cnt_literal_consts;
	assert(all_params == m_ds.GetNumMacroNames());

	// Write the info rows.
	WriteSummaryReportRow(rprt, L"#free_params_table", L"Free conditional parameters", m_cnt_free_params, L"padding-top:2px;");
	WriteSummaryReportRow(rprt, L"#dbl_def_preventers_table", L"Double definition preventers", m_cnt_dbl_def_preventers);
	WriteSummaryReportRow(rprt, L"#other_params_table", L"Other conditional parameters", m_cnt_other_params);
	WriteSummaryReportRow(rprt, L"#literal_consts_table", L"Literal constant parameters", m_cnt_literal_consts);
	WriteSummaryReportRow(rprt, NULL, L"All parameters", all_params);

	rprt.EmitElementEnd(summary_table);
}

void TCpanCondParamsClassificationHelper::DumpFreeCondParsParams(THtmlDocument &rprt)
{
	WriteCategoryTextHeader(rprt, L"free_params_table", L"Free conditional compilation parameters");

	TSimpleHtmlElement main_table(L"table");
	rprt.EmitElementBeg(main_table);
	WriteParamsTableHeader(rprt, FALSE);

	assert(m_cnt_free_params == 0);
	for (TListIter<TCpanMacroNameObject> iter(m_ds.m_macro_names); iter; ++iter)
	{
		TCpanMacroNameObject &mno = iter.CurrItem();
		if (mno.m_category_assigned == TRUE)
			continue;

		// The category is not assigned yet.
		if (mno.m_key_info.key_type == kt_name && mno.m_cond_param_uses.NumItems() > 0 && mno.m_definitions.NumItems() == 0)
		{
			// Current name was used as a cond param and it is not defined anywhere in the code.
			WriteOrdinaryParamInfo(rprt, ++m_cnt_free_params, mno);
			mno.m_category_assigned = TRUE;
		}
	}

	if (m_cnt_free_params == 0)
	{
		WriteEmptyTableMessage(rprt, L"8", L"Free conditional compilation params are not available.");
	}

	rprt.EmitElementEnd(main_table);
	rprt.WriteLine();
}

void TCpanCondParamsClassificationHelper::DumpDoubleDefPreventerParams(THtmlDocument &rprt)
{
	WriteCategoryTextHeader(rprt, L"dbl_def_preventers_table", L"Double definition preventer conditional compilation params");

	TSimpleHtmlElement main_table(L"table");
	rprt.EmitElementBeg(main_table);
	WriteParamsTableHeader(rprt, FALSE);

	assert(m_cnt_dbl_def_preventers == 0);
	for (TListIter<TCpanMacroNameObject> iter(m_ds.m_macro_names); iter; ++iter)
	{
		TCpanMacroNameObject &mno = iter.CurrItem();
		if (mno.m_category_assigned == TRUE)
			continue;

		// The category is not assigned yet.
		if (mno.m_key_info.key_type == kt_name && mno.m_cond_param_uses.NumItems() > 0)
		{
			// The criteria below is a little bit fuzzy. Leave it in this state for now.
			int num_defs = mno.m_definitions.NumItems();
			if (num_defs == 1 && mno.m_cnt_def_operation_uses == 1 || num_defs > 0 && wcsstr(mno.m_key_info.key_name, L"_DEFINED") != NULL)
			{
				WriteOrdinaryParamInfo(rprt, ++m_cnt_dbl_def_preventers, mno);
				mno.m_category_assigned = TRUE;
			}
		}
	}

	if (m_cnt_dbl_def_preventers == 0)
	{
		WriteEmptyTableMessage(rprt, L"8", L"The double definition preventers are not available.");
	}

	rprt.EmitElementEnd(main_table);
	rprt.WriteLine();
}

void TCpanCondParamsClassificationHelper::DumpOtherNonLiteralConstParams(THtmlDocument &rprt)
{
	WriteCategoryTextHeader(rprt, L"other_params_table", L"Other conditional compilation parameters");

	TSimpleHtmlElement main_table(L"table");
	rprt.EmitElementBeg(main_table);
	WriteParamsTableHeader(rprt, FALSE);

	assert(m_cnt_other_params == 0);
	for (TListIter<TCpanMacroNameObject> iter(m_ds.m_macro_names); iter; ++iter)
	{
		TCpanMacroNameObject &mno = iter.CurrItem();
		if (mno.m_category_assigned == TRUE)
			continue;

		// The category is not assigned.
		if (mno.m_key_info.key_type == kt_name)
		{
			WriteOrdinaryParamInfo(rprt, ++m_cnt_other_params, mno);
			mno.m_category_assigned = TRUE;
		}
	}

	if (m_cnt_other_params == 0)
	{
		WriteEmptyTableMessage(rprt, L"8", L"Other conditional compilation parameters are not available.");
	}

	rprt.EmitElementEnd(main_table);
	rprt.WriteLine();
}

void TCpanCondParamsClassificationHelper::DumpLiteralConstParameters(THtmlDocument &rprt)
{
	WriteCategoryTextHeader(rprt, L"literal_consts_table", L"Literal constant parameters");

	TSimpleHtmlElement main_table(L"table");
	rprt.EmitElementBeg(main_table);
	WriteParamsTableHeader(rprt, TRUE);

	assert(m_cnt_literal_consts == 0);
	for (TListIter<TCpanMacroNameObject> iter(m_ds.m_macro_names); iter; ++iter)
	{
		TCpanMacroNameObject &mno = iter.CurrItem();
		if (mno.m_category_assigned == TRUE)
			continue;

		if (mno.m_key_info.key_type != kt_name)
		{
			WriteLiteralConstParamInfo(rprt, ++m_cnt_literal_consts, mno);
			mno.m_category_assigned = TRUE;
		}
	}

	if (m_cnt_literal_consts == 0)
	{
		WriteEmptyTableMessage(rprt, L"8", L"The literal constant params are not available.");
	}

	rprt.EmitElementEnd(main_table);
	rprt.WriteLine();
}

void TCpanCondParamsClassificationHelper::WriteCategoryTextHeader(THtmlDocument &rprt, const wchar_t *href_to_table, const wchar_t *category_name)
{
	// Write a <br> HTML element that is marked with an element id.
	TAdvancedHtmlElement sect_break(L"br");
	sect_break.SetElementId(href_to_table);
	rprt.EmitElement(sect_break);

	// Whrite a text header with the name of the category.
	TSimpleHtmlElement sect_header(L"h4", category_name);
	rprt.EmitElement(sect_header);
}

void TCpanCondParamsClassificationHelper::WriteParamsTableHeader(THtmlDocument &rprt, bool lit_consts_table)
{
	TArrayBasedHtmlContainer header_row(L"tr");

	// Allocate headers for the first 2 columns.
	TAdvancedHtmlElement hdr_count(L"td", L"Cnt", -1, L"arp_table_header");
	TAdvancedHtmlElement hdr_name(L"td", L"Name", -1, L"arp_table_header");
	hdr_count.SetCssStyle(L"text-align:right;width:30px;");
	hdr_name.SetCssStyle(L"min-width:220px;");
	header_row.AppendElements(&hdr_count, &hdr_name);

	// Allocate counter headers for ordinary conditional parameter. These objects should
	// be allocated regardless if they will be used later or not.
	TArrayBasedHtmlContainer hdr_cnt1(L"td", L"arp_table_header", crlm_before_and_after_object);
	TArrayBasedHtmlContainer hdr_cnt2(L"td", L"arp_table_header", crlm_before_and_after_object);
	TArrayBasedHtmlContainer hdr_cnt3(L"td", L"arp_table_header", crlm_before_and_after_object);
	TArrayBasedHtmlContainer hdr_cnt4(L"td", L"arp_table_header", crlm_before_and_after_object);
	TArrayBasedHtmlContainer hdr_cnt5(L"td", L"arp_table_header", crlm_before_and_after_object);

	TSimpleHtmlElement hdr_cnt1_inx(L"span", L"(1)", -1, L"arp_cpu_index", crlm_none);
	TSimpleHtmlElement hdr_cnt2_inx(L"span", L"(2)", -1, L"arp_cpu_index", crlm_none);
	TSimpleHtmlElement hdr_cnt3_inx(L"span", L"(3)", -1, L"arp_cpu_index", crlm_none);
	TSimpleHtmlElement hdr_cnt4_inx(L"span", L"(4)", -1, L"arp_cpu_index", crlm_none);
	TSimpleHtmlElement hdr_cnt5_inx(L"span", L"(5)", -1, L"arp_cpu_index", crlm_none);

	TSimpleHtmlElement hdr_cnt1_body(NULL, L"Simple Definitions");
	TSimpleHtmlElement hdr_cnt2_body(NULL, L"Complex Definitions");
	TSimpleHtmlElement hdr_cnt3_body(NULL, L"Undefinitions");
	TSimpleHtmlElement hdr_cnt4_body(NULL, L"Arithm Operation Uses");
	TSimpleHtmlElement hdr_cnt5_body(NULL, L"Defined Operation Uses");

	hdr_cnt1.AppendElements(&hdr_cnt1_inx, &hdr_cnt1_body);
	hdr_cnt2.AppendElements(&hdr_cnt2_inx, &hdr_cnt2_body);
	hdr_cnt3.AppendElements(&hdr_cnt3_inx, &hdr_cnt3_body);
	hdr_cnt4.AppendElements(&hdr_cnt4_inx, &hdr_cnt4_body);
	hdr_cnt5.AppendElements(&hdr_cnt5_inx, &hdr_cnt5_body);

	// Allocate counter header for literal const param. These objects should be allocated
	// regardless if they will be used later or not.
	TArrayBasedHtmlContainer hdr_lit_const_cnt6(L"td", L"arp_table_header", crlm_before_and_after_object);
	TSimpleHtmlElement hdr_lit_const_cnt6_inx(L"span", L"(6)", -1, L"arp_cpu_index", crlm_none);
	TSimpleHtmlElement hdr_lit_const_cnt6_body(NULL, L"Arithmetic Operation Uses");

	hdr_lit_const_cnt6.SetCssStyle(L"text-align:right;");
	hdr_lit_const_cnt6.SetFirstAppAttr(L"colspan", L"5");
	hdr_lit_const_cnt6.AppendElements(&hdr_lit_const_cnt6_inx, &hdr_lit_const_cnt6_body);

	// Add appropriate columns.
	if (lit_consts_table == FALSE)
		header_row.AppendElements(&hdr_cnt1, &hdr_cnt2, &hdr_cnt3, &hdr_cnt4, &hdr_cnt5);
	else header_row.AppendElement(&hdr_lit_const_cnt6);

	// Allocate an empty column header.
	TArrayBasedHtmlContainer hdr_pad(L"td", L"arp_table_header", crlm_before_and_after_object);
	TImageHtmlElement empty_rect(L"/Images/MainFrame/TranspRect.GIF");
	hdr_pad.AppendElement(&empty_rect);
	header_row.AppendElement(&hdr_pad);

	// Allocate the final column header.
	TAdvancedHtmlElement hdr_control(L"td", L"Control&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;", -1, L"arp_table_header");
	hdr_control.SetCssStyle(L"padding-left:3px;");
	hdr_control.SetBodyEmitMode(rdem_no_std_escaping);
	header_row.AppendElement(&hdr_control);

	// Emit the row object.
	rprt.EmitElement(header_row);
}

void TCpanCondParamsClassificationHelper::CheckWriteHeaderRow(THtmlDocument &rprt, int cnt_objects, bool lit_consts_table)
{
	if (cnt_objects != 1 && (cnt_objects % 70) == 1)
	{
		TSimpleHtmlElement small_row(L"tr", NULL, 0, L"arp_tiny_row");
		rprt.EmitElement(small_row);
		WriteParamsTableHeader(rprt, lit_consts_table);
	}
}

void TCpanCondParamsClassificationHelper::WriteOrdinaryParamInfo(THtmlDocument &rprt, int cnt_objects, TCpanMacroNameObject &mno)
{
	assert(mno.m_cnt_literal_const_uses == 0);
	CheckWriteHeaderRow(rprt, cnt_objects, FALSE);

	TSimpleHtmlElement info_row(L"tr");
	rprt.EmitElementBeg(info_row);

	// Write cells with the count of objects and with the object name.
	wchar_t buff_cnt[40];
	TSimpleHtmlElement cell_count(L"td", _itow(cnt_objects, buff_cnt, 10), -1, L"arp_objs_cnt");
	TSimpleHtmlElement cell_name(L"td", mno.Name(), -1, L"arp_object_name");
	rprt.EmitElements(cell_count, cell_name);

	// Write cells with counts of use types.
	wchar_t buff_use1[40], buff_use2[40], buff_use3[40], buff_use4[40], buff_use5[40];
	TSimpleHtmlElement cell_cnt1(L"td", NumUsesText(mno.m_cnt_simple_defs, buff_use1), -1, NumUsesStyle(mno.m_cnt_simple_defs));
	TSimpleHtmlElement cell_cnt2(L"td", NumUsesText(mno.m_cnt_complex_defs, buff_use2), -1, NumUsesStyle(mno.m_cnt_complex_defs));
	TSimpleHtmlElement cell_cnt3(L"td", NumUsesText(mno.m_cnt_undef_stmts, buff_use3), -1, NumUsesStyle(mno.m_cnt_undef_stmts));
	TSimpleHtmlElement cell_cnt4(L"td", NumUsesText(mno.m_cnt_arithm_operand_uses, buff_use4), -1, NumUsesStyle(mno.m_cnt_arithm_operand_uses));
	TSimpleHtmlElement cell_cnt5(L"td", NumUsesText(mno.m_cnt_def_operation_uses, buff_use5), -1, NumUsesStyle(mno.m_cnt_def_operation_uses));
	rprt.EmitElements(cell_cnt1, cell_cnt2, cell_cnt3, cell_cnt4, cell_cnt5);

	// Write the padding cell and the control cell and close the row.
	wchar_t buff_locs_name[80];
	swprintf(buff_locs_name, 80, L"cprm%d_locs", mno.m_key_info.key_id);
	WritePadddingAndControlCells(rprt, buff_locs_name);
	rprt.EmitElementEnd(info_row);

	// Write invisible rows with use locations.
	WriteUseLocationsInfo(rprt, buff_locs_name, mno);
}

void TCpanCondParamsClassificationHelper::WriteLiteralConstParamInfo(THtmlDocument &rprt, int cnt_objects, TCpanMacroNameObject &mno)
{
	assert(mno.m_cnt_simple_defs == 0 && mno.m_cnt_complex_defs == 0 && mno.m_cnt_undef_stmts == 0);
	assert(mno.m_cnt_arithm_operand_uses == 0 && mno.m_cnt_def_operation_uses == 0);
	assert(mno.m_cnt_literal_const_uses > 0);
	CheckWriteHeaderRow(rprt, cnt_objects, TRUE);

	TSimpleHtmlElement info_row(L"tr");
	rprt.EmitElementBeg(info_row);

	// Write cells with the name of the object and with the count of its uses.
	wchar_t buff_cnt_objs[40], buff_cnt_uses[40];
	TSimpleHtmlElement cell_count(L"td", _itow(cnt_objects, buff_cnt_objs, 10), -1, L"arp_objs_cnt");
	TSimpleHtmlElement cell_name(L"td", mno.Name(), -1, L"arp_object_name");
	TAdvancedHtmlElement cell_cnt1(L"td", _itow(mno.m_cnt_literal_const_uses, buff_cnt_uses, 10), -1, L"arp_num_uses");
	cell_cnt1.SetFirstAppAttr(L"colspan", L"5");
	rprt.EmitElements(cell_count, cell_name, cell_cnt1);

	// Write the padding cell and the control cell and close the row.
	wchar_t buff_locs_name[80];
	swprintf(buff_locs_name, 80, L"lit_const%d_locs", mno.m_key_info.key_id);
	WritePadddingAndControlCells(rprt, buff_locs_name);
	rprt.EmitElementEnd(info_row);

	// Write invisible rows with use locations.
	WriteUseLocationsInfo(rprt, buff_locs_name, mno);
}

void TCpanCondParamsClassificationHelper::WritePadddingAndControlCells(THtmlDocument &rprt, const wchar_t *loc_rows_name)
{
	// Generate small padding cell.
	TSimpleHtmlElement padding_cell(L"td", NULL, 0, L"arp_padding_small", crlm_before_whole_obj);
	rprt.EmitElement(padding_cell);

	// Generate cell with a single "span" element inside.
	TArrayBasedHtmlContainer control_cell(L"td", NULL, crlm_after_whole_obj);
	TAdvancedHtmlElement control_span(L"span", L"[+] Show use locations", -1, L"csa_sh_control", crlm_none);
	control_span.SetFirstAppAttr(L"LocsRow", loc_rows_name);
	control_cell.AppendElement(&control_span);
	rprt.EmitElement(control_cell);
}

void TCpanCondParamsClassificationHelper::WriteUseLocationsInfo(THtmlDocument &rprt, const wchar_t *loc_rows_name, TCpanMacroNameObject &mno)
{
	// Open an invisible table row with and an element id.
	TAdvancedHtmlElement locs_row(L"tr");
	locs_row.SetElementId(loc_rows_name);
	locs_row.SetCssStyle(L"display:none");
	rprt.EmitElementBeg(locs_row);

	// Allocate two cells that will horizontally span threw the whole table.
	TAdvancedHtmlElement locs_cell1(L"td"), locs_cell2(L"td");
	locs_cell1.SetFirstAppAttr(L"colspan", L"2");
	locs_cell2.SetFirstAppAttr(L"colspan", L"7");
	rprt.EmitElement(locs_cell1);
	rprt.EmitElementBeg(locs_cell2);

	// The contents of the second cell is a HTML table.
	TSimpleHtmlElement inner_table(L"table");
	rprt.EmitElementBeg(inner_table);
	WriteUseLocations(rprt, mno.m_definitions, 1);
	WriteUseLocations(rprt, mno.m_cond_param_uses, 0);

	// Finalize the list of locations with a small row that will separate the current locs from the next object.
	TSimpleHtmlElement small_row(L"tr", NULL, 0, L"arp_tiny_row");
	rprt.EmitElement(small_row);
	rprt.EmitElementEnd(inner_table);

	// Close the second cell and the bigger row.
	rprt.EmitElementEnd(locs_cell2);
	rprt.EmitElementEnd(locs_row);
}

void TCpanCondParamsClassificationHelper::WriteUseLocations(THtmlDocument &rprt, TCpanNameUseInfosArray &locations, int psess_inx)
{
	int iloc = 0;
	while (iloc < locations.NumItems())
	{
		// Generate one row for all locations in the same file.
		TSimpleHtmlElement fname_row(L"tr", NULL, 0, NULL, crlm_before_and_after_object);
		rprt.EmitElementBeg(fname_row);
		GenSingleFileNameCell(rprt, m_show_src_scipt_name, locations, iloc, psess_inx, crlm_none);
		rprt.EmitElementEnd(fname_row);
	}
}

void TCpanCondParamsClassificationHelper::WriteSummaryReportRow(THtmlDocument &rprt, const wchar_t *href_to_table, const wchar_t *category_name, int num_params, const wchar_t *extra_style)
{
	TArrayBasedHtmlContainer cell_cat_name(L"td", L"arp_category_name", crlm_before_and_after_object);
	cell_cat_name.SetCssStyle(extra_style);

	THyperLinkHtmlElement link_cat_name(href_to_table, category_name);
	TSimpleHtmlElement msg_cat_name(NULL, category_name);

	if (href_to_table != NULL)
		cell_cat_name.AppendElement(&link_cat_name);
	else cell_cat_name.AppendElement(&msg_cat_name);

	wchar_t buff_num_params[40];
	TAdvancedHtmlElement cell_num_params(L"td", _itow(num_params, buff_num_params, 10), -1, L"arp_num_uses_bold");
	cell_num_params.SetCssStyle(extra_style);

	TArrayBasedHtmlContainer summary_table_row(L"tr");
	summary_table_row.AppendElements(&cell_cat_name, &cell_num_params);
	rprt.EmitElement(summary_table_row);
}

// -----------------------------------------------------------------------------
//	======================  TCpanProcessor  =======================
// -----------------------------------------------------------------------------

void TCpanProcessor::CompareDefinitionSets(const wchar_t *root_dir1, const wchar_t *root_dir2, const wchar_t **list_of_fname_exts)
{
	TDateTime dt_start = CurrDateTime();
	TMidLevRamLogger mldb(&m_cpan_proc_console);
	TCondParsAnalysisDataStorage cpan_info1, cpan_info2;

	// Collect macro names from both subdirectories.
	bool res1 = CollectMacroNames(TCpanMidLevScanCbkHandler::scp_macro_defns_list, mldb, cpan_info1, root_dir1, list_of_fname_exts);
	bool res2 = CollectMacroNames(TCpanMidLevScanCbkHandler::scp_macro_defns_list, mldb, cpan_info2, root_dir2, list_of_fname_exts);

	// Allocate and init the report helper object.
	TCpanDefnListsIntersectionHelper rprt_hlpr(m_cpan_proc_console, cpan_info1, cpan_info2);
	if (rprt_hlpr.PrepareHelper(mldb) == FALSE)
		return;

	// Generate the main HTML report and the source files.
	rprt_hlpr.WriteHtmlReport(L"OutFiles", L"DefnsIntersectionReport", TRUE);
	rprt_hlpr.DumpSourceFiles(L"OutFiles\\DefnsIsectSources", TRUE, TRUE);

	wchar_t duration_buffer[80];
	wprintf(L"Overall duration:\t %s\r\n\r\n", FormatDuration(CurrDateTime()-dt_start, duration_buffer, 80));

	// Do the cleanup.
	rprt_hlpr.CleanupHelper(mldb);
}

void TCpanProcessor::ClassifyCondParsParams(const wchar_t *report_fnames_core, const wchar_t *root_directory, const wchar_t **list_of_fname_exts)
{
	TDateTime dt_start = CurrDateTime();
	wchar_t fname_buffer[80];

	TMidLevRamLogger mldb(&m_cpan_proc_console);
	TCondParsAnalysisDataStorage cpan_info;

	// Do the scanning in two passes. The first pass will collect the cond pars params while ignoring the #define/#undef
	// statements. Then the second pass will collect the #define/#undef statements only for those names that were collected
	// during the first pass. This approach significantly reduces the memory foot print.
	bool res1 = CollectCondParams(mldb, cpan_info, root_directory, list_of_fname_exts, NULL);
	bool res2 = CollectMacroNames(TCpanMidLevScanCbkHandler::scp_cond_pars_params, mldb, cpan_info, root_directory, list_of_fname_exts);

	// Allocate and init the report helper object.
	TCpanCondParamsClassificationHelper rprt_hlpr(m_cpan_proc_console, cpan_info);
	if (rprt_hlpr.PrepareHelper(mldb) == FALSE)
		return;

	// Generate the main HTML report. This will also do the categorization.
	swprintf(fname_buffer, 80, L"CondParamsClassification_%s_Report", report_fnames_core);
	swprintf(rprt_hlpr.m_show_src_scipt_name, sizeof(rprt_hlpr.m_show_src_scipt_name)/sizeof(wchar_t), L"Src_%s.php", report_fnames_core);
	rprt_hlpr.WriteHtmlReport(L"OutFiles", fname_buffer, TRUE);

	// Summarize the dumped lists above.
	wprintf(L"%-14s:\t %d\r\n", L"Free CondParsParams", rprt_hlpr.m_cnt_free_params);
	wprintf(L"%-14s:\t %d\r\n", L"DoubleDefPrev Params", rprt_hlpr.m_cnt_dbl_def_preventers);
	wprintf(L"%-14s:\t %d\r\n", L"Other CondParsParams", rprt_hlpr.m_cnt_other_params);
	wprintf(L"%-14s:\t %d\r\n", L"LiteralConst Params", rprt_hlpr.m_cnt_literal_consts);
	wprintf(L"\r\n");

	// Generate secondary HTML report and the source file dumps.
	swprintf(fname_buffer, 80, L"CondParamsClassification_%s_Summary", report_fnames_core);
	THtmlReportHelper::HtmlBodyWriterMethod writer = (THtmlReportHelper::HtmlBodyWriterMethod)&TCpanCondParamsClassificationHelper::WriteSummaryReportBody;
	rprt_hlpr.WriteHtmlReport(L"OutFiles", fname_buffer, TRUE, writer);

	// Generate the source files.
	swprintf(fname_buffer, 80, L"OutFiles\\CondParam_%s_Sources", report_fnames_core);
	rprt_hlpr.DumpSourceFiles(fname_buffer, TRUE, TRUE);

	wchar_t duration_buffer[80];
	wprintf(L"Overall duration:\t %s\r\n\r\n", FormatDuration(CurrDateTime()-dt_start, duration_buffer, 80));

	// Do the cleanup.
	rprt_hlpr.CleanupHelper(mldb);
}

int TCpanProcessor::DumpAllCondParsParams(TCondParsAnalysisDataStorage &ds)
{
	ds.SortNamesAlphabetically();
	wprintf(L"\r\nAll cond pars params.\r\n\r\n");

	int cnt = 0;
	for (TListIter<TCpanMacroNameObject> iter(ds.m_macro_names); iter; ++iter)
	{
		if (iter.CurrItem().m_cond_param_uses.NumItems() > 0)
			DumpSingleCondParsParam(++cnt, iter.CurrItem());
	}

	wprintf(L"\r\n");
	return(cnt);
}

void TCpanProcessor::DumpSingleCondParsParam(int cnt, TCpanMacroNameObject &mno)
{
	int cnt_redefs = mno.m_definitions.NumItems();
	int cnt_cond_pars_uses = mno.m_cond_param_uses.NumItems();

	wchar_t buff_uses_redefs[80];
	if (cnt_redefs == 0)
		swprintf(buff_uses_redefs, 80, L"(%d use%s)", cnt_cond_pars_uses, PluralSuffix(cnt_cond_pars_uses));
	else swprintf(buff_uses_redefs, 80, L"(%d use%s, %d redef%s)", cnt_cond_pars_uses, PluralSuffix(cnt_cond_pars_uses), cnt_redefs, PluralSuffix(cnt_redefs));

	wprintf(L" %6d. %-54s %-28s\t", cnt, mno.Name(), buff_uses_redefs);

	if (mno.m_cnt_literal_const_uses > 0)
		wprintf(L" %d literal_const_use%s", mno.m_cnt_literal_const_uses, PluralSuffix(mno.m_cnt_literal_const_uses));
	if (mno.m_cnt_arithm_operand_uses > 0)
		wprintf(L" %d arithmetic_operand_use%s", mno.m_cnt_arithm_operand_uses, PluralSuffix(mno.m_cnt_arithm_operand_uses));
	if (mno.m_cnt_def_operation_uses > 0)
		wprintf(L" %d defined_operation_use%s", mno.m_cnt_def_operation_uses, PluralSuffix(mno.m_cnt_def_operation_uses));

	wprintf(L"\r\n");
}


bool TCpanProcessor::CollectMacroNames(TCpanMidLevScanCbkHandler::TScanPurpose scan_type, TMidLevRamLogger &logger, TCondParsAnalysisDataStorage &ds, const wchar_t *root_directory, const wchar_t **list_of_fname_exts)
{
	// Setup the scanning props.
	TMidLevScannerOptions scan_opts;
	scan_opts.Clear();
	scan_opts.built_in_macros_fid = -1;				// Do not load any built in conditions file.
	scan_opts.disab_include_stmts = TRUE;
	scan_opts.disab_cond_pars_stmts = TRUE;
	scan_opts.disab_misc_prepr_stmts = TRUE;
	scan_opts.disab_macro_calls = TRUE;

	bool res = RunTheSession(scan_type, scan_opts, logger, ds, root_directory, list_of_fname_exts, L"CollectMacroNames");

	if (scan_type == TCpanMidLevScanCbkHandler::scp_macro_defns_list)
		wprintf(L"MacroNamesDetected:\t %lu\r\n\r\n", ds.GetNumMacroNames());
	else if (scan_type == TCpanMidLevScanCbkHandler::scp_cond_pars_params)
		wprintf(L"CondParamsWithDefinitionsDetected:\t %lu\r\n\r\n", ds.GetNumObjectsWithDefns());

	return(res);
}

bool TCpanProcessor::CollectCondParams(TMidLevRamLogger &logger, TCondParsAnalysisDataStorage &ds, const wchar_t *root_directory, const wchar_t **list_of_fname_exts,
											const wchar_t *expandable_macros_file_name)
{
	// Setup the scanning props. Note that cleaning the options structure sets the mode of processing
	// of the unknown names to "ceum_condit", i.e. sets the cond compilation status of the code below
	// the statement to "conditional".
	TMidLevScannerOptions scan_opts;
	scan_opts.Clear();
	scan_opts.built_in_macros_fid = -1;				// Do not load any built in conditions file.
	scan_opts.disab_define_stmts = TRUE;
	scan_opts.disab_include_stmts = TRUE;
	scan_opts.disab_misc_prepr_stmts = TRUE;

	bool res = RunTheSession(TCpanMidLevScanCbkHandler::scp_cond_pars_params, scan_opts, logger, ds, root_directory, list_of_fname_exts, L"CollectCondParams");
	wprintf(L"CondParamsDetected:\t %lu\r\n\r\n", ds.GetNumMacroNames());
	return(res);
}

bool TCpanProcessor::RunTheSession(TCpanMidLevScanCbkHandler::TScanPurpose scan_type, TMidLevScannerOptions &scan_opts, TMidLevRamLogger &logger, TCondParsAnalysisDataStorage &ds,
									const wchar_t *root_directory, const wchar_t **list_of_fname_exts, const wchar_t *sess_name)
{
	// Instantiate special callback handler that will wrap the passed logger.
	TCpanMidLevScanCbkHandler cpan_logger(&logger);
	cpan_logger.SetupAnalysisTask(scan_type, &ds);

	// Use generic parser and driver. There is notihng special here.
	TGenericParser parser;
	TParsingDriver driver(parser, cpan_logger, &scan_opts);
	driver.Scanner().SetupTraceMode(sctr_main_phase | sctr_err_warn, FALSE);

	// Do the parsing.
	TBatchParsingSummary sumry;
	wprintf(L"TCpanProcessor::%s: Startup.\r\n\r\n", sess_name);
	driver.ProcessSubtreeFilesWithParsing(0, lang_cpp, root_directory, TRUE, list_of_fname_exts, NULL, NULL, NULL, FALSE, FALSE, FALSE, &sumry);
	wprintf(L"\r\nTCpanHelper::%s: Complete.\r\n\r\n", sess_name);

	// Show the summary of results.
	wprintf(L"DirsProcessed:\t %lu\r\n", sumry.dirs_processed);
	wprintf(L"FilesProcessed:\t %lu\r\n", sumry.files_processed);

	if (sumry.num_errors != 0)
		wprintf(L"NumErrors:\t %lu\r\n", sumry.num_errors);
	if (sumry.num_warnings != 0)
		wprintf(L"NumWarnings:\t %lu\r\n", sumry.num_warnings);
	if (sumry.run_time_errors != 0)
		wprintf(L"RunTimeErrors:\t %lu\r\n", sumry.run_time_errors);

	wchar_t buffer[80];
	wprintf(L"Duration:\t %s\r\n", FormatDuration(sumry.batch_duration, buffer, 80));
	wprintf(L"\r\n");

	return(sumry.num_errors == 0 && sumry.run_time_errors == 0);
}


