//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Backus-Naur productions grammar processor.
//

#ifndef	TextScan_GrammarStructure_H
#define	TextScan_GrammarStructure_H

#ifndef   TextScan_MidScan_H
#include  "TextScan/MidScan.H"
#endif

struct TGrammar;

// ----------  Part 1. Symbols of the grammar  -----------

//
//     Terminal symbols
//  ----------------------
//
//  Terminal symbols of the target language can be only distinguished by their lex_type and lex_sybtype.
//  For example new keywords should be added to the dict of tokens in the scanner. There is no other way
//  to do this on the grammar level. This means that it is necesary to modify scanner to add new types
//  of terminals.
//
//  Terminals can appear in the grammar definition either as names or as character constants.
//  In both cases they should be explicitly defined as terminal symbols before they can be used
//  in the grammar rules. Ascii and unicode charconsts are identical in the source of the grammar.
//
struct TTerminalSymbol
{
	// Symbol representation in the grammar definition.
	bool						sym_type_name;		// FALSE - charconst, TRUE - name.
	__int64					char_value;				// Value of the charconst in UNICODE.
	wchar_t					*name_value;			// This pointer can point either to static memoty or to the heap allocation.
													// This cannot be determined from the structure itself.
	WORD					app_id;					// Application id, assigned with modifier __id(n).
													// Value of the field is zero when the app id is not set.

	// Representation of the symbol in the target language.
	TLexemaType				lex_type;
	bool						check_subt;
	TLexSubtype				lex_subt;

	ID						event_id;				// Id of the parsing database event with description of the symbol intro area.

	// Misc fields.
	int						num_rule_uses;			// Terminal symbols can be present only in the rule right hand sides.

	// Note: These methods should not be called when the array of terminal symbols is in the read only mode.
	inline void  ReleaseObject()	{ if (name_value != NULL) { free(name_value); name_value = NULL; } }
	inline void  WipeObject()		{ name_value = NULL; }
};

//
//  Some lexema types, like comments or EOLs, can be ignored in the target language.
//  All such lexemas shold be explicitly defined in the grammar.
//
struct TIgnoreLexRecord
{
	// Type and subtype of lexema in the target language.
	TLexemaType				lex_type;
	bool						check_subt;
	TLexSubtype				lex_subt;
};

//
//  Some lexema types can be explictily marked as error lexemas. This allows emitting specific
//  error messages once such lexemas are met.
//
struct TErrorLexRecord
{
	// Type and subtype of lexema in the target language.
	TLexemaType				lex_type;
	bool						check_subt;
	TLexSubtype				lex_subt;

	// This message will be emitted when this type of lexema is found in the target language source file.
	wchar_t					*error_message;

	// Note: These methods should not be called when array of error lexema definitions is in RO mode.
	inline void  ReleaseObject()	{ if (error_message != NULL) { free(error_message); error_message = NULL; } }
	inline void  WipeObject()		{ error_message = NULL; }
};

//
// This restriction actually restricts type of rules that particular non terminal can be expanded to.
//
enum TNonTermRestrType : unsigned char
{
	ntrs_none,		// no restrictions.
	ntrs_list,		// __list
	ntrs_seq,			// __seq
};

//
//  Non terminal symbol can be only a name, i.e. an identifier. Non terminals do not have explicit definition.
//  They are defined by their first appearance in the grammar. Rules for each non terminal cannot be present
//  in more than one rules section.
//
struct TNonTerminalSymbol
{
	wchar_t					*symbol_name;			// Name of the non terminal in the grammar definition.

	WORD					app_id;					// Application id, assigned with modifier __id(n).
													// Value of the field is zero when the app id is not set.
	WORD					rules_sect;				// Id of the rules section with definitions for the symbol.

	bool						public_symbol;
	TNonTermRestrType		restr_type;
	bool						solid_symbol;			// Symbol is considered solid when it can not have empty representation.

	ID						event_id;				// Id of the parsing database event with descr of the symbol intro area.

	int						num_lhs_rule_uses;
	int						num_rhs_rule_uses;

	// Note: These methods should not be called when the array of non terminals is in the RO mode.
	inline void  ReleaseObject()	{ if (symbol_name != NULL) { free(symbol_name); symbol_name = NULL; } }
	inline void  WipeObject()		{ symbol_name = NULL; }
};

typedef  TStructsArrayEx<TTerminalSymbol, 16, 256>		TTerminalSymbolsArray;
typedef  TStructsArray<TIgnoreLexRecord, 16, 256>		TIgnoreLexRecordsArray;
typedef  TStructsArrayEx<TErrorLexRecord, 16, 256>		TErrorLexRecordsArray;
typedef  TStructsArrayEx<TNonTerminalSymbol, 16, 256>	TNonTerminalsArray;

// --------  Part 2. Grammar rules and parsing states  --------------

//
//  Symbols representation in the grammar.
//
//  Symbol value 0 is EOF. This symbol is treated as a terminal symbol. It differs from all other terminal symbols
//  because it is not defined explictly. This means it is present in every grammar. At the same time it has no
//  representation in the grammar definition, so it cannot appear in rules. This guarantees that EOF will be never
//  shifted into the stack of the grammar based parser.
//
enum TGrammarSymbolBases
{
	TerminalSymbolsBase		=	0,
	IgnoreLexTypeBase			=	3500,
	ErrorLexTypeBase			=	3800,

	NonTerminalsBase			=	4000,
	RuleObjectsBase				=	9000,

	LocationObjectsBase			=	12000,
	NestedResultsBase			=	14000,
	XpctConflictsBase			=	15000,

	NextCategoryBase			=	16000,			// Base of the next category is used to verify that xpct conflict symbol has
													// reasonable value.
};

//
//  Values from this enum are simiar to the symbol values, although they do not correspond
//  to any object in the grammar.
//
enum TGrammarSymbolBaseExtras
{
	GenericIgnoreLex			=	3799,			// This value should be used by the app level grammar based parser as return
													// value from ReviewInputSymbol() when app layer parser wants to tell to generic
													// parser that lexema should be ignored. This value should be never used
													// during grammar parsing and analysis.
	NonTermForwardDefnsBase	=	7000,			// This is part of the range of the non terminal values. Objects with symbol value
													// from this subrange can exist only while the grammar defn file is being scanned.
													// After scanning all undefined non terminals are moved to the main non terms table.
	RuleNonTermObject			=	24000,			// This value is used only in the PrGrammarDefnObject structure.
													// It marks all events that introduce left hand side non terminal of one or more rules.
	XpctConflictNameObject		=	15999,			// This value is used only in the PrGrammarDefnObject structure.
													// It marks all events that introduce the names of the expected conflicts.
};

struct TSymbolsArray : public TStructsArray<WORD, 16, 256>
{
	TSymbolsArray() : TStructsArray() { }
	TSymbolsArray(WORD *data, int len_data) : TStructsArray(data, len_data) { }

	inline bool IsSymInBuffer(WORD sym)
	{
		for (int isym=0; isym<NumItems(); ++isym)
		{
			if (ItemRef(isym) == sym)
				return(TRUE);
		}

		return(FALSE);
	}

	inline bool IsEqualTo(TSymbolsArray &other_inst)
	{
		if (NumItems() != other_inst.NumItems())
			return(FALSE);

		// Lengths are identical. Compare the contents.
		for (int isym=0; isym<NumItems(); ++isym)
		{
			if (ItemRef(isym) != other_inst[isym])
				return(FALSE);
		}

		// All elements are identical also.
		return(TRUE);
	}

	inline void FillArray(WORD sym_value)
	{
		for (int isym=0; isym<NumItems(); ++isym)
			ItemRef(isym) = sym_value;
	}

	static int __cdecl CompareSymbols(const WORD *ps1, const WORD *ps2)
	{
		if (*ps1 > *ps2)
			return(1);
		if (*ps1 < *ps2)
			return(-1);

		return(0);
	}
};

typedef TStructsArrayEx2<TSymbolsArray, 4, 256> TSymbolsArrayArray;

//
//  This structure does not allow moving it from one place in the memory to another because it contains
//  pointers to its fields. This is adresseed by having only one place in the code that moves this structure.
//  This is done when the new rule is added.
//
struct TGrammarRule
{
	WORD				non_term;
	TSymbolsArray		symbols;

	WORD				rule_app_id;					// Application id of the rule, it is assigned with modifier __id(n).
	wchar_t				*rule_app_id_name;

	ID					non_term_event_id;
	ID					rule_body_event_id;

	inline int  Length() { return(symbols.NumItems()); }

	// Note: These methods should not be called when the array of rules is in RO mode.
	inline void FixObjectMove()	{ symbols.FixObjectMove(); }
	inline void ReleaseObject()	{ symbols.ReleaseObject(); if (rule_app_id_name != NULL) { free(rule_app_id_name); rule_app_id_name = NULL; } }
	inline void WipeObject()		{ symbols.WipeObject(); rule_app_id_name = NULL; }
};

typedef TStructsArrayEx2<TGrammarRule, 16, 256> TGrammarRulesArray;

//
//  Parsing states are numbered 0, 1, 2, ..... k-1.
//  When the parsing state is not yet assigned, the value 0xFFFF is used.
//

struct TRulePosition
{
	WORD		irule;				// Zero based index of the rule.
	WORD		isym;				// Position is in front of the symbol with the index isym. Position can also be
									// after the last symbol in the right hand side of the rule.
	WORD		action_sym;			// Action symbol is used olny in LR case. In SLR case it has the fixed value 0xFFFF.
									// This field contains terminal symbol that triggers applying the rule once this symbol
									// is found after the rule.
	WORD		mark;				// This field is used to speed up creation of set of the parsing states.
									// This field has no application level meaning.

	inline bool  CompareTo(TRulePosition &other_pos) { return(irule == other_pos.irule && isym == other_pos.isym && action_sym == other_pos.action_sym); }
	inline DWORD CalcHashValue() { return(irule ^ isym ^ (action_sym << 3)); }

	static int __cdecl CompareLight(const TRulePosition *rp1, const TRulePosition *rp2);
};

typedef TStructsArray<TRulePosition, 16, 64> TRulePositionsArray;

enum TFullRulePosOrigin : BYTE
{
	rpo_initial_placement,				// This origin can be present only at the beginning of the path and only for reduce
									// conflicting action.
	rpo_rule_start_state,
	rpo_rule_call_place,				// This is place where the rule was called or this is the first and the only one step of the path
									// for the shift conflicting action. In the last case discovered places stay infont of the terminals.
	rpo_step_over_symbol,

	rpo_step_over_non_term,			// This step is added when derivation path is stepping over the transparent non terminal.
	rpo_non_term_defn_rule,
	rpo_step_up_the_tree,

	rpo_axioma_action_sym,			// This step can be present only in the paths for the nested grammars.

	rpo_start_state_ctx,
	rpo_call_place_ctx,
	rpo_step_over_ctx,
	rpo_step_up_ctx,

	rpo_axioma_action_ctx,				// This step can be present only in the paths for the nested grammars.

	rpo_num_types,
};

struct TFullRulePosition
{
	union
	{
		struct
		{
			union
			{
				struct
				{
					WORD				irule;
					BYTE				isym;
					TFullRulePosOrigin		origin;
				};

				DWORD		dwHeader;
			};

			WORD			istate;
			WORD			action_sym;
		};

		UINT64				qwValue;
	};

	inline void Clear() { qwValue = 0; }

	static int __cdecl CompareLight(const void *pp1, const void *pp2);

	static wchar_t				*GetFullRulePosOriginEnumName(TFullRulePosOrigin value);
	static TFullRulePosOrigin		GetFullRulePosOriginFromStringPtr(TStringPtr &data);
};

typedef TStructsArray<TFullRulePosition, 64, 128> TFullRulePositionsArray;

//
// This is information about the range of positions in some array of positions that belong to certain rule
// or non terminal definition. This info is used as index in the grammar conflicts analysis code.
//
struct TRulePosIterIndexRecord
{
	WORD				pars_stt;
	int					pos_beg;
	int					num_positions;			// When when num_positions <= 0, this means that record is empty.
};

typedef  TStructsArray<TRulePosIterIndexRecord, 1, 256>  TRulePosIterIndexLevel1;
typedef  TStructsArrayEx2<TRulePosIterIndexLevel1, 1, 256>  TRulePosIterIndexLevel2;

//
//  Structure of the Level 2 index for "RuleStartStatesIndex". Length of the Level 2 array is equal to the number of rules
//  in the grammar. Individual Level 1 records should be picked up using the index of the rule. Level 1 record is an array
//  of 0 or more elements. Each element of the Level 1 array is non empty. It describes possible state that can be present
//  at the beginning of the rule. Besides the state number, Level 1 record contains index of the first position inside the state
//  and the number of positions that desribe starting states for particular irule, istate. Level 1 array has at least one record in
//  its buffer that is not included into its length. This is an empty record. This record is needed for speeding up the rule start
//  states iterator.
//
//  Structure of the Level 2 index for "NonTermDefnRulesIterIndex". This index is a simple matrix that describes positions
//  for each non terminal and parsing state.
//

struct TGrammarDerivationPath
{
	__int64				cnt_similar;

	//
	// NB: memory class of the derivation steps should match the RW state of the array.
	//

	int					derv_path_len;
	TFullRulePosition		*derv_steps;
							// Number of steps is always known when derivation path is created and this number
							// never changes. Memory class of the steps data cannot be determined on this level.

	int GetHiddenConflictPosition()
	{
		// Paths for shift and reduce conflicting actions differ in their structure. In the first case search for the path is
		// very simple. It is necessary to find all rules where the conflict state stays in front of the conflicting symbol.
		// In other words the path itself will always have only one step. And type of this step will be rpo_rule_call_place
		// because this iterator is used to find these places although no rule is called here.
		// Paths for reduce actions always contain several steps and its last step is typically rpo_step_over_symbol and
		// in rare cases it is rpo_axioma_action_sym.
		for (int istep=derv_path_len-1; istep >= 0; --istep)
		{
			TFullRulePosOrigin origin = derv_steps[istep].origin;
			if (origin == rpo_step_over_symbol || origin == rpo_rule_call_place)
				return(istep);
			else if (origin == rpo_axioma_action_sym)
				return(istep-1);
		}

		return(-1);
	}

	// Note: These methods should not be called when array of derivation paths array is in RO mode.
	inline void ReleaseObject()	{ if (derv_steps != NULL) { free(derv_steps); derv_steps = NULL; } }
	inline void WipeObject()		{ derv_steps = NULL; }
};

typedef  TStructsArrayEx<TGrammarDerivationPath, 1, 64>  TGrammarDerivationPathsArray;

//
//  Array of derivation paths is typically associated with particular conflicting action of the grammar conflict.
//
struct TConflictingActionAnalysisResults
{
	enum TGenResult
	{
		// Initial state.
		gres_not_started,				// Data is not available because processing is not started yet.

		// Intermediate states.
		gres_proc_running,				// Processing is currently active. Some worker process is running.
		gres_aborted_by_user,			// Iteration was aborted by request from the scanner console.
		gres_timeout,

		// Final state, no problem.
		gres_ok,						// Processing is complete.

		// Final states with problem.
		gres_no_paths_found,			// Iteration succeeded, but no derivation paths were found.
		gres_out_of_memory,			// Error allocating space for newly discovered path.
		gres_too_many_paths,			// Iteration was aborted because the number of paths has reached the max value that
										// is stored the analysis props. Note that actual number of stored paths can be less than
										// the limit becasue similar paths are filtered out after stopping the iteration process.
		gres_num_types,
	};

	TGenResult						m_generation_result;
	__int64							m_generation_steps;
	TDateTime						m_generation_time;

	__int64							m_too_deep_truncations;
	__int64							m_partial_iteration_aborts;
										// These fields give details on how generation was carried out.
										// They do not describe the results themselves.

	TGrammarDerivationPathsArray		m_derivation_paths;

public:

	void Clear()
	{
		m_generation_result = gres_not_started;
		m_generation_steps = 0;
		m_generation_time = 0;

		m_too_deep_truncations = 0;
		m_partial_iteration_aborts = 0;
		m_derivation_paths.Clear();
	}

	TGrammarDerivationPath *AppendPathProto(int path_len)
	{
		TFullRulePosition *steps = (TFullRulePosition*)malloc(path_len*sizeof(TFullRulePosition));
		if (steps == NULL)
			return(NULL);

		TGrammarDerivationPath new_path = { 1, path_len, steps };
		if (m_derivation_paths.AppendItem(new_path) == FALSE)
		{
			free(steps);
			return(NULL);
		}

		return(m_derivation_paths.ItemPtr(m_derivation_paths.NumItems()-1));
	}

	static const wchar_t	*GetGenResultEnumName(TGenResult value);
	static const wchar_t	*GetGenResultDescription(TGenResult value, bool want_first_cap);
	static TGenResult		GetGenResultFromStringPtr(TStringPtr &data);

	// Note: These methods should not be called when array of derivation paths array is in the RO mode.
	inline void FixObjectMove()	{ m_derivation_paths.FixObjectMove(); }
	inline void ReleaseObject()	{ m_derivation_paths.ReleaseObject(); }
	inline void WipeObject()		{ m_derivation_paths.WipeObject(); }
};

typedef  TStructsArrayEx2<TConflictingActionAnalysisResults, 2, 8>  TGrammarConflictAnalysisResults;

//
//  Parsing state is a set of rule positions.
//
//  NB:	Positions in the state are _ALWAYS_ sorted in the ascending order:  irule -> isym -> action_sym.
//		Further analysis relies on this.
//
class TParsingState : public TStructsArray<TRulePosition, 64, 1024>
{
public:

	TParsingState() : TStructsArray<TRulePosition, 64, 1024>() { }
	TParsingState(TRulePosition *data, int len_data) : TStructsArray<TRulePosition, 64, 1024>(data, len_data) { }

	void ClearMarkFields() { for (int i=0; i<NumItems(); ++i) ItemRef(i).mark = 0; }

	bool CompareTo(TParsingState &other_stt)
	{
		if (NumItems() != other_stt.NumItems())
			return(FALSE);

		for (int i=0; i<NumItems(); ++i)
		{
			if (ItemRef(i).CompareTo(other_stt[i]) == FALSE)
				return(FALSE);
		}

		return(TRUE);
	}

	int FindFirstPosForRule(int irule, bool want_initial_pos)
	{
		if (num_items <= 0)
			return(-1);
		if (extended_data[0].irule == irule)
		{
			if (want_initial_pos == FALSE)
				return(0);
			return((extended_data[0].isym == 0) ? 0 : -1);
		}

		// The parsing state is not empty and it starts with position in some other rule.
		int lo_good = 0;
		int hi_bad = num_items;
		while ((hi_bad - lo_good) > 1)
		{
			int inx_test = (hi_bad+lo_good)/2;
			if (extended_data[inx_test].irule < irule)
			{
				lo_good = inx_test;
			}
			else if (extended_data[inx_test].irule > irule)
			{
				hi_bad = inx_test;
			}
			else
			{
				// Record for the requested rule is present.
				int inx_ret = inx_test;
				while (inx_test > 0)
				{
					if (extended_data[--inx_test].irule != irule)
					{
						if (want_initial_pos == FALSE)
							return(inx_ret);
						return((extended_data[inx_ret].isym == 0) ? inx_ret : -1);
					}

					inx_ret = inx_test;
				}

				assert(FALSE);
				return(0);
			}
		}

		// Parsing state has no positions for the requested rule.
		return(-1);
	}
};

class TParsingStatesArray : public TStructsArrayEx2<TParsingState, 64, 1024>
{
public:

	TParsingStatesArray() : TStructsArrayEx2<TParsingState, 64, 1024>() { num_positions = 0; }
	TParsingStatesArray(TParsingState *data, int len_data, int num_positions_in_data) : TStructsArrayEx2<TParsingState, 64, 1024>(data, len_data) { num_positions = num_positions_in_data; }

	int		num_positions;			// This is total number of rule positions in all parsing states. This field
									// is used for diagnostics only. It does not have any app meaning.
};

//
//  Source grammar can be annotated with markers that define places, where grammar conflicts are expected.
//  Grammar parser will verify if these places really generate conflicts or not.
//
struct TExpectedConflictLocation
{
	wchar_t					*location_name;				// Name of the location object as it was specified in the loc marker.
														// All location names are unique in the whole hierarchy of grammars.
	WORD					action_symbol;				// Action symbol is always assigned when the object is being created.
														// It is set to the symbol in the marker if marker stays at the end of
														// the rule or to the symbol in the rule if loc marker stays in front or
														// in the middle. It can be set to EOF in case of syntax errors.
	TRulePositionsArray		positions;					// This array is filled during parsing the grammar source files.
														// In case of syntax errors this array can be empty. Positions inside
														// the rule have in their action symbol field the value 0xFFFF.
	WORD					num_xpct_conflicts;			// Number of expected conflicts that mention this location object.
														// Note that when the source grammar is scanned, expected conflicts
														// may belong to any grammar. After generating target grammars
														// the location objects and expected conflicts are local references.
	TSymbolsArray			conforming_states;			// Parsing states are added to the set after the grammar conversion.
														// This list contains parsing states that are present in all positions
														// that are stored in the list of positions in this object.
	//
	//  Note on the mark filled in the list of positions. Possible values:
	//
	//		FALSE	--	This is ordinary position.
	//		TRUE	--	This is pseudo position. It is not participating in the process of determining the list of conforming states.
	//					It turns out that in some cases there are places where only some of the conforming states are present.
	//					To avoid generation of the warning such places should be marked with pseudo loc marker.
	//

	// Note: These methods should not be called when an array of expected confilt locations is in the RO mode.
	inline void FixObjectMove()	{ positions.FixObjectMove(); conforming_states.FixObjectMove(); }
	inline void ReleaseObject()	{ if (location_name != NULL) { free(location_name); location_name = NULL; } positions.ReleaseObject(); conforming_states.ReleaseObject(); }
	inline void WipeObject()		{ location_name = NULL; positions.WipeObject(); conforming_states.WipeObject(); }
};

typedef  TStructsArrayEx2<TExpectedConflictLocation, 8, 32>  TExpectedConflictLocationsArray;

//
//  Nested proc result can be present ONLY in expected conflict resolution statement.
//
struct TNestedProcessingResult
{
	WORD					nest_res_app_id;
	wchar_t					*nest_res_name;

	ID						event_id;					// Id of event with intro area.

	// Note: These methods should not be called when an array of nested results is in the RO mode.
	inline  void  ReleaseObject()	{ if (nest_res_name != NULL) { free(nest_res_name); nest_res_name = NULL; } }
	inline  void  WipeObject()		{ nest_res_name = NULL; }
};

typedef  TStructsArray<TNestedProcessingResult, 64, 128>  TNestedProcessingResultsArray;

//
//  Expected grammar conflict is a set of expected conflict locations. In the correct grammar definition conflict location identifies
//  set of grammar conflicts. These sets are groupped by the same conflict resolution strategy into expected grammar conflict.
//
struct TExpectedGrammarConflict
{
	wchar_t				*xpct_conflict_name;

	WORD				xpct_conflict_app_id;

	TSymbolsArray		xpct_conflict_locations;
							// Zero based indexes of the location objects.

	int					num_conforming_conflicts;

	TSymbolsArrayArray	resolution;
							// This is array of arrays. Fields of the inner array are either valid app_ids or the zero value.
							// Zero value means the shift action. App ids corerspond either to the grammar rule or to the nested
							// processing result. Length of each inner array is equal to the number of action codes, associated with
							// symbols in the right hand sides of the target rules of the nested grammar. Length of the outer array
							// is equal to the number of conflict resolution goals in the nested grammar.

	TGrammar			*nested_grammar;
							// Nested grammar is used for resolving participating grammar conflicts.

	ID					event_id;						// Id of event with the intro area.

	// Note: These methods should not be called when an array of expected grammar conflicts is in RO mode.
	inline	void FixObjectMove()	{ xpct_conflict_locations.FixObjectMove(); resolution.FixObjectMove(); }
	inline	void ReleaseObject();	// This method cannot be implemeted inline because of the forward declaration of TGrammar.
	inline	void WipeObject()		{ xpct_conflict_name = NULL; xpct_conflict_locations.WipeObject(); resolution.WipeObject(); nested_grammar = NULL; }
};

typedef  TStructsArrayEx2<TExpectedGrammarConflict, 8, 32>  TExpectedGrammarConflictsArray;

//
//  Info about conflicts is generated by the grammar definition conversion procedure. This information is not present
//  in the source of the grammar directly.
//
//  Note about wanted_actions. Array of actions can contain sift and reduce actions. Shift action can be present
//  only once in the beginning of the array if conflict has shift-reduce nature. If conflict is reduce-reduce, all actions
//  are reduce actions.
//
//  To find out location of conficts in rules it is necessary to scan parsing state locations for shift action. For reduce
//  action conflict place is at the end of its rule.
//

typedef TStructsArray<WORD, 4, 256> TGrammarConflictActionsArray;

struct TGrammarConflict
{
	WORD							parser_state;
	WORD							input_symbol;

	TGrammarConflictActionsArray		conflicting_actions;

	short							conflict_location_inx;
										// This fields contains zero based index of the loc object in the current grammar. When grammar conflict is not
										// owned by any loc object, value of this field is -1. Negative value, starting from -2, indicates number of
										// compatible ambiguous location objects.

	short							expected_conflict_inx;
										// Index of the xpct conflict in the same grammar. When grammar conflict is not part of any expected conflict,
										// value of this field is -1. Negative value indicates number of compatible ambiguous expected conflicts.

	TGrammar						*nested_grammar;
										// This field is checked and used by the grammar based parser to decide if nested parsing is needed or not.
										// Nested grammar is not owned here. It is owned by the expected grammar conflict.

	TSymbolsArray					conflict_resolution;
										//
										// This field is not empty only for those grammar conflicts that belong to expected conflicts and these expected
										// conflicts should have nested grammars. Length of this array is equal to the number of resolution rules in
										// the nested grammar. Values, stored in the array, have one of the following forms:
										//
										//		actb_shift + istate
										//		actb_reduce + irule
										//		actb_nested_result + inres
										//

	TGrammarConflictAnalysisResults		analysis_results;
										// Analysis results are generated after the grammar conversion and they can be missing.
										// Lengh of this array is either 0 or it is equal to the number of conflicting actions.

	inline int		NumActions()		{ return(conflicting_actions.NumItems()); }
	inline WORD	GetAction(int iact)	{ return(conflicting_actions[iact]); }

	bool BuildAnalysisResultsStub()
	{
		// The conflict should be filled with data at this point.
		assert(NumActions() > 0);

		// Kill the old analysis data if any.
		analysis_results.Clear();

		TConflictingActionAnalysisResults results_stub;
		results_stub.Clear();

		// Fill in the analysis results array with empty records.
		for (int iact=0; iact<NumActions(); ++iact)
		{
			if (analysis_results.AppendItem(results_stub) == FALSE)
			{
				analysis_results.Clear();
				return(FALSE);
			}
		}

		// Success.
		return(TRUE);
	}

	// Note: These methods should not be called when an array of grammar conflicts is in the RO mode.
	inline void FixObjectMove()	{ conflicting_actions.FixObjectMove(); conflict_resolution.FixObjectMove(); analysis_results.FixObjectMove(); }
	inline void ReleaseObject()	{ conflicting_actions.ReleaseObject(); conflict_resolution.ReleaseObject(); analysis_results.ReleaseObject(); }
	inline void WipeObject()		{ conflicting_actions.WipeObject(); conflict_resolution.WipeObject(); analysis_results.WipeObject(); }
};

typedef TStructsArrayEx2<TGrammarConflict, 64, 1024> TGrammarConflictsArray;

enum TNonTerminalsSortOrder
{
	ntsm_sym_val,					// This is order of symbol definitions in the grammar. In other words order of records
									// that descrive terminal or non terminal symbols.
	ntsm_alpha_num,
	ntsm_special_anum,				// Symbols (non terminals) are compared by their last subword first. Last subword
									// starts from any non lower case letter. This can be either capital letter or underscore.
									// If last subnames match, then the beginnings of the names are compared.
	ntsm_sect_rule,					// Symbols are sorted by the rule sections. Inside the section symbols are sorted
									// by the order of rules, that define them.
	ntsm_num_types,
};

class TGrammarSymbolsHelper
{
public:

	static WORD	AddTerminalSymbol(TTerminalSymbolsArray &dest, bool sym_type, __int64 sym_num_val, TStrPtrInfo &sym_str_val_ptr, WORD app_id,
										TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt, ID event_id);
	static bool	AddIgnoreLexRecord(TIgnoreLexRecordsArray &dest, TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt);
	static bool	AddErrorLexRecord(TErrorLexRecordsArray &dest, TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt, TStrPtrInfo &error_msg_ptr);

	static WORD	AddNonTerminalSymbol(TNonTerminalsArray &dest, TStrPtrInfo &name_ptr, WORD app_id, WORD rules_sect, ID event_id);
	static bool	AddGrammarRule(TGrammarRulesArray &dest, WORD rule_non_term, TSymbolsArray &rule_symbols, WORD rule_app_id, const wchar_t *rule_app_id_name, ID non_term_event_id, ID body_event_id);
	static bool	AddNestedProcResult(TNestedProcessingResultsArray &dest, WORD nres_app_id, const wchar_t *nres_name, ID event_id);
	static bool	AddExpectedConflict(TExpectedGrammarConflictsArray &dest, const wchar_t *xpct_name, WORD xpct_app_id, int cnt_locations, ID event_id);

	static WORD	FindSymbol(TTerminalSymbolsArray &term_syms, TNonTerminalsArray &non_terms, TLexema &lex);
	static WORD	FindSymbol(TTerminalSymbolsArray &term_syms, TNonTerminalsArray &non_terms, TStrPtrInfo &name_ptr);
	static int		FindLocationObject(TExpectedConflictLocationsArray &locs_array, TStrPtrInfo &loc_object_name);

	static wchar_t	*GetSymbolName(TTerminalSymbolsArray &term_symbols, TNonTerminalsArray &non_terms, WORD sym, wchar_t *buff_40_chars);
					// Symbol name for terminals and non terminals.

	static int		GetFirstRuleForNonTerm(TGrammarRulesArray &rule_defns, WORD sym);
	static int		GetRuleOffs(TGrammarRulesArray &rule_defns, int rule_inx);

	static bool	CreateSortedTerminalSymbolsList(TTerminalSymbolsArray &term_sym_defns, TSymbolsArray &term_syms_list, bool sort_alphabetically = FALSE);
	static bool	CreateSortedNonTerminalsList(TNonTerminalsArray &non_term_defns, TGrammarRulesArray *rules, TSymbolsArray &non_terms_list, TNonTerminalsSortOrder sorting_mode = ntsm_alpha_num, int rules_sect_restr = -1);
	static void	SortAndDumpNonTerminalsList(TDestinationFile &rprt, TNonTerminalsArray &non_term_defns, TGrammarRulesArray *rules, TSymbolsArray &non_terms_list, TNonTerminalsSortOrder sorting_mode = ntsm_alpha_num);
	static void	WriteReportDelimiter(TDestinationFile &rprt, bool long_delim = FALSE);

private:

	struct TSymbolsSortingCtx { TTerminalSymbolsArray *m_term_syms; TNonTerminalsArray *m_non_terms; TGrammarRulesArray *m_rule_defns; TNonTerminalsSortOrder m_sort_mode; };

	static void		SortSymbols(TTerminalSymbolsArray *term_syms, TNonTerminalsArray *non_terms, TGrammarRulesArray *rules, TSymbolsArray &syms, TNonTerminalsSortOrder sorting_mode = ntsm_alpha_num);
	static int __cdecl	CompareGrammarSymbols(void *context, const WORD *ps1, const WORD *ps2);
						// Context is TSymbolsSortingCtx, params p1 and p2 are pointers to WORDs.
};

// ----------  Part 3. Grammar structure  -----------

enum TLexToSymConverterArraySetupMode
{
	ltsc_use_setup_data,
	ltsc_fill_unmapped,
	ltsc_fill_eof,
};

//
//  Lex to sym converter converts lexemas that are picked up from the source code in the target language
//  into the symbol value that can be used in the grammar based parser.
//
struct TLexToSymConverter
{
	TLexToSymConverter() { Clear(); }
	TLexToSymConverter(TLexToSymConverterArraySetupMode ltsc_main, WORD *main_data,
					TLexToSymConverterArraySetupMode ltsc_comment, WORD *comment_data,
					TLexToSymConverterArraySetupMode ltsc_number, WORD *number_data,
					TLexToSymConverterArraySetupMode ltsc_floating_point, WORD *floating_point_data,
					TLexToSymConverterArraySetupMode ltsc_charconst, WORD *charconst_data,
					TLexToSymConverterArraySetupMode ltsc_string, WORD *string_data,
					TLexToSymConverterArraySetupMode ltsc_keyword, WORD *keyword_data,
					TLexToSymConverterArraySetupMode ltsc_name, WORD *name_data,
					TLexToSymConverterArraySetupMode ltsc_endofline, WORD *endofline_data)
	{
		SetupArray(ltsc_main, m_lex_data, main_data, sizeof(m_lex_data));
		SetupArray(ltsc_comment, m_ltx_comment_data, comment_data, sizeof(m_ltx_comment_data));
		SetupArray(ltsc_number, m_ltx_number_data, number_data, sizeof(m_ltx_number_data));
		SetupArray(ltsc_floating_point, m_ltx_floating_point_data, floating_point_data, sizeof(m_ltx_floating_point_data));
		SetupArray(ltsc_charconst, m_ltx_charconst_data, charconst_data, sizeof(m_ltx_charconst_data));
		SetupArray(ltsc_string, m_ltx_string_data, string_data, sizeof(m_ltx_string_data));
		SetupArray(ltsc_keyword, m_ltx_keyword_data, keyword_data, sizeof(m_ltx_keyword_data));
		SetupArray(ltsc_name, m_ltx_name_data, name_data, sizeof(m_ltx_name_data));
		SetupArray(ltsc_endofline, m_lex_endofline_data, endofline_data, sizeof(m_lex_endofline_data));
	}

	enum
	{
		lt_eof					= 0,
		lt_check_subtype		= 0xFFFE,		// This value is never given out from the GetSymFromLex() method.
		lt_sym_empty			= 0xFFFF,
	};

	enum
	{											// This is max number of the name subtypes that the app level parser
		lt_num_name_types		= 16,			// can assign to the names in the source code. This value should be
	};											// even power of 2.

	void Clear(WORD sym = lt_sym_empty)
	{
		int i;

		for (i=0; i<ltx_num_lexema_types; ++i)
			m_lex_data[i] = sym;

		for (i=0; i<lct_num_comment_types; ++i)
			m_ltx_comment_data[i] = sym;

		for (i=0; i<lnt_num_number_types; ++i)
			m_ltx_number_data[i] = sym;

		for (i=0; i<lfp_num_floating_point_types; ++i)
			m_ltx_floating_point_data[i] = sym;

		for (i=0; i<lchct_num_charconst_types; ++i)
			m_ltx_charconst_data[i] = sym;

		for (i=0; i<lstrt_num_string_types; ++i)
			m_ltx_string_data[i] = sym;

		for (i=0; i<ltkn_num_keyword_vals; ++i)
			m_ltx_keyword_data[i] = sym;

		for (i=0; i<lt_num_name_types; ++i)
			m_ltx_name_data[i] = sym;

		for (i=0; i<leolt_num_eol_types; ++i)
			m_lex_endofline_data[i] = sym;
	}

	//
	//  Possible expected return values from GetSymFromLex():
	//
	//		lt_eof <= terminalSymbol < lt_eof+m_grammar->symbols.NumItems();
	//				--	Lexema was successfully mapped to the terminal symbol of the grammar.
	//
	//		IgnoreLexTypeBase <= ignoreValue < IgnoreLexTypeBase+m_grammar->ignore_lex.NumItems();
	//				--	Lexema was recognized as silently ignorable lexema.
	//
	//		ErrorLexTypeBase <= errorValue < ErrorLexTypeBase+m_grammar->error_lex.NumItems();
	//				--	Parser should issue specific error message and ignore the lexema.
	//
	//		lt_sym_empty (0xFFFF)
	//				--	This lexema type is not assigned to anything in the grammar. Parser should issue
	//					generic error message and ignore this lexema.
	//

	inline WORD GetSymFromLex(TLexema &lex)
	{
		// Check the primary lexema type.
		if (m_lex_data[lex.type] != lt_check_subtype)
			return(m_lex_data[lex.type]);

		// Check lexema subtype.
		if (lex.type == ltx_keyword)
			return(m_ltx_keyword_data[(short)lex.num_value]);
		else if (lex.type == ltx_name)
			return(m_ltx_name_data[lex.subtype & (lt_num_name_types-1)]);
		else if (lex.type == ltx_number)
			return(m_ltx_number_data[lex.number_type]);
		else if (lex.type == ltx_floating_point)
			return(m_ltx_floating_point_data[lex.floating_point_type]);
		else if (lex.type == ltx_string)
			return(m_ltx_string_data[lex.string_type]);
		else if (lex.type == ltx_comment)
			return(m_ltx_comment_data[lex.comment_type]);
		else if (lex.type == ltx_charconst)
			return(m_ltx_charconst_data[lex.charconst_type]);
		else if (lex.type == ltx_eol)
			return(m_lex_endofline_data[lex.endofline_type]);

		// Bogus lexema type.
		return(lt_sym_empty);
	}

	inline void SetupArray(TLexToSymConverterArraySetupMode ltsc, WORD *dest_array, WORD *data, int array_len_in_bytes)
	{
		if (ltsc == ltsc_fill_unmapped)
		{
			// Prepare array in the root grammar mode.
			memset(dest_array, 0xFF, array_len_in_bytes);
		}
		else if (ltsc == ltsc_fill_eof)
		{
			// Prepare array in the nested grammar mode.
			memset(dest_array, 0x00, array_len_in_bytes);
		}
		else
		{
			// Use the passed setup data.
			assert(data != NULL);
			memcpy(dest_array, data, array_len_in_bytes);
		}
	}

	inline bool IsSameValue(WORD *data_array, int array_len_in_items, WORD value)
	{
		for (int inx=0; inx<array_len_in_items; ++inx)
		{
			if (data_array[inx] != value)
				return(FALSE);
		}
		return(TRUE);
	}

	//
	// Note that default filler for the root grammar is lt_sym_empty (0xFFFF) and default filler for all nested
	// grammars is lt_eof (0x0000).
	//

	// Main lex type switch. The ltx_eof slot is always filled with EOF value because EOF is added to the source
	// grammar in the very beginning of scanning and then, when symbols are copied to the root and nested
	// grammars, EOF slot is explictly marked to be copied to the grammar being generated.
	WORD		m_lex_data[ltx_num_lexema_types];

	// Lexema subtypes switch.
	WORD		m_ltx_comment_data[lct_num_comment_types];
	WORD		m_ltx_number_data[lnt_num_number_types];
	WORD		m_ltx_floating_point_data[lfp_num_floating_point_types];
	WORD		m_ltx_charconst_data[lchct_num_charconst_types];
	WORD		m_ltx_string_data[lstrt_num_string_types];
	WORD		m_ltx_keyword_data[ltkn_num_keyword_vals];
	WORD		m_ltx_name_data[lt_num_name_types];
	WORD		m_lex_endofline_data[leolt_num_eol_types];
};

enum TAnalysisActionBases
{
	actb_shift			=	0,
	actb_reduce			=	20000,				// Reduce base should be big enough to allow allocation of big enough
												// number of parsing states. In one case there was about 14000 parsing
												// states.
	actb_conflict		=	23000,
	actb_nested_result	=	29000,
	actb_bogus_val		=	31000,

	actb_generic_syntax_error	= 0xFFFF,		// Action for current parsing situation is not defined.
												// This is unrecoverable generic syntax error.
};

//
// Structure of the field when TAnalysisTable is used as FIRSTs and FOLLOWs matrix.
//
#define  FUN_FIRST_MASK		0xFF00
#define  FUN_FIRST_FLAG		0x0100
#define  FUN_FOLLOWS_MASK	0x00FF
#define  FUN_FOLLOWS_FLAG	0x0001

//
//  Major purpose of this class is to get pointer to the table inited to NULL in the default ctor.
//
struct TAnalysisTable
{
		TAnalysisTable(WORD *ptr = NULL, int width = 0, int rows_used = 0) { table_body = ptr; table_owned = FALSE; table_width = width; table_height = -1; table_rows_used = rows_used; }
		~TAnalysisTable() { ReleaseTable(); }

	enum { empty_cell = 0xFFFF };

	bool		IsInited() { return((table_body != NULL) ? TRUE : FALSE); }

	bool		SetupTable(int num_states, int num_symbols);
	void		ReleaseTable();
	void		TakeContentsFrom(TAnalysisTable &other_table);

	bool		CheckRowExistence(int row_index);
				// This method reallocates the table if necessary. Retrun value is FALSE when attempt to extend the table fails.

	inline void	IncRowsUsed() { assert(table_rows_used < table_height); table_rows_used++; }
	inline int		GetRowsUsed() { return(table_rows_used); }

	inline WORD	*GetBodyPtr() { return(table_body); }
	inline int		GetWidth() { return(table_width); }

	inline int		GetIndex(WORD state, WORD sym_table) { return(state*table_width+sym_table); }
	inline WORD	GetAction(WORD state, WORD sym_table) { return(table_body[state*table_width+sym_table]); }
	inline WORD	*GetRowPtr(WORD state) { return(table_body+state*table_width); }

	inline WORD	*GetNonTermRowPtr(WORD non_term) { return(table_body+(non_term-NonTerminalsBase)*table_width); }
					// This method is used when the table is used as storage for values of functions FIRST(x) and FOLLOWS(x).
protected:

	WORD			*table_body;		// This field should be used to check if table is inited or not.
	bool				table_owned;		// Flag shows if table body is in the heap or it is in the read only static data.
	int				table_width;			// Width of analysis table. Width is the same to the number of analysis table symbols.
	int				table_height;		// This is the allocated height.
	int				table_rows_used;
};

// Names of the analysis table generation algorithms.
enum TGrammarConvMethod
{
	grcm_unkn,
	grcm_slr,
	grcm_lr1,
	grcm_lalr,
};

enum TGrammarProcessingResult
{
	grpr_none,							// The grammar is empty or its processing is not finished.
	grpr_syntax_errors,
	grpr_conversion_errors,
	grpr_full_success,
};

struct TGrammarProps
{
	enum { MAX_DESCRIPTION_LEN = 60 };
	enum { MAX_CBK_NAME_LEN = 20 };
	enum { MAX_PREFERRED_PREFIX_LEN = 10 };

	wchar_t							grammar_description[MAX_DESCRIPTION_LEN];
	DWORD							grammar_revision;

	wchar_t							grms_file_name[MAX_PATH];			// Full name of the primary grammar source file.
	long								grms_file_length;
	TDateTime						grms_file_date;

	ID								parsing_id;
	TGrammarProcessingResult			processing_result;					// Processing result is the same in all grammars in the tree of grammars.
																		// This is by design. This simplifies checking the state of the grammars.
	THighLevelLangType				lang_type;
	wchar_t							cbk_handler_name[MAX_CBK_NAME_LEN];
	wchar_t							preferred_emitting_prefix[MAX_PREFERRED_PREFIX_LEN];

	inline void Clear() { memset(this, 0, sizeof(TGrammarProps)); }
};

struct TGrammar
{
	//
	// This class does not have constructors. This is needed to allow initing this class with constants.
	// At the same time this class has data fields that do have ctors. All these fields have default ctors
	// and ctors with parameters. Such design allows using the following code patterns:
	//
	//	TGrammar g1;			<-- Child ctors are called, but the rest of the fields are not inited.
	//	g1.Clear();				<-- Explicit Clear() is needed.
	//
	//	g2 = new TGramamar();	<-- Child ctors are called, but the rest of the fields are not inited.
	//	g2 ->Clear();				<-- Explicit Clear() is needed.
	//
	//	TGrammar g3 = { L"fname" };
	//							<-- First field is inited and default child ctors are called for fields
	//							<-- that have ctors. Rest of the fields are filled with 0.
	//
	//	TGrammar g4 = { L"fname", ..., TTerminalSymbolsArray(name, len) };
	//							<-- First fields is inited and non default ctors are called only for
	//							<-- explicitly inited members. Rest of the fields are filled with 0.
	//

	~TGrammar() { Clear(); }

	void Clear();
			// Note that TGrammar class does not have ctors to allow initing TGrammar instances with constants.
			// Nevertheless the fields of the grammar do have ctors. This means that when the grammar is allocated,
			// these ctors are called. This methods releases previous contents of the structure including switching
			// all TStructsArray fields into read/write mode.

	void TakeContentsFrom(TGrammar &other_grammar);
			// Method releases prev contents of "this" grammar and copies pointers in the fields of the passed grammar
			// if these fields are in read only mode. If fields of the passed grammar are in read/write mode, this method
			// moves contents of these fields from parameter, thus making the passed structure empty.

	enum { sym_EOF = 0 };
	enum { MAX_NAME_LENGTH = 40 };			// Max allowed length of the names of terminal and non term symbols.
												// This number does not include space for the NULL terminator.
	//
	// Data fields.
	//

	TGrammar						*parent_grammar;
	WORD							parent_xpct_inx;					// Index of the xpct conflict in the parent grammar.

	TGrammarProps					grm_props;

	TTerminalSymbolsArray				symbols;
	TIgnoreLexRecordsArray			ignore_lex;
	TErrorLexRecordsArray				error_lex;

	WORD							axioma_ident;						// When the value of the axioma is 0, this means that axioma is not
																		// defined yet.
	TNonTerminalsArray				non_terminals;
	TGrammarRulesArray				rules;

	TExpectedConflictLocationsArray		location_objects;
	TNestedProcessingResultsArray		nested_processing_results;
	TExpectedGrammarConflictsArray		xpct_conflicts;

	TParsingStatesArray				parsing_states;
	TGrammarConflictsArray			conflicts;

	TLexToSymConverter				lex_to_sym_conv;
	TAnalysisTable					analysis_table;						// This table is always built by the external code.

	//
	// Methods.
	//

	inline bool  GetXpndError() const { return(conflicts.GetXpndError()); }
					// This method should be used to check initialization of big grammars that contain
					// more than 1024 conflicts.

	inline int		NumTerminals() const		{ return(symbols.NumItems()); }
	inline int		NumNonTerminals() const	{ return(non_terminals.NumItems()); }
	inline int		NumRules() const			{ return(rules.NumItems()); }
	inline int		NumXpctConflicts() const	{ return(xpct_conflicts.NumItems()); }
	inline int		NumParsingStates() const	{ return(parsing_states.NumItems()); }
	inline int		NumConflicts() const		{ return(conflicts.NumItems()); }

	WORD	FindSymbol(TLexema &lex) { return(TGrammarSymbolsHelper::FindSymbol(symbols, non_terminals, lex)); }
	WORD	FindSymbol(const wchar_t *symbol_name);
				// Methods return symbol code or zero. Methods cannot find EOF.

	int		FindExpectedConflict(const wchar_t *xpct_conflict_name);
	int		FindConflictLocationObject(const wchar_t *loc_object_name);
				// Methods return either index of the object that belogs to "this" grammar or -1 if object with the passed name is not existing.

	int		FindRuleByAppId(WORD app_id);
	int		FindNestedResultByAppId(WORD app_id);
				// Methods return index of the object that belogs to "this" grammar or -1 if object with passed app_id is not available.

	wchar_t	*GetSymbolName(WORD sym, wchar_t *buff_40_chars);
				// Name can be retrieved for  terminals, ignore lex records, error lex records and non terminals.

	WORD	GetSymbolAppId(WORD sym);
				// App id can be retrieved for terminals and non terminals only.

	WORD	GetNextParsingState(WORD stt, WORD sym);
	bool		IsEscapeAbleSeq(WORD *seq, int seq_len);
	bool		IsRulePartSolid(WORD irule, int ipos_beg, int ipos_end = -1);

	int		GetExpectedConflictInx(int grammar_conflict_inx);
	int		GetNumGrammarConflicts(int xpct_conflict_inx);

	int		GetFirstGrammarConflictInx(int xpct_conflict_inx);
	int		GetNextGrammarConflictInx(int xpct_conflict_inx, int grammar_conflict_inx);
				//
				// In these 2 methods the passed xpct_conflict_inx may have special values:
				//
				//		-3	--	all ambiguous grammar conflicts;
				//		-2	--	all single xpct owned grammar conflicts;
				//		-1	--	all unexpected grammar conflicts;
				//

	DWORD	GetBasicHashValue() const { return((DWORD)(grm_props.grms_file_date >> 4) ^ NumParsingStates() ^ (NumConflicts() << 3)); }

	static wchar_t		*GetParsingActionName(WORD action, wchar_t *buff_40_chars, bool want_first_cap = TRUE);
	static wchar_t		*GetShortParsingActionName(WORD action, wchar_t *buff_40_chars);

	WORD	AddGrammarConflict(WORD state, WORD symbol);
	bool		AddGrammarConflictAction(int conflict_inx, WORD wanted_action);

	WORD	AllocAnalysisState();

	inline WORD SymTableFromSymGrammar(WORD sym) const { return((sym < NonTerminalsBase) ? sym : sym-NonTerminalsBase+symbols.NumItems()); }
	inline WORD SymGrammarFromSymTable(WORD sym) const { return((sym < symbols.NumItems()) ? sym : sym-symbols.NumItems()+NonTerminalsBase); }

	bool		GenerateFirstsAndFollows(TAnalysisTable &res_table);
				//
				// Method creates matrix of "num_non_terminals" rows and "num_terminal_symbols" columns.
				//
				//	Meaning of the table slot:
				//
				//	   Hi-Byte	--	The byte is not 0 if non terminal (row) can start from the current terminal (column).
				//					First column corresponds to the empty symbol.
				//
				//	   Lo-Myte	--	The byte is not 0 if terminal symbol (column) can follow the current non terminal (row).
				//					First column corresponds to the EOF symbol.
				//

	bool		GenerateNonTerminalFirsts(TAnalysisTable &res_table, TAnalysisTable &prepared_firsts_and_follows);
				//
				// Method creates matrix of "num_non_terminals" rows and "num_non_terminals" columns.
				//
				// Only the Hi-byte is meaningful and it has the same structure as in the method above.
				//

	static inline bool AddToFirstsSet(WORD *syms_set, WORD term_sym)
	{
		WORD val = syms_set[term_sym] & FUN_FIRST_MASK;
		syms_set[term_sym] |= FUN_FIRST_FLAG;
		return((val != 0) ? FALSE : TRUE);
	}

	static inline bool AddToFollowsSet(WORD *syms_set, WORD term_sym)
	{
		WORD val = syms_set[term_sym] & FUN_FOLLOWS_MASK;
		syms_set[term_sym] |= FUN_FOLLOWS_FLAG;
		return((val != 0) ? FALSE : TRUE);
	}

	bool		GenerateRuleStartStatesIterIndex(TRulePosIterIndexLevel2 &index, void (*ProgressHandler)(void *context, int items_done, int total_items) = NULL, void *context = NULL);
	bool		GenerateNonTermDefnRulesIterIndex(TRulePosIterIndexLevel2 &index, void (*ProgressHandler)(void *context, int items_done, int total_items) = NULL, void *context = NULL);

	int		GetGrammarNesting();
	bool		GenerateGrammarName(TNameBuffer &buffer, const wchar_t *delim_seq);
				// These methods expect that hierarchy of the grammars is in correct state.

	int			GetNumGrammars(bool scan_hier = TRUE) const;		// Total number of grammars in the tree including the root grammar.
	int			GetGrammarIndex() const;							// Method returns zero based index of the grammar in the grammars tree.
	TGrammar	*GetGrammarByIndex(int igrm) const;				// Methos allows to retrieve any grammar of the tree starting from any grammar.

	int			GetNumGrammarConflictsHier() const;
	int			GetNumConflictingActionsHier() const;
	int			GetNumUnexpectedConflictsHier() const;

	void			SetProcessingResultHier(TGrammarProcessingResult res);

	enum { NumParsSttRulePositionsPerLine = 8 };		// Number of TRulePosition structures on one line when parsing state is dumped.
	enum { NumAnalysisTableSlotsPerLine = 20 };		// Number of WORDs on one line when analysis table of the grammar is emitted.

	void			EmitTables(TGenericConsole *console, const wchar_t *console_msg_prefix, TDestinationFile &rprt, const wchar_t *names_prefix, bool emit_ext_comments = FALSE);
					//
					// This method expects that report file is already prepared for writing. Method is closing the passed report after generating the data.
					// After writing data method writes success/failure trace message to console. It is still possible to check if emitting succeeded or not
					// by checking the error flag in the passed report file.
					//
					//	names_prefix	-  prefix of the name of the grammar structure and all supporting arrays and structures.
					//

	static const wchar_t *DEF_GRM_SRC_SUBDIR;
					// It is recommended to emit the grammar source code into the subdirectory with this name.

	bool			EmitTablesEx(TGenericConsole *console, const wchar_t *console_msg_prefix, const wchar_t *subdir_name, const wchar_t *file_name_suffix_with_extension,
							const wchar_t *structs_and_arrays_names_prefix, bool emit_ext_comments = FALSE);
					//
					// When name of the subdirectory is absolute or intermediate (look for details in comments to TPathHelper::IsRelativePath), emitted file
					// is placed into this directory. Otherwise, when subdir_name is relative, this name is considered as relative to the directory of the grammar
					// main source file. If this directory is not existing, it is created. When subdir_name is null or empty, destination file is placed into directory
					// of the main grammar source file. It is recommended to pass DEF_GRM_SRC_SUBDIR as a subdirectory name.
					//

	void			EmitGrammarTablesHier(TDestinationFile &rprt, const wchar_t *names_prefix, const wchar_t *parent_prefix, bool emit_ext_comments);
	void			EmitSingleGrammarConflictInfo(TDestinationFile &rprt, const wchar_t *names_prefix, int iconflict);

	static void	EmitSymbolsArray(TDestinationFile &rprt, TSymbolsArray &data, int num_values_per_line = 8);
					// Method emits 3 or more lines:
					//
					//    {
					//         ......   // Symbols of array.
					//    };
					//

	static const wchar_t *MakeBool(bool val, bool align = FALSE) { return((val == TRUE) ? ((align == TRUE) ? L" TRUE" : L"TRUE") : L"FALSE"); }
	static const wchar_t *MakeOptionalName(wchar_t *buffer_with_grammar_max_name_len_size, const wchar_t *name);
	static const wchar_t *MakeLexSubtypeEnumName(TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt, wchar_t *buff_40_chars);
	static const wchar_t *MakeNonTermRestrTypeEnumName(TNonTermRestrType val);
	static const wchar_t *MakeGrammarProcResEnumName(TGrammarProcessingResult val);
	static const wchar_t *GetProcResultName(TGrammarProcessingResult val);

	static wchar_t *StrDupe(const wchar_t *str);
	static wchar_t *StrDupe(TStringPtr &str_ptr);
	static wchar_t *StrDupe(TStrPtrInfo &str_ptr_info);

	inline static void FreeStr(wchar_t *str) { if (str != NULL) free(str); }

private:

	bool		ProcessGrammarsHier(int &grammars_inx_base, int &igrm, TGrammar **pgrm);
	void		WriteTraceMessage(TGenericConsole *console, const wchar_t *format, ...);
};

// ----------  Part 4. Grammar object iterators  -----------

class TRulesIterator
{
public:

	TRulesIterator(TGrammar &grammar) : m_grammar(grammar) { m_irule = 0; m_num_rules = m_grammar.NumRules(); }

	inline void		Reset()				{ m_irule = 0; }
	inline void		Seek(int index)		{ m_irule = index; }
	inline operator	bool() const			{ return(m_irule < m_num_rules); }
	inline void		operator ++ ()		{ if (m_irule < m_num_rules) m_irule++; }

	inline TGrammarRule	&CurrRule()		{ return(m_grammar.rules[m_irule]); }
	inline int				&CurrRuleIndex()	{ return(m_irule); }

protected:

	TGrammar		&m_grammar;
	int				m_irule;
	int				m_num_rules;
};

class TRuleSymbolsIterator
{
public:

	TRuleSymbolsIterator(TGrammar &grammar) : m_rules_iter(grammar) { m_isym = 0; if (m_rules_iter == TRUE) { m_num_symbols = CurrRule().Length(); CheckAndSearchForSymbol(); } }

	inline void		Reset()				{ m_isym = 0; m_rules_iter.Reset(); if (m_rules_iter == TRUE) { m_num_symbols = CurrRule().Length(); CheckAndSearchForSymbol(); } }
	inline operator	bool() const			{ return(m_rules_iter); }
	inline void		operator ++ ()		{ if (m_rules_iter == TRUE) { m_isym++; CheckAndSearchForSymbol(); } }

	inline TGrammarRule	&CurrRule()		{ return(m_rules_iter.CurrRule()); }
	inline WORD			CurrSymbol()		{ return(m_curr_symbol); }

protected:

	void CheckAndSearchForSymbol()
	{
		assert(m_rules_iter == TRUE);
		while (m_isym >= m_num_symbols)
		{
			m_isym = 0; ++m_rules_iter;
			if (m_rules_iter == TRUE)
				m_num_symbols = CurrRule().Length();
			else break;
		}

		if (m_rules_iter == TRUE)
			m_curr_symbol = CurrRule().symbols[m_isym];
	}

protected:

	int				m_isym;
	int				m_num_symbols;
	WORD			m_curr_symbol;
						// These fields are considered valid only when iteration is not complete.

	TRulesIterator		m_rules_iter;
};

class TParsingStatesIterator
{
public:  // Note: This class has no ctor because it is used in unions.

	inline void	Setup(TGrammar *grammar) { m_grammar = grammar; m_num_states = grammar->NumParsingStates(); m_istate = 0; }
					// This iterator iterates all parsing steas in the grammar. After calling method Setup() iterator points to the state 0.

	inline void		Reset()				{ m_istate = 0; }
	inline void		Seek(int index)		{ m_istate = index; }
	inline void		SeekToEnd()		{ m_istate = m_num_states; }
	inline operator	bool() const			{ return(m_istate < m_num_states); }
	inline void		operator ++ ()		{ if (m_istate < m_num_states) m_istate++; }

	inline TParsingState	&CurrState() const		{ return(m_grammar->parsing_states[m_istate]); }
	inline WORD			CurrStateIndex() const	{ return((WORD)m_istate); }

protected:

	TGrammar		*m_grammar;
	int				m_istate;
	int				m_num_states;
};

class TParsingStatePositionsIterator
{
public:  // Note: This class has no ctor because it is used in unions.

	inline void  Setup(TParsingState &pars_stt, int start_from = 0, int positions_to_iterate = -1) { m_rule_pos = pars_stt.DataPtr()+start_from; m_num_positions = (positions_to_iterate >= 0) ? positions_to_iterate : pars_stt.NumItems(); }

	inline void		SeekToEnd()		{ m_num_positions = 0; }
	inline operator	bool() const			{ return(m_num_positions > 0); }
	inline void		operator ++ ()		{ if (m_num_positions > 0) { m_rule_pos++; m_num_positions--; } }
	inline void		ShiftForward(int num_steps) { assert(num_steps <= m_num_positions); m_rule_pos += num_steps; m_num_positions -= num_steps; }

	inline TRulePosition	&CurrPos() const { return(*m_rule_pos); }
	inline int			NumAvalablePositions() const { return(m_num_positions); }

protected:

	TRulePosition		*m_rule_pos;
	int				m_num_positions;			// Number of remaining positions including the current position.

	friend struct TNonTermDefnRulesIterator;			// This class is defined as friend for perf reasons only. It is no making any special
												// manipulations that are not possible using the regular methods.
};

//-------------------  Inline methods  ----------------------------------

inline void TExpectedGrammarConflict::ReleaseObject()
{
	if (xpct_conflict_name != NULL)
	{
		free(xpct_conflict_name);
		xpct_conflict_name = NULL;
	}

	xpct_conflict_locations.ReleaseObject();
	resolution.ReleaseObject();

	if (nested_grammar != NULL)
	{
		delete nested_grammar;
		nested_grammar = NULL;
	}
}

#endif	// TextScan_GrammarStructure_H


