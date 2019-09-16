//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "WinUi/DialogsDock.H"
#include  "WinUi/DialogControlHelpers.H"

//------------------------------------------------------------------------
//  ==================  TBasicDialogsDock  ======================
//------------------------------------------------------------------------

TBasicDialogsDock::TBasicDialogsDock(short controls_base, short controls_area_len,
								short show_hide_event_ctrl_id, bool need_first_splitter,
								int splitter_lo_delim, int splitter_width, int splitter_hi_delim,
								DWORD *splitter_frgr_color)
{
	m_hOwnerDialog = NULL;

	m_ctrl_base_splt = controls_base;
	m_ctrl_base_dlg = controls_base+controls_area_len/2;
	m_show_hide_event = show_hide_event_ctrl_id;

	m_splt_need_first = need_first_splitter;
	m_splt_lo_delim = splitter_lo_delim;
	m_splt_width = splitter_width;
	m_splt_hi_delim = splitter_hi_delim;

	m_splt_specific_frgr_color = FALSE;
	if (splitter_frgr_color != NULL)
	{
		m_splt_frgr_color = *splitter_frgr_color;
		m_splt_specific_frgr_color = TRUE;
	}
}

void TBasicDialogsDock::ResetDock()
{
	ReleaseOwnedDialogs();
	m_ownership_data.Clear();
	data().ResetHelper();
}

void TBasicDialogsDock::SetupReferenceRect(HWND hDlg, bool refresh_resplit_weights, RECT *new_ref_rect)
{
	data().SetupReferenceRect(hDlg, refresh_resplit_weights, new_ref_rect);
}

bool TBasicDialogsDock::ShowDialog(TBasicDialog *dlg, bool bypass_ownership, long preferred_dialog_width, TBasicDialog *ins_before, bool variab_size_dlg, long min_dialog_width)
{
	// Check, maybe the dock is already full.
	if (NumDialogs() >= (m_ctrl_base_dlg-m_ctrl_base_splt))
		return(FALSE);

	// Check if the passed dialog is opened or not. If it is not opened, open it and verify that its window id is zero.
	HWND hWnd = dlg->GetWnd();
	bool bWndLocallyCreated = FALSE;
	if (hWnd == NULL)
	{
		hWnd = dlg->OpenNonModalDialog(m_hOwnerDialog);
		bWndLocallyCreated = TRUE;
	}
	else
	{
		::SetParent(hWnd, m_hOwnerDialog);
	}

	if (hWnd == NULL || ::GetWindowLong(hWnd, GWL_ID) != 0)
		return(FALSE);

	if (preferred_dialog_width >= 0)
	{
		RECT dlg_rect;
		::GetWindowRect(hWnd, &dlg_rect);

		if (IsHorzDock() == TRUE)
			::SetWindowPos(hWnd, NULL, 0, 0, preferred_dialog_width, dlg_rect.bottom-dlg_rect.top, SWP_NOMOVE | SWP_NOZORDER);
		else ::SetWindowPos(hWnd, NULL, 0, 0, dlg_rect.right-dlg_rect.left, preferred_dialog_width, SWP_NOMOVE | SWP_NOZORDER);
	}

	// Find position to insert at and prepare ids for new objects. Note that if caller will pass ins_before as NULL
	// or some dialog that currently does not belong to the dock (say it is hidden), then the target dialog will be
	// appened to the end.
	int inx_before = FindDialog(ins_before);
	int new_splt_id = m_ctrl_base_splt+inx_before;
	int new_dlg_id  = m_ctrl_base_dlg+inx_before;

	// Prefix the control Ids.
	RenumberControls(inx_before, TRUE);

	// Create the splitter control window.
	HWND hSplitter = NULL;
	if (m_splt_need_first == TRUE || NumDialogs() > 0)
	{
		if (m_splt_need_first == FALSE && inx_before == 0)
		{
			// This will be a splitter in the front of the old first dialog, not in the front of the dialog that is being inserted now.
			new_splt_id++;
		}

		hSplitter = ::CreateWindowW(((IsVertDock() == TRUE) ? DLG_CTRL_HORZ_SPLITTER : DLG_CTRL_VERT_SPLITTER),
								NULL, WS_CHILD, 0, 0, m_splt_width, 1, m_hOwnerDialog, (HMENU)(INT_PTR)new_splt_id, dlg->GetInst(), NULL);
		if (hSplitter == NULL)
		{
			// Creation of the splitter window failed. Revert the state of the dock back.
			RenumberControls(inx_before, FALSE);
			if (bWndLocallyCreated == TRUE)
				::DestroyWindow(hWnd);
			return(FALSE);
		}
	}

	// Set the control ID to the dialog window.
	::SetWindowLong(hWnd, GWL_ID, new_dlg_id);

	// Insert information about new controls into three lists.
	bool r1 = TRUE, r2 = TRUE;
	if (inx_before > 0 || m_splt_need_first == TRUE)
	{
		// Ordinary procedure with splitter in front of the dialog. Order of insertions is opposide. Push the dialog first.
		int inx_in_helper = AdjustHelperInxForSplitter(inx_before);
		r1 = data().SetupControl(m_hOwnerDialog, new_dlg_id,
							TRUE, m_splt_hi_delim, m_splt_hi_delim,
							!variab_size_dlg, min_dialog_width, -1,
							cpsm_stretch_size, inx_in_helper, FALSE);
		r2 = data().SetupControl(m_hOwnerDialog, new_splt_id,
							TRUE, m_splt_lo_delim, m_splt_lo_delim,
							TRUE, m_splt_width, m_splt_width,
							cpsm_stretch_size, inx_in_helper, FALSE);
	}
	else
	{
		assert(inx_before == 0);
		if (NumDialogs() > 0)
		{
			// Special procedure with splitter after the dialog. Push the splitter first and the dialog after it.
			r2 = data().SetupControl(m_hOwnerDialog, new_splt_id,
								TRUE, m_splt_lo_delim, m_splt_lo_delim,
								TRUE, m_splt_width, m_splt_width,
								cpsm_stretch_size, 0, FALSE);
			r1 = data().SetupControl(m_hOwnerDialog, new_dlg_id,
								TRUE, m_splt_hi_delim, m_splt_hi_delim,
								!variab_size_dlg, min_dialog_width, -1,
								cpsm_stretch_size, 0, FALSE);
		}
		else
		{
			// Show the dialog only.
			assert(hSplitter == NULL);
			r1 = data().SetupControl(m_hOwnerDialog, new_dlg_id,
								TRUE, 0, 0, !variab_size_dlg, min_dialog_width, -1,
								cpsm_stretch_size, 0, TRUE);
		}
	}

	// Save the bypass_ownership flag and check the results.
	bool r3 = m_ownership_data.InsertItem(inx_before, bypass_ownership);
	if (r1 == FALSE || r2 == FALSE || r3 == FALSE)
	{
		// One of the operations failed. Revert the changes back.
		if (r1 == TRUE)
			data().ReleaseControl(m_hOwnerDialog, new_dlg_id);
		if (r2 == TRUE && hSplitter != NULL)
			data().ReleaseControl(m_hOwnerDialog, new_splt_id);
		if (r3 == TRUE)
			m_ownership_data.ReleaseItem(inx_before);

		RenumberControls(inx_before, FALSE);
		return(FALSE);
	}

	if (inx_before == 0)
	{
		// The item was inserted into the beginning of the list.
		data().m_data[0].m_front_delim.m_min_width = 0;
		if (NumDialogs() > 1)
			data().m_data[AdjustHelperInxForSplitter(1)].m_front_delim.m_min_width = m_splt_lo_delim;
	}

	// Full success.
	if (m_show_hide_event != 0)
		::SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(m_show_hide_event, TRUE), 0);

	data().PositionToRect(m_hOwnerDialog);
	if (hSplitter != NULL)
		::ShowWindow(hSplitter, SW_SHOW);

	::ShowWindow(hWnd, SW_SHOW);
	return(TRUE);
}

bool TBasicDialogsDock::HideDialog(TBasicDialog *dlg)
{
	int inx = FindDialog(dlg);
	if (inx == NumDialogs())
	{
		// This dialog does not belong to the dock.
		return(FALSE);
	}

	// The dialog is present in the dock. Pick up its current width as it would be if the width of the dock
	// would be equal to the sum of the widths of all its elements.
	long resplit_based_width = data().GetResplitWeight(AdjustHelperInxForDialog(inx));

	if (inx == 0 && NumDialogs() > 1)
	{
		// The item is removed from the beginning while it is not the only one object in the list.
		// Update the lo_delims. It is funny that updated item can be either delim or a dialog.
		// Regardless of that they should be updated in the same way.
		data().m_data[2].m_front_delim.m_min_width = 0;
	}

	// Remove contols from the helper.
	int splitter_id = 0;
	if (m_splt_need_first == TRUE || inx > 0)
		splitter_id = m_ctrl_base_splt+inx;
	else if (NumDialogs() > 1)
		splitter_id = m_ctrl_base_splt+1;

	if (splitter_id != 0)
	{
		// The splitter is not more needed.
		data().ReleaseControl(m_hOwnerDialog, splitter_id, FALSE);
		::DestroyWindow(::GetDlgItem(m_hOwnerDialog, splitter_id));
	}

	data().ReleaseControl(m_hOwnerDialog, m_ctrl_base_dlg+inx, TRUE);

	HWND hWnd = ::GetDlgItem(m_hOwnerDialog, m_ctrl_base_dlg+inx);
	assert(hWnd == dlg->GetWnd());

	// Hide the window of the dialog and reset its id.
	::ShowWindow(hWnd, SW_HIDE);
	if (m_show_hide_event != 0)
		::SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(m_show_hide_event, FALSE), 0);
	::SetWindowLong(hWnd, GWL_ID, 0);

	// Save the current resplit weight in the size of the dialog.
	RECT rc;
	::GetWindowRect(hWnd, &rc);
	data().SetPrimaryPos(rc, 0, resplit_based_width);
	::SetWindowPos(hWnd, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER | SWP_NOMOVE);

	// Objects got shifted in the helper's list. Reassign their ids.
	RenumberControls(inx, FALSE);
	m_ownership_data.ReleaseItem(inx);

	// This method is not destroying the dialog object. Ownership on the dlg object is returned back
	// to the caller even if the dialog was owned by the dock.
	return(TRUE);
}

void TBasicDialogsDock::ProcessResplitCommand(int ctrl_id, int ntf_code)
{
	// Check that the source of the event falls into the range of control ids of the splitters that were created by this helper.
	if (ctrl_id < m_ctrl_base_splt || ctrl_id >= m_ctrl_base_dlg)
	{
		// Controls id is not in the control ids range of this object.
		return;
	}

	// Process event from the splitter. The ntf_code is the resplit change value in pixels. This is a signed value that
	// comes in a low 16 bits of the 32 bit integer.
	short shift_value = (short)ntf_code;
	data().AdjustControlPosition(m_hOwnerDialog, ctrl_id, shift_value, shift_value);
}

void TBasicDialogsDock::BroadcastCommand(int ctrl_id, int ntf_code, LPARAM lParam)
{
	for (int inx=0; inx<NumDialogs(); ++inx)
	{
		HWND hWnd = ::GetDlgItem(m_hOwnerDialog, m_ctrl_base_dlg+inx);
		if (hWnd == NULL)
			continue;
		::SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(ctrl_id, ntf_code), lParam);
	}
}

TBasicDialog *TBasicDialogsDock::GetChildDialog(int inx)
{
	if (inx < 0 || inx >= NumDialogs())
		return(NULL);

	HWND hWnd = ::GetDlgItem(m_hOwnerDialog, m_ctrl_base_dlg+inx);
	if (hWnd == NULL)
		return(NULL);

	// Return whatever this control keeps in its USERDATA memory.
	return((TBasicDialog*)(LONG_PTR)::GetWindowLong(hWnd, GWL_USERDATA));
}

int TBasicDialogsDock::FindDialog(TBasicDialog *dlg)
{
	int num_dlgs = NumDialogs();
	if (dlg != NULL)
	{
		for (int inx=0; inx<num_dlgs; ++inx)
		{
			TBasicDialog *inst = GetChildDialog(inx);
			if (inst == NULL)
				continue;
			if (inst == dlg)
				return(inx);
		}
	}

	return(num_dlgs);
}

void TBasicDialogsDock::RenumberControls(int inx_start, bool ins_mode)
{
	if (inx_start < 0)
	{
		// There are no objects to update.
		return;
	}

	// Depending on direction of the shift, controls are processed either from beginning to the end
	// or in the opposite direction. Modify both the id of the control and reference to it in the hlpr.
	int num_dlgs = NumDialogs();
	if (ins_mode == TRUE)
	{
		for (int i=num_dlgs-1; i >= inx_start; --i)
		{
			RenumberOneControl(AdjustHelperInxForSplitter(i), m_ctrl_base_splt+i, 1);
			RenumberOneControl(AdjustHelperInxForDialog(i), m_ctrl_base_dlg+i, 1);
		}
	}
	else
	{
		for (int i=inx_start; i<num_dlgs; ++i)
		{
			RenumberOneControl(AdjustHelperInxForSplitter(i), m_ctrl_base_splt+i+1, -1);
			RenumberOneControl(AdjustHelperInxForDialog(i), m_ctrl_base_dlg+i+1, -1);
		}
	}
}

void TBasicDialogsDock::RenumberOneControl(int inx_in_adj_helper, int expected_ctrl_id, int ctrl_id_shift)
{
	// Index can be negative for missing splitter before the first dlg.
	if (inx_in_adj_helper < 0)
		return;

	// Pick up the slot and check that it contains ctrl with expected id.
	TSpringBasedControlPosHelper::TSpringControlPosInfo &slot = data().m_data[inx_in_adj_helper];
	assert(slot.m_ctrl_id == expected_ctrl_id);

	// Change the coltrol id in the slot and in the ctlr itself.
	HWND hWnd = ::GetDlgItem(m_hOwnerDialog, slot.m_ctrl_id);
	slot.m_ctrl_id += ctrl_id_shift;
	if (hWnd != NULL)
		::SetWindowLong(hWnd, GWL_ID, slot.m_ctrl_id);
}

void TBasicDialogsDock::ReleaseOwnedDialogs()
{
	// Release all owned dialogs.
	int num_dlgs = NumDialogs();
	for (int inx=0; inx<num_dlgs; ++inx)
	{
		// All dialog windows should be closed regardless if they are owned or not.
		TBasicDialog *inst = GetChildDialog(inx);
		if (inst == NULL)
			continue;

		if (inst->GetWnd() != NULL)
			::DestroyWindow(inst->GetWnd());

		if (m_ownership_data[inx] == TRUE)
		{
			// This dialog is owned by the dock. Destroy it.
			delete inst;
		}
	}
}

// ------------------------------------------------------------------------
//   ===============  THorzBasicDialogsDockDialog ===================
// ------------------------------------------------------------------------

THorzBasicDialogsDockDialog::THorzBasicDialogsDockDialog(TBasicDialog *proto, short dock_weight_in_dbu, short show_hide_event_ctrl_id,
									bool need_first_splitter, int splitter_width, bool bottom_align, DWORD *splitter_frgr_color)
				: m_dock(1000, 2000, show_hide_event_ctrl_id, need_first_splitter, 0, splitter_width, 0, bottom_align, 0, 0, splitter_frgr_color)
{
	if (proto != NULL)
		SetDialogParams(proto);

	// Prepare the dialog template in the data field of this class.
	WORD *buff_ptr = (WORD*)m_dialog_template;
	PrepareBasicDialogTemplate(buff_ptr, (WS_CHILD | WS_VISIBLE), 0, dock_weight_in_dbu, dock_weight_in_dbu, FALSE);

	// Assign the address of template to the data field of the base class.
	m_inMemoryDlgTemplate = (DLGTEMPLATE*)m_dialog_template;
}

LRESULT THorzBasicDialogsDockDialog::OnInitDialog(HWND hDefFocusControl)
{
	// Tell the splitter to use the whole space of the curr dialog.
	m_dock.SetOwner(m_hDlg);
	RECT clientRect;
	::GetClientRect(m_hDlg, &clientRect);
	m_dock.SetupReferenceRect(m_hDlg, FALSE, &clientRect);
	return(TRUE);
}

void THorzBasicDialogsDockDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	// Only the resplitting messages should come here.
	m_dock.ProcessResplitCommand(controlId, notificationCode);
}

void THorzBasicDialogsDockDialog::OnWmSize()
{
	RECT clientRect;
	::GetClientRect(m_hDlg, &clientRect);
	m_dock.PositionToRect(&clientRect);
	::InvalidateRect(m_hDlg, NULL, TRUE);
}

// ------------------------------------------------------------------------
//   ===============  TVertBasicDialogsDockDialog ===================
// ------------------------------------------------------------------------

TVertBasicDialogsDockDialog::TVertBasicDialogsDockDialog(TBasicDialog *proto, short dock_weight_in_dbu, short show_hide_event_ctrl_id,
									bool need_first_splitter, int splitter_height, bool bottom_align, DWORD *splitter_frgr_color)
				: m_dock(1000, 2000, show_hide_event_ctrl_id, need_first_splitter, 0, splitter_height, 0, bottom_align, 0, 0, splitter_frgr_color)
{
	if (proto != NULL)
		SetDialogParams(proto);

	// Prepare the dialog template in the data field of this class.
	WORD *buff_ptr = (WORD*)m_dialog_template;
	PrepareBasicDialogTemplate(buff_ptr, (WS_CHILD | WS_VISIBLE), 0, dock_weight_in_dbu, dock_weight_in_dbu, FALSE);

	// Assign the address of template to the data field of the base class.
	m_inMemoryDlgTemplate = (DLGTEMPLATE*)m_dialog_template;
}

LRESULT TVertBasicDialogsDockDialog::OnInitDialog(HWND hDefFocusControl)
{
	// Tell the splitter to use the whole space of the curr dialog.
	m_dock.SetOwner(m_hDlg);
	RECT clientRect;
	::GetClientRect(m_hDlg, &clientRect);
	m_dock.SetupReferenceRect(m_hDlg, FALSE, &clientRect);
	return(TRUE);
}

void TVertBasicDialogsDockDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	// Only the resplitting messages should come here.
	m_dock.ProcessResplitCommand(controlId, notificationCode);
}

void TVertBasicDialogsDockDialog::OnWmSize()
{
	RECT clientRect;
	::GetClientRect(m_hDlg, &clientRect);
	m_dock.PositionToRect(&clientRect);
	::InvalidateRect(m_hDlg, NULL, TRUE);
}


