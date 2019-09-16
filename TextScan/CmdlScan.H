//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	TextScan_CmdlScan_H
#define	TextScan_CmdlScan_H

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
//         Command line parsing.
// -----------------------------------------------------------------

//
//  Possible command line lexema types.
//
enum TCmdlLexType
{
	cmdl_empty,			// Brand new lexema after the constructor.
							// The cmdl raw scanner cannot return this type of lexema.
	cmdl_comment,			// For now this can be only the C++ style commemt.
							// Comments are not allowed beween the name of the key and the key value.
	cmdl_key_param,		// Key param can be with a value and without a value.
							// To detect presence of value, call str_value.IsEmpty().
	cmdl_pos_param,		// Positional parameter. The numbering of pos params starts from 0. Param 0
							// stands for the name of the executable. The praram number is assigned by
							// the higher level, not by the raw scanner.
	cmdl_eof,
	cmdl_error,
	cmdl_num_lex_types,
};

class TCmdlLexema
{
public:

	TCmdlLexema() { type = cmdl_empty; key_name_origin.Clear(); value_origin.Clear(); }
	TCmdlLexema(TCmdlLexema &lx) : type(lx.type), num_value(lx.num_value), str_value(lx.str_value), key_name_origin(lx.key_name_origin), value_origin(lx.value_origin) { }

	void operator = (TCmdlLexema &lx)
	{
		type = lx.type; num_value = lx.num_value; str_value = lx.str_value;
		key_name_origin = lx.key_name_origin; value_origin = lx.value_origin;
	}

	bool		IsLex() const { return(type >= cmdl_key_param && type <= cmdl_eof); }
	bool		IsEqualTo(TCmdlLexema &lex) const;

	void		Clear() { type = cmdl_empty; num_value = 0; str_value.Clear(); key_name_origin.Clear(); value_origin.Clear(); }

	wchar_t	*SprintfLexema(wchar_t *buffer_at_least_120_chars, int buff_len);
						// Method puts a NULL terminated string into the buffer. The string contains short representation of the lexema.
						// This string never contains crlf characters. The return value is the first parameter - address of the buffer.

	static const wchar_t *GetLexTypeName(TCmdlLexType tp, bool want_first_cap);

protected:

	TCmdlLexType	SetComment() { type = cmdl_comment; num_value = 0; return(cmdl_comment); }
	TCmdlLexType	SetKeyParam(int key_id) { type = cmdl_key_param; num_value = key_id; str_value.Clear(); return(cmdl_key_param); }
	TCmdlLexType	SetKeyParam(int key_id, TStringPtr &key_value) { type = cmdl_key_param; num_value = key_id; str_value = key_value; return(cmdl_key_param); }
	TCmdlLexType	SetPosParam() { type = cmdl_pos_param; num_value = 0; return(cmdl_pos_param); }
	TCmdlLexType	SetEndOfFileLex() { type = cmdl_eof; return(cmdl_eof); }		// Name of this method is not "SetEndOfFile" just to make it different from existing Win32 API name.
	TCmdlLexType	SetError(int error_code, const wchar_t *error_msg) { type = cmdl_error; num_value = error_code; str_value = error_msg; return(cmdl_error); }
						// The parser/compiler can expect that after returning an error lexema the scanner moves forward for at least one position.

	void		SetKeyNameSrcInfo(TFileInfo *finfo, long lex_beg, long end_offs) { key_name_origin.src_area.file_info = finfo; key_name_origin.src_area.area_beg = lex_beg; key_name_origin.src_area.area_len = end_offs-lex_beg; }
	void		SetKeyNameSrcLen(long end_offs) { key_name_origin.src_area.area_len = end_offs-key_name_origin.src_area.area_beg; }
	void		SetValueSrcInfo(TFileInfo *finfo, long lex_beg, long end_offs) { value_origin.src_area.file_info = finfo; value_origin.src_area.area_beg = lex_beg; value_origin.src_area.area_len = end_offs-lex_beg; }
	void		SetValueSrcLen(long end_offs) { value_origin.src_area.area_len = end_offs-value_origin.src_area.area_beg; }
				// The lexema is considered to be incomplete without calling these methods. Note that raw scanner is setting only
				// the src_area fields in the origin structures. The upper layer should set the rest of the fields like linear area if needed.
public:

	//
	//  All data fields are defined as public for simplicity.
	//  Although only the raw scanner should set or modify them.
	//

	TCmdlLexType	type;

	int				num_value;			// This field is valid for key params, pos params and error lexemas. Pos params are numbered
										// using the zero based index, and the index value of zero is used for the name of the application.
	TStringPtr		str_value;

	PrEventHeader	key_name_origin;	// Location of the prefix is applicable only to key_params lexemas.
	PrEventHeader	value_origin;		// Location of the value is applicable to all non empty lexemas.

	friend class TCmdlRawScanner;
	friend class TMidLevScanner;
};

struct TCmdlLexemaInfo
{
	TCmdlLexType	type;

	int				num_value;
	wchar_t			*str_value;			// The instance of the structure is not owning the body of the string.

	TSourceLocation	key_name_origin;
	TSourceLocation	value_origin;

	void		Clear() { memset(this, 0, sizeof(TCmdlLexemaInfo)); }
	bool		Init(ID parsId, TCmdlLexema &lex, TStringsDict *stringsDict = NULL);

	void GetOrigin(TSourceLocation &dest_origin) const
	{
		if (key_name_origin.src_area.file_id == 0)
		{
			dest_origin = value_origin;
		}
		else
		{
			dest_origin = key_name_origin;
			dest_origin.lin_area.ExtendEndingTo(value_origin.lin_area);
			dest_origin.src_area.ExtendEndingTo(value_origin.src_area);
		}
	}
};

enum TCmdlKeyProp : unsigned char
{
	cmkp_none,		// The property is not allowed.
	cmkp_opt,		// The prop can be present and can be absent.
	cmkp_req,		// The property is mandatory.
};

// This definition should be used for initing the TCmdlKeysDictEntry structure with constants.
#define DefLN(s)	(short)(sizeof(s)/sizeof(wchar_t)-1), (s)

//
//  Array of dict entries ends with a record that has NULL in its key_name field.
//
//  Note: It is possible to make default processing all of unknown keys. To achieve this it is necesary to place at the end
//  of the keys dictionary an entry with the key name equal to one of the key start char and length of the name equal to 1.
//  Delim prop should not be cmkp_req and value prop should not be cmkp_none. Such record will match to any key that
//  starts from the specified key start char. No special changes were added to the key search procedure to support this
//  behavior.
//
struct TCmdlKeysDictEntry
{
	short				key_ident;			// Identifier of the key.
	short				key_name_len;		// Length of the name including prefix.
	wchar_t				*key_name;			// Name of the key should include the key
											// prefix character, like minus or slash.
	TCmdlKeyProp		delim_prop;
	TCmdlKeyProp		value_prop;
};

enum TCmdlKeyParamId
{
	cmdk_unexpected_key		= 0,
	cmdk_ignore_key			= 1,

	cmdk_analysis_tool_key		= 2,
	cmdk_include_path			= 3,
	cmdk_predefined_cond		= 4,

	cmdk_nested_cmdl_disk		= 5,
	cmdk_nested_cmdl_temp		= 6,
	cmdk_nested_src_disk		= 7,
	cmdk_nested_src_temp		= 8,

	cmdk_user_ids_base = 100,
};

// -----------------------------------------------------------------
//       Scanner for text files with the command line syntax.
// -----------------------------------------------------------------

class TCmdlRawScanner
{
public:
			TCmdlRawScanner(const wchar_t *key_start_characters = NULL, TCmdlKeysDictEntry *dict = NULL, TFileInfo *file = NULL, TStringsDict *strings_dict = NULL);

	void		SetKeyParamsInfo(const wchar_t *key_start_characters, TCmdlKeysDictEntry *dict) { key_start_chars = (wchar_t*)key_start_characters;  keys_dict = dict; }
	void		Setup(TFileInfo *src_file, TStringsDict *strings_dict) { src = src_file; buff_ptr = (src != NULL) ? src->info.file_body : NULL; generated_strings = strings_dict; Rewind(); }

	void		Rewind() { buff_length = (src != NULL) ? src->info.file_len : 0; offs = num_scanned = 0; }
	void		RewindToEof() { offs = buff_length; }

	bool		Seek(long new_scan_pos, long len_to_scan);
				// The return value is FALSE only when the passed positions are outside of the file. The length to scan can be -1.
				// This means that the scanner should scan till the end of the file.

	TCmdlLexType	GetLex(TCmdlLexema &lex);
						// This method cannot return cmdl_emty lexema. All other lexema types have lex.value_origin field properly filled.

	int		GetNumScannedLexemas() { return(num_scanned); }
				// Number of detected lexemas since the latest call to Rewind() or Seek(). End of lines and eofs are not counted in.
				// The errors are treated as lexemas. Every error lexema shifts the file pointer at least for one character.

	bool				IsEof() const { return(offs >= buff_length); }
	const TFileInfo		*GetFileInfo() const { return(src); }
	const wchar_t		*GetFileName() const { return(src->info.file_name); }
						// Simple accessors.

	void		GetCurrPos(TSourceArea &area) { assert(src != NULL); area.file_info = src; area.area_beg = offs; area.area_len = 0; }
				// Method returns an empty area at the curr position or asserts if there is no source file in the scanner.

	// The ranges of the error code values are specified in the TParsingErrorCodeBases enum.
	enum TCmdlScannerErrorCode
	{
		cscn_err_base	= 430,

		cscn_err_eof_in_string			= 431,
		cscn_err_keyval_missing			= 432,
		cscn_err_eof_in_keyval			= 433,
		cscn_err_oom_on_errgen		= 434,

		cscn_err_max	= 439,
	};

	static const wchar_t	*GetErrorText(int err_code);
							// Method returns address of some static non empty string even for unknown error codes.
							// Note that the returned string can be a format string.
protected:

	long		GetCmdlSpaceLen(long start_offs);
				//
				// Possible return values:
				//
				//		-1	-  passed offset points to the end of the file.
				//		 0	-  symbol at the passed offset is not a space symbol.
				//		val	-  the positive value is the length of the space symbols seq in chars.
				//

	long		SkipSpaces();
				// When the return value is postive, this means that the scanner is standing in front of the comment lexema.

	TCmdlKeysDictEntry *CheckKeysDict();
				// Method looks for the key right at the current position that should be not an EOF.

	bool		ScanValue(TCmdlLexema &lex);
				// This methods sets only the str_value and value_origin fields. It is not modifying any other fields.

	bool		ScanQuotedString(long &end_of_value_inx);
				// This method should be called only from ScanValue(). The return value shows if closing quote was found or not.
				// On return the offset stays either after the quote or at the end of the buffer.

	TCmdlLexType SetRawScanningError(TCmdlLexema &lex, int err_code, TStringPtr *extra_ctx_data = NULL);
					// Method fills lexema with information about error. Passed lexema should have prepared beginning
					// of the value origin. This method updates only the length of this area.

protected:

	wchar_t				*key_start_chars;
	TCmdlKeysDictEntry	*keys_dict;

	TFileInfo				*src;						// This is parameter of the constructor. Object is not owning
													// or modifying this data.
	wchar_t				*buff_ptr;					// Copy of src->file_body for fast access.
	long					buff_length;					// Length of the area that is allowed to scan. Method Seek()
													// can restrict scanning area to some part of the file.
	long					offs;						// Index into the file data. It points to
													// the first unconsidered char in the file.
	int					num_scanned;				// Number of lexemas either from the beginning of the file
													// or from the latest call to the Seek() method.

	TStringsDict			*generated_strings;
							// This scanner is  using this dict only for storing generated non constant error messages.
};

#endif	// TextScan_CmdlScan_H


