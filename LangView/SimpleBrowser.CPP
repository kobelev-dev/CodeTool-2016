//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "TextScan/MidStoreXml.H"
#include  "LangView/SimpleBrowser.H"
#include  "LangView/LangViewResources.H"

// -----------------------------------------------------------------------------
//    =============  TLangViewBrowserHistoryControlDialog  =================
// -----------------------------------------------------------------------------

TLangViewBrowserHistoryControlDialog::TLangViewBrowserHistoryControlDialog()
		: TBasicDialog(NULL, IDD_LANGV_SVF_HISTORY_CONTROL)
{
	m_curr_step = m_total_steps = 0;
	m_owner = NULL;
}

LRESULT TLangViewBrowserHistoryControlDialog::OnInitDialog(HWND hDefaultFocusControl)
{
	SetState(0, 0);
	SetFocusControl(IDC_LVBHC_BTN_STEP_BACK);
	return(FALSE);
}

void TLangViewBrowserHistoryControlDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	assert(m_owner != NULL);

	switch (controlId)
	{
		case IDC_LVBHC_BTN_BEG:
				assert(m_curr_step > 0);
				m_owner->PlayHistoryTo(0);
				break;

		case IDC_LVBHC_BTN_STEP_BACK:
				assert(m_curr_step > 0);
				m_owner->PlayHistoryTo(m_curr_step-1);
				break;

		case IDC_LVBHC_BTN_STEP_FORW:
				assert(m_curr_step < m_total_steps-1);
				m_owner->PlayHistoryTo(m_curr_step+1);
				break;

		case IDC_LVBHC_BTN_END:
				assert(m_curr_step < m_total_steps-1);
				m_owner->PlayHistoryTo(m_total_steps-1);
				break;
	}
}

void TLangViewBrowserHistoryControlDialog::SetState(int new_curr_step, int new_total_steps)
{
	// Verify the input parameters.
	assert(new_curr_step >= 0 && new_total_steps >= 0);

	if (new_total_steps > 0)
	{
		assert(new_curr_step < new_total_steps);
	}
	else
	{
		assert(new_curr_step == 0);
	}

	// Accept values.
	m_curr_step = new_curr_step;
	m_total_steps = new_total_steps;

	// Resetup buttons.
	if (m_total_steps <= 1)
	{
		// Disable all control buttons.
		EnableControl(IDC_LVBHC_BTN_BEG, FALSE);
		EnableControl(IDC_LVBHC_BTN_STEP_BACK, FALSE);
		EnableControl(IDC_LVBHC_BTN_STEP_FORW, FALSE);
		EnableControl(IDC_LVBHC_BTN_END, FALSE);
	}
	else
	{
		// Check, if other steps are available on both sides of the curr step.
		EnableControl(IDC_LVBHC_BTN_BEG, (m_curr_step > 0) ? TRUE : FALSE);
		EnableControl(IDC_LVBHC_BTN_STEP_BACK, (m_curr_step > 0) ? TRUE : FALSE);
		EnableControl(IDC_LVBHC_BTN_STEP_FORW, (m_curr_step < m_total_steps-1) ? TRUE : FALSE);
		EnableControl(IDC_LVBHC_BTN_END, (m_curr_step < m_total_steps-1) ? TRUE : FALSE);
	}

	// Resetup the state label.
	EnableControl(IDC_LVBHC_LAB_STATE, (m_total_steps > 0) ? TRUE : FALSE);
	SetControlTextFmt(IDC_LVBHC_LAB_STATE, L"Step %d out of %d.", ((m_total_steps > 0) ? m_curr_step+1 : 0), m_total_steps);
}

// --------------------------------------------------------------------
//   ==============  TLangViewSimpleBrowser ===================
// --------------------------------------------------------------------

static wchar_t *g_SimpleBrowser_LocalParsSess_OpenFileFilterSpec = L"All Files (*.*)\0*.*\0";
static wchar_t *g_SimpleBrowser_LocalParsSess_DialogTitle = L" Open Source File ";

TLangViewSimpleBrowser::TLangViewSimpleBrowser(TLangViewBrowserFrame *parent_handler, TMidLevStoreReader *db_link, TSimpleBrowserParsingDriverInfo *drivers_array, int num_drivers)
		: TLangViewBrowserFrame(parent_handler, db_link, IDD_LANGV_SVF_MAIN_FRAME),
				m_ofn_source_file(NULL, g_SimpleBrowser_LocalParsSess_OpenFileFilterSpec, L"CPP", g_SimpleBrowser_LocalParsSess_DialogTitle),
				m_main_vert_dock(NULL,  10, IDC_SMPBRW_SHOW_HIDE_EVENT),
				m_upper_horz_dock(NULL, 160, IDC_SMPBRW_SHOW_HIDE_EVENT),
				m_lower_horz_dock(NULL, 80, IDC_SMPBRW_SHOW_HIDE_EVENT),
				m_upper_middle_dock(NULL, 100, IDC_SMPBRW_SHOW_HIDE_EVENT),
				m_lower_left_dock(NULL, 100, IDC_SMPBRW_SHOW_HIDE_EVENT),
				m_lower_right_dock(NULL, 110, IDC_SMPBRW_SHOW_HIDE_EVENT),
				m_events_tree(db_link), m_cpp_item_locs_list(db_link, TRUE), m_source_view(db_link, cbrt_src_primary),
				m_events_list(db_link, TRUE),
				m_lex_src_nesting(db_link), m_cond_pars_nesting(db_link),
				m_nesting_context(db_link, cbrt_src_nest_ctx),
				m_slct_details(db_link),
				m_slct_context(db_link, cbrt_event_dtls_ctx, 0, RGB(255, 255, 255))
{
	// Accept the list of external parsing drivers if any.
	m_external_drivers_info = drivers_array;
	m_num_external_drivers = num_drivers;

	// Splitters and docks.
	m_main_vert_dock.SetDialogParams(this);
	m_upper_horz_dock.SetDialogParams(this);
	m_lower_horz_dock.SetDialogParams(this);
	m_lower_left_dock.SetDialogParams(this);
	m_lower_right_dock.SetDialogParams(this);

	// Main child browsers.
	m_events_tree.SetDialogParams(this);
	m_events_tree.SetOwner(this);
	m_source_view.SetDialogParams(this);
	m_source_view.SetOwner(this);
	m_events_list.SetDialogParams(this);
	m_events_list.SetOwner(this);

	// Selection origin browsers.
	m_lex_src_nesting.SetDialogParams(this);
	m_lex_src_nesting.SetOwner(this);
	m_cond_pars_nesting.SetDialogParams(this);
	m_cond_pars_nesting.SetOwner(this);
	m_nesting_context.SetDialogParams(this);
	m_nesting_context.SetOwner(this);
	m_nesting_context.SetShowHideScrollersMode(TRUE, FALSE);
	m_nesting_context.SetSpecialClickProcessingModes(TRUE, TRUE);

	// Selection details browsers.
	m_slct_details.SetDialogParams(this);
	m_slct_details.SetOwner(this);
	m_slct_context.SetDialogParams(this);
	m_slct_context.SetOwner(this);
	m_slct_context.SetSpecialClickProcessingModes(TRUE, TRUE);

	// Browsing history.
	m_brows_hist_control.SetDialogParams(this);
	m_brows_hist_control.SetOwner(this);

	m_curr_hist_step = 0;
	m_task_parsing_id = m_task_events_mask = 0;
}

void TLangViewSimpleBrowser::SetupDataBase(TMidLevStoreReader *db_link)
{
	// Setup database into the base object.
	TLangViewBrowserFrame::SetupDataBase(db_link);

	// Proparage database into the child viewers.
	m_events_tree.SetDataBase(db_link);
	m_source_view.SetDataBase(db_link);
	m_events_list.SetDataBase(db_link);
	m_lex_src_nesting.SetDataBase(db_link);
	m_cond_pars_nesting.SetDataBase(db_link);
	m_nesting_context.SetDataBase(db_link);
	m_slct_details.SetDataBase(db_link);
	m_slct_context.SetDataBase(db_link);
}

LRESULT TLangViewSimpleBrowser::OnInitDialog(HWND hDefaultFocusControl)
{
	// Setup objects on the dialog level.
	SetControlPos(IDCANCEL, 0, -30);

	/* -BUGBUG-
	HMENU hFileMenu = ::GetSubMenu(::GetMenu(m_hDlg), 0);
	if (hFileMenu != NULL)
	{
		// The file menu is present in the dialog.
		// Isert 2 sample menu items.
		BOOL r1 = ::InsertMenuW(hFileMenu, IDM_SMPBRW_MENU_FILE_EXIT, MF_BYCOMMAND | MF_STRING, 20398, L"Abra-cadabra");
		BOOL r2 = ::InsertMenuW(hFileMenu, IDM_SMPBRW_MENU_FILE_EXIT, MF_BYCOMMAND | MF_SEPARATOR, -1, NULL);
	}
	*/

	// Create main dock dialog and place it over the whole frame.
	HWND hMainDockWnd = m_main_vert_dock.OpenNonModalDialog(m_hDlg, IDC_SMPBRW_MAIN_VERT_DOCK);
	if (hMainDockWnd == NULL)
		return(-1);

	// Place the main dock over the whole frame.
	RECT rc_client;
	::GetClientRect(m_hDlg, &rc_client);

	int upper_offs = 2;
	SetControlPos(IDC_SMPBRW_MAIN_VERT_DOCK, 0, upper_offs);
	SetControlSize(IDC_SMPBRW_MAIN_VERT_DOCK, rc_client.right, rc_client.bottom-upper_offs);
	SetupControlPositioning(IDC_SMPBRW_UPPER_GRP_BOX, cpm_fix_top_side);
	SetupControlPositioning(IDC_SMPBRW_MAIN_VERT_DOCK, cpm_fix_all_corners);

	// Add upper and lower horz docks to the main vert dock.
	if (m_main_vert_dock.ShowChildDialog(&m_upper_horz_dock, FALSE) == FALSE)
		return(-1);
	if (m_main_vert_dock.ShowChildDialog(&m_lower_horz_dock, FALSE) == FALSE)
		return(-1);

	// Add objects to the upper dock.
	if (m_upper_horz_dock.ShowChildDialog(&m_events_tree, FALSE) == FALSE)
		return(-1);
	if (m_upper_horz_dock.ShowChildDialog(&m_source_view, FALSE) == FALSE)
		return(-1);

	// Add objects to the lower dock.
	if (m_lower_horz_dock.ShowChildDialog(&m_lower_left_dock, FALSE) == FALSE)
		return(-1);
	if (m_lower_horz_dock.ShowChildDialog(&m_events_list, FALSE) == FALSE)
		return(-1);
	if (m_lower_horz_dock.ShowChildDialog(&m_lower_right_dock, FALSE) == FALSE)
		return(-1);

	// Add objects to the left dock.
	if (m_lower_left_dock.ShowChildDialog(&m_lex_src_nesting, FALSE) == FALSE)
		return(-1);
	if (m_lower_left_dock.ShowChildDialog(&m_cond_pars_nesting, FALSE) == FALSE)
		return(-1);
	if (m_lower_left_dock.ShowChildDialog(&m_nesting_context, FALSE) == FALSE)
		return(-1);

	// Add objects to the right dock.
	if (m_lower_right_dock.ShowChildDialog(&m_slct_details, FALSE) == FALSE)
		return(-1);
	if (m_lower_right_dock.ShowChildDialog(&m_slct_context, FALSE) == FALSE)
		return(-1);
	if (m_lower_right_dock.ShowChildDialog(&m_brows_hist_control, FALSE, -1, NULL, FALSE) == FALSE)
		return(-1);

	// Hide some of the child browsers immediately. They were displayed just to make sure that
	// they can be opened without problems.
	m_lower_left_dock.HideChildDialog(&m_nesting_context);
	m_lower_right_dock.HideChildDialog(&m_slct_context);
	m_lower_right_dock.HideChildDialog(&m_brows_hist_control);

	// Load app level data into the browser.
	if (m_task_events_mask != 0)
		::PostMessage(m_hDlg, WM_COMMAND, IDC_SMPBRW_EXEC_TASK, 0);

	// Success.
	return(FALSE);
}

void TLangViewSimpleBrowser::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDM_SMPBRW_MENU_FILE_OPEN_AND_RUN:
				{
					if (m_ofn_source_file.GetOpenFileName(m_hDlg) == TRUE)
					{
						// The file is selected.
						MsgBox(m_ofn_source_file.GetCurrFileName());
					}
				}
				break;

		case IDCANCEL:
		case IDM_SMPBRW_MENU_FILE_EXIT:
				{
					::DestroyWindow(m_hDlg);
				}
				break;

		case IDM_SMPBRW_MENU_ACTS_WINDOW_SIZE:
				{
					// Set special window size for making screen shots in the consistent resolution.
					::SetWindowPos(m_hDlg, NULL, 0, 0, 1200, 820, SWP_NOMOVE | SWP_NOZORDER);
				}
				break;

		case IDM_SMPBRW_MENU_ACTS_COLOR_EDIT:
				{
					// Display the Windows color select dialog.
					TControlHelper::ShowColorSelectDialogEx(this, L" Select color ");
				}
				break;

		case IDM_SMPBRW_MENU_HELP_ABOUT:
				{
					// Display the about box.
					TLangViewAboutBox about_box(L"Simple Viewer Application", L"Version October 2011", COMMUNITY_EDITION);
					about_box.OpenModalDialog(m_hDlg);
				}
				break;

		case IDC_SMPBRW_EXEC_TASK:
				{
					SetUserCancel(FALSE);
					HCURSOR hPrevCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

					// Less details.
					// m_events_tree.PrepareIncludeFilesReport(m_task_parsing_id, FALSE, 0);

					// More details.
					m_events_tree.PrepareIncludeFilesReport(m_task_parsing_id, TRUE, LERRC_ERRORS);

					m_events_list.LoadParsingEvents(m_task_parsing_id, m_task_events_mask);
					if (hPrevCursor != NULL)
						::SetCursor(hPrevCursor);
				}
				break;
	}
}

bool TLangViewSimpleBrowser::OnWmDestroy()
{
	// The database reader instance might be leaked here.
	SetupDataBase(NULL);
	m_task_parsing_id = m_task_events_mask = 0;
	return(TLangViewBrowserFrame::OnWmDestroy());
}

void TLangViewSimpleBrowser::ProcessSingleClick(TLangViewChildBrowser *event_sender, TLangViewClickEvent &event_info)
{
	if (event_sender == &m_source_view)
	{
		// Select passed event in all main viewers.
		m_source_view.SetSelection(event_info, FALSE);
		m_events_tree.SetSelection(event_info, TRUE);
		m_events_list.SetSelection(event_info, TRUE);

		// Propagate this event to secondary viewers.
		SetupSecondaryViewers(event_info);
	}
	else if (event_sender == &m_events_tree || event_sender == &m_events_list)
	{
		// These browsers should send only the mid lev events.
		assert(event_info.type == clet_mid_lev_event);
		SetupClickEvent(event_info, FALSE);
	}
	else if (event_sender == &m_lex_src_nesting || event_sender == &m_cond_pars_nesting)
	{
		// Remove potential selection from the other listbox.
		if (event_sender == &m_lex_src_nesting)
			m_cond_pars_nesting.ResetSelection();
		else m_lex_src_nesting.ResetSelection();

		// Show selection.
		event_sender->SetSelection(event_info, FALSE);

		// Show selection context.
		m_lower_left_dock.ShowChildDialog(&m_nesting_context, FALSE);
		SetupSourceCodeViewer(m_nesting_context, event_info);
	}
}

void TLangViewSimpleBrowser::ProcessDoubleClick(TLangViewChildBrowser *event_sender, TLangViewClickEvent &event_info)
{
	if (event_sender == &m_lex_src_nesting || event_sender == &m_cond_pars_nesting || event_sender == &m_nesting_context || event_sender == &m_slct_context)
	{
		// Propagate current context of these secondary viewers into the main viewers.
		SetupClickEvent(event_info, TRUE);
	}
}

void TLangViewSimpleBrowser::RemoveSelection(TLangViewChildBrowser *event_sender)
{
	switch (event_sender->GetType())
	{
		case cbrt_lex_src_nest:
		case cbrt_cond_pars_nest:
				{
					// Remove selection in the nesting views only and hide the nesting source context dialog.
					m_lex_src_nesting.ResetSelection();
					m_cond_pars_nesting.ResetSelection();
					m_lower_left_dock.HideChildDialog(&m_nesting_context);
				}
				break;

		case cbrt_src_nest_ctx:
				{
					// Remove selection in nesting viewers.
					m_lex_src_nesting.ResetSelection();
					m_cond_pars_nesting.ResetSelection();
					m_nesting_context.ResetSelection();
				}
				break;

		case cbrt_event_dtls_ctx:
				{
					// Remove selection only locally.
					m_slct_context.ResetSelection();
				}
				break;

		default:
			{
				// Remove selection everywhere.
				m_events_tree.ResetSelection();
				m_source_view.ResetSelection();
				m_events_list.ResetSelection();
				m_lex_src_nesting.ResetContents();
				m_cond_pars_nesting.ResetContents();
				m_lower_left_dock.HideChildDialog(&m_nesting_context);
				m_slct_details.ResetContents();
				m_lower_right_dock.HideChildDialog(&m_slct_context);
			}
			break;
	}
}

void  TLangViewSimpleBrowser::ProcessResortEvent(TLangViewChildBrowser *event_sender, int list_id, DWORD new_sort_order)
{
	// Simply call the requestor back.
	event_sender->SetSortOrder(list_id, new_sort_order);
}

void TLangViewSimpleBrowser::SetupClickEvent(TLangViewClickEvent &event_info, bool show_margins)
{
	//
	// This is global setup that comletely changes the state of the browser.
	//

	// Show and select this event on the main code viewer.
	m_events_tree.SetSelection(event_info, show_margins);
	m_events_list.SetSelection(event_info, show_margins);
	SetupSourceCodeViewer(m_source_view, event_info);

	// Select event on the main file view.
	if (event_info.type != clet_code_area || event_info.code_area.lin_area.linear_beg > 0)
		m_source_view.SetSelection(event_info, TRUE);

	// Propagate this event to secondary viewers.
	SetupSecondaryViewers(event_info);
}

void TLangViewSimpleBrowser::SetupSecondaryViewers(TLangViewClickEvent &event_info)
{
	// Convert click event into the event header that contains info about the clicked place.
	CtvEventHeader nesting_header;
	nesting_header.Clear();

	TSourceLocation evnt_src_loc;
	if (event_info.GetSrcLocation(evnt_src_loc) == TRUE && evnt_src_loc.parsing_id != 0)
	{
		// Passed event has a source location that belongs to some parsing session.
		if (evnt_src_loc.lin_area.linear_beg > 0)
		{
			nesting_header.Init(evnt_src_loc.parsing_id, evnt_src_loc.lin_area);
		}
		else if (evnt_src_loc.source_ref != 0 && m_db->GetLexSourceEnterEvent(evnt_src_loc.parsing_id, evnt_src_loc.source_ref) == TRUE)
		{
			// Use empty linear area at the beginning of the lex src enclosure inside it.
			TLinearArea lin_beg_inside = { m_db->EventData().src_enter.LinearPosAfterEntering(), 0 };
			nesting_header.Init(m_db->EventData().hdr.parsing_id, lin_beg_inside);
		}
	}
	else if (event_info.type == clet_mid_lev_event)
	{
		// This type of event should have non empty event header.
		nesting_header = event_info.mldb_event_data.hdr;
	}

	// Setup nesting viewers.
	if (nesting_header.parsing_id != 0)
	{
		// Event header was retrieved.
		m_lex_src_nesting.LoadContents(nesting_header);
		m_cond_pars_nesting.LoadContents(nesting_header);
	}
	else
	{
		// Passed event does not have any associated territory.
		m_lex_src_nesting.ResetContents();
		m_cond_pars_nesting.ResetContents();
	}

	// Nesting context should be initially invisible.
	m_lower_left_dock.HideChildDialog(&m_nesting_context);

	// Show click event details.
	m_slct_details.LoadContents(event_info);

	// Check/display the event context.
	bool call_context_displayed = FALSE;
	if (event_info.type == clet_mid_lev_event)
	{
		if (event_info.mldb_event_data.hdr.event_type == pset_lex_source_enter)
		{
			// Display the lower context. Note that ShowChildDialog() method will work fine regardless if
			// the history control dialog is visible or not.
			m_lower_right_dock.ShowChildDialog(&m_slct_context, FALSE, -1, &m_brows_hist_control);
			m_slct_context.LoadContents(NULL, event_info.mldb_event_data.src_enter);
			call_context_displayed = TRUE;
		}
		else if (event_info.mldb_event_data.hdr.event_type == pset_cond_param_use)
		{
			CtvCondParamUseEvent &cprm_use = event_info.mldb_event_data.cond_param;
			if (cprm_use.decor_mdef_id != 0 && m_db->GetSourceRefEvent(event_info.mldb_event_data.hdr.parsing_id, cprm_use.decor_mdef_id) == TRUE)
			{
				// Cond param use has related macrodefinition and the source ref event of this macrodef was retrieved.
				m_lower_right_dock.ShowChildDialog(&m_slct_context, FALSE, -1, &m_brows_hist_control);
				m_slct_context.LoadContents(NULL, m_db->EventData().src_enter);
				call_context_displayed = TRUE;

				// Retrieve the src location of this event to do the proper scrolling because the context file might be huge.
				TSourceLocation cprm_def_loc;
				if (m_db->GetSourceLocationInfo(cprm_def_loc, event_info.mldb_event_data.hdr.parsing_id, cprm_use.decor_mdef_id) == TRUE)
				{
					m_slct_context.ScrollViewerTo(cprm_def_loc.src_area);
				}
			}
		}
	}

	if (call_context_displayed == FALSE)
		m_lower_right_dock.HideChildDialog(&m_slct_context);
}

void TLangViewSimpleBrowser::SetupSourceCodeViewer(TLangViewSourceFileDialog &browser, TLangViewClickEvent &event_info)
{
	// Pick up the call event that defines the territory of the click event if it has any.
	CtvLexSourceEnterEvent call_event;
	call_event.Clear();

	TSourceLocation evnt_src_loc;
	if (event_info.GetSrcLocation(evnt_src_loc) == TRUE && evnt_src_loc.parsing_id != 0)
	{
		// Passed event contains source location that belongs to some parsing space.
		if (evnt_src_loc.lin_area.linear_beg > 0 && m_db->GetSourceRefEvent(evnt_src_loc.parsing_id, evnt_src_loc.lin_area.linear_beg) == TRUE)
			call_event = m_db->EventData().src_enter;
		else if (evnt_src_loc.source_ref != 0 && m_db->GetLexSourceEnterEvent(evnt_src_loc.parsing_id, evnt_src_loc.source_ref) == TRUE)
			call_event = m_db->EventData().src_enter;
	}
	else if (event_info.type == clet_mid_lev_event && event_info.mldb_event_data.hdr.area.IsNull() == FALSE)
	{
		// Parsing event has the source location. Try to resolve this location.
		if (m_db->GetSourceRefEvent(event_info.mldb_event_data) == TRUE && m_db->EventData().hdr.event_id != 0)
			call_event = m_db->EventData().src_enter;
	}

	// Check the results of picking.
	if (call_event.hdr.event_type == pset_lex_source_enter)
	{
		// Current click event belongs to some source enclosure.
		browser.LoadContents(NULL, call_event);
	}
	else
	{
		// The source enclosure is missing or this is some data corruption.
		browser.ResetContents();
	}
}


