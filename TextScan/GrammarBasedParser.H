//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Backus-Naur productions grammar processor.
//

#ifndef	TextScan_GrammarBasedParser_H
#define	TextScan_GrammarBasedParser_H

#ifndef   Common_LookAsideTable_H
#include  "Common/LookAsideTable.H"
#endif
#ifndef   TextScan_GrammarStructure_H
#include  "TextScan/GrammarStructure.H"
#endif
#ifndef   TextScan_ParsingDriver_H
#include  "TextScan/ParsingDriver.H"
#endif

// ----------  Part 5. Target language parser  -------------

//
//  App level parser can either use this structure directly or it can create derived structure
//  for keeping its own information about the symbol.
//
class TSymbolStateItem : public TListItem
{
public:

	TSymbolStateItem() { m_parsing_state = m_symbol = 0xFFFF; m_dbg_data = m_app_data_ptr = NULL; }
				// The m_lex field is inited with default ctor.

	~TSymbolStateItem() { assert(m_dbg_data == NULL); }

	virtual bool GetAppLayerDescription(TDestinationFile &doc) { return(FALSE); }
				// This method may return multiline description. There is one non obvious aspect in the implementaion of this method.
				// When the grammar rule is processed, the app layer tends to take the app layer data from the symbols of the rule and
				// move this data to the non terminal of the rule that is being constructed. Nevertheless these rule symbols are still part
				// of the parser stack. Application should not destroy state of these rule symbols completely at least to extent that
				// the method above should still return the same description for these "partially destroyed" symbols.

	PrEventHeader	&GetOriginHeader() { return((m_symbol < NonTerminalsBase) ? m_lex.origin : m_non_term_origin); }

public:

	WORD			m_parsing_state;	// Parsing state has meaning only when the object is part of the parser stack.
										// The object can also reside in the queue of the scanned symbols.

	WORD			m_symbol;			//
										// Symbol code in the grammar notation.
										//
										//		 0		--	ltx_eof;
										//	        i		--	terminal;
										//	    4000+j	--	non terminal;
										//
										//  --- Values below can be used only inside the ReadSymbol() method ----
										//
										//	    3500+m	--	ignore lexema;
										//	    3800+n	--	error lexema with app msg;
										//	    0xFFFF		--	unmappable lexema.
										//
												//
	TLexema				m_lex;					// This field contains lexema when value of the m_symbol field belongs to the range
												// of terminal symbol values. In fact, the lexema field and the info about the non terminal
												// origin are not used at the same time, but it is not possible to use union because lexema
												// and one of its fields have a copy constuctor.
												//
				//
				// It would be logical to fill in the origin of the non terminal in the base parser class. Nevertheless in the current
				// design this is not so because AllocateSymbolStateItem() allocates the struct and makes the app level processing
				// for the non term in the same call. Since filling the origin should be done at the very beginning, this is done in
				// the application code.
				//
												//
	PrEventHeader	m_non_term_origin;			// Notes on the values of the fields of this structure:
												//
												//	  cn			--	Always zero.
												//	  source_ref	--	Ref of the beg of the non terminal area.
												//	  cond_ref	--	Id of the cond pars area at the non term beg.
												//	  lin_area		--	Always correct linear area.
												//	  src_area	--	Area at the beginning of the non term.
												//

	TListItem			*m_dbg_data;		// Pointer to the debugger data or NULL. TGenericParserConsole is responsible
										// for handling this field. In regular parsing session the value of the field is NULL.

	// Derived class can keep its app layer specific symbol state in the union below.
	// Note that only application layer sets up and changes this union, including the m_app_id field case.
	union
	{
		TListItem		*m_app_data_ptr;
		int			m_app_data_int;

		struct
		{
			WORD		m_app_id;
			WORD		m_sym_meaning_ex;
		};
	};

};

#ifdef WANT_GRAMMAR_USE_INFO

struct TParsingStateUseInfo
{
	int		m_stt_cnt_use;
};

struct TGrammarRuleUseInfo
{
	int		m_rule_cnt_use;
};

struct TGrammarUseInfo
{
};

#endif

//
//  This enum defines bits in the trace mask of the mid lev scanner. The HI byte in the trace mask of
//  the Mid Lev scanner is dedicated to the generic parser and to the application level parsers.
//
enum TGrammarBasedParserTraceFlags
{
	gptr_all					= 0xFF000000,
	gptr_all_non_verbose	= 0x7F000000,

	gptr_start_stop			= 0x01000000,		// 3 places.

	gptr_symbol_pickup		= 0x02000000,		// 2 places.
	gptr_symbol_update		= 0x04000000,		// 1 place.
	gptr_symbol_discard		= 0x08000000,		// 1 place.

	gptr_action_pickup		= 0x10000000,		// 2 places (pickup+Enter/ExitNesting).
	gptr_action_update		= 0x20000000,		// 1 place.
	gptr_error_recovery		= 0x40000000,		// 3 places.

	gptr_verbose			= 0x80000000,		// This flag adds current state of the parser to all tracing messages.

		#define TPRF_GRAM_BASED_PARS L"ParserCore"
};

struct TParsingLayerInfo
{
	TGrammar				*m_layer_grammar;					// Grammar of the suspended layer.

	TGrammarConflict			*m_layer_conflict;					// Conflict that caused the suspension.

	TList						m_pars_stack;
	int						m_pars_cstk;

	WORD					m_pars_state;						// Parsing state as it was when parsing was interrupted by the conflict resolution.

	TList						m_input_queue;						// Input queue of the suspended layer.

	TList						*m_upper_layer_input_queue;
	TSymbolStateItem			*m_upper_layer_next_symbol;
	int						m_upper_layer_nesting_index;

	TSymbolStateItem			*m_preview_latest_processed_symbol;
	int						m_preview_latest_processed_index;
};

class TGrammarBasedParser : public TGenericParser
{
public:

	TGrammarBasedParser(TGrammar *grammar);
	~TGrammarBasedParser() { ResetParser(); }

	TGrammar	*RootGrammar() const { return(m_root_grammar); }
	void			SetRootGrammar(TGrammar *grammar) { assert(m_cstk == 0); assert(m_nest_cstk == 0); m_root_grammar = m_grammar = grammar; }

	//
	// Inherited virtual methods, common to all generic parsers.
	//

	enum { ParserIdentCode = 0x317 };

	virtual WORD		GetParserIdentificationCode() const { return(ParserIdentCode); }
						// This method is not expected to be overwritten by the derived classes. It shows that parser
						// is a grammar based parser. This method should not provide extra details.

	virtual bool		SetupParserConsole(TGenericParserConsole *console) { m_parser_console = console; return(TRUE); }
						// It is important that once this method is overwritten, it should return TRUE. This means that this
						// parser supports the parser console functionality.

	virtual void		ResetParser();
	virtual bool		Parse();

	//
	// New virtual methods.
	//

	virtual THighLevelLangType	GetParserLangType() const { return(lang_none); }
								// Return value should describe type of the language of the callback handler, not the language of
								// the current grammar that might be wrong or corrupted.

	virtual const wchar_t		*GetParserDescription(wchar_t *buff_80_chars) const { return(L"Stub"); }
								// Method should return the human readable description of the parser. Return value should be
								// either a static pointer or it should point into the passed buffer.

	virtual bool				CheckGrammar() const { return(TRUE); }
								// This method is typically called at the beginning of parsing.  It should verify that the grammar
								// is present, that it does not contain errors and that it is compatible with the particular app level
								// parser.
protected:

	enum { LOOK_ASIDE_LIST_LEN = 256 };
	enum { PARSER_STACK_MAX_LEN = 50000 };
	enum { MAX_GRAMMAR_NESTING = 20 };

	typedef  TLookAsideTable<TSymbolStateItem, LOOK_ASIDE_LIST_LEN>  TStateItemsLookAside;

	virtual void ReviewInputSymbol(TSymbolStateItem *item) { }
	virtual void ReviewParsingAction(WORD &action) { }
				//
				//  These are two out of three major callbacks that derived class should implement.
				//

		//
		//  Derived class can use the TSymbolStateItem for keeling state of the symbol or it can use its own derived class. Since symbol
		//  state items are constantly allocated/recycled on every step of the parsing process, it is highly recommended to use lookaside
		//  lists. Note that derived class is responsible for initing/cleaning up the m_app_data field of the TSymbolStateItem.
		//
		//  When AllocateSymbolStateItem() is called with param not equal to 0xFFFF this means that grammar rule is about to be applied.
		//  At this point decision to apply the rule cannot be changed. The rule will be applied. Application should use this callback to do
		//  the app specific work for the non terminal that is being created.
		//

	virtual TSymbolStateItem *AllocateSymbolStateItem(WORD irule = 0xFFFF) { return(m_look_aside_list.GetItem()); }
				//
				//  This is the third major callback that derived class should implement. Derived class can return objects of some
				//  app layer class that is derived from TSymbolStateItem. Every app layer parser should overwrite this method.
				//  Default implementation is provided just to make the base class functional.
				//

	virtual void RecycleSymbolStateItem(TSymbolStateItem *item)  { m_look_aside_list.RecycleItem(item); }
				// Derived class should do the app layer cleanup in this method.

	TSymbolStateItem *PickUpRecycledSymbolStateItem() { return(m_look_aside_list.GetItemNoAlloc()); }
				// Method returns instances that were recycled to the look aside list. All these instances at some point were
				// returned by the AllocateSymbolStateItem() method. Other instances cannot be returned.

	void		SetAppNote(const wchar_t *msg);
				// Derived class may call this method from any of the 4 app callback methods above. Primary purpose of this
				// method is to give explanation on why derived class decided to change the parsing action or the symbol value
				// of the symbol that was picked from the input stream.

	void		InsertInputQueueSymbol(int ins_before_inx, TSymbolStateItem *sym_state_item);
	void		UpdateInputQueueSymbol(int inp_symbol_inx, WORD new_terminal_symbol_value);
	void		RemoveInputQueueSymbol(int inp_symbol_inx, TSymbolStateItem **removed_sym_state_item);
				// All these methods are used by the derived class to recover from the syntax errors. If derived class will not
				// recover from the syntax error, then core parser will issue the critical error and terminate.

	WORD	GetParsingActionForInpSym(int inp_symbol_inx);
				// This method searches for the object in the input queue and returns the parsing action according to the analysis table.
				// It is not doing any other work. This method is helpful for the error recovery code.

	ID					GetParsingId() { return(m_scanner->CbkHandler().GetParsingId()); }
	PrEventHeader		*GetCurrentLocation();
							// Search for the leftmost lexema in the input queue.

	TSymbolStateItem		*GetStackSlotPtr(int one_based_index_from_right_side);
	TSymbolStateItem		*GetStackSlotPtr(TGrammarRule &rule, int zero_based_rule_sym_index) { return(GetStackSlotPtr(rule.Length()-zero_based_rule_sym_index)); }
							// Param value 1 corresponds to the rightmost slot in the parsing stack. Parameter value 0 results
							// in returning the NULL value.

	TSymbolStateItem		*GetInpSymPtr(int zero_based_inp_symbol_index);
	TSymbolStateItem		*GetInpSymPtrEx(int zero_based_inp_symbol_index);
							// Second method reads symbols from the source if the requested number of symbols is not present
							// in the queue. Both methods can return NULL.

	TSymbolStateItem		*GetSavedStackSlotPtr(int num_right_slots);
	TGrammar			*GetSavedGrammar() { return(m_saved_grammar); }
	TGrammarConflict		*GetSavedConflict() { return(m_saved_conflict); }
							// This group of method allows to inspect the final state of the latest exited nesting parsing.
							// This in necessary for processing the nested result.
private:

	bool		PreprocessParsingAction(TGenericParserConsoleEvent action_origin, WORD &wAction, TSymbolStateItem *&leftmost_input_item);
				// Method traces picked action, shows it to the debugger, allows app layer to review this action. If app layer has changed
				// the action for any reason, method traces this and shows the updated action to the debugger.

	void		PatchLexemaInfoToSymbol(TSymbolStateItem *item, WORD sym);

protected:

	// Ranges of the error code values are specified in TParsingErrorCodeBases enum.
	enum TGrammarBasedParserErrorCode
	{
		err_base = 1400,

		err_pars_msg_success,				// This is the msg code. This is not an error code. It is used
											// for emitting the final message of the persing success.
		err_parsing_aborted,				// Generic error when exact reason for abort is
											// unclear.
		err_missing_grammar,				// Parsing was started and grammar was not set
											// into the parser.
		err_incompatible_grammar,			// Passed grammar is incompatible with instantiated
											// callback handler.
		err_aborted_by_debugger,			// Parsing was aborted from
											// TGenericParserConsole.
		err_critical_oom,					// This may happen when symbol state items are
											// created for lexemas and non terminals.
		err_parser_stack_overflow,			// Parsing stack growth is limited with
											// reasonable constant (50000).
		err_nesting_stack_overflow,			// Nesting of conflict resolution
											// parsings is too deep.
		err_generic_syntax_error,			// Sequence of lexemas in the input file does not
											// meet any rule in the grammar.
		err_unresolved_conflict,				// Application layer has not resolved conflict that
											// has no nested grammar for conflict resolution.
		err_unresvoled_nested_proc,			// Nested processing result was not resolved by
											// the app layer callback.
		err_prohibited_lex_type,				// Grammar explicitly prohibited using this lex type/subt
											// and it has specific error message for this.
		err_unmappable_lex_type,			// Parser is not able to assign symbol value to lexema because its
											// lex type/subtype is missing in the list of the grammar terminals.
		err_max,
	};

	void		ReportParsingMessage(int msg_code, const wchar_t *format, ...);
	void		ReportError(int err_code, const wchar_t *format, ...);
	void		ReportError(TLexema &err_loc_lex, int err_code, const wchar_t *format, ...);

	WORD	ReadSymbol();
				//
				//     This method reads lexema from the source file and puts is to the end of
				//  the input queue regarless if the input queue is empty or not.
				//
				//   Possible return values
				//  ------------------------------------------------
				//       0		- EOF is retrived or it was already present
				//			   in the input queue.
				//      val		- New valid terminal symbol accroding to
				//			   the current grammar.
				//    0xFFFF	- Reading attempt failed. Most likely the critical
				//			   error was raised.
				//

	void		PrepareNonTerminalArea(PrEventHeader &non_term_area, TSymbolStateItem *first_rule_sym);
				// This method collects source areas of the stack symbos starting from the passed symbol
				// and up to the end of the stack.
private:

	TSymbolStateItem *AllocateSymbolStateItemForLexema();

	void		CleanupSymbolStateItemsList(TList &sym_stt_list);
	void		DiscardInputSymbol(TSymbolStateItem *item);

	void		TraceCurrStateIfRequested(const wchar_t *header);
				// When gptr_verbose tracing flag is active, this method emits current state of the parser.

	void		TraceHeader(const wchar_t *header);
	const   wchar_t	*PrepareLexSubtypeEnumName(TSymbolStateItem *item, wchar_t *buff_40_chars);

	inline  void		SendParserConsoleEvent(TGenericParserConsoleEvent event_code, WORD &event_data, DWORD event_data_ex = 0xFFFFFFFF)
						{ if (m_parser_console != NULL) m_parser_console->ProcessEvent(event_code, event_data, event_data_ex); }
protected:

	TGrammar			*m_root_grammar;
	TGrammar			*m_grammar;
							// Converted grammars.

#ifdef WANT_GRAMMAR_USE_INFO
	TGrammarUseInfo		m_root_grammar_use_info;
	TGrammarUseInfo		*m_curr_grammar_use_info;
#endif

	TList					m_stack;
	int					m_cstk;
							// Classical stack of the symbols and states.

	WORD				m_state;
							// Current parsing state.

	TList					m_inp_symbols;
							// List of symbols that are already retrieved from the source file but are not yet consumed by
							// the parsing process. In typical case this list will contain just one element. Complex grammars
							// can read more symbols to resolve ambuguities or do the error recovery. This list may also
							// temporarily keep the non terminals after applying the rules.

	TList					*m_ulayer_input_queue;
	TSymbolStateItem		*m_ulayer_next_symbol;
	int					m_ulayer_nesting_index;

	//
	// This group of fields describes state of taking symbol from the input queues of the interrupted
	// layers to the input queue of the current layer.
	//

	TList					m_symbols_preview_queue;
							// This list is used when new symbols are retrieved from the input stream by nested grammars.

	TSymbolStateItem		*m_spq_latest_retvd_symbol;
	int					m_spq_latest_retvd_index;
							// Pointer and index into the list of preview symbols.

	TParsingLayerInfo		m_nest_stk[MAX_GRAMMAR_NESTING];
	int					m_nest_cstk;
							// Stack of suspended parsings. When the root grammar is active, this stack is empty.

	TSymbolStateItem		*m_non_term_proto;
							//
							// This object is allocated before calling the console on reduce action. At this point this
							// symbol is not yet added to the parsing stack or the input symbols queue (depending on
							// the DIRECT_GOTO_ON_REDUCE_ACTION definition).
							//
							//   NB:	If parsing of the whole program terminates with success, this data
							//		field contains the axioma of the grammar.
							//

	wchar_t				m_app_callback_note[1024];
							// This buffer is cleared before every call of the application callback. The upper layer may
							// write to this buffer some explanation of why this or that decision was taken. This message
							// will be displayed in the parsing debugger if it is attached.

	TList					m_saved_stack;
	TGrammar			*m_saved_grammar;
	TGrammarConflict		*m_saved_conflict;
							// When any nested parsing is exited, its stack, grammar and conflict are not immediately
							// discarded, but they are moved to these data fields. Data stays in this field either until
							// the next exit will happen or the parser will be reset. This allows the app layer to inspect
							// the result of the nested parsing. Saved grammar and saved conflict are not owned.

	TStateItemsLookAside	m_look_aside_list;
							// This is a look aside table for the symbol state objects. Objects in this array should be ready
							// for freeing. Derived class can use this array if it implements its own Allocate/Recycle
							// SymbolStateItem() methods.

	friend class TLangViewVisualParserFrame;
	friend class TLangViewVisualParserLoadPanel;
	friend class TLangViewVisualParserConsoleObject;
};

#endif	// TextScan_GrammarBasedParser_H


