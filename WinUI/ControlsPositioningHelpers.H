//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	WinUi_ControlsPositioningHelpers_H
#define	WinUi_ControlsPositioningHelpers_H

#ifndef   Common_Buffers_H
#include  "Common/Buffers.H"
#endif

enum TControlPosMode : WORD
{
	cpm_none,
	cpm_fix_left_top,
	cpm_fix_left_bottom,
	cpm_fix_right_top,
	cpm_fix_right_bottom,
	cpm_fix_left_side,
	cpm_fix_right_side,
	cpm_fix_top_side,
	cpm_fix_bottom_side,
	cpm_fix_all_corners,
};

struct TControlPosInfo
{
	short				m_controlId;
	TControlPosMode		m_controlPosMode;
};

class TControlPosHelper
{
public:

	bool		SetupControl(int ctrlId, TControlPosMode positioningMode);
				// This method can be used for registering new controls as well as for updating positioning mode
				// of the already registered controls. After setting up the control or updating its mode it is necessary
				// to call RecalcControlProps().

	bool		SetupControlsList(const TControlPosInfo *controlsInfo);
				// This method simply processes all controls from the passed list in a loop. List of controls ends
				// with the record that has control id equal to zero. After setting up the list or updating props
				// it is still necessary to call the RecalcAllControlsProps() method.

	bool		ReleaseControl(int ctrlId);
				// Method removes control from the list of maintained controls.

	void		ResetHelper() { m_data.SetNumItems(0); }
				// This method removes all controls from the list.

	void		RecalcControlProps(HWND hDlg, int ctrlId, RECT &current_ref_rect);
	void		RecalcAllControlsProps(HWND hDlg, RECT &current_ref_rect);
				// These methods should be called after shifting/resizing controls from inside the application.

	void		PositionToRect(HWND hDlg, RECT &new_ref_rect);
				// This method shifts/resizes al its copntrols to aling to the new rect.

	int		NumControls() const { return(m_data.NumItems()); }
				// This method is primarily needed to detect if the helper is empty or not.

protected:

	//
	// This helper stores offsets from the sides of the control to the latest reference rect.
	// Note that offsets can be positive and negative.
	//
	struct TControlPosProps
	{
		short				m_ctrlId;
		TControlPosMode		m_ctrlMode;

		long					m_leftOffs;
		long					m_topOffs;
		long					m_rightOffs;
		long					m_bottomOffs;
	};

	typedef  TStructsArray<TControlPosProps, 16, 16>  TControlPosPropsArray;

	int		GetControlIndex(int ctrlId);
	void		RecalcControlPropsInternal(int iCtrl, HWND hDlg, RECT &current_ref_rect);
	void		GetNewControlRect(HWND hCtrl, TControlPosProps &props, RECT &new_reference_rect, RECT &new_control_rect);

	TControlPosPropsArray		m_data;
};

//
//  Spring based helper has the following structure of items that it is maintaining:
//
//		|														|
//		|.<delim_1>.<obj_body_1>....<delim_k>.<obj_body_k>.<trailer>.	|
//		|														|
//
//  Trailer object is present all the time. Although it can have zero widtht.
//
//  Delim/Object/Trailer objects are very similar. They can either have fixed size or they can be stretcheable
//  with possibly minimal size.
//
//  State of the helper fall into one of the following situations:
//
//    avail_width < total_min_width	=>	Push some of the objects out on one of the sides of the reference rect
//									depending on the value of the "bottom_align" property.
//
//    avail_width > total_min_width && m_cnt_springs == 0
//								=>	Make either the delim of the first object or the trailer object a forced
//									spring and align all objects according to the "bottom_align" prop.
//
//    avail_width > total_min_width && m_cnt_springs > 0
//								=>	This is regular situation. Distribute available space between the springs.
//

//
// This enum defines how spring based helper maintains pos/size of the control in the secondary direction.
//

enum TControlPosSecDirMode
{
	cpsm_none,
	cpsm_align_left,
	cpsm_align_center,
	cpsm_align_right,
	cpsm_stretch_size,
};

class TSpringBasedControlPosHelper
{
public:

	TSpringBasedControlPosHelper(bool fixed_trailer, long min_trailer_width, long curr_trailer_width,
								bool bottom_align, long sec_dir_lo_margin = 0, long sec_dir_hi_margin = 0);

	int		NumControls() const { return(m_data.NumItems()); }

	long		GetFreeDistributableSpace();
				// Free space value can be negative when referencee area is not wide enough to keep all registeed objects.

	void		SetupReferenceRect(HWND hDlg, bool refresh_resplit_weights, RECT *new_reference_rect = NULL);
				// Reference rect should be set up before setting up the controls.

	bool		SetupControl(HWND hDlg, int ctrlId,
						bool fixed_front_delim_width, long min_front_delim_width, long curr_front_delim_width,
						bool fixed_control_width, long min_control_width, long curr_control_width,
						TControlPosSecDirMode sec_dir_mode, int insert_before_item = -1, bool reposition_controls_now = FALSE);
				//
				//  Min or current width of the front delim or min or current width of the control itself can be specified as -1.
				//  In this case code will pick up width from the current position/size of the controls.
				//

	bool		ReleaseControl(HWND hDlg, int ctrlId, bool reposition_controls_now = FALSE);

	void		ResetHelper();

	void		PositionToRect(HWND hDlg, RECT *new_reference_rect = NULL);
				// Method shifts/resizes registerd controls to cover the passed rect.

	bool		GetShiftingLimits(HWND hDlg, int ctrlId, long &lo_margin, long &hi_margin);
				// Margins are always non negative.

	bool		AdjustControlPosition(HWND hDlg, int ctrlId, short left_side_shift_value, short right_side_shift_value);
				// Return value is FALSE when control with the specified Id is not present or when it cannot be moved.
				// Shift values are expressed as short because split helper reports shift value as short. Defining params
				// as short makes typecasting more simple.
protected:

	int			GetControlIndex(int ctrlId);
					// If requested control is not found, method returns the number of controls in the m_data.

	void			SetupReferenceRectInternal(RECT *new_ref_rect);

	long			GetResplitWeight(int object_index) { return((object_index >= 0 && object_index < m_data.NumItems()) ? m_data[object_index].m_body.m_resplit_weight/10 : 0); }
					// Picking up the resplit weight is necessary for the dialogs dock only. This is method is not used locally.

	bool			GetControlRect(HWND hDlg, short ctrl_id, RECT &rect);

	virtual void	GetPrimaryPos(RECT &item_rect, long &item_beg, long &item_end) = 0;
	virtual void	GetSecondaryPos(RECT &item_rect, long &item_beg, long &item_end) = 0;
	virtual void	SetPrimaryPos(RECT &item_rect, long item_beg, long item_end) = 0;
	virtual void	SetSecondaryPos(RECT &item_rect, long item_beg, long item_end) = 0;

protected:

	struct TSpringObject
	{
		bool				m_fixed_size;			// This flag shows if width of the object should ever change.
		bool				m_temp_fixed;			// This flag converts object into temp fixed width object. This is needed to satisfy formulas of  "zakon Guka".

		long				m_min_width;			// Min width for the expandable objects. For fixed size objects this field stores the width.

		long				m_resplit_weight;		// In many cases this is the same to the curr width. This value is not changing when the reference rect
												// rect is changing. It does change when resplit happens.
		long				m_assigned_width;

		void		SetResplitWeight(long value)	{ long w = __max(m_min_width, value); m_resplit_weight = (w != 0) ? 10*w : 1; }
		long		GetWidth(bool use_min_width)	{ return((use_min_width == TRUE) ? m_min_width : m_assigned_width); }
	};

	struct TSpringControlPosInfo
	{
		short					m_ctrl_id;
		TSpringObject				m_front_delim;
		TSpringObject				m_body;

		TControlPosSecDirMode		m_sec_dir_mode;
		long						m_sec_dir_body_len;
	};

	typedef  TStructsArray<TSpringControlPosInfo, 32, 32>  TSpringControlPosInfosArray;

	struct THelperSpaceInfo
	{
		long			m_fixed_objects_len;		// Total length of all fixed size objects.
		long			m_min_springs_len;			// Total min length of all springs.

		long			m_springs_weight;			// Total weight of all springs.
		long			m_springs_count;			// Number of springs.

		long			m_space_to_assign;			// Space, that should be assigned to the springs.
		long			m_assigned_space;			// Space, that is already assigned to the springs.

		long		GetMinWidth() { return(m_fixed_objects_len+m_min_springs_len); }
	};

private:

	static bool	LeftCondition(long iBeg, long rBeg, THelperSpaceInfo &info) { return(iBeg == rBeg+info.GetMinWidth() || info.m_springs_count > 0 && iBeg >= rBeg+info.GetMinWidth()); }
	static bool	FixedWidthCondition(TSpringControlPosInfo &slot, long iBeg, long iEnd) { return(slot.m_body.m_fixed_size == FALSE || iEnd-iBeg == slot.m_body.m_min_width); }
	static bool	VariableWidthCondition(TSpringControlPosInfo &slot, long iBeg, long iEnd) { return(slot.m_body.m_fixed_size == TRUE || iEnd-iBeg >= slot.m_body.m_min_width); }
	static bool	RightCondition(long iEnd, long rEnd, THelperSpaceInfo &info) { return(iEnd == rEnd-info.GetMinWidth() || info.m_springs_count > 0 && iEnd <= rEnd-info.GetMinWidth()); }

protected:

	static void	AddSpring(TSpringObject &spring, THelperSpaceInfo &info);
	static void	RemoveSpring(TSpringObject &spring, THelperSpaceInfo &info);
	void			PrepareSpaceInfoInternal(HWND hDlg, RECT &ref_rect, int inx_from, int inx_after_last, THelperSpaceInfo &info, TSpringObject &trailer_space_info, bool refresh_resplit_weights);
	void			PrepareLeftRightSpaceInfo(HWND hDlg, int ctrl_inx, THelperSpaceInfo &left_info, THelperSpaceInfo &right_info);

	static void	AssignSpringLen(TSpringObject &spring, THelperSpaceInfo &info);
	static void	AssignSpringExtra(TSpringObject &spring, long &extra_space_available);
	void			DistributeSpaceInternal(int inx_from, int inx_after_last, THelperSpaceInfo &info, TSpringObject &trailer_space_info);

	void			PositionControlsTopAlign(HWND hDlg, RECT &ref_rect, bool use_min_width, int inx_from, int inx_after_last);
	void			PositionControlsBottomAlign(HWND hDlg, RECT &ref_rect, bool use_min_width, int inx_from, int inx_after_last, TSpringObject &trailer_space_info);
	void			DistributeSpaceAndPositionControls(HWND hDlg, RECT &ref_rect, int inx_from, int inx_after_last, THelperSpaceInfo &info, TSpringObject &trailer_space_info);
	void			CalcSecondaryDirPos(int inx, RECT &control_rect, long secondary_dir_area_beg, long secondary_dir_area_end);

protected:

	TSpringObject					m_trailer_space;
	long							m_initial_trailer_weight;
	bool							m_bottom_align;

	long							m_sec_dir_lo_margin;
	long							m_sec_dir_hi_margin;

	TSpringControlPosInfosArray		m_data;
	THelperSpaceInfo				m_space_info;

	RECT						m_reference_rect;

	friend class TBasicDialogsDock;
};

class THorzSpringControlPosHelper : public TSpringBasedControlPosHelper
{
public:

	THorzSpringControlPosHelper(bool fixed_trailer = TRUE, long min_trailer_width = 0, long curr_trailer_width = 0,
										bool bottom_align = FALSE, long sec_dir_lo_margin = 0, long sec_dir_hi_margin = 0)
					: TSpringBasedControlPosHelper(fixed_trailer, min_trailer_width, curr_trailer_width, bottom_align, sec_dir_lo_margin, sec_dir_hi_margin) { }
protected:

	void GetPrimaryPos(RECT &item_rect, long &item_beg, long &item_end) { item_beg = item_rect.left; item_end = item_rect.right; }
	void GetSecondaryPos(RECT &item_rect, long &item_beg, long &item_end) { item_beg = item_rect.top; item_end = item_rect.bottom; }
	void SetPrimaryPos(RECT &item_rect, long item_beg, long item_end) { item_rect.left = item_beg; item_rect.right = item_end; }
	void SetSecondaryPos(RECT &item_rect, long item_beg, long item_end) { item_rect.top = item_beg; item_rect.bottom = item_end; }
};

class TVertSpringControlPosHelper : public TSpringBasedControlPosHelper
{
public:

	TVertSpringControlPosHelper(bool fixed_trailer = TRUE, long min_trailer_width = 0, long curr_trailer_width = 0,
										bool bottom_align = FALSE, long sec_dir_lo_margin = 0, long sec_dir_hi_margin = 0)
					: TSpringBasedControlPosHelper(fixed_trailer, min_trailer_width, curr_trailer_width, bottom_align, sec_dir_lo_margin, sec_dir_hi_margin) { }
protected:

	void GetPrimaryPos(RECT &item_rect, long &item_beg, long &item_end) { item_beg = item_rect.top; item_end = item_rect.bottom; }
	void GetSecondaryPos(RECT &item_rect, long &item_beg, long &item_end) { item_beg = item_rect.left; item_end = item_rect.right; }
	void SetPrimaryPos(RECT &item_rect, long item_beg, long item_end) { item_rect.top = item_beg; item_rect.bottom = item_end; }
	void SetSecondaryPos(RECT &item_rect, long item_beg, long item_end) { item_rect.left = item_beg; item_rect.right = item_end; }
};

#endif	// WinUi_ControlsPositioningHelpers_H


