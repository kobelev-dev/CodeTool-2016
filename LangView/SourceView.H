//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_SourceView_H
#define	LangView_SourceView_H

#ifndef   Xml_HtmlGenDecors_H
#include  "Xml/HtmlGenDecors.H"
#endif
#ifndef   ScreenItems_DecoratedFileView_H
#include  "ScreenItems/DecoratedFileView.H"
#endif
#ifndef   TextScan_MidStore_H
#include  "TextScan/MidStore.H"
#endif
#ifndef   LangView_LangView_H
#include  "LangView/LangView.H"
#endif
#ifndef   LangView_SyntaxHighlight_H
#include  "LangView/SyntaxHighlight.H"
#endif

// - - - - - - - -  Source View Style  - - - - - - - - - -

enum TLangViewExtBkgrParamInx
{
	ebpi_code_area_slct_bkgr,						// Background color of the selected line numbers area.

	ebpi_active_fragm_nums_bkgr1,
	ebpi_active_fragm_marks_bkgr1,
	ebpi_active_fragm_nums_bkgr2,
	ebpi_active_fragm_marks_bkgr2,

	ebpi_cpp_object_bkgr,							// Transparent bkgr. This decor describes the whole area of the high level language object.

	ebpi_num_types,
};

enum TLangViewExtFrgrParamInx
{
	efpi_code_area_slct_lnums,						// Font of the selected line number.
	efpi_num_types,
};

struct TLangViewSourceFileFramingProps
{
	short	frp_lines_above, frp_lines_below;			// Height of the margins for the non whole file enclosures, like macro and macro
													// param calls. These margins are used only when the range of lines to display
													// is not given directly.
	short	frp_slct_above, frp_slct_below;			// Minimal visible spacing around the selected element in the number of lines
													// when this element is scrolled into the view.
};

//
//  Ownership on all returned pointers still belongs to the source view style object. Application layer
//  does not need to dispose them.
//
//  Note that this object is derived not from the style of the decorated file view, but from the generic
//  screen item style.
//
class TLangViewSourceFileStyle : public TScreenItemStyle
{
public:
			TLangViewSourceFileStyle(bool want_vert_text_offs = TRUE, bool want_2pixel_frame_shadow = FALSE);

	enum { type_ID = 0x02050 };

	short	StyleTypeId() const { return(type_ID); }

	bool		SetupDefaultSrcProps();
	bool		SetupForDeviceContext(HDC hDC);

	void		SetTabsSpacesVisibMode(bool visib_tabs, bool visib_spaces);

	const TDecoratedFileViewStyle			*GetWholeFileStyle(TSrcViewWholeFileStyleInx inx) const;
	const TLangViewSourceFileFramingProps	&FramingProps() const { return(m_framing_props); }

	void				GetCodeStatusBkgrStyle(TConditionalStatus code_status, TBasicStyle *&line_nums, TBasicStyle *&marks_area, TBasicStyle *&line_body);

	TBasicStyle		*GetIndexingKeyUseStyle(TKeyType fkt, TRawContextType ctxt, bool active_code);

	TTextStyle		*GetCmdlLexemaFrgrStyle(TSrcViewCmdlLexemaStyleInx inx, bool active_code);
	TTextStyle		*GetCLangLexemaFrgrStyle(TSrcViewCLangLexemaStyleInx inx, bool active_code);

	TBasicStyle		*GetMidLevObjectBkgrStyle(TSrcViewMidLevBkgrStyleInx inx, short &vert_ext_up, short &vert_ext_down, bool active_code);
	TTextStyle		*GetMidLevObjectFrgrStyle(TSrcViewMidLevFrgrStyleInx inx, bool active_code);

	TBasicStyle		*GetCppObjectBkgrStyle(short &vert_exts);					// Cpp objects can be present only in the active code.
	TTextStyle		*GetCppObjectFrgrStyle(TCppSourceAreaType inx);				// Cpp objects can be present only in the active code.

	TScreenItem		*GetSpacelessDecorScreenItem(TSrcViewSpacelessDecorInx obj_type, short &horz_offset, short &vert_offset,
														short &vert_spacing_below, bool active_code, int &cnt_decor_creation_errors);
						// This method creates and returns inserted, imposed inline and imposed whole line decorations.
						// The caller is responsible for understanding the actual type of the returned decoration.

	long				GetScopeChangeExtraSpace()	{ return(2); }
	TColor			GetObjectSelectionBkgrColor()	{ return(RGB(0, 0, 0)); }
	TColor			GetObjectSelectionFrgrColor()	{ return(RGB(255, 255, 212)); }

	TBasicStyle		*GetBkgrParam(TLangViewExtBkgrParamInx inx);
	TTextStyle		*GetFrgrParam(TLangViewExtFrgrParamInx inx);

	inline bool		CheckStylePrepared() const { return(m_stls_prepared == TRUE); }

protected:

	bool								m_vert_text_offs;								// This field enables the top-bottom areas above/below the text lines.
																					// Note that these areas are not empty in the default static props.

	bool								m_frame_shadow;								// When the value if this field is TRUE, the frame shadow is added. The width
																					// and color of the shadow is specified in the props of the decorated file view
																					// style.
	bool								m_stls_prepared;

	TDecoratedFileViewStyle			m_file_view_styles[wfsi_num_types];
	TTextStyle						m_line_nums_styles[wfsi_num_types];
	TTextStyle						m_default_text_styles[wfsi_num_types];
	TTextStyle						m_line_overflow_styles[wfsi_num_types];

	TBitmapStyle						m_non_ascii_picts;								// This bitmap is passed to the decorated file view layer during the style setup
																					// process. It is not used in the source file viewer directly.
	TLangViewSourceFileFramingProps	m_framing_props;

	TBasicStyle						m_code_status_line_nums_style[cexr_num_types];
	TBasicStyle						m_code_status_marks_area_style[cexr_num_types];
	TBasicStyle						m_code_status_line_body_style[cexr_num_types];
	TBasicStyle						m_indexing_key_use_style[rctx_num_types];

	TTextStyle						m_cmdl_lexema_frgr_style[rcmsi_num_types];
	TTextStyle						m_clang_lexema_frgr_style[rclsi_num_types];

	TBasicStyle						m_mid_lev_obj_bkgr_style[mlbsi_num_types];
	TTextStyle						m_mid_lev_obj_frgr_style[mlfsi_num_types];

	TTextStyle						m_cpp_object_frgr_style[cxa_num_types];

	TBitmapStyle						m_spaceless_decors_style;

	TBasicStyle						m_ext_bkgr_style[ebpi_num_types];
	TTextStyle						m_ext_frgr_style[efpi_num_types];

};

//
//  ---- Major class of this header -----
//
class TLangViewSourceFileScreenItem : public TDecoratedFileViewScreenItem
{
public:

	TLangViewSourceFileScreenItem();
		// The destructor is not needed because all resources are released by dector of the base class and dectors of the data fields.

	virtual void	SetStyle(TScreenItemStyle *stl) { assert(IsInited() == FALSE); if (stl != NULL) assert(stl->StyleTypeId() == TLangViewSourceFileStyle::type_ID); TScreenItem::SetStyle(stl); }

	void		ResetSourceFile();
				// Method resets underlying decorated file view and releases all syntax related objects.

	bool		SetupSourceFile(TBasicDialog *dlg, TScreenItemsViewObject &scr_view, TFileInfo &file, bool bypass_file_info_data,
								int line_from = 1, int num_lines = 0);
				//
				// This method is used to setup the compilation independent files. Note that the parsing area covers the whole
				// file. The passed file can have any type of the raw syntax. Default values for the requested lines range will
				// display the whole file.
				//
				// The passed file should have: file name, file body and the lines info. All these fields should be not NULL.
				// Otherwise this method will fail and the return value will be FALSE.
				//
				// Once the ownership on the file data is not bypassed, the file data is expected to be persistent during the whole
				// lifetime of the object or until the next setup.
				//

	bool		SetupSourceFile(TBasicDialog *dlg, TScreenItemsViewObject &scr_view, TMidLevStoreReader *db, TMidLevScanner *active_scanner,
								ID call_pars_id, ID call_event_id, int line_from = 0, int num_lines = 0);
	bool		SetupSourceFile(TBasicDialog *dlg, TScreenItemsViewObject &scr_view, TMidLevStoreReader *db, TMidLevScanner *active_scanner,
								CtvLexSourceEnterEvent &call_event, int line_from = 0, int num_lines = 0);
				//
				//  These methods set up files that were participating in compilations. Participation means that this file was
				//  entered by some lex source enter event. When the range of visible lines is not specified, it is derived from
				//  the area that was paricipating in the parsing process plus some framing lines above and below this area.
				//
				//  When the active scanner is not NULL, this means that current database contains info about the parsing that
				//  is not finished. In this case the scanner is used to pick up the current state of parsing.
				//

	const TFileInfo					&FileInfo() const { return(m_file); }
									// Note that returned file info does not contain the file name in it. To get the name of
									// the file, it is necessary to use either FileName() or FileNameForDisplay() methods.

	const wchar_t					*FileName() const { return((m_fname_buffer[0] != 0) ? m_fname_buffer : NULL); }
	const wchar_t					*FileNameForDisplay() const { return((m_fname_buffer[0] != 0) ? m_fname_buffer : L"<none>"); }

	CtvLexSourceEnterEvent		&SrcEnterEvent() { return(m_src_enter_event); }

	bool		PrepareClickEventInfo(TMidLevStoreReader *db, TMidLevScanner *active_scanner, TLangViewClickEvent &buffer, TScreenItemsViewMouseNotify *mntfi, wchar_t **const_err_msg);
	int		GetDecorsListFromPoint(long x, long y, TScreenItem **buffer, int &len_buff, TScreenItem *worker_item);
	void		GetCtxMenuObjectDescription(TSourceViewDecorOwner *owner, DWORD owner_data, wchar_t *buffer, long buff_len);
	long		GetScrollPosForSourceArea(TSourceAreaInfo &area, long curr_scroll, long window_height, bool show_margins);
	void		ScrollToSourceArea(TSourceAreaInfo &src_area, bool want_horz_scroll, bool show_margins);

	bool		SelectClickEventArea(TBasicDialog *dlg, TMidLevStoreReader *db, TMidLevScanner *active_scanner, TLangViewClickEvent &event, bool show_margins);
	void		ResetClickEventAreaSelection();

	void		SetupSpecialLocationMarker(long src_offs = -1);
				// This method is expected to be used with unparsed source files. Negative value of the param
				// removes the marker if it is currently present.
protected:

	inline TLangViewSourceFileStyle	*Style() { return((TLangViewSourceFileStyle*)m_style); }

	bool		AcceptFileData(TBasicDialog *dlg, TScreenItemsViewObject &scr_view, TFileInfo &file, bool bypass_file_info_data, int line_from, int num_lines);
	bool		FinalizeSourceFileSetup(TBasicDialog *dlg);

	bool		ReconstructCmdlLexema(TCmdlLexema &buff_for_lexema, DWORD owner_data, wchar_t **const_err_msg);
	bool		ReconstructCLangLexema(TLexema &buff_for_lexema, DWORD owner_data, wchar_t **const_err_msg);

	long		GetLinearOffset(long src_file_offset, bool non_empty_area);
				// This method returns zero if the current file does not belong to any parsing session or if the passed
				// offest does not belong to the current active area.

	//----- Decors group 2. Single MidLev objects. -----


	void		ApplyMidLevImposedDecor(TSourceViewDecorOwner *decor_owner, TSrcViewSpacelessDecorInx inx, long src_offs);
				// The icon is inserted at the end of the passed linear area.

	//
	// The weight of the decor owner is used while sorting items of the right button context menu.
	// Items with bigger weights appear at the bottom of the menu.
	//
	enum TDecorOwnerWeightBase
	{
		fvdw_raw_lex_object	= 100,		// Cmdl, CLang lexemas and the current scan pos.
		fvdw_mid_lev_object	= 200,		// MidLev events and MacroHdr objects.
		fvdw_paste_buff_area	= 300,		// Area of the file where the paste buffering was taking place.
		fvdw_file_area_status	= 400,		// CodeStatus, OutOfScope, AbortedScan Unscanned.
	};

	static TDecorOwnerWeightBase g_BuiltInDecorOwnerTypeWeights[svdo_app_decor_types_base];

	static short		GetCtxMenuWeight(TSourceViewDecorOwner *owner);
						 // This method gives a room for the app defined decor owner types and makes it evident
						 // how the context menu items are sorted. Objects with built in owner types return values
						 // from the TDecorOwnerWeightBase enum.

	 static void		 GetCtxMenuMidLevEventDescr(CtvParsingEventInfo &data, wchar_t *buffer, long buff_len);
						 // This method is a worker function for GetCtxMenuObjectDescription(). Its purpose is
						 // to keep the size of the calling function reasonable. This method is called only from there.

	static void		GetCtxMenuMacroDefHeaderOwnerDataDescr(DWORD owner_data, wchar_t *buffer, int len_buff);
						// This method returns a short description that fits into the 40 char buffer.

protected:

	class TSourceViewScreenItemBuilder : public TSyntaxHighlightDocumentBuilder
	{
	public:
			TSourceViewScreenItemBuilder(TLangViewSourceFileStyle *style, TLangViewSourceFileScreenItem *owner) : m_src_file_style(*style), m_owner_screen_item(*owner) { }

		virtual void	ApplyFileViewStyleToUpperArea(TSrcViewWholeFileStyleInx view_style_inx, long upper_area_end, bool whole_line_area_style);
		virtual void	ApplyFileViewStyleToLowerArea(TSrcViewWholeFileStyleInx view_style_inx, long lower_area_beg, bool whole_line_area_style);
		virtual void	ApplyUnscannedAreaStyle(TSrcViewWholeFileStyleInx view_style_inx, long area_beg, long area_end, bool area_end_whole_line_style);
		virtual void	ApplySpecialNumsMarksBackgrounds(long src_pars_beg_offs, long src_pars_end_offs, bool whole_line_area_style);
		virtual void	SetupUnscannedAreaEntranceIcon(long area_entrance_offs, bool lower_layer_icon);
		virtual void	SetupUnscannedLineEndArea(long real_src_active_end);

		virtual void	ApplyMidLevBkgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewMidLevBkgrStyleInx style_inx, TSourceAreaInfo &src_area);
		virtual void	ApplyMidLevFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewMidLevFrgrStyleInx style_inx, TSourceAreaInfo &src_area);
		virtual void	ApplyMidLevWlbkDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, long line_beg, long num_lines);

		virtual void	ApplyCppObjectBkgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSourceAreaInfo &src_area);
		virtual void	ApplyCppObjectFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TCppSourceAreaType area_type, TSourceAreaInfo &src_area);

		virtual void	ApplyCodeStatusDecors(TSourceViewDecorOwner *owner, bool whole_line_beg_style, bool whole_line_end_style);

		virtual void	ApplyCmdlRawDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewCmdlLexemaStyleInx style_inx, TSourceArea &area);
		virtual void	ApplyCLangRawDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewCLangLexemaStyleInx style_inx, TSourceArea &area);
		virtual void	ApplyMacroHdrFrgrDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewMidLevFrgrStyleInx style_inx, TSourceArea &area);

		virtual void	ApplyInlineSpaclessDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewSpacelessDecorInx decor_type_inx, short horz_order, long src_area_offs);
		virtual void	ApplyWholeLineSpaclessDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewSpacelessDecorInx decor_type_inx, short horz_order, long line_number);

		virtual bool	GetVisibleArea(long &visible_area_beg, long &visible_area_len);
		virtual void	ReportProcessingProblem(const wchar_t *message);

	protected:

		TLangViewSourceFileStyle				&m_src_file_style;
		TLangViewSourceFileScreenItem		&m_owner_screen_item;
	};

protected:

	//----- General support group -----

	void		DecorateLineBeg(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder,
								TSimpleLineInfo *linfo, long offs_end, long extra_spacing_value,
								const TBasicStyle *nums_style = NULL, const TBasicStyle *marks_style = NULL, const TBasicStyle *text_bkgr_style = NULL,
								const TTextStyle *line_nums_frgr_style = NULL, const TTextStyle *text_frgr_style = NULL);

	void		DecorateLineMiddle(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder,
								long area_beg, long area_end, long extra_spacing_front, long extra_spacing_back,
								const TBasicStyle *nums_style = NULL, const TBasicStyle *marks_style = NULL, const TBasicStyle *text_bkgr_style = NULL,
								const TTextStyle *line_nums_frgr_style = NULL, const TTextStyle *text_frgr_style = NULL);

	void		DecorateLineEnd(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder,
								TSimpleLineInfo *linfo, long offs_beg, long extra_spacing_value,
								const TBasicStyle *nums_style = NULL, const TBasicStyle *marks_style = NULL, const TBasicStyle *text_bkgr_style = NULL,
								const TTextStyle *line_nums_frgr_style = NULL, const TTextStyle *text_frgr_style = NULL);

	void		DecorateLinesArea(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder,
								TSimpleLineInfo *linfo_beg, TSimpleLineInfo *linfo_end,
								const TBasicStyle *nums_style = NULL, const TBasicStyle *marks_style = NULL, const TBasicStyle *text_bkgr_style = NULL,
								const TTextStyle *line_nums_frgr_style = NULL, const TTextStyle *text_frgr_style = NULL);

				//
				// The whole group of methods below has the same set of parameters, as in the base class. The difference is only
				// in the type of the owner pointer and type the zorder param (it is defined as enum here). Methods do additional
				// checks, call corresponding method in the base class and update the errors count if needed.
				//

	void		CreateBkgrDecoration(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder, long area_beg, long area_len, const TBasicStyle *style, short upper_ext, short lower_ext);
	void		CreateFrgrDecoration(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder, long area_beg, long area_len, const TTextStyle *style);
	void		CreateInsertedDecoration(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder, long area_offs, TScreenItem *scr_item, short decor_horz_order, long baseline_vert_offs);
	void		CreateImposedDecoration(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder, long area_offs, TScreenItem *scr_item, long horz_offs, long baseline_vert_offs);
	void		CreateHorzSpacingDecoration(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder, long area_offs, long spacing_val_left, long spacing_val_right);
	void		CreateSpecTabDecoration(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder, long area_offs, short decor_horz_order);

	void		CreateWholeLineBkgrDecoration(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder, long line_beg, long num_lines, const TBasicStyle *style, WORD cover_mask);
	void		CreateLineNumFrgrDecoration(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder, long line_beg, long num_lines, const TTextStyle *style);
	void		CreateSpecTabsAreaDecoration(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder, long line_beg, long num_lines);
	void		CreateWholeLineImposedDecoration(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder, long line_num, TScreenItem *scr_item, long horz_offs, long baseline_vert_offs);
	void		CreateWholeLineSpacingDecoration(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder, long line_num, bool above_spc, short spc_value, bool accept_wl_bkgr);
	void		CreateTopBottomAreaBkgrDecoration(TSourceViewDecorOwner *owner, bool cover_top_area, bool cover_bottom_area, const TBasicStyle *style, WORD cover_mask);

protected:

	TFileInfo								m_file;
	wchar_t								m_fname_buffer[2*MAX_PATH];
											//
											//	The m_file.file_name field is always NULL.
											//	The m_file.file_body and m_file.lines_info fields are ALWAYS not NULL.
											//
											// The type of the raw syntax is taken from the m_file.rscn_type field. Note that this field never
											// owns the file body and the lines info. This info is either owned by the base class or it is owned
											// by the application layer.
											//

	CtvLexSourceEnterEvent				m_src_enter_event;
											// This field is clear when the displayed file is not part of any parsing session. Otherwise this field
											// describes the currently displayed lex source enclosure.

	TSourceAreaInfo						m_active_area;
											// This is an active area of the file where the lexemas can be highlighted. When the parsing is not
											// finished or when it was aborted, this area can be smaller than what is defined in the lex src call
											// event. Note that in cases of the suffix based paste bufferings there will be small gaps inside
											// the active area where the raw scanning should not take place.

	TSourceAreaInfo						m_code_stts_area;
											// This is the area that is covered with the code status decorations. It is not identical to the active
											// area only in the case of the macro lex src enclosure where it also contains the macrodef header.

	TSourceViewDecorOwnersContext		m_decor_owners;
											// List of all dynamic decor owners and all static decor owners.

	int									m_decor_creation_errors;
	int									m_decor_owner_creation_errors;

	TStringsDict							m_local_strings_dict;
											// This dict is used when the click event structures are prepared.

	friend class TSourceViewScreenItemBuilder;
};

//
//  Default template: IDD_LANGV_SVF_SOURCE_FILE_VIEW.
//  This dialog may also use other templates.
//
class TLangViewSourceFileDialog : public TLangViewChildBrowser
{
public:

	TLangViewSourceFileDialog(TMidLevStoreReader *db_link, TLangViewChildBrowserType type = cbrt_src_primary,
									short template_id = 0, TColor bkgr_color = RGB(226, 242, 255), TColor bkgr_color_no_file = RGB(255, 255, 255));

	LRESULT		OnInitDialog(HWND hDefaultFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);
	void			OnWmSize();

	void		SetShowHideScrollersMode(bool horz_scroll, bool vert_scroll);

				//
				//  Source file can be loaded only to the opened (but may be still invisible) dialog. This restriction
				//  is posed because OnInitDialog() is doing initialization that requires DC.
				//

	void		SetSpecialClickProcessingModes(bool send_reset_slct_on_click, bool send_encl_loc_on_dblclk)
					{ m_send_reset_slct_on_click = send_reset_slct_on_click; m_send_encl_loc_on_dblclk = send_encl_loc_on_dblclk; }

	bool		LoadContents(TFileInfo &unparsed_src_file, bool bypass_file_info_data, long scroll_pos = 0);
	bool		LoadContents(TMidLevScanner *active_scanner, ID pars_id, ID lex_src_enter_evid, long scroll_pos = 0);
	bool		LoadContents(TMidLevScanner *active_scanner, CtvLexSourceEnterEvent &call_event, long scroll_pos = 0);
				// Value of the scroll pos param is the preferred vert scrolling position in pixels.

	void		ScrollViewerTo(TSourceAreaInfo &src_area) { m_file_item.ScrollToSourceArea(src_area, TRUE, TRUE); }
				// This method will apply default visibility margins.

	const TFileInfo		&CurrFileInfo() const { return(m_file_item.FileInfo()); }
	const wchar_t		*CurrFileName() const { return(m_file_item.FileName()); }

	void		SetupSpecialLocationMarker(long src_offs = -1) { m_file_item.SetupSpecialLocationMarker(src_offs); }

	//
	//  Inherited virtual methods.
	//

	virtual void	ResetContents();
	virtual void	SetSelection(TLangViewClickEvent &event, bool show_margins) { m_file_item.SelectClickEventArea(this, m_db, m_active_scanner, event, show_margins); }
	virtual void	ResetSelection() { m_file_item.ResetClickEventAreaSelection(); }

protected:

	bool		LoadContentsInternal(TMidLevScanner *active_scanner, long scroll_pos);

	void		RefreshFileItemMinWidth();
	void		RefresMainViewerProps();

	void		ProcessClickEvent(TScreenItemsViewNtfCode ntf_code, TScreenItemsViewMouseNotify *event_info);
	void		ProcessRightMouseClick(TScreenItemsViewMouseNotify *event_info);

	TScreenItemsViewObject			m_view_helper;

	TLangViewSourceFileStyle			m_view_style;
	TLangViewSourceFileScreenItem		m_file_item;

	TMidLevScanner					*m_active_scanner;

private:

	TColor							m_items_view_bkgr_color;
	TColor							m_items_view_bkgr_color_no_file;

	bool								m_show_hide_scrollers_horz;
	bool								m_show_hide_scrollers_vert;
										// When value of the field is TRUE for the corresponding direction, then scrollers are
										// present only when the file is loaded regardless of its size. When the file is not present,
										// the scrollers are not visible. When the value of the field is FALSE the scrollers are visible
										// all the time.

	bool								m_send_reset_slct_on_click;
	bool								m_send_encl_loc_on_dblclk;
										// Once these flags are to TRUE, they instruct processing single and/or double clicks
										// in a special way.
};

#endif	// LangView_SourceView_H


