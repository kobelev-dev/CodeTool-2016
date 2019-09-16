//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_SimpleBrowser_H
#define	LangView_SimpleBrowser_H

#ifndef   WinUi_DialogsDock_H
#include  "WinUi/DialogsDock.H"
#endif
#ifndef   LangView_MidLevEventsTree_H
#include  "LangView/MidLevEventsTree.H"
#endif
#ifndef   LangView_CppItemsTree_H
#include  "LangView/CppItemsTree.H"
#endif
#ifndef   LangView_CppDefinitionsView_H
#include  "LangView/CppDefinitionsView.H"
#endif
#ifndef   LangView_SourceView_H
#include  "LangView/SourceView.H"
#endif
#ifndef   LangView_ChildBrowsers_H
#include  "LangView/ChildBrowsers.H"
#endif
#ifndef   LangView_ClickEventDetails_H
#include  "LangView/ClickEventDetails.H"
#endif

enum TLangViewSimpleBrowserEvent
{
	svet_load,
	svet_load_and_select,
	svet_remove_selection,
	svet_resort,
	svet_nesting_select,
	svet_nest_remove_slct,
};

struct TLangViewSimpleBrowserState
{
	TLangViewSimpleBrowserEvent	recent_event;
									// This field is a sort of comment. Event history execution is implemented based on
									// comparison of the current state of the viewer and the requested post condition.

	CtvLexSourceEnterEvent		src_enclosure;
									//
									//   The following situtaions are possible:
									//
									//           hdr                hdr
									//      parsing_id     event_area.file_id
									//  ---------------------------------------------------------
									//		  0			  0			Main source view is empty.
									//		  0			!=0			Raw parsed file.
									//		!=0			  x			Call event describes parsing lex src enclosure.
									//

	TLangViewClickEvent			click_event;
									// Type of event in this field shows if selection is existing. Note that selection may exist
									// but there can be no major source file enclosure if selected event has no source area.

	DWORD						events_sort_order;
									// All the time this field contains some valid value.

	TLangViewChildBrowserType		nesting_slct;
	TLangViewClickEvent			nesting_slct_data;
									// When value of the child browser field is cbrt_none this means that there is no
									// secondary selection.
};

typedef TStructsArray<TLangViewSimpleBrowserState, 256, 256> TLangViewSimpleBrowserHistory;

//
//  Template: IDD_LANGV_SVF_HISTORY_CONTROL
//
class TLangViewBrowserHistoryControlDialog : public TBasicDialog
{
public:

	TLangViewBrowserHistoryControlDialog();

	LRESULT		OnInitDialog(HWND hDefaultFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	void			SetOwner(TLangViewBrowserFrame *owner) { m_owner = owner; }
	void			SetState(int new_curr_step, int new_total_steps);

protected:

	int						m_curr_step, m_total_steps;
								// Internally steps are numbered starting from 0, but on the screen they are displayed
								// as numbered from 1.

	TLangViewBrowserFrame	*m_owner;
};

//
// This structure is used when it is necessary to pass information about parsers that can be called
// from inside the TLangViewSimpleBrowser dialog.
//
struct TSimpleBrowserParsingDriverInfo
{
	TParsingDriver		*m_loc_sess_driver;

	wchar_t				*m_loc_sess_menu_item_text;
	wchar_t				*m_loc_sess_file_open_title;

	wchar_t				*m_loc_sess_base_dir;					// If the value is NULL, then the current dir of the process is used.
	wchar_t				*m_loc_sess_cmdl_format;				// Param should contain format with one "%s" parameter or NULL.
	TParseEnvInfo			*m_loc_sess_cmdl_env_info;				// This param should be not NULL only if the cmdl_format is not NULL.
};

//
//  Template: IDD_LANGV_SVF_MAIN_FRAME
//
class TLangViewSimpleBrowser : public TLangViewBrowserFrame
{
public:

	TLangViewSimpleBrowser(TLangViewBrowserFrame *parent_handler, TMidLevStoreReader *db_link, TSimpleBrowserParsingDriverInfo *drivers_array = NULL, int num_drivers = 0);
					// The database reader is passed in the "use only" mode.

	void			SetupDataBase(TMidLevStoreReader *db_link);
					// The database reader is passed in the "use only" mode.

	void			SetupTask(ID parsing_id, PSET_TYPES_MASK events_mask, TCppDatabase *cpp_db = NULL) { assert(m_hDlg == NULL); m_task_parsing_id = parsing_id; m_task_events_mask = events_mask; }
					//
					//  This method should be called before opening the dialog. It allows filling in the initial view when the dialog
					//  opens. Passing parsing_id equal to 0 means asking for the latest parsing session.
					//

	LRESULT		OnInitDialog(HWND hDefaultFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);
	bool			OnWmDestroy();

	void		ProcessSingleClick(TLangViewChildBrowser *event_sender, TLangViewClickEvent &event_info);
	void		ProcessDoubleClick(TLangViewChildBrowser *event_sender, TLangViewClickEvent &event_info);
	void		RemoveSelection(TLangViewChildBrowser *event_sender);
	void		ProcessResortEvent(TLangViewChildBrowser *event_sender, int list_id, DWORD new_sort_order);
	void		PlayHistoryTo(int new_curr_history_step) { }

protected:

	void		SetupClickEvent(TLangViewClickEvent &event_info, bool show_margins);
	void		SetupSecondaryViewers(TLangViewClickEvent &event_info);
	void		SetupSourceCodeViewer(TLangViewSourceFileDialog &browser, TLangViewClickEvent &event_info);


	//
	// Local parsing sessions support.
	//

	TSimpleBrowserParsingDriverInfo				*m_external_drivers_info;
	int										m_num_external_drivers;
	TFileOpenHelper							m_ofn_source_file;

	//
	// Dock helpers.
	//

	TVertBasicDialogsDockDialog				m_main_vert_dock;
	THorzBasicDialogsDockDialog				m_upper_horz_dock;
	THorzBasicDialogsDockDialog				m_lower_horz_dock;

	TVertBasicDialogsDockDialog				m_upper_middle_dock;
	TVertBasicDialogsDockDialog				m_lower_left_dock;
	TVertBasicDialogsDockDialog				m_lower_right_dock;

	//
	// Child browsers.
	//

	TLangViewMidLevTreeDialog					m_events_tree;
	TLangViewCppItemsTreeDialog				m_cpp_items_tree;
	TLangViewParsingEventsListDialog			m_cpp_item_locs_list;
	TLangViewSourceFileDialog					m_source_view;
	TLangViewCppDefnsSpaceViewer				m_cpp_items_list;

	TLangViewLexSourcesNestingDialog			m_lex_src_nesting;
	TLangViewCondParseNestingDialog			m_cond_pars_nesting;
	TLangViewSourceFileDialog					m_nesting_context;
	TLangViewParsingEventsListDialog			m_events_list;
	TLangViewClickEventDetailsDialog			m_slct_details;
	TLangViewSourceFileDialog					m_slct_context;

	//
	// Browsing history related fields.
	//

	TLangViewBrowserHistoryControlDialog		m_brows_hist_control;
	TLangViewSimpleBrowserHistory				m_history;
	int										m_curr_hist_step;				// Zero based index of the history record.

	//
	// Opening task related fields.
	//

	ID										m_task_parsing_id;				// Zero id refers to the latest parsing sessison.
	PSET_TYPES_MASK						m_task_events_mask;
												// The task is not empty when the events mask is not zero.
												// Local parsing session is modifying both fields.

	TCppDatabase							*m_curr_cpp_database;
												// When this data member is not NULL, the viewer is opened in the cpp language layout.
												// Local parsing session is updating this field.
};

#endif	// LangView_SimpleBrowser_H


