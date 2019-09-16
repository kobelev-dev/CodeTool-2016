//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Backus-Naur productions grammar processor.
//

#ifndef	TextScan_GrammarDefinitionParser_H
#define	TextScan_GrammarDefinitionParser_H

#ifndef   TextScan_GrammarStructure_H
#include  "TextScan/GrammarStructure.H"
#endif
#ifndef   TextScan_ParsingDriver_H
#include  "TextScan/ParsingDriver.H"
#endif

// ----------  Part 4. Grammar definition parser  ------------

class TGrammarDefinitionParser;

typedef  TStructsArray<PrEventHeader, 8, 16>  PrEventHeadersArray;

struct TGrammarRuleOrigin
{
	PrEventHeader			non_term_origin;		// Origin of the non terminal of the rule. Note that this non
													// terminal can be shared between several rules.
	PrEventHeader			rule_origin;				// Origin of the rule intro symbol. Note that for regular rules
													// this is either ":" or "|". For resolution rules this symbol is
													// the same to the rule non terminal.
	PrEventHeadersArray		symbol_origins;

	bool						rule_propagated;		// Rule was propagated into the nested grammar. This field
													// is set/checked only in the source rules. In resolution rules
													// this fields is always FALSE.

	// Note: These methods cannot be called when array of the rule origins is in read only mode.
	inline void FixObjectMove()	{ symbol_origins.FixObjectMove(); }
	inline void ReleaseObject()	{ symbol_origins.ReleaseObject(); }
	inline void WipeObject()		{ symbol_origins.WipeObject(); }
};

typedef  TStructsArrayEx2<TGrammarRuleOrigin, 8, 256>  TGrammarRuleOriginsArray;

// Middle part of the report names for the source grammar.
#define  SOURCE_GRAMMAR_NAME		L"_SOURCE_GRAMMAR_"

// File names for various report files.
#define  BASIC_REPORT_FNAME		L"basic_report.txt"
#define  CONV_REPORT_FNAME			L"followers_and_states.txt"
#define  CONFLICTS_INFO_FNAME		L"conflicts.txt"

class TGrammarDefnParserReportFile : public TDestinationFile
{
public:

	TGrammarDefnParserReportFile(TGrammarDefinitionParser *inst, const wchar_t *cgrm_name, int cgrm_index,
										int num_symbols, int num_non_terms, int num_rules, const wchar_t *report_fname);
				// Ctor writes stnd header about the passed grammar into the report.

	void WriteSectionHeader(const wchar_t *hdr_msg);
	void CloseReport(const wchar_t *error_msg_to_console);
	void DeleteReport();

protected:

	TGrammarDefinitionParser		&m_grdef_parser;
};

//
// These trace flags are stored in HIBYTE of MidLev parser trace flags.
//
enum TGrammarDefinitionParserTraceFlags
{
	gdtr_all			= 0xFF000000,
	gdtr_steps		= 0x01000000,		// This flag activates tracing on entry/exit of major grammar conversion functions.
										// Iterative functions also generate progress info under this flag.
	gdtr_reports		= 0x02000000,		// This tracing flag enables generation of grammar parsing and conversion reports.
										// These reports are text files that are stored on the disk. As of today there are 3 different
										// reports. At the end of generation of each report one trace line is emitted.
	gdtr_conflicts	= 0x04000000,		// This flag activates 3 trace locations. Trace happens on conflict creation and adding
										// conflicting actions into the conflict.

	#define TGDP_GENERAL        L"GrmDefPars"
};

class TGrammarDefinitionParser : public TGenericParser
{
protected:

	TGrammarDefinitionParser(TGrammar &dest_grammar, const wchar_t *dir_for_reports = NULL, TMidLevScanner *scanner = NULL, bool build_parsing_table = TRUE);
		// When the last parameter is FALSE, the grammar definition parser only scans the grammar sources.
		// It is not building the set of the parsing states, and it is not building the list of grammar conflicts.

public:

	TGrammar	&DestGrammar() { return(m_dest_grammar); }

	//
	// Inherited virtual methods.
	//

	WORD	GetParserIdentificationCode() const { return(0x93); }
	void		ResetParser();
	bool		Parse();

	enum  { MAX_APP_ID_VALUE = 20000 };

	static const wchar_t *DEF_REPORTS_SUBDIR;			// Name of the subdirectory for storing report files.

protected:

	//
	//  General methods group.
	//

	bool		GenerateGrammarNameForReporting(TFileNameBuffer &file_name_buff, TNameBuffer &name_buff, TGrammar &grm);
				// Method generates grammar name like "_RootGrammar_" or "_<first_xpct_conflict>_<next_xpct_conflict>_..._" plus
				// it ensures that passed file name buffer is long enough to store the report file name for this grammar.

	bool		PrepareReporsDirectory(bool emit_errors);
				// Method creates subdirectory "Reports" in the directory of the main source file if subdirectory for placing reports was not
				// explicitly given by the upper layer.

	wchar_t	*GenerateReportFileName(TFileNameBuffer &file_name_buff, const wchar_t *grm_name, int grm_index, const wchar_t *fname_suffix);
				// Method creates file name: "<m_reports_directory>/grc[_inx]<grm_name><fname_suffix>". It is expected that passed
				// file_name_buff is long enough to keep this name.

	void		GenerateBasicReport(TFileNameBuffer &fnb, const wchar_t *grm_name, int grm_index, TTerminalSymbolsArray &term_syms, TNonTerminalsArray &non_terms, TGrammarRulesArray &rules);
				//  Method sends 5 or 6 messages to the console plus it generates 1 text file report.

	// Ranges of the error code values are specified in TParsingErrorCodeBases enum.
	enum TGrammarDefnParserErrorCode
	{
		gerr_base							= 1300,

		// Parse group.
		gerr_sect_hdr_error					= 1301,		// Ctx only (5 times).
		gerr_sect_not_defined				= 1302,		// Ctx only (1 time).
		gerr_bld_tabs_skipped				= 1303,		// Eof only (4 times).
		gerr_xpcts_not_closed				= 1304,		// Eof only (1 time).

		// Scanning statements.
		gerr_warn_ctrl_stmt_error			= 1305,		// Ctx only (2 times).
		gerr_grm_descr_stmt_error			= 1306,		// Ctx only (3 times).
		gerr_grm_rev_stmt_error			= 1307,		// Ctx only (4 times).
		gerr_lang_type_stmt_error			= 1308,		// Ctx only (7 times).
		gerr_symbol_def_stmt_error			= 1309,		// Ctx only (7+4 times) + ConversionError (1 time).
		gerr_ignore_lex_stmt_error			= 1310,		// Ctx only (4+1 times).
		gerr_error_lex_stmt_error			= 1311,		// Ctx only (6+1 times).
		gerr_axioma_stmt_error				= 1312,		// Ctx only (4+1 times).
		gerr_rule_stmt_error				= 1313,		// Ctx only (38+6 times).
		gerr_xpct_cfct_stmt_error			= 1314,		// Ctx only (7+3 times).
		gerr_xpct_location_error				= 1315,		// Ctx only (4 times).
		gerr_xpct_resolution_error			= 1316,		// Ctx only (26+1 times).
		gerr_public_sym_stmt_error			= 1317,		// Ctx only (4+1 times).

		// Misc.
		gerr_err_proc_non_term_fwd			= 1319,		// Ctx only (1 time).

		// Child grammar preparation.
		gerr_chld_grammar_prep_error		= 1323,		// Conversion only (28 times).

		// Source grammar integrity check group.
		gerr_symbols_missing				= 1329,		// Eof only (1 time).
		gerr_non_terms_missing				= 1330,		// Eof only (1 time).
		gerr_axioma_missing				= 1331,		// Eof only (1 time).
		gerr_rules_missing					= 1332,		// Eof only (1 time).
		gerr_list_seq_restr_violation			= 1333,		// Intro lexema (2 times).
		gerr_public_non_term_error			= 1334,		// Intro lexema (1 time).
		gerr_unused_terminal				= 1335,		// Intro lexema (1 time).
		gerr_no_lhs_for_non_term			= 1336,		// Intro lexema (1 time) - No rules where non terminal is used in the left hand side.
		gerr_no_rhs_for_non_term			= 1337,		// Intro lexema (1 time) - No rules where non terminal is used in the right hand side.
		gerr_identical_non_terms			= 1338,		// Intro lexema (1 time)

		// Generic reporting group.
		gerr_preparing_reports_dir			= 1340,		// ConversionError (3 times).

		// Generate scanning reports group.
		gerr_scanning_rprt					= 1342,		// ReportMessage (6 times).
		gerr_no_mem_for_non_terms_list	= 1343,		// ConversionError (1 time).

		// Convert grammar group.
		gerr_no_mem_for_flwrs_rprt			= 1350,		// ConversionError (1 time).
		gerr_no_mem_for_work_buffer		= 1351,		// ConversionError (1 time).
		gerr_bad_firsts_follows_set			= 1352,		// ConversionError (2 times).
		gerr_unused_rule					= 1353,		// Intro lexema (1 time). (WARN)
		gerr_cfct_location_markers			= 1354,		// ConversionError (6 times) + ConversionWarning (3 times).
		gerr_conversion_rprt				= 1355,		// ReportMessage (6 times).

		// Assign parsing states group.
		gerr_no_mem_for_analysis_table		= 1356,		// ConversionError (1 time).
		gerr_cannot_propagare_state		= 1357,		// ConversionError (2 times).
		gerr_no_mem_for_new_pars_state	= 1358,		// ConversionError (2 times).
		gerr_no_mem_for_prpg_work_list	= 1359,		// ConversionError (2 times).
		gerr_no_mem_for_pars_stts_set		= 1360,		// ConversionError (1 time).
		gerr_no_mem_for_rule_position		= 1361,		// ConversionError (1 time).
		gerr_too_many_positions_in_stt		= 1362,		// ConversionError (1 time).
		gerr_too_many_errors				= 1363,		// ConversionError (1 time).
		gerr_aborted_by_the_user			= 1364,		// ConversionError (1 time).

		// Assign resolve actions group.
		gerr_no_mem_for_conflits			= 1368,		// ConversionError (3 times).

		// Support tables group.
		gerr_no_mem_for_slr_prpg_info		= 1372,		// ConversionError (3 times).
		gerr_no_mem_for_spec_firsts		= 1373,		// ConversionError (1 time).
		gerr_no_mem_for_followers_list		= 1374,		// ConversionError (2 times).

		gerr_max							= 1399,
	};

	void		ReportMessage(TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...);
	void		ReportWarning(TLexema &location_lex, TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...);
	void		ReportWarning(PrEventHeader &origin, TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...);
	void		ReportError(TLexema &location_lex, TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...);
	void		ReportError(PrEventHeader &origin, TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...);
	void		ReportConversionWarning(TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...);
	void		ReportConversionError(TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...);
	void		ReportErrWarnInternal(PrEventHeader *ctx, TLexErrorClass err_class, int msg_code, const wchar_t *format, va_list vargs);
	void		DeleteReportFile(const wchar_t *fname);

	//
	//  Grammar parsing related structures.
	//

	// This structure stores info about location markers only when the rule is scanned. When the rule is saved
	// into the grammar, the loc markers of the rule are also saved into the grammar and info from this structure
	// is dismissed.
	struct TConflictLocationMarkerInfo
	{
		TRulePosition		position;

		int				loc_index;					// Index of localtion object in the array of known location objects when scanned location name
													// is already known. The value is -1 when name of location is not registered.
		wchar_t			*loc_name;					// Name of the location when this name is not registered yet or NULL. When this structure is part
													// of array, either its loc_index field should be valid (i.e. index >= 0) or its loc_name field
													// should be not NULL.
		PrEventHeader	loc_name_origin;			// Origin of the location marker name lexema.

		inline void  ReleaseObject()	{ if (loc_name != NULL) { TGrammar::FreeStr(loc_name); loc_name = NULL; } }
		inline void  WipeObject()		{ loc_name = NULL; }
	};

	typedef  TStructsArrayEx<TConflictLocationMarkerInfo, 16, 64>  TConflictLocationMarkerInfosArray;

	// Storage for objects from the source grammar.
	struct TSourceGrammarSupp
	{
		TTerminalSymbolsArray				m_symbols;
		PrEventHeadersArray				m_symbol_areas;

		TIgnoreLexRecordsArray			m_ignore_lex;
		PrEventHeadersArray				m_ignore_lex_areas;

		TErrorLexRecordsArray				m_error_lex;
		PrEventHeadersArray				m_error_lex_areas;

		TNonTerminalsArray				m_fwd_def_non_terminals;
		PrEventHeadersArray				m_fwd_def_non_term_areas;

		TNonTerminalsArray				m_non_terminals;
		PrEventHeadersArray				m_non_term_areas;

		TGrammarRulesArray				m_rules;
		TGrammarRuleOriginsArray			m_rule_intro_areas;

		TSymbolsArray					m_non_term_defns_complete;
											// Values of the elements in this array are TRUE if rules for the non term were noticed
											// and they were followed by definitions for some other non terminal. Non consequtive
											// defintions for the same non terminal generate warnings.

		TExpectedConflictLocationsArray		m_location_objects;
		PrEventHeadersArray				m_loc_object_intro_areas;

		TNestedProcessingResultsArray		m_nested_processing_results;
		PrEventHeadersArray				m_nested_proc_res_intro_areas;

		wchar_t *GetSymbolName(WORD sym, wchar_t *buff_40_chars)
		{
			if (sym >= NonTermForwardDefnsBase && sym < NonTermForwardDefnsBase+m_fwd_def_non_terminals.NumItems())
			{
				wchar_t *sym_name = m_fwd_def_non_terminals[sym-NonTermForwardDefnsBase].symbol_name;
				assert(sym_name != NULL);
				return(sym_name);
			}

			return(TGrammarSymbolsHelper::GetSymbolName(m_symbols, m_non_terminals, sym, buff_40_chars));
		}

		WORD FindSymbol(TLexema &lex)
		{
			WORD sym = 0;
			if (lex.type == ltx_name)
				sym = FindNonTerminalForwardDef(lex.str_value);
			if (sym == 0)
				sym = TGrammarSymbolsHelper::FindSymbol(m_symbols, m_non_terminals, lex);
			return(sym);
		}

		WORD FindSymbol(TStrPtrInfo &name_ptr)
		{
			WORD sym = FindNonTerminalForwardDef(name_ptr);
			if (sym == 0)
				sym = TGrammarSymbolsHelper::FindSymbol(m_symbols, m_non_terminals, name_ptr);
			return(sym);
		}

		void BumpSymbolRuleUse(WORD sym, bool inc_counts = TRUE)
		{
			if (sym > TerminalSymbolsBase && sym < m_symbols.NumItems())
			{
				if (inc_counts = TRUE)
					m_symbols[sym].num_rule_uses++;
				else m_symbols[sym].num_rule_uses--;
			}
			else if (sym >= NonTermForwardDefnsBase && sym < NonTermForwardDefnsBase+m_fwd_def_non_terminals.NumItems())
			{
				assert(m_fwd_def_non_terminals[sym-NonTermForwardDefnsBase].symbol_name != NULL);

				if (inc_counts = TRUE)
					m_fwd_def_non_terminals[sym-NonTermForwardDefnsBase].num_rhs_rule_uses++;
				else m_fwd_def_non_terminals[sym-NonTermForwardDefnsBase].num_rhs_rule_uses--;
			}
			else if (sym >= NonTerminalsBase && sym < NonTerminalsBase+m_non_terminals.NumItems())
			{
				if (inc_counts = TRUE)
					m_non_terminals[sym-NonTerminalsBase].num_rhs_rule_uses++;
				else m_non_terminals[sym-NonTerminalsBase].num_rhs_rule_uses--;
			}
			else
			{
				// Symbol value is bogus.
				assert(FALSE);
			}
		}

		void	ReleaseArrays()
		{
			m_symbols.Clear();
			m_symbol_areas.Clear();
			m_ignore_lex.Clear();
			m_ignore_lex_areas.Clear();
			m_error_lex.Clear();
			m_error_lex_areas.Clear();

			m_non_terminals.Clear();
			m_non_term_areas.Clear();
			m_fwd_def_non_terminals.Clear();
			m_fwd_def_non_term_areas.Clear();
			m_rules.Clear();
			m_rule_intro_areas.Clear();
			m_non_term_defns_complete.Clear();

			m_location_objects.Clear();
			m_loc_object_intro_areas.Clear();
			m_nested_processing_results.Clear();
			m_nested_proc_res_intro_areas.Clear();
		}

	private:

		WORD FindNonTerminalForwardDef(TStrPtrInfo &name_ptr)
		{
			for (WORD ifwd=0; ifwd<m_fwd_def_non_terminals.NumItems(); ++ifwd)
			{
				wchar_t *fwd_def_name = m_fwd_def_non_terminals[ifwd].symbol_name;
				if (fwd_def_name != NULL && name_ptr == fwd_def_name)
				{
					return(NonTermForwardDefnsBase+ifwd);
				}
			}

			// Passed name is not a name of any forward non term definition.
			return(0);
		}
	};

	// Parsing support info for single expected conflict.
	struct TExpectedConflictSupp : public TListItem
	{
		TExpectedConflictSupp(TExpectedConflictSupp *parent) { m_xpct_proto.xpct_conflict_name = NULL; m_parent_supp = parent; if (parent != NULL) parent->m_child_supps.AppendItem(this); }
		~TExpectedConflictSupp() { if (m_xpct_proto.xpct_conflict_name != NULL) TGrammar::FreeStr(m_xpct_proto.xpct_conflict_name); ReleaseChildren(); }

		TExpectedGrammarConflict			m_xpct_proto;
		PrEventHeader					m_intro_area;

		TGrammarRulesArray				m_resolution_rules;
		TGrammarRuleOriginsArray			m_res_rule_intro_areas;
											//
											// The left hand non terminals of the resolution rules are equal to zero when rules are stored
											// in this structure. Non terminals are assigned to the rules later when these rules are copied into
											// the nested grammar.
											//
											// Number of rules in this array is strictly identical to the length of the resolution array in
											// the description of the expected conflict.
											//
											// Axioma of the main grammar cannot be present in the right hand side of any resolution
											// rule. This is explicitly checked by the scanning code.
											//
											// Note that non term area and rule intro area are identical in the resolution rules.
											//

		TSymbolsArray					m_terminal_symbols_bk_refs;
		TSymbolsArray					m_non_terminals_bk_refs;

		TSymbolsArray					m_generated_rules_bk_refs;
											// Length of this array is the number of rules that were taken from the source grammar into
											// the current grammar. The value of each element of the array is an index of the rule in the src
											// grammar.

		TExpectedConflictSupp				*m_parent_supp;
		TList								m_child_supps;

		void ReleaseChildren()
		{
			while (m_child_supps.IsEmpty() == FALSE)
			{
				TExpectedConflictSupp *supp = (TExpectedConflictSupp*)m_child_supps.GetFirst();
				m_child_supps.RemoveItem(supp);
				delete supp;
			}
		}
	};

	bool		GetIntroOrigin(PrEventHeader &hdr, WORD scanning_phase_hierarchial_sym);
				// Method returns src info for: terminals, ignore_lex, error_lex, non_terms, rules, loc_objects, nested_results;
				// Besides that, it returns info from the hierarchy of expected conflicts for: resolution_rules, xpct_conflicts.

	bool		GetRuleOriginHier(PrEventHeader &hdr, WORD sym, int &inx_base, TExpectedConflictSupp &ctx);
	bool		GetExpectedConflictOriginHier(PrEventHeader &hdr, WORD sym, int &inx_base, TExpectedConflictSupp &ctx);
				// Helper methods for GetIntroOrigin().

	bool		GetPreparedGrammarSymbolOrigin(PrEventHeader *&org_info, int igrammar, WORD sym);
	bool		GetPreparedGrammarRuleOrigin(TGrammarRuleOrigin *&org_info, int igrammar, int irule);
				// These methods should be primarily used when the scanning is complete. The first method allows
				// picking up the info for terminal symbols and for non terminals.

	bool		GetGrammarSuppHier(TExpectedConflictSupp *&grm_supp, int igrammar, int &igrm_base, TExpectedConflictSupp &ctx);
				// This method expects that hier of supps is completely built.

	wchar_t	*PrepareSrcLocInfo(PrEventHeader &hdr, wchar_t *line_num_buff, int len_buff);
				// Return value is the name of the file. Buffer is filled either with line number or with text "(fileOffs=%ld)".

	WORD	FindObjectByAppId(WORD app_id);
				// Method searches for symbols, non terminals, rules, nested proc results and expected conflicts.
				// Only these 5 object types can have application ids.

	WORD	FindObjectByAppIdHier(WORD app_id, int &rules_inx_base, int &xpct_conflicts_inx_base, TExpectedConflictSupp &ctx);
				// Helper method for FindObjectByAppId.

	bool		CheckLexTypeMapping(TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt, PrEventHeader &conflict_origin);
				// Method returns TRUE if processing for passed lex type and optionally subtype is not defined yet.
				// Return value is FALSE if there is a conflict plus it returns origin of the conflicting definition.

	bool		CheckName(PrEventHeader &ctx, TStrPtrInfo &name_ptr, TGrammarDefnParserErrorCode err_code);
				// Method ensures that name is not too long and that no object with this name is defined yet.

	WORD	CheckNameHier(TStrPtrInfo &name_ptr, int &rules_inx_base, int &xpct_conflicts_inx_base, TExpectedConflictSupp &ctx);
				// This is worker method for CheckName().

	void		DoTheScanning();

	void		ScanWarningsControlStatement();
	void		ScanGrammarDescription(TLexema &lex_beg);
	void		ScanGrammarRevision(TLexema &lex_beg);
	void		ScanLanguageTypeDefinition(TLexema &lex_beg);

	void		ScanTerminalSymbolDefinition(TLexema &lex_beg);
	void		ScanIgnoreLexTypeDefinition(TLexema &lex_beg);
	void		ScanErrorLexTypeDefinition(TLexema &lex_beg);
	void		ScanAxiomaDefinition(TLexema &lex_beg);
	void		ScanRuleDefinition(TLexema &lex_beg);

	void		ScanExpectedConflictDefnHeader(TLexema &lex_beg);
	void		ScanConflictLocationReferences(TLexema &lex_beg);
	void		ScanConflictResolutionDefinition(TLexema &lex_beg);
	void		ExitExpectedConflictDefinition(TLexema &lex_end);

	void		DecrementXpctConflictRefCountsHier(TExpectedConflictSupp &xpct);

	bool		SaveRuleRightHandSideSymbol(WORD sym, TLexema &lex, PrEventHeadersArray &rule_symbol_areas);
	bool		SaveGrammarRule(PrEventHeader &context, WORD non_term, WORD rule_app_id, const wchar_t *rule_app_id_name,
									TConflictLocationMarkerInfosArray &location_markers_array, PrEventHeader &rule_non_term_intro_area,
									PrEventHeader &rule_intro_area, PrEventHeadersArray &symbol_areas);

	void		ScanPublicNonTerminalsStatement();

	TLexemaType	GetLexema(TLexema &lex, bool ignore_comments_and_eols = TRUE);

	void		ScanToSemicolon(TLexema &lex);
	void		ScanToClosingCurvedBracket(TLexema &lex);

	bool		ScanLexTypeAndSubtype(TLexema &lex, TLexemaType &lext, bool &check_subt, TLexSubtype &subt, TGrammarDefnParserErrorCode err_code);
				// In case of success the parameter lex contains lexema that stays after the recognized lext/subt. This lexema
				// can be either a valid or an invalid follower. In case of error method scans to the eof or to the semicolon.

	bool		ScanObjectId(TLexema &lex, WORD &object_app_id, wchar_t *app_id_name_buffer, WORD exception_sym, TGrammarDefnParserErrorCode err_code);
				// Method expects that the "__id" keyword is already scanned and passed in as the first parameter. Return value
				// shows if a correct id was retrieved or not and in case of success the param lex contains the right parenthesis.
				// Note that input stream "__id(0)" results in the ret value equal to TRUE and object_app_id value equal to zero.

	bool		GetObjectIdFromMacroDefnsTable(TLexema &err_ctx, const wchar_t *macro_prefix, TStrPtrInfo &symbol_name_ptr, WORD &app_id, TGrammarDefnParserErrorCode err_code);
				// Return value is FALSE when the macro definition is existing and its body is either not a pure number or this
				// number it is out of the allowed range for app ids (0 ... MAX_APP_ID_VALUE) or some object with this id is
				// already existing.

	bool		GetNameOfTheCurrentMacro(TLexema &err_ctx, wchar_t *buffer_with_grammar_max_name_len_size);
				// If current scanning context is a macro context and this macro consists of single a number or a charconst,
				// the name of this macro is placed into the passed buffer and the return value is TRUE.

	bool		CheckObjectIdValue(TLexema &err_ctx, __int64 value, WORD exception_sym, TGrammarDefnParserErrorCode err_code);
				// Method checks that passed value fits into the proper range and that object with passed id is not yet defined.

	WORD	RegisterNonTerminal(TLexema &lex, bool fill_rules_sect, TGrammarDefnParserErrorCode err_code);
				// When this method is called, it should be known that non terminal with passed name is not existing.
				// Method is registering non term in the SOURCE grammar.

	WORD	ConvertForwardDefIntoNonTerminal(PrEventHeader &ctx, WORD forward_def_sym, WORD rules_sect, TGrammarDefnParserErrorCode err_code);
				// This method copies record from one list to another, kiils symbol_name field in the original record
				// and fixes symbol value in regular rules and all conflict resolution rules.

	void		FixNonTermForwardDefSymHier(TExpectedConflictSupp &ctx, WORD forward_def_sym, WORD non_term_sym);
				// Method fixes the passed xpct and all its children recursively.

	void		SourceGrammarIntegrityCheck(TLexema &lex_ctx);
				// This method only reports errors and warnings. It is not modifying the data structures.

	//
	//  Grammar preparation methods.
	//

	void		DoThePostScanningWork();

	void        PrepareGrammarsHier(TGrammar &grm, TExpectedConflictSupp &grm_supp);
	void        PrepareSingleGrammar(TGrammar &grm, TExpectedConflictSupp &grm_supp);

	bool        PrepareHelperMask(TSymbolsArray &mask, int num_items_needed);
	void        PrepareLexToSymConverter(TGrammar &grm);
	void        PrepareLexToSymConverterSlot(TGrammar &grm, TLexemaType lext, bool check_subt, TLexSubtype subt, WORD sym_value);
	void        PrepareNonTermTransparencyFields(TGrammar &grm);

	//
	//  Grammar conversion methods.
	//

	void		ConvertGrammarsHier(TGrammar &grm);
	void		ConvertSingleGrammar(TGrammar &grm);
				// When this method is called, conversion method should be already set in the grammar structure.

	WORD	AllocateNewParsingState();
				// This method actually makes intial allocation of analysis table or it enlarges existing table if needed.
				// If return value is 0xFFFF, this means that allocation failed and error is already reported inside.

	void		AssignParsingStates();
	bool		AddSimpleRuleStartPositions(TParsingState &stt_set, WORD non_term);
	bool		AddRuleStartPositions(TParsingState &stt_set, WORD non_term, WORD *beta_seq, int beta_seq_len, WORD action_sym);
	bool		AddPositionsAndTasks(TParsingState &stt_set, WORD non_term, WORD action_sym);
	bool		AppendRulePosition(TParsingState &stt_set, WORD irule, WORD ipos, WORD action_sym);
	bool		AppendParsingState(TParsingState &stt_set);

	void		AssignResolveActions();
	bool		ProcessConflict(WORD conflict_state, WORD conflict_sym, int  irule, WORD existing_analysis_table_cell_value);

	bool		GeneratePropagationInfo(TParsingStatesArray &res_array);
				// This method is used for SLR case only. Function generates array of TRulePosition arrays.
				// Length of "res_array" is the same to the number of non terminals in the grammar. Each particular array
				// member corresponds to one non terminal. Array member itself consists of zero or more rule positions.
				// All these positions are starting positions. Members of array are sorted by the rule index.

	bool		GenerateSpecialFirstSetsForRules(TAnalysisTable &res_table);
				// This method is used for LR case only. Method creates matrix of "num_rules" rows by "num_terminal_symbols"
				// columns.

	void		GenerateFirstsSet(WORD *set, WORD *seq, int seq_len);
				// This method is used for LR case only. First param is buffer with "num_terminal_symbols" slots.
				// The buffer is filled with TRUE/FALSE values.

	inline void	ClearNewStateHashTable() { memset(m_new_stt_hash, 0xFF, sizeof(m_new_stt_hash)); }
	inline WORD	SymTableFromSymGrammar(WORD sym) const { return((sym < NonTerminalsBase) ? sym : sym-NonTerminalsBase+m_conv_grm->symbols.NumItems()); }
	inline WORD	SymGrammarFromSymTable(WORD sym) const { return((sym < m_conv_grm->symbols.NumItems()) ? sym : sym-m_conv_grm->symbols.NumItems()+NonTerminalsBase); }

	static int __cdecl	CompareRulePositions(const TRulePosition *pos1, const TRulePosition *pos2);

	void		GenerateFollowersAndParsingStatesReport(TGrammar &grammar, TAnalysisTable &followers_table);

	//
	//  Grammar post processing methods.
	//

	void		PostProcessGrammarsHier(TGrammar &grm);
	void		PostProcessSingleGrammar(TGrammar &grm);

	bool		CheckLocObjectPosition(TGrammar &grm, TSymbolsArray &states_mask, TRulePosition &pos);
				// Method iterates all parsing states and removes parsing states that are not applicable to the passed rule position
				// from the passed states mask. Return value shows if any state was applicable to the passed rule position or not.

	void		ReportActionCodesAmbiguity(TGrammar &grm, int iconflict, TExpectedGrammarConflict &xpct, int itarg, int icode1, int icode2);
	const wchar_t		*PreapeResolutionActionCode(TGrammar &grm, WORD code, wchar_t *buffer_80_chars);

	void		GenerateConflictsReport(TFileNameBuffer &file_name_buff, const wchar_t *grm_name, TGrammar &grammar);

protected:

	TGrammar					&m_dest_grammar;
									// Destination grammar is the reference to the external grammar object. In typical scenario this
									// reference is passed to TGrammarDefinitionParserHelper ctor. This ctor passes this ref to the ctor
									// of this class. Result of the grammar definition parsing is stored in this external grammar.
									// Note that when the source files are scanned, this grammar is not used at all.

	bool							m_build_parsing_table;				// This is rarely used flag that instrcts to skip building
																	// the set of the parsing states.
	TGrammarProps				m_src_props;
	WORD						m_axioma;

	TSourceGrammarSupp			m_src_data;
	TExpectedConflictSupp			m_xpcts_root;						// Root of the TExpectedConflictSupp instances tree.
																	// This field also contains info about relation between symbols and
																	// rules of the root grammar to syms and rules of the src grammar.
	TExpectedConflictSupp			*m_cxpct;
	int							m_cnt_nesting;

	TGrammarSectionType			m_curr_section;
	WORD						m_rule_sects_cnt;					// Delimiter <rules> starts the new rules section.

	TSymbolsArray				m_sym_buffer;

	bool							m_warn_on_unused_symbols;
	bool							m_warn_on_rules_without_ids;
	bool							m_warn_on_unused_loc_markers;

	WORD						m_latest_rule_non_term;				// This field is needed only for detection of non consequtive rules
																	// for the same non terminal.
	bool							m_scanning_phase;
	TFileNameBuffer				m_reports_directory;

private:

	// This structure is used in LR algorithm only.
	struct TGenClosureTask : public THashedStructsArrayItem
	{
		WORD		non_term;
		WORD		irule;					// Rule index or 0xFFFF for initial task.
		WORD		*first_syms_set;			// This field can be NULL if set is empty.
		WORD		action_sym;

		inline TGenClosureTask() { }

		inline TGenClosureTask(WORD nt, WORD ir, WORD *fss, WORD as) { non_term = nt; irule = ir; first_syms_set = fss; action_sym = as; }
				// This ctor is needed because MS compiler claims that all derived types are non aggregates.

		static inline bool Compare(TGenClosureTask &t1, TGenClosureTask &t2) { return(t1.non_term == t2.non_term && t1.irule == t2.irule && t1.action_sym == t2.action_sym); }
	};

	// This structure is used in LR conversion algorithm only.
	class TGenClosureTasksArray : public THashedStructsArray<TGenClosureTask, 512, 2048, 128>
	{
		public:

		TGenClosureTasksArray() : THashedStructsArray<TGenClosureTask, 512, 2048, 128>() { }

		DWORD	CalcHashValue(TGenClosureTask &item) { return(item.non_term ^ item.irule ^ item.action_sym); }
		int		CompareRecords(TGenClosureTask &i1, TGenClosureTask &i2) { return(TGenClosureTask::Compare(i1, i2) == FALSE); }
	};

	//
	// Grammar conversion related fields are reset only when conversion starts. They are not inited during the general reset.
	//

	TGrammar					*m_conv_grm;

	bool							m_slr_method;							// The value of this field (FALSE) is currently set only in the code.
																		// Support for SLR is still present but it should be enabled with #define.
	TParsingStatesArray			m_slr_propagation_info;

	TSymbolsArray				m_lr_work_set;							// Array of TRUE/FALSE values. Length of this array is NumTerminals().

	TGenClosureTasksArray			m_lr_gen_contexts_task;

	TAnalysisTable				m_lr_rule_firsts;							// This is matrix of TRUE/FALSE values.
																		// It has NumRules() rows and NumTerminals() columns.
	enum { NEW_STT_BUCKETS = 1024 };

	WORD						m_new_stt_hash[NEW_STT_BUCKETS];		// Note: Zero is a valid position index.
																		// Bucket is empty when value of the index is 0xFFFF.

	TAnalysisTable				m_firsts_and_follows;					// This field is used in both methods.

	friend class TGrammarDefnParserReportFile;
	friend class TGrammarDefinitionParserHelper;
	friend class TLangViewLoadGrammarHelper;
	friend class THtmlGrammarSourceFile;
};

class TGrammarDefinitionParserHelper
{
public:

			TGrammarDefinitionParserHelper(TMidLevScanCbkHandler &midlev_cbk, TGrammar &dest_grammar,
												const wchar_t *directory_for_reports = NULL, bool build_parsing_table = TRUE);
					//
					//	The midLev callback is a required parameter. Syntax errors in the grammar definition are reported
					//	to this callback. There is no other way to get them.
					//

			TMidLevScanner			&Scanner() { return(m_driver.Scanner()); }
										// This is a read accessor to the mid lev scanner.

			TGrammarDefinitionParser	&Parser() { return(m_parser); }

	//
	// External entry point.
	//
	bool		ParseGrammarDefinitionFile(const wchar_t *file_name_with_the_grammar_definition,
											DWORD enabled_tracing_flags = 0, int *num_syntax_errors = NULL);
					//
					//  Note on passing the grammar file name and the reports directory. If these names are passed as
					//  relative paths, they are treated as relative paths to the current directory of the process.
					//
					//  When the reports directory is passed as NULL, all reports are placed into the subdirectory "Reports"
					//  of the main grammar source file directory.
					//
protected:

	TGrammarDefinitionParser		m_parser;
	TParsingDriver				m_driver;
};


#endif	// TextScan_GrammarDefinitionParser_H


