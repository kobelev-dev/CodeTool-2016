//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/FormatNumber.H"
#include  "Common/DateTimeHelper.H"
#include  "TextScan/GrammarAnalysis.H"
#include  "TextScan/GrammarAnalysisXml.H"

//------------------------------------------------------------------------
//  ===================  Static finctions  =======================
//------------------------------------------------------------------------

void WriteTooManyPathsNote(TXmlWriter &doc)
{
	doc.WriteLine();
	doc.WriteXmlComment(NULL, L"Note that when the analysis ends with \"gres_too_many_paths\", the number of stored", crlf_add_add);
	doc.WriteXmlComment(NULL, L"derivation paths can be less than the limit taken from the analysis props because similar", crlf_add_add);
	doc.WriteXmlComment(NULL, L"paths are filtered out after stopping the analysis process.", crlf_add_add);
}

//------------------------------------------------------------------------
//  ==============  Grammar Analysis Xml Schema  ==================
//------------------------------------------------------------------------

// <FullRulePosition>
static TXmlDictRecord xtags_FullRulePosition[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"iRule"),						gcnxt_frp_irule,				},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"iSym"),						gcnxt_frp_isym,				},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_string,		DefTag(L"Origin"),						gcnxt_frp_origin,				},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"iState"),						gcnxt_frp_istate,				},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ActSym"),					gcnxt_frp_actsym,				},
	{ xdrt_eof }
};

// <GrammarDerivationPath>
static TXmlDictRecord xtags_GrammarDerivationPath[] =
{
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_number,	DefTag(L"iConflict"),					gcnxt_derv_path_iconflict,		},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_number,	DefTag(L"iAction"),					gcnxt_derv_path_iaction,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"iPath"),						gcnxt_derv_path_ipath,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"CntSimilar"),					gcnxt_derv_path_cnt_similar,	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"PathLen"),					gcnxt_derv_path_path_len,		},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"PathStep"),					gcnxt_derv_path_step,			xtef_none,	xtags_FullRulePosition },
	{ xdrt_eof }
};

// <ConflictingActionAnalysisResults>
static TXmlDictRecord xtags_ConflictingActionAnalysisResults[] =
{
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_number,	DefTag(L"iConflict"),					gcnxt_act_res_iconflict,			},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_number,	DefTag(L"iAction"),					gcnxt_act_res_iaction,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"GenResult"),					gcnxt_act_res_gen_result,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"GenSteps"),					gcnxt_act_res_gen_steps,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"GenTime"),					gcnxt_act_res_gen_time,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"GenTimeAsText"),				gcnxt_act_res_gen_time_text,	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"TooDeepTruncs"),				gcnxt_act_res_deep_truncs,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"PartialAborts"),				gcnxt_act_res_pitrt_aborts,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumPaths"),					gcnxt_act_res_num_paths,		},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"DerivationPath"),				gcnxt_act_res_derv_path,		xtef_none,	xtags_GrammarDerivationPath },
	{ xdrt_eof }
};

// <GrammarConflictAnalysisResults>
static TXmlDictRecord xtags_GrammarConflictAnalysisResults[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"iConflict"),					gcnxt_anres_iconflict,			},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"ConflictingActionAnalysis"),		gcnxt_anres_action_res,		xtef_none,	xtags_ConflictingActionAnalysisResults },
	{ xdrt_eof }
};

// <GrammarConflictAnalysisProps>
static TXmlDictRecord xtags_GrammarConflictAnalysisProps[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"UseAllNonTermDefnRules"),		gcnxt_anpr_use_all_ntdrs,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"SearchForNonEmptyCtx"),		gcnxt_anpr_search_ext_ctx,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"MaxDerivationPathLen"),		gcnxt_anpr_max_path_len,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"MaxNumDerivationPaths"),		gcnxt_anpr_max_num_paths,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"StepDurationInSeconds"),		gcnxt_anpr_step_duration,		},
	{ xdrt_eof }
};

// <GrammarSummaryInfo>
static TXmlDictRecord xtags_GrammarSummaryInfo[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"FileName"),					gcnxt_grmi_src_file_name,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FileLength"),					gcnxt_grmi_src_file_length,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FileDate"),					gcnxt_grmi_src_file_date,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"FileDateAsText"),				gcnxt_grmi_src_file_dtx,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumRules"),					gcnxt_grmi_num_rules,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumSates"),					gcnxt_grmi_num_states,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumConflicts"),				gcnxt_grmi_num_conflicts,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumGrammars"),				gcnxt_grmi_num_grammars,		},
	{ xdrt_eof }
};

// <NestedGrammarAnalysis>
TXmlDictRecord xtags_NestedGrammarAnalysis[] =
{
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_number,	DefTag(L"iGrammar"),					gcnxt_andata_igrammar,		},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"iXpct"),						gcnxt_andata_ixpct,			},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_string,		DefTag(L"XpctConflictName"),			gcnxt_andata_xpct_name,		},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"NestedGrammarAnalysis"),		gcnxt_andata_nested_grm,		xtef_none,	xtags_NestedGrammarAnalysis			},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"GrammarConflictAnalysis"),		gcnxt_andata_conflict_data,		xtef_none,	xtags_GrammarConflictAnalysisResults	},
	{ xdrt_eof }
};

// <ConflictsAnalysisDatabase>
TXmlDictRecord xtags_ConflictsAnalysisDatabase[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_guid,		DefTag(L"GuidDatabase"),				gcnxt_andata_guid_database,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"WriterName"),				gcnxt_andata_writer_name,		},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"AnalysisProps"),				gcnxt_andata_analysis_props,	xtef_none,	xtags_GrammarConflictAnalysisProps		},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"GrammarInfo"),				gcnxt_andata_grm_info,			xtef_none,	xtags_GrammarSummaryInfo				},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"NestedGrammarAnalysis"),		gcnxt_andata_nested_grm,		xtef_none,	xtags_NestedGrammarAnalysis			},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"GrammarConflictAnalysis"),		gcnxt_andata_conflict_data,		xtef_none,	xtags_GrammarConflictAnalysisResults	},
	{ xdrt_eof }
};

// <_Root_Containers_Directory_>
const TXmlDictRecord xtags_GrammarAnalysisRootContainersDirectory[] =
{
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"FullRulePosition"),					gcnxt_full_rule_pos,		 xtef_none,	xtags_FullRulePosition					},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"GrammarDerivationPath"),			gcnxt_derivation_path,		 xtef_none,	xtags_GrammarDerivationPath			},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"ConflictingActionAnalysisResults"),	gcnxt_cfct_ation_anres,		 xtef_none,	xtags_ConflictingActionAnalysisResults	},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"GrammarConflictAnalysisResults"),	gcnxt_conflict_anres,		 xtef_none,	xtags_GrammarConflictAnalysisResults	},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"GrammarConflictAnalysisProps"),	gcnxt_conflict_anprops,		 xtef_none,	xtags_GrammarConflictAnalysisProps		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"GrammarSummaryInfo"),			gcnxt_grammar_sumry,		 xtef_none,	xtags_GrammarSummaryInfo				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"GrammarConflictsAnalysisData"),	gcnxt_grammar_analysis,	 xtef_none,	xtags_ConflictsAnalysisDatabase			},
	{ xdrt_eof }
};

//-----------------------------------------------------------------------------
//  ==============  TGrammarAnalysisDatabaseXmlWriter  ==================
//-----------------------------------------------------------------------------

void TGrammarAnalysisDatabaseXmlWriter::DumpFullRulePos(TXmlWriter &doc, WORD tag_id, TFullRulePosition &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, gcnxt_full_rule_pos, xtags_GrammarAnalysisRootContainersDirectory);

	doc.WriteNumTag(gcnxt_frp_irule, data.irule);
	doc.WriteNumTag(gcnxt_frp_isym, data.isym);
	doc.WriteStringTag(gcnxt_frp_origin, TFullRulePosition::GetFullRulePosOriginEnumName(data.origin));
	doc.WriteNumTag(gcnxt_frp_istate, data.istate);

	if (data.action_sym != 0xFFFF)
		doc.WriteNumTag(gcnxt_frp_actsym, data.action_sym);

	doc.CloseContainerEx(rcnr_dct);
}

void TGrammarAnalysisDatabaseXmlWriter::DumpDerivationPath(TXmlWriter &doc, WORD tag_id, int iconflict, int iaction, int ipath, TGrammarDerivationPath &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, gcnxt_derivation_path, xtags_GrammarAnalysisRootContainersDirectory);

	doc.WriteNumTag(gcnxt_derv_path_iconflict, iconflict);
	doc.WriteNumTag(gcnxt_derv_path_iaction, iaction);
	doc.WriteNumTag(gcnxt_derv_path_ipath, ipath);

	doc.WriteNumTag(gcnxt_derv_path_cnt_similar, data.cnt_similar);
	doc.WriteNumTag(gcnxt_derv_path_path_len, data.derv_path_len);

	for (int istep=0; istep<data.derv_path_len; ++istep)
		DumpFullRulePos(doc, gcnxt_derv_path_step, data.derv_steps[istep]);

	doc.CloseContainerEx(rcnr_dct);
}

void TGrammarAnalysisDatabaseXmlWriter::DumpActionAnalysisResults(TXmlWriter &doc, WORD tag_id, int iconflict, int iaction, TConflictingActionAnalysisResults &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, gcnxt_cfct_ation_anres, xtags_GrammarAnalysisRootContainersDirectory);

	doc.WriteNumTag(gcnxt_act_res_iconflict, iconflict);
	doc.WriteNumTag(gcnxt_act_res_iaction, iaction);

	doc.CloseTagHeaderIfNeeded();
	doc.WriteLine();

	doc.WriteStringTag(gcnxt_act_res_gen_result, TConflictingActionAnalysisResults::GetGenResultEnumName(data.m_generation_result));
	doc.WriteNumTag(gcnxt_act_res_gen_steps, data.m_generation_steps);
	doc.WriteNumTag(gcnxt_act_res_gen_time, data.m_generation_time);

	wchar_t buffer[80];
	doc.WriteStringTag(gcnxt_act_res_gen_time_text, FormatDuration(data.m_generation_time, buffer, 80, TRUE));
	doc.WriteNumTag(gcnxt_act_res_deep_truncs, data.m_too_deep_truncations);
	doc.WriteNumTag(gcnxt_act_res_pitrt_aborts, data.m_partial_iteration_aborts);

	if (data.m_generation_result == TConflictingActionAnalysisResults::gres_too_many_paths)
		WriteTooManyPathsNote(doc);

	doc.WriteLine();
	doc.WriteNumTag(gcnxt_act_res_num_paths, data.m_derivation_paths.NumItems());
	for (int ipath=0; ipath<data.m_derivation_paths.NumItems(); ++ipath)
		DumpDerivationPath(doc, gcnxt_act_res_derv_path, iconflict, iaction, ipath, data.m_derivation_paths[ipath]);

	doc.CloseContainerEx(rcnr_dct);
}

void TGrammarAnalysisDatabaseXmlWriter::DumpConflictAnalysisResults(TXmlWriter &doc, WORD tag_id, int iconflict, TGrammarConflictAnalysisResults &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, gcnxt_conflict_anres, xtags_GrammarAnalysisRootContainersDirectory);

	doc.WriteNumTag(gcnxt_anres_iconflict, iconflict);

	for (int iaction=0; iaction<data.NumItems(); ++iaction)
		DumpActionAnalysisResults(doc, gcnxt_anres_action_res, iconflict, iaction, data[iaction]);

	doc.CloseContainerEx(rcnr_dct);
}

void TGrammarAnalysisDatabaseXmlWriter::DumpConflictAnalysisProps(TXmlWriter &doc, WORD tag_id, TGrammarConflictAnalysisProps &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, gcnxt_conflict_anprops, xtags_GrammarAnalysisRootContainersDirectory);

	doc.WriteBoolTag(gcnxt_anpr_use_all_ntdrs, data.m_use_all_non_term_defn_rules);
	doc.WriteBoolTag(gcnxt_anpr_search_ext_ctx, data.m_search_for_non_empty_ctx);
	doc.WriteNumTag(gcnxt_anpr_max_path_len, data.m_max_path_length);
	doc.WriteNumTag(gcnxt_anpr_max_num_paths, data.m_max_num_paths);
	doc.WriteNumTag(gcnxt_anpr_step_duration, data.m_action_proc_timeout);

	doc.CloseContainerEx(rcnr_dct);
}

void TGrammarAnalysisDatabaseXmlWriter::DumpGrammarSummary(TXmlWriter &doc, WORD tag_id, TGrammar &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, gcnxt_grammar_sumry, xtags_GrammarAnalysisRootContainersDirectory);

	doc.WriteStringTag(gcnxt_grmi_src_file_name, data.grm_props.grms_file_name);
	doc.WriteNumTag(gcnxt_grmi_src_file_length, data.grm_props.grms_file_length);

	doc.WriteNumTag(gcnxt_grmi_src_file_date, data.grm_props.grms_file_date);
	if (data.grm_props.grms_file_date != 0)
	{
		wchar_t date_buff_text[80];
		doc.WriteStringTag(gcnxt_grmi_src_file_dtx, FormatDateTime(data.grm_props.grms_file_date, date_buff_text, 80, FALSE));
	}

	doc.WriteNumTag(gcnxt_grmi_num_rules, data.NumRules());
	doc.WriteNumTag(gcnxt_grmi_num_states, data.NumParsingStates());
	doc.WriteNumTag(gcnxt_grmi_num_conflicts, data.NumConflicts());
	doc.WriteNumTag(gcnxt_grmi_num_grammars, data.GetNumGrammars());

	doc.CloseContainerEx(rcnr_dct);
}

void TGrammarAnalysisDatabaseXmlWriter::DumpGrammarAnalysisData(TXmlWriter &doc, WORD tag_id, GUID &guid_db, TGrammarConflictAnalysisProps &props, TGrammar &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, gcnxt_grammar_analysis, xtags_GrammarAnalysisRootContainersDirectory);

	if (data.parent_grammar != NULL)
	{
		// Dump info about the current grammar as tag attributes.
		doc.WriteNumTag(gcnxt_andata_igrammar, data.GetGrammarIndex());
		doc.WriteNumTag(gcnxt_andata_ixpct, data.parent_xpct_inx);
		doc.WriteStringTag(gcnxt_andata_xpct_name, data.parent_grammar->xpct_conflicts[data.parent_xpct_inx].xpct_conflict_name);
	}

	bool child_grammars_avail = FALSE;
	for (int ixpct1=0; ixpct1<data.NumXpctConflicts(); ++ixpct1)
	{
		if (data.xpct_conflicts[ixpct1].nested_grammar != NULL)
		{
			child_grammars_avail = TRUE;
			break;
		}
	}

	if (data.parent_grammar == NULL || child_grammars_avail == TRUE || data.NumConflicts() > 0)
	{
		// Inner tags are always present in this container.
		doc.CloseTagHeaderIfNeeded();
		doc.WriteLine();

		if (data.parent_grammar == NULL)
		{
			wchar_t exe_fname[2*MAX_PATH];
			int nChrs = ::GetModuleFileNameW(NULL, exe_fname, 2*MAX_PATH);
			if (nChrs == 0)
				wcscpy(exe_fname, L"Error-getting-the-exe-file-name");

			doc.WriteGuidTag(gcnxt_andata_guid_database, guid_db);
			doc.WriteStringTag(gcnxt_andata_writer_name, exe_fname);
			doc.WriteLine();

			DumpConflictAnalysisProps(doc, gcnxt_andata_analysis_props, props);
			doc.WriteLine();

			DumpGrammarSummary(doc, gcnxt_andata_grm_info, data);
			doc.WriteLine();
		}

		if (child_grammars_avail == TRUE)
		{
			for (int ixpct2=0; ixpct2<data.NumXpctConflicts(); ++ixpct2)
			{
				if (data.xpct_conflicts[ixpct2].nested_grammar != NULL)
				{
					DumpGrammarAnalysisData(doc, gcnxt_andata_nested_grm, guid_db, props, *data.xpct_conflicts[ixpct2].nested_grammar);
					doc.WriteLine();
				}
			}
		}

		if (data.NumConflicts() > 0)
		{
			for (int iconflict=0; iconflict<data.NumConflicts(); ++iconflict)
			{
				TGrammarConflict &cfct = data.conflicts[iconflict];
				assert(cfct.analysis_results.NumItems() == 0 || cfct.analysis_results.NumItems() == cfct.NumActions());

				// This method emits an empty conflict container if the analysis results are empty.
				DumpConflictAnalysisResults(doc, gcnxt_andata_conflict_data, iconflict, cfct.analysis_results);
				doc.WriteLine();
			}
		}
	}

	doc.CloseContainerEx(rcnr_dct);
}

//----------------------------------------------------------------------------------
//  ===================  TFullRulePositionXmlReader  ========================
//----------------------------------------------------------------------------------

void TFullRulePositionXmlReader::ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case gcnxt_frp_irule:
				m_data->irule = (WORD)tagValue;
				break;

		case gcnxt_frp_isym:
				m_data->isym = (BYTE)tagValue;
				break;

		case gcnxt_frp_istate:
				m_data->istate = (WORD)tagValue;
				break;

		case gcnxt_frp_actsym:
				m_data->action_sym = (WORD)tagValue;
				break;
	}
}

void TFullRulePositionXmlReader::ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName)
{
	assert(tagId == gcnxt_frp_origin);
	m_data->origin = TFullRulePosition::GetFullRulePosOriginFromStringPtr(strValue);
	if (m_data->origin == rpo_num_types)
	{
		m_parser->ReportXmlAppError(L"Error assembling the TFullRulePosOrigin from \"%.*s\".", strValue.GetLength(), strValue.GetBodyPtr());
	}
}

//-----------------------------------------------------------------------------------
//  ================  TGrammarAnalysisDatabaseXmlReader  ====================
//-----------------------------------------------------------------------------------

TGrammarAnalysisDatabaseXmlReader::TGrammarAnalysisDatabaseXmlReader(GUID &guid_db, TGrammarConflictAnalysisProps &analysis_props, TGrammar *dest_grammar,
																			int igrammar_to_preserve, int iconflict_to_preserve, int iaction_to_preserve)
			: m_database_guid(guid_db), m_analysis_props(analysis_props), m_destination_grammar(dest_grammar)
{
	assert(dest_grammar != NULL);

	m_curr_grammar = NULL;
	m_curr_grammar_shadow = NULL;

	m_curr_conflict = NULL;
	m_curr_conflict_index = -1;

	m_curr_action = NULL;
	m_curr_action_next_ipath = -1;

	m_curr_path = NULL;
	m_curr_path_next_istep = -1;

	m_igrammar_to_preserve = igrammar_to_preserve;
	m_iconflict_to_preserve = iconflict_to_preserve;
	m_iaction_to_preserve = iaction_to_preserve;
}

bool TGrammarAnalysisDatabaseXmlReader::ScanXmlSource(TXmlStream &xmlSource, TGenericConsole *console, int *errs_cnt)
{
	TXmlReader reader(xtags_ConflictsAnalysisDatabase, L"GrammarConflictsAnalysisData", console);
	bool res = reader.Parse(xmlSource, this);

	if (errs_cnt != NULL)
		*errs_cnt = reader.GetErrorsCount();

	return(res);
}

TXmlContainerHandler *TGrammarAnalysisDatabaseXmlReader::OpenSubContainer(WORD tagId, TStringPtr &tagName)
{
	switch (tagId)
	{
		// <GrammarDerivationPath>
		case gcnxt_derv_path_step:
				{
					if (m_curr_path == NULL)
						return(&TXmlReader::s_skip_subtree_handler);

					if (m_curr_path_next_istep < 0)
					{
						if (m_curr_path_next_istep == -1)
						{
							m_parser->ReportXmlAppError(L"The number of derivation path steps is missing.");
						}

						return(&TXmlReader::s_skip_subtree_handler);
					}
					else if (m_curr_path_next_istep >= m_curr_path->derv_path_len)
					{
						m_parser->ReportXmlAppError(L"The number of derivation path steps exceeds the number of steps that was manifested before.");
						return(&TXmlReader::s_skip_subtree_handler);
					}

					// The step was successfully entered.
					m_curr_step_reader.SetDestination(m_curr_path->derv_steps+m_curr_path_next_istep);
					m_curr_path_next_istep++;
					return(&m_curr_step_reader);
				}
				break;

		// <ConflictingActionAnalysisResults>
		case gcnxt_act_res_derv_path:
				{
					if (m_curr_action == NULL)
						return(&TXmlReader::s_skip_subtree_handler);

					if (m_curr_action_next_ipath < 0)
					{
						if (m_curr_action_next_ipath == -1)
						{
							m_parser->ReportXmlAppError(L"The number of derivation paths is missing.");
						}

						return(&TXmlReader::s_skip_subtree_handler);
					}
					else if (m_curr_action_next_ipath == 0 && m_curr_action->m_derivation_paths.NumItems() == 0)
					{
						m_parser->ReportXmlAppError(L"The number of derivation paths is stated as zero. The paths should not be present.");
						return(&TXmlReader::s_skip_subtree_handler);
					}
				}
				break;

		// <GrammarConflictAnalysisResults>
		case gcnxt_anres_action_res:
				{
					if (m_curr_conflict == NULL)
						return(&TXmlReader::s_skip_subtree_handler);

					assert(m_curr_conflict->NumActions() > 0);
					if (m_curr_conflict->analysis_results.NumItems() != m_curr_conflict->NumActions())
					{
						// Analysis results are missing or the number of records in the current grammar is bogus.
						// Recreate a array of analysis result stubs.
						if (m_curr_conflict->BuildAnalysisResultsStub() == FALSE)
						{
							m_parser->ReportXmlAppError(L"Error creating conflicting action analysis records.");
							return(&TXmlReader::s_skip_subtree_handler);
						}
					}
				}
				break;

		//
		// <ConflictsAnalysisDatabase>
		//

		case gcnxt_andata_analysis_props:
		case gcnxt_andata_grm_info:
				// No preparation is needed.
				break;

		case gcnxt_andata_nested_grm:
		case gcnxt_andata_conflict_data:
				{
					if (m_curr_grammar == NULL)
					{
						if (m_curr_grammar_shadow != NULL)
						{
							// The shadow grammar is not NULL. This means that before entering the current container the current grammar was not NULL.
							m_parser->ReportXmlAppError(L"The index of the expected conflict that should own data of the current container is missing or it is bogus.");
							m_parser->ReportXmlAppError(L"Processing of the subcontainer is skipped.");
						}

						return(&TXmlReader::s_skip_subtree_handler);
					}
				}
				break;

		default:
			assert(FALSE);
			break;
	}

	return(this);
}

void TGrammarAnalysisDatabaseXmlReader::BeginNodeProcessing(WORD tagId)
{
	switch (tagId)
	{
		// Root container.
		case ROOT_CONTAINER_TAG_ID:
				{
					if (m_igrammar_to_preserve >= 0 && m_iconflict_to_preserve >= 0 && m_iaction_to_preserve >= 0)
					{
						// Preservation params are not empty. Check them.
						TGrammar *grm = m_destination_grammar->GetGrammarByIndex(m_igrammar_to_preserve);
						if (grm == NULL)
						{
							m_parser->ReportXmlAppError(L"The zero based index of the grammar to preserve is bogus: %d. NumGrammars: %d.", m_igrammar_to_preserve, m_destination_grammar->GetNumGrammars());
						}
						else if (m_iconflict_to_preserve >= grm->NumConflicts())
						{
							m_parser->ReportXmlAppError(L"The zero based index of the conflict to preserve is bogus: %d. NumConflicts: %d.", m_iconflict_to_preserve, grm->NumConflicts());
						}
						else if (m_iaction_to_preserve >= grm->conflicts[m_iconflict_to_preserve].NumActions())
						{
							m_parser->ReportXmlAppError(L"The zero based index of the conflicting action to preserve is bogus: %d. NumConflictingActions: %d.", m_iaction_to_preserve, grm->conflicts[m_iconflict_to_preserve].NumActions());
						}
					}

					memset(&m_database_guid, 0, sizeof(m_database_guid));
					m_analysis_props.SetDefaultValues();
				}
				break;

		// <ConflictsAnalysisDatabase>
		case gcnxt_andata_nested_grm:
				{
					assert(m_curr_grammar != NULL);
					m_curr_grammar_shadow = m_curr_grammar;
					m_curr_grammar = NULL;
				}
				break;
	}
}

void TGrammarAnalysisDatabaseXmlReader::FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems)
{
	switch (tagId)
	{
		// <ConflictingActionAnalysisResults>
		case gcnxt_act_res_derv_path:
				{
					// Process exit from the derivation path definition.
					if (m_curr_path != NULL)
					{
						if (nodeScanningProblems == FALSE && m_curr_path_next_istep >= 0 && m_curr_path_next_istep != m_curr_path->derv_path_len)
						{
							m_parser->ReportXmlAppError(L"Some of the manifested derivation path steps are missing. The expected number of steps is: %d.", m_curr_path->derv_path_len);
						}

						m_curr_path = NULL;
						m_curr_path_next_istep = -1;
					}
				}
				break;

		// <GrammarConflictAnalysisResults>
		case gcnxt_anres_action_res:
				{
					// Process exit from the conflicting action analysis data container.
					if (m_curr_action != NULL)
					{
						if (nodeScanningProblems == FALSE && m_curr_action_next_ipath >= 0 && m_curr_action_next_ipath != m_curr_action->m_derivation_paths.NumItems())
						{
							m_parser->ReportXmlAppError(L"Some of the manifested derivation paths are missing. The expected number of paths is: %d.", m_curr_action->m_derivation_paths.NumItems());
						}

						m_curr_action = NULL;
						m_curr_action_next_ipath = -1;
					}
				}
				break;

		//
		// <ConflictsAnalysisDatabase>
		//

		case gcnxt_andata_grm_info:
				{
					assert(m_curr_grammar == NULL);
					if (nodeScanningProblems == FALSE)
					{
						// All grammar info tags are present and their values are fine.
						// Set the poiner to a non NULL value. This will allow entering other containers.
						m_curr_grammar = m_destination_grammar;
					}
				}
				break;

		case gcnxt_andata_nested_grm:
				{
					// Process exit from the nested grammar.
					if (m_curr_grammar != NULL)
					{
						assert(m_curr_grammar->parent_grammar != NULL);
						m_curr_grammar = m_curr_grammar->parent_grammar;
					}
					else
					{
						m_curr_grammar = m_curr_grammar_shadow;
					}
				}
				break;

		case gcnxt_andata_conflict_data:
				{
					// Process exit from the grammar conflict analysis data container.
					m_curr_conflict = NULL;
					m_curr_conflict_index = -1;
				}
				break;
	}
}

void TGrammarAnalysisDatabaseXmlReader::ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		//
		// <GrammarConflictAnalysisProps>
		//

		case gcnxt_anpr_use_all_ntdrs:
				{
					m_analysis_props.m_use_all_non_term_defn_rules = tagValue;
				}
				break;

		case gcnxt_anpr_search_ext_ctx:
				{
					m_analysis_props.m_search_for_non_empty_ctx = tagValue;
				}
				break;

		default:
			assert(FALSE);
			break;
	}
}

void TGrammarAnalysisDatabaseXmlReader::ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		//
		// <GrammarDerivationPath>
		//

		case gcnxt_derv_path_iconflict:
		case gcnxt_derv_path_iaction:
				break;	// Ignore these fields.

		case gcnxt_derv_path_ipath:
				{
					assert(m_curr_action != NULL);
					assert(m_curr_action_next_ipath >= 0);

					if (tagValue != m_curr_action_next_ipath)
					{
						m_parser->ReportXmlAppError(L"The indexes of the derivation paths are not sequential.");
						break;
					}
					else if (tagValue >= m_curr_action->m_derivation_paths.NumItems())
					{
						m_parser->ReportXmlAppError(L"The number of derivation paths exceeds the number of derivation paths that was manifested before.");
						break;
					}

					// An index of the current path is fine.
					m_curr_path = m_curr_action->m_derivation_paths.ItemPtr(m_curr_action_next_ipath);
					m_curr_action_next_ipath++;
				}
				break;

		case gcnxt_derv_path_cnt_similar:
				{
					if (m_curr_path != NULL)
						m_curr_path->cnt_similar = tagValue;
				}
				break;

		case gcnxt_derv_path_path_len:
				{
					if (m_curr_path != NULL)
					{
						int num_steps = (int)tagValue;
						if (num_steps < 0 || num_steps > TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN)
						{
							m_parser->ReportXmlAppError(L"The number of steps in the derivation path. This number should be in the range from 0 to %d.", TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN);
							m_curr_path_next_istep = -2;
							break;
						}

						// Allocate space for the derivation steps.
						if (num_steps > 0)
						{
							m_curr_path->derv_steps = (TFullRulePosition*)malloc(num_steps*sizeof(TFullRulePosition));
							if (m_curr_path->derv_steps == NULL)
							{
								m_parser->ReportXmlAppError(L"Error allocating memory for the derivation path steps.");
								m_curr_path_next_istep = -2;
								break;
							}
						}

						m_curr_path->derv_path_len = (int)num_steps;
						m_curr_path_next_istep = 0;
					}
				}
				break;

		//
		// <ConflictingActionAnalysisResults>
		//

		case gcnxt_act_res_iconflict:
				break;	// Ignore this field.

		case gcnxt_act_res_iaction:
				{
					assert(m_curr_conflict != NULL);
					int iaction = (int)tagValue;
					if (iaction >= 0 && iaction < m_curr_conflict->NumActions())
					{
						if (m_igrammar_to_preserve >= 0)
						{
							TGrammar *grm = m_destination_grammar->GetGrammarByIndex(m_igrammar_to_preserve);
							if (m_curr_grammar == grm && m_curr_conflict_index == m_iconflict_to_preserve && iaction == m_iaction_to_preserve)
							{
								// Ctor params ask to preserve data, that is currently stored in this conflicting action.
								break;
							}
						}

						assert(iaction < m_curr_conflict->analysis_results.NumItems());
						m_curr_action = m_curr_conflict->analysis_results.ItemPtr(iaction);

						// Kill the existing data in this conflicting action if any.
						m_curr_action->Clear();
					}
					else
					{
						m_parser->ReportXmlAppError(L"Current grammar conflict does not contain an action with the index %d.", iaction);
					}
				}
				break;

		case gcnxt_act_res_gen_steps:
				{
					if (m_curr_action != NULL)
						m_curr_action->m_generation_steps = tagValue;
				}
				break;

		case gcnxt_act_res_gen_time:
				{
					if (m_curr_action != NULL)
						m_curr_action->m_generation_time = tagValue;
				}
				break;

		case gcnxt_act_res_deep_truncs:
				{
					if (m_curr_action != NULL)
						m_curr_action->m_too_deep_truncations = tagValue;
				}
				break;

		case gcnxt_act_res_pitrt_aborts:
				{
					if (m_curr_action != NULL)
						m_curr_action->m_partial_iteration_aborts = tagValue;
				}
				break;

		case gcnxt_act_res_num_paths:
				{
					if (m_curr_action != NULL)
					{
						// At the beginning the array of derivation paths should be empty.
						assert(m_curr_action->m_derivation_paths.NumItems() == 0);

						int max_num_paths = 1000000;
						if (tagValue < 0 || tagValue > max_num_paths)
						{
							m_parser->ReportXmlAppError(L"The number of derivation paths is bogus. This number should be in the range from 0 to %d.", max_num_paths);
							m_curr_action_next_ipath = -2;
							break;
						}

						// Create array of the derivation path stubs.
						TGrammarDerivationPath path = { 0, 0, NULL };
						for (int ipath=0; ipath<tagValue; ++ipath)
						{
							if (m_curr_action->m_derivation_paths.AppendItem(path) == FALSE)
							{
								m_parser->ReportXmlAppError(L"Error creating a derivation path record.");
								m_curr_action->m_derivation_paths.Clear();
								m_curr_action_next_ipath = -2;
								break;
							}
						}

						// The array was prepared.
						m_curr_action_next_ipath = 0;
					}
				}
				break;

		//
		// <GrammarConflictAnalysisResults>
		//

		case gcnxt_anres_iconflict:
				{
					// Finalize entering the conflict analysis data.
					assert(m_curr_grammar != NULL);
					int iconflict = (int)tagValue;
					if (iconflict >= 0 && iconflict < m_curr_grammar->NumConflicts())
					{
						m_curr_conflict = m_curr_grammar->conflicts.ItemPtr(iconflict);
						m_curr_conflict_index = iconflict;
					}
					else
					{
						m_parser->ReportXmlAppError(L"Current grammar does not contain grammar conflict with the index %d.", iconflict);
					}
				}
				break;

		//
		// <GrammarConflictAnalysisProps>
		//

		case gcnxt_anpr_max_path_len:
				{
					m_analysis_props.m_max_path_length = (int)tagValue;
				}
				break;

		case gcnxt_anpr_max_num_paths:
				{
					m_analysis_props.m_max_num_paths = (int)tagValue;
				}
				break;

		case gcnxt_anpr_step_duration:
				{
					m_analysis_props.m_action_proc_timeout = (int)tagValue;
				}
				break;

		//
		// <GrammarSummaryInfo>
		//

		case gcnxt_grmi_src_file_length:
				{
					if (m_destination_grammar->grm_props.grms_file_length != tagValue)
					{
						m_parser->ReportXmlAppError(L"The length of the main grammar source file is wrong. The expected value is: %ld.",
												m_destination_grammar->grm_props.grms_file_length);
					}
				}
				break;

		case gcnxt_grmi_src_file_date:
				{
					if (m_destination_grammar->grm_props.grms_file_date != tagValue)
					{
						wchar_t buffer[80];
						m_parser->ReportXmlAppError(L"The date of the main grammar source file is wrong. The expected value is: %s.",
												FormatInt64(m_destination_grammar->grm_props.grms_file_date, buffer, 80, fnms_dec_signed, L'_'));
					}
				}
				break;

		case gcnxt_grmi_num_rules:
				{
					if (m_destination_grammar->NumRules() != tagValue)
					{
						m_parser->ReportXmlAppError(L"The number of rules in the root grammar is wrong. The expected value is: %d.", m_destination_grammar->NumRules());
					}
				}
				break;

		case gcnxt_grmi_num_states:
				{
					if (m_destination_grammar->NumParsingStates() != tagValue)
					{
						m_parser->ReportXmlAppError(L"The number of parsing states in the root grammar is wrong. The expected value is: %d.", m_destination_grammar->NumParsingStates());
					}
				}
				break;

		case gcnxt_grmi_num_conflicts:
				{
					if (m_destination_grammar->NumConflicts() != tagValue)
					{
						m_parser->ReportXmlAppError(L"The number of grammar conflicts in the root grammar is wrong. The expected value is: %d.", m_destination_grammar->NumConflicts());
					}
				}
				break;

		case gcnxt_grmi_num_grammars:
				{
					int num_grammars = m_destination_grammar->GetNumGrammars();
					if (num_grammars != tagValue)
					{
						m_parser->ReportXmlAppError(L"The number of grammars in the tree of grammars is wrong. The expected value is: %d.", num_grammars);
					}
				}
				break;

		//
		// <NestedGrammarAnalysis>
		//

		case gcnxt_andata_igrammar:
				break;	// Ignore this field.

		case gcnxt_andata_ixpct:
				{
					// Finalize entering the nested grammar.
					assert(m_curr_grammar_shadow != NULL);
					int ixpct = (int)tagValue;
					if (ixpct >= 0 && ixpct < m_curr_grammar_shadow->NumXpctConflicts() && m_curr_grammar_shadow->xpct_conflicts[ixpct].nested_grammar != NULL)
					{
						m_curr_grammar = m_curr_grammar_shadow->xpct_conflicts[ixpct].nested_grammar;
					}
					else
					{
						m_parser->ReportXmlAppError(L"Current grammar does not contain nested grammar in the expected conflict with the index %d.", ixpct);
					}
				}
				break;

		default:
			assert(FALSE);
			break;
	}
}

void TGrammarAnalysisDatabaseXmlReader::ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		//
		// <ConflictingActionAnalysisResults>
		//

		case gcnxt_act_res_gen_result:
				{
					if (m_curr_action != NULL)
					{
						// Check for the aliases first.
						if (strValue == L"gres_aborted")
						{
							m_curr_action->m_generation_result = TConflictingActionAnalysisResults::gres_aborted_by_user;
						}
						else
						{
							// Check for the regular enum names.
							TConflictingActionAnalysisResults::TGenResult gen_res = TConflictingActionAnalysisResults::GetGenResultFromStringPtr(strValue);
							if (gen_res != TConflictingActionAnalysisResults::gres_num_types)
							{
								m_curr_action->m_generation_result = gen_res;
							}
							else
							{
								m_parser->ReportXmlAppError(L"Error assembling the TGenResult value from \"%.*s\".", strValue.GetLength(), strValue.GetBodyPtr());
							}
						}
					}
				}
				break;

		case gcnxt_act_res_gen_time_text:
				break;	// Ignore this field.

		//
		// <GrammarSummaryInfo>
		//

		case gcnxt_grmi_src_file_name:
				{
					TFileNameBuffer dest_grammar_short_fname;
					if (TPathHelper::ExtractShortName(dest_grammar_short_fname, m_destination_grammar->grm_props.grms_file_name) == FALSE)
					{
						m_parser->ReportXmlAppError(L"Error getting the short name of the main grammar source file of the destination grammar.");
						break;
					}

					wchar_t fname_buffer[2*MAX_PATH];
					strValue.CopyWithTruncationTo(fname_buffer, 2*MAX_PATH);

					TFileNameBuffer grammar_info_short_fname;
					if (TPathHelper::ExtractShortName(grammar_info_short_fname, fname_buffer) == FALSE)
					{
						m_parser->ReportXmlAppError(L"Error getting the short name of the main grammar source file from the XML data field.");
						break;
					}

					if (dest_grammar_short_fname != grammar_info_short_fname)
					{
						m_parser->ReportXmlAppError(L"The name of the main grammar source file is wrong. The expected value is: \"%s\".", dest_grammar_short_fname.DataPtr());
					}
				}
				break;

		case gcnxt_grmi_src_file_dtx:
				break;	// Ignore this field.

		//
		// <NestedGrammarAnalysis>
		//

		case gcnxt_andata_xpct_name:
				break;	// Ignore this field.

		//
		// <ConflictsAnalysisDatabase>
		//

		case gcnxt_andata_writer_name:
				break;	// Ignore this field.

		default:
			assert(FALSE);
			break;
	}
}

void TGrammarAnalysisDatabaseXmlReader::ProcessGuidField(WORD tagId, GUID &tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		// <ConflictsAnalysisDatabase>
		case gcnxt_andata_guid_database:
				m_database_guid = tagValue;
				break;

		default:
			assert(FALSE);
			break;
	}
}

//------------------------------------------------------------------------
//  ==============  Grammar Analysis Xml Schema  ==================
//------------------------------------------------------------------------

// <RuleStartStatesIterator>
static TXmlDictRecord xtags_RuleStartStatesIterator[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"RuleInx"),						gcsxt_rssi_irule,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"RuleSymInx"),					gcsxt_rssi_rule_isym,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"SttAtRuleSym"),					gcsxt_rssi_state_at_risym,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ActionSymRestr"),					gcsxt_rssi_actsym_restr,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"StartupFlag"),					gcsxt_rssi_startup_flag,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"SttInxLen"),						gcsxt_rssi_stt_inx_len,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"PositionsAvail"),					gcsxt_rssi_positions_avail,		},
	{ xdrt_eof }
};

// <RuleCallPlacesIterator>
static TXmlDictRecord xtags_RuleCallPlacesIterator[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParsingState"),					gcsxt_rcpi_parsing_state,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NonTerm"),						gcsxt_rcpi_non_term,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ActionSymRestr"),					gcsxt_rcpi_action_sym,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"StartupFlag"),					gcsxt_rcpi_startup_flag,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"PositionsAvail"),					gcsxt_rcpi_positions_avail,		},
	{ xdrt_eof }
};

// <NonTermDefnRulesIterator>
static TXmlDictRecord xtags_NonTermDefnRulesIterator[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"RuleStartState"),					gcsxt_ntdi_rule_start_state,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"RuleNonTerm"),					gcsxt_ntdi_rule_non_term,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"IterDiffActSyms"),					gcsxt_ntdi_iter_diff_asyms,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"StartupFlag"),					gcsxt_ntdi_startup_flag,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"LatestRuleInx"),					gcsxt_ntdi_latest_irule,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"PositionsAvail"),					gcsxt_ntdi_positions_avail,		},
	{ xdrt_eof }
};

// <BuildGrammarDerivationSlot>
static TXmlDictRecord xtags_BuildGrammarDerivationSlot[] =
{
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_number,	DefTag(L"iSlot"),							gcsxt_slot_islot,					},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"FullRulePosition"),					gcsxt_slot_frp,					xtef_none,   xtags_FullRulePosition				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"UpperLayer"),					gcsxt_slot_upper_layer,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"RuleStartStatesIter"),				gcsxt_slot_rule_start_states_iter,		xtef_none,   xtags_RuleStartStatesIterator		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"RuleCallPlacesIter"),				gcsxt_slot_rule_call_places_iter,		xtef_none,   xtags_RuleCallPlacesIterator		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"NonTermDefnRulesIter"),			gcsxt_slot_nterm_defn_rules_iter,		xtef_none,   xtags_NonTermDefnRulesIterator	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"MoreBackwCtx"),					gcsxt_slot_need_more_backw_ctx,	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"MoreForwCtx"),					gcsxt_slot_need_more_forw_ctx,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"SetupsCnt"),						gcsxt_slot_setup_cnt,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"PositionsCnt"),					gcsxt_slot_positions_cnt,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"LayerSetupsCnt"),					gcsxt_slot_setup_cnts_count,			},
	{ xdrt_eof }
};

// <SavedConflictingActionAnalysisState>
TXmlDictRecord xtags_SavedConflictingActionAnalysisState[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_guid,		DefTag(L"GuidDatabase"),						gcsxt_sas_database_guid,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"GrammarIndex"),						gcsxt_sas_igrammar,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ConflictIndex"),						gcsxt_sas_iconflict,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ConflicingActionIndex"),				gcsxt_sas_iaction,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"GenResult"),							gcsxt_sas_genres,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"GenTimeAsText"),						gcnxt_sas_gentime_text,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumPaths"),							gcsxt_sas_numpaths,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"CurrLevel"),							gcsxt_sas_clev,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"StepsCount"),						gcsxt_sas_cnt_steps,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"IterSetupsCount"),					gcsxt_sas_iter_setup_cnt,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"CurrLevBottom"),						gcsxt_sas_clev_bottom,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"WantBackCtx"),						gcsxt_sas_wback_ctx,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"WantForwCtx"),						gcsxt_sas_wforw_ctx,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"CtxPathsPresent"),					gcsxt_sas_ctx_paths_pres,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"MaxTransPathLen"),					gcsxt_sas_max_trans_path,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"MaxSlotsHashingAreas"),				gcsxt_sas_max_shash_areas,	},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"BuildGrammarDerivationSlot"),			gcsxt_sas_derivation_slot,		xtef_none,	xtags_BuildGrammarDerivationSlot		   },
	{ xdrt_eof }
};

// <_Root_Containers_Directory_>
const TXmlDictRecord xtags_GrammarAnalysisStateRootContainersDirectory[] =
{
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"RuleStartStatesIterator"),				gcsxt_rule_start_states_iter,		xtef_none,	xtags_RuleStartStatesIterator			   },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"RuleCallPlacesIterator"),				gcsxt_rule_call_places_iter,		xtef_none,	xtags_RuleCallPlacesIterator				   },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"NonTermDefnRulesIterator"),			gcsxt_nterm_defn_rules_iter,	xtef_none,	xtags_NonTermDefnRulesIterator			   },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"BuildGrammarDerivationSlot"),			gcsxt_build_grm_deriv_slot,		xtef_none,	xtags_BuildGrammarDerivationSlot		   },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"SavedConflictingActionAnalysisState"),	gcsxt_saved_analysis_state,		xtef_none,	xtags_SavedConflictingActionAnalysisState	   },
	{ xdrt_eof }
};

//-------------------------------------------------------------------------------------------------
//	===================  TSavedConflictingActionAnalysisStateXmlWriter  ======================
//-------------------------------------------------------------------------------------------------

void TSavedConflictingActionAnalysisStateXmlWriter::DumpRuleStartStatesIterator(TXmlWriter &doc, WORD tag_id, TRuleStartStatesIterator &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, gcsxt_rule_start_states_iter, xtags_GrammarAnalysisStateRootContainersDirectory);

	doc.WriteNumTag(gcsxt_rssi_irule, data.m_irule);
	doc.WriteNumTag(gcsxt_rssi_rule_isym, data.m_rule_isym);

	if (data.m_state_at_rule_isym != 0xFFFF)
		doc.WriteNumTag(gcsxt_rssi_state_at_risym, data.m_state_at_rule_isym);
	if (data.m_action_sym_restr != 0xFFFF)
		doc.WriteNumTag(gcsxt_rssi_actsym_restr, data.m_action_sym_restr);
	if (data.m_startup_flag == TRUE)
		doc.WriteBoolTag(gcsxt_rssi_startup_flag, data.m_startup_flag);

	doc.WriteNumTag(gcsxt_rssi_stt_inx_len, data.m_stts_index_len);
	doc.WriteNumTag(gcsxt_rssi_positions_avail, data.m_positions_iter.NumAvalablePositions());

	doc.CloseContainerEx(rcnr_dct);
}

void TSavedConflictingActionAnalysisStateXmlWriter::DumpRuleCallPlacesIterator(TXmlWriter &doc, WORD tag_id, TRuleCallPlacesIterator &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, gcsxt_rule_call_places_iter, xtags_GrammarAnalysisStateRootContainersDirectory);

	doc.WriteNumTag(gcsxt_rcpi_parsing_state, data.m_state_before_non_term);
	doc.WriteNumTag(gcsxt_rcpi_non_term, data.m_non_term);

	if (data.m_action_sym_restr != 0xFFFF)
		doc.WriteNumTag(gcsxt_rcpi_action_sym, data.m_action_sym_restr);
	if (data.m_startup_flag == TRUE)
		doc.WriteBoolTag(gcsxt_rcpi_startup_flag, data.m_startup_flag);

	doc.WriteNumTag(gcsxt_rcpi_positions_avail, data.m_positions_iter.NumAvalablePositions());

	doc.CloseContainerEx(rcnr_dct);
}

void TSavedConflictingActionAnalysisStateXmlWriter::DumpNonTermDefnRulesIterator(TXmlWriter &doc, WORD tag_id, TNonTermDefnRulesIterator &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, gcsxt_nterm_defn_rules_iter, xtags_GrammarAnalysisStateRootContainersDirectory);

	doc.WriteNumTag(gcsxt_ntdi_rule_start_state, data.m_rule_starting_state);
	doc.WriteNumTag(gcsxt_ntdi_rule_non_term, data.m_rule_non_term);
	doc.WriteBoolTag(gcsxt_ntdi_iter_diff_asyms, data.m_iter_diff_action_syms);

	if (data.m_startup_flag == TRUE)
		doc.WriteBoolTag(gcsxt_ntdi_startup_flag, data.m_startup_flag);
	if (data.m_iter_diff_action_syms == TRUE)
		doc.WriteNumTag(gcsxt_ntdi_latest_irule, data.m_latest_irule);

	doc.WriteNumTag(gcsxt_ntdi_positions_avail, data.m_positions_iter.NumAvalablePositions());

	doc.CloseContainerEx(rcnr_dct);
}

void TSavedConflictingActionAnalysisStateXmlWriter::DumpBuildDerivationPathSlot(TXmlWriter &doc, WORD tag_id, int ilayer, TBuildGrammarDerivationSlot &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, gcsxt_build_grm_deriv_slot, xtags_GrammarAnalysisStateRootContainersDirectory);

	doc.WriteNumTag(gcsxt_slot_islot, ilayer);
	TGrammarAnalysisDatabaseXmlWriter::DumpFullRulePos(doc, gcsxt_slot_frp, data.m_frp);

	if (data.m_upper_layer != -1)
		doc.WriteNumTag(gcsxt_slot_upper_layer, data.m_upper_layer);

	// Dump contents of the iterator only when the type of the slot is iterateable.
	switch (data.m_frp.origin)
	{
		case rpo_rule_start_state:
		case rpo_start_state_ctx:
				{
					DumpRuleStartStatesIterator(doc, gcsxt_slot_rule_start_states_iter, data.m_rule_start_states_iter);
				}
				break;

		case rpo_rule_call_place:
		case rpo_call_place_ctx:
				{
					DumpRuleCallPlacesIterator(doc, gcsxt_slot_rule_call_places_iter, data.m_rule_call_places_iter);
				}
				break;

		case rpo_non_term_defn_rule:
				{
					DumpNonTermDefnRulesIterator(doc, gcsxt_slot_nterm_defn_rules_iter, data.m_non_term_defn_rules_iter);
				}
				break;
	}

	doc.WriteBoolTag(gcsxt_slot_need_more_backw_ctx, data.m_need_more_backw_ctx);
	doc.WriteBoolTag(gcsxt_slot_need_more_forw_ctx, data.m_need_more_forw_ctx);
	doc.WriteNumTag(gcsxt_slot_setup_cnt, data.m_setup_cnt);
	doc.WriteNumTag(gcsxt_slot_positions_cnt, data.m_positions_cnt);
	doc.WriteNumTag(gcsxt_slot_setup_cnts_count, data.m_setup_cnts_count);

	doc.CloseContainerEx(rcnr_dct);
}

void TSavedConflictingActionAnalysisStateXmlWriter::DumpGrammarAnalysisState(TXmlWriter &doc, WORD tag_id, GUID &core_db_guid, TGrammarConflictAnalysisHelper &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, gcsxt_saved_analysis_state, xtags_GrammarAnalysisStateRootContainersDirectory);

	doc.CloseTagHeaderIfNeeded();
	doc.WriteLine();

	doc.WriteGuidTag(gcsxt_sas_database_guid, core_db_guid);
	doc.WriteLine();

	doc.WriteNumTag(gcsxt_sas_igrammar, data.m_igrammar);
	doc.WriteNumTag(gcsxt_sas_iconflict, data.m_iconflict);
	doc.WriteNumTag(gcsxt_sas_iaction, data.m_iaction);
	doc.WriteLine();

	if (data.m_result != NULL)
	{
		doc.WriteStringTag(gcsxt_sas_genres, TConflictingActionAnalysisResults::GetGenResultEnumName(data.m_result->m_generation_result));

		wchar_t buffer[80];
		doc.WriteStringTag(gcnxt_sas_gentime_text, FormatDuration(data.m_result->m_generation_time, buffer, 80, TRUE));
		doc.WriteNumTag(gcsxt_sas_numpaths, data.m_result->m_derivation_paths.NumItems());

		if (data.m_result->m_generation_result == TConflictingActionAnalysisResults::gres_too_many_paths)
			WriteTooManyPathsNote(doc);
	}
	else
	{
		doc.WriteStringTag(gcsxt_sas_genres, L"n/a");
	}

	doc.WriteLine();

	doc.WriteNumTag(gcsxt_sas_clev, data.m_clev);
	doc.WriteNumTag(gcsxt_sas_cnt_steps, data.m_cnt_steps);
	doc.WriteNumTag(gcsxt_sas_iter_setup_cnt, data.m_iter_setup_cnt);

	if (data.m_clev_bottom > 0)
	{
		doc.WriteNumTag(gcsxt_sas_clev_bottom, data.m_clev_bottom);
		doc.WriteBoolTag(gcsxt_sas_wback_ctx, data.m_want_backw_ctx);
		doc.WriteBoolTag(gcsxt_sas_wforw_ctx, data.m_want_forw_ctx);
		doc.WriteBoolTag(gcsxt_sas_ctx_paths_pres, data.m_ctx_paths_present);
	}

	int max_path_len = data.GetMaxTransientPathLength();
	doc.WriteNumTag(gcsxt_sas_max_trans_path, max_path_len);
	doc.WriteNumTag(gcsxt_sas_max_shash_areas, data.GetMaxTransientSlotsHashingAreas());
	doc.WriteLine();

	if (data.m_clev > 0)
	{
		for (int islot=0; islot<max_path_len; ++islot)
		{
			DumpBuildDerivationPathSlot(doc, gcsxt_sas_derivation_slot, islot, data.m_curr_path[islot]);
			doc.WriteLine();
		}
	}

	doc.CloseContainerEx(rcnr_dct);
}

//---------------------------------------------------------------------------------
//  =================  TRuleStartStatesIteratorXmlReader  ====================
//---------------------------------------------------------------------------------

void TRuleStartStatesIteratorXmlReader::BeginNodeProcessing(WORD tagId)
{
	assert(m_data != NULL);
	assert(m_data->m_grammar != NULL);
	assert(m_index2 != NULL);

	// Setup values of the default fields.
	m_data->m_state_at_rule_isym = 0xFFFF;
	m_data->m_action_sym_restr = 0xFFFF;
	m_data->m_startup_flag = FALSE;
}

void TRuleStartStatesIteratorXmlReader::FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems)
{
	if (nodeScanningProblems == FALSE)
	{
		if (m_data->m_irule < 0 || m_data->m_irule >= m_data->m_grammar->NumRules())
		{
			m_parser->ReportXmlAppError(L"The value of the rule index is bogus: %d. NumRules: %d.", m_data->m_irule, m_data->m_grammar->NumRules());
			return;
		}

		m_data->PrepareIterationSpecial(*m_index2, m_positions_available);
	}

	m_data = NULL;
	m_index2 = NULL;
}

void TRuleStartStatesIteratorXmlReader::ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case gcsxt_rssi_startup_flag:
				m_data->m_startup_flag = tagValue;
				break;
	}
}

void TRuleStartStatesIteratorXmlReader::ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case gcsxt_rssi_irule:
				m_data->m_irule = (int)tagValue;
				break;

		case gcsxt_rssi_rule_isym:
				m_data->m_rule_isym = (int)tagValue;
				break;

		case gcsxt_rssi_state_at_risym:
				m_data->m_state_at_rule_isym = (WORD)tagValue;
				break;

		case gcsxt_rssi_actsym_restr:
				m_data->m_action_sym_restr = (WORD)tagValue;
				break;

		case gcsxt_rssi_stt_inx_len:
				m_data->m_stts_index_len = (int)tagValue;
				break;

		case gcsxt_rssi_positions_avail:
				m_positions_available = (int)tagValue;
				break;
	}
}

//-------------------------------------------------------------------------------
//  ================  TRuleCallPlacesIteratorXmlReader  ====================
//-------------------------------------------------------------------------------

void TRuleCallPlacesIteratorXmlReader::BeginNodeProcessing(WORD tagId)
{
	assert(m_data != NULL);
	assert(m_data->m_grammar != NULL);

	// Setup values of the default fields.
	m_data->m_action_sym_restr = 0xFFFF;
	m_data->m_startup_flag = FALSE;
}

void TRuleCallPlacesIteratorXmlReader::FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems)
{
	if (nodeScanningProblems == FALSE)
	{
		if (m_data->m_state_before_non_term < 0 || m_data->m_state_before_non_term >= m_data->m_grammar->NumParsingStates())
		{
			m_parser->ReportXmlAppError(L"The value of the parsing state is bogus: %d. NumParsingStates: %d.", m_data->m_state_before_non_term, m_data->m_grammar->NumParsingStates());
			return;
		}

		m_data->PrepareIterationSpecial(m_positions_available);
	}

	m_data = NULL;
}

void TRuleCallPlacesIteratorXmlReader::ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case gcsxt_rcpi_startup_flag:
				m_data->m_startup_flag = tagValue;
				break;
	}
}

void TRuleCallPlacesIteratorXmlReader::ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case gcsxt_rcpi_parsing_state:
				m_data->m_state_before_non_term = (WORD)tagValue;
				break;

		case gcsxt_rcpi_non_term:
				m_data->m_non_term = (WORD)tagValue;
				break;

		case gcsxt_rcpi_action_sym:
				m_data->m_action_sym_restr = (WORD)tagValue;
				break;

		case gcsxt_rcpi_positions_avail:
				m_positions_available = (int)tagValue;
				break;
	}
}

//------------------------------------------------------------------------------------
//  =================  TNonTermDefnRulesIteratorXmlReader  ====================
//------------------------------------------------------------------------------------

void TNonTermDefnRulesIteratorXmlReader::BeginNodeProcessing(WORD tagId)
{
	assert(m_data != NULL);
	assert(m_data->m_grammar != NULL);
	assert(m_index2 != NULL);

	// Setup values of the default fields.
	m_data->m_startup_flag = FALSE;
	m_data->m_latest_irule = 0xFFFF;
}

void TNonTermDefnRulesIteratorXmlReader::FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems)
{
	if (nodeScanningProblems == FALSE)
	{
		if (m_data->m_rule_starting_state < 0 || m_data->m_rule_starting_state >= m_data->m_grammar->NumParsingStates())
		{
			m_parser->ReportXmlAppError(L"The value of the rule starting state is bogus: %d. NumParsingStates: %d.", m_data->m_rule_starting_state, m_data->m_grammar->NumParsingStates());
			return;
		}

		m_data->PrepareIterationSpecial(*m_index2, m_positions_available);
	}

	m_data = NULL;
	m_index2 = NULL;
}

void TNonTermDefnRulesIteratorXmlReader::ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case gcsxt_ntdi_iter_diff_asyms:
				m_data->m_iter_diff_action_syms = tagValue;
				break;

		case gcsxt_ntdi_startup_flag:
				m_data->m_startup_flag = tagValue;
				break;
	}
}

void TNonTermDefnRulesIteratorXmlReader::ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case gcsxt_ntdi_rule_start_state:
				m_data->m_rule_starting_state = (WORD)tagValue;
				break;

		case gcsxt_ntdi_rule_non_term:
				m_data->m_rule_non_term = (WORD)tagValue;
				break;

		case gcsxt_ntdi_latest_irule:
				m_data->m_latest_irule = (int)tagValue;
				break;

		case gcsxt_ntdi_positions_avail:
				m_positions_available = (int)tagValue;
				break;
	}
}

//-------------------------------------------------------------------------------------------
//  ===================  TBuildGrammarDerivationSlotXmlReader  ======================
//-------------------------------------------------------------------------------------------

TBuildGrammarDerivationSlotXmlReader::TBuildGrammarDerivationSlotXmlReader()
{
	m_data = NULL;
	m_grammar = NULL;
	m_rule_start_states_index_ptr = NULL;
	m_non_term_defn_rules_index_ptr = NULL;
}

void TBuildGrammarDerivationSlotXmlReader::SetDestination(TBuildGrammarDerivationSlot *dest_buffer, TGrammar *grammar, TRulePosIterIndexLevel2 &rule_start_states_index, TRulePosIterIndexLevel2 &non_term_defn_rules_index)
{
	m_data = dest_buffer;
	m_grammar = grammar;
	m_rule_start_states_index_ptr = &rule_start_states_index;
	m_non_term_defn_rules_index_ptr = &non_term_defn_rules_index;
}

TXmlContainerHandler *TBuildGrammarDerivationSlotXmlReader::OpenSubContainer(WORD tagId, TStringPtr &tagName)
{
	if (tagId != gcsxt_slot_frp && m_data->m_frp.origin == (TFullRulePosOrigin)0xFF)
	{
		m_parser->ReportXmlAppError(L"Definition of the current rule position, that comes from the <FullRulePosition> tag should preceed the definition of the iterator.");
		return(&TXmlReader::s_skip_subtree_handler);
	}

	switch (tagId)
	{
		case gcsxt_slot_frp:
				{
					m_frp_reader.SetDestination(&(m_data->m_frp));
					return(&m_frp_reader);
				}
				break;

		case gcsxt_slot_rule_start_states_iter:
				{
					if (m_data->m_frp.origin != rpo_rule_start_state && m_data->m_frp.origin != rpo_start_state_ctx)
					{
						m_parser->ReportXmlAppError(L"The type of the iterator does not match the type of the rule position origin (%s).", TFullRulePosition::GetFullRulePosOriginEnumName(m_data->m_frp.origin));
						return(&TXmlReader::s_skip_subtree_handler);
					}

					m_iterator_present = TRUE;
					m_iter1_reader.SetDestination(&(m_data->m_rule_start_states_iter), m_grammar, *m_rule_start_states_index_ptr);
					return(&m_iter1_reader);
				}
				break;

		case gcsxt_slot_rule_call_places_iter:
				{
					if (m_data->m_frp.origin != rpo_rule_call_place && m_data->m_frp.origin != rpo_call_place_ctx)
					{
						m_parser->ReportXmlAppError(L"The type of the iterator does not match the type of the rule position origin (%s).", TFullRulePosition::GetFullRulePosOriginEnumName(m_data->m_frp.origin));
						return(&TXmlReader::s_skip_subtree_handler);
					}

					m_iterator_present = TRUE;
					m_iter2_reader.SetDestination(&(m_data->m_rule_call_places_iter), m_grammar);
					return(&m_iter2_reader);
				}
				break;

		case gcsxt_slot_nterm_defn_rules_iter:
				{
					if (m_data->m_frp.origin != rpo_non_term_defn_rule)
					{
						m_parser->ReportXmlAppError(L"The type of the iterator does not match the type of the rule position origin (%s).", TFullRulePosition::GetFullRulePosOriginEnumName(m_data->m_frp.origin));
						return(&TXmlReader::s_skip_subtree_handler);
					}

					m_iterator_present = TRUE;
					m_iter3_reader.SetDestination(&(m_data->m_non_term_defn_rules_iter), m_grammar, *m_non_term_defn_rules_index_ptr);
					return(&m_iter3_reader);
				}
				break;
	}

	assert(FALSE);
	return(NULL);
}

void TBuildGrammarDerivationSlotXmlReader::BeginNodeProcessing(WORD tagId)
{
	assert(m_data != NULL);
	assert(m_grammar != NULL);
	assert(m_rule_start_states_index_ptr != NULL);
	assert(m_non_term_defn_rules_index_ptr != NULL);

	m_data->m_frp.origin = (TFullRulePosOrigin)0xFF;
	m_data->m_upper_layer = -1;
	m_iterator_present = FALSE;
}

void TBuildGrammarDerivationSlotXmlReader::FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems)
{
	if (nodeScanningProblems == FALSE)
	{
		TFullRulePosOrigin origin = m_data->m_frp.origin;
		if (origin == rpo_rule_start_state || origin == rpo_start_state_ctx || origin == rpo_rule_call_place || origin == rpo_call_place_ctx || origin == rpo_non_term_defn_rule)
		{
			// Type of the full rule position origin reqires description of the iterator.
			if (m_iterator_present == FALSE)
			{
				m_parser->ReportXmlAppError(L"The definition of the iterator state is missing.");
			}
		}
	}

	m_data = NULL;
	m_grammar = NULL;
	m_rule_start_states_index_ptr = NULL;
	m_non_term_defn_rules_index_ptr = NULL;
}

void TBuildGrammarDerivationSlotXmlReader::ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case gcsxt_slot_need_more_backw_ctx:
				m_data->m_need_more_backw_ctx = tagValue;
				break;

		case gcsxt_slot_need_more_forw_ctx:
				m_data->m_need_more_forw_ctx = tagValue;
				break;
	}
}

void TBuildGrammarDerivationSlotXmlReader::ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case gcsxt_slot_upper_layer:
				m_data->m_upper_layer = (int)tagValue;
				break;

		case gcsxt_slot_setup_cnt:
				m_data->m_setup_cnt = (DWORD)tagValue;
				break;

		case gcsxt_slot_positions_cnt:
				m_data->m_positions_cnt = (int)tagValue;
				break;

		case gcsxt_slot_setup_cnts_count:
				m_data->m_setup_cnts_count = tagValue;
				break;
	}
}

//--------------------------------------------------------------------------------------------------
//	===================  TSavedConflictingActionAnalysisStateXmlReader  ======================
//--------------------------------------------------------------------------------------------------

void TSavedConflictingActionAnalysisStateXmlReader::SetDestination(GUID &core_db_guid, TGrammarConflictAnalysisHelper &dest_buffer, TGrammar *root_grammar, int dest_igrammar)
{
	assert(root_grammar != NULL);
	assert(dest_igrammar >= 0);

	m_core_db_guid = &core_db_guid;
	m_dest_data = &dest_buffer;

	// Set only the root directory in the helper. The current directory will be set up later.
	m_dest_data->m_root_grammar = root_grammar;
	m_dest_data->m_igrammar = dest_igrammar;
}

TXmlContainerHandler *TSavedConflictingActionAnalysisStateXmlReader::OpenSubContainer(WORD tagId, TStringPtr &tagName)
{
	assert(tagId == gcsxt_sas_derivation_slot);

	if (m_dest_data->m_curr_grammar == NULL)
	{
		// Passed index of the current grammar is bogus or it was not possible to build the indexes.
		return(&TXmlReader::s_skip_subtree_handler);
	}

	if (m_next_islot < 0)
	{
		if (m_next_islot == -1)
		{
			m_parser->ReportXmlAppError(L"The value of the <CurrLevel> tag is not set.");
		}

		return(&TXmlReader::s_skip_subtree_handler);
	}
	else if (m_next_islot >= m_max_trans_path_len)
	{
		m_parser->ReportXmlAppError(L"The number of derivation slots exceeds the limit (%d).", m_max_trans_path_len);
		return(&TXmlReader::s_skip_subtree_handler);
	}

	// It is ok to start reading the slot.
	m_slot_reader.SetDestination(&(m_dest_data->m_curr_path[m_next_islot]), m_dest_data->m_curr_grammar, m_dest_data->m_rule_start_states_index, m_dest_data->m_non_term_defn_rules_index);
	m_next_islot++;
	return(&m_slot_reader);
}

void TSavedConflictingActionAnalysisStateXmlReader::BeginNodeProcessing(WORD tagId)
{
	assert(m_core_db_guid != NULL);
	assert(m_dest_data != NULL);

	// Check index of the current grammar that was passed into SetDestination() and build the indexes.
	int igrammar = m_dest_data->m_igrammar;
	if (m_dest_data->SetGrammarAndPrepareIndexes(m_dest_data->m_root_grammar, igrammar) == FALSE)
	{
		m_parser->ReportXmlAppError(L"An index of the current grammar is bogus or it was not possible to build the iteration indexes.");
		m_dest_data->m_igrammar = igrammar;
	}

	m_next_islot = -1;
	m_max_trans_path_len = TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN;
}

void TSavedConflictingActionAnalysisStateXmlReader::FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems)
{
	if (nodeScanningProblems == FALSE)
	{
		if (m_next_islot >= 0 && m_next_islot < m_max_trans_path_len)
		{
			m_parser->ReportXmlAppError(L"The number of derivation slots (%d) is less than the value of the <MaxTransPathLen> tag (%d).", m_next_islot, m_max_trans_path_len);
		}
	}

	for (int i1=m_max_trans_path_len; i1<TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN; ++i1)
	{
		m_dest_data->m_curr_path[i1].m_frp.origin = (TFullRulePosOrigin)0xFF;
		m_dest_data->m_curr_path[i1].m_setup_cnts_count = 0;
	}

	m_core_db_guid = NULL;
	m_dest_data = NULL;
}

void TSavedConflictingActionAnalysisStateXmlReader::ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case gcsxt_sas_wback_ctx:
				m_dest_data->m_want_backw_ctx = tagValue;
				break;

		case gcsxt_sas_wforw_ctx:
				m_dest_data->m_want_forw_ctx = tagValue;
				break;

		case gcsxt_sas_ctx_paths_pres:
				m_dest_data->m_ctx_paths_present = tagValue;
				break;
	}
}

void TSavedConflictingActionAnalysisStateXmlReader::ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case gcsxt_sas_igrammar:
				{
					if (m_dest_data->m_igrammar != tagValue)
					{
						m_parser->ReportXmlAppError(L"The value of the grammar index from the XML file (%d) does not match the expected grammar index (%d).", (int)tagValue, m_dest_data->m_igrammar);
					}
				}
				break;

		case gcsxt_sas_iconflict:
				{
					m_dest_data->m_iconflict = (int)tagValue;
				}
				break;

		case gcsxt_sas_iaction:
				{
					m_dest_data->m_iaction = (int)tagValue;
				}
				break;

		case gcsxt_sas_clev:
				{
					int value = (int)tagValue;
					if (value < 0 || value > TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN)
					{
						m_parser->ReportXmlAppError(L"The value of the <CurrLevel> tag (%d) is bogus. The value should be in the range from 0 to %d.", value, TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN);
						break;
					}

					m_dest_data->m_clev = value;
					m_next_islot = 0;
				}
				break;

		case gcsxt_sas_cnt_steps:
				{
					m_dest_data->m_cnt_steps = tagValue;
				}
				break;

		case gcsxt_sas_iter_setup_cnt:
				{
					m_dest_data->m_iter_setup_cnt = (DWORD)tagValue;
				}
				break;

		case gcsxt_sas_clev_bottom:
				{
					m_dest_data->m_clev_bottom = (int)tagValue;
				}
				break;

		case gcsxt_sas_max_trans_path:
				{
					m_max_trans_path_len = (int)tagValue;
				}
				break;
	}
}

void TSavedConflictingActionAnalysisStateXmlReader::ProcessGuidField(WORD tagId, GUID &tagValue, TStringPtr &tagName)
{
	assert(tagId == gcsxt_sas_database_guid);
	*m_core_db_guid = tagValue;
}


