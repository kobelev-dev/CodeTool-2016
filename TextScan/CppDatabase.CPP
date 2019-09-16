//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   In memory storage for C/C++ objects.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/FormatNumber.H"
#include  "TextScan/CppDatabase.H"

//---------------------------------------------------------------------------
//  =====================  TCppItemBase  ========================
//---------------------------------------------------------------------------

TCppItemBase::~TCppItemBase()
{
	// Release unsubmitted parsing events if any.
	for (int inx=0; inx<m_cib_ctv_refs.NumItems(); ++inx)
	{
		if ((m_cib_ctv_refs[inx].ctv_ref_type & clink_unsubm_flag) != 0)
		{
			assert(m_cib_ctv_refs[inx].ctv_unsubm_info != NULL);
			m_cib_ctv_refs[inx].ctv_unsubm_info->RecycleEventInfo();
		}
	}
}

void TCppItemBase::AddListOfChildren(TList &items_to_add, int items_destination)
{
	// Simply add all elements of the passed list as children one by one.
	while (items_to_add.IsEmpty() == FALSE)
	{
		TCppItemBase *item = (TCppItemBase*)items_to_add.GetFirst();
		items_to_add.RemoveItem(item);
		item->m_cib_parent = NULL;
		AddChildItem(item, items_destination);
	}
}

TCppLabelDefn *TCppItemBase::FindNamedLabelHier(ID key_id)
{
	// Check the current instance.
	if (ItemType() == cxx_label)
	{
		TCppLabelDefn *label = (TCppLabelDefn*)this;
		if (label->DefnKeyId() == key_id)
			return(label);
	}

	// Iterate and check direct children of the current instance if any.
	for (TCppDirectChildrenIter iter(this); iter; ++iter)
	{
		TCppLabelDefn *label_item = iter.CurrItem().FindNamedLabelHier(key_id);
		if (label_item != NULL)
			return(label_item);
	}

	// Current subtree (including its root) does not contain the requested label.
	return(NULL);
}

TCppCaseLabelDefn *TCppItemBase::FindCaseLabelHier(TCppCaseLabelDefn *case_label)
{
	if (case_label->m_default == FALSE && case_label->m_const_value == FALSE)
	{
		// This function finds only default labels and those labels that have valid value field.
		return(NULL);
	}

	if (ItemType() == cxx_case_label)
	{
		// Current item is a case label.
		TCppCaseLabelDefn *label = (TCppCaseLabelDefn*)this;
		if (label->m_default == TRUE && case_label->m_default == TRUE)
		{
			// Both labels are default labels.
			return(label);
		}
		else if (label->m_default == FALSE && case_label->m_default == FALSE)
		{
			// Both labels are not default.
			if (label->m_const_value == TRUE)
			{
				// Compare the values.
				if (label->m_value == case_label->m_value)
					return(label);
			}
		}
	}
	else if (ItemType() != cxx_switch_stmt)
	{
		// Iterate and check direct children of the current object.
		for (TCppDirectChildrenIter iter(this); iter; ++iter)
		{
			TCppCaseLabelDefn *chld_item = iter.CurrItem().FindCaseLabelHier(case_label);
			if (chld_item != NULL)
				return(chld_item);
		}
	}

	// Current subtree (including its root) does not contain the requested case label.
	return(NULL);
}

bool TCppItemBase::IsTemplateParam(TCppItemBase *item)
{
	if (item->ItemType() == cxx_template_type_param || item->ItemType() == cxx_template_template_param)
		return(TRUE);

	if (item->ItemType() == cxx_data_field)
	{
		TCppDataFieldDefn *data_field = (TCppDataFieldDefn*)item;
		if (data_field->m_strg_class == strgc_template_param)
			return(TRUE);
	}

	// All other objects cannot be parameters of the template.
	return(FALSE);
}

bool TCppItemBase::IsTemplateDefinitionArgsList(TList *params_list, TList *args_list)
{
	//
	// This method returns TRUE when the passed template args list is a straightforward duplication
	// of the template params from the template header, like this:
	//
	//	   template<class x1, int x2> class T1<x1, x2>;
	//
	// In all other cases, the return value is FALSE.
	//

	// Check for trivial cases. Valid template definition should have non empty list of parameters
	// and non empty list of arguments.
	if (params_list == NULL || params_list->IsEmpty() == TRUE)
		return(FALSE);
	else if (args_list == NULL || args_list->IsEmpty() == TRUE)
		return(FALSE);

	// Both lists are non NULL and they are both not empty. Compare elements of these lists.
	TCppDefnBase *curr_param = GetFirstTemplateParameter(params_list);
	TCppItemBase *curr_arg = (TCppItemBase*)(args_list->GetFirst());
	while (curr_param != NULL || curr_arg != NULL)
	{
		if ((curr_param == NULL || curr_arg == NULL))
		{
			// The lengths of the lists are different.
			return(FALSE);
		}

		// Compare the current param and the current argument.
		if (curr_param->ItemType() == cxx_template_template_param)
		{
			if (curr_arg->ItemType() != cxx_template_template_argument)
				return(FALSE);

			// The types of the objects are compatible.
			TCppTemplateTemplateArgument *templ_templ_arg = (TCppTemplateTemplateArgument*)curr_arg;
			if (templ_templ_arg->m_arg_type != TCppTemplateTemplateArgument::ctarg_template_template_param || templ_templ_arg->m_template_param == NULL)
				return(FALSE);
			else if (curr_param->DefnKeyId() != templ_templ_arg->m_template_param->DefnKeyId())
				return(FALSE);
		}
		else if (curr_param->ItemType() == cxx_template_type_param)
		{
			if (curr_arg->ItemType() != cxx_template_type_argument)
				return(FALSE);

			// The types of the objects are compatible.
			TCppDefnBase *type_arg_value = ((TCppTemplateTypeArgument*)curr_arg)->m_argument_value;
			if (type_arg_value == NULL || type_arg_value->ItemType() != cxx_template_type_param)
				return(FALSE);
			else if (curr_param->DefnKeyId() != type_arg_value->DefnKeyId())
				return(FALSE);
		}
		else if (curr_param->ItemType() == cxx_data_field)
		{
			if (curr_arg->ItemType() != cxx_operand_expr)
				return(FALSE);

			// The types of the objects are compatible.
			TCppOperandExpr *data_arg_oprnd = (TCppOperandExpr*)curr_arg;
			if (data_arg_oprnd->m_oprnd_type != eopr_data_field || data_arg_oprnd->m_data_field_ref == NULL)
				return(FALSE);
			else if (curr_param->DefnKeyId() != data_arg_oprnd->m_data_field_ref->DefnKeyId())
				return(FALSE);
		}
		else
		{
			// The type of the parameter object is bogus.
			assert(FALSE);
			return(FALSE);
		}

		// Shift to the next pair of params/args.
		curr_param = GetNextTemplateParameter(params_list, curr_param);
		curr_arg = (TCppItemBase*)(args_list->GetNext(curr_arg));
	}

	// The passed lists are compatible.
	return(TRUE);
}

bool TCppItemBase::CompareTemplateParams(TCppItemBase *param1, TCppItemBase *param2)
{
	if (param1->ItemType() != param2->ItemType())
		return(FALSE);

	// The types of params are the same. Compare extra props of some of the types.
	// Note that type parameters do not have any proprs at all.
	if (param1->ItemType() == cxx_template_template_param)
	{
		// Compare lists of params in these template tempate params.
		if (param1->NumDirectChildren() != param2->NumDirectChildren())
			return(FALSE);

		// The number of params is the same. Compare params one by one.
		for (TCppDirectChildrenIter iter1(param1), iter2(param2); (iter1 == TRUE && iter2 == TRUE); ++iter1, ++iter2)
		{
			if (CompareTemplateParams(iter1.CurrItemPtr(), iter1.CurrItemPtr()) == FALSE)
				return(FALSE);
		}
	}
	else if (param1->ItemType() == cxx_data_field)
	{
		// Compare data types of these params.
		TCppDataTypeBase *param1_type = ((TCppDataFieldDefn*)param1)->m_field_type;
		TCppDataTypeBase *param2_type = ((TCppDataFieldDefn*)param2)->m_field_type;
		if (CompareSameDatabaseDataTypes(param1_type, param2_type) == FALSE)
			return(FALSE);
	}

	// Passed params are equivalent.
	return(TRUE);
}

bool TCppItemBase::CompareTemplateParamLists(TList *params_list1, TList *params_list2)
{
	TCppDefnBase *param1 = GetFirstTemplateParameter(params_list1);
	TCppDefnBase *param2 = GetFirstTemplateParameter(params_list2);
	while (param1 != NULL || param2 != NULL)
	{
		if (param1 == NULL || param2 == NULL)
		{
			// The lengths of the template param lists are different.
			return(FALSE);
		}
		else if (CompareTemplateParams(param1, param2) == FALSE)
		{
			// The params are different.
			return(FALSE);
		}

		// Shift to the next pair of template params if any.
		param1 = GetNextTemplateParameter(params_list1, param1);
		param2 = GetNextTemplateParameter(params_list2, param2);
	}

	// Param lists matched. Note that default values if any can be still different.
	return(TRUE);
}

bool TCppItemBase::CompareSameDatabaseFunctionTypes(TCppFunctionTypeBase *type1, TCppFunctionTypeBase *type2, bool do_full_compare)
{
	// Both params should be meaningful. Ensure this.
	assert(type1 != NULL && type1->IsFunctionType() == TRUE);
	assert(type2 != NULL && type2->IsFunctionType() == TRUE);

	// Remove alias layers from both params, if they are present.
	while (type1->ItemType() == cxx_function_type_alias)
		type1 = ((TCppFunctionTypeAliasDefn*)type1)->m_base_type;
	while (type2->ItemType() == cxx_function_type_alias)
		type2 = ((TCppFunctionTypeAliasDefn*)type2)->m_base_type;

	if (type1->ItemType() == cxx_unresolved_code_entry_point || type2->ItemType() == cxx_unresolved_code_entry_point)
	{
		// At least one of the passed types is an unresolved type.
		if (type1->ItemType() == cxx_unresolved_code_entry_point && type2->ItemType() == cxx_unresolved_code_entry_point)
		{
			// Both params are unresolved function types.
			return(type1->ItemId() == type1->ItemId());
		}

		// Unresolved function type cannot be equivalent to any resolved func type.
		return(FALSE);
	}

	// All non alias function types are derived from the function type in this class library.
	// This means that exact types of the passed classes should not be compared.
	TCppFunctionTypeDefn &func1 = *(TCppFunctionTypeDefn*)type1;
	TCppFunctionTypeDefn &func2 = *(TCppFunctionTypeDefn*)type2;

	if (do_full_compare == TRUE)
	{
		// Compare misc props.
		if (func1.m_accs_level != func2.m_accs_level || func1.m_func_attrs != func1.m_func_attrs)
			return(FALSE);
	}

	// Compare params of the function types.
	TCppDataFieldDefn *prm1 = func1.GetFirstParam();
	TCppDataFieldDefn *prm2 = func2.GetFirstParam();
	while (prm1 != NULL || prm2 != NULL)
	{
		if (prm1 == NULL || prm2 == NULL)
		{
			// The number of params is different.
			return(FALSE);
		}
		else if (CompareSameDatabaseDataTypes(prm1->m_field_type, prm2->m_field_type) == FALSE)
		{
			// Types of params are different.
			return(FALSE);
		}

		// Params matched. Shift to the next pair of params if any.
		prm1 = func1.GetNextParam(prm1);
		prm2 = func2.GetNextParam(prm2);
	}

	// These function types as identical.
	return(TRUE);
}

const wchar_t *TCppItemBase::GetItemStructName(TCppItemType item_type)
{
	switch (item_type)
	{
		// CondPars high level analysis.
		case cxx_cond_pars_param:			return(L"TCppCondParsParam");

		// Definitions 1.
		case cxx_built_in_type:					return(L"TCppBuiltInTypeDefn");
		case cxx_bit_field_type:				return(L"TCppBitFieldTypeDefn");
		case cxx_cv_ptr_ref_type:				return(L"TCppCvPtrRefTypeDefn");
		case cxx_array_type:					return(L"TCppArrayTypeDefn");
		case cxx_data_type_alias:				return(L"TCppDataTypeAliasDefn");

		// Definitions 2.
		case cxx_enum_type:					return(L"TCppEnumTypeDefn");
		case cxx_enum_member:				return(L"TCppEnumMemberDefn");
		case cxx_struct_type:					return(L"TCppStructTypeDefn");
		case cxx_base_class_spec:				return(L"TCppBaseClassSpec");
		case cxx_field_align_spec:				return(L"TCppFieldAlignSpec");
		case cxx_friend_spec:					return(L"TCppFriendSpec");
		case cxx_data_field:					return(L"TCppDataFieldDefn");
		case cxx_data_field_initializer:			return(L"TCppDataFieldInitializer");

		// Definitions 3.
		case cxx_function_type:				return(L"TCppFunctionTypeDefn");
		case cxx_function_type_alias:			return(L"TCppFunctionTypeAliasDefn");
		case cxx_code_entry_point:				return(L"TCppCodeEntryPointDefn");
		case cxx_possible_exception_spec:		return(L"TCppPossibleExceptionSpec");

		// Definitions 4.
		case cxx_namespace:					return(L"TCppNamespaceDefn");
		case cxx_global_namespace:			return(L"TCppGlobalNamespaceDefn");
		case cxx_namespace_alias:				return(L"TCppNamespaceAliasDefn");
		case cxx_using_declaration:				return(L"TCppUsingDeclarationDefn");
		case cxx_using_directive:				return(L"TCppUsingDirectiveDefn");
		case cxx_asm_insert:					return(L"TCppAsmInsertDefn");
		case cxx_linkage_spec:				return(L"TCppLinkageSpec");

		// Templates1.
		case cxx_class_template:				return(L"TCppClassTemplateDefn");
		case cxx_class_template_spec:			return(L"TCppClassTemplateSpecialization");
		case cxx_class_template_inst:			return(L"TCppClassTemplateInstantiation");
		case cxx_func_template:				return(L"TCppFunctionTemplateDefn");
		case cxx_func_template_spec:			return(L"TCppFunctionTemplateSpecialization");
		case cxx_func_template_inst:			return(L"TCppFunctionTemplateInstantiation");

		// Templates2.
		case cxx_template_type_param:			return(L"TCppTemplateTypeParamDefn");
		case cxx_template_template_param:		return(L"TCppTemplateTemplateParamDefn");
		case cxx_template_type_argument:		return(L"TCppTemplateTypeArgument");
		case cxx_template_template_argument:	return(L"TCppTemplateTemplateArgument");

		// Unresolved objects.
		case cxx_unresolved_name_info:		return(L"TCppUnresolvedNameInfo");
		case cxx_unresolved_data_type:			return(L"TCppUnresolvedDataType");
		case cxx_unresolved_struct_type:		return(L"TCppUnresolvedStructType");
		case cxx_unresolved_data_field:			return(L"TCppUnresolvedDataField");
		case cxx_unresolved_code_entry_point:	return(L"TCppUnresolvedCodeEntryPoint");
		case cxx_unresolved_class_template:		return(L"TCppUnresolvedClassTemplate");
		case cxx_unresolved_func_template:		return(L"TCppUnresolvedFunctionTemplate");

		// Statements 1.
		case cxx_empty_stmt:					return(L"TCppEmptyStatement");
		case cxx_if_stmt:						return(L"TCppIfStatement");
		case cxx_switch_stmt:					return(L"TCppSwitchStatement");
		case cxx_for_stmt:					return(L"TCppForStatement");
		case cxx_while_stmt:					return(L"TCppWhileStatement");
		case cxx_do_while_stmt:				return(L"TCppDoWhileStatement");

		// Statements 2.
		case cxx_goto_stmt:					return(L"TCppGotoStatement");
		case cxx_break_stmt:					return(L"TCppBreakStatement");
		case cxx_continue_stmt:				return(L"TCppContinueStatement");
		case cxx_return_stmt:					return(L"TCppReturnStatement");

		// Statements 3.
		case cxx_block:						return(L"TCppBlockDefn");
		case cxx_try_block:					return(L"TCppTryBlockDefn");
		case cxx_catch_block:					return(L"TCppCatchBlockDefn");
		case cxx_light_block:					return(L"TCppLightBlockDefn");
		case cxx_label:						return(L"TCppLabelDefn");
		case cxx_case_label:					return(L"TCppCaseLabelDefn");

		// Expressions 1.
		case cxx_operand_expr:				return(L"TCppOperandExpr");
		case cxx_unary_expr:					return(L"TCppUnaryExpr");
		case cxx_binary_expr:					return(L"TCppBinaryExpr");
		case cxx_multiop_expr:				return(L"TCppMultiOpExpr");
		case cxx_assignment_expr:				return(L"TCppAssignmentExpr");

		// Expressions 2.
		case cxx_throw_expr:					return(L"TCppThrowExpr");
		case cxx_conditional_expr:				return(L"TCppConditionalExpr");
		case cxx_field_access_expr:			return(L"TCppFieldAccessExpr");
		case cxx_field_access_ptr_expr:			return(L"TCppFieldAccessPtrExpr");
		case cxx_array_index_expr:			return(L"TCppArrayIndexExpr");
		case cxx_function_call_expr:			return(L"TCppFunctionCallExpr");

		// Expressions 3.
		case cxx_cast_expr:					return(L"TCppCastExpr");
		case cxx_sizeof_expr:					return(L"TCppSizeofExpr");
		case cxx_new_expr:					return(L"TCppNewExpr");
		case cxx_delete_expr:					return(L"TCppDeleteExpr");
		case cxx_typeid_expr:					return(L"TCppTypeidExpr");
	}

	// Unknown or unsupported structure type.
	return(L"___UnknownDefnStructType");
}

bool TCppItemBase::CompareTypesInternal(TCppDefnBase *type1, TCppCvPtrRefSpec *ut1_layers, int num_ut1_layers, TCppDefnBase *type2, TCppCvPtrRefSpec *ut2_layers, int num_ut2_layers)
{
	// Both primary params of this method should be meaningful.
	assert(type1 != NULL && type2 != NULL);

	// Remove data alias layers from both params, if they are present.
	while (type1->ItemType() == cxx_data_type_alias)
		type1 = ((TCppDataTypeAliasDefn*)type1)->m_base_type;
	while (type2->ItemType() == cxx_data_type_alias)
		type2 = ((TCppDataTypeAliasDefn*)type2)->m_base_type;

	// Remove function alias layers from both params, if they are present.
	while (type1->ItemType() == cxx_function_type_alias)
		type1 = ((TCppFunctionTypeAliasDefn*)type1)->m_base_type;
	while (type2->ItemType() == cxx_function_type_alias)
		type2 = ((TCppFunctionTypeAliasDefn*)type2)->m_base_type;

	if (ut1_layers == NULL && ut2_layers == NULL)
	{
		if (type1->ItemType() == cxx_cv_ptr_ref_type && type2->ItemType() != cxx_cv_ptr_ref_type)
		{
			// Convert Type1 to its base type and an array of modifiers.
			TCppCvPtrRefTypeDefn *cvpr1 = (TCppCvPtrRefTypeDefn*)type1;
			type1 = cvpr1->m_base_type;
			ut1_layers = cvpr1->m_layers.DataPtr();
			num_ut1_layers = cvpr1->m_layers.NumItems();
		}
		else if (type1->ItemType() != cxx_cv_ptr_ref_type && type2->ItemType() == cxx_cv_ptr_ref_type)
		{
			// Convert Type2 to its base type and an array of modifiers.
			TCppCvPtrRefTypeDefn *cvpr2 = (TCppCvPtrRefTypeDefn*)type2;
			type2 = cvpr2->m_base_type;
			ut2_layers = cvpr2->m_layers.DataPtr();
			num_ut2_layers = cvpr2->m_layers.NumItems();
		}
	}

	if (ut1_layers != NULL)
	{
		// First type has unprocessed indirection layers.
		assert(num_ut1_layers > 0 && ut2_layers == NULL && num_ut2_layers == 0);

		if (type2->ItemType() == cxx_cv_ptr_ref_type)
		{
			// Compare unprocessed indirection layers of the first type to the indirection layers that are available in the second type.
			TCppCvPtrRefTypeDefn *cvpr2 = (TCppCvPtrRefTypeDefn*)type2;
			int cvpr2_layers = cvpr2->m_layers.NumItems();
			assert(cvpr2_layers > 0);

			int num_layers_to_compare = ___min(num_ut1_layers, cvpr2_layers);
			for (int offs=1; offs <= num_layers_to_compare; ++offs)
			{
				if (ut1_layers[num_ut1_layers-offs].Compare(cvpr2->m_layers[cvpr2_layers-offs]) == FALSE)
					return(FALSE);
			}

			// All layers that were compared matched.
			if (num_ut1_layers < cvpr2_layers)
			{
				// Second type has unprocessed layers.
				return(CompareTypesInternal(type1, NULL, 0, cvpr2->m_base_type, cvpr2->m_layers.DataPtr(), cvpr2_layers-num_layers_to_compare));
			}
			else if (num_ut1_layers > cvpr2_layers)
			{
				// First type still has unprocessed layers.
				return(CompareTypesInternal(type1, ut1_layers, num_ut1_layers-num_layers_to_compare, cvpr2->m_base_type, NULL, 0));
			}
		}
		else if (num_ut1_layers == 1 && ut1_layers->IsSimplePointer() == TRUE && type2->ItemType() == cxx_array_type)
		{
			// First type contains one layer of pure indirection while the second type is an array. Ignore this
			// indirection layer in the first type. Array and pure pointer to array are basically the same thing.
		}
		else
		{
			// Unreconciled indirection layers are present. The types are different.
			return(FALSE);
		}
	}
	else if (ut2_layers != NULL)
	{
		// Second type has unprocessed indirection layers.
		assert(num_ut2_layers > 0 && ut1_layers == NULL && num_ut1_layers == 0);

		if (type1->ItemType() == cxx_cv_ptr_ref_type)
		{
			// Compare indirection layers that are available in the first type to the unprocessed indirection layers of the second type.
			TCppCvPtrRefTypeDefn *cvpr1 = (TCppCvPtrRefTypeDefn*)type1;
			int cvpr1_layers = cvpr1->m_layers.NumItems();
			assert(cvpr1_layers > 0);

			int num_layers_to_compare = ___min(cvpr1_layers, num_ut2_layers);
			for (int offs=1; offs <= num_layers_to_compare; ++offs)
			{
				if (cvpr1->m_layers[cvpr1_layers-offs].Compare(ut2_layers[num_ut2_layers-offs]) == FALSE)
					return(FALSE);
			}

			// All layers that were compared matched.
			if (cvpr1_layers < num_ut2_layers)
			{
				// Second type still has unprocessed layers.
				return(CompareTypesInternal(cvpr1->m_base_type, NULL, 0, type2, ut2_layers, num_ut2_layers-num_layers_to_compare));
			}
			else if (cvpr1_layers > num_ut2_layers)
			{
				// First type has unprocessed layers.
				return(CompareTypesInternal(cvpr1->m_base_type, cvpr1->m_layers.DataPtr(), cvpr1_layers-num_layers_to_compare, type2, NULL, 0));
			}
		}
		else if (num_ut2_layers == 1 && ut2_layers->IsSimplePointer() == TRUE && type1->ItemType() == cxx_array_type)
		{
			// Second type contains one layer of pure indirection while the first type is an array. Ignore this
			// indirection layer in the second type. Array and pure pointer to array are basically the same thing.
		}
		else
		{
			// Unreconciled indirection layers are present. The types are different.
			return(FALSE);
		}
	}

	if (type1->IsDataType() == TRUE && type2->IsDataType() == TRUE)
	{
		// Both types are data types. To be the same they should belong to the same classes.
		if (type1->ItemType() != type2->ItemType())
			return(FALSE);
	}
	else if (type1->IsFunctionType() == TRUE && type2->IsFunctionType() == TRUE)
	{
		// Both base types are function types.
		return(CompareSameDatabaseFunctionTypes((TCppFunctionTypeBase*)type1, (TCppFunctionTypeBase*)type2));
	}
	else
	{
		// The types are different.
		return(FALSE);
	}

	// Compare types that belong to the same class.
	switch (type1->ItemType())
	{
		case cxx_built_in_type:
				{
					// Built in types have prefixed ids. Compare them.
					return(type1->ItemId() == type2->ItemId());
				}

		case cxx_bit_field_type:
				{
					// The types of both params are known. It is ok to do the typecast.
					TCppBitFieldTypeDefn *bitf1 = (TCppBitFieldTypeDefn*)type1;
					TCppBitFieldTypeDefn *bitf2 = (TCppBitFieldTypeDefn*)type2;

					// Check if bit sizes are const or variable.
					if (bitf1->m_bit_length <= 0 || bitf2->m_bit_length <= 0)
						return(FALSE);

					// Bit sizes and base types should match.
					return(bitf1->m_bit_length == bitf2->m_bit_length && CompareTypesInternal(bitf1->m_base_type, NULL, 0, bitf2->m_base_type, NULL, 0) == TRUE);
				}

		case cxx_cv_ptr_ref_type:
				{
					// The types of both params are known. It is ok to do the typecast.
					TCppCvPtrRefTypeDefn *cvpr1 = (TCppCvPtrRefTypeDefn*)type1;
					TCppCvPtrRefTypeDefn *cvpr2 = (TCppCvPtrRefTypeDefn*)type2;
					int cvpr1_layers = cvpr1->m_layers.NumItems();
					int cvpr2_layers = cvpr2->m_layers.NumItems();
					assert(cvpr1_layers > 0 && cvpr2_layers > 0);

					// Compare as many indirection layers as possible.
					int num_layers_to_compare = ___min(cvpr1_layers, cvpr2_layers);
					for (int offs=1; offs <= num_layers_to_compare; ++offs)
					{
						if (cvpr1->m_layers[cvpr1_layers-offs].Compare(cvpr2->m_layers[cvpr2_layers-offs]) == FALSE)
							return(FALSE);
					}

					// All layers that were compared matched.
					if (cvpr1_layers < cvpr2_layers)
					{
						// Second type has unprocessed layers.
						return(CompareTypesInternal(cvpr1->m_base_type, NULL, 0, cvpr2->m_base_type, cvpr2->m_layers.DataPtr(), cvpr2_layers-num_layers_to_compare));
					}
					else if (cvpr1_layers > cvpr2_layers)
					{
						// First type still has unprocessed layers.
						return(CompareTypesInternal(cvpr1->m_base_type, cvpr1->m_layers.DataPtr(), cvpr1_layers-num_layers_to_compare, cvpr2->m_base_type, NULL, 0));
					}

					// The number of indirection layers is the same. Compare the base types.
					return(CompareTypesInternal(cvpr1->m_base_type, NULL, 0, cvpr2->m_base_type, NULL, 0));
				}

		case cxx_array_type:
				{
					// First compare the number of dimensions and the base types.
					TCppArrayTypeDefn *array1 = (TCppArrayTypeDefn*)type1;
					TCppArrayTypeDefn *array2 = (TCppArrayTypeDefn*)type1;
					if (array1->m_dimensions.NumItems() != array2->m_dimensions.NumItems())
						return(FALSE);
					else if (CompareTypesInternal(array1->m_base_type, NULL, 0, array2->m_base_type, NULL, 0) == FALSE)
						return(FALSE);

					// Compare dimensions one by one excluding the first index.
					for (int inx=1; inx<array1->m_dimensions.NumItems(); ++inx)
					{
						if (array1->m_dimensions[inx].m_num_elements < 0 || array2->m_dimensions[inx].m_num_elements < 0)
						{
							// At least one dimension is expressed with non const expr expression. Consider these
							// dimensions as different nor now.
							return(FALSE);
						}
						else if (array1->m_dimensions[inx].m_num_elements != array2->m_dimensions[inx].m_num_elements)
						{
							// The numeric sizes are different.
							return(FALSE);
						}
					}

					// All props matched.
					return(TRUE);
				}

		case cxx_enum_type:
		case cxx_struct_type:
		case cxx_class_template:
		case cxx_class_template_spec:
		case cxx_class_template_inst:
		case cxx_template_type_param:
		case cxx_unresolved_data_type:
		case cxx_unresolved_struct_type:
		case cxx_unresolved_class_template:
				{
					// This method compares types that belong to the same database. This means that types
					// of these categories are equivalent when their item ids are the same.
					return(type1->ItemId() == type2->ItemId());
				}
	}

	// Consider this bogus situation as different types.
	assert(FALSE);
	return(FALSE);
}

TCppDefnBase *TCppItemBase::GetFirstTemplateParameter(const TList *members_list)
{
	if (members_list == NULL)
		return(NULL);

	TCppDefnBase *param = (TCppDefnBase*)members_list->GetFirst();
	while (param != NULL && IsTemplateParam(param) == FALSE)
		param = (TCppDefnBase*)members_list->GetNext(param);

	return(param);
}

TCppDefnBase *TCppItemBase::GetNextTemplateParameter(const TList *members_list, TCppDefnBase *param)
{
	if (members_list == NULL || param == NULL)
		return(NULL);

	param = (TCppDefnBase*)members_list->GetNext(param);
	while (param != NULL && IsTemplateParam(param) == FALSE)
		param = (TCppDefnBase*)members_list->GetNext(param);

	return(param);
}

//---------------------------------------------------------------------------
//  =====================  TCppDefnBase  ========================
//---------------------------------------------------------------------------

static const wchar_t *g_CppElabTypeFriendlyName1[elab_num_types] =
{
	L"elab_none", L"enum", L"struct", L"union", L"class",
};

static const wchar_t *g_CppElabTypeFriendlyName2[elab_num_types] =
{
	L"Elab_None", L"Enum", L"Struct", L"Union", L"Class",
};

static const wchar_t *g_CppElabTypeFriendlyName3[elab_num_types] =
{
	L"elabs_nones", L"enums", L"structs", L"unions", L"classes",
};

static const wchar_t *g_CppElabTypeFriendlyName4[elab_num_types] =
{
	L"Elabs_Nones", L"Enums", L"Structs", L"Unions", L"Classes",
};

static const wchar_t *g_CppDefnProblemEnumName[cdpr_num_types] =
{
	L"cdpr_none",
	L"cdpr_ambig",	L"cdpr_unapprop",		L"cdpr_unres",
	L"cdpr_errloc",	L"cdpr_dupe",			L"cdpr_conflict",
};

const wchar_t *TCppDefnBase::GetElabTypeName(TCppElabType elab, bool want_first_cap, bool plural_case)
{
	if (elab < 0 || elab >= elab_num_types)
		return(L"BogusElabTypeValue");

	if (plural_case == FALSE)
		return((want_first_cap == FALSE) ? g_CppElabTypeFriendlyName1[elab] : g_CppElabTypeFriendlyName2[elab]);
	else return((want_first_cap == FALSE) ? g_CppElabTypeFriendlyName3[elab] : g_CppElabTypeFriendlyName4[elab]);
}

const wchar_t *TCppDefnBase::GetObjectTypeFriendlyName(wchar_t *buff80, TCppItemType item_type, TCppElabType elab, bool want_first_cap)
{
	// Create name of the object that can be used in the context of the error message.
	switch (item_type)
	{
		case cxx_built_in_type:
				return((want_first_cap == TRUE) ? L"Built in type" : L"built in type");

		case cxx_bit_field_type:
				return((want_first_cap == TRUE) ? L"Bit field type" : L"bit field type");

		case cxx_cv_ptr_ref_type:
		case cxx_array_type:
		case cxx_data_type_alias:
				return((want_first_cap == TRUE) ? L"Type definition" : L"type definition");

		case cxx_enum_type:
				return((want_first_cap == TRUE) ? L"Enum" : L"enum");

		case cxx_enum_member:
				return((want_first_cap == TRUE) ? L"Enum member" : L"enum member");

		case cxx_struct_type:
				return(GetElabTypeName(elab, want_first_cap));

		case cxx_base_class_spec:
				return((want_first_cap == TRUE) ? L"Base class spec" : L"base class spec");

		case cxx_class_template:
				swprintf(buff80, 80, L"%s template", GetElabTypeName(elab, want_first_cap));
				return(buff80);

		case cxx_class_template_spec:
				swprintf(buff80, 80, L"%s template specialization", GetElabTypeName(elab, want_first_cap));
				return(buff80);

		case cxx_class_template_inst:
				swprintf(buff80, 80, L"%s template instantiation", GetElabTypeName(elab, want_first_cap));
				return(buff80);

		case cxx_unresolved_data_type:
				swprintf(buff80, 80, L"%s data type", ((want_first_cap == TRUE) ? L"Unresolved" : L"unresolved"));
				return(buff80);

		case cxx_unresolved_struct_type:
				swprintf(buff80, 80, L"%s %s", ((want_first_cap == TRUE) ? L"Unresolved" : L"unresolved"), ((elab != elab_none) ? GetElabTypeName(elab) : L"structured data type"));
				return(buff80);

		case cxx_unresolved_class_template:
				swprintf(buff80, 80, L"%s %s template", ((want_first_cap == TRUE) ? L"Unresolved" : L"unresolved"), ((elab != elab_none) ? GetElabTypeName(elab) : L"ELAB-IS-NOT-SET"));
				return(buff80);

		default:
			// Other classes do not have friendly names. Return the name of the cpp class.
			return(TCppItemHelper::GetItemTypeEnumName(item_type));
	}
}

const wchar_t *TCppDefnBase::GetDefnProblemEnumName(TCppDefnProblem cdpr)
{
	if (cdpr < 0 || cdpr >= cdpr_num_types)
		return(L"BogusDefnProblemValue");

	return(g_CppDefnProblemEnumName[cdpr]);
}

void TCppDefnBase::AddToMembersListAndDefnsSpace(TCppItemBase *item, TList &members_list, TCppDefnsSpace &defns_space)
{
	// The passed item should not be part of any list. Ensure this.
	assert(item != NULL && item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Add passed item to the primary list of members.
	item->m_cib_parent = this;
	members_list.AppendItem(item);

	if (item->IsDefinition() == TRUE && ((TCppDefnBase*)item)->m_defn_proxy.IsInList() == FALSE)
	{
		// Passed item is a definition and it is not part of the defintions space. Note that the app code may add
		// object to the defns space as an unnamed object first and then call this method. It may also happen that
		// defn is already added to the defns space of some other object. This is why the second part of the condition
		// above is so important.
		defns_space.AddRegularDefinition((TCppDefnBase*)item);
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

int TCppDefnBase::GetNumEnumTypes(TCppDataTypeBase *base_type) const
{
	TList *members_list = GetNestedMembersList();
	if (members_list == NULL)
		return(0);

	int cnt_enum_types = 0;
	for (TListIter<TCppItemBase> iter(members_list); iter; ++iter)
	{
		if (iter.CurrItem().ItemType() == cxx_enum_type)
		{
			if (base_type != NULL)
			{
				// Caller wants info on only some specific enums.
				TCppEnumTypeDefn &enum_type = (TCppEnumTypeDefn&)iter.CurrItem();
				if (enum_type.m_base_type != NULL && CompareSameDatabaseDataTypes(base_type, enum_type.m_base_type) == TRUE)
					cnt_enum_types++;
			}
			else
			{
				// Caller wants the number of all enum types.
				cnt_enum_types++;
			}
		}
	}

	return(cnt_enum_types);
}

int TCppDefnBase::GetNumStructTypes(TCppElabType elab_type_only) const
{
	TList *members_list = GetNestedMembersList();
	if (members_list == NULL)
		return(0);

	if (elab_type_only <= elab_none)
		elab_type_only = elab_num_types;

	int cnt_struct_types = 0;
	for (TListIter<TCppItemBase> iter(members_list); iter; ++iter)
	{
		if (iter.CurrItem().ItemType() == cxx_struct_type)
		{
			if (elab_type_only == elab_num_types || ((TCppStructTypeDefn&)iter.CurrItem()).DefnElabType() == elab_type_only)
				cnt_struct_types++;
		}
	}

	return(cnt_struct_types);
}

int TCppDefnBase::GetNumDataFields(TCppStorageClass strg_class_only) const
{
	TList *members_list = GetNestedMembersList();
	if (members_list == NULL)
		return(0);

	if (strg_class_only < 0)
		strg_class_only = strgc_num_types;

	int cnt_data_fields = 0;
	for (TListIter<TCppItemBase> iter(members_list); iter; ++iter)
	{
		if (iter.CurrItem().ItemType() == cxx_data_field)
		{
			if (strg_class_only == strgc_num_types || ((TCppDataFieldDefn&)iter.CurrItem()).m_strg_class == strg_class_only)
				cnt_data_fields++;
		}
	}

	return(cnt_data_fields);
}

int TCppDefnBase::GetNumCodeEntryPoints(TCppSpecFuncType spcft_only) const
{
	TList *members_list = GetNestedMembersList();
	if (members_list == NULL)
		return(0);

	if (spcft_only < 0)
		spcft_only = spcft_num_types;

	int cnt_code_entry_points = 0;
	for (TListIter<TCppItemBase> iter(members_list); iter; ++iter)
	{
		if (iter.CurrItem().ItemType() == cxx_code_entry_point)
		{
			if (spcft_only == spcft_num_types || ((TCppCodeEntryPointDefn&)iter.CurrItem()).m_spec_type == spcft_only)
				cnt_code_entry_points++;
		}
	}

	return(cnt_code_entry_points);
}

TCppDefnBase *TCppDefnBase::FindNamedDirectChildDefn(const wchar_t *object_name)
{
	assert(object_name != NULL && object_name[0] != 0);
	TList *members_list = GetNestedMembersList();
	if (members_list == NULL)
		return(0);

	TCppDefnBase *candidate = NULL;
	for (TListIter<TCppItemBase> iter(members_list); iter; ++iter)
	{
		if (iter.CurrItem().IsDefinition() == FALSE)
			continue;

		TCppDefnBase *defn = (TCppDefnBase*)iter.CurrItemPtr();
		if (defn->DefnName() != NULL && wcscmp(defn->DefnName(), object_name) == 0)
		{
			if (candidate != NULL)
				return(NULL);
			candidate = defn;
		}
	}

	return(candidate);
}

//-----------------------------------------------------------------------------
//  ====================  TCppDataTypeBase  =======================
//-----------------------------------------------------------------------------

TCppDataTypeBase *TCppDataTypeBase::GetUnderlyingNumericType(TCppDataTypeBase *data_type, bool template_conext)
{
	for(;;)
	{
		assert(data_type != NULL && data_type->IsDataType() == TRUE);

		switch (data_type->ItemType())
		{
			case cxx_built_in_type:
					{
						TCppBuiltInTypeDefn *bltin_type = (TCppBuiltInTypeDefn*)data_type;
						if (bltin_type->m_type == bltin_type_bool || bltin_type->m_type == bltin_type_char || bltin_type->m_type == bltin_type_int)
							return(data_type);

						// All other built in types are not numeric.
						break;
					}

			case cxx_bit_field_type:
					{
						TCppBitFieldTypeDefn *bf_type = (TCppBitFieldTypeDefn*)data_type;
						data_type = bf_type->m_base_type;
						continue;
					}

			case cxx_cv_ptr_ref_type:
					{
						TCppCvPtrRefTypeDefn *cvpr_type = (TCppCvPtrRefTypeDefn*)data_type;
						if (cvpr_type->m_layers.NumItems() <= 0)
						{
							// This is bogus indirection type without modification layers.
							if (cvpr_type->m_base_type->IsDataType() == TRUE)
							{
								data_type = (TCppDataTypeBase*)cvpr_type->m_base_type;
								continue;
							}
						}
						else if (cvpr_type->m_layers.NumItems() == 1)
						{
							if (cvpr_type->m_layers[0].cvpr_pointer == FALSE && cvpr_type->m_layers[0].cvpr_reference == FALSE)
							{
								// Type contains single modification layer that is just const and/or volatile. Ignore this layer.
								if (cvpr_type->m_base_type->IsDataType() == TRUE)
								{
									data_type = (TCppDataTypeBase*)cvpr_type->m_base_type;
									continue;
								}
							}
						}

						// Pointers and references cannot be used as numeric type.
						break;
					}

			case cxx_data_type_alias:
					{
						TCppDataTypeAliasDefn *alias_type = (TCppDataTypeAliasDefn*)data_type;
						data_type = alias_type->m_base_type;
						continue;
					}

			case cxx_enum_type:
					{
						TCppEnumTypeDefn *enum_type = (TCppEnumTypeDefn*)data_type;
						if (enum_type->m_base_type == NULL)
							return(enum_type);

						data_type = enum_type->m_base_type;
						continue;
					}

			case cxx_template_type_param:
			case cxx_unresolved_data_type:
			case cxx_unresolved_struct_type:
					{
						if (template_conext == TRUE)
						{
							// These types may become numeric during the template instantiation.
							// Consider these types as potentially convertible to numeric.
							return(data_type);
						}

						// Unresolved types are not numeric in the non template context.
						break;
					}
		}

		// Conversion failed.
		return(NULL);
	}
}

//-----------------------------------------------------------------------------
//  ====================  TCppExpressionBase  =======================
//-----------------------------------------------------------------------------

const wchar_t *TCppExpressionBase::OperationEnumName() const
{
	if (m_operation == 0 || m_operation == opr_none)
		return(L"OPR_NONE");

	return(TLexema::GetKeywordIdEnumName(m_operation));
}

TCppBuiltInTypeIdent TCppExpressionBase::GetBuiltInTypeIdFromNumberSubtype(TLexNumberType subt)
{
	if (subt >= lnt_s8bit && subt <= lnt_u64bit)
	{
		// Make a one to one translation.
		return((TCppBuiltInTypeIdent)(bltin_id_sint8+(subt-lnt_s8bit)));
	}

	// Param is strange or bogus.
	assert(FALSE);
	return(bltin_id_sint32);
}

TCppBuiltInTypeIdent TCppExpressionBase::GetBuiltInTypeIdFromCharconstSubtype(TLexCharConstType subt)
{
	// Make simplified translation.
	if (subt == lchct_unicode)
		return(bltin_id_uchar16);
	else return(bltin_id_schar8);
}

TCppBuiltInTypeIdent TCppExpressionBase::GetBuiltInTypeIdFromStringSubtype(TLexStringType subt)
{
	// Make simplified translation.
	if (subt == lstrt_unicode)
		return(bltin_id_uchar16);
	else return(bltin_id_schar8);
}

TLexNumberType TCppExpressionBase::GetNumericOperationResultType(TLexNumberType subt1, TLexNumberType subt2)
{
	if (subt1 == subt2)
		return(subt1);

	// The subtypes are different.
	int sz_subt1 = LEX_NUM_SUBT_SIZE_PART(subt1);
	int sz_subt2 = LEX_NUM_SUBT_SIZE_PART(subt2);
	int sz_rez = ___max(sz_subt1, sz_subt2);
	int unsigned_flag = 0;
	if (LEX_NUM_SUBT_UNSIGEND(subt1) == TRUE || LEX_NUM_SUBT_UNSIGEND(subt2) == TRUE)
		int unsigned_flag = 1;

	return((TLexNumberType)(sz_rez+unsigned_flag));
}

TLexNumberType TCppExpressionBase::GetNumericSubtypeFromValue(__int64 value)
{
	if ((value & 0x8000000000000000) == 0)
	{
		// The passed value is positive.
		if ((value & 0x7FFFFFFF00000000) != 0)
			return(lnt_s64bit);
		else if ((value & 0x80000000) != 0)
			return(lnt_u32bit);
		else if ((value & 0x7FFF0000) != 0)
			return(lnt_s32bit);
		else if ((value & 0x8000) != 0)
			return(lnt_u16bit);
		else if ((value & 0x7F00) != 0)
			return(lnt_s16bit);

		// The value fits into a byte.
		if ((value & 0x80) != 0)
			return(lnt_u8bit);
		else return(lnt_s8bit);
	}
	else
	{
		// The passed value is negative.
		if ((value & 0x7FFFFFFF80000000) != 0x7FFFFFFF80000000)
			return(lnt_u64bit);
		else if ((value & 0x7FFF8000) != 0x7FFF8000)
			return(lnt_s32bit);
		else if ((value & 0x7F80) != 0x7F80)
			return(lnt_s16bit);

		// The value fits into a byte.
		return(lnt_s8bit);
	}
}

//-----------------------------------------------------------------------------
//  ===================  TCppFunctionModifierSpec  ====================
//-----------------------------------------------------------------------------

int TCppFunctionModifierSpec::GetNumParams() const
{
	if (m_params_beg == NULL && m_params_end == NULL)
		return(0);

	// This function layer has non empty list of params and/or exception specs.
	assert(m_params_beg != NULL && m_params_end != NULL);

	int cnt_params = 0;
	TList func_props_list(m_params_beg, m_params_end);
	for (TListIter<TCppItemBase> iter(func_props_list); iter; ++iter)
	{
		if (TCppFunctionTypeDefn::IsParamItem(iter.CurrItemPtr()) == TRUE)
			cnt_params++;
	}

	// Return the number of parameter records if any.
	func_props_list.Clear();
	return(cnt_params);
}

TCppDataFieldDefn *TCppFunctionModifierSpec::GetFirstParam() const
{
	if (m_params_beg == NULL && m_params_end == NULL)
		return(NULL);

	// This function layer has non empty list of params and/or exception specs.
	assert(m_params_beg != NULL && m_params_end != NULL);
	TList func_props_list(m_params_beg, m_params_end);

	TCppItemBase *param = (TCppItemBase*)func_props_list.GetFirst();
	while (param != NULL && TCppFunctionTypeDefn::IsParamItem(param) == FALSE)
		param = (TCppItemBase*)func_props_list.GetNext(param);

	func_props_list.Clear();
	return((TCppDataFieldDefn*)param);
}

TCppDataFieldDefn *TCppFunctionModifierSpec::GetNextParam(TCppDataFieldDefn *curr_param) const
{
	if (m_params_beg == NULL && m_params_end == NULL)
		return(NULL);

	// This function layer has non empty list of params and/or exception specs.
	assert(m_params_beg != NULL && m_params_end != NULL);
	TList func_props_list(m_params_beg, m_params_end);

	TCppItemBase *param = (TCppItemBase*)func_props_list.GetNext(curr_param);
	while (param != NULL && TCppFunctionTypeDefn::IsParamItem(param) == FALSE)
		param = (TCppItemBase*)func_props_list.GetNext(param);

	func_props_list.Clear();
	return((TCppDataFieldDefn*)param);
}

void TCppFunctionModifierSpec::SetupFunctionTypeObject(TCppFunctionTypeDefn *func_type_defn)
{
	// In this context the const attribute means that member function is not changing its object instance.
	// Note that function attributes are a set of flags. Just raise the appropriate flag.
	if (m_this_cv_spec.cvpr_const == TRUE)
		func_type_defn->m_func_attrs = (TCppFunctionAttr)(func_type_defn->m_func_attrs | fcta_this_const);
	if (m_this_cv_spec.cvpr_volatile == TRUE)
		func_type_defn->m_func_attrs = (TCppFunctionAttr)(func_type_defn->m_func_attrs | fcta_this_volatile);

	if (m_params_beg != NULL || m_params_end != NULL)
	{
		// This function layer has non empty list of params and/or possible exception specs.
		assert(m_params_beg != NULL && m_params_end != NULL);
		TList func_props_list(m_params_beg, m_params_end);
		func_type_defn->AddListOfChildren(func_props_list);
		m_params_beg = m_params_end = NULL;
	}
}

//-----------------------------------------------------------------------------
//  ===================  TCppDeclaratorModifier  ======================
//-----------------------------------------------------------------------------

void TCppDeclaratorModifier::ReleaseResources()
{
	if (m_dmt == dclmdf_array)
	{
		// Release the template expression if any.
		if (m_array_spec.m_num_elements_expr != NULL)
			delete m_array_spec.m_num_elements_expr;
	}
	else if (m_dmt == dclmdf_function && (m_func_spec.m_params_beg != NULL || m_func_spec.m_params_end != NULL))
	{
		// This function layer owns func params and/or exception specs.
		assert(m_func_spec.m_params_beg != NULL && m_func_spec.m_params_end != NULL);
		TList func_props_list(m_func_spec.m_params_beg, m_func_spec.m_params_end);
		func_props_list.DeleteAll();

		// Clear the resource pointers.
		m_func_spec.m_params_beg = m_func_spec.m_params_end = NULL;
	}
}

//-----------------------------------------------------------------------------
//  =====================  TCppDefnsSpace  ========================
//-----------------------------------------------------------------------------

void TCppDefnsSpace::Clear()
{
	// Clear named buckets. Note that buckets are not owning their items.
	TList *pslot1 = GetNamedBucket(0);
	int num_named_buckets = NumNamedBuckets();
	for (int i1=0; i1<num_named_buckets; ++i1, ++pslot1)
		pslot1->Clear();

	// Clear special buckets. Note that buckets are not owning their items.
	TList *pslot2 = GetSpecialBucket(0);
	int num_special_buckets = NumSpecialBuckets();
	for (int i2=0; i2<num_special_buckets; ++i2, ++pslot2)
		pslot2->Clear();
}

void TCppDefnsSpace::AddRegularDefinition(TCppDefnBase *defn)
{
	assert(defn != NULL);

	// Check the type of the passed definition.
	switch (defn->ItemType())
	{
		case cxx_cv_ptr_ref_type:
		case cxx_array_type:
				break;

		case cxx_function_type:
				{
					// Function type can represent only as a regular function. Ensure this.
					TCppFunctionTypeDefn *func = (TCppFunctionTypeDefn*)defn;
					assert(func->m_spec_type == spcft_none);
				}
				break;

		case cxx_data_type_alias:
		case cxx_enum_type:
		case cxx_enum_member:
		case cxx_struct_type:
		case cxx_data_field:
		case cxx_function_type_alias:
		case cxx_namespace:
		case cxx_namespace_alias:
		case cxx_template_type_param:						// This is template param that is a data type.
		case cxx_template_template_param:					// This is template param that is a template of the class.
				break;

		case cxx_base_class_spec:
				{
					// These objects are always named. This is the FIRST group of possible named overloading.
					// Although overloading should be rather rare case here. It happens only when base classes
					// come from different classes/namespaces and they have the same short name.
				}
				break;

		case cxx_class_template:
		case cxx_class_template_spec:
		case cxx_class_template_inst:
				{
					// These object types are always named. This is the SECOND group of possible overloading.
				}
				break;

		case cxx_code_entry_point:
		case cxx_func_template:
		case cxx_func_template_spec:
		case cxx_func_template_inst:
				{
					// Only simple functions and constructors are considered as named and they are added to named buckets.
					// Destructors, overloaded operators and overloaded conversions are considered as non named and they
					// are added to special buckets. This is the THIRD group of possible named overloading.
					TCppCodeEntryPointDefn *func = (TCppCodeEntryPointDefn*)defn;
					if (func->m_spec_type >= spcft_dector)
					{
						// Add passed item to the special buckets.
						assert(defn->m_defn_proxy.IsInList() == FALSE);
						AddToSpecialBuckets(defn);
						return;
					}
				}
				break;

		default:
			// Unexpected type of the object.
			return;
			assert(FALSE);
	}

	// This is a named definition.
	assert(defn->m_defn_proxy.IsInList() == FALSE);

	// Definition should be added to the defns space only if its key id is not zero. Zero key id is valid and
	// it is used in several different cases. For example zero key id is used for handling name collisions.
	// In this case an alternative name "name_@123" is created and this name does not have any key_id.
	if (defn->DefnKeyId() != 0)
		AddToNamedBuckets(defn);
}

void TCppDefnsSpace::AddSpecialDefinition(TCppDefnBase *defn)
{
	assert(defn != NULL && defn->DefnKeyId() == 0);

	// Verify the type of the passed definition.
	switch (defn->ItemType())
	{
		case cxx_bit_field_type:
				{
					// The bit field type should be cached only if it contains const expression.
					TCppBitFieldTypeDefn *bft = (TCppBitFieldTypeDefn*)defn;
					if (bft->m_bit_length <= 0)
						return;
				}
				break;

		case cxx_cv_ptr_ref_type:
		case cxx_array_type:
		case cxx_function_type:
				break;

		case cxx_unresolved_data_type:
		case cxx_unresolved_struct_type:
		case cxx_unresolved_data_field:
		case cxx_unresolved_code_entry_point:
		case cxx_unresolved_class_template:
		case cxx_unresolved_func_template:
				break;

		default:
			// Unexpected type of the object.
			assert(FALSE);
			return;
	}

	// Add passed item to the defns space.
	assert(defn->m_defn_proxy.IsInList() == FALSE);
	AddToSpecialBuckets(defn);
}

TCppBitFieldTypeDefn *TCppDefnsSpace::LookupBitFieldType(ID base_defn_id, short bit_length)
{
	assert(base_defn_id != 0);
	assert(bit_length >= 0);

	// Search the appropriate hash table bucket. Overloading cannot take place.
	TCppDefnsBucket *slot = GetSpecialBucket(base_defn_id ^ bit_length);
	for (TCppDefnsSpaceBucketDefnTypeIter<TCppBitFieldTypeDefn> iter(slot, cxx_bit_field_type); iter; ++iter)
	{
		TCppBitFieldTypeDefn *bft = iter.ItemPtr();
		if (bft->m_bit_length > 0 && bft->m_base_type->ItemId() == base_defn_id && bft->m_bit_length == bit_length)
			return(bft);
	}

	// Requested bit field type is not available.
	return(NULL);
}

TCppCvPtrRefTypeDefn *TCppDefnsSpace::LookupUnnamedCvPtrRefType(ID base_defn_id, TCppDeclaratorModifier *layers, int num_layers)
{
	assert(base_defn_id != 0);
	assert(num_layers > 0);

	// Search the appropriate hash table bucket. Overloading cannot take place.
	TCppDefnsBucket *slot = GetSpecialBucket(base_defn_id ^ num_layers);
	for (TCppDefnsSpaceBucketDefnTypeIter<TCppCvPtrRefTypeDefn> iter(slot, cxx_cv_ptr_ref_type); iter; ++iter)
	{
		TCppCvPtrRefTypeDefn *cvpr = iter.ItemPtr();
		if (cvpr->m_base_type->ItemId() != base_defn_id || cvpr->m_layers.NumItems() != num_layers)
			continue;

		bool full_match = TRUE;
		for (int inx=0; inx<num_layers; ++inx)
		{
			if (cvpr->m_layers[inx].Compare(layers[inx].m_cvpr_spec) == FALSE)
			{
				full_match = FALSE;
				break;
			}
		}

		if (full_match == TRUE)
			return(cvpr);
	}

	// Requested prefixed type is not available.
	return(NULL);
}

TCppArrayTypeDefn *TCppDefnsSpace::LookupUnnamedArrayType(ID base_defn_id, TCppDeclaratorModifier *dimensions, int num_dimensions)
{
	assert(base_defn_id != 0);
	assert(num_dimensions> 0);

	// Search the appropriate hash table bucket. Overloading cannot take place.
	TCppDefnsBucket *slot = GetSpecialBucket(base_defn_id ^ (int)dimensions[0].m_array_spec.m_num_elements);
	for (TCppDefnsSpaceBucketDefnTypeIter<TCppArrayTypeDefn> iter(slot, cxx_array_type); iter; ++iter)
	{
		TCppArrayTypeDefn *arr = iter.ItemPtr();
		if (arr->m_base_type->ItemId() != base_defn_id || arr->m_dimensions.NumItems() != num_dimensions)
			continue;

		bool full_match = TRUE;
		for (int inx=0; inx<num_dimensions; ++inx)
		{
			if (arr->m_dimensions[inx].m_num_elements < 0 || dimensions[inx].m_array_spec.m_num_elements < 0)
			{
				full_match = FALSE;
				break;
			}
			else if (arr->m_dimensions[inx].m_num_elements != dimensions[inx].m_array_spec.m_num_elements)
			{
				full_match = FALSE;
				break;
			}
		}

		if (full_match == TRUE)
			return(arr);
	}

	// Requested array type is not available.
	return(NULL);
}

TCppFunctionTypeDefn *TCppDefnsSpace::LookupUnnamedFunctionType(ID ret_value_type_id, TCppDeclaratorModifier *func_mdf)
{
	// Info about the return value type should be meaningful.
	assert(ret_value_type_id != 0);

	// Search the appropriate hash bucket. Note that overloading may take place.
	TCppDefnsBucket *slot = GetSpecialBucket(ret_value_type_id);
	for (TCppDefnsSpaceBucketDefnTypeIter<TCppFunctionTypeDefn> iter(slot, cxx_function_type); iter; ++iter)
	{
		TCppFunctionTypeDefn *func = iter.ItemPtr();
		if (func->m_return_value_type->ItemId() == ret_value_type_id && func->CompareParameterLists(func_mdf) == TRUE)
		{
			return(func);
		}
	}

	// There is no function type with the requested return value type and list of params.
	return(NULL);
}

TCppDefnBase *TCppDefnsSpace::LookupUnresolvedObject(TCppItemType item_type, TCppUnresolvedNameInfo *unres_name_info, TCppDefnProblem item_problem)
{
	assert(item_type > cxx_none && item_type < cxx_num_types);
	assert(unres_name_info != NULL && item_problem != cdpr_none);

	// Search the appropriate hash table bucket. Use the object type filtering.
	TCppDefnsBucket *slot = GetSpecialBucket(unres_name_info->GetHashValue());
	for (TCppDefnsSpaceBucketDefnTypeIter<TCppDefnBase> iter(slot, item_type); iter; ++iter)
	{
		TList *unres_names_list = iter.Item().GetUnresolvedNamesList();
		assert(unres_names_list != NULL);

		if (unres_names_list->NumItems() == 1 && iter.Item().DefnProblem() == item_problem)
		{
			// The number of name infos in the current object is one and the type of the problem is the same.
			if (TCppUnresolvedNameInfo::CompareNameInfoInstances((TCppUnresolvedNameInfo*)unres_names_list->GetFirst(), unres_name_info) == TRUE)
				return(iter.ItemPtr());
		}
	}

	// Requested unresolved object is not available.
	return(NULL);
}

TCppDefnBase *TCppDefnsSpace::LookupUnresolvedObject(TCppItemType item_type, TList *unres_name_infos_seq, TCppDefnProblem item_problem)
{
	assert(item_type > cxx_none && item_type < cxx_num_types);
	assert(unres_name_infos_seq != NULL && item_problem != cdpr_none);

	// Search the appropriate hash table bucket. Use the object type filtering.
	TCppDefnsBucket *slot = GetSpecialBucket(TCppUnresolvedNameInfo::GetHashValueForNamesSeq(unres_name_infos_seq));
	for (TCppDefnsSpaceBucketDefnTypeIter<TCppDefnBase> iter(slot, item_type); iter; ++iter)
	{
		TList *unres_names_list = iter.Item().GetUnresolvedNamesList();
		assert(unres_names_list != NULL);

		if (iter.Item().DefnProblem() == item_problem && TCppUnresolvedNameInfo::CompareNameInfoSequencies(unres_names_list, unres_name_infos_seq) == TRUE)
		{
			// The type of the problem and all name infos in the current object matched.
			return(iter.ItemPtr());
		}
	}

	// Requested unresolved object is not available.
	return(NULL);
}

void TCppDefnsSpace::AddToNamedBuckets(TCppDefnBase *defn)
{
	assert(defn->m_defn_key_id != 0);
	assert(defn->m_defn_proxy.IsInList() == FALSE);

	// Bucket is selected based on the key id that was assigned to the name by the mid_lev callback.
	GetNamedBucket(defn->m_defn_key_id)->AppendItem(defn->m_defn_proxy);
}

void TCppDefnsSpace::AddToSpecialBuckets(TCppDefnBase *defn)
{
	assert(defn->m_defn_proxy.IsInList() == FALSE);

	// Prepare the hash value depending on the type of the definition.
	DWORD hash_val;
	switch (defn->m_cib_type)
	{
		case cxx_bit_field_type:
				{
					// Use base type item_id and bit length.
					TCppBitFieldTypeDefn *bft = (TCppBitFieldTypeDefn*)defn;
					hash_val = bft->m_base_type->ItemId() ^ bft->m_bit_length;
				}
				break;

		case cxx_cv_ptr_ref_type:
				{
					// Use base type item_id and number of modification layers.
					TCppCvPtrRefTypeDefn *cvpr = (TCppCvPtrRefTypeDefn*)defn;
					hash_val = cvpr->m_base_type->ItemId() ^ cvpr->m_layers.NumItems();
				}
				break;

		case cxx_array_type:
				{
					// Use base type defn_id and first dimension of the array.
					TCppArrayTypeDefn *arr = (TCppArrayTypeDefn*)defn;
					hash_val = arr->m_base_type->ItemId() ^ (int)arr->m_dimensions[0].m_num_elements;
				}
				break;

		case cxx_function_type:
				{
					// This is definition of the nameless function type. Use the id of the return value type.
					TCppFunctionTypeDefn *func = (TCppFunctionTypeDefn*)defn;
					assert(func->m_spec_type == spcft_none);
					hash_val = func->m_return_value_type->ItemId();
				}
				break;

		case cxx_code_entry_point:
		case cxx_func_template:
		case cxx_func_template_inst:
		case cxx_func_template_spec:
				{
					// Special defns space can contain destructors, overloaded operators and overloaded conversions.
					TCppCodeEntryPointDefn *func = (TCppCodeEntryPointDefn*)defn;
					assert(func->m_spec_type >= spcft_dector);

					if (func->m_spec_type == spcft_dector)
					{
						// Place destructor into the bucket with index zero.
						hash_val = 0;
					}
					else if (func->m_spec_type == spcft_operator)
					{
						// Use operator token value.
						hash_val = func->m_operator_token;
					}
					else
					{
						// Use item_id of the return value type.
						hash_val = func->m_return_value_type->ItemId();
					}
				}
				break;

		case cxx_unresolved_data_type:
		case cxx_unresolved_struct_type:
		case cxx_unresolved_data_field:
		case cxx_unresolved_code_entry_point:
		case cxx_unresolved_class_template:
		case cxx_unresolved_func_template:
				{
					// All these classes base their hash value on the props of their name seqs.
					TList *unres_names_list = defn->GetUnresolvedNamesList();
					assert(unres_names_list != NULL);

					hash_val = TCppUnresolvedNameInfo::GetHashValueForNamesSeq(unres_names_list);
				}
				break;

		default:
			{
				// This method should not be called with any other type of definition.
				assert(FALSE);
				return;
			}
	}

	// Add definition to the appropriate special bucket.
	GetSpecialBucket(hash_val)->AppendItem(defn->m_defn_proxy);
}

//-----------------------------------------------------------------------------
//  ==================  TCppNewDefnsSearchHelper  ====================
//-----------------------------------------------------------------------------

bool TCppNewDefnsSearchHelper::DoSearch(TCppElabType key_elab, TCppNameInfoCore &key_info_core)
{
	// Clear the output variables.
	m_single_defn = NULL;
	m_defns_bucket = NULL;

	// Passed container should contain a nested defns space. Ensure this.
	TCppDefnsSpace *space = m_search_container->GetNestedDefnsSpace();
	assert(space != NULL);

	switch (key_info_core.m_info_type)
	{
		case cnit_identifier:
		case cnit_template_id:
		case cnit_constructor:
		case cnit_ctor_template:
				{
					// Search the named defns bucket. This searcher expects that objects that allow overloading follow
					// the elaboration rules that state that base class specs and functions should always be non elaborated
					// while the class templates (plus specializations and instantiations) should always be elaborated.
					assert(key_info_core.m_key_id != 0);

					TCppDefnsBucket *slot = space->GetNamedBucket(key_info_core.m_key_id);
					if (key_elab == elab_none)
					{
						// Use bucket iterator with the key_id filtering.
						for (TCppDefnsSpaceBucketKeyIdIter<TCppDefnBase> iter(slot, key_info_core.m_key_id); iter; ++iter)
						{
							TCppDefnBase &item = iter.Item();
							if (item.m_defn_elab_type == elab_none)
							{
								// Object with the requested key id and empty elaboration type is present.
								if (item.ItemType() == cxx_base_class_spec || item.IsFunctionFamily() == TRUE)
								{
									// Current object is overloadable. Return this object with its bucket.
									m_single_defn = &item;
									m_defns_bucket = slot;
									return(TRUE);
								}
								else
								{
									// This is non overloadable object. Return this object.
									m_single_defn = &item;
									return(TRUE);
								}
							}
						}
					}
					else
					{
						// Use bucket iterator with the key_id filtering.
						for (TCppDefnsSpaceBucketKeyIdIter<TCppDefnBase> iter(slot, key_info_core.m_key_id); iter; ++iter)
						{
							TCppDefnBase &item = iter.Item();
							if (item.m_defn_elab_type == key_elab)
							{
								// Object with the requested key id and requested non empty elaboration type is present.
								if (item.IsClassTemplateFamily() == TRUE)
								{
									// Current object is overloadable. Return this object with its bucket.
									m_single_defn = &item;
									m_defns_bucket = slot;
									return(TRUE);
								}
								else
								{
									// This is non overloadable object. Return this object.
									m_single_defn = &item;
									return(TRUE);
								}
							}
						}
					}
				}
				break;

		case cnit_dector:
				{
					// Destructor does not have any extra props. It is either present in the defns space or not.
					assert(key_elab == elab_none);

					TCppDefnsBucket *slot = space->GetSpecialBucket(0);
					for (TCppDefnsSpaceBucketDefnTypeIter<TCppCodeEntryPointDefn> iter(slot, cxx_code_entry_point); iter; ++iter)
					{
						if (iter.Item().m_spec_type == spcft_dector)
						{
							// Destructor is a function. This means that it is an overloadable object.
							m_single_defn = iter.ItemPtr();
							m_defns_bucket = slot;
							return(TRUE);
						}
					}
				}
				break;

		case cnit_ovld_operator:
		case cnit_ovld_op_template:
				{
					// Lookup is based on the operation code.
					assert(key_elab == elab_none);

					TOperatorToken op_code = key_info_core.m_ovld_operator_token;
					assert(op_code != 0 && op_code != opr_none);

					TCppDefnsBucket *slot = space->GetSpecialBucket(op_code);
					for (TCppDefnsSpaceBucketIterator iter(slot); iter; ++iter)
					{
						if (iter.Item().IsFunctionFamily() == TRUE)
						{
							TCppCodeEntryPointDefn &func = (TCppCodeEntryPointDefn&)iter.Item();
							if (func.m_spec_type == spcft_operator && func.m_operator_token == op_code)
							{
								// Overloaded operator is a function. This means that it is an overloadable object.
								m_single_defn = iter.ItemPtr();
								m_defns_bucket = slot;
								return(TRUE);
							}
						}
					}
				}
				break;

		case cnit_ovld_conversion:
		case cnit_ovld_conv_template:
				{
					// Lookup is based on the id of the ovld conversion destination type.
					assert(key_elab == elab_none && key_info_core.m_ovld_conversion_dest != NULL);

					ID result_defn_id = key_info_core.m_ovld_conversion_dest->ItemId();
					assert(result_defn_id != 0);

					TCppDefnsBucket *slot = space->GetSpecialBucket(result_defn_id);
					for (TCppDefnsSpaceBucketIterator iter(slot); iter; ++iter)
					{
						if (iter.Item().IsFunctionFamily() == TRUE)
						{
							TCppCodeEntryPointDefn &func = (TCppCodeEntryPointDefn&)iter.Item();
							if (func.m_spec_type == spcft_typecast && func.m_return_value_type->ItemId() == result_defn_id)
							{
								// Overloaded conversion is a function. This means that it is an overloadable object.
								m_single_defn = iter.ItemPtr();
								m_defns_bucket = slot;
								return(TRUE);
							}
						}
					}
				}
				break;

		default:
			// The type of the name is bogus.
			assert(FALSE);
			break;
	}

	// Noting is found.
	return(FALSE);
}

TCppClassTemplateDefn *TCppNewDefnsSearchHelper::FindClassTemplateOverload(ID key_id, TList *template_params_list)
{
	// Ensure that the search query was executed and that it found a bucket of class templates.
	assert(m_single_defn != NULL && m_single_defn->DefnKeyId() == key_id);
	if (m_single_defn->IsClassTemplateFamily() == FALSE)
		return(NULL);

	// Discovered bucket belongs to the class templates group. Iterate it.
	for (TCppDefnsSpaceBucketIterator iter(m_defns_bucket); iter; ++iter)
	{
		if (iter.Item().ItemType() != cxx_class_template || iter.Item().DefnKeyId() != key_id)
			continue;

		TCppClassTemplateDefn &defn = (TCppClassTemplateDefn&)iter.Item();
		if (TCppItemBase::CompareTemplateParamLists(defn.GetNestedMembersList(), template_params_list) == TRUE)
		{
			// Class template with compatible params list is available.
			return(&defn);
		}
	}

	// Requested class template overload is not available.
	return(NULL);
}

TCppCodeEntryPointDefn *TCppNewDefnsSearchHelper::FindCodeEntryPointOverload(TCppNameInfoCore &key_info_core, TCppDeclaratorModifier *func_mdf)
{
	// Ensure that the search query was executed and that it found a bucket of functions.
	assert(m_single_defn != NULL);
	if (m_single_defn->IsFunctionFamily() == FALSE)
		return(NULL);

	// Iterate the discovered bucket.
	for (TCppDefnsSpaceBucketIterator iter(m_defns_bucket); iter; ++iter)
	{
		if (iter.Item().ItemType() != cxx_code_entry_point)
			continue;

		TCppCodeEntryPointDefn &func = (TCppCodeEntryPointDefn&)iter.Item();
		if (func.CompareFunctionNames(key_info_core) == TRUE)
		{
			// The names matched. Compare the lists of params.
			if (func.CompareParameterLists(func_mdf) == TRUE)
				return(&func);
		}
	}

	// Requested code entry point overload is not available.
	return(NULL);
}

TCppFunctionTemplateDefn *TCppNewDefnsSearchHelper::FindFunctionTemplateOverload(TList *template_params_list, TCppNameInfoCore &key_info_core, TCppDeclaratorModifier *func_mdf)
{
	// Ensure that search query was executed and that it found bucket of functions.
	assert(m_single_defn != NULL);
	if (m_single_defn->IsFunctionFamily() == FALSE)
		return(NULL);

	// Iterate the discovered bucket.
	for (TCppDefnsSpaceBucketIterator iter(m_defns_bucket); iter; ++iter)
	{
		if (iter.Item().ItemType() != cxx_func_template)
			continue;

		TCppFunctionTemplateDefn &func_template = (TCppFunctionTemplateDefn&)iter.Item();
		if (TCppItemBase::CompareTemplateParamLists(func_template.GetNestedMembersList(), template_params_list) == TRUE)
		{
			// Function template with compatible list of template params is available.
			if (func_template.CompareFunctionNames(key_info_core) == TRUE && func_template.CompareParameterLists(func_mdf) == TRUE)
				return(&func_template);
		}
	}

	// Requested function template overload is not available.
	return(NULL);
}

int TCppNewDefnsSearchHelper::GetNumberOfOverloads(TCppNameInfoCore &key_info_core, TCppDefnBase *single_defn, TCppDefnsBucket *defns_bucket)
{
	// Check for simple cases first.
	if (single_defn == NULL)
		return(0);
	else if (defns_bucket == NULL)
		return(1);

	// Multiple overloads are possible.
	if (single_defn->IsFunctionFamily() == TRUE)
	{
		// Iterate discovered bucket using the most generic iterator.
		int cnt_func_overloads = 0;
		for (TCppDefnsSpaceBucketIterator iter((TCppDefnsBucket*)defns_bucket); iter; ++iter)
		{
			// Only functions and function templates are considered to be primary objects.
			if (iter.Item().ItemType() == cxx_code_entry_point || iter.Item().ItemType() == cxx_func_template)
			{
				TCppCodeEntryPointDefn &func = (TCppCodeEntryPointDefn&)iter.Item();
				if (func.CompareFunctionNames(key_info_core) == TRUE)
					cnt_func_overloads++;
			}
		}

		// Return the overall result.
		return(cnt_func_overloads);
	}
	else if (single_defn->IsClassTemplateFamily() == TRUE)
	{
		// Count the number of class templates regardless of their elaboration.
		return(defns_bucket->GetNumObjects(cxx_class_template, elab_num_types, key_info_core.m_key_id));
	}
	else if (single_defn->ItemType() == cxx_base_class_spec)
	{
		// Count the number of base classes that share the same name.
		return(defns_bucket->GetNumObjects(cxx_base_class_spec, elab_none, key_info_core.m_key_id));
	}

	// Passed params are bogus.
	assert(FALSE);
	return(0);
}

//------------------------------------------------------------------------------
//  =================  TCppExistingDefnsSearchHelper  ===================
//------------------------------------------------------------------------------

bool TCppExistingDefnsSearchHelper::DoSearch(TCppElabType key_elab, TCppNameInfoCore &key_info, bool search_hier)
{
	// Clear the output variables.
	m_single_defn = NULL;
	m_defns_bucket = NULL;
	m_ambig_defns.Clear();
	m_ambig_defns_buffering_error = FALSE;

	// Move the search key data to the data members of the class. This design is used for perf reasons.
	m_search_key_elab = key_elab;
	m_search_key_info = &key_info;

	m_curr_search_layer = m_search_container;
	TCppSearchHelperContext root_ctx = { m_search_container, NULL };
	if (search_hier == FALSE)
	{
		// Search only one layer.
		SearchDefnsLayer(root_ctx, m_search_container->GetNestedDefnsSpace(), m_search_container->GetAssocDefnsSpaceRefsList());
	}
	else
	{
		// Search the whole nesting hierarchy.
		TCppDefnsSpace *prev_defns_space = NULL;
		while (m_curr_search_layer != NULL)
		{
			// Skip the function headers. They are present between the blocks and owning classes/namespaces.
			if (m_curr_search_layer->IsFunctionFamily() == FALSE)
			{
				TCppDefnsSpace *defns_space = m_curr_search_layer->GetNestedDefnsSpace();

				// The lines below are needed to speed up processing the nameless structures. These structures are using
				// defns tables of their parents. This procedure allows searching these defn spaces only once. Although there
				// are consequencies in case if a nameless structure has a base class. Ignore this aspect for now.
				if (defns_space == prev_defns_space)
					defns_space = NULL;
				else prev_defns_space = defns_space;

				// Search the layer of definitions that consists of the defns space itself plus the hierarchy of the base classes
				// and the list of the associated nameaspaces.
				SearchDefnsLayer(root_ctx, defns_space, m_curr_search_layer->GetAssocDefnsSpaceRefsList());
				if (m_single_defn != NULL || IsAmbigResult() == TRUE)
				{
					// Something was discovered on the current layer.
					break;
				}
			}

			// Shift to the upper layer.
			m_curr_search_layer = (TCppDefnBase*)m_curr_search_layer->ItemParent();
		}
	}

	m_search_key_info = NULL;
	return(m_single_defn != NULL || IsAmbigResult() == TRUE);
}

void TCppExistingDefnsSearchHelper::SearchDefnsLayer(TCppSearchHelperContext &context, TCppDefnsSpace *defns_space, TList *assoc_spaces_list)
{
	if (defns_space != NULL)
	{
		// Search the passed defns space.
		SearchSingleDefnsSpace(defns_space);
	}

	if (assoc_spaces_list != NULL)
	{
		// Search the passed associated list of spaces.
		for (TListIter<TCppAssocDefnsSpaceRef> iter(assoc_spaces_list); iter; ++iter)
		{
			TCppDefnBase *assoc_space_ref = iter.CurrItem().m_assoc_space_ref_object;
			assert(assoc_space_ref != NULL);

			// Resolve association reference object into the space container object.
			TCppDefnBase *assoc_container = NULL;
			switch (assoc_space_ref->ItemType())
			{
				case cxx_base_class_spec:
						{
							// Associated container is the base class.
							assoc_container = ((TCppBaseClassSpec*)assoc_space_ref)->m_base_class;
						}
						break;

				case cxx_using_directive:
						{
							// Associated container is the user defined namespace.
							TCppUsingDirectiveDefn *using_dir_object = (TCppUsingDirectiveDefn*)assoc_space_ref;
							assoc_container = using_dir_object->m_namespace_to_use;
						}
						break;
			}

			if (assoc_container != NULL)
			{
				if (context.SearchContext(assoc_container) == FALSE)
				{
					// This container was not searched yet.
					TCppSearchHelperContext child_ctx = { assoc_container, &context };
					SearchDefnsLayer(child_ctx, assoc_container->GetNestedDefnsSpace(), assoc_container->GetAssocDefnsSpaceRefsList());
				}
			}
		}
	}
}

void TCppExistingDefnsSearchHelper::SearchSingleDefnsSpace(TCppDefnsSpace *defns_space)
{
	switch (m_search_key_info->m_info_type)
	{
		case cnit_identifier:
		case cnit_template_id:
		case cnit_constructor:
		case cnit_ctor_template:
				{
					// Pick up the named defns slot.
					TCppDefnsBucket *slot = defns_space->GetNamedBucket(m_search_key_info->m_key_id);
					if (m_search_key_elab == elab_none)
					{
						// Caller is looking for defns with any elaboration. This means that defns of all elaboration types should be
						// considered. For simplicity of the code the elaborated objects and the non elaborated objects are processed
						// separately. Result of processing elaborated objects is used only when there are no results for unelaborated
						// objects at all.
						TCppDefnBase *candidate = NULL, *elab_candidate = NULL;
						TCppDefnBasePtrsArray ambig_defns, elab_ambig_defns;
						bool ambig_defns_buffering_error = FALSE, elab_ambig_defns_buffering_error = FALSE;

						// Use iterator with the key_id filtering.
						for (TCppDefnsSpaceBucketKeyIdIter<TCppDefnBase> iter(slot, m_search_key_info->m_key_id); iter; ++iter)
						{
							TCppDefnBase *item = iter.ItemPtr();
							if (item->m_defn_elab_type == elab_none)
							{
								// Note that this loop treats base class specs as non overloadable objects that create ambiguity.
								// This is one of the differences between this searcher and the new defn name clearance searcher.
								if (item->IsFunctionFamily() == TRUE)
								{
									// BUGBUG May 2012 - exclude ctors for a while.
									if (((TCppCodeEntryPointDefn*)item)->m_spec_type != spcft_ctor)
									{
										// (1) This is an unelaborated overloadable definition.
										if (ambig_defns.NumItems() > 0)
										{
											// The ambiguity is already present. Figure out if current object is an overload of one
											// of the already discovered ambiguous objects or not.
											bool func_sample_present = FALSE;
											for (int iamb=0; iamb<ambig_defns.NumItems(); ++iamb)
											{
												if (ambig_defns[iamb]->IsFunctionFamily() == TRUE)
												{
													func_sample_present = TRUE;
													break;
												}
											}

											if (func_sample_present = FALSE)
											{
												if (ambig_defns.AppendItem(item) == FALSE)
													ambig_defns_buffering_error = TRUE;
											}
										}
										else if (candidate != NULL)
										{
											if (candidate->IsFunctionFamily() == FALSE)
											{
												ambig_defns.AppendItem(candidate);
												ambig_defns.AppendItem(item);
												candidate = NULL;
											}
										}
										else
										{
											// This is brand new object. Nothing was discovered before.
											candidate = item;
										}
									}
								}
								else
								{
									// (2) This is unelaborated non overloadable definition.
									if (ambig_defns.NumItems() > 0)
									{
										if (ambig_defns.AppendItem(item) == FALSE)
											ambig_defns_buffering_error = TRUE;
									}
									else if (candidate != NULL)
									{
										ambig_defns.AppendItem(candidate);
										ambig_defns.AppendItem(item);
										candidate = NULL;
									}
									else
									{
										// This is brand new object. Nothing was discovered before.
										candidate = item;
									}
								}
							}
							else
							{
								// This is some elaborated object.
								if (item->IsClassTemplateFamily() == TRUE)
								{
									// (3) This is elaborated overloadable definition.
									if (elab_ambig_defns.NumItems() > 0)
									{
										// The ambiguity is already present. Figure out if current object is an overload of one
										// of the already discovered ambiguous objects or not.
										bool compatible_ambig_sample_present = FALSE;
										for (int iamb=0; iamb<elab_ambig_defns.NumItems(); ++iamb)
										{
											if (elab_ambig_defns[iamb]->IsClassTemplateFamily() == TRUE && elab_ambig_defns[iamb]->DefnElabType() == item->DefnElabType())
											{
												compatible_ambig_sample_present = TRUE;
												break;
											}
										}

										if (compatible_ambig_sample_present = FALSE)
										{
											if (elab_ambig_defns.AppendItem(item) == FALSE)
												elab_ambig_defns_buffering_error = TRUE;
										}
									}
									else if (elab_candidate != NULL)
									{
										if (elab_candidate->IsClassTemplateFamily() == FALSE || elab_candidate->DefnElabType() != item->DefnElabType())
										{
											elab_ambig_defns.AppendItem(elab_candidate);
											elab_ambig_defns.AppendItem(item);
											elab_candidate = NULL;
										}
									}
									else
									{
										// This is brand new object. Nothing was discovered before.
										elab_candidate = item;
									}
								}
								else
								{
									// (4) This is elaborated non overloadable defn.
									if (elab_ambig_defns.NumItems() > 0)
									{
										if (elab_ambig_defns.AppendItem(item) == FALSE)
											elab_ambig_defns_buffering_error = TRUE;
									}
									else if (elab_candidate != NULL)
									{
										elab_ambig_defns.AppendItem(elab_candidate);
										elab_ambig_defns.AppendItem(item);
										elab_candidate = NULL;
									}
									else
									{
										// This is brand new object. Nothing was discovered before.
										elab_candidate = item;
									}
								}
							}
						}

						// Sum up the results of the search.
						if (ambig_defns.NumItems() > 0)
						{
							SummarizeDiscoveredAmbiguity(ambig_defns, ambig_defns_buffering_error);
						}
						else if (candidate != NULL)
						{
							SummarizeDiscoveredObject(candidate, (candidate->IsFunctionFamily() == TRUE) ? slot : NULL);
						}
						else if (elab_ambig_defns.NumItems() > 0)
						{
							SummarizeDiscoveredAmbiguity(elab_ambig_defns, elab_ambig_defns_buffering_error);
						}
						else if (elab_candidate != NULL)
						{
							SummarizeDiscoveredObject(elab_candidate, (elab_candidate->IsClassTemplateFamily() == TRUE) ? slot : NULL);
						}
					}
					else
					{
						// Caller is looking for defns with only one specific non empty elaboration. Note that among overloadable
						// objects only the class templates can have non empty elaboration. This means it is ok to check only for this
						// family of objects inside the loop.
						for (TCppDefnsSpaceBucketKeyIdIter<TCppDefnBase> iter(slot, m_search_key_info->m_key_id); iter; ++iter)
						{
							TCppDefnBase &item = iter.Item();
							if (item.m_defn_elab_type == m_search_key_elab)
							{
								// Object with requested key id and elaboration type is present.
								if (item.IsClassTemplateFamily() == TRUE)
								{
									// Current object is overloadable.
									SummarizeDiscoveredObject(iter.ItemPtr(), slot);
									break;
								}
								else
								{
									// This is non overloadable object.
									SummarizeDiscoveredObject(iter.ItemPtr(), NULL);
									break;
								}
							}
						}
					}
				}
				break;

		case cnit_dector:
				{
					// Destructor does not have any extra props. It is either present or not.
					assert(m_search_key_elab == elab_none);
					TCppDefnsBucket *slot = defns_space->GetSpecialBucket(0);
					for (TCppDefnsSpaceBucketDefnTypeIter<TCppCodeEntryPointDefn> iter(slot, cxx_code_entry_point); iter; ++iter)
					{
						if (iter.Item().m_spec_type == spcft_dector)
						{
							// Destructor is a function. This means it is a flavor of overloadable objects.
							SummarizeDiscoveredObject(iter.ItemPtr(), slot);
							break;
						}
					}
				}
				break;

		case cnit_ovld_operator:
		case cnit_ovld_op_template:
				{
					// Lookup is based on the operation code.
					assert(m_search_key_elab == elab_none);
					TCppDefnsBucket *slot = defns_space->GetSpecialBucket(m_search_key_info->m_ovld_operator_token);
					for (TCppDefnsSpaceBucketIterator iter(slot); iter; ++iter)
					{
						if (iter.Item().IsFunctionFamily() == TRUE)
						{
							TCppCodeEntryPointDefn &func = (TCppCodeEntryPointDefn&)iter.Item();
							if (func.m_spec_type == spcft_operator && func.m_operator_token == m_search_key_info->m_ovld_operator_token)
							{
								SummarizeDiscoveredObject(iter.ItemPtr(), slot);
								break;
							}
						}
					}
				}
				break;

		case cnit_ovld_conversion:
		case cnit_ovld_conv_template:
				{
					// Lookup is based on the id of the destination type.
					assert(m_search_key_elab == elab_none && m_search_key_info->m_ovld_conversion_dest != NULL);
					ID result_defn_id = m_search_key_info->m_ovld_conversion_dest->ItemId();
					assert(result_defn_id != 0);

					// Search the appropriate hash bucket.
					TCppDefnsBucket *slot = defns_space->GetSpecialBucket(result_defn_id);
					for (TCppDefnsSpaceBucketIterator iter(slot); iter; ++iter)
					{
						if (iter.Item().IsFunctionFamily() == TRUE)
						{
							TCppCodeEntryPointDefn &func = (TCppCodeEntryPointDefn&)iter.Item();
							if (func.m_spec_type == spcft_typecast && func.m_return_value_type->ItemId() == result_defn_id)
							{
								SummarizeDiscoveredObject(iter.ItemPtr(), slot);
								break;
							}
						}
					}
				}
				break;

		default:
			assert(FALSE);
			break;
	}
}

void TCppExistingDefnsSearchHelper::SummarizeDiscoveredObject(TCppDefnBase *single_defn, TCppDefnsBucket *defns_bucket)
{
	assert(single_defn != NULL);
	if (IsAmbigResult() == TRUE)
	{
		// Add current discovery to the already existing list of ambiguities.
		if (m_ambig_defns.AppendItem(single_defn) == FALSE)
			m_ambig_defns_buffering_error = TRUE;
	}
	else if (m_single_defn != NULL)
	{
		// Current discovery creates ambiguity with the already existing result. Note that it is ok not to check
		// the result of appending here because ambig defns buffer has non empty initial storage.
		m_ambig_defns.AppendItem(m_single_defn);
		m_ambig_defns.AppendItem(single_defn);
		m_single_defn = NULL;
		m_defns_bucket = NULL;
	}
	else
	{
		// Move current discovery into the data fields of the class.
		m_single_defn = single_defn;
		m_defns_bucket = defns_bucket;
	}
}

void TCppExistingDefnsSearchHelper::SummarizeDiscoveredAmbiguity(TCppDefnBasePtrsArray &ambig_defns, bool ambig_defns_buffering_error)
{
	if (m_single_defn != NULL)
	{
		// Add the already existing discovery to the list of ambiguities.
		m_ambig_defns.AppendItem(m_single_defn);

		m_single_defn = NULL;
		m_defns_bucket = NULL;
	}

	// Append new ambiguities to the existing list of ambiguities.
	for (int iamb=0; iamb<ambig_defns.NumItems(); ++iamb)
	{
		if (m_ambig_defns.AppendItem(ambig_defns[iamb]) == FALSE)
			m_ambig_defns_buffering_error = TRUE;
	}

	if (ambig_defns_buffering_error == TRUE)
	{
		// Buffering error somehow miraculously happened.
		m_ambig_defns_buffering_error = TRUE;
	}
}

//-----------------------------------------------------------------------------
//  ====================  TCppBuiltInTypeDefn  ======================
//-----------------------------------------------------------------------------

static const wchar_t *g_CppBuiltInTypeName[bltin_ids_max_plus1-bltin_ids_base] =
{
	L"BuiltInTypeIdsBase",
	L"void",				L"bool",
	L"__schar8",			L"__uchar8",
	L"__schar16",			L"__uchar16",
	L"__int8",				L"__uint8",
	L"__int16",			L"__uint16",
	L"__int32",			L"__uint32",
	L"__int64",			L"__uint64",
	L"__float4",			L"__double8",
	L"__double10",		L"__ellipses",
};

static TCppBuiltInTypeType g_CppBuiltInTypeType[bltin_ids_max_plus1-bltin_ids_base] =
{
	bltin_type_void,								// Built in types base.
	bltin_type_void,		bltin_type_bool,			// void, bool.
	bltin_type_char,		bltin_type_char,			// char8.
	bltin_type_char,		bltin_type_char,			// char16.
	bltin_type_int,			bltin_type_int,				// int8.
	bltin_type_int,			bltin_type_int,				// int16.
	bltin_type_int,			bltin_type_int,				// int32.
	bltin_type_int,			bltin_type_int,				// int64.
	bltin_type_float,		bltin_type_float,			// float, double.
	bltin_type_float,		bltin_type_ellips,			// double10, ellipses.
};

static bool g_CppBuiltInTypeUnsignedProp[bltin_ids_max_plus1-bltin_ids_base] =
{
	TRUE,				// Base.
	TRUE,	TRUE,		// void, bool.
	FALSE,	TRUE,		// char8.
	FALSE,	TRUE,		// char16.
	FALSE,	TRUE,		// int8.
	FALSE,	TRUE,		// int16.
	FALSE,	TRUE,		// int32.
	FALSE,	TRUE,		// int64.
	FALSE,	FALSE,		// float, double.
	FALSE,	TRUE,		// long double, ellipses.
};

static short g_CppBuiltInTypeLength[bltin_ids_max_plus1-bltin_ids_base] =
{
	0,					// Base.
	0,	1,				// void, bool.
	1,	1,				// char8.
	2,	2,				// char16.
	1,	1,				// int8.
	2,	2,				// int16.
	4,	4,				// int32.
	8,	8,				// int64.
	4,	8,				// float, double.
	16,	0,				// long double, ellipses.
};

static const wchar_t *g_CppBuiltInTypeTypeEnumNames[bltin_num_types] =
{
	L"bltin_type_void",
	L"bltin_type_bool",
	L"bltin_type_char",
	L"bltin_type_int",
	L"bltin_type_float",
	L"bltin_type_ellips",
};

void TCppBuiltInTypeDefn::SetBuiltInTypeProps(TCppBuiltInTypeIdent ident)
{
	// Objects with the built in type should have ids only from the certain range.
	assert(ident > bltin_ids_base && ident < bltin_ids_max_plus1);

	m_cib_id = ident;
	SetNameInfo(g_CppBuiltInTypeName[ident-bltin_ids_base], 0, cdpr_none);

	m_type = g_CppBuiltInTypeType[ident-bltin_ids_base];
	m_unsigned = g_CppBuiltInTypeUnsignedProp[ident-bltin_ids_base];
	m_byte_length = g_CppBuiltInTypeLength[ident-bltin_ids_base];
}

const wchar_t *TCppBuiltInTypeDefn::GetBuiltInTypeFriendlyName(TCppBuiltInTypeIdent ident)
{
	if (ident < bltin_ids_base || ident >= bltin_ids_max_plus1)
		return(L"BogusBuiltInTypeIdent");

	return(g_CppBuiltInTypeName[ident-bltin_ids_base]);
}

const wchar_t *TCppBuiltInTypeDefn::GetBuiltInTypeTypeEnumName(TCppBuiltInTypeType val)
{
	if (val < 0 || val >= bltin_num_types)
		return(L"BogusBuiltInTypeType");

	return(g_CppBuiltInTypeTypeEnumNames[val]);
}

//-----------------------------------------------------------------------------
//  ===================  TCppBitFieldTypeDefn  =======================
//-----------------------------------------------------------------------------

void TCppBitFieldTypeDefn::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into data field.
	assert(item_destination == cdefn_chdest_default);
	if (m_bit_length_expr != NULL)
		delete m_bit_length_expr;

	assert(item->IsExpression() == TRUE);
	m_bit_length_expr = (TCppExpressionBase*)item;

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  ====================  TCppArrayTypeDefn  =======================
//-----------------------------------------------------------------------------

void TCppArrayTypeDefn::AddChildItem(TCppItemBase *item, int item_destination_layer_index)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Accept the pointer.
	assert(item_destination_layer_index >= 0 && item_destination_layer_index < m_dimensions.NumItems());
	TCppArrayDimensionSpec &dim_spec = m_dimensions.ItemRef(item_destination_layer_index);
	if (dim_spec.m_num_elements_expr != NULL)
		delete dim_spec.m_num_elements_expr;

	assert(item->IsExpression() == TRUE);
	dim_spec.m_num_elements_expr = (TCppExpressionBase*)item;

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppArrayTypeDefn::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_special_count = 0;
	FindNextChildInternal(iter);
}

void TCppArrayTypeDefn::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	iter.m_curr_child = NULL;
	if (curr_chld == NULL)
	{
		iter.m_special_count = m_dimensions.NumItems();
		return;
	}

	// Look for dimension that contains the passed item.
	iter.m_special_count = 0;
	while (iter.m_special_count < m_dimensions.NumItems())
	{
		if (m_dimensions[iter.m_special_count].m_num_elements_expr == curr_chld)
		{
			iter.m_curr_child = curr_chld;
			return;
		}

		iter.m_special_count++;
	}
}

void TCppArrayTypeDefn::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_special_count < m_dimensions.NumItems())
	{
		iter.m_special_count++;
		FindNextChildInternal(iter);
	}
}

void	TCppArrayTypeDefn::FindNextChildInternal(TCppDirectChildrenIter &iter) const
{
	while (iter.m_special_count < m_dimensions.NumItems())
	{
		if (m_dimensions[iter.m_special_count].m_num_elements_expr != NULL)
		{
			// Some non NULL child is available.
			iter.m_curr_child = m_dimensions[iter.m_special_count].m_num_elements_expr;
			return;
		}

		iter.m_special_count++;
	}

	// There are no more children in the description of the array.
	iter.m_curr_child = NULL;
}

//-----------------------------------------------------------------------------
//  =====================  TCppEnumTypeDefn  ======================
//-----------------------------------------------------------------------------

void TCppEnumTypeDefn::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->ItemType() == cxx_enum_member);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	assert(item_destination == cdefn_chdest_default);
	m_members.AppendItem(item);

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

int TCppEnumTypeDefn::GetNumEnumMembers() const
{
	int cnt_members = 0;
	for (TListIter<TCppItemBase> iter((TList&)m_members); iter; ++iter)
	{
		if (iter.CurrItem().ItemType() == cxx_enum_member)
			cnt_members++;
	}

	return(cnt_members);
}

//-----------------------------------------------------------------------------
//  ====================  TCppEnumMemberDefn  =====================
//-----------------------------------------------------------------------------

void TCppEnumMemberDefn::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into data field.
	assert(item_destination == cdefn_chdest_default);
	if (m_value_expr != NULL)
		delete m_value_expr;

	assert(item->IsExpression() == TRUE);
	m_value_expr = (TCppExpressionBase*)item;

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  =====================  TCppStructTypeDefn  ======================
//-----------------------------------------------------------------------------

void TCppStructTypeDefn::AddBaseClassesList(TList *base_classes_list)
{
	while (base_classes_list->IsEmpty() == FALSE)
	{
		TCppBaseClassSpec *spec = (TCppBaseClassSpec*)base_classes_list->GetFirst();
		assert(spec->ItemType() == cxx_base_class_spec);

		m_assoc_defn_spaces.AppendItem(spec->m_assoc_ref);

		base_classes_list->RemoveItem(spec);
		AddChildItem(spec);
	}
}

int TCppStructTypeDefn::GetNumTemplateParams() const
{
	int cnt_params = 0;
	for (TListIter<TCppItemBase> iter((TList&)m_members); iter; ++iter)
	{
		if (IsTemplateParam(iter.CurrItemPtr()) == TRUE)
			cnt_params++;
	}

	return(cnt_params);
}

//-----------------------------------------------------------------------------
//  =====================  TCppFieldAlignSpec  ======================
//-----------------------------------------------------------------------------

void TCppFieldAlignSpec::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into data field.
	assert(item_destination == cdefn_chdest_default);
	if (m_align_value_expr != NULL)
		delete m_align_value_expr;

	assert(item->IsExpression() == TRUE);
	m_align_value_expr = (TCppExpressionBase*)item;

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  ======================  TCppDataFieldDefn  ======================
//-----------------------------------------------------------------------------

static const wchar_t *g_CppAccessLevelName[accs_num_types] =
{
	L"public", L"protected", L"private",
};

static const wchar_t *g_CppStorageClassFriendlyName[strgc_num_types] =
{
	L"strgc_none", L"register", L"auto", L"static", L"extern", L"mutable", L"strgc_param", L"strgc_template_param", L"strgc_catch_block_param",
};

static const wchar_t *g_CppStorageClassEnumName[strgc_num_types] =
{
	L"strgc_none", L"strgc_register", L"strgc_auto", L"strgc_static", L"strgc_extern", L"strgc_mutable", L"strgc_param", L"strgc_template_param", L"strgc_catch_block_param",
};

void TCppDataFieldDefn::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	assert(item_destination == cdefn_chdest_dtfld_init);
	if (m_field_initializer != NULL)
		delete m_field_initializer;

	assert(item->ItemType() == cxx_data_field_initializer || item->IsExpression() == TRUE);
	m_field_initializer = item;

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

const wchar_t *TCppDataFieldDefn::GetAccessLevelFriendlyName(TCppAccessLevel accs)
{
	if (accs < 0 || accs >= accs_num_types)
		return(L"BogusAccessLevelValue");

	return(g_CppAccessLevelName[accs]);
}

const wchar_t *TCppDataFieldDefn::GetStorageClassFriendlyName(TCppStorageClass strgc)
{
	if (strgc < 0 || strgc >= strgc_num_types)
		return(L"BogusStorageClassValue");

	return(g_CppStorageClassFriendlyName[strgc]);
}

const wchar_t *TCppDataFieldDefn::GetStorageClassEnumName(TCppStorageClass strgc)
{
	if (strgc < 0 || strgc >= strgc_num_types)
		return(L"BogusStorageClassValue");

	return(g_CppStorageClassEnumName[strgc]);
}

//-----------------------------------------------------------------------------
//  ==================  TCppDataFieldInitializer  =======================
//-----------------------------------------------------------------------------

static const wchar_t *g_CppInitalizerTypeEnumName[cinit_num_types] =
{
	L"cinit_none", L"cinit_single_expr", L"cinit_exprs_list", L"cinit_aggregate",
};

void TCppDataFieldInitializer::AddChildItem(TCppItemBase *item, int item_destination)
{
	// The passed child should be either other initializer or some sort of the expression.
	assert(item != NULL && item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->ItemType() == cxx_data_field_initializer || item->IsExpression() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	assert(item_destination == cdefn_chdest_default);
	m_init_elements.AppendItem(item);

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

const wchar_t *TCppDataFieldInitializer::GetInitializerTypeEnumName(TCppInitalizerType init_type)
{
	if (init_type < 0 || init_type >= cinit_num_types)
		return(L"BogusDataFieldInitializerTypeValue");

	return(g_CppInitalizerTypeEnumName[init_type]);
}

//-----------------------------------------------------------------------------
//  ===================  TCppFunctionTypeDefn  ======================
//-----------------------------------------------------------------------------

static const wchar_t *g_CppSpecFuncTypeEnumName[spcft_num_types] =
{
	L"spcft_none", L"spcft_ctor", L"spcft_dector", L"spcft_operator", L"spcft_typecast",
};

bool TCppFunctionTypeDefn::IsParamItem(TCppItemBase *item)
{
	if (item->ItemType() == cxx_data_field)
	{
		TCppDataFieldDefn *data_field = (TCppDataFieldDefn*)item;
		if (data_field->m_strg_class == strgc_param)
			return(TRUE);
	}

	// All other objects are not function params.
	return(FALSE);
}

int TCppFunctionTypeDefn::GetNumParams() const
{
	int cnt_params = 0;
	for (TListIter<TCppItemBase> iter((TList&)m_members); iter; ++iter)
	{
		if (IsParamItem(iter.CurrItemPtr()) == TRUE)
			cnt_params++;
	}

	return(cnt_params);
}

TCppDataFieldDefn *TCppFunctionTypeDefn::GetFirstParam() const
{
	TCppItemBase *param = (TCppItemBase*)m_members.GetFirst();
	while (param != NULL && IsParamItem(param) == FALSE)
		param = (TCppItemBase*)m_members.GetNext(param);
	return((TCppDataFieldDefn*)param);
}

TCppDataFieldDefn *TCppFunctionTypeDefn::GetNextParam(TCppDataFieldDefn *curr_param) const
{
	TCppItemBase *param = (TCppItemBase*)m_members.GetNext(curr_param);
	while (param != NULL && IsParamItem(param) == FALSE)
		param = (TCppItemBase*)m_members.GetNext(param);
	return((TCppDataFieldDefn*)param);
}

int TCppFunctionTypeDefn::GetNumThrowSpecs() const
{
	int cnt_throw_specs = 0;
	for (TListIter<TCppItemBase> iter((TList&)m_members); iter; ++iter)
	{
		if (iter.CurrItem().ItemType() == cxx_possible_exception_spec)
			cnt_throw_specs++;
	}

	return(cnt_throw_specs);
}

TCppPossibleExceptionSpec *TCppFunctionTypeDefn::GetFirstThrowSpec() const
{
	TCppItemBase *throw_spec = (TCppItemBase*)m_members.GetFirst();
	while (throw_spec != NULL && throw_spec->ItemType() != cxx_possible_exception_spec)
		throw_spec = (TCppItemBase*)m_members.GetNext(throw_spec);
	return((TCppPossibleExceptionSpec*)throw_spec);
}

TCppPossibleExceptionSpec *TCppFunctionTypeDefn::GetNextThrowSpec(TCppPossibleExceptionSpec *curr_throw_spec) const
{
	TCppItemBase *throw_spec = (TCppItemBase*)m_members.GetNext(curr_throw_spec);
	while (throw_spec != NULL && throw_spec->ItemType() != cxx_possible_exception_spec)
		throw_spec = (TCppItemBase*)m_members.GetNext(throw_spec);
	return((TCppPossibleExceptionSpec*)throw_spec);
}

bool TCppFunctionTypeDefn::CompareFunctionNames(TCppNameInfoCore &name_info) const
{
	switch (name_info.m_info_type)
	{
		case cnit_identifier:
		case cnit_template_id:
				{
					if (m_spec_type == spcft_none && DefnKeyId() == name_info.m_key_id)
						return(TRUE);
				}
		case cnit_constructor:
		case cnit_ctor_template:
				{
					if (m_spec_type == spcft_ctor && DefnKeyId() == name_info.m_key_id)
						return(TRUE);
				}
				break;

		case cnit_dector:
				{
					if (m_spec_type == spcft_dector)
						return(TRUE);
				}
				break;

		case cnit_ovld_operator:
		case cnit_ovld_op_template:
				{
					if (m_spec_type == spcft_operator && m_operator_token == name_info.m_ovld_operator_token)
						return(TRUE);
				}
				break;

		case cnit_ovld_conversion:
		case cnit_ovld_conv_template:
				{
					if (m_spec_type == spcft_typecast && m_return_value_type != NULL && m_return_value_type->ItemId() == name_info.m_ovld_conversion_dest->ItemId())
						return(TRUE);
				}
				break;

		default:
			assert(FALSE);
			break;
	}

	// The names are different.
	return(FALSE);
}

bool TCppFunctionTypeDefn::CompareParameterLists(TCppDeclaratorModifier *func_mdf_layer) const
{
	if (func_mdf_layer == NULL)
	{
		// Missing func mdfs info is compatible with function without params.
		return(GetNumParams() == 0);
	}

	assert(func_mdf_layer->m_dmt == dclmdf_function);
	if (func_mdf_layer->m_func_spec.GetNumParams() != GetNumParams())
	{
		// The number of params is different.
		return(FALSE);
	}

	TCppDataFieldDefn *local_prm = GetFirstParam();
	TCppDataFieldDefn *extern_prm = func_mdf_layer->m_func_spec.GetFirstParam();
	while (local_prm != NULL && extern_prm != NULL)
	{
		// Compare only the types of params.
		if (TCppItemBase::CompareSameDatabaseDataTypes(local_prm->m_field_type, extern_prm->m_field_type) == FALSE)
			return(FALSE);

		// Shift to the next pair of params if any.
		local_prm = GetNextParam(local_prm);
		extern_prm = func_mdf_layer->m_func_spec.GetNextParam(extern_prm);
	}

	// List of params are identical.
	return(TRUE);
}

bool TCppFunctionTypeDefn::CompareFunctionTypes(TCppFunctionTypeDefn *func_type1, TCppFunctionTypeDefn *func_type2, bool compare_param_lists)
{
	if (func_type1->m_spec_type != func_type2->m_spec_type)
		return(FALSE);

	// Compare the names of the functions.
	switch (func_type1->m_spec_type)
	{
		case spcft_operator:
				{
					if (func_type1->m_operator_token != func_type2->m_operator_token)
						return(FALSE);
				}
				break;

		case spcft_typecast:
				{
					if (CompareSameDatabaseDataTypes(func_type1->m_return_value_type, func_type2->m_return_value_type) == FALSE)
						return(FALSE);
				}
				break;

		default:
			{
				if (func_type1->DefnKeyId() != func_type2->DefnKeyId())
					return(FALSE);
			}
			break;
	}

	if (compare_param_lists == TRUE && CompareSameDatabaseFunctionTypes(func_type1, func_type2, FALSE) == FALSE)
		return(FALSE);

	// The passed objects describe identical function types.
	return(TRUE);
}

void TCppFunctionTypeDefn::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	assert(item_destination == cdefn_chdest_default);
	m_members.AppendItem(item);

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

const wchar_t *TCppFunctionTypeDefn::GetSpecFuncTypeEnumName(TCppSpecFuncType spcft)
{
	if (spcft < 0 || spcft >= spcft_num_types)
		return(L"BogusSpecFuncTypeValue");

	return(g_CppSpecFuncTypeEnumName[spcft]);
}

//-----------------------------------------------------------------------------
//  ==================  TCppCodeEntryPointDefn  ======================
//-----------------------------------------------------------------------------

int TCppCodeEntryPointDefn::GetNumTemplateParams() const
{
	int cnt_params = 0;
	for (TListIter<TCppItemBase> iter((TList&)m_members); iter; ++iter)
	{
		if (IsTemplateParam(iter.CurrItemPtr()) == TRUE)
			cnt_params++;
	}

	return(cnt_params);
}

//-----------------------------------------------------------------------------
//  =================  TCppGlobalNamespaceDefn  =====================
//-----------------------------------------------------------------------------

void TCppGlobalNamespaceDefn::AddChildItem(TCppItemBase *item, int item_destination)
{
	if (item == NULL)
		return;

	// Note that implementation of this method is VERY similar to the DefnBase::AddToMembersListAndDefnsSpace().
	// The primary reason why AddToMembersListAndDefnsSpace() cannot be used for this class is because this object
	// does not have parent to send notification to. This class should send ntf to the database object.
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item_destination == cdefn_chdest_default);

	// Add to the local list.
	item->m_cib_parent = this;
	m_members.AppendItem(item);

	if (item->IsDefinition() == TRUE && ((TCppDefnBase*)item)->m_defn_proxy.IsInList() == FALSE)
	{
		// Passed item is definition and it is not part of the defintions space.
		m_defns_table.AddRegularDefinition((TCppDefnBase*)item);
	}

	// Call the database owner to assign the missing ids and add new items to the pointers index.
	// Note that notification is not sent to the upper parent here.
	m_owner_database->ProcessNewChildNotification(item);
}

void TCppGlobalNamespaceDefn::NewIndirectChildNotification(TCppItemBase *items_subtree_root)
{
	// This method cannot be implemented inline because it is using contents of the cpp database.
	m_owner_database->ProcessNewChildNotification(items_subtree_root);
}

void TCppGlobalNamespaceDefn::Clear()
{
	m_members.DeleteAll();
	m_defns_table.Clear();
}

//-----------------------------------------------------------------------------
//  ====================  TCppLinkageSpec  =========================
//-----------------------------------------------------------------------------

bool TCppLinkageSpec::CompareLinkageSpecs(const TCppLinkageSpec *spec1, const TCppLinkageSpec *spec2)
{
	if (spec1 == NULL && spec2 == NULL)
	{
		return(TRUE);
	}
	else if (spec1 != NULL && spec2 != NULL)
	{
		return(spec1->m_spec_key_id == spec2->m_spec_key_id);
	}

	// One spec is present while the other spec is missing.
	return(FALSE);
}

//-------------------------------------------------------------------------------
//  ===============  TCppClassTemplateSpecialization  =====================
//-------------------------------------------------------------------------------

void TCppClassTemplateSpecialization::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	assert(item_destination == ctplt_chdest_default);
	m_specialization_params.AppendItem(item);

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppClassTemplateSpecialization::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	// Iterate specialization params first.
	iter.m_special_count = 0;
	iter.m_curr_child = (TCppItemBase*)m_specialization_params.GetFirst();

	if (iter.m_curr_child == NULL)
	{
		// Iterate regular template members.
		iter.m_special_count = 1;
		iter.m_curr_child = (TCppItemBase*)m_members.GetFirst();
	}
}

void TCppClassTemplateSpecialization::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
	else if (m_specialization_params.IsItemInList(curr_chld) == TRUE)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (m_members.IsItemInList(curr_chld) == TRUE)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppClassTemplateSpecialization::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			// Pick up the next specialization param.
			iter.m_curr_child = (TCppItemBase*)m_specialization_params.GetNext(iter.m_curr_child);
			if (iter.m_curr_child == NULL)
			{
				// Start regular template members iteration.
				iter.m_special_count = 1;
				iter.m_curr_child = (TCppItemBase*)m_members.GetFirst();
			}
		}
		else
		{
			// Pick up the next item from the list of the class members.
			iter.m_curr_child = (TCppItemBase*)m_members.GetNext(iter.m_curr_child);
			if (iter.m_curr_child == NULL)
				iter.m_special_count = 2;
		}
	}
}

//-----------------------------------------------------------------------------
//  =================  TCppClassTemplateInstantiation  ==================
//-----------------------------------------------------------------------------

void TCppClassTemplateInstantiation::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	if (item_destination == cdefn_chdest_cltplt_iprm)
	{
		// Add passed pointer into the list.
		m_instantiation_arguments.AppendItem(item);

		// Send notification to the parent if any.
		if (m_cib_parent != NULL)
			m_cib_parent->NewIndirectChildNotification(item);
	}
	else
	{
		// Pass this call to the parent class.
		assert(item_destination == cdefn_chdest_default);
		TCppStructTypeDefn::AddChildItem(item, item_destination);
	}
}

void TCppClassTemplateInstantiation::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	// Start with iteration of the list of the instantiation params.
	iter.m_special_count = 0;
	iter.m_curr_child = (TCppItemBase*)m_instantiation_arguments.GetFirst();

	if (iter.m_curr_child == NULL)
	{
		// For this or that reason the list of instantiation params is empty.
		iter.m_special_count = 1;
		iter.m_curr_child = (TCppItemBase*)m_members.GetFirst();
	}
}

void TCppClassTemplateInstantiation::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
	else if (m_instantiation_arguments.IsItemInList(curr_chld) == TRUE)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (m_members.IsItemInList(curr_chld) == TRUE)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppClassTemplateInstantiation::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			// Pick up the next instantiation param.
			iter.m_curr_child = (TCppItemBase*)m_instantiation_arguments.GetNext(iter.m_curr_child);
			if (iter.m_curr_child == NULL)
			{
				// Continue with iteration of the second list.
				iter.m_special_count = 1;
				iter.m_curr_child = (TCppItemBase*)m_members.GetFirst();
			}
		}
		else
		{
			// Pick up the next item from the list of the class members.
			iter.m_curr_child = (TCppItemBase*)m_members.GetNext(iter.m_curr_child);
			if (iter.m_curr_child == NULL)
				iter.m_special_count = 2;
		}
	}
}

//------------------------------------------------------------------------------
//  ==============  TCppFunctionTemplateSpecialization  ====================
//------------------------------------------------------------------------------

void TCppFunctionTemplateSpecialization::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	assert(item_destination == ctplt_chdest_default);
	m_specialization_params.AppendItem(item);

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppFunctionTemplateSpecialization::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	// Iterate specialization params first.
	iter.m_special_count = 0;
	iter.m_curr_child = (TCppItemBase*)m_specialization_params.GetFirst();

	if (iter.m_curr_child == NULL)
	{
		// Iterate the function type members.
		iter.m_special_count = 1;
		iter.m_curr_child = (TCppItemBase*)m_members.GetFirst();
	}
}

void TCppFunctionTemplateSpecialization::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
	else if (m_specialization_params.IsItemInList(curr_chld) == TRUE)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (m_members.IsItemInList(curr_chld) == TRUE)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppFunctionTemplateSpecialization::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			// Pick up the next specialization param.
			iter.m_curr_child = (TCppItemBase*)m_specialization_params.GetNext(iter.m_curr_child);
			if (iter.m_curr_child == NULL)
			{
				// Start regular function type members iteration.
				iter.m_special_count = 1;
				iter.m_curr_child = (TCppItemBase*)m_members.GetFirst();
			}
		}
		else
		{
			// Pick up the next item from the list of the function type members.
			iter.m_curr_child = (TCppItemBase*)m_members.GetNext(iter.m_curr_child);
			if (iter.m_curr_child == NULL)
				iter.m_special_count = 2;
		}
	}
}

//-----------------------------------------------------------------------------
//  ================  TCppFunctionTemplateInstantiation  ==================
//-----------------------------------------------------------------------------

void TCppFunctionTemplateInstantiation::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	if (item_destination == cdefn_chdest_fntplt_iprm)
	{
		// Add passed pointer into the list.
		m_instantiation_arguments.AppendItem(item);

		// Send notification to the parent if any.
		if (m_cib_parent != NULL)
			m_cib_parent->NewIndirectChildNotification(item);
	}
	else
	{
		// Pass this call to the parent class.
		assert(item_destination == cdefn_chdest_default);
		TCppCodeEntryPointDefn::AddChildItem(item, item_destination);
	}
}

void TCppFunctionTemplateInstantiation::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	// Start with iteration of the list of the instantiation params.
	iter.m_special_count = 0;
	iter.m_curr_child = (TCppItemBase*)m_instantiation_arguments.GetFirst();

	if (iter.m_curr_child == NULL)
	{
		// For this or that reason the list of instantiation params is empty.
		iter.m_special_count = 1;
		iter.m_curr_child = (TCppItemBase*)m_members.GetFirst();
	}
}

void TCppFunctionTemplateInstantiation::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
	else if (m_instantiation_arguments.IsItemInList(curr_chld) == TRUE)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (m_members.IsItemInList(curr_chld) == TRUE)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppFunctionTemplateInstantiation::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			// Pick up the next instantiation param.
			iter.m_curr_child = (TCppItemBase*)m_instantiation_arguments.GetNext(iter.m_curr_child);
			if (iter.m_curr_child == NULL)
			{
				// Continue with iteration of the second list.
				iter.m_special_count = 1;
				iter.m_curr_child = (TCppItemBase*)m_members.GetFirst();
			}
		}
		else
		{
			// Pick up the next item from the list of the function type members.
			iter.m_curr_child = (TCppItemBase*)m_members.GetNext(iter.m_curr_child);
			if (iter.m_curr_child == NULL)
				iter.m_special_count = 2;
		}
	}
}

//-----------------------------------------------------------------------------
//  ==============  TCppTemplateTemplateParamDefn  ====================
//-----------------------------------------------------------------------------

void TCppTemplateTemplateParamDefn::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	assert(item_destination == ctplt_chdest_default);
	m_template_params.AppendItem(item);

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  ================  TCppTemplateTemplateArgument  ===================
//-----------------------------------------------------------------------------

static const wchar_t *g_CppTemplateTemplateArgumentTypeEnumName[TCppTemplateTemplateArgument::ctarg_num_types] =
{
	L"ctarg_none", L"ctarg_template_defn", L"ctarg_template_template_param", L"ctarg_class_templates_bucket",
};

const wchar_t *TCppTemplateTemplateArgument::GetReferencedDefnName()
{
	switch (m_arg_type)
	{
		case ctarg_template_defn:
				return((m_argument_value != NULL) ? m_argument_value->DefnName() : NULL);

		case ctarg_template_template_param:
				return((m_template_param != NULL) ? m_template_param->DefnName() : NULL);
	}

	// In all other cases show the subtype of the object.
	return(GetArgTypeEnumName(m_arg_type));
}

const wchar_t *TCppTemplateTemplateArgument::GetArgTypeEnumName(TCppTemplTemplArgType ctarg)
{
	if (ctarg < 0 || ctarg >= ctarg_num_types)
		return(L"BogusTemplateTemplateArgumentTypeValue");

	return(g_CppTemplateTemplateArgumentTypeEnumName[ctarg]);
}

//-----------------------------------------------------------------------------
//  ===================  TCppUnresolvedNameInfo  ====================
//-----------------------------------------------------------------------------

static const wchar_t *g_CppNameInfoTypeEnumNames[cnit_num_types] =
{
	L"cnit_none",
	L"cnit_identifier",
	L"cnit_template_id",
	L"cnit_constructor",
	L"cnit_ctor_template",
	L"cnit_dector",
	L"cnit_ovld_operator",
	L"cnit_ovld_op_template",
	L"cnit_ovld_conversion",
	L"cnit_ovld_conv_template",
};

void TCppUnresolvedNameInfo::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Allowed children are those item typess that can be present in the template args list.
	assert(item->ItemType() == cxx_template_type_argument || item->ItemType() == cxx_template_template_argument || item->IsExpression() == TRUE);

	// Setup the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	assert(item_destination == cunr_chdest_default);
	m_template_id_args.AppendItem(item);

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

const wchar_t *TCppUnresolvedNameInfo::GetNameInfoTypeEnumName(TCppNameInfoType nitp)
{
	if (nitp < 0 || nitp >= cnit_num_types)
		return(L"BogusNameInfoType");

	return(g_CppNameInfoTypeEnumNames[nitp]);
}

bool TCppUnresolvedNameInfo::CompareNameInfoSequencies(TList *list1, TList *list2)
{
	if (list1 == NULL || list2 == NULL)
		return(FALSE);

	// Both lists are present. Compare their lengths.
	if (list1->NumItems() != list2->NumItems())
		return(FALSE);

	// Lengths of the sequences are the same. It is necessary to compare members of the seqs.
	for (TListIter<TCppUnresolvedNameInfo> iter1(list1), iter2(list2); (iter1 == TRUE && iter2 == TRUE); ++iter1, ++iter2)
	{
		if (CompareNameInfoInstances(&iter1.CurrItem(), &iter2.CurrItem()) == FALSE)
			return(FALSE);
	}

	// All elements of the lists are identical.
	return(TRUE);
}

bool TCppUnresolvedNameInfo::CompareNameInfoInstances(TCppUnresolvedNameInfo *obj1, TCppUnresolvedNameInfo *obj2)
{
	assert(obj1 != NULL && obj2 != NULL);
	if (obj1->m_node_type != obj2->m_node_type)
		return(FALSE);

	// The types of objects are the same. Compare less important props.
	switch (obj1->m_node_type)
	{
		case cnit_none:
				{
					// Uninited objects are considered to be identical.
					return(TRUE);
				}

		case cnit_identifier:
		case cnit_template_id:
		case cnit_constructor:
		case cnit_ctor_template:
				{
					// Compare the names.
					if (obj1->DefnKeyId() != obj2->DefnKeyId())
						return(FALSE);

					// Compare optional instantiation argument lists.
					return(CompareNameInfoTemplateArgLists(obj1->m_template_id_args, obj2->m_template_id_args));
				}

		case cnit_dector:
				{
					// Compare the names.
					return(obj1->DefnKeyId() == obj2->DefnKeyId());
				}

		case cnit_ovld_operator:
		case cnit_ovld_op_template:
				{
					// Compare operators that are being overloaded.
					if (obj1->m_operator_token != obj2->m_operator_token)
						return(FALSE);

					// Compare optional instantiation argument lists.
					return(CompareNameInfoTemplateArgLists(obj1->m_template_id_args, obj2->m_template_id_args));
				}

		case cnit_ovld_conversion:
		case cnit_ovld_conv_template:
				{
					// Compare the destination data types.
					if (TCppItemBase::CompareSameDatabaseDataTypes(obj1->m_conversion_dest_type, obj2->m_conversion_dest_type) == FALSE)
						return(FALSE);

					// Compare optional instantiation argument lists.
					return(CompareNameInfoTemplateArgLists(obj1->m_template_id_args, obj2->m_template_id_args));
				}
	}

	// Consider these objects as different.
	assert(FALSE);
	return(FALSE);
}

bool TCppUnresolvedNameInfo::CompareNameInfoTemplateArgLists(TList &list1, TList &list2)
{
	if (list1.NumItems() != list2.NumItems())
		return(FALSE);

	// -BUGBUG-
	return(TRUE);
}

DWORD TCppUnresolvedNameInfo::GetHashValue()
{
	switch (m_node_type)
	{
		case cnit_none:
				{
					return(0);
				}

		case cnit_identifier:
		case cnit_template_id:
		case cnit_constructor:
		case cnit_ctor_template:
				{
					return(DefnKeyId());
				}

		case cnit_dector:
				{
					return(0);
				}

		case cnit_ovld_operator:
		case cnit_ovld_op_template:
				{
					return(m_operator_token);
				}

		case cnit_ovld_conversion:
		case cnit_ovld_conv_template:
				{
					return(m_conversion_dest_type->ItemId());
				}
	}

	assert(FALSE);
	return(0);
}

DWORD TCppUnresolvedNameInfo::GetHashValueForNamesSeq(TList *unres_names_list)
{
	if (unres_names_list == NULL)
	{
		assert(FALSE);
		return(0);
	}

	DWORD val = 0;
	for (TListIter<TCppUnresolvedNameInfo> iter(unres_names_list); iter; ++iter)
		val += iter.CurrItem().GetHashValue();

	return(val);
}

//-----------------------------------------------------------------------------
//  ===================  TCppUnresolvedDataType  ====================
//-----------------------------------------------------------------------------

void TCppUnresolvedDataType::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->ItemType() == cxx_unresolved_name_info);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	assert(item_destination == cunr_chdest_default);
	m_name_elements.AppendItem(item);

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  ===================  TCppUnresolvedStructType  ===================
//-----------------------------------------------------------------------------

void TCppUnresolvedStructType::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->ItemType() == cxx_unresolved_name_info);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	assert(item_destination == cunr_chdest_default);
	m_name_elements.AppendItem(item);

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  ===================  TCppUnresolvedDataField  ====================
//-----------------------------------------------------------------------------

void TCppUnresolvedDataField::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->ItemType() == cxx_unresolved_name_info);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	assert(item_destination == cunr_chdest_default);
	m_name_elements.AppendItem(item);

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  ==================  TCppUnresolvedCodeEntryPoint  ==================
//-----------------------------------------------------------------------------

void TCppUnresolvedCodeEntryPoint::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->ItemType() == cxx_unresolved_name_info);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	assert(item_destination == cunr_chdest_default);
	m_name_elements.AppendItem(item);

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  =================  TCppUnresolvedClassTemplate  ===================
//-----------------------------------------------------------------------------

void TCppUnresolvedClassTemplate::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->ItemType() == cxx_unresolved_name_info);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	assert(item_destination == cunr_chdest_default);
	m_name_elements.AppendItem(item);

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  ================  TCppUnresolvedFunctionTemplate  ==================
//-----------------------------------------------------------------------------

void TCppUnresolvedFunctionTemplate::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->ItemType() == cxx_unresolved_name_info);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	assert(item_destination == cunr_chdest_default);
	m_name_elements.AppendItem(item);

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  =====================  TCppIfStatement  ========================
//-----------------------------------------------------------------------------

void TCppIfStatement::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->IsStatementBase() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	if (item_destination == cstmt_chdest_if_cond)
	{
		if (m_if_condition != NULL)
			delete m_if_condition;

		m_if_condition = (TCppStatementBase*)item;
	}
	else if (item_destination == cstmt_chdest_if_then)
	{
		if (m_then_statement != NULL)
			delete m_then_statement;

		m_then_statement = (TCppStatementBase*)item;
	}
	else if (item_destination == cstmt_chdest_if_else)
	{
		if (m_else_statement != NULL)
			delete m_else_statement;

		m_else_statement = (TCppStatementBase*)item;
	}
	else
	{
		// Bogus destination. Do not do anything.
		assert(FALSE);
		return;
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppIfStatement::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_special_count = 0;
	iter.m_curr_child = m_if_condition;

	if (iter.m_curr_child == NULL)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = m_then_statement;
	}

	if (iter.m_curr_child == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = m_else_statement;
	}
}

void TCppIfStatement::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 3;
		iter.m_curr_child = NULL;
	}
	else if (curr_chld == m_if_condition)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_then_statement)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_else_statement)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 3;
		iter.m_curr_child = NULL;
	}
}

void TCppIfStatement::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			iter.m_special_count = 1;
			iter.m_curr_child = m_then_statement;

			if (iter.m_curr_child == NULL)
			{
				iter.m_special_count = 2;
				iter.m_curr_child = m_else_statement;
			}
		}
		else if (iter.m_special_count == 1)
		{
			iter.m_special_count = 2;
			iter.m_curr_child = m_else_statement;
		}
		else
		{
			iter.m_special_count = 3;
			iter.m_curr_child = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
//  ====================  TCppSwitchStatement  ======================
//-----------------------------------------------------------------------------

void TCppSwitchStatement::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->IsStatementBase() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	if (item_destination == cstmt_chdest_sw_cond)
	{
		if (m_condition != NULL)
			delete m_condition;

		m_condition = (TCppStatementBase*)item;
	}
	else if (item_destination == cstmt_chdest_sw_stmt)
	{
		if (m_statement != NULL)
			delete m_statement;

		m_statement = (TCppStatementBase*)item;
	}
	else
	{
		// Bogus destination. Do not do anything.
		assert(FALSE);
		return;
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

int TCppSwitchStatement::NumCaseLabels() const
{
	int count = 0;
	for (TCppChildrenSubtreeIter iter((TCppSwitchStatement*)this); iter; ++iter)
	{
		TCppItemBase &item = iter.CurrItem();
		if (item.ItemType() == cxx_case_label)
		{
			// Current item is a case label.
			count++;
		}
		else if (item.ItemType() == cxx_switch_stmt)
		{
			// Ignore contents of this statement.
			iter.BlockCurrItemChildrenIteration();
		}
	}

	return(count);
}

void TCppSwitchStatement::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_special_count = 0;
	iter.m_curr_child = m_condition;

	if (iter.m_curr_child == NULL)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = m_statement;
	}
}

void TCppSwitchStatement::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
	else if (curr_chld == m_condition)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_statement)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppSwitchStatement::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			iter.m_special_count = 1;
			iter.m_curr_child = m_statement;
		}
		else
		{
			iter.m_special_count = 2;
			iter.m_curr_child = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
//  =====================  TCppForStatement  =======================
//-----------------------------------------------------------------------------

void TCppForStatement::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	if (item_destination == cstmt_chdest_for_expr1)
	{
		if (m_init_statement != NULL)
			delete m_init_statement;

		assert(item->IsStatementBase() == TRUE);
		m_init_statement = (TCppStatementBase*)item;
	}
	else if (item_destination == cstmt_chdest_for_expr2)
	{
		if (m_check_continue != NULL)
			delete m_check_continue;

		assert(item->IsStatementBase() == TRUE);
		m_check_continue = (TCppExpressionBase*)item;
	}
	else if (item_destination == cstmt_chdest_for_expr3)
	{
		if (m_reinit_expression != NULL)
			delete m_reinit_expression;

		assert(item->IsStatementBase() == TRUE);
		m_reinit_expression = (TCppExpressionBase*)item;
	}
	else if (item_destination == cstmt_chdest_for_body)
	{
		if (m_body_statement != NULL)
			delete m_body_statement;

		assert(item->IsStatementBase() == TRUE);
		m_body_statement = (TCppStatementBase*)item;
	}
	else
	{
		// Bogus destination. Do not do anything.
		assert(FALSE);
		return;
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppForStatement::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_special_count = 0;
	iter.m_curr_child = m_init_statement;

	if (iter.m_curr_child == NULL)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = m_check_continue;
	}

	if (iter.m_curr_child == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = m_reinit_expression;
	}

	if (iter.m_curr_child == NULL)
	{
		iter.m_special_count = 3;
		iter.m_curr_child = m_body_statement;
	}
}

void TCppForStatement::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 4;
		iter.m_curr_child = NULL;
	}
	else if (curr_chld == m_init_statement)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_check_continue)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_reinit_expression)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_body_statement)
	{
		iter.m_special_count = 3;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 4;
		iter.m_curr_child = NULL;
	}
}

void TCppForStatement::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			iter.m_special_count = 1;
			iter.m_curr_child = m_check_continue;

			if (iter.m_curr_child == NULL)
			{
				iter.m_special_count = 2;
				iter.m_curr_child = m_reinit_expression;
			}

			if (iter.m_curr_child == NULL)
			{
				iter.m_special_count = 3;
				iter.m_curr_child = m_body_statement;
			}
		}
		else if (iter.m_special_count == 1)
		{
			iter.m_special_count = 2;
			iter.m_curr_child = m_reinit_expression;

			if (iter.m_curr_child == NULL)
			{
				iter.m_special_count = 3;
				iter.m_curr_child = m_body_statement;
			}
		}
		else if (iter.m_special_count == 2)
		{
			iter.m_special_count = 3;
			iter.m_curr_child = m_body_statement;

		}
		else
		{
			iter.m_special_count = 4;
			iter.m_curr_child = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
//  ====================  TCppWhileStatement  ======================
//-----------------------------------------------------------------------------

void TCppWhileStatement::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->IsStatementBase() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	if (item_destination == cstmt_chdest_wh_cond)
	{
		if (m_while_condition != NULL)
			delete m_while_condition;

		m_while_condition = (TCppStatementBase*)item;
	}
	else if (item_destination == cstmt_chdest_wh_stmt)
	{
		if (m_body_statement != NULL)
			delete m_body_statement;

		m_body_statement = (TCppStatementBase*)item;
	}
	else
	{
		// Bogus destination. Do not do anything.
		assert(FALSE);
		return;
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppWhileStatement::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_special_count = 0;
	iter.m_curr_child = m_while_condition;

	if (iter.m_curr_child == NULL)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = m_body_statement;
	}
}

void TCppWhileStatement::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
	else if (curr_chld == m_while_condition)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_body_statement)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppWhileStatement::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			iter.m_special_count = 1;
			iter.m_curr_child = m_body_statement;
		}
		else
		{
			iter.m_special_count = 2;
			iter.m_curr_child = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
//  ===================  TCppDoWhileStatement  ======================
//-----------------------------------------------------------------------------

void TCppDoWhileStatement::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	if (item_destination == cstmt_chdest_dw_stmt)
	{
		if (m_body_statement != NULL)
			delete m_body_statement;

		assert(item->IsStatementBase() == TRUE);
		m_body_statement = (TCppStatementBase*)item;
	}
	else if (item_destination == cstmt_chdest_dw_cond)
	{
		if (m_do_while_expression != NULL)
			delete m_do_while_expression;

		assert(item->IsExpression() == TRUE);
		m_do_while_expression = (TCppExpressionBase*)item;
	}
	else
	{
		// Bogus destination. Do not do anything.
		assert(FALSE);
		return;
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppDoWhileStatement::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_special_count = 0;
	iter.m_curr_child = m_body_statement;

	if (iter.m_curr_child == NULL)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = m_do_while_expression;
	}
}

void TCppDoWhileStatement::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
	else if (curr_chld == m_body_statement)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_do_while_expression)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppDoWhileStatement::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			iter.m_special_count = 1;
			iter.m_curr_child = m_do_while_expression;
		}
		else
		{
			iter.m_special_count = 2;
			iter.m_curr_child = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
//  ===================  TCppReturnStatement  =======================
//-----------------------------------------------------------------------------

void TCppReturnStatement::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	assert(item_destination == cstmt_chdest_ret_expr);
	if (m_return_value != NULL)
		delete m_return_value;

	assert(item->IsExpression() == TRUE);
	m_return_value = (TCppExpressionBase*)item;

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  ====================  TCppLightBlockDefn  =======================
//-----------------------------------------------------------------------------

void TCppLightBlockDefn::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store the passed pointer at the appropiate side of the list.
	if (item_destination == cstmt_chdest_default)
	{
		// Add to the end of the list.
		m_members.AppendItem(item);
	}
	else
	{
		// Add to the beginning of the list.
		assert(item_destination == cstmt_chdest_ltblk_prepend);
		m_members.PrependItem(item);
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  ====================  TCppCaseLabelDefn  =======================
//-----------------------------------------------------------------------------

void TCppCaseLabelDefn::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field into the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into data field.
	assert(item_destination == cstmt_chdest_default);
	if (m_value_expr != NULL)
		delete m_value_expr;

	assert(item->IsExpression() == TRUE);
	m_value_expr = (TCppExpressionBase*)item;

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  ====================  TCppOperandExpr  ========================
//-----------------------------------------------------------------------------

static const wchar_t *g_CppExprOperandTypeEnumName[eopr_num_types] =
{
	L"eopr_none",

	L"eopr_bool_literal",			L"eopr_num_literal",
	L"eopr_charconst_literal",		L"eopr_string_literal",

	L"eopr_enum_literal",
	L"eopr_this",

	L"eopr_data_field",				L"eopr_code_entry_point",
	L"eopr_ovld_defns_bucket",
};

bool TCppOperandExpr::GetConstBoolValue(bool &bool_value) const
{
	bool_value = FALSE;

	if (m_oprnd_type == eopr_bool_literal)
	{
		bool_value = m_bool_value;
		return(TRUE);
	}
	else if (m_oprnd_type == eopr_num_literal)
	{
		bool_value = (m_num_value == 0) ? FALSE : TRUE;
		return(TRUE);
	}
	else if (m_oprnd_type == eopr_charconst_literal)
	{
		bool_value = (m_charconst_value == 0) ? FALSE : TRUE;
		return(TRUE);
	}
	else if (m_oprnd_type == eopr_enum_literal)
	{
		if (m_enum_member_ref != NULL && m_enum_member_ref->m_const_value == TRUE)
		{
			bool_value = (m_enum_member_ref->m_value == 0) ? FALSE : TRUE;
			return(TRUE);
		}
	}

	// All other types are not convertible.
	return(FALSE);
}

bool TCppOperandExpr::ConvertToNumericLiteral()
{
	if (m_oprnd_type == eopr_bool_literal)
	{
		// Result is an unsigned 8-bit integer.
		SetNumericLiteral(lnt_u8bit, m_bool_value);
		return(TRUE);
	}
	else if (m_oprnd_type == eopr_num_literal)
	{
		// Situation is already fine.
		return(TRUE);
	}
	else if (m_oprnd_type == eopr_charconst_literal)
	{
		if (m_charconst_subt == lchct_ascii)
		{
			// Result is a signed 8-bit integer.
			SetNumericLiteral(lnt_s8bit, m_charconst_value);
			return(TRUE);
		}
		else if (m_charconst_subt == lchct_unicode)
		{
			// Result is an unsigned 16-bit integer.
			SetNumericLiteral(lnt_u16bit, m_charconst_value);
			return(TRUE);
		}
	}
	else if (m_oprnd_type == eopr_enum_literal)
	{
		if (m_enum_member_ref != NULL && m_enum_member_ref->m_const_value == TRUE)
		{
			// Current enum member allows conversion to the numeric literal.
			SetNumericLiteral(m_enum_member_ref->m_value_num_subt, m_enum_member_ref->m_value);
			return(TRUE);
		}
	}

	// All other types are not convertible.
	return(FALSE);
}

bool TCppOperandExpr::GetConstOperandValue(TCppOperandExpr &expr_value) const
{
	expr_value.Clear();

	// Process literal cases of the current instance.
	switch (m_oprnd_type)
	{
		case eopr_bool_literal:			expr_value.SetBoolLiteral(m_bool_value);
									break;
		case eopr_num_literal:			expr_value.SetNumericLiteral(m_num_subt, m_num_value);
									break;
		case eopr_charconst_literal:		expr_value.SetCharConstLiteral(m_charconst_subt, m_charconst_value);
									break;
		case eopr_string_literal:		expr_value.SetStringLiteral(m_string_subt, m_string_value);
									break;
		case eopr_enum_literal:		expr_value.SetEnumLiteral(m_enum_member_ref);
									break;
		default:
			{
				// All other types of operands are not convertible.
				return(FALSE);
			}
	}

	// Success.
	return(TRUE);
}

const wchar_t *TCppOperandExpr::GetTextRepresentation(wchar_t *buffer, int buffer_len, bool want_ref_num)
{
	switch (m_oprnd_type)
	{
		case eopr_bool_literal:
				{
					return((m_bool_value == FALSE) ? L"false" : L"true");
				}

		case eopr_num_literal:
				{
					swprintf(buffer, buffer_len, L"%I64d (0x%I64X)", m_num_value, m_num_value);
					return(buffer);
				}

		case eopr_charconst_literal:
				{
					swprintf(buffer, buffer_len, L"0x%I64X", m_charconst_value);
					return(buffer);
				}

		case eopr_string_literal:
				{
					swprintf(buffer, buffer_len, L"\"%.*s\"", m_string_value.m_len, m_string_value.m_body);
					return(buffer);
				}

		case eopr_enum_literal:
				{
					if (m_enum_member_ref == NULL)
						return(L"null_enum_member_ref");
					else if (want_ref_num == FALSE)
						return(m_enum_member_ref->DefnName());

					swprintf(buffer, buffer_len, L"%s (%I64d)", m_enum_member_ref->DefnName(), m_enum_member_ref->m_value);
					return(buffer);
				}

		case eopr_data_field:
				{
					if (m_data_field_ref == NULL)
						return(L"null_data_field_ref");
					else if (want_ref_num == FALSE)
						return(m_data_field_ref->DefnName());

					swprintf(buffer, buffer_len, L"%s (%lu)", m_data_field_ref->DefnName(), m_data_field_ref->ItemId());
					return(buffer);
				}

		case eopr_code_entry_point:
				{
					if (m_code_entry_point_ref == NULL)
						return(L"null_data_field_ref");
					else if (want_ref_num == FALSE)
						return(m_code_entry_point_ref->DefnName());

					swprintf(buffer, buffer_len, L"%s (%lu)", m_code_entry_point_ref->DefnName(), m_code_entry_point_ref->ItemId());
					return(buffer);
				}
	}

	// Do not show any details for all other types of operands.
	return(NULL);
}

const wchar_t *TCppOperandExpr::GetOperandTypeEnumName(TCppExprOperandType oprnd_type)
{
	if (oprnd_type < 0 || oprnd_type >= eopr_num_types)
		return(L"BogusExprOperandTypeValue");

	return(g_CppExprOperandTypeEnumName[oprnd_type]);
}

//-----------------------------------------------------------------------------
//  =====================  TCppUnaryExpr  =========================
//-----------------------------------------------------------------------------

void TCppUnaryExpr::SetOperation(TOperatorToken opr)
{
	assert(opr == 0 || opr == opr_none || opr == opr_plusplus || opr == opr_minusminus || opr == opr_deref_ptr || opr == opr_get_addr || opr == opr_unplus || opr == opr_unminus || opr == opr_bang || opr == opr_tilda || opr == opr_plpl_postfix || opr == opr_mnmn_postfix);
	m_operation = opr;
}

bool TCppUnaryExpr::GetConstOperandValue(TCppOperandExpr &expr_value) const
{
	expr_value.Clear();

	// Evaluate the operand of expression first.
	if (m_operand == NULL || m_operand->GetConstOperandValue(expr_value) == FALSE)
		return(FALSE);

	switch (m_operation)
	{
		case opr_plusplus:
		case opr_plpl_postfix:
				{
					if (expr_value.m_oprnd_type == eopr_charconst_literal || expr_value.m_oprnd_type == eopr_enum_literal)
						expr_value.ConvertToNumericLiteral();

					if (expr_value.m_oprnd_type != eopr_num_literal)
						return(FALSE);

					// The operand is numeric or it was converted to numeric. Do the action.
					expr_value.m_num_value++;
				}
				break;

		case opr_minusminus:
		case opr_mnmn_postfix:
				{
					if (expr_value.m_oprnd_type == eopr_charconst_literal || expr_value.m_oprnd_type == eopr_enum_literal)
						expr_value.ConvertToNumericLiteral();

					if (expr_value.m_oprnd_type != eopr_num_literal)
						return(FALSE);

					// The operand is numeric or it was converted to numeric. Do the action.
					expr_value.m_num_value--;
				}
				break;

		case opr_deref_ptr:
		case opr_get_addr:
				{
					// These operations cannot be used in const expressions.
					return(FALSE);
				}

		case opr_unplus:
				{
					// This is no operation action.
					return(TRUE);
				}

		case opr_unminus:
				{
					if (expr_value.m_oprnd_type == eopr_charconst_literal || expr_value.m_oprnd_type == eopr_enum_literal)
						expr_value.ConvertToNumericLiteral();

					if (expr_value.m_oprnd_type != eopr_num_literal)
						return(FALSE);

					// The operand is numeric or it was converted to numeric. Do the action.
					expr_value.m_num_value = -expr_value.m_num_value;
				}
				break;

		case opr_bang:
				{
					// This is bool operation.
					bool bool_value;
					if (expr_value.GetConstBoolValue(bool_value) == FALSE)
						return(FALSE);

					// The bool value is extracted. Set the new value.
					expr_value.SetBoolLiteral((bool_value == FALSE) ? TRUE : FALSE);
				}
				break;

		case opr_tilda:
				{
					// Invert the value where it is possible.
					switch (expr_value.m_oprnd_type)
					{
						case eopr_bool_literal:			expr_value.m_bool_value = (expr_value.m_bool_value == FALSE) ? TRUE : FALSE;
													break;
						case eopr_charconst_literal:		expr_value.m_charconst_value = ~expr_value.m_charconst_value;
													break;
						case eopr_num_literal:			expr_value.m_num_value = ~expr_value.m_num_value;
													break;
						case eopr_enum_literal:
								{
									if (expr_value.ConvertToNumericLiteral() == FALSE)
										return(FALSE);

									// The operand was converted to numeric literal. Do the action.
									expr_value.m_num_value = -expr_value.m_num_value;
								}
								break;

						default:
							{
								// This operation is not applicable to other types of operands.
								return(FALSE);
							}
					}
				}
				break;

		default:
			{
				// The operation code in the expression is bogus.
				assert(FALSE);
				return(FALSE);
			}
	}

	// Success.
	return(TRUE);
}

void TCppUnaryExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	assert(item_destination == -1 || item_destination == cexpr_chdest_unary_oprnd);
	if (m_operand != NULL)
		delete m_operand;

	assert(item->IsExpression() == TRUE);
	m_operand = (TCppExpressionBase*)item;

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//------------------------------------------------------------------------------
//  =====================  TCppBinaryExpr  =========================
//------------------------------------------------------------------------------

void TCppBinaryExpr::SetOperation(TOperatorToken opr)
{
	assert(opr == 0 || opr == opr_none || opr == opr_eq || opr == opr_ne || opr == opr_lt || opr == opr_gt || opr == opr_le || opr == opr_ge || opr == opr_bitlsh || opr == opr_bitrsh || opr == opr_minus || opr == opr_div || opr == opr_rmnd);
	m_operation = opr;
}

bool TCppBinaryExpr::GetConstOperandValue(TCppOperandExpr &expr_value) const
{
	expr_value.Clear();

	// Evaluate the first operand of the expression.
	if (m_operand1 == NULL || m_operand1->GetConstOperandValue(expr_value) == FALSE)
		return(FALSE);

	// Evaluate the second operand of the expression.
	TCppOperandExpr second_operand_value;
	if (m_operand2 == NULL || m_operand2->GetConstOperandValue(second_operand_value) == FALSE)
		return(FALSE);

	// Both operands are contants. Apply the operation.
	switch (m_operation)
	{
		case opr_eq:
				{
					if (expr_value.m_oprnd_type == eopr_bool_literal && second_operand_value.m_oprnd_type == eopr_bool_literal)
					{
						// Both operands are boolean.
						expr_value.SetBoolLiteral(expr_value.m_bool_value == second_operand_value.m_bool_value);
					}
					else if (expr_value.m_oprnd_type == eopr_charconst_literal && second_operand_value.m_oprnd_type == eopr_charconst_literal)
					{
						// Both operands are charconsts.
						expr_value.SetBoolLiteral(expr_value.m_charconst_subt == second_operand_value.m_charconst_subt && expr_value.m_charconst_value == second_operand_value.m_charconst_value);
					}
					else if (expr_value.m_oprnd_type == eopr_string_literal || second_operand_value.m_oprnd_type == eopr_string_literal)
					{
						// At least one of the operands is string.
						if (expr_value.m_oprnd_type != second_operand_value.m_oprnd_type || expr_value.m_string_subt != second_operand_value.m_string_subt)
						{
							// One of the operands is not a string or the subtypes of the strings do not match.
							expr_value.SetBoolLiteral(FALSE);
						}
						else
						{
							// Compare the bodies of the strings.
							int len2 = second_operand_value.m_string_value.m_len;
							expr_value.SetBoolLiteral(expr_value.m_string_value.m_len == len2 && memcmp(expr_value.m_string_value.m_body, second_operand_value.m_string_value.m_body, len2*sizeof(wchar_t)) == 0);
						}
					}
					else
					{
						// Operands may be convertible to numeric values.
						if (expr_value.ConvertToNumericLiteral() == FALSE || second_operand_value.ConvertToNumericLiteral() == FALSE)
							return(FALSE);

						// Numeric values have the same representation for all numeric subtypes.
						expr_value.SetBoolLiteral(expr_value.m_num_value == second_operand_value.m_num_value);
					}
				}
				break;

		case opr_ne:
				{
					if (expr_value.m_oprnd_type == eopr_bool_literal && second_operand_value.m_oprnd_type == eopr_bool_literal)
					{
						// Both operands are boolean.
						expr_value.SetBoolLiteral(expr_value.m_bool_value != second_operand_value.m_bool_value);
					}
					else if (expr_value.m_oprnd_type == eopr_charconst_literal && second_operand_value.m_oprnd_type == eopr_charconst_literal)
					{
						// Both operands are charconsts.
						expr_value.SetBoolLiteral(expr_value.m_charconst_subt != second_operand_value.m_charconst_subt || expr_value.m_charconst_value != second_operand_value.m_charconst_value);
					}
					else if (expr_value.m_oprnd_type == eopr_string_literal || second_operand_value.m_oprnd_type == eopr_string_literal)
					{
						// At least one of the operands is string.
						if (expr_value.m_oprnd_type != second_operand_value.m_oprnd_type || expr_value.m_string_subt != second_operand_value.m_string_subt)
						{
							// One of the operands is not a string or the subtypes of the strings do not match.
							expr_value.SetBoolLiteral(TRUE);
						}
						else
						{
							// Compare the bodies of the strings.
							int len2 = second_operand_value.m_string_value.m_len;
							expr_value.SetBoolLiteral(expr_value.m_string_value.m_len != len2 || memcmp(expr_value.m_string_value.m_body, second_operand_value.m_string_value.m_body, len2*sizeof(wchar_t)) != 0);
						}
					}
					else
					{
						// Operands may be convertible to numeric values.
						if (expr_value.ConvertToNumericLiteral() == FALSE || second_operand_value.ConvertToNumericLiteral() == FALSE)
							return(FALSE);

						// Numeric values have the same representation for all numeric subtypes.
						expr_value.SetBoolLiteral(expr_value.m_num_value != second_operand_value.m_num_value);
					}
				}
				break;

		case opr_lt:
				{
					if (expr_value.m_oprnd_type == eopr_string_literal || second_operand_value.m_oprnd_type == eopr_string_literal)
					{
						// At least one of the operands is string.
						if (expr_value.m_oprnd_type != second_operand_value.m_oprnd_type || expr_value.m_string_subt != second_operand_value.m_string_subt)
							return(FALSE);

						// Both operands are strings of the same subtype. Compare the bodies of the strings.
						int len = ___min(expr_value.m_string_value.m_len, second_operand_value.m_string_value.m_len);
						expr_value.SetBoolLiteral(wcsncmp(expr_value.m_string_value.m_body, second_operand_value.m_string_value.m_body, len) < 0);
					}
					else
					{
						// Operands may be convertible to numeric values.
						if (expr_value.ConvertToNumericLiteral() == FALSE || second_operand_value.ConvertToNumericLiteral() == FALSE)
							return(FALSE);

						// Compare the numeric values.
						expr_value.SetBoolLiteral(expr_value.m_num_value < second_operand_value.m_num_value);
					}
				}
				break;

		case opr_gt:
				{
					if (expr_value.m_oprnd_type == eopr_string_literal || second_operand_value.m_oprnd_type == eopr_string_literal)
					{
						// At least one of the operands is string.
						if (expr_value.m_oprnd_type != second_operand_value.m_oprnd_type || expr_value.m_string_subt != second_operand_value.m_string_subt)
							return(FALSE);

						// Both operands are strings of the same subtype. Compare the bodies of the strings.
						int len = ___min(expr_value.m_string_value.m_len, second_operand_value.m_string_value.m_len);
						expr_value.SetBoolLiteral(wcsncmp(expr_value.m_string_value.m_body, second_operand_value.m_string_value.m_body, len) > 0);
					}
					else
					{
						// Operands may be convertible to numeric values.
						if (expr_value.ConvertToNumericLiteral() == FALSE || second_operand_value.ConvertToNumericLiteral() == FALSE)
							return(FALSE);

						// Compare the numeric values.
						expr_value.SetBoolLiteral(expr_value.m_num_value > second_operand_value.m_num_value);
					}
				}
				break;

		case opr_le:
				{
					if (expr_value.m_oprnd_type == eopr_string_literal || second_operand_value.m_oprnd_type == eopr_string_literal)
					{
						// At least one of the operands is string.
						if (expr_value.m_oprnd_type != second_operand_value.m_oprnd_type || expr_value.m_string_subt != second_operand_value.m_string_subt)
							return(FALSE);

						// Both operands are strings of the same subtype. Compare the bodies of the strings.
						int len = ___min(expr_value.m_string_value.m_len, second_operand_value.m_string_value.m_len);
						expr_value.SetBoolLiteral(wcsncmp(expr_value.m_string_value.m_body, second_operand_value.m_string_value.m_body, len) <= 0);
					}
					else
					{
						// Operands may be convertible to numeric values.
						if (expr_value.ConvertToNumericLiteral() == FALSE || second_operand_value.ConvertToNumericLiteral() == FALSE)
							return(FALSE);

						// Compare the numeric values.
						expr_value.SetBoolLiteral(expr_value.m_num_value <= second_operand_value.m_num_value);
					}
				}
				break;

		case opr_ge:
				{
					if (expr_value.m_oprnd_type == eopr_string_literal || second_operand_value.m_oprnd_type == eopr_string_literal)
					{
						// At least one of the operands is string.
						if (expr_value.m_oprnd_type != second_operand_value.m_oprnd_type || expr_value.m_string_subt != second_operand_value.m_string_subt)
							return(FALSE);

						// Both operands are strings of the same subtype. Compare the bodies of the strings.
						int len = ___min(expr_value.m_string_value.m_len, second_operand_value.m_string_value.m_len);
						expr_value.SetBoolLiteral(wcsncmp(expr_value.m_string_value.m_body, second_operand_value.m_string_value.m_body, len) >= 0);
					}
					else
					{
						// Operands may be convertible to numeric values.
						if (expr_value.ConvertToNumericLiteral() == FALSE || second_operand_value.ConvertToNumericLiteral() == FALSE)
							return(FALSE);

						// Compare the numeric values.
						expr_value.SetBoolLiteral(expr_value.m_num_value >= second_operand_value.m_num_value);
					}
				}
				break;

		case opr_bitlsh:
				{
					// Operation can be carried out when both operands can be converted to numeric.
					if (expr_value.ConvertToNumericLiteral() == FALSE || second_operand_value.ConvertToNumericLiteral() == FALSE)
						return(FALSE);

					// Result has numeric subtype of the first operand.
					expr_value.m_num_value <<= second_operand_value.m_num_value;
				}
				break;

		case opr_bitrsh:
				{
					// Operation can be carried out when both operands can be converted to numeric.
					if (expr_value.ConvertToNumericLiteral() == FALSE || second_operand_value.ConvertToNumericLiteral() == FALSE)
						return(FALSE);

					// Result has numeric subtype of the first operand.
					expr_value.m_num_value >>= second_operand_value.m_num_value;
				}
				break;

		case opr_minus:
				{
					// Operation can be carried out when both operands can be converted to numeric.
					if (expr_value.ConvertToNumericLiteral() == FALSE || second_operand_value.ConvertToNumericLiteral() == FALSE)
						return(FALSE);

					// Both operands were converted to the numeric type.
					expr_value.m_num_subt = GetNumericOperationResultType(expr_value.m_num_subt, second_operand_value.m_num_subt);
					expr_value.m_num_value -= second_operand_value.m_num_value;
				}
				break;

		case opr_div:
				{
					// Operation can be carried out when both operands can be converted to numeric.
					if (expr_value.ConvertToNumericLiteral() == FALSE || second_operand_value.ConvertToNumericLiteral() == FALSE)
						return(FALSE);

					// Both operands were converted to the numeric type.
					if (second_operand_value.m_num_value == 0)
					{
						// This is division by zero. The result is undefined.
						return(FALSE);
					}

					// Keep numeric subtype of the first operand.
					expr_value.m_num_value /= second_operand_value.m_num_value;
				}
				break;

		case opr_rmnd:
				{
					// Operation can be carried out when both operands can be converted to numeric.
					if (expr_value.ConvertToNumericLiteral() == FALSE || second_operand_value.ConvertToNumericLiteral() == FALSE)
						return(FALSE);

					// Both operands were converted to the numeric type.
					if (second_operand_value.m_num_value == 0)
					{
						// This is division by zero. The result is undefined.
						return(FALSE);
					}

					// Keep numeric subtype of the first operand.
					expr_value.m_num_value %= second_operand_value.m_num_value;
				}
				break;

		default:
			{
				// The operation code is bogus.
				assert(FALSE);
				return(FALSE);
			}
	}

	// Success.
	return(TRUE);
}

void TCppBinaryExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->IsExpression() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	if (item_destination == cexpr_chdest_binary_oprnd1)
	{
		if (m_operand1 != NULL)
			delete m_operand1;

		m_operand1 = (TCppExpressionBase*)item;
	}
	else if (item_destination == cexpr_chdest_binary_oprnd2)
	{
		if (m_operand2 != NULL)
			delete m_operand2;

		m_operand2 = (TCppExpressionBase*)item;
	}
	else
	{
		// The destination of the operand is bogus. Do not do anything.
		assert(FALSE);
		return;
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppBinaryExpr::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_special_count = 0;
	iter.m_curr_child = m_operand1;

	if (iter.m_curr_child == NULL)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = m_operand2;
	}
}

void TCppBinaryExpr::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
	else if (curr_chld == m_operand1)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_operand2)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppBinaryExpr::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			iter.m_special_count = 1;
			iter.m_curr_child = m_operand2;
		}
		else
		{
			iter.m_special_count = 2;
			iter.m_curr_child = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
//  ====================  TCppMultiOpExpr  =========================
//-----------------------------------------------------------------------------

void TCppMultiOpExpr::SetOperation(TOperatorToken opr)
{
	assert(opr == 0 || opr == opr_none || opr == opr_comma || opr == opr_logand || opr == opr_logor || opr == opr_bitand || opr == opr_bitor || opr == opr_bitxor || opr == opr_plus || opr == opr_mul);
	m_operation = opr;
}

bool TCppMultiOpExpr::GetConstOperandValue(TCppOperandExpr &expr_value) const
{
	expr_value.Clear();

	// Evaluate the first operand of the expression.
	TCppExpressionBase *operand = (TCppExpressionBase*)m_operands.GetFirst();
	if (operand == NULL || operand->GetConstOperandValue(expr_value) == FALSE)
		return(FALSE);

	// At least one operand is present in the expression.
	for(;;)
	{
		// Pick up the next operand.
		operand = (TCppExpressionBase*)m_operands.GetNext(operand);
		if (operand == NULL)
		{
			// Applying all operations succeeded or there was only one operand.
			break;
		}

		// The operand is available. Pick up its const value.
		TCppOperandExpr curr_operand_value;
		if (operand->GetConstOperandValue(curr_operand_value) == FALSE)
			return(FALSE);

		switch (m_operation)
		{
			case opr_comma:
					{
						// Discard the accumulated value.
						curr_operand_value.GetConstOperandValue(expr_value);
					}
					break;

			case opr_logand:
					{
						// Extract bool value from both operands.
						bool val1, val2;
						if (expr_value.GetConstBoolValue(val1) == FALSE || curr_operand_value.GetConstBoolValue(val2) == FALSE)
							return(FALSE);

						// Set the result back.
						expr_value.SetBoolLiteral(val1 && val2);
					}
					break;

			case opr_logor:
					{
						// Extract bool value from both operands.
						bool val1, val2;
						if (expr_value.GetConstBoolValue(val1) == FALSE || curr_operand_value.GetConstBoolValue(val2) == FALSE)
							return(FALSE);

						// Set the result back.
						expr_value.SetBoolLiteral(val1 || val2);
					}
					break;

			case opr_bitand:
					{
						if (expr_value.m_oprnd_type == eopr_bool_literal && curr_operand_value.m_oprnd_type == eopr_bool_literal)
						{
							// Both operands are boolean.
							expr_value.m_bool_value &= curr_operand_value.m_bool_value;
						}
						else if (expr_value.m_oprnd_type == eopr_charconst_literal && curr_operand_value.m_oprnd_type == eopr_charconst_literal && expr_value.m_charconst_subt == curr_operand_value.m_charconst_subt)
						{
							// Both operands are charconsts of the same subtype.
							expr_value.m_charconst_value &= curr_operand_value.m_charconst_value;
						}
						else if (expr_value.ConvertToNumericLiteral() == TRUE && curr_operand_value.ConvertToNumericLiteral() == TRUE)
						{
							// Both operands were converted to the numeric type.
							expr_value.m_num_subt = GetNumericOperationResultType(expr_value.m_num_subt, curr_operand_value.m_num_subt);
							expr_value.m_num_value &= curr_operand_value.m_num_value;
						}
						else
						{
							// Types of operands do not allow to carry out the operation.
							return(FALSE);
						}
					}
					break;

			case opr_bitor:
					{
						if (expr_value.m_oprnd_type == eopr_bool_literal && curr_operand_value.m_oprnd_type == eopr_bool_literal)
						{
							// Both operands are boolean.
							expr_value.m_bool_value |= curr_operand_value.m_bool_value;
						}
						else if (expr_value.m_oprnd_type == eopr_charconst_literal && curr_operand_value.m_oprnd_type == eopr_charconst_literal && expr_value.m_charconst_subt == curr_operand_value.m_charconst_subt)
						{
							// Both operands are charconsts of the same subtype.
							expr_value.m_charconst_value |= curr_operand_value.m_charconst_value;
						}
						else if (expr_value.ConvertToNumericLiteral() == TRUE && curr_operand_value.ConvertToNumericLiteral() == TRUE)
						{
							// Both operands were converted to the numeric type.
							expr_value.m_num_subt = GetNumericOperationResultType(expr_value.m_num_subt, curr_operand_value.m_num_subt);
							expr_value.m_num_value |= curr_operand_value.m_num_value;
						}
						else
						{
							// Types of operands do not allow to carry out the operation.
							return(FALSE);
						}
					}
					break;

			case opr_bitxor:
					{
						if (expr_value.m_oprnd_type == eopr_bool_literal && curr_operand_value.m_oprnd_type == eopr_bool_literal)
						{
							// Both operands are boolean.
							expr_value.m_bool_value ^= curr_operand_value.m_bool_value;
						}
						else if (expr_value.m_oprnd_type == eopr_charconst_literal && curr_operand_value.m_oprnd_type == eopr_charconst_literal && expr_value.m_charconst_subt == curr_operand_value.m_charconst_subt)
						{
							// Both operands are charconsts of the same subtype.
							expr_value.m_charconst_value ^= curr_operand_value.m_charconst_value;
						}
						else if (expr_value.ConvertToNumericLiteral() == TRUE && curr_operand_value.ConvertToNumericLiteral() == TRUE)
						{
							// Both operands were converted to the numeric type.
							expr_value.m_num_subt = GetNumericOperationResultType(expr_value.m_num_subt, curr_operand_value.m_num_subt);
							expr_value.m_num_value ^= curr_operand_value.m_num_value;
						}
						else
						{
							// Types of operands do not allow to carry out the operation.
							return(FALSE);
						}
					}
					break;

			case opr_plus:
					{
						// Operation can be carried out when both operands can be converted to numeric.
						if (expr_value.ConvertToNumericLiteral() == FALSE || curr_operand_value.ConvertToNumericLiteral() == FALSE)
						{
							// Types of operands do not allow to do the operation.
							return(FALSE);
						}

						// Both operands were converted to the numeric type.
						expr_value.m_num_subt = GetNumericOperationResultType(expr_value.m_num_subt, curr_operand_value.m_num_subt);
						expr_value.m_num_value += curr_operand_value.m_num_value;
					}
					break;

			case opr_mul:
					{
						// Operation can be carried out when both operands can be converted to numeric.
						if (expr_value.ConvertToNumericLiteral() == FALSE || curr_operand_value.ConvertToNumericLiteral() == FALSE)
						{
							// Types of operands do not allow to do the operation.
							return(FALSE);
						}

						// Both operands were converted to the numeric type.
						expr_value.m_num_subt = GetNumericOperationResultType(expr_value.m_num_subt, curr_operand_value.m_num_subt);
						expr_value.m_num_value *= curr_operand_value.m_num_value;
					}
					break;

			default:
				{
					// The operation is bogus.
					assert(FALSE);
					return(FALSE);
				}
		}
	}

	// Success.
	return(TRUE);
}

void TCppMultiOpExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->IsExpression() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Add passed pointer into the list.
	if (item_destination == cexpr_chdest_multiop_prepend)
	{
		m_operands.PrependItem(item);
	}
	else
	{
		assert(item_destination == -1 || item_destination == cexpr_chdest_multiop_append);
		m_operands.AppendItem(item);
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  ===================  TCppAssignmentExpr  =======================
//-----------------------------------------------------------------------------

void TCppAssignmentExpr::SetOperation(TOperatorToken opr)
{
	assert(opr == 0 || opr == opr_none || opr == opr_assign || opr == opr_plus_asgn || opr == opr_minus_asgn || opr == opr_mul_asgn || opr == opr_div_asgn || opr == opr_rmnd_asgn || opr == opr_lsh_asgn || opr == opr_rsh_asgn || opr == opr_btand_asgn || opr == opr_btor_asgn || opr == opr_btxor_asgn);
	m_operation = opr;
}

void TCppAssignmentExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->IsExpression() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	if (item_destination == cexpr_chdest_assgn_oprnd1)
	{
		if (m_operand1 != NULL)
			delete m_operand1;

		m_operand1 = (TCppExpressionBase*)item;
	}
	else if (item_destination == cexpr_chdest_assgn_oprnd2)
	{
		if (m_operand2 != NULL)
			delete m_operand2;

		m_operand2 = (TCppExpressionBase*)item;
	}
	else
	{
		// Bogus destination. Do not do anything.
		assert(FALSE);
		return;
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppAssignmentExpr::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_special_count = 0;
	iter.m_curr_child = m_operand1;

	if (iter.m_curr_child == NULL)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = m_operand2;
	}
}

void TCppAssignmentExpr::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
	else if (curr_chld == m_operand1)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_operand2)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppAssignmentExpr::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			iter.m_special_count = 1;
			iter.m_curr_child = m_operand2;
		}
		else
		{
			iter.m_special_count = 2;
			iter.m_curr_child = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
//  =====================  TCppThrowExpr  =========================
//-----------------------------------------------------------------------------

void TCppThrowExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	assert(item_destination == -1 || item_destination == cexpr_chdest_throw_oprnd);
	if (m_operand != NULL)
		delete m_operand;

	assert(item->IsExpression() == TRUE);
	m_operand = (TCppExpressionBase*)item;

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//------------------------------------------------------------------------------
//  ====================  TCppConditionalExpr  =======================
//------------------------------------------------------------------------------

bool TCppConditionalExpr::GetConstOperandValue(TCppOperandExpr &expr_value) const
{
	// Evaluate the first operand of the expression first.
	if (m_operand1 == NULL || m_operand1->GetConstOperandValue(expr_value) == FALSE)
		return(FALSE);

	// First operand represents some constant. Pick up its bool value.
	bool bool_value;
	if (expr_value.GetConstBoolValue(bool_value) == FALSE)
		return(FALSE);

	if (bool_value == TRUE)
	{
		// Process the second operand.
		if (m_operand2 == NULL || m_operand2->GetConstOperandValue(expr_value) == FALSE)
			return(FALSE);
	}
	else
	{
		// Process the third operand.
		if (m_operand3 == NULL || m_operand3->GetConstOperandValue(expr_value) == FALSE)
			return(FALSE);
	}

	// Success.
	return(TRUE);
}

void TCppConditionalExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->IsExpression() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store the passed pointer in the right data field.
	if (item_destination == cexpr_chdest_cond_oprnd1)
	{
		if (m_operand1 != NULL)
			delete m_operand1;

		m_operand1 = (TCppExpressionBase*)item;
	}
	else if (item_destination == cexpr_chdest_cond_oprnd2)
	{
		if (m_operand2 != NULL)
			delete m_operand2;

		m_operand2 = (TCppExpressionBase*)item;
	}
	else if (item_destination == cexpr_chdest_cond_oprnd3)
	{
		if (m_operand3 != NULL)
			delete m_operand3;

		m_operand3 = (TCppExpressionBase*)item;
	}
	else
	{
		// Bogus destination. Do not do anything.
		assert(FALSE);
		return;
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppConditionalExpr::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_special_count = 0;
	iter.m_curr_child = m_operand1;

	if (iter.m_curr_child == NULL)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = m_operand2;
	}

	if (iter.m_curr_child == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = m_operand3;
	}
}

void TCppConditionalExpr::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 3;
		iter.m_curr_child = NULL;
	}
	else if (curr_chld == m_operand1)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_operand2)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_operand3)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 3;
		iter.m_curr_child = NULL;
	}
}

void TCppConditionalExpr::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			iter.m_special_count = 1;
			iter.m_curr_child = m_operand2;

			if (iter.m_curr_child == NULL)
			{
				iter.m_special_count = 2;
				iter.m_curr_child = m_operand3;
			}
		}
		else if (iter.m_special_count == 1)
		{
			iter.m_special_count = 2;
			iter.m_curr_child = m_operand3;
		}
		else
		{
			iter.m_special_count = 3;
			iter.m_curr_child = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
//  ===================  TCppFieldAccessExpr  =======================
//-----------------------------------------------------------------------------

void TCppFieldAccessExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	assert(item_destination == cexpr_chdest_fld_acc_oprnd);
	if (m_struct_operand != NULL)
		delete m_struct_operand;

	assert(item->IsExpression() == TRUE);
	m_struct_operand = (TCppExpressionBase*)item;

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//------------------------------------------------------------------------------
//  ==================  TCppFieldAccessPtrExpr  =======================
//------------------------------------------------------------------------------

void TCppFieldAccessPtrExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->IsExpression() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store the passed pointer in the right data field.
	if (item_destination == cexpr_chdest_fp_acc_oprnd1)
	{
		if (m_struct_operand != NULL)
			delete m_struct_operand;

		m_struct_operand = (TCppExpressionBase*)item;
	}
	else if (item_destination == cexpr_chdest_fp_acc_oprnd2)
	{
		if (m_struct_member_ptr != NULL)
			delete m_struct_member_ptr;

		m_struct_member_ptr = (TCppExpressionBase*)item;
	}
	else
	{
		// Bogus destination. Do not do anything.
		assert(FALSE);
		return;
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppFieldAccessPtrExpr::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_special_count = 0;
	iter.m_curr_child = m_struct_operand;

	if (iter.m_curr_child == NULL)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = m_struct_member_ptr;
	}
}

void TCppFieldAccessPtrExpr::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
	else if (curr_chld == m_struct_operand)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_struct_member_ptr)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppFieldAccessPtrExpr::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			iter.m_special_count = 1;
			iter.m_curr_child = m_struct_member_ptr;
		}
		else
		{
			iter.m_special_count = 2;
			iter.m_curr_child = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
//  ===================  TCppArrayIndexExpr  =======================
//-----------------------------------------------------------------------------

bool TCppArrayIndexExpr::GetConstOperandValue(TCppOperandExpr &expr_value) const
{
	expr_value.Clear();

	// Evaluate the array operand of the expression first.
	if (m_array_operand == NULL || m_array_operand->GetConstOperandValue(expr_value) == FALSE)
		return(FALSE);

	// Const calculation is possible only if the array operand is string and there is one index.
	if (expr_value.m_oprnd_type != eopr_string_literal || m_index_expressions.NumItems() != 1)
		return(FALSE);

	// Pick up the const value of the index.
	TCppOperandExpr index_value;
	if (((TCppExpressionBase*)(m_index_expressions.GetFirst()))->GetConstOperandValue(index_value) == FALSE)
		return(FALSE);

	// Convert literal value to numeric value.
	if (index_value.ConvertToNumericLiteral() == FALSE)
		return(FALSE);

	// The index value is available. Check the boundaries.
	if (index_value.m_num_value < 0 || index_value.m_num_value >= expr_value.m_string_value.m_len)
		return(FALSE);

	// The boundaries are fine. Prepare the charconst value.
	TLexCharConstType ch_subt = TLexema::GetCharConstTypeFromStringType(expr_value.m_string_subt);
	wchar_t ch_value = expr_value.m_string_value.m_body[index_value.m_num_value];
	expr_value.SetCharConstLiteral(ch_subt, ch_value);

	// Success.
	return(TRUE);
}

void TCppArrayIndexExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->IsExpression() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store the passed pointer in the right data field.
	if (item_destination == cexpr_chdest_index_array)
	{
		if (m_array_operand != NULL)
			delete m_array_operand;

		m_array_operand = (TCppExpressionBase*)item;
	}
	else if (item_destination == cexpr_chdest_index_params)
	{
		// Add passed pointer into the list.
		m_index_expressions.AppendItem(item);
	}
	else
	{
		// Bogus destination. Do not do anything.
		assert(FALSE);
		return;
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppArrayIndexExpr::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_special_count = 0;
	iter.m_curr_child = m_array_operand;

	if (iter.m_curr_child == NULL)
	{
		// The array variable is missing for some strange reason.
		iter.m_special_count = 1;
		iter.m_curr_child = (TCppItemBase*)m_index_expressions.GetFirst();
	}
}

void TCppArrayIndexExpr::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
	else if (curr_chld == m_array_operand)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (m_index_expressions.IsItemInList(curr_chld) == TRUE)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppArrayIndexExpr::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			iter.m_special_count = 1;
			iter.m_curr_child = (TCppItemBase*)m_index_expressions.GetFirst();
		}
		else
		{
			iter.m_curr_child = (TCppItemBase*)m_index_expressions.GetNext(iter.m_curr_child);
			if (iter.m_curr_child == NULL)
				iter.m_special_count = 2;
		}
	}
}

//-----------------------------------------------------------------------------
//  ===================  TCppFunctionCallExpr  =======================
//-----------------------------------------------------------------------------

void TCppFunctionCallExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->IsExpression() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store the passed pointer in the right data field.
	if (item_destination == cexpr_chdest_fcall_func)
	{
		if (m_function != NULL)
			delete m_function;

		m_function = (TCppExpressionBase*)item;
	}
	else if (item_destination == cexpr_chdest_fcall_params)
	{
		// Add passed pointer into the list.
		m_parameters.AppendItem(item);
	}
	else
	{
		// The destination is bogus. Do not do anything.
		assert(FALSE);
		return;
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppFunctionCallExpr::AddCallParameters(TCppExpressionBase *params_expr)
{
	if (params_expr->m_operation == opr_comma)
	{
		// Expression is the list of expressions. Add each expression as a separate parameter.
		TCppMultiOpExpr *mop_expr = (TCppMultiOpExpr*)params_expr;
		AddListOfChildren(mop_expr->m_operands, cexpr_chdest_fcall_params);
		delete params_expr;
	}
	else
	{
		// This is some other type of expression. Add this expr as one parameter.
		AddChildItem(params_expr, cexpr_chdest_fcall_params);
	}
}

void TCppFunctionCallExpr::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_special_count = 0;
	iter.m_curr_child = m_function;

	if (iter.m_curr_child == NULL)
	{
		// The function to call is missing for unclear reason.
		iter.m_special_count = 1;
		iter.m_curr_child = (TCppItemBase*)m_parameters.GetFirst();
	}
}

void TCppFunctionCallExpr::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
	else if (curr_chld == m_function)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (m_parameters.IsItemInList(curr_chld) == TRUE)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppFunctionCallExpr::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			// Pick up the first actual parameter of the call.
			iter.m_special_count = 1;
			iter.m_curr_child = (TCppItemBase*)m_parameters.GetFirst();
		}
		else
		{
			// Pick up the next actual parameter of the call.
			iter.m_curr_child = (TCppItemBase*)m_parameters.GetNext(iter.m_curr_child);
			if (iter.m_curr_child == NULL)
				iter.m_special_count = 2;
		}
	}
}

//-----------------------------------------------------------------------------
//  =====================  TCppCastExpr  =========================
//-----------------------------------------------------------------------------

bool TCppCastExpr::GetConstOperandValue(TCppOperandExpr &expr_value) const
{
	expr_value.Clear();

	// Evaluate the operand of the expression.
	if (m_operand == NULL || m_operand->GetConstOperandValue(expr_value) == FALSE)
		return(FALSE);

	// Disallow this for now.
	return(FALSE);
}

void TCppCastExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	assert(item_destination == -1 || item_destination == cexpr_chdest_cast_oprnd);
	if (m_operand != NULL)
		delete m_operand;

	assert(item->IsExpression() == TRUE);
	m_operand = (TCppExpressionBase*)item;

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  =====================  TCppSizeofExpr  =========================
//-----------------------------------------------------------------------------

bool TCppSizeofExpr::GetConstOperandValue(TCppOperandExpr &expr_value) const
{
	expr_value.Clear();

	// Disallow this for now.
	return(FALSE);
}

void TCppSizeofExpr::SetDataTypeOperand(TCppItemBase *item)
{
	// This method expects that SIZEOF of the data type is picked.
	assert(item != NULL && item->ItemParent() != NULL && item->IsDataType() == TRUE);

	m_expr_operand = FALSE;
	m_data_type = (TCppDataTypeBase*)item;
}

void TCppSizeofExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	// This method expects that size of expression result is picked.
	assert(item != NULL && item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->IsExpression() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	assert(item_destination == cexpr_chdest_sizeof_oprnd);
	if (m_expr_operand == TRUE && m_operand != NULL)
		delete m_operand;

	m_expr_operand = TRUE;
	m_operand = (TCppExpressionBase*)item;

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppSizeofExpr::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_special_count = 0;
	if (m_expr_operand == TRUE)
	{
		// Current expression evaluates the size of the result of computation.
		iter.m_curr_child = m_operand;
	}
	else
	{
		// Current expression evaluates the size of some data type.
		iter.m_curr_child = NULL;
	}
}

void TCppSizeofExpr::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	iter.m_special_count = 0;
	if (curr_chld != NULL && m_expr_operand == TRUE && curr_chld == m_operand)
	{
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_curr_child = NULL;
	}
}

void TCppSizeofExpr::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	iter.m_curr_child = NULL;
}

//-----------------------------------------------------------------------------
//  =====================  TCppNewExpr  ==========================
//-----------------------------------------------------------------------------

void TCppNewExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->IsExpression() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	if (item_destination == cexpr_chdest_new_plc_params)
	{
		// Add passed pointer into the list.
		m_new_placement_exprs.AppendItem(item);
	}
	else if (item_destination == cexpr_chdest_new_init_params)
	{
		// Add passed pointer into the list.
		m_init_param_exprs.AppendItem(item);
	}
	else if (item_destination == cexpr_chdest_new_ctor_call)
	{
		// Replace the old call of the constructor if any.
		if (m_ctor_call_expr != NULL)
			delete m_ctor_call_expr;

		assert(item->ItemType() == cxx_function_call_expr);
		m_ctor_call_expr = (TCppFunctionCallExpr*)item;
	}
	else
	{
		// The destination is bogus. Do not do anything.
		assert(FALSE);
		return;
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppNewExpr::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	// Start iterating the new placement params.
	iter.m_special_count = 0;
	iter.m_curr_child = (TCppItemBase*)m_new_placement_exprs.GetFirst();

	if (iter.m_curr_child == NULL)
	{
		// The placement params are missing. This is typical case.
		iter.m_special_count = 1;
		iter.m_curr_child = (TCppItemBase*)m_init_param_exprs.GetFirst();

		if (iter.m_curr_child == NULL)
		{
			// The init params are missing. This happens often.
			iter.m_special_count = 2;
			iter.m_curr_child = m_ctor_call_expr;
		}
	}
}

void TCppNewExpr::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 3;
		iter.m_curr_child = NULL;
	}
	else if (m_new_placement_exprs.IsItemInList(curr_chld) == TRUE)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (m_init_param_exprs.IsItemInList(curr_chld) == TRUE)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else if (curr_chld == m_ctor_call_expr)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 3;
		iter.m_curr_child = NULL;
	}
}

void TCppNewExpr::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0)
		{
			// Pick up the next new placement param.
			iter.m_curr_child = (TCppItemBase*)m_new_placement_exprs.GetNext(iter.m_curr_child);
			if (iter.m_curr_child == NULL)
			{
				// Continue with iteration of the init params if any.
				iter.m_special_count = 1;
				iter.m_curr_child = (TCppItemBase*)m_init_param_exprs.GetFirst();

				if (iter.m_curr_child == NULL)
				{
					// Continue iteration with the ctor call expr if any.
					iter.m_special_count = 2;
					iter.m_curr_child = m_ctor_call_expr;
				}
			}
		}
		else if (iter.m_special_count == 1)
		{
			// Pick up the next item from the list of init params.
			iter.m_curr_child = (TCppItemBase*)m_init_param_exprs.GetNext(iter.m_curr_child);
			if (iter.m_curr_child == NULL)
			{
				// Continue iteration with the ctor call expr if any.
				iter.m_special_count = 2;
				iter.m_curr_child = m_ctor_call_expr;
			}
		}
		else
		{
			iter.m_special_count = 2;
			iter.m_curr_child = m_ctor_call_expr;
		}
	}
}

//-----------------------------------------------------------------------------
//  =====================  TCppDeleteExpr  ========================
//-----------------------------------------------------------------------------

void TCppDeleteExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	assert(item != NULL);
	assert(item->IsInList() == FALSE && item->ItemParent() == NULL);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store passed pointer in the right data field.
	assert(item_destination == -1 || item_destination == cexpr_chdest_delete_oprnd);
	if (m_operand != NULL)
		delete m_operand;

	assert(item->IsExpression() == TRUE);
	m_operand = (TCppExpressionBase*)item;

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

//-----------------------------------------------------------------------------
//  ====================  TCppTypeidExpr  =========================
//-----------------------------------------------------------------------------

void TCppTypeidExpr::SetDataTypeOperand(TCppItemBase *item)
{
	// This method expects that the TYPEID of the data type is picked.
	assert(item != NULL && item->ItemParent() != NULL && item->IsDataType() == TRUE);

	if (m_expr_operand == TRUE && m_operand != NULL)
		delete m_operand;

	m_expr_operand = FALSE;
	m_data_type = (TCppDataTypeBase*)item;
}

void TCppTypeidExpr::SetStaticTypeStruct(TCppItemBase *item)
{
	assert(item != NULL && item->ItemParent() != NULL && item->ItemType() == cxx_data_field);

	if (m_dynamic_result == TRUE && m_type_resolution_code != NULL)
		delete m_type_resolution_code;

	m_dynamic_result = FALSE;
	m_static_type_struct = (TCppDataFieldDefn*)item;
}
void TCppTypeidExpr::AddChildItem(TCppItemBase *item, int item_destination)
{
	// This method expects that typeid of expression result is picked.
	assert(item != NULL && item->IsInList() == FALSE && item->ItemParent() == NULL);
	assert(item->IsExpression() == TRUE);

	// Set the parent field in the passed item.
	item->m_cib_parent = this;

	// Store the passed pointer in the right data field.
	if (item_destination == cexpr_chdest_typeid_oprnd)
	{
		if (m_expr_operand == TRUE && m_operand != NULL)
			delete m_operand;

		m_expr_operand = TRUE;
		m_operand = (TCppExpressionBase*)item;
	}
	else if (item_destination == cexpr_chdest_typeid_dyn_code)
	{
		if (m_dynamic_result == TRUE && m_type_resolution_code != NULL)
			delete m_type_resolution_code;

		m_dynamic_result = TRUE;
		m_type_resolution_code = item;
	}
	else
	{
		// The destination is bogus. Do not do anything.
		assert(FALSE);
		return;
	}

	// Send notification to the parent if any.
	if (m_cib_parent != NULL)
		m_cib_parent->NewIndirectChildNotification(item);
}

void TCppTypeidExpr::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (m_expr_operand == TRUE)
	{
		// Current expression picks up the typeid of the result of computation.
		iter.m_special_count = 0;
		iter.m_curr_child = m_operand;

		if (iter.m_curr_child == NULL && m_dynamic_result == TRUE)
		{
			iter.m_special_count = 1;
			iter.m_curr_child = m_type_resolution_code;

			if (iter.m_curr_child == NULL)
				iter.m_special_count = 2;
		}
	}
	else if (m_dynamic_result == TRUE)
	{
		// Current expression contains the type resolution code.
		iter.m_special_count = 1;
		iter.m_curr_child = m_type_resolution_code;

		if (iter.m_curr_child == NULL)
			iter.m_special_count = 2;
	}
	else
	{
		// Current expression does not have children.
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppTypeidExpr::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	if (curr_chld == NULL)
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
	else if (m_expr_operand == TRUE && curr_chld == m_operand)
	{
		iter.m_special_count = 0;
		iter.m_curr_child = curr_chld;
	}
	else if (m_dynamic_result == TRUE && curr_chld == m_type_resolution_code)
	{
		iter.m_special_count = 1;
		iter.m_curr_child = curr_chld;
	}
	else
	{
		iter.m_special_count = 2;
		iter.m_curr_child = NULL;
	}
}

void TCppTypeidExpr::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	if (iter.m_curr_child != NULL)
	{
		if (iter.m_special_count == 0 && m_dynamic_result == TRUE)
		{
			// Current expression contains the type resolution code.
			iter.m_special_count = 1;
			iter.m_curr_child = m_type_resolution_code;

			if (iter.m_curr_child == NULL)
				iter.m_special_count = 2;
		}
		else
		{
			iter.m_special_count = 2;
			iter.m_curr_child = NULL;
		}
	}
}

//----------------------------------------------------------------------------
//  ====================  TCppObjectsFactory  ======================
//----------------------------------------------------------------------------

TCppItemBase *TCppObjectsFactory::AllocateCppObject(TCppItemType defn_type, ID id)
{
	// Create object of the appropriate class depending on the requested type.
	TCppItemBase *item = NULL;
	switch (defn_type)
	{
		//
		//  High level conditional parsing analysis.
		//
		case cxx_cond_pars_param:		item = new TCppCondParsParam(id);
										break;

		//
		//  Definitions group 1.
		//
		case cxx_built_in_type:				item = new TCppBuiltInTypeDefn(id);
										break;
		case cxx_bit_field_type:			item = new TCppBitFieldTypeDefn(id);
										break;
		case cxx_cv_ptr_ref_type:			item = new TCppCvPtrRefTypeDefn(id);
										break;
		case cxx_array_type:				item = new TCppArrayTypeDefn(id);
										break;
		case cxx_data_type_alias:			item = new TCppDataTypeAliasDefn(id);
										break;

		//
		//  Definitions group 2.
		//
		case cxx_enum_type:				item = new TCppEnumTypeDefn(id);
										break;
		case cxx_enum_member:			item = new TCppEnumMemberDefn(id);
										break;
		case cxx_struct_type:				item = new TCppStructTypeDefn(id);
										break;
		case cxx_base_class_spec:			item = new TCppBaseClassSpec(id);
										break;
		case cxx_field_align_spec:			item = new TCppFieldAlignSpec(id);
										break;
		case cxx_friend_spec:				item = new TCppFriendSpec(id);
										break;
		case cxx_data_field:				item = new TCppDataFieldDefn(id);
										break;
		case cxx_data_field_initializer:		item = new TCppDataFieldInitializer(id);
										break;

		//
		//  Definitions group 3.
		//
		case cxx_function_type:				item = new TCppFunctionTypeDefn(id);
											break;
		case cxx_function_type_alias:			item = new TCppFunctionTypeAliasDefn(id);
											break;
		case cxx_code_entry_point:				item = new TCppCodeEntryPointDefn(id);
											break;
		case cxx_possible_exception_spec:		item = new TCppPossibleExceptionSpec(id);
											break;

		//
		//  Definitions group 4.
		//
		//  Note that global namespace cannot be created using this factory.
		//
		case cxx_namespace:					item = new TCppNamespaceDefn(id);
											break;
		case cxx_namespace_alias:				item = new TCppNamespaceAliasDefn(id);
											break;
		case cxx_using_declaration:				item = new TCppUsingDeclarationDefn(id);
											break;
		case cxx_using_directive:				item = new TCppUsingDirectiveDefn(id);
											break;
		case cxx_asm_insert:					item = new TCppAsmInsertDefn(id);
											break;
		case cxx_linkage_spec:				item = new TCppLinkageSpec(id);
											break;

		//
		//  Templates group 1.
		//
		case cxx_class_template:				item = new TCppClassTemplateDefn(id);
											break;
		case cxx_class_template_spec:			item = new TCppClassTemplateSpecialization(id);
											break;
		case cxx_class_template_inst:			item = new TCppClassTemplateInstantiation(id);
											break;
		case cxx_func_template:				item = new TCppFunctionTemplateDefn(id);
											break;
		case cxx_func_template_spec:			item = new TCppFunctionTemplateSpecialization(id);
											break;
		case cxx_func_template_inst:			item = new TCppFunctionTemplateInstantiation(id);
											break;

		//
		//	Templates group 2.
		//
		case cxx_template_type_param:			item = new TCppTemplateTypeParamDefn(id);
											break;
		case cxx_template_template_param:		item = new TCppTemplateTemplateParamDefn(id);
											break;
		case cxx_template_type_argument:		item = new TCppTemplateTypeArgument(id);
											break;
		case cxx_template_template_argument:	item = new TCppTemplateTemplateArgument(id);
											break;

		//
		//	Unresolved objects.
		//
		case cxx_unresolved_name_info:		item = new TCppUnresolvedNameInfo(id);
											break;
		case cxx_unresolved_data_type:			item = new TCppUnresolvedDataType(id);
											break;
		case cxx_unresolved_struct_type:		item = new TCppUnresolvedStructType(id);
											break;
		case cxx_unresolved_data_field:			item = new TCppUnresolvedDataField(id);
											break;
		case cxx_unresolved_code_entry_point:	item = new TCppUnresolvedCodeEntryPoint(id);
											break;
		case cxx_unresolved_class_template:		item = new TCppUnresolvedClassTemplate(id);
											break;
		case cxx_unresolved_func_template:		item = new TCppUnresolvedFunctionTemplate(id);
											break;

		//
		//  Statements group 1.
		//
		case cxx_empty_stmt:				item = new TCppEmptyStatement(id);
										break;
		case cxx_if_stmt:					item = new TCppIfStatement(id);
										break;
		case cxx_switch_stmt:				item = new TCppSwitchStatement(id);
										break;
		case cxx_for_stmt:				item = new TCppForStatement(id);
										break;
		case cxx_while_stmt:				item = new TCppWhileStatement(id);
										break;
		case cxx_do_while_stmt:			item = new TCppDoWhileStatement(id);
										break;

		//
		//  Statements group 2.
		//
		case cxx_goto_stmt:				item = new TCppGotoStatement(id);
										break;
		case cxx_break_stmt:				item = new TCppBreakStatement(id);
										break;
		case cxx_continue_stmt:			item = new TCppContinueStatement(id);
										break;
		case cxx_return_stmt:				item = new TCppReturnStatement(id);
										break;

		//
		//  Statements group 3.
		//
		case cxx_block:					item = new TCppBlockDefn(id);
										break;
		case cxx_try_block:				item = new TCppTryBlockDefn(id);
										break;
		case cxx_catch_block:				item = new TCppCatchBlockDefn(id);
										break;
		case cxx_light_block:				item = new TCppLightBlockDefn(id);
										break;
		case cxx_label:					item = new TCppLabelDefn(id);
										break;
		case cxx_case_label:				item = new TCppCaseLabelDefn(id);
										break;

		//
		//  Expressions group 1.
		//
		case cxx_operand_expr:			item = new TCppOperandExpr(id);
										break;
		case cxx_unary_expr:				item = new TCppUnaryExpr(id);
										break;
		case cxx_binary_expr:				item = new TCppBinaryExpr(id);
										break;
		case cxx_multiop_expr:			item = new TCppMultiOpExpr(id);
										break;
		case cxx_assignment_expr:			item = new TCppAssignmentExpr(id);
										break;

		//
		//	Expressions group 2.
		//
		case cxx_throw_expr:				item = new TCppThrowExpr(id);
										break;
		case cxx_conditional_expr:			item = new TCppConditionalExpr(id);
										break;
		case cxx_field_access_expr:		item = new TCppFieldAccessExpr(id);
										break;
		case cxx_field_access_ptr_expr:		item = new TCppFieldAccessPtrExpr(id);
										break;
		case cxx_array_index_expr:		item = new TCppArrayIndexExpr(id);
										break;
		case cxx_function_call_expr:		item = new TCppFunctionCallExpr(id);
										break;

		//
		//	Expressions group 3.
		//
		case cxx_cast_expr:				item = new TCppCastExpr(id);
										break;
		case cxx_sizeof_expr:				item = new TCppSizeofExpr(id);
										break;
		case cxx_new_expr:				item = new TCppNewExpr(id);
										break;
		case cxx_delete_expr:				item = new TCppDeleteExpr(id);
										break;
		case cxx_typeid_expr:				item = new TCppTypeidExpr(id);
										break;

		//
		//  Other group.
		//
		default:
			assert(FALSE);
			break;
	}

	// Return the result of allocation (maybe NULL).
	return(item);
}

//------------------------------------------------------------------------------
//  ======================  TCppDatabase  =========================
//------------------------------------------------------------------------------

TCppDatabase::TCppDatabase(ID parsing_id)
		: THighLevelDatabase(parsing_id)
{
	m_global_namesp.m_owner_database = this;
	m_next_object_ident = CPP_OBJECT_IDS_BASE;

	memset(m_built_in_cpp_object_ptrs, 0, sizeof(m_built_in_cpp_object_ptrs));
	m_cpp_object_ptrs_expansion_error = FALSE;
	m_cpp_object_ptrs_double_pub_error = FALSE;
}

void TCppDatabase::ClearDatabase()
{
	m_global_namesp.Clear();

	m_global_strings_dict.DeleteAll();
	m_next_object_ident = CPP_OBJECT_IDS_BASE;

	m_cpp_object_ptrs.ReleaseData();
	memset(m_built_in_cpp_object_ptrs, 0, sizeof(m_built_in_cpp_object_ptrs));
	m_cpp_object_ptrs_expansion_error = FALSE;
	m_cpp_object_ptrs_double_pub_error = FALSE;
}

bool TCppDatabase::PrepareAndSetBitFieldTypeName(TCppBitFieldTypeDefn *bft_object, bool named_base_type)
{
	assert(bft_object != NULL);

	// Prepare name buffer of an appropriate length.
	wchar_t buff40[40];
	TTextBuffer256 name_buffer;
	const wchar_t *base_type_name = GetBaseTypeName(bft_object->m_base_type, buff40, named_base_type);
	if (EnlargeCppNameBuffer(bft_object, name_buffer, wcslen(base_type_name)+40) == FALSE)
		return(FALSE);

	// Buffer for the name has enough space.
	if (bft_object->m_bit_length > 0)
	{
		// Use simple procedure.
		swprintf(name_buffer.DataPtr(), name_buffer.NumAllocedItems(), L"%s: %hd", base_type_name, bft_object->m_bit_length);
	}
	else
	{
		// The value is expressed using a non const expression. Use an unique name.
		if (bft_object->ItemId() == 0)
			bft_object->m_cib_id = m_next_object_ident++;
		swprintf(name_buffer.DataPtr(), name_buffer.NumAllocedItems(), L"%s: expr_@%lu", base_type_name, bft_object->ItemId());
	}

	return(SetupPersistentName(bft_object, name_buffer));
}

bool TCppDatabase::PrepareAndSetCvPtrRefTypeName(TCppCvPtrRefTypeDefn *cvpr_object, bool named_base_type)
{
	assert(cvpr_object != NULL);

	// Pick up the name of the base type.
	wchar_t buff40[40];
	const wchar_t *base_type_name = GetBaseTypeName(cvpr_object->m_base_type, buff40, named_base_type);

	TTextBuffer256 name_buffer;
	if (cvpr_object->m_layers.NumItems() <= 0)
	{
		// The modifier layers are missing.
		name_buffer.Append(base_type_name);
		name_buffer.Append(L" (no-mdf-layers)");
	}
	else
	{
		// Layers are present. First layer differs from all subsequent layers. It may have only const/volatile flags
		// without ptr/ref flags. If this is so, it should be processed differently.
		int first_layer_inx = 0;
		TCppCvPtrRefSpec &first_spec = cvpr_object->m_layers[0];
		if (first_spec.IsEmpty() == FALSE && first_spec.cvpr_pointer == FALSE && first_spec.cvpr_reference == FALSE)
		{
			// Prepend name of the base type with the CV prefix.
			name_buffer.Append(GetConstVolatileFlagsTextRepr(first_spec));
			name_buffer.Append(L" ");
			name_buffer.Append(base_type_name);
			first_layer_inx = 1;
		}
		else
		{
			// Start right from the name of the base type.
			name_buffer.Append(base_type_name);
		}

		for (int ilayer=first_layer_inx; ilayer<cvpr_object->m_layers.NumItems(); ++ilayer)
		{
			TCppCvPtrRefSpec &spec = cvpr_object->m_layers[ilayer];
			if (spec.IsEmpty() == TRUE)
			{
				// For some unclear reason all flags are missing.
				name_buffer.Append(L" (empty-mdf-layer)");
				continue;
			}

			// The flags are available. Process the ptr and ref flags first.
			if (spec.cvpr_pointer == TRUE && spec.cvpr_reference == TRUE)
			{
				name_buffer.Append(L" (bogus layer with both *& flags at the same time)");
			}
			else if (spec.cvpr_pointer != FALSE)
			{
				// This is pointer modiffication layer.
				if (spec.cvpr_struct != NULL)
				{
					name_buffer.Append(L" ");
					name_buffer.Append(GetStructTypeName(spec.cvpr_struct));
					name_buffer.Append(L"::*");
				}
				else
				{
					name_buffer.Append(L"*");
				}
			}
			else if (spec.cvpr_reference != FALSE)
			{
				// This is reference modiffication layer.
				if (spec.cvpr_struct != NULL)
				{
					name_buffer.Append(L" ");
					name_buffer.Append(GetStructTypeName(spec.cvpr_struct));
					name_buffer.Append(L"::&");
				}
				else
				{
					name_buffer.Append(L" &");
				}
			}
			else
			{
				name_buffer.Append(L" (bogus layer without *& flags)");
			}

			// Process const and volatile modifiers of the current layer.
			const wchar_t *cv_suffix = GetConstVolatileFlagsTextRepr(spec);
			if (cv_suffix != NULL)
			{
				name_buffer.Append(L" ");
				name_buffer.Append(cv_suffix);
			}
		}
	}

	if (name_buffer.GetXpndError() == TRUE)
	{
		cvpr_object->SetNameInfo(L"Name_Alloc_Error", 0, cdpr_none);
		return(FALSE);
	}

	// Name generation in the local buffer succeeeded.
	return(SetupPersistentName(cvpr_object, name_buffer));
}

bool TCppDatabase::PrepareAndSetArrayTypeName(TCppArrayTypeDefn *array_object, bool named_base_type)
{
	assert(array_object != NULL);
	int num_dims = array_object->m_dimensions.NumItems();

	// Pick up the name of the base type.
	wchar_t buff40[40];
	const wchar_t *base_type_name = GetBaseTypeName(array_object->m_base_type, buff40, named_base_type);

	TTextBuffer256 name_buffer;
	name_buffer.Append(base_type_name);

	if (num_dims <= 0)
	{
		// Dimensions are missing. The object is bogus.
		name_buffer.Append(L" [<no-dims>]");
	}
	else if (num_dims == 1 && array_object->m_dimensions[0].m_num_elements == 0)
	{
		// Array is an empty vector.
		name_buffer.Append(L" []");
	}
	else
	{
		// This is generic case. Use the complex procedure.
		name_buffer.Append(L" [");

		for (int idim=0; idim<num_dims; ++idim)
		{
			wchar_t small_buff[40];
			__int64 dim_value = array_object->m_dimensions[idim].m_num_elements;
			if (dim_value >= 0)
			{
				// Dimension has fixed size.
				swprintf(small_buff, 40, L"%I64d", dim_value);
			}
			else
			{
				// There is no fixed size.
				TCppExpressionBase *num_elements_expr = array_object->m_dimensions[idim].m_num_elements_expr;
				if (num_elements_expr == NULL)
					wcscpy(small_buff, L"null_num_elements_expr");
				else swprintf(small_buff, 40, L"expr_@%lu", num_elements_expr->ItemId());
			}

			name_buffer.Append(small_buff);
			if (idim != num_dims-1)
				name_buffer.Append(L", ");
		}

		name_buffer.Append(L']');
	}

	if (name_buffer.GetXpndError() == TRUE)
	{
		array_object->SetNameInfo(L"Name_Alloc_Error", 0, cdpr_none);
		return(FALSE);
	}

	// Name generation in the local buffer succeeeded.
	return(SetupPersistentName(array_object, name_buffer));
}

bool TCppDatabase::PrepareAndSetUnnamedFuncTypeName(TCppFunctionTypeDefn *func_type_object, bool named_base_type)
{
	assert(func_type_object != NULL);

	// Pick up the name of the return value type.
	wchar_t buff40[40];
	const wchar_t *return_value_type_name = GetBaseTypeName(func_type_object->m_return_value_type, buff40, named_base_type);

	// Name of the object will be assembled in the buffer.
	TTextBuffer256 name_buffer;
	name_buffer.Append(return_value_type_name);
	name_buffer.Append(L" (");

	// Iterate function params if any.
	bool first_param = TRUE;
	for (TCppDirectChildrenIter iter(func_type_object); iter; ++iter)
	{
		TCppDataFieldDefn *data_field = iter.CurrItemAsDataField(strgc_param);
		if (data_field != NULL)
		{
			if (first_param == TRUE)
				first_param = FALSE;
			else name_buffer.Append(L", ");

			// This is parameter of the function.
			name_buffer.Append(GetFuncParamTypeName(data_field->m_field_type));
		}
	}

	name_buffer.Append(L')');
	if (name_buffer.GetXpndError() == TRUE)
	{
		func_type_object->SetNameInfo(L"Name_Alloc_Error", 0, cdpr_none);
		return(FALSE);
	}

	// Name generation in the local buffer succeeeded.
	return(SetupPersistentName(func_type_object, name_buffer));
}

bool TCppDatabase::PrepareAndSetCaseLabelName(TCppCaseLabelDefn *case_lab_object)
{
	assert(case_lab_object != NULL);

	if (case_lab_object->m_default == TRUE)
	{
		// Setup a simple static string.
		case_lab_object->SetNameInfo(L"default:", 0, cdpr_none);
		return(TRUE);
	}
	else if (case_lab_object->m_const_value == TRUE)
	{
		// Show the value of the const expression.
		wchar_t name_buffer[80];
		swprintf(name_buffer, 80, L"%I64d:", case_lab_object->m_value);
		return(SetupPersistentName(case_lab_object, name_buffer));
	}
	else if (case_lab_object->m_value_expr != NULL)
	{
		// Show the id of the expr that defines the value of the case label.
		wchar_t name_buffer[80];
		swprintf(name_buffer, 40, L"_expr_@%lu:", case_lab_object->m_value_expr->ItemId());
		return(SetupPersistentName(case_lab_object, name_buffer));
	}
	else
	{
		// The expr object is missing.
		case_lab_object->SetNameInfo(L"<null_value_expr>:", 0, cdpr_none);
		return(TRUE);
	}
}

bool TCppDatabase::PrepareAndSetParserGeneratedName(TCppDefnBase *defn_object, const wchar_t *name_proto)
{
	assert(defn_object != NULL && name_proto !=  NULL);

	// Setup the id of the object if it is not there yet.
	if (defn_object->ItemId() == 0)
		defn_object->m_cib_id = m_next_object_ident++;

	// Prepare buffer for the name of an appropriate length.
	TTextBuffer256 name_buffer;
	if (EnlargeCppNameBuffer(defn_object, name_buffer, wcslen(name_proto)+40) == FALSE)
		return(FALSE);

	// Name buffer has enough space. Format the name.
	swprintf(name_buffer.DataPtr(), name_buffer.NumAllocedItems(), L"%s_@%lu", name_proto, defn_object->ItemId());
	ChangeSpacesToUnderscores(name_buffer);
	return(SetupPersistentName(defn_object, name_buffer));
}

wchar_t *TCppDatabase::CreateDestructorName(const wchar_t *class_name)
{
	// Prepare name buffer with an appropriate length.
	TTextBuffer256 name_buffer;
	if (name_buffer.ReserveSpace(wcslen(class_name)+4) == FALSE)
		return(NULL);

	// Buffer for the destructor name has enough space.
	swprintf(name_buffer.DataPtr(), name_buffer.NumAllocedItems(), L"~%s", class_name);
	return(m_global_strings_dict.RegisterStr(name_buffer));
}

wchar_t *TCppDatabase::CreateTemplateIdName(const wchar_t *template_name, TList *template_arguments)
{
	// Allocate the name buffer. Place name of the template and its arguments there.
	TTextBuffer256 name_buffer;
	name_buffer.Append(template_name);
	AppendTemplateArguments(name_buffer, template_arguments);

	// Check the results of buffering.
	if (name_buffer.GetXpndError() == TRUE)
		return(NULL);

	// Return the persistent name.
	return(m_global_strings_dict.RegisterStr(name_buffer));
}

wchar_t *TCppDatabase::CreateOverloadedOperatorName(TOperatorToken operation, bool op_template_form, TList *template_arguments)
{
	// Get text representation of the operator name.
	const wchar_t *operation_name = TLexema::GetArithmOperationText(operation);
	if (operation == opr_funcall)
		operation_name = L"()";

	// The name of the object will be assembled in the buffer.
	TTextBuffer256 name_buffer;
	name_buffer.Append(L"operator ");
	name_buffer.Append(operation_name);
	if (op_template_form == TRUE)
	{
		// Append arguments or empty angle brackets if passed list of arguments is NULL.
		AppendTemplateArguments(name_buffer, template_arguments);
	}

	// Check the results of buffering.
	if (name_buffer.GetXpndError() == TRUE)
		return(NULL);

	// Return the persistent name.
	return(m_global_strings_dict.RegisterStr(name_buffer));
}

wchar_t *TCppDatabase::CreateOverloadedConversionName(TCppDataTypeBase *dest_type, bool conv_template_form, TList *template_arguments)
{
	wchar_t buff40[40];
	const wchar_t *dest_type_name = GetBaseTypeName(dest_type, buff40, TRUE);

	// The name of the object will be assembled in the buffer.
	TTextBuffer256 name_buffer;
	name_buffer.Append(L"operator ");
	name_buffer.Append(dest_type_name);
	if (conv_template_form == TRUE)
	{
		// Append arguments or empty angle brackets if passed list of arguments is NULL.
		AppendTemplateArguments(name_buffer, template_arguments);
	}

	// Check the results of buffering.
	if (name_buffer.GetXpndError() == TRUE)
		return(NULL);

	// Return the persistent name.
	return(m_global_strings_dict.RegisterStr(name_buffer));
}

wchar_t *TCppDatabase::CreateAlternativeName(const wchar_t *name_proto, ID name_suffix)
{
	// Prepare name buffer of an appropriate length.
	TTextBuffer256 name_buffer;
	if (name_buffer.ReserveSpace(wcslen(name_proto)+40) == FALSE)
		return(NULL);

	// Name buffer has enough space. Format the name.
	swprintf(name_buffer.DataPtr(), name_buffer.NumAllocedItems(), L"%s_@%lu", name_proto, name_suffix);
	ChangeSpacesToUnderscores(name_buffer);
	return(m_global_strings_dict.RegisterStr(name_buffer));
}

wchar_t *TCppDatabase::CreateAlternativeNameEx(const wchar_t *name_prefix, const wchar_t *name_proto, ID name_suffix)
{
	// Prepare name buffer of an appropriate length.
	TTextBuffer256 name_buffer;
	if (name_buffer.ReserveSpace(wcslen(name_prefix)+wcslen(name_proto)+40) == FALSE)
		return(NULL);

	// Name buffer has enough space. Format the name.
	swprintf(name_buffer.DataPtr(), name_buffer.NumAllocedItems(), L"%s_%s_@%lu", name_prefix, name_proto, name_suffix);
	ChangeSpacesToUnderscores(name_buffer);
	return(m_global_strings_dict.RegisterStr(name_buffer));
}

wchar_t *TCppDatabase::CreateUnresolvedObjectName(const wchar_t *name_prefix, TList *unres_name_infos, const wchar_t *name_proto, ID name_suffix)
{
	// The name will be assembled in the buffer.
	TTextBuffer256 name_buffer;
	if (name_prefix != NULL)
	{
		// Caller has passed the prefix.
		name_buffer.Append(name_prefix);
		name_buffer.Append(L"_");
	}

	bool layer_names_are_present = FALSE;
	if (unres_name_infos != NULL)
	{
		// List of the unresolved layer names is present. Nevertheless it might be empty.
		for (TListIter<TCppUnresolvedNameInfo> iter(unres_name_infos); iter; ++iter)
		{
			if (layer_names_are_present == TRUE)
				name_buffer.Append(L"::");

			assert(iter.CurrItem().ItemType() == cxx_unresolved_name_info);
			const wchar_t *name = iter.CurrItem().DefnName();
			if (name == NULL)
				name = L"<null_name>";
			else if (name[0] == 0)
				name = L"<empty_name>";

			name_buffer.Append(name);
			layer_names_are_present = TRUE;
		}
	}

	if (name_proto != NULL)
	{
		if (layer_names_are_present == TRUE)
			name_buffer.Append(L"::");

		name_buffer.Append((name_proto[0] != 0) ? name_proto : L"<empty_name>");
	}

	if (name_buffer.IsEmpty() == TRUE || unres_name_infos != NULL || name_proto != NULL)
		name_buffer.Append(L"_");

	wchar_t loc_buff[40];
	swprintf(loc_buff, 40, L"@%lu", name_suffix);
	name_buffer.Append(loc_buff);

	// Check the results of buffering.
	if (name_buffer.GetXpndError() == TRUE)
		return(NULL);

	// Generation of the name in the local buffer succeeeded.
	ChangeSpacesToUnderscores(name_buffer);
	return(m_global_strings_dict.RegisterStr(name_buffer));
}

const wchar_t *TCppDatabase::GenerateQualifiedName(TTextBuffer256 &name_buffer, TCppDefnBase *defn, bool want_buffer_overflow_subst)
{
	// Kill previous contents of the buffer if any.
	name_buffer.ClearBuffer();

	if (defn->ItemType() == cxx_global_namespace)
	{
		name_buffer.Append(L"::<<global_namespace>>::");
		return(name_buffer.DataPtr());
	}

	TCppItemBase *curr_item = defn;
	while (curr_item != NULL)
	{
		if (curr_item != defn)
			name_buffer.Insert(0, L"::");

		if (curr_item->ItemType() == cxx_global_namespace)
		{
			// Passed definition is published in the database.
			assert(curr_item->ItemParent() == NULL);
			break;
		}
		else if (curr_item->IsDefinition() == TRUE)
		{
			// Current item is a definition. All definitions have names.
			const wchar_t *name = ((TCppDefnBase*)curr_item)->DefnName();
			if (name == NULL)
				name = L"<null_name>";
			else if (name[0] == 0)
				name = L"<empty_name>";

			name_buffer.Insert(0, name);
		}
		else
		{
			// Current item does not have any name. Show the name of the item type.
			name_buffer.Insert(0, curr_item->ItemTypeEnumName());
		}

		// Shift to the upper item.
		curr_item = curr_item->ItemParent();
		if (curr_item == defn)
		{
			name_buffer.Insert(0, L"<<parent_items_loop>>::");
			break;
		}
	}

	if (curr_item == NULL)
	{
		// Passed definition is not part of the cpp database.
		name_buffer.Insert(0, L"<<unpublished_item>>::");
	}

	// Check the results of buffering.
	if (name_buffer.GetXpndError() == TRUE)
	{
		if (want_buffer_overflow_subst == FALSE)
			return(NULL);

		name_buffer.ClearBuffer();
		name_buffer.Append(L"<Error_Generating_Qualified_Name>");
	}

	return(name_buffer.DataPtr());
}

const wchar_t *TCppDatabase::GetBaseTypeName(const TCppDefnBase *base_type_defn, wchar_t *buff40, bool named_base_type)
{
	// Return real name or its various substitutions.
	if (base_type_defn == NULL)
		return(L"null_base_type");

	if (named_base_type == FALSE)
	{
		// Prepare the cooked name of the type.
		if (base_type_defn->IsDataType() == TRUE)
		{
			swprintf(buff40, 40, L"@DataType%lu", base_type_defn->ItemId());
		}
		else if (base_type_defn->IsFunctionType() == TRUE)
		{
			swprintf(buff40, 40, L"@FunctionType%lu", base_type_defn->ItemId());
		}
		else
		{
			swprintf(buff40, 40, L"@Type%lu", base_type_defn->ItemId());
		}

		// The name was prepared in the buffer.
		return(buff40);
	}

	const wchar_t *base_type_name = base_type_defn->DefnName();
	if (base_type_name == NULL)
		return(L"null_base_type_name");
	else if (base_type_name[0] == 0)
		return(L"empty_base_type_name");

	// Passed base type has a valid name.
	return(base_type_name);
}

const wchar_t *TCppDatabase::GetStructTypeName(const TCppDataTypeBase *struct_type_defn)
{
	// Return either a real name or its substitutions.
	if (struct_type_defn == NULL)
		return(L"null_struct_type");

	const wchar_t *struct_type_name = struct_type_defn->DefnName();
	if (struct_type_name == NULL)
		return(L"null_struct_type_name");
	else if (struct_type_name[0] == 0)
		return(L"empty_struct_type_name");

	// Passed struct type has some valid name.
	return(struct_type_name);
}

const wchar_t *TCppDatabase::GetFuncParamTypeName(const TCppDataTypeBase *param_type_defn)
{
	// Return either a real name or its substitutions.
	if (param_type_defn == NULL)
		return(L"null_param_type");

	const wchar_t *param_type_name = param_type_defn->DefnName();
	if (param_type_name == NULL)
		return(L"null_param_type_name");
	else if (param_type_name[0] == 0)
		return(L"empty_param_type_name");

	// Passed param type has some valid name.
	return(param_type_name);
}

const wchar_t *TCppDatabase::GetTemplateTypeArgumentName(const TCppDataTypeBase *template_type_argument)
{
	// Return either a real name or its substitutions.
	if (template_type_argument == NULL)
		return(L"null_template_type_argument");

	const wchar_t *argument_name = template_type_argument->DefnName();
	if (argument_name == NULL)
		return(L"null_template_type_argument_name");
	else if (argument_name[0] == 0)
		return(L"empty_template_type_argument_name");

	// Passed type argument has a valid name.
	return(argument_name);
}

const wchar_t *TCppDatabase::GetTemplateTemplateArgumentName(const wchar_t *argument_name)
{
	// Return either a real name or its substitutions.
	if (argument_name == NULL)
		return(L"null_template_template_argument_name");
	else if (argument_name[0] == 0)
		return(L"empty_template_template_argument_name");

	// Passed template template argument name is fine.
	return(argument_name);
}

const wchar_t *TCppDatabase::GetConstVolatileFlagsTextRepr(const TCppCvPtrRefSpec &spec)
{
	if (spec.cvpr_const != FALSE && spec.cvpr_volatile != FALSE)
		return(L"const volatile");
	else if (spec.cvpr_const != FALSE)
		return(L"const");
	else if (spec.cvpr_volatile != FALSE)
		return(L"volatile");

	// Both flags are missing.
	return(NULL);
}

const wchar_t *TCppDatabase::GetDeclTypeSpecPrefixFriendlyName(TCppDeclTypeSpecPrefix pfx)
{
	if (pfx == declpr_none)
		return(L"declpr_none");
	else if (pfx == declpr_typedef)
		return(L"typedef");
	else if (pfx == declpr_friend)
		return(L"friend");

	return(L"declpr_bogus_value");
}

const wchar_t *TCppDatabase::GetFunctionAttributeFriendlyName(TCppFunctionAttr attr)
{
	if (attr == fcta_none)
		return(L"fcta_none");
	else if (attr == fcta_inline)
		return(L"inline");
	else if (attr == fcta_virtual)
		return(L"virtual");
	else if (attr == fcta_explicit)
		return(L"explicit");
	else if (attr == fcta_static)
		return(L"static");
	else if (attr == fcta_this_const)
		return(L"_this_const");
	else if (attr == fcta_this_volatile)
		return(L"_this_volatile");
	else if (attr == fcta_pure)
		return(L"pure");

	return(L"fcta_bogus_value");
}

const wchar_t *TCppDatabase::GetFuncAttrsMaskFriendlyName(wchar_t *buffer256, TCppFunctionAttr set_of_attrs)
{
	// Use simple approach. Expect that passed buffer is big enough.
	buffer256[0] = 0;

	bool first_flag = TRUE;
	for (int mask=1; mask != 0x10000; mask <<= 1)
	{
		if ((set_of_attrs & mask) != 0)
		{
			if (first_flag == FALSE)
				wcscat(buffer256, L" ");
			else first_flag = FALSE;

			wcscat(buffer256, GetFunctionAttributeFriendlyName((TCppFunctionAttr)mask));
		}
	}

	return(buffer256);
}

void TCppDatabase::ProcessNewChildNotification(TCppItemBase *items_subtree_root)
{
	assert(items_subtree_root != NULL);

	if (items_subtree_root->m_cib_id >= bltin_ids_base)
	{
		// Id of the passed item belongs to the special range of ids.
		int offs = items_subtree_root->m_cib_id-bltin_ids_base;
		assert(offs < (bltin_ids_max_plus1-bltin_ids_base) && m_built_in_cpp_object_ptrs[offs] == NULL);

		if (offs < (bltin_ids_max_plus1-bltin_ids_base))
			m_built_in_cpp_object_ptrs[offs] = items_subtree_root;
	}
	else
	{
		// This is an ordinary item. Check presence of the id in the passed item.
		if (items_subtree_root->ItemId() == 0)
			items_subtree_root->m_cib_id = m_next_object_ident++;

		// Check for the free space in the pointers index.
		int ptrs_index = items_subtree_root->m_cib_id-CPP_OBJECT_IDS_BASE;
		int old_ptrs_num_items = m_cpp_object_ptrs.NumItems();
		if (ptrs_index >= old_ptrs_num_items)
		{
			if (m_cpp_object_ptrs.ReserveTotalBufferSpace(ptrs_index+1) == TRUE)
			{
				int new_ptrs_num_items = m_cpp_object_ptrs.NumAllocedItems();
				m_cpp_object_ptrs.SetNumItems(new_ptrs_num_items);
				for (int inx=old_ptrs_num_items; inx<new_ptrs_num_items; ++inx)
					m_cpp_object_ptrs[inx] = NULL;
			}
			else
			{
				m_cpp_object_ptrs_expansion_error = TRUE;
			}
		}

		// Update the pointers index.
		if (ptrs_index < m_cpp_object_ptrs.NumItems())
		{
			if (m_cpp_object_ptrs[ptrs_index] == NULL)
				m_cpp_object_ptrs[ptrs_index] = items_subtree_root;
			else m_cpp_object_ptrs_double_pub_error = TRUE;
		}
	}

	// Process children of this item if any.
	for (TCppDirectChildrenIter iter(items_subtree_root); iter; ++iter)
		ProcessNewChildNotification(iter.CurrItemPtr());
}

void TCppDatabase::AppendTemplateArguments(TTextBuffer256 &name_buffer, TList *args_list)
{
	// Note that this procedure dumps arguments as they were specified.
	if (args_list == NULL || args_list->IsEmpty() == TRUE)
	{
		name_buffer.Append(L"<>");
	}
	else
	{
		name_buffer.Append(L"<");

		bool first_arg = TRUE;
		for (TListIter<TCppItemBase> iter(args_list); iter; ++iter, first_arg = FALSE)
		{
			if (first_arg == FALSE)
				name_buffer.Append(L", ");

			if (iter.CurrItem().ItemType() == cxx_template_type_argument)
			{
				// This is type parameter.
				TCppTemplateTypeArgument &arg = (TCppTemplateTypeArgument&)iter.CurrItem();
				name_buffer.Append(GetTemplateTypeArgumentName(arg.m_argument_value));
			}
			else if (iter.CurrItem().ItemType() == cxx_template_template_argument)
			{
				// This is template template parameter.
				TCppTemplateTemplateArgument &arg = (TCppTemplateTemplateArgument&)iter.CurrItem();
				name_buffer.Append(GetTemplateTemplateArgumentName(arg.GetReferencedDefnName()));
			}
			else if (iter.CurrItem().IsExpression() == TRUE)
			{
				// This is a data parameter.
				if (iter.CurrItem().ItemType() == cxx_operand_expr)
				{
					TCppOperandExpr &oprnd = (TCppOperandExpr&)iter.CurrItem();
					if (oprnd.m_oprnd_type == eopr_num_literal)
					{
						wchar_t buff40[40];
						swprintf(buff40, 40, L"%I64d", oprnd.m_num_value);
						name_buffer.Append(buff40);
					}
					else
					{
						name_buffer.Append(L"oprnd_expr");
					}
				}
				else
				{
					name_buffer.Append(L"arithm_expr");
				}
			}
			else
			{
				// This is a bogus param. Write the type of the object to clarify the situation.
				name_buffer.Append(TCppItemBase::GetItemStructName(iter.CurrItem().ItemType()));
			}
		}

		name_buffer.Append(L">");
	}
}

void TCppDatabase::ChangeSpacesToUnderscores(TTextBuffer256 &name_buffer)
{
	for (wchar_t *data_ptr=name_buffer.DataPtr(); data_ptr[0] != 0; ++data_ptr)
	{
		if (*data_ptr == L' ')
			*data_ptr = L'_';
	}
}

bool TCppDatabase::EnlargeCppNameBuffer(TCppDefnBase *defn, TTextBuffer256 &buffer, int extra_wchars_needed)
{
	if (buffer.ReserveSpace(extra_wchars_needed) == FALSE)
	{
		// There is no more memory or this is severe data corruption.
		defn->SetNameInfo(L"Name_Alloc_Error", 0, cdpr_none);
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TCppDatabase::SetupPersistentName(TCppDefnBase *defn, const wchar_t *non_persistent_name)
{
	const wchar_t *persistent_name = m_global_strings_dict.RegisterStr(non_persistent_name);
	if (persistent_name == NULL)
	{
		// Setup some dummy name.
		defn->SetNameInfo(L"Name_Alloc_Error", 0, cdpr_none);
		return(FALSE);
	}

	// Success.
	defn->SetNameInfo(persistent_name, 0, cdpr_none);
	return(TRUE);
}

//------------------------------------------------------------------------------
//  =====================  TCppDataStorage  ========================
//------------------------------------------------------------------------------

THighLevelDatabase *TCppDataStorage::AllocateParsSessDatabase(ID parsing_id)
{
	assert(parsing_id != 0 && parsing_id != TProjectParsSessId);
	TCppDatabase *new_database = new TCppDatabase(parsing_id);
	if (new_database == NULL)
		return(NULL);

	// Allocation succeeded.
	new_database->m_data_store_owned = TRUE;
	m_database_buckets[parsing_id & (NUM_DATABASE_BUCKETS-1)].AppendItem(new_database);
	m_latest_database = new_database;
	return(new_database);
}


