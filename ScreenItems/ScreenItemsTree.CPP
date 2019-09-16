//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Placing trees of the screen items.
//
//   Do not include the file math.h from here. It is included from ScreenItemsTree.H
//   using special #define statement for picking up definition of the number pi.
//

#define    STRICT
#include  <windows.h>
#include  <stdio.h>
#include  <assert.h>

#include  "Common/FormatNumber.H"
#include  "ScreenItems/ScreenItemsTree.H"

// ------------------------------------------------------------------------
//   ====================  TGenericTreeNode  ====================
// ------------------------------------------------------------------------

struct TNodeBalancingInfo
{
	int					chld_weight;
	TGenericTreeNode		*chld_ptr;
};

void TGenericTreeNode::AdoptChild(TGenericTreeNode *child_node, TGenericTreeNode *insert_before, bool bypass_ownership)
{
	assert(child_node != NULL);

	if (insert_before != NULL)
		m_children.InsertBefore(child_node, insert_before);
	else m_children.AppendItem(child_node);

	child_node->m_owner = this;
	child_node->SetOwnedFlag(bypass_ownership);
}

void TGenericTreeNode::AcceptChildren(TGenericTreeNode &other_node)
{
	while (other_node.m_children.IsEmpty() == FALSE)
	{
		TGenericTreeNode *item = (TGenericTreeNode*)other_node.m_children.GetFirst();
		other_node.m_children.RemoveItem(item);
		AdoptChild(item, NULL, ((item->m_flags & etgi_owned) != 0) ? TRUE : FALSE);
	}
}

void TGenericTreeNode::ReleaseChildren()
{
	// Release all owned children.
	while (m_children.IsEmpty() == FALSE)
	{
		TGenericTreeNode *item = (TGenericTreeNode*)m_children.GetFirst();
		m_children.RemoveItem(item);

		if ((item->m_flags & etgi_owned) != 0)
			delete item;
	}
}

TGenericTreeNode::ChildrenVisibilityState TGenericTreeNode::GetChildrenVisibilityState()
{
	if (m_children.IsEmpty() == TRUE)
		return(cvs_no_chilren);

	// Some children are present. Check their state.
	bool first_item_visible = GetFirstChild(TRUE)->GetVisiblityState();
	for (TListIter<TGenericTreeNode> iter(m_children); iter; ++iter)
	{
		TGenericTreeNode &item = iter.CurrItem();
		if (item.GetVisiblityState() != first_item_visible)
		{
			return(cvs_mixture);
		}
	}

	// All children have identical visiblity state.
	return((first_item_visible == TRUE) ? cvs_all_visible : cvs_all_hidden);
}

void TGenericTreeNode::SetChildrenVisibilityState(bool value)
{
	for (TListIter<TGenericTreeNode> iter(m_children); iter; ++iter)
	{
		iter.CurrItem().m_flags &= ~etgi_visible;
		if (value == TRUE)
			iter.CurrItem().m_flags |= etgi_visible;
	}
}

static int __cdecl CompareNodeBalancingInfos(const TNodeBalancingInfo *bs1, const TNodeBalancingInfo *bs2)
{
	if (bs1->chld_weight > bs2->chld_weight)
		return(-1);
	if (bs1->chld_weight < bs2->chld_weight)
		return(1);

	return(0);
}

int TGenericTreeNode::BalanceVisibleChildren()
{
	// Array of visible children in their original order.
	TStructsArray<TNodeBalancingInfo, 100, 500> local_chlds;

	// Array of visible children in the different order.
	TStructsArray<TNodeBalancingInfo, 100, 500> mixed_chlds;

	int total_weight = 1;
	int num_chld = 0;

	// Process the clidlren. This will also collect their relative weights.
	for (TListIter<TGenericTreeNode> iter(m_children); iter; ++iter)
	{
		TGenericTreeNode *item = &iter.CurrItem();
		if (item->GetVisiblityState() == FALSE)
			continue;

		if (local_chlds.ReserveSpace() == FALSE)
		{
			// Memory allocation failure. Show this part of the tree unbalanced instead of failing
			// the whole placing process.
			return(total_weight);
		}

		TNodeBalancingInfo inst = { item->BalanceVisibleChildren(), item };
		local_chlds.AppendItem(inst);
		total_weight += inst.chld_weight;
		num_chld++;
	}

	// Check that number of children is big enough to do any balancing and prepare the dest space.
	if (num_chld <= 3 || mixed_chlds.ReserveSpace(num_chld) == FALSE)
		return(total_weight);
	mixed_chlds.SetNumItems(num_chld);

	// Sort the children according to their weights.
	local_chlds.QuickSort(CompareNodeBalancingInfos);
	if (local_chlds[0].chld_weight == 1)
	{
		// All the children are simple. There is no point in entangling them.
		return(total_weight);
	}

	// Remove all visible children from the list and clear the dest array.
	for (int i1 = 0; i1 < num_chld; ++i1)
	{
		m_children.RemoveItem(local_chlds[i1].chld_ptr);
		mixed_chlds[i1].chld_ptr = NULL;
	}

	// Process the most heavy children first. Ask the derived class for initial index into the dest array.
	int dest_inx = GetChildrenBalancingSeed() % num_chld;
	int step = num_chld/2;
	int slots = 2;
	int isl = 0;
	int phase = 0;

	for (int src = 0; src < num_chld; ++src)
	{
		// Check that the place is free in the dest array.
		while (mixed_chlds[dest_inx].chld_ptr != NULL)
		{
			dest_inx++;
			if (dest_inx >= num_chld)
				dest_inx = 0;
		}

		// Fill in the pointer.
		mixed_chlds[dest_inx].chld_ptr = local_chlds[src].chld_ptr;

		// Prepare the next dest index.
		if (++isl >= slots)
		{
			if (phase == 0)
			{
				int small_step = step/2;
				if (small_step == 0)
					small_step = 1;
				dest_inx = (dest_inx+small_step) % num_chld;
				phase = 1;
			}
			else
			{
				slots *= 2;
				step = num_chld/slots;
				if (step == 0)
					step = 1;
				phase = 0;
			}

			isl = 0;
		}

		dest_inx = (dest_inx+step) % num_chld;
	}

	// Merge the entangled pointers back into the list of children.
	for (int i2 = 0; i2 < num_chld; ++i2)
		m_children.PrependItem(mixed_chlds[i2].chld_ptr);

	// The tree should be not destroyed after these manipulations.
	return(total_weight);
}

int TGenericTreeNode::NumDirectChildren(bool show_all) const
{
	if (show_all == TRUE)
		return(m_children.NumItems());

	// Count the number of visible children.
	int cnt = 0;
	for (TListIter<TGenericTreeNode> iter((TList&)m_children); iter; ++iter)
	{
		TGenericTreeNode &item = iter.CurrItem();
		if (item.GetVisiblityState() == TRUE)
			cnt++;
	}

	return(cnt);
}

int TGenericTreeNode::NumSubtreeNodes(bool show_all_children) const
{
	int cnt = 1;

	for (TListIter<TGenericTreeNode> iter((TList&)m_children); iter; ++iter)
	{
		TGenericTreeNode &item = iter.CurrItem();
		if (item.GetVisiblityState() == TRUE)
			cnt += item.NumSubtreeNodes(show_all_children);
	}

	return(cnt);
}

TGenericTreeNode *TGenericTreeNode::GetFirstChild(bool show_all)
{
	TGenericTreeNode *item = (TGenericTreeNode*)m_children.GetFirst();
	if (show_all == TRUE)
		return(item);

	while (item != NULL && item->GetVisiblityState() == FALSE)
	{
		item = (TGenericTreeNode*)m_children.GetNext(item);
	}

	return(item);
}

TGenericTreeNode *TGenericTreeNode::GetLastChild(bool show_all)
{
	TGenericTreeNode *item = (TGenericTreeNode*)m_children.GetLast();
	if (show_all == TRUE)
		return(item);

	while (item != NULL && item->GetVisiblityState() == FALSE)
	{
		item = (TGenericTreeNode*)m_children.GetPrev(item);
	}

	return(item);
}

TGenericTreeNode *TGenericTreeNode::GetNextSibling(bool show_all)
{
	TGenericTreeNode *item = this;

	do
	{
		item = (TGenericTreeNode*)(m_owner->m_children.GetNext(item));
		if (show_all == TRUE)
			return(item);
	}
	while (item != NULL && item->GetVisiblityState() == FALSE);

	return(item);
}

TGenericTreeNode *TGenericTreeNode::GetPrevSibling(bool show_all)
{
	TGenericTreeNode *item = this;

	do
	{
		item = (TGenericTreeNode*)(m_owner->m_children.GetPrev(item));
		if (show_all == TRUE)
			return(item);
	}
	while (item != NULL && item->GetVisiblityState() == FALSE);

	return(item);
}

bool TGenericTreeNode::ShiftSubtree(long shift_x, long shift_y)
{
	// Shift the root as a separate action.
	m_pos_x += shift_x;
	m_pos_y += shift_y;

	// Iterate all children if any.
	TGenericTreeNodesIterator iter(this);
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TGenericTreeNode *item = iter.GetCurrentItem();
		long pos_x, pos_y;
		item->GetPosition(pos_x, pos_y);
		item->SetPosition(pos_x+shift_x, pos_y+shift_y);
	}

	return(iter.CheckIterationSuccess());
}

void TGenericTreeNode::PrepareConcentricModeUpperLinkItem(HDC hDC, TLineStyle *link_style, void *context)
{
	if (m_owner != NULL && link_style != NULL)
	{
		// Prepare the link item.
		m_tree_link_item.SetStyle(link_style);

		long pos_x, pos_y;
		m_owner->GetPosition(pos_x, pos_y);
		m_tree_link_item.SetJctPos(0, pos_x, pos_y, FALSE);
		m_tree_link_item.SetJctPos(1, m_pos_x, m_pos_y, TRUE);
	}
	else
	{
		// The link item should be not visible.
		m_tree_link_item.SetStyle(NULL);
	}
}

void TGenericTreeNode::PrepareRectLinesModeUpperLinkItem(HDC hDC, TLineStyle *link_style, TRectLinesModePlacingProps &props, void *context)
{
	if (m_owner != NULL && link_style != NULL)
	{
		// Prepare the link item.
		m_tree_link_item.SetStyle(link_style);

		// Pick up the position of the owner and use it as a starting point.
		long owner_pos_x, owner_pos_y;
		m_owner->GetPosition(owner_pos_x, owner_pos_y);
		m_tree_link_item.SetJctPos(0, m_placer_data[0], owner_pos_y+m_owner->GetRectLinesModePlacingParam(hDC, rcln_size_y, props, context)-2, FALSE);

		long upper_x = m_pos_x + GetRectLinesModePlacingParam(hDC, rcln_upper_link_offset, props, context);
		if (m_placer_data[0] != upper_x)
		{
			// Use 3 sections link.
			m_tree_link_item.SetJctPos(1, m_placer_data[0], m_placer_data[1], FALSE);
			m_tree_link_item.SetJctPos(2, upper_x, m_placer_data[1], FALSE);
			m_tree_link_item.SetJctPos(3, upper_x, m_pos_y, TRUE);
		}
		else
		{
			// Use simple one section link.
			m_tree_link_item.SetJctPos(1, upper_x, m_pos_y, TRUE);
		}
	}
	else
	{
		// There should be no upper link.
		m_tree_link_item.SetStyle(NULL);
	}
}

// ----------------------------------------------------------------------
//   ================  TBasicTreeGraphPlacer  ====================
// ----------------------------------------------------------------------

void TBasicTreeGraphPlacer::ShowMajorStatus(const wchar_t *msg)
{
	if (m_console != NULL)
	{
		m_console->SetMajorStatus(msg);
	}
}

void TBasicTreeGraphPlacer::ShowCurrentPlacingLevel(int level, int highest_existing_level)
{
	if (m_console != NULL)
	{
		TDateTime curr_time = CurrDateTime();
		if ((curr_time - m_prev_placing_layer_display_time) > 180*MILLI_SEC)
		{
			wchar_t buffer[80];
			swprintf(buffer, 80, L"Processing level  %d out of %d ...", level, highest_existing_level);
			m_console->SetMinorStatus(buffer);
			m_prev_placing_layer_display_time = curr_time;
		}
	}
}

void TBasicTreeGraphPlacer::ClearMinorStatus()
{
	if (m_console != NULL)
	{
		m_console->SetMinorStatus(NULL);
	}
}

void TBasicTreeGraphPlacer::ShowErrorMessage(const wchar_t *msg)
{
	if (m_console != NULL)
	{
		m_console->ShowRunTimeError(msg);
		DebugTrace(L"TreeGraphPlacer: %s", msg);
	}
}

void TBasicTreeGraphPlacer::ShowOutOfMemoryError()
{
	if (m_console != NULL)
	{
		m_console->ShowRunTimeError(L"Low on memory. The placing was aborted.");
		DebugTrace(L"TreeGraphPlacer: Low on memory, aborting.");
	}
}

void TBasicTreeGraphPlacer::DebugTrace(const wchar_t *format, ...)
{
	if (m_console != NULL && m_tracing_active == TRUE)
	{
		// Generate the UNICODE string.
		wchar_t message_buff[2048];
		va_list vargs;
		va_start(vargs, format);
		vswprintf(message_buff, 2048, format, vargs);
		va_end(vargs);

		// Pass it to the callback.
		m_console->HandleTrace(message_buff);
	}
}

// ------------------------------------------------------------------------
//    ================ TGenericTreeNodesIterator ===================
// ------------------------------------------------------------------------

TGenericTreeNodesIterator::TGenericTreeNodesIterator(TGenericTreeNode *root_node, bool iterate_all_children, TBasicTreeGraphPlacer *placer)
{
	m_root = root_node;
	m_iter_all = iterate_all_children;

	m_iter_problem = FALSE;

	m_iter_level = 0;
	m_placer = placer;
}

void TGenericTreeNodesIterator::StartIteration(TGenericTreeNode *start_with, int max_level, TNodesIterationDirection dir)
{
	//
	//  When after calling this function m_iter_level data field is not assigned a non zero value, this means
	//  that iteration has failed to start.
	//

	// Clear the prev state of the iteration if any.
	m_iter_level = 0;

	// Do simple verification of the max allowed level and assign it before the first point of exit.
	if (max_level < 0)
		max_level = 0;
	if (max_level > TREE_ITEMS_PLACER_MAX_TREE_DEPTH)
		max_level = TREE_ITEMS_PLACER_MAX_TREE_DEPTH;
	m_iter_max_level = max_level;

	// Ensure that startup item exists and that it is accessible for iteration.
	if (start_with == NULL)
	{
		start_with = (dir == ndi_dir_forward) ? m_root->GetFirstChild(m_iter_all) : m_root->GetLastChild(m_iter_all);
		if (start_with == NULL)
		{
			// The tree is empty. Iteration was not really started but it has succeeded.
			return;
		}
	}

	if (m_iter_all == FALSE && start_with->GetVisiblityState() == FALSE)
	{
		// Iteration has failed to start.
		ReportIterationError(L"Unable to start the iteration. Initial node is not visible.");
		return;
	}

	// Find out the level of the startup item.
	int initial_level = 0;
	TGenericTreeNode *i1 = start_with;
	while (i1->m_owner != NULL)
	{
		initial_level++;
		i1 = i1->m_owner;
		if (i1 == m_root)
			break;

		// Ensure that passed initial item belongs to the iterated tree.
		assert(i1 != NULL);

		assert(initial_level <= TREE_ITEMS_PLACER_MAX_TREE_DEPTH);
		if (initial_level > TREE_ITEMS_PLACER_MAX_TREE_DEPTH)
		{
			// The tree seems to be corrupted. Present this as iteration failed to start.
			ReportIterationError(L"Unable to start the iteration. Error detecting the depth level of the initial node.");
			return;
		}
	}

	if (initial_level == 0)
	{
		ReportIterationError(L"Unable to start the iteration. Initial node has no parent.");
		return;
	}
	else if (m_iter_max_level != 0 && initial_level > m_iter_max_level)
	{
		ReportIterationError(L"Unable to start the iteration. Initial node is too deep in the tree.");
		return;
	}
	else if (m_nodes.ReserveTotalBufferSpace(initial_level+2) == FALSE)
	{
		ReportIterationError(L"Unable to start the iteration. Error allocating the list of nodes.");
		return;
	}

	// Fill in the initial state of the iterator.
	m_nodes[0] = m_nodes[initial_level+1] = NULL;
	TGenericTreeNode *i2 = start_with;
	for (int inx = initial_level; inx > 0; --inx, i2 = i2->m_owner)
		m_nodes[inx] = i2;

	// Iteration has managed to start.
	m_iter_level = initial_level;
}

void TGenericTreeNodesIterator::SeekToLevel(int requested_item_level, TNodesIterationDirection dir)
{
	if (m_iter_level > 0 && m_iter_level != requested_item_level)
	{
		StepIterationAtLevel(requested_item_level, dir);
	}
}

void TGenericTreeNodesIterator::StepIteration(TNodesIterationDirection dir)
{
	//
	// Make a non empty step to the next item.
	//

	if (m_iter_level == 0)
	{
		// The iteration is not active.
		return;
	}

	for(;;)
	{
		// Retrieve the previous current item.
		TGenericTreeNode *curr = m_nodes[m_iter_level];

		// Attempt 1. Try to iterate the children of the current node.
		if ((m_iter_max_level == 0 || m_iter_level < m_iter_max_level) && m_nodes[m_iter_level+1] == NULL)
		{
			// State of iteration allows to enter the next level. Check if current item has appropriate children or not.
			TGenericTreeNode *new_level_item =  (dir == ndi_dir_forward) ? curr->GetFirstChild(m_iter_all) : curr->GetLastChild(m_iter_all);
			if (new_level_item != NULL)
			{
				// Check if there is space in the list of nodes.
				if (m_nodes.ReserveTotalBufferSpace(m_iter_level+3) == TRUE)
				{
					// Occupy the slot.
					m_nodes[m_iter_level+1] = new_level_item;
					m_nodes[m_iter_level+2] = NULL;
					m_iter_level++;
					return;
				}
				else
				{
					// No more memory. Abort the iteration.
					ReportIterationError(L"Unable to expand the list of iterated nodes.");
					m_iter_level = 0;
					return;
				}
			}
		}

		//
		// Eiter all childen of the current item were already iterated, or they are blocked by m_iter_max_level limit
		// or the current item has no visible children.
		//

		// Attempt 2. Try to seek to the nearest sibling of the current item.
		TGenericTreeNode *new_sibling_item = (dir == ndi_dir_backward) ? curr->GetPrevSibling(m_iter_all) : curr->GetNextSibling(m_iter_all);
		if (new_sibling_item != NULL)
		{
			m_nodes[m_iter_level] = new_sibling_item;

			// Put the stamp that children of this new item were not iterated yet.
			m_nodes[m_iter_level+1] = NULL;
			return;
		}

		//
		// Current item has no siblings in the requested direction.
		//

		// Attempt 3. Step up and try to find something there. Note that in normal case the for(;;) loop is not exited.
		// This means that upper item, that is already iterated, will not be presented to the upper layer.
		m_iter_level--;

		if (m_iter_level <= 0)
		{
			// Iteration has successfully terminated.
			break;
		}
	}
}

void TGenericTreeNodesIterator::StepIterationAtLevel(int requested_item_level, TNodesIterationDirection dir)
{
	assert(requested_item_level > 0);
	if (m_iter_max_level != 0 && requested_item_level > m_iter_max_level)
	{
		ReportIterationError(L"StepIterationAtLevel: Requested item level is too big. Aborting the iteration.");
		m_iter_level = 0;
		return;
	}

	int saved_iter_max_level = m_iter_max_level;
	m_iter_max_level = requested_item_level;

	while (m_iter_level != 0)
	{
		StepIteration(dir);
		if (m_iter_level == requested_item_level)
			break;
	}

	m_iter_max_level = saved_iter_max_level;
}

// ----------------------------------------------------------------------
//   ===============  TConcentricModeTreeGraphPlacer  ==============
// ----------------------------------------------------------------------

//
//  Concentric graph layout constants.
//

const double g_plc_node_radius_driven_layer_step_koef = 1.6;
	// This value is used when two adjacent layers are populated with too big nodes. When sum of the max radiuses
	// from nodes off the layers multiplied by this koefficient is bigger than min radius step calculated by other means,
	// then this radius step is used.

const double g_plc_between_groups_extra = 1.7;
	// Ratio of the step between the groups of children and the step inside the group of nodes.

const double g_plc_max_bissect_family_angle = 0.80*M_PI;
	// Max allowed angle for the family for the outside-in placing style.

const double g_plc_max_sparced_family_angle = 0.83*M_PI;
	// Max allowed angle for the family of the children in the inside-out placing style.

const double g_plc_max_sparced_incln_angle  = 0.40*M_PI;
	// Max allowed angle between the outer side beams of the family and the line from the center
	// that goes threw the root of the family.

#define PLC_GRAD(x)		(x/M_PI*180)
#define PLC_RADIAN(x)	(((double)x)/180*M_PI)

TConcentricModeTreeGraphPlacer::TConcentricModeTreeGraphPlacer(TGenericConsoleEx *pCons)
				: TBasicTreeGraphPlacer(pCons)
{
	m_root = NULL;
	m_iter.SetPlacer(this);
}

bool TConcentricModeTreeGraphPlacer::DoConcentricPlacing(HDC hDC, TGenericTreeNode *root_node, TConcentricModePlacingProps &props,
														void **contexts_array, int *max_populated_level, long *max_used_radius)
{
	assert(root_node != NULL);
	m_root = root_node;
	m_iter.SetRootNode(root_node);

	if (max_populated_level != NULL)
		*max_populated_level = 0;
	if (max_used_radius != NULL)
		*max_used_radius = 0;

	// Process info about the minimum layers spacing.
	m_min_radius_step = 1.3*props.nodes_arc_step;
	m_num_radius_step_infos = 0;
	if (props.min_radius_steps != NULL)
	{
		int ils = 0;
		while (props.min_radius_steps[ils] != 0)
		{
			m_min_radius_step = props.min_radius_steps[ils];
			m_num_radius_step_infos = ++ils;
		}
	}

	// Prepare other placing constants.
	m_inside_group_arc_step = props.nodes_arc_step;
	m_between_groups_arc_step = m_inside_group_arc_step * g_plc_between_groups_extra;
	m_clock_wise = props.clock_wise;

	m_dir_forw = (m_clock_wise == TRUE) ? ndi_dir_forward : ndi_dir_backward;
	m_dir_backw = (m_clock_wise == TRUE) ? ndi_dir_backward : ndi_dir_forward;

	// Check, if the passed tree needs balancing or not.
	if (props.balance_the_tree == TRUE)
	{
		m_root->BalanceVisibleChildren();
	}

	// Position the root.
	m_root->SetPosition(0, 0);

	// Put initial level, that cannot be used for enlarging the radiuses.
	m_prev_radius_inc_level = 0;
	m_prev_radius_inc_value = m_min_radius_step/4;

	// Allocate and init the local data variables. At this point depth of the tree is not known yet. Lengths of all these arrays
	// should be kept in sync with the length of m_lev_radius. Slot [0] stores info about the root of the tree.
	TCnctIntsArray		level_nodes;
	TCnctIntsArray		level_groups;
	TCnctIntsArray		level_max_diameter;
	TCnctNodePtrsArray	max_children_node;						// Pointer to node with the biggest number of children on the layer
																// or NULL if none of the items on the layer have children.
	TCnctIntsArray		max_children_node_children;
	TCnctBoolsArray		level_is_dense;

	int min_dense_lev = 0;
	int highest_populated_level = 0;								// Level 0 is occupied bt the root object, levels starting from 1 are
																// populated by the regular tree nodes. This is index of the last existing
																// element in all arrays.
	DebugTrace(L"TConcentricModeTreeGraphPlacer::DoPlacing");

	//
	// Step 1.
	//
	// Find out how many nodes and node groups reside on each level, identify items with the biggest number of children for each level.
	//
	ShowMajorStatus(L"Determine levels population ...");

	// The data for the root level (level 0) is not used later. Nevertheless it is still filled for debugging purposes.
	level_nodes[0] = 1;
	level_groups[0] = 1;
	level_max_diameter[0] = m_root->GetConcentricModePlacingParam(hDC, cnct_node_diameter, contexts_array[m_root->GetPolyInx()]);
	max_children_node[0] = m_root;
	max_children_node_children[0] = m_root->NumDirectChildren(FALSE);
	level_is_dense[0] = FALSE;

	m_lev_radius[0] = 0;
	m_lev_radius.SetNumItems(1);

	// First iteration. Nothing is known about the tree yet.
	for (m_iter.StartIteration(); m_iter.CheckIteration(); m_iter.StepIteration())
	{
		TGenericTreeNode *item = m_iter.GetCurrentItem();
		int clev = m_iter.GetCurrentLevel();

		if (clev >= m_lev_radius.NumItems())
		{
			//
			// New tree level is found. Allocate the space for this level in all arrays.
			//

			if (level_nodes.ReserveTotalBufferSpace(clev+1) == FALSE)
			{
				ShowOutOfMemoryError();
				return(FALSE);
			}

			if (level_groups.ReserveTotalBufferSpace(clev+1) == FALSE)
			{
				ShowOutOfMemoryError();
				return(FALSE);
			}

			if (level_max_diameter.ReserveTotalBufferSpace(clev+1) == FALSE)
			{
				ShowOutOfMemoryError();
				return(FALSE);
			}

			if (max_children_node.ReserveTotalBufferSpace(clev+1) == FALSE)
			{
				ShowOutOfMemoryError();
				return(FALSE);
			}

			if (max_children_node_children.ReserveTotalBufferSpace(clev+1) == FALSE)
			{
				ShowOutOfMemoryError();
				return(FALSE);
			}

			if (level_is_dense.ReserveTotalBufferSpace(clev+1) == FALSE)
			{
				ShowOutOfMemoryError();
				return(FALSE);
			}

			int extra_levs = clev+1 - m_lev_radius.NumItems();
			if (m_lev_radius.ReserveSpace(extra_levs) == FALSE)
			{
				ShowOutOfMemoryError();
				return(FALSE);
			}

			// Fill in the allocated cells.
			for (int ii = m_lev_radius.NumItems(); ii <= clev; ++ii)
			{
				level_nodes[ii] = level_groups[ii] = 0;
				level_max_diameter[ii] = 0;
				max_children_node[ii] = NULL;
				max_children_node_children[ii] = 0;
				level_is_dense[ii] = FALSE;
				m_lev_radius[ii] = 0;
			}

			m_lev_radius.SetNumItems(clev+1);
			highest_populated_level = clev;
		}

		level_nodes[clev]++;
		if (GetNextNeighbor(item) == NULL)
			level_groups[clev]++;

		int diam = item->GetConcentricModePlacingParam(hDC, cnct_node_diameter, contexts_array[m_root->GetPolyInx()]);
		if (diam > level_max_diameter[clev])
			level_max_diameter[clev] = diam;

		int num_children = item->NumDirectChildren(FALSE);
		if (num_children > max_children_node_children[clev])
		{
			max_children_node[clev] = item;
			max_children_node_children[clev] = num_children;
		}
	}

	if (m_iter.CheckIterationSuccess() == FALSE)
		return(FALSE);

	// The loop below that will do some useful work and print debug info about the layers.
	DebugTrace(L"---------------------- Info about the passed tree ----------------------");
	for (int lev1=1; lev1 <= highest_populated_level; ++lev1)
	{
		assert(level_nodes[lev1] != 0);
		double arc_len = (level_nodes[lev1]-level_groups[lev1]) * m_inside_group_arc_step + level_groups[lev1] * m_between_groups_arc_step;

		// Use max diameters of the layer nodes as first approximation for the radius increase.
		double max_radiuses_sum = (level_max_diameter[lev1-1]+level_max_diameter[lev1])/2;
		double radius_delta_from_layer_nodes = g_plc_node_radius_driven_layer_step_koef*max_radiuses_sum;

		// Use values from the props for this layer as other approximation for the radius increase.
		double radius_delta_from_props = m_min_radius_step;
		if (lev1 <= m_num_radius_step_infos)
			radius_delta_from_props = props.min_radius_steps[lev1-1];

		double min_radius = m_lev_radius[lev1-1] + __max(radius_delta_from_layer_nodes, radius_delta_from_props);
		if (arc_len < min_radius * (2*M_PI))
		{
			// Level is populated sparsely. Use min possible radius.
			m_lev_radius[lev1] = min_radius;
		}
		else
		{
			// Determine the radius from the population of the level.
			m_lev_radius[lev1] = arc_len / (2*M_PI);
			level_is_dense[lev1] = TRUE;
			if (min_dense_lev == 0)
				min_dense_lev = lev1;
		}

		DebugTrace(L"TreeInfo: level=%3d, lev_nodes=%4d, lev_groups=%4d, dense=%s, initial_radius=%.1lf.",
					lev1, level_nodes[lev1], level_groups[lev1], (level_is_dense[lev1] == TRUE) ? L"TRUE" : L"-no-", m_lev_radius[lev1]);
	}

	//
	// Step 2.
	//
	// Calc initial approximation for the radiuses of the levels. Find out what radiuses are driven by the population of the level.
	//

	ShowMajorStatus(L"Placing ...");

	// Step 2a. This step is used for the "beam mode" only. Max_angle should persist when the radiuses are enlarged.
	double max_beam_angle = 0;
	int most_wide_lev = 1;				// Initial value is set here in this way for the case when the tree
										// consists of only one linear branch.
	if (min_dense_lev == 0)
	{
		// Find level with the biggest opening angle.
		for (int lev2 = 1; lev2 <= highest_populated_level; ++lev2)
		{
			double arc_len = (level_nodes[lev2]-level_groups[lev2]) * m_inside_group_arc_step + (level_groups[lev2]-1) * m_between_groups_arc_step;
			if (arc_len/m_lev_radius[lev2] > max_beam_angle)
			{
				// The way of setting the lev_radius above should guarantee that the max_beam_angle is not bigger than 2pi.
				max_beam_angle = arc_len/m_lev_radius[lev2];
				most_wide_lev = lev2;
			}
		}
	}

	//
	// Step 3.
	//
	// Do the placing itself.
	//

	double circ_grps_extra = g_plc_between_groups_extra;
	if (max_populated_level != NULL)
		*max_populated_level = highest_populated_level;

	// Check if tree has children or not.
	if (highest_populated_level == 0)
	{
		// There are no other nodes besides the root.
		return(TRUE);
	}

	StartOver:
	{
		// Check the value of the radius to start with.
		double start_over_radius = m_lev_radius[(min_dense_lev > 0) ? min_dense_lev : most_wide_lev];
		if (start_over_radius >= 100000000)
		{
			ShowErrorMessage(L"Placing was aborted because radius is getting too big.\n"
							L"Some of the graph nodes on the inner level may overlap.");
			DebugTrace(L"StartOver: radius is getting too big (%.1lf), aborting.", start_over_radius);
			return(FALSE);
		}
	}

	// Distinguish between 2 styles of the object placing.
	if (min_dense_lev != 0)
	{
		min_dense_lev = 1;

		//
		//  This is a "full sun" placing mode. Items are evenly placed all over around the center.
		//
		ShowCurrentPlacingLevel(min_dense_lev, highest_populated_level);
		DebugTrace(L"StartOver: \"full_sun_mode\". min_dense_lev=%d.", min_dense_lev);

		// Place nodes on the lowest_dense_level in circular mode.
		double ingrp_step = (2*M_PI*m_lev_radius[min_dense_lev]) / (level_nodes[min_dense_lev] + level_groups[min_dense_lev]*(circ_grps_extra-1));
		if (min_dense_lev == 1 && level_groups[min_dense_lev] == 1)
			ingrp_step = (2*M_PI*m_lev_radius[min_dense_lev]) / level_nodes[min_dense_lev];

		if (PlaceLayerInCircularMode(props.placing_direction*M_PI/180, min_dense_lev, ingrp_step, ingrp_step*circ_grps_extra) == FALSE)
		{
			EnlargeRadiuses(min_dense_lev);
			goto StartOver;
		}

		// Place nodes of all lower levels in the bissect mode.
		for (int lev3 = min_dense_lev-1; lev3 > 0; --lev3)
		{
			ShowCurrentPlacingLevel(lev3, highest_populated_level);
			if (m_lev_radius[lev3+1] > 2*m_lev_radius[lev3])
				m_lev_radius[lev3] = m_lev_radius[lev3+1]/2;

			bool family_problem;
			if (PlaceLayerInBissectMode(lev3, TRUE, family_problem) == FALSE)
			{
				EnlargeRadiuses((family_problem == TRUE) ? lev3+1 : lev3);
				circ_grps_extra *= 1.2;
				goto StartOver;
			}
		}
	}
	else
	{
		most_wide_lev = 1;

		//
		//  This is a "beam style" placing mode when the angle of the beam is driven by the most densly populated level.
		//
		ShowCurrentPlacingLevel(most_wide_lev, highest_populated_level);
		DebugTrace(L"StartOver: \"beam_style\". most_wide_lev=%d.", most_wide_lev);

		// Calc the leftmost angle and place items on the "most wide" level.
		double start_angle = (props.placing_direction*M_PI/180) + max_beam_angle/2;
		int grps = level_groups[most_wide_lev];
		double ingrp_step  = (max_beam_angle*m_lev_radius[most_wide_lev]) / (level_nodes[most_wide_lev] - grps + (grps-1)*circ_grps_extra);

		if (PlaceLayerInCircularMode(start_angle, most_wide_lev, ingrp_step, ingrp_step*circ_grps_extra) == FALSE)
		{
			EnlargeRadiuses(most_wide_lev);
			goto StartOver;
		}

		// Place nodes on all lower levels in bissect mode.
		for (int lev3 = most_wide_lev-1; lev3 > 0; --lev3)
		{
			ShowCurrentPlacingLevel(lev3, highest_populated_level);
			if (m_lev_radius[lev3+1] > 2*m_lev_radius[lev3])
				m_lev_radius[lev3] = m_lev_radius[lev3+1]/2;

			bool family_problem;
			if (PlaceLayerInBissectMode(lev3, FALSE, family_problem) == FALSE)
			{
				EnlargeRadiuses((family_problem == TRUE) ? lev3+1 : lev3);
				circ_grps_extra *= 1.2;
				goto StartOver;
			}
		}
	}

	// Process the rest of the higher levels if they are populated.
	int outer_lev = ((min_dense_lev > 0) ? min_dense_lev : most_wide_lev) + 1;
	double tight_koef = 0;
	while (outer_lev <= highest_populated_level)
	{
		// Update the display and check the radius.
		ShowCurrentPlacingLevel(outer_lev, highest_populated_level);
		DebugTrace(L"Placing outer level. lev=%d, radius=%.1lf, tight_koef=%.3lf.", outer_lev, m_lev_radius[outer_lev], tight_koef);

		if (m_lev_radius[outer_lev] >= 100000000)
		{
			ShowErrorMessage(L"Placing was aborted because radius is getting too big.\n"
							L"Some of the graph nodes on the most outer level may overlap.");
			DebugTrace(L"Radius is getting too big, aborting.");
			return(FALSE);
		}

		// Start processing the level from the node with the biggest number of children.
		bool first_family_problem = FALSE;
		if (PlaceLayerInSparcedMode(outer_lev, max_children_node[outer_lev-1], tight_koef, first_family_problem) == TRUE)
		{
			// Proceed to the next level.
			outer_lev++;
			tight_koef = 0;
		}
		else
		{
			if (first_family_problem == FALSE && tight_koef < 1.0)
			{
				// Increase the tight coefficient without enlarging the radius.
				tight_koef += 0.1;
				continue;
			}

			// Tightening the koef has not helped. Enlarge the radius. Some of the items might get negative coordinates.
			tight_koef = 0;
			EnlargeRadiuses(outer_lev);
		}
	}

	// If the control has managed to come here, this means that all visible items were successfully placed.
	if (max_used_radius != NULL)
		*max_used_radius = (long)m_lev_radius[highest_populated_level];
	return(TRUE);
}

// This is static method.
bool TConcentricModeTreeGraphPlacer::DisplayGraph(TScreenItemsViewObject &view_helper, TList &screen_items_list,
										TGenericTreeNode *root_node, TConcentricModePlacingProps &placing_props, void **contexts_array,
										int num_visible_objects_ctrl_id, int *num_visible_objects, bool show_prepared_tree,
										TGenericConsoleEx *pCons)
{
	// The graph should not be empty and the viewer should already have the control.
	assert(root_node != NULL);
	assert(view_helper.GetControl() != NULL);

	screen_items_list.Clear();

	// Remove the old picture from the screen items view control.
	if (show_prepared_tree == TRUE)
	{
		view_helper.ShowDataSet(&screen_items_list, TRUE);
	}

	if (num_visible_objects != NULL)
		*num_visible_objects = 0;

	// Clear the num_items control. It may reflect old state of the tree.
	HWND hViewHelperParent = ::GetParent(view_helper.GetControl());
	if (num_visible_objects_ctrl_id != 0 && hViewHelperParent != NULL)
	{
		::SetDlgItemTextW(hViewHelperParent, num_visible_objects_ctrl_id, NULL);
	}

	// Instantiate the local placer and iterator.
	TConcentricModeTreeGraphPlacer placer(pCons);
	TGenericTreeNodesIterator iter(root_node, FALSE, &placer);

	// Update the status panel.
	placer.ShowMajorStatus(L"Placing objects ...");
	placer.ClearMinorStatus();

	HDC hDC = ::GetDC(view_helper.GetControl());
	if (hDC == NULL)
	{
		placer.ShowErrorMessage(L"Error getting window DC.");
		return(FALSE);
	}

	// Prepare all nodes for placing.
	root_node->PrepareForPlacing(hDC, plc_mode_concentric, TRUE, contexts_array[root_node->m_poly_inx]);
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TGenericTreeNode *item = iter.GetCurrentItem();
		item->PrepareForPlacing(hDC, plc_mode_concentric, FALSE, contexts_array[item->m_poly_inx]);
	}

	// Position the structure.
	if (placer.DoConcentricPlacing(hDC, root_node, placing_props, contexts_array) == FALSE)
	{
		placer.ShowErrorMessage(L"Graph placing code has failed to place the graph.");
		::ReleaseDC(view_helper.GetControl(), hDC);
		return(FALSE);
	}

	// Update the status panel.
	placer.ShowMajorStatus(L"Preparing tree ...");
	placer.ClearMinorStatus();

	// Prepare screen items.
	root_node->PrepareConcentricModeScreenItems(hDC, contexts_array[root_node->m_poly_inx]);
	int num_objects = 1;
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TGenericTreeNode *item = iter.GetCurrentItem();
		item->PrepareConcentricModeScreenItems(hDC, contexts_array[item->m_poly_inx]);
		num_objects++;
	}

	// Link all prepared link screen items into the painting list.
	root_node->ShowLinkScreenItems(plc_mode_concentric, screen_items_list);
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TGenericTreeNode *item = iter.GetCurrentItem();
		item->ShowLinkScreenItems(plc_mode_concentric, screen_items_list);
	}

	// Link all prepared screen items into the painting list.
	root_node->ShowBkgrScreenItems(plc_mode_concentric, screen_items_list);
	root_node->ShowFrgrScreenItems(plc_mode_concentric, screen_items_list);
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TGenericTreeNode *item = iter.GetCurrentItem();
		item->ShowBkgrScreenItems(plc_mode_concentric, screen_items_list);
		item->ShowFrgrScreenItems(plc_mode_concentric, screen_items_list);
	}

	if (show_prepared_tree == TRUE)
	{
		// Show new contents of the screen items list.
		view_helper.ShowDataSet(&screen_items_list, FALSE);
		view_helper.ScrollDataSetIntoCenter();
	}

	if (num_visible_objects != NULL)
		*num_visible_objects = num_objects;

	if (num_visible_objects_ctrl_id != 0 && hViewHelperParent != NULL)
	{
		wchar_t buffer[40];
		::SetDlgItemTextW(hViewHelperParent, num_visible_objects_ctrl_id, FormatInt64(num_objects, buffer, 40));
	}

	// Update the status panel.
	placer.ShowMajorStatus(NULL);
	placer.ClearMinorStatus();

	// Final actions.
	::ReleaseDC(view_helper.GetControl(), hDC);
	return(TRUE);
}

bool TConcentricModeTreeGraphPlacer::PlaceLayerInCircularMode(double start_angle, int level, double in_grp_arc_step, double btw_grps_arc_step)
{
	//
	// Place nodes on the level evenly.
	//

	double radius = m_lev_radius[level];
	assert(radius > 0);

	double angle = start_angle;
	double insd_grp_angle = in_grp_arc_step/radius;
	double btw_grps_angle = btw_grps_arc_step/radius;

	// Iterate all visible items of the level.
	m_iter.StartIteration(NULL, level, m_dir_forw);
	for (m_iter.SeekToLevel(level, m_dir_forw); m_iter.CheckIteration(); m_iter.StepIterationAtLevel(level, m_dir_forw))
	{
		TGenericTreeNode *item = m_iter.GetCurrentItem();
		PlaceNode(item, radius, angle);
		angle -= (GetNextNeighbor(item) != NULL) ? insd_grp_angle : btw_grps_angle;
	}

	// This method all the times returns TRUE.
	return(TRUE);
}

bool TConcentricModeTreeGraphPlacer::PlaceLayerInBissectMode(int level, bool full_circle_mode, bool &family_problem)
{
	// Upper layer should be already positioned.
	double radius = m_lev_radius[level];
	assert(radius > 0);

	TGenericTreeNode *first_with_children = NULL;
	TGenericTreeNode *last_with_children = NULL;

	// Stage 1. Process only those items, that have children.
	m_iter.StartIteration(NULL, level, m_dir_forw);
	for (m_iter.SeekToLevel(level, m_dir_forw); m_iter.CheckIteration(); m_iter.StepIterationAtLevel(level, m_dir_forw))
	{
		TGenericTreeNode *item = m_iter.GetCurrentItem();
		TGenericTreeNode *left_child = GetFirstChld(item);
		TGenericTreeNode *right_child = GetLastChld(item);

		if (left_child == NULL || right_child == NULL)
			continue;

		if (first_with_children == NULL)
			first_with_children = item;

		double left_angle = GetDirectionValue(left_child, m_root);
		double right_angle = GetDirectionValue(right_child, m_root);
		double diff = GetAngleValue(left_angle, right_angle);

		if (diff > g_plc_max_bissect_family_angle)
		{
			family_problem = TRUE;
			DebugTrace(L"Bissect family angle failure. item=%08lX, left_chld_dir=%.1lf, right_chld_dir=%.1lf, diff=%.1lf.",
						item, PLC_GRAD(left_angle), PLC_GRAD(right_angle), PLC_GRAD(diff));
			return(FALSE);
		}

		if (left_angle < right_angle)
			left_angle += 2*M_PI;

		PlaceNode(item, radius, (left_angle+right_angle)/2);
		last_with_children = item;
	}

	// Stage 2. Process childless items that sit in between the items with children. In the meanwhile count childless items
	// that stay to the left and to the right of the items with children.
	assert(first_with_children != NULL && last_with_children != NULL);

	int left_items = 0, left_groups = 0;
	int right_items = 0, right_groups = 0;
	int middle_items = 0, middle_groups = 0;
	bool first_with_chld_found = FALSE, last_with_chld_found = FALSE;

	TGenericTreeNode *prev_with_children = NULL;
	m_iter.StartIteration(NULL, level, m_dir_forw);
	for (m_iter.SeekToLevel(level, m_dir_forw); m_iter.CheckIteration(); m_iter.StepIterationAtLevel(level, m_dir_forw))
	{
		TGenericTreeNode *item = m_iter.GetCurrentItem();
		if (GetFirstChld(item) == NULL)
		{
			// Current item has no children.
			if (first_with_chld_found == FALSE)
			{
				left_items++;
				if (GetNextNeighbor(item) == NULL)
					left_groups++;
			}
			else if (last_with_chld_found == FALSE)
			{
				middle_items++;
				if (GetNextNeighbor(item) == NULL)
					middle_groups++;
			}
			else
			{
				right_items++;
				if (GetNextNeighbor(item) == NULL)
					right_groups++;
			}
		}
		else
		{
			// Children are present.
			if (first_with_chld_found == TRUE && last_with_chld_found == FALSE)
			{
				// Potentially some "middle" type items were noticed.
				assert(prev_with_children != NULL);

				if (middle_items > 0)
				{
					// Process items of this middle group.
					double left_bndr_angle = GetDirectionValue(prev_with_children, m_root);
					double right_bndr_angle = GetDirectionValue(item, m_root);
					double diff = GetAngleValue(left_bndr_angle, right_bndr_angle);

					// Process prev_with_children like it is in the "middle" group.
					middle_items++;
					if (GetNextNeighbor(prev_with_children) == NULL)
						middle_groups++;

					double len_ins_grp = (middle_items-middle_groups) * m_inside_group_arc_step;
					double ins_grp_step = (middle_groups > 0) ? m_inside_group_arc_step : (diff*radius)/middle_items;
					double btw_grps = (middle_groups > 0) ? (diff*radius-len_ins_grp)/middle_groups : 0;

					if (ins_grp_step < m_inside_group_arc_step || btw_grps && btw_grps < m_between_groups_arc_step)
					{
						family_problem = FALSE;
						DebugTrace(L"Bissect placing middle items failure.");
						return(FALSE);
					}

					PlaceBissectSpecial(prev_with_children, level, m_dir_forw, ins_grp_step, btw_grps);
				}

				middle_items = middle_groups = 0;
			}

			if (item == first_with_children)
				first_with_chld_found = TRUE;
			if (item == last_with_children)
				last_with_chld_found = TRUE;

			prev_with_children = item;
		}
	}

	// Stage 3. Process left and right childless items if any.
	if (left_items > 0 || right_items > 0)
	{
		double left_angle = GetDirectionValue(first_with_children, m_root);
		double right_angle = GetDirectionValue(last_with_children, m_root);
		double len_avail = GetAngleValue(right_angle, left_angle)*radius;

		//
		// NB: linear step inside the group can be only m_inside_group_arc_step because at least one group
		// is existing (end of the tree itself).
		//
		len_avail -= (left_items-left_groups) * m_inside_group_arc_step;

		// Process last_with_children like it is in the "right" group.
		right_items++;
		if (GetNextNeighbor(last_with_children) == NULL)
			right_groups++;

		assert(right_groups != 0);

		len_avail -= (right_items-right_groups) * m_inside_group_arc_step;

		// Calc the step between the groups based on the available length and mode.
		double btw_grps = (len_avail)/(left_groups+right_groups);
		if (btw_grps < m_between_groups_arc_step)
		{
			family_problem = FALSE;
			DebugTrace(L"Bissect placing side items failure.");
			return(FALSE);
		}

		if (full_circle_mode == FALSE)
			btw_grps = m_between_groups_arc_step;

		// Finaly do the placing of the side items.
		if (left_items > 0)
			PlaceBissectSpecial(first_with_children, level, m_dir_backw, m_inside_group_arc_step, btw_grps);
		if (right_items > 1)
			PlaceBissectSpecial(last_with_children, level, m_dir_forw, m_inside_group_arc_step, btw_grps);
	}

	// Everything has succeeded.
	return(TRUE);
}

bool TConcentricModeTreeGraphPlacer::PlaceLayerInSparcedMode(int level, TGenericTreeNode *initial_prev_level_item, double tight_koef, bool &first_family_problem)
{
	double radius = m_lev_radius[level];
	double ins_angle = m_inside_group_arc_step/radius;
	double btw_angle = ins_angle * g_plc_between_groups_extra;
	double left_curr, right_curr;
	int prev_lev = level-1;

	// Step 1. Process the children of the initial item.
	first_family_problem = FALSE;
	int num_chld1 = initial_prev_level_item->NumDirectChildren(FALSE);
	assert(num_chld1 > 0);

	double initial_angle = GetDirectionValue(initial_prev_level_item, m_root);
	left_curr  = initial_angle + (num_chld1-1)*ins_angle/2;
	right_curr = initial_angle - (num_chld1-1)*ins_angle/2;
	double left_dir = GetDirectionValue(radius, left_curr, initial_prev_level_item);
	double right_dir = GetDirectionValue(radius, right_curr, initial_prev_level_item);
	double first_family_algle = GetAngleValue(left_dir, right_dir);

	if (first_family_algle > g_plc_max_sparced_family_angle)
	{
		DebugTrace(L"Outlev initial family angle failure. InititalFamilyRoot=%08lX, nChld=%d, angle=%.1lf(grad).",
					initial_prev_level_item, num_chld1, PLC_GRAD(first_family_algle));
		first_family_problem = TRUE;
		return(FALSE);
	}

	//
	// Place initial family item.
	//
	//

#ifdef DEEP_TRACE_CONCENTRIC_PLACING
	DebugTrace(L"PlaceLayerInSparcedMode. InititalFamilyRoot=%08lX, nChld=%d, left_side_dir=%.1lf(grad), right_side_dir=%.1lf(grad).",
				initial_prev_level_item, num_chld1, PLC_GRAD(left_dir), PLC_GRAD(right_dir));
#endif

	PlaceSparcedSpecial(GetFirstChld(initial_prev_level_item), level, left_curr, m_dir_forw);

	// Step 2. Propagate the wave to the right.
	m_iter.StartIteration(initial_prev_level_item, prev_lev, m_dir_forw);
	for (m_iter.StepIterationAtLevel(prev_lev, m_dir_forw); m_iter.CheckIteration(); m_iter.StepIterationAtLevel(prev_lev, m_dir_forw))
	{
		TGenericTreeNode *item = m_iter.GetCurrentItem();
		int nchld = item->NumDirectChildren(FALSE);
		if (nchld == 0)
			continue;

		right_curr -= btw_angle;

		// Find the best place for the family on the circle.
		double item_drct  = GetDirectionValue(item, m_root);
		double beam_width = (nchld-1)*ins_angle;
		double best_left  = NormalizeAngleRightTo(item_drct+beam_width/2, left_curr);
		if (best_left < right_curr)
		{
			right_curr = best_left + (right_curr-best_left)*tight_koef;
		}

		// Check that the place on the circe is not too far from the family root node.
		double left_side_drct = GetDirectionValue(radius, right_curr, item);
		if (GetDirectionsDifference(left_side_drct, item_drct) > g_plc_max_sparced_incln_angle)
		{
			DebugTrace(L"OutlevRightWave first angle failure. familyRoot=%08lX, nChld=%d, familyRootDrct=%.1lf(grad), left_side_drct=%.1lf(grad).",
						item, nchld, PLC_GRAD(item_drct), PLC_GRAD(left_side_drct));
			return(FALSE);
		}

		double right_side_drct = GetDirectionValue(radius, right_curr-beam_width, item);
		if (GetDirectionsDifference(right_side_drct, item_drct) > g_plc_max_sparced_incln_angle)
		{
			DebugTrace(L"OutlevRightWave second angle failure. familyRoot=%08lX, nChld=%d, familyRootDrct=%.1lf(grad), right_side_drct=%.1lf(grad).",
						item, nchld, PLC_GRAD(item_drct), PLC_GRAD(right_side_drct));
			return(FALSE);
		}

		// Place the items on the circle.
		PlaceSparcedSpecial(GetFirstChld(item), level, right_curr, m_dir_forw);
		right_curr -= beam_width;
	}

	// Step 3. Propagate the wave to the left.
	m_iter.StartIteration(initial_prev_level_item, prev_lev, m_dir_backw);
	for (m_iter.StepIterationAtLevel(prev_lev, m_dir_backw); m_iter.CheckIteration(); m_iter.StepIterationAtLevel(prev_lev, m_dir_backw))
	{
		TGenericTreeNode *item = m_iter.GetCurrentItem();
		int nchld = item->NumDirectChildren(FALSE);
		if (nchld == 0)
			continue;

		left_curr += btw_angle;

		// Find the best place for the family on the circle.
		double item_drct  = GetDirectionValue(item, m_root);
		double beam_width = (nchld-1)*ins_angle;
		double best_right = NormalizeAngleLeftTo(item_drct-beam_width/2, right_curr);
		if (best_right > left_curr)
		{
			left_curr = best_right - (best_right-left_curr)*tight_koef;
		}

		// Ensure that the place on the circe is not too far from the family root node.
		double right_side_drct = GetDirectionValue(radius, left_curr, item);
		if (GetDirectionsDifference(right_side_drct, item_drct) > g_plc_max_sparced_incln_angle)
		{
			DebugTrace(L"OutlevLeftWave first angle failure. familyRoot=%08lX, nChld=%d, familyRootDrct=%.1lf, right_side_drct=%.1lf.",
						item, nchld, PLC_GRAD(item_drct), PLC_GRAD(right_side_drct));
			return(FALSE);
		}

		double left_side_drct = GetDirectionValue(radius, left_curr+beam_width, item);
		if (GetDirectionsDifference(left_side_drct, item_drct) > g_plc_max_sparced_incln_angle)
		{
			DebugTrace(L"OutlevLeftWave second angle failure. familyRoot=%08lX, nChld=%d, familyRootDrct=%.1lf, left_side_drct=%.1lf.",
						item, nchld, PLC_GRAD(item_drct), PLC_GRAD(left_side_drct));
			return(FALSE);
		}

		// Place the items on the circle.
		PlaceSparcedSpecial(GetLastChld(item), level, left_curr, m_dir_backw);
		left_curr += beam_width;
	}

	// Step 4. Verify that these waves have not overlapped.
	double waves_diff = left_curr - right_curr;
	if (waves_diff > 2*M_PI-btw_angle+0.000001)
	{
		DebugTrace(L"Outlev placing waves overlap failure. Waves_Diff=%.1lf.", PLC_GRAD(waves_diff));
		return(FALSE);
	}

	// Everything went fine.
	return(TRUE);
}

double TConcentricModeTreeGraphPlacer::EnlargeRadiuses(int level)
{
	assert(level != 0);

	double increase_value = (level == m_prev_radius_inc_level) ? m_prev_radius_inc_value + m_min_radius_step/4 : m_min_radius_step/8;
	double prev_radius = 0;
	for (int lev1 = level; lev1 < m_lev_radius.NumItems(); ++lev1)
	{
		assert(m_lev_radius[lev1] != 0);
		if (prev_radius != 0 && prev_radius + m_min_radius_step <= m_lev_radius[lev1])
			break;

		m_lev_radius[lev1] += increase_value;
		prev_radius = m_lev_radius[lev1];
	}

	m_prev_radius_inc_level = level;
	m_prev_radius_inc_value = increase_value;
	return(increase_value);
}

void TConcentricModeTreeGraphPlacer::PlaceNode(TGenericTreeNode *item, double radius, double angle)
{
#ifdef DEEP_TRACE_CONCENTRIC_PLACING
	DebugTrace(L"PlaceNode: item=%08lX, radius=%.1lf, angle=%.1lf(grad).", item, radius, PLC_GRAD(angle));
#endif

	long pos_x = (long)(radius*cos(angle));
	long pos_y = (long)(-radius*sin(angle));
	item->SetPosition(pos_x, pos_y);
}

double TConcentricModeTreeGraphPlacer::GetDirectionValue(TGenericTreeNode *dir_to_item, TGenericTreeNode *dir_from_item)
{
	long pos_to_x, pos_to_y, pos_from_x, pos_from_y;
	dir_to_item->GetPosition(pos_to_x, pos_to_y);
	dir_from_item->GetPosition(pos_from_x, pos_from_y);
	return(atan2((double)(pos_from_y-pos_to_y), (double)(pos_to_x-pos_from_x)));
}

double TConcentricModeTreeGraphPlacer::GetDirectionValue(double radius, double angle, TGenericTreeNode *dir_from_item)
{
	double pos_to_x = radius*cos(angle);
	double pos_to_y = -radius*sin(angle);
	long   pos_from_x, pos_from_y;
	dir_from_item->GetPosition(pos_from_x, pos_from_y);
	return(atan2(pos_from_y-pos_to_y, pos_to_x-pos_from_x));
}

void TConcentricModeTreeGraphPlacer::PlaceBissectSpecial(TGenericTreeNode *border_with_childen, int level,
														TNodesIterationDirection dir, double step_ins_grp, double step_btw_grps)
{
	double radius = m_lev_radius[level];
	double angle  = GetDirectionValue(border_with_childen, m_root);
	double insd_grp_angle = step_ins_grp/radius;
	double btw_grps_angle = step_btw_grps/radius;

	// Iterate childless visible items of the level.
	TGenericTreeNodesIterator local_iter(m_root, FALSE, this);
	local_iter.StartIteration(border_with_childen, level, dir);
	for (local_iter.SeekToLevel(level, dir); local_iter.CheckIteration(); local_iter.StepIterationAtLevel(level, dir))
	{
		TGenericTreeNode *item = local_iter.GetCurrentItem();
		if (item != border_with_childen)
		{
			// Current item is not the initial one.
			if (GetFirstChld(item) != NULL)
				break;

			PlaceNode(item, radius, angle);
		}

		if (dir == ndi_dir_forward)
		{
			angle -= (GetNextNeighbor(item) != NULL) ? insd_grp_angle : btw_grps_angle;
		}
		else
		{
			angle += (GetPrevNeighbor(item) != NULL) ? insd_grp_angle : btw_grps_angle;
		}
	}
}

void TConcentricModeTreeGraphPlacer::PlaceSparcedSpecial(TGenericTreeNode *family_beg, int level, double initial_angle, TNodesIterationDirection dir)
{
	double radius = m_lev_radius[level];
	double angle  = initial_angle;
	double angle_step = m_inside_group_arc_step/radius;

	// Iterate all visible items of this family of chidren.
	TGenericTreeNode *item = family_beg;
	while (item != NULL)
	{
		PlaceNode(item, radius, angle);
		if (dir == m_dir_forw)
		{
			item = GetNextNeighbor(item);
			angle -= angle_step;
		}
		else
		{
			item = GetPrevNeighbor(item);
			angle += angle_step;
		}
	}
}

// -----------------------------------------------------------------------------
//   ===============  TRectLinesModeTreeGraphPlacer  ====================
// -----------------------------------------------------------------------------

TRectLinesModeTreeGraphPlacer::TRectLinesModeTreeGraphPlacer(TGenericConsoleEx *pCons)
			: TBasicTreeGraphPlacer(pCons)
{
	m_root = NULL;
	m_iter1.SetPlacer(this);
	m_iter2.SetPlacer(this);
	m_iter3.SetPlacer(this);
	m_iter4.SetPlacer(this);
	m_poly_inx_contexts = NULL;
}

bool TRectLinesModeTreeGraphPlacer::DoRectLinesPlacing(HDC hDC, TGenericTreeNode *root_node, TRectLinesModePlacingProps &props_data, void **contexts_array)
{
	m_root = root_node;
	m_iter1.SetRootNode(root_node);
	m_iter2.SetRootNode(root_node);
	m_iter3.SetRootNode(root_node);
	m_iter4.SetRootNode(root_node);

	// Move params of the method into the data fields of the class to make them accessible from other methods.
	m_poly_inx_contexts = contexts_array;
	m_props = props_data;

	// Display the start of the process to the user.
	DebugTrace(L"TRectLinesModeTreeGraphPlacer::DoPlacing");
	ShowMajorStatus(L"Placing ...");

	// Allocate an array for storing the info about levels of the tree and fill in the root slot. Note that logical
	// "topmost layer" always contains only one node.
	TStructsArray<TPcbPlacerLevelInfo, 50, 100> lni;
	lni.SetNumItems(1);
	lni[0].max_item_height = GetHeight(hDC, m_root);
	lni[0].max_num_children = m_root->NumDirectChildren(FALSE);

	// Step 1a. Figure out the heights of the stripes for placing tree nodes and the required spacing between
	// the stripes for all levels of the tree.
	for (m_iter1.StartIteration(); m_iter1.CheckIteration(); m_iter1.StepIteration())
	{
		TGenericTreeNode *item = m_iter1.GetCurrentItem();
		int clev = m_iter1.GetCurrentLevel();

		if (clev >= lni.NumItems())
		{
			// New level of the tree is discovered. Reserve a record in the array for this new layer.
			if (lni.ReserveTotalBufferSpace(clev+1) == FALSE)
			{
				ShowOutOfMemoryErrorEx();
				return(FALSE);
			}

			lni.SetNumItems(clev+1);

			// Fill in the level info with initial values.
			lni[clev].max_item_height = 0;
			lni[clev].max_num_children = 0;
		}

		// Process the max props of the level.
		long hgt = GetHeight(hDC, item);
		if (hgt > lni[clev].max_item_height)
			lni[clev].max_item_height = hgt;

		long nch = item->NumDirectChildren(FALSE);
		if (nch > lni[clev].max_num_children)
			lni[clev].max_num_children = nch;
	}

	// Step 1b. Assign vertical positions to the levels. All required information is available in the variables
	// that describe the levels of the tree. This action is not modifying the tree itself.
	long curr_y_normal = lni[0].vert_pos_nornal = 0;
	long curr_y_reversed = lni[0].vert_pos_root_down = 0;
	for (int c1=1; c1<lni.NumItems(); ++c1)
	{
		long hgt_horz_links = (lni[c1-1].max_num_children-1)*m_props.vert_lines_step;
		curr_y_normal += lni[c1-1].max_item_height + m_props.off_bottom + hgt_horz_links + m_props.off_top;
		curr_y_reversed -= lni[c1].max_item_height + m_props.off_bottom + hgt_horz_links + m_props.off_top;
		lni[c1].vert_pos_nornal = curr_y_normal;
		lni[c1].vert_pos_root_down = curr_y_reversed;
	}

	// Step 2. This is major step. Process items of the tree level by level from the bottom to the top.
	for (int c2=lni.NumItems()-1; c2 >= 1; --c2)
	{
		long pos_x = 0;
		long layer_pos_y = (m_props.up_side_down_placing == FALSE) ? lni[c2].vert_pos_nornal : lni[c2].vert_pos_root_down;

		// Iterate items that belong to the current level.
		m_iter1.StartIteration(NULL, c2);
		for (m_iter1.SeekToLevel(c2); m_iter1.CheckIteration(); m_iter1.StepIterationAtLevel(c2))
		{
			// Prepare the vert position that centers the item vertically on the stripe that halds the items of the layer.
			long pos_y_adjusted = layer_pos_y+(lni[c2].max_item_height-GetHeight(hDC, m_iter1.GetCurrentItem()))/2;

			// Shift the item or its children right if needed.
			pos_x = SetupItemPos(hDC, m_iter1.GetCurrentItem(), c2, pos_x, pos_y_adjusted);
		}
	}

	// Process the root object separately. When the root has no children (rare case) place it at the horz position 0.
	// In regular case pass the proposed horizontal position as -1. This will force shifting the root to the right above
	// its children and will most likely assign some positive value to its horz position.
	long root_pos_x = (lni[0].max_num_children == 0) ? 0 : -1;
	SetupItemPos(hDC, m_root, 0, root_pos_x, 0);

	// Step 3. Propagate fixes in the horz positions of the objects and misc stuffs.
	UpdateHorzPositions(m_root, 0);
	FixChildlessFamilyMembersGaps(m_root);

	// Step 4. Setup the upper links of the children.
	SetupChildLinks(hDC, m_root, GetHorzLinksBaseHorzLevel(lni[0]));
	for (m_iter1.StartIteration(); m_iter1.CheckIteration(); m_iter1.StepIteration())
	{
		SetupChildLinks(hDC, m_iter1.GetCurrentItem(), GetHorzLinksBaseHorzLevel(lni[m_iter1.GetCurrentLevel()]));
	}

	// Discharge the placer object.
	m_poly_inx_contexts = NULL;
	return(m_iter1.CheckIterationSuccess() == TRUE && m_iter2.CheckIterationSuccess() == TRUE && m_iter3.CheckIterationSuccess() == TRUE && m_iter4.CheckIterationSuccess() == TRUE);
}

// This is static method.
bool TRectLinesModeTreeGraphPlacer::DisplayGraph(TScreenItemsViewObject &view_helper, TList &screen_items_list,
												TGenericTreeNode *root_node, TRectLinesModePlacingProps &placing_props, void **contexts_array,
												int num_visible_objects_ctrl_id, int *num_visible_objects, bool show_prepared_tree,
												TGenericConsoleEx *pCons)
{
	// The graph should not be empty and the viewer should already have the control.
	assert(root_node != NULL);
	assert(view_helper.GetControl() != NULL);

	screen_items_list.Clear();

	// Remove the old picture from the screen items view control.
	if (show_prepared_tree == TRUE)
	{
		view_helper.ShowDataSet(&screen_items_list, TRUE);
	}

	if (num_visible_objects != NULL)
		*num_visible_objects = 0;

	// Clear the num_items control. It may reflect the old state of the tree.
	HWND hViewHelperParent = ::GetParent(view_helper.GetControl());
	if (num_visible_objects_ctrl_id != 0 && hViewHelperParent != NULL)
	{
		::SetDlgItemTextW(hViewHelperParent, num_visible_objects_ctrl_id, NULL);
	}

	// Instantiate the local placer and the iterator.
	TRectLinesModeTreeGraphPlacer placer(pCons);
	TGenericTreeNodesIterator iter(root_node, FALSE, &placer);

	// Update the status panel.
	placer.ShowMajorStatus(L"Placing objects ...");
	placer.ClearMinorStatus();

	HDC hDC = ::GetDC(view_helper.GetControl());
	if (hDC == NULL)
	{
		placer.ShowErrorMessage(L"Error getting window DC.");
		return(FALSE);
	}

	// Prepare all nodes for placing.
	root_node->PrepareForPlacing(hDC, plc_mode_rect_lines, TRUE, contexts_array[root_node->m_poly_inx]);
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TGenericTreeNode *item = iter.GetCurrentItem();
		item->PrepareForPlacing(hDC, plc_mode_rect_lines, FALSE, contexts_array[item->m_poly_inx]);
	}

	// Position the structure.
	if (placer.DoRectLinesPlacing(hDC, root_node, placing_props, contexts_array) == FALSE)
	{
		placer.ShowErrorMessage(L"Graph placing code has failed to place the graph.");
		::ReleaseDC(view_helper.GetControl(), hDC);
		return(FALSE);
	}

	// Update the status panel.
	placer.ShowMajorStatus(L"Preparing tree ...");
	placer.ClearMinorStatus();

	// Prepare the screen items.
	root_node->PrepareRectLinesModeScreenItems(hDC, placing_props, contexts_array[root_node->m_poly_inx]);
	int num_objects = 1;
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TGenericTreeNode *item = iter.GetCurrentItem();
		item->PrepareRectLinesModeScreenItems(hDC, placing_props, contexts_array[item->m_poly_inx]);
		num_objects++;
	}

	// Link all prepared link screen items into the painting list.
	root_node->ShowLinkScreenItems(plc_mode_rect_lines, screen_items_list);
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TGenericTreeNode *item = iter.GetCurrentItem();
		item->ShowLinkScreenItems(plc_mode_rect_lines, screen_items_list);
	}

	// Link all prepared body screen items into the painting list.
	root_node->ShowBkgrScreenItems(plc_mode_rect_lines, screen_items_list);
	root_node->ShowFrgrScreenItems(plc_mode_rect_lines, screen_items_list);
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TGenericTreeNode *item = iter.GetCurrentItem();
		item->ShowBkgrScreenItems(plc_mode_rect_lines, screen_items_list);
		item->ShowFrgrScreenItems(plc_mode_rect_lines, screen_items_list);
	}

	if (show_prepared_tree == TRUE)
	{
		// Show new contents of the screen items list.
		view_helper.ShowDataSet(&screen_items_list, FALSE);
		view_helper.ScrollDataSetIntoCenter();
	}

	if (num_visible_objects != NULL)
		*num_visible_objects = num_objects;

	if (num_visible_objects_ctrl_id != 0 && hViewHelperParent != NULL)
	{
		wchar_t buffer[40];
		::SetDlgItemTextW(hViewHelperParent, num_visible_objects_ctrl_id, FormatInt64(num_objects, buffer, 40));
	}

	// Update the status panel.
	placer.ShowMajorStatus(NULL);
	placer.ClearMinorStatus();

	// Final actions.
	::ReleaseDC(view_helper.GetControl(), hDC);
	return(TRUE);
}

long TRectLinesModeTreeGraphPlacer::SetupItemPos(HDC hDC, TGenericTreeNode *item, int lev, long pos_x, long pos_y)
{
	//
	//  This method assumes that all objects on all layers below the passed item if any are already placed.
	//  At this stage this means that the following fields contain the following values:
	//
	//		m_placer_data[0]		--	  Deferred horz shift of the item itself and its subtree.
	//		m_placer_data[1]		--	  Min offset from the beg of the current item to the beginning of the next item if any.
	//		m_pos_x, m_pos_y	--	  Preliminary item position.
	//
	//  Note that these fields are not set in the passed item. They will be set in this method.
	//
	void *citem_context = m_poly_inx_contexts[item->m_poly_inx];

	// Check, if the passed item has children or not.
	long width = GetWidth(hDC, item);
	long pos_x_updated = pos_x;
	TGenericTreeNode *ib = item->GetFirstChild(FALSE);
	if (ib != NULL)
	{
		for(;;)
		{
			// Check proposed position of the item against already existing positions of its children.
			TGenericTreeNode *ie = item->GetLastChild(FALSE);
			long xb, xe, dummy_y;
			ib->GetPosition(xb, dummy_y);
			ie->GetPosition(xe, dummy_y);
			xb += ib->m_placer_data[0];
			xe += ie->m_placer_data[0]+GetWidth(hDC, ie);

			if (width < xe-xb)
			{
				// Children are wider than their parent. Ensure that parent stays above its children.
				long diff_frag = ((xe-xb)-width)/5;
				if (pos_x < xb)
				{
					long new_pos_x = xb+diff_frag;
					if (pos_x_updated >= new_pos_x)
						break;

					// Shift the item right.
					pos_x_updated = new_pos_x;
				}
				else if (pos_x+width > xe)
				{
					// Shift children of the item to the right.
					PrepareItemsShift(ib, lev+1, (pos_x+width)-xe+diff_frag);
				}
			}
			else if (width > xe-xb)
			{
				// The item is wider than its children. Verify that children are completely covered by the item.
				long diff_frag = (width-(xe-xb))/3;
				if (pos_x > xb)
				{
					// Shift children of the item right. Do not use the diff_frag value here. It turns out that graps
					// tend to look better with this approach.
					PrepareItemsShift(ib, lev+1, pos_x-xb);
				}
				else if (pos_x+width < xe)
				{
					long new_pos_x = xe-width+diff_frag;
					if (pos_x_updated >= new_pos_x)
						break;

					// Shift the item right.
					pos_x_updated = new_pos_x;
				}
			}
			else
			{
				// Widths are exactly identical. Ensure that parent is exactly above its children.
				if (pos_x < xb)
				{
					if (pos_x_updated >= xb)
						break;

					pos_x_updated = xb;
				}
				else if (pos_x > xb)
				{
					// Shift children of the item to the right.
					PrepareItemsShift(ib, lev+1, pos_x-xb);
				}
			}

			// Verify potential overlap between the link screen items. Pick up the node to the left of the first child of the current
			// node. If this node is existing, its parent link may collide with the link to the first child of the current node.
			m_iter2.StartIteration(ib, lev+1, ndi_dir_backward);
			m_iter2.StepIterationAtLevel(lev+1, ndi_dir_backward);
			if (m_iter2.CheckIteration() == TRUE)
			{
				// Node to left of the first child is present. Pick up the horz position of its upper link.
				TGenericTreeNode *lnode = m_iter2.GetCurrentItem();
				long lnode_x, lnode_y;
				lnode->GetPosition(lnode_x, lnode_y);
				long lnode_upper_x = lnode_x+lnode->m_placer_data[0]+lnode->GetRectLinesModePlacingParam(hDC, rcln_upper_link_offset, m_props, m_poly_inx_contexts[lnode->m_poly_inx]);

				// Pick up the min good horz position of the link to the first child of the current node.
				long citem_flink_min_x = pos_x_updated + item->GetRectLinesModePlacingParam(hDC, rcln_chld_links_offset, m_props, citem_context);
				citem_flink_min_x -= 3*item->GetRectLinesModePlacingParam(hDC, rcln_chld_links_step, m_props, citem_context);

				if (lnode_upper_x > citem_flink_min_x)
				{
					// The links collide or they are too close to each other. Shift the current node to the right.
					pos_x_updated += lnode_upper_x-citem_flink_min_x;
					continue;
				}
			}

			// The situation with the links is fine.
			break;
		}

		// Iterate children to fix the "small shift" case. Small shift is situation when one of the children has
		// shift that is less than the horz step between the children links below the object. Fix this small shift
		// by either shifting the object itself or by shifting all its children to the right.
		long pos_x1 = pos_x_updated + item->GetRectLinesModePlacingParam(hDC, rcln_chld_links_offset, m_props, citem_context);
		long step_x = item->GetRectLinesModePlacingParam(hDC, rcln_chld_links_step, m_props, citem_context);
		TGenericTreeNode *chld = ib;
		while (chld != NULL)
		{
			// Find the pos_x of the upper link for this child.
			long chld_x, chld_y;
			chld->GetPosition(chld_x, chld_y);
			long upper_x = chld_x+chld->m_placer_data[0]+chld->GetRectLinesModePlacingParam(hDC, rcln_upper_link_offset, m_props, m_poly_inx_contexts[chld->m_poly_inx]);

			if (upper_x > pos_x1 && upper_x <= pos_x1+step_x)
			{
				pos_x_updated += upper_x-pos_x1;
				break;
			}
			else if (upper_x < pos_x1 && upper_x >= pos_x1-step_x)
			{
				PrepareItemsShift(ib, lev+1, pos_x1-upper_x);
				break;
			}

			pos_x1 += step_x;
			chld = chld->GetNextSibling(FALSE);
		}
	}

	// Apply possibly updated position to the item.
	item->SetPosition(pos_x_updated, pos_y);

	// Prepare positioning of the next item on this level.
	long horz_space_after = (item->m_owner == NULL) ? 0 : ((item->GetNextSibling(FALSE) != NULL) ? m_props.horz_step_inside_family : m_props.horz_step_between_families);

	// Initital shift offset of the item itself should be zero.
	item->m_placer_data[0] = 0;
	item->m_placer_data[1] = width+horz_space_after;

	// Return the suggested position for the beginning of the next item if any.
	return(pos_x_updated+width+horz_space_after);
}

void TRectLinesModeTreeGraphPlacer::PrepareItemsShift(TGenericTreeNode *ib, int lev, long shift_x)
{
	assert(ib->m_owner != NULL);
	assert(shift_x >= 0);

	// The current family should be shifted right as a whole.
	TGenericTreeNode *curr_family_chld = ib;
	while (curr_family_chld != NULL)
	{
		curr_family_chld->m_placer_data[0] += shift_x;
		curr_family_chld = curr_family_chld->GetNextSibling(FALSE);
	}

	// Check for the overlaps between subtrees starting from the passed item.
	m_iter2.StartIteration(ib, lev);
	for (m_iter2.SeekToLevel(lev); m_iter2.CheckIteration(); m_iter2.StepIterationAtLevel(lev))
	{
		// Check the item to the right of the current item.
		if (FixPotentialOverlap(m_iter2.GetCurrentItem(), lev, lev) == TRUE)
		{
			// Right item on the current level is present. This means that it is also necessary to
			// iterate rightmost items on every child level that is existing in the current subtree.
			m_iter3.SetRootNode(m_iter2.GetCurrentItem());
			m_iter3.StartIteration(NULL, 0, ndi_dir_backward);
			for (int level_right_side=1; m_iter3.CheckIteration(); ++level_right_side)
			{
				m_iter3.SeekToLevel(level_right_side, ndi_dir_backward);
				if (m_iter3.CheckIteration() == FALSE)
					break;

				// Current subtree contains an item on the current layer.
				if (FixPotentialOverlap(m_iter3.GetCurrentItem(), lev+level_right_side, lev) == FALSE)
					break;
			}
		}
	}
}

bool TRectLinesModeTreeGraphPlacer::FixPotentialOverlap(TGenericTreeNode *item, int item_level, int root_level)
{
	// Pick up the object to the right of the passed item.
	m_iter4.StartIteration(item);
	m_iter4.StepIterationAtLevel(item_level);
	if (m_iter4.CheckIteration() == FALSE)
		return(FALSE);

	// Next item on the curr level is present. Figure out the horz positions of both items.
	TGenericTreeNode *curr_subtr_root = NULL, *right_subtr_root = NULL;
	long curr_subtr_item_x = GetCurrHorzPosAndLocalRoot(item, item_level, root_level, curr_subtr_root);
	long next_subtr_item_x = GetCurrHorzPosAndLocalRoot(m_iter4.GetCurrentItem(), item_level, root_level, right_subtr_root);

	// Check the relation between the horz positions.
	long overlap = curr_subtr_item_x+item->m_placer_data[1]-next_subtr_item_x;
	if (overlap > 0)
		right_subtr_root->m_placer_data[0] += overlap;

	// The right side object is present and it was processed.
	return(TRUE);
}

long TRectLinesModeTreeGraphPlacer::GetCurrHorzPosAndLocalRoot(TGenericTreeNode *item, int item_level, int root_level, TGenericTreeNode *&subtr_root)
{
	long item_x, item_y;
	item->GetPosition(item_x, item_y);

	while (item != NULL && item_level >= root_level)
	{
		item_x += item->m_placer_data[0];
		subtr_root = item;
		item = item->m_owner;
		item_level--;
	}

	return(item_x);
}

long TRectLinesModeTreeGraphPlacer::GetHorzLinksBaseHorzLevel(TPcbPlacerLevelInfo &clni)
{
	if (m_props.up_side_down_placing == FALSE)
	{
		// Upper horz links should start at the bottom of the current items stripe plus the bottom offset.
		return(clni.vert_pos_nornal+clni.max_item_height+m_props.off_bottom);
	}
	else
	{
		// Upper horz links should start at the top of the current items stripe minus offset and max horz links area.
		return(clni.vert_pos_root_down-m_props.off_top-(clni.max_num_children-1)*m_props.vert_lines_step);
	}
}

void TRectLinesModeTreeGraphPlacer::UpdateHorzPositions(TGenericTreeNode *item, long shift_x)
{
	shift_x += item->m_placer_data[0];
	if (shift_x != 0)
	{
		// Current item needs shifting.
		long px, py;
		item->GetPosition(px, py);
		item->SetPosition(px+shift_x, py);
	}

	// Iterate all children regardless if the shift is currently present or not.
	TGenericTreeNode *chld = item->GetFirstChild(FALSE);
	while (chld != NULL)
	{
		UpdateHorzPositions(chld, shift_x);
		chld = chld->GetNextSibling(FALSE);
	}
}

void TRectLinesModeTreeGraphPlacer::FixChildlessFamilyMembersGaps(TGenericTreeNode *item)
{
	TGenericTreeNode *last_childless_chld = NULL;
	TGenericTreeNode *chld = item->GetFirstChild(FALSE);
	bool item_with_childs_noticed = FALSE;
	while (chld != NULL)
	{
		if (chld->GetFirstChild(FALSE) != NULL)
		{
			// Current child has its own children. Fix them.
			FixChildlessFamilyMembersGaps(chld);
			item_with_childs_noticed = TRUE;
		}
		else if (item_with_childs_noticed == FALSE)
		{
			// Current child belongs to the initial serie of childless items.
			last_childless_chld = chld;
		}

		chld = chld->GetNextSibling(FALSE);
	}

	if (last_childless_chld != NULL)
	{
		// Passed item has some seq of leading childless children.
		TGenericTreeNode *curr_chld = last_childless_chld;
		TGenericTreeNode *next_chld = curr_chld->GetNextSibling(FALSE);
		if (next_chld != NULL)
		{
			long next_chld_x, next_chld_y;
			next_chld->GetPosition(next_chld_x, next_chld_y);

			while (curr_chld != NULL)
			{
				long curr_chld_x, curr_chld_y;
				curr_chld->GetPosition(curr_chld_x, curr_chld_y);

				// Calculate the rightmost allowed position for the current child.
				long expected_curr_chld_x = next_chld_x-curr_chld->m_placer_data[1];
				if (curr_chld_x < expected_curr_chld_x)
				{
					curr_chld->SetPosition(expected_curr_chld_x, curr_chld_y);
					expected_curr_chld_x = curr_chld_x;
				}

				// Go on the next sibling to the left.
				curr_chld = curr_chld->GetPrevSibling(FALSE);
				next_chld_x = curr_chld_x;
			}
		}
	}
}

void TRectLinesModeTreeGraphPlacer::SetupChildLinks(HDC hDC, TGenericTreeNode *item, long base_lev_y)
{
	// Check, if passed item has visible children or not.
	TGenericTreeNode *chld = item->GetFirstChild(FALSE);
	if (chld == NULL)
		return;

	// This item has children. Prepare the iteration.
	long px, py;
	item->GetPosition(px, py);
	void *context = m_poly_inx_contexts[item->m_poly_inx];

	long step_x = item->GetRectLinesModePlacingParam(hDC, rcln_chld_links_step, m_props, context);
	long pos_x1 = px + item->GetRectLinesModePlacingParam(hDC, rcln_chld_links_offset, m_props, context);
	long pos_x2 = pos_x1 + (item->NumDirectChildren(FALSE)-1)*step_x;

	long step_y = m_props.vert_lines_step;
	long lev_y1 = base_lev_y;
	long lev_y2 = base_lev_y;

	// Iterate children of the passed item from left to right first.
	int cnt_left = 0;
	TGenericTreeNode *chld_left = NULL;
	TGenericTreeNode *chld_last_left = NULL;
	while (chld != NULL)
	{
		// Find pos_x of the upper link for this child.
		long chld_x, chld_y;
		chld->GetPosition(chld_x, chld_y);
		long upper_x = chld_x+chld->GetRectLinesModePlacingParam(hDC, rcln_upper_link_offset, m_props, m_poly_inx_contexts[chld->m_poly_inx]);

		if (upper_x > pos_x1)
		{
			// This child should be processed in the next loop.
			break;
		}

		// Setup a link either with a left shift or with the straight vertical link.
		chld->m_placer_data[0] = pos_x1;
		chld->m_placer_data[1] = lev_y1;

		if (upper_x < pos_x1)
		{
			cnt_left++;
			chld_last_left = chld;
		}

		pos_x1 += step_x;
		lev_y1 += step_y;

		chld_left = chld;
		chld = chld->GetNextSibling(FALSE);
	}

	if (m_props.up_side_down_placing == TRUE)
	{
		// Replace the vert positions in the links.
		lev_y1 = base_lev_y;
		while (chld_left != NULL)
		{
			chld_left->m_placer_data[1] = lev_y1;
			lev_y1 += step_y;
			chld_left = chld_left->GetPrevSibling(FALSE);
		}
	}

	// Check if all children were iterated.
	int cnt_right = 0;
	if (chld != NULL)
	{
		// Some of the children require link with a right shift.
		TGenericTreeNode *chld_right = chld;
		TGenericTreeNode *chld2 = item->GetLastChild(FALSE);
		while (chld2 != NULL)
		{
			// Setup link with a right shift.
			chld2->m_placer_data[0] = pos_x2;
			chld2->m_placer_data[1] = lev_y2;
			cnt_right++;
			pos_x2 -= step_x;
			lev_y2 += step_y;

			if (chld2 == chld)
				break;

			chld2 = chld2->GetPrevSibling(FALSE);
		}

		if (m_props.up_side_down_placing == TRUE)
		{
			// Replace the vert positions in the links.
			lev_y2 = base_lev_y;
			while (chld_right != NULL)
			{
				chld_right->m_placer_data[1] = lev_y2;
				lev_y2 += step_y;
				chld_right = chld_right->GetNextSibling(FALSE);
			}
		}
	}

	if (cnt_left != 0 && cnt_right != 0)
	{
		// There are links with shift in both directions.
		long half_step = step_y/2;
		if (cnt_left > cnt_right)
		{
			// Shift right links down (up) for 1/2 step.
			while (chld != NULL)
			{
				chld->m_placer_data[1] += half_step;
				chld = chld->GetNextSibling(FALSE);
			}
		}
		else if (cnt_left < cnt_right)
		{
			// Shift left links down (up) for 1/2 step.
			while (chld_last_left != NULL)
			{
				chld_last_left->m_placer_data[1] += half_step;
				chld_last_left = chld_last_left->GetPrevSibling(FALSE);
			}
		}
	}
}


