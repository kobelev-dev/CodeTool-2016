//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_GrammarView_H
#define	LangView_GrammarView_H

#ifndef   Common_StacksAndQueues_H
#include  "Common/StacksAndQueues.H"
#endif
#ifndef   WinUi_DialogsDock_H
#include  "WinUi/DialogsDock.H"
#endif
#ifndef   ScreenItems_ScreenItemsGrid_H
#include  "ScreenItems/ScreenItemsGrid.H"
#endif
#ifndef   ScreenItems_ScreenItemsTree_H
#include  "ScreenItems/ScreenItemsTree.H"
#endif
#ifndef   TextScan_GrammarAnalysis_H
#include  "TextScan/GrammarAnalysis.H"
#endif
#ifndef   TextScan_RamBasedStore_H
#include  "TextScan/RamBasedStore.H"
#endif
#ifndef   LangView_LangView_H
#include  "LangView/LangView.H"
#endif
#ifndef   LangView_LexItems_H
#include  "LangView/LexItems.H"
#endif
#ifndef   LangView_GrammarViewHelpers_H
#include  "LangView/GrammarViewHelpers.H"
#endif
#ifndef   LangView_LangViewResources_H
#include  "LangView/LangViewResources.H"
#endif

//
//  Possible view types in the RulesGrid dialog.
//
enum TLangViewRulesGridDialogMode
{
	grdm_none,

	grdm_grms_overview,				//  (1)  ShowGrammarsTreeOverview(root_grammar);
	grdm_terminal_symbols,				//  (2)  ShowTerminalSymbols(grammar);
	grdm_non_terminals,				//  (3)  ShowNonTerminals(grammar);
	grdm_non_term_depends,			//  (4)  ShowNonTerminalDependencies(grammar, non_term);
	grdm_grammar_rules,				//  (5)  ShowGrammarRules(grammar, -1/symbol/actb_reduce+irule);
	grdm_parsing_states,				//  (6)  ShowParsingStates(grammar, -1/istate);

	grdm_processing_status,				//  (7)  ShowConflictsAnalysisStatus(grammar);
	grdm_xpct_conflicts,					//  (8)  ShowXpctConflicts(grammar, -1/ixpct);
	grdm_grammar_conflicts,			//  (9)  ShowGrammarConflicts(grammar, -4/-3/-2/-1/iconflict);
	grdm_key_rules_xpct,				// (10) ShowKeyRulesForXpctConflicts(grammar, -1/ixpct);
	grdm_key_rules_conflict,				// (11) ShowKeyRulesForGrammarConflicts(grammar, -4/-3/-2/-1/iconflict);

	grdm_dplist_differs_xpct,			// (12) ShowDiffersTreeForXpctConflicts(grammar, iconflict, -1/ixpct);
	grdm_dplist_differs_conflict,			// (13) ShowDiffersTreeForGrammarConflicts(grammar, iconflict, -4/-3/-2/-1/iconflict);
	grdm_dplist_symbols_xpct,			// (14) ShowSymbolSeqsForXpctConflicts(grammar, iconflict, -1/ixpct);
	grdm_dplist_symbols_conflict,		// (15) ShowSymbolSeqsForGrammarConflicts(grammar, iconflict, -4/-3/-2/-1/iconflict);

	grdm_dplist_differs_action,			// (16) ShowDiffersTreeForConflictingAction(grammar, iconflict, iaction);
	grdm_dplist_symbols_action,			// (17) ShowSymbolSeqsForConflictingAction(grammar, iconflict, iaction);

	grdm_deriv_path_ast,				// (18) ShowDerivationPathAsAst(grammar, iconflict, iaction, ipath);
	grdm_deriv_path_seq,				// (19) ShowDerivationPathAsSeq(grammar, iconflict, iaction, ipath);
	grdm_deriv_path_steps,				// (20) ShowDerivationPathAsSteps(grammar, iconflict, iaction, ipath);

	grdm_num_types,
};

//
//  Possible ways of displaying the parsing state transitions.
//
enum TParsingStateTransitionsViewMode
{
	pstt_trnv_none,						// Parsing state transitions are not displayed at all.

	pstt_trnv_simple,					// SLR/LR. Parsing state transitions are shown without action symbols.

	pstt_trnv_show_act_syms,			// SLR/LR. In SLR case rule action symbols are associated with non terminals.
										// So, they apply to all final states of the rule. In LR case each parsing state
										// has its own set of rule action symbols.
	pstt_trnv_num_types,
};

//
//  Grammar rules, parsing states, grammar conflict, derivation paths as grid of objects or as tree of objects.
//
//  Template: IDD_LANGV_GRMV_RULES_GRID
//
class TLangViewRulesGridDialog : public TLangViewChildBrowser
{
public:

	TLangViewRulesGridDialog(TMidLevStoreReader *db_link);
	~TLangViewRulesGridDialog() { }

	void		ShowTitles(bool show_curr_dialog_mode, bool show_curr_grammar_name) { m_show_dialog_mode = show_curr_dialog_mode; m_show_grammar_name = show_curr_grammar_name; }
				// Visibility of the titles will be updated when the dialog will be opened next time or when the dialog mode will be changed.

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	void		ProcessMouseClick(TScreenItemsViewNtfCode notificationCode, TScreenItemsViewMouseNotify *mouse_ntf);
	void		ProcessMainGridRowBkgrClick(TScreenItemsViewNtfCode notificationCode, int irow);
	void		ProcessMainGridOrTreeLexItemClick(TScreenItemsViewNtfCode notificationCode, TLangViewLexItem *lex_item);
	void		ProcessMainGridSpecLabelClick(TScreenItemsViewNtfCode notificationCode, TGrammarViewLabelItem *label_item);

	void		ProcessRightBottomCornerEmptyViewerClick(long mouse_x, long mouse_y);
	void		ProcessRightBottomCornerClick(long mouse_x, long mouse_y);

	void		AddShowStateTransitionsModeMenu(TMenuItemInfo *menu_info, int &menu_items_used);
	void		AddShowXpctConflictNameMenu(TMenuItemInfo *menu_info, int &menu_items_used);
	void		ExecPopupMenuCommand(DWORD menu_command, wchar_t *dynamic_item_buff);
	void		UpdateScreenShotForClipboard(HDC hMemoryDC, long image_size_x, long image_size_y);

	void		ResetContents() { ResetContentsInternal(FALSE); }
	void		SetSelection(TLangViewClickEvent &event, bool show_margins) { }
	void		ResetSelection();
	void		SetSortOrder(int list_id, DWORD new_sort_order) { assert(FALSE); }

	struct TDialogStateInfo
	{
		TLangViewRulesGridDialogMode		m_mode;
		TGrammar						*m_grammar;
		int								m_index1, m_index2, m_index3;
		long								m_scroll_pos_x, m_scroll_pos_y;

		bool CompareTo(const TDialogStateInfo &inst) const
		{
			return(m_mode == inst.m_mode && m_grammar == inst.m_grammar && m_index1 == inst.m_index1 && m_index2 == inst.m_index2 && m_index3 == inst.m_index3);
		}
	};

	void		ResetContentsInternal(bool preserve_symbol_lists_slct = FALSE);
	void		PlaceCurrentContentsTo(TDialogStateInfo &info_buffer);
	void		LoadContentsFrom(TLangViewRulesGridDialog *inst);
	void		LoadContentsFromInfo(TDialogStateInfo &info);

	void		SetGrammarSymbolsListSelection(WORD slct_sym_frgr, TSymbolsArray *slct_syms_bkgr);
				// Method takes ownership on the passed non terminals list.

	//
	//  Public load methods. Each dialog mode has its own load function.
	//

	void		ShowGrammarsTreeOverview(TGrammar *root_grammar);
				//
				//	Method shows the structure of the grammars tree. It verifies that passed
				//	grammar is really a root grammar.
				//

	void		ShowTerminalSymbols(TGrammar *grammar);
				//
				//	Method shows list of the terminal symbols if any.
				//

	void		ShowNonTerminals(TGrammar *grammar);
				//
				//	Method shows list of non terminals if any.
				//

	void		ShowNonTerminalDependencies(TGrammar *grammar, WORD non_term);
				//
				//	Method shows tree of non terminal dependencies.
				//
				//	Each symbol of the tree has either ALL symbols that can be used to build this symbol or none of them.
				//	Children of the symbol are not displayed either if this symbol is foreign to the root if the tree or if it is
				//	already present (with ALL its children) somewhere else on the graph.
				//

	void		ShowGrammarRules(TGrammar *grammar, int display_task = -1);
				//
				//	Meaning of the display task parameter:
				//
				//         -1			--	display all rules;
				//     symbol_value	--	show symbol use report;
				//   actb_reduce+irule	--	display only one rule;
				//

	void		ShowParsingStates(TGrammar *grammar, int istate = -1);
				//
				//	Meaning of the display task parameter:
				//
				//         -1			--	display all states;
				//        istate			--	show only one sate;
				//

	void		ShowConflictsAnalysisStatus(TGrammar *grammar);
				//
				//	Method shows either single text message or list of conflicts and actions that have problems
				//	in the conflict analysis.
				//

	void		ShowXpctConflicts(TGrammar *grammar, int ixpct = -1);
				//
				//	Meaning of the display task parameter:
				//
				//         -1			--	display all expected conflicts;
				//        ixpct			--	show only one xpct;
				//

	void		ShowGrammarConflicts(TGrammar *grammar, int display_task = -4);
				//
				//	Meaning of the display task parameter:
				//
				//         -4			--	show all grammar conflicts;
				//         -3			--	show all ambiguous grammar conflicts;
				//         -2			--	show all successfully assigned grammar conflicts;
				//         -1			--	show all unexpected grammar conflicts;
				//       iconflict		--	show one grammar conflict;
				//

	void		ShowKeyRulesForXpctConflicts(TGrammar *grammar, int ixpct = -1);
				//
				//	Meaning of the display task parameter:
				//
				//         -1			--	display key rules for all expected conflicts;
				//        ixpct			--	show key rules for only one xpct;
				//

	void		ShowKeyRulesForGrammarConflicts(TGrammar *grammar, int display_task = -4);
				//
				//	Meaning of the display task parameter:
				//
				//         -4			--	show all grammar conflicts;
				//         -3			--	show all ambiguous grammar conflicts;
				//         -2			--	show all successfully assigned grammar conflicts;
				//         -1			--	show all unexpected grammar conflicts;
				//       iconflict		--	show key rules for only one grammar conflict;
				//

	void		ShowDiffersTreeForXpctConflicts(TGrammar *grammar, int ixpct = -1);
				//
				//	Meaning of the display task parameter:
				//
				//         -1			--	display key rules for all expected conflicts;
				//         ixpct			--	show key rules for only one xpct;
				//

	void		ShowDiffersTreeForGrammarConflicts(TGrammar *grammar, int display_task = -4);
				//
				//	Meaning of the display task parameter:
				//
				//         -4			--	show all grammar conflicts;
				//         -3			--	show all ambiguous grammar conflicts;
				//         -2			--	show all successfully assigned grammar conflicts;
				//         -1			--	show all unexpected grammar conflicts;
				//       iconflict		--	show key rules for only one grammar conflict;
				//

	void		ShowSymbolSeqsForXpctConflicts(TGrammar *grammar, int ixpct = -1);
				//
				//	Meaning of the display task parameter:
				//
				//         -1			--	display key rules for all expected conflicts;
				//        ixpct			--	show key rules for only one xpct;
				//

	void		ShowSymbolSeqsForGrammarConflicts(TGrammar *grammar, int display_task = -4);
				//
				//	Meaning of the display task parameter:
				//
				//         -4			--	show all grammar conflicts;
				//         -3			--	show all ambiguous grammar conflicts;
				//         -2			--	show all successfully assigned grammar conflicts;
				//         -1			--	show all unexpected grammar conflicts;
				//       iconflict		--	show key rules for only one grammar conflict;
				//

	void		ShowDiffersTreeForConflictingAction(TGrammar *grammar, int iconflict, int iaction);
				//
				//	This method shows data for one conflicting action only.
				//

	void		ShowSymbolSeqsForConflictingAction(TGrammar *grammar, int iconflict, int iaction);
				//
				//	This method shows data for one conflicting action only.
				//

	void		ShowDerivationPathAsAst(TGrammar *grammar, int iconflict, int iaction, int ipath);
				//
				//	This method shows data for one derivation path only.
				//

	void		ShowDerivationPathAsSeq(TGrammar *grammar, int iconflict, int iaction, int ipath);
				//
				//	This method shows data for one derivation path only.
				//

	void		ShowDerivationPathAsSteps(TGrammar *grammar, int iconflict, int iaction, int ipath);
				//
				//	This method shows data for one derivation path only.
				//

	TLangViewRulesGridDialogMode		GetCurrDialogMode() const { return(m_dialog_mode); }
	TGrammar						*GetCurrGrammar() const { return(m_grammar); }

	int		GetSingleItemModeObjIndex() const { return(m_single_obj_index); }
				// This method is needed for visual parser to check if current state of the parser is still equal to
				// the currently displayed parsing state of the viewer or not.

	void		ResetStateTransitionsHelper() { m_sttr_helper.SetGrammar(NULL); }
				// This method is used after loading the grammar definition because in this case grammar structure
				// changes its contents while its address remains the same.

protected:	//-----  Section 2  -----------------------

	bool		ShowRulesViewRule(TLangViewBuildStateTransitionsHelper *sttr_hlpr, int irule, TBasicStyle *row_hdr_color, WORD symbol_to_highlight = 0xFFFF);
	bool		ShowStatesViewState(TLangViewBuildStateTransitionsHelper *sttr_hlpr, int istate, bool show_state_transitions);

	enum	{ rule_body_row_height = 40, pars_stt_on_clnm_ref_row_height = 3 };

	int		ShowRuleBody(int ins_before_row, int irule, int iclmn_start = 0, WORD symbol_to_highlight = 0xFFFF, bool key_conflicting_rule = FALSE, bool want_spc_for_stt_on_clmn_ref_and_delim = FALSE);
				// This method creates new row. Param iclmn_start is expressed in symbols or "logical" columns. It can
				// have any value including negative. In this case extra columns will be added at the left side of the grid.

	bool		ShowParsingStateOnClmnRef(int ins_before_row, WORD istate, TLangViewRulesGridRowMeaning row_meaning, WORD inx2 = 0, WORD inx3 = 0, int inx4 = 0);
	bool		ShowParsingStateLabelOnClmnRef(int irow, WORD istate);

	bool		AddMissingSymbolColumns(int &iclmn_start, int seq_len);
				// Passed iclmn_start can be negative. Method adds appropriate number of columns to allow displaying
				// symbols sequence of the passed length.

	void		AddGrammarSymbolSelection(int irow, int iclmn);
				// Method expects that the passed cell already contains the grammar symbol screen item in the layer 0.
				// It adds the selection object into the layer 1 of the same cell.

protected:	//-----  Section 3  -----------------------

	TParsingStateTransitionsViewMode	GetCurrTrnvMode();
	TLangViewBuildStateTransitionsHelper *GetPreparedTransitionsHelper(bool want_index);

	bool		ShowStateTransitionRows(TLangViewBuildStateTransitionsHelper *sttr_hlpr, int iclmn_rule_start, int irule, int isym, WORD pars_stt_at_isym,
											WORD rule_action_sym_restr, bool expand_pars_stts_left, bool expand_pars_stts_right);

	enum	{ state_transitions_row_height = 12 };

	bool		ShowRuleStatesTree(TLangViewBuildStateTransitionsHelper *sttr_hlpr, TGrammarViewLabelItem *owner, int main_grid_irow, int iclmn_start, int result_set_irow, int result_set_iclmn, int num_rows);
	bool		ShowRuleInitialStates(TLangViewBuildStateTransitionsHelper *sttr_hlpr, TGrammarViewLabelItem *owner, int main_grid_irow, int iclmn_start, int result_set_irow, int num_rows);

	enum	{ action_symbols_grid_cell_size_x = 34, action_symbols_grid_cell_size_y = 24 };

	int		ShowRuleActionSymbols(int main_grid_irow, int final_stt_irow, int final_stt_sym_clmn, int num_rows, TSymbolsArray *action_symbols_buff);
				// All params of the method do not have relation to the result set. This method expects that layer [0] of the passed cell
				// contains the final parsing state what will become the owner of the items in the inner grid. Return value is the number
				// of added row pairs or -1 in case of failure.

	int		CreateStateTransitionsRow(const wchar_t *descr, int row_height, int irule);
	bool		CreateExtraRowsPair(int main_grid_irow, int num_rows, long extra_rows_height);

protected:	//-----  Section 4  -----------------------

	bool		ShowXpctConflictOverview(int ixpct, bool &grammar_conflicts_available);
	bool		ShowGrammarConflictOverview(int iconflict, TBasicStyle *upper_delim_style, bool allow_showing_xpct_name);

	enum	{ xpct_name_left_shift = 5 };

	bool		ShowXpctConflictHeader(int ixpct);
	bool		ShowGrammarConflictHeader(int iconflict, TBasicStyle *upper_delim_style, bool allow_showing_xpct_name, bool allow_lower_obscuring_line);
	bool		ShowConflicingActionHeader(int iconflict, int iaction, TBasicStyle *upper_delim_style);
	bool		CreateFancyObscuringLine(const wchar_t *descr, int irow, TBasicStyle *bkgr_style);
	void		ShowConflictAnalysisDetails(int irow, int iconflict, int iaction);

	bool		CheckConflictAnalysisResultsPresence(int iconflict);
				// Return value is TRUE if conflict is in problem or analysis was started at least for one of its actions.

	bool		CheckConflictsAnalysisForConflict(int iconflict, bool &processing_started, bool &processing_finished);
				// Return value and parameters show if any action of the conflict falls into the corresponding category.

	bool		CheckConflictsAnalysisForAction(int iconflict, int iaction, bool &processing_started, bool &processing_finished);
				// Return value is TRUE if there are processing problems and FALSE if there is no problems.

protected:	//-----  Section 5  -----------------------

	bool		ShowXpctConflictKeyRules(int ixpct, bool &grammar_conflicts_available);
	bool		ShowGrammarConflictKeyRules(int iconflict, int iaction);

	enum     { actions_map_cell_x = 30, action_map_cell_y = 11 };

	TScreenItemsGridItem  *CreateActionsMapGrid(int ixpct, int num_emb_rows, int num_emb_columns, TDerivationPathAnalysisHelper::TConflictContextOriginsArray &orgs);

	bool		ShowConflictingActionKeyRules(int iconflict, int iaction, TBasicStyle *upper_delim_style);
	bool		ShowCtxRecordKeyRulesAndSeqs(TDerivationPathAnalysisHelper::TConflictContextRecord &ctx_record, bool &seqs_present);

	enum	{ symbol_seqs_row_height = 25 };

	bool		ShowGrammarSymbols(int irow, int iclmn_start, WORD *sym_data, int sym_data_len, bool show_lex_items);

protected:	//-----  Section 6  -----------------------

	bool		ShowXpctConflictSymbolSeqs(int ixpct, bool &grammar_conflicts_available);
	bool		ShowGrammarConflictSymbolSeqs(int iconflict, TBasicStyle *upper_delim_style, bool allow_showing_xpct_name);
	bool		ShowConflictingActionSymbolSeqs(int iconflict, int iaction, TBasicStyle *upper_delim_style);
	bool		ShowDerivationPathAsSymbolsSeq(int iconflict, int iaction, int ipath, bool show_path_in_lex_items);
	bool		ShowDerivationStepDetails(int irow, int ipath, int istep, TFullRulePosition &path_slot, int special_rule_index);

protected:	//-----  Section 7  -----------------------

	void		ResetSecondaryFlags();
	void		FixSimpleParam(int &param);
	void		FixGrammarConflictParam(int &param);
	void		StoreMajorTaskParams(TLangViewRulesGridDialogMode mode, TGrammar *grm, int inx2 = -1, int inx3 = -1, int inx4 = -1);
	void		SetUpdateControlsLayout();

	bool		IsTreeBasedDialogMode() const { return(m_dialog_mode == grdm_grms_overview || m_dialog_mode == grdm_non_term_depends || m_dialog_mode == grdm_dplist_differs_xpct || m_dialog_mode == grdm_dplist_differs_conflict || m_dialog_mode == grdm_dplist_differs_action || m_dialog_mode == grdm_deriv_path_ast); }

	void		PrepareNewDialogMode(const wchar_t *mode_name, const wchar_t *status_panel_msg);
	bool		CheckCurrGrammarForXpctConflictsPresence();
	bool		CheckCurrGrammarForGrammarConflictsPresence(int display_task);
	bool		PrepareGridCreation();

	int		CreateHeaderRows(const wchar_t *rows_descr, TLangViewRulesGridRowMeaning row_meaning,
						TBasicStyle *upper_delim_style, int header_height, TBasicStyle *bkgr_style, TBasicStyle *lower_delim_style,
						WORD inx2 = 0, WORD inx3 = 0, int inx4 = 0, int ins_before_row = SCRITEMS_GRID_APPEND,
						TLangViewRulesGridRowMeaningEx ex_meaning = ROW_EX_none);

	bool		CreateMessageRow(const wchar_t *row_descr, TLangViewRulesGridRowMeaning row_meaning,
						int iclmn, long horz_offset, const wchar_t *message, TBasicStyle *lower_delim_style = NULL,
						WORD inx2 = 0, WORD inx3 = 0, int inx4 = 0, int ins_before_row = SCRITEMS_GRID_APPEND);

	void		AddThreeRowsFooter(const wchar_t *rows_descr, int middle_row_height, TBasicStyle *middle_row_bkgr_style, bool want_full_footer = TRUE);

	void		PrepareAndShowCurrentGrid();

	void		PrepareAndShowCurrentTree(int *min_radius_steps_info = NULL, TLangViewTreeNode *conflicting_symbol = NULL);
				// Method expects that upper layer has created the tree and attached it to the m_tree_items_root data field.

	void		MapDerivationPathAstSpashIcon(TLangViewTreeNode *conflicting_symbol, TTreeItemsPlacingMode placing_mode);
	void		RemapDerivationPathAstRuleLabels(TRectLinesModePlacingProps &rect_lines_props);

	void		PrepareAndShowNoObjectsMessage(const wchar_t *message, TTextStyle *style = NULL);
	void		PrepareAndShowGrammarNotInFullSucceesMessage();

protected:	//-----  Section 8  -----------------------

	TTextItem80				*CreateTextScreenItem(const wchar_t *descr, TTextStyle *text_style = NULL);
								// Text labels are non clickable.

	TGrammarViewLabelItem	*CreateSpecLabelItem(const wchar_t *descr, TGrammarViewLabelItem::TLabelType gvlt, WORD app_value);
								// Method selects style for the text screen item based on the desired app type of the label.

	TLangViewLexItem			*CreateSymbolLexItem(WORD sym, short &off_bottom);
								// Parsing state objects are always clickable.

	TScreenItemsGridItem		*CreateInnerGrid(int num_rows, int num_columns, long cell_x, long cell_y, TBasicStyle *bkgr_style = NULL);
								// Inner grid is used for rule action symbols and participating conflicting actions.

	bool		CreateCenteredTextLabel(const wchar_t *descr, int physical_row_inx, int physical_clmn_inx, short off_bottom,
						TTextStyle *text_style, const wchar_t *format, ...);

	bool		CreateLeftAlignedTextLabel(const wchar_t *descr, int physical_row_inx, int physical_clmn_inx, short horz_shift,
						TTextStyle *text_style, const wchar_t *format, ...);

protected:	//-----  Section 9	-----------------------

	enum TermNonTermSymbolsTableLabelAlignment : unsigned short
	{
		// In fact this value is used as negative signed short, but because of the problems
		// in MS compiler this enum is defined using unsigned base type.
		SYMS_TABLE_CELL_CENTER = 0x8000,
	};

	enum	{ term_sym_clmn_sym, term_sym_clmn_name, term_sym_clmn_app_id };
	enum	{ non_term_clmn_sym, non_term_clmn_name, non_term_clmn_public, non_term_clmn_restr, non_term_clmn_app_id, non_term_clmn_starters };

	enum TSymbolsTableColumnMeaning
	{
		SYMS_ROW_none, SYMS_ROW_grammar_symbol,
	};

	void		AddTermNonTermSymbolsTableColumn(int clmn_width, TBasicStyle *delim_style);
	int		AddTermNonTermSymbolsTableRow(const wchar_t *descr, TSymbolsTableColumnMeaning row_meaning, int row_height, TBasicStyle *bkgr_style = NULL, TBasicStyle *delim_style = NULL, WORD inx2 = 0);
	bool		AddTermNonTermSymbolsTableTextLabel(const wchar_t *descr, int row_inx, int clmn_inx, short horz_shift, TTextStyle *text_style, const wchar_t *format, ...);

	bool		AddTerminalSymbolsTableTerminalRow(WORD term_sym, TBasicStyle *row_bkgr, bool select_frgr);
	bool		AddNonTermSymbolsTableNonTermRow(WORD non_term, TBasicStyle *row_bkgr, bool select_frgr, bool right_align_symbol_name, TAnalysisTable &first_and_follows, TAnalysisTable &non_term_non_term_starters);

	void		CreateSymbolsTreeSelections(TLangViewSpecialTreeNode &root, TLangViewLexItemSelectionType selt, WORD slct_value);
	bool		ReleaseSymbolsTreeSelections(TLangViewLexItemSelectionType selt);

	void		LoadStylesProfile(bool prepare_styles);
	void		PrepareStylesForDeviceContext(HDC hDC);

	void		PrepareBasicProfileStyle(TScreenItemStylePropsCollection &profile_props, TBasicStyle &style, const wchar_t *style_name);
	void		PrepareTextProfileStyle(TScreenItemStylePropsCollection &profile_props, TTextStyle &style, const wchar_t *style_name);
	void		PrepareLineProfileStyle(TScreenItemStylePropsCollection &profile_props, TLineStyle &style, const wchar_t *style_name);

	static void UpdateClipboardImageCallback(void *context, HDC hMemoryDC, SIZE &image_size)
	{
		TLangViewRulesGridDialog *inst = (TLangViewRulesGridDialog*)context;
		inst->UpdateScreenShotForClipboard(hMemoryDC, image_size.cx, image_size.cy);
	}

	static void BuildIndexesProgressCallbackHandler(void *context, int items_done, int total_items)
	{
		// This callback handler is only opening console without changing messages on it.
		TLangViewRulesGridDialog *inst = (TLangViewRulesGridDialog*)context;
		inst->m_local_console.CheckOpenStatusPanel();
	}

	// View styles profile handling.
	static   wchar_t		*g_StylesProfileName;
	static   TDateTime		g_LatestScannedProfileDate;

	// Clipboard image generation props.
	static   bool			g_need_image_origin_footer;

protected:

	long									m_initial_main_view_top;
	long									m_main_view_off_bottom;

	bool									m_show_dialog_mode;					// This flag should be set before calling any of the major ShowXyz() method.
	bool									m_show_grammar_name;				// This flag should be set before calling any of the major ShowXyz() method.

	TLangViewRulesGridDialogMode			m_dialog_mode;

	TGrammar							*m_grammar;							// The grammar is never owned by this dialog.
	int									m_single_obj_index;						// Object index or the category type.
	int									m_single_sub_obj_index;
	int									m_single_sub_sub_obj_index;

	TCircularStack<TDialogStateInfo, 20>		m_contents_history;
	bool									m_step_back_proc_flag;					// This flag is TRUE only during the step back processing.

	// Common flags that affect non terminals on all types of views.
	float									m_zoom_level[grdm_num_types];
	TTreeItemsPlacingMode					m_tree_placing_mode[grdm_num_types];
	bool									m_hgtlt_transp_non_terms;
	bool									m_show_terminals_grm_names;			// FALSE	-  show lexema type that terminal symbol represents.
																				// TRUE	-  show name of the terminal in the grammar source.
	// Grammars tree overview.
	bool									m_show_individual_grm_conflicts;

	// Symbol lists and non terminal dependencies.
	bool									m_terminals_sort_alpha_num;
	TNonTerminalsSortOrder				m_non_terms_list_sort_mode;
	bool									m_show_non_term_starters;				// When the flag is TRUE, terminal symbols that can start non terminals are displayed.
	bool									m_non_term_deps_show_terms;

	// Rules view - element[0], symbol use report - element[1] and parsing states view - element[2].
	TParsingStateTransitionsViewMode		m_pstt_trnv_mode[3];

	// Symbol use report flags.
	bool									m_sym_rprt_show_defn_rules;
	bool									m_sym_rprt_show_sym_uses;
	bool									m_sym_rprt_highlight_sym;

	bool									m_states_show_interm_positions;		// When this field is TRUE, only intermediate and final positions are displayed.
																				// Rules, where given parsing state stays in the beginning of the rule are skipped.
	// Conflict overviews, key rules and derivation paths as symbol sequencies.
	bool									m_show_xpct_conflict_name;

	// Conflicts overview flags (grdm_xpct_conflicts and grdm_grammar_conflicts).
	bool									m_show_conflicting_action_hdrs;
	bool									m_show_directly_conflicting_rules;

	// List of all grammar conflicts.
	bool									m_show_with_analysis_only;

	// Predecessors/followers report.
	bool									m_show_key_conflicting_rules;
	bool									m_merge_diff_key_rule_seqs;
	bool									m_show_predecessor_seqs;
	bool									m_show_follower_seqs;

	// Derivation paths/path as sequecies/sequecie of symbols.
	bool									m_show_path_seq_props;
	bool									m_show_single_path_seq_lex_items;

	// Derivation path as a sequence of steps.
	bool									m_show_deriv_path_dtls;
	bool									m_show_deriv_path_from_bottom;

	//
	//  View object and top level screen items.
	//

	TScreenItemsViewObject				m_view_helper;
	TLangViewGrammarRulesGrid			m_rules_grid;
	TLangViewSpecialTreeNode				m_tree_items_root;					// Exact type of the object is changed for every type of the view that uses this node.
	TTextItem							m_no_objects_message;				// This object does not need the text buffer because it displays only static messages.

	TLangViewSpecialTreeNode				m_aux_upper_tree_root;				// For now the second tree is displayed only for non terminal depends and only
																			// when the main tree is displayed in rect lines mode.

	TScreenItemsGridItem					m_symbols_table_grid;				// This grid is used on the terminals and non terms views only.

	TBitmapItem							m_cfct_sym_highlight1;
	TBitmapItem							m_cfct_sym_highlight2;
											// These objects highlight conflicting symbol on the derivation path AST views.

	WORD								m_grm_symbols_list_frgr_slct;		// Default value is 0xFFFF when nothing is selected.
	TSymbolsArray						*m_non_terms_list_bkgr_slct;
											// These two data fields describe current selection in the list of terminals and list of non terms.

	TScreenItemPtrsArray					m_tree_view_extra_items;
											// This array of pointers owns its objects. This is needed because painting list is not owning
											// its members. Info about selection screen items should be stored somewhere to prevent
											// leaking. Plus this array is used for removing current selection.

	int									m_sym_rprt_defn_rules;
	int									m_sym_rprt_use_rules;
											// These fields contain info about generated symbol use report.

	DWORD								m_grm_view_labels_owner;
	DWORD								m_frgr_lex_items_owner;			// This is owner of the TLangViewLexItem instances only. Do not mix them with
																			// TLangViewTreeNode instances. LexItem tree nodes are "self owned". In other
																			// words the owner field there contains pointer to the internal TLangViewLexItem.
	TLangViewBuildStateTransitionsHelper		m_sttr_helper;

	//
	//  Styles.
	//

	TLangViewLexItemStyle					m_lex_items_style;

	TBasicStyle							m_dark_delim_style;
	TBasicStyle							m_semi_light_delim_style;
	TBasicStyle							m_light_delim_style;
	TBasicStyle							m_symbols_vert_delim_style;
	TBasicStyle							m_light_syms_vert_delim_style;

	TBasicStyle							m_grammar_rules_hdr_style;
	TBasicStyle							m_parsing_states_hdr_style;

	TBasicStyle							m_xpct_conflict_hdr_style;
	TBasicStyle							m_grammar_cfct_hdr_style;
	TBasicStyle							m_cfcting_action_hdr_style;
	TBasicStyle							m_xpct_actions_map_hdr_style;

	TBasicStyle							m_derivation_path_hdr_style;
	TBasicStyle							m_symbol_use_rprt_hdr_style;

	TBasicStyle							m_predec_flwr_seqs_bkgr_style;

	TBasicStyle							m_non_term_clmn_style;
	TBasicStyle							m_pars_stt_align_clmn_style;
	TBasicStyle							m_conflict_align_clmn_style;
	TBasicStyle							m_derv_path_align_clmn_style;

	TBasicStyle							m_extern_non_term_slct_style;
	TBasicStyle							m_recurs_non_term_slct_style;

	TTextStyle							m_rule_index_text_style;
	TTextStyle							m_key_rule_index_text_style;
	TTextStyle							m_bold_index_text_style;
	TTextStyle							m_sect_title_text_style;

	TTextStyle							m_row_descr_text_style;
	TTextStyle							m_row_comment_text_style;
	TTextStyle							m_analysis_warning_text_style;
	TTextStyle							m_analysis_error_text_style;
	TTextStyle							m_action_missing_text_style;
	TTextStyle							m_action_present_text_style;
	TTextStyle							m_small_comment_text_style;

	TTextStyle							m_pars_stt_label_text_style;
	TTextStyle							m_no_objects_text_style;

	TTextStyle							m_clipboard_image_title_style;
	TTextStyle							m_clipboard_image_footer_style;

	TLineStyle							m_pars_stt_link_style;
	TLineStyle							m_action_syms_link_style;

private:

	TLangViewConsole						m_local_console;
};

//
//  Template: IDD_LANGV_GRMV_MAIN_FRAME
//
class TLangViewGrammarBrowser : public TLangViewBrowserFrame
{
public:

	TLangViewGrammarBrowser(TLangViewBrowserFrame *parent_handler, TMidLevStoreReader *db_link);

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	bool		IsGrammarBrowser() const { return(TRUE); }

	void		ProcessSingleClick(TLangViewChildBrowser *inst, TLangViewClickEvent &event_info);
	void		ProcessDoubleClick(TLangViewChildBrowser *inst, TLangViewClickEvent &event_info);
	void		RemoveSelection(TLangViewChildBrowser *inst);

	void		SetupGrammar(TGrammar *grammar, TMidLevStoreReader *db_link);
				// It is ok to pass NULL as the database link if info about the parsing session is not available.

	void		PromoteChildViewerContents(TLangViewRulesGridDialog *inst, bool promote_to_top);
	bool		CanPromoteChildViewer(TLangViewChildBrowser *inst) { return(inst != &m_summary && inst != &m_terminal_symbols && inst != &m_non_terminals && inst != &m_level1_view); }
	void		SetSymbolsListSelection(WORD slct_sym_frgr, TSymbolsArray *slct_syms_bkgr);

	TLangViewBuildStateTransitionsHelper		*GetStateTransitionsHelper() { return(&m_shared_sttr_helper); }

protected:

	enum { pri_viewer, sec_viewer, tri_viewer, frt_viewer, num_dest_viewers };
	typedef TLangViewRulesGridDialog *TDestViewersArray[num_dest_viewers];

	TLangViewRulesGridDialog *GetNextLayerViewer(TLangViewRulesGridDialog *inst);

	void		PrepareDestViewers(TDestViewersArray &dvs, TLangViewRulesGridDialog *inst_src);
	void		SetupDestViewers(TDestViewersArray &dvs, TLangViewRulesGridDialog *inst, TLangViewClickEvent &event_info);
	void		DoGrammarDefnLoading(bool build_parsing_table);
	void		DoMiscGrammarAnalysis(void (TLangViewMiscGrammarAnalysisHelper::*pfn)(TDestinationFile &rprt));
	void		SetMenuEnabledState(bool new_value);

protected:

	TFileOpenHelper							m_ofn_anres;

	// Dock helpers.
	THorzBasicDialogsDockDialog				m_global_horz_dock;
	TVertBasicDialogsDockDialog				m_symbols_vert_dock;
	TVertBasicDialogsDockDialog				m_main_vert_dock;
	THorzBasicDialogsDockDialog				m_upper_horz_dock;
	THorzBasicDialogsDockDialog				m_lower_horz_dock;
	TVertBasicDialogsDockDialog				m_lower_vert_dock;

	// Child viewers.
	TLangViewRulesGridDialog					m_summary;
	TLangViewRulesGridDialog					m_terminal_symbols;
	TLangViewRulesGridDialog					m_non_terminals;
	TLangViewRulesGridDialog					m_level1_view;
	TLangViewRulesGridDialog					m_level2_view;
	TLangViewRulesGridDialog					m_level3_view;
	TLangViewRulesGridDialog					m_level4_view;
	TLangViewRulesGridDialog					m_level5_view;

	TGrammar								*m_browser_grammar;
												// This grammar will be passed to the child viewers when the browser frame will be opened.

	TLangViewBuildStateTransitionsHelper			m_shared_sttr_helper;

private:

	TGrammar								m_local_grammar;
	TMidLevRamLogger							m_local_grammar_parsing_log;
	TMidLevRamStoreReader					m_local_grammar_parsing_log_reader;
};

#endif	// LangView_GrammarView_H


