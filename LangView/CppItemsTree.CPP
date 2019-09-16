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

#include  "Common/FormatNumber.H"
#include  "LangView/CppItemsTree.H"
#include  "LangView/LangViewResources.H"

// -------------------------------------------------------------------------
//   ===============  TLangViewCppItemsTreeDialog  ==================
// -------------------------------------------------------------------------

TLangViewCppItemsTreeDialog::TLangViewCppItemsTreeDialog()
		: TLangViewChildBrowser(NULL, cbrt_cpp_items_tree, IDD_LANGV_CHLD_CPP_ITEMS_TREE)
{
	m_placing_mode = plc_mode_concentric;
	m_citems_tree_root.SetLexItemClickOwner();

	m_selected_node = NULL;
	m_selection_object = NULL;
}

TLangViewCppItemsTreeDialog::~TLangViewCppItemsTreeDialog()
{
	m_citems_tree_root.ReleaseChildren();

	if (m_selection_object != NULL)
		delete m_selection_object;
}

LRESULT TLangViewCppItemsTreeDialog::OnInitDialog(HWND hDefaultFocusControl)
{
	// Prepare resizing of the dialog.
	AdjustControlRightSide(IDC_CPPITR_BODY);
	SetupControlPositioning(IDC_CPPITR_TITLE, cpm_fix_top_side);
	SetupControlPositioning(IDC_CPPITR_BODY, cpm_fix_all_corners);

	// Prepare neutral lex items style without passing any specific grammar.
	m_lex_items_style.SetupLexItemsProps(NULL, 0);
	m_lex_items_style.SetupForDeviceContextEx(m_hDlg);

	// Prepare the screen items viewer.
	m_citems_view_helper.SetWindow(::GetDlgItem(m_hDlg, IDC_CPPITR_BODY));
	m_citems_view_helper.SetStndScrollableProps(RGB(255, 255, 255), 40, 40);

	ShowCurrentContents();
	SetFocusControl(IDC_CPPITR_BODY);
	return(FALSE);
}

void TLangViewCppItemsTreeDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDC_CPPITR_BODY:
				{
					TScreenItemsViewMouseNotify *mouse_ntf = (TScreenItemsViewMouseNotify*)hWndControl;
					switch (notificationCode)
					{
						case xiv_left_btn_down:
						case xiv_left_btn_dblclk:
								{
									// Save the color of the pixel under the cursor tip.
									m_citems_view_helper.SaveClickedPointColor(mouse_ntf);

									// Button was pressed in the screen items area.
									if (mouse_ntf->object != NULL)
									{
										// Some object was clicked.
										TLangViewClickEvent click_event_info;
										TLangViewLexItem *lex_item = (TLangViewLexItem*)mouse_ntf->object->GetOwner();
										if (lex_item->PrepareClickEvent(click_event_info) == TRUE)
										{
											// Clicked item prepared the meaningful event.
											SendForProcessingFromScreenItemsView((TScreenItemsViewNtfCode)notificationCode, click_event_info);
										}
									}
									else
									{
										// User has clicked the background or some transparent object.
										if (m_owner != NULL)
											m_owner->RemoveSelection(this);
									}
								}
								break;

						case xiv_right_btn_down:
								{
									// Right button was pressed in the screen items area.
									if (mouse_ntf->object == NULL)
									{
										// The background was clicked.
										ProcessControlMenu(mouse_ntf->click_place.x, mouse_ntf->click_place.y);
									}
								}
								break;

						case xiv_corner_clicked:
								{
									// This is lower right corner between the scrollbars.
									TScreenItemsViewNotify *nti = (TScreenItemsViewNotify*)hWndControl;
									if (nti->right_btn == TRUE)
										ProcessControlMenu(nti->pos_x, nti->pos_y);
								}
								break;
					}
				}
				break;
	}
}

void TLangViewCppItemsTreeDialog::LoadCppItems(TCppItemBase &items_subtree_root)
{
	ResetContents();

	m_citems_tree_root.SetCppItemPtr(&items_subtree_root);
	LoadCppItemFrontReferences(&m_citems_tree_root);
	LoadCppItemChildren(&m_citems_tree_root, &items_subtree_root);
	LoadCppItemBackReferences(&m_citems_tree_root);

	if (m_hDlg != NULL)
		ShowCurrentContents();
}

void TLangViewCppItemsTreeDialog::AcceptCppItemsTree(TLangViewCppItemTreeNode &visible_lex_items_root)
{
	ResetContents();

	m_citems_tree_root.AcceptChildren(visible_lex_items_root);
	m_citems_tree_root.SetCppItemPtr(visible_lex_items_root.GetCppItem());
	visible_lex_items_root.SetCppItemPtr(NULL);

	if (m_hDlg != NULL)
		ShowCurrentContents();
}

void TLangViewCppItemsTreeDialog::ResetContents()
{
	m_citems_tree_root.ReleaseChildren();
	m_citems_tree_root.SetCppItemPtr(NULL);
	ResetViewInternal();
}

void TLangViewCppItemsTreeDialog::SetSelection(TLangViewClickEvent &event, bool show_margins)
{
	ResetSelection();
	if (event.type != clet_cpp_object || event.cpp_item == NULL)
		return;

	// Passed event describes non empty cpp item. Look for this item in the current tree.
	TLangViewCppItemTreeNode *new_slct_node = NULL;
	if (m_citems_tree_root.GetCppItem() != NULL && m_citems_tree_root.GetCppItem()->ItemId() == event.cpp_item->ItemId())
	{
		new_slct_node = &m_citems_tree_root;
	}
	else
	{
		TGenericTreeNodesIterator iter(&m_citems_tree_root);
		for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
		{
			TLangViewTreeNode *basic_node = (TLangViewTreeNode*)iter.GetCurrentItem();
			if (basic_node->GetItemType() == litp_cpp_database_item)
			{
				TLangViewCppItemTreeNode *node = (TLangViewCppItemTreeNode*)basic_node;
				if (node->GetCppItem()->ItemId() == event.cpp_item->ItemId())
				{
					// Requested item is present in the tree.
					new_slct_node = node;
					break;
				}
			}
		}

		if (iter.CheckIterationSuccess() == FALSE)
			MsgBox(L"Some of the nodes were not iterated.");
	}

	if (new_slct_node != NULL)
	{
		// Show selection.
		CreateAndShowSelectionObject(new_slct_node);

		if (m_selection_object != NULL)
		{
			if (show_margins == TRUE || m_citems_view_helper.CheckFullOrPartialScrollOut(*(new_slct_node->GetPrimaryBkgrItem())) == TRUE)
			{
				// Scroll selection into the view.
				long margins = 14;
				m_citems_view_helper.ScrollScreenItemIntoTheView(*m_selection_object, margins, margins);
			}
		}
	}
}

void TLangViewCppItemsTreeDialog::ResetSelection()
{
	if (m_selection_object != NULL)
	{
		// Initiate repainting the place where current selection object is staying.
		m_citems_view_helper.InvalidateScreenItemRect(m_selection_object);
		m_citems_view_helper.RemoveFromInternalDataSet(*m_selection_object);
		delete m_selection_object;
	}

	m_selected_node = NULL;
	m_selection_object = NULL;
}

bool TLangViewCppItemsTreeDialog::LoadCppItemChildren(TGenericTreeNode *owner_tree_node, TCppItemBase *owner_database_item)
{
	assert(owner_database_item != NULL);
	for (TCppDirectChildrenIter iter(owner_database_item); iter; ++iter)
	{
		TCppItemBase *item = &iter.CurrItem();
		TLangViewCppItemTreeNode *node = new TLangViewCppItemTreeNode(owner_tree_node, 0, item);
		if (node == NULL)
		{
			MsgBox(L"Error loading cpp items.");
			return(FALSE);
		}

		// Make this new node clickable.
		node->SetLexItemClickOwner();

		if (LoadCppItemFrontReferences(node) == FALSE)
			return(FALSE);

		// Process children of this item if any.
		if (LoadCppItemChildren(node, item) == FALSE)
			return(FALSE);

		if (LoadCppItemBackReferences(node) == FALSE)
			return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TLangViewCppItemsTreeDialog::LoadCppItemFrontReferences(TLangViewCppItemTreeNode *item_node)
{
	switch (item_node->GetCppItem()->ItemType())
	{
		case cxx_bit_field_type:
				{
					TCppBitFieldTypeDefn *bf_type = (TCppBitFieldTypeDefn*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, bf_type->m_base_type, L"Missing: BaseType") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_cv_ptr_ref_type:
				{
					TCppCvPtrRefTypeDefn *cvpr_type = (TCppCvPtrRefTypeDefn*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, cvpr_type->m_base_type, L"Missing: BaseType") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_array_type:
				{
					TCppArrayTypeDefn *array_type = (TCppArrayTypeDefn*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, array_type->m_base_type, L"Missing: BaseType") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_data_type_alias:
				{
					TCppDataTypeAliasDefn *alias_type = (TCppDataTypeAliasDefn*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, alias_type->m_base_type, L"Missing: BaseType") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_enum_type:
				{
					TCppEnumTypeDefn *enum_type = (TCppEnumTypeDefn*)item_node->GetCppItem();
					if (CreateCppItemOptionalLink(item_node, enum_type->m_base_type) == FALSE)
						return(FALSE);
				}
				break;

		case cxx_base_class_spec:
				{
					TCppBaseClassSpec *base_spec = (TCppBaseClassSpec*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, base_spec->m_base_class, L"Missing: BaseClass") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_friend_spec:
				{
					TCppFriendSpec *friend_spec = (TCppFriendSpec*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, friend_spec->m_friendly_object, L"Missing: FriendlyObject") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_data_field:
				{
					TCppDataFieldDefn *data_field = (TCppDataFieldDefn*)item_node->GetCppItem();
					if (CreateCppItemOptionalLink(item_node, data_field->m_linkage_spec) == FALSE)
						return(FALSE);
					if (CreateCppItemMandatoryLink(item_node, data_field->m_field_type, L"Missing: DataFieldType") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_function_type:
		case cxx_code_entry_point:
		case cxx_func_template:
		case cxx_func_template_spec:
		case cxx_func_template_inst:
				{
					TCppFunctionTypeDefn *func_type = (TCppFunctionTypeDefn*)item_node->GetCppItem();
					if (CreateCppItemOptionalLink(item_node, func_type->m_linkage_spec) == FALSE)
						return(FALSE);

					if (func_type->m_spec_type == spcft_none || func_type->m_spec_type == spcft_operator)
					{
						// These special function types have return value. Others not.
						if (CreateCppItemMandatoryLink(item_node, func_type->m_return_value_type, L"Missing: ReturnValueType") == FALSE)
							return(FALSE);
					}

					if (item_node->GetCppItem()->ItemType() == cxx_func_template_spec)
					{
						TCppFunctionTemplateSpecialization *func_templ_spec = (TCppFunctionTemplateSpecialization*)func_type;
						if (CreateCppItemMandatoryLink(item_node, func_templ_spec->m_base_template, L"Missing: BaseFunctionTemplate") == FALSE)
							return(FALSE);
					}
					else if (item_node->GetCppItem()->ItemType() == cxx_func_template_inst)
					{
						TCppFunctionTemplateInstantiation *func_templ_inst = (TCppFunctionTemplateInstantiation*)func_type;
						if (CreateCppItemMandatoryLink(item_node, func_templ_inst->m_original_template, L"Missing: OriginalFunctionTemplate") == FALSE)
							return(FALSE);
					}
				}
				break;

		case cxx_function_type_alias:
				{
					TCppFunctionTypeAliasDefn *alias_type = (TCppFunctionTypeAliasDefn*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, alias_type->m_base_type, L"Missing: BaseType") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_possible_exception_spec:
				{
					TCppPossibleExceptionSpec *exception_spec = (TCppPossibleExceptionSpec*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, exception_spec->m_exception_type, L"Missing: PossibleExceptionType") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_namespace_alias:
				{
					TCppNamespaceAliasDefn*namesp_alias = (TCppNamespaceAliasDefn*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, namesp_alias->m_base_namespace, L"Missing: BaseNamespace") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_using_declaration:
				{
					TCppUsingDeclarationDefn *using_decl = (TCppUsingDeclarationDefn*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, using_decl->m_defn_to_use, L"Missing: DefnToUse") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_using_directive:
				{
					TCppUsingDirectiveDefn *using_dir = (TCppUsingDirectiveDefn*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, using_dir->m_namespace_to_use, L"Missing: NamespaceToUse") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_class_template_spec:
				{
					TCppClassTemplateSpecialization *class_templ_spec = (TCppClassTemplateSpecialization*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, class_templ_spec->m_base_template, L"Missing: BaseClassTemplate") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_class_template_inst:
				{
					TCppClassTemplateInstantiation *class_templ_inst = (TCppClassTemplateInstantiation*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, class_templ_inst->m_original_template, L"Missing: OriginalClassTemplate") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_template_type_argument:
				{
					TCppTemplateTypeArgument *type_arg = (TCppTemplateTypeArgument*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, type_arg->m_argument_value, L"Missing: TypeArgumentValue") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_template_template_argument:
				{
					TCppTemplateTemplateArgument *templ_arg = (TCppTemplateTemplateArgument*)item_node->GetCppItem();
					switch (templ_arg->m_arg_type)
					{
						case TCppTemplateTemplateArgument::ctarg_template_defn:
								{
									if (CreateCppItemMandatoryLink(item_node, templ_arg->m_argument_value, L"Missing: TemplateTemplateArgumentValue") == FALSE)
										return(FALSE);
								}
								break;

						case TCppTemplateTemplateArgument::ctarg_template_template_param:
								{
									if (CreateCppItemMandatoryLink(item_node, templ_arg->m_template_param, L"Missing: TemplateTemplateArgumentValue") == FALSE)
										return(FALSE);
								}
								break;
					}
				}
				break;

		case cxx_unresolved_name_info:
				{
					TCppUnresolvedNameInfo *unres_name_info = (TCppUnresolvedNameInfo*)item_node->GetCppItem();
					if (unres_name_info->m_node_type == cnit_ovld_conversion || unres_name_info->m_node_type == cnit_ovld_conv_template)
					{
						if (CreateCppItemMandatoryLink(item_node, unres_name_info->m_conversion_dest_type, L"Missing: OvldConversionDestType") == FALSE)
							return(FALSE);
					}
				}
				break;

		case cxx_goto_stmt:
				{
					TCppGotoStatement *goto_stmt = (TCppGotoStatement*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, goto_stmt->m_label, L"Missing: GotoStmtLabel") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_catch_block:
				{
					TCppCatchBlockDefn *catch_block = (TCppCatchBlockDefn*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, catch_block->m_exception, L"Missing: CatchBlockParameter") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_cast_expr:
				{
					TCppCastExpr *cast_expr = (TCppCastExpr*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, cast_expr->m_dest_type, L"Missing: CastDestinationType") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_sizeof_expr:
				{
					TCppSizeofExpr *sizeof_expr = (TCppSizeofExpr*)item_node->GetCppItem();
					if (sizeof_expr->m_expr_operand == FALSE)
					{
						if (CreateCppItemMandatoryLink(item_node, sizeof_expr->m_data_type, L"Missing: SizeofDataType") == FALSE)
							return(FALSE);
					}
				}
				break;

		case cxx_new_expr:
				{
					// Show destination type even if the ctor call is present. The dest type should be always present.
					TCppNewExpr *new_expr = (TCppNewExpr*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, new_expr->m_new_type, L"Missing: NewType") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_typeid_expr:
				{
					TCppTypeidExpr *typeid_expr = (TCppTypeidExpr*)item_node->GetCppItem();
					if (typeid_expr->m_expr_operand == FALSE)
					{
						if (CreateCppItemMandatoryLink(item_node, typeid_expr->m_data_type, L"Missing: TypeidDataType") == FALSE)
							return(FALSE);
					}
				}
				break;
	}

	// Extra nodes were created or they were not needed.
	return(TRUE);
}

bool TLangViewCppItemsTreeDialog::LoadCppItemBackReferences(TLangViewCppItemTreeNode *item_node)
{
	switch (item_node->GetCppItem()->ItemType())
	{
		case cxx_template_type_param:
				{
					TCppTemplateTypeParamDefn *type_param = (TCppTemplateTypeParamDefn*)item_node->GetCppItem();
					if (CreateCppItemOptionalLink(item_node, type_param->m_default_value) == FALSE)
						return(FALSE);
				}
				break;

		case cxx_template_template_param:
				{
					TCppTemplateTemplateParamDefn *templ_param = (TCppTemplateTemplateParamDefn*)item_node->GetCppItem();
					if (CreateCppItemOptionalLink(item_node, templ_param->m_default_value) == FALSE)
						return(FALSE);
				}
				break;

		case cxx_field_access_expr:
				{
					TCppFieldAccessExpr *fa_expr = (TCppFieldAccessExpr*)item_node->GetCppItem();
					if (CreateCppItemMandatoryLink(item_node, fa_expr->m_struct_member_defn, L"Missing: FieldName") == FALSE)
						return(FALSE);
				}
				break;

		case cxx_typeid_expr:
				{
					TCppTypeidExpr *typeid_expr = (TCppTypeidExpr*)item_node->GetCppItem();
					if (typeid_expr->m_dynamic_result == FALSE)
					{
						if (CreateCppItemOptionalLink(item_node, typeid_expr->m_static_type_struct) == FALSE)
							return(FALSE);
					}
				}
				break;

	}

	// Extra nodes were created or they were not needed.
	return(TRUE);
}

bool TLangViewCppItemsTreeDialog::CreateCppItemOptionalLink(TLangViewCppItemTreeNode *item_node, TCppItemBase *referenced_item)
{
	if (referenced_item == NULL)
	{
		// The referenced item is missing. There is no need to do anything.
		return(TRUE);
	}

	// Do the job.
	return(CreateCppItemDimObject(item_node, referenced_item));
}

bool TLangViewCppItemsTreeDialog::CreateCppItemMandatoryLink(TLangViewCppItemTreeNode *item_node, TCppItemBase *referenced_item, const wchar_t *missing_link_name)
{
	if (referenced_item != NULL)
	{
		// The referenced item is present.
		return(CreateCppItemDimObject(item_node, referenced_item));
	}

	// Create red substitution for the missing link.
	assert(item_node != NULL);
	TLangViewSpecialTreeNode *red_node = new TLangViewSpecialTreeNode(item_node);
	if (red_node == NULL)
	{
		MsgBox(L"Error creating referenced cpp item substitute.");
		return(FALSE);
	}

	// Setup the props.
	red_node->SetLexItemClickOwner();
	red_node->Item().SetMissingLinkProps(missing_link_name);

	// Success.
	return(TRUE);
}

bool TLangViewCppItemsTreeDialog::CreateCppItemDimObject(TLangViewCppItemTreeNode *item_node, TCppItemBase *dim_object_proto)
{
	assert(item_node != NULL && dim_object_proto != NULL);
	TLangViewCppItemTreeNode *dim_node = new TLangViewCppItemTreeNode(item_node, 0, dim_object_proto);
	if (dim_node == NULL)
	{
		MsgBox(L"Error creating referenced cpp item.");
		return(FALSE);
	}

	// Make this new node clickable and dim.
	dim_node->SetLexItemClickOwner();
	dim_node->SetDimStyleMode(TRUE);

	// Success.
	return(TRUE);
}

void TLangViewCppItemsTreeDialog::ProcessControlMenu(long click_pos_x, long click_pos_y)
{
	TLangViewChildBrowser::TSIVMR sivmr = ProcessScreenItemsViewControlMenu(m_citems_view_helper, m_placing_mode, click_pos_x, click_pos_y);
	if (sivmr == TLangViewChildBrowser::sivmr_reload_view)
	{
		// Repaint the view.
		ShowCurrentContents();
	}
	else if (sivmr == TLangViewChildBrowser::sivmr_copy_to_clipborad)
	{
		// Create the screenshot.
		RECT margins = { 40, 30, 40, 40 };
		PutScreenShotToClipboard(m_citems_view_helper, &margins);
	}
}

void TLangViewCppItemsTreeDialog::ShowCurrentContents()
{
	// Remove current data set from the view if any.
	TLangViewTreeNode *slct_node = m_selected_node;
	ResetViewInternal();

	// Place and display the tree.
	void *contexts = &m_lex_items_style;
	switch (m_placing_mode)
	{
		case plc_mode_concentric:
				{
					TConcentricModePlacingProps concentric_props = TLangViewLexItemStyle::g_DefaultConcentricPlacingProps;
					int radius_steps[2] =  { 80, 0 };
					concentric_props.min_radius_steps = radius_steps;
					concentric_props.placing_direction = 0;

					TConcentricModeTreeGraphPlacer::DisplayGraph(m_citems_view_helper, m_citems_view_helper.InternalDataSet(), &m_citems_tree_root,
													concentric_props, &contexts);
				}
				break;

		case plc_mode_rect_lines:
				{
					TRectLinesModeTreeGraphPlacer::DisplayGraph(m_citems_view_helper, m_citems_view_helper.InternalDataSet(), &m_citems_tree_root,
													TLangViewLexItemStyle::g_DefaultRectLinesPlacingProps, &contexts);
				}
				break;

		default:
			assert(FALSE);
			break;
	}

	if (slct_node != NULL)
		CreateAndShowSelectionObject(slct_node);

	m_citems_view_helper.ScrollDataSetIntoCenter();
}

void TLangViewCppItemsTreeDialog::ResetViewInternal()
{
	if (m_selection_object != NULL)
		delete m_selection_object;

	m_selected_node = NULL;
	m_selection_object = NULL;

	m_citems_view_helper.InternalDataSet().Clear();
	m_citems_view_helper.ShowInternalDataSet(TRUE);
}

void TLangViewCppItemsTreeDialog::CreateAndShowSelectionObject(TLangViewTreeNode *node)
{
	assert(m_selected_node == NULL && m_selection_object == NULL);

	TScreenItem *sel_obj = node->CreateSelectionScreenItem(list_bold_blue);
	if (sel_obj == NULL)
	{
		MsgBox(L"Error creating selection object.");
		return;
	}

	// Add item to the painting list and invalidate its rect.
	m_citems_view_helper.InternalDataSet().PrependItem(sel_obj);
	m_citems_view_helper.InvalidateScreenItemRect(sel_obj);

	// Success. Save the data.
	m_selected_node = node;
	m_selection_object = sel_obj;
}


