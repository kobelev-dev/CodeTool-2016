//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_MidLevEventsTree_H
#define	LangView_MidLevEventsTree_H

#ifndef   TextScan_MidStore_H
#include  "TextScan/MidStore.H"
#endif
#ifndef   LangView_LangView_H
#include  "LangView/LangView.H"
#endif
#ifndef   LangView_LexItems_H
#include  "LangView/LexItems.H"
#endif

//
//  Template: IDD_LANGV_CHLD_MID_LEV_TREE
//
class TLangViewMidLevTreeDialog : public TLangViewChildBrowser
{
public:

	TLangViewMidLevTreeDialog(TMidLevStoreReader *db_link);
	~TLangViewMidLevTreeDialog();

	LRESULT		OnInitDialog(HWND hDefaultFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	void			LoadParsingEvents(ID pars_id, PSET_TYPES_MASK event_types_mask, ID src_ref = 0, ID min_evid = 0, ID max_evid = 0);
	void			PrepareIncludeFilesReport(ID pars_id, bool add_hidden_inc_events, LERRC_TYPES_MASK err_warn_classes_mask = LERRC_ERRORS);

	//
	// Inherited virtual methods.
	//

	virtual void	ResetContents();
	virtual void	SetSelection(TLangViewClickEvent &event, bool show_margins);
	virtual void	ResetSelection();

protected:

	void			ProcessControlMenu(long click_pos_x, long click_pos_y);
	void			ShowCurrentContents();
	void			ResetViewInternal();
	void			CreateAndShowSelectionObject(TLangViewTreeNode *node);
	void			UpdateScreenShotForClipboard(HDC hMemoryDC, SIZE &image_size) { }

	static void UpdateScreenShotCallback(void *context, HDC hMemoryDC, SIZE &image_size)
	{
		TLangViewMidLevTreeDialog *inst = (TLangViewMidLevTreeDialog*)context;
		inst->UpdateScreenShotForClipboard(hMemoryDC, image_size);
	}

protected:

	TLangViewLexItemStyle					m_lex_items_style;
	TTextStyle							m_message_style;

	TScreenItemsViewObject				m_mlev_view_helper;
	TTreeItemsPlacingMode					m_placing_mode;

	TLangViewSpecialTreeNode				m_mlev_tree_root;
	TTextItem							m_message;

	TLangViewTreeNode					*m_selected_node;				// This pointer, if it is not NULL, points to some object in the local tree.
	TScreenItem							*m_selection_object;			// This is Rect or Circle selection object.
};

#endif	// LangView_MidLevEventsTree_H


