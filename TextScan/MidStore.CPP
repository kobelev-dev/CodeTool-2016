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

#include  "Common/FormatNumber.H"
#include  "TextScan/MidStore.H"

//----------------------------------------------------------------------
//  ================  CtvLexSourceEnterEvent  ===================
//----------------------------------------------------------------------

TLongId CtvLexSourceEnterEvent::GetEnclosureDocId() const
{
	assert(src_type > srct_none && src_type < srct_num_types);

	if (LEX_SRC_GEN_PBUFF(src_type) == TRUE)
	{
		// Source file is the temp file from the current parsing.
		TIdStruct id = { hdr.parsing_id, paste_buff_fid };
		return(id.doc_id);
	}
	else if (LEX_SRC_WHOLE_FILE(src_type) == TRUE)
	{
		// Source file is the file that was entered with this call stmt.
		return(inc_file_info.doc_id);
	}
	else if (LEX_SRC_MACRO_OR_MPARAM(src_type) == TRUE)
	{
		// Source file is an area where the macro or macro param was defined.
		return(mcall_def_area.doc_id);
	}
	else
	{
		// This should be a self changing macro.
		assert(src_type == srct_scgm);
		return(scgm_scan_area.doc_id);
	}
}

long CtvLexSourceEnterEvent::GetEnclosureSrcStart() const
{
	assert(src_type > srct_none && src_type < srct_num_types);

	if (LEX_SRC_MACRO_OR_MPARAM(src_type) == TRUE)
	{
		// This is a macro source type.
		return(mcall_def_area.area_beg);
	}
	else if (src_type == srct_scgm)
	{
		// This is a self changing macro.
		return(scgm_scan_area.area_beg);
	}
	else
	{
		// This is a file source or a paste buffer. Scanning starts from the beginning.
		return(0);
	}
}

long CtvLexSourceEnterEvent::GetEnclosureSrcEnd() const
{
	assert(src_type > srct_none && src_type < srct_num_types);

	if (LEX_SRC_GEN_PBUFF(src_type) == TRUE)
	{
		// Source file is a temp file from the current parsing.
		return(paste_buff_len);
	}
	else if (LEX_SRC_WHOLE_FILE(src_type) == TRUE)
	{
		// Source file is a file that was entered with this call stmt.
		return(inc_file_info.info.file_len);
	}
	else if (LEX_SRC_MACRO_OR_MPARAM(src_type) == TRUE)
	{
		// Source file is an area where a macro or a macro param was defined.
		return(mcall_def_area.AreaEnd());
	}
	else
	{
		// This should be a self changing macro.
		assert(src_type == srct_scgm);
		return(scgm_scan_area.AreaEnd());
	}
}

//---------------------------------------------------------------------
//  =================  CtvParsingEventInfo  ====================
//---------------------------------------------------------------------

static const wchar_t *g_CtvLexSrcEnterTypeNames[srct_num_types] =
{
	L"LexSrcEnter-0",
	L"Command line invocation",					// srct_cmdl
	L"Analysis tool profile invocation",				// srct_ant_env
	L"LexSrcEnter-3",
	L"Source file invocation",					// srct_file
	L"Built in definitions file invocation",			// srct_bltin_defs
	L"Major source file invocation",				// srct_mj_src
	L"Self changing macro invocation",			// srct_scgm
	L"Macro definition invocation",				// srct_macro
	L"Macro parameter call",					// srct_mparam
	L"Macro def invocation (paste buffer)",			// srct_pbuff_macro
	L"Macro parameter call (paste buffer)",		// srct_pbuff_mparam
};

static const wchar_t *g_CtvLexSrcEnterTypeNamesForListBox[srct_num_types] =
{
	L"LexSrcEnter-0",
	L"CmdLineEnter",							// srct_cmdl
	L"AntEnvEnter",							// srct_ant_env
	L"LexSrcEnter-3",
	L"#include",								// srct_file
	L"BuiltInDefsEnter",						// srct_bltin_defs
	L"MajorSrcFileCall",						// srct_mj_src
	L"SelfChgMacroCall",						// srct_scgm
	L"MacroCall",								// srct_macro
	L"MacroParamCall",						// srct_mparam
	L"PasteBuffEnter (macro)",					// srct_pbuff_macro
	L"PasteBuffEnter (mprm)",					// srct_pbuff_mparam
};

static const wchar_t *g_CtvLexSrcExitTypeNames[srct_num_types] =
{
	L"LexSrcExit-0",
	L"Command line enclosure exit",				// srct_cmdl
	L"Analysis tool profile enclosure exit",			// srct_ant_env
	L"LexSrcExit-3",
	L"Source file enclosure exit",					// srct_file
	L"Built in definitions file exit",					// srct_bltin_defs
	L"Major source file exit",					// srct_mj_src
	L"Self changing macro exit",					// srct_scgm
	L"Macro definition exit",						// srct_macro
	L"Macro parameter call exit",					// srct_mparam
	L"Macro definition exit (paste buffer)",			// srct_pbuff_macro
	L"Macro parameter call exit (paste buffer)",		// srct_pbuff_mparam
};

static const wchar_t *g_CtvLexSrcExitTypeNamesForListBox[srct_num_types] =
{
	L"LexSrcExit-0",
	L"LexSrcExit (Cmdl)",						// srct_cmdl
	L"LexSrcExit (AntEnv)",						// srct_ant_env
	L"LexSrcExit-3",
	L"LexSrcExit (File)",						// srct_file
	L"LexSrcExit (BltDef)",						// srct_bltin_defs
	L"LexSrcExit (MjSrc)",						// srct_mj_src
	L"LexSrcExit (SlfChg)",						// srct_scgm
	L"LexSrcExit (Macro)",						// srct_macro
	L"LexSrcExit (mParam)",					// srct_mparam
	L"PasteBuffExit (Macro)",					// srct_pbuff_macro
	L"PasteBuffExit (mPrm)",					// srct_pbuff_mparam
};

static const wchar_t *g_CtvExprResNames[cexr_num_types] =
{
	L"False", L"True", L"FalseDoubt", L"TrueDoubt", L"Undefined",
};

static const wchar_t *g_CtvCodeSttNames[cexr_num_types] =
{
	L"Passive", L"Active", L"PassiveDoubt", L"ActiveDoubt", L"Conditional",
};

static wchar_t *g_CtvGrammarDefnSectionNames[grsc_num_types] =
{
	L"<none>",		L"<description>",	   L"<revision>",		L"<langtype>",
	L"<symbols>",	L"<ignore>",		   L"<errors>",		L"<axioma>",
	L"<rules>",		L"<conflicts>",	   L"<locations>",		L"<resolution>",
};

long CtvParsingEventInfo::GetSize()
{
	switch (hdr.event_type)
	{
		case pset_register_drive_subst:		return(sizeof(CtvDriveSubstRegEvent));
		case pset_register_temp_file:		return(sizeof(CtvTempFileRegEvent));

		case pset_paste_buffering_change:	return(sizeof(CtvPasteBufferingChgEvent));
		case pset_lex_source_enter:		return(sizeof(CtvLexSourceEnterEvent));
		case pset_lex_source_exit:			return(sizeof(CtvLexSourceExitEvent));
		case pset_macro_library_change:	return(sizeof(CtvMacroLibraryChangeEvent));
		case pset_cond_parsing_change:		return(sizeof(CtvCondParsingChangeEvent));
		case pset_misc_preproc_stmt:		return(sizeof(CtvMiscPreprocStmtEvent));
		case pset_new_include_path:		return(sizeof(CtvNewIncPathEvent));
		case pset_error_warning_event:		return(sizeof(CtvErrorWarningEvent));
		case pset_cond_param_use:		return(sizeof(CtvCondParamUseEvent));

		case pset_grammar_section:		return(sizeof(CtvGrammarDefnSection));
		case pset_grammar_object:			return(sizeof(CtvGrammarDefnObject));
		case pset_cpp_item_ref:			return(sizeof(CtvCppItemRefEvent));
	}

	assert(FALSE);
	return(sizeof(CtvParsingEventInfo));
}

const wchar_t *CtvParsingEventInfo::GetEventTypeName()
{
	// Pick up general description that is based only on the type of the event.
	const wchar_t *stmt_name = hdr.GetEventTypeName();

	// Replace generic message with more detailed msg for some of the event types.
	if (hdr.event_type == pset_paste_buffering_change)
	{
		if (pbuff_chg.evt_enter_cn == 0)
			return(L"Paste buffering enter");
		else return(L"Paste buffering exit");
	}
	else if (hdr.event_type == pset_lex_source_enter)
	{
		// Special procedure for entering the LexSource.
		if (src_enter.src_type < 0 || src_enter.src_type >= srct_num_types)
			return(L"Bogus src_enter.src_type value");

		stmt_name = g_CtvLexSrcEnterTypeNames[src_enter.src_type];
	}
	else if (hdr.event_type == pset_lex_source_exit)
	{
		// Special procedure for exiting the LexSource.
		if (src_exit.src_type < 0 || src_exit.src_type >= srct_num_types)
			return(L"Bogus src_exit.src_type value");

		stmt_name = g_CtvLexSrcExitTypeNames[src_exit.src_type];
	}
	else if (hdr.event_type == pset_misc_preproc_stmt)
	{
		if (misc_stmt.mis_event_type == exmis_failed_include)
			stmt_name = L"File call statement";
		else if (misc_stmt.mis_event_type == exmis_app_name)
			stmt_name = L"The name of the application";
	}
	else if (hdr.event_type == pset_error_warning_event)
	{
		stmt_name = (err_warn.err_class == lerrc_message) ? L"Parsing message" :
			((LEX_ERROR_CLASS_ERROR(err_warn.err_class) == TRUE) ? L"Error message" : L"Warning message");
	}

	return(stmt_name);
}

const wchar_t *CtvParsingEventInfo::GetEventTypeNameForListBox()
{
	//
	// This method should be in sync with the method CompareEventTypeNamesForListBox().
	// All event types that get names based not only on the event type itself, should be properly
	// reflected in the sorting function.
	//

	// Pick up the name of the event type.
	const wchar_t *stmt_name = hdr.GetEventTypeNameForListBox();

	// Show more details for some of the events.
	if (hdr.event_type == pset_none)
	{
		// This is some code location.
		stmt_name = L"CodePlace";
	}
	else if (hdr.event_type == pset_paste_buffering_change)
	{
		if (pbuff_chg.evt_enter_cn == 0)
			stmt_name = L"PasteBufferingBeg";
		else stmt_name = L"PasteBufferingEnd";
	}
	else if (hdr.event_type == pset_lex_source_enter)
	{
		// Special procedure for entering the LexSource.
		assert(src_enter.src_type >= 0 && src_enter.src_type < srct_num_types);
		stmt_name = g_CtvLexSrcEnterTypeNamesForListBox[src_enter.src_type];
	}
	else if (hdr.event_type == pset_lex_source_exit)
	{
		// Special procedure for exiting the LexSource.
		assert(src_exit.src_type >= 0 && src_exit.src_type < srct_num_types);
		stmt_name = g_CtvLexSrcExitTypeNamesForListBox[src_exit.src_type];
	}
	else if (hdr.event_type == pset_macro_library_change)
	{
		if (GET_DEFINED_STT(mdef_chg.mdef_status) == TRUE)
			stmt_name = L"#define";
		else stmt_name = L"#undef";
	}
	else if (hdr.event_type == pset_cond_parsing_change)
	{
		TPreprocToken stmt = cond_pars.stmt_type;
		if (stmt == mpr_ifdef)				stmt_name = L"#ifdef";
		else if (stmt == mpr_ifndef)			stmt_name = L"#ifndef";
		else if (stmt == mpr_if)			stmt_name = L"#if";
		else if (stmt == mpr_elif)			stmt_name = L"#elif";
		else if (stmt == mpr_else)			stmt_name = L"#else";
		else if (stmt == mpr_endif)			stmt_name = L"#endif";
	}
	else if (hdr.event_type == pset_misc_preproc_stmt)
	{
		// All misc events, that are not mentioned below will have the event name "MiscEvent".
		TMiscEventEx stmt = misc_stmt.mis_event_type;
		if (stmt == exmis_blocked_stmt)			stmt_name = L"BlockedStmt";
		else if (stmt == exmis_failed_include)	stmt_name = L"FileCallStmt";
		else if (stmt == exmis_pragma)			stmt_name = L"#pragma";
		else if (stmt == exmis_error)			stmt_name = L"#error";
		else if (stmt == exmis_import)			stmt_name = L"#import";
		else if (stmt == exmis_line)				stmt_name = L"#line";
		else if (stmt == exmis_using)			stmt_name = L"#using";
	}
	else if (hdr.event_type == pset_error_warning_event)
	{
		stmt_name = (err_warn.err_class == lerrc_message) ? L"-Message-" :
			((LEX_ERROR_CLASS_ERROR(err_warn.err_class) == TRUE) ? L"=Error=" : L"-Warning-");
	}
	else if (hdr.event_type == pset_cpp_item_ref)
	{
		// Show the type of related cpp item.
		stmt_name = TCppItemHelper::GetItemTypeDisplayName(cpp_item.cpp_item_type);
	}

	return(stmt_name);
}

const wchar_t *CtvParsingEventInfo::GetEventDescriptionForListBox(wchar_t *buffer, long buff_len)
{
	assert(buff_len >= 120);
	buffer[0] = 0;

	wchar_t fid_buff[40];
	switch (hdr.event_type)
	{
		case pset_none:
				{
					return(L"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
				}

		case pset_register_drive_subst:
				{
					// Give out the external mapping.
					return(drv_subst.subst_info.external_subst);
				}

		case pset_register_temp_file:
				{
					// Give out the name of the file.
					return(temp_file.temp_file_info.info.file_name);
				}

		case pset_paste_buffering_change:
				{
					swprintf(buffer, buff_len, L"NewBuffLevel=%d, BufferFid=%s.",
							pbuff_chg.new_buff_level, TFileInfo::PrepareFileId(hdr.parsing_id, pbuff_chg.buffer_fid, fid_buff, 40));
				}
				break;

		case pset_lex_source_enter:
				{
					// Desription should differ for different types of the lex sources.
					switch (src_enter.src_type)
					{
						// Group of fields #1. All values for the inc file group of fields.
						case srct_cmdl:
						case srct_ant_env:
						case srct_file:
						case srct_bltin_defs:
						case srct_mj_src:
								{
									// Give out the name of the file.
									return(src_enter.inc_file_info.info.file_name);
								}

						// Group of fields #2. Enum members order differs from the union fields order.
						case srct_scgm:
								{
									swprintf(buffer, buff_len, L"File: %s.", src_enter.scgm_scan_area.PrepareFileId(fid_buff, 40));
								}
								break;

						// Group of fields #3.
						case srct_macro:
								{
									// Give out the name of the macro.
									return(src_enter.mcall_def_name);
								}

						case srct_mparam:
								{
									// Show the param number.
									swprintf(buffer, buff_len, L"param #%d.", src_enter.mcall_param_inx+1);
								}
								break;

						// Group of fields #4.
						case srct_pbuff_macro:
						case srct_pbuff_mparam:
								{
									// Paste buff enters have just a few fields to display.
									swprintf(buffer, buff_len, L"BufferFid=%s%s.",
											TFileInfo::PrepareFileId(hdr.parsing_id, src_enter.paste_buff_fid, fid_buff, 40),
											(src_enter.append_tail == TRUE) ? L", AppendTail=TRUE" : L"");
								}
								break;

						default:
							// Bogus type of lexemas source.
							return(L"Bogus type of lexema source.");
							break;
					}
				}
				break;

		case pset_lex_source_exit:
				{
					swprintf(buffer, buff_len, L"EvtEnterCn=%lu.", src_exit.source_ref);
				}
				break;

		case pset_macro_library_change:
				{
					// Give out the name of the macro.
					return(mdef_chg.mdef_name);
				}

		case pset_cond_parsing_change:
				{
					swprintf(buffer, buff_len, L"ExprResult=%s, NewCodeStt=%s, NewCodeLevel=%d.",
							g_CtvExprResNames[cond_pars.expr_result],  g_CtvCodeSttNames[cond_pars.new_code_stt], cond_pars.new_code_level);
				}
				break;

		case pset_misc_preproc_stmt:
				{
					// These are several different events that share the same event type.
					if (misc_stmt.mis_event_type == exmis_app_name)
					{
						// First word in the command line.
						swprintf(buffer, buff_len, L"ApplicationName: %s.", misc_stmt.evt_str_data);
					}
					else if (misc_stmt.mis_event_type == exmis_mjsrc_area)
					{
						// This is an area that defines the delayed major source file call.
						return(misc_stmt.file_name);
					}
					else if (misc_stmt.mis_event_type == exmis_blocked_stmt)
					{
						return(L"Blocked whole line macroprocessor statement.");
					}
					else if (misc_stmt.mis_event_type == exmis_failed_include)
					{
						// This is an #include stmt that has not entered any file.
						THiddenIncudeStmtDtls dtls = (THiddenIncudeStmtDtls)misc_stmt.evt_details;
						swprintf(buffer, buff_len, L"NonCallReason: %s.", PrMiscPreprocStmtEvent::GetNonCallReasonName(dtls));
					}
					else if (misc_stmt.mis_event_type == exmis_pragma || misc_stmt.mis_event_type == exmis_error)
					{
						// Give out the explanation text.
						return(misc_stmt.evt_str_data);
					}
					else if (misc_stmt.mis_event_type == exmis_import)
					{
						// Give out the name of the file if any.
						return(misc_stmt.file_name);
					}
					else if (misc_stmt.mis_event_type == exmis_line)
					{
						// Give out the line number and the name of the file if any.
						if ((misc_stmt.file_name == NULL))
							swprintf(buffer, buff_len, L"Line %d.", misc_stmt.line_num);
						else swprintf(buffer, buff_len, L"Line %d of \"%s\".", misc_stmt.line_num, misc_stmt.file_name);
					}
					else if (misc_stmt.mis_event_type == exmis_using)
					{
						// Give out the name of the file if any.
						return(misc_stmt.file_name);
					}
				}
				break;

		case pset_new_include_path:
				{
					// Give out the name of the directory.
					return(inc_path.inc_directory);
				}

		case pset_error_warning_event:
				{
					// Show the error code and the error message.
					swprintf(buffer, buff_len, L"%d: %s", err_warn.err_code, err_warn.err_msg);
				}
				break;

		case pset_cond_param_use:
				{
					// Give out the name of the parameter.
					return(cond_param.cond_param_name);
				}

		case pset_grammar_section:
				{
					if (grm_sect.section_type >= grsc_none && grm_sect.section_type < grsc_num_types)
						return(g_CtvGrammarDefnSectionNames[grm_sect.section_type]);

					swprintf(buffer, buff_len, L"BogusSectionType=%lu.", grm_sect.section_type);
				}
				break;

		case pset_grammar_object:
				{
					swprintf(buffer, buff_len, L"Symbol=%hd, AppId=%hd.", grm_object.object_sym, grm_object.object_app_id);
				}
				break;

		case pset_cpp_item_ref:
				{
					if (cpp_item.project_level_cpp_item_id == 0)
						swprintf(buffer, buff_len, L"CppItemId=%lu.", cpp_item.pars_sess_cpp_item_id);
					else swprintf(buffer, buff_len, L"ParsSessCppItemId=%lu, ProjectLevelCppItemId=%lu.",
							cpp_item.pars_sess_cpp_item_id, cpp_item.project_level_cpp_item_id);
				}
				break;

		default:
			return(L"Bogus event type.");
	}

	// Some description was placed into the buffer. Return value should be NULL to indicate
	// that generated desription is not a static const string.
	return(NULL);
}

// Public comparison for sorting.
int CtvParsingEventInfo::ComparePidEvidForListBox(const CtvParsingEventInfo *p1, const CtvParsingEventInfo *p2)
{
	// Compare parsing id and event id.
	if (p1->hdr.parsing_id > p2->hdr.parsing_id)
		return(1);
	if (p1->hdr.parsing_id < p2->hdr.parsing_id)
		return(-1);
	if (p1->hdr.event_id > p2->hdr.event_id)
		return(1);
	if (p1->hdr.event_id < p2->hdr.event_id)
		return(-1);

	// This situation should happen only if one of the items is the CodeArea item. Nevertheless compare
	// the event types. Use reverse order here to show the CodeArea item after its "prototype" event.
	if (p1->hdr.event_type > p2->hdr.event_type)
		return(-1);
	if (p1->hdr.event_type < p2->hdr.event_type)
		return(1);

	// Everything is identical. Even this situation is possible. This happens during searching for foreign event
	// after sorting events in the listbox table.
	return(0);
}

// Public comparison for sorting.
int CtvParsingEventInfo::CompareTypeDescrForListBox(const CtvParsingEventInfo *p1, const CtvParsingEventInfo *p2)
{
	int res1 = p1->CompareEventTypeNamesForListBoxInternal(*p2);
	if (res1 != 0)
		return(res1);
	int res2 = p1->CompareEventDescriptionsForListBoxInternal(*p2);
	if (res2 != 0)
		return(res2);

	// Event name and description are both equal. Fall back to the parsingId and eventId.
	return(ComparePidEvidForListBox(p1, p2));
}

// Private method.
int CtvParsingEventInfo::CompareEventTypeNamesForListBoxInternal(const CtvParsingEventInfo &other_inst) const
{
	// Order events by their event types. (Primary field).
	if (hdr.event_type > other_inst.hdr.event_type)
		return(1);
	if (hdr.event_type < other_inst.hdr.event_type)
		return(-1);

	// Both events have the same event type. Pick up the value of the secondary field. Only those events,
	// that base their names not only on the event_type, should be processed here. Macro def/undef is an
	// exception.
	int val_this_inst = 0, val_other_inst = 0;
	switch (hdr.event_type)
	{
		case pset_paste_buffering_change:
				{
					// Put buffering enter events in front of buffering exit events.
					val_this_inst = (pbuff_chg.evt_enter_cn != 0) ? TRUE : FALSE;
					val_other_inst = (other_inst.pbuff_chg.evt_enter_cn != 0) ? TRUE : FALSE;
				}
				break;

		case pset_lex_source_enter:
				{
					// Sort events by the type of the source.
					val_this_inst = src_enter.src_type;
					val_other_inst = other_inst.src_enter.src_type;
				}
				break;

		case pset_lex_source_exit:
				{
					// Sort events by the type of the source.
					val_this_inst = src_exit.src_type;
					val_other_inst =  other_inst.src_exit.src_type;
				}
				break;

		case pset_cond_parsing_change:
				{
					// Sort events by statement token value.
					val_this_inst = cond_pars.stmt_type;
					val_other_inst = other_inst.cond_pars.stmt_type;
				}
				break;

		case pset_misc_preproc_stmt:
				{
					// Sort events by statement token value.
					val_this_inst = misc_stmt.mis_event_type;
					val_other_inst = other_inst.misc_stmt.mis_event_type;
				}
				break;

		case pset_error_warning_event:
				{
					// Make the following order: messages, errors, warnings.
					if (err_warn.err_class != lerrc_message)
						val_this_inst = (LEX_ERROR_CLASS_ERROR(err_warn.err_class) == TRUE) ? 2 : 1;
					if (other_inst.err_warn.err_class != lerrc_message)
						val_other_inst = (LEX_ERROR_CLASS_ERROR(other_inst.err_warn.err_class) == TRUE) ? 2 : 1;
				}
				break;

		case pset_cpp_item_ref:
				{
					// Sort events by the type of their cpp items.
					val_this_inst = cpp_item.cpp_item_type;
					val_other_inst = other_inst.cpp_item.cpp_item_type;
				}
				break;
	}

	// Compare values of the secondary field.
	if (val_this_inst > val_other_inst)
		return(1);
	if (val_this_inst < val_other_inst)
		return(-1);

	// Event types are equal.
	return(0);
}

// Private method.
int CtvParsingEventInfo::CompareEventDescriptionsForListBoxInternal(const CtvParsingEventInfo &other_inst) const
{
	// Pick up the fields that will participate in comparison.
	DWORD val1, val2;
	const wchar_t *str1 = GetDescriptionMeasureForListBoxInternal(val1);
	const wchar_t *str2 = other_inst.GetDescriptionMeasureForListBoxInternal(val2);

	// Evaluate results of the measurement.
	if (str1 != NULL && str2 != NULL)
	{
		int res = wcscmp(str1, str2);
		if (res != 0)
			return(res);
	}
	else
	{
		if (str1 != NULL)
			return(1);
		if (str2 != NULL)
			return(-1);
	}

	if (val1 < val2)
		return(1);
	if (val1 > val2)
		return(-1);

	// Event descriptions are equal.
	return(0);
}

// Private method.
const wchar_t *CtvParsingEventInfo::GetDescriptionMeasureForListBoxInternal(DWORD &num_val) const
{
	num_val = 0;
	wchar_t *str_val = NULL;

	switch (hdr.event_type)
	{
		case pset_register_drive_subst:
				{
					// Sort by the external mapping.
					str_val = drv_subst.subst_info.external_subst;
				}
				break;

		case pset_register_temp_file:
				{
					// Sort by the temp file name.
					str_val = temp_file.temp_file_info.info.file_name;
				}
				break;

		case pset_paste_buffering_change:
				{
					// Sort by the FID of the buffer.
					num_val = pbuff_chg.buffer_fid;
				}
				break;

		case pset_lex_source_enter:
				{
					// Sort order depends on the type of the lex source.
					switch (src_enter.src_type)
					{
						case srct_cmdl:
						case srct_ant_env:
						case srct_file:
						case srct_bltin_defs:
						case srct_mj_src:
								// Sort by the file name.
								str_val = src_enter.inc_file_info.info.file_name;
								break;

						case srct_scgm:
								// Sort by file id, this will be actually type of the macro.
								num_val = src_enter.scgm_scan_area.file_id;
								break;

						case srct_macro:
								// Sort by the name of the macro.
								str_val = src_enter.mcall_def_name;
								break;

						case srct_mparam:
								// Sort by params order.
								num_val = src_enter.mcall_param_inx;
								break;

						case srct_pbuff_macro:
						case srct_pbuff_mparam:
								// Paste buffering enters are sorted by FID.
								num_val = src_enter.paste_buff_fid;
								break;

						default:
							assert(FALSE);
							break;
					}
				}
				break;

		case pset_lex_source_exit:
				{
					// Sort by src enter event id.
					num_val = src_exit.source_ref;
				}
				break;

		case pset_macro_library_change:
				{
					// Sort by the name of the macro.
					str_val = mdef_chg.mdef_name;
				}
				break;

		case pset_cond_parsing_change:
				{
					// Sort by the expression result.
					num_val = cond_pars.expr_result;
				}
				break;

		case pset_misc_preproc_stmt:
				{
					// Different stmt types use different sorting criteria.
					switch (misc_stmt.mis_event_type)
					{
						case exmis_app_name:
								str_val = misc_stmt.evt_str_data;
								break;

						case exmis_mjsrc_area:
								str_val = misc_stmt.file_name;
								break;

						case exmis_blocked_stmt:
						case exmis_failed_include:
								num_val = misc_stmt.evt_details;
								break;

						case exmis_pragma:
						case exmis_error:
								str_val = misc_stmt.evt_str_data;
								break;

						case exmis_line:
								num_val = misc_stmt.line_num;
								str_val = misc_stmt.file_name;
								break;

						case exmis_import:
						case exmis_using:
								str_val = misc_stmt.file_name;
								break;

						default:
							assert(FALSE);
							break;
					}
				}
				break;

		case pset_new_include_path:
				{
					// Sort by include directory.
					str_val = inc_path.inc_directory;
				}
				break;

		case pset_error_warning_event:
				{
					// Sort by error code value.
					num_val = err_warn.err_code;
				}
				break;

		case pset_cond_param_use:
				{
					// Sort by parameter name.
					str_val = cond_param.cond_param_name;
				}
				break;

		case pset_grammar_section:
				{
					// Sort by section type.
					num_val = grm_sect.section_type;
				}
				break;

		case pset_grammar_object:
				{
					// Sort by symbol value.
					num_val = grm_object.object_sym;
				}
				break;

		case pset_cpp_item_ref:
				{
					// Sort by the Id of the cpp item in the project.
					num_val = cpp_item.project_level_cpp_item_id;
				}
				break;
	}

	return(str_val);
}

//----------------------------------------------------------------------------
//  ==================  TSourceFilesTableItem  =======================
//----------------------------------------------------------------------------

TSourceFilesTableItem::TSourceFilesTableItem(ID pars_id, ID file_id)
{
	// Ctor for temp lookup objects.
	data.pars_id = pars_id;
	data.file_id = file_id;
	data.info.file_name = NULL;
	data.info.file_body = NULL;
	data.lines_info = NULL;
}

TSourceFilesTableItem::TSourceFilesTableItem(TFileInfo &info)
{
	// Fill in the database info about the file.
	data = info;
	data.lines_info = NULL;
	original_file_len  = info.info.file_len;
	original_file_date = info.info.file_date;
	original_file_crc  = info.info.file_crc;
}

bool TSourceFilesTableItem::CheckDateLengthCrc()
{
	// Dates check is not exact because its msec part may vary especially when the SQL database is used.
	if (data.info.file_len == original_file_len && ___abs(data.info.file_date-original_file_date) < 100000 && data.info.file_crc == original_file_crc)
	{
		// All fields are the identical.
		return(TRUE);
	}

	// This is a different revision of the file.
	return(FALSE);
}

//-----------------------------------------------------------------------------------
//  =====================  TMidLevStoreReader  ==========================
//-----------------------------------------------------------------------------------

TMidLevStoreReader::TMidLevStoreReader(bool allow_other_revs, bool recheck_cached_revs)
{
	// Init object as a primary store.
	m_primary_store = NULL;

	// Clear the iteration buffers.
	m_ci.Clear();
	m_fi.Clear();
	m_pfi = NULL;
	m_ki.Clear();

	m_event.Clear();
	m_kf_ref.Clear();
	m_kp_ref.Clear();

	// Derived class should set up a non NULL value into this field.
	m_pStringsDict = NULL;

	m_allow_other_revisions = allow_other_revs;
	m_recheck_cached_files  = recheck_cached_revs;

	// There is no console in the beginning.
	m_reader_console = NULL;
}

TMidLevStoreReader::~TMidLevStoreReader()
{
	if (m_primary_store != NULL)
	{
		// Notify the parent store about destruction of its child.
		m_primary_store->RemoveFromRegistrationList(this, FALSE);
	}
	else
	{
		// Release all owned database readers if any.
		for (int inx=0; inx<m_allocated_secondary_stores.NumItems(); ++inx)
		{
			m_allocated_secondary_stores[inx]->m_primary_store = NULL;
			delete m_allocated_secondary_stores[inx];
		}
	}
}

bool TMidLevStoreReader::GetLexSrcEnterEventAllowNullEventId(ID parsing_id, ID event_id)
{
	if (parsing_id != 0 && event_id == 0)
	{
		// This is a special case when a non existing source ref is retrieved.
		PrepareEmptyLexSrcEnterEvent(parsing_id);
		return(TRUE);
	}

	return(GetLexSourceEnterEvent(parsing_id, event_id));
}

bool TMidLevStoreReader::GetCondParsChgEventAllowNullEventId(ID parsing_id, ID event_id)
{
	if (parsing_id != 0 && event_id == 0)
	{
		// This is a special case when a non existing cond ref is retrieved.
		PrepareEmptyCondParsChangeEvent(parsing_id);
		return(TRUE);
	}

	return(GetCondParsingChangeEvent(parsing_id, event_id));
}

bool TMidLevStoreReader::GetSourceRefEvent(ID pars_id, ID event_id)
{
	// It is necessary to retrieve the event data because its linear offset is needed.
	if (GetParsingEvent(pars_id, event_id) == FALSE)
		return(FALSE);

	return(GetSourceRefEvent(EventData()));
}

bool TMidLevStoreReader::GetSourceRefEvent(CtvParsingEventInfo &event)
{
	// Check for events that have source ref event id as one of their data fields.
	ID pars_id = event.hdr.parsing_id;
	switch (event.hdr.event_type)
	{
		case pset_lex_source_enter:
				{
					if (event.src_enter.source_ref == 0)
						return(FALSE);
					return(GetLexSourceEnterEvent(pars_id, event.src_enter.source_ref));
				}

		case pset_lex_source_exit:
				{
					if (event.src_exit.source_ref == 0)
						return(FALSE);
					return(GetLexSourceEnterEvent(pars_id, event.src_exit.source_ref));
				}

		case pset_cond_parsing_change:
				{
					if (event.cond_pars.source_ref == 0)
						return(FALSE);
					return(GetLexSourceEnterEvent(pars_id, event.cond_pars.source_ref));
				}

		default:
			break;
	}

	// Parent event should be picked up using the linear offset.
	return(GetSourceRefEvent(pars_id, event.hdr.area.linear_beg));
}

bool TMidLevStoreReader::GetCondRefEvent(ID pars_id, ID event_id)
{
	// It is necessary to retrieve the event data because its linear offset is needed.
	if (GetParsingEvent(pars_id, event_id) == FALSE)
		return(FALSE);

	return(GetCondRefEvent(EventData()));
}

bool TMidLevStoreReader::GetCondRefEvent(CtvParsingEventInfo &event)
{
	// Check for events that have cond ref event id as one of their data fields.
	ID pars_id = event.hdr.parsing_id;
	switch (event.hdr.event_type)
	{
		case pset_lex_source_enter:
				{
					if (event.src_enter.cond_ref == 0)
					{
						PrepareEmptyCondParsChangeEvent(pars_id);
						return(TRUE);
					}

					return(GetCondParsingChangeEvent(pars_id, event.src_enter.cond_ref));
				}

		case pset_lex_source_exit:
				{
					if (event.src_exit.cond_ref == 0)
					{
						PrepareEmptyCondParsChangeEvent(pars_id);
						return(TRUE);
					}

					return(GetCondParsingChangeEvent(pars_id, event.src_exit.cond_ref));
				}

		case pset_cond_parsing_change:
				{
					if (event.cond_pars.cond_ref == 0)
					{
						PrepareEmptyCondParsChangeEvent(pars_id);
						return(TRUE);
					}

					return(GetCondParsingChangeEvent(pars_id, event.cond_pars.cond_ref));
				}

		default:
			break;
	}

	// Parent event should be picked up using the linear offset.
	return(GetCondRefEvent(pars_id, event.hdr.area.linear_beg));
}

TMidLevStoreReader *TMidLevStoreReader::CreateSecondaryStore()
{
	if (m_primary_store != NULL)
	{
		// Curent store is not the primary store. Forward this request to the primary store.
		return(m_primary_store->CreateSecondaryStore());
	}

	// Pick up an object from the cache or allocate a new database reader.
	TMidLevStoreReader *qp = m_secondary_stores_look_aside.GetItemNoAlloc();
	if (qp == NULL)
	{
		// The look aside list is empty.
		qp = AllocateNewSecondaryStore();
		if (qp == NULL)
		{
			// This is an allocation failure.
			return(NULL);
		}
	}

	// Setup the child object.
	qp->m_primary_store = this;
	qp->SetupErrorCallback(m_reader_console);

	// Record info about the newly created child.
	if (m_allocated_secondary_stores.AppendItem(qp) == FALSE)
	{
		qp->m_primary_store = NULL;
		delete qp;
		return(NULL);
	}

	// Success.
	return(qp);
}

bool TMidLevStoreReader::GetSourceLocationInfo(TSourceLocation &buffer, ID parsing_id, ID event_id, TMidLevScanner *active_sess_scanner)
{
	// Pick up the event data using the passed Ids.
	if (GetParsingEvent(parsing_id, event_id) == FALSE)
		return(FALSE);

	// Call the worker function.
	return(GetSourceLocationInfo(buffer, parsing_id, EventData().hdr.area, active_sess_scanner));
}

bool TMidLevStoreReader::GetSourceLocationInfo(TSourceLocation &buffer, ID parsing_id, TLinearArea &lin_area, TMidLevScanner *active_sess_scanner)
{
	// Check if passed event has relation to the source code or not.
	if (lin_area.linear_beg <= 0 || lin_area.linear_len < 0)
	{
		wchar_t buffer[240];
		swprintf(buffer, 240, L"Linear area (Beg=%ld, Len=%ld) cannot be mapped to the source code.", lin_area.linear_beg, lin_area.linear_len);
		ShowError(buffer);
		return(FALSE);
	}

	// Figure out the props of the parsing session that is being queried.
	if (GetCompilationInfo(parsing_id) == FALSE)
		return(FALSE);

	// This parsing session is known.
	CtvCompilationInfo &shdr = CompilationData();
	assert(shdr.lang_type != lang_none);

	long max_linear_offs;
	if (shdr.parsing_status == -1)
	{
		// Requested parsing session is not finished.
		if (active_sess_scanner == NULL)
		{
			wchar_t buffer[240];
			swprintf(buffer, 240, L"The mid lev scanner is missing while the parsing session with ID %ld is still opened.\r\n"
							L"It is not possible to retrieve the source location info.", parsing_id);
			ShowDataContentsError(buffer);
			return(FALSE);
		}

		// The scanner is available. Check the session that it is currently running.
		ID scanner_session_id = active_sess_scanner->CbkHandler().GetParsingId();
		if (scanner_session_id == 0)
		{
			wchar_t buffer[240];
			swprintf(buffer, 240, L"The parsing session with ID %ld is opened while the passed mid lev scanner does not contain any\r\n"
							L"active parsing session. It is not possible to retrieve the source location info.", parsing_id);
			ShowDataContentsError(buffer);
			return(FALSE);
		}
		else if (scanner_session_id != parsing_id)
		{
			wchar_t buffer[240];
			swprintf(buffer, 240, L"The parsing session with ID %ld is opened while the passed mid lev scanner is running parsing\r\n"
							L"session with ID %ld.  It is not possible to retrieve the source location info.", parsing_id, scanner_session_id);
			ShowDataContentsError(buffer);
			return(FALSE);
		}

		// The scanner is available and it is running the right session.
		max_linear_offs = active_sess_scanner->GetCurrentLinearOffset();
	}
	else
	{
		// Max linear offset can be picked up from the parsing session header.
		max_linear_offs = shdr.summary.final_linear_offs;
	}

	assert(max_linear_offs >= 0);
	if (lin_area.AreaEnd() > max_linear_offs)
	{
		// Passed linear area sticks outside of the available linear space.
		wchar_t buffer[240];
		swprintf(buffer, 240, L"The passed linear area (Beg=%ld, Len=%ld) sticks outside of the linear space of the parsing\r\n"
						L"session. The parsing session ID is %ld, its max linear offset is %ld.", lin_area.linear_beg, lin_area.linear_len, parsing_id, max_linear_offs);
		ShowDataContentsError(buffer);
		return(FALSE);
	}

	// Pass real processing to the protected virtual method.
	return(GetSourceLocationInfoInternal(buffer, parsing_id, lin_area, max_linear_offs));
}

bool TMidLevStoreReader::GetSourceAreaBodyInfo(TSourceAreaBodyInfo &body_info, TSourceAreaInfo &area, bool *file_version_mismatch_flag, bool silent_mode)
{
	// Pick up the file body.
	if (GetFileInformation(area.pars_id, area.file_id, TRUE, file_version_mismatch_flag, silent_mode) == FALSE)
	{
		// Error is already displayed.
		return(FALSE);
	}

	if (area.AreaBeg() < 0 || area.AreaEnd() > FileData().info.file_len)
	{
		if (silent_mode == FALSE)
		{
			wchar_t buffer[240];
			swprintf(buffer, 240, L"Source area (Beg=%ld, Len=%ld) sticks outside of the file body (FileLen=%ld).", area.area_beg, area.area_len, FileData().info.file_len);
			ShowError(buffer);
		}

		return(FALSE);
	}

	// The file is known and it body is available. Find an appropriate line in this file.
	TSimpleLineInfo *lib = FileData().GetLineInfoFromFileOffset(area.AreaBeg());
	TSimpleLineInfo *lie = FileData().GetLineInfoFromFileOffset(area.AreaEnd());
	if (lib == NULL || lie == NULL)
	{
		if (silent_mode == FALSE)
			ShowError(L"Error picking up the lines info for the source area.");

		return(FALSE);
	}

	// All parts of the data are fine. Fill in the area info.
	body_info.area.Init(FileDataPtr(), area);

	// Fill in the area beg info.
	body_info.first_line_num = FileData().GetLineNumber(lib);
	body_info.first_line_charpos = area.area_beg-lib->line_offs+1;
	body_info.fline_body_ptr = body_info.area.AreaBegPtr();
	body_info.fline_body_len = __min(area.area_len, lib->LineEnd()-area.area_beg);

	// Fill in the area end info.
	body_info.last_line_num  = FileData().GetLineNumber(lie);

	// Success.
	return(TRUE);
}

long TMidLevStoreReader::GetSourceAreaLineBeg(TSourceAreaInfo area)
{
	// Pick up the file body.
	if (GetFileInformation(area.pars_id, area.file_id, TRUE) == FALSE)
	{
		// Error is already displayed.
		return(-1);
	}

	long line_num = FileData().GetLineNumber(area.area_beg);
	if (line_num < 0)
	{
		wchar_t buffer[240];
		swprintf(buffer, 240, L"Error getting line number for the source area in the file \"%s\", file offset: %ld.", FileData().info.file_name, area.area_beg);
		ShowError(buffer);
		return(-1);
	}

	return(line_num);
}

wchar_t *TMidLevStoreReader::RegisterString(const wchar_t *stmt_name, short param_inx, const wchar_t *str_data)
{
	assert(m_pStringsDict != NULL);
	if (str_data == NULL || str_data[0] == 0)
		return(NULL);

	// Registration function will check if this string is already known or not.
	wchar_t *cached_str = m_pStringsDict->RegisterStr(str_data);
	if (cached_str != NULL)
		return(cached_str);

	// Caching the string failed. Display an error.
	wchar_t buffer[240];
	swprintf(buffer, 240, L"Error caching string for stmt [%s], param %hd.\n\n"
			L"String contents: \"%.40s%s\"\n\n"
			L"Do you want to continue?",
			stmt_name, param_inx, str_data, (wcslen(str_data) > 40) ? L"..." : L"");

	// If user will answer NO, it is necessary to set abort flag on the lower level.
	if (ShowError(buffer, MB_YESNO) == IDNO)
		SetUserCancel(TRUE);

	// Failure.
	return(NULL);
}

bool TMidLevStoreReader::CheckParsingSessionConsistency(ID parsing_id)
{
	if (parsing_id == 0)
	{
		// Check the latest parsing session.
		if (GetLatestCompilationInfo() == FALSE)
			return(FALSE);

		parsing_id = CompilationData().parsing_id;
	}

	// Make simple checks on the header of the parsing session.
	if (GetCompilationInfo(parsing_id) == FALSE)
	{
		wchar_t buffer[240];
		swprintf(buffer, 240, L"DatabaseConsistencyCheck: Unable to pick up the parsing session header for the pasring session %ld.", parsing_id);
		ShowDataContentsError(buffer);
		return(FALSE);
	}
	else if (CompilationData().lang_type == lang_none)
	{
		wchar_t buffer[240];
		swprintf(buffer, 240, L"DatabaseConsistencyCheck: The parsing session %ld is a raw indexing session.", parsing_id);
		ShowDataContentsError(buffer);
		return(FALSE);
	}

	// Iterate all available parsing events, whatever the parsing session contains.
	if (StartParsingEventsIteration(parsing_id, PSET_ALL_TYPES) == FALSE)
	{
		wchar_t buffer[240];
		swprintf(buffer, 240, L"DatabaseConsistencyCheck: Unable to start events iteration for the parsing session %ld.", parsing_id);
		ShowDataContentsError(buffer);
		return(FALSE);
	}

	bool overall_success = TRUE;
	int cnt_events_in_iteration = 0;

	ID prev_evt_id = 0;
	long prev_evt_lin_beg = 0;
	CtvEventType prev_evt_type;

	while (NextParsingEvent() == TRUE && CheckUserCancel() == FALSE)
	{
		CtvEventHeader &evt_hdr = EventData().hdr;
		cnt_events_in_iteration++;

		if (evt_hdr.event_id != prev_evt_id+1)
		{
			wchar_t buffer[240];
			swprintf(buffer, 240, L"DatabaseConsistencyCheck: Event %lu (%s) was picked after the event %lu (%s) in the parsing session %lu.",
					evt_hdr.event_id, evt_hdr.GetEventTypeName(), prev_evt_id, CtvEventHeader::GetEventTypeName(prev_evt_type), parsing_id);
			ShowDataContentsError(buffer);
			overall_success = FALSE;
		}

		/* -BUGBUG-
		if (check_event_linear_beg_steady_increase == TRUE && evt_hdr.area.linear_beg < prev_evt_lin_beg)
		{
			wchar_t buffer[380];
			swprintf(buffer, 380, L"DatabaseConsistencyCheck:\r\n\r\nLinear area begs are not steadily inclreasing in the parsing session %lu."
							L"\r\n\r\n    Event: \t%lu  (%s)\r\n    LinearBeg: \t%ld"
							L"\r\n\r\n    Event: \t%lu  (%s)\r\n    LinearBeg: \t%ld", parsing_id,
							prev_evt_id, CtvEventHeader::GetEventTypeName(prev_evt_type), prev_evt_lin_beg,
							evt_hdr.event_id, evt_hdr.GetEventTypeName(), evt_hdr.area.linear_beg);
			ShowDataContentsError(buffer);
			overall_success = FALSE;
		}
		*/

		prev_evt_id = evt_hdr.event_id;
		prev_evt_lin_beg = evt_hdr.area.linear_beg;
		prev_evt_type = evt_hdr.event_type;
	}

	CloseParsingEventsIteration();

	int cnt_events_in_sess_hdr = CompilationData().summary.mid_lev_events;
	if (cnt_events_in_iteration != cnt_events_in_sess_hdr && CheckUserCancel() == FALSE)
	{
		wchar_t buffer[240];
		swprintf(buffer, 240, L"DatabaseConsistencyCheck: Events iteration returned %d event%s while the session header tells it should be %d event%s.",
							cnt_events_in_iteration, PluralSuffix(cnt_events_in_iteration), cnt_events_in_sess_hdr, PluralSuffix(cnt_events_in_sess_hdr));
		ShowDataContentsError(buffer);
		overall_success = FALSE;
	}

	// Return the accumulated state.
	return(overall_success);
}

void TMidLevStoreReader::PrepareEmptyLexSrcEnterEvent(ID parsing_id)
{
	m_event.Clear();
	m_event.hdr.parsing_id = parsing_id;
	m_event.hdr.event_type = pset_lex_source_enter;
}

void TMidLevStoreReader::PrepareEmptyCondParsChangeEvent(ID parsing_id)
{
	m_event.Clear();
	m_event.hdr.parsing_id = parsing_id;
	m_event.hdr.event_type = pset_cond_parsing_change;
	m_event.cond_pars.stmt_code_stt = cexr_hard_true;
	m_event.cond_pars.new_code_stt	= cexr_hard_true;
}

TFileInfo *TMidLevStoreReader::LookupCachedFile(ID pars_id, ID file_id, bool *file_version_mismatch, bool &cached_item_present)
{
	if (file_version_mismatch != NULL)
		*file_version_mismatch = FALSE;

	// Check the cache, maybe the file is already there.
	TSourceFilesTableItem lookup_item(pars_id, file_id);
	TSourceFilesTableItem *pCachedFileInfo = m_files_cache.FindRecord(&lookup_item);
	if (pCachedFileInfo == NULL)
	{
		cached_item_present = FALSE;
		return(NULL);
	}

	// Something is present in the cache.
	cached_item_present = TRUE;

	// Reverify the revision of the file.
	if (pCachedFileInfo->CheckDateLengthCrc() == FALSE)
	{
		// Cached revision differs from what is required by the database.
		if (file_version_mismatch != NULL)
			*file_version_mismatch = TRUE;

		if (m_recheck_cached_files == TRUE)
		{
			if (ShowFileRevisionQuestion(pCachedFileInfo, TRUE) != IDYES)
			{
				// Question was having buttons "YES/NO". User does not want to use this file.
				return(NULL);
			}
		}
	}

	// Return a pointer to the cached item.
	return(&(pCachedFileInfo->data));
}

TFileInfo *TMidLevStoreReader::CacheDiskFile(TFileInfo &info, bool *file_version_mismatch)
{
	// Ensure that this is a disk file.
	assert(info.pars_id == 0);

	if (file_version_mismatch != NULL)
		*file_version_mismatch = FALSE;

	// Allocate a hash table item for this file.
	TSourceFilesTableItem *psfti = new TSourceFilesTableItem(info);
	if (psfti == NULL)
	{
		ShowFileLoadingError(info, ldres_out_of_memory);
		return(NULL);
	}

	// Make loop to allow several attempts to load the file.
	for(;;)
	{
		if (info.file_id >= TFileInfo::DiskFileIdsBase)
		{
			// Load the file from the disk. This will also pick up the length and date of the disk file.
			TLoadSaveResult load_res = TFileBodyHelper::LoadAsciiFileBody(psfti->data.info, info.info.file_name);
			if (load_res != ldres_success)
			{
				ShowFileLoadingError(info, load_res);
				delete psfti;
				return(NULL);
			}
		}
		else
		{
			// This is id of the well known file.
			TBuiltInFileInfo *pbltin = TBuiltInFileInfo::GetBuiltInMacroFileInfo(info.file_id);
			if (pbltin == NULL)
			{
				wchar_t file_id_buff[40], message_buff[80+2*MAX_PATH+120];
				swprintf(message_buff, sizeof(message_buff)/sizeof(wchar_t),
						L"Error loading BUILT IN file: %s\r\n\r\n"
						L"FileId: \t\t%s\r\nReason: \tfile is missing.             ",
						info.info.file_name, info.PrepareFileId(file_id_buff, 40));
				ShowError(message_buff);
				delete psfti;
				return(NULL);
			}

			// Enusure file props and duplicate the file data.
			assert(wcscmp(info.info.file_name, pbltin->fname) == 0);
			long flen = (long)wcslen(pbltin->fbody);
			assert(flen == info.info.file_len);

			psfti->data.info.file_body = (wchar_t*)malloc((flen+1)*sizeof(wchar_t));
			if (psfti->data.info.file_body == NULL)
			{
				wchar_t message_buff[80+2*MAX_PATH];
				swprintf(message_buff, sizeof(message_buff)/sizeof(wchar_t),
						L"Error duplicating body of the BUILT IN file.\r\n\r\n"
						L"File: \t%s\r\n  ", info.info.file_name);
				ShowError(message_buff);
				delete psfti;
				return(NULL);
			}

			// Copy file data.
			wcscpy(psfti->data.info.file_body, pbltin->fbody);
		}

		// Body of the file was successfully loaded. Check the revision of the file.
		if (psfti->CheckDateLengthCrc() == FALSE)
		{
			// Loaded file differs from what the database wants. Qestion should be displayed even
			// if settings do not allow other revisions. In this case message box will be retry/cancel.
			switch (ShowFileRevisionQuestion(psfti, FALSE))
			{
				case IDNO:
				case IDRETRY:
						{
							// Try to load the file once again.
							psfti->data.ReleaseBasicFileInfo();
							continue;
						}

				case IDYES:
						{
							// Accept the file with a different date/len/crc.
							if (file_version_mismatch != NULL)
								*file_version_mismatch = TRUE;
						}
						break;

				default:
					{
						// Loading attempt failed completely.
						delete psfti;
						return(NULL);
					}
			}
		}

		// Break the loop.
		break;
	}

	// Final action. This action will build the lines info in the heap.
	return(AddFileItemToCache(psfti));
}

TFileInfo *TMidLevStoreReader::CacheTempFile(TFileInfo &info)
{
	// Ensure that this is a temp file.
	assert(info.pars_id != 0);
	assert(info.info.file_name != NULL);
	assert(info.info.file_body != NULL);

	// Allocate storage for hash item.
	TSourceFilesTableItem *psfti = new TSourceFilesTableItem(info);
	if (psfti == NULL)
	{
		ShowFileLoadingError(info, ldres_out_of_memory);
		return(NULL);
	}

	// Allocate storage for name and body of the file.
	psfti->data.info.file_name = (wchar_t*)malloc((wcslen(info.info.file_name)+1)*sizeof(wchar_t));
	psfti->data.info.file_body = (wchar_t*)malloc((info.info.file_len+1)*sizeof(wchar_t));
	if (psfti->data.info.file_name == NULL || psfti->data.info.file_body == NULL)
	{
		// It is ok not to check what actually failed. Proper fields will be released when TSourceFilesTableItem will be cleaned up.
		ShowFileLoadingError(info, ldres_out_of_memory);
		delete psfti;
		return(NULL);
	}

	// Copy the file name and the file body.
	wcscpy(psfti->data.info.file_name, info.info.file_name);
	wcscpy(psfti->data.info.file_body, info.info.file_body);

	// Final action.
	return(AddFileItemToCache(psfti));
}

TFileInfo *TMidLevStoreReader::AddFileItemToCache(TSourceFilesTableItem *psfti)
{
	// Build line descriptors for this file.
	if (psfti->data.BuildLinesInfo() == FALSE)
	{
		ShowFileLoadingError(psfti->data, ldres_out_of_memory);
		delete psfti;
		return(NULL);
	}

	// Add this file descriptor to the cache.
	TSourceFilesTableItem *added_psfti = m_files_cache.AddRecord(psfti);
	assert(added_psfti == psfti);

	// Success.
	return(&(psfti->data));
}

int TMidLevStoreReader::ShowFileRevisionQuestion(TSourceFilesTableItem *pfi, bool item_from_cache)
{
	// Prepare the message because error callback does not accept format strings.
	wchar_t file_id_buff[40], database_file_date[80], disk_file_date[80];
	wchar_t message_buff[80+2*MAX_PATH+120+240];
	swprintf(message_buff, sizeof(message_buff)/sizeof(wchar_t),
			L"   Version of the file on disk differs from the version of the file in the database. \r\n\r\n"
			L"   File name: \t\t%s\r\n   File ident:\t\t%s\r\n\r\n"
			L"   Database file date: \t%s\r\n   Database file length: \t%ld\r\n   Database file crc: \t\t%08X\r\n\r\n"
			L"   Disk file date: \t\t%s\r\n   Disk file length: \t%ld\r\n   Disk file crc: \t\t%08X\r\n\r\n"
			L"\t   Do you want to use this revision instead?\r\n",
			pfi->data.info.file_name, pfi->data.PrepareFileId(file_id_buff, 40),
			FormatDateTime(pfi->original_file_date, database_file_date, 80),
			pfi->original_file_len, pfi->original_file_crc,
			FormatDateTime(pfi->data.info.file_date, disk_file_date, 80),
			pfi->data.info.file_len, pfi->data.info.file_crc);

	if (item_from_cache == FALSE && m_allow_other_revisions == TRUE)
		wcscat(message_buff, L"\r\n(Press NO to retry loading the file)\r\n");

	// Display the message.
	int res = ShowError(message_buff, (item_from_cache == TRUE) ? MB_YESNO : (m_allow_other_revisions ? MB_YESNOCANCEL : MB_RETRYCANCEL));
	return(res);
}

void TMidLevStoreReader::ShowFileLoadingError(TFileInfo &info, TLoadSaveResult load_res)
{
	// Error message was not displayed yet.
	wchar_t message_buff[80+2*MAX_PATH+120],  file_id_buff[40];
	swprintf(message_buff, sizeof(message_buff)/sizeof(wchar_t),
			L"Error loading file: %s\r\n\r\n"
			L"FileId: \t\t%s\r\nReason: \t%s.             ",
			info.info.file_name, info.PrepareFileId(file_id_buff, 40),
			TFileBodyHelper::GetLoadSaveResultText(load_res));
	ShowError(message_buff);
}

void TMidLevStoreReader::ShowDataContentsError(const wchar_t *message)
{
	if (CheckUserCancel() == TRUE)
		return;

	wchar_t buffer[380+MAX_PATH];

	swprintf(buffer, 380+MAX_PATH, L"%s\r\n\r\nDo you want to continue?\r\n", message);
	if (ShowError(buffer, MB_YESNO) == IDNO)
		SetUserCancel(TRUE);
}

void TMidLevStoreReader::RemoveFromRegistrationList(TMidLevStoreReader *qp, bool add_to_look_aside_list)
{
	if (m_primary_store != NULL)
	{
		// Pass this request to the parent store.
		m_primary_store->RemoveFromRegistrationList(qp, add_to_look_aside_list);
	}
	else
	{
		// Remove object from the registration list.
		for (int inx=0; inx<m_allocated_secondary_stores.NumItems(); ++inx)
		{
			if (m_allocated_secondary_stores[inx] == qp)
			{
				m_allocated_secondary_stores.ReleaseItem(inx);
				break;
			}
		}

		if (add_to_look_aside_list == TRUE)
		{
			// Add object to the look aside list or release it.
			qp->m_primary_store = NULL;
			m_secondary_stores_look_aside.RecycleItem(qp);
		}
	}
}


