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

#include  "TextScan/GrammarBasedParser.H"

#define DIRECT_GOTO_ON_REDUCE_ACTION

//----------------------------------------------------------------------------
//  ==================  TGrammarBasedParser  ======================
//----------------------------------------------------------------------------

static const wchar_t g_MinusesLine[] = L"--------------------------------------------------------------------------------";

TGrammarBasedParser::TGrammarBasedParser(TGrammar *grammar)
		: TGenericParser(NULL)
{
	// Verify the error code values from the local enum.
	assert(err_base == perb_grammar_based_base);
	assert(err_max <= perb_grammar_based_max);

	m_root_grammar = grammar;
	m_nest_cstk = 0;
	m_non_term_proto = NULL;
	m_app_callback_note[0] = 0;
	m_saved_grammar = NULL;
	m_saved_conflict = NULL;
	ResetParser();
}

void TGrammarBasedParser::ResetParser()
{
	m_grammar = m_root_grammar;

	CleanupSymbolStateItemsList(m_stack);
	m_cstk = m_state = 0;
	CleanupSymbolStateItemsList(m_inp_symbols);

	m_ulayer_input_queue = NULL;
	m_ulayer_next_symbol = NULL;
	m_ulayer_nesting_index = -1;

	CleanupSymbolStateItemsList(m_symbols_preview_queue);
	m_spq_latest_retvd_symbol = NULL;
	m_spq_latest_retvd_index = -1;

	while (m_nest_cstk > 0)
	{
		CleanupSymbolStateItemsList(m_nest_stk[m_nest_cstk-1].m_pars_stack);
		CleanupSymbolStateItemsList(m_nest_stk[m_nest_cstk-1].m_input_queue);
		m_nest_cstk--;
	}

	if (m_non_term_proto != NULL)
	{
		if (m_non_term_proto->m_dbg_data != NULL)
		{
			m_parser_console->RecycleParserConsoleData(m_non_term_proto->m_dbg_data);
			m_non_term_proto->m_dbg_data = NULL;
		}

		RecycleSymbolStateItem(m_non_term_proto);
		m_non_term_proto = NULL;
	}

	m_app_callback_note[0] = 0;
	CleanupSymbolStateItemsList(m_saved_stack);
	m_saved_grammar = NULL;
	m_saved_conflict = NULL;

	// Clear message from the prev run if any.
	m_critical_error_details[0] = 0;

	// Objects in the look aside list are expected to be ready for deleting.
	m_look_aside_list.ClearTable();

	// Reset the base class as the final step.
	TGenericParser::ResetParser();
}

bool TGrammarBasedParser::Parse()
{
	// Bring the parser to its initial state.
	ResetParser();

	// Trace startup.
	if (m_scanner->GetCurrTraceMode() & gptr_start_stop)
	{
		// Parsing state should not be emitted here because it is empty.
		if (m_scanner->GetCurrTraceMode() & gptr_verbose)
			TraceHeader(L"Parsing Startup");

		// Emit info about the used grammar.
		m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
		m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"ParsingStartup. Grammar: \"%s\".",
							(m_root_grammar != NULL) ? m_root_grammar->grm_props.grms_file_name : L"none");
		m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"ParsingStartup. NumRules: %d, NumStates: %d, NumConflicts: %d.",
							(m_root_grammar != NULL) ? m_root_grammar->NumRules() : 0,
							(m_root_grammar != NULL) ? m_root_grammar->NumParsingStates() : 0,
							(m_root_grammar != NULL) ? m_root_grammar->NumConflicts() : 0);
		m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
	}

	// Check the grammar condition and verify compatibility between the grammar and the currently instantiated callback handler.
	WORD bGrammarOk = (WORD)(m_root_grammar != NULL && m_root_grammar->grm_props.processing_result == grpr_full_success && m_root_grammar->NumParsingStates() > 0 && m_root_grammar->parent_grammar == NULL && CheckGrammar() == TRUE);

	// Send notification to the debugger.
	SendParserConsoleEvent(pcev_parsing_startup, bGrammarOk);

	// Check, if parsing can be started or not.
	if (bGrammarOk == FALSE || CheckForCriticalError() == TRUE)
	{
		// Parsing cannot be started. Exit should be traced here because function will be exited using
		// the return statement. Rest of the code in this function is breaking out of the main loop and
		// exit msg is traced after the end of the loop.
		bool bCriticalError = CheckForCriticalError();
		int nCritcErrorCode = GetCriticalErrorCode();

		if (m_scanner->GetCurrTraceMode() & gptr_start_stop)
			TraceCurrStateIfRequested(L"Parsing Startup Failure");

		if (bGrammarOk == FALSE)
		{
			// One of the condictions above is FALSE. Find this condtion and write an appropriate error message.
			// Note that procedure below should be kept in close sync with TLangViewVisualParserFrame::SetupStoppedState().
			if (m_root_grammar == NULL)
			{
				if (m_scanner->GetCurrTraceMode() & gptr_start_stop)
					m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"Grammar is missing in the parser.");

				if (bCriticalError == FALSE)
				{
					RaiseCriticalError(NULL, err_missing_grammar, L"Grammar is missing.");
				}
			}
			else if (m_root_grammar->grm_props.processing_result == grpr_none)
			{
				if (m_scanner->GetCurrTraceMode() & gptr_start_stop)
					m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"Installed grammar is empty.");

				if (bCriticalError == FALSE)
				{
					RaiseCriticalError(NULL, err_missing_grammar, L"Grammar is empty.");
				}
			}
			else if (m_root_grammar->grm_props.processing_result != grpr_full_success)
			{
				if (m_scanner->GetCurrTraceMode() & gptr_start_stop)
					m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"Grammar contains syntax or conversion errors.");

				if (bCriticalError == FALSE)
				{
					if (m_root_grammar->grm_props.processing_result == grpr_syntax_errors)
						RaiseCriticalError(NULL, err_missing_grammar, L"Grammar syntax errors.");
					else RaiseCriticalError(NULL, err_missing_grammar, L"Grammar conversion errors.");
				}
			}
			else if (m_root_grammar->NumParsingStates() <= 0)
			{
				if (m_scanner->GetCurrTraceMode() & gptr_start_stop)
					m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"Installed grammar does not have parsing table.");

				if (bCriticalError == FALSE)
				{
					RaiseCriticalError(NULL, err_missing_grammar, L"The parsing table is missing.");
				}
			}
			else if (m_root_grammar->parent_grammar != NULL)
			{
				if (m_scanner->GetCurrTraceMode() & gptr_start_stop)
					m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"Grammar is not the root grammar.");

				if (bCriticalError == FALSE)
				{
					RaiseCriticalError(NULL, err_missing_grammar, L"Initial grammar is not the root grammar.");
				}
			}
			else
			{
				if (m_scanner->GetCurrTraceMode() & gptr_start_stop)
					m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"Grammar is not compatible with the callback handler.");

				if (bCriticalError == FALSE)
				{
					// Both handlers have not raised any critical error. Detailed message does not make sense here because
					// debugger has already displayed this situation.
					RaiseCriticalError(NULL, err_incompatible_grammar, L"Incompatible grammar.",
									L"Passed grammar is incompatible with the instantiated callback handler.");
				}
			}
		}

		if (bCriticalError == TRUE)
		{
			if (m_scanner->GetCurrTraceMode() & gptr_start_stop)
				m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"ParsingStartupFailure. Abort was raised by the parser console (err_code=%d).", nCritcErrorCode);
		}

		// Parsing startup failed.
		return(FALSE);
	}

	//
	// Loop over lexemas in the source file. This is the major loop in the parser.
	//
	for(;;)
	{
		// Ensure that input queue is not empty.
		if (m_inp_symbols.IsEmpty() == TRUE)
		{
			if (ReadSymbol() == 0xFFFF)
			{
				break;
			}
		}

		// Pick up the left hand side symbol from the input queue.
		TSymbolStateItem *item = (TSymbolStateItem*)m_inp_symbols.GetFirst();
		assert(item != NULL);

		//
		//  Pick up the action code from the current grammar table. Only 4 types of actions can be picked up:
		//
		//		-- Shift
		//		-- Reduce
		//		-- Conflict
		//		-- GenericSyntaxError
		//
		WORD wAction = m_grammar->analysis_table.GetAction(m_state, m_grammar->SymTableFromSymGrammar(item->m_symbol));
		TParsingActionOriginInfo initial_action_info = { wAction, 0 };

		// Trace and potentially update the action.
		if (PreprocessParsingAction(pcev_action_pickup, wAction, item) == FALSE)
		{
			// Preprocessing of the action failed with the critical error.
			break;
		}

		if (m_nest_cstk > 0)
		{
			// Check for success of the nested parsing. It is better to do this here rather than when the reduce action
			// will be executed. Nested parsing ends when the rule that derives to axioma is about to be executed.
			if (wAction >= actb_reduce && wAction < actb_conflict)
			{
				// Current action is reduce.
				WORD rule_inx = wAction-actb_reduce;
				if (m_grammar->rules[rule_inx].non_term == m_grammar->axioma_ident)
				{
					// Target symbol of the rule is axioma.
					if (m_cstk == m_grammar->rules[rule_inx].Length())
					{
						//
						//     Nested parsing success.
						//
						// Note that reduce action that was just picked up will not be executed in a regular way. Instead of doing
						// reduce, this action will be discarded together with parser stack of the current layer and current action
						// will be converted into the conflict resolution action.
						//

						//
						// Pick up the resolution action. This action can be one of 3 types:
						//
						//		-- Shift
						//		-- Reduce
						//		-- NestedResult
						//
						TParsingLayerInfo &pli = m_nest_stk[m_nest_cstk-1];
						assert(rule_inx <= pli.m_layer_conflict->conflict_resolution.NumItems());
						wAction = pli.m_layer_conflict->conflict_resolution[rule_inx];
						initial_action_info.initial_action = wAction;
						initial_action_info.conflict_action = actb_conflict + (pli.m_layer_conflict-pli.m_layer_grammar->conflicts.DataPtr());

						if (m_scanner->GetCurrTraceMode() & gptr_action_pickup)
						{
							TraceCurrStateIfRequested(L"Exiting Nested Grammar");
							WORD wTargetSym = 0;
							if (m_grammar->rules[rule_inx].Length() > 0)
								wTargetSym = m_grammar->rules[rule_inx].symbols[0];

							wchar_t buff_40_chars1[40], buff_40_chars2[40];
							m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"ExitingNestedGrammar(lev%d): TargetSymbol: %s(%hd), Resolution: %s.",
												m_nest_cstk, m_grammar->GetSymbolName(wTargetSym, buff_40_chars1), wTargetSym,
												pli.m_layer_grammar->GetShortParsingActionName(wAction, buff_40_chars2));
						}

						// Notify console about exiting from this parsing layer. Show the original action that was taken from the parsing table.
						WORD wOriginalAction = actb_reduce+rule_inx;
						SendParserConsoleEvent(pcev_exit_nesting, wOriginalAction);
						if (CheckForCriticalError() == TRUE)
							break;

						//
						//  Exit the nesting layer.
						//

						CleanupSymbolStateItemsList(m_saved_stack);
						m_saved_stack.TakeDataFrom(m_stack);
						m_saved_grammar = m_grammar;
						m_saved_conflict = pli.m_layer_conflict;

						m_nest_cstk--;
						m_grammar = pli.m_layer_grammar;
						m_stack.TakeDataFrom(pli.m_pars_stack);
						m_cstk = pli.m_pars_cstk;
						m_state = pli.m_pars_state;
						CleanupSymbolStateItemsList(m_inp_symbols);
						m_inp_symbols.TakeDataFrom(pli.m_input_queue);
						item = (TSymbolStateItem*)m_inp_symbols.GetFirst();
						assert(item != NULL);

						m_ulayer_input_queue = pli.m_upper_layer_input_queue;
						m_ulayer_next_symbol = pli.m_upper_layer_next_symbol;
						m_ulayer_nesting_index = pli.m_upper_layer_nesting_index;
						m_spq_latest_retvd_symbol = pli.m_preview_latest_processed_symbol;
						m_spq_latest_retvd_index = pli.m_preview_latest_processed_index;

						// Trace and potentially update the resolution action. Note that if parsing action is NestedResult, then
						// it should be changed by the app layer callback either to the shift or to the reduce action, otherwise
						// this will be a critical error and parsing will be aborted.
						if (PreprocessParsingAction(pcev_action_resolution, wAction, item) == FALSE)
						{
							// Preprocessing of the action failed with the critical error.
							break;
						}
					}
				}
			}
		}

		//
		//  Apply the parsing action.
		//

		wchar_t buff_40_chars[40];
		if (wAction == actb_generic_syntax_error)
		{
			// (1) Grammar does not know what to do. This is called generic syntax error. This is a critical error.
			wchar_t error_message[256];
			swprintf(error_message, 256, L"Generic syntax error on the nesting level %d. State: %hd, Symbol: %s(%hd).",
					m_nest_cstk, m_state, m_grammar->GetSymbolName(item->m_symbol, buff_40_chars), item->m_symbol);

			RaiseCriticalError((item->m_symbol < NonTerminalsBase) ? &(item->m_lex.origin) : NULL,
							err_generic_syntax_error, L"Generic syntax error.", error_message);
			break;
		}
		else if (wAction >= actb_bogus_val)
		{
			// (2) Wrong action value that does not mean anything.
			wchar_t error_message[256];
			swprintf(error_message, 256, L"Bogus parsing action value (%hu) on nesting level %d. State: %hd, Symbol: %s(%hd).",
					wAction, m_nest_cstk, m_state, m_grammar->GetSymbolName(item->m_symbol, buff_40_chars), item->m_symbol);

			RaiseCriticalError((item->m_symbol < NonTerminalsBase) ? &(item->m_lex.origin) : NULL,
							err_generic_syntax_error, L"Bogus parsing action.", error_message);

			swprintf(m_critical_error_details, TGenericParser::CRITICAL_ERROR_DTLS_LEN, L"Action value:  %hu.", wAction);
			break;
		}
		else if (wAction >= actb_nested_result)
		{
			// (3) Application layer has not resolved the nested result into the action that can be executed by the grammar based parser.
			wchar_t error_message[256];
			TNestedProcessingResult &nres = m_grammar->nested_processing_results[wAction-actb_nested_result];
			swprintf(error_message, 256, L"Unresolved nested processing result on the nesting level %d. Result name: %s, app_id: %hd.",
					m_nest_cstk, (nres.nest_res_name != NULL) ? nres.nest_res_name : L"none", nres.nest_res_app_id);

			RaiseCriticalError((item->m_symbol < NonTerminalsBase) ? &(item->m_lex.origin) : NULL,
							err_unresvoled_nested_proc, L"Unresolved nested result.", error_message);

			swprintf(m_critical_error_details, TGenericParser::CRITICAL_ERROR_DTLS_LEN, L"Nested result AppId:  %hd.", nres.nest_res_app_id);
			break;
		}
		else if (wAction >= actb_conflict)
		{
			// (4) Grammar came to the conflict. Check if there is nested grammar for this conflict or not.
			TGrammarConflict &cfct = m_grammar->conflicts[wAction-actb_conflict];
			if (cfct.nested_grammar == NULL)
			{
				// (4.1) There is no nested grammar and this conflict was not resolved by the upper layer. This is a critical error.
				wchar_t error_message[256];
				swprintf(error_message, 256, L"Unresolved parsing conflict C%d on nesting level %d. State: %hd, Symbol: %s(%hd).",
						wAction-actb_conflict, m_nest_cstk, m_state, m_grammar->GetSymbolName(item->m_symbol, buff_40_chars), item->m_symbol);

				RaiseCriticalError((item->m_symbol < NonTerminalsBase) ? &(item->m_lex.origin) : NULL,
								err_unresolved_conflict, L"Unresolved parsing conflict.", error_message);

				swprintf(m_critical_error_details, TGenericParser::CRITICAL_ERROR_DTLS_LEN, L"Conflict:  C%d.", wAction-actb_conflict);
				break;
			}
			else if (m_nest_cstk >= MAX_GRAMMAR_NESTING)
			{
				// (4.2) Nested grammar is present, but it is not possible to start the nested parsing because grammar conflicts are nested too deep.
				wchar_t error_message[256];
				swprintf(error_message, 256, L"Grammar nesting overflow. Current nesting level: %d.", m_nest_cstk);

				RaiseCriticalError((item->m_symbol < NonTerminalsBase) ? &(item->m_lex.origin) : NULL,
								err_nesting_stack_overflow, L"Nesting stack overflow.", error_message);
				break;
			}
			else
			{
				// (4.3) Start nested grammar parsing.
				if (m_scanner->GetCurrTraceMode() & gptr_action_pickup)
				{
					TraceCurrStateIfRequested(L"Entering Nested Grammar");
					int ixpct = m_grammar->GetExpectedConflictInx(wAction-actb_conflict);
					assert(ixpct >= 0 && ixpct < m_grammar->NumXpctConflicts());
					m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"EnteringNestedGrammar(lev%d): %s.",
										m_nest_cstk, m_grammar->xpct_conflicts[ixpct].xpct_conflict_name);
				}

				//
				//  Enter the nesting layer.
				//

				// Save the current state.
				TParsingLayerInfo &pli = m_nest_stk[m_nest_cstk++];
				pli.m_layer_grammar = m_grammar;
				pli.m_layer_conflict = &cfct;

				pli.m_pars_stack.TakeDataFrom(m_stack);
				pli.m_pars_cstk = m_cstk;
				pli.m_pars_state = m_state;
				pli.m_input_queue.TakeDataFrom(m_inp_symbols);

				pli.m_upper_layer_input_queue = m_ulayer_input_queue;
				pli.m_upper_layer_next_symbol = m_ulayer_next_symbol;
				pli.m_upper_layer_nesting_index = m_ulayer_nesting_index;
				pli.m_preview_latest_processed_symbol = m_spq_latest_retvd_symbol;
				pli.m_preview_latest_processed_index = m_spq_latest_retvd_index;

				// Switch to the nested grammar.
				m_grammar = cfct.nested_grammar;

				// Parsing stack of the new layer should be empty and initial parsing state is always 0.
				m_cstk = m_state = 0;

				// Direct reading lexemas to the input queue of the upper layer.
				m_ulayer_input_queue = &(pli.m_input_queue);
				m_ulayer_next_symbol = (TSymbolStateItem*)pli.m_input_queue.GetFirst();
				m_ulayer_nesting_index = m_nest_cstk-1;

				// Skip the conflicting symbol.
				m_ulayer_next_symbol = (TSymbolStateItem*)m_ulayer_input_queue->GetNext(m_ulayer_next_symbol);
				if (m_ulayer_next_symbol == NULL)
				{
					// Symbols on this layer are over.
					TParsingLayerInfo &pli = m_nest_stk[m_ulayer_nesting_index];
					m_ulayer_input_queue = pli.m_upper_layer_input_queue;
					m_ulayer_next_symbol = pli.m_upper_layer_next_symbol;
					m_ulayer_nesting_index = pli.m_upper_layer_nesting_index;
				}

				// Show initial state of the nested parsing to debugger.
				SendParserConsoleEvent(pcev_enter_nesting, wAction);
				if (CheckForCriticalError() == TRUE)
					break;
			}
		}
		else if (wAction >= actb_reduce)
		{
			// (5) Reduce action.
			WORD rule_inx = wAction-actb_reduce;
			int rhs_length = m_grammar->rules[rule_inx].Length();

		#ifdef _DEBUG
			// Create the time mark.
			TDateTime callback_entrance_time = CurrDateTime();
		#endif

			// Create state item for the new non terminal. Derived class should do all the necessary work including
			// moving info from symbol state items in the stack to the new non terminal state item inside this call.
			m_app_callback_note[0] = 0;
			m_non_term_proto = AllocateSymbolStateItem(rule_inx);

		#ifdef _DEBUG
			// Pick up the time that was spent in the callback.
			__int64 processing_duration_msec = (CurrDateTime()-callback_entrance_time)/10000;
		#endif

			// Check if symbol allocation succeeded or not.
			if (m_non_term_proto == NULL)
			{
				// This is a critical error.
				if (CheckForCriticalError() == FALSE)
				{
					// Derived class has not issued the critical error. Emit it here.
					WORD rule_non_term = m_grammar->rules[rule_inx].non_term;
					wchar_t error_message[256];
					swprintf(error_message, 256, L"Out of memory while creating SymbolStateItem for non terminal %s(%hd).",
							m_grammar->GetSymbolName(rule_non_term, buff_40_chars), rule_non_term);

					RaiseCriticalError((item->m_symbol < NonTerminalsBase) ? &(item->m_lex.origin) : NULL,
									err_critical_oom, L"Out of memory for TSymbolStateItem.", error_message);
				}

				break;
			}

			// Setup other fields in the non terminal prototype.
			m_non_term_proto->m_symbol = m_grammar->rules[rule_inx].non_term;

			// Notify console about the reduce action. Do this when symbol state item for the new non terminal
			// is already created. Do not check for abort condition until reduce action will not be complete.
			SendParserConsoleEvent(pcev_action_resolve, wAction, initial_action_info.dw_action_origin);
			if (CheckForCriticalError() == TRUE)
				break;

			// Remove symbols and states from the stack. The number of symbols to remove is equal to the length
			// of the rule that is being applied.
			if (rhs_length > 0)
			{
				assert(m_cstk >= rhs_length);
				m_cstk -= rhs_length;
				while (rhs_length > 0)
				{
					TSymbolStateItem *item_to_remove = (TSymbolStateItem*)m_stack.GetLast();
					assert(item_to_remove != NULL);
					m_stack.RemoveItem(item_to_remove);
					m_state = item_to_remove->m_parsing_state;

					// Check if debugger data is released or not.
					if (item_to_remove->m_dbg_data != NULL)
					{
						m_parser_console->RecycleParserConsoleData(item_to_remove->m_dbg_data);
						item_to_remove->m_dbg_data = NULL;
					}

					RecycleSymbolStateItem(item_to_remove);
					rhs_length--;
				}
			}

			// Check for the end of parsing condition.
			if (m_non_term_proto->m_symbol == m_grammar->axioma_ident)
			{
				// Stack should be empty and input symbol should be EOF.
				if (m_stack.IsEmpty() == TRUE && GetInpSymPtr(0)->m_symbol == TLexToSymConverter::lt_eof)
				{
					// This should be the top layer parsing.
					assert(m_nest_cstk == 0);
					ReportParsingMessage(err_pars_msg_success, L"Parsing succeeded.");
					break;
				}
			}

		#ifdef DIRECT_GOTO_ON_REDUCE_ACTION
			{
				// Add new non terminal to the stack and change the parsing state. This variant is considered to be classic.
				m_non_term_proto->m_parsing_state = m_state;
				m_stack.AppendItem(m_non_term_proto);
				m_cstk++;
				WORD wGotoAction = m_grammar->analysis_table.GetAction(m_state, m_grammar->SymTableFromSymGrammar(m_non_term_proto->m_symbol));
				assert(wGotoAction < actb_reduce);
				m_state = wGotoAction;
			}
		#else
			{
				// Add new non terminal to the beginning of the input queue. This variant of code is more simple.
				m_inp_symbols.PrependItem(m_non_term_proto);
			}
		#endif

			// Processing reduce action is complete.
			m_non_term_proto = NULL;
		}
		else
		{
			// (6) This is shift action. It is not allowed to shift in the eof character.
			assert(item->m_symbol != TLexToSymConverter::lt_eof);

			// Ensure that parser stack has reasonable length.
			if (m_cstk > PARSER_STACK_MAX_LEN)
			{
				wchar_t error_message[256];
				swprintf(error_message, 256, L"Parser stack overflow on the nesting level %d. Current stack length: %d.", m_nest_cstk, PARSER_STACK_MAX_LEN);
				RaiseCriticalError((item->m_symbol < NonTerminalsBase) ? &(item->m_lex.origin) : NULL,
								err_parser_stack_overflow, L"Parser stack overflow.", error_message);
				break;
			}

			// Notify the console.
			SendParserConsoleEvent(pcev_action_shift, wAction, initial_action_info.dw_action_origin);
			if (CheckForCriticalError() == TRUE)
				break;

			// Remove symbol from the input queue and push it together with the current parsing state into the stack of the parser.
			m_inp_symbols.RemoveItem(item);
			item->m_parsing_state = m_state;
			m_stack.AppendItem(item);
			m_cstk++;

			// Change the current parsing state.
			m_state = wAction-actb_shift;
		}
	}

	// Trace the final state.
	if (m_scanner->GetCurrTraceMode() & gptr_start_stop)
	{
		TraceCurrStateIfRequested(L"+++ Final Parser State +++");
		m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"ParsingResult(lev%d): %s.",
							m_nest_cstk, (CheckForCriticalError() == TRUE) ? GetCriticalErrorDescription() : L"Success");
	}

	// Send notification that parser is about to terminate. Do not check for critical error here because
	// parsing is going to stop in any case.
	WORD dummy_evt_data = 0;
	SendParserConsoleEvent(pcev_parsing_complete, dummy_evt_data);

	// Give out the results of parsing.
	return(CheckForCriticalError() == FALSE && m_scanner->GetErrorsCount() == 0);
}

void TGrammarBasedParser::SetAppNote(const wchar_t *msg)
{
	// Store the passed string with possible truncation.
	int len_buff = sizeof(m_app_callback_note)/sizeof(wchar_t);
	wcsncpy(m_app_callback_note, msg, len_buff);
	m_app_callback_note[len_buff-1] = 0;
}

void TGrammarBasedParser::InsertInputQueueSymbol(int ins_before_inx, TSymbolStateItem *sym_state_item)
{
	// Trace this call.
	if (m_scanner->GetCurrTraceMode() & gptr_error_recovery)
	{
		wchar_t buff_40_chars[40];
		TraceCurrStateIfRequested(L"Syntax Error Recovery");
		m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"ErrorRecovery(lev%d): Inserting symbol %s(%hd) before input queue item with index %d.",
							m_nest_cstk, m_grammar->GetSymbolName(sym_state_item->m_symbol, buff_40_chars), sym_state_item->m_symbol, ins_before_inx);
	}

	// Insert the symbol.
	if (ins_before_inx >= 0)
	{
		TSymbolStateItem *item = GetInpSymPtr(ins_before_inx);
		assert(item != NULL);
		m_inp_symbols.InsertBefore(sym_state_item, item);
	}
	else
	{
		m_inp_symbols.AppendItem(sym_state_item);
		ins_before_inx = m_inp_symbols.NumItems()-1;
	}

	// Notify the debugger.
	SendParserConsoleEvent(pcev_errec_sym_added, (WORD&)ins_before_inx);
}

void TGrammarBasedParser::UpdateInputQueueSymbol(int symbol_inx, WORD new_terminal_symbol_value)
{
	// Look for the symbol.
	TSymbolStateItem *item = GetInpSymPtr(symbol_inx);
	assert(item != NULL);

	// Trace this call.
	if (m_scanner->GetCurrTraceMode() & gptr_error_recovery)
	{
		wchar_t buff_40_chars1[40], buff_40_chars2[40];
		TraceCurrStateIfRequested(L"Syntax Error Recovery");
		m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"ErrorRecovery(lev%d): Update symbol at index %d from symbol %s(%hd) to symbol value %s(%hd).",
							m_nest_cstk, symbol_inx, m_grammar->GetSymbolName(item->m_symbol, buff_40_chars1), item->m_symbol,
							m_grammar->GetSymbolName(new_terminal_symbol_value, buff_40_chars2), new_terminal_symbol_value);
	}

	// Do the requested action. Note that debugger has its own data field where is stores the symbol value.
	// Field in the symbol state is updated before calling the debugger to show this new value to  the debugger.
	item->m_symbol = new_terminal_symbol_value;

	// Notify the debugger.
	SendParserConsoleEvent(pcev_errec_sym_updated, (WORD&)symbol_inx);

	// Now patch lexema in the symbol state item.
	PatchLexemaInfoToSymbol(item, item->m_symbol);
}

void TGrammarBasedParser::RemoveInputQueueSymbol(int symbol_inx, TSymbolStateItem **removed_sym_state_item)
{
	// Look for the symbol. It should be present and it should not be EOF.
	TSymbolStateItem *item = GetInpSymPtr(symbol_inx);
	assert(item != NULL);
	assert(item->m_symbol != TLexToSymConverter::lt_eof);

	// Trace this call.
	if (m_scanner->GetCurrTraceMode() & gptr_error_recovery)
	{
		wchar_t buff_40_chars[40];
		TraceCurrStateIfRequested(L"Syntax Error Recovery");
		m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"ErrorRecovery(lev%d): Remove input symbol %s(%hd) at index %d.",
							m_nest_cstk, m_grammar->GetSymbolName(item->m_symbol, buff_40_chars), item->m_symbol, symbol_inx);
	}

	// Notify debugger before removing the symbol.
	SendParserConsoleEvent(pcev_errec_sym_removed, (WORD&)symbol_inx);

	// Do the requested action.
	if (removed_sym_state_item != NULL)
	{
		// Caller wants to use this symbol in some way.
		*removed_sym_state_item = item;
	}
	else
	{
		// Get rid of the symbol.
		DiscardInputSymbol(item);
	}
}

WORD TGrammarBasedParser::GetParsingActionForInpSym(int inp_symbol_inx)
{
	// Look for the symbol.
	TSymbolStateItem *item = GetInpSymPtr(inp_symbol_inx);
	assert(item != NULL);

	// Return action code for this symbol.
	return(m_grammar->analysis_table.GetAction(m_state, m_grammar->SymTableFromSymGrammar(item->m_symbol)));
}

PrEventHeader *TGrammarBasedParser::GetCurrentLocation()
{
	// Look for the first terminal symbol in the input queue. In typical case all symbols in this queue are terminals.
	for (TListIter<TSymbolStateItem> iter(m_inp_symbols); iter; ++iter)
	{
		if (iter.CurrItem().m_symbol < NonTerminalsBase)
			return(&(iter.CurrItem().m_lex.origin));
	}

	// Input queue does not contain terminal symbols.
	return(NULL);
}

TSymbolStateItem *TGrammarBasedParser::GetStackSlotPtr(int num_right_slots)
{
	assert(num_right_slots >= 0);
	if (num_right_slots == 0)
		return(NULL);

	// Prepick the last item of the stack.
	TSymbolStateItem *item = (TSymbolStateItem*)m_stack.GetLast();
	assert(item != NULL);

	// Shift the pointer as many times as the caller wants.
	assert(num_right_slots <= m_cstk);
	while (--num_right_slots > 0)
	{
		item = (TSymbolStateItem*)m_stack.GetPrev(item);
		assert(item != NULL);
	}

	return(item);
}

TSymbolStateItem *TGrammarBasedParser::GetInpSymPtr(int symbol_index)
{
	TSymbolStateItem *item = (TSymbolStateItem*)m_inp_symbols.GetFirst();
	while (symbol_index-- > 0 && item != NULL)
		item = (TSymbolStateItem*)m_inp_symbols.GetNext(item);
	return(item);
}

TSymbolStateItem *TGrammarBasedParser::GetInpSymPtrEx(int symbol_index)
{
	// Pick up the leftmost item of the input queue.
	TSymbolStateItem *item = (TSymbolStateItem*)m_inp_symbols.GetFirst();
	if (item == NULL)
	{
		if (ReadSymbol() == 0xFFFF)
			return(NULL);

		item = (TSymbolStateItem*)m_inp_symbols.GetFirst();
	}

	// Shift pointer as many times as caller has requested.
	while (symbol_index-- > 0)
	{
		item = (TSymbolStateItem*)m_inp_symbols.GetNext(item);
		if (item == NULL)
		{
			if (ReadSymbol() == 0xFFFF)
				return(NULL);

			item = (TSymbolStateItem*)m_inp_symbols.GetLast();
		}
	}

	// Return value should not be NULL.
	assert(item != NULL);
	return(item);
}

TSymbolStateItem *TGrammarBasedParser::GetSavedStackSlotPtr(int num_right_slots)
{
	assert(num_right_slots >= 0);
	if (num_right_slots == 0)
		return(NULL);

	// Prepick the last item of the stack.
	TSymbolStateItem *item = (TSymbolStateItem*)m_saved_stack.GetLast();
	assert(item != NULL);

	// Shift the pointer as many times as the caller wants.
	while (--num_right_slots > 0)
	{
		item = (TSymbolStateItem*)m_saved_stack.GetPrev(item);
		assert(item != NULL);
	}

	return(item);
}

bool TGrammarBasedParser::PreprocessParsingAction(TGenericParserConsoleEvent action_origin, WORD &wAction, TSymbolStateItem *&leftmost_input_item)
{
	assert(action_origin == pcev_action_pickup || action_origin == pcev_action_resolution);
	WORD action_before_preprocessing = wAction;

	// Trace curr state of the parser and the picked action.
	if (m_scanner->GetCurrTraceMode() & gptr_action_pickup)
	{
		wchar_t buff_40_chars[40];
		TraceCurrStateIfRequested((action_origin == pcev_action_pickup) ? L"Parsing Action Pickup" : L"Parsing Action as Nested Grammar Result");
		m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"ParsingAction%s(lev%d): %s.",
							((action_origin == pcev_action_pickup) ? L"Pickup" : L"AsNestedGrammarResult"),
							m_nest_cstk, m_grammar->GetParsingActionName(wAction, buff_40_chars));
	}

	// Show picked parsing action to the debugger. Note that debugger may update the action.
	SendParserConsoleEvent(action_origin, wAction);
	WORD after_debugger_action = wAction;
	if (CheckForCriticalError() == TRUE)
	{
		return(FALSE);
	}

	// Allow derived class to inspect/update the parsing action. Application layer may also do other actions,
	// like error recovery modifications in the input queue.
	m_app_callback_note[0] = 0;
	ReviewParsingAction(wAction);
	if (CheckForCriticalError() == TRUE)
	{
		return(FALSE);
	}

	if (wAction != action_before_preprocessing)
	{
		// Trace the updated parsing action.
		if (m_scanner->GetCurrTraceMode() & gptr_action_update)
		{
			wchar_t buff_40_chars[40];
			TraceCurrStateIfRequested(L"Updated Parsing Action");
			m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"UpdatedParsingAction(lev%d): %s.", m_nest_cstk, m_grammar->GetParsingActionName(wAction, buff_40_chars));
		}

		if (wAction != after_debugger_action)
		{
			// Notify debugger that app layer has changed the parsing action.
			SendParserConsoleEvent(pcev_action_update, wAction);
			if (CheckForCriticalError() == TRUE)
			{
				return(FALSE);
			}
		}

		// Repick the input queue item because it might change.
		leftmost_input_item = (TSymbolStateItem*)m_inp_symbols.GetFirst();
		assert(leftmost_input_item != NULL);
	}

	// Success.
	return(TRUE);
}

void TGrammarBasedParser::PatchLexemaInfoToSymbol(TSymbolStateItem *item, WORD sym)
{
	// Passed symbol should be lexema.
	assert(sym < IgnoreLexTypeBase);
	TTerminalSymbol &sym_info = m_grammar->symbols[sym];

	item->m_lex.type = sym_info.lex_type;
	if (sym_info.check_subt == TRUE)
	{
		if (sym_info.lex_type == ltx_keyword)
		{
			// Reconstruct the subtype from the token id.
			TLexKeywordType subt = TLexema::GetKeywordTypeFromTokenId(sym_info.lex_subt.subtype);
			assert(subt != ltkn_num_keyword_types);

			item->m_lex.subtype = subt;
			item->m_lex.num_value = sym_info.lex_subt.subtype;
		}
		else
		{
			// Simply fill in the subtype.
			item->m_lex.subtype = sym_info.lex_subt.subtype;
		}
	}
}

void TGrammarBasedParser::ReportParsingMessage(int msg_code, const wchar_t *format, ...)
{
	// Prepare the message.
	wchar_t buff[MAX_PATH+80];
	va_list vargs;
	va_start(vargs, format);
	vswprintf(buff, MAX_PATH, format, vargs);
	va_end(vargs);

	// Ask scanner to generate and save the error.
	m_scanner->AddErrorWarning(NULL, 0, lerrc_message, msg_code, buff);
}

void TGrammarBasedParser::ReportError(int err_code, const wchar_t *format, ...)
{
	// Prepare the message.
	wchar_t buff[MAX_PATH+80];
	va_list vargs;
	va_start(vargs, format);
	vswprintf(buff, MAX_PATH, format, vargs);
	va_end(vargs);

	// Ask scanner to generate and save the error.
	m_scanner->AddErrorWarning(NULL, 0, lerrc_syntax_err, err_code, buff);
}

void TGrammarBasedParser::ReportError(TLexema &err_location, int err_code, const wchar_t *format, ...)
{
	// Prepare the message.
	wchar_t buff[MAX_PATH+80];
	va_list vargs;
	va_start(vargs, format);
	vswprintf(buff, MAX_PATH, format, vargs);
	va_end(vargs);

	// Ask scanner to generate and save the error.
	m_scanner->AddErrorWarning(&err_location.origin, 0, lerrc_syntax_err, err_code, buff);
}

WORD TGrammarBasedParser::ReadSymbol()
{
	// Check if EOF is already retrieved from the source file or not.
	TSymbolStateItem *item = (TSymbolStateItem*)m_inp_symbols.GetLast();
	if (item != NULL && item->m_symbol == TLexToSymConverter::lt_eof)
	{
		// EOF is already present in the input queue. There is no point to read more.
		if (m_scanner->GetCurrTraceMode() & gptr_symbol_pickup)
		{
			if (m_scanner->GetCurrTraceMode() & gptr_verbose)
				TraceHeader(L"Read Symbol");
			m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"ReadSymbol(lev%d): EOF is already present in the input queue.", m_nest_cstk);
		}

		return(TLexToSymConverter::lt_eof);
	}

	//
	// Loop on reading and/or modifying/discarding lexemas.
	//
	wchar_t buff_40_ch1[40], buff_40_ch2[40];
	for(;;)
	{
		//
		//  Step 1. Allocate or retrieve the symbol state item structure and setup the m_lex field there.
		//
		if (m_symbols_preview_queue.IsEmpty() == FALSE && m_nest_cstk == 0)
		{
			// In this case SymbolStateItem should not be allocated. Just retrieve the first symbol from
			// the preview queue. There is no need to update the queue reading pointers.
			item = (TSymbolStateItem*)m_symbols_preview_queue.GetFirst();
			m_symbols_preview_queue.RemoveItem(item);
		}
		else
		{
			// New lexema needs a slot to be stored in.
			item = AllocateSymbolStateItemForLexema();
			if (item == NULL)
			{
				// Critical error is already announced.
				return(0xFFFF);
			}

			if (m_ulayer_next_symbol != NULL)
			{
				// Symbol should be taken from the queue of the upper layer.
				assert(m_nest_cstk != 0);
				TSymbolStateItem *ulayer_sym = m_ulayer_next_symbol;
				m_ulayer_next_symbol = (TSymbolStateItem*)m_ulayer_input_queue->GetNext(m_ulayer_next_symbol);

				if (m_ulayer_next_symbol == NULL)
				{
					// Symbols on this layer are over.
					TParsingLayerInfo &pli = m_nest_stk[m_ulayer_nesting_index];
					m_ulayer_input_queue = pli.m_upper_layer_input_queue;
					m_ulayer_next_symbol = pli.m_upper_layer_next_symbol;
					m_ulayer_nesting_index = pli.m_upper_layer_nesting_index;
				}

				// Copy lexema into the output symbol.
				item->m_lex = ulayer_sym->m_lex;
			}
			else if (m_symbols_preview_queue.IsEmpty() == FALSE && (m_spq_latest_retvd_symbol == NULL || m_symbols_preview_queue.GetNext(m_spq_latest_retvd_symbol) != NULL))
			{
				// Symbol should be taken from the preview queue.
				assert(m_nest_cstk != 0);
				if (m_spq_latest_retvd_symbol == NULL)
				{
					// Start reading the preview queue.
					m_spq_latest_retvd_symbol = (TSymbolStateItem*)m_symbols_preview_queue.GetFirst();
				}
				else
				{
					// Pick up the next symbol.
					m_spq_latest_retvd_symbol = (TSymbolStateItem*)m_symbols_preview_queue.GetNext(m_spq_latest_retvd_symbol);
				}

				assert(m_spq_latest_retvd_symbol != NULL);
				m_spq_latest_retvd_index++;

				// Copy lexema into the output symbol.
				item->m_lex = m_spq_latest_retvd_symbol->m_lex;
			}
			else
			{
				// Symbol should be taken from the source file.
				m_scanner->Scan(item->m_lex);

				if (m_nest_cstk != 0)
				{
					// Save scanned lexema in the preview queue.
					m_symbols_preview_queue.AppendItem(item);
					m_spq_latest_retvd_symbol = item;
					m_spq_latest_retvd_index++;

					// Create another item for giving it out.
					item = AllocateSymbolStateItemForLexema();
					if (item == NULL)
					{
						return(0xFFFF);
					}

					// Copy scanned lexema into the output item.
					item->m_lex = m_spq_latest_retvd_symbol->m_lex;
				}
			}
		}

		//
		//  Step 2. Assign the symbol value according to the current grammar.
		//
		item->m_symbol = m_grammar->lex_to_sym_conv.GetSymFromLex(item->m_lex);
		assert(item->m_symbol != TLexToSymConverter::lt_check_subtype);

		// Trace.
		if (m_scanner->GetCurrTraceMode() & gptr_symbol_pickup)
		{
			if (m_scanner->GetCurrTraceMode() & gptr_verbose)
				TraceHeader(L"Read Symbol");

			m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"ReadSymbol(lev%d): New symbol %s(%hd), %s(%s) is retrieved.",
								m_nest_cstk, m_grammar->GetSymbolName(item->m_symbol, buff_40_ch1), item->m_symbol,
								TLexema::GetLexTypeEnumName(item->m_lex.type), PrepareLexSubtypeEnumName(item, buff_40_ch2));
		}

		// Append item to the input queue.
		WORD initial_sym_value = item->m_symbol;
		m_inp_symbols.AppendItem(item);

		// Notify the debugger about this symbol.
		SendParserConsoleEvent(pcev_symbol_pickup, item->m_symbol);
		if (CheckForCriticalError() == TRUE)
		{
			// Do not remove the symbol.
			return(0xFFFF);
		}

		//
		//  Step 3. Present this symbol to the app callback handler. The app layer may update the m_symbol field.
		//  It may also do other changes in the input queue for the error recovery or similar things.
		//
		m_app_callback_note[0] = 0;
		ReviewInputSymbol(item);
		if (CheckForCriticalError() == TRUE)
		{
			// Do not remove the symbol.
			return(0xFFFF);
		}

		// Check if app has changed the symbol value or not.
		if (item->m_symbol != initial_sym_value)
		{
			// Trace.
			if (m_scanner->GetCurrTraceMode() & gptr_symbol_update)
			{
				if (m_scanner->GetCurrTraceMode() & gptr_verbose)
				{
					// Temporarily change the symbol value back to print the parser state.
					WORD new_sym_saved = item->m_symbol;
					item->m_symbol = initial_sym_value;
					TraceHeader(L"Read Symbol");
					item->m_symbol = new_sym_saved;
				}

				m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"ReadSymbol(lev%d): App layer changed symbol value from %s(%hd) to %s(%hd).",
									m_nest_cstk, m_grammar->GetSymbolName(initial_sym_value, buff_40_ch1), initial_sym_value,
									m_grammar->GetSymbolName(item->m_symbol, buff_40_ch2), item->m_symbol);
			}

			// Notify debugger about the symbol change.
			SendParserConsoleEvent(pcev_symbol_update, item->m_symbol);
			if (CheckForCriticalError() == TRUE)
			{
				// Do not remove the symbol.
				return(0xFFFF);
			}

			// Patch lexema in the symbol state item.
			PatchLexemaInfoToSymbol(item, item->m_symbol);
		}

		//
		//  Step 4. Give this symbol out or discard it.
		//

		// Check if current symbol can be given out or not.
		if (item->m_symbol < m_grammar->symbols.NumItems())
		{
			// This is terminal symbol. Give it out.
			break;
		}

		// Once the lexema is not given out, it should be discarded. Check if error message should be issued or not.
		if (item->m_symbol == TLexToSymConverter::lt_sym_empty)
		{
			// There are no instructons on what to do with this symbol. Issue the default error.
			ReportError(item->m_lex, err_unmappable_lex_type,
						L"Processing for %s(%s) is not defined. Lexema is ignored.",
						TLexema::GetLexTypeEnumName(item->m_lex.type), PrepareLexSubtypeEnumName(item, buff_40_ch2));
		}
		else if (item->m_symbol >= ErrorLexTypeBase)
		{
			// Issue the lexema specific error.
			int ierr = item->m_symbol-ErrorLexTypeBase;
			assert(ierr < m_grammar->error_lex.NumItems());
			ReportError(item->m_lex, err_prohibited_lex_type, m_grammar->error_lex[ierr].error_message);
		}
		else
		{
			// Symbol value should be one of the allowed ignoreable values. Ensure this.
			assert(item->m_symbol == GenericIgnoreLex || (item->m_symbol >= IgnoreLexTypeBase && item->m_symbol < IgnoreLexTypeBase+m_grammar->ignore_lex.NumItems()));
		}

		// Trace the symbol discarding.
		if (m_scanner->GetCurrTraceMode() & gptr_symbol_discard)
		{
			if (m_scanner->GetCurrTraceMode() & gptr_verbose)
				TraceHeader(L"Read Symbol");

			m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"ReadSymbol(lev%d): Discarding input symbol %s(%hd), %s(%s).",
								m_nest_cstk, m_grammar->GetSymbolName(item->m_symbol, buff_40_ch1), item->m_symbol,
								TLexema::GetLexTypeEnumName(item->m_lex.type), PrepareLexSubtypeEnumName(item, buff_40_ch2));
		}

		// Notify the debugger.
		SendParserConsoleEvent(pcev_symbol_discard, item->m_symbol);
		if (CheckForCriticalError() == TRUE)
		{
			// Do not remove the symbol.
			return(0xFFFF);
		}

		// Discad this symbol and go on scanning.
		DiscardInputSymbol(item);
	}

	// New terminal symbol (maybe EOF) was retrieved from the source.
	return(item->m_symbol);
}

void TGrammarBasedParser::PrepareNonTerminalArea(PrEventHeader &non_term_area, TSymbolStateItem *first_rule_sym)
{
	assert(first_rule_sym != NULL);

	// Set the area of the non terminal to the area of the first rule symbol.
	non_term_area = first_rule_sym->GetOriginHeader();
	non_term_area.cn = 0;

	TSymbolStateItem*sym = (TSymbolStateItem*)m_stack.GetNext(first_rule_sym);
	while (sym != NULL)
	{
		// Extend the area of the non terminal and shift to the next symbol.
		non_term_area.ExtendPrHeader(sym->GetOriginHeader());
		sym = (TSymbolStateItem*)m_stack.GetNext(sym);
	}
}

TSymbolStateItem *TGrammarBasedParser::AllocateSymbolStateItemForLexema()
{
	// Call the app layer callback.
	m_app_callback_note[0] = 0;
	TSymbolStateItem *lexema_item = AllocateSymbolStateItem();
	if (lexema_item == NULL)
	{
		// This is a critical error.
		if (CheckForCriticalError() == FALSE)
		{
			// Derived class has not issued critical error. Emit it here.
			RaiseCriticalError(NULL, err_critical_oom, L"Out of memory for TSymbolStateItem.",
							L"Out of memory while creating SymbolStateItem for storing the input symbol.");
		}
	}

	return(lexema_item);
}

void TGrammarBasedParser::CleanupSymbolStateItemsList(TList &sym_stt_list)
{
	while (sym_stt_list.IsEmpty() == FALSE)
	{
		// Extract the first item from the list.
		TSymbolStateItem  *item = (TSymbolStateItem*)sym_stt_list.GetLast();
		sym_stt_list.RemoveItem(item);

		// Ensure it has no debug data.
		if (item->m_dbg_data != NULL)
		{
			m_parser_console->RecycleParserConsoleData(item->m_dbg_data);
			item->m_dbg_data = NULL;
		}

		// Pass it to the application callback for recycling.
		RecycleSymbolStateItem(item);
	}
}

void TGrammarBasedParser::DiscardInputSymbol(TSymbolStateItem *item)
{
	m_inp_symbols.RemoveItem(item);

	// Ensure that passed symbol has no debug data.
	if (item->m_dbg_data != NULL)
	{
		m_parser_console->RecycleParserConsoleData(item->m_dbg_data);
		item->m_dbg_data = NULL;
	}

	RecycleSymbolStateItem(item);
}

void TGrammarBasedParser::TraceCurrStateIfRequested(const wchar_t *header)
{
	if ((m_scanner->GetCurrTraceMode() & gptr_verbose) == 0)
	{
		// This type of tracing is not active.
		return;
	}

	// Emit the header.
	if (header != NULL)
		TraceHeader(header);

	TTextBuffer<1024> buff;
	wchar_t buffer[256], buff_40_chars[40];

	buff.Clear();
	int num_stack_symbols = m_stack.NumItems();
	swprintf(buffer, 256, L"PARS_STK_LAYER(%d)[%ld]%s", m_nest_cstk, num_stack_symbols, (num_stack_symbols > 0) ? L": " : L";");
	buff.Append(buffer);

	TListIter<TSymbolStateItem> iter1(m_stack);
	for (int i1=1; iter1; ++iter1, ++i1)
	{
		swprintf(buffer, 256, L"%hd.", iter1.CurrItem().m_parsing_state);
		buff.Append(buffer);

		WORD sym_val = iter1.CurrItem().m_symbol;
		buff.Append(m_grammar->GetSymbolName(sym_val, buff_40_chars));

		swprintf(buffer, 256, L"(%hd)%s", sym_val, (i1 != num_stack_symbols) ? L", " : L";");
		buff.Append(buffer);
	}

	m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, buff);

	// Emit current parsing state.
	buff.Clear();
	swprintf(buffer, 256, L"STATE: %hd;", m_state);
	buff.Append(buffer);
	m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, buff);

	// Emit list of retrieved symbols.
	buff.Clear();
	int num_inp_symbols = m_inp_symbols.NumItems();
	swprintf(buffer, 256, L"INPUT_QUEUE[%ld]%s", num_inp_symbols, (num_inp_symbols > 0) ? L": " : L";");
	buff.Append(buffer);

	TListIter<TSymbolStateItem> iter2(m_inp_symbols);
	for (int i2=1; iter2; ++iter2, ++i2)
	{
		WORD sym_val = iter2.CurrItem().m_symbol;
		buff.Append(m_grammar->GetSymbolName(sym_val, buff_40_chars));

		swprintf(buffer, 256, L"(%hd)%s", sym_val, (i2 != num_inp_symbols) ? L", " : L";");
		buff.Append(buffer);
	}

	m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, buff);

	// Emit unprocessed non terminal.
	if (m_non_term_proto != NULL)
	{
		m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"NON_TERM_PROTO: %s(%hd);",
							m_grammar->GetSymbolName(m_non_term_proto->m_symbol, buff_40_chars), m_non_term_proto->m_symbol);
	}
}

void TGrammarBasedParser::TraceHeader(const wchar_t *header)
{
	m_scanner->TraceEvent(NULL, TPRF_GRAM_BASED_PARS, L"------- %s %.*s", header, 60-wcslen(header), g_MinusesLine);
}

const wchar_t *TGrammarBasedParser::PrepareLexSubtypeEnumName(TSymbolStateItem *item, wchar_t *buff_40_chars)
{
	TLexSubtype lex_subt;
	lex_subt.subtype = item->m_lex.subtype;
	if (item->m_lex.type == ltx_keyword)
		lex_subt.subtype = (DWORD)item->m_lex.num_value;

	return(TGrammar::MakeLexSubtypeEnumName(item->m_lex.type, TRUE, lex_subt, buff_40_chars));
}


