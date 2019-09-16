//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Conditional parsing structure analysis.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "Common/FormatNumber.H"
#include  "TextScan/ParsingDriver.H"
#include  "LangView/HtmlGenSource.H"
#include  "Processing/CppAnalysisLib.H"

// -----------------------------------------------------------------------------
//	=================  TCppDetectInheritanceHelper  ====================
// -----------------------------------------------------------------------------

bool TCppDetectInheritanceHelper::BuildInheritanceTree(TLangViewCppItemTreeNode &visible_objects_root, TCppItemBase &cpp_items_subtree)
{
	// Delete children of the passed lex items root if any.
	visible_objects_root.ReleaseChildren();
	visible_objects_root.SetCppItemPtr(NULL);

	// Passed root should be accessible from other methods of the class.
	m_visible_root = &visible_objects_root;

	wprintf(L"Building inheritance tree ....\r\n");
	wprintf(L"-------------------------------------------\r\n");

	int cnt_passes = 0;
	for(;;)
	{
		m_cnt_unproc_bases = 0;
		m_cnt_new_derivatives = 0;

		// Process the root cpp item.
		if (cpp_items_subtree.IsStructType() == TRUE)
		{
			if (ProcessStructuredType((TCppStructTypeDefn*)&cpp_items_subtree) == FALSE)
			{
				m_visible_root = NULL;
				return(FALSE);
			}
		}

		// Iterate children of the root cpp item.
		for (TCppChildrenSubtreeIter iter(cpp_items_subtree); iter; ++iter)
		{
			TCppItemBase &item = iter.CurrItem();
			if (item.IsStructType() == TRUE)
			{
				if (ProcessStructuredType((TCppStructTypeDefn*)&item) == FALSE)
				{
					m_visible_root = NULL;
					return(FALSE);
				}
			}
			else if (item.IsNamespace() == FALSE)
			{
				// Current item is not a structure and it is not a namespace. Do not iterate its children.
				iter.BlockCurrItemChildrenIteration();
			}
		}

		// Print the results of the pass.
		wprintf(L"%3d. ClassObjects: %6d, UnprocessedBaseClasses: %5d, NewDerivedClasses: %5d.\r\n",
				++cnt_passes, m_visible_root->NumSubtreeNodes(FALSE)-1, m_cnt_unproc_bases, m_cnt_new_derivatives);

		if (m_cnt_new_derivatives == 0)
			break;
	}

	// Success.
	wprintf(L"\r\n");
	m_visible_root = NULL;
	return(TRUE);
}

bool TCppDetectInheritanceHelper::ProcessStructuredType(TCppStructTypeDefn *struct_type)
{
	// Look for the base classes of the passed struct and process them.
	bool base_classes_present = FALSE;
	for (TCppDirectChildrenIter iter(struct_type); iter; ++iter)
	{
		if (iter.CurrItem().ItemType() == cxx_base_class_spec)
		{
			// Passed struct type has a base class.
			base_classes_present = TRUE;
			TCppBaseClassSpec &spec = (TCppBaseClassSpec&)iter.CurrItem();
			ID base_class_id = spec.m_base_class->ItemId();

			// Look for this base class in the tree of the visible objects.
			bool base_class_node_found = FALSE;
			TGenericTreeNodesIterator iter(m_visible_root);
			for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
			{
				TLangViewCppItemTreeNode *node = (TLangViewCppItemTreeNode*)iter.GetCurrentItem();
				if (node->GetCppItem()->ItemId() == base_class_id)
				{
					// Create a child item of this node and do not abort the iteration.
					base_class_node_found = TRUE;
					if (AddClassObject(node, struct_type) == FALSE)
						return(FALSE);
				}
			}

			if (base_class_node_found == FALSE)
				m_cnt_unproc_bases++;
		}
	}

	if (base_classes_present == FALSE)
	{
		// This class does not have base classes. It should be added as a child to the root.
		if (AddClassObject(m_visible_root, struct_type) == FALSE)
			return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TCppDetectInheritanceHelper::AddClassObject(TLangViewCppItemTreeNode *parent_node, TCppStructTypeDefn *struct_type)
{
	// The node for the passed structure may be already existing. Check for this.
	for (TGenericTreeNodeDirectChildrenIter<TLangViewCppItemTreeNode> iter(parent_node); iter; ++iter)
	{
		if (iter.CurrItem().GetCppItem()->ItemId() == struct_type->ItemId())
			return(TRUE);
	}

	// This is a new type of node.
	TLangViewCppItemTreeNode *new_node = new TLangViewCppItemTreeNode(parent_node, 0, struct_type);
	if (new_node == NULL)
	{
		wprintf(L"Creating tree node for the class \"%s\" has failed. The processing is aborted.\r\n\r\n", struct_type->DefnName());
		return(FALSE);
	}

	// Success.
	m_cnt_new_derivatives++;
	return(TRUE);
}

// -------------------------------------------------------------------------------
//	==================  TOverwrittienMethodsReport  =====================
// -------------------------------------------------------------------------------

bool TOverwrittienMethodsReport::GenerateReport(TLangViewCppItemTreeNode *root_node, TCppDatabase &cpp_database, const wchar_t *short_report_name)
{
	bool overall_success = TRUE;

	wprintf(L"Building overriden methods matrix ....\r\n");
	wprintf(L"-----------------------------------------------\r\n");

	// Iterate the passed tree of derived classes. The purpose of this iteration is creation of the list of the override
	// roots, i.e. a list of methods that do not override any other method but at the same time are overriden in one
	// of the derived classes. Note that the root class itself is not processed in the loop.
	TGenericTreeNodesIterator iter(root_node);
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TLangViewCppItemTreeNode *node = (TLangViewCppItemTreeNode*)iter.GetCurrentItem();
		assert(node->GetCppItem()->IsStructType() == TRUE);

		// Iterate members of the current class.
		for (TCppDirectChildrenIter iter(node->GetCppItem()); iter; ++iter)
		{
			TCppCodeEntryPointDefn *instance_method = iter.CurrItemAsCodeEntryPoint(FALSE);
			if (instance_method == NULL)
				continue;

			// Current member is an instance method. Iterate its tree of the override parents if any.
			if (ProcessMethodOverrideParents(instance_method->m_override_protos) == FALSE)
			{
				overall_success = FALSE;
				break;
			}
		}
	}

	// Sort the list of dicovered root methods.
	wprintf(L"  Overriden methods discovered: %d.\r\n\r\n", m_method_override_roots.NumItems());
	m_method_override_roots.QuickSort(CompareCodeEntryPointPtrs);

	// Generate the report.
	m_classes_hier_root = root_node;
	WriteHtmlReport(L"Outfiles", short_report_name, TRUE);

	// The work is complete.
	m_classes_hier_root = NULL;
	return(overall_success);
}

void TOverwrittienMethodsReport::WriteHtmlReportBody(THtmlDocument &rprt)
{
	TSimpleHtmlElement main_table(L"table", NULL, 0, L"arp_ovrd_methods_table");
	rprt.EmitElementBeg(main_table);

	assert(m_classes_hier_root != NULL);
	if (m_method_override_roots.NumItems() == 0)
	{
		WriteEmptyTableMessage(rprt, L"10", L"The overriden methods are missing.", TRUE);
		rprt.EmitElementEnd(main_table);
		return;
	}

	// Generate the header of the table with the names of the methods.
	GenerateTableHeaderRows(rprt);

	// Generate the delimiter rows.
	wchar_t colspan_buff[40];
	TArrayBasedHtmlContainer row1(L"tr", L"arp_ovrd_methods_delim_row");
	TSimpleHtmlElement row2(L"tr", NULL, 0, L"arp_ovrd_methods_tiny_row");
	TAdvancedHtmlElement row1_cell1(L"td"), row1_cell2(L"td", NULL, 0, L"crp_indent_cell");
	row1_cell1.SetFirstAppAttr(L"colspan", _itow(NUM_CLASS_HIER_COLUMNS+2+m_method_override_roots.NumItems()+1, colspan_buff, 10));
	row1.AppendElements(&row1_cell1, &row1_cell2);

	rprt.WriteLine();
	rprt.EmitElements(row1, row2);
	rprt.WriteLine();

	// Generate the body of the table.
	GenerateReportRowForClass(rprt, m_classes_hier_root->GetCppItem(), 0);
	TGenericTreeNodesIterator iter(m_classes_hier_root);
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TLangViewCppItemTreeNode *node = (TLangViewCppItemTreeNode*)iter.GetCurrentItem();
		GenerateReportRowForClass(rprt, node->GetCppItem(), iter.GetCurrentLevel());
	}

	rprt.EmitElementEnd(main_table);
}

bool TOverwrittienMethodsReport::ProcessMethodOverrideParents(TCppCodeEntryPointDefnPtrsArray &data)
{
	// Iterate the passed array of the override parents if any.
	for (int ibase=0; ibase<data.NumItems(); ++ibase)
	{
		if (data[ibase]->m_override_protos.NumItems() == 0)
		{
			// Current override parent is an override root.
			if (ProcessMethodOverrideRoot(data[ibase]) == FALSE)
				return(FALSE);
		}
		else
		{
			// Current override parent overrides some other method.
			if (ProcessMethodOverrideParents(data[ibase]->m_override_protos) == FALSE)
				return(FALSE);
		}
	}

	// All allocations if any succeeded.
	return(TRUE);
}

bool TOverwrittienMethodsReport::ProcessMethodOverrideRoot(TCppCodeEntryPointDefn *entry_point)
{
	// Search the list of discovered roots for this method. Maybe this method is already known.
	for (int iroot=0; iroot<m_method_override_roots.NumItems(); ++iroot)
	{
		if (TCppFunctionTypeDefn::CompareFunctionTypes(m_method_override_roots[iroot], entry_point, TRUE) == TRUE)
		{
			// The method is already known.
			return(TRUE);
		}
	}

	// The method is new. Add it to the list of roots.
	if (m_method_override_roots.AppendItem(entry_point) == FALSE)
	{
		m_helper_console.HandleTrace(L"Error extending the list of the override roots.", TGenericConsole::ErrorHighlightMode);
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

void TOverwrittienMethodsReport::GenerateTableHeaderRows(THtmlDocument &rprt)
{
	wchar_t colspan_buff[40];
	int num_roots = m_method_override_roots.NumItems();

	for (int iroot=0; iroot <= num_roots; ++iroot)
	{
		// Each name of the method occupies its own row.
		TSimpleHtmlElement row(L"tr", NULL, 0, NULL, crlm_outline_object_frames);
		rprt.EmitElementBeg(row);

		// Make an offset above the names of the classes.
		TAdvancedHtmlElement class_names_balance_cell(L"td");
		class_names_balance_cell.SetFirstAppAttr(L"colspan", _itow(NUM_CLASS_HIER_COLUMNS+2, colspan_buff, 10));

		if (iroot == num_roots-1)
		{
			// This row has a very special offset cell.
			class_names_balance_cell.SetSecondAppAttr(L"rowspan", L"2");
			class_names_balance_cell.SetCssClass(L"arp_ovrd_methods_class_names_title_cell");
			class_names_balance_cell.SetElementBody(L"Class names");
		}

		// The last row of the header does not have an offset cell because the cell above occupies two rows.
		if (iroot < num_roots)
			rprt.EmitElement(class_names_balance_cell);

		// Issue padding cells with the icons inside.
		for (int ipad=0; ipad<iroot && ipad<num_roots; ++ipad)
		{
			if (ipad % 2 == 0)
				rprt.AddSoftNewLine();

			// Use non empty CSS class only for the last row of the versical sticks to reduce the size of the report file.
			TArrayBasedHtmlContainer padding_cell(L"td", (iroot == num_roots) ? L"arp_ovrd_methods_core_cell" : NULL, crlm_none);
			TImageHtmlElement icon1(L"/Images/Reports/VertStick.gif", NULL, NULL, crlm_none);
			padding_cell.AppendElement(&icon1);
			rprt.EmitElement(padding_cell);

		}

		if (iroot < num_roots)
		{
			// Issue a cell with the name of the method.
			TAdvancedHtmlElement method_name_cell(L"td", m_method_override_roots[iroot]->BasicItemName());
			method_name_cell.SetFirstAppAttr(L"colspan", _itow(num_roots+1-iroot, colspan_buff, 10));
			rprt.EmitElement(method_name_cell);
		}
		else
		{
			// This is a final row. Issue an empty cell.
			TSimpleHtmlElement empty_cell(L"td", NULL, 0, NULL, crlm_none);
			rprt.EmitElement(empty_cell);
		}

		rprt.EmitElementEnd(row);
	}
}

void TOverwrittienMethodsReport::GenerateReportRowForClass(THtmlDocument &rprt, TCppItemBase *cpp_item, int zero_based_layer)
{
	// Open the big row.
	TSimpleHtmlElement row(L"tr", NULL, 0, NULL, crlm_outline_object_frames);
	rprt.EmitElementBeg(row);

	// Generate indentation cells if needed and open the name cell.
	GeneratePaddingCells(rprt, zero_based_layer, NUM_CLASS_HIER_COLUMNS, L"crp_indent_cell");
	OpenAlignedCell(rprt, zero_based_layer, NUM_CLASS_HIER_COLUMNS, L"crp_body_cell");

	// Emit the name of the class and close the aligned cell.
	TSimpleHtmlElement class_name(NULL, cpp_item->BasicItemName(), -1, NULL, crlm_none);
	rprt.EmitElement(class_name);
	CloseAlignedCell(rprt);

	// Split a bit the name sof the class from the body of the table.
	GeneratePaddingCells(rprt, 1, 1, L"crp_indent_cell");

	// Iterate the list of overriden methods.
	for (int iroot=0; iroot<m_method_override_roots.NumItems(); ++iroot)
	{
		// Look for definition of this method in the current class.
		TCppCodeEntryPointDefn *local_method = NULL;
		for (TCppDirectChildrenIter iter(cpp_item); iter; ++iter)
		{
			TCppCodeEntryPointDefn *instance_method = iter.CurrItemAsCodeEntryPoint(FALSE);
			if (instance_method == NULL)
				continue;

			if (TCppFunctionTypeDefn::CompareFunctionTypes(m_method_override_roots[iroot], instance_method, TRUE) == TRUE)
			{
				local_method = instance_method;
				break;
			}
		}

		// Check the results of the search.
		const wchar_t *css_style = NULL;
		const wchar_t *cell_body = NULL;
		if (local_method == NULL)
		{
			// Show a dim minus symbol.
			css_style = L"arp_ovrd_methods_dim_cell";
			cell_body = L"-";
		}
		else if (local_method->m_override_protos.NumItems() == 0)
		{
			// This method was introduced in the current class.
			css_style = L"arp_ovrd_methods_new_cell";
			cell_body = L"new";
		}
		else
		{
			// This method was overriden in the current class.
			cell_body = L"ovrd";

			if (local_method->m_accs_level != m_method_override_roots[iroot]->m_accs_level)
			{
				// The access level is different.
				css_style = L"arp_ovrd_methods_highlighted_ovrd_cell";
			}
		}

		if (iroot % 6 == 0)
			rprt.AddSoftNewLine();

		// Emit the cell.
		TSimpleHtmlElement cell(L"td", cell_body, -1, css_style, crlm_none);
		rprt.EmitElement(cell);
	}

	// Emit the final balancing cell and close the row.
	TSimpleHtmlElement final_cell(L"td", NULL, 0, NULL, crlm_none);
	rprt.EmitElement(final_cell);
	rprt.EmitElementEnd(row);
}


