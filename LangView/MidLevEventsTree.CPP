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
#include  "LangView/MidLevEventsTree.H"
#include  "LangView/LangViewResources.H"

// ----------------------------------------------------------------------
//   ===============  TLangViewMidLevTreeDialog  =================
// ----------------------------------------------------------------------

TLangViewMidLevTreeDialog::TLangViewMidLevTreeDialog(TMidLevStoreReader *db_link)
		: TLangViewChildBrowser(db_link, cbrt_mid_lev_events_tree, IDD_LANGV_CHLD_MID_LEV_TREE),
		m_mlev_tree_root(NULL, 0, slit_center_button)
{
	m_placing_mode = plc_mode_rect_lines;
	m_mlev_tree_root.SetLexItemClickOwner();

	m_selected_node = NULL;
	m_selection_object = NULL;
}

TLangViewMidLevTreeDialog::~TLangViewMidLevTreeDialog()
{
	m_mlev_tree_root.ReleaseChildren();

	if (m_selection_object != NULL)
		delete m_selection_object;
}

LRESULT TLangViewMidLevTreeDialog::OnInitDialog(HWND hDefaultFocusControl)
{
	// Prepare resizing.
	AdjustControlRightSide(IDC_MLEVTR_BODY);
	SetupControlPositioning(IDC_MLEVTR_TITLE, cpm_fix_top_side);
	SetupControlPositioning(IDC_MLEVTR_BODY, cpm_fix_all_corners);

	// Prepare the lex items style without passing any specific grammar.
	m_lex_items_style.SetupLexItemsProps(NULL, 0);
	m_lex_items_style.SetupForDeviceContextEx(m_hDlg);

	// Prepare the screen items viewer.
	m_mlev_view_helper.SetWindow(::GetDlgItem(m_hDlg, IDC_MLEVTR_BODY));
	m_mlev_view_helper.SetStndScrollableProps(RGB(255, 255, 255), 40, 40);

	if (m_mlev_tree_root.GetFirstChild(FALSE) != NULL)
		ShowCurrentContents();

	SetFocusControl(IDC_MLEVTR_BODY);
	return(FALSE);
}

void TLangViewMidLevTreeDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDC_MLEVTR_BODY:
				{
					TScreenItemsViewMouseNotify *mouse_ntf = (TScreenItemsViewMouseNotify*)hWndControl;
					switch (notificationCode)
					{
						case xiv_left_btn_down:
						case xiv_left_btn_dblclk:
								{
									// Save the color of the pixel under the cursor tip.
									m_mlev_view_helper.SaveClickedPointColor(mouse_ntf);

									// Button was pressed in the screen items area.
									if (mouse_ntf->object != NULL)
									{
										TLangViewClickEvent click_event_info;
										TLangViewLexItem *lex_item = (TLangViewLexItem*)mouse_ntf->object->GetOwner();
										if (lex_item->PrepareClickEvent(click_event_info) == TRUE)
										{
											// Clicked item prepared meaningful event.
											SendForProcessingFromScreenItemsView((TScreenItemsViewNtfCode)notificationCode, click_event_info);
										}
									}
									else
									{
										// User clicked the background or some transparent object.
										if (m_owner != NULL)
											m_owner->RemoveSelection(this);
									}
								}
								break;

						case xiv_right_btn_down:
								{
									// Button was pressed in the screen items area.
									if (mouse_ntf->object == NULL)
										ProcessControlMenu(mouse_ntf->click_place.x, mouse_ntf->click_place.y);
								}
								break;

						case xiv_corner_clicked:
								{
									// Lower right corner was clicked.
									TScreenItemsViewNotify *nti = (TScreenItemsViewNotify*)hWndControl;
									if (nti->right_btn == TRUE)
										ProcessControlMenu(nti->pos_x, nti->pos_y);
								}
								break;
					}
				}
	}
}

void TLangViewMidLevTreeDialog::LoadParsingEvents(ID pars_id, PSET_TYPES_MASK event_types_mask, ID src_ref, ID min_evid, ID max_evid)
{
	MsgBox(L"ParsingEventsTreeDialog::LoadParsingEvents - NIY.");
}

void TLangViewMidLevTreeDialog::PrepareIncludeFilesReport(ID pars_id, bool add_hidden_inc_events, LERRC_TYPES_MASK err_warn_event_classes_mask)
{
	assert(m_db != NULL);
	ResetContents();

	if (pars_id == TProjectParsSessId)
	{
		// Replace this with the latest session id.
		pars_id = 0;
	}

	PSET_TYPES_MASK evts_mask = PSET_TYPE_BIT(pset_lex_source_enter) | PSET_TYPE_BIT(pset_lex_source_exit) | PSET_TYPE_BIT(pset_misc_preproc_stmt) | PSET_TYPE_BIT(pset_error_warning_event);
	if (m_db->StartParsingEventsIteration(pars_id, evts_mask) == TRUE)
	{
		TLangViewTreeNode *curr_nodes_owner = &m_mlev_tree_root;
		short nodes_owner_level = 0, curr_enc_level = 0;
		bool local_abort = FALSE;
		while (m_db->NextParsingEvent() == TRUE && local_abort == FALSE)
		{
			CtvParsingEventInfo &evt = m_db->EventData();
			switch (evt.hdr.event_type)
			{
				case pset_lex_source_enter:
						{
							CtvLexSourceEnterEvent &enter_evt = (CtvLexSourceEnterEvent&)evt;
							assert(enter_evt.src_type > srct_none && enter_evt.src_type < srct_num_types);
							assert(enter_evt.new_src_level == curr_enc_level+1);

							if (LEX_SRC_WHOLE_FILE(enter_evt.src_type) == TRUE)
							{
								// This is entering the file source. Current enclosure should be either the whole file or none.
								TLangViewMidLevObjectTreeNode *node = new TLangViewMidLevObjectTreeNode(curr_nodes_owner, 0, evt);
								if (node == NULL)
								{
									MsgBox(L"Error allocating the lex scr enter tree node. Some of the parsing events were not processed.");
									local_abort = TRUE;
									break;
								}

								node->SetLexItemClickOwner();

								// The node was created. Update the info about the current tree layer.
								curr_nodes_owner = node;
								nodes_owner_level = enter_evt.new_src_level;
							}

							// Counter of levels should be increased regardless of the event subtype.
							curr_enc_level++;
						}
						break;

				case pset_lex_source_exit:
						{
							CtvLexSourceExitEvent &exit_evt = (CtvLexSourceExitEvent&)evt;
							if (nodes_owner_level == curr_enc_level)
							{
								curr_nodes_owner = curr_nodes_owner->GetOwner();
								assert(curr_nodes_owner != NULL);
								nodes_owner_level--;
							}

							curr_enc_level--;
						}
						break;

				case pset_misc_preproc_stmt:
						{
							CtvMiscPreprocStmtEvent &misc_evt = (CtvMiscPreprocStmtEvent&)evt;
							if (misc_evt.mis_event_type == exmis_failed_include && add_hidden_inc_events == TRUE || misc_evt.mis_event_type == exmis_error)
							{
								// Event should belong to the tracked nesting layer. Ensure this.
								if (curr_enc_level != nodes_owner_level)
								{
									MsgBox(L"Misc preproc event belongs to the bogus lex src enclusure type. Some of the parsing events were not processed.");
									local_abort = TRUE;
									break;
								}

								TLangViewMidLevObjectTreeNode *node = new TLangViewMidLevObjectTreeNode(curr_nodes_owner, 0, evt);
								if (node == NULL)
								{
									MsgBox(L"Error allocating misc preproc event node. Some of the parsing events were not processed.");
									local_abort = TRUE;
									break;
								}

								node->SetLexItemClickOwner();
							}
						}
						break;

				case pset_error_warning_event:
						{
							// In some cases events will have wrong parent. Nevertheless do this.
							CtvErrorWarningEvent &err_warn_evt = (CtvErrorWarningEvent&)evt;
							if ((err_warn_event_classes_mask & LERRC_TYPE_BIT(err_warn_evt.err_class)) != 0)
							{
								TLangViewMidLevObjectTreeNode *node = new TLangViewMidLevObjectTreeNode(curr_nodes_owner, 0, evt);
								if (node == NULL)
								{
									MsgBox(L"Error allocating error warning event node. Some of the parsing events were not processed.");
									local_abort = TRUE;
									break;
								}

								node->SetLexItemClickOwner();
							}
						}
						break;
			}
		}

		m_db->CloseParsingEventsIteration();
	}

	if (m_hDlg != NULL && m_mlev_tree_root.GetFirstChild(FALSE) != NULL)
	{
		// The dialog is opened and the tree is not empty.
		ShowCurrentContents();
	}
}

void TLangViewMidLevTreeDialog::ResetContents()
{
	m_mlev_tree_root.ReleaseChildren();
	ResetViewInternal();
}

void TLangViewMidLevTreeDialog::SetSelection(TLangViewClickEvent &event, bool show_margins)
{
	ResetSelection();
	if (event.type != clet_mid_lev_event)
		return;

	// Passed event describes mid lev parsing event. Look for this event in the current tree.
	TLangViewTreeNode *new_slct_node = NULL;
	TGenericTreeNodesIterator iter(&m_mlev_tree_root);
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TLangViewTreeNode *basic_node = (TLangViewTreeNode*)iter.GetCurrentItem();
		if (basic_node->GetItemType() == litp_mid_lev_object)
		{
			TLangViewMidLevObjectTreeNode *node = (TLangViewMidLevObjectTreeNode*)basic_node;
			if (node->ObjectType() == mlit_parsing_event && node->Event().hdr.Equal(event.mldb_event_data.hdr) == TRUE)
			{
				// Requested item is present in the tree.
				new_slct_node = node;
				break;
			}
		}
	}

	if (iter.CheckIterationSuccess() == FALSE)
	{
		MsgBox(L"Some of the nodes were not iterated.");
	}

	if (new_slct_node != NULL)
	{
		// Show selection.
		CreateAndShowSelectionObject(new_slct_node);

		if (m_selection_object != NULL)
		{
			if (show_margins == TRUE || m_mlev_view_helper.CheckFullOrPartialScrollOut(*(new_slct_node->GetPrimaryBkgrItem())) == TRUE)
			{
				// Scroll selection into the view.
				long margins = 14;
				m_mlev_view_helper.ScrollScreenItemIntoTheView(*m_selection_object, margins, margins);
			}
		}
	}
}

void TLangViewMidLevTreeDialog::ResetSelection()
{
	if (m_selection_object != NULL)
	{
		// Initiate repainting the place where current selection object is staying.
		m_mlev_view_helper.InvalidateScreenItemRect(m_selection_object);
		m_mlev_view_helper.RemoveFromInternalDataSet(*m_selection_object);
		delete m_selection_object;
	}

	m_selected_node = NULL;
	m_selection_object = NULL;
}

void TLangViewMidLevTreeDialog::ProcessControlMenu(long click_pos_x, long click_pos_y)
{
	TLangViewChildBrowser::TSIVMR sivmr = ProcessScreenItemsViewControlMenu(m_mlev_view_helper, m_placing_mode, click_pos_x, click_pos_y);
	if (sivmr == TLangViewChildBrowser::sivmr_reload_view)
	{
		// Repaint the view.
		ShowCurrentContents();
	}
	else if (sivmr == TLangViewChildBrowser::sivmr_copy_to_clipborad)
	{
		// Place visible part of the tree to the clipboard.
		RECT margins = { 40, 30, 40, 40 };
		PutScreenShotToClipboard(m_mlev_view_helper, &margins);
	}
}

void TLangViewMidLevTreeDialog::ShowCurrentContents()
{
	// Remove the old data set if any.
	TLangViewTreeNode *slct_node = m_selected_node;
	ResetViewInternal();

	// Hide the link between root lex source and the center button object.
	TLangViewTreeNode *root_src = (TLangViewTreeNode*)m_mlev_tree_root.GetFirstChild(FALSE);
	if (root_src != NULL)
		root_src->SetSelectedStateEx(FALSE, FALSE, FALSE, FALSE);

	// Place the tree.
	void *contexts = &m_lex_items_style;
	switch (m_placing_mode)
	{
		case plc_mode_concentric:
				{
					TConcentricModeTreeGraphPlacer::DisplayGraph(m_mlev_view_helper, m_mlev_view_helper.InternalDataSet(), &m_mlev_tree_root,
													TLangViewLexItemStyle::g_DefaultConcentricPlacingProps, &contexts, 0, NULL, FALSE);
				}
				break;

		case plc_mode_rect_lines:
				{
					TRectLinesModeTreeGraphPlacer::DisplayGraph(m_mlev_view_helper, m_mlev_view_helper.InternalDataSet(), &m_mlev_tree_root,
													TLangViewLexItemStyle::g_DefaultRectLinesPlacingProps, &contexts, 0, NULL, FALSE);
				}
				break;

		default:
			assert(FALSE);
			break;
	}

	// Hide the center button object.
	m_mlev_tree_root.ResetScreenItems(m_mlev_view_helper.InternalDataSet());

	if (slct_node != NULL)
		CreateAndShowSelectionObject(slct_node);

	// For now show the center of the set even though the selection might be scrolled out.
	m_mlev_view_helper.ShowDataSet(&m_mlev_view_helper.InternalDataSet(), FALSE);
	m_mlev_view_helper.ScrollDataSetIntoCenter();
}

void TLangViewMidLevTreeDialog::ResetViewInternal()
{
	if (m_selection_object != NULL)
		delete m_selection_object;

	m_selected_node = NULL;
	m_selection_object = NULL;

	m_mlev_view_helper.InternalDataSet().Clear();
	m_mlev_view_helper.ShowInternalDataSet(TRUE);
}

void TLangViewMidLevTreeDialog::CreateAndShowSelectionObject(TLangViewTreeNode *node)
{
	assert(m_selected_node == NULL && m_selection_object == NULL);

	TScreenItem *sel_obj = node->CreateSelectionScreenItem(list_bold_blue);
	if (sel_obj == NULL)
	{
		MsgBox(L"Error creating selection object.");
		return;
	}

	// Add item to the painting list and invalidate its rect.
	m_mlev_view_helper.InternalDataSet().PrependItem(sel_obj);
	m_mlev_view_helper.InvalidateScreenItemRect(sel_obj);

	// Success. Save the data.
	m_selected_node = node;
	m_selection_object = sel_obj;
}


