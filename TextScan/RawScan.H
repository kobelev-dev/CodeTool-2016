//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Scanning files with C-style syntax.
//

#ifndef	TextScan_RawScan_H
#define	TextScan_RawScan_H

#ifndef   Common_StringPointer_H
#include  "Common/StringPointer.H"
#endif
#ifndef   Common_HashTable_H
#include  "Common/HashTable.H"
#endif
#ifndef   TextScan_CoreTypes_H
#include  "TextScan/CoreTypes.H"
#endif

// -----------------------------------------------------------------
//         Lexema as a piece of the text file.
// -----------------------------------------------------------------

//
//  Lexema types in the C/C++ source file.
//
enum TLexemaType
{
	ltx_empty,					// Brand new lexema after the default constructor.
								// Raw scanner cannot return this type of lexema.
	ltx_comment,				// Comment is needed as a separate lexema type to allow
								// code viewers to paint them in appropriate style.
	ltx_number,					// The number is an integer number here.
	ltx_floating_point,			// This is a floating point constant.

	ltx_charconst,
	ltx_string,
	ltx_keyword,				// These are various types of operators and delimiters
								// plus all reserved keywords.
	ltx_name,					// This type of lexema is called "identifier" in many high level
								// languages.
	ltx_eol,						// End of the line.
	ltx_eof,						// End of file.
	ltx_error,					// Scanner came over some character or sequence of chars
								// that cannot be part of any lexema.
	ltx_num_lexema_types,
};

enum TLexCommentType
{
	lct_c_style,					// Single line and multiline C style comment.
	lct_cpp_endofline,
	lct_cpp_wholeline,
	lct_csh_trislash,

	lct_num_comment_types,
};

enum TLexNumberType
{
	lnt_s8bit,
	lnt_u8bit,
	lnt_s16bit,
	lnt_u16bit,
	lnt_s32bit,
	lnt_u32bit,
	lnt_s64bit,
	lnt_u64bit,
	lnt_s128bit,					// These 128 bit constants are not real constants.
	lnt_u128bit,					// There is no real support in the MS compiler for this type of consts and there is no
								// knowledge about plans to support it in future. Nevertheless some existing headers
								// do contain constants with these suffixes.
	lnt_num_number_types,
};

#define LEX_NUM_SUBT_UNSIGEND(x)  (((x) & 1) ? TRUE : FALSE)
#define LEX_NUM_SUBT_SIZE_PART(x)  ((x) & ~1)
#define LEX_NUM_SUBT_MAKE_UNSIGEND(x)  ((TLexNumberType)((x) | 1))

enum TLexFloatingPointType
{
	lfp_32bit,					// In many cases this type is called "float".
	lfp_64bit,					// In many cases this type is called "double".
	lfp_128bit,					// In many cases this type is called "long double".

	lfp_num_floating_point_types,
};

//
// Note that char const may contain several chars. To determine the number of characters
// in the constant it is necessary to examine the num_val field of the lexema.
//
enum TLexCharConstType
{
	lchct_ascii,
	lchct_unicode,
	lchct_csh_verb,

	lchct_num_charconst_types,
};

enum TLexStringType
{
	lstrt_ascii,
	lstrt_unicode,
	lstrt_csh_verb,

	lstrt_num_string_types,
};

//
// Each keyword has its own 16 bit keyword value that should not match to any other value
// across all subtypes of tokens.
//
enum TLexKeywordType
{
	ltkn_separ,
	ltkn_arithm,
	ltkn_preproc,
	ltkn_clang,
	ltkn_cpp,
	ltkn_csh,

	ltkn_num_keyword_types,
	ltkn_num_keyword_vals = 300,		// Biggest keyword Id value.
};

enum TSeparToken
{
	spr_none = 9,

	spr_lcurvbr,			// '{'
	spr_rcurvbr,			// '}'
	spr_semicol,			// ';'
	spr_ellipsis,			// '...'

	spr_max_token,
};

//
// Note: Range of numeric values from "opr_comma" to "opr_colon" is used directly in the conditional
// expressions processing for detecting whether some lexema is valid operation for the cond expr or not.
//
enum TOperatorToken
{
	opr_none = 29,

	opr_defined,													// Macroprocessor only.

	opr_comma,													// ','
	opr_lpar,			opr_rpar,								// '(' and  ')'
	opr_bang,			opr_tilda,								// '!' and '~'

	opr_unplus,			opr_unminus,							// These lexemas cannot be discoveded by the raw scanner.
	opr_deref_ptr,		opr_get_addr,							// These lexemas cannot be discoveded by the raw scanner.
	opr_arr_index,		opr_funcall,								// These lexemas cannot be discoveded by the raw scanner.

	opr_mul,			opr_div,				opr_rmnd,			// '*'   and  '/'   and '%'
	opr_plus,			opr_minus,								// '+'   and  '-'
	opr_bitlsh,			opr_bitrsh,								// '<<'  and  '>>'
	opr_bitand,			opr_bitor,			opr_bitxor,			// '&'   and  '|'   and '^'
	opr_lt,				opr_le,				opr_gt,				// '<'   and  '<='  and '>'
	opr_ge,				opr_eq,				opr_ne,				// '>='  and  '=='  and '!='
	opr_logand,			opr_logor,								// '&&'  and  '||'
	opr_question,		opr_colon,								// '?'   and  ':'

	opr_dblcolon,												// '::'
	opr_dot,				opr_arrow,								// '.'  and  '->'
	opr_dotstar,			opr_arrowstar,							// '.*' and  '->*'
	opr_lbracket,		opr_rbracket,							// '['  and  ']'

	opr_plusplus,		opr_minusminus,						// '++'  and  '--'
	opr_plpl_postfix,		opr_mnmn_postfix,						// These lexemas cannot be discoveded by the raw scanner.

	opr_sizeof,													// sizeof
	opr_new,			opr_delete,								// new, delete
	opr_new_array,		opr_delete_array,						// These lexemas cannot be discoveded by the raw scanner.
	opr_throw,			opr_typeid,								// throw, typeid
	opr_cast,													// This lexema cannot be discoveded by the raw scanner.
	opr_const_cast,		opr_dyna_cast,							// const_cast, dynamic_cast
	opr_rein_cast,		opr_static_cast,							// reinterpret_cast, static_cast

	opr_assign,													// '='
	opr_mul_asgn,		opr_div_asgn,		opr_rmnd_asgn,		// '*='  and  '/='  and '%='
	opr_plus_asgn,		opr_minus_asgn,						// '+='  and  '-='
	opr_lsh_asgn,		opr_rsh_asgn,							// '<<=' and  '>>='
	opr_btand_asgn,		opr_btor_asgn,		opr_btxor_asgn,		// '&='  and  '|='  and '^='

	opr_max_token,
};

enum TPreprocToken
{
	mpr_none = 99,

	mpr_prm_chr,		mpr_prm_str,		mpr_prm_cnct,
	mpr_vargs,

	mpr_define,			mpr_undef,			mpr_include,
	mpr_ifdef,			mpr_ifndef,			mpr_if,
	mpr_elif,			mpr_else,			mpr_endif,
	mpr_pragma,		mpr_error,
	mpr_import,			mpr_line,			mpr_using,

	mpr_warning,		mpr_region,			mpr_endregion,		// Special C# preprocessor statements.

	mpr_max_token,
};

enum TCLangToken
{
	clg_none = 129,

	clg_asm,			clg_auto,			clg_break,
	clg_case,			clg_catch,			clg_char,
	clg_const,			clg_continue,
	clg_default,			clg_do,				clg_double,
	clg_else,			clg_enum,			clg_extern,
	clg_float,			clg_for,				clg_goto,
	clg_if,				clg_int,				clg_long,
	clg_register,			clg_return,
	clg_short,			clg_signed,			clg_static,
	clg_struct,			clg_switch,
	clg_try,				clg_typedef,
	clg_union,			clg_unsigned,
	clg_void,			clg_volatile,
	clg_wchar_t,			clg_while,

	clg_max_token,
};

enum TCppToken
{
	cpp_none = 179,

	cpp_bool,
	cpp_class,			cpp_explicit,			cpp_export,
	cpp_false,			cpp_friend,
	cpp_inline,			cpp_mutable,
	cpp_namespace,		cpp_operator,
	cpp_private,			cpp_protected,		cpp_public,
	cpp_template,		cpp_this,			cpp_true,
	cpp_typename,		cpp_using,			cpp_virtual,

	cpp_max_token,
};

enum TCshToken
{
	csh_none = 219,

	csh_abstract,	csh_as,			csh_base,		csh_bool,
	csh_break,		csh_byte,		csh_case,		csh_catch,
	csh_char,		csh_checked,	csh_class,		csh_const,
	csh_continue,	csh_decimal,	csh_default,		csh_delegate,
	csh_do,			csh_double,		csh_else,		csh_enum,
	csh_event,		csh_explicit,		csh_extern,		csh_false,
	csh_finally,		csh_fixed,		csh_float,		csh_for,
	csh_foreach,	csh_goto,		csh_if,			csh_implicit,
	csh_in,			csh_int,			csh_interface,	csh_internal,
	csh_is,			csh_lock,		csh_long,		csh_namespace,
	csh_new,		csh_null,		csh_object,		csh_operator,
	csh_out,			csh_override,	csh_params,		csh_private,
	csh_protected,	csh_public,		csh_readonly,	csh_ref,
	csh_return,		csh_sbyte,		csh_sealed,		csh_short,
	csh_sizeof,		csh_stackalloc,	csh_static,		csh_string,
	csh_struct,		csh_switch,		csh_this,		csh_throw,
	csh_true,		csh_try,			csh_typeof,		csh_uint,
	csh_ulong,		csh_unchecked,	csh_unsafe,		csh_ushort,
	csh_using,		csh_virtual,		csh_void,		csh_volatile,
	csh_while,

	csh_max_token,
};

enum TLexEndOfLineType
{
	leolt_normal,					// Ordinary situation.
	leolt_withbksl,					// There was a backslash in front of the eol. The source area of this
									// lexema starts in front of the backslash and its length is zero.
	leolt_num_eol_types,
};

//
// Each class of errors has its own range of error and warning codes. Application should expect that
// all these codes cannot overlap. Class of the error should just simplify processing the type and
// origin of the error.
//
enum TLexErrorClass
{
	lerrc_message		= 0,			// This is message, like beginning of the file parsing.
	lerrc_raw_scanner	= 3,			// Raw scanner does not have warnings. It has only errors.
									// Note this can be either the cmdl or the C/C++ raw scanner error.
	lerrc_preproc_warn	= 4,
	lerrc_preproc_err	= 5,
	lerrc_syntax_warn	= 6,
	lerrc_syntax_err		= 7,

	lerrc_analysis_warn	= 8,			// Analysis or test code can generate this warning.
	lerrc_analysis_err	= 9,			// Analysis or test code can generate this error.

	lerrc_num_errorclass_types		= 10,
};

#define LEX_ERROR_CLASS_ERROR(x)		(((x) & ~1) != 0 && ((x) & 1) != 0)
#define LEX_ERROR_CLASS_WARNING(x)	(((x) & ~1) != 0 && ((x) & 1) == 0)

// Error classes as a bit mask.
typedef DWORD LERRC_TYPES_MASK;
#define LERRC_TYPE_BIT(x)	(1 << (x))
#define LERRC_ALL_TYPES		(PSET_TYPES_MASK)(-1)
#define LERRC_WARNINGS		(1 << (lerrc_preproc_warn) | 1 << (lerrc_syntax_warn) | 1 << (lerrc_analysis_warn))
#define LERRC_ERRORS		(1 << (lerrc_raw_scanner) | 1 << (lerrc_preproc_err) | 1 << (lerrc_syntax_err) | 1 << (lerrc_analysis_err))

//
//  Subtype is not defined as WORD type to avoid defining all participating enums as derived
//  from WORD. These derivations make the source code less nice in some code editors.
//
union TLexSubtype
{
	DWORD	subtype;
				// This union member is used for copying one lexema object into another. This field is defined
				// as DWORD because this is the default size for enums. Grammar based parser may store
				// the subtype of the name in this field.

	TLexCommentType			comment_type;
	TLexNumberType			number_type;
	TLexFloatingPointType		floating_point_type;
	TLexCharConstType		charconst_type;
	TLexStringType			string_type;
	TLexKeywordType			keyword_type;
	TLexEndOfLineType		endofline_type;
	TLexErrorClass			error_class;
};

// This definition is used for initing keyword dict structures with constants.
#define DefKW(s)	 (s), (BYTE)(sizeof(s)/sizeof(wchar_t)-1)

struct TKeywordsDictEntry
{
	wchar_t			*name;				// Name of the keyword as it appears in the source code. The name should be
										// zero terminated regardless of the fact that its length is stored in the next field.
	BYTE			name_len;

	BYTE			keyword_subtype;
	short			keyword_value;

	wchar_t			*keyword_name;	// Name of the enum member that describes this keyword.
};

struct TLexemaInfo;

//
// This is maybe the most important class in the whole project.
//
class TLexema
{
public:

	TLexema() { type = ltx_empty; on_doubt = FALSE; origin.src_area.file_info = NULL; }

	void operator = (TLexema &ltx)
	{
		type = ltx.type; subtype = ltx.subtype;
		on_doubt = ltx.on_doubt; first_in_line = ltx.first_in_line;

		if (ltx.type != ltx_floating_point)
		{
			num_value = ltx.num_value; str_value = ltx.str_value;
		}
		else
		{
			mantissa1 = ltx.mantissa1; mantissa2 = ltx.mantissa2; mantissa3 = ltx.mantissa3; exponent = ltx.exponent;
		}

		origin = ltx.origin;
	}

	void		Clear() { type = ltx_empty; on_doubt = FALSE; origin.src_area.file_info = NULL; }

	void		AppendTo(TTextBuffer256 &result);
	bool		IsEqualTo(TLexema &ltx);

	inline bool	IsLex() const { return(type >= ltx_comment && type <= ltx_eof); }
	inline bool	IsComment(TLexCommentType subt = lct_num_comment_types) { return(type == ltx_comment && (subt == lct_num_comment_types || comment_type == subt)); }
	inline bool	IsPreprocToken() const { return(type == ltx_keyword && keyword_type == ltkn_preproc); }
	inline bool	IsKeyword(short id) const { return(type == ltx_keyword && ((short)num_value) == id); }
	inline bool	IsName(const wchar_t *name) const { return(type == ltx_name && str_value == name); }
	inline bool	IsEol() const { return(type == ltx_eol); }
	inline bool	IsEof() const { return(type == ltx_eof); }

	float			GetFloatValue();
	double		GetDoubleValue() { assert(type == ltx_floating_point); return(GetDoubleValueFromParts(mantissa1, exponent)); }
					// These methods may return a positive infinity value.

	inline long	AreaBeg() const { return(origin.src_area.area_beg); }
	inline long	AreaEnd() const { return(origin.src_area.area_beg+origin.src_area.area_len); }

	wchar_t		*SprintfLexema(wchar_t *buffer, long buff_len);
					// Method puts short, null terminated representation of the lexema into the buffer. Generated string
					// cannot contain crlf characters. The return value is the address of the passed buffer.

	const wchar_t		*GetCssStyleName() const;
						// The CSS style name is used for highlighting syntax of the source code on the WEB pages.
public:

	//
	// All data fields are defined as public for simplicity. Although only the raw/mid lev scanner should set or modify them.
	//

	TLexemaType  type;

	union
	{
		DWORD				subtype;

		TLexCommentType		comment_type;
		TLexNumberType		number_type;
		TLexFloatingPointType	floating_point_type;
		TLexCharConstType	charconst_type;
		TLexStringType		string_type;
		TLexKeywordType		keyword_type;
		TLexEndOfLineType	endofline_type;
		TLexErrorClass		error_class;

		//
		// Lexema subtype is defined as a union member to simplify copying one lexema into another.
		// Fist field of the union is defined as DWORD because this is the default size for enums.
		//
		// Note on using subtype for name lexemas. From the raw scanner point of view all names
		// are the same. They do not have any subtype. But is is convenient to color names in the source
		// with different subtypes. Typically this is done based on definition of particular name in the source
		// program. For example variable names receive one subtype, type names receive othersubtype.
		// This allows introducing context sensitive info into the context free grammar of the language.
		// Sets of name subtypes are specific to each language. So, particular name subtypes are defined
		// in each language header. Grammar header puts restriction on the possible number of different
		// name subtypes.
		//
	};

	bool				on_doubt;
	bool				first_in_line;				// The value of this data filed is TRUE when lexema is the first lexema on the line.

		//
		//  NB1:	It is not important if the previous line has finished with an ordinary eol or with eol
		//		that has a backslash in front of it.
		//  NB2:	Spaces and tabstops that can be present in the beginning of the line do not prevent
		//		setting this flag to TRUE.
		//  NB3:	Lexema may occupy seveal lines, like a multiline C-style comment. In this case lexema,
		//		that follows the multiline lexema, does not have the "first_in_line" flag set.
		//

	union
	{
		struct
		{
			UINT64			num_value;			// Note that the num value is always a positive unsigned value or it is zero.
												// Literal integer constant cannot be negative.
			TStrPtrInfo		str_value;			// Note that this field contains pointer to a non NULL terminated string. String value
												// is used in string and name lexemas where it is not NULL terminated and in
												// error lexemas where it contains the error message. On the contrary the error
												// message is always NULL terminated.
		};

		struct
		{
			UINT64			mantissa1;			// Most important 64 bits of mantissa of the floating point constant.
			DWORD			mantissa2;			// Intermediate 32 bits of mantissa of the floating point constant.
			WORD			mantissa3;			// Least important 16 bits of mantissa of the floating point constant.
			WORD			exponent;			// Exponent value and sign of the floating point constant.
												// Altough the sign in the floating point literal is always zero.
		};

			// The floating point constants are stored accoring to the IEEE-754-2008 standard. In particular the quadruple-precision
			// (16 bytes) format is used to store all floating point values regardless of the number of the decimal digits that are present
			// in the source code.
	};

	PrEventHeader	origin;				// MidLev scanner is not assigning individual change number to each lexema that
										// is retrieved from the lex source. Although it marks every lexema with the change
										// number of the latest event.
protected:

	void				SetEmpty() { type = ltx_empty; }
						// This method should be rarely used.

	TLexemaType		SetComment(TLexCommentType subt) { type = ltx_comment; comment_type = subt; return(ltx_comment); }
						// Contents of the comment should be extracted directly from the file if it is needed.

	TLexemaType		SetNumber(TLexNumberType subt, UINT64 val) { type = ltx_number; number_type = subt; num_value = val; return(ltx_number); }
	TLexemaType		SetNumberSubtype(TLexNumberType subt) { type = ltx_number; number_type = subt; return(ltx_number); }
	TLexemaType		SetFloatingPoint(TLexFloatingPointType subt, int exp, UINT64 mant1, UINT64 mant2);
						// Zero value of the passed exponent corresponds to position of the floationg point
						// right in front of the MSB of the first mantissa.

	TLexemaType		SetCharConst(TLexCharConstType subt, UINT64 val) { type = ltx_charconst; charconst_type = subt; num_value = val; return(ltx_charconst); }
	TLexemaType		SetString(TLexStringType subt, TStringPtr &str) { type = ltx_string; string_type = subt; str_value = str; return(ltx_string); }
						// String value points directly into the source file except for the strings that contain escape seqs.

	TLexemaType		SetKeyword(TLexKeywordType subt, int keyword_id) { type = ltx_keyword; keyword_type = subt; num_value = keyword_id; return(ltx_keyword); }
	TLexemaType		SetName(TStringPtr &str, bool inline_space) { type = ltx_name; subtype = 0; num_value = inline_space; str_value = str; return(ltx_name); }
						// Name stores info on whether there is a space at is end or not. This is necessary for scanning
						// macrodefinitions to distinguish the macro with params from the macro whoes body starts
						// with the opening parenthesis.

	TLexemaType		SetEndOfLine(TLexEndOfLineType subt) { type = ltx_eol; endofline_type = subt; return(ltx_eol); }
						// Subtype in the eol lexema is important for scanning macrodefinitions. They bypass eols with
						// the backslash in front of them and stop on normal ones.

	TLexemaType		SetEndOfFileLex() { type = ltx_eof; origin.lin_area.linear_len = 0; origin.src_area.area_len = 0; return(ltx_eof); }
						// Sooner or later any scanner will come to the end of file. Name of this method is not "SetEndOfFile" just
						// to make it different from existing Win32 API name.

	TLexemaType		SetError(TLexErrorClass subt, int err_num, const wchar_t *err_msg) { type = ltx_error; error_class = subt; num_value = err_num; str_value = err_msg; return(ltx_error); }
						// Every parser/compiler can expect that after returning the error the scanner moves forward
						// for at least one position.

	void				SetSrcBeg(TFileInfo *finfo, long lex_beg) { origin.src_area.file_info = finfo; origin.src_area.area_beg = lex_beg; origin.src_area.area_len = 0; }
	void				SetSrcLen(long offs) { origin.src_area.area_len = offs-origin.src_area.area_beg; }
						// Lexema is considered to be incomplete without calling these methods.
public:

	static const wchar_t		*GetLexTypeName(TLexemaType lext, bool want_first_cap);
	static const wchar_t		*GetKeywordTypeName(TLexKeywordType kwdt, bool want_first_cap);
	static const wchar_t		*GetLexSubtypeName(TLexemaType lext, TLexSubtype subt);
								// Methods return human readable descriptions.

	static const wchar_t		*GetLexTypeEnumName(TLexemaType lext);
	static const wchar_t		*GetLexSubtypeEnumName(TLexemaType lext, TLexSubtype subt);
								// Methods return names of enum members.

	static const wchar_t		*GetLexCommentTypeEnumName(TLexCommentType subt);
	static const wchar_t		*GetLexNumberTypeEnumName(TLexNumberType subt);
	static const wchar_t		*GetLexFloatingPointTypeEnumName(TLexFloatingPointType subt);
	static const wchar_t		*GetLexCharConstTypeEnumName(TLexCharConstType subt);
	static const wchar_t		*GetLexStringTypeEnumName(TLexStringType subt);
	static const wchar_t		*GetLexKeywordTypeEnumName(TLexKeywordType subt);
	static const wchar_t		*GetLexEndOfLineTypeEnumName(TLexEndOfLineType subt);
	static const wchar_t		*GetLexErrorClassEnumName(TLexErrorClass ecls);
								// Methods return names of enum members.

	static const wchar_t		*GetKeywordIdEnumName(int keyword_id, bool *reserved_val = NULL);
								// Method returns member of an appropriate enum if the passed keyword_id is defined
								// and it returns "BogusKeywordIdValue" if the value does not belong to any range.

	static TLexemaType		GetLexTypeFromEnumName(const wchar_t *string);
	static TLexCommentType	GetLexCommentTypeFromEnumName(const wchar_t *string);
	static TLexNumberType		GetLexNumberTypeFromEnumName(const wchar_t *string);
	static TLexFloatingPointType	GetLexFloatingPointTypeFromEnumName(const wchar_t *string);
	static TLexCharConstType	GetLexCharConstTypeFromEnumName(const wchar_t *string);
	static TLexStringType		GetLexStringTypeFromEnumName(const wchar_t *string);
	static short				GetLexKeywordIdFromEnumName(const wchar_t *string);
	static TLexEndOfLineType	GetLexEndOfLineTypeFromEnumName(const wchar_t *string);

								//
								//  In case of success methods return members of their corresponding enums.
								//  In case of error they return:
								//
								//     GetLexTypeFromEnumName			-  ltx_num_lexema_types
								//     GetLexCommentTypeFromEnumName	-  lct_num_comment_types
								//     GetLexNumberTypeFromEnumName		-  lnt_num_number_types
								//     GetLexFloatingPointTypeFromEnumName	-  lfp_num_floating_point_types
								//     GetLexCharConstTypeFromEnumName	-  lchct_num_charconst_types
								//     GetLexStringTypeFromEnumName		-  lstrt_num_string_types
								//     GetLexKeywordIdFromEnumName		-  0
								//     GetLexEndOfLineTypeFromEnumName	-  leolt_num_eol_types
								//

	static const wchar_t		*GetErrorClassName(TLexErrorClass error_class);
								// Method returns 20-80 char static description of the passed error class.

	static const wchar_t		*GetCssStyleName(TLexemaType lext, TLexSubtype subt);
	static const wchar_t		*GetIncFnStringCssStyleName();
								// The return value is always a static string. It does not need any releasing.

	static const wchar_t		*GetArithmOperationText(int keyword_id);
	static const wchar_t		*GetKeywordText(int keyword_id);

	static TLexKeywordType		GetKeywordTypeFromTokenId(int keyword_id);

	static const wchar_t		*GetKeywordNameFromDict(const TKeywordsDictEntry *dict, int keyword_id);
	static const wchar_t		*GetKeywordEnumNameFromDict(const TKeywordsDictEntry *dict, int keyword_id);
	static short				GetKeywordIdFromDict(const TKeywordsDictEntry *dict, const wchar_t *string);

	static TLexCharConstType	GetCharConstTypeFromStringType(TLexStringType subt);
	static TLexStringType		GetStringTypeFromCharConstType(TLexCharConstType subt);

	static double				GetDoubleValueFromParts(UINT64 mantissa1_value, WORD exponent_value);

	friend class TRawScanner;
	friend class TMidLevScanner;
	friend class TCondExprTreeItem;
	friend class TCondExpressionScanner;
};

//
//  This structure is very close to TLexema.
//
struct TLexemaInfo
{
	TLexemaType			type;
	DWORD				subtype;				// Note that subtype is not an union here.

	bool					on_doubt;
	bool					first_in_line;

	union
	{
		struct
		{
			UINT64			num_value;			// Note that num value is a positive unsigned value.
			wchar_t			*str_value;			// This string is always NULL terminated.
												// Instance of the structure is not owning the body of the string.
		};

		struct
		{
			UINT64			mantissa1;			// Most important 64 bits of mantissa of the floating point constant.
			DWORD			mantissa2;			// Intermediate 32 bits of mantissa of the floating point constant.
			WORD			mantissa3;			// Least important 16 bits of mantissa of the floating point constant.
			WORD			exponent;			// Exponent value and sign of the floating point constant.
		};
	};

	TSourceLocation		origin;

	void		Clear() { memset(this, 0, sizeof(TLexemaInfo)); }

	bool		Init(ID pars_id, TLexema &lex, TStringsDict *stringsDict);
				// It is ok to pass NULL as the value of the strings dict ptr if the string value is not needed.

	double	GetDoubleValue() { assert(type == ltx_floating_point); return(TLexema::GetDoubleValueFromParts(mantissa1, exponent)); }
};

// -----------------------------------------------------------------
//      Scanner for text files with the C-style syntax.
// -----------------------------------------------------------------

struct TRawScanSeekInfo
{
	long			offs_beg;
	long			offs;
	int			num_scanned;
	bool			new_line_stt;
	bool			inc_fname_mode;
};

struct TRawScannerProps
{
	enum TNumberConstSizeSuffix
	{
		nbss_none,				// No suffix.
		nbss_long,				// Single l or L.
		nbss_long_long,			// Double l or L.
		nbss_num_types,
	};

	enum TFloatingPointConstSizeSuffix
	{
		fpss_none,				// No suffix.
		fpss_float,				// Character f or F.
		fpss_long,				// Character l or L.
		fpss_num_types,
	};

	TLexNumberType			m_number_subt[nbss_num_types];
								// Subtypes of the integer constants accroding to their suffix if any.
								// Data fileds should contain only the signed integer subtypes.

	TLexFloatingPointType		m_floating_point_subt[fpss_num_types];
								// Subtypes of the floating point constants accroding to their suffix if any.
};

class TRawScanner
{
public:
			TRawScanner(TStringsDict *stringsDict = NULL, TRawScannerProps *props = NULL);
			TRawScanner(TFileInfo *srcFile, TStringsDict *stringsDict, TRawScannerProps *props = NULL);

	void		Setup(TFileInfo *srcFile, TStringsDict *stringsDict);
				// Method sets up the file source and prepares for scanning the whole file. Non NULL strings dict
				// should be passed either in the ctor or in the setup method.

	void		Rewind() { buff_length = src->info.file_len; offs = 0; num_scanned = 0; new_line_stt = TRUE; inc_fname_mode = FALSE; }
	void		RewindToEof() { offs = buff_length; }

	bool		Seek(long new_scan_pos, long len_to_scan, bool new_scan_pos_new_line_stt, bool new_scan_pos_inc_fname_stt);
	void		SeekTo(TRawScanSeekInfo &info) { offs_beg = info.offs_beg; offs = info.offs; num_scanned = info.num_scanned; new_line_stt = info.new_line_stt; inc_fname_mode = info.inc_fname_mode; }
	bool		SeekToLexBeg(TLexema &lex, bool inc_fn_mode);
				// The return value is FALSE only when the passed position is outside of the file. The length
				// of the area to scan can be -1. This means "scan till the end of the file".

	void		SetSpecialFileNameMode(bool new_val) { inc_fname_mode = new_val; }
	void		SkipSpaces();

	TLexemaType  GetLex(TLexema &lex);
	TLexemaType  SniffNextLexema(short &token_code, TSourceArea *lex_beg_point = NULL);
						// Returned lexemas never have the doubt flag set. This flag is set by the mid lev scanner.

	const  TFileInfo	*GetFileInfo() const { return(src); }
	const  wchar_t		*GetFileName() const { return(src->info.file_name);  }

	inline	bool		IsEof() const { return(offs >= buff_length); }
	inline	bool		GetFirstInLineState() const { return(new_line_stt); }
	inline	bool		GetSpecialFileNameMode() const { return(inc_fname_mode); }
	inline	void		GetCurrPos(TRawScanSeekInfo &info) const { info.offs_beg = offs_beg; info.offs = offs; info.num_scanned = num_scanned; info.new_line_stt = new_line_stt; info.inc_fname_mode = inc_fname_mode; }
	inline	long		GetScanAreaEnd() const { return(buff_length); }

			void		GetCurrPos(TSourceArea &area);
						// Method returns empty area at the current position or asserts if there is no source file in the scanner.

			int		GetNumScannedLexemas() const { return(num_scanned); }
						// Number of detected lexemas since the latest call to Rewind() or Seek(). The ends of lines and eofs
						// are not counted. Errors are treated as lexemas. Every error lexema shifts the file pointer forward
						// at least for one character.

	// Ranges of the error code values are specified in the TParsingErrorCodeBases enum.
	enum TRawScannerErrorCode
	{
		rscn_err_base	= 230,

		rscn_err_eof_in_c_comment			= 231,

		rscn_err_hex_const_incomplete		= 232,			// 0x and no more digits after that.
		rscn_err_hex_const_overflow		= 233,
		rscn_err_oct_const_overflow			= 234,
		rscn_err_dec_const_overflow			= 235,

		rscn_err_int_s8_overflow			= 236,
		rscn_err_int_u8_overflow			= 237,
		rscn_err_int_s16_overflow			= 238,
		rscn_err_int_u16_overflow			= 239,
		rscn_err_int_s32_overflow			= 240,
		rscn_err_int_u32_overflow			= 241,
		rscn_err_int_s64_overflow			= 242,

		rscn_err_oom_in_floating_conv		= 243,
		rscn_err_exponent_incomplete		= 244,			// Digits are missing after the "e" symbol.
		rscn_err_exponent_overflow			= 245,

		rscn_err_expn_4byte_overflow		= 246,
		rscn_err_expn_8byte_overflow		= 247,
		rscn_err_expn_16byte_overflow		= 248,

		rscn_err_eol_in_charconst			= 250,
		rscn_err_eof_in_charconst			= 251,
		rscn_err_eol_in_the_string			= 252,
		rscn_err_eof_in_the_string			= 253,
		rscn_err_wrong_esc_seq				= 254,
		rscn_err_charconst_overflow			= 255,
		rscn_err_oom_in_string_conv		= 256,

		rscn_err_ctrl_z_char					= 260,
		rscn_err_bogus_char				= 269,

		rscn_err_max	= 289,
	};

	static const wchar_t *GetErrorText(int err_code);
						// Method returns address of some static non empty string even for unknown error codes.
						// For now the string cannot be a format string.
protected:

	enum TQuotedStringDelim { qsd_single, qsd_double, qsd_angle };

	TLexemaType		ScanQuotedString(TLexema &lex, TQuotedStringDelim closing_delim, bool proc_escapes, bool unicode_string);
	bool				CheckKeywordsDict(TLexema &lex, long start_offs, const TKeywordsDictEntry *dict);

	TLexemaType		ScanHexNumber(TLexema &lex);
	TLexemaType		ScanOctNumber(TLexema &lex);
	TLexemaType		ScanDecNumber(TLexema &lex, wchar_t first_digit);
						// All these methods are using offset that was taking place at the beginning of scanning
						// the current lexema. This offset is passed in the origin field of the passed lexema.

	TLexemaType		ScanNumberSuffix(TLexema &lex, bool unsigned_flag, bool delayed_overflow, int ovfl_error_code);
	int				CheckNumberValue(TLexNumberType num_subtype, UINT64 num_value, bool minus_flag);

	TLexemaType		ScanFloatingPointConst(TLexema &lex, long whole_part_offs, long whole_part_len, wchar_t char_after_the_whole_part);
	bool				DivideFloatingPointBufferBy2(wchar_t *fpb, long fpb_len, long &value_beg_offs);
	bool				MultiptyFloatingPointBufferBy2(wchar_t *fpb, long &fpb_len);

	long				SkipSpacesInternal();

	TLexemaType		SetRawScanError(TLexema &lex, int error_code);
						// This method fills the passed lexema with the information about the error. Note that this scanner
						// is setting only const strings. Error messages in this raw scanner are never formatted.

	inline static bool	IsDigitChar(wchar_t ch) { return(ch >= L'0' && ch <= L'9'); }
	inline static bool	IsNameChar(wchar_t ch) { return(ch >= L'A' && ch <= L'Z' || ch >= L'a' && ch <= L'z' || ch >= L'0' && ch <= L'9' || ch == L'_' || ch == L'$'); }
						// Unfortunately Microsoft headers sometimes use "$" in the names of identifiers.

protected:

	TFileInfo				*src;					// Parameter of the constructor. Object is not owning or modifying this data.
	bool					csh_mode;				// This mode is set in the Setup() method based on the rscn_field of the file.

	wchar_t				*buff_ptr;				// Copy of the src->file_body for fast access.
	long					buff_length;				// Length of the area that is allowed to scan. Method Seek() can restrict
												// the scanning area to some part of the file.
	long					offs_beg;				// Initial offset into the buffer as it was in the ctor or in the latest seek call.
	long					offs;					// Index into the file data. It points to the first unconsidered
												// character in the file buffer.
	int					num_scanned;			// Number of lexemas either from the beginnging of the file or from the latest call
												// to the methods Seek() or Rewind(). This count does not include the EOF and EOL
												// lexemas.
	bool					new_line_stt;			// The field is TRUE when the latest lexema that is already given out is ltx_eol.
												// Value of this field will be copied into the next lexema.
	bool					inc_fname_mode;		// This is a special flag that controls scanning of strings. When this flag is set,
												// strings in angle brackets, like <some_text>, are reported as unicode strings
												// plus the escape chars are not processed. Raw scanner is not setting/resetting
												// this flag. This is always done from outside.

	TStringsDict			*generated_strings;
							// This dictionary is used for storing literal strings that contain escape sequencies.
							// For now this is the only one use of this dict.

	TRawScannerProps		scanner_props;

	wchar_t				floating_point_conv_buffer[128];
							// This temp buffer is used during conversion of the floating point constants
							// into the binary representation.

	friend class TMidLevScanner;
};

#endif	// TextScan_RawScan_H


