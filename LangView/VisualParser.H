//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_VisualParser_H
#define	LangView_VisualParser_H

#ifndef   Common_LookAsideTable_H
#include  "Common/LookAsideTable.H"
#endif
#ifndef   WinUi_BasicDialog_H
#include  "WinUi/BasicDialog.H"
#endif
#ifndef   WinUi_ListBoxObject_H
#include  "WinUi/ListBoxObject.H"
#endif
#ifndef   WinUi_DialogsDock_H
#include  "WinUi/DialogsDock.H"
#endif
#ifndef   TextScan_GrammarAnalysis_H
#include  "TextScan/GrammarAnalysis.H"
#endif
#ifndef   TextScan_RamBasedStore_H
#include  "TextScan/RamBasedStore.H"
#endif
#ifndef   LangView_LangView_H
#include  "LangView/LangView.H"
#endif
#ifndef   LangView_SourceView_H
#include  "LangView/SourceView.H"
#endif
#ifndef   LangView_GrammarView_H
#include  "LangView/GrammarView.H"
#endif
#ifndef   LangView_LoadGrammarHelpers_H
#include  "LangView/LoadGrammarHelpers.H"
#endif
#ifndef   LangView_CppDefinitionsView_H
#include  "LangView/CppDefinitionsView.H"
#endif
#ifndef   LangView_SimpleBrowser_H
#include  "LangView/SimpleBrowser.H"
#endif

// Forward declarations for major classes in this file.
class TLangViewVisualParserFrame;
class TLangViewVisualParserLoadPanel;

//
//  This is MODAL dialog. Possible return values:
//
//          0 ... nDrvs-1	--	Index of the new driver;
//         -1			--	Dialog failed to open.
//         -2			--	User pressed Cancel.
//
//  This class picks up the list of available parsing drivers directly from the data fields of its owner.
//
//  Template: IDD_LANGV_VPARS_SELECT_DRIVER
//
class TLangViewVisualParserSelectDriver : public TBasicDialog
{
public:

	TLangViewVisualParserSelectDriver();

	void			SetOwner(TLangViewVisualParserLoadPanel *owner) { m_owner = owner; }

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	void			OnWmSize();
					// This method shows/hides the note below the main list box.

	bool			OnWmDestroy() { m_list_hlp.ResetListBox(); return(FALSE); }
					// Return value shows that the object was not deleted. This is important.

protected:

	enum ViewColumns
	{
		lvc_curr_driver,
		lvc_parser_lang_type, lvc_parser_descr,
		lvc_grm_proc_res, lvc_grm_descr, lvc_grm_vers,
	};

	enum FontColorSlotIndexes
	{
		fcslot_curr_mark = lbst_app_indexes_base,
		fcslot_curr_mark_sel,
		fcslot_no_grammar,
		fcslot_no_grammar_sel,
	};

	TLangViewVisualParserLoadPanel		*m_owner;

	TListBoxObject					m_list_hlp;
};

//
// ------- Load grammar, Load source file ------------
//
//  Template: IDD_LANGV_VPARS_LOAD_PANEL
//
class TLangViewVisualParserLoadPanel : public TBasicDialog
{
public:

	TLangViewVisualParserLoadPanel(bool initial_cpp_prompt = TRUE);

	void			SetOwner(TLangViewVisualParserFrame *owner);
	void			SetDriversInfo(TParsingDriver **drivers, int num_drivers, int curr_drv_inx);
	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	TParsingDriver			*GetDriver()		{ return((m_drivers_array != NULL) ? m_drivers_array[m_curr_driver_inx] : NULL); }
	TGrammarBasedParser		*GetParser()		{ return((GetDriver() != NULL) ? (TGrammarBasedParser*)&(GetDriver()->Parser()) : NULL); }
	TGrammar				*GetGrammar()	{ return((GetParser() != NULL) ? GetParser()->RootGrammar() : NULL); }

	void		UpdateLowerFrameOffset(long lower_frame_offs);

protected:

	bool		CheckParsingDriversPresence(bool full_check);
	bool		CheckParsingDriverIdle();

	void		DoGrammarLoadingSession(TLangViewLoadGrammarHelper &helper);

protected:

	TLangViewVisualParserFrame			*m_owner;

	TFileOpenHelper						m_ofn_source;
	TFileOpenHelper						m_ofn_cmdl;

	TParsingDriver						**m_drivers_array;
	int									m_num_drivers;
	int									m_curr_driver_inx;

	TLangViewVisualParserSelectDriver		m_select_driver;

	TGrammar							m_processed_grammar;
	TMidLevRamLogger						m_processed_grammar_parsing_log;
											// These fields contain results of SUCCESSFUL grammar loading. They contain data only
											// if grammar loading session was run at least once. For simplicity the logger is instantiated
											// in the single parsing session mode.

	friend class TLangViewVisualParserSelectDriver;
};

enum TLangViewVisualParserState
{
	vpst_idle,
	vpst_stopped,
	vpst_stopped_error,		// Parsing process is stopped because of the error. It is possible only to continue.
							// This state was introduced primarily because immediate setting state to vpst_final
							// is not good since worker tread is still running.
	vpst_running_step,
	vpst_running_run,
	vpst_running_break,
	vpst_running_abort,
	vpst_final,				// Final state. At this point the background thread has already terminated.
};

//
// --------- Step, Run, Break, Abort -------------
//
//  Template: IDD_LANGV_VPARS_CONTROL_PANEL
//
class TLangViewVisualParserControlPanel : public TBasicDialog
{
public:

	TLangViewVisualParserControlPanel();

	void			SetOwner(TLangViewVisualParserFrame *owner) { m_owner = owner; }
	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	void		SetupInitialMaskCheckBoxesState(DWORD run_mask, DWORD step_mask);
				// This method can be called only at the beginning from TLangViewVisualParserFrame::OnInitDialog().

	void		SetVisibleState(TLangViewVisualParserState stt,
						TGenericParserConsoleEvent stt_chk_box = pcev_none,
						const wchar_t *stt_label = NULL,
						const wchar_t *stt_message = NULL,
						const wchar_t *stt_dtls1 = NULL,
						const wchar_t *stt_dtls2 = NULL,
						const wchar_t *stt_dtls3 = NULL,
						const wchar_t *application_note = NULL,
						TGrammar *grammar = NULL, int iconflict = -1);
				//
				// Method updates the state of buttons and labels. It is not doing anything else. Note that this panel
				// does not have its own state. Messages should be omitted only for vpst_idle. All other debugger states
				// should pass the message parameter explicitly.
				//

	int		GetCurrentConflictingActionSelection();
				// Return value -1 means that user selected to keep the initial grammar conflict. Otherwise the return
				// value is the index of the selected conflicting action.

	long		GetLowerFrameOffset();
				// This method is needed for fancy adjustement of the controls on other panels.

protected:

	enum { numManualCfctResRadioBtns = 4 };

	void		SetupDarkBlueText(int rc_id);
	void		HideManualConflictResolutionControls();

	void		SetupMaskCheckBoxes(DWORD run_mask, DWORD step_mask, TGenericParserConsoleEvent mask_bit);
	void		UpdateMaskCheckBoxesAndFlags(int rc_clicked_chk_box, TGenericParserConsoleEvent mask_bit);
	void		UpdateMaskCheckBoxesBackground(TGenericParserConsoleEvent stt_chk_box);

	TLangViewVisualParserFrame	*m_owner;
									// The WM_COMMAND events from buttons and checkboxes are procesed in the main frame dialog.

	TGenericParserConsoleEvent		m_stt_chkbox;
									// This is event code that is either pcev_none that means that none of the checkboxes is highlighted,
									// or this code of highlighted checkbox. The correspondance between values and check box pairs is
									// the same to run/step masks data fields in the main  frame.
};

//
// ------------- Source code breakpoints ------------------
//
//  Template: IDD_LANGV_VPARS_SRC_BRK_POINTS
//
class TLangViewVisualParserSrcBreakPoints : public TBasicDialog
{
public:

	TLangViewVisualParserSrcBreakPoints();

	void			SetOwner(TLangViewVisualParserFrame *owner) { m_owner = owner; }
	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	void			UpdateLowerFrameOffset(long lower_frame_offs);

protected:

	TLangViewVisualParserFrame	*m_owner;

	TListBoxObject				m_list_hlp;
};

//
// -------------- Abstract syntax tree ------------------
//
//  Template: IDD_LANGV_VPARS_SYNTAX_TREE
//
class TLangViewAbstractSyntaxTreeViewer : public TLangViewChildBrowser
{
public:

	TLangViewAbstractSyntaxTreeViewer();

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	void			OnWmSize();
					// This method shows/hides the current zoom label.

	//
	//  Inherited virtual methods.
	//

	void		ResetContents();
	void		SetSelection(TLangViewClickEvent &event, bool ext_margins);
	void		ResetSelection();

	//
	//  New methods.
	//

	void		SetupBkgrColor(TColor bkgr_color = RGB(255, 255, 255), bool paint_now = TRUE);
	void		SetupUpperLabel(int lab_index, TTextStyle *style, const wchar_t *message, bool paint_now = TRUE);
	void		SetupUpperPicture(TBitmapStyle *style, short icon_index, bool paint_now = TRUE);
	void		SetupParsingState(WORD pars_stt, TTextStyle *style, bool paint_now = TRUE);
	void		SetupTree(TTreeItemsPlacingMode view_mode, TLangViewLexItemStyle *view_style,
								TVisualParserLexItemType root_type, WORD root_index,
								TLangViewTreeNode *subtree, bool clone_subtree = TRUE,
								bool paint_now = TRUE);

	void		UpdateUpperLabelStyle(int lab_index, TTextStyle *style, bool paint_now = TRUE);
	void		UpdateLexItemsStyle(TLangViewLexItemStyle *view_style, bool paint_now = TRUE);
	void		UpdateRootIndex(WORD new_root_inx, bool paint_now = TRUE);

	void		PrepareAndShowScreenItems(TGenericConsoleEx *cons = NULL);

	enum { NUM_UPPER_LABELS = 2 };

	TVisualParserLexItemType		GetRootType()			{ return(m_root.GetParserNodeType());  }
	WORD						GetRootIndex()			{ return(m_root.GetParserSlotIndex()); }
	WORD						GetParsingState()			{ return(m_pars_stt); }
	DWORD						GetUniqueSymbolId()		{ return((m_root.GetLowerLeftChild() != NULL) ? m_root.GetLowerLeftChild()->GetUniqueNodeId() : 0); }
	TColor						GetBkgrColor()			{ return(m_bkgr_color); }

	const wchar_t					*GetUpperLabelDataPtr(int lab_index) { return(m_upper_labels[lab_index].GetText()); }
	TTextStyle					*GetParsSttStyle()		{ return((TTextStyle*)m_pars_stt_scr_item.GetBasicStyle()); }
	TLangViewLexItemStyle			*GetLexItemsStyle()		{ return(m_view_style); }

	void		RestoreOriginalSize() { assert(m_hDlg != NULL); ::SetWindowPos(m_hDlg, NULL, 0, 0, m_original_rect.right-m_original_rect.left, m_original_rect.bottom-m_original_rect.top, SWP_NOMOVE | SWP_NOZORDER); }

protected:

	enum
	{
		concentric_margin_x = 24,  pcb_margin_x = 30,
		concentric_margin_y = 44,  pcb_margin_y = 26,
	};

	void			ShowZoom(float new_zoom);
	void			ProcessBrgrContextMenu(long click_pos_x, long click_pos_y);

	wchar_t		*PrepareSpecialOriginDescription(wchar_t *buffer, int buff_len, WORD final_action, TParsingActionOriginInfo &info);

protected:

	TScreenItemsViewObject			m_view_hlpr;

	TLangViewVisualParserTreeNode		m_root;
										// Root object may represent different objects. Most often this will be parser slot item
										// with only one child: lexema or non terminal.

	TLangViewVisualParserTreeNode		m_null;
										// This object is used only when Setup() method receives NULL subtree or when
										// the cloning of the passed subtree fails.

	WORD							m_pars_stt;
	TTextItem80						m_pars_stt_scr_item;
	TBitmapItem						m_upper_picture;
	TTextItem80						m_upper_labels[NUM_UPPER_LABELS];

private:

	TTreeItemsPlacingMode				m_view_mode;
	TLangViewLexItemStyle				*m_view_style;
	TColor							m_bkgr_color;

	RECT							m_original_rect;
										// This rect is set up in processing the InitDialog and it is needed to restore the size
										// of the viewer when it is retrieved from the look aside list.
};

//
// -------- Parser state and future actions -------------
//
//  Template: IDD_LANGV_VPARS_CURRENT_STATE
//
class TLangViewVisualParserCurrStatePanel : public TBasicDialog
{
public:

	TLangViewVisualParserCurrStatePanel();

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	void			OnWmSize();
					// This method centers the data set horizontally.

	void		ResetContents();
	void		SetupParsingState(WORD new_state, bool final_state, bool paint_now = TRUE);
	void		SetupLowerComment(int comment_index, TTextStyle *style, const wchar_t *message, bool paint_now = TRUE);
	void		PrepareAndShowContents();

	void		RestoreOriginalSize() { assert(m_hDlg != NULL); ::SetWindowPos(m_hDlg, NULL, 0, 0, m_original_rect.right-m_original_rect.left, m_original_rect.bottom-m_original_rect.top, SWP_NOMOVE | SWP_NOZORDER); }

protected:

	enum { NUM_LOWER_COMMENTS = 4 };

	void		SetupBackgroundColor(bool paint_now);
	void		MapTextScreenItem(HDC hDC, TTextItem &scr_item, long &pos_y);

	WORD						m_curr_state;
	bool							m_final_state;

	TScreenItemsViewObject		m_view_hlpr;

	TTextItem80					m_label_scr_item[3];
	TTextItem80					m_curr_state_scr_item;
	TRectItem					m_delimiter_scr_item[10];
	TTextItem80					m_comment_scr_items[NUM_LOWER_COMMENTS];

	TTextStyle					m_label_style;
	TTextStyle					m_curr_state_style;
	TTextStyle					m_final_style;
	TBasicStyle					m_delimiter_style1;
	TBasicStyle					m_delimiter_style2;

	RECT						m_original_rect;
};

//
// ----- EmptyStack, EmptyInput -------------
//
// This class has 2 resource templates that differ in their width:
//
//		IDD_LANGV_VPARS_EMPTY_STACK, IDD_LANGV_VPARS_EMPTY_INPUT
//
class TLangViewVisualParserSupportPanel : public TBasicDialog
{
public:

	TLangViewVisualParserSupportPanel() : TBasicDialog(NULL, 0) { m_nest_layer = 0; m_bkgr_color = RGB(255, 255, 255); }
		// Template ID will be set up later separately for each instance of the panel.

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl) { }

	void			SetupNestingLayerProps(int nest_layer, TColor bkgr_clr);
					// Nesting layer is passed only for displaying it in the static text title when it is not 0.

	void			RestoreOriginalSize() { assert(m_hDlg != NULL); ::SetWindowPos(m_hDlg, NULL, 0, 0, m_original_rect.right-m_original_rect.left, m_original_rect.bottom-m_original_rect.top, SWP_NOMOVE | SWP_NOZORDER); }

protected:

	void			MapTextScreenItem(HDC hDC, TTextStyle &style, TTextItem &scr_item, long offs_y);

	int							m_nest_layer;
	TColor						m_bkgr_color;

	TScreenItemsViewObject		m_view_hlpr;
	TTextItem80					m_label1, m_label2;
	TTextStyle					m_style1, m_style2;

	RECT						m_original_rect;

	friend class TLangViewVisualParserFrame;
};

//
// ----- ParsingDriver, GrammarBasedParser and Grammar -------------
//
//  Template: IDD_LANGV_VPARS_GRAMMAR_INFO
//
class TLangViewParserAndGrammarInfo : public TBasicDialog
{
public:

	TLangViewParserAndGrammarInfo();

	void		SetOwner(TLangViewVisualParserFrame *owner);
	void		SetDriver(TParsingDriver *driver);

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);
	void			OnWmSize();

	protected:

	void		UpdateButton(int ctrl_id);
	void		SetupControlContents();

	struct TFileProblemInfo
	{
		TFileProblemInfo(TGrammar *grm = NULL) { grammar = grm; load_res = ldres_success; }

		TGrammar			*grammar;
		TLoadSaveResult		load_res;
		long					disk_file_len;		// Length of the picked up file.
		TDateTime			disk_file_date;		// Date of the picked up file.
	};

	void ShowFileProblemMessage(TFileProblemInfo *info, bool forceDisplay);

	TLangViewVisualParserFrame		*m_owner;
	TGrammarBasedParser				*m_curr_parser;

	TScreenItemsViewObject			m_view_hlpr;

	TDecoratedFileViewStyle			m_file_view_style;
	TTextStyle						m_file_line_nums_style;
	TTextStyle						m_file_default_text_style;
	TTextStyle						m_file_horz_ovfl_style;

	TDecoratedFileViewScreenItem		m_file_view_scr_item;
	TLangViewGrammarBrowser			m_grammar_view;

	TFileProblemInfo					m_source_loading_problem;
};

//
// ------------ Parser console object  -----------------------
//
//  Note: Only the foreground UI thread shoud call methods of this object.
//
//  Inherited virtual methods ProcessEvent(), RecycleParserConsoleData() should be called on the worker thread only.
//
class TLangViewVisualParserConsoleObject : public TGenericParserConsole
{
public:
			TLangViewVisualParserConsoleObject();
			~TLangViewVisualParserConsoleObject();

	void		SetOwner(TLangViewVisualParserFrame *owner) { m_owner = owner; }

	void		ProcessEvent(TGenericParserConsoleEvent event_code, WORD &event_data, DWORD event_data_ex);
	void		RecycleParserConsoleData(TListItem *data);

	bool		ProcessSourceFileMethod(DWORD initial_suspension_mask, TParsingDriver &driver, TProcessSourceFileInfo &info);
	bool		ProcessCommandLineMethod(DWORD initial_suspension_mask, TParsingDriver &driver, TProcessCommandLineInfo &info);
	void		SetupSuspensionMask(DWORD new_mask);
	void		ResumeThread() { ::SetEvent(m_hEvent); }
	void		AbortParsingProcess(bool worker_thread_stopped);

	WORD	GetSavedSymbol() { return(m_saved_symbol); }
	WORD	GetSavedAction() { return(m_saved_action); }

	TSymbolStateItem				*GetLatestPickedSymbol() { return(m_latest_picked_symbol); }
	TGenericParserConsoleEvent		GetLatestEvent() { return(m_latest_event); }

	void		SetSavedAction(WORD action) { m_saved_action = action; }

	bool		CheckThread(bool &termination_happened_right_now, DWORD &exit_code);
				// Return value shows current state of the background thread: TRUE - active, FALSE - already terminated.

protected:

	void		CreateRecreateLexemaLexItem(TSymbolStateItem *slot, WORD final_action, TParsingActionOriginInfo spec_origin_info);
	bool		LaunchThread(DWORD initial_suspension_mask, LPTHREAD_START_ROUTINE thread_entry_point);

	static DWORD WINAPI ProcessSourceFileThreadProc(void *context)
	{
		// Thread return value can be only TRUE or FALSE.
		TLangViewVisualParserConsoleObject *ctx = (TLangViewVisualParserConsoleObject*)context;
		assert(ctx->m_driver != NULL);

		bool parsing_sess_started = FALSE;
		ctx->m_driver->ProcessSourceFile(ctx->m_src_file_info, &parsing_sess_started);
		return(parsing_sess_started);
	}

	static DWORD WINAPI ProcessCommandLineThreadProc(void *context)
	{
		// Thread return value can be only TRUE or FALSE.
		TLangViewVisualParserConsoleObject *ctx = (TLangViewVisualParserConsoleObject*)context;
		assert(ctx->m_driver != NULL);

		bool parsing_sess_started = FALSE;
		ctx->m_driver->ProcessCommandLine(ctx->m_cmd_line_info, &parsing_sess_started);
		return(parsing_sess_started);
	}

	TLangViewVisualParserFrame	*m_owner;

	TParsingDriver				*m_driver;

	WORD						m_saved_symbol;
	WORD						m_saved_action;

	TSymbolStateItem				*m_latest_picked_symbol;
									// This field can be used only inside processing pcev_symbol_update and
									// pcev_symbol_discard events.

	TGenericParserConsoleEvent		m_latest_event;

	bool							m_abort_request;
	bool							m_abort_raised;

	HANDLE						m_hEvent;
	HANDLE						m_hWorkerThread;
	DWORD						m_dwThreadId;
									// When hThread is not NULL, this means that parsing process is active.

	DWORD						m_dwThreadExitCode;
									//
									//	0x103	--	STILL_ACTIVE.
									//	    0		--	Parsing startup failure. This can be either thread starting
									//				problem or an app layer error.
									//	    1		--	Parser was called.
									//

	DWORD						m_suspension_mask;
									// Mask of event types that cause parsing suspension.

	TProcessSourceFileInfo			m_src_file_info;
	TProcessCommandLineInfo		m_cmd_line_info;
									// These fields are needed for keeping params of the call for a short time.
};

//
// -------------  MainFrame  -----------------------------------
//
//  Template: IDD_LANGV_VPARS_MAIN_FRAME
//
class TLangViewVisualParserFrame : public TLangViewBrowserFrame
{
public:

	TLangViewVisualParserFrame(TLangViewBrowserFrame *parent_handler = NULL, TMidLevStoreReader *db_link = NULL, bool cpp_prompt = TRUE);

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);
	void			OnWmTimer(int timer_id);
	bool			OnWmDestroy();

	void		ProcessSingleClick(TLangViewChildBrowser *inst, TLangViewClickEvent &event_info);
	void		ProcessDoubleClick(TLangViewChildBrowser *inst, TLangViewClickEvent &event_info);
	void		RemoveSelection(TLangViewChildBrowser *inst);

	void		SetParsingDriversInfo(TParsingDriver **drivers, int num_drvs, int curr_drv_inx) { m_load_panel.SetDriversInfo(drivers, num_drvs, curr_drv_inx); m_grammar_info.SetDriver(drivers[curr_drv_inx]); }
				// This method sets driver into the load panel only. Note that this dialog can work without the load panel.
				// Load panel passes this driver to the frame dialog each time it tries to start the parsing session.

	bool		StartProcessingSourceFile(TParsingDriver &driver, TProcessSourceFileInfo &info);
	bool		StartProcessingCommandLine(TParsingDriver &driver, TProcessCommandLineInfo &info);
				//
				// These methods start parsing on the background thread. It is allowed to start only one parsing at a time.
				// If return value is TRUE, this means that parsing was successfully inited and caller should wait for async
				// result, i.e. call to OnParsingComplete(). If return value is FALSE, this means sync failure.
				// At this point reason of the failure is already displayed in the message box.
				//

	virtual  void		OnInitDialogPrepareMasks() { }
	virtual  void		OnInitDialogDoAppActions() { }
						// These callbacks are called when OnInitDialog() is about to be exited. App layer may modify
						// the suspension masks and/or load some source file from inside of these callbacks.

	virtual  void		OnParsingComplete(bool parsing_succeeded) { }
						// Method is called when parsing is finished and final state just became visible to the user.

protected:

	void		ProcessConsoleNtf(TGenericParserConsoleEvent evt_code, WORD evt_data);
	void		SetupStoppedState(TGenericParserConsoleEvent evt_code, WORD evt_data);
	void		SetupRunningState(TLangViewVisualParserState new_stt, bool processing_startup = FALSE);
	void		SetupFinalState(DWORD thread_exit_code, bool &parsing_success);

	void		LoadParsingSession(const wchar_t *session_root_fname);
	void		UnloadParsingSession();

	void		SetupConsoleMask(TLangViewVisualParserState stt);
	void		UpdateSuspensionMasksFromControlPanel(bool run_mask, TGenericParserConsoleEvent mask_bit, bool new_maks_value);

	bool		UpdateParserStateSplitter(TGenericParserConsoleEvent event, DWORD event_dtls);
				//
				// Return value indicates the selected layout:
				//
				//	TRUE  - single tree with "Program" in the root;
				//	FALSE - parsing stack, curr state and input queue;
				//
				// NB: This function is also responsible for initing/updating state of the LexItems styles.
				//

	void		UpdateParserAndGrammarInfo(TParsingDriver *driver) { m_grammar_info.SetDriver(driver); }
	void		UpdateParsingStateDetails();
	void		UpdateParsingLocationMarker();

	void		SetupUpperLabels(TLangViewAbstractSyntaxTreeViewer *panel, TSymbolStateItem &slot, int istyle);
	bool		CheckUpperLabelsState(TLangViewAbstractSyntaxTreeViewer *panel, TSymbolStateItem &slot);
	int		GetDescriptionLineLength(wchar_t *descr_body);

	//
	//  Name table viewers handling section.
	//

	void		NameTableView_InitChildDialogs();
	bool		NameTableView_OnInitDialog();
				// These functions should prepare all available viewers for all supported languages.

	void		NameTableView_LoadParsingSession();
	void		NameTableView_UnloadParsingSession();
	void		NameTableView_SetupStoppedState();
	void		NameTableView_SetupRunningState();
	void		NameTableView_SetupFinalState(bool parsing_succeeded);
				// These functions should setup table viewers only for the current language based on the current
				// value of m_name_table_view_code data field.

	bool		CheckLexemaBreakPoint();

	inline	TLangViewTreeNode	*GetSlotData(TSymbolStateItem &slot) { return((TLangViewTreeNode*)slot.m_dbg_data); }

	bool		PrepareForProcessing(TParsingDriver &driver);

	TLangViewAbstractSyntaxTreeViewer		*AllocateSyntaxTreeViewer();
	TLangViewVisualParserSupportPanel		*AllocateEmptyStackPanel();
	void									RecycleSyntaxTreeViewer(TLangViewAbstractSyntaxTreeViewer *inst);

	void		ShowAsyncMessage(const wchar_t *message);
				// This method should be called ONLY with static const messages. Otherwise the memory leak or the memory corruption will happen.

	void		ShowSourceFileLoadingProblem(TLoadSaveResult load_res, const wchar_t *fname, bool forceDisplay = FALSE);

	class TParserStateDockIterationHelper
	{
	public:

		TParserStateDockIterationHelper(TLangViewVisualParserFrame *owner, THorzBasicDialogsDockDialog &dock) : m_owner(owner), m_dock(dock) { Reset(); }

		void		Reset();
		void		ShiftToNextDialog();
		bool		IsIterationComplete() { return(m_inx == m_num_dialogs); }

		void				ShowDialogBeforeCurr(TBasicDialog &dlg, bool bypass_ownership);
		void				HideAndDismissCurrDialog();
		TBasicDialog		*RemoveCurrDialog(bool &ownership_returned);


		TBasicDialog							*m_curr_dialog;
		short								m_template_id;

		TLangViewAbstractSyntaxTreeViewer		*m_syntax_tree;
		TVisualParserLexItemType				m_syntax_tree_root_type;
												// Value of the field is vplit_num_types when curr panel is not a syntax tree.
	private:

		void PickUpCurrentDialog();

		TLangViewVisualParserFrame		*m_owner;
		THorzBasicDialogsDockDialog		&m_dock;

		int								m_inx;
		int								m_num_dialogs;
	};

protected:

	enum { MainTimerIdent = 50 };
	enum { STV_LOOK_ASIDE_LEN = 32 };

	//
	// State and control vars.
	//

	TLangViewVisualParserState						m_vpst;

	TGrammarBasedParser							*m_parser;
													// This field is not NULL only when the parsing session is active.

	TLangViewVisualParserConsoleObject				m_parser_console;
	TDateTime									m_latest_resume_time;
	TDialogBasedConsole							m_wait_panel;
	TDialogBasedConsole							m_proc_stt_panel;

	DWORD										m_step_brks_mask;
	DWORD										m_run_brks_mask;
													//
													// These masks represent state of the check boxes on the control panel
													// rather than types of events. Bits in these masks correspond to check
													// boxes as 1 to 1. In particular:
													//
													//    pcev_parsing_startup		-  "Startup"
													//    pcev_symbol_pickup		-  "Symbol Pickup"
													//    pcev_symbol_update		-  "Symbol Update"
													//    pcev_symbol_discard		-  "Symbol Discard"
													//    pcev_errec_sym_added		-  "Error Recovery"
													//
													//    pcev_action_pickup		-  "Action Pickup"
													//    pcev_grpars_cfct_pickup	-  "Conflict Pickup"
													//    pcev_action_update		-  "Action Update"
													//    pcev_action_shift			-  "Action Symbol Shift"
													//    pcev_action_resolve		-  "Action Apply Rule"
													//    pcev_enter_nesting		-  "Enter nesting"
													//    pcev_exit_nesting			-  "Exit nesting"
													//
													// Masks should not contain bits that are not mentioned here.
													//
													// In fact, "Termination" checkboxes are existing, but there is no
													// corresponding bit for them in the masks because these check boxes
													// are always checked and disabled.
													//

	DWORD										m_lex_item_ids_cnt;

	TList											m_expected_files;				// These files are expected to be used but they are
																				// not loaded yet.
	TList											m_lexema_breakpoints;
	TList											m_pars_stt_breakpoints;

	// Dock helpers.
	TVertBasicDialogsDockDialog					m_main_vert_dock;
	THorzBasicDialogsDockDialog					m_upper_horz_dock;
	THorzBasicDialogsDockDialog					m_main_horz_dock;
	TVertBasicDialogsDockDialog					m_second_vert_dock;
	TVertBasicDialogsDockDialog					m_right_vert_dock;
	THorzBasicDialogsDockDialog					m_middle_horz_dock;
	THorzBasicDialogsDockDialog					m_lower_horz_dock;

	// Child dialogs for the upper splitter.
	TLangViewVisualParserLoadPanel					m_load_panel;
	TLangViewVisualParserControlPanel				m_control_panel;
	TLangViewVisualParserSrcBreakPoints				m_scr_break_points;

	// Child browsers for the middle splitter.
	TLangViewVisualParserCurrStatePanel				m_curr_pars_stt;
	TLangViewVisualParserSupportPanel				m_empty_input;

	// Child browsers for the lower splitter.
	TLangViewParserAndGrammarInfo				m_grammar_info;
	TLangViewRulesGridDialog						m_pars_stt_details;
	TLangViewSourceFileDialog						m_src_view;

	// Child browsers for the vert right splitter.
	TParserNamesTableViewerType					m_nam_tab_view_code;			// This code is set each time the parsing session is loaded.
	TLangViewCppDefnsSpaceViewer					m_cpp_global_namesp;
	TLangViewClickEventDetailsDialog				m_cpp_item_details;
	TLangViewCppItemsTreeDialog					m_cpp_items_report;

	// MidLev database contents viewer.
	TLangViewSimpleBrowser						m_mid_lev_viewer;

	// Async messages support.
	TLoadSaveResult								m_source_file_load_res;
	wchar_t										m_source_file_name[2*MAX_PATH];

	// Screen item styles.
	TBitmapStyle									m_synt_tree_picts_style;
	TColor										m_synt_tree_bkgr_color[TLangViewLexItemStyle::NUM_DIM_GRADES];
	TTextStyle									m_synt_tree_upper_msgs_style[TLangViewLexItemStyle::NUM_DIM_GRADES];
	TTextStyle									m_synt_tree_pars_stt_style[TLangViewLexItemStyle::NUM_DIM_GRADES];
	TLangViewLexItemStyle							m_lex_items_styles[TLangViewLexItemStyle::NUM_DIM_GRADES];

	TStringsDict									m_lex_info_strs_dict;

	typedef  TLookAsideTable<TLangViewAbstractSyntaxTreeViewer, STV_LOOK_ASIDE_LEN>  TSyntaxTreeViewersLookAside;

	// Child viewers lookaside list.
	TSyntaxTreeViewersLookAside					m_stv_lookaside_list;

	friend class TLangViewVisualParserSelectDriver;
	friend class TLangViewVisualParserLoadPanel;
	friend class TLangViewVisualParserControlPanel;
	friend class TLangViewVisualParserConsoleObject;
};

#endif	// LangView_VisualParser_H


