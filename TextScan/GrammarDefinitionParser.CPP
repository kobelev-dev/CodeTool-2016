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
#include  "TextScan/GrammarDefinitionParser.H"

#define	USE_CURV_BRACKET_EXTENSION			// YACC compatibility option is OFF by default. Note that even when
												// it is ON there is no full compatibility with YACC.
#undef	DUMP_RULE_POS_BUCKETS_LENGTHS			// Debug code for evaluataing quality of the hash function.
#undef	WANT_SLR_SUPPORT						// Switch this definition ON to enable the SLR grammar conversion method.
												// Disabling SLR gives minor performance boost.

//--------------------------------------------------------------------------
//  ============  TGrammarDefnParserReportFile  =====================
//--------------------------------------------------------------------------

TGrammarDefnParserReportFile::TGrammarDefnParserReportFile(TGrammarDefinitionParser *inst, const wchar_t *cgrm_name, int cgrm_index,
												int num_symbols, int num_non_terms, int num_rules, const wchar_t *fname)
			: TDestinationFile(fname), m_grdef_parser(*inst)
{
	// Open the disk file. It is ok not to check success of opening here because at the end of writing there
	// will be an overall success/failure check.
	PrepareDiskFile();

	// Write basic header into the report.
	WriteLine();
	WriteFmtLine(L"Source file: %s", m_grdef_parser.m_src_props.grms_file_name);
	WriteFmtLine(L"Grammar: %s", cgrm_name);
	if (cgrm_index >= 0)
		WriteFmtLine(L"Grammar index: %d", cgrm_index);
	WriteFmtLine(L"Num terminal symbols: %d", num_symbols);
	WriteFmtLine(L"Num non terminals: %d", num_non_terms);
	WriteFmtLine(L"Num rules: %d", num_rules);

	// Write empty line after all headers.
	WriteLine();
}

void TGrammarDefnParserReportFile::WriteSectionHeader(const wchar_t *hdr_msg)
{
	WriteFmtLine(L"  %s.", hdr_msg);
	TGrammarSymbolsHelper::WriteReportDelimiter(*this);
}

void TGrammarDefnParserReportFile::CloseReport(const wchar_t *error_message)
{
	// Write final empty line and close the report.
	WriteLine();
	Close();

	// Write name of this report file to console.
	if (GetErrorFlag() == FALSE)
		m_grdef_parser.Scanner()->TraceEvent(NULL, TGDP_GENERAL, L"Rprt: \"%s\".", FileName());
	else m_grdef_parser.Scanner()->TraceEvent(NULL, TGDP_GENERAL, L"Error writing %s to: \"%s\".", error_message, FileName());
}

void TGrammarDefnParserReportFile::DeleteReport()
{
	// Close and delete the report.
	if (GetCurrLen() == 0)
		WriteLine();

	Close();
	DeleteClosedDiskFile();

	// Notify console in case of error.
	if (GetErrorFlag() == TRUE)
		m_grdef_parser.Scanner()->TraceEvent(NULL, TGDP_GENERAL, L"Error deleting report: \"%s\".", FileName());
}

//----------------------------------------------------------------------------
//  ================  TGrammarDefinitionParser  ======================
//----------------------------------------------------------------------------

// Eof symbol is not defined in the grammar explicitly. Nevertheless it is present in the table
// of terminal symbols of every grammar.
static PrEventHeader g_EofPseudoSymbolHeader;

// Name of the subdirectory for storing parsing reports and gramme sources as HTML tables.
const wchar_t *TGrammarDefinitionParser::DEF_REPORTS_SUBDIR = L"Reports";

TGrammarDefinitionParser::TGrammarDefinitionParser(TGrammar &grammar, const wchar_t *dir_for_reports, TMidLevScanner *pSrc, bool build_parsing_table)
		: TGenericParser(pSrc), m_dest_grammar(grammar), m_xpcts_root(NULL)
{
	// Verify error codes from the local enum.
	assert(gerr_base == perb_grammar_defn_base);
	assert(gerr_max <= perb_grammar_defn_max);

	m_build_parsing_table = build_parsing_table;

	m_src_props.Clear();
	m_axioma = 0;

	m_cxpct = &m_xpcts_root;
	m_cnt_nesting = 0;

	m_curr_section = grsc_none;
	m_rule_sects_cnt = 0;

	m_warn_on_unused_symbols = TRUE;
	m_warn_on_rules_without_ids = FALSE;
	m_warn_on_unused_loc_markers = TRUE;

	m_latest_rule_non_term = 0xFFFF;
	m_scanning_phase = FALSE;

	if (TPathHelper::IsEmptyPath(dir_for_reports) == FALSE)
	{
		m_reports_directory.Append(dir_for_reports);
		TPathHelper::ConvertToBackSlashes(m_reports_directory);
		TPathHelper::RemoveTrailingSlash(m_reports_directory);
		assert(m_reports_directory.GetXpndError() == FALSE);
	}

	m_slr_method = TRUE;
}

void TGrammarDefinitionParser::ResetParser()
{
	TGenericParser::ResetParser();

	if (m_scanner != NULL)
		m_scanner->MidLevReset(TRUE, TRUE);

	m_src_props.Clear();
	m_axioma = 0;

	m_src_data.ReleaseArrays();
	m_xpcts_root.ReleaseChildren();

	m_cxpct = &m_xpcts_root;
	m_cnt_nesting = 0;

	m_curr_section = grsc_none;
	m_rule_sects_cnt = 0;

	// Restore the default state of warnings.
	m_warn_on_unused_symbols = TRUE;
	m_warn_on_rules_without_ids = FALSE;
	m_warn_on_unused_loc_markers = TRUE;

	m_latest_rule_non_term = 0xFFFF;
	m_scanning_phase = FALSE;
}

bool TGrammarDefinitionParser::Parse()
{
	//
	// Scanning and processing the grammar definition file.
	//
	wchar_t buffer1[80], buffer2[80];
	TDateTime dt_start = CurrDateTime();
	if (m_scanner->GetCurrTraceMode() & gdtr_steps)
	{
		// Report start of scanning.
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"----------------------- Grammar parsing -----------------------");
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"%s: Scanning startup.", FormatDateTime(dt_start, buffer1, 80, FALSE));

		TFileInfoItem *src_info = m_scanner->MajorSrcInfo().mj_src_info;
		if (src_info != NULL)
		{
			// Name of the major source file is already known. It may be not known yet if the command line is used.
			TFileNameBuffer src_file_name;
			src_file_name.Append(src_info->data.info.file_name);
			TPathHelper::ConvertToBackSlashes(src_file_name);
			m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"File name: \"%s\".", src_file_name.DataPtr());
		}
	}

	// Ensure that scanner is already set because syntax errors should be reported to it.
	assert(m_scanner != NULL);
	m_src_props.parsing_id = m_scanner->CbkHandler().GetParsingId();

	// Wipe existing contents of the destination grammar if any. This will also set the processing_result field to grpr_none.
	m_dest_grammar.Clear();

	if (PrepareReporsDirectory(FALSE) == TRUE)
	{
		// Wipe the source grammar report file from the previous run if any.
		TFileNameBuffer fname_buffer;
		DeleteReportFile(GenerateReportFileName(fname_buffer, SOURCE_GRAMMAR_NAME, -1, BASIC_REPORT_FNAME));
	}

	// First slot in the terminal symbols table should have fixed value.
	TTerminalSymbol eof_pseudo_sym = { TRUE, 0, TGrammar::StrDupe(L"<EOF>"), 0, ltx_eof };
	if (eof_pseudo_sym.name_value != NULL)
	{
		// Symbol defn can be added to the grammar.
		m_src_data.m_symbols.AppendItem(eof_pseudo_sym);
		m_src_data.m_symbol_areas.AppendItem(g_EofPseudoSymbolHeader);

		// Read the source file.
		m_scanning_phase = TRUE;
		m_scanner->Console().SetMajorStatus(L"Scanning ...");
		DoTheScanning();
		m_scanning_phase = FALSE;

		// Check the results of scanning and build the analysis tables.
		DoThePostScanningWork();
	}
	else
	{
		// Write an error.
		ReportConversionError(gerr_symbol_def_stmt_error, L"Out of memory while adding definition of the EOF symbol to the grammar.");
	}

	// Do the final actions.
	if (m_scanner->GetCurrTraceMode() & gdtr_steps)
	{
		// Report process termination.
		TDateTime dt_now = CurrDateTime();
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"---------------------- End of processing ----------------------");
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"%s: Elapsed time: %s.",
							FormatDateTime(dt_now, buffer1, 80, FALSE), FormatDuration(dt_now-dt_start, buffer2, 80, TRUE));
	}

	// Give out the results of parsing.
	return(m_scanner->GetErrorsCount() == 0);
}

bool TGrammarDefinitionParser::GenerateGrammarNameForReporting(TFileNameBuffer &file_name_buff, TNameBuffer &name_buff, TGrammar &grm)
{
	name_buff.ClearBuffer();
	if (grm.parent_grammar == NULL)
	{
		// This grammar has fixed name.
		name_buff.Append(L"_RootGrammar_");
	}
	else
	{
		// Process path of nested grammars up to the root grammar.
		name_buff.Append(L"_");
		TGrammar *pGrm = &grm;
		while (pGrm->parent_grammar != NULL)
		{
			name_buff.Insert(0, pGrm->parent_grammar->xpct_conflicts[pGrm->parent_xpct_inx].xpct_conflict_name);
			name_buff.Insert(0, L"_");
			pGrm = pGrm->parent_grammar;
		}
	}

	// Reserve space in the report name buffer.
	if (file_name_buff.ReserveSpace(m_reports_directory.NumItems() + name_buff.NumItems()+MAX_PATH) == FALSE)
	{
		return(FALSE);
	}

	// Give out the success flag from the buffer.
	return(name_buff.GetXpndError() == FALSE);
}

bool TGrammarDefinitionParser::PrepareReporsDirectory(bool emit_errors)
{
	if (TPathHelper::IsEmptyPath(m_reports_directory) == FALSE)
		return(TRUE);

	// Directory name was not passed explicitly.
	TFileInfoItem *src_info = m_scanner->MajorSrcInfo().mj_src_info;
	if (src_info == NULL)
	{
		if (emit_errors == TRUE)
			ReportConversionError(gerr_preparing_reports_dir, L"Info on the major grammar source file is missing.");
		return(FALSE);
	}

	// Setup report directory as subdirectory of the dir with the primary source file.
	TFileNameBuffer dir_buffer;
	if (TPathHelper::GetDirectoryName(dir_buffer, src_info->data.info.file_name) == TRUE && dir_buffer.Append(L"\\") == TRUE && dir_buffer.Append(DEF_REPORTS_SUBDIR) == TRUE)
	{
		if (::CreateDirectoryW(dir_buffer, NULL) == FALSE)
		{
			DWORD err = ::GetLastError();
			if (err != ERROR_ALREADY_EXISTS)
			{
				if (emit_errors == TRUE)
					ReportConversionError(gerr_preparing_reports_dir, L"Error creating the reports directory: \"%s\". Win32 error: %d.", dir_buffer.DataPtr(), err);
				return(FALSE);
			}
		}
	}
	else
	{
		if (emit_errors == TRUE)
			ReportConversionError(gerr_preparing_reports_dir, L"Error preparing the name of the reports directory.");
		return(FALSE);
	}

	// Success.
	m_reports_directory.TakeContentsFrom(dir_buffer);
	return(TRUE);
}

wchar_t *TGrammarDefinitionParser::GenerateReportFileName(TFileNameBuffer &file_name_buff, const wchar_t *grm_name, int grm_index, const wchar_t *fname_suffix)
{
	wchar_t inx_buff[20];
	if (grm_index >= 0)
		swprintf(inx_buff, 20, L"_%02d", grm_index);
	else inx_buff[0] = 0;

	swprintf(file_name_buff.DataPtr(), file_name_buff.NumAllocedItems(), L"%s\\grc%s%s%s", m_reports_directory.DataPtr(), inx_buff, grm_name, fname_suffix);
	return(file_name_buff.DataPtr());
}

void TGrammarDefinitionParser::GenerateBasicReport(TFileNameBuffer &file_name_buff, const wchar_t *grm_name, int grm_index, TTerminalSymbolsArray &term_syms, TNonTerminalsArray &non_terms, TGrammarRulesArray &rules)
{
	// Dump basic report about the processed statements.
	ReportMessage(gerr_scanning_rprt, L"-----------------------------------------");
	ReportMessage(gerr_scanning_rprt, L"Grammar: %s", grm_name);
	if (grm_index >= 0)
		ReportMessage(gerr_scanning_rprt, L"Grammar index: %d", grm_index);
	ReportMessage(gerr_scanning_rprt, L"Terminal symbols: %d", term_syms.NumItems());
	ReportMessage(gerr_scanning_rprt, L"Non terminals: %d", non_terms.NumItems());
	ReportMessage(gerr_scanning_rprt, L"Rules: %d", rules.NumItems());

	if (m_scanner->GetCurrTraceMode() & gdtr_reports)
	{
		if (PrepareReporsDirectory(TRUE) == FALSE)
			return;

		// Instantiate and open the report file.
		TGrammarDefnParserReportFile rprt(this, grm_name, grm_index, term_syms.NumItems(), non_terms.NumItems(), rules.NumItems(),
										GenerateReportFileName(file_name_buff, grm_name, grm_index, BASIC_REPORT_FNAME));

		// Make the terminal symbols report.
		rprt.WriteSectionHeader(L"Terminal symbols");
		for (int i1=0; i1<term_syms.NumItems(); ++i1)
		{
			wchar_t buff_40_chars[40];
			rprt.WriteFmtLine(L" (sym=%03d): %s", i1, TGrammarSymbolsHelper::GetSymbolName(term_syms, non_terms, i1, buff_40_chars));
		}

		TGrammarSymbolsHelper::WriteReportDelimiter(rprt);

		// Make the non terminal symbols report.
		TSymbolsArray non_terms_list;
		if (TGrammarSymbolsHelper::CreateSortedNonTerminalsList(non_terms, &rules, non_terms_list, ntsm_sym_val) == TRUE)
		{
			// Dump the same list of non terminals in 4 different formats.
			TGrammarSymbolsHelper::SortAndDumpNonTerminalsList(rprt, non_terms, &rules, non_terms_list, ntsm_sym_val);
			TGrammarSymbolsHelper::SortAndDumpNonTerminalsList(rprt, non_terms, &rules, non_terms_list, ntsm_alpha_num);
			TGrammarSymbolsHelper::SortAndDumpNonTerminalsList(rprt, non_terms, &rules, non_terms_list, ntsm_special_anum);
			TGrammarSymbolsHelper::SortAndDumpNonTerminalsList(rprt, non_terms, &rules, non_terms_list, ntsm_sect_rule);
		}
		else
		{
			ReportConversionError(gerr_no_mem_for_non_terms_list, L"Low on memory while creating the list of non terminals.");
		}

		// Make the rules report.
		rprt.WriteLine();
		rprt.WriteLine(L"  Grammar rules");
		WORD prev_non_term = 0;
		for (int i2=0; i2<rules.NumItems(); ++i2)
		{
			TGrammarRule &rule = rules[i2];
			wchar_t buff_40_chars[40];
			if (prev_non_term != rule.non_term)
				rprt.WriteLine(L"------------------------------------------------------------------------------------------------------");

			rprt.WriteFmt(L" (inx=%03d) %s(%hd):", i2, TGrammarSymbolsHelper::GetSymbolName(term_syms, non_terms, rule.non_term, buff_40_chars), rule.non_term);

			for (int isym=0; isym<rule.Length(); ++isym)
				rprt.WriteFmt(L" %s(%hd)", TGrammarSymbolsHelper::GetSymbolName(term_syms, non_terms, rule.symbols[isym], buff_40_chars), rule.symbols[isym]);

			rprt.WriteLine();
			prev_non_term = rule.non_term;
		}

		// Close the report.
		rprt.WriteLine(L"------------------------------------------------------------------------------------------------------");
		rprt.CloseReport(L"basic report");
	}
}

void TGrammarDefinitionParser::ReportMessage(TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...)
{
	// Simple processing.
	va_list vargs;
	va_start(vargs, format);
	ReportErrWarnInternal(NULL, lerrc_message, err_code, format, vargs);
	va_end(vargs);
}

void TGrammarDefinitionParser::ReportWarning(TLexema &err_lex, TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...)
{
	// Simple processing.
	va_list vargs;
	va_start(vargs, format);
	ReportErrWarnInternal(&err_lex.origin, lerrc_syntax_warn, err_code, format, vargs);
	va_end(vargs);
}

void TGrammarDefinitionParser::ReportWarning(PrEventHeader &origin, TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...)
{
	// Simple processing.
	va_list vargs;
	va_start(vargs, format);
	ReportErrWarnInternal(&origin, lerrc_syntax_warn, err_code, format, vargs);
	va_end(vargs);
}

void TGrammarDefinitionParser::ReportError(TLexema &err_lex, TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...)
{
	// Simple processing.
	va_list vargs;
	va_start(vargs, format);
	ReportErrWarnInternal(&err_lex.origin, lerrc_syntax_err, err_code, format, vargs);
	va_end(vargs);
}

void TGrammarDefinitionParser::ReportError(PrEventHeader &origin, TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...)
{
	// Simple processing.
	va_list vargs;
	va_start(vargs, format);
	ReportErrWarnInternal(&origin, lerrc_syntax_err, err_code, format, vargs);
	va_end(vargs);
}

void TGrammarDefinitionParser::ReportConversionWarning(TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...)
{
	// Simple processing.
	va_list vargs;
	va_start(vargs, format);
	ReportErrWarnInternal(NULL, lerrc_syntax_warn, err_code, format, vargs);
	va_end(vargs);
}

void TGrammarDefinitionParser::ReportConversionError(TGrammarDefnParserErrorCode err_code, const wchar_t *format, ...)
{
	// Simple processing.
	va_list vargs;
	va_start(vargs, format);
	ReportErrWarnInternal(NULL, lerrc_syntax_err, err_code, format, vargs);
	va_end(vargs);
}

void TGrammarDefinitionParser::ReportErrWarnInternal(PrEventHeader *ctx, TLexErrorClass err_class, int msg_code, const wchar_t *format, va_list vargs)
{
	// Prepare the message.
	wchar_t buff[2*MAX_PATH+80];
	if (vargs != NULL)
		vswprintf(buff, 2*MAX_PATH+80, format, vargs);

	// Call scanner to generate and save the error.
	m_scanner->AddErrorWarning(ctx, 0, err_class, msg_code, (vargs != NULL) ? buff : format);
}

void TGrammarDefinitionParser::DeleteReportFile(const wchar_t *fname)
{
	TDestinationFile rprt(fname);
	if (rprt.WipeDiskFile() == FALSE)
	{
		// Just trace this.
		Scanner()->TraceEvent(NULL, TGDP_GENERAL, L"Error deleting report: \"%s\".", rprt.FileName());
	}
}

bool TGrammarDefinitionParser::GetIntroOrigin(PrEventHeader &hdr, WORD sym)
{
	// Note: Do not allow to give out the header of the EOF symbol.
	if (sym > TerminalSymbolsBase && sym < TerminalSymbolsBase+m_src_data.m_symbols.NumItems())
	{
		// Terminal symbol.
		hdr = m_src_data.m_symbol_areas[sym-TerminalSymbolsBase];
		return(TRUE);
	}
	else if (sym >= IgnoreLexTypeBase && sym < IgnoreLexTypeBase+m_src_data.m_ignore_lex.NumItems())
	{
		// Ignore lex type record.
		hdr = m_src_data.m_ignore_lex_areas[sym-IgnoreLexTypeBase];
		return(TRUE);
	}
	else if (sym >= ErrorLexTypeBase && sym < ErrorLexTypeBase+m_src_data.m_error_lex.NumItems())
	{
		// Error lex type record.
		hdr = m_src_data.m_error_lex_areas[sym-ErrorLexTypeBase];
		return(TRUE);
	}
	else if (sym >= NonTermForwardDefnsBase && sym < NonTermForwardDefnsBase+m_src_data.m_fwd_def_non_terminals.NumItems())
	{
		// Forward declaration of the non terminal.
		if (m_src_data.m_fwd_def_non_terminals[sym-NonTermForwardDefnsBase].symbol_name == NULL)
		{
			assert(FALSE);
			hdr.Clear();
			return(FALSE);
		}

		hdr = m_src_data.m_fwd_def_non_term_areas[sym-NonTermForwardDefnsBase];
		return(TRUE);
	}
	else if (sym >= NonTerminalsBase && sym < NonTerminalsBase+m_src_data.m_non_terminals.NumItems())
	{
		// Non terminal.
		hdr = m_src_data.m_non_term_areas[sym-NonTerminalsBase];
		return(TRUE);
	}
	else if (sym >= RuleObjectsBase && sym < LocationObjectsBase)
	{
		// This is some grammar rule.
		if (sym < RuleObjectsBase+m_src_data.m_rules.NumItems())
		{
			hdr = m_src_data.m_rule_intro_areas[sym-RuleObjectsBase].rule_origin;
			return(TRUE);
		}

		int rules_inx_base = RuleObjectsBase+m_src_data.m_rules.NumItems();
		return(GetRuleOriginHier(hdr, sym, rules_inx_base, m_xpcts_root));
	}
	else if (sym >= LocationObjectsBase && sym < LocationObjectsBase+m_src_data.m_location_objects.NumItems())
	{
		// Expected conflict location object.
		hdr = m_src_data.m_loc_object_intro_areas[sym-LocationObjectsBase];
		return(TRUE);
	}
	else if (sym >= NestedResultsBase && sym < NestedResultsBase+m_src_data.m_nested_processing_results.NumItems())
	{
		// Nested processing result.
		hdr = m_src_data.m_nested_proc_res_intro_areas[sym-NestedResultsBase];
		return(TRUE);
	}
	else if (sym >= XpctConflictsBase && sym < NextCategoryBase)
	{
		// This should be an expected grammar conflict.
		int xpcts_inx_base = XpctConflictsBase;
		return(GetExpectedConflictOriginHier(hdr, sym, xpcts_inx_base, m_xpcts_root));
	}

	// Input symbol is not falling into any defined range of symbols.
	assert(FALSE);
	hdr.Clear();
	return(FALSE);
}

bool TGrammarDefinitionParser::GetRuleOriginHier(PrEventHeader &hdr, WORD sym, int &inx_base, TExpectedConflictSupp &ctx)
{
	// Check the range of the local rules.
	int num_local_objects = ctx.m_resolution_rules.NumItems();
	if (sym-inx_base < num_local_objects)
	{
		// Symbol value falls into the range of the current object.
		hdr = ctx.m_res_rule_intro_areas[sym-inx_base].rule_origin;
		return(TRUE);
	}

	// Iterate nested objects.
	inx_base += num_local_objects;
	for (TListIter<TExpectedConflictSupp> iter(ctx.m_child_supps); iter; ++iter)
	{
		if (GetRuleOriginHier(hdr, sym, inx_base, iter.CurrItem()) == TRUE)
			return(TRUE);
	}

	// Passed symbol does not belong to the current subtree.
	assert(inx_base < LocationObjectsBase);
	hdr.Clear();
	return(FALSE);
}

bool TGrammarDefinitionParser::GetExpectedConflictOriginHier(PrEventHeader &hdr, WORD sym, int &inx_base, TExpectedConflictSupp &ctx)
{
	// Check local xpct conflicts.
	int num_local_objects = 0;
	for (TListIter<TExpectedConflictSupp> iter1(ctx.m_child_supps); iter1; ++iter1)
	{
		if (inx_base+num_local_objects == sym)
		{
			// Requested conflict is found.
			hdr = iter1.CurrItem().m_intro_area;
			return(TRUE);
		}

		num_local_objects++;
	}

	// Iterate same conflicts once again.
	inx_base += num_local_objects;
	for (TListIter<TExpectedConflictSupp> iter2(ctx.m_child_supps); iter2; ++iter2)
	{
		if (GetExpectedConflictOriginHier(hdr, sym, inx_base, iter2.CurrItem()) == TRUE)
			return(TRUE);
	}

	// Passed symbol does not belong to the current subtree.
	assert(inx_base < NextCategoryBase);
	hdr.Clear();
	return(FALSE);
}

bool TGrammarDefinitionParser::GetPreparedGrammarSymbolOrigin(PrEventHeader *&org_info, int igrammar, WORD sym)
{
	assert(igrammar >= 0);

	int igrm_base = 1;
	TExpectedConflictSupp *grm_supp = &m_xpcts_root;
	if (igrammar <= 0 || GetGrammarSuppHier(grm_supp, igrammar, igrm_base, m_xpcts_root) == TRUE)
	{
		// The passed index of the grammar is good.
		if (sym >= TerminalSymbolsBase && sym-TerminalSymbolsBase < grm_supp->m_terminal_symbols_bk_refs.NumItems())
		{
			org_info = m_src_data.m_symbol_areas.ItemPtr(grm_supp->m_terminal_symbols_bk_refs[sym-TerminalSymbolsBase]);
			return(TRUE);
		}
		if (sym >= NonTerminalsBase && sym-NonTerminalsBase < grm_supp->m_non_terminals_bk_refs.NumItems())
		{
			org_info = m_src_data.m_non_term_areas.ItemPtr(grm_supp->m_non_terminals_bk_refs[sym-NonTerminalsBase]);
			return(TRUE);
		}
	}

	// The params are bad or the parsing was not successful.
	org_info = NULL;
	return(FALSE);
}

bool TGrammarDefinitionParser::GetPreparedGrammarRuleOrigin(TGrammarRuleOrigin *&org_info, int igrammar, int irule)
{
	assert(igrammar >= 0 && irule >= 0);

	int igrm_base = 1;
	TExpectedConflictSupp *grm_supp = &m_xpcts_root;
	if (igrammar <= 0 || GetGrammarSuppHier(grm_supp, igrammar, igrm_base, m_xpcts_root) == TRUE)
	{
		// The passed index of the grammar is good.
		int num_res_rules = grm_supp->m_resolution_rules.NumItems();
		if (irule < num_res_rules)
		{
			// Requested rule is a resolution rule.
			org_info = grm_supp->m_res_rule_intro_areas.ItemPtr(irule);
			return(TRUE);
		}
		else if (irule-num_res_rules < grm_supp->m_generated_rules_bk_refs.NumItems())
		{
			// Requested rule was taken from the source grammar.
			org_info = m_src_data.m_rule_intro_areas.ItemPtr(grm_supp->m_generated_rules_bk_refs[irule-num_res_rules]);
			return(TRUE);
		}
	}

	// The params are bad or the parsing was not successful.
	org_info = NULL;
	return(FALSE);
}

bool TGrammarDefinitionParser::GetGrammarSuppHier(TExpectedConflictSupp *&grm_supp, int igrammar, int &igrm_base, TExpectedConflictSupp &ctx)
{
	// Check local supps.
	int num_local_grammars = 0;
	for (TListIter<TExpectedConflictSupp> iter1(ctx.m_child_supps); iter1; ++iter1)
	{
		if (iter1.CurrItem().m_resolution_rules.NumItems() == 0)
			continue;

		// Current supp contains the nested grammar.
		if (igrm_base+num_local_grammars == igrammar)
		{
			// Requested supp is found.
			grm_supp = &(iter1.CurrItem());
			return(TRUE);
		}

		num_local_grammars++;
	}

	// Iterate same supps once again.
	igrm_base += num_local_grammars;
	for (TListIter<TExpectedConflictSupp> iter2(ctx.m_child_supps); iter2; ++iter2)
	{
		if (GetGrammarSuppHier(grm_supp, igrammar, igrm_base, iter2.CurrItem()) == TRUE)
			return(TRUE);
	}

	// Grammar with the passed index does not belong to the current subtree.
	grm_supp = NULL;
	return(FALSE);
}

wchar_t *TGrammarDefinitionParser::PrepareSrcLocInfo(PrEventHeader &hdr, wchar_t *line_num_buff, int len_buff)
{
	assert(len_buff >= 40);

	// Pick up the file that is mentioned in the passed loc info.
	TFileInfo *finfo = hdr.src_area.file_info;
	if (finfo == NULL)
	{
		// File is unknown.
		swprintf(line_num_buff, len_buff, L"(fileOffs=%ld)", hdr.src_area.area_beg);
		return(L"NullFileInfo");
	}

	// Convert file offset into the line number.
	long lnum = -1;
	if (finfo->CheckLinesInfo() == TRUE || finfo->BuildLinesInfo() == TRUE)
		lnum = finfo->GetLineNumber(hdr.src_area.area_beg);

	if (lnum > 0)
	{
		swprintf(line_num_buff, len_buff, L"%ld", lnum);
	}
	else
	{
		swprintf(line_num_buff, len_buff, L"(fileOffs=%ld)", hdr.src_area.area_beg);
	}

	// Return name of the file.
	return(finfo->info.file_name);
}

WORD TGrammarDefinitionParser::FindObjectByAppId(WORD id)
{
	if (id == 0)
	{
		// Objects with zero app ids are considered as "not existing".
		return(0);
	}

	// Search among the terminal symbols.
	for (int i1=1; i1<m_src_data.m_symbols.NumItems(); ++i1)
	{
		if (m_src_data.m_symbols[i1].app_id == id)
		{
			return((WORD)(TerminalSymbolsBase+i1));
		}
	}

	// Search among non terminal forward declarations.
	for (int i2=0; i2<m_src_data.m_fwd_def_non_terminals.NumItems(); ++i2)
	{
		if (m_src_data.m_fwd_def_non_terminals[i2].symbol_name != NULL && m_src_data.m_fwd_def_non_terminals[i2].app_id == id)
		{
			return((WORD)(NonTermForwardDefnsBase+i2));
		}
	}

	// Search among established non terminals.
	for (int i3=0; i3<m_src_data.m_non_terminals.NumItems(); ++i3)
	{
		if (m_src_data.m_non_terminals[i3].app_id == id)
		{
			return((WORD)(NonTerminalsBase+i3));
		}
	}

	// Search among the source rules.
	for (int i4=0; i4<m_src_data.m_rules.NumItems(); ++i4)
	{
		if (m_src_data.m_rules[i4].rule_app_id == id)
		{
			return((WORD)(RuleObjectsBase+i4));
		}
	}

	// Search among the nested processing results.
	for (int i5=0; i5<m_src_data.m_nested_processing_results.NumItems(); ++i5)
	{
		if (m_src_data.m_nested_processing_results[i5].nest_res_app_id == id)
		{
			return((WORD)(NestedResultsBase+i5));
		}
	}

	// Search among rules, nested results and expected conflicts.
	int rules_inx_base = RuleObjectsBase+m_src_data.m_rules.NumItems();
	int xpct_conflicts_inx_base = XpctConflictsBase;
	return(FindObjectByAppIdHier(id, rules_inx_base, xpct_conflicts_inx_base, m_xpcts_root));
}

WORD TGrammarDefinitionParser::FindObjectByAppIdHier(WORD app_id, int &rules_inx_base, int &xpct_conflicts_inx_base, TExpectedConflictSupp &ctx)
{
	// Search among local grammar rules.
	for (int irule=0; irule<ctx.m_resolution_rules.NumItems(); ++irule)
	{
		if (ctx.m_resolution_rules[irule].rule_app_id == app_id)
		{
			return((WORD)(rules_inx_base+irule));
		}
	}

	// Search among expected grammar conflicts.
	int num_local_xpcts = 0;
	for (TListIter<TExpectedConflictSupp> iter1(ctx.m_child_supps); iter1; ++iter1)
	{
		if (iter1.CurrItem().m_xpct_proto.xpct_conflict_app_id == app_id)
		{
			return((WORD)(xpct_conflicts_inx_base+num_local_xpcts));
		}
		num_local_xpcts++;
	}

	// Process the nested grammars if any.
	rules_inx_base += ctx.m_resolution_rules.NumItems();
	xpct_conflicts_inx_base += num_local_xpcts;
	for (TListIter<TExpectedConflictSupp> iter2(ctx.m_child_supps); iter2; ++iter2)
	{
		WORD res = FindObjectByAppIdHier(app_id, rules_inx_base, xpct_conflicts_inx_base, iter2.CurrItem());
		if (res != 0)
		{
			return(res);
		}
	}

	// Passed app_id does not belong to the current subtree.
	return(0);
}

bool TGrammarDefinitionParser::CheckLexTypeMapping(TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt, PrEventHeader &conflict_origin)
{
	//
	//  Method returns TRUE if passed lex type is not mapped to anything and can be used for new object.
	//

	// Check the table of terminal symbols.
	for (int i1=TerminalSymbolsBase+1; i1<m_src_data.m_symbols.NumItems(); ++i1)
	{
		if (m_src_data.m_symbols[i1].lex_type != lex_type)
		{
			continue;
		}

		if (m_src_data.m_symbols[i1].check_subt == FALSE)
		{
			// Processing for this lex type is already defined for all sybtypes.
			conflict_origin = m_src_data.m_symbol_areas[i1];
			return(FALSE);
		}

		if (check_subt == FALSE || m_src_data.m_symbols[i1].lex_subt.subtype == lex_subt.subtype)
		{
			// This is subtype processing mismatch.
			conflict_origin = m_src_data.m_symbol_areas[i1];
			return(FALSE);
		}
	}

	// Check the table of ignore lex records.
	for (int i2=0; i2<m_src_data.m_ignore_lex.NumItems(); ++i2)
	{
		if (m_src_data.m_ignore_lex[i2].lex_type != lex_type)
		{
			continue;
		}

		if (m_src_data.m_ignore_lex[i2].check_subt == FALSE)
		{
			// Processing for this lex type is already defined for all sybtypes.
			conflict_origin = m_src_data.m_ignore_lex_areas[i2];
			return(FALSE);
		}

		if (check_subt == FALSE || m_src_data.m_ignore_lex[i2].lex_subt.subtype == lex_subt.subtype)
		{
			// Subtype processing mismatch.
			conflict_origin = m_src_data.m_ignore_lex_areas[i2];
			return(FALSE);
		}
	}

	// Check table of error lex records.
	for (int i3=0; i3<m_src_data.m_error_lex.NumItems(); ++i3)
	{
		if (m_src_data.m_error_lex[i3].lex_type != lex_type)
		{
			continue;
		}

		if (m_src_data.m_error_lex[i3].check_subt == FALSE)
		{
			// Processing for this lex type is already defined for all sybtypes.
			conflict_origin = m_src_data.m_error_lex_areas[i3];
			return(FALSE);
		}

		if (check_subt == FALSE || m_src_data.m_error_lex[i3].lex_subt.subtype == lex_subt.subtype)
		{
			// Subtype processing mismatch.
			conflict_origin = m_src_data.m_error_lex_areas[i3];
			return(FALSE);
		}
	}

	// Processing for this lexema type is not defined yet.
	return(TRUE);
}

bool TGrammarDefinitionParser::CheckName(PrEventHeader &ctx, TStrPtrInfo &name_ptr, TGrammarDefnParserErrorCode err_code)
{
	// Length of the names in the grammar definition are limited for simplicity of comparison and printing.
	if (name_ptr.m_len > TGrammar::MAX_NAME_LENGTH)
	{
		wchar_t loc_buff[80];
		ReportError(ctx, err_code, L"The name \"%s...\" is longer than %d max allowed characters.", name_ptr.CopyWithTruncationTo(loc_buff, 80), TGrammar::MAX_NAME_LENGTH);
		return(FALSE);
	}

	// Check for reserved names.
	if (name_ptr == L"EOF" || name_ptr == L"Eof" || name_ptr == L"eof")
	{
		wchar_t loc_buff[80];
		ReportError(ctx, err_code, L" The name \"%s\" is a reserved word.", name_ptr.CopyWithTruncationTo(loc_buff, 80), TGrammar::MAX_NAME_LENGTH);
		return(FALSE);
	}

	wchar_t name_buff[TGrammar::MAX_NAME_LENGTH+1];
	name_ptr.CopyToVerifiedBuffer(name_buff);

	// Ensure that the passed name is not a grammar symbol.
	WORD other_sym = m_src_data.FindSymbol(name_ptr);
	if (other_sym != 0)
	{
		// Symbol with this name is already existing.
		PrEventHeader other_sym_origin;
		GetIntroOrigin(other_sym_origin, other_sym);
		wchar_t line_num_buff[40];
		wchar_t *fname = PrepareSrcLocInfo(other_sym_origin, line_num_buff, 40);

		if (other_sym < NonTerminalsBase)
			ReportError(ctx, err_code, L"The name \"%s\" is already registered as a terminal symbol of the grammar on the line %s of \"%s\".", name_buff, line_num_buff, fname);
		else ReportError(ctx, err_code, L"The name \"%s\" is already registered as a non terminal symbol of the grammar on the line %s of \"%s\".", name_buff, line_num_buff, fname);

		return(FALSE);
	}

	// Ensure that passed name is not an "rule app_id name".
	int irule = -1;
	for (int inx1=0; inx1<m_src_data.m_rules.NumItems(); ++inx1)
	{
		wchar_t *app_id_name = m_src_data.m_rules[inx1].rule_app_id_name;
		if (app_id_name != NULL && name_ptr == app_id_name)
		{
			irule = inx1;
			break;
		}
	}

	if (irule != -1)
	{
		wchar_t line_num_buff[40];
		ReportError(ctx, err_code, L"The name \"%s\" is already registered as the name of an application id of the rule on the line %s of \"%s\".",
					name_buff, line_num_buff, PrepareSrcLocInfo(m_src_data.m_rule_intro_areas[irule].rule_origin, line_num_buff, 40));
		return(FALSE);
	}

	// Ensure that passed name is not a conflict location.
	int iloc = -1;
	for (int inx2=0; inx2<m_src_data.m_location_objects.NumItems(); ++inx2)
	{
		if (name_ptr == m_src_data.m_location_objects[inx2].location_name)
		{
			iloc = inx2;
			break;
		}
	}

	if (iloc != -1)
	{
		wchar_t line_num_buff[40];
		ReportError(ctx, err_code, L"The name \"%s\" is already registered as an expected conflict location on the line %s of \"%s\".",
					name_buff, line_num_buff, PrepareSrcLocInfo(m_src_data.m_loc_object_intro_areas[iloc], line_num_buff, 40));
		return(FALSE);
	}

	// Ensure that passed name is not an "nested proc result app_id name".
	int inres = -1;
	for (int inx3=0; inx3<m_src_data.m_nested_processing_results.NumItems(); ++inx3)
	{
		wchar_t *app_id_name = m_src_data.m_nested_processing_results[inx3].nest_res_name;
		if (app_id_name != NULL && name_ptr == app_id_name)
		{
			inres = inx3;
			break;
		}
	}

	if (inres != -1)
	{
		wchar_t line_num_buff[40];
		ReportError(ctx, err_code, L"The name \"%s\" is already registered as a name of the nested processing result on the line %s of \"%s\".",
					name_buff, line_num_buff, PrepareSrcLocInfo(m_src_data.m_nested_proc_res_intro_areas[inres], line_num_buff, 40));
		return(FALSE);
	}

	// Ensure that passed name is not an expected conflict. Whole existing grammars hierarchy should be scanned.
	int rules_inx_base = RuleObjectsBase+m_src_data.m_rules.NumItems();
	int xpct_conflicts_inx_base = XpctConflictsBase;
	int sym_dupe = CheckNameHier(name_ptr, rules_inx_base, xpct_conflicts_inx_base, m_xpcts_root);
	if (sym_dupe != 0)
	{
		PrEventHeader other_sym_origin;
		GetIntroOrigin(other_sym_origin, sym_dupe);
		wchar_t line_num_buff[40];
		ReportError(ctx, err_code, L"The name \"%s\" is already registered as an expected conflict name or a name of the rule app id on the line %s of \"%s\".",
					name_buff, line_num_buff, PrepareSrcLocInfo(other_sym_origin, line_num_buff, 40));
		return(FALSE);
	}

	// Success. Name is fine and unique.
	return(TRUE);
}

WORD TGrammarDefinitionParser::CheckNameHier(TStrPtrInfo &name_ptr, int &rules_inx_base, int &xpct_conflicts_inx_base, TExpectedConflictSupp &ctx)
{
	// Search among local grammar rules.
	for (int irule=0; irule<ctx.m_resolution_rules.NumItems(); ++irule)
	{
		wchar_t *app_id_name = ctx.m_resolution_rules[irule].rule_app_id_name;
		if (app_id_name != NULL && name_ptr == app_id_name)
		{
			return((WORD)(rules_inx_base+irule));
		}
	}

	// Search among expected grammar conflicts.
	int num_local_xpcts = 0;
	for (TListIter<TExpectedConflictSupp> iter1(ctx.m_child_supps); iter1; ++iter1)
	{
		if (name_ptr == iter1.CurrItem().m_xpct_proto.xpct_conflict_name)
		{
			return((WORD)(xpct_conflicts_inx_base+num_local_xpcts));
		}

		num_local_xpcts++;
	}

	// Process the nested grammars if any.
	rules_inx_base += ctx.m_resolution_rules.NumItems();
	xpct_conflicts_inx_base += num_local_xpcts;
	for (TListIter<TExpectedConflictSupp> iter2(ctx.m_child_supps); iter2; ++iter2)
	{
		WORD res = CheckNameHier(name_ptr, rules_inx_base, xpct_conflicts_inx_base, iter2.CurrItem());
		if (res != 0)
		{
			return(res);
		}
	}

	// Passed name does not belong to the current subtree.
	return(0);
}

// /\/\/\/\/\/\/\/\  Grammar parsing methods  /\/\/\/\/\/\/\/\/\/\/\

void TGrammarDefinitionParser::DoTheScanning()
{
	//
	//        Major loop.
	//   ----------------------
	//
	//  The grammar definition does not have any beginning or end.
	//  It is simply a sequence of statements. Process these stamtements one by one.
	//

	TLexema lex;
	while (GetLexema(lex) != ltx_eof)
	{
		// Check for the beginning of the new section.
		if (lex.IsKeyword(opr_lt) == TRUE)
		{
			// This is a beginning of the section header.
			PrEventHeader sect_header_origin = lex.origin;
			TGrammarSectionType new_sect = grsc_none;
			if (GetLexema(lex, FALSE) == ltx_name)
			{
				// The name of the section is present. Check if this string belongs to the list of the section names or not.
				new_sect = GetGrammarDefnSectionType(lex.str_value);
				if (new_sect != grsc_none)
				{
					// The name of the section is known.
					if (GetLexema(lex, FALSE) == ltx_keyword && (short)lex.num_value == opr_gt)
					{
						// The section header terminator is present. Check if this section is good for the current context or not.
						if (m_cnt_nesting == 0)
						{
							// Current context is a root context.
							if (new_sect == grsc_locations || new_sect == grsc_resolution)
							{
								ReportError(lex, gerr_sect_hdr_error, L"Sections <locations> and <resolution> cannot be present in the root context.");
								ScanToSemicolon(lex);
								continue;
							}
						}
						else
						{
							// This is a nested context.
							if (new_sect != grsc_conflicts && new_sect != grsc_locations && new_sect != grsc_resolution)
							{
								ReportError(lex, gerr_sect_hdr_error, L"Nested contexts can contain only <locations>, <resolution> and <conflicts> sections.");
								ScanToSemicolon(lex);
								continue;
							}
						}

						// Accept the beginning of the new section.
						m_curr_section = new_sect;
						if (new_sect == grsc_rules)
							m_rule_sects_cnt++;

						// Report the area of the sect header to the parsing database.
						PrGrammarDefnSection info;
						sect_header_origin.ExtendPrHeader(lex.origin);
						info.hdr.Setup(&sect_header_origin, m_scanner->GetNextCN());
						info.section_type = new_sect;
						m_scanner->CbkHandler().GrammarSectionNotification(&info);
						m_scanner->PostProsessMidLevCbkCall(info.hdr);
						continue;
					}
					else
					{
						ReportError(lex, gerr_sect_hdr_error, L"The section header terminator is missing.");
					}
				}
				else
				{
					wchar_t loc_buff[80];
					ReportError(lex, gerr_sect_hdr_error, L"Incorrect name of the section: \"%s\".", lex.str_value.CopyWithTruncationTo(loc_buff, 80));
				}
			}
			else
			{
				ReportError(lex, gerr_sect_hdr_error, L"The name of the section is missing.");
			}

			// Scanning the section header has failed. Skip lexemas up to the beginning of the next statement.
			ScanToSemicolon(lex);
			continue;
		}

		if (lex.IsKeyword(spr_semicol) == TRUE)
		{
			// There are several semicolons in sequence or the grammar definition file starts with semicolon. Ignore this lexema.
			continue;
		}

		if (lex.IsKeyword(spr_rcurvbr) == TRUE)
		{
			//
			//  This is an end of the expected conflict definition.
			//

			// Check if this end is valid or not.
			if (m_cnt_nesting == 0)
			{
				// This is unpaired closing curved bracket.
				ReportError(lex, gerr_xpct_cfct_stmt_error, L"Bogus unpaired closing curved bractet - ignoring.");
				continue;
			}

			// Do the work.
			ExitExpectedConflictDefinition(lex);

			// Current lexema is processed.
			continue;
		}

		//
		// Current lexema should be a beginning of a statement.
		//

		if (lex.IsName(L"warnings") == TRUE || lex.IsName(L"__warnings") == TRUE)
		{
			ScanWarningsControlStatement();
			continue;
		}

		switch (m_curr_section)
		{
			case grsc_none:
					{
						// The type of the section is not defined yet.
						ReportError(lex, gerr_sect_not_defined, L"The grammar section is not declared yet.");
						ScanToSemicolon(lex);
					}
					break;

			case grsc_descr:
					ScanGrammarDescription(lex);
					break;

			case grsc_revision:
					ScanGrammarRevision(lex);
					break;

			case grsc_langtype:
					ScanLanguageTypeDefinition(lex);
					break;

			case grsc_symbols:
					ScanTerminalSymbolDefinition(lex);
					break;

			case grsc_ignore:
					ScanIgnoreLexTypeDefinition(lex);
					break;

			case grsc_errors:
					ScanErrorLexTypeDefinition(lex);
					break;

			case grsc_axioma:
					ScanAxiomaDefinition(lex);
					break;

			case grsc_rules:
					{
						// Rules section allows more than one type of the statement.
						if (lex.IsName(L"__public") == TRUE || lex.IsKeyword(cpp_public) == TRUE || lex.IsKeyword(csh_public) == TRUE)
						{
							ScanPublicNonTerminalsStatement();
						}
						else
						{
							ScanRuleDefinition(lex);
						}
					}
					break;

			case grsc_conflicts:
					{
						//
						//  This function will scan only the header of the expected conflict definition:
						//
						//		name  [ __id( number ) ]  {
						//
						//  In case of success it will switch into the nested context.
						//
						ScanExpectedConflictDefnHeader(lex);
					}
					break;

			case grsc_locations:
					ScanConflictLocationReferences(lex);
					break;

			case grsc_resolution:
					ScanConflictResolutionDefinition(lex);
					break;

			default:
				// Bogus current section value.
				assert(FALSE);
				break;
		}
	}

	if (m_cnt_nesting > 0)
	{
		// This is unpaired closing curved bracket.
		ReportError(lex, gerr_xpcts_not_closed, L"Expected conflict definitions (%d) are not closed.", m_cnt_nesting);
	}

	// File info can be NULL if the source file name is bogus.
	TFileInfoItem *src_info = m_scanner->MajorSrcInfo().mj_src_info;
	if (src_info == NULL)
	{
		// This case is considered as hard error.
		ReportError(lex, gerr_bld_tabs_skipped, L"Name or body of the major source file is missing.");
	}
	else
	{
		// Fill in the information about the major source file.
		wcscpy(m_src_props.grms_file_name, src_info->data.info.file_name);
		m_src_props.grms_file_length = src_info->data.info.file_len;
		m_src_props.grms_file_date = src_info->data.info.file_date;

		// Move non terminal forward declaration into the main list of non terminals if any.
		for (WORD ifwd=0; ifwd<m_src_data.m_fwd_def_non_terminals.NumItems(); ++ifwd)
		{
			if (m_src_data.m_fwd_def_non_terminals[ifwd].symbol_name != NULL)
			{
				// Current non terminal does not have rules. Do not issue an error now. It will be generated
				// later during the grammar integrity check.
				if (ConvertForwardDefIntoNonTerminal(lex.origin, NonTermForwardDefnsBase+ifwd, 0, gerr_err_proc_non_term_fwd) == 0)
					break;
			}
		}

		// Source file is processed. Apply the post scanning checks.
		if (m_scanner->GetCurrTraceMode() & gdtr_steps)
		{
			m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"----------------------- Integrity check -----------------------");
		}

		SourceGrammarIntegrityCheck(lex);

		TFileNameBuffer file_name_buffer;
		GenerateBasicReport(file_name_buffer, SOURCE_GRAMMAR_NAME, -1, m_src_data.m_symbols, m_src_data.m_non_terminals, m_src_data.m_rules);
	}
}

void TGrammarDefinitionParser::ScanWarningsControlStatement()
{
	//
	//  Warnings control statement should look like:
	//
	//    warnings <warn_flag_name> [ , <warn_flag_name> ... ] ;
	//    __warnings <warn_flag_name> [ , <warn_flag_name> ... ] ;
	//
	//  Possible flags:
	//
	//    ignore_unused_terminals	   --	   Block warning if terminal symbol is not used in any rule. Default value is TRUE.
	//	warn_on_rules_without_ids	   --	   Issue warning if rule has no app id. Default value is FALSE.
	//	ignore_unused_loc_markers	   --   Do not write warning if conflict location marker is not used in any xpct conflict.
	//

	// Loop on scanning the flag names.
	bool something_controlled = FALSE;
	TLexema lex;
	while (GetLexema(lex) != ltx_eof && lex.IsKeyword(spr_semicol) == FALSE)
	{
		if (lex.IsKeyword(opr_comma) == TRUE)
		{
			// The comma is optional. Ignore it.
			continue;
		}

		// Lexema should be the name of some warning.
		something_controlled = TRUE;

		if (lex.IsName(L"ignore_unused_terminals") == TRUE)
		{
			m_warn_on_unused_symbols = FALSE;
			continue;
		}
		else if (lex.IsName(L"warn_on_rules_without_ids") == TRUE)
		{
			m_warn_on_rules_without_ids = TRUE;
			continue;
		}
		else if (lex.IsName(L"ignore_unused_loc_markers") == TRUE)
		{
			m_warn_on_unused_loc_markers = FALSE;
			continue;
		}

		// Lexema was not recognized.
		ReportError(lex, gerr_warn_ctrl_stmt_error, L"Bogus lexema in the list of names of warnings.");
	}

	if (something_controlled == FALSE)
	{
		ReportError(lex, gerr_warn_ctrl_stmt_error, L"Warning names are missing.");
	}
}

void TGrammarDefinitionParser::ScanGrammarDescription(TLexema &lex_beg)
{
	//
	//  Grammar description should look like:
	//
	//    "<description_string>" ;
	//

	// Check if description is present and if is it fine or not.
	if (lex_beg.type != ltx_string)
	{
		ReportError(lex_beg, gerr_grm_descr_stmt_error, L"Grammar description should be a double quoted string lexema.");
		ScanToSemicolon(lex_beg);
		return;
	}
	else if (lex_beg.str_value.m_len >= TGrammarProps::MAX_DESCRIPTION_LEN)
	{
		ReportError(lex_beg, gerr_grm_descr_stmt_error, L"The grammar description is too long (%d). The max allowed length is %d.", lex_beg.str_value.m_len, TGrammarProps::MAX_DESCRIPTION_LEN-1);
		ScanToSemicolon(lex_beg);
		return;
	}

	// Save the description.
	lex_beg.str_value.CopyToVerifiedBuffer(m_src_props.grammar_description);

	// Check that semicolon is present at the end of the stmt.
	GetLexema((lex_beg));
	if (lex_beg.IsKeyword(spr_semicol) != TRUE)
	{
		ReportError(lex_beg, gerr_grm_descr_stmt_error, L"The grammar description is not followed by the semicolon.");
		ScanToSemicolon(lex_beg);
		return;
	}
}

void TGrammarDefinitionParser::ScanGrammarRevision(TLexema &lex_beg)
{
	//
	//  Grammar revision should look like:
	//
	//    <number> ;
	//

	// Check if revision is present and if is it fine or not.
	if (lex_beg.type != ltx_number)
	{
		ReportError(lex_beg, gerr_grm_rev_stmt_error, L"The grammar revision should be a numeric value.");
		ScanToSemicolon(lex_beg);
		return;
	}
	else if (lex_beg.num_value < 0)
	{
		ReportError(lex_beg, gerr_grm_rev_stmt_error, L"The grammar revision cannot be negative.");
		ScanToSemicolon(lex_beg);
		return;
	}
	else if (lex_beg.num_value >= 0x100000000)
	{
		ReportError(lex_beg, gerr_grm_rev_stmt_error, L"The grammar revision is too big. The value should fit into the 4 byte integer.");
		ScanToSemicolon(lex_beg);
		return;
	}

	// Grammar revision is present.
	m_src_props.grammar_revision = (DWORD)lex_beg.num_value;

	// Check that semicolon is present at the end of the stmt.
	GetLexema((lex_beg));
	if (lex_beg.IsKeyword(spr_semicol) != TRUE)
	{
		ReportError(lex_beg, gerr_grm_rev_stmt_error, L"The grammar revision is not followed by the semicolon.");
		ScanToSemicolon(lex_beg);
		return;
	}
}

void TGrammarDefinitionParser::ScanLanguageTypeDefinition(TLexema &lex_beg)
{
	//
	//  High level language type definition should look like:
	//
	//    <lang_type_name> [ [ , ] <cbk_handler_name> [ [ , ] <preferred_source_emitting_prefix> ] ] ;
	//
	//  All three parameters should be either names or strings.
	//

	// Language type definition should consist of only one lexema.
	if (lex_beg.type != ltx_name && lex_beg.type != ltx_string)
	{
		ReportError(lex_beg, gerr_lang_type_stmt_error, L"The name of the target language type is missing.");
		ScanToSemicolon(lex_beg);
		return;
	}

	TLexema lang_type_lex = lex_beg;

	// Pick up the next lexema.
	GetLexema(lex_beg);

	// Try to assemble name of the lang type from several lexemas.
	while (lang_type_lex.type == ltx_name && (lex_beg.IsKeyword(opr_minus) || lex_beg.type == ltx_name))
	{
		// Check the next lexema.
		if (lang_type_lex.origin.source_ref != lex_beg.origin.source_ref || lang_type_lex.origin.src_area.AreaEnd() != lex_beg.origin.src_area.AreaBeg())
		{
			// There is something between these lexemas. Break the sequence.
			break;
		}

		// Latest lexema continues the seq of initial lexema.
		lang_type_lex.str_value.m_len += lex_beg.origin.src_area.area_len;
		lang_type_lex.origin.src_area.ExtendEndingTo(lex_beg.origin.src_area);
		GetLexema(lex_beg);
	}

	// Convert sting value of the lexema into the NULL terminated string.
	if (lang_type_lex.str_value.m_len > 2*TGrammar::MAX_NAME_LENGTH)
	{
		ReportError(lang_type_lex, gerr_lang_type_stmt_error, L"The name of the target language type is too long. The statement is ignored.");
		ScanToSemicolon(lex_beg);
		return;
	}

	wchar_t lang_type_lex_str_val[2*TGrammar::MAX_NAME_LENGTH+1];
	lang_type_lex.str_value.CopyToVerifiedBuffer(lang_type_lex_str_val);

	// Check if string is a known language name or not.
	THighLevelLangType langt = THighLevelDatabase::GetLangTypeFromString(lang_type_lex_str_val);
	if (langt == lang_num_types)
	{
		ReportError(lang_type_lex, gerr_lang_type_stmt_error, L"The name of the target language is unknown. The statement is ignored.");
		ScanToSemicolon(lex_beg);
		return;
	}
	else if (langt == lang_none)
	{
		ReportError(lang_type_lex, gerr_lang_type_stmt_error, L"The name of the target language cannot be \"None\" or \"RawInxSess\". The statement is ignored.");
		ScanToSemicolon(lex_beg);
		return;
	}

	// Store the language type.
	m_src_props.lang_type = langt;

	// Check lexema after the language type name.
	if (lex_beg.IsKeyword(opr_comma) == TRUE)
		GetLexema(lex_beg);

	if (lex_beg.type == ltx_name || lex_beg.type == ltx_string)
	{
		// Name of the callback handler is present.
		if (lex_beg.str_value.m_len >= TGrammarProps::MAX_CBK_NAME_LEN)
		{
			ReportError(lex_beg, gerr_lang_type_stmt_error, L"The name of the callback handler is too long. The max allowed length is %d.", TGrammarProps::MAX_CBK_NAME_LEN-1);
			ScanToSemicolon(lex_beg);
			return;
		}

		// Store the cbk handler name.
		lex_beg.str_value.CopyToVerifiedBuffer(m_src_props.cbk_handler_name);

		// Pick up the next lexema.
		GetLexema(lex_beg);
		if (lex_beg.IsKeyword(opr_comma) == TRUE)
			GetLexema(lex_beg);

		if (lex_beg.type == ltx_name || lex_beg.type == ltx_string)
		{
			// Preferred prefix is present.
			if (lex_beg.str_value.m_len >= TGrammarProps::MAX_PREFERRED_PREFIX_LEN)
			{
				ReportError(lex_beg, gerr_lang_type_stmt_error, L"The preferred emitting prefix is too long The max allowed length is %d.", TGrammarProps::MAX_PREFERRED_PREFIX_LEN-1);
				ScanToSemicolon(lex_beg);
				return;
			}

			// Store the prefix and pick up the next lexema.
			lex_beg.str_value.CopyToVerifiedBuffer(m_src_props.preferred_emitting_prefix);
			GetLexema(lex_beg);
		}
	}

	// Check that semicolon is present at the end of the stmt.
	if (lex_beg.IsKeyword(spr_semicol) != TRUE)
	{
		ReportError(lex_beg, gerr_lang_type_stmt_error, L"The target language description statement is not terminated with the semicolon.");
		ScanToSemicolon(lex_beg);
		return;
	}
}

void TGrammarDefinitionParser::ScanTerminalSymbolDefinition(TLexema &lex_beg)
{
	//
	//  Terminal symbol definition should look like:
	//
	//    <charconst>	: <lex_type>					[ __id(<number>) ] ;
	//    <name>		: <lex_type>					[ __id(<number>) ] ;
	//    <charconst>	: <lex_type> <lex_subtype>		[ __id(<number>) ] ;
	//    <name>		: <lex_type> <lex_subtype>		[ __id(<number>) ] ;
	//
	//  NB: Terminal symbols should be defined before using them in the rules.
	//

	// Check if passed lexema can be used as name for the terminal symbol or not.
	if (lex_beg.type == ltx_name)
	{
		// Laxema is a name. Ensure that name is not too long and that it is unique.
		if (CheckName(lex_beg.origin, lex_beg.str_value, gerr_symbol_def_stmt_error) == FALSE)
		{
			// Error is already reported.
			ScanToSemicolon(lex_beg);
			return;
		}
	}
	else if (lex_beg.type == ltx_charconst)
	{
		// Lexema is a charconst. Check that other terminal with this name is not defined.
		WORD other_sym = TGrammarSymbolsHelper::FindSymbol(m_src_data.m_symbols, m_src_data.m_non_terminals, lex_beg);
		if (other_sym != 0)
		{
			PrEventHeader other_sym_origin;
			GetIntroOrigin(other_sym_origin, other_sym);
			wchar_t line_num_buff[40];
			ReportError(lex_beg, gerr_symbol_def_stmt_error, L"Symbol with this name is already defined on line %s of \"%s\".",
						line_num_buff, PrepareSrcLocInfo(other_sym_origin, line_num_buff, 40));
			ScanToSemicolon(lex_beg);
			return;
		}
	}
	else
	{
		ReportError(lex_beg, gerr_symbol_def_stmt_error, L"Terminal symbol name is missing.");
		ScanToSemicolon(lex_beg);
		return;
	}

	// Verify the delimiter after the name.
	TLexema lex;
	if (GetLexema(lex) != ltx_keyword || (short)lex.num_value != opr_colon)
	{
		ReportError(lex, gerr_symbol_def_stmt_error, L"A colon after the terminal symbol name is missing.");
		ScanToSemicolon(lex);
		return;
	}

	// Scan lexema type and optional subtype.
	TLexemaType lex_type;
	bool check_subt;
	TLexSubtype lex_subt;
	lex.type = ltx_empty;
	if (ScanLexTypeAndSubtype(lex, lex_type, check_subt, lex_subt, gerr_symbol_def_stmt_error) == FALSE)
	{
		// Error should be already reported.
		return;
	}

	// Check that processing for this lex type/subtype is not yet defined.
	PrEventHeader conflict_origin;
	bool lext_unmapped = CheckLexTypeMapping(lex_type, check_subt, lex_subt, conflict_origin);
	if (lext_unmapped == FALSE)
	{
		wchar_t line_num_buff[40];
		ReportError(lex, gerr_symbol_def_stmt_error, L"Processing for this lexema type is already defined on the line %s of \"%s\".",
					line_num_buff, PrepareSrcLocInfo(conflict_origin, line_num_buff, 40));
		ScanToSemicolon(lex);
		return;
	}

	// Check if application id of the terminal symbol is present or not.
	WORD app_id = 0;
	if (lex.IsName(L"__id") == TRUE)
	{
		// The id header is present. Scan the body of the specifier.
		if (ScanObjectId(lex, app_id, NULL, 0, gerr_symbol_def_stmt_error) == FALSE)
		{
			// All problems including the app symbol duplication are reported.
			return;
		}

		// Pick up the next lexema.
		GetLexema(lex);
	}

	if (lex_beg.type == ltx_name)
	{
		// Check for implicit app_id that may be present in the macro dictionary.
		if (GetObjectIdFromMacroDefnsTable(lex, L"sym_", lex_beg.str_value, app_id, gerr_symbol_def_stmt_error) == FALSE)
		{
			// Error is already reported.
			return;
		}
	}

	// Verify presence of the trailing delimiter.
	if (lex.IsKeyword(spr_semicol) == FALSE)
	{
		ReportError(lex, gerr_symbol_def_stmt_error, L"Semicolon at the end of the statement is missing.");
		ScanToSemicolon(lex);
		return;
	}

	//
	// All parts of the statement were successfully scanned and verified.
	//

	// Save the intro area.
	if (m_src_data.m_symbol_areas.AppendItem(lex_beg.origin) == FALSE)
	{
		ReportError(lex, gerr_symbol_def_stmt_error, L"Out of memory while saving teminal symbol intro area.");
		return;
	}

	// Add symbol to the grammar.
	ID event_id = m_scanner->GetNextCN();
	WORD new_sym = TGrammarSymbolsHelper::AddTerminalSymbol(m_src_data.m_symbols, lex_beg.type == ltx_name, lex_beg.num_value, lex_beg.str_value, app_id, lex_type, check_subt, lex_subt, event_id);
	if (new_sym == 0)
	{
		ReportError(lex, gerr_symbol_def_stmt_error, L"Out of memory while adding teminal symbol definition to the grammar.");
		m_src_data.m_symbol_areas.ReleaseLastItem();
		return;
	}

	// Report terminal symbol area to the parsing database.
	PrGrammarDefnObject info;
	info.hdr.Setup(&lex_beg.origin, event_id);
	info.object_sym = TerminalSymbolsBase+m_src_data.m_symbols.NumItems()-1;
	info.object_app_id = app_id;
	info.extra_context_area.Clear();
	m_scanner->CbkHandler().GrammarObjectNotification(&info);
	m_scanner->PostProsessMidLevCbkCall(info.hdr);

	// Full success.
	assert(m_src_data.m_symbol_areas.NumItems() == m_src_data.m_symbols.NumItems());
}

void TGrammarDefinitionParser::ScanIgnoreLexTypeDefinition(TLexema &lex_beg)
{
	//
	//  Ignore lex type definition should look like:
	//
	//    <lex_type> ;
	//    <lex_type> <lex_subtype> ;
	//

	// Scan lexema type and optional subtype.
	TLexemaType lex_type;
	bool check_subt;
	TLexSubtype lex_subt;
	if (ScanLexTypeAndSubtype(lex_beg, lex_type, check_subt, lex_subt, gerr_ignore_lex_stmt_error) == FALSE)
	{
		// Error is already reported.
		return;
	}

	// Check that processing for this lex type is not yet defined.
	PrEventHeader conflict_origin;
	bool lext_unmapped = CheckLexTypeMapping(lex_type, check_subt, lex_subt, conflict_origin);
	if (lext_unmapped == FALSE)
	{
		wchar_t line_num_buff[40];
		ReportError(lex_beg, gerr_ignore_lex_stmt_error, L"Processing for this lexema type is already defined on line %s of \"%s\".",
					line_num_buff, PrepareSrcLocInfo(conflict_origin, line_num_buff, 40));
		ScanToSemicolon(lex_beg);
		return;
	}

	// Verify presence of the trailing delimiter.
	if (lex_beg.IsKeyword(spr_semicol) == FALSE)
	{
		ReportError(lex_beg, gerr_ignore_lex_stmt_error, L"Semicolon at the end of the statement is missing.");
		ScanToSemicolon(lex_beg);
		return;
	}

	//
	// All parts of the statement were successfully scanned and verified.
	//

	// Save intro area locally.
	if (m_src_data.m_ignore_lex_areas.AppendItem(lex_beg.origin) == FALSE)
	{
		ReportError(lex_beg, gerr_ignore_lex_stmt_error, L"Out of memory while saving ignore lex intro area.");
		return;
	}

	// Add record to the grammar.
	if (TGrammarSymbolsHelper::AddIgnoreLexRecord(m_src_data.m_ignore_lex, lex_type, check_subt, lex_subt) == FALSE)
	{
		ReportError(lex_beg, gerr_ignore_lex_stmt_error, L"Out of memory while adding ignore lex type record to the grammar.");
		m_src_data.m_ignore_lex_areas.ReleaseLastItem();
		return;
	}

	// Full success.
	assert(m_src_data.m_ignore_lex_areas.NumItems() == m_src_data.m_ignore_lex.NumItems());
}

void TGrammarDefinitionParser::ScanErrorLexTypeDefinition(TLexema &lex_beg)
{
	//
	//  Error lex type definition should look like:
	//
	//    <lex_type> : "message" ;
	//    <lex_type> <lex_subtype> : "message" ;
	//

	// Scan lexema type and optional subtype.
	TLexemaType lex_type;
	bool check_subt;
	TLexSubtype lex_subt;
	if (ScanLexTypeAndSubtype(lex_beg, lex_type, check_subt, lex_subt, gerr_error_lex_stmt_error) == FALSE)
	{
		// Error is already reported.
		return;
	}

	// Check that processing for this lex type is not yet defined.
	PrEventHeader conflict_origin;
	bool lext_unmapped = CheckLexTypeMapping(lex_type, check_subt, lex_subt, conflict_origin);
	if (lext_unmapped == FALSE)
	{
		wchar_t line_num_buff[40];
		ReportError(lex_beg, gerr_error_lex_stmt_error, L"Processing for this lexema type is already defined on line %s of \"%s\".",
					line_num_buff, PrepareSrcLocInfo(conflict_origin, line_num_buff, 40));
		ScanToSemicolon(lex_beg);
		return;
	}

	// Verify delimiter after the lex type and optional subtyte.
	if (lex_beg.IsKeyword(opr_colon) == FALSE)
	{
		ReportError(lex_beg, gerr_error_lex_stmt_error, L"Colon after the lex type/subtype is missing.");
		ScanToSemicolon(lex_beg);
		return;
	}

	// Pick up the error message.
	TLexema lex_msg;
	if (GetLexema(lex_msg) != ltx_string || lex_msg.str_value.m_len == 0)
	{
		ReportError(lex_msg, gerr_error_lex_stmt_error, L"Error message for the lexex type/subtype is missing.");
		ScanToSemicolon(lex_msg);
		return;
	}

	// Verify delimiter at the end of the statement.
	if (GetLexema(lex_beg) != ltx_keyword || (short)lex_beg.num_value != spr_semicol)
	{
		ReportError(lex_beg, gerr_error_lex_stmt_error, L"Semicolon at the end of the statement is missing.");
		ScanToSemicolon(lex_beg);
		return;
	}

	//
	// All parts of the statement were successfully scanned and verified.
	//

	// Save the intro area locally.
	if (m_src_data.m_error_lex_areas.AppendItem(lex_beg.origin) == FALSE)
	{
		ReportError(lex_beg, gerr_error_lex_stmt_error, L"Out of memory while saving the error lex intro area.");
		return;
	}

	// Add new record to the grammar.
	if (TGrammarSymbolsHelper::AddErrorLexRecord(m_src_data.m_error_lex, lex_type, check_subt, lex_subt, lex_msg.str_value) == FALSE)
	{
		ReportError(lex_beg, gerr_error_lex_stmt_error, L"Out of memory while adding the error lex type record to the grammar.");
		m_src_data.m_error_lex_areas.ReleaseLastItem();
		return;
	}

	// Full success.
	assert(m_src_data.m_error_lex_areas.NumItems() == m_src_data.m_error_lex.NumItems());
}

void TGrammarDefinitionParser::ScanAxiomaDefinition(TLexema &lex_beg)
{
	//
	//  Axioma definition should look like:
	//
	//    <name> ;
	//

	// Axioma body should consist of only one lexema.
	if (lex_beg.type != ltx_name)
	{
		ReportError(lex_beg, gerr_axioma_stmt_error, L"Non terminal symbol name is missing.");
		ScanToSemicolon(lex_beg);
		return;
	}

	// Find or register this name as non terminal.
	WORD sym = m_src_data.FindSymbol(lex_beg);
	if (sym == 0)
	{
		// This name is not a grammar symbol. Register this symbol as non terminal. If this name is already
		// used for some other object type, registration function will discover this and reject the registration.
		sym = RegisterNonTerminal(lex_beg, FALSE, gerr_axioma_stmt_error);
		if (sym == 0)
		{
			// Registration failed. Error is already reported.
			ScanToSemicolon(lex_beg);
			return;
		}
	}
	else if (sym < NonTerminalsBase)
	{
		// Terminal symbol cannot be used as axioma.
		PrEventHeader origin_loc;
		GetIntroOrigin(origin_loc, sym);
		wchar_t symbol_name_buff[40], line_num_buff[40];
		ReportError(lex_beg, gerr_axioma_stmt_error, L"Name \"%s\" cannot be used as axioma because it is already defined as terminal symbol on line %s of \"%s\".",
					m_src_data.GetSymbolName(sym, symbol_name_buff), line_num_buff, PrepareSrcLocInfo(origin_loc, line_num_buff, 40));
		ScanToSemicolon(lex_beg);
		return;
	}

	// Ensure that axioma is not defined yet.
	if (m_axioma != 0)
	{
		PrEventHeader origin_loc;
		GetIntroOrigin(origin_loc, m_axioma);
		wchar_t line_num_buff[40];
		ReportError(lex_beg, gerr_axioma_stmt_error, L"The axioma is already defined on the line %s of \"%s\".",
					line_num_buff, PrepareSrcLocInfo(origin_loc, line_num_buff, 40));
		ScanToSemicolon(lex_beg);
		return;
	}

	// Ensure that semicolon is present after the axioma name.
	if (GetLexema(lex_beg) != ltx_keyword || (short)lex_beg.num_value != spr_semicol)
	{
		ReportError(lex_beg, gerr_axioma_stmt_error, L"The name of the axioma is not followed by the semicolon.");
		ScanToSemicolon(lex_beg);
		return;
	}

	// Full success. Assign the axioma id.
	m_axioma = sym;
}

void TGrammarDefinitionParser::ScanRuleDefinition(TLexema &lex_beg)
{
	//
	//  Rule definition should look like:
	//
	//    <non_term> [ <non_term_attr> ... ]
	//          : [ <conflict_location_marker> ... ] <term_or_non_term> [ [ <conflict_location_marker> ... ] <term_or_non_term> ... ] [ <conflict_location_marker> ... ] [ <rule_attr> ... ]
	//          ;
	//
	//    <non_term> [ <non_term_attr> ... ]
	//          | [ <conflict_location_marker> ... ] <term_or_non_term> [ [ <conflict_location_marker> ... ] <term_or_non_term> ... ] [ <conflict_location_marker> ... ] [ <rule_attr> ... ]
	//          | [ <conflict_location_marker> ... ] <term_or_non_term> [ [ <conflict_location_marker> ... ] <term_or_non_term> ... ] [ <conflict_location_marker> ... ] [ <rule_attr> ... ]
	//          | [ <conflict_location_marker> ... ] <term_or_non_term> [ [ <conflict_location_marker> ... ] <term_or_non_term> ... ] [ <conflict_location_marker> ... ] [ <rule_attr> ... ]
	//          ....
	//          ;
	//
	//   Non terminal attributes:
	//
	//          __seq
	//          __list
	//          __id( <number> )
	//
	//   Rule attributes:
	//
	//          __id( <number> )
	//
	//   Conflict location marker:
	//
	//          [ <name>[*] [ , <name>[*] ... ] ]
	//          [ <name>-<terminal_symbol>[*] [ , <name>-<terminal_symbol>[*] ... ] ]
	//
	//   Note:	Numeric app ids of symbols, non terminals, rules, nested results and expected grammar conflicts
	//			should be unique across all nested grammars because they share the same space of ids.
	//
	//   Note:	Terminal symbols should be defined ahead. Unknown charconst (i.e. something like 'xyz') will generate
	//			syntax error. Unregistered name will be registered as a new non terminal.
	//
	//   Note:	When name of the conflict location marker or terminal symbol in the conflict location marker is followed
	//			by an asterisk (*), this means that this location may contain only some of the required parsing states.
	//			In other words it should not be used while building the parsing states intersection.
	//

	// Rule should begin with the name of the non terminal.
	if (lex_beg.type != ltx_name)
	{
		ReportError(lex_beg, gerr_rule_stmt_error, L"Name of the rule non terminal is missing.");
		ScanToSemicolon(lex_beg);
		return;
	}

	// Find or register name at the left hand side of the rule.
	WORD non_term = m_src_data.FindSymbol(lex_beg);
	if (non_term == 0)
	{
		// The name is unknown as a symbol of the source data. If this name is already used for object
		// of some other type, the registration function will discover this and reject the registration.
		non_term = RegisterNonTerminal(lex_beg, TRUE, gerr_rule_stmt_error);
		if (non_term == 0)
		{
			// Registration failed. Error is already reported.
			ScanToSemicolon(lex_beg);
			return;
		}
	}
	else if (non_term < NonTerminalsBase)
	{
		// The name is a terminal symbol.
		ReportError(lex_beg, gerr_rule_stmt_error, L"Name of the terminal symbol cannot be used in the left hand side of the rule.");
		ScanToSemicolon(lex_beg);
		return;
	}
	else if (non_term >= NonTermForwardDefnsBase)
	{
		// This name is already known as a forward declaration. Convert this forward declaration into the regular non term.
		non_term = ConvertForwardDefIntoNonTerminal(lex_beg.origin, non_term, m_rule_sects_cnt, gerr_rule_stmt_error);
		if (non_term == 0)
		{
			// Error is already reported.
			ScanToSemicolon(lex_beg);
			return;
		}
	}
	else
	{
		// The name is already known and it is some non terminal. Check the rule section in the description of this non terminal.
		if (m_src_data.m_non_terminals[non_term-NonTerminalsBase].rules_sect != m_rule_sects_cnt)
		{
			wchar_t buff_40_chars[40];
			ReportError(lex_beg, gerr_rule_stmt_error, L"Non terminal \"%s\"(%hd) already has rules in the other rules section.", m_src_data.GetSymbolName(non_term, buff_40_chars), non_term);
			ScanToSemicolon(lex_beg);
			return;
		}
	}

	// Non terminal is already existing or it was just registered. Pick up its description.
	PrEventHeader rule_non_term_origin = lex_beg.origin;
	TNonTerminalSymbol &non_term_info = m_src_data.m_non_terminals[non_term-NonTerminalsBase];

	if (non_term != m_latest_rule_non_term)
	{
		// This rule defines different non terminal than the prev rule.
		if (m_latest_rule_non_term != 0xFFFF)
		{
			// Mark prev non term definitions as complete.
			m_src_data.m_non_term_defns_complete[m_latest_rule_non_term-NonTerminalsBase] = TRUE;
		}

		if (m_src_data.m_non_term_defns_complete[non_term-NonTerminalsBase] == TRUE)
		{
			// Issue warning.
			wchar_t buff_40_chars[40];
			ReportWarning(lex_beg, gerr_rule_stmt_error, L"Rules for the non terminal \"%s\"(%hd) are not consecutive.", m_src_data.GetSymbolName(non_term, buff_40_chars), non_term);
		}

		m_latest_rule_non_term = non_term;
	}

	// Scan the non terminal attributes.
	TLexema lex;
	while (GetLexema(lex) != ltx_eof && lex.IsKeyword(spr_semicol) == FALSE && lex.IsKeyword(opr_colon) == FALSE && lex.IsKeyword(opr_bitor) == FALSE)
	{
		if (lex.IsKeyword(opr_comma) == TRUE)
			continue;

		// Check for possible non terminal modifiers.
		if (lex.IsName(L"__seq") == TRUE)
		{
			// Ensure that other restriction modifiers are not present.
			if (non_term_info.restr_type == ntrs_list)
			{
				ReportError(lex, gerr_rule_stmt_error, L"The non terminal restrictions \"__seq\" and \"__list\" cannot be combined.");
				ScanToSemicolon(lex);
				return;
			}

			non_term_info.restr_type = ntrs_seq;
		}
		else if (lex.IsName(L"__list") == TRUE)
		{
			// Ensure that other restriction modifiers are not present.
			if (non_term_info.restr_type == ntrs_seq)
			{
				ReportError(lex, gerr_rule_stmt_error, L"The non terminal restrictions \"__list\" and \"__seq\" cannot be combined.");
				ScanToSemicolon(lex);
				return;
			}

			non_term_info.restr_type = ntrs_list;
		}
		else if (lex.IsName(L"__id") == TRUE)
		{
			// Scan the identifier value. Note, this is an id of the non terminal, not an id of the rule.
			WORD explicit_app_id = 0;
			if (ScanObjectId(lex, explicit_app_id, NULL, non_term, gerr_rule_stmt_error) == FALSE)
			{
				// Errors are already reported.
				ScanToSemicolon(lex);
				return;
			}

			if (explicit_app_id != 0)
			{
				// Compare the scanned id with the id of the existing definition if any.
				if (non_term_info.app_id != 0 && non_term_info.app_id != explicit_app_id)
				{
					ReportError(lex, gerr_rule_stmt_error,
								L"Other rule or implicit macrodef assigned a different app id (%hd) to the non terminal of the current rule. The rule is ignored.",
								non_term_info.app_id);
					ScanToSemicolon(lex);
					return;
				}

				// Save the app_id.
				non_term_info.app_id = explicit_app_id;
			}
		}
		else
		{
			// Unknown non terminal modifier.
			ReportError(lex, gerr_rule_stmt_error, L"Unknown non terminal modifier.");
			ScanToSemicolon(lex);
			return;
		}
	}

	// Check if current non terminal definition is preliminary or not.
	if (non_term_info.event_id == 0)
	{
		// Fix the definition location.
		m_src_data.m_non_term_areas[non_term-NonTerminalsBase] = lex_beg.origin;

		// Report non terminal area to the parsing database.
		PrGrammarDefnObject info;
		info.hdr.Setup(&lex_beg.origin, m_scanner->GetNextCN());
		info.object_sym = non_term;
		info.object_app_id = non_term_info.app_id;
		info.extra_context_area.Clear();
		m_scanner->CbkHandler().GrammarObjectNotification(&info);
		m_scanner->PostProsessMidLevCbkCall(info.hdr);

		// Save the symbol intro event id.
		non_term_info.event_id  = info.hdr.cn;
	}

	// Ensure that next lexema is either colon or bit OR operation.
	if (lex.IsKeyword(opr_colon) == FALSE && lex.IsKeyword(opr_bitor) == FALSE)
	{
		ReportError(lex, gerr_rule_stmt_error, L"Colon or bit OR symbol is missing after the non terminal name.");
		ScanToSemicolon(lex);
		return;
	}

	// Report the rule left hand side non terminal area.
	rule_non_term_origin.cn = m_scanner->GetNextCN();
	{
		PrGrammarDefnObject info;
		info.hdr.Setup(&rule_non_term_origin, rule_non_term_origin.cn);
		info.object_sym = RuleNonTermObject+(non_term-NonTerminalsBase);
		info.object_app_id = non_term_info.app_id;
		info.extra_context_area.Clear();
		m_scanner->CbkHandler().GrammarObjectNotification(&info);
		m_scanner->PostProsessMidLevCbkCall(info.hdr);
	}

	// Variables for accumulating info about the source location of the objects that belong to the current rule.
	WORD rule_app_id = 0;
	wchar_t rule_app_id_name[TGrammar::MAX_NAME_LENGTH+1];
	PrEventHeader rule_intro_area = lex.origin;
	PrEventHeadersArray rule_symbol_areas;
	TConflictLocationMarkerInfosArray location_markers;
	rule_app_id_name[0] = 0;

	//
	//    Loop over the right hand side rule symbols.
	// --------------------------------------------
	//
	m_sym_buffer.Clear();
	while (GetLexema(lex) != ltx_eof)
	{
		// Case 1. Possible delimiter.
		if (lex.IsKeyword(opr_comma) == TRUE)
		{
			// Ignore commas.
			continue;
		}

		// Case 2. Teminal symbol as character constant.
		else if (lex.type == ltx_charconst)
		{
			WORD sym = m_src_data.FindSymbol(lex);
			if (sym == 0)
			{
				ReportError(lex, gerr_rule_stmt_error, L"Undefined terminal symbol in the rule. The rule is ignored.");
				ScanToSemicolon(lex);
				return;
			}
			else
			{
				// Current charconst is the name of some terminal symbol.
				if (SaveRuleRightHandSideSymbol(sym, lex, rule_symbol_areas) == FALSE)
				{
					// Saving failed.
					ScanToSemicolon(lex);
					return;
				}
			}
		}

		// Case 3. Rule symbol or rule attribute.
		else if (lex.type == ltx_name)
		{
			// Check for rule id case first.
			if (lex.IsName(L"__id") == TRUE)
			{
				// Scan the identifier value.
				if (ScanObjectId(lex, rule_app_id, rule_app_id_name, 0, gerr_rule_stmt_error) == FALSE)
				{
					// Errors are already reported.
					ScanToSemicolon(lex);
					return;
				}

				continue;
			}

			// This name should be either the name of the terminal or the name of the non terminal.
			WORD sym = m_src_data.FindSymbol(lex);
			if (sym == 0)
			{
				// The name is unknown. Register it.
				sym = RegisterNonTerminal(lex, FALSE, gerr_rule_stmt_error);
				if (sym == 0)
				{
					// Registration failed. Error is already reported.
					ScanToSemicolon(lex);
					return;
				}
			}

			// Current name is the name of some terminal or non terminal symbol.
			if (SaveRuleRightHandSideSymbol(sym, lex, rule_symbol_areas) == FALSE)
			{
				// Saving failed.
				ScanToSemicolon(lex);
				return;
			}
		}

		// Case 4. Location marker.
		else if (lex.IsKeyword(opr_lbracket) == TRUE)
		{
			//
			//  This is opening bracket of the expected conflict location marker.
			//
			bool right_bracket_present = FALSE;
			bool symbol_retrieved = FALSE;
			while (symbol_retrieved == TRUE || GetLexema(lex) != ltx_eof)
			{
				symbol_retrieved = FALSE;
				if (lex.IsKeyword(spr_semicol) == TRUE)
				{
					// End of the rule statement.
					ReportError(lex, gerr_rule_stmt_error, L"Conflict location marker was not properly terminated.");
					break;
				}
				else if (lex.IsKeyword(opr_rbracket) == TRUE)
				{
					// End of the marker.
					right_bracket_present = TRUE;
					break;
				}
				else if (lex.IsKeyword(opr_comma) == TRUE)
				{
					// Ignore this symbol.
					continue;
				}
				else if (lex.type == ltx_name)
				{
					// Name of the conflict location is present. Note that the mark field of the position field is inited with zero.
					// This means that location is an ordinary location by default. Later on this field can be changed to TRUE.
					TConflictLocationMarkerInfo loc_marker = { { m_src_data.m_rules.NumItems(), m_sym_buffer.NumItems(), 0xFFFF, 0 }, -1, NULL, lex.origin };
					bool loc_marker_success = FALSE;

					// Check if name of the location is already known or not.
					loc_marker.loc_index = TGrammarSymbolsHelper::FindLocationObject(m_src_data.m_location_objects, lex.str_value);
					if (loc_marker.loc_index == -1)
					{
						// Location object with this name is not existing. If the retrieved name is not good for
						// any reason the CheckName() function will issue at least one syntax error.
						if (CheckName(lex.origin, lex.str_value, gerr_rule_stmt_error) == TRUE)
						{
							// This is a correct unused name.
							loc_marker.loc_name = TGrammar::StrDupe(lex.str_value);
							if (loc_marker.loc_name == NULL)
							{
								ReportError(lex, gerr_rule_stmt_error, L"Out of memory while storing conflict location name.");
							}
							else
							{
								loc_marker_success = TRUE;
							}
						}
					}
					else
					{
						// Conflict location object with this name is already existing and index of this object
						// is already set into the info structure. The name pointer should be NULL in this case.
						loc_marker_success = TRUE;
					}

					// Check if an action symbol is present after the name of the marker or not.
					GetLexema(lex);
					if (lex.IsKeyword(opr_minus) == TRUE)
					{
						// Delimiter is present. Check the next symbol.
						GetLexema(lex);
						if (lex.type == ltx_charconst || lex.type == ltx_name)
						{
							// This can be the name of the grammar symbol.
							if (lex.type == ltx_name && (lex.str_value == L"EOF" || lex.str_value == L"Eof" || lex.str_value == L"eof"))
							{
								// This is special case because FindSymbol() is not returning description of the EOF symbol.
								loc_marker.position.action_sym = 0;
							}
							else
							{
								WORD sym = m_src_data.FindSymbol(lex);
								if (sym == 0)
								{
									ReportError(lex, gerr_rule_stmt_error, L"Action symbol does not define any terminal symbol of the grammar.");
									loc_marker_success = FALSE;
								}
								else if (sym >= NonTerminalsBase)
								{
									ReportError(lex, gerr_rule_stmt_error, L"Action symbol of the conflict location marker cannot be a non terminal.");
									loc_marker_success = FALSE;
								}
								else
								{
									// Valid action symbol is present.
									loc_marker.position.action_sym = sym;
								}
							}

							if (loc_marker_success == TRUE)
							{
								// Check for the asterisk after the action symbol.
								GetLexema(lex);
								if (lex.IsKeyword(opr_mul) == TRUE)
									loc_marker.position.mark = TRUE;
								else symbol_retrieved = TRUE;
							}
						}
						else
						{
							// The symbol after the delimiter is bogus.
							ReportError(lex, gerr_rule_stmt_error, L"Action symbol is missing in the conflict location marker.");
							loc_marker_success = FALSE;
							if (lex.IsKeyword(spr_semicol) == TRUE || lex.IsKeyword(opr_rbracket) == TRUE)
								symbol_retrieved = TRUE;
						}
					}
					else if (lex.IsKeyword(opr_mul) == TRUE)
					{
						// This is pseudo location mark. It will not be used for determining the set of the parsing states.
						loc_marker.position.mark = TRUE;
					}
					else
					{
						// Send this symbol to the outer loop for processing.
						symbol_retrieved = TRUE;
					}

					if (loc_marker_success == TRUE)
					{
						// The marker is good. Add it to the array of the marker infos.
						if (location_markers.AppendItem(loc_marker) == FALSE)
						{
							ReportError(lex, gerr_rule_stmt_error, L"Out of memory while temporary storing conflict location marker.");
						}
					}

					// Clean the marker structure.
					loc_marker.ReleaseObject();
				}
				else
				{
					// The symbol after the left bracket is bogus. Report the error and continue.
					ReportError(lex, gerr_rule_stmt_error, L"Bogus symbol inside the conflict location marker.");
				}
			}

			if (right_bracket_present == FALSE)
			{
				if (lex.IsEof() == TRUE)
					ReportError(lex, gerr_rule_stmt_error, L"A right bracket is missing at the end of the conflict location marker.");
				break;
			}
		}

#ifdef USE_CURV_BRACKET_EXTENSION
		// Case 5. YACC/BISON compatibility construct.
		else if (lex.IsKeyword(spr_lcurvbr) == TRUE)
		{
			// Temp fix. Ignore everything up to the next curved bracket.
			int cnt_br = 1;
			while (GetLexema(lex) != ltx_eof)
			{
				if (lex.IsKeyword(spr_lcurvbr) == TRUE)
				{
					cnt_br++;
				}
				else if (lex.IsKeyword(spr_rcurvbr) == TRUE)
				{
					if (--cnt_br == 0)
						break;
				}
			}

			if (lex.IsEof() == TRUE)
				break;
		}
#endif

		// Case 6. Symbols that can be present at the end of the rule.
		else if (lex.IsKeyword(opr_colon) == TRUE || lex.IsKeyword(opr_bitor) == TRUE || lex.IsKeyword(spr_semicol) == TRUE)
		{
			// Register the rule.
			if (SaveGrammarRule(lex.origin, non_term, rule_app_id, rule_app_id_name, location_markers, rule_non_term_origin, rule_intro_area, rule_symbol_areas) == FALSE)
			{
				// Saving failed.
				ScanToSemicolon(lex);
				return;
			}

			// Check the symbol that stays at the end of the rule.
			if (lex.IsKeyword(spr_semicol) == TRUE)
			{
				// End of the statement.
				break;
			}

			// Clear data fields that were accumulating the props of the rule.
			m_sym_buffer.Clear();
			rule_app_id = 0;
			rule_app_id_name[0] = 0;
			rule_intro_area = lex.origin;
			rule_symbol_areas.Clear();
			location_markers.Clear();
		}

		// Case 7. Bogus lexema inside the rule definition.
		else
		{
			// Emit syntax error and continue.
			ReportError(lex, gerr_rule_stmt_error, L"Bogus lexema inside the rule.");
		}
	}

	// Check that semicolon is present at the end of the rule.
	if (lex.IsKeyword(spr_semicol) == FALSE)
	{
		ReportError(lex, gerr_rule_stmt_error, L"Semicolon is missing at the end of the rule.");
		ScanToSemicolon(lex);
		return;
	}

	// Full success.
	assert(m_src_data.m_rule_intro_areas.NumItems() == m_src_data.m_rules.NumItems());
}

void TGrammarDefinitionParser::ScanExpectedConflictDefnHeader(TLexema &lex_beg)
{
	//
	//  Expected conflict definition should look like:
	//
	//		<name> [ __id( <number> ) ]
	//		{
	//			...
	//			<inner_sections_and_statements>
	//			....
	//		}
	//
	//  This function scans only the header of this definition:
	//
	//		<name> [ __id( <number> ) ]
	//		{
	//
	assert(m_curr_section == grsc_conflicts);

	// Statement should begin with the name of the conflict.
	if (lex_beg.type != ltx_name)
	{
		ReportError(lex_beg, gerr_xpct_cfct_stmt_error, L"The name of the expected grammar conflict is missing.");
		ScanToSemicolon(lex_beg);
		return;
	}

	// Pick up the next lexema and check it.
	TLexema lex;
	GetLexema(lex);
	WORD app_id = 0;
	if (lex.IsName(L"__id") == TRUE)
	{
		// Scan identifier value.
		if (ScanObjectId(lex, app_id, NULL, 0, gerr_xpct_cfct_stmt_error) == FALSE)
		{
			// Errors are already reported.
			if (lex.IsKeyword(spr_lcurvbr) == TRUE)
				ScanToClosingCurvedBracket(lex);
			else ScanToSemicolon(lex);
			return;
		}

		// Pick up the next lexema after the the id.
		GetLexema(lex);
	}

	// Next lexema should be the curved bracket.
	if (lex.IsKeyword(spr_lcurvbr) != TRUE)
	{
		ReportError(lex, gerr_xpct_cfct_stmt_error, L"An opening curved bracket is missing after the name of the expected grammar conflict.");
		ScanToSemicolon(lex);
		return;
	}

	// Ensure that the name of the object is fine.
	if (CheckName(lex.origin, lex_beg.str_value, gerr_xpct_cfct_stmt_error) == FALSE)
	{
		// Error is already reported.
		ScanToClosingCurvedBracket(lex);
		return;
	}

	// Check for the implicit app_id that might come from the macro dictionary.
	if (GetObjectIdFromMacroDefnsTable(lex, L"xpct_", lex_beg.str_value, app_id, gerr_xpct_cfct_stmt_error) == FALSE)
	{
		// Error is already reported.
		ScanToClosingCurvedBracket(lex);
		return;
	}

	//
	// All parts of the statement were successfully scanned and verified.
	//

	// Duplicate name and check the name duplication success.
	wchar_t *xpct_name = TGrammar::StrDupe(lex_beg.str_value);
	if (xpct_name == NULL)
	{
		ReportError(lex, gerr_xpct_cfct_stmt_error, L"Out of memory while saving the name of expected conflict definition.");
		ScanToClosingCurvedBracket(lex);
		return;
	}

	// Allocate new support structure.
	TExpectedConflictSupp *xpct_supp = new TExpectedConflictSupp(m_cxpct);
	if (xpct_supp == NULL)
	{
		TGrammar::FreeStr(xpct_name);
		ReportError(lex, gerr_xpct_cfct_stmt_error, L"Out of memory while allocating the expected conflict support structure.");
		ScanToClosingCurvedBracket(lex);
		return;
	}

	// Fill in the new structure.
	xpct_supp->m_xpct_proto.xpct_conflict_name = xpct_name;
	xpct_supp->m_xpct_proto.xpct_conflict_app_id = app_id;
	xpct_supp->m_xpct_proto.num_conforming_conflicts = 0;
	xpct_supp->m_xpct_proto.nested_grammar = NULL;
	xpct_supp->m_xpct_proto.event_id = 0;
	xpct_supp->m_intro_area = lex_beg.origin;

	// Report expected conflict area to the parsing database. Note that the index of this expected
	// conflict is not known yet. And even worse, it can differ for different grammars in the hierarchy.
	// For simplicity the event is sent with a generic symbol value. This seems to be enough for now.
	PrGrammarDefnObject info;
	info.hdr.Setup(&lex_beg.origin, m_scanner->GetNextCN());
	info.object_sym = XpctConflictNameObject;
	info.object_app_id = app_id;
	info.extra_context_area.Clear();
	m_scanner->CbkHandler().GrammarObjectNotification(&info);
	m_scanner->PostProsessMidLevCbkCall(info.hdr);

	// Full success. Change the global state of the grammar defn parser.
	m_cxpct = xpct_supp;
	m_cnt_nesting++;
	m_curr_section = grsc_none;
}

void TGrammarDefinitionParser::ScanConflictLocationReferences(TLexema &lex)
{
	//
	//  List of conflict location references in the expected grammar conflict definition should look like:
	//
	//    <conflict_location_name> [ , <conflict_location_name> ... ] ;
	//
	assert(m_cnt_nesting > 0);

	// Loop on the names of the location objects. Note that the name of the first location object is passed in the parameter of the method.
	while (lex.IsEof() == FALSE && lex.IsKeyword(spr_semicol) == FALSE)
	{
		if (lex.IsKeyword(opr_comma) == TRUE)
		{
			GetLexema(lex);
			continue;
		}

		if (lex.type != ltx_name)
		{
			ReportError(lex, gerr_xpct_location_error, L"Bogus lexema in place of the expected conflict location name.");
			ScanToSemicolon(lex);
			return;
		}

		// The name is picked up from the source. Conflict location object with this name should be already defined.
		int iloc = TGrammarSymbolsHelper::FindLocationObject(m_src_data.m_location_objects, lex.str_value);
		if (iloc == -1)
		{
			wchar_t loc_buff[80];
			ReportError(lex, gerr_xpct_location_error, L"The name \"%s\" is not a name of the conflict location object.", lex.str_value.CopyWithTruncationTo(loc_buff, 80));
			ScanToSemicolon(lex);
			return;
		}

		// Location object with this name is available. Create reference to this location object.
		WORD loc_inx = (WORD)iloc;
		if (m_cxpct->m_xpct_proto.xpct_conflict_locations.AppendItem(loc_inx) == FALSE)
		{
			ReportError(lex, gerr_xpct_location_error, L"Low on memory while creating a reference to the conflict location object.");
			ScanToSemicolon(lex);
			return;
		}

		// Increment the refs count in the location object.
		m_src_data.m_location_objects[iloc].num_xpct_conflicts++;

		// Pick up the next lexema.
		GetLexema(lex);
	}

	// Ensure that the semicolon is present at the end of the statement.
	if (lex.IsKeyword(spr_semicol) == FALSE)
	{
		ReportError(lex, gerr_xpct_location_error, L"Semicolon is missing at the end of the definition of the expected grammar conflict.");
		ScanToSemicolon(lex);
		return;
	}
}

void TGrammarDefinitionParser::ScanConflictResolutionDefinition(TLexema &lex_beg)
{
	//
	//  Expected grammar conflict resolution should look like:
	//
	//    <action_code> [ , <action_code> ... ] : ;
	//    <action_code> [ , <action_code> ... ] : <terminal_or_non_term_name_or_empty_placeholder> [ , <terminal_or_non_term_name_or_empty_placeholder> ... ] ;
	//
	//  Where <action_code> can be:
	//
	//		shift			--	Keyword, it means the "shift" action. Shift action will be taken if conforming grammar
	//						conflict is the shift/reduce conflict.
	//		<number>	--	App_id of some existing rule object. Applying this rule should be the reduce action in one
	//						or more grammar conflicts that belong to the current expected conflict. This rule should have
	//						location marker at its end and this loc object should alreadybelong to the current expected
	//						conflict.
	//		<number>	--	App id of the already existing nested result object or the new app_id that is not assigned
	//						to anything yet. In the second case new nested result object with this id will be created.
	//						Mentioning the nested result object in the list of action codes puts obligation on the callback
	//						handler to change this result either to the shift or to the resolve action once processing of
	//						the nested grammar ends with this nested result. Nested result cannot coexist with other
	//						action codes in the list of action codes. Otherwise this will raise the ambiguity.
	//
	//  Notes on the target symbols:
	//
	//		Each target symbol (terminal symbol, non terminal or the empty placeholder) can be present only once in all
	//		conflict resolution statements of one prticular expected conflict.
	//
	//		Terminal or non terminal should be already defined at the moment of scanning the resolution statement.
	//
	//		Each terminal or non terminal or empty placeholder generates a resolution rule in the nested grammar. So, when
	//		the next target symbol is processed, all already existing rules are checked to ensure that the current target symbol
	//		was not mentioned in the current expected conflict before.
	//
	//		Axioma of the main grammar cannot be used as the target symbol.
	//
	assert(m_cnt_nesting > 0);

	if (m_axioma == 0)
	{
		// Since axioma should not be present in the list of the target symbols, the axioma itself should be
		// already defined. Ensure this.
		ReportError(lex_beg, gerr_xpct_resolution_error, L"Axioma symbol should be defined before scannning the resolution of the expected conflict.");
		ScanToSemicolon(lex_beg);
		return;
	}

	//
	//   Part 1. Loop on picking up and buffering the action codes. List of the action codes cannot be empty.
	//
	TSymbolsArray actions_array;				// This is the buffer for temp storing the scanned action codes.
	PrEventHeader rule_intro_origin;			// This origin data will be used in the second loop but it should be allocated now
											// because end of the action codes marker is the intro symbol for the first rule.

	bool action_shift_present = FALSE;
	bool action_rule_id_present = FALSE;
	bool action_app_target_present = FALSE;
	for(;;)
	{
		// Check the current lexema.
		WORD action_code;
		if (lex_beg.IsEof() == TRUE || lex_beg.IsKeyword(opr_colon) == TRUE || lex_beg.IsKeyword(spr_semicol) == TRUE)
		{
			// End of the action codes list.
			rule_intro_origin = lex_beg.origin;
			break;
		}
		else if (lex_beg.IsName(L"shift") == TRUE || lex_beg.IsName(L"Shift") == TRUE || lex_beg.IsName(L"SHIFT") == TRUE)
		{
			if (action_shift_present == TRUE)
			{
				ReportError(lex_beg, gerr_xpct_resolution_error, L"Shift action is already present in the list of actions.");
				ScanToSemicolon(lex_beg);
				return;
			}

			if (action_app_target_present == TRUE)
			{
				ReportError(lex_beg, gerr_xpct_resolution_error, L"Shift action cannot coexist with the nested result action in the same statement.");
				ScanToSemicolon(lex_beg);
				return;
			}

			// The action code is shift.
			action_code = 0;
			action_shift_present = TRUE;
		}
		else if (lex_beg.type == ltx_number)
		{
			// Ensure that this number can be used as object app_id.
			int num_val = (int)lex_beg.num_value;
			if (num_val <= 0 || num_val > MAX_APP_ID_VALUE)
			{
				ReportError(lex_beg, gerr_xpct_resolution_error, L"Value %I64d cannot be used as rule app_id or nested processing result app_id.", lex_beg.num_value);
				ScanToSemicolon(lex_beg);
				return;
			}

			// Look for objects with this id.
			WORD sym = FindObjectByAppId((WORD)lex_beg.num_value);

			// Check results of lookup.
			if (sym == 0 || (sym >= NestedResultsBase && sym < XpctConflictsBase))
			{
				// Object is not existing or this is an already existing nested_proc_result object.
				if (action_shift_present == TRUE)
				{
					ReportError(lex_beg, gerr_xpct_resolution_error, L"Shift action is already present in the statement. Nested processing result code is not allowed in this case.");
					ScanToSemicolon(lex_beg);
					return;
				}
				else if (action_rule_id_present == TRUE)
				{
					ReportError(lex_beg, gerr_xpct_resolution_error, L"Rule action is already present in the statement. Nested processing result code is not allowed in this case.");
					ScanToSemicolon(lex_beg);
					return;
				}
				else if (action_app_target_present == TRUE)
				{
					ReportError(lex_beg, gerr_xpct_resolution_error, L"Nested processing result code cannot be used more than once in the same statement.");
					ScanToSemicolon(lex_beg);
					return;
				}

				// App target action is noticed and it is applicable.
				action_app_target_present = TRUE;
			}
			else if (sym >= RuleObjectsBase && sym < LocationObjectsBase)
			{
				// Rule object.
				if (action_app_target_present == TRUE)
				{
					ReportError(lex_beg, gerr_xpct_resolution_error, L"Rule action cannot coexist with the nested result action in the same statement.");
					ScanToSemicolon(lex_beg);
					return;
				}

				// Ensure that this rule_id is not already present in the list of actions.
				for (int iact=0; iact<actions_array.NumItems(); ++iact)
				{
					if (actions_array[iact] == (WORD)lex_beg.num_value)
					{
						ReportError(lex_beg, gerr_xpct_resolution_error, L"Rule with id %I64d is already present in the list of actions.", lex_beg.num_value);
						ScanToSemicolon(lex_beg);
						return;
					}
				}

				// Rule id action is noticed and it is applicable.
				action_rule_id_present = TRUE;
			}
			else
			{
				// Other type of object.
				ReportError(lex_beg, gerr_xpct_resolution_error, L"Object with id %I64d is existing but it is not a rule or a nested processing result.", lex_beg.num_value);
				ScanToSemicolon(lex_beg);
				return;
			}

			if (sym == 0)
			{
				//
				//  This is definition of the new nested result object.
				//
				WORD nres_app_id = (WORD)lex_beg.num_value;
				wchar_t nres_name[TGrammar::MAX_NAME_LENGTH+1];
				if (GetNameOfTheCurrentMacro(lex_beg, nres_name) == FALSE)
					swprintf(nres_name, TGrammar::MAX_NAME_LENGTH+1, L"%hd", nres_app_id);

				// Store the origin of the object.
				if (m_src_data.m_nested_proc_res_intro_areas.AppendItem(lex_beg.origin) == FALSE)
				{
					ReportError(lex_beg, gerr_xpct_resolution_error, L"Out of memory while saving nested processing result intro area with app_id %hd.", nres_app_id);
					ScanToSemicolon(lex_beg);
					return;
				}

				// Create new nested processing result object.
				if (TGrammarSymbolsHelper::AddNestedProcResult(m_src_data.m_nested_processing_results, nres_app_id, nres_name, 0) == FALSE)
				{
					ReportError(lex_beg, gerr_xpct_resolution_error, L"Cannot create nested processing result object with app_id %hd.", nres_app_id);
					m_src_data.m_nested_proc_res_intro_areas.ReleaseLastItem();
					ScanToSemicolon(lex_beg);
					return;
				}
			}

			// Object with this id is existing or it was just created.
			action_code = (WORD)lex_beg.num_value;
		}
		else
		{
			// Bogus lexema.
			ReportError(lex_beg, gerr_xpct_resolution_error, L"Bogus lexema instead of the action code in the conflict resolution statement.");
			ScanToSemicolon(lex_beg);
			return;
		}

		// Action code is fine. Store it.
		if (actions_array.AppendItem(action_code) == FALSE)
		{
			ReportError(lex_beg, gerr_xpct_resolution_error, L"Unable to store the action code %hd.", action_code);
			ScanToSemicolon(lex_beg);
			return;
		}

		// Pick up the next lexema.
		GetLexema(lex_beg);
	}

	// Ensure that list of actions is not empty.
	if (actions_array.NumItems() == 0)
	{
		ReportError(lex_beg, gerr_xpct_resolution_error, L"The list of action codes is missing.");
		ScanToSemicolon(lex_beg);
		return;
	}

	// Pick up info about the current expected conflict.
	TSymbolsArrayArray &resolution = m_cxpct->m_xpct_proto.resolution;

	//
	//   Part 2.	This is loop on picking up the terminal symbols, non terminals and empty placeholders. Once the symbol
	//			is picked and it is valid, the resolution rule is immediately generated for this symbol. This effectively converts
	//			the source structure like:
	//
	//				ac1, ac2		:	s1, s2, s3;
	//				ac3, ac4, ac2	:	s4, , s5;			// Note the empty place holder that is used on this line.
	//												// The action symbol ac2 is used once again. This is ok.
	//		into:
	//
	//			ac1, ac2		:	s1;
	//			ac1, ac2		:	s2;
	//			ac1, ac2		:	s3;
	//			ac3, ac4, ac2	:	s4;
	//			ac3, ac4, ac2	:	;
	//			ac3, ac4, ac2	:	s5;
	//
	//		Each symbol to the right of the colon generates one resolution rule.
	//

	TLexema lex;
	WORD symbol = 0;
	PrEventHeadersArray res_symbol_origins;
	while (GetLexema(lex) != ltx_eof)
	{
		if (lex.type == ltx_charconst || lex.type == ltx_name)
		{
			if (symbol != 0)
			{
				// Non empty symbol is already assigned.
				ReportError(lex, gerr_xpct_resolution_error, L"Delimiter between the names of terminal symbol or non terminal is missing.");
				ScanToSemicolon(lex);
				return;
			}

			symbol = m_src_data.FindSymbol(lex);
			if (symbol == 0)
			{
				ReportError(lex, gerr_xpct_resolution_error, L"The name of the terminal symbol or non terminal is missing.");
				ScanToSemicolon(lex);
				return;
			}
			else if (symbol == m_axioma)
			{
				ReportError(lex, gerr_xpct_resolution_error, L"Axioma cannot be present in the list of the target symbols. Rework the grammar.");
				ScanToSemicolon(lex);
				return;
			}

			// Store location of the symbol. Result check can be omitted because resolution rules cannot have more than one symbol.
			res_symbol_origins.AppendItem(lex.origin);
		}
		else if (lex.IsKeyword(opr_colon) == TRUE || lex.IsKeyword(opr_bitor) == TRUE || lex.IsKeyword(opr_comma) == TRUE || lex.IsKeyword(spr_semicol) == TRUE)
		{
			//
			//  Save rule with empty or non empty symbol in its right hand side.
			//

			// Ensure that rule for this symbol is not created yet.
			for (int irule=0; irule<m_cxpct->m_resolution_rules.NumItems(); ++irule)
			{
				int rule_len = m_cxpct->m_resolution_rules[irule].Length();
				if (rule_len == 0)
				{
					if (symbol == 0)
					{
						ReportError(lex, gerr_xpct_resolution_error, L"Empty rule is already present as one of the expected conflict resolution alternatives.");
						ScanToSemicolon(lex);
						return;
					}
				}
				else
				{
					assert(rule_len == 1);

					// It is ok not to check that symbol in the rule is not EOF because rules cannot have EOFs in their right hand sides.
					if (m_cxpct->m_resolution_rules[irule].symbols[0] == symbol)
					{
						wchar_t buff_40_chars[40];
						ReportError(lex, gerr_xpct_resolution_error, L"Grammar symbol %s(%hd) is already present as one of the expected conflict resolution alternatives.",
									m_src_data.GetSymbolName(symbol, buff_40_chars), symbol);
						ScanToSemicolon(lex);
						return;
					}
				}
			}

			//
			//  At this point 2 things are available:
			//
			//	--	List of actions (actions_array). This is list of actions that should be taken in different grammar
			//		conflicts that will be associated with the current expected conflict.
			//
			//	--	Target symbol (symbol). Once nested grammar will come to this symbol, the appropriate action
			//		from the list of actions should be taken.
			//
			//  Later on other code will verify that each participating grammar conflict has unambiguous action among
			//  the list of actions and that every action is used in at least one participating grammar conflict.
			//

			// Create copy of the actions array. This is necessary because original array might be still needed.
			TSymbolsArray actions_array_copy;
			if (actions_array_copy.AppendItems(actions_array.DataPtr(), actions_array.NumItems()) == FALSE)
			{
				ReportError(lex, gerr_xpct_resolution_error, L"Out of memory while duplicating list of actions.");
				ScanToSemicolon(lex);
				return;
			}

			// Create entry in the resolutions array.
			if (resolution.AppendItem(actions_array_copy) == FALSE)
			{
				ReportError(lex, gerr_xpct_resolution_error, L"Out of memory while adding entry to the conflict resolutions array.");
				ScanToSemicolon(lex);
				return;
			}

			// Create rule for this symbol.
			WORD rule_non_term = 0;
			TSymbolsArray rule_symbols;
			if (symbol != 0)
				rule_symbols.AppendItem(symbol);

			if (TGrammarSymbolsHelper::AddGrammarRule(m_cxpct->m_resolution_rules, rule_non_term, rule_symbols, 0, NULL, 0, 0) == FALSE)
			{
				ReportError(lex, gerr_xpct_resolution_error, L"Out of memory while adding generated conflict resolution rule.");
				resolution.ReleaseLastItem();
				ScanToSemicolon(lex);
				return;
			}

			// For now use the right hand sude symbol or the trailing delimiter both as rule non term and rule intro symbol.
			TGrammarRuleOrigin res_rule_origin = { rule_intro_origin, rule_intro_origin };
			res_rule_origin.symbol_origins.TakeContentsFrom(res_symbol_origins);
			res_rule_origin.rule_propagated = FALSE;

			if (m_cxpct->m_res_rule_intro_areas.AppendItem(res_rule_origin) == FALSE)
			{
				ReportError(lex, gerr_xpct_resolution_error, L"Out of memory while adding generated conflict resolution rule origin.");
				m_cxpct->m_resolution_rules.ReleaseLastItem();
				resolution.ReleaseLastItem();
				ScanToSemicolon(lex);
				return;
			}

			if (symbol != 0)
				m_src_data.BumpSymbolRuleUse(symbol);

			// Check type of the current symbol.
			if (lex.IsKeyword(spr_semicol) == TRUE)
			{
				// End of the statement.
				break;
			}

			// Clear the target symbol and continue scanning.
			symbol = 0;
			res_symbol_origins.Clear();
			rule_intro_origin = lex.origin;
		}
		else
		{
			ReportError(lex, gerr_xpct_resolution_error, L"Bogus symbol instead of the name of the terminal symbol or non terminal.");
			ScanToSemicolon(lex);
			return;
		}
	}

	// Ensure that semicolon is present at the end of the statement.
	if (lex.IsKeyword(spr_semicol) == FALSE)
	{
		ReportError(lex, gerr_xpct_resolution_error, L"Semicolon is missing at the end of conflict resolution statement.");
		ScanToSemicolon(lex);
		return;
	}
}

void TGrammarDefinitionParser::ExitExpectedConflictDefinition(TLexema &lex_end)
{
	assert(m_cnt_nesting > 0);
	TExpectedConflictSupp *xpct_supp = m_cxpct;

	// Exit from the current nesting. Reduce the nesting count and change the current section type back
	// to <conflicts> as it was when the header of the expected conflict was scanned.
	m_cxpct = m_cxpct->m_parent_supp;
	m_cnt_nesting--;
	m_curr_section = grsc_conflicts;

	// Check contents of the expected conflict.
	if (xpct_supp->m_xpct_proto.xpct_conflict_locations.NumItems() == 0)
	{
		// Conflict without locations does not make any sense.
		ReportError(lex_end, gerr_xpct_cfct_stmt_error, L"Names of the conflict location objects are missing in the expected conflict \"%s\". Expected conflict is dismissed.",
					xpct_supp->m_xpct_proto.xpct_conflict_name);

		// Get rid of this structure.
		DecrementXpctConflictRefCountsHier(*xpct_supp);
		m_cxpct->m_child_supps.RemoveItem(xpct_supp);
		delete xpct_supp;
	}
	else if (xpct_supp->m_resolution_rules.NumItems() == 0 && xpct_supp->m_child_supps.IsEmpty() == FALSE)
	{
		// Conficts are defined while the rules are not. There is no place for these conflicts to surface on.
		ReportError(lex_end, gerr_xpct_cfct_stmt_error, L"Conflict resolution rules are missing in the expected conflict \"%s\". Expected conflict is dismissed.",
					xpct_supp->m_xpct_proto.xpct_conflict_name);

		// Get rid of this structure.
		DecrementXpctConflictRefCountsHier(*xpct_supp);
		m_cxpct->m_child_supps.RemoveItem(xpct_supp);
		delete xpct_supp;
	}
}

void TGrammarDefinitionParser::DecrementXpctConflictRefCountsHier(TExpectedConflictSupp &xpct)
{
	// Decrement the conflict location object ref counts.
	TSymbolsArray &loc_refs = xpct.m_xpct_proto.xpct_conflict_locations;
	for (int iloc=0; iloc<loc_refs.NumItems(); ++iloc)
	{
		WORD loc_inx = loc_refs[iloc];
		m_src_data.m_location_objects[loc_inx].num_xpct_conflicts--;
	}

	// Decrement the rule uses ref counts.
	for (int irule=0; irule<xpct.m_resolution_rules.NumItems(); ++irule)
	{
		int rule_len = xpct.m_resolution_rules[irule].Length();
		if (rule_len > 0)
		{
			assert(rule_len == 1);
			m_src_data.BumpSymbolRuleUse(xpct.m_resolution_rules[irule].symbols[0], FALSE);
		}
	}

	// Do similar thing in nested conflicts if any.
	for (TListIter<TExpectedConflictSupp> iter(xpct.m_child_supps); iter; ++iter)
	{
		DecrementXpctConflictRefCountsHier(iter.CurrItem());
	}
}

bool TGrammarDefinitionParser::SaveRuleRightHandSideSymbol(WORD sym, TLexema &lex, PrEventHeadersArray &areas)
{
	if (m_sym_buffer.AppendItem(sym) == TRUE)
	{
		// Symbol was placed to the buffer.
		if (areas.AppendItem(lex.origin) == TRUE)
		{
			m_src_data.BumpSymbolRuleUse(sym);
		}
		else
		{
			ReportError(lex, gerr_rule_stmt_error, L"Out of memory while processing terminal or non terminal symbol area in the rule.");
			m_sym_buffer.IncNumItems(-1);
			return(FALSE);
		}
	}
	else
	{
		ReportError(lex, gerr_rule_stmt_error, L"Out of memory while processing terminal or non terminal symbol in the rule.");
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TGrammarDefinitionParser::SaveGrammarRule(PrEventHeader &ctx, WORD non_term, WORD rule_app_id, const wchar_t *rule_app_id_name,
												TConflictLocationMarkerInfosArray &markers_array, PrEventHeader &rule_non_term_intro_area,
												PrEventHeader &rule_intro_area, PrEventHeadersArray &symbol_areas)
{
	// Check the passed rule before saving it.
	if (m_sym_buffer.NumItems() == 1 && m_sym_buffer[0] == non_term)
	{
		// This rule is plain taftology. Reject this rule.
		ReportError(ctx, gerr_rule_stmt_error, L"Single symbol on the right side of the rule is identical to the left side symbol. The rule is ignored.");
		return(FALSE);
	}

	// Check for rules duplication.
	for (int irule=0; irule<m_src_data.m_rules.NumItems(); ++irule)
	{
		TGrammarRule &rule = m_src_data.m_rules[irule];

		// Check the left hand sides.
		if (non_term != rule.non_term)
			continue;

		// Compare the right hand sides.
		if (m_sym_buffer.IsEqualTo(rule.symbols) == TRUE)
		{
			// Rules are the same. Issue error and ignore the scanned rule.
			wchar_t buff_40_chars[40];
			ReportError(ctx, gerr_rule_stmt_error,
						L"Rule is identical to R%d (%s.%d). The rule is ignored.",
						irule, m_src_data.GetSymbolName(rule.non_term, buff_40_chars), TGrammarSymbolsHelper::GetRuleOffs(m_src_data.m_rules, irule));
			return(FALSE);
		}
	}

	// Save rule intro areas.
	TGrammarRuleOrigin rule_origin = { rule_non_term_intro_area, rule_intro_area };
	rule_origin.symbol_origins.TakeContentsFrom(symbol_areas);
	rule_origin.rule_propagated = FALSE;

	if (m_src_data.m_rule_intro_areas.AppendItem(rule_origin) == FALSE)
	{
		ReportError(ctx, gerr_rule_stmt_error, L"Out of memory while saving the rule intro area.");
		rule_origin.ReleaseObject();
		return(FALSE);
	}

	// Add rule to the grammar.
	ID event_id = m_scanner->GetNextCN();
	if (TGrammarSymbolsHelper::AddGrammarRule(m_src_data.m_rules, non_term, m_sym_buffer, rule_app_id, rule_app_id_name, rule_non_term_intro_area.cn, event_id) == FALSE)
	{
		// Adding failed.
		ReportError(ctx, gerr_rule_stmt_error, L"Out of memory while adding rule to the grammar.");
		m_src_data.m_rule_intro_areas.ReleaseLastItem();
		return(FALSE);
	}

	// Pick up the new number of rules.
	int num_rules = m_src_data.m_rules.NumItems();

	// Bump left hand side non terminal use.
	m_src_data.m_non_terminals[non_term-NonTerminalsBase].num_lhs_rule_uses++;

	// Process conflict location markers if any.
	for (int imarker=0; imarker<markers_array.NumItems(); ++imarker)
	{
		// Check if new location object should be created or not.
		int iloc = markers_array[imarker].loc_index;
		if (iloc == -1)
		{
			// Name of the object in the slot should be not NULL.
			wchar_t *loc_name = markers_array[imarker].loc_name;
			assert(loc_name != NULL);

			// Try to repick the conflict location index.
			TStrPtrInfo loc_name_ptr = { loc_name, wcslen(loc_name) };
			iloc = TGrammarSymbolsHelper::FindLocationObject(m_src_data.m_location_objects, loc_name_ptr);
			if (iloc == -1)
			{
				// Redo the name cleanup before creating the new conflict location.
				WORD sym_val = m_src_data.FindSymbol(loc_name_ptr);
				if (sym_val != 0)
				{
					// Symbol with this name is existing.
					if (sym_val < NonTerminalsBase)
						ReportError(ctx, gerr_rule_stmt_error, L"Name in the conflict location marker (%s) is already registered as a terminal symbol of the grammar.", loc_name);
					else ReportError(ctx, gerr_rule_stmt_error, L"Name in the conflict location marker (%s) is already registered as a non terminal symbol of the grammar.", loc_name);
				}
				else
				{
					// Save the object intro area.
					if (m_src_data.m_loc_object_intro_areas.AppendItem(markers_array[imarker].loc_name_origin) == TRUE)
					{
						// Create new conflict location object.
						TExpectedConflictLocation new_loc = { loc_name, 0xFFFF, TRulePositionsArray(), 0, TSymbolsArray() };
						if (m_src_data.m_location_objects.AppendItem(new_loc) == TRUE)
						{
							// Pick up the index of the new conflict location object and mark the name in the array of location markers as consumed.
							iloc = m_src_data.m_location_objects.NumItems()-1;
							markers_array[imarker].loc_name = NULL;
						}
						else
						{
							// Value of iloc variable is already -1. This will block furter processing of the current location marker.
							ReportError(ctx, gerr_rule_stmt_error, L"Out of memory while creating conflict location object.");
							m_src_data.m_loc_object_intro_areas.ReleaseLastItem();
						}
					}
					else
					{
						ReportError(ctx, gerr_rule_stmt_error, L"Out of memory while saving expected conflict location intro area.");
					}
				}
			}
		}

		// Check if finding/creation succeeded or not.
		if (iloc != -1)
		{
			// Conflict location object was identified or created.
			wchar_t *loc_name = m_src_data.m_location_objects[iloc].location_name;
			WORD loc_action_symbol = m_src_data.m_location_objects[iloc].action_symbol;
			TRulePositionsArray &loc_positions = m_src_data.m_location_objects[iloc].positions;

			// Process the position part of the current marker.
			TRulePosition &pos = markers_array[imarker].position;
			WORD marker_act_sym = 0xFFFF;

			// Ensure that action symbol is properly present/absent.
			if (pos.isym >= m_src_data.m_rules[num_rules-1].Length())
			{
				// Location marker is at the end of the rule. Such marker requires an action symbol.
				if (pos.action_sym == 0xFFFF)
				{
					ReportError(ctx, gerr_rule_stmt_error, L"Conflict location marker (%s) is at the end of the rule. It does not contain the required action symbol.", loc_name);
					if (loc_action_symbol == 0xFFFF)
					{
						// Loc object was just created. Set the action symbol there (EOF) to indicate that it is not brand new.
						m_src_data.m_location_objects[iloc].action_symbol = 0;
					}

					iloc = -1;
				}
				else
				{
					marker_act_sym = pos.action_sym;
				}
			}
			else
			{
				// Location marker stays in the middle of the rule. Such marker should not have an action symbol.
				// Once the action symbol is missing, it cannot be compared to the symbol of the rule that stays after it.
				if (pos.action_sym != 0xFFFF)
				{
					ReportError(ctx, gerr_rule_stmt_error, L"Conflict location marker (%s) stays in the middle of the rule. It should not contain any action symbol.", loc_name);
					if (loc_action_symbol == 0xFFFF)
					{
						// Loc object was just created. Set the action symbol there (EOF) to indicate that it is not brand new.
						m_src_data.m_location_objects[iloc].action_symbol = 0;
					}

					iloc = -1;
				}
				else
				{
					marker_act_sym = m_src_data.m_rules[num_rules-1].symbols[pos.isym];
				}
			}

			if (iloc != -1)
			{
				assert(marker_act_sym != 0xFFFF);

				// Ensure action symbol values compatibility.
				if (loc_action_symbol != 0xFFFF)
				{
					// Action symbol is present in the marker object. This means that the marker object is not new.
					if (pos.action_sym != 0xFFFF && pos.action_sym != loc_action_symbol)
					{
						wchar_t buff1[40], buff2[40];
						ReportError(ctx, gerr_rule_stmt_error, L"Action symbol (%s) in the conflict location marker (%s) differs from the action symbol (%s) in the definition of the conflict location object.",
									m_src_data.GetSymbolName(pos.action_sym, buff1), loc_name, m_src_data.GetSymbolName(loc_action_symbol, buff2));
						iloc = -1;
					}
					else if (pos.action_sym == 0xFFFF && marker_act_sym != loc_action_symbol)
					{
						wchar_t buff1[40], buff2[40];
						ReportError(ctx, gerr_rule_stmt_error, L"Symbol of the rule (%s) that follows the conflict location marker (%s) differs from the action symbol (%s) in the definition of the conflict location object.",
									m_src_data.GetSymbolName(marker_act_sym, buff1), loc_name, m_src_data.GetSymbolName(loc_action_symbol, buff2));
						iloc = -1;
					}
				}
				else if (marker_act_sym >= NonTerminalsBase)
				{
					wchar_t buff1[40];
					ReportError(ctx, gerr_rule_stmt_error, L"Symbol of the rule (%s) that follows the conflict location marker (%s) is non terminal.",
								m_src_data.GetSymbolName(marker_act_sym, buff1), loc_name);

					// Assign fake action symbol (EOF) to the conflict location object.
					m_src_data.m_location_objects[iloc].action_symbol = 0;
					iloc = -1;
				}
				else
				{
					// Assign action symbol to the conflict location object.
					m_src_data.m_location_objects[iloc].action_symbol = marker_act_sym;
				}
			}

			if (iloc != -1)
			{
				// Ensure that current position is not present in the object already.
				for (int ipos=0; ipos<loc_positions.NumItems(); ++ipos)
				{
					if (loc_positions[ipos].CompareTo(pos) == TRUE)
					{
						ReportError(ctx, gerr_rule_stmt_error, L"Conflict location marker (%s) is already present in the current conflict location object definition.", loc_name);
						iloc = -1;
					}
				}
			}

			if (iloc != -1)
			{
				// It is ok to add current loc marker to the conflict location object.
				if (loc_positions.AppendItem(pos) == FALSE)
				{
					// Appending failed.
					ReportError(ctx, gerr_rule_stmt_error, L"Out of memory while adding conflict location marker to the conflict location object description.");
				}
			}
		}
	}

	// Check the rule id presence if scanning options require this.
	if (rule_app_id == 0 && m_warn_on_rules_without_ids == TRUE)
	{
		wchar_t buff_40_chars[40];
		ReportWarning(ctx, gerr_rule_stmt_error, L"Rule for non terminal \"%s\".%d has no application id.",
					m_src_data.GetSymbolName(non_term, buff_40_chars), TGrammarSymbolsHelper::GetRuleOffs(m_src_data.m_rules, num_rules-1));
	}

	// Send notification to the parsing events database.
	PrGrammarDefnObject info;
	info.hdr.Setup(&rule_intro_area, event_id);
	info.object_sym = RuleObjectsBase+num_rules-1;
	info.object_app_id = rule_app_id;
	info.extra_context_area = rule_non_term_intro_area;
	m_scanner->CbkHandler().GrammarObjectNotification(&info);
	m_scanner->PostProsessMidLevCbkCall(info.hdr);

	// Success.
	assert(m_src_data.m_rule_intro_areas.NumItems() == num_rules);
	return(TRUE);
}

void TGrammarDefinitionParser::ScanPublicNonTerminalsStatement()
{
	// Loop on scanning names of the non terminals.
	TLexema lex;
	bool something_defined = FALSE;
	while (GetLexema(lex) != ltx_eof && lex.IsKeyword(spr_semicol) == FALSE)
	{
		if (lex.IsKeyword(opr_comma) == TRUE)
		{
			// The comma is optional. Ignore it.
			continue;
		}

		something_defined = TRUE;
		if (lex.type != ltx_name)
		{
			// Non terminals can be only name lexemas.
			ReportError(lex, gerr_public_sym_stmt_error, L"Bogus lexema in the deifinition of the public non terminals.");
			break;
		}

		WORD non_term = m_src_data.FindSymbol(lex);
		if (non_term == 0)
		{
			// Name is unknown. Register it.
			non_term = RegisterNonTerminal(lex, FALSE, gerr_public_sym_stmt_error);
			if (non_term == 0)
			{
				// Error is already reported.
				break;
			}
		}
		else if (non_term < NonTerminalsBase)
		{
			ReportError(lex, gerr_public_sym_stmt_error, L"The name of the terminal symbol cannot be used in the definition of the public non terminals.");
			continue;
		}

		// Current name represents a non terminal.
		if (non_term >= NonTermForwardDefnsBase)
		{
			if (m_src_data.m_fwd_def_non_terminals[non_term-NonTermForwardDefnsBase].public_symbol == TRUE)
			{
				wchar_t buff_40_chars[40];
				ReportError(lex, gerr_public_sym_stmt_error, L"Non terminal \"%s\"(%hd) is already defined as public.", m_src_data.GetSymbolName(non_term, buff_40_chars), non_term);
				continue;
			}

			// Mark this forward declaration as public.
			m_src_data.m_fwd_def_non_terminals[non_term-NonTermForwardDefnsBase].public_symbol = TRUE;
		}
		else
		{
			if (m_src_data.m_non_terminals[non_term-NonTerminalsBase].public_symbol == TRUE)
			{
				wchar_t buff_40_chars[40];
				ReportError(lex, gerr_public_sym_stmt_error, L"Non terminal \"%s\"(%hd) is already defined as public.", m_src_data.GetSymbolName(non_term, buff_40_chars), non_term);
				continue;
			}

			// Mark this non terminal as public.
			m_src_data.m_non_terminals[non_term-NonTerminalsBase].public_symbol = TRUE;
		}
	}

	if (something_defined == FALSE)
	{
		ReportError(lex, gerr_public_sym_stmt_error, L"Non terminal names are missing.");
	}

	// Ensure that statement is scanned completely.
	ScanToSemicolon(lex);
}

TLexemaType TGrammarDefinitionParser::GetLexema(TLexema &lex, bool ignore_comments_eols)
{
	// Rotate the loop until lexema that can be given out will show up. This loop cannot be
	// infinite becase sooner or later the scanner will return EOF.
	TLexemaType lext;
	for(;;)
	{
		lext = m_scanner->Scan(lex);

		if (lext == ltx_error)
			continue;
		if (ignore_comments_eols == TRUE && (lext == ltx_comment || lext == ltx_eol))
			continue;

		break;
	}

	return(lext);
}

void TGrammarDefinitionParser::ScanToSemicolon(TLexema &lex)
{
	// Check if passed lexema is already semicolon or not.
	if (lex.IsKeyword(spr_semicol) == TRUE)
		return;

	// Skip everything till the semicolon or EOF.
	while (GetLexema(lex) != ltx_eof)
	{
		if (lex.IsKeyword(spr_semicol) == TRUE)
			break;
	}
}

void TGrammarDefinitionParser::ScanToClosingCurvedBracket(TLexema &lex)
{
	// Check if passed lexema is already a curved bracket or not.
	if (lex.IsKeyword(spr_rcurvbr) == TRUE)
		return;

	// Skip everything till the scurved bracket or EOF.
	while (GetLexema(lex) != ltx_eof)
	{
		if (lex.IsKeyword(spr_rcurvbr) == TRUE)
			break;
	}
}

bool TGrammarDefinitionParser::ScanLexTypeAndSubtype(TLexema &lex, TLexemaType &lext, bool &chk_subt, TLexSubtype &subt, TGrammarDefnParserErrorCode err_code)
{
	// Pick up the next lexema if passed lexema is empty.
	if (lex.type == ltx_empty)
		GetLexema(lex);

	// Check that current lexema is name of some lexema type.
	if (lex.type == ltx_name)
	{
		// Convert sting value of the lexema into the NULL terminated string.
		if (lex.str_value.m_len > 2*TGrammar::MAX_NAME_LENGTH)
		{
			ReportError(lex, err_code, L"Name of the lexema type is too long.");
			ScanToSemicolon(lex);
			return(FALSE);
		}

		wchar_t lext_str_val[2*TGrammar::MAX_NAME_LENGTH+1];
		lex.str_value.CopyToVerifiedBuffer(lext_str_val);
		lext = TLexema::GetLexTypeFromEnumName(lext_str_val);
	}

	if (lex.type != ltx_name || lext == ltx_num_lexema_types)
	{
		ReportError(lex, err_code, L"Lexema type name is missing.");
		ScanToSemicolon(lex);
		return(FALSE);
	}

	if (lext == ltx_empty || lext >= ltx_eof)
	{
		ReportError(lex, err_code, L"Invalid lexema type name. ltx_empty, ltx_eof and ltx_error are not allowed.");
		ScanToSemicolon(lex);
		return(FALSE);
	}

	// Scan and process the next lexema.
	chk_subt = FALSE;
	switch (GetLexema(lex))
	{
		case ltx_number:
				{
					// Only name lexemas can have numeric subtype.
					if (lext != ltx_name)
					{
						ReportError(lex, err_code, L"Only the name lexemas can have numeric subtype.");
						ScanToSemicolon(lex);
						return(FALSE);
					}

					if ((short)lex.num_value < 0 || (short)lex.num_value >= TLexToSymConverter::lt_num_name_types)
					{
						ReportError(lex, err_code, L"The subtype value (val=%I64d) is either negative or it is greater or equal than %d.",
									lex.num_value, TLexToSymConverter::lt_num_name_types);
						ScanToSemicolon(lex);
						return(FALSE);
					}

					subt.subtype = (DWORD)lex.num_value;
					chk_subt = TRUE;
				}
				break;

		case ltx_keyword:
				{
					if ((short)lex.num_value == opr_colon || (short)lex.num_value == spr_semicol)
					{
						// A valid delimiter is detected after a valid lexema type name.
						return(TRUE);
					}

					ReportError(lex, err_code, L"Bogus token after the lexema type name.");
					ScanToSemicolon(lex);
					return(FALSE);
				}
				break;

		case ltx_name:
				{
					if (lex.IsName(L"__id") == TRUE)
					{
						// Lexema type does not have subtype after it.
						return(TRUE);
					}

					// Convert sting value of the lexema into NULL terminated string.
					if (lex.str_value.m_len > 2*TGrammar::MAX_NAME_LENGTH)
					{
						ReportError(lex, err_code, L"Name of the lexema subtype is too long.");
						ScanToSemicolon(lex);
						return(FALSE);
					}

					wchar_t lex_str_val[2*TGrammar::MAX_NAME_LENGTH+1];
					lex.str_value.CopyToVerifiedBuffer(lex_str_val);

					switch (lext)
					{
						case ltx_comment:
								{
									// Try to recognize the comment subtype.
									subt.comment_type = TLexema::GetLexCommentTypeFromEnumName(lex_str_val);
									if (subt.comment_type != lct_num_comment_types)
										chk_subt = TRUE;
								}
								break;

						case ltx_number:
								{
									// Try to recognize the number subtype.
									subt.number_type = TLexema::GetLexNumberTypeFromEnumName(lex_str_val);
									if (subt.number_type != lnt_num_number_types)
										chk_subt = TRUE;
								}
								break;

						case ltx_floating_point:
								{
									// Try to recognize the floating point sonstant subtype.
									subt.floating_point_type = TLexema::GetLexFloatingPointTypeFromEnumName(lex_str_val);
									if (subt.number_type != lfp_num_floating_point_types)
										chk_subt = TRUE;
								}
								break;

						case ltx_charconst:
								{
									// Try to recognize the charconst subtype.
									subt.charconst_type = TLexema::GetLexCharConstTypeFromEnumName(lex_str_val);
									if (subt.charconst_type != lchct_num_charconst_types)
										chk_subt = TRUE;
								}
								break;

						case ltx_string:
								{
									// Try to recognize the string subtype.
									subt.string_type = TLexema::GetLexStringTypeFromEnumName(lex_str_val);
									if (subt.string_type != lstrt_num_string_types)
										chk_subt = TRUE;
								}
								break;

						case ltx_keyword:
								{
									// Try to recognize the keyword name.
									subt.subtype = TLexema::GetLexKeywordIdFromEnumName(lex_str_val);
									if (subt.subtype != 0)
										chk_subt = TRUE;
								}
								break;

						case ltx_name:
								{
									// Names do not have subtypes.
									ReportError(lex, err_code, L"Name lexema cannot have name subtype specifier.");
									ScanToSemicolon(lex);
									return(FALSE);
								}

						case ltx_eol:
								{
									// Try to recognize the end of line subtype.
									subt.endofline_type = TLexema::GetLexEndOfLineTypeFromEnumName(lex_str_val);
									if (subt.endofline_type != leolt_num_eol_types)
										chk_subt = TRUE;
								}
								break;

						default:
							assert(FALSE);
							break;
					}
				}
				break;

		case ltx_eof:
				{
					ReportError(lex, err_code, L"Subtype name or delimiter is missing after the lexema type name.");
				}
				return(FALSE);

		default:
			{
				ReportError(lex, err_code, L"Bogus lexema after the lexema type name.");
				ScanToSemicolon(lex);
			}
			return(FALSE);
	}

	// Once control came here this means that subtype name was detected. It can be both valid and invalid.
	if (chk_subt == FALSE)
	{
		// Name of the subtype was not recognized.
		ReportError(lex, err_code, L"Invalid lexema subtype name.");
		ScanToSemicolon(lex);
		return(FALSE);
	}

	// Pick up lexema that stays after the recognized lexema subtype.
	GetLexema(lex);

	// Full success.
	return(TRUE);
}

bool TGrammarDefinitionParser::ScanObjectId(TLexema &lex, WORD &object_id, wchar_t *app_id_name_buffer, WORD exception_sym, TGrammarDefnParserErrorCode err_code)
{
	// This method expects the "__id" keyword is already scanned.
	assert(lex.IsName(L"__id") == TRUE);

	object_id = 0;
	if (app_id_name_buffer != NULL)
		app_id_name_buffer[0] = 0;

	// Scan the first lexema.
	if (GetLexema(lex) != ltx_keyword || (short)lex.num_value != opr_lpar)
	{
		ReportError(lex, err_code, L"An opening parenthesis is missing.");
		ScanToSemicolon(lex);
		return(FALSE);
	}

	// Scan the id value.
	if (GetLexema(lex) != ltx_number)
	{
		ReportError(lex, err_code, L"The value of the object app id should be numeric.");
		ScanToSemicolon(lex);
		return(FALSE);
	}

	// Check the value of the number.
	if (CheckObjectIdValue(lex, lex.num_value, exception_sym, err_code) == FALSE)
	{
		ScanToSemicolon(lex);
		return(FALSE);
	}

	// Give the id out. Note that the value 0 is a legal app_id value.
	object_id = (WORD)lex.num_value;

	if (app_id_name_buffer != NULL)
	{
		// This is a special case when it is necessary to check if this number is coming from the macro definition or not.
		GetNameOfTheCurrentMacro(lex, app_id_name_buffer);
	}

	// Scan the final lexema.
	if (GetLexema(lex) != ltx_keyword || (short)lex.num_value != opr_rpar)
	{
		ReportError(lex, err_code, L" The closing parenthesis is missing.");
		ScanToSemicolon(lex);
		return(FALSE);
	}

	// Full success.
	return(TRUE);
}

bool TGrammarDefinitionParser::GetObjectIdFromMacroDefnsTable(TLexema &err_ctx, const wchar_t *macro_prefix, TStrPtrInfo &symbol_name_ptr,
																	WORD &app_id, TGrammarDefnParserErrorCode err_code)
{
	// Check the length of the passed name.
	if (symbol_name_ptr.m_len > 2*TGrammar::MAX_NAME_LENGTH-10)
	{
		wchar_t loc_buff[80];
		ReportError(err_ctx, err_code, L"Name of the macrodefinition \"%s\" is too long.", symbol_name_ptr.CopyWithTruncationTo(loc_buff, 80));
		ScanToSemicolon(err_ctx);
		return(FALSE);
	}

	// Prepare name of the macro and look for this macro.
	wchar_t name_buff[2*TGrammar::MAX_NAME_LENGTH+1];
	wcscpy(name_buff, macro_prefix);
	symbol_name_ptr.CopyToVerifiedBuffer(name_buff+wcslen(name_buff));
	TStrPtrInfo macro_name_ptr = { name_buff, wcslen(name_buff) };
	TMacroDefinition *macro = m_scanner->FindMacro(macro_name_ptr);
	if (macro == NULL)
	{
		// Macro with this name is not available. This situation is fine.
		return(TRUE);
	}

	// Check the discovered macro.
	if (macro->num_params > 0 || macro->var_args_macro == TRUE)
	{
		ReportError(err_ctx, err_code, L"Macrodefinition \"%s\" has parameters.", name_buff);
		ScanToSemicolon(err_ctx);
		return(FALSE);
	}
	else if (macro->scst_body_lex.type != ltx_number)
	{
		ReportError(err_ctx, err_code, L"Body of the macrodefinition \"%s\" is not a pure number.", name_buff);
		ScanToSemicolon(err_ctx);
		return(FALSE);
	}

	// Ensure that the number in the body of the macrodefinition is fine.
	if (CheckObjectIdValue(err_ctx, macro->scst_body_lex.num_value, 0, err_code) == FALSE)
	{
		ScanToSemicolon(err_ctx);
		return(FALSE);
	}

	// The value of the number is fine. Pick ip up.
	WORD implicit_app_id = (WORD)macro->scst_body_lex.num_value;
	if (implicit_app_id != 0)
	{
		// Ensure that explicit id is either missing or it is the same to the implicit id.
		if (app_id != 0 && app_id != implicit_app_id)
		{
			// Both ids are present and they are not the same.
			ReportError(err_ctx, err_code, L"Application Id is present both as an explicit __id(%hd) and as a macro in the macro dictionary with the body %hd.", app_id, implicit_app_id);
			ScanToSemicolon(err_ctx);
			return(FALSE);
		}

		// Accept the implicit id.
		app_id = implicit_app_id;
	}

	// Success.
	return(TRUE);
}

bool TGrammarDefinitionParser::GetNameOfTheCurrentMacro(TLexema &err_ctx, wchar_t *buffer_with_grammar_max_name_len_size)
{
	assert(m_scanner->src_cnt > 0);
	if (LEX_SRC_MACRO_OR_PBUFF_MACRO(m_scanner->src_stack[m_scanner->src_cnt-1].src_type) == TRUE)
	{
		// Current parsing context is the macro context.
		TMacroDefinition *macro = m_scanner->src_stack[m_scanner->src_cnt-1].macro_context;
		if (macro->scst_body_lex.type == ltx_number)
		{
			// Body of the current macro consists of one number only. Use name of this macro as the name
			// of the application id.
			if (CheckName(err_ctx.origin, macro->def_name, gerr_xpct_resolution_error) == TRUE)
			{
				// It is ok to use this name.
				macro->def_name.CopyToVerifiedBuffer(buffer_with_grammar_max_name_len_size);
				return(TRUE);
			}
		}
	}

	// Current context does not meet the required conditions.
	buffer_with_grammar_max_name_len_size[0] = 0;
	return(FALSE);
}

bool TGrammarDefinitionParser::CheckObjectIdValue(TLexema &err_ctx, __int64 value, WORD exception_sym, TGrammarDefnParserErrorCode err_code)
{
	// Check if passed id fits into the proper range or not.
	if (value < 0 || value > MAX_APP_ID_VALUE)
	{
		ReportError(err_ctx, err_code, L"Value of the app id (%I64d) is negative or it is bigger than %d.", value, MAX_APP_ID_VALUE);
		return(FALSE);
	}

	// Ensure that there is no object with this id.
	WORD sym = FindObjectByAppId((WORD)value);
	if (sym != 0 && sym != exception_sym)
	{
		PrEventHeader origin_loc;
		GetIntroOrigin(origin_loc, sym);
		wchar_t line_num_buff[40];
		ReportError(err_ctx, err_code, L"Object with app id %I64d is already defined on the line %s of \"%s\".",
					value, line_num_buff, PrepareSrcLocInfo(origin_loc, line_num_buff, 40));
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

WORD TGrammarDefinitionParser::RegisterNonTerminal(TLexema &lex, bool fill_rules_sect, TGrammarDefnParserErrorCode err_code)
{
	// Register this name as non terminal.
	assert(lex.type == ltx_name);

	// There is no grammar symbol with this name. Check for other cases.
	if (CheckName(lex.origin, lex.str_value, err_code) == FALSE)
	{
		// Error is already reported.
		return(0);
	}

	// Check the macro dictionary for the implicit app_id of the object. Registering non terminal differs from registering
	// other objects because its explicit app_id is not scanned yet and it is not clear if it will be found later or not.
	WORD app_id = 0;
	if (GetObjectIdFromMacroDefnsTable(lex, L"sym_", lex.str_value, app_id, err_code) == FALSE)
	{
		// Error is already reported.
		return(0);
	}

	// All checks passed. It is ok to save this symbol.
	if (fill_rules_sect == FALSE)
	{
		// Create forward declaration. Start from saving the intro area.
		if (m_src_data.m_fwd_def_non_term_areas.AppendItem(lex.origin) == FALSE)
		{
			ReportError(lex, err_code, L"Out of memory while saving non terminal forward declaration intro area.");
			return(0);
		}

		// Add symbol to the source info.
		WORD non_term = TGrammarSymbolsHelper::AddNonTerminalSymbol(m_src_data.m_fwd_def_non_terminals, lex.str_value, app_id, 0, 0);
		if (non_term == 0)
		{
			ReportError(lex, err_code, L"Out of memory while adding non terminal forward declaration to the grammar.");
			m_src_data.m_fwd_def_non_term_areas.ReleaseLastItem();
			return(0);
		}

		// Success.
		assert(m_src_data.m_fwd_def_non_term_areas.NumItems() == m_src_data.m_fwd_def_non_terminals.NumItems());
		return(non_term+(NonTermForwardDefnsBase-NonTerminalsBase));
	}
	else
	{
		// Create regular definition. Start from saving the intro area.
		if (m_src_data.m_non_term_areas.AppendItem(lex.origin) == FALSE)
		{
			ReportError(lex, err_code, L"Out of memory while saving non terminal intro area.");
			return(0);
		}

		// Reserve space for the non term completeness field.
		WORD dummy = FALSE;
		if (m_src_data.m_non_term_defns_complete.AppendItem(dummy) == FALSE)
		{
			ReportError(lex, err_code, L"Out of memory while saving non terminal props value.");
			m_src_data.m_non_term_areas.ReleaseLastItem();
			return(0);
		}

		// Add symbol to the source info.
		WORD non_term = TGrammarSymbolsHelper::AddNonTerminalSymbol(m_src_data.m_non_terminals, lex.str_value, app_id, m_rule_sects_cnt, 0);
		if (non_term == 0)
		{
			ReportError(lex, err_code, L"Out of memory while adding non terminal to the grammar.");
			m_src_data.m_non_term_areas.ReleaseLastItem();
			m_src_data.m_non_term_defns_complete.ReleaseLastItem();
			return(0);
		}

		// Success.
		assert(m_src_data.m_non_term_areas.NumItems() == m_src_data.m_non_terminals.NumItems());
		return(non_term);
	}
}

WORD TGrammarDefinitionParser::ConvertForwardDefIntoNonTerminal(PrEventHeader &ctx, WORD forward_def_sym, WORD rules_sect, TGrammarDefnParserErrorCode err_code)
{
	assert(forward_def_sym >= NonTermForwardDefnsBase && forward_def_sym < NonTermForwardDefnsBase+m_src_data.m_fwd_def_non_terminals.NumItems());
	int fwd_inx = forward_def_sym-NonTermForwardDefnsBase;
	assert(m_src_data.m_fwd_def_non_terminals[fwd_inx].symbol_name != NULL);

	// Move the intro area.
	if (m_src_data.m_non_term_areas.AppendItem(m_src_data.m_fwd_def_non_term_areas[fwd_inx]) == FALSE)
	{
		ReportError(ctx, err_code, L"Out of memory while moving the non term forward declaration intro area to the regular non terminal intro area.");
		return(0);
	}

	// Reserve space for the non term completeness field.
	WORD dummy = FALSE;
	if (m_src_data.m_non_term_defns_complete.AppendItem(dummy) == FALSE)
	{
		ReportError(ctx, err_code, L"Out of memory while saving props value while converting non term forward declaration into the regular definition.");
		m_src_data.m_non_term_areas.ReleaseLastItem();
		return(0);
	}

	// Move the main info structure. Note that in case of success this will also clear the symbol name
	// pointer in the source structure.
	if (m_src_data.m_non_terminals.AppendItem(m_src_data.m_fwd_def_non_terminals[fwd_inx]) == FALSE)
	{
		ReportError(ctx, err_code, L"Out of memory while converting non term forward declaration info into the regular non terminal.");
		m_src_data.m_non_term_areas.ReleaseLastItem();
		m_src_data.m_non_term_defns_complete.ReleaseLastItem();
		return(0);
	}

	// Set the rules section stamp.
	WORD non_term = NonTerminalsBase+m_src_data.m_non_terminals.NumItems()-1;
	m_src_data.m_non_terminals[non_term-NonTerminalsBase].rules_sect = rules_sect;

	// Fix the axioma symbol.
	if (m_axioma == forward_def_sym)
		m_axioma = non_term;

	// Fix the right hand sides in all regular source rules.
	for (int i1=0; i1<m_src_data.m_rules.NumItems(); ++i1)
	{
		for (int i2=0; i2<m_src_data.m_rules[i1].Length(); ++i2)
		{
			if (m_src_data.m_rules[i1].symbols[i2] == forward_def_sym)
				m_src_data.m_rules[i1].symbols[i2] = non_term;
		}
	}

	// Fix right hand sides in the resolution rules.
	FixNonTermForwardDefSymHier(m_xpcts_root, forward_def_sym, non_term);

	// Success.
	return(non_term);
}

void TGrammarDefinitionParser::FixNonTermForwardDefSymHier(TExpectedConflictSupp &ctx, WORD forward_def_sym, WORD non_term_sym)
{
	// Process local resolution rules.
	for (int irule=0; irule<ctx.m_resolution_rules.NumItems(); ++irule)
	{
		// Length of the right hand side in the reolution rule can be only 0 or 1.
		if (ctx.m_resolution_rules[irule].Length() > 0 && ctx.m_resolution_rules[irule].symbols[0] == forward_def_sym)
			ctx.m_resolution_rules[irule].symbols[0] = non_term_sym;
	}

	// Process nested expected conflicts if any.
	for (TListIter<TExpectedConflictSupp> iter(ctx.m_child_supps); iter; ++iter)
	{
		FixNonTermForwardDefSymHier(iter.CurrItem(), forward_def_sym, non_term_sym);
	}
}

void TGrammarDefinitionParser::SourceGrammarIntegrityCheck(TLexema &lex_ctx)
{
	assert(m_xpcts_root.m_resolution_rules.NumItems() == 0);

	// Check that all required object categories are present.
	if (m_src_data.m_symbols.NumItems() <= 1)
		ReportError(lex_ctx, gerr_symbols_missing, L"Terminal symbol definitions are missing.");
	if (m_axioma == 0)
		ReportError(lex_ctx, gerr_axioma_missing, L"Axioma definition is missing.");
	if (m_src_data.m_non_terminals.NumItems() == 0)
		ReportError(lex_ctx, gerr_non_terms_missing, L"Non terminal symbol definitions are missing.");
	if (m_src_data.m_rules.NumItems() == 0)
		ReportError(lex_ctx, gerr_rules_missing, L"Rule definitions are missing.");

	// Check terminal symbols.
	if (m_warn_on_unused_symbols == TRUE)
	{
		// Issue warnings on terminals that were never used in the right hand sides of the rules.
		for (int isym=1; isym<m_src_data.m_symbols.NumItems(); ++isym)
		{
			if (m_src_data.m_symbols[isym].num_rule_uses == 0)
			{
				wchar_t buff_40_chars[40];
				wchar_t *frame_str = (m_src_data.m_symbols[isym].sym_type_name == TRUE) ? L"\"" : L"";
				ReportWarning(m_src_data.m_symbol_areas[isym], gerr_unused_terminal,
							L"Terminal symbol %s%s%s (sym=%d) is not used in any rule.",
							frame_str, m_src_data.GetSymbolName(isym, buff_40_chars), frame_str, isym);
			}
		}
	}

	// Issue warnings on the non terminal use patterns.
	for (int ii1=0; ii1<m_src_data.m_non_terminals.NumItems(); ++ii1)
	{
		WORD non_term = NonTerminalsBase+ii1;
		TNonTerminalSymbol &non_term_info = m_src_data.m_non_terminals[ii1];

		if (non_term_info.num_lhs_rule_uses == 0)
		{
			// Curr non terminal does not have rules that define it.
			ReportError(m_src_data.m_non_term_areas[ii1], gerr_no_lhs_for_non_term,
						L"%s \"%s\" (sym=%d).",
						((non_term == m_axioma) ? L"There are no rules that define axioma" : L"Undefined non terminal"), non_term_info.symbol_name, non_term);
		}

		if (non_term != m_axioma && non_term_info.num_rhs_rule_uses == 0)
		{
			// Current non term is not an axioma and it is not used in the right hand sides of the rules.
			// This symbol is useless. Report warning because this rule will not be picked into any grammar.
			ReportWarning(m_src_data.m_non_term_areas[ii1], gerr_no_rhs_for_non_term,
						L"Non terminal \"%s\" (sym=%d) is not used in the right hand side of any rule.", non_term_info.symbol_name, non_term);
		}

		// Look for identical non terminals. Use the most straightforward approach.
		for (int ii2=ii1+1; ii2<m_src_data.m_non_terminals.NumItems(); ++ii2)
		{
			if (non_term_info.num_lhs_rule_uses != m_src_data.m_non_terminals[ii2].num_lhs_rule_uses)
				continue;

			bool identical_non_terminal = TRUE;
			WORD other_non_term = NonTerminalsBase+ii2;

			// Compare rules for these non terminals.
			for (int ir1=0; ir1 < m_src_data.m_rules.NumItems(); ++ir1)
			{
				TGrammarRule &r1 = m_src_data.m_rules[ir1];
				if (r1.non_term == non_term)
				{
					bool identical_rule_present = FALSE;
					for (int ir2=0; ir2 < m_src_data.m_rules.NumItems(); ++ir2)
					{
						TGrammarRule &r2 = m_src_data.m_rules[ir2];
						if (r2.non_term == other_non_term)
						{
							if (r1.symbols.IsEqualTo(r2.symbols) == TRUE)
							{
								// Right sides of these rules are identical.
								identical_rule_present = TRUE;
								break;
							}
						}
					}

					if (identical_rule_present == FALSE)
					{
						// Non terminals are different.
						identical_non_terminal = FALSE;
						break;
					}
				}
			}

			if (identical_non_terminal == TRUE)
			{
				// This situation is not fatal.
				ReportWarning(m_src_data.m_non_term_areas[ii1], gerr_identical_non_terms,
								L"Non terminal \"%s\" (sym=%d) is identical to \"%s\" (sym=%d).",
								non_term_info.symbol_name, non_term, m_src_data.m_non_terminals[ii2].symbol_name, other_non_term);
			}
		}
	}

	// Check if rules really follow "__seq" and "__list" restrictions of their rule non terminals. Only the source data
	// should be checked because nested grammars cannot have scanned rules for now.
	for (int irule1=0; irule1 < m_src_data.m_rules.NumItems(); ++irule1)
	{
		TGrammarRule &rule = m_src_data.m_rules[irule1];
		int rlen = rule.Length();
		TNonTermRestrType restr = m_src_data.m_non_terminals[rule.non_term-NonTerminalsBase].restr_type;

		if (restr == ntrs_seq)
		{
			//
			// Rules, that define non terminal with "__seq" restriction, should look like:
			//
			//   non_term: <sym>
			//   non_term: non_term <sym>
			//   non_term: <sym> non_term
			//
			bool rule_ok = FALSE;
			if (rlen == 1)
			{
				// Sequence initialization.
				assert(rule.symbols[0] != rule.non_term);
				rule_ok = TRUE;
			}

			if (rlen == 2)
			{
				if (rule.symbols[0] == rule.non_term && rule.symbols[1] != rule.non_term)
				{
					// Left recursion.
					rule_ok = TRUE;
				}

				if (rule.symbols[0] != rule.non_term && rule.symbols[1] == rule.non_term)
				{
					// Right recursion.
					rule_ok = TRUE;
				}
			}

			if (rule_ok == FALSE)
			{
				// Current rule does not comply to the patterns above.
				wchar_t buff_40_chars[40];
				ReportError(m_src_data.m_rule_intro_areas[irule1].rule_origin, gerr_list_seq_restr_violation,
							L"Rule with index %d (%s.%d) does not comply with its non terminal \"__seq\" restriction.",
							irule1, m_src_data.GetSymbolName(rule.non_term, buff_40_chars), TGrammarSymbolsHelper::GetRuleOffs(m_src_data.m_rules, irule1));
			}
		}

		if (restr == ntrs_list)
		{
			//
			// Rules, that define non terminal with "__list" restriction, should look like:
			//
			//   non_term: <sym>
			//   non_term: non_term <terminal> <sym>
			//   non_term: <sym> <terminal> non_term
			//
			bool rule_ok = FALSE;
			if (rlen == 1)
			{
				// List initialization.
				assert(rule.symbols[0] != rule.non_term);
				rule_ok = TRUE;
			}

			if (rlen == 3)
			{
				if (rule.symbols[0] == rule.non_term && rule.symbols[1] < NonTerminalsBase && rule.symbols[2] != rule.non_term)
				{
					// Left recursion.
					rule_ok = TRUE;
				}

				if (rule.symbols[0] != rule.non_term && rule.symbols[1] < NonTerminalsBase && rule.symbols[2] == rule.non_term)
				{
					// Right recursion.
					rule_ok = TRUE;
				}
			}

			if (rule_ok == FALSE)
			{
				// Current rule does not comply to the patterns above.
				wchar_t buff_40_chars[40];
				ReportError(m_src_data.m_rule_intro_areas[irule1].rule_origin, gerr_list_seq_restr_violation,
							L"Rule with index %d (%s.%d) does not comply with its non terminal \"__list\" restriction.",
							irule1, m_src_data.GetSymbolName(rule.non_term, buff_40_chars), TGrammarSymbolsHelper::GetRuleOffs(m_src_data.m_rules, irule1));
			}
		}
	}

	// Check that non terminals on the right hand sides of the rules belong to the same rules section or that they are declared as public.
	for (int irule2=0; irule2 < m_src_data.m_rules.NumItems(); ++irule2)
	{
		TGrammarRule &rule = m_src_data.m_rules[irule2];
		WORD rule_section = m_src_data.m_non_terminals[rule.non_term-NonTerminalsBase].rules_sect;
		assert(rule_section != 0);

		int rlen = rule.Length();
		for (int isym=0; isym<rlen; ++isym)
		{
			if (rule.symbols[isym] >= NonTerminalsBase)
			{
				TNonTerminalSymbol &nti = m_src_data.m_non_terminals[rule.symbols[isym]-NonTerminalsBase];
				if (nti.rules_sect != 0 && nti.public_symbol == FALSE && nti.rules_sect != rule_section)
				{
					wchar_t buff_40_chars1[40], buff_40_chars2[40];
					ReportError(m_src_data.m_rule_intro_areas[irule2].symbol_origins[isym], gerr_public_non_term_error,
								L"Symbol \"%s\" that is used in R%hd (%s.%d) is not public and it belongs to the different rules section.",
								m_src_data.GetSymbolName(rule.symbols[isym], buff_40_chars1), irule2, m_src_data.GetSymbolName(rule.non_term, buff_40_chars2),
								TGrammarSymbolsHelper::GetRuleOffs(m_src_data.m_rules, irule2));
				}
			}
		}
	}

	// Check expected conflict locations.
	for (int iloc=0; iloc < m_src_data.m_location_objects.NumItems(); ++iloc)
	{
		// Pick up the info.
		wchar_t *loc_name = m_src_data.m_location_objects[iloc].location_name;
		TRulePositionsArray &positions = m_src_data.m_location_objects[iloc].positions;
		if (positions.NumItems() == 0)
		{
			// This object was created with errors. Ignore it.
			continue;
		}

		// Check the object integrity. Ensure that at least one location stays at the end of the rule.
		bool final_pos_present = FALSE;
		for (int ip1=0; ip1<positions.NumItems(); ++ip1)
		{
			if (positions[ip1].isym >= m_src_data.m_rules[positions[ip1].irule].Length())
			{
				final_pos_present = TRUE;
				break;
			}
		}

		if (final_pos_present == FALSE)
		{
			// All rule positions are intermediate.
			ReportError(m_src_data.m_loc_object_intro_areas[iloc], gerr_cfct_location_markers,
						L"Conflict location object \"%s\" does not contain location markers in the final rule positions.", loc_name);
		}

		if (m_src_data.m_location_objects[iloc].num_xpct_conflicts <= 0 && m_warn_on_unused_loc_markers == TRUE)
		{
			// Expected conflict inx is missing.
			ReportWarning(m_src_data.m_loc_object_intro_areas[iloc], gerr_cfct_location_markers,
						L"Conflict location object \"%s\" does not belong to any expected grammar conflict.", loc_name);
		}
	}
}

// /\/\/\/\/\/\/\/\/\/\/\/\ Post scanning processing /\/\/\/\/\/\/\/\/\/\/\/\/\/\

void TGrammarDefinitionParser::DoThePostScanningWork()
{
	// Setup the intermediate proc result.
	m_src_props.processing_result = grpr_syntax_errors;
	m_dest_grammar.SetProcessingResultHier(m_src_props.processing_result);

	// Check point 1.
	if (m_scanner->GetErrorsCount() > 0)
	{
		// Check point 1 failed.
		ReportConversionWarning(gerr_bld_tabs_skipped, L"Grammar preparation and conversion was skipped because of the scanning errors.");
		return;
	}

	// Propagate scanned information from the source data to the root grammar and to the nested grammars.
	m_scanner->Console().SetMajorStatus(L"Preparing grammars ...");
	PrepareGrammarsHier(m_dest_grammar, m_xpcts_root);

	// Check for unused rules.
	for (int irule=0; irule<m_src_data.m_rules.NumItems(); ++irule)
	{
		if (m_src_data.m_rule_intro_areas[irule].rule_propagated == FALSE)
		{
			wchar_t buff_40_chars[40];
			ReportConversionWarning(gerr_unused_rule,
									L"Source rule with index %d (%s.%d) is not used in any grammar.",
									irule, m_src_data.GetSymbolName(m_src_data.m_rules[irule].non_term, buff_40_chars),
									TGrammarSymbolsHelper::GetRuleOffs(m_src_data.m_rules, irule));
		}
	}

	if (m_build_parsing_table == TRUE)
	{
		// Check point 2.
		if (m_scanner->GetErrorsCount() > 0)
		{
			// Check point 2 failed.
			ReportConversionWarning(gerr_bld_tabs_skipped, L"Grammar conversion was skipped because of the syntax errors.");
			return;
		}

		// Setup the intermediate proc result.
		m_src_props.processing_result = grpr_conversion_errors;
		m_dest_grammar.SetProcessingResultHier(m_src_props.processing_result);

		// Process grammar rules in all grammars.
		ConvertSingleGrammar(m_dest_grammar);
		ConvertGrammarsHier(m_dest_grammar);

		// Check point 3.
		if (m_scanner->GetErrorsCount() > 0)
		{
			// Check point 3 failed.
			ReportConversionWarning(gerr_bld_tabs_skipped, L"Post processing was skipped because of the grammar conversion errors.");
			return;
		}

		if (m_scanner->GetCurrTraceMode() & gdtr_steps)
		{
			m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"----------------------- Post processing -----------------------");
		}

		// Process grammar conflicts and conflict resolution arrays.
		m_scanner->Console().SetMajorStatus(L"Post processing grammars ...");
		PostProcessGrammarsHier(m_dest_grammar);

		if (m_scanner->GetCurrTraceMode() & gdtr_steps)
		{
			wchar_t buffer80[80];
			if (m_dest_grammar.GetNumUnexpectedConflictsHier() == 0)
				wcscpy(buffer80, L"NONE");
			else swprintf(buffer80, 80, L"%d", m_dest_grammar.GetNumUnexpectedConflictsHier());

			m_scanner->TraceEvent(NULL, TGDP_GENERAL, L">>");
			m_scanner->TraceEvent(NULL, TGDP_GENERAL, L">>  Unexpected conflicts: %s.", buffer80);
			m_scanner->TraceEvent(NULL, TGDP_GENERAL, L">>");
		}
	}

	// Check point 4.
	if (m_scanner->GetErrorsCount() == 0)
	{
		// Grammar conversion succeeded.
		m_src_props.processing_result = grpr_full_success;
		m_dest_grammar.SetProcessingResultHier(m_src_props.processing_result);
	}
}

void TGrammarDefinitionParser::PrepareGrammarsHier(TGrammar &grm, TExpectedConflictSupp &grm_supp)
{
	// Process the passed grammar.
	PrepareSingleGrammar(grm, grm_supp);

	// Process resolution of expected conflicts if any.
	int ixpct = 0;
	for (TListIter<TExpectedConflictSupp> iter(grm_supp.m_child_supps); iter; ++iter, ++ixpct)
	{
		TExpectedConflictSupp &csupp = iter.CurrItem();
		if (csupp.m_resolution_rules.NumItems() > 0)
		{
			// Current expected confict has resolution rules. Allocate grammar for these rules.
			TGrammar *nested_grammar = new TGrammar();
			if (nested_grammar == NULL)
			{
				ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while allocating the nested grammar.");
				return;
			}

			// Reset this grammar and link it into the tree.
			nested_grammar->Clear();
			nested_grammar->parent_grammar = &grm;
			nested_grammar->parent_xpct_inx = ixpct;
			grm.xpct_conflicts[ixpct].nested_grammar = nested_grammar;

			// Prepare this grammar and all its children.
			PrepareGrammarsHier(*nested_grammar, csupp);
		}
	}
}

void TGrammarDefinitionParser::PrepareSingleGrammar(TGrammar &grm, TExpectedConflictSupp &grm_supp)
{
	// Copy the general info.
	assert(m_axioma != 0);
	grm.grm_props = m_src_props;
	grm.axioma_ident = m_axioma;

	// Create masks for tracking transformation of the major object types. Note that mask contain value 0xFFFF in
	// every field after the preparation.
	m_sym_buffer.SetNumItems(0);
	TSymbolsArray symbols_mask, non_terms_mask, rules_mask;
	bool syms_msk_ok = PrepareHelperMask(symbols_mask, m_src_data.m_symbols.NumItems());
	bool nterms_msk_ok = PrepareHelperMask(non_terms_mask, m_src_data.m_non_terminals.NumItems());
	bool rules_msk_ok = PrepareHelperMask(rules_mask, m_src_data.m_rules.NumItems());
	if (syms_msk_ok == FALSE || nterms_msk_ok == FALSE || rules_msk_ok == FALSE)
	{
		// Error is already reported.
		return;
	}

	// Mark EOF and axioma as used symbols. This will force duplicating them into the dest grammar.
	symbols_mask[0] = TRUE;
	non_terms_mask[m_axioma-NonTerminalsBase] = TRUE;

	// Create initial list of the used non terminals.
	if (grm.parent_grammar == NULL)
	{
		// This is the root grammar. This grammar should include all rules that are accessible from the axioma
		// either directly or indirectly. Include only the axioma symbol onto the list to process. The code below will
		// discover all other required rules.
		m_sym_buffer.AppendItem(m_axioma);
	}
	else
	{
		// Duplicate resolution rules from the rules supp to the grammar.
		assert(grm_supp.m_resolution_rules.NumItems() > 0);
		for (int irule=0; irule<grm_supp.m_resolution_rules.NumItems(); ++irule)
		{
			TGrammarRule &rule = grm_supp.m_resolution_rules[irule];

			// Length of the right hand side can be only zero or one here. This means that this operation cannot fail.
			TSymbolsArray rule_symbols_copy;
			rule_symbols_copy.AppendItems(rule.symbols.DataPtr(), rule.Length());

			// Add rule to the curr grammar.
			if (TGrammarSymbolsHelper::AddGrammarRule(grm.rules, m_axioma, rule_symbols_copy, rule.rule_app_id, rule.rule_app_id_name, rule.non_term_event_id, rule.rule_body_event_id) == FALSE)
			{
				ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while duplicating the resolution rule.");
				return;
			}
		}

		// In the nested grammar it is necessaary to include rules, that define symbols, that are used in the resolution rules.
		// Axioma is not needed. Iterate resolution rules. All symbols in those rules need processing.
		for (TRulesIterator iter1(grm); iter1; ++iter1)
		{
			TGrammarRule &rule = iter1.CurrRule();
			if (rule.Length() == 0)
				continue;

			assert(rule.Length() == 1);
			WORD first_sym = rule.symbols[0];
			if (first_sym >= NonTerminalsBase)
			{
				assert(first_sym != m_axioma);

				// The symbol is non terminal. Request copying rules that define this non terminal from the source data.
				if (m_sym_buffer.AppendItem(first_sym) == FALSE)
				{
					ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while creating initial list of non terminals to process.");
					return;
				}

				assert(non_terms_mask[first_sym-NonTerminalsBase] == 0xFFFF);
				non_terms_mask[first_sym-NonTerminalsBase] = TRUE;
			}
			else
			{
				// Mark the fact that this terminal symbol was used.
				assert(symbols_mask[first_sym] == 0xFFFF);
				symbols_mask[first_sym] = TRUE;
			}
		}
	}

	// Process the "list of non terminals to process". Note that this list can grow during processing.
	for (int isym1=0; isym1<m_sym_buffer.NumItems(); ++isym1)
	{
		// Pick up the next non terminal that requires processing.
		WORD sym = m_sym_buffer[isym1];
		bool something_propagared = FALSE;

		// Copy rules from the root grammar that define this symbol.
		for (int irule2=0; irule2<m_src_data.m_rules.NumItems(); ++irule2)
		{
			TGrammarRule &rule = m_src_data.m_rules[irule2];
			if (rule.non_term != sym)
				continue;

			// At least one rule for the current non terminal is present.
			something_propagared = TRUE;

			for (int i=0; i<rule.Length(); ++i)
			{
				WORD rule_sym = rule.symbols[i];
				if (rule_sym >= NonTerminalsBase)
				{
					if (rule_sym == m_axioma)
					{
						// This is an attempt to use axioma in the right hand side of the rule.
						if (grm.parent_grammar == NULL)
						{
							ReportConversionError(gerr_chld_grammar_prep_error, L"This parser does not allow using axioma in the body of the grammar rules.");
						}
						else
						{
							ReportConversionError(gerr_chld_grammar_prep_error, L"Rules of the nested grammar cannot mention axioma of the main grammar.");
						}

						// Issue one more error with explanation.
						wchar_t line_num_buff[40];
						wchar_t *file_name = PrepareSrcLocInfo(m_src_data.m_rule_intro_areas[irule2].rule_origin, line_num_buff, 40);
						ReportConversionError(gerr_chld_grammar_prep_error, L"Check axioma use on the line %s of \"%s\".", line_num_buff, file_name);
						return;
					}

					if (non_terms_mask[rule_sym-NonTerminalsBase] == 0xFFFF)
					{
						// Rules for this symbol should be processed also.
						if (m_sym_buffer.AppendItem(rule_sym) == FALSE)
						{
							ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while adding more symbols to the list of non terminals to process.");
							return;
						}

						non_terms_mask[rule_sym-NonTerminalsBase] = TRUE;
					}
				}
				else
				{
					// Store the fact that current terminal symbol was used in the rules.
					symbols_mask[rule_sym] = TRUE;
				}
			}

			// Mark this rule in the rules mask for future processing.
			rules_mask[irule2] = TRUE;
		}

		if (something_propagared == FALSE)
		{
			// Rules for the current non terminal are missing. Do not stop the processing because this is an app level error.
			wchar_t buff_40_chars[40];
			ReportConversionError(gerr_chld_grammar_prep_error, L"There are no rules that define the non terminal symbol \"%s\" (%d).",
								m_src_data.GetSymbolName(sym, buff_40_chars), sym);
		}
	}

	//
	//  It is clear now WHAT symbols, non terminals and rules should be copied from the source data to the current
	//  dest grammar. The ignore lex records and the error lex records will be duplicated entirely without checking.
	//

	// Generate terminal symbol definitions.
	for (int isym2=0; isym2<m_src_data.m_symbols.NumItems(); ++isym2)
	{
		if (symbols_mask[isym2] != TRUE)
			continue;

		// Add symbol definition.
		TTerminalSymbol &info = m_src_data.m_symbols[isym2];
		TStrPtrInfo info_name_ptr;
		info_name_ptr.SetData(info.name_value);
		if (TGrammarSymbolsHelper::AddTerminalSymbol(grm.symbols, info.sym_type_name, info.char_value, info_name_ptr, info.app_id, info.lex_type, info.check_subt, info.lex_subt, info.event_id) == 0)
		{
			// Check for special case of adding EOF symbol that has value 0.
			if (isym2 != 0 || grm.NumTerminals() == 0)
			{
				ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while creating local terminal symbol description.");
				return;
			}
		}

		// Store index of the original terminal symbol in the supp structure for possible future use.
		WORD isym_as_word = isym2;
		if (grm_supp.m_terminal_symbols_bk_refs.AppendItem(isym_as_word) == FALSE)
		{
			ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while storing the back ref of the propagated terminal symbol.");
			return;
		}

		// Store the new symbol value of this terminal in the mask.
		symbols_mask[isym2] = (WORD)(grm.NumTerminals()-1);
	}

	// Ensure that at least one terminal was registered. This may happen in case if all rules of the grammar contain non terminals only.
	if (grm.NumTerminals() == 0)
	{
		// Write error and continue processing.
		ReportConversionError(gerr_chld_grammar_prep_error, L"Root or nested grammar rules does not contain terminal symbols.");
	}

	// Setup ignore lex and error lex arrays.
	if (grm.parent_grammar == NULL)
	{
		//
		// This is the root grammar.
		//

		// Duplicate the ignore lex records.
		for (int iign=0; iign<m_src_data.m_ignore_lex.NumItems(); ++iign)
		{
			TIgnoreLexRecord ign_rec = m_src_data.m_ignore_lex[iign];
			if (TGrammarSymbolsHelper::AddIgnoreLexRecord(grm.ignore_lex, ign_rec.lex_type, ign_rec.check_subt, ign_rec.lex_subt) == FALSE)
			{
				ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while storing ignore lex data.");
				return;
			}
		}

		// Error records should be added one by one.
		for (int ierr=0; ierr<m_src_data.m_error_lex.NumItems(); ++ierr)
		{
			TErrorLexRecord &erec = m_src_data.m_error_lex[ierr];
			TStrPtrInfo erec_err_msg_ptr = { erec.error_message, wcslen(erec.error_message) };
			if (TGrammarSymbolsHelper::AddErrorLexRecord(grm.error_lex, erec.lex_type, erec.check_subt, erec.lex_subt, erec_err_msg_ptr) == FALSE)
			{
				ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while storing error lex data.");
				return;
			}
		}
	}
	else
	{
		// This is a nested grammar. Use data from the root grammar in the read only mode.
		grm.ignore_lex.SetupReadOnlyData(m_dest_grammar.ignore_lex.DataPtr(), m_dest_grammar.ignore_lex.NumItems());
		grm.error_lex.SetupReadOnlyData(m_dest_grammar.error_lex.DataPtr(), m_dest_grammar.error_lex.NumItems());
	}

	// Generate non terminal definitions.
	for (int isym3=0; isym3<m_src_data.m_non_terminals.NumItems(); ++isym3)
	{
		if (non_terms_mask[isym3] != TRUE)
			continue;

		// Add non terminal definition.
		TNonTerminalSymbol &info = m_src_data.m_non_terminals[isym3];
		TStrPtrInfo info_symbol_name_ptr = { info.symbol_name, wcslen(info.symbol_name) };
		if (TGrammarSymbolsHelper::AddNonTerminalSymbol(grm.non_terminals, info_symbol_name_ptr, info.app_id, info.rules_sect, info.event_id) == 0)
		{
			ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while creating local non terminal definition.");
			return;
		}

		// Duplicate other props.
		TNonTerminalSymbol &new_sym_info = grm.non_terminals[grm.NumNonTerminals()-1];
		new_sym_info.public_symbol = info.public_symbol;
		new_sym_info.restr_type = info.restr_type;

		// Store index of the original non terminal in the supp structure for possible future use.
		WORD isym_as_word = isym3;
		if (grm_supp.m_non_terminals_bk_refs.AppendItem(isym_as_word) == FALSE)
		{
			ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while storing the back ref of the propagated non terminal.");
			return;
		}

		// Store new symbol value of this non terminal in the mask.
		non_terms_mask[isym3] = (WORD)(NonTerminalsBase+grm.NumNonTerminals()-1);
	}

	// Generate the rule definitions.
	for (int irule=0; irule<m_src_data.m_rules.NumItems(); ++irule)
	{
		if (rules_mask[irule] != TRUE)
			continue;

		// Current source rule should be propagated into the current grammar.
		TGrammarRule &rule = m_src_data.m_rules[irule];

		// Generate copy of the rule right hand side.
		TSymbolsArray rule_symbols_copy;
		if (rule_symbols_copy.AppendItems(rule.symbols.DataPtr(), rule.Length()) == FALSE)
		{
			ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while duplicating rule symbols to root/nested nested grammar.");
			return;
		}

		// Add rule to the curr grammar.
		if (TGrammarSymbolsHelper::AddGrammarRule(grm.rules, rule.non_term, rule_symbols_copy, rule.rule_app_id, rule.rule_app_id_name, rule.non_term_event_id, rule.rule_body_event_id) == FALSE)
		{
			ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while adding copy of the rule to root/nested grammar.");
			return;
		}

		// Store index of the original rule in the supp structure for possible future use.
		WORD irule_as_word = irule;
		if (grm_supp.m_generated_rules_bk_refs.AppendItem(irule_as_word) == FALSE)
		{
			ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while storing the back ref of the propagated rule.");
			return;
		}

		// Mark original rule as propagated.
		m_src_data.m_rule_intro_areas[irule].rule_propagated = TRUE;
		rules_mask[irule] = (WORD)(grm.NumRules()-1);
	}

	// Generated rules are currenlty using symbol values from the source data. Update values
	// of the symbols and collect the symbol use statistics.
	for (TRulesIterator iter3(grm); iter3; ++iter3)
	{
		TGrammarRule &rule = iter3.CurrRule();
		rule.non_term = non_terms_mask[rule.non_term-NonTerminalsBase];
		grm.non_terminals[rule.non_term-NonTerminalsBase].num_lhs_rule_uses++;

		for (int i=0; i<rule.Length(); ++i)
		{
			WORD src_sym = rule.symbols[i];
			WORD dst_sym = (src_sym < NonTerminalsBase) ? symbols_mask[src_sym] : non_terms_mask[src_sym-NonTerminalsBase];
			rule.symbols[i] = dst_sym;

			// Bump using this symbol in the current grammar.
			if (dst_sym < NonTerminalsBase)
			{
				grm.symbols[dst_sym].num_rule_uses++;
			}
			else
			{
				grm.non_terminals[dst_sym-NonTerminalsBase].num_rhs_rule_uses++;
			}
		}
	}

	int ixpct = 0;
	for (TListIter<TExpectedConflictSupp> iter(grm_supp.m_child_supps); iter; ++iter, ++ixpct)
	{
		// Pick up the conflict descriptor and create its copy in the passed grammar.
		TExpectedConflictSupp &csupp = iter.CurrItem();
		TExpectedGrammarConflict &xpct_proto = csupp.m_xpct_proto;
		if (TGrammarSymbolsHelper::AddExpectedConflict(grm.xpct_conflicts, xpct_proto.xpct_conflict_name, xpct_proto.xpct_conflict_app_id, xpct_proto.xpct_conflict_locations.NumItems(), xpct_proto.event_id) == FALSE)
		{
			ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while adding expected conflict descriptor.");
			return;
		}

		// Process the list of location object references. Copy referenced loc objects from the source data to the local
		// grammar. Fill in the indexes of the loc objects in the local grammar into the expected conflict definitions.
		TExpectedGrammarConflict &xcf = grm.xpct_conflicts[ixpct];
		for (int iloc=0; iloc<csupp.m_xpct_proto.xpct_conflict_locations.NumItems(); ++iloc)
		{
			WORD orig_loc_inx = csupp.m_xpct_proto.xpct_conflict_locations[iloc];
			TExpectedConflictLocation &orig_loc = m_src_data.m_location_objects[orig_loc_inx];

			// Check, maybe this loc object is already copied to the current grammar.
			TStrPtrInfo loc_object_name_ptr = { orig_loc.location_name, wcslen(orig_loc.location_name) };
			int local_inx = TGrammarSymbolsHelper::FindLocationObject(grm.location_objects, loc_object_name_ptr);
			if (local_inx < 0)
			{
				// This object should be copied from the source data to the local grammar.
				WORD loc_grm_act_sym = symbols_mask[orig_loc.action_symbol];
				TExpectedConflictLocation new_loc = { TGrammar::StrDupe(orig_loc.location_name), loc_grm_act_sym, TRulePositionsArray(), 0, TSymbolsArray(), };
				if (new_loc.location_name == NULL)
				{
					ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while propagating location object name.");
					return;
				}

				// Create copy of positions with modified rule index and modified action sym.
				for (int ipos=0; ipos<orig_loc.positions.NumItems(); ++ipos)
				{
					TRulePosition pos = orig_loc.positions[ipos];
					pos.irule = rules_mask[pos.irule];
					if (pos.irule == 0xFFFF)
					{
						ReportConversionError(gerr_chld_grammar_prep_error, L"Location object (%s) marks position in the rule, that is not propagated into the current grammar.", orig_loc.location_name);
						new_loc.ReleaseObject();
						return;
					}

					pos.action_sym = (pos.action_sym == 0xFFFF) ? 0xFFFF : symbols_mask[pos.action_sym];
					if (new_loc.positions.AppendItem(pos) == FALSE)
					{
						ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while propagating the location object position.");
						new_loc.ReleaseObject();
						return;
					}
				}

				// Create new conflict location object.
				if (grm.location_objects.AppendItem(new_loc) == FALSE)
				{
					ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while propagating the conflict location object.");
					new_loc.ReleaseObject();
					return;
				}

				local_inx = grm.location_objects.NumItems()-1;
			}

			xcf.xpct_conflict_locations[iloc] = local_inx;
			grm.location_objects[local_inx].num_xpct_conflicts++;
		}

		// Create the resolution data. While doing this, copy referenced nested processing result objects into the local grammar.
		for (int ires=0; ires < csupp.m_xpct_proto.resolution.NumItems(); ++ires)
		{
			TSymbolsArray &appIds = csupp.m_xpct_proto.resolution[ires];
			for (int inx=0; inx < appIds.NumItems(); ++inx)
			{
				WORD appId = appIds[inx];
				WORD sym = FindObjectByAppId(appId);
				if (sym >= NestedResultsBase && sym < XpctConflictsBase)
				{
					// This app id represents the nested result object. Check, if it is present in the local grammar or not.
					int localInx = grm.FindNestedResultByAppId(appId);
					if (localInx < 0)
					{
						// This object is missing in the local list. Create new nested processing result object.
						TNestedProcessingResult &nest_res = m_src_data.m_nested_processing_results[sym-NestedResultsBase];
						if (TGrammarSymbolsHelper::AddNestedProcResult(grm.nested_processing_results, appId, nest_res.nest_res_name, nest_res.event_id) == FALSE)
						{
							ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while saving nested processing result object.");
							return;
						}
					}
				}
			}

			// Create copy of this resolution vector.
			TSymbolsArray appIdsCopy;
			if (appIdsCopy.AppendItems(appIds.DataPtr(), appIds.NumItems()) == FALSE)
			{
				ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while duplicating the resolution vector.");
				return;
			}

			if (xcf.resolution.AppendItem(appIdsCopy) == FALSE)
			{
				ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while appending the resolution vector.");
				return;
			}
		}
	}

	// Compile terminal symbol, ignore lex and error lex defns into the mapper that will map lexemas to terminal symbols.
	PrepareLexToSymConverter(grm);

	// List of rules is ready. It is possible to determine now what non terminals can be resolved to notihing.
	PrepareNonTermTransparencyFields(grm);

	// Emit basic report about the prepared grammar.
	TFileNameBuffer file_name_buffer;
	TNameBuffer grammar_name_buffer;
	if (GenerateGrammarNameForReporting(file_name_buffer, grammar_name_buffer, grm) == TRUE)
	{
		GenerateBasicReport(file_name_buffer, grammar_name_buffer, grm.GetGrammarIndex(), grm.symbols, grm.non_terminals, grm.rules);
	}
	else
	{
		ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while generating the grammar name.");
	}
}

bool TGrammarDefinitionParser::PrepareHelperMask(TSymbolsArray &mask, int num_items_needed)
{
	// Allocate space.
	if (mask.ReserveTotalBufferSpace(num_items_needed) == FALSE)
	{
		ReportConversionError(gerr_chld_grammar_prep_error, L"Out of memory while allocating space for the helper mask. The requested length is: %d.", num_items_needed);
		return(FALSE);
	}

	// Clear the mask.
	for (int inx=0; inx<num_items_needed; ++inx)
		mask[inx] = 0xFFFF;
	return(TRUE);
}

void TGrammarDefinitionParser::PrepareLexToSymConverter(TGrammar &grm)
{
	// Fill in table with either the error code for the root grammar or wih EOF symbol for the nested grammars.
	WORD sym_filler = (grm.parent_grammar != NULL) ? 0 : 0xFFFF;
	grm.lex_to_sym_conv.Clear(sym_filler);

	// Iterate all terminals. First item in the table is the EOF character.
	for (int i1=0; i1<grm.NumTerminals(); ++i1)
	{
		TTerminalSymbol &sym = grm.symbols[i1];
		PrepareLexToSymConverterSlot(grm, sym.lex_type, sym.check_subt, sym.lex_subt, TerminalSymbolsBase+i1);
	}

	// Iterate all lexema types that should be ignored.
	for (int i2=0; i2<grm.ignore_lex.NumItems(); ++i2)
	{
		TIgnoreLexRecord &ign = grm.ignore_lex[i2];
		PrepareLexToSymConverterSlot(grm, ign.lex_type, ign.check_subt, ign.lex_subt, IgnoreLexTypeBase+i2);
	}

	// Iterate all lexema types that should not appear in the code of the target language.
	for (int i3=0; i3<grm.error_lex.NumItems(); ++i3)
	{
		TErrorLexRecord &err = grm.error_lex[i3];
		PrepareLexToSymConverterSlot(grm, err.lex_type, err.check_subt, err.lex_subt, ErrorLexTypeBase+i3);
	}
}

void TGrammarDefinitionParser::PrepareLexToSymConverterSlot(TGrammar &grm, TLexemaType lext, bool check_subt, TLexSubtype subt, WORD sym_value)
{
	assert(sym_value < TLexToSymConverter::lt_check_subtype);
	WORD &mti = grm.lex_to_sym_conv.m_lex_data[lext];

	if (check_subt == FALSE)
	{
		// This lexema type should be completely unassigned.
		assert(mti == TLexToSymConverter::lt_eof || mti == TLexToSymConverter::lt_sym_empty);
		mti = sym_value;
	}
	else
	{
		// Check the state of the main type.
		if (mti == TLexToSymConverter::lt_eof || mti == TLexToSymConverter::lt_sym_empty)
		{
			// This lex type has no assignments at all. Change main state of this lexema type
			// to "partially assigned".
			mti = TLexToSymConverter::lt_check_subtype;
		}
		else
		{
			// Ensure that state is "partially assigned".
			assert(mti == TLexToSymConverter::lt_check_subtype);
		}

		// Pick up the right descriptor for the passed subtype.
		WORD *subt_info = NULL;
		switch (lext)
		{
			case ltx_comment:
					subt_info = &grm.lex_to_sym_conv.m_ltx_comment_data[subt.comment_type];
					break;

			case ltx_number:
					subt_info = &grm.lex_to_sym_conv.m_ltx_number_data[subt.number_type];
					break;

			case ltx_floating_point:
					subt_info = &grm.lex_to_sym_conv.m_ltx_floating_point_data[subt.floating_point_type];
					break;

			case ltx_charconst:
					subt_info = &grm.lex_to_sym_conv.m_ltx_charconst_data[subt.charconst_type];
					break;

			case ltx_string:
					subt_info = &grm.lex_to_sym_conv.m_ltx_string_data[subt.string_type];
					break;

			case ltx_keyword:
					subt_info = &grm.lex_to_sym_conv.m_ltx_keyword_data[subt.subtype];
					break;

			case ltx_name:
					subt_info = &grm.lex_to_sym_conv.m_ltx_name_data[subt.subtype & (TLexToSymConverter::lt_num_name_types-1)];
					break;

			case ltx_eol:
					subt_info = &grm.lex_to_sym_conv.m_lex_endofline_data[subt.endofline_type];
					break;

			default:
				assert(FALSE);
				return;
		}

		// Check the state of descriptor.
		assert(*subt_info == TLexToSymConverter::lt_eof || *subt_info == TLexToSymConverter::lt_sym_empty);

		// Assign processing for this subtype.
		*subt_info = sym_value;
	}
}

void TGrammarDefinitionParser::PrepareNonTermTransparencyFields(TGrammar &grm)
{
	// By default all non terminals are solid.
	bool something_modified = TRUE;
	while (something_modified == TRUE)
	{
		something_modified = FALSE;
		for (TRulesIterator iter(grm); iter; ++iter)
		{
			TGrammarRule &rule = iter.CurrRule();
			if (grm.non_terminals[rule.non_term-NonTerminalsBase].solid_symbol == TRUE)
			{
				// Non terminal symbol of the curr rule is still solid. Check if current rule can make symbol transparent of not.
				if (rule.Length() == 0 || grm.IsEscapeAbleSeq(rule.symbols.DataPtr(), rule.Length()) == TRUE)
				{
					// Mark non terminal as escapable.
					grm.non_terminals[rule.non_term-NonTerminalsBase].solid_symbol = FALSE;
					something_modified = TRUE;
				}
			}
		}
	}
}

// /\/\/\/\/\/\/\/\/\/\  Grammar conversion methods  /\/\/\/\/\/\/\/\/\/\/\

void TGrammarDefinitionParser::ConvertGrammarsHier(TGrammar &grm)
{
	// Convert direct children first. This order of processing is needed to process grammars in the order
	// of increasing the grammar index.
	for (int ixpct1=0; ixpct1<grm.xpct_conflicts.NumItems(); ++ixpct1)
	{
		TGrammar *ngrm1 = grm.xpct_conflicts[ixpct1].nested_grammar;
		if (ngrm1 != NULL)
			ConvertSingleGrammar(*ngrm1);
	}

	// Convert the indirect children.
	for (int ixpct2=0; ixpct2<grm.xpct_conflicts.NumItems(); ++ixpct2)
	{
		TGrammar *ngrm2 = grm.xpct_conflicts[ixpct2].nested_grammar;
		if (ngrm2 != NULL)
			ConvertGrammarsHier(*ngrm2);
	}
}

void TGrammarDefinitionParser::ConvertSingleGrammar(TGrammar &grm)
{
	// Store the pointer to the grammar under conversion.
	m_conv_grm = &grm;

	// Update the console.
	int igrm = grm.GetGrammarIndex();
	int ngrms = grm.GetNumGrammars();
	wchar_t msg_cons[80];
	swprintf(msg_cons, 80, L"Converting grammar  %d of %d ...", igrm+1, ngrms);
	m_scanner->Console().SetMajorStatus(msg_cons);

	TFileNameBuffer file_name_buffer;
	TNameBuffer grammar_name_buffer;
	if (GenerateGrammarNameForReporting(file_name_buffer, grammar_name_buffer, grm) == FALSE)
	{
		ReportConversionError(gerr_no_mem_for_work_buffer, L"Out of memory while generating the grammar name.");
		return;
	}

	if (m_scanner->GetCurrTraceMode() & (gdtr_reports | gdtr_steps))
	{
		// Report the start of conversion.
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"---------------------------------------------------------------------");
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"Converting: %s   (%d of %d)", grammar_name_buffer.DataPtr(), igrm+1, ngrms);
	}

	// Set up the conversion method.
	m_slr_method = FALSE;

	// Initiate FIRSTs and FOLLOWs matrix. This is done here for both conversion methods because after splitting
	// the source and root grammars all grammars become generated and this also means that all rules are active.
	if (m_scanner->GetCurrTraceMode() & gdtr_steps)
	{
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"Generating FIRSTs and FOLLOWs ...");
	}

	if (grm.GenerateFirstsAndFollows(m_firsts_and_follows) == FALSE)
	{
		ReportConversionError(gerr_no_mem_for_followers_list, L"Low on memory while creating matrix of non terminal followers.");
		return;
	}

	// Check the FIRSTs and FOLLOWs matrix.
	for (int irow=0; irow<grm.NumNonTerminals(); ++irow)
	{
		int num_firsts = 0, num_follows = 0;
		WORD *row_data = m_firsts_and_follows.GetRowPtr(irow);
		for (int iclmn=0; iclmn<m_firsts_and_follows.GetWidth(); ++iclmn)
		{
			if ((row_data[iclmn] & FUN_FIRST_MASK) != 0)
				num_firsts++;
			if ((row_data[iclmn] & FUN_FOLLOWS_MASK) != 0)
				num_follows++;
		}

		if (num_firsts == 0)
		{
			ReportConversionWarning(gerr_bad_firsts_follows_set, L"Non terminal \"%s\" (sym=%d) does not start with any terminal symbol and it cannot be empty.",
									grm.non_terminals[irow].symbol_name, NonTerminalsBase+irow);
		}

		if (num_follows == 0)
		{
			ReportConversionWarning(gerr_bad_firsts_follows_set, L"Non terminal \"%s\" (sym=%d) is not followed by any terminal symbol and EOF.",
									grm.non_terminals[irow].symbol_name, NonTerminalsBase+irow);
		}
	}

	// Prepare the support tables.
#ifdef WANT_SLR_SUPPORT
	if (m_slr_method == TRUE)
	{
		// Create SLR state propagation info.
		if (GeneratePropagationInfo(m_slr_propagation_info) == FALSE)
		{
			// Error is already reported.
			return;
		}
	}
	else
#endif
	{
		// Precalculate first sets for the rule right hands without their first symbol. This means that only rules
		// with at least two symbols in their right hand sides will have non emty sets.
		if (GenerateSpecialFirstSetsForRules(m_lr_rule_firsts) == FALSE)
		{
			// Error is already reported.
			return;
		}

		// Allocate slots in the work set.
		if (m_lr_work_set.ReserveTotalBufferSpace(grm.symbols.NumItems()) == FALSE)
		{
			ReportConversionError(gerr_no_mem_for_work_buffer, L"Out of memory while allocating work buffer. Required length: %d.", grm.symbols.NumItems());
			return;
		}
	}

	//
	//    Assign parsing states and check the results.
	//  ------------------------------------------
	//
	AssignParsingStates();
	if (m_scanner->GetErrorsCount() != 0)
		return;

	// Report results of the state assignment.
	if (m_scanner->GetCurrTraceMode() & gdtr_reports)
	{
		GenerateFollowersAndParsingStatesReport(grm, m_firsts_and_follows);
	}

	//
	//    Fill in the reduce actions.
	//  --------------------------
	//
	AssignResolveActions();
	if (m_scanner->GetErrorsCount() != 0)
		return;

	// Second std report.
	wchar_t buff[80];
	swprintf(buff, 80, L"%d", grm.NumConflicts());
	ReportMessage(gerr_conversion_rprt, L"Num used rules: %d", grm.NumRules());
	ReportMessage(gerr_conversion_rprt, L"Num analysis states: %d", grm.NumParsingStates());
	ReportMessage(gerr_conversion_rprt, L"Num grammar conflicts: %s", (grm.NumConflicts() != 0) ? buff : L"NONE");

	// Print final message.
	assert(m_scanner->GetErrorsCount() == 0);
	ReportMessage(gerr_conversion_rprt, L"Grammar conversion succeeded.");
}

WORD TGrammarDefinitionParser::AllocateNewParsingState()
{
	// Count of allocated states is stored in the grammar.
	WORD new_stt = m_conv_grm->AllocAnalysisState();
	if (new_stt == 0xFFFF)
	{
		ReportConversionError(gerr_no_mem_for_analysis_table,
							L"Out of memory while allocating/reallocating analysis matrix (%d states by %d symbols).",
							m_conv_grm->analysis_table.GetRowsUsed(), m_conv_grm->NumTerminals()+m_conv_grm->NumNonTerminals());
	}

	return(new_stt);
}

void TGrammarDefinitionParser::AssignParsingStates()
{
	if (m_scanner->GetCurrTraceMode() & gdtr_steps)
	{
		// Report start of the process.
		wchar_t buffer[80];
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"%s: AssignParsingStates(%s).",
							FormatDateTime(CurrDateTime(), buffer, 80, FALSE), (m_slr_method == TRUE) ? L"SLR" : L"LR");
	}

	//
	// Set of parsing states is called here "set of sets" because each parsing state is represented by
	// the set of rule positions where this parsing state appears. Rule position is tuple of eiter two or
	// three elements:
	//
	//      SLR:	<irule, ipos>
	//      LR:	<irule, ipos, action_symbol>
	//
	// Set of rule positions is called here simply set. Index of this set in the "set of sets" is called
	// parsing state. This index is also used as an index into the actions table.
	//

	// Release old parsing states and old analyis matrix if any.
	m_conv_grm->parsing_states.ReleaseObject();
	m_conv_grm->analysis_table.ReleaseTable();

	// Allocate initial part of analysis matrix.
	WORD initial_state = AllocateNewParsingState();
	if (initial_state == 0xFFFF)
	{
		// Memory error. It should be already reported.
		return;
	}

	// Construct the initial set of positions.
	TParsingState initial_set;
	ClearNewStateHashTable();
#ifdef WANT_SLR_SUPPORT
	if (m_slr_method == TRUE)
	{
		// Use the SLR method.
		if (AddSimpleRuleStartPositions(initial_set, m_conv_grm->axioma_ident) == FALSE)
		{
			// Error is already reported.
			return;
		}
	}
	else
#endif
	{
		// This is more complex LR case.
		if (m_conv_grm->parent_grammar == NULL)
		{
			// Add positions for the EOF only.
			if (AddRuleStartPositions(initial_set, m_conv_grm->axioma_ident, NULL, 0, TerminalSymbolsBase) == FALSE)
			{
				// Error is already reported.
				return;
			}
		}
		else
		{
			// Nested grammar. Add positions for all terminals.
			for (int sym=0; sym<m_conv_grm->NumTerminals(); ++sym)
			{
				if (AddRuleStartPositions(initial_set, m_conv_grm->axioma_ident, NULL, 0, sym) == FALSE)
					return;
			}
		}

		if (initial_set.NumItems() > 1)
			initial_set.QuickSort(CompareRulePositions);
	}

	// Add initial set to the "set of sets".
	if (AppendParsingState(initial_set) == FALSE)
	{
		// It is hard to imagine that this can happen.
		return;
	}

	//
	//		Major loop.
	//	--------------------------
	//  Generate all possible derivations from already registered parsing states.
	//

	int iset = 0;
	int prev_iset = 0, prev_num_states = 0;
	int num_rules = m_conv_grm->NumRules();
	TParsingState new_set;
	while (iset < m_conv_grm->parsing_states.NumItems())
	{
		// Consider all possible derivations from the current set.
		m_conv_grm->parsing_states[iset].ClearMarkFields();
		int curr_set_len = m_conv_grm->parsing_states[iset].NumItems();
		for (int ipos=0; ipos<curr_set_len; ++ipos)
		{
			// Repick the current set on each rotation of the loop because appending new set may cause relocation of the big set of sets.
			TParsingState &curr_set = m_conv_grm->parsing_states[iset];
			if (curr_set[ipos].mark != 0)
			{
				// Current element of the set is already processed.
				continue;
			}

			TGrammarRule &rule1 = m_conv_grm->rules[curr_set[ipos].irule];
			if (curr_set[ipos].isym >= rule1.Length())
			{
				// Position is at the end of the rule. This position cannot generate the shift actions. Ignore it.
				continue;
			}

			// Pick up the shift symbol from the rule.
			WORD shift_symbol = rule1.symbols[curr_set[ipos].isym];

			// Start creation of the new parsing state. When parser is in current state (iset) and "shift_symbol" is shifted
			// into the stack, state will change to this "new state".
			new_set.SetNumItems(0);
			ClearNewStateHashTable();

			// Look for other positions in the curr set that have the same shift symbol. Add these positions to the new set
			// and mark them as processed. Note that first iteration of the loop will add current position to the new set.
			for (int ip=ipos; ip<curr_set_len; ++ip)
			{
				if (curr_set[ip].mark != 0)
					continue;

				TGrammarRule &rule2 = m_conv_grm->rules[curr_set[ip].irule];
				int cs_isym = curr_set[ip].isym;
				if (cs_isym >= rule2.Length())
					continue;

				if (rule2.symbols[cs_isym] == shift_symbol)
				{
					// First or identical next symbol.
					if (AppendRulePosition(new_set, curr_set[ip].irule, cs_isym+1, curr_set[ip].action_sym) == FALSE)
					{
						return;
					}

					// Check if rule has next symbol and if this symbol is available, check if it is terminal or non terminal.
					if (cs_isym+1 < rule2.Length() && rule2.symbols[cs_isym+1] >= NonTerminalsBase)
					{
						// Symbol is present and it is non terminal. New set should contain start positions in all rules
						// that define this non terminal.
						WORD bsym = rule2.symbols[cs_isym+1];
#ifdef WANT_SLR_SUPPORT
						if (m_slr_method == TRUE)
						{
							// Use the SLR method.
							if (AddSimpleRuleStartPositions(new_set, bsym) == FALSE)
							{
								return;
							}
						}
						else
#endif
						{
							// Use more complex LR method.
							WORD *beta_seq = rule2.symbols.ItemPtr(cs_isym+2);
							int beta_seq_len = rule2.Length()-(cs_isym+2);
							if (AddRuleStartPositions(new_set, bsym, beta_seq, beta_seq_len, curr_set[ip].action_sym) == FALSE)
							{
								return;
							}
						}
					}

					// Mark the curr position as processed.
					curr_set[ip].mark = 1;
				}
			}

			// Sort positions in the new set.
			if (new_set.NumItems() > 1)
				new_set.QuickSort(CompareRulePositions);

			// Construction of the new parsing state is complete. Check if new_set is something new or it is already
			// present somewhere in the big set of sets.
			WORD dest_state = 0xFFFF;
			for (int ii=0; ii < m_conv_grm->parsing_states.NumItems(); ++ii)
			{
				if (new_set.CompareTo(m_conv_grm->parsing_states[ii]) == TRUE)
				{
					// Identical set is found.
					dest_state = (WORD)ii;
					break;
				}
			}

			if (dest_state == 0xFFFF)
			{
				// This is a new set. Create new row in the actions table.
				dest_state = AllocateNewParsingState();
				if (dest_state == 0xFFFF)
				{
					// Memory error. It should be already reported.
					return;
				}

				// Length of the "set of sets" and height of the actions table should be identical.
				assert(dest_state == m_conv_grm->parsing_states.NumItems());

				// Add new set to the "set of sets".
				if (AppendParsingState(new_set) == FALSE)
				{
					return;
				}
			}

			// Mark action table with sift action for current state and current shift symbol.
			WORD *stt_row = m_conv_grm->analysis_table.GetRowPtr(iset);
			int shift_sym_index = SymTableFromSymGrammar(shift_symbol);
			assert(stt_row[shift_sym_index] == 0xFFFF);
			stt_row[shift_sym_index] = actb_shift + dest_state;
		}

		// Check for abort and trace the end of processing for the current set.
		if (m_conv_grm->parsing_states.NumItems() != prev_num_states && iset-prev_iset > 10)
		{
			if (m_scanner->GetCurrTraceMode() & gdtr_steps)
			{
				wchar_t buffer[80];
				m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"%s: Curr state: %4d, total states: %4d, total positions: %7d.",
									FormatDateTime(CurrDateTime(), buffer, 80, FALSE),
									iset, m_conv_grm->parsing_states.NumItems(), m_conv_grm->parsing_states.num_positions);
			}

			if (m_scanner->Console().GetAbortFlag() == TRUE)
			{
				ReportConversionError(gerr_aborted_by_the_user, L"Grammar conversion was aborted by the user.");
				return;
			}

			prev_iset = iset;
		}

		// Proceed to the next set.
		iset++;
		prev_num_states = m_conv_grm->parsing_states.NumItems();
	}

	if (m_scanner->GetCurrTraceMode() & gdtr_steps)
	{
		wchar_t buffer[80];
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"%s: Total parsing states: %d.",
							FormatDateTime(CurrDateTime(), buffer, 80, FALSE), m_conv_grm->parsing_states.NumItems());
	}

	// Success.
	return;
}

bool TGrammarDefinitionParser::AddSimpleRuleStartPositions(TParsingState &stt_set, WORD non_term)
{
	// Pick up array for the passed non terminal.
	assert(non_term >= NonTerminalsBase);
	TParsingState &prpg_info = m_slr_propagation_info[non_term-NonTerminalsBase];

	// Array that is being appended should be not empty.
	if (prpg_info.NumItems() == 0)
	{
		wchar_t buff_40_chars[40];
		ReportConversionError(gerr_cannot_propagare_state, L"Empty SLR state propagation array for symbol: %s(%hd).",
							m_conv_grm->GetSymbolName(non_term, buff_40_chars), non_term);
		return(FALSE);
	}

	for (int ipos=0; ipos<prpg_info.NumItems(); ++ipos)
	{
		TRulePosition &pos = prpg_info[ipos];
		if (AppendRulePosition(stt_set, pos.irule, pos.isym, pos.action_sym) == FALSE)
		{
			ReportConversionError(gerr_no_mem_for_new_pars_state, L"Low on memory while adding SLR start positions. Array length: %d.",
								stt_set.NumItems());
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

bool TGrammarDefinitionParser::AddRuleStartPositions(TParsingState &stt_set, WORD non_term, WORD *beta_seq, int beta_seq_len, WORD action_sym)
{
	// Initiate work set with FIRST(beta) symbols.
	WORD num_terminals = (WORD)m_conv_grm->symbols.NumItems();
	GenerateFirstsSet(m_lr_work_set.DataPtr(), beta_seq, beta_seq_len);

	// Add passed non_term, m_lr_work_set and passed next symbol as initial record to the "work to do" list.
	m_lr_gen_contexts_task.Clear();
	TGenClosureTask initial_task(non_term, 0xFFFF, m_lr_work_set.DataPtr(), action_sym);
	if (m_lr_gen_contexts_task.FindOrAppend(initial_task) == 0)
	{
		ReportConversionError(gerr_no_mem_for_prpg_work_list, L"Low on memory while initing LR start positions work list.");
		return(FALSE);
	}

	// Rutinely process the "work to do" records one by one.
	for (int itask=1; itask<m_lr_gen_contexts_task.NumItems(); ++itask)
	{
		// Load task fields info into locals because array of tasks can be relocated during processing of the task.
		WORD  task_non_term =   m_lr_gen_contexts_task[itask].non_term;
		WORD *task_firsts_set = m_lr_gen_contexts_task[itask].first_syms_set;
		WORD  task_action_sym = m_lr_gen_contexts_task[itask].action_sym;

		if (task_firsts_set != NULL)
		{
			// Task item contains some non empty set of FIRST symbols. Iterate members of this set except for the empty symbol.
			for (WORD sym=TerminalSymbolsBase+1; sym<num_terminals; ++sym)
			{
				if (task_firsts_set[sym] != 0)
				{
					if (AddPositionsAndTasks(stt_set, task_non_term, sym) == FALSE)
						return(FALSE);
				}
			}

			// If empty symbol is in the set, clear the pointer to enable processing the next symbol from the task.
			if (task_firsts_set[0] != 0)
				task_firsts_set = NULL;
		}

		if (task_firsts_set == NULL)
		{
			// Task had no FIRSTs pointer or pointer contained an empty symbol.
			if (AddPositionsAndTasks(stt_set, task_non_term, task_action_sym) == FALSE)
				return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

bool TGrammarDefinitionParser::AddPositionsAndTasks(TParsingState &stt_set, WORD non_term, WORD action_sym)
{
	// Iterate rules that define passed non terminal.
	bool something_defined = FALSE;
	int num_rules = m_conv_grm->NumRules();
	TGrammarRule *rule = m_conv_grm->rules.DataPtr();
	for (int irule=0; irule<num_rules; ++irule, ++rule)
	{
		if (rule->non_term != non_term)
		{
			// Curr rule defines some other non terminal.
			continue;
		}

		// Set of rules for passed non terminal is not empty. Mark this fact and activate the rule.
		something_defined = TRUE;

		// Construct rule position and add it to the passed set of positions if it is not already there.
		if (AppendRulePosition(stt_set, irule, 0, action_sym) == FALSE)
		{
			return(FALSE);
		}

		// Check if this rule can generate new work item or not.
		int rule_len = rule->symbols.NumItems();
		if (rule_len > 0 && rule->symbols[0] >= NonTerminalsBase)
		{
			// This rule generates new work item. Create it.
			TGenClosureTask new_task(rule->symbols[0], irule, NULL, action_sym);
			if (rule_len > 1)
			{
				// Rule has second symbol.
				if (rule->symbols[1] >= NonTerminalsBase)
				{
					// Second symbol is also non terminal. This means that its FIRST set should be processed.
					new_task.first_syms_set = m_lr_rule_firsts.GetRowPtr(irule);
				}
				else
				{
					// Use second symbol from the rule as next symbol.
					new_task.action_sym = rule->symbols[1];
				}
			}

			// Check if this work item is new or not.
			if (m_lr_gen_contexts_task.FindOrAppend(new_task) == 0)
			{
				ReportConversionError(gerr_no_mem_for_prpg_work_list,
									L"Low on memory for LR start positions work list. Array length: %d.", m_lr_gen_contexts_task.NumRealItems());
				return(FALSE);
			}
		}
	}

	// Ensure that rules for passed non terminal are present.
	assert(something_defined == TRUE);
	return(TRUE);
}

bool TGrammarDefinitionParser::AppendRulePosition(TParsingState &stt_set, WORD irule, WORD ipos, WORD action_sym)
{
	// Construct position and find its hash bucket.
	TRulePosition new_pos = { irule, ipos, action_sym };
	int hash_inx = new_pos.CalcHashValue() & (NEW_STT_BUCKETS-1);

	// Check for similar items in the passed set of positions.
	TRulePosition *stt_set_data = stt_set.DataPtr();
	int inx = m_new_stt_hash[hash_inx];
	while (inx != 0xFFFF)
	{
		TRulePosition &curr = stt_set_data[inx];
		if (curr.irule == new_pos.irule && curr.isym == new_pos.isym && curr.action_sym == new_pos.action_sym)
		{
			// Identical record was found.
			return(TRUE);
		}

		inx = curr.mark;
	}

	// This is a new position. Append it to the set.
	new_pos.mark = m_new_stt_hash[hash_inx];
	if (stt_set.AppendItem(new_pos) == FALSE)
	{
		// Failure to add the new item.
		ReportConversionError(gerr_no_mem_for_rule_position, L"Low on memory while adding rule position to helper. Curr set len: %d.", stt_set.NumItems());
		return(FALSE);
	}

	// Ensure that index of this new position still fits into WORD.
	int new_item_inx = stt_set.NumItems()-1;
	if (new_item_inx >= 0xFFFF)
	{
		ReportConversionError(gerr_too_many_positions_in_stt, L"New parsing state contains more than 65534 positions.");
		return(FALSE);
	}

	// Update the hash table.
	m_new_stt_hash[hash_inx] = new_item_inx;
	return(TRUE);
}

bool TGrammarDefinitionParser::AppendParsingState(TParsingState &stt_set)
{

#ifdef DUMP_RULE_POS_BUCKETS_LENGTHS
	//
	// This is debug code. It evaluates quality of the hash function. Hash function distributes rule positions between
	// the hash buckets. This piece of code dump lengths of the hash buckets before changing the data structures.
	//
	if (m_scanner->GetCurrTraceMode() & gdtr_steps)
	{
		int cnt_empty = 0, biggest_len = 0, biggest_inx = 0;
		for (int ib=0; ib<NEW_STT_BUCKETS; ++ib)
		{
			int cnt = 0, inx = m_new_stt_hash[ib];
			while (inx != 0xFFFF)
			{
				cnt++;
				inx = stt_set[inx].mark;
			}

			if (cnt == 0)
				cnt_empty++;

			if (cnt > biggest_len)
			{
				biggest_len = cnt; biggest_inx = ib;
			}
		}

		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"Num positions: %5d, cnt_empty: %5d, biggest_len: %4d, biggest_inx: %4d.",
							set_len, cnt_empty, biggest_len, biggest_inx);
	}
#endif

	int set_len = stt_set.NumItems();
	if (m_conv_grm->parsing_states.AppendItem(stt_set) == FALSE)
	{
		ReportConversionError(gerr_no_mem_for_pars_stts_set,
							L"Low on memory while adding new parsing state. Num parsing states: %d.", m_conv_grm->parsing_states.NumItems());
		return(FALSE);
	}

	// Appending succeeded.
	m_conv_grm->parsing_states.num_positions += set_len;

	// Success.
	return(TRUE);
}

void TGrammarDefinitionParser::AssignResolveActions()
{
	if (m_scanner->GetCurrTraceMode() & gdtr_steps)
	{
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"AssignResolveActions(%s)", (m_slr_method == TRUE) ? L"SLR" : L"LR");
	}

	// Ensure that analysis table is already present.
	assert(m_conv_grm->analysis_table.IsInited() == TRUE);

	// Clear the conflicts info in the grammar if any.
	m_conv_grm->conflicts.ReleaseObject();

	// Iterate all final positions in all rules.
	int follows_table_width = m_firsts_and_follows.GetWidth();
	for (int istt=0; istt<m_conv_grm->parsing_states.NumItems(); ++istt)
	{
		WORD *stt_base = m_conv_grm->analysis_table.GetRowPtr(istt);
		TParsingState &state_info = m_conv_grm->parsing_states[istt];

		// Iterate positions of the current  state.
		for (int ipos=0; ipos<state_info.NumItems(); ++ipos)
		{
			WORD irule = state_info[ipos].irule;
			TGrammarRule &rule = m_conv_grm->rules[irule];
			if (state_info[ipos].isym < rule.Length())
			{
				// This is some intermediate position.
				continue;
			}

			// Current position is final position of the rule.
#ifdef WANT_SLR_SUPPORT
			if (m_slr_method == TRUE)
			{
				// In SLR method reduce actions should be filled into all slots that are known as possible followers
				// for the non terminal of the rule.
				WORD *non_term_info = m_firsts_and_follows.GetNonTermRowPtr(rule.non_term);

				// Fill reduce action for curr rule.
				for (WORD sym=0; sym<follows_table_width; ++sym)
				{
					if ((non_term_info[sym] & FUN_FOLLOWS_MASK) != 0)
					{
						// Current terminal symbol can follow non terminal of the rule.
						if (stt_base[sym] == TAnalysisTable::empty_cell)
						{
							// Nothing is present in the current cell yet. Put reduce action into this cell.
							stt_base[sym] = actb_reduce+irule;
						}
						else if (stt_base[sym] != actb_reduce+irule)
						{
							// Cell is already occupied with some different action. Generate or update the conflict.
							WORD symbol = SymGrammarFromSymTable(sym);
							if (ProcessConflict(istt, symbol, irule, stt_base[sym]) == FALSE)
							{
								return;
							}
						}
					}
				}
			}
			else
#endif
			{
				// In LR(1) method reduce action should be filled only into one column. Symbol of this column is stored
				// in the position itself in the "action_sym" field.
				WORD action_sym = state_info[ipos].action_sym;
				if (stt_base[action_sym] == TAnalysisTable::empty_cell)
				{
					// Nothing is present in the current cell yet. Put reduce action into this cell.
					stt_base[action_sym] = actb_reduce+irule;
				}
				else if (stt_base[action_sym] != actb_reduce+irule)
				{
					// The cell is already occupied with some different action. Generate or update the conflict.
					assert(action_sym < NonTerminalsBase);
					if (ProcessConflict(istt, action_sym, irule, stt_base[action_sym]) == FALSE)
					{
						return;
					}
				}
			}
		}
	}

	// Success.
	return;
}

bool TGrammarDefinitionParser::ProcessConflict(WORD stt, WORD sym, int irule, WORD existing_cell_value)
{
	wchar_t buff_40_chars[40];
	int conflict_inx = -1;
	if (existing_cell_value < actb_conflict)
	{
		// Conflict record is not created yet.
		conflict_inx = m_conv_grm->AddGrammarConflict(stt, sym);
		if (conflict_inx == 0xFFFF)
		{
			ReportConversionError(gerr_no_mem_for_conflits,
								L"Low on memory while creating conflict info. Num conflicts: %d.", m_conv_grm->conflicts.NumItems());
			return(FALSE);
		}

		// Trace conflict creation.
		if (m_scanner->GetCurrTraceMode() & gdtr_conflicts)
		{
			m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"New parsing conflict #%d. State: %hd, Symbol: %s(%hd).",
								conflict_inx, stt, m_conv_grm->GetSymbolName(sym, buff_40_chars), sym);
		}

		// Fill analysis table with the index of the conflict.
		WORD *stt_row = m_conv_grm->analysis_table.GetRowPtr(stt);
		stt_row[sym] = (WORD)(actb_conflict+conflict_inx);

		// Add intial info about the current cell value.
		if (m_conv_grm->AddGrammarConflictAction(conflict_inx, existing_cell_value) == FALSE)
		{
			ReportConversionError(gerr_no_mem_for_conflits, L"Low on memory while adding initial conflict record.");
			return(FALSE);
		}

		// Trace initial conflicting action.
		if (m_scanner->GetCurrTraceMode() & gdtr_conflicts)
		{
			int action_inx = m_conv_grm->conflicts[conflict_inx].conflicting_actions.NumItems()-1;
			m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"Conflict #%d. Action %d: %s.",
								conflict_inx, action_inx, m_conv_grm->GetParsingActionName(existing_cell_value, buff_40_chars));
		}
	}
	else
	{
		// Pick up the conflict index out of the current cell value.
		conflict_inx = existing_cell_value-actb_conflict;
	}

	// Add info about the current action.
	WORD wanted_action = actb_reduce+irule;
	if (m_conv_grm->AddGrammarConflictAction(conflict_inx, wanted_action) == FALSE)
	{
		ReportConversionError(gerr_no_mem_for_conflits, L"Low on memory while adding conflicting action. Array length: %d.",
							m_conv_grm->conflicts[conflict_inx].conflicting_actions.NumItems());
		return(FALSE);
	}

	// Trace additional conflicting action.
	if (m_scanner->GetCurrTraceMode() & gdtr_conflicts)
	{
		int action_inx = m_conv_grm->conflicts[conflict_inx].conflicting_actions.NumItems()-1;
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"Conflict #%d. Action %d: %s.",
							conflict_inx, action_inx, m_conv_grm->GetParsingActionName(wanted_action, buff_40_chars));
	}

	return(TRUE);
}

// /\/\/\/\/\/\/\/\ Conversion support methods  /\/\/\/\/\/\/\/\/\/\/\

bool TGrammarDefinitionParser::GeneratePropagationInfo(TParsingStatesArray &res_array)
{
	// Get rid of the prev contents of array if any.
	res_array.ReleaseObject();

	if (m_scanner->GetCurrTraceMode() & gdtr_steps)
	{
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"Generating SLR propagation info for %d non terminals.", m_conv_grm->NumNonTerminals());
	}

	// Initiate resulting aray with slots for every non terminal.
	TParsingState initial_state;
	for (int ib=0; ib<m_conv_grm->NumNonTerminals(); ++ib)
	{
		if (res_array.AppendItem(initial_state) == FALSE)
		{
			ReportConversionError(gerr_no_mem_for_slr_prpg_info,
								L"Low on memory while creating SLR propagation info. Array length: %d.", res_array.NumItems());
			return(FALSE);
		}
	}

	// Iterare non terminals.
	for (int isym=0; isym<m_conv_grm->NumNonTerminals(); ++isym)
	{
		TParsingState &stt_set = res_array[isym];
		m_sym_buffer.SetNumItems(0);
		ClearNewStateHashTable();

		// Add current symbol to the "list to do".
		WORD curr_sym = isym+NonTerminalsBase;
		if (m_sym_buffer.AppendItem(curr_sym) == FALSE)
		{
			ReportConversionError(gerr_no_mem_for_slr_prpg_info,
								L"Low on memory while initing the SLR work list. Array length: %d.", m_sym_buffer.NumItems());
			return(FALSE);
		}

		int inx_to_process = 0;
		while (inx_to_process < m_sym_buffer.NumItems())
		{
			// Pick up the next symbol to search for rules that define it.
			WORD curr_non_term = m_sym_buffer[inx_to_process++];

			// Scan all rules.
			for (TRulesIterator iter(*m_conv_grm); iter; ++iter)
			{
				TGrammarRule &rule = iter.CurrRule();
				if (rule.non_term == curr_non_term)
				{
					// Curr rule defines current non terminal of the outer loop. Add beginning of this rule to the curr set of positions.
					if (AppendRulePosition(stt_set, iter.CurrRuleIndex(), 0, 0xFFFF) == FALSE)
						return(FALSE);

					// Check, if right hand side of this rule starts with non terminal, this non terminal should be processed also.
					if (rule.Length() > 0 && rule.symbols[0] >= NonTerminalsBase)
					{
						// Add symbol for processing only if it is not already present in the "to do" list.
						if (m_sym_buffer.IsSymInBuffer(rule.symbols[0]) == FALSE)
						{
							if (m_sym_buffer.AppendItem(rule.symbols[0]) == FALSE)
							{
								ReportConversionError(gerr_no_mem_for_slr_prpg_info,
													L"Low on memory while adding symbol to SLR work list. Array length: %d.", m_sym_buffer.NumItems());
								return(FALSE);
							}
						}
					}
				}
			}
		}

		// Sort positions in the array.
		if (stt_set.NumItems() > 1)
			stt_set.QuickSort(CompareRulePositions);
	}

	// Success.
	return(TRUE);
}

bool TGrammarDefinitionParser::GenerateSpecialFirstSetsForRules(TAnalysisTable &res_table)
{
	// Prepare the table.
	res_table.ReleaseTable();
	int num_terminals = m_conv_grm->NumTerminals();
	if (res_table.SetupTable(m_conv_grm->NumRules(), num_terminals) == FALSE)
	{
		ReportConversionError(gerr_no_mem_for_spec_firsts, L"Low on memory while creating matrix for special rule firsts set.");
		return(FALSE);
	}

	// Init the table.
	for (int irule=0; irule<m_conv_grm->NumRules(); ++irule)
	{
		WORD *pset = res_table.GetRowPtr(irule);
		TGrammarRule &rule = m_conv_grm->rules[irule];
		if (rule.Length() == 0 || rule.symbols[0] < NonTerminalsBase)
		{
			// Rule is empty or it starts from terminal symbol.
			GenerateFirstsSet(pset, NULL, 0);
		}
		else
		{
			GenerateFirstsSet(pset, rule.symbols.ItemPtr(1), rule.Length()-1);
		}
	}

	// Success.
	return(TRUE);
}

void TGrammarDefinitionParser::GenerateFirstsSet(WORD *set, WORD *seq, int seq_len)
{
	// Length of the set is the number of terminals in the grammar.
	WORD set_length = (WORD)m_conv_grm->symbols.NumItems();

	// Clear the set first.
	for (WORD i1=0; i1<set_length; ++i1)
		set[i1] = FALSE;

	bool add_empty_sym = TRUE;
	for (int inx=0; inx<seq_len; ++inx)
	{
		WORD sym = seq[inx];
		if (sym >= NonTerminalsBase)
		{
			// Add all first symbols of the curr non terminal except for the empty symbol to the destination set.
			WORD *sym_info = m_firsts_and_follows.GetNonTermRowPtr(sym);
			for (WORD i2=1; i2<set_length; ++i2)
			{
				if ((sym_info[i2] & FUN_FIRST_MASK) != 0)
					set[i2] = TRUE;
			}

			if ((sym_info[0] & FUN_FIRST_MASK) == 0)
			{
				// Non terminal cannot be represented by an empty symbol.
				add_empty_sym = FALSE;
				break;
			}
		}
		else
		{
			assert(sym != 0 && sym < set_length);
			set[sym] = TRUE;
			add_empty_sym = FALSE;
			break;
		}
	}

	// If all symbols of the seq contain empty symbol in their FIRSTs or the seq is empty, then add
	// empty symbol (eof) to the resut set.
	if (add_empty_sym == TRUE)
		set[0] = TRUE;
}

int __cdecl TGrammarDefinitionParser::CompareRulePositions(const TRulePosition *pos1, const TRulePosition *pos2)
{
	if (pos1->irule > pos2->irule)
		return(1);
	else if (pos1->irule < pos2->irule)
		return(-1);

	if (pos1->isym > pos2->isym)
		return(1);
	else if (pos1->isym < pos2->isym)
		return(-1);

	if (pos1->action_sym > pos2->action_sym)
		return(1);
	else if (pos1->action_sym < pos2->action_sym)
		return(-1);

	// Positions should never be identical.
	assert(FALSE);
	return(0);
}

void TGrammarDefinitionParser::GenerateFollowersAndParsingStatesReport(TGrammar &grm, TAnalysisTable &followers_table)
{
	TFileNameBuffer file_name_buffer;
	TNameBuffer grammar_name_buffer;
	if (GenerateGrammarNameForReporting(file_name_buffer, grammar_name_buffer, grm) == FALSE)
	{
		ReportConversionError(gerr_no_mem_for_work_buffer, L"Out of memory while generating grammar name for the follower and parsing states report.");
		return;
	}

	if (PrepareReporsDirectory(TRUE) == FALSE)
		return;

	TGrammarDefnParserReportFile rprt(this, grammar_name_buffer, grm.GetGrammarIndex(), grm.NumTerminals(), grm.NumNonTerminals(), grm.NumRules(),
									GenerateReportFileName(file_name_buffer, grammar_name_buffer, grm.GetGrammarIndex(), CONV_REPORT_FNAME));

	if (m_scanner->GetCurrTraceMode() & gdtr_reports)
	{
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"Generating parsing states report...");
	}

	// Print non terminals of the curr grammar in the alphabetical order.
	wchar_t buff_40_chars[40];
	int num_terminals = grm.NumTerminals();
	TSymbolsArray non_terms_list;

	rprt.WriteLine();
	if (TGrammarSymbolsHelper::CreateSortedNonTerminalsList(grm.non_terminals, &(grm.rules), non_terms_list) == TRUE)
	{
		// Check for symbols that do not have followers.
		bool something_without_followers = FALSE;
		for (int jj1=0; jj1<non_terms_list.NumItems(); ++jj1)
		{
			WORD non_term = non_terms_list[jj1];
			WORD *sym_set = followers_table.GetNonTermRowPtr(non_term);

			bool has_followers = FALSE;
			for (int offs=0; offs<num_terminals; ++offs)
			{
				if ((sym_set[offs] & FUN_FOLLOWS_MASK) != 0)
				{
					has_followers = TRUE;
					break;
				}
			}

			if (has_followers == FALSE)
			{
				rprt.WriteFmtLine(L"  %-24s (sym=%d) has no followers.", grm.GetSymbolName(non_term, buff_40_chars), non_term);
				non_terms_list[jj1] = 0;
				something_without_followers = TRUE;
			}
		}

		if (something_without_followers == TRUE)
		{
			// Write delimiter between 2 parts of report.
			rprt.WriteLine();
			rprt.WriteFmtLine(L"=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
			rprt.WriteLine();
		}

		// Print symbols that have non empty list of followers.
		for (int jj2=0; jj2<non_terms_list.NumItems(); ++jj2)
		{
			WORD non_term = non_terms_list[jj2];
			if (non_term == 0)
			{
				// Symbol was already processed in the prev loop.
				continue;
			}

			// Write name of the symbol.
			rprt.WriteFmtLine(L"  %s (sym=%d).", grm.GetSymbolName(non_term, buff_40_chars), non_term);
			TGrammarSymbolsHelper::WriteReportDelimiter(rprt);

			// Iterate available followers.
			int cnt = 1;
			WORD *sym_set = followers_table.GetNonTermRowPtr(non_term);
			for (int offs=0; offs<num_terminals; ++offs)
			{
				if ((sym_set[offs] & FUN_FOLLOWS_MASK) != 0)
				{
					// Offs is member of sym_set.
					rprt.WriteFmtLine(L"   %3d. %s (sym=%d).", cnt++, grm.GetSymbolName(offs, buff_40_chars), offs);
				}
			}

			// Add empty line between lists.
			rprt.WriteLine();
		}
	}
	else
	{
		ReportConversionError(gerr_no_mem_for_flwrs_rprt, L"Low on memory while generating followers report.");
	}

	if (grm.NumParsingStates() < 2000)
	{
		// Number of parsing states is not huge. Iterate sets of rule configurations.
		rprt.WriteLine();
		TParsingStatesArray &stts_set = grm.parsing_states;
		for (int iset=0; iset<stts_set.NumItems(); ++iset)
		{
			rprt.WriteFmtLine(L"  Parsing state %04d.", iset);
			TGrammarSymbolsHelper::WriteReportDelimiter(rprt);

			// Iterate all rules that have current state.
			TParsingState &curr_state = stts_set[iset];
			wchar_t buff_40_chars[40];
			for (int ipos=0; ipos<curr_state.NumItems(); ++ipos)
			{
				TRulePosition curr_pos = curr_state[ipos];
				if (curr_pos.irule < 0 || curr_pos.irule >= grm.NumRules())
				{
					rprt.WriteFmtLine(L" (inx=%03d) - Bogus rule index.", curr_pos.irule);
					continue;
				}

				TGrammarRule &rule = grm.rules[curr_pos.irule];
				if (curr_pos.isym < 0 || curr_pos.isym > rule.Length())
				{
					rprt.WriteFmtLine(L" (inx=%03d) - Bogus symbol index: %hd.", curr_pos.irule, curr_pos.isym);
					continue;
				}

				// Print rule with marking stored position and next position if stored position is not the final pos in the rule.
				rprt.WriteFmt(L" (irule=%03d) %s:",
				curr_pos.irule, grm.GetSymbolName(rule.non_term, buff_40_chars));
				for (int isym=0; isym <= rule.Length(); ++isym)
				{
					if (isym == curr_pos.isym)
					{
						// This is position of the current state.
						rprt.WriteFmt(L" <%hd>", iset);
					}
					else if (isym == curr_pos.isym+1)
					{
						// It is safe to get next state from analysis table.
						WORD next_stt = grm.analysis_table.GetAction((WORD)iset, SymTableFromSymGrammar(rule.symbols[curr_pos.isym]));

						// Value 0xFFFF may appear if parsing states assignment was aborted in the middle.
						assert(next_stt < actb_reduce || next_stt == 0xFFFF);
						if (next_stt == 0xFFFF)
							rprt.Write(L" <FFFF>");
						else rprt.WriteFmt(L" <%hd>", next_stt);
					}

					// Print the right hand symbol except for the last position.
					if (isym < rule.Length())
						rprt.WriteFmt(L" %s", grm.GetSymbolName(rule.symbols[isym], buff_40_chars));
				}

				if (curr_pos.action_sym != 0xFFFF)
					rprt.WriteFmt(L"    [%s]", grm.GetSymbolName(curr_pos.action_sym, buff_40_chars));

				rprt.WriteLine();
			}

			rprt.WriteLine();
		}
	}
	else
	{
		rprt.WriteLine();
		rprt.WriteLine(L"=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
		rprt.WriteLine(L"  Dumping the rule positions was skipped because the grammar is too big.");
		rprt.WriteLine(L"=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
		rprt.WriteLine();
	}

	rprt.CloseReport(L"non terminal followers and parsing states report");
}

// /\/\/\/\/\/\/\  Grammar post processing methods  /\/\/\/\/\/\/\/\

void TGrammarDefinitionParser::PostProcessGrammarsHier(TGrammar &grm)
{
	// Post process the grammar itself.
	PostProcessSingleGrammar(grm);

	// Process child grammars if any.
	for (int ixpct=0; ixpct<grm.xpct_conflicts.NumItems(); ++ixpct)
	{
		TGrammar *chld = grm.xpct_conflicts[ixpct].nested_grammar;
		if (chld != NULL)
			PostProcessGrammarsHier(*chld);
	}
}

void TGrammarDefinitionParser::PostProcessSingleGrammar(TGrammar &grm)
{
	TFileNameBuffer file_name_buffer;
	TNameBuffer grammar_name_buffer;
	if (GenerateGrammarNameForReporting(file_name_buffer, grammar_name_buffer, grm) == FALSE)
	{
		ReportConversionError(gerr_cfct_location_markers, L"Out of memory while generating grammar name for post processing the grammar.");
		return;
	}

	if (m_scanner->GetCurrTraceMode() & gdtr_steps)
	{
		m_scanner->TraceEvent(NULL, TGDP_GENERAL, L"Processing: %s", grammar_name_buffer.DataPtr());
	}

	// Allocate mask for tracking state of the parsing states. This mask will be built for each location object.
	// This means that there is no point in initing it here. Just reserve the space.
	TSymbolsArray states_mask;
	if (states_mask.ReserveSpace(grm.NumParsingStates()) == FALSE)
	{
		ReportConversionError(gerr_cfct_location_markers, L"Out of memory while allocating buffer for the parsing states mask.");
		return;
	}

	//
	//   Step 1.
	//
	//  Generate list of conforming parsing states for each location object in the current grammar. Loc objects
	//  were propagated from the source data to the root/nested grammars. This means that this step will processes
	//  only one grammar.
	//
	for (int iloc=0; iloc<grm.location_objects.NumItems(); ++iloc)
	{
		// Pick up the info.
		wchar_t *loc_name = grm.location_objects[iloc].location_name;
		WORD action_symbol = grm.location_objects[iloc].action_symbol;
		TRulePositionsArray &positions = grm.location_objects[iloc].positions;
		assert(action_symbol < NonTerminalsBase && positions.NumItems() > 0);

		// Mark all states in the mask as valid states for the current loc object. After processing all positions
		// of the loc object this mask should contain only those states that are valid for all loc object positions.
		for (WORD is1=0; is1<grm.NumParsingStates(); ++is1)
			states_mask[is1] = TRUE;

		// Process positions of the loc object.
		for (int ipos=0; ipos<positions.NumItems(); ++ipos)
		{
			if (positions[ipos].mark == TRUE)
			{
				// This is pseudo position. It should be ignored.
				continue;
			}

			// Check if at lest one state is fine for the current pos and remove those states, that are not fine,
			// from the mask of conforming parsing states.
			if (CheckLocObjectPosition(grm, states_mask, positions[ipos]) == FALSE)
			{
				// Current rule position is not present in any state.
				WORD irule = positions[ipos].irule;
				if (irule == 0xFFFF)
				{
					// Location marker stays in the rule that was not included into the current grammar.
					ReportConversionError(gerr_cfct_location_markers,
										L"Conflict location marker \"%s\" stays in the rule that was not included into the current grammar.", loc_name);
				}
				else if (positions[ipos].isym < grm.rules[irule].Length())
				{
					// This is an intermediate rule position. This should be pretty rare case because this means that
					// this rule does not have positions at all. Nevertheless issue an error on this.
					ReportConversionError(gerr_cfct_location_markers,
										L"Conflict location marker \"%s\" stays in the rule (irule=%d) that is not involved into the analysis process.", loc_name, irule);
				}
				else
				{
					// This is final position of the rule. It may happen that reason of this problem is the same to the one a few
					// lines above. Although this is highly unlikely. More likely is that location object contains wrong action symbol.
					// This cannot be checked at the scanning phase because list of valid action symbols is not known yet.
					assert(action_symbol == positions[ipos].action_sym);

					wchar_t buff1[40];
					ReportConversionError(gerr_cfct_location_markers,
										L"Conflict location marker \"%s\" contains wrong action symbol %s (%hd) that does not belong to the set of action symbols of R%d.",
										loc_name, grm.GetSymbolName(action_symbol, buff1), action_symbol, irule);
				}
			}
		}

		// Save conforming states that were discovered in the loop above into the location object of the current
		// grammar as the list of states.
		TSymbolsArray &conforming_states = grm.location_objects[iloc].conforming_states;
		for (WORD is2=0; is2<grm.NumParsingStates(); ++is2)
		{
			if (states_mask[is2] == FALSE)
				continue;

			if (conforming_states.AppendItem(is2) == FALSE)
			{
				ReportConversionError(gerr_cfct_location_markers, L"Out of memory while adding conforming parsing state to conflict location object.");
				break;
			}

			// Ensure that location markers are set everywhere, where the current conforming parsing state is present.
			TParsingStatePositionsIterator iter;
			WORD prev_irule = 0xFFFF, prev_isym = 0xFFFF;
			for (iter.Setup(grm.parsing_states[is2]); iter; ++iter)
			{
				 bool verification_needed = FALSE;
				TGrammarRule &stt_pos_rule = grm.rules[iter.CurrPos().irule];
				if (iter.CurrPos().isym >= stt_pos_rule.Length())
				{
					// Position is at the end. Check the action symbol that is stored in the position.
					if (iter.CurrPos().action_sym == action_symbol)
						verification_needed = TRUE;
				}
				else
				{
					// Check the rule symbol and process this location only once.
					if (stt_pos_rule.symbols[iter.CurrPos().isym] == action_symbol && (iter.CurrPos().irule != prev_irule || iter.CurrPos().isym != prev_isym))
						verification_needed = TRUE;
				}

				if (verification_needed == TRUE)
				{
					// Current position should be marked with the location marker. Check this.
					bool position_marked = FALSE;
					for (int ips=0; ips<positions.NumItems(); ++ips)
					{
						if (positions[ips].irule == iter.CurrPos().irule && positions[ips].isym == iter.CurrPos().isym)
						{
							position_marked = TRUE;
							break;
						}
					}

					if (position_marked == FALSE)
					{
						ReportConversionWarning(gerr_cfct_location_markers,
											L"Position at R%hd, symbol index %hd contains parsing state %hd. This position should be marked with conflict location marker \"%s\".",
											iter.CurrPos().irule, iter.CurrPos().isym, is2, loc_name);
					}
				}

				prev_irule = iter.CurrPos().irule;
				prev_isym = iter.CurrPos().isym;
			}
		}

		// Ensure that there is at least one conforming state.
		if (conforming_states.NumItems() == 0)
		{
			ReportConversionError(gerr_cfct_location_markers, L"Conflict location object \"%s\" has no conforming parsing states.", loc_name);
		}
	}

	// Check for conforming state intersections between different conflict location objects that share the same
	// action symbol. Compare non empty objects with identical action symbols.
	for (int il1=0; il1<grm.location_objects.NumItems(); ++il1)
	{
		// Pick up the info.
		WORD act_sym1 = grm.location_objects[il1].action_symbol;
		TSymbolsArray &stts1 = grm.location_objects[il1].conforming_states;
		if (stts1.NumItems() == 0)
			continue;

		// Compare current loc object to all subsequent loc objects.
		for (int il2=il1+1; il2<grm.location_objects.NumItems(); ++il2)
		{
			if (grm.location_objects[il2].action_symbol != act_sym1)
				continue;

			// Both location objects have same action symbol.
			TSymbolsArray &stts2 = grm.location_objects[il2].conforming_states;
			if (stts2.NumItems() == 0)
				continue;

			// Both loc objects have non empty sets of conforming states. Look for intersection between those states.
			int istt1 = 0, istt2 = 0;
			while (istt1 < stts1.NumItems() && istt2 < stts2.NumItems())
			{
				WORD s1 = stts1[istt1], s2 = stts2[istt2];

				if (s1 < s2)
				{
					istt1++;
					continue;
				}

				if (s2 < s1)
				{
					istt2++;
					continue;
				}

				// States are identical.
				wchar_t buff2[40];
				ReportConversionError(gerr_cfct_location_markers,
									L"Conflict location objects \"%s\" and \"%s\", action symbol %s(%hd), share the parsing state %d.",
									grm.location_objects[il1].location_name, grm.location_objects[il2].location_name,
									grm.GetSymbolName(act_sym1, buff2), act_sym1, s1);
				istt1++;
				istt2++;
			}
		}
	}

	// Do not continue if errors are present above.
	if (m_scanner->GetErrorsCount() != 0)
		return;

	//
	//   Step 2.
	//
	//  At this point is is clear that:
	//
	//		-- All location objects of the grammar have conforming states.
	//		-- There is no intersection between pasing states that conform to different location objects.
	//
	//  Establish the correlation between the grammar conflicts that were discovered during the grammar conversion
	//  and expected conflict location objects.
	//
	for (int il3=0; il3<grm.location_objects.NumItems(); ++il3)
	{
		// Pick up conforming states and action sym from the curr loc object.
		TSymbolsArray &conf_states = grm.location_objects[il3].conforming_states;
		WORD act_sym = grm.location_objects[il3].action_symbol;

		// Iterate the conforming parsing states.
		for (int is3=0; is3<conf_states.NumItems(); ++is3)
		{
			// Iterare the grammar conflicts.
			bool grammar_conflict_found = FALSE;
			for (int icf1=0; icf1<grm.NumConflicts(); ++icf1)
			{
				TGrammarConflict &cfct = grm.conflicts[icf1];
				if (cfct.parser_state != conf_states[is3] || cfct.input_symbol != act_sym)
					continue;

				// Current grammar conflict conforms to the current location object. Establish the connection.
				grammar_conflict_found = TRUE;

				if (cfct.conflict_location_inx == -1)
				{
					// This conflict is not linked to any location object yet.
					cfct.conflict_location_inx = il3;
				}
				else if (cfct.conflict_location_inx >= 0)
				{
					// The first ambiguous location object is found.
					wchar_t buff[40];
					ReportConversionWarning(gerr_cfct_location_markers,
										L"Grammar conflict C%d, parsing state %hd, action symbol %s(%hd), corresponds to \"%s\" and \"%s\" location objects at the same time.",
										icf1, conf_states[is3], grm.GetSymbolName(act_sym, buff), act_sym,
										grm.location_objects[cfct.conflict_location_inx].location_name, grm.location_objects[il3].location_name);

					cfct.conflict_location_inx = -2;
				}
				else
				{
					// Next ambiguous location object is found.
					wchar_t buff[40];
					ReportConversionWarning(gerr_cfct_location_markers,
										L"Grammar conflict C%d, parsing state %hd, action symbol %s(%hd), corresponds to location object \"%s\" also.",
										icf1, conf_states[is3], grm.GetSymbolName(act_sym, buff), act_sym,
										grm.location_objects[il3].location_name);

					cfct.conflict_location_inx--;
				}

				// Look for expected conflicts that reference this location object.
				for (int ixp1=0; ixp1 < grm.NumXpctConflicts(); ++ixp1)
				{
					TSymbolsArray &locs = grm.xpct_conflicts[ixp1].xpct_conflict_locations;
					for (int ilcr=0; ilcr < locs.NumItems(); ++ilcr)
					{
						if (locs[ilcr] == il3)
						{
							// Current expected conflict references the current location object.
							if (cfct.expected_conflict_inx == -1)
							{
								// This conflict is not linked to any expected conflict yet.
								cfct.expected_conflict_inx = ixp1;
								cfct.nested_grammar = grm.xpct_conflicts[ixp1].nested_grammar;
							}
							else if (cfct.expected_conflict_inx >= 0)
							{
								// The first ambiguous expected conflict is found.
								wchar_t buff[40];
								ReportConversionWarning(gerr_cfct_location_markers,
													L"Grammar conflict C%d, parsing state %hd, action symbol %s(%hd), corresponds to \"%s\" and \"%s\"  expected conflicts at the same time.",
													icf1, conf_states[is3], grm.GetSymbolName(act_sym, buff), act_sym,
													grm.xpct_conflicts[cfct.expected_conflict_inx].xpct_conflict_name, grm.xpct_conflicts[ixp1].xpct_conflict_name);

								cfct.expected_conflict_inx = -2;
								cfct.nested_grammar = NULL;
							}
							else
							{
								// Next ambiguous expected conflict is found.
								wchar_t buff[40];
								ReportConversionWarning(gerr_cfct_location_markers,
													L"Grammar conflict C%d, parsing state %hd, action symbol %s(%hd), corresponds to expected conflict \"%s\" also.",
													icf1, conf_states[is3], grm.GetSymbolName(act_sym, buff), act_sym, grm.xpct_conflicts[ixp1].xpct_conflict_name);

								cfct.expected_conflict_inx--;
							}
						}
					}
				}
			}

			if (grammar_conflict_found == FALSE)
			{
				// Parsing state does not correspond to any grammar conflict.
				wchar_t buff[40];
				ReportConversionError(gerr_cfct_location_markers,
									L"Location object \"%s\" contains conforming parsing state %hd, action symbol %s(%hd), that does not correspond to any grammar conflict.",
									grm.location_objects[il3].location_name, conf_states[is3], grm.GetSymbolName(act_sym, buff), act_sym);
			}
		}
	}

	// Check for unexpected grammar conflicts.
	int num_unexpected_conflicts = 0;
	for (int icf2=0; icf2<grm.NumConflicts(); ++icf2)
	{
		TGrammarConflict &cfct = grm.conflicts[icf2];
		if (cfct.expected_conflict_inx == -1)
		{
			wchar_t buff[40];
			ReportConversionWarning(gerr_cfct_location_markers,
									L"Unexpected grammar conflict C%d. State: %hd, action symbol: %s(%hd).",
									icf2, cfct.parser_state, grm.GetSymbolName(cfct.input_symbol, buff), cfct.input_symbol);

			num_unexpected_conflicts++;
		}
	}

	if (num_unexpected_conflicts != 0)
	{
		// Current grammar has unexpected conflicts.
		ReportMessage(gerr_conversion_rprt, L"Num unexpected conflicts: %d", num_unexpected_conflicts);
	}

	// Do not continue if errors are present above.
	if (m_scanner->GetErrorsCount() != 0)
		return;

	//
	//   Step 3.
	//
	//  Gerenate resolution data in the grammar conflicts. Resolution data is present only in those conflicts, that belong
	//  to expected conflicts that have nested grammars.
	//
	//  Resolution data describes what action should be taken once nested parsing comes to some particular resolution rule.
	//  The action can be either the nested result (i.e. call the callback handler) or it can be one of the conflicting actions.
	//
	for (int icf3=0; icf3<grm.NumConflicts(); ++icf3)
	{
		TGrammarConflict &cfct = grm.conflicts[icf3];
		if (cfct.expected_conflict_inx < 0)
		{
			// Current conflict has no associated xpct conflict or it is ambiguous.
			continue;
		}

		// Check if expected conflict has nested grammar or not.
		TExpectedGrammarConflict &xpct = grm.xpct_conflicts[cfct.expected_conflict_inx];
		if (xpct.nested_grammar == NULL)
		{
			// Expected conflict has no nested grammar.
			continue;
		}

		// Once the nested grammar is present, the resolution should be non empty.
		assert(xpct.resolution.NumItems() > 0);

		//
		//  Each element of the resolution array describes one target rule in the nested grammar or, in other words,
		//  one possible result of the nested parsing. It is necessary to correlate all possible results of the nested parsing
		//  to the conflicting actions of the current grammar conflict.
		//

		// Iterate target rules of the nested grammar.
		bool nest_res_target_present = FALSE;
		for (int itarg=0; itarg<xpct.resolution.NumItems(); ++itarg)
		{
			// Allocate slot in the resolutions array of the grammar conflict. Place there initial value 0xFFFF (syntax error).
			// It will be replaced with the meaningful value later.
			WORD initial_res_val = 0xFFFF;
			if (cfct.conflict_resolution.AppendItem(initial_res_val) == FALSE)
			{
				ReportConversionError(gerr_cfct_location_markers, L"Out of memory while generating grammar conflict resolutions array.");
				return;
			}

			// Pick up the list of action codes, that is associated with the current target rule. This list should be non empty.
			TSymbolsArray &res = xpct.resolution[itarg];
			assert(res.NumItems() > 0);

			int applicable_icode = -1;
			for (int icode=0; icode<res.NumItems(); ++icode)
			{
				WORD action_code = res[icode];
				if (action_code == 0)
				{
					// This is action shift. This action can be present on as first conflicting action.
					WORD act0 = cfct.GetAction(0);
					if (act0 < actb_reduce)
					{
						// Current conflict is the shift/reduce conflict.
						if (applicable_icode < 0)
						{
							// Fill in the resolution slot with the shift action.
							cfct.conflict_resolution[itarg] = act0;
							applicable_icode = icode;
						}
						else
						{
							// Something is already picked up.
							ReportActionCodesAmbiguity(grm, icf3, xpct, itarg, applicable_icode, icode);
						}
					}
				}
				else
				{
					// Look for the object with this app_id. This can be either the grammar rule or the nested processing result.
					int nested_ires = grm.FindNestedResultByAppId(action_code);
					if (nested_ires >= 0)
					{
						// This is nested result.
						assert(res.NumItems() == 1);
						cfct.conflict_resolution[itarg] = actb_nested_result+nested_ires;
						nest_res_target_present = TRUE;
						applicable_icode = icode;
					}
					else
					{
						// This should be the id of the rule. Pick up this rule.
						int irule = grm.FindRuleByAppId(action_code);
						assert(irule >= 0);

						// Look for this rule among the conflicting actions.
						for (int iac1=0; iac1<cfct.NumActions(); ++iac1)
						{
							if (cfct.GetAction(iac1) == actb_reduce+irule)
							{
								if (applicable_icode < 0)
								{
									// This is the first match. Fill in the action and save icode.
									cfct.conflict_resolution[itarg] = (WORD)(actb_reduce+irule);
									applicable_icode = icode;
								}
								else
								{
									// This not the first match.
									ReportActionCodesAmbiguity(grm, icf3, xpct, itarg, applicable_icode, icode);
								}
							}
						}
					}
				}
			}

			if (applicable_icode == -1)
			{
				// Conforming reduce action is missing.
				ReportConversionError(gerr_cfct_location_markers,
									L"Unable to assign resolution action to target rule R%d in the nested grammar, generated for grammar conflict C%d, location object \"%s\", that belong to expected conflict \"%s\".",
									itarg, icf3, grm.location_objects[cfct.conflict_location_inx].location_name, xpct.xpct_conflict_name);
			}
		}

		if (nest_res_target_present == FALSE)
		{
			// Nested processing result is missing in the list of resolution actions of the current grammar conflict.
			// This means that this conflict will be resolved automatically without involving the callback handler.
			// Ensure that all conflicting actions are present in the resolution vector that was generated above.
			for (int iac2=0; iac2<cfct.NumActions(); ++iac2)
			{
				bool action_present = FALSE;
				for (int ires2=0; ires2<cfct.conflict_resolution.NumItems(); ++ires2)
				{
					if (cfct.GetAction(iac2) == cfct.conflict_resolution[ires2])
					{
						action_present = TRUE;
						break;
					}
				}

				if (action_present == FALSE)
				{
					wchar_t buff40[40];
					ReportConversionWarning(gerr_cfct_location_markers,
											L"Conflicting action \"%s\" is not present in the resolution vector for grammar conflict C%d, that belong to expected conflict \"%s\".",
											grm.GetParsingActionName(cfct.GetAction(iac2), buff40), icf3, xpct.xpct_conflict_name);
				}
			}
		}
	}

	// Once resolution arrays are generated, check if all rule ids that were mentioned in the resolution sections
	// of expected conflicts were used in resolition actions of at least one grammar conflict. Iterate all expected conflicts.
	for (int ixp1=0; ixp1<grm.xpct_conflicts.NumItems(); ++ixp1)
	{
		TExpectedGrammarConflict &xpct = grm.xpct_conflicts[ixp1];
		if (xpct.nested_grammar == NULL)
			continue;

		// Current conflict has nested grammar. Iterate its resolution slots.
		for (int ita1=0; ita1<xpct.resolution.NumItems(); ++ita1)
		{
			TSymbolsArray &res = xpct.resolution[ita1];
			assert(res.NumItems() > 0);

			if (res[0] == 0)
			{
				// Ignore this resolution slot for shift action.
				continue;
			}

			if (grm.FindNestedResultByAppId(res[0]) >= 0)
			{
				// Ignore this nested result slot.
				continue;
			}

			// Current resolution slot is list of rule ids.
			for (int ires3=0; ires3<res.NumItems(); ++ires3)
			{
				int irule = grm.FindRuleByAppId(res[ires3]);
				assert(irule >= 0);

				// Look for this rule among all grammar conflicts that belong to the current expected conflict.
				bool rule_id_used = FALSE;
				int iconflict = grm.GetFirstGrammarConflictInx(ixp1);
				while (iconflict >= 0 && rule_id_used == FALSE)
				{
					TGrammarConflict &cfct = grm.conflicts[iconflict];
					for (int ires4=0; ires4<cfct.conflict_resolution.NumItems(); ++ires4)
					{
						if (cfct.conflict_resolution[ires4] == actb_reduce+irule)
						{
							rule_id_used = TRUE;
							break;
						}
					}

					iconflict = grm.GetNextGrammarConflictInx(ixp1, iconflict);
				}

				if (rule_id_used == FALSE)
				{
					// Send warning about this unused rule.
					ReportConversionWarning(gerr_cfct_location_markers,
											L"Rule with id %d was mentioned as resolution action in expected conflict \"%s\". But it was not used for creating resolving action in any grammar conflict.",
											res[ires3], xpct.xpct_conflict_name);
				}
			}
		}
	}

	// Report final results of the grammar conflicts discovery.
	if (m_scanner->GetCurrTraceMode() & gdtr_reports)
	{
		GenerateConflictsReport(file_name_buffer, grammar_name_buffer, grm);
	}

}

bool TGrammarDefinitionParser::CheckLocObjectPosition(TGrammar &grm, TSymbolsArray &states_mask, TRulePosition &pos)
{
	assert(pos.mark == FALSE);

	// Allocate variable for the overall result.
	bool conforming_states_present = FALSE;

	WORD irule = pos.irule;
	WORD isym = pos.isym;

	// Look for states that the passed position may have.
	TParsingStatesIterator iter1;
	for (iter1.Setup(&grm); iter1; ++iter1)
	{
		// Allocate flag that will show the result of positions iteration.
		bool state_valid_for_position = FALSE;

		// Iterate positions of the current parsing state.
		WORD istate = iter1.CurrStateIndex();
		TParsingStatePositionsIterator iter2;
		for (iter2.Setup(grm.parsing_states[istate]); iter2; ++iter2)
		{
			if (iter2.CurrPos().irule != irule || iter2.CurrPos().isym != isym)
				continue;

			// Passed rule position belongs to the current parsing state.
			if (pos.action_sym == 0xFFFF)
			{
				// This is an intermediate rule position. Current state is the conforming parsing state.
				state_valid_for_position = TRUE;
				break;
			}

			// This is final rule position.
			if (iter2.CurrPos().action_sym != 0xFFFF)
			{
				// This is LR case. Action symbol is stored in the position itself. Compare action symbols.
				if (iter2.CurrPos().action_sym == pos.action_sym)
				{
					// Symbol from location marker is one of the action symbols of the current rule.
					state_valid_for_position = TRUE;
					break;
				}
			}
			else
			{
				// This is SLR case. Rule has set of followers that are stored as a mask in a row of the followers table.
				WORD non_term = grm.rules[irule].non_term;
				WORD *row_ptr = m_firsts_and_follows.GetNonTermRowPtr(non_term);
				if ((row_ptr[pos.action_sym] & FUN_FOLLOWS_MASK) != 0)
				{
					// Symbol from location marker is one of the followers of the rule non terminal.
					state_valid_for_position = TRUE;
					break;
				}
			}
		}

		if (state_valid_for_position == TRUE)
		{
			// Passed rule position belongs to the current parsing state.
			conforming_states_present = TRUE;
		}
		else
		{
			// Remove current state from the intersection mask. Mask should contain only valid parsing states.
			states_mask[istate] = FALSE;
		}
	}

	return(conforming_states_present);
}

void TGrammarDefinitionParser::ReportActionCodesAmbiguity(TGrammar &grm, int iconflict, TExpectedGrammarConflict &xpct, int itarg, int icode1, int icode2)
{
	int iloc = grm.conflicts[iconflict].conflict_location_inx;
	assert(iloc >= 0);

	ReportConversionError(gerr_cfct_location_markers,
						L"Grammar conflict C%d, location object \"%s\", that belongs to expected conflict \"%s\", has ambiguous action codes.",
						iconflict, grm.location_objects[iloc].location_name, xpct.xpct_conflict_name);

	wchar_t *targ_name = NULL,  targ_name_buff[40];
	TGrammarRule &res_rule = xpct.nested_grammar->rules[itarg];
	if (res_rule.Length() == 0)
		targ_name = L"<EmptySym>";
	else targ_name = grm.GetSymbolName(res_rule.symbols[0], targ_name_buff);

	TSymbolsArray &res = xpct.resolution[itarg];
	wchar_t code1_buff[80], code2_buff[80];

	ReportConversionError(gerr_cfct_location_markers,
						L"These ambiguous codes belong to the nested resolution target %s (R%d). These action codes are %s and %s.",
						targ_name, itarg, PreapeResolutionActionCode(grm, res[icode1], code1_buff), PreapeResolutionActionCode(grm, res[icode2], code2_buff));
}

const wchar_t *TGrammarDefinitionParser::PreapeResolutionActionCode(TGrammar &grm, WORD code, wchar_t *buffer_80_chars)
{
	if (code == 0)
		return(L"Shift");

	// This should be app id of the rule.
	int irule = grm.FindRuleByAppId(code);
	assert(irule >= 0);

	swprintf(buffer_80_chars, 80, L"R%d (AppId=%hu)", irule, code);
	return(buffer_80_chars);
}

void TGrammarDefinitionParser::GenerateConflictsReport(TFileNameBuffer &file_name_buff, const wchar_t *grm_name, TGrammar &grm)
{
	if (PrepareReporsDirectory(TRUE) == FALSE)
		return;

	int grm_index = grm.GetGrammarIndex();
	if (grm.NumConflicts() == 0)
	{
		// There are no conflicts in the grammar.
		DeleteReportFile(GenerateReportFileName(file_name_buff, grm_name, grm_index, CONFLICTS_INFO_FNAME));
		return;
	}

	// Create non empty report in the disk file.
	TGrammarDefnParserReportFile rprt(this, grm_name, grm_index, grm.NumTerminals(), grm.NumNonTerminals(), grm.NumRules(),
									GenerateReportFileName(file_name_buff, grm_name, grm_index, CONFLICTS_INFO_FNAME));

	if (grm.NumXpctConflicts() > 0)
	{
		// Iterate expected conflicts.
		rprt.WriteLine();
		for (int ix=0; ix<grm.NumXpctConflicts(); ++ix)
		{
			rprt.WriteFmtLine(L"  Expected conflict X%d. Name: %s, NumGrammarConflicts: %d.", ix, grm.xpct_conflicts[ix].xpct_conflict_name, grm.GetNumGrammarConflicts(ix));
			TGrammarSymbolsHelper::WriteReportDelimiter(rprt, TRUE);

			int icon = grm.GetFirstGrammarConflictInx(ix);
			if (icon >= 0)
			{
				rprt.Write(L"     Conflicts: ");
				bool first_conflict = TRUE;

				while (icon >= 0)
				{
					rprt.WriteFmt(L"%sC%d", ((first_conflict == TRUE) ? L"" : L", "), icon);
					icon = grm.GetNextGrammarConflictInx(ix, icon);
					first_conflict = FALSE;
				}

				rprt.WriteLine(L".");
			}
			else
			{
				rprt.WriteLine(L"     Participating grammar conflicts are missing.");
			}

			// Add empty line between conflicts.
			rprt.WriteLine();
		}
	}

	// Iterate conflicts.
	rprt.WriteLine();
	for (int ic=0; ic<grm.NumConflicts(); ++ic)
	{
		wchar_t buff_40_chars[40];
		TGrammarConflict &grcf = grm.conflicts[ic];
		rprt.WriteFmtLine(L"  Grammar conflict %d. Parsing state: %d, symbol: %s(%hd).",
						ic, grcf.parser_state, grm.GetSymbolName(grcf.input_symbol, buff_40_chars), grcf.input_symbol);
		TGrammarSymbolsHelper::WriteReportDelimiter(rprt);

		// Iterate possible conflicting actions.
		assert(grcf.conflicting_actions.NumItems() > 0);
		for (int ir=0; ir<grcf.conflicting_actions.NumItems(); ++ir)
		{
			WORD action =  grcf.conflicting_actions[ir];
			if (action < actb_reduce)
			{
				// Action is "shift to other parsing state".
				assert(action < grm.NumParsingStates());
				rprt.WriteFmtLine(L"     Action %d: Shift to state %hd.", ir+1, action);
			}
			else
			{
				// Action is "apply rule".
				int irule = action-actb_reduce;
				assert(irule >= 0 && irule < grm.NumRules());
				WORD rule_non_term = grm.rules[irule].non_term;
				rprt.WriteFmtLine(L"     Action %d: Reduce using rule %d (%s.%d).", ir+1, irule, grm.GetSymbolName(rule_non_term, buff_40_chars),
								TGrammarSymbolsHelper::GetRuleOffs(grm.rules, irule));
			}
		}

		short ixpct = grcf.expected_conflict_inx;
		if (ixpct >= 0)
		{
			rprt.WriteFmtLine(L"     XpctConflict: %s.", grm.xpct_conflicts[ixpct].xpct_conflict_name);
		}
		else if (ixpct == -1)
		{
			rprt.WriteFmtLine(L"     UNEXPECTED CONFLICT");
		}
		else
		{
			rprt.WriteFmtLine(L"     Xpct, but it is ambiguous between %d expected conflicts.", -ixpct);
		}

		int num_reses = grcf.conflict_resolution.NumItems();
		if (num_reses > 0)
		{
			// Write the resolution array.
			wchar_t buff40[40];
			rprt.Write(L"     Resolution: ");
			for (int ir=0; ir<num_reses; ++ir)
				rprt.WriteFmt(L"%s%s", grm.GetShortParsingActionName(grcf.conflict_resolution[ir], buff40), (ir != num_reses-1) ? L", " : L".");
			rprt.WriteLine(L"");
		}

		// Add empty line between conflicts.
		rprt.WriteLine();
	}

	rprt.CloseReport(L"grammar conflicts report");
}

//---------------------------------------------------------------------------
//  ============  TGrammarDefinitionParserHelper  =====================
//---------------------------------------------------------------------------

TGrammarDefinitionParserHelper::TGrammarDefinitionParserHelper(TMidLevScanCbkHandler &midlev_cbk, TGrammar &dest_grammar, const wchar_t *dir_for_reports, bool build_parsing_table)
			: m_parser(dest_grammar, dir_for_reports, NULL, build_parsing_table), m_driver(m_parser, midlev_cbk)
{
}

bool TGrammarDefinitionParserHelper::ParseGrammarDefinitionFile(const wchar_t *src_fname, DWORD tracing_flags, int *num_syntax_errors)
{
	if (tracing_flags != 0)
	{
		// Some non empty tracing is requested.
		m_driver.Scanner().SetupTraceMode(tracing_flags, TRUE);
	}

	// Process the file in C/C++ mode.
	bool res = m_driver.ProcessSourceFile(0, lang_grammar_def, src_fname);
	if (num_syntax_errors != NULL)
		*num_syntax_errors = m_driver.Scanner().GetErrorsCount();

	return(res);
}


