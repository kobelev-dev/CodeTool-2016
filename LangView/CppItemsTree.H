//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_CppItemsTree_H
#define	LangView_CppItemsTree_H

#ifndef   TextScan_CppDatabase_H
#include  "TextScan/CppDatabase.H"
#endif
#ifndef   LangView_LangView_H
#include  "LangView/LangView.H"
#endif
#ifndef   LangView_LexItems_H
#include  "LangView/LexItems.H"
#endif

//
//  Template: IDD_LANGV_CHLD_CPP_ITEMS_TREE
//
class TLangViewCppItemsTreeDialog : public TLangViewChildBrowser
{
public:

	TLangViewCppItemsTreeDialog();
	~TLangViewCppItemsTreeDialog();

	LRESULT		OnInitDialog(HWND hDefaultFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	void			LoadCppItems(TCppItemBase &items_subtree_root);
					// Display the passed item as the root of the tree and display its children.

	void			AcceptCppItemsTree(TLangViewCppItemTreeNode &visible_lex_items_root);
					// Dispaly external tree of CppItems.

	//
	// Inherited virtual methods.
	//

	virtual void	ResetContents();
	virtual void	SetSelection(TLangViewClickEvent &event, bool show_margins);
	virtual void	ResetSelection();

protected:

	bool			LoadCppItemChildren(TGenericTreeNode *owner_tree_node, TCppItemBase *owner_database_item);
	bool			LoadCppItemFrontReferences(TLangViewCppItemTreeNode *item_node);
	bool			LoadCppItemBackReferences(TLangViewCppItemTreeNode *item_node);

	bool			CreateCppItemOptionalLink(TLangViewCppItemTreeNode *item_node, TCppItemBase *referenced_item);
	bool			CreateCppItemMandatoryLink(TLangViewCppItemTreeNode *item_node, TCppItemBase *referenced_item, const wchar_t *missing_link_name);
	bool			CreateCppItemDimObject(TLangViewCppItemTreeNode *item_node, TCppItemBase *dim_object_proto);

	void			ProcessControlMenu(long click_pos_x, long click_pos_y);
	void			ShowCurrentContents();

	void			ResetViewInternal();
	void			CreateAndShowSelectionObject(TLangViewTreeNode *node);

protected:

	TLangViewLexItemStyle					m_lex_items_style;
	TTextStyle							m_message_style;

	TScreenItemsViewObject				m_citems_view_helper;
	TTreeItemsPlacingMode					m_placing_mode;

	TLangViewCppItemTreeNode				m_citems_tree_root;
	TTextItem							m_message;

	TLangViewTreeNode					*m_selected_node;			// This pointer, if it is not NULL, points to some object in the local tree.
	TScreenItem							*m_selection_object;		// This is either a Rect or a Circle selection object.
};

#endif	// LangView_CppItemsTree_H


