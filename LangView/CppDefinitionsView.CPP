//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "LangView/CppDefinitionsView.H"
#include  "LangView/LangViewResources.H"

// ------------------------------------------------------------------------------
//   =================  TLangViewCppItemListBoxItem  ====================
// ------------------------------------------------------------------------------

void TLangViewCppItemListBoxItem::FormatItem(TBasicListBoxObject &owner, int level_delta)
{
	// Get rid of the previous contents if any.
	ClearBkgrAndCells(owner);

	// Figure out the nesting level of the item. There is a trick here because the global namespace as a cpp item
	// is typically not displayed. The loop below assigns level 0 to the children of the global namespace.
	int level = -1;
	TCppItemBase *item = m_cpp_item->ItemParent();
	while (item != NULL)
	{
		level++;
		item = item->ItemParent();
	}

	if (level < 0)
		level = 0;

	assert(level >= level_delta);
	level -= level_delta;
	if (level >= num_name_clmns)
		level = num_name_clmns-1;

	// Display the name of the object.
	wchar_t buffer_item_name[80];
	const wchar_t *item_name = GetNameColumn(buffer_item_name, 80);
	if (item_name != NULL)
	{
		// The name is present.
		SetCellWidth(owner, level, num_name_clmns-level);
		if (item_name != buffer_item_name)
			SetExtStringToCell(owner, level, item_name);
		else SetStringToCell(owner, level, item_name);
	}

	// Display the type of the object.
	wchar_t buffer_type_name[80];
	const wchar_t *item_type_name = GetTypeColumn(buffer_type_name, 80);
	if (item_type_name != NULL)
	{
		// Type of the object is present.
		if (item_type_name != buffer_type_name)
			SetExtStringToCell(owner, TypeClmnInx(), item_type_name);
		else SetStringToCell(owner, TypeClmnInx(), buffer_type_name);
	}

	// Display the details if any.
	wchar_t buffer_details[80];
	const wchar_t *dtls = GetDetailsColumn(buffer_details, 80);
	if (dtls != NULL)
	{
		// Current object has non empty details.
		if (dtls != buffer_details)
			SetExtStringToCell(owner, DtlsClmnInx(), dtls);
		else SetStringToCell(owner, DtlsClmnInx(), buffer_details);
	}

	// Display the definition Id.
	if (m_cpp_item->ItemId() < bltin_ids_base)
		SetFmtStrToCell(owner, IdentClmnInx(), L"%lu", m_cpp_item->ItemId());
	else SetFmtStrToCell(owner, IdentClmnInx(), L"Std+%lu", m_cpp_item->ItemId()-bltin_ids_base);
}

const wchar_t *TLangViewCppItemListBoxItem::GetNameColumn(wchar_t *buffer, int buffer_len)
{
	assert(m_cpp_item != NULL);
	TCppItemType cit = m_cpp_item->ItemType();

	if (cit == cxx_block || cit == cxx_try_block || cit == cxx_catch_block)
	{
		// Blocks should display the name of the object type as their primary label.
		return(TCppItemHelper::GetItemTypeDisplayName(cit));
	}
	else if (cit == cxx_asm_insert)
	{
		// Show body of the asm insert in apostrophes.
		TCppAsmInsertDefn *asm_ins = (TCppAsmInsertDefn*)m_cpp_item;
		swprintf(buffer, buffer_len, L"\"%.*s\"", asm_ins->m_asm_insert.m_len, asm_ins->m_asm_insert.m_body);
		return(buffer);
	}

	if (m_cpp_item->IsDefinition() == TRUE)
	{
		// Display the name field.
		TCppDefnBase *defn = (TCppDefnBase*)m_cpp_item;
		return(defn->DefnName());
	}

	// Use the stnd procedure.
	return(TCppItemHelper::GetItemTypeDisplayName(cit));
}

const wchar_t *TLangViewCppItemListBoxItem::GetTypeColumn(wchar_t *buffer, int buffer_len)
{
	assert(m_cpp_item != NULL);
	switch (m_cpp_item->ItemType())
	{
		//
		// Definitions.
		//

		case cxx_struct_type:
				{
					TCppStructTypeDefn *defn = (TCppStructTypeDefn*)m_cpp_item;
					return(TCppDefnBase::GetElabTypeName(defn->DefnElabType(), TRUE));
				}

		case cxx_field_align_spec:
		case cxx_friend_spec:
				{
					// Name of the item class is already displayed in the primary column.
					return(NULL);
				}

		case cxx_data_field_initializer:
				{
					TCppDataFieldInitializer *initializer = (TCppDataFieldInitializer*)m_cpp_item;
					if (initializer->m_init_type == cinit_none)
					{
						// Do not show this for now.
						return(NULL);
					}

					return(TCppDataFieldInitializer::GetInitializerTypeEnumName(initializer->m_init_type));
				}

		case cxx_possible_exception_spec:
		case cxx_linkage_spec:
				{
					// Name of the item class is already displayed in the primary column.
					return(NULL);
				}

		//
		// Templates.
		//

		case cxx_class_template:
				{
					return(StructItemTypeName(buffer, buffer_len, L"%sTemplate"));
				}

		case cxx_class_template_spec:
				{
					return(StructItemTypeName(buffer, buffer_len, L"%sTemplateSpec"));
				}

		case cxx_class_template_inst:
				{
					return(StructItemTypeName(buffer, buffer_len, L"%sTemplateInst"));
				}

		case cxx_template_type_argument:
		case cxx_template_template_argument:
				{
					// Name of the item class is already displayed in the primary column.
					return(NULL);
				}

		//
		// Statements.
		//

		case cxx_empty_stmt:
		case cxx_if_stmt:
		case cxx_switch_stmt:
		case cxx_for_stmt:
		case cxx_while_stmt:
		case cxx_do_while_stmt:
		case cxx_goto_stmt:
		case cxx_break_stmt:
		case cxx_continue_stmt:
		case cxx_return_stmt:
		case cxx_block:
		case cxx_try_block:
		case cxx_catch_block:
		case cxx_light_block:
				{
					// Name of the item class is already displayed in the primary column.
					return(NULL);
				}

		//
		//  Expressions.
		//

		case cxx_operand_expr:
				{
					TCppOperandExpr *expr = (TCppOperandExpr*)m_cpp_item;
					assert(expr->m_operation == 0 || expr->m_operation == opr_none);

					if (expr->m_oprnd_type < 0 || expr->m_oprnd_type >= eopr_num_types)
						return(L"Bogus_Operand_Type");
					else if (expr->m_oprnd_type == eopr_none)
						return(L"Operand_Type_Is_Not_Set");

					return(TCppOperandExpr::GetOperandTypeEnumName(expr->m_oprnd_type));
				}

		case cxx_unary_expr:
		case cxx_binary_expr:
		case cxx_multiop_expr:
		case cxx_assignment_expr:
		case cxx_throw_expr:
		case cxx_conditional_expr:
		case cxx_field_access_expr:
		case cxx_field_access_ptr_expr:
		case cxx_array_index_expr:
		case cxx_function_call_expr:
		case cxx_cast_expr:
		case cxx_sizeof_expr:
		case cxx_new_expr:
		case cxx_delete_expr:
		case cxx_typeid_expr:
				{
					TCppExpressionBase *expr = (TCppExpressionBase*)m_cpp_item;
					if (expr->m_operation == 0 || expr->m_operation == opr_none)
						return(L"Operation_Is_Not_Set");

					return(TLexema::GetKeywordIdEnumName(expr->m_operation));
				}

	}

	// Use the stnd procedure.
	return(TCppItemHelper::GetItemTypeDisplayName(m_cpp_item->ItemType()));
}

const wchar_t *TLangViewCppItemListBoxItem::GetDetailsColumn(wchar_t *buffer, int buffer_len)
{
	assert(m_cpp_item != NULL);

	switch (m_cpp_item->ItemType())
	{
		//
		// Definitions 2.
		//

		case cxx_enum_type:
				{
					TCppEnumTypeDefn *defn = (TCppEnumTypeDefn*)m_cpp_item;
					if (defn->m_defined == FALSE)
						return(L"Forward-Decl");

					swprintf(buffer, buffer_len, L"NumMembers: %d.", defn->GetNumEnumMembers());
					return(buffer);
				}

		case cxx_enum_member:
				{
					swprintf(buffer, buffer_len, L"Value: %I64d.", ((TCppEnumMemberDefn*)m_cpp_item)->m_value);
					return(buffer);
				}

		case cxx_struct_type:
				{
					TCppStructTypeDefn *defn = (TCppStructTypeDefn*)m_cpp_item;
					return((defn->m_defined == FALSE) ? L"Forward-Decl" : NULL);
				}

		case cxx_base_class_spec:
				{
					TCppBaseClassSpec *spec = (TCppBaseClassSpec*)m_cpp_item;
					const wchar_t *virt_prefix = (spec->m_virtual == TRUE) ? L"virtual " : L"";
					swprintf(buffer, buffer_len, L"%s%s", virt_prefix, TCppDataFieldDefn::GetAccessLevelFriendlyName(spec->m_accs_level));
					return(buffer);
				}

		case cxx_field_align_spec:
				{
					TCppFieldAlignSpec *spec = (TCppFieldAlignSpec*)m_cpp_item;
					if (spec->m_align_value < 0)
						return(L"VariableAlignment");

					// This is fixed alignment.
					swprintf(buffer, buffer_len, L"AlignVal: %hd.", spec->m_align_value);
					return(buffer);
				}

		case cxx_friend_spec:
				{
					TCppFriendSpec *spec = (TCppFriendSpec*)m_cpp_item;
					return((spec->m_friendly_object != NULL) ? spec->m_friendly_object->DefnName() : L"null_friend_ref");
				}

		case cxx_data_field:
				{
					TCppDataFieldDefn *data_field = (TCppDataFieldDefn*)m_cpp_item;
					if (data_field->m_field_type == NULL)
						return(L"null_data_type");

					// Show names of the built in types directly.
					if (data_field->m_field_type->ItemType() == cxx_built_in_type)
						return(data_field->m_field_type->DefnName());

					// Show the id of the type object.
					swprintf(buffer, buffer_len, L"Type%lu", data_field->m_field_type->ItemId());
					return(buffer);
				}

		//
		// Definitions 3.
		//

		case cxx_function_type:
		case cxx_code_entry_point:
				{
					TCppFunctionTypeDefn *func_type = (TCppFunctionTypeDefn*)m_cpp_item;
					swprintf(buffer, buffer_len, L"Params: %d.", func_type->GetNumParams());
					return(buffer);
				}

		case cxx_possible_exception_spec:
				{
					TCppPossibleExceptionSpec *spec = (TCppPossibleExceptionSpec*)m_cpp_item;
					return((spec->m_exception_type != NULL) ? spec->m_exception_type->DefnName() : L"null_exception_type");
				}

		//
		// Definitions 4.
		//

		case cxx_asm_insert:
				{
					TCppAsmInsertDefn *asm_ins = (TCppAsmInsertDefn*)m_cpp_item;
					swprintf(buffer, buffer_len, L"Len: %d.", asm_ins->m_asm_insert.m_len);
					return(buffer);
				}

		case cxx_linkage_spec:
				{
					TCppLinkageSpec *spec = (TCppLinkageSpec*)m_cpp_item;
					swprintf(buffer, buffer_len, L"extern \"%s\"", (spec->m_spec_name != NULL) ? spec->m_spec_name : L"null_spec_name");
					return(buffer);
				}

		//
		//  Templates.
		//

		case cxx_class_template:
		case cxx_class_template_spec:
		case cxx_class_template_inst:
				{
					TCppStructTypeDefn *defn = (TCppStructTypeDefn*)m_cpp_item;
					return((defn->m_defined == FALSE) ? L"Forward-Decl" : NULL);
				}

		case cxx_template_type_argument:
				{
					TCppTemplateTypeArgument *arg = (TCppTemplateTypeArgument*)m_cpp_item;
					return(TCppDatabase::GetTemplateTypeArgumentName(arg->m_argument_value));
				}

		case cxx_template_template_argument:
				{
					TCppTemplateTemplateArgument *arg = (TCppTemplateTemplateArgument*)m_cpp_item;
					return(TCppDatabase::GetTemplateTemplateArgumentName(arg->GetReferencedDefnName()));
				}

		//
		//  Unresolved objects.
		//

		case cxx_unresolved_data_type:
		case cxx_unresolved_struct_type:
		case cxx_unresolved_data_field:
		case cxx_unresolved_code_entry_point:
		case cxx_unresolved_class_template:
		case cxx_unresolved_func_template:
				{
					assert(m_cpp_item->IsDefinition() == TRUE);
					TList *unres_names_list = ((TCppDefnBase*)m_cpp_item)->GetUnresolvedNamesList();
					if (unres_names_list == NULL)
						return(L"null_unres_names_list");

					swprintf(buffer, buffer_len, L"UnresNames: %d.", unres_names_list->NumItems());
					return(buffer);
				}

		//
		//  Statements.
		//

		case cxx_goto_stmt:
				{
					TCppGotoStatement *stmt = (TCppGotoStatement*)m_cpp_item;
					if (stmt->m_label == NULL)
						return(L"null_label");

					// Label object is present.
					const wchar_t *label_name = stmt->m_label->DefnName();
					swprintf(buffer, buffer_len, L"%s (%lu)", ((label_name != NULL) ? label_name : L"null_label_name"), stmt->m_label->ItemId());
					return(buffer);
				}

		//
		//  Expressions.
		//

		case cxx_operand_expr:
				{
					// Pick up description for the most common types of the operands.
					TCppOperandExpr *operand = (TCppOperandExpr*)m_cpp_item;
					const wchar_t *description = operand->GetTextRepresentation(buffer, buffer_len, TRUE);
					if (description != NULL)
						return(description);

					// Show generic props of the expression.
					return(ExprBaseItemDetails(buffer, buffer_len, operand));
				}

		case cxx_unary_expr:
		case cxx_binary_expr:
		case cxx_multiop_expr:
		case cxx_assignment_expr:
		case cxx_throw_expr:
		case cxx_conditional_expr:
		case cxx_field_access_expr:
		case cxx_field_access_ptr_expr:
		case cxx_array_index_expr:
		case cxx_function_call_expr:
		case cxx_cast_expr:
		case cxx_sizeof_expr:
		case cxx_new_expr:
		case cxx_delete_expr:
		case cxx_typeid_expr:
				{
					return(ExprBaseItemDetails(buffer, buffer_len, (TCppExpressionBase*)m_cpp_item));
				}
	}

	// Do not show anything by default.
	return(NULL);
}

const wchar_t *TLangViewCppItemListBoxItem::StructItemTypeName(wchar_t *buffer, int buffer_len, const wchar_t *label_format)
{
	assert(m_cpp_item->IsStructType() == TRUE);
	TCppStructTypeDefn *struct_defn = (TCppStructTypeDefn*)m_cpp_item;

	swprintf(buffer, buffer_len, label_format, TCppDefnBase::GetElabTypeName(struct_defn->DefnElabType(), TRUE));
	return(buffer);
}

const wchar_t *TLangViewCppItemListBoxItem::ExprBaseItemDetails(wchar_t *buffer, int buffer_len, const TCppExpressionBase *expr)
{
	if (expr->m_lvalue_expr == TRUE && expr->m_const_expr == TRUE)
	{
		return(L"L-VALUE, CONST.");
	}
	else if (expr->m_lvalue_expr == TRUE)
	{
		return(L"L-VALUE");
	}
	else if (expr->m_const_expr == TRUE)
	{
		return(L"CONST");
	}

	// Do not show anything.
	return(NULL);
}

// ------------------------------------------------------------------------
//   ==============  TLangViewCppItemsListBoxTable  =================
// ------------------------------------------------------------------------

void TLangViewCppItemsListBoxTable::Seek(int inx)
{
	// Normalize the passed position and store it.
	if (inx < 0)
		inx = 0;
	if (inx > m_data.NumItems())
		inx = m_data.NumItems();

	m_pos = inx;
}

TLangViewCppItemListBoxItem *TLangViewCppItemsListBoxTable::ReadItem(bool read_forward)
{
	// Pick up the pointer to the object in the data table.
	TCppItemBase *item;
	if (read_forward == TRUE)
	{
		if (m_pos >= m_data.NumItems())
			return(NULL);
		item = m_data[m_pos++];
	}
	else
	{
		if (m_pos <= 0)
			return(NULL);
		item = m_data[--m_pos];
	}

	// Pick up the ListBoxItem.
	TLangViewCppItemListBoxItem *lvi = m_lookaside_data.GetItem();
	if (lvi == NULL)
	{
		m_owner->MsgBox(L"Out of memory on new TLangViewCppItemListBoxItem.");
		return(NULL);
	}

	// Prepare the item and return it.
	lvi->Clear();
	lvi->SetupItem(item);
	lvi->FormatItem(*m_fmt_owner, m_visible_level_delta);
	return(lvi);
}

bool TLangViewCppItemsListBoxTable::LoadDefinitions(TCppItemBase &items_parent, int items_nest_level, bool natural_sort_order)
{
	// Reset the prev contents if any.
	ResetTableContents();

	// Recursively process the passed subtree. Root of the tree will not be displayed.
	int num_added = ProcessDefnsSubtree(0, items_parent, natural_sort_order);

	// Save nesting level of the passed defns space.
	m_visible_level_delta = items_nest_level;
	return(num_added >= 0);
}

int TLangViewCppItemsListBoxTable::FindCppItemIndexById(ID cpp_item_id) const
{
	if (cpp_item_id == 0)
		return(-1);

	for (int inx=0; inx < m_data.NumItems(); ++inx)
	{
		if (m_data[inx]->ItemId() == cpp_item_id)
			return(inx);
	}

	// Requested item is not available.
	return(-1);
}

int TLangViewCppItemsListBoxTable::ProcessDefnsSubtree(int ins_before, TCppItemBase &items_parent, bool natural_sort_order)
{
	bool res = TRUE;
	int inx_beg = ins_before;
	int num_added = 0;

	// Pass 1. Process items of the current layer.
	for (TCppDirectChildrenIter iter1(items_parent); iter1; ++iter1)
	{
		TCppItemBase *item = &iter1.CurrItem();
		if (m_data.InsertItem(ins_before++, item) == FALSE)
		{
			m_owner->MsgBox(L"Error adding the Cpp item.");
			res = FALSE;
			break;
		}

		num_added++;
	}

	if (res == TRUE)
	{
		// Pass 2. Process subtrees.
		ins_before = inx_beg;
		for (TCppDirectChildrenIter iter2(items_parent); iter2; ++iter2)
		{
			ins_before++;
			int num_child_items = ProcessDefnsSubtree(ins_before, iter2.CurrItem(), natural_sort_order);
			if (num_child_items < 0)
			{
				m_owner->MsgBox(L"Error adding the Cpp items subtree.");
				res = FALSE;
				break;
			}

			num_added += num_child_items;
			ins_before += num_child_items;
		}
	}

	return((res == TRUE) ? num_added : -1);
}

// --------------------------------------------------------------------------
//   ==============  TLangViewCppDefnsSpaceViewer  ==================
// --------------------------------------------------------------------------

static const TControlPosInfo g_CppDefnsSpaceViewerCtrls[] =
{
	IDC_CPP_GLB_NAMESP_LAB_BKGR,		cpm_fix_top_side,
	IDC_CPP_GLB_NAMESP_LABEL,			cpm_fix_left_top,
	IDC_CPP_GLB_NAMESP_NUM_ITEMS,		cpm_fix_right_top,
	IDC_CPP_GLB_NAMESP_TITLE,			cpm_fix_top_side,
	IDC_CPP_GLB_NAMESP_LISTBOX,		cpm_fix_all_corners,
	0, cpm_none,
};

static TListBoxColumnProps g_CppDefnsSpaceColumnsInfo[] =
{
	{ align_left,		8,	10,	   0,	    0,		},		// Name (initial column)
	{ align_left,		0,	10,	   0,	    0,		},		// Name (intermediate column, it will be duplicated several times in real setup)
	{ align_left,		0,	40,	  70,	    6,		},		// Name (final column)
	{ align_left,		0,	36,	  60,	    6,		},		// Type
	{ align_left,		0,	30,	  30,	    6,		},		// Details
	{ align_right,		0,	34,	  20,	   11,	},		// Id
	LBX_CLMNP_EOF
};

static TListBoxColumnTilteProps g_CppDefnsSpaceTitleInfo[] =
{
	{ 0,	 FALSE,	L"Name"	},		// Width of the column title will be set up later.
	{ 1,	 FALSE,	L"Type"	},
	{ 1,	 FALSE,	L"Dtls"	},
	{ 1,	 FALSE,	L"Id"	},
	{ 0,	 FALSE,	NULL	},		// EOF record.
};

TLangViewCppDefnsSpaceViewer::TLangViewCppDefnsSpaceViewer()
		: TLangViewChildBrowser(NULL, cbrt_cpp_items_list, IDD_LANGV_CPPV_GLOBAL_NAMESP)
{
	m_cpp_items_table.SetOwners(this, &m_list_hlp);
	m_list_hlp.SetupListBoxTable((TBasicListBoxTable*)&m_cpp_items_table, FALSE);
	m_latest_upper_item_inx = 0;
}

LRESULT TLangViewCppDefnsSpaceViewer::OnInitDialog(HWND hDefaultFocusControl)
{
	// Prepare control fonts and resizing.
	SetControlFont(IDC_CPP_GLB_NAMESP_LABEL, TAHOMA, 7);
	SetControlFont(IDC_CPP_GLB_NAMESP_NUM_ITEMS, TAHOMA, 7);
	ShiftControl(IDC_CPP_GLB_NAMESP_LAB_BKGR, -1, 0);
	ShiftControl(IDC_CPP_GLB_NAMESP_LABEL, 0, -1);
	ShiftControl(IDC_CPP_GLB_NAMESP_NUM_ITEMS, 0, -1);
	AdjustControlRightSide(IDC_CPP_GLB_NAMESP_TITLE);
	AdjustControlRightSide(IDC_CPP_GLB_NAMESP_LISTBOX);
	SetupControlsListPositioning(g_CppDefnsSpaceViewerCtrls);

	// Step1. Setup windows.
	m_list_hlp.SetWindow(::GetDlgItem(m_hDlg, IDC_CPP_GLB_NAMESP_LISTBOX));
	m_list_hlp.SetTitleWindow(::GetDlgItem(m_hDlg, IDC_CPP_GLB_NAMESP_TITLE));

	// Step2. Setup props without auto select and with stnd styles.
	TGenericListBoxObjectProps props_data;
	props_data.PrepareForSingleSelect(FALSE);
	m_list_hlp.SetupProps(props_data, FALSE);
	m_list_hlp.SetupSystemStdStyles();

	// Step3. Setup the column props and the column titles.
	int name_clmns = TLangViewCppItemListBoxItem::num_name_clmns;
	assert(name_clmns >= 3 && name_clmns <= 200);

	// Length of the array should be the const expression. This is why the array definition does not look nice.
	TListBoxColumnProps clmns_info[(TLangViewCppItemListBoxItem::num_name_clmns-3)+sizeof(g_CppDefnsSpaceColumnsInfo)/sizeof(TListBoxColumnProps)];
	int dest_clmn = 0;
	clmns_info[dest_clmn++] = g_CppDefnsSpaceColumnsInfo[0];
	for (int ic1=1; ic1<name_clmns-1; ++ic1)
		clmns_info[dest_clmn++] = g_CppDefnsSpaceColumnsInfo[1];
	for (int ic2=2; ic2<sizeof(g_CppDefnsSpaceColumnsInfo)/sizeof(TListBoxColumnProps); ++ic2)
		clmns_info[dest_clmn++] = g_CppDefnsSpaceColumnsInfo[ic2];

	assert(dest_clmn == sizeof(clmns_info)/sizeof(TListBoxColumnProps));
	m_list_hlp.SetupColumnProps(clmns_info);

	TListBoxColumnTilteProps clmn_titles[sizeof(g_CppDefnsSpaceTitleInfo)/sizeof(TListBoxColumnTilteProps)];
	for (int ic3=0; ic3<sizeof(g_CppDefnsSpaceTitleInfo)/sizeof(TListBoxColumnTilteProps); ++ic3)
		clmn_titles[ic3] = g_CppDefnsSpaceTitleInfo[ic3];
	clmn_titles[0].title_clmn_width = name_clmns;
	m_list_hlp.SetupColumnTitles(clmn_titles);

	// Step4. Finalize the setup.
	m_list_hlp.RebuildListBox();
	SetControlTextFmt(IDC_CPP_GLB_NAMESP_NUM_ITEMS, L"NumItems: %d", m_list_hlp.GetNumItems());

	// Widows should not set focus anywhere.
	return(FALSE);
}

void TLangViewCppDefnsSpaceViewer::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDC_CPP_GLB_NAMESP_LISTBOX:
				{
					TListBoxNotificationCode ntf_code = (TListBoxNotificationCode)notificationCode;
					TListBoxMouseEventInfo *mouse_info = (TListBoxMouseEventInfo*)hWndControl;
					if (ntf_code == lbnc_left_btn_down || ntf_code == lbnc_left_btn_dblclk)
					{
						if (mouse_info->inside_item_rect == FALSE)
						{
							// User has clicked below all items.
							if (m_owner != NULL)
								m_owner->RemoveSelection(this);
							break;
						}
						else if (mouse_info->clicked_item != NULL)
						{
							// Clicked item is available. This list box hosts only one type of items.
							TLangViewCppItemListBoxItem *casted_item = (TLangViewCppItemListBoxItem*)mouse_info->clicked_item;

							// Prepare the click event. Only one type of event is possible.
							TLangViewClickEvent event_info;
							event_info.type = clet_cpp_object;
							event_info.cpp_item = casted_item->GetCppItem();
							SendForProcessingFromListBox(ntf_code, event_info);
						}
					}
					else if (ntf_code == lbnc_right_btn_down)
					{
					}
				}
				break;
	}
}

// Inherited virtual method.
void TLangViewCppDefnsSpaceViewer::ResetContents()
{
	// Save the current upper item index and kill the data.
	m_latest_upper_item_inx = m_list_hlp.GetUpperItemIndex();
	m_cpp_items_table.ResetTableContents();
	m_list_hlp.RebuildListBox();
}

// Inherited virtual method.
void TLangViewCppDefnsSpaceViewer::SetSelection(TLangViewClickEvent &event, bool ext_margins)
{
	int index_to_select = -1;
	if (event.type == clet_cpp_object && event.cpp_item != NULL)
	{
		index_to_select = m_cpp_items_table.FindCppItemIndexById(event.cpp_item->ItemId());
	}

	if (index_to_select >= 0)
	{
		m_list_hlp.SelectItemByIndex(index_to_select);
		m_list_hlp.ScrollItemIntoTheView(index_to_select, ext_margins);
	}
	else
	{
		m_list_hlp.RemoveSelection();
	}
}

// Inherited virtual method.
void TLangViewCppDefnsSpaceViewer::ResetSelection()
{
	m_list_hlp.RemoveSelection();
}

void TLangViewCppDefnsSpaceViewer::LoadCppItems(TCppItemBase &items_parent, int child_items_nest_level)
{
	m_cpp_items_table.LoadDefinitions(items_parent, child_items_nest_level, TRUE);
	if (m_hDlg != NULL)
	{
		m_list_hlp.ScrollItemIntoTheView(m_latest_upper_item_inx, FALSE);
		m_list_hlp.RebuildListBox();
		SetControlTextFmt(IDC_CPP_GLB_NAMESP_NUM_ITEMS, L"NumItems: %d", m_list_hlp.GetNumItems());
	}
}

TCppItemBase *TLangViewCppDefnsSpaceViewer::GetSelectedCppItem()
{
	int slct_inx = m_list_hlp.GetSelectionIndex();
	if (slct_inx < 0)
		return(NULL);

	return(m_cpp_items_table.GetCppItem(slct_inx));
}


