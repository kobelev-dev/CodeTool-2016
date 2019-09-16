//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Backus-Naur productions grammar processor.
//

#ifndef	TextScan_GrammarAnalysis_H
#define	TextScan_GrammarAnalysis_H

#ifndef   TextScan_GrammarStructure_H
#include  "TextScan/GrammarStructure.H"
#endif
#ifndef   TextScan_ParsingDriver_H
#include  "TextScan/ParsingDriver.H"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Note: This structure has no ctors because it is used in a union.
struct TRuleStartStatesIterator  /* (1) */
{
	//
	//  NB: If the "parsing_state_at_rule_isym" param value is 0xFFFF, then the value of rule_isym param is not used.
	//
	//  After calling the PrepareIteration(..)/PrepareNoIndexIteration(..), the iterator is not pointing anywhere.
	//  A typical use pattern is:
	//
	//		iter.Prepare(...);
	//		for(;;)
	//		{
	//			iter.Step();
	//			if (iter == FALSE)
	//				break;
	//			//
	//			// Call CurrState()/CurrActionSymbol() methods.
	//			// Do other processing.
	//			//
	//		}
	//
	//  Iterator does not know what method was used for its preparation.
	//

	void PrepareIteration(TGrammar *grammar, TRulePosIterIndexLevel2 &index2, int irule, int rule_isym, WORD parsing_state_at_rule_isym, WORD action_sym_restr)
	{
		// Init info and restriction fields.
		m_grammar = grammar;
		m_rule_symbols = grammar->rules[irule].symbols.DataPtr();

		m_irule = irule;
		m_rule_isym = rule_isym;
		m_state_at_rule_isym = parsing_state_at_rule_isym;
		m_action_sym_restr = action_sym_restr;

		// Init iteration state fields.
		m_startup_flag = TRUE;
		m_stts_index = index2[irule].DataPtr();
		m_stts_index_len = index2[irule].NumItems();

		// In the current design of the rule start statex index, Level 1 array always contains the final dummy element.
		// This allows picking up the record that stays after all records that are included into the length of array.
		m_positions_iter.Setup(m_grammar->parsing_states[m_stts_index->pars_stt], m_stts_index->pos_beg, m_stts_index->num_positions);
		m_curr_state = m_stts_index->pars_stt;
	}

	void PrepareNoIndexIteration(TGrammar *grammar, int irule, int rule_isym, WORD parsing_state_at_rule_isym, WORD action_sym_restr)
	{
		// Init info and restriction fields.
		m_grammar = grammar;
		m_rule_symbols = grammar->rules[irule].symbols.DataPtr();

		m_irule = irule;
		m_rule_isym = rule_isym;
		m_state_at_rule_isym = parsing_state_at_rule_isym;
		m_action_sym_restr = action_sym_restr;

		// Init iteration state fields.
		m_startup_flag = TRUE;
		m_stts_index = NULL;
		m_stts_index_len = grammar->NumParsingStates();
		m_no_index_stts_iter.Setup(grammar);
		m_positions_iter.Setup(m_no_index_stts_iter.CurrState());
		m_curr_state = 0;
	}

	// The return value shows whether iterator contains some valid current state or not, i.e. if CurrState() and CurrActionSymbol()
	// can be called or not. The method Step() can be called at any time without checking anything. After calling Step(), the method
	// bool() may return a different value.
	inline operator bool() const
	{
		// This field can be used to check an overall state both for index and no index cases.
		return(m_stts_index_len > 0);
	}

	void Step()
	{
		// Iterators should not be shifted after startup.
		if (m_startup_flag == FALSE && m_stts_index_len > 0)
			StepIters();
		else m_startup_flag = FALSE;

		// Shift iterator to the next conforming state if any.
		while (m_stts_index_len > 0 && CheckCurrState() == FALSE)
			StepIters();
	}

	void StepNoIndex()
	{
		// Iterators should not be shifted after startup.
		if (m_startup_flag == FALSE && m_no_index_stts_iter == TRUE)
			StepItersNoIndex();
		else m_startup_flag = FALSE;

		// Shift iterator to the next conforming state if any.
		while (m_no_index_stts_iter == TRUE && CheckCurrState() == FALSE)
			StepItersNoIndex();
	}

	void SeekToEnd()
	{
		m_stts_index_len = 0;
	}

	void SeekToEndNoIndex()
	{
		m_stts_index_len = 0;
		m_no_index_stts_iter.SeekToEnd();
	}

	//
	//  This iterator delivers a parsing state and an action symbol.
	//
	//  If iterator is started with the action_sym_restr equal to 0xFFFF, then the action symbol may differ from one iteration
	//  step to anoter. Otherwise an action symbol will be all the times the same - the one that was passed as a resitriction.
	//

	inline WORD CurrState() const { return(m_curr_state); }
	inline WORD CurrActionSymbol() const { return(m_positions_iter.CurrPos().action_sym); }

protected:

	void PrepareIterationSpecial(TRulePosIterIndexLevel2 &index2, int num_remaining_positions)
	{
		// This method is called by the XML reader. It expects that a lot of fields are already set.
		m_rule_symbols = m_grammar->rules[m_irule].symbols.DataPtr();
		m_stts_index = index2[m_irule].DataPtr()+(index2[m_irule].NumItems()-m_stts_index_len);

		if (m_stts_index_len > 0)
		{
			m_positions_iter.Setup(m_grammar->parsing_states[m_stts_index->pars_stt], m_stts_index->pos_beg, m_stts_index->num_positions);
			m_positions_iter.ShiftForward(m_positions_iter.NumAvalablePositions()-num_remaining_positions);
			m_curr_state = m_stts_index->pars_stt;
		}
	}

	bool CheckCurrState() const
	{
		TRulePosition &pos = m_positions_iter.CurrPos();

		// Ensure that the current position belongs to the right rule and that it is a starting position.
		if (pos.irule != m_irule || pos.isym != 0)
			return(FALSE);

		if (m_action_sym_restr != 0xFFFF)
		{
			// Check the action symbol restriction.
			if (pos.action_sym != m_action_sym_restr)
				return(FALSE);
		}

		if (m_state_at_rule_isym != 0xFFFF)
		{
			// Check whether curr state derives to the targer state or not.
			WORD derived_state = m_curr_state;
			for (int isym=0; isym<m_rule_isym; ++isym)
			{
				derived_state = m_grammar->GetNextParsingState(derived_state, m_rule_symbols[isym]);
			}

			if (derived_state != m_state_at_rule_isym)
			{
				// Current starting state derives to a different target state.
				return(FALSE);
			}
		}

		// Full success.
		return(TRUE);
	}

	void StepIters()
	{
		++m_positions_iter;
		if (m_positions_iter == FALSE)
		{
			assert(m_stts_index_len > 0);

			// Move to the next parsing state.
			m_stts_index++;
			m_stts_index_len--;

			// In the current design of the rule start statex index, the Level 1 array contains a final dummy element.
			// This allows picking up the record that stays after all records that are included into the length of the array.
			m_positions_iter.Setup(m_grammar->parsing_states[m_stts_index->pars_stt], m_stts_index->pos_beg, m_stts_index->num_positions);
			m_curr_state = m_stts_index->pars_stt;
		}
	}

	void StepItersNoIndex()
	{
		++m_positions_iter;
		if (m_positions_iter == FALSE)
		{
			assert(m_stts_index_len > 0);
			assert(m_no_index_stts_iter == TRUE);

			// Move to the next parsing state.
			m_stts_index_len--;
			++m_no_index_stts_iter;
			m_positions_iter.Setup(m_no_index_stts_iter.CurrState());
			m_curr_state = m_no_index_stts_iter.CurrStateIndex();
		}
	}

protected: /* (1) */

	// Grammar and rule info. These fields are not changing during the iteration.
	TGrammar						*m_grammar;
	WORD							*m_rule_symbols;

	// Parsing state restrictions. These fields are not changing during the iteration.
	int								m_irule, m_rule_isym;
	WORD							m_state_at_rule_isym, m_action_sym_restr;

	// State of the iteration.
	bool								m_startup_flag;
	TRulePosIterIndexRecord			*m_stts_index;					// The field has meaning only for index based operation.
	int								m_stts_index_len;				// This field is used both in index and NO INDEX methods.
	TParsingStatesIterator				m_no_index_stts_iter;			// The field is used only in NO INDEX methods.
	TParsingStatePositionsIterator		m_positions_iter;
	WORD							m_curr_state;

	friend class TSavedConflictingActionAnalysisStateXmlWriter;
	friend class TRuleStartStatesIteratorXmlReader;
};

// Note: This structure has no ctors because it is used in a union.
struct TRuleCallPlacesIterator  /* (2) */
{
	//
	//  After calling the PrepareIteration() method, the iterator is not pointing anywhere. It is necessary to call
	//  method Step() before quering the current state of the iterator.
	//
	void PrepareIteration(TGrammar *grammar, WORD state_before_non_term, WORD non_term, WORD action_sym_restr)
	{
		// Init all data fields.
		m_grammar = grammar;
		m_state_before_non_term = state_before_non_term;
		m_non_term = non_term;
		m_action_sym_restr = action_sym_restr;

		m_startup_flag = TRUE;
		m_positions_iter.Setup(grammar->parsing_states[state_before_non_term]);
	}

	inline operator bool() const
	{
		return(m_positions_iter == TRUE);
	}

	inline WORD GetNonTerm() const { return(m_non_term); }

	void Step()
	{
		// Iterators should not be shifted after startup.
		if (m_startup_flag == FALSE && m_positions_iter == TRUE)
			++m_positions_iter;
		else m_startup_flag = FALSE;

		// Shift iterator to the next conforming state if any.
		while (m_positions_iter == TRUE && CheckCurrState() == FALSE)
			++m_positions_iter;
	}

	void SeekToEnd()
	{
		m_positions_iter.SeekToEnd();
	}

	//
	//  This iterator delivers olny a rule position with the fixed parsing state.
	//

	inline TRulePosition &CurrPos() const { return(m_positions_iter.CurrPos()); }

protected: /* (2) */

	void PrepareIterationSpecial(int num_remaining_positions)
	{
		// This method is called by the XML reader. It expects that the grammar, parsing state, non terminal symbol,
		// action restriciton symbol and the startup flag fields are already set.
		m_positions_iter.Setup(m_grammar->parsing_states[m_state_before_non_term]);
		m_positions_iter.ShiftForward(m_positions_iter.NumAvalablePositions()-num_remaining_positions);
	}

	bool CheckCurrState() const
	{
		TRulePosition &pos = m_positions_iter.CurrPos();
		if (m_action_sym_restr != 0xFFFF)
		{
			if (pos.action_sym != m_action_sym_restr)
				return(FALSE);
		}

		TGrammarRule &rule = m_grammar->rules[pos.irule];
		if (rule.symbols.NumItems() == 0 || rule.symbols[pos.isym] != m_non_term)
			return(FALSE);

		// Current position stays in front of the requested non terminal with the proper parsing state and action symbol.
		return(TRUE);
	}

	// Grammar and restrictions info. These fields are not changing during the iteration.
	TGrammar						*m_grammar;
	WORD							m_state_before_non_term;			// This field is needed only for XML I/O.
	WORD							m_non_term;
	WORD							m_action_sym_restr;

	// State of the iteration. Inner iterator is prepared only once when the PrepareIteration() method is called.
	bool								m_startup_flag;
	TParsingStatePositionsIterator		m_positions_iter;

	friend class TSavedConflictingActionAnalysisStateXmlWriter;
	friend class TRuleCallPlacesIteratorXmlReader;
};

// Note: This structure has no ctors because it is used in a union.
struct TNonTermDefnRulesIterator  /* (3) */
{
	//
	//  After calling the PrepareIteration(), the iterator is not pointing anywhere.
	//
	void PrepareIteration(TGrammar *grammar, TRulePosIterIndexLevel2 &index2, WORD rule_starting_state, WORD rule_non_term, bool iterate_diff_action_symbols)
	{
		// Init info and restriction fields.
		m_grammar = grammar;
		m_rule_starting_state = rule_starting_state;
		m_rule_non_term = rule_non_term;
		m_iter_diff_action_syms = iterate_diff_action_symbols;

		// Init the iteration state fields.
		m_startup_flag = TRUE;
		TRulePosIterIndexRecord &irec = index2[rule_non_term-NonTerminalsBase][rule_starting_state];
		m_positions_iter.Setup(grammar->parsing_states[rule_starting_state], irec.pos_beg, irec.num_positions);
		m_latest_irule = 0xFFFF;
	}

	inline operator bool() const
	{
		return(m_positions_iter.m_num_positions > 0);
	}

	void Step()
	{
		// Iterators should not be shifted after startup.
		if (m_startup_flag == FALSE && m_positions_iter.m_num_positions > 0)
			++m_positions_iter;
		else m_startup_flag = FALSE;

		// Shift iterator to next conforming state if any.
		while (m_positions_iter.m_num_positions > 0 && CheckCurrState() == FALSE)
			++m_positions_iter;
	}

	void SeekToEnd()
	{
		m_positions_iter.SeekToEnd();
	}

	//
	//  This iterator delivers a rule index and a rule triggering symbol.
	//

	inline TGrammarRule	&CurrRule() const				{ return(m_grammar->rules[m_positions_iter.m_rule_pos->irule]); }
	inline int				CurrRuleInx() const				{ return(m_positions_iter.m_rule_pos->irule); }

	inline WORD			CurrentActionSymbol() const	{ return((m_iter_diff_action_syms == TRUE) ? m_positions_iter.m_rule_pos->action_sym : 0xFFFF); }
							// This method is called CurrentActionSymbol and not CurrActionSymbol to make it diffferent
							// from a similar method in the TRuleStartStatesIterator.
protected: /* (3) */

	void PrepareIterationSpecial(TRulePosIterIndexLevel2 &index2, int num_remaining_positions)
	{
		// This method is called by the XML reader. It expects that the grammar, parsing state, non terminal symbol
		// and several other fields are already set.
		TRulePosIterIndexRecord &irec = index2[m_rule_non_term-NonTerminalsBase][m_rule_starting_state];
		m_positions_iter.Setup(m_grammar->parsing_states[m_rule_starting_state], irec.pos_beg, irec.num_positions);
		m_positions_iter.ShiftForward(m_positions_iter.NumAvalablePositions()-num_remaining_positions);
	}

	bool CheckCurrState()
	{
		TRulePosition &pos = m_positions_iter.CurrPos();
		if (pos.isym != 0)
			return(FALSE);

		TGrammarRule &rule = m_grammar->rules[pos.irule];
		if (rule.non_term != m_rule_non_term)
			return(FALSE);

		if (m_iter_diff_action_syms == FALSE)
		{
			if (pos.irule == m_latest_irule)
				return(FALSE);
			m_latest_irule = pos.irule;
		}

		// Current position stays at the beginning of the rule that defines the requested non terminal.
		return(TRUE);
	}

	// Grammar and restrictions info. These fields are not changing during the iteration.
	TGrammar						*m_grammar;
	WORD							m_rule_starting_state;			// This field is needed only for XML I/O.
	WORD							m_rule_non_term;
	bool								m_iter_diff_action_syms;

	// State of the iteration. Inner iterator is prepared only once when the PrepareIteration() method is called.
	bool								m_startup_flag;
	int								m_latest_irule;
	TParsingStatePositionsIterator		m_positions_iter;

	friend class TSavedConflictingActionAnalysisStateXmlWriter;
	friend class TNonTermDefnRulesIteratorXmlReader;
};

struct TBuildGrammarDerivationSlot
{
protected:

	TFullRulePosition		m_frp;						// The origin of the rule position controls what iterator from the union below
													// should be used. Some origin types are not iterateable. This means that none
													// of iterators are inited in the slot.
	int					m_upper_layer;				// This field shows if curr slot is inside the non term defns subtree (value >= 0)
													// or not (value == -1). Note that this field is checked without checking the type
													// of the slot origin. Only the following slot types can have this field set:
													//
													//		non_term_defn_rule	--	Value from the slot in front of it.
													//		step_over_non_term	--	Value from the slot in front of it. And this slot
													//								in front is always non_term_defn_rule.
													//		step_up_the_tree		--	Complex computed value.
													//		step_over_ctx			--	Complex computed value when search for extra
													//								ctx is started. Later on the value is -1.
													//		step_up_ctx			--	Complex computed value.
													//

	// Look into GetNextPosition() method to see what option from the union below is currentrly used.
	union
	{
		TRuleStartStatesIterator		m_rule_start_states_iter;
		TRuleCallPlacesIterator			m_rule_call_places_iter;
		TNonTermDefnRulesIterator		m_non_term_defn_rules_iter;
	};

	bool					m_need_more_backw_ctx;
	bool					m_need_more_forw_ctx;

	//
	// Statistics and consistence check fields. These fields are not used for making decisions on the iteration process continuation.
	//

	DWORD				m_setup_cnt;				// This setup count allows to track for how long the slot is occupied by the certain
													// iterator. This field has no app level meaning.
	int					m_positions_cnt;			// Number of positions, discovered during one particular setup of the slot.
													// This field allows to check/track if at least one position was discovered or not.
	__int64				m_setup_cnts_count;		// Total number of setup events on the slot. This field allows to see distribution
													// of setup events between different layers.
protected:

	void PrepareSlot(DWORD setup_cnt, TFullRulePosOrigin phase, WORD irule, WORD isym, WORD istate, WORD act_sym)
	{
		m_frp.origin = phase;
		m_frp.irule = irule;
		m_frp.isym = (BYTE)isym;
		m_frp.istate = istate;
		m_frp.action_sym = act_sym;

		m_upper_layer = -1;

		m_setup_cnt = setup_cnt;
		m_positions_cnt = 0;
		m_setup_cnts_count++;
	}

	bool GetNextPosition();
	void SeekToEnd();

	friend class TSavedConflictingActionAnalysisStateXmlWriter;
	friend class TSavedConflictingActionAnalysisStateXmlReader;
	friend class TBuildGrammarDerivationSlotXmlReader;
	friend class TGrammarConflictAnalysisHelper;
	friend class TBuildGrammarDerivationSlotTest;
};

enum TGrammarAnalysisHelperTraceFlags
{
	gcfan_critical	=  0x1,		// Start/End of the analysis process, reason of the abnormal termination if any.
	gcfan_progress	=  0x2,		// Additional messages that are emitted every several seconds to indicate the progress.
	gcfan_verbose	=  0x4,		// Detailed tracing of each step. This flag can be used in debug builds only.

	#define GCFAN_PREFIX			L"CfctAnalysis"
};

class TBasicGrammarAnalysisHelper
{
public:

	TBasicGrammarAnalysisHelper(TGenericConsoleEx &console, DWORD tracing_mask) : m_console(console)  { m_tracing_mask = tracing_mask; }

	enum TAnalysisConsoleLabels
	{
		cons_label_num_scheduled_actions	= 2000,
		cons_label_num_started_actions		= 2001,
		cons_label_num_no_paths_actions	= 2002,
		cons_label_num_remaining_actions	= 2003,
	};

	enum
	{
		DHM = TGenericConsole::DefHighlightMode,
		RedAttr = (FOREGROUND_RED | FOREGROUND_INTENSITY),
		YellowAttr =  (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY),
		ActionSelectionAttr = (FOREGROUND_GREEN | FOREGROUND_INTENSITY),
	};

	void		TraceMsgAttr(WORD highlight_mode, const wchar_t *message);
	void		TraceMsgWithDateAttr(WORD highlight_mode, const wchar_t *format, ...);
	void		TraceStdDelimiterLine(WORD highlight_mode = DHM);

	void		HardAssert(bool condition, const wchar_t *msg);

	static const wchar_t *DEF_AN_RES_SUBDIR;
				// Default subdirectory name for storing the results of analysis.

protected:

	static void BuildIndexesProgressCallbackHandler(void *context, int items_done, int total_items)
	{
		// Let Windows rotate its message loop.
		TBasicGrammarAnalysisHelper *inst = (TBasicGrammarAnalysisHelper*)context;
		inst->m_console.ProcessWindowsMessages();
	}

	TGenericConsoleEx		&m_console;
	DWORD				m_tracing_mask;

};

struct TGrammarConflictAnalysisTask
{
	int				m_only_igrammar;				//	-1	-	process all grammars in the tree of grammars.
	int				m_only_iconflict;				//	-1	-	process all conflicts of the grammar.
													//	-2	-	process conflicts from the list of grammar conflict indexes.
													// -3-ixpct -	process all grammar conflicts that belong to one xpct conflict.
	int				m_only_iaction;					//	-1	-	process all conflicting actions of the grammar conflict.

	TSymbolsArray	m_cfct_indexes_list;				// Array of the grammar conflict indexes.

	void SetDefaultValues()
	{
		m_only_igrammar = m_only_iconflict = m_only_iaction = -1;
		m_cfct_indexes_list.Clear();
	}

	void TakeDataFrom(TGrammarConflictAnalysisTask &other_inst)
	{
		m_only_igrammar = other_inst.m_only_igrammar;
		m_only_iconflict = other_inst.m_only_iconflict;
		m_only_iaction = other_inst.m_only_iaction;
		m_cfct_indexes_list.TakeContentsFrom(other_inst.m_cfct_indexes_list);
	}
};

struct TGrammarConflictAnalysisProps
{
	bool			m_use_all_non_term_defn_rules;
					//
					// When this option is TRUE, the rules like
					//
					//   NonTerm: NonTerm a B c D;
					//
					// are processed as regular rules. This potentially allows finding more path contexts
					// but this dramatically slows down the search.
					//

	bool			m_search_for_non_empty_ctx;
					// This option enables additional search if a conflicting path has empty context
					// at its beginning or at its end.

	int			m_max_path_length;										// Max len of the path in steps.
					// Processing code will adjust the value  to fit into
					// the range 4 ... MAX_DERIV_PATH_LEN.

	int			m_max_num_paths;
					// When this number of derivation paths is discovered,
					// the processing of the conflicting action is aborted.

	int			m_action_proc_timeout;									// Timeout in seconds.
					// Processing code will adjust the value to fit into
					// the range from 1 second to 1 year. A zero value means
					// a one year timeout.

	void SetDefaultValues()
	{
		m_use_all_non_term_defn_rules = FALSE;
		m_search_for_non_empty_ctx = FALSE;
		m_max_path_length = 0;					// No restriction on the paths length.
		m_max_num_paths = 0;					// No restriction on the number of paths.
		m_action_proc_timeout = 400;				// 6 min 20 sec.
	}
};

extern int g_ConflictAnalysisHelper_SlotsHashingGroupType[];

class TGrammarConflictAnalysisHelper : public TBasicGrammarAnalysisHelper
{
public:

	//
	//  Note on the rule action symbols iteration.
	//
	//  During the iteration positions with different action symbols are treated as different. Only when rule start
	//  states are iterated, the action sym restriction is used because this iteration always starts from the end of
	//  the rule and action symbol of the current position is used as iteration restriction.
	//
	//  When paths are saved, only irule/isym/step_origin are compared. This means that paths that differ only
	//  in action symbol and even the parsing state of the steps, will be treated as similar.
	//

	TGrammarConflictAnalysisHelper(TGenericConsoleEx &console, DWORD tracing_mask);

	void		ResetMajorFields();
	void		SetProps(TGrammarConflictAnalysisProps &props) { m_analysis_props = props; }
	bool		SetGrammarAndPrepareIndexes(TGrammar *root_grammar, int igrammar);

	bool		ProcessConflicts(TGrammar *root_grammar, int igrammar = -1, int iconflict = -1, int iaction = -1);
				// This method builds indexes for the requested grammar and rebuilds them if several grammars are involved.
				// The return value is TRUE when there were no processing errors at all.

	bool		ContinueCurrentProcessing();
				// This method expects that object already contains processing props, grammar to process and interrupted state
				// of processing as it was left either from prior processing or after reading the XML data.

	enum { MAX_DERIV_PATH_LEN = 4096 };

protected:

	enum { DERV_PATHS_HASH_LEN = 1024 };			// The value should be a strict power of 2.
	enum { PATH_SLOTS_HASH_LEN = 1024 };			// The value should be a strict power of 2.
	enum { PATH_SLOTS_HASH_AREAS = 24 };			// Any reasonable value.

	struct TPathSlotsHashingArea
	{
		int							m_slots_hash_table[PATH_SLOTS_HASH_LEN];
		int							m_bottom_slot_index;
											// This hash area describes sequence of slots starting from m_bottom_slot_index
											// up to m_clev-2.
	};

	bool		GeneratePathsForGrammar(int igrammar, int iconflict = -1, int iaction = -1);
	bool		GeneratePathsForConflict(int iconflict, int iaction = -1);
	bool		GeneratePathsForAction(int iaction);

	void		PrepareHashTables();
	void		PrepareMajorLoop();
	void		DoMajorLoop();
	void		DoExtraCtxLoop();

	bool		ProcessDerivationStep();
	bool		InstantiateDerivationPath();
	void		AddStepOverCtxOrUpCtxSlot(int ilayer);
	void		AddStartStateCtxSlot();
	bool		SearchForMoreContext();

	bool		SaveDerivationPath(bool ctx_slots_present);
				// Before saving the path this method checks, whether the current path is already known or not.
				// If the curr path is known, this method will only bump the cnt_similar counter in the path and return TRUE.
				// There are 2 similarity checks. First, the path is checked as-is among all existing paths. Second check
				// applies only to paths that have ctx slots and those ctx slots have redundant loops. Algorithm tries to find
				// saved path that does not have triple loops that the current path has. If such path is present, the current
				// path is not saved.

	bool		CheckStepsOrgCtx(int istep) { return(m_curr_path[istep].m_frp.origin == rpo_call_place_ctx && m_curr_path[istep+1].m_frp.origin == rpo_step_over_ctx && m_curr_path[istep+2].m_frp.origin == rpo_start_state_ctx); }
	bool		CheckStepsPosCtx(int is1, int is2) { return(m_curr_path[is1].m_frp.dwHeader == m_curr_path[is2].m_frp.dwHeader && m_curr_path[is1+1].m_frp.dwHeader == m_curr_path[is2+1].m_frp.dwHeader && m_curr_path[is1+2].m_frp.dwHeader == m_curr_path[is2+2].m_frp.dwHeader); }
				// These two methods are helper methods that are used only in SaveDerivationPath() for finding redundant loops.

	inline void AddToSlotsHashTable(int slot_index)
	{
		TBuildGrammarDerivationSlot &slot = m_curr_path[slot_index];

		// There are 2 groups of the derivation step origins: The groups of steps that are going up the AST tree.
		// This group is called group 0. The other group travels inside the growing down subtree. This is group is
		// called group 1. These groups strictly interchange each other. This means that the last bit of the hashing
		// areas count shows if current step should be appended to the current area or it should start the new area.
		if (g_ConflictAnalysisHelper_SlotsHashingGroupType[slot.m_frp.origin] != (m_slots_hashing_areas_index & 1))
		{
			// Start the new slots hashing area.
			m_slots_hashing_areas_index++;
			HardAssert(m_slots_hashing_areas_index < PATH_SLOTS_HASH_AREAS, L"Derivation path slots HASHING AREAS overflow. This situation should never happen.");
			m_slots_hashing_areas[m_slots_hashing_areas_index].m_bottom_slot_index = slot_index;
		}

		// Add slot to the current hashing area.
		TPathSlotsHashingArea &area = m_slots_hashing_areas[m_slots_hashing_areas_index];
		int slot_hash = (slot.m_frp.irule ^ slot.m_frp.istate) & (PATH_SLOTS_HASH_LEN-1);
		m_next_same_hash_slot[slot_index] = area.m_slots_hash_table[slot_hash];
		area.m_slots_hash_table[slot_hash] = slot_index;
	}

	inline void RemoveFromSlotsHashTable(int slot_index)
	{
		TBuildGrammarDerivationSlot &slot = m_curr_path[slot_index];
		int slot_hash = (slot.m_frp.irule ^ slot.m_frp.istate) & (PATH_SLOTS_HASH_LEN-1);

		// Pop the slot out of the current hashing area.
		TPathSlotsHashingArea &area = m_slots_hashing_areas[m_slots_hashing_areas_index];
		area.m_slots_hash_table[slot_hash] = m_next_same_hash_slot[slot_index];

		if (slot_index <= area.m_bottom_slot_index)
		{
			// Abandon the current hashing area. Note that when the last slot will be removed from
			// the area[0], the count of hashing areas will become -1. This is not a problem because
			// the code even allows to put the same slot back into the hash.
			m_slots_hashing_areas_index--;
		}
	}

	int		LookForIdenticalSlot();

	void		ClearStatistics();
	int		GetMaxTransientPathLength();
	int		GetMaxTransientSlotsHashingAreas();

	bool		CheckForAbortCases();
				// When return value is TRUE, this means that one of the abort cases is detected. In this case state of the object is already changed eiter
				// to complete or to partial termination of iteration. When return value is FALSE, iteration can be continued.

	void		ReportTooDeepDerivationPath(bool extra_context = FALSE);

protected:

	void		TraceOverallProgress(bool want_empty_line);
	void		TraceCurrentState(int max_slots = MAX_DERIV_PATH_LEN);
	void		TraceSlotLayerSetupCounts(const wchar_t *method_prefix);
	void		TraceFinalStatistics(TDateTime dt_start, bool overall_result, const wchar_t *method_prefix);

protected:

	enum
	{
		PROGRESS_REPORT_FREQ =
							#ifdef _DEBUG
								0x300000
							#else
								0x2900000
							#endif
	};

	enum
	{
		DEEP_NEST_REPORT_FREQ =
							#ifdef _DEBUG
								36000
							#else
								4230000
							#endif
	};

	TGrammar						*m_root_grammar;
	TGrammar						*m_curr_grammar;

	TRulePosIterIndexLevel2			m_rule_start_states_index;
	TRulePosIterIndexLevel2			m_non_term_defn_rules_index;
	TAnalysisTable					m_firsts_and_follows;
										// All indexes are based on the current grammar and they do not change during
										// the analysis. They are generated using methods of the TGrammar class.

	TGrammar						*m_indexes_grammar;
	DWORD							m_indexes_grammar_hash;
										// These fields are used for avoiding rebuilding indexes between processing
										// different parsing actions.

	int								m_igrammar;						// Index of the current grammar in the tree of grammars.
	int								m_iconflict;
	int								m_iaction;

	WORD							m_curr_conflicting_symbol;			// Copy of the grammar conflict structure field.

	TConflictingActionAnalysisResults		*m_result;							// Pointer into the grammar structure field.

	TStructsArray<int, 1024, 1024>		m_next_same_hash_path;
	int								m_paths_hash_table[DERV_PATHS_HASH_LEN];

	TBuildGrammarDerivationSlot		m_curr_path[MAX_DERIV_PATH_LEN];

	int								m_next_same_hash_slot[MAX_DERIV_PATH_LEN];
	TPathSlotsHashingArea				m_slots_hashing_areas[PATH_SLOTS_HASH_AREAS];
	int								m_slots_hashing_areas_index;
										//
										// This hash table contains information about all occupied slots except for the last one.
										// Last item should not be present in the hash because after calling slot.GetNextPosition()
										// on the tip slot of the stack, the rest of the stack is searched for identical position.
										// If such position is found, the loop goes on without processing the current tip position.
										//
										// Index of the hashing areas is pointing inside the m_slots_hashing_areas array. This means
										// that its value is the number of occupied areas minus one.
										//

	int								m_clev;								// Number of occupied slots in the stack.

	__int64							m_cnt_steps;						// Count of the major loop rotations.
	DWORD							m_iter_setup_cnt;					// This is an aux count that is used to mark the derivation
																		// slots for the purpose of tracking the progress of iteration.
	int								m_saved_clev;						// This field is used for temp storing m_clev when processing
																		// is stopped by timeout or the abort signal from console.
	int								m_clev_bottom;
	bool								m_want_backw_ctx;
	bool								m_want_forw_ctx;
	bool								m_ctx_paths_present;
										// These 4 fields are used when extra context is searched.

	TGrammarConflictAnalysisProps		m_analysis_props;

	int								m_index_creation_errors;
	int								m_anres_allocation_errors;
										// Statistics fields. These fields are used only for reporting.

	int								m_cnt_actions_processed;
	int								m_action_processing_results[TConflictingActionAnalysisResults::gres_num_types];
	__int64							m_num_too_deep_truncations;
	__int64							m_num_partial_iter_aborts;
										// Statistics fields. These fields are used only for reporting.

	int								m_progress_report_cnt;
	int								m_deep_nest_report_cnt;
	TDateTime						m_timeout_date_time;
										// These fields control frequency of sending trace messages to console and the time,
										// allocated for the whole processing.

	friend class TSavedConflictingActionAnalysisStateXmlWriter;
	friend class TSavedConflictingActionAnalysisStateXmlReader;
	friend class TGrammarConflictAnalysisDriver;
	friend class TBuildGrammarDerivationSlotTest;
};

class TDerivationPathsArraySimplificationHelper
{
public:

	static void	RemovePathsWithRedunantSteps(TConflictingActionAnalysisResults *results_set);

protected:

	static bool	CheckStepsTriple(TFullRulePosition *steps, int istep)
						{ return((steps[istep].origin == rpo_rule_start_state || steps[istep].origin == rpo_initial_placement) && steps[istep+1].origin == rpo_rule_call_place && steps[istep+2].origin == rpo_step_over_symbol); }
	static bool	CheckParsingStates(TFullRulePosition *steps, int is1, int is2)
						{ return(steps[is1].istate == steps[is2].istate && steps[is1+1].istate == steps[is2+1].istate && steps[is1+2].istate == steps[is2+2].istate); }
	static bool	CheckRuleSymbolPos2(TFullRulePosition *steps, int is1, int is2)
						{ return(steps[is1+1].dwHeader == steps[is2+1].dwHeader && steps[is1+2].dwHeader == steps[is2+2].dwHeader); }
	static bool	CheckRuleSymbolPos3(TFullRulePosition *steps, int is1, int is2)
						{ return(steps[is1].dwHeader == steps[is2].dwHeader && steps[is1+1].dwHeader == steps[is2+1].dwHeader && steps[is1+2].dwHeader == steps[is2+2].dwHeader); }

};

class TDerivationPathPostProcessingHelper : public TBasicGrammarAnalysisHelper
{
public:
			TDerivationPathPostProcessingHelper(TGenericConsoleEx &console, DWORD tracing_mask)
						: TBasicGrammarAnalysisHelper(console, tracing_mask) { }

	void		RemoveRedundantCtxAnalysisSteps(TGrammar *grammar, int iconflict, int iaction);

protected:

	bool		CheckStepsOriginCtx(TFullRulePosition *steps, int istep)
						{ return(steps[istep].origin == rpo_call_place_ctx && steps[istep+1].origin == rpo_step_over_ctx && steps[istep+2].origin == rpo_start_state_ctx); }
	bool		CheckStepsPositionCtx(TFullRulePosition *steps, int is1, int is2)
						{ return(steps[is1].dwHeader == steps[is2].dwHeader && steps[is1+1].dwHeader == steps[is2+1].dwHeader && steps[is1+2].dwHeader == steps[is2+2].dwHeader); }
};

class TDerivationPathAnalysisHelper : public TBasicGrammarAnalysisHelper
{
public:
			TDerivationPathAnalysisHelper(TGenericConsoleEx &console, DWORD tracing_mask, TGrammar *grammar)
						: TBasicGrammarAnalysisHelper(console, tracing_mask) { m_grammar = grammar; }
public:

	// The key rule position is one of the steps of the derivation path. It reveals the sift/shift nature of the parsing conflict.
	struct TKeyRulePosition
	{
		WORD		key_irule;
		WORD		key_isym;
		int			cnt_refs;

		static int	Compare(TKeyRulePosition &pos1, TKeyRulePosition &pos2);
					//
					//   Ret val:	 1:	pos1  >  pos2
					//			 0:	pos1 == pos2
					//			-1:	pos1  <  pos2
					//
	};

	// Items in this array are sorted in the increasing order of the pair <irule, isym>.
	struct TKeyRulePositionsArray : public TStructsArray<TKeyRulePosition, 4, 16>
	{
		bool		IntegrateKeyRulePos(WORD irule, WORD isym);
					// Items should be added to the array only with this method. This method adds new record
					// if needed and it maintains the right sort order of the records.

		bool		FindKeyRulePos(TKeyRulePosition &key_pos, int &ins_before_inx);
					// This method is not called from outside of the outer structure.
	};

public:

	// The origin of the conflict context symbols sequence. The conflict context sequence is either a sequence of
	// predecessor symbols or a sequence of followers. These sequencies can be built for every derivation path.
	// Derivation path is called origin of the context sequence if this path generates this sequence.
	struct TConflictContextSeqOrigin
	{
		WORD		iconflict;
		WORD		iaction;
		int			ipath;
	};

	typedef  TStructsArray<TConflictContextSeqOrigin, 16, 256>  TConflictContextSeqOriginsArray;

	struct TConflictContextSeq
	{
		TSymbolsArray					seq;
		TConflictContextSeqOriginsArray		origins;		// This is actually a list of the derivation path references.

		inline void FixObjectMove()	{ seq.FixObjectMove(); origins.FixObjectMove(); }
		inline void ReleaseObject()	{ seq.ReleaseObject(); origins.ReleaseObject(); }
		inline void WipeObject()		{ seq.WipeObject(); origins.WipeObject(); }
	};

	//
	//  This is an array of conflict context sequencies. Each sequence is accompanied with the list of derivation paths
	//  that generate this seqinece. This array expects that its items are sorted.
	//
	struct TConflictContextSeqsArray : public TStructsArrayEx2<TConflictContextSeq, 16, 256>
	{
		bool		IntegrateSymbolsSeq(TSymbolsArray &seq, WORD ic, WORD ia, int ip);
		bool		IntegrateSymbolsSeq(TSymbolsArray &seq, TConflictContextSeqOriginsArray &orgs);
					// Both methods work fine if passed seq is unknown and when it is already present in the array.
					// In both cases it is expected that passed origins are not present in the data structure.

		void		SortInPredecessorsStyle() { QuickSort(CompareContextSeqs); }

	private:

		static int  __cdecl	CompareContextSeqs(const TConflictContextSeq *pseq1, const TConflictContextSeq *pseq2);

		bool		MergeSymbolsSeq(int &ins_before_inx, TSymbolsArray &seq);
		bool		FindSymbolsSeq(TSymbolsArray &seq, int &ins_before_inx);
	};

public:

	// Origin of the conflict context record.
	struct TConflictContextOrigin
	{
		WORD		iconflict;
		WORD		iaction;

		static int __cdecl	Compare(const TConflictContextOrigin *org1, const TConflictContextOrigin *org2);
	};

	typedef  TStructsArray<TConflictContextOrigin, 16, 256>  TConflictContextOriginsArray;

	struct TConflictContextRecord
	{
		TConflictContextOriginsArray		origins;
										// Origins are not used when the record is describing the context of the conflicting action.
										// They are important only for describing context of the expected conflict.

		bool							new_origins;
										// This flag is used when the context record is part of array that describes the context of
										// the expected conflict and this array was generated using "Merge == No" mode.
										// When this flag is TRUE, it indicates that the current record has different set of origins
										// than the previous record.

		TKeyRulePositionsArray			key_positions;

		TConflictContextSeqsArray		predecessors;
		TConflictContextSeqsArray		followers;

		inline void FixObjectMove()	{ origins.FixObjectMove(); key_positions.FixObjectMove(); predecessors.FixObjectMove(); followers.FixObjectMove(); }
		inline void ReleaseObject()	{ origins.ReleaseObject(); key_positions.ReleaseObject(); predecessors.ReleaseObject(); followers.ReleaseObject(); }
		inline void WipeObject()		{ origins.WipeObject(); key_positions.WipeObject(); predecessors.WipeObject(); followers.WipeObject(); }

		void		TakeContentsFrom(TConflictContextRecord &rec);
		bool		IntegrateSequencies(TConflictContextRecord &rec);

		static int	CompareOriginArrays(TConflictContextRecord &rc1, TConflictContextRecord &rc2);
	};

	struct TConflictContextRecordsArray : public TStructsArrayEx2<TConflictContextRecord, 16, 256>
	{
		bool		IntegrateKeyRulePos(int &irecord, WORD irule, WORD isym, WORD iconflict = 0xFFFF, WORD iaction = 0xFFFF);
		void		SortByOriginSetsAndKeyRulePositions();
					// These methods expect that all records of the array have only one key rule position.

		bool		FindRecordWithKeyRulePos(TKeyRulePosition &key_pos, int &ins_before_inx);
					// This method is not called from outside of the structure.
	};

public:

	//
	//  Public API.
	//

	bool		GetDerivationPathContext(TSymbolsArray &buf_predecessors, TSymbolsArray &buf_followers, int iconflict, int iaction, int ipath);
				// Method expects that requested path is present in the grammar. Method simply converts path into two sequencies of grammar symbols.
				// On successful exit the buf_followers is always non empty because it always starts with a conflicting symbol.

	bool		GenerateConflictingActionContext(TConflictContextRecordsArray &buffer, int iconflict, int iaction, bool merge_different_key_rule_seqs);
				// Depending on the value of the "merge" parameter this method will either generate an array that will contain only
				// one record or generate an array of several records where each record will have only one key rule position.
				// In both cases the origin fields will be empty because all data belongs to the same conflicting action that was requested.

	bool		GenerateExpectedConflictContext(TConflictContextRecordsArray &buffer, int ixpct, bool merge_different_key_rule_seqs);
				// This method generates buffer that always may contain several records regardless of the value of the merge_different_key_rule_seqs
				// parameter.
protected:

	bool		AppendRuleTrailingSymbols(TSymbolsArray &buf, TFullRulePosition &pos, int sym_offs = 0);
	bool		MergeConflictingActionContext(TConflictContextRecordsArray &buffer, TConflictContextRecordsArray &contexts, int iconflict, int iaction);

	static int		CompareSymbolArrays(TSymbolsArray &a1, TSymbolsArray &a2);
					//
					//   Ret val:	 1:	a1  >  a2
					//			 0:	a1 == a2
					//			-1:	a1  <  a2
					//
protected:

	TGrammar		*m_grammar;
};

//
// This iterator looks for parsing state and symbol pairs that generate shift into the given destination state.
//
struct TPreceedingStatesIterator
{
	WORD		m_state;
	WORD		m_symbol;
	int			m_cnt_alternatives;

	bool		GetFirstPreceedingState(TGrammar *grammar, WORD dest_state);
	bool		GetNextPreceedingState(TGrammar *grammar, WORD dest_state);
				// When the return value is TRUE, all fields of the structure contain meaningful values.
private:

	bool		GetPreceedingState(TGrammar *grammar, WORD dest_state, WORD curr_state, WORD curr_symbol);
};

typedef TStructsArray<TPreceedingStatesIterator, 2, 32> TPreceedingStateIteratorsArray;

class TParsingStateContextsAnalysisHelper
{
public:
			TParsingStateContextsAnalysisHelper(TGenericConsole &console) { m_final_state = 0xFFFF; m_cnt_slots = 0; m_grammar = NULL; }

	bool		PrepareIteration(TGrammar *grammar, WORD dest_state);
				// The return value FALSE indicates a memory allocation failure.

	bool		GetFullTreeContext();
	bool		GetLimitedTreeContext();
				// The return value shows if a first/next context was discovered or not.

	int		GetCurrSeqLen() const { return(m_cnt_slots); }
	WORD	GetCurrSeqStepState(int inx) const { return(m_curr_seq[inx].m_state); }
	WORD	GetCurrSeqStepSymbol(int inx) const { return(m_curr_seq[inx].m_symbol); }

protected:

	bool		FindDuplicatedPair(TPreceedingStatesIterator &slot);

	WORD							m_final_state;

	TPreceedingStateIteratorsArray		m_curr_seq;
	int								m_cnt_slots;

	bool								m_full_path_discovered;
	int								m_max_single_alt_layer;

	TGrammar						*m_grammar;
};

#endif	// TextScan_GrammarAnalysis_H


