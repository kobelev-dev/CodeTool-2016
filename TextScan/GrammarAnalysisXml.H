//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	TextScan_GrammarAnalysisXml_H
#define	TextScan_GrammarAnalysisXml_H

#ifndef   TextScan_GrammarAnalysis_H
#include  "TextScan/GrammarAnalysis.H"
#endif
#ifndef   Common_XmlReader_H
#include  "Xml/XmlReader.H"
#endif
#ifndef   Common_XmlWriter_H
#include  "Xml/XmlWriter.H"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

extern TXmlDictRecord xtags_ConflictsAnalysisDatabase[];
extern TXmlDictRecord xtags_SavedConflictingActionAnalysisState[];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum TGrammarAnalysisDatabaseXmlTags
{
	// <FullRulePosition>
	gcnxt_frp_irule						= 3001,
	gcnxt_frp_isym						= 3002,
	gcnxt_frp_origin						= 3003,
	gcnxt_frp_istate						= 3004,
	gcnxt_frp_actsym					= 3005,

	// <GrammarDerivationPath>
	gcnxt_derv_path_iconflict			= 3010,
	gcnxt_derv_path_iaction				= 3011,
	gcnxt_derv_path_ipath				= 3012,
	gcnxt_derv_path_cnt_similar			= 3013,
	gcnxt_derv_path_path_len			= 3014,
	gcnxt_derv_path_step				= 3015,

	// <ConflictingActionAnalysisResults>
	gcnxt_act_res_iconflict				= 3020,
	gcnxt_act_res_iaction				= 3021,
	gcnxt_act_res_gen_result			= 3022,
	gcnxt_act_res_gen_steps			= 3023,
	gcnxt_act_res_gen_time				= 3024,
	gcnxt_act_res_gen_time_text		= 3025,
	gcnxt_act_res_deep_truncs			= 3026,
	gcnxt_act_res_pitrt_aborts			= 3027,
	gcnxt_act_res_num_paths			= 3028,
	gcnxt_act_res_derv_path			= 3029,

	// <GrammarConflictAnalysisResults>
	gcnxt_anres_iconflict				= 3030,
	gcnxt_anres_action_res				= 3031,

	// <GrammarConflictAnalysisProps>
	gcnxt_anpr_use_all_ntdrs			= 3040,
	gcnxt_anpr_search_ext_ctx			= 3041,
	gcnxt_anpr_max_path_len			= 3042,
	gcnxt_anpr_max_num_paths			= 3043,
	gcnxt_anpr_step_duration			= 3044,

	// <GrammarSummaryInfo>
	gcnxt_grmi_src_file_name			= 3050,
	gcnxt_grmi_src_file_length			= 3051,
	gcnxt_grmi_src_file_date			= 3052,
	gcnxt_grmi_src_file_dtx				= 3053,
	gcnxt_grmi_num_rules				= 3054,
	gcnxt_grmi_num_states				= 3055,
	gcnxt_grmi_num_conflicts			= 3056,
	gcnxt_grmi_num_grammars			= 3057,

	// <GrammarConflictsAnalysisData>
	gcnxt_andata_igrammar				= 3060,
	gcnxt_andata_ixpct					= 3061,
	gcnxt_andata_xpct_name			= 3062,
	gcnxt_andata_guid_database			= 3063,
	gcnxt_andata_writer_name			= 3064,
	gcnxt_andata_analysis_props		= 3065,
	gcnxt_andata_grm_info				= 3066,
	gcnxt_andata_nested_grm			= 3067,
	gcnxt_andata_conflict_data			= 3068,

	// <RootContainersDirectory>
	gcnxt_full_rule_pos					= 4200,
	gcnxt_derivation_path				= 4201,
	gcnxt_cfct_ation_anres				= 4202,
	gcnxt_conflict_anres					= 4203,
	gcnxt_conflict_anprops				= 4204,
	gcnxt_grammar_sumry				= 4205,
	gcnxt_grammar_analysis				= 4206,
};

class TGrammarAnalysisDatabaseXmlWriter
{
public:

	static void	DumpFullRulePos					(TXmlWriter &doc, WORD tag_id, TFullRulePosition &data);
	static void	DumpDerivationPath				(TXmlWriter &doc, WORD tag_id, int iconflict, int iaction, int ipath, TGrammarDerivationPath &data);
	static void	DumpActionAnalysisResults		(TXmlWriter &doc, WORD tag_id, int iconflict, int iaction, TConflictingActionAnalysisResults &data);
	static void	DumpConflictAnalysisResults	(TXmlWriter &doc, WORD tag_id, int iconflict, TGrammarConflictAnalysisResults &data);

	static void	DumpConflictAnalysisProps		(TXmlWriter &doc, WORD tag_id, TGrammarConflictAnalysisProps &data);
	static void	DumpGrammarSummary			(TXmlWriter &doc, WORD tag_id, TGrammar &data);
	static void	DumpGrammarAnalysisData		(TXmlWriter &doc, WORD tag_id, GUID &guid_db, TGrammarConflictAnalysisProps &props, TGrammar &data);
};

class TFullRulePositionXmlReader : public TXmlContainerHandler
{
public:

	TFullRulePositionXmlReader() { m_data = NULL; }

	void			SetDestination(TFullRulePosition *dest_buffer) { m_data = dest_buffer; }

	virtual void	BeginNodeProcessing(WORD tagId) { assert(m_data != NULL); m_data->qwValue = 0; m_data->origin = (TFullRulePosOrigin)0xFF; m_data->action_sym = 0xFFFF; }
	virtual void	FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems) { m_data = NULL; }

	virtual void	ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName);
	virtual void	ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName);
					// The name of the tag is provided only for error reporting. The callback handler should not check it.

protected:

	TFullRulePosition		*m_data;
};

class TGrammarAnalysisDatabaseXmlReader : public TXmlContainerHandler
{
public:

	TGrammarAnalysisDatabaseXmlReader(GUID &guid_db, TGrammarConflictAnalysisProps &analysis_props, TGrammar *dest_grammar,
												int igrammar_to_preserve = -1, int iconflict_to_preserve = -1, int iaction_to_preserve = -1);
		//
		//  This XML document reader reads the conflict analysis results data and stores this data in the destination grammar.
		//  The reader checks if XML document is compatible with the destination grammar or not. Note that only short names
		//  of the main grammar source files should match, not the directory names. The last 3 parameters allow skipping
		//  analysis results data for one conflicting action.
		//
		//  Note that if data for some conflicting action is missing in the XML document, then this action in the destination
		//  grammar will be simply not updated. Reader is not checking that data for all grammars/conflicts/actions is really
		//  present. Conflicts and actions may follow any order in the XML document. Reader insures that once the analysis
		//  data is present for some conflict, then the number of analysis result records will be strictly identical to the number
		//  of conflicting actions in the conflict.
		//
		//  Handling of the optional parameters of the ctor. Either all 3 params should be passed and they should be all non
		//  negative, or they should not be passed at all.
		//

	bool			ScanXmlSource(TXmlStream &xmlSource, TGenericConsole *console = NULL, int *errs_cnt = NULL);
					// This is realively simple method that hides the description of the tags array and the name
					// of the root container.

	bool			CheckGrammarsMatch() const { return(m_curr_grammar != NULL); }
					// This method should be called when the scanning is already finished. If the return value is FALSE,
					// this means that the destination grammar was not changed.

	virtual TXmlContainerHandler *OpenSubContainer(WORD tagId, TStringPtr &tagName);
					// The name of the tag is provided only for error reporting. The callback handler should not check it.

	virtual void	BeginNodeProcessing(WORD tagId);
	virtual void	FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems);

	virtual void	ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName);
	virtual void	ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName);
	virtual void	ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName);
	virtual void	ProcessGuidField(WORD tagId, GUID &tagValue, TStringPtr &tagName);
					// The name of the tag is provided only for error reporting. The callback handler should not check it.

protected:

	TGrammar						*m_curr_grammar;
	TGrammar						*m_curr_grammar_shadow;
										//
										// At the beginning of scanning the current grammar is NULL. It is set to non NULL after successful scanning
										// the grammar info, which means that XML doc is compatible with the destination grammar. When the scanning
										// is finished, this field allows to check results of the grammar info comparison.
										//
										// When container, that describes the nested grammar is entered, the current grammar is moved to
										// the shadow grammar and the current grammar becomes NULL. If processing of the ixpct field in the nested
										// grammar succeeds, then current grammar gets non NULL value and shadow grammar is not more used.
										// If processing of the ixpct field fails, then values of these fields are not changing.
										//
										// On the return from the nested grammar description the current grammar is restored back.
										//

	TGrammarConflict					*m_curr_conflict;
	int								m_curr_conflict_index;
										// Conflict index is needed only for preserving data in the selected action. There is no need to store the grammar
										// index for that because the grammar pointers are compared for this purpose. Action index is compared right
										// in the tag handler where its value is available as param of the method.

	TConflictingActionAnalysisResults		*m_curr_action;
	int								m_curr_action_next_ipath;
										// Path containers in the XML document should start from ipath=0 and they should have sequential ipath field.
										// Number of path containers should match the number of paths in the <NumPaths> field. Otherwise the parsing
										// error will be issued.

	TGrammarDerivationPath			*m_curr_path;
	int								m_curr_path_next_istep;
										// Step containers do not have an istep fields. This means that their sequence is not checked. Only the number
										// of steps should match the value in the <PathLen> field. Otherwise the parsing error will be issued.

	TFullRulePositionXmlReader			m_curr_step_reader;

private:

	GUID							&m_database_guid;
	TGrammarConflictAnalysisProps		&m_analysis_props;
	TGrammar						*m_destination_grammar;

	int								m_igrammar_to_preserve;
	int								m_iconflict_to_preserve;
	int								m_iaction_to_preserve;
};

enum TSavedConflictingActionAnalysisStateXmlTags
{
	// <RuleStartStatesIterator>
	gcsxt_rssi_irule							= 5010,
	gcsxt_rssi_rule_isym					= 5011,
	gcsxt_rssi_state_at_risym				= 5012,
	gcsxt_rssi_actsym_restr					= 5013,
	gcsxt_rssi_startup_flag					= 5014,
	gcsxt_rssi_stt_inx_len					= 5015,
	gcsxt_rssi_positions_avail				= 5016,

	// <RuleCallPlacesIterator>
	gcsxt_rcpi_parsing_state					= 5020,
	gcsxt_rcpi_non_term						= 5021,
	gcsxt_rcpi_action_sym					= 5022,
	gcsxt_rcpi_startup_flag					= 5023,
	gcsxt_rcpi_positions_avail				= 5024,

	// <NonTermDefnRulesIterator>
	gcsxt_ntdi_rule_start_state				= 5030,
	gcsxt_ntdi_rule_non_term				= 5031,
	gcsxt_ntdi_iter_diff_asyms				= 5032,
	gcsxt_ntdi_startup_flag					= 5033,
	gcsxt_ntdi_latest_irule					= 5034,
	gcsxt_ntdi_positions_avail				= 5035,

	// <BuildGrammarDerivationSlot>
	gcsxt_slot_islot							= 5040,
	gcsxt_slot_frp							= 5041,
	gcsxt_slot_upper_layer					= 5042,
	gcsxt_slot_rule_start_states_iter			= 5043,
	gcsxt_slot_rule_call_places_iter			= 5044,
	gcsxt_slot_nterm_defn_rules_iter			= 5045,
	gcsxt_slot_need_more_backw_ctx		= 5046,
	gcsxt_slot_need_more_forw_ctx			= 5047,
	gcsxt_slot_setup_cnt						= 5048,
	gcsxt_slot_positions_cnt					= 5049,
	gcsxt_slot_setup_cnts_count				= 5050,

	// <SavedConflictingActionAnalysisState>
	gcsxt_sas_database_guid				= 5101,
	gcsxt_sas_igrammar						= 5102,
	gcsxt_sas_iconflict						= 5103,
	gcsxt_sas_iaction						= 5104,
	gcsxt_sas_genres						= 5105,
	gcnxt_sas_gentime_text					= 5106,
	gcsxt_sas_numpaths						= 5107,
	gcsxt_sas_clev							= 5108,
	gcsxt_sas_cnt_steps						= 5109,
	gcsxt_sas_iter_setup_cnt				= 5110,
	gcsxt_sas_clev_bottom					= 5111,
	gcsxt_sas_wback_ctx					= 5112,
	gcsxt_sas_wforw_ctx					= 5113,
	gcsxt_sas_ctx_paths_pres				= 5114,
	gcsxt_sas_max_trans_path				= 5115,
	gcsxt_sas_max_shash_areas				= 5116,
	gcsxt_sas_derivation_slot				= 5117,

	// <RootContainersDirectory>
	gcsxt_rule_start_states_iter				= 5200,
	gcsxt_rule_call_places_iter				= 5201,
	gcsxt_nterm_defn_rules_iter				= 5202,
	gcsxt_build_grm_deriv_slot				= 5203,
	gcsxt_saved_analysis_state				= 5204,
};

class TSavedConflictingActionAnalysisStateXmlWriter
{
public:
	static void	DumpRuleStartStatesIterator		(TXmlWriter &doc, WORD tag_id, TRuleStartStatesIterator &data);
	static void	DumpRuleCallPlacesIterator			(TXmlWriter &doc, WORD tag_id, TRuleCallPlacesIterator &data);
	static void	DumpNonTermDefnRulesIterator	(TXmlWriter &doc, WORD tag_id, TNonTermDefnRulesIterator &data);

	static void	DumpBuildDerivationPathSlot		(TXmlWriter &doc, WORD tag_id, int ilayer, TBuildGrammarDerivationSlot &data);
	static void	DumpGrammarAnalysisState		(TXmlWriter &doc, WORD tag_id, GUID &core_db_guid, TGrammarConflictAnalysisHelper &data);
};

class TRuleStartStatesIteratorXmlReader : public TXmlContainerHandler
{
public:

	TRuleStartStatesIteratorXmlReader() { m_data = NULL; m_index2 = NULL; }

	void			SetDestination(TRuleStartStatesIterator *dest_buffer, TGrammar *grammar, TRulePosIterIndexLevel2 &index2) { m_data = dest_buffer; m_data->m_grammar = grammar; m_index2 = &index2; }

	virtual void	BeginNodeProcessing(WORD tagId);
	virtual void	FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems);

	virtual void	ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName);
	virtual void	ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName);
					// Name of the tag is provided to simplify the error reporting. App should not check it.
protected:

	TRuleStartStatesIterator			*m_data;
	TRulePosIterIndexLevel2			*m_index2;
	int								m_positions_available;
};

class TRuleCallPlacesIteratorXmlReader : public TXmlContainerHandler
{
public:

	TRuleCallPlacesIteratorXmlReader() { m_data = NULL; }

	void			SetDestination(TRuleCallPlacesIterator *dest_buffer, TGrammar *grammar) { m_data = dest_buffer; m_data->m_grammar = grammar; }

	virtual void	BeginNodeProcessing(WORD tagId);
	virtual void	FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems);

	virtual void	ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName);
	virtual void	ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName);
					// The name of the tag is provided only for error reporting. The callback handler should not check it.

protected:

	TRuleCallPlacesIterator				*m_data;
	int								m_positions_available;
};

class TNonTermDefnRulesIteratorXmlReader : public TXmlContainerHandler
{
public:

	TNonTermDefnRulesIteratorXmlReader() { m_data = NULL; m_index2 = NULL; }

	void			SetDestination(TNonTermDefnRulesIterator *dest_buffer, TGrammar *grammar, TRulePosIterIndexLevel2 &index2) { m_data = dest_buffer; m_data->m_grammar = grammar; m_index2 = &index2; }

	virtual void	BeginNodeProcessing(WORD tagId);
	virtual void	FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems);

	virtual void	ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName);
	virtual void	ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName);
					// The name of the tag is provided only for error reporting. The callback handler should not check it.

protected:

	TNonTermDefnRulesIterator			*m_data;
	TRulePosIterIndexLevel2			*m_index2;
	int								m_positions_available;
};

class TBuildGrammarDerivationSlotXmlReader : public TXmlContainerHandler
{
public:

	TBuildGrammarDerivationSlotXmlReader();

	void			SetDestination(TBuildGrammarDerivationSlot *dest_buffer, TGrammar *grammar, TRulePosIterIndexLevel2 &rule_start_states_index, TRulePosIterIndexLevel2 &non_term_defn_rules_index);

	virtual TXmlContainerHandler	*OpenSubContainer(WORD tagId, TStringPtr &tagName);
									// The name of the tag is provided only for error reporting. The callback handler should not check it.

	virtual void	BeginNodeProcessing(WORD tagId);
	virtual void	FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems);

	virtual void	ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName);
	virtual void	ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName);
					// The name of the tag is provided only for error reporting. The callback handler should not check it.

protected:

	TBuildGrammarDerivationSlot				*m_data;
	TGrammar								*m_grammar;
	TRulePosIterIndexLevel2					*m_rule_start_states_index_ptr;
	TRulePosIterIndexLevel2					*m_non_term_defn_rules_index_ptr;

	TFullRulePositionXmlReader					m_frp_reader;
	TRuleStartStatesIteratorXmlReader			m_iter1_reader;
	TRuleCallPlacesIteratorXmlReader			m_iter2_reader;
	TNonTermDefnRulesIteratorXmlReader		m_iter3_reader;
	bool										m_iterator_present;
};

class TSavedConflictingActionAnalysisStateXmlReader : public TXmlContainerHandler
{
public:

	TSavedConflictingActionAnalysisStateXmlReader() { m_core_db_guid = NULL; m_dest_data = NULL; }

	void			SetDestination(GUID &core_db_guid, TGrammarConflictAnalysisHelper &dest_buffer, TGrammar *root_grammar, int dest_igrammar);
					// Index of the destination grammar is given outside. This simplifies procedure of building the iterator indexes.
					// Later on this handler will check that this index matches the index of the grammar that is stored in the file.

	virtual TXmlContainerHandler	*OpenSubContainer(WORD tagId, TStringPtr &tagName);
									// The name of the tag is provided only for error reporting. The callback handler should not check it.

	virtual void	BeginNodeProcessing(WORD tagId);
	virtual void	FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems);

	virtual void	ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName);
	virtual void	ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName);
	virtual void	ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName) { }		// This container has only optional string fields.
	virtual void	ProcessGuidField(WORD tagId, GUID &tagValue, TStringPtr &tagName);
					// The name of the tag is provided only for error reporting. The callback handler should not check it.

protected:

	GUID									*m_core_db_guid;
	TGrammarConflictAnalysisHelper				*m_dest_data;

	TBuildGrammarDerivationSlotXmlReader		m_slot_reader;
	int										m_next_islot;
	int										m_max_trans_path_len;
};

#endif	// TextScan_GrammarAnalysisXml_H


