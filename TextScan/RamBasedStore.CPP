//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "TextScan/RamBasedStore.H"

//-------------------------------------------------------------------------
//  =================  TMidLevRamParsingLog  =====================
//-------------------------------------------------------------------------

TMidLevRamBasedParsingLog::TMidLevRamBasedParsingLog(TStringsDict *strs_dict)
{
	// The ctor is creating the log and immediately puts it into the active state.
	m_session_header.Clear();
	m_session_header.parsing_status = -1;		// This is indicator that writing to the log is not finished.

	m_cnt_errors = 0;
	m_pStrDict = strs_dict;
}

bool TMidLevRamBasedParsingLog::SetupHeader(PrInitParsingInfo1 &data, ID pars_id)
{
	assert(data.parsing_date != 0);
	assert(data.base_dir != NULL);

	// Accept ids.
	m_session_header.batch_id = data.batch_id;
	m_session_header.parsing_id = pars_id;

	// Copy in the parsing date.
	m_session_header.parsing_date = data.parsing_date;

	// Duplicate the directory name.
	m_session_header.base_dir = StrDupe(data.base_dir);
	if (m_session_header.base_dir == NULL)
		return(FALSE);

	// Copy the flags.
	m_session_header.allow_intermediate_paths = data.allow_intermediate_paths;
	m_session_header.allow_unsubsted_paths = data.allow_unsubsted_paths;
	return(TRUE);
}

bool TMidLevRamBasedParsingLog::SetupPrepHeaderInfo(PrInitParsingInfo2 &data)
{
	assert(data.prep_phase_opts != NULL);

	// Copy information about the command line.
	m_session_header.cmd_line_origin = data.cmdl_origin;
	if (data.cmdl_file != NULL)
	{
		m_session_header.cmd_line_doc_id = data.cmdl_file->data.doc_id;
		m_session_header.cmd_line_file_name = StrDupe(data.cmdl_file->data.info.file_name);

		if (m_session_header.cmd_line_file_name == NULL)
			return(FALSE);
	}

	// Copy info about the major src file.
	m_session_header.major_src_pos_param = data.mj_src_pos_prm;

	// Copy the options.
	m_session_header.prep_opts = *data.prep_phase_opts;
	return(TRUE);
}

bool TMidLevRamBasedParsingLog::SetupMainHeaderInfo(PrInitParsingInfo3 &data)
{
	assert(data.major_src_file != NULL);
	assert(data.options != NULL);

	// Fill in the session type.
	m_session_header.lang_type = data.lang_type;

	// Copy information about the major source file.
	m_session_header.major_src_doc_id.doc_id = data.major_src_file->data.doc_id;
	m_session_header.major_src_file_name = StrDupe(data.major_src_file->data.info.file_name);
	if (m_session_header.major_src_file_name == NULL)
		return(FALSE);

	if (data.major_src_file->data.pars_id != 0)
	{
		// The major src file is a parsing session dependent file. Fill in the session id of the current
		// parsing session into its file descriptor. The file should be already registered.
		ID fid = data.major_src_file->data.file_id;
		assert(data.major_src_file->data.pars_id == m_session_header.parsing_id && fid < NumEventPtrs());
		assert(m_event_ptrs[fid] != NULL && m_event_ptrs[fid]->hdr.event_type == pset_register_temp_file);
		TFileInfo &mj_src_info = m_event_ptrs[fid]->temp_file.temp_file_info;
		mj_src_info.major_src_latest_pars_id = m_session_header.parsing_id;
	}

	if (data.obj_proto_file != NULL)
	{
		// A non empty obj file name was passed.
		m_session_header.obj_file_doc_id = data.obj_proto_file->data.doc_id;
		m_session_header.obj_file_name = StrDupe(data.obj_proto_file->data.info.file_name);
		if (m_session_header.obj_file_name == NULL)
			return(FALSE);
	}

	// Copy in the options.
	m_session_header.scan_opts = *data.options;
	return(TRUE);
}

void TMidLevRamBasedParsingLog::FinalizeHeaderInfo(TMidLevPrepOptions *prep_options, TMidLevScannerOptions *scan_options, TMidLevParsingSummary &data, int final_pars_status)
{
	assert(final_pars_status >= 0);

	m_session_header.prep_opts = *prep_options;
	m_session_header.scan_opts = *scan_options;
	m_session_header.summary = data;
	m_session_header.parsing_status = final_pars_status;
}

void TMidLevRamBasedParsingLog::FinalizeHeaderInfoRawSess(TRawIndexingSessSummary &sumry, TFileInfoItem *mj_src_file)
{
	m_session_header.raw_summary = sumry;

	if (mj_src_file != NULL)
	{
		m_session_header.major_src_doc_id.doc_id = mj_src_file->data.doc_id;
		m_session_header.major_src_file_name = StrDupe(mj_src_file->data.info.file_name);
	}

	// For now the raw session cannot be aborted.
	m_session_header.parsing_status = 0;
}

bool TMidLevRamBasedParsingLog::AddEvent(CtvParsingEventInfo *pev, long event_length)
{
	// Store the body of the event.
	CtvParsingEventInfo *pspace = (CtvParsingEventInfo*)m_buffers.AddDwordAlignedData(pev, event_length);
	if (pspace == NULL)
	{
		return(FALSE);
	}

	// Check, if the array of event pointers is long enough or not.
	ID evid = pev->hdr.event_id;
	assert(evid != 0);

	if (evid >= NumEventPtrs())
	{
		// The main array of event pointers should be extended.
		if (m_event_ptrs.ReserveTotalBufferSpace(evid+1) == FALSE)
		{
			m_cnt_errors++;
			return(FALSE);
		}

		// Fill all new array slots with NULLs.
		int num_cells = m_event_ptrs.NumAllocedItems();
		for (int inx=NumEventPtrs(); inx<num_cells; ++inx)
			m_event_ptrs[inx] = NULL;

		// Include reserved slots into the pseudo count of events.
		m_event_ptrs.SetNumItems(num_cells);
	}

	// Verify that event with the passed event id is not registered yet.
	if (m_event_ptrs[evid] != NULL)
	{
		assert(FALSE);
		return(FALSE);
	}

	// Save the event pointer.
	m_event_ptrs[evid] = pspace;

	// Carry out the event specific actions.
	if (pspace->hdr.event_type == pset_lex_source_enter || pspace->hdr.event_type == pset_lex_source_exit)
	{
		// Add this event into the LexSrc index. Verify that event ids in the index steadily increase.
		if (m_lex_src_index.NumItems() > 0)
		{
			assert(m_lex_src_index[m_lex_src_index.NumItems()-1]->hdr.event_id < evid);
		}

		if (m_lex_src_index.AppendItem(pspace) == FALSE)
		{
			// Updating the index failed. Cancel the primary event registration.
			m_event_ptrs[evid] = NULL;
			m_cnt_errors++;
			return(FALSE);
		}
	}
	else if (pspace->hdr.event_type == pset_cond_parsing_change)
	{
		// Add this event into the CondParsingChanges index. Verify that event ids in the index steadily increase.
		if (m_cond_pars_index.NumItems() > 0)
		{
			assert(m_cond_pars_index[m_cond_pars_index.NumItems()-1]->hdr.event_id < evid);
		}

		if (m_cond_pars_index.AppendItem(pspace) == FALSE)
		{
			// Updating the index failed. Cancel the primary event registration.
			m_event_ptrs[evid] = NULL;
			m_cnt_errors++;
			return(FALSE);
		}
	}

	// Update the data fields that describe relation between the linear space and the event ids.
	int inx_beg = LexSrcIndexIndexFromLinearOffs(pev->hdr.area.AreaBeg());
	if (inx_beg >= 0)
	{
		// Linear area of this event starts in the context of some lex src enclosure.
		int inx_end = inx_beg;
		if (pev->hdr.area.linear_len > 0)
			inx_end = LexSrcIndexIndexFromLinearOffs(pev->hdr.area.AreaEnd());
	}

	// The event is added. This is a full success.
	return(TRUE);
}

bool TMidLevRamBasedParsingLog::SetTempFileRawIndexingData(ID raw_sess_id, PrRawIndexingFileInfo &hdr_info, BYTE *inx_data, long inx_data_len)
{
	// Ensure that passed file belongs to the current parsing session and that it is existing.
	ID fid = hdr_info.doc_id.file_id;
	assert(hdr_info.doc_id.pars_id == m_session_header.parsing_id);
	assert(fid < NumEventPtrs());

	TMidLevRamLogTempFileRecord *evt = (TMidLevRamLogTempFileRecord*)m_event_ptrs[fid];
	assert(evt != NULL && evt->hdr.event_type == pset_register_temp_file);

	// Event in current log is fine. Copy in the raw indexing session header.
	evt->temp_file_info.raw_sess_id = raw_sess_id;
	evt->temp_file_info.raw_indexing_date = hdr_info.parsing_date;

	if (inx_data_len > 0)
	{
		// Copy results of the indexing to the log.
		wchar_t *wcdata = StrDupe((wchar_t*)inx_data, (inx_data_len+1)/sizeof(wchar_t));
		if (wcdata == NULL)
			return(FALSE);

		// Save the data copy pointer.
		assert(evt->indexing_data == NULL);
		evt->indexing_data = (BYTE*)wcdata;
	}
	return(TRUE);
}

wchar_t *TMidLevRamBasedParsingLog::StrDupe(const wchar_t *str, long len)
{
	if (len < 0)
		len = (long)wcslen(str)+1;

	return((wchar_t*)m_buffers.AddWordAlignedData((void*)str, len*sizeof(wchar_t)));
}

wchar_t *TMidLevRamBasedParsingLog::RegisterStr(const wchar_t *str)
{
	if (str == NULL)
		return(NULL);

	wchar_t *res = m_pStrDict->RegisterStr(str);
	if (res == NULL)
		m_cnt_errors++;

	return(res);
}

wchar_t *TMidLevRamBasedParsingLog::RegisterStr(TStringPtr &str_ptr)
{
	wchar_t *res = m_pStrDict->RegisterStr(str_ptr.GetBodyPtr(), str_ptr.GetLength());
	if (res == NULL)
		m_cnt_errors++;

	return(res);
}

wchar_t *TMidLevRamBasedParsingLog::RegisterStr(TStrPtrInfo &str_ptr_info)
{
	wchar_t *res = m_pStrDict->RegisterStr(str_ptr_info.m_body, str_ptr_info.m_len);
	if (res == NULL)
		m_cnt_errors++;

	return(res);
}

ID TMidLevRamBasedParsingLog::GetSourceRefFromLinearOffs(long lin_offs)
{
	assert(lin_offs > 0);
	int inx = LexSrcIndexIndexFromLinearOffs(lin_offs);
	if (inx < 0)
	{
		// Passsed offset stays before the first lex src call if any.
		return(0);
	}

	if (m_lex_src_index[inx]->hdr.event_type == pset_lex_source_enter)
	{
		// Return the id of this statement.
		return(m_lex_src_index[inx]->hdr.event_id);
	}
	else
	{
		// Return the id of the parent parent (double parent) of this stmt.
		int parent_evid = m_lex_src_index[inx]->src_exit.source_ref;
		return((parent_evid == 0) ? 0 : m_event_ptrs[parent_evid]->src_enter.source_ref);
	}
}

ID TMidLevRamBasedParsingLog::GetCondRefFromLinearOffs(long lin_offs)
{
	assert(lin_offs > 0);
	int inx = CondParsIndexIndexFromLinearOffs(lin_offs);
	if (inx < 0)
	{
		// Passsed offset stay before the first cond parsing structure if any.
		return(0);
	}

	CtvCondParsingChangeEvent &cond_pars_stmt = m_cond_pars_index[inx]->cond_pars;
	if (cond_pars_stmt.stmt_type < mpr_endif)
	{
		// This is regular statement that has its own cond parsing area. Check if the passed offset
		// belongs to the statement itself or it belongs to the area after the statement.
		return((lin_offs <= cond_pars_stmt.hdr.area.AreaEnd()) ? cond_pars_stmt.cond_ref : cond_pars_stmt.hdr.event_id);
	}
	else
	{
		// This is the closing statement. There is no difference between the area after the statement
		// and the area of the stmt itself. Return an id of the parent cond parsing area.
		return(cond_pars_stmt.cond_ref);
	}
}

bool TMidLevRamBasedParsingLog::GetSrcLocInfo(TMidLevRamStoreReader *db, TSourceLocation &buffer, TLinearArea &lin_area, long max_linear_offs)
{
	assert(m_session_header.lang_type != lang_none);
	assert(lin_area.linear_beg >= 0 && lin_area.linear_len >= 0);
	assert(max_linear_offs >= 0 && lin_area.AreaEnd() <= max_linear_offs);

	int inx = LexSrcIndexIndexFromLinearOffs(lin_area.linear_beg);
	if (inx < 0)
	{
		// Passed linear area does not have any source location.
		return(FALSE);
	}

	// Assign variables that depend on the type of the record that was found in the index.
	CtvLexSourceEnterEvent *lex_src_enter;
	long lin_start, src_start;
	ID calls_search_start;

	if (m_lex_src_index[inx]->hdr.event_type == pset_lex_source_enter)
	{
		// Beginning of the passed linear area stays in the beginning of the lex src enclosure before any nested call.
		lex_src_enter = &(m_lex_src_index[inx]->src_enter);
		lin_start = lex_src_enter->LinearPosAfterEntering();
		src_start = lex_src_enter->GetEnclosureSrcStart();
		calls_search_start = lex_src_enter->hdr.event_id+1;
	}
	else
	{
		// Beginning of the passed linear area stays after the exit of some nested call statement. Pick up the id of
		// the call statement that corresponds to this exit event.
		ID latest_call_evid = m_lex_src_index[inx]->src_exit.source_ref;
		assert(latest_call_evid > 0 && latest_call_evid < NumEventPtrs() && m_event_ptrs[latest_call_evid] != NULL);
		CtvLexSourceEnterEvent &latest_call_event = m_event_ptrs[latest_call_evid]->src_enter;
		ID latest_call_src_ref = latest_call_event.source_ref;

		if (latest_call_src_ref == 0)
		{
			// Passed linear offset stays after the exit from the call that does not belong to any source code.
			// Most likely this is the topmost lex source call. Such linear offset has no source location.
			return(FALSE);
		}

		// Ensure that the source ref of this call event corresponds to some other call event.
		assert(latest_call_src_ref < NumEventPtrs() && m_event_ptrs[latest_call_src_ref] != NULL);
		assert(m_event_ptrs[latest_call_src_ref]->hdr.event_type == pset_lex_source_enter);

		// Pick up an event that started this lex source enclosure.
		lex_src_enter = &(m_event_ptrs[latest_call_src_ref]->src_enter);
		lin_start = m_lex_src_index[inx]->src_exit.linear_pos_after_return;
		src_start = latest_call_event.src_offs_after_exit;
		calls_search_start = m_lex_src_index[inx]->src_exit.hdr.event_id+1;
	}

	//
	//  At this point it was possible to locate the call event that describes the enclosure in question.
	//  Retrieve the doc_id of the source file and pick up the end of the source area. Note that this end
	//  is based on the type of the lex source. It may happen that parsing session was aborted or this
	//  enclosure is currently being parsed and its end is simply not reached yet.
	//
	assert(lin_start > 0 && lin_start <= lin_area.linear_beg);
	TLongId src_doc_id = lex_src_enter->GetEnclosureDocId();

	// Start filling the buffer.
	buffer.Clear();
	buffer.parsing_id = m_session_header.parsing_id;
	buffer.lin_area.linear_beg = lin_area.linear_beg;
	buffer.source_ref = lex_src_enter->hdr.event_id;
	buffer.src_area.doc_id = src_doc_id;
	buffer.src_area.area_beg = src_start+(lin_area.linear_beg-lin_start);

	// Figure out what part of the passed linear area belongs to the lex source nesting that corresponds
	// to the beginning of this linear area.
	long lin_curr = lin_area.AreaBeg();
	long lin_end = lin_area.AreaEnd();
	ID curr_search_evid = calls_search_start;
	ID evid_after_limit = lex_src_enter->evt_exit_id;

	if (evid_after_limit == 0)
	{
		// The lex src enclose is not closed. Iterate all events up to the end of the parsing log.
		evid_after_limit = NumEventPtrs();
	}

	while (lin_curr < lin_end)
	{
		// Look for the lex src calls on the current lex sources nesting level. It is necessary to look for such calls
		// because once the exit event of the call is missing, this call will not be present in the index and it will not
		// be discovered in the search above. The whole loop is needed only to safeguard the unclosed lex sources.
		CtvLexSourceEnterEvent *pLexSrcCall = NULL;
		while (curr_search_evid < evid_after_limit)
		{
			CtvParsingEventInfo *pev = m_event_ptrs[curr_search_evid++];
			if (pev != NULL && pev->hdr.event_type == pset_lex_source_enter)
			{
				// A call statement is found.
				pLexSrcCall = &(pev->src_enter);
				break;
			}
		}

		// Check the results of the search.
		if (pLexSrcCall != NULL)
		{
			// Current scope still contains a lex source call.
			assert(pLexSrcCall->source_ref == buffer.source_ref);

			if (lin_end <= pLexSrcCall->hdr.area.AreaEnd())
			{
				// Linear area ended without entering this call. The source area is not partial.
				buffer.src_area.area_len += (lin_end-lin_curr);
				lin_curr = lin_end;
				break;
			}
			else
			{
				// This call was entered. Meanwhile extend the source area to the end of the call src area.
				buffer.src_area.area_len += (pLexSrcCall->hdr.area.AreaEnd()-lin_curr);
				lin_curr = pLexSrcCall->hdr.area.AreaEnd();

				if (pLexSrcCall->evt_exit_id > 0)
				{
					// The call is closed.
					assert(pLexSrcCall->evt_exit_linear_pos > 0);
					if (lin_end <= pLexSrcCall->evt_exit_linear_pos)
					{
						// Linear area ended inside this call.
						buffer.partial_src_area = TRUE;
						break;
					}

					// Push the linear offset to the point after the exit from this call. Do not shift over potential extra
					// paste buff consumed area after the call. This area does not differ from any other area after it.
					lin_curr = pLexSrcCall->evt_exit_linear_pos+1;
					curr_search_evid = pLexSrcCall->evt_exit_id+1;
				}
				else
				{
					// The call is not closed. This also means that the linear area ended somewhere inside this call.
					// The source area is partial and its first segment ends at the end of the call event area.
					buffer.partial_src_area = TRUE;
					break;
				}
			}
		}
		else
		{
			// Curr scope does not have more lex src calls.
			long scope_lin_end;
			if (lex_src_enter->evt_exit_id != 0)
			{
				// The call is closed. Pick up the linear offset of its exit event.
				scope_lin_end = lex_src_enter->evt_exit_linear_pos;
			}
			else
			{
				// The call is not closed. Use the passed max possible linear offset.
				scope_lin_end = max_linear_offs;
			}

			if (lin_end <= scope_lin_end)
			{
				// Linear area ended before the end or right at the end of the current source nesting. This means that
				// the source area is not partial.
				buffer.src_area.area_len += (lin_end-lin_curr);
				lin_curr = lin_end;
			}
			else
			{
				// Linear area sticks outside of the current enclosure. The source area is partial.
				buffer.src_area.area_len += (scope_lin_end-lin_curr);
				buffer.partial_src_area = TRUE;
				buffer.src_ext_goes_up = TRUE;
				lin_curr = scope_lin_end;
			}

			break;
		}
	}

	// Finalize filling the buffer.
	buffer.lin_area.linear_len = lin_curr-lin_area.linear_beg;
	assert(buffer.lin_area.linear_len >= 0);
	return(TRUE);
}

int TMidLevRamBasedParsingLog::LexSrcIndexIndexFromLinearOffs(long lin_offs)
{
	if (m_lex_src_index.NumItems() == 0 || lin_offs < LexSrcLinearOffs(0))
	{
		// Parsing session does not have lex source nestings or the passed offset stays before
		// the first lex src enter statement.
		return(-1);
	}

	// Use the binary search.
	int lo_good = 0;
	int hi_bad = m_lex_src_index.NumItems();
	while ((hi_bad - lo_good) > 1)
	{
		int inx_test = (hi_bad+lo_good)/2;
		if (lin_offs >= LexSrcLinearOffs(inx_test))
			lo_good = inx_test;
		else hi_bad = inx_test;
	}

	// Return the index of the element that starts contiguous linear area that covers the passed linear offset.
	assert(hi_bad == lo_good+1);
	return(lo_good);
}

int TMidLevRamBasedParsingLog::CondParsIndexIndexFromLinearOffs(long lin_offs)
{
	if (m_cond_pars_index.NumItems() == 0 || lin_offs < CondParsLinearOffs(0))
	{
		// Current parsing session does not have conditional parsing structures or the passed offset
		// stays before the first cond parsing statement.
		return(-1);
	}

	// Use the binary search.
	int lo_good = 0;
	int hi_bad = m_cond_pars_index.NumItems();
	while ((hi_bad - lo_good) > 1)
	{
		int inx_test = (hi_bad+lo_good)/2;
		if (lin_offs >= CondParsLinearOffs(inx_test))
			lo_good = inx_test;
		else hi_bad = inx_test;
	}

	// Return index of the element that starts the linear area that covers the passed linear offset.
	assert(hi_bad == lo_good+1);
	return(lo_good);
}

//---------------------------------------------------------------------------
//  ===============  TMidLevRamBasedDiskFileInfo  ====================
//---------------------------------------------------------------------------

bool TMidLevRamBasedDiskFileInfo::SetRawIndexingData(ID raw_sess_id, PrRawIndexingFileInfo &hdr_info, BYTE *inx_data, long inx_data_len)
{
	assert(data.doc_id == hdr_info.doc_id.doc_id);
	assert(indexing_data == NULL);

	indexing_data = (inx_data_len != 0) ? (BYTE*)malloc(inx_data_len) : NULL;
	if (inx_data_len != 0 && indexing_data == NULL)
	{
		// Memory problem.
		return(FALSE);
	}

	// Accept the rest of the data.
	data.raw_sess_id = raw_sess_id;
	data.raw_indexing_date = hdr_info.parsing_date;

	if (inx_data_len != 0)
		memcpy(indexing_data, inx_data, inx_data_len);

	return(TRUE);
}

bool TMidLevRamBasedDiskFileInfo::AddParsingId(ID pars_id)
{
	int inx = FindParsIdInx(pars_id);
	if (inx >= 0 && pars_refs[inx] == pars_id)
	{
		// Passed session id is already present in the list.
		return(TRUE);
	}

	if (pars_refs.InsertItem(inx+1, pars_id) == FALSE)
	{
		// This is a memory problem.
		return(FALSE);
	}

	data.pars_sess_refs_len = pars_refs.NumItems();
	return(TRUE);
}

bool TMidLevRamBasedDiskFileInfo::RemoveParsingId(ID pars_id)
{
	int inx = FindParsIdInx(pars_id);
	if (inx < 0 || pars_refs[inx] != pars_id)
	{
		// There is nothing to remove.
		return(FALSE);
	}

	// Parsing session with the passed id is available.
	pars_refs.ReleaseItem(inx);
	data.pars_sess_refs_len = pars_refs.NumItems();
	return(TRUE);
}

bool TMidLevRamBasedDiskFileInfo::CheckParsingId(ID pars_id)
{
	int inx = FindParsIdInx(pars_id);
	return((inx >= 0 && pars_refs[inx] == pars_id) ? TRUE : FALSE);
}

int TMidLevRamBasedDiskFileInfo::FindParsIdInx(ID pars_id)
{
	if (pars_refs.NumItems() == 0 || pars_id < pars_refs[0])
		return(-1);

	// Use the binary search.
	int lo_good = 0;
	int hi_bad = pars_refs.NumItems();
	while ((hi_bad - lo_good) > 1)
	{
		int inx_test = (hi_bad+lo_good)/2;
		if (pars_id >= pars_refs[inx_test])
			lo_good = inx_test;
		else hi_bad = inx_test;
	}

	assert(hi_bad == lo_good+1);
	return(lo_good);
}

//---------------------------------------------------------------------------
//  ===============  TMidLevRamBasedDiskFilesArray  ==================
//---------------------------------------------------------------------------

void TMidLevRamBasedDiskFilesArray::InitDiskFilesArray()
{
	Clear();

	// The array of disk files needs special initialization. It should be filled with empty records for the whole range
	// of built in file ids. These records should have file_id == 0 and they should not be present in the hash lists.
	TMidLevRamBasedDiskFileInfo data;
	data.Clear();

	assert(TFileInfo::DiskFileIdsBase <= MID_LEV_RAM_STORE_INITIAL_DISK_FILES_NUM);
	for (int cnt=1; cnt < TFileInfo::DiskFileIdsBase; ++cnt)
		AppendItem(data);
}

//---------------------------------------------------------------------------
//  ==============  TMidLevRamBasedIndexingKeyInfo  ==================
//---------------------------------------------------------------------------

bool TMidLevRamBasedIndexingKeyInfo::FindFileRefInfo(ID pars, ID fid, CtvKeyFileRefInfo &buffer)
{
	TIdStruct id = { pars, fid };
	int inx = GetFileRefIndex(id.doc_id);
	if (inx < 0 || file_refs[inx].doc_id != id.doc_id)
	{
		// Info about the file with the passed id is not available.
		return(FALSE);
	}

	// Give out the data.
	buffer = file_refs[inx];
	return(TRUE);
}

bool TMidLevRamBasedIndexingKeyInfo::FindParsRefInfo(ID pars, CtvKeyParsRefInfo &buffer)
{
	int inx = GetParsRefIndex(pars);
	if (inx < 0 || pars_refs[inx].pars_id != pars)
	{
		// Info about the parsing session with the passed id is not available.
		return(FALSE);
	}

	// Give out the data.
	buffer = pars_refs[inx];
	return(TRUE);
}

bool TMidLevRamBasedIndexingKeyInfo::UpdateFileRef(TLongId doc_id, int new_use_cnt, DWORD new_use_flags)
{
	int inx = GetFileRefIndex(doc_id);
	if (inx < 0 || file_refs[inx].doc_id != doc_id)
	{
		// Info about the passed file was not present before.
		if (new_use_cnt == 0)
		{
			// There is no need to remove info that was not present.
			return(TRUE);
		}
		else
		{
			// Add info about new file.
			CtvKeyFileRefInfo info = { data.key_id, doc_id, new_use_cnt, new_use_flags };
			if (file_refs.InsertItem(inx+1, info) == FALSE)
			{
				// Memory error.
				return(FALSE);
			}

			data.key_file_use_cnt += new_use_cnt;
			data.key_file_refs_len++;
		}
	}
	else
	{
		// This file is already known.
		if (new_use_cnt == 0)
		{
			// Remove info about this file.
			data.key_file_use_cnt -= file_refs[inx].use_cnt;
			data.key_file_refs_len--;
			file_refs.ReleaseItem(inx);
		}
		else
		{
			// Update info about this file.
			data.key_file_use_cnt -= file_refs[inx].use_cnt;
			data.key_file_use_cnt += new_use_cnt;
			file_refs[inx].use_cnt = new_use_cnt;
			file_refs[inx].use_flags = new_use_flags;
		}
	}

	// Success.
	return(TRUE);
}

bool TMidLevRamBasedIndexingKeyInfo::UpdateParsRef(ID pars, int new_use_cnt, DWORD new_use_flags)
{
	int inx = GetParsRefIndex(pars);
	if (inx < 0 || pars_refs[inx].pars_id != pars)
	{
		// Info about the passed parsing was not present before.
		if (new_use_cnt == 0)
		{
			// Remove info that was not present.
			return(TRUE);
		}
		else
		{
			// Add info about new file.
			CtvKeyParsRefInfo info = { data.key_id, pars, new_use_cnt, new_use_flags };
			if (pars_refs.InsertItem(inx+1, info) == FALSE)
			{
				// Memory error.
				return(FALSE);
			}

			// Update the data.
			data.key_pars_use_cnt += new_use_cnt;
			data.key_pars_refs_len++;
		}
	}
	else
	{
		// This file is already known.
		if (new_use_cnt == 0)
		{
			// Remove info about this file.
			data.key_pars_use_cnt -= pars_refs[inx].use_cnt;
			data.key_pars_refs_len--;
			pars_refs.ReleaseItem(inx);
		}
		else
		{
			// Update info about this file.
			data.key_pars_use_cnt -= pars_refs[inx].use_cnt;
			data.key_pars_use_cnt += new_use_cnt;
			pars_refs[inx].use_cnt = new_use_cnt;
			pars_refs[inx].use_flags = new_use_flags;
		}
	}

	// Success.
	return(TRUE);
}

int TMidLevRamBasedIndexingKeyInfo::GetFileRefIndex(TLongId id)
{
	if (file_refs.NumItems() == 0 || id < file_refs[0].doc_id)
		return(-1);

	// Use the binary search.
	int lo_good = 0;
	int hi_bad = file_refs.NumItems();
	while ((hi_bad - lo_good) > 1)
	{
		int inx_test = (hi_bad+lo_good)/2;
		if (id >= file_refs[inx_test].doc_id)
			lo_good = inx_test;
		else hi_bad = inx_test;
	}

	assert(hi_bad == lo_good+1);
	return(lo_good);
}

int TMidLevRamBasedIndexingKeyInfo::GetParsRefIndex(ID pars)
{
	if (pars_refs.NumItems() == 0 || pars < pars_refs[0].pars_id)
		return(-1);

	// Use the binary search.
	int lo_good = 0;
	int hi_bad = pars_refs.NumItems();
	while ((hi_bad - lo_good) > 1)
	{
		int inx_test = (hi_bad+lo_good)/2;
		if (pars >= pars_refs[inx_test].pars_id)
			lo_good = inx_test;
		else hi_bad = inx_test;
	}

	assert(hi_bad == lo_good+1);
	return(lo_good);
}

//-----------------------------------------------------------------------------
//  ================  TMidLevRamParsingDataStorage  ===================
//-----------------------------------------------------------------------------

TMidLevRamBasedDataStorage::TMidLevRamBasedDataStorage()
{
	m_batch_id_cnt = 4;
	m_pars_id_cnt = 20;
}

TMidLevRamBasedDataStorage::~TMidLevRamBasedDataStorage()
{
	m_parsings.DeleteAll();

	// Iteration should be started from 1 because the base class is not using the slot with the index 0.
	for (int idf=1; idf<m_disk_files.NumItems(); ++idf)
		m_disk_files[idf].ReleaseData();
	for (int iky=1; iky<m_indexing_keys.NumItems(); ++iky)
		m_indexing_keys[iky].ReleaseData();
}

TMidLevRamBasedParsingLog *TMidLevRamBasedDataStorage::CreateParsingLog(PrInitParsingInfo1 &init_info)
{
	TMidLevRamBasedParsingLog *new_log;
	new_log = new TMidLevRamBasedParsingLog(&m_strs_dict);
	if (new_log == NULL)
		return(NULL);

	if (new_log->SetupHeader(init_info, m_pars_id_cnt++) == FALSE)
	{
		delete new_log;
		return(NULL);
	}

	// Set the preliminary lang type to mark this session as non raw.
	new_log->m_session_header.lang_type = lang_unknown;

	m_parsings.AppendItem(new_log);
	return(new_log);
}

ID TMidLevRamBasedDataStorage::LookupOrRegisterDiskFile(TFileInfo &info, ID bk_ref_pars_id, ID &raw_indexing_sess_id)
{
	// Ensure that this is not a temp file or other parsing session dependent type of file.
	assert(info.pars_id == 0);

	int inx = 0;
	int prev_num_files = m_disk_files.NumRealItems();
	if (info.file_id != 0)
	{
		// It looks like this file is already registered.
		inx = (int)info.file_id;
		assert(inx > 0 && inx < m_disk_files.NumItems());

		if (info.file_id < TFileInfo::DiskFileIdsBase && m_disk_files[inx].data.file_id == 0)
		{
			// This is a built in file that is not registered yet. Register this file.
			wchar_t *fn = m_strs_dict.RegisterStr(info.info.file_name);
			if (fn == NULL)
				return(0);

			m_disk_files[inx].Init(info, fn);
			m_disk_files[inx].data.file_id = info.file_id;
			m_disk_files[inx].data.info.file_body = NULL;
			m_disk_files.AddToHashIndex(inx);
		}

		// Ensure that registered file is identical to the passed one.
		assert(wcscmp(info.info.file_name, m_disk_files[inx].data.info.file_name) == 0);
	}
	else
	{
		// Create a proto file item for this file.
		TMidLevRamBasedDiskFileInfo proto_item;
		proto_item.Init(info, info.info.file_name);

		// Check for identical item.
		inx = m_disk_files.FindOrAppend(proto_item);
		if (prev_num_files != m_disk_files.NumRealItems())
		{
			// This file was just added to the table.
			wchar_t *fn = m_strs_dict.RegisterStr(info.info.file_name);
			if (fn == NULL)
			{
				// Release this record and return.
				m_disk_files.ReleaseLatestItem();
				return(0);
			}

			m_disk_files[inx].data.info.file_name = fn;
			m_disk_files[inx].data.file_id = inx;
		}
	}

	// Check for failure on the hashed array level.
	if (inx <= 0)
		return(0);

	if (bk_ref_pars_id != 0)
	{
		// Add passed pars_id to back refs list of this file.
		if (m_disk_files[inx].AddParsingId(bk_ref_pars_id) == FALSE)
		{
			if (prev_num_files != m_disk_files.NumRealItems())
			{
				// This file was added just now. Cancel its registration.
				m_disk_files.ReleaseLatestItem();
			}
			else
			{
				// Situation is not nice. The requested file was already present in the database
				// and updating its back refs list failed. Return failure as an overall result.
			}

			return(0);
		}
	}

	// Everyting succeeded.
	raw_indexing_sess_id = m_disk_files[inx].data.raw_sess_id;
	return(inx);
}

ID TMidLevRamBasedDataStorage::LookupOrRegisterIndexingKey(TKeyInfoItem &info)
{
	int inx = 0;
	int prev_num_keys = m_indexing_keys.NumRealItems();
	if (info.data.key_id != 0)
	{
		// It looks like this param is already registered. Recheck that this is the same param.
		inx = (int)info.data.key_id-TKeyInfo::IndexingKeyIdsBase;
		assert(inx > 0 && inx < m_indexing_keys.NumItems());
		assert(info.data.key_type == m_indexing_keys[inx].data.key_type);
		assert(wcscmp(info.data.key_name, m_indexing_keys[inx].data.key_name) == 0);
	}
	else
	{
		// Create proto item for this indexing key.
		TMidLevRamBasedIndexingKeyInfo proto_item;
		proto_item.Init(info.data.key_type, info.data.key_name);

		// Check for identical item.
		inx = m_indexing_keys.FindOrAppend(proto_item);
		if (prev_num_keys != m_indexing_keys.NumRealItems())
		{
			// This key was just added to the table.
			wchar_t *kn = m_strs_dict.RegisterStr(info.data.key_name);
			if (kn == NULL)
			{
				// Release this record and return.
				m_indexing_keys.ReleaseLatestItem();
				return(0);
			}

			m_indexing_keys[inx].data.key_name = kn;
			m_indexing_keys[inx].data.key_id = inx+TKeyInfo::IndexingKeyIdsBase;
		}
	}

	// Check for allocation success.
	return((inx > 0) ? inx+TKeyInfo::IndexingKeyIdsBase : 0);
}

TMidLevRamBasedParsingLog *TMidLevRamBasedDataStorage::FindParsingLog(ID pars_id, ID *p_min_pid, ID *p_max_pid)
{
	// Loop over all known parsings to get the requested parsing log. During iteration collect info
	// about the min and max available parsing ids to allow showing better error message.
	ID min_pid = 0, max_pid = 0;
	TMidLevRamBasedParsingLog *curr_log = (TMidLevRamBasedParsingLog*)m_parsings.GetLast();
	if (curr_log != NULL)
	{
		min_pid = curr_log->m_session_header.parsing_id;
		max_pid = min_pid;
	}

	while (curr_log != NULL)
	{
		if (curr_log->m_session_header.parsing_id == pars_id)
		{
			return(curr_log);
		}

		curr_log = (TMidLevRamBasedParsingLog*)m_parsings.GetPrev(curr_log);
		if (curr_log != NULL)
			min_pid = curr_log->m_session_header.parsing_id;
	}

	// Requested parsing session is missing. Give out the min and/or the max info.
	if (p_min_pid != NULL)
		*p_min_pid = min_pid;
	if (p_max_pid != NULL)
		*p_max_pid = max_pid;
	return(NULL);
}

void TMidLevRamBasedDataStorage::TakeContentsFrom(TMidLevRamBasedDataStorage &other_inst)
{
	m_parsings.TakeDataFrom(other_inst.m_parsings);

	m_batch_id_cnt = other_inst.m_batch_id_cnt;
	m_pars_id_cnt = other_inst.m_pars_id_cnt;
	other_inst.m_batch_id_cnt = 4;
	other_inst.m_pars_id_cnt = 20;

	m_disk_files.TakeContentsFrom(other_inst.m_disk_files);
	other_inst.m_disk_files.InitDiskFilesArray();

	m_indexing_keys.TakeContentsFrom(other_inst.m_indexing_keys);
	m_strs_dict.TakeContentsFrom(other_inst.m_strs_dict);
}

//--------------------------------------------------------------------------------
//  =====================  TMidLevRamLogger  =========================
//--------------------------------------------------------------------------------

bool TMidLevRamLogger::LookupOrRegisterDiskFileInfo(TFileInfo &info)
{
	assert(m_cp != NULL || m_raw_indexing_sess == TRUE);
	assert(info.pars_id == 0);

	// This function differs from other functions in the cbk handler. It should not show error
	// in the case of failure.
	ID raw_indexing_sess_id;
	ID fid = m_data.LookupOrRegisterDiskFile(info, ((m_raw_indexing_sess == TRUE) ? ParsingId() : 0), raw_indexing_sess_id);
	if (fid != 0)
	{
		// Fill in the retrieved info into the passed structure.
		info.file_id = fid;
		info.raw_sess_id = raw_indexing_sess_id;
	}

	return((fid != 0) ? TRUE : FALSE);
}

bool TMidLevRamLogger::LookupOrRegisterIndexingKey(TKeyInfoItem &info)
{
	assert(m_cp != NULL || m_raw_indexing_sess == TRUE);

	ID kid = m_data.LookupOrRegisterIndexingKey(info);
	if (kid != 0)
	{
		info.data.key_id = kid;
	}
	else
	{
		ShowProcessingError(L"Error registering an indexing key.\r\n\r\n"
						L"   Param name: \t%.120s%s\r\n"
						L"   Param type: \t%d",
						info.data.key_name, ((wcslen(info.data.key_name) > 120) ? L"..." : L""), info.data.key_type);
	}

	return((kid != 0) ? TRUE : FALSE);
}

bool TMidLevRamLogger::StartRawIndexingSession(PrInitParsingInfo1 &init_info)
{
	assert(m_cp == NULL);
	return(TMidLevScanCbkHandler::StartRawIndexingSession(init_info));
}

void TMidLevRamLogger::FinishRawIndexingSession(TRawIndexingSessSummary &sumry, TFileInfoItem *mj_src_file)
{
	assert(m_cp != NULL);
	m_cp->FinalizeHeaderInfoRawSess(sumry, mj_src_file);
	m_cp = NULL;

	// Finalize session in the base class.
	TMidLevScanCbkHandler::FinishRawIndexingSession(sumry, mj_src_file);
}

bool TMidLevRamLogger::StartParsing(PrInitParsingInfo1 &init_info)
{
	assert(m_cp == NULL && m_raw_indexing_sess == FALSE);

	if (m_single_parsing_logger == TRUE)
		m_data.DeleteAllParsingLogs();

	// Create the new parsing session.
	m_cp = m_data.CreateParsingLog(init_info);
	if (m_cp == NULL)
	{
		ShowProcessingError(L"Low on memory in the TMidLevRamLogger::StartParsing.");
		return(FALSE);
	}

	// Fill in the data fields of the base class.
	m_curr_batch_id = m_cp->m_session_header.batch_id;
	m_curr_parsing_id = m_cp->m_session_header.parsing_id;
	m_raw_indexing_sess = FALSE;
	return(TRUE);
}

bool TMidLevRamLogger::StartPrepPhase(PrInitParsingInfo2 &init_info)
{
	assert(m_cp != NULL);
	if (m_cp->SetupPrepHeaderInfo(init_info) == FALSE)
	{
		ShowProcessingError(L"Low on memory in the TMidLevRamLogger::StartPrepPhase.");
		return(FALSE);
	}

	return(TRUE);
}

bool TMidLevRamLogger::StartMainPhase(PrInitParsingInfo3 &init_info)
{
	assert(m_cp != NULL);
	if (m_cp->SetupMainHeaderInfo(init_info) == FALSE)
	{
		ShowProcessingError(L"Low on memory in the TMidLevRamLogger::StartMainPhase.");
		return(FALSE);
	}

	if (init_info.major_src_file->data.pars_id == 0)
	{
		// Major src file of the sesssion is the parsing independent file. This is typical case. Fill in the curr parsing
		// session id into the descriptor of this file. This file should be already registered. Ensure this.
		ID fid = init_info.major_src_file->data.file_id;
		if (fid < TFileInfo::DiskFileIdsBase || fid >= (ID)m_data.m_disk_files.NumItems())
		{
			ShowProcessingError(L"Fid of the major source file is bogus in the TMidLevRamLogger::StartMainPhase.");
			return(FALSE);
		}
		else if (m_data.m_disk_files[fid].data.file_id == 0)
		{
			ShowProcessingError(L"Major source file is not registered in the TMidLevRamLogger::StartMainPhase.");
			return(FALSE);
		}

		// Establish a link in the single linked list.
		TFileInfo &mj_src_info = m_data.m_disk_files[fid].data;
		ID old_sess_id = mj_src_info.major_src_latest_pars_id;
		mj_src_info.major_src_latest_pars_id = ParsingId();
		m_cp->m_session_header.major_src_prev_pars_id = old_sess_id;
	}

	return(TRUE);
}

void TMidLevRamLogger::FinalizeParsing(TMidLevPrepOptions *prep_options, TMidLevScannerOptions *scan_options, TMidLevParsingSummary &summary, int final_pars_status)
{
	// Finalize parsing session in the current parsing log.
	assert(m_cp != NULL);
	m_cp->FinalizeHeaderInfo(prep_options, scan_options, summary, final_pars_status);
	m_cp = NULL;

	// Finalize parsing session in the base class.
	TMidLevScanCbkHandler::FinalizeParsing(prep_options, scan_options, summary, final_pars_status);
}

// ------------ Parsing events handling -----------------

void TMidLevRamLogger::RegisterDriveSubstData(PrDriveSubstRegistration *info)
{
	assert(m_cp != NULL);
	int cnt_errs = m_cp->GetErrCnt();

	CtvDriveSubstRegEvent data;
	data.hdr.Init(ParsingId(), pset_register_drive_subst, info->hdr);
	data.subst_info.internal_prefix = m_cp->RegisterStr(info->subst_info.internal_prefix);
	data.subst_info.external_subst = m_cp->RegisterStr(info->subst_info.external_subst);

	// Check for allocation errors.
	if (cnt_errs != m_cp->GetErrCnt())
	{
		ShowEventPreparationError(info->hdr, data.hdr.event_type);
		return;
	}

	if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(CtvDriveSubstRegEvent)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
	}
}

void TMidLevRamLogger::RegisterTempFileData(PrTempFileRegistration *info)
{
	assert(m_cp != NULL);
	assert(info->file_info.pars_id == m_cp->m_session_header.parsing_id);
	assert(info->file_info.info.file_name != NULL && info->file_info.info.file_body != NULL);
	assert(info->file_info.strg_type == strg_temp || info->file_info.strg_type == strg_interm);

	// Move the file name and the body of the file into the buffers of the parsing session log.
	TBasicFileInfo &bfi = info->file_info.info;
	wchar_t *fn = m_cp->StrDupe(bfi.file_name);
	wchar_t *fb = m_cp->StrDupe(bfi.file_body, bfi.file_len);
	if (fn == NULL || fb == NULL)
	{
		ShowProcessingError(L"Error storing the file name or the file body of the temporary file.");
		return;
	}

	// Allocate the lines info if needed.
	TSimpleLineInfo *linfo = NULL;
	long linfo_len = 0, longest_line_len = 0;
	if (info->file_info.rscn_type != rscn_binary)
	{
		// Temp file is a text file. Create the lines info in the heap.
		linfo = TSimpleLineInfo::BuildLinesInfoFromData(bfi.file_body, bfi.file_len, 0, linfo_len, longest_line_len);
		if (linfo == NULL || linfo_len <= 0)
		{
			ShowProcessingError(L"Error building the the lines info of the temporary file.");
			return;
		}

		// Duplicate lines info into the data buffers and release the lines info in the heap.
		wchar_t *wclinfo = m_cp->StrDupe((wchar_t*)linfo, linfo_len*sizeof(TSimpleLineInfo)/sizeof(wchar_t));
		TSimpleLineInfo::FreeLinesInfo(&linfo);
		if (wclinfo == NULL)
		{
			ShowProcessingError(L"Error storing the the lines info of the temporary file.");
			return;
		}

		// Allocations went fine. Give out the new lines info.
		linfo = (TSimpleLineInfo*)wclinfo;
	}

	// Prepare the temp file descriptor record.
	TMidLevRamLogTempFileRecord data;
	memset(&data, 0, sizeof(TMidLevRamLogTempFileRecord));
	data.hdr.Init(ParsingId(), pset_register_temp_file, info->hdr);
	data.temp_file_info = info->file_info;
	data.temp_file_info.info.file_name = fn;
	data.temp_file_info.info.file_body = fb;
	data.temp_file_info.lines_info = linfo;
	data.temp_file_info.lines_info_len = linfo_len;
	data.temp_file_info.longest_line_len = longest_line_len;
	data.temp_file_info.pars_sess_refs_len = 1;

	if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(TMidLevRamLogTempFileRecord)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
	}
}

void TMidLevRamLogger::PasteBuffChangeNotification(PrPasteBufferingChange *info)
{
	assert(m_cp != NULL);

	if (info->cn_enter != 0)
	{
		// Check presence of the buffering enter event.
		if (info->cn_enter >= m_cp->NumEventPtrs() || m_cp->m_event_ptrs[info->cn_enter] == NULL)
		{
			ShowProcessingError(L"Paste buffering enter event is missing while attempting to process the paste buffering exit event.\r\n\r\n"
							L"   Parsing id: \t\t%lu\r\n"
							L"   Paste buffering enter event id: \t%lu\r\n"
							L"   Paste buffering exit event id: \t%lu",
							m_cp->m_session_header.parsing_id, info->cn_enter, info->hdr.cn);
			return;
		}
	}

	// Convert the Pr event into the Ctv event. Failure cannot happen during this preparation.
	CtvPasteBufferingChgEvent data;
	data.hdr.Init(ParsingId(), pset_paste_buffering_change, info->hdr);
	data.parent_buff_ref = info->parent_cn;
	data.evt_enter_cn = info->cn_enter;
	data.new_buff_level = info->new_buff_level;
	data.buff_type = info->buff_type;
	data.buffer_fid = info->buffer->data.file_id;
	data.evt_exit_linear = 0;
	data.real_linear_end = 0;						// Real value will be set up later when this paste buffer
												// will be completely scanned as the source of lexemas.

	if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(CtvPasteBufferingChgEvent)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
	}

	if (info->cn_enter != 0)
	{
		// Modifiy the paste buffering enter event. Do this after the last potentital point of failure.
		CtvPasteBufferingChgEvent &evt_enter = m_cp->m_event_ptrs[info->cn_enter]->pbuff_chg;
		assert(evt_enter.hdr.event_type == pset_paste_buffering_change);
		evt_enter.evt_exit_linear = data.hdr.area.linear_beg;
	}
}

void TMidLevRamLogger::LexSourceEnterNotification(PrLexSourceEnter *info)
{
	assert(m_cp != NULL);
	int cnt_errs = m_cp->GetErrCnt();

	// Prepare the header section.
	TMidLevRamLogLexSourceEnterEvent data;
	data.Clear();
	data.hdr.Init(ParsingId(), pset_lex_source_enter, info->hdr);

	// Copy the referencies.
	data.source_ref = info->hdr.source_ref;
	data.cond_ref = info->hdr.cond_ref;
	data.paste_buffering_ref = info->pbuff_enter_ref;

	// General info about the lex source.
	data.src_type = info->src_type;
	data.on_doubt = info->on_doubt;
	data.inc_fn_mode = info->inc_fn_mode;
	data.new_src_level = info->new_src_level;

	switch (data.src_type)
	{
		case srct_cmdl:
		case srct_ant_env:
		case srct_file:
		case srct_bltin_defs:
		case srct_mj_src:
				{
					// Group 1. File source. The file that is being entered should be present.
					// Duplicate its name and kill the file body and the lines info.
					assert(info->include_file != NULL);
					data.inc_path_id = info->inc_path_id;
					data.inc_file_info = info->include_file->data;
					data.inc_file_info.info.file_name = m_cp->RegisterStr(info->include_file->data.info.file_name);
					data.inc_file_info.info.file_body = NULL;
					data.inc_file_info.AcceptLinesInfo(NULL, 0, 0);
					data.inc_token_area = info->mpr_token.lin_area;
					data.inc_fname_area = info->stmt_fname.lin_area;
					data.pos_params_cnt = info->pos_params_cnt;
					data.evt_exit_pos_prms_cnt = -1;
				}
				break;

		case srct_scgm:
				{
					// Group 2. Self changing macro.
					data.scgm_scan_area.Init(info->scgm_scan_area);
				}
				break;

		case srct_macro:
		case srct_mparam:
				{
					// Group 3. Macro source.
					if (info->src_type == srct_macro)
					{
						// Fill in data from the macro definition.
						data.mcall_def_id = info->macro->hdr.cn;
						data.mcall_def_name = m_cp->RegisterStr(info->macro->def_name);
						data.mcall_def_area.Init(info->macro->macro_body.src_area);
					}
					else
					{
						// For the macro param calls the mcall_def_id field should be zero because the macro params are
						// basically a temporary macros that are existing only during the macro call. These macro are not
						// explicitly registered in the database. This means that they do not have any Id.
						data.mcall_def_id = 0;
						if (info->param_inx < info->macro->num_params)
						{
							// This is a non vararg param that has a name.
							data.mcall_def_name = m_cp->RegisterStr(info->macro->def_param_name[info->param_inx]);
						}

						data.mcall_def_area.Init(info->name_area.src_area);
					}

					data.mcall_xpnd_macros = info->xpnd_macros;
					data.mcall_paste_prefix = info->paste_prefix;
					data.mcall_paste_suffix = info->paste_suffix;

					if (info->src_type == srct_mparam)
					{
						data.mcall_param_inx = info->param_inx;
						data.mcall_param_decor = info->param_decor;
					}
					else if (info->src_type == srct_macro)
					{
						data.mcall_name_area = info->name_area.lin_area;
					}
				}
				break;

		case srct_pbuff_macro:
		case srct_pbuff_mparam:
				{
					// Group 4. Paste buffering.
					assert(info->paste_buff != NULL);
					data.append_tail = info->append_tail;
					data.paste_buff_fid = info->paste_buff->data.file_id;
					data.paste_buff_len = 0;								// Real value will be set when the exit event will be processed.
				}
				break;

		default:
			{
				// Do not allow unsupported types of lex sources.
				assert(FALSE);
			}
			break;
	}

	// Check for allocation errors and process the event.
	if (cnt_errs != m_cp->GetErrCnt())
	{
		ShowEventPreparationError(info->hdr, data.hdr.event_type);
		return;
	}
	else if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(TMidLevRamLogLexSourceEnterEvent)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
		return;
	}

	// The basic processing of the event succeeeded.
	if (info->src_type == srct_mj_src)
	{
		// Store the id of the current event.
		m_cp->m_session_header.major_src_enter_evt = info->hdr.cn;
	}
}

void TMidLevRamLogger::LexSourceExitNotification(PrLexSourceExit *info)
{
	assert(m_cp != NULL);

	// Check the presence of the lex source enter event.
	if (info->hdr.source_ref >= m_cp->NumEventPtrs() || m_cp->m_event_ptrs[info->hdr.source_ref] == NULL)
	{
		ShowProcessingError(L"Lex source enter event is missing while attempting to process the lex source exit event.\r\n\r\n"
						L"   Parsing id: \t\t%lu\r\n"
						L"   Lex source enter event id: \t%lu\r\n"
						L"   Lex source exit event id: \t%lu",
						m_cp->m_session_header.parsing_id, info->hdr.source_ref, info->hdr.cn);
		return;
	}

	// Convert the Pr event into the Ctv event. Failure cannot happen during this preparation.
	CtvLexSourceExitEvent data;
	data.hdr.Init(ParsingId(), pset_lex_source_exit, info->hdr);
	data.source_ref = info->hdr.source_ref;
	data.cond_ref = info->hdr.cond_ref;
	data.paste_buff_ref = info->pbuff_exit_ref;
	data.src_type = info->src_type;
	data.linear_pos_after_return = info->linear_pos_after_return;

	// Send prepared event for processing.
	if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(CtvLexSourceExitEvent)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
		return;
	}

	// Modifiy the lex source enter event. Do this after the last potentital point of failure.
	CtvLexSourceEnterEvent &evt_enter = m_cp->m_event_ptrs[info->hdr.source_ref]->src_enter;
	assert(evt_enter.hdr.event_type == pset_lex_source_enter);

	evt_enter.evt_exit_id = info->hdr.cn;
	evt_enter.evt_exit_cond_ref = info->hdr.cond_ref;
	evt_enter.evt_exit_pbuff_ref = info->pbuff_exit_ref;

	evt_enter.evt_exit_linear_pos = info->hdr.lin_area.AreaBeg();
	evt_enter.evt_exit_linear_after_ret = info->linear_pos_after_return;
	evt_enter.src_offs_after_exit = info->src_offs_after_return;

	if (LEX_SRC_WHOLE_FILE(info->src_type) == TRUE)
	{
		// Accept the currnt count of the pos params in the cmdl.
		evt_enter.evt_exit_pos_prms_cnt = info->pos_params_count;
	}
	else if (LEX_SRC_GEN_PBUFF(info->src_type) == TRUE)
	{
		if (info->pbuff_origin_exit_evid != 0)
		{
			// Update the paste buffering exit event.
			CtvPasteBufferingChgEvent &evt_pbuff_exit = m_cp->m_event_ptrs[info->pbuff_origin_exit_evid]->pbuff_chg;
			assert(evt_pbuff_exit.hdr.event_type == pset_paste_buffering_change);
			assert(evt_pbuff_exit.evt_enter_cn != 0);

			evt_pbuff_exit.real_linear_end = info->linear_pos_after_return;

			// Update the paste buffering enter event.
			CtvPasteBufferingChgEvent &evt_pbuff_enter = m_cp->m_event_ptrs[evt_pbuff_exit.evt_enter_cn]->pbuff_chg;
			assert(evt_pbuff_enter.hdr.event_type == pset_paste_buffering_change);
			assert(evt_pbuff_enter.evt_enter_cn == 0);

			evt_pbuff_enter.real_linear_end = info->linear_pos_after_return;
		}

		evt_enter.paste_buff_len = info->pbuff_final_length;
	}
}

void TMidLevRamLogger::MacroLibraryChangeNotification(PrMacroLibraryChange *info)
{
	assert(m_cp != NULL);
	int cnt_errs = m_cp->GetErrCnt();

	// Convert Pr event into the Ctv event.
	CtvMacroLibraryChangeEvent data;
	TMacroDefinition *prev_def = (TMacroDefinition*)(info->macro->redefinitions_history.GetFirst());
	data.hdr.Init(ParsingId(), pset_macro_library_change, info->hdr);
	data.prev_def_id = (prev_def != NULL) ? prev_def->hdr.cn : 0;
	data.mdef_status = info->macro->def_status;
	data.mdef_origin_type = info->macro->def_origin_type;
	data.mdef_key_id = info->macro->key_info->data.key_id;
	data.mdef_name = m_cp->RegisterStr(info->macro->def_name);
	data.num_params = (short)info->macro->num_params;
	data.va_args_tail = info->macro->var_args_macro;
	data.token_area = info->mpr_token.lin_area;
	data.name_area = info->name_area.lin_area;
	data.body_area = info->macro->macro_body.lin_area;

	// Check for the allocation errors.
	if (cnt_errs != m_cp->GetErrCnt())
	{
		ShowEventPreparationError(info->hdr, data.hdr.event_type);
		return;
	}

	// Send event for processing.
	if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(CtvMacroLibraryChangeEvent)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
	}
}

void TMidLevRamLogger::CondParsSttChangeNotification(PrCondParsingSttChange *info)
{
	assert(m_cp != NULL);

	if (info->prev_stmt_id != 0)
	{
		// Check the availability of the prev cond parsing event on this level.
		if (info->prev_stmt_id >= m_cp->NumEventPtrs() || m_cp->m_event_ptrs[info->prev_stmt_id] == NULL)
		{
			ShowProcessingError(L"Cond parsing event is missing while attempting to process the dependent cond parsing event.\r\n\r\n"
							L"   Parsing id: \t\t%lu\r\n"
							L"   Prev cond parsing event id: \t%lu\r\n"
							L"   Curr cond parsing event id: \t%lu",
							m_cp->m_session_header.parsing_id, info->prev_stmt_id, info->hdr.cn);
			return;
		}
	}

	// Convert Pr event into the Ctv event. Failure cannot happen during preparation of this event.
	CtvCondParsingChangeEvent data;
	data.hdr.Init(ParsingId(), pset_cond_parsing_change, info->hdr);
	data.source_ref = info->hdr.source_ref;
	data.cond_ref = info->hdr.cond_ref;
	data.prev_stmt_id = info->prev_stmt_id;
	data.next_stmt_id = 0;

	data.stmt_type = info->stmt_type;
	data.expr_smpl_res = info->expr_smpl_res;
	data.expr_result = info->expr_res;
	data.stmt_code_stt = info->stmt_code_stt;
	data.new_code_stt = info->new_code_stt;
	data.stmt_code_level = info->stmt_code_level;
	data.new_code_level = info->new_code_level;

	data.scan_expr_error_id = info->scan_expr_error;
	data.scan_expr_details = info->scan_expr_details;
	data.num_expr_nodes = (info->expr_code != NULL) ? info->expr_code->CountNodes() : 0;
	data.stmt_token_area = info->mpr_token.lin_area;
	data.cond_expr_area = info->expr_area.lin_area;

	// Send event for processing.
	if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(CtvCondParsingChangeEvent)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
		return;
	}

	// Modify description of the previous cond parsing event on the current level.
	// Do this after the last potential point of failure.
	if (info->prev_stmt_id != 0)
	{
		CtvCondParsingChangeEvent &cpars = m_cp->m_event_ptrs[info->prev_stmt_id]->cond_pars;
		assert(cpars.hdr.event_type == pset_cond_parsing_change);

		cpars.next_stmt_id = info->hdr.cn;
	}
}

void TMidLevRamLogger::MiscPreprocEventNotification(PrMiscPreprocStmtEvent *info)
{
	assert(m_cp != NULL);
	assert(info->hdr.src_area.area_len >= 0);
	int cnt_errs = m_cp->GetErrCnt();

	// Convert Pr event into the Ctv event.
	CtvMiscPreprocStmtEvent data;
	data.hdr.Init(ParsingId(), pset_misc_preproc_stmt, info->hdr);
	data.mis_event_type = info->ex_event_type;
	data.token_area = info->mpr_token.lin_area;
	data.code_stt = info->code_stt;
	data.evt_details = info->evt_details;
	data.evt_str_data = m_cp->RegisterStr(info->evt_str_data);
	data.line_num = info->line_num;
	data.line_num_area = info->line_area.lin_area;
	data.doc_ident = info->doc_ident;
	data.file_name = (info->fname_data.IsEmpty() == FALSE) ? m_cp->RegisterStr(info->fname_data) : NULL;
	data.fname_area = info->fname_area.lin_area;

	// Check for allocation errors and process the event.
	if (cnt_errs != m_cp->GetErrCnt())
	{
		ShowEventPreparationError(info->hdr, data.hdr.event_type);
		return;
	}
	else if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(CtvMiscPreprocStmtEvent)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
		return;
	}

	// The basic processing of the event succeeeded.
	if (info->ex_event_type == exmis_mjsrc_area)
	{
		m_cp->m_session_header.major_src_scan_evt = info->hdr.cn;
	}
}

void TMidLevRamLogger::NewIncludePathNotification(PrNewIncPathEvent *info)
{
	assert(m_cp != NULL);
	int cnt_errs = m_cp->GetErrCnt();

	// Convert Pr event into the Ctv event.
	CtvNewIncPathEvent data;
	data.hdr.Init(ParsingId(), pset_new_include_path, info->hdr);
	data.inc_path = m_cp->RegisterStr(info->inc_path);
	data.inc_directory = m_cp->RegisterStr(info->inc_directory);

	// Check for allocation errors.
	if (cnt_errs != m_cp->GetErrCnt())
	{
		ShowEventPreparationError(info->hdr, data.hdr.event_type);
		return;
	}

	// Send event for processing.
	if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(CtvNewIncPathEvent)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
	}
}

void TMidLevRamLogger::ParsingErrorNotification(PrErrorWarningEvent *info)
{
	assert(m_cp != NULL);
	int cnt_errs = m_cp->GetErrCnt();

	// Convert Pr event into the Ctv event.
	CtvErrorWarningEvent data;
	data.hdr.Init(ParsingId(), pset_error_warning_event, info->hdr);
	data.parent_event = info->parent_event;
	data.err_class = info->err_class;
	data.err_code = info->err_code;
	data.err_msg = m_cp->RegisterStr(info->err_msg);
	data.ext_context = info->ext_context.lin_area;

	// Check for allocation errors.
	if (cnt_errs != m_cp->GetErrCnt())
	{
		ShowEventPreparationError(info->hdr, data.hdr.event_type);
		return;
	}

	// Send event for processing.
	if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(CtvErrorWarningEvent)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
	}
}

void TMidLevRamLogger::CondParamUseNotification(PrCondParamUseEvent *info)
{
	assert(m_cp != NULL);
	assert(info->param != NULL);
	assert(info->param->item_type != cecit_value && info->param->item_type != cecit_operation);
	assert(info->param->key_info != NULL);
	int cnt_errs = m_cp->GetErrCnt();

	// Convert Pr event into the Ctv event.
	CtvCondParamUseEvent data;
	data.hdr.Init(ParsingId(), pset_cond_param_use, info->hdr);
	data.cond_stmt_id = info->cond_stmt_cn;
	data.cond_stmt_code_stt = info->code_stt;
	data.cond_param_id = info->param->key_info->data.key_id;
	data.cond_param_type = info->param->item_type;
	data.cond_param_name = m_cp->RegisterStr(info->param->key_info->data.key_name);

	if (data.cond_param_type == cecit_decorated || data.cond_param_type == cecit_name_def || data.cond_param_type == cecit_variable)
	{
		data.decor_mdef_id = info->param->decor_cn;
		data.decor_mdef_status = info->param->decor_state;
	}
	else
	{
		data.decor_mdef_id = 0;
		data.decor_mdef_status = mds_hard_undef;
	}

	// Check for allocation errors.
	if (cnt_errs != m_cp->GetErrCnt())
	{
		ShowEventPreparationError(info->hdr, data.hdr.event_type);
		return;
	}

	// Send event for processing.
	if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(CtvCondParamUseEvent)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
	}
}

void TMidLevRamLogger::GrammarSectionNotification(PrGrammarDefnSection *info)
{
	// Convert Pr event into the Ctv event.
	CtvGrammarDefnSection data;
	data.hdr.Init(ParsingId(), pset_grammar_section, info->hdr);
	data.section_type = info->section_type;

	// Send event for processing.
	if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(CtvGrammarDefnSection)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
	}
}

void TMidLevRamLogger::GrammarObjectNotification(PrGrammarDefnObject *info)
{
	// Convert Pr event into the Ctv event.
	CtvGrammarDefnObject data;
	data.hdr.Init(ParsingId(), pset_grammar_object, info->hdr);
	data.object_sym = info->object_sym;
	data.object_app_id = info->object_app_id;
	data.extra_context_id = info->extra_context_area.cn;

	// Send event for processing.
	if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(CtvGrammarDefnObject)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
	}
}

void TMidLevRamLogger::CppItemNotification(PrCppItemReferenceEvent *info)
{
	// Do minimal check on the passed event data.
	assert(info->cpp_item_type != cxx_none && info->cpp_item_id != 0);
	if (info->cpp_item_type < cxx_cond_pars_param || info->cpp_item_type >= cxx_num_types)
	{
		ShowEventContentsError(info->hdr, pset_cpp_item_ref);
		return;
	}
	else if (info->cpp_item_id == 0)
	{
		ShowEventContentsError(info->hdr, pset_cpp_item_ref);
		return;
	}

	if (info->event_related_areas.NumItems() == 0)
	{
		// Check, if passed event is a modification of the whole area of an already existing event or not.
		if (info->hdr.cn < m_cp->NumEventPtrs() && m_cp->m_event_ptrs[info->hdr.cn] != NULL)
		{
			// This is updating of an already existing event.
			CtvEventType existing_evt = m_cp->m_event_ptrs[info->hdr.cn]->hdr.event_type;
			if (existing_evt != pset_cpp_item_ref)
			{
				ShowEventContentsErrorEx(info->hdr, existing_evt, L"Event is not a CppItemRef event");
				return;
			}

			// Do a light check of other props of the event.
			CtvCppItemRefEvent &existing_ctv_ref = m_cp->m_event_ptrs[info->hdr.cn]->cpp_item;
			assert(existing_ctv_ref.cpp_item_type == info->cpp_item_type);
			assert(existing_ctv_ref.pars_sess_cpp_item_id == info->cpp_item_id);

			// Update the overall event area.
			existing_ctv_ref.hdr.area = info->hdr.lin_area;
			return;
		}
	}

	// Prepare a new CtvEvent.
	CtvCppItemRefEvent data;
	data.hdr.Init(ParsingId(), pset_cpp_item_ref, info->hdr);

	data.cpp_item_type = info->cpp_item_type;
	data.cpp_item_nesting = info->cpp_item_nesting;
	data.pars_sess_cpp_item_id = info->cpp_item_id;
	data.project_level_cpp_item_id = 0;

	// Process the first src area that should be stored in the core event record.
	if (info->event_related_areas.NumItems() > 0)
	{
		// Pick up the first area.
		data.ext_src_area.m_cpp_atype = info->event_related_areas[0].m_atype;
		data.ext_src_area.m_cpp_lin_area = info->event_related_areas[0].m_header.lin_area;
	}
	else
	{
		// Mark the field as empty.
		memset(&data.ext_src_area, 0, sizeof(TCppSourceArea));
	}

	data.extra_areas_temp_file_id = 0;
	if (info->event_related_areas.NumItems() > 1)
	{
		// Passed event contains more than one extra source area. A temp file is needed to store this info.
		ID cn = m_scanner->GetNextCN();
		wchar_t bin_file_name[40];
		swprintf(bin_file_name, 40, L"@CppAreas%d", cn);

		// Ensure that the temp data buffer has enough space in it.
		int num_file_records = info->event_related_areas.NumItems()-1;
		if (m_cpp_src_areas_buffer.NumAllocedItems() < num_file_records && m_cpp_src_areas_buffer.ReserveTotalBufferSpace(num_file_records) == FALSE)
		{
			ShowEventProcessingError(info->hdr, data.hdr.event_type);
			return;
		}

		// Prepare data in the local buffer.
		TCppSourceAreaHeader *src_hdr = info->event_related_areas.ItemPtr(1);
		for (int inx=0; inx<num_file_records; ++inx, ++src_hdr)
		{
			m_cpp_src_areas_buffer[inx].m_cpp_atype = src_hdr->m_atype;
			m_cpp_src_areas_buffer[inx].m_cpp_lin_area = src_hdr->m_header.lin_area;
		}

		// Prepare the temp file descriptor.
		PrTempFileRegistration reg_info;
		reg_info.hdr.Setup(&(info->hdr), m_scanner->GetNextCN());
		reg_info.hdr.AdjustToAreaBeg();

		reg_info.file_info.Clear();
		reg_info.file_info.file_id = cn;
		reg_info.file_info.info.file_name = bin_file_name;
		reg_info.file_info.info.file_body = (wchar_t*)m_cpp_src_areas_buffer.DataPtr();
		reg_info.file_info.info.file_len = num_file_records*sizeof(TCppSourceArea)/sizeof(wchar_t);
		reg_info.file_info.strg_type = strg_interm;
		reg_info.file_info.rscn_type = rscn_binary;
		reg_info.file_info.pars_sess_refs_len = 1;

		// Submit this event for processing.
		int cnt_errs = m_cp->GetErrCnt();
		RegisterTempFileData(&reg_info);
		m_scanner->PostProsessMidLevCbkCall(reg_info.hdr);
		if (cnt_errs != m_cp->GetErrCnt())
		{
			// Do not store the main event.
			return;
		}

		// Place the Id of the temp file into the main data record.
		data.extra_areas_temp_file_id = cn;
	}

	// Send the main event for processing.
	if (m_cp->AddEvent((CtvParsingEventInfo*)&data, sizeof(CtvCppItemRefEvent)) == FALSE)
	{
		ShowEventProcessingError(info->hdr, data.hdr.event_type);
	}
}

bool TMidLevRamLogger::SetParsSessIndexingKeyUseStatus(TKeyInfoItem *info)
{
	assert(m_curr_parsing_id != 0);
	assert(info->data.key_id != 0);

	int inx = info->data.key_id-TKeyInfo::IndexingKeyIdsBase;
	TMidLevRamBasedIndexingKeyInfo &ki = m_data.m_indexing_keys[inx];
	return(ki.UpdateParsRef(m_curr_parsing_id, info->use_cnt, info->use_flags));
}

bool TMidLevRamLogger::SetRawSessIndexingKeyUseStatus(TLongId doc_id, TKeyInfoItem *info)
{
	assert(m_raw_indexing_sess == TRUE);
	assert(info->data.key_id != 0);

	int inx = info->data.key_id-TKeyInfo::IndexingKeyIdsBase;
	TMidLevRamBasedIndexingKeyInfo &ki = m_data.m_indexing_keys[inx];
	return(ki.UpdateFileRef(doc_id, info->use_cnt, info->use_flags));
}

bool TMidLevRamLogger::SetRawIndexingData(PrRawIndexingFileInfo &hdr_info, BYTE *inx_data, long inx_data_len)
{
	assert(m_raw_indexing_sess == TRUE);
	assert(hdr_info.doc_id.doc_id != 0);

	// Check the type of the file.
	if (hdr_info.doc_id.pars_id == 0)
	{
		// This is a disk file.
		int inx = hdr_info.doc_id.file_id;
		if (inx <= 0 || inx >= m_data.m_disk_files.NumItems() || m_data.m_disk_files[inx].data.file_id != hdr_info.doc_id.file_id)
			return(FALSE);

		TMidLevRamBasedDiskFileInfo &finfo = m_data.m_disk_files[inx];
		return(finfo.SetRawIndexingData(m_curr_parsing_id, hdr_info, inx_data, inx_data_len));
	}
	else
	{
		// This is a temp file.
		TMidLevRamBasedParsingLog *log = m_data.FindParsingLog(hdr_info.doc_id.pars_id);
		if (log == NULL)
			return(FALSE);

		return(log->SetTempFileRawIndexingData(m_curr_parsing_id, hdr_info, inx_data, inx_data_len));
	}
}

TMidLevStoreReader *TMidLevRamLogger::AllocateReaderObject(wchar_t *buffer_4_max_path)
{
	// Database reader needs non empty error callback.
	assert(m_console != NULL);

#ifdef DISALLOW_ACTIVE_SESSION_READING
	// Check the current state of the logger.
	if (GetParsingId() != 0)
	{
		if (buffer_4_max_path != NULL)
			wcscpy(buffer_4_max_path, L"Unable to create a database reader object because the parsing session is currently acrive.");
		return(NULL);
	}
	else if (IsRawIndexingSessionActive() == TRUE)
	{
		if (buffer_4_max_path != NULL)
			wcscpy(buffer_4_max_path, L"Unable to create a database reader object because the raw indexing session is currently acrive.");
		return(NULL);
	}
#endif

	// Allocate a new reader.
	TMidLevRamStoreReader *reader = new TMidLevRamStoreReader();
	if (reader == NULL)
	{
		if (buffer_4_max_path != NULL)
			wcscpy(buffer_4_max_path, L"Error allocating a new TMidLevRamStoreReader object.");
		return(NULL);
	}

	// Prepare the reader.
	reader->SetupDataSource(&m_data);
	reader->SetupErrorCallback(m_console);

	// Clear the error message buffer.
	if (buffer_4_max_path != NULL)
		*buffer_4_max_path = 0;
	return(reader);
}

void TMidLevRamLogger::DisposeReaderObject(TMidLevStoreReader *reader)
{
	delete reader;
}

void TMidLevRamLogger::AttachReader(TMidLevRamStoreReader *reader)
{
	// Note that this reader may have its own callback handler.
	reader->SetupDataSource(&m_data);
}

void TMidLevRamLogger::TakeContentsFrom(TMidLevRamLogger &other_inst)
{
	// Parsing should not be active.
	assert(m_cp == NULL);
	assert(other_inst.m_cp == NULL);

	m_data.TakeContentsFrom(other_inst.m_data);
	m_single_parsing_logger = other_inst.m_single_parsing_logger;
}

// ---------------- Support routines ---------------------

void TMidLevRamLogger::ShowEventError(PrEventHeader &hdr, CtvEventType etp, const wchar_t *message)
{
	// Do not show the message if user has already cancelled.
	if (m_console->GetAbortFlag() == TRUE)
		return;

	// Prepare the message.
	wchar_t buffer[380+MAX_PATH];
	swprintf(buffer, 380+MAX_PATH,
			L"============ %s =============\r\n\r\n"
			L"Parsing id: \t%lu\r\n"
			L"Event id: \t%lu\r\n"
			L"Event type: \t%s\r\n"
			L"File name: \t%s\r\n"
			L"\r\nDo you want to continue?\r\n",
			message, ParsingId(), hdr.cn,
			CtvEventHeader::GetEventTypeName(etp),
			(hdr.src_area.file_info != NULL) ? hdr.src_area.file_info->info.file_name : L"n/a");

	// Show the message.
	if (m_console->ShowRunTimeError(buffer, MB_YESNO) == IDNO)
		m_console->SetAbortFlag(TRUE);
}

void TMidLevRamLogger::ShowEventError(CtvEventHeader &hdr, const wchar_t *message)
{
	// Do not show the message if the user has already cancelled.
	if (m_console->GetAbortFlag() == TRUE)
		return;

	// Prepare the message.
	wchar_t buffer[380+MAX_PATH];
	swprintf(buffer, 380+MAX_PATH,
			L"============ %s =============\r\n\r\n"
			L"Parsing id: \t%lu\r\n"
			L"Event id: \t%lu\r\n"
			L"Event type: \t%s\r\n"
			L"\r\nDo you want to continue?\r\n",
			message, hdr.parsing_id, hdr.event_id, hdr.GetEventTypeName());

	// Show the message.
	if (m_console->ShowRunTimeError(buffer, MB_YESNO) == IDNO)
		m_console->SetAbortFlag(TRUE);
}

//--------------------------------------------------------------------------------------
//  =====================  TMidLevRamStoreReader  ==========================
//--------------------------------------------------------------------------------------

TMidLevRamStoreReader::TMidLevRamStoreReader(bool allow_other_revs, bool recheck_cached_revs)
		: TMidLevStoreReader(allow_other_revs, recheck_cached_revs)
{
	m_data = NULL;
	m_iter_type = icat_none;
	m_curr_log = NULL;
}

void TMidLevRamStoreReader::SetupDataSource(TMidLevRamBasedDataStorage *data)
{
	// Ensure that iteration is not active and that data source is set into the root of the tree of stores.
	assert(m_iter_type == icat_none);
	assert(m_primary_store == NULL);

	m_data = data;
	m_curr_log = NULL;

	m_pStringsDict = &(data->m_strs_dict);
	m_files_cache.DeleteAll();
}

// Group 1a. Picking up top level objects.

bool TMidLevRamStoreReader::GetCompilationInfo(ID parsing_id, bool silent_mode)
{
	assert(IterationInactive() == TRUE);

	// Check, maybe the latest parsing is the one that is needed.
	if (m_curr_log != NULL && m_curr_log->m_session_header.parsing_id == parsing_id)
	{
		m_ci = m_curr_log->m_session_header;
		return(TRUE);
	}

	// Check, maybe the most recent parsing was requested.
	if (parsing_id == 0)
		return(GetLatestCompilationInfo(silent_mode));

	// Search for the requested parsing log.
	ID min_pid = 0, max_pid = 0;
	m_curr_log = m_data->FindParsingLog(parsing_id, &min_pid, &max_pid);
	if (m_curr_log != NULL)
	{
		m_ci = m_curr_log->m_session_header;
		return(TRUE);
	}

	// The requested parsing session is not available.
	if (silent_mode == FALSE)
	{
		wchar_t buffer[240], sub_buff[80];
		if (min_pid == 0 && max_pid == 0)
			wcscpy(sub_buff, L"The database does not contain parsing sessions.");
		else swprintf(sub_buff, 240, L"The range of available parsing ids spans from %lu to %lu.", min_pid, max_pid);

		swprintf(buffer, 240, L"Parsing log with id=%lu (0x%X) is missing in the parsing database.\r\n%s", parsing_id, parsing_id, sub_buff);
		ShowDataContentsError(buffer);
	}

	// Failure.
	return(FALSE);
}

bool TMidLevRamStoreReader::GetLatestCompilationInfo(bool silent_mode)
{
	assert(IterationInactive() == TRUE);

	m_curr_log = (TMidLevRamBasedParsingLog*)m_data->m_parsings.GetLast();
	if (m_curr_log == NULL)
	{
		// The data store is empty.
		if (silent_mode == FALSE)
			ShowDataContentsError(L"Unable to get the latest parsing log. The data source is empty.");

		return(FALSE);
	}

	m_ci = m_curr_log->m_session_header;
	return(TRUE);
}

bool TMidLevRamStoreReader::GetFileInformation(ID pars_id, ID file_id, bool want_body_and_linfo, bool *file_version_mismatch_flag, bool silent_mode)
{
	assert(IterationInactive() == TRUE);

	// Clear the output fields.
	m_fi.Clear();
	m_pfi = NULL;
	if (file_version_mismatch_flag != NULL)
		file_version_mismatch_flag = FALSE;

	if (pars_id == 0)
	{
		// This is a parsing indepenedent file. Info about these files is stored on the data storage level.
		TMidLevRamBasedDiskFileInfo *info = GetDiskFilePtr(file_id, silent_mode);
		if (info == NULL)
			return(FALSE);

		// Pick up the body of the file if it is requested.
		return(PrepareDiskFileInfo(info->data, want_body_and_linfo, file_version_mismatch_flag));
	}
	else
	{
		// This is a temp file.
		TMidLevRamLogTempFileRecord *info = GetTempFilePtr(pars_id, file_id, silent_mode);
		if (info == NULL)
			return(FALSE);

		// Temp file registration event was successfully retrieved.
		m_fi = info->temp_file_info;
		m_pfi = &(info->temp_file_info);
		if (want_body_and_linfo == FALSE)
		{
			// Kill the file body and lines info if any.
			m_fi.info.file_body = NULL;
			m_fi.lines_info = NULL;
			m_fi.lines_info_len = 0;
			m_fi.longest_line_len = 0;
			m_pfi = NULL;
		}

		return(TRUE);
	}
}

bool TMidLevRamStoreReader::CheckFileInfoPresence(ID pars_id, ID file_id)
{
	assert(IterationInactive() == TRUE);
	if (pars_id == 0)
	{
		// Check the array of the disk files.
		return(file_id > 0 && file_id < (DWORD)(m_data->m_disk_files.NumItems()) && m_data->m_disk_files[file_id].data.file_id == file_id);
	}
	else
	{
		if (m_curr_log ==  NULL || m_curr_log->m_session_header.parsing_id != pars_id)
		{
			if (GetCompilationInfo(pars_id, TRUE) == FALSE)
				return(FALSE);
		}

		// Parsing session with the passed parsing id is present in the database.
		if (file_id == 0 || file_id >= m_curr_log->NumEventPtrs())
			return(FALSE);

		// File id belongs to the range of events in the requested parsing session.
		CtvParsingEventInfo *pev = m_curr_log->m_event_ptrs[file_id];
		return(pev != NULL && pev->hdr.event_type == pset_register_temp_file);
	}
}

int TMidLevRamStoreReader::GetNumDiskFiles(bool want_files_array_len) const
{
	if (want_files_array_len == TRUE)
		return(m_data->m_disk_files.NumItems());
	else return(m_data->m_disk_files.NumItems()-TFileInfo::DiskFileIdsBase);
}

bool TMidLevRamStoreReader::GetIndexingKeyInfo(ID key_id)
{
	assert(IterationInactive() == TRUE);

	int inx = (int)key_id-TKeyInfo::IndexingKeyIdsBase;
	if (inx < 0 || inx >= m_data->m_indexing_keys.NumItems())
	{
		// The index is out of range.
		wchar_t buffer[240];
		swprintf(buffer, 240,
				L"Description of the indexing key with id=%lu is missing in the parsing database.\r\n"
				L"Indexing keys are available in the range of ids from %d to %d.",
				key_id, TKeyInfo::IndexingKeyIdsBase+1, TKeyInfo::IndexingKeyIdsBase+m_data->m_indexing_keys.NumItems()-1);

		ShowDataContentsError(buffer);
		return(FALSE);
	}

	// Move record from the data source to the data field of the base class.
	m_ki = m_data->m_indexing_keys[inx].data;
	return(TRUE);
}

bool TMidLevRamStoreReader::DeleteParsingSession(ID pars_id)
{
	assert(IterationInactive() == TRUE);

	if (GetCompilationInfo(pars_id) == FALSE)
		return(FALSE);

	m_data->DeleteParsingLog(m_curr_log);
	m_curr_log = NULL;
	return(TRUE);
}

// Group 1b. Iterating top level objects.

bool TMidLevRamStoreReader::StartFilesIteration(bool show_dfiles, bool show_tfiles, ID pars_id_restr, bool want_body_and_linfo, bool want_indexing_info)
{
	StartTopLevelIteration(icat_files);
	m_show_diskf_restr = show_dfiles;
	m_show_tempf_restr = show_tfiles;
	m_pars_id_restr = pars_id_restr;
	m_wbody_instr = want_body_and_linfo;
	m_wrinx_instr = want_indexing_info;
	return(TRUE);
}

bool TMidLevRamStoreReader::NextBatchId(ID &id)
{
	assert(IterationActive(icat_batch) == TRUE);

	ID prev_batch_id = (m_curr_log != NULL) ? m_curr_log->m_session_header.batch_id : 0;
	while (NextCompilationRecodInternal() == TRUE)
	{
		ID curr_batch_id = m_curr_log->m_session_header.batch_id;
		if (curr_batch_id == 0)
			continue;

		if (prev_batch_id == 0 || curr_batch_id != prev_batch_id)
		{
			id = curr_batch_id;
			return(TRUE);
		}
	}

	// End of the data set.
	return(FALSE);
}

bool TMidLevRamStoreReader::NextCompilationRecord()
{
	assert(IterationActive(icat_pars_sess) == TRUE);

	while (NextCompilationRecodInternal() == TRUE)
	{
		if (m_batch_restr == 0 || m_curr_log->m_session_header.batch_id == m_batch_restr)
		{
			m_ci = m_curr_log->m_session_header;
			return(TRUE);
		}
	}

	// End of the data set.
	return(FALSE);
}

bool TMidLevRamStoreReader::NextFileRecord()
{
	assert(IterationActive(icat_files) == TRUE);

	m_fi.Clear();
	m_pfi = NULL;

	TFileInfo *info;
	while (NextFileRecordInternal(&info) == TRUE)
	{
		if (m_wrinx_instr == TRUE && info->raw_indexing_date == 0)
		{
			// Raw indexing info is required and it is not available. Skip this file.
			continue;
		}

		if (info->pars_id == 0)
		{
			// This is a disk file.
			if (PrepareDiskFileInfo(*info, m_wbody_instr, NULL) == TRUE)
				return(TRUE);

			// Picking up the body of the file failed. Do not show this file to the upper layer.
			continue;
		}
		else
		{
			m_fi = *info;
			m_pfi = info;
			if (m_wbody_instr == FALSE)
			{
				// Kill the file body and lines info if any.
				m_fi.info.file_body = NULL;
				m_fi.lines_info = NULL;
				m_fi.lines_info_len = 0;
				m_fi.longest_line_len = 0;
				m_pfi = NULL;
			}

			return(TRUE);
		}
	}

	// End of the data set.
	return(FALSE);
}

bool TMidLevRamStoreReader::NextIndexingKeyRecord()
{
	assert(IterationActive(icat_keys) == TRUE);

	if (m_curr_inx >= m_data->m_indexing_keys.NumItems())
	{
		// End of the data set is reached.
		return(FALSE);
	}

	// Move record from the data source to the data field of the base class.
	m_ki = m_data->m_indexing_keys[m_curr_inx++].data;
	return(TRUE);
}

// Group 2b. Looking for parent events.

bool TMidLevRamStoreReader::GetSourceRefEvent(ID pars_id, long linear_offs)
{
	assert(IterationInactive() == TRUE);

	if (GetCompilationInfo(pars_id) == FALSE)
		return(FALSE);

	ID ref = m_curr_log->GetSourceRefFromLinearOffs(linear_offs);
	if (ref == 0)
		return(FALSE);

	return(GetParsingEventInternal(pars_id, ref, pset_lex_source_enter));
}

bool TMidLevRamStoreReader::GetCondRefEvent(ID pars_id, long linear_offs)
{
	assert(IterationInactive() == TRUE);

	if (GetCompilationInfo(pars_id) == FALSE)
		return(FALSE);

	ID ref = m_curr_log->GetCondRefFromLinearOffs(linear_offs);
	if (ref == 0)
	{
		PrepareEmptyCondParsChangeEvent(pars_id);
		return(TRUE);
	}

	return(GetParsingEventInternal(pars_id, ref, pset_cond_parsing_change));
}

// Group 2c. Events misc.

bool TMidLevRamStoreReader::GetParsingEventsArray(CtvParsingEventPtrsArray &buffer, ID parsing_id, PSET_TYPES_MASK evts_mask, ID min_evid, ID max_evid)
{
	// Clear the input buffer.
	buffer.SetNumItems(0);

	TMidLevRamBasedParsingLog *current_log = NULL;
	if (parsing_id == TProjectParsSessId)
	{
		// Caller wants to iterate all parsing sessions.
		assert(IterationInactive() == TRUE);
		current_log = (TMidLevRamBasedParsingLog*)(m_data->m_parsings.GetFirst());
		if (current_log != NULL)
		{
			// Setup the discovered log as current.
			m_curr_log = current_log;
			m_ci = m_curr_log->m_session_header;
			parsing_id = m_ci.parsing_id;
			assert(parsing_id != TProjectParsSessId);
		}
	}

	bool res = TRUE;
	for(;;)
	{
		// Start iteration with the requested params.
		if (StartEventsIteration(parsing_id, evts_mask, 0, min_evid, max_evid, pset_none) == FALSE)
			return(FALSE);

		// Iterate events and give out the pointers to them.
		int inx = m_curr_inx;
		while (inx <= (int)m_evid_restr)
		{
			CtvParsingEventInfo *pev = m_curr_log->m_event_ptrs[inx++];

			// Check the event existence restriction.
			if (pev == NULL)
				continue;

			// Check the category restriction.
			if (m_evt_types_mask != PSET_ALL_TYPES && (m_evt_types_mask & PSET_TYPE_BIT(pev->hdr.event_type)) == 0)
				continue;

			// Give out the pointer.
			if (buffer.AppendItem(pev) == FALSE)
			{
				ShowDataContentsError(L"Error assembling CtvParsingEventPtrsArray (low on memory).");
				res = FALSE;
				break;
			}
		}

		// Close iteration.
		if (CloseParsingEventsIteration() == FALSE)
			return(FALSE);

		if (current_log != NULL)
		{
			// Do the outer loop step.
			current_log = (TMidLevRamBasedParsingLog*)(m_data->m_parsings.GetNext(current_log));
			if (current_log != NULL)
			{
				// Setup the discovered log as current.
				m_curr_log = current_log;
				m_ci = m_curr_log->m_session_header;
				parsing_id = m_ci.parsing_id;
				assert(parsing_id != TProjectParsSessId);
				continue;
			}
		}

		// A single session was requested or this is an end of the list of parsing sessions.
		break;
	}

	// Give out the overall result.
	return(res);
}

// Group 3a. Iterating parsing events of some compilation.

bool TMidLevRamStoreReader::StartCondParsChgCondRefsIteration(ID parsing_id, ID cond_ref, ID min_evid, ID max_evid, bool ascending_iter_order)
{
	if (StartEventsIteration(parsing_id, PSET_ALL_TYPES, 0, min_evid, max_evid, pset_cond_parsing_change) == FALSE)
		return(FALSE);

	// Assign the rest of the iteration restrictions. Note that cond ref equal to zero is a valid
	// restriction value. This method does not allow iteration without the cond ref restriction.
	m_cond_ref_restr = cond_ref;
	CtvParsingEventInfo *pev = NULL;
	if (cond_ref != 0)
	{
		if (cond_ref >= m_curr_log->NumEventPtrs())
		{
			wchar_t buffer[240];
			swprintf(buffer, 240,
					L"StartCondParsChgCondRefsIteration(ParsingId=%lu):\r\n\r\n"
					L"Passed cond ref (%lu) exceeds the number of events in the log (%d).",
					parsing_id, cond_ref, m_curr_log->NumEventPtrs());

			ShowDataContentsError(buffer);
			CloseIteration();
			return(FALSE);
		}

		// Ensure that cond pars event with the passed id is existing.
		pev = m_curr_log->m_event_ptrs[cond_ref];
		if (pev == NULL || pev->hdr.event_type != pset_cond_parsing_change || pev->cond_pars.stmt_type == mpr_endif)
		{
			wchar_t buffer[240];
			swprintf(buffer, 240,
					L"StartCondParsChgCondRefsIteration(ParsingId=%lu):\r\n\r\n"
					L"Event with id (%lu) is missing or it is not a valid cond parsing change event.",
					parsing_id, cond_ref);

			ShowDataContentsError(buffer);
			CloseIteration();
			return(FALSE);
		}

		// Cond ref event is available. Check the lower range of its child events.
		if (m_curr_inx <= (int)cond_ref)
		{
			m_curr_inx = cond_ref+1;
		}

		// Check the higher range of its child events.
		if (pev->cond_pars.next_stmt_id != 0 && m_evid_restr >= pev->cond_pars.next_stmt_id)
		{
			m_evid_restr = pev->cond_pars.next_stmt_id-1;
		}
	}
	else
	{
		// Caller asked to iterate the root area. It is covering all events. Set the min event id to 1 if
		// it is not set. Note that the max event id is already verified in the StartEventsIteration() method.
		if (m_curr_inx == 0)
			m_curr_inx = 1;
	}

	// Save iteration direction and check iteration boundaries once again.
	m_evts_ascending_order = ascending_iter_order;
	if (ascending_iter_order == TRUE)
	{
		if (m_curr_inx > (int)cond_ref+1)
		{
			if ((ID)m_curr_inx >= m_curr_log->NumEventPtrs())
				m_curr_inx = m_curr_log->NumEventPtrs()-1;

			// There is a gap between the first event that is known to have the right cond ref and the min evid restr.
			int inx_test = cond_ref+1;
			while (inx_test < m_curr_inx)
			{
				// Pick up the next event and increment the index.
				CtvParsingEventInfo *pev_test = m_curr_log->m_event_ptrs[inx_test++];
				if (pev_test == NULL)
					continue;

				// Check for events that may change the current source_ref.
				if (pev_test->hdr.event_type == pset_cond_parsing_change && pev_test->cond_pars.stmt_type != mpr_endif)
				{
					if (pev_test->cond_pars.next_stmt_id != 0)
						inx_test = pev_test->cond_pars.next_stmt_id;
					else inx_test = m_curr_log->NumEventPtrs();
				}
			}

			// Events with the index inx_test points into the area with the right cond ref.
			m_curr_inx = inx_test;
		}
	}
	else
	{
		// Swap the boundaries because iteration will go in the descending order.
		ID idTemp = m_curr_inx;
		m_curr_inx = m_evid_restr;
		m_evid_restr = idTemp;

		if (pev != NULL && pev->cond_pars.next_stmt_id != 0)
		{
			// The cond parsing area is properly closed. Situation is more simple.
			if (m_curr_inx < (int)pev->cond_pars.next_stmt_id-1)
			{
				// There is a gap between the first event with the right cond ref and the min evid restr.
				int inx_test = pev->cond_pars.next_stmt_id-1;
				while (inx_test > m_curr_inx)
				{
					// Pick up the next event and increment the index.
					CtvParsingEventInfo *pev_test = m_curr_log->m_event_ptrs[inx_test--];
					if (pev_test == NULL)
						continue;

					// Check for events that may change the current cond_ref.
					if (pev_test->hdr.event_type == pset_cond_parsing_change)
					{
						TPreprocToken stmt = pev_test->cond_pars.stmt_type;
						if (stmt == mpr_elif || stmt == mpr_else || stmt == mpr_endif)
						{
							assert(pev_test->cond_pars.prev_stmt_id != 0);
							inx_test = pev_test->cond_pars.prev_stmt_id;
						}
					}
				}

				// Events with the index inx_test points into the area with the right cond ref.
				m_curr_inx = inx_test;
			}
		}
		else
		{
			// Caller wants to iterate objects in the root area or the requested area is not closed. Position of the end
			// of the area is not clear. Iterate all evens from the very beg of the area in the forward direction.
			// This basically means that events will be iterated twice, but this seems to be the simpliest code.
			int inx_last_good = cond_ref+1;

			int inx_test = cond_ref+1;
			while (inx_test <= m_curr_inx)
			{
				// Pick up the next event and increment the index.
				CtvParsingEventInfo *pev_test = m_curr_log->m_event_ptrs[inx_test++];
				if (pev_test == NULL)
					continue;

				// Current event is existing and it belongs to the right cond ref.
				inx_last_good = inx_test;

				// Check for events that may change the current source_ref.
				if (pev_test->hdr.event_type == pset_cond_parsing_change && pev_test->cond_pars.stmt_type != mpr_endif)
				{
					if (pev_test->cond_pars.next_stmt_id != 0)
						inx_test = pev_test->cond_pars.next_stmt_id;
					else inx_test = m_curr_log->NumEventPtrs();
				}
			}

			// If event with inx_test is existing, it should have the correct cond_ref.
			m_curr_inx = inx_last_good;
		}
	}

	return(TRUE);
}

// Group 3b. Iterating file/pars bkrefs and raw indexing key uses.

bool TMidLevRamStoreReader::StartFileParsRefsIteration(ID pars_id, ID file_id)
{
	assert(IterationInactive() == TRUE);

	if (pars_id == 0)
	{
		// This is parsing indepenedent disk file.
		TMidLevRamBasedDiskFileInfo *info = GetDiskFilePtr(file_id, FALSE);
		if (info == NULL)
			return(FALSE);

		m_citer_pars_bkrefs = &(info->pars_refs);
	}
	else
	{
		// This is a temp file.
		TMidLevRamLogTempFileRecord *info = GetTempFilePtr(pars_id, file_id, FALSE);
		if (info == NULL)
			return(FALSE);

		m_citer_pars_bkrefs = &m_dummy_pars_bkref;
		m_dummy_pars_bkref[0] = pars_id;
	}

	StartTopLevelIteration(icat_file_pars);
	return(TRUE);
}

bool TMidLevRamStoreReader::StartFileIndexingSessionsIteration(ID pars_id, ID file_id, ID key_id)
{
	assert(IterationInactive() == TRUE);

	if (pars_id == 0)
	{
		// This is a disk file.
		TMidLevRamBasedDiskFileInfo *info = GetDiskFilePtr(file_id, FALSE);
		if (info == NULL)
			return(FALSE);

		m_raw_data_reader.Setup(info->indexing_data, key_id);
	}
	else
	{
		// This is a temp file.
		TMidLevRamLogTempFileRecord *info = GetTempFilePtr(pars_id, file_id, FALSE);
		if (info == NULL)
			return(FALSE);

		m_raw_data_reader.Setup(info->indexing_data, key_id);
	}

	StartTopLevelIteration(icat_file_key);
	return(TRUE);
}

bool TMidLevRamStoreReader::NextFileParsRefsParsingId(ID &id)
{
	assert(IterationActive(icat_file_pars) == TRUE);

	if (m_curr_inx >= m_citer_pars_bkrefs->NumItems())
	{
		// End of the data set is reached.
		return(FALSE);
	}

	// Give out the next id.
	id = (*m_citer_pars_bkrefs)[m_curr_inx++];
	return(TRUE);
}

bool TMidLevRamStoreReader::NextKeyFileRefRecord()
{
	assert(IterationActive(icat_key_file) == TRUE);

	CtvKeyFileRefsArray &refs = m_citer_indexing_key->file_refs;
	if (m_curr_inx >= refs.NumItems())
	{
		// End of the data set is reached.
		return(FALSE);
	}

	// Give out the next bkref.
	m_kf_ref = refs[m_curr_inx++];
	return(TRUE);
}

bool TMidLevRamStoreReader::NextKeyParsRefRecord()
{
	assert(IterationActive(icat_key_pars) == TRUE);

	CtvKeyParsRefsArray &refs = m_citer_indexing_key->pars_refs;
	if (m_curr_inx >= refs.NumItems())
	{
		// End of the data set is reached.
		return(FALSE);
	}

	// Give out the next bkref.
	m_kp_ref = refs[m_curr_inx++];
	return(TRUE);
}

// Group 4. Support methods.

TMidLevStoreReader *TMidLevRamStoreReader::AllocateNewSecondaryStore()
{
	// Allocate new database reader.
	TMidLevRamStoreReader *qp = new TMidLevRamStoreReader(m_allow_other_revisions, m_recheck_cached_files);
	if (qp == NULL)
		return(NULL);

	// Setup and return the object.
	qp->SetupDataSource(m_data);
	return(qp);
}

//  - - - - - -  Prorected methods  - - - - - - -

bool  TMidLevRamStoreReader::GetSourceLocationInfoInternal(TSourceLocation &buffer, ID parsing_id, TLinearArea &lin_area, long max_linear_offs)
{
	// This method is protected. It should be called only with correct parsing ids.
	bool res = GetCompilationInfo(parsing_id);
	assert(res == TRUE);

	// The requested parsing log object is available.
	return(m_curr_log->GetSrcLocInfo(this, buffer, lin_area, max_linear_offs));
}

TMidLevRamBasedDiskFileInfo *TMidLevRamStoreReader::GetDiskFilePtr(ID file_id, bool silent_mode)
{
	int inx = (int)file_id;
	if (inx <= 0 || inx >= m_data->m_disk_files.NumItems())
	{
		// Passed file_id is out of range. Check if disk files table contains registered files or not.
		if (silent_mode == FALSE)
		{
			wchar_t buffer[240];
			if (m_data->m_disk_files.NumItems() > TFileInfo::DiskFileIdsBase)
			{
				// Registered disk files are present.
				swprintf(buffer, 240,
						L"Description of the disk file with id \"%lu\" (0x%X) is missing in the parsing database.\r\n"
						L"Disk files are available in the range of ids from %d to %d.\r\n"
						L"(Note that lower side of the range is not covering the built in files.)",
						file_id, file_id, TFileInfo::DiskFileIdsBase, m_data->m_disk_files.NumRealItems());
			}
			else
			{
				// Table contains records only for the built in files.
				swprintf(buffer, 240,
						L"Database has no disk file descriptions.\r\nFile with id %lu cannot be there.",
						file_id);
			}

			ShowDataContentsError(buffer);
		}

		return(NULL);
	}

	if (m_data->m_disk_files[inx].data.file_id != file_id)
	{
		if (silent_mode == FALSE)
		{
			wchar_t buffer[240];
			TMidLevRamBasedDiskFileInfo &slot_data = m_data->m_disk_files[inx];
			if (file_id < TFileInfo::DiskFileIdsBase && slot_data.data.file_id == 0)
			{
				swprintf(buffer, 240, L"Database does not contain built in file with an id %lu.", file_id);
			}
			else
			{
				swprintf(buffer, 240, L"The database is corrupted.\r\nSlot for the file with an id %lu is occupied with a file that has an id %lu.",
						file_id, slot_data.data.file_id);
			}

			ShowDataContentsError(buffer);
		}

		return(NULL);
	}

	// File record is available.
	return(&(m_data->m_disk_files[inx]));
}

TMidLevRamLogTempFileRecord *TMidLevRamStoreReader::GetTempFilePtr(ID pars_id, ID file_id, bool silent_mode)
{
	// In fact, the data storage stores the storage specific derived data. Retrieve this data.
	return((TMidLevRamLogTempFileRecord*)GetParsingEventPointer(pars_id, file_id, pset_register_temp_file, TRUE, silent_mode));
}

bool TMidLevRamStoreReader::PrepareDiskFileInfo(TFileInfo &info, bool want_body_and_linfo, bool *file_version_mismatch_flag)
{
	if (want_body_and_linfo == TRUE)
	{
		// Check/update the cache of the base object.
		bool cached_item_present;
		TFileInfo *cached_fi = LookupCachedFile(info.pars_id, info.file_id, file_version_mismatch_flag, cached_item_present);
		if (cached_item_present == FALSE)
			cached_fi = CacheDiskFile(info, file_version_mismatch_flag);

		if (cached_fi == NULL)
			return(FALSE);

		// Loading body of the file and its caching succeeded.
		m_fi = *cached_fi;
		m_pfi = cached_fi;
	}
	else
	{
		// Copy the file descriptor itself. The pointer should remain NULL.
		m_fi = info;
		assert(m_pfi == NULL);

		if (file_version_mismatch_flag != NULL)
			file_version_mismatch_flag = FALSE;

		// Kill the file body and the lines info if any.
		m_fi.info.file_body = NULL;
		m_fi.lines_info = NULL;
		m_fi.lines_info_len = 0;
		m_fi.longest_line_len = 0;
	}

	// Success.
	return(TRUE);
}

CtvParsingEventInfo *TMidLevRamStoreReader::GetParsingEventPointer(ID parsing_id, ID event_id, CtvEventType expected_evt, bool want_full_check, bool silent_mode)
{
	// If current parsing session is not the requested one, find it in the list of parsing sessions.
	if (m_curr_log ==  NULL || m_curr_log->m_session_header.parsing_id != parsing_id)
	{
		if (GetCompilationInfo(parsing_id, silent_mode) == FALSE)
			return(NULL);
	}

	if (event_id == 0 || event_id >= m_curr_log->NumEventPtrs())
	{
		// The passed event id is bogus.
		if (silent_mode == FALSE)
		{
			wchar_t buffer[240];
			swprintf(buffer, 240,
					L"GetParsingEvent:\r\n\r\n"
					L"Event with the parsing_id=%lu, event_id=%lu (0x%X) is missing in the database.\r\n\r\n"
					L"    Requested event type: \t%s.\r\n    Current events log length: \t%lu.",
					parsing_id, event_id, event_id, (expected_evt == pset_none) ? L"AnyType": CtvEventHeader::GetEventTypeName(expected_evt),
					m_curr_log->NumEventPtrs());

			ShowDataContentsError(buffer);
		}

		return(NULL);
	}

	if (want_full_check == FALSE)
	{
		// Caller asks for light verification.
		return(m_curr_log->m_event_ptrs[event_id]);
	}

	if (m_curr_log->m_event_ptrs[event_id] == NULL)
	{
		// The event id is more or less ok but the requested event is missing.
		if (silent_mode == FALSE)
		{
			wchar_t buffer[240];
			swprintf(buffer, 240,
					L"GetParsingEvent:\r\n\r\n"
					L"Event with the parsing_id=%lu, event_id=%lu (0x%X) is missing in the database.\r\n\r\n"
					L"    Requested event type: \t%s.",
					parsing_id, event_id, event_id, (expected_evt == pset_none) ? L"AnyType": CtvEventHeader::GetEventTypeName(expected_evt));

			ShowDataContentsError(buffer);
		}

		return(NULL);
	}

	CtvParsingEventInfo *pev = m_curr_log->m_event_ptrs[event_id];
	if (expected_evt != pset_none && pev->hdr.event_type != expected_evt)
	{
		// Wrong event type.
		if (silent_mode == FALSE)
		{
			wchar_t buffer[240];
			swprintf(buffer, 240,
					L"GetParsingEvent:\r\n\r\n"
					L"Event with the parsing_id=%lu, event_id=%lu (0x%X) has unexpected type.\r\n\r\n"
					L"    Type of existing event: \t   %s\r\n    Requested event type: \t   %s",
					parsing_id, event_id, event_id, pev->hdr.GetEventTypeName(), CtvEventHeader::GetEventTypeName(expected_evt));

			ShowDataContentsError(buffer);
		}
		return(NULL);
	}

	// Requested event is available.
	return(pev);
}

bool TMidLevRamStoreReader::GetParsingEventInternal(ID parsing_id, ID event_id, CtvEventType expected_evt)
{
	CtvParsingEventInfo *pev = GetParsingEventPointer(parsing_id, event_id, expected_evt, TRUE, FALSE);
	if (pev ==  NULL)
	{
		// The error is already displayed.
		return(FALSE);
	}

	// Move body of the event from the data source into the data field of the base class.
	memcpy(&m_event, pev, pev->GetSize());
	return(TRUE);
}

void TMidLevRamStoreReader::StartTopLevelIteration(TIterCategory icat)
{
	assert(IterationInactive() == TRUE);

	m_iter_type = icat;
	m_curr_log = NULL;
	m_parsing_logs_eof = FALSE;
	m_curr_inx = 1;
}

bool TMidLevRamStoreReader::StartEventsIteration(ID pid, PSET_TYPES_MASK evts_mask, ID src_ref, ID min_evid, ID max_evid, CtvEventType events_category)
{
	assert(IterationInactive() == TRUE);

	if (GetCompilationInfo(pid) == FALSE)
		return(FALSE);

	m_iter_type = EvtIterCat(events_category);
	m_curr_inx = min_evid;

	m_evt_types_mask = evts_mask;
	m_src_ref_restr = src_ref;
	m_cond_ref_restr = 0;
	m_evid_restr = max_evid;
	m_evts_ascending_order = TRUE;

	if (src_ref != 0)
	{
		// Ensure that call event with the passed event id is existing.
		if (src_ref >= m_curr_log->NumEventPtrs())
		{
			wchar_t buffer[240];
			swprintf(buffer, 240,
					L"StartEventsIteration:\r\n\r\n"
					L"Passed source ref (%lu) exceeds the number of events in the parsing log (%d).",
					src_ref, m_curr_log->NumEventPtrs());

			ShowDataContentsError(buffer);
			CloseIteration();
			return(FALSE);
		}

		CtvParsingEventInfo *pev = m_curr_log->m_event_ptrs[src_ref];
		if (pev == NULL || pev->hdr.event_type != pset_lex_source_enter)
		{
			wchar_t buffer[240];
			swprintf(buffer, 240,
						L"StartEventsIteration:\r\n\r\n"
						L"Event with id (%lu) is missing or it is not a lex source enter event.",
						src_ref);

			ShowDataContentsError(buffer);
			CloseIteration();
			return(FALSE);
		}

		// Call event is available. Check the range of its child events.
		if (m_curr_inx <= (int)src_ref+1)
		{
			// First event after the lex src enter always has the correct source_ref.
			m_curr_inx = src_ref+1;
		}
		else
		{
			if ((ID)m_curr_inx >= m_curr_log->NumEventPtrs())
				m_curr_inx = m_curr_log->NumEventPtrs()-1;

			// There is a gap between the first event with the right source ref and the min evid restr.
			int inx_test = src_ref+1;
			while (inx_test < m_curr_inx)
			{
				// Pick up the event and increment the index.
				CtvParsingEventInfo *pev_in_gap = m_curr_log->m_event_ptrs[inx_test++];
				if (pev_in_gap == NULL)
					continue;

				// Check for events that may change the current source_ref.
				if (pev_in_gap->hdr.event_type == pset_lex_source_enter)
				{
					if (pev_in_gap->src_enter.evt_exit_id != 0)
						inx_test = pev_in_gap->src_enter.evt_exit_id+1;
					else inx_test = m_curr_log->NumEventPtrs();
				}
			}

			// If event with the inx_test is existing, it should have the correct source ref.
			m_curr_inx = inx_test;
		}

		if (pev->src_enter.evt_exit_id != 0 && (m_evid_restr == 0 || m_evid_restr > pev->src_enter.evt_exit_id))
			m_evid_restr = pev->src_enter.evt_exit_id;
	}

	// Ensure that the evid restriction can be used as a loop boundary.
	if (m_evid_restr == 0 || m_evid_restr >= m_curr_log->NumEventPtrs())
		m_evid_restr = m_curr_log->NumEventPtrs()-1;

	return(TRUE);
}

bool TMidLevRamStoreReader::StartKeyRefsIteration(ID key_id, TIterCategory iter_type)
{
	assert(FALSE);
	return(FALSE);
}

bool TMidLevRamStoreReader::NextCompilationRecodInternal()
{
	if (m_parsing_logs_eof == TRUE)
	{
		// End of the data set was already reached.
		return(FALSE);
	}

	if (m_curr_log == NULL)
		m_curr_log = (TMidLevRamBasedParsingLog*)m_data->m_parsings.GetFirst();
	else m_curr_log = (TMidLevRamBasedParsingLog*)m_data->m_parsings.GetNext(m_curr_log);

	if (m_curr_log == NULL)
	{
		// End of the data set was reached right now.
		m_parsing_logs_eof = TRUE;
		return(FALSE);
	}

	// Some non NULL parsing is still present.
	return(TRUE);
}

bool TMidLevRamStoreReader::NextFileRecordInternal(TFileInfo **ppInfo)
{
	if (m_parsing_logs_eof == TRUE)
	{
		// Iteration already terminated.
		*ppInfo = NULL;
		return(FALSE);
	}

	//
	// Switch between possible stages of iteration.
	//

	if (m_show_diskf_restr == TRUE)
	{
		// Disk files phase was requested and it has not finished yet.
		while (m_curr_inx < m_data->m_disk_files.NumItems())
		{
			TMidLevRamBasedDiskFileInfo &info = m_data->m_disk_files[m_curr_inx++];
			if (info.data.file_id == 0)
			{
				// This should be some built in file index.
				continue;
			}

			if (m_pars_id_restr != 0 && info.CheckParsingId(m_pars_id_restr) == FALSE)
			{
				continue;
			}

			// Give out the pointer to this file.
			*ppInfo = &(info.data);
			return(TRUE);
		}

		// Disk files phase ended.
		m_show_diskf_restr = FALSE;
	}

	if (m_show_tempf_restr == TRUE)
	{
		if (m_curr_log == NULL)
		{
			if (m_pars_id_restr != 0)
			{
				// Look only in one parsing session.
				m_curr_log = m_data->FindParsingLog(m_pars_id_restr);
				m_curr_inx = 1;
			}
			else
			{
				// Process all available sessions.
				NextCompilationRecodInternal();
			}
		}

		while (m_curr_log != NULL)
		{
			while ((ID)m_curr_inx < m_curr_log->NumEventPtrs())
			{
				CtvParsingEventInfo *pev = m_curr_log->m_event_ptrs[m_curr_inx++];
				if (pev == NULL || pev->hdr.event_type != pset_register_temp_file)
				{
					continue;
				}

				// Give out the pointer to this file.
				*ppInfo = &(((TMidLevRamLogTempFileRecord*)pev)->temp_file_info);
				return(TRUE);
			}

			// The current log ended.
			if (m_pars_id_restr == 0)
			{
				// Try to proceed to the next one.
				NextCompilationRecodInternal();
			}
			else
			{
				// Only one parsing log should be processed.
				break;
			}
		}

		// Temp files phase ended.
		m_show_tempf_restr = FALSE;
	}

	// End of the whole data set.
	m_parsing_logs_eof = TRUE;
	*ppInfo = NULL;
	return(FALSE);
}

bool TMidLevRamStoreReader::NextParsingEventInternal1(CtvEventType event_category)
{
	assert(IterationActive(EvtIterCat(event_category)) == TRUE);

	while (m_curr_inx <= (int)m_evid_restr)
	{
		// Pick up the next event and increment the index.
		CtvParsingEventInfo *pev = m_curr_log->m_event_ptrs[m_curr_inx++];

		// Check the existence restriction.
		if (pev == NULL)
			continue;

		// Check the source_ref restriction if this type of restr is present.
		if (m_src_ref_restr != 0)
		{
			// Iteration setup code guarantees that current event matches the source_ref restriction.
			// Check for events that may change the current source_ref.
			if (pev->hdr.event_type == pset_lex_source_enter)
			{
				if (pev->src_enter.evt_exit_id != 0)
					m_curr_inx = pev->src_enter.evt_exit_id+1;
				else m_curr_inx = m_curr_log->NumEventPtrs();
			}
		}

		// Check the category restriction.
		if (event_category == pset_none)
		{
			if (m_evt_types_mask != PSET_ALL_TYPES && (m_evt_types_mask & PSET_TYPE_BIT(pev->hdr.event_type)) == 0)
				continue;
		}
		else
		{
			if (pev->hdr.event_type != event_category)
				continue;
		}

		// Move data from the data source into the data field of the base class.
		memcpy(&m_event, pev, pev->GetSize());
		return(TRUE);
	}

	// End of the data set.
	m_curr_inx = m_curr_log->NumEventPtrs();
	return(FALSE);
}

bool TMidLevRamStoreReader::NextParsingEventInternal2(CtvEventType event_category)
{
	assert(IterationActive(EvtIterCat(event_category)) == TRUE);

	// This iteration can go in both directions. It checks the cond_ref restriction if it is present.
	if (m_evts_ascending_order == TRUE)
	{
		// Iterate starting from small evids and progress to big evids.
		while (m_curr_inx <= (int)m_evid_restr)
		{
			// Pick up the next event and increment the index.
			CtvParsingEventInfo *pev = m_curr_log->m_event_ptrs[m_curr_inx++];

			// Check the existence restriction.
			if (pev == NULL)
				continue;

			// Iteration setup code guarantees that current event matches the cond_ref restriction.
			// Check for events that may change the current cond_ref.
			if (pev->hdr.event_type == pset_cond_parsing_change && pev->cond_pars.stmt_type != mpr_endif)
			{
				if (pev->cond_pars.next_stmt_id != 0)
					m_curr_inx = pev->cond_pars.next_stmt_id;
				else m_curr_inx = m_curr_log->NumEventPtrs();
			}

			// Check the category restriction.
			if (event_category != pset_none && pev->hdr.event_type != event_category)
				continue;

			// Move data from the data source into the data field of the base class.
			memcpy(&m_event, pev, pev->GetSize());
			assert(m_event.hdr.event_type != pset_cond_parsing_change || m_event.cond_pars.cond_ref == m_cond_ref_restr);
			return(TRUE);
		}
	}
	else
	{
		// Iterate starting from big evids to small evids.
		while (m_curr_inx >= (int)m_evid_restr)
		{
			// Pick up event and decrement the index.
			CtvParsingEventInfo *pev = m_curr_log->m_event_ptrs[m_curr_inx--];

			// Check the existence restriction.
			if (pev == NULL)
				continue;

			// Iteration setup code guarantees that current event matches the cond_ref restriction.
			// Check for events that may change the current cond_ref.
			if (pev->hdr.event_type == pset_cond_parsing_change)
			{
				TPreprocToken stmt = pev->cond_pars.stmt_type;
				if (stmt == mpr_elif || stmt == mpr_else || stmt == mpr_endif)
				{
					assert(pev->cond_pars.prev_stmt_id != 0);
					m_curr_inx = pev->cond_pars.prev_stmt_id;
				}
			}

			// Check the category restriction.
			if (event_category != pset_none && pev->hdr.event_type != event_category)
				continue;

			// Move data from the data source into the data field of the base class.
			memcpy(&m_event, pev, pev->GetSize());
			assert(m_event.hdr.event_type != pset_cond_parsing_change || m_event.cond_pars.cond_ref == m_cond_ref_restr);
			return(TRUE);
		}
	}

	// This is an end of the data set.
	if (m_evts_ascending_order == TRUE)
		m_curr_inx = m_curr_log->NumEventPtrs();
	else m_curr_inx = -1;

	return(FALSE);
}

bool TMidLevRamStoreReader::CloseIteration(TIterCategory category)
{
	if (category != icat_none)
	{
		// Check that application is closing right iteration type.
		assert(m_iter_type == category);
	}

	m_iter_type = icat_none;
	return(TRUE);
}


