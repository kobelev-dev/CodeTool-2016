//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Backus-Naur productions grammar processor.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/FormatNumber.H"
#include  "Common/DateTimeHelper.H"
#include  "TextScan/GrammarStructure.H"
#include  "TextScan/GrammarAnalysis.H"
#include  "Xml/XmlWriter.H"

#undef	WANT_EXTRA_DEEP_CTX				// This is temp evaliation feature.

//---------------------------------------------------------------------------
//  ================  TBuildGrammarDerivationSlot  ===================
//---------------------------------------------------------------------------

bool TBuildGrammarDerivationSlot::GetNextPosition()
{
	switch (m_frp.origin)
	{
		case rpo_rule_start_state:
		case rpo_start_state_ctx:
				{
					TRuleStartStatesIterator &iter = m_rule_start_states_iter;
					iter.Step();
					if (iter == TRUE)
					{
						// Starting state that derives to the requested state if the requested rule position is found.
						m_frp.istate = iter.CurrState();
						m_frp.action_sym = iter.CurrActionSymbol();
						return(TRUE);
					}
				}
				break;

		case rpo_rule_call_place:
		case rpo_call_place_ctx:
				{
					TRuleCallPlacesIterator &iter = m_rule_call_places_iter;
					iter.Step();
					if (iter == TRUE)
					{
						// Place where requested state stays before the requested non terminal or conflicting input symbol is found.
						TRulePosition &pos = iter.CurrPos();
						m_frp.irule = pos.irule;
						m_frp.isym = (BYTE)pos.isym;
						m_frp.action_sym = pos.action_sym;
						return(TRUE);
					}
				}
				break;

		case rpo_non_term_defn_rule:
				{
					TNonTermDefnRulesIterator &iter = m_non_term_defn_rules_iter;
					iter.Step();
					if (iter == TRUE)
					{
						// Some rule that defines target non terminal is found. Fill in the starting position of the current rule in the iter.
						m_frp.irule = iter.CurrRuleInx();
						m_frp.action_sym = iter.CurrentActionSymbol();
						return(TRUE);
					}
				}
				break;

		default:
			{
				// All other slot types are not iterateable.
				return(FALSE);
			}
	}

	// Iteration is complete.
	return(FALSE);
}

void TBuildGrammarDerivationSlot::SeekToEnd()
{
	switch (m_frp.origin)
	{
		case rpo_rule_start_state:
		case rpo_start_state_ctx:
				{
					TRuleStartStatesIterator &iter = m_rule_start_states_iter;
					iter.SeekToEnd();
				}
				break;

		case rpo_rule_call_place:
		case rpo_call_place_ctx:
				{
					TRuleCallPlacesIterator &iter = m_rule_call_places_iter;
					iter.SeekToEnd();
				}
				break;

		case rpo_non_term_defn_rule:
				{
					TNonTermDefnRulesIterator &iter = m_non_term_defn_rules_iter;
					iter.SeekToEnd();
				}
				break;
	}
}

//---------------------------------------------------------------------------
//  ================  TBasicGrammarAnalysisHelper  ===================
//---------------------------------------------------------------------------

// Name of the subdirectory for storing results of conflicts analysis.
const wchar_t *TBasicGrammarAnalysisHelper::DEF_AN_RES_SUBDIR = L"AnRes";

void TBasicGrammarAnalysisHelper::TraceMsgAttr(WORD highlight_mode, const wchar_t *message)
{
	wchar_t buffer[8196];
	swprintf(buffer, 8196, L"%s: %s", GCFAN_PREFIX, message);
	m_console.HandleTrace(buffer, highlight_mode);
}

void TBasicGrammarAnalysisHelper::TraceMsgWithDateAttr(WORD highlight_mode, const wchar_t *format, ...)
{
	// Generate first part of the message that is present all the time.
	wchar_t buffer[8196], buffer1[80];
	int free_area_len = sizeof(buffer)/sizeof(wchar_t);
	TDateTime dt_now = CurrDateTime();
	swprintf(buffer, free_area_len, L"%s: %s: ", GCFAN_PREFIX, FormatDateTime(dt_now, buffer1, 80, FALSE));

	if (format != NULL)
	{
		// Add passed information.
		int prefix_len = (int)wcslen(buffer);
		va_list vargs;
		va_start(vargs, format);
		vswprintf(buffer+prefix_len, free_area_len-prefix_len, format, vargs);
		va_end(vargs);
	}

	// Submit resulting string (maybe truncated) to console.
	m_console.HandleTrace(buffer, highlight_mode);
}

void TBasicGrammarAnalysisHelper::TraceStdDelimiterLine(WORD highlight_mode)
{
	TraceMsgWithDateAttr(highlight_mode, L"---------------------------------------------------------------------------------------");
}

void TBasicGrammarAnalysisHelper::HardAssert(bool condition, const wchar_t *msg)
{
	if (condition == FALSE)
	{
		if (msg != NULL)
		{
			TraceMsgWithDateAttr(RedAttr, msg);
		}

		TraceMsgWithDateAttr(RedAttr, L"Assertion failed.");
		wprintf(L"\r\n");

		// Crash the process.
		BYTE *p = NULL;
		*p = 0xFF;
	}
}

//-----------------------------------------------------------------------
//  =============  TGrammarConflictAnalysisHelper  =================
//-----------------------------------------------------------------------

int g_ConflictAnalysisHelper_SlotsHashingGroupType[rpo_num_types] =
{
	0,		// rpo_initial_placement
	0,		// rpo_rule_start_state
	0,		// rpo_rule_call_place
	0,		// rpo_step_over_symbol

	1,		// rpo_step_over_non_term
	1,		// rpo_non_term_defn_rule
	1,		// rpo_step_up_the_tree
	0,		// rpo_axioma_action_sym

	0,		// rpo_start_state_ctx
	0,		// rpo_call_place_ctx
	0,		// rpo_step_over_ctx
	1,		// rpo_step_up_ctx
	0,		// rpo_axioma_action_ctx
};

TGrammarConflictAnalysisHelper::TGrammarConflictAnalysisHelper(TGenericConsoleEx &console, DWORD tracing_mask)
			: TBasicGrammarAnalysisHelper(console, tracing_mask)
{
	// Verify the enum values.
	assert((DERV_PATHS_HASH_LEN & (DERV_PATHS_HASH_LEN-1)) == 0);
	assert((PATH_SLOTS_HASH_LEN & (PATH_SLOTS_HASH_LEN-1)) == 0);

	m_indexes_grammar = NULL;
	m_indexes_grammar_hash = 0;
	ResetMajorFields();
}

void TGrammarConflictAnalysisHelper::ResetMajorFields()
{
	// Reset the grammar and the current task vars.
	m_root_grammar = m_curr_grammar = NULL;
	m_igrammar = m_iconflict = m_iaction = -1;

	// Set no results and default props.
	m_result = NULL;
	m_clev = m_saved_clev = 0;
	m_clev_bottom = 0;
	m_want_backw_ctx = m_want_forw_ctx = FALSE;
	m_ctx_paths_present = FALSE;
	m_analysis_props.SetDefaultValues();
}

bool TGrammarConflictAnalysisHelper::SetGrammarAndPrepareIndexes(TGrammar *root_grammar, int igrammar)
{
	assert(root_grammar != NULL);
	assert(igrammar >= 0);

	// Setup passed root grammar into the data field of the class.
	m_root_grammar = root_grammar;
	m_curr_grammar = root_grammar->GetGrammarByIndex(igrammar);
	m_igrammar = igrammar;

	if (m_curr_grammar == NULL)
	{
		if (m_tracing_mask & gcfan_critical)
			TraceMsgWithDateAttr(DHM, L"PrepareIndexes: Index of the current grammar is bogus: %d. NumGrammars: %d.", igrammar, root_grammar->GetNumGrammars());

		m_igrammar = -1;
		return(FALSE);
	}

	if (m_curr_grammar != m_indexes_grammar || m_curr_grammar->GetBasicHashValue() != m_indexes_grammar_hash)
	{
		// Release prev indexes data if any.
		m_rule_start_states_index.Clear();
		m_non_term_defn_rules_index.Clear();
		m_firsts_and_follows.ReleaseTable();

		m_indexes_grammar = NULL;
		m_indexes_grammar_hash = 0;

		// First index.
		if (m_tracing_mask & gcfan_critical)
			TraceMsgWithDateAttr(DHM, L"PrepareIndexes: RuleStartStates.");

		if (m_curr_grammar->GenerateRuleStartStatesIterIndex(m_rule_start_states_index, BuildIndexesProgressCallbackHandler, this) == FALSE)
		{
			if (m_tracing_mask & gcfan_critical)
				TraceMsgWithDateAttr(DHM, L"PrepareIndexes: Error creating first index.");
			return(FALSE);
		}

		// Second index.
		if (m_tracing_mask & gcfan_critical)
			TraceMsgWithDateAttr(DHM, L"PrepareIndexes: NonTermDefnRules.");

		if (m_curr_grammar->GenerateNonTermDefnRulesIterIndex(m_non_term_defn_rules_index, BuildIndexesProgressCallbackHandler, this) == FALSE)
		{
			if (m_tracing_mask & gcfan_critical)
				TraceMsgWithDateAttr(DHM, L"PrepareIndexes: Error creating second index.");
			return(FALSE);
		}

		// Third index.
		if (m_tracing_mask & gcfan_critical)
			TraceMsgWithDateAttr(DHM, L"PrepareIndexes: FirstsAndFollows.");

		if (m_curr_grammar->GenerateFirstsAndFollows(m_firsts_and_follows) == FALSE)
		{
			if (m_tracing_mask & gcfan_critical)
				TraceMsgWithDateAttr(DHM, L"PrepareIndexes: Error creating third index.");
			return(FALSE);
		}

		if (m_tracing_mask & gcfan_critical)
			TraceMsgWithDateAttr(DHM, L"PrepareIndexes: Complete.");

		m_indexes_grammar = m_curr_grammar;
		m_indexes_grammar_hash = m_curr_grammar->GetBasicHashValue();
	}

	// Success.
	return(TRUE);
}

bool TGrammarConflictAnalysisHelper::ProcessConflicts(TGrammar *grammar, int igrammar, int iconflict, int iaction)
{
	bool overall_result = TRUE;
	TDateTime dt_start = CurrDateTime();
	if (m_tracing_mask & gcfan_critical)
	{
		TraceStdDelimiterLine();
		TraceMsgWithDateAttr(DHM, L"ProcessConflicts: Startup.");
	}

	// Check the passed grammar.
	HardAssert(grammar != NULL, L"The grammar to process is missing.");
	HardAssert(grammar->parent_grammar == NULL, L"The grammar to process is not the root grammar.");
	HardAssert(grammar->grm_props.processing_result == grpr_full_success, L"The grammar to process is not in the full success state.");

	// Make the processing.
	ClearStatistics();
	m_root_grammar = grammar;
	if (igrammar >= 0)
	{
		// Process conflicts of one grammar.
		if (GeneratePathsForGrammar(igrammar, iconflict, iaction) == FALSE)
			overall_result = FALSE;
	}
	else
	{
		// Iterate all grammars.
		for (int igrm=0; igrm<grammar->GetNumGrammars(); ++igrm)
		{
			if (GeneratePathsForGrammar(igrm, iconflict, iaction) == FALSE)
			{
				overall_result = FALSE;
				break;
			}
		}
	}

	// Do the final tracing and return the overall result.
	TraceFinalStatistics(dt_start, overall_result, L"ProcessConflicts");
	return(overall_result);
}

bool TGrammarConflictAnalysisHelper::ContinueCurrentProcessing()
{
	// This method is called when state of iteration is restored from the persistent repository.
	// Ensure that most important variables are set.
	assert(m_root_grammar != NULL);
	assert(m_curr_grammar != NULL);
	assert(m_curr_grammar->NumConflicts() > 0);
	assert(m_clev > 0);

	bool overall_result = TRUE;
	TDateTime dt_start = CurrDateTime();
	if (m_tracing_mask & gcfan_critical)
	{
		TraceStdDelimiterLine();
		TraceMsgWithDateAttr(DHM, L"ContinueCurrentProcessing: Startup.");
	}

	// Clear statistics fields before entering the processing functions.
	ClearStatistics();

	// Pick up the conflict and conflicting symbol.
	TGrammarConflict &cfct = m_curr_grammar->conflicts[m_iconflict];
	assert(cfct.analysis_results.NumItems() == cfct.NumActions());
	m_curr_conflicting_symbol = cfct.input_symbol;
	assert(m_curr_conflicting_symbol < IgnoreLexTypeBase);

	// Prepare the paths array.
	m_result = cfct.analysis_results.ItemPtr(m_iaction);
	PrepareHashTables();

	// Check for the pending abort.
	if (m_console.GetAbortFlag() == TRUE)
	{
		if (m_tracing_mask & gcfan_critical)
			TraceMsgWithDateAttr(DHM, L"Abort from console is already pending.");
		return(overall_result);
	}

	PrepareMajorLoop();

	if (m_clev_bottom > 0)
	{
		// Extra ctx search was interrupted. Resume this search.
		DoExtraCtxLoop();
	}

	if (m_clev_bottom == 0)
	{
		// Extra ctx search is not active. Run the primary paths search.
		DoMajorLoop();
	}

	// Do the final tracing and return the overall result.
	TraceSlotLayerSetupCounts(L"ContinueCurrentProcessing");
	TraceFinalStatistics(dt_start, overall_result, L"ContinueCurrentProcessing");
	return(overall_result);
}

bool TGrammarConflictAnalysisHelper::GeneratePathsForGrammar(int igrammar, int iconflict, int iaction)
{
	// At this point the root grammar should be already set.
	assert(m_root_grammar != NULL);
	HardAssert(igrammar >= 0 && igrammar < m_root_grammar->GetNumGrammars(), L"Index of the grammar is bogus.");

	bool overall_result = TRUE;
	TDateTime dt_start = CurrDateTime();
	if (m_tracing_mask & gcfan_critical)
	{
		TraceStdDelimiterLine();
		TraceMsgWithDateAttr(DHM, L"GenPathsForGrammar: Startup. igrammar=%d, NumGrammars=%d.", igrammar, m_root_grammar->GetNumGrammars());
	}

	// Set the current grammar.
	if (SetGrammarAndPrepareIndexes(m_root_grammar, igrammar) == FALSE)
	{
		// Success/faulire tracing is done inside the SetGrammarAndPrepareIndexes().
		m_index_creation_errors++;
		overall_result = FALSE;
	}
	else
	{
		// Do the major processing. For simplicity is is better to check for conflicts presence here and not right after
		// the strartup trace becasue SetGrammarAndPrepareIndexes() sets up the m_curr_grammar field.
		if (m_curr_grammar->NumConflicts() == 0)
		{
			// Conflicts are missing.
			if (m_tracing_mask & gcfan_critical)
				TraceMsgWithDateAttr(DHM, L"GenPathsForGrammar: Current grammar has no conflicts.");
		}
		else if (iconflict >= 0)
		{
			// Process just one conflict.
			if (GeneratePathsForConflict(iconflict, iaction) == FALSE)
				overall_result = FALSE;
		}
		else
		{
			// Iterate all grammar conflicts.
			for (iconflict=0; iconflict<m_curr_grammar->NumConflicts(); ++iconflict)
			{
				// Process all actions in the curr conflict.
				if (GeneratePathsForConflict(iconflict, -1) == FALSE)
				{
					overall_result = FALSE;
					break;
				}
			}
		}
	}

	if (m_tracing_mask & gcfan_critical)
	{
		wchar_t buffer2[80];
		TraceStdDelimiterLine();
		TraceMsgWithDateAttr(DHM, L"GenPathsForGrammar: Elapsed time: %s.", FormatDuration(CurrDateTime()-dt_start, buffer2, 80, TRUE));
	}

	return(overall_result);
}

bool TGrammarConflictAnalysisHelper::GeneratePathsForConflict(int iconflict, int iaction)
{
	// At this point current grammar should be already set and passed conflict index should be valid.
	assert(m_curr_grammar != NULL);
	assert(m_igrammar == m_curr_grammar->GetGrammarIndex());
	HardAssert(iconflict >= 0 && iconflict < m_curr_grammar->NumConflicts(), L"Index of the conflict is bogus.");

	bool overall_result = TRUE;
	TDateTime dt_start = CurrDateTime();
	if (m_tracing_mask & gcfan_critical)
	{
		TraceStdDelimiterLine();
		TraceMsgWithDateAttr(DHM, L"GenPathsForConflict: Startup. igrammar=%d, iconflict=%d, NumConflicts=%d.", m_igrammar, iconflict, m_curr_grammar->NumConflicts());
	}

	// Pick up info about the conflict.
	m_iconflict = iconflict;
	TGrammarConflict &conflict = m_curr_grammar->conflicts[iconflict];
	int num_actions = conflict.NumActions();
	assert(num_actions > 0);

	if (conflict.analysis_results.NumItems() != num_actions)
	{
		// Analysis results are missing or the number of records is bogus. Kill the old data and build the new one.
		if (conflict.BuildAnalysisResultsStub() == FALSE)
		{
			// Trace this situation.
			if (m_tracing_mask & gcfan_critical)
				TraceMsgWithDateAttr(DHM, L"GenPathsForConflict: Cannot allocate storage for the paths array.");

			m_anres_allocation_errors++;
			overall_result = FALSE;
		}
	}

	if (overall_result = TRUE)
	{
		if (iaction >= 0)
		{
			// Process just one action.
			if (GeneratePathsForAction(iaction) == FALSE)
				overall_result = FALSE;
		}
		else
		{
			// Iterate all conflicting actions.
			for (int iact2=0; iact2<num_actions; ++iact2)
			{
				if (GeneratePathsForAction(iact2) == FALSE)
				{
					overall_result = FALSE;
					break;
				}
			}
		}
	}

	if (m_tracing_mask & gcfan_critical)
	{
		wchar_t buffer2[80];
		TraceStdDelimiterLine();
		TraceMsgWithDateAttr(DHM, L"GenPathsForConflict: Elapsed time: %s.", FormatDuration(CurrDateTime()-dt_start, buffer2, 80, TRUE));
	}

	return(overall_result);
}

bool TGrammarConflictAnalysisHelper::GeneratePathsForAction(int iaction)
{
	// Pick up the conflict and check the preconditions.
	TGrammarConflict &cfct = m_curr_grammar->conflicts[m_iconflict];
	HardAssert(iaction >= 0 && iaction < cfct.NumActions(), L"Index of the conflicting action is bogus.");
	HardAssert(cfct.analysis_results.NumItems() == cfct.NumActions(), L"Number of conflict analysis records is not idetical to the number of conflicting actions.");

	// Initial tracing.
	TDateTime dt_start = CurrDateTime();
	if (m_tracing_mask & gcfan_critical)
	{
		TraceStdDelimiterLine();
		TraceMsgWithDateAttr(DHM, L"GenPathsForAction: Startup. igrammar=%d, iconflict=%d, iaction=%d, NumActions=%d.", m_igrammar, m_iconflict, iaction, cfct.NumActions());
	}

	m_iaction = iaction;
	m_curr_conflicting_symbol = cfct.input_symbol;
	assert(m_curr_conflicting_symbol < IgnoreLexTypeBase);

	// Prepare the paths array.
	m_result = cfct.analysis_results.ItemPtr(iaction);
	m_result->Clear();

	// Prepare the slots stack for determining the longest transient path.
	for (int i1=0; i1<MAX_DERIV_PATH_LEN; ++i1)
	{
		m_curr_path[i1].m_frp.origin = (TFullRulePosOrigin)0xFF;
		m_curr_path[i1].m_setup_cnts_count = 0;
	}

	m_clev = 0;
	m_clev_bottom = 0;
	PrepareHashTables();

	// Init statistics variables.
	m_cnt_steps = 0;
	m_iter_setup_cnt = 0;

	// Check for pending abort.
	if (m_console.GetAbortFlag() == TRUE)
	{
		if (m_tracing_mask & gcfan_critical)
			TraceMsgWithDateAttr(DHM, L"GenPathsForAction: Abort from console is already pending.");

		m_result->m_generation_result = TConflictingActionAnalysisResults::gres_aborted_by_user;
		m_cnt_actions_processed++;
		m_action_processing_results[m_result->m_generation_result]++;
		return(FALSE);
	}

	// Pick up the conflicting action.
	WORD conflicting_action = cfct.conflicting_actions[iaction];
	assert(conflicting_action < actb_conflict);

	//
	//	Fill in the initial slots to start the search.
	// ------------------------------------------
	//
	if (conflicting_action < actb_reduce)
	{
		// This is shift action. It is necessary find all places where the conflicting state stays in front of the conflicting symbol.
		// Such iteration is achieved using the rule call places iterator, although no rule is called in this case.
		m_curr_path[0].PrepareSlot(++m_iter_setup_cnt, rpo_rule_call_place, 0xFFFF, 0xFFFF, cfct.parser_state, 0xFFFF);
		m_curr_path[0].m_rule_call_places_iter.PrepareIteration(m_curr_grammar, cfct.parser_state, cfct.input_symbol, 0xFFFF);
		m_clev = 1;
	}
	else
	{
		// This is reduce action. Put initial position at the end of the conflicting rule.
		WORD irule = conflicting_action-actb_reduce;
		int rule_len = m_curr_grammar->rules[irule].symbols.NumItems();
		m_curr_path[0].PrepareSlot(++m_iter_setup_cnt, rpo_initial_placement, irule, rule_len, cfct.parser_state, m_curr_conflicting_symbol);
		m_clev = 1;

		if (m_curr_grammar->rules[irule].non_term != m_curr_grammar->axioma_ident)
		{
			// This is regular case.
			if (rule_len > 0)
			{
				// The rule is not empty. Iterate all starting states of this rule.
				AddToSlotsHashTable(m_clev-1);
				m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_rule_start_state, irule, 0, 0xFFFF, 0xFFFF);
				m_curr_path[m_clev].m_rule_start_states_iter.PrepareIteration(m_curr_grammar, m_rule_start_states_index, irule, rule_len, cfct.parser_state, m_curr_conflicting_symbol);
				m_clev++;
			}
			else
			{
				// The rule is empty. This means that there is only one relevant starting state. And this state is the same
				// to the final state. There is no need for iterating the rule start states. Look for places where this rule is called.
				AddToSlotsHashTable(m_clev-1);
				m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_rule_call_place, 0xFFFF, 0xFFFF, cfct.parser_state, 0xFFFF);
				m_curr_path[m_clev].m_rule_call_places_iter.PrepareIteration(m_curr_grammar, cfct.parser_state, m_curr_grammar->rules[irule].non_term, 0xFFFF);
				m_clev++;
			}
		}
		else
		{
			// Inital placement has put the current position at the end of the rule that defines axioma. Axioma cannot
			// be called anywhere. This is possible only if the conflicting symbol is one of the axioma followers. This can
			// happen only in the nested grammars. Look for this position.
			TParsingStatePositionsIterator iter;
			bool pos_present = FALSE;
			for (iter.Setup(m_curr_grammar->parsing_states[cfct.parser_state]); iter; ++iter)
			{
				TRulePosition &pos = iter.CurrPos();
				if (pos.irule == irule && pos.isym == rule_len && pos.action_sym == m_curr_conflicting_symbol)
				{
					pos_present = TRUE;
					break;
				}
			}

			// Ensure that this position is really existing.
			HardAssert(pos_present == TRUE, L"Conflict in the rule that defines axioma does not have conflicting symbol as action symbol in any of the final positions of this rule.");

			// Add axioma follower path slot and save this path.
			AddToSlotsHashTable(m_clev-1);
			m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_axioma_action_sym, 0xFFFF, 0xFFFF, cfct.parser_state, m_curr_conflicting_symbol);
			m_clev++;

			if (InstantiateDerivationPath() == FALSE)
				return(FALSE);
		}
	}

	//
	//	Do the job.
	// ---------------------
	//
	PrepareMajorLoop();
	DoMajorLoop();

	// Report results that were collected so far.
	if (m_tracing_mask & gcfan_critical)
	{
		TraceSlotLayerSetupCounts(L"GenPathsForAction");

		TraceStdDelimiterLine();
		wchar_t buffer2[80], buffer3[80];
		TraceMsgWithDateAttr(DHM, L"GenPathsForAction: Result: %s.", TConflictingActionAnalysisResults::GetGenResultDescription(m_result->m_generation_result, FALSE));
		TraceMsgWithDateAttr(DHM, L"GenPathsForAction: %d unique path%s.", m_result->m_derivation_paths.NumItems(), PluralSuffix(m_result->m_derivation_paths.NumItems()));
		TraceMsgWithDateAttr(DHM, L"GenPathsForAction: TOTAL %s iteration steps.", FormatInt64(m_cnt_steps, buffer3, 80, fnms_dec_signed, L'_'));

		if (m_result->m_too_deep_truncations > 0)
			TraceMsgWithDateAttr(DHM, L"GenPathsForAction: %s too deep truncations.", FormatInt64(m_result->m_too_deep_truncations, buffer2, 80, fnms_dec_signed, L'_'));

		if (m_result->m_partial_iteration_aborts > 0)
			TraceMsgWithDateAttr(DHM, L"GenPathsForAction: %s patial iteration aborts.", FormatInt64(m_result->m_partial_iteration_aborts, buffer2, 80, fnms_dec_signed, L'_'));

		TraceMsgWithDateAttr(DHM, L"GenPathsForAction: Elapsed time: %s (igrammar=%d, iconflict=%d, iaction=%d).", FormatDuration(CurrDateTime()-dt_start, buffer2, 80, TRUE), m_igrammar, m_iconflict, m_iaction);
	}

	// Processing action sucseeds if there is no pending abort.
	return((m_console.GetAbortFlag() == TRUE) ? FALSE : TRUE);
}

void TGrammarConflictAnalysisHelper::PrepareHashTables()
{
	// Clear the paths hash table.
	m_next_same_hash_path.Clear();
	for (int ix1=0; ix1<DERV_PATHS_HASH_LEN; ++ix1)
		m_paths_hash_table[ix1] = -1;

	// Add paths to the paths hash table if any.
	for (int ix2=0; ix2<m_result->m_derivation_paths.NumItems(); ++ix2)
	{
		TGrammarDerivationPath &path = m_result->m_derivation_paths[ix2];
		TFullRulePosition &mid_frp = path.derv_steps[path.derv_path_len/4];
		int path_hash = (mid_frp.irule ^ mid_frp.isym ^ path.derv_path_len) & (DERV_PATHS_HASH_LEN-1);
		m_next_same_hash_path.AppendItem(m_paths_hash_table[path_hash]);
		m_paths_hash_table[path_hash] = ix2;
	}

	// Clear the slots stack.
	for (int ix3=0; ix3<PATH_SLOTS_HASH_AREAS; ++ix3)
	{
		TPathSlotsHashingArea &area = m_slots_hashing_areas[ix3];
		area.m_bottom_slot_index = -1;

		for (int ix3=0; ix3<PATH_SLOTS_HASH_LEN; ++ix3)
			area.m_slots_hash_table[ix3] = -1;
	}

	// Set the stack to the state like the area[0] is entered.
	m_slots_hashing_areas[0].m_bottom_slot_index = 0;
	m_slots_hashing_areas_index = 0;

	// Add slots to the hash if any except for the last one.
	for (int ix5=0; ix5<m_clev-1; ++ix5)
		AddToSlotsHashTable(ix5);
}

void TGrammarConflictAnalysisHelper::PrepareMajorLoop()
{
	// Normalize the max path length.
	if (m_analysis_props.m_max_path_length == 0)
		m_analysis_props.m_max_path_length = MAX_DERIV_PATH_LEN;
	m_analysis_props.m_max_path_length = ___min(MAX_DERIV_PATH_LEN, ___max(6, m_analysis_props.m_max_path_length));

	// Prepare fields that control frequency of emitting console messages about the progress.
	m_progress_report_cnt = 1;
	m_deep_nest_report_cnt = 0;

	// Pick up the timeout value from the props.
	int act_proc_duration_in_seconds = m_analysis_props.m_action_proc_timeout;
	if (act_proc_duration_in_seconds <= 0)
	{
		// Change passed value into the number of seconds in the year.
		act_proc_duration_in_seconds = 365*24*3600;
	}

	// Setup the conflicting action processing timeout.
	m_timeout_date_time = CurrDateTime() + ___min(ONE_YEAR, ___max(ONE_SECOND, ONE_SECOND*act_proc_duration_in_seconds));

}

void TGrammarConflictAnalysisHelper::DoMajorLoop()
{
	assert(m_result != NULL);
	assert(m_clev > 0);

	TDateTime dt_start = CurrDateTime();
	__int64 earlier_truncations = m_result->m_too_deep_truncations;
	__int64 earlier_iter_aborts = m_result->m_partial_iteration_aborts;

	// Major loop.
	m_saved_clev = 0;
	while (m_clev > 0)
	{
		// Do the iteration step.
		TBuildGrammarDerivationSlot &slot = m_curr_path[m_clev-1];
		if (slot.GetNextPosition() == TRUE)
		{
			// Position is available. Process it.
			m_cnt_steps++;
			slot.m_positions_cnt++;

			// Do tracing for the current position.
#ifdef _DEBUG
			if (m_tracing_mask & gcfan_verbose)
			{
				// Do tracing on each step.
				TraceOverallProgress(FALSE);
			}
			else
#endif
			if (m_tracing_mask & gcfan_progress)
			{
				// Do tracing only once a while.
				m_progress_report_cnt--;
				if (m_progress_report_cnt <= 0)
				{
					m_progress_report_cnt = PROGRESS_REPORT_FREQ;
					TraceOverallProgress(TRUE);
				}
			}

			// Derivation path should consist of only different slots. Otherwise the stack will grow infinitely.
			int islot = LookForIdenticalSlot();
			if (islot >= 0)
			{
				// This slot is already present in the path. Its processing should be skipped otherwise the whole
				// processing will enter the infinite recursion.
#ifdef _DEBUG
				if (m_tracing_mask & gcfan_verbose)
				{
					wchar_t buffer[128];
					swprintf(buffer, 128, L"GenPathsForAction: Slot duplication with slot %d.", islot);
					TraceMsgAttr(DHM, buffer);
				}
#endif
				continue;
			}

			// Execute the slot type specific processing.
			switch (slot.m_frp.origin)
			{
				case rpo_rule_start_state:
						{
							if (m_clev < m_analysis_props.m_max_path_length)
							{
								// Look for rules that use this starting position. Note that all action symbols should be iterated.
								AddToSlotsHashTable(m_clev-1);
								m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_rule_call_place, 0xFFFF, 0xFFFF, slot.m_frp.istate, 0xFFFF);
								m_curr_path[m_clev].m_rule_call_places_iter.PrepareIteration(m_curr_grammar, slot.m_frp.istate, m_curr_grammar->rules[slot.m_frp.irule].non_term, 0xFFFF);
								m_clev++;
#ifdef _DEBUG
								if (m_tracing_mask & gcfan_verbose)
									TraceMsgAttr(DHM, L"GenPathsForAction: Start rule call places iteration.");
#endif
							}
							else
							{
								ReportTooDeepDerivationPath();
							}
						}
						break;

				case rpo_rule_call_place:
						{
							// Initial placement for conflicting action "shift" puts RuleCallPlace iter into the first slot with conflicting symbol
							// instead of the non terminal. Check for this special case.
							WORD rule_sym = m_curr_grammar->rules[slot.m_frp.irule].symbols[slot.m_frp.isym];
							if (rule_sym == m_curr_conflicting_symbol)
							{
								// Conflicting place for the shift action is found.
								if (InstantiateDerivationPath() == FALSE)
								{
									// Instantiation failed. Abort the iteration.
									m_clev = 0;
								}
							}
							else
							{
								if (m_clev < m_analysis_props.m_max_path_length)
								{
									// Fill the next slot with the "step over the symbol" layer and make generic processing for new location.
									WORD next_state = m_curr_grammar->GetNextParsingState(slot.m_frp.istate, rule_sym);
									AddToSlotsHashTable(m_clev-1);
									m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_step_over_symbol, slot.m_frp.irule, slot.m_frp.isym+1, next_state, slot.m_frp.action_sym);
									m_clev++;
#ifdef _DEBUG
									if (m_tracing_mask & gcfan_verbose)
										TraceMsgAttr(DHM, L"GenPathsForAction: Add step over the symbol slot.");
#endif
									if (ProcessDerivationStep() == FALSE)
									{
										// Processing failed. Abort iteration.
										m_clev = 0;
									}
								}
								else
								{
									ReportTooDeepDerivationPath();
								}
							}
						}
						break;

				case rpo_non_term_defn_rule:
						{
							if (m_analysis_props.m_use_all_non_term_defn_rules == FALSE)
							{
								// This is the most often used value of the option. Check type of the current rule.
								TGrammarRule &rule = m_curr_grammar->rules[slot.m_frp.irule];
								if (rule.symbols.NumItems() > 0 && rule.symbols[0] == rule.non_term)
								{
									// Current rule contains the rule non terminal right at the beginning of the rule.
									WORD *pff = m_firsts_and_follows.GetNonTermRowPtr(rule.symbols[0]);
									if ((pff[0] & FUN_FIRST_MASK) == 0)
									{
										// This non terminal cannot be created out of nothing. Ignore this rule because it cannot help
										// finding paths that will be different from paths that were/will be found using other rules for
										// this non terminal.
										break;
									}

									// This non terminal can be created starting from the empty symbol.
									if (m_clev < m_analysis_props.m_max_path_length)
									{
										// Step over this non terminal like an empty symbol.
										WORD next_state = m_curr_grammar->GetNextParsingState(slot.m_frp.istate, rule.symbols[0]);
										AddToSlotsHashTable(m_clev-1);
										m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_step_over_non_term, slot.m_frp.irule, 1, next_state, slot.m_frp.action_sym);
										m_curr_path[m_clev].m_upper_layer = slot.m_upper_layer;
										m_clev++;
									}
									else
									{
										ReportTooDeepDerivationPath();
										break;
									}
								}
							}

							if (ProcessDerivationStep() == FALSE)
							{
								// Processing failed. Abort iteration.
								m_clev = 0;
							}
						}
						break;
			}

			// Check for abort cases on only some of the steps.
			if (((WORD)m_cnt_steps) == 0 && m_saved_clev == 0)
			{
				if (CheckForAbortCases() == TRUE)
				{
					continue;
				}
			}

		}
		else
		{
			// Iteration on this level is complete or the current slot is non iterateable.
			if (slot.m_positions_cnt == 0)
			{
				// Iterator has not found any positions.
				if (slot.m_frp.origin == rpo_rule_start_state || slot.m_frp.origin == rpo_rule_call_place || slot.m_frp.origin == rpo_non_term_defn_rule)
				{
					// This type of slot is iterateable and it should find something.
					if (m_tracing_mask & gcfan_critical)
					{
						TraceMsgWithDateAttr(DHM, L"GenPathsForAction: No positions for the inner slot were noticed.");
						TraceMsgWithDateAttr(DHM, L"GenPathsForAction: Dumping FULL derivation stack, not just its tiny top part.");
						TraceCurrentState();
					}

					// These situations should never happen.
					if (slot.m_frp.origin == rpo_rule_call_place)
						HardAssert(slot.m_rule_call_places_iter.GetNonTerm() != m_curr_grammar->axioma_ident, L"Iteration is looking for the call places of axioma while it should not do this.");
					if (m_result->m_too_deep_truncations <= 0 && m_result->m_partial_iteration_aborts <= 0)
						HardAssert(FALSE, L"Iteration has not found positions for the iterateable slot type.");
				}
			}

			// Update the stack the of slots.
			m_clev--;
			RemoveFromSlotsHashTable(m_clev-1);
		}
	}

	if (m_saved_clev > 0)
	{
		// The loop was exited only temorarily. Restore the m_clev.
		m_clev = m_saved_clev;
	}

	if (m_clev == 0)
	{
		// Iteration finished either because everyting was iterated or because iteration has permanently failed.
		TDerivationPathsArraySimplificationHelper::RemovePathsWithRedunantSteps(m_result);

		if (m_result->m_generation_result < TConflictingActionAnalysisResults::gres_no_paths_found)
		{
			// No errors happened. Check for nasty "no paths" case.
			if (m_result->m_derivation_paths.NumItems() > 0)
			{
				// Full success.
				m_result->m_generation_result = TConflictingActionAnalysisResults::gres_ok;
			}
			else
			{
				// Derivation paths are missing.
				if (m_tracing_mask & gcfan_critical)
					TraceMsgWithDateAttr(DHM, L"GenPathsForAction: No derivation paths found.");

				m_result->m_generation_result = TConflictingActionAnalysisResults::gres_no_paths_found;
			}
		}
	}

	__int64 delta_truncations = m_result->m_too_deep_truncations-earlier_truncations;
	__int64 delta_iter_aborts = m_result->m_partial_iteration_aborts-earlier_iter_aborts;

	// Fill in the missing fields in the paths array structure.
	m_result->m_generation_steps = m_cnt_steps;
	m_result->m_generation_time += CurrDateTime()-dt_start;

	// Update statistics.
	m_cnt_actions_processed++;
	m_action_processing_results[m_result->m_generation_result]++;
	m_num_too_deep_truncations += delta_truncations;
	m_num_partial_iter_aborts += delta_iter_aborts;
}

void TGrammarConflictAnalysisHelper::DoExtraCtxLoop()
{
	// Continue the interrupted extra ctx search.
	assert(m_result != NULL);
	assert(m_clev > 0);
	assert(m_clev_bottom > 0);
	assert(m_clev > m_clev_bottom);

	TDateTime dt_start = CurrDateTime();
	__int64 earlier_truncations = m_result->m_too_deep_truncations;
	__int64 earlier_iter_aborts = m_result->m_partial_iteration_aborts;

	//
	//	The worker function.
	// -------------------------
	//
	m_saved_clev = 0;
	SearchForMoreContext();

	if (m_saved_clev > 0)
	{
		// The loop was exited only temorarily. Restore the m_clev.
		m_clev = m_saved_clev;
	}

	__int64 delta_truncations = m_result->m_too_deep_truncations-earlier_truncations;
	__int64 delta_iter_aborts = m_result->m_partial_iteration_aborts-earlier_iter_aborts;

	// Fill in the missing fields in the paths array structure.
	m_result->m_generation_steps = m_cnt_steps;
	m_result->m_generation_time += CurrDateTime()-dt_start;

	// Update statistics. Note that only some of the fields are updated.
	m_num_too_deep_truncations += delta_truncations;
	m_num_partial_iter_aborts += delta_iter_aborts;
}

bool TGrammarConflictAnalysisHelper::ProcessDerivationStep()
{
	// Ensure that current slot type is either the non_term_defn or the step_over_symbol or step_over_non_term.
	TFullRulePosition cfrp = m_curr_path[m_clev-1].m_frp;
	assert(cfrp.origin == rpo_step_over_symbol || cfrp.origin == rpo_step_over_non_term || cfrp.origin == rpo_non_term_defn_rule);
	int clev_possible_step_over = m_clev-1;

	// Pick up info from the curr slot and its corresponding rule.
	TGrammarRule *rule = m_curr_grammar->rules.ItemPtr(cfrp.irule);
	int rule_len = rule->symbols.NumItems();

	// Check for the special case first.
	if (cfrp.isym == rule_len)
	{
		// Current position stays at the end of the rule. If position is inside the non term defns tree, it is necessary
		// to go up the tree till some symbol will be found or till the top of the tree. If not, there is no need to do anything.
		int org_layer = m_curr_path[m_clev-1].m_upper_layer;
		while (cfrp.isym == rule_len && org_layer >= 0)
		{
			// Check if slots array still has free space or not.
			if (m_clev >= m_analysis_props.m_max_path_length)
			{
				ReportTooDeepDerivationPath();
				return(TRUE);
			}

			// Change the current position variables.
			cfrp = m_curr_path[org_layer].m_frp;
			org_layer = m_curr_path[org_layer].m_upper_layer;

			// Repick the info about the new current rule.
			rule = m_curr_grammar->rules.ItemPtr(cfrp.irule);
			rule_len = rule->symbols.NumItems();

			// Note that cfrp is a real structure, not a reference. This means that it is ok to modify its fields.
			assert(cfrp.isym < rule_len);
			cfrp.istate = m_curr_grammar->GetNextParsingState(cfrp.istate, rule->symbols[cfrp.isym]);
			cfrp.isym++;

			// Fill in the next slot with the step up the tree symbol record.
			AddToSlotsHashTable(m_clev-1);
			m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_step_up_the_tree, cfrp.irule, cfrp.isym, cfrp.istate, cfrp.action_sym);
			m_curr_path[m_clev].m_upper_layer = org_layer;
			m_clev++;

#ifdef _DEBUG
			if (m_tracing_mask & gcfan_verbose)
				TraceMsgAttr(DHM, L"ProcDervStep: Add step up the tree slot.");
#endif
		}
	}

	// Current rule should be non empty.
	assert(rule_len > 0);
	if (cfrp.isym >= rule_len)
	{
		// Current position stays at the end of the rule.
		if (m_clev < m_analysis_props.m_max_path_length)
		{
			if (rule->non_term == m_curr_grammar->axioma_ident)
			{
				// Design of this parser does not allow using axioma in the right sides of the rules. This means that it does not make
				// sense to iterate places where current non terminal might be called. Check the action symbol of the current position.
				// In the nested grammar it can be exactly what the process is looking for.
				if (cfrp.action_sym == m_curr_conflicting_symbol)
				{
					AddToSlotsHashTable(m_clev-1);
					m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_axioma_action_sym, 0xFFFF, 0xFFFF, cfrp.istate, cfrp.action_sym);
					m_clev++;

					if (InstantiateDerivationPath() == FALSE)
						return(FALSE);
				}
			}
			else
			{
				// Look for the starting states in this rule.
				AddToSlotsHashTable(m_clev-1);
				m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_rule_start_state, cfrp.irule, 0, 0xFFFF, 0xFFFF);

				// Look for the nearest rule call place up the path to get restriction on the rule start states.
				int ilev = clev_possible_step_over-1;
				while (ilev >= 0)
				{
					if (m_curr_path[ilev].m_frp.origin == rpo_rule_call_place)
						break;
					ilev--;
				}

				// Ensure that the rule call place is present.
				HardAssert(ilev >= 0, L"The rule call place slot is missing.");

				// Ensure that discovered rule call place is located in the right rule.
				TFullRulePosition &rcp_frp = m_curr_path[ilev].m_frp;
				HardAssert(rcp_frp.irule == cfrp.irule, L"The nearest rule call place points to some other rule.");
				HardAssert(rcp_frp.isym < cfrp.isym, L"The nearest rule call place points to bogus rule symbol.");

				// Prepare rule start states iteration withit restriction.
				m_curr_path[m_clev].m_rule_start_states_iter.PrepareIteration(m_curr_grammar, m_rule_start_states_index, cfrp.irule, rcp_frp.isym, rcp_frp.istate, cfrp.action_sym);
				m_clev++;

#ifdef _DEBUG
				if (m_tracing_mask & gcfan_verbose)
					TraceMsgAttr(DHM, L"ProcDervStep: Start rule start states iteration.");
#endif
			}
		}
		else
		{
			ReportTooDeepDerivationPath();
		}
	}
	else
	{
		// Position stays somewhere in the middle of the rule. Pick up the symbol from the rule.
		WORD curr_sym = rule->symbols[cfrp.isym];
		if (curr_sym < NonTerminalsBase)
		{
			// Rule contains terminal symbol.
			if (curr_sym == m_curr_conflicting_symbol)
			{
				// This is exactly what the whole procedure was looking for. Instantiate the current path into the results set.
				if (InstantiateDerivationPath() == FALSE)
					return(FALSE);
			}
#ifdef _DEBUG
			else
			{
				// This is some other terminal symbol.
				if (m_tracing_mask & gcfan_verbose)
					TraceMsgAttr(DHM, L"ProcDervStep: Non conflicting terminal symbol.");
			}
#endif
		}
		else
		{
			// Rule contains non terminal in its current position.
			WORD *pff = m_firsts_and_follows.GetNonTermRowPtr(curr_sym);
			if ((pff[0] & FUN_FIRST_MASK) == 0 && (pff[m_curr_conflicting_symbol] & FUN_FIRST_MASK) == 0)
			{
				// This non terminal cannot be created from nothing and it cannot start from the conflicting symbol.
				// There is no point in iterating possible expansions of this non terminal.
#ifdef _DEBUG
				if (m_tracing_mask & gcfan_verbose)
					TraceMsgAttr(DHM, L"ProcDervStep: Unacceptable non terminal symbol.");
#endif
			}
			else if (m_clev < m_analysis_props.m_max_path_length)
			{
				// Look for rules that define this non terminal.
				AddToSlotsHashTable(m_clev-1);
				m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_non_term_defn_rule, 0xFFFF, 0, cfrp.istate, 0xFFFF);
				m_curr_path[m_clev].m_upper_layer = m_clev-1;
				m_curr_path[m_clev].m_non_term_defn_rules_iter.PrepareIteration(m_curr_grammar, m_non_term_defn_rules_index, cfrp.istate, curr_sym, TRUE);
				m_clev++;
#ifdef _DEBUG
				if (m_tracing_mask & gcfan_verbose)
					TraceMsgAttr(DHM, L"ProcDervStep: Start non term defn rules iteration.");
#endif
			}
			else
			{
				ReportTooDeepDerivationPath();
			}
		}
	}

	// Processing succeeded.
	return(TRUE);
}

bool TGrammarConflictAnalysisHelper::InstantiateDerivationPath()
{
	assert(m_clev > 0);

	if (m_analysis_props.m_search_for_non_empty_ctx == FALSE)
	{
		// Props are asking to save the path without looking for extra ctx.
		return(SaveDerivationPath(FALSE));
	}

	if (m_curr_path[m_clev-1].m_frp.origin == rpo_axioma_action_sym)
	{
		// Looking for more context is is not possible. Save existing path in its current state.
		return(SaveDerivationPath(FALSE));
	}

	// Check if current path has solid predecessors and followers, i.e. if symbols that stay before
	// and/or after the conflicting symbol and can be resolved into the empty symbol or not.
	bool solid_predecessors = FALSE;
	bool solid_followers = FALSE;

	int org_layer = m_clev-1;
	int latest_layer = -1;
	while (org_layer >= 0)
	{
		if (solid_followers == FALSE)
		{
			// Check if trailing part of the curr rule is solid or not.
			TFullRulePosition &cfrp = m_curr_path[org_layer].m_frp;
			if (m_curr_grammar->IsRulePartSolid(cfrp.irule, cfrp.isym+1) == TRUE)
				solid_followers = TRUE;
		}

		// Step to the next layer if any.
		latest_layer = org_layer;
		org_layer = m_curr_path[org_layer].m_upper_layer;
	}

	// Note that the loop below will be executed at least once, because latest_layer var will get
	// non negative value at least during the first rotaion of the loop above.
	while (latest_layer >= 0 && solid_predecessors == FALSE)
	{
		// Pick up the info about the layer.
		TFullRulePosition &cfrp = m_curr_path[latest_layer].m_frp;

		if (cfrp.origin == rpo_step_up_the_tree)
		{
			// Skip the whole empty subtree.
			assert(latest_layer > 0);
			latest_layer = m_curr_path[latest_layer-1].m_upper_layer;
			assert(latest_layer >= 0);
		}
		else
		{
			if (cfrp.origin == rpo_initial_placement || cfrp.origin == rpo_rule_call_place)
			{
				// These types of steps may have solid symbols at the beginning of their rules.
				if (m_curr_grammar->IsRulePartSolid(cfrp.irule, 0, cfrp.isym) == TRUE)
				{
					solid_predecessors = TRUE;
					break;
				}
			}

			// Step up to the next layer.
			latest_layer--;
		}
	}

	if (solid_predecessors == TRUE && solid_followers == TRUE)
	{
		// Both contexts are present. It is ok to save the passed path in its current state.
		return(SaveDerivationPath(FALSE));
	}

	// Save value of the stack marker before adding any ctx related slots.
	m_clev_bottom = m_clev;

	// Add skip over frames for all already available followers. This loop will work at least once
	// to step over the conflicting symbol.
	org_layer = m_clev-1;
	while (org_layer >= 0)
	{
		if (m_clev < m_analysis_props.m_max_path_length)
		{
			// Fill in the slot with step over the symbols or with step up the tree record.
			AddStepOverCtxOrUpCtxSlot(org_layer);
			org_layer = m_curr_path[org_layer].m_upper_layer;
			m_curr_path[m_clev-1].m_upper_layer = org_layer;
		}
		else
		{
			ReportTooDeepDerivationPath();
			return(TRUE);
		}

		// A bit tricky check now. Check the latest slot in the stack that was created just a few lines above.
		TFullRulePosition &cfrp = m_curr_path[m_clev-1].m_frp;
		if (cfrp.isym < m_curr_grammar->rules[cfrp.irule].Length())
		{
			// One more step over is needed.
			if (m_clev < m_analysis_props.m_max_path_length)
			{
				AddStepOverCtxOrUpCtxSlot(m_clev-1);
				m_curr_path[m_clev-1].m_upper_layer = org_layer;
			}
			else
			{
				ReportTooDeepDerivationPath();
				return(TRUE);
			}
		}
	}

	// Initiate looking for more context.
	if (m_clev < m_analysis_props.m_max_path_length)
	{
		TFullRulePosition &cfrp = m_curr_path[m_clev-1].m_frp;
		if (m_curr_grammar->rules[cfrp.irule].non_term == m_curr_grammar->axioma_ident)
		{
			// Further search is not possible.
			if (cfrp.action_sym != TLexToSymConverter::lt_eof && solid_followers == FALSE)
			{
				// Axioma action symbol belongs to the grammar symbols. Adding this symbol may make the path context not empty.
				AddToSlotsHashTable(m_clev-1);
				m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_axioma_action_ctx, 0xFFFF, 0xFFFF, cfrp.istate, cfrp.action_sym);
				m_clev++;
			}

			return(SaveDerivationPath(TRUE));
		}
		else
		{
			// Iterate starting states of the current rule.
			AddStartStateCtxSlot();
		}
	}
	else
	{
		ReportTooDeepDerivationPath();
		return(TRUE);
	}

	// Look for more context if possible.
	m_want_backw_ctx = !solid_predecessors;
	m_want_forw_ctx = !solid_followers;
	m_ctx_paths_present = FALSE;
	return(SearchForMoreContext());
}

void TGrammarConflictAnalysisHelper::AddStepOverCtxOrUpCtxSlot(int ilayer)
{
	TFullRulePosition &cfrp = m_curr_path[ilayer].m_frp;
	TSymbolsArray &rule_syms = m_curr_grammar->rules[cfrp.irule].symbols;
	WORD isym = cfrp.isym;
	WORD istate = cfrp.istate;

	TFullRulePosOrigin origin;
	if (ilayer == m_clev-1)
	{
		// Step over the ctx action. Shift current position to the end of the rule.
		origin = rpo_step_over_ctx;
		while (isym < rule_syms.NumItems())
			istate = m_curr_grammar->GetNextParsingState(istate, rule_syms[isym++]);
	}
	else
	{
		origin = rpo_step_up_ctx;
		// Step up the tree. Shift current position over one symbol only.
		istate = m_curr_grammar->GetNextParsingState(istate, rule_syms[isym++]);
	}

	AddToSlotsHashTable(m_clev-1);
	m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, origin, cfrp.irule, isym, istate, cfrp.action_sym);
	m_clev++;

#ifdef _DEBUG
	if (m_tracing_mask & gcfan_verbose)
		TraceMsgAttr(DHM, L"MoreCtxSerch: Add step over ctx slot.");
#endif
}

void TGrammarConflictAnalysisHelper::AddStartStateCtxSlot()
{
	TFullRulePosition &cfrp = m_curr_path[m_clev-1].m_frp;
	AddToSlotsHashTable(m_clev-1);
	m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_start_state_ctx, cfrp.irule, 0, 0xFFFF, 0xFFFF);
	m_curr_path[m_clev].m_rule_start_states_iter.PrepareIteration(m_curr_grammar, m_rule_start_states_index, cfrp.irule, cfrp.isym, cfrp.istate, cfrp.action_sym);
	m_clev++;

#ifdef _DEBUG
	if (m_tracing_mask & gcfan_verbose)
		TraceMsgAttr(DHM, L"MoreCtxSerch: Start rule start states iteration.");
#endif
}

bool TGrammarConflictAnalysisHelper::SearchForMoreContext()
{
	// Major loop.
	while (m_clev > m_clev_bottom)
	{
		TBuildGrammarDerivationSlot &slot = m_curr_path[m_clev-1];
		if (slot.GetNextPosition() == TRUE)
		{
			// Position is available. Process it.
			m_cnt_steps++;

			// Do tracing for the current position.
#ifdef _DEBUG
			if (m_tracing_mask & gcfan_verbose)
			{
				// Do tracing on each step.
				TraceOverallProgress(FALSE);
			}
			else
#endif
			if (m_tracing_mask & gcfan_progress)
			{
				// Do tracing only once a while.
				m_progress_report_cnt--;
				if (m_progress_report_cnt <= 0)
				{
					m_progress_report_cnt = PROGRESS_REPORT_FREQ;
					TraceOverallProgress(TRUE);
				}
			}

			// Look for identical slots.
			int islot = LookForIdenticalSlot();
			if (islot >= 0)
			{
#ifdef _DEBUG
				if (m_tracing_mask & gcfan_verbose)
				{
					wchar_t buffer[128];
					swprintf(buffer, 128, L"SearchMoreCtx: Slot duplication with slot %d.", islot);
					TraceMsgAttr(DHM, buffer);
				}
#endif
				continue;
			}

			// Execute the slot type specific processing.
			switch (slot.m_frp.origin)
			{
				case rpo_start_state_ctx:
						{
							if (m_clev < m_analysis_props.m_max_path_length)
							{
								// Look for rules that use this starting position. Note that all action symbols will be iterated.
								AddToSlotsHashTable(m_clev-1);
								m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_call_place_ctx, 0xFFFF, 0xFFFF, slot.m_frp.istate, 0xFFFF);
								m_curr_path[m_clev].m_rule_call_places_iter.PrepareIteration(m_curr_grammar, slot.m_frp.istate, m_curr_grammar->rules[slot.m_frp.irule].non_term, 0xFFFF);
								m_clev++;
#ifdef _DEBUG
								if (m_tracing_mask & gcfan_verbose)
									TraceMsgAttr(DHM, L"SearchMoreCtx: Start rule call places iteration.");
#endif
							}
							else
							{
								ReportTooDeepDerivationPath();
								m_ctx_paths_present = TRUE;
							}
						}
						break;

				case rpo_call_place_ctx:
						{
							// Check what current position changes.
							WORD irule = slot.m_frp.irule;
#ifdef WANT_EXTRA_DEEP_CTX
							bool want_more_backw_ctx = TRUE;
							bool want_more_forw_ctx = TRUE;
#else
							bool want_more_backw_ctx = m_want_backw_ctx;
							if (m_want_backw_ctx == TRUE)
							{
								if (m_curr_grammar->IsRulePartSolid(irule, 0, slot.m_frp.isym) == TRUE)
									want_more_backw_ctx = FALSE;
							}

							bool want_more_forw_ctx = m_want_forw_ctx;
							if (m_want_forw_ctx == TRUE)
							{
								if (m_curr_grammar->IsRulePartSolid(irule, slot.m_frp.isym+1) == TRUE)
									want_more_forw_ctx = FALSE;
							}

							// Process position taking into account changes in the predecessors/followers props that were done above.
							if (want_more_backw_ctx == FALSE && want_more_forw_ctx == FALSE)
							{
								// Context with solid symbols on both sides is found.
								if (SaveDerivationPath(TRUE) == FALSE)
									return(FALSE);

								m_ctx_paths_present = TRUE;
							}
							else
#endif
							{
								// It is necessary to look for more context.
								if (m_clev < m_analysis_props.m_max_path_length-1)
								{
									if (m_curr_grammar->rules[irule].non_term == m_curr_grammar->axioma_ident)
									{
										// Further search is not possible.
										if (slot.m_frp.action_sym != TLexToSymConverter::lt_eof && want_more_forw_ctx == TRUE)
										{
											// Axioma action symbol belongs to the grammar symbols. Adding this symbol may make the path context not empty.
											AddToSlotsHashTable(m_clev-1);
											m_curr_path[m_clev].PrepareSlot(++m_iter_setup_cnt, rpo_axioma_action_ctx, 0xFFFF, 0xFFFF, slot.m_frp.istate, slot.m_frp.action_sym);
											m_clev++;
										}

										if (SaveDerivationPath(TRUE) == FALSE)
											return(FALSE);

										m_ctx_paths_present = TRUE;
									}
									else
									{
										// Fill the next slot with "step over the symbol" layer and start the new iteration of the starting states.
										m_curr_path[m_clev].m_need_more_backw_ctx = m_want_backw_ctx;
										m_curr_path[m_clev].m_need_more_forw_ctx = m_want_forw_ctx;
										AddStepOverCtxOrUpCtxSlot(m_clev-1);
										AddStartStateCtxSlot();

										m_want_backw_ctx = want_more_backw_ctx;
										m_want_forw_ctx = want_more_forw_ctx;
									}
								}
								else
								{
#ifdef WANT_EXTRA_DEEP_CTX
									if (SaveDerivationPath(TRUE) == FALSE)
										return(FALSE);
#else
									ReportTooDeepDerivationPath();
#endif
									m_ctx_paths_present = TRUE;
								}
							}
						}
						break;

				default:
					HardAssert(FALSE, L"Bogus slot type while searching for conflict context.");
					break;
			}

			// Check for abort cases on only some of the steps.
			if (((WORD)m_cnt_steps) == 0 && m_saved_clev == 0)
			{
				if (CheckForAbortCases() == TRUE)
				{
					continue;
				}
			}

		}
		else
		{
			// Iteration on this level is complete.
			m_clev--;
			RemoveFromSlotsHashTable(m_clev-1);

			if (m_curr_path[m_clev].m_frp.origin == rpo_step_over_ctx)
			{
				// Restore the flag values.
				m_want_backw_ctx = m_curr_path[m_clev].m_need_more_backw_ctx;
				m_want_forw_ctx = m_curr_path[m_clev].m_need_more_forw_ctx;
			}
		}
	}

	if (m_saved_clev == 0)
	{
		// The loop is permanently exited.
		if (m_ctx_paths_present == FALSE)
		{
			// Situation is bad. Crash the process both in Debug and Release builds.
			HardAssert(FALSE, L"Iteration has not found non empty context paths. Current derivation path is about to be lost.");
		}

		m_clev_bottom = 0;
	}

	// Iteration is complete.
	return(TRUE);
}

bool TGrammarConflictAnalysisHelper::SaveDerivationPath(bool ctx_slots_present)
{
	// Calculate the hash value of the current path.
	TFullRulePosition &mid_frp = m_curr_path[m_clev/4].m_frp;
	int path_hash = (mid_frp.irule ^ mid_frp.isym ^ m_clev) & (DERV_PATHS_HASH_LEN-1);

	// It is necessary to figure out if current path is really new or it is similar to some already known path.
	// Compare current path with all paths that fall into the appropriate hash bucket.
	for (int ipath=m_paths_hash_table[path_hash]; ipath >= 0; ipath=m_next_same_hash_path[ipath])
	{
		TGrammarDerivationPath &curr_path = m_result->m_derivation_paths.ItemRef(ipath);

		// Check the length of the path and some random slot in the middle. Paths tend to have identical slots at the beginning and at their ends.
		if (curr_path.derv_path_len != m_clev || curr_path.derv_steps[m_clev/2].dwHeader != m_curr_path[m_clev/2].m_frp.dwHeader)
			continue;

		// Path lengths and middle slots are the same. Compare all slots one by one now.
		bool paths_similar = TRUE;
		TFullRulePosition *path_steps = curr_path.derv_steps;
		for (int istep=0; istep<m_clev; ++istep, ++path_steps)
		{
			if (path_steps->dwHeader != m_curr_path[istep].m_frp.dwHeader)
			{
				// Rule index, rule symbool index or the slot origin  - at least one of these field are different.
				paths_similar = FALSE;
				break;
			}
			else if ((path_steps->origin == rpo_axioma_action_sym || path_steps->origin == rpo_axioma_action_ctx) && path_steps->action_sym != m_curr_path[istep].m_frp.action_sym)
			{
				// These types of steps should differ in the action symbol also.
				paths_similar = FALSE;
				break;
			}
		}

		if (paths_similar == TRUE)
		{
			// Current path is similar to other already known path.
			curr_path.cnt_similar++;

			// Success.
			return(TRUE);
		}
	}

	if (ctx_slots_present == TRUE)
	{
		//
		// Parameter of the method tells that ctx steps are present in the curr path. Look for ctx loops that should look like:
		//
		//	   rpo_call_place_ctx -> rpo_step_over_ctx -> rpo_start_state_ctx.
		//
		#define LOOPS_ARRAY_LEN 11
		int loop_beg[LOOPS_ARRAY_LEN];			// Index of the step that begins of the loop.
		int loop_len[LOOPS_ARRAY_LEN];
		int num_loops = 0, loops_total_len = 0;

		for (int istep=0; istep<m_clev-2; ++istep)
		{
			if (CheckStepsOrgCtx(istep) == FALSE)
				continue;

			// Current step starts the loop triple.
			int itriple = istep+3;
			int itriple_beg = itriple;
			int itriple_end = -1;
			while (itriple < m_clev-2)
			{
				if (CheckStepsOrgCtx(itriple) == FALSE)
					break;
				if (CheckStepsPosCtx(istep, itriple) == FALSE)
					break;

				// Current triple matches the previous triple.
				itriple += 3;
				itriple_end = itriple;
			}

			if (itriple_end != -1)
			{
				// Redundant loop is present. Fill in the loop slot.
				int curr_loop_len = itriple_end-itriple_beg;
				loop_beg[num_loops] = itriple_beg;
				loop_len[num_loops] = curr_loop_len;
				loops_total_len += curr_loop_len;
				num_loops++;

				if (num_loops == LOOPS_ARRAY_LEN-1)
				{
					// The path has too many loops. Process only first N of them.
					break;
				}

				// Continue the loops search.
				istep = itriple;
			}
		}

		// Check if extra ctx loops were found or not.
		if (num_loops > 0)
		{
			// Loops are present. Put end of the loops data mark.
			loop_beg[num_loops] = -1;

			// Look for similar path among existing paths.
			for (int ip2=0; ip2<m_result->m_derivation_paths.NumItems(); ++ip2)
			{
				int num_steps2 = m_result->m_derivation_paths.ItemRef(ip2).derv_path_len;
				TFullRulePosition *stp2 = m_result->m_derivation_paths.ItemRef(ip2).derv_steps;

				// Make several attempts to match the path if there are more than one loop. First time use all discovered
				// loops. Second time skip the first loop and try to match loops starting from the second loop, etc.
				bool paths_similar = FALSE;
				int expct_delta = loops_total_len;

				for (int beg_iloop=0; beg_iloop<num_loops; ++beg_iloop)
				{
					// Knowing the number of loops that will be used and knowing the total numer of redundant trilples in
					// these loops, it is possible to tell the length of the path that may match. Check the length of the path.
					if (m_clev-expct_delta != num_steps2)
					{
						expct_delta -= loop_len[beg_iloop];
						continue;
					}

					// Index is2 in the loop below will be increased sequentially and it will check all slots of the ip2 path.
					int iloop = beg_iloop;
					paths_similar = TRUE;
					for (int is1=0, is2=0; is2<num_steps2; ++is1, ++is2)
					{
						if (is1 == loop_beg[iloop])
						{
							is1 += loop_len[iloop++];
						}

						TFullRulePosition &frp = m_curr_path[is1].m_frp;
						if (frp.dwHeader != stp2[is2].dwHeader)
						{
							paths_similar = FALSE;
							break;
						}
						else if ((frp.origin == rpo_axioma_action_sym || frp.origin == rpo_axioma_action_ctx) && frp.action_sym != stp2[is2].action_sym)
						{
							paths_similar = FALSE;
							break;
						}
					}

					if (paths_similar == TRUE)
					{
						break;
					}
				}

				if (paths_similar == TRUE)
				{
					// Current path is similar to other already known path.
					m_result->m_derivation_paths.ItemRef(ip2).cnt_similar++;

					// Success.
					return(TRUE);
				}
			}
		}
	}

	// Current path is really new. Allocate storage for this new path.
	TGrammarDerivationPath *new_path = m_result->AppendPathProto(m_clev);
	if (m_next_same_hash_path.ReserveSpace() == FALSE || new_path == NULL)
	{
		if (m_tracing_mask & gcfan_critical)
			TraceMsgWithDateAttr(DHM, L"SaveDervPath: Cannot allocate storage for new derivation path.");

		m_result->m_generation_result = TConflictingActionAnalysisResults::gres_out_of_memory;

		if (new_path != NULL)
			m_result->m_derivation_paths.ReleaseLastItem();

		return(FALSE);
	}

	// Update the hash table. Availablility of the space is already verified.
	m_next_same_hash_path.AppendItem(m_paths_hash_table[path_hash]);
	m_paths_hash_table[path_hash] = m_result->m_derivation_paths.NumItems()-1;

	// Copy derivation steps into the allocated storage.
	for (int istp=0; istp<m_clev; ++istp)
		new_path->derv_steps[istp].qwValue = m_curr_path[istp].m_frp.qwValue;

	int num_paths = m_result->m_derivation_paths.NumItems();

	// Full succees.
	if (m_tracing_mask & gcfan_progress)
	{
		wchar_t buffer2[80];
		TraceMsgWithDateAttr(DHM, L"New path(%02d): %d path steps, %s iteration steps.", num_paths, m_clev, FormatInt64(m_cnt_steps, buffer2, 80, fnms_dec_signed, L'_'));
	}

	// Check the overall number of discovered paths.
	if (m_analysis_props.m_max_num_paths > 0 && num_paths >= m_analysis_props.m_max_num_paths)
	{
		if (m_tracing_mask & gcfan_critical)
			TraceMsgWithDateAttr(DHM, L"SaveDervPath: Too many paths (%d) were discovered.", num_paths);

		m_result->m_generation_result = TConflictingActionAnalysisResults::gres_too_many_paths;
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

void TGrammarConflictAnalysisHelper::ClearStatistics()
{
	m_index_creation_errors = 0;
	m_anres_allocation_errors = 0;

	m_cnt_actions_processed = 0;
	memset(m_action_processing_results, 0, sizeof(m_action_processing_results));

	m_num_too_deep_truncations = 0;
	m_num_partial_iter_aborts = 0;
}

int TGrammarConflictAnalysisHelper::GetMaxTransientPathLength()
{
	int longest_transient_path = MAX_DERIV_PATH_LEN;
	for (int i2=0; i2<MAX_DERIV_PATH_LEN; ++i2)
	{
		if (m_curr_path[i2].m_frp.origin == (TFullRulePosOrigin)0xFF)
		{
			longest_transient_path = i2;
			break;
		}
	}

	return(longest_transient_path);
}

int TGrammarConflictAnalysisHelper::GetMaxTransientSlotsHashingAreas()
{
	int max_hashing_areas_count = PATH_SLOTS_HASH_AREAS;
	for (int i3=0; i3<PATH_SLOTS_HASH_AREAS; ++i3)
	{
		if (m_slots_hashing_areas[i3].m_bottom_slot_index < 0)
		{
			max_hashing_areas_count = i3;
			break;
		}
	}

	return(max_hashing_areas_count);
}

int TGrammarConflictAnalysisHelper::LookForIdenticalSlot()
{
	TBuildGrammarDerivationSlot &slot = m_curr_path[m_clev-1];

	// In fact only iterateable slot types should come here. Ensure this.
	TFullRulePosOrigin origin = slot.m_frp.origin;
	assert(origin == rpo_rule_start_state || origin == rpo_rule_call_place || origin == rpo_non_term_defn_rule || origin == rpo_start_state_ctx || origin == rpo_call_place_ctx);

	if (g_ConflictAnalysisHelper_SlotsHashingGroupType[origin] != (m_slots_hashing_areas_index & 1))
	{
		// Special search procedure is needed that should scan beginnings of all hashing areas. If passed slot is
		// identical to the beginning of some area, then the passed slot is consided as duplication.
		// Tricky situation happens when all hash tables become empty. This normally takes place when the action
		// shift is processed. In this case the count of areas is -1, but the loop entering condition in the loop below
		// allows this.
		for (int iarea=0; iarea <= m_slots_hashing_areas_index; ++iarea)
		{
			int islot1 = m_slots_hashing_areas[iarea].m_bottom_slot_index;
			TBuildGrammarDerivationSlot &sa1 = m_curr_path[islot1];
			if (sa1.m_frp.qwValue == slot.m_frp.qwValue)
			{
				if (slot.m_frp.origin == rpo_non_term_defn_rule)
				{
					// In this case the calling slots should be identical also.
					if (m_curr_path[sa1.m_upper_layer].m_frp.qwValue == m_curr_path[slot.m_upper_layer].m_frp.qwValue)
						return(islot1);
				}
				else
				{
					// Equivalence of the values is enough.
					return(islot1);
				}
			}
		}
	}
	else
	{
		// Use regular procedure that will look only in the current slots hashing area. The code below has tricky situation
		// when nothing is pushed into the hash table but the passed slot is already in m_curr_path[0]. The code below
		// works fine becase the hash table is empty. So, the m_curr_path[0] is not checked against itself.
		TPathSlotsHashingArea &area = m_slots_hashing_areas[m_slots_hashing_areas_index];
		int islot2 = area.m_slots_hash_table[(slot.m_frp.irule ^ slot.m_frp.istate) & (PATH_SLOTS_HASH_LEN-1)];
		while (islot2 >= 0)
		{
			TBuildGrammarDerivationSlot &sa2 = m_curr_path[islot2];
			if (sa2.m_frp.qwValue == slot.m_frp.qwValue)
			{
				if (slot.m_frp.origin == rpo_non_term_defn_rule)
				{
					// In this case the calling slots should be identical also.
					if (m_curr_path[sa2.m_upper_layer].m_frp.qwValue == m_curr_path[slot.m_upper_layer].m_frp.qwValue)
						return(islot2);
				}
				else
				{
					// Equivalence of the values is enough.
					return(islot2);
				}
			}

			islot2 = m_next_same_hash_slot[islot2];
		}
	}

	// Passed slot is not present in the current hash table.
	return(-1);
}

bool TGrammarConflictAnalysisHelper::CheckForAbortCases()
{
	if (m_console.GetAbortFlag() == TRUE)
	{
		if (m_tracing_mask & gcfan_critical)
			TraceMsgWithDateAttr(DHM, L"Aborting by request from console.");

		m_result->m_generation_result = TConflictingActionAnalysisResults::gres_aborted_by_user;
		m_saved_clev = m_clev;
		m_clev = 0;
		return(TRUE);
	}

	if (CurrDateTime() > m_timeout_date_time)
	{
		if (m_tracing_mask & gcfan_critical)
			TraceMsgWithDateAttr(DHM, L"Aborting by timeout.");

		m_result->m_generation_result = TConflictingActionAnalysisResults::gres_timeout;
		m_saved_clev = m_clev;
		m_clev = 0;
		return(TRUE);
	}

	DWORD paramValue;
	if (m_console.GetPostedParamValue(0, paramValue, TRUE) == TRUE)
	{
		// User requested to abort current iteration and proceed to other possible branches.
		if ((int)paramValue < m_clev-1)
		{
			// Abort iteration on all layers above the passed layer. This will force to do immediate step on the passed layer.
			if (m_tracing_mask & gcfan_critical)
			{
				TraceMsgAttr(DHM, L"");
				TraceMsgWithDateAttr(DHM, L"Aborting higher layers and making forced iteration step at layer %lu.", paramValue);
			}

			m_result->m_partial_iteration_aborts++;
			for (int ilev=paramValue+1; ilev<m_clev; ++ilev)
			{
				m_curr_path[ilev].SeekToEnd();
			}
		}
		else if ((int)paramValue == m_clev-1)
		{
			// Trace aborting error.
			if (m_tracing_mask & gcfan_critical)
				TraceMsgWithDateAttr(DHM, L"Aborting iteration setup above layer %lu is not possible because it is the current active layer", paramValue);
		}
		else
		{
			// Trace aborting error.
			if (m_tracing_mask & gcfan_critical)
				TraceMsgWithDateAttr(DHM, L"Error aborting iteration setup on layers above %lu. Current number of iteration layers is %d", paramValue, m_clev);
		}
	}

	// Abort cases are not detected.
	return(FALSE);
}

void TGrammarConflictAnalysisHelper::ReportTooDeepDerivationPath(bool extra_context)
{
	assert(m_clev >= m_analysis_props.m_max_path_length);
	m_result->m_too_deep_truncations++;

	if (m_tracing_mask & gcfan_progress)
	{
		if (--m_deep_nest_report_cnt <= 0)
		{
			m_deep_nest_report_cnt = DEEP_NEST_REPORT_FREQ;
			wchar_t buff1[80];
			TraceMsgWithDateAttr(DHM, L"Nesting is too deep (m_clev=%d)%s, %s steps.", m_clev,
							((extra_context == TRUE) ? L" in extra ctx search" : L""), FormatInt64(m_cnt_steps, buff1, 80, fnms_dec_signed, L'_'));
		}
	}
}

void TGrammarConflictAnalysisHelper::TraceOverallProgress(bool want_empty_line)
{
	if (want_empty_line == TRUE)
		TraceMsgAttr(DHM, L"");

	wchar_t buff[80];
	TraceMsgWithDateAttr(DHM, L"Progress: %d path%s discovered, %s step%s.", m_result->m_derivation_paths.NumItems(), PluralSuffix(m_result->m_derivation_paths.NumItems()),
						FormatInt64(m_cnt_steps, buff, 80, fnms_dec_signed, L'_'), PluralSuffix(m_cnt_steps));
	TraceCurrentState(12);
}

void TGrammarConflictAnalysisHelper::TraceCurrentState(int max_slots)
{
	int slots_to_trace = __min(m_clev, max_slots);

	// Line 1: Header.
	wchar_t buffer[4096], *pbuff;
	swprintf(buffer, 4096, L"-- G%d,C%d,A%d: ----------- clev=%d ----------- want_extra_ctx=%s ------------------ (%d conflict%s) ----------",
			m_igrammar, m_iconflict, m_iaction, m_clev, (m_analysis_props.m_search_for_non_empty_ctx == TRUE) ? L"YES" : L"NO", m_curr_grammar->NumConflicts(), PluralSuffix(m_curr_grammar->NumConflicts()));
	TraceMsgAttr(DHM, buffer);

	// Line 2: Slot type.
	pbuff = buffer;
	for (int i1=0; i1<slots_to_trace; ++i1)
	{
		wchar_t *slot_type = L"[=UnkSl=]";
		switch (m_curr_path[i1].m_frp.origin)
		{
			case rpo_initial_placement:		slot_type = L"[InitPlc]";		break;

			case rpo_rule_start_state:		slot_type = L"[RuStart]";	break;
			case rpo_rule_call_place:		slot_type = L"[RuCallP]";	break;
			case rpo_step_over_symbol:	slot_type = L"[StepOvr]";	break;

			case rpo_step_over_non_term:	slot_type = L"[StepOvN]";	break;
			case rpo_non_term_defn_rule:	slot_type = L"[NonTerD]";	break;
			case rpo_step_up_the_tree:		slot_type = L"[StepUpT]";	break;

			case rpo_axioma_action_sym:	slot_type = L"[AxiomaS]";	break;

			case rpo_start_state_ctx:		slot_type = L"[RuStCtx]";	break;
			case rpo_call_place_ctx:		slot_type = L"[RuCaCtx]";	break;
			case rpo_step_over_ctx:		slot_type = L"[StepCtx]";	break;
			case rpo_step_up_ctx:			slot_type = L"[StepUpC]";	break;

			case rpo_axioma_action_ctx:	slot_type = L"[AxiomaC]";	break;
		}

		wcscpy(pbuff, slot_type);
		pbuff += wcslen(pbuff);
	}

	if (slots_to_trace < m_clev)
		wcscpy(pbuff, L" ....");
	TraceMsgAttr(DHM, buffer);

	// Line 3: irule.
	pbuff = buffer;
	for (int i2=0; i2<slots_to_trace; ++i2)
	{
		swprintf(pbuff, 40, L" rul=%3d ", m_curr_path[i2].m_frp.irule);
		pbuff += wcslen(pbuff);
	}

	if (slots_to_trace < m_clev)
		wcscpy(pbuff, L" ....");
	TraceMsgAttr(DHM, buffer);

	// Line 4: istate.
	pbuff = buffer;
	for (int i3=0; i3<slots_to_trace; ++i3)
	{
		swprintf(pbuff, 40, L" st=%4d ", m_curr_path[i3].m_frp.istate);
		pbuff += wcslen(pbuff);
	}

	if (slots_to_trace < m_clev)
		wcscpy(pbuff, L" ....");
	TraceMsgAttr(DHM, buffer);

	// Line 5: iterator setup count.
	pbuff = buffer;
	for (int i6=0; i6<slots_to_trace; ++i6)
	{
		swprintf(pbuff, 40, L" se=%04d ", m_curr_path[i6].m_setup_cnt % 10000);
		pbuff += wcslen(pbuff);
	}

	if (slots_to_trace < m_clev)
		wcscpy(pbuff, L" ....");
	TraceMsgAttr(DHM, buffer);
}

void TGrammarConflictAnalysisHelper::TraceSlotLayerSetupCounts(const wchar_t *method_prefix)
{
	if (m_tracing_mask & gcfan_critical)
	{
		// Print slots useage statistics.
		int longest_transient_path = GetMaxTransientPathLength();
		TraceStdDelimiterLine();
		for (int i3=0; i3<longest_transient_path; ++i3)
		{
			wchar_t buffer1[80];
			TraceMsgWithDateAttr(DHM, L"  Slot[%04d]: SetupsCount: %15s.", i3, FormatInt64(m_curr_path[i3].m_setup_cnts_count, buffer1, 80, fnms_dec_signed, L'_'));
		}

		TraceMsgWithDateAttr(DHM, L"%s: Longest transient path:  %d.", method_prefix, longest_transient_path);
	}
}

void TGrammarConflictAnalysisHelper::TraceFinalStatistics(TDateTime dt_start, bool overall_result, const wchar_t *method_prefix)
{
	if (m_tracing_mask & gcfan_critical)
	{
		wchar_t buffer2[80];
		TraceStdDelimiterLine();
		TraceMsgWithDateAttr(DHM, L"%s: Props: %s action proc timeout.", method_prefix, FormatDuration(ONE_SECOND*m_analysis_props.m_action_proc_timeout, buffer2, 80, TRUE));
		TraceMsgWithDateAttr(DHM, L"%s: Total conflicting actions:  %d.", method_prefix, m_root_grammar->GetNumConflictingActionsHier());
		TraceMsgWithDateAttr(DHM, L"%s: Conflicting actions processed:  %d.", method_prefix, m_cnt_actions_processed);

		if (overall_result == TRUE)
		{
			TraceMsgWithDateAttr(DHM, L"%s: Processing has FULLY SUCCEEDED.", method_prefix);
		}
		else
		{
			TraceMsgWithDateAttr(DHM, L"%s: PROCESSING ERRORS WERE NOTICED.", method_prefix);

			if (m_index_creation_errors > 0)
				TraceMsgWithDateAttr(DHM, L"%s:    -- %d time%s creating indexes failed;", method_prefix, m_index_creation_errors, PluralSuffix(m_index_creation_errors));
			if (m_anres_allocation_errors > 0)
				TraceMsgWithDateAttr(DHM, L"%s:    -- %d time%s creating analysis res array failed;", method_prefix, m_anres_allocation_errors, PluralSuffix(m_anres_allocation_errors));

			for (int ires=0; ires<TConflictingActionAnalysisResults::gres_num_types; ++ires)
			{
				if (ires == TConflictingActionAnalysisResults::gres_not_started || ires == TConflictingActionAnalysisResults::gres_ok)
					continue;

				int num_cases = m_action_processing_results[ires];
				if (num_cases > 0)
				{
					const wchar_t *problem_name = (ires == TConflictingActionAnalysisResults::gres_aborted_by_user) ? L"abort by user" :
										TConflictingActionAnalysisResults::GetGenResultDescription((TConflictingActionAnalysisResults::TGenResult)ires, FALSE);

					TraceMsgWithDateAttr(DHM, L"%s:    -- %d time%s %s happened;", method_prefix, num_cases, PluralSuffix(num_cases), problem_name);
				}
			}

			if (m_num_too_deep_truncations > 0)
				TraceMsgWithDateAttr(DHM, L"%s:    -- %s time%s too deep path truncation happened.", method_prefix,
						FormatInt64(m_num_too_deep_truncations, buffer2, 80, fnms_dec_signed, L'_'), PluralSuffix(m_num_too_deep_truncations));

			if (m_num_partial_iter_aborts > 0)
				TraceMsgWithDateAttr(DHM, L"%s:    -- %s time%s patrial iteration abort happened.", method_prefix,
						FormatInt64(m_num_partial_iter_aborts, buffer2, 80, fnms_dec_signed, L'_'), PluralSuffix(m_num_partial_iter_aborts));
		}

		TraceMsgWithDateAttr(DHM, L"%s: Elapsed time: %s.", method_prefix, FormatDuration(CurrDateTime()-dt_start, buffer2, 80, TRUE));
	}
}

//---------------------------------------------------------------------------------
//  ==============  TDerivationPathsArraySimplificationHelper  ==================
//---------------------------------------------------------------------------------

void TDerivationPathsArraySimplificationHelper::RemovePathsWithRedunantSteps(TConflictingActionAnalysisResults *results_set)
{
	//
	// Scan array of paths and remove paths where the same rule is used more than once in a simple loop.
	//
	assert(results_set != NULL);

	// Handle rare case where same path has more than one loop sequence with outer while loop.
	// This should be not so time consuming because number of paths is typically not big.
	bool smth_removed = TRUE;
	while (smth_removed == TRUE)
	{
		smth_removed = FALSE;
		for (int ip1=0; ip1<results_set->m_derivation_paths.NumItems(); ++ip1)
		{
			// Pick up info about the current path.
			int num_steps = results_set->m_derivation_paths.ItemRef(ip1).derv_path_len;
			TFullRulePosition *steps = results_set->m_derivation_paths.ItemRef(ip1).derv_steps;

			// Check if current path has redundant loops or not.
			int loop_beg = -1;
			int loop_end = -1;
			for (int istep=0; istep<num_steps-2; ++istep)
			{
				if (CheckStepsTriple(steps, istep) == FALSE)
					continue;

				int itriple = istep+3;
				loop_beg = itriple;
				while (itriple < num_steps-2)
				{
					if (CheckStepsTriple(steps, itriple) == FALSE)
						break;

					if (CheckParsingStates(steps, istep, itriple) == FALSE)
						break;

					if (itriple == loop_beg)
					{
						if (CheckRuleSymbolPos2(steps, istep, itriple) == FALSE)
							break;
					}
					else
					{
						if (CheckRuleSymbolPos3(steps, itriple-3, itriple) == FALSE)
							break;
					}

					// Current triple matches the previous triple.
					itriple += 3;
					loop_end = itriple;
				}

				if (loop_end != -1)
					break;
			}

			// Check if something was found or not.
			if (loop_end == -1)
				continue;

			int loop_len = loop_end-loop_beg;

			// Current path has loops. Search for more simple similar path.
			for (int ip2=0; ip2<results_set->m_derivation_paths.NumItems(); ++ip2)
			{
				if (ip2 == ip1)
					continue;

				int num_steps2 = results_set->m_derivation_paths.ItemRef(ip2).derv_path_len;
				TFullRulePosition *stp2 = results_set->m_derivation_paths.ItemRef(ip2).derv_steps;
				if (num_steps-loop_len != num_steps2)
					continue;

				bool paths_similar = TRUE;
				for (int is2=0; is2<num_steps2; ++is2)
				{
					int is1 = (is2 < loop_beg) ? is2 : is2+loop_len;
					if (steps[is1].dwHeader != stp2[is2].dwHeader || steps[is1].istate != stp2[is2].istate)
					{
						paths_similar = FALSE;
						break;
					}
				}

				if (paths_similar == TRUE)
				{
					// Remove the first path from the paths array because it contains similar more simple path.
					results_set->m_derivation_paths.ReleaseItem(ip1--);
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
//  ==============  TDerivationPathPostProcessingHelper  ==================
//-----------------------------------------------------------------------------

void TDerivationPathPostProcessingHelper::RemoveRedundantCtxAnalysisSteps(TGrammar *grammar, int iconflict, int iaction)
{
	TGrammarConflict &cfct = grammar->conflicts[iconflict];
	assert(cfct.analysis_results.NumItems() == cfct.NumActions());

	if (m_tracing_mask & gcfan_critical)
	{
		TraceStdDelimiterLine();
		TraceMsgWithDateAttr(DHM, L"RemoveRedundantCtxAnalysisSteps: iconflict=%d, iaction=%d.", iconflict, iaction);
	}

	// Pick up the paths array.
	TConflictingActionAnalysisResults *result = cfct.analysis_results.ItemPtr(iaction);
	assert(result->m_derivation_paths.NumItems() > 0);

	if (m_tracing_mask & gcfan_critical)
		TraceMsgWithDateAttr(DHM, L"NumPaths: %d", result->m_derivation_paths.NumItems());

	bool smth_removed = TRUE;
	int ipass = 0;
	while (smth_removed == TRUE)
	{
		smth_removed = FALSE;
		if (m_tracing_mask & gcfan_critical)
			TraceMsgWithDateAttr(DHM, L"-----------------------(iPass: %d)-------------------------------", ++ipass);

		for (int ip1=0; ip1<result->m_derivation_paths.NumItems(); ++ip1)
		{
			// Pick up the info about the current path.
			int num_steps = result->m_derivation_paths.ItemRef(ip1).derv_path_len;
			TFullRulePosition *steps = result->m_derivation_paths.ItemRef(ip1).derv_steps;

			// Check if current path has redundant loops or not.
			int loop_beg = -1;
			int loop_end = -1;
			for (int istep=0; istep<num_steps-2; ++istep)
			{
				if (CheckStepsOriginCtx(steps, istep) == FALSE)
					continue;

				// Current place looks like the beginning of the loop.
				int itriple = istep+3;
				loop_beg = itriple;
				while (itriple < num_steps-2)
				{
					if (CheckStepsOriginCtx(steps, itriple) == FALSE)
						break;

					if (CheckStepsPositionCtx(steps, itriple-3, itriple) == FALSE)
						break;

					// Current triple matches the previous triple.
					itriple += 3;
					loop_end = itriple;
				}

				if (loop_end != -1)
					break;
			}

			// Check if something was found or not.
			if (loop_end == -1)
				continue;

			// Current path has redundant loops.
			int loop_len = loop_end-loop_beg;
			if (m_tracing_mask & gcfan_critical)
				TraceMsgWithDateAttr(DHM, L"Redundant loop. ipath=%d, beg_istep=%d, loop_len=%d.", ip1, loop_beg, loop_len/3);

			// Search for similar more simple path.
			for (int ip2=0; ip2<result->m_derivation_paths.NumItems(); ++ip2)
			{
				if (ip2 == ip1)
					continue;

				int num_steps2 = result->m_derivation_paths.ItemRef(ip2).derv_path_len;
				TFullRulePosition *stp2 = result->m_derivation_paths.ItemRef(ip2).derv_steps;
				if (num_steps-loop_len != num_steps2)
					continue;

				bool paths_similar = TRUE;
				for (int is2=0; is2<num_steps2; ++is2)
				{
					int is1 = (is2 < loop_beg) ? is2 : is2+loop_len;
					if (steps[is1].dwHeader != stp2[is2].dwHeader)
					{
						paths_similar = FALSE;
						break;
					}
				}

				if (paths_similar == TRUE)
				{
					if (m_tracing_mask & gcfan_critical)
						TraceMsgWithDateAttr(DHM, L"    Similar Path. ipath=%d,", ip2);
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------------------
//  ====================  TDerivationPathAnalysisHelper  =========================
// ----------------------------------------------------------------------------------------

int TDerivationPathAnalysisHelper::TKeyRulePosition::Compare(TKeyRulePosition &pos1, TKeyRulePosition &pos2)
{
	// Compare rule indexes.
	if (pos1.key_irule > pos2.key_irule)
		return(1);
	if (pos1.key_irule < pos2.key_irule)
		return(-1);

	// Compare symbol indexes.
	if (pos1.key_isym > pos2.key_isym)
		return(1);
	if (pos1.key_isym < pos2.key_isym)
		return(-1);

	// Positions are identical.
	return(0);
}

// ----------------------------------------------------------------------------------------

bool TDerivationPathAnalysisHelper::TKeyRulePositionsArray::IntegrateKeyRulePos(WORD irule, WORD isym)
{
	TKeyRulePosition key_pos = { irule, isym, 1 };

	int ins_before_inx;
	if (FindKeyRulePos(key_pos, ins_before_inx) == FALSE)
	{
		// This is new key rule position.
		if (ins_before_inx == -1)
		{
			if (AppendItem(key_pos) == FALSE)
				return(FALSE);
		}
		else
		{
			if (InsertItem(ins_before_inx, key_pos) == FALSE)
				return(FALSE);
		}
	}
	else
	{
		// Bump refs counter in the existing record.
		ItemRef(ins_before_inx).cnt_refs++;
	}

	// Allocation succeeded if it was needed.
	return(TRUE);
}

bool TDerivationPathAnalysisHelper::TKeyRulePositionsArray::FindKeyRulePos(TKeyRulePosition &key_pos, int &ins_before_inx)
{
	ins_before_inx = -1;
	bool identical_pos_present = FALSE;
	for (int i=0; i<NumItems(); ++i)
	{
		int res = TKeyRulePosition::Compare(key_pos, ItemRef(i));
		if (res == 0)
		{
			// Positions are identical. Report index of this record.
			ins_before_inx = i;
			identical_pos_present = TRUE;
			break;
		}
		else if (res == -1)
		{
			// Array contains record that is bigger than the passed position.
			// This is the right place to insert the new pos.
			ins_before_inx = i;
			break;
		}
	}

	return(identical_pos_present);
}

// ----------------------------------------------------------------------------------------

bool TDerivationPathAnalysisHelper::TConflictContextSeqsArray::IntegrateSymbolsSeq(TSymbolsArray &seq, WORD ic, WORD ia, int ip)
{
	// Process the sequence.
	int ins_before_inx;
	if (FindSymbolsSeq(seq, ins_before_inx) == FALSE)
	{
		// This is the new sequence. Note that search function returned the place where the seq should be inserted.
		if (MergeSymbolsSeq(ins_before_inx, seq) == FALSE)
		{
			return(FALSE);
		}
	}

	// Add initial origin or yet another origin to the list of origins of the sequence.
	TConflictContextSeqOrigin origin = { ic, ia, ip };
	if (ItemRef(ins_before_inx).origins.AppendItem(origin) == FALSE)
	{
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TDerivationPathAnalysisHelper::TConflictContextSeqsArray::IntegrateSymbolsSeq(TSymbolsArray &seq, TConflictContextSeqOriginsArray &orgs)
{
	// Process the sequence.
	int ins_before_inx;
	if (FindSymbolsSeq(seq, ins_before_inx) == FALSE)
	{
		// This is the new sequence. Note that search function returned the place where the seq should be inserted.
		if (MergeSymbolsSeq(ins_before_inx, seq) == FALSE)
		{
			return(FALSE);
		}
	}

	// Append array of the sequence origins.
	if (ItemRef(ins_before_inx).origins.AppendItems(orgs.DataPtr(), orgs.NumItems()) == FALSE)
	{
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

int TDerivationPathAnalysisHelper::TConflictContextSeqsArray::CompareContextSeqs(const TConflictContextSeq *pseq1, const TConflictContextSeq *pseq2)
{
	// These two calls are tricky because seqs may be moved before the call to this function.
	((TConflictContextSeq*)pseq1)->FixObjectMove();
	((TConflictContextSeq*)pseq2)->FixObjectMove();

	// Compare symbols from right to left.
	int iseq1 = pseq1->seq.NumItems()-1;
	int iseq2 = pseq2->seq.NumItems()-1;
	while (iseq1 >= 0 || iseq2 >= 0)
	{
		if (iseq1 < 0)
			return(-1);
		if (iseq2 < 0)
			return(1);

		if (pseq1->seq[iseq1] < pseq2->seq[iseq2])
			return(-1);
		if (pseq1->seq[iseq1] > pseq2->seq[iseq2])
			return(1);

		iseq1--;
		iseq2--;
	}

	// Passed seqs are identical while all seqs in this array should be different.
	assert(FALSE);
	return(0);
}

bool TDerivationPathAnalysisHelper::TConflictContextSeqsArray::MergeSymbolsSeq(int &ins_before_inx, TSymbolsArray &seq)
{
	// Prepare new seq record. This record should contain dupe of the passed sequence and empty list of origins.
	TConflictContextSeq seq_record;
	if (seq_record.seq.AppendItems(seq.DataPtr(), seq.NumItems()) == FALSE)
	{
		return(FALSE);
	}

	// Append or insert this record.
	if (ins_before_inx == -1)
	{
		if (AppendItem(seq_record) == FALSE)
		{
			seq_record.ReleaseObject();
			return(FALSE);
		}

		// Give out the index of the appended record.
		ins_before_inx = NumItems()-1;
	}
	else
	{
		// Passed index of the existing record becomes the index of the new record.
		if (InsertItem(ins_before_inx, seq_record) == FALSE)
		{
			seq_record.ReleaseObject();
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

bool TDerivationPathAnalysisHelper::TConflictContextSeqsArray::FindSymbolsSeq(TSymbolsArray &seq, int &ins_before_inx)
{
	ins_before_inx = -1;
	bool identical_seq_present = FALSE;
	for (int i=0; i<NumItems(); ++i)
	{
		int res = TDerivationPathAnalysisHelper::CompareSymbolArrays(seq, ItemRef(i).seq);
		if (res == 0)
		{
			// Sequences are identical.
			ins_before_inx = i;
			identical_seq_present = TRUE;
			break;
		}
		else if (res == -1)
		{
			// This is the place where the new seq should be inserted.
			ins_before_inx = i;
			break;
		}
	}

	return(identical_seq_present);
}

// ----------------------------------------------------------------------------------------

int __cdecl TDerivationPathAnalysisHelper::TConflictContextOrigin::Compare(const TConflictContextOrigin *org1, const TConflictContextOrigin *org2)
{
	// Compare grammar conflict indexes.
	if (org1->iconflict > org2->iconflict)
		return(1);
	if (org1->iconflict < org2->iconflict)
		return(-1);

	// Compare conflicting action indexes.
	if (org1->iaction > org2->iaction)
		return(1);
	if (org1->iaction < org2->iaction)
		return(-1);

	// Origins are identical.
	return(0);
}

// ----------------------------------------------------------------------------------------

void TDerivationPathAnalysisHelper::TConflictContextRecord::TakeContentsFrom(TConflictContextRecord &rc)
{
	key_positions.TakeContentsFrom(rc.key_positions);
	predecessors.TakeContentsFrom(rc.predecessors);
	followers.TakeContentsFrom(rc.followers);

	origins.TakeContentsFrom(rc.origins);
	new_origins = rc.new_origins;
}

bool TDerivationPathAnalysisHelper::TConflictContextRecord::IntegrateSequencies(TConflictContextRecord &rec)
{
	// Merge predecessors.
	for (int ipr=0; ipr<rec.predecessors.NumItems(); ++ipr)
	{
		TConflictContextSeq &seq = rec.predecessors[ipr];
		if (predecessors.IntegrateSymbolsSeq(seq.seq, seq.origins) == FALSE)
		{
			return(FALSE);
		}
	}

	// Merge followers.
	for (int ifl=0; ifl<rec.followers.NumItems(); ++ifl)
	{
		TConflictContextSeq &seq = rec.followers[ifl];
		if (followers.IntegrateSymbolsSeq(seq.seq, seq.origins) == FALSE)
		{
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

int TDerivationPathAnalysisHelper::TConflictContextRecord::CompareOriginArrays(TConflictContextRecord &rc1, TConflictContextRecord &rc2)
{
	int len1 = rc1.origins.NumItems();
	int len2 = rc2.origins.NumItems();
	int min_len = __min(len1, len2);

	for (int ii=0; ii<min_len; ++ii)
	{
		// Check contents of the slots.
		int res = TConflictContextOrigin::Compare(rc1.origins.ItemPtr(ii), rc2.origins.ItemPtr(ii));
		if (res != 0)
			return(res);
	}

	// Compare lengths.
	if (len1 > len2)
		return(1);
	if (len1 < len2)
		return(-1);

	// Origin arrays are identical.
	return(0);
}

// ----------------------------------------------------------------------------------------

bool TDerivationPathAnalysisHelper::TConflictContextRecordsArray::IntegrateKeyRulePos(int &irecord, WORD irule, WORD isym, WORD iconflict, WORD iaction)
{
	TKeyRulePosition key_pos = { irule, isym, 1 };
	if (FindRecordWithKeyRulePos(key_pos, irecord) == FALSE)
	{
		// This is a new key rule position. Prepare new record in the array for this key rule.
		TConflictContextRecord rec;
		rec.new_origins = FALSE;
		rec.key_positions.AppendItem(key_pos);

		if (irecord == -1)
		{
			if (AppendItem(rec) == FALSE)
				return(FALSE);

			// Give out the index of the appended record.
			irecord = NumItems()-1;
		}
		else
		{
			if (InsertItem(irecord, rec) == FALSE)
				return(FALSE);
		}
	}

	if (iconflict != 0xFFFF && iaction != 0xFFFF)
	{
		// The origin of the key rule position is passed. Find/add this origin.
		TConflictContextOrigin origin = { iconflict, iaction };
		TConflictContextOriginsArray &orgs = ItemRef(irecord).origins;
		bool org_present = FALSE;
		for (int iorg=0; iorg<orgs.NumItems(); ++iorg)
		{
			if (TConflictContextOrigin::Compare(&origin, orgs.ItemPtr(iorg)) == 0)
			{
				org_present = TRUE;
				break;
			}
		}

		if (org_present == FALSE)
		{
			if (orgs.AppendItem(origin) == FALSE)
				return(FALSE);
		}
	}

	return(TRUE);
}

void TDerivationPathAnalysisHelper::TConflictContextRecordsArray::SortByOriginSetsAndKeyRulePositions()
{
	for (int i1=NumItems(); i1 >= 2; --i1)
	{
		for (int i2=1; i2<i1; ++i2)
		{
			assert(ItemRef(i2-1).key_positions.NumItems() == 1);
			assert(ItemRef(i2).key_positions.NumItems() == 1);

			// Compare origin sets of the records.
			int order_res = TConflictContextRecord::CompareOriginArrays(ItemRef(i2-1), ItemRef(i2));

			if (order_res == 0)
			{
				// Origin sets are identical. Compare key rule positions.
				order_res = TKeyRulePosition::Compare(ItemRef(i2-1).key_positions[0], ItemRef(i2).key_positions[0]);
			}

			if (order_res > 0)
			{
				// Swap 2 adjacent records.
				TConflictContextRecord rec;
				rec.TakeContentsFrom(ItemRef(i2-1));
				ItemRef(i2-1).TakeContentsFrom(ItemRef(i2));
				ItemRef(i2).TakeContentsFrom(rec);
			}
		}
	}
}

bool TDerivationPathAnalysisHelper::TConflictContextRecordsArray::FindRecordWithKeyRulePos(TKeyRulePosition &key_pos, int &ins_before_inx)
{
	ins_before_inx = -1;
	bool identical_key_pos_present = FALSE;
	for (int i=0; i<NumItems(); ++i)
	{
		assert(ItemRef(i).key_positions.NumItems() == 1);

		int res = TKeyRulePosition::Compare(key_pos, ItemRef(i).key_positions[0]);
		if (res == 0)
		{
			// Positions are identical.
			ins_before_inx = i;
			identical_key_pos_present = TRUE;
			break;
		}
		else if (res == -1)
		{
			// This is the place where the new record should be inserted.
			ins_before_inx = i;
			break;
		}
	}

	return(identical_key_pos_present);
}

// ----------------------------------------------------------------------------------------

bool TDerivationPathAnalysisHelper::GetDerivationPathContext(TSymbolsArray &buf_predecessors, TSymbolsArray &buf_followers, int iconflict, int iaction, int ipath)
{
	assert(m_grammar != NULL);
	assert(m_grammar->grm_props.processing_result == grpr_full_success);

	// Clear the output vars.
	buf_predecessors.Clear();
	buf_followers.Clear();

	// Pick up info about the conflict, action and path.
	TGrammarConflict &cfct = m_grammar->conflicts[iconflict];
	assert(cfct.analysis_results.NumItems() == cfct.NumActions());

	TGrammarDerivationPathsArray &paths = cfct.analysis_results[iaction].m_derivation_paths;
	assert(ipath >= 0 && ipath < paths.NumItems());

	int path_len = paths[ipath].derv_path_len;
	TFullRulePosition *path_steps = paths[ipath].derv_steps;
	assert(path_len > 0);

	// Allocate stack for tracking state of the non term defns trees.
	TFullRulePositionsArray stk;

	// Process derivation steps.
	bool step_over_ctx_found = FALSE;
	for (int istep=0; istep<path_len; ++istep)
	{
		TFullRulePosOrigin origin = path_steps[istep].origin;
		if (origin == rpo_initial_placement || origin == rpo_rule_call_place || origin == rpo_call_place_ctx)
		{
			// Check if there are symbols in front of the current pos or not.
			WORD isym = path_steps[istep].isym;
			if (isym > 0)
			{
				// Symbols are present. They are predecessors of existing predecessors if any.
				TGrammarRule &rule = m_grammar->rules[path_steps[istep].irule];
				if (buf_predecessors.InsertItems(0, rule.symbols.DataPtr(), isym) == FALSE)
				{
					// Out of memory case.
					return(FALSE);
				}
			}
		}
		else if (origin == rpo_non_term_defn_rule)
		{
			// This is step down the non terminals tree. Save position from where this step was stepping out.
			assert(istep > 0);
			if (stk.AppendItem(path_steps[istep-1]) == FALSE)
			{
				// Out of memory case.
				return(FALSE);
			}
		}
		else if (origin == rpo_step_up_the_tree || origin == rpo_step_up_ctx)
		{
			// This is step up the tree.
			if (step_over_ctx_found == TRUE)
			{
				// Remaining part of the rule belongs to the followers.
				if (AppendRuleTrailingSymbols(buf_followers, path_steps[istep]) == FALSE)
				{
					// Out of memory case.
					return(FALSE);
				}
			}

			// Cancel the latest tree level.
			assert(stk.NumItems() > 0);
			stk.ReleaseLastItem();
		}
		else if (origin == rpo_step_over_ctx)
		{
			// Symbols from the current rule are followers. Previous derivation step shows the initial position of the step.
			// First step over ctx over conflicting symbol that should be included, others should not include non term call place.
			assert(istep > 0);

			int sym_offs = (step_over_ctx_found == FALSE) ? 0 : 1;
			if (AppendRuleTrailingSymbols(buf_followers, path_steps[istep-1], sym_offs) == FALSE)
			{
				// Out of memory case.
				return(FALSE);
			}

			step_over_ctx_found = TRUE;
		}
		else if (origin == rpo_axioma_action_sym || origin == rpo_axioma_action_ctx)
		{
			// Symbol from the step does not belong to any rule but it is the follower.
			if (buf_followers.AppendItem(path_steps[istep].action_sym) == FALSE)
			{
				// Out of memory case.
				return(FALSE);
			}
		}
	}

	// Process potential followers in the last step in the sequence.
	TFullRulePosOrigin lso = path_steps[path_len-1].origin;
	if (lso >= rpo_rule_call_place && lso <= rpo_step_up_the_tree)
	{
		// Paths that do not have context section and end on this type of step can have followers only in their last step.
		// And these followers were not processed in the loop above.
		assert(buf_followers.NumItems() == 0);
		if (AppendRuleTrailingSymbols(buf_followers, path_steps[path_len-1]) == FALSE)
		{
			// Out of memory case.
			return(FALSE);
		}
	}
	else if (lso == rpo_call_place_ctx)
	{
		// When step with this origin stays inside the path, it is always followed by the rpo_step_over_ctx step and followers
		// symbols from this rule are processed in the loop above. When this step is the last step, these symbols should be
		// processed explicitly.
		if (AppendRuleTrailingSymbols(buf_followers, path_steps[path_len-1], 1) == FALSE)
		{
			// Out of memory case.
			return(FALSE);
		}
	}

	// Process pending stack of the non term defns tree layers if any. This stack contains info about the non processed followers.
	for (int istk=stk.NumItems()-1; istk >= 0; --istk)
	{
		// Exclude one symbol from the rule that is the non terminal that is being defined.
		if (AppendRuleTrailingSymbols(buf_followers, stk[istk], 1) == FALSE)
		{
			// Out of memory case.
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

bool TDerivationPathAnalysisHelper::GenerateConflictingActionContext(TConflictContextRecordsArray &buffer, int iconflict, int iaction, bool merge_different_key_rule_seqs)
{
	assert(m_grammar != NULL);
	assert(m_grammar->grm_props.processing_result == grpr_full_success);

	// Clear the output buffer.
	buffer.Clear();

	// Check if analysis results are available.
	if (m_grammar->conflicts[iconflict].analysis_results.NumItems() == 0)
		return(TRUE);

	// Pick up derivation paths for the passed conflicting action.
	TGrammarDerivationPathsArray &paths = m_grammar->conflicts[iconflict].analysis_results[iaction].m_derivation_paths;
	if (paths.NumItems() == 0)
		return(TRUE);

	// Iterate available derivation paths.
	for (int ipath=0; ipath<paths.NumItems(); ++ipath)
	{
		// Pick up the key conflicting position of the current path.
		int key_istep = paths[ipath].GetHiddenConflictPosition();
		TFullRulePosition &key_pos = paths[ipath].derv_steps[key_istep];

		// Figure out the appropriate buffer record.
		int irecord = -1;
		if (merge_different_key_rule_seqs == TRUE)
		{
			// Requested mode expects that buffer should contain only one record.
			irecord = 0;

			// Check if this record already added to the buffer or not yet.
			if (buffer.NumItems() == 0)
			{
				// It is ok not to check the result.
				TConflictContextRecord rec;
				rec.new_origins = FALSE;
				buffer.AppendItem(rec);
			}

			// Modify the list of key rule postions in the record.
			if (buffer[0].key_positions.IntegrateKeyRulePos(key_pos.irule, key_pos.isym) == FALSE)
			{
				// Out of memory case.
				buffer.Clear();
				return(FALSE);
			}
		}
		else
		{
			// Requested mode expects that buffer may have several records and each record of the buffer should have
			// only one key rule position. Find appropriate record for the current key rule pos or create the new record
			// for this position.
			if (buffer.IntegrateKeyRulePos(irecord, key_pos.irule, key_pos.isym) == FALSE)
			{
				// Out of memory case.
				buffer.Clear();
				return(FALSE);
			}

			// Integration procedure should give out the valid record index.
			assert(irecord >= 0);
		}

		// Instantiate ref to the appropriate buffer record.
		TConflictContextRecord &rec = buffer[irecord];

		// Pick up leading and trailing strings for the current path.
		TSymbolsArray predecessors, followers;
		if (GetDerivationPathContext(predecessors, followers, iconflict, iaction, ipath) == FALSE)
		{
			// Out of memory case.
			buffer.Clear();
			return(FALSE);
		}

		// Integrate leading sequence into the ctx record.
		if (predecessors.NumItems() > 0)
		{
			// Predecessors are not empty.
			if (rec.predecessors.IntegrateSymbolsSeq(predecessors, (WORD)iconflict, (WORD)iaction, ipath) == FALSE)
			{
				// Out of memory case.
				buffer.Clear();
				return(FALSE);
			}
		}

		// Integrate trailing sequence into the ctx record. This seq should be present all the times.
		assert(followers.NumItems() > 0);
		if (rec.followers.IntegrateSymbolsSeq(followers, (WORD)iconflict, (WORD)iaction, ipath) == FALSE)
		{
			// Out of memory case.
			buffer.Clear();
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

bool TDerivationPathAnalysisHelper::GenerateExpectedConflictContext(TConflictContextRecordsArray &buffer, int ixpct, bool merge_different_key_rule_seqs)
{
	assert(m_grammar != NULL);
	assert(m_grammar->grm_props.processing_result == grpr_full_success);

	// Clear the output buffer.
	buffer.Clear();

	// Iterate grammar conflicts that belong to the passed expected conflict.
	int iconflict = m_grammar->GetFirstGrammarConflictInx(ixpct);
	while (iconflict >= 0)
	{
		// Iterate actions of the current grammar conflict.
		int num_actions = m_grammar->conflicts[iconflict].NumActions();
		for (int iaction=0; iaction<num_actions; ++iaction)
		{
			// Generate list of key rule positions for the current action. Ask not to merge the sequences from different
			// key rule positions. This means that resulting context may have several records.
			TConflictContextRecordsArray contexts;
			if (GenerateConflictingActionContext(contexts, iconflict, iaction, FALSE) == FALSE)
			{
				// Out of memory case.
				buffer.Clear();
				return(FALSE);
			}

			//
			// It is necessary to merge context that was just retrieved into the bigger context. Idea of this merge is to
			// generate complete list of different key rule positions that appear in different conflicting actions. Each key rule
			// position is commented with a list of actions where this key rule appears and with a list of predecessors/followers
			// that are present in different derivation paths that share this key rule position. Each record of the global buffer
			// still has only one key rule pos.
			//
			if (MergeConflictingActionContext(buffer, contexts, iconflict, iaction) == FALSE)
			{
				// Out of memory case.
				buffer.Clear();
				return(FALSE);
			}
		}

		// Shift to the next grammar conflict.
		iconflict = m_grammar->GetNextGrammarConflictInx(ixpct, iconflict);
	}

	// Sort origins in each record of the array.
	for (int irec1=0; irec1<buffer.NumItems(); ++irec1)
	{
		buffer[irec1].origins.QuickSort(TConflictContextOrigin::Compare);
	}

	//
	// At this point the buffer is an array of records where every record has only one key_position and origins
	// in each record are sorted.
	//

	// Resort the big array to place records with identical sets of origins together.
	buffer.SortByOriginSetsAndKeyRulePositions();

	//
	// Now it is time to process the "merge_different_key_rule_seqs" param. When this param is TRUE, then records that
	// have identical sets of origins are merged into one. This results in records that may have several different key rule positions
	// and sequencies of these key positions are merged also. If this param is FALSE, then only the "new_origins" flag is set/reset
	// that shows if current row starts the new area of identical origins set. This flag is a minor addition that slightly simplifies later
	// processing of the generated buffer.
	//
	for (int irec2=0; irec2<buffer.NumItems(); ++irec2)
	{
		buffer[irec2].new_origins = TRUE;
		if (irec2 > 0)
		{
			if (TConflictContextRecord::CompareOriginArrays(buffer[irec2-1], buffer[irec2]) == 0)
			{
				// Origin sets are identical.
				if (merge_different_key_rule_seqs == TRUE)
				{
					// Merge current record into the previous one.
					TKeyRulePosition &pos = buffer[irec2].key_positions[0];
					if (buffer[irec2-1].key_positions.IntegrateKeyRulePos(pos.key_irule, pos.key_isym) == FALSE)
					{
						// Out of memory case.
						buffer.Clear();
						return(FALSE);
					}

					if (buffer[irec2-1].IntegrateSequencies(buffer[irec2]) == FALSE)
					{
						// Out of memory case.
						buffer.Clear();
						return(FALSE);
					}

					buffer.ReleaseItem(irec2--);
				}
				else
				{
					// Just mark this record as record with the same set of orgins.
					buffer[irec2].new_origins = FALSE;
				}
			}
		}
	}

	// Success.
	return(TRUE);
}

bool TDerivationPathAnalysisHelper::AppendRuleTrailingSymbols(TSymbolsArray &buf, TFullRulePosition &pos, int sym_offs)
{
	TGrammarRule &rule = m_grammar->rules[pos.irule];
	WORD isym = pos.isym+sym_offs;

	if (isym < rule.Length())
	{
		// Symbols to append are present.
		if (buf.AppendItems(rule.symbols.ItemPtr(isym), rule.Length()-isym) == FALSE)
		{
			// Out of memory case.
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

bool TDerivationPathAnalysisHelper::MergeConflictingActionContext(TConflictContextRecordsArray &buffer, TConflictContextRecordsArray &contexts, int iconflict, int iaction)
{
	// Iterate array of context records that is passed as second. Origins in all these records are empty because
	// they all belong to the same conflicting action. Plus all these records should contain only one key rule position.
	for (int i=0; i<contexts.NumItems(); ++i)
	{
		TConflictContextRecord &rec = contexts[i];
		assert(rec.key_positions.NumItems() == 1);

		// Find/create record for the current key conflicting rule.
		int irecord = -1;
		if (buffer.IntegrateKeyRulePos(irecord, rec.key_positions[0].key_irule, rec.key_positions[0].key_isym, (WORD)iconflict, (WORD)iaction) == FALSE)
		{
			// Out of memory case.
			buffer.Clear();
			return(FALSE);
		}

		// Merge predecessors and followers.
		if (buffer[irecord].IntegrateSequencies(rec) == FALSE)
		{
			// Out of memory case.
			buffer.Clear();
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

int TDerivationPathAnalysisHelper::CompareSymbolArrays(TSymbolsArray &a1, TSymbolsArray &a2)
{
	int inx = 0;
	while (inx < a1.NumItems() && inx < a2.NumItems())
	{
		// Compare array elements.
		if (a1[inx] > a2[inx])
			return(1);
		if (a1[inx] < a2[inx])
			return(-1);
		inx++;
	}

	// Compare array lengths.
	if (a1.NumItems() > a2.NumItems())
		return(1);
	if (a1.NumItems() < a2.NumItems())
		return(-1);

	// Arrays are identical.
	return(0);
}

//-----------------------------------------------------------------------
//  ================  TPreceedingStatesIterator  ===================
//-----------------------------------------------------------------------

bool TPreceedingStatesIterator::GetFirstPreceedingState(TGrammar *grammar, WORD dest_state)
{
	assert(grammar != NULL && grammar->grm_props.processing_result == grpr_full_success);
	assert(dest_state != 0 && dest_state < grammar->NumParsingStates());

	m_cnt_alternatives = 0;
	return(GetPreceedingState(grammar, dest_state, 0, 0));
}

bool TPreceedingStatesIterator::GetNextPreceedingState(TGrammar *grammar, WORD dest_state)
{
	WORD curr_state = m_state;
	WORD curr_symbol = m_symbol+1;

	if (curr_symbol < NonTerminalsBase)
	{
		if (curr_symbol >= grammar->NumTerminals())
			curr_symbol = NonTerminalsBase;
	}
	else
	{
		if (curr_symbol-NonTerminalsBase >= grammar->NumNonTerminals())
		{
			curr_symbol = 0;
			curr_state++;
		}
	}

	return(GetPreceedingState(grammar, dest_state, curr_state, curr_symbol));
}

bool TPreceedingStatesIterator::GetPreceedingState(TGrammar *grammar, WORD dest_state, WORD curr_state, WORD curr_symbol)
{
	int row_width = grammar->analysis_table.GetWidth();
	int sym_inx = grammar->SymTableFromSymGrammar(curr_symbol);

	while (curr_state < grammar->NumParsingStates())
	{
		WORD *prow = grammar->analysis_table.GetRowPtr(curr_state);
		while (sym_inx < row_width)
		{
			if (prow[sym_inx] == dest_state)
			{
				// Required pair is found.
				m_state = curr_state;
				m_symbol = grammar->SymGrammarFromSymTable(sym_inx);
				m_cnt_alternatives++;
				return(TRUE);
			}

			sym_inx++;
		}

		curr_state++;
		sym_inx = 0;
	}

	// Required pair was not discovered.
	m_state = 0xFFFF;
	m_symbol = 0xFFFF;
	return(FALSE);
}

//-----------------------------------------------------------------------
//  =============  TParsingStateContextsAnalysisHelper  ===============
//-----------------------------------------------------------------------

bool TParsingStateContextsAnalysisHelper::PrepareIteration(TGrammar *grammar, WORD dest_state)
{
	assert(grammar != NULL && grammar->grm_props.processing_result == grpr_full_success);
	assert(dest_state != 0 && dest_state < grammar->NumParsingStates());

	m_final_state = dest_state;
	if (m_curr_seq.ReserveTotalBufferSpace(grammar->NumParsingStates()+1) == FALSE)
		return(FALSE);

	m_cnt_slots = 0;
	m_grammar = grammar;
	return(TRUE);
}

bool TParsingStateContextsAnalysisHelper::GetFullTreeContext()
{
	bool stt_sym_available;
	if (m_cnt_slots == 0)
	{
		// Initialize the procedure.
		m_curr_seq[0].m_state = m_final_state;
		m_curr_seq[0].m_symbol = 0xFFFF;
		stt_sym_available = m_curr_seq[1].GetFirstPreceedingState(m_grammar, m_final_state);
		m_cnt_slots = 2;
	}
	else
	{
		// Current sequence should be the target sequence.
		assert(m_cnt_slots > 1 && m_curr_seq[m_cnt_slots-1].m_state == 0);

		// Shift iterator of the current layer.
		stt_sym_available = m_curr_seq[m_cnt_slots-1].GetNextPreceedingState(m_grammar, m_curr_seq[m_cnt_slots-2].m_state);
	}

	for(;;)
	{
		while (stt_sym_available == TRUE)
		{
			// Current slot contains the new position. Process it.
			TPreceedingStatesIterator &slot = m_curr_seq[m_cnt_slots-1];

			if (slot.m_state == 0)
			{
				// Current state is an initial state. This is what the whole procedure was looking for this.
				return(TRUE);
			}
			else if (FindDuplicatedPair(slot) == FALSE)
			{
				// Discovered state/symbol pair is new in the path. It is ok to occupy the slot without checking
				// the length of the stack because stack is already allocated for the max possible length.
				stt_sym_available = m_curr_seq[m_cnt_slots++].GetFirstPreceedingState(m_grammar, slot.m_state);
				continue;
			}

			// Current state is already present in the sequence. Look for other state/symbol pair.
			stt_sym_available = slot.GetNextPreceedingState(m_grammar, m_curr_seq[m_cnt_slots-2].m_state);
		}

		// Current slot does not have more state/symbol pairs.
		m_cnt_slots--;
		if (m_cnt_slots <= 1)
			break;

		// Shift iterator of the new current layer.
		stt_sym_available = m_curr_seq[m_cnt_slots-1].GetNextPreceedingState(m_grammar, m_curr_seq[m_cnt_slots-2].m_state);
	}

	// There are no more sequencies.
	return(FALSE);
}

bool TParsingStateContextsAnalysisHelper::GetLimitedTreeContext()
{
	if (m_cnt_slots == 0)
	{
		// Initialize the procedure.
		m_curr_seq[0].m_state = m_final_state;
		m_curr_seq[0].m_symbol = 0xFFFF;
		bool stt_sym_available = m_curr_seq[1].GetFirstPreceedingState(m_grammar, m_final_state);
		m_cnt_slots = 2;

		return(stt_sym_available);
	}

	TPreceedingStatesIterator &slot = m_curr_seq[m_cnt_slots-1];
	if (slot.m_state != 0 && slot.m_cnt_alternatives == 1)
	{
		return(m_curr_seq[m_cnt_slots++].GetFirstPreceedingState(m_grammar, slot.m_state));
	}

	for(;;)
	{
		// Shift iterator of the current layer.
		if (m_curr_seq[m_cnt_slots-1].GetNextPreceedingState(m_grammar, m_curr_seq[m_cnt_slots-2].m_state) == TRUE)
			return(TRUE);

		m_cnt_slots--;
		if (m_cnt_slots <= 1)
			break;
	}

	// There are no more sequencies.
	return(FALSE);
}

bool TParsingStateContextsAnalysisHelper::FindDuplicatedPair(TPreceedingStatesIterator &slot)
{
	TPreceedingStatesIterator *pslots_data = m_curr_seq.DataPtr();
	for (int inx=0; inx<m_cnt_slots-1; ++inx, ++pslots_data)
	{
		if (/*pslots_data->m_state == slot.m_state &&*/ pslots_data->m_symbol == slot.m_symbol)
			return(TRUE);
	}

	// Passed slot is unique in the current sequence.
	return(FALSE);
}


