//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   The C/C++ macroprocessor level.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/FormatNumber.H"
#include  "TextScan/MidScan.H"

//------------------------------------------------------------------------

static const wchar_t *g_CondStatusName[cexr_num_types] =
{
	L"FalseSure",			L"TrueSure",
	L"FalseDoubt",			L"TrueDoubt",
	L"Unknown",
};

const wchar_t *g_ParsingPhaseNames[mlph_num_items] =
{
	L"None",
	L"RawIndexing",		L"FileReg",
	L"Preparation",		L"Scanning",
	L"MajorSrcEof",
};

//------------------------------------------------------------------------
//  ====================  TFileInfoItem  ========================
//------------------------------------------------------------------------

TFileInfoItem::~TFileInfoItem()
{
	data.ReleaseLinesInfo();
	if (interm_file_body == NULL)
	{
		// Basic file info owns the file name and the file body allocations.
		data.ReleaseBasicFileInfo();
	}
	else
	{
		// This should be an intermediate file.
		assert(data.strg_type == strg_interm);
		delete interm_file_body;
	}
}

//-------------------------------------------------------------------------
//  ===================  TIntermFileBody  ========================
//-------------------------------------------------------------------------

TIntermFileBody::TIntermFileBody(TFileInfoItem *owner, PrEventHeader *creation_ctx)
{
	// Ensure that the owner has a proper storage type.
	assert(owner != NULL);
	assert(owner->data.strg_type == strg_interm);

	// Establish the link between these two objects.
	fd = owner;
	fd->interm_file_body = this;
	fd->data.info.file_name = fname;
	fd->data.info.file_body = data.DataPtr();

	swprintf(fname, 24, L"@xsf-%lu-nofid", fd->data.pars_id);
	pure_area_beg = -1;
	line_beg_stt = TRUE;
	wrt_complete = rdn_complete = FALSE;

	event_hdr = *creation_ctx;
	event_hdr.cn = 0;
	latest_pbuff_exit = 0;
	saved_body_len = -1;
}

void TIntermFileBody::RefreshFileIdent()
{
	swprintf(fname, 24, L"@xsf-%lu-%lu", fd->data.pars_id, fd->data.file_id);
	event_hdr.cn = fd->data.file_id;
}

bool TIntermFileBody::AddToBufferInternal(wchar_t *data_ptr, bool *data_esc_ptr, long data_offs, long data_len, bool orig_data)
{
	assert(pure_area_beg < 0);

	if (data.ReserveSpace(data_len) == FALSE || escaped_chr.ReserveSpace(data_len) == FALSE)
	{
		// Space allocation failed.
		return(FALSE);
	}

	//
	// Subsequent actions cannot fail.
	//

	if (orig_data == TRUE)
		pure_area_beg = data.NumItems();
	data.AppendItems(data_ptr+data_offs, data_len);

	for (int ii=0; ii<data_len; ii++)
	{
		bool val = (data_esc_ptr != NULL) ? data_esc_ptr[data_offs+ii] : FALSE;
		escaped_chr.AppendItem(val);
	}

	// Success.
	UpdateFileBody();
	return(TRUE);
}

bool TIntermFileBody::EscapeQuotes(long off_beg, long area_len)
{
	if (area_len <= 0)
	{
		// Empty areas do not need any escaping.
		return(TRUE);
	}

	// Count the number of chars that should be escaped.
	int cnt = 0;
	for (int inx1=off_beg; inx1<off_beg+area_len; ++inx1)
	{
		if ((data[inx1] == L'\'' || data[inx1] == L'\"') && escaped_chr[inx1] == FALSE)
			cnt++;
	}

	if (cnt == 0)
	{
		// It turned out that there is nothing to escape.
		return(TRUE);
	}

	// Reserve the space for all chars that need escaping at once.
	if (data.ReserveSpace(cnt) == FALSE || escaped_chr.ReserveSpace(cnt) == FALSE)
	{
		// Space allocation failed.
		return(FALSE);
	}

	// Loop over the area again and do the actual escaping.
	wchar_t esc_char = L'\\';
	bool    chr_escaped = TRUE;
	for (int inx2=off_beg; inx2<off_beg+area_len; ++inx2)
	{
		if (data[inx2] != L'\'' && data[inx2] != L'\"' || escaped_chr[inx2] == TRUE)
			continue;

		// Insert the new items and update the related vars.
		data.InsertItem(inx2, esc_char);
		escaped_chr.InsertItem(inx2+1, chr_escaped);
		inx2++; area_len++;
	}

	return(TRUE);
}

void TIntermFileBody::Truncate(long new_data_len, bool set_rdn_complete)
{
	assert(new_data_len >= pure_area_beg);
	if (set_rdn_complete == TRUE)
		rdn_complete = TRUE;

	// Save the current state for possible future restoring.
	saved_body_len = data.NumItems();

	// Truncate the body.
	if (new_data_len >= 0)
	{
		assert(new_data_len <= data.NumItems());
		data.SetNumItems(new_data_len);
		UpdateFileBody();
	}
}

void TIntermFileBody::RevertLatestTruncation()
{
	if (saved_body_len < 0)
		return;

	// The truncation to revert is available.
	data.SetNumItems(saved_body_len);
	UpdateFileBody();
}

void TIntermFileBody::UpdateFileBody()
{
	fd->data.info.file_body = data.DataPtr();
	fd->data.info.file_len = data.NumItems();
	fd->data.UpdateCrc();
}

//-------------------------------------------------------------------------
//	===============  PrMiscPreprocStmtEvent  =====================
//-------------------------------------------------------------------------

static const wchar_t *g_MiscEventExEnumNames[exmis_num_types] =
{
	L"exmis_none",
	L"exmis_app_name",
	L"exmis_mjsrc_area",
	L"exmis_blocked_stmt"
	L"exmis_failed_include",
	L"exmis_pragma",
	L"exmis_error",
	L"exmis_import",
	L"exmis_line",
	L"exmis_using",
};

static const wchar_t *g_HiddenIncDetailsNames[pinc_num_types] =
{
	L"None",					// pinc_none
	L"SyntaxError",			// pinc_syntax_error
	L"PassiveCode",			// pinc_pasive_code
	L"FileNotFound",			// pinc_file_not_found
	L"FileLoadingError",		// pinc_file_loading_err
	L"PragmaOnce",			// pinc_pragma_once
	L"NestingTooDeep",		// pinc_nesting_deep
};

void PrMiscPreprocStmtEvent::Prepare(ID cn, TMiscEventEx stmt_type, TConditionalStatus stmt_code_stt, PrEventHeader &token_origin, PrEventHeader &last_lex_origin)
{
	memset(this, 0, sizeof(PrMiscPreprocStmtEvent));

	// Setup the header.
	hdr = token_origin;
	hdr.ExtendPrHeader(last_lex_origin);
	hdr.cn = cn;

	// Setup other fields.
	ex_event_type = stmt_type;
	mpr_token = token_origin;
	code_stt = stmt_code_stt;
}

void PrMiscPreprocStmtEvent::PrepareFileNameSection(TLexema &file_name_lex, TMidLevIncludeHelper &hlpr, TFileInfoItem *file_item)
{
	assert(file_name_lex.type == ltx_string);

	doc_ident = (file_item != NULL) ? file_item->data.doc_id : 0;

	if (file_item != NULL)
	{
		// The file is present in the cache.
		fname_data = file_item->data.info.file_name;
	}
	else if (hlpr.load_res == ldres_success)
	{
		// The file was located, but it was not loaded.
		fname_data = hlpr.main_location.DataPtr();
	}
	else
	{
		// The file is not found.
		fname_data = file_name_lex.str_value;
	}

	fname_area = file_name_lex.origin;
}

void PrMiscPreprocStmtEvent::PrepareFileNameSection(TFileInfoItem *file_item, PrEventHeader &fname_lex_origin)
{
	assert(file_item != NULL);

	doc_ident = file_item->data.doc_id;
	fname_data = file_item->data.info.file_name;
	fname_area = fname_lex_origin;
}

const wchar_t *PrMiscPreprocStmtEvent::GetMiscEventExEnumName(TMiscEventEx val)
{
	if (val < 0 || val >= exmis_num_types)
		return(L"BogusMiscEventEx");

	return(g_MiscEventExEnumNames[val]);
}

const wchar_t *PrMiscPreprocStmtEvent::GetNonCallReasonName(THiddenIncudeStmtDtls val)
{
	if (val < 0 || val >= pinc_num_types)
		return(L"BogusHiddenIncDetailsVal");

	return(g_HiddenIncDetailsNames[val]);
}

//---------------------------------------------------------------------------
//  ====================  Static Functions  =========================
//---------------------------------------------------------------------------

static const wchar_t *g_GrammarDefn_SectionTypeNames1[grsc_num_types] =
{
	L"None",				// grsc_none,
	L"Description",		// grsc_descr,
	L"Revision",			// grsc_revision,
	L"LangType",			// grsc_langtype,
	L"Symbols",			// grsc_symbols,
	L"Ignore",			// grsc_ignore,
	L"Errors",				// grsc_errors,
	L"Axioma",			// grsc_axioma,
	L"Rule",				// grsc_rules,
	L"Conflicts",			// grsc_conflicts
	L"Locations",			// grsc_locations,
	L"Resolution",			// grsc_resolution,
};

static const wchar_t *g_GrammarDefn_SectionTypeNames2[grsc_num_types] =
{
	L"<none>",
	L"description",		L"revision",		L"langtype",
	L"symbols",		L"ignore",		L"errors",
	L"axioma",		L"rules",			L"conflicts",
	L"locations",		L"resolution",
};

const wchar_t *GetGrammarDefnSectionTypeName(TGrammarSectionType val)
{
	if (val < 0 || val >= grsc_num_types)
		return(L"BogusGrammarSectionType");

	return(g_GrammarDefn_SectionTypeNames1[val]);
}

TGrammarSectionType GetGrammarDefnSectionType(TStrPtrInfo &str_ptr)
{
	for (int isn=grsc_none+1; isn<grsc_num_types; ++isn)
	{
		if (str_ptr == g_GrammarDefn_SectionTypeNames2[isn])
		{
			return((TGrammarSectionType)isn);
		}
	}

	// The passed string does not match anything.
	return(grsc_none);
}

//---------------------------------------------------------------------------
//  ===============  TMidLevScanCmdlCbkHandler  =====================
//---------------------------------------------------------------------------

void TMidLevScanCmdlCbkHandler::SetupScanner(TMidLevScanner *scanner)
{
	m_scanner = scanner;

	if (m_scanner != NULL)
		m_scanner->cmdl_cbk_handler = this;
}

//---------------------------------------------------------------------------
//  ================  TMidLevScanCbkHandler  =======================
//---------------------------------------------------------------------------

void TMidLevScanCbkHandler::SetupScanner(TMidLevScanner *scanner)
{
	m_scanner = scanner;

	if (m_scanner != NULL)
		m_scanner->cbk_handler = this;
}

void TMidLevScanCbkHandler::ShowProcessingError(const wchar_t *format, ...)
{
	// Do not show the message if the user has already cancelled.
	if (m_console->GetAbortFlag() == TRUE)
		return;

	// Format the error message.
	va_list vargs;
	va_start(vargs, format);
	wchar_t proc_err_buff[2048+80];
	vswprintf(proc_err_buff, 2048, format, vargs);
	va_end(vargs);

	if (m_console->IsUserResponseSupported() == TRUE)
	{
		// Append the continue question. The space for this question is always available.
		wcscat(proc_err_buff, L"\r\n\r\nDo you want to continue?\r\n");
	}

	// Show the message.
	if (m_console->ShowRunTimeError(proc_err_buff, MB_YESNO) == IDNO)
		m_console->SetAbortFlag(TRUE);
}

//--------------------------------------------------------------------------
//  =================  TMidLevIncludeHelper  =======================
//--------------------------------------------------------------------------

TFileInfoItem *TMidLevIncludeHelper::LoadIncludeFile(TLexema &fname_lex)
{
	// The name of the file from the statement should be not empty.
	assert(fname_lex.type = ltx_string);
	assert(fname_lex.str_value.IsEmpty() == FALSE);

	// Trace the name of the file that the helper is looking for.
	if (scanner.trace_mask & sctr_new_file)
	{
		wchar_t loc_buff[MAX_PATH];
		scanner.TraceEvent(&fname_lex.origin, TPRF_INCLUDE_HLPR, L"Looking for: \"%s\".", fname_lex.str_value.CopyWithTruncationTo(loc_buff, MAX_PATH));
	}

	// Reset the output variables.
	load_res = ldres_fname_missing;			// The file is not found yet.
	main_location.ClearBuffer();				// This will make the main_location an emty string.
	inc_path_ident = 0;					// No inc path was used.

	// Prepare other data vars.
	inc_file_item = NULL;
	locations_list.ClearBuffer();
	err_ctx = &fname_lex.origin;

	// Get the type of the file name and copy it into the buffer.
	bool sys_dir = (fname_lex.string_type == lstrt_unicode) ? TRUE : FALSE;
	TFileNameBuffer inc_fname;
	inc_fname.Append(fname_lex.str_value.m_body, fname_lex.str_value.m_len);
	if (inc_fname.GetXpndError() == TRUE)
	{
		wchar_t loc_buff[80];
		scanner.AddOutOfMemError(err_ctx, mloom_incl_hlp_loc1, fname_lex.str_value.CopyWithTruncationTo(loc_buff, 80));
		return(NULL);
	}

	// Convert forward slashes into the backslashes if any.
	TPathHelper::ConvertToBackSlashes(inc_fname);

	// Check the type of the path from the stmt.
	if (TPathHelper::IsAbsolutePath(inc_fname) == TRUE)
	{
		// The path is absolute. Emit the warning.
		scanner.AddWarning(err_ctx, (sys_dir == TRUE) ? TMidLevScanner::mscn_err_ihlp_abs_sys_fname : TMidLevScanner::mscn_err_ihlp_abs_user_fname, 0,
						(load_file_flag == TRUE) ? L"#include" : L"preprocessor");

		// There is just one place on the disk to check for this file.
		CheckAbsoluteLocation(inc_fname, NULL, TRUE);
	}
	else if (TPathHelper::IsRelativePath(inc_fname) == FALSE)
	{
		// This is an intermediate path. It referes either to the current dir on a certain drive or this is
		// an absolute directoty path on a current drive.
		if (scanner.allow_intermediate_paths == FALSE)
		{
			// Options do not allow such paths.
			scanner.AddError(err_ctx, TMidLevScanner::mscn_err_ihlp_path_semi_rel);
			return(NULL);
		}

		// This path is still not nice. Emit a warning.
		scanner.AddWarning(err_ctx, TMidLevScanner::mscn_err_ihlp_path_semi_rel);

		// Resolve the passed name using the Windows features.
		wchar_t fname_buff[2*MAX_PATH];
		DWORD full_path_res = ::GetFullPathNameW(inc_fname, 2*MAX_PATH, fname_buff, NULL);
		if (full_path_res == 0)
		{
			// This is a Win32 problem.
			wchar_t err_code_buff[40];
			scanner.AddError(err_ctx, TMidLevScanner::mscn_err_ihlp_bad_intermp, 0, _itow(::GetLastError(), err_code_buff, 10));
			return(NULL);
		}
		else if (full_path_res >= 2*MAX_PATH)
		{
			// The resulting name is too long.
			wchar_t path_len_buff[40];
			scanner.AddError(err_ctx, TMidLevScanner::mscn_err_ihlp_long_intermp, 0, _itow(full_path_res, path_len_buff, 10));
			return(NULL);
		}
		else
		{
			// Resolving the name succeeded.
			CheckAbsoluteLocation(fname_buff, NULL, TRUE);
		}
	}
	else
	{
		// The path is fully relative.
		if (sys_dir == TRUE && inc_fname[0] == L'.' && inc_fname[1] == L'.')
		{
			scanner.AddWarning(err_ctx, TMidLevScanner::mscn_err_ihlp_upper_sys_ref);
		}

		// Check possible locations.
		CheckRelativeLocations(inc_fname, TRUE);
	}

	// Check if the file was actually found or not.
	if (load_res == ldres_fname_missing && load_file_flag == TRUE)
	{
		// The file was not loaded because it was not found.
		assert(inc_file_item == NULL);
		scanner.AddError(err_ctx, TMidLevScanner::mscn_err_ihlp_file_not_found);
	}

	// Check for duplications using the short name only.
	if (report_dupls == TRUE)
	{
		TFileNameBuffer short_fname;
		if (TPathHelper::ExtractShortName(short_fname, inc_fname) == FALSE)
		{
			scanner.AddOutOfMemError(err_ctx, mloom_incl_hlp_loc2);
		}
		else if (wcscmp(inc_fname, short_fname) != 0)
		{
			// Check duplications of the short name along the paths. This will gererate only warnings
			// and will not load the file even if it will be found somewhere.
			locations_list.ClearBuffer();
			CheckRelativeLocations(short_fname, FALSE);
		}
	}

	// The file could be taken from the cache or loaded from somewhere. Give out the results.
	return(inc_file_item);
}

void TMidLevIncludeHelper::CheckRelativeLocations(const wchar_t *fname_from_stmt, bool load_file_pass)
{
	assert(TPathHelper::IsRelativePath(fname_from_stmt) == TRUE);

	// Iterate the include paths list. Iteration should not be stopped when the file is found because
	// more warnings can be emitted.
	for (TListIter<TIncludePathItem> iter(scanner.inc_paths); iter; ++iter)
	{
		TIncludePathItem &item = iter.CurrItem();
		if (TPathHelper::ConstructPath(buffer1, item.inc_directory, fname_from_stmt) == TRUE)
		{
			// Constructing the absolute path succeeded.
			CheckAbsoluteLocation(buffer1, &item, load_file_pass);
		}
		else
		{
			// The file name construction failed.
			if (buffer1.GetXpndError() == TRUE)
				scanner.AddOutOfMemError(err_ctx, mloom_incl_hlp_loc3);
			else scanner.AddWarning(err_ctx, TMidLevScanner::mscn_err_ihlp_make_fname1, 0, item.inc_directory);
		}
	}

	if (load_res == ldres_fname_missing || load_file_pass == FALSE)
	{
		if (scanner.src_cnt > 0 && (scanner.src_stack[scanner.src_cnt-1].src_type == srct_file || scanner.src_stack[scanner.src_cnt-1].src_type == srct_mj_src))
		{
			// The current lex scr context is the file context. Retrieve the name of the file.
			const wchar_t *layer_fname = scanner.src_stack[scanner.src_cnt-1].GetFileName();
			if (TPathHelper::GetDirectoryName(buffer2, layer_fname) == TRUE && TPathHelper::ConstructPath(buffer1, buffer2, fname_from_stmt) == TRUE)
			{
				// Constructing the absolute path succeeded.
				TIncludePathItem dummy_item(NULL, NULL);
				CheckAbsoluteLocation(buffer1, &dummy_item, load_file_pass);
			}
			else
			{
				// Getting girectory name or applying the relative path failed.
				if (buffer1.GetXpndError() == TRUE || buffer2.GetXpndError() == TRUE)
					scanner.AddOutOfMemError(err_ctx, mloom_incl_hlp_loc4);
				else scanner.AddWarning(err_ctx, TMidLevScanner::mscn_err_ihlp_make_fname2, 0, layer_fname);
			}
		}
	}
}

void TMidLevIncludeHelper::CheckAbsoluteLocation(const wchar_t *abs_file_name, TIncludePathItem *path_item, bool load_file_pass)
{
	assert(TPathHelper::IsAbsolutePath(abs_file_name) == TRUE);

	if (scanner.CreateSubstedPath(err_ctx, buffer2, abs_file_name) == TRUE)
	{
		// Substituting succeeded also. Check this name.
		if (load_file_pass == TRUE)
		{
			if (load_res == ldres_fname_missing)
			{
				// Try to load the file.
				CheckFileName(abs_file_name, path_item, buffer2);
			}
			else if (report_dupls == TRUE && path_item != NULL && path_item->inc_directory != NULL)
			{
				// This call originates from processing some include path.
				CheckOtherPlace(abs_file_name, buffer2);
			}
		}
		else
		{
			// This is an aux check using a short file name.
			assert(report_dupls == TRUE);
			CheckShortName(abs_file_name, buffer2);
		}
	}
}

void TMidLevIncludeHelper::CheckFileName(const wchar_t *abs_file_name, TIncludePathItem *path_item, const wchar_t *substed_file_name)
{
	// The file is not found anywhere yet. Try to load it.
	assert(load_res == ldres_fname_missing);

	// Check the files cache and the file system.
	inc_file_item = scanner.FindCachedFile(abs_file_name);
	if (inc_file_item != NULL)
	{
		// The file is available in the cache.
		load_res = ldres_success;
		main_location.SetupReadOnlyString(inc_file_item->data.info.file_name);
		inc_path_ident = (path_item != NULL) ? path_item->inc_path_id : 0;

		if (scanner.trace_mask & sctr_new_file)
		{
			wchar_t file_id_buff[40];
			scanner.TraceEvent(err_ctx, TPRF_INCLUDE_HLPR, L"Success: Cached file retrieved: FileId=%s, FileName=\"%s\".",
							inc_file_item->data.PrepareFileId(file_id_buff, 40), inc_file_item->data.info.file_name);
		}
	}
	else if (TPathHelper::IsPathAFile(substed_file_name) == TRUE)
	{
		// Passed path corresponds to a file.
		if (report_curr_dir_loads == TRUE)
		{
			scanner.AddWarning(err_ctx, TMidLevScanner::mscn_err_ihlp_curr_dir_load);
		}

		// Try to load and register this file.
		if (load_file_flag == TRUE)
		{
			TFileInfo finfo;
			finfo.Clear();
			finfo.Init(strg_disk, rscn_c_cpp);
			load_res = TFileBodyHelper::LoadAsciiFileBody(finfo.info, abs_file_name, substed_file_name, FALSE);

			if (load_res == ldres_success)
			{
				// Register and cache this file.
				inc_file_item = scanner.RegisterNewFile(err_ctx, finfo);
				if (inc_file_item == NULL)
				{
					// An error was detected and it is already reported.
					finfo.ReleaseBasicFileInfo();
				}
			}
			else
			{
				// Write an error about the loading failure.
				scanner.AddError(err_ctx, TMidLevScanner::mscn_err_ihlp_file_load_error, 0, TFileBodyHelper::GetLoadSaveResultText(load_res), abs_file_name);
				if (scanner.trace_mask & sctr_new_file)
				{
					scanner.TraceEvent(err_ctx, TPRF_INCLUDE_HLPR,
								L"Error (%s) while loading file: \"%s\".", TFileBodyHelper::GetLoadSaveResultText(load_res), substed_file_name);
				}
			}
		}
		else
		{
			// The request was only to identify the location, not to load the file.
			load_res = ldres_success;
		}

		assert(load_res != ldres_fname_missing);

		main_location.ClearBuffer();
		if (main_location.Append(abs_file_name) == FALSE)
		{
			scanner.AddOutOfMemError(err_ctx, mloom_incl_hlp_loc5, abs_file_name);
			main_location.ClearBuffer();
			main_location.Append(L"---Bogus--Main--Location---");
		}

		inc_path_ident = (path_item != NULL) ? path_item->inc_path_id : 0;

		if (path_item != NULL && path_item->inc_directory != NULL)
			path_item->cnt_use++;
	}
}

void TMidLevIncludeHelper::CheckOtherPlace(const wchar_t *abs_file_name, const wchar_t *substed_file_name)
{
	assert(load_res != ldres_fname_missing);
	const wchar_t *main_loc_fname = (inc_file_item != NULL) ? inc_file_item->data.info.file_name : main_location;

	if (wcscmp(main_loc_fname, abs_file_name) != 0 && TPathHelper::IsPathAFile(substed_file_name) == TRUE)
	{
		//
		//  It is clear that:
		//
		//      -- the file was already found;
		//      -- passed file name differs from the original place.
		//      -- passed file name is a file;
		//
		//  This means that include file could be taken from this location also. Write a warning.
		//

		// Check if message about this location is already posted.
		if (IsLocationReported(abs_file_name) == FALSE)
		{
			// Write warning about this location.
			scanner.AddWarning(err_ctx, TMidLevScanner::mscn_err_ihlp_other_path_loc, 0, main_loc_fname, abs_file_name);

			// Add this location to the list of places where this file can be found. The length of the data is passed directly.
			// This number of chars will be added to the buffer plus the method Append() will add its own termination zero.
			// So, there will be two zeroes at the end of the buffer.
			locations_list.Append(abs_file_name, (long)wcslen(abs_file_name)+1);
		}
	}
}

void TMidLevIncludeHelper::CheckShortName(const wchar_t *abs_file_name, const wchar_t *substed_file_name)
{
	if (TPathHelper::IsPathAFile(substed_file_name) == TRUE)
	{
		// The substed file name corresponds to an existing file. Issue a warning if this is not done yet.
		if (IsLocationReported(abs_file_name) == FALSE)
		{
			// This is a new place. Comment it.
			if (load_res != ldres_fname_missing)
			{
				const wchar_t *main_loc_fname = (inc_file_item != NULL) ? inc_file_item->data.info.file_name : main_location;
				int err_msg_id = (wcscmp(main_loc_fname, abs_file_name) == 0) ? TMidLevScanner::mscn_err_ihlp_short_name_also1 : TMidLevScanner::mscn_err_ihlp_short_name_also2;
				scanner.AddWarning(err_ctx, err_msg_id, 0, main_loc_fname, abs_file_name);
			}
			else
			{
				scanner.AddWarning(err_ctx, TMidLevScanner::mscn_err_ihlp_short_name_cload, 0, abs_file_name);
			}

			// Add this location to the list of places where this file was found.
			locations_list.Append(abs_file_name, (long)wcslen(abs_file_name)+1);
		}
	}
}

bool TMidLevIncludeHelper::IsLocationReported(const wchar_t *file_name)
{
	// There should be two zeroes at the end of the buffer.
	wchar_t *ptr_dup = locations_list.DataPtr();
	while (ptr_dup != NULL && ptr_dup[0] != 0)
	{
		if (wcscmp(ptr_dup, file_name) == 0)
		{
			// This place was already reported.
			return(TRUE);
		}

		ptr_dup += wcslen(ptr_dup)+1;
	}

	// This is unknown place.
	return(FALSE);
}

//---------------------------------------------------------------------------
//  =====================  TLexemasSource  =======================
//---------------------------------------------------------------------------

static const wchar_t *g_LexSourceTypeEnumNames[srct_num_types] =
{
	L"srct_none",				L"srct_cmdl",
	L"srct_ant_env",			L"srct_value3",
	L"srct_file",				L"srct_bltin_defs",
	L"srct_mj_src",			L"srct_scgm",
	L"srct_macro",			L"srct_mparam",
	L"srct_pbuff_macro",		L"srct_pbuff_mparam",
};

const wchar_t *TLexemasSource::GetLexSourceTypeEnumName(TLexSourceType val)
{
	if (val < 0 || val >= srct_num_types)
		return(L"BogusLexSourceTypeValue");

	return(g_LexSourceTypeEnumNames[val]);
}

//----------------------------------------------------------------------------
//  =================  TCommandLineTaskInfo  =======================
//----------------------------------------------------------------------------

void TCommandLineTaskInfo::Init(TParseEnvInfo &env, int mj_src_prm, THighLevelLangType langt)
{
	cmdl_origin.Clear();
	cmdl_ks_chars  = env.cmdl_ks_chars;
	cmdl_keys_dict = env.cmdl_keys_dict;
	cmdl_rscn_type = env.cmdl_rscn_type;

	mj_src_pos_param = mj_src_prm;
	mj_src_strg_type = env.src_file_strg;
	mj_src_rscn_type = env.src_file_rscn;

	src_unicode = FALSE;
	lang_type = langt;
}

//----------------------------------------------------------------------------
//  ====================  TMidLevScanner  ==========================
//----------------------------------------------------------------------------

TMidLevScanner::TMidLevScanner(TMidLevScanCbkHandler *mid_lev_cbk, TMidLevScanCmdlCbkHandler *cmdl_cbk)
{
	// Verify the error code values from the local enum.
	assert(mscn_err_base == perb_mid_scan_base);
	assert(mscn_err_max <= perb_mid_scan_max);

	//
	// Setup only those fields that will not be inited with the call to the MidLevReset() at the end of the ctor.
	//
	parsing_phase = mlph_none;

	// These pointers should be reseted before calling the MidLevReset() otherwise it will try to purge them.
	major_src_info.ResetPointers();

	src_cnt = 0;
	pbuff_cnt = 0;
	cond_cnt = 0;

	SetupCbkHandler(mid_lev_cbk);
	SetupCmdlCbkHandler(cmdl_cbk);

	// The tracing setup is done only once. It is not reset between the parsing sessions.
	trace_mask = 0;					// The tracing is disabled by default.
	next_trace_evtid = 1;

	// Reset method will init the the fields that were not prepared with the code above.
	MidLevReset(TRUE, TRUE);
}

TMidLevScanner::~TMidLevScanner()
{
	// Do the high level actions.
	while (src_cnt > 0)
		ExitFromLexSource(NULL);

	drive_substs.DeleteAll();
	inc_paths.DeleteAll();
	known_files.DeleteAll();
	temp_files.DeleteAll();

	macrodefs_list.DeleteAll();
	keys_list.DeleteAll();

	// Reset the back link from the callback handlers.
	cbk_handler->SetupScanner(NULL);
	if (cmdl_cbk_handler != NULL)
		cmdl_cbk_handler->SetupScanner(NULL);
}

void TMidLevScanner::MidLevReset(bool full_reset, bool reset_disk_files_cache)
{
	// High level actions.
	while (src_cnt > 0)
		ExitFromLexSource(NULL);

	// Phase and options.
	if (parsing_phase != mlph_none)
	{
		if (trace_mask & sctr_major_actions)
		{
			// Make the trace.
			TraceEvent(NULL, TPRF_MLEV_RESET, L"Changing phase: %s --> %s.", g_ParsingPhaseNames[parsing_phase], g_ParsingPhaseNames[mlph_none]);
		}

		SetPhase(mlph_none);
	}

	if (full_reset == TRUE)
	{
		cmdl_task_info.Clear();
	}

	if (full_reset == TRUE || reset_disk_files_cache == TRUE)
	{
		if (major_src_info.cmd_line_info != NULL)
			PurgeCachedFile(major_src_info.cmd_line_info);
		if (major_src_info.mj_src_info != NULL)
			PurgeCachedFile(major_src_info.mj_src_info);
		if (major_src_info.obj_file_info != NULL)
			PurgeCachedFile(major_src_info.obj_file_info);
	}

	if (full_reset == TRUE)
	{
		major_src_info.Clear();
		prep_opts.Clear();
		scan_opts_proto.Init();
		options.Init();
		abort_err_code = 0;
		abort_err_class = lerrc_message;
	}

	// Info about the dirs and paths.
	base_directory.ClearBuffer();
	drive_substs.DeleteAll();
	allow_intermediate_paths = FALSE;
	allow_non_substed_paths = FALSE;
	inc_paths.DeleteAll();

	// Disk files, built in and proto files.
	if (reset_disk_files_cache == TRUE)
	{
		// Note that if the reset_disk_files_cache flag is accompanied with the full_reset flag,
		// there is is no need to reset the session status in the known files as a separate step.
		known_files.DeleteAll();
	}
	else if (full_reset == TRUE)
	{
		// This call will clear the "pragma once" flags and reset the usage counts.
		known_files.ResetSessionStts();
	}

	// Temp and intermediate files.
	if (full_reset == TRUE)
	{
		temp_files.DeleteAll();
		for (int i=0; i<mbit_num_types; ++i)
		{
			scgm_info[i].DeleteAll();
			scgm_info[i].file_info = NULL;
			scgm_info[i].dwData = 0;
		}
	}

	// Sources nesting stack.
	src_cnt = 0;
	src_on_doubt = FALSE;
	src_cnt_min = 0;
	pbuff_cnt = 0;

	// Conditional compilation stack.
	cond_cnt = 0;
	active_code = TRUE;
	conditional_area = FALSE;
	cond_pars_status = cexr_hard_true;
	cond_area_cn = 0;

	// Macrodefinitions dictionary.
	macrodefs_list.DeleteAll();
	if (full_reset == TRUE)
		keys_list.DeleteAll();

	// Misc data fields.
	max_rprt_linear = 0;
	saved_lex.Clear();
	include_fname_mode = FALSE;
	latest_cn = 0;
	if (full_reset == TRUE)
	{
		raw_sess_summary.Clear();
		summary.Clear();
		summary.cond_parsing_areas = 1;
		summary.final_linear_offs = 0;				// Parsing starts with current linear offset equal to zero. When the first
												// lex source will be entered, the linear offset will become one.
	}

	curr_trace_attr = TGenericConsole::DefHighlightMode;

	// This call will also clear the abort parsing confirmation flag.
	console_data_stt.Clear();
	status_callback_cnt = 0;

	// Conditional statement expressions support.
	soft_macro_expansion_mode = FALSE;
	macro_call_happened = FALSE;

	// Misc stuffs.
	block_paths_substing = FALSE;
	if (full_reset == TRUE)
		misc_strings_dict.DeleteAll();
}

bool TMidLevScanner::BeginParsingSession(ID batch_id, const wchar_t *parsing_base_dir, TDriveSubstInfo *drive_substs_array,
											bool allow_interm_paths, bool allow_unsubsted_paths)
{
	// Call to this function starts the mid lev parsing session. There is no other way to start it.
	if (trace_mask & sctr_major_actions)
	{
		TraceFuncEnter(NULL, TPRF_BEG_PARS_SESS);
	}

	// Kill the prev parsing session if any and start a new one.
	MidLevReset(TRUE);
	SetPhase(mlph_filereg);
	InitConsoleInternal();

	// Call special function that will resolve the base dir and maybe return the error codes back.
	DWORD curr_dir_len, curr_dir_win32_error;
	wchar_t parsing_base_dir_buffer[2*MAX_PATH];
	wchar_t *base_dir = PrepareBaseDirectory(parsing_base_dir_buffer, parsing_base_dir, curr_dir_len, curr_dir_win32_error);

	PrInitParsingInfo1 init_info =
	{
		batch_id, CurrDateTime(), base_dir,
		allow_interm_paths, allow_unsubsted_paths,
	};

	// Send the callback. This call will also allocate a parsing ID for this session.
	if (cbk_handler->StartParsing(init_info) == FALSE)
	{
		// The session preparation failed. Error should be already displayed.
		if (trace_mask & sctr_major_actions)
		{
			TraceFuncExit(NULL, TPRF_BEG_PARS_SESS, L"Failure in cbk->StartParsing(). Parsing was not started.");
		}

		// Return the parsing phase back.
		SetPhase(mlph_none);
		return(FALSE);
	}

	// Process the file handling options as a second step because this method may report parsing errors.
	if (SetupFileHandlingFields(parsing_base_dir, base_dir, curr_dir_len, curr_dir_win32_error, drive_substs_array, allow_interm_paths, allow_unsubsted_paths) == FALSE)
	{
		// An error is already reported from inside.
		cbk_handler->FinalizeParsing(&prep_opts, &scan_opts_proto, summary, mscn_err_misc_bad_fsys_setup);
		if (trace_mask & sctr_major_actions)
		{
			TraceFuncExit(NULL, TPRF_BEG_PARS_SESS, L"Failure in the SetupFileHandlingFields(). The parsing was not started.");
		}

		// Return the parsing phase back.
		SetPhase(mlph_none);
		return(FALSE);
	}

	// The callback has accepted the session start and the file handling props.
	if (trace_mask & sctr_major_actions)
	{
		TraceFuncExit(NULL, TPRF_BEG_PARS_SESS, L"Success: ParsingId=%d.", cbk_handler->GetParsingId());
	}

	// Method succeeded.
	return(TRUE);
}

bool TMidLevScanner::BeginRawIndexingSession(ID batch_id, const wchar_t *parsing_base_dir, TDriveSubstInfo *drive_substs_array,
												bool allow_interm_paths, bool allow_unsubsted_paths)
{
	// Call to this function starts the raw parsing session. There is no other way to start it.
	if (trace_mask & sctr_major_actions)
	{
		TraceFuncEnter(NULL, TPRF_BEG_RAW_SESS);
	}

	// Kill the prev parsing session if any and start a new one.
	MidLevReset(FALSE);
	SetPhase(mlph_indexing);
	InitConsoleInternal();

	// Call special function that will resolve the base dir and maybe return the error codes back.
	DWORD curr_dir_len, curr_dir_win32_error;
	wchar_t parsing_base_dir_buffer[2*MAX_PATH];
	wchar_t *base_dir = PrepareBaseDirectory(parsing_base_dir_buffer, parsing_base_dir, curr_dir_len, curr_dir_win32_error);

	PrInitParsingInfo1 init_info =
	{
		batch_id, CurrDateTime(), base_dir,
		allow_interm_paths, allow_unsubsted_paths,
	};

	// Send a callback.
	if (cbk_handler->StartRawIndexingSession(init_info) == FALSE)
	{
		// Session preparation failed. An error should be already displayed.
		if (trace_mask & sctr_major_actions)
		{
			TraceFuncExit(NULL, TPRF_BEG_RAW_SESS, L"Failure in cbk->StartRawIndexingSession(). The parsing was not started.");
		}

		// Return the parsing phase back.
		SetPhase(mlph_none);
		return(FALSE);
	}

	// Process the file handling options as a second step because this method may report parsing errors.
	if (SetupFileHandlingFields(parsing_base_dir, base_dir, curr_dir_len, curr_dir_win32_error, drive_substs_array, allow_interm_paths, allow_unsubsted_paths) == FALSE)
	{
		// An error is already reported from inside.
		GetRawIndexingSummary(raw_sess_summary);
		cbk_handler->FinishRawIndexingSession(raw_sess_summary);
		if (trace_mask & sctr_major_actions)
		{
			TraceFuncExit(NULL, TPRF_BEG_RAW_SESS, L"Failure in the SetupFileHandlingFields(). The parsing was not started.");
		}

		// Return the parsing phase back.
		SetPhase(mlph_none);
		return(FALSE);
	}

	// Callback has accepted the session start and the file handling props.
	if (trace_mask & sctr_major_actions)
	{
		TraceFuncExit(NULL, TPRF_BEG_RAW_SESS, L"Success: ParsingId=%d.", cbk_handler->GetParsingId());
	}

	// Method succeeded.
	return(TRUE);
}

//
// Create file info for disk/temp/proto file.
//
TFileInfoItem *TMidLevScanner::CreateFileInfo(PrEventHeader *src_context, TFileStorageType strg, TRawScanningType rscn, const wchar_t *fname)
{
	assert(parsing_phase != mlph_none);
	assert(strg == strg_disk || strg == strg_temp || strg == strg_proto);

	// Convert passed file name into the absolute form if it is not absolute already.
	TFileNameBuffer buffer;
	if (CreateAbsolutePath(src_context, buffer, fname) == FALSE)
	{
		if (trace_mask & sctr_new_file)
			TraceEvent(src_context, TPRF_CREATE_FINFO, L"Failure in the CreateAbsolutePath().");
		return(NULL);
	}

	// Check, maybe this file is already known.
	if (strg != strg_temp)
	{
		TFileInfoItem *pfi = FindCachedFile(buffer);
		if (pfi != NULL)
		{
			// This file name is known. It should have the same major file attributes.
			assert(pfi->data.strg_type == strg);
			assert(pfi->data.rscn_type == rscn);

			if (trace_mask & sctr_new_file)
			{
				wchar_t file_id_buff[40];
				TraceEvent(src_context, TPRF_CREATE_FINFO, L"Success: Cached file retrieved: FileId=%s, FileName=\"%s\".",
							pfi->data.PrepareFileId(file_id_buff, 40), pfi->data.info.file_name);
			}

			return(pfi);
		}
	}

	// Prepare the copy of the file name and prepare the file body.
	TFileInfo finfo;
	finfo.Clear();
	finfo.Init(strg, rscn);
	if (strg != strg_proto)
	{
		// Convert this file info into the emulated env terms if needed.
		TFileNameBuffer substed_fname;
		if (CreateSubstedPath(src_context, substed_fname, buffer) == FALSE)
		{
			// Creation failed. Syntax error about this is already reported.
			if (trace_mask & sctr_new_file)
			{
				TraceEvent(src_context, TPRF_CREATE_FINFO, L"Failure in the CreateSubstedPath().");
			}

			return(NULL);
		}

		// The resulting path should correspond to some file object.
		if (TPathHelper::IsPathAFile(substed_fname) == FALSE)
		{
			int ec = (wcscmp(buffer, substed_fname) == 0) ? mscn_err_srcst_file_not_found1 : mscn_err_srcst_file_not_found1;
			AddError(src_context, ec, 0, buffer, substed_fname);
			if (trace_mask & sctr_new_file)
			{
				TraceEvent(src_context, TPRF_CREATE_FINFO, L"Failure - the file is not existing: \"%s\".", substed_fname.DataPtr());
			}

			return(NULL);
		}

		// This file is either a temp file or it was not used before. Load this file from the disk.
		TLoadSaveResult res = TFileBodyHelper::LoadAsciiFileBody(finfo.info, buffer, substed_fname, FALSE);
		if (res != ldres_success)
		{
			int ec = (wcscmp(buffer, substed_fname) == 0) ? mscn_err_srcst_file_load_error1 : mscn_err_srcst_file_load_error2;
			AddError(src_context, ec, 0, TFileBodyHelper::GetLoadSaveResultText(res), buffer, substed_fname);
			if (trace_mask & sctr_new_file)
			{
				TraceEvent(src_context, TPRF_CREATE_FINFO, L"Failure - error loading file: %s.", TFileBodyHelper::GetLoadSaveResultText(res));
			}

			return(NULL);
		}
	}
	else
	{
		// Proto files have neither date nor body.
		TLoadSaveResult res = TFileBodyHelper::SetupInMemFileBody(finfo.info, buffer, NULL, 0);
		if (res != ldres_success)
		{
			AddOutOfMemError(src_context, mloom_create_finfo_loc1, buffer);
			if (trace_mask & sctr_new_file)
			{
				TraceEvent(src_context, TPRF_CREATE_FINFO, L"Failure - cannot setup the in mem file body: %s.", TFileBodyHelper::GetLoadSaveResultText(res));
			}

			return(NULL);
		}
	}

	// Register this file descriptor with the callback. This method will also emit one event based trace under the sctr_new_file tag.
	TFileInfoItem *file_item = RegisterNewFile(src_context, finfo);
	if (file_item == NULL)
	{
		// An error was detected and it is already reported.
		finfo.ReleaseBasicFileInfo();
		return(NULL);
	}

	// Success.
	assert(file_item->data.file_id != 0);
	return(file_item);
}

//
// Create file info for file with a body in memory. These files have temp/builtin storage type.
//
TFileInfoItem *TMidLevScanner::CreateFileInfo(PrEventHeader *src_context, TFileStorageType strg, TRawScanningType rscn,
											const wchar_t *fname, const wchar_t *fbody, long file_len, TDateTime fdate)
{
	assert(parsing_phase != mlph_none);
	assert(strg == strg_temp || strg == strg_builtin);

	// Check, maybe this file is already known.
	if (strg == strg_builtin)
	{
		assert(fdate != 0);
		TFileInfoItem *pfi = FindCachedFile(0, (ID)fdate);
		if (pfi != NULL)
		{
			// Ensure that major file attributes are the same.
			assert(pfi->data.strg_type == strg);
			assert(pfi->data.rscn_type == rscn);

			if (trace_mask & sctr_new_file)
			{
				wchar_t file_id_buff[40];
				TraceEvent(src_context, TPRF_CREATE_FINFO, L"Success: Cached built-in file retrieved: FileId=%s, FileName=\"%s\".",
							pfi->data.PrepareFileId(file_id_buff, 40), pfi->data.info.file_name);
			}

			return(pfi);
		}
	}

	// Create and prepare the file descriptor. SetupInMemFileBody will duplicate the file name and the body into the heap.
	TFileInfo finfo;
	finfo.Clear();
	finfo.Init(strg, rscn);
	if (strg == strg_builtin)
	{
		// Built in files have well known IDs. Caller passes this ID in the file date parameter.
		// This may sound strange, but this is the current design.
		finfo.file_id = (ID)fdate;
	}

	TLoadSaveResult res = TFileBodyHelper::SetupInMemFileBody(finfo.info, fname, fbody, file_len, fdate);
	if (res != ldres_success)
	{
		AddOutOfMemError(src_context, mloom_create_finfo_loc2, fname);
		if (trace_mask & sctr_new_file)
		{
			TraceEvent(src_context, TPRF_CREATE_FINFO, L"Failure - cannot setup the in mem file body: %s.", TFileBodyHelper::GetLoadSaveResultText(res));
		}

		return(NULL);
	}

	// Register this file descriptor with the callback. In case of success RegisterNewFile() takes ownership on
	// the file name and body. This method will also emit one event based trace under the sctr_new_file tag.
	TFileInfoItem *file_item = RegisterNewFile(src_context, finfo);
	if (file_item == NULL)
	{
		// An error was detected and it is already reported.
		finfo.ReleaseBasicFileInfo();
		return(NULL);
	}

	// Success.
	assert(file_item->data.file_id != 0);
	return(file_item);
}

TFileInfoItem *TMidLevScanner::FindCachedFile(const wchar_t *fname)
{
	assert(fname != NULL && fname[0] != 0);

	// Check the table of the disk files first.
	TFileInfoItem *item = known_files.LookupFileInfoItem(fname);
	if (item != NULL)
		return(item);

	// Iterate the list of temp files.
	for (TListIter<TFileInfoItem> iter(temp_files); iter; ++iter)
	{
		TFileInfoItem *item = &iter.CurrItem();
		if (wcscmp(item->data.info.file_name, fname) == 0)
			return(item);
	}

	// Unknown file name.
	return(NULL);
}

TFileInfoItem *TMidLevScanner::FindCachedFile(ID pars_id, ID file_id)
{
	assert(file_id != 0);

	if (pars_id == 0)
	{
		// This file should be stored in the hash table the of disk files. Use slow iteration of all files.
		return(known_files.LookupFileInfoItem(file_id));
	}

	// Temp file should belong to the curr session.
	assert(pars_id == cbk_handler->GetParsingId());

	// Iterate the list of temp files.
	for (TListIter<TFileInfoItem> iter(temp_files); iter; ++iter)
	{
		TFileInfoItem *item = &iter.CurrItem();

		// It is ok to check only the file_id because all temp files in the list belong to the current session.
		if (item->data.file_id == file_id)
			return(item);
	}

	// Unknown file Id.
	return(NULL);
}

void TMidLevScanner::ExtractFilesCache(TList &disk_files_to, TList &temp_files_to)
{
	// Reset the local pointers if any.
	major_src_info.ResetPointers();
	for (int i=0; i<mbit_num_types; ++i)
	{
		scgm_info[i].DeleteAll();
		scgm_info[i].file_info = NULL;
	}

	// Give out the tables.
	known_files.ConvertToList(disk_files_to);
	temp_files_to.TakeDataFrom(temp_files);
}

bool TMidLevScanner::BeginPrepPhase(TFileInfoItem *cmdl, TCommandLineTaskInfo &cmdl_info, TMidLevPrepOptions *cmdl_opts, TMidLevScannerOptions *main_opts_proto)
{
	assert(cmdl != NULL);
	assert(parsing_phase == mlph_filereg);

	if (trace_mask & sctr_major_actions)
	{
		TraceFuncEnter(NULL, TPRF_PREP_PHASE, L"MajorSrcPrm=%d, CmdlFileName=\"%s\".", cmdl_info.mj_src_pos_param, cmdl->data.info.file_name);
	}

	if (CheckAbortParsingState() == TRUE)
	{
		if (trace_mask & sctr_major_actions)
		{
			TraceFuncExit(NULL, TPRF_PREP_PHASE, L"Failure because the parsing is aborted.");
		}

		return(FALSE);
	}

	// Start the new parsing phase.
	SetPhase(mlph_preparing);

	// Send a callback notification. The passed options contain only preliminary values. Final values will
	// be sent to the mid lev callback when the session will be closed.
	PrInitParsingInfo2 init_info =
	{
		cmdl_info.cmdl_origin, cmdl, cmdl_info.mj_src_pos_param,
		(cmdl_opts != NULL) ? cmdl_opts : &prep_opts
	};

	if (cbk_handler->StartPrepPhase(init_info) == FALSE)
	{
		// Session preparation failed. An error should be already displayed.
		if (trace_mask & sctr_major_actions)
		{
			TraceFuncExit(NULL, TPRF_PREP_PHASE, L"Failure in cbk->StartPrepPhase().");
		}

		// Return the parsing phase back.
		SetPhase(mlph_filereg);
		return(FALSE);
	}

	// Callback has accepted the phase change request. Save the parameters.
	major_src_info.cmd_line_ident.doc_id = cmdl->data.doc_id;
	major_src_info.cmd_line_info = cmdl;
	cmdl_task_info = cmdl_info;

	if (cmdl_opts != NULL)
		prep_opts = *cmdl_opts;
	if (main_opts_proto != NULL)
		scan_opts_proto = *main_opts_proto;

	// Main options for this parsing phase will be taken from the prep options.
	options = prep_opts.scan_opts;

	// Setup the command line as the source of lexemas. Scan the whole line from the beginning up to its end.
	assert(cmdl->data.file_id != 0);
	if (EnterFileSource(0, cmdl, srct_cmdl, mdo_predefined, 0, -1, TRUE, FALSE, NULL, NULL, NULL) == FALSE)
	{
		// An error should be already displayed. Although is not clear what that might be.
		if (trace_mask & sctr_major_actions)
		{
			TraceFuncExit(NULL, TPRF_PREP_PHASE, L"Error nesting the command line lex source.");
		}

		// Return the parsing phase back.
		SetPhase(mlph_filereg);
		return(FALSE);
	}

	// The command line source was entered. Check, if it is necessary to enterthe analysis tool env file
	// and/or the file with definitions of the built in macros. If this is so, enter the proper file.
	FirstLexSourceEnterActions();

	// Method succeeded.
	if (trace_mask & sctr_major_actions)
	{
		// Make the trace.
		wchar_t fid_buff[40];
		TraceFuncExit(NULL, TPRF_PREP_PHASE, L"Success: ParsingId=%d, cmdl.file_id=%s, MajorSrcPrm=%d.",
					cbk_handler->GetParsingId(), cmdl->data.PrepareFileId(fid_buff, 40), cmdl_task_info.mj_src_pos_param);
	}

	// Full success.
	return(TRUE);
}

bool TMidLevScanner::BeginMainPhase(THighLevelLangType lang_type, TFileInfoItem *mj_src_prm, TFileInfoItem *obj_proto,
										PrEventHeader **call_origin, TMidLevScannerOptions *opts)
{
	assert(mj_src_prm != NULL);
	PrEventHeader *ftr_ctx = (call_origin != NULL) ? call_origin[0] : NULL;
	if (trace_mask & (sctr_major_actions | sctr_main_phase))
	{
		TraceFuncEnter(ftr_ctx, TPRF_MAIN_PHASE, L"FileName=\"%s\".", mj_src_prm->data.info.file_name);
	}

	// Main phase can either start immediately after opening the parsing session (if the commnad line is not used)
	// or it can start after processing either the whole cmdl or only a part of the cmdl.
	assert(parsing_phase == mlph_filereg || parsing_phase == mlph_preparing);
	if (CheckAbortParsingState() == TRUE)
	{
		if (trace_mask & (sctr_major_actions | sctr_main_phase))
		{
			TraceFuncExit(ftr_ctx, TPRF_MAIN_PHASE, L"Failure because the parsing is aborted.");
		}

		return(FALSE);
	}

	// Switch to the new parsing phase.
	SetPhase(mlph_scanning);

	// Send a callback. Passed options contain only a preliminary values. Final values will be sent
	// to the callback handler only when the session will be closed.
	PrInitParsingInfo3 init_info =
	{
		lang_type, mj_src_prm, obj_proto,
		(opts != NULL) ? opts : &options
	};

	if (cbk_handler->StartMainPhase(init_info) == FALSE)
	{
		// The session preparation failed. An error should be already displayed.
		if (trace_mask & (sctr_major_actions | sctr_main_phase))
		{
			TraceFuncExit(ftr_ctx, TPRF_MAIN_PHASE, L"Failure in cbk->StartMainPhase().");
		}

		// Proceed directly to the "Major EOF".
		SetPhase(mlph_major_eof);
		return(FALSE);
	}

	// Callback has accepted the main phase start request. Save the parameters.
	cmdl_task_info.lang_type = lang_type;
	major_src_info.mj_src_ident.doc_id = mj_src_prm->data.doc_id;
	major_src_info.mj_src_info = mj_src_prm;
	major_src_info.mj_src_entered = TRUE;
	if (obj_proto != NULL)
	{
		major_src_info.obj_file_ident.doc_id = obj_proto->data.doc_id;
		major_src_info.obj_file_info = obj_proto;
	}

	if (opts != NULL)
	{
		// Set both copies of options to the passed param.
		if (opts != &scan_opts_proto)
			scan_opts_proto = *opts;
		options = *opts;
	}

	// Setup the new lexemas source.
	if (EnterFileSource(0, mj_src_prm, srct_mj_src, mdo_definition,
					0, -1, TRUE, FALSE,
					(call_origin != NULL) ? call_origin[0] : NULL,
					(call_origin != NULL) ? call_origin[1] : NULL,
					(call_origin != NULL) ? call_origin[2] : NULL) == FALSE)
	{
		// An error should be already displayed.
		if (trace_mask & (sctr_major_actions | sctr_main_phase))
		{
			TraceFuncExit(ftr_ctx, TPRF_MAIN_PHASE, L"Error nesting the major file source.");
		}

		// Proceed directly to the "Major EOF" phase.
		SetPhase(mlph_major_eof);
		return(FALSE);
	}

	// The major source file was entered.
	if (major_src_info.cmd_line_info == NULL)
	{
		// The command line was not used. This is the first lex source.
		FirstLexSourceEnterActions();
	}

	// Method succeeded.
	if (trace_mask & sctr_major_actions)
	{
		// Make the trace.
		wchar_t buff_fid[40];
		mj_src_prm->data.PrepareFileId(buff_fid, 40);
		TraceFuncExit(ftr_ctx, TPRF_MAIN_PHASE, L"Success: ParsingId=%d, major_src_fid=%s.", cbk_handler->GetParsingId(), buff_fid);
	}

	// Full success.
	Console().SetConsoleLabel(sclb_major_file_name, mj_src_prm->data.info.file_name);
	return(TRUE);
}

TLexemaType TMidLevScanner::Scan(TLexema &lex)
{
	// Trace using flag that is used only once here.
	if (trace_mask & sctr_scan_enter)
	{
		TraceFuncEnter(NULL, TPRF_SCAN, L"Function entered.");
	}

	// There should be at least one nested source. Paste buffering should not be active at this time.
	assert(parsing_phase >= mlph_preparing);
	assert(src_cnt > 0 && pbuff_cnt == 0);

	//
	//  Loop on processing lexemas and macro directives. This loop breakes, when the first lexema
	//  that belongs to the active code path is found. This lexema is returned to the upper layer.
	//
	for(;;)
	{
		// The StatusCallBack is processed here and not in the MidScan() because this makes the code more simple.
		if (--status_callback_cnt < 0)
		{
			// Update the console panel and recalculate the value that controls delay that should elapse before
			// the next console update.
			status_callback_cnt = UpdateConsoleInternal(FALSE);

			// Check for external abort event.
			if (CheckAbortParsingState() == TRUE)
			{
				// Create EOF lexema right here.
				GetCurrScanPos(lex.origin, latest_cn);
				lex.first_in_line = src_stack[src_cnt-1].GetFirstInLine();
				lex.on_doubt = src_on_doubt;
				lex.SetEndOfFileLex();

				// Resetting the calback cnt will force the scanner to come here again if application will call
				// the Scan() method once more.
				status_callback_cnt = 0;
				break;
			}
		}

		if (options.raw_scanning_mode == TRUE)
		{
			// Do the simple stuff.
			GetLoLex(lex);
			break;
		}

		// Pick up the next lexema.
		MidScan(lex);

		if (lex.type == ltx_eof)
		{
			// MidScan can return EOF only when the end of the lex sources root is reached. Ensure this.
			assert(src_cnt == 1);

			if (major_src_info.mj_src_info != NULL && prep_opts.immed_mj_src_load == FALSE && major_src_info.mj_src_entered == FALSE)
			{
				// The delayed loading of the major source file was requested. Enter the major source file.
				PrEventHeader *src_loc[3] = { &lex.origin, NULL, NULL };
				BeginMainPhase(cmdl_task_info.lang_type,
							major_src_info.mj_src_info, major_src_info.obj_file_info,
							src_loc, &scan_opts_proto);
				continue;
			}

			if (cmdl_task_info.mj_src_pos_param != 0 && major_src_info.mj_src_prm_found == FALSE)
			{
				// Caller has asked for the pos param that is not available. Emit an error in the terms where the name
				// of the app is considered to be a param zero and other pos params are countered starting from one.
				int cnt_pos_params = summary.cmdl_pos_params-1;

				wchar_t buff_num_params[80];
				if (cnt_pos_params == 1)
					wcscpy(buff_num_params, L", while there is only 1 pos param on the command line");
				else swprintf(buff_num_params, 80, L", while there are only %s pos param%s", cnt_pos_params, PluralSuffix(cnt_pos_params));

				wchar_t buff1[20];
				AddError(&lex.origin, mscn_err_cmdl_no_major_src, 0, _itow(cmdl_task_info.mj_src_pos_param, buff1, 10),
						(cnt_pos_params <= 0) ? L" and there are no pos params on the command line" : buff_num_params);
			}

			if (cond_cnt != 0 && console_data_stt.abort_msg_displayed == FALSE)
			{
				// Some number of the cond parsing layers were not exited.
				wchar_t buff_num_layers[60];
				swprintf(buff_num_layers, 60, L"%d conditional parsing layers are", cond_cnt);
				AddError(&lex.origin, mscn_err_srcst_noemp_condstk, 0, (cond_cnt == 1) ? L"one conditional parsing layer is" : buff_num_layers);
				cond_cnt = 0;
			}

			if (src_stack[src_cnt-1].src_type == srct_mj_src)
			{
				// The major source file was not exited because it is a root of all lex sources. Mark the global state
				// as the major lex source eof.
				if (trace_mask & sctr_major_actions)
				{
					TraceEvent(&lex.origin, TPRF_SCAN_PHASE, L"Setting parsing phase to %s.", g_ParsingPhaseNames[mlph_major_eof]);
				}

				SetPhase(mlph_major_eof);
			}

			// Give this EOF lexema out.
			break;
		}

		if (lex.IsPreprocToken() == TRUE)
		{
			// Process the macro statement that should follow this token.
			assert(((short)lex.num_value) >= mpr_define);
			ProcessMacroDirective(lex);
			continue;
		}

		if (active_code == FALSE)
		{
			// The current code path is passive. Continue scanning.
			continue;
		}

		if (lex.type == ltx_error)
		{
			// Log this error before giving it out. The code below is using the fact that all C/C++ error
			// lexemas always contain a zero terminated string value.
			AddErrorWarning(&lex.origin, 0, lex.error_class, (int)lex.num_value, lex.str_value.m_body);
		}

		if (parsing_phase != mlph_scanning)
		{
			// Current lexema is coming not from the major source file.
			if (lex.type == ltx_comment || lex.type == ltx_eol)
			{
				// Ignore lexemas that cannot change the state of the main parser.
				continue;
			}

			if (prep_opts.allow_mid_lev_lexemas == FALSE)
			{
				// Config is not allowing these lexemas to come out (typical case).
				AddError(&lex.origin, mscn_err_misc_lex_from_cmdl);
				continue;
			}
		}

		// This lexema belongs to one of the active code paths.
		break;
	}

	// Do the tracing.
	if (trace_mask & sctr_scan_exit)
	{
		TraceFuncExitLexema(&lex.origin, TPRF_SCAN, L"Lex", lex);
	}

	// Update the summary.
	if (lex.type != ltx_eof)
		summary.mid_lev_lexemas++;
	return(lex.type);
}

bool TMidLevScanner::ExecRawIndexing(TFileInfoItem *cached_file_info)
{
	assert(parsing_phase == mlph_indexing);
	assert(cached_file_info->data.raw_sess_id == 0);
	return(FALSE);
}

bool TMidLevScanner::ExecRawIndexingForCachedFiles()
{
	assert(parsing_phase == mlph_indexing);
	bool result = TRUE;

	// Iterate list of disk files.
	for (known_files.StartIteration(); known_files.CheckIteration(); known_files.StepIteration())
	{
		if (known_files.CurrIteratedItem()->data.raw_sess_id == 0)
		{
			// This file is not indexed yet.
			result &= ExecRawIndexing(known_files.CurrIteratedItem());
		}
	}

	// Iterate list of temp files.
	for (TListIter<TFileInfoItem> iter(temp_files); iter; ++iter)
	{
		if (iter.CurrItem().data.raw_sess_id == 0)
		{
			// This file is not indexed yet.
			result &= ExecRawIndexing(&iter.CurrItem());
		}
	}

	return(result);
}

void TMidLevScanner::CloseParsingSession(bool reset_disk_files_cache)
{
	if (trace_mask & sctr_major_actions)
	{
		TraceFuncEnter(NULL, TPRF_CL_PARS_SESS, L"CurrPhase=%s.", g_ParsingPhaseNames[parsing_phase]);
	}

	// Check for pending external abort.
	bool parsing_aborted = CheckAbortParsingState();

	// Check whether all lexemas were picked or not.
	if (abort_err_code == 0 && (src_cnt >= 2 || (src_cnt == 1 && src_stack[0].IsInEof() == FALSE)))
	{
		// Abort is not raised and the application has not picked up all lexemas from the lex sources.
		AbortParsingInternal(NULL, mscn_err_phsw_nest_src_unex, lerrc_preproc_err);
	}

	// Save the self changing macro bodies if any.
	for (int i=0; i<mbit_num_types; ++i)
	{
		if (scgm_info[i].file_info != NULL)
			SaveIntermFile(scgm_info[i].file_info);
	}

	// Save the indexing keys use info.
	for (keys_list.StartIteration(); keys_list.CheckIteration(); keys_list.StepIteration())
	{
		TKeyInfoItem *key_info = keys_list.CurrIteratedItem();
		if (key_info->use_cnt != 0 || key_info->use_flags != 0)
		{
			if (cbk_handler->SetParsSessIndexingKeyUseStatus(key_info) == FALSE)
			{
				// Updating the key info failed.
				AddError(NULL, mscn_err_phsw_indx_key_updt, 0, key_info->data.key_name);
			}
		}
	}

	// Exit from the final lex source if any.
	if (src_cnt > 0)
	{
		// This is normal situation because method Scan() is not exiting from the topmost lex source.
		assert(src_cnt == 1);
		PrEventHeader loct;
		GetCurrScanPos(loct);
		ExitFromLexSource(&loct);
	}

	// Close the session in the callback handler.
	ID parsingId = cbk_handler->GetParsingId();
	GetParsingSummary(summary);
	cbk_handler->FinalizeParsing(&prep_opts, &scan_opts_proto, summary, abort_err_code);

	// Close the session in the scanner.
	SetPhase(mlph_none);
	MidLevReset(FALSE, reset_disk_files_cache);

	// Trace the session closing.
	if (trace_mask & sctr_major_actions)
	{
		TraceFuncExit(NULL, TPRF_CL_PARS_SESS, L"Success: ParsingId=%d%s.", parsingId, (parsing_aborted == TRUE) ? L" !!!! Parsing was aborted !!!!" : L"");
	}
}

void TMidLevScanner::CloseRawIndexingSession(bool reset_disk_files_cache, TFileInfoItem *major_sess_file)
{
	assert(parsing_phase == mlph_indexing);
	assert(src_cnt == 0);

	if (trace_mask & sctr_major_actions)
	{
		TraceFuncEnter(NULL, TPRF_CL_RAW_SESS);
	}

	// Close the session in the callback and reset the scanner.
	ID parsingId = cbk_handler->GetParsingId();
	GetRawIndexingSummary(raw_sess_summary);
	cbk_handler->FinishRawIndexingSession(raw_sess_summary, major_sess_file);

	SetPhase(mlph_none);
	MidLevReset(FALSE, reset_disk_files_cache);

	// Trace the session closing.
	if (trace_mask & sctr_major_actions)
	{
		TraceFuncExit(NULL, TPRF_CL_RAW_SESS, L"Success: ParsingId=%d.", parsingId);
	}
}

void TMidLevScanner::GetParsingSummary(TMidLevParsingSummary &buffer)
{
	// Some of the summary counters are not updated on the fly. Update the summary.
	summary.macro_dict_size = macrodefs_list.NumItems();
	summary.different_macro_used = macrodefs_list.GetNumCalledMacros();
	summary.cond_params_num = keys_list.GetNumDifferentCondParams();
	summary.final_cn = latest_cn;

	if (&buffer != &summary)
		buffer = summary;
}

void TMidLevScanner::GetRawIndexingSummary(TRawIndexingSessSummary &buffer)
{
	assert(parsing_phase == mlph_indexing || parsing_phase == mlph_none);

	if (&buffer != &raw_sess_summary)
		buffer = raw_sess_summary;

	buffer.num_parsing_msgs = summary.num_parsing_msgs;
	buffer.num_warnings = summary.num_warnings;
	buffer.num_errors = summary.num_errors;
}

/* public: 1 */

void TMidLevScanner::AddIncludePath(TStringPtr &inc_path_ptr, PrEventHeader *key_area, PrEventHeader *path_area)
{
	assert(inc_path_ptr.GetLength() > 0);

	// Prepare the area of the whole object.
	PrEventHeader object_area, *parea = &object_area;
	if (key_area != NULL)
	{
		// At least one source area is passed.
		object_area = *key_area;
		if (path_area != NULL && key_area->source_ref == path_area->source_ref)
			object_area.ExtendPrHeader(*path_area);
	}
	else
	{
		// There is no source info at all.
		object_area.Clear();
		parea = NULL;
	}

	// This action is allowed only for certain parsing phases.
	if (parsing_phase != mlph_filereg && parsing_phase != mlph_preparing)
		AddWarning(parea, mscn_err_incp_bad_pars_phase);

	// Make the trace.
	if (trace_mask & sctr_new_path)
	{
		wchar_t loc_buff[MAX_PATH];
		TraceFuncEnter(parea, TPRF_ADD_INC_PATH, L"ParsingPhase=%s, Path=\"%s\".",
						g_ParsingPhaseNames[parsing_phase], inc_path_ptr.CopyWithTruncationTo(loc_buff, MAX_PATH));
	}

	// Convert the passed path into the NULL terminated representation.
	TTextBuffer80 inc_path;
	if (ConvertToTextBuffer(parea, inc_path, inc_path_ptr) == FALSE)
	{
		// An error is already reported.
		if (trace_mask & sctr_new_path)
			TraceFuncExit(parea, TPRF_ADD_INC_PATH, L"Failure in the ConvertToTextBuffer().");
		return;
	}

	// Check, if the passed path is appropriate or not.
	TFileNameBuffer absolute_path;
	if (CreateAbsolutePath(parea, absolute_path, inc_path, FALSE) == FALSE)
	{
		// An error is already reported.
		if (trace_mask & sctr_new_path)
			TraceFuncExit(parea, TPRF_ADD_INC_PATH, L"Failure in the CreateAbsolutePath().");
		return;
	}

	// Ensure that there is no slash at the end.
	TPathHelper::RemoveTrailingSlash(absolute_path);

	// Create a substitution if needed.
	TFileNameBuffer substed_path;
	if (CreateSubstedPath(parea, substed_path, absolute_path) == FALSE)
	{
		// Creation failed. Syntax error about this is already reported.
		if (trace_mask & sctr_new_path)
			TraceFuncExit(parea, TPRF_ADD_INC_PATH, L"Failure in the CreateSubstedPath().");
		return;
	}

	// Check if the destination directory is existing or not.
	if (TPathHelper::IsPathADir(substed_path) == FALSE)
	{
		// Add warning and do not discard this include path.
		AddWarning(parea, (substed_path == inc_path) ? mscn_err_incp_dir_missing1 : mscn_err_incp_dir_missing1,
					0, substed_path, inc_path);
	}

	// Check for duplications.
	bool path_duplication = FALSE;
	for (TListIter<TIncludePathItem> iter(inc_paths); iter; ++iter)
	{
		TIncludePathItem &item = iter.CurrItem();
		if (inc_path == item.inc_path)
		{
			// Same path on the command line.
			AddWarning(parea, mscn_err_incp_dupl_inc_path, 0, inc_path);
			path_duplication = TRUE;
		}
		else if (absolute_path == item.inc_directory)
		{
			// Different paths that resulted in the same directory.
			AddWarning(parea, mscn_err_incp_dupl_inc_dir, 0, absolute_path, inc_path, item.inc_path);
			path_duplication = TRUE;
		}
	}

	if (path_duplication == TRUE)
	{
		// Do not record this path.
		if (trace_mask & sctr_new_path)
			TraceFuncExit(parea, TPRF_ADD_INC_PATH, L"Path duplication. The path is not registered.");
		return;
	}

	// Allocate object to keep the data.
	wchar_t *inc_path_dict = misc_strings_dict.RegisterStr(inc_path);
	wchar_t *absolute_path_dict = misc_strings_dict.RegisterStr(absolute_path);
	TIncludePathItem *inc_path_item = NULL;
	if (inc_path_dict != NULL && absolute_path_dict != NULL)
		inc_path_item = new TIncludePathItem(inc_path_dict, absolute_path_dict);

	// Check the allocation results.
	if (inc_path_item == NULL)
	{
		// Note that dict strings do not need releasing and path item was not allocared. So, there is nothing to relase.
		AddError(parea, mscn_err_incp_oom_for_ipath, 0, inc_path);
		if (trace_mask & sctr_new_path)
			TraceFuncExit(parea, TPRF_ADD_INC_PATH, L"OOM. The path is not registered.");
		return;
	}

	// Send the structural callback.
	ID cn = GetNextCN();
	PrNewIncPathEvent info;
	info.hdr = object_area;
	info.hdr.cn = cn;
	info.inc_path = inc_path_dict;
	info.inc_directory  = absolute_path_dict;
	cbk_handler->NewIncludePathNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Add new path to the list and update the summary.
	inc_path_item->inc_path_id = info.hdr.cn;
	inc_paths.AppendItem(inc_path_item);
	summary.inc_paths_num++;

	if (trace_mask & sctr_new_path)
	{
		TraceFuncExit(parea, TPRF_ADD_INC_PATH, L"Success: PathId=%ld, Path=\"%s\".", cn, inc_path.DataPtr());
	}
}

bool TMidLevScanner::UpdateMacroLibrary(TMacroDefinition *macro, TLexema &name_lex, PrEventHeader &token_area)
{
	assert(name_lex.type == ltx_name);

	if (trace_mask & sctr_new_macro)
	{
		wchar_t loc_buff[80];
		TraceFuncEnter(&token_area, TPRF_UPDATE_MLIB,
				L"MacroStt=%s, Nature=%s, NumParams=%d%s, Name=\"%s\".",
				TMacroDefinition::GetStatusName(macro->def_status, FALSE), TMacroDefinition::GetOriginName(macro->def_origin_type, FALSE),
				macro->num_params, ((macro->var_args_macro == TRUE) ? L"+var_args" : L""), macro->def_name.CopyWithTruncationTo(loc_buff, 80));
	}

	// Check, whether a macro with this name is already existing or not.
	TKeyInfoItem *key_info = NULL;
	TMacroDefinition *old_mdef = macrodefs_list.FindRecord(macro);
	if (old_mdef != NULL)
	{
		if (macro->def_origin_type == mdo_builtin || old_mdef->def_origin_type == mdo_builtin)
		{
			// This implenetation prohibits modification of the built in names or conversion of simple name into the builtin name.
			AddError(&name_lex.origin, (macro->def_origin_type == mdo_builtin) ? mscn_err_mlib_creat_builtin : mscn_err_mlib_modif_builtin);
			delete macro;

			if (trace_mask & sctr_new_macro)
			{
				TraceFuncExit(&token_area, TPRF_UPDATE_MLIB, (old_mdef->def_origin_type == mdo_builtin) ?
						L"Failure - attempt to modify the built in macro." : L"Failure - attempt to convert macro into a built in macro.");
			}

			return(FALSE);
		}

		// Pick up the indexing key info from the old definition.
		key_info = old_mdef->key_info;
	}

	if (key_info == NULL)
	{
		// Find the key for this macro.
		key_info = GetIndexingKeyFromLex(name_lex);
		if (key_info == NULL)
		{
			// An error is already reported.
			delete macro;
			if (trace_mask & sctr_new_macro)
			{
				// Make the trace.
				TraceFuncExit(&token_area, TPRF_UPDATE_MLIB, L"Failure - unable to lookup/register the indexing key.");
			}

			return(FALSE);
		}
	}

	if (old_mdef != NULL)
	{
		// Insert previous definition into the history of current macro defn.
		macrodefs_list.RemoveRecord(old_mdef);
		macro->redefinitions_history.TakeDataFrom(old_mdef->redefinitions_history);
		macro->redefinitions_history.PrependItem(old_mdef);
	}

	// Add this definition to the table.
	macro->hdr.cn = GetNextCN();
	macro->key_info = key_info;
	key_info->BumpUsage((GET_DEFINED_STT(macro->def_status) == TRUE) ? mlku_define : mlku_undef);
	macrodefs_list.AddRecord(macro);

	// Send the structural callback.
	PrMacroLibraryChange info;
	info.hdr.Setup(&(macro->hdr), macro->hdr.cn);
	info.macro = macro;
	info.mpr_token = token_area;
	info.name_area = name_lex.origin;
	cbk_handler->MacroLibraryChangeNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Emit non critical errors and warning messages if any.
	if (old_mdef != NULL)
	{
		PrEventHeader *err_ctx = &name_lex.origin;
		if (GET_DEFINED_STT(macro->def_status) == TRUE)
		{
			// Name gets defined. Check appropriate extra constraints.
			if (GET_DEFINED_STT(old_mdef->def_status) == TRUE)
			{
				// Check if these definitions are identical or not.
				if (old_mdef->CompareDefinitions(*macro, &misc_strings_dict) == TRUE)
				{
					if (options.report_macro_redef_to_same_val == TRUE)
						AddWarning(err_ctx, mscn_err_mlib_redef_to_same);
				}
				else
				{
					if (options.report_macro_redef_to_diff_val == TRUE)
						AddError(err_ctx, mscn_err_mlib_redef_to_diff);
				}
			}
		}
		else
		{
			// The name gets undefined.
			if (GET_DEFINED_STT(old_mdef->def_status) == FALSE)
			{
				AddWarning(err_ctx, mscn_err_mlib_undef_twice);
			}
		}

		// Warnings, that do not depend on the new defined state.
		if (old_mdef->def_origin_type == mdo_predefined)
		{
			AddWarning(err_ctx, mscn_err_mlib_modif_predef);
		}
	}

	// Update the summary.
	if (old_mdef == NULL && macro->def_origin_type != mdo_definition)
	{
		// This is new and not simple name.
		if (macro->def_origin_type == mdo_predefined)
			summary.predefined_conds++;
		else summary.built_in_conds++;
	}

	// Trace this fact.
	if (trace_mask & sctr_new_macro)
	{
		TraceFuncExit(&token_area, TPRF_UPDATE_MLIB, L"Success: The macro library was updated.");
	}

	// Success.
	return(TRUE);
}

void TMidLevScanner::ShowRunTimeError(const wchar_t *format, ...)
{
	if (Console().GetAbortFlag() == TRUE)
	{
		// User has already signalled that he/she does not want to see more messages.
		return;
	}

	// Prepare the message.
	va_list vargs;
	va_start(vargs, format);
	wchar_t msg_buffer[2048+80];
	vswprintf(msg_buffer, 2048, format, vargs);
	va_end(vargs);

	if (Console().IsUserResponseSupported() == TRUE)
		wcscat(msg_buffer, L"\r\n\r\nDo you want to continue?\r\n");

	// Display the message and check the result.
	if (Console().ShowRunTimeError(msg_buffer, MB_YESNO) == IDNO)
	{
		// Raise the abort flag.
		Console().SetAbortFlag(TRUE);
	}
}

ID TMidLevScanner::AddErrorWarning(PrEventHeader *src_ctx, ID parent_event, TLexErrorClass err_class, int err_code, const wchar_t *error_message)
{
	assert(parsing_phase != mlph_none);

	// Find out the type of event and update the statistics. Update parsing session summary
	// regardless of the type of the session that is currentrly running.
	wchar_t *prefix, *errno_name;
	WORD trace_msg_color = TGenericConsole::DefHighlightMode;
	if (err_class == lerrc_message)
	{
		prefix = TPRF_PARS_MSG;
		errno_name = L"MsgCode";
		summary.num_parsing_msgs++;
	}
	else if (LEX_ERROR_CLASS_ERROR(err_class) == FALSE)
	{
		prefix = TPRF_WARNING;
		errno_name = L"WarnCode";
		trace_msg_color = TGenericConsole::ErrorHighlightMode;
		summary.num_warnings++;
	}
	else
	{
		prefix = TPRF_SYNT_ERR;
		errno_name = L"ErrCode";
		trace_msg_color = TGenericConsole::ErrorHighlightMode;
		summary.num_errors++;
	}

	// Prepare the change number.
	ID cn = GetNextCN();

	if (trace_mask & sctr_err_warn)
	{
		// Do the tracing.
		curr_trace_attr = trace_msg_color;
		TraceEvent(src_ctx, prefix, L"CN=%02lu, %s=%d, \"%s\".", cn, errno_name, err_code, error_message);
		curr_trace_attr = TGenericConsole::DefHighlightMode;
	}

	// Construct the mid lev event structure.
	PrErrorWarningEvent info;
	PrepareCallbackHeaderSpecial(info.hdr, cn, src_ctx);
	info.parent_event = parent_event;
	info.err_class = err_class;
	info.err_code = err_code;
	info.err_msg = (wchar_t*)error_message;
	info.ext_context.Clear();

	// Send notification to the upper layer.
	cbk_handler->ParsingErrorNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// This action is not passing the major loop where the console update counter is decremented.
	// Update this counter here to avoid big delays between the console updates.
	status_callback_cnt--;
	return(cn);
}

bool TMidLevScanner::CheckAbortParsingState()
{
	if (console_data_stt.abort_msg_displayed == TRUE)
		return(TRUE);

	if (abort_err_code != 0)
	{
		// Application has called AbortParsing(). Process this.
		int err_code = abort_err_code;
		abort_err_code = 0;
		AbortParsingInternal(NULL, err_code, abort_err_class);
		return(TRUE);
	}
	else if (Console().GetAbortFlag() == TRUE)
	{
		// Interactive user wants to abort the parsing process.
		AbortParsingInternal(NULL, mscn_err_misc_aborted_by_user, lerrc_preproc_err);
		return(TRUE);
	}

	return(FALSE);
}

void TMidLevScanner::GetCurrScanPos(PrEventHeader &hdr, ID cn)
{
	// The scan pos is available only when the sources are present.
	assert(parsing_phase != mlph_none);
	assert(src_cnt > 0);

	if (saved_lex.type != ltx_empty)
	{
		// Report current pos at the beg of the saved lexema.
		hdr = saved_lex.origin;
		hdr.AdjustToAreaBeg();
	}
	else
	{
		// Pick up the source area from the current raw scanner. Both scanners return a zero length areas.
		TLexemasSource &lex_src = src_stack[src_cnt-1];
		if (LEX_SRC_CMDL(lex_src.src_type) == TRUE)
			lex_src.cmdl_scanner.GetCurrPos(hdr.src_area);
		else lex_src.scanner.GetCurrPos(hdr.src_area);

		// Add the mid lev fields.
		SetupMidLevOrigin(hdr, lex_src);
	}

	// Setup the CN exactly as it is passed.
	hdr.cn = cn;
}

long TMidLevScanner::GetCurrentLinearOffset()
{
	assert(parsing_phase != mlph_none);

	if (src_cnt <= 0)
	{
		return(summary.final_linear_offs);
	}
	else
	{
		PrEventHeader hdr;
		GetCurrScanPos(hdr);
		return(hdr.lin_area.AreaBeg());
	}
}

TMacroDefinition *TMidLevScanner::CloneMacro(TMacroDefinition *proto)
{
	TMacroDefinition *macro = cbk_handler->CreateMacroDefinition(proto->def_name, proto->def_status, proto->def_origin_type);
	if (macro == NULL)
		return(NULL);

	// Copy the rest of the scalar fields.
	macro->hdr				= proto->hdr;
	macro->def_blt_in_type	= proto->def_blt_in_type;
	macro->num_params		= proto->num_params;
	macro->var_args_macro	= proto->var_args_macro;
	for (int i=0; i<proto->num_params; ++i)
		macro->def_param_name[i] = proto->def_param_name[i];

	// Clone the body of the macro.
	macro->macro_body = proto->macro_body;
	macro->scst_body_lex = proto->scst_body_lex;

	// The macro history is not cloned.
	return(macro);
}

TKeyInfoItem *TMidLevScanner::GetIndexingKey(PrEventHeader *err_ctx, TKeyType key_type, TStringPtr &name, ID &err_ident, bool &malloc_problem)
{
	// Lookup the passed name in the hash table first. Note that lookup item is created using special
	// ctor that takes TStringPtr argument and expects that sting is not NULL terminated there.
	TKeyInfoItem dummy(name);
	TKeyInfoItem *item = keys_list.FindRecord(&dummy);
	if (item != NULL)
	{
		// A key item for this name is already present.
		if (trace_mask & sctr_new_key)
		{
			wchar_t loc_buff[80];
			TraceEvent(err_ctx, TPRF_NEW_KEY, L"Existing key: Id=%lu, Name=\"%s\".", item->data.key_id, name.CopyWithTruncationTo(loc_buff, 80));
		}

		return(item);
	}

	// Move the passed string to the heap and create an object for storing info about this key.
	wchar_t *name_dict = misc_strings_dict.RegisterStr(name.GetBodyPtr(), name.GetLength());
	TKeyInfoItem *new_item = NULL;
	if (name_dict != NULL)
		new_item = cbk_handler->CreateKeyInfoItem(key_type, name_dict);

	// Check the results of allocation.
	if (new_item == NULL)
	{
		wchar_t loc_buff[80];
		name.CopyWithTruncationTo(loc_buff, 80);

		AddOutOfMemError(err_ctx, mloom_reg_inx_key_loc1, loc_buff);
		malloc_problem = TRUE;

		if (trace_mask & sctr_new_key)
		{
			TraceEvent(err_ctx, TPRF_NEW_KEY, L"Failure - cannot create TKeyInfoItem, Name=\"%s\".", loc_buff);
		}

		return(NULL);
	}

	// Register this key with the database.
	if (cbk_handler->LookupOrRegisterIndexingKey(*new_item) == FALSE)
	{
		err_ident = mscn_err_misc_indexing_key_reg;
		AddError(err_ctx, err_ident, 0, name_dict);
		delete new_item;

		if (trace_mask & sctr_new_key)
		{
			TraceEvent(err_ctx, TPRF_NEW_KEY, L"Failure - cannot register key, Name=\"%s\".", name_dict);
		}

		return(NULL);
	}

	// Ensure that the database has assigned a non zero key id.
	assert(new_item->data.key_id != 0);

	if (trace_mask & sctr_new_key)
	{
		TraceEvent(err_ctx, TPRF_NEW_KEY, L"Key created: Id=%lu, Name=\"%s\".", new_item->data.key_id, name_dict);
	}

	// Full succeess. Add this object to the table.
	keys_list.AddRecord(new_item);
	return(new_item);
}

TKeyInfoItem *TMidLevScanner::GetIndexingKeyFromLex(TLexema &lex)
{
	ID err_ident = 0;
	bool malloc_problem = FALSE;
	return(GetIndexingKeyFromLexEx(lex, err_ident, malloc_problem));
}

TKeyInfoItem *TMidLevScanner::GetIndexingKeyFromLexEx(TLexema &lex, ID &err_ident, bool &malloc_problem)
{
	//
	// This method is not fully public because of the err_ident and the malloc_problem params.
	// Caller is responsible for setting appropriate initial values that can be either zero/FALSE or
	// something else depending on the situatuion.
	//

	// Ensure that the passed lexema has a correct type.
	assert(lex.type == ltx_number || lex.type == ltx_charconst || lex.type == ltx_name);

	// Prepare the name of the key.
	TKeyType ktype;
	TStringPtr name;
	wchar_t name_buffer[60];
	if (lex.type == ltx_number)
	{
		// The name of the key needs to be prepared.
		FormatInt64(lex.num_value, name_buffer, 60, (LEX_NUM_SUBT_UNSIGEND(lex.number_type) == TRUE) ? fnms_dec_unsigned : fnms_dec_signed);
		ktype = kt_number;
		name.SetData(name_buffer);
	}
	else if (lex.type == ltx_charconst)
	{
		// Prepare the name of the key as a hex integer.
		FormatInt64(lex.num_value, name_buffer, 60, fnms_hex);
		ktype = kt_number;
		name.SetData(name_buffer);
	}
	else
	{
		// Pick up the props of the name lexema.
		ktype = kt_name;
		name = lex.str_value;
	}

	// Call the worker method.
	return(GetIndexingKey(&lex.origin, ktype, name, err_ident, malloc_problem));
}

const wchar_t *TMidLevScanner::GetErrorText(int err_code)
{
	//
	// Note that error messages can have only "%s" param specifiers.
	//
	switch (err_code)
	{

	// Errors while accepting the file proc params.
	case mscn_err_subst_no_base_dir:			return(L"The current directory should be specified explicitly when the list of drive substitutions is not empty.");
	case mscn_err_subst_interm_paths:			return(L"Options should not allow intermediate paths when the list of drive substitutions is not empty.");
	case mscn_err_subst_curr_dir_err1:			return(L"GetCurrentDirectory() failure: %s (dec).");
	case mscn_err_subst_curr_dir_err2:			return(L"GetCurrentDirectory() requires buffer with the zise: %s (dec).");
	case mscn_err_subst_base_not_abs:			return(L"The base parsing directory is not absolute: [%s].");

	// Errors while parsing command line and keys.
	case mscn_err_cmdl_no_app_name:			return(L"The name of the compiler application is missing on the command line.");
	case mscn_err_cmdl_no_major_src:			return(L"The name of the major source file is missing. The requested param is %s%s.");
	case mscn_err_cmdl_no_prep_phase:			return(L"Command line keys shoud stay before the source file names. Current phase: \"%s\".");
	case mscn_err_cmdl_pcnd_no_name:			return(L"The name of the predefined condition is missing.");
	case mscn_err_cmdl_oom_for_pcnd:			return(L"Error allocating storage for a predefined condition (macro) record.");
	case mscn_err_cmdl_pcnd_bad_delim:		return(L"Bogus lexema after the name of the predefined condition.");
	case mscn_err_cmdl_bad_atkey_src:			return(L"Analysis tool key is present in the wrong type of the source file. The key is ignored.");
	case mscn_err_cmdl_unkn_atool_key:		return(L"Unknown analysis tool key value.");

	// Errors while locating and loading include files.
	case mscn_err_ihlp_abs_sys_fname:			return(L"Absolute path in the %s statement that refers to the system directory.");
	case mscn_err_ihlp_abs_user_fname:			return(L"Absolute path in the %s statement.");
	case mscn_err_ihlp_path_semi_rel:			return(L"The file name is using current directory on the non current drive or the current drive.");
	case mscn_err_ihlp_bad_intermp:			return(L"Unable to resolve an intermediate path. Win32 error: %s.");
	case mscn_err_ihlp_long_intermp:			return(L"Unable to resolve an intermediate path. The length of the path is too big: %s.");
	case mscn_err_ihlp_upper_sys_ref:			return(L"Reference to an upper directory of the system directory.");
	case mscn_err_ihlp_file_not_found:			return(L"Error loading include file (file not found).");
	case mscn_err_ihlp_make_fname1:			return(L"Error generating the name of the file to load while using the include path directory [%s].");
	case mscn_err_ihlp_make_fname2:			return(L"Error generating file name of the file to load from the directory of another file [%s].");
	case mscn_err_ihlp_curr_dir_load:			return(L"Loading file from the directory of the calling file without using any include path.");
	case mscn_err_ihlp_file_load_error:			return(L"Error loading include file (%s) from [%s].");
	case mscn_err_ihlp_other_path_loc:			return(L"The file, that was taken from [%s], is also present at [%s] where it can be accessed using a different include path.");
	case mscn_err_ihlp_short_name_also1:		return(L"The file was taken from [%s]. This file can also be taken from this location if loading it with a short name.");
	case mscn_err_ihlp_short_name_also2:		return(L"The file was taken from [%s]. The file can also be found at [%s] if loading it with a short name.");
	case mscn_err_ihlp_short_name_cload:		return(L"The include statement contains a path. If trying to load this file by its short name, it can be found at [%s].");

	// Errors while switching between parsing phases.
	case mscn_err_phsw_nest_src_unex:			return(L"Application has not retrieved all lexemas from the lexema sources.");
	case mscn_err_phsw_indx_key_updt:			return(L"Error updating indexing key status for: [%s].");

	// Errors in setting base dir and include paths.
	case mscn_err_incp_oom_for_ipath:			return(L"Error allocating storage for an include path descriptor for [%s].");
	case mscn_err_incp_semi_rel_dir:			return(L"The include path is semi relative [%s]. Current options do not allow using such paths.");
	case mscn_err_incp_semi_rel_file:			return(L"The name of the file is semi relative [%s]. Current options do not allow using such file names.");
	case mscn_err_incp_append_relat:			return(L"Error appending the path [%s] to the base directory [%s].");
	case mscn_err_incp_interm_path_conv:		return(L"Error getting absolute path from [%s]. Win32 error: %s.");
	case mscn_err_incp_bad_pars_phase:		return(L"Include path is added not during the parsing preparation phase.");
	case mscn_err_incp_dir_missing1:			return(L"The include directory [%s] is missing.");
	case mscn_err_incp_dir_missing2:			return(L"The directory [%s] is missing. It was derived using the path [%s].");
	case mscn_err_incp_dupl_inc_path:			return(L"Duplication of the include path [%s]. The include path is ignored.");
	case mscn_err_incp_dupl_inc_dir:			return(L"Duplicaton of the include directory [%s]. Path1: [%s]. Path2: [%s].");
	case mscn_err_incp_subst_missing:			return(L"Unable to find substitution for the path [%s] while options require that all paths should be substituted.");

	// Core scanning plus nesting new lex sources.
	case mscn_err_srcst_file_not_found1:			return(L"Error loading file: [%s]. File not found.");
	case mscn_err_srcst_file_not_found2:			return(L"Error loading file: [%s]. File not found. The substituted name is [%s].");
	case mscn_err_srcst_file_load_error1:		return(L"Error loading file (%s) from disk [%s].");
	case mscn_err_srcst_file_load_error2:		return(L"Error loading file (%s) from disk [%s]. The substituted name is [%s].");
	case mscn_err_srcst_noemp_condstk:		return(L"Unexpected end of file when the %s not closed.");						// The msg is fime. The inInsert is complex hre.
	case mscn_err_srcst_oom_for_fitem:			return(L"Error allocating storage for the file cache descriptor.");
	case mscn_err_srcst_oom_for_ifbody:		return(L"Error allocating storage for the intermediate file body.");
	case mscn_err_srcst_cant_reg_file:			return(L"Unable to lookup or assign a file id to [%s].");
	case mscn_err_srcst_date_len_mism:			return(L"Callback handler rejected registering the file (NULL fid): [%s].");
	case mscn_err_srcst_nesting_deep1:			return(L"The include files nesting is too deep. Include files, macro calls and macro params share the same stack.");
	case mscn_err_srcst_pr_once_in_mjsrc:		return(L"The pragma once statement flag is already set on the major source file. The major source file is dismissed.");
	case mscn_err_srcst_nesting_deep2:			return(L"The macro calls nesting is too deep. Include files, macro calls and macro params share the same stack.");
	case mscn_err_srcst_nesting_deep3:			return(L"The macro params nesting is too deep. Include files, macro calls and macro params share the same stack.");
	case mscn_err_srcst_nesting_deep4:			return(L"The paste buffering nesting is too deep. Include files, macro calls and macro params share the same stack.");
	case mscn_err_srcst_single_hash_sym:		return(L"The hash symbol (\"#\") is not followed by any macroprocessor statement keyword.");
	case mscn_err_srcst_inln_macro_token:		return(L"Inline macro operations are not allowed in the file context.");
	case mscn_err_srcst_stmt_macrotkn:			return(L"Macro directives are not allowed in the macro context.");
	case mscn_err_srcst_varg_bad_ctx:			return(L"Var args cannot be invoked from a non var args macro context.");
	case mscn_err_srcst_preproc_notlbeg:		return(L"The preprocessor directive does not start from the beginning of the line.");
	case mscn_err_srcst_preproc_other:			return(L"The preprocessor directive with the token value %s is not supported. The statement is ignored.");

	// Paste buffering related errors.
	case mscn_err_pbuff_eof_suff_nob:			return(L"EOF with the paste suffix after the call while there is no active paste buffering.");
	case mscn_err_pbuff_oom_for_pbuff:			return(L"Out of memory while allocating a paste buffer.");
	case mscn_err_pbuff_nesting_deep:			return(L"The paste buffering nesting is too deep. The nesting limit is %s.");
	case mscn_err_pbuff_quote_failure:			return(L"Out of memory while adding a quote to the paste buffer.");
	case mscn_err_pbuff_delim_failure:			return(L"Out of memory while adding a space delimiter to the paste buffer.");
	case mscn_err_pbuff_lexema_failure:			return(L"Out of memory while adding a lexema to the paste buffer.");
	case mscn_err_pbuff_stail_failure:			return(L"Out of memory while adding an unscanned part of the current lexema source to the paste buffer.");
	case mscn_err_pbuff_esc_failure:			return(L"Out of memory while escaping single or double quotes in the paste buffer.");
	case mscn_err_pbuff_cnct_lex_first:			return(L"Concatenation operator (##) cannot be the first lexema in the macro definition or in the macro param.");
	case mscn_err_pbuff_cnct_lex_last:			return(L"Concatenation operator (##) cannot be the last lexema in the macro definition or in the macro param.");
	case mscn_err_pbuff_str_chr_param:			return(L"The preprocessor operator (# or #@) is not followed by the name of the macro parameter.");

	// Errors while updating the macro library.
	case mscn_err_mlib_creat_builtin:			return(L"Attempt to convert simple macro into a builtin macro. The statement is ignored.");
	case mscn_err_mlib_modif_builtin:			return(L"Modification of the builtin macro. The statement is ignored.");
	case mscn_err_mlib_redef_to_same:			return(L"Redefinition of existing macro to the same value.");
	case mscn_err_mlib_redef_to_diff:			return(L"Redefinition of existing macro to a different value.");
	case mscn_err_mlib_undef_twice:			return(L"Duplication of the macro undefinition.");
	case mscn_err_mlib_modif_predef:			return(L"Modification of the predefined macro. The statement is NOT ignored.");

	// Errors in #define and #undef statements.
	case mscn_err_mdef_name_token:			return(L"The keyword \"%s\" is used as the name of the macro in the %s statement. The statement is ignored.");
	case mscn_err_mdef_no_macro_name:		return(L"The name of the macro is missing.");
	case mscn_err_mdef_oom_for_macro:		return(L"Error allocating storage for the macro definition record.");
	case mscn_err_mdef_no_mprm_lst_end:		return(L"An end of the macro parameters list is missing.");
	case mscn_err_mdef_no_mprm_name:		return(L"The name of the macro parameter is missing.");
	case mscn_err_mdef_no_vargs_paren:		return(L"Bogus lexema after the triple dots in the macro definition header.");
	case mscn_err_mdef_mprm_name_dupe:		return(L"Duplication of the macro parameter name.");
	case mscn_err_mdef_too_many_mprms:		return(L"Macro definition has too many parameters. Only %s parameters are allowed.");
	case mscn_err_mdef_no_mprm_separ:		return(L"The macro parameter names delimiter (comma) is missing.");
	case mscn_err_mdef_mprm_name_token:		return(L"Keyword \"%s\" cannot be used as the name of the macro parameter. The parameter is ignored.");
	case mscn_err_mdef_mprm_lst_error:		return(L"Error in the list of parameters of the macro.");
	case mscn_err_mdef_prstkn_in_mdef:		return(L"Preprocessor statement inside the macro definition.");

	// Misc issues while scanning.
	case mscn_err_misc_bad_fsys_setup:			return(L"Bogus value of the built in file id: %s.");
	case mscn_err_misc_bltin_bad_fid:			return(L"Unknown built in file id: %s.");
	case mscn_err_misc_aborted_by_user:		return(L"Aborted by the user.");
	case mscn_err_misc_prst_extra_lex:			return(L"Unexpected lexema before the end of the line in the macroprocessor statement.");
	case mscn_err_misc_not_impl_yet:			return(L"Not implemented yet.");
	case mscn_err_misc_out_of_memory:		return(L"Out of memory in location %s, the current context is \"%s\".");
	case mscn_err_misc_indexing_key_reg:		return(L"Error registering the indexing key \"%s\".");
	case mscn_err_misc_atool_env_miss:			return(L"The analysis tool profile [%s] is missing.");
	case mscn_err_misc_lex_from_cmdl:			return(L"Current config is not allowing lexemas to come not from the major source file.");

	// Errors while scanning the macro call.
	case mscn_err_mcall_eol_in_mcall:			return(L"End of line in the middle of the macro call. Use \"\\\" to continue on the next line.");
	case mscn_err_mcall_eof_in_mcall:			return(L"End of file or end of the macro definition in the middle of the macro call.");
	case mscn_err_mcall_mprm_not_foll:			return(L"The macro parameter is not followed by the comma delimiter.");
	case mscn_err_mcall_varg_not_foll:			return(L"The var args macro parameter is not followed by the comma delimiter.");
	case mscn_err_mcall_varg_bad_ctx:			return(L"A var args token is detected outside of the var args macro context.");
	case mscn_err_mcall_err_adding:			return(L"There is no more room for adding the macro call parameters.");
	case mscn_err_mcall_more_actual:			return(L"The number of params in the macro call (%s) exceeds the number of params in the macro definition (%s).");
	case mscn_err_mcall_less_actual:			return(L"The number of params in the macro call (%s) is less than the number of params in the macro definition (%s).");

	// Errors in cond parsing statements.
	case mscn_err_cond_nesting_deep:			return(L"The conditional parsing nesting is too deep. Current configuatrion allows only %s nesting layers.");
	case mscn_err_cond_stack_underflow:		return(L"Unexpected #elif, #else or #endif statement.");
	case mscn_err_cond_elif_after_else:			return(L"The #elif statement stays after the #else statement on the same level of the conditional parsing nesting.");
	case mscn_err_cond_second_else:			return(L"More that one #else statement on the same conditional parsing nesting level.");
	case mscn_err_cond_ifdef_no_name:			return(L"A name to test is missing after the macroprocessor statement token.");
	case mscn_err_cond_oom_for_code:			return(L"Out of memory while scanning the conditional expression.");
	case mscn_err_cond_rdef_cond_used:		return(L"Redefined condition is used in the defined(x) operation.");
	case mscn_err_cond_unkn_name_used:		return(L"Unknown name in the #ifdef, #ifndef or defined(x) operation.");
	case mscn_err_cond_oom_for_clone:			return(L"Out of memory while processing the conditional expression.");

	// Errors and messages from other preproc stmts.
	case mscn_err_ostm_incl_no_fname:			return(L"The name of the include file is missing.");
	case mscn_err_ostm_pragma_no_type:		return(L"The type of the #pragma statement is missing.");
	case mscn_err_ostm_pragma_bad_once:		return(L"Parsing has reached the \"#pragma once\" statement that resides not in a regular disk file.");
	case mscn_err_ostm_pragma_once_dup:		return(L"Parsing has reached the \"#pragma once\" statement while this flag for the current file is already set.");
	case mscn_err_ostm_pragma_note:			return(L"Parsing has reached the \"#pragma note\" statement.");
	case mscn_err_ostm_error_stmt:			return(L"Parsing has reached an #error statement.");
	case mscn_err_ostm_import_no_fname:		return(L"The name of the file is missing in the #import statement.");
	case mscn_err_ostm_line_no_fname:			return(L"The name of the file is expected in the #line statement.");
	case mscn_err_ostm_line_no_lnum:			return(L"The line number is missing in the #line statement.");
	case mscn_err_ostm_using_no_fname:		return(L"The name of the file is missing in the #using statement.");

	// Errors while scanning conditional expressions.
	case mscn_err_cexpr_empty_lexema:		return(L"Empty lexema inside the conditional expression.");
	case mscn_err_cexpr_64bit_negnum:			return(L"Unsigned 64 bit arithmetic is not supported in the conditional expressions.");
	case mscn_err_cexpr_128bit_number:		return(L"The 128 bit numbers are not supported in the conditional expressions.");
	case mscn_err_cexpr_64bit_charconst:		return(L"Character constants with the 64 bit size of the value are not supported in the conditional expressions.");
	case mscn_err_cexpr_foating_point_lexema:	return(L"Floating point constant inside the conditional expression.");
	case mscn_err_cexpr_string_lexema:			return(L"String lexema inside the conditional expression.");
	case mscn_err_cexpr_operation_missing:		return(L"Operation is missing after the operand.");
	case mscn_err_cexpr_unary_op_error:		return(L"Prefix unary operation after the operand.");
	case mscn_err_cexpr_lpar_no_operand1:		return(L"An operand is missing after the opening parenthesis.");
	case mscn_err_cexpr_no_oprnd_op:			return(L"An operand is missing before the operation.");
	case mscn_err_cexpr_no_oprnd_bop:			return(L"An operand is missing between the operations.");
	case mscn_err_cexpr_no_oprnd_rpar:		return(L"An operand is missing before the closing parenthesis.");
	case mscn_err_cexpr_rpars_than_lpars:		return(L"The number of closing parenthesis is bigger than the number of opening parenthesis.");
	case mscn_err_cexpr_unkn_operation:		return(L"Unknown conditional expression operation.");
	case mscn_err_cexpr_defnd_params:			return(L"Error in the parameter of the defined(x) operation.");
	case mscn_err_cexpr_defnd_no_rpar:			return(L"A closing parenthesis is missing after the parameter of the defined(x) operation.");
	case mscn_err_cexpr_unkn_mparams:		return(L"Error getting parameters of the unknown macro in the conditional expression.");
	case mscn_err_cexpr_no_operation:			return(L"An operation is missing between the operands.");
	case mscn_err_cexpr_expr_missing:			return(L"The conditional expression is missing.");
	case mscn_err_cexpr_lpar_no_operand2:		return(L"An operand is missing after the opening parenthesis.");
	case mscn_err_cexpr_unop_no_operand:		return(L"An operand is missing after the unary operation.");
	case mscn_err_cexpr_biop_no_operand:		return(L"An operand is missing after the binary or ternary operation.");
	case mscn_err_cexpr_no_operand:			return(L"The operand is missing.");
	case mscn_err_cexpr_expr_uncomplete:		return(L"The expression is uncomplete.");
	case mscn_err_cexpr_out_of_memory:		return(L"Low on memory while scanning the conditional expression.");
	case mscn_err_cexpr_stack_overflow1:		return(L"The parenthesis are nested too deep or the conditional expression is too long (operation).");
	case mscn_err_cexpr_stack_overflow2:		return(L"The parenthesis are nested too deep or the conditional expression is too long (operand).");
	case mscn_err_cexpr_resv_unopr:			return(L"An operand for resolving the unary operation is missing.");
	case mscn_err_cexpr_resv_condopr1:		return(L"The conditional operation is uncomplete. The colon part is missing.");
	case mscn_err_cexpr_resv_condopr2:		return(L"The conditional operation is uncomplete.");
	case mscn_err_cexpr_resv_biop:				return(L"Operands for resolving the binary operation are missing.");
	case mscn_err_cexpr_resv_unkn_op:			return(L"Unknown operation in the internal stack of the conditional compilation expression parser.");
	case mscn_err_cexpr_redef_predef_used:		return(L"Redefined predefined condition is used in the conditional expression.");
	case mscn_err_cexpr_known_undef:			return(L"Undefined (but known) name is used as an operand in the conditional expression.");
	case mscn_err_cexpr_unknown_name:		return(L"Unknown name in the conditional expression.");

	}

	// Unknown error code.
	return(L"TMidLevScanner: Unknown error/warning code.");
}

bool TMidLevScanner::GetFileOffsInfo(TFileInfo *finfo, long loc_offs, long &line_num, long &char_pos, wchar_t **line_beg, long *line_len)
{
	assert(finfo != NULL);

	// Ensure that the passed file has lines info data.
	if (finfo->CheckLinesInfo() == FALSE)
	{
		// Build the missing lines info.
		if (finfo->BuildLinesInfo() == FALSE)
		{
			// Allocation has failed.
			return(FALSE);
		}
	}

	// Find line descriptor for the passed file offset.
	TSimpleLineInfo *linfo = finfo->GetLineInfoFromFileOffset(loc_offs);
	if (linfo == NULL)
		return(FALSE);

	// Char offset cannot be bigger than the line length.
	line_num = finfo->GetLineNumber(linfo);
	char_pos = loc_offs-linfo->line_offs+1;

	if (line_beg != NULL && line_len != NULL)
	{
		// Give out the info about the line.
		*line_beg = finfo->info.file_body+linfo->line_offs;
		*line_len = linfo->line_len;
	}

	// Success.
	return(TRUE);
}

void TMidLevScanner::PrepareCallbackHeader(PrEventHeader &hdr, ID cn, PrEventHeader *ctx)
{
	assert(parsing_phase != mlph_none);

	if (ctx != NULL)
	{
		// Once the context is passed, it should be non empty.
		assert(ctx->source_ref != 0);
		hdr = *ctx;
	}
	else if (src_cnt > 0)
	{
		// Source ctx is available.
		GetCurrScanPos(hdr);
	}
	else
	{
		// Prepare a header that has no relation to the source code.
		hdr.Clear();
	}

	// Use the passed CN directly.
	hdr.cn = cn;
}

void TMidLevScanner::PrepareCallbackHeaderSpecial(PrEventHeader &hdr, ID cn, PrEventHeader *context)
{
	assert(parsing_phase != mlph_none);

	if (context == NULL || context->source_ref == 0)
	{
		// The event context is not passed or it is empty.
		if (src_cnt > 0)
		{
			// Fill header with the current scanning position.
			GetCurrScanPos(hdr);
		}
		else
		{
			// Reset the header.
			hdr.Clear();
		}
	}
	else
	{
		// The context is passed. It can be passed only when the lex sources are available.
		assert(src_cnt > 0 && context->lin_area.linear_beg > 0);
		hdr = *context;
	}

	// Use the passed CN directly.
	hdr.cn = cn;
}

/* protected: 2 */

static TConditionalStatus g_CodeStatusFromOldLayerAndNewLayer[cexr_num_types][cexr_num_types] =
{
	// History: hard_FALSE
	{  cexr_hard_false,		cexr_hard_false,	cexr_hard_false,	cexr_hard_false,	cexr_hard_false	},

	// History: hard_TRUE
	{  cexr_hard_false,		cexr_hard_true,	cexr_soft_false,	cexr_soft_true,	cexr_undefined	},

	// History: soft_FALSE
	{  cexr_hard_false,		cexr_soft_false,	cexr_soft_false,	cexr_soft_false,	cexr_soft_false	},

	// History: soft_TRUE
	{  cexr_hard_false,		cexr_soft_true,	cexr_soft_false,	cexr_soft_true,	cexr_undefined	},

	// History: Undefined
	{  cexr_hard_false,		cexr_undefined,	cexr_soft_false,	cexr_undefined,	cexr_undefined	},
};

void TMidLevScanner::SetupIntegralCondParsingState(int cond_pars_stack_len)
{
	active_code = TRUE;
	conditional_area = FALSE;
	cond_pars_status = cexr_hard_true;

	for (int inx=0; inx<cond_pars_stack_len; ++inx)
	{
		if (GET_ACTIVE_STATUS(cond_stk[inx].code_stts) == FALSE)
			active_code = FALSE;

		if (GET_SURE_STATUS(cond_stk[inx].code_stts) == FALSE)
			conditional_area = TRUE;

		cond_pars_status = g_CodeStatusFromOldLayerAndNewLayer[cond_pars_status][cond_stk[inx].code_stts];
	}
}

void TMidLevScanner::AbortParsingInternal(PrEventHeader *abort_location, int err_code, TLexErrorClass err_class)
{
	if (console_data_stt.abort_msg_displayed == TRUE)
	{
		// The abort state is already declared.
		assert(FALSE);
		return;
	}

	if (trace_mask & sctr_major_actions)
	{
		// Make the trace.
		TraceEvent(abort_location, TPRF_ABORT_PARS, L"ErrorCode=%d.", err_code);
	}

	// Record the state and send an error.
	assert(abort_err_code == 0);
	abort_err_code = err_code;
	abort_err_class = err_class;
	AddErrWarnInternal(abort_location, 0, err_class, err_code, NULL, NULL, NULL);

	// Exit only from the nested lex sources. The root lex source should remain at the place
	// where it is now to allow setting proper location for futher errors and messages.
	assert(pbuff_cnt == 0);
	while (src_cnt > 1)
	{
		// Pick up the current scanning location.
		PrEventHeader loct;
		GetCurrScanPos(loct);
		ExitFromLexSource(&loct);
		saved_lex.SetEmpty();
	}

	saved_lex.Clear();
	include_fname_mode = FALSE;

	// The current state is equivalent to the "Major EOF" now.
	SetPhase(mlph_major_eof);

	// Set the abort flags in console.
	Console().SetMajorStatus(L"Aborting ...");
	UpdateConsoleInternal(TRUE);
	console_data_stt.abort_msg_displayed = TRUE;
}

ID TMidLevScanner::AddErrWarnInternal(PrEventHeader *ctx, ID parent_event, TLexErrorClass err_class, int err_code,
										const wchar_t *ext1, const wchar_t *ext2, const wchar_t *ext3)
{
	// Construct the error message.
	wchar_t msg_buffer[2048];
	wchar_t *err_msg = (wchar_t*)GetErrorText(err_code);
	if (ext1 != NULL)
	{
		// Error message should have at least one extended parameter.
		assert(wcschr(err_msg, L'%') != NULL);
		swprintf(msg_buffer, 2048, err_msg, ext1, ext2, ext3);
		err_msg = msg_buffer;
	}

	// Call the public worker method.
	return(AddErrorWarning(ctx, parent_event, err_class, err_code, err_msg));
}

TFileInfoItem *TMidLevScanner::RegisterNewFile(PrEventHeader *error_context, TFileInfo &finfo)
{
	// Bypassed file should not be present in the cache.
	assert(finfo.file_id == 0 || finfo.strg_type == strg_builtin);

	// Create the file descriptor with the help of the mid lev callback. This place is the only one place
	// in the code where the CreateFileInfoItem() is called.
	TFileInfoItem *fdesc = cbk_handler->CreateFileInfoItem(finfo);
	if (fdesc == NULL)
	{
		AddError(error_context, mscn_err_srcst_oom_for_fitem);
		if (trace_mask & sctr_new_file)
			TraceEvent(error_context, TPRF_REG_NEW_FILE, L"Failure - cannot create TFileInfoItem.");
		return(NULL);
	}

	// Intermediate files require second step of creation.
	if (finfo.strg_type == strg_interm)
	{
		// Caller should pass neither a file name nor a body. Ensure this.
		assert(finfo.info.file_name == NULL);
		assert(finfo.info.file_body == NULL);

		// Create a body for the file.
		fdesc->interm_file_body = new TIntermFileBody(fdesc, error_context);
		if (fdesc->interm_file_body == NULL)
		{
			AddError(error_context, mscn_err_srcst_oom_for_ifbody);
			delete fdesc;

			if (trace_mask & sctr_new_file)
				TraceEvent(error_context, TPRF_REG_NEW_FILE, L"Failure - cannot create TIntermFileBody.");
			return(NULL);
		}
	}

	// Save descriptor in the appropriate table.
	TFileStorageType strg = finfo.strg_type;
	if (strg == strg_disk || strg == strg_builtin || strg == strg_proto)
	{
		// Register the file or retrieve the file_id from the callback handler. This call will also add reference from the file
		// to the current parsing session in the database.
		if (cbk_handler->LookupOrRegisterDiskFileInfo(finfo) == FALSE)
		{
			AddError(error_context, mscn_err_srcst_cant_reg_file, 0, finfo.info.file_name);

			// Zero out file name and file body pointers to prevent double freeing.
			fdesc->data.info.file_name = fdesc->data.info.file_body = NULL;
			delete fdesc;

			if (trace_mask & sctr_new_file)
				TraceEvent(error_context, TPRF_REG_NEW_FILE, L"Failure in cbk->LookupOrRegisterDiskFileInfo().");
			return(NULL);
		}

		// Check the response from the callback handler.
		if (finfo.file_id == 0)
		{
			AddError(error_context, mscn_err_srcst_date_len_mism, 0, finfo.info.file_name);

			// Zero out file name and file body pointers to prevent double freeing.
			fdesc->data.info.file_name = fdesc->data.info.file_body = NULL;
			delete fdesc;

			if (trace_mask & sctr_new_file)
				TraceEvent(error_context, TPRF_REG_NEW_FILE, L"Failure in cbk->LookupOrRegisterDiskFileInfo(). It returned NULL file_id.");
			return(NULL);
		}

		// Save the file id and info about the raw indexing, returned by the callback or passed by
		// the caller in case of the built in file.
		fdesc->data.file_id = finfo.file_id;
		fdesc->data.raw_sess_id = finfo.raw_sess_id;

		// Everything went fine. Add descriptor to the files cache.
		TFileInfoItem *fd2 = known_files.AddRecord(fdesc);
		assert(fd2 == fdesc);
		summary.files_cache_size++;
	}
	else if (strg == strg_temp || strg == strg_interm)
	{
		// Assign an Id to the file.
		ID file_id = GetNextCN();
		fdesc->data.pars_id = finfo.pars_id = cbk_handler->GetParsingId();
		fdesc->data.file_id = finfo.file_id = file_id;

		if (strg == strg_temp)
		{
			// Notify the callback about this file.
			PrTempFileRegistration reg_info;
			reg_info.hdr.Setup(error_context, file_id);
			reg_info.file_info = fdesc->data;
			cbk_handler->RegisterTempFileData(&reg_info);
			PostProsessMidLevCbkCall(reg_info.hdr);
		}
		else
		{
			// Setup the name of the intermediate file. This will also save the file_id into the file creation
			// context that is part of the interm file body.
			fdesc->interm_file_body->RefreshFileIdent();
		}

		// Save descriptor in the list of the temp files.
		temp_files.AppendItem(fdesc);
		if (strg == strg_temp)
			summary.temp_files_num++;
		else summary.interm_files_num++;
	}
	else
	{
		// Unknown file storage type.
		assert(FALSE);

		if (trace_mask & sctr_new_file)
		{
			TraceEvent(error_context, TPRF_REG_NEW_FILE, L"Failure - unknown file storage type: %d.", (int)strg);
		}

		return(NULL);
	}

	// The TFileInfoItem has taken ownership over the file name and file body. Clear these pointers in
	// the param of the current method to avoid double freeing.
	finfo.info.file_name = finfo.info.file_body = NULL;

	if (trace_mask & sctr_new_file)
	{
		wchar_t file_id_buff[40];
		TraceEvent(error_context, TPRF_REG_NEW_FILE, L"Success: FileId=%s, FileName=\"%s\".",
					fdesc->data.PrepareFileId(file_id_buff, 40), fdesc->data.info.file_name);
	}

	// Return full success.
	return(fdesc);
}

TFileInfoItem *TMidLevScanner::CreateIntermFile(PrEventHeader *ctx, TRawScanningType rscn)
{
	// At the current moment of creation of the interm files these files do not have neither a name not a body.
	TFileInfo finfo;
	finfo.Clear();
	finfo.Init(strg_interm, rscn);
	TFileInfoItem *fi = RegisterNewFile(ctx, finfo);
	return(fi);
}

void TMidLevScanner::SaveIntermFile(TFileInfoItem *pmsf)
{
	// This method should be called only with the INTERM file items.
	assert(pmsf->data.strg_type == strg_interm);
	assert(pmsf->interm_file_body != NULL);
	pmsf->interm_file_body->UpdateFileBody();

	// Send a callback that contains a final state of the file body.
	PrTempFileRegistration reg_info;
	reg_info.hdr = pmsf->interm_file_body->GetEventHeader();
	reg_info.file_info = pmsf->data;
	cbk_handler->RegisterTempFileData(&reg_info);
	PostProsessMidLevCbkCall(reg_info.hdr);

	// Mark the object.
	pmsf->interm_file_body->SetWriteComplete();
}

bool TMidLevScanner::PurgeCachedFile(TFileInfoItem *info)
{
	return(FALSE);
}

bool TMidLevScanner::PurgeCachedIndexingKey(TKeyInfoItem *info)
{
	return(FALSE);
}

bool TMidLevScanner::EnterFileSource(ID inc_path_id, TFileInfoItem *file_item,
							TLexSourceType srct, TMacroDefOrigin defs_nature,
							long starting_pos, long scan_length,
							bool new_line_stt, bool source_on_doubt,
							PrEventHeader *stmt_area, PrEventHeader *token_area,
							PrEventHeader *fname_area)
{
	assert(file_item != NULL);

	// Check whether the entering will happen or not. Note that the CN will be used in both cases.
	bool fail_enter = src_cnt >= MAX_INCLUDE_STACK_LEN || file_item->pragma_once_noticed == TRUE;
	ID cn = GetNextCN();

	// Trace entering or at least an attempt to enter the file source.
	if (trace_mask & sctr_lex_src_enter)
	{
		wchar_t fid_buff[40];
		file_item->data.PrepareFileId(fid_buff, 40);
		wchar_t *fail_reason_msg = L"";
		if (file_item->pragma_once_noticed == TRUE)
			fail_reason_msg = L" (Pragma ONCE is set - entering aborted)";
		else if (src_cnt >= MAX_INCLUDE_STACK_LEN)
			fail_reason_msg = L" (SourcesStackOveflow - entering aborted)";

		TraceEvent(stmt_area, TPRF_ENTER_FILE,
				L"CN=%lu%s, FileId=%s%s, FileName=\"%s\".",
				cn, fail_reason_msg, fid_buff, ((srct == srct_scgm) ? L"(SelfChangingMacro)" : L""),
				file_item->data.info.file_name);
	}

	if (fail_enter == TRUE)
	{
		// The pragma once is set or the lex sources stack overflow has happened.
		assert(parsing_phase >= mlph_scanning);

		if (src_cnt >= MAX_INCLUDE_STACK_LEN)
		{
			AddError(stmt_area, mscn_err_srcst_nesting_deep1);
		}
		else if (srct == srct_mj_src)
		{
			assert(file_item->pragma_once_noticed == TRUE);
			AddError(stmt_area, mscn_err_srcst_pr_once_in_mjsrc);
		}

		// Send this callback ONLY for #include statements.
		if (srct == srct_file)
		{
			TLexSourceType curr_srct = src_stack[src_cnt-1].src_type;
			if (curr_srct == srct_file || curr_srct == srct_mj_src)
			{
				// This is really an #include stmt. Send the structural callback.
				assert(stmt_area != NULL && token_area != NULL && fname_area != NULL);
				PrMiscPreprocStmtEvent info;
				info.Prepare(cn, exmis_failed_include, cond_pars_status, *stmt_area, *stmt_area);
				info.mpr_token = *token_area;
				info.evt_details = (file_item->pragma_once_noticed == TRUE) ? pinc_pragma_once : pinc_nesting_deep;
				info.PrepareFileNameSection(file_item, *fname_area);

				// Send the structural callback.
				cbk_handler->MiscPreprocEventNotification(&info);
				PostProsessMidLevCbkCall(info.hdr);
			}
		}

		// The lex source was not entered.
		return(FALSE);
	}

	// Prepare the linear pos of the first byte inside the new lex source.
	long new_linear = ((stmt_area != NULL) ? stmt_area->lin_area.AreaEnd() : summary.final_linear_offs)+1;

	// Prepare the callback header.
	PrLexSourceEnter info;
	memset(&info, 0, sizeof(info));
	PrepareCallbackHeader(info.hdr, cn, stmt_area);
	info.pbuff_enter_ref = (pbuff_cnt > 0) ? pbuff_stk[pbuff_cnt-1].cn_enter : 0;

	info.src_type = srct;
	info.on_doubt = src_on_doubt | source_on_doubt;
	info.inc_fn_mode = include_fname_mode;
	info.new_src_level = src_cnt+1;

	if (srct == srct_scgm)
	{
		// Self changing macro body group of fields.
		info.scgm_file_item = file_item;
		info.scgm_scan_area.file_info = &(file_item->data);
		info.scgm_scan_area.area_beg = starting_pos;
		info.scgm_scan_area.area_len = scan_length;
	}
	else
	{
		// Include file group of fields.
		info.inc_path_id = inc_path_id;
		info.include_file = file_item;
		if (token_area != NULL)
			info.mpr_token = *token_area;
		if (fname_area != NULL)
			info.stmt_fname = *fname_area;

		// Passing the number of the scanned pos params is primarily needed for decorating the source code.
		info.pos_params_cnt = (short)summary.cmdl_pos_params;
	}

	// Send the structural callback.
	cbk_handler->LexSourceEnterNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Report the call area to the current lex source level. This should be done only if the current source is available.
	if (src_cnt > 0)
	{
		assert(stmt_area != NULL);
		src_stack[src_cnt-1].ltst_call_area = *stmt_area;
	}

	// Fill in the stack frame.
	TLexemasSource &src = src_stack[src_cnt];
	src.src_type = srct;
	src.mdefs_nature = defs_nature;
	src.ltst_call_area.Clear();
	src.cn_enter = cn;
	src.initial_linear = new_linear;
	src.curr_linear_beg = new_linear-starting_pos;
	src.source_on_doubt = src_on_doubt | source_on_doubt;
	src.file_context = file_item;
	src.macro_context  = NULL;
	src.mprm_def_level = -1;
	src.mparam_decor = (TPreprocToken)0;
	src.xpnd_macros = TRUE;
	src.paste_suff = FALSE;

	// Update other lex source related variables.
	src_cnt++;
	src_on_doubt = src.source_on_doubt;

	// Setup the parsing area.
	if (srct == srct_cmdl || srct == srct_ant_env)
	{
		src.cmdl_scanner.SetKeyParamsInfo(cmdl_task_info.cmdl_ks_chars, cmdl_task_info.cmdl_keys_dict);
		src.cmdl_scanner.Setup(&(file_item->data), &misc_strings_dict);
		src.cmdl_scanner.Seek(starting_pos, scan_length);
	}
	else
	{
		src.scanner.Setup(&(file_item->data), &misc_strings_dict);
		src.scanner.Seek(starting_pos, scan_length, new_line_stt, include_fname_mode);
	}

	// Update the parsing summary.
	file_item->loads_count++;
	if (srct != srct_scgm)
	{
		summary.file_source_loads++;
		if (file_item->loads_count == 1)
		{
			summary.different_file_srcs++;
			summary.diff_file_srcs_len += file_item->data.info.file_len;
		}
	}

	// Full success - the lex source was entered.
	return(TRUE);
}

bool TMidLevScanner::EnterMacroSource(TMacroDefinition &mdef, PrEventHeader *stmt_area, PrEventHeader *name_area, bool xpnd_macros, bool paste_suff)
{
	// Check that the macro with params is not called on an empty lex src stack.
	assert(mdef.num_params == 0 || src_cnt > 0);
	assert(mdef.macro_call_flag == FALSE);

	// Pick up CN depending on whether an entering will happen or not.
	ID cn = (src_cnt < MAX_INCLUDE_STACK_LEN) ? GetNextCN() : 0;

	// Trace entering the macro definition.
	if (trace_mask & sctr_lex_src_enter)
	{
		wchar_t cn_buff[40];
		if (cn != 0)
			swprintf(cn_buff, 40, L"CN=%lu", cn);
		else wcscpy(cn_buff, L"(SourcesStackOvfl)");

		wchar_t loc_buff[80];
		TraceEvent(stmt_area, TPRF_ENTER_MDEF, L"%s, MacrodefCN=%lu, MacroName=\"%s\"", cn_buff, mdef.hdr.cn, mdef.def_name.CopyWithTruncationTo(loc_buff, 80));
	}

	if (src_cnt >= MAX_INCLUDE_STACK_LEN)
	{
		// Lex sources stack overflow.
		AddError(stmt_area, mscn_err_srcst_nesting_deep2);
		return(FALSE);
	}

	// Prepare the linear pos of the first byte inside the new lex source.
	long new_linear = ((stmt_area != NULL) ? stmt_area->lin_area.AreaEnd() : summary.final_linear_offs)+1;

	// Send the structural callback.
	PrLexSourceEnter info;
	memset(&info, 0, sizeof(info));
	PrepareCallbackHeader(info.hdr, cn, stmt_area);
	info.pbuff_enter_ref = (pbuff_cnt > 0) ? pbuff_stk[pbuff_cnt-1].cn_enter : 0;

	info.src_type  = srct_macro;
	info.on_doubt = src_on_doubt | GET_DOUBT_STT(mdef.def_status);
	info.inc_fn_mode = include_fname_mode;
	info.new_src_level = src_cnt+1;

	// Macro source group of fields.
	info.macro = &mdef;
	info.xpnd_macros  = xpnd_macros;
	info.paste_prefix = (pbuff_cnt > 0 && pbuff_stk[pbuff_cnt-1].suff_buffering == TRUE) ? TRUE : FALSE;
	info.paste_suffix = paste_suff;

	info.param_inx = -1;
	if (name_area != NULL)
		info.name_area = *name_area;

	cbk_handler->LexSourceEnterNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Report call area to the current lex source level.
	if (src_cnt > 0)
	{
		assert(stmt_area != NULL);
		src_stack[src_cnt-1].ltst_call_area = *stmt_area;
	}

	// Fill in the lex src frame with the macro call info.
	TLexemasSource &src = src_stack[src_cnt];
	src.src_type  = srct_macro;
	src.mdefs_nature = mdo_definition;
	src.ltst_call_area.Clear();
	src.cn_enter = cn;
	src.initial_linear = new_linear;
	src.curr_linear_beg = new_linear-mdef.macro_body.src_area.area_beg;
	src.source_on_doubt = src_on_doubt;
	src.file_context = NULL;
	src.macro_context  = &mdef;
	src.mprm_def_level = src_cnt-1;				// Parameters of the macro being called aree defined on the curr nesting layer.
	src.mparam_decor = (TPreprocToken)0;
	src.xpnd_macros = xpnd_macros;
	src.paste_suff = paste_suff;
	src.src_param_inx = info.param_inx;
	src.src_name_area = info.name_area;

	// Update the integral variables.
	src_cnt++;
	src_on_doubt |= GET_DOUBT_STT(mdef.def_status);

	// Setup the parsing area.
	src.scanner.Setup(mdef.macro_body.src_area.file_info, &misc_strings_dict);
	src.scanner.Seek(mdef.macro_body.src_area.area_beg, mdef.macro_body.src_area.area_len, FALSE, include_fname_mode);

	// The macro was successfully called.
	mdef.key_info->BumpUsage(mlku_mcall);
	mdef.def_use_count++;
	mdef.macro_call_flag = TRUE;
	summary.macro_calls_num++;
	macro_call_happened = TRUE;

	// Check for a self changing macro.
	if (mdef.def_blt_in_type != mbit_simple)
	{
		// This is a special macro.
		ServeSelfChangingMacroCall(mdef.def_blt_in_type);
	}

	return(TRUE);
}

bool TMidLevScanner::EnterMacroParamSource(PrEventHeader *stmt_area, int param_inx, TPreprocToken param_decor, bool xpnd_macros, bool paste_suff)
{
	// Check that the macro param is called from inside the macro context.
	assert(src_cnt > 0 && stmt_area != NULL);
	assert(LEX_SRC_GEN_MACRO(src_stack[src_cnt-1].src_type) == TRUE);
	TMacroDefinition &mdef = *src_stack[src_cnt-1].macro_context;
	TSourceArea &def_area = mdef.prm_val[param_inx].src_area;

	// Ensure that the definition layer for this macro params is available.
	int mprm_level = src_stack[src_cnt-1].mprm_def_level;
	assert(mprm_level >= 0 && mprm_level < src_cnt-1);

	// Pick up the CN depending on whether the entering the lex source will happen or not.
	ID cn = (src_cnt < MAX_INCLUDE_STACK_LEN) ? GetNextCN() : 0;

	// Trace entering the macro parameter.
	if (trace_mask & sctr_lex_src_enter)
	{
		wchar_t cn_buff[40];
		if (cn != 0)
			swprintf(cn_buff, 40, L"CN=%lu", cn);
		else wcscpy(cn_buff, L"(SourcesStackOvfl)");

		wchar_t loc_buff[80];
		TraceEvent(stmt_area, TPRF_ENTER_MPRM,
					L"%s, ParamNum=%d, ParamName=\"%s\", ParamDecor=%d.",
					cn_buff, param_inx+1,
					(param_inx < mdef.num_params) ? mdef.def_param_name[param_inx].CopyWithTruncationTo(loc_buff, 80) : L"__varg__",
					(int)param_decor);
	}

	if (src_cnt >= MAX_INCLUDE_STACK_LEN)
	{
		// Lex sources stack overflow.
		AddError(stmt_area, mscn_err_srcst_nesting_deep3);
		return(FALSE);
	}

	// Check, if an ordinary param or a var_args param is invoked.
	if (param_inx >= mdef.num_params)
	{
		// One of the var_args params is invoked. Save the param index into the macro. The macro will also
		// keep real paste_suffix that will be used with the last passed param.
		assert(param_inx < mdef.num_passed_params);
		mdef.next_prm_index = param_inx+1;

		if (param_inx == mdef.num_params)
		{
			mdef.real_paste_suff = paste_suff;
			paste_suff = FALSE;
		}

		if (param_inx == mdef.num_passed_params-1)
		{
			paste_suff = mdef.real_paste_suff;
		}
	}

	// Prepare the linear pos of the first byte inside the new lex source.
	long new_linear = stmt_area->lin_area.AreaEnd()+1;

	// Send the structural callback.
	PrLexSourceEnter info;
	memset(&info, 0, sizeof(info));
	info.hdr.Setup(stmt_area, cn);
	info.pbuff_enter_ref = (pbuff_cnt > 0) ? pbuff_stk[pbuff_cnt-1].cn_enter : 0;

	info.src_type = srct_mparam;
	info.on_doubt = src_on_doubt;
	info.inc_fn_mode = include_fname_mode;
	info.new_src_level = src_cnt+1;

	// Setup the macro source group of fields.
	info.macro = &mdef;
	info.xpnd_macros  = xpnd_macros;
	info.paste_prefix = (pbuff_cnt > 0 && pbuff_stk[pbuff_cnt-1].suff_buffering == TRUE) ? TRUE : FALSE;
	info.paste_suffix = paste_suff;

	info.param_inx = param_inx;					// The macro param index is zero based.
	info.param_decor = param_decor;
	info.name_area = mdef.prm_val[param_inx];

	cbk_handler->LexSourceEnterNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Report the call area to the current lex src level.
	src_stack[src_cnt-1].ltst_call_area = *stmt_area;

	// Fill in the lex source frame with the mparam call info.
	TLexemasSource &src = src_stack[src_cnt];
	src.src_type = srct_mparam;
	src.mdefs_nature = mdo_definition;
	src.ltst_call_area.Clear();
	src.cn_enter = cn;
	src.initial_linear = new_linear;
	src.curr_linear_beg = new_linear-def_area.area_beg;
	src.source_on_doubt = src_on_doubt;
	src.file_context = NULL;

	if (src_stack[mprm_level].macro_context != NULL)
	{
		// The params level is a macro level.
		src.macro_context  = src_stack[mprm_level].macro_context;
		src.mprm_def_level = src_stack[mprm_level].mprm_def_level;
	}
	else
	{
		// The params level is a source file level.
		src.macro_context  = NULL;
		src.mprm_def_level = -1;
	}

	src.mparam_decor = param_decor;
	src.xpnd_macros = xpnd_macros;
	src.paste_suff = paste_suff;
	src.src_param_inx = info.param_inx;
	src.src_name_area = info.name_area;

	// Update the integral variables. The doubt state cannot change here.
	src_cnt++;

	// Setup the parsing area.
	src.scanner.Setup(def_area.file_info, &misc_strings_dict);
	src.scanner.Seek(def_area.area_beg, def_area.area_len, FALSE, include_fname_mode);

	// Full Success.
	summary.mparam_calls_num++;
	return(TRUE);
}

bool TMidLevScanner::EnterPasteBuffSource(PrEventHeader *ctx, bool suff_buffer, bool append_tail, TFileInfoItem *pbuff_buff, TLexemasSource *lex_src_proto)
{
	// At least one source layer should be present, the passed file should be an interm file and
	// the passed context should have zero length.
	assert(src_cnt > 0);
	assert(ctx != NULL && ctx->lin_area.linear_len == 0 && ctx->src_area.area_len == 0);
	assert(pbuff_buff != NULL && pbuff_buff->data.strg_type == strg_interm);

	// Pick up the CN depending on whether an entering will happen or not.
	ID cn = (src_cnt < MAX_INCLUDE_STACK_LEN) ? GetNextCN() : 0;

	// Precalculate additional length of the data that might be added to the paste buffer for more accurate tracing.
	TLexemasSource &curr_src = src_stack[src_cnt-1];
	long new_data_len = 0;
	if (append_tail == TRUE && curr_src.scanner.IsEof() == FALSE)
		new_data_len = curr_src.scanner.buff_length-curr_src.scanner.offs;

	// Trace entering the paste buffer.
	if (trace_mask & sctr_lex_src_enter)
	{
		wchar_t cn_buff[40], buff_fid[40];
		if (cn != 0)
			swprintf(cn_buff, 40, L"CN=%lu", cn);
		else wcscpy(cn_buff, L"(SourcesStackOvfl)");

		TraceEvent(ctx, TPRF_ENTER_PBUF,
					L"%s, Type=%s, BufferFid=%s, BufferLen=%ld, PureAreaBeg=%ld.",
					cn_buff, (( suff_buffer == TRUE) ? L"SuffixBased" : L"SrcLevelBased"),
					pbuff_buff->data.PrepareFileId(buff_fid, 40),
					pbuff_buff->interm_file_body->DataLength()+new_data_len,
					pbuff_buff->interm_file_body->DataLength());
	}

	if (src_cnt >= MAX_INCLUDE_STACK_LEN)
	{
		// Include stack overflow. Recycle the bypassed intermediate file.
		AddError(ctx, mscn_err_srcst_nesting_deep4);
		SaveIntermFile(pbuff_buff);

		// Failure.
		return(FALSE);
	}

	if (new_data_len > 0)
	{
		// Add source data from the current layer starting from its current pos till its end and mark
		// this source as original data in the paste buff.
		if (pbuff_buff->interm_file_body->AddToBuffer(curr_src.file_context, curr_src.scanner.src, curr_src.scanner.offs, new_data_len, TRUE) == FALSE)
		{
			AddError(ctx, mscn_err_pbuff_stail_failure);
		}
	}

	// Prepare the linear pos of the first byte inside the new lex source.
	long new_linear = ctx->lin_area.AreaEnd()+1;

	// Send the structural callback.
	PrLexSourceEnter info;
	memset(&info, 0, sizeof(info));
	info.hdr.Setup(ctx, cn);
	info.pbuff_enter_ref = (pbuff_cnt > 0) ? pbuff_stk[pbuff_cnt-1].cn_enter : 0;

	info.src_type = (TLexSourceType)(((curr_src.src_type != srct_scgm) ? curr_src.src_type : srct_macro) | LEX_SRC_PBUFF_FLAG);
	info.on_doubt = curr_src.source_on_doubt;
	info.inc_fn_mode = include_fname_mode;
	info.new_src_level = src_cnt+1;

	// Copy the macro context info.
	info.macro = curr_src.macro_context;
	info.xpnd_macros  = curr_src.xpnd_macros;
	info.paste_prefix = (pbuff_cnt > 0 && pbuff_stk[pbuff_cnt-1].suff_buffering == TRUE) ? TRUE : FALSE;
	info.paste_suffix = curr_src.paste_suff;

	info.param_inx = curr_src.src_param_inx;				// The macro param index is zero based.
	info.param_decor = curr_src.mparam_decor;
	info.name_area = curr_src.src_name_area;

	// Fill in the paste buff info.
	info.append_tail = append_tail;
	info.paste_buff = pbuff_buff;

	cbk_handler->LexSourceEnterNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Report the call area to the current lex src level.
	src_stack[src_cnt-1].ltst_call_area = *ctx;

	// Fill in the new nesting frame with the info from the curr frame.
	TLexemasSource &src = src_stack[src_cnt];
	src = curr_src;									// <-- This statement will copy all fields of the structure.
	src.src_type = info.src_type;
	src.mdefs_nature = mdo_definition;
	src.ltst_call_area.Clear();
	src.cn_enter = cn;
	src.initial_linear = new_linear;
	src.curr_linear_beg = new_linear;
	src.file_context = pbuff_buff;

	if (lex_src_proto != NULL)
	{
		// Take the data fields from the passed lex source. In fact this source was just exited.
		src.mparam_decor = lex_src_proto->mparam_decor;
		src.xpnd_macros = lex_src_proto->xpnd_macros;
		src.paste_suff = lex_src_proto->paste_suff;
	}

	// Update the integral variables. The doubt status cannot change here.
	src_cnt++;

	// Setup parsing area starting from the beginning of the file.
	src.scanner.Setup(&(pbuff_buff->data), &misc_strings_dict);
	src.scanner.SetSpecialFileNameMode(include_fname_mode);

	// Full Success.
	summary.paste_buff_sources++;
	return(TRUE);
}

static const wchar_t *g_SourceTypeName[srct_num_types] =
{
	L"Bogus_val_0",	L"cmdl",			L"ant_env",		L"Bogus_val_3",
	L"file",			L"builtin_defs",	L"major_src",		L"self_chg_macro",
	L"macro",		L"mparam",		L"macro_pbuff",	L"mparam_pbuff",
};

void TMidLevScanner::ExitFromLexSource(PrEventHeader *eof_area)
{
	// Ensure that there is a lex source to exit from.
	assert(src_cnt > 0);
	TLexemasSource &src = src_stack[src_cnt-1];
	if (eof_area != NULL)
		assert(eof_area->source_ref == src.cn_enter);

	// Allocate a CN for this event.
	ID cn = GetNextCN();

	// Trace exit from this lex source.
	if (trace_mask & sctr_lex_src_exit)
	{
		TraceEvent(eof_area, TPRF_LSRC_EXIT,
					L"CN=%lu, SourceType=%s, PasteSuffixes=%d%d, NewSrcCnt=%d.",
					cn, g_SourceTypeName[src.src_type], ((src.mparam_decor == 0) && src.paste_suff), src.paste_suff, src_cnt-1);
	}

	ID pbuff_exit_evid = 0;
	long pbuff_extra_tail_len = 0;
	if (LEX_SRC_GEN_PBUFF(src.src_type) == TRUE)
	{
		// Put the "reading_complete" mark on the intermediate file.
		TFileInfoItem *pmsf = src.file_context;
		pmsf->interm_file_body->Truncate(src.scanner.offs, TRUE);
		pbuff_exit_evid = pmsf->interm_file_body->latest_pbuff_exit;

		// Use this chance to trace the contents of the paste buffer.
		if (trace_mask & sctr_paste_buff)
		{
			wchar_t fid_buff[40];
			long len = pmsf->interm_file_body->DataLength();

			TraceEvent(eof_area, TPRF_PBUFF_CONTS,
						L"Paste buff exited: BufferFid=%s, DataLen=%lu, Data=\"%.*s%s\".",
						pmsf->data.PrepareFileId(fid_buff, 40), len,
						(len <= 200) ? len : 200, pmsf->data.info.file_body, (len > 200) ? L"..." : L"");
		}

		// Report the contents of the buffer to the callback handler.
		SaveIntermFile(pmsf);

		if (src_cnt >= 2)
		{
			// Adjust the scanning position of the upper layer scanner if the paste buffer has consumed
			// part of the upper layer code that was not scanned yet.
			long pa_beg = pmsf->interm_file_body->GetPureAreaBeg();
			if (pa_beg >= 0 && src.scanner.offs > pa_beg)
			{
				pbuff_extra_tail_len = src.scanner.offs-pa_beg;
				src_stack[src_cnt-2].scanner.offs += pbuff_extra_tail_len;
				src_stack[src_cnt-2].scanner.new_line_stt = src.scanner.new_line_stt;
			}
		}
	}

	// Figure out the linear pos of the first byte after the return from the source. When the eof_area is
	// not passed, this means that the scanner is either destroyed or reset. In this case the linear value
	// is not important at all.
	long linear_after_return = (eof_area != NULL) ? eof_area->lin_area.AreaEnd()+1+pbuff_extra_tail_len : 0;

	// Send the structural callback.
	if (eof_area != NULL)
	{
		// When the parsing is active, the eof_area is always not NULL. This param is used as an indicator
		// if the callback notification is needed or not.
		PrLexSourceExit info;
		info.hdr.Setup(eof_area, cn);
		info.hdr.AdjustToAreaEnd();

		info.pbuff_exit_ref = (pbuff_cnt > 0) ? pbuff_stk[pbuff_cnt-1].cn_enter : 0;
		info.linear_pos_after_return = linear_after_return;
		info.src_offs_after_return = (src_cnt >= 2) ? src_stack[src_cnt-2].ltst_call_area.src_area.AreaEnd()+pbuff_extra_tail_len : 0;
		info.src_type = src.src_type;
		info.new_src_level = src_cnt-1;

		if (LEX_SRC_WHOLE_FILE(src.src_type) == TRUE)
		{
			info.pos_params_count = (short)summary.cmdl_pos_params;
		}
		else if (LEX_SRC_GEN_PBUFF(src.src_type) == TRUE)
		{
			info.pbuff_origin_exit_evid = pbuff_exit_evid;
			info.pbuff_final_length = src.scanner.offs;
		}

		cbk_handler->LexSourceExitNotification(&info);
		PostProsessMidLevCbkCall(info.hdr);
	}

	//
	// Dismiss the current lex source.
	//
	src_cnt--;
	src_on_doubt = (src_cnt > 0) ? src_stack[src_cnt-1].source_on_doubt : FALSE;

	if (src.src_type == srct_macro)
	{
		// Clear the "macro is busy" flag.
		src.macro_context->macro_call_flag = FALSE;
	}

	long linear_delta = linear_after_return-(src.initial_linear-1)-pbuff_extra_tail_len;
	if (src_cnt > 0)
	{
		// There are still unexited lex sources.
		src_stack[src_cnt-1].curr_linear_beg += linear_delta;
	}
	else
	{
		// Report the gained linear offset to the parsing summary.
		summary.final_linear_offs += linear_delta;
	}

	//
	// The exit process is complete. Process rare special cases.
	//
	if (src.src_type == srct_ant_env)
	{
		// After exiting from the analysis tool env file the file with the built in macros should be entered.
		assert(src_cnt > 0);
		PrEventHeader ctx;
		GetCurrScanPos(ctx);
		LoadBuiltInMacrosFile(&ctx);
	}
	else if (src.src_type == srct_mparam)
	{
		TMacroDefinition &mdef = *src_stack[src_cnt-1].macro_context;
		int next_index = mdef.next_prm_index;
		if (next_index >= mdef.num_params && next_index < mdef.num_passed_params)
		{
			// Call next var_arg parameter.
			EnterMacroParamSource(&src_stack[src_cnt-1].ltst_call_area, next_index, (TPreprocToken)0, src.xpnd_macros, FALSE);
		}
	}
	else if (src.src_type == srct_mj_src && parsing_phase != mlph_major_eof)
	{
		if (trace_mask & sctr_major_actions)
		{
			if (eof_area != NULL)
			{
				// Make the trace.
				PrEventHeader ctx;
				if (src_cnt > 0)
					GetCurrScanPos(ctx);

				TraceEvent((src_cnt > 0) ? &ctx : NULL, TPRF_EXIT_PHASE, L"Setting phase to %s.", g_ParsingPhaseNames[mlph_major_eof]);
			}
		}

		SetPhase(mlph_major_eof);
	}
}

bool TMidLevScanner::EnterPasteBuffLayer(PrEventHeader *event_area, TPreprocToken buff_type, bool paste_suff, TFileInfoItem *forced_buff)
{
	// Event area should be always present and it should always have zero length.
	assert(src_cnt > 0);
	assert(event_area != NULL);
	assert(event_area->lin_area.linear_len == 0);
	assert(event_area->src_area.area_len == 0);

	// Allocate either a new paste buffer or use an existing one.
	TFileInfoItem *pbuff = NULL, *new_pbuff = NULL;
	if (forced_buff != NULL)
	{
		// Use the bypassed buffer.
		pbuff = forced_buff;
	}
	else if (pbuff_cnt == 0 || buff_type == mpr_prm_cnct || (pbuff_stk[pbuff_cnt-1].suff_buffering == FALSE && paste_suff == TRUE))
	{
		// Buffering was not active before or the new layer requires its own buffer.
		TRawScanningType rscn = src_stack[src_cnt-1].GetFileInfo()->rscn_type;
		pbuff = new_pbuff = CreateIntermFile(event_area, rscn);
		if (new_pbuff == NULL)
		{
			AddError(event_area, mscn_err_pbuff_oom_for_pbuff);
			if (trace_mask & sctr_paste_buff)
			{
				TraceEvent(event_area, TPRF_PBUFF_ENTER, L"Failure - out of memory on the new paste buffer allocation.");
			}

			return(FALSE);
		}
	}
	else
	{
		// Use buffer from the current buffering layer.
		TPasteBufferingSlot &upper_slot = pbuff_stk[pbuff_cnt-1];
		pbuff = upper_slot.buffer;
		if (upper_slot.suff_buffering == FALSE)
		{
			// Both buffering layers are level based. Add a delimiter.
			wchar_t ch = upper_slot.GetDelimChar();
			if (pbuff->interm_file_body->AddToBuffer(&ch, 1) == FALSE)
			{
				// Pasting a delimiter failed.
				AddError(event_area, (ch == L' ') ? mscn_err_pbuff_delim_failure : mscn_err_pbuff_quote_failure);
			}
		}
	}

	// Check for availability of the buffering stack slot.
	if (pbuff_cnt >= MAX_PASTE_BUFF_STACK_LEN)
	{
		// Paste buffering stack overflow.
		wchar_t buff_limit[40];
		AddError(event_area, mscn_err_pbuff_nesting_deep, 0, _itow(MAX_PASTE_BUFF_STACK_LEN, buff_limit, 10));
		if (new_pbuff != NULL)
		{
			// Release the object that was just allocated.
			delete new_pbuff;
		}

		if (forced_buff != NULL)
		{
			// This object is shown to the callback handler here. Doing so significantly simplifies the upper layer.
			SaveIntermFile(forced_buff);
		}

		if (trace_mask & sctr_paste_buff)
		{
			TraceEvent(event_area, TPRF_PBUFF_ENTER, L"Failure - paste bufferings stack overflow.");
		}

		return(FALSE);
	}

	// Everything that may fail has succeeded. Allocate a CN.
	ID cn = GetNextCN();

	// Trace the paste buff layer entering.
	if (trace_mask & sctr_paste_buff)
	{
		wchar_t fid_buff[40];
		TraceEvent(event_area, TPRF_PBUFF_ENTER,
					L"Success: CN=%lu, Lev=%d, Type=%s, BufferFid=%s, BufferPos=%ld.",
					cn, pbuff_cnt+1, ((buff_type == mpr_prm_cnct) ? L"SuffixBased" : L"SrcLevelBased"),
					pbuff->data.PrepareFileId(fid_buff, 40), pbuff->interm_file_body->DataLength());
	}

	// Send the structural callback.
	PrPasteBufferingChange info;
	info.hdr.Setup(event_area, cn);
	info.parent_cn = (pbuff_cnt > 0) ? pbuff_stk[pbuff_cnt-1].cn_enter : 0;
	info.cn_enter = 0;
	info.new_buff_level = pbuff_cnt+1;
	info.buff_type = buff_type;
	info.buffer = pbuff;
	cbk_handler->PasteBuffChangeNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Modify the data structure.
	TPasteBufferingSlot &slot = pbuff_stk[pbuff_cnt++];
	slot.cn_enter = cn;
	slot.suff_buffering = (buff_type == mpr_prm_cnct) ? TRUE : FALSE;
	switch (buff_type)
	{
		case mpr_prm_chr:	slot.frame_char = L'\'';
							break;
		case mpr_prm_str:	slot.frame_char = L'\"';
							break;
		default:
			assert(buff_type == mpr_prm_cnct);
			slot.frame_char = 0;
			break;
	}

	slot.lev_buff_level = src_cnt+1;
	slot.buffer = pbuff;
	slot.beg_offs = pbuff->interm_file_body->DataLength();

	// Update the summary. This is a full success.
	summary.paste_buff_areas++;
	return(TRUE);
}

void TMidLevScanner::ExitPasteBuffLayer(PrEventHeader *event_area, TFileInfoItem **ppbuff_buff)
{
	// Event area should be always present and it should be empty.
	assert(pbuff_cnt > 0);
	assert(event_area != NULL);
	assert(event_area->lin_area.linear_len == 0);
	assert(event_area->src_area.area_len == 0);

	TPasteBufferingSlot &slot = pbuff_stk[pbuff_cnt-1];
	TIntermFileBody *pbuff_body = slot.buffer->interm_file_body;

	if (slot.suff_buffering == FALSE)
	{
		if (pbuff_body->DataLength() > slot.beg_offs)
		{
			// Escape all single and double quotes.
			long offs = slot.beg_offs+1;
			if (pbuff_body->EscapeQuotes(offs, pbuff_body->DataLength()-offs) == FALSE)
			{
				AddError(event_area, mscn_err_pbuff_esc_failure);
			}
		}
		else
		{
			// The buffering is empty. Add an opening quote.
			if (pbuff_body->AddToBuffer(&slot.frame_char, 1) == FALSE)
			{
				AddError(event_area, mscn_err_pbuff_quote_failure);
			}
		}

		// Add a temination quote.
		if (pbuff_body->AddToBuffer(&slot.frame_char, 1) == FALSE)
		{
			AddError(event_area, mscn_err_pbuff_quote_failure);
		}
	}

	ID cn = GetNextCN();
	if (trace_mask & sctr_paste_buff)
	{
		wchar_t fid_buff[40];
		TraceEvent(event_area, TPRF_PBUFF_EXIT, L"Success: CN=%lu, Lev=%d, Type=%s, BufferFid=%s, BufferPos=%ld.",
					cn, pbuff_cnt, ((slot.suff_buffering == TRUE) ? L"SuffixBased" : L"SrcLevelBased"),
					slot.buffer->data.PrepareFileId(fid_buff, 40), pbuff_body->DataLength());
	}

	// Send the mid lev callback.
	PrPasteBufferingChange info;
	info.hdr.Setup(event_area, cn);
	info.parent_cn = (pbuff_cnt > 1) ? pbuff_stk[pbuff_cnt-2].cn_enter : 0;
	info.cn_enter = slot.cn_enter;
	info.new_buff_level = pbuff_cnt-1;
	info.buff_type = (slot.suff_buffering == TRUE) ? mpr_prm_cnct : ((slot.frame_char == L'\"') ? mpr_prm_str : mpr_prm_chr);
	info.buffer = slot.buffer;
	cbk_handler->PasteBuffChangeNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Update the data structure.
	pbuff_body->latest_pbuff_exit = info.hdr.cn;
	pbuff_cnt--;

	// This action is not changing the summary.
	if (ppbuff_buff != NULL)
	{
		// Give out the buffer that was used on this level.
		*ppbuff_buff = slot.buffer;
	}
}

bool TMidLevScanner::ServeSelfChangingMacroCall(TBuiltInMacroType mbit)
{
	assert(mbit > mbit_simple && mbit < mbit_num_types);

	// Prepare info about the macro call origin.
	assert(src_cnt >= 2);
	TLexemasSource &call_src = src_stack[src_cnt-2];
	assert(call_src.src_type != srct_cmdl);
	TFileInfo *fi = (TFileInfo*)call_src.scanner.GetFileInfo();

	// Use empty sized area at the current scan pos for tracing, errors and the call origin.
	PrEventHeader lctx;
	GetCurrScanPos(lctx);

	// Figure out what should be generated for this call. Handlers should place text into the buffer and fill in
	// the ident/scan areas of this text into the TSelfChangeMacroGenHistItem.
	TTextBuffer80 buffer;
	TFileInfo dummy_file;
	TSelfChangeMacroGenHistItem si(dummy_file);
	switch (mbit)
	{
		case mbit_file:
				{
					// Give out short name of the file with the macro call.
					wchar_t *sfname = wcsrchr(fi->info.file_name, L'\\');
					if (sfname == NULL)
						sfname = wcsrchr(fi->info.file_name, L'/');

					if (sfname == NULL)
						sfname = fi->info.file_name;
					else sfname++;

					buffer.Append(sfname);
					si.ident_area.area_len = buffer.NumItems();
					buffer.Append(L"\r\n");
					si.scan_area.area_len = buffer.NumItems()-2;

					if (buffer.GetXpndError() == TRUE)
					{
						AddOutOfMemError(&lctx, mloom_serve_scgm_loc0);
						return(FALSE);
					}
				}
				break;

		case mbit_line:
				{
					// Resolve and give out the current line number.
					long lnum = -1;
					if (fi->CheckLinesInfo() == TRUE || fi->BuildLinesInfo() == TRUE)
					{
						// The lines info was already existing or it was just created.
						TSourceArea area;
						call_src.scanner.GetCurrPos(area);
						lnum = fi->GetLineNumber(area.area_beg);
					}

					wchar_t buff[80];
					swprintf(buff, 80, L"%lu\r\n", lnum);
					bool res = buffer.Append(buff);
					assert(res == TRUE);

					si.ident_area.area_len = buffer.NumItems();
					si.scan_area.area_len = buffer.NumItems()-2;
				}
				break;

		case mbit_function:
				{
					// NYI.
					bool res = buffer.Append(L"NYI\r\n");
					assert(res == TRUE);

					si.ident_area.area_len = buffer.NumItems();
					si.scan_area.area_len = buffer.NumItems()-2;
				}
				break;

		case mbit_counter:
				{
					// Give out the next value of the counter.
					wchar_t buff[80];
					swprintf(buff, 80, L"%lu\r\n", ++scgm_info[mbit].dwData);
					bool res = buffer.Append(buff);
					assert(res == TRUE);

					si.ident_area.area_len = buffer.NumItems();
					si.scan_area.area_len = buffer.NumItems()-2;
				}
				break;

		default:
			assert(FALSE);
			return(FALSE);
	}

	// Put current buff location into the dummy_file descriptor.
	dummy_file.info.file_body = buffer.DataPtr();
	assert(buffer.NumItems() > 0);

	// Look for identical info in the hash.
	TFileInfoItem *finfo = scgm_info[mbit].file_info;
	TSelfChangeMacroGenHistItem *item = scgm_info[mbit].FindRecord(&si);
	if (item == NULL)
	{
		// This item is not available in the hash. Ensure that the interm file is availabe.
		if (finfo == NULL)
		{
			// This macro has not been used yet.
			finfo = CreateIntermFile(&lctx, rscn_c_cpp);
			if (finfo == NULL)
			{
				AddOutOfMemError(&lctx, mloom_serve_scgm_loc1);
				return(FALSE);
			}

			scgm_info[mbit].file_info = finfo;
		}

		// Append generated data to the interm file.
		long old_len = finfo->interm_file_body->DataLength();
		if (finfo->interm_file_body->AddToBuffer(buffer, buffer.NumItems()) == FALSE)
		{
			AddOutOfMemError(&lctx, mloom_serve_scgm_loc2);
			return(FALSE);
		}

		if (wcschr(buffer, L'\n') != NULL)
		{
			// The lines info table is out of date.
			finfo->data.ReleaseLinesInfo();
		}

		// Create item for the hash table.
		si.ident_area.file_info = &(finfo->data);
		si.scan_area.file_info = &(finfo->data);
		si.ident_area.area_beg += old_len;
		si.scan_area.area_beg += old_len;
		item = new TSelfChangeMacroGenHistItem(si.ident_area, si.scan_area);
		if (item == NULL)
		{
			AddOutOfMemError(&lctx, mloom_serve_scgm_loc3);
			return(FALSE);
		}

		// Add item to the table.
		scgm_info[mbit].AddRecord(item);
	}

	// THe data to scan is prepared. Nest the source file.
	return(EnterFileSource(0, finfo, srct_scgm, mdo_definition, item->scan_area.area_beg, item->scan_area.area_len, TRUE, FALSE, &lctx, NULL, NULL));
}

void TMidLevScanner::ProcessMacroDirective(TLexema &lexema)
{
	assert(src_cnt > 0);
	assert(LEX_SRC_WHOLE_FILE(src_stack[src_cnt-1].src_type) == TRUE);
	assert(lexema.IsPreprocToken() == TRUE);

	// Do not exit the current lex source while processing the macro directive.
	int prev_auto_exit_level = SetLexSrcExitLimit(src_cnt);
	assert(prev_auto_exit_level == 0);

	if (lexema.first_in_line == FALSE)
	{
		// The stmt token should stay at the beginning of the line.
		AddError(&lexema.origin, mscn_err_srcst_preproc_notlbeg);
	}

	// Switch between different types of directives.
	PrEventHeader dummy_final_lex_origin;
	switch ((short)lexema.num_value)
	{
		case mpr_define:
				{
					if (active_code == TRUE && options.disab_define_stmts == FALSE)
					{
						ProcessDefineStmt(lexema);
					}
					else
					{
						ProcessEmptyStmt(lexema, sctr_stmt_def_undef, TPRF_STMT_DEFINE);
					}
				}
				break;

		case mpr_undef:
				{
					if (active_code == TRUE && options.disab_define_stmts == FALSE)
					{
						ProcessUndefStmt(lexema);
					}
					else
					{
						ProcessEmptyStmt(lexema, sctr_stmt_def_undef, TPRF_STMT_UNDEF);
					}
				}
				break;

		case mpr_include:
				{
					if (options.disab_include_stmts == FALSE)
					{
						// Include statements check for the active/passive code inside.
						ProcessIncludeStmt(lexema);
					}
					else
					{
						ProcessEmptyStmt(lexema, sctr_stmt_include, TPRF_STMT_INCLUDE);
					}
				}
				break;

		case mpr_ifdef:
		case mpr_ifndef:
		case mpr_if:
		case mpr_elif:
		case mpr_else:
		case mpr_endif:
				{
					if (options.disab_cond_pars_stmts == FALSE)
					{
						// Conditional statememnts should be processed even in the passive code.
						ProcessConditionalStmt(lexema);
					}
					else
					{
						ProcessEmptyStmt(lexema, sctr_stmt_cond_pars, TPRF_STMT_CONDPR);
					}
				}
				break;

		case mpr_pragma:
				{
					if (active_code == TRUE && options.disab_misc_prepr_stmts == FALSE)
					{
						ProcessPragmaStmt(lexema);
					}
					else
					{
						ProcessEmptyStmt(lexema, sctr_stmt_pragma_error, TPRF_STMT_PRAGMA);
					}
				}
				break;

		case mpr_error:
				{
					if (active_code == TRUE && options.disab_misc_prepr_stmts == FALSE)
					{
						// An error statement may abort the parsing.
						ProcessErrorStmt(lexema);
					}
					else
					{
						ProcessEmptyStmt(lexema, sctr_stmt_pragma_error, TPRF_STMT_ERROR);
					}
				}
				break;

		case mpr_import:
				{
					if (active_code == TRUE && options.disab_misc_prepr_stmts == FALSE)
					{
						ProcessImportStmt(lexema);
					}
					else
					{
						ProcessEmptyStmt(lexema, sctr_stmt_misc_mid_lev, TPRF_STMT_IMPORT);
					}
				}
				break;

		case mpr_line:
				{
					if (active_code == TRUE && options.disab_misc_prepr_stmts == FALSE)
					{
						ProcessLineStmt(lexema);
					}
					else
					{
						ProcessEmptyStmt(lexema, sctr_stmt_misc_mid_lev, TPRF_STMT_LINE);
					}
				}
				break;

		case mpr_using:
				{
					if (active_code == TRUE && options.disab_misc_prepr_stmts == FALSE)
					{
						ProcessUsingStmt(lexema);
					}
					else
					{
						ProcessEmptyStmt(lexema, sctr_stmt_misc_mid_lev, TPRF_STMT_USING);
					}
				}
				break;

		default:
			{
				// Most likely the input param is bogus.
				if (trace_mask & sctr_stmt_misc_mid_lev)
					TraceFuncEnter(&lexema.origin, TPRF_STMT_BOGUS, L"Bogus token type.");

				wchar_t buff_num_val[40];
				AddError(&lexema.origin, mscn_err_srcst_preproc_other, 0, _itow((int)lexema.num_value, buff_num_val, 10));
				ScanTillEol(lexema, TRUE, dummy_final_lex_origin);

				if (trace_mask & sctr_stmt_misc_mid_lev)
					TraceFuncExit(&dummy_final_lex_origin, TPRF_STMT_BOGUS, L"Bogus token type.");
			}
			break;
	}

	// Return back the previous high water mark of the lex sources exit.
	SetLexSrcExitLimit(prev_auto_exit_level);
}

void TMidLevScanner::ProcessEmptyStmt(TLexema &directive_lexema, TMidLevScannerTraceFlags mask, const wchar_t *stmt_prefix)
{
	if (trace_mask & mask)
	{
		TraceFuncEnter(&directive_lexema.origin, stmt_prefix, (active_code == TRUE) ? L"The statement is ignored because of the pasring options." : L"The statement is in the passive code.");
	}

	PrEventHeader dummy_final_lex_origin;
	ScanTillEol(directive_lexema, TRUE, dummy_final_lex_origin);

	if (trace_mask & mask)
	{
		TraceFuncExit(&dummy_final_lex_origin, stmt_prefix, L"End of processing.");
	}
}

void TMidLevScanner::ProcessDefineStmt(TLexema &token_lex)
{
	if (trace_mask & sctr_stmt_def_undef)
	{
		TraceFuncEnter(&token_lex.origin, TPRF_STMT_DEFINE);
	}

	// Scan the name of the macro.
	TLexema name_lex;
	PrEventHeader final_lex_origin;
	if (GetPrStmtLoLex(name_lex) != ltx_name)
	{
		// There is no name. It is hard to do anything with this statement.
		if (name_lex.type == ltx_keyword)
			AddWarning(&name_lex.origin, mscn_err_mdef_name_token, 0, TLexema::GetKeywordText((int)name_lex.num_value), L"#define");
		else AddError(&name_lex.origin, mscn_err_mdef_no_macro_name);

		ScanTillEol(name_lex, TRUE, final_lex_origin);

		if (trace_mask & sctr_stmt_def_undef)
		{
			TraceFuncExit(&final_lex_origin, TPRF_STMT_DEFINE, L"Failure - the name of the macro is missing.");
		}

		return;
	}

	// Allocate the core structure to store information about the macro definition.
	TMacroDefinition *macro = cbk_handler->CreateMacroDefinition(name_lex.str_value, (conditional_area == TRUE) ? mds_soft_def : mds_hard_def, src_stack[src_cnt-1].mdefs_nature);
	if (macro == NULL)
	{
		// This is either an out of memory case or the app layer has decided not to create the macro object.
		AddError(&name_lex.origin, mscn_err_mdef_oom_for_macro);
		ScanTillEol(name_lex, TRUE, final_lex_origin);

		if (trace_mask & sctr_stmt_def_undef)
		{
			TraceFuncExit(&final_lex_origin, TPRF_STMT_DEFINE, L"Failure - cannot create the TMacroDefinition object.");
		}

		return;
	}

	if (macro->def_origin_type == mdo_builtin)
	{
		// Check the name for the self changing macro names.
		if (macro->def_name == L"__FILE__")
			macro->def_blt_in_type = mbit_file;
		else if (macro->def_name == L"__LINE__")
			macro->def_blt_in_type = mbit_line;
		else if (macro->def_name == L"__FUNCTION__")
			macro->def_blt_in_type = mbit_function;
		else if (macro->def_name == L"__COUNTER__")
			macro->def_blt_in_type = mbit_counter;
	}

	//
	// Check if this macro definition has parameters or not and scan them if any.
	//

	// Check the inline space flag (num_val) in the name_lex lexema. It should be FALSE to allow this macro to have params.
	// The GetLoLex() can be called here directly because the call to GetPrStmtLoLex() above has emptied the saved_lex if any.
	TLexema body_beg_lex;
	GetLoLex(body_beg_lex);

	if ((short)name_lex.num_value == 0 && body_beg_lex.IsKeyword(opr_lpar) == TRUE)
	{
		// This macro has parameters. The opening parenthesis is already scanned.
		TLexema lex;
		bool mprm_name_expected = TRUE;
		for (bool first_lexema = TRUE;; first_lexema = FALSE)
		{
			// Pick up the next lexema.
			GetPrStmtLoLex(lex);

			// Check for the params list terminators.
			if (lex.IsKeyword(opr_rpar) == TRUE)
			{
				// An end of parameters list was reached.
				if (first_lexema == FALSE && mprm_name_expected == TRUE)
					AddError(&lex.origin, mscn_err_mdef_no_mprm_name);
				break;
			}
			else if (lex.type == ltx_eof)
			{
				AddError(&lex.origin, mscn_err_mdef_no_mprm_lst_end);
				break;
			}
			else if (lex.type == ltx_eol)
			{
				// The scanning function should give out only the naked eols.
				assert(lex.endofline_type == leolt_normal);
				AddError(&lex.origin, mscn_err_mdef_no_mprm_lst_end);
				break;
			}

			if (macro->var_args_macro == TRUE)
			{
				// This is an extra lexema after the triple dots.
				AddError(&lex.origin, mscn_err_mdef_no_vargs_paren);
				continue;
			}

			// Check for other lexemas that cannot terminate the list of params.
			if (lex.type == ltx_name)
			{
				// Check that there is no duplication of the param names.
				bool dupl_found = FALSE;
				for (int ii=0; ii<macro->num_params; ++ii)
				{
					if (lex.str_value == macro->def_param_name[ii])
					{
						AddError(&lex.origin, mscn_err_mdef_mprm_name_dupe);
						dupl_found = TRUE;
						break;
					}
				}

				if (dupl_found == FALSE)
				{
					// The is no duplication. Add this parameter.
					if (macro->AddMacroParam(lex.str_value) == FALSE)
					{
						// Adding the macro parameter failed.
						wchar_t buff_max_num_params[40];
						AddError(&lex.origin, mscn_err_mdef_too_many_mprms, 0, _itow(MAX_MACRO_PARAMS_NUM, buff_max_num_params, 10));
					}
				}

				mprm_name_expected = FALSE;
			}
			else if (lex.IsKeyword(opr_comma) == TRUE)
			{
				if (mprm_name_expected == TRUE)
					AddError(&lex.origin, mscn_err_mdef_no_mprm_name);

				mprm_name_expected = TRUE;
			}
			else if (lex.IsKeyword(spr_ellipsis) == TRUE)
			{
				// This is an var_args macro.
				if (mprm_name_expected == FALSE)
					AddError(&lex.origin, mscn_err_mdef_no_mprm_separ);

				macro->var_args_macro = TRUE;
				mprm_name_expected = FALSE;
			}
			else if (lex.type == ltx_keyword && lex.num_value >= clg_none && lex.num_value <= cpp_max_token && mprm_name_expected == TRUE)
			{
				// This is some other keyword with an "identifier style".
				AddError(&lex.origin, mscn_err_mdef_mprm_name_token, 0, TLexema::GetKeywordText((int)lex.num_value));
				mprm_name_expected = FALSE;
			}
			else
			{
				// This is some bogus lexema. It cannot be a raw error. These errors are filtered out by the GetMidLevStatementLex(lex).
				AddError(&lex.origin, mscn_err_mdef_mprm_lst_error);
			}
		}

		if (lex.IsKeyword(opr_rpar) == TRUE)
		{
			// Pick up the first lexema of the body.
			GetFilteredLoLex(body_beg_lex);
		}
		else
		{
			// The scanning was not terminated normally. Use the existing lexema.
			body_beg_lex = lex;
		}
	}
	else if (CanFilterOut(body_beg_lex) == TRUE)
	{
		// Pick up a better lexema for the beg of the body.
		GetFilteredLoLex(body_beg_lex);
	}

	//
	// Scan the body of the macro till the eol or the eof character and do not report raw parsing errors if any.
	//

	TLexema lex = body_beg_lex;
	int cnt_lex = 0;
	for(;;)
	{
		if (lex.type == ltx_eof)
			break;

		if (lex.type == ltx_eol && lex.endofline_type == leolt_normal)
		{
			// Scanning the body of the macro came to its normal end.
			break;
		}

		if (lex.type != ltx_comment && lex.type != ltx_eol)
			cnt_lex++;

		if (cnt_lex == 1 && lex.type >= ltx_number && lex.type <= ltx_charconst)
			macro->scst_body_lex = lex;

		if (lex.type == ltx_keyword && lex.keyword_type == ltkn_preproc && (short)lex.num_value >= mpr_define)
			AddWarning(&lex.origin, mscn_err_mdef_prstkn_in_mdef);

		// Macro definition may contain a lot of lexemas inside. Update the console counter here.
		status_callback_cnt--;

		// Pick up the next lexema.
		GetLoLex(lex);
	}

	if (cnt_lex != 1 || macro->num_params != 0 || macro->var_args_macro == TRUE)
		macro->scst_body_lex.Clear();

	// The definition is scanned. Maybe with errors.
	PrEventHeader stmt_area = token_lex.origin;
	PrEventHeader body_area = body_beg_lex.origin;
	stmt_area.ExtendPrHeader(lex.origin);
	body_area.ExtendPrHeader(lex.origin);
	macro->SetupDefinitionArea(stmt_area);
	macro->SetupBodyArea(body_area);

	// Protected method will do major part of the work.
	bool res = UpdateMacroLibrary(macro, name_lex, token_lex.origin);

	// Trace this statement.
	if (trace_mask & sctr_stmt_def_undef)
	{
		wchar_t loc_buff[80];
		TraceFuncExit(&lex.origin, TPRF_STMT_DEFINE, (res == TRUE) ? L"Success: CN=%lu, Name=\"%s\"." : L"Failure in the UpdateMacroLibrary().",
					macro->hdr.cn, macro->def_name.CopyWithTruncationTo(loc_buff, 80));
	}
}

void TMidLevScanner::ProcessUndefStmt(TLexema &token_lex)
{
	if (trace_mask & sctr_stmt_def_undef)
	{
		TraceFuncEnter(&token_lex.origin, TPRF_STMT_UNDEF);
	}

	TLexema name_lex;
	PrEventHeader final_lex_origin;

	// Scan the name of the macro to undefine.
	if (GetPrStmtLoLex(name_lex) != ltx_name)
	{
		if (name_lex.type == ltx_keyword)
			AddWarning(&name_lex.origin, mscn_err_mdef_name_token, 0, TLexema::GetKeywordText((int)name_lex.num_value), L"#undef");
		else AddError(&name_lex.origin, mscn_err_mdef_no_macro_name);

		ScanTillEol(name_lex, TRUE, final_lex_origin);

		if (trace_mask & sctr_stmt_def_undef)
		{
			TraceFuncExit(&final_lex_origin, TPRF_STMT_UNDEF, L"Failure - the name of the macro is missing.");
		}

		return;
	}

	// Scan all other lexemas till the end of the line. If there will be some errors, report them but still process the statement.
	ScanTillEol(name_lex, FALSE, final_lex_origin);

	// Allocate a core structure to keep the information about the new state of the macro definition.
	TMacroDefinition *macro = cbk_handler->CreateMacroDefinition(name_lex.str_value, (conditional_area == TRUE) ? mds_soft_undef : mds_hard_undef,
														src_stack[src_cnt-1].mdefs_nature);
	if (macro == NULL)
	{
		// Out of memory or an app layer decided not to create the macro object.
		AddError(&name_lex.origin, mscn_err_mdef_oom_for_macro);
		if (trace_mask & sctr_stmt_def_undef)
		{
			TraceFuncExit(&final_lex_origin, TPRF_STMT_UNDEF, L"Failure - cannot create the TMacroDefinition object.");
		}
		return;
	}

	// Complete the missing fields in the macro.
	PrEventHeader stmt_area = token_lex.origin;
	stmt_area.ExtendPrHeader(final_lex_origin);
	macro->SetupDefinitionArea(stmt_area);

	// Protected method will do the major part of the work.
	bool res = UpdateMacroLibrary(macro, name_lex, token_lex.origin);

	// Trace this statement.
	if (trace_mask & sctr_stmt_def_undef)
	{
		TraceFuncExit(&final_lex_origin, TPRF_STMT_UNDEF, (res == TRUE) ? L"Success: CN=%lu." : L"Failure in the UpdateMacroLibrary().", macro->hdr.cn);
	}
}

void TMidLevScanner::ProcessIncludeStmt(TLexema &token_lex)
{
	if (trace_mask & sctr_stmt_include)
	{
		TraceFuncEnter(&token_lex.origin, TPRF_STMT_INCLUDE);
	}

	SetIncFileNameMode(TRUE);

	// Retrieve the name of the file to include. Note that the macro expansion is possible here.
	TLexema fname_lex;
	THiddenIncudeStmtDtls dtls = pinc_none;
	if (GetPrStmtMidLex(fname_lex) != ltx_string)
	{
		AddError(&fname_lex.origin, mscn_err_ostm_incl_no_fname);
		dtls = pinc_syntax_error;
	}

	// Scan till the eol and report errors if something is present after the name of the file. The statement
	// will be executed even if these errors will be found.
	PrEventHeader stmt_end_area;
	ScanTillEol(fname_lex, FALSE, stmt_end_area);
	SetIncFileNameMode(FALSE);

	// Check if the statement is in the active code or not.
	if (dtls == pinc_none && active_code == FALSE)
	{
		// Entering the file should not take place.
		dtls = pinc_pasive_code;
	}

	// Load the file or look for this file in the cache.
	TMidLevIncludeHelper hlpr(this, TRUE, options.report_includes_from_same_dir, options.report_include_file_duplications);
	TFileInfoItem *file_item = NULL;
	if (dtls == pinc_none)
	{
		file_item = hlpr.LoadIncludeFile(fname_lex);
		if (file_item == NULL)
		{
			dtls = (hlpr.load_res == ldres_fname_missing) ? pinc_file_not_found : pinc_file_loading_err;
		}
	}

	// Check if all steps have succeeded or not.
	if (dtls == pinc_none)
	{
		// No errors were detected so far. Call the EnterFileSource(). It will call appropriate structural callback
		// even if it will encounter errors inside and the file will be not called.
		PrEventHeader stmt_area = token_lex.origin;
		stmt_area.ExtendPrHeader(stmt_end_area);
		bool res = EnterFileSource(hlpr.inc_path_ident, file_item, srct_file, mdo_definition,  0, -1, TRUE, FALSE, &stmt_area, &token_lex.origin, &fname_lex.origin);

		// Trace this statement.
		if (trace_mask & sctr_stmt_include)
		{
			TraceFuncExit(&stmt_end_area, TPRF_STMT_INCLUDE, (res == TRUE) ? L"Success." : L"Failure in the EnterFileSource().");
		}
	}
	else
	{
		// Entering the include file should not take place.
		ID cn = GetNextCN();

		// Send the structural notification (mpr_include).
		PrMiscPreprocStmtEvent info;
		info.Prepare(cn, exmis_failed_include, cond_pars_status, token_lex.origin, stmt_end_area);
		info.evt_details = dtls;

		info.doc_ident = (file_item != NULL) ? file_item->data.doc_id : 0;

		if (dtls >= pinc_file_loading_err)
			info.fname_data = hlpr.main_location.DataPtr();
		else if (dtls >= pinc_pasive_code)
			info.fname_data = fname_lex.str_value;

		if (dtls >= pinc_pasive_code)
			info.fname_area = fname_lex.origin;

		cbk_handler->MiscPreprocEventNotification(&info);
		PostProsessMidLevCbkCall(info.hdr);

		// Trace this statement.
		if (trace_mask & sctr_stmt_include)
		{
			TraceFuncExit(&stmt_end_area, TPRF_STMT_INCLUDE, L"CN=%lu, NonEnterReason=%s.", cn, PrMiscPreprocStmtEvent::GetNonCallReasonName(dtls));
		}
	}
}

static TConditionalStatus g_CodeStatusFromHistoryAndExprResult[cexr_num_types][cexr_num_types] =
{
	// History: hard_FALSE.
	{  cexr_hard_false,		cexr_hard_true,	cexr_soft_false,	cexr_soft_true,	cexr_undefined	},

	// History: hard_TRUE.
	{  cexr_hard_false,		cexr_hard_false,	cexr_hard_false,	cexr_hard_false,	cexr_hard_false	},

	// History: soft_FALSE.
	{  cexr_hard_false,		cexr_soft_true,	cexr_soft_false,	cexr_soft_true,	cexr_undefined	},

	// History: soft_TRUE.
	{  cexr_hard_false,		cexr_soft_false,	cexr_soft_false,	cexr_soft_false,	cexr_soft_false	},

	// History: Undefined.
	{  cexr_hard_false,		cexr_undefined,	cexr_undefined,	cexr_undefined,	cexr_undefined	},
};

static TConditionalStatus g_NewHistoryFromHistoryAndExprResult[cexr_num_types][cexr_num_types] =
{
	// History: hard_FALSE.
	{  cexr_hard_false,		cexr_hard_true,	cexr_soft_false,	cexr_soft_true,	cexr_undefined	},

	// History: hard_TRUE.
	{  cexr_hard_true,		cexr_hard_true,	cexr_hard_true,	cexr_hard_true,	cexr_hard_true	},

	// History: soft_FALSE.
	{  cexr_soft_false,		cexr_soft_true,	cexr_soft_false,	cexr_soft_true,	cexr_undefined	},

	// History: soft_TRUE.
	{  cexr_soft_true,		cexr_soft_true,	cexr_soft_true,	cexr_soft_true,	cexr_soft_true		},

	// History: Undefined.
	{  cexr_undefined,		cexr_undefined,	cexr_undefined,	cexr_undefined,	cexr_undefined	},
};

void TMidLevScanner::ProcessConditionalStmt(TLexema &token_lex)
{
	assert(token_lex.type == ltx_keyword && token_lex.keyword_type == ltkn_preproc);
	TPreprocToken stmt_type = (TPreprocToken)token_lex.num_value;

	if (trace_mask & sctr_stmt_cond_pars)
	{
		TraceFuncEnter(&token_lex.origin, TPRF_STMT_CONDPR, L"CondParsNest=%d, StmtType=%s.", cond_cnt, TLexema::GetKeywordIdEnumName(stmt_type));
	}

	//
	// Step1: Check the high level points: Conditional parsing level should be in the correct state and in case
	// if an #else statement was already noticed, there should be no #elif or #else again. Prepare the nesting
	// shift variables.
	//
	int old_level = cond_cnt;
	int stmt_shift = 0;
	int code_shift = 0;

	if (stmt_type == mpr_ifdef || stmt_type == mpr_ifndef || stmt_type == mpr_if)
	{
		// These keywords open a new cond parsing nesting.
		if (cond_cnt >= MAX_IFDEF_STACK_LEN)
		{
			// There are more #ifxxs than the stack can keep.
			wchar_t buff_max_nesting[40];
			AddError(&token_lex.origin, mscn_err_cond_nesting_deep, 0, _itow(MAX_IFDEF_STACK_LEN, buff_max_nesting, 10));

			// This is a hard error. The statement should be ignored.
			PrEventHeader dummy_stmt_end_area;
			ScanTillEol(token_lex, TRUE, dummy_stmt_end_area);

			if (trace_mask & sctr_stmt_cond_pars)
			{
				TraceFuncExit(&dummy_stmt_end_area, TPRF_STMT_CONDPR, L"Failure - the conditional parsing nesting is too deep.");
			}

			return;
		}

		// Assign the shift value.
		code_shift = 1;
	}
	else
	{
		if (cond_cnt <= 0)
		{
			// There are more #else statements than there were #if stmts.
			AddError(&token_lex.origin, mscn_err_cond_stack_underflow);

			// This is hard error. Statement should will be ignored.
			PrEventHeader dummy_stmt_end_area;
			ScanTillEol(token_lex, TRUE, dummy_stmt_end_area);

			if (trace_mask & sctr_stmt_cond_pars)
			{
				TraceFuncExit(&dummy_stmt_end_area, TPRF_STMT_CONDPR, L"Failure - the cond pars stack underflow.");
			}

			return;
		}

		// Assign the shift values.
		stmt_shift = -1;
		if (stmt_type == mpr_endif)
			code_shift = -1;
	}

	// Update an integral state of the cond pars vars. They should reflect the state of the statement layer.
	if (stmt_shift != 0)
	{
		// Restore the integral cond pars state to the level of the stmt.
		SetupIntegralCondParsingState(old_level+stmt_shift);
		cond_area_cn = GetCondRefForLevel(old_level+stmt_shift);
	}

	// Allocate a CN for this event.
	ID cn = GetNextCN();

	if (stmt_type == mpr_elif || stmt_type == mpr_else)
	{
		// The code above has already checked that the cond pars nesting level is not zero.
		if (cond_stk[cond_cnt-1].else_flag == TRUE)
		{
			// These errors are not hard. The statement will be processed.
			AddError(&token_lex.origin, (stmt_type == mpr_elif) ? mscn_err_cond_elif_after_else : mscn_err_cond_second_else);
		}
	}

	//
	// Step2: Calculate expression if the statement requires this and check that there is nothing else till the end of the line.
	//
	ID						scan_error_cn = 0;
	TCondExprScanningDetails	scan_res_details = cescd_none;
	PrEventHeader			expr_area, stmt_end_area;
	TCondExprTreeItem		*unsimplified_expr_code = NULL;
	TConditionalStatus			expr_res = cexr_undefined;
	TCondExprSmplResult		smpl_stt = cesr_unsimpl;

	switch (stmt_type)
	{
		case mpr_ifdef:
		case mpr_ifndef:
				{
					// Only the name and the comments are allowed after the statement token. Scanning function
					// will also scan till the end of the startement.
					ScanIfdefExpression(cn, old_level+stmt_shift, ((stmt_type == mpr_ifndef) ? TRUE : FALSE), token_lex,
									scan_error_cn, scan_res_details, expr_area, stmt_end_area, &unsimplified_expr_code);
					expr_res = EvaluateCondExprTree(scan_error_cn, smpl_stt, &expr_area, unsimplified_expr_code);
				}
				break;

		case mpr_if:
		case mpr_elif:
				{
					// General form of an arithmetic expression can be present here. Scanning function will
					// also scan till the end of the statement.
					ScanCondExpression(cn, old_level+stmt_shift, scan_error_cn, scan_res_details,
									expr_area, stmt_end_area, &unsimplified_expr_code);
					expr_res = EvaluateCondExprTree(scan_error_cn, smpl_stt, &expr_area, unsimplified_expr_code);
				}
				break;

		case mpr_else:
				{
					// The semantics of the "#else" statement is identical to the semantics of the "#elif 1" statement.
					expr_res = cexr_hard_true;

					// Allow lexemas to follow the statement token.
					ScanTillEol(token_lex, TRUE, stmt_end_area);

					// Place empty expression area at the end of the token.
					expr_area = token_lex.origin;
					expr_area.AdjustToAreaEnd();
				}
				break;

		case mpr_endif:
				{
					// Allow lexemas to follow the statement token.
					ScanTillEol(token_lex, TRUE, stmt_end_area);

					expr_area = token_lex.origin;
					expr_area.AdjustToAreaEnd();
				}
				break;

		default:
			{
				assert(FALSE);
				ScanTillEol(token_lex, TRUE, stmt_end_area);
			}
			break;
	}

	// Calculate the whole stmt area.
	PrEventHeader whole_stmt_area = token_lex.origin;
	whole_stmt_area.ExtendPrHeader(stmt_end_area);

	//
	// Step3: Update the state of the conditional compilation parser.
	//
	ID prev_stmt_id = 0;

	// Update the stack cell.
	switch (stmt_type)
	{
		case mpr_ifdef:
		case mpr_ifndef:
		case mpr_if:
				{
					// Open a new nesting level. Fill in the cell.
					TCondCompLevel &lev_info = cond_stk[cond_cnt];
					lev_info.area_cn = cn;
					lev_info.code_stts = expr_res;
					lev_info.elif_hist = expr_res;
					lev_info.else_flag = FALSE;
					cond_cnt++;
				}
				break;

		case mpr_elif:
		case mpr_else:
				{
					// Update the contents of the same cell.
					TCondCompLevel &lev_info = cond_stk[cond_cnt-1];
					TConditionalStatus old_hist = lev_info.elif_hist;
					prev_stmt_id = lev_info.area_cn;
					lev_info.area_cn = cn;
					lev_info.code_stts = g_CodeStatusFromHistoryAndExprResult[old_hist][expr_res];
					lev_info.elif_hist = g_NewHistoryFromHistoryAndExprResult[old_hist][expr_res];

					if (stmt_type == mpr_else)
						lev_info.else_flag = TRUE;
				}
				break;

		case mpr_endif:
				{
					// The cell is about to be freed, there is nothing to update there.
					prev_stmt_id = cond_stk[cond_cnt-1].area_cn;
					cond_cnt--;
				}
				break;

		default:
			assert(FALSE);
			break;
	}

	// Calc the cond parsing integral properties using the contents of the stack.
	TConditionalStatus stmt_code_status = cond_pars_status;
	SetupIntegralCondParsingState(cond_cnt);
	cond_area_cn = (stmt_type != mpr_endif) ? cn : GetCondRefForLevel(cond_cnt);

	//
	// Step4: Notifications.
	//

	// Send the structural callback.
	PrCondParsingSttChange info;
	info.hdr.Setup(&whole_stmt_area, cn);
	info.hdr.cond_ref = GetCondRefForLevel(old_level+stmt_shift);

	info.prev_stmt_id = prev_stmt_id;
	info.stmt_type = stmt_type;
	info.expr_smpl_res = smpl_stt;
	info.expr_res = expr_res;

	info.stmt_code_stt = stmt_code_status;
	info.new_code_stt = cond_pars_status;
	info.stmt_code_level = old_level+stmt_shift;
	info.new_code_level = cond_cnt;

	info.scan_expr_error = scan_error_cn;
	info.scan_expr_details = scan_res_details;
	info.expr_code = unsimplified_expr_code;
	info.mpr_token = token_lex.origin;
	info.expr_area = expr_area;

	cbk_handler->CondParsSttChangeNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Update the summary.
	if (stmt_type != mpr_endif)
		summary.cond_parsing_areas++;

	// Free local resources. Check before deleting because notification function is allowed to grab
	// the code of the expression.
	if (info.expr_code != NULL)
		delete info.expr_code;

	// Trace this statement.
	if (trace_mask & sctr_stmt_cond_pars)
	{
		TraceFuncExit(&stmt_end_area, TPRF_STMT_CONDPR, L"CN=%lu, StmtType=%s ExprRes=%s", cn, TLexema::GetKeywordIdEnumName(stmt_type), g_CondStatusName[expr_res]);
	}
}

void TMidLevScanner::ProcessPragmaStmt(TLexema &token_lex)
{
	if (trace_mask & sctr_stmt_pragma_error)
	{
		TraceFuncEnter(&token_lex.origin, TPRF_STMT_PRAGMA);
	}

	// Pick up the next lexema. It is expected to be a name.
	TLexema pragma_type_lex;
	TPragmaType pragma_type = pragma_type_other;
	if (GetPrStmtMidLex(pragma_type_lex) != ltx_name)
	{
		AddError((pragma_type_lex.origin.lin_area.linear_len > 0) ? &pragma_type_lex.origin : &token_lex.origin, mscn_err_ostm_pragma_no_type);
		pragma_type = pragma_synt_error;
	}

	// Scan the rest of the line.
	PrEventHeader stmt_end_area;
	ScanTillEol(pragma_type_lex, TRUE, stmt_end_area);

	// Check type of the #pragma statement. Some of the pragmas are recognized and processed right here.
	wchar_t pragma_type_name[80];
	if (pragma_type != pragma_synt_error)
	{
		// Process contents of the statement.
		if (pragma_type_lex.str_value == L"once")
		{
			TFileInfoItem *file_item = src_stack[src_cnt-1].file_context;
			if (file_item == NULL)
			{
				// This is either a temp file or this is an internal error.
				AddError(&pragma_type_lex.origin, mscn_err_ostm_pragma_bad_once);
				pragma_type = pragma_synt_error;
			}
			else
			{
				// The current source is a regular disk file.
				if (file_item->pragma_once_noticed == TRUE)
					AddWarning(&pragma_type_lex.origin, mscn_err_ostm_pragma_once_dup);

				file_item->pragma_once_noticed = TRUE;
				pragma_type = pragma_type_once;
			}
		}
		else if (pragma_type_lex.str_value == L"note" || pragma_type_lex.str_value == L"NOTE")
		{
			// This is a special feature of this parser. It allows to place a note (warning) when parsing
			// passes a certain place.
			AddWarning(&pragma_type_lex.origin, mscn_err_ostm_pragma_note);
			pragma_type = pragma_type_note;
		}

		// Extract the name.
		pragma_type_lex.str_value.CopyWithTruncationTo(pragma_type_name, 80);
	}
	else
	{
		// Assign some explanation of what had happened.
		wcscpy(pragma_type_name, L"<The type of pragma is missing>");
	}

	// Send the structural notification (#pragma).
	ID cn = GetNextCN();
	PrMiscPreprocStmtEvent info;
	info.Prepare(cn, exmis_pragma, cond_pars_status, token_lex.origin, stmt_end_area);
	info.evt_details = pragma_type;
	info.evt_str_data = pragma_type_name;
	cbk_handler->MiscPreprocEventNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Trace this statement.
	if (trace_mask & sctr_stmt_pragma_error)
	{
		TraceFuncExit(&stmt_end_area, TPRF_STMT_PRAGMA, L"CN=%lu, Type=%s.", cn, pragma_type_name);
	}
}

void TMidLevScanner::ProcessErrorStmt(TLexema &token_lex)
{
	if (trace_mask & sctr_stmt_pragma_error)
	{
		TraceFuncEnter(&token_lex.origin, TPRF_STMT_ERROR);
	}

	// Skip non empty lexemas till the end of the line.
	PrEventHeader stmt_end_area;
	ScanTillEol(token_lex, TRUE, stmt_end_area);

	// Prepare the callback data.
	ID cn = GetNextCN();
	PrMiscPreprocStmtEvent info;
	info.Prepare(cn, exmis_error, cond_pars_status, token_lex.origin, stmt_end_area);
	info.evt_details = (active_code == TRUE && options.ignore_error_stmt == FALSE) ? error_stmt_abort : error_stmt_cont;

	// Prepare a string that will contain the whole statement. This string can be truncated in an OOM case.
	TSourceArea &area = info.hdr.src_area;
	TStringPtr stmt_str_ptr(area.file_info->info.file_body+area.area_beg, area.area_len);
	TTextBuffer80 stmt_str_buff;
	if (area.AreaEnd() > token_lex.origin.src_area.AreaEnd())
		ConvertToTextBuffer(&token_lex.origin, stmt_str_buff, stmt_str_ptr);
	else stmt_str_buff.Append(L"<An error explanation text is missing>");

	// Send the structural notification (#error).
	info.evt_str_data = stmt_str_buff.DataPtr();
	cbk_handler->MiscPreprocEventNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Process the statement.
	if (active_code == TRUE)
	{
		// Either abort the parsing or just write an error.
		if (options.ignore_error_stmt == FALSE)
		{
			AbortParsingInternal(&token_lex.origin, mscn_err_ostm_error_stmt, lerrc_preproc_err);
		}
		else
		{
			AddError(&token_lex.origin, mscn_err_ostm_error_stmt);
		}
	}

	// Trace this statement.
	if (trace_mask & sctr_stmt_pragma_error)
	{
		const wchar_t *dtls_msg = (active_code == FALSE) ?
						L"#error in passive code" : ((options.ignore_error_stmt == TRUE) ? L"Error posted. Continue" : L"Parsing was ABORTED");
		TraceFuncExit(&stmt_end_area, TPRF_STMT_ERROR, L"CN=%lu, Dtls=%d, %s.", cn, (int)info.evt_details, dtls_msg);
	}
}

void TMidLevScanner::ProcessImportStmt(TLexema &token_lex)
{
	if (trace_mask & sctr_stmt_misc_mid_lev)
	{
		TraceFuncEnter(&token_lex.origin, TPRF_STMT_IMPORT);
	}

	SetIncFileNameMode(TRUE);

	// Scan the line with the stmt. The next lexema should be a file name.
	TLexema fname_lex;
	TMiscStmtDtls dtls = mis_dtls_ok;
	if (GetPrStmtMidLex(fname_lex) != ltx_string)
	{
		AddError((fname_lex.origin.lin_area.linear_len > 0) ? &fname_lex.origin : &token_lex.origin, mscn_err_ostm_import_no_fname);
		dtls = mis_dtls_synt_error;
	}

	// Scan the rest of the line.
	PrEventHeader stmt_end_area;
	ScanTillEol(fname_lex, TRUE, stmt_end_area);
	SetIncFileNameMode(FALSE);

	// Process the contents of the statement.
	ID cn = GetNextCN();
	TMidLevIncludeHelper hlpr(this, FALSE, options.report_includes_from_same_dir, options.report_include_file_duplications);
	TFileInfoItem *file_item = NULL;
	if (dtls == mis_dtls_ok)
	{
		// The helper is in the "find only" mode. Non NULL return value is possible only when the file is already present in the cache.
		file_item = hlpr.LoadIncludeFile(fname_lex);
	}

	// Send the structural notification (#import).
	PrMiscPreprocStmtEvent info;
	info.Prepare(cn, exmis_import, cond_pars_status, token_lex.origin, stmt_end_area);
	info.evt_details = dtls;
	if (dtls == mis_dtls_ok)
		info.PrepareFileNameSection(fname_lex, hlpr, file_item);

	cbk_handler->MiscPreprocEventNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Trace this statement.
	if (trace_mask & sctr_stmt_misc_mid_lev)
	{
		TraceFuncExit(&stmt_end_area, TPRF_STMT_IMPORT, L"CN=%lu, Dtls=%d.", cn, (int)dtls);
	}
}

void TMidLevScanner::ProcessLineStmt(TLexema &token_lex)
{
	if (trace_mask & sctr_stmt_misc_mid_lev)
	{
		TraceFuncEnter(&token_lex.origin, TPRF_STMT_LINE);
	}

	// Set the file name mode to TRUE in advance for the case if line num will be missing and
	// the file name will go directly.
	SetIncFileNameMode(TRUE);

	// The next lexema should be either a line number or a file name.
	TLexema line_num_lex, fname_lex;
	TMiscStmtDtls dtls = mis_dtls_ok;
	PrEventHeader stmt_end_area;
	if (GetPrStmtMidLex(line_num_lex) == ltx_number)
	{
		// The number is present on the line. Pick up the next lexema
		GetPrStmtMidLex(fname_lex);
		if (fname_lex.type != ltx_string && fname_lex.type != ltx_comment && fname_lex.type != ltx_eol)
		{
			// Something is present. This something is neither a string nor something skippable.
			AddError((fname_lex.origin.lin_area.linear_len > 0) ? &fname_lex.origin : &line_num_lex.origin, mscn_err_ostm_line_no_fname);
		}

		ScanTillEol(fname_lex, TRUE, stmt_end_area);
		SetIncFileNameMode(FALSE);

		if (fname_lex.type != ltx_string)
			fname_lex.Clear();
	}
	else
	{
		// The line number is missing. This is a syntax error.
		AddError((line_num_lex.origin.lin_area.linear_len > 0) ? &line_num_lex.origin : &token_lex.origin, mscn_err_ostm_line_no_lnum);

		if (line_num_lex.type == ltx_string)
		{
			// Assume that the line number is missing and the picked up lexema is a file name.
			fname_lex = line_num_lex;
		}

		ScanTillEol(line_num_lex, TRUE, stmt_end_area);
		SetIncFileNameMode(FALSE);

		line_num_lex.Clear();
		dtls = mis_dtls_synt_error;
	}

	TTextBuffer80 str_val_buffer;
	if (fname_lex.type == ltx_string)
	{
		// Prepare the statement string. The string might be truncated in an OOM case.
		TStringPtr loc_str_ptr(fname_lex.str_value);
		ConvertToTextBuffer(&stmt_end_area, str_val_buffer, loc_str_ptr);
	}
	else
	{
		// This operation cannot fail.
		str_val_buffer.Append(L"n/a");
	}

	// Process the contents of the statement.
	ID cn = GetNextCN();
	TMidLevIncludeHelper hlpr(this, FALSE, options.report_includes_from_same_dir, options.report_include_file_duplications);
	TFileInfoItem *file_item = NULL;
	if (fname_lex.type == ltx_string)
		file_item = hlpr.LoadIncludeFile(fname_lex);

	// Send the structural notification (#line).
	PrMiscPreprocStmtEvent info;
	info.Prepare(cn, exmis_line, cond_pars_status, token_lex.origin, stmt_end_area);
	info.evt_details = dtls;

	if (line_num_lex.type == ltx_number)
	{
		info.line_num  = (long)line_num_lex.num_value;
		info.line_area = line_num_lex.origin;
	}

	if (fname_lex.type == ltx_string)
	{
		info.PrepareFileNameSection(fname_lex, hlpr, file_item);
	}

	cbk_handler->MiscPreprocEventNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Trace this statement.
	if (trace_mask & sctr_stmt_misc_mid_lev)
	{
		TraceFuncExit(&stmt_end_area, TPRF_STMT_LINE, L"CN=%lu, Line=%d.", cn, (dtls == mis_dtls_ok) ? (int)line_num_lex.num_value : 0);
	}
}

void TMidLevScanner::ProcessUsingStmt(TLexema &token_lex)
{
	if (trace_mask & sctr_stmt_misc_mid_lev)
	{
		TraceFuncEnter(&token_lex.origin, TPRF_STMT_USING);
	}

	SetIncFileNameMode(TRUE);

	// Scan the line with the statement. The next lexema should be a file name.
	TLexema fname_lex;
	TMiscStmtDtls dtls = mis_dtls_ok;
	if (GetPrStmtMidLex(fname_lex) != ltx_string)
	{
		AddError((fname_lex.origin.lin_area.linear_len > 0) ? &fname_lex.origin : &token_lex.origin, mscn_err_ostm_using_no_fname);
		dtls = mis_dtls_synt_error;
	}

	// Scan the rest of the line.
	PrEventHeader stmt_end_area;
	ScanTillEol(fname_lex, TRUE, stmt_end_area);
	SetIncFileNameMode(FALSE);

	// Process the contents of the statement.
	ID cn = GetNextCN();
	TMidLevIncludeHelper hlpr(this, FALSE, options.report_includes_from_same_dir, options.report_include_file_duplications);
	TFileInfoItem *file_item = NULL;
	if (dtls == mis_dtls_ok)
	{
		// The helper is in the "find only" mode. Non NULL return value is possible only when the file is already present in the cache.
		file_item = hlpr.LoadIncludeFile(fname_lex);
	}

	// Send the structural notification (#using).
	PrMiscPreprocStmtEvent info;
	info.Prepare(cn, exmis_using, cond_pars_status, token_lex.origin, stmt_end_area);
	info.evt_details = dtls;
	if (dtls == mis_dtls_ok)
		info.PrepareFileNameSection(fname_lex, hlpr, file_item);

	cbk_handler->MiscPreprocEventNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	// Trace this statement.
	if (trace_mask & sctr_stmt_misc_mid_lev)
	{
		TraceFuncExit(&stmt_end_area, TPRF_STMT_USING, L"CN=%lu, Dtls=%d.", cn, (int)dtls);
	}
}

void TMidLevScanner::ScanCondExpression(ID stmt_cn, int stmt_cond_lev,
							ID &error_cn, TCondExprScanningDetails &scan_dtls,
							PrEventHeader &expr_area, PrEventHeader &stmt_end_area,
							TCondExprTreeItem **ppcode)
{
	assert(soft_macro_expansion_mode == FALSE);

	// Instantiate the scanner and scan the expression.
	TCondExpressionScanner expr_scanner(*this);
	TLexema lex_after_expr;
	error_cn = expr_scanner.ScanExpression(stmt_cn, stmt_cond_lev, cescm_process_commas, scan_dtls, expr_area, lex_after_expr, ppcode);
	assert(soft_macro_expansion_mode == FALSE);

	// Scan the remaining part of the line if any.
	ScanTillEol(lex_after_expr, (error_cn == 0) ? FALSE : TRUE, stmt_end_area);
}

void TMidLevScanner::ScanIfdefExpression(ID stmt_cn, int stmt_cond_lev,
							bool ifndef_stmt, TLexema &token_lex,
							ID &error_cn, TCondExprScanningDetails &scan_dtls,
							PrEventHeader &expr_area, PrEventHeader &stmt_end_area,
							TCondExprTreeItem **ppcode)
{
	error_cn = 0;
	*ppcode = NULL;

	// Scan the name of the macro to test. Allow comments to stay in front of the name.
	// MS compiler is doing similar thing.
	TLexema name_lex;
	GetPrStmtLoLex(name_lex);
	expr_area = name_lex.origin;

	// Scan the rest of the string.
	ScanTillEol(name_lex, TRUE, stmt_end_area);

	// Check if the retrieved lexema is a name or not.
	if (name_lex.type != ltx_name)
	{
		error_cn = AddError(&name_lex.origin, mscn_err_cond_ifdef_no_name);
		scan_dtls = (TCondExprScanningDetails)(scan_dtls | cescd_syntax_error);
		return;
	}

	// Find the indexing key for this name.
	bool malloc_problem = FALSE;
	TKeyInfoItem *key = GetIndexingKeyFromLexEx(name_lex, error_cn, malloc_problem);
	if (key == NULL)
	{
		// An error is already reported by the expression scanner.
		if (error_cn != 0)
			scan_dtls = (TCondExprScanningDetails)(scan_dtls | cescd_syntax_error);

		if (malloc_problem == TRUE)
		{
			error_cn = mscn_err_misc_out_of_memory;
			scan_dtls = (TCondExprScanningDetails)(scan_dtls | cescd_malloc_problem);
		}

		return;
	}

	//
	// The name and its indexing key were succesfully retrieved. Create 3 pieces of code.
	// One of them will maybe not used.
	//
	//    oprnd -- name code item.
	//    code1 -- defined operation.
	//    code2 -- bang operation.
	//
	TLexema operation_lex = token_lex;
	TCondExprTreeItem *oprnd = CreateNameTreeItem(stmt_cn, src_cnt, stmt_cond_lev, key, name_lex, scan_dtls);

	operation_lex.SetKeyword(ltkn_arithm, opr_defined);
	TCondExprTreeItem *code1 = new TCondExprTreeItem(operation_lex, ceprt_unary);
	if (code1 != NULL && code1->AddSourceLexema(operation_lex, TCondExprTreeItem::alm_primary) == FALSE)
	{
		// Error in preparing the first code item.
		delete code1;
		code1 = NULL;
	}

	operation_lex.SetKeyword(ltkn_arithm, opr_bang);
	TCondExprTreeItem *code2 = new TCondExprTreeItem(operation_lex, ceprt_unary);
	if (code2 != NULL && code2->AddSourceLexema(operation_lex, TCondExprTreeItem::alm_primary) == FALSE)
	{
		// Error in preparing the second code item.
		delete code2;
		code2 = NULL;
	}

	// Check the results of allocation.
	if (oprnd == NULL || code1 == NULL || code2 == NULL)
	{
		error_cn = AddError(&name_lex.origin, mscn_err_cond_oom_for_code);

		if (oprnd != NULL)
			delete oprnd;
		if (code1 != NULL)
			delete code1;
		if (code2 != NULL)
			delete code2;

		scan_dtls = (TCondExprScanningDetails)(scan_dtls | cescd_malloc_problem);
		return;
	}

	// Combine the resulting code out of the pieces and give it out.
	code1->AddOperand(oprnd);
	if (ifndef_stmt == TRUE)
	{
		code2->AddOperand(code1);
		*ppcode = code2;
	}
	else
	{
		*ppcode = code1;
		delete code2;
	}
}

TCondExprTreeItem *TMidLevScanner::CreateNameTreeItem(ID stmt_cn, int stmt_src_lev,  int stmt_cond_lev,
														TKeyInfoItem *key_info, TLexema &name_lexema, TCondExprScanningDetails &dtls)
{
	assert(name_lexema.type == ltx_name);
	TMacroDefinition  *macro = macrodefs_list.LookupMacro(name_lexema.str_value);
	TCondExprTreeItem *operand = NULL;

	if (macro != NULL)
	{
		//
		//  This name is known at least somehow.
		//
		if (macro->redefinitions_history.IsEmpty() == FALSE)
		{
			// This name was redefined at least once.
			if (macro->def_origin_type == mdo_predefined)
				dtls = (TCondExprScanningDetails)(dtls | cescd_redefined_predef);

			AddWarning(&name_lexema.origin, mscn_err_cond_rdef_cond_used);
		}

		// Create decoration code item for this the existing macro definition.
		operand = new TCondExprTreeItem(key_info, name_lexema, macro, TRUE);
		if (operand == NULL || operand->AddSourceLexema(name_lexema, TCondExprTreeItem::alm_primary) == FALSE)
		{
			return(NULL);
		}
	}
	else
	{
		//
		//  There is no info about this name.
		//
		dtls = (TCondExprScanningDetails)(dtls | cescd_unkn_name_def_stt);
		if (options.report_def_opers_with_unkn_names == TRUE)
		{
			AddWarning(&name_lexema.origin, mscn_err_cond_unkn_name_used);
		}

		TLexemaListItem *name_item = new TLexemaListItem(name_lexema);
		if (name_item == NULL)
		{
			return(NULL);
		}

		// Create a variable type code item.
		TLexemasList var_body;
		var_body.AppendItem(name_item);
		operand = new TCondExprTreeItem(key_info, var_body, TRUE);
		if (operand == NULL)
		{
			var_body.DeleteAll();
			return(NULL);
		}
	}

	// Creation of the code item succeeded. Send the structural callback about the cond param use.
	key_info->BumpUsage(mlku_cprm_defstt);
	PrCondParamUseEvent info;
	info.hdr.Setup(&name_lexema.origin, GetNextCN());

	info.cond_stmt_cn = stmt_cn;
	info.lex_src_nest = stmt_src_lev;
	info.cond_pars_nest = stmt_cond_lev;
	info.code_stt = cond_pars_status;
	info.param = operand;

	cbk_handler->CondParamUseNotification(&info);
	PostProsessMidLevCbkCall(info.hdr);

	return(operand);
}

TConditionalStatus TMidLevScanner::EvaluateCondExprTree(ID &err_cn, TCondExprSmplResult &smpl_stt, PrEventHeader *err_ctx, TCondExprTreeItem *code_tree)
{
	smpl_stt = cesr_unsimpl;
	if (code_tree == NULL)
		return(cexr_undefined);

	// Create a copy of the orginal code.
	TCondExprTreeItem *code_copy = code_tree->Clone();
	if (code_copy == NULL)
	{
		ID new_err_cn = AddError(err_ctx, mscn_err_cond_oom_for_clone);
		if (err_cn == 0)
			err_cn = new_err_cn;
		return(cexr_undefined);
	}

	// Simplify the copy of the expression.
	TConditionalStatus expr_res = cexr_undefined;
	smpl_stt = code_copy->SimplifyExpression(options.expr_mode);

	// Check the results of the simplification.
	if (code_copy->item_type == cecit_value)
	{
		int out_val = 0;
		if (code_copy->value != 0)
			out_val |= 1;
		if (code_copy->on_doubt == TRUE)
			out_val |= 2;

		expr_res = (TConditionalStatus)out_val;
	}

	// Release the copy of the code.
	delete code_copy;
	return(expr_res);
}

/* protected: 3 */

TLexemaType TMidLevScanner::MidScan(TLexema &lex)
{
	//
	//    This is the most important function in the whole mid lev parser.
	// ------------------------------------------------------------------
	//

	// The paste buffering should not be active.
	assert(pbuff_cnt == 0);
	if (trace_mask & sctr_mid_scan)
	{
		TraceFuncEnter(NULL, TPRF_MIDSCAN);
	}

	for(;;)
	{
		// At least one lex source should be present.
		assert(src_cnt > 0);
		TLexemasSource &src = src_stack[src_cnt-1];

		//
		// P0: Use the saved lex or pick up the new lexema.
		//
		if (saved_lex.type != ltx_empty)
		{
			lex = saved_lex;
			saved_lex.SetEmpty();
		}
		else
		{
			GetLoLex(lex);
		}

		//
		// P1:	lex			- any type of lexema;
		//		saved_lex	- empty.
		//
		// Process ltx_empty, ltx_eof.
		//
		if (trace_mask & sctr_mid_scan)
		{
			TraceEventLexema(&lex.origin, TPRF_MIDSCAN_P1, L"Lex", lex);
		}

		if (lex.type == ltx_empty)
		{
			// This may happen only in the command line context when the new lex source is nested.
			continue;
		}
		else if (lex.type == ltx_eof)
		{
			if (pbuff_cnt == 0)
			{
				// No paste buffering is active.
				if (src.paste_suff == TRUE)
				{
					// Ignore the paste_suff from the lex source slot. Situation with no buffering and suffix is possible
					// when the lex source is empty (buffering failed to start) and the call stmt is followed with ##.
					AddWarning(&lex.origin, mscn_err_pbuff_eof_suff_nob);
				}

				// Check the EOF protection level.
				if (src_cnt == 1 || src_cnt <= src_cnt_min)
				{
					// Give this EOF lexema out.
					break;
				}

				// Exit from the current lex source and go on scanning.
				ExitFromLexSource(&lex.origin);
				continue;
			}

			//
			// Some paste buffering is active.
			//
			TPasteBufferingSlot &pb_slot = pbuff_stk[pbuff_cnt-1];

			// Pick up the fields from the current lex source before it is exited.
			int eof_src_level = src_cnt;
			bool paste_suff_call = src.paste_suff;

			// Create a copy of the current lex_src that is about to be exited. This copy is needed for passing
			// it into the EnterPasteBuffSource() as parameter.
			TLexemasSource src_proto = src;

			// The current lex source should be exited in any case.
			ExitFromLexSource(&lex.origin);

			if (pb_slot.suff_buffering == TRUE)
			{
				// Nothing special is needed. Continue adding lexemas to the current suffix based buffer.
				// Ensure that lex source was entered with the paste suffix.
				assert(paste_suff_call == TRUE);
			}
			else
			{
				//
				//  This is a level based buffering.  Pick up the new current scanning position.
				//  Note that getting the current scan pos always returns an empty area.
				//
				PrEventHeader evt_ctx;
				GetCurrScanPos(evt_ctx, lex.origin.cn);

				if (paste_suff_call == TRUE)
				{
					// This situation can happen only on the top level of the level based buffering.
					assert(eof_src_level == pb_slot.lev_buff_level);

					// Check existence and type of the upper layer buffering.
					if (pbuff_cnt == 1 || pbuff_stk[pbuff_cnt-2].suff_buffering == FALSE)
					{
						// There is no upper layer or the upper layer layer is also level based. Exit the current level based
						// buffering layer and immediately enter the suffix based buffering using the same buffer. Entering
						// the buff layer cannot fail because of the combination of param values. Nevertheless ensure this.
						TFileInfoItem *pbuff;
						ExitPasteBuffLayer(&evt_ctx, &pbuff);
						bool res = EnterPasteBuffLayer(&evt_ctx, mpr_prm_cnct, TRUE, pbuff);
						assert(res == TRUE);
					}
					else
					{
						// Current layer should share its buffer with the upper layer. Simply exit the current buffering layer.
						assert(pb_slot.buffer == pbuff_stk[pbuff_cnt-2].buffer);
						ExitPasteBuffLayer(&evt_ctx);
					}
				}
				else
				{
					// Current buffering layer is a level based and the current eof does not have a paste suffix after it.
					if (eof_src_level > pb_slot.lev_buff_level)
					{
						// It is not high time to stop the buffering.
						assert(eof_src_level > src_cnt_min);
						continue;
					}

					// Stop the current level based buffering.
					TFileInfoItem *pbuff;
					ExitPasteBuffLayer(&evt_ctx, &pbuff);

					if (pbuff_cnt == 0)
					{
						// Setup current buffer as a new source of lexemas.
						EnterPasteBuffSource(&evt_ctx, FALSE, FALSE, pbuff, &src_proto);
					}
					else if (pbuff_stk[pbuff_cnt-1].suff_buffering == TRUE)
					{
						// Exit one more buffering layer and immediately enter the curr buffer as the source of lexemas.
						TFileInfoItem *upper_pbuff;
						ExitPasteBuffLayer(&evt_ctx, &upper_pbuff);
						assert(pbuff == upper_pbuff);
						EnterPasteBuffSource(&evt_ctx, TRUE, FALSE, pbuff, &src_proto);
					}
					else
					{
						// Ensure that the upper layer shares buffer with the layer that was just exited.
						assert(pbuff == pbuff_stk[pbuff_cnt-1].buffer);
					}
				}
			}

			// Go on scanning.
			continue;
		}

		//
		// P2:	lex			- lexema of any type except for ltx_empty and ltx_eof.
		//		saved_lex	- empty.
		//
		// Process all lex types in the file context.
		//
		assert(LEX_SRC_CMDL(src.src_type) == FALSE);
		assert(lex.type == ltx_eol || lex.origin.src_area.area_len > 0);

		if (LEX_SRC_WHOLE_FILE(src.src_type) == TRUE)
		{
			// Current context is a file context.
			assert(pbuff_cnt == 0);

			// The rules in the file context are more simple. The file ctx cannot have inline macro operators and macro params.
			// Only one possible sequence of more than one lexema is the macro call.
			TMacroDefinition *mdef;
			if (lex.type == ltx_name && options.disab_macro_calls == FALSE && IsMacroName(lex.str_value, mdef) == TRUE)
			{
				PrEventHeader stmt_area = lex.origin;
				if (mdef->num_params > 0 || mdef->var_args_macro == TRUE)
				{
					// When the macro has params, its name is really a macro call only when there is an opening
					// parenthesis after the name. Otherwise it is a name lexema. Check for this. Note that parenthesis
					// does not need to stay immediately after the name of the macro like it is in the macro definition.
					TRawScanSeekInfo next_pos;
					TLexema lpar_lex;
					GetNextPosAndFilteredLoLex(src, next_pos, lpar_lex);
					if (lpar_lex.IsKeyword(opr_lpar) == TRUE)
					{
						// This is a macro call because parameters are present.
						PrEventHeader mcall_area_end;
						ScanMacroCallParams(*mdef, mcall_area_end);
						stmt_area.ExtendPrHeader(mcall_area_end);
					}
					else
					{
						// This is not a macro call. Rewind the source back. Subsequent scanning should pick up the next_lex.
						RewindMidScan(&lpar_lex.origin, src, next_pos);
						mdef = NULL;
					}
				}

				if (mdef != NULL)
				{
					// Enter the macro source. Do not check the return value. Errors are reported inside if any.
					EnterMacroSource(*mdef, &stmt_area, &lex.origin, TRUE, FALSE);
					continue;
				}
			}
			else if (lex.IsPreprocToken() == TRUE)
			{
				if ((short)lex.num_value >= mpr_prm_chr && (short)lex.num_value <= mpr_vargs)
				{
					if (lex.first_in_line == TRUE && (short)lex.num_value == mpr_prm_str)
					{
						// Check whether the current symbol "#" is followed by the EOL or not.
						TRawScanSeekInfo seek_info;
						src.scanner.GetCurrPos(seek_info);

						// Skip all comment lexemas if any.
						TLexema cont_lex;
						while (src.scanner.GetLex(cont_lex) == ltx_comment);

						// Restore the raw scaner and check the retrieved lexema.
						src.scanner.SeekTo(seek_info);
						if (cont_lex.IsEol() == TRUE || cont_lex.IsEof() == TRUE)
						{
							AddWarning(&lex.origin, mscn_err_srcst_single_hash_sym);
							continue;
						}
					}

					// This is an inline preprocessor token in the file context.
					AddError(&lex.origin, mscn_err_srcst_inln_macro_token);
					continue;
				}
			}

			// Return current lexema to the upper layer.
			assert(saved_lex.type == ltx_empty);
			break;
		}

		//
		// P3: Current context is a macro context.
		//
		//		lex			- eol or real lexema with a body;
		//		saved_lex	- empty.
		//
		// Get rid of some simple cases.
		//
		assert(LEX_SRC_MACRO_OR_SCGM(src.src_type) == TRUE);

		// Get rid of the comments and new lines.
		if (lex.type == ltx_comment || lex.type == ltx_eol)
		{
			if (pbuff_cnt > 0)
			{
				// Ignore this lexema.
				continue;
			}
			else
			{
				// Give this lexema out.
				break;
			}
		}

		// Check for some of the preprocessor tokens.
		if (lex.IsPreprocToken() == TRUE)
		{
			if ((short)lex.num_value >= mpr_define)
			{
				// Stmt preprocessor tokens in the macro context.
				AddError(&lex.origin, mscn_err_srcst_stmt_macrotkn);
				continue;
			}
			else if ((short)lex.num_value == mpr_prm_cnct)
			{
				// This lexema should be either reported as a syntax error or ignored.
				if (src.scanner.GetNumScannedLexemas() == 1)
				{
					// The ## lexema should not be the first lexema in the source.
					AddError(&lex.origin, mscn_err_pbuff_cnct_lex_first);
				}
				else
				{
					// Pick up lexema that stays after the ##.
					TRawScanSeekInfo next_lex_pos;
					GetNextPosAndFilteredLoLex(src, next_lex_pos, saved_lex);
					if (saved_lex.type == ltx_eof)
					{
						// The ## seq should not be the last lexema in the source.
						AddError(&lex.origin, mscn_err_pbuff_cnct_lex_last);
					}
					else
					{
						// Suffix based buffering should be already active.
						assert(pbuff_cnt > 0);
						assert(pbuff_stk[pbuff_cnt-1].suff_buffering == TRUE);
					}

					if (next_lex_pos.offs != saved_lex.AreaBeg())
					{
						// Continue scanning from the next_lex.
						RewindMidScan(&saved_lex.origin, src, next_lex_pos);
					}
				}

				// Ignore this lexema. In the case of the suffix based buffering the fact that the correct type
				// of buffering is active means that the lexema is processed.
				continue;
			}
		}

		//
		// P4: Current context is a macro context.
		//
		//		lex			- real lexema with a body;
		//		saved_lex	- empty.
		//
		// Look for the macro and the macro param call sequences.
		//
		// These cases do require a second lexema. Scan the current source and place lexema after the curr lexema
		// into the saved_lex.
		//
		TRawScanSeekInfo next_lex_pos;
		GetNextPosAndFilteredLoLex(src, next_lex_pos, saved_lex);
		assert(saved_lex.type != ltx_empty);

		if (trace_mask & sctr_mid_scan)
		{
			TraceEventLexema(&lex.origin, TPRF_MIDSCAN_P4, L"saved_lex", saved_lex);
		}

		// Check for simple and decorated macro param invocations.
		TMacroDefinition *mdef = NULL;
		int prm_inx = -1;
		TPreprocToken prm_decor = (TPreprocToken)0;
		PrEventHeader mcall_area, prm_decor_area;
		if (lex.type == ltx_name)
		{
			// The current lexema is a name. Check, maybe this name is a name of the macro or a macro param.
			// Check for the macro param name first.
			if (IsNamedMacroParam(lex.str_value, prm_inx) == TRUE)
			{
				// This is a simple macro param call. The variable prm_inx is inited.
				mcall_area = lex.origin;
			}
			else if (IsMacroName(lex.str_value, mdef) == TRUE)
			{
				if (mdef->num_params == 0 && mdef->var_args_macro == FALSE)
				{
					// This is a simple macro call that cannot have params.
					mcall_area = lex.origin;
				}
				else if (saved_lex.IsKeyword(opr_lpar) == TRUE)
				{
					// The current sequence is a macro call with parameters.
					mcall_area = lex.origin;
					PrEventHeader mcall_area_end;
					ScanMacroCallParams(*mdef, mcall_area_end);
					mcall_area.ExtendPrHeader(mcall_area_end);
					GetNextPosAndFilteredLoLex(src, next_lex_pos, saved_lex);
				}
				else
				{
					// This name is not a macro. Discard the mdef.
					mdef = NULL;
				}
			}
		}
		else if (lex.IsPreprocToken() == TRUE)
		{
			if ((short)lex.num_value == mpr_prm_chr || (short)lex.num_value == mpr_prm_str)
			{
				// Current lexema is one of the param_decor elements.
				if (saved_lex.type == ltx_name && IsNamedMacroParam(saved_lex.str_value, prm_inx) == TRUE)
				{
					// This is a decorated macro param invocation.
					prm_decor = (TPreprocToken)(short)lex.num_value;
					prm_decor_area = lex.origin;
					prm_decor_area.AdjustToAreaBeg();
					mcall_area = lex.origin;
					mcall_area.ExtendPrHeader(saved_lex.origin);
					lex = saved_lex;
					GetNextPosAndFilteredLoLex(src, next_lex_pos, saved_lex);
				}
				else
				{
					// This is a bogus inline preprocessor token that is not followed by the macro param name.
					AddError(&lex.origin, mscn_err_pbuff_str_chr_param);
					if (next_lex_pos.offs != saved_lex.AreaBeg())
					{
						// Continue scanning from the next_lex.
						RewindMidScan(&saved_lex.origin, src, next_lex_pos);
					}
					continue;
				}
			}
			else
			{
				// Attempt to call var_args part of the macro params. The code above should filter out all other types
				// of macro tokens. Ensure this.
				assert((short)lex.num_value == mpr_vargs);
				TMacroDefinition *mdef = src_stack[src_cnt-1].macro_context;
				if (LEX_SRC_MACRO_OR_PBUFF_MACRO(src.src_type) == TRUE && mdef->var_args_macro == TRUE)
				{
					// This context may have var_args.
					if (mdef->num_passed_params > mdef->num_params)
					{
						// There are non empty var_arg params to invoke. Give out an index of the first var_arg param.
						// EnterMacroParamSource() and ExitLexSource() will do the rest of the work.
						prm_inx = mdef->num_params;
						mcall_area = lex.origin;
					}
					else
					{
						// There is nothing to call.
						if (saved_lex.IsKeyword(mpr_prm_cnct) == TRUE)
						{
							// Both lexemas should be ignored.
							saved_lex.SetEmpty();
						}
						else if (next_lex_pos.offs != saved_lex.AreaBeg())
						{
							// Continue scanning from the next_lex.
							RewindMidScan(&saved_lex.origin, src, next_lex_pos);
						}
						continue;
					}
				}
				else
				{
					// Var_args token stays in the wrong context.
					AddError(&lex.origin, mscn_err_srcst_varg_bad_ctx);
					if (next_lex_pos.offs != saved_lex.AreaBeg())
					{
						// Continue scanning from the next_lex.
						RewindMidScan(&saved_lex.origin, src, next_lex_pos);
					}
					continue;
				}
			}
		}
		else if (lex.IsKeyword(opr_comma) == TRUE && saved_lex.IsKeyword(mpr_vargs) == TRUE)
		{
			// Check for special case when the var_args can be present and they are missing. Both lexemas
			// should be ignored in this case.
			TMacroDefinition *mdef = src_stack[src_cnt-1].macro_context;
			if (mdef->var_args_macro == TRUE && mdef->num_passed_params <= mdef->num_params)
			{
				// Ignore these lexemas.
				saved_lex.SetEmpty();
				continue;
			}
		}

		//
		// P5: The scanned sequence can be:
		//
		//		--	non macro_call/mparam_call lexema stored in lex;
		//		--	macro param call (if prm_inx >= 0);
		//		--	macro call (if mdef != NULL);
		//
		// Check if the saved_lex contains buffering instructions or not.
		//
		bool paste_suff = FALSE;
		PrEventHeader paste_suff_origin;
		if (saved_lex.IsKeyword(mpr_prm_cnct) == TRUE)
		{
			// A cnct suffix is present. Check if the suffix based buffering should be started or continued.
			// The ## lexema will be accepted in any case. So, it is fine that the next call will clobber the saved_lex.
			PrEventHeader saved_origin = saved_lex.origin;
			GetNextPosAndFilteredLoLex(src, next_lex_pos, saved_lex);
			if (saved_lex.type == ltx_eof)
			{
				// Paste suffix is not allowed before the end of the macro.
				AddError(&saved_origin, mscn_err_pbuff_cnct_lex_last);
			}
			else
			{
				// Both suffix flags get the same value.
				paste_suff = TRUE;
				paste_suff_origin = saved_origin;
			}
		}
		else if (saved_lex.type == ltx_eof)
		{
			// Use inherited paste suffix instead of the inline paste suffix.
			paste_suff = src.paste_suff;
			paste_suff_origin = saved_lex.origin;
		}

		// Adjust the length of the paste buff lex source if needed.
		long saved_buff_len = -1;
		if (LEX_SRC_GEN_PBUFF(src.src_type) == TRUE)
		{
			// Current context is the paste buffer.
			TIntermFileBody *pbb = src.file_context->interm_file_body;
			assert(pbb != NULL);

			// Check if the saved_lex stays in the second part of the buffer or not.
			if (pbb->GetPureAreaBeg() >= 0 && next_lex_pos.offs >= pbb->GetPureAreaBeg())
			{
				//
				// Part of the paste buffer that was generated by pasting is completely scanned. Next_lex_pos stays
				// in the never scanned tail of the lex source where the pasting ended. Adjust the length of the paste
				// buffer to the next_lex_pos. This will actually put the paste buff source into the EOF state.
				//
				if (trace_mask & sctr_file_retr)
				{
					TraceRawUnscan(&lex.origin, L"Pseudo eof is placed at", next_lex_pos);
				}

				pbb->Truncate(next_lex_pos.offs);
				saved_buff_len = src.scanner.buff_length;
				src.scanner.SeekTo(next_lex_pos);
				src.scanner.buff_length = next_lex_pos.offs;
				saved_lex.SetEmpty();
			}
		}

		//
		// P6: Process macro and macro param invocations.
		//
		if (trace_mask & sctr_mid_scan)
		{
			// Trace out the results of checking the suffix.
			wchar_t buff[80];
			if (saved_buff_len != -1)
				swprintf(buff, 80, L", saved_buff_len=%ld", saved_buff_len);
			else buff[0] = 0;

			TraceEvent(&lex.origin, TPRF_MIDSCAN_P6, L"paste_suff=%s%s.", (paste_suff == TRUE) ? L"TRUE" : L"false", buff);
		}

		if (mdef != NULL)
		{
			//
			// Call the macro.
			//

			if (saved_lex.type != ltx_empty)
			{
				// Discard the saved_lex because it stays after the macro call.
				RewindMidScan(&saved_lex.origin, src, next_lex_pos, L"Scan after return from");
			}

			// Do not check the return value. Errors will be reported inside if any.
			EnterMacroSource(*mdef, &mcall_area, &lex.origin, TRUE, paste_suff);
			continue;
		}
		else if (prm_inx != -1)
		{
			//
			// Call the macro param.
			//

			if (prm_decor != 0)
			{
				// Enter a new buffering layer.
				if (EnterPasteBuffLayer(&prm_decor_area, prm_decor, paste_suff) == FALSE)
				{
					// An error is already displayed.
					continue;
				}
			}

			if (saved_lex.type != ltx_empty)
			{
				// Discard the saved_lex because it stays after the macro call.
				RewindMidScan(&saved_lex.origin, src, next_lex_pos, L"Scan after return from");
			}

			// Figure out if this macro should be expanded or not.
			bool upper_level_pbuff = (pbuff_cnt > 0 && pbuff_stk[pbuff_cnt-1].suff_buffering == FALSE);
			bool xpnd_macros = prm_decor == 0 && upper_level_pbuff == FALSE;

			// Do not check the return value. Errors will be reported inside if any.
			EnterMacroParamSource(&mcall_area, prm_inx, prm_decor, xpnd_macros, paste_suff);
			continue;
		}

		//
		// P7: lex - pseudo terminal lexema.
		//

		// Check, whether the suffix based buffering should be started or continued.
		if (paste_suff == TRUE && src.mparam_decor == 0)
		{
			// Start a new suffix based buffering or continue an existing one.
			if (pbuff_cnt == 0 || pbuff_stk[pbuff_cnt-1].suff_buffering == FALSE)
			{
				// Open a new suffix based buffering.
				PrEventHeader event_area = lex.origin;
				event_area.AdjustToAreaBeg();
				if (EnterPasteBuffLayer(&event_area, mpr_prm_cnct, TRUE) == FALSE)
				{
					// An error is already displayed.
					continue;
				}
			}

			// The buffering layer and the interm buffer are available. Add current lexema to the buffer.
			assert(pbuff_cnt > 0);
			TIntermFileBody *pbuff_body = pbuff_stk[pbuff_cnt-1].buffer->interm_file_body;
			if (pbuff_body->AddToBuffer(src.file_context, src.scanner.src, lex.origin) == FALSE)
			{
				// Pasting lexema failed.
				AddError(&lex.origin, mscn_err_pbuff_lexema_failure);
			}

			// Current lexema was successfully added to the buffer.
			continue;
		}

		// Check whether the buffering is active or not. If so, add current lexema to the buffer.
		if (pbuff_cnt > 0)
		{
			// Some buffering is active.
			TPasteBufferingSlot &pb_slot = pbuff_stk[pbuff_cnt-1];

			// Check the type of this buffering.
			if (pb_slot.suff_buffering == TRUE)
			{
				//
				//  Situation looks like:
				//
				//    a ## b ## lex something
				//
				// Abandon the current lexema (lex). It will be rescanned again because the tail of the current
				// source will be appended to the buffer and rescanned when this buffer will be entered.
				//
				src.scanner.SeekToLexBeg(lex, include_fname_mode);
				saved_lex.SetEmpty();

				if (trace_mask & sctr_file_retr)
				{
					TraceRawUnscan(&saved_lex.origin, L"Potential rescan from", lex);
				}

				if (saved_buff_len != -1)
				{
					//
					// In fact the current source is also a suffix based buffer. This is pretty rare situation,
					// this looks like the following:
					//
					//    2 ## #x ## 7
					//
					// In the deepest case the buff leveling here will be 3. Restore the scanning area length
					// and the length of the interm file.
					//
					src.scanner.buff_length = saved_buff_len;
					src.file_context->interm_file_body->RevertLatestTruncation();
				}

				// Stop the buffering and enter the resulting buffer.
				TFileInfoItem *pbuff_buff;
				lex.origin.AdjustToAreaBeg();
				ExitPasteBuffLayer(&lex.origin, &pbuff_buff);
				EnterPasteBuffSource(&lex.origin, TRUE, TRUE, pbuff_buff);
			}
			else
			{
				// Place current lexema into the buffer after a space or frame opening char and go on.
				TIntermFileBody *pbuff_body = pb_slot.buffer->interm_file_body;
				wchar_t ch = pb_slot.GetDelimChar();
				if (pbuff_body->AddToBuffer(&ch, 1) == FALSE)
				{
					// Pasting the delimiter char failed.
					AddError(&lex.origin, (ch == L' ') ? mscn_err_pbuff_delim_failure : mscn_err_pbuff_quote_failure);
					continue;
				}

				if (pbuff_body->AddToBuffer(src.file_context, src.scanner.src, lex.origin) == FALSE)
				{
					// Pasting lexema failed.
					AddError(&lex.origin, mscn_err_pbuff_lexema_failure);
				}
			}

			// Current lexema was added to the buffer.
			continue;
		}

		//
		// P8: Give out the current lexema.
		//
		break;
	}

	if (trace_mask & sctr_mid_scan)
	{
		TraceFuncExitLexema(&lex.origin, TPRF_MIDSCAN, L"Lex", lex);
	}

	// Paste buffering should not be active.
	assert(pbuff_cnt == 0);
	assert(lex.type != ltx_empty);
	return(lex.type);
}

void TMidLevScanner::RewindMidScan(PrEventHeader *ctx, TLexemasSource &src, TRawScanSeekInfo &pos_info, const wchar_t *ext_msg)
{
	// This method can be called only from the file context or from the macro context.
	src.scanner.SeekTo(pos_info);
	if (trace_mask & sctr_file_retr)
	{
		TraceRawUnscan(ctx, (ext_msg != NULL) ? ext_msg : L"Rescan starts at", pos_info);
	}

	saved_lex.SetEmpty();
}

TLexemaType TMidLevScanner::GetLoLex(TLexema &lex)
{
	assert(src_cnt > 0);
	TLexemasSource &src = src_stack[src_cnt-1];
	TLexemaType type = ltx_empty;

	if (LEX_SRC_CMDL(src.src_type) == TRUE)
	{
		// This is a command line. This type of source is handled with the command line raw scanner. Process command line
		// lexemas with a loop because they cannot be used as part of the macro constructs or macroprocessor statements.
		TCmdlLexema cmdl_lex;
		for(;;)
		{
			int curr_lex_src_cnt = src_cnt;
			src.cmdl_scanner.GetLex(cmdl_lex);
			assert(cmdl_lex.type != cmdl_empty);

			if (cmdl_lex.type == cmdl_pos_param)
			{
				// Assign the pos param index. This will also update the parsing summary.
				// Note that the pos param index is zero based and the name of the application is param[0].
				cmdl_lex.num_value = summary.cmdl_pos_params++;
			}

			// Fill in the mid level part of lexema origin.
			SetupMidLevOrigin(cmdl_lex.key_name_origin, src);
			SetupMidLevOrigin(cmdl_lex.value_origin, src);

			// Trace this lexema.
			if (trace_mask & sctr_cmdl_retr)
			{
				wchar_t lexema_buff[128];
				TraceEvent(&cmdl_lex.value_origin, TPRF_CMDL_RAW_LEX, L"CmdlLexema: %s", cmdl_lex.SprintfLexema(lexema_buff, 128));
			}

			if (cmdl_lex.type == cmdl_comment)
			{
				// The lexema is processed.
				continue;
			}

			// Check for lexema with an application name.
			if (src_cnt == 1 && src.cmdl_scanner.GetNumScannedLexemas() == 1)
			{
				// First lexema should be an application name.
				if (cmdl_lex.type == cmdl_pos_param)
				{
					// The name of the app is found. Make a zero terminated string. It might be truncated in an OOM case.
					TTextBuffer80 app_name_buffer;
					ConvertToTextBuffer(&cmdl_lex.value_origin, app_name_buffer, cmdl_lex.str_value);

					// Send the mid lev event (exmis_app_name).
					PrMiscPreprocStmtEvent info;
					info.Prepare(GetNextCN(), exmis_app_name, cond_pars_status, cmdl_lex.value_origin, cmdl_lex.value_origin);
					info.mpr_token.Clear();

					info.evt_str_data = app_name_buffer.DataPtr();
					cbk_handler->MiscPreprocEventNotification(&info);
					PostProsessMidLevCbkCall(info.hdr);

					if (trace_mask & sctr_cmdl_key_param)
					{
						TraceEvent(&cmdl_lex.value_origin, TPRF_CMDL_COMPIL, L"CN=%lu, AppName=\"%s\".", info.hdr.cn, app_name_buffer.DataPtr());
					}

					// The lexema is processed.
					continue;
				}
				else
				{
					// First lexema should be the positional param. Write an error and continue processing.
					AddError(&cmdl_lex.value_origin, mscn_err_cmdl_no_app_name);
				}
			}

			// Process this command line lexema.
			switch (cmdl_lex.type)
			{
				case cmdl_key_param:
						{
							// Process the key parameter.
							if (cmdl_cbk_handler == NULL || cmdl_cbk_handler->OnKeyParam(cmdl_lex) == FALSE)
							{
								// The callback handler is missing or it is asking to perform the stnd processing.
								StdProcessKeyParam(cmdl_lex);
							}
							else if (trace_mask & sctr_cmdl_key_param)
							{
								TraceEvent(&cmdl_lex.value_origin, TPRF_CMDL_KEY, L"The key param was handled by the cmdl cbk handler.");
							}

							// Update the summary.
							summary.cmdl_key_params++;
						}
						break;

				case cmdl_pos_param:
						{
							// Process the positional parameter. The count of the pos params is already updated. This happened
							// right after picking up the lexema.
							if (cmdl_cbk_handler == NULL || cmdl_cbk_handler->OnPosParam(cmdl_lex) == FALSE)
							{
								// The callback handler is missing or it is asking for stnd processing.
								StdProcessPosParam(cmdl_lex);
							}
							else if (trace_mask & sctr_cmdl_pos_param)
							{
								TraceEvent(&cmdl_lex.value_origin, TPRF_CMDL_POS, L"The pos param was handled by the cmdl cbk handler.");
							}
						}
						break;

				case cmdl_eof:
						{
							// End of the primary or nested command line.
							if (trace_mask & sctr_cmdl_key_param)
							{
								TraceEvent(&cmdl_lex.value_origin, TPRF_CMDL_EOF, L"EOF detected.");
							}
						}
						break;

				case cmdl_error:
						{
							// Report the raw scanning error.
							AddErrorWarning(&cmdl_lex.value_origin, 0, lerrc_raw_scanner, cmdl_lex.num_value, cmdl_lex.str_value.GetBodyPtr());
							if (trace_mask & sctr_cmdl_key_param)
							{
								TraceEvent(&cmdl_lex.value_origin, TPRF_CMDL_IGNORE, L"Raw scanning error in the command line.");
							}
						}
						break;

				default:
					{
						// Unknown type of the command line lexema.
						assert(FALSE);
						if (trace_mask & sctr_cmdl_key_param)
						{
							TraceEvent(&cmdl_lex.value_origin, TPRF_CMDL_IGNORE, L"Bogus lexema type: %d.", (int)cmdl_lex.type);
						}
						break;
					}
			}

			if (cmdl_lex.type == cmdl_eof || curr_lex_src_cnt != src_cnt)
			{
				// Give this lexema to the upper layer.
				break;
			}
		}

		// This is a cmdl lexema that cannot be processes locally. Create a C/C++ lexema out of this cmdl lexema.
		if (cmdl_lex.type != cmdl_eof)
		{
			// Clear the lexema. The upper layer will call the GetLoLex() again. The return value variable is already set.
			lex.Clear();
		}
		else
		{
			// Create an EOF lexema. Eof from the source file and the cmdl eof look similar.
			lex.on_doubt = src_on_doubt;
			lex.first_in_line = FALSE;
			lex.origin = cmdl_lex.value_origin;
			type = lex.SetEndOfFileLex();
		}

		// Trace giving out the lexema that was created from the cmdl lexema.
		if (trace_mask & sctr_file_retr)
		{
			TraceEventLexema(&lex.origin, TPRF_RAW_LEX, L"Clang lexema from cmdl", lex);
		}
	}
	else
	{
		// This is a regular C/C++ file. These types of sources are processed by the TRawScanner.
		// Here is the only one place where the raw scanner is called.
		type = src.scanner.GetLex(lex);

		// Setup position of this lexema in the overall scanning process.
		lex.on_doubt = src_on_doubt;
		SetupMidLevOrigin(lex.origin, src);

		// Update the summary.
		if (type != ltx_eof)
		{
			if (LEX_SRC_GEN_PBUFF(src.src_type) == TRUE)
			{
				summary.pbuff_src_lexemas++;
			}
			else if (LEX_SRC_GEN_FILE(src.src_type) == TRUE)
			{
				summary.file_src_lexemas++;
			}
			else
			{
				summary.macro_src_lexemas++;
			}
		}

		// Trace this fact.
		if (trace_mask & sctr_file_retr)
		{
			TraceEventLexema(&lex.origin, TPRF_RAW_LEX, (include_fname_mode == TRUE) ? L"Lex (inc_fname_mode==TRUE)" : L"Lex", lex);
		}
	}

	return(type);
}

TLexemaType TMidLevScanner::GetFilteredLoLex(TLexema &lex)
{
	do { GetLoLex(lex); }
	while (CanFilterOut(lex) == TRUE);

	return lex.type;
}

TLexemaType TMidLevScanner::GetPrStmtLoLex(TLexema &lex)
{
	do { GetLoLexWithSavedLexCheck(lex); }
	while (CanFilterOut(lex));

	if (lex.type == ltx_error)
		AddErrorWarning(&lex.origin, 0, lex.error_class, (int)lex.num_value, lex.str_value.m_body);

	return lex.type;
}

TLexemaType TMidLevScanner::GetPrStmtMidLex(TLexema &lex)
{
	do { MidScan(lex); }
	while (CanFilterOut(lex));

	if (lex.type == ltx_error)
		AddErrorWarning(&lex.origin, 0, lex.error_class, (int)lex.num_value, lex.str_value.m_body);

	return lex.type;
}

bool TMidLevScanner::IsNamedMacroParam(TStrPtrInfo &name, int &prm_inx)
{
	// This method can be called in the macro context only.
	TMacroDefinition *mdef = src_stack[src_cnt-1].macro_context;
	if (mdef == NULL || mdef->num_params == 0)
	{
		prm_inx = -1;
		return(FALSE);
	}

	// Current macro context has params.
	prm_inx = mdef->IsMacroParamName(name);
	return((prm_inx >= 0) ? TRUE : FALSE);
}

bool TMidLevScanner::IsMacroName(TStrPtrInfo &name, TMacroDefinition *&mdef)
{
	if (src_stack[src_cnt-1].xpnd_macros == TRUE)
	{
		mdef = macrodefs_list.LookupMacro(name);
		if (mdef != NULL)
		{
			// The passed name is a macro. Check its status and props.
			if (GET_DEFINED_STT(mdef->def_status) == FALSE)
			{
				// This macro is currently undefined.
				mdef = NULL;
			}
			else if (mdef->macro_call_flag == TRUE)
			{
				// This macro definition is already called. Recursive macro calls are not allowed. Parser will simply
				// return name of the macro as a name lexema. Existing C/C++ code expects this.
				mdef = NULL;
			}
			else if (soft_macro_expansion_mode == TRUE && mdef->scst_body_lex.type != ltx_empty)
			{
				//
				// This is a special case when macro definition should not be expanded, but it should be used
				// as "decorated value". Conditional expression scanner will do this special processing later.
				// Although there is a complex trick when paste buffering is about to begin. In this case the macro
				// should be called. Cond expr scanner will not notice this cond param. It will deal with contents of
				// the paste buffer instead. Handling this situation in a better way is too complex.
				//
				bool upper_paste_suff = (saved_lex.IsEof() == TRUE && src_stack[src_cnt-1].paste_suff == TRUE);
				if (saved_lex.IsKeyword(mpr_prm_cnct) == FALSE && upper_paste_suff == FALSE)
				{
					// The paste buffering is not looming. Return this lexema as a name.
					mdef = NULL;
				}
			}
		}
	}
	else
	{
		// The current context is not allowing macro expansion.
		mdef = NULL;
	}

	return((mdef != NULL) ? TRUE : FALSE);
}

void TMidLevScanner::ScanMacroCallParams(TMacroDefinition &mdef, PrEventHeader &mcall_area_end)
{
	// An opening parenthesis is already scanned. Ensure that the passed macro can have params.
	assert(mdef.num_params > 0 || mdef.var_args_macro == TRUE);
	mcall_area_end.Clear();

	if (trace_mask & sctr_mid_scan)
	{
		TraceFuncEnter(NULL, TPRF_MCALL_PRMS, L"NumFormalParams=%d.", mdef.num_params);
	}

	int cnt_br = 0;
	TMacroDefinition *var_args_ctx = NULL;
	PrEventHeader param_area, delim_area;
	param_area.Clear();
	delim_area.Clear();

	// Loop on getting parameters. Note that the number of parameters in the call can differ from
	// the number of parameters in the macro definition.
	mdef.StartMacroCallParamsPassing();
	TLexSourceType src_type = src_stack[src_cnt-1].src_type;
	TMacroDefinition *macro_ctx = src_stack[src_cnt-1].macro_context;
	for(;;)
	{
		// Pull next lexema out of the source.
		TLexema lex;
		GetLoLex(lex);

		// Check, if the current lexema is part of the macro param or if it is macro param delimiter
		// or it is an end of the params list.
		switch (lex.type)
		{
			case ltx_empty:
					{
						// Entering the new lex sources should not happen here.
						assert(FALSE);
						continue;
					}

			case ltx_keyword:
					{
						switch ((short)lex.num_value)
						{
							case opr_lpar:
									{
										cnt_br++;
									}
									break;

							case opr_rpar:
									{
										if (cnt_br == 0)
										{
											// This is the closing parenthesis of the macro params list.
											delim_area = lex.origin;
											mcall_area_end = lex.origin;
										}
										else
										{
											--cnt_br;
										}
									}
									break;

							case opr_comma:
									{
										if (cnt_br == 0)
										{
											delim_area = lex.origin;
										}
									}
									break;

							case mpr_vargs:
									{
										if (param_area.source_ref != 0)
										{
											// Emit the collected lexemas into the parameter.
											AddError(&lex.origin, mscn_err_mcall_mprm_not_foll);
											delim_area = lex.origin;
											delim_area.AdjustToAreaBeg();

											if (mdef.AddCallParam(param_area, delim_area) == FALSE)
												AddError(&lex.origin, mscn_err_mcall_err_adding);

											cnt_br = 0;
											param_area.Clear();
											delim_area.Clear();
										}

										if (LEX_SRC_MACRO_OR_PBUFF_MACRO(src_type) == TRUE && macro_ctx->var_args_macro == TRUE)
										{
											// Current context is var_args macro.
											var_args_ctx = macro_ctx;
										}
										else
										{
											// Wrong type of context.
											AddError(&lex.origin, mscn_err_mcall_varg_bad_ctx);
										}

										// Do not execute the code that stays after the switch.
										continue;
									}
						}
					}
					break;

			case ltx_eol:
					{
						if (CanFilterOut(lex) == FALSE)
						{
							// Non ignorable Eol inside the macro call.
							AddError(&lex.origin, mscn_err_mcall_eol_in_mcall);

							// Return this lexema back to the lex source.
							src_stack[src_cnt-1].scanner.SeekToLexBeg(lex, include_fname_mode);
							if (trace_mask & sctr_file_retr)
							{
								TraceRawUnscan(&lex.origin, L"Rescan starts at: ", lex);
							}

							// Report an end of the call area.
							lex.origin.AdjustToAreaBeg();
							delim_area = lex.origin;
							mcall_area_end = lex.origin;
						}
					}
					break;

			case ltx_eof:
					{
						// The macro call cannot not be scattered across several lexema sources.
						AddError(&lex.origin, mscn_err_mcall_eof_in_mcall);
						delim_area = lex.origin;
						mcall_area_end = lex.origin;
					}
					break;

			default:
				{
					// All other types of lexemas including the ltx_error can be a part of the mparam
					// without additional verification.
				}
				break;
		}

		if (delim_area.source_ref == 0)
		{
			// An nnd of macro param is not detected yet.
			if (CanFilterOut(lex) == FALSE)
			{
				// Check if the var_args token is detected.
				if (var_args_ctx != NULL)
				{
					// Bogus situation when the var_args token is not followed by the proper delimiter.
					AddError(&delim_area, mscn_err_mcall_varg_not_foll);
					delim_area = lex.origin;
					delim_area.AdjustToAreaBeg();

					if (mdef.AddVargsCallParam(var_args_ctx, delim_area) == FALSE)
						AddError(&lex.origin, mscn_err_mcall_err_adding);

					var_args_ctx = NULL;
				}

				// Add current lexema to the macro param area.
				if (param_area.source_ref == 0)
				{
					// First lexema of the macro param.
					param_area = lex.origin;
				}
				else
				{
					// Extend the macro param area.
					param_area.ExtendPrHeader(lex.origin);
				}
			}
		}
		else
		{
			// Save the param info that was collected so far.
			if (var_args_ctx == NULL)
			{
				// Ordinary inline param.
				if (param_area.source_ref == 0)
				{
					// Use empty area at the beginning of the delimiter as param.
					param_area = delim_area;
					param_area.AdjustToAreaBeg();
				}

				if (mdef.AddCallParam(param_area, delim_area) == FALSE)
					AddError(&delim_area, mscn_err_mcall_err_adding);

				param_area.Clear();
			}
			else
			{
				// This is passing the var_args from one context to another.
				assert(param_area.source_ref == 0);
				if (mdef.AddVargsCallParam(var_args_ctx, delim_area) == FALSE)
					AddError(&delim_area, mscn_err_mcall_err_adding);

				var_args_ctx = NULL;
			}

			delim_area.Clear();
		}

		if (mcall_area_end.source_ref != 0)
		{
			// An end of the parameters list is detected.
			break;
		}
	}

	// Finalize setting params.
	int num_actual_params = mdef.FinishMacroCallParamsPassing();

	// Check the number of actual parameters.
	if (mdef.num_passed_params < mdef.num_params)
	{
		// An expected number of parameters was not scanned.
		wchar_t buff_scanned[40], buff_expected[40];
		AddWarning(&mcall_area_end, mscn_err_mcall_less_actual, 0,
					_itow(mdef.num_passed_params, buff_scanned, 10), _itow(mdef.num_params, buff_expected, 10));

		// Fill the rest of params with the empty areas.
		PrEventHeader dummy_prm = mcall_area_end;
		dummy_prm.AdjustToAreaBeg();

		for (int ik=mdef.num_passed_params; ik<mdef.num_params; ++ik)
			mdef.AddCallParam(dummy_prm, mcall_area_end);
	}
	else if (mdef.num_passed_params > mdef.num_params && mdef.var_args_macro == FALSE)
	{
		// The number of scanned params in a non var_args macro exceeds the number of params
		// in the macro definition.
		wchar_t buff_scanned[40], buff_expected[40];
		AddWarning(&mcall_area_end, mscn_err_mcall_more_actual, 0,
					_itow(mdef.num_passed_params, buff_scanned, 10), _itow(mdef.num_params, buff_expected, 10));
	}

	// Trace the number of actual params.
	if (trace_mask & sctr_mid_scan)
	{
		TraceFuncExit(NULL, TPRF_MCALL_PRMS, L"NumActualParams=%d.", num_actual_params);
	}
}

void TMidLevScanner::ScanTillEol(TLexema &latest_scanned, bool allow_lexemas, PrEventHeader &stmt_end_area)
{
	// This function should be used for processing the mid lev statements only.
	assert(src_cnt >= src_cnt_min);

	TLexema lex = latest_scanned;
	bool error_reported = FALSE;
	bool initial_lex = TRUE;

	// It is allowed to come here without the prescanned lex. In this case the next lexema should be picked up.
	if (lex.type == ltx_empty)
	{
		GetLoLexWithSavedLexCheck(lex);
		initial_lex = FALSE;
	}

	for(;;)
	{
		if (lex.type == ltx_eol)
		{
			if (CanFilterOut(lex) == FALSE)
			{
				// End of line is on a non auto exit lex source level. Plus this is real eol, not an escaped eol.
				break;
			}
		}
		else if (lex.type == ltx_eof)
		{
			if (src_cnt <= src_cnt_min)
			{
				// End of file is on the non auto exit lex src level.
				break;
			}

			// Exit from this source layer.
			ExitFromLexSource(&lex.origin);
		}
		else if (initial_lex == FALSE && allow_lexemas == FALSE && error_reported == FALSE)
		{
			// Write an error about the unexpected lexema.
			if (lex.type == ltx_error)
			{
				// Raw scanner can return only errors, it is not returning warnings.
				AddErrorWarning(&lex.origin, 0, lex.error_class, (int)lex.num_value, lex.str_value.m_body);
			}
			else if (lex.type >= ltx_number && lex.type <= ltx_name)
			{
				AddError(&lex.origin, mscn_err_misc_prst_extra_lex);
			}

			error_reported = TRUE;
		}

		// Pick up the next lexema.
		GetLoLexWithSavedLexCheck(lex);
		initial_lex = FALSE;
	}

	// Function returns header that contains an empty area of the place where the scanning was stopped.
	// Source nesting should be equal to the current source exit limit.
	assert(src_cnt == src_cnt_min);
	stmt_end_area = lex.origin;
	stmt_end_area.AdjustToAreaBeg();
}

void TMidLevScanner::SetIncFileNameMode(bool new_val)
{
	if (src_cnt > 0)
		src_stack[src_cnt-1].scanner.SetSpecialFileNameMode(new_val);
	include_fname_mode = new_val;
}

/* protected: 4 */

void TMidLevScanner::StdProcessKeyParam(TCmdlLexema &cmdl_lex)
{
	// This method should be called with the key params only.
	assert(cmdl_lex.type == cmdl_key_param);

	// Check for stnd key ids.
	switch (cmdl_lex.num_value)
	{
		case cmdk_analysis_tool_key:
				{
					CmdlProcessAnalysisToolKey(cmdl_lex);
				}
				break;

		case cmdk_include_path:
				{
					// Worker function will do all necessary checks.
					if (trace_mask & sctr_cmdl_key_param)
						TraceFuncEnter(&cmdl_lex.value_origin, TPRF_CMDL_PATH);

					AddIncludePath(cmdl_lex.str_value, &cmdl_lex.key_name_origin, &cmdl_lex.value_origin);

					if (trace_mask & sctr_cmdl_key_param)
						TraceFuncExit(&cmdl_lex.value_origin, TPRF_CMDL_PATH);
				}
				break;

		case cmdk_predefined_cond:
				{
					CmdlProcessDefinitionKey(cmdl_lex);
				}
				break;

		case cmdk_nested_cmdl_temp:
				{
					// All rsp files are treated as temp files for now.
					CmdlNestLexSource(cmdl_lex, strg_temp, cmdl_task_info.cmdl_rscn_type, srct_cmdl);
				}
				break;

		case cmdk_nested_src_disk:
				{
					// Nesting of the preliminary source. This is not a major soucre file.
					CmdlNestLexSource(cmdl_lex, strg_disk, cmdl_task_info.mj_src_rscn_type, srct_file);
				}
				break;

		default:
			{
				if (trace_mask & sctr_cmdl_key_param)
				{
					TraceEvent(&cmdl_lex.value_origin, TPRF_CMDL_IGNORE, L"Key param is ignored in StdProcessKeyParam().");
				}
			}
			break;
	}
}

void TMidLevScanner::StdProcessPosParam(TCmdlLexema &cmdl_lex)
{
	assert(cmdl_lex.type == cmdl_pos_param);

	// First check if current param is the name of the mj_src or not.
	if (cmdl_lex.num_value != cmdl_task_info.mj_src_pos_param)
	{
		// This is some other file name.
		if (trace_mask & sctr_cmdl_pos_param)
		{
			TraceEvent(&cmdl_lex.value_origin, TPRF_CMDL_POS, L"PrmNum=%d, MajorSrcPrm=%d. Skipping.", cmdl_lex.num_value, cmdl_task_info.mj_src_pos_param);
		}
		return;
	}

	// Trace the func entering. This will be a long processing.
	if (trace_mask & sctr_cmdl_pos_param)
	{
		TraceFuncEnter(&cmdl_lex.value_origin, TPRF_CMDL_MJSRC, L"Processing MajorScrPrm (PrmNum=%lu).", cmdl_lex.num_value);
	}

	// Set the flag that param is present even if the param value is bogus.
	major_src_info.mj_src_prm_found = TRUE;

	TTextBuffer80 str_val_buffer;
	if (ConvertToTextBuffer(&cmdl_lex.value_origin, str_val_buffer, cmdl_lex.str_value) == FALSE)
	{
		// Error is already reported.
		if (trace_mask & sctr_cmdl_pos_param)
		{
			TraceFuncExit(&cmdl_lex.value_origin, TPRF_CMDL_MJSRC, L"Failure - out of memory while preparing the pos param file name.");
		}
		return;
	}

	// Load the file from the disk and assign fid to it.
	TFileInfoItem *finfo = CreateFileInfo(&cmdl_lex.value_origin, cmdl_task_info.mj_src_strg_type, cmdl_task_info.mj_src_rscn_type, str_val_buffer);
	if (finfo == NULL)
	{
		// Erorr sould be already reported.
		if (trace_mask & sctr_cmdl_pos_param)
		{
			TraceFuncExit(&cmdl_lex.value_origin, TPRF_CMDL_MJSRC, L"Failure in the CreateFileInfo().");
		}
		return;
	}

	// Save info about this file.
	major_src_info.mj_src_ident.doc_id = finfo->data.doc_id;
	major_src_info.mj_src_info = finfo;

	if (prep_opts.immed_mj_src_load == FALSE)
	{
		// Info for processing the major src is already set up. Send a mid lev event (exmis_mjsrc_area).
		// This call will also set the major_src_scan_evt field in the parsing header.
		PrMiscPreprocStmtEvent info;
		info.Prepare(GetNextCN(), exmis_mjsrc_area, cond_pars_status, cmdl_lex.value_origin, cmdl_lex.value_origin);
		info.mpr_token.Clear();

		info.PrepareFileNameSection(finfo, cmdl_lex.value_origin);
		cbk_handler->MiscPreprocEventNotification(&info);
		PostProsessMidLevCbkCall(info.hdr);

		if (trace_mask & sctr_cmdl_pos_param)
		{
			TraceFuncExit(&cmdl_lex.value_origin, TPRF_CMDL_MJSRC, L"Success: CN=%lu. The info is recorded for later processing.", info.hdr.cn);
		}
	}
	else
	{
		// Immediately enter the major source file. Prepare the source locations.
		PrEventHeader *src_loc[3] = { &cmdl_lex.value_origin, NULL, &cmdl_lex.value_origin };
		bool res = BeginMainPhase(cmdl_task_info.lang_type, finfo, major_src_info.obj_file_info, src_loc, &scan_opts_proto);

		if (trace_mask & sctr_cmdl_pos_param)
		{
			TraceFuncExit(&cmdl_lex.value_origin, TPRF_CMDL_MJSRC, (res == TRUE) ? L"Success: The major source file is entered." : L"Failure in the BeginMainPhase().");
		}
	}
}

void TMidLevScanner::FirstLexSourceEnterActions()
{
	// Refresh the command line handler.
	if (cmdl_cbk_handler != NULL)
		cmdl_cbk_handler->OnStart();

	PrEventHeader ctx;
	GetCurrScanPos(ctx);

	bool smth_entered = LoadAnalysisToolProfile(&ctx);
	if (smth_entered == FALSE)
	{
		// Env profile was not requested or its loading falied. Try to do the second step of actions.
		LoadBuiltInMacrosFile(&ctx);
	}
}

bool TMidLevScanner::LoadAnalysisToolProfile(PrEventHeader *src_ctx)
{
	// At least something should be present in the sources stack.
	assert(src_cnt > 0);

	// Check if the tool profile is needed or not.
	if (scan_opts_proto.want_analysis_env == FALSE)
	{
		if (trace_mask & sctr_major_actions)
			TraceEvent(src_ctx, TPRF_DONT_WANT_CE, L"Analysis Tool Env was not requested.");
		return(FALSE);
	}

	// Profile is requested.
	if (trace_mask & sctr_major_actions)
		TraceFuncEnter(src_ctx, TPRF_LOAD_ANT_ENV);

	// Prepare the name of the profile.
	wchar_t profile_fname[2*MAX_PATH+80];
	TPathHelper::GetExeDirectoryName(profile_fname, 2*MAX_PATH);
	wcscat(profile_fname, L"AnalysisTool.env");

	// Check if this file is existing.
	if (TPathHelper::IsPathAFile(profile_fname) == FALSE)
	{
		// The file is missing.
		AddError(src_ctx, mscn_err_misc_atool_env_miss, 0, profile_fname);
		if (trace_mask & sctr_major_actions)
			TraceFuncExit(src_ctx, TPRF_LOAD_ANT_ENV, L"File is not existing: \"%s\".", profile_fname);
		return(FALSE);
	}

	// Move name of the file to the heap.
	wchar_t *profile_fname_dict = misc_strings_dict.RegisterStr(profile_fname);
	if (profile_fname_dict == NULL)
	{
		AddOutOfMemError(src_ctx, mloom_ld_an_tool_profile1, profile_fname);
		if (trace_mask & sctr_major_actions)
			TraceFuncExit(src_ctx, TPRF_LOAD_ANT_ENV, L"Out of memory while preparing: \"%s\".", profile_fname);
		return(FALSE);
	}

	// Prepare the fake cmdl lexema.
	TStringPtr profile_fname_ptr(profile_fname_dict);
	TCmdlLexema key_lex;
	key_lex.SetKeyParam(cmdk_analysis_tool_key, profile_fname_ptr);
	GetCurrScanPos(key_lex.key_name_origin, latest_cn);
	GetCurrScanPos(key_lex.value_origin, latest_cn);

	// Call target function with the special set of flags.
	block_paths_substing = TRUE;
	bool res = CmdlNestLexSource(key_lex, strg_disk, cmdl_task_info.cmdl_rscn_type, srct_ant_env);
	block_paths_substing = FALSE;

	if (trace_mask & sctr_major_actions)
		TraceFuncExit(src_ctx, TPRF_LOAD_ANT_ENV, (res == TRUE) ? L"The file was loaded and entered." : L"Failure in the CmdlNestLexSource().");

	// Return value shows if lex source nesting succeeded or not.
	return(res);
}

bool TMidLevScanner::LoadBuiltInMacrosFile(PrEventHeader *src_ctx)
{
	// Take value from opts proto. This is the right place during the current phase.
	ID fid = (scan_opts_proto.raw_scanning_mode == TRUE) ? (ID)-1 : scan_opts_proto.built_in_macros_fid;

	// Make the trace.
	if (trace_mask & sctr_major_actions)
	{
		TraceFuncEnter(src_ctx, TPRF_LOAD_BUILTIN, L"FileId=%d%s.", fid, (fid != 0) ? L"" : L" (latest version of built in macros)");
	}

	// Check if built in conditions are requested.
	if (fid == (ID)-1)
	{
		if (trace_mask & sctr_major_actions)
		{
			TraceFuncExit(src_ctx, TPRF_LOAD_BUILTIN, (scan_opts_proto.raw_scanning_mode == TRUE) ?
						L"The raw scanning mode is requested. The built in macros are not applicable." : L"The fid is -1. The built in macros are not requested.");
		}
		return(FALSE);
	}

	// Check that fid belongs to the range of the built in file fids.
	if (fid >= TFileInfo::DiskFileIdsBase)
	{
		wchar_t buff[40];
		AddError(src_ctx, mscn_err_misc_bltin_bad_fid, 0, _itow(fid, buff, 10));
		if (trace_mask & sctr_major_actions)
		{
			TraceFuncExit(src_ctx, TPRF_LOAD_BUILTIN, L"Failure - FID does not belong to the built in files range.");
		}
		return(FALSE);
	}

	// Find info about requested built in file.
	TBuiltInFileInfo *pbltin = TBuiltInFileInfo::GetBuiltInMacroFileInfo(fid);
	if (pbltin == NULL)
	{
		wchar_t buff[40];
		AddError(src_ctx, mscn_err_misc_bltin_unkn_fid, 0, _itow(fid, buff, 10));
		if (trace_mask & sctr_major_actions)
		{
			TraceFuncExit(src_ctx, TPRF_LOAD_BUILTIN, L"Failure - error loading the built in file body.");
		}
		return(FALSE);
	}

	// Create file descriptor. Pass in ID of the file in the date param.
	TFileInfoItem *finfo = CreateFileInfo(src_ctx, strg_builtin, pbltin->rscn, pbltin->fname, pbltin->fbody, -1, pbltin->fid);
	if (finfo == NULL)
	{
		AddOutOfMemError(src_ctx, mloom_ld_builtin_macro_loc1);
		if (trace_mask & sctr_major_actions)
		{
			TraceFuncExit(src_ctx, TPRF_LOAD_BUILTIN, L"Failure - cannot duplucate and register a built in file.");
		}
	}

	// Enter this file descriptor.
	bool res = EnterFileSource(0, finfo, srct_bltin_defs, mdo_builtin, 0, -1, TRUE, FALSE, src_ctx, NULL, NULL);

	// Make the final trace.
	if (trace_mask & sctr_major_actions)
	{
		TraceFuncExit(src_ctx, TPRF_LOAD_BUILTIN, (res == TRUE) ? L"Success: The file was loaded and entered." : L"Failure in the EnterFileSource().");
	}

	return(res);
}

void TMidLevScanner::CmdlProcessAnalysisToolKey(TCmdlLexema &cmdl_lex)
{
	assert(cmdl_lex.type == cmdl_key_param);
	assert(cmdl_lex.num_value == cmdk_analysis_tool_key);

	// Ensure that this key comes from the right file.
	if (src_stack[src_cnt-1].src_type != srct_ant_env)
	{
		AddError(&cmdl_lex.value_origin, mscn_err_cmdl_bad_atkey_src);
		if (trace_mask & sctr_cmdl_ant_key)
		{
			TraceEvent(&cmdl_lex.value_origin, TPRF_CMDL_ANTKEY, L"Analysis tool yey comes from the wrong type of lex source: %s.", TLexemasSource::GetLexSourceTypeEnumName(src_stack[src_cnt-1].src_type));
		}
		return;
	}

	bool unknown_key = FALSE;
	TStringPtr &val = cmdl_lex.str_value;

	if (val == L"immed_mj_src_load")
	{
		prep_opts.immed_mj_src_load = TRUE;
	}
	else if (val == L"raw_scanning_mode")
	{
		scan_opts_proto.raw_scanning_mode = TRUE;
	}
	else if (val == L"disab_define_stmts")
	{
		scan_opts_proto.disab_define_stmts = TRUE;
	}
	else if (val == L"disab_include_stmts")
	{
		scan_opts_proto.disab_include_stmts = TRUE;
	}
	else if (val == L"disab_cond_pars_stmts")
	{
		scan_opts_proto.disab_cond_pars_stmts = TRUE;
	}
	else if (val == L"disab_misc_prepr_stmts")
	{
		scan_opts_proto.disab_misc_prepr_stmts = TRUE;
	}
	else if (val == L"disab_macro_calls")
	{
		scan_opts_proto.disab_macro_calls = TRUE;
	}
	else if (val == L"ignore_error_stmt")
	{
		scan_opts_proto.ignore_error_stmt = TRUE;
	}
	else
	{
		// Unknown value.
		unknown_key = TRUE;
		AddError(&cmdl_lex.value_origin, mscn_err_cmdl_unkn_atool_key);
		if (trace_mask & sctr_cmdl_ant_key)
		{
			TraceEvent(&cmdl_lex.value_origin, TPRF_CMDL_ANTKEY, L"Unknown subkey name: \"%.*s\".", val.GetLength(), val.GetBodyPtr());
		}
	}

	// Trace if name of the key was recognized above.
	if (unknown_key == FALSE && trace_mask & sctr_cmdl_ant_key)
	{
		TraceEvent(&cmdl_lex.value_origin, TPRF_CMDL_ANTKEY, L"Key \"%.*s\" was processed.", val.GetLength(), val.GetBodyPtr());
	}
}

void TMidLevScanner::CmdlProcessDefinitionKey(TCmdlLexema &cmdl_lex)
{
	assert(cmdl_lex.type == cmdl_key_param);

	// Make preliminary tracing.
	if (trace_mask & sctr_cmdl_key_param)
	{
		wchar_t loc_buff[80];
		TraceFuncEnter(&cmdl_lex.value_origin, TPRF_CMDL_MDEF, L"Value=\"%s\".", cmdl_lex.str_value.CopyWithTruncationTo(loc_buff, 80));
	}

	// Instantiate the C/C++ raw scanner to process the value of the key param.
	PrEventHeader &macro_area = cmdl_lex.value_origin;
	long area_beg = macro_area.src_area.area_beg;
	long area_len = macro_area.src_area.area_len;
	wchar_t *area_ptr = macro_area.src_area.file_info->info.file_body+area_beg;
	if (area_len >= 2 && ((*area_ptr == L'\"' && area_ptr[area_len-1] == L'\"') || (*area_ptr == L'\'' && area_ptr[area_len-1] == L'\'')))
	{
		// Value area contains quoted string. Strip the quotes.
		area_beg++;
		area_len -= 2;
	}

	TRawScanner raw_scanner(macro_area.src_area.file_info, &misc_strings_dict);
	raw_scanner.Seek(area_beg, area_len, FALSE, FALSE);

	// Value of the key should start with the name of the macro. Pick it up.
	TLexema name_lex;
	if (CmdlRawScanCmdlArea(raw_scanner, name_lex, macro_area) != ltx_name)
	{
		AddError(&name_lex.origin, mscn_err_cmdl_pcnd_no_name);
		if (trace_mask & sctr_cmdl_key_param)
			TraceFuncExit(&cmdl_lex.value_origin, TPRF_CMDL_MDEF, L"Failure - the macro name is missing.");
		return;
	}

	// Allocate core structure for the macro.
	TMacroDefinition *macro = cbk_handler->CreateMacroDefinition(name_lex.str_value, mds_hard_def, src_stack[src_cnt-1].mdefs_nature);
	if (macro == NULL)
	{
		// Out of memory or app decided not to create the macro object.
		AddError(&cmdl_lex.value_origin, mscn_err_cmdl_oom_for_pcnd);
		if (trace_mask & sctr_cmdl_key_param)
			TraceFuncExit(&cmdl_lex.value_origin, TPRF_CMDL_MDEF, L"Failure - cannot create the TMacroDefinition object.");
		return;
	}

	// Pick up lexema that stays after the name.
	TLexema body_beg_lex;
	CmdlRawScanCmdlArea(raw_scanner, body_beg_lex, macro_area);
	TLexema lex = body_beg_lex;

	// Right after the name there can be either an equal sign or eof.
	if (body_beg_lex.IsKeyword(opr_assign) == TRUE)
	{
		// This predefined condition looks to have non empty body. In any case equal sign is not a part of the body.
		CmdlRawScanCmdlArea(raw_scanner, body_beg_lex, macro_area);
		lex = body_beg_lex;

		int cnt_lex = 0;
		while (lex.type != ltx_eof)
		{
			cnt_lex++;
			if (cnt_lex == 1 && lex.type >= ltx_number && lex.type <= ltx_charconst)
				macro->scst_body_lex = lex;

			// Pick up the next lexema.
			CmdlRawScanCmdlArea(raw_scanner, lex, macro_area);
		}

		if (cnt_lex != 1)
			macro->scst_body_lex.Clear();
	}
	else if (body_beg_lex.type != ltx_eof)
	{
		// Bogus lexema after the macro name.
		AddError(&body_beg_lex.origin, mscn_err_cmdl_pcnd_bad_delim);
		body_beg_lex.origin.AdjustToAreaBeg();
		lex.origin.AdjustToAreaBeg();
	}

	// Fill in the missing parts and call the target function.
	PrEventHeader whole_stmt_area = cmdl_lex.key_name_origin;
	PrEventHeader body_area = body_beg_lex.origin;
	whole_stmt_area.ExtendPrHeader(lex.origin);
	body_area.ExtendPrHeader(lex.origin);
	macro->SetupDefinitionArea(whole_stmt_area);
	macro->SetupBodyArea(body_area);

	// Protected method will do major part of the work.
	bool res = UpdateMacroLibrary(macro, name_lex, cmdl_lex.key_name_origin);
	if (trace_mask & sctr_cmdl_key_param)
	{
		TraceFuncExit(&cmdl_lex.value_origin, TPRF_CMDL_MDEF, (res == TRUE) ? L"Success: CN=%lu." : L"Failure in the UpdateMacroLibrary().", macro->hdr.cn);
	}
}

bool TMidLevScanner::CmdlNestLexSource(TCmdlLexema &cmdl_lex, TFileStorageType strg, TRawScanningType rscn, TLexSourceType srct)
{
	assert(cmdl_lex.type == cmdl_key_param);
	assert(cmdl_lex.str_value.IsEmpty() == FALSE);

	// Make the trace.
	if (trace_mask & sctr_cmdl_key_param)
	{
		wchar_t loc_buff[80];
		TraceFuncEnter(&cmdl_lex.value_origin, TPRF_CMDL_NEST,
					L"StrgType=%s, RawScanningType=%s, Name=\"%s\".",
					TFileInfo::GetStrgTypeEnumName(strg), TFileInfo::GetRscnTypeEnumName(rscn), cmdl_lex.str_value.CopyWithTruncationTo(loc_buff, 80));
	}

	// Check the parsing phase.
	if (parsing_phase != mlph_preparing)
	{
		AddError(&cmdl_lex.value_origin, mscn_err_cmdl_no_prep_phase, 0, g_ParsingPhaseNames[parsing_phase]);
		if (trace_mask & sctr_cmdl_key_param)
			TraceFuncExit(&cmdl_lex.value_origin, TPRF_CMDL_NEST, L"Failure -the parsing phase is wrong.");

		return(FALSE);
	}

	TTextBuffer80 str_val_buffer;
	if (ConvertToTextBuffer(&cmdl_lex.value_origin, str_val_buffer, cmdl_lex.str_value) == FALSE)
	{
		// Error is already reported.
		if (trace_mask & sctr_cmdl_key_param)
			TraceFuncExit(&cmdl_lex.value_origin, TPRF_CMDL_NEST, L"Failure - out of memory while preparing the file name.");

		return(FALSE);
	}

	// Load the file from the disk and assign fid to it.
	TFileInfoItem *finfo = CreateFileInfo(&cmdl_lex.value_origin, strg, rscn, str_val_buffer);
	if (finfo == NULL)
	{
		// Erorr sould be already reported.
		if (trace_mask & sctr_cmdl_key_param)
			TraceFuncExit(&cmdl_lex.value_origin, TPRF_CMDL_NEST, L"Failure in the CreateFileInfo().");

		return(FALSE);
	}

	// Combine the key and value parts into one area.
	PrEventHeader stmt_area = cmdl_lex.key_name_origin;
	stmt_area.ExtendPrHeader(cmdl_lex.value_origin);

	// Call the target function. It will do error reporting if needed.
	bool res = EnterFileSource(0, finfo, srct, mdo_predefined, 0, -1, TRUE, FALSE,
							&stmt_area, (srct != srct_ant_env) ? &cmdl_lex.key_name_origin : NULL,
							(srct != srct_ant_env) ? &cmdl_lex.value_origin : NULL);

	if (trace_mask & sctr_cmdl_key_param)
	{
		TraceFuncExit(&cmdl_lex.value_origin, TPRF_CMDL_NEST, (res == TRUE) ? L"Success." : L"Failure in the EnterFileSource().");
	}

	return(res);
}

TLexemaType TMidLevScanner::CmdlRawScanCmdlArea(TRawScanner &raw_scanner, TLexema &lex, PrEventHeader &org_proto)
{
	// This method is used when command line macro definition is scanned. Method should not
	// be used for any other purposes.
	raw_scanner.GetLex(lex);
	lex.on_doubt = src_on_doubt;
	lex.origin.cn = org_proto.cn;
	lex.origin.source_ref = org_proto.source_ref;
	lex.origin.cond_ref = org_proto.cond_ref;
	lex.origin.PrepareLinearArea(org_proto.lin_area.linear_beg-org_proto.src_area.area_beg);

	if (trace_mask & sctr_cmdl_retr)
	{
		TraceEventLexema(&lex.origin, TPRF_CMDL_CLG_LEX, L"Lexema", lex);
	}

	// Consider this lexema as lexema from the file source.
	summary.file_src_lexemas++;
	return(lex.type);
}

wchar_t * TMidLevScanner::PrepareBaseDirectory(wchar_t *buffer_2_max_path_len, const wchar_t *parsing_base_dir, DWORD &resvd_dir_len, DWORD &resvd_dir_win32_error)
{
	//
	// This is slightly weird piece of code because it is necessary to resolve the base directory before opening
	// the parsing session in the callback. Base directory is one of parameters of the call that opens the session.
	// At the same time it is not possible to report errors when session is not opened. The is why resolution is
	// done here and errors are reported to the caller. This method should be kept in close sync with
	// the SetupFileHandlingFields() method.
	//
	resvd_dir_win32_error = 0;

	if (parsing_base_dir == NULL || parsing_base_dir[0] == 0)
	{
		// Passed base directory is empty. Pick up the current directory of the process.
		resvd_dir_len = ::GetCurrentDirectoryW(2*MAX_PATH, buffer_2_max_path_len);
		if (resvd_dir_len == 0)
		{
			resvd_dir_win32_error = ::GetLastError();
		}
		else if (resvd_dir_len < 2*MAX_PATH)
		{
			// Success.
			resvd_dir_len = -1;
			return(buffer_2_max_path_len);
		}
	}
	else
	{
		// Still copy the string into the internal buffer.
		resvd_dir_len = (DWORD)wcslen(parsing_base_dir);
		if (resvd_dir_len < 2*MAX_PATH)
		{
			wcscpy(buffer_2_max_path_len, parsing_base_dir);
			for (DWORD inx=0; inx<resvd_dir_len; ++inx)
			{
				if (buffer_2_max_path_len[inx] == L'/')
					buffer_2_max_path_len[inx] = L'\\';
			}

			if (buffer_2_max_path_len[resvd_dir_len-1] == L'\\')
				buffer_2_max_path_len[resvd_dir_len-1] = 0;

			// Success.
			resvd_dir_len = -1;
			return(buffer_2_max_path_len);
		}
	}

	// Passed directory name is bogus or the attempt to pick up the current directory failed.
	return((parsing_base_dir != NULL) ? (wchar_t*)parsing_base_dir : L"");
}

bool TMidLevScanner::SetupFileHandlingFields(const wchar_t *parsing_base_dir, wchar_t *base_dir, DWORD resvd_len, DWORD resvd_error,
												TDriveSubstInfo *drive_substs_array, bool allow_interm_paths, bool allow_unsubsted_paths)
{
	if (drive_substs_array != NULL)
	{
		// Drive substs are non empty. This requires to pass the base directory explicitly.
		if (parsing_base_dir == NULL || parsing_base_dir[0] == 0)
		{
			AddError(NULL, mscn_err_subst_no_base_dir);
			return(FALSE);
		}

		if (allow_interm_paths == TRUE)
		{
			AddError(NULL, mscn_err_subst_interm_paths);
			return(FALSE);
		}
	}

	if (resvd_len == 0)
	{
		// Win32 failure happened.
		wchar_t buff40[40];
		AddError(NULL, mscn_err_subst_curr_dir_err1, 0, _itow(resvd_error, buff40, 10));
		return(FALSE);
	}
	else if (resvd_len != -1)
	{
		// For unclear reason the current directory is too long.
		wchar_t buff40[40];
		AddError(NULL, mscn_err_subst_curr_dir_err2, 0, _itow(resvd_len, buff40, 10));
		return(FALSE);
	}

	if (TPathHelper::IsAbsolutePath(base_dir) == FALSE)
	{
		AddError(NULL, mscn_err_subst_base_not_abs, 0, base_dir);
		return(FALSE);
	}

	if (drive_substs_array != NULL)
	{
		// Process array of the subst rules.
		while (drive_substs_array->internal_prefix != NULL && drive_substs_array->external_subst != NULL)
		{
			if (drive_substs_array->internal_prefix[0] == 0 || drive_substs_array->external_subst[0] == 0)
			{
				assert(FALSE);
				break;
			}

			// Allocate the new subst item.
			wchar_t *internal_prefix_dict = misc_strings_dict.RegisterStr(drive_substs_array->internal_prefix);
			wchar_t *external_subst_dict = misc_strings_dict.RegisterStr(drive_substs_array->external_subst);
			TDriveSubstItem *subst_item = NULL;
			if (internal_prefix_dict != NULL && external_subst_dict != NULL)
				subst_item = new TDriveSubstItem(internal_prefix_dict, external_subst_dict);

			if (subst_item == NULL)
			{
				AddOutOfMemError(NULL, mloom_create_dsubst_loc1, drive_substs_array->internal_prefix);
				return(FALSE);
			}

			// Send MidLev callback.
			PrDriveSubstRegistration info;
			info.hdr.Setup(NULL, GetNextCN());
			info.subst_info = *drive_substs_array;
			cbk_handler->RegisterDriveSubstData(&info);
			PostProsessMidLevCbkCall(info.hdr);

			// Add current item to the list and proceed to the next subst.
			subst_item->drive_subst_id = info.hdr.cn;
			drive_substs.AppendItem(subst_item);
			summary.drive_substs_num++;

			// Shift to the next item of array.
			drive_substs_array++;
		}
	}

	// Accept the base directory.
	base_directory.ClearBuffer();
	if (base_directory.Append(base_dir) == FALSE)
	{
		AddOutOfMemError(NULL, mloom_accept_base_dir, base_dir);
		return(FALSE);
	}

	// Accept the rest of the file handling fields.
	allow_intermediate_paths = allow_interm_paths;
	allow_non_substed_paths = allow_unsubsted_paths;
	return(TRUE);
}

bool TMidLevScanner::CreateAbsolutePath(PrEventHeader *ctx, TFileNameBuffer &buffer, const wchar_t *file_name, bool fname_is_file)
{
	assert(file_name != NULL && file_name[0] != 0);
	buffer.ClearBuffer();

	wchar_t fname_buff[2*MAX_PATH];
	if (TPathHelper::IsAbsolutePath(file_name) == FALSE)
	{
		// This path has some relative components.
		if (TPathHelper::IsRelativePath(file_name) == TRUE)
		{
			// Passed path is fully relative. Construct absolute path using the base_directory.
			assert(TPathHelper::IsEmptyPath(base_directory) == FALSE);
			assert(TPathHelper::IsAbsolutePath(base_directory) == TRUE);

			if (TPathHelper::ConstructPath(buffer, base_directory, file_name, fname_is_file) == FALSE)
			{
				if (buffer.GetXpndError() == TRUE)
					AddOutOfMemError(ctx, mloom_create_abs_loc1, file_name);
				else AddError(ctx, mscn_err_incp_append_relat, 0, file_name, base_directory);
				return(FALSE);
			}

			// Success.
			TPathHelper::ConvertToBackSlashes(buffer);
			return(TRUE);
		}
		else
		{
			// This path refers to the current directory on some drive or this is an full directory path on
			// the current drive. Check, if options allow such paths.
			if (allow_intermediate_paths == FALSE)
			{
				int ec = (fname_is_file == TRUE) ? mscn_err_incp_semi_rel_file : mscn_err_incp_semi_rel_dir;
				AddError(ctx, ec, 0, file_name);
				return(FALSE);
			}

			// Resolve the passed name using the Windows features.
			if (::GetFullPathNameW(file_name, 2*MAX_PATH, fname_buff, NULL) == 0)
			{
				wchar_t err_code_buff[40];
				AddError(ctx, mscn_err_incp_interm_path_conv, 0, file_name, _itow(::GetLastError(), err_code_buff, 10));
				return(FALSE);
			}

			// Change the pointer to point into the local buff.
			file_name = fname_buff;
		}
	}

	// It is ok to use the path as it was passed.
	if (buffer.Append(file_name) == FALSE)
	{
		AddOutOfMemError(ctx, mloom_create_abs_loc2, file_name);
		return(FALSE);
	}

	// Success.
	TPathHelper::ConvertToBackSlashes(buffer);
	return(TRUE);
}

bool TMidLevScanner::CreateSubstedPath(PrEventHeader *ctx, TFileNameBuffer &buffer, const wchar_t *file_name)
{
	assert(TPathHelper::IsAbsolutePath(file_name) == TRUE);
	buffer.ClearBuffer();

	if (block_paths_substing == FALSE && drive_substs.IsEmpty() == FALSE)
	{
		// This path should be substed and the list of subts is not empty.
		for (TListIter<TDriveSubstItem> iter(drive_substs); iter; ++iter)
		{
			TDriveSubstItem &item = iter.CurrItem();
			if (wcsncmp(file_name, item.fname_prefix, item.prefix_len) == 0)
			{
				// Matching prefix was found.
				buffer.Append(item.extern_subst, item.subst_len);
				buffer.Append(file_name+item.prefix_len);
				if (buffer.GetXpndError() == TRUE)
				{
					AddOutOfMemError(ctx, mloom_drv_subst_loc1, file_name);
					return(FALSE);
				}

				// Substing succceeded.
				return(TRUE);
			}
		}

		// Prefix match was not found.
		if (allow_non_substed_paths == FALSE)
		{
			AddError(ctx, mscn_err_incp_subst_missing, 0, file_name);
			return(FALSE);
		}
	}

	// It is ok to use the path as it was passed.
	if (buffer.Append(file_name) == FALSE)
	{
		AddOutOfMemError(ctx, mloom_drv_subst_loc2, file_name);
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TMidLevScanner::ConvertToTextBuffer(PrEventHeader *ctx, TTextBuffer80 &buffer, TStringPtr &string_ptr)
{
	buffer.ClearBuffer();
	bool res = buffer.Append(string_ptr.GetBodyPtr(), string_ptr.GetLength());
	if (res == FALSE)
	{
		wchar_t loc_buff[80];
		AddOutOfMemError(ctx, mloom_convert_to_text, string_ptr.CopyWithTruncationTo(loc_buff, 80));

		// Passed buffer still has room for 80 characters. Place truncated string there with dots at the end.
		buffer.Append(loc_buff);
	}

	return res;
}

static const wchar_t *g_TraceHdrMsg1 = L"SeqNm Nest-Cstt:  TraceStmtPrefix: [DocId.LineNum.ChPos]: (Char pos calculation assumes that tab spacing is 1)";
static const wchar_t *g_TraceHdrMsg2 = L"-----------------------------------------------------------------------------------------------------------------------";

static const wchar_t *g_CodeSttsNames[cexr_num_types] =
{
	L"Pass", L"Actv", L"ActD", L"PasD", L"Cond",
};

void TMidLevScanner::TraceColumnTitles()
{
	Console().HandleTrace(g_TraceHdrMsg2);
	Console().HandleTrace(g_TraceHdrMsg1);
	Console().HandleTrace(g_TraceHdrMsg2);
}

void TMidLevScanner::TraceMsgInternal(PrEventHeader *context, const wchar_t *prefix, const wchar_t *action_label, const wchar_t *format, va_list vargs)
{
	// Prepare the action label.
	wchar_t act_lab_buff[80];
	if (action_label != NULL)
		swprintf(act_lab_buff, 80, L"(%s)", action_label);
	else act_lab_buff[0] = 0;

	// Generate first part of the message that is present all the time.
	wchar_t buffer[8196], *pbuff = buffer;
	swprintf(pbuff, 8196, L"%05ld N%03d-%s: %s%s: ", next_trace_evtid++, src_cnt, g_CodeSttsNames[cond_pars_status], prefix, act_lab_buff);
	pbuff += wcslen(pbuff);
	long free_area_len = 8196-(long)(pbuff-buffer);

	if (context != NULL)
	{
		// Append information about the origin of the message.
		TFileInfo *fi = context->src_area.file_info;
		if (fi == NULL)
		{
			// Empty area.
			swprintf(pbuff, free_area_len, L"[fi=NULL.area_offs-%ld.area_len-%ld]: ", context->src_area.area_beg, context->src_area.area_len);
		}
		else
		{
			// Pointer to the file info is present.
			long line_num = -1, char_pos = -1;
			if (GetFileOffsInfo(fi, context->src_area.area_beg, line_num, char_pos) == TRUE)
			{
				// Line number and char pos were retrieved.
				swprintf(pbuff, free_area_len, L"[%s%04ld.ln%05ld.ch%03ld]: ", (fi->pars_id != 0) ? L"Temp-" : L"F", fi->file_id, line_num, char_pos);
			}
			else
			{
				// Most likely this is the file offset failure.
				wchar_t file_id_buff[40];
				swprintf(pbuff, free_area_len, L"[%s.file_offs-%06ld.file_len-%ld]: ", fi->PrepareFileIdShort(file_id_buff, 40), context->src_area.area_beg, context->src_area.file_info->info.file_len);
			}
		}

		pbuff += wcslen(pbuff);
		free_area_len = 8196-(long)(pbuff-buffer);
	}

	// Add formatted app parameters. They will be truncated if they will not fit.
	if (vargs != NULL)
		vswprintf(pbuff, free_area_len, format, vargs);
	else if (format != NULL)
		swprintf(pbuff, free_area_len, L"%s", format);

	// Submit the resulting string to the console.
	Console().HandleTrace(buffer, curr_trace_attr);
}

void TMidLevScanner::TraceLexemaInternal(PrEventHeader *context, const wchar_t *prefix, const wchar_t *action_label, const wchar_t *lexema_label, TLexema &lex)
{
	// At this point decision to trace is already made.
	wchar_t lexema_buff[256], message_buff[512];
	swprintf(message_buff, 512, L"%s: SrcBeg=%ld, %s", lexema_label, lex.origin.src_area.area_beg, lex.SprintfLexema(lexema_buff, 128));

	TraceMsgInternal(context, prefix, action_label, message_buff, NULL);
}

void TMidLevScanner::TraceRawUnscan(PrEventHeader *ctx, const wchar_t *pos_info_label, TRawScanSeekInfo &pos)
{
	TraceEvent(ctx, TPRF_RAW_UNSCAN, L"%s: PosFrom=%ld.", pos_info_label, pos.offs);
}

void TMidLevScanner::TraceRawUnscan(PrEventHeader *ctx, const wchar_t *pos_info_label, TLexema &lex)
{
	TraceLexemaInternal(ctx, TPRF_RAW_UNSCAN, NULL, pos_info_label, lex);
}

void TMidLevScanner::InitConsoleInternal()
{
	// The console should be always present.
	assert(&Console() != NULL);

	// Clear prev state of the data if any. This will also force repainting the console on the next call.
	console_data_stt.parsing_start_time = CurrDateTime();
	console_data_stt.prev_update_time = CurrDateTime();
	console_data_stt.prev_update_rots_cnt = 50;

	// Setup initital state of the data fields.
	Console().SetConsoleLabel(sclb_major_file_name, NULL);
	Console().SetConsoleLabel(sclb_mid_lev_lexemas, L"0");
	Console().SetConsoleLabel(sclb_mid_lev_events, L"0");
	Console().SetConsoleLabel(sclb_num_errors, L"none");
	Console().SetConsoleLabel(sclb_num_warnings, L"none");
	Console().SetConsoleLabel(sclb_elapsed_time, L"00:00:00");
	Console().SetMajorStatus(L"Scanning...");
}

long TMidLevScanner::UpdateConsoleInternal(bool force_update)
{
	wchar_t buff[80];
	TDateTime curr_time = CurrDateTime();

	//
	// Display some of the fields from the summary.
	//

	if (summary.mid_lev_lexemas != console_data_stt.mid_lev_lexemas)
	{
		console_data_stt.mid_lev_lexemas = summary.mid_lev_lexemas;
		ShowConsoleValue(sclb_mid_lev_lexemas, console_data_stt.mid_lev_lexemas);
	}

	if (summary.mid_lev_events != console_data_stt.mid_lev_events)
	{
		console_data_stt.mid_lev_events = summary.mid_lev_events;
		ShowConsoleValue(sclb_mid_lev_events, console_data_stt.mid_lev_events);
	}

	if (summary.num_errors != console_data_stt.num_errors)
	{
		console_data_stt.num_errors = summary.num_errors;
		ShowConsoleValue(sclb_num_errors, console_data_stt.num_errors);
	}

	if (summary.num_warnings != console_data_stt.num_warnings)
	{
		console_data_stt.num_warnings = summary.num_warnings;
		ShowConsoleValue(sclb_num_warnings, console_data_stt.num_warnings);
	}

	// Update the elapsed time.
	TDateTime curr_elapsed = curr_time-console_data_stt.parsing_start_time;
	TDateTime elaps_diff = curr_elapsed-console_data_stt.elapsed_time;
	if ((elaps_diff & ~0x7FFFFF) != 0 || force_update == TRUE)
	{
		// It is high time to update the elapsed time field.
		console_data_stt.elapsed_time = curr_elapsed;
		Console().SetConsoleLabel(sclb_elapsed_time, FormatDuration(curr_elapsed, buff, 80));
	}

	// Prepare number of rotations that should pass before updating the console again.
	TDateTime updates_diff = curr_time-console_data_stt.prev_update_time;
	long new_update_rots = console_data_stt.prev_update_rots_cnt;
	if (updates_diff < 1000000)
		new_update_rots *= 2;
	else if (updates_diff > 5000000)
		new_update_rots /= 2;

	// Check for overflow after the miltiplication.
	if (new_update_rots < 0)
		new_update_rots = 5000000;

	console_data_stt.prev_update_time = curr_time;
	console_data_stt.prev_update_rots_cnt = (new_update_rots > 0) ? new_update_rots : 1;
	return(new_update_rots);
}

void TMidLevScanner::ShowConsoleValue(TScannerConsoleLabelIndex lab_index, ULONG value)
{
	wchar_t buffer[80];
	FormatInt64(value, buffer, 80, fnms_dec_unsigned, L' ');
	Console().SetConsoleLabel(lab_index, buffer);
}


