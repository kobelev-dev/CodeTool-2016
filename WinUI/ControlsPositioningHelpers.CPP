//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "WinUi/ControlsPositioningHelpers.H"

//-------------------------------------------------------------------------
//  =================  TControlPosHelper  ========================
//-------------------------------------------------------------------------

bool TControlPosHelper::SetupControl(int ctrlId, TControlPosMode positioningMode)
{
	int iCtrl = GetControlIndex(ctrlId);
	if (iCtrl < 0)
	{
		// Append the new control.
		TControlPosProps newProps = { ctrlId, positioningMode };
		if (m_data.AppendItem(newProps) == FALSE)
			return(FALSE);

		iCtrl = m_data.NumItems()-1;
	}

	// Fill in the new or already existing cell.
	TControlPosProps &props = m_data[iCtrl];
	props.m_ctrlId = ctrlId;
	props.m_ctrlMode = positioningMode;
	props.m_leftOffs = props.m_topOffs = 0;
	props.m_rightOffs = props.m_bottomOffs = 0;

	// Full success.
	return(TRUE);
}

bool TControlPosHelper::SetupControlsList(const TControlPosInfo *controlsInfo)
{
	bool overallRes = TRUE;
	while (controlsInfo->m_controlId != 0)
	{
		if (SetupControl(controlsInfo->m_controlId, controlsInfo->m_controlPosMode) == FALSE)
			overallRes = FALSE;
		controlsInfo++;
	}
	return(overallRes);
}

bool TControlPosHelper::ReleaseControl(int ctrlId)
{
	int iCtrl = GetControlIndex(ctrlId);
	if (iCtrl < 0)
		return(FALSE);

	m_data.ReleaseItem(iCtrl);
	return(TRUE);
}

void TControlPosHelper::RecalcControlProps(HWND hDlg, int ctrlId, RECT &current_ref_rect)
{
	int iCtrl = GetControlIndex(ctrlId);
	if (iCtrl < 0)
		return;

	RecalcControlPropsInternal(iCtrl, hDlg, current_ref_rect);
}

void TControlPosHelper::RecalcAllControlsProps(HWND hDlg, RECT &current_ref_rect)
{
	for (int iCtrl=0; iCtrl<m_data.NumItems(); ++iCtrl)
	{
		RecalcControlPropsInternal(iCtrl, hDlg, current_ref_rect);
	}
}

void TControlPosHelper::PositionToRect(HWND hDlg, RECT &new_ref_rect)
{
	if (hDlg == NULL || ::IsIconic(hDlg) == TRUE)
		return;

	for (int iCtrl=0; iCtrl<m_data.NumItems(); ++iCtrl)
	{
		HWND hCtrl = ::GetDlgItem(hDlg, m_data[iCtrl].m_ctrlId);
		if (hCtrl == NULL)
			continue;

		RECT rect;
		GetNewControlRect(hCtrl, m_data[iCtrl], new_ref_rect, rect);
		::SetWindowPos(hCtrl, NULL, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, SWP_NOZORDER);
	}
}

int TControlPosHelper::GetControlIndex(int ctrlId)
{
	// Plain iterate all registered controls.
	for (int iCtrl=0; iCtrl<m_data.NumItems(); ++iCtrl)
	{
		if (m_data[iCtrl].m_ctrlId == ctrlId)
			return(iCtrl);
	}

	// Control with the passed ID is not registered.
	return(-1);
}

void TControlPosHelper::RecalcControlPropsInternal(int iCtrl, HWND hDlg, RECT &current_ref_rect)
{
	if (hDlg == NULL)
		return;

	assert(iCtrl >= 0 && iCtrl < m_data.NumItems());
	TControlPosProps &props = m_data[iCtrl];
	HWND hCtrl = ::GetDlgItem(hDlg, props.m_ctrlId);
	if (hCtrl == NULL)
		return;

	// Control is existing. Pick up its rect.
	RECT ctrlRect;
	::GetWindowRect(hCtrl, &ctrlRect);
	::ScreenToClient(hDlg, (POINT*)(&ctrlRect.left));
	::ScreenToClient(hDlg, (POINT*)(&ctrlRect.right));

	// Recalc the side offsets.
	props.m_leftOffs = ctrlRect.left - current_ref_rect.left;
	props.m_topOffs = ctrlRect.top  - current_ref_rect.top;
	props.m_rightOffs = current_ref_rect.right  - ctrlRect.right;
	props.m_bottomOffs = current_ref_rect.bottom - ctrlRect.bottom;
}

void TControlPosHelper::GetNewControlRect(HWND hCtrl, TControlPosProps &props, RECT &new_ref_rect, RECT &new_control_rect)
{
	// Pick up the current state of the control.
	RECT ctrlRect;
	::GetWindowRect(hCtrl, &ctrlRect);
	long curr_size_x = ctrlRect.right-ctrlRect.left;
	long curr_size_y = ctrlRect.bottom-ctrlRect.top;

	// Calc the new position of the control and its size.
	long new_pos_x,  new_pos_y, new_size_x, new_size_y;
	switch (props.m_ctrlMode)
	{
		case cpm_fix_left_top:
				new_pos_x = new_ref_rect.left + props.m_leftOffs;
				new_pos_y = new_ref_rect.top + props.m_topOffs;
				new_size_x = curr_size_x;
				new_size_y = curr_size_y;
				break;

		case cpm_fix_left_bottom:
				new_pos_x = new_ref_rect.left + props.m_leftOffs;
				new_pos_y = new_ref_rect.bottom - props.m_bottomOffs - curr_size_y;
				new_size_x = curr_size_x;
				new_size_y = curr_size_y;
				break;

		case cpm_fix_right_top:
				new_pos_x = new_ref_rect.right - props.m_rightOffs - curr_size_x;
				new_pos_y = new_ref_rect.top + props.m_topOffs;
				new_size_x = curr_size_x;
				new_size_y = curr_size_y;
				break;

		case cpm_fix_right_bottom:
				new_pos_x = new_ref_rect.right - props.m_rightOffs - curr_size_x;
				new_pos_y = new_ref_rect.bottom - props.m_bottomOffs - curr_size_y;
				new_size_x = curr_size_x;
				new_size_y = curr_size_y;
				break;

		case cpm_fix_left_side:
				new_pos_x = new_ref_rect.left + props.m_leftOffs;
				new_pos_y = new_ref_rect.top + props.m_topOffs;
				new_size_x =  curr_size_x;
				new_size_y = (new_ref_rect.bottom - props.m_bottomOffs) - (new_ref_rect.top + props.m_topOffs);
				break;

		case cpm_fix_right_side:
				new_pos_x = new_ref_rect.right - props.m_rightOffs - curr_size_x;
				new_pos_y = new_ref_rect.top + props.m_topOffs;
				new_size_x =  curr_size_x;
				new_size_y = (new_ref_rect.bottom - props.m_bottomOffs) - (new_ref_rect.top + props.m_topOffs);
				break;

		case cpm_fix_top_side:
				new_pos_x = new_ref_rect.left + props.m_leftOffs;
				new_pos_y = new_ref_rect.top + props.m_topOffs;
				new_size_x = (new_ref_rect.right - props.m_rightOffs) - (new_ref_rect.left + props.m_leftOffs);
				new_size_y =  curr_size_y;
				break;

		case cpm_fix_bottom_side:
				new_pos_x = new_ref_rect.left + props.m_leftOffs;
				new_pos_y = new_ref_rect.bottom - props.m_bottomOffs - curr_size_y;
				new_size_x = (new_ref_rect.right - props.m_rightOffs) - (new_ref_rect.left + props.m_leftOffs);
				new_size_y =  curr_size_y;
				break;

		case cpm_fix_all_corners:
				new_pos_x = new_ref_rect.left + props.m_leftOffs;
				new_pos_y = new_ref_rect.top + props.m_topOffs;
				new_size_x = (new_ref_rect.right - props.m_rightOffs) - (new_ref_rect.left + props.m_leftOffs);
				new_size_y = (new_ref_rect.bottom - props.m_bottomOffs) - (new_ref_rect.top + props.m_topOffs);
				break;

		default:
			// The positioning mode is corrupted.
			new_pos_x = new_pos_y = 0;
			new_size_x = new_size_y = 4;
			break;
	}

	// Store the new pos and the new size into the rect.
	new_control_rect.left = new_pos_x;
	new_control_rect.top  = new_pos_y;
	new_control_rect.right  = new_pos_x+new_size_x;
	new_control_rect.bottom = new_pos_y+new_size_y;
}

//--------------------------------------------------------------------------
//  ==============  TSpringBasedControlPosHelper  ====================
//--------------------------------------------------------------------------

TSpringBasedControlPosHelper::TSpringBasedControlPosHelper(bool fixed_trailer, long min_trailer_width, long curr_trailer_width,
														bool bottom_align, long secondary_dir_lo_marg, long secondary_dir_hi_marg)
{
	if (min_trailer_width < 0)
		min_trailer_width = 0;
	if (curr_trailer_width < 0)
		curr_trailer_width = 0;

	memset(&m_trailer_space, 0, sizeof(m_trailer_space));
	m_trailer_space.m_fixed_size = fixed_trailer;
	m_trailer_space.m_min_width  = min_trailer_width;
	m_trailer_space.SetResplitWeight(curr_trailer_width);
	m_initial_trailer_weight = curr_trailer_width;

	m_bottom_align = bottom_align;
	m_sec_dir_lo_margin = secondary_dir_lo_marg;
	m_sec_dir_hi_margin = secondary_dir_hi_marg;

	memset(&m_space_info, 0, sizeof(m_space_info));
	AddSpring(m_trailer_space, m_space_info);
	memset(&m_reference_rect, 0, sizeof(m_reference_rect));
}

long TSpringBasedControlPosHelper::GetFreeDistributableSpace()
{
	long ref_beg, ref_end;
	GetPrimaryPos(m_reference_rect, ref_beg, ref_end);
	long avail_space = (ref_end-ref_beg)-m_space_info.GetMinWidth();
	return(__max(avail_space, 0));
}

void TSpringBasedControlPosHelper::SetupReferenceRect(HWND hDlg, bool refresh_resplit_weights, RECT *new_reference_rect)
{
	if (new_reference_rect != NULL)
		SetupReferenceRectInternal(new_reference_rect);

	if (refresh_resplit_weights == TRUE)
		PrepareSpaceInfoInternal(hDlg, m_reference_rect, 0, m_data.NumItems(), m_space_info, m_trailer_space, TRUE);
}

bool TSpringBasedControlPosHelper::SetupControl(HWND hDlg, int ctrlId,
											bool fixed_front_delim_width, long min_front_delim_width, long curr_front_delim_width,
											bool fixed_control_width, long min_control_width, long curr_control_width,
											TControlPosSecDirMode secd_mode, int insert_before_item, bool reposition_controls_now)
{
	assert(secd_mode >= cpsm_align_left && secd_mode <= cpsm_stretch_size);

	// Check that control really exists.
	RECT control_rect;
	if (hDlg == NULL || GetControlRect(hDlg, ctrlId, control_rect) == FALSE)
		return(FALSE);

	if (insert_before_item < 0 || insert_before_item > m_data.NumItems())
	{
		// Index of the item to insert in front is bogus. Insert to the end of the list.
		insert_before_item = m_data.NumItems();
	}

	// Remove the old descriptor if any.
	int obj_inx = GetControlIndex(ctrlId);
	if (obj_inx != m_data.NumItems())
	{
		RemoveSpring(m_data[obj_inx].m_front_delim, m_space_info);
		RemoveSpring(m_data[obj_inx].m_body, m_space_info);
		m_data.ReleaseItem(obj_inx);
		if (obj_inx < insert_before_item)
			insert_before_item--;
	}

	long item_beg, item_end;
	GetPrimaryPos(control_rect, item_beg, item_end);
	long item_secondary_beg, item_secondary_end;
	GetSecondaryPos(control_rect, item_secondary_beg, item_secondary_end);

	if (min_front_delim_width < 0 || curr_front_delim_width < 0)
	{
		RECT prev_item_rect;
		long prev_item_beg, prev_item_end;
		if (insert_before_item == 0 || GetControlRect(hDlg, m_data[insert_before_item-1].m_ctrl_id, prev_item_rect) == FALSE)
		{
			// There is no object in front of the control that is being inserted. Use the ref rect.
			GetPrimaryPos(m_reference_rect, prev_item_beg, prev_item_end);
		}
		else
		{
			// Pick up the position of the object in front.
			GetPrimaryPos(prev_item_rect, prev_item_beg, prev_item_end);
		}

		if (min_front_delim_width < 0)
		{
			// Pick up the current value of the delimiter.
			min_front_delim_width = item_beg-prev_item_end;
			if (min_front_delim_width < 0)
				min_front_delim_width = 0;
		}

		if (curr_front_delim_width < 0)
		{
			// Pick up the current value of the delimiter.
			curr_front_delim_width = item_beg-prev_item_end;
			if (curr_front_delim_width < 0)
				curr_front_delim_width = 0;
		}
	}

	if (min_control_width < 0)
	{
		// Use the current width of the control.
		min_control_width = item_end-item_beg;
		if (min_control_width < 0)
			min_control_width = 0;
	}

	if (curr_control_width < 0)
	{
		// Pick up the current width of the control.
		curr_control_width = item_end-item_beg;
		if (curr_control_width < 0)
			curr_control_width = 0;
	}

	// Prepare a new slot for this object.
	TSpringControlPosInfo props;
	memset(&props, 0, sizeof(props));
	props.m_ctrl_id = ctrlId;
	props.m_front_delim.m_fixed_size = fixed_front_delim_width;
	props.m_front_delim.m_min_width  = min_front_delim_width;
	props.m_front_delim.SetResplitWeight(curr_front_delim_width);
	props.m_body.m_fixed_size = fixed_control_width;
	props.m_body.m_min_width  = min_control_width;
	props.m_body.SetResplitWeight(curr_control_width);
	props.m_sec_dir_mode = secd_mode;
	props.m_sec_dir_body_len = item_secondary_end-item_secondary_beg;

	if (m_data.InsertItem(insert_before_item, props) == FALSE)
		return(FALSE);

	// Finalize the setup.
	AddSpring(props.m_front_delim, m_space_info);
	AddSpring(props.m_body, m_space_info);

	// Show the results if requested.
	if (reposition_controls_now == TRUE)
	{
		PositionToRect(hDlg);
		::InvalidateRect(hDlg, &m_reference_rect, TRUE);
	}

	// Full success.
	return(TRUE);
}

bool TSpringBasedControlPosHelper::ReleaseControl(HWND hDlg, int ctrlId, bool reposition_controls_now)
{
	int iCtrl = GetControlIndex(ctrlId);
	if (iCtrl == m_data.NumItems())
		return(FALSE);

	RemoveSpring(m_data[iCtrl].m_front_delim, m_space_info);
	RemoveSpring(m_data[iCtrl].m_body, m_space_info);
	m_data.ReleaseItem(iCtrl);

	if (reposition_controls_now == TRUE)
	{
		PositionToRect(hDlg);
		::InvalidateRect(hDlg, &m_reference_rect, TRUE);
	}

	// Full success.
	return(TRUE);
}

void TSpringBasedControlPosHelper::ResetHelper()
{
	// Reset the current state.
	m_trailer_space.SetResplitWeight(m_initial_trailer_weight);
	m_data.SetNumItems(0);
	memset(&m_space_info, 0, sizeof(m_space_info));
	AddSpring(m_trailer_space, m_space_info);
	memset(&m_reference_rect, 0, sizeof(m_reference_rect));
}

void TSpringBasedControlPosHelper::PositionToRect(HWND hDlg, RECT *new_reference_rect)
{
	if (new_reference_rect != NULL)
		SetupReferenceRectInternal(new_reference_rect);

	if (m_data.NumItems() == 0)
	{
		// The helper is empty. There is nothing to position.
		return;
	}

	// Call the worker method.
	DistributeSpaceAndPositionControls(hDlg, m_reference_rect, 0, m_data.NumItems(), m_space_info, m_trailer_space);
}

bool TSpringBasedControlPosHelper::GetShiftingLimits(HWND hDlg, int ctrlId, long &lo_margin, long &hi_margin)
{
	int  ctrl_inx = GetControlIndex(ctrlId);
	if (ctrl_inx == m_data.NumItems())
	{
		// Control with this id is not known or it has somehow disappeared.
		lo_margin = hi_margin = 0;
		return(FALSE);
	}

	THelperSpaceInfo left_info, right_info;
	PrepareLeftRightSpaceInfo(hDlg, ctrl_inx, left_info, right_info);
	lo_margin = left_info.GetMinWidth();
	hi_margin = right_info.GetMinWidth();
	return(TRUE);
}

bool TSpringBasedControlPosHelper::AdjustControlPosition(HWND hDlg, int ctrlId, short left_shift_value, short right_shift_value)
{
	int ctrl_inx = GetControlIndex(ctrlId);
	if (ctrl_inx == m_data.NumItems())
		return(FALSE);

	RECT ctrl_rect;
	if (GetControlRect(hDlg, ctrlId, ctrl_rect) == FALSE)
		return(FALSE);

	long ref_beg, ref_end;
	GetPrimaryPos(m_reference_rect, ref_beg, ref_end);
	if (m_space_info.m_springs_count == 0 || ref_end-ref_beg <= m_space_info.GetMinWidth())
	{
		// There are no springs or there is no space to distribute. The position cannot be adjusted.
		return(TRUE);
	}

	//
	//  The springs are available and the space is available.
	//

	long item_beg, item_end;
	GetPrimaryPos(ctrl_rect, item_beg, item_end);
	item_beg += left_shift_value;
	item_end += right_shift_value;

	if (item_beg < ref_beg)
		item_beg = ref_beg;
	if (item_end < ref_beg)
		item_end = ref_beg;

	if (item_beg > ref_end)
		item_beg = ref_end;
	if (item_end > ref_end)
		item_end = ref_end;

	assert(item_beg <= item_end);

	TSpringControlPosInfo &slot = m_data[ctrl_inx];
	THelperSpaceInfo left_info, right_info;
	PrepareLeftRightSpaceInfo(hDlg, ctrl_inx, left_info, right_info);

	// Check the restrictions.
	if (LeftCondition(item_beg, ref_beg, left_info) == FALSE)
	{
		// The left restriction is voilated. Fix it by shifting the whole object.
		long ctrl_size = item_end-item_beg;
		item_beg = ref_beg+left_info.GetMinWidth();
		item_end = item_beg+ctrl_size;

		if (slot.m_body.m_fixed_size == TRUE)
		{
			if (item_end-item_beg != slot.m_body.m_min_width)
			{
				if (right_info.m_springs_count == 0)
				{
					// Align object to the right side.
					item_end = ref_end-right_info.GetMinWidth();
					item_beg = item_end-slot.m_body.m_min_width;
				}
				else
				{
					// Fix the right size of the object.
					item_end = item_beg+slot.m_body.m_min_width;
				}
			}
			else if (RightCondition(item_end, ref_end, right_info) == FALSE)
			{
				// Align object to the right side.
				item_end = ref_end-right_info.GetMinWidth();
				item_beg = item_end-slot.m_body.m_min_width;
			}
		}
		else
		{
			if (item_end-item_beg < slot.m_body.m_min_width)
			{
				if (right_info.m_springs_count == 0)
				{
					// Align object to the right side.
					item_end = ref_end-right_info.GetMinWidth();
					item_beg = item_end-slot.m_body.m_min_width;
				}
				else
				{
					// Fix the right size of the object.
					item_end = item_beg+slot.m_body.m_min_width;
				}
			}
			else if (RightCondition(item_end, ref_end, right_info) == FALSE)
			{
				// Right restriction is voilated. Fix it by shifting only the right side.
				item_end = ref_end-right_info.GetMinWidth();
			}
		}
	}
	else if (FixedWidthCondition(slot, item_beg, item_end) == FALSE)
	{
		// Body width violation for control with the fixed size.
		if (left_info.m_springs_count == 0)
		{
			// Align object to the left side.
			item_beg = ref_beg+left_info.GetMinWidth();
			item_end = item_beg+slot.m_body.m_min_width;
		}
		else  if (right_info.m_springs_count == 0)
		{
			// Align object to the right side.
			item_end = ref_end-right_info.GetMinWidth();
			item_beg = item_end-slot.m_body.m_min_width;
		}
		else
		{
			// Fix the size by preserving the center of the control.
			item_beg = (item_end+item_beg-slot.m_body.m_min_width)/2;
			item_end = item_beg+slot.m_body.m_min_width;

			if (LeftCondition(item_beg, ref_beg, left_info) == FALSE)
			{
				// Align object to the left side.
				item_beg = ref_beg+left_info.GetMinWidth();
				item_end = item_beg+slot.m_body.m_min_width;
			}
			else if (RightCondition(item_end, ref_end, right_info) == FALSE)
			{
				// Align object to the right side.
				item_end = ref_end-right_info.GetMinWidth();
				item_beg = item_end-slot.m_body.m_min_width;
			}
		}
	}
	else if (VariableWidthCondition(slot, item_beg, item_end) == FALSE)
	{
		// Body width violation for control with the variable size.
		if (left_info.m_springs_count == 0 && right_info.m_springs_count == 0)
		{
			// This is rare case when both sides of the object should be aligned.
			item_beg = ref_beg+left_info.GetMinWidth();
			item_end = ref_end-right_info.GetMinWidth();
		}
		else
		{
			// Fix the size by preserving the center of the control.
			item_beg = (item_end+item_beg-slot.m_body.m_min_width)/2;
			item_end = item_beg+slot.m_body.m_min_width;

			if (LeftCondition(item_beg, ref_beg, left_info) == FALSE)
			{
				// Align object to the left side.
				item_beg = ref_beg+left_info.GetMinWidth();
				item_end = item_beg+slot.m_body.m_min_width;
			}
			else if (RightCondition(item_end, ref_end, right_info) == FALSE)
			{
				// Align object to the right side.
				item_end = ref_end-right_info.GetMinWidth();
				item_beg = item_end-slot.m_body.m_min_width;
			}
		}
	}
	else if (RightCondition(item_end, ref_end, right_info) == FALSE)
	{
		// Only the right restriction is voilated. Fix it by shifting the whole object.
		long ctrl_size = item_end-item_beg;
		item_end = ref_end-right_info.GetMinWidth();
		item_beg = item_end-ctrl_size;
	}

	// Ensure once again that all conditions are satisfied.
	assert(LeftCondition(item_beg, ref_beg, left_info) == TRUE);
	assert(FixedWidthCondition(slot, item_beg, item_end) == TRUE);
	assert(VariableWidthCondition(slot, item_beg, item_end) == TRUE);
	assert(RightCondition(item_end, ref_end, right_info) == TRUE);

	//
	// New position of the object is finally determined.
	//

	RECT left_rect = m_reference_rect;
	SetPrimaryPos(left_rect, ref_beg, item_beg);
	DistributeSpaceAndPositionControls(hDlg, left_rect, 0, ctrl_inx, left_info, slot.m_front_delim);

	RECT right_rect = m_reference_rect;
	SetPrimaryPos(right_rect, item_end, ref_end);
	DistributeSpaceAndPositionControls(hDlg, right_rect, ctrl_inx+1, m_data.NumItems(), right_info, m_trailer_space);

	RECT new_ctrl_rect = m_reference_rect;
	SetPrimaryPos(new_ctrl_rect, item_beg, item_end);
	long ref_secondary_beg, ref_secondary_end;
	GetSecondaryPos(new_ctrl_rect, ref_secondary_beg, ref_secondary_end);
	CalcSecondaryDirPos(ctrl_inx, new_ctrl_rect, ref_secondary_beg, ref_secondary_end);

	HWND hCtrl = ::GetDlgItem(hDlg, slot.m_ctrl_id);
	if (hCtrl != NULL)
		::SetWindowPos(hCtrl, NULL, new_ctrl_rect.left, new_ctrl_rect.top, new_ctrl_rect.right-new_ctrl_rect.left, new_ctrl_rect.bottom-new_ctrl_rect.top, SWP_NOZORDER);

	// Adjust the resplit weights in all controls.
	PrepareSpaceInfoInternal(hDlg, m_reference_rect, 0, m_data.NumItems(), m_space_info, m_trailer_space, TRUE);
	return(TRUE);
}

int TSpringBasedControlPosHelper::GetControlIndex(int ctrlId)
{
	for (int iCtrl=0; iCtrl<m_data.NumItems(); ++iCtrl)
	{
		if (m_data[iCtrl].m_ctrl_id == ctrlId)
			return(iCtrl);
	}
	return(m_data.NumItems());
}

void TSpringBasedControlPosHelper::SetupReferenceRectInternal(RECT *new_ref_rect)
{
	if (new_ref_rect != NULL && new_ref_rect != &m_reference_rect)
	{
		m_reference_rect = *new_ref_rect;
		if (m_bottom_align == FALSE)
		{
			if (m_reference_rect.right < m_reference_rect.left)
				m_reference_rect.right = m_reference_rect.left;
			if (m_reference_rect.bottom < m_reference_rect.top)
				m_reference_rect.bottom = m_reference_rect.top;
		}
		else
		{
			if (m_reference_rect.right < m_reference_rect.left)
				m_reference_rect.left = m_reference_rect.right;
			if (m_reference_rect.bottom < m_reference_rect.top)
				m_reference_rect.top = m_reference_rect.bottom;
		}
	}
}

bool TSpringBasedControlPosHelper::GetControlRect(HWND hDlg, short ctrl_id, RECT &rect)
{
	HWND hCtrl = ::GetDlgItem(hDlg, ctrl_id);
	if (hCtrl == NULL)
		return(FALSE);

	::GetWindowRect(hCtrl, &rect);
	::ScreenToClient(hDlg, (POINT*)&(rect.left));
	::ScreenToClient(hDlg, (POINT*)&(rect.right));
	return(TRUE);
}

void TSpringBasedControlPosHelper::AddSpring(TSpringObject &spring, THelperSpaceInfo &info)
{
	assert(spring.m_min_width >= 0);

	if (spring.m_fixed_size == TRUE)
	{
		// The object has a fixed width.
		info.m_fixed_objects_len += spring.m_min_width;
	}
	else
	{
		assert(spring.m_resplit_weight >= 0);

		// The object has a variable width.
		info.m_min_springs_len += spring.m_min_width;
		info.m_springs_weight += spring.m_resplit_weight;
		info.m_springs_count++;
	}
}

void TSpringBasedControlPosHelper::RemoveSpring(TSpringObject &spring, THelperSpaceInfo &info)
{
	if (spring.m_fixed_size == TRUE)
	{
		// This is object with the fixed width.
		info.m_fixed_objects_len -= spring.m_min_width;
	}
	else
	{
		// This is object with the variable width.
		info.m_min_springs_len -= spring.m_min_width;
		info.m_springs_weight -= spring.m_resplit_weight;
		info.m_springs_count--;
	}
}

void TSpringBasedControlPosHelper::PrepareSpaceInfoInternal(HWND hDlg, RECT &ref_rect, int inx_from, int inx_after_last,
														THelperSpaceInfo &info, TSpringObject &trailer_space_info, bool refresh_resplit_weights)
{
	memset(&info, 0, sizeof(info));

	long guide_beg, guide_end;
	GetPrimaryPos(ref_rect, guide_beg, guide_end);
	long guide_secondary_beg, guide_secondary_end;
	GetSecondaryPos(ref_rect, guide_secondary_beg, guide_secondary_end);

	assert(guide_end >= guide_beg);
	assert(guide_secondary_end >= guide_secondary_beg);

	// Process the range of the passed objects.
	long prev_item_end = guide_beg;
	for (int iCtrl=inx_from; iCtrl<inx_after_last; ++iCtrl)
	{
		TSpringControlPosInfo &slot = m_data[iCtrl];
		if (refresh_resplit_weights == TRUE)
		{
			RECT rect;
			if (GetControlRect(hDlg, slot.m_ctrl_id, rect) == TRUE)
			{
				long item_beg, item_end;
				GetPrimaryPos(rect, item_beg, item_end);
				long item_secondary_beg, item_secondary_end;
				GetSecondaryPos(rect, item_secondary_beg, item_secondary_end);

				long front_delim_len = item_beg-prev_item_end;
				if (front_delim_len < 0)
					front_delim_len = 0;

				long item_body_len = item_end-item_beg;
				if (item_body_len < 0)
					item_body_len = 0;

				// Fill in the current resplit weight and secondary direction length.
				slot.m_front_delim.SetResplitWeight(front_delim_len);
				slot.m_body.SetResplitWeight(item_body_len);
				slot.m_sec_dir_body_len = item_secondary_end-item_secondary_beg;

				prev_item_end = item_end;
			}
			else
			{
				// Fill in the props with the dummy values.
				slot.m_front_delim.SetResplitWeight(0);
				slot.m_body.SetResplitWeight(0);
				slot.m_sec_dir_body_len = guide_secondary_end-guide_secondary_beg;
			}
		}

		AddSpring(slot.m_front_delim, info);
		AddSpring(slot.m_body, info);
	}

	if (refresh_resplit_weights == TRUE)
	{
		// Process the trailer.
		long trailer_len = guide_end-prev_item_end;
		if (trailer_len < 0)
			trailer_len = 0;
		m_trailer_space.SetResplitWeight(trailer_len);
	}

	AddSpring(m_trailer_space, info);
}

void TSpringBasedControlPosHelper::PrepareLeftRightSpaceInfo(HWND hDlg, int ctrl_inx, THelperSpaceInfo &left_info, THelperSpaceInfo &right_info)
{
	assert(ctrl_inx >= 0 && ctrl_inx < m_data.NumItems());

	RECT rect = m_reference_rect;
	long ref_beg, ref_end;
	GetPrimaryPos(rect, ref_beg, ref_end);

	RECT ctrl_rect;
	GetControlRect(hDlg, m_data[ctrl_inx].m_ctrl_id, ctrl_rect);
	long ctrl_beg, ctrl_end;
	GetPrimaryPos(rect, ctrl_beg, ctrl_end);

	SetPrimaryPos(rect, ref_beg, ctrl_beg);
	PrepareSpaceInfoInternal(hDlg, rect, 0, ctrl_inx, left_info, m_data[ctrl_inx].m_front_delim, FALSE);
	SetPrimaryPos(rect, ctrl_end, ref_end);
	PrepareSpaceInfoInternal(hDlg, rect, ctrl_inx+1, m_data.NumItems(), right_info, m_trailer_space, FALSE);
}

void TSpringBasedControlPosHelper::AssignSpringLen(TSpringObject &spring, THelperSpaceInfo &info)
{
	long len = 0;
	if (spring.m_fixed_size == TRUE || spring.m_temp_fixed == TRUE)
	{
		// The spring has fixed width or it is temporarily excluded from the stretching process.
		len = spring.m_min_width;
	}
	else
	{
		// This spring is expandable.
		if (info.m_springs_weight > 0)
		{
			// Assign proportional part from the space to distribute.
			len = (spring.m_resplit_weight*info.m_space_to_assign)/info.m_springs_weight;
		}
		else if (info.m_springs_count > 0)
		{
			// In normal situation total weight should be not zero. Nevertheless implement the backup solution where space
			// is distributed evenly between all available springs.
			len = info.m_space_to_assign/info.m_springs_count;
		}

		if (len < spring.m_min_width)
		{
			// This item should be temp excluded from the distribution process.
			spring.m_temp_fixed = TRUE;
			len = spring.m_min_width;

			info.m_springs_weight -= spring.m_resplit_weight;
			info.m_springs_count--;
			info.m_space_to_assign -= len;

			// This function is called only from DistributeSpaceInternal(). And DistributeSpaceInternal() itself can be called
			// only for distribution of positive amount of space between the positive number of springs. This means that all
			// springs should never collapse during the distribution. Ensure this.
			assert(info.m_springs_count > 0);
		}
		else
		{
			info.m_assigned_space += len;
		}
	}

	spring.m_assigned_width = len;
}

void	TSpringBasedControlPosHelper::AssignSpringExtra(TSpringObject &spring, long &extra_space)
{
	if (spring.m_fixed_size == FALSE && spring.m_temp_fixed == FALSE && extra_space > 0)
	{
		// The passed spring can accept extra width.
		spring.m_assigned_width++;
		extra_space--;
	}
}

void TSpringBasedControlPosHelper::DistributeSpaceInternal(int inx_from, int inx_after_last, THelperSpaceInfo &info, TSpringObject &trailer_space)
{
	assert(info.m_springs_count > 0);
	assert(info.m_space_to_assign > info.m_min_springs_len);

	// Remove all temp fixed size flags if any.
	for (int inx=inx_from; inx < inx_after_last; ++inx)
	{
		TSpringControlPosInfo &slot = m_data[inx];
		slot.m_front_delim.m_temp_fixed = FALSE;
		slot.m_body.m_temp_fixed = FALSE;
	}

	trailer_space.m_temp_fixed = FALSE;

	// Do the major assignment loop.
	for(;;)
	{
		assert(info.m_space_to_assign >= 0);
		long springs_count = info.m_springs_count;
		info.m_assigned_space = 0;

		for (int inx=inx_from; inx < inx_after_last; ++inx)
		{
			TSpringControlPosInfo &slot = m_data[inx];
			AssignSpringLen(slot.m_front_delim, info);
			AssignSpringLen(slot.m_body, info);
		}

		AssignSpringLen(trailer_space, info);

		if (springs_count == info.m_springs_count)
		{
			// There were no spring collapsing during the latest iteration.
			break;
		}
	}

	long extra_space = info.m_space_to_assign-info.m_assigned_space;
	while (extra_space > 0)
	{
		// Due to the rounding some space was not assigned.
		for (int inx=inx_from; inx < inx_after_last; ++inx)
		{
			TSpringControlPosInfo &slot = m_data[inx];
			AssignSpringExtra(slot.m_front_delim, extra_space);
			AssignSpringExtra(slot.m_body, extra_space);
		}

		AssignSpringExtra(trailer_space, extra_space);
	}
}

void TSpringBasedControlPosHelper::PositionControlsTopAlign(HWND hDlg, RECT &ref_rect, bool use_min_width, int inx_from, int inx_after_last)
{
	long guide_beg, guide_end;
	GetPrimaryPos(ref_rect, guide_beg, guide_end);
	long guide_secondary_beg, guide_secondary_end;
	GetSecondaryPos(ref_rect, guide_secondary_beg, guide_secondary_end);
	assert(guide_end >= guide_beg);

	long prev_item_end = guide_beg;
	for (int iCtrl=inx_from; iCtrl<inx_after_last; ++iCtrl)
	{
		RECT rect;
		TSpringControlPosInfo &slot = m_data[iCtrl];
		long item_beg = prev_item_end+slot.m_front_delim.GetWidth(use_min_width);
		long item_end = item_beg+slot.m_body.GetWidth(use_min_width);
		SetPrimaryPos(rect, item_beg, item_end);
		CalcSecondaryDirPos(iCtrl, rect, guide_secondary_beg, guide_secondary_end);

		HWND hCtrl = ::GetDlgItem(hDlg, slot.m_ctrl_id);
		if (hCtrl != NULL)
			::SetWindowPos(hCtrl, NULL, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, SWP_NOZORDER);

		prev_item_end = item_end;
	}
}

void TSpringBasedControlPosHelper::PositionControlsBottomAlign(HWND hDlg, RECT &ref_rect, bool use_min_width, int inx_from, int inx_after_last, TSpringObject &trailer_space_info)
{
	long guide_beg, guide_end;
	GetPrimaryPos(ref_rect, guide_beg, guide_end);
	long guide_secondary_beg, guide_secondary_end;
	GetSecondaryPos(ref_rect, guide_secondary_beg, guide_secondary_end);
	assert(guide_end >= guide_beg);

	// Iterate controls in the reversed order.
	long item_end = guide_end-trailer_space_info.GetWidth(use_min_width);
	for (int iCtrl=inx_after_last-1; iCtrl >= inx_from; --iCtrl)
	{
		RECT rect;
		TSpringControlPosInfo &slot = m_data[iCtrl];
		long item_beg = item_end-slot.m_body.GetWidth(use_min_width);
		SetPrimaryPos(rect, item_beg, item_end);
		CalcSecondaryDirPos(iCtrl, rect, guide_secondary_beg, guide_secondary_end);

		HWND hCtrl = ::GetDlgItem(hDlg, slot.m_ctrl_id);
		if (hCtrl != NULL)
			::SetWindowPos(hCtrl, NULL, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, SWP_NOZORDER);

		item_end = item_beg-slot.m_front_delim.GetWidth(use_min_width);
	}
}

void TSpringBasedControlPosHelper::DistributeSpaceAndPositionControls(HWND hDlg, RECT &ref_rect, int inx_from, int inx_after_last,
																		THelperSpaceInfo &info, TSpringObject &trailer_space_info)
{
	if (inx_from >= inx_after_last)
		return;

	bool use_min_width = TRUE;
	long ref_beg, ref_end;
	GetPrimaryPos(ref_rect, ref_beg, ref_end);
	if (info.m_springs_count > 0 && ref_end-ref_beg > info.GetMinWidth())
	{
		// The springs are availlable and there is some space to distribute between them.
		use_min_width = FALSE;
		THelperSpaceInfo loc_info = info;
		loc_info.m_space_to_assign = (ref_end-ref_beg)-info.m_fixed_objects_len;
		DistributeSpaceInternal(inx_from, inx_after_last, loc_info, trailer_space_info);
	}

	if (m_bottom_align == FALSE)
	{
		PositionControlsTopAlign(hDlg, ref_rect, use_min_width, inx_from, inx_after_last);
	}
	else
	{
		PositionControlsBottomAlign(hDlg, ref_rect, use_min_width, inx_from, inx_after_last, trailer_space_info);
	}
}

void TSpringBasedControlPosHelper::CalcSecondaryDirPos(int inx, RECT &control_rect, long secondary_dir_area_beg, long secondary_dir_area_end)
{
	long lo_offs;
	long item_beg, item_end;
	switch (m_data[inx].m_sec_dir_mode)
	{
		case cpsm_align_left:
				item_beg = secondary_dir_area_beg+m_sec_dir_lo_margin;
				item_end = item_beg + m_data[inx].m_sec_dir_body_len;
				break;

		case cpsm_align_center:
				lo_offs = ((secondary_dir_area_end-secondary_dir_area_beg)-m_data[inx].m_sec_dir_body_len)/2;
				item_beg = secondary_dir_area_beg + lo_offs;
				item_end = item_beg + m_data[inx].m_sec_dir_body_len;
				break;

		case cpsm_align_right:
				item_end = secondary_dir_area_end-m_sec_dir_hi_margin;
				item_beg = item_end - m_data[inx].m_sec_dir_body_len;
				break;

		case cpsm_stretch_size:
				item_beg = secondary_dir_area_beg+m_sec_dir_lo_margin;
				item_end = secondary_dir_area_end-m_sec_dir_hi_margin;
				m_data[inx].m_sec_dir_body_len = item_end - item_beg;
				break;

		default:
			// Assign dummy values to make the data corruption more visible.
			item_beg = 10; item_end = 20;
			break;
	}

	// Set up the data in the control rect.
	SetSecondaryPos(control_rect, item_beg, item_end);
}


