//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	WinUi_DialogsDock_H
#define	WinUi_DialogsDock_H

#ifndef   WinUi_BasicDialog_H
#include  "WinUi/BasicDialog.H"
#endif

class TBasicDialogsDock
{
public:

	TBasicDialogsDock(short controls_base, short controls_area_len, short show_hide_event_ctrl_id,
						bool need_first_splitter, int splitter_lo_delim, int splitter_width, int splitter_hi_delim,
						DWORD *splitter_frgr_color = NULL);
			// The length of the control Ids area should be twice bigger than the maximum number of dialogs in the dock.
			// Splitter foreground color pointer should be in the static memory only.

	void		SetOwner(HWND hDlg) { m_hOwnerDialog = hDlg; }
	void		ResetDock();

	void		SetupReferenceRect(HWND hDlg, bool refresh_resplit_weights, RECT *new_ref_rect = NULL);
				// The dock helper will place its objects on the whole surface of the bypassed rect without any spacing around.
				// Delimiters will be plased only between the objects.

	bool		ShowDialog(TBasicDialog *dlg, bool bypass_ownership, long preferred_dialog_width = -1, TBasicDialog *ins_before = NULL, bool variable_size_dialog = TRUE, long min_dialog_width = 0);
				// If the dialog, that is being inserted, is not currently opened, it will be opened. Position of the dialog is not important.
				// Size of the dialog in the primary direction will be used as a width split request. Size in the secondary direction is not
				// important at all. Method will reveal the window and assign an id to it. Id of this window may change over the time.

	bool		HideDialog(TBasicDialog *dlg);
				// The TBasicDialog object or its window are not destroyed. The dialog is made invisible while its splitter is destroyed.
				// Return value is FALSE only if dialog does not belong to the dock.

	void		ProcessResplitCommand(int ctrl_id, int notification_code);
				// Primary purpose of this method is to process events from splitters. Parent dialog is expected to bypass all events
				// with control ids in the range that was specified in the constructor.

	void		PositionToRect(RECT *new_reference_rect = NULL) { data().PositionToRect(m_hOwnerDialog, new_reference_rect); }

	void		BroadcastCommand(int ctrl_id, int ntf_code, LPARAM lParam = 0);
				// Method sends WM_COMMAND to all docked dialogs.

	bool		IsSplitter(int ctrl_id) { return((ctrl_id >= m_ctrl_base_splt && ctrl_id < m_ctrl_base_dlg) ? TRUE : FALSE); }

	int				NumDialogs() { return((data().NumControls()+1)/2); }
	TBasicDialog		*GetChildDialog(int inx);
	bool				GetDialogOwnedFlag(int inx) { return(m_ownership_data[inx]); }

protected:

	virtual TSpringBasedControlPosHelper &data() = 0;
					// Caller retrieves the reference to the helper to bypass it to the size adjust
					// scheme of the higher level.

	virtual bool	IsHorzDock() = 0;
	virtual bool	IsVertDock() = 0;

	inline int		AdjustHelperInxForSplitter(int i) { return(2*i - ((m_splt_need_first == TRUE) ? 0 : 1)); }
	inline int		AdjustHelperInxForDialog(int i)  { return(2*i + ((m_splt_need_first == TRUE) ? 1 : 0)); }
					// Argument is the index of the dialog in the dock. This index can have values 0..NumDialogs()-1.
					// Note that splitter adjusts helper index of the first dialog (inx == 0) to the negative value when first splitter is not needed.
					// This is ok because this splitter is not existing and this situation is checked later in the code.
					// Return value is index of control in TSingleSpringAdjustHelper, that is provided by derived class. These values should be
					// in the range 0..data().NumItems().

	int			FindDialog(TBasicDialog *dlg);
					// This method returns either zero based index of the dialog in the dock or the number of dialogs in the dock
					// if passed dialog does not belong to the dock.

	void			RenumberControls(int inx_start, bool ins_mode);
	void			RenumberOneControl(int inx_in_adj_helper, int xpct_ctrl_id, int ctrl_id_shift);
	void			ReleaseOwnedDialogs();

protected:

	HWND						m_hOwnerDialog;

	int							m_ctrl_base_splt;
	int							m_ctrl_base_dlg;

	short						m_show_hide_event;				// Event with this id is sent to the docked dialog when it is about
																// to be opened or closed.

	TStructsArray<bool, 16, 8>		m_ownership_data;				// This array has the same length to the number of dialogs in the dock.

	bool							m_splt_need_first;				// This flag indicates if the first dialog needs splitter in front of it.
																// This field is set in ctor and it is never changed later.
	int							m_splt_lo_delim;
	int							m_splt_width;
	int							m_splt_hi_delim;

	bool							m_splt_specific_frgr_color;
	DWORD						m_splt_frgr_color;
};

class THorzBasicDialogsDock : public TBasicDialogsDock
{
public:

	THorzBasicDialogsDock(short controls_base, short controls_area_len, short show_hide_event_ctrl_id,
								bool need_first_splitter, int splitter_lo_delim, int splitter_width, int splitter_hi_delim,
								bool bottom_align, long secondary_dir_lo_marg = 0, long secondary_dir_hi_marg = 0,
								DWORD *splitter_frgr_color = NULL)
				: TBasicDialogsDock(controls_base, controls_area_len, show_hide_event_ctrl_id,
								need_first_splitter, splitter_lo_delim, splitter_width, splitter_hi_delim,
								splitter_frgr_color),
				m_hlpr(TRUE, splitter_hi_delim, splitter_hi_delim, bottom_align, secondary_dir_lo_marg, secondary_dir_hi_marg) { }

	~THorzBasicDialogsDock() { ReleaseOwnedDialogs(); }

	virtual  TSpringBasedControlPosHelper &data() { return(m_hlpr); }
	virtual  bool  IsHorzDock()	{ return(TRUE); }
	virtual  bool  IsVertDock()	{ return(FALSE); }

protected:

	THorzSpringControlPosHelper	m_hlpr;
};

class TVertBasicDialogsDock : public TBasicDialogsDock
{
public:

	TVertBasicDialogsDock(short controls_base, short controls_area_len, short show_hide_event_ctrl_id,
								bool need_first_splitter, int splitter_lo_delim, int splitter_width, int splitter_hi_delim,
								bool bottom_align, long secondary_dir_lo_marg = 0, long secondary_dir_hi_marg = 0,
								DWORD *splitter_frgr_color = NULL)
				: TBasicDialogsDock(controls_base, controls_area_len, show_hide_event_ctrl_id,
								need_first_splitter, splitter_lo_delim, splitter_width, splitter_hi_delim,
								splitter_frgr_color),
				m_hlpr(TRUE, splitter_hi_delim, splitter_hi_delim, bottom_align, secondary_dir_lo_marg, secondary_dir_hi_marg) { }

	~TVertBasicDialogsDock() { ReleaseOwnedDialogs(); }

	virtual  TSpringBasedControlPosHelper &data() { return(m_hlpr); }
	virtual  bool  IsHorzDock()	{ return(FALSE); }
	virtual  bool  IsVertDock()	{ return(TRUE); }

protected:

	TVertSpringControlPosHelper		m_hlpr;
};

#define BASIC_DIALOGS_DOCK_TEMPL_BUFF_LEN (sizeof(DLGTEMPLATE)+20)

// This dialog does not need any dialog template in resources.
class THorzBasicDialogsDockDialog : public TBasicDialog
{
public:

	THorzBasicDialogsDockDialog(TBasicDialog *proto, short dock_weight_in_dbu, short show_hide_event_ctrl_id,
						bool need_first_splitter = FALSE, int splitter_width = 3, bool bottom_align = FALSE,
						DWORD *splitter_frgr_color = NULL);

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);
	void			OnWmSize();
	bool			OnWmDestroy() { ResetDialogsDock(); return(FALSE); }

	bool		ShowChildDialog(TBasicDialog *dlg, bool bypass_ownership, long preferred_dialog_width = -1, TBasicDialog *ins_before = NULL, bool variable_size_dialog = TRUE, long min_dialog_width = 0)
												{ return(m_dock.ShowDialog(dlg, bypass_ownership, preferred_dialog_width, ins_before, variable_size_dialog, min_dialog_width)); }
	bool		HideChildDialog(TBasicDialog *dlg)	{ return(m_dock.HideDialog(dlg)); }

	void		ResetDialogsDock() { m_dock.ResetDock(); }
	void		BroadcastCommand(int ctrl_id, int ntf_code, LPARAM lParam = 0) { m_dock.BroadcastCommand(ctrl_id, ntf_code, lParam); }
	void		RecalcProps() { m_dock.SetupReferenceRect(m_hDlg, TRUE); }

	int			NumDialogs()					{ return(m_dock.NumDialogs()); }
	TBasicDialog	*GetChildDialog(int inx)		{ return(m_dock.GetChildDialog(inx)); }
	bool			GetDialogOwnedFlag(int inx)	{ return(m_dock.GetDialogOwnedFlag(inx)); }

protected:

	THorzBasicDialogsDock		m_dock;
	DWORD					m_dialog_template[BASIC_DIALOGS_DOCK_TEMPL_BUFF_LEN/sizeof(DWORD)];
								// The template is prepared in the ctor and it is never changed.
};

// This dialog does not need any dialog template in resources.
class TVertBasicDialogsDockDialog : public TBasicDialog
{
public:

	TVertBasicDialogsDockDialog(TBasicDialog *proto, short dock_weight_in_dbu, short show_hide_event_ctrl_id,
						bool need_first_splitter = FALSE, int splitter_height = 3, bool bottom_align = FALSE,
						DWORD *splitter_frgr_color = NULL);

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);
	void			OnWmSize();
	bool			OnWmDestroy() { ResetDialogsDock(); return(FALSE); }

	bool		ShowChildDialog(TBasicDialog *dlg, bool bypass_ownership, long preferred_dialog_height = -1, TBasicDialog *ins_before = NULL, bool variable_size_dialog = TRUE, long min_dialog_height = 0)
												{ return(m_dock.ShowDialog(dlg, bypass_ownership, preferred_dialog_height, ins_before, variable_size_dialog, min_dialog_height)); }
	bool		HideChildDialog(TBasicDialog *dlg)	{ return(m_dock.HideDialog(dlg)); }

	void		ResetDialogsDock() { m_dock.ResetDock(); }
	void		BroadcastCommand(int ctrl_id, int ntf_code, LPARAM lParam = 0) { m_dock.BroadcastCommand(ctrl_id, ntf_code, lParam); }
	void		RecalcProps() { m_dock.SetupReferenceRect(m_hDlg, TRUE); }

	int			NumDialogs()					{ return(m_dock.NumDialogs()); }
	TBasicDialog	*GetChildDialog(int inx)		{ return(m_dock.GetChildDialog(inx)); }
	bool			GetDialogOwnedFlag(int inx)	{ return(m_dock.GetDialogOwnedFlag(inx)); }

protected:

	TVertBasicDialogsDock		m_dock;
	DWORD					m_dialog_template[BASIC_DIALOGS_DOCK_TEMPL_BUFF_LEN/sizeof(DWORD)];
								// The template is prepared in the ctor and it is never changed.
};

#endif	// WinUi_DialogsDock_H


