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
#include  "TextScan/GrammarStructure.H"

//----------------------------------------------------------------------------
//  ====================  TRulePosition  ===========================
//----------------------------------------------------------------------------

int __cdecl TRulePosition::CompareLight(const TRulePosition *rp1, const TRulePosition *rp2)
{
	// Compare rule indexes.
	if (rp1->irule > rp2->irule)
		return(1);
	if (rp1->irule < rp2->irule)
		return(-1);

	// Compare symbol positions inside the rule.
	if (rp1->isym > rp2->isym)
		return(1);
	if (rp1->isym < rp2->isym)
		return(-1);

	// Positions are identical.
	return(0);
}

//------------------------------------------------------------------------
//  ==================  TFullRulePosition  ========================
//------------------------------------------------------------------------

static const wchar_t *g_FullRulePosOriginEnumName[rpo_num_types] =
{
	L"rpo_initial_placement",
	L"rpo_rule_start_state",
	L"rpo_rule_call_place",
	L"rpo_step_over_symbol",

	L"rpo_step_over_non_term",
	L"rpo_non_term_defn_rule",
	L"rpo_step_up_the_tree",
	L"rpo_axioma_action_sym",

	L"rpo_start_state_ctx",
	L"rpo_call_place_ctx",
	L"rpo_step_over_ctx",
	L"rpo_step_up_ctx",
	L"rpo_axioma_action_ctx",
};

int __cdecl TFullRulePosition::CompareLight(const void *pp1, const void *pp2)
{
	TFullRulePosition &p1 = *((TFullRulePosition*)pp1);
	TFullRulePosition &p2 = *((TFullRulePosition*)pp2);

	// Compare the rule indexes.
	if (p1.irule > p2.irule)
		return(1);
	if (p1.irule < p2.irule)
		return(-1);

	// Compare the symbol positions inside the rule.
	if (p1.isym > p2.isym)
		return(1);
	if (p1.isym < p2.isym)
		return(-1);

	// Compare the position origins.
	if (p1.origin > p2.origin)
		return(1);
	if (p1.origin < p2.origin)
		return(-1);

	// Positions are identical.
	return(0);
}

wchar_t *TFullRulePosition::GetFullRulePosOriginEnumName(TFullRulePosOrigin value)
{
	if (value >= 0 && value < rpo_num_types)
		return((wchar_t*)g_FullRulePosOriginEnumName[value]);

	assert(FALSE);
	return(L"TFullRulePosOrigin_BogusValue");
}

TFullRulePosOrigin TFullRulePosition::GetFullRulePosOriginFromStringPtr(TStringPtr &data)
{
	for (int inx=0; inx<rpo_num_types; ++inx)
	{
		if (data == g_FullRulePosOriginEnumName[inx])
		{
			return((TFullRulePosOrigin)inx);
		}
	}

	// Passed string does not correspond to any name of the position origin.
	return(rpo_num_types);
}

//-------------------------------------------------------------------------
//  ==============  TConflictingActionAnalysisResults  ==================
//-------------------------------------------------------------------------

const wchar_t *g_ConflictingActionAnalysisGenResultEnumNames[TConflictingActionAnalysisResults::gres_num_types] =
{
	L"gres_not_started",

	L"gres_proc_running",
	L"gres_aborted_by_user",
	L"gres_timeout",

	L"gres_ok",

	L"gres_no_paths_found",
	L"gres_out_of_memory",
	L"gres_too_many_paths",
};

const wchar_t *TConflictingActionAnalysisResults::GetGenResultEnumName(TGenResult value)
{
	if (value >= 0 && value < gres_num_types)
		return(g_ConflictingActionAnalysisGenResultEnumNames[value]);

	assert(FALSE);
	return(L"TGenResult_BogusVal");
}

const wchar_t *TConflictingActionAnalysisResults::GetGenResultDescription(TGenResult value, bool want_first_cap)
{
	switch (value)
	{
		case gres_not_started:			return((want_first_cap == TRUE) ? L"Not started" : L"not started");

		case gres_proc_running:		return((want_first_cap == TRUE) ? L"= Running =" : L"= running =");
		case gres_aborted_by_user:	return((want_first_cap == TRUE) ? L"Aborted by user" : L"aborted by user");
		case gres_timeout:			return((want_first_cap == TRUE) ? L"Timeout" : L"timeout");

		case gres_ok:				return(L"Ok");

		case gres_no_paths_found:		return((want_first_cap == TRUE) ? L"No paths found" : L"no paths found");
		case gres_out_of_memory:		return((want_first_cap == TRUE) ? L"Out of memory" : L"out of memory");
		case gres_too_many_paths:		return((want_first_cap == TRUE) ? L"Too many paths" : L"too many paths");
	}

	assert(FALSE);
	return(L"TGenResult_BogusValue");
}

TConflictingActionAnalysisResults::TGenResult TConflictingActionAnalysisResults::GetGenResultFromStringPtr(TStringPtr &data)
{
	for (int inx=0; inx<gres_num_types; ++inx)
	{
		if (data == g_ConflictingActionAnalysisGenResultEnumNames[inx])
		{
			return((TGenResult)inx);
		}
	}

	return(gres_num_types);
}

//---------------------------------------------------------------------------
//  =================  TGrammarSymbolsHelper  =====================
//---------------------------------------------------------------------------

WORD TGrammarSymbolsHelper::AddTerminalSymbol(TTerminalSymbolsArray &dest, bool sym_type, __int64 sym_num_val, TStrPtrInfo &sym_str_val_ptr, WORD app_id,
													TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt, ID event_id)
{
	TTerminalSymbol new_term_symbol =
	{
		sym_type, sym_num_val, ((sym_str_val_ptr.IsEmpty() == FALSE) ? TGrammar::StrDupe(sym_str_val_ptr) : NULL),
		app_id, lex_type, check_subt, lex_subt.subtype,
		event_id,
	};

	if (sym_str_val_ptr.IsEmpty() == FALSE && new_term_symbol.name_value == NULL)
	{
		// Out of memory while duplicating name of the terminal symbol.
		return(0);
	}

	if (dest.AppendItem(new_term_symbol) == FALSE)
	{
		// Out of memory on expanding list of terminal symbols or this list is in the read only mode.
		TGrammar::FreeStr(new_term_symbol.name_value);
		return(0);
	}

	// Success.
	return((WORD)(dest.NumItems()-1));
}

bool TGrammarSymbolsHelper::AddIgnoreLexRecord(TIgnoreLexRecordsArray &dest, TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt)
{
	TIgnoreLexRecord new_ignore_lex = { lex_type, check_subt, lex_subt.subtype, };

	if (dest.AppendItem(new_ignore_lex) == FALSE)
	{
		// Out of memory on expanding list of ignore lex records or this list is in  the read only mode.
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TGrammarSymbolsHelper::AddErrorLexRecord(TErrorLexRecordsArray &dest, TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt, TStrPtrInfo &error_msg_ptr)
{
	assert(error_msg_ptr != NULL);
	TErrorLexRecord new_error_lex = { lex_type, check_subt, lex_subt.subtype, TGrammar::StrDupe(error_msg_ptr), };

	if (new_error_lex.error_message == NULL)
	{
		// Out of memory while duplicating the error message.
		return(FALSE);
	}

	if (dest.AppendItem(new_error_lex) == FALSE)
	{
		// Out of memory on expanding list of error lex records or this list is in the read only mode.
		TGrammar::FreeStr(new_error_lex.error_message);
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

WORD TGrammarSymbolsHelper::AddNonTerminalSymbol(TNonTerminalsArray &dest, TStrPtrInfo &name_ptr, WORD app_id, WORD rules_sect, ID event_id)
{
	TNonTerminalSymbol new_non_terminal =
	{
		TGrammar::StrDupe(name_ptr),	// Non term name is the first field.
		app_id,							// Application specific Id.
		rules_sect,						// Current rules section or unknown section.
		FALSE,							// Non public symbol.
		ntrs_none,						// Symbol is not restricted.
		TRUE,							// Setup non term as solid. This will be corrected later.
		event_id,
	};

	if (new_non_terminal.symbol_name == NULL)
	{
		// Out of memory while duplicating the non terminal name.
		return(0);
	}

	if (dest.AppendItem(new_non_terminal) == FALSE)
	{
		// Out of memory on expanding list of non terminals or this list is in the read only mode.
		TGrammar::FreeStr(new_non_terminal.symbol_name);
		return(0);
	}

	// Success.
	return((WORD)(NonTerminalsBase+dest.NumItems()-1));
}

bool TGrammarSymbolsHelper::AddGrammarRule(TGrammarRulesArray &dest, WORD rule_non_term, TSymbolsArray &rhs, WORD rule_app_id, const wchar_t *rule_app_id_name, ID nt_event_id, ID body_event_id)
{
	wchar_t *app_id_name = NULL;
	if (rule_app_id_name != NULL && rule_app_id_name[0] != 0)
	{
		app_id_name = TGrammar::StrDupe(rule_app_id_name);
		if (app_id_name == NULL)
			return(FALSE);
	}

	// Allocate rule proto with empty right hand side and append it.
	TGrammarRule new_rule =
	{
		rule_non_term, TSymbolsArray(), rule_app_id, app_id_name, nt_event_id, body_event_id,
	};

	if (dest.AppendItem(new_rule) == FALSE)
	{
		if (app_id_name != NULL)
			TGrammar::FreeStr(app_id_name);
		return(FALSE);
	}

	// Fill in the right hand side. This call will move data from the parameter to the new rule structure.
	// This means that there will be no allocation of any new resource here.
	dest[dest.NumItems()-1].symbols.TakeContentsFrom(rhs);

	// Success.
	return(TRUE);
}

bool TGrammarSymbolsHelper::AddNestedProcResult(TNestedProcessingResultsArray &dest, WORD nres_app_id, const wchar_t *nres_name, ID event_id)
{
	wchar_t *local_name = NULL;
	if (nres_name != NULL && nres_name[0] != 0)
	{
		local_name = TGrammar::StrDupe(nres_name);
		if (local_name == NULL)
			return(FALSE);
	}

	TNestedProcessingResult nested_res = { nres_app_id, local_name, event_id };
	if (dest.AppendItem(nested_res) == FALSE)
	{
		if (local_name != NULL)
			TGrammar::FreeStr(local_name);
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TGrammarSymbolsHelper::AddExpectedConflict(TExpectedGrammarConflictsArray &dest, const wchar_t *xpct_name, WORD xpct_app_id, int cnt_locations, ID event_id)
{
	// Allocate a record proto.
	TExpectedGrammarConflict new_xpct_conflict = { TGrammar::StrDupe(xpct_name), xpct_app_id, TSymbolsArray(), 0, TSymbolsArrayArray(), NULL, event_id };

	// Check the result of the name allocation.
	if (new_xpct_conflict.xpct_conflict_name == NULL)
		return(FALSE);

	// Append the record.
	if (dest.AppendItem(new_xpct_conflict) == FALSE)
	{
		TGrammar::FreeStr(new_xpct_conflict.xpct_conflict_name);
		return(FALSE);
	}

	TExpectedGrammarConflict &xcf = dest[dest.NumItems()-1];
	for (int cnt=0; cnt<cnt_locations; ++cnt)
	{
		WORD ref = 0xFFFF;
		if (xcf.xpct_conflict_locations.AppendItem(ref) == FALSE)
		{
			dest.ReleaseLastItem();
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

WORD TGrammarSymbolsHelper::FindSymbol(TTerminalSymbolsArray &term_syms, TNonTerminalsArray &non_terms, TLexema &lex)
{
	if (lex.type != ltx_charconst && lex.type != ltx_name)
		return(0);

	// Look among terminal symbols. Terminal can be either charconst or name.
	for (WORD i1=TerminalSymbolsBase+1; i1<term_syms.NumItems(); ++i1)
	{
		if (term_syms[i1].sym_type_name == TRUE)
		{
			// Current symbol is name.
			if (lex.type == ltx_name && lex.str_value == term_syms[i1].name_value)
			{
				return(TerminalSymbolsBase+i1);
			}
		}
		else
		{
			// Current symbol is charconst.
			if (lex.type == ltx_charconst && lex.num_value == term_syms[i1].char_value)
			{
				return(TerminalSymbolsBase+i1);
			}
		}
	}

	// Look among non terminals. These can be only names.
	if (lex.type == ltx_name)
	{
		for (WORD i2=0; i2<non_terms.NumItems(); ++i2)
		{
			if (lex.str_value == non_terms[i2].symbol_name)
			{
				return(NonTerminalsBase+i2);
			}
		}
	}

	// Passed lexema does not correspond to any known symbol.
	return(0);
}

WORD TGrammarSymbolsHelper::FindSymbol(TTerminalSymbolsArray &term_syms, TNonTerminalsArray &non_terms, TStrPtrInfo &name_ptr)
{
	for (WORD i1=TerminalSymbolsBase+1; i1<term_syms.NumItems(); ++i1)
	{
		if (term_syms[i1].sym_type_name == TRUE && name_ptr == term_syms[i1].name_value)
		{
			return(TerminalSymbolsBase+i1);
		}
	}

	for (WORD i2=0; i2<non_terms.NumItems(); ++i2)
	{
		if (name_ptr == non_terms[i2].symbol_name)
		{
			return(NonTerminalsBase+i2);
		}
	}

	return(0);
}

int TGrammarSymbolsHelper::FindLocationObject(TExpectedConflictLocationsArray &locs_array, TStrPtrInfo &loc_object_name)
{
	// Iterate registered expected conflict locations.
	for (int i=0; i<locs_array.NumItems(); ++i)
	{
		if (loc_object_name == locs_array[i].location_name)
		{
			return(i);
		}
	}

	// Object is not existing.
	return(-1);
}

wchar_t *TGrammarSymbolsHelper::GetSymbolName(TTerminalSymbolsArray &term_syms, TNonTerminalsArray &non_terms, WORD sym, wchar_t *buff_40_chars)
{
	if (sym < term_syms.NumItems())
	{
		if (term_syms[sym].sym_type_name == FALSE)
		{
			// This is char const based terminal symbol. Lexema stores chars of the multi character charconsts in the num_val field
			// in the normal order. Since the current target platform has intel byte order, sequence of chars need reversion.
			int i = 40;
			buff_40_chars[--i] = 0;
			buff_40_chars[--i] = L'\'';
			__int64 val = term_syms[sym].char_value;

			int cnt = 0;
			while ((val & 0xFFFF) != 0 && cnt++ < 4)
			{
				buff_40_chars[--i] = (wchar_t)val;
				val >>= 16;
			}

			buff_40_chars[--i] = L'\'';
			return(buff_40_chars+i);
		}
		else
		{
			// This is name based terminal symbol. Simply return the name.
			return(term_syms[sym].name_value);
		}
	}
	else if (sym >= NonTerminalsBase && sym < NonTerminalsBase+non_terms.NumItems())
	{
		// Symbol value falls into the non terminals range.
		return(non_terms[sym-NonTerminalsBase].symbol_name);
	}
	else
	{
		// All other values are not considered as symbols.
		swprintf(buff_40_chars, 40, L"BogusSymValue(%hd)", sym);
		return(buff_40_chars);
	}
}

int TGrammarSymbolsHelper::GetFirstRuleForNonTerm(TGrammarRulesArray &rule_defns, WORD sym)
{
	for (int i=0; i<rule_defns.NumItems(); ++i)
	{
		if (rule_defns[i].non_term == sym)
			return(i);
	}

	// There are no rules for the passed non terminal.
	return(-1);
}

int TGrammarSymbolsHelper::GetRuleOffs(TGrammarRulesArray &rule_defns, int rule_inx)
{
	int offs = 1;
	WORD non_term = rule_defns[rule_inx].non_term;

	// Add number of rules for this non terminal that stay before the rule with the passed index.
	for (int irule=0; irule<rule_inx; ++irule)
	{
		if (rule_defns[irule].non_term == non_term)
			offs++;
	}

	return(offs);
}

bool TGrammarSymbolsHelper::CreateSortedTerminalSymbolsList(TTerminalSymbolsArray &term_sym_defns, TSymbolsArray &term_syms_list, bool sort_alphabetically)
{
	// Reserve space for all terminal symbols.
	term_syms_list.SetNumItems(0);
	if (term_syms_list.ReserveSpace(term_sym_defns.NumItems()) == FALSE)
		return(FALSE);

	// Fill in the array.
	int sym_limit = TerminalSymbolsBase+term_sym_defns.NumItems();
	for (WORD sym=TerminalSymbolsBase; sym < sym_limit; ++sym)
	{
		term_syms_list.AppendItem(sym);
	}

	// Sort this list and return success.
	if (sort_alphabetically == TRUE)
		SortSymbols(&term_sym_defns, NULL, NULL, term_syms_list, ntsm_alpha_num);
	return(TRUE);
}

bool TGrammarSymbolsHelper::CreateSortedNonTerminalsList(TNonTerminalsArray &non_term_defns, TGrammarRulesArray *rules, TSymbolsArray &non_terms_list, TNonTerminalsSortOrder sorting_mode, int rules_sect_restr)
{
	// Reserve space for all non terminals.
	non_terms_list.SetNumItems(0);
	if (non_terms_list.ReserveSpace(non_term_defns.NumItems()) == FALSE)
		return(FALSE);

	// Fill in the array.
	int sym_limit = NonTerminalsBase+non_term_defns.NumItems();
	for (WORD sym=NonTerminalsBase; sym < sym_limit; ++sym)
	{
		if (rules_sect_restr <= 0 || non_term_defns[sym-NonTerminalsBase].rules_sect == rules_sect_restr)
			non_terms_list.AppendItem(sym);
	}

	// Sort this list and return success.
	if (sorting_mode != ntsm_sym_val)
		SortSymbols(NULL, &non_term_defns, rules, non_terms_list, sorting_mode);
	return(TRUE);
}

void TGrammarSymbolsHelper::SortAndDumpNonTerminalsList(TDestinationFile &rprt, TNonTerminalsArray &non_term_defns, TGrammarRulesArray *rules, TSymbolsArray &non_terms_list, TNonTerminalsSortOrder sorting_mode)
{
	// Sort the list.
	SortSymbols(NULL, &non_term_defns, rules, non_terms_list, sorting_mode);

	// Use the sort order to select the dumping format.
	switch (sorting_mode)
	{
		case ntsm_sym_val:
		case ntsm_alpha_num:
				{
					// Dump list of symbols without highlighting anything.
					rprt.WriteLine();
					if (sorting_mode == ntsm_sym_val)
						rprt.WriteLine(L"  Non terminals in the symbol value order");
					else rprt.WriteLine(L"  Non terminals in the alphabetical order");

					WriteReportDelimiter(rprt);

					for (int i1=0; i1<non_terms_list.NumItems(); ++i1)
					{
						WORD sym = non_terms_list[i1];
						wchar_t *name = non_term_defns[sym-NonTerminalsBase].symbol_name;
						rprt.WriteFmtLine(L" (sym=%d): %s", sym, name);
					}

					WriteReportDelimiter(rprt);
				}
				break;

		case ntsm_special_anum:
				{
					// Highlight change of the last word in the name.
					rprt.WriteLine();
					rprt.WriteLine(L"  Special non terminals groupping");

					wchar_t *prev_subn = NULL;
					for (int i2=0; i2<non_terms_list.NumItems(); ++i2)
					{
						WORD sym = non_terms_list[i2];
						wchar_t *name = non_term_defns[sym-NonTerminalsBase].symbol_name;
						wchar_t *subn = name+wcslen(name)-1;
						while (subn > name && *subn >= 'a' && *subn <= 'z')
							subn--;

						if (prev_subn == NULL || wcscmp(prev_subn, subn) != 0)
							WriteReportDelimiter(rprt);

						rprt.WriteFmtLine(L" (sym=%d): %40s", sym, name);
						prev_subn = subn;
					}

					WriteReportDelimiter(rprt);
				}
				break;

		case ntsm_sect_rule:
				{
					// Highlight changes of the rule section.
					rprt.WriteLine();
					rprt.WriteLine(L"  Non terminals in the natural order");

					WORD prev_rules_sect = 0xFFFF;
					for (int i3=0; i3<non_terms_list.NumItems(); ++i3)
					{
						WORD sym = non_terms_list[i3];
						TNonTerminalSymbol &info = non_term_defns[sym-NonTerminalsBase];
						if (info.rules_sect != prev_rules_sect)
						{
							if (info.rules_sect == 0)
								rprt.WriteLine(L"---(UndefinedNonTerminals)---------------------------------");
							else rprt.WriteFmtLine(L"---(%02d)---------------------------------------------------", info.rules_sect);

							prev_rules_sect = info.rules_sect;
						}

						rprt.WriteFmtLine(L" (sym=%d): %s", sym, info.symbol_name);
					}

					WriteReportDelimiter(rprt);
				}
				break;
	}
}

void TGrammarSymbolsHelper::WriteReportDelimiter(TDestinationFile &rprt, bool long_delim)
{
	if (long_delim == TRUE)
		rprt.Write(L"-------------------------------");

	rprt.WriteLine(L"------------------------------------------------------------------");
}

void TGrammarSymbolsHelper::SortSymbols(TTerminalSymbolsArray *term_syms, TNonTerminalsArray *non_terms, TGrammarRulesArray *rules, TSymbolsArray &syms, TNonTerminalsSortOrder sorting_mode)
{
	if (syms.NumItems() > 1)
	{
		TSymbolsSortingCtx ctx = { term_syms, non_terms, rules, sorting_mode };
		syms.QuickSortWithCtx(&ctx, CompareGrammarSymbols);
	}
}

int __cdecl TGrammarSymbolsHelper::CompareGrammarSymbols(void *context, const WORD *ps1, const WORD *ps2)
{
	TSymbolsSortingCtx *ctx = (TSymbolsSortingCtx*)context;

	WORD sym1 = *ps1;
	WORD sym2 = *ps2;

	if (ctx->m_sort_mode == ntsm_sym_val)
	{
		// Return difference between the symbol values.
		return(((int)sym1)-((int)sym2));
	}

	// Symbols can be simply identical. Check for this.
	if (sym1 == sym2)
		return(0);

	// Check for different symbol types case.
	if (sym1 >= NonTerminalsBase && sym2 < NonTerminalsBase)
		return(1);
	if (sym1 < NonTerminalsBase && sym2 >= NonTerminalsBase)
		return(-1);

	if (sym1 < NonTerminalsBase)
	{
		assert(ctx->m_term_syms != NULL);
		TTerminalSymbolsArray &term_syms = *(ctx->m_term_syms);

		// Both symbols should be terminals. EnsureThis.
		assert(sym1 < term_syms.NumItems());
		assert(sym2 < term_syms.NumItems());

		// Check for special EOF case. EOF name should not be compared.
		if (sym1 == 0)
			return(-1);
		if (sym2 == 0)
			return(1);

		bool sym1_name = term_syms[sym1].sym_type_name;
		if (sym1_name != term_syms[sym2].sym_type_name)
		{
			// Terminal symbols have different types.
			return((sym1_name == TRUE) ? 1 : -1);
		}
		else
		{
			if (sym1_name == TRUE)
			{
				// Both terminals are names. Do special comparison for the first characters
				// to display the lowercased names first.
				wchar_t fch1 = term_syms[sym1].name_value[0];
				wchar_t fch2 = term_syms[sym2].name_value[0];
				if (fch1 >= L'A' && fch1 <= L'Z' && fch2 >= L'a' && fch2 <= L'z')
					return(1);
				if (fch1 >= L'a' && fch1 <= L'z' && fch2 >= L'A' && fch2 <= L'Z')
					return(-1);

				// Compare names lexicogaphically.
				return(wcscmp(term_syms[sym1].name_value, term_syms[sym2].name_value));
			}
			else
			{
				// Both terminals are char consts.
				if (term_syms[sym1].char_value > term_syms[sym2].char_value)
					return(1);
				if (term_syms[sym1].char_value < term_syms[sym2].char_value)
					return(-1);

				return(0);
			}
		}
	}
	else
	{
		assert(ctx->m_non_terms != NULL);
		TNonTerminalsArray &non_terms = *(ctx->m_non_terms);

		// Both symbols should be non terminals. Ensure this.
		assert(sym1 >= NonTerminalsBase && sym1 < NonTerminalsBase+non_terms.NumItems());
		assert(sym2 >= NonTerminalsBase && sym2 < NonTerminalsBase+non_terms.NumItems());

		wchar_t *sym1_name = non_terms[sym1-NonTerminalsBase].symbol_name;
		wchar_t *sym2_name = non_terms[sym2-NonTerminalsBase].symbol_name;
		if (ctx->m_sort_mode == ntsm_alpha_num)
		{
			// Use simple names comparison.
			return(wcscmp(sym1_name, sym2_name));
		}
		else if (ctx->m_sort_mode == ntsm_special_anum)
		{
			// Copy names to the local buffer and set pointers to last chars of the names.
			wchar_t buff1[TGrammar::MAX_NAME_LENGTH+1];
			wchar_t buff2[TGrammar::MAX_NAME_LENGTH+1];
			wcscpy(buff1, sym1_name);
			wcscpy(buff2, sym2_name);
			wchar_t *subn1 = buff1+wcslen(buff1)-1;
			wchar_t *subn2 = buff2+wcslen(buff2)-1;

			// Compare subwords of the names moving from the last subword to the first subword.
			for(;;)
			{
				while (subn1 > buff1 && *subn1 >= 'a' && *subn1 <= 'z')
					subn1--;
				while (subn2 > buff2 && *subn2 >= 'a' && *subn2 <= 'z')
					subn2--;

				int res = wcscmp(subn1, subn2);
				if (res != 0)
					return(res);

				// Subnames are identical.
				*subn1-- = 0;
				*subn2-- = 0;

				// Check, if something is present in front of the subwords or not.
				if (subn1 >= buff1 && subn2 < buff2)
					return(1);
				if (subn1 < buff1 && subn2 >= buff2)
					return(-1);
				if (subn1 < buff1 && subn2 < buff2)
					return(0);
			}
		}
		else
		{
			assert(ctx->m_sort_mode == ntsm_sect_rule);
			assert(ctx->m_rule_defns != NULL);

			// Compare rule sections.
			WORD sect1 = non_terms[sym1-NonTerminalsBase].rules_sect;
			WORD sect2 = non_terms[sym2-NonTerminalsBase].rules_sect;

			if (sect1 > sect2)
				return(1);
			if (sect1 < sect2)
				return(-1);

			// Section index is the same.
			if (sect1 != 0)
			{
				// Find out and compare the first rule indexes.
				int irule1 = GetFirstRuleForNonTerm(*(ctx->m_rule_defns), sym1);
				int irule2 = GetFirstRuleForNonTerm(*(ctx->m_rule_defns), sym2);
				return(irule1-irule2);
			}
			else
			{
				// These non terminals does not have rules that define them.
				// Compare the symbol values.
				return(((int)sym1)-((int)sym2));
			}
		}
	}
}

//---------------------------------------------------------------------------
//  ====================  TAnalysisTable  =========================
//---------------------------------------------------------------------------

bool TAnalysisTable::SetupTable(int num_states, int num_symbols)
{
	ReleaseTable();

	if (num_states < 50)
		num_states = 50;

	// Allocate and clear the analysis table.
	int len_alloc = num_states*num_symbols*sizeof(WORD);
	table_body = (WORD*)malloc(len_alloc);
	if (table_body == NULL)
		return(FALSE);

	memset(table_body, 0xFF, len_alloc);
	table_owned  = TRUE;
	table_width  = num_symbols;
	table_height = num_states;
	table_rows_used = 0;

	// Success.
	return(TRUE);
}

void TAnalysisTable::ReleaseTable()
{
	if (table_owned == TRUE && table_body != NULL)
	{
		free(table_body);
		table_body = NULL;
	}

	table_rows_used = 0;
}

void TAnalysisTable::TakeContentsFrom(TAnalysisTable &other_table)
{
	ReleaseTable();
	table_body = other_table.table_body;
	table_owned  = other_table.table_owned;
	table_width  = other_table.table_width;
	table_height = other_table.table_height;
	table_rows_used = other_table.table_rows_used;

	if (table_owned == TRUE)
		other_table.table_body = NULL;
}

bool TAnalysisTable::CheckRowExistence(int row_index)
{
	if (table_body == NULL || table_height < 0)
	{
		// This operation is not applicable to non owned tables.
		assert(FALSE);
		return(FALSE);
	}

	if (row_index < table_height)
	{
		// Row with this index is already existing.
		return(TRUE);
	}

	// Reallocate the existing table.
	int new_height = __max((3*table_height)/2, row_index+32);
	int new_len = new_height*table_width*sizeof(WORD);
	WORD *new_table = (WORD*)realloc(table_body, new_len);
	if (new_table == NULL)
		return(FALSE);

	// Save results and update the data fields.
	table_body = new_table;
	int old_len = table_height*table_width*sizeof(WORD);
	memset(((BYTE*)table_body)+old_len, 0xFF, new_len-old_len);
	table_height = new_height;
	return(TRUE);
}

//-------------------------------------------------------------------------------
//  ======================  TGrammar  =============================
//-------------------------------------------------------------------------------

// Name of the subdirectory for storing compiled grammar tables as .cxx file and grammar source files as .cxx data.
const wchar_t *TGrammar::DEF_GRM_SRC_SUBDIR = L"Obj";

void TGrammar::Clear()
{
	//
	// Clear the data members.
	//

	parent_grammar = NULL;
	parent_xpct_inx = 0xFFFF;
	grm_props.Clear();

	symbols.SetupReadWriteMode();
	ignore_lex.SetupReadWriteMode();
	error_lex.SetupReadWriteMode();

	axioma_ident = 0;
	non_terminals.SetupReadWriteMode();
	rules.SetupReadWriteMode();

	location_objects.SetupReadWriteMode();
	nested_processing_results.SetupReadWriteMode();
	xpct_conflicts.SetupReadWriteMode();

	parsing_states.SetupReadWriteMode();
	conflicts.SetupReadWriteMode();

	lex_to_sym_conv.Clear();
	analysis_table.ReleaseTable();
}

void TGrammar::TakeContentsFrom(TGrammar &other_grammar)
{
	Clear();

	//
	// Take data from the passed instance field by field.
	//

	parent_grammar = other_grammar.parent_grammar;
	parent_xpct_inx = other_grammar.parent_xpct_inx;
	grm_props = other_grammar.grm_props;

	symbols.TakeContentsFrom(other_grammar.symbols);
	ignore_lex.TakeContentsFrom(other_grammar.ignore_lex);
	error_lex.TakeContentsFrom(other_grammar.error_lex);

	axioma_ident = other_grammar.axioma_ident;
	non_terminals.TakeContentsFrom(other_grammar.non_terminals);
	rules.TakeContentsFrom(other_grammar.rules);

	location_objects.TakeContentsFrom(other_grammar.location_objects);
	nested_processing_results.TakeContentsFrom(other_grammar.nested_processing_results);
	xpct_conflicts.TakeContentsFrom(other_grammar.xpct_conflicts);

	parsing_states.TakeContentsFrom(other_grammar.parsing_states);
	conflicts.TakeContentsFrom(other_grammar.conflicts);

	lex_to_sym_conv = other_grammar.lex_to_sym_conv;
	analysis_table.TakeContentsFrom(other_grammar.analysis_table);
}

WORD TGrammar::FindSymbol(const wchar_t *symbol_name)
{
	for (WORD i1=TerminalSymbolsBase+1; i1<NumTerminals(); ++i1)
	{
		if (symbols[i1].sym_type_name == TRUE && wcscmp(symbols[i1].name_value, symbol_name) == 0)
			return(i1);
	}

	for (WORD i2=0; i2<NumNonTerminals(); ++i2)
	{
		if (wcscmp(non_terminals[i2].symbol_name, symbol_name) == 0)
			return(i2+NonTerminalsBase);
	}

	return(0);
}

int TGrammar::FindExpectedConflict(const wchar_t *xpct_conflict_name)
{
	// Iterate registered expected conflicts.
	for (int ixpct=0; ixpct<xpct_conflicts.NumItems(); ++ixpct)
	{
		if (wcscmp(xpct_conflicts[ixpct].xpct_conflict_name, xpct_conflict_name) == 0)
			return(ixpct);
	}

	// Object is not existing.
	return(-1);
}

int TGrammar::FindConflictLocationObject(const wchar_t *loc_object_name)
{
	// Iterate existing conflict location objects.
	for (int iloc=0; iloc<location_objects.NumItems(); ++iloc)
	{
		if (wcscmp(location_objects[iloc].location_name, loc_object_name) == 0)
			return(iloc);
	}

	// Object is not existing.
	return(-1);
}

int TGrammar::FindRuleByAppId(WORD app_id)
{
	if (app_id == 0)
	{
		// Zero cannot be an app_id of any rule.
		return(-1);
	}

	// Iterate grammar rules.
	for (int irule=0; irule<rules.NumItems(); ++irule)
	{
		if (rules[irule].rule_app_id == app_id)
			return(irule);
	}

	// Object is not existing.
	return(-1);
}

int TGrammar::FindNestedResultByAppId(WORD app_id)
{
	if (app_id == 0)
		return(-1);

	// Iterate nested result objects.
	for (int inr=0; inr<nested_processing_results.NumItems(); ++inr)
	{
		if (nested_processing_results[inr].nest_res_app_id == app_id)
			return(inr);
	}

	// Object is not existing.
	return(-1);
}

wchar_t *TGrammar::GetSymbolName(WORD sym, wchar_t *buff_40_chars)
{
	if (sym >= IgnoreLexTypeBase && sym < ignore_lex.NumItems()+IgnoreLexTypeBase)
	{
		swprintf(buff_40_chars, 40, L"LexIgnore+%hd", sym-IgnoreLexTypeBase);
		return(buff_40_chars);
	}
	else if (sym >= ErrorLexTypeBase && sym < error_lex.NumItems()+ErrorLexTypeBase)
	{
		swprintf(buff_40_chars, 40, L"LexError+%hd", sym-ErrorLexTypeBase);
		return(buff_40_chars);
	}
	else if (sym == TLexToSymConverter::lt_sym_empty)
	{
		// Processing for this lexema type is not defined in the grammar.
		return(L"LexTypeUnassigned");
	}
	else
	{
		// Use symbols helper for all other cases.
		return(TGrammarSymbolsHelper::GetSymbolName(symbols, non_terminals, sym, buff_40_chars));
	}
}

WORD TGrammar::GetSymbolAppId(WORD sym)
{
	assert(sym < RuleObjectsBase);
	if (sym >= NonTerminalsBase)
	{
		return(non_terminals[sym-NonTerminalsBase].app_id);
	}

	assert(sym < IgnoreLexTypeBase);
	return(symbols[sym].app_id);
}

WORD TGrammar::GetNextParsingState(WORD stt, WORD sym)
{
	// Function returns next parsing state along the rule.
	WORD next_stt = analysis_table.GetAction(stt, SymTableFromSymGrammar(sym));

	// In typical situation the shift action will be picked up.
	if (next_stt >= actb_reduce)
	{
		// All shift/reduce conflicts have their first conflicting action shift.
		assert(next_stt >= actb_conflict && next_stt < actb_conflict+conflicts.NumItems());
		next_stt = conflicts[next_stt-actb_conflict].conflicting_actions[0];
	}

	assert(next_stt < NumParsingStates());
	return(next_stt);
}

bool TGrammar::IsEscapeAbleSeq(WORD *seq, int seq_len)
{
	while (seq_len-- > 0)
	{
		WORD sym = *seq++;
		if (sym < NonTerminalsBase)
		{
			// This is terminal symbol.
			return(FALSE);
		}

		if (non_terminals[sym-NonTerminalsBase].solid_symbol == TRUE)
		{
			// This non terminal has no empty representation.
			return(FALSE);
		}
	}

	// All symbols in the passed seq can be resolved to nothing.
	return(TRUE);
}

bool TGrammar::IsRulePartSolid(WORD irule, int ipos_beg, int ipos_end)
{
	TSymbolsArray &syms = rules[irule].symbols;
	if (ipos_end < 0)
		ipos_end = syms.NumItems();

	if (ipos_beg >= ipos_end)
		return(FALSE);

	return(IsEscapeAbleSeq(syms.ItemPtr(ipos_beg), ipos_end-ipos_beg) == FALSE);
}

int TGrammar::GetExpectedConflictInx(int iconflict)
{
	assert(iconflict >= 0 && iconflict < NumConflicts());
	TGrammarConflict &cfct = conflicts[iconflict];

	if (cfct.expected_conflict_inx >= 0)
	{
		// Current grammar conflict belongs to certain expected conflict.
		return(cfct.expected_conflict_inx);
	}

	// Passed grammar conflict is ambiguous or unexpected.
	return(-1);
}

int TGrammar::GetNumGrammarConflicts(int xpct_inx)
{
	int num_grcs = 0;
	for (int igrc=0; igrc<NumConflicts(); ++igrc)
	{
		if (GetExpectedConflictInx(igrc) == xpct_inx)
			num_grcs++;
	}

	return(num_grcs);
}

int TGrammar::GetFirstGrammarConflictInx(int xpct_conflict_inx)
{
	assert(xpct_conflict_inx >= -4 && xpct_conflict_inx < NumXpctConflicts());

	for (int iconflict=0; iconflict<NumConflicts(); ++iconflict)
	{
		int xpct_inx = conflicts[iconflict].expected_conflict_inx;
		if (xpct_conflict_inx >= 0 && xpct_conflict_inx == xpct_inx)
		{
			// Passed expected conflict is not empty.
			return(iconflict);
		}
		else if (xpct_conflict_inx == -1)
		{
			if ( xpct_inx == -1)
				return(iconflict);
		}
		else if (xpct_conflict_inx == -2)
		{
			if ( xpct_inx >= 0)
				return(iconflict);
		}
		else if (xpct_conflict_inx == -3)
		{
			if ( xpct_inx <= -2)
				return(iconflict);
		}
		else if (xpct_conflict_inx == -4)
		{
			return(iconflict);
		}
	}

	// The grammar has no conflicts of the requested category.
	return(-1);
}

int TGrammar::GetNextGrammarConflictInx(int xpct_conflict_inx, int grammar_conflict_inx)
{
	// Serch for more conflicts starting from the next grammar conflict, after the passed index.
	for (int iconflict=grammar_conflict_inx+1; iconflict<NumConflicts(); ++iconflict)
	{
		int xpct_inx = conflicts[iconflict].expected_conflict_inx;
		if (xpct_conflict_inx >= 0 && xpct_conflict_inx == xpct_inx)
		{
			// Passed expected conflict has one more member.
			return(iconflict);
		}
		else if (xpct_conflict_inx == -1)
		{
			if ( xpct_inx == -1)
				return(iconflict);
		}
		else if (xpct_conflict_inx == -2)
		{
			if ( xpct_inx >= 0)
				return(iconflict);
		}
		else if (xpct_conflict_inx == -3)
		{
			if ( xpct_inx <= -2)
				return(iconflict);
		}
		else if (xpct_conflict_inx == -4)
		{
			return(iconflict);
		}
	}

	// The grammar has no more conflicts of the requested category.
	return(-1);
}

wchar_t *TGrammar::GetParsingActionName(WORD action, wchar_t *buff_40_chars, bool wfc)
{
	if (action == actb_generic_syntax_error)
	{
		return(L"GENERIC SYNTAX ERROR");
	}
	else if (action >= actb_bogus_val)
	{
		swprintf(buff_40_chars, 40, L"%sogus action value %hd", ((wfc == TRUE) ? L"B" : L"b"), action);
	}
	else if (action >= actb_nested_result)
	{
		swprintf(buff_40_chars, 40, L"%sested result %d", ((wfc == TRUE) ? L"N" : L"n"), action-actb_nested_result);
	}
	else if (action >= actb_conflict)
	{
		swprintf(buff_40_chars, 40, L"%sarsing conflict %d", ((wfc == TRUE) ? L"P" : L"p"), action-actb_conflict);
	}
	else if (action >= actb_reduce)
	{
		swprintf(buff_40_chars, 40, L"%spply rule %d", ((wfc == TRUE) ? L"A" : L"a"), action-actb_reduce);
	}
	else
	{
		swprintf(buff_40_chars, 40, L"%shift to state %hd", ((wfc == TRUE) ? L"S" : L"s"), action);
	}

	return(buff_40_chars);
}

wchar_t *TGrammar::GetShortParsingActionName(WORD action, wchar_t *buff_40_chars)
{
	if (action == actb_generic_syntax_error)
	{
		return(L"ERR");
	}
	else if (action >= actb_bogus_val)
	{
		swprintf(buff_40_chars, 40, L"Bogus%hu", action);
	}
	else if (action >= actb_nested_result)
	{
		swprintf(buff_40_chars, 40, L"NR%d", action-actb_nested_result);
	}
	else if (action >= actb_conflict)
	{
		swprintf(buff_40_chars, 40, L"C%d", action-actb_conflict);
	}
	else if (action >= actb_reduce)
	{
		swprintf(buff_40_chars, 40, L"R%d", action-actb_reduce);
	}
	else
	{
		swprintf(buff_40_chars, 40, L"S%hd", action);
	}

	return(buff_40_chars);
}

WORD TGrammar::AddGrammarConflict(WORD state, WORD symbol)
{
	// Add core conflist info to the list of conflicts.
	TGrammarConflict info = { state, symbol, TGrammarConflictActionsArray(), -1, -1 };
	if (conflicts.AppendItem(info) == FALSE)
		return(-1);

	// Conflict creation succeeded.
	return((WORD)(conflicts.NumItems()-1));
}

bool TGrammar::AddGrammarConflictAction(int conflict_inx, WORD wanted_action)
{
	assert(conflict_inx >= 0 && conflict_inx < conflicts.NumItems());
	if (conflicts[conflict_inx].conflicting_actions.AppendItem(wanted_action) == FALSE)
		return(FALSE);

	return(TRUE);
}

WORD TGrammar::AllocAnalysisState()
{
	if (analysis_table.IsInited() == FALSE)
	{
		// The table is not inited yet.
		assert(NumRules() > 0);
		if (analysis_table.SetupTable(NumRules(), symbols.NumItems()+NumNonTerminals()) == FALSE)
			return(0xFFFF);
	}

	// Table was already existing or it was just created.
	if (analysis_table.CheckRowExistence(analysis_table.GetRowsUsed()) == FALSE)
		return(0xFFFF);

	analysis_table.IncRowsUsed();
	return(analysis_table.GetRowsUsed()-1);
}

bool TGrammar::GenerateFirstsAndFollows(TAnalysisTable &res_table)
{
	//
	//  Create table with the following size:
	//
	//     num_rows		- number of non terminals in the grammar.
	//     num_columns	- number of terminal symbols.
	//
	//  It is ok if table is not empty at the beginning.
	//  Analysis tables allows resetup.
	//

	res_table.ReleaseTable();
	int num_terminals = symbols.NumItems();
	if (res_table.SetupTable(NumNonTerminals(), num_terminals) == FALSE)
	{
		// Upper layer should emit the error.
		return(FALSE);
	}

	//
	//  Meaning of the table slot:
	//
	//     Hi-Byte  --	byte is not 0 if non terminal (row) can start from the current terminal (column).
	//				First column corresponds to the empty symbol.
	//
	//     Lo-Myte  --	byte is not 0 if terminal (column) can follow the current non terminal (row).
	//				First column corresponds to the EOF symbol.
	//
	WORD *table = res_table.GetBodyPtr();
	memset(table, 0, NumNonTerminals()*res_table.GetWidth()*sizeof(WORD));

	// Fuction FIRST. Each row of the matrix is considered as set of terminal symbols. Matrix as a whole is treated
	// as set of sets. Rotate this loop until it will stop discovering new members of the sets.
	for(;;)
	{
		int num_added = 0;
		for (TRulesIterator iter1(*this); iter1; ++iter1)
		{
			TGrammarRule &rule = iter1.CurrRule();
			int rule_len = rule.Length();
			WORD *rule_non_term_set = res_table.GetNonTermRowPtr(rule.non_term);

			if (rule_len == 0)
			{
				// Length of the right hand side of the rule is zero. Add empty symbol to the "rule.non_term" set.
				if (AddToFirstsSet(rule_non_term_set, TerminalSymbolsBase) == TRUE)
					num_added++;
			}
			else
			{
				// Process symbols on the right side of the rule.
				bool right_side_can_derive_to_empty = TRUE;
				for (int isym=0; isym<rule_len; ++isym)
				{
					WORD sym = rule.symbols[isym];
					if (sym < NonTerminalsBase)
					{
						// Curr symbol is terminal. Add it to the "rule.non_term" set.
						if (AddToFirstsSet(rule_non_term_set, sym) == TRUE)
							num_added++;

						right_side_can_derive_to_empty = FALSE;
						break;
					}
					else
					{
						// Curr symbol is non terminal. Add all members of its FIRST(sym) set that are discovered so far
						// except for empty symbol to the "rule.non_term" set.
						WORD *curr_sym_set = res_table.GetNonTermRowPtr(sym);
						for (int offs=TerminalSymbolsBase+1; offs<num_terminals; ++offs)
						{
							if ((curr_sym_set[offs] & FUN_FIRST_MASK) != 0)
							{
								// Symbol with the value offs is member of the curr_sym set.
								if (AddToFirstsSet(rule_non_term_set, offs) == TRUE)
									num_added++;
							}
						}

						if ((curr_sym_set[TerminalSymbolsBase] & FUN_FIRST_MASK) == 0)
						{
							// Current non terminal cannot be derived to nothing. This means that non terminal
							// of the rule cannot be derived to nothing either.
							right_side_can_derive_to_empty = FALSE;
							break;
						}
					}
				}

				if (right_side_can_derive_to_empty == TRUE)
				{
					// Whole right hand side of the rule can be derived to nothing. Add empty symbol to "rule.non_term" set.
					if (AddToFirstsSet(rule_non_term_set, TerminalSymbolsBase) == TRUE)
						num_added++;
				}
			}
		}

		// Check results of the loop pass.
		if (num_added == 0)
			break;
	}

	// Fuction FOLLOWS. Add Eof symbol as possible follower to the axioma symbol.
	WORD *axioma_set = res_table.GetNonTermRowPtr(axioma_ident);
	axioma_set[TerminalSymbolsBase] |= FUN_FOLLOWS_FLAG;

	// Scan rules of the grammar until this loop will stop discovering new followers.
	for(;;)
	{
		int num_added = 0;
		for (TRulesIterator iter2(*this); iter2; ++iter2)
		{
			TGrammarRule &rule = iter2.CurrRule();
			int rule_len = rule.Length();
			if (rule_len == 0)
			{
				// Empty rules cannot generate FOLLOWS symbols.
				continue;
			}

			// Look for non terminals on the right side of the curr rule.
			for (int isym=0; isym<rule_len; ++isym)
			{
				if (rule.symbols[isym] < NonTerminalsBase)
				{
					// Curr symbol is terminal. Ignore it.
					continue;
				}

				//
				//  Right side of the rule looks like:
				//
				//     alpha B beta
				//
				//  where B is current non terminal, and beta is some string, maybe empty.
				//
				//  Dynamically generate and process FIRST(beta).
				//
				bool  beta_can_derive_to_empty = TRUE;
				WORD *b_set = res_table.GetNonTermRowPtr(rule.symbols[isym]);
				for (int jj=isym+1; jj<rule_len; ++jj)
				{
					WORD sym = rule.symbols[jj];
					if (sym < NonTerminalsBase)
					{
						// Curr symbol is terminal. Add it to FOLLOWS(B).
						if (AddToFollowsSet(b_set, sym) == TRUE)
							num_added++;

						beta_can_derive_to_empty = FALSE;
						break;
					}
					else
					{
						// Curr symbol is non terminal. Add its first symbols except for the empty symbol if any to FOLLOWS(B).
						WORD *curr_sym_set = res_table.GetNonTermRowPtr(sym);
						for (int offs=TerminalSymbolsBase+1; offs<num_terminals; ++offs)
						{
							if ((curr_sym_set[offs] & FUN_FIRST_MASK) != 0)
							{
								// Symbol with the value offs is member of the curr_sym set.
								if (AddToFollowsSet(b_set, offs) == TRUE)
									num_added++;
							}
						}

						if ((curr_sym_set[TerminalSymbolsBase] & FUN_FIRST_MASK) == 0)
						{
							// Current non terminal cannot be derived to nothing. This means that remaining part
							// of the string cannot generate more elements for the FIRST(beta).
							beta_can_derive_to_empty = FALSE;
							break;
						}
					}
				}

				if (beta_can_derive_to_empty == TRUE)
				{
					// Beta part is empty or it can derive to nothing. Add followers of the rule non terminal to FOLLOWS(B).
					WORD *rule_non_term_set = res_table.GetNonTermRowPtr(rule.non_term);
					for (int offs=TerminalSymbolsBase; offs<num_terminals; ++offs)
					{
						if ((rule_non_term_set[offs] & FUN_FOLLOWS_MASK) != 0)
						{
							// Symbol with the value offs is member of the curr_sym set.
							if (AddToFollowsSet(b_set, offs) == TRUE)
								num_added++;
						}
					}
				}
			}
		}

		// Check results of the loop pass.
		if (num_added == 0)
			break;
	}

	// Success.
	return(TRUE);
}

bool TGrammar::GenerateNonTerminalFirsts(TAnalysisTable &res_table, TAnalysisTable &prepared_firsts_and_follows)
{
	// Create square matrix.
	res_table.ReleaseTable();
	if (res_table.SetupTable(NumNonTerminals(), NumNonTerminals()) == FALSE)
		return(FALSE);

	// Init the whole table with zeroes.
	WORD *table = res_table.GetBodyPtr();
	memset(table, 0, NumNonTerminals()*res_table.GetWidth()*sizeof(WORD));

	for(;;)
	{
		int num_added = 0;
		for (TRulesIterator iter1(*this); iter1; ++iter1)
		{
			TGrammarRule &rule = iter1.CurrRule();
			WORD *rule_non_term_set = res_table.GetNonTermRowPtr(rule.non_term);

			// Process symbols on the right hand side of the rule.
			for (int isym=0; isym<rule.Length(); ++isym)
			{
				WORD sym = rule.symbols[isym];
				if (sym >= NonTerminalsBase)
				{
					// Curr symbol is non terminal. Add it to the FIRSTs of the rule non term.
					if (AddToFirstsSet(rule_non_term_set, sym-NonTerminalsBase) == TRUE)
						num_added++;

					// Add all members of the FIRSTs for this non term that are discovered so far.
					WORD *curr_sym_set = res_table.GetNonTermRowPtr(sym);
					for (int offs=0; offs<NumNonTerminals(); ++offs)
					{
						if ((curr_sym_set[offs] & FUN_FIRST_MASK) != 0)
						{
							// Symbol with the value offs is a member of the curr_sym set.
							if (AddToFirstsSet(rule_non_term_set, offs) == TRUE)
								num_added++;
						}
					}

					if ((prepared_firsts_and_follows.GetNonTermRowPtr(sym)[TerminalSymbolsBase] & FUN_FIRST_MASK) != 0)
					{
						// Current non terminal can be created out of nothing. Proceed to the next symbol of the rule if any.
						continue;
					}
				}

				// Current rule symbol is terminal symbol or it is non transparent non terminal.
				break;
			}
		}

		// Check results of the loop pass.
		if (num_added == 0)
			break;
	}

	// Success.
	return(TRUE);
}

bool TGrammar::GenerateRuleStartStatesIterIndex(TRulePosIterIndexLevel2 &index, void (*ProgressHandler)(void *context, int items_done, int total_items), void *context)
{
	// Release the old state of the index if any.
	index.Clear();

	// Rule starting state index.
	for (int irule=0; irule<NumRules(); ++irule)
	{
		TRulePosIterIndexLevel1 starting_stts_for_rule;
		for (int istt1=0; istt1<NumParsingStates(); ++istt1)
		{
			int first_pos_inx = parsing_states[istt1].FindFirstPosForRule(irule, TRUE);
			if (first_pos_inx < 0)
				continue;

			// Starting position for the current state is available.
			TRulePosIterIndexRecord inx_rec = { istt1, first_pos_inx, 0 };
			int pos_inx = first_pos_inx+1;
			int parsing_state_length = parsing_states[istt1].NumItems();
			TRulePosition *pos = parsing_states[istt1].ItemPtr(pos_inx);
			while (pos_inx < parsing_state_length)
			{
				if (pos->isym != 0 || pos->irule != irule)
					break;
				pos_inx++;
				pos++;
			}

			inx_rec.num_positions = pos_inx-first_pos_inx;
			if (starting_stts_for_rule.AppendItem(inx_rec) == FALSE)
			{
				index.Clear();
				return(FALSE);
			}
		}

		// Add the final empty record that is needed for simplification and speeding up the rule start states iterator.
		TRulePosIterIndexRecord final_rec = { 0, 0, 0 };
		if (starting_stts_for_rule.AppendItem(final_rec) == FALSE)
		{
			index.Clear();
			return(FALSE);
		}

		// Add array of possible starting states to the global list even if it is empty.
		if (index.AppendItem(starting_stts_for_rule) == FALSE)
		{
			index.Clear();
			return(FALSE);
		}

		// The final record in the Level 1 array should not be included into the numer of array items.
		index[irule].ReleaseLastItem();

		// Call the callback to give chance for updating UI.
		if ((irule & 0xF) == 0 && ProgressHandler != NULL)
			(*ProgressHandler)(context, irule, NumRules());
	}

	// Success.
	return(TRUE);
}

bool TGrammar::GenerateNonTermDefnRulesIterIndex(TRulePosIterIndexLevel2 &index, void (*ProgressHandler)(void *context, int items_done, int total_items), void *context)
{
	// Non terminal defn rules index.
	for (int sym=NonTerminalsBase; sym<NonTerminalsBase+NumNonTerminals(); ++sym)
	{
		TRulePosIterIndexLevel1 rules_for_symbol;
		for (int istt2=0; istt2<NumParsingStates(); ++istt2)
		{
			TRulePosIterIndexRecord inx_rec = { istt2, 0, 0 };
			TParsingStatePositionsIterator iter;
			int pos_inx = 0;
			for (iter.Setup(parsing_states[istt2]); iter; ++iter, ++pos_inx)
			{
				if (rules[iter.CurrPos().irule].non_term != sym || iter.CurrPos().isym != 0)
					continue;

				// Starting position for the curr non terminal.
				if (inx_rec.num_positions <= 0)
				{
					// First position.
					inx_rec.pos_beg = pos_inx;
					inx_rec.num_positions = 1;
				}
				else
				{
					// More positions.
					inx_rec.num_positions = pos_inx-inx_rec.pos_beg+1;
				}
			}

			// Add record regardless if positions are available or not. This index is used as a matrix.
			// So, all records should be present.
			if (rules_for_symbol.AppendItem(inx_rec) == FALSE)
			{
				return(FALSE);
			}
		}

		// Add info for the current non_term symbol to global index.
		if (index.AppendItem(rules_for_symbol) == FALSE)
		{
			return(FALSE);
		}

		// Call the callback to give chance for updating UI.
		if (ProgressHandler != NULL && ((sym-NonTerminalsBase) & 0xF) == 0)
			(*ProgressHandler)(context, sym-NonTerminalsBase, NumNonTerminals());
	}

	// Success.
	return(TRUE);
}

int TGrammar::GetGrammarNesting()
{
	int cnt_nest = 0;
	TGrammar *cgrm = parent_grammar;

	// This loop has certain risk of being infinite. Nevertheless.
	while (cgrm != NULL)
	{
		cnt_nest++;
		cgrm = cgrm->parent_grammar;
	}

	return(cnt_nest);
}

bool TGrammar::GenerateGrammarName(TNameBuffer &buffer, const wchar_t *delim_seq)
{
	buffer.ClearBuffer();
	if (parent_grammar == NULL)
	{
		// This grammar has fixed name.
		buffer.Append(L"Root");
	}
	else if (parent_xpct_inx == 0xFFFF)
	{
		// The hierarchy seems to be broken.
		buffer.Append(L"Hierarchy_Is_Broken");
	}
	else
	{
		// Process path of nested grammars up to the root.
		int ixpct = parent_xpct_inx;
		TGrammar *cgrm = parent_grammar;
		while (cgrm != NULL)
		{
			if (cgrm != parent_grammar)
			{
				// Separate names with delimiter.
				buffer.Insert(0, delim_seq);
			}

			// Add one more xpct conflict name.
			buffer.Insert(0, cgrm->xpct_conflicts[ixpct].xpct_conflict_name);

			// Shift the loop variables.
			ixpct = cgrm->parent_xpct_inx;
			cgrm = cgrm->parent_grammar;
		}
	}

	// Give out the success state of the buffer.
	return(buffer.GetXpndError() == FALSE);
}

int TGrammar::GetNumGrammars(bool scan_hier_default_is_TRUE) const
{
	TGrammar *grm = (TGrammar*)this;
	if (scan_hier_default_is_TRUE == TRUE)
	{
		// Navigate to the root of the tree. Take the risk of the infinite loop.
		while (grm->parent_grammar != NULL)
			grm = grm->parent_grammar;
	}

	int cnt = 1;		// The value one stands for "this" grammar.
	for (int ixpct=0; ixpct<grm->xpct_conflicts.NumItems(); ++ixpct)
	{
		if (grm->xpct_conflicts[ixpct].nested_grammar != NULL)
			cnt += grm->xpct_conflicts[ixpct].nested_grammar->GetNumGrammars(FALSE);
	}
	return(cnt);
}

int TGrammar::GetGrammarIndex() const
{
	// Check for the root of the tree case.
	if (parent_grammar == NULL)
		return(0);

	// Navigate to the root of the tree. Take the risk of the infinite loop.
	TGrammar *grm = (TGrammar*)this;
	while (grm->parent_grammar != NULL)
		grm = grm->parent_grammar;

	// Travel the subtree.
	int grammars_inx_base = 1, igrm = -1;
	TGrammar *this_grammar = (TGrammar*)this;
	grm->ProcessGrammarsHier(grammars_inx_base, igrm, &this_grammar);
	return(igrm);
}

TGrammar *TGrammar::GetGrammarByIndex(int igrm) const
{
	assert(igrm >= 0);

	// Navigate to the root of the tree. Take the risk of the infinite loop.
	TGrammar *grm = (TGrammar*)this;
	while (grm->parent_grammar != NULL)
		grm = grm->parent_grammar;

	// Check for the root of the tree.
	if (igrm == 0)
		return(grm);

	// Travel the subtree.
	int grammars_inx_base = 1;
	TGrammar *pgrm = NULL;
	grm->ProcessGrammarsHier(grammars_inx_base, igrm, &pgrm);
	return(pgrm);
}

int TGrammar::GetNumGrammarConflictsHier() const
{
	int cnt = NumConflicts();
	for (int ixpct=0; ixpct<xpct_conflicts.NumItems(); ++ixpct)
	{
		if (xpct_conflicts[ixpct].nested_grammar != NULL)
			cnt += xpct_conflicts[ixpct].nested_grammar->GetNumGrammarConflictsHier();
	}

	return(cnt);
}

int TGrammar::GetNumConflictingActionsHier() const
{
	int cnt = 0;
	for (int iconflict=0; iconflict<conflicts.NumItems(); ++iconflict)
	{
		cnt += conflicts[iconflict].NumActions();
	}

	for (int ixpct=0; ixpct<xpct_conflicts.NumItems(); ++ixpct)
	{
		if (xpct_conflicts[ixpct].nested_grammar != NULL)
			cnt += xpct_conflicts[ixpct].nested_grammar->GetNumConflictingActionsHier();
	}

	return(cnt);
}

int TGrammar::GetNumUnexpectedConflictsHier() const
{
	int cnt = 0;
	for (int iconflict=0; iconflict<NumConflicts(); ++iconflict)
	{
		if (conflicts[iconflict].conflict_location_inx == -1)
			cnt++;
	}

	for (int ixpct=0; ixpct<xpct_conflicts.NumItems(); ++ixpct)
	{
		if (xpct_conflicts[ixpct].nested_grammar != NULL)
			cnt += xpct_conflicts[ixpct].nested_grammar->GetNumUnexpectedConflictsHier();
	}

	return(cnt);
}

void TGrammar::SetProcessingResultHier(TGrammarProcessingResult res)
{
	// Current design expects that all granmmars in the hier have the same status.
	grm_props.processing_result = res;
	for (int ixpct=0; ixpct<xpct_conflicts.NumItems(); ++ixpct)
	{
		if (xpct_conflicts[ixpct].nested_grammar != NULL)
			xpct_conflicts[ixpct].nested_grammar->SetProcessingResultHier(res);
	}
}

void TGrammar::EmitTables(TGenericConsole *console, const wchar_t *console_msg_prefix, TDestinationFile &rprt, const wchar_t *names_prefix, bool emit_ext_comments)
{
	if (names_prefix == NULL)
		names_prefix = grm_props.preferred_emitting_prefix;

	// Header of the file.
	rprt.WriteLine(L"#include    <stdio.h>");
	rprt.WriteLine(L"#include    <windows.h>");
	rprt.WriteLine(L"#include    <assert.h>");
	rprt.WriteLine(L"");
	rprt.WriteLine(L"#include    \"TextScan/GrammarStructure.h\"");
	rprt.WriteLine(L"");

	// Call the major worker method.
	EmitGrammarTablesHier(rprt, names_prefix, NULL, emit_ext_comments);

	// Close report and check results.
	rprt.Close();
	if (console != NULL)
	{
		// Emit success/failure traces to the console.
		wchar_t prefix[40],  buffer[1024];
		console->PrepareConsolePrefix(prefix, console_msg_prefix);
		if (rprt.GetErrorFlag() == FALSE)
		{
			swprintf(buffer, 1024, L"%sSuccess writing to: \"%s\".", prefix, rprt.FileName());
			console->HandleTrace(buffer);

			swprintf(buffer, 1024, L"%sStruct name: %s_grammar.", prefix, names_prefix);
			console->HandleTrace(buffer);

			wchar_t buffer_len[80];
			swprintf(buffer, 1024, L"%sFile length: %s.", prefix, FormatInt64(rprt.GetCurrLen(), buffer_len, 80, fnms_dec_signed, L'_'));
			console->HandleTrace(buffer);
		}
		else
		{
			swprintf(buffer, 1024, L"%sError writing grammar tables to: \"%s\".", prefix, rprt.FileName());
			console->HandleTrace(buffer);
		}
	}
}

bool TGrammar::EmitTablesEx(TGenericConsole *console, const wchar_t *console_msg_prefix, const wchar_t *subdir_name, const wchar_t *file_name_suffix_with_ext,
								const wchar_t *structs_and_arrays_names_prefix, bool emit_ext_comments)
{
	wchar_t prefix[40], err_msg_buffer[360];
	TGenericConsole::PrepareConsolePrefix(prefix, console_msg_prefix);
	int len_prefix = wcslen(prefix);

	TFileNameBuffer dest_directory_name;
	if (TPathHelper::PrepareDestDirectory(err_msg_buffer+len_prefix, 360-len_prefix, dest_directory_name, subdir_name, grm_props.grms_file_name, L"major grammar source file") == FALSE)
	{
		wcsncpy(err_msg_buffer, prefix, len_prefix);
		console->HandleTrace(err_msg_buffer);
		return(FALSE);
	}

	TDestinationFile rprt;
	if (TPathHelper::PrepareDestFile(err_msg_buffer+len_prefix, 360-len_prefix, rprt, dest_directory_name, NULL, grm_props.grms_file_name, file_name_suffix_with_ext, L"generated grammar tables file") == FALSE)
	{
		// Error is already reported.
		return(FALSE);
	}

	// Emit the data and close the file. This call will also write the success/failure messages.
	EmitTables(console, console_msg_prefix, rprt, structs_and_arrays_names_prefix, emit_ext_comments);
	return(rprt.GetErrorFlag() == FALSE);
}

void TGrammar::EmitGrammarTablesHier(TDestinationFile &rprt, const wchar_t *names_prefix, const wchar_t *parent_prefix, bool emit_ext_comments)
{
	assert(names_prefix != NULL);

	// Look for the nested grammars.
	bool nested_grammars_present = FALSE;
	for (int ich1=0; ich1<xpct_conflicts.NumItems(); ++ich1)
	{
		if (xpct_conflicts[ich1].nested_grammar != NULL)
		{
			nested_grammars_present = TRUE;
			break;
		}
	}

	if (nested_grammars_present == TRUE)
	{
		// Emit forward declaration for the current instance.
		rprt.WriteFmtLine(L"// Forward declaration.");
		rprt.WriteFmtLine(L"extern TGrammar %s_grammar;   // (igrammar=%d).", names_prefix, GetGrammarIndex());
		rprt.WriteFmtLine(L"");
	}

	// ----------------------------------
	//
	//    Process the nested grammars if any.
	//
	// ----------------------------------

	if (xpct_conflicts.NumItems() > 0)
	{
		TNameBuffer chld_prefix;
		for (int ich2=0; ich2<xpct_conflicts.NumItems(); ++ich2)
		{
			// Check if nested grammar is present in the currext xpct conflict or not.
			if (xpct_conflicts[ich2].nested_grammar != NULL)
			{
				// Allocate buffer for the name prefix.
				wchar_t *xpcf_name = xpct_conflicts[ich2].xpct_conflict_name;
				int req_len = (int)wcslen(names_prefix)+1+(int)wcslen(xpcf_name)+1;
				if (chld_prefix.ReserveSpace(req_len) == FALSE)
				{
					// This should be rare situation. Raise the error flag in the destination file. This will cause
					// error message at the end of writing.
					rprt.SetWritingError();
				}

				// Prepare name prefix of the nested grammar.
				swprintf(chld_prefix.DataPtr(), req_len, L"%s_%s", names_prefix, xpcf_name);

				// Make the recursive call.
				xpct_conflicts[ich2].nested_grammar->EmitGrammarTablesHier(rprt, chld_prefix.DataPtr(), names_prefix, emit_ext_comments);
			}
		}
	}

	// Hide local string allocations into the block.
	{
		// Prepare emitting the beginning header.
		wchar_t buff_igrm[80];
		swprintf(buff_igrm, 80, L" (igrammar=%d) ", GetGrammarIndex());
		int len_fixed = 2+wcslen(names_prefix)+8+2+8+wcslen(buff_igrm);
		int num_equals = 76;
		int num_minuses = __max((num_equals-4-len_fixed)/2, 16);
		num_equals = 2+num_minuses+len_fixed+num_minuses+2;

		// Emit the beginning header.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmt(L"// ");
		for (int ieq1=0; ieq1<num_equals; ++ieq1) rprt.Write(L"=");
		rprt.WriteLine();
		rprt.WriteFmt(L"//   ");
		for (int imn1=0; imn1<num_minuses-2; ++imn1) rprt.Write(L"-");
		rprt.WriteFmt(L"  %s_grammar  --------%s", names_prefix, buff_igrm);
		for (int imn2=0; imn2<num_minuses+2; ++imn2) rprt.Write(L"-");
		rprt.WriteLine();
		rprt.WriteFmt(L"// ");
		for (int ieq2=0; ieq2<num_equals; ++ieq2) rprt.Write(L"=");
		rprt.WriteLine();
		rprt.WriteFmtLine(L"//");
		rprt.WriteLine();
	}

	if (symbols.NumItems() != 0)
	{
		// Emit the terminal symbol definitions.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Terminal symbols of the grammar (igrammar=%d).", GetGrammarIndex());
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"static TTerminalSymbol %s_terminal_symbols[%d] = ", names_prefix, symbols.NumItems());
		rprt.WriteFmtLine(L"{");

		for (int i1=0; i1<symbols.NumItems(); ++i1)
		{
			if (symbols[i1].sym_type_name == FALSE)
			{
				// Char const.
				rprt.WriteFmt(L"  { FALSE, %14I64d, L\"\", %5hd, ", symbols[i1].char_value, symbols[i1].app_id);
			}
			else
			{
				// String.
				wchar_t symbol_name_buff[80];
				swprintf(symbol_name_buff, 80, L"L\"%s\",", symbols[i1].name_value);
				rprt.WriteFmt(L"  { TRUE, 0, %-18s %5hd, ", symbol_name_buff, symbols[i1].app_id);
			}

			wchar_t lext_enum_name[80], subt_enum_name[80], buff_40ch[40];
			swprintf(lext_enum_name, 80, L"%s,", TLexema::GetLexTypeEnumName(symbols[i1].lex_type));
			swprintf(subt_enum_name, 80, L"%s,", MakeLexSubtypeEnumName(symbols[i1].lex_type, symbols[i1].check_subt, symbols[i1].lex_subt, buff_40ch));

			rprt.WriteFmtLine(L"%-20s %s, %-16s %3lu, %2d },  // %d",
							lext_enum_name, MakeBool(symbols[i1].check_subt, TRUE), subt_enum_name,
							symbols[i1].event_id, symbols[i1].num_rule_uses, i1);
		}

		if (emit_ext_comments == TRUE)
			rprt.WriteFmtLine(L"};  // %s_terminal_symbols", names_prefix);
		else rprt.WriteFmtLine(L"};");

		rprt.WriteLine();
	}

	if (ignore_lex.NumItems() != 0)
	{
		// Emit the ignore lex records.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Ignore these types of lexemas.");
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"static TIgnoreLexRecord %s_ignore_lex_records[%d] = ", names_prefix, ignore_lex.NumItems());
		rprt.WriteFmtLine(L"{");

		for (int i2=0; i2<ignore_lex.NumItems(); ++i2)
		{
			wchar_t enum_name_buff[40], buff_40ch[40];
			swprintf(enum_name_buff, 40, L"%s,", TLexema::GetLexTypeEnumName(ignore_lex[i2].lex_type));
			rprt.WriteFmtLine(L"  { %-14s %s, %s },",
							enum_name_buff, MakeBool(ignore_lex[i2].check_subt, TRUE),
							MakeLexSubtypeEnumName(ignore_lex[i2].lex_type, ignore_lex[i2].check_subt, ignore_lex[i2].lex_subt, buff_40ch));
		}

		if (emit_ext_comments == TRUE)
			rprt.WriteFmtLine(L"};  // %s_ignore_lex_records", names_prefix);
		else rprt.WriteFmtLine(L"};");

		rprt.WriteLine();
	}

	if (error_lex.NumItems() != 0)
	{
		// Emit the error lex records.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Emit errors on these types of lexemas.");
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"static TErrorLexRecord %s_error_lex_records[%d] = ", names_prefix, error_lex.NumItems());
		rprt.WriteFmtLine(L"{");

		for (int i3=0; i3<error_lex.NumItems(); ++i3)
		{
			wchar_t buff_40ch[40];
			rprt.WriteFmtLine(L"  { %s, %s, %s, L\"%s\" },",
							TLexema::GetLexTypeEnumName(error_lex[i3].lex_type), MakeBool(error_lex[i3].check_subt),
							MakeLexSubtypeEnumName(error_lex[i3].lex_type, error_lex[i3].check_subt, error_lex[i3].lex_subt, buff_40ch),
							error_lex[i3].error_message);
		}

		if (emit_ext_comments == TRUE)
			rprt.WriteFmtLine(L"};  // %s_error_lex_records", names_prefix);
		else rprt.WriteFmtLine(L"};");

		rprt.WriteLine();
	}

	if (non_terminals.NumItems() != 0)
	{
		int longest_non_term_name = 4;
		for (int isym=0; isym<non_terminals.NumItems(); ++isym)
		{
			int len = (int)wcslen(non_terminals[isym].symbol_name);
			if (len > longest_non_term_name)
				longest_non_term_name = len;
		}

		// Emit the non terminal symbol definitions.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Non terminals of the grammar (igrammar=%d).", GetGrammarIndex());
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"//	symbol_name");
		rprt.WriteFmtLine(L"//	app_id");
		rprt.WriteFmtLine(L"//	rules_sect		-	rules section with non terminal rules.");
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"//	public_symbol		-	TRUE/FALSE.");
		rprt.WriteFmtLine(L"//	restr_type		-	ntrs_none/ntrs_list/ntrs_seq.");
		rprt.WriteFmtLine(L"//	solid_symbol		-	TRUE/FALSE.");
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"//	event_id		-	id of event with symbol intro area.");
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"//	num_lhs_rule_uses	-	rule left side uses.");
		rprt.WriteFmtLine(L"//	num_rhs_rule_uses	-	rule right side uses.");
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"static TNonTerminalSymbol %s_non_terminals[%d] = ", names_prefix, NumNonTerminals());
		rprt.WriteFmtLine(L"{");

		for (int i4=0; i4<NumNonTerminals(); ++i4)
		{
			wchar_t symbol_name_buff[80];
			swprintf(symbol_name_buff, 80, L"L\"%s\",", non_terminals[i4].symbol_name);
			rprt.WriteFmtLine(L"  { %-*s %5hd, %3hd, %s, %s, %s, %5lu, %2d, %2d, },	// %d (%d)",
							longest_non_term_name+4, symbol_name_buff,
							non_terminals[i4].app_id,
							non_terminals[i4].rules_sect,
							MakeBool(non_terminals[i4].public_symbol, TRUE),
							MakeNonTermRestrTypeEnumName(non_terminals[i4].restr_type),
							MakeBool(non_terminals[i4].solid_symbol, TRUE),
							non_terminals[i4].event_id,
							non_terminals[i4].num_lhs_rule_uses,
							non_terminals[i4].num_rhs_rule_uses,
							NonTerminalsBase+i4, symbols.NumItems()+i4);
		}

		if (emit_ext_comments == TRUE)
			rprt.WriteFmtLine(L"};  // %s_non_terminals", names_prefix);
		else rprt.WriteFmtLine(L"};");

		rprt.WriteLine();
	}

	if (rules.NumItems() != 0)
	{
		// Emit symbols from the right hand sides of the rules.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Right hand side symbols of the grammar rules. These arrays are emitted");
		rprt.WriteFmtLine(L"// only for those rules, that have non empty right hand sides.");
		rprt.WriteFmtLine(L"//");

		int max_rhs_length = 0;
		int max_app_id_name_len = 5;
		for (TRulesIterator iter(*this); iter; ++iter)
		{
			if (iter.CurrRule().Length() > max_rhs_length)
				max_rhs_length = iter.CurrRule().Length();

			wchar_t *app_id_name = iter.CurrRule().rule_app_id_name;
			if (app_id_name != NULL && (int)wcslen(app_id_name)+4 > max_app_id_name_len)
				max_app_id_name_len = (int)wcslen(app_id_name)+4;
		}

		int len_width = (max_rhs_length >= 100) ? 3 : ((max_rhs_length >= 10) ? 2 : 1);
		for (int i5a=0; i5a<rules.NumItems(); ++i5a)
		{
			if (rules[i5a].Length() == 0)
				continue;

			rprt.WriteFmt(L"static WORD %s_rule_%03d_rhs_symbols[%*d] = { ", names_prefix, i5a, len_width, rules[i5a].Length());
			for (int isym=0; isym<rules[i5a].Length(); ++isym)
			{
				rprt.WriteFmt(L"%hd%s ", rules[i5a].symbols[isym], (isym != (rules[i5a].Length()-1)) ? L"," : L"");
			}

			rprt.WriteLine(L"};");
		}

		rprt.WriteLine();

		// Emit the list of rules.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Rules of the grammar (igrammar=%d).", GetGrammarIndex());
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"static TGrammarRule %s_rules[%d] = ", names_prefix, NumRules());
		rprt.WriteFmtLine(L"{");

		for (int i5b=0; i5b<NumRules(); ++i5b)
		{
			wchar_t rhs_name_buff[80];
			if (rules[i5b].Length() != 0)
				swprintf(rhs_name_buff, 80, L"%s_rule_%03d_rhs_symbols,", names_prefix, i5b);
			else wcscpy(rhs_name_buff, L"NULL,");

			wchar_t app_id_name_buff[TGrammar::MAX_NAME_LENGTH+1];
			rprt.WriteFmtLine(L"  { %4hd, TSymbolsArray(%-*s %2d), %5hd, %-*s %4lu, %4lu, },  // %d",
							rules[i5b].non_term, wcslen(names_prefix)+22, rhs_name_buff, rules[i5b].Length(),
							rules[i5b].rule_app_id, max_app_id_name_len, MakeOptionalName(app_id_name_buff, rules[i5b].rule_app_id_name),
							rules[i5b].non_term_event_id, rules[i5b].rule_body_event_id, i5b);
		}

		if (emit_ext_comments == TRUE)
			rprt.WriteFmtLine(L"};  // %s_rules", names_prefix);
		else rprt.WriteFmtLine(L"};");

		rprt.WriteLine();
	}

	if (location_objects.NumItems() != 0)
	{
		// Emit the rule positions from expected conflict locations.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Application defined expected conflict location rule positions.");
		rprt.WriteFmtLine(L"//");

		for (int i6a=0; i6a<location_objects.NumItems(); ++i6a)
		{
			TRulePositionsArray &positions_array = location_objects[i6a].positions;
			rprt.WriteFmtLine(L"static TRulePosition %s_xpct_conflict_rule_positions_%04d[%d] = ", names_prefix, i6a, positions_array.NumItems());
			rprt.WriteFmtLine(L"{");

			int cnt_in_line = 0;
			for (int ipos=0; ipos<positions_array.NumItems(); ++ipos)
			{
				if (cnt_in_line == 0)
					rprt.Write(L"  ");

				rprt.WriteFmt(L"{ %3hu, %2hu, %5hu,  %s },", positions_array[ipos].irule, positions_array[ipos].isym, positions_array[ipos].action_sym, MakeBool((positions_array[ipos].mark == TRUE) ? TRUE : FALSE, TRUE));

				if (++cnt_in_line >= 4 || ipos == positions_array.NumItems()-1)
				{
					rprt.WriteLine(L"");
					cnt_in_line = 0;
				}
				else
				{
					rprt.Write(L" ");
				}
			}

			rprt.WriteLine(L"};");
		}

		rprt.WriteLine();

		// Emit lists of conforming parsing states from expected conflict locations.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Expected conflict location conforming parsing states.");
		rprt.WriteFmtLine(L"//");

		for (int i6b=0; i6b<location_objects.NumItems(); ++i6b)
		{
			TSymbolsArray &states_array = location_objects[i6b].conforming_states;
			if (states_array.NumItems() > 0)
			{
				rprt.WriteFmtLine(L"static WORD %s_xpct_conflict_conforming_parsing_states_%04d[%d] = ", names_prefix, i6b, states_array.NumItems());
				EmitSymbolsArray(rprt, states_array);
			}
		}

		rprt.WriteLine();

		// Find out length of the longest xpct_conflict_location name.
		int longest_xpct_loc_len = 4;
		for (int i6c=0; i6c<location_objects.NumItems(); ++i6c)
		{
			int len = (int)wcslen(location_objects[i6c].location_name);
			if (len > longest_xpct_loc_len)
				longest_xpct_loc_len = len;
		}

		// Emit info about the expected conflict locations.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Expected conflict locations.");
		rprt.WriteFmtLine(L"//");

		rprt.WriteFmtLine(L"static TExpectedConflictLocation %s_xpct_conflict_locations[%d] = ", names_prefix, location_objects.NumItems());
		rprt.WriteFmtLine(L"{");

		for (int i6d=0; i6d<location_objects.NumItems(); ++i6d)
		{
			TExpectedConflictLocation &loc = location_objects[i6d];
			wchar_t loc_name_buff[80];
			swprintf(loc_name_buff, 80, L"L\"%s\"", loc.location_name);

			rprt.WriteFmt(L"  { %.*s, %4hu, TRulePositionsArray(%s_xpct_conflict_rule_positions_%04d, %3d), %5hu, ",
						longest_xpct_loc_len+4, loc_name_buff, loc.action_symbol, names_prefix, i6d, loc.positions.NumItems(), loc.num_xpct_conflicts);

			if (loc.conforming_states.NumItems() > 0)
				rprt.WriteFmtLine(L"TSymbolsArray(%s_xpct_conflict_conforming_parsing_states_%04d, %3d) },", names_prefix, i6d, loc.conforming_states.NumItems());
			else rprt.WriteFmtLine(L"TSymbolsArray(NULL, 0) },");
		}

		if (emit_ext_comments == TRUE)
			rprt.WriteFmtLine(L"};  // %s_xpct_conflict_locations", names_prefix);
		else rprt.WriteFmtLine(L"};");

		rprt.WriteLine();
	}

	if (nested_processing_results.NumItems() != 0)
	{
		// Emit application defined results of the nested grammars processing.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Application defined nested grammar processing results.");
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"static TNestedProcessingResult %s_nested_results[%d] = ", names_prefix, nested_processing_results.NumItems());
		rprt.WriteFmtLine(L"{");

		int max_nres_name_len = 5;
		for (int i6y=0; i6y<nested_processing_results.NumItems(); ++i6y)
		{
			wchar_t *nr_name = nested_processing_results[i6y].nest_res_name;
			if (nr_name != NULL && (int)wcslen(nr_name)+4 > max_nres_name_len)
				max_nres_name_len = (int)wcslen(nr_name)+4;
		}

		for (int i6z=0; i6z<nested_processing_results.NumItems(); ++i6z)
		{
			TNestedProcessingResult &nres = nested_processing_results[i6z];
			wchar_t nres_name_buff[TGrammar::MAX_NAME_LENGTH+1];
			rprt.WriteFmtLine(L"  { %6hd,  %-*s  %d  }, //  %d", nres.nest_res_app_id,
							max_nres_name_len, MakeOptionalName(nres_name_buff, nres.nest_res_name), nres.event_id, i6z);
		}

		if (emit_ext_comments == TRUE)
			rprt.WriteFmtLine(L"};  // %s_nested_results", names_prefix);
		else rprt.WriteFmtLine(L"};");

		rprt.WriteLine();
	}

	if (xpct_conflicts.NumItems() != 0)
	{
		// Check if at least one conflict has resolution data or not. Find out the length of the longest expected conflict name.
		bool locations_data_present = FALSE;
		bool resolution_data_present = FALSE;
		int longest_xpct_cfct_name_len = 4;
		for (int i6e=0; i6e<xpct_conflicts.NumItems(); ++i6e)
		{
			if (xpct_conflicts[i6e].xpct_conflict_locations.NumItems() > 0)
				locations_data_present = TRUE;
			if (xpct_conflicts[i6e].resolution.NumItems() > 0)
				resolution_data_present = TRUE;

			int len = (int)wcslen(xpct_conflicts[i6e].xpct_conflict_name);
			if (len > longest_xpct_cfct_name_len)
				longest_xpct_cfct_name_len = len;
		}

		if (locations_data_present == TRUE)
		{
			// Emit the locations data.
			rprt.WriteFmtLine(L"//");
			rprt.WriteFmtLine(L"// Expected grammar conflict locations data (indexes).");
			rprt.WriteFmtLine(L"//");

			for (int i6fk=0; i6fk<xpct_conflicts.NumItems(); ++i6fk)
			{
				TSymbolsArray &locs = xpct_conflicts[i6fk].xpct_conflict_locations;
				if (locs.NumItems() == 0)
					continue;

				rprt.WriteFmtLine(L"static WORD %s_xpct_conflict_locations_data_%04d[%d] = ", names_prefix, i6fk, locs.NumItems());
				EmitSymbolsArray(rprt, locs);
			}

			rprt.WriteLine();
		}

		if (resolution_data_present == TRUE)
		{
			// Emit the resolution data.
			rprt.WriteFmtLine(L"//");
			rprt.WriteFmtLine(L"// Expected grammar conflict resolution data (zero for shift action or app_ids).");
			rprt.WriteFmtLine(L"//");

			for (int i6fa=0; i6fa<xpct_conflicts.NumItems(); ++i6fa)
			{
				TSymbolsArrayArray &resolution = xpct_conflicts[i6fa].resolution;
				if (resolution.NumItems() == 0)
					continue;

				for (int i6fb=0; i6fb<resolution.NumItems(); ++i6fb)
				{
					rprt.WriteFmtLine(L"static WORD %s_xpct_conflict_resolution_data_%04d_%04d[%d] = ", names_prefix, i6fa, i6fb, resolution[i6fb].NumItems());
					EmitSymbolsArray(rprt, resolution[i6fb]);
				}
			}

			rprt.WriteLine(L"");

			// Emit the resolution info.
			rprt.WriteFmtLine(L"//");
			rprt.WriteFmtLine(L"// Expected grammar conflict resolutions (zero for shift action or app_id arrays).");
			rprt.WriteFmtLine(L"//");

			for (int i6fc=0; i6fc<xpct_conflicts.NumItems(); ++i6fc)
			{
				TSymbolsArrayArray &resolution = xpct_conflicts[i6fc].resolution;
				if (resolution.NumItems() == 0)
					continue;

				rprt.WriteFmtLine(L"static TSymbolsArray %s_xpct_conflict_resolutions_%04d[%d] = ", names_prefix, i6fc, resolution.NumItems());
				rprt.WriteFmtLine(L"{");

				for (int i6fd=0; i6fd<resolution.NumItems(); ++i6fd)
				{
					rprt.WriteFmtLine(L"  TSymbolsArray(%s_xpct_conflict_resolution_data_%04d_%04d, %d),", names_prefix, i6fc, i6fd, resolution[i6fd].NumItems());
				}

				rprt.WriteFmtLine(L"};");
			}

			rprt.WriteLine();
		}

		// Emit info about the expected grammar conflicts.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Expected grammar conflicts (igrammar=%d).", GetGrammarIndex());
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"static TExpectedGrammarConflict %s_xpct_grammar_conflicts[%d] = ", names_prefix, xpct_conflicts.NumItems());
		rprt.WriteFmtLine(L"{");

		for (int i6g=0; i6g<xpct_conflicts.NumItems(); ++i6g)
		{
			TExpectedGrammarConflict &xpct = xpct_conflicts[i6g];
			wchar_t xpct_cfct_name_buff[80];
			swprintf(xpct_cfct_name_buff, 80, L"L\"%s\"", xpct.xpct_conflict_name);
			rprt.WriteFmt(L"  { %.*s, %5hu, ", longest_xpct_cfct_name_len+4, xpct_cfct_name_buff, xpct.xpct_conflict_app_id);

			if (xpct.xpct_conflict_locations.NumItems() > 0)
				rprt.WriteFmt(L"  TSymbolsArray(%s_xpct_conflict_locations_data_%04d, %2d), ", names_prefix, i6g, xpct.xpct_conflict_locations.NumItems());
			else rprt.WriteFmt(L"  TSymbolsArray(), ");

			rprt.WriteFmt(L"%5d, ", xpct.num_conforming_conflicts);

			if (xpct.resolution.NumItems() > 0 || xpct.nested_grammar != NULL)
			{
				if (xpct.resolution.NumItems() > 0)
					rprt.WriteFmt(L"  TSymbolsArrayArray(%s_xpct_conflict_resolutions_%04d, %2d), ", names_prefix, i6g, xpct.resolution.NumItems());
				else rprt.WriteFmt(L"  TSymbolsArrayArray(), ");

				if (xpct.nested_grammar != NULL)
					rprt.WriteFmt(L"&%s_%s_grammar, ", names_prefix, xpct.xpct_conflict_name);
			}
			else
			{
				rprt.Write(L"  ");
			}

			rprt.WriteFmtLine(L"},  // %2d", i6g);
		}

		if (emit_ext_comments == TRUE)
			rprt.WriteFmtLine(L"};  // %s_xpct_grammar_conflicts", names_prefix);
		else rprt.WriteFmtLine(L"};");

		rprt.WriteLine();
	}

	if (parsing_states.NumItems() != 0)
	{
		// Emit positions for each parsing state.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Rule positions of the parsing states for the LR1 conversion method.");
		rprt.WriteFmtLine(L"// All positions are emitted. Not only those that differ by irule, isym.");
		rprt.WriteFmtLine(L"// Available action symbols can be veiwed/inspected below.");
		rprt.WriteFmtLine(L"//");

		for (int i7=0; i7<parsing_states.NumItems(); ++i7)
		{
			TParsingState &stt = parsing_states[i7];
			rprt.WriteFmtLine(L"static TRulePosition %s_rule_positions_for_parsing_state_%04d[%d] = ", names_prefix, i7, stt.NumItems());
			rprt.WriteFmtLine(L"{");

			int cnt_in_line = 0;
			for (int ipos=0; ipos<stt.NumItems(); ++ipos)
			{
				if (cnt_in_line == 0)
					rprt.Write(L"  ");

				rprt.WriteFmt(L"{ %3hu, %2hu, %3hu },", stt[ipos].irule, stt[ipos].isym, stt[ipos].action_sym);

				if (++cnt_in_line >= NumParsSttRulePositionsPerLine || ipos == stt.NumItems()-1)
				{
					rprt.WriteLine(L"");
					cnt_in_line = 0;
				}
				else
				{
					rprt.Write(L" ");
				}
			}

			rprt.WriteLine(L"};");
		}

		rprt.WriteLine();

		// Emit the list of positions.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Parsing states (igrammar=%d).", GetGrammarIndex());
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"static TParsingState %s_parsing_states[%d] = ", names_prefix, parsing_states.NumItems());
		rprt.WriteFmtLine(L"{");

		for (int i8=0; i8<parsing_states.NumItems(); ++i8)
		{
			rprt.WriteFmtLine(L"  TParsingState(%s_rule_positions_for_parsing_state_%04d, %5d),  // %d", names_prefix, i8, parsing_states[i8].NumItems(), i8);
		}

		if (emit_ext_comments == TRUE)
			rprt.WriteFmtLine(L"};  // %s_parsing_states", names_prefix);
		else rprt.WriteFmtLine(L"};");

		rprt.WriteLine();
	}

	// When array of conflicts is long, it should be emitted as sequence of several segments. Otherwise MS compiler
	// is not able to compile this array. When arrays is emitted in segments there is minor chance that its assembling
	// will fail when complete grammar will be instantiated.
	int max_len_cfcts_segm = conflicts.NumItemsReallocExtra();

	if (conflicts.NumItems() != 0)
	{
		// Check if at least one grammar conflict has resolution data or analysis results.
		bool resolution_data_present = FALSE;
		bool analysis_results_present = FALSE;
		for (int icf0=0; icf0<conflicts.NumItems(); ++icf0)
		{
			if (conflicts[icf0].conflict_resolution.NumItems() > 0)
				resolution_data_present = TRUE;

			if (conflicts[icf0].analysis_results.NumItems() > 0)
				analysis_results_present = TRUE;
		}

		if (analysis_results_present == TRUE)
		{
			//
			// Emit conflicts analysis results data.
			//

			// Positions of the derivation paths.
			rprt.WriteFmtLine(L"//");
			rprt.WriteFmtLine(L"// Grammar conflict analysis derivation path rule positions.");
			rprt.WriteFmtLine(L"//");

			for (int icf1=0; icf1<conflicts.NumItems(); ++icf1)
			{
				if (conflicts[icf1].analysis_results.NumItems() == 0)
					continue;

				// Current conflict has analysis results at least for one of its conflicting actions.
				int num_action_records = conflicts[icf1].analysis_results.NumItems();
				assert(num_action_records == conflicts[icf1].NumActions());

				for (int iac1=0; iac1<num_action_records; ++iac1)
				{
					TGrammarDerivationPathsArray &paths = conflicts[icf1].analysis_results[iac1].m_derivation_paths;
					if (paths.NumItems() == 0)
						continue;

					// Current conflicting action contains paths.
					for (int ipath1=0; ipath1<paths.NumItems(); ++ipath1)
					{
						// Pick up the path and ensure that it is not empty.
						TGrammarDerivationPath &path = paths[ipath1];
						assert(path.derv_path_len > 0 && path.derv_steps != NULL);

						rprt.WriteFmtLine(L"static TFullRulePosition %s_conflict_analysis_full_rule_pos_%03d_%03d_%03d[%d] = ", names_prefix, icf1, iac1, ipath1, path.derv_path_len);
						rprt.WriteFmtLine(L"{");

						for (int istep1=0; istep1<path.derv_path_len; ++istep1)
						{
							TFullRulePosition &pos = path.derv_steps[istep1];
							rprt.WriteFmtLine(L"  { %4hd, %3hd, %23s, %5hd, %3hd },  // %d",
											pos.irule, (WORD)pos.isym, TFullRulePosition::GetFullRulePosOriginEnumName(pos.origin), pos.istate, pos.action_sym, istep1);
						}

						rprt.WriteFmtLine(L"};");
					}
				}
			}

			rprt.WriteFmtLine(L"");

			// Emit derivation paths themselves.
			rprt.WriteFmtLine(L"//");
			rprt.WriteFmtLine(L"// Grammar conflict analysis derivation paths.");
			rprt.WriteFmtLine(L"//");

			for (int icf2=0; icf2<conflicts.NumItems(); ++icf2)
			{
				if (conflicts[icf2].analysis_results.NumItems() == 0)
					continue;

				int num_action_records = conflicts[icf2].analysis_results.NumItems();
				assert(num_action_records == conflicts[icf2].NumActions());

				for (int iac2=0; iac2<num_action_records; ++iac2)
				{
					TGrammarDerivationPathsArray &paths = conflicts[icf2].analysis_results[iac2].m_derivation_paths;
					if (paths.NumItems() == 0)
						continue;

					// Current (iconflict, iaction) contains paths.
					rprt.WriteFmtLine(L"static TGrammarDerivationPath %s_conflict_analysis_derivation_paths_%03d_%03d[%d] = ", names_prefix, icf2, iac2, paths.NumItems());
					rprt.WriteFmtLine(L"{");

					for (int ipath2=0; ipath2<paths.NumItems(); ++ipath2)
					{
						TGrammarDerivationPath &path = paths.ItemRef(ipath2);
						rprt.WriteFmtLine(L"  { %14I64d, %2d, %s_conflict_analysis_full_rule_pos_%03d_%03d_%03d },  // %d",
										path.cnt_similar, path.derv_path_len, names_prefix, icf2, iac2, ipath2, ipath2);
					}

					rprt.WriteFmtLine(L"};");
				}
			}

			rprt.WriteFmtLine(L"");

			// Array of paths arrays.
			rprt.WriteFmtLine(L"//");
			rprt.WriteFmtLine(L"// Grammar conflict analysis results.");
			rprt.WriteFmtLine(L"//");

			for (int icf3=0; icf3<conflicts.NumItems(); ++icf3)
			{
				if (conflicts[icf3].analysis_results.NumItems() == 0)
					continue;

				// Current conflict contains the analysis results.
				int num_action_records = conflicts[icf3].analysis_results.NumItems();
				assert(num_action_records == conflicts[icf3].NumActions());

				rprt.WriteFmtLine(L"static TConflictingActionAnalysisResults %s_conflict_analysis_results_%03d[%d] = ", names_prefix, icf3, num_action_records);
				rprt.WriteFmtLine(L"{");

				for (int iac3=0; iac3<num_action_records; ++iac3)
				{
					TConflictingActionAnalysisResults &act_res = conflicts[icf3].analysis_results[iac3];

					// Analysis result name.
					wchar_t gen_res_name[80];
					swprintf(gen_res_name, 80, L"TConflictingActionAnalysisResults::%s,", TConflictingActionAnalysisResults::GetGenResultEnumName(act_res.m_generation_result));

					rprt.WriteFmt(L"  { %-62s %16I64d, %18I64d, %6I64d, %6I64d,", gen_res_name,
								act_res.m_generation_steps, act_res.m_generation_time, act_res.m_too_deep_truncations, act_res.m_partial_iteration_aborts);

					// Derivation paths in the array if any.
					if (act_res.m_derivation_paths.NumItems() > 0)
						rprt.WriteFmt(L"  TGrammarDerivationPathsArray(%s_conflict_analysis_derivation_paths_%03d_%03d, %3d) ", names_prefix, icf3, iac3, act_res.m_derivation_paths.NumItems());
					else rprt.WriteFmt(L"  TGrammarDerivationPathsArray(NULL, 0) ");

					wchar_t buffer2[80], buffer3[80];
					rprt.WriteFmtLine(L"},  // %s steps, %s elapsed time.", FormatInt64(act_res.m_generation_steps, buffer2, 80, fnms_dec_signed, L'_'),
									FormatDuration(act_res.m_generation_time, buffer3, 80, TRUE));
				}

				rprt.WriteFmtLine(L"};");
			}

			rprt.WriteFmtLine(L"");
		}

		// Emit conflicting actions for each conflict.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Conflicting actions.");
		rprt.WriteFmtLine(L"//");

		for (int i9=0; i9<conflicts.NumItems(); ++i9)
		{
			TGrammarConflict &cfct = conflicts[i9];
			rprt.WriteFmtLine(L"static WORD %s_conflicting_actions_%03d[%d] = ", names_prefix, i9, cfct.conflicting_actions.NumItems());
			rprt.WriteFmtLine(L"{");

			int cnt_in_line = 0;
			for (int irec=0; irec<cfct.conflicting_actions.NumItems(); ++irec)
			{
				if (cnt_in_line == 0)
					rprt.Write(L"  ");

				rprt.WriteFmt(L"%5hu, ", cfct.conflicting_actions[irec]);
				if (++cnt_in_line >= 8 || irec == cfct.conflicting_actions.NumItems()-1)
				{
					rprt.WriteLine();
					cnt_in_line = 0;
				}
			}

			rprt.WriteLine(L"};");
		}

		rprt.WriteLine();

		if (resolution_data_present == TRUE)
		{
			// Emit the conflict resolution data.
			rprt.WriteFmtLine(L"//");
			rprt.WriteFmtLine(L"// Conflict resolutions.");
			rprt.WriteFmtLine(L"//");
			for (int i10=0; i10<conflicts.NumItems(); ++i10)
			{
				TSymbolsArray &resolution = conflicts[i10].conflict_resolution;
				if (resolution.NumItems() == 0)
					continue;

				rprt.WriteFmtLine(L"static WORD %s_conflict_resolution_data_%03d[%d] = ", names_prefix, i10, resolution.NumItems());
				EmitSymbolsArray(rprt, resolution);
			}

			rprt.WriteLine();
		}

		// Emit the array of conflicts.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Grammar conflicts (igrammar=%d).", GetGrammarIndex());
		rprt.WriteFmtLine(L"//");

		if (conflicts.NumItems() <= max_len_cfcts_segm)
		{
			// Emit info about all conflicts as one segment.
			rprt.WriteFmtLine(L"static TGrammarConflict %s_conflicts[%d] = ", names_prefix, conflicts.NumItems());
			rprt.WriteFmtLine(L"{");

			for (int i11a=0; i11a<conflicts.NumItems(); ++i11a)
				EmitSingleGrammarConflictInfo(rprt, names_prefix, i11a);

			if (emit_ext_comments == TRUE)
				rprt.WriteFmtLine(L"};  // %s_conflicts", names_prefix);
			else rprt.WriteFmtLine(L"};");
		}
		else
		{
			// Emit info about conflicts as sequence of several segments.
			int num_conflicts_to_emit = conflicts.NumItems();
			int segment_index_base = 0;
			while (num_conflicts_to_emit > 0)
			{
				int len_segment = (num_conflicts_to_emit > max_len_cfcts_segm) ? max_len_cfcts_segm : num_conflicts_to_emit;
				rprt.WriteFmtLine(L"static TGrammarConflict %s_conflicts_segment_%02d[%d] = ", names_prefix, segment_index_base/max_len_cfcts_segm, len_segment);
				rprt.WriteFmtLine(L"{");

				for (int i11b=0; i11b<len_segment; ++i11b)
					EmitSingleGrammarConflictInfo(rprt, names_prefix, segment_index_base+i11b);

				rprt.WriteFmtLine(L"};");

				segment_index_base += len_segment;
				num_conflicts_to_emit -= len_segment;
			}

			// Emit directory of segments.
			int num_segments = (conflicts.NumItems()+max_len_cfcts_segm-1)/max_len_cfcts_segm;
			rprt.WriteLine(L"");
			rprt.WriteFmtLine(L"static TGrammarConflict *%s_conflict_segments_directory[%d] = ", names_prefix, num_segments);
			rprt.WriteFmtLine(L"{");

			for (int i11c=0; i11c<num_segments; ++i11c)
				rprt.WriteFmtLine(L"  %s_conflicts_segment_%02d, ", names_prefix, i11c);

			rprt.WriteFmtLine(L"};");
		}

		rprt.WriteFmtLine(L"");
	}

	// Main lexema types data.
	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_lex_data, ltx_num_lexema_types, (parent_grammar == NULL) ? 0xFFFF : 0) == FALSE)
	{
		rprt.WriteFmtLine(L"// Lexema types processing.");
		rprt.WriteFmtLine(L"static WORD %s_lex_type_data[] = ", names_prefix);
		rprt.WriteFmtLine(L"{");

		for (int lsc0=0; lsc0<ltx_num_lexema_types; ++lsc0)
		{
			rprt.WriteFmtLine(L"  %5hd,  // %s", lex_to_sym_conv.m_lex_data[lsc0], TLexema::GetLexTypeEnumName((TLexemaType)lsc0));
		}

		rprt.WriteFmtLine(L"};");
		rprt.WriteFmtLine(L"");
	}

	// Comment subtypes data.
	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_comment_data, lct_num_comment_types, (parent_grammar == NULL) ? 0xFFFF : 0) == FALSE)
	{
		rprt.WriteFmtLine(L"// Comment types processing.");
		rprt.WriteFmtLine(L"static WORD %s_comment_type_data[] = ", names_prefix);
		rprt.WriteFmtLine(L"{");

		for (int lsc1=0; lsc1<lct_num_comment_types; ++lsc1)
		{
			rprt.WriteFmtLine(L"  %5hd,  // %s",
							lex_to_sym_conv.m_ltx_comment_data[lsc1],
							TLexema::GetLexCommentTypeEnumName((TLexCommentType)lsc1));
		}

		rprt.WriteFmtLine(L"};");
		rprt.WriteFmtLine(L"");
	}

	// Number subtypes data.
	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_number_data, lnt_num_number_types, (parent_grammar == NULL) ? 0xFFFF : 0) == FALSE)
	{
		rprt.WriteFmtLine(L"// Number types processing.");
		rprt.WriteFmtLine(L"static WORD %s_number_type_data[] = ", names_prefix);
		rprt.WriteFmtLine(L"{");

		for (int lsc2a=0; lsc2a<lnt_num_number_types; ++lsc2a)
		{
			rprt.WriteFmtLine(L"  %5hd,  // %s",
							lex_to_sym_conv.m_ltx_number_data[lsc2a],
							TLexema::GetLexNumberTypeEnumName((TLexNumberType)lsc2a));
		}

		rprt.WriteFmtLine(L"};");
		rprt.WriteFmtLine(L"");
	}

	// Floating point subtypes data.
	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_floating_point_data, lfp_num_floating_point_types, (parent_grammar == NULL) ? 0xFFFF : 0) == FALSE)
	{
		rprt.WriteFmtLine(L"// Floating point constant types processing.");
		rprt.WriteFmtLine(L"static WORD %s_floating_point_type_data[] = ", names_prefix);
		rprt.WriteFmtLine(L"{");

		for (int lsc2b=0; lsc2b<lfp_num_floating_point_types; ++lsc2b)
		{
			rprt.WriteFmtLine(L"  %5hd,  // %s",
							lex_to_sym_conv.m_ltx_floating_point_data[lsc2b],
							TLexema::GetLexFloatingPointTypeEnumName((TLexFloatingPointType)lsc2b));
		}

		rprt.WriteFmtLine(L"};");
		rprt.WriteFmtLine(L"");
	}

	// Charconst subtypes data.
	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_charconst_data, lchct_num_charconst_types, (parent_grammar == NULL) ? 0xFFFF : 0) == FALSE)
	{
		rprt.WriteFmtLine(L"// Char const types processing.");
		rprt.WriteFmtLine(L"static WORD %s_charconst_type_data[] = ", names_prefix);
		rprt.WriteFmtLine(L"{");

		for (int lsc3=0; lsc3<lchct_num_charconst_types; ++lsc3)
		{
			rprt.WriteFmtLine(L"  %5hd,  // %s",
							lex_to_sym_conv.m_ltx_charconst_data[lsc3],
							TLexema::GetLexCharConstTypeEnumName((TLexCharConstType)lsc3));
		}

		rprt.WriteFmtLine(L"};");
		rprt.WriteFmtLine(L"");
	}

	// String subtypes data.
	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_string_data, lstrt_num_string_types, (parent_grammar == NULL) ? 0xFFFF : 0) == FALSE)
	{
		rprt.WriteFmtLine(L"// String types processing.");
		rprt.WriteFmtLine(L"static WORD %s_string_type_data[] = ", names_prefix);
		rprt.WriteFmtLine(L"{");

		for (int lsc4=0; lsc4<lstrt_num_string_types; ++lsc4)
		{
			rprt.WriteFmtLine(L"  %5hd,  // %s",
							lex_to_sym_conv.m_ltx_string_data[lsc4],
							TLexema::GetLexStringTypeEnumName((TLexStringType)lsc4));
		}

		rprt.WriteFmtLine(L"};");
		rprt.WriteFmtLine(L"");
	}

	// Keyword subtypes data.
	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_keyword_data, ltkn_num_keyword_vals, (parent_grammar == NULL) ? 0xFFFF : 0) == FALSE)
	{
		rprt.WriteFmtLine(L"// Token ids processing.");
		rprt.WriteFmtLine(L"static WORD %s_keyword_type_data[] = ", names_prefix);
		rprt.WriteFmtLine(L"{");

		int max_used_token = 0;
		for (int lsc5=0; lsc5<ltkn_num_keyword_vals; ++lsc5)
		{
			wchar_t buff[80];
			bool reserved_val;
			wchar_t *enum_name = (wchar_t*)TLexema::GetKeywordIdEnumName(lsc5, &reserved_val);
			if (reserved_val == TRUE)
			{
				swprintf(buff, 80, L"%d (reserved value)", lsc5);
				enum_name = buff;
			}

			rprt.WriteFmtLine(L"  %5hd,  // %s (%d)", lex_to_sym_conv.m_ltx_keyword_data[lsc5], enum_name, lsc5);
		}

		rprt.WriteFmtLine(L"};");
		rprt.WriteFmtLine(L"");
	}

	// Name subtypes data.
	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_name_data, TLexToSymConverter::lt_num_name_types, (parent_grammar == NULL) ? 0xFFFF : 0) == FALSE)
	{
		rprt.WriteFmtLine(L"// Name subtypes processing.");
		rprt.WriteFmtLine(L"static WORD %s_name_type_data[] = ", names_prefix);
		rprt.WriteFmtLine(L"{");

		for (int lsc6=0; lsc6<TLexToSymConverter::lt_num_name_types; ++lsc6)
		{
			rprt.WriteFmtLine(L"  %5hd,  // %d", lex_to_sym_conv.m_ltx_name_data[lsc6], lsc6);
		}

		rprt.WriteFmtLine(L"};");
		rprt.WriteFmtLine(L"");
	}

	// End of line subtypes data.
	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_lex_endofline_data, leolt_num_eol_types, (parent_grammar == NULL) ? 0xFFFF : 0) == FALSE)
	{
		rprt.WriteFmtLine(L"// End of line types processing.");
		rprt.WriteFmtLine(L"static WORD %s_endofline_type_data[] = ", names_prefix);
		rprt.WriteFmtLine(L"{");

		for (int lsc7=0; lsc7<leolt_num_eol_types; ++lsc7)
		{
			rprt.WriteFmtLine(L"  %5hd,  // %s",
							lex_to_sym_conv.m_lex_endofline_data[lsc7],
							TLexema::GetLexEndOfLineTypeEnumName((TLexEndOfLineType)lsc7));
		}

		rprt.WriteFmtLine(L"};");
		rprt.WriteFmtLine(L"");
	}

	if (analysis_table.IsInited() == TRUE)
	{
		// Emit the analysis table.
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"// Syntax analysis table (%d parsing state row%s, %d symbol column%s).",
							analysis_table.GetRowsUsed(), PluralSuffix(analysis_table.GetRowsUsed()), analysis_table.GetWidth(), PluralSuffix(analysis_table.GetWidth()));
		rprt.WriteFmtLine(L"//");
		rprt.WriteFmtLine(L"static WORD %s_analysis_table[%d] = ", names_prefix, analysis_table.GetRowsUsed()*analysis_table.GetWidth());
		rprt.WriteFmtLine(L"{");

		for (int stt=0; stt<analysis_table.GetRowsUsed(); ++stt)
		{
			// Get pointer to the beginnning of actions for the current state.
			WORD *ptab = analysis_table.GetRowPtr(stt);

			rprt.WriteFmtLine(L"  // State %d.", stt);

			int cnt_in_line = 0;
			for (int isym=0; isym<analysis_table.GetWidth(); ++isym)
			{
				if (cnt_in_line == 0)
					rprt.WriteFmt(L"  ");

				rprt.WriteFmt(L"%5hu,", ptab[isym]);

				if (++cnt_in_line >= NumAnalysisTableSlotsPerLine || isym == analysis_table.GetWidth()-1)
				{
					if (cnt_in_line >= NumAnalysisTableSlotsPerLine)
						rprt.WriteFmtLine(L" // %d", isym+1-NumAnalysisTableSlotsPerLine);
					else rprt.WriteLine(L"");

					cnt_in_line = 0;
				}
				else
				{
					rprt.Write(L" ");
				}
			}

			if (stt != analysis_table.GetRowsUsed()-1)
				rprt.WriteFmtLine(L"");
		}

		if (emit_ext_comments == TRUE)
			rprt.WriteFmtLine(L"};  // %s_analysis_table", names_prefix);
		else rprt.WriteFmtLine(L"};");

		rprt.WriteFmtLine(L"");
	}

	// Prolog.
	rprt.WriteFmtLine(L"//");
	rprt.WriteFmtLine(L"// Grammar definition (igrammar=%d).", GetGrammarIndex());
	rprt.WriteFmtLine(L"//");
	rprt.WriteFmtLine(L"TGrammar %s_grammar = ", names_prefix);
	rprt.WriteFmtLine(L"{");

	// Info about the parent.
	rprt.WriteFmtLine(L"    // Parent grammar info.");

	if (parent_grammar == NULL)
	{
		rprt.WriteFmtLine(L"    NULL, -1, // Root grammar.");
		rprt.WriteFmtLine(L"");
	}
	else
	{
		assert(parent_prefix != NULL);
		assert(parent_xpct_inx >= 0);

		rprt.WriteFmtLine(L"    &%s_grammar,", parent_prefix);
		rprt.WriteFmtLine(L"    %hd, // Index of xpct conflict of this grammar in the parent grammar.", parent_xpct_inx);
		rprt.WriteFmtLine(L"");
	}

	// Props of the grammar.
	rprt.WriteFmtLine(L"    // Grammar props.");
	rprt.WriteFmtLine(L"    {");

	// Info about the grammar.
	rprt.WriteFmtLine(L"        // General info.");
	rprt.WriteFmtLine(L"        L\"%s\", // Description.", grm_props.grammar_description);
	rprt.WriteFmtLine(L"        0x%X, // Revision.", grm_props.grammar_revision);
	rprt.WriteFmtLine(L"");

	// Part 1. Information about the grammar source.
	rprt.WriteFmtLine(L"        // Information about the grammar source.");
	rprt.Write(L"        L\"");

	int grms_fname_len = (int)wcslen(grm_props.grms_file_name);
	for (int ii=0; ii<grms_fname_len; ++ii)
	{
		if (grm_props.grms_file_name[ii] == L'\\' || grm_props.grms_file_name[ii] == L'/')
			rprt.Write(L"\\\\");
		else rprt.WriteFmt(L"%c", grm_props.grms_file_name[ii]);
	}

	// Hide allocation of the local string into the block.
	{
		wchar_t loc_date_buff[80];
		rprt.WriteLine(L"\",");
		rprt.WriteFmtLine(L"        %ld,  // File length.", grm_props.grms_file_length);
		rprt.WriteFmtLine(L"        %I64d, // File date (%s).", grm_props.grms_file_date, FormatDateTime(grm_props.grms_file_date, loc_date_buff, 80, FALSE));
		rprt.WriteFmtLine(L"");
	}

	// Part 2. Parsing session id and processing result.
	rprt.WriteFmtLine(L"        // Session Id in the parsing events database and processing result.");
	rprt.WriteFmtLine(L"        %lu,", grm_props.parsing_id);
	rprt.WriteFmtLine(L"        %s,", MakeGrammarProcResEnumName(grm_props.processing_result));
	rprt.WriteFmtLine(L"");

	// Part 3. High level language type and other info.
	rprt.WriteFmtLine(L"        // High level language type (if known).");
	rprt.WriteFmtLine(L"        %s,", THighLevelDatabase::GetLangTypeEnumName(grm_props.lang_type));
	rprt.WriteFmtLine(L"");
	rprt.WriteFmtLine(L"        // Callback handler info and source code emitting info.");
	rprt.WriteFmtLine(L"        L\"%s\",", grm_props.cbk_handler_name);
	rprt.WriteFmtLine(L"        L\"%s\", // Preferred prefix of the data fields when grammar is emitted as a source code.", grm_props.preferred_emitting_prefix);

	// End of the grammar props.
	rprt.WriteFmtLine(L"    },");
	rprt.WriteFmtLine(L"");

	// Part 4. Source arrays.
	rprt.WriteFmtLine(L"    // Terminal symbol objects.");

	if (symbols.NumItems() != 0)
		rprt.WriteFmtLine(L"    TTerminalSymbolsArray(%s_terminal_symbols, %d),", names_prefix, NumTerminals());
	else rprt.WriteFmtLine(L"    TTerminalSymbolsArray(),");

	if (ignore_lex.NumItems() != 0)
		rprt.WriteFmtLine(L"    TIgnoreLexRecordsArray(%s_ignore_lex_records, %d),", names_prefix, ignore_lex.NumItems());
	else rprt.WriteFmtLine(L"    TIgnoreLexRecordsArray(),");

	if (error_lex.NumItems() != 0)
		rprt.WriteFmtLine(L"    TErrorLexRecordsArray(%s_error_lex_records, %d),", names_prefix, error_lex.NumItems());
	else rprt.WriteFmtLine(L"    TErrorLexRecordsArray(),");

	// Part 5. Axioma, non terminals and rules.
	rprt.WriteFmtLine(L"");
	rprt.WriteFmtLine(L"    // Axioma.");
	rprt.WriteFmtLine(L"    %hd,  // Axioma symbol.", axioma_ident);
	rprt.WriteFmtLine(L"");

	rprt.WriteFmtLine(L"    // Non terminals and rules.");

	if (non_terminals.NumItems() != 0)
		rprt.WriteFmtLine(L"    TNonTerminalsArray(%s_non_terminals, %d),", names_prefix, NumNonTerminals());
	else rprt.WriteFmtLine(L"    TNonTerminalsArray(),");

	if (rules.NumItems() != 0)
		rprt.WriteFmtLine(L"    TGrammarRulesArray(%s_rules, %d),", names_prefix, NumRules());
	else rprt.WriteFmtLine(L"    TGrammarRulesArray(),");

	rprt.WriteFmtLine(L"");
	rprt.WriteFmtLine(L"    // Expected conclict related objects.");

	if (location_objects.NumItems() != 0)
		rprt.WriteFmtLine(L"    TExpectedConflictLocationsArray(%s_xpct_conflict_locations, %d),", names_prefix, location_objects.NumItems());
	else rprt.WriteFmtLine(L"    TExpectedConflictLocationsArray(),");

	if (nested_processing_results.NumItems() != 0)
		rprt.WriteFmtLine(L"    TNestedProcessingResultsArray(%s_nested_results, %d),", names_prefix, nested_processing_results.NumItems());
	else rprt.WriteFmtLine(L"    TNestedProcessingResultsArray(),");

	if (xpct_conflicts.NumItems() != 0)
		rprt.WriteFmtLine(L"    TExpectedGrammarConflictsArray(%s_xpct_grammar_conflicts, %d),", names_prefix, xpct_conflicts.NumItems());
	else rprt.WriteFmtLine(L"    TExpectedGrammarConflictsArray(),");

	rprt.WriteFmtLine(L"");

	// Part 6. Parsing states and conflicts.
	rprt.WriteFmtLine(L"    // Derived arrays.");

	if (parsing_states.NumItems() != 0)
		rprt.WriteFmtLine(L"    TParsingStatesArray(%s_parsing_states, %d, %d),", names_prefix, NumParsingStates(), parsing_states.num_positions);
	else rprt.WriteFmtLine(L"    TParsingStatesArray(),");

	if (conflicts.NumItems() <= 0)
	{
		// There are no conflicts in the grammar.
		rprt.WriteFmtLine(L"    TGrammarConflictsArray(),");
	}
	else if (conflicts.NumItems() <= max_len_cfcts_segm)
	{
		// Info about conflicts was emitted as one segment.
		rprt.WriteFmtLine(L"    TGrammarConflictsArray(%s_conflicts, %d),", names_prefix, NumConflicts());
	}
	else
	{
		// Info about conflicts was emitted as sequence of several segments.
		rprt.WriteFmtLine(L"    TGrammarConflictsArray(%s_conflict_segments_directory, %d),", names_prefix, NumConflicts());
	}

	rprt.WriteFmtLine(L"");

	// Part 7. Lexema to terminal symbol converter.
	rprt.WriteFmtLine(L"    // Lex to sym converter.");
	rprt.WriteFmtLine(L"    TLexToSymConverter");
	rprt.WriteFmtLine(L"    (");

	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_lex_data, ltx_num_lexema_types, (parent_grammar == NULL) ? 0xFFFF : 0x0000) == FALSE)
		rprt.WriteFmtLine(L"        ltsc_use_setup_data, %s_lex_type_data,", names_prefix);
	else rprt.WriteFmtLine(L"        %s NULL, // lex_type_data.", (parent_grammar == NULL) ? L"ltsc_fill_unmapped, " : L"ltsc_fill_eof,");

	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_comment_data, lct_num_comment_types, (parent_grammar == NULL) ? 0xFFFF : 0x0000) == FALSE)
		rprt.WriteFmtLine(L"        ltsc_use_setup_data, %s_comment_type_data,", names_prefix);
	else rprt.WriteFmtLine(L"        %s NULL, // comment_type_data.", (parent_grammar == NULL) ? L"ltsc_fill_unmapped, " : L"ltsc_fill_eof,");

	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_number_data, lnt_num_number_types, (parent_grammar == NULL) ? 0xFFFF : 0x0000) == FALSE)
		rprt.WriteFmtLine(L"        ltsc_use_setup_data, %s_number_type_data,", names_prefix);
	else rprt.WriteFmtLine(L"        %s NULL, // number_type_data.", (parent_grammar == NULL) ? L"ltsc_fill_unmapped, " : L"ltsc_fill_eof,");

	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_floating_point_data, lfp_num_floating_point_types, (parent_grammar == NULL) ? 0xFFFF : 0x0000) == FALSE)
		rprt.WriteFmtLine(L"        ltsc_use_setup_data, %s_floating_point_type_data,", names_prefix);
	else rprt.WriteFmtLine(L"        %s NULL, // floating_point_type_data.", (parent_grammar == NULL) ? L"ltsc_fill_unmapped, " : L"ltsc_fill_eof,");

	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_charconst_data, lchct_num_charconst_types, (parent_grammar == NULL) ? 0xFFFF : 0x0000) == FALSE)
		rprt.WriteFmtLine(L"        ltsc_use_setup_data, %s_charconst_type_data,", names_prefix);
	else rprt.WriteFmtLine(L"        %s NULL, // charconst_type_data.", (parent_grammar == NULL) ? L"ltsc_fill_unmapped, " : L"ltsc_fill_eof,");

	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_string_data, lstrt_num_string_types, (parent_grammar == NULL) ? 0xFFFF : 0x0000) == FALSE)
		rprt.WriteFmtLine(L"        ltsc_use_setup_data, %s_string_type_data,", names_prefix);
	else rprt.WriteFmtLine(L"        %s NULL, // string_type_data.", (parent_grammar == NULL) ? L"ltsc_fill_unmapped, " : L"ltsc_fill_eof,");

	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_keyword_data, ltkn_num_keyword_vals, (parent_grammar == NULL) ? 0xFFFF : 0x0000) == FALSE)
		rprt.WriteFmtLine(L"        ltsc_use_setup_data, %s_keyword_type_data,", names_prefix);
	else rprt.WriteFmtLine(L"        %s NULL, // keyword_type_data.", (parent_grammar == NULL) ? L"ltsc_fill_unmapped, " : L"ltsc_fill_eof,");

	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_ltx_name_data, TLexToSymConverter::lt_num_name_types, (parent_grammar == NULL) ? 0xFFFF : 0x0000) == FALSE)
		rprt.WriteFmtLine(L"        ltsc_use_setup_data, %s_name_type_data,", names_prefix);
	else rprt.WriteFmtLine(L"        %s NULL, // name_type_data.", (parent_grammar == NULL) ? L"ltsc_fill_unmapped, " : L"ltsc_fill_eof,");

	if (lex_to_sym_conv.IsSameValue(lex_to_sym_conv.m_lex_endofline_data, leolt_num_eol_types, (parent_grammar == NULL) ? 0xFFFF : 0x0000) == FALSE)
		rprt.WriteFmtLine(L"        ltsc_use_setup_data, %s_endofline_type_data", names_prefix);
	else rprt.WriteFmtLine(L"        %s NULL  // endofline_type_data.", (parent_grammar == NULL) ? L"ltsc_fill_unmapped, " : L"ltsc_fill_eof,");

	rprt.WriteFmtLine(L"    ),");
	rprt.WriteFmtLine(L"");

	// Part 8. Analysis table.
	rprt.WriteFmtLine(L"    // Analysis table.");
	if (analysis_table.IsInited() == TRUE)
		rprt.WriteFmtLine(L"    TAnalysisTable(%s_analysis_table, %d, %d),", names_prefix, analysis_table.GetWidth(), analysis_table.GetRowsUsed());
	else rprt.WriteFmtLine(L"    TAnalysisTable(),");

	// Epilog.
	if (emit_ext_comments == TRUE)
		rprt.WriteFmtLine(L"};  // %s_grammar", names_prefix);
	else rprt.WriteFmtLine(L"};");

	rprt.WriteFmtLine(L"");
	rprt.WriteFmtLine(L"");
}

void TGrammar::EmitSingleGrammarConflictInfo(TDestinationFile &rprt, const wchar_t *names_prefix, int iconflict)
{
	TGrammarConflict &cfct = conflicts[iconflict];
	rprt.WriteFmt(L"  { %5hd, %3hd, TGrammarConflictActionsArray(%s_conflicting_actions_%03d, %2d), %5hd, %5hd, ",
				cfct.parser_state, cfct.input_symbol, names_prefix, iconflict, cfct.conflicting_actions.NumItems(),
				cfct.conflict_location_inx, cfct.expected_conflict_inx);

	if (cfct.nested_grammar != NULL)
	{
		// Look for the name of the expected conflict.
		assert(cfct.expected_conflict_inx >= 0 && cfct.expected_conflict_inx < xpct_conflicts.NumItems());
		rprt.WriteFmt(L"&%s_%s_grammar, ", names_prefix, xpct_conflicts[cfct.expected_conflict_inx].xpct_conflict_name);
	}
	else
	{
		rprt.WriteFmt(L"NULL, ");
	}

	if (cfct.conflict_resolution.NumItems() > 0)
		rprt.WriteFmt(L"TSymbolsArray(%s_conflict_resolution_data_%03d, %2d), ", names_prefix, iconflict, cfct.conflict_resolution.NumItems());
	else rprt.WriteFmt(L"TSymbolsArray(), ");

	if (cfct.analysis_results.NumItems() > 0)
		rprt.WriteFmtLine(L"TGrammarConflictAnalysisResults(%s_conflict_analysis_results_%03d, %2d) ", names_prefix, iconflict, cfct.analysis_results.NumItems());

	rprt.WriteFmtLine(L"},  // %d", iconflict);
}

void TGrammar::EmitSymbolsArray(TDestinationFile &rprt, TSymbolsArray &data, int num_values_per_line)
{
	rprt.WriteLine(L"{");
	int cnt_in_line = 0;
	for (int isym=0; isym<data.NumItems(); ++isym)
	{
		if (cnt_in_line == 0)
			rprt.Write(L"  ");

		rprt.WriteFmt(L"%5hu,", data[isym]);

		if (++cnt_in_line >= num_values_per_line || isym == data.NumItems()-1)
		{
			rprt.WriteLine();
			cnt_in_line = 0;
		}
		else
		{
			rprt.Write(L" ");
		}
	}

	rprt.WriteLine(L"};");
}

const wchar_t *TGrammar::MakeOptionalName(wchar_t *buffer_with_grammar_max_name_len_size, const wchar_t *name)
{
	if (name == NULL || name[0] == 0)
		return(L"NULL,");

	swprintf(buffer_with_grammar_max_name_len_size, TGrammar::MAX_NAME_LENGTH+1, L"L\"%s\",", name);
	return(buffer_with_grammar_max_name_len_size);
}

const wchar_t *TGrammar::MakeLexSubtypeEnumName(TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt, wchar_t *buff_40ch)
{
	if (check_subt == FALSE)
		return(L"0");

	switch (lex_type)
	{
		case ltx_comment:
				return(TLexema::GetLexCommentTypeEnumName(lex_subt.comment_type));

		case ltx_number:
				return(TLexema::GetLexNumberTypeEnumName(lex_subt.number_type));

		case ltx_floating_point:
				return(TLexema::GetLexFloatingPointTypeEnumName(lex_subt.floating_point_type));

		case ltx_charconst:
				return(TLexema::GetLexCharConstTypeEnumName(lex_subt.charconst_type));

		case ltx_string:
				return(TLexema::GetLexStringTypeEnumName(lex_subt.string_type));

		case ltx_keyword:
				{
					// Token lexemas should use generic field of the union because the subtype contains
					// the keyword_id and not the token subtype.
					return(TLexema::GetKeywordIdEnumName(lex_subt.subtype));
				}

		case ltx_name:
				{
					// Only derived class knows what the subtype of the name means. Return subtype of the name
					// in the numeric form for now.
					swprintf(buff_40ch, 40, L"%lu", lex_subt.subtype);
					return(buff_40ch);
				}

		case ltx_eol:
				return(TLexema::GetLexEndOfLineTypeEnumName(lex_subt.endofline_type));
	}

	// Other lex types do not support subtype differentiation.
	return(L"n/a");
}

const wchar_t *TGrammar::MakeNonTermRestrTypeEnumName(TNonTermRestrType val)
{
	switch (val)
	{
		case ntrs_none:	return(L"ntrs_none");
		case ntrs_seq:	return(L"ntrs_seq");
		case ntrs_list:	return(L"ntrs_list");
	}

	assert(FALSE);
	return(L"NonTermRestr_BogusValue");
}

const wchar_t *TGrammar::MakeGrammarProcResEnumName(TGrammarProcessingResult val)
{
	switch (val)
	{
		case grpr_none:				return(L"grpr_none");
		case grpr_syntax_errors:		return(L"grpr_syntax_errors");
		case grpr_conversion_errors:	return(L"grpr_conversion_errors");
		case grpr_full_success:			return(L"grpr_full_success");
	}

	return(L"grpr_unknown");
}

const wchar_t *TGrammar::GetProcResultName(TGrammarProcessingResult val)
{
	switch (val)
	{
		case grpr_none:				return(L"Empty");
		case grpr_syntax_errors:		return(L"SyntaxErrors");
		case grpr_conversion_errors:	return(L"ConversErrors");
		case grpr_full_success:			return(L"Success");
	}

	return(L"Unknown");
}

wchar_t *TGrammar::StrDupe(const wchar_t *str)
{
	wchar_t *res = (wchar_t*)malloc((wcslen(str)+1)*sizeof(wchar_t));
	if (res != NULL)
		wcscpy(res, str);
	return(res);
}

wchar_t *TGrammar::StrDupe(TStringPtr &str_ptr)
{
	wchar_t *res = (wchar_t*)malloc((str_ptr.GetLength()+1)*sizeof(wchar_t));
	if (res != NULL)
		str_ptr.CopyToVerifiedBuffer(res);
	return(res);
}

wchar_t *TGrammar::StrDupe(TStrPtrInfo &str_ptr_info)
{
	wchar_t *res = (wchar_t*)malloc((str_ptr_info.m_len+1)*sizeof(wchar_t));
	if (res != NULL)
		str_ptr_info.CopyToVerifiedBuffer(res);
	return(res);
}

bool TGrammar::ProcessGrammarsHier(int &grammars_inx_base, int &igrm, TGrammar **pgrm)
{
	// Process direct nested grammars first.
	for (int ixpct1=0; ixpct1<xpct_conflicts.NumItems(); ++ixpct1)
	{
		TGrammar *ngrm1 = xpct_conflicts[ixpct1].nested_grammar;
		if (ngrm1 != NULL)
		{
			if (grammars_inx_base == igrm)
			{
				// Grammar is found by its index.
				*pgrm = ngrm1;
				return(TRUE);
			}
			else if (ngrm1 == *pgrm)
			{
				// Grammar is found by its pointer.
				igrm = grammars_inx_base;
				return(TRUE);
			}

			// Shift the index.
			grammars_inx_base++;
		}
	}

	// Process the subtrees.
	for (int ixpct2=0; ixpct2<xpct_conflicts.NumItems(); ++ixpct2)
	{
		TGrammar *ngrm2 = xpct_conflicts[ixpct2].nested_grammar;
		if (ngrm2 != NULL && ngrm2->ProcessGrammarsHier(grammars_inx_base, igrm, pgrm) == TRUE)
		{
			// Something is found in this subtree.
			return(TRUE);
		}
	}

	// The passed index or the passed grammar pointer does not belong to the current subtree.
	return(FALSE);
}

void TGrammar::WriteTraceMessage(TGenericConsole *console, const wchar_t *format, ...)
{
	if (console == NULL)
		return;

	wchar_t buffer[1024];
	va_list vargs;

	va_start(vargs, format);
	vswprintf(buffer, 1024, format, vargs);
	va_end(vargs);

	console->HandleTrace(buffer);
}


