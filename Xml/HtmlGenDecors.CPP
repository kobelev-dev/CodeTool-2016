//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//	Second version of the HTML generation.
//

#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "Common/HashTable.H"
#include  "Xml/HtmlGenDecors.H"

// ---------------------------------------------------------------------------
//  ==============  THtmlGenDecorOwnersContext  =====================
// ---------------------------------------------------------------------------

void THtmlGenDecorOwnersContext::ResetContext()
{
	for (int ibkt=0; ibkt<NUM_BUCKETS; ++ibkt)
		m_simple_owners[ibkt].DeleteAll();

	m_other_owners.DeleteAll();
}

TRawHtmlElement *THtmlGenDecorOwnersContext::FindOrCreateSimpleOwner(const wchar_t *css_class, const wchar_t *css_style, const wchar_t *img_src)
{
	assert(css_class != NULL || img_src != NULL);

	// Make the hash value out of the passed props.
	unsigned short hash_value = 0;
	if (css_class != NULL)
		hash_value ^= THashTableItem<TSimpleDecorOwner>::CalcStrHash(css_class);
	if (img_src != NULL)
		hash_value ^= THashTableItem<TSimpleDecorOwner>::CalcStrHash(img_src);

	TList &bucket = m_simple_owners[hash_value & (NUM_BUCKETS-1)];

	// Check the hash bucket for an identical object.
	for (TListIter<TSimpleDecorOwner> iter(bucket); iter; ++iter)
	{
		if (iter.CurrItem().CompareProps(css_class, css_style, img_src) == TRUE)
			return(iter.CurrItemPtr());
	}

	// A new object is needed.
	if (img_src != NULL)
	{
		// Add the passed name of the image to the strings dict.
		img_src = m_misc_app_strings.RegisterStr(img_src);
		if (img_src == NULL)
		{
			m_owner_object_creation_errors++;
			return(NULL);
		}
	}

	// Allocate the owner object.
	TSimpleDecorOwner *owner = new TSimpleDecorOwner(css_class, css_style, img_src);
	if (owner == NULL)
	{
		m_owner_object_creation_errors++;
		return(NULL);
	}

	// Store the object and return success.
	bucket.AppendItem(owner);
	return(owner);
}

TRawHtmlElement *THtmlGenDecorOwnersContext::CreateAppDelegateProxy(THtmlGenAppDelegate *app_delegate, const wchar_t *css_class, const wchar_t *css_style, const wchar_t *img_src)
{
	assert(app_delegate != NULL && (css_class != NULL || img_src != NULL));

	if (img_src != NULL)
	{
		// Add the passed name of the image to the strings dict.
		img_src = m_misc_app_strings.RegisterStr(img_src);
		if (img_src == NULL)
		{
			m_owner_object_creation_errors++;
			return(NULL);
		}
	}

	// App delegate proxies are not hashed in any way. A new object is always needed.
	TSimpleDecorOwner *proxy = new TSimpleDecorOwner(css_class, css_style, img_src);
	if (proxy == NULL)
	{
		m_owner_object_creation_errors++;
		return(NULL);
	}

	// Setup the props of the object, store it and return success.
	proxy->m_elem_app_delegate = app_delegate;
	m_other_owners.AppendItem(proxy);
	return(proxy);
}

TRawHtmlElement *THtmlGenDecorOwnersContext::CreateHyperLinkOwner(const wchar_t *href_value)
{
	// Add the passed name of the image to the strings dict.
	wchar_t *persistent_href_value = m_misc_app_strings.RegisterStr(href_value);
	if (persistent_href_value == NULL)
	{
		m_owner_object_creation_errors++;
		return(NULL);
	}

	TAppAttrsDecorOwner *owner = new TAppAttrsDecorOwner(L"a", L"href", persistent_href_value);
	if (owner == NULL)
	{
		m_owner_object_creation_errors++;
		return(NULL);
	}

	// Store the object and return success.
	m_other_owners.AppendItem(owner);
	return(owner);
}

TRawHtmlElement *THtmlGenDecorOwnersContext::CreateMarksAreaLabel(const wchar_t *label_value, const wchar_t *css_class)
{
	// Cache the body of the label.
	wchar_t *persistent_label_value = m_misc_app_strings.RegisterStr(label_value);
	if (persistent_label_value == NULL)
	{
		m_owner_object_creation_errors++;
		return(NULL);
	}

	TSimpleHtmlElement *makrs_area_label = new TSimpleHtmlElement(L"span", persistent_label_value, -1, css_class, crlm_none);
	if (makrs_area_label == NULL)
	{
		m_owner_object_creation_errors++;
		return(NULL);
	}

	// Store the object and return success.
	m_other_owners.AppendItem(makrs_area_label);
	return(makrs_area_label);
}

TRawHtmlElement *THtmlGenDecorOwnersContext::CreateAppAttrsOwner(const wchar_t *attr1_name, const wchar_t *attr1_value, const wchar_t *attr2_name, const wchar_t *attr2_value)
{
	wchar_t *persistent_attr1_value = NULL;
	if (attr1_name != NULL && attr1_value != NULL)
	{
		// Cache the value of the first attribute.
		persistent_attr1_value = m_misc_app_strings.RegisterStr(attr1_value);
		if (persistent_attr1_value == NULL)
		{
			m_owner_object_creation_errors++;
			return(NULL);
		}
	}

	wchar_t *persistent_attr2_value = NULL;
	if (attr2_name != NULL && attr2_value != NULL)
	{
		// Cache the value of the second attribute.
		persistent_attr2_value = m_misc_app_strings.RegisterStr(attr2_value);
		if (persistent_attr2_value == NULL)
		{
			m_owner_object_creation_errors++;
			return(NULL);
		}
	}

	TAppAttrsDecorOwner *app_attrs_owner = new TAppAttrsDecorOwner(L"span", attr1_name, persistent_attr1_value, attr2_name, persistent_attr2_value);
	if (app_attrs_owner == NULL)
	{
		m_owner_object_creation_errors++;
		return(NULL);
	}

	// Store the object and return success.
	m_other_owners.AppendItem(app_attrs_owner);
	return(app_attrs_owner);
}

// ---------------------------------------------------------------------------------------
//  ==============  THtmlGenDecorOwnersContext::TSimpleDecorOwner  ================
// ---------------------------------------------------------------------------------------

THtmlGenDecorOwnersContext::TSimpleDecorOwner::TSimpleDecorOwner(const wchar_t *css_class, const wchar_t *css_style, const wchar_t *img_src)
		: TRawHtmlElement((img_src == NULL) ? L"span" : L"img")
{
	assert(img_src != NULL || css_class != NULL);

	m_elem_css_class = (wchar_t*)css_class;
	m_elem_css_style = (wchar_t*)css_style;
	m_elem_img_src = (wchar_t*)img_src;
	m_elem_app_delegate = NULL;
}

void THtmlGenDecorOwnersContext::TSimpleDecorOwner::EmitSpecificAttrs(THtmlDocument &doc)
{
	if (m_elem_img_src != NULL)
		doc.EmitAttribute(L"src", m_elem_img_src);

	if (m_elem_app_delegate != NULL)
		m_elem_app_delegate->EmitSpecificAttrs(doc);
}

bool THtmlGenDecorOwnersContext::TSimpleDecorOwner::CompareProps(const wchar_t *css_class, const wchar_t *css_style, const wchar_t *img_src)
{
	if (m_elem_css_class == NULL && css_class != NULL || m_elem_css_class != NULL && css_class == NULL)
		return(FALSE);
	else if (m_elem_css_class != NULL && css_class != NULL && wcscmp(m_elem_css_class, css_class) != 0)
		return(FALSE);

	if (m_elem_css_style == NULL && css_style != NULL || m_elem_css_style != NULL && css_style == NULL)
		return(FALSE);
	else if (m_elem_css_style != NULL && css_style != NULL && wcscmp(m_elem_css_style, css_style) != 0)
		return(FALSE);

	if (m_elem_img_src == NULL && img_src != NULL || m_elem_img_src != NULL && img_src == NULL)
		return(FALSE);
	else if (m_elem_img_src != NULL && img_src != NULL && wcscmp(m_elem_img_src, img_src) != 0)
		return(FALSE);

	// The properties are identical.
	return(TRUE);
}

// ---------------------------------------------------------------------------------------
//  =============  THtmlGenDecorOwnersContext::TAppAttrsDecorOwner  ================
// ---------------------------------------------------------------------------------------

void THtmlGenDecorOwnersContext::TAppAttrsDecorOwner::EmitSpecificAttrs(THtmlDocument &doc)
{
	if (m_attr1_name != NULL)
		doc.EmitAttribute(m_attr1_name, m_attr1_value);

	if (m_attr2_name != NULL)
		doc.EmitAttribute(m_attr2_name, m_attr2_value);
}

//----------------------------------------------------------------------------------
//	===========  THtmlGenericDecoratedArea::TDecoratedAreaProxy  ==============
//----------------------------------------------------------------------------------

void THtmlGenericDecoratedArea::TDecoratedAreaProxy::EmitSpecificAttrs(THtmlDocument &doc)
{
	if (m_colspan_buff[0] != 0)
		doc.EmitAttribute(L"colspan", m_colspan_buff);

	m_base_element->EmitSpecificAttrs(doc);
}

void THtmlGenericDecoratedArea::TDecoratedAreaProxy::PrepareExtAttrs(THtmlObjectEmittingHelper &hlpr, const wchar_t *spec_tab_css_style, short spec_tab_num_clmns)
{
	if (spec_tab_css_style != NULL)
	{
		// Non NULL style from the spec tab affects the current subcolumn and all subsequent subcolumns.
		hlpr.m_curr_css_class = (wchar_t*)spec_tab_css_style;
	}

	// Setup the default values.
	m_css_class_attr = hlpr.m_curr_css_class;
	m_css_style_attr = NULL;
	m_colspan_buff[0] = 0;

	// Check for the special cases first.
	if (hlpr.m_num_subcolumns <= 1)
	{
		// The subcolumns are not enabled. The spec tabs if any should be ignored.
		return;
	}
	else if (hlpr.m_curr_line_subcolumns <= 1)
	{
		// There is only one cell on the line that spans over all subcolumns.
		swprintf(m_colspan_buff, 16, L"%d", hlpr.m_num_subcolumns);
		return;
	}

	if (spec_tab_num_clmns < 2)
		spec_tab_num_clmns = 1;

	assert(hlpr.m_curr_line_subcolumns <= hlpr.m_num_subcolumns);
	assert(hlpr.m_inx_subcolumn+spec_tab_num_clmns <= hlpr.m_curr_line_subcolumns);

	// Both the subcolumns and the spec tabs are present. Figure out the current type of subcolumn.
	if (hlpr.m_inx_subcolumn == 0)
	{
		// This is the first subcolumn.
		m_css_style_attr = L"margin-right:0;padding-right:0;width:0px;";
		if (spec_tab_num_clmns > 1)
			swprintf(m_colspan_buff, 16, L"%d", spec_tab_num_clmns);
	}
	else if (hlpr.m_inx_subcolumn < hlpr.m_curr_line_subcolumns-1)
	{
		// This is an intermediate subcolumn.
		m_css_style_attr = L"padding-left:0;margin-left:0;margin-right:0;padding-right:0;width:0px;";
		if (spec_tab_num_clmns > 1)
			swprintf(m_colspan_buff, 16, L"%d", spec_tab_num_clmns);
	}
	else
	{
		// This is the last subcolumn on the current line.
		m_css_style_attr = L"padding-left:0;margin-left:0;width:100%;";
		if (hlpr.m_inx_subcolumn < hlpr.m_num_subcolumns-1)
		{
			// The layout of the table expects more subcolumns.
			swprintf(m_colspan_buff, 16, L"%d", hlpr.m_num_subcolumns-hlpr.m_inx_subcolumn);
		}
	}

	// Increment the subcolumn index.
	hlpr.m_inx_subcolumn += spec_tab_num_clmns;
}

//----------------------------------------------------------------------------------
//	====================  THtmlGenericDecoratedArea  =====================
//----------------------------------------------------------------------------------

void THtmlGenericDecoratedArea::ResetDecorations()
{
	m_bkgr_decors.SetNumItems(0);
	m_frgr_decors.SetNumItems(0);
	m_spaceless_decors.SetNumItems(0);

	m_initial_css_style = NULL;
	m_first_subclmn_width = 1;
}

bool THtmlGenericDecoratedArea::SetBkgrDecor(TRawHtmlElement *owner, short zorder, long area_beg, long area_len)
{
	// The owner of the background decor cannot be NULL.
	assert(owner != NULL);

	// Check/ensure that the passed decoration area is not empty.
	if (area_len <= 0)
		return(TRUE);

	THtmlBasicDecor decor = { owner, zorder, area_beg, area_len };
	int insert_before_inx = 0;

	if (m_bkgr_decors.NumItems() > 0 && THtmlBasicDecor::CompareAsBkgrDecors(decor, m_bkgr_decors[0]) >= 0)
	{
		// Find the right place to insert the new decor.
		long lo_good = 0;
		long hi_bad = m_bkgr_decors.NumItems();
		while ((hi_bad - lo_good) > 1)
		{
			long inx_test = (hi_bad+lo_good)/2;
			if (THtmlBasicDecor::CompareAsBkgrDecors(decor, m_bkgr_decors[inx_test]) >= 0)
				lo_good = inx_test;
			else hi_bad = inx_test;
		}

		// Insert decor right after the smaller or equal item.
		insert_before_inx = lo_good+1;
	}

	// Insert or append the record. Method below supports both cases.
	return(m_bkgr_decors.InsertItem(insert_before_inx, decor));
}

bool THtmlGenericDecoratedArea::SetFrgrDecor(TRawHtmlElement *owner, short zorder, long area_beg, long area_len)
{
	// Foreground decor owner can be NULL. This means that the area should be excluded from rendering.
	// Still check/ensure that the passed decoration area is not empty.
	if (area_len <= 0)
		return(TRUE);

	// Add object to the right place into the list of frgr decors.
	int ins_before_inx = m_frgr_decors.NumItems();
	for (int inx=0; inx<m_frgr_decors.NumItems(); ++inx)
	{
		if (m_frgr_decors[inx].m_zorder > zorder)
		{
			ins_before_inx = inx;
			break;
		}
	}

	// Insert or append the record. The method below supports both cases.
	 THtmlBasicDecor decor = { owner, zorder, area_beg, area_len };
	 return(m_frgr_decors.InsertItem(ins_before_inx, decor));
}

bool THtmlGenericDecoratedArea::SetSpacelessDecor(TRawHtmlElement *owner, short horz_order, long decor_offs)
{
	THtmlSpacelessDecor decor = { owner, decor_offs, horz_order, 0, NULL };
	return(AddSpacelessDecorInternal(decor));
}

bool THtmlGenericDecoratedArea::SetSpecialTab(short horz_order, long decor_offs, const wchar_t *spec_tab_bkgr, short spec_tab_num_clmns)
{
	THtmlSpacelessDecor decor = { NULL, decor_offs, horz_order, spec_tab_num_clmns, (wchar_t*)spec_tab_bkgr };
	return(AddSpacelessDecorInternal(decor));
}

bool THtmlGenericDecoratedArea::EmitArea(THtmlObjectEmittingHelper &emt_ctx, const wchar_t *body_data_ptr, long body_data_len, long area_beg, long area_len)
{
	assert(body_data_len >= 0);
	assert(area_beg >= 0 && area_len >= 0);
	assert(area_beg+area_len <= body_data_len);

	// Setup the rendering context with the props of the current line.
	emt_ctx.PreprareForRendering(m_initial_css_style, NumSubColumns());

	// Emit the initial opening tag. Note that the area is emitted in a full form even if it has no body.
	TDecoratedAreaProxy element_proxy(this);
	element_proxy.PrepareExtAttrs(emt_ctx, NULL, m_first_subclmn_width);
	emt_ctx.m_doc.EmitElementBeg(element_proxy);

	// Check for the top level cases.
	bool overall_alloc_success = TRUE;
	if (m_bkgr_decors.NumItems() == 0 && m_frgr_decors.NumItems() == 0 && m_spaceless_decors.NumItems() == 0)
	{
		// Current area does not have decorations. Simply emit the body if any.
		if (area_len > 0)
			emt_ctx.m_doc.EmitRawData(body_data_ptr+area_beg, area_len, FALSE, FALSE, emt_ctx.m_data_emit_mode);
	}
	else if (area_len == 0)
	{
		// Current area has empty body. Process spaceless decors if any. Do not check the area offset for simplicity.
		for (int isp2=0; isp2<m_spaceless_decors.NumItems(); ++isp2)
			ProcessSpacelessDecor(emt_ctx, element_proxy, isp2);
	}
	else
	{
		// Current area has non empty body and decorations. Verify/extend the length of the overlap bufffer.
		if (emt_ctx.m_overlap_buffer.NumAllocedItems() < area_len)
		{
			// Enlarge the array of pointers.
			if (emt_ctx.m_overlap_buffer.ReserveTotalBufferSpace(area_len) == FALSE)
			{
				emt_ctx.m_doc.EmitElementEnd(element_proxy);
				return(FALSE);
			}
		}

		// Clear the frgr decors overlap map.
		for (int ich1=0; ich1<area_len; ++ich1)
			emt_ctx.m_overlap_buffer[ich1] = NULL;

		// Prepare the character decorations map.
		for (int inx=0; inx<m_frgr_decors.NumItems(); ++inx)
		{
			THtmlBasicDecor &dcr = m_frgr_decors[inx];
			assert(dcr.m_decor_area_beg >= area_beg);
			assert(dcr.m_decor_area_beg+dcr.m_decor_area_len <= area_len);

			int offs = dcr.m_decor_area_beg-area_beg;
			for (int ich2=0; ich2<dcr.m_decor_area_len; ++ich2)
			{
				// Offset in the decoration was verified above using the asserts. Nevertheless recheck this
				// condition using the regular if statement.
				if (offs >= 0 && offs < area_len)
					emt_ctx.m_overlap_buffer[offs++] = &dcr;
			}
		}

		// Run the main loop.
		int ib = 0;
		int ibkgr = 0;
		int ispaceless = 0;
		while (ib < area_len)
		{
			// Check for bkgr decors that should be closed before rendering the current symbol.
			int inx_temp_closed_bottom = emt_ctx.m_opened_bkgr_decors.NumItems();
			int ilayer1 = inx_temp_closed_bottom-1;
			while (ilayer1 >= 0)
			{
				int perm_close_layer = emt_ctx.FindBkgrEndLayerAt(ilayer1, area_beg+ib);
				if (perm_close_layer < 0)
					break;

				// Layer, that should be permanently closed, is present at the current character offset.
				// Close this layer and all layers, that stay above this layer.
				while (ilayer1 >= perm_close_layer)
				{
					// The decor should be exited, maybe temporarily.
					emt_ctx.m_doc.EmitElementEnd(*emt_ctx.m_opened_bkgr_decors[ilayer1]->m_basic_owner);
					ilayer1--;
				}

				// Remove the item from the stack of the opened layers.
				emt_ctx.m_opened_bkgr_decors.ReleaseItem(perm_close_layer);
				inx_temp_closed_bottom = perm_close_layer;
			}

			// Process spaceless decors that should stay outside of the bkgr decors.
			while (CheckSpacelessAt(ispaceless, area_beg+ib, THtmlSpacelessDecor::BKGR_COVER_RIGHT-1) == TRUE)
				ProcessSpacelessDecor(emt_ctx, element_proxy, ispaceless++);

			// Check for bkgr decors that should be entered before rendering the current symbol.
			if (CheckBkgrBegAt(ibkgr, area_beg+ib) == TRUE)
			{
				// Unprocessed bkgr decor starts right in front of the current symbol. Fugure out how its zorder
				// correlates to the zorder of the currently opened bkgr decors.
				int ib_to_close = emt_ctx.FindInsBeforeIndex(m_bkgr_decors[ibkgr].m_zorder);

				// Make the first walk down the stack temporarily closing some of the layers.
				int ilayer2 = inx_temp_closed_bottom-1;
				while (ilayer2 >= ib_to_close)
				{
					// Bkgr decor should be temporarily closed.
					emt_ctx.m_doc.EmitElementEnd(*emt_ctx.m_opened_bkgr_decors[ilayer2]->m_basic_owner);
					inx_temp_closed_bottom = ilayer2;
					ilayer2--;
				}

				// Do the second walk up the stack, reopening layers and inserting new ones.
				while (CheckBkgrBegAt(ibkgr, area_beg+ib) == TRUE)
				{
					THtmlBasicDecor *dcr = &m_bkgr_decors[ibkgr++];
					int ibefore_to_open = emt_ctx.FindInsBeforeIndex(dcr->m_zorder);
					while (inx_temp_closed_bottom < ibefore_to_open)
					{
						emt_ctx.m_doc.EmitElementBeg(*emt_ctx.m_opened_bkgr_decors[inx_temp_closed_bottom]->m_basic_owner);
						inx_temp_closed_bottom++;
					}

					if (emt_ctx.m_opened_bkgr_decors.InsertItem(ibefore_to_open, dcr) == FALSE)
						overall_alloc_success = FALSE;
				}
			}

			// Reopen the rest of the layers, some of them maybe for the first time.
			while (inx_temp_closed_bottom < emt_ctx.m_opened_bkgr_decors.NumItems())
			{
				emt_ctx.m_doc.EmitElementBeg(*emt_ctx.m_opened_bkgr_decors[inx_temp_closed_bottom]->m_basic_owner);
				inx_temp_closed_bottom++;
			}

			// Check for spaceless decors that should be processed before rendering the current symbol.
			while (CheckSpacelessAt(ispaceless, area_beg+ib) == TRUE)
				ProcessSpacelessDecor(emt_ctx, element_proxy, ispaceless++);

			// Find consequtive area that belongs to the same frgr decoration, does not contain bkgr state changes
			// and is not interrupted by the spaceless decorations.
			THtmlBasicDecor *frgr_area = emt_ctx.m_overlap_buffer[ib];

			int ic = ib+1;
			int last_layer_index = emt_ctx.m_opened_bkgr_decors.NumItems()-1;
			while (ic < area_len)
			{
				if (frgr_area != emt_ctx.m_overlap_buffer[ic])
					break;
				else if (CheckBkgrBegAt(ibkgr, area_beg+ic) == TRUE)
					break;
				else if (emt_ctx.FindBkgrEndLayerAt(last_layer_index, area_beg+ic) >= 0)
					break;
				else if (CheckSpacelessAt(ispaceless, area_beg+ic) == TRUE)
					break;

				ic++;
			}

			// Process the discovered piece of the line body.
			assert(ic > ib);
			if (frgr_area == NULL)
			{
				// The area is not covered with any decor.
				emt_ctx.m_doc.EmitRawData(body_data_ptr+area_beg+ib, ic-ib, FALSE, FALSE, emt_ctx.m_data_emit_mode);
			}
			else if (frgr_area->m_basic_owner != NULL)
			{
				// The area is covered with decor that has a non NULL owner. Note that when the area
				// is covered by the frgr decor that has a NULL owner, this area is not rendered at all.
				emt_ctx.m_doc.EmitElementBeg(*frgr_area->m_basic_owner);
				emt_ctx.m_doc.EmitRawData(body_data_ptr+area_beg+ib, ic-ib, FALSE, FALSE, emt_ctx.m_data_emit_mode);
				emt_ctx.m_doc.EmitElementEnd(*frgr_area->m_basic_owner);
			}

			// Check for spaceless decors that should be processed before rendering the current symbol.
			while (CheckSpacelessAt(ispaceless, area_beg+ic, THtmlSpacelessDecor::BKGR_COVER_LEFT) == TRUE)
				ProcessSpacelessDecor(emt_ctx, element_proxy, ispaceless++);

			// Shift the variable of the outer loop.
			ib = ic;
		}

		// Close the unclosed bkgr decors if any.
		while (emt_ctx.m_opened_bkgr_decors.NumItems() > 0)
		{
			emt_ctx.m_doc.EmitElementEnd(*emt_ctx.m_opened_bkgr_decors[emt_ctx.m_opened_bkgr_decors.NumItems()-1]->m_basic_owner);
			emt_ctx.m_opened_bkgr_decors.ReleaseLastItem();
		}

		// Process the unprocessed spaceless decors if any.
		while (ispaceless < m_spaceless_decors.NumItems())
			ProcessSpacelessDecor(emt_ctx, element_proxy, ispaceless++);
	}

	// Emit the final closing tag.
	emt_ctx.m_doc.EmitElementEnd(element_proxy);
	return(overall_alloc_success);
}

bool THtmlGenericDecoratedArea::AddSpacelessDecorInternal(THtmlSpacelessDecor &decor_struct)
{
	int insert_before_inx = 0;
	if (m_spaceless_decors.NumItems() > 0 && THtmlSpacelessDecor::CompareHorzPositions(decor_struct, m_spaceless_decors[0]) >= 0)
	{
		// Find the right place to insert the new decor.
		long lo_good = 0;
		long hi_bad = m_spaceless_decors.NumItems();
		while ((hi_bad - lo_good) > 1)
		{
			long inx_test = (hi_bad+lo_good)/2;
			if (THtmlSpacelessDecor::CompareHorzPositions(decor_struct, m_spaceless_decors[inx_test]) >= 0)
				lo_good = inx_test;
			else hi_bad = inx_test;
		}

		// Insert decoration right after the smaller or equal item.
		insert_before_inx = lo_good+1;
	}

	// Insert or append the new record. Method below supports both cases.
	return(m_spaceless_decors.InsertItem(insert_before_inx, decor_struct));
}

void THtmlGenericDecoratedArea::ProcessSpacelessDecor(THtmlObjectEmittingHelper &emt_ctx, TDecoratedAreaProxy &element_proxy, int spaceless_inx)
{
	assert(spaceless_inx >= 0 && spaceless_inx < m_spaceless_decors.NumItems());
	THtmlSpacelessDecor &decor = m_spaceless_decors[spaceless_inx];

	if (decor.IsSpecialTab() == FALSE)
	{
		// Render the inserted decoration. Most likely this will be a bitmap.
		emt_ctx.m_doc.EmitElement(*decor.m_spaceless_owner);
	}
	else if (emt_ctx.m_num_subcolumns >= 2 && emt_ctx.m_inx_subcolumn < emt_ctx.m_curr_line_subcolumns)
	{
		// Process the special tabstop. Close all opened bkgr layers if any.
		for (int il1=emt_ctx.m_opened_bkgr_decors.NumItems()-1; il1 >= 0; --il1)
			emt_ctx.m_doc.EmitElementEnd(*emt_ctx.m_opened_bkgr_decors[il1]->m_basic_owner);

		// Close and open back the main element.
		emt_ctx.m_doc.EmitElementEnd(element_proxy);
		element_proxy.PrepareExtAttrs(emt_ctx, decor.m_spec_tab_new_bkgr, decor.m_spec_tab_clmns);
		emt_ctx.m_doc.EmitElementBeg(element_proxy);

		// Open back all opened bkgr layers.
		for (int il2=0; il2 < emt_ctx.m_opened_bkgr_decors.NumItems(); ++il2)
			emt_ctx.m_doc.EmitElementBeg(*emt_ctx.m_opened_bkgr_decors[il2]->m_basic_owner);
	}
}

//----------------------------------------------------------------------------------
//	=====================  THtmlDecoratedLineInfo  ======================
//----------------------------------------------------------------------------------

void THtmlDecoratedLineInfo::ResetDecorations()
{
	m_nums_css_style = NULL;
	m_line_num_frgr_decor = NULL;

	m_marks_area.ResetContainer();
	m_marks_area.SetCssClass(NULL);
	m_line_decors.ResetDecorations();
}

void THtmlDecoratedLineInfo::SetWholeLineStyles(const wchar_t *nums_css_style, const wchar_t *marks_css_style, const wchar_t *data_css_style)
{
	m_nums_css_style = (wchar_t*)nums_css_style;
	m_marks_area.SetCssClass(marks_css_style);
	m_line_decors.m_initial_css_style = (wchar_t*)data_css_style;
}

bool THtmlDecoratedLineInfo::EmitLine(THtmlObjectEmittingHelper &emt_ctx, long line_number, const wchar_t *body_data_ptr, long body_data_len, long line_area_beg, long line_area_len)
{
	if (body_data_len < 0)
		body_data_len = wcslen(body_data_ptr);
	if (line_area_len < 0)
		line_area_len = body_data_len;

	// The line is always emitted as a non empty object.
	TSimpleHtmlElement row_element(L"tr");
	emt_ctx.m_doc.EmitElementBeg(row_element);

	// Emit the line number cell.
	TAdvancedHtmlElement line_num_cell(L"td", NULL, 0, m_nums_css_style);

	wchar_t line_num_id_buff[80];
	swprintf(line_num_id_buff, 80, L"%sL%ld", ((emt_ctx.m_line_nums_id_prefix != NULL) ? emt_ctx.m_line_nums_id_prefix : L""), line_number);
	line_num_cell.SetElementId(line_num_id_buff);
	emt_ctx.m_doc.EmitElementBeg(line_num_cell);

	if (m_line_num_frgr_decor != NULL)
		emt_ctx.m_doc.EmitElementBeg(*m_line_num_frgr_decor);

	wchar_t line_num_buff[40];
	swprintf(line_num_buff, 40, L"%ld", line_number);
	emt_ctx.m_doc.EmitRawData(line_num_buff, wcslen(line_num_buff));

	if (m_line_num_frgr_decor != NULL)
		emt_ctx.m_doc.EmitElementEnd(*m_line_num_frgr_decor);

	emt_ctx.m_doc.EmitElementEnd(line_num_cell);

	// Emit the marks area cell.
	emt_ctx.m_doc.EmitElement(m_marks_area);

	// Emit the body of the line. This can be either one cell or a sequence of cells.
	bool res = m_line_decors.EmitArea(emt_ctx, body_data_ptr, body_data_len, line_area_beg, line_area_len);

	// Close the object.
	emt_ctx.m_doc.EmitElementEnd(row_element);
	return(res);
}

//----------------------------------------------------------------------------------
//	=======================  THtmlDecoratedFile  ========================
//----------------------------------------------------------------------------------

THtmlDecoratedFile::THtmlDecoratedFile(const wchar_t *css_class, const wchar_t *css_style, THtmlCrLfMode crlf_mode)
	: TRawHtmlElement(L"div")
{
	// Clear fileds that mark owned resources and call the general reset.
	m_lines_info_owned = FALSE;
	m_display_lines = NULL;
	ResetDecoratedFile();

	m_dfl_element_id_div[0] = 0;
	m_dfl_css_class_table = NULL;

	m_dfl_css_class_div = (wchar_t*)css_class;
	m_dfl_css_style_div = (wchar_t*)css_style;
	m_dfl_crlf_mode_div = crlf_mode;
}

void THtmlDecoratedFile::EmitElementBody(THtmlDocument &doc)
{
	if (m_file_data == NULL)
	{
		TSimpleHtmlElement data(L"p", L"The file is missing.");
		doc.EmitElement(data);
	}
	else if (m_num_isect_lines <= 0)
	{
		TSimpleHtmlElement data(L"p", L"An intersection between the file data and the range of visible lines is empty.");
		doc.EmitElement(data);
	}
	else
	{
		// Emit all visible lines if any.
		EmitLinesRange(doc, m_first_isect_line, m_num_isect_lines, TRUE, TRUE);
	}
}

void THtmlDecoratedFile::EmitLinesRange(THtmlDocument &doc, long line_beg, long num_lines, bool want_top_area, bool want_bottom_area)
{
	if (GetIntersectionWithTheVisibleLinesRange(line_beg, num_lines) == FALSE)
	{
		// The inresection is empty. Emit an explanation message.
		wchar_t buff_msg[120];
		swprintf(buff_msg, 120, L"The requested range of lines (%ld ... %ld) does not have intersection with the range of visible lines (%ld ... %ld).",
				line_beg, line_beg+num_lines-1, m_first_isect_line, m_first_isect_line+m_num_isect_lines-1);

		TSimpleHtmlElement data(L"p", buff_msg);
		doc.EmitElement(data);
		return;
	}

	// Open the <table> element.
	TLinesRangeEmtCtx emt_ctx(m_dfl_css_class_table);
	doc.EmitElementBeg(emt_ctx.m_table_element);

	if (want_top_area == TRUE && m_props.m_top_area_height > 0)
		EmitTopBottmArea(doc, m_top_area_decors, m_props.m_top_area_height);

	assert(num_lines >= 0);
	THtmlObjectEmittingHelper hlpr(doc);
	hlpr.m_data_emit_mode = m_props.m_file_data_emit_mode;
	if (m_line_numbers_id_prefix[0] != 0)
		hlpr.m_line_nums_id_prefix = m_line_numbers_id_prefix;

	int ibefore = 0;
	while (num_lines > 0 && ibefore < m_subcolumns_info.NumItems())
	{
		// The requested lines range is not empty and an unprocessed spec tabs area is still existing.
		long line_area_start = m_subcolumns_info[ibefore].m_line_start;
		long line_area_after = line_area_start+m_subcolumns_info[ibefore].m_num_lines;

		if (line_area_start <= line_beg)
		{
			if (line_area_after <= line_beg)
			{
				// Current spec tabs area has not intercestion with the requested range.
				ibefore++;
			}
			else if (line_area_after < line_beg+num_lines)
			{
				// Only part of the requested range is covered with the current spec tabs area.
				long len_fragm = line_area_after-line_beg;
				EmitLinesRangeLow(hlpr, emt_ctx, ibefore, line_beg, len_fragm);
				line_beg += len_fragm;
				num_lines -= len_fragm;
				ibefore++;
			}
			else
			{
				// The whole requested range is covered with the current spec tabs area.
				EmitLinesRangeLow(hlpr, emt_ctx, ibefore, line_beg, num_lines);
				num_lines = 0;
			}
		}
		else
		{
			if (line_area_start >= line_beg+num_lines)
			{
				// Current spec tabs area starts after the requested range of lines.
				break;
			}
			else
			{
				// Process part of the requested range that is not covered with the spec tabs area.
				long len_fragm = line_area_start-line_beg;
				EmitLinesRangeLow(hlpr, emt_ctx, -1, line_beg, len_fragm);
				line_beg += len_fragm;
				num_lines -= len_fragm;
			}
		}
	}

	// Process the rest of the requested lines that are not covered by any area of the spec tabs.
	if (num_lines > 0)
		EmitLinesRangeLow(hlpr, emt_ctx, -1, line_beg, num_lines);

	if (want_bottom_area == TRUE && m_props.m_bottom_area_height > 0)
		EmitTopBottmArea(doc, m_bottom_area_decors, m_props.m_bottom_area_height);

	// Finally close the table.
	doc.EmitElementEnd(emt_ctx.m_table_element);
}

void THtmlDecoratedFile::EmitSpecialInsert(THtmlDocument &doc, long line_num_after, const wchar_t *msg_css_class, const wchar_t *msg_body)
{
	// Open the <table> and the <tr> elements.
	TLinesRangeEmtCtx emt_ctx(m_dfl_css_class_table);
	TSimpleHtmlElement spec_row(L"tr");
	doc.EmitElementBeg(emt_ctx.m_table_element);
	doc.EmitElementBeg(spec_row);

	int inx_dlines = line_num_after-m_display_lines_first_line;
	if (inx_dlines >= 0 && inx_dlines < m_display_lines_len)
	{
		// Passed line number belongs to the range of visible lines.
		THtmlDecoratedLineInfo &li = m_display_lines[inx_dlines];

		// Pick up the bkgr styles from this line.
		THtmlCrLfMode crlf_mode = crlm_none;
		const wchar_t *nums_css_style = li.m_nums_css_style;
		const wchar_t *marks_css_style = li.m_marks_area.GetClassAttr(crlf_mode);
		const wchar_t *body_css_style = li.LineDecors().m_initial_css_style;

		// The style of the line body is combination of the expected bkgr style and the passed frgr style.
		wchar_t body_css_style_buff[160];
		swprintf(body_css_style_buff, 160, L"%s %s", (body_css_style != NULL) ? body_css_style : m_props.m_def_line_data_css_style,
				(msg_css_class != NULL) ? msg_css_class : L"");

		// Allocate and emit the line elements.
		TSimpleHtmlElement cell1(L"td", NULL, 0, ((nums_css_style != NULL) ? nums_css_style : m_props.m_def_nums_css_style));
		TSimpleHtmlElement cell2(L"td", NULL, 0, ((marks_css_style != NULL) ? marks_css_style : m_props.m_def_marks_css_style));
		TSimpleHtmlElement cell3(L"td", msg_body, -1, body_css_style_buff);
		doc.EmitElements(cell1, cell2, cell3);
	}

	// Close the row and the table elements.
	doc.EmitElementEnd(spec_row);
	doc.EmitElementEnd(emt_ctx.m_table_element);
}

void THtmlDecoratedFile::ResetDecoratedFile()
{
	m_props.Clear();
	m_line_numbers_id_prefix[0] = 0;

	m_file_data = NULL;
	m_file_data_len = 0;
	m_file_data_src_offs = 0;
	m_file_data_first_line = 1;

	if (m_lines_info_owned != FALSE)
	{
		assert(m_lines_info != NULL);
		free(m_lines_info);
		m_lines_info_owned = FALSE;
	}

	m_lines_info = NULL;
	m_lines_info_len = 0;

	if (m_display_lines != NULL)
	{
		delete[] m_display_lines;
		m_display_lines = NULL;
	}

	m_display_lines_len = 0;
	m_display_lines_first_line = 0;

	m_first_isect_line = 0;
	m_num_isect_lines = 0;

	m_subcolumns_info.Clear();
	memset(m_top_area_decors, 0, sizeof(m_top_area_decors));
	memset(m_bottom_area_decors, 0, sizeof(m_bottom_area_decors));

	m_dfl_element_id_div[0] = 0;
	m_dfl_css_class_table = NULL;
}

void THtmlDecoratedFile::SetupElementId(const wchar_t *element_id)
{
	if (element_id != NULL && element_id[0] != 0)
	{
		// Accept the non empty element id.
		wcsncpy(m_dfl_element_id_div, element_id, 40);
		m_dfl_element_id_div[39] = 0;
	}
	else
	{
		// The id of the element is missing or it will be set up later.
		m_dfl_element_id_div[0] = 0;
	}
}

bool THtmlDecoratedFile::Setup(THtmlDecoratedFileProps &props, const wchar_t *line_numbers_id_prefix, const wchar_t *fdata, long fdata_len_in_wchars,
							long fdata_src_offs, long fdata_first_line, TSimpleLineInfo *linfo, long linfo_len, long first_visib_line, long num_visib_lines)
{
	// Bring object to its initial state.
	ResetDecoratedFile();

	if (fdata == NULL)
	{
		// This is the case of a missing file. All other parameters are not important.
		return(TRUE);
	}

	// Accept the props without any verification.
	m_props = props;

	if (line_numbers_id_prefix != NULL && line_numbers_id_prefix[0] != 0)
	{
		// Accept the non empty prefix.
		wcsncpy(m_line_numbers_id_prefix, line_numbers_id_prefix, 40);
		m_line_numbers_id_prefix[39] = 0;
	}
	else
	{
		m_line_numbers_id_prefix[0] = 0;
	}


	// The file data is not missing. Although its length can be zero.
	if (fdata_len_in_wchars < 0)
	{
		// The length of the file data is not given explicitly.
		fdata_len_in_wchars = (long)wcslen(fdata);
	}

	// Create the lines info if needed.
	bool local_linfo = FALSE;
	if (linfo == NULL)
	{
		// A non empty file was passed without the lines info.
		assert(linfo_len == 0);
		long longest_line_len = 0;
		linfo = TSimpleLineInfo::BuildLinesInfoFromData(fdata, fdata_len_in_wchars, fdata_src_offs, linfo_len, longest_line_len);
		if (linfo == NULL)
			return(FALSE);

		local_linfo = TRUE;
	}

	// Ensure that the lines info is not empty.
	assert(linfo_len > 0);

	// Verify the range of lines to display.
	if (num_visib_lines <= 0)
	{
		// The number of lines to display is not passed directly. Find out an appropriate number of lines.
		if (first_visib_line <= fdata_first_line+linfo_len-1)
		{
			// At least the last line of the file data will be visible. Maybe there will be line overflow lines
			// before the lines with the file data.
			num_visib_lines = fdata_first_line+linfo_len-first_visib_line;
		}
		else
		{
			// The intersection is empty. Show one dummy line.
			num_visib_lines = 1;
		}
	}

	// Important step. Allocate an array of the line infos.
	assert(num_visib_lines > 0);
	THtmlDecoratedLineInfo *lines_array = new THtmlDecoratedLineInfo[num_visib_lines];
	if (lines_array == NULL)
	{
		if (local_linfo == TRUE)
			free(linfo);

		// The whole setup failed.
		return(FALSE);
	}

	//
	// Accept various data fields.
	//

	// File data.
	m_file_data = (wchar_t*)fdata;
	m_file_data_len = fdata_len_in_wchars;
	m_file_data_src_offs = fdata_src_offs;
	m_file_data_first_line = fdata_first_line;

	// Lines info.
	m_lines_info = linfo;
	m_lines_info_len = linfo_len;
	m_lines_info_owned = local_linfo;

	// The range of lines to display.
	m_display_lines = lines_array;
	m_display_lines_len = num_visib_lines;
	m_display_lines_first_line = first_visib_line;

	// Determine the meaningful intersection between the file data and the range of lines to display.
	long max_beg = __max(fdata_first_line, first_visib_line);
	long min_end = __min(fdata_first_line+linfo_len, first_visib_line+num_visib_lines);
	if (max_beg < min_end)
	{
		// The intersection is non empty.
		m_first_isect_line = max_beg;
		m_num_isect_lines = min_end-max_beg;
	}

	// Success.
	return(TRUE);
}

bool THtmlDecoratedFile::SetBkgrDecoration(TRawHtmlElement *owner, short zorder, long area_beg, long area_len)
{
	bool full_success = TRUE;

	// Iterate the lines that intersect with the passed file area.
	TFileSpaceIterationInfo iter_info(area_beg, area_len, m_lines_info, m_lines_info_len);
	while (THtmlDecoratedLineInfo *li = IterateVisibleFileSpace(iter_info))
	{
		if (iter_info.num_chars == 0)
		{
			// The bkgr decoration is not applicable to the line borders or to empty lines.
			continue;
		}

		full_success = li->LineDecors().SetBkgrDecor(owner, zorder, iter_info.char_beg, iter_info.num_chars);
		if (full_success == FALSE)
			break;
	}

	return(full_success);
}

bool THtmlDecoratedFile::SetFrgrDecoration(TRawHtmlElement *owner, short zorder, long area_beg, long area_len)
{
	bool full_success = TRUE;

	// Iterate the lines that intersect with the passed file area.
	TFileSpaceIterationInfo iter_info(area_beg, area_len, m_lines_info, m_lines_info_len);
	while (THtmlDecoratedLineInfo *li = IterateVisibleFileSpace(iter_info))
	{
		if (iter_info.num_chars == 0)
		{
			// The frgr decoration is not applicable to the line borders or to empty lines.
			continue;
		}

		full_success = li->LineDecors().SetFrgrDecor(owner, zorder, iter_info.char_beg, iter_info.num_chars);
		if (full_success == FALSE)
			break;
	}

	return(full_success);
}

bool THtmlDecoratedFile::SetSpacelessDecoration(TRawHtmlElement *owner, short horz_order, long area_offs)
{
	TFileSpaceIterationInfo iter_info(area_offs, 0, m_lines_info, m_lines_info_len);
	THtmlDecoratedLineInfo *li = IterateVisibleFileSpace(iter_info);

	bool full_success = TRUE;
	if (li != NULL)
		full_success = li->LineDecors().SetSpacelessDecor(owner, horz_order, iter_info.char_beg);

	return(full_success);
}

bool THtmlDecoratedFile::SetSpecialTabDecoration(short horz_order, long area_offs)
{
	TFileSpaceIterationInfo iter_info(area_offs, 0, m_lines_info, m_lines_info_len);
	THtmlDecoratedLineInfo *li = IterateVisibleFileSpace(iter_info);

	bool full_success = TRUE;
	if (li != NULL)
		full_success = li->LineDecors().SetSpecialTab(horz_order, iter_info.char_beg);

	return(full_success);
}

bool THtmlDecoratedFile::SetWholeLineBkgrDecoration(long line_beg, long num_lines, const wchar_t *nums_css_style, const wchar_t *marks_css_style, const wchar_t *line_data_css_style)
{
	while (THtmlDecoratedLineInfo *li = IterateVisibleLinesSpace(line_beg, num_lines))
	{
		li->SetWholeLineStyles(nums_css_style, marks_css_style, line_data_css_style);
	}

	// This method always succeeds.
	return(TRUE);
}

bool THtmlDecoratedFile::SetLineNumberFrgrDecoration(long line_beg, long num_lines, TRawHtmlElement *owner)
{
	while (THtmlDecoratedLineInfo *li = IterateVisibleLinesSpace(line_beg, num_lines))
	{
		li->SetLineNumberStyle(owner);
	}

	// This method always succeeds.
	return(TRUE);
}

bool THtmlDecoratedFile::SetSpecTabsAreaDecoration(long line_beg, long num_lines)
{
	// Get intersection between the passed lines range and the visible lines.
	if (GetIntersectionWithTheVisibleLinesRange(line_beg, num_lines) == FALSE)
		return(TRUE);

	// The intersection is not empty. Skip all records that stay before the new area.
	int ibefore = 0;
	while (ibefore < m_subcolumns_info.NumItems() && m_subcolumns_info[ibefore].m_line_start+m_subcolumns_info[ibefore].m_num_lines <= line_beg)
		ibefore++;

	if (ibefore < m_subcolumns_info.NumItems() && m_subcolumns_info[ibefore].m_line_start < line_beg)
	{
		// Adjust the length of the current record and skip it. Note that it might happen that this area is longer
		// than the new area that is being inserted. Nevertheless use a simple approach.
		m_subcolumns_info[ibefore].m_num_lines = line_beg-m_subcolumns_info[ibefore].m_line_start;
		ibefore++;
	}

	while (ibefore < m_subcolumns_info.NumItems() && m_subcolumns_info[ibefore].m_line_start+m_subcolumns_info[ibefore].m_num_lines <= line_beg+num_lines)
	{
		// Current record is competely covered with the new area of tabs. Cancel this record.
		m_subcolumns_info.ReleaseItem(ibefore);
	}

	if (ibefore < m_subcolumns_info.NumItems() && m_subcolumns_info[ibefore].m_line_start < line_beg+num_lines)
	{
		// Adjust the beginning of this record.
		m_subcolumns_info[ibefore].m_num_lines -= line_beg+num_lines-m_subcolumns_info[ibefore].m_line_start;
		m_subcolumns_info[ibefore].m_line_start = line_beg+num_lines;
	}

	// Add a new record to the array.
	TSpecTabsAreaInfo info = { line_beg, num_lines };
	return(m_subcolumns_info.InsertItem(ibefore, info));
}

bool THtmlDecoratedFile::SetMarksAreaDecoration(long line_number, short horz_order, TRawHtmlElement *owner)
{
	long num_lines = 1;
	THtmlDecoratedLineInfo *li = IterateVisibleLinesSpace(line_number, num_lines);

	bool full_success = TRUE;
	if (li != NULL)
		full_success = li->AppendMark(owner);

	return(full_success);
}

bool THtmlDecoratedFile::SetTopBottomAreaDecoration(bool cover_top, bool cover_bottom, const wchar_t *nums_css_style, const wchar_t *marks_css_style, const wchar_t *line_data_css_style)
{
	if (cover_top == TRUE)
	{
		m_top_area_decors[0] = (wchar_t*)nums_css_style;
		m_top_area_decors[1] = (wchar_t*)marks_css_style;
		m_top_area_decors[2] = (wchar_t*)line_data_css_style;
	}

	if (cover_bottom == TRUE)
	{
		m_bottom_area_decors[0] = (wchar_t*)nums_css_style;
		m_bottom_area_decors[1] = (wchar_t*)marks_css_style;
		m_bottom_area_decors[2] = (wchar_t*)line_data_css_style;
	}

	// This method always succeeds.
	return(TRUE);
}

bool THtmlDecoratedFile::GetIntersectionWithVisibleFileArea(long &area_beg, long &area_len, bool allow_zero_length_area)
{
	if (m_num_isect_lines <= 0 || area_len < 0 || (allow_zero_length_area == FALSE && area_len == 0))
		return(FALSE);

	long visible_beg = m_lines_info[m_first_isect_line-m_file_data_first_line].LineBeg();
	long visible_end = m_lines_info[m_first_isect_line+m_num_isect_lines-1-m_file_data_first_line].LineEnd();

	if (area_beg < visible_beg)
	{
		// Passed area starts before the current visible area.
		area_len -= (visible_beg-area_beg);
		if (area_len < 0 || (allow_zero_length_area == FALSE && area_len == 0))
			return(FALSE);

		area_beg = visible_beg;
	}

	if (area_beg > visible_end || (allow_zero_length_area == FALSE && area_beg == visible_end))
	{
		// Passed area starts after the current visible area.
		return(FALSE);
	}

	if (area_beg+area_len > visible_end)
	{
		area_len = visible_end-area_beg;
	}

	// The intersection is not empty or the caller allowed a zero length area.
	assert(area_len > 0 || (allow_zero_length_area == TRUE && area_len == 0));
	return(TRUE);
}

bool THtmlDecoratedFile::GetIntersectionWithTheVisibleLinesRange(long &line_beg, long &num_lines)
{
	if (m_num_isect_lines <= 0 || num_lines <= 0)
		return(FALSE);

	if (line_beg < m_first_isect_line)
	{
		// Adjust the beg of the requested range forward.
		num_lines -= m_first_isect_line-line_beg;
		if (num_lines <= 0)
			return(FALSE);

		line_beg = m_first_isect_line;
	}

	if (line_beg >= m_first_isect_line+m_num_isect_lines)
		return(FALSE);

	if (line_beg+num_lines > m_first_isect_line+m_num_isect_lines)
	{
		// Adjust the length of the requested range.
		num_lines = m_first_isect_line+m_num_isect_lines-line_beg;
	}

	// A non empty intersection is present.
	return(TRUE);
}

long THtmlDecoratedFile::GetLineNumber(long src_offset)
{
	TSimpleLineInfo *li = TSimpleLineInfo::FindLineInfo(m_lines_info, m_lines_info_len, src_offset-m_file_data_src_offs);
	if (li == NULL)
		return(-1);

	// Passed offset belongs to one of the lines of the file data.
	return((li-m_lines_info)+m_file_data_first_line);
}

TSimpleLineInfo *THtmlDecoratedFile::GetLineInfo(long line_number)
{
	long inx = line_number-m_file_data_first_line;
	return((inx >= 0 && inx < m_lines_info_len) ? m_lines_info+inx : NULL);
}

THtmlDecoratedLineInfo *THtmlDecoratedFile::IterateVisibleFileSpace(TFileSpaceIterationInfo &info)
{
	// Check that visble intersection exists and get intersection of the area with the file data.
	if (m_file_data == NULL || m_num_isect_lines <= 0 || info.StepIteration() == FALSE)
		return(NULL);

	if (info.line_inx+m_file_data_first_line < m_first_isect_line)
	{
		// Area begins in the invisible part of the file.
		if (info.line_inx+m_file_data_first_line < m_first_isect_line-1)
		{
			// Skip several lines in one step.
			long line_inx_to = m_first_isect_line-m_file_data_first_line-1;
			long new_beg = m_lines_info[line_inx_to].line_offs;
			info.area_len -= new_beg-info.area_beg;
			if (info.area_len < 0)
				return(NULL);

			info.line_inx = line_inx_to;
			info.area_beg = new_beg;
		}

		// File data space iterator stands before the first visible line. Step onto this line and get intersection with it.
		if (info.StepIteration() == FALSE)
			return(NULL);
	}

	if (info.line_inx+m_file_data_first_line >= m_first_isect_line+m_num_isect_lines)
	{
		// All or part of the area stays after the visible part of the file.
		info.area_len = -1;
		return(NULL);
	}

	// Area has intersection with the visible part of the file.
	return(m_display_lines+(info.line_inx+m_file_data_first_line-m_display_lines_first_line));
}

THtmlDecoratedLineInfo *THtmlDecoratedFile::IterateVisibleLinesSpace(long &line_beg, long &num_lines)
{
	if (m_num_isect_lines <= 0 || num_lines <= 0)
		return(NULL);

	if (line_beg < m_first_isect_line)
	{
		// Adjust the line beg forward.
		num_lines -= m_first_isect_line-line_beg;
		line_beg = m_first_isect_line;
		if (num_lines <= 0)
			return(NULL);
	}

	if (line_beg >= m_first_isect_line+m_num_isect_lines)
	{
		num_lines = -1;
		return(NULL);
	}

	// A non empty intersection is present. Pick up the index before updating the parameters.
	long inx = line_beg-m_display_lines_first_line;
	line_beg++;
	num_lines--;
	return(m_display_lines+inx);
}

THtmlDecoratedLineInfo *THtmlDecoratedFile::GetDcrtLineInfo(long line_number)
{
	long inx = line_number-m_display_lines_first_line;
	return((inx >= 0 && inx < m_display_lines_len) ? m_display_lines+inx : NULL);
}

void THtmlDecoratedFile::EmitTopBottmArea(THtmlDocument &doc, wchar_t **area_decors, int area_height)
{
	wchar_t buff_height[40];
	swprintf(buff_height, 40, L"height:%dpx", area_height);
	TAdvancedHtmlElement ext_row(L"tr");
	ext_row.SetCssStyle(buff_height);

	TSimpleHtmlElement cell1(L"td", NULL, 0, ((area_decors[0] != NULL) ? area_decors[0] : m_props.m_def_nums_css_style));
	TSimpleHtmlElement cell2(L"td", NULL, 0, ((area_decors[1] != NULL) ? area_decors[1] : m_props.m_def_marks_css_style));
	TSimpleHtmlElement cell3(L"td", NULL, 0, ((area_decors[2] != NULL) ? area_decors[2] : m_props.m_def_line_data_css_style));

	doc.EmitElementBeg(ext_row);
	doc.EmitElements(cell1, cell2, cell3);
	doc.EmitElementEnd(ext_row);
}

void THtmlDecoratedFile::EmitLinesRangeLow(THtmlObjectEmittingHelper &hlpr, TLinesRangeEmtCtx &emt_ctx, int spec_tabs_area_inx, long line_beg, long num_lines)
{
	assert(m_file_data != NULL && num_lines > 0);

	// Setup the number of subcolumns.
	hlpr.m_num_subcolumns = 0;
	if (spec_tabs_area_inx >= 0)
	{
		// Find out the biggest number of subcolumns in the passed range of lines.
		for (long iline1=0; iline1<num_lines; ++iline1)
		{
			int inx_dlines = line_beg+iline1-m_display_lines_first_line;
			assert(inx_dlines >= 0 && inx_dlines < m_display_lines_len);
			THtmlDecoratedLineInfo &li = m_display_lines[inx_dlines];

			int curr_line_clmns = li.LineDecors().NumSubColumns();
			if (curr_line_clmns > hlpr.m_num_subcolumns)
				hlpr.m_num_subcolumns = curr_line_clmns;
		}
	}

	if (emt_ctx.m_first_range == FALSE)
	{
		// It is necessary to reopen the big element.
		hlpr.m_doc.EmitElementEnd(emt_ctx.m_table_element);
		hlpr.m_doc.EmitElementBeg(emt_ctx.m_table_element);
	}

	// Dump the requested range of lines one by one.
	for (long iline2=0; iline2<num_lines; ++iline2)
	{
		long line_number = line_beg+iline2;

		int inx_dlines = line_number-m_display_lines_first_line;
		assert(inx_dlines >= 0 && inx_dlines < m_display_lines_len);
		THtmlDecoratedLineInfo &li = m_display_lines[inx_dlines];

		// Setup the whole area styles. Note that they will not be reverted back.
		THtmlCrLfMode crlf_mode = crlm_none;
		if (li.m_nums_css_style == NULL)
			li.m_nums_css_style = m_props.m_def_nums_css_style;
		if (li.m_marks_area.GetClassAttr(crlf_mode) == NULL)
			li.m_marks_area.SetCssClass(m_props.m_def_marks_css_style);
		if (li.LineDecors().m_initial_css_style == NULL)
			li.LineDecors().m_initial_css_style = m_props.m_def_line_data_css_style;

		TSimpleLineInfo &line_info = m_lines_info[line_number-m_file_data_first_line];
		li.EmitLine(hlpr, line_number, m_file_data+line_info.line_offs-m_file_data_src_offs, line_info.line_len);
	}

	// The lines were emitted.
	emt_ctx.m_first_range = FALSE;
}


