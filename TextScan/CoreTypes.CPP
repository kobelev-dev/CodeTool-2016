//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <string.h>
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "TextScan/CoreTypes.H"

//---------------------------------------------------------------------------
//  ======================  TFileInfo  ===========================
//---------------------------------------------------------------------------

static const wchar_t *g_FileInfoStrgTypeEnumNames[strg_num_types] =
{
	L"strg_disk",
	L"strg_temp",
	L"strg_interm",
	L"strg_builtin",
	L"strg_proto",
};

static const wchar_t *g_FileInfoRscnTypeEnumNames[rscn_num_types] =
{
	L"rscn_none",
	L"rscn_binary",
	L"rscn_c_cpp",
	L"rscn_csh",
	L"rscn_cmdl_ms_cl",
	L"rscn_cmdl_ms_csh",
	L"rscn_cmdl_ms_mc",
};

bool TFileInfo::CloneFileNameAndBodyAndLinesInfo(TFileInfo &file_info)
{
	assert(file_info.IsNull() == FALSE);

	// Copy in the data.
	*this = file_info;
	info.file_name = NULL;
	info.file_body = NULL;
	lines_info = NULL;
	lines_info_len = 0;

	// Allocate space for the name of the file.
	info.file_name = (wchar_t*)malloc((wcslen(file_info.info.file_name)+1)*sizeof(wchar_t));
	if (info.file_name == NULL)
		return(FALSE);

	if (file_info.info.file_body != NULL)
	{
		// Allocate space for the body of the file.
		info.file_body = (wchar_t*)malloc((file_info.info.file_len+1)*sizeof(wchar_t));
		if (info.file_body == NULL)
		{
			free(info.file_name);
			info.file_name = NULL;
			return(FALSE);
		}

		// Copy the data.
		memcpy(info.file_body, file_info.info.file_body, (file_info.info.file_len+1)*sizeof(wchar_t));
	}

	if (BuildLinesInfo() == FALSE)
	{
		free(info.file_name);
		info.file_name = NULL;
		free(info.file_body);
		info.file_body = NULL;
	}

	// Success.
	wcscpy(info.file_name, file_info.info.file_name);
	return(TRUE);
}

long TFileInfo::SkipEndOfLineForward(long offs)
{
	// Skip the end of the line if passed position stays at the end of the line.
	TSimpleLineInfo *li = GetLineInfoFromFileOffset(offs);
	assert(li != NULL);

	if (offs == li->LineEnd() && (li-lines_info) < lines_info_len-1)
	{
		// Shift the file offset to the beg of the next line.
		offs = (li+1)->LineBeg();
	}

	return(offs);
}

long TFileInfo::SkipEndOfLineBackward(long offs)
{
	// Skip the eol backward if passed position stayes at the beg of the line.
	TSimpleLineInfo *li = GetLineInfoFromFileOffset(offs);
	assert(li != NULL);

	if (offs == li->line_offs && li > lines_info)
	{
		// Shift the file offset to the end of the previous line.
		offs = (li-1)->LineEnd();
	}

	return(offs);
}

long TFileInfo::AdjustToLineBeg(long offs) const
{
	TSimpleLineInfo *li = GetLineInfoFromFileOffset(offs);
	assert(li != NULL);
	return(li->LineBeg());
}

long TFileInfo::AdjustToLineEnd(long offs) const
{
	TSimpleLineInfo *li = GetLineInfoFromFileOffset(offs);
	assert(li != NULL);
	return(li->LineEnd());
}

long TFileInfo::AdjustToPrevLineEnd(long offs, long &curr_line_beg)
{
	// Shift passed position to the end of the prev line if possible.
	TSimpleLineInfo *li = GetLineInfoFromFileOffset(offs);
	assert(li != NULL);

	curr_line_beg = li->line_offs;
	if (li > lines_info)
	{
		// This is not the first line in the file. Shift the file offset to the end of the previous line.
		offs = (li-1)->LineEnd();
	}
	else
	{
		offs = curr_line_beg;
	}

	return(offs);
}

long TFileInfo::AdjustToCurrLineEnd(long offs, long &next_line_beg)
{
	// Shift to the end of the current line.
	TSimpleLineInfo *li = GetLineInfoFromFileOffset(offs);
	assert(li != NULL);

	long line_end = li->LineEnd();
	if ((li-lines_info) < lines_info_len-1)
	{
		// This is not the last line in the file. Shift the file offest to the beg of the next line.
		next_line_beg = (li+1)->line_offs;
	}
	else
	{
		next_line_beg = line_end;
	}

	return(line_end);
}
const wchar_t *TFileInfo::PrepareFileId(ID parsId, ID fid, wchar_t *buffer, long buffLen)
{
	assert(buffLen >= 40);

	if (parsId == 0)
	{
		swprintf(buffer, buffLen, L"%lu", fid);
	}
	else
	{
		swprintf(buffer, buffLen, L"%lu.%lu (temp)", parsId, fid);
	}

	return(buffer);
}

const wchar_t *TFileInfo::PrepareFileIdShort(ID parsId, ID fid, wchar_t *buffer, long buffLen)
{
	assert(buffLen >= 40);

	if (parsId == 0)
	{
		swprintf(buffer, buffLen, L"F%lu", fid);
	}
	else
	{
		swprintf(buffer, buffLen, L"T%lu.%lu", parsId, fid);
	}

	return(buffer);
}

const wchar_t *TFileInfo::GetStrgTypeEnumName(TFileStorageType val)
{
	if (val >= 0 && val < strg_num_types)
		return(g_FileInfoStrgTypeEnumNames[val]);

	return(L"BogusStrgType");
}

const wchar_t *TFileInfo::GetRscnTypeEnumName(TRawScanningType val)
{
	if (val >= 0 && val < rscn_num_types)
		return(g_FileInfoRscnTypeEnumNames[val]);

	return(L"BogusRscnType");
}

//------------------------------------------------------------------------
//  ====================  TSourceArea  ========================
//------------------------------------------------------------------------

void TSourceArea::Init(TFileInfo *fi, TSourceAreaInfo &src)
{
	assert(fi->doc_id == src.doc_id);

	file_info = fi;
	area_beg = src.area_beg;
	area_len = src.area_len;
}

//------------------------------------------------------------------------
//  ===================  TCppItemHelper  =======================
//------------------------------------------------------------------------

static const wchar_t *g_CppItemDisplayNames[cxx_num_types] =
{
	L"EmptyCppItem",				// cxx_none

	L"BogusVal=1.",
	L"BogusVal=2.",
	L"BogusVal=3.",
	L"BogusVal=4.",
	L"BogusVal=5.",
	L"BogusVal=6.",
	L"BogusVal=7.",
	L"BogusVal=8.",
	L"BogusVal=9.",
	L"BogusVal=10.",
	L"BogusVal=11.",
	L"BogusVal=12.",
	L"BogusVal=13.",
	L"BogusVal=14.",

	L"CondParsParam",			// cxx_cond_pars_param

	L"BuiltInType",				// cxx_built_in_type
	L"BitFieldType",				// cxx_bit_field_type
	L"CvPtrRefType",				// cxx_cv_ptr_ref_type
	L"ArrayType",					// cxx_array_type
	L"DataTypeAlias",				// cxx_data_type_alias

	L"Enum",						// cxx_enum_type
	L"EnumMember",				// cxx_enum_member
	L"StructType",					// cxx_struct_type
	L"BaseClass",					// cxx_base_class_spec
	L"FieldAlignSpec",				// cxx_field_align_spec
	L"FriendSpec",					// cxx_friend_spec
	L"DataField",					// cxx_data_field
	L"Initializer",					// cxx_data_field_initializer

	L"FunctionType",				// cxx_function_type
	L"FuncTypeAlias",				// cxx_function_type_alias
	L"CodeEntryPoint",				// cxx_code_entry_point
	L"ExceptionSpec",				// cxx_possible_exception_spec

	L"Namespace",				// cxx_namespace
	L"GlobalNamespace",			// cxx_global_namespace
	L"NamespaceAlias",			// cxx_namespace_alias
	L"UsingDeclaration",			// cxx_using_declaration
	L"UsingDirective",				// cxx_using_directive
	L"Asm-Insert",				// cxx_asm_insert
	L"LinkageSpec",				// cxx_linkage_spec

	L"BogusVal=40.",
	L"BogusVal=41.",

	L"ClassTemplate",				// cxx_class_template
	L"ClassTemplateSpec",			// cxx_class_template_spec
	L"ClassTemplateInst",			// cxx_class_template_inst
	L"FunctionTemplate",			// cxx_func_template
	L"FuncTemplateSpec",			// cxx_func_template_spec
	L"FuncTemplateInst",			// cxx_func_template_inst

	L"TypeParam",				// cxx_template_type_param
	L"TemplateParam",				// cxx_template_template_param
	L"TypeArgument",				// cxx_template_type_argument
	L"TemplateArgument",			// cxx_template_template_argument

	L"UnresolvedNameInfo",		// cxx_unresolved_name_info
	L"UnresolvedDataType",			// cxx_unresolved_data_type
	L"UnresolvedStructType",		// cxx_unresolved_struct_type
	L"UnresolvedDataField",			// cxx_unresolved_data_field
	L"UnresolvedCodeEntryPoint",	// cxx_unresolved_code_entry_point
	L"UnresolvedClassTemplate",	// cxx_unresolved_class_template
	L"UnresolvedFuncTemplate",		// cxx_unresolved_func_template

	L"BogusVal=59.",
	L"BogusVal=60.",

	L"Empty-Stmt",				// cxx_empty_stmt
	L"If-Stmt",					// cxx_if_stmt
	L"Switch-Stmt",				// cxx_switch_stmt
	L"For-Stmt",					// cxx_for_stmt
	L"While-Stmt",				// cxx_while_stmt
	L"Do-While-Stmt",				// cxx_do_while_stmt

	L"Goto-Stmt",					// cxx_goto_stmt
	L"Break-Stmt",				// cxx_break_stmt
	L"Continue-Stmt",				// cxx_continue_stmt
	L"Return-Stmt",				// cxx_return_stmt

	L"Block",						// cxx_block
	L"TryBlock",					// cxx_try_block
	L"CatchBlock",				// cxx_catch_block
	L"LightBlock",					// cxx_light_block
	L"Label",						// cxx_label
	L"CaseLabel",					// cxx_case_label

	L"Operand-Expr",				// cxx_operand_expr
	L"Unary-Expr",				// cxx_unary_expr
	L"Binary-Expr",				// cxx_binary_expr
	L"Multi-Op-Expr",				// cxx_multiop_expr
	L"Assignment-Expr",			// cxx_assignment_expr

	L"Throw-Expr",				// cxx_throw_expr
	L"Conditional-Expr",			// cxx_conditional_expr
	L"Field-Access-Expr",			// cxx_field_access_expr
	L"Field-Accs-Ptr-Expr",			// cxx_field_access_ptr_expr
	L"Array-Index-Expr",			// cxx_array_index_expr
	L"Function-Call-Expr",			// cxx_function_call_expr

	L"Cast-Expr",					// cxx_cast_expr
	L"Sizeof-Expr",				// cxx_sizeof_expr
	L"New-Expr",					// cxx_new_expr
	L"Delete-Expr",				// cxx_delete_expr
	L"Typeid-Expr",				// cxx_typeid_expr
};

// Enum member names.
static const wchar_t *g_CppItemEnumNames[cxx_num_types] =
{
	L"cxx_none",							// 0

	L"cxx_BogusValue_1",
	L"cxx_BogusValue_2",
	L"cxx_BogusValue_3",
	L"cxx_BogusValue_4",
	L"cxx_BogusValue_5",
	L"cxx_BogusValue_6",
	L"cxx_BogusValue_7",
	L"cxx_BogusValue_8",
	L"cxx_BogusValue_9",
	L"cxx_BogusValue_10",
	L"cxx_BogusValue_11",
	L"cxx_BogusValue_12",
	L"cxx_BogusValue_13",
	L"cxx_BogusValue_14",

	L"cxx_cond_pars_param",				// 15

	L"cxx_built_in_type",					// 16
	L"cxx_bit_field_type",					// 17
	L"cxx_cv_ptr_ref_type",					// 18
	L"cxx_array_type",						// 19
	L"cxx_data_type_alias",					// 20

	L"cxx_enum_type",						// 21
	L"cxx_enum_member",					// 22
	L"cxx_struct_type",						// 23
	L"cxx_base_class_spec",				// 24
	L"cxx_field_align_spec",					// 25
	L"cxx_friend_spec",					// 26
	L"cxx_data_field",						// 27
	L"cxx_data_field_initializer",				// 28

	L"cxx_function_type",					// 29
	L"cxx_function_type_alias",				// 30
	L"cxx_code_entry_point",				// 31
	L"cxx_possible_exception_spec",			// 32

	L"cxx_namespace",					// 33
	L"cxx_global_namespace",				// 34
	L"cxx_namespace_alias",				// 35
	L"cxx_using_declaration",				// 36
	L"cxx_using_directive",					// 37
	L"cxx_asm_insert",						// 38
	L"cxx_linkage_spec",					// 39

	L"cxx_BogusValue_40",
	L"cxx_BogusValue_41",

	L"cxx_class_template",					// 42
	L"cxx_class_template_spec",				// 43
	L"cxx_class_template_inst",				// 44
	L"cxx_func_template",					// 45
	L"cxx_func_template_spec",				// 46
	L"cxx_func_template_inst",				// 47

	L"cxx_template_type_param",			// 48
	L"cxx_template_template_param",		// 49
	L"cxx_template_type_argument",			// 50
	L"cxx_template_template_argument",		// 51

	L"cxx_unresolved_name_info",			// 52
	L"cxx_unresolved_data_type",			// 53
	L"cxx_unresolved_struct_type",			// 54
	L"cxx_unresolved_data_field",			// 55
	L"cxx_unresolved_code_entry_point",		// 56
	L"cxx_unresolved_class_template",		// 57
	L"cxx_unresolved_func_template",		// 58

	L"cxx_BogusValue_59",
	L"cxx_BogusValue_60",

	L"cxx_empty_stmt",					// 61
	L"cxx_if_stmt",						// 62
	L"cxx_switch_stmt",					// 63
	L"cxx_for_stmt",						// 64
	L"cxx_while_stmt",						// 65
	L"cxx_do_while_stmt",					// 66

	L"cxx_goto_stmt",						// 67
	L"cxx_break_stmt",					// 68
	L"cxx_continue_stmt",					// 69
	L"cxx_return_stmt",					// 70

	L"cxx_block",							// 71
	L"cxx_try_block",						// 72
	L"cxx_catch_block",					// 73
	L"cxx_light_block",						// 74
	L"cxx_label",							// 75
	L"cxx_case_label",						// 76

	L"cxx_operand_expr",					// 77
	L"cxx_unary_expr",					// 78
	L"cxx_binary_expr",					// 79
	L"cxx_multiop_expr",					// 80
	L"cxx_assignment_expr",				// 81

	L"cxx_throw_expr",					// 82
	L"cxx_conditional_expr",				// 83
	L"cxx_field_access_expr",				// 84
	L"cxx_field_access_ptr_expr",			// 85
	L"cxx_array_index_expr",				// 86
	L"cxx_function_call_expr",				// 87

	L"cxx_cast_expr",						// 88
	L"cxx_sizeof_expr",					// 89
	L"cxx_new_expr",						// 90
	L"cxx_delete_expr",					// 91
	L"cxx_typeid_expr",					// 92
};

static const wchar_t *g_CppSourceAreaTypeEnumNames[cxa_num_types] =
{
	L"cxa_none",				L"cxa_bright_keyword",		L"cxa_namespace",

	L"cxa_enum_fwd_decl",		L"cxa_struct_fwd_decl",		L"cxa_union_fwd_decl",		L"cxa_class_fwd_decl",
	L"cxa_enum_defn",			L"cxa_struct_defn",			L"cxa_union_defn",			L"cxa_class_defn",

	L"cxa_enum_member",
	L"cxa_data_field_xtrn",		L"cxa_data_field_defn",		L"cxa_function_proto",		L"cxa_function_header",

	L"cxa_nameless_bft",		L"cxa_nameless_cvpr",		L"cxa_nameless_array",		L"cxa_nameless_fntp",
};

const wchar_t *TCppItemHelper::GetItemTypeDisplayName(TCppItemType item_type)
{
	if (item_type < 0 || item_type >= cxx_num_types)
		return(L"BogusCppItemType");

	return(g_CppItemDisplayNames[item_type]);
}

const wchar_t *TCppItemHelper::GetItemTypeEnumName(TCppItemType item_type)
{
	if (item_type < 0 || item_type >= cxx_num_types)
		return(L"BogusCppItemType");

	return(g_CppItemEnumNames[item_type]);
}

const wchar_t	*TCppItemHelper::GetSrcAreaTypeEnumName(TCppSourceAreaType val)
{
	if (val < 0 || val >= cxa_num_types)
		return(L"BogusCppSourceAreaType");

	return(g_CppSourceAreaTypeEnumNames[val]);
}

//-----------------------------------------------------------------------------
//  ====================  CtvEventHeader  ==========================
//-----------------------------------------------------------------------------

// Messages for the names of the events.
static const wchar_t *g_CtvMidLevEventTypeNames[pset_num_types] =
{
	L"Bogus event type (zero)",				// pset_none
	L"Drive subst registration",				// pset_register_drive_subst
	L"Temp file registration",				// pset_register_temp_file
	L"Paste buffering state change",			// pset_paste_buffering_change
	L"Lex source enter",					// pset_lex_source_enter
	L"Lex source exit",						// pset_lex_source_exit
	L"Macro def/undef statement",			// pset_macro_library_change
	L"Conditional parsing statement",			// pset_cond_parsing_change
	L"Misc preprocessor statement",			// pset_misc_preproc_stmt
	L"Include path definition",				// pset_new_include_path
	L"Error or warning",					// pset_error_warning_event
	L"Conditional parameter use",			// pset_cond_param_use
	L"Grammar definition section",			// pset_grammar_section
	L"Grammar definition object",			// pset_grammar_object
	L"Cpp object reference",				// pset_cpp_item_ref
};

// Messages for the events listbox.
static const wchar_t *g_CtvMidLevEventTypeNamesForListBox[pset_num_types] =
{
	L"EmptyEvent",						// pset_none
	L"DriveSubst",							// pset_register_drive_subst
	L"RegisterTempFile",					// pset_register_temp_file
	L"PasteBuffChg",						// pset_paste_buffering_change
	L"LexSrcEnter",						// pset_lex_source_enter
	L"LexSrcExit",							// pset_lex_source_exit
	L"DefUndefStmt",						// pset_macro_library_change
	L"CondParsStmt",						// pset_cond_parsing_change
	L"MiscEvent",							// pset_misc_preproc_stmt
	L"IncludePath",						// pset_new_include_path
	L"ErrorWarn",							// pset_error_warning_event
	L"CondParamUse",						// pset_cond_param_use
	L"GrammarSection",					// pset_grammar_section
	L"GrammarObject",					// pset_grammar_object
	L"CppItemRef",						// pset_cpp_item_ref
};

// Enum member names.
static const wchar_t *g_CtvMidLevEventTypeEnumNames[pset_num_types] =
{
	L"pset_none",
	L"pset_register_drive_subst",
	L"pset_register_temp_file",
	L"pset_paste_buffering_change",
	L"pset_lex_source_enter",
	L"pset_lex_source_exit",
	L"pset_macro_library_change",
	L"pset_cond_parsing_change",
	L"pset_misc_preproc_stmt",
	L"pset_new_include_path",
	L"pset_error_warning_event",
	L"pset_cond_param_use",
	L"pset_grammar_section",
	L"pset_grammar_object",
	L"pset_cpp_item_ref",
};

const wchar_t *CtvEventHeader::GetEventTypeName()
{
	if (event_type < 0 || event_type >= pset_num_types)
		return(L"Bogus event_type value");

	return(g_CtvMidLevEventTypeNames[event_type]);
}

const wchar_t *CtvEventHeader::GetEventTypeNameForListBox()
{
	if (event_type < 0 || event_type >= pset_num_types)
		return(L"BogusEventType");

	return(g_CtvMidLevEventTypeNamesForListBox[event_type]);
}

const wchar_t *CtvEventHeader::GetEventTypeName(CtvEventType val)
{
	if (val < 0 || val >= pset_num_types)
		return(L"BogusEventTypeValue");

	return(g_CtvMidLevEventTypeNames[val]);
}

const wchar_t *CtvEventHeader::GetEventTypeEnumName(CtvEventType val)
{
	if (val < 0 || val >= pset_num_types)
		return(L"Bogus CtvEventType enum value");

	return(g_CtvMidLevEventTypeEnumNames[val]);
}

const wchar_t *CtvEventHeader::GetEventTypeNameForXml(CtvEventType val)
{
	if (val < 0 || val >= pset_num_types)
		return(L"BogusEventType");

	return(g_CtvMidLevEventTypeNamesForListBox[val]);
}

//-------------------------------------------------------------------------------
//  =====================  TKeyInfo  ===============================
//-------------------------------------------------------------------------------

static const wchar_t *g_KeyTypeNames[kt_num_types] =
{
	L"Name",
	L"Number",
	L"Token",
};

static const wchar_t *g_RawContextTypeNames[rctx_num_types] =
{
	L"Normal",
	L"String",
	L"Comment",
};

const wchar_t *TKeyInfo::GetKeyTypeName(TKeyType ktp, wchar_t *buff_40_chars)
{
	if (ktp < 0 || ktp >= kt_num_types)
	{
		swprintf(buff_40_chars, 40, L"BogusVal(%d)", (int)ktp);
		return(buff_40_chars);
	}

	return(g_KeyTypeNames[ktp]);
}

const wchar_t *TKeyInfo::GetRawContextTypeName(TRawContextType ctxt, wchar_t *buff_40_chars)
{
	if (ctxt < 0 || ctxt >= rctx_num_types)
	{
		swprintf(buff_40_chars, 40, L"BogusVal(%d)", (int)ctxt);
		return(buff_40_chars);
	}

	return(g_RawContextTypeNames[ctxt]);
}

//------------------------------------------------------------------------
//  =================  THighLevelDatabase  ======================
//------------------------------------------------------------------------

static const wchar_t *g_HighLevelLangTypeNames[lang_num_types] =
{
	L"RawIndexingSess",		// lang_none,
	L"Unknown",				// lang_unknown,
	L"GrammarDefn",			// lang_grammar_def,
	L"C-lang",				// lang_clang,
	L"Cpp",					// lang_cpp,
	L"C-like-lang",			// lang_c_like,
	L"C-sharp",				// lang_csh,
};

static const wchar_t *g_HighLevelLangTypeEnumNames[lang_num_types] =
{
	L"lang_none",
	L"lang_unknown",
	L"lang_grammar_def",
	L"lang_clang",
	L"lang_cpp",
	L"lang_c_like",
	L"lang_csh",
};

const wchar_t *THighLevelDatabase::GetLangTypeName(THighLevelLangType val)
{
	if (val < 0 || val >= lang_num_types)
		return(L"BogusHighLevelLangType");

	return(g_HighLevelLangTypeNames[val]);
}

const wchar_t *THighLevelDatabase::GetLangTypeEnumName(THighLevelLangType val)
{
	if (val < 0 || val >= lang_num_types)
		return(L"lang_bogus_high_level_lang_type");

	return(g_HighLevelLangTypeEnumNames[val]);
}

THighLevelLangType THighLevelDatabase::GetLangTypeFromString(const wchar_t *name)
{
	if (_wcsicmp(name, L"none") == 0)
		return(lang_none);

	for (int inx=0; inx<lang_num_types; ++inx)
	{
		if (_wcsicmp(name, g_HighLevelLangTypeNames[inx]) == 0 || _wcsicmp(name, g_HighLevelLangTypeEnumNames[inx]) == 0)
		{
			// Passed string describes one of the known types of the high level languages.
			return((THighLevelLangType)inx);
		}
	}

	// The string is bogus.
	return(lang_num_types);
}

//--------------------------------------------------------------------------
//  =================  THighLevelDataStorage  ======================
//--------------------------------------------------------------------------

void THighLevelDataStorage::ResetStorage()
{
	for (int ibkt=0; ibkt<NUM_DATABASE_BUCKETS; ++ibkt)
	{
		TList &bucket = m_database_buckets[ibkt];
		while (bucket.IsEmpty() == FALSE)
		{
			THighLevelDatabase *db = (THighLevelDatabase*)bucket.GetFirst();
			bucket.RemoveItem(db);

			if (db->m_data_store_owned == TRUE)
				delete db;
		}
	}

	m_latest_database = NULL;
}

THighLevelDatabase *THighLevelDataStorage::FindParsSessDatabase(ID parsing_id)
{
	assert(parsing_id != TProjectParsSessId);
	if (parsing_id == 0 || m_latest_database != NULL && m_latest_database->GetParsingSessionId() == parsing_id)
	{
		// Return the database of the latest session whether it is NULL or not.
		return(m_latest_database);
	}

	// Search the appropriate hash bucket.
	for (TListIter<THighLevelDatabase> iter(m_database_buckets[parsing_id & (NUM_DATABASE_BUCKETS-1)]); iter; ++iter)
	{
		if (iter.CurrItem().GetParsingSessionId() == parsing_id)
			return(iter.CurrItemPtr());
	}

	// The pars sess level database for the requested session id is not available.
	return(NULL);
}

void THighLevelDataStorage::DeleteParsSessDatabase(ID parsing_id)
{
	// Pick up the database.
	THighLevelDatabase *db = FindParsSessDatabase(parsing_id);
	if (db == NULL)
		return;

	// The session is available. Dispose it.
	m_database_buckets[parsing_id & (NUM_DATABASE_BUCKETS-1)].RemoveItem(db);
	if (db->m_data_store_owned == TRUE)
		delete db;

	// This databases manager is not handling the history of sessions creation.
	if (db == m_latest_database)
		m_latest_database = NULL;
}


