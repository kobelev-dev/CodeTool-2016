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

#include  "LangView/SpecTabHelpers.H"

// ---------------------------------------------------------------------------------
//  ===================  TSpecTabLinesRangeLayout  =======================
// ---------------------------------------------------------------------------------

bool TSpecTabLinesRangeLayout::AddLineInfo(long line_number, TSpecTabLineLayout &line_info)
{
	if (m_lines_layout.NumItems() == 0)
	{
		// Accept the beginning of the new alyout.
		m_lines_range_beg = line_number;
	}
	else
	{
		// Passed line should stay after the lines that already belong to the current layout.
		assert(line_number >= m_lines_range_beg+m_lines_layout.NumItems());

		long len_gap = line_number-(m_lines_range_beg+m_lines_layout.NumItems());
		while (len_gap-- > 0)
		{
			// Add a dummy line with an empty layout.
			TSpecTabLineLayout dummy_line_info = { { 0, 0 }, TSpecTabPosInfosArray() };
			if (m_lines_layout.AppendItem(dummy_line_info) == FALSE)
				return(FALSE);
		}
	}

	// Accept the layout of the line. This call will move the data in from the passed line info if any.
	return(m_lines_layout.AppendItem(line_info));
}

void TSpecTabLinesRangeLayout::ReleaseLineInfosFromTop(int num_lines)
{
	assert(m_lines_layout.NumItems() > 0);
	assert(num_lines < m_lines_layout.NumItems());

	m_lines_layout.ReleaseItems(0, num_lines);
	m_lines_range_beg += num_lines;
}

void TSpecTabLinesRangeLayout::ReleaseLineInfosFromBottom(int num_lines)
{
	assert(m_lines_layout.NumItems() > 0);
	assert(num_lines < m_lines_layout.NumItems());

	// The line number of the beg of the layout is not changing.
	m_lines_layout.ReleaseItems(m_lines_layout.NumItems()-num_lines, num_lines);
}

int TSpecTabLinesRangeLayout::ApplyLayout(THtmlDecoratedFile &dest)
{
	long num_lines = m_lines_layout.NumItems();
	int cnt_errors = 0;

	// Do not apply layouts that consist of only one line.
	if (num_lines > 1)
	{
		long	line_beg = m_lines_range_beg;
		if (dest.GetIntersectionWithTheVisibleLinesRange(line_beg, num_lines) == TRUE)
		{
			// The layout is not empty and the intersection with the range of visible lines is present.
			for (int inx=0; inx<num_lines; ++inx)
			{
				TSpecTabLineLayout &line_layout = m_lines_layout[line_beg-m_lines_range_beg+inx];
				for (int ilex=0; ilex<line_layout.m_spec_tabs.NumItems(); ++ilex)
				{
					// Use the horz order zero for all spec tabs.
					if (dest.SetSpecialTabDecoration(0, line_layout.m_spec_tabs[ilex].m_src_offs) == FALSE)
						cnt_errors++;
				}
			}

			// Apply the lines range info.
			if (dest.SetSpecTabsAreaDecoration(line_beg, num_lines) == FALSE)
				cnt_errors++;
		}
	}

	return(cnt_errors);
}

// ---------------------------------------------------------------------------------
//  ====================  TSpecTabsBasicProcessor  =======================
// ---------------------------------------------------------------------------------

TSpecTabsBasicProcessor::TSpecTabsBasicProcessor(THtmlDecoratedFile &rprt, int &cnt_apply_decor_errors, int &cnt_temp_struct_alloc_errors)
		: m_dest_file(rprt), m_apply_decor_errors_count(cnt_apply_decor_errors), m_temp_struct_alloc_errors_count(cnt_temp_struct_alloc_errors)
{
	m_curr_line_num = 0;
	m_curr_line_layout.Clear();
}

void TSpecTabsBasicProcessor::ProcessLexema(TLexema &lex)
{
	assert(lex.type >= ltx_comment && lex.type <= ltx_error);
	if (lex.first_in_line)
	{
		long new_line_num = m_dest_file.GetLineNumber(lex.origin.src_area.area_beg);
		assert(new_line_num == m_curr_line_num+1);
		TSimpleLineInfo *new_line_info = m_dest_file.GetLineInfo(new_line_num);
		assert(new_line_info != NULL);

		// Accept the info about the line.
		m_curr_line_num = new_line_num;
		m_curr_line_layout.m_line_info = *new_line_info;
		m_curr_line_layout.m_spec_tabs.Clear();
	}

	// Beginning of the passed lexema should belong to the current line.
	assert(lex.origin.src_area.AreaBeg() >= m_curr_line_layout.m_line_info.LineBeg());
	assert(lex.origin.src_area.AreaBeg() <= m_curr_line_layout.m_line_info.LineEnd());

	if (lex.IsEol() == TRUE)
	{
		// Send this lexema to the callback handler.
		ProcessEndOfLine(lex.endofline_type, lex.origin);
	}
	else if (lex.IsEof() == TRUE)
	{
		// Finalize the current line as if an ordinary EOL is present.
		ProcessEndOfLine(leolt_normal, lex.origin);

		// Dump the accumulated layout if any.
		ApplyAccumulatedLayout();
	}
	else
	{
		// This should be some lexema with a body.
		assert(lex.origin.src_area.area_len > 0);

		if (lex.origin.src_area.AreaEnd() <= m_curr_line_layout.m_line_info.LineEnd())
		{
			// Current  lexema completely belongs to the current line.
			ProcessNonEmptyLexema(FALSE, lex, FALSE);
		}
		else
		{
			// Current lexema sticks out of the current line.
			bool front_ext = FALSE;
			while (lex.origin.src_area.AreaEnd() > m_curr_line_layout.m_line_info.LineEnd())
			{
				// Report the beg or middle of the passed lexema to the callback.
				ProcessNonEmptyLexema(front_ext, lex, TRUE);

				// Report the end of line to the callback.
				PrEventHeader psevdo_eol_origin;
				psevdo_eol_origin.Clear();
				psevdo_eol_origin.src_area.file_info = lex.origin.src_area.file_info;
				psevdo_eol_origin.src_area.area_beg = m_curr_line_layout.m_line_info.LineEnd();
				ProcessEndOfLine(leolt_num_eol_types, psevdo_eol_origin);

				// Prepare vars for the next line.
				TSimpleLineInfo *next_linfo = m_dest_file.GetLineInfo(m_curr_line_num+1);
				assert(next_linfo != NULL);

				m_curr_line_num++;
				m_curr_line_layout.m_line_info = *next_linfo;
				m_curr_line_layout.m_spec_tabs.Clear();

				front_ext = TRUE;
			}

			// Report the end of the current lexema to the callback.
			assert(lex.origin.src_area.AreaEnd() >= m_curr_line_layout.m_line_info.LineBeg());
			assert(lex.origin.src_area.AreaEnd() <= m_curr_line_layout.m_line_info.LineEnd());
			ProcessNonEmptyLexema(TRUE, lex, FALSE);
		}
	}
}

// ---------------------------------------------------------------------------------
//  =================  TSpecTabDefineRangesProcessor  =====================
// ---------------------------------------------------------------------------------

TSpecTabDefinitionRangesProcessor::TSpecTabDefinitionRangesProcessor(THtmlDecoratedFile &rprt, int &cnt_apply_decor_errors, int &cnt_temp_struct_alloc_errors)
		: TSpecTabsBasicProcessor(rprt, cnt_apply_decor_errors, cnt_temp_struct_alloc_errors)
{
	m_line_type = dplt_none;
	m_cnt_line_lexemas = 0;
	m_prev_spec_eol = FALSE;

	m_latest_defns = 0;
	m_latest_nbrks = 0;
}

void TSpecTabDefinitionRangesProcessor::ProcessNonEmptyLexema(bool front_ext, TLexema &lex, bool back_ext)
{
	if (front_ext == TRUE)
	{
		if (m_line_type == dplt_defn_line)
			m_line_type = dplt_defn_cont;
	}
	else if (lex.first_in_line == TRUE)
	{
		// This is a first lexema on s new line. Detect the type of the line.
		if (lex.IsKeyword(mpr_define) == TRUE)
		{
			// This is a macro definition.
			m_line_type = dplt_defn_line;
		}
		else if ((m_line_type == dplt_defn_line || m_line_type == dplt_defn_cont) && m_prev_spec_eol == TRUE)
		{
			// This is continuation of the macro definition.
			m_line_type = dplt_defn_cont;
		}
		else if (lex.IsPreprocToken() == TRUE && (short)lex.num_value >= mpr_define)
		{
			// This is a whole line macroprocessor statement.
			m_line_type = dplt_none;
		}
		else if (lex.IsComment(lct_cpp_wholeline) == TRUE)
		{
			// This is a whole comment line.
			m_line_type = dplt_none;
		}
		else
		{
			// This is some other type of the line. It cannot belong to the definitions range.
			m_line_type = dplt_breaker;
		}
	}

	// Process current lexema according to the type of the current line.
	if (m_line_type == dplt_defn_line)
	{
		if (m_cnt_line_lexemas == 2)
		{
			AddLexemaToCurrLineLayout(lex);
		}

		if (m_cnt_line_lexemas >= 2 && lex.IsComment(lct_cpp_endofline) == TRUE)
		{
			AddLexemaToCurrLineLayout(lex);
		}
	}
	else if (m_line_type == dplt_defn_cont)
	{
		if (m_cnt_line_lexemas == 0)
		{
			AddLexemaToCurrLineLayout(lex);
		}

		if (front_ext == FALSE && (lex.IsComment(lct_cpp_endofline) == TRUE || lex.IsComment(lct_cpp_wholeline) == TRUE))
		{
			AddLexemaToCurrLineLayout(lex);
		}
	}

	// Finalize processing the current lexema.
	m_cnt_line_lexemas++;
}

void TSpecTabDefinitionRangesProcessor::ProcessEndOfLine(TLexEndOfLineType lex_eol_subt, PrEventHeader &lex_eol_origin)
{
	if (lex_eol_subt == leolt_withbksl)
	{
		// This is a special form of the eol. Passed source area starts in front of the backslash.
		AddEolToCurrLineLayout(lex_eol_subt, lex_eol_origin);
	}

	if (m_line_type == dplt_none)
	{
		// The current line is something neutral. It is not a definition itself but at the same time
		// it should not break the area of sequential definitions.
		assert(m_curr_line_layout.IsEmpty() == TRUE);

		if (m_latest_defns > 0)
		{
			// Definitions are present above the curren tline.
			if (m_latest_nbrks >= 6)
			{
				// The number of lines that are not a definition stmts is too big.
				ApplyAccumulatedLayout();
			}
			else
			{
				// Add the curr line to the lines range.
				if (SaveCurrentLineLayout() == TRUE)
				{
					m_latest_nbrks++;
				}
			}
		}
	}
	else if (m_line_type == dplt_defn_line)
	{
		if (m_curr_line_layout.IsEmpty() == TRUE)
		{
			// This is an empty definition. Add a tabstop to the end of the macrodef name.
			AddEolToCurrLineLayout(lex_eol_subt, lex_eol_origin);
		}

		if (SaveCurrentLineLayout() == TRUE)
		{
			if (m_latest_nbrks == 0)
			{
				// This is one more defn is the seq of defns.
				m_latest_defns++;
			}
			else
			{
				// This definition follows one or more non breaking lines.
				m_latest_defns = 1;
				m_latest_nbrks = 0;
			}
		}
	}
	else if (m_line_type == dplt_defn_cont)
	{
		assert(m_latest_defns > 0 && m_latest_nbrks == 0);
		SaveCurrentLineLayout();
	}
	else
	{
		// Current line cannot belong to the layout.
		if (m_latest_defns > 0)
			ApplyAccumulatedLayout();
	}

	// Prepare variables for processing the next line.
	m_line_type = dplt_none;
	m_cnt_line_lexemas = 0;
	m_prev_spec_eol = (lex_eol_subt >= leolt_withbksl);
}

void TSpecTabDefinitionRangesProcessor::ApplyAccumulatedLayout()
{
	if (m_accumulated_layout.IsEmpty() == FALSE)
	{
		// The latest "layout non breaker lines" should not be included into the layout.
		m_accumulated_layout.ReleaseLineInfosFromBottom(m_latest_nbrks);

		// Apply the layout and reset it.
		m_apply_decor_errors_count += m_accumulated_layout.ApplyLayout(m_dest_file);
		m_accumulated_layout.Clear();
		m_latest_defns = m_latest_nbrks = 0;
	}
}

// ---------------------------------------------------------------------------------
//	===============  TSpecTabGrammarLanguageProcessor  ===================
// ---------------------------------------------------------------------------------

TSpecTabGrammarLanguageProcessor::TSpecTabGrammarLanguageProcessor(THtmlDecoratedFile &rprt, int &cnt_apply_decor_errors, int &cnt_temp_struct_alloc_errors)
		: TSpecTabsBasicProcessor(rprt, cnt_apply_decor_errors, cnt_temp_struct_alloc_errors)
{
	m_sect_hdr_beg_flag = FALSE;
	m_sect_hdr_section = grsc_none;
	m_sect_header_line = FALSE;

	m_xpct_name_noticed = FALSE;
	m_xpct_conflicts_nesting = 0;

	m_curr_section = grsc_none;
	m_cnt_line_lexemas = 0;
}

void TSpecTabGrammarLanguageProcessor::ProcessNonEmptyLexema(bool front_ext, TLexema &lex, bool back_ext)
{
	// Process switching between the grammar defn sections.
	if (lex.IsKeyword(opr_lt) == TRUE)
		m_sect_hdr_beg_flag = TRUE;
	else if (lex.IsKeyword(opr_gt) == TRUE || lex.IsKeyword(spr_semicol) == TRUE)
		m_sect_hdr_beg_flag = FALSE;

	// Change the type of the section only on the root level of the xpct conflicts. Contents of the xpct conflict
	// will be processed under the <conflicts> section in spite of the fact that section type changes inside.
	if (m_xpct_conflicts_nesting == 0 && m_sect_hdr_beg_flag == TRUE && lex.type == ltx_name)
	{
		// Dump current layout and block current line from being part of any layout.
		if (m_curr_section != grsc_rules)
			ApplyAccumulatedLayout();
		else if (m_accumulated_layout.IsEmpty() == FALSE)
			FinalizeAndApplyRulesLayout();

		m_curr_line_layout.m_spec_tabs.Clear();

		m_sect_hdr_beg_flag = FALSE;
		m_sect_hdr_section = GetGrammarDefnSectionType(lex.str_value);
		m_sect_header_line = TRUE;
	}

	// Check for the name of the root xpct conflict.
	if (lex.first_in_line == TRUE)
	{
		m_xpct_name_noticed = FALSE;
		if (m_curr_section == grsc_conflicts && lex.type == ltx_name)
			m_xpct_name_noticed = TRUE;
	}

	// Process the xpct conflicts nesting count.
	if (lex.IsKeyword(spr_lcurvbr) == TRUE)
		m_xpct_conflicts_nesting++;
	else if (lex.IsKeyword(spr_rcurvbr) == TRUE)
		m_xpct_conflicts_nesting--;

	if (m_sect_header_line == FALSE)
	{
		// Do the high level processing of the current lexema.
		if (m_curr_section == grsc_langtype)
		{
			if (m_cnt_line_lexemas == 2 && lex.IsComment(lct_cpp_endofline) == TRUE)
				AddLexemaToCurrLineLayout(lex);
		}
		else if (m_curr_section == grsc_symbols)
		{
			if (m_cnt_line_lexemas >= 2 && m_cnt_line_lexemas <= 4 && lex.type == ltx_name)
				AddLexemaToCurrLineLayout(lex);
		}
		else if (m_curr_section == grsc_rules)
		{
		}
		else if (m_curr_section == grsc_conflicts)
		{
			if (m_xpct_name_noticed == TRUE && m_cnt_line_lexemas == 1 && lex.IsComment(lct_cpp_endofline) == TRUE)
				AddLexemaToCurrLineLayout(lex);
		}
	}

	// Finalize processing the current lexema.
	m_cnt_line_lexemas++;
}

void TSpecTabGrammarLanguageProcessor::ProcessEndOfLine(TLexEndOfLineType lex_eol_subt, PrEventHeader &lex_eol_origin)
{
	if (m_sect_header_line == FALSE)
	{
		// Section type was not switching on the current line.
		if (m_curr_section == grsc_langtype)
		{
			if (m_curr_line_layout.IsEmpty() == FALSE)
				SaveCurrentLineLayout();
			else ApplyAccumulatedLayout();
		}
		else if (m_curr_section == grsc_symbols)
		{
			SaveCurrentLineLayout();
		}
		else if (m_curr_section == grsc_rules)
		{
			if (m_curr_line_layout.IsEmpty() == FALSE)
				SaveCurrentLineLayout();
			else if (m_accumulated_layout.IsEmpty() == FALSE)
				FinalizeAndApplyRulesLayout();
		}
		else if (m_curr_section == grsc_conflicts)
		{
			SaveCurrentLineLayout();
		}
	}

	// Prepare for the next line.
	m_sect_header_line = FALSE;
	m_curr_section = m_sect_hdr_section;
	m_cnt_line_lexemas = 0;
}

void TSpecTabGrammarLanguageProcessor::ApplyAccumulatedLayout()
{
	// Apply the layout and reset it.
	m_apply_decor_errors_count += m_accumulated_layout.ApplyLayout(m_dest_file);
	m_accumulated_layout.Clear();
}

void TSpecTabGrammarLanguageProcessor::FinalizeAndApplyRulesLayout()
{
}


