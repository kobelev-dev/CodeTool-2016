//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Scanning files with C-style syntax.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "TextScan/RawScan.H"

//-----------------------------------------------------------------------
//  ===================  TLexema  ===========================
//-----------------------------------------------------------------------

static const wchar_t *g_Lexema_LexTypeNames1[ltx_num_lexema_types] =
{
	L"Empty lexema",				// ltx_empty
	L"Comment",					// ltx_comment
	L"Number",					// ltx_number
	L"Floating point constant",		// ltx_floating_point
	L"Character constant",			// ltx_charconst
	L"String constant",				// ltx_string
	L"Keyword",					// ltx_keyword
	L"Name",						// ltx_name
	L"EOL",						// ltx_eol
	L"EOF",						// ltx_eof
	L"ErrWarn",					// ltx_error
};

static const wchar_t *g_Lexema_LexTypeNames2[ltx_num_lexema_types] =
{
	L"empty lexema",				// ltx_empty
	L"comment",					// ltx_comment
	L"number",					// ltx_number
	L"floating point constant",		// ltx_floating_point
	L"character constant",			// ltx_charconst
	L"string constant",				// ltx_string
	L"keyword",					// ltx_keyword
	L"name",						// ltx_name
	L"EOL",						// ltx_eol
	L"EOF",						// ltx_eof
	L"error",						// ltx_error
};

static const wchar_t *g_Lexema_LexTypeEnumNames[ltx_num_lexema_types] =
{
	L"ltx_empty",
	L"ltx_comment",
	L"ltx_number",
	L"ltx_floating_point",
	L"ltx_charconst",
	L"ltx_string",
	L"ltx_keyword",
	L"ltx_name",
	L"ltx_eol",
	L"ltx_eof",
	L"ltx_error",
};

//
// Comment subtype (1)
//

static const wchar_t *g_Lexema_CommentTypeNames[lct_num_comment_types] =
{
	L"C style comment",							// lct_c_style
	L"C++ style comment at the end of the line",		// lct_cpp_endofline
	L"C++ style comment that covers the whole line",	// lct_cpp_wholeline
	L"C# style comment for documenting APIs",		// lct_csh_trislash
};

static const wchar_t *g_Lexema_CommentTypeEnumNames[lct_num_comment_types] =
{
	L"lct_c_style",
	L"lct_cpp_endofline",
	L"lct_cpp_wholeline",
	L"lct_csh_trislash",
};

//
// Number subtype (2)
//

static const wchar_t *g_Lexema_NumberTypeNames[lnt_num_number_types] =
{
	L"Signed 8 bit",				// lnt_s8bit
	L"Unsigned 8 bit",				// lnt_u8bit
	L"Signed 16 bit",				// lnt_s16bit
	L"Unsigned 16 bit",				// lnt_u16bit
	L"Signed 32 bit",				// lnt_s32bit
	L"Unsigned 32 bit",				// lnt_u32bit
	L"Signed 64 bit",				// lnt_s64bit
	L"Unsigned 64 bit",				// lnt_u64bit
	L"Signed 128 bit",				// lnt_s128bit
	L"Unsigned 128 bit",			// lnt_u128bit
};

static const wchar_t *g_Lexema_NumberTypeEnumNames[lnt_num_number_types] =
{
	L"lnt_s8bit",
	L"lnt_u8bit",
	L"lnt_s16bit",
	L"lnt_u16bit",
	L"lnt_s32bit",
	L"lnt_u32bit",
	L"lnt_s64bit",
	L"lnt_u64bit",
	L"lnt_s128bit",
	L"lnt_u128bit",
};

//
// Floating point subtype (3)
//

static const wchar_t *g_Lexema_FloatingPointTypeNames[lfp_num_floating_point_types] =
{
	L"Float (32 bit)",				// lfp_32bit
	L"Double (64 bit)",				// lfp_64bit
	L"Long double (128 bit)",		// lfp_128bit
};

static const wchar_t *g_Lexema_FloatingPointTypeEnumNames[lfp_num_floating_point_types] =
{
	L"lfp_32bit",
	L"lfp_64bit",
	L"lfp_128bit",
};

//
// Charconst subtype (4)
//

static const wchar_t *g_Lexema_CharConstTypeNames[lchct_num_charconst_types] =
{
	L"Ascii char const",				// lchct_ascii
	L"Unicode char const",			// lchct_unicode
	L"C# verbatim char const",		// lchct_csh_verb
};

static const wchar_t *g_Lexema_CharConstTypeEnumNames[lchct_num_charconst_types] =
{
	L"lchct_ascii",
	L"lchct_unicode",
	L"lchct_csh_verb",
};

//
// String subtype (5)
//

static const wchar_t *g_Lexema_StringTypeNames[lstrt_num_string_types] =
{
	L"Ascii string",					// lstrt_ascii
	L"Unicode string",				// lstrt_unicode
	L"C# verbatim string",			// lstrt_csh_verb
};

static const wchar_t *g_Lexema_StringTypeEnumNames[lstrt_num_string_types] =
{
	L"lstrt_ascii",
	L"lstrt_unicode",
	L"lstrt_csh_verb",
};

//
// Keyword subtype (6)
//

static const wchar_t *g_Lexema_KeywordTypeNames1[ltkn_num_keyword_types] =
{
	L"Separator token",			// ltkn_separ
	L"Arithmetic token",			// ltkn_arithm
	L"Preprocessor token",			// ltkn_preproc
	L"C language keyword",			// ltkn_clang
	L"C++ language keyword",		// ltkn_cpp
	L"C# language keyword",		// ltkn_csh
};

static const wchar_t *g_Lexema_KeywordTypeNames2[ltkn_num_keyword_types] =
{
	L"separator token",				// ltkn_separ
	L"arithmetic token",			// ltkn_arithm
	L"preprocessor token",			// ltkn_preproc
	L"C language keyword",			// ltkn_clang
	L"C++ language keyword",		// ltkn_cpp
	L"C# language keyword",		// ltkn_csh
};

static const wchar_t *g_Lexema_KeywordTypeEnumNames[ltkn_num_keyword_types] =
{
	L"ltkn_separ",
	L"ltkn_arithm",
	L"ltkn_preproc",
	L"ltkn_clang",
	L"ltkn_cpp",
	L"ltkn_csh",
};

//
// End of line subtype (7)
//

static const wchar_t *g_Lexema_EndOfLineTypeNames[leolt_num_eol_types] =
{
	L"Ordinary",					// leolt_normal
	L"Eol with backslash in front",	// leolt_withbksl
};

static const wchar_t *g_Lexema_EndOfLineTypeEnumNames[leolt_num_eol_types] =
{
	L"leolt_normal",
	L"leolt_withbksl",
};

//
// Error class subtype (8)
//

static const wchar_t *g_Lexema_ErrorClassNames[lerrc_num_errorclass_types] =
{
	L"Parsing message",			// lerrc_message		0
	L"Bogus errClass (1)",			// bogus_value		1
	L"Bogus errClass (2)",			// bogus_value		2
	L"Raw scanner error",			// lerrc_raw_scanner	3
	L"Preprocessor warning",		// lerrc_preproc_warn	4
	L"Preprocessor error",			// lerrc_preproc_err	5
	L"Syntax warning",				// lerrc_syntax_warn	6
	L"Syntax error",				// lerrc_syntax_err	7
	L"Analysis warning",			// lerrc_analysis_warn	8
	L"Analysis error",				// lerrc_analysis_err	9
};

static const wchar_t *g_Lexema_ErrorClassEnumNames[lerrc_num_errorclass_types] =
{
	L"lerrc_message",
	L"BogusErrClass(1)",
	L"BogusErrClass(2)",
	L"lerrc_raw_scanner",
	L"lerrc_preproc_warn",
	L"lerrc_preproc_err",
	L"lerrc_syntax_warn",
	L"lerrc_syntax_err",
	L"lerrc_analysis_warn",
	L"lerrc_analysis_err",
};

//
//	Keyword classes (tokens)
//

static const wchar_t *g_Lexema_StdSeparTokenEnumNames[spr_max_token-spr_none] =
{
	L"spr_none",
	L"spr_lcurvbr",
	L"spr_rcurvbr",
	L"spr_semicol",
	L"spr_ellipsis",
};

static const wchar_t *g_Lexema_StdSeparTokenRepr[opr_max_token-opr_none] =
{
	L"<X>",
	L"{",
	L"}",
	L";",
	L"...",
};

static const wchar_t *g_Lexema_StdArithmTokenEnumNames[opr_max_token-opr_none] =
{
	L"opr_none",			L"opr_defined",

	L"opr_comma",		L"opr_lpar",			L"opr_rpar",
	L"opr_bang",			L"opr_tilda",
	L"opr_unplus",			L"opr_unminus",
	L"opr_deref_ptr",		L"opr_get_addr",
	L"opr_arr_index",		L"opr_funcall",

	L"opr_mul",			L"opr_div",			L"opr_rmnd",
	L"opr_plus",			L"opr_minus",
	L"opr_bitlsh",			L"opr_bitrsh",
	L"opr_bitand",			L"opr_bitor",			L"opr_bitxor",
	L"opr_lt",				L"opr_le",			L"opr_gt",
	L"opr_ge",			L"opr_eq",			L"opr_ne",
	L"opr_logand",		L"opr_logor",
	L"opr_question",		L"opr_colon",

	L"opr_dblcolon",
	L"opr_dot",			L"opr_arrow",
	L"opr_dotstar",		L"opr_arrowstar",
	L"opr_lbracket",		L"opr_rbracket",
	L"opr_plusplus",		L"opr_minusminus",
	L"opr_plpl_postfix",		L"opr_mnmn_postfix",

	L"opr_sizeof",			L"opr_new",			L"opr_delete",
	L"opr_new_array",		L"opr_delete_array",
	L"opr_throw",			L"opr_typeid",			L"opr_cast",
	L"opr_const_cast",		L"opr_dyna_cast",
	L"opr_rein_cast",		L"opr_static_cast",

	L"opr_assign",
	L"opr_mul_asgn",		L"opr_div_asgn",		L"opr_rmnd_asgn",
	L"opr_plus_asgn",		L"opr_minus_asgn",
	L"opr_lsh_asgn",		L"opr_rsh_asgn",
	L"opr_btand_asgn",	L"opr_btor_asgn",		L"opr_btxor_asgn",
};

static const wchar_t *g_Lexema_StdArithmTokenRepr[opr_max_token-opr_none] =
{

	L"<none>",		L"defined",

	L",",				L"(",				L")",
	L"!",				L"~",

	L"+",			L"-",
	L"*",				L"&",
	L"[]",			L"call",

	L"*",				L"/",				L"%",
	L"+",			L"-",
	L"<<",			L">>",
	L"&",			L"|",				L"^",
	L"<",			L">=",			L">",
	L">=",			L"==",			L"!=",
	L"&&",			L"||",
	L"?",				L":",

	L"::",
	L".",				L"->",
	L".*",			L"->*",
	L"[",				L"]",
	L"++",			L"--",
	L"++",			L"--",

	L"sizeof",			L"new",			L"delete",
	L"new []",		L"delete []",
	L"throw",			L"typeid",			L"(cast)",
	L"const_cast",		L"dyna_cast",
	L"rein_cast",		L"static_cast",

	L"=",
	L"*=",			L"/=",			L"%=",
	L"+=",			L"-=",
	L"<<=",			L">>=",
	L"&=",			L"|=",			L"^=",
};

static const wchar_t *g_Lexema_InlinePreProcTokenEnumNames[mpr_define-mpr_none] =
{
	L"mpr_none", L"mpr_prm_chr", L"mpr_prm_str", L"mpr_prm_cnct",
};

// The first element should be the mpr_vargs because this is the only one element that is not preceeded with #.
// The code below is using this fact.
static const TKeywordsDictEntry g_Lexema_PreProcTokensDict[mpr_max_token-mpr_vargs+1] =
{
	{  DefKW(L"__VA_ARGS__"),	ltkn_preproc,		mpr_vargs,		L"mpr_vargs"		},
	{  DefKW(L"define"),			ltkn_preproc,		mpr_define,		L"mpr_define"		},
	{  DefKW(L"undef"),			ltkn_preproc,		mpr_undef,		L"mpr_undef"		},
	{  DefKW(L"include"),			ltkn_preproc,		mpr_include,		L"mpr_include"	},
	{  DefKW(L"ifdef"),			ltkn_preproc,		mpr_ifdef,		L"mpr_ifdef"		},
	{  DefKW(L"ifndef"),			ltkn_preproc,		mpr_ifndef,		L"mpr_ifndef"		},
	{  DefKW(L"if"),				ltkn_preproc,		mpr_if,			L"mpr_if"			},
	{  DefKW(L"elif"),				ltkn_preproc,		mpr_elif,			L"mpr_elif"		},
	{  DefKW(L"else"),				ltkn_preproc,		mpr_else,		L"mpr_else"		},
	{  DefKW(L"endif"),			ltkn_preproc,		mpr_endif,		L"mpr_endif"		},
	{  DefKW(L"pragma"),			ltkn_preproc,		mpr_pragma,		L"mpr_pragma"	},
	{  DefKW(L"error"),			ltkn_preproc,		mpr_error,		L"mpr_error"		},
	{  DefKW(L"import"),			ltkn_preproc,		mpr_import,		L"mpr_import"		},
	{  DefKW(L"line"),				ltkn_preproc,		mpr_line,			L"mpr_line"		},
	{  DefKW(L"using"),			ltkn_preproc,		mpr_using,		L"mpr_using"		},
	{  DefKW(L"warning"),			ltkn_preproc,		mpr_warning,		L"mpr_warning"	},
	{  DefKW(L"region"),			ltkn_preproc,		mpr_region,		L"mpr_region"		},
	{  DefKW(L"endregion"),		ltkn_preproc,		mpr_endregion,	L"mpr_endregion"	},
	{  NULL, 0, 0, 0, NULL  }
};

static const TKeywordsDictEntry g_Lexema_CLangTokensDict[clg_max_token-clg_none] =
{
	{  DefKW(L"asm"),				ltkn_clang,		clg_asm,			L"clg_asm"		},
	{  DefKW(L"auto"),			ltkn_clang,		clg_auto,			L"clg_auto"		},
	{  DefKW(L"break"),			ltkn_clang,		clg_break,		L"clg_break"		},
	{  DefKW(L"case"),			ltkn_clang,		clg_case,			L"clg_case"		},
	{  DefKW(L"catch"),			ltkn_clang,		clg_catch,		L"clg_catch"		},
	{  DefKW(L"char"),			ltkn_clang,		clg_char,			L"clg_char"		},
	{  DefKW(L"const"),			ltkn_clang,		clg_const,		L"clg_const"		},
	{  DefKW(L"continue"),			ltkn_clang,		clg_continue,		L"clg_continue"	},
	{  DefKW(L"default"),			ltkn_clang,		clg_default,		L"clg_default"		},
	{  DefKW(L"do"),				ltkn_clang,		clg_do,			L"clg_do"			},
	{  DefKW(L"double"),			ltkn_clang,		clg_double,		L"clg_double"		},
	{  DefKW(L"else"),				ltkn_clang,		clg_else,			L"clg_else"		},
	{  DefKW(L"enum"),			ltkn_clang,		clg_enum,		L"clg_enum"		},
	{  DefKW(L"extern"),			ltkn_clang,		clg_extern,		L"clg_extern"		},
	{  DefKW(L"float"),			ltkn_clang,		clg_float,			L"clg_float"		},
	{  DefKW(L"for"),				ltkn_clang,		clg_for,			L"clg_for"			},
	{  DefKW(L"goto"),			ltkn_clang,		clg_goto,			L"clg_goto"		},
	{  DefKW(L"if"),				ltkn_clang,		clg_if,			L"clg_if"			},
	{  DefKW(L"int"),				ltkn_clang,		clg_int,			L"clg_int"			},
	{  DefKW(L"long"),				ltkn_clang,		clg_long,			L"clg_long"		},
	{  DefKW(L"register"),			ltkn_clang,		clg_register,		L"clg_register"		},
	{  DefKW(L"return"),			ltkn_clang,		clg_return,		L"clg_return"		},
	{  DefKW(L"short"),			ltkn_clang,		clg_short,		L"clg_short"		},
	{  DefKW(L"signed"),			ltkn_clang,		clg_signed,		L"clg_signed"		},
	{  DefKW(L"static"),			ltkn_clang,		clg_static,		L"clg_static"		},
	{  DefKW(L"struct"),			ltkn_clang,		clg_struct,		L"clg_struct"		},
	{  DefKW(L"switch"),			ltkn_clang,		clg_switch,		L"clg_switch"		},
	{  DefKW(L"try"),				ltkn_clang,		clg_try,			L"clg_try"			},
	{  DefKW(L"typedef"),			ltkn_clang,		clg_typedef,		L"clg_typedef"		},
	{  DefKW(L"union"),			ltkn_clang,		clg_union,		L"clg_union"		},
	{  DefKW(L"unsigned"),			ltkn_clang,		clg_unsigned,		L"clg_unsigned"	},
	{  DefKW(L"void"),				ltkn_clang,		clg_void,			L"clg_void"		},
	{  DefKW(L"volatile"),			ltkn_clang,		clg_volatile,		L"clg_volatile"		},
	{  DefKW(L"wchar_t"),			ltkn_clang,		clg_wchar_t,		L"clg_wchar_t"	},
	{  DefKW(L"while"),			ltkn_clang,		clg_while,		L"clg_while"		},
	{  NULL, 0, 0, 0, NULL }
};

static const TKeywordsDictEntry g_Lexema_CppLangTokensDict[cpp_max_token-cpp_none] =
{
	{  DefKW(L"bool"),				ltkn_cpp,			cpp_bool,			L"cpp_bool"		},
	{  DefKW(L"class"),			ltkn_cpp,			cpp_class,		L"cpp_class"		},
	{  DefKW(L"explicit"),			ltkn_cpp,			cpp_explicit,		L"cpp_explicit"		},
	{  DefKW(L"export"),			ltkn_cpp,			cpp_export,		L"cpp_export"		},
	{  DefKW(L"false"),			ltkn_cpp,			cpp_false,		L"cpp_false"		},
	{  DefKW(L"friend"),			ltkn_cpp,			cpp_friend,		L"cpp_friend"		},
	{  DefKW(L"inline"),			ltkn_cpp,			cpp_inline,		L"cpp_inline"		},
	{  DefKW(L"mutable"),			ltkn_cpp,			cpp_mutable,		L"cpp_mutable"	},
	{  DefKW(L"namespace"),		ltkn_cpp,			cpp_namespace,	L"cpp_namespace"	},
	{  DefKW(L"operator"),			ltkn_cpp,			cpp_operator,		L"cpp_operator"	},
	{  DefKW(L"private"),			ltkn_cpp,			cpp_private,		L"cpp_private"		},
	{  DefKW(L"protected"),		ltkn_cpp,			cpp_protected,	L"cpp_protected"	},
	{  DefKW(L"public"),			ltkn_cpp,			cpp_public,		L"cpp_public"		},
	{  DefKW(L"template"),			ltkn_cpp,			cpp_template,		L"cpp_template"	},
	{  DefKW(L"this"),				ltkn_cpp,			cpp_this,			L"cpp_this"		},
	{  DefKW(L"true"),				ltkn_cpp,			cpp_true,			L"cpp_true"		},
	{  DefKW(L"typename"),		ltkn_cpp,			cpp_typename,	L"cpp_typename"	},
	{  DefKW(L"using"),			ltkn_cpp,			cpp_using,		L"cpp_using"		},
	{  DefKW(L"virtual"),			ltkn_cpp,			cpp_virtual,		L"cpp_virtual"		},
	{  NULL, 0, 0, 0, NULL }
};

static const TKeywordsDictEntry g_Lexema_CshLangTokensDict[csh_max_token-csh_none] =
{
	{  DefKW(L"abstract"),			ltkn_csh,			csh_abstract,		L"csh_abstract"	},
	{  DefKW(L"as"),				ltkn_csh,			csh_as,			L"csh_as"			},
	{  DefKW(L"base"),			ltkn_csh,			csh_base,		L"csh_base"		},
	{  DefKW(L"bool"),				ltkn_csh,			csh_bool,			L"csh_bool"		},
	{  DefKW(L"break"),			ltkn_csh,			csh_break,		L"csh_break"		},
	{  DefKW(L"byte"),			ltkn_csh,			csh_byte,			L"csh_byte"		},
	{  DefKW(L"case"),			ltkn_csh,			csh_case,		L"csh_case"		},
	{  DefKW(L"catch"),			ltkn_csh,			csh_catch,		L"csh_catch"		},
	{  DefKW(L"char"),			ltkn_csh,			csh_char,			L"csh_char"		},
	{  DefKW(L"checked"),			ltkn_csh,			csh_checked,		L"csh_checked"	},
	{  DefKW(L"class"),			ltkn_csh,			csh_class,		L"csh_class"		},
	{  DefKW(L"const"),			ltkn_csh,			csh_const,		L"csh_const"		},
	{  DefKW(L"continue"),			ltkn_csh,			csh_continue,		L"csh_continue"	},
	{  DefKW(L"decimal"),			ltkn_csh,			csh_decimal,		L"csh_decimal"	},
	{  DefKW(L"default"),			ltkn_csh,			csh_default,		L"csh_default"		},
	{  DefKW(L"delegate"),			ltkn_csh,			csh_delegate,		L"csh_delegate"	},
	{  DefKW(L"do"),				ltkn_csh,			csh_do,			L"csh_do"		},
	{  DefKW(L"double"),			ltkn_csh,			csh_double,		L"csh_double"		},
	{  DefKW(L"else"),				ltkn_csh,			csh_else,			L"csh_else"		},
	{  DefKW(L"enum"),			ltkn_csh,			csh_enum,		L"csh_enum"		},
	{  DefKW(L"event"),			ltkn_csh,			csh_event,		L"csh_event"		},
	{  DefKW(L"explicit"),			ltkn_csh,			csh_explicit,		L"csh_explicit"		},
	{  DefKW(L"extern"),			ltkn_csh,			csh_extern,		L"csh_extern"		},
	{  DefKW(L"false"),			ltkn_csh,			csh_false,		L"csh_false"		},
	{  DefKW(L"finally"),			ltkn_csh,			csh_finally,		L"csh_finally"		},
	{  DefKW(L"fixed"),			ltkn_csh,			csh_fixed,		L"csh_fixed"		},
	{  DefKW(L"float"),			ltkn_csh,			csh_float,			L"csh_float"		},
	{  DefKW(L"for"),				ltkn_csh,			csh_for,			L"csh_for"		},
	{  DefKW(L"foreach"),			ltkn_csh,			csh_foreach,		L"csh_foreach"	},
	{  DefKW(L"goto"),			ltkn_csh,			csh_goto,			L"csh_goto"		},
	{  DefKW(L"if"),				ltkn_csh,			csh_if,			L"csh_if"			},
	{  DefKW(L"implicit"),			ltkn_csh,			csh_implicit,		L"csh_implicit"		},
	{  DefKW(L"in"),				ltkn_csh,			csh_in,			L"csh_in"			},
	{  DefKW(L"int"),				ltkn_csh,			csh_int,			L"csh_int"		},
	{  DefKW(L"interface"),			ltkn_csh,			csh_interface,		L"csh_interface"	},
	{  DefKW(L"internal"),			ltkn_csh,			csh_internal,		L"csh_internal"	},
	{  DefKW(L"is"),				ltkn_csh,			csh_is,			L"csh_is"			},
	{  DefKW(L"lock"),				ltkn_csh,			csh_lock,			L"csh_lock"		},
	{  DefKW(L"long"),				ltkn_csh,			csh_long,			L"csh_long"		},
	{  DefKW(L"namespace"),		ltkn_csh,			csh_namespace,	L"csh_namespace"	},
	{  DefKW(L"new"),				ltkn_csh,			csh_new,			L"csh_new"		},
	{  DefKW(L"null"),				ltkn_csh,			csh_null,			L"csh_null"		},
	{  DefKW(L"object"),			ltkn_csh,			csh_object,		L"csh_object"		},
	{  DefKW(L"operator"),			ltkn_csh,			csh_operator,		L"csh_operator"	},
	{  DefKW(L"out"),				ltkn_csh,			csh_out,			L"csh_out"		},
	{  DefKW(L"override"),			ltkn_csh,			csh_override,		L"csh_override"	},
	{  DefKW(L"params"),			ltkn_csh,			csh_params,		L"csh_params"	},
	{  DefKW(L"private"),			ltkn_csh,			csh_private,		L"csh_private"		},
	{  DefKW(L"protected"),		ltkn_csh,			csh_protected,	L"csh_protected"	},
	{  DefKW(L"public"),			ltkn_csh,			csh_public,		L"csh_public"		},
	{  DefKW(L"readonly"),			ltkn_csh,			csh_readonly,		L"csh_readonly"	},
	{  DefKW(L"ref"),				ltkn_csh,			csh_ref,			L"csh_ref"		},
	{  DefKW(L"return"),			ltkn_csh,			csh_return,		L"csh_return"		},
	{  DefKW(L"sbyte"),			ltkn_csh,			csh_sbyte,		L"csh_sbyte"		},
	{  DefKW(L"sealed"),			ltkn_csh,			csh_sealed,		L"csh_sealed"		},
	{  DefKW(L"short"),			ltkn_csh,			csh_short,		L"csh_short"		},
	{  DefKW(L"sizeof"),			ltkn_csh,			csh_sizeof,		L"csh_sizeof"		},
	{  DefKW(L"stackalloc"),		ltkn_csh,			csh_stackalloc,	L"csh_stackalloc"	},
	{  DefKW(L"static"),			ltkn_csh,			csh_static,		L"csh_static"		},
	{  DefKW(L"string"),			ltkn_csh,			csh_string,		L"csh_string"		},
	{  DefKW(L"struct"),			ltkn_csh,			csh_struct,		L"csh_struct"		},
	{  DefKW(L"switch"),			ltkn_csh,			csh_switch,		L"csh_switch"		},
	{  DefKW(L"this"),				ltkn_csh,			csh_this,			L"csh_this"		},
	{  DefKW(L"throw"),			ltkn_csh,			csh_throw,		L"csh_throw"		},
	{  DefKW(L"true"),				ltkn_csh,			csh_true,			L"csh_true"		},
	{  DefKW(L"try"),				ltkn_csh,			csh_try,			L"csh_try"		},
	{  DefKW(L"typeof"),			ltkn_csh,			csh_typeof,		L"csh_typeof"		},
	{  DefKW(L"uint"),				ltkn_csh,			csh_uint,			L"csh_uint"		},
	{  DefKW(L"ulong"),			ltkn_csh,			csh_ulong,		L"csh_ulong"		},
	{  DefKW(L"unchecked"),		ltkn_csh,			csh_unchecked,	L"csh_unchecked"	},
	{  DefKW(L"unsafe"),			ltkn_csh,			csh_unsafe,		L"csh_unsafe"		},
	{  DefKW(L"ushort"),			ltkn_csh,			csh_ushort,		L"csh_ushort"		},
	{  DefKW(L"using"),			ltkn_csh,			csh_using,		L"csh_using"		},
	{  DefKW(L"virtual"),			ltkn_csh,			csh_virtual,		L"csh_virtual"		},
	{  DefKW(L"void"),				ltkn_csh,			csh_void,			L"csh_void"		},
	{  DefKW(L"volatile"),			ltkn_csh,			csh_volatile,		L"csh_volatile"		},
	{  DefKW(L"while"),			ltkn_csh,			csh_while,		L"csh_while"		},
	{  NULL, 0, 0, 0, NULL }
};

void TLexema::AppendTo(TTextBuffer256 &str)
{
	wchar_t buffer[80];
	switch (type)
	{
		case ltx_empty:
				{
					str.Append(L"<empty_lexema>");
				}
				break;

		case ltx_comment:
				{
					str.Append(L" /* comment */ ");
				}
				break;

		case ltx_number:
				{
					swprintf(buffer, 80, L"%I64d", num_value);
					str.Append(buffer);
				}
				break;

		case ltx_floating_point:
				{
					swprintf(buffer, 80, L"%lg", GetDoubleValue());
					str.Append(buffer);
				}
				break;

		case ltx_charconst:
				{
					swprintf(buffer, 80, L"%I64d", num_value);
					str.Append(buffer);
				}
				break;

		case ltx_string:
				{
					if (string_type == lchct_unicode)
						str.Append(L"L");
					str.AppendQuoted(str_value.CopyWithTruncationTo(buffer, 80), -1, FALSE);
				}
				break;

		case ltx_keyword:
				{
					str.Append(GetKeywordText((int)num_value));
				}
				break;

		case ltx_name:
				{
					str.Append(str_value.CopyWithTruncationTo(buffer, 80));
				}
				break;

		case ltx_eol:
				{
					str.Append(L"\r\n");
				}
				break;

		default:
			assert(FALSE);
			break;
	}
}

bool TLexema::IsEqualTo(TLexema &lex)
{
	// Compare the types of lexemas.
	if (type != lex.type)
		return(FALSE);

	// The types are identical. Check other props.
	switch (type)
	{
		case ltx_number:
				return(subtype == lex.subtype && num_value == lex.num_value);

		case ltx_floating_point:
				return(subtype == lex.subtype && mantissa1 == lex.mantissa1 && mantissa2 == lex.mantissa2 && mantissa3 == lex.mantissa3 && exponent == lex.exponent);

		case ltx_charconst:
				return(subtype == lex.subtype && num_value == lex.num_value);

		case ltx_string:
				return(subtype == lex.subtype && str_value == lex.str_value);

		case ltx_keyword:
				return(subtype == lex.subtype && num_value == lex.num_value);

		case ltx_name:
				return(str_value == lex.str_value);

		case ltx_error:
				return(num_value == lex.num_value);
	}

	// Other lexema types do not have extra props to compare.
	return(TRUE);
}

float TLexema::GetFloatValue()
{
	assert(type == ltx_floating_point);

	// Assemble the IEEE-754 4-bytes format out of the internal IEEE-754 16-bytes format.
	union { DWORD int_copy; float float_copy; } float_value;

	if (exponent <= 0x7F)
	{
		// The value of the constant is too small or it is zero. Setup the floating point zero value.
		float_value.int_copy = 0;
	}
	else if (exponent <= 0x7F7F)
	{
		// The 32 bit format uses 9 bits for the sign and exponent field and 23 bits for the mantissa field.
		float_value.int_copy = (DWORD)(mantissa1 >> (64-23));
		float_value.int_copy |= ((DWORD)(exponent-0x3FFF+0x7F) & 0xFF) << 23;
	}
	else
	{
		// The value of the constant is too big. Setup a positive infinity value.
		float_value.int_copy = 0x7F800000;
	}

	return(float_value.float_copy);
}

wchar_t *TLexema::SprintfLexema(wchar_t *buffer, long buffer_len)
{
	assert(buffer_len >= 80);

	wchar_t lc_buff[160];
	switch (type)
	{
		case ltx_empty:
				wcscpy(buffer, L"EmptyLexema");
				break;

		case ltx_comment:
				swprintf(buffer, buffer_len, L"Comment, %s, len=%ld", GetLexCommentTypeEnumName(comment_type), origin.src_area.area_len);
				break;

		case ltx_number:
				swprintf(buffer, buffer_len, L"Number, subt=%s, val=%I64d, val=0x%016I64X", GetLexNumberTypeEnumName(number_type), num_value, num_value);
				break;

		case ltx_floating_point:
				{
					// Show this floating point constant in the HEX format.
					double dbl_value = GetDoubleValue();
					swprintf(buffer, buffer_len, L"FloatingPointConst, subt=%s, exp=%04X, mant=%04X.%04X.%04X.%04X.%04X.%04X.%04X, val=%lg",
							GetLexFloatingPointTypeEnumName(floating_point_type), exponent,
							(DWORD)((mantissa1 >> 48) & 0xFFFF), (DWORD)((mantissa1 >> 32) & 0xFFFF), (DWORD)((mantissa1 >> 16) & 0xFFFF),
							(DWORD)(mantissa1 & 0xFFFF), (mantissa2 >> 16) & 0xFFFF, mantissa2 & 0xFFFF, mantissa3, dbl_value);
				}
				break;

		case ltx_charconst:
				swprintf(buffer, buffer_len, L"ChrCst, subt=%s, val=%I64d, val=0x%016I64X", GetLexCharConstTypeEnumName(charconst_type), num_value, num_value);
				break;

		case ltx_string:
				swprintf(buffer, buffer_len, L"String, val=%s\"%s\"", (string_type == lstrt_unicode) ? L"L" : L"", str_value.CopyWithTruncationTo(lc_buff, 160));
				break;

		case ltx_keyword:
				swprintf(buffer, buffer_len, L"KeyWrd, id=%d(%s)", (int)num_value, GetKeywordIdEnumName((int)num_value));
				break;

		case ltx_name:
				swprintf(buffer, buffer_len, L"-Name-, \"%s\"", str_value.CopyWithTruncationTo(lc_buff, 160));
				break;

		case ltx_eol:
				{
					if (endofline_type == leolt_normal)
						swprintf(buffer, buffer_len, L" <eol>");
					else swprintf(buffer, buffer_len, L" <eol>, type=%s, src_area_len=%d", GetLexEndOfLineTypeEnumName(endofline_type), origin.src_area.area_len);
				}
				break;

		case ltx_eof:
				wcscpy(buffer, L"==EOF==");
				break;

		case ltx_error:
				swprintf(buffer, buffer_len, L"%s, Class=%d, Errno=%d, Msg=\"%s\"", (LEX_ERROR_CLASS_ERROR(error_class) == TRUE) ? L"*Error" : L"Warning", (int)error_class, (int)num_value, str_value.CopyWithTruncationTo(lc_buff, 160));
				break;

		default:
			swprintf(buffer, buffer_len, L"Unknown lexema type=%d", type);
			break;
	}

	return(buffer);
}

const wchar_t *TLexema::GetCssStyleName() const
{
	TLexSubtype subt;
	subt.subtype = (type != ltx_keyword) ? subtype : (DWORD)num_value;
	return(GetCssStyleName(type, subt));
}

TLexemaType TLexema::SetFloatingPoint(TLexFloatingPointType subt, int exp, UINT64 mant1, UINT64 mant2)
{
	// Setup the lexema type fields.
	type = ltx_floating_point;
	floating_point_type = subt;

	if (mant1 == 0)
	{
		// This is the zero value. In this case other params should also be zeroes.
		assert(exp == 0 && mant2 == 0);
	}
	else
	{
		// The value of mantissa should be normalized. Ensure this.
		assert((mant1 & 0x8000000000000000) != 0);
		assert(exp >= -16382+1 && exp <= 16383+1);

		// Apply the required bias to the exponent. Additional one is subtracted here because the mantissa
		// is shifted left for one bit below.
		exp += 0x3FFF-1;
		exp &= 0x7FFF;
		assert(exp > 0 && exp < 0x7FFF);

		// Remove the leading bit from the mantissa. This is needed because IEEE-754 represenation uses
		// the concept of hidden mantissa bit.
		mant1 <<= 1;
		if ((mant2 & 0x8000000000000000) != 0)
			mant1 |= 1;
		mant2 <<= 1;
	}

	// Setup the data fields.
	mantissa1 = mant1;
	mantissa2 = (DWORD)(mant2 >> 32);
	mantissa3 = (WORD)(mant2 >> 16);
	exponent = exp;

	return(ltx_floating_point);
}

// Static method.
const wchar_t *TLexema::GetLexTypeName(TLexemaType lext, bool want_first_cap)
{
	if (lext < 0 || lext >= ltx_num_lexema_types)
		return(L"Bogus lex type value");

	return((want_first_cap == TRUE) ? g_Lexema_LexTypeNames1[lext] : g_Lexema_LexTypeNames2[lext]);
}

// Static method.
const wchar_t *TLexema::GetKeywordTypeName(TLexKeywordType kwdt, bool want_first_cap)
{
	if (kwdt < 0 || kwdt >= ltkn_num_keyword_types)
		return(L"Bogus lex type value");

	return((want_first_cap == TRUE) ? g_Lexema_KeywordTypeNames1[kwdt] : g_Lexema_KeywordTypeNames2[kwdt]);
}

// Static method.
const wchar_t *TLexema::GetLexSubtypeName(TLexemaType lext, TLexSubtype subt)
{
	switch (lext)
	{
		case ltx_comment:
				{
					if (subt.comment_type < 0 || subt.comment_type >= lct_num_comment_types)
						break;
					return(g_Lexema_CommentTypeNames[subt.comment_type]);
				}

		case ltx_number:
				{
					if (subt.number_type < 0 || subt.number_type >= lnt_num_number_types)
						break;
					return(g_Lexema_NumberTypeNames[subt.number_type]);
				}

		case ltx_floating_point:
				{
					if (subt.floating_point_type < 0 || subt.floating_point_type >= lfp_num_floating_point_types)
						break;
					return(g_Lexema_FloatingPointTypeNames[subt.floating_point_type]);
				}

		case ltx_charconst:
				{
					if (subt.charconst_type < 0 || subt.charconst_type >= lchct_num_charconst_types)
						break;
					return(g_Lexema_CharConstTypeNames[subt.charconst_type]);
				}

		case ltx_string:
				{
					if (subt.string_type < 0 || subt.string_type >= lstrt_num_string_types)
						break;
					return(g_Lexema_StringTypeNames[subt.string_type]);
				}

		case ltx_keyword:
				{
					if (subt.keyword_type < 0 || subt.keyword_type >= ltkn_num_keyword_types)
						break;
					return(g_Lexema_KeywordTypeNames1[subt.keyword_type]);
				}

		case ltx_eol:
				{
					if (subt.endofline_type < 0 || subt.endofline_type >= leolt_num_eol_types)
						break;
					return(g_Lexema_EndOfLineTypeNames[subt.endofline_type]);
				}

		case ltx_error:
				{
					if (subt.error_class < 0 || subt.error_class >= lerrc_num_errorclass_types)
						break;
					return(g_Lexema_ErrorClassNames[subt.error_class]);
				}

		default:
			{
				return(L"n/a");
			}
	}

	return(L"Bogus subtype value");
}

//  - - - - - - - - Enum names handling - - - - - - - - - -

const wchar_t *TLexema::GetLexTypeEnumName(TLexemaType lext)
{
	if (lext < 0 || lext >= ltx_num_lexema_types)
		return(L"BogusLexType");

	return(g_Lexema_LexTypeEnumNames[lext]);
}

const wchar_t *TLexema::GetLexSubtypeEnumName(TLexemaType lext, TLexSubtype subt)
{
	switch (lext)
	{
		case ltx_comment:		return(GetLexCommentTypeEnumName(subt.comment_type));
		case ltx_number:			return(GetLexNumberTypeEnumName(subt.number_type));
		case ltx_floating_point:		return(GetLexFloatingPointTypeEnumName(subt.floating_point_type));
		case ltx_charconst:		return(GetLexCharConstTypeEnumName(subt.charconst_type));
		case ltx_string:			return(GetLexStringTypeEnumName(subt.string_type));
		case ltx_keyword:			return(GetLexKeywordTypeEnumName(subt.keyword_type));
		case ltx_name:			return(L"n/a");
		case ltx_eol:				return(GetLexEndOfLineTypeEnumName(subt.endofline_type));
		case ltx_eof:				return(L"n/a");
		case ltx_error:			return(GetLexErrorClassEnumName(subt.error_class));
	}

	return(L"BogusLexType");
}

const wchar_t *TLexema::GetLexCommentTypeEnumName(TLexCommentType subt)
{
	if (subt < 0 || subt >= lct_num_comment_types)
		return(L"BogusLexCommentType");

	return(g_Lexema_CommentTypeEnumNames[subt]);
}

const wchar_t *TLexema::GetLexNumberTypeEnumName(TLexNumberType subt)
{
	if (subt < 0 || subt >= lnt_num_number_types)
		return(L"BogusLexNumberType");

	return(g_Lexema_NumberTypeEnumNames[subt]);
}

const wchar_t *TLexema::GetLexFloatingPointTypeEnumName(TLexFloatingPointType subt)
{
	if (subt < 0 || subt >= lfp_num_floating_point_types)
		return(L"BogusLexFloatingPointType");

	return(g_Lexema_FloatingPointTypeEnumNames[subt]);
}

const wchar_t *TLexema::GetLexCharConstTypeEnumName(TLexCharConstType subt)
{
	if (subt < 0 || subt >= lchct_num_charconst_types)
		return(L"BogusLexCharConstType");

	return(g_Lexema_CharConstTypeEnumNames[subt]);
}

const wchar_t *TLexema::GetLexStringTypeEnumName(TLexStringType subt)
{
	if (subt < 0 || subt >= lstrt_num_string_types)
		return(L"BogusLexStringType");

	return(g_Lexema_StringTypeEnumNames[subt]);
}

const wchar_t *TLexema::GetLexKeywordTypeEnumName(TLexKeywordType subt)
{
	if (subt < 0 || subt >= ltkn_num_keyword_types)
		return(L"BogusLexTokenType");

	return(g_Lexema_KeywordTypeEnumNames[subt]);
}

const wchar_t *TLexema::GetLexEndOfLineTypeEnumName(TLexEndOfLineType subt)
{
	if (subt < 0 || subt >= leolt_num_eol_types)
		return(L"BogusLexEndOfLineType");

	return(g_Lexema_EndOfLineTypeEnumNames[subt]);
}

const wchar_t *TLexema::GetLexErrorClassEnumName(TLexErrorClass ecls)
{
	if (ecls < 0 || ecls >= lerrc_num_errorclass_types)
		return(L"BogusLexErrorClass");

	return(g_Lexema_ErrorClassEnumNames[ecls]);
}

const wchar_t *TLexema::GetKeywordIdEnumName(int keyword_id, bool *reserved_val)
{
	// Assume that token id belongs to one of the possible ranges.
	if (reserved_val != NULL)
		*reserved_val = FALSE;

	if (keyword_id > spr_none && keyword_id < spr_max_token)
	{
		// Separator token.
		return((wchar_t*)g_Lexema_StdSeparTokenEnumNames[keyword_id-spr_none]);
	}
	else if (keyword_id > opr_none && keyword_id < opr_max_token)
	{
		// Arithmetic operation token.
		return((wchar_t*)g_Lexema_StdArithmTokenEnumNames[keyword_id-opr_none]);
	}
	else if (keyword_id > mpr_none && keyword_id < mpr_max_token)
	{
		// Preprocessor token.
		if (keyword_id <= mpr_prm_cnct)
			return((wchar_t*)g_Lexema_InlinePreProcTokenEnumNames[keyword_id-mpr_none]);

		return(GetKeywordEnumNameFromDict(g_Lexema_PreProcTokensDict, keyword_id));
	}
	else if (keyword_id > clg_none && keyword_id < clg_max_token)
	{
		// C language token.
		return(GetKeywordEnumNameFromDict(g_Lexema_CLangTokensDict, keyword_id));
	}
	else if (keyword_id > cpp_none && keyword_id < cpp_max_token)
	{
		// C++ language token.
		return(GetKeywordEnumNameFromDict(g_Lexema_CppLangTokensDict, keyword_id));
	}
	else if (keyword_id > csh_none && keyword_id < csh_max_token)
	{
		// C# language token.
		return(GetKeywordEnumNameFromDict(g_Lexema_CshLangTokensDict, keyword_id));
	}

	// Value falls out of all known ranges.
	if (reserved_val != NULL)
		*reserved_val = TRUE;
	return(L"BogusKeywordIdValue");
}

TLexemaType TLexema::GetLexTypeFromEnumName(const wchar_t *string)
{
	for (int itp=0; itp<ltx_num_lexema_types; ++itp)
	{
		if (wcscmp(g_Lexema_LexTypeEnumNames[itp], string) == 0)
			return((TLexemaType)itp);
	}

	return(ltx_num_lexema_types);
}

TLexCommentType TLexema::GetLexCommentTypeFromEnumName(const wchar_t *string)
{
	for (int i=0; i<lct_num_comment_types; ++i)
	{
		if (wcscmp(g_Lexema_CommentTypeEnumNames[i], string) == 0)
			return((TLexCommentType)i);
	}

	return(lct_num_comment_types);
}

TLexNumberType TLexema::GetLexNumberTypeFromEnumName(const wchar_t *string)
{
	for (int i=0; i<lnt_num_number_types; ++i)
	{
		if (wcscmp(g_Lexema_NumberTypeEnumNames[i], string) == 0)
			return((TLexNumberType)i);
	}

	return(lnt_num_number_types);
}

TLexFloatingPointType TLexema::GetLexFloatingPointTypeFromEnumName(const wchar_t *string)
{
	for (int i=0; i<lfp_num_floating_point_types; ++i)
	{
		if (wcscmp(g_Lexema_FloatingPointTypeEnumNames[i], string) == 0)
			return((TLexFloatingPointType)i);
	}

	return(lfp_num_floating_point_types);
}

TLexCharConstType TLexema::GetLexCharConstTypeFromEnumName(const wchar_t *string)
{
	for (int i=0; i<lchct_num_charconst_types; ++i)
	{
		if (wcscmp(g_Lexema_CharConstTypeEnumNames[i], string) == 0)
			return((TLexCharConstType)i);
	}

	return(lchct_num_charconst_types);
}

TLexStringType TLexema::GetLexStringTypeFromEnumName(const wchar_t *string)
{
	for (int i=0; i<lstrt_num_string_types; ++i)
	{
		if (wcscmp(g_Lexema_StringTypeEnumNames[i], string) == 0)
			return((TLexStringType)i);
	}

	return(lstrt_num_string_types);
}

short TLexema::GetLexKeywordIdFromEnumName(const wchar_t *string)
{
	short val;

	// Check for separator token.
	for (int i1=0; i1<spr_max_token-spr_none; ++i1)
	{
		if (wcscmp(g_Lexema_StdSeparTokenEnumNames[i1], string) == 0)
			return(i1+spr_none);
	}

	// Check for arithmetic operation token.
	for (int i2=0; i2<opr_max_token-opr_none; ++i2)
	{
		if (wcscmp(g_Lexema_StdArithmTokenEnumNames[i2], string) == 0)
			return(i2+opr_none);
	}

	// Check for inline preprocessor token.
	for (int i3=0; i3 <= mpr_prm_cnct-mpr_none; ++i3)
	{
		if (wcscmp(g_Lexema_InlinePreProcTokenEnumNames[i3], string) == 0)
			return(i3+mpr_none);
	}

	// Check for preprocessor statement token.
	val = GetKeywordIdFromDict(g_Lexema_PreProcTokensDict, string);
	if (val != 0)
		return(val);

	// Check for C language keyword.
	val = GetKeywordIdFromDict(g_Lexema_CLangTokensDict, string);
	if (val != 0)
		return(val);

	// Check for C++ language keyword.
	val = GetKeywordIdFromDict(g_Lexema_CppLangTokensDict, string);
	if (val != 0)
		return(val);

	// Check for C# language keyword.
	val = GetKeywordIdFromDict(g_Lexema_CshLangTokensDict, string);
	if (val != 0)
		return(val);

	// Value does not meet any token enum name.
	return(0);
}

TLexEndOfLineType TLexema::GetLexEndOfLineTypeFromEnumName(const wchar_t *string)
{
	for (int isubt=0; isubt<leolt_num_eol_types; ++isubt)
	{
		if (wcscmp(g_Lexema_EndOfLineTypeEnumNames[isubt], string) == 0)
			return((TLexEndOfLineType)isubt);
	}

	return(leolt_num_eol_types);
}

const wchar_t *TLexema::GetErrorClassName(TLexErrorClass error_class)
{
	if (error_class < 0 || error_class >= lerrc_num_errorclass_types)
		return(L"Bogus error_class value");

	return(g_Lexema_ErrorClassNames[error_class]);
}

const wchar_t *TLexema::GetCssStyleName(TLexemaType lext, TLexSubtype subt)
{
	if (lext == ltx_comment)
	{
		return(GetLexCommentTypeEnumName(subt.comment_type));
	}
	else if (lext == ltx_keyword)
	{
		// In this case the subtype param carries the keyword value, not the keyword type.
		if (subt.subtype < spr_max_token)
		{
			// Curved brackets, semicolon and ellipses.
			return(L"tkn_separ");
		}
		else if (subt.subtype < mpr_define)
		{
			// This range includes all arithmetic operators plus inline preprocessor tokens.
			return(L"tkn_arithm");
		}
		else if (subt.subtype < mpr_max_token)
		{
			// These are wholeline preprocessor statements.
			return(L"tkn_preproc");
		}
		else
		{
			// These are C/Cpp/Csh keywords.
			return(L"tkn_keyword");
		}
	}
	else if (lext == ltx_error)
	{
		// Error lexema is a rare thing. Nevertheless.
		return(L"ltx_raw_error");
	}

	// The rest of lexema types and subtypes should be described with their enum identifier.
	return(GetLexTypeEnumName(lext));
}

const wchar_t *TLexema::GetIncFnStringCssStyleName()
{
	// This method is used for dumping grammar defn source files into HTML.
	return(L"ltx_string_inc_fn");
}

const wchar_t *TLexema::GetArithmOperationText(int keyword_id)
{
	if (keyword_id > opr_none && keyword_id < opr_max_token)
	{
		if (keyword_id == opr_defined)
			return(L"def");
		if (keyword_id == opr_unplus)
			return(L"un+");
		if (keyword_id == opr_unminus)
			return(L"un-");

		return((wchar_t*)g_Lexema_StdArithmTokenRepr[keyword_id-opr_none]);
	}

	return(L"unkn");
}

const wchar_t *TLexema::GetKeywordText(int keyword_id)
{
	if (keyword_id > spr_none && keyword_id < spr_max_token)
	{
		// Separator token.
		return(g_Lexema_StdSeparTokenRepr[keyword_id-spr_none]);
	}
	else if (keyword_id > opr_none && keyword_id < opr_max_token)
	{
		// Arithmetic operation or arithmetic keyword.
		return(g_Lexema_StdArithmTokenRepr[keyword_id-opr_none]);
	}
	else if (keyword_id > mpr_none && keyword_id < mpr_max_token)
	{
		// Preprocessor operator or statement.
		if (keyword_id <= mpr_prm_cnct)
			return(g_Lexema_InlinePreProcTokenEnumNames[keyword_id-mpr_none]);

		return(GetKeywordNameFromDict(g_Lexema_PreProcTokensDict, keyword_id));
	}
	else if (keyword_id > clg_none && keyword_id < clg_max_token)
	{
		// C language operator or keyword.
		return(GetKeywordNameFromDict(g_Lexema_CLangTokensDict, keyword_id));
	}
	else if (keyword_id > cpp_none && keyword_id < cpp_max_token)
	{
		// C++ language operator or keyword.
		return(GetKeywordNameFromDict(g_Lexema_CppLangTokensDict, keyword_id));
	}
	else if (keyword_id > csh_none && keyword_id < csh_max_token)
	{
		// C# language operator or keyword.
		return(GetKeywordNameFromDict(g_Lexema_CshLangTokensDict, keyword_id));
	}

	// Value is out of all ranges.
	return(L"BogusTokenIdValue");
}

TLexKeywordType TLexema::GetKeywordTypeFromTokenId(int keyword_id)
{
	if (keyword_id > spr_none && keyword_id < spr_max_token)
	{
		// Separator.
		return(ltkn_separ);
	}
	else if (keyword_id > opr_none && keyword_id < opr_max_token)
	{
		// Arithmetic operation or arithmetic keyword.
		return(ltkn_arithm);
	}
	else if (keyword_id > mpr_none && keyword_id < mpr_max_token)
	{
		// Preprocessor operator or statement.
		return(ltkn_preproc);
	}
	else if (keyword_id > clg_none && keyword_id < clg_max_token)
	{
		// C language operator or keyword.
		return(ltkn_clang);
	}
	else if (keyword_id > cpp_none && keyword_id < cpp_max_token)
	{
		// C++ language operator or keyword.
		return(ltkn_cpp);
	}
	else if (keyword_id > csh_none && keyword_id < csh_max_token)
	{
		// C# language operator or keyword.
		return(ltkn_csh);
	}

	// Value is out of all defined ranges.
	return(ltkn_num_keyword_types);
}

const wchar_t *TLexema::GetKeywordNameFromDict(const TKeywordsDictEntry *dict, int keyword_id)
{
	// The binary search is not used here because the dict is not sorted plus this function is not massively used.
	while (dict->name != NULL)
	{
		if (dict->keyword_value == keyword_id)
			return(dict->name);
		dict++;
	}

	return(L"BogusTokenId");
}

const wchar_t *TLexema::GetKeywordEnumNameFromDict(const TKeywordsDictEntry *dict, int keyword_id)
{
	// The binary search is not used here because the dict is not sorted plus this function is not massively used.
	while (dict->name != NULL)
	{
		if (dict->keyword_value == keyword_id)
			return(dict->keyword_name);
		dict++;
	}

	return(L"BogusTokenId");
}

short TLexema::GetKeywordIdFromDict(const TKeywordsDictEntry *dict, const wchar_t *string)
{
	// The binary search is not used here because the dict is not sorted plus this function is not massively used.
	while (dict->name != NULL)
	{
		if (wcscmp(dict->keyword_name, string) == 0)
			return(dict->keyword_value);
		dict++;
	}

	return(0);
}

TLexCharConstType TLexema::GetCharConstTypeFromStringType(TLexStringType subt)
{
	if (subt < 0 || subt >= lstrt_num_string_types)
		return(lchct_num_charconst_types);

	// The passed value belongs to the proper range.
	return((TLexCharConstType)subt);
}

TLexStringType TLexema::GetStringTypeFromCharConstType(TLexCharConstType subt)
{
	if (subt < 0 || subt >= lchct_num_charconst_types)
		return(lstrt_num_string_types);

	// The passed value belongs to the proper range.
	return((TLexStringType)subt);
}

double TLexema::GetDoubleValueFromParts(UINT64 mantissa1_value, WORD exponent_value)
{
	// Assemble the IEEE-754 8-bytes format out of the internal IEEE-754 16-bytes format.
	union { UINT64 int64_copy; double double_copy; } double_value;

	if (exponent_value <= 0xF)
	{
		// The value of the constant is too small or it is zero. Setup the floating point zero value.
		double_value.int64_copy = 0;
	}
	else if (exponent_value <= 0x7FEF)
	{
		// The 64 bit format uses 12 bits for the sign and exponent field and 52 bits for the mantissa field.
		double_value.int64_copy = mantissa1_value >> (64-52);
		double_value.int64_copy |= ((UINT64)(exponent_value-0x3FFF+0x3FF) & 0x7FF) << 52;
	}
	else
	{
		// The value of the floating point constant is too big. Setup a positive infinity value.
		double_value.int64_copy = 0x7FF0000000000000;
	}

	return(double_value.double_copy);
}

//------------------------------------------------------------------------
//  ===================  TLexemaInfo  =========================
//------------------------------------------------------------------------

bool TLexemaInfo::Init(DWORD pars_id, TLexema &lex, TStringsDict *stringsDict)
{
	type			= lex.type;
	subtype		= lex.subtype;

	on_doubt		= lex.on_doubt;
	first_in_line	= lex.first_in_line;

	bool retValue = TRUE;

	if (type != ltx_floating_point)
	{
		num_value = lex.num_value;

		if ((type == ltx_string || type == ltx_name || type == ltx_error) && stringsDict != NULL)
		{
			// Place the sting into the dict.
			wchar_t *dict_str = stringsDict->RegisterStr(lex.str_value.m_body, lex.str_value.m_len);
			if (dict_str == NULL)
				retValue = FALSE;

			str_value = dict_str;
		}
		else
		{
			// The string value is not available or it was not requested .
			str_value = NULL;
		}
	}
	else
	{
		// Copy the floating point fields.
		mantissa1 = lex.mantissa1;
		mantissa2 = lex.mantissa2;
		mantissa3 = lex.mantissa3;
		exponent = lex.exponent;
	}

	origin.InitFromPrHeader(pars_id, lex.origin);
	return(retValue);
}

//---------------------------------------------------------------------------------
//  ======================  TRawScanner  =============================
//---------------------------------------------------------------------------------

static const wchar_t g_RawScan_StringEscapeSequences[] =
{
	L"rntbf"				// Cr, Lf, Tab, Backspace, Formfeed.
						// These sequencies are simply converted into one char.
	L"0123456789xX"		// Binary values in a numeric form.
						// Simplified implementation: one NULL char.
	L"\'\""				// Single and double quotes. Sequence is converted either
						// into a singe or into a double quote appropriately.
	L"\\"					// Backslash. The sequence is converted into a single
						// backshash character.
	L"\r\n"				// End of line with the backslash in front of it.
						// These chars (\n, \r\n) are converted into nothing.
	L"av?"				// Rare used escape chars.
};

static const wchar_t g_RawScan_SimpleSeparAndArithmTokenStartChars[] =
{
	L"()+-*/=,.[]!~%<>&^|?:{};",
};

static const TKeywordsDictEntry g_RawScan_CLangArithmKeywordsDict[] =
{
	{  DefKW(L"defined"),			ltkn_arithm,	opr_defined,		L"opr_defined"	},
	{  DefKW(L"sizeof"),			ltkn_arithm,	opr_sizeof,		L"opr_sizeof"		},
	{  DefKW(L"new"),				ltkn_arithm,	opr_new,			L"opr_new"		},
	{  DefKW(L"delete"),			ltkn_arithm,	opr_delete,		L"opr_delete"		},
	{  DefKW(L"throw"),			ltkn_arithm,	opr_throw,		L"opr_throw"		},
	{  DefKW(L"typeid"),			ltkn_arithm,	opr_typeid,		L"opr_typeid"		},
	{  DefKW(L"const_cast"),		ltkn_arithm,	opr_const_cast,	L"opr_const_cast"	},
	{  DefKW(L"dynamic_cast"),		ltkn_arithm,	opr_dyna_cast,	L"opr_dyna_cast"	},
	{  DefKW(L"reinterpret_cast"),	ltkn_arithm,	opr_rein_cast,		L"opr_rein_cast"	},
	{  DefKW(L"static_cast"),		ltkn_arithm,	opr_static_cast,	L"opr_static_cast"	},
	{  NULL, 0, 0, 0, NULL }
};

static const wchar_t g_RawScan_QuotedStringTrailers[3] =
{
	L'\'', L'\"', L'>'
};

static const TKeywordsDictEntry g_RawScan_NumberSuffixesDict[] =
{
	{  DefKW(L"i8"),		0,	lnt_s8bit,		L"lnt_s8bit"	},
	{  DefKW(L"i16"),		0,	lnt_s16bit,	L"lnt_s16bit"	},
	{  DefKW(L"i32"),		0,	lnt_s32bit,	L"lnt_s32bit"	},
	{  DefKW(L"i64"),		0,	lnt_s64bit,	L"lnt_s64bit"	},
	{  DefKW(L"i128"),		0,	lnt_s128bit,	L"lnt_s128bit"	},
	{  NULL, 0, 0, 0, NULL }
};

static TRawScannerProps g_RawScan_StdScanningProps =
{
	{
		lnt_s32bit,		// No suffix.
		lnt_s32bit,		// Single L suffix.
		lnt_s64bit,		// Double L suffix.
	},
	{
		lfp_64bit,			// No suffix.
		lfp_32bit,			// F suffix.
		lfp_128bit,		// L suffix.
	},
};

static const wchar_t g_RawScan_DivideBy2[16] =
{
	L'0', L'0', L'1', L'1', L'2', L'2', L'3', L'3', L'4', L'4', L'x', L'x', L'x', L'x', L'x', L'x',
};

static const wchar_t g_RawScan_MultiplyBy2[16] =
{
	L'0', L'2', L'4', L'6', L'8', L'0', L'2', L'4', L'6', L'8', L'x', L'x', L'x', L'x', L'x', L'x',
};

TRawScanner::TRawScanner(TStringsDict *stringsDict, TRawScannerProps *props)
{
	// Verify the error code values from the local enum.
	assert(rscn_err_base == perb_raw_scan_base);
	assert(rscn_err_max <= perb_raw_scan_max);

	src = NULL;
	buff_ptr = NULL;
	generated_strings = stringsDict;

	if (props != NULL)
		scanner_props = *props;
	else scanner_props = g_RawScan_StdScanningProps;
}

TRawScanner::TRawScanner(TFileInfo *srcFile, TStringsDict *stringsDict, TRawScannerProps *props)
{
	// Verify the error code values from the local enum.
	assert(rscn_err_base == perb_raw_scan_base);
	assert(rscn_err_max <= perb_raw_scan_max);

	generated_strings = NULL;
	Setup(srcFile, stringsDict);

	if (props != NULL)
		scanner_props = *props;
	else scanner_props = g_RawScan_StdScanningProps;
}

void TRawScanner::Setup(TFileInfo *src_file, TStringsDict *strings_dict)
{
	assert(src_file != NULL);
	assert(src_file->info.file_len >= 0);

	src = src_file;
	csh_mode = (src->rscn_type == rscn_csh) ? TRUE : FALSE;
	buff_ptr = src_file->info.file_body;
	buff_length = src_file->info.file_len;

	if (buff_length != 0)
	{
		// Probe the input file a bit to avoid unexpected JPFs later.
		assert(buff_ptr != NULL);
		wchar_t ch1 = buff_ptr[0];
		wchar_t ch2 = buff_ptr[buff_length-1];
		wchar_t ch3 = ch1+ch2;					// This variable and computation are needed to confuse the optimiser.
	}

	offs_beg = offs = 0;
	num_scanned = 0;
	new_line_stt = TRUE;							// The beginning of the file is treated as the beginning of the line.
	inc_fname_mode = FALSE;

	if (strings_dict != NULL)
		generated_strings = strings_dict;
	assert(generated_strings != NULL);
}

bool TRawScanner::Seek(long new_scan_pos, long len_to_scan, bool new_scan_pos_new_line_state, bool new_scan_pos_inc_fname_state)
{
	// Check that new scanning area belongs to the file.
	if (new_scan_pos < 0 || new_scan_pos > src->info.file_len)
		return(FALSE);
	if (len_to_scan < 0)
		len_to_scan = src->info.file_len-new_scan_pos;
	if (new_scan_pos+len_to_scan > src->info.file_len)
		return(FALSE);

	// Accept the parameters.
	buff_length = new_scan_pos+len_to_scan;
	offs_beg = offs = new_scan_pos;

	num_scanned = 0;
	new_line_stt = new_scan_pos_new_line_state;
	inc_fname_mode = new_scan_pos_inc_fname_state;

	return(TRUE);
}

bool TRawScanner::SeekToLexBeg(TLexema &lex, bool inc_fn_mode)
{
	// Ensure that passed lexema was taken from the current file.
	assert(lex.origin.src_area.file_info == src);
	if (lex.origin.src_area.area_beg < offs_beg || lex.origin.src_area.area_beg > buff_length)
		return(FALSE);

	// Accept the data.
	offs = lex.origin.src_area.area_beg;
	new_line_stt = lex.first_in_line;
	inc_fname_mode = inc_fn_mode;
	return(TRUE);
}

void TRawScanner::SkipSpaces()
{
	while (offs < buff_length)
	{
		wchar_t ch = buff_ptr[offs];
		if (ch == L' ' || ch == L'\t' || ch == L'\f' || ch == 0)
		{
			offs++;
			continue;
		}

		break;
	}
}

TLexemaType TRawScanner::GetLex(TLexema &lex)
{
	// Reset the previous state of the passed lexema if any.
	lex.Clear();
	lex.first_in_line = new_line_stt;
	new_line_stt = FALSE;

	// Skip empty spaces in front of the next lexema. Check for the end of line.
	while (offs < buff_length)
	{
		wchar_t ch = buff_ptr[offs];
		if (ch == L' ' || ch == L'\t' || ch == L'\f' || ch == 0)
		{
			// This is an empty character.
			offs++;
			continue;
		}

		if (ch == L'\n')
		{
			lex.SetSrcBeg(src, offs);
			offs++;

			new_line_stt = TRUE;
			return(lex.SetEndOfLine(leolt_normal));
		}
		else if (ch == L'\r')
		{
			lex.SetSrcBeg(src, offs);
			offs++;

			if (offs < buff_length && buff_ptr[offs] == L'\n')
				offs++;

			new_line_stt = TRUE;
			return(lex.SetEndOfLine(leolt_normal));
		}

		if (ch == L'\\' && offs < buff_length-1)
		{
			// Check, if this is a backslash before the end of the line. If this is so, it should be given out
			// as a new line lexema with the special flag.
			if (buff_ptr[offs+1] == L'\n')
			{
				lex.SetSrcBeg(src, offs);
				offs += 2;

				new_line_stt = TRUE;
				return(lex.SetEndOfLine(leolt_withbksl));
			}
			else if (buff_ptr[offs+1] == L'\r')
			{
				lex.SetSrcBeg(src, offs);
				offs += 2;

				if (offs < buff_length && buff_ptr[offs] == L'\n')
					offs++;

				new_line_stt = TRUE;
				return(lex.SetEndOfLine(leolt_withbksl));
			}
		}

		break;
	}

	// Scanning came either to an end of the file or to the beginning of some non empty lexema.
	// Set the current scanning position as the beginning of the new lexema.
	num_scanned++;
	lex.SetSrcBeg(src, offs);

	if (offs >= buff_length)
	{
		// Nothing was found before the end of the file.
		return(lex.SetEndOfFileLex());
	}

	// Some non empty character is present in the file. Advance the count of lexemas now because
	// current non empty char will result in some lexema in this or that way.
	wchar_t ch = buff_ptr[offs++];

	//
	// Block -1a-
	//
	if (ch == L'/' && offs < buff_length && buff_ptr[offs] == L'*')
	{
		// This is a C style comment.
		offs++;
		bool comment_complete = FALSE;
		while (offs < buff_length)
		{
			if (buff_ptr[offs] == L'*' && offs < buff_length-1 && buff_ptr[offs+1] == L'/')
			{
				offs += 2;
				comment_complete = TRUE;
				break;
			}

			offs++;
		}

		if (comment_complete == TRUE)
		{
			lex.SetSrcLen(offs);
			return(lex.SetComment(lct_c_style));
		}
		else
		{
			// End of file inside the C style comment.
			lex.SetSrcLen(offs);
			return(SetRawScanError(lex, rscn_err_eof_in_c_comment));
		}
	}

	//
	// Block -1b-
	//
	if (ch == L'/' && offs < buff_length && buff_ptr[offs] == L'/')
	{
		// This is a C++ or a C# style comment.
		offs++;
		TLexCommentType subt = (lex.first_in_line == TRUE) ? lct_cpp_wholeline : lct_cpp_endofline;

		if (offs < buff_length && buff_ptr[offs] == L'/' && csh_mode == TRUE)
		{
			// There are three slashes in a row.
			subt = lct_csh_trislash;
			offs++;
		}

		long pnt_body_beg = offs;
		while (offs < buff_length)
		{
			if (buff_ptr[offs] == L'\n' || buff_ptr[offs] == L'\r' && offs < buff_length-1 && buff_ptr[offs+1] == L'\n')
				break;
			offs++;
		}

		if (offs > pnt_body_beg && buff_ptr[offs-1] == L'\\')
		{
			// Do not include the backslash into the body of the comment.
			offs--;
		}

		lex.SetSrcLen(offs);
		return(lex.SetComment(subt));
	}

	bool unicode_flag = FALSE;
	bool process_escape_chars = TRUE;

	//
	// Block -2a-
	//
	if (ch == L'L' && offs < buff_length && inc_fname_mode == FALSE)
	{
		// This can be a beginning of the UNICODE string or the UNICODE char constant.
		wchar_t ch2 = buff_ptr[offs];
		if (ch2 == L'\'' || ch2 == L'\"')
		{
			offs++;
			ch = ch2;
			unicode_flag = TRUE;
		}
	}

	//
	// Block -2b-
	//
	if (ch == L'@' && offs < buff_length && csh_mode == TRUE)
	{
		// This can be a beginning of the C# verbatim string or the C# verbatim char constant.
		wchar_t ch2 = buff_ptr[offs];
		if (ch2 == L'\'' || ch2 == L'\"')
		{
			offs++;
			ch = ch2;
			process_escape_chars = FALSE;
		}
	}

	//
	// Block -2c-
	//
	if (ch == L'\'')
	{
		// This is a beginning of a single quoted string.
		ScanQuotedString(lex, qsd_single, process_escape_chars, unicode_flag);
		if (lex.type != ltx_string)
			return(lex.type);

		if (lex.str_value.m_len > 4)
		{
			// Character constant contains too many characters.
			return(SetRawScanError(lex, rscn_err_charconst_overflow));
		}

		// Convert string into the numeric value.
		UINT64 val = 0;
		for (int inx=0; inx<lex.str_value.m_len; ++inx)
		{
			val <<= 16;
			val |= lex.str_value.m_body[inx];
		}

		lex.str_value.Clear();
		return(lex.SetCharConst((unicode_flag == TRUE) ? ((process_escape_chars == TRUE) ? lchct_unicode : lchct_csh_verb) : lchct_ascii, val));
	}

	//
	// Block -2d-
	//
	if (ch == L'\"')
	{
		// This is a double quoted string.
		return(ScanQuotedString(lex, qsd_double, (inc_fname_mode == TRUE) ? FALSE : TRUE, unicode_flag));
	}
	else if (ch == L'<' && inc_fname_mode == TRUE)
	{
		// This is a specail case of <sys/stat.h> like file name.
		return(ScanQuotedString(lex, qsd_angle, FALSE, FALSE));
	}

	//
	// Block -3a-
	//
	if (IsDigitChar(ch) == TRUE)
	{
		// Check for the floating point constant first. If the sequence of digits is followed with a dot
		// or with an exponent symbol, this is a floating point constant. Otherwisw it is an integer.
		long offs1 = offs;
		while (offs1 < buff_length)
		{
			wchar_t ch4 = buff_ptr[offs1];

			if (ch4 == L'.' || ch4 == L'e' || ch4 == L'E')
			{
				// The sequence of decimal digits is followed either by a dot or by an exponent character.
				int whole_part_beg = offs-1;
				offs = offs1;
				return(ScanFloatingPointConst(lex, whole_part_beg, offs1-whole_part_beg, ch4));
			}
			else if (IsDigitChar(ch4) == FALSE)
			{
				// This is an end of the number.
				break;
			}

			offs1++;
		}

		// This should be an integer number - oct, dec or hex.
		if (ch == L'0')
		{
			// This is an octal or a hex number. According to the C/Cpp syntax decimal number cannot
			// start with zero. Assemble the value of the number.
			if (offs < buff_length && (buff_ptr[offs] == L'x' || buff_ptr[offs] == L'X'))
			{
				// This is a HEX number. Consume the "x" character.
				offs++;
				return(ScanHexNumber(lex));
			}
			else
			{
				// This is an octal number. The leading zero can be discarded.
				return(ScanOctNumber(lex));
			}
		}
		else
		{
			// This is a decimal number. The leading digit is needed.
			return(ScanDecNumber(lex, ch));
		}
	}
	else if (ch == L'.' && offs < buff_length && IsDigitChar(buff_ptr[offs]) == TRUE)
	{
		// The dot character is followed by a digit. Unscan the dot. It will be consumed inside the method.
		// This approach makes the overall code simpler.
		offs--;
		return(ScanFloatingPointConst(lex, offs, 0, ch));
	}

	//
	// Block -4a-
	//
	if (wcschr(g_RawScan_SimpleSeparAndArithmTokenStartChars, ch) != NULL)
	{
		// This looks to be a beginnig of an arithmetic token. Preset the length of lexema to 1 char.
		lex.SetSrcLen(offs);
		switch (ch)
		{
			case L'{':	return(lex.SetKeyword(ltkn_separ, spr_lcurvbr));
			case L'}':	return(lex.SetKeyword(ltkn_separ, spr_rcurvbr));
			case L';':	return(lex.SetKeyword(ltkn_separ, spr_semicol));
			case L',':		return(lex.SetKeyword(ltkn_arithm, opr_comma));
			case L'(':	return(lex.SetKeyword(ltkn_arithm, opr_lpar));
			case L')':	return(lex.SetKeyword(ltkn_arithm, opr_rpar));

			case L'!':	if (offs < buff_length && buff_ptr[offs] == L'=')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_ne));
						}
						else
						{
							return(lex.SetKeyword(ltkn_arithm, opr_bang));
						}

			case L'~':	return(lex.SetKeyword(ltkn_arithm, opr_tilda));

			case L'+':	if (offs < buff_length && buff_ptr[offs] == L'+')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_plusplus));
						}
						else if (offs < buff_length && buff_ptr[offs] == L'=')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_plus_asgn));
						}
						else
						{
							return(lex.SetKeyword(ltkn_arithm, opr_plus));
						}

			case L'-':	if (offs < buff_length && buff_ptr[offs] == L'-')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_minusminus));
						}
						else if (offs < buff_length && buff_ptr[offs] == L'>')
						{
							if (offs+1 < buff_length && buff_ptr[offs+1] == L'*' && csh_mode == FALSE)
							{
								offs += 2;
								lex.SetSrcLen(offs);
								return(lex.SetKeyword(ltkn_arithm, opr_arrowstar));
							}
							else
							{
								lex.SetSrcLen(++offs);
								return(lex.SetKeyword(ltkn_arithm, opr_arrow));
							}
						}
						else if (offs < buff_length && buff_ptr[offs] == L'=')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_minus_asgn));
						}
						else
						{
							return(lex.SetKeyword(ltkn_arithm, opr_minus));
						}

			case L'*':	if (offs < buff_length && buff_ptr[offs] == L'=')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_mul_asgn));
						}
						else
						{
							return(lex.SetKeyword(ltkn_arithm, opr_mul));
						}

			case L'/':	if (offs < buff_length && buff_ptr[offs] == L'=')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_div_asgn));
						}
						else
						{
							return(lex.SetKeyword(ltkn_arithm, opr_div));
						}

			case L'%':	if (offs < buff_length && buff_ptr[offs] == L'=')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_rmnd_asgn));
						}
						else
						{
							return(lex.SetKeyword(ltkn_arithm, opr_rmnd));
						}

			case L'<':	if (offs < buff_length && buff_ptr[offs] == L'=')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_le));
						}
						else if (offs < buff_length && buff_ptr[offs] == L'<')
						{
							if (offs+1 < buff_length && buff_ptr[offs+1] == L'=')
							{
								offs += 2;
								lex.SetSrcLen(offs);
								return(lex.SetKeyword(ltkn_arithm, opr_lsh_asgn));
							}
							else
							{
								lex.SetSrcLen(++offs);
								return(lex.SetKeyword(ltkn_arithm, opr_bitlsh));
							}
						}
						else
						{
							return(lex.SetKeyword(ltkn_arithm, opr_lt));
						}

			case L'>':	if (offs < buff_length && buff_ptr[offs] == L'=')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_ge));
						}
						else if (offs < buff_length && buff_ptr[offs] == L'>')
						{
							if (offs+1 < buff_length && buff_ptr[offs+1] == L'=')
							{
								offs += 2;
								lex.SetSrcLen(offs);
								return(lex.SetKeyword(ltkn_arithm, opr_rsh_asgn));
							}
							else
							{
								lex.SetSrcLen(++offs);
								return(lex.SetKeyword(ltkn_arithm, opr_bitrsh));
							}
						}
						else
						{
							return(lex.SetKeyword(ltkn_arithm, opr_gt));
						}

			case L'&':	if (offs < buff_length && buff_ptr[offs] == L'&')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_logand));
						}
						else if (offs < buff_length && buff_ptr[offs] == L'=')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_btand_asgn));
						}
						else
						{
							return(lex.SetKeyword(ltkn_arithm, opr_bitand));
						}

			case L'|':	if (offs < buff_length && buff_ptr[offs] == L'|')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_logor));
						}
						else if (offs < buff_length && buff_ptr[offs] == L'=')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_btor_asgn));
						}
						else
						{
							return(lex.SetKeyword(ltkn_arithm, opr_bitor));
						}

			case L'^':	if (offs < buff_length && buff_ptr[offs] == L'=')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_btxor_asgn));
						}
						else
						{
							return(lex.SetKeyword(ltkn_arithm, opr_bitxor));
						}

			case L'=':	if (offs < buff_length && buff_ptr[offs] == L'=')
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_eq));
						}
						else
						{
							return(lex.SetKeyword(ltkn_arithm, opr_assign));
						}

			case L'?':	return(lex.SetKeyword(ltkn_arithm, opr_question));

			case L':':	if (offs < buff_length && buff_ptr[offs] == L':' && csh_mode == FALSE)
						{
							lex.SetSrcLen(++offs);
							return(lex.SetKeyword(ltkn_arithm, opr_dblcolon));
						}
						else
						{
							return(lex.SetKeyword(ltkn_arithm, opr_colon));
						}

			case L'[':	return(lex.SetKeyword(ltkn_arithm, opr_lbracket));
			case L']':	return(lex.SetKeyword(ltkn_arithm, opr_rbracket));

			case L'.':		if (offs < buff_length)
						{
							if (buff_ptr[offs] == L'*' && csh_mode == FALSE)
							{
								lex.SetSrcLen(++offs);
								return(lex.SetKeyword(ltkn_arithm, opr_dotstar));
							}
							else if (buff_ptr[offs] == L'.' && offs+1 < buff_length && buff_ptr[offs+1] == L'.')
							{
								offs += 2;
								lex.SetSrcLen(offs);
								return(lex.SetKeyword(ltkn_separ, spr_ellipsis));
							}
							else
							{
								return(lex.SetKeyword(ltkn_arithm, opr_dot));
							}
						}
						else
						{
							// Strange dot right at the end of the source file.
							return(lex.SetKeyword(ltkn_arithm, opr_dot));
						}

			default:
				{
					// This switch should find only those chars, that are available in the TSeparToken and ArithmTokenStartChars arrays.
					assert(FALSE);
				}
				break;
		}
	}
	else if (csh_mode == FALSE && CheckKeywordsDict(lex, offs-1, g_RawScan_CLangArithmKeywordsDict) == TRUE)
	{
		// This is some C/C++ arithmetic token.
		return(lex.type);
	}

	// Block -4b-
	if (ch == L'#')
	{
		if (csh_mode == FALSE)
		{
			// This is one of the C/C++ preprocessor tokens.
			if (offs < buff_length && buff_ptr[offs] == L'@')
			{
				// This is a "make char" param modifier.
				lex.SetSrcLen(++offs);
				return(lex.SetKeyword(ltkn_preproc, mpr_prm_chr));
			}

			if (offs < buff_length && buff_ptr[offs] == L'#')
			{
				// This is a "concatenate" paste buffering operator.
				lex.SetSrcLen(++offs);
				return(lex.SetKeyword(ltkn_preproc, mpr_prm_cnct));
			}

			// Start the search from the second entry in the keywords dict because the first entry can be present
			// only in the context of the macro call. The first entry is very close to the simple identifier.
			long new_offs = SkipSpacesInternal();
			if (CheckKeywordsDict(lex, new_offs, g_Lexema_PreProcTokensDict+1) == TRUE)
			{
				// This is some preprocessor statement token.
				return(lex.type);
			}

			// This character should be treated as a "make string" param modifier.
			lex.SetSrcLen(offs);
			return(lex.SetKeyword(ltkn_preproc, mpr_prm_str));
		}
		else
		{
			// Check for the C# preprocessor statement. Skip the first entry in the keywords dict
			// because it does not apply for C#.
			long new_offs = SkipSpacesInternal();
			if (CheckKeywordsDict(lex, new_offs, g_Lexema_PreProcTokensDict+1) == TRUE)
				return(lex.type);
		}
	}

	// Block -4c-
	// Check the keyword dictionaries.
	if (csh_mode == FALSE)
	{
		if (CheckKeywordsDict(lex, offs-1, g_Lexema_CLangTokensDict) == TRUE)
		{
			// This is some C language token.
			return(lex.type);
		}

		if (CheckKeywordsDict(lex, offs-1, g_Lexema_CppLangTokensDict) == TRUE)
		{
			// This is some Cpp language token .
			return(lex.type);
		}

		// Check for the "__VA_ARGS__" token. It starts with an underscore.
		short vargs_token_len = g_Lexema_PreProcTokensDict[0].name_len;
		if (ch == L'_' && offs-1+vargs_token_len <= buff_length && wcsncmp(buff_ptr+offs-1, g_Lexema_PreProcTokensDict[0].name, vargs_token_len) == 0)
		{
			short vt_len_m1 = vargs_token_len-1;
			if (offs+vt_len_m1 >= buff_length || IsNameChar(buff_ptr[offs+vt_len_m1]) == FALSE)
			{
				// This is the __VA_ARGS__ token.
				offs += vt_len_m1;
				lex.SetSrcLen(offs);
				return(lex.SetKeyword(ltkn_preproc, mpr_vargs));
			}
		}
	}
	else
	{
		if (CheckKeywordsDict(lex, offs-1, g_Lexema_CshLangTokensDict) == TRUE)
		{
			// Some C# language token was found.
			return(lex.type);
		}
	}

	// Block -5a-
	// Check for the name lexema. Current char cannot be a digit. This was checked above.
	if (IsNameChar(ch) == TRUE)
	{
		// This is some name. It is necessary to detect the end of this name.
		long pnt_beg = offs-1;
		bool inline_space = FALSE;
		while (offs < buff_length)
		{
			wchar_t ch5 = buff_ptr[offs];
			if (ch5 == 0 || ch5 == L' ' || ch5 == L'\t')
			{
				inline_space = TRUE;
				break;
			}
			else if (IsNameChar(ch5) == FALSE)
			{
				break;
			}

			offs++;
		}

		// For this scanner any name is simply a name.
		TStringPtr str(buff_ptr+pnt_beg, offs-pnt_beg);
		lex.SetSrcLen(offs);
		return(lex.SetName(str, inline_space));
	}

	// Block -6a-
	// Current character cannot start any valid lexema.
	lex.SetSrcLen(offs);
	if (ch == 0x1A)
		return(SetRawScanError(lex, rscn_err_ctrl_z_char));
	else return(SetRawScanError(lex, rscn_err_bogus_char));
}

TLexemaType TRawScanner::SniffNextLexema(short &token_code, TSourceArea *lex_beg)
{
	//
	// This method provides limited investigation of what is the next lexema. It is not affecting the state of the parser.
	//

	if (lex_beg != NULL)
	{
		lex_beg->file_info = src;
		lex_beg->area_len = 0;
	}

	long loc_pnt = offs;
	while (loc_pnt < buff_length)
	{
		wchar_t ch1 = buff_ptr[loc_pnt++];
		if (ch1 == 0 || ch1 == L' ' || ch1 == L'\t' || ch1 == L'\f')
		{
			continue;
		}

		if (lex_beg != NULL)
		{
			// Report the beginning of the place found.
			lex_beg->area_beg = loc_pnt-1;
		}

		// Check the single char combinations.
		if (ch1 == L'\n')
		{
			return(ltx_eol);
		}
		if (ch1 == L'\"')
		{
			return(ltx_string);
		}
		if (ch1 == L'(')
		{
			token_code = opr_lpar;
			return(ltx_keyword);
		}

		// The rest of the interesting combinations require at least 2 chars.
		if (loc_pnt >= buff_length)
			break;

		wchar_t ch2 = buff_ptr[loc_pnt++];
		if (ch1 == L'\r' && ch2 == L'\n')
		{
			return(ltx_eol);
		}
		if (ch1 == L'\\')
		{
			// This can be a backslash before the end of the line.
			if (ch2 == L'\n')
				return(ltx_eol);
			if (ch2 == L'\r' && loc_pnt < buff_length && buff_ptr[loc_pnt] == L'\n')
				return(ltx_eol);
		}
		if (ch1 == L'/' && ch2 == L'*')
		{
			token_code = lct_c_style;
			return(ltx_comment);
		}
		if (ch1 == L'/' && ch2 == L'/')
		{
			token_code = lct_cpp_endofline;
			return(ltx_comment);
		}

		// This is something else, many different cases are possible.
		return(ltx_empty);
	}

	// Nothing was found till the end of the source.
	if (lex_beg != NULL)
		lex_beg->area_beg = buff_length;
	return(ltx_empty);
}

void TRawScanner::GetCurrPos(TSourceArea &area)
{
	assert(src != NULL);
	area.file_info = src;
	area.area_beg = offs;
	area.area_len = 0;
}

const wchar_t *TRawScanner::GetErrorText(int err_code)
{
	switch (err_code)
	{
		case rscn_err_eof_in_c_comment:		return(L"End of file inside the C style comment.");

		case rscn_err_hex_const_incomplete:		return(L"The hex constant is incomplete.");
		case rscn_err_hex_const_overflow:		return(L"The hex constant is too long.");
		case rscn_err_oct_const_overflow:		return(L"The octal constant is too long.");
		case rscn_err_dec_const_overflow:		return(L"The decimal constant is too long.");

		case rscn_err_int_s8_overflow:			return(L"The signed 8-bit constant is too long.");
		case rscn_err_int_u8_overflow:			return(L"The unsigned 8-bit constant is too big.");
		case rscn_err_int_s16_overflow:			return(L"The signed 16-bit constant is too long.");
		case rscn_err_int_u16_overflow:			return(L"The unsigned 16-bit constant is too big.");
		case rscn_err_int_s32_overflow:			return(L"The signed 32-bit constant is too long.");
		case rscn_err_int_u32_overflow:			return(L"The unsigned 32-bit constant is too big.");
		case rscn_err_int_s64_overflow:			return(L"The signed 64-bit constant is too long.");

		case rscn_err_oom_in_floating_conv:		return(L"Out of memory while converting floating point constant into the binary form.");
		case rscn_err_exponent_incomplete:		return(L"The exponent part of the floating point constant is incomplete.");
		case rscn_err_exponent_overflow:		return(L"The value of the exponent is out of the valid range.");

		case rscn_err_expn_4byte_overflow:		return(L"The exponent is too big for the 32 bit floating point constant. The max allowed value of the binary exponent is +127.");
		case rscn_err_expn_8byte_overflow:		return(L"The exponent is too big for the 64 bit floating point constant. The max allowed value of the binary exponent is +1023.");
		case rscn_err_expn_16byte_overflow:	return(L"The exponent is too big for the 128 bit floating point constant. The max allowed value of the binary exponent is +16383.");

		case rscn_err_eol_in_charconst:			return(L"End of line inside the quoted character constant.");
		case rscn_err_eof_in_charconst:			return(L"End of file inside the quoted character constant.");
		case rscn_err_eol_in_the_string:			return(L"End of line inside the literal string.");
		case rscn_err_eof_in_the_string:			return(L"End of file inside the literal string.");

		case rscn_err_wrong_esc_seq:			return(L"Unknown or incorrect escape sequence.");
		case rscn_err_charconst_overflow:		return(L"Character constant contains too many characters.");

		case rscn_err_oom_in_string_conv:		return(L"Out of memory while converting the escape sequence.");
		case rscn_err_ctrl_z_char:				return(L"The Ctrl+Z character (old fashioned EOF) is discovered. The character is ignored.");
		case rscn_err_bogus_char:				return(L"Current character cannot start any valid lexema. The character is ignored.");
	}

	return(L"Unknown RawScan error code.");
}

TLexemaType TRawScanner::ScanQuotedString(TLexema &lex, TQuotedStringDelim delim_type, bool process_escapes, bool unicode_string)
{
	//
	// When this method is called the opening delimiter is already consumed. The file pointer points
	// to the first character inside the string.
	//
	bool need_esc_conv = FALSE;		// This flag will be set to TRUE if at least one esc sequence will be found.

	// Look for the end of the string.
	long beg_inx = offs;
	long end_inx = -1;
	long unkn_esc_inx = -1;
	for(;;)
	{
		if (offs >= buff_length)
		{
			// An end of the source file is reached.
			lex.SetSrcLen(offs);
			return(SetRawScanError(lex, (delim_type == qsd_single) ? rscn_err_eof_in_charconst : rscn_err_eof_in_the_string));
		}

		// Offset is incremented on every loop cycle at least once. This guarantees that this loop cannot be infinite.
		wchar_t ch = buff_ptr[offs++];

		if (ch == g_RawScan_QuotedStringTrailers[delim_type])
		{
			// Closing delimiter was successfully found.
			end_inx = offs-1;
			break;
		}

		if (ch == L'\n')
		{
			// End of line is inside the text string.
			offs--;
			if (buff_ptr[offs-1] == L'\r')
				offs--;

			lex.SetSrcLen(offs);
			return(SetRawScanError(lex, (delim_type == qsd_single) ? rscn_err_eol_in_charconst : rscn_err_eol_in_the_string));
		}

		if (ch == L'\\' && process_escapes == TRUE)
		{
			// Check for the prefix of the escape sequence.
			if (offs >= buff_length)
				continue;

			wchar_t ch_esc1 = buff_ptr[offs++];
			if (wcschr(g_RawScan_StringEscapeSequences, ch_esc1) != NULL)
			{
				need_esc_conv = TRUE;
				if (ch_esc1 == L'\r')
				{
					// If the next char is LF, it should be consumed also.
					if (offs >= buff_length)
						continue;

					if (buff_ptr[offs] == L'\n')
					{
						offs++;
					}
					else if (unkn_esc_inx < 0)
					{
						// Treat this as a wrong esc sequence.
						unkn_esc_inx = offs;
					}
				}
			}
			else
			{
				// This is an unknown escape sequence. Set the flag and look for the end of the string.
				// Postion of the error will be at the current point and the next scanning position will be
				// after the end of the string.
				if (unkn_esc_inx < 0)
					unkn_esc_inx = offs;
			}
		}
	}

	if (unkn_esc_inx >= 0)
	{
		// There was an unknown escape sequence somewhere in the string.
		lex.SetSrcLen(unkn_esc_inx);
		return(SetRawScanError(lex, rscn_err_wrong_esc_seq));
	}

	// An end of the string was successfully found.
	lex.SetSrcLen(offs);
	TLexStringType subt = (inc_fname_mode == TRUE && (delim_type != qsd_single)) ?
							((delim_type == qsd_angle) ? lstrt_unicode : lstrt_ascii) : ((unicode_string == TRUE) ? lstrt_unicode : lstrt_ascii);

	// Check if the escape conversion is needed or not.
	if (need_esc_conv == TRUE && delim_type != qsd_angle)
	{
		// Copy part of the source file into the TextBuffer.
		TTextBuffer80 str_buff;
		if (str_buff.Append(buff_ptr+beg_inx, end_inx-beg_inx) == FALSE)
			return(SetRawScanError(lex, rscn_err_oom_in_string_conv));

		// Do conversion right in the buffer because string can become only shorter.
		wchar_t *pnt_src = str_buff.DataPtr();
		wchar_t *pnt_dst = pnt_src;
		while (*pnt_src != 0)
		{
			wchar_t ch = *pnt_src++;
			if (ch != L'\\')
			{
				// Ordinary character.
				*pnt_dst++ = ch;
			}
			else
			{
				// Beginning of the escape sequence.
				wchar_t ch1 = *pnt_src++;
				if (ch1 == 0)
					break;

				switch (ch1)
				{
					case L'r':	*pnt_dst++ = L'\r';		break;
					case L'n':	*pnt_dst++ = L'\n';	break;
					case L't':		*pnt_dst++ = L'\t';		break;
					case L'b':	*pnt_dst++ = L'\b';	break;
					case L'f':		*pnt_dst++ = L'\f';		break;

					case L'0': case L'1': case L'2': case L'3': case L'4':
					case L'5': case L'6': case L'7': case L'8': case L'9':
					case L'x': case L'X':
						{
							// A bit simplified approach is implemented for now.
							*pnt_dst++ = L'\0';
						}
						break;

					case L'\'':	*pnt_dst++ = L'\'';		break;
					case L'\"':	*pnt_dst++ = L'\"';		break;
					case L'\\':	*pnt_dst++ = L'\\';		break;

					case L'\r':
						{
							// Cr and CrLf with the backslash is in front should disappear during
							// the escape conversion.
							if (*pnt_src == L'\n')
								pnt_src++;
						}
						break;

					case L'\n':
						{
							// Same sitation as in the case above.
						}
						break;

					case L'a':	*pnt_dst++ = L'\a';	break;
					case L'v':	*pnt_dst++ = L'\v';	break;
					case L'?':	*pnt_dst++ = L'?';		break;

					default:
						{
							// Strings with unknown escapes should be already filtered out.
							// Mimic this processing here as if there is no esc sequence.
							assert(FALSE);
							*pnt_dst++ = ch;
							*pnt_dst++ = ch1;
						}
						break;
				}
			}
		}

		// Finaly put the NULL terminator.
		*pnt_dst++ = 0;

		// The string is converted. Add it to the dict.
		pnt_src = str_buff.DataPtr();
		int str_len = (int)(INT_PTR)(pnt_dst-pnt_src);
		wchar_t *dict_str = generated_strings->RegisterStr(pnt_src, str_len);
		if (dict_str == NULL)
			return(SetRawScanError(lex, rscn_err_oom_in_string_conv));

		// Put this string into the lexema object.
		TStringPtr str(dict_str, str_len);
		return(lex.SetString(subt, str));
	}
	else
	{
		// There is no need for conversion. Instantiate this string as part of the source file.
		TStringPtr str(buff_ptr+beg_inx, end_inx-beg_inx);
		return(lex.SetString(subt, str));
	}
}

bool TRawScanner::CheckKeywordsDict(TLexema &lex, long start_offs, const TKeywordsDictEntry *dict)
{
	wchar_t ch = buff_ptr[start_offs];
	wchar_t *next_ch = buff_ptr+start_offs+1;

	while (dict->name != NULL)
	{
		int len_tail = dict->name_len-1;
		if (dict->name[0] != ch || offs+len_tail > buff_length || wcsncmp(next_ch, dict->name+1, len_tail) != 0)
		{
			// Current token description does not match the chars in the source file.
			dict++;
			continue;
		}

		// The token word is present in the source stream. It is also necessary to check that it is properly terminated.
		long new_offs = start_offs+1+len_tail;
		if (new_offs >= buff_length)
		{
			// The word stays exactly at the end of the file.
			offs = buff_length;
			lex.SetSrcLen(offs);
			lex.SetKeyword((TLexKeywordType)dict->keyword_subtype, dict->keyword_value);
			return(TRUE);
		}

		// Check the character after the name of the token.
		if (IsNameChar(buff_ptr[new_offs]) == FALSE)
		{
			// The char, that is present after the word, cannot be part of a longer name.
			offs = new_offs;
			lex.SetSrcLen(offs);
			lex.SetKeyword((TLexKeywordType)dict->keyword_subtype, dict->keyword_value);
			return(TRUE);
		}

		// Shift to the next dict record.
		dict++;
	}

	return(FALSE);
}

TLexemaType TRawScanner::ScanHexNumber(TLexema &lex)
{
	// When this method is called, the "0x" prefix is already accepted and the current buffer offset points
	// to the first meaningful digit of the number.
	if (offs >= buff_length)
	{
		// This is just a "0x" at the end of the file.
		lex.SetSrcLen(offs);
		return(SetRawScanError(lex, rscn_err_hex_const_incomplete));
	}

	bool delayed_overflow = FALSE;
	UINT64 val = 0;

	while (offs < buff_length)
	{
		wchar_t ch1 = buff_ptr[offs];
		if (ch1 >= L'0' && ch1 <= L'9')
		{
			offs++;
			if ((val & 0xF000000000000000) != 0)
			{
				// Postpone returning an overflow error.
				delayed_overflow = TRUE;
			}

			val = (val << 4) | ch1 & 0xF;
			continue;
		}
		else if (ch1 >= L'a' && ch1 <= L'f' || ch1 >= L'A' && ch1 <= L'F')
		{
			offs++;
			if ((val & 0xF000000000000000) != 0)
			{
				// Postpone returning an overflow error.
				delayed_overflow = TRUE;
			}

			val = (val << 4) | ((ch1 & 0xF) + 9);
			continue;
		}

		break;
	}

	// The value is assembled. Note that all hex and oct values are treated as unsigned.
	lex.num_value = val;
	return(ScanNumberSuffix(lex, TRUE, delayed_overflow, rscn_err_hex_const_overflow));
}

TLexemaType TRawScanner::ScanOctNumber(TLexema &lex)
{
	// When this method is called, the leading zero is already accepted and the current buffer offset points
	// to the first meaningful digit of the number.
	bool delayed_overflow = FALSE;
	UINT64 val = 0;

	while (offs < buff_length)
	{
		wchar_t ch1 = buff_ptr[offs];
		if (ch1 >= L'0' && ch1 <= L'7')
		{
			offs++;
			if ((val & 0xE000000000000000) != 0)
			{
				// Postpone returning an overflow error.
				delayed_overflow = TRUE;
			}

			val = (val << 3) | ch1 & 7;
			continue;
		}

		break;
	}

	// The value is assembled. Note that all hex and oct values are treated as unsigned.
	lex.num_value = val;
	return(ScanNumberSuffix(lex, TRUE, delayed_overflow, rscn_err_oct_const_overflow));
}

TLexemaType TRawScanner::ScanDecNumber(TLexema &lex, wchar_t first_digit)
{
	bool delayed_overflow = FALSE;
	UINT64 val = first_digit-L'0';

	// Assemble the value as an unsigned number. More exact range will be rechecked later.
	while (offs < buff_length)
	{
		wchar_t ch1 = buff_ptr[offs];
		if (IsDigitChar(ch1) == TRUE)
		{
			offs++;
			UINT64 prev_val = val;

			val = val*10 + (ch1-L'0');
			if (val < prev_val)
			{
				// Postpone returning an overflow error.
				delayed_overflow = TRUE;
			}

			continue;
		}

		break;
	}

	// Check the suffix and determine the subtype.
	lex.num_value = val;
	return(ScanNumberSuffix(lex, FALSE, delayed_overflow, rscn_err_dec_const_overflow));
}

TLexemaType TRawScanner::ScanNumberSuffix(TLexema &lex, bool unsigned_flag, bool delayed_overflow, int ovfl_error_code)
{
	// Check if there is any suffix after the digits or not.
	TLexNumberType num_subtype = lnt_num_number_types;
	bool explicit_unsigned_flag = FALSE;

	while (offs < buff_length)
	{
		wchar_t ch = buff_ptr[offs];
		if (ch == L'u' || ch == L'U')
		{
			if (explicit_unsigned_flag == TRUE)
				break;

			// This is the first unsigned flag in the sequence of suffixes.
			explicit_unsigned_flag = TRUE;
			offs++;
			continue;
		}
		if (ch == L'l' || ch == L'L')
		{
			if (num_subtype != lnt_num_number_types)
				break;

			offs++;
			if (offs < buff_length && (buff_ptr[offs] == L'l' || buff_ptr[offs] == L'L'))
			{
				// This is a double "L" suffix.
				num_subtype = scanner_props.m_number_subt[TRawScannerProps::nbss_long_long];
				offs++;
			}
			else
			{
				// This is a single "L" suffix.
				num_subtype = scanner_props.m_number_subt[TRawScannerProps::nbss_long];
			}

			continue;
		}
		if (ch == L'i')
		{
			if (num_subtype != lnt_num_number_types)
				break;

			// Iterate the dict of MS extension suffixes.
			const TKeywordsDictEntry *dict_entry = g_RawScan_NumberSuffixesDict;
			while (dict_entry->name != NULL)
			{
				int len_curr_suff = dict_entry->name_len;
				if (offs+len_curr_suff <= buff_length && wcsncmp(buff_ptr+offs, dict_entry->name, len_curr_suff) == 0)
				{
					// The suffix is recognized.
					num_subtype = (TLexNumberType)dict_entry->keyword_value;
					offs += len_curr_suff;
					break;
				}

				// Current suffix description does not match the chars in the source file.
				dict_entry++;
			}

			if (num_subtype != lnt_num_number_types)
				continue;
		}

		// None of the possible suffixes matched.
		break;
	}

	bool implicit_subtype = FALSE;
	if (num_subtype == lnt_num_number_types)
	{
		// Pick up the default size for this number constant.
		num_subtype = scanner_props.m_number_subt[TRawScannerProps::nbss_none];
		implicit_subtype = TRUE;
	}

	// Combine the implicit and explicit unsigned flags.
	unsigned_flag |= explicit_unsigned_flag;
	if (unsigned_flag == TRUE)
		num_subtype = LEX_NUM_SUBT_MAKE_UNSIGEND(num_subtype);

	// Regardless of what result will be decided later, the length of this lexema is already known.
	assert(num_subtype >= 0 && num_subtype <= lnt_num_number_types);
	lex.SetSrcLen(offs);

	if (num_subtype == lnt_s128bit || num_subtype == lnt_u128bit)
	{
		// This is not a real constant. This is more a walk around to avoid generation of the syntax error.
		return(lex.SetNumberSubtype(num_subtype));
	}
	else if (delayed_overflow == TRUE)
	{
		// This flag indicates that an unsigned 64 bit overflow has already happened.
		return(SetRawScanError(lex, ovfl_error_code));
	}

	bool minus_flag = FALSE;
	if (lex.origin.src_area.area_beg > offs_beg && buff_ptr[lex.origin.src_area.area_beg-1] == L'-')
		minus_flag = TRUE;

	int err_code;
	if (implicit_subtype == TRUE)
	{
		// Find the right subtype for the current value.
		for(;;)
		{
			err_code = CheckNumberValue(num_subtype, lex.num_value, minus_flag);
			if (err_code == 0)
				break;

			// The value of the number is too big. Increase the subtype.
			if (minus_flag == FALSE)
			{
				assert(num_subtype < lnt_u64bit);
				num_subtype = (TLexNumberType)(num_subtype+1);
			}
			else
			{
				if (num_subtype >= lnt_s64bit)
					break;
				num_subtype = (TLexNumberType)(num_subtype+2);
			}
		}
	}
	else
	{
		// Check the value of the constant against its explicit subtype.
		err_code = CheckNumberValue(num_subtype, lex.num_value, minus_flag);
	}

	if (err_code != 0)
		return(SetRawScanError(lex, err_code));

	// The constraints on the value are satisfied.
	return(lex.SetNumberSubtype(num_subtype));
}

int TRawScanner::CheckNumberValue(TLexNumberType num_subtype, UINT64 num_value, bool minus_flag)
{
	switch (num_subtype)
	{
		case lnt_s8bit:	if (num_value > 0x80 || num_value == 0x80 && minus_flag == FALSE)
							return(rscn_err_int_s8_overflow);
						break;

		case lnt_u8bit:	if (num_value >= 0x100)
							return(rscn_err_int_u8_overflow);
						break;

		case lnt_s16bit:	if (num_value > 0x8000 || num_value == 0x8000 && minus_flag == FALSE)
							return(rscn_err_int_s16_overflow);
						break;

		case lnt_u16bit:	if (num_value >= 0x10000)
							return(rscn_err_int_u16_overflow);
						break;

		case lnt_s32bit:	if (num_value > 0x80000000 || num_value == 0x80000000 && minus_flag == FALSE)
							return(rscn_err_int_s32_overflow);
						break;

		case lnt_u32bit:	if (num_value >= 0x100000000)
							return(rscn_err_int_u32_overflow);
						break;

		case lnt_s64bit:	if (num_value > 0x8000000000000000 || num_value == 0x8000000000000000 && minus_flag == FALSE)
							return(rscn_err_int_s64_overflow);
						break;

		case lnt_u64bit:	// Any value is fine for this subtype of the number.
						break;

		default:
			assert(FALSE);
	}

	// Passed value is fine for the selected number subtype.
	return(0);
}

TLexemaType TRawScanner::ScanFloatingPointConst(TLexema &lex, long whole_part_offs, long whole_part_len, wchar_t char_after_the_whole_part)
{
	// When this method is called, the char that stays after the optional whole part is not accepted yet.
	wchar_t char_after = char_after_the_whole_part;

	// Setup the value variables that describe a constant equal to zero. Note that the floating point
	// constant is always a positive value or it is equal to zero.
	int exponent = 0;
	UINT64 mant1 = 0, mant2 = 0;

	// Check for the whole part.
	if (whole_part_len > 0)
	{
		// The whole part of the number is not empty.
		assert(whole_part_offs < offs);

		// Strip the leading zeroes if any. This is needed to figure out if the whole part is zero or not.
		while (whole_part_len > 0)
		{
			if (buff_ptr[whole_part_offs] != L'0')
				break;

			whole_part_offs++;
			whole_part_len--;
		}

		if (whole_part_len > 0)
		{
			// The whole part is not equal to zero. Copy it to the local buffer.
			wchar_t *fpb = floating_point_conv_buffer;
			if (whole_part_len > sizeof(floating_point_conv_buffer)/sizeof(wchar_t))
			{
				fpb = (wchar_t*)malloc(whole_part_len*sizeof(wchar_t));
				if (fpb == NULL)
				{
					lex.SetSrcLen(offs);
					return(SetRawScanError(lex, rscn_err_oom_in_floating_conv));
				}
			}

			// The allocation succeeded or it was not needed.
			wcsncpy(fpb, buff_ptr+whole_part_offs, whole_part_len);

			long value_beg_offs = 0;
			while (value_beg_offs < whole_part_len)
			{
				// Shift the mantissa to the right and increase the exponent. Note that the least significant
				// binary digit will be lost. This is not a problem here.
				mant2 >>= 1;
				if ((mant1 & 1) != 0)
					mant2 |= 0x8000000000000000;

				mant1 >>= 1;
				exponent++;

				if (exponent < 0)
				{
					if (fpb != floating_point_conv_buffer)
						free(fpb);

					lex.SetSrcLen(offs);
					return(SetRawScanError(lex, rscn_err_exponent_overflow));
				}

				// Generate the next binary digit and add it to the mantissa.
				bool remainder = DivideFloatingPointBufferBy2(fpb, whole_part_len, value_beg_offs);
				if (remainder == TRUE)
					mant1 |= 0x8000000000000000;
			}

			if (fpb != floating_point_conv_buffer)
				free(fpb);
		}
	}

	//
	// After scanning the whole part of the number either the exponent and both mantissas are all zeroes
	// or the exponent contains a positive value and the MSB of the mantissa is set.
	//

	// Check for the fraction part.
	if (char_after == L'.')
	{
		// The fraction is present. Accept the dot.
		offs++;

		// Check for the sequence of decimal digits after the dot.
		long fract_part_offs = offs;
		while (offs < buff_length)
		{
			if (IsDigitChar(buff_ptr[offs]) == FALSE)
				break;
			offs++;
		}

		if (fract_part_offs < offs)
		{
			// The fraction part of the number is not empty.
			long fract_part_len = offs-fract_part_offs;

			// Strip the trailing zeroes if any. This is primarily needed to figure out if the fraction part is zero or not.
			long last_digit_offs = offs-1;
			while (fract_part_len > 0)
			{
				if (buff_ptr[last_digit_offs] != L'0')
					break;

				last_digit_offs--;
				fract_part_len--;
			}

			if (fract_part_len > 0)
			{
				// The fraction part is not zero. Copy it to the local buffer.
				wchar_t *fpb = floating_point_conv_buffer;
				if (fract_part_len > sizeof(floating_point_conv_buffer)/sizeof(wchar_t))
				{
					fpb = (wchar_t*)malloc(fract_part_len*sizeof(wchar_t));
					if (fpb == NULL)
					{
						lex.SetSrcLen(offs);
						return(SetRawScanError(lex, rscn_err_oom_in_floating_conv));
					}
				}

				// The allocation succeeded or it was not needed.
				wcsncpy(fpb, buff_ptr+fract_part_offs, fract_part_len);

				int bits_in_mantissa = exponent;
				assert(bits_in_mantissa >= 0);

				if (exponent == 0)
				{
					// The whole part of the value is zero. This means that the most significant bit in the mantissa its NOT set.
					// Run the procedure that will convert the zero leading bits of the fraction into the negative exponent.
					assert(mant1 == 0 && mant2 == 0);

					while (MultiptyFloatingPointBufferBy2(fpb, fract_part_len) == FALSE)
					{
						exponent--;
						if (exponent >= 0)
						{
							// The value became too small. Convert the value into zero and break out of the loop.
							exponent = 0;
							bits_in_mantissa = 128;
							break;
						}
					}

					if (bits_in_mantissa != 128)
					{
						// The exponent underflow has not happened.
						mant1 = 0x8000000000000000;
						bits_in_mantissa = 1;
					}
				}

				// Pick up bits from the fraction part and set them into the mantissa. Note that exponent value is not changing
				// during this process. If the fraction part is too long, the process will stop when there will be no more room
				// in the mantissa. Remaining part of the fraction will be simply discarded.

				if (bits_in_mantissa < 64)
				{
					UINT64 mask1 = 0x8000000000000000 >> bits_in_mantissa;
					while (bits_in_mantissa < 64 && fract_part_len > 0)
					{
						bool carry_bit = MultiptyFloatingPointBufferBy2(fpb, fract_part_len);
						if (carry_bit == TRUE)
							mant1 |= mask1;

						mask1 >>= 1;
						bits_in_mantissa++;
					}
				}

				if (bits_in_mantissa >= 64 && bits_in_mantissa < 128)
				{
					UINT64 mask2 = 0x8000000000000000 >> (bits_in_mantissa-64);
					while (bits_in_mantissa < 128 && fract_part_len > 0)
					{
						bool carry_bit = MultiptyFloatingPointBufferBy2(fpb, fract_part_len);
						if (carry_bit == TRUE)
							mant2 |= mask2;

						mask2 >>= 1;
						bits_in_mantissa++;
					}
				}

				if (fpb != floating_point_conv_buffer)
					free(fpb);
			}
		}

		char_after = (offs < buff_length) ? buff_ptr[offs] : 0;
	}

	//
	// After scanning the fraction part of the number either the exponent and both mantissas are all zeroes
	// or the exponent contains any value (negative, zero, positive) and the MSB of the mantissa is set.
	//

	// Check for the exponent part.
	int dec_exponent_value = 0;
	bool negative_dec_exponent = FALSE;
	if (char_after == L'e' || char_after == L'E')
	{
		// The exponent character is present. Accept the char.
		offs++;

		if (offs >= buff_length)
		{
			lex.SetSrcLen(offs);
			return(SetRawScanError(lex, rscn_err_exponent_incomplete));
		}

		// Check for the explicit sign of the exponent.
		char_after = buff_ptr[offs];
		if (char_after == L'+')
		{
			offs++;
		}
		else if (char_after == L'-')
		{
			negative_dec_exponent = TRUE;
			offs++;
		}

		// Scan the digits of the exponent. At least one digit should be present.
		bool exp_digits_present = FALSE;
		while (offs < buff_length)
		{
			wchar_t ch = buff_ptr[offs];
			if (IsDigitChar(ch) == FALSE)
				break;

			dec_exponent_value = dec_exponent_value*10 + (ch-L'0');
			if (dec_exponent_value < 0)
			{
				lex.SetSrcLen(offs);
				return(SetRawScanError(lex, rscn_err_exponent_overflow));
			}

			exp_digits_present = TRUE;
			offs++;
		}

		if (exp_digits_present == FALSE)
		{
			lex.SetSrcLen(offs);
			return(SetRawScanError(lex, rscn_err_exponent_incomplete));
		}

		char_after = (offs < buff_length) ? buff_ptr[offs] : 0;
	}

	// Apply decimal exponent to the mantissa.
	if (dec_exponent_value != 0 && mant1 != 0)
	{
		// The value of the whole and fraction parts is not equal to zero.
	}

	// Process the suffix that may be present after the number.
	TLexFloatingPointType fpt = scanner_props.m_floating_point_subt[TRawScannerProps::fpss_none];
	if (char_after == L'f' || char_after == L'F')
	{
		fpt = scanner_props.m_floating_point_subt[TRawScannerProps::fpss_float];
		offs++;
	}
	else if (char_after == L'l' || char_after == L'L')
	{
		fpt = scanner_props.m_floating_point_subt[TRawScannerProps::fpss_long];
		offs++;
	}

	// Verify resulting exponent against the selected format.
	// Testing shows that MS compiler silently converts small values to positive zero.
	lex.SetSrcLen(offs);
	switch (fpt)
	{
		case lfp_32bit:
				{
					if (exponent < -126+1)
					{
						// Convert this small value into zero.
						exponent = 0;
						mant1 = mant2 = 0;
					}
					else if (exponent > 127+1)
					{
						// The value is out of fange.
						return(SetRawScanError(lex, rscn_err_expn_4byte_overflow));
					}
				}
				break;

		case lfp_64bit:
				{
					if (exponent < -1022+1)
					{
						// Convert this small value into zero.
						exponent = 0;
						mant1 = mant2 = 0;
					}
					else if (exponent > 1023+1)
					{
						// The value is out of fange.
						return(SetRawScanError(lex, rscn_err_expn_8byte_overflow));
					}

				}
				break;

		case lfp_128bit:
				{
					if (exponent < -16382+1)
					{
						// Convert this small value into zero.
						exponent = 0;
						mant1 = mant2 = 0;
					}
					else if (exponent > 16383+1)
					{
						// The value is out of fange.
						return(SetRawScanError(lex, rscn_err_expn_16byte_overflow));
					}
				}
				break;

		default:
			assert(FALSE);
	}

	// Setup the lexema value and return success.
	return(lex.SetFloatingPoint(fpt, exponent, mant1, mant2));
}

bool TRawScanner::DivideFloatingPointBufferBy2(wchar_t *fpb, long fpb_len, long &value_beg_offs)
{
	long curr_digit_offs = fpb_len-1;
	assert(curr_digit_offs >= 0 && curr_digit_offs >= value_beg_offs);

	// Process the last digit of the number separately.
	wchar_t ch1 = fpb[curr_digit_offs];
	fpb[curr_digit_offs--] = g_RawScan_DivideBy2[ch1 & 15];
	bool remainder = ((ch1 & 1) != 0) ? TRUE : FALSE;

	// Process other digits if any.
	while (curr_digit_offs >= value_beg_offs)
	{
		wchar_t ch2 = fpb[curr_digit_offs];
		if ((ch2 & 1) != 0)
			fpb[curr_digit_offs+1] += 5;

		fpb[curr_digit_offs--] = g_RawScan_DivideBy2[ch2 & 15];
	}

	// Strip the leading zeroes that may have been generated with the loop above.
	while (value_beg_offs < fpb_len)
	{
		if (fpb[value_beg_offs] != L'0')
			break;
		value_beg_offs++;
	}

	return(remainder);
}

bool TRawScanner::MultiptyFloatingPointBufferBy2(wchar_t *fpb, long &fpb_len)
{
	long curr_digit_offs = fpb_len-1;
	assert(curr_digit_offs >= 0);

	// Process digits starting fom the last one.
	wchar_t carry_bit = 0;
	while (curr_digit_offs >= 0)
	{
		wchar_t ch = fpb[curr_digit_offs];
		fpb[curr_digit_offs--] = g_RawScan_MultiplyBy2[ch & 15] + carry_bit;
		carry_bit = (ch >= L'5') ? TRUE : FALSE;
	}

	// Strip the trailing zeroes that may have been generated with the loop above.
	while (fpb_len > 0)
	{
		if (fpb[fpb_len-1] != L'0')
			break;
		fpb_len--;
	}

	return((carry_bit == 1) ? TRUE : FALSE);
}

long TRawScanner::SkipSpacesInternal()
{
	long offset = offs;
	while (offset < buff_length)
	{
		wchar_t ch = buff_ptr[offset];
		if (ch == 0 || ch == L' ' || ch == L'\t' || ch == L'\f')
		{
			offset++;
			continue;
		}

		break;
	}

	return(offset);
}

TLexemaType TRawScanner::SetRawScanError(TLexema &lex, int error_code)
{
	// Prepare string that points into the static memory.
	const wchar_t *error_msg = GetErrorText(error_code);
	return(lex.SetError(lerrc_raw_scanner, error_code, error_msg));
}


