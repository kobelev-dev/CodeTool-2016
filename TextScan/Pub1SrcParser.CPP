//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "TextScan/Pub1SrcParser.H"
#include  "Grammars/Gra_Pub1e/Pub1_AppIds.H"

// ------------------------------------------------------------------
//   ================  TPub1SrcParser  =====================
// ------------------------------------------------------------------

void TPub1SrcParser::ReviewParsingAction(WORD &action)
{
	if (action >= actb_bogus_val)
	{
		// This should be the syntax error. Do nothing. This parser is not doing any error recovery.
		assert(action == actb_generic_syntax_error);
		return;
	}
	else if (action >= actb_nested_result)
	{
		// This type of action indicates that nested grammar has returned result that requires support
		// from the callback. Only one nested result is defined in this grammar. Pick up the app id of this
		// nested processing result and check it.
		WORD nested_res_app_id = m_grammar->nested_processing_results[action-actb_nested_result].nest_res_app_id;
		assert(nested_res_app_id == nres_TypeVarName);

		// This nested result can be achieved only when the leftmost object in the parsing stack of the exited
		// grammar is the sym_SpecialQualName. Ensure this assumption.
		TSymbolStateItem *item = GetSavedStackSlotPtr(1);
		assert(item != NULL && GetSavedGrammar()->GetSymbolAppId(item->m_symbol) == sym_SpecialQualName);

		//
		//  (1) Handler should decide what option of the current grammar conflict the parser should take.
		//  Decision is based on the type of assembled name.
		//
		TGrammarConflict *cfct = GetSavedConflict();
		switch (item->m_app_data_int)
		{
			case symt_namesp:
					// Syntax error.
					action = 0xFFFF;
					SetAppNote(L"Name, that stays after the parenthesis, is a namespace name. This is an error.");
					break;

			case symt_type:
					// Reduce.
					action = cfct->GetAction(1);
					SetAppNote(L"Name, that stays after the parenthesis, is the name of type. This means that current context is the beginning of parameters specification.");
					break;

			case symt_variab:
					// Shift.
					action = cfct->GetAction(0);
					SetAppNote(L"Name, that stays after the parenthesis, is the name of variable. This means that current context is the beginning of declarator in parenthesis.");
					break;

			default:
				assert(FALSE);
				break;
		}
	}
	else if (action >= actb_conflict)
	{
		// This grammar has no unexpected conflicts. Ensure this.
		TGrammarConflict &cfct = m_grammar->conflicts[action-actb_conflict];
		assert(cfct.expected_conflict_inx >= 0);

		//
		//  (2) Check for conflicts that need direct resolution. In this grammar 3 conflicts do not have
		//  nested grammars and they expect direct resolution by the callback handler.
		//
		switch (m_grammar->xpct_conflicts[cfct.expected_conflict_inx].xpct_conflict_app_id)
		{
			case xpct_Qualified_OR_MoreQualified:
			case xpct_Qual_OR_MoreQual_1:
			case xpct_Qual_OR_MoreQual_2:
					{
						// All these conflicts can appear only after the "QualifiedName" symbol. Ensure this.
						TSymbolStateItem *item = GetStackSlotPtr(1);
						assert(item != NULL && m_grammar->GetSymbolAppId(item->m_symbol) == sym_QualifiedName);
						assert(item->m_app_data_int != symt_none);

						// Check the type of the qualified name, assembled so far.
						if (item->m_app_data_int == symt_namesp)
						{
							// Type of the name is namespace. Continue assembling the name.
							action = cfct.conflicting_actions[0];
							SetAppNote(L"Rightmost object in the parsing stack is the namespace name. Continue assembling the qualified name.");
						}
						else
						{
							// Type of the name is type or variable. Stop assembling the name.
							action = cfct.conflicting_actions[1];
							SetAppNote(L"Rightmost object in the parsing stack is name of the type or name of the variable. Stop assembling the qualified name.");
						}
					}
					break;
		}
	}
	else if (action >= actb_reduce)
	{
		//
		//  (3) Do additional app layer syntax check.
		//  In this grammar 3 rules have additional application layer constrains.
		//
		switch (m_grammar->rules[action-actb_reduce].rule_app_id)
		{
			case rule_QualName:
					{
						// First name in this rule should be a namespace name.
						if (GetStackSlotPtr(3)->m_app_data_int != symt_namesp)
						{
							action = 0xFFFF;  // Syntax error.
							SetAppNote(L"First name is not the namespace name.");
						}
					}
					break;

			case rule_TypeSpec:
					{
						// Name should be a name of the type.
						if (GetStackSlotPtr(1)->m_app_data_int != symt_type)
						{
							action = 0xFFFF;  // Syntax error.
							SetAppNote(L"Last name is not the name of type.");
						}
					}
					break;

			case rule_NamedCoreDecl:
					{
						// Name should be a name of the variable.
						if (GetStackSlotPtr(1)->m_app_data_int != symt_variab)
						{
							action = 0xFFFF;  // Syntax error.
							SetAppNote(L"Last name is not the variable name.");
						}
					}
					break;
		}
	}
}

TSymbolStateItem *TPub1SrcParser::AllocateSymbolStateItem(WORD irule)
{
	// Allocate or pick up the symbol state structure first.
	TSymbolStateItem *non_term_proto = TGrammarBasedParser::AllocateSymbolStateItem(irule);
	if (non_term_proto == NULL)
		return(NULL);

	// Clear the app layer field and return the object if the terminal symbol is being constructed.
	non_term_proto->m_app_data_int = symt_none;
	if (irule == 0xFFFF)
		return(non_term_proto);

	//
	//  (4) This handler assigns and promotes type of the name when non terminal represents simple or
	//  qualified name. All other non terminals do not have any app value.
	//
	//  Pick up the rightmost symbol from the parser stack and check the rule that is about to be applied.
	//
	TSymbolStateItem *item = GetStackSlotPtr(1);
	assert(item != NULL);

	switch (m_grammar->rules[irule].rule_app_id)
	{
		case rule_SimpleName:
		case rule_RootName:
				{
					// Setup the initial type of the name.
					non_term_proto->m_app_data_int = GetNameType(item);
				}
				break;

		case rule_QualName:
				{
					// Type of the combined name is the type of the last name.
					non_term_proto->m_app_data_int = GetNameType(item);
				}
				break;

		case rule_QualNameEx:
				{
					// Promote namespace/type/variable flag.
					non_term_proto->m_app_data_int = item->m_app_data_int;
				}
				break;
	}

	return(non_term_proto);
}


