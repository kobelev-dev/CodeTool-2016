//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	ScreenItems_ScreenItemsTree_H
#define	ScreenItems_ScreenItemsTree_H

#ifndef   _INC_MATH
#define    _USE_MATH_DEFINES		// This definition is needed to pick up the M_PI from math.h.
#include  <math.h>
#endif

#ifndef   Common_Buffers_H
#include  "Common/Buffers.H"
#endif
#ifndef   Common_DoubleLinkedList_H
#include  "Common/DoubleLinkedList.H"
#endif
#ifndef   Common_GenericConsole_H
#include  "Common/GenericConsole.H"
#endif
#ifndef   Common_DateTimeHelper_H
#include  "Common/DateTimeHelper.H"
#endif

#ifndef   ScreenItems_ScreenItemsView_H
#include  "ScreenItems/ScreenItemsView.H"
#endif

#define TREE_ITEMS_PLACER_MAX_TREE_DEPTH  50000
			// This value is not a hard limitation like the size of some array. It is used to avoid things
			// like infinite loops that may happen in the case of broken data sets.

//
//  These are possible types of displaying a tree-like structures. Different types of placing are supported
//  by different placers. Each placer has its own placing properties.
//
enum TTreeItemsPlacingMode
{
	plc_mode_concentric,
	plc_mode_rect_lines,
};

enum TConcentricPlacingModeParamIndex
{
	cnct_node_diameter,				// Tree item decides itself what diameter it should have.
									// Placing and painting code simply picks up the diameter and uses it.
	cnct_num_types,
};

enum TRectLinesPlacingModeParamIndex
{
	rcln_none,

	rcln_size_x,						// Tree item decides itself what width and height it should have.
	rcln_size_y,						// Placing and painting code simply picks up the size and uses it.

	rcln_upper_link_offset,
	rcln_chld_links_offset,
	rcln_chld_links_step,

	rcln_num_types,
};

//
//  When some layer has items that have different heights, the height of the items stripe is set to
//  the height of the tallest item. The items are vertically centered on the stripe of their layer.
//
struct TRectLinesModePlacingProps
{
	int		horz_step_inside_family;
	int		horz_step_between_families;

	int		off_top;
	int		off_bottom;
	int		vert_lines_step;

	bool		up_side_down_placing;
};

//
//  This is a generic node that forms the tree of abstract nodes. Derived classes define visual
//  representation of the nodes. The base class paints only the link to its parent node.
//
class TGenericTreeNode : public TListItem
{
public:

		TGenericTreeNode(TGenericTreeNode *owner = NULL, BYTE poly_inx = 0) { m_owner = owner; if (owner != NULL) owner->m_children.AppendItem(this); m_flags = (etgi_visible | etgi_owned); m_poly_inx = poly_inx; }
			// Parameter poly_inx should be used for identifying the type of derived class. Its primary use is an index into
			// the array of context pointers in the placer.

		~TGenericTreeNode() { ReleaseChildren(); }
			// When the whole tree is deleted, noting special is needed. Nevertheless when some item or a subtree should be destroyed,
			// then this item or subtree should be removed from the tree first and only then deleted with operator delete.

	void			SetOwnedFlag(bool value) { if (value == TRUE) m_flags |= etgi_owned; else m_flags &= ~etgi_owned; }
					// If object is not owned by its parent, the object itself and all its subtree will not be automaticaly deleted
					// when root of the tree is deleted.

	void			AdoptChild(TGenericTreeNode *child_node, TGenericTreeNode *insert_before = NULL, bool bypass_ownership = TRUE);
	void			AcceptChildren(TGenericTreeNode &other_node);

	void			RemoveChild(TGenericTreeNode *child_node) { assert(m_children.IsItemInList(child_node) == TRUE); m_children.RemoveItem(child_node); child_node->m_owner = NULL; }
	void			ReleaseChildren();

	enum ChildrenVisibilityState
	{
		cvs_no_chilren,
		cvs_all_hidden,			// All children of the node are hidden.
		cvs_mixture,			// Some children are visible, some children are hidden.
		cvs_all_visible,			// All children of the node are visible.
		cvs_num_states,
	};

	bool					GetVisiblityState() const { return(((m_flags & etgi_visible) != 0) ? TRUE : FALSE); }

	ChildrenVisibilityState	GetChildrenVisibilityState();
	void					SetChildrenVisibilityState(bool new_value);
							// These methods work only with direct children of the object.

	int					BalanceVisibleChildren();
							// Return value is the weight of the subtree. Weight is the number of items in the subtree including the root.

	int					NumDirectChildren(bool show_all_children) const;
	int					NumSubtreeNodes(bool show_all_children) const;
							// The count of subtree nodes includes the root node.

	TGenericTreeNode		*GetParentNode() { return(m_owner); }

	TGenericTreeNode		*GetFirstChild(bool show_all_children);
	TGenericTreeNode		*GetLastChild(bool show_all_children);
	TGenericTreeNode		*GetNextSibling(bool show_all_children);
	TGenericTreeNode		*GetPrevSibling(bool show_all_children);

	BYTE		GetPolyInx() const { return(m_poly_inx); }

	void			GetPosition(long &pos_x, long &pos_y) { pos_x = m_pos_x; pos_y = m_pos_y; }
	void			SetPosition(long pos_x, long pos_y) { m_pos_x = pos_x; m_pos_y = pos_y; }
					// For concentric mode position is position of the center of the circle. For rect lines mode position
					// is the left-top corner of the base rect of the main rect of the tree node. When application wants
					// to place something outside of the main rect, it should handle this using off_top, off_bottom fields
					// of the rect lines placing props.
public:

	virtual int		GetChildrenBalancingSeed()  { return(0); }
	virtual void	PrepareForPlacing(HDC hDC, TTreeItemsPlacingMode mode, bool root_object_flag, void *context) = 0;

	virtual int		GetConcentricModePlacingParam(HDC hDC, TConcentricPlacingModeParamIndex inx, void *context) = 0;
	virtual int		GetRectLinesModePlacingParam(HDC hDC, TRectLinesPlacingModeParamIndex inx, TRectLinesModePlacingProps &props, void *context) = 0;
					// Placing params can be queried only when the object is already prepared for placing using the PrepareForPlacing() method.

	virtual void	PrepareConcentricModeScreenItems(HDC hDC, void *context) = 0;
	virtual void	PrepareRectLinesModeScreenItems(HDC hDC, TRectLinesModePlacingProps &props, void *context) = 0;
					// Derived class should prepare all screen items that it wants to display inside these methods.

	virtual void	ShowLinkScreenItems(TTreeItemsPlacingMode mode, TList &screen_items) = 0;
	virtual void	ShowBkgrScreenItems(TTreeItemsPlacingMode mode, TList &screen_items) = 0;
	virtual void	ShowFrgrScreenItems(TTreeItemsPlacingMode mode, TList &screen_items) = 0;

public:

	bool			ShiftSubtree(long shift_x, long shift_y);

	TLineStyle	*GetTreeLinkStyle() { return(m_tree_link_item.Style()); }

	void			PrepareConcentricModeUpperLinkItem(HDC hDC, TLineStyle *link_style, void *context);
	void			PrepareRectLinesModeUpperLinkItem(HDC hDC, TLineStyle *link_style, TRectLinesModePlacingProps &props, void *context);

	void			ShowPreparedItem(TList &items_list, TScreenItem &item) { if (item.GetBasicStyle() != NULL) { item.Clear(); items_list.AppendItem(item); } }

protected:

	TGenericTreeNode		*m_owner;				// Poniter to item of the higher level or NULL for the root item.
	TList					m_children;

	enum { etgi_visible = 1, etgi_owned = 2 };

	BYTE				m_flags;

	BYTE				m_poly_inx;				// This index is useful when the tree contains objects, created using different classes.

	long					m_placer_data[2];
	long					m_pos_x, m_pos_y;

	TLinesChainItem3		m_tree_link_item;		// One section of this chain of lines is needed for concentric mode and either one
												// section or three sections for the rect lines mode.

	friend class TGenericTreeNodesIterator;
	friend class TConcentricModeTreeGraphPlacer;
	friend class TRectLinesModeTreeGraphPlacer;

	template <class> friend class TGenericTreeNodeDirectChildrenIter;
};

//
//  This iterator iterates one layer of children of any tree node object. Typical use of this iterator
//  in the app code should look like:
//
//		for (TGenericTreeNodeDirectChildrenIter<SomeNodeItemClass> iter(some_node); iter; ++iter)
//		{
//			SomeNodeItemClass &item = iter.CurrItem();
//			item.SomeMethod();
//		}
//
//	This iterator works fine when "some_node" does not have children.
//
template <class nodeClass = TGenericTreeNode>
class TGenericTreeNodeDirectChildrenIter
{
public:

	inline	TGenericTreeNodeDirectChildrenIter(TGenericTreeNode &item) { m_parent_item = &item; m_current_child = (TGenericTreeNode*)item.m_children.GetFirst(); }
	inline	TGenericTreeNodeDirectChildrenIter(TGenericTreeNode *item) { m_parent_item = item; m_current_child = (TGenericTreeNode*)item->m_children.GetFirst(); }

	inline	operator		bool() const			{ return(m_current_child != NULL); }
	inline	bool				IsComplete() const	{ return(m_current_child == NULL); }

	inline	nodeClass	&CurrItem() const		{ return((nodeClass&)(*m_current_child)); }
	inline	nodeClass	*CurrItemPtr() const	{ return((nodeClass&)m_current_child); }
								// These methods return valid reference/pointer only when iterator is not in the "end-of-iteration" state.

	inline	void		operator ++()		{ m_current_child = (TGenericTreeNode*)m_parent_item->m_children.GetNext(m_current_child); }
	inline	void		ShiftDown()		{ m_current_child = (TGenericTreeNode*)m_parent_item->m_children.GetNext(m_current_child); }

	inline	void		SeekToFirst()							{ m_current_child = (TGenericTreeNode*)m_parent_item->m_children.GetFirst(); }
	inline	void		SeekToItem(TGenericTreeNode *item)		{ m_current_child = item; }
						// Last method should be used with care because it assumes that passed item is a valid direct child.
protected:

	TGenericTreeNode			*m_parent_item;
	TGenericTreeNode			*m_current_child;
};

//
//  Basic placer is essentially a wrapper around the console object.
//
class TBasicTreeGraphPlacer
{
protected:

	TBasicTreeGraphPlacer(TGenericConsoleEx *pCons = NULL) { m_console = pCons; m_prev_placing_layer_display_time = 0; m_tracing_active = FALSE; }

	void		ShowMajorStatus(const wchar_t *msg);
	void		ShowCurrentPlacingLevel(int level, int highest_existing_level);
	void		ClearMinorStatus();

	void		ShowErrorMessage(const wchar_t *msg);
	void		ShowOutOfMemoryError();
				// These methods show passed msg on the popup dialog and write it to the debug logger.

	void		EnableTracing(bool value) { m_tracing_active = value; }

	void		DebugTrace(const wchar_t *format, ...);
				// This method sends tracing to the console if the console is present.
private:

	TGenericConsoleEx		*m_console;

	TDateTime			m_prev_placing_layer_display_time;
	bool					m_tracing_active;

	friend class TGenericTreeNodesIterator;
	friend class TConcentricModeTreeGraphPlacer;
	friend class TRectLinesModeTreeGraphPlacer;
};

enum TNodesIterationDirection
{
	ndi_dir_backward,
	ndi_dir_forward,
};

class TGenericTreeNodesIterator
{
public:

		TGenericTreeNodesIterator(TGenericTreeNode *root_node = NULL, bool iterate_all_children = FALSE, TBasicTreeGraphPlacer *placer = NULL);
				//
				// Note that passed root is not iterated. Iterator iterates only the children of the passed root. It is ok to pass
				// the placer as NULL. Iteration success flag can be used to figure out the iteration success.
				//

	void		SetPlacer(TBasicTreeGraphPlacer *placer)		{ m_placer = placer; }
	void		SetRootNode(TGenericTreeNode *root_node)	{ m_root = root_node; }
	void		ResetIterationError()						{ m_iter_problem = FALSE; }

	void		StartIteration(TGenericTreeNode *start_iteration_with = NULL, int max_node_level = 0, TNodesIterationDirection dir = ndi_dir_forward);
				//
				// After calling this method iteration may start and may not start. This may happen either when the tree
				// is empty (or everything is invisible) or when the starting item cannot be used for starting the iteration.
				// When the iteration is finished, it is good to check its success.
				//

	void		SeekToLevel(int item_level, TNodesIterationDirection dir = ndi_dir_forward);
				//
				// This method assumes that iteration is already started. If current level is equal to requested level, the method
				// is not doing anything. Otherwise method goes up or down the tree to find the item that matches the requested
				// level. Note that iteration may terminate without finding the appropriate item.
				//

	TGenericTreeNode	*GetCurrentItem() const	{ return(m_nodes[m_iter_level]); }
	int				GetCurrentLevel() const	{ return(m_iter_level); }

	bool				CheckIteration() const		{ return(m_iter_level != 0); }
	void				StepIteration(TNodesIterationDirection dir = ndi_dir_forward);
	void				StepIterationAtLevel(int item_level, TNodesIterationDirection dir = ndi_dir_forward);

	bool				CheckIterationSuccess() const { return(m_iter_problem == FALSE); }

protected:

	void		ReportIterationError(const wchar_t *msg) { if (m_placer != NULL) m_placer->ShowErrorMessage(msg); m_iter_problem = TRUE; }

	typedef  TStructsArray<TGenericTreeNode*, 40, 200>  TGenericTreeNodePtrsArray;

	TGenericTreeNode			*m_root;
	bool						m_iter_all;				// Iterate all items regardless of thier visibility.

	bool						m_iter_problem;			// Note that starting new iteration is not resetting the problem flag.
													// This flag may indicate prooblem in the non recent iteration.
	int						m_iter_level;			// Iteration is active when the current level is greater than zero.
													// Direct children of the root are considered to be at the level 1.
	int						m_iter_max_level;

	TGenericTreeNodePtrsArray	m_nodes;				// Code never sets or checks the number of items in this array.
													// It only controls the size of the array buffer. Valid items are
													// placed at the levels [1] .... [m_iter_level+1].
	TBasicTreeGraphPlacer		*m_placer;				// Placer is needed only for reporting errors.
};

struct TConcentricModePlacingProps
{
	bool			balance_the_tree;
	bool			clock_wise;				// Place objects clockwise.

	int			*min_radius_steps;		// This is an array of one or more intergers that describes min radius steps between the layers.
										// The end of array is marked with a zero element. If array starts from zero, this is equivalent to
										// NULL in this field. Last element of this array is used as min raduis step for all subsequent
										// layers. If array is NULL, then 1.3 multiplied by the nodes arc step is used.
	int			nodes_arc_step;			// This is the distance between the nodes, that belong to the same nodes family measured
										// along the arc. This value is very important and it should not be zero.
	short		placing_direction;		// Direction of the tree growth. When the value is zero, the tree gows left.
										// When the value is -90, the tree grows down.
	short		placing_angle;			// Total width of the corner in graduses.
};

class TConcentricModeTreeGraphPlacer : protected TBasicTreeGraphPlacer
{
public:

	TConcentricModeTreeGraphPlacer(TGenericConsoleEx *pCons = NULL);

	bool			DoConcentricPlacing(HDC hDC, TGenericTreeNode *root_node, TConcentricModePlacingProps &props_data, void **contexts_array,
									int *max_populated_level = NULL, long *max_used_radius = NULL);
					//
					//   Center of the root of the tree is placed at 0,0.
					//

	static bool	DisplayGraph(TScreenItemsViewObject &view_helper, TList &screen_items_list,
								TGenericTreeNode *root_node, TConcentricModePlacingProps &placing_props, void **contexts_array,
								int num_visible_objects_ctrl_id = 0, int *num_visible_objects = NULL, bool show_prepared_tree = TRUE,
								TGenericConsoleEx *pCons = NULL);
protected:

	bool		PlaceLayerInCircularMode(double start_angle, int level, double in_grp_arc_step, double btw_grps_arc_step);
				// Method iterates all nodes of the level and checks for arc overflow.

	bool		PlaceLayerInBissectMode(int level, bool full_circle_mode, bool &family_problem);
				// This method expects that upper level is already positioned.

	bool		PlaceLayerInSparcedMode(int level, TGenericTreeNode *initial_prev_level_item, double tight_koef, bool &first_family_problem);
				// Placing starts from the node with the biggest numbers of children. After that one wave goes to the right and
				// another wave goes to the left. Param tight_koef should  stay in the range 0.0 .. 1.0.

	double	EnlargeRadiuses(int level);
	void		PlaceNode(TGenericTreeNode *item, double radius, double angle);

	double	GetDirectionValue(TGenericTreeNode *dir_to_item, TGenericTreeNode *dir_from_item);
	double	GetDirectionValue(double radius, double angle, TGenericTreeNode *dir_from_item);
				// Return values are angles in the range -pi...pi.

	static double GetAngleValue(double angle_from, double angle_to)
	{
		// Both angles are expected to be normalized (i.e. -pi ... pi).
		double fi_res = angle_from - angle_to;
		if (angle_to > angle_from)
			fi_res += 2*M_PI;
		return(fi_res);
	}

	static double GetDirectionsDifference(double angle1, double angle2)
	{
		// Both angles are expected to be normalized (i.e. -pi ... pi).
		double fi_res = angle1 - angle2;

		if (fi_res > M_PI)
		{
			fi_res = 2*M_PI - fi_res;
		}
		else if (fi_res < 0)
		{
			if (fi_res < -M_PI)
				fi_res += 2*M_PI;
			else fi_res = - fi_res;
		}

		return(fi_res);
	}

	static double NormalizeAngleRightTo(double angle, double angle_to)
	{
		//
		// Satisfy the condition: angle_to-2pi < angle < angle_to.
		//

		while (angle > angle_to)
			angle -= 2*M_PI;
		while (angle < angle_to-2*M_PI)
			angle += 2*M_PI;

		return(angle);
	}

	static double NormalizeAngleLeftTo(double angle, double angle_to)
	{
		//
		// Satisfy the condition: angle_to < angle < angle_to+2pi.
		//

		while (angle < angle_to)
			angle += 2*M_PI;
		while (angle > angle_to+2*M_PI)
			angle -= 2*M_PI;

		return(angle);
	}

	void   PlaceBissectSpecial(TGenericTreeNode *border_with_childen, int level, TNodesIterationDirection dir, double step_ins_grp, double step_btw_grps);
	void   PlaceSparcedSpecial(TGenericTreeNode *family_of_children_beg, int level, double initial_angle, TNodesIterationDirection dir);

	inline TGenericTreeNode *GetFirstChld(TGenericTreeNode *item)		{ return((m_clock_wise == TRUE) ? item->GetFirstChild(FALSE) : item->GetLastChild(FALSE)); }
	inline TGenericTreeNode *GetLastChld(TGenericTreeNode *item)			{ return((m_clock_wise == TRUE) ? item->GetLastChild(FALSE) : item->GetFirstChild(FALSE)); }
	inline TGenericTreeNode *GetPrevNeighbor(TGenericTreeNode *item)	{ return((m_clock_wise == TRUE) ? item->GetPrevSibling(FALSE) : item->GetNextSibling(FALSE)); }
	inline TGenericTreeNode *GetNextNeighbor(TGenericTreeNode *item)	{ return((m_clock_wise == TRUE) ? item->GetNextSibling(FALSE) : item->GetPrevSibling(FALSE)); }

	enum {  NUM_INITAL_LEVELS = 40, EXT_LEVELS_STEP = 120 };

	typedef	TStructsArray<bool, NUM_INITAL_LEVELS, EXT_LEVELS_STEP>				TCnctBoolsArray;
	typedef	TStructsArray<int, NUM_INITAL_LEVELS, EXT_LEVELS_STEP>					TCnctIntsArray;
	typedef	TStructsArray<double, NUM_INITAL_LEVELS, EXT_LEVELS_STEP>				TCnctDoublesArray;
	typedef	TStructsArray<TGenericTreeNode*, NUM_INITAL_LEVELS, EXT_LEVELS_STEP>	TCnctNodePtrsArray;

protected:

	TGenericTreeNode					*m_root;
	TCnctDoublesArray					m_lev_radius;
	TGenericTreeNodesIterator			m_iter;

	int								m_prev_radius_inc_level;
	double							m_prev_radius_inc_value;

	//
	// These fields are not changing during the placing process. They are set at the beginning
	// of placing based on the diameter of the graph node.
	//

	double							m_min_radius_step;				// Step between the arcs when radius is not "population driven".
	int								m_num_radius_step_infos;

	double							m_inside_group_arc_step;		// Linear step along the arc inside the family of children.
	double							m_between_groups_arc_step;	// Linear step along the arc between different families of children.
	bool								m_clock_wise;					// Place objects clockwise.

	TNodesIterationDirection			m_dir_forw, m_dir_backw;		// These are directions of iteraton that are based on the requested direction
																	// of placing nodes around the center.
};

class TRectLinesModeTreeGraphPlacer : protected TBasicTreeGraphPlacer
{
public:

	TRectLinesModeTreeGraphPlacer(TGenericConsoleEx *pCons = NULL);

	bool			DoRectLinesPlacing(HDC hDC, TGenericTreeNode *root_node, TRectLinesModePlacingProps &props_data, void **contexts_array);
					//
					//	Top side of the root is placed at the horz level 0. The left side can have any value, but most likely
					//	it will be small positive value.
					//

	static bool	DisplayGraph(TScreenItemsViewObject &view_helper, TList &screen_items_list,
								TGenericTreeNode *root_node, TRectLinesModePlacingProps &placing_props, void **contexts_array,
								int num_visible_objects_ctrl_id = 0, int *num_visible_objects = NULL, bool show_prepared_tree = TRUE,
								TGenericConsoleEx *pCons = NULL);

protected:

	struct TPcbPlacerLevelInfo
	{
		long		max_item_height;		// Max item height across all items on the level. This is the height
										// of the stripe that is occupied by the tree objects.
		long		max_num_children;		// Max number of children on the level. This value is needed
										// to setup proper spacing between the rows of objects.
		long		vert_pos_nornal;		// Vert pos of the top of the items stripe when the root of the tree is on the top of the graph.
		long		vert_pos_root_down;	// Vertical position of the top of the items stripe.
	};

	inline long	GetWidth(HDC hDC, TGenericTreeNode *item)	{ return(item->GetRectLinesModePlacingParam(hDC, rcln_size_x, m_props, m_poly_inx_contexts[item->m_poly_inx])); }
	inline long	GetHeight(HDC hDC, TGenericTreeNode *item)	{ return(item->GetRectLinesModePlacingParam(hDC, rcln_size_y, m_props, m_poly_inx_contexts[item->m_poly_inx])); }
	inline void	ShowOutOfMemoryErrorEx()			{ ShowOutOfMemoryError(); m_poly_inx_contexts = NULL; }

	long		SetupItemPos(HDC hDC, TGenericTreeNode *item, int lev, long pos_x, long pos_y);
				// Return value is the position of the next item if any on the same horz layer. This position includes
				// the appropriate spacing either inside the family or between the families.

	void		PrepareItemsShift(TGenericTreeNode *start_item, int level_to_iterate, long shift_x);
	bool		FixPotentialOverlap(TGenericTreeNode *item, int item_level, int root_level);
	long		GetCurrHorzPosAndLocalRoot(TGenericTreeNode *item, int item_level, int root_level, TGenericTreeNode *&subtr_root);
	long		GetHorzLinksBaseHorzLevel(TPcbPlacerLevelInfo &clni);

	void		UpdateHorzPositions(TGenericTreeNode *item, long shift_x);
	void		FixChildlessFamilyMembersGaps(TGenericTreeNode *item);
	void		SetupChildLinks(HDC hDC, TGenericTreeNode *item, long base_lev_y);

	TGenericTreeNode					*m_root;
	TGenericTreeNodesIterator			m_iter1, m_iter2, m_iter3, m_iter4;

private:

	TRectLinesModePlacingProps			m_props;
	void								**m_poly_inx_contexts;
};

template <class basic_symbol>
class TGenericTreeNodeEx : public TGenericTreeNode
{
public:

	TGenericTreeNodeEx(TGenericTreeNode *owner = NULL, BYTE poly_inx = 0) : TGenericTreeNode(owner, poly_inx) { }

	virtual	void		SetStyle(TScreenItemStyle *stl) { m_basic_symbol.SetStyle(stl); }
	virtual	void		PrepareForPlacing(HDC hDC, TTreeItemsPlacingMode mode, bool root_object_flag, void *context) { }

	virtual	void		PrepareConcentricModeScreenItems(HDC hDC, void *context);
	virtual	void		PrepareRectLinesModeScreenItems(HDC hDC, TRectLinesModePlacingProps &props, void *context);

	virtual	void		ShowLinkScreenItems(TTreeItemsPlacingMode mode, TList &screen_items);
	virtual	void		ShowBkgrScreenItems(TTreeItemsPlacingMode mode, TList &screen_items);
	virtual	void		ShowFrgrScreenItems(TTreeItemsPlacingMode mode, TList &screen_items);

	virtual	wchar_t		*MainLabelText(void *context) = 0;
	virtual	wchar_t		*SymbolLabelText(void *context) = 0;

protected:

	basic_symbol	m_basic_symbol;
};

template <class basic_symbol>
void TGenericTreeNodeEx<basic_symbol>::PrepareConcentricModeScreenItems(HDC hDC, void *context)
{
	TLineStyle *lineStyle = (TLineStyle*)context;
	assert(lineStyle->StyleTypeId() == TLineStyle::type_ID);
	PrepareConcentricModeUpperLinkItem(hDC, lineStyle, context);

	TScreenItemStyle *basicStyle = m_basic_symbol.GetBasicStyle();
	assert(basicStyle->StyleTypeId() == TBasicSymbolStyle::type_ID);

	TBasicSymbolStyleInfo &style_info = ((TBasicSymbolStyle*)basicStyle)->GetBasicStyleInfo();
	m_basic_symbol.PrepareConcentricModeItems(hDC, style_info, m_pos_x, m_pos_y, MainLabelText(context), -1, SymbolLabelText(context), -1);
}

template <class basic_symbol>
void TGenericTreeNodeEx<basic_symbol>::PrepareRectLinesModeScreenItems(HDC hDC, TRectLinesModePlacingProps &props, void *context)
{
	TLineStyle *lineStyle = (TLineStyle*)context;
	assert(lineStyle->StyleTypeId() == TLineStyle::type_ID);
	PrepareRectLinesModeUpperLinkItem(hDC, lineStyle, props, context);

	TScreenItemStyle *basicStyle = m_basic_symbol.GetBasicStyle();
	assert(basicStyle->StyleTypeId() == TBasicSymbolStyle::type_ID);

	TBasicSymbolStyleInfo &style_info = ((TBasicSymbolStyle*)basicStyle)->GetBasicStyleInfo();
	m_basic_symbol.PrepareRectLinesModeItems(hDC, style_info, m_pos_x, m_pos_y, 0, MainLabelText(context), -1, SymbolLabelText(context), -1);
}

template <class basic_symbol>
void TGenericTreeNodeEx<basic_symbol>::ShowLinkScreenItems(TTreeItemsPlacingMode mode, TList &screen_items)
{
	// Placing mode is not important for this method.
	ShowPreparedItem(screen_items, m_tree_link_item);
}

template <class basic_symbol>
void TGenericTreeNodeEx<basic_symbol>::ShowBkgrScreenItems(TTreeItemsPlacingMode mode, TList &screen_items)
{
	if (mode == plc_mode_concentric)
	{
		ShowPreparedItem(screen_items, m_basic_symbol.m_def_circle);
	}
	else if (mode == plc_mode_rect_lines)
	{
		ShowPreparedItem(screen_items, m_basic_symbol.m_main_rect);
		ShowPreparedItem(screen_items, m_basic_symbol.m_lower_rect);
	}
}

template <class basic_symbol>
void TGenericTreeNodeEx<basic_symbol>::ShowFrgrScreenItems(TTreeItemsPlacingMode mode, TList &screen_items)
{
	// Placing mode is not important for this method.
	ShowPreparedItem(screen_items, m_basic_symbol.m_main_label);
	ShowPreparedItem(screen_items, m_basic_symbol.m_symbol_label);
	ShowPreparedItem(screen_items, m_basic_symbol.m_app_icon);
}

#endif	// ScreenItems_ScreenItemsTree_H


