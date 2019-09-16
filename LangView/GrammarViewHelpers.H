//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_GrammarViewHelpers_H
#define	LangView_GrammarViewHelpers_H

#ifndef   WinUi_BasicDialog_H
#include  "WinUi/BasicDialog.H"
#endif
#ifndef   ScreenItems_ScreenItemsGrid_H
#include  "ScreenItems/ScreenItemsGrid.H"
#endif
#ifndef   TextScan_GrammarStructure_H
#include  "TextScan/GrammarStructure.H"
#endif
#ifndef   LangView_LexItems_H
#include  "LangView/LexItems.H"
#endif

//
// Small text label with an optional link line.
//
class TGrammarViewLabelItem : public TTextItem60
{
public:

	TGrammarViewLabelItem(TTextStyle *stl = NULL) : TTextItem60(stl) { m_label_type = gvlt_none; m_app_value = 0xFFFF; m_dest_link_object = NULL; m_action_symbols_grid = NULL; }
	~TGrammarViewLabelItem() { if (m_action_symbols_grid != NULL) delete m_action_symbols_grid; }

	enum TLabelType
	{
		gvlt_none,
		gvlt_grammar_symbol,
		gvlt_cfct_inp_symbol,			// Conflict input symbol is similar to grammar symbol, only it is dim.
		gvlt_parsing_state,
		gvlt_xpct_conflict,
		gvlt_grammar_conflict,
		gvlt_parsing_action,				// This label can describe only shift and reduce parsing actions.
	};									// Action cannot be conflict, nesting result, etc.

	void SetupGrammarSymbolObject(TGrammar *grammar, TLangViewLexItemStyle &lex_items_style, WORD sym, bool bright_symbol);
	void SetupXpctConflictObject(TGrammar *grammar, WORD ixpct);
	void SetupObject(TLabelType gvlt, WORD app_value);

	void SetupLink(TScreenItem *dest_object, TLineStyle *link_style, long *special_src_y = NULL);

public:

	TLabelType				m_label_type;
	WORD					m_app_value;

	TLinesChainItem1			m_dest_link;					// Link object is inserted into the painting list separately and independently
															// from the object itself.
	TScreenItem				*m_dest_link_object;			// Dest link object is not owned by this object.

	TScreenItemsGridItem		*m_action_symbols_grid;
};

enum TLangViewRulesGridRowMeaning
{
	ROW_none,								// Various types of delimiers. These rows are not clickable.

	ROW_grammar_rule_body,
	ROW_parsing_state_header,

	ROW_xpct_conflict_header,
	ROW_grammar_conflict_header,

	ROW_conflicting_action_header,
	ROW_derivation_path_body,				// Derivation path as sequence of  symbols.
};

enum TLangViewRulesGridRowMeaningEx
{
	ROW_EX_none,
	ROW_EX_state_transitions,				// State transitions rows should be highlighted for post processing inside
											// the PrepareAndShowCurrentGrid() method.
};

class TLangViewGrammarRulesGrid
{
public:

	TLangViewGrammarRulesGrid() { m_use_wide_extra_pars_stt = FALSE; m_rule_index_clmn = 0; m_alignment_ref_clmn = -1; }

	~TLangViewGrammarRulesGrid()
	{
		// Reset the grid before destroying the row info buffers .
		m_grid.Reset();
	}

	void		Reset();
	void		SetExtraParsSttClmnMode(bool want_wide_clmn) { m_use_wide_extra_pars_stt = want_wide_clmn; }
	bool		CreateInitialLayout(TBasicDialog *dlg, TBasicStyle *dark_delim_style,
					TBasicStyle *non_term_clmn_bkgr_style, TBasicStyle *pars_stt_clmn_bkgr_style, TBasicStyle *alignment_clmn_bkgr_style,
					bool want_row_descr_clmn, bool want_rule_index_and_rule_non_term_clmn, bool want_wide_rule_index_clmn,
					bool want_alignment_clmn, bool want_wide_alignment_clmn, bool want_alignment_clmn_ext_spc, bool want_wide_trailing_clmn);

	bool		CreateRuleSymbolColumn(TBasicDialog *dlg, TBasicStyle *pars_stt_clmn_bkgr_style, bool right_side_column);
	bool		CreateParsSttColumnsPair(TBasicDialog *dlg, int isym);
	void		RemoveRuleSymbolColumn(TBasicDialog *dlg, int isym);

	int		CreateRow(TBasicDialog *dlg, const wchar_t *descr, TLangViewRulesGridRowMeaning row_meaning,
					int row_height, TBasicStyle *bkgr_style = NULL, TBasicStyle *delim_style = NULL,
					WORD inx2 = 0, WORD inx3 = 0, int inx4 = 0, int ins_before_row = SCRITEMS_GRID_APPEND,
					TLangViewRulesGridRowMeaningEx ex_meaning = ROW_EX_none);

	void		RemoveRowDelimiter(int irow);
	void		RemoveLastRowRowDelimiter() { RemoveRowDelimiter(m_grid.NumRows()-1); }

	void		SetupCenteredCell(TBasicDialog *dlg, const wchar_t *descr, int physical_row_inx, int physical_clmn_inx, TScreenItem *screen_item,
					short off_bottom = 0, short horz_shift = 0);
	void		SetupLeftAlignedCellWithRowBkgr(TBasicDialog *dlg, const wchar_t *descr, int physical_row_inx, int physical_clmn_inx, TScreenItem *scr_item,
					short off_bottom = 0, short horz_shift = 0);
						//
						//  Both methods grab ownership on the passed screen item.
						//

	void		UpdateGridColumnWidth(int clmn_inx, long min_width);
	void		ShiftMainLayerItem(int row_inx, int clmn_inx, short shift_x, short shift_y);

	TScreenItemsGridItem	&GetGrid() { return(m_grid); }

	//
	//  Horzontal structure of the grid.
	//

	bool		IsNonTermClmnPresent() const		{ return(m_rule_index_clmn >= 0); }

	int		RowDescrClmn() const					{ return(clmn_row_descr); }
	int		RuleIndexClmn() const					{ assert(m_rule_index_clmn >= 0); return(m_rule_index_clmn+clmn_rule_index_offs); }
	int		RuleNonTermClmn() const				{ assert(m_rule_index_clmn >= 0); return(m_rule_index_clmn+clmn_rule_non_term_offs); }
	int		RuleFrontSpaceClmn() const			{ return(m_rule_index_clmn+clmn_rule_front_space_offs); }

	int		ParsSttClmn(int isym, int offs = 0) const	{ return(m_sym_clmns_info[isym].m_pstt_clmn+offs); }
				// Symbol index should stay in the range 0 ... NumSymbolColumns().

	int		RuleSymClmn(int isym) const			{ return(m_sym_clmns_info[isym].m_symbol_clmn); }
				// Symbol index should stay in the range 0 ... NumSymbolColumns()-1.

	int		TrailingClmn() const					{ return(m_grid.NumColumns()-1); }

	int		NumParsSttClmns(int isym) const		{ return(m_sym_clmns_info[isym].NumParsingStateColumns()); }
	int		NumSymbolColumns() const			{ return(m_sym_clmns_info.NumItems()-1); }

	int		GetAlignmentSymIndex() const		{ return(m_alignment_ref_clmn); }

	int		RowDescrClmnWidth() const			{ return(row_descr_clmn_width); }
	int		RuleSymbolClmnWidth() const		{ return(rule_symbol_clmn_width); }
	int		ExtraParsSttClmnWidth() const		{ return((m_use_wide_extra_pars_stt == FALSE) ? extra_pars_stt_clmn_width : extra_pars_stt_clmn_width_10k_states); }

	//
	//	Vertical structure of the grid.
	//

	union TAppDataEx
	{
		struct
		{
			WORD		m_inx2;
			WORD		m_inx3;
			int			m_inx4;
		};

		UINT64			m_data;
	};

	TLangViewRulesGridRowMeaning		GetRowMeaning(int irow)		{ return((TLangViewRulesGridRowMeaning)LOWORD(m_grid.GetRowAppData(irow))); }
	TLangViewRulesGridRowMeaningEx	GetRowMeaningEx(int irow)	{ return((TLangViewRulesGridRowMeaningEx)HIWORD(m_grid.GetRowAppData(irow))); }

	WORD							GetIndex2(int irow)				{ TAppDataEx dex; dex.m_data = m_grid.GetRowAppDataEx(irow); return(dex.m_inx2); }
	WORD							GetIndex3(int irow)				{ TAppDataEx dex; dex.m_data = m_grid.GetRowAppDataEx(irow); return(dex.m_inx3); }
	int								GetIndex4(int irow)				{ TAppDataEx dex; dex.m_data = m_grid.GetRowAppDataEx(irow); return(dex.m_inx4); }

	int								NumRows()					{ return(m_grid.NumRows()); }

protected:

	enum { clmn_row_descr = 0 };		// This is optional column. It is present in some of the views only.

	enum
	{
		clmn_rule_index_offs = 0,
		clmn_rule_non_term_offs = 1,
		clmn_rule_front_space_offs = 2,
		clmn_first_parsing_state_offs = 3,
		clmn_first_rule_symbol_offs = 4,
	};

	enum
	{
		row_descr_clmn_width = 170,
		rule_index_clmn_width = 70,
		rule_index_clmn_width_extended = 90,
		rule_front_space_clmn_width = 30,
		trailing_clmn_width_normal = 20,
		trailing_clmn_width_extended = 90,
	};

	enum
	{
		rule_symbol_clmn_width = 120,
		extra_pars_stt_clmn_width = 38,
		extra_pars_stt_clmn_width_10k_states = 45,
	};

	struct TSymClmnInfo
	{
		int		m_pstt_clmn;			// Central parsing state column index.
		int		m_symbol_clmn;			// Index of the rule symbol column.

		void		AssignLastColumnsPair(TScreenItemsGridItem &grid) { assert(grid.NumColumns() >= 2); m_pstt_clmn = grid.NumColumns()-2; m_symbol_clmn = grid.NumColumns()-1; }
		int		NumParsingStateColumns() { return(2*(m_symbol_clmn-m_pstt_clmn)-1); }
	};

	typedef TStructsArray<TSymClmnInfo, 32, 64> TSymClmnInfosArray;

protected:

	TScreenItemsGridItem				m_grid;
	TScreenItemsGridBuffsArray			m_grid_info_storage;

	bool								m_use_wide_extra_pars_stt;

	int								m_rule_index_clmn;			// This is physical column. In other words, this is low level index of the screen
																// items grid. For now the value of this field can be either 0 or 1.
	int								m_alignment_ref_clmn;		// This is logical column in rule symbols. It describes symbol column that has
																// highlighted alignment column in front of it.
	TSymClmnInfosArray				m_sym_clmns_info;
};

struct TRuleStateTransitionNode
{
	WORD		istate;
	short		same_state_rows;		// This is number of rows that are occupied by the same parsing state in
										// the column. Only the first row in the range is stamped with this length.
										// All other rows contain -1 in this field. This info is collected starting only
										// from the second column. First column contains -1 in this field.
};

typedef TStructsArray<TRuleStateTransitionNode, 12, 12> TRuleStateTransitionsArray;

class TRuleStateTransitionsRow
{
public:

	TRuleStateTransitionsRow(TSymbolsArray *pbuff) { m_action_symbols = pbuff; }

	TSymbolsArray				m_initial_states;				// When rows are initially created inside PrepareStateTransitionRows(), this field
															// is empty. When transitions are created for empty rule, there is only one column
															// in the martix and and this field remains empty. For all other rules the destination
															// states are merged. After merging this array contains one or more initial states.
															// Note that after merging states the m_transitions[0] contains the state 0xFFFF.
	TRuleStateTransitionsArray		m_transitions;				// Array of transitions always contains at least 1 item.
	TSymbolsArray				*m_action_symbols;			// Action symbols array, if they are collected, is not NULL only in the first row of
															// the range of rows with identical final parsing state.

	inline void	FixObjectMove()	{ m_initial_states.FixObjectMove(); m_transitions.FixObjectMove(); }
	inline void	ReleaseObject()	{ m_initial_states.ReleaseObject(); m_transitions.ReleaseObject(); if (m_action_symbols != NULL) { delete m_action_symbols; m_action_symbols = NULL; } }
	inline void	WipeObject()		{ m_initial_states.WipeObject(); m_transitions.WipeObject(); m_action_symbols = NULL; }
};

typedef TStructsArrayEx2<TRuleStateTransitionsRow, 16, 64> TRuleStateTransitionRowsArray;

class TLangViewBuildStateTransitionsHelper
{
public:

	TLangViewBuildStateTransitionsHelper() { m_grammar = NULL; }

	void		SetGrammar(TGrammar *grammar) { if (grammar != m_grammar) { m_grammar = grammar; m_rule_start_stts_index.Clear(); } }

	bool		IndexesPresent() const { return(m_rule_start_stts_index.NumItems() > 0); }
	bool		PrepareIterationIndexes(void (*ProgressHandler)(void *context, int items_done, int total_items) = NULL, void *context = NULL);

	void		ClearResult() { m_result.Clear(); }

	bool		PrepareStateTransitionRows(TBasicDialog *dlg, int irule, int isym, WORD pars_stt_at_isym, WORD action_sym_restr,
											bool expand_pars_stts_left, bool expand_pars_stts_right, bool collect_action_symbols);
				//
				//  Current app layer code is not using action_sym_restr param. Although it can be still passed.
				//  In this case the result set might be empty.
				//

	int		NumResultRows() const { return(m_result.NumItems()); }
	int		NumResultColumns() const { return((NumResultRows() > 0) ? m_result[0].m_transitions.NumItems() : -1); }

	const	TRuleStateTransitionsRow &ResultRow(int irow) const { return(m_result[irow]); }

protected:

	bool		CreateStateTransitionsRow(TBasicDialog *dlg, int irule, int isym, WORD pars_stt_at_isym, int num_transition_nodes,
											WORD action_symbol = 0xFFFF, TSymbolsArray **syms_buff = NULL);

	bool		ProcessFinalStatesRange(TBasicDialog *dlg, int irow_start, int &num_rows, int num_columns);
	bool		ProcessInitialStatesRange(TBasicDialog *dlg, int irow_start, int &num_rows, int &num_rows_total);
	void		ProcessDestinationStates(int irow_start, int num_rows, int num_columns);

	static int  __cdecl	CompareTransitionRows(void *raw_ctx, const TRuleStateTransitionsRow *pel1, const TRuleStateTransitionsRow *pel2);

protected:

	TGrammar						*m_grammar;					// Grammar is not owned by the helper.
	TRulePosIterIndexLevel2			m_rule_start_stts_index;

	TRuleStateTransitionRowsArray		m_result;

};

class TLangViewGrammarTreesHelper
{
public:

	TLangViewGrammarTreesHelper(TBasicDialog *dlg, TGrammar *grammar) : m_dialog(*dlg), m_grammar(*grammar) { }

	TLangViewTreeNode	*CreateGrammarOverviewHier(TLangViewTreeNode *parent_node, TGrammar *grammar, bool show_inividual_grammar_conflicts);
							// This method is not setting any props into the passed parent node and this node can be NULL. It creates
							// single child for the passed grammar and appropriate subchildren for this grammar.

	void		CreateNonTerminalDependenciesTree(TLangViewSpecialTreeNode *parent_node, WORD non_term, bool show_terminals);
	void		CreateNonTerminalRuleUsesTree(TLangViewSpecialTreeNode *parent_node, WORD non_term);

	void		CreatePathsTreeForXpctConflict(TLangViewSpecialTreeNode *parent_node, int cat_ixpct);
	void		CreatePathsTreeForGrammarConflict(TLangViewSpecialTreeNode *parent_node, int cat_iconflict);
	void		CreatePathsTreeForConflictingAction(TLangViewSpecialTreeNode *parent_node, int iconflict, int iaction);
				// These methods set appropriate props into the passed parent node and create children under this node.
				// They show message boxes when they are not able to display the whole tree. After return the passed parent
				// node owns the tree that these methods managed to create.

	void		CreateDerivationPathAst(TLangViewTreeNode *parent_node, TGrammarDerivationPath &path, WORD conflicting_symbol_value, TLangViewTreeNode *&conflicting_symbol);
				// conflicting symbol value is passed only for calling asserts. The conflicting symbol object is given out for its additional decoration.

protected:

	bool		CreateGrammarConflictInstances(TLangViewSpecialTreeNode *parent_node, TGrammar *grammar);

	bool		CreateXpctConflictTree(TLangViewSpecialTreeNode *parent_node, int ixpct);
	bool		CreateGrammarConflictSubtree(TLangViewSpecialTreeNode *parent_node, int iconflict);
	bool		CreateGrammarConflictTree(TLangViewSpecialTreeNode *parent_node, int iconflict);
	bool		CreateConflictingActionTree(TLangViewSpecialTreeNode *parent_node, int iconflict, int iaction);

	TLangViewTreeNode	*CreateAstFragmentForRule(WORD irule, TLangViewTreeNode *non_term = NULL, WORD imark = 0xFFFF, TLangViewTreeNode *mark_item = NULL);
	TLangViewTreeNode	*CreateSymbolTreeNode(TLangViewTreeNode *owner, WORD sym);

	void		HighlightUpperLink(TLangViewTreeNode *node);
	void		HighlightRightLink(TLangViewTreeNode *node, bool thick_link = TRUE);
	void		HighlightDescendingPath(TLangViewTreeNode *non_term_defn_root, TLangViewTreeNode *conflicting_symbol);

	static TSpecialLexItemType	GetCategoryTypeFromCategoryCode(int cat_code);
	static bool				IsSymbolPresentAmongChildren(TLangViewTreeNode *node, WORD symbol);

	typedef TStructsArray<TLangViewNonTerminalTreeNode*, 256, 256> TNonTerminalTreeNodePtrsArray;

	void		CheckAddToTheToDoList(TNonTerminalTreeNodePtrsArray &list_to_do, TLangViewNonTerminalTreeNode *candidate_node);

protected:

	TBasicDialog			&m_dialog;
	TGrammar			&m_grammar;
};

class TLangViewMiscGrammarAnalysisHelper
{
public:

	TLangViewMiscGrammarAnalysisHelper(TLangViewBrowserFrame *dlg_frame, TGrammar *grammar) : m_dialog_frame(*dlg_frame), m_grammar(*grammar) { }

	void		FindEmptyAndTransparentNonTerminals(TDestinationFile &rprt);
				// Empty non terminals have empty rules. Transparent non terms allow empty representatuion.

	void		FindPrimitiveNonTerminals(TDestinationFile &rprt);
				// Primitive non terminals are ones, that have either empty rules, or rules that consist of terminal symbols only.

	void		CreateNonTermsUseStatistics(TDestinationFile &rprt);
	void		CreateNonTermDefsDistanceStatistics(TDestinationFile &rprt);
	void		CreateNonTermDefsCallsDistanceStatistics(TDestinationFile &rprt);

	void		CreatePathLengthsStatistics(TDestinationFile &rprt);
	void		CreateStarterSymbolDiffsReport(TDestinationFile &rprt);
	void		CreateStartersReportForSymbolsPair(TDestinationFile &rprt, WORD sym1, WORD sym2, TAnalysisTable &first_and_follows, TAnalysisTable &non_term_non_term_starters);
	void		DumpMasksRelation(TDestinationFile &rprt, WORD sym1, WORD sym2, TAnalysisTable &first_and_follows, TAnalysisTable &non_term_non_term_starters, bool (*pfn)(WORD mask_elem1, WORD mask_elem2));

	void		ClassifyAllGrammarConflicts(TDestinationFile &rprt) { ClassifyGrammarConflicts(rprt, FALSE); }
	void		ClassifyUnexpectedConflicts(TDestinationFile &rprt) { ClassifyGrammarConflicts(rprt, TRUE); }
	void		ClassifyGrammarConflicts(TDestinationFile &rprt, bool unexpected_conflicts_only);

	static bool	CollectDirectlyInvolvedPositions(TRulePositionsArray &directly_conflicting_positions, TGrammar *grammar, int iconflict);
	static void	WriteDelimiter(TDestinationFile &rprt) { rprt.WriteLine(L"--------------------------------------------------------------------------------------"); }

protected:

	struct TNonTermSuppInfo
	{
		WORD				m_sym;
		double				m_symbol_weight;
		TSymbolsArray		m_dependent_symbols;

		static int __cdecl CompareByWeight(const TNonTermSuppInfo *si1, const TNonTermSuppInfo *si2)
		{
			// Compare weights and push bigger values up.
			if (si1->m_symbol_weight > si2->m_symbol_weight)
				return(-1);
			else if (si1->m_symbol_weight < si2->m_symbol_weight)
				return(1);

			// The counts are identical. Compare by symbol value and push bigger values down.
			if (si1->m_sym > si2->m_sym)
				return(1);
			else if (si1->m_sym < si2->m_sym)
				return(-1);

			assert(FALSE);
			return(0);
		}

		inline void FixObjectMove()	{ m_dependent_symbols.FixObjectMove(); }
		inline void ReleaseObject()	{ m_dependent_symbols.ReleaseObject(); }
		inline void WipeObject()		{ m_dependent_symbols.WipeObject(); }
	};

	typedef TStructsArrayEx2<TNonTermSuppInfo, 256, 1024> TNonTermSuppInfosArray;

	int		GetNonTerminalSymbolsDistance(WORD sym1, WORD sym2);
	bool		PrepareUseInOtherNonTermRulesDependencies(TNonTermSuppInfosArray &infos);

	struct TGrammarConflictsSignature
	{
		bool					m_signature_dumped;

		TRulePositionsArray	m_directly_involved_positions;
		TSymbolsArray		m_conflict_indexes;

		inline void FixObjectMove()	{ m_directly_involved_positions.FixObjectMove(); m_conflict_indexes.FixObjectMove(); }
		inline void ReleaseObject()	{ m_directly_involved_positions.ReleaseObject(); m_conflict_indexes.ReleaseObject(); }
		inline void WipeObject()		{ m_directly_involved_positions.WipeObject(); m_conflict_indexes.WipeObject(); }
	};

	typedef TStructsArrayEx2<TGrammarConflictsSignature, 32, 1024> TGrammarConflictSignaturesArray;

	bool		CollectRulesForSameRightSidePattern(TGrammarConflictSignaturesArray &sigs, TSymbolsArray &rules_list, int &num_conflicts_in_pattern, TSymbolsArray &rule_body, WORD rule_isym);
	bool		CollectRulesForSameSingleSymPattern(TGrammarConflictSignaturesArray &sigs, TSymbolsArray &rules_list, int &num_conflicts_in_pattern, WORD input_sym);
	bool		IsSameRuleBodyPositions(TRulePositionsArray &array, TSymbolsArray &rule_body, WORD rule_isym);
	bool		IsSameInputSymbolConflicts(TSymbolsArray &conflict_indexes, WORD input_sym);
	int		NumDifferentInputSymbols(TSymbolsArray &conflict_indexes);
	void		DumpConflictsSignature(TDestinationFile &rprt, int isig, TGrammarConflictsSignature &sig);

	static int __cdecl CompareConflictIndexes(void *ctx, const WORD *pc1, const WORD *pc2)
	{
		TLangViewMiscGrammarAnalysisHelper *hlpr = (TLangViewMiscGrammarAnalysisHelper*)ctx;
		TGrammarConflict &cfct1 = hlpr->m_grammar.conflicts[*pc1];
		TGrammarConflict &cfct2 = hlpr->m_grammar.conflicts[*pc2];

		// Compare the input symbols first.
		if (cfct1.input_symbol > cfct2.input_symbol)
			return(1);
		else if (cfct1.input_symbol < cfct2.input_symbol)
			return(-1);

		// Compare conflict indexes.
		if (*pc1 > *pc2)
			return(1);
		else if (*pc1 < *pc2)
			return(-1);

		assert(FALSE);
		return(0);
	}

protected:

	TLangViewBrowserFrame		&m_dialog_frame;
	TGrammar					&m_grammar;
};

#endif	// LangView_GrammarViewHelpers_H


