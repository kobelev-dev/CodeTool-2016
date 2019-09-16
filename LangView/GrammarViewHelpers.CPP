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

#include  "TextScan/GrammarAnalysis.H"
#include  "LangView/GrammarViewHelpers.H"

// -------------------------------------------------------------------------
//    =================  TGrammarViewLabelItem  ===================
// -------------------------------------------------------------------------

void TGrammarViewLabelItem::SetupGrammarSymbolObject(TGrammar *grammar, TLangViewLexItemStyle &lex_items_style, WORD sym, bool bright_symbol)
{
	assert(sym < grammar->NumTerminals() || sym >= NonTerminalsBase && sym < NonTerminalsBase+grammar->NumNonTerminals());

	m_label_type = (bright_symbol) ? gvlt_grammar_symbol : gvlt_cfct_inp_symbol;
	m_app_value = sym;

	if (sym >= NonTerminalsBase)
	{
		SetStyle(lex_items_style.GetNonTermLabelStyle(sym));
		SetText(grammar->non_terminals[sym-NonTerminalsBase].symbol_name);
	}
	else
	{
		TTerminalSymbol &sym_info = grammar->symbols[sym];
		SetStyle(lex_items_style.GetLexemaTypeStyle(sym_info.lex_type, sym_info.check_subt, sym_info.lex_subt, bright_symbol));

		wchar_t  buff_40_chars[40];
		if (lex_items_style.GetShowLexemasGrammarNameMode() == FALSE)
		{
			// Show type of the source code lexema that this grammar defn lexema is mapped to.
			SetText(lex_items_style.GetLexemaTypeLabel(sym_info.lex_type, sym_info.check_subt, sym_info.lex_subt, buff_40_chars));
		}
		else
		{
			// Show the name as it is defined in the grammar.
			SetText(grammar->GetSymbolName(sym, buff_40_chars));
		}
	}
}

void TGrammarViewLabelItem::SetupXpctConflictObject(TGrammar *grammar, WORD ixpct)
{
	assert(ixpct >= 0 && ixpct < grammar->NumXpctConflicts());

	m_label_type = gvlt_xpct_conflict;
	m_app_value = ixpct;
	SetTextFmt(L"X%hu: %s", ixpct, grammar->xpct_conflicts[ixpct].xpct_conflict_name);
}

void TGrammarViewLabelItem::SetupObject(TLabelType gvlt, WORD app_value)
{
	assert(gvlt != gvlt_grammar_symbol && gvlt != gvlt_cfct_inp_symbol && gvlt != gvlt_xpct_conflict);

	m_label_type = gvlt;
	m_app_value = app_value;

	if (gvlt == gvlt_grammar_conflict)
	{
		SetTextFmt(L"C%hu", app_value);
	}
	else if (gvlt == gvlt_parsing_action)
	{
		wchar_t buff_40_chars[40];
		SetText(TGrammar::GetShortParsingActionName(app_value, buff_40_chars));
	}
	else
	{
		SetTextFmt(L"%hu", app_value);
	}
}

void TGrammarViewLabelItem::SetupLink(TScreenItem *dest_object, TLineStyle *link_style, long *special_src_y)
{
	assert(dest_object != NULL);

	// Style of the link should be set even if it NULL.
	m_dest_link.SetStyle(link_style);

	if (link_style != NULL)
	{
		long src_y = (special_src_y != NULL) ? *special_src_y : (m_bounding_rect.top+m_bounding_rect.bottom)/2;
		RECT *dest_rc = dest_object->GetBoundingRect();
		long dest_y = (dest_rc->top+dest_rc->bottom)/2;

		long ext_offs = (link_style->m_props.line_width > 1) ? 1: 0;
		if (dest_rc->left > m_bounding_rect.right)
		{
			m_dest_link.SetJctPos(0, m_bounding_rect.right-ext_offs, src_y);
			m_dest_link.SetJctPos(1, dest_rc->left-1+ext_offs, dest_y, TRUE);
		}
		else if (dest_rc->right < m_bounding_rect.left)
		{
			m_dest_link.SetJctPos(0, m_bounding_rect.left-1+ext_offs, src_y);
			m_dest_link.SetJctPos(1, dest_rc->right-ext_offs, dest_y, TRUE);
		}
	}

	m_dest_link_object = dest_object;
}

// -------------------------------------------------------------------------
//    ===============  TLangViewGrammarRulesGrid  ==================
// -------------------------------------------------------------------------

void TLangViewGrammarRulesGrid::Reset()
{
	m_grid.Reset();
	m_grid_info_storage.Clear();

	m_rule_index_clmn = 0;
	m_alignment_ref_clmn = -1;
	m_sym_clmns_info.Clear();
}

bool TLangViewGrammarRulesGrid::CreateInitialLayout(TBasicDialog *dlg, TBasicStyle *dark_delim_style,
										TBasicStyle *non_term_clmn_bkgr_style, TBasicStyle *pars_stt_clmn_bkgr_style, TBasicStyle *alignment_clmn_bkgr_style,
										bool want_row_descr_clmn, bool want_rule_index_and_rule_non_term_clmn, bool want_wide_rule_index_clmn,
										bool want_alignment_clmn, bool want_wide_alignment_clmn, bool want_alignment_clmn_ext_spc, bool want_wide_trailing_clmn)
{
	// Initial set of columns is added without checking result. This is possible because screen items grid
	// and array of logical column infos both contain preallocated slots.
	assert(m_grid.GetNumInitialColumnSlots() >= 10);

	Reset();

	//
	// Part 1. Add columns.
	//
	TScreenItemsGridColumnInfo clmn_info;

	// Check if conflict viewing specific column is needed or not.
	if (want_row_descr_clmn == TRUE)
	{
		// (1) Add row description column.
		clmn_info.Setup(row_descr_clmn_width);
		clmn_info.SetupDelim(1, dark_delim_style);
		m_grid.AddColumn(SCRITEMS_GRID_APPEND, clmn_info);

		// Reflect this column in the data fields of the class.
		m_rule_index_clmn = 1;
	}

	if (want_rule_index_and_rule_non_term_clmn == TRUE)
	{
		// (2) Rule index column.
		clmn_info.Setup((want_wide_rule_index_clmn == FALSE) ? rule_index_clmn_width : rule_index_clmn_width_extended);
		clmn_info.SetupDelim(1, dark_delim_style);
		m_grid.AddColumn(SCRITEMS_GRID_APPEND, clmn_info);

		// (3) Rule non terminal column.
		clmn_info.Setup(rule_symbol_clmn_width, non_term_clmn_bkgr_style);
		clmn_info.SetupDelim(1, dark_delim_style);
		m_grid.AddColumn(SCRITEMS_GRID_APPEND, clmn_info);
	}
	else
	{
		// Reflect absence of these columns in the data fields of the class.
		m_rule_index_clmn -= 2;
	}

	// (4) Front space column (column without delimiter).
	clmn_info.Setup(rule_front_space_clmn_width);
	m_grid.AddColumn(SCRITEMS_GRID_APPEND, clmn_info);

	// (5) Initial parsing state column.
	clmn_info.Setup(0);
	clmn_info.SetupDelim(1, pars_stt_clmn_bkgr_style);
	m_grid.AddColumn(SCRITEMS_GRID_APPEND, clmn_info);

	// Check if alignment column should be present or not.
	if (want_alignment_clmn == TRUE)
	{
		assert(alignment_clmn_bkgr_style != NULL);

		// Symbol column in front of the alignment column.
		clmn_info.Setup(rule_symbol_clmn_width);
		m_grid.AddColumn(SCRITEMS_GRID_APPEND, clmn_info);

		// Add description of the logical symbol column.
		TSymClmnInfo sym_clmn_info1;
		sym_clmn_info1.AssignLastColumnsPair(m_grid);
		m_sym_clmns_info.AppendItem(sym_clmn_info1);

		// Alignment column is a special parsing state column.
		clmn_info.Setup(0);
		clmn_info.SetupDelim((want_wide_alignment_clmn == TRUE) ? 5 : 3, alignment_clmn_bkgr_style);
		m_grid.AddColumn(SCRITEMS_GRID_APPEND, clmn_info);

		// Rule symbol column, that was just created above has logical index 0. Delimiter stays in front of the rule
		// symbol column with index 1. Parsing state stays in front of the rule symbol. Grammar conflict description
		// stores index of the symbol after the conflict. In both cases alignment column index should be 1.
		m_alignment_ref_clmn = 1;
	}

	// Trailing column.
	clmn_info.Setup((want_wide_trailing_clmn == FALSE) ? trailing_clmn_width_normal : trailing_clmn_width_extended);
	m_grid.AddColumn(SCRITEMS_GRID_APPEND, clmn_info);

	// Add description of the logical symbol column.
	TSymClmnInfo sym_clmn_info2;
	sym_clmn_info2.AssignLastColumnsPair(m_grid);
	m_sym_clmns_info.AppendItem(sym_clmn_info2);

	if (want_alignment_clmn == TRUE && want_alignment_clmn_ext_spc == TRUE)
	{
		// Add extra spacing around the alignment column. These columns appear like additional parsing state
		// columns, but all view modes that need them are not using them.
		clmn_info.Setup(12);
		int iclmn = m_sym_clmns_info[m_alignment_ref_clmn].m_pstt_clmn;
		m_grid.AddColumn(iclmn, clmn_info);
		m_grid.AddColumn(iclmn+2, clmn_info);
		m_sym_clmns_info[m_alignment_ref_clmn].m_pstt_clmn += 1;
		m_sym_clmns_info[m_alignment_ref_clmn].m_symbol_clmn += 2;
	}

	//
	// Part 2. Add upper empty row.
	//
	TScreenItemsGridRowInfo row_info;
	row_info.Setup(14);
	if (m_grid.AddRow(SCRITEMS_GRID_APPEND, row_info, &m_grid_info_storage) == FALSE)
	{
		dlg->MsgBox(L"CreateInitialLayout: Error adding the topmost row.");
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TLangViewGrammarRulesGrid::CreateRuleSymbolColumn(TBasicDialog *dlg, TBasicStyle *pars_stt_clmn_bkgr_style, bool right_side_clmn)
{
	// Ensure integrity of the columns structure.
	assert(RuleSymClmn(NumSymbolColumns()) == TrailingClmn());

	if (m_sym_clmns_info.ReserveSpace() == FALSE)
	{
		dlg->MsgBox(L"CreateRuleSymbolColumn: Error enlarging the symbol columns info.");
		return(FALSE);
	}

	TScreenItemsGridColumnInfo clmn_info_pars_stt;
	clmn_info_pars_stt.Setup(0);
	clmn_info_pars_stt.SetupDelim(1, pars_stt_clmn_bkgr_style);

	TScreenItemsGridColumnInfo clmn_info_symbol;
	clmn_info_symbol.Setup(rule_symbol_clmn_width);

	int ins_before_clmn = (right_side_clmn == FALSE) ? ParsSttClmn(0) : TrailingClmn();
	if (right_side_clmn == FALSE)
	{
		// Insert the rule symbol column first.
		if (m_grid.AddColumn(ins_before_clmn, clmn_info_symbol) == FALSE)
		{
			dlg->MsgBox(L"CreateRuleSymbolColumn: Error inserting the rule symbol column.");
			return(FALSE);
		}
		else if (m_grid.AddColumn(ins_before_clmn, clmn_info_pars_stt) == FALSE)
		{
			dlg->MsgBox(L"CreateRuleSymbolColumn: Error inserting the parsing state column.");
			m_grid.DeleteColumn(ins_before_clmn);
			return(FALSE);
		}

		// Bump column numbers for all existing symbol columns.
		for (int inx=0; inx<m_sym_clmns_info.NumItems(); ++inx)
		{
			m_sym_clmns_info[inx].m_pstt_clmn += 2;
			m_sym_clmns_info[inx].m_symbol_clmn += 2;
		}

		// Insert new record into the beginning of array.
		TSymClmnInfo sym_clmn_info1 = { ins_before_clmn, ins_before_clmn+1 };
		m_sym_clmns_info.InsertItem(0, sym_clmn_info1);

		if (m_alignment_ref_clmn >= 0)
			m_alignment_ref_clmn++;
	}
	else
	{
		// Insert the parsing state column first.
		if (m_grid.AddColumn(ins_before_clmn, clmn_info_pars_stt) == FALSE)
		{
			dlg->MsgBox(L"CreateRuleSymbolColumn: Error inserting the parsing state column.");
			return(FALSE);
		}
		else if (m_grid.AddColumn(ins_before_clmn, clmn_info_symbol) == FALSE)
		{
			dlg->MsgBox(L"CreateRuleSymbolColumn: Error inserting the rule symbol column.");
			m_grid.DeleteColumn(ins_before_clmn);
			return(FALSE);
		}

		// Append description of the new column to the end of array.
		TSymClmnInfo sym_clmn_info2 = { ins_before_clmn+1, ins_before_clmn+2 };
		m_sym_clmns_info.AppendItem(sym_clmn_info2);
	}

	// Success.
	return(TRUE);
}

bool TLangViewGrammarRulesGrid::CreateParsSttColumnsPair(TBasicDialog *dlg, int isym)
{
	// Ensure the columns structure integrity.
	assert(RuleSymClmn(NumSymbolColumns()) == TrailingClmn());

	assert(isym >= 0 && isym <= NumSymbolColumns());
	TSymClmnInfo &info = m_sym_clmns_info[isym];

	// Additional parsing state columns are simple white columns without any delimiters.
	TScreenItemsGridColumnInfo clmn_info;
	clmn_info.Setup(ExtraParsSttClmnWidth());

	if (m_grid.AddColumn(info.m_symbol_clmn, clmn_info) == FALSE)
	{
		dlg->MsgBox(L"Error creating additional right side parsing state column.");
		return(FALSE);
	}

	int delta = info.m_symbol_clmn-1-info.m_pstt_clmn;
	if (m_grid.AddColumn(info.m_pstt_clmn-delta, clmn_info) == FALSE)
	{
		dlg->MsgBox(L"Error creating additional left side parsing state column.");
		m_grid.DeleteColumn(info.m_symbol_clmn);
		return(FALSE);
	}

	// Update the symbol columns info.
	info.m_pstt_clmn++;
	info.m_symbol_clmn += 2;
	for (int inx=isym+1; inx<m_sym_clmns_info.NumItems(); ++inx)
	{
		m_sym_clmns_info[inx].m_pstt_clmn += 2;
		m_sym_clmns_info[inx].m_symbol_clmn += 2;
	}

	// Success.
	return(TRUE);
}

void TLangViewGrammarRulesGrid::RemoveRuleSymbolColumn(TBasicDialog *dlg, int isym)
{
	assert(isym >= 0 && isym <= NumSymbolColumns());
	TSymClmnInfo &info = m_sym_clmns_info[isym];

	// Update the grid.
	int num_columns_to_delete = info.NumParsingStateColumns()+1;
	for (int cnt=0; cnt<num_columns_to_delete; ++ cnt)
	{
		// Index of the column to delete is always the same.
		m_grid.DeleteColumn(info.m_symbol_clmn-info.NumParsingStateColumns());
	}

	if (isym <m_alignment_ref_clmn)
		m_alignment_ref_clmn--;

	// Update the symbol columns info.
	m_sym_clmns_info.ReleaseItem(isym);
	for (int inx=isym; inx<m_sym_clmns_info.NumItems(); ++inx)
	{
		m_sym_clmns_info[inx].m_pstt_clmn -= num_columns_to_delete;
		m_sym_clmns_info[inx].m_symbol_clmn -= num_columns_to_delete;
	}

	// Ensure the columns structure integrity after the action.
	assert(RuleSymClmn(NumSymbolColumns()) == TrailingClmn());
}

int TLangViewGrammarRulesGrid::CreateRow(TBasicDialog *dlg, const wchar_t *descr, TLangViewRulesGridRowMeaning row_meaning,
										int row_height, TBasicStyle *bkgr_style, TBasicStyle *delim_style, WORD inx2, WORD inx3, int inx4,
										int ins_before_row, TLangViewRulesGridRowMeaningEx ex_meaning)
{
	// Prepare the row properties.
	TScreenItemsGridRowInfo row_info;
	row_info.Setup(row_height, bkgr_style);
	if (delim_style != NULL)
	{
		// This row requires lower delimiter of 1 pixel height.
		row_info.SetupDelim(1, delim_style);
	}

	// Setup the app data in the props.
	TAppDataEx data_ex = { inx2, inx3, inx4 };
	row_info.m_row_app_data = MAKELONG(row_meaning, ex_meaning);
	row_info.m_row_app_data_ex = data_ex.m_data;

	if (ins_before_row < 0)
		ins_before_row = SCRITEMS_GRID_APPEND;

	if (m_grid.AddRow(ins_before_row, row_info, &m_grid_info_storage) == FALSE)
	{
		if (ins_before_row != SCRITEMS_GRID_APPEND)
			dlg->MsgBoxFmt(MB_OK, L"Error adding row (%s) before the row with index %d.", (descr != NULL) ? descr : L"NoDescr", ins_before_row);
		else dlg->MsgBoxFmt(MB_OK, L"Error adding row (%s) to the bottom of the grid.", (descr != NULL) ? descr : L"NoDescr");

		// Creation failed.
		return(-1);
	}

	// Row creation succeeded. Return index of the new row.
	return((ins_before_row == SCRITEMS_GRID_APPEND) ? m_grid.NumRows()-1 : ins_before_row);
}

void TLangViewGrammarRulesGrid::RemoveRowDelimiter(int irow)
{
	TScreenItemsGridRowInfo row_info;
	m_grid.GetRowInfo(irow, row_info);
	row_info.m_delim_height = 0;
	row_info.m_bkgr_delim_style = NULL;
	m_grid.UpdateRowInfo(irow, row_info);
}

void TLangViewGrammarRulesGrid::SetupCenteredCell(TBasicDialog *dlg, const wchar_t *descr, int physical_row_inx, int physical_clmn_inx, TScreenItem *screen_item,
													short off_bottom, short horz_shift)
{
	TScreenItemsGridCellData cell_data;
	cell_data.Clear();
	cell_data.PlaceInTheCenter(screen_item, TRUE);
	cell_data.m_off_bottom = (char)off_bottom;
	cell_data.m_scr_item_shift_x[0] = horz_shift;
	m_grid.SetCellData(physical_row_inx, physical_clmn_inx, cell_data);
}

void TLangViewGrammarRulesGrid::SetupLeftAlignedCellWithRowBkgr(TBasicDialog *dlg, const wchar_t *descr, int physical_row_inx, int physical_clmn_inx,
													TScreenItem *scr_item, short off_bottom, short horz_shift)
{
	// Ckeck if passed row has background or not.
	TScreenItemsGridRowInfo row_info;
	m_grid.GetRowInfo(physical_row_inx, row_info);
	TRectItem *bkgr_rect = NULL;
	if (row_info.m_bkgr_style != NULL)
	{
		// Create background rect because passed screen item can span over several cells.
		bkgr_rect = new TRectItem(row_info.m_bkgr_style);
		if (bkgr_rect != NULL)
		{
			// Setup size of the rect.
			bkgr_rect->SetBaseRectSize(scr_item->GetWidth()+7*horz_shift+3, row_info.m_height);
		}
		else
		{
			dlg->MsgBoxFmt(MB_OK, L"Error creating bkgr rect for the left aligned cell (%s).", (descr != NULL) ? descr : L"NoDescr");
		}
	}

	// Prepare the cell info.
	TScreenItemsGridCellData cell_data;
	cell_data.Clear();
	cell_data.PlaceLeftCenter(NULL, FALSE);
	cell_data.PlaceAtLayer(0, scr_item, TRUE, horz_shift);
	cell_data.m_off_bottom = (char)off_bottom;
	if (bkgr_rect != NULL)
		cell_data.PlaceAtLayer(1, bkgr_rect, TRUE);

	// Pass this cell info into the grid object.
	m_grid.SetCellData(physical_row_inx, physical_clmn_inx, cell_data);
}

void TLangViewGrammarRulesGrid::UpdateGridColumnWidth(int clmn_inx, long min_width)
{
	// Figure out width of the widest item in the column.
	long max_width = 0;
	for (int row_inx=0; row_inx<m_grid.NumRows(); ++row_inx)
	{
		TScreenItem *cell_item = m_grid.GetMainLayerScreenItem(row_inx, clmn_inx);
		if (cell_item != NULL && m_grid.GetCellHorzAlignment(row_inx, clmn_inx) == align_center)
		{
			long width = cell_item->GetWidth();
			if (width > max_width)
				max_width = width;
		}
	}

	long new_clmn_width = __max(max_width+20, min_width);
	if (m_grid.ColumnWidth(clmn_inx) != new_clmn_width)
	{
		// Update props of the column.
		TScreenItemsGridColumnInfo clmn_info;
		m_grid.GetColumnInfo(clmn_inx, clmn_info);
		clmn_info.m_width = new_clmn_width-clmn_info.m_delim_width;
		m_grid.UpdateColumnInfo(clmn_inx, clmn_info);
	}
}

void TLangViewGrammarRulesGrid::ShiftMainLayerItem(int row_inx, int clmn_inx, short shift_x, short shift_y)
{
	TScreenItemsGridCellData cell_data;
	m_grid.GetCellData(row_inx, clmn_inx, cell_data);
	cell_data.m_scr_item_shift_x[0] += shift_x;
	cell_data.m_scr_item_shift_y[0] += shift_y;
	m_grid.SetCellData(row_inx, clmn_inx, cell_data);
}

// -------------------------------------------------------------------------------
//    ===============  TLangViewBuildStateTransitionsHelper  ==================
// -------------------------------------------------------------------------------

bool TLangViewBuildStateTransitionsHelper::PrepareIterationIndexes(void (*ProgressHandler)(void *context, int items_done, int total_items), void *context)
{
	if (m_grammar != NULL && m_rule_start_stts_index.NumItems() == 0)
	{
		if (m_grammar->grm_props.processing_result != grpr_full_success)
		{
			// Do not create indexes. State transitions will not be displayed.
			return(TRUE);
		}

		// Rule start states index is required for faster generation of the parsing state transitions.
		if (m_grammar->GenerateRuleStartStatesIterIndex(m_rule_start_stts_index, ProgressHandler, context) == FALSE)
		{
			return(FALSE);
		}
	}

	// Full success.
	return(TRUE);
}

bool TLangViewBuildStateTransitionsHelper::PrepareStateTransitionRows(TBasicDialog *dlg, int irule, int isym, WORD pars_stt_at_isym, WORD action_sym_restr,
																	bool expand_pars_stts_left, bool expand_pars_stts_right, bool collect_action_symbols)
{
	assert(m_grammar != NULL);
	assert(m_grammar->NumParsingStates() > 0);

	// Do basic params verification.
	assert(irule >= 0 && irule < m_grammar->NumRules());
	assert(isym >= 0 && isym <= m_grammar->rules[irule].Length());
	assert(pars_stt_at_isym == 0xFFFF || pars_stt_at_isym < m_grammar->NumParsingStates());
	assert(action_sym_restr == 0xFFFF || action_sym_restr < m_grammar->NumTerminals());

	// Kill the old results set if any.
	m_result.Clear();

	//
	//  Step 1: Search for possible positions in the passed rule.
	// ----------------------------------------------------------
	//

	// Adjust the input parameters.
	if (isym == 0 && expand_pars_stts_left == TRUE)
	{
		// Caller requested positions for the beginning of the rule. In this case left expansion is not possible.
		expand_pars_stts_left = FALSE;
	}

	// Prepare row creation vars.
	int isym_beg = isym;
	int num_columns = 1;						// This is number of the state transition nodes in all rows of the results set.
	if (expand_pars_stts_left == TRUE)
	{
		isym_beg = 0;
		num_columns += isym;
	}

	if (expand_pars_stts_right == TRUE)
	{
		num_columns += m_grammar->rules[irule].Length()-isym;
	}

	//
	//  These are the data variables of the major loop of the step1. First part of the loop prepares
	//  these variables, second part of the loop processes them. Third part of the loop shifts iterator.
	//
	WORD stt;
	TRulePosition pos;

	// Allocate and init iterators.
	TParsingStatesIterator iter1;
	TParsingStatePositionsIterator iter2;
	TRuleStartStatesIterator iter3;
	if (expand_pars_stts_left == FALSE)
	{
		if (pars_stt_at_isym == 0xFFFF)
		{
			// Iterate positions of all parsing states.
			iter1.Setup(m_grammar);
			iter2.Setup(m_grammar->parsing_states[0]);
		}
		else
		{
			// Iterate positions of only one parsing state.
			iter1.Setup(m_grammar);
			iter1.Seek(pars_stt_at_isym);
			iter2.Setup(m_grammar->parsing_states[pars_stt_at_isym]);
		}
	}
	else
	{
		// Iterate the rule starting states.
		assert(isym > 0);
		if (m_rule_start_stts_index.NumItems() > 0)
		{
			// Data indexing is available.
			iter3.PrepareIteration(m_grammar, m_rule_start_stts_index, irule, isym, pars_stt_at_isym, action_sym_restr);
			iter3.Step();
		}
		else
		{
			// Use slow iteration.
			iter3.PrepareNoIndexIteration(m_grammar, irule, isym, pars_stt_at_isym, action_sym_restr);
			iter3.StepNoIndex();
		}

		assert(iter3 == TRUE);

		// Fill in fields that are not changing during iteration.
		pos.irule = irule;
		pos.isym = 0;
	}

	// Run the positions search loop.
	bool data_avail = TRUE;
	WORD prev_stt = 0xFFFF;
	TRulePosition prev_pos = { 0xFFFF, 0xFFFF };
	TSymbolsArray *curr_row_buff = NULL;
	while (data_avail == TRUE)
	{
		// Fill iteration step state vars.
		bool use_curr_pos = TRUE;
		if (expand_pars_stts_left == FALSE)
		{
			// Pick up the curr state from iter1/iter2.
			stt = iter1.CurrStateIndex();
			pos = iter2.CurrPos();

			// Current position needs additional verification.
			if (pos.irule != irule || pos.isym != isym)
			{
				use_curr_pos = FALSE;
			}
			else if (action_sym_restr != 0xFFFF && pos.action_sym != action_sym_restr)
			{
				use_curr_pos = FALSE;
			}
		}
		else
		{
			// Third iterator modifies only the state and the action symbol. Curr pos does not require any additional filtering.
			stt = iter3.CurrState();
			pos.action_sym = iter3.CurrActionSymbol();
		}

		// Check if current position is appropriate or not.
		if (use_curr_pos == TRUE)
		{
			// Do the high level processing of the curr position.
			if (collect_action_symbols == FALSE)
			{
				// Action symbols are not needed. This means that rows should be created only for different parsing states.
				if (stt != prev_stt)
				{
					// Create state transitions row with default action symbol 0xFFFF and without the symbols buffer.
					if (CreateStateTransitionsRow(dlg, irule, isym_beg, stt, num_columns) == FALSE)
						return(FALSE);
				}
			}
			else
			{
				if (stt == prev_stt)
				{
					// Add new action symbol to the list of action symbols of the current row. It is ok not to check the result
					// of appending because space in the buffer was reserved for all terminals of the grammar.
					assert(curr_row_buff != NULL);
					curr_row_buff->AppendItem(pos.action_sym);
				}
				else
				{
					// Create new row with symbols buffer.
					if (CreateStateTransitionsRow(dlg, irule, isym_beg, stt, num_columns, pos.action_sym, &curr_row_buff) == FALSE)
						return(FALSE);
				}
			}

			// Current position is the previous position now.
			prev_stt = stt;
			prev_pos = pos;
		}

		// Third part. Shift iterators to the next position.
		if (expand_pars_stts_left == FALSE)
		{
			// Shift to the next position in the current state.
			++iter2;
			if (iter2 == FALSE)
			{
				// All positions of the current state are processed.
				if (pars_stt_at_isym == 0xFFFF)
				{
					// Shift to the next parsing state.
					++iter1;
					if (iter1 == FALSE)
					{
						// End of processing.
						data_avail = FALSE;
					}
					else
					{
						// Inner iterator should be reinited.
						iter2.Setup(m_grammar->parsing_states[iter1.CurrStateIndex()]);
					}
				}
				else
				{
					// End of processing.
					data_avail = FALSE;
				}
			}
		}
		else
		{
			// Shift to the next rule starting state.
			if (m_rule_start_stts_index.NumItems() > 0)
			{
				// Indexing data is available.
				iter3.Step();
			}
			else
			{
				// Use slow iteration.
				iter3.StepNoIndex();
			}

			if (iter3 == FALSE)
				data_avail = FALSE;
		}
	}

	if (m_result.NumItems() <= 0)
	{
		// The upper layer should decide what to do in this situation.
		return(TRUE);
	}

	//
	//  Step 2: Sort the state transition rows.
	// -------------------------------------
	//

	if (m_result.NumItems() > 1)
	{
		// It is necessary to sort the created rows.
		m_result.QuickSortWithCtx(this, CompareTransitionRows);
	}

	// The resulting matrix should contain at least one state transition.
	if (num_columns <= 1)
	{
		// This is rare case when state transitions are created for the empty rule. Mark all rows of the result set
		// as beginnings of the row ranges. Each range consists of only one row.
		for (int irow1=0; irow1<m_result.NumItems(); ++irow1)
			m_result[irow1].m_transitions[0].same_state_rows = 1;

		// No more processing is needed.
		return(TRUE);
	}

	//
	//  Step 3: Merge identical parsing states.
	// ------------------------------------
	//

	int final_state_range_beg = 0;
	WORD final_state_range_state = m_result[0].m_transitions[num_columns-1].istate;
	while (final_state_range_beg < m_result.NumItems())
	{
		int range_beg_old = final_state_range_beg;
		for (int irow2=final_state_range_beg+1; irow2<m_result.NumItems(); ++irow2)
		{
			// Check if current row begins the new range or not.
			WORD final_state = m_result[irow2].m_transitions[num_columns-1].istate;
			if (final_state != final_state_range_state)
			{
				// Process the intermediate range.
				int final_state_range_len = irow2-final_state_range_beg;
				if (ProcessFinalStatesRange(dlg, final_state_range_beg, final_state_range_len, num_columns) == FALSE)
					return(FALSE);

				// Reassign the variables of the outer loop.
				final_state_range_beg += final_state_range_len;
				final_state_range_state = final_state;
				break;
			}
		}

		if (final_state_range_beg == range_beg_old)
			break;
	}

	// Process the last range of final states.
	assert(final_state_range_state != 0xFFFF);
	int last_final_state_range_len = m_result.NumItems()-final_state_range_beg;
	if (ProcessFinalStatesRange(dlg, final_state_range_beg, last_final_state_range_len, num_columns) == FALSE)
		return(FALSE);

	// All necessary allocations succeeded.
	return(TRUE);
}

bool TLangViewBuildStateTransitionsHelper::CreateStateTransitionsRow(TBasicDialog *dlg, int irule, int isym, WORD pars_stt_at_isym, int num_transition_nodes,
																	WORD action_symbol, TSymbolsArray **syms_buff)
{
	assert(isym >= 0 && num_transition_nodes >= 1);
	assert(isym+num_transition_nodes <= m_grammar->rules[irule].Length()+1);

	// Allocate symbols buffer if needed.
	TSymbolsArray *ptr_buff = NULL;
	if (syms_buff != NULL)
	{
		// Caller requested to allocate the action symbols buffer.
		ptr_buff = new TSymbolsArray();
		if (ptr_buff == NULL)
		{
			dlg->MsgBox(L"Error creating symbols buffer for rule action symbols.");
			return(FALSE);
		}

		// Reserve the space in the buffer for the max possible length because it is already known.
		if (ptr_buff->ReserveSpace(m_grammar->NumTerminals()) == FALSE)
		{
			dlg->MsgBox(L"Error reserving space in the symbols buffer for rule action symbols.");
			delete ptr_buff;
			return(FALSE);
		}

		// Save the passed action symbol into the buffer.
		ptr_buff->AppendItem(action_symbol);
	}

	// Allocate the row prototype.
	TRuleStateTransitionsRow row(ptr_buff);

	// Setup the transition nodes in the new row object.
	WORD stt = pars_stt_at_isym;
	TSymbolsArray &rule_symbols = m_grammar->rules[irule].symbols;
	for (int offs=0; offs<num_transition_nodes; ++offs)
	{
		TRuleStateTransitionNode stt_trans_node = { stt, -1 };
		if (row.m_transitions.AppendItem(stt_trans_node) == FALSE)
		{
			dlg->MsgBox(L"Error appending state transition node to the state transitions row.");
			row.ReleaseObject();
			return(FALSE);
		}

		if (offs < num_transition_nodes-1)
			stt = m_grammar->GetNextParsingState(stt, rule_symbols[isym+offs]);
	}

	// Append the row to the results set.
	if (m_result.AppendItem(row) == FALSE)
	{
		dlg->MsgBox(L"Error appending state transitions row to the data set.");
		row.ReleaseObject();
		return(FALSE);
	}

	if (syms_buff != NULL)
	{
		// Give out the symbols buffer.
		*syms_buff = ptr_buff;
	}

	// Success.
	return(TRUE);
}

bool TLangViewBuildStateTransitionsHelper::ProcessFinalStatesRange(TBasicDialog *dlg, int irow_start, int &num_rows, int num_columns)
{
	assert(irow_start >= 0 && num_rows >= 1);
	assert(irow_start+num_rows <= m_result.NumItems());
	assert(num_columns >= 2);

	// Pick up the action symbols buffer.
	TSymbolsArray*act_syms = m_result[irow_start].m_action_symbols;
	if (act_syms != NULL)
	{
		// The first row contains the action symbols. This means that all other rows should contain
		// identical arrays. Ensure this and release all these duplicate buffers.
		for (int irow1=irow_start+1; irow1<irow_start+num_rows; ++irow1)
		{
			TSymbolsArray*syms_buffer = m_result[irow1].m_action_symbols;
			m_result[irow1].m_action_symbols = NULL;

			assert(syms_buffer != NULL);
			assert(act_syms->IsEqualTo(*syms_buffer) == TRUE);
			delete syms_buffer;
		}
	}

	// Compact ranges of initial states that result in the same state in the second column.
	int second_state_range_beg = irow_start;
	WORD second_state_range_state = m_result[irow_start].m_transitions[1].istate;
	while (second_state_range_beg < irow_start+num_rows)
	{
		int range_beg_old = second_state_range_beg;
		for (int irow2=second_state_range_beg+1; irow2<irow_start+num_rows; ++irow2)
		{
			// Check if current row begins the new range or this is an end of the last range.
			WORD second_state = m_result[irow2].m_transitions[1].istate;
			if (second_state != second_state_range_state)
			{
				// Process the intermediate range.
				int second_state_range_len = irow2-second_state_range_beg;
				if (ProcessInitialStatesRange(dlg, second_state_range_beg, second_state_range_len, num_rows) == FALSE)
					return(FALSE);

				// Reassign the variables of the outer loop.
				second_state_range_beg += second_state_range_len;
				second_state_range_state = second_state;
				break;
			}
		}

		if (second_state_range_beg == range_beg_old)
			break;
	}

	// Process the last range of compatible initial states.
	assert(second_state_range_state != 0xFFFF);
	int second_state_final_range_len = irow_start+num_rows-second_state_range_beg;
	if (ProcessInitialStatesRange(dlg, second_state_range_beg, second_state_final_range_len, num_rows) == FALSE)
		return(FALSE);

	// Process states in the rest of the columns. Note that this operation cannot fail.
	ProcessDestinationStates(irow_start, num_rows, num_columns);
	return(TRUE);
}

bool TLangViewBuildStateTransitionsHelper::ProcessInitialStatesRange(TBasicDialog *dlg, int irow_start, int &num_rows, int &num_rows_total)
{
	assert(irow_start >= 0 && num_rows >= 1);
	assert(irow_start+num_rows <= m_result.NumItems());

	// Check, if it makes sense to arrange states in the leftmost column as a set of subcolumns.
	int matrix_height = num_rows;
	int matrix_width = 1;

	// Allocate space for the initial central column plus verify that there are no action symbol buffers
	// in the passed rows range except maybe for the first row.
	WORD empty_stt = 0xFFFF;
	for (int ir1=irow_start; ir1<irow_start+num_rows; ++ir1)
	{
		if (ir1 != irow_start)
			assert(m_result[ir1].m_action_symbols == NULL);

		m_result[ir1].m_initial_states.AppendItem(empty_stt);
	}

	while (matrix_height > 6 && matrix_height >= 3*matrix_width)
	{
		// Arrange initial states into the bigger number of columns.
		for (int ir2=irow_start; ir2<irow_start+num_rows; ++ir2)
		{
			if (m_result[ir2].m_initial_states.AppendItem(empty_stt) == FALSE)
			{
				dlg->MsgBox(L"Error adding state slot to the array of initial states (1).");
				return(FALSE);
			}

			if (m_result[ir2].m_initial_states.AppendItem(empty_stt) == FALSE)
			{
				dlg->MsgBox(L"Error adding state slot to the array of initial states (2).");
				return(FALSE);
			}
		}

		matrix_width += 2;
		matrix_height = (num_rows+(matrix_width-1))/matrix_width;
		if (matrix_height == 0)
			matrix_height = 1;
	}

	// Move initial states into the upper subset of rows.
	for (int row_offs=0; row_offs<num_rows; ++row_offs)
	{
		// Pick up the initial state and wipe it.
		WORD stt = m_result[irow_start+row_offs].m_transitions[0].istate;
		m_result[irow_start+row_offs].m_transitions[0].istate = 0xFFFF;

		// Store the state.
		int dest_row = irow_start+(row_offs%matrix_height);
		m_result[dest_row].m_initial_states[row_offs/matrix_height] = stt;
	}

	// Get rid of the extra rows.
	while (num_rows > matrix_height)
	{
		m_result.ReleaseItem(irow_start+matrix_height);
		num_rows--;
		num_rows_total--;
	}

	// Success.
	return(TRUE);
}

void TLangViewBuildStateTransitionsHelper::ProcessDestinationStates(int irow_start, int num_rows, int num_columns)
{
	assert(irow_start >= 0 && num_rows >= 1);
	assert(irow_start+num_rows <= m_result.NumItems());

	// Process columns starting from the second column because the first column was processed
	// with different algorithm that possibly converted it into the matrix of states.
	for (int iclmn=1; iclmn<num_columns; ++iclmn)
	{
		// Look for the range of identical states in the current column.
		int range_beg = irow_start;
		WORD range_state = m_result[irow_start].m_transitions[iclmn].istate;
		while (range_beg < irow_start+num_rows)
		{
			int range_beg_old = range_beg;
			for (int irow1=range_beg+1; irow1<irow_start+num_rows; ++irow1)
			{
				WORD curr_state = m_result[irow1].m_transitions[iclmn].istate;
				if (curr_state != range_state)
				{
					// Store the length of the range into the first row.
					int range_len = irow1-range_beg;
					m_result[range_beg].m_transitions[iclmn].same_state_rows = range_len;

					// Reassign the variables of the outer loop.
					range_beg += range_len;
					range_state = curr_state;
					break;
				}
			}

			if (range_beg == range_beg_old)
				break;
		}

		// Assign the length of the last range.
		assert(range_state != 0xFFFF);
		m_result[range_beg].m_transitions[iclmn].same_state_rows = irow_start+num_rows-range_beg;
	}
}

int TLangViewBuildStateTransitionsHelper::CompareTransitionRows(void *raw_ctx, const TRuleStateTransitionsRow *row1, const TRuleStateTransitionsRow *row2)
{
	// These two calls are tricky because rows may be moved before the call to this function.
	((TRuleStateTransitionsRow*)row1)->FixObjectMove();
	((TRuleStateTransitionsRow*)row2)->FixObjectMove();
	assert(row1->m_transitions.NumItems() == row2->m_transitions.NumItems());

	// Compare parsing states in columns from right to left.
	int clmn_end_inx = row1->m_transitions.NumItems()-1;
	for (int iclmn=clmn_end_inx; iclmn >= 0; --iclmn)
	{
		WORD stt1 = row1->m_transitions[iclmn].istate;
		WORD stt2 = row2->m_transitions[iclmn].istate;

		if (stt1 < stt2)
			return(-1);
		if (stt1 > stt2)
			return(1);
	}

	// Passed rows are identical while all rows in the matrix should be different.
	assert(FALSE);
	return(0);
}

// ---------------------------------------------------------------------------
//    ===============  TLangViewGrammarTreesHelper  ==================
// ---------------------------------------------------------------------------

TLangViewTreeNode *TLangViewGrammarTreesHelper::CreateGrammarOverviewHier(TLangViewTreeNode *parent_node, TGrammar *grammar, bool show_inividual_grm_conflicts)
{
	// Parent node can be NULL, while the passed grammar cannot.
	assert(grammar != NULL);
	WORD igrammar = (WORD)grammar->GetGrammarIndex();

	// Create node for the passed grammar itself.
	TLangViewSpecialTreeNode *grm_node = new TLangViewSpecialTreeNode(parent_node);
	if (grm_node == NULL)
	{
		m_dialog.MsgBox(L"Out of memory while creating the grammar node.");
		return(NULL);
	}

	// Set the props of the grammar.
	grm_node->SetLexItemClickOwner();
	grm_node->Item().SetGrammarProps(igrammar);

	// Check for rules.
	if (grammar->NumRules() > 0)
	{
		TLangViewSpecialTreeNode *rules_node = new TLangViewSpecialTreeNode(grm_node);
		if (rules_node == NULL)
		{
			m_dialog.MsgBox(L"Out of memory while creating the rules category node.");
			return(grm_node);
		}

		rules_node->SetLexItemClickOwner();
		rules_node->Item().SetGrammarCategoryProps(slit_grammar_rules, igrammar, grammar->NumRules());
	}

	// Check for parsing states.
	if (grammar->NumParsingStates() > 0)
	{
		TLangViewSpecialTreeNode *states_node = new TLangViewSpecialTreeNode(grm_node);
		if (states_node == NULL)
		{
			m_dialog.MsgBox(L"Out of memory while creating the parsing states category node.");
			return(grm_node);
		}

		states_node->SetLexItemClickOwner();
		states_node->Item().SetGrammarCategoryProps(slit_parsing_states, igrammar, grammar->NumParsingStates());
	}

	// Check for expected conflicts.
	if (grammar->NumXpctConflicts() > 0)
	{
		// Create node that will represent all xpct conflicts of this grammar.
		TLangViewSpecialTreeNode *xpcts_node = new TLangViewSpecialTreeNode(grm_node);
		if (xpcts_node == NULL)
		{
			m_dialog.MsgBox(L"Out of memory while creating the expected conflicts category node.");
			return(grm_node);
		}

		// Set the props of xpct conflicts category node.
		xpcts_node->SetLexItemClickOwner();
		xpcts_node->Item().SetGrammarCategoryProps(slit_xpct_conflicts, igrammar, grammar->NumXpctConflicts());

		// Iterate the expected conflicts.
		for (int ixpct=0; ixpct<grammar->NumXpctConflicts(); ++ixpct)
		{
			TLangViewSpecialTreeNode *node = new TLangViewSpecialTreeNode(xpcts_node);
			if (node == NULL)
			{
				m_dialog.MsgBox(L"Out of memory while creating the expected conflict instance node.");
				return(grm_node);
			}

			// Set the props.
			node->SetLexItemClickOwner();
			node->Item().SetXpctConflictProps(igrammar, ixpct);

			if (grammar->xpct_conflicts[ixpct].nested_grammar != NULL)
			{
				// Current xpct conflicts owns nested grammar.
				CreateGrammarOverviewHier(node, grammar->xpct_conflicts[ixpct].nested_grammar, show_inividual_grm_conflicts);
			}
		}
	}

	// Classify the grammar conflicts if any.
	int num_conflicts = grammar->NumConflicts();
	int num_ambiguous = 0, num_assigned = 0, num_unexpected = 0;
	for (int iconflict=0; iconflict<num_conflicts; ++iconflict)
	{
		short ixpct = grammar->conflicts[iconflict].expected_conflict_inx;
		if (ixpct <= -2)
		{
			// Current grammar conflict is compatible with more than one expected conflict.
			num_ambiguous++;
		}
		else if (ixpct == -1)
		{
			// Current grammar conflict is not associated with any expected conflict.
			num_unexpected++;
		}
		else
		{
			// Current grammar conflict is owned by single expected conflict.
			num_assigned++;
		}
	}

	assert(num_ambiguous+num_assigned+num_unexpected == num_conflicts);

	// Sum up the classification.
	TLangViewSpecialTreeNode *categories_parent = grm_node;
	if ((num_ambiguous != 0 && num_ambiguous != num_conflicts) || (num_assigned != 0 && num_assigned != num_conflicts) || (num_unexpected != 0 && num_unexpected != num_conflicts))
	{
		// Conflicts of the grammar belong to more than one category.
		categories_parent = new TLangViewSpecialTreeNode(grm_node);
		if (categories_parent == NULL)
		{
			m_dialog.MsgBox(L"Out of memory while creating the all grammar conflicts category node.");
			return(grm_node);
		}

		categories_parent->SetLexItemClickOwner();
		categories_parent->Item().SetGrammarCategoryProps(slit_all_grammar_conflicts, igrammar, num_conflicts);
	}

	if (num_ambiguous > 0)
	{
		// Ambiguous grammar conflicts are present.
		TLangViewSpecialTreeNode *node = new TLangViewSpecialTreeNode(categories_parent);
		if (node == NULL)
		{
			m_dialog.MsgBox(L"Out of memory while creating the ambiguous grammar conflicts category node.");
			return(grm_node);
		}

		node->SetLexItemClickOwner();
		node->Item().SetGrammarCategoryProps(slit_ambig_grm_conflicts, igrammar, num_ambiguous);

		if (show_inividual_grm_conflicts == TRUE)
		{
			if (CreateGrammarConflictInstances(node, grammar) == FALSE)
				return(grm_node);
		}
	}

	if (num_assigned > 0)
	{
		// Assigned grammar conflicts are present.
		TLangViewSpecialTreeNode *node = new TLangViewSpecialTreeNode(categories_parent);
		if (node == NULL)
		{
			m_dialog.MsgBox(L"Out of memory while creating the assigned grammar conflicts category node.");
			return(grm_node);
		}

		node->SetLexItemClickOwner();
		node->Item().SetGrammarCategoryProps(slit_assigned_grm_conflicts, igrammar, num_assigned);

		if (show_inividual_grm_conflicts == TRUE)
		{
			if (CreateGrammarConflictInstances(node, grammar) == FALSE)
				return(grm_node);
		}
	}

	if (num_unexpected > 0)
	{
		// Unexpected grammar conflicts are present.
		TLangViewSpecialTreeNode *node = new TLangViewSpecialTreeNode(categories_parent);
		if (node == NULL)
		{
			m_dialog.MsgBox(L"Out of memory while creating the unexpected grammar conflicts category node.");
			return(grm_node);
		}

		node->SetLexItemClickOwner();
		node->Item().SetGrammarCategoryProps(slit_unexp_grm_conflicts, igrammar, num_unexpected);

		if (show_inividual_grm_conflicts == TRUE)
		{
			if (CreateGrammarConflictInstances(node, grammar) == FALSE)
				return(grm_node);
		}
	}

	// All allocations succeeded at least on the current level.
	return(grm_node);
}

void TLangViewGrammarTreesHelper::CreateNonTerminalDependenciesTree(TLangViewSpecialTreeNode *parent_node, WORD non_term, bool show_terminals)
{
	assert(non_term >= NonTerminalsBase && non_term < NonTerminalsBase+m_grammar.NumNonTerminals());
	WORD non_term_sect = m_grammar.non_terminals[non_term-NonTerminalsBase].rules_sect;

	// Create the initial non terminal node.
	TLangViewNonTerminalTreeNode *initial_node = (TLangViewNonTerminalTreeNode*)CreateSymbolTreeNode(parent_node, non_term);
	if (initial_node == NULL)
		return;

	// Allocate list of non terminals to process and add initial item there.
	TNonTerminalTreeNodePtrsArray list_to_do;
	list_to_do.AppendItem(initial_node);

	// Process the list. Note that this list can grow inside the loop.
	int inx_to_process = 0;
	while (inx_to_process < list_to_do.NumItems())
	{
		TLangViewNonTerminalTreeNode *node = list_to_do[inx_to_process++];
		WORD curr_non_term = node->GetSymbolValue();

		for (TRulesIterator iter(m_grammar); iter; ++iter)
		{
			TGrammarRule &rule = iter.CurrRule();
			if (rule.non_term != curr_non_term)
				continue;

			// Current rule defines the current non terminal from the "list to do".
			for (int isym=0; isym<rule.Length(); ++isym)
			{
				WORD sym = rule.symbols[isym];
				if (sym < NonTerminalsBase && show_terminals == FALSE)
					continue;

				// Check, if current symbol is already present on the current layer or not.
				if (IsSymbolPresentAmongChildren(node, sym) == TRUE)
				{
					// Layer should contain all symbols that current non terminal depends on.
					// But each symbol should be present only once.
					continue;
				}

				// This symbol is new on the current layer. Add this symbol to the current layer.
				TLangViewTreeNode *new_child_node = CreateSymbolTreeNode(node, sym);
				if (new_child_node == NULL)
					return;

				// Check, if symbol should be added to the "list to do" or not.
				if (sym >= NonTerminalsBase && m_grammar.non_terminals[sym-NonTerminalsBase].rules_sect == non_term_sect)
				{
					// Current non terminal belongs to the same rules section.
					CheckAddToTheToDoList(list_to_do, (TLangViewNonTerminalTreeNode*)new_child_node);
				}
			}
		}
	}
}

void TLangViewGrammarTreesHelper::CreateNonTerminalRuleUsesTree(TLangViewSpecialTreeNode *parent_node, WORD non_term)
{
	assert(non_term >= NonTerminalsBase && non_term < NonTerminalsBase+m_grammar.NumNonTerminals());
	WORD non_term_sect = m_grammar.non_terminals[non_term-NonTerminalsBase].rules_sect;

	// Create the initial non terminal node.
	TLangViewNonTerminalTreeNode *initial_node = (TLangViewNonTerminalTreeNode*)CreateSymbolTreeNode(parent_node, non_term);
	if (initial_node == NULL)
		return;

	// Allocate list of non terminals to process and add initial item there.
	TNonTerminalTreeNodePtrsArray list_to_do;
	list_to_do.AppendItem(initial_node);

	// Process the list. Note that this list can grow inside the loop.
	int inx_to_process = 0;
	while (inx_to_process < list_to_do.NumItems())
	{
		TLangViewNonTerminalTreeNode *node = list_to_do[inx_to_process++];
		WORD curr_non_term = node->GetSymbolValue();

		for (TRulesIterator iter(m_grammar); iter; ++iter)
		{
			TGrammarRule &rule = iter.CurrRule();
			if (rule.symbols.IsSymInBuffer(curr_non_term) == FALSE)
				continue;

			// Current rule uses the current non terminal from the "list to do".
			// Check, if non terminal of this rule is already present on the current layer or not.
			if (IsSymbolPresentAmongChildren(node, rule.non_term) == TRUE)
			{
				// The layer should contain all symbols that use the current non terminal.
				// But each symbol should be present only once.
				continue;
			}

			// This symbol is new on the current layer. Add this symbol to the current layer.
			TLangViewTreeNode *new_child_node = CreateSymbolTreeNode(node, rule.non_term);
			if (new_child_node == NULL)
				return;

			// Check, if this non terminal should be added to the "list to do" or not.
			if (m_grammar.non_terminals[rule.non_term-NonTerminalsBase].rules_sect == non_term_sect)
			{
				// Current symbol belongs to the same rules section.
				CheckAddToTheToDoList(list_to_do, (TLangViewNonTerminalTreeNode*)new_child_node);
			}
		}
	}
}

void TLangViewGrammarTreesHelper::CreatePathsTreeForXpctConflict(TLangViewSpecialTreeNode *parent_node, int cat_ixpct)
{
	int num_xpcts = m_grammar.NumXpctConflicts();

	if (cat_ixpct < 0)
	{
		// Setup props to the category node that will describe all available expected conflicts.
		parent_node->SetLexItemClickOwner();
		parent_node->Item().SetGrammarCategoryProps(slit_xpct_conflicts, (WORD)m_grammar.GetGrammarIndex(), num_xpcts);

		// Iterate all expected conflicts.
		for (int ixpct=0; ixpct<num_xpcts; ++ixpct)
		{
			TLangViewSpecialTreeNode *xpct_node = new TLangViewSpecialTreeNode(parent_node);
			if (xpct_node == NULL)
			{
				m_dialog.MsgBox(L"Out of memory while creating the expected grammar conflict instance node.");
				return;
			}

			// Add grammar conflicts, that belong to the current expected conflict.
			if (CreateXpctConflictTree(xpct_node, ixpct) == FALSE)
				return;
		}
	}
	else
	{
		// Only one expected conflict should be processed.
		assert(cat_ixpct < num_xpcts);
		CreateXpctConflictTree(parent_node, cat_ixpct);
	}
}

void TLangViewGrammarTreesHelper::CreatePathsTreeForGrammarConflict(TLangViewSpecialTreeNode *parent_node, int cat_iconflict)
{
	WORD igrammar = (WORD)m_grammar.GetGrammarIndex();

	if (cat_iconflict <= -4)
	{
		// Set up the props to the passed parent.
		parent_node->SetLexItemClickOwner();
		parent_node->Item().SetGrammarCategoryProps(slit_all_grammar_conflicts, igrammar, m_grammar.NumConflicts());

		// Iterate all grammar conflicts.
		for (int inx=0; inx<m_grammar.NumConflicts(); ++inx)
		{
			if (CreateGrammarConflictSubtree(parent_node, inx) == FALSE)
				return;
		}
	}
	else if (cat_iconflict < 0)
	{
		// Display conflicts that belong to the certain conflicts category.
		int cnt_instances = 0;
		int iconflict = m_grammar.GetFirstGrammarConflictInx(cat_iconflict);
		while (iconflict >= 0)
		{
			if (CreateGrammarConflictSubtree(parent_node, iconflict) == FALSE)
				return;

			// Shift the loop variables.
			iconflict = m_grammar.GetNextGrammarConflictInx(cat_iconflict, iconflict);
			cnt_instances++;
		}

		// Once the number of instances is known, set up the props of the category object.
		parent_node->SetLexItemClickOwner();
		parent_node->Item().SetGrammarCategoryProps(GetCategoryTypeFromCategoryCode(cat_iconflict), igrammar, cnt_instances);
	}
	else
	{
		// Caller wants to get info on the single grammar conflict.
		assert(cat_iconflict < m_grammar.NumConflicts());
		CreateGrammarConflictTree(parent_node, cat_iconflict);
	}
}

void TLangViewGrammarTreesHelper::CreatePathsTreeForConflictingAction(TLangViewSpecialTreeNode *parent_node, int iconflict, int iaction)
{
	// Plain call the worker method.
	CreateConflictingActionTree(parent_node, iconflict, iaction);
}

void TLangViewGrammarTreesHelper::CreateDerivationPathAst(TLangViewTreeNode *parent_node, TGrammarDerivationPath &path, WORD conflicting_symbol_value, TLangViewTreeNode *&conflicting_symbol_prm)
{
	assert(parent_node != NULL);

	// Object for conflicting symbol is not created yet.
	conflicting_symbol_prm = NULL;

	// The passed path should be not empty.
	if (path.derv_path_len <= 0)
		return;

	// Create initial subtree for the initial rule.
	TGrammarRule &initial_rule = m_grammar.rules[path.derv_steps[0].irule];
	TLangViewTreeNode *root = CreateAstFragmentForRule(path.derv_steps[0].irule);
	if (root == NULL)
	{
		// Eror is already displayed.
		return;
	}

	// Look for the appropriate child in the rule. Inside this method the variable curr_node describes postion
	// in the rule in front of this symbol.
	TLangViewTreeNode *curr_node = (TLangViewTreeNode*)(root->GetFirstChild(TRUE)->GetNextSibling(TRUE));
	assert(curr_node != NULL);

	if (path.derv_steps[0].isym > 0)
	{
		int initial_sym_inx = path.derv_steps[0].isym;
		if (initial_sym_inx == initial_rule.Length())
			initial_sym_inx--;

		while (initial_sym_inx > 0)
		{
			curr_node = (TLangViewTreeNode*)(curr_node->GetNextSibling(TRUE));
			assert(curr_node != NULL);
			initial_sym_inx--;
		}
	}

	// History of processing the non_term_defn_rule and step_up_the_tree slots.
	TLangViewTreeNode *non_term_defn_root = NULL;
	int non_term_defn_layer = 0;

	// Track the conflicting symbol search.
	TLangViewTreeNode *conflicting_symbol = NULL;
	if (path.derv_steps[0].origin == rpo_rule_call_place)
	{
		// This is initial placement in the analysis of the action "shift". Highlight link that points into this
		// symbol. Even if this symbol stays in the beginning of the rule, it will have rule label in front of it.
		TLangViewTreeNode *prev_node = (TLangViewTreeNode*)(curr_node->GetPrevSibling(TRUE));
		assert(prev_node != NULL);
		HighlightRightLink(prev_node);
	}

	// In the rare case of the rpo_axioma_action_sym/rpo_axioma_action_ctx steps the object for this symbol
	// is created inside the loop but this object should be attached to the tree when the loop is finished.
	TLangViewTreeNode *axioma_action_symbol = NULL;
	bool axioma_action_sym_non_ctx = FALSE;

	//
	// Iterate rest of the derivation path steps starting from the step[1]. Note that the most
	// important variables of the loop are:
	//
	//		root			--	Current root of the AST.
	//		curr_node	--	Node that moves from the initial position up to the end of the path
	//						pointing at the part of the AST should be updated on each step.
	//
	for (int i=1; i<path.derv_path_len; ++i)
	{
		TFullRulePosition &cfrp = path.derv_steps[i];
		TGrammarRule &rule = m_grammar.rules[cfrp.irule];

		switch (cfrp.origin)
		{
			case rpo_initial_placement:
					{
						// This type of path step should not be present in the middle of the path.
						assert(FALSE);
					}
					break;

			case rpo_rule_start_state:
			case rpo_start_state_ctx:
					{
						// This step does not need processing. Do real processing in the next step.
					}
					break;

			case rpo_rule_call_place:
			case rpo_call_place_ctx:
					{
						// Create new layer on top of the current tree.
						assert(curr_node->GetOwner() == root);
						TLangViewTreeNode *new_root = CreateAstFragmentForRule(cfrp.irule, NULL, cfrp.isym, root);
						if (new_root == NULL)
						{
							delete root;
							return;
						}

						if (cfrp.origin == rpo_rule_call_place)
						{
							HighlightUpperLink(curr_node);
						}

						// Shift the current node up.
						curr_node = root;
						root = new_root;
					}
					break;

			case rpo_step_over_symbol:
			case rpo_step_over_non_term:
					{
						if (cfrp.isym < rule.Length())
						{
							TLangViewTreeNode *new_curr_node = (TLangViewTreeNode*)(curr_node->GetNextSibling(TRUE));
							assert(new_curr_node != NULL);

							if (non_term_defn_layer == 0)
							{
								HighlightRightLink(curr_node);
							}

							// Shift the current node right.
							curr_node = new_curr_node;
						}
					}
					break;

			case rpo_non_term_defn_rule:
					{
						// Create new layer at the bottom of the tree.
						if (CreateAstFragmentForRule(cfrp.irule, curr_node) == NULL)
						{
							delete root;
							return;
						}

						if (non_term_defn_layer++ == 0)
						{
							// This beginning of the descending tree. Store this node.
							non_term_defn_root = curr_node;
						}

						// Shift the current node down. Note that non term defn slot always sets position to the beginning of the rule.
						curr_node = (TLangViewTreeNode*)(curr_node->GetFirstChild(TRUE)->GetNextSibling(TRUE));
						assert(curr_node != NULL);
					}
					break;

			case rpo_step_up_the_tree:
			case rpo_step_up_ctx:
					{
						// Shift the current node up.
						curr_node = curr_node->GetOwner();
						assert(curr_node != NULL);

						if (cfrp.origin == rpo_step_up_the_tree)
						{
							// Update vars that track processing of the non term defn subtree.
							assert(non_term_defn_layer > 0);
							if (--non_term_defn_layer == 0)
							{
								// The whole tree was travelled. The conflicting symbol was not found. Ensure that path came
								// to the same node where the non term defn subtree was started.
								assert(curr_node == non_term_defn_root);
								non_term_defn_root = NULL;
							}
						}

						// It is also necessary to shift the node right, if the right node is existing.
						TLangViewTreeNode *next_sibl = (TLangViewTreeNode*)(curr_node->GetNextSibling(TRUE));
						if (next_sibl != NULL)
						{
							if (cfrp.origin == rpo_step_up_the_tree && non_term_defn_layer == 0)
							{
								HighlightRightLink(curr_node);
							}

							curr_node = next_sibl;
						}
					}
					break;

			case rpo_axioma_action_sym:
			case rpo_axioma_action_ctx:
					{
						// Ensure correct state of the path processing.
						TLangViewTreeNode *axioma_sym = curr_node->GetOwner();
						assert(axioma_sym != NULL && axioma_sym->GetItemType() == litp_non_terminal);
						assert(axioma_sym->GetSymbolValue() == m_grammar.axioma_ident);
						assert(axioma_sym == root);

						if (cfrp.origin == rpo_axioma_action_sym)
							assert(cfrp.action_sym == conflicting_symbol_value);

						// Create lexema object that will represent the rule action symbol.
						axioma_action_symbol = CreateSymbolTreeNode(NULL, cfrp.action_sym);
						if (axioma_action_symbol == NULL)
						{
							delete root;
							return;
						}

						// Setup the object.
						axioma_action_symbol->SetSelectedStateEx2(FALSE, FALSE, -1, -1);

						if (cfrp.origin == rpo_axioma_action_sym)
						{
							assert(conflicting_symbol == NULL);
							conflicting_symbol = axioma_action_symbol;

							axioma_action_sym_non_ctx = TRUE;
							HighlightUpperLink(curr_node);
						}
						else
						{
							// Conflicting symbol should be already spotted.
							assert(conflicting_symbol != NULL);
						}
					}
					break;

			case rpo_step_over_ctx:
					{
						if (conflicting_symbol == NULL)
						{
							// This is beginning of the cxt part of the path. This step should be present only when current node
							// is the conflicting symbol. Ensure this and save the pointer.
							assert(curr_node->GetItemType() == litp_lexema);
							assert(curr_node->GetSymbolValue() == conflicting_symbol_value);
							conflicting_symbol = curr_node;

							if (non_term_defn_layer > 0)
							{
								HighlightDescendingPath(non_term_defn_root, curr_node);
							}
						}

						// Shift current node to the end of the current rule.
						for(;;)
						{
							TLangViewTreeNode *next_sibl = (TLangViewTreeNode*)(curr_node->GetNextSibling(TRUE));
							if (next_sibl == NULL)
								break;

							curr_node = next_sibl;
						}
					}
					break;

			default:
				{
					// Bogus slot type.
					assert(FALSE);
				}
				break;
		}
	}

	if (conflicting_symbol == NULL)
	{
		// The path does not include the extra context.
		assert(curr_node->GetItemType() == litp_lexema);
		assert(curr_node->GetSymbolValue() == conflicting_symbol_value);
		conflicting_symbol = curr_node;

		if (non_term_defn_layer > 0)
		{
			HighlightDescendingPath(non_term_defn_root, curr_node);
		}
	}

	// Construct the top of the tree.
	parent_node->AdoptChild(root);
	if (axioma_action_symbol != NULL)
	{
		parent_node->AdoptChild(axioma_action_symbol);
		HighlightRightLink(root, axioma_action_sym_non_ctx);
	}

	// Give out the pointer to the conflicting symbol.
	assert(conflicting_symbol != NULL);
	conflicting_symbol_prm = conflicting_symbol;
}

bool TLangViewGrammarTreesHelper::CreateGrammarConflictInstances(TLangViewSpecialTreeNode *parent_node, TGrammar *grammar)
{
	WORD igrammar = (WORD)grammar->GetGrammarIndex();
	for (int iconflict=0; iconflict<grammar->NumConflicts(); ++iconflict)
	{
		short ixpct = grammar->conflicts[iconflict].expected_conflict_inx;

		// Category of requested conflicts is passed in the type of the passed parent node.
		bool show_conflict = FALSE;
		switch (parent_node->Item().GetSpecType())
		{
			case slit_all_grammar_conflicts:
					{
						show_conflict = TRUE;
					}
					break;

			case slit_ambig_grm_conflicts:
					{
						if (ixpct <= -2)
							show_conflict = TRUE;
					}
					break;

			case slit_assigned_grm_conflicts:
					{
						if (ixpct >= 0)
							show_conflict = TRUE;
					}
					break;

			case slit_unexp_grm_conflicts:
					{
						if (ixpct == -1)
							show_conflict = TRUE;
					}
					break;

			default:
				assert(FALSE);
				break;
		}

		if (show_conflict == TRUE)
		{
			TLangViewSpecialTreeNode *node = new TLangViewSpecialTreeNode(parent_node);
			if (node == NULL)
			{
				m_dialog.MsgBox(L"Out of memory while creating the grammar conflict instance node.");
				return(FALSE);
			}

			// Set the props.
			node->SetLexItemClickOwner();
			node->Item().SetGrammarConflictProps(igrammar, iconflict);
		}
	}

	// Success.
	return(TRUE);
}

bool TLangViewGrammarTreesHelper::CreateXpctConflictTree(TLangViewSpecialTreeNode *parent_node, int ixpct)
{
	// Passed parent node should not be not NULL.
	assert(parent_node != NULL);
	assert(ixpct >= 0 && ixpct < m_grammar.NumXpctConflicts());

	// Setup the props of the parent.
	parent_node->SetLexItemClickOwner();
	parent_node->Item().SetXpctConflictProps((WORD)m_grammar.GetGrammarIndex(), ixpct);

	// Iterate participating grammar conflicts.
	int iconflict = m_grammar.GetFirstGrammarConflictInx(ixpct);
	while (iconflict >= 0)
	{
		if (CreateGrammarConflictSubtree(parent_node, iconflict) == FALSE)
		{
			// The error dialog should be already displayed.
			return(FALSE);
		}

		iconflict = m_grammar.GetNextGrammarConflictInx(ixpct, iconflict);
	}

	// Success.
	return(TRUE);
}

bool TLangViewGrammarTreesHelper::CreateGrammarConflictSubtree(TLangViewSpecialTreeNode *parent_node, int iconflict)
{
	TLangViewSpecialTreeNode *node = new TLangViewSpecialTreeNode(parent_node);
	if (node == NULL)
	{
		m_dialog.MsgBox(L"Out of memory while creating the grammar conflict instance node.");
		return(FALSE);
	}

	// Call the worker method for the node that was just created.
	return(CreateGrammarConflictTree(node, iconflict));
}

bool TLangViewGrammarTreesHelper::CreateGrammarConflictTree(TLangViewSpecialTreeNode *parent_node, int iconflict)
{
	// Passed parent node should not be not NULL.
	assert(parent_node != NULL);
	assert(iconflict >= 0 && iconflict < m_grammar.NumConflicts());

	// Setup the props of the parent.
	parent_node->SetLexItemClickOwner();
	parent_node->Item().SetGrammarConflictProps((WORD)m_grammar.GetGrammarIndex(), iconflict);

	// Iterate conflicting actions.
	int num_actions = m_grammar.conflicts[iconflict].NumActions();
	for (int iaction=0; iaction<num_actions; ++iaction)
	{
		TLangViewSpecialTreeNode *action_node = new TLangViewSpecialTreeNode(parent_node);
		if (action_node == NULL)
		{
			m_dialog.MsgBox(L"Unable to create LexItem for the conflicting action object.");
			return(FALSE);
		}

		// Attach subtree of paths if any.
		if (CreateConflictingActionTree(action_node, iconflict, iaction) == FALSE)
		{
			// The error dialog should be already displayed.
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

bool TLangViewGrammarTreesHelper::CreateConflictingActionTree(TLangViewSpecialTreeNode *parent_node, int iconflict, int iaction)
{
	// Passed parent node should not be not NULL.
	assert(parent_node != NULL);

	// Passed indexes should be valid also.
	assert(iconflict >= 0 && iconflict < m_grammar.NumConflicts());
	assert(iaction >= 0 && iaction < m_grammar.conflicts[iconflict].NumActions());

	WORD igrammar = (WORD)m_grammar.GetGrammarIndex();
	TGrammarConflict &cfct = m_grammar.conflicts[iconflict];

	// Fill in the props into the parent conflicting action object.
	parent_node->SetLexItemClickOwner();
	parent_node->Item().SetConflictingActionProps(igrammar, (WORD)iconflict, (WORD)iaction, cfct.conflicting_actions[iaction]);

	// Check if passed conflict contains the analysis results or not. At this point it is known for sure that grammar
	// is not NULL and that it is in the full success state. At the same time it may have analysis results and it also
	// may have no results.
	if (cfct.analysis_results.NumItems() == 0 || cfct.analysis_results.NumItems() != cfct.NumActions())
	{
		// There are no analysis results or the number of analysis result records is bogus.
		return(TRUE);
	}

	// Check, if paths are avalilable or not. Note that this code is not checking the analysis result code.
	// Only presence of the derivation paths is important.
	TGrammarDerivationPathsArray &paths = cfct.analysis_results[iaction].m_derivation_paths;
	if (paths.NumItems() == 0)
	{
		// There are no paths to display.
		return(TRUE);
	}

	// Iterate available derivation paths.
	for (int ipath=0; ipath<paths.NumItems(); ++ipath)
	{
		// Pick up the path info.
		int path_len = paths[ipath].derv_path_len;
		assert(path_len > 0);
		TFullRulePosition *steps = paths[ipath].derv_steps;

		// Instantiate missing parts of the path.
		bool new_path = FALSE;
		bool shared_path = FALSE;
		TLangViewSpecialTreeNode *curr_owner = parent_node;
		for (int istep=0; istep<path_len; ++istep)
		{
			TLangViewSpecialTreeNode *curr_node = NULL;
			TLangViewSpecialTreeNode *chld = (TLangViewSpecialTreeNode*)(curr_owner->GetFirstChild(TRUE));
			while (chld != NULL && chld->Item().GetSpecType() != slit_derivation_path)
			{
				// Compare contents of the curr child to of the current derivation step.
				if (chld->Item().GetRuleIndex() == steps[istep].irule && chld->Item().GetRuleSymIndex() == steps[istep].isym)
				{
					// Passed derivation path shares some nodes with other path/paths that is/are already present in the tree.
					shared_path = TRUE;
					curr_node = chld;
					break;
				}

				chld = (TLangViewSpecialTreeNode*)(chld->GetNextSibling(TRUE));
			}

			if (curr_node == NULL)
			{
				// Current derivation step is not present in the tree.
				curr_node = new TLangViewSpecialTreeNode(curr_owner);
				if (curr_node == NULL)
				{
					m_dialog.MsgBox(L"Cannot allocate derivation path step object.");
					return(FALSE);
				}

				curr_node->SetLexItemClickOwner();
				curr_node->Item().SetRulePositionProps(igrammar, steps[istep].irule, steps[istep].isym, steps[istep].action_sym);
				new_path = TRUE;
			}

			curr_owner = curr_node;
		}

		// All paths in the array of paths should be different.
		assert(new_path == TRUE);

		// Add final derivation path object.
		TLangViewSpecialTreeNode *path_label_node = new TLangViewSpecialTreeNode(curr_owner);
		if (path_label_node == NULL)
		{
			m_dialog.MsgBox(L"Cannot allocate derivation path label object.");
			return(FALSE);
		}

		// Fill in the props of the current path label.
		path_label_node->SetLexItemClickOwner();
		path_label_node->Item().SetDerivationPathProps(igrammar, iconflict, iaction, ipath);
	}

	// Full success.
	return(TRUE);
}

TLangViewTreeNode *TLangViewGrammarTreesHelper::CreateAstFragmentForRule(WORD irule, TLangViewTreeNode *non_term_prm, WORD imark, TLangViewTreeNode *mark_item)
{
	// Pick up the rule.
	TGrammarRule &rule = m_grammar.rules[irule];

	TLangViewTreeNode *non_term = non_term_prm;
	if (non_term == NULL)
	{
		// Create the root of the subtree.
		non_term = CreateSymbolTreeNode(NULL, rule.non_term);
		if (non_term == NULL)
			return(NULL);
	}

	// Create label with the rule number.
	TLangViewSpecialTreeNode *rule_label = new TLangViewSpecialTreeNode(non_term);
	if (rule_label == NULL)
	{
		m_dialog.MsgBox(L"Out of memory while creating rule label LexItem.");
		if (non_term_prm == NULL)
		{
			// Non terminal of the rule was created locally. Relase it.
			delete non_term;
		}

		return(NULL);
	}

	// Setup the props of the rule label.
	rule_label->SetLexItemClickOwner();
	rule_label->Item().SetRuleLabelProps(m_grammar.GetGrammarIndex(), irule);
	rule_label->SetSelectedStateEx(FALSE, FALSE, TRUE, FALSE);

	if (rule.Length() == 0)
	{
		// Display empty symbol as the rule right hand side.
		assert(imark == 0xFFFF);
		TLangViewSpecialTreeNode *empty_symbol = new TLangViewSpecialTreeNode(non_term, 0, slit_empty_symbol);
		if (empty_symbol == NULL)
		{
			m_dialog.MsgBox(L"Out of memory while creating LexItem for empty terminal symbol.");
			if (non_term_prm == NULL)
			{
				// Non terminal of the rule was created locally. Relase it.
				delete non_term;
			}

			return(NULL);
		}

		empty_symbol->SetLexItemClickOwner();
	}
	else
	{
		if (imark == rule.Length())
		{
			// In this method position after the last symbol is the same to the position in front of the last symbol.
			imark--;
		}

		// Create children of the rule non terminal.
		for (int isym=0; isym<rule.Length(); ++isym)
		{
			WORD sym = rule.symbols[isym];
			TLangViewTreeNode *sym_item;
			if (isym == imark)
			{
				// Adopt the passed child.
				assert(mark_item != NULL && mark_item->GetItemType() == litp_non_terminal);
				assert(mark_item->GetSymbolValue() == sym);
				non_term->AdoptChild(mark_item);
				sym_item = mark_item;
			}
			else
			{
				// Create the brand new LexItem.
				sym_item = CreateSymbolTreeNode(non_term, rule.symbols[isym]);
				if (sym_item == NULL)
				{
					if (non_term_prm == NULL)
					{
						// Non terminal of the rule was created locally. Relase it.
						delete non_term;
					}

					return(NULL);
				}
			}

			if (isym != rule.Length()-1)
				sym_item->SetSelectedStateEx(TRUE, FALSE, TRUE, FALSE);
		}
	}

	// Success.
	return(non_term);
}

TLangViewTreeNode *TLangViewGrammarTreesHelper::CreateSymbolTreeNode(TLangViewTreeNode *owner, WORD sym)
{
	// Create the lex item.
	TLangViewTreeNode *lex_item = NULL;

	if (sym < NonTerminalsBase)
	{
		// Passed symbol is terminal symbol.
		assert(sym < m_grammar.NumTerminals());
		TTerminalSymbol &sym_info = m_grammar.symbols[sym];
		lex_item = new TLangViewLexemaTreeNode(owner, 0, sym, sym_info.lex_type, sym_info.check_subt, sym_info.lex_subt.subtype);
	}
	else
	{
		// Passed symbol is non terminal.
		assert(sym < NonTerminalsBase+m_grammar.NumNonTerminals());
		lex_item = new TLangViewNonTerminalTreeNode(owner, 0, sym);
	}

	if (lex_item == NULL)
	{
		wchar_t buff40[40];
		m_dialog.MsgBoxFmt(MB_OK, L"Out of memory while creating LexItem for %s(%hu).", m_grammar.GetSymbolName(sym, buff40), sym);
		return(NULL);
	}

	// Return the non NULL object.
	lex_item->SetLexItemClickOwner();
	return(lex_item);
}

void TLangViewGrammarTreesHelper::HighlightUpperLink(TLangViewTreeNode *node)
{
	assert(node->GetOwner() != NULL);
	node->SetSelectedStateEx2(TRUE, TRUE, -1, -1);
}

void TLangViewGrammarTreesHelper::HighlightRightLink(TLangViewTreeNode *node, bool thick_link)
{
	assert(node->GetNextSibling(TRUE) != NULL);
	node->SetSelectedStateEx2(-1, -1, TRUE, thick_link);
}

void TLangViewGrammarTreesHelper::HighlightDescendingPath(TLangViewTreeNode *non_term_defn_root, TLangViewTreeNode *conflicting_symbol)
{
	// This looks like a dangerous infinite loop.
	TLangViewTreeNode *node = conflicting_symbol;
	while (node != non_term_defn_root)
	{
		// Loop on highlighting links on the current layer.
		for(;;)
		{
			TLangViewTreeNode *prev_node = (TLangViewTreeNode*)(node->GetPrevSibling(TRUE));
			if (prev_node == NULL || prev_node->GetItemType() == litp_special_lex_item)
				break;

			HighlightRightLink(prev_node);
			node = prev_node;
		}

		// Highlight the upper link.
		TLangViewTreeNode *upper_node = node->GetOwner();
		assert(upper_node != NULL);
		HighlightUpperLink(node);
		node = upper_node;
	}
}

TSpecialLexItemType TLangViewGrammarTreesHelper::GetCategoryTypeFromCategoryCode(int cat_code)
{
	switch (cat_code)
	{
		case -3:	  return(slit_ambig_grm_conflicts);
		case -2:	  return(slit_assigned_grm_conflicts);
		case -1:	  return(slit_unexp_grm_conflicts);
	}

	// Map all other category codes to the most generic spec item category type.
	return(slit_all_grammar_conflicts);
}

bool TLangViewGrammarTreesHelper::IsSymbolPresentAmongChildren(TLangViewTreeNode *node, WORD symbol)
{
	TLangViewTreeNode *chld = (TLangViewTreeNode*)(node->GetFirstChild(TRUE));
	while (chld != NULL)
	{
		if (chld->GetSymbolValue() == symbol)
			return(TRUE);

		// Shift the loop variable.
		chld = (TLangViewTreeNode*)(chld->GetNextSibling(TRUE));
	}

	// The symbol is not present.
	return(FALSE);
}

void TLangViewGrammarTreesHelper::CheckAddToTheToDoList(TNonTerminalTreeNodePtrsArray &list_to_do, TLangViewNonTerminalTreeNode *candidate_node)
{
	for (int inx=0; inx<list_to_do.NumItems(); ++inx)
	{
		if (list_to_do[inx]->GetSymbolValue() == candidate_node->GetSymbolValue())
		{
			// This symbol is already present in the list.
			return;
		}
	}

	// This symbol should be processed.
	if (list_to_do.AppendItem(candidate_node) == FALSE)
	{
		m_dialog.MsgBox(L"Out of memory while adding symbol into the to do list.");
		return;
	}
}

// -------------------------------------------------------------------------------
//	===============  TLangViewMiscGrammarAnalysisHelper  =================
// -------------------------------------------------------------------------------

void TLangViewMiscGrammarAnalysisHelper::FindEmptyAndTransparentNonTerminals(TDestinationFile &rprt)
{
	// Empty non terminals have empty defining rules. Check if such non terminals are present of not.
	bool empty_rules_present = FALSE;
	for (TRulesIterator iter1(m_grammar); iter1; ++iter1)
	{
		if (iter1.CurrRule().Length() == 0)
		{
			empty_rules_present = TRUE;
			break;
		}
	}

	if (empty_rules_present == FALSE)
	{
		rprt.WriteLine(L"Transparent non terminals are missing in the currently selected grammar.");
		return;
	}

	// Generate list of non terminals that have empty defining rules.
	rprt.WriteLine(L"    These non terminals have empty rules.");
	rprt.WriteLine(L"----------------------------------------------------------------------------");

	int empty_non_terms = 0;
	for (TRulesIterator iter2(m_grammar); iter2; ++iter2)
	{
		TGrammarRule &rule = iter2.CurrRule();
		if (rule.Length() == 0)
		{
			rprt.WriteFmtLine(L"  %hd: %s  (R%d)", rule.non_term, m_grammar.non_terminals[rule.non_term-NonTerminalsBase].symbol_name, iter2.CurrRuleIndex());
			empty_non_terms++;
		}
	}

	rprt.WriteLine(L"-----------------------------------------------------------------");
	rprt.WriteFmtLine(L"    Num empty non terminals: %d.", empty_non_terms);

	// Generate list of transparent non terminals that allow construction out of nothing.
	rprt.WriteLine();
	rprt.WriteLine(L"    These non terminals can be constructed from empty symbol.");
	rprt.WriteLine(L"----------------------------------------------------------------------------");

	int transp_non_terms = 0;
	for (int isym=0; isym<m_grammar.NumNonTerminals(); ++isym)
	{
		if (m_grammar.non_terminals[isym].solid_symbol == FALSE)
		{
			rprt.WriteFmtLine(L"  %hd: %s", NonTerminalsBase+isym, m_grammar.non_terminals[isym].symbol_name);
			transp_non_terms++;
		}
	}

	rprt.WriteLine(L"-----------------------------------------------------------------");
	rprt.WriteFmtLine(L"    Num transparent non terminals: %d.", transp_non_terms);
}

void TLangViewMiscGrammarAnalysisHelper::FindPrimitiveNonTerminals(TDestinationFile &rprt)
{
	// Primitive non terminals allow construction directly from terminal symbols.
	rprt.WriteLine(L"    These non terminals can be constructed directly from terminal symbols.");
	rprt.WriteLine(L"--------------------------------------------------------------------------------------------------");

	int primitive_non_terms = 0;
	int primitive_non_term_rules = 0;
	WORD prev_non_term_rules_sect = 0xFFFF;

	rprt.WriteLine();
	for (int isym=0; isym<m_grammar.NumNonTerminals(); ++isym)
	{
		bool header_emitted = FALSE;
		WORD sym = NonTerminalsBase+isym;

		for (TRulesIterator iter(m_grammar); iter; ++iter)
		{
			TGrammarRule &rule = iter.CurrRule();
			if (rule.non_term != sym)
				continue;

			// Look for non terminals in the right hand side.
			bool primitive_rule = TRUE;
			for (int ich=0; ich<rule.Length(); ++ich)
			{
				if (rule.symbols[ich] >= NonTerminalsBase)
				{
					primitive_rule = FALSE;
					break;
				}
			}

			if (primitive_rule == TRUE)
			{
				WORD rules_sect = m_grammar.non_terminals[isym].rules_sect;

				if (header_emitted == FALSE)
				{
					if (rules_sect != prev_non_term_rules_sect)
						rprt.WriteLine(L"----------------------------------------------");

					rprt.WriteFmt(L"  %hd: %s.  \t", sym, m_grammar.non_terminals[isym].symbol_name);
					primitive_non_terms++;
				}

				rprt.WriteFmt(L"%sR%d", ((header_emitted == TRUE) ? L", " : L""), iter.CurrRuleIndex());

				primitive_non_term_rules++;
				prev_non_term_rules_sect = rules_sect;
				header_emitted = TRUE;
			}
		}

		if (header_emitted == TRUE)
			rprt.WriteLine(L".");
	}

	if (primitive_non_terms == 0)
	{
		rprt.WriteLine(L"  Primitive non terminals are missing.");
	}
	else
	{
		rprt.WriteLine(L"-----------------------------------------------------------------");
		rprt.WriteFmtLine(L"    Num primitive non terminals: %d,   Num primitive rules: %d.", primitive_non_terms, primitive_non_term_rules);
	}
}

void TLangViewMiscGrammarAnalysisHelper::CreateNonTermsUseStatistics(TDestinationFile &rprt)
{
	rprt.WriteLine(L"    Statistics on the intensity of the non terminals use.");
	rprt.WriteLine(L"---------------------------------------------------------------------------------------------------");

	TNonTermSuppInfosArray non_terms_info;
	if (PrepareUseInOtherNonTermRulesDependencies(non_terms_info) == FALSE)
		return;

	// Weight of the symbol is the number of other symbols that it depend on.
	for (int isym1=0; isym1<m_grammar.NumNonTerminals(); ++isym1)
	{
		non_terms_info[isym1].m_symbol_weight = non_terms_info[isym1].m_dependent_symbols.NumItems();
	}

	// Sort symbols by the dependency value.
	non_terms_info.QuickSort(TNonTermSuppInfo::CompareByWeight);

	// Emit the results.
	int prev_num_uses = 0;
	for (int isym2=0; isym2<non_terms_info.NumItems(); ++isym2)
	{
		WORD sym = non_terms_info[isym2].m_sym;
		int num_uses = non_terms_info[isym2].m_dependent_symbols.NumItems();
		if (num_uses != prev_num_uses && prev_num_uses != 0)
			rprt.WriteLine(L"-----------------------------------------------------------------------");

		rprt.WriteFmtLine(L"  %3d.  %hd: %s  \t(NumUses: %d)", isym2+1, sym, m_grammar.non_terminals[sym-NonTerminalsBase].symbol_name, num_uses);
		prev_num_uses = num_uses;
	}
}

void TLangViewMiscGrammarAnalysisHelper::CreateNonTermDefsDistanceStatistics(TDestinationFile &rprt)
{
	rprt.WriteLine(L"    Information on the distance between definitions of non terminals and their use.");
	rprt.WriteLine(L"------------------------------------------------------------------------------------------------");

	TNonTermSuppInfosArray non_terms_info;
	if (PrepareUseInOtherNonTermRulesDependencies(non_terms_info) == FALSE)
		return;

	// Weight of the symbol is the average distance between the definition of the non term and its use.
	for (int isym1=0; isym1<m_grammar.NumNonTerminals(); ++isym1)
	{
		WORD sym = non_terms_info[isym1].m_sym;
		double sym_weight = 0;

		TSymbolsArray &deps = non_terms_info[isym1].m_dependent_symbols;
		if (deps.NumItems() > 0)
		{
			for (int idep=0; idep<deps.NumItems(); ++idep)
			{
				int dist  = GetNonTerminalSymbolsDistance(deps[idep], sym);
				sym_weight += pow(dist, 1.8);
			}

			sym_weight /= deps.NumItems();
			non_terms_info[isym1].m_symbol_weight = sym_weight;
		}
	}

	// Sort symbols by the distance value.
	non_terms_info.QuickSort(TNonTermSuppInfo::CompareByWeight);

	// Emit the results.
	double overall_weight = 0;
	for (int isym2=0; isym2<non_terms_info.NumItems(); ++isym2)
	{
		WORD sym = non_terms_info[isym2].m_sym;
		double weight = non_terms_info[isym2].m_symbol_weight;
		rprt.WriteFmtLine(L"  %3d.  %hd: %s  \t(CallToDefDistanceMeasure: %.4f, UseIntensity: %d.)",
						isym2+1, sym, m_grammar.non_terminals[sym-NonTerminalsBase].symbol_name,
						weight, non_terms_info[isym2].m_dependent_symbols.NumItems());
		overall_weight += weight;
	}

	rprt.WriteLine(L"------------------------------------------------");
	rprt.WriteFmtLine(L"    Overall weight: %.4f", overall_weight);
}

void TLangViewMiscGrammarAnalysisHelper::CreateNonTermDefsCallsDistanceStatistics(TDestinationFile &rprt)
{
	// Select parsing state for generating possible contexts.
	int ctxs_for_stt = 942;

	rprt.WriteLine();
	rprt.WriteFmtLine(L"    Context sequencies for parsing state %d.", ctxs_for_stt);
	rprt.WriteLine(L"------------------------------------------------------------------------------------------------");

	TParsingStateContextsAnalysisHelper helper(m_dialog_frame.Console());
	bool init_succ = helper.PrepareIteration(&m_grammar, ctxs_for_stt);

	int iseq = 0;
	while (helper.GetFullTreeContext() == TRUE && iseq < 1000)
	{
		rprt.WriteFmt(L"%4d.    ", ++iseq);
		for (int inx=helper.GetCurrSeqLen()-1; inx >= 0; --inx)
		{
			wchar_t buff40[40];
			rprt.WriteFmt(L" <%hd> %s", helper.GetCurrSeqStepState(inx), m_grammar.GetSymbolName(helper.GetCurrSeqStepSymbol(inx), buff40));
		}

		rprt.WriteLine();
	}

	rprt.WriteLine();
	rprt.WriteLine();

	// --------------------------------------

	rprt.WriteLine(L"    Information on the distance between different non terminal call places.");
	rprt.WriteLine(L"------------------------------------------------------------------------------------------------");

	TNonTermSuppInfosArray non_terms_info;
	if (PrepareUseInOtherNonTermRulesDependencies(non_terms_info) == FALSE)
		return;

	for (int isym1=0; isym1<m_grammar.NumNonTerminals(); ++isym1)
	{
		double sym_weight = 0;
		int num_pairs = 0;

		TSymbolsArray &deps = non_terms_info[isym1].m_dependent_symbols;
		for (int idep1=0; idep1<deps.NumItems(); ++idep1)
		{
			for (int idep2=idep1+1; idep2<deps.NumItems(); ++idep2)
			{
				int dist  = GetNonTerminalSymbolsDistance(deps[idep1], deps[idep2]);
				sym_weight += pow(dist, 1.8);
				num_pairs++;
			}

		}

		if (num_pairs > 0)
		{
			sym_weight /= deps.NumItems();
			non_terms_info[isym1].m_symbol_weight = sym_weight;
		}
	}

	// Sort symbols by the distance value.
	non_terms_info.QuickSort(TNonTermSuppInfo::CompareByWeight);

	// Emit the results.
	double overall_weight = 0;
	for (int isym2=0; isym2<non_terms_info.NumItems(); ++isym2)
	{
		WORD sym = non_terms_info[isym2].m_sym;
		double weight = non_terms_info[isym2].m_symbol_weight;
		if (weight <= 0)
			break;

		rprt.WriteFmtLine(L"  %3d.  %hd: %s  \t(BetweenCallsDistanceMeasure: %.4f)", isym2+1, sym, m_grammar.non_terminals[sym-NonTerminalsBase].symbol_name, weight);
		overall_weight += weight;
	}

	rprt.WriteLine(L"------------------------------------------------");
	rprt.WriteFmtLine(L"    Overall weight: %.4f", overall_weight);
}

void TLangViewMiscGrammarAnalysisHelper::CreatePathLengthsStatistics(TDestinationFile &rprt)
{
	if (m_grammar.NumConflicts() == 0)
	{
		rprt.WriteFmtLine(L"Current grammar (igrm=%d) has no conflicts.", m_grammar.GetGrammarIndex());
		return;
	}

	// Allocate storage for gathering statistics.
	int cnt_paths_by_length[TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN];
	int min_length = TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN, max_length = 0;
	memset(cnt_paths_by_length, 0, sizeof(cnt_paths_by_length));

	int sample_iconflict[TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN];
	int sample_iaction[TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN];
	int sample_ipath[TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN];

	int cnt_paths = 0;
	for (int iconflict=0; iconflict<m_grammar.NumConflicts(); ++iconflict)
	{
		// Pick up the conflict.
		TGrammarConflict &cfct = m_grammar.conflicts[iconflict];
		for (int iact=0; iact<cfct.analysis_results.NumItems(); ++iact)
		{
			// Pick up the analysis results for the action.
			TConflictingActionAnalysisResults &res = cfct.analysis_results[iact];
			for (int ipath=0; ipath<res.m_derivation_paths.NumItems(); ++ipath)
			{
				int num_steps = res.m_derivation_paths[ipath].derv_path_len;
				assert(num_steps >0 && num_steps < TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN);

				if (cnt_paths_by_length[num_steps] == 0)
				{
					sample_iconflict[num_steps] = iconflict;
					sample_iaction[num_steps] = iact;
					sample_ipath[num_steps] = ipath;
				}

				cnt_paths_by_length[num_steps]++;
				cnt_paths++;

				if (num_steps < min_length)
					min_length = num_steps;
				if (num_steps > max_length)
					max_length = num_steps;
			}
		}
	}

	if (cnt_paths == 0)
	{
		rprt.WriteFmtLine(L"Current grammar (igrm=%d) has no derivation paths.", m_grammar.GetGrammarIndex());
		return;
	}

	rprt.WriteFmtLine(L"Total number of derivation paths: \t%d.\r\n", cnt_paths);
	for (int len=min_length; len <= max_length; ++len)
	{
		if (cnt_paths_by_length[len] != 0)
			rprt.WriteFmtLine(L"Len: %3d, Number of paths:  %4d  (C%d.A%d.P%d).", len, cnt_paths_by_length[len], sample_iconflict[len], sample_iaction[len], sample_ipath[len]);
	}
}

void TLangViewMiscGrammarAnalysisHelper::CreateStarterSymbolDiffsReport(TDestinationFile &rprt)
{
	if (m_grammar.NumTerminals() == 0)
	{
		rprt.WriteFmtLine(L"Current grammar (igrm=%d) has no terminal symbols.", m_grammar.GetGrammarIndex());
		return;
	}
	else if (m_grammar.NumNonTerminals() == 0)
	{
		rprt.WriteFmtLine(L"Current grammar (igrm=%d) has no non terminals.", m_grammar.GetGrammarIndex());
		return;
	}

	TAnalysisTable first_and_follows, non_term_non_term_starters;
	if (m_grammar.GenerateFirstsAndFollows(first_and_follows) == FALSE || m_grammar.GenerateNonTerminalFirsts(non_term_non_term_starters, first_and_follows) == FALSE)
	{
		rprt.WriteFmtLine(L"Error generating FIRSTs and FOLLOWs sets.");
		return;
	}

	WORD sym1 = 4014;		// SimpleTypeSpecifier.
	WORD sym2 = 4107;		// ConstantExpression.
	// WORD sym1 = 4116;	// ShiftExpression.
	CreateStartersReportForSymbolsPair(rprt, sym1, sym2, first_and_follows, non_term_non_term_starters);
}

void TLangViewMiscGrammarAnalysisHelper::CreateStartersReportForSymbolsPair(TDestinationFile &rprt, WORD sym1, WORD sym2, TAnalysisTable &first_and_follows, TAnalysisTable &non_term_non_term_starters)
{
	assert(sym1 >= NonTerminalsBase && sym1 < NonTerminalsBase+m_grammar.NumNonTerminals());
	assert(sym2 >= NonTerminalsBase && sym2 < NonTerminalsBase+m_grammar.NumNonTerminals());

	wchar_t buff_40_chars1[40], buff_40_chars2[40];

	// Write Header1.
	rprt.WriteFmtLine(L"  Common starters for %s (sym=%d) and %s (sym=%d).", m_grammar.GetSymbolName(sym1, buff_40_chars1), sym1, m_grammar.GetSymbolName(sym2, buff_40_chars2), sym2);
	WriteDelimiter(rprt);

	// Write Header2.
	rprt.WriteFmtLine(L"  Unique starters for %s (sym=%d) and not for %s (sym=%d).", m_grammar.GetSymbolName(sym1, buff_40_chars1), sym1, m_grammar.GetSymbolName(sym2, buff_40_chars2), sym2);
	WriteDelimiter(rprt);

	// Write Header3.
	rprt.WriteFmtLine(L"  Unique starters for %s (sym=%d) and not for %s (sym=%d).", m_grammar.GetSymbolName(sym2, buff_40_chars2), sym2, m_grammar.GetSymbolName(sym1, buff_40_chars1), sym1);
	WriteDelimiter(rprt);

	/* -BUGBUG-
	// Iterate available followers.
	int cnt = 1;
	WORD *sym_set = followers_table.GetNonTermRowPtr(non_term);
	for (int offs=0; offs<num_terminals; ++offs)
	{
		if ((sym_set[offs] & FUN_FOLLOWS_MASK) != 0)
		{
			// Offs is member of sym_set.
			rprt.WriteFmtLine(L"   %3d. %s (sym=%d).", cnt++, grm.GetSymbolName(offs, buff_40_chars), offs);
		}
	}
	*/

	// Add empty line between lists.
	rprt.WriteLine();
}

void TLangViewMiscGrammarAnalysisHelper::DumpMasksRelation(TDestinationFile &rprt, WORD sym1, WORD sym2, TAnalysisTable &first_and_follows, TAnalysisTable &non_term_non_term_starters, bool (*pfn)(WORD mask_elem1, WORD mask_elem2))
{
	int cnt = 1;

	// Iterate terminal symbols.
	WORD *faf_set1 = first_and_follows.GetNonTermRowPtr(sym1);
	WORD *faf_set2 = first_and_follows.GetNonTermRowPtr(sym2);
	for (int offs=0; offs<m_grammar.NumTerminals(); ++offs)
	{
		if ((*pfn)(faf_set1[offs], faf_set2[offs]) == TRUE)
		{
			wchar_t buff_40_chars[40];
			rprt.WriteFmtLine(L"   %3d. %s (sym=%d).", cnt++, m_grammar.GetSymbolName(offs, buff_40_chars), offs);
		}
	}

	// Add empty line between lists.
	rprt.WriteLine();
}

void TLangViewMiscGrammarAnalysisHelper::ClassifyGrammarConflicts(TDestinationFile &rprt, bool unexpected_conflicts_only)
{
	if (m_grammar.NumConflicts() == 0)
	{
		rprt.WriteFmtLine(L"Current grammar (igrm=%d) has no conflicts.", m_grammar.GetGrammarIndex());
		return;
	}

	// Generate the conflicts classification.
	TGrammarConflictSignaturesArray signatures_list;

	// Iterate conflicts and build the list of different signatures.
	for (int iconflict=0; iconflict<m_grammar.NumConflicts(); ++iconflict)
	{
		// Pick up the conflict.
		TGrammarConflict &cfct = m_grammar.conflicts[iconflict];
		if (unexpected_conflicts_only == TRUE && cfct.expected_conflict_inx != -1)
		{
			// Ignore this conflict. It is either expected or ambiguous.
			continue;
		}

		// It is necessary to collect all directly involved rule positions.
		TRulePositionsArray directly_conflicting_positions;
		if (CollectDirectlyInvolvedPositions(directly_conflicting_positions, &m_grammar, iconflict) == FALSE)
		{
			rprt.WriteLine(L"Error generating grammar conflicts classification (1).");
			return;
		}

		// Find out if current conflict has new signature or this is something already known.
		int existing_sig_index = -1;
		for (int isig=0; isig < signatures_list.NumItems() && existing_sig_index < 0; ++isig)
		{
			if (directly_conflicting_positions.NumItems() != signatures_list[isig].m_directly_involved_positions.NumItems())
				continue;

			existing_sig_index = isig;
			for (int ipos=0; ipos < directly_conflicting_positions.NumItems(); ++ipos)
			{
				if (TRulePosition::CompareLight(directly_conflicting_positions.ItemPtr(ipos), signatures_list[isig].m_directly_involved_positions.ItemPtr(ipos)) != 0)
				{
					existing_sig_index = -1;
					break;
				}
			}
		}

		// Allocate copy of the conflict index as WORD because it should be passed as reference.
		WORD icon = iconflict;
		if (existing_sig_index < 0)
		{
			// This is new signature.
			TGrammarConflictsSignature new_sig = { FALSE };
			if (signatures_list.AppendItem(new_sig) == FALSE)
			{
				rprt.WriteLine(L"Error generating grammar conflicts classification (2).");
				return;
			}

			// Fill in the signature. These actions are not allocating resources.
			signatures_list[signatures_list.NumItems()-1].m_directly_involved_positions.TakeContentsFrom(directly_conflicting_positions);
			signatures_list[signatures_list.NumItems()-1].m_conflict_indexes.AppendItem(icon);
		}
		else
		{
			// This signature is knowm. Add index of the current conflict to the list of conflicts.
			if (signatures_list[existing_sig_index].m_conflict_indexes.AppendItem(icon) == FALSE)
			{
				rprt.WriteLine(L"Error generating grammar conflicts classification (3).");
				return;
			}
		}
	}

	if (signatures_list.NumItems() == 0)
	{
		if (unexpected_conflicts_only == TRUE)
			rprt.WriteFmtLine(L"Current grammar (igrm=%d) has no unexpected conflicts.", m_grammar.GetGrammarIndex());
		else rprt.WriteFmtLine(L"List of grammar conflists (igrm=%d) is empty for unclear reason.", m_grammar.GetGrammarIndex());
		return;
	}

	// Sort array of conflict indexes to group them by the conflicting symbol.
	for (int isig0=0; isig0<signatures_list.NumItems(); ++isig0)
		signatures_list[isig0].m_conflict_indexes.QuickSortWithCtx(this, CompareConflictIndexes);

	// Write the report header.
	rprt.WriteLine();
	rprt.WriteFmtLine(L"   Grammar conflicts classification%s.", (unexpected_conflicts_only == TRUE) ? L" (UNEXPECTED CONFLICTS ONLY)" : L"");
	rprt.WriteLine(L"--------------------------------------------------------------------------------------------------------------------------");

	int sigs_cnt = 0;
	wchar_t buff_40ch[40];

	// First loop dumps signatures that match the confused promotion pattern. This means that all
	// directly conflicting rules have the same right hand side and conflict is reduce-reduce.
	int confused_promo_patterns_cnt = 0;
	for (int isig1=0; isig1<signatures_list.NumItems(); ++isig1)
	{
		if (signatures_list[isig1].m_signature_dumped == TRUE)
			continue;

		TRulePositionsArray &positions_array = signatures_list[isig1].m_directly_involved_positions;
		assert(positions_array.NumItems() > 0);

		TGrammarRule &first_pos_rule = m_grammar.rules[positions_array[0].irule];
		if (positions_array[0].isym != first_pos_rule.Length())
			continue;
		else if (IsSameRuleBodyPositions(positions_array, first_pos_rule.symbols, first_pos_rule.Length()) == FALSE)
			continue;

		int num_conflicts_in_pattern;
		TSymbolsArray participating_rules_list;
		if (CollectRulesForSameRightSidePattern(signatures_list, participating_rules_list, num_conflicts_in_pattern, first_pos_rule.symbols, first_pos_rule.Length()) == FALSE)
		{
			rprt.WriteLine(L"Error generating grammar conflicts classification (4).");
			return;
		}

		// Write contents of the pattern as the pattern header.
		rprt.WriteLine();
		rprt.WriteLine(L"=====================================================================");
		rprt.WriteFmt(L"====   %d: Ambiguous ", ++confused_promo_patterns_cnt);
		for (int isym=0; isym<first_pos_rule.symbols.NumItems(); ++isym)
		{
			rprt.WriteFmt(L" %s(%hd)", m_grammar.GetSymbolName(first_pos_rule.symbols[isym], buff_40ch), first_pos_rule.symbols[isym]);
		}

		rprt.WriteFmtLine(L" \t NumConflicts: %d.", num_conflicts_in_pattern);
		rprt.WriteLine(L"==== ----------------------------------------------------------------");

		// Write all different non terminals that this pattern might be reduced to.
		for (int rule_index_index=0; rule_index_index<participating_rules_list.NumItems(); ++rule_index_index)
		{
			WORD irule = participating_rules_list[rule_index_index];
			WORD rule_non_term = m_grammar.rules[irule].non_term;
			rprt.WriteFmtLine(L"====          R%hd: \t%s(%hd)", irule, m_grammar.GetSymbolName(rule_non_term, buff_40ch), rule_non_term);
		}

		rprt.WriteLine(L"=====================================================================");

		// Current signature belongs to some new confused promotion pattern.
		for (int isig2=isig1; isig2<signatures_list.NumItems(); ++isig2)
		{
			if (signatures_list[isig2].m_signature_dumped == TRUE)
				continue;

			if (IsSameRuleBodyPositions(signatures_list[isig2].m_directly_involved_positions, first_pos_rule.symbols, first_pos_rule.Length()) == FALSE)
				continue;

			DumpConflictsSignature(rprt, ++sigs_cnt, signatures_list[isig2]);
		}
	}

	// Second loop dumps signatures with ony one possible action symbol.
	int single_inp_sym_patterns_cnt = 0;
	for (int isig3=0; isig3<signatures_list.NumItems(); ++isig3)
	{
		if (signatures_list[isig3].m_signature_dumped == TRUE)
			continue;

		TSymbolsArray &conflict_indexes = signatures_list[isig3].m_conflict_indexes;
		assert(conflict_indexes.NumItems() > 0);

		WORD input_sym = m_grammar.conflicts[conflict_indexes[0]].input_symbol;
		if (IsSameInputSymbolConflicts(conflict_indexes, input_sym) == FALSE)
			continue;

		int num_conflicts_in_pattern;
		TSymbolsArray participating_rules_list;
		if (CollectRulesForSameSingleSymPattern(signatures_list, participating_rules_list, num_conflicts_in_pattern, input_sym) == FALSE)
		{
			rprt.WriteLine(L"Error generating grammar conflicts classification (5).");
			return;
		}

		// Write the signatures group header.
		rprt.WriteLine();
		rprt.WriteLine(L"====================================================================");
		rprt.WriteFmtLine(L"====   %d: Single inp sym  %s(%hd),  NumConflicts: %d.", ++single_inp_sym_patterns_cnt, m_grammar.GetSymbolName(input_sym, buff_40ch), input_sym, num_conflicts_in_pattern);
		rprt.WriteLine(L"==== ---------------------------------------------------------------");

		// List all rules that belong to this group of signatures.
		rprt.Write(L"====          ");
		for (int rule_index_index=0; rule_index_index<participating_rules_list.NumItems(); ++rule_index_index)
			rprt.WriteFmt(L"R%hd%s", participating_rules_list[rule_index_index], (rule_index_index != participating_rules_list.NumItems()-1) ? L", " : L".");
		rprt.WriteLine();

		rprt.WriteLine(L"====================================================================");

		for (int isig4=isig3; isig4<signatures_list.NumItems(); ++isig4)
		{
			if (signatures_list[isig4].m_signature_dumped == TRUE)
				continue;

			if (IsSameInputSymbolConflicts(signatures_list[isig4].m_conflict_indexes, input_sym) == FALSE)
				continue;

			DumpConflictsSignature(rprt, ++sigs_cnt, signatures_list[isig4]);
		}
	}

	// Final loop on dumps signatures that do not match the patterns above.
	bool other_patterns_present = FALSE, smth_undumped = TRUE;
	int min_num_inp_symbols = 2;

	while (smth_undumped == TRUE)
	{
		smth_undumped = FALSE;
		for (int isig5=0; isig5<signatures_list.NumItems(); ++isig5)
		{
			if (signatures_list[isig5].m_signature_dumped == TRUE)
				continue;

			smth_undumped = TRUE;
			if (NumDifferentInputSymbols(signatures_list[isig5].m_conflict_indexes) <= min_num_inp_symbols)
			{
				if (other_patterns_present == FALSE)
				{
					rprt.WriteLine();
					rprt.WriteLine(L"==============================================================");
					rprt.WriteLine(L"====   Other conflict signatures.");
					rprt.WriteLine(L"==============================================================");
					other_patterns_present = TRUE;
				}

				DumpConflictsSignature(rprt, ++sigs_cnt, signatures_list[isig5]);
			}
		}

		min_num_inp_symbols++;
	}
}

bool TLangViewMiscGrammarAnalysisHelper::CollectDirectlyInvolvedPositions(TRulePositionsArray &directly_conflicting_positions, TGrammar *grammar, int iconflict)
{
	assert(grammar != NULL && iconflict >= 0 && iconflict <grammar->NumConflicts());
	TGrammarConflict &cfct = grammar->conflicts[iconflict];
	directly_conflicting_positions.Clear();

	// Iterate conflicting actions of the current conflict.
	for (int iact=0; iact<cfct.NumActions(); ++iact)
	{
		WORD act = cfct.conflicting_actions[iact];
		if (act < actb_reduce)
		{
			// This is shift action. It is necessary to look for rules.
			assert(act-actb_shift < grammar->NumParsingStates());

			TParsingStatePositionsIterator iter;
			WORD prev_irule = 0xFFFF, prev_isym = 0xFFFF;
			for (iter.Setup(grammar->parsing_states[cfct.parser_state]); iter; ++iter)
			{
				TRulePosition &pos = iter.CurrPos();
				if (pos.irule == prev_irule && pos.isym == prev_isym)
					continue;

				prev_irule = pos.irule;
				prev_isym = pos.isym;

				TGrammarRule &rule = grammar->rules[pos.irule];
				if (pos.isym >= rule.Length() || rule.symbols[pos.isym] != cfct.input_symbol)
					continue;

				// This is new position and it is not at the end of the rule.
				if (directly_conflicting_positions.AppendItem(pos) == FALSE)
					return(FALSE);
			}
		}
		else
		{
			// This is reduce action. Only one rule is involved.
			assert(act-actb_reduce < grammar->NumRules());

			TRulePosition reduce_pos = { act-actb_reduce, grammar->rules[act-actb_reduce].Length() };
			if (directly_conflicting_positions.AppendItem(reduce_pos) == FALSE)
				return(FALSE);
		}
	}

	// Sort array of collected positions.
	assert(directly_conflicting_positions.NumItems() > 0);
	directly_conflicting_positions.QuickSort(TRulePosition::CompareLight);
	return(TRUE);
}

int TLangViewMiscGrammarAnalysisHelper::GetNonTerminalSymbolsDistance(WORD sym1, WORD sym2)
{
	int dist1 = ___abs(((int)sym1)-((int)sym2));
	int dist2 = ___abs(((int)sym1+m_grammar.NumNonTerminals())-((int)sym2));
	return(___min(dist1, dist2));
}

bool TLangViewMiscGrammarAnalysisHelper::PrepareUseInOtherNonTermRulesDependencies(TNonTermSuppInfosArray &non_terms_info)
{
	// Create initial list with empty dependencies.
	non_terms_info.Clear();
	for (int inx=0; inx<m_grammar.NumNonTerminals(); ++inx)
	{
		TNonTermSuppInfo info = { NonTerminalsBase+inx };
		if (non_terms_info.AppendItem(info) == FALSE)
		{
			m_dialog_frame.MsgBox(L"Error creating list of supp info structures (1).");
			return(FALSE);
		}
	}

	for (int isym=0; isym<m_grammar.NumNonTerminals(); ++isym)
	{
		WORD sym = NonTerminalsBase+isym;

		// Look for rules that contain current non terminal in their left hand sides.
		bool dependency_present = FALSE;
		WORD prev_rule_non_term = 0xFFFF;
		for (TRulesIterator iter(m_grammar); iter; ++iter)
		{
			TGrammarRule &rule = iter.CurrRule();
			if (rule.non_term == sym)
				continue;

			if (rule.non_term != prev_rule_non_term)
				dependency_present = FALSE;

			if (dependency_present == FALSE)
			{
				for (int ich=0; ich<rule.Length(); ++ich)
				{
					if (rule.symbols[ich] == sym)
					{
						// Rule of the current inner loop non term is using the non terminal of the outer loop.
						// Add inner non terminal to the list of dependencies of the outer non terminal.
						if (non_terms_info[isym].m_dependent_symbols.AppendItem(rule.non_term) == FALSE)
						{
							m_dialog_frame.MsgBox(L"Error creating list of supp info structures (2).");
							return(FALSE);
						}

						dependency_present = TRUE;
						break;
					}
				}
			}

			prev_rule_non_term = rule.non_term;
		}
	}

	// Success.
	return(TRUE);
}

bool TLangViewMiscGrammarAnalysisHelper::CollectRulesForSameRightSidePattern(TGrammarConflictSignaturesArray &sigs, TSymbolsArray &rules_list, int &num_conflicts_in_pattern, TSymbolsArray &rule_body, WORD rule_isym)
{
	num_conflicts_in_pattern = 0;

	for (int isig=0; isig<sigs.NumItems(); ++isig)
	{
		TRulePositionsArray &positions = sigs[isig].m_directly_involved_positions;
		if (sigs[isig].m_signature_dumped == TRUE || IsSameRuleBodyPositions(positions, rule_body, rule_isym) == FALSE)
			continue;

		// Current signature belongs to the requested pattern.
		num_conflicts_in_pattern += sigs[isig].m_conflict_indexes.NumItems();

		// Iterate positions of this signature.
		for (int ipos=0; ipos<positions.NumItems(); ++ipos)
		{
			WORD irule = positions[ipos].irule;
			if (rules_list.IsSymInBuffer(irule) == FALSE && rules_list.AppendItem(irule) == FALSE)
				return(FALSE);
		}
	}

	// List of all different participating rules was generated.
	rules_list.QuickSort(TSymbolsArray::CompareSymbols);
	return(TRUE);
}

bool TLangViewMiscGrammarAnalysisHelper::CollectRulesForSameSingleSymPattern(TGrammarConflictSignaturesArray &sigs, TSymbolsArray &rules_list, int &num_conflicts_in_pattern, WORD input_sym)
{
	num_conflicts_in_pattern = 0;

	for (int isig=0; isig<sigs.NumItems(); ++isig)
	{
		if (sigs[isig].m_signature_dumped == TRUE || IsSameInputSymbolConflicts(sigs[isig].m_conflict_indexes, input_sym) == FALSE)
			continue;

		// Current signature belongs to the requested pattern.
		num_conflicts_in_pattern += sigs[isig].m_conflict_indexes.NumItems();

		// Iterate positions of this signature.
		TRulePositionsArray &positions = sigs[isig].m_directly_involved_positions;
		for (int ipos=0; ipos<positions.NumItems(); ++ipos)
		{
			WORD irule = positions[ipos].irule;
			if (rules_list.IsSymInBuffer(irule) == FALSE && rules_list.AppendItem(irule) == FALSE)
				return(FALSE);
		}
	}

	// List of all different participating rules was generated.
	rules_list.QuickSort(TSymbolsArray::CompareSymbols);
	return(TRUE);
}

bool TLangViewMiscGrammarAnalysisHelper::IsSameRuleBodyPositions(TRulePositionsArray &array, TSymbolsArray &rule_body, WORD rule_isym)
{
	// Rreturn TRUE if all positions in the passed array are end of the rule positions and rule body
	// in all these positions is the same to the passed rule body.
	for (int ipos=0; ipos<array.NumItems(); ++ipos)
	{
		TGrammarRule &rule = m_grammar.rules[array[ipos].irule];
		if (array[ipos].isym != rule_isym || rule.symbols.IsEqualTo(rule_body) == FALSE)
			return(FALSE);
	}

	// All positions in the passed array of positions match the criteria.
	return(TRUE);
}

bool TLangViewMiscGrammarAnalysisHelper::IsSameInputSymbolConflicts(TSymbolsArray &conflict_indexes, WORD input_sym)
{
	for (int icon=0; icon<conflict_indexes.NumItems(); ++icon)
	{
		if (m_grammar.conflicts[conflict_indexes[icon]].input_symbol != input_sym)
			return(FALSE);
	}

	// All conflicts have the same input symbol.
	return(TRUE);
}

int TLangViewMiscGrammarAnalysisHelper::NumDifferentInputSymbols(TSymbolsArray &conflict_indexes)
{
	int cnt_different_symbols = 0;
	WORD prev_input_sym = 0xFFFF;
	for (int icon=0; icon<conflict_indexes.NumItems(); ++icon)
	{
		WORD input_sym = m_grammar.conflicts[conflict_indexes[icon]].input_symbol;
		if (input_sym != prev_input_sym)
		{
			cnt_different_symbols++;
			prev_input_sym = input_sym;
		}
	}

	return(cnt_different_symbols);
}

void TLangViewMiscGrammarAnalysisHelper::DumpConflictsSignature(TDestinationFile &rprt, int isig, TGrammarConflictsSignature &sig)
{
	// First write the signature index.
	rprt.WriteLine();
	rprt.WriteFmt(L"    %3d: ", isig);

	// Dump array of rules that constitute the signature.
	for (int ipos=0; ipos < sig.m_directly_involved_positions.NumItems(); ++ipos)
	{
		TRulePosition &pos = sig.m_directly_involved_positions[ipos];
		rprt.WriteFmt(L"%sR%hu", ((ipos != 0) ? L", " : L""), pos.irule);
	}

	// Finalize the conflicts signature header.
	rprt.WriteFmtLine(L". \t NumConflicts: %d.", sig.m_conflict_indexes.NumItems());
	rprt.Write(L"   -----------------------------------------------------");

	// Dump conflict ids that conforms to the current signature.
	WORD prev_input_symbol = 0xFFFF;
	for (int icon=0; icon < sig.m_conflict_indexes.NumItems(); ++icon)
	{
		WORD iconflict = sig.m_conflict_indexes[icon];
		TGrammarConflict &cfct = m_grammar.conflicts[iconflict];
		if (cfct.input_symbol != prev_input_symbol)
		{
			wchar_t buff_40ch[40];
			rprt.WriteLine((prev_input_symbol == 0xFFFF) ? L"" : L".");
			rprt.WriteFmt(L"	  InputSym: %s(%hd).  \t", m_grammar.GetSymbolName(cfct.input_symbol, buff_40ch), cfct.input_symbol);
		}

		rprt.WriteFmt(L"%sC%hu", ((cfct.input_symbol != prev_input_symbol) ? L"" : L", "), iconflict);
		prev_input_symbol = cfct.input_symbol;
	}

	rprt.WriteLine(L".");

	// Stamp the signature as dumped.
	sig.m_signature_dumped = TRUE;
}


