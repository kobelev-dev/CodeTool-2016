//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "TextScan/GrammarStructure.H"
#include  "LangView/SyntaxHighlight.H"

// ----------------------------------------------------------------------------
//  ==================  TSourceViewDecorOwner  =====================
// ----------------------------------------------------------------------------

TSourceViewDecorOwner::TSourceViewDecorOwner(TSrcViewDecorOwnerType otp)
{
	// Owner type should not allow multiple instantiations except for the code area.
	assert(otp <= svdo_code_area || otp >= svdo_cmdl_lexema);

	m_type = otp;
	ClearEventData();
}

TSourceViewDecorOwner::TSourceViewDecorOwner(TSrcViewDecorOwnerType otp, TSourceAreaInfo &area, CtvParsingEventInfo &event)
{
	assert(otp >= svdo_code_area && otp <= svdo_parsing_event);

	m_type = otp;
	m_src_area = area;
	m_event_data = event;
}

void TSourceViewDecorOwner::EmitSpecificAttrs(THtmlDocument &doc)
{
	wchar_t buff_app_id[80];
	if (m_type == svdo_buffering_area)
	{
		// This is paste buffering area. Show an id of the intro event.
		swprintf(buff_app_id, 80, L"%ld.%ld", m_event_data.hdr.parsing_id, m_event_data.hdr.event_id);
		doc.EmitAttribute(L"BuffArea", buff_app_id);
	}
	else if (m_type == svdo_parsing_event || m_type == svdo_macro_hdr_object)
	{
		if (m_event_data.hdr.event_type != pset_cpp_item_ref)
		{
			// This is a preprocessing layer event, a syntax error or a warning message.
			swprintf(buff_app_id, 80, L"%ld.%ld", m_event_data.hdr.parsing_id, m_event_data.hdr.event_id);
			doc.EmitAttribute(L"MidLev", buff_app_id);
		}
		else
		{
			if (m_event_data.cpp_item.pars_sess_cpp_item_id != 0)
			{
				// Local parsing session Cpp object reference.
				swprintf(buff_app_id, 80, L"%ld.%ld", m_event_data.hdr.parsing_id, m_event_data.cpp_item.pars_sess_cpp_item_id);
				doc.EmitAttribute(L"Cps", buff_app_id);
			}

			if (m_event_data.cpp_item.project_level_cpp_item_id != 0)
			{
				// Global project Cpp object reference.
				swprintf(buff_app_id, 80, L"%ld", m_event_data.cpp_item.project_level_cpp_item_id);
				doc.EmitAttribute(L"Cpr", buff_app_id);
			}
		}
	}
}

// ---------------------------------------------------------------------------
//  ==============  TSourceViewDecorOwnersContext  ===================
// ---------------------------------------------------------------------------

TSourceViewDecorOwnersContext::TSourceViewDecorOwnersContext(int &cnt_owner_creation_errors)
	: m_owner_creation_errors(cnt_owner_creation_errors), m_out_of_scope_code_owner(svdo_out_of_scope_code),
	m_aborted_scan_code_owner(svdo_aborted_scan_code), m_unscanned_code_owner(svdo_unscanned_code),
	m_cmdl_lexemas_owner(svdo_cmdl_lexema), m_clang_lexemas_owner(svdo_clang_lexema),
	m_curr_scan_pos_owner(svdo_curr_scan_pos), m_macro_header_owner(svdo_macro_hdr_object)
{
	m_latest_decor_owner = NULL;
}

void TSourceViewDecorOwnersContext::ResetContext()
{
	m_decor_owners.DeleteAll();
	m_latest_decor_owner = NULL;
	m_macro_header_owner.ClearEventData();
}

TSourceViewDecorOwner *TSourceViewDecorOwnersContext::CreateDecorOwner(TSrcViewDecorOwnerType owner_type, TSourceAreaInfo &area, CtvParsingEventInfo &event)
{
	// Verify the type of the owner. It should allow multiple instances.
	assert(owner_type == svdo_code_area || owner_type == svdo_buffering_area || owner_type == svdo_parsing_event);

	TSourceViewDecorOwner *owner = new TSourceViewDecorOwner(owner_type, area, event);
	if (owner == NULL)
	{
		// Creation of the owner failed.
		m_owner_creation_errors++;
		return(NULL);
	}

	//
	//  List of decorations should be ordered by the beginnings of the src areas of the owner objects.
	//

	if (m_latest_decor_owner == NULL)
	{
		// This means that the list is currently empty.
		m_decor_owners.AppendItem(owner);
		m_latest_decor_owner = owner;
	}
	else
	{
		// Find out a better direction for look.
		int res = CompareDecorOwners(owner, m_latest_decor_owner);
		TSourceViewDecorOwner *curr = m_latest_decor_owner;
		if (res > 0)
		{
			// Search for the right place by moving to the bottom of the file.
			for(;;)
			{
				TSourceViewDecorOwner *next = (TSourceViewDecorOwner*)m_decor_owners.GetNext(curr);
				if (next == NULL)
				{
					m_decor_owners.AppendItem(owner);
					break;
				}
				else if (CompareDecorOwners(owner, next) < 0)
				{
					m_decor_owners.InsertBefore(owner, next);
					break;
				}

				curr = next;
			}
		}
		else
		{
			// Search for the right place moving to the top of the file.
			for(;;)
			{
				TSourceViewDecorOwner *prev = (TSourceViewDecorOwner*)m_decor_owners.GetPrev(curr);
				if (prev == NULL || CompareDecorOwners(owner, prev) >= 0)
				{
					m_decor_owners.InsertBefore(owner, curr);
					break;
				}

				curr = prev;
			}
		}

		// Save the recent pointer. Most likely the next item will be inserted somewhere around it.
		m_latest_decor_owner = owner;
	}

	// Success.
	return(owner);
}

TSourceViewDecorOwner *TSourceViewDecorOwnersContext::GetInactiveAreaDecorOwner(TSrcViewWholeFileStyleInx style_inx)
{
	switch (style_inx)
	{
		case wfsi_out_of_scope:	return(&m_out_of_scope_code_owner);
		case wfsi_aborted_scan:	return(&m_aborted_scan_code_owner);
		case wfsi_unscanned:		return(&m_unscanned_code_owner);
	}

	assert(FALSE);
	return(NULL);
}

int TSourceViewDecorOwnersContext::CompareDecorOwners(const TSourceViewDecorOwner *owner1, const TSourceViewDecorOwner *owner2)
{
	// Compare the beginnings of the source areas.
	if (owner1->m_src_area.AreaBeg() > owner2->m_src_area.AreaBeg())
		return(1);
	else if (owner1->m_src_area.AreaBeg() < owner2->m_src_area.AreaBeg())
		return(-1);

	// Compare the types of the objects.
	if (owner1->m_type > owner2->m_type)
		return(1);
	else if (owner1->m_type < owner2->m_type)
		return(-1);

	// Compare the long ids of events.
	return(CtvEventHeader::CompareHeaders(owner1->m_event_data.hdr, owner2->m_event_data.hdr));
}

// ---------------------------------------------------------------------------
//  ====================  TSyntaxHighlighter  =======================
// ---------------------------------------------------------------------------

TSyntaxHighlighter::TSyntaxHighlighter(TSyntaxHighlightDocumentBuilder &doc_builder, TSourceViewDecorOwnersContext &decor_owners)
		: m_doc_builder(doc_builder), m_decor_owners(decor_owners)
{
	m_mldb_reader = NULL;
	m_active_scanner = NULL;

	m_lex_src_call.Clear();
	m_src_file.Clear();
	m_src_file_data_owned = FALSE;
}

TSyntaxHighlighter::~TSyntaxHighlighter()
{
	if (m_src_file_data_owned == TRUE)
	{
		// Release the file data.
		m_src_file.ReleaseBasicFileInfo();
		m_src_file.ReleaseLinesInfo();
	}
}

void TSyntaxHighlighter::DecorateStandAloneFile(TFileInfo &file_info)
{
	// Ensure that this object was not used before.
	assert(m_lex_src_call.hdr.parsing_id == 0 && m_lex_src_call.hdr.event_id == 0);
	assert(m_src_file.file_id == 0 && m_src_file.info.file_name == NULL);

	// Accept passed file without picking up the ownership.
	m_src_file = file_info;

	// Active area should cover the whole file.
	TSourceAreaInfo active_area = { file_info.pars_id, file_info.file_id, 0, m_src_file.info.file_len };

	// Highlight the raw syntax in the file if possible.
	if (RAW_SCAN_CMDL_STYLE(m_src_file.rscn_type) == TRUE)
	{
		// This is file with the command line syntax.
		SetupCmdlRawSyntaxDecors(active_area, 0, TParseEnvInfo::FindParseEnvForRawScanningType(m_src_file.rscn_type), TRUE);
	}
	else if (RAW_SCAN_CLANG_STYLE(m_src_file.rscn_type) == TRUE)
	{
		// This is C/C++ source file, grammar definition file or a file with the C# syntax.
		SetupClangRawSyntaxDecors(active_area, TRUE);
	}
}

bool TSyntaxHighlighter::LoadLexScrEnclosureFile(CtvLexSourceEnterEvent &lex_src_call, TMidLevStoreReader *mldb_reader, TMidLevScanner *active_scanner)
{
	// Ensure that this object was not used before.
	assert(m_lex_src_call.hdr.parsing_id == 0 && m_lex_src_call.hdr.event_id == 0);
	assert(m_src_file.file_id == 0 && m_src_file.info.file_name == NULL);
	assert(m_src_file_data_owned == FALSE);

	// Accept the passed parameters.
	m_lex_src_call = lex_src_call;
	m_mldb_reader = mldb_reader;
	m_active_scanner = active_scanner;

	// Pick up and check the id of the file that should be displayed.
	TIdStruct src_doc_id;
	src_doc_id.doc_id = lex_src_call.GetEnclosureDocId();
	if (src_doc_id.file_id == 0)
	{
		ReportProblem(L"TSyntaxHighlighter: Error setting up the source file for the lex source enter event %ld.%ld.", lex_src_call.hdr.parsing_id, lex_src_call.hdr.event_id);
		return(FALSE);
	}

	// The id of the source file is available. Look for the file itself.
	if (mldb_reader->CheckFileInfoPresence(src_doc_id.pars_id, src_doc_id.file_id) == FALSE)
	{
		// Database does not contain information about this file. Check the scanner.
		if (active_scanner != NULL && active_scanner->CbkHandler().GetParsingId() == lex_src_call.hdr.parsing_id)
		{
			// The active scanner is available and it is running the right session.
			TFileInfoItem *scanner_file_item = active_scanner->FindCachedFile(src_doc_id.pars_id, src_doc_id.file_id);
			if (scanner_file_item != NULL)
			{
				// This file is available in the scanner. Duplicate its name and body to allow displaying this file
				// in case if the scanner will be released before closing the viewer.
				if (m_src_file.CloneFileNameAndBodyAndLinesInfo(scanner_file_item->data) == FALSE)
				{
					ReportProblem(L"TSyntaxHighlighter: Unable to duplicate the file data for: \"%s\", FileLen=%ld, NumLines=%ld.",
								scanner_file_item->data.info.file_name, scanner_file_item->data.info.file_len, scanner_file_item->data.NumLines());
					return(FALSE);
				}

				// The file data is owned now.
				m_src_file_data_owned = TRUE;
			}
		}
	}

	if (m_src_file.IsNull() == TRUE)
	{
		// Look for this file in the database. Ask the database to return the body of the file.
		if (mldb_reader->GetFileInformation(src_doc_id.pars_id, src_doc_id.file_id, TRUE) == FALSE)
		{
			// An error is already displayed.
			return(FALSE);
		}

		// Pick up the prepared data. The file name, body and lines info are already cached in the file reader object.
		m_src_file = mldb_reader->FileData();
	}

	// Pick up the borders of the lex src area as they were sketched at the point of entering the lex src enclosure.
	long src_start = lex_src_call.GetEnclosureSrcStart();
	long src_end = lex_src_call.GetEnclosureSrcEnd();

	if (LEX_SRC_MACRO_OR_MPARAM(lex_src_call.src_type) == TRUE && src_end == 0)
	{
		// This is rare case when the paste buffer is not exited yet. This means that the actual length of
		// the lex src is not known yet. Use the length of the file as it was retrieved from the scanner.
		assert(lex_src_call.evt_exit_id == 0 && m_src_file_data_owned == TRUE);
		src_end = m_src_file.info.file_len;
	}

	// Setup the active area info. Later on this info can be updated when the inactive areas will be set up.
	m_encl_active_area.doc_id = m_src_file.doc_id;
	m_encl_active_area.area_beg = src_start;
	m_encl_active_area.area_len = src_end-src_start;
	m_encl_code_stts_area = m_encl_active_area;
	m_encl_whole_line_active_area_end = lex_src_call.WholeLineLexSrcStyle();

	// Success.
	return(TRUE);
}

void TSyntaxHighlighter::ExtractFileData(TFileInfo &file_info, bool &file_data_owned)
{
	// The implementation of this method is pretty straightforward.
	file_info = m_src_file;
	file_data_owned = m_src_file_data_owned;
	m_src_file_data_owned = FALSE;
}

void TSyntaxHighlighter::ResolveLinesRangeToDisplay(int num_upper_rows, int num_lower_rows, int &line_from, int &num_lines)
{
	// Ensure that the object is prepared for processing the parsed file.
	assert(m_lex_src_call.hdr.parsing_id != 0 && m_lex_src_call.hdr.event_id != 0);
	assert(m_src_file.file_id != 0 && m_src_file.info.file_body != NULL);

	if (line_from > 0)
	{
		// The range of lines to display is given explicitly. Note that the passed number
		// of lines will be used without any verification.
		return;
	}

	// Pick up the line numbers for the borders of the active area.
	long line_start = m_src_file.GetLineNumber(m_encl_active_area.AreaBeg());
	long line_end = m_src_file.GetLineNumber(m_encl_active_area.AreaEnd());
	assert(line_start > 0 && line_end > 0 && line_start <= line_end);

	// Setup the start line.
	line_from = line_start-num_upper_rows;
	if (line_from < 1)
		line_from = 1;

	// Setup the number of lines to display regardless of the value of the passed parameter.
	long display_end = line_end+num_lower_rows;
	if (display_end > m_src_file.NumLines())
		display_end = m_src_file.NumLines();

	num_lines = (display_end-line_from)+1;
}

void TSyntaxHighlighter::DecorateLexSrcEnclosure()
{
	// Show the grey frame around the active area if needed.
	SetupNonActiveAreaDecors();

	// Highlight all mid level events that apply to the active parsing area. Do this before highlighting
	// the paste buffering areas and code status areas because these procedures iterate decor owners
	// instead of reading the parsing events from the database.
	SetupMidLevEventDecors(m_lex_src_call.hdr.parsing_id, m_lex_src_call.hdr.event_id);

	// Show the paste buff backgrounds and the framing icons.
	SetupPasteBuffAreaDecors();

	// Highlight areas of the code according to their conditional status.
	SetupCodeStatusDecors();

	if (m_lex_src_call.src_type == srct_macro)
	{
		// Decorate the header of the macro.
		SetupMacroHeaderDecors();
	}

	// Highlight lexemas in the active parsing area.
	SetupRawSyntaxDecors();
}

void TSyntaxHighlighter::SetupNonActiveAreaDecors()
{
	// This method should be called only for parsed files. Ensure this.
	ID pars_id = m_lex_src_call.hdr.parsing_id;
	assert(pars_id != 0 && m_lex_src_call.hdr.event_id != 0 && m_mldb_reader != NULL);

	// Pick up the borders of the visible area of the file.
	long visible_area_beg, visible_area_len;
	if (m_doc_builder.GetVisibleArea(visible_area_beg, visible_area_len) == FALSE)
	{
		// The situation is bogus. Nothing is visible. Do not highlight anything.
		return;
	}

	// Apply the "out of the parsing scope" areas first. Note that the "out of scope area" is the area that was
	// originally not expected to be scanned when the lex source was entered. In reality the scanned area can
	// be shorter if the parsing sesssion is still running or if it was aborted in the middle for some reason.
	long lower_out_of_scope_area_beg = -1;
	bool lower_out_of_scope_area_whole_line = FALSE;

	// There are about 10 different types of enclosures. Only three of them (srct_macro, srct_mparam, srct_scgm)
	// are not the "whole file". All other enclosure types scan the whole file.
	if (m_lex_src_call.src_type == srct_macro)
	{
		// This type of enclosure has special area that is displayed as active. This area covers the whole
		// macro definition. It is bigger than the area of the body of the macro.
		TSourceLocation whole_macro_area;
		if (m_mldb_reader->GetMacroLibraryChangeEvent(pars_id, m_lex_src_call.mcall_def_id) == FALSE)
		{
			// An error should be already reported.
			return;
		}
		else if (m_mldb_reader->GetSourceLocationInfo(whole_macro_area, m_mldb_reader->EventData(), m_active_scanner) == FALSE)
		{
			// An error should be already reported.
			return;
		}

		// The source location of the macrodef statement was retrieved.
		assert(whole_macro_area.src_area.doc_id == m_encl_active_area.doc_id);

		long code_status_area_beg = m_src_file.AdjustToLineBeg(whole_macro_area.src_area.AreaBeg());
		if (visible_area_beg <= code_status_area_beg)
		{
			// The upper "out-of-scope" area is present.
			m_doc_builder.ApplyFileViewStyleToUpperArea(wfsi_out_of_scope, m_src_file.SkipEndOfLineBackward(code_status_area_beg), TRUE);
		}

		long code_status_area_end = m_src_file.AdjustToLineEnd(whole_macro_area.src_area.AreaEnd());
		if (visible_area_beg+visible_area_len >= code_status_area_end)
		{
			// The lower "out-of-scope" area is present.
			m_doc_builder.ApplyFileViewStyleToLowerArea(wfsi_out_of_scope, m_src_file.SkipEndOfLineForward(code_status_area_end), TRUE);
			lower_out_of_scope_area_beg = code_status_area_end;
			lower_out_of_scope_area_whole_line = TRUE;
		}

		// Save the updated code status area into the data member of the class.
		assert(code_status_area_beg <= code_status_area_end);
		m_encl_code_stts_area.area_beg = code_status_area_beg;
		m_encl_code_stts_area.area_len = code_status_area_end-code_status_area_beg;

		// Macrodefinitions that are only one line high should have brighter line nums bkgr and marks area bkgr.
		m_doc_builder.ApplySpecialNumsMarksBackgrounds(m_encl_code_stts_area.AreaBeg(), m_encl_code_stts_area.AreaEnd(), TRUE);
	}
	else if (m_lex_src_call.src_type == srct_mparam || m_lex_src_call.src_type == srct_scgm)
	{
		// This is an "inline style" enclosure.
		long parsing_beg = m_encl_active_area.AreaBeg();
		long parsing_end = m_encl_active_area.AreaEnd();

		if (visible_area_beg <= parsing_beg)
		{
			// The upper "out-of-scope" area is present.
			m_doc_builder.ApplyFileViewStyleToUpperArea(wfsi_out_of_scope, parsing_beg, FALSE);
		}

		if (visible_area_beg+visible_area_len >= parsing_end)
		{
			// The lower "out-of-scope" area is present.
			m_doc_builder.ApplyFileViewStyleToLowerArea(wfsi_out_of_scope, parsing_end, FALSE);
			lower_out_of_scope_area_beg = parsing_end;
			lower_out_of_scope_area_whole_line = FALSE;
		}

		// Inline active areas should have brighter line nums area and marks area backgrounds.
		m_doc_builder.ApplySpecialNumsMarksBackgrounds(parsing_beg, parsing_end, FALSE);
	}

	// Check if an "aborted_scan" or an "unscanned" areas are existing or not.
	bool sess_running, partial_active_area;
	long real_src_active_end = GetRealActiveAreaEnd(sess_running, partial_active_area);
	if (real_src_active_end >= 0)
	{
		// The data was retrieved.
		if (real_src_active_end < m_encl_active_area.AreaEnd())
		{
			// The unscanned area is not empty. Update the data fields of the class that describe the active area.
			m_encl_active_area.area_len = real_src_active_end-m_encl_active_area.area_beg;
			m_encl_code_stts_area.area_len = real_src_active_end-m_encl_code_stts_area.area_beg;
			m_encl_whole_line_active_area_end = FALSE;

			if (real_src_active_end <= visible_area_beg+visible_area_len)
			{
				// The unscanned area is visible, at least partly.
				TSrcViewWholeFileStyleInx wfsi = (sess_running == FALSE) ? wfsi_aborted_scan : wfsi_unscanned;
				if (lower_out_of_scope_area_beg < 0)
				{
					// The "out of scope" area is not available or it is completely invisible. Show the unscanned
					// area up to the end of the file view.
					m_doc_builder.ApplyFileViewStyleToLowerArea(wfsi, real_src_active_end, FALSE);
				}
				else
				{
					// The unscanned area should cover only the part of the view.
					m_doc_builder.ApplyUnscannedAreaStyle(wfsi, real_src_active_end, lower_out_of_scope_area_beg, lower_out_of_scope_area_whole_line);
				}
			}
		}

		if (sess_running == TRUE && m_lex_src_call.evt_exit_id == 0)
		{
			// This is an unexited area in the active session. Highlight the beginning of the unscanned area.
			m_doc_builder.SetupUnscannedAreaEntranceIcon(real_src_active_end, partial_active_area);
			m_encl_whole_line_active_area_end = FALSE;

			if (real_src_active_end == m_encl_active_area.AreaEnd() && m_lex_src_call.WholeLineLexSrcStyle() == TRUE)
			{
				// The right area of the line after the text should be displayed as unscanned.
				m_doc_builder.SetupUnscannedLineEndArea(real_src_active_end);
			}
		}
	}
}

void TSyntaxHighlighter::SetupMidLevEventDecors(ID pars_sess, ID src_encl_id, PSET_TYPES_MASK events_mask)
{
	// This method should be called only for parsed files. Ensure this.
	assert(pars_sess != 0 && src_encl_id != 0 && m_mldb_reader != NULL);

	// Pick up the secondary store because the passed store will be busy iterating the parsing events.
	TMidLevStoreReader *sec_store = m_mldb_reader->CreateSecondaryStore();
	if (sec_store == NULL)
	{
		ReportProblem(L"TSyntaxHighlighter: Unable to allocate secondary database reader for applying the mid lev event decors.\r\n");
		return;
	}

	// Process events of all passed types except for the lex src exit events.
	events_mask &= ~PSET_TYPE_BIT(pset_lex_source_exit);
	TSourceAreaInfo event_area = { m_src_file.pars_id, m_src_file.file_id };
	long curr_mid_lev_lin_src_delta = -1;

	if (sec_store->StartParsingEventsIteration(pars_sess, events_mask, src_encl_id) == TRUE)
	{
		while (sec_store->NextParsingEvent() == TRUE)
		{
			CtvParsingEventInfo &info = sec_store->EventData();

			/* -BUGBUG-
			if (info.hdr.event_type != pset_cpp_item_ref)
			{
				// Convert linear area of the event, that is stored in its header, into the source area inside the current file.
				event_area.area_beg = info.hdr.area.linear_beg-lin_source_delta;
				event_area.area_len = info.hdr.area.linear_len;

				if (info.hdr.event_type == pset_lex_source_enter && info.src_enter.src_type == srct_file || info.hdr.event_type == pset_cond_parsing_change || info.hdr.event_type == pset_misc_preproc_stmt || info.hdr.event_type == pset_error_warning_event)
				{
					// These events may have lex src calls on their territory. This means that the length of the source area
					// may differ from the length of the linear area. Pick up the source length from the database. Do not check
					// the return code from this method. It is not modifying the return buffer when it fails. In this case the area
					// will be wrong, but there is no other easy solution here.
					long src_beg_before = event_area.area_beg;
					GetLocalSrcAreaInfo(dlg, sec_store, active_scanner, event_area, info.hdr.area, L"WholeMidLevEventDecor", (info.hdr.event_type == pset_error_warning_event) ? TRUE : FALSE);
					assert(event_area.area_beg == src_beg_before);
				}

				// Prepare linear area delta for the next event.
				curr_evt_lin_src_delta = lin_source_delta;
				if (info.hdr.event_type == pset_lex_source_enter)
				{
					if (info.src_enter.evt_exit_id != 0)
					{
						// This lex source enter event was exited. Update the linear offset.
						lin_source_delta = info.src_enter.evt_exit_linear_after_ret-event_area.AreaEnd();
					}
					else
					{
						// Lex src call was not exited. This should be the last event in the iteration.
						// This is ensured with the assert statement at the beginning of the loop.
						lin_source_delta = 0;
					}
				}
			}
			else
			*/

			{
				// Use the laborious method.
				GetLocalSrcAreaInfo(event_area, info.hdr.area, L"CppItemLinkEventDecor", FALSE);

				curr_mid_lev_lin_src_delta = info.hdr.area.linear_beg-event_area.area_beg;
			}

			TSourceViewDecorOwner *owner = m_decor_owners.CreateDecorOwner(svdo_parsing_event, event_area, info);
			if (owner == NULL)
			{
				// An error should be already displayed/recorded.
				continue;
			}

			// Apply various decorations depending on the type of the event.
			DecorateMidLevEvent(owner);
		}

		sec_store->CloseParsingEventsIteration();
	}

	// Finalize the iteration.
	m_mldb_reader->ReleaseSecondaryStore(sec_store);
}

void TSyntaxHighlighter::SetupPasteBuffAreaDecors()
{
	// This method should be called only for parsed files. Ensure this.
	ID pars_id = m_lex_src_call.hdr.parsing_id;
	assert(pars_id != 0 && m_lex_src_call.hdr.event_id != 0 && m_mldb_reader != NULL);

	TStructsArray<TSourceViewDecorOwner*, 32, 128> opened_paste_buff_areas;

	TSourceViewDecorOwner *paste_buff_area_owner = NULL;
	if (m_lex_src_call.paste_buffering_ref != 0)
	{
		// This lex source was entered when the paste buffering was active.
		paste_buff_area_owner = StartPasteBufferingArea(m_encl_active_area.AreaBeg(), m_lex_src_call.paste_buffering_ref);
	}

	// Iterate existing decor owners. Decor owners should be iterated using the while loop because new owners
	// can be added inside the loop and these new owners should not be noticed by the loop.
	TSourceViewDecorOwner *curr_owner = (TSourceViewDecorOwner*)m_decor_owners.m_decor_owners.GetFirst();
	while (curr_owner != NULL)
	{
		CtvParsingEventInfo info;
		info.Clear();
		TSourceAreaInfo event_area = { m_src_file.pars_id, m_src_file.file_id };

		// Take care of the paste buffering state if any.
		if (info.hdr.event_type == pset_paste_buffering_change && info.pbuff_chg.evt_enter_cn == 0)
		{
			if (paste_buff_area_owner != NULL)
			{
				// Current pbuff start event should be the direct child of the current pbuff area.
				if (info.pbuff_chg.parent_buff_ref != paste_buff_area_owner->m_event_data.hdr.event_id)
				{
					ReportProblem(L"Paste buffering area %lu.%lu is not the parent area for the starting area %lu.%lu that has parent area %lu.%lu.\r\n",
								pars_id, paste_buff_area_owner->m_event_data.hdr.event_id, pars_id, info.hdr.event_id, info.pbuff_chg.parent_buff_ref);
				}

				// Fill in the length of existing pbuff area and create new decoration.
				FinalizePasteBufferingArea(paste_buff_area_owner, event_area.AreaBeg());
				paste_buff_area_owner = NULL;
			}

			// Start the new area.
			paste_buff_area_owner = StartPasteBufferingArea(event_area.AreaEnd(), info);
		}
		else if (info.hdr.event_type == pset_paste_buffering_change && info.pbuff_chg.evt_enter_cn != 0)
		{
			// Current pbuff end event should correspond to the current pbuff area that should be not empty.
			if (paste_buff_area_owner == NULL)
			{
				ReportProblem(L"Paste buffering exit event %lu.%lu stays outside of any active paste buffering area.\r\n",
							pars_id, info.hdr.event_id);
			}
			else
			{
				if (paste_buff_area_owner->m_event_data.hdr.event_id != info.pbuff_chg.evt_enter_cn)
				{
					ReportProblem(L"Paste buffering exit event %lu.%lu, evt enter cn %lu, does not correspond to the current paste buffering area %lu.%lu.\r\n",
								pars_id, info.hdr.event_id, info.pbuff_chg.evt_enter_cn, pars_id, paste_buff_area_owner->m_event_data.hdr.event_id);
				}

				// Fill in the length of existing pbuff area and create the decoration.
				FinalizePasteBufferingArea(paste_buff_area_owner, event_area.AreaBeg());
				paste_buff_area_owner = NULL;
			}

			if (info.pbuff_chg.parent_buff_ref != 0)
			{
				// This is exit from the non topmost buffering area.
				assert(info.pbuff_chg.new_buff_level > 0);
				paste_buff_area_owner = StartPasteBufferingArea(event_area.AreaEnd(), info.pbuff_chg.parent_buff_ref);
			}
		}
		else if (info.hdr.event_type == pset_lex_source_enter)
		{
			if (paste_buff_area_owner == NULL)
			{
				if (info.src_enter.paste_buffering_ref != 0)
				{
					ReportProblem(L"Lex src call %lu.%lu has paste buff enter ref mark is equal to %lu\r\nwhile currently there is no active paste buffering area.\r\n",
								pars_id, info.hdr.event_id, info.src_enter.paste_buffering_ref);
				}

				if (info.src_enter.evt_exit_pbuff_ref != 0)
				{
					// Start the new area.
					paste_buff_area_owner = StartPasteBufferingArea(event_area.AreaEnd(), info.src_enter.evt_exit_pbuff_ref);
				}
			}
			else
			{
				if (paste_buff_area_owner->m_event_data.hdr.event_id != info.src_enter.paste_buffering_ref)
				{
					ReportProblem(L"Paste buffering area %lu.%lu extends up the beg of the lex src call %lu.%lu,\r\nwhile the paste buff enter ref mark of this lex src call event is equal to %lu.\r\n",
								pars_id, paste_buff_area_owner->m_event_data.hdr.event_id, pars_id, info.hdr.event_id, info.src_enter.paste_buffering_ref);
				}

				// Process the pbuff exit flag in the lex src call event.
				if (info.src_enter.evt_exit_pbuff_ref == 0)
				{
					// Current pbuff area has ended inside this lex src call.
					FinalizePasteBufferingArea(paste_buff_area_owner, event_area.AreaBeg());
					paste_buff_area_owner = NULL;
				}
				else if (paste_buff_area_owner->m_event_data.hdr.event_id != info.src_enter.evt_exit_pbuff_ref)
				{
					// The paste buff area should be restarted.
					FinalizePasteBufferingArea(paste_buff_area_owner, event_area.AreaEnd());
					paste_buff_area_owner = StartPasteBufferingArea(event_area.AreaEnd(), info.src_enter.evt_exit_pbuff_ref);
				}
			}
		}

		curr_owner = (TSourceViewDecorOwner*)m_decor_owners.m_decor_owners.GetNext(curr_owner);
	}

	if (paste_buff_area_owner != NULL)
	{
		// The lex source was exited with an active paste buffering.
		if (paste_buff_area_owner->m_event_data.hdr.event_id != m_lex_src_call.evt_exit_pbuff_ref)
		{
			ReportProblem(L"Paste buffering area %lu.%lu extends up the end of the lex src enclosure,\r\nwhile the paste buff exit ref mark of the lex src enclosure is equal to %lu.\r\n",
						pars_id, paste_buff_area_owner->m_event_data.hdr.event_id, m_lex_src_call.evt_exit_pbuff_ref);
		}

		// Close the current area.
		FinalizePasteBufferingArea(paste_buff_area_owner, m_encl_active_area.AreaEnd());
		paste_buff_area_owner = NULL;
	}
}

void TSyntaxHighlighter::SetupCodeStatusDecors()
{
	// This method should be called only for parsed files. Ensure this.
	ID pars_id = m_lex_src_call.hdr.parsing_id;
	assert(pars_id != 0 && m_lex_src_call.hdr.event_id != 0 && m_mldb_reader != NULL);

	// Retrieve the "initial code status" event. This is the cond ref parent of the call event.
	if (m_mldb_reader->GetCondParsChgEventAllowNullEventId(pars_id, m_lex_src_call.cond_ref) == FALSE)
	{
		// This is a database corruption. An error should be already displayed.
		return;
	}

	// Move event data out of the database reader object.
	CtvCondParsingChangeEvent initial_code_stt_event = m_mldb_reader->EventData().cond_pars;

	// Procedure for the whole file significantly differs from all other cases. Process this case separately.
	if (LEX_SRC_WHOLE_FILE(m_lex_src_call.src_type) == TRUE)
	{
		// Apply complex procedure that may involve several code status changes.
		ApplyMultiAreaCodeStatusProc(initial_code_stt_event);
	}
	else
	{
		// All other cases should have just one conditional parsing area. Ensure this by iterating all available decor owners.
		for (TListIter<TSourceViewDecorOwner> iter(m_decor_owners.m_decor_owners); iter; ++iter)
		{
			TSourceViewDecorOwner *owner = &iter.CurrItem();
			if (owner->m_type == svdo_parsing_event)
			{
				if (owner->m_event_data.hdr.event_type == pset_cond_parsing_change)
				{
					ReportProblem(L"TSyntaxHighlighter: Unexpected cond parsing change statement.\r\n\r\n"
								L"    ParsingId: \t%lu\r\n    EventId: \t%lu\r\n",
								owner->m_event_data.hdr.parsing_id, owner->m_event_data.hdr.event_id);
				}
				else if (owner->m_event_data.hdr.event_type == pset_lex_source_enter)
				{
					CtvLexSourceEnterEvent &cev = owner->m_event_data.src_enter;

					if (cev.cond_ref != initial_code_stt_event.hdr.event_id)
					{
						ReportProblem(L"TSyntaxHighlighter: Lex source call has bogus value in the cond ref field (%ld).\r\n\r\n"
									L"    ParsingId: \t\t%lu\r\n    LexSrcCallEventId: \t%lu\r\n",
									cev.cond_ref, owner->m_event_data.hdr.parsing_id, owner->m_event_data.hdr.event_id);
					}
					else if (cev.evt_exit_id != 0 && cev.evt_exit_cond_ref != initial_code_stt_event.hdr.event_id)
					{
						ReportProblem(L"TSyntaxHighlighter: Lex source call has changed the cond parsing code status.\r\n\r\n"
									L"    ParsingId: \t\t%lu\r\n    LexSrcCallEventId: \t%lu\r\n",
									owner->m_event_data.hdr.parsing_id, owner->m_event_data.hdr.event_id);
					}
				}
			}
		}

		// Highlight the area.
		ApplySingleAreaCodeStatusProc(initial_code_stt_event);
	}
}

void TSyntaxHighlighter::SetupRawSyntaxDecors()
{
	// This method should be called for highlighting the lex source enclosures.
	if (RAW_SCAN_CMDL_STYLE(m_src_file.rscn_type) == TRUE)
	{
		// File with the command line syntax.
		assert(LEX_SRC_WHOLE_FILE(m_lex_src_call.src_type) == TRUE);
		SetupCmdlRawSyntaxDecors(m_encl_active_area, m_lex_src_call.pos_params_cnt, TParseEnvInfo::FindParseEnvForRawScanningType(m_src_file.rscn_type), FALSE);
	}
	else if (RAW_SCAN_CLANG_STYLE(m_src_file.rscn_type) == TRUE)
	{
		// This is a C/C++ source file, grammar definition or a file with the C# syntax.
		SetupClangRawSyntaxDecors(m_encl_active_area, FALSE, m_lex_src_call.inc_fn_mode);
	}
}

void TSyntaxHighlighter::SetupMacroHeaderDecors()
{
	// This method should be called for highlighting the macrodef call enclosures.
	assert(m_lex_src_call.src_type == srct_macro && m_mldb_reader != NULL);

	// Retrieve information about the macro definition.
	if (m_mldb_reader->GetMacroLibraryChangeEvent(m_lex_src_call.hdr.parsing_id, m_lex_src_call.mcall_def_id) == FALSE)
		return;

	// Retrieve the source location of this definition.
	TSourceLocation whole_macro_area;
	if (m_mldb_reader->GetSourceLocationInfo(whole_macro_area, m_mldb_reader->EventData(), m_active_scanner) == FALSE)
		return;

	// Store the retrieved data.
	assert(whole_macro_area.src_area.doc_id == m_src_file.doc_id);
	CtvMacroLibraryChangeEvent &mdef_info = m_mldb_reader->EventData().mdef_chg;
	m_decor_owners.SetupMacroHeaderInfo(whole_macro_area.src_area, mdef_info);
	long lin_src_delta = whole_macro_area.lin_area.linear_beg-whole_macro_area.src_area.area_beg;

	// Highlight the stmt token and the name of the macro.
	DecorateMacroHdrLexema(mhod_mdef_token, mlfsi_mdef_token, mdef_info.token_area.linear_beg-lin_src_delta, mdef_info.token_area.linear_len);
	DecorateMacroHdrLexema(mhod_mdef_name,  mlfsi_mdef_name,  mdef_info.name_area.linear_beg-lin_src_delta, mdef_info.name_area.linear_len);

	// Macro params area stays in between the end of the name area and the beginning of the body area if
	// the body of the macro is not empty or till the end of the macrodef if the body of the macro is empty.
	long prms_area_beg = mdef_info.name_area.AreaEnd()-lin_src_delta;
	long prms_area_end = ((mdef_info.body_area.linear_len > 0) ? mdef_info.body_area.AreaBeg() : mdef_info.hdr.area.AreaEnd())-lin_src_delta;
	if (prms_area_end > prms_area_beg)
	{
		// Area for the list of the macro params is not empty. Instantiate a raw scanner to scan this area.
		TStringsDict dict;
		TRawScanner scanner(&m_src_file, &dict);
		scanner.Seek(prms_area_beg, prms_area_end-prms_area_beg, TRUE, FALSE);

		TLexema lex;
		int param_inx = 0;
		while (scanner.GetLex(lex) != ltx_eof)
		{
			if (lex.IsKeyword(opr_lpar) == TRUE || lex.IsKeyword(opr_comma) == TRUE || lex.IsKeyword(opr_rpar) == TRUE || lex.IsKeyword(opr_eq) == TRUE)
			{
				// This is one of the delimiters.
				DecorateMacroHdrLexema(mhod_mdef_delim, mlfsi_mdef_delim, lex.origin.src_area.area_beg, lex.origin.src_area.area_len);
			}
			else if (lex.type == ltx_name)
			{
				// This is a name of the macro parameter.
				DecorateMacroHdrLexema(param_inx << mhod_param_inx_shift, mlfsi_mdef_param, lex.origin.src_area.area_beg, lex.origin.src_area.area_len);
				param_inx++;
			}
			else
			{
				// This is come other lexema, most likely this is a comment, but who knows.
				DecorateMacroHdrLexema(mhod_mdef_other, mlfsi_mdef_other, lex.origin.src_area.area_beg, lex.origin.src_area.area_len);
			}
		}
	}
}

// ---------- Support methods for group 1. Non active areas. -------------

long TSyntaxHighlighter::GetRealActiveAreaEnd(bool &sess_running, bool &partial_active_area)
{
	ID pars_id = m_lex_src_call.hdr.parsing_id;
	sess_running = partial_active_area = FALSE;

	if (m_mldb_reader->GetCompilationInfo(pars_id) == FALSE)
	{
		// An error should be already reported.
		return(-1);
	}

	// The parsing session header was retrieved. Ensure that this is not a raw session.
	CtvCompilationInfo sess_hdr = m_mldb_reader->CompilationData();
	assert(sess_hdr.lang_type != lang_none);
	sess_running = (sess_hdr.parsing_status == -1) ? TRUE : FALSE;

	long max_linear_offs = -1;
	if (m_lex_src_call.evt_exit_id != 0)
	{
		// Current lex src enclosure has the lex src exit event in the database. This means that the linear
		// position after the last scanned byte in the current lex source enclosure is known.
		max_linear_offs = m_lex_src_call.evt_exit_linear_pos;
	}
	else
	{
		// The exit event is missing. Try to pick up this info from the database or from the scanner.
		if (sess_hdr.parsing_status > 0)
		{
			// The parsing session was aborted. And even more, for this or that reason the current particular
			// call was not closed with an exit event. Nevertheless the max linear offset can be picked up from
			// the session header.
			max_linear_offs = sess_hdr.summary.final_linear_offs;
		}
		else if (sess_hdr.parsing_status < 0 && m_active_scanner != NULL && m_active_scanner->CbkHandler().GetParsingId() == pars_id)
		{
			// The parsing is currently active, the scanner is available and it is running the right session.
			max_linear_offs = m_active_scanner->GetCurrentLinearOffset();
		}
	}

	if (max_linear_offs <= 0)
	{
		ReportProblem(L"TSyntaxHighlighter: Error getting the linear end of the scanned area for the lex src enclosure %ld.%ld.", pars_id, m_lex_src_call.hdr.event_id);
		return(-1);
	}

	// The linear offset of the end of the scanned area is available.
	long lin_beg = m_lex_src_call.LinearPosAfterEntering();
	assert(max_linear_offs >= lin_beg);

	// Pick up the end of the source area that was actually scanned.
	TSourceLocation buffer;
	TLinearArea scanned_linear_area = { lin_beg, max_linear_offs-lin_beg };
	if (m_mldb_reader->GetSourceLocationInfo(buffer, pars_id, scanned_linear_area, m_active_scanner) == FALSE)
	{
		// An error should be already reported.
		return(-1);
	}

	// The scanned source area was retrieved. This source area should start inside the current enclosure
	// because its starting linear pos belongs to the current enclosure. Ensure this.
	assert(buffer.source_ref == m_lex_src_call.hdr.event_id && buffer.src_area.doc_id == m_encl_active_area.doc_id);
	assert(buffer.src_area.AreaEnd() <= m_encl_active_area.AreaEnd());

	// Success.
	partial_active_area = buffer.partial_src_area;
	return(buffer.src_area.AreaEnd());
}

// ---------- Support methods for group 2. Mid lev events and cpp objects. -------------

static const TSrcViewDecorZorder g_SNHLT_MidLevBkgrStyleInxToZorder[mlbsi_num_types] =
{
	svzo_mpr_stmt_bkgr,			// mlbsi_mpr_stmt_bkgr
	svzo_paste_buff_bkgr,			// mlbsi_paste_buff_area
	svzo_mcall_stmt_bkgr,			// mlbsi_macro_call_area
	svzo_error_warning,			// mlbsi_warning
	svzo_error_warning,			// mlbsi_error
};

void TSyntaxHighlighter::DecorateMidLevEvent(TSourceViewDecorOwner *decor_owner)
{
	// Ensure that passed owner describes the mid lev event in the current file.
	assert(decor_owner != NULL && decor_owner->m_type == svdo_parsing_event);
	assert(decor_owner->m_src_area.doc_id == m_src_file.doc_id);

	// Pick up some props of the event.
	CtvParsingEventInfo &evt_info = decor_owner->m_event_data;
	TSourceAreaInfo &src_area = decor_owner->m_src_area;
	long lin_src_delta = evt_info.hdr.area.linear_beg-src_area.area_beg;

	// Apply different types of decorations to different types of statements.
	switch (evt_info.hdr.event_type)
	{
		case pset_register_drive_subst:
				{
					// This should be a spaceless event. Just mark its location.
					ApplySpacelessDecor(decor_owner, spcdi_drive_subst_event_mark, svho_default, src_area);
				}
				break;

		case pset_register_temp_file:
				{
					// This should be a spaceless event. Just mark its location.
					ApplySpacelessDecor(decor_owner, spcdi_temp_file_reg_mark, svho_default, src_area);
				}
				break;

		case pset_paste_buffering_change:
				{
					/*
					if (evt_info.pbuff_chg.evt_enter_cn == 0)
					{
						// This is entering the paste buff area.
						ApplyMidLevImposedDecor(decor_owner, spcdi_paste_buff_beg_mark, evt_lin_area, lin_src_delta);
					}
					else
					{
						// This is exiting from the paste buff area.
						ApplyMidLevImposedDecor(decor_owner, spcdi_paste_buff_end_mark, evt_lin_area, lin_src_delta);
					}
					*/
				}
				break;

		case pset_lex_source_enter:
				{
					switch (evt_info.src_enter.src_type)
					{
						//
						//  First process calls that should be marked only with the icon. These types of calls are
						//  spaceless except for rare case of immediate processing of the major source file.
						//

						case srct_ant_env:		ApplySpacelessDecor(decor_owner, spcdi_ant_env_call_mark, svho_default, src_area);
												break;
						case srct_bltin_defs:		ApplySpacelessDecor(decor_owner, spcdi_bltin_defs_call_mark, svho_default, src_area);
												break;

						case srct_mj_src:		ApplySpacelessDecor(decor_owner, spcdi_mj_src_call_mark, svho_default, src_area);
												if (src_area.area_len > 0)
												{
													// This is the major source file that was loaded immediately when the pos param
													// was processed. Note that this is not the default mode of processing.
													ApplyMidLevFrgrDecor(decor_owner, mlfsi_cmdl_mj_src_file, src_area);
												}
												break;

						case srct_scgm:			ApplySpacelessDecor(decor_owner, spcdi_scgm_call_mark, svho_default, src_area);
												break;
						case srct_pbuff_macro:
						case srct_pbuff_mparam:	ApplySpacelessDecor(decor_owner, spcdi_paste_buff_call_mark, svho_default, src_area);
												break;

						default:
							{
								if (src_area.area_len == 0)
								{
									// In the normal situation all types of calls that were not handled above should have non empty
									// source area. This is not so. Report this problem and paint at least something.
									ReportProblem(L"TSyntaxHighlighter: Spaceless LexSrcEnter with subtype: %ld.", evt_info.src_enter.src_type);
									ApplySpacelessDecor(decor_owner, spcdi_bad_spaceless_call_mark, svho_default, src_area);
								}
								else
								{
									// The call event has non empty body.
									switch (evt_info.src_enter.src_type)
									{
										case srct_cmdl:
												{
													// This type of source is used for the root cmdl and nested cmdls. The root cmdl is always called
													// from nowhere. This means that this type of call event cannot come here. Nested cmdl is a key
													// parameter. Its key name part is stored in the token area, and its fname part in the fname area.
													ApplyMidLevBkgrDecor(decor_owner, mlbsi_mpr_stmt_bkgr, src_area);

													if (evt_info.src_enter.inc_token_area.IsNull() == FALSE)
														ApplyMidLevFrgrDecor(decor_owner, mlfsi_mpr_stmt_token, evt_info.src_enter.inc_token_area, lin_src_delta);
													if (evt_info.src_enter.inc_fname_area.IsNull() == FALSE)
														ApplyMidLevFrgrDecor(decor_owner, mlfsi_include_fname, evt_info.src_enter.inc_fname_area, lin_src_delta);
												}
												break;

										case srct_file:
												{
													// This type of source is used in the #include stmts and in the cmdl forced include files. It is not used
													// anywhere else. The #include stmt is a whole line statement and the forced include file is a key param.
													// Check the type of source that the viewer is currently displaying (not the type of event under processing)
													// to distinguish between these cases.
													if (LEX_SRC_CMDL(m_lex_src_call.src_type) == TRUE)
														ApplyMidLevBkgrDecor(decor_owner, mlbsi_mpr_stmt_bkgr, src_area);
													else ApplyMidLevWholeLineBkgr(decor_owner, src_area);

													// Show the aux areas if they are present.
													if (evt_info.src_enter.inc_token_area.IsNull() == FALSE)
														ApplyMidLevFrgrDecor(decor_owner, mlfsi_mpr_stmt_token, evt_info.src_enter.inc_token_area, lin_src_delta);
													if (evt_info.src_enter.inc_fname_area.IsNull() == FALSE)
														ApplyMidLevFrgrDecor(decor_owner, mlfsi_include_fname, evt_info.src_enter.inc_fname_area);
												}
												break;

										case srct_macro:
												{
													ApplyMidLevBkgrDecor(decor_owner, mlbsi_macro_call_area, src_area);
													ApplyMidLevFrgrDecor(decor_owner, mlfsi_mcall_macro_name, evt_info.src_enter.mcall_name_area, lin_src_delta);
												}
												break;

										case srct_mparam:
												{
													ApplyMidLevBkgrDecor(decor_owner, mlbsi_macro_call_area, src_area);
													ApplyMidLevFrgrDecor(decor_owner, mlfsi_mcall_mparam_name, src_area);
												}
												break;

										default:
											{
												// Bogus or unexpected lex src call subtype.
												ReportProblem(L"TSyntaxHighlighter: Unexpected LexSrcEnter subtype: %ld.", evt_info.src_enter.src_type);
											}
											break;
									}
								}
							}
					}
				}
				break;

		case pset_macro_library_change:
				{
					// Stmts: #define/#undef. Note that definitions may come either from a regular file or from the command line.
					if (LEX_SRC_CMDL(m_lex_src_call.src_type) == TRUE)
						ApplyMidLevBkgrDecor(decor_owner, mlbsi_mpr_stmt_bkgr, src_area);
					else ApplyMidLevWholeLineBkgr(decor_owner, src_area);

					// The token and name are always present. Do not highlight the body in any way.
					ApplyMidLevFrgrDecor(decor_owner, mlfsi_mpr_stmt_token, evt_info.mdef_chg.token_area, lin_src_delta);
					ApplyMidLevFrgrDecor(decor_owner, mlfsi_macro_def_name, evt_info.mdef_chg.name_area, lin_src_delta);
				}
				break;

		case pset_cond_parsing_change:
				{
					// Stmts: #ifdef/#ifndef/#if/#elif/#else/#endif.
					ApplyMidLevWholeLineBkgr(decor_owner, src_area);
					ApplyMidLevFrgrDecor(decor_owner, mlfsi_mpr_stmt_token, evt_info.cond_pars.stmt_token_area, lin_src_delta);
				}
				break;

		case pset_misc_preproc_stmt:
				{
					// Misc stmts: #pragma/#error/etc.
					switch (evt_info.misc_stmt.mis_event_type)
					{
						case exmis_app_name:
								ApplyMidLevFrgrDecor(decor_owner, mlfsi_cmdl_app_name, src_area);
								break;

						case exmis_mjsrc_area:
								{
									// The whole event area and the file info area are the same. Use the whole event area for simplicity.
									ApplyMidLevFrgrDecor(decor_owner, mlfsi_cmdl_mj_src_file, src_area);
								}
								break;

						case exmis_blocked_stmt:
								ApplyMidLevWholeLineBkgr(decor_owner, src_area);
								ApplyMidLevFrgrDecor(decor_owner, mlfsi_mprep_blocked_stmt, src_area);
								break;

						case exmis_failed_include:
								{
									// This an #include stmt that was not entered.
									ApplyMidLevWholeLineBkgr(decor_owner, src_area);

									if (evt_info.misc_stmt.fname_area.IsNull() == FALSE)
										ApplyMidLevFrgrDecor(decor_owner, mlfsi_mprep_failed_inc_fn, evt_info.misc_stmt.fname_area);
								}
								break;

						case exmis_pragma:
						case exmis_error:
						case exmis_import:
						case exmis_line:
						case exmis_using:
								{
									// These events can come only from the regular C/C++ files and they can have only the whole line style.
									assert(m_lex_src_call.src_type == srct_file || m_lex_src_call.src_type == srct_mj_src);
									ApplyMidLevWholeLineBkgr(decor_owner, src_area);
									ApplyMidLevFrgrDecor(decor_owner, mlfsi_mpr_stmt_token, evt_info.misc_stmt.token_area, lin_src_delta);

									// Show the aux areas if they are present.
									if (evt_info.misc_stmt.line_num_area.IsNull() == FALSE)
										ApplyMidLevFrgrDecor(decor_owner, mlfsi_mprep_other_fields, evt_info.misc_stmt.line_num_area);
									if (evt_info.misc_stmt.fname_area.IsNull() == FALSE)
										ApplyMidLevFrgrDecor(decor_owner, mlfsi_mprep_other_fields, evt_info.misc_stmt.fname_area);
								}
								break;

						default:
							{
								// Bogus or unexpected event subtype.
								ReportProblem(L"TSyntaxHighlighter: Unexpected MiscPreprocStmt subtype: %ld.", evt_info.misc_stmt.mis_event_type);
							}
							break;
					}
				}
				break;

		case pset_new_include_path:
				{
					// There is no direct MidLev statement here. Highlight the backround.
					ApplyMidLevBkgrDecor(decor_owner, mlbsi_mpr_stmt_bkgr, src_area);
				}
				break;

		case pset_error_warning_event:
				{
					// This parsing event does not have an associated  mid lev statement.
					if (evt_info.err_warn.err_class != lerrc_message)
					{
						// Apply the background only when the area is not empty.
						bool isError = LEX_ERROR_CLASS_ERROR(evt_info.err_warn.err_class);
						if (src_area.area_len > 0)
							ApplyMidLevBkgrDecor(decor_owner, (isError == TRUE) ? mlbsi_error_bkgr : mlbsi_warning_bkgr, src_area);

						// The icons are always painted.
						TSrcViewSpacelessDecorInx whln_dcr_inx, inline_dcr_inx;
						whln_dcr_inx = (isError == TRUE) ? spcdi_error_whole_line_mark : spcdi_warning_whole_line_mark;
						inline_dcr_inx = (isError == TRUE) ? spcdi_error_area_end_mark : spcdi_warning_area_end_mark;
						ApplyMarksAreaDecor(decor_owner, whln_dcr_inx, svho_default, src_area);
						ApplySpacelessDecor(decor_owner, inline_dcr_inx, svho_err_warn_mark, src_area);
					}
				}
				break;

		case pset_cond_param_use:
				{
					// Modify the foreground.
					ApplyMidLevFrgrDecor(decor_owner, mlfsi_cond_param_use, src_area);
				}
				break;

		case pset_grammar_section:
				{
					// Modify the foreground.
					ApplyMidLevFrgrDecor(decor_owner, mlfsi_grammar_section, src_area);
				}
				break;

		case pset_grammar_object:
				{
					// This type of event can carry info about several different grammar objects. Only some of them
					// should be highlighted. Other objects are less meaningful when they are shown on the source listing.
					if (evt_info.grm_object.object_sym >= RuleNonTermObject && evt_info.grm_object.object_sym < RuleNonTermObject+(RuleObjectsBase-NonTerminalsBase))
					{
						ApplyMidLevFrgrDecor(decor_owner, mlfsi_grammar_rule_nterm, src_area);
					}
					else if (evt_info.grm_object.object_sym == XpctConflictNameObject)
					{
						ApplyMidLevFrgrDecor(decor_owner, mlfsi_grammar_xpct_name, src_area);
					}
				}
				break;

		case pset_cpp_item_ref:
				{
					// This is a reference to an item in the Cpp database.
					TSrcViewDecorZorder zo = (TSrcViewDecorZorder)(svzo_cpp_bkgrs_base+evt_info.cpp_item.cpp_item_nesting);
					m_doc_builder.ApplyCppObjectBkgrDecor(decor_owner, zo, src_area);

					if (evt_info.cpp_item.ext_src_area.m_cpp_atype != cxa_none)
					{
						// Highlight the non empty area from the event body.
						ApplyCppObjectFrgrDecor(decor_owner, evt_info.cpp_item.ext_src_area);
					}

					if (evt_info.cpp_item.extra_areas_temp_file_id != 0)
					{
						// Load the temp file with an array of additional areas.
						if (m_mldb_reader->GetFileInformation(m_lex_src_call.hdr.parsing_id, evt_info.cpp_item.extra_areas_temp_file_id, TRUE) == TRUE)
						{
							// The file is available.
							TCppSourceArea *cpp_areas = (TCppSourceArea*)m_mldb_reader->FileData().info.file_body;
							int num_areas = m_mldb_reader->FileData().info.file_len*sizeof(wchar_t)/sizeof(TCppSourceArea);
							assert(cpp_areas != NULL && num_areas > 0);

							for (int iarea=0; iarea<num_areas; ++iarea)
								ApplyCppObjectFrgrDecor(decor_owner, cpp_areas[iarea]);
						}
					}
				}
				break;

		default:
			{
				// Bogus or unexpected event type.
				ReportProblem(L"TSyntaxHighlighter: Unexpected MidLevEvent type: %ld.", evt_info.hdr.event_type);
			}
			break;
	}
}

void TSyntaxHighlighter::ApplyMidLevWholeLineBkgr(TSourceViewDecorOwner *decor_owner, TSourceAreaInfo &src_area)
{
	// Convert statement area offsets into the line numbers.
	long line_beg = m_src_file.GetLineNumber(src_area.AreaBeg());
	long line_end = m_src_file.GetLineNumber(src_area.AreaEnd());
	assert(line_beg > 0 && line_end > 0);

	// Call the worker method.
	m_doc_builder.ApplyMidLevWlbkDecor(decor_owner, svzo_mpr_stmt_bkgr, line_beg, line_end-line_beg+1);
}

void TSyntaxHighlighter::ApplyMidLevBkgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewMidLevBkgrStyleInx inx, TSourceAreaInfo &src_area)
{
	m_doc_builder.ApplyMidLevBkgrDecor(decor_owner, g_SNHLT_MidLevBkgrStyleInxToZorder[inx], inx, src_area);
}

void TSyntaxHighlighter::ApplyMidLevFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewMidLevFrgrStyleInx inx, TSourceAreaInfo &src_area)
{
	m_doc_builder.ApplyMidLevFrgrDecor(decor_owner, svzo_mid_lev_frgr, inx, src_area);
}

void TSyntaxHighlighter::ApplyMidLevFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewMidLevFrgrStyleInx inx, TLinearArea &lin_area, long lin_src_delta)
{
	// Caller guarantees that mapping the linear area into the source area is trivial.
	TSourceAreaInfo src_area = { 0, 0, lin_area.linear_beg-lin_src_delta, lin_area.linear_len };
	m_doc_builder.ApplyMidLevFrgrDecor(decor_owner, svzo_mid_lev_frgr, inx, src_area);
}

void TSyntaxHighlighter::ApplyMidLevFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewMidLevFrgrStyleInx inx, TLinearArea &lin_area)
{
	TSourceAreaInfo buffer;
	if (GetLocalSrcAreaInfo(buffer, lin_area, L"MidLevEventFrgrDecor") == TRUE)
		m_doc_builder.ApplyMidLevFrgrDecor(decor_owner, svzo_mid_lev_frgr, inx, buffer);
}

void TSyntaxHighlighter::ApplyCppObjectFrgrDecor(TSourceViewDecorOwner *decor_owner, TCppSourceArea cpp_area)
{
	// All areas in the Cpp item ref events may come from any lex src enclosure. Remapping is always needed.
	TSourceAreaInfo buffer;
	if (GetLocalSrcAreaInfo(buffer, cpp_area.m_cpp_lin_area, L"CppObjectFrgrDecor") == TRUE)
		m_doc_builder.ApplyCppObjectFrgrDecor(decor_owner, svzo_cpp_item_frgr, cpp_area.m_cpp_atype, buffer);
}

TSourceViewDecorOwner *TSyntaxHighlighter::StartPasteBufferingArea(long src_area_beg, ID paste_buff_area_beg_event)
{
	if (m_mldb_reader->GetPasteBufferingChgEvent(m_lex_src_call.hdr.parsing_id, paste_buff_area_beg_event) == FALSE)
		return(NULL);
	return(StartPasteBufferingArea(src_area_beg, m_mldb_reader->EventData()));
}

TSourceViewDecorOwner *TSyntaxHighlighter::StartPasteBufferingArea(long src_area_beg, CtvParsingEventInfo &area_beg_event)
{
	TSourceAreaInfo area = { m_src_file.pars_id, m_src_file.file_id, src_area_beg, 0 };
	return(m_decor_owners.CreateDecorOwner(svdo_buffering_area, area, area_beg_event));
}

void TSyntaxHighlighter::FinalizePasteBufferingArea(TSourceViewDecorOwner *area_owner, long src_area_end)
{
	long area_len = src_area_end-area_owner->m_src_area.area_beg;
	if (area_len <= 0)
	{
		if (area_len < 0)
		{
			ReportProblem(L"Paste buffering area %lu.%lu has negative length of %lu.\r\n",
						area_owner->m_event_data.hdr.parsing_id, area_owner->m_event_data.hdr.event_id, area_len);
		}

		// Dispose the object. It will not be displayed.
		/*
		m_decor_owners.RemoveItem(area_owner);
		if (area_owner == m_latest_decor_owner)
			m_latest_decor_owner = (TSourceViewDecorOwner*)m_decor_owners.GetFirst();
		*/

		delete area_owner;
		return;
	}

	area_owner->m_src_area.area_len = area_len;
	/*
	ApplyMidLevBkgrDecor(area_owner, mlbsi_paste_buff_area, area_owner->m_src_area);
	*/
}

void TSyntaxHighlighter::ApplyMultiAreaCodeStatusProc(CtvCondParsingChangeEvent &initial_stt_event)
{
	// Code status areas are createded by the loop that iterates the MidLev events that were loaded earlier.
	ID pars_id  = m_lex_src_call.hdr.parsing_id;
	CtvCondParsingChangeEvent curr_area_intro = initial_stt_event;
	long area_beg = m_src_file.AdjustToLineBeg(m_encl_code_stts_area.area_beg);

	// Iterate existing decor owners. Decor owners should be iterated using the while loop because new owners
	// can be added inside the loop and these new owners should not be noticed by the loop.
	TSourceViewDecorOwner *curr_owner = (TSourceViewDecorOwner*)m_decor_owners.m_decor_owners.GetFirst();
	while (curr_owner != NULL)
	{
		if (curr_owner->m_type == svdo_parsing_event)
		{
			// Current owner is a parsing event. Allocate local variable to simplify expressions.
			TSourceAreaInfo &event_area = curr_owner->m_src_area;

			// Check the type of the parsing event.
			if (curr_owner->m_event_data.hdr.event_type == pset_cond_parsing_change)
			{
				switch (curr_owner->m_event_data.cond_pars.stmt_type)
				{
					case mpr_ifdef:
					case mpr_ifndef:
					case mpr_if:
							{
								// This is the beginning of a new cond parsing area. The satement itself belongs to the current area.
								// Release the pending area and include the body of the current statement into it.
								long  next_line_beg;
								CreateCodeStatusArea(curr_area_intro, area_beg, m_src_file.AdjustToCurrLineEnd(event_area.AreaEnd(), next_line_beg));
								curr_area_intro = curr_owner->m_event_data.cond_pars;
								area_beg = next_line_beg;
							}
							break;

					case mpr_elif:
					case mpr_else:
					case mpr_endif:
							{
								// The area of the satement itself belongs to its parent. Pick up the description of this area
								// from the database.
								if (m_mldb_reader->GetCondParsChgEventAllowNullEventId(pars_id, curr_owner->m_event_data.cond_pars.cond_ref) == FALSE)
								{
									// Plain failure. It is hard to tell what it is better to do here. Skip this statement.
									break;
								}

								// Release the accumulated area in front of the statement.
								long  prev_line_end;
								CreateCodeStatusArea(curr_area_intro, area_beg, m_src_file.AdjustToPrevLineEnd(event_area.area_beg, prev_line_end));

								if (curr_owner->m_event_data.cond_pars.stmt_type != mpr_endif)
								{
									// Release the area of the statement itself.
									long  next_line_beg;
									CreateCodeStatusArea(m_mldb_reader->EventData().cond_pars, area_beg, m_src_file.AdjustToCurrLineEnd(event_area.AreaEnd(), next_line_beg));
									curr_area_intro = curr_owner->m_event_data.cond_pars;
									area_beg = next_line_beg;
								}
								else
								{
									// New area is the continuation of the parent area. Do not release it right now. It will be relased
									// with the next statement or at the end of the lex src enclosure.
									curr_area_intro = m_mldb_reader->EventData().cond_pars;
									area_beg = m_src_file.AdjustToLineBeg(event_area.AreaBeg());
								}
							}
							break;

					default:
						// The cond parsing event is corrupted.
						assert(FALSE);
						break;
				}
			}
			else if (curr_owner->m_event_data.hdr.event_type == pset_lex_source_enter)
			{
				//
				// The lex src call may change the cond parsing area. Once this happens, the new code status area
				// should be started. There is an important pont here. The cond pars statement may contain macro calls
				// on its territory. These calls cannot change the cond parsing status and they should be ignored.
				// The list of owners in the source viewer is ordered by the linear offset. This means that macro calls
				// from the cond pars expressions will appear after their owning cond pars statements.
				//
				CtvLexSourceEnterEvent &call_event = curr_owner->m_event_data.src_enter;
				if (call_event.hdr.area.AreaBeg() >= curr_area_intro.hdr.area.AreaEnd())
				{
					// Current lex src call is an indepenedent call that does not belong to the latest cond pars expr.
					if (call_event.evt_exit_id != 0 && call_event.evt_exit_cond_ref != curr_area_intro.hdr.event_id)
					{
						// The cond parsing area has really changed.
						if (call_event.src_type != srct_file && call_event.src_type != srct_mj_src)
						{
							// Such changes may appear only after the file inclusions in the files that have the C/C++ style syntax.
							ReportProblem(L"Non file enclosure lex source call has changed the cond parsing state.\r\n\r\n"
										L"	  ParsingId: \t%lu\r\n	  EventId: \t%lu\r\n",
										pars_id, call_event.hdr.event_id);
						}

						// Retrieve the cond pars statement, create the code status area and make this statement a new area beg event.
						if (m_mldb_reader->GetCondParsChgEventAllowNullEventId(pars_id, call_event.evt_exit_cond_ref) == TRUE)
						{
							// Reading from the database went fine. Release the existing accumulated area.
							long  next_line_beg;
							CreateCodeStatusArea(curr_area_intro, area_beg, m_src_file.AdjustToCurrLineEnd(event_area.AreaEnd(), next_line_beg));
							curr_area_intro = m_mldb_reader->EventData().cond_pars;
							area_beg = next_line_beg;
						}
					}
				}
			}
		}

		curr_owner = (TSourceViewDecorOwner*)m_decor_owners.m_decor_owners.GetNext(curr_owner);
	}

	// The loop has finished or it was never entered. Process the final code status area.
	// Pass the last param as -1 to indicate that the area goes up to the end of the active area.
	// This ending can be inline, not the whole line as all other areas, released above.
	CreateCodeStatusArea(curr_area_intro, area_beg, -1);
}

void TSyntaxHighlighter::ApplySingleAreaCodeStatusProc(CtvCondParsingChangeEvent &initial_stt_event)
{
	// Create the code status owner for this area and add it to the list.
	TSourceViewDecorOwner *area_owner = m_decor_owners.CreateDecorOwner(svdo_code_area, m_encl_code_stts_area, (CtvParsingEventInfo&)initial_stt_event);
	if (area_owner == NULL)
		return;

	// Highlight the active area.
	m_doc_builder.ApplyCodeStatusDecors(area_owner, m_lex_src_call.WholeLineLexSrcStyle(), m_encl_whole_line_active_area_end);
}

void TSyntaxHighlighter::CreateCodeStatusArea(CtvCondParsingChangeEvent &intro_event, long area_beg, long area_end)
{
	// Note that event id of the passed intro event can be zero if the area that is going to be created is the root
	// of the conditional parsing structure or when the parsing session does not have any cond pars stmts at all.
	assert(intro_event.hdr.parsing_id == m_lex_src_call.hdr.parsing_id);
	assert(intro_event.hdr.event_type == pset_cond_parsing_change);

	bool whole_line_end = TRUE;
	if (area_end < 0)
	{
		// Use the end of the active area as the end of the code area.
		area_end = m_encl_active_area.AreaEnd();
		whole_line_end = m_encl_whole_line_active_area_end;
	}

	// Check if the code area is empty or not. An empty area can be passed. For example this happens when
	// there is nothing between the #if and #endif statements.
	long area_len = area_end-area_beg;
	if (area_len <= 0)
		return;

	// Create the decor's owner. Note that the area will almost always differ from the area in the intro event.
	TSourceAreaInfo area = { m_src_file.pars_id, m_src_file.file_id, area_beg, area_len };
	TSourceViewDecorOwner *owner = m_decor_owners.CreateDecorOwner(svdo_code_area, area, (CtvParsingEventInfo&)intro_event);
	if (owner == NULL)
	{
		// Error should be already displayed/recorded.
		return;
	}

	// Apply decors to one or more lines.
	m_doc_builder.ApplyCodeStatusDecors(owner, TRUE, whole_line_end);
}

void TSyntaxHighlighter::SetupCmdlRawSyntaxDecors(TSourceAreaInfo &active_area, int next_pos_param_index, TParseEnvInfo *pars_env_info, bool show_raw_errors)
{
	// Instantiate and setup the raw scanner.
	TStringsDict dict;
	TCmdlRawScanner scanner(NULL, NULL, &m_src_file, &dict);

	if (pars_env_info != NULL)
	{
		// Set up the info about possible command line keys.
		scanner.SetKeyParamsInfo(pars_env_info->cmdl_ks_chars, pars_env_info->cmdl_keys_dict);
	}

	if (active_area.AreaBeg() > 0 || active_area.AreaEnd() < m_src_file.info.file_len)
	{
		// The area to scan is not covering the whole file. This should be a rare case.
		scanner.Seek(active_area.area_beg, active_area.area_len);
	}

	// It is necessary to monitor the local lex src call events because they may change the numbering
	// of positional params. Call events have a field that tells the number of scanned pos params at the exit.
	TSourceViewDecorOwner *latest_lex_src_call_event = GetFirstLocalLexSrcCall(FALSE);

	TCmdlLexema lex;
	while (scanner.GetLex(lex) != cmdl_eof)
	{
		// Apply decor to this lexema.
		switch (lex.type)
		{
			case cmdl_comment:
					{
						DWORD owner_data = (lex.value_origin.src_area.AreaBeg() & cmod_scan_pos_mask);
						m_doc_builder.ApplyCmdlRawDecor(owner_data, svzo_raw_object, rcmsi_comment, lex.value_origin.src_area);
					}
					break;

			case cmdl_key_param:
					{
						DWORD owner_data = (lex.key_name_origin.src_area.AreaBeg() & cmod_scan_pos_mask);
						m_doc_builder.ApplyCmdlRawDecor(owner_data, svzo_raw_object, rcmsi_key_key, lex.key_name_origin.src_area);
						m_doc_builder.ApplyCmdlRawDecor(owner_data, svzo_raw_object, rcmsi_key_val, lex.value_origin.src_area);
					}
					break;

			case cmdl_pos_param:
					{
						// Check if it is necessary to update the index of the pos param or not.
						while (latest_lex_src_call_event != NULL)
						{
							if (latest_lex_src_call_event->m_src_area.AreaBeg() >= lex.value_origin.src_area.AreaBeg())
								break;

							// Current lex src call stays in front of the current lexema.
							assert(LEX_SRC_WHOLE_FILE(latest_lex_src_call_event->m_event_data.src_enter.src_type) == TRUE);
							next_pos_param_index = latest_lex_src_call_event->m_event_data.src_enter.evt_exit_pos_prms_cnt;
							latest_lex_src_call_event = GetNextLocalLexSrcCall(latest_lex_src_call_event, FALSE);
						}

						lex.num_value = next_pos_param_index++;
						DWORD owner_data = (lex.value_origin.src_area.AreaBeg() & cmod_scan_pos_mask) | (lex.num_value << cmod_pos_prm_val_shift);
						m_doc_builder.ApplyCmdlRawDecor(owner_data, svzo_raw_object, rcmsi_pos_param, lex.value_origin.src_area);
					}
					break;

			case cmdl_error:
					{
						if (show_raw_errors == TRUE)
						{
							// This is a parsing independent file. Highlight this error.
							DWORD owner_data = (lex.value_origin.src_area.AreaBeg() & cmod_scan_pos_mask);
							m_doc_builder.ApplyCmdlRawDecor(owner_data, svzo_raw_object, rcmsi_raw_scan_error, lex.value_origin.src_area);
						}
					}
					break;
		}
	}
}

void TSyntaxHighlighter::SetupClangRawSyntaxDecors(TSourceAreaInfo &active_area, bool stand_alone_file_mode, bool initial_inc_fn_mode)
{
	if (active_area.area_len <= 0)
	{
		// The active area is missing. Most like this is so because the source file is empty.
		return;
	}

	// Instantiate and setup the raw scanner.
	TStringsDict dict;
	TRawScanner scanner(&m_src_file, &dict);
	scanner.SetSpecialFileNameMode(initial_inc_fn_mode);
	m_non_empty_lex_present = FALSE;

	long curr_area_beg = active_area.area_beg;
	long curr_area_len = active_area.area_len;

	// Pick up the first local lex source call if any that has a non empty paste buffering gap
	// after its body. This is needed because this gap should not be scanned by the raw scanner.
	TSourceViewDecorOwner *lex_src_call_with_pbuff_gap = GetFirstLocalLexSrcCall(TRUE);
	while (lex_src_call_with_pbuff_gap != NULL && lex_src_call_with_pbuff_gap->m_event_data.src_enter.src_offs_after_exit <= curr_area_beg)
		lex_src_call_with_pbuff_gap = GetNextLocalLexSrcCall(lex_src_call_with_pbuff_gap, TRUE);

	if (lex_src_call_with_pbuff_gap != NULL)
	{
		long call_area_end = lex_src_call_with_pbuff_gap->m_src_area.AreaEnd();
		if (call_area_end <= curr_area_beg)
		{
			// Adjust the beg of the active area.
			long offs_after = lex_src_call_with_pbuff_gap->m_event_data.src_enter.src_offs_after_exit;
			assert(offs_after > call_area_end && offs_after > curr_area_beg);

			curr_area_len = curr_area_beg+curr_area_len-offs_after;
			curr_area_beg = offs_after;
		}
		else if (call_area_end < curr_area_beg+curr_area_len)
		{
			// Reduce the length of the current area to scan.
			curr_area_len = call_area_end-curr_area_beg;
		}
	}

	// Loop over contiguous sections that do not include paste buffering gaps.
	for(;;)
	{
		assert(curr_area_len > 0);

		if (curr_area_beg > 0 || curr_area_beg+curr_area_len < m_src_file.info.file_len)
		{
			// Only the part of the file should be annotated. Find out if the active area starts from the beg of the line or not.
			TSimpleLineInfo *lb = m_src_file.GetLineInfoFromFileOffset(curr_area_beg);
			assert(lb != NULL);

			// Restrict the parsing area in the scanner.
			bool initial_newline_flag = (curr_area_beg == lb->LineBeg()) ? TRUE : FALSE;
			scanner.Seek(curr_area_beg, curr_area_len, initial_newline_flag, scanner.GetSpecialFileNameMode());
		}

		// The raw scanner is responsible for terminating the loop. Sooner or later it will give out the EOF lexema.
		TLexema lex;
		while (scanner.GetLex(lex) != ltx_eof)
		{
			// Apply decor to the current lexema.
			if (lex.type != ltx_error || stand_alone_file_mode == TRUE)
				DecorateCLangLexema(lex, stand_alone_file_mode, scanner.GetSpecialFileNameMode());

			// Mimic the behavior of the MidLev parser that sets/resets the include file name scanning mode.
			if (lex.IsKeyword(mpr_include) == TRUE || lex.IsKeyword(mpr_import) == TRUE || lex.IsKeyword(mpr_line) == TRUE || lex.IsKeyword(mpr_using) == TRUE)
			{
				scanner.SetSpecialFileNameMode(TRUE);
			}
			else if (lex.IsEol() == TRUE)
			{
				scanner.SetSpecialFileNameMode(FALSE);
			}

			if (lex.type >= ltx_number && lex.type <= ltx_name)
				m_non_empty_lex_present = TRUE;
		}

		// Check if the current section is the last one or not.
		if (lex_src_call_with_pbuff_gap == NULL)
			break;

		// Pick up the area after the end of the curr paste buffering gap.
		curr_area_beg = lex_src_call_with_pbuff_gap->m_event_data.src_enter.src_offs_after_exit;
		curr_area_len = active_area.AreaEnd()-curr_area_beg;
		if (curr_area_len <= 0)
			break;

		// Check if there are other other calls with paste buffering gaps after the current one or not.
		lex_src_call_with_pbuff_gap = GetNextLocalLexSrcCall(lex_src_call_with_pbuff_gap, TRUE);
		if (lex_src_call_with_pbuff_gap != NULL)
		{
			long call_area_end = lex_src_call_with_pbuff_gap->m_src_area.AreaEnd();
			if (call_area_end < curr_area_beg+curr_area_len)
			{
				// Adjust the length of the area to scan.
				curr_area_len = call_area_end-curr_area_beg;
				if (curr_area_len <= 0)
					break;
			}
		}
	}
}

static TSrcViewCLangLexemaStyleInx g_SNHLT_CLangCommentToStyleInx[lct_num_comment_types] =
{
	rclsi_cmnt_c_style,  rclsi_cmnt_cpp_eol,  rclsi_cmnt_cpp_whl,  rclsi_cmnt_csh_trs,
};

static TSrcViewCLangLexemaStyleInx g_SNHLT_CLangCharConstToStyleInx[lchct_num_charconst_types] =
{
	rclsi_chconst_a,  rclsi_chconst_w,  rclsi_chconst_w,
};

static TSrcViewCLangLexemaStyleInx g_SNHLT_CLangStringConstToStyleInx[lstrt_num_string_types] =
{
	rclsi_string_a,  rclsi_string_w,  rclsi_string_w,
};

static TSrcViewCLangLexemaStyleInx g_SNHLT_CLangKeywordTypeToStyleInx[ltkn_num_keyword_types] =
{
	rclsi_tkn_separ,  rclsi_tkn_arithm,  rclsi_tkn_preproc,  rclsi_tkn_clang,  rclsi_tkn_cpp,  rclsi_tkn_csh,
};

void TSyntaxHighlighter::DecorateCLangLexema(TLexema &lex, bool stand_alone_file_mode, bool inc_fn_mode)
{
	// Convert lexema fields into the style index.
	TSrcViewCLangLexemaStyleInx style_inx;
	switch (lex.type)
	{
		case ltx_comment:
				if (lex.comment_type == lct_c_style && m_non_empty_lex_present == FALSE)
					style_inx = rclsi_cmnt_c_style_big;
				else style_inx = g_SNHLT_CLangCommentToStyleInx[lex.comment_type];
				break;

		case ltx_number:
		case ltx_floating_point:
				style_inx = rclsi_number;
				break;

		case ltx_charconst:
				style_inx = g_SNHLT_CLangCharConstToStyleInx[lex.charconst_type];
				break;

		case ltx_string:
				{
					if (inc_fn_mode == FALSE)
					{
						// This is simple case. Use display style according to the subtype of the string.
						style_inx = g_SNHLT_CLangStringConstToStyleInx[lex.string_type];
					}
					else
					{
						// Display this string with a background only in the parsing independent context. Otherwise a more
						// complex highlighting procedure will be used that is based on the areas in various mid lev events.
						if (stand_alone_file_mode == TRUE)
							style_inx = rclsi_string_inc_fn;
						else style_inx = rclsi_string_spec;
					}
				}
				break;

		case ltx_keyword:
				style_inx = g_SNHLT_CLangKeywordTypeToStyleInx[lex.keyword_type];
				break;

		case ltx_name:
				style_inx = rclsi_name;
				break;

		case ltx_error:
				{
					// Raw scanning error is a rare thing. It may appear in the parsing independent files only.
					assert(stand_alone_file_mode == TRUE);
					style_inx = rclsi_raw_scan_error;
					TSourceAreaInfo src_area;
					src_area.Init(lex.origin.src_area);
					ApplySpacelessDecor(NULL, spcdi_error_area_end_mark, svho_err_warn_mark, src_area);
				}
				break;

		default:
			// All other lexema types are not highlighted.
			return;
	}

	// Prepare the owner data.
	DWORD owner_data = (lex.origin.src_area.area_beg & clod_scan_pos_mask);
	if (lex.first_in_line == TRUE)
		owner_data |= clod_new_line_stt;
	if (inc_fn_mode == TRUE)
		owner_data |= clod_inc_fname_stt;

	// Call the document builder that will add an appropriate decor.
	m_doc_builder.ApplyCLangRawDecor(owner_data, svzo_raw_object, style_inx, lex.origin.src_area);
}

void TSyntaxHighlighter::DecorateMacroHdrLexema(DWORD owner_data_high_byte, TSrcViewMidLevFrgrStyleInx inx, long src_area_beg, long src_area_len)
{
	assert((owner_data_high_byte & mhod_scan_pos_mask) == 0);
	DWORD owner_data = owner_data_high_byte | (src_area_beg & mhod_scan_pos_mask);

	// Call the document builder that will add an appropriate decor.
	TSourceArea area = { NULL, src_area_beg, src_area_len };
	m_doc_builder.ApplyMacroHdrFrgrDecor(owner_data, svzo_mid_lev_frgr, inx, area);
}

void TSyntaxHighlighter::ApplyMarksAreaDecor(TSourceViewDecorOwner *decor_owner, TSrcViewSpacelessDecorInx inx, short horz_order, TSourceAreaInfo &src_area)
{
	// Place decoration in the marks area of the line.
	long line_beg = m_src_file.GetLineNumber(src_area.AreaBeg());
	assert(line_beg > 0);

	TSrcViewDecorZorder zo = svzo_mid_lev_frgr;
	if (inx == spcdi_warning_whole_line_mark || inx == spcdi_error_whole_line_mark)
		zo = svzo_error_warning;

	m_doc_builder.ApplyWholeLineSpaclessDecor(decor_owner, zo, inx, horz_order, line_beg);
}

void TSyntaxHighlighter::ApplySpacelessDecor(TSourceViewDecorOwner *decor_owner, TSrcViewSpacelessDecorInx inx, short horz_order, TSourceAreaInfo &src_area)
{
	if (horz_order == svho_default)
		horz_order = svho_icon_ids_base+inx;

	TSrcViewDecorZorder zo = svzo_mid_lev_frgr;
	if (inx == spcdi_warning_area_end_mark || inx == spcdi_error_area_end_mark)
		zo = svzo_error_warning;

	// Passed areas should have zero length or the mark should be located at the end of the area.
	m_doc_builder.ApplyInlineSpaclessDecor(decor_owner, zo, inx, horz_order, src_area.AreaEnd());
}

bool TSyntaxHighlighter::GetLocalSrcAreaInfo(TSourceAreaInfo &buffer, TLinearArea &lin_area, const wchar_t *area_descr, bool allow_partial_area)
{
	assert(m_lex_src_call.hdr.parsing_id != 0 && m_mldb_reader != NULL);

	// Pick up the source info.
	TSourceLocation loc_area;
	if (m_mldb_reader->GetSourceLocationInfo(loc_area, m_lex_src_call.hdr.parsing_id, lin_area, m_active_scanner) == FALSE)
	{
		ReportProblem(L"Error getting the source location info for \"%s\". ParsingId=%d, LinearBeg=%ld, LinearLen=%ld.\r\n",
					area_descr, m_lex_src_call.hdr.parsing_id, lin_area.linear_beg, lin_area.linear_len);
		return(FALSE);
	}

	// Ensure that the picked area belongs to the current file and that it does not stick out of it.
	if (loc_area.src_area.doc_id != m_src_file.doc_id)
	{
		wchar_t buff1[40], buff2[40];
		ReportProblem(L"Retrieved source area for \"%s\" belongs to the %s while the current file is %s.\r\n",
					area_descr, loc_area.src_area.PrepareFileIdShort(buff1, 40), m_src_file.PrepareFileIdShort(buff2, 40));
		return(FALSE);
	}
	else if (allow_partial_area == FALSE && loc_area.partial_src_area == TRUE)
	{
		if (loc_area.src_ext_goes_up == TRUE)
			ReportProblem(L"Retrieved source area for \"%s\" is longer than the current file.\r\n", area_descr);
		else ReportProblem(L"Retrieved source area for \"%s\" ends in the local lex source call.\r\n", area_descr);
		return(FALSE);
	}

	// Success. Give out the result.
	buffer = loc_area.src_area;
	return(TRUE);
}

TSourceViewDecorOwner *TSyntaxHighlighter::GetFirstLocalLexSrcCall(bool want_paste_buff_gap_call)
{
	TSourceViewDecorOwner *owner = (TSourceViewDecorOwner*)m_decor_owners.m_decor_owners.GetFirst();
	while (owner != NULL)
	{
		if (owner->m_type == svdo_parsing_event && owner->m_event_data.hdr.event_type == pset_lex_source_enter)
		{
			// Current owner is a lex source call object. Check the second condition.
			if (want_paste_buff_gap_call == FALSE || owner->m_event_data.src_enter.evt_exit_linear_after_ret-owner->m_event_data.src_enter.evt_exit_linear_pos > 1)
				break;
		}

		// Shift to the next owner in the list of owners.
		owner = (TSourceViewDecorOwner*)m_decor_owners.m_decor_owners.GetNext(owner);
	}

	return(owner);
}

TSourceViewDecorOwner *TSyntaxHighlighter::GetNextLocalLexSrcCall(TSourceViewDecorOwner *owner, bool want_paste_buff_gap_call)
{
	assert(owner != NULL);
	owner = (TSourceViewDecorOwner*)m_decor_owners.m_decor_owners.GetNext(owner);

	while (owner != NULL)
	{
		if (owner->m_type == svdo_parsing_event && owner->m_event_data.hdr.event_type == pset_lex_source_enter)
		{
			// Current owner is a lex source call object. Check the second condition.
			if (want_paste_buff_gap_call == FALSE || owner->m_event_data.src_enter.evt_exit_linear_after_ret-owner->m_event_data.src_enter.evt_exit_linear_pos > 1)
				break;
		}

		// Shift to the next owner in the list of owners.
		owner = (TSourceViewDecorOwner*)m_decor_owners.m_decor_owners.GetNext(owner);
	}

	return(owner);
}

void TSyntaxHighlighter::ReportProblem(const wchar_t *format, ...)
{
	// Assemble the message.
	wchar_t buffer[2012];
	va_list vargs;
	va_start(vargs, format);
	vswprintf(buffer, 2012, format, vargs);
	va_end(vargs);

	// Send it to the doc builder for processing.
	m_doc_builder.ReportProcessingProblem(buffer);
}


