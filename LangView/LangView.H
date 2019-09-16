//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_LangView_H
#define	LangView_LangView_H

#ifndef   WinUi_DialogBasedConsole_H
#include  "WinUi/DialogBasedConsole.H"
#endif
#ifndef   WinUi_ListBoxObject_H
#include  "WinUi/ListBoxObject.H"
#endif
#ifndef   ScreenItems_ScreenItemsView_H
#include  "ScreenItems/ScreenItemsView.H"
#endif
#ifndef   ScreenItems_ScreenItemsTree_H
#include  "ScreenItems/ScreenItemsTree.H"
#endif
#ifndef   TextScan_MidStore_H
#include  "TextScan/MidStore.H"
#endif
#ifndef   TextScan_CppDatabase_H
#include  "TextScan/CppDatabase.H"
#endif
#ifndef   TextScan_GrammarStructure_H
#include  "TextScan/GrammarStructure.H"
#endif

//  - - - - - - - Events Browsing Infrastructure - - - - - - - - - -

//
//  Possible types of the click event.
//
enum TLangViewClickEventType
{
	clet_none,					// Prepared event cannot have the event type "none". This value can
								// only be used in the empty click event structure.
	clet_pars_session,			// Parsing session as it is understood by the mid lev events database.
	clet_filesys_directory,		// This is a directory in the filesystem.
	clet_file_object,				// File that is either a part of the MidLevStore or simply an any file.
	clet_indexing_key,			// Indexing key as a top level object of the mid lev events database.

	clet_code_area,				// Place in the code during compilation or some place in any src file.
	clet_indexing_key_use,		// Indexing key use in some particular place in the file.
	clet_cmdl_lexema,			// Raw lexema from the file with the command line syntax.
	clet_clang_lexema,			// Raw lexema from the file with the C/C++/C# style syntax.

	clet_mid_lev_event,			// Preprocessor event or a reference to a language level object.
	clet_grbr_object,			// Some object in the grammar definition space. These objects do not
								// have any relation to any place in the source code.
	clet_grm_pars_symbol,		// Partially parsed object in the grammar based parsing.
								// This can be either a lexema or a non terminal of the grammar.
	clet_cpp_object,				// Object from the C/C++ database.

	clet_num_types,
};

//
// Event subtype for the clet_grbr_object event.
//
enum TGrammarBrowserEventType
{
	grbr_evt_grammar,				// igrammar.
	grbr_evt_symbol,				// Symbol value. This can be either a terminal symbol or a non terminal.
	grbr_evt_grammar_rule,			// -1/irule.
	grbr_evt_parsing_state,			// -1/istate.
	grbr_evt_xpct_conflict,			// -1/ixpct.
	grbr_evt_grammar_conflict,		// -4/-3/-2/-1/iconflict.
	grbr_evt_conflicting_action,		// iconflict, iaction.
	grbr_evt_derivation_path,		// iconflict, iaction, ipath.
};

struct TLangViewClickEvent
{
	TLangViewClickEventType		type;			// This field is valid for any type of event.

	union
	{
		// Group 1. Top level database object and directory infos.
		CtvCompilationInfo			pars_sess_data;
		TFileInfo					file_data;
		TKeyInfo					key_data;

		// Group 2. Code areas and raw lexemas.
		TSourceLocation			code_area;
		TKeyUseInfo				key_use_data;
		TCmdlLexemaInfo			cmdl_lexema;
		TLexemaInfo				clang_lexema;

		// Group 3. Parsing events.
		CtvParsingEventInfo				mldb_event_data;

		// Group 4. Grammar space objects.
		struct
		{
			TGrammarBrowserEventType	grbr_evt;
			WORD						grbr_grm_index;			// Grammar index is relevant for all event subtypes.
			WORD						grbr_obj_inx;
			WORD						grbr_sub_obj_inx;
			int							grbr_path_index;
		};

		// Group 5. Language level objects.
		TCppItemBase			*cpp_item;
	};

	void Clear()
	{
		memset(this, 0, sizeof(TLangViewClickEvent));
	}

	void SetSourceLocationEvent(const TSourceLocation &area)
	{
		type = clet_code_area;
		code_area = area;
	}

	void SetMidLevEvent(const CtvParsingEventInfo &evt)
	{
		type = clet_mid_lev_event;
		mldb_event_data = evt;
	}

	void SetGrammarBrowserEvent(TGrammarBrowserEventType evt, WORD grm_index, WORD obj_inx = 0xFFFF, WORD sub_obj_inx = 0xFFFF, int path_index = -1)
	{
		type = clet_grbr_object;
		grbr_evt = evt;
		grbr_grm_index = grm_index;
		grbr_obj_inx = obj_inx;
		grbr_sub_obj_inx = sub_obj_inx;
		grbr_path_index = path_index;
	}

	bool GetSrcLocation(TSourceLocation &loc) const
	{
		if (type == clet_code_area)
		{
			loc = code_area;
			return(TRUE);
		}
		else if (type == clet_indexing_key_use)
		{
			loc = key_use_data.origin;
			return(TRUE);
		}
		else if (type == clet_cmdl_lexema)
		{
			cmdl_lexema.GetOrigin(loc);
			return(TRUE);
		}
		else if (type == clet_clang_lexema)
		{
			loc = clang_lexema.origin;
			return(TRUE);
		}

		// Events of all other types do not contain the source location info.
		return(FALSE);
	}

	bool GetLinearArea(ID &parsing_id, TLinearArea &area) const
	{
		TSourceLocation src_loc;
		if (GetSrcLocation(src_loc) == TRUE)
		{
			parsing_id = src_loc.parsing_id;
			area = src_loc.lin_area;
			return(TRUE);
		}
		else  if (type == clet_mid_lev_event)
		{
			parsing_id = mldb_event_data.hdr.parsing_id;
			area = mldb_event_data.hdr.area;
			return(TRUE);
		}

		// Events of all other types do not contain the linear area info.
		return(FALSE);
	}
};

class TLangViewBrowserFrame;

enum TLangViewChildBrowserType
{
	cbrt_none,

	// Simple browser.
	cbrt_src_primary,
	cbrt_lex_src_nest,
	cbrt_cond_pars_nest,
	cbrt_src_nest_ctx,
	cbrt_events_list,
	cbrt_event_details,
	cbrt_event_dtls_ctx,

	// Grammar browser.
	cbrt_grbr_rules_grid,

	// Visual parser.
	cbrt_vpars_syntax_tree,

	// C/C++ name tables.
	cbrt_cpp_items_list,

	// Other browsers.
	cbrt_mid_lev_events_tree,
	cbrt_cpp_items_tree,

	// Simple browser - continuation.
	cbrt_global_files_list,
	cbrt_global_files_tree,
	cbrt_indexing_keys_list,
	cbrt_cpp_src_areas_list,					// List of the src areas that belong to certain cpp database item.
};

class TLangViewChildBrowser : public TBasicDialog
{
public:

	TLangViewChildBrowser(TMidLevStoreReader *db_link, TLangViewChildBrowserType browser_type, short dialog_template_ident)
				: TBasicDialog(NULL, dialog_template_ident) { m_type = browser_type; m_owner = NULL; m_db = db_link; }

	TLangViewChildBrowserType GetType() const { return(m_type); }

	void			SetOwner(TLangViewBrowserFrame *owner) { m_owner = owner; }
	virtual void	SetDataBase(TMidLevStoreReader *db_link) { if (db_link != m_db) { ResetContents(); m_db = db_link; } }

	//
	//	Note:	Loading contents of the browser is a browser type specific action.
	//			That is why it is not defined here, in the base class.
	//

	virtual void	ResetContents() = 0;

	virtual void	SetSelection(TLangViewClickEvent &event, bool show_upper_lower_ext_margins) = 0;
	virtual void	ResetSelection() = 0;

	virtual void	SetSortOrder(int list_id, DWORD new_sort_order) { }

protected:

	void			ResetUserCancel();
	void			SendForProcessingFromListBox(TListBoxNotificationCode notificationCode, TLangViewClickEvent &info);
	void			SendForProcessingFromScreenItemsView(TScreenItemsViewNtfCode notificationCode, TLangViewClickEvent &info);

	enum TSIVMR { sivmr_none, sivmr_reload_view, sivmr_copy_to_clipborad };

	TSIVMR		ProcessScreenItemsViewControlMenu(TScreenItemsViewObject &view_helper, TTreeItemsPlacingMode &placing_mode, long click_x, long click_y);

	typedef void (*ClipboardImageGenCallback)(void *context, HDC hMemoryDC, SIZE &bitmap_size);

	void			PutScreenShotToClipboard(TScreenItemsViewObject &items_view, RECT *margins_rect, ClipboardImageGenCallback extra_cbk = NULL, void *cbk_context = NULL);

protected:

	TLangViewChildBrowserType			m_type;

	TLangViewBrowserFrame			*m_owner;
	TMidLevStoreReader				*m_db;					// The pointer is not owned.
};

//
//  The TLangViewBrowserFrame features:
//
//	1.	Root of the hierarchy can have any memory allocation class. Child windows can stay only
//		in the heap memory they they are auto released ("delete this") when the window is closed.
//
//	2.	Browser frame can be used as an error callback for the MidLevDataStore and similar objects.
//
class TLangViewBrowserFrame : public TBasicDialog
{
public:

		TLangViewBrowserFrame(TLangViewBrowserFrame *parent_handler, TMidLevStoreReader *db_link, short resource_template_id);
		~TLangViewBrowserFrame() { if (m_parent != NULL) m_parent->m_child_browsers.RemoveItem(this); }

	virtual bool	OnWmDestroy();
					// Derived classes typically should not override this method. If the current instance is a child browser, this method
					// notifies the parent and deletes the instance.

	virtual void	ChildDestructionNotification(TBasicDialog *inst) { }
					// This method is called when one of the child windows is being closed. When child viewer is deleted and its window
					// is not opened, this method is not called. Derived class should perform appropriate cleanup.

	void CloseChildBrowsers()
	{
		// This class expects that child browsers should be created on the heap only.
		while (m_child_browsers.IsEmpty() == FALSE)
			delete m_child_browsers.GetFirst();
	}

	TMidLevStoreReader	*DataBase() { return(m_db); }
	TGenericConsole		&Console() { return(m_frame_console); }

	virtual void SetupDataBase(TMidLevStoreReader *db_link)
	{
		if (m_db != NULL)
		{
			// Reset callback in the existing store.
			m_db->SetupErrorCallback(NULL);
		}

		m_db = db_link;
		if (m_db != NULL)
		{
			// Set the current dialog as a destination for the errors and events.
			m_db->SetupErrorCallback(&m_frame_console);
		}
	}

	void AttachReader(TMidLevStoreReader *db_link)
	{
		db_link->SetupErrorCallback(&m_frame_console);
	}

	void SetUserCancel(bool new_val)
	{
		m_frame_console.SetAbortFlag(new_val);
	}

	virtual bool	IsGrammarBrowser() const { return(FALSE); }
					// Group of methods for detecting availability of the additional functionality.

	virtual void	ProcessSingleClick(TLangViewChildBrowser *inst, TLangViewClickEvent &event_info) = 0;
	virtual void	ProcessDoubleClick(TLangViewChildBrowser *inst, TLangViewClickEvent &event_info) = 0;
	virtual void	RemoveSelection(TLangViewChildBrowser *inst) = 0;
	virtual void	ProcessResortEvent(TLangViewChildBrowser *inst, int list_id, DWORD new_sort_order) { assert(FALSE); }

	virtual void	PlayHistoryTo(int new_history_step) { assert(FALSE); }

protected:

	class TLangViewBrowserConsole : public TGenericConsole
	{
	public:

		TLangViewBrowserConsole() { m_owner_frame = NULL; }

		virtual int ShowRunTimeError(const wchar_t *message, int buttons = CONS_BTN_OK, bool inc_errors_count = TRUE)
		{
			assert(m_owner_frame != NULL);

			if (inc_errors_count == TRUE)
				BumpErrorsCount();

			// Use owner frame as a parent for the message box.
			return(m_owner_frame->MsgBox(message, buttons, 0, 10));
		}

		virtual bool	IsUserResponseSupported() const { return(TRUE); }

	protected:

		TBasicDialog		*m_owner_frame;

		friend class TLangViewBrowserFrame;
	};

protected:

	TLangViewBrowserFrame		*m_parent;				// When this data field is NULL, this means that the cuurent instance is the root of
														// the hierarchy. This also means that its error handling data fields will be used.
	TList							m_child_browsers;		// Instances of TLangViewBrowserFrame class only.

	TMidLevStoreReader			*m_db;

	TLangViewBrowserConsole		m_frame_console;
};

class TLangViewConsole : public TDialogBasedConsole
{
public:

	TLangViewConsole(HINSTANCE hInst = NULL, const wchar_t *console_title = NULL);

	virtual void	PrepareStatusPanel(const wchar_t *initial_main_msg = NULL, bool want_control_button = TRUE);
	virtual void	CloseStatusPanel();

	void			CheckOpenStatusPanel();
	void			OpenStatusPanelSpecial();

	virtual void	SetMajorStatus(const wchar_t *msg = NULL);
	virtual void	SetMinorStatus(const wchar_t *msg = NULL);

	void			SetMinorStatusAndLastUpdateTime(const wchar_t *msg = NULL);

	bool			UpdateStatusPanel(int num_processed, int num_total, const wchar_t *msg_prefix = NULL);
					// This method opens the console panel only when enough time has passed since it was prepared.
					// Besides that it sets an appropriate minor status. The return value is FALSE when user requests
					// to abort current processing.
protected:

	TDateTime			m_processing_started;
	TDateTime			m_last_update;
	HCURSOR			m_prev_mouse_cursor;
};

//
//  Template: IDD_LANGV_ABOUT_BOX
//
class TLangViewAboutBox : public TBasicDialog
{
public:

	TLangViewAboutBox(const wchar_t *app_name, const wchar_t *app_version, const wchar_t *version_purpose);

	#define COMMUNITY_EDITION	L"Community edition - not for professional use"

	LRESULT		OnInitDialog(HWND hDefFocusControl);

protected:

	void		SetLabelColor(int rcId) { SetControlColor(rcId, RGB(0, 0, 64)); }

	wchar_t			*m_app_name;			// A static string is expected.
	wchar_t			*m_app_version;		// A static string that should include the word "version" is expected.
	wchar_t			*m_version_purpose;	// A static string with an arbitrary contents.
};

#endif	// LangView_LangView_H


