//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "TextScan/MidScan.H"

//---------------------------------------------------------------------------
//  ====================  TCondExprHelper  =======================
//---------------------------------------------------------------------------

// Expression calculation/simplification result.
static const wchar_t *g_ConditionalStatusNames1[cexr_num_types] =
{
	L"False for sure",						// cexr_hard_false
	L"True for sure",						// cexr_hard_true
	L"False with doubt",					// cexr_soft_false
	L"True with doubt",						// cexr_soft_true
	L"Unknown",							// cexr_undefined
};

// The source code status.
static const wchar_t *g_ConditionalStatusNames2[cexr_num_types] =
{
	L"Passive (P)",
	L"Active (A)",
	L"Passive with doubt (PD)",
	L"Active with doubt (AD)",
	L"Conditional (C)",
};

// Enum member names.
static const wchar_t *g_ConditionalStatusEnumNames[cexr_num_types] =
{
	L"cexr_hard_false",
	L"cexr_hard_true",
	L"cexr_soft_false",
	L"cexr_soft_true",
	L"cexr_undefined",
};

// Expression simplication status.
static const wchar_t *g_CondExprSmplResultNamesShort[cesr_num_types] =
{
	L"Unsimplifiable",						// cesr_unsimpl
	L"PartiallySimplifed",					// cesr_simplif
	L"Value",								// cesr_value
};

// Expression simplication status.
static const wchar_t *g_CondExprSmplResultNamesLong[cesr_num_types] =
{
	L"Unsimplifiable",						// cesr_unsimpl
	L"Partial simplif",						// cesr_simplif
	L"Value",								// cesr_value
};

// Processing unknown names in defined(x) operator.
static const wchar_t *g_CondExprUnknNamesProcMode[ceum_num_types] =
{
	L"Conditonal",							// ceum_condit,
	L"UndefWithDoubt",					// ceum_undef_doubt,
	L"UndefNoDoubt",						// ceum_undef_nodoubt,
};

// CondExprTreeItem type name short form.
static const wchar_t *g_CondExprTreeItemTypeNamesShort[cecit_num_types] =
{
	L"Value",								// cecit_value
	L"LiteralConst",						// cecit_constant
	L"Decoration",							// cecit_decorated
	L"MacroNameDef",						// cecit_name_def
	L"Variable",							// cecit_variable
	L"VariableDef",						// cecit_variab_def
	L"Operation",							// cecit_operation
};

// CondExprTreeItem type name detailed form.
static const wchar_t *g_CondExprTreeItemTypeNamesLong[cecit_num_types] =
{
	L"Value (result of simplification)",			// cecit_value
	L"Literal constant",						// cecit_constant
	L"Decorated constant",					// cecit_decorated
	L"Macro name in the defined stmt",		// cecit_name_def
	L"Unknown or undefined value",			// cecit_variable
	L"Unknown name in the defined stmt",		// cecit_variab_def
	L"Operation",							// cecit_operation
};

const wchar_t *TCondExprHelper::GetCondStatusName(wchar_t *buff_80_chars, TConditionalStatus val, bool code_status_mode)
{
	if (val < 0 || val >= cexr_num_types)
		swprintf(buff_80_chars, 80, L"Bogus value (%d)", (int)val);
	else wcscpy(buff_80_chars, (code_status_mode == TRUE) ? g_ConditionalStatusNames2[val] : g_ConditionalStatusNames1[val]);

	return(buff_80_chars);
}

const wchar_t *TCondExprHelper::GetCondStatusEnumName(wchar_t *buff_80_chars, TConditionalStatus val)
{
	if (val < 0 || val >= cexr_num_types)
		swprintf(buff_80_chars, 80, L"BogusConditionalStatusValue(%d)", (int)val);
	else wcscpy(buff_80_chars, g_ConditionalStatusEnumNames[val]);

	return(buff_80_chars);
}

const wchar_t *TCondExprHelper::GetSmplResultName(wchar_t *buff_80_chars, TCondExprSmplResult val, bool long_form)
{
	if (long_form == TRUE)
	{
		if (val < 0 || val >= cesr_num_types)
			swprintf(buff_80_chars, 80, L"Bogus value (%d)", (int)val);
		else wcscpy(buff_80_chars, g_CondExprSmplResultNamesLong[val]);
	}
	else
	{
		if (val < 0 || val >= cesr_num_types)
			swprintf(buff_80_chars, 80, L"BogusCondExprSmplResultValue(%d)", (int)val);
		else wcscpy(buff_80_chars, g_CondExprSmplResultNamesShort[val]);
	}

	return(buff_80_chars);
}

const wchar_t *TCondExprHelper::GetUnknNamesProcModeName(wchar_t *buff_80_chars, TCondExprUnknNamesProcMode val)
{
	if (val < 0 || val >= ceum_num_types)
		swprintf(buff_80_chars, 80, L"BogusValue(%d)", (int)val);
	else wcscpy(buff_80_chars, g_CondExprUnknNamesProcMode[val]);

	return(buff_80_chars);
}

const wchar_t *TCondExprHelper::GetCondExprTreeItemTypeName(wchar_t *buff_80_chars, TCondExprTreeItemType val, bool long_form)
{
	if (long_form == TRUE)
	{
		if (val < 0 || val >= cecit_num_types)
			swprintf(buff_80_chars, 80, L"Bogus value (%d)", (int)val);
		else wcscpy(buff_80_chars, g_CondExprTreeItemTypeNamesLong[val]);
	}
	else
	{
		if (val < 0 || val >= cecit_num_types)
			swprintf(buff_80_chars, 80, L"BogusCondExprTreeItemTypeValue(%d)", (int)val);
		else wcscpy(buff_80_chars, g_CondExprTreeItemTypeNamesShort[val]);
	}

	return(buff_80_chars);
}

//---------------------------------------------------------------------------
//  ===================  TCondExprTreeItem  =======================
//---------------------------------------------------------------------------

TCondExprTreeItem::TCondExprTreeItem(TCondExprIntermediateValue computed_value, bool val_doubt)
{
	item_type = cecit_value;
	key_info = NULL;

	value = computed_value;
	on_doubt = val_doubt;
}

TCondExprTreeItem::TCondExprTreeItem(TKeyInfoItem *ki, TLexema &literal_const)
{
	item_type = cecit_constant;
	key_info = ki;

	TLexemaType lext = literal_const.type;
	assert(lext == ltx_number || lext == ltx_charconst);

	value = literal_const.num_value;
	on_doubt = literal_const.on_doubt;
}

TCondExprTreeItem::TCondExprTreeItem(TKeyInfoItem *ki, TLexema &decor_source, TMacroDefinition *mdef, bool defined_oper)
{
	// This ctor should be used for creating operands when the name has a macro.
	assert(mdef != NULL);

	item_type  = (defined_oper == TRUE) ? cecit_name_def : cecit_decorated;
	key_info = ki;

	if (defined_oper == TRUE)
	{
		value = (TCondExprIntermediateValue)mdef->def_status;
	}
	else
	{
		// The passed macro should be defined and it should be a single lexema macro.
		assert(GET_DEFINED_STT(mdef->def_status) == TRUE);
		assert(mdef->scst_body_lex.type == ltx_number || mdef->scst_body_lex.type == ltx_charconst);
		value = mdef->scst_body_lex.num_value;
	}

	on_doubt = decor_source.on_doubt || GET_DOUBT_STT(mdef->def_status);

	decor_cn = mdef->hdr.cn;
	decor_state = mdef->def_status;
	decor_nature = mdef->def_origin_type;
	decor_simplif = TRUE;
}

TCondExprTreeItem::TCondExprTreeItem(TKeyInfoItem *ki, TLexemasList &src, bool defined_oper)
{
	item_type = (defined_oper == TRUE) ? cecit_variab_def : cecit_variable;
	key_info = ki;

	value = 0;			// The value is not used. In the cecit_variab_def case the value will be derived from the smpl permissions.
	on_doubt = TRUE;

	// Fill in these data fields with the default values. It may happen that the name is known but
	// is currently undefined. Using such name as an operand results in a "variable" code item.
	// Nevertheless the info about the latest #undef statement for this name will be filled later.
	decor_cn = 0;
	decor_state = mds_hard_undef;
	decor_nature = mdo_definition;
	decor_simplif = FALSE;

	AddSourceLexemasList(src, FALSE, alm_primary);
}

TCondExprTreeItem::TCondExprTreeItem(TLexema &oper_lexema, TCondExprArithmPrt oper_prt)
{
	//
	// At the beginning the list of operands is always empty. It will be filled later using the AddOperand() method.
	//
	item_type = cecit_operation;
	key_info = NULL;

	on_doubt = oper_lexema.on_doubt;
	operation_code = (TOperatorToken)oper_lexema.num_value;
	operation_prt  = oper_prt;
}

void TCondExprTreeItem::AddOperand(TCondExprTreeItem *operand, bool add_to_the_end)
{
	assert(item_type == cecit_operation);

	if (add_to_the_end == TRUE)
		operands.AppendItem(operand);
	else operands.PrependItem(operand);
}

bool TCondExprTreeItem::AddSourceLexema(TLexema &source_lex, TAddLexemasMode inc_mode)
{
	TLexemaListItem *item = new TLexemaListItem(source_lex);
	if (item == NULL)
		return(FALSE);

	item->primary_src = (inc_mode == alm_primary) ? TRUE : FALSE;
	source.AppendItem(item);
	on_doubt |= source_lex.on_doubt;
	return(TRUE);
}

bool TCondExprTreeItem::AddSourceLexemasList(TLexemasList &src, bool clone_list, TAddLexemasMode inc_mode)
{
	if (clone_list == TRUE)
	{
		TLexemasList copy_list;
		bool new_doubt = FALSE;

		// Iterate the list of the source lexemas.
		for (TListIter<TLexemaListItem> iter(src); iter; ++iter)
		{
			TLexemaListItem *item = &iter.CurrItem();
			TLexemaListItem *cp_item = new TLexemaListItem(item->lex);

			if (cp_item == NULL)
			{
				copy_list.DeleteAll();
				return(FALSE);
			}

			if (inc_mode == alm_keep_origin)
				cp_item->primary_src = item->primary_src;
			else cp_item->primary_src = (inc_mode == alm_primary) ? TRUE : FALSE;

			copy_list.AppendItem(cp_item);
			new_doubt |= cp_item->lex.on_doubt;
		}

		source.AppendList(copy_list);
		on_doubt |= new_doubt;
	}
	else
	{
		// Pick up the doubt from the list items and set up the primary source flag.
		for (TListIter<TLexemaListItem> iter(src); iter; ++iter)
		{
			TLexemaListItem *item = &iter.CurrItem();

			if (item->lex.on_doubt == TRUE)
				on_doubt = TRUE;
			if (inc_mode != alm_keep_origin)
				item->primary_src = (inc_mode == alm_primary) ? TRUE : FALSE;
		}

		// Simply take the list. This operation cannot fail.
		source.AppendList(src);
	}

	return(TRUE);
}

TCondExprTreeItem *TCondExprTreeItem::Clone()
{
	// Create item with a temporary type.
	TCondExprTreeItem *item = new TCondExprTreeItem(value, on_doubt);
	if (item == NULL)
		return(NULL);

	// Put correct type and key info.
	item->item_type = item_type;
	item->key_info = key_info;

	// Copy the type specific fields.
	switch (item_type)
	{
		case cecit_decorated:
		case cecit_name_def:
		case cecit_variable:
		case cecit_variab_def:
				{
					item->decor_cn = decor_cn;
					item->decor_state = decor_state;
					item->decor_nature = decor_nature;
					item->decor_simplif = decor_simplif;
				}
				break;
		case cecit_operation:
				{
					item->operation_code = operation_code;
					item->operation_prt = operation_prt;

					// Iterate operands of this operation.
					for (TListIter<TCondExprTreeItem> iter(operands); iter; ++iter)
					{
						TCondExprTreeItem *cp_opr = iter.CurrItem().Clone();
						if (cp_opr == NULL)
						{
							delete item;
							return(NULL);
						}

						item->AddOperand(cp_opr);
					}
				}
				break;
	}

	// Duplicate info about the source lexemas.
	if (item->AddSourceLexemasList(source, TRUE, alm_keep_origin) == FALSE)
	{
		delete item;
		return(NULL);
	}

	// Return full success.
	return(item);
}

bool TCondExprTreeItem::IsSimplifiable(TCondExprSimplificationProps &smpl_props)
{
	switch (item_type)
	{
		case cecit_constant:
				{
					if (smpl_props.smpl_constants == TRUE)
						return(TRUE);
				}
				break;

		case cecit_decorated:
		case cecit_name_def:
				{
					// Check the permissions on undecoration.
					if (decor_simplif == TRUE && smpl_props.smpl_simplif == TRUE)
						return(TRUE);
				}
				break;

		case cecit_variab_def:
				{
					if (smpl_props.unkn_names != ceum_condit)
						return(TRUE);
				}
				break;

		case cecit_operation:
				{
					// Operations are simplifiable all the time.
					return(TRUE);
				}
	}

	// Rest of the cases are always unsimplifiable.
	return(FALSE);
}

TCondExprSmplResult TCondExprTreeItem::SimplifyExpression(TCondExprSimplificationProps &smpl_props)
{
	// Process simple cases that cannot have children items.
	if (item_type != cecit_operation)
	{
		// This can be a value, constant, decorated value or a variable. In this case there is no any tree or recursion.
		switch (item_type)
		{
			case cecit_value:
					return(cesr_value);

			case cecit_constant:
					if (smpl_props.smpl_constants == TRUE)
					{
						// Convert literal const code item into the value code item.
						PatchToValue();
						return(cesr_value);
					}
					return(cesr_unsimpl);

			case cecit_decorated:
					// Check if the permissions allows undecoration.
					if (decor_simplif == TRUE && smpl_props.smpl_simplif == TRUE)
					{
						// At this moment the definition state should be "defined". Ensure this.
						assert(GET_DEFINED_STT(decor_state) == TRUE);

						// Convert decoration into value.
						PatchToValue();
						return(cesr_value);
					}
					return(cesr_unsimpl);
		}

		// Either the variable or the data is corrupted.
		return(cesr_unsimpl);
	}

	//
	// This code item is some sort of operation.
	//
	bool smpl_happened = FALSE;

	// Simplify all operands except for operands of the defined(x) operation. Defined(x) operation is a special
	// case because its operand cannot be an expression plus it should be undecorated in a special way.
	if (operation_code != opr_defined)
	{
		for (TListIter<TCondExprTreeItem> iter(operands); iter; ++iter)
		{
			TCondExprTreeItem *item = &iter.CurrItem();
			if (item->item_type == cecit_operation)
			{
				if (item->SimplifyExpression(smpl_props) != cesr_unsimpl)
					smpl_happened = TRUE;
			}
			else if (item->item_type == cecit_decorated || item->item_type == cecit_constant)
			{
				// Simplification flag is not set here to TRUE because although undecoration is modification
				// of the expression, it is at the same time not a simplification of the expression.
				item->SimplifyExpression(smpl_props);
			}
		}
	}

	// Check if is it possible to merge <operands_of_our_operands> with <our_operands> or not.
	bool merging_happened = FALSE;
	TCondExprTreeItem *curr_our_opr = (TCondExprTreeItem*)operands.GetFirst();
	while (curr_our_opr != NULL)
	{
		TCondExprTreeItem *item = curr_our_opr;

		// Shifting of the loop variable should take place at the beginning of the loop because the list
		// of operands is modified.
		curr_our_opr = (TCondExprTreeItem*)operands.GetNext(curr_our_opr);

		if (item->item_type == cecit_operation && item->operation_code == operation_code)
		{
			// This operand is an operation of the same type as the operation of the current instance.
			// It can make sense to try to merge them.
			switch (operation_code)
			{
				case opr_plus:
				case opr_bitand: case opr_bitor: case opr_bitxor:
				case opr_logand: case opr_logor:
						{
							TCondExprTreeItem *opr_of_opr = (TCondExprTreeItem*)item->operands.GetFirst();
							while (opr_of_opr != NULL)
							{
								TCondExprTreeItem *opr = opr_of_opr;
								opr_of_opr = (TCondExprTreeItem*)item->operands.GetNext(opr_of_opr);
								item->operands.RemoveItem(opr);
								operands.InsertBefore(opr, item);
							}

							operands.RemoveItem(item);
							AddSourceLexemasList(item->source, FALSE, alm_secondary);
							delete item;

							merging_happened = TRUE;
						}
						break;
			}
		}
	}

	// Check for the constant operands on this level and for other possible simplifications.
	switch (operation_code)
	{
		case opr_defined:
				{
					smpl_happened |= CalcDefinedOperation(smpl_props);
				}
				break;

		case opr_bang: case opr_tilda:
		case opr_unplus: case opr_unminus:
				{
					smpl_happened |= CalcOneOperandOperation();

					// Check the item type again because the calculation may have changed it.
					if (item_type == cecit_operation && operation_code == opr_bang)
					{
						// Check for the special case of the !(!(x)) expression.
						TCondExprTreeItem *opr1 = (TCondExprTreeItem*)operands.GetFirst();
						if (opr1->item_type == cecit_operation && opr1->operation_code == opr_bang)
						{
							TCondExprTreeItem *opr2 = (TCondExprTreeItem*)opr1->operands.GetFirst();
							operands.RemoveItem(opr1);
							opr1->operands.RemoveItem(opr2);
							SetContents(opr2, on_doubt);
							AddSourceLexemasList(opr1->source, FALSE, alm_secondary);

							delete opr1;
							delete opr2;
							smpl_happened = TRUE;
						}
					}
				}
				break;

		case opr_minus: case opr_div: case opr_rmnd:
		case opr_bitlsh: case opr_bitrsh:
		case opr_lt: case opr_le: case opr_gt:
		case opr_ge: case opr_eq: case opr_ne:
				{
					smpl_happened |= CalcTwoOperandsOperation();
				}
				break;

		case opr_question:
				{
					smpl_happened |= CalcConditionalOperation();
				}
				break;

		case opr_comma:
				{
					smpl_happened |= CalcCommaOperation();
				}
				break;

		case opr_plus:
		case opr_bitand: case opr_bitor: case opr_bitxor:
		case opr_logand: case opr_logor:
				{
					smpl_happened |= CalcMultiOperandsOperation(TRUE, merging_happened);
				}
				break;

		case opr_mul:
				{
					smpl_happened |= CalcMultiOperandsOperation(FALSE, merging_happened);
				}
				break;
	}

	// Check results of calculation.
	if (item_type == cecit_value)
		return(cesr_value);
	if (smpl_happened == TRUE)
		return(cesr_simplif);

	return(cesr_unsimpl);
}

bool TCondExprTreeItem::CreateTextRepresentation(TTextBuffer256 &result, TCondExprArithmPrt current_prt)
{
	bool success = TRUE;

	switch (item_type)
	{
		case cecit_value:
				{
					// Simply append the value.
					wchar_t buffer[80];
					swprintf(buffer, 80, L"%I64d", value);
					result.Append(buffer);
				}
				break;

		case cecit_constant:
		case cecit_decorated:
		case cecit_name_def:
		case cecit_variable:
		case cecit_variab_def:
				{
					// Append all primary source lexemas that constitute the code item.
					for (TListIter<TLexemaListItem> iter(source); iter; ++iter)
					{
						if (iter.CurrItem().primary_src == TRUE)
							iter.CurrItem().lex.AppendTo(result);
					}
				}
				break;

		case cecit_operation:
				{
					if (operation_prt < ceprt_unary && operation_prt <= current_prt)
					{
						// It is necessary to enclose the code of the operation into the parenthesis.
						result.Append(L'(');
						success &= CreateTextRepresentation(result, ceprt_lowest);
						result.Append(L')');
					}
					else
					{
						TLexema opr;
						opr.SetKeyword(ltkn_arithm, operation_code);
						switch (operation_code)
						{
							case opr_bang:	case opr_tilda:	case opr_defined:
							case opr_unplus:	case opr_unminus:
									{
										success &= MakeTextReprForOneOperand(opr, operation_prt, operands, result);
									}
									break;

							case opr_minus:	case opr_div:		case opr_rmnd:
							case opr_bitlsh:	case opr_bitrsh:
							case opr_lt:		case opr_le:		case opr_gt:
							case opr_ge:		case opr_eq:		case opr_ne:
									{
										success &= MakeTextReprForTwoOperands(opr, operation_prt, operands, result);
									}
									break;

							case opr_question:
									{
										success &= MakeTextReprForThreeOperands(opr, operation_prt, operands, result);
									}
									break;

							case opr_plus:	case opr_mul:	case opr_comma:
							case opr_bitand:	case opr_bitor:	case opr_bitxor:
							case opr_logand:	case opr_logor:
									{
										success &= MakeTextReprForManyOperands(opr, operation_prt, operands, result);
									}
									break;

							default:
								{
									// Incorrect token value.
									result.Append(L"<=wrong_operation:");
									opr.AppendTo(result);
									result.Append(L":=>");
									success = FALSE;
								}
								break;
						}
					}
				}
				break;
	}

	return(success == TRUE && result.GetXpndError() == FALSE);
}

int TCondExprTreeItem::CountNodes()
{
	// All non operation items always consist of one node.
	if (item_type != cecit_operation)
		return(1);

	// The number of nodes in the operation is the number of its operands plus 1.
	int count = 1;
	for (TListIter<TCondExprTreeItem> iter(operands); iter; ++iter)
	{
		count += iter.CurrItem().CountNodes();
	}

	return(count);
}

bool TCondExprTreeItem::CalcDefinedOperation(TCondExprSimplificationProps &smpl_props)
{
	if (operands.NumItems() != 1)
		return(FALSE);

	TCondExprTreeItem *operand = (TCondExprTreeItem*)operands.GetFirst();
	assert(operand != NULL);

	if (operand->item_type == cecit_variab_def)
	{
		// There is no info about this name at all.
		if (smpl_props.unkn_names == ceum_condit)
		{
			// Do not simplify this case.
			return(FALSE);
		}

		// This case should be reported as the "name is not defined".
		AddSourceLexemasList(operand->source, FALSE, alm_secondary);
		SetValue(0, (smpl_props.unkn_names == ceum_undef_doubt) ? TRUE : FALSE);
		return(TRUE);
	}

	// This should be some decorated variable.
	assert(operand->item_type == cecit_name_def);
	if (operand->decor_simplif == FALSE || smpl_props.smpl_simplif == FALSE)
	{
		// Permissions are not allowing this simplification.
		return(FALSE);
	}

	// Get value info out of the current state of definition.
	AddSourceLexemasList(operand->source, FALSE, alm_secondary);
	SetValue(GET_DEFINED_STT(operand->value), (GET_DOUBT_STT(operand->value) == TRUE) ? TRUE : FALSE);
	return(TRUE);
}

bool TCondExprTreeItem::CalcOneOperandOperation()
{
	if (operands.NumItems() != 1)
		return(FALSE);

	TCondExprTreeItem *operand = (TCondExprTreeItem*)operands.GetFirst();
	if (operand->IsNumericValue() == FALSE)
		return(FALSE);

	TCondExprIntermediateValue value = operand->value;
	bool ext_doubt = operand->on_doubt;

	switch (operation_code)
	{
		case opr_bang:
				value =  (value != 0) ? 0 : 1;
				break;

		case opr_tilda:
				value = ~value;
				break;

		case opr_unplus:
				// There is nothing to do here.
				break;

		case opr_unminus:
				value = -value;
				break;
	}

	// The operation was resolved into the value.
	AddSourceLexemasList(operand->source, FALSE, alm_secondary);
	SetValue(value, ext_doubt);
	return(TRUE);
}

bool TCondExprTreeItem::CalcTwoOperandsOperation()
{
	if (operands.NumItems() != 2)
		return(FALSE);

	TCondExprTreeItem *operand1 = (TCondExprTreeItem*)operands.GetFirst();
	TCondExprTreeItem *operand2 = (TCondExprTreeItem*)operands.GetNext(operand1);

	if (operand1->IsNumericValue() == FALSE)
	{
		if (operand2->IsNumericValue() == FALSE)
		{
			// When both operands are not numbers, the simplification is not possible.
			return(FALSE);
		}

		// The first operand is not a numeric, the second one is numeric.
		TCondExprIntermediateValue val2 = operand2->value;
		bool doubt2 = operand2->on_doubt;

		switch (operation_code)
		{
			case opr_minus:
			case opr_bitlsh:
			case opr_bitrsh:
					{
						if (val2 == 0)
							break;
						return(FALSE);
					}

			case opr_div:
					{
						if (val2 == 1)
							break;

						if (val2 == -1)
						{
							// This can be converted into an unary minus.
							operation_code = opr_unminus;
							operation_prt = ceprt_unary;
							operands.RemoveItem(operand2);
							AddSourceLexemasList(operand2->source, FALSE, alm_secondary);
							on_doubt |= doubt2;
							delete operand2;
							return(TRUE);
						}
					}
					break;

			case opr_rmnd:
					{
						if (val2 == 1)
							break;

						if (val2 < 0)
						{
							// Testing shows that result is all the time 0 regardless of what was the first operand.
							AddSourceLexemasList(operand1->source, FALSE, alm_secondary);
							AddSourceLexemasList(operand2->source, FALSE, alm_secondary);
							SetValue(0, operand1->on_doubt || doubt2);
							return(TRUE);
						}
						return(FALSE);
					}

			default:
				return(FALSE);
		}

		// Simplify this expression to the first operand.
		operands.RemoveItem(operand1);
		operands.RemoveItem(operand2);
		SetContents(operand1, on_doubt || operand1->on_doubt || doubt2);
		AddSourceLexemasList(operand2->source, FALSE, alm_secondary);
		delete operand1;
		delete operand2;

		// The expression was simpified.
		return(TRUE);
	}

	if (operand2->IsNumericValue() == FALSE)
	{
		// The first operand is numeric, the second one is not.
		if (operation_code == opr_minus && operand1->value == 0)
		{
			// This can be converted into unary minus.
			operation_code = opr_unminus;
			operation_prt = ceprt_unary;
			operands.RemoveItem(operand1);
			AddSourceLexemasList(operand1->source, FALSE, alm_secondary);
			on_doubt |= operand1->on_doubt;
			delete operand1;
			return(TRUE);
		}

		return(FALSE);
	}

	// Both operands are numeric constants.
	TCondExprIntermediateValue value1 = operand1->value;
	TCondExprIntermediateValue value2 = operand2->value;
	bool res_doubt = operand1->on_doubt || operand2->on_doubt;
	TCondExprIntermediateValue res_value = 0;

	switch (operation_code)
	{
		case opr_minus:
				res_value = value1 - value2;
				break;

		case opr_div:
				if (value2 == 0)
					return(FALSE);
				res_value =  value1 / value2;
				break;

		case opr_rmnd:
				if (value2 == 0)
					return(FALSE);
				res_value =  value1 % value2;
				break;

		case opr_bitlsh:
				// In case if the value2 is negative do the same as the current compiler.
				res_value =  value1 << value2;
				break;

		case opr_bitrsh:
				// In case if the value2 is negative do the same as the current compiler.
				res_value =  value1 >> value2;
				break;

		case opr_lt:
				res_value = (value1 < value2) ? 1 : 0;
				break;

		case opr_le:
				res_value = (value1 <= value2) ? 1 : 0;
				break;

		case opr_gt:
				res_value = (value1 > value2) ? 1 : 0;
				break;

		case opr_ge:
				res_value = (value1 >= value2) ? 1 : 0;
				break;

		case opr_eq:
				res_value = (value1 == value2) ? 1 : 0;
				break;

		case opr_ne:
				res_value = (value1 != value2) ? 1 : 0;
				break;

		default:
			assert(FALSE);
			return(FALSE);
	}

	// The operation was resolved into a value.
	AddSourceLexemasList(operand1->source, FALSE, alm_secondary);
	AddSourceLexemasList(operand2->source, FALSE, alm_secondary);
	SetValue(res_value, res_doubt);
	return(TRUE);
}

bool TCondExprTreeItem::CalcMultiOperandsOperation(bool const_align_style, bool merging_happened)
{
	assert(operands.NumItems() >= 2);

	bool constants_detected = FALSE;
	bool variables_detected = FALSE;
	bool calculation_happened = FALSE;

	// Values will be set only if constants will be detected.
	TCondExprIntermediateValue integral_const_value;
	bool integral_const_doubt;

	// First loop over all operands. It will detect whether at least anything can be calculated and whether
	// there are constants that have non trivial values.
	for (TListIter<TCondExprTreeItem> iter(operands); iter; ++iter)
	{
		TCondExprTreeItem &opr = iter.CurrItem();
		if (opr.IsNumericValue() == FALSE)
		{
			variables_detected = TRUE;
			continue;
		}

		TCondExprIntermediateValue operand_value = opr.value;
		if (operation_code == opr_logand || operation_code == opr_logor)
		{
			operand_value = (operand_value != 0) ? 1 : 0;
		}

		if (constants_detected == FALSE)
		{
			integral_const_value = operand_value;
			integral_const_doubt = opr.on_doubt;
			constants_detected = TRUE;
			continue;
		}

		switch (operation_code)
		{
			case opr_plus:
					{
						integral_const_value += operand_value;
						integral_const_doubt |= opr.on_doubt;
					}
					break;

			case opr_mul:
			case opr_bitand:
			case opr_logand:
					{
						if (integral_const_value == 0)
						{
							// Integral result cannot change.
							if (operand_value == 0)
							{
								// The doubt can be reduced.
								integral_const_doubt &= opr.on_doubt;
							}
							else
							{
								// The doubt is not changing.
							}
						}
						else
						{
							if (operand_value == 0)
							{
								// The doubt should be replaced.
								integral_const_doubt = opr.on_doubt;
								integral_const_value = 0;
							}
							else
							{
								// The doubt should be increased.
								integral_const_doubt |= opr.on_doubt;

								// Modify the integral result.
								if (operation_code == opr_mul)
									integral_const_value *= operand_value;
								else integral_const_value &= operand_value;
							}
						}
					}
					break;

			case opr_bitor:
			case opr_logor:
					{
						if (integral_const_value == -1)
						{
							// Integral result cannot change.
							if (operand_value == -1)
							{
								// The doubt can be reduced.
								integral_const_doubt &= opr.on_doubt;
							}
							else
							{
								// The doubt is not changing.
							}
						}
						else
						{
							if (operand_value == -1)
							{
								// The doubt should be replaced.
								integral_const_doubt = opr.on_doubt;
								integral_const_value = -1;
							}
							else
							{
								// The doubt should be increased.
								integral_const_doubt |= opr.on_doubt;

								// Modify the integral result.
								integral_const_value |= operand_value;
							}
						}
					}
					break;

			case opr_bitxor:
					{
						integral_const_value ^= opr.value;
						integral_const_doubt |= opr.on_doubt;
					}
					break;
		}

		// There are at least 2 constants in this expression.
		calculation_happened = TRUE;
	}

	if (variables_detected == FALSE)
	{
		// The whole expression consists of constants only.
		RemoveConstants(this);
		SetValue(integral_const_value, integral_const_doubt);
		return(TRUE);
	}

	if (constants_detected == FALSE)
	{
		// There were no constants among the operands, there is nothing to simplify.
		return(FALSE);
	}

	//
	// Expression is some mixture of constants and variables.
	//

	// Check if the expression became trivial or not.
	switch (operation_code)
	{
		case opr_mul:
		case opr_bitand:
		case opr_logand:
				{
					if (integral_const_value == 0)
					{
						CollectOperandSources(NULL);
						SetValue(0, integral_const_doubt);
						return(TRUE);
					}
				}
				break;

		case opr_bitor:
				{
					if (integral_const_value == -1)
					{
						CollectOperandSources(NULL);
						SetValue(-1, integral_const_doubt);
						return(TRUE);
					}
				}
				break;

		case opr_logor:
				{
					if (integral_const_value == 1)
					{
						CollectOperandSources(NULL);
						SetValue(1, integral_const_doubt);
						return(TRUE);
					}
				}
				break;
	}

	if (calculation_happened == FALSE && merging_happened == FALSE)
	{
		// There was just one constant, keep it in its original state.
		return(FALSE);
	}

	// Check whether is it possible to remove the constants completelty or not.
	bool remove_completely = FALSE;
	switch (operation_code)
	{
		case opr_plus:
		case opr_bitor:
		case opr_bitxor:
		case opr_logor:
				{
					if (integral_const_value == 0)
						remove_completely = TRUE;
				}
				break;

		case opr_mul:
		case opr_logand:
				{
					if (integral_const_value == 1)
						remove_completely = TRUE;
				}
				break;

		case opr_bitand:
				{
					if (integral_const_value == -1)
						remove_completely = TRUE;
				}
				break;
	}

	if (remove_completely == TRUE)
	{
		RemoveConstants(this);
		if (operands.NumItems() == 1)
		{
			// Just one variable is left, cancel the operation.
			TCondExprTreeItem *operand = (TCondExprTreeItem*)operands.GetFirst();
			operands.RemoveItem(operand);
			SetContents(operand, on_doubt || integral_const_doubt);
			delete operand;
		}

		return(TRUE);
	}

	// Add one integral constant.
	TCondExprIntermediateValue new_const_val = integral_const_value;
	TCondExprTreeItem *new_const = new TCondExprTreeItem(new_const_val, integral_const_doubt);
	if (new_const == NULL)
		return(FALSE);

	RemoveConstants(new_const);
	AddOperand(new_const, const_align_style);
	return(TRUE);
}

bool TCondExprTreeItem::CalcConditionalOperation()
{
	if (operands.NumItems() != 3)
		return(FALSE);

	TCondExprTreeItem *operand1 = (TCondExprTreeItem*)operands.GetFirst();
	if (operand1->IsNumericValue() == FALSE)
		return(FALSE);

	bool cond_doubt = operand1->on_doubt;

	TCondExprTreeItem *operand2 = (TCondExprTreeItem*)operands.GetNext(operand1);
	TCondExprTreeItem *operand3 = (TCondExprTreeItem*)operands.GetNext(operand2);

	TCondExprIntermediateValue cond_value = operand1->value;
	TCondExprTreeItem *result_operand = (cond_value != 0) ? operand2 : operand3;

	operands.RemoveItem(result_operand);
	bool old_doubt = on_doubt;
	CollectOperandSources(result_operand);
	SetContents(result_operand, old_doubt || cond_doubt);
	delete result_operand;

	return(TRUE);
}

bool TCondExprTreeItem::CalcCommaOperation()
{
	// Surprise, but the comma operation can be simplified all the times regardless of the nature of its operands.
	TCondExprTreeItem *operand = (TCondExprTreeItem*)operands.GetLast();
	if (operand == NULL)
		return(FALSE);

	operands.RemoveItem(operand);
	bool old_doubt = on_doubt;
	CollectOperandSources(operand);
	SetContents(operand, old_doubt);
	delete operand;
	return(TRUE);
}

bool TCondExprTreeItem::MakeTextReprForOneOperand(TLexema oper, TCondExprArithmPrt opr_prt, TList &operands, TTextBuffer256 &result)
{
	int num_operands = operands.NumItems();
	if (num_operands != 1)
	{
		ReportBogusNumberOfOperands(oper, num_operands, result);
		return(FALSE);
	}

	oper.AppendTo(result);
	if (((short)oper.num_value) == opr_defined)
		result.Append(L'(');

	bool success = ((TCondExprTreeItem*)operands.GetFirst())->CreateTextRepresentation(result, opr_prt);
	if (((short)oper.num_value) == opr_defined)
		result.Append(L')');

	return(success);
}

bool TCondExprTreeItem::MakeTextReprForTwoOperands(TLexema oper, TCondExprArithmPrt opr_prt, TList &operands, TTextBuffer256 &result)
{
	int num_operands = operands.NumItems();
	if (num_operands != 2)
	{
		ReportBogusNumberOfOperands(oper, num_operands, result);
		return(FALSE);
	}

	TCondExprTreeItem *first_operand = (TCondExprTreeItem*)operands.GetFirst();
	bool success = ((TCondExprTreeItem*)first_operand)->CreateTextRepresentation(result, opr_prt);
	result.Append(L' ');
	oper.AppendTo(result);
	result.Append(L' ');
	success &= ((TCondExprTreeItem*)operands.GetNext(first_operand))->CreateTextRepresentation(result, opr_prt);

	return(success);
}

bool TCondExprTreeItem::MakeTextReprForThreeOperands(TLexema oper, TCondExprArithmPrt opr_prt, TList &operands, TTextBuffer256 &result)
{
	int num_operands = operands.NumItems();
	if (num_operands != 3)
	{
		ReportBogusNumberOfOperands(oper, num_operands, result);
		return(FALSE);
	}

	if (((short)oper.num_value) != opr_question)
	{
		result.Append(L"<=wrong_operation(");
		oper.AppendTo(result);
		result.Append(L")_for_3_operands_code_item=>");
		return(FALSE);
	}

	bool success = TRUE;
	TCondExprTreeItem *first_operand  = (TCondExprTreeItem*)operands.GetFirst();
	TCondExprTreeItem *second_operand = (TCondExprTreeItem*)operands.GetNext(first_operand);
	TCondExprTreeItem *third_operand  = (TCondExprTreeItem*)operands.GetNext(second_operand);

	success &= first_operand->CreateTextRepresentation(result, opr_prt);
	result.Append(L" ? ");
	success &= second_operand->CreateTextRepresentation(result, opr_prt);
	result.Append(L" : ");
	success &= third_operand->CreateTextRepresentation(result, opr_prt);

	return(success);
}

bool TCondExprTreeItem::MakeTextReprForManyOperands(TLexema oper, TCondExprArithmPrt opr_prt, TList &operands, TTextBuffer256 &result)
{
	int num_operands = operands.NumItems();
	if (num_operands < 2)
	{
		ReportBogusNumberOfOperands(oper, num_operands, result);
		return(FALSE);
	}

	bool success = TRUE;
	TCondExprTreeItem *operand = (TCondExprTreeItem*)operands.GetFirst();
	success &= operand->CreateTextRepresentation(result, opr_prt);
	for(;;)
	{
		operand = (TCondExprTreeItem*)operands.GetNext(operand);
		if (operand == NULL)
			break;

		if (((short)oper.num_value) != opr_comma)
			result.Append(L' ');

		oper.AppendTo(result);
		result.Append(L' ');
		success &= operand->CreateTextRepresentation(result, opr_prt);
	}

	return(success);
}

void TCondExprTreeItem::ReportBogusNumberOfOperands(TLexema operation, int num_operands, TTextBuffer256 &result)
{
	wchar_t buff[80];
	swprintf(buff, 80, L"<=wrong_number_of_operands(%d)_for_", num_operands);
	result.Append(buff);
	operation.AppendTo(result);
	result.Append(L"_=>");
}

void TCondExprTreeItem::SetValue(TCondExprIntermediateValue num_val, bool ext_doubt)
{
	//
	//  Method converts current inst into a value code item and keeps its source list unchanged.
	//
	operands.DeleteAll();

	PatchToValue();
	value = num_val;
	on_doubt |= ext_doubt;
}

void TCondExprTreeItem::RemoveConstants(TCondExprTreeItem *value_item)
{
	// This is a protected method, it can be called only from inside the simplification code.
	if (item_type != cecit_operation)
		return;

	TCondExprTreeItem *opr = (TCondExprTreeItem*)operands.GetFirst();
	while (opr != NULL)
	{
		TCondExprTreeItem *opr_cur = opr;
		opr = (TCondExprTreeItem*)operands.GetNext(opr);
		if (opr_cur->item_type == cecit_value)
		{
			operands.RemoveItem(opr_cur);
			value_item->AddSourceLexemasList(opr_cur->source, FALSE, alm_secondary);
			value_item->on_doubt |= opr_cur->on_doubt;
			delete opr_cur;
		}
	}
}

void TCondExprTreeItem::CollectOperandSources(TCondExprTreeItem *except_item)
{
	if (item_type != cecit_operation)
		return;

	TCondExprTreeItem *opr = (TCondExprTreeItem*)operands.GetFirst();
	while (opr != NULL)
	{
		TCondExprTreeItem *opr_cur = opr;
		opr = (TCondExprTreeItem*)operands.GetNext(opr);
		if (opr_cur != except_item)
		{
			opr_cur->CollectOperandSources(NULL);
			AddSourceLexemasList(opr_cur->source, FALSE, alm_secondary);
		}
	}
}

void TCondExprTreeItem::SetContents(TCondExprTreeItem *operand, bool extra_doubt)
{
	//
	//  Method takes contents from the 'operand' parameter and places it into local data fields.
	//  This is equivalent to conversion of the current instance into the bypassed operand.
	//

	item_type		= operand->item_type;
	key_info			= operand->key_info;
	value			= operand->value;
	on_doubt			= operand->on_doubt || extra_doubt;
	decor_cn			= operand->decor_cn;
	decor_state		= operand->decor_state;
	decor_nature		= operand->decor_nature;
	decor_simplif		= operand->decor_simplif;
	operation_code	= operand->operation_code;
	operation_prt		= operand->operation_prt;
	operands.TakeDataFrom(operand->operands);

	// Use special procedure for the sources list. Clear the "primary_src" flag in all existing sources.
	for (TListIter<TLexemaListItem> iter(source); iter; ++iter)
	{
		iter.CurrItem().primary_src = FALSE;
	}

	// Append the new sources keeping their original flag value.
	AddSourceLexemasList(operand->source, FALSE, alm_keep_origin);
}

//-------------------------------------------------------------------------------------
//  ====================  TCondExpressionScanner  ==========================
//-------------------------------------------------------------------------------------

ID TCondExpressionScanner::ScanExpression(ID cond_stmt_cn, int cond_expr_level, TCondExprScanningMode mode,
								TCondExprScanningDetails &res_details, PrEventHeader &expression_area, TLexema &lex_after_expr,
								TCondExprTreeItem **ppcode)
{
	assert(ppcode != NULL);

	// Prefill the out parameters.
	res_details = cescd_none;
	expression_area.Clear();
	lex_after_expr.Clear();
	*ppcode = NULL;

	// Setup the class data members of the scanner.
	scan_mode = mode;
	stmt_cn = cond_stmt_cn;
	stmt_src_level = src.GetCurrLexSrcNesting();
	stmt_cond_level = cond_expr_level;
	stt = cess_left_brace;
	pdetails = &res_details;
	malloc_problem = FALSE;
	stk1_len = stk2_len = 0;

	// Clear the scanning related fields.
	expr_area.Clear();
	cmtd_lex_area.Clear();
	lex.Clear();
	lex_src_level = -1;
	lex_rescan_req = FALSE;
	latest_error = 0;

	// Prepare the scanner.
	src.soft_macro_expansion_mode = TRUE;
	src.macro_call_happened = FALSE;

	// Scan the expression.
	for(;;)
	{
		// Pick up the next lexema and process it.
		switch (ExprScan())
		{
			case ltx_empty:
					{
						ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_empty_lexema);
						continue;
					}

			case ltx_comment:
					{
						// Comment lexemas should not come out the ExprScan() method.
						assert(FALSE);
						break;
					}

			case ltx_number:
			case ltx_charconst:
					{
						// The literal values can be present in the expression.
						if (lex.on_doubt == TRUE)
							ReportDetail(cescd_operand_on_doubt);

						if (ProcessLiteralConstLexema() == TRUE)
							continue;
					}
					break;

			case ltx_floating_point:
					{
						ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_foating_point_lexema);
						continue;
					}

			case ltx_string:
					{
						ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_string_lexema);
						continue;
					}

			case ltx_keyword:
					{
						TOperatorToken num_val = (TOperatorToken)lex.num_value;
						if (num_val == opr_defined || (num_val >= opr_lpar && num_val <= opr_colon) || (num_val == opr_comma && (scan_mode & cescm_process_commas) != 0))
						{
							// This is a valid token for this expression.
							if (lex.on_doubt == TRUE)
								ReportDetail(cescd_operation_on_doubt);

							if (ProcessTokenLexema() == TRUE)
								continue;
						}
						else
						{
							return(ProcessNonExpressionLexema(expression_area, ppcode, lex_after_expr));
						}
					}
					break;

			case ltx_name:
					{
						// This is either an unknown name or this is a soft macro non expansion.
						if (lex.on_doubt == TRUE)
							ReportDetail(cescd_operand_on_doubt);

						if (ProcessNameLexema() == TRUE)
							continue;
					}
					break;

			case ltx_eol:
					{
						// When an EOL is returned by the ExprScan(), it cannot be ignored.
						return(ProcessNonExpressionLexema(expression_area, ppcode, lex_after_expr));
					}

			case ltx_eof:
					{
						// Eof is always an alien lexema for the conditional expressions.
						return(ProcessNonExpressionLexema(expression_area, ppcode, lex_after_expr));
					}

			case ltx_error:
					{
						// This is a raw scanning error and it is already reported.
						continue;
					}

			default:
				{
					// The type of lexema is bad. Most likely this is some data corruption.
					assert(FALSE);
					break;
				}
		}

		break;
	}

	// Parsing came to a serious error or this is an OOM condition.
	assert(latest_error != 0 || malloc_problem == TRUE);
	return(TerminateWithError(expression_area));
}

TCondExprArithmPrt TCondExpressionScanner::GetOperationPriority(TOperatorToken keyword_id)
{
	TCondExprArithmPrt prt = ceprt_highest;

	switch (keyword_id)
	{
		case opr_bang:
		case opr_tilda:
		case opr_defined:
		case opr_unplus:
		case opr_unminus:	prt = ceprt_unary;		break;
		case opr_mul:
		case opr_div:
		case opr_rmnd:		prt = ceprt_mul;		break;
		case opr_plus:
		case opr_minus:		prt = ceprt_sum;		break;
		case opr_bitlsh:
		case opr_bitrsh:		prt = ceprt_shift;		break;
		case opr_lt:
		case opr_le:
		case opr_gt:
		case opr_ge:			prt = ceprt_compar;	break;
		case opr_eq:
		case opr_ne:			prt = ceprt_eqne;		break;
		case opr_bitand:		prt = ceprt_bitand;		break;
		case opr_bitxor:		prt = ceprt_bitxor;		break;
		case opr_bitor:		prt = ceprt_bitor;		break;
		case opr_logand:		prt = ceprt_logand;	break;
		case opr_logor:		prt = ceprt_logor;		break;
		case opr_question:
		case opr_colon:		prt = ceprt_condit;		break;
		case opr_comma:		prt = ceprt_comma;	break;
		case opr_lpar:		prt = ceprt_lowest;		break;
	}

	return(prt);
}

bool TCondExpressionScanner::ProcessLiteralConstLexema()
{
	// Create and prepare the code representation for this literal const operand.
	TKeyInfoItem *key = src.GetIndexingKeyFromLexEx(lex, latest_error, malloc_problem);
	if (key == NULL)
	{
		// The error is already reported by the scanner.
		return(FALSE);
	}

	// Check for the overflow errors but continue building the operand even if they are present.
	if (lex.type == ltx_number && lex.number_type == lnt_u64bit && ((__int64)lex.num_value) < 0)
	{
		ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_64bit_negnum);
	}
	else if (lex.type == ltx_number && lex.number_type >= lnt_s128bit)
	{
		ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_128bit_number);
	}
	else if (lex.type == ltx_charconst && ((__int64)lex.num_value) < 0)
	{
		ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_64bit_charconst);
	}

	TCondExprTreeItem *operand = new TCondExprTreeItem(key, lex);
	if (operand == NULL || operand->AddSourceLexema(lex, TCondExprTreeItem::alm_primary) == FALSE)
	{
		if (operand != NULL)
			delete operand;

		malloc_problem = TRUE;
		return(FALSE);
	}

	// Creation of the code succeeded. Send the structural callback about this literal const parameter.
	key->BumpUsage(mlku_cprm_const);
	PrCondParamUseEvent info;
	PrepareCondParamUseEvent(info, lex);
	info.param = operand;
	src.cbk_handler->CondParamUseNotification(&info);
	src.PostProsessMidLevCbkCall(info.hdr);

	// Try to push this literal const into the stack of operands.
	return(ProcessOperand(operand));
}

bool TCondExpressionScanner::ProcessTokenLexema()
{
	bool res = TRUE;

	TOperatorToken keyword_id = (TOperatorToken)lex.num_value;
	switch (keyword_id)
	{
		case opr_lpar:
				{
					if (stt == cess_operand)
					{
						ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_operation_missing);
						return(FALSE);
					}

					res = PushOperation(lex);
					stt = cess_left_brace;
				}
				break;

		case opr_bang:
		case opr_tilda:
				{
					if (stt == cess_operand)
					{
						ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_unary_op_error);
						return(FALSE);
					}

					res = PushOperation(lex);
					stt = cess_pref_unary;
				}
				break;

		case opr_defined:
				{
					// This is a special case for scanning the parameter of the deifned(x) operation.
					// State of the scanner will be changed from inside.
					res = ProcessDefinedOperation();
				}
				break;

		case opr_plus:
		case opr_minus:
				{
					if (stt == cess_left_brace || stt == cess_pref_unary)
					{
						// In this state of the parser the plus and minus are treated as unary operaions.
						lex.num_value = (keyword_id == opr_plus) ? opr_unplus : opr_unminus;
						res = PushOperation(lex);
						stt = cess_pref_unary;
						break;
					}
				}

				//
				// There should be no break here. Fall threw into the binary operations.
				//

		case opr_mul:		case opr_div:			case opr_rmnd:
		case opr_bitlsh:		case opr_bitrsh:
		case opr_bitand:		case opr_bitor:		case opr_bitxor:
		case opr_lt:			case opr_le:			case opr_gt:
		case opr_ge:			case opr_eq:			case opr_ne:
		case opr_logand:		case opr_logor:
		case opr_question:	case opr_colon:		case opr_comma:
				{
					if (stt == cess_left_brace)
					{
						if (stk2_len != 0)
							ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_lpar_no_operand1);
						else ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_no_oprnd_op);
						return(FALSE);
					}
					else if (stt != cess_operand)
					{
						ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_no_oprnd_bop);
						return(FALSE);
					}

					// This is a binary or ternary operation after the operand.
					res = ResolveStack(GetOperationPriority(keyword_id));
					if (res == FALSE)
						break;

					res = PushOperation(lex);
					stt = cess_binary;
				}
				break;

		case opr_rpar:
				{
					// Closing parenthesis.
					if (stt != cess_operand)
					{
						ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_no_oprnd_rpar);
						return(FALSE);
					}

					// Try to resolve the existing items on the stack.
					res = ResolveStack(ceprt_lowest);
					if (res == FALSE)
						break;

					// Check, if the opening parenthesis is on the top of the stack or not.
					if (stk2_len == 0 || stk2[stk2_len-1]->operation_code != opr_lpar)
					{
						// No, it is not there.
						ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_rpars_than_lpars);
						return(FALSE);
					}

					// Take the opening parenthesis out of the stack.
					stk2_len--;
					stt = cess_operand;
				}
				break;

		default:
			{
				// Unknown operation.
				ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_unkn_operation);
				return(FALSE);
			}
	}

	return(res);
}

bool TCondExpressionScanner::ProcessNameLexema()
{
	// Prepare the structural notification header.
	PrCondParamUseEvent info;
	PrepareCondParamUseEvent(info, lex);

	// Retrieve the key info for this name.
	TKeyInfoItem *key = src.GetIndexingKeyFromLexEx(lex, latest_error, malloc_problem);
	if (key == NULL)
	{
		// The error is already reported by the scanner.
		return(FALSE);
	}

	TMacroDefinition *macro = src.macrodefs_list.LookupMacro(lex.str_value);
	if (macro != NULL)
	{
		// Current name corresponds to some macro and the macro with this name was not called.
		if (macro->def_origin_type == mdo_predefined && macro->redefinitions_history.IsEmpty() == FALSE)
		{
			ReportDetail(cescd_redefined_predef);
			src.AddWarning(&lex.origin, TMidLevScanner::mscn_err_cexpr_redef_predef_used);
		}

		if (GET_DEFINED_STT(macro->def_status) == TRUE)
		{
			//
			//  (1 of 3) Lexema is the name of some defined macro that was not called.
			//  This can happen only in the case of the soft macro non expansion.
			//
			assert(macro->scst_body_lex.type != ltx_empty);
			TCondExprTreeItem *operand = new TCondExprTreeItem(key, lex, macro, FALSE);
			if (operand == NULL || operand->AddSourceLexema(lex, TCondExprTreeItem::alm_primary) == FALSE)
			{
				if (operand != NULL)
					delete operand;

				// The error about the malloc problem will be reported in a single place later.
				malloc_problem = TRUE;
				return(FALSE);
			}

			// Send the structural callback about this decoration.
			key->BumpUsage(mlku_cprm_value);
			info.param = operand;
			src.cbk_handler->CondParamUseNotification(&info);
			src.PostProsessMidLevCbkCall(info.hdr);

			// Push this decorated value into the stack.
			return(ProcessOperand(operand));
		}
		else
		{
			//
			//  (2 of 3) This name is known but it is not defined in the moment.
			//
			ReportDetail(cescd_undef_name_value);
			if ((scan_mode & cescm_allow_unkn_names) == 0)
			{
				ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_known_undef);
			}
		}
	}
	else
	{
		//
		// (2 of 3) There is no info about this name.
		//
		ReportDetail(cescd_unkn_name_value);
		if ((scan_mode & cescm_allow_unkn_names) == 0)
		{
			ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_unknown_name);
		}
	}

	// Current lexema is "variable". Create a list of lexemas that constitute its source.
	TLexemasList var_body;
	TLexemaListItem *name_item = new TLexemaListItem(lex);
	if (name_item != NULL)
	{
		var_body.AppendItem(name_item);
	}
	else
	{
		malloc_problem = TRUE;
		return(FALSE);
	}

	// Check if this name has parameters after it or not. Scan the next lexema with locking the current
	// lex src nesting level to prevent exiting from the current lex source. This is necessary to report
	// the CondParamUse event from the right scanning area.
	PrEventHeader name_lex_origin = lex.origin;
	int prev_src_exit_mark = src.SetLexSrcExitLimit(src.GetCurrLexSrcNesting());
	ExprScan();
	if (lex.IsKeyword(opr_lpar) == TRUE)
	{
		// This name has parameters behind it.
		ReportDetail(cescd_nontrivial_macro);

		// Build the list of lexems that constitute this lost of params.
		if (ScanParamsOfAnUnknownMacro(var_body) == FALSE)
		{
			var_body.DeleteAll();
			src.SetLexSrcExitLimit(prev_src_exit_mark);
			return(FALSE);
		}
	}
	else if (lex.IsEof() == FALSE)
	{
		// Mark the current lexema as unprocessed.
		lex_rescan_req = TRUE;
	}

	// Restore the level of the exit lock back.
	src.SetLexSrcExitLimit(prev_src_exit_mark);

	TCondExprTreeItem *operand = new TCondExprTreeItem(key, var_body, FALSE);
	if (operand == NULL)
	{
		var_body.DeleteAll();
		malloc_problem = TRUE;
		return(FALSE);
	}

	if (macro != NULL)
	{
		// The macro is existing but it is currently undefined. It still makes sense  to fill in the info
		// about this macro into the code item.
		operand->decor_cn = macro->hdr.cn;
		operand->decor_state = macro->def_status;
		operand->decor_nature = macro->def_origin_type;
		operand->decor_simplif = TRUE;
	}

	// Send the structural callback about this variable.
	key->BumpUsage(mlku_cprm_value);
	info.param = operand;
	src.cbk_handler->CondParamUseNotification(&info);
	src.PostProsessMidLevCbkCall(info.hdr);

	// Push this unevaluatable value into the stack.
	return(ProcessOperand(operand, &name_lex_origin));
}

bool TCondExpressionScanner::ProcessDefinedOperation()
{
	// Save the operation lexema.
	TLexema operation_lexema = lex;

	// Get the parameter name first.
	int  cnt_lpars = 0;
	bool err_flag = FALSE;
	for(;;)
	{
		switch (ExprScanMacroName())
		{
			case ltx_keyword:
					if (((short)lex.num_value) == opr_lpar)
					{
						cnt_lpars++;
						continue;
					}

					err_flag = TRUE;
					break;

			case ltx_name:
					// The name was finaly found.
					break;

			default:
				err_flag = TRUE;
				break;
		}

		break;
	}

	if (err_flag == TRUE)
	{
		ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_defnd_params);
		return(FALSE);
	}

	// Some name was retrieved from the source file.
	TLexema param_name = lex;
	if (lex.on_doubt == TRUE)
		ReportDetail(cescd_operand_on_doubt);

	// If there were some number of opening parenthesis, then it is necessary to pick up the closing ones.
	while (cnt_lpars > 0)
	{
		ExprScanMacroName();
		if (lex.IsKeyword(opr_rpar) == TRUE)
		{
			cnt_lpars--;
			continue;
		}

		break;
	}

	if (cnt_lpars > 0)
	{
		// The required number of closing parenthesis is not available.
		ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_defnd_no_rpar);
		return(FALSE);
	}

	// The scanning part went fine. Build the code item.
	TKeyInfoItem *key = src.GetIndexingKeyFromLexEx(param_name, latest_error, malloc_problem);
	if (key == NULL)
	{
		// The error is already reported by the scanner.
		return(FALSE);
	}

	TCondExprTreeItem *operand = src.CreateNameTreeItem(stmt_cn, stmt_src_level, stmt_cond_level, key, param_name, *pdetails);
	if (operand == NULL)
	{
		malloc_problem = TRUE;
		return(FALSE);
	}
	else if (PushOperation(operation_lexema) == FALSE)
	{
		delete operand;
		return(FALSE);
	}
	else if (PushOperand(operand) == FALSE)
	{
		delete operand;
		return(FALSE);
	}

	stt = cess_operand;
	if (ResolveStack(ceprt_mul) == FALSE)
		return(FALSE);

	// All steps succeeded.
	return(TRUE);
}

bool TCondExpressionScanner::ScanParamsOfAnUnknownMacro(TLexemasList &dest_list)
{
	// When this method is called, an opening parenthesis is already taken from the lexemas source.
	// And this lexema should be currently stored in the data field "lex" of the scanner object.
	assert(lex.IsKeyword(opr_lpar) == TRUE);

	// Add the scanned parenthesis to the source list.
	TLexemaListItem *par_item = new TLexemaListItem(lex);
	if (par_item == NULL)
	{
		malloc_problem = TRUE;
		return(FALSE);
	}

	dest_list.AppendItem(par_item);
	int cnt_br = 0;
	bool prms_list_complete = FALSE;

	// Loop on getting lexemas from the source.
	while (prms_list_complete == FALSE)
	{
		bool ignore_lex_flag = FALSE;
		switch (ExprScan(&ignore_lex_flag))
		{
			case ltx_comment:
			case ltx_number:
			case ltx_floating_point:
			case ltx_charconst:
			case ltx_string:
					break;

			case ltx_keyword:
					{
						switch ((short)lex.num_value)
						{
							case opr_lpar:
									{
										cnt_br++;
									}
									break;

							case opr_rpar:
									{
										if (cnt_br > 0)
										{
											--cnt_br;
										}
										else
										{
											prms_list_complete = TRUE;
										}
									}
									break;
						}
					}
					break;

			case ltx_name:
					break;

			case ltx_eol:
					{
						if (ignore_lex_flag == FALSE)
						{
							// An end of expression is reached.
							ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_unkn_mparams);
							return(FALSE);
						}
					}
					break;

			case ltx_eof:
					{
						// An end of expression is reached.
						ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_unkn_mparams);
						return(FALSE);
					}

			case ltx_error:
					break;

			default:
				{
					// The type of lexema is bogus.
					assert(FALSE);
					return(FALSE);
				}
		}

		// Control came to this point. This means that lexema should be added to the source of an unknown macro.
		if (lex_src_level == stmt_src_level)
		{
			TLexemaListItem *lexema_item = new TLexemaListItem(lex);
			if (lexema_item != NULL)
			{
				dest_list.AppendItem(lexema_item);
			}
			else
			{
				malloc_problem = TRUE;
				return(FALSE);
			}
		}
	}

	// This is either a right parenthesis or an eof.
	return(TRUE);
}

bool TCondExpressionScanner::ProcessOperand(TCondExprTreeItem *operand, PrEventHeader *spec_error_ctx)
{
	if (stt == cess_operand)
	{
		if (operand->item_type == cecit_constant && operand->value < 0)
		{
			// This is special case where the minus sign was included into the lexema by the raw scanner.
			// Simulate situation like a plus sign is present in the source.
			if (ResolveStack(ceprt_sum) == FALSE)
			{
				delete operand;
				return(FALSE);
			}

			// Create new lexema out of the current number lexema and push it.
			TLexema syntetic_operation = lex;
			syntetic_operation.type = ltx_keyword;
			syntetic_operation.keyword_type = ltkn_arithm;
			syntetic_operation.num_value = opr_plus;
			if (PushOperation(syntetic_operation) == FALSE)
			{
				delete operand;
				return(FALSE);
			}
		}
		else
		{
			// The situation is bad. There are 2 operands in a row.
			ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_no_operation, spec_error_ctx);
			delete operand;
			return(FALSE);
		}
	}

	TCondExpressionScannerState prev_stt = stt;

	if (PushOperand(operand) == FALSE)
	{
		delete operand;
		return(FALSE);
	}

	stt = cess_operand;

	if (prev_stt == cess_pref_unary && ResolveStack(ceprt_mul) == FALSE)
	{
		// Unary operations were present, but there were problems while resolving them into the code.
		return(FALSE);
	}

	return(TRUE);
}

ID TCondExpressionScanner::ProcessNonExpressionLexema(PrEventHeader &expression_area, TCondExprTreeItem **ppcode, TLexema &lex_after_expr)
{
	//
	//  This method is called when some lexema that cannot be part of the expression is retrieved.
	//  Lexema will be put back to the lexemas source and the current stack of the scanner should be
	//  completely resolveable.
	//

	// Check if any macro call was taking place or not.
	if (src.macro_call_happened == TRUE)
		ReportDetail(cescd_nontrivial_macro);
	src.soft_macro_expansion_mode = FALSE;

	// Give the alien lexema back to the caller.
	assert(lex.type != ltx_empty);
	lex_after_expr = lex;

	// Assemble and give out the expression area.
	AssembleExprArea();
	expression_area = expr_area;

	if (stk1_len == 0 && stk2_len == 0)
	{
		// Nothing was found until now. This looks to be an empty expression.
		if ((scan_mode & cescm_allow_empty_expr) == 0)
			ReportError(TRUE, TMidLevScanner::mscn_err_cexpr_expr_missing);

		return(TerminateProcessingExpression());
	}

	if (stt != cess_operand)
	{
		switch (stt)
		{
			case cess_left_brace:
					ReportError(TRUE, TMidLevScanner::mscn_err_cexpr_lpar_no_operand2);
					break;

			case cess_pref_unary:
					ReportError(TRUE, TMidLevScanner::mscn_err_cexpr_unop_no_operand);
					break;

			case cess_binary:
					ReportError(TRUE, TMidLevScanner::mscn_err_cexpr_biop_no_operand);
					break;

			default:
				ReportError(TRUE, TMidLevScanner::mscn_err_cexpr_no_operand);
				break;
		}

		return(TerminateProcessingExpression());
	}

	if (ResolveStack(ceprt_lowest) == FALSE)
	{
		// An error message should come from inside.
		return(TerminateProcessingExpression());
	}

	if (stk1_len != 1 || stk2_len != 0)
	{
		ReportError(TRUE, TMidLevScanner::mscn_err_cexpr_expr_uncomplete);
		return(TerminateProcessingExpression());
	}

	if (malloc_problem == TRUE)
	{
		ReportError(TRUE, TMidLevScanner::mscn_err_cexpr_out_of_memory);
		return(TerminateProcessingExpression());
	}

	// Everything was resolved succesfully. Valid expression code is available.
	*ppcode = stk1[0];
	stk1_len = 0;

	// Errors that were not preventing the code creation may still exist.
	return(TerminateProcessingExpression());
}

ID TCondExpressionScanner::TerminateProcessingExpression()
{
	assert(malloc_problem == FALSE);
	if (latest_error != 0)
	{
		// Some syntax error was detected.
		ReportDetail(cescd_syntax_error);
	}

	return(latest_error);
}

ID TCondExpressionScanner::TerminateWithError(PrEventHeader &expression_area)
{
	// Terminate the scanning in case of syntax error or in case of the malloc problem.
	// Check if any macro call was taking place.
	if (src.macro_call_happened == TRUE)
		ReportDetail(cescd_nontrivial_macro);
	src.soft_macro_expansion_mode = FALSE;

	// Assemble and give out the expression area.
	AssembleExprArea();
	expression_area = expr_area;

	if (latest_error != 0)
	{
		// Some syntax error was detected.
		ReportDetail(cescd_syntax_error);
	}

	ID prev_latest_error = latest_error;
	if (malloc_problem == TRUE)
	{
		ReportDetail(cescd_malloc_problem);
		ReportError(TRUE, TMidLevScanner::mscn_err_cexpr_out_of_memory);
	}

	// There should be at least one error reported.
	assert(latest_error != 0);
	return((prev_latest_error != 0) ? prev_latest_error : latest_error);
}

bool TCondExpressionScanner::PushOperation(TLexema &oper_lexema)
{
	if (stk2_len >= MAX_COND_EXPR_STACK_LEN)
	{
		ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_stack_overflow1);
		return(FALSE);
	}

	// Create new code item for this operation.
	TCondExprArithmPrt prt = GetOperationPriority((TOperatorToken)oper_lexema.num_value);
	TCondExprTreeItem *operation = new TCondExprTreeItem(oper_lexema, prt);
	if (operation == NULL || operation->AddSourceLexema(oper_lexema, TCondExprTreeItem::alm_primary) == FALSE)
	{
		if (operation != NULL)
			delete operation;

		malloc_problem = TRUE;
		return(FALSE);
	}

	// There is enough room in the stack.
	stk2[stk2_len++] = operation;
	return(TRUE);
}

bool TCondExpressionScanner::PushOperand(TCondExprTreeItem *operand)
{
	if (stk1_len >= MAX_COND_EXPR_STACK_LEN)
	{
		ReportError(FALSE, TMidLevScanner::mscn_err_cexpr_stack_overflow2);
		return(FALSE);
	}

	// There is enough room in the stack.
	stk1[stk1_len++] = operand;
	return(TRUE);
}

bool TCondExpressionScanner::ResolveStack(TCondExprArithmPrt resolve_up_to)
{
	// This method sould be called only in the cess_operand state.
	assert(stt == cess_operand);
	if (stk2_len == 0)
		return(TRUE);

	while (stk2_len > 0 && stk2[stk2_len-1]->operation_prt > resolve_up_to)
	{
		// Pick up the operation from the bottom of the stack.
		TOperatorToken oper = stk2[stk2_len-1]->operation_code;
		TCondExprArithmPrt prt  = stk2[stk2_len-1]->operation_prt;

		if (oper == opr_bang || oper == opr_tilda || oper == opr_defined || oper ==  opr_unplus || oper == opr_unminus)
		{
			// All these operations requre just one operand. Also they have associativity from right to left.
			if (stk1_len < 1)
			{
				ReportError(TRUE, TMidLevScanner::mscn_err_cexpr_resv_unopr);
				return(FALSE);
			}

			TCondExprTreeItem *dest_op = stk2[stk2_len-1];
			dest_op->AddOperand(stk1[stk1_len-1]);
			stk1[stk1_len-1] = dest_op;
			stk2_len--;
			continue;
		}

		// Check for the conditional operation. The associativity is from right to left.
		if (oper == opr_question)
		{
			ReportError(TRUE, TMidLevScanner::mscn_err_cexpr_resv_condopr1);
			return(FALSE);
		}

		if (oper == opr_colon)
		{
			int inx_end = stk2_len-1;
			int inx_beg = inx_end-1;
			while (inx_beg > 0 && stk2[inx_beg]->operation_code == opr_colon)
			{
				inx_end--; inx_beg--;
			}

			if (inx_beg < 0 || stk2[inx_beg]->operation_code != opr_question || stk1_len < (stk2_len-inx_beg+1))
			{
				ReportError(TRUE, TMidLevScanner::mscn_err_cexpr_resv_condopr2);
				return(FALSE);
			}

			// Everything is fine with this conditional operation.
			AssembleMultiOperandOperation(inx_beg, inx_end);
			continue;
		}

		// Look for the beginning of the block of operations with the same priority.
		int inx_beg = stk2_len-1;
		while (inx_beg > 0)
		{
			if (stk2[inx_beg-1]->operation_prt != prt)
				break;
			inx_beg--;
		}

		// All these operations on the stack require exactly 2 operands each.
		if (stk1_len < (stk2_len-inx_beg)+1)
		{
			ReportError(TRUE, TMidLevScanner::mscn_err_cexpr_resv_biop);
			return(FALSE);
		}

		int inx_end = inx_beg;
		switch (oper)
		{
			case opr_div:
			case opr_rmnd:
			case opr_minus:
			case opr_bitlsh:
			case opr_bitrsh:
			case opr_lt:
			case opr_le:
			case opr_gt:
			case opr_ge:
			case opr_eq:
			case opr_ne:
					{
						// These operations are implemented as pure binary.
						AssembleMultiOperandOperation(inx_beg, inx_end);
					}
					break;

			case opr_plus:
			case opr_mul:
			case opr_bitand:
			case opr_bitxor:
			case opr_bitor:
			case opr_logand:
			case opr_logor:
			case opr_comma:
					{
						// These operations can take more than 2 operands. Find out what part of the stack
						// is filled with the same operation.
						while (inx_end+1 < stk2_len)
						{
							if (stk2[inx_end+1]->operation_code != oper)
								break;
							inx_end++;
						}

						AssembleMultiOperandOperation(inx_beg, inx_end);
					}
					break;

			default:
				{
					// When everything is fine, control should never come here.
					ReportError(TRUE, TMidLevScanner::mscn_err_cexpr_resv_unkn_op);
					return(FALSE);
				}
		}
	}

	// Maybe something was resolved, in any case no error was detected.
	return(TRUE);
}

void TCondExpressionScanner::AssembleMultiOperandOperation(int inx_beg, int inx_end)
{
	int offs1 = stk2_len-(inx_beg-1);		// Position of the first operand is: stkX_len-offs1;
	int shift = (inx_end-inx_beg)+1;		// THe number of operations to take.

	// Get operands from the operands stack and put them into the operation. This loop will take (shift+1) operands.
	// Put this operation back into the stack of operands. Since one operand is placed back to the stack of operands,
	// it looses only (shift) operands.
	TCondExprTreeItem *operation = stk2[inx_beg];
	for (int inx1 = 0; inx1 <= shift; ++inx1)
		operation->AddOperand(stk1[stk1_len-offs1+inx1]);
	stk1[stk1_len-offs1] = operation;

	// Delete the extra operation objects.
	for (int inx2 = inx_beg+1; inx2 <= inx_end; ++inx2)
	{
		operation->AddSourceLexemasList(stk2[inx2]->source, FALSE, TCondExprTreeItem::alm_primary);
		delete stk2[inx2];
	}

	// Shift the tails of the stacks in case when assembling happens not exactly at the end of the stack.
	int beg1 = stk1_len-offs1+1;
	int beg2 = stk2_len-offs1+1;
	for (int inx3 = 0; inx3 < stk2_len-(inx_end+1); ++inx3)
	{
		stk1[beg1+inx3] = stk1[beg1+shift+inx3];
		stk2[beg2+inx3] = stk2[beg2+shift+inx3];
	}

	// Reduce the lengths of the stacks.
	stk1_len -= shift;
	stk2_len -= shift;
}

// \/\/\/\/\/\/  Local scanner methods  \/\/\/\/\/\/\/\/

void TCondExpressionScanner::PrepareCondParamUseEvent(PrCondParamUseEvent &info, TLexema &lex)
{
	info.hdr.Setup(&lex.origin, src.GetNextCN());
	info.cond_stmt_cn = stmt_cn;
	info.lex_src_nest = stmt_src_level;
	info.cond_pars_nest = stmt_cond_level;
	info.code_stt = src.cond_pars_status;
	info.param = NULL;
}

TLexemaType TCondExpressionScanner::ExprScan(bool *ignore_lex_flag)
{
	if (ignore_lex_flag != NULL)
		*ignore_lex_flag = FALSE;

	bool first_lex = (lex_src_level == -1) ? TRUE : FALSE;
	if (lex_rescan_req == FALSE)
	{
		// The data field lex does not contain any lexema. Pick un the new lexema.
		for(;;)
		{
			if (first_lex == TRUE)
			{
				if (src.GetCurrLexSrcNesting() == stmt_src_level)
				{
					// Fill in the expr area with the current scanning position.
					src.GetCurrScanPos(expr_area);
				}
			}
			else
			{
				if (lex_src_level == stmt_src_level)
				{
					// Save the origin of the lexema that belongs to the expression.
					cmtd_lex_area = lex.origin;
				}
			}

			// Read the next lexema from the source.
			src.MidScan(lex);
			lex_src_level = src.GetCurrLexSrcNesting();

			// Check for some of the lexemas that should be ignored.
			if (lex.type == ltx_comment)
			{
				// Report comment into the scanning details and continue.
				ReportDetail(cescd_comment_in_expr);
				if (ignore_lex_flag != NULL)
				{
					*ignore_lex_flag = TRUE;
					break;
				}

				continue;
			}
			else if (lex.type == ltx_eol && src.CanFilterOut(lex) == TRUE)
			{
				// The EOLs inside the expression are ok only for certain types of expressions.
				// Plus eols can be present inside macros in any type of expressions.
				if (ignore_lex_flag != NULL)
				{
					*ignore_lex_flag = TRUE;
					break;
				}

				continue;
			}

			if (lex.type == ltx_error)
			{
				// Report a raw scanning error here. Use a trick that all error messages from
				// the C/C++ raw scanner are always zero terminated.
				latest_error = src.AddErrorWarning(&lex.origin, 0, lex.error_class, (int)lex.num_value, lex.str_value.m_body);
			}

			// Give this lexema out.
			break;
		}
	}

	if (first_lex == TRUE && src.GetCurrLexSrcNesting() == stmt_src_level)
	{
		// This is the first lexema and it comes from the right layer of sources.
		expr_area = lex.origin;
	}

	lex_rescan_req = FALSE;
	return(lex.type);
}

TLexemaType TCondExpressionScanner::ExprScanMacroName()
{
	assert(lex_rescan_req == FALSE);

	if (lex_src_level == stmt_src_level)
	{
		// Save the origin of the lexema that belongs to the expression.
		cmtd_lex_area = lex.origin;
	}

	// Read the next lexema from the source.
	src.GetPrStmtLoLex(lex);
	lex_src_level = src.GetCurrLexSrcNesting();
	return(lex.type);
}

void TCondExpressionScanner::AssembleExprArea()
{
	// Lexemas scanning should happen at least once.
	assert(expr_area.src_area.file_info != NULL);

	// Extend the expression area up to and including the last lexema.
	if (lex_src_level == stmt_src_level && lex_rescan_req == FALSE)
	{
		// Use the latest lexema.
		expr_area.ExtendPrHeader(lex.origin);
	}
	else if (cmtd_lex_area.src_area.file_info != NULL)
	{
		// Extend to the end of the commited lexema.
		expr_area.ExtendPrHeader(cmtd_lex_area);
	}

	// Try to add the call area on the initial level in case if expression ended inside the macro call.
	assert(stmt_src_level > 0);
	PrEventHeader &mcall = src.src_stack[stmt_src_level-1].ltst_call_area;
	if (mcall.source_ref != 0 && mcall.src_area.area_beg >= expr_area.src_area.area_beg)
	{
		// The call area can be used for extending the expression area.
		expr_area.ExtendPrHeader(mcall);
	}
}

void TCondExpressionScanner::ReportError(bool whole_expr, int err_code, PrEventHeader *spec_error_ctx)
{
	if (whole_expr == TRUE)
	{
		AssembleExprArea();
		latest_error = src.AddError(&expr_area, err_code);
	}
	else if (spec_error_ctx != NULL)
	{
		latest_error = src.AddError(spec_error_ctx, err_code);
	}
	else
	{
		latest_error = src.AddError(&lex.origin, err_code);
	}
}


