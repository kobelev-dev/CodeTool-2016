//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	TextScan_CoreTypes_H
#define	TextScan_CoreTypes_H

#ifndef   Common_Buffers_H
#include  "Common/Buffers.H"
#endif
#ifndef   Common_DoubleLinkedList_H
#include  "Common/DoubleLinkedList.H"
#endif
#ifndef   Common_FileDataHelper_H
#include  "Common/FileDataHelper.H"
#endif

// - - - - - -  Section 1. Database object identifiers, disk and temp file identifiers  - - - - - -

typedef	DWORD		ID;
typedef	__int64		TLongId;		// This is a combination of the parsing_id (Lo DWORD) and the file_id (Hi DWORD).
									// The hi and lo DWORDs are expressed in the wrong (i.e. Intel) byte order.

//
//  TLongId and TIdStruct can have 2 different forms:
//
//     (0, file_id)		--	The file_id is the parsing independent ID of the disk file.
//     (pars_id, evt_id)	--	This is ID of the file, that has meaning only inside certain parsing session.
//						Second part of the id is event id that describes the temp file.
//

typedef THeapBasedArray<ID, 256> TParsingIdsArray;

enum { TProjectParsSessId = 0xFFFFFFFF };
			// The database of the high level language objects can have either individual parsing sesssion
			// level or the whole project level. This Id describes the project level database.

union TIdStruct
{
	struct
	{
		ID			pars_id;

		union
		{
			ID			file_id;
			ID			evt_id;
		};
	};

	TLongId			doc_id;

	inline void	Clear() { doc_id = 0; }
	inline void	Init(ID fid) { pars_id = 0; file_id = fid; }
	inline void	Init(TLongId id) { doc_id = id; }
	inline void	Init(ID pars, ID fid_evid) { pars_id = pars; file_id = fid_evid; }
};

typedef TStructsArray<TIdStruct, 8, 512> TIdStructsArray;

enum TFileStorageType
{
	strg_disk,					// This is an ordinary disk file. It is expected that contents of this file are not changing after
								// the compilation and this file may be used in other parsing sessions under the same file id.
	strg_temp,					// The temp file is expected to be generated during the compilation on the fly. The name of
								// the file is not meaningful. Most likely it will be deleted by the compliler that created it.
	strg_interm,					// This file never existed on the disk. This is only an in memory buffer. This storage type is used
								// for keeping results of the paste buff operators and bodies of the self change macros.
	strg_builtin,					// This type of file is used for storing definitions, that are defined inside the complier, like builtin
								// macros or the language keywords.
	strg_proto,					// This is a placeholder for the disk file name. This storage type is used for describing binary
								// files that are never opened and loaded. Proto files do not have body, length, date, etc.
	strg_num_types,
};

enum TRawScanningType
{
	rscn_none,					// There is no raw scanning for this type of files at all. This can be a build log, a .mc resource
								// file or any other plain text file.
	rscn_binary,					// There is no raw scanning for this type of file because this is not a text file. This file type
								// is used for creating file protos, like .obj files or other internal data structures.
	rscn_c_cpp,					// A file with the C/C++ compatible syntax. The high level language may not be the C/C++.
								// For example the grammar definition files use this raw scanning type.
	rscn_csh,					// C# syntax. (Not really implemented). The raw scanner is the same to C/C++, but it runs
								// in a different mode, that is set based on the rscn_type field in the file info struct.

	rscn_cmdl_ms_cl,			// MS C/C++ compiler command line.
	rscn_cmdl_ms_csh,			// MS C# compiler command line.
	rscn_cmdl_ms_mc,			// MS .mc compiler command line.
								// This toolkit expects that each type of the command line has its own raw scanning type.
	rscn_num_types,
};

#define  RAW_SCAN_CLANG_STYLE(x)	((x) >= rscn_c_cpp && (x) <= rscn_csh)
#define  RAW_SCAN_CMDL_STYLE(x)	((x) >= rscn_cmdl_ms_cl && (x) <= rscn_cmdl_ms_mc)

enum TParsingErrorCodeBases
{
	perb_raw_scan_base			=  230,			// C/C++ raw scanner (TRawScanner::TRawScannerErrorCode).
	perb_raw_scan_max			=  289,

	perb_cmdl_scan_base		=  430,			// Command line raw scanner (TCmdlRawScanner::TCmdlScannerErrorCode).
	perb_cmdl_scan_max		=  459,

	perb_mid_scan_base			= 1020,			// MidLev scanner (TMidLevScanner::TMidScanErrorCode).
	perb_mid_scan_max			= 1299,

	perb_grammar_defn_base	= 1300,			// Grammar definition parser (TGrammarDefinitionParser::TGrammarDefnParserErrorCode).
	perb_grammar_defn_max	= 1399,

	perb_grammar_based_base	= 1400,			// Grammar based parser (TGrammarBasedParser::TGrammarBasedParserErrorCode).
	perb_grammar_based_max	= 1499,

	perb_cpp_parser_base		= 1500,			// C/C++ parser (TCppErrorCode, TCppCriticalErrorCode).
	perb_cpp_parser_max		= 2399,

	perb_test_code_base		= 3700,			// Test code.
	perb_test_code_max			= 3899,
};

//
// NB:	This structure can be either empty or it can contain info about some file.
//		Non empty structure has non NULL pointer in the info.file_name field.
//
//  All data fields are declared as public to allow initing this structure with initializers.
//
struct TFileInfo
{
	union
	{
		struct
		{
			ID		pars_id;			// When this field is zero, the structure describes a parsing independent file.
			ID		file_id;			// File id is either globally unique or parsing session unique.
		};

		TLongId		doc_id;
	};

	TBasicFileInfo			info;
	TFileStorageType		strg_type;
	TRawScanningType		rscn_type;

public:

	TSimpleLineInfo		*lines_info;						// Info about the lengths of the lines. This field can be NULL.
	long					lines_info_len;
	long					longest_line_len;

public:

	//
	// The fields below are set only when the file is retrieved from the database.
	// This info is not available during the regular parsing process.
	//

	ID				major_src_latest_pars_id;			// Id of the latest parsing session where this file
														// was used as the major source file.
	ID				raw_sess_id;
	TDateTime		raw_indexing_date;

	ULONG			pars_sess_refs_len;
						// The number of different parsing sessions that were using this file. Note that the session
						// reference record contains only an identifier of the parsing session and noting else.
public:

	static const ID		DiskFileIdsBase = 101;
						// The space of the disk file Ids and the space of the built in file Ids is split into two non
						// overlapping ranges. Ids of the built in files can be only less than this constant.
public:

	inline void Clear() { memset(this, 0, sizeof(TFileInfo)); }

	void Init(TFileStorageType strg, TRawScanningType rscn)
	{
		doc_id = 0;
		memset(&info, 0, sizeof(TBasicFileInfo));
		strg_type = strg; rscn_type = rscn;
		lines_info = NULL;
		lines_info_len = 0;
	}

	bool IsNull() const { return(info.file_name == NULL); }
	void ReleaseBasicFileInfo() { info.FreeBasicFileInfo(); }
	bool CloneFileNameAndBodyAndLinesInfo(TFileInfo &file_info);

	void UpdateCrc()
	{
		info.file_crc = (info.file_body != NULL) ? TFileBodyHelper::CalcCrc32(info.file_body, info.file_len*sizeof(wchar_t)) : 0;
	}

	bool BuildLinesInfo()
	{
		if (lines_info != NULL)
			return(TRUE);

		lines_info = TSimpleLineInfo::BuildLinesInfoFromData(info.file_body, info.file_len, 0, lines_info_len, longest_line_len);
		return(lines_info != NULL);
	}

	inline bool CheckLinesInfo() const
	{
		return(lines_info != NULL);
	}

	inline void ReleaseLinesInfo()
	{
		TSimpleLineInfo::FreeLinesInfo(&lines_info);
		lines_info_len = 0;
	}

	inline void AcceptLinesInfo(TSimpleLineInfo *info, long info_len, long lgst_ln_len)
	{
		lines_info = info; lines_info_len = info_len; longest_line_len = lgst_ln_len;
	}

	inline void GetLinesInfoFrom(TFileInfo &inst)
	{
		lines_info = inst.lines_info; lines_info_len = inst.lines_info_len; longest_line_len = inst.longest_line_len;
	}

	inline long NumLines() const { return(lines_info_len); }

	inline wchar_t *GetLineDataForLineNumber(int line_number) const
	{
		assert(line_number >= 1 && line_number <= lines_info_len);
		return(info.file_body+lines_info[line_number-1].line_offs);
	}

	inline long GetLineOffsForLineNumber(int line_number) const
	{
		assert(line_number >= 1 && line_number <= lines_info_len);
		return(lines_info[line_number-1].line_offs);
	}

	inline long GetLineLengthForLineNumber(int line_number) const
	{
		assert(line_number >= 1 && line_number <= lines_info_len);
		return(lines_info[line_number-1].line_len);
	}

	inline long GetLineNumber(long file_offs)
	{
		TSimpleLineInfo *li = TSimpleLineInfo::FindLineInfo(lines_info, lines_info_len, file_offs);
		if (li == NULL)
			return(-1);
		return((long)(li-lines_info)+1);
	}

	inline long GetLineNumber(TSimpleLineInfo *li)
	{
		long ln = (long)(li-lines_info)+1;
		if (ln <= 0 || ln > lines_info_len)
			return(-1);
		return(ln);
	}

	inline long GetCharPos(long file_offs)
	{
		TSimpleLineInfo *li = TSimpleLineInfo::FindLineInfo(lines_info, lines_info_len, file_offs);
		if (li == NULL)
			return(-1);
		return(file_offs-li->line_offs+1);
	}

	TSimpleLineInfo *GetLineInfoFromFileOffset(long file_offs) const
	{
		return(TSimpleLineInfo::FindLineInfo(lines_info, lines_info_len, file_offs));
	}

	inline TSimpleLineInfo *GetLineInfoFromLineNumber(long line_number)
	{
		if (line_number < 1 || line_number > lines_info_len)
			return(NULL);
		return(lines_info+line_number-1);
	}

	long		SkipEndOfLineForward(long offs);
	long		SkipEndOfLineBackward(long offs);
	long		AdjustToLineBeg(long offs) const;
	long		AdjustToLineEnd(long offs) const;
	long		AdjustToPrevLineEnd(long offs, long &curr_line_beg);
	long		AdjustToCurrLineEnd(long offs, long &next_line_beg);

	inline const wchar_t *PrepareFileId(wchar_t *buffer, long buff_len) const
	{
		// The id is formatted as "%lu" or as "%lu.lu(temp)".
		return(PrepareFileId(pars_id, file_id, buffer, buff_len));
	}

	inline const wchar_t *PrepareFileIdShort(wchar_t *buffer, long buff_len) const
	{
		// The id is formatted as "F%lu" or as "T%lu.%lu".
		return(PrepareFileIdShort(pars_id, file_id, buffer, buff_len));
	}

	static const wchar_t *PrepareFileId(ID pars, ID fid, wchar_t *buffer, long buff_len);
						// The id is formatted as "%lu" or as "%lu.lu(temp)".

	static const wchar_t *PrepareFileIdShort(ID pars, ID fid, wchar_t *buffer, long buff_len);
						// The id is formatted as "F%lu" or as "T%lu.%lu".

	static const wchar_t *GetStrgTypeEnumName(TFileStorageType val);
	static const wchar_t *GetRscnTypeEnumName(TRawScanningType val);

	friend class TFileInfoItem;
	friend class TSourceFilesTableItem;
};

//
//  Drive subst info is used when the scanner is reconstructing the parsing environment
//  that was taking place on this computer some time earlier or on some other box.
//
struct TDriveSubstInfo
{
	wchar_t		*internal_prefix;
					// The  mame of the drive, share or subdirectory as it appears on the command line
					// or in the source file. This field cannot be NULL or empty.

	wchar_t		*external_subst;
					// Location of the files where they can be picked up when they are needed.
					// The prefix that is defined in the previous field is replaced with the value of this
					// field. This field cannot be NULL or empty.
};

// - - - - - -  Section 2. Areas in the files  - - - - - -

struct TSourceAreaInfo;

//
//  Area in the source file. It is not important if this file was used in any parsing session or not.
//
struct TSourceArea
{
	TFileInfo			*file_info;			// The pointer is always not NULL.
	long				area_beg;			// Offset from the beginning of the file in WCHARs.
	long				area_len;

	inline void Clear() { memset(this, 0, sizeof(TSourceArea)); }

	void Init(TFileInfo *fi, TSourceAreaInfo &src);

	inline bool IsNull() const { return(file_info == NULL); }

	bool IsEqualTo(TSourceArea &area)
	{
		if (file_info->doc_id != area.file_info->doc_id)
			return(FALSE);
		else if (area_beg != area.area_beg || area_len != area.area_len)
			return(FALSE);

		return(TRUE);
	}

	inline long AreaBeg() const { return(area_beg); }
	inline long AreaEnd() const { return(area_beg+area_len); }

	inline wchar_t *AreaBegPtr() const { return(file_info->info.file_body+area_beg); }

	inline void ExtendEndingTo(const TSourceArea &area)
	{
		assert(file_info->doc_id == area.file_info->doc_id && AreaBeg() <= area.AreaEnd());
		area_len = area.AreaEnd()-area_beg;
	}
};

//
//  TSourceAreaInfo is used when the data is retrieved from the parsing database.
//
struct TSourceAreaInfo
{
	union
	{
		//
		// When the union is inited with constants, the first member of the union is used.
		// This is why the order of definitions in this union is important.
		//
		struct
		{
			ID       pars_id;
			ID       file_id;
		};

		TLongId      doc_id;
	};

	long             area_beg;			// Offset from the beginning of the file.
	long             area_len;

	inline void Clear() { doc_id = 0; area_beg = area_len = 0; }

	void Init(TSourceArea &area)
	{
		doc_id  = (area.file_info != NULL) ? area.file_info->doc_id : 0;

		area_beg = area.area_beg;
		area_len = area.area_len;
	}

	inline bool IsNull() const { return(file_id == 0); }
				// When the structure is not empty at least the file_id should be not zero.

	bool IsEqualTo(TSourceAreaInfo &area)
	{
		if (pars_id != area.pars_id || file_id != area.file_id)
			return(FALSE);
		else if (area_beg != area.area_beg || area_len != area.area_len)
			return(FALSE);

		return(TRUE);
	}

	inline long AreaBeg() const { return(area_beg); }
	inline long AreaEnd() const { return(area_beg+area_len); }

	inline bool Contains(long offs)
	{
		return(offs >= area_beg && offs <= area_beg+area_len);
	}

	inline bool ContainsStrict(long offs)
	{
		// The back border of the area should be included in the area only if the area is empty.
		return(offs >= area_beg && (offs < area_beg+area_len || area_len == 0));
	}

	bool Intersects(TSourceAreaInfo &other_area)
	{
		//
		// After the return the object will contain a non empty intersection if this intersection
		// is present, otherwise the object will be not changed.
		//
		assert(pars_id == other_area.pars_id);
		assert(file_id == other_area.file_id);

		long dp = area_beg - other_area.area_beg;
		if (dp < 0)
		{
			if (area_len <= -dp)
				return(FALSE);

			area_beg = other_area.area_beg;
			area_len = __min(area_len+dp, other_area.area_len);
		}
		else
		{
			if (other_area.area_len <= dp)
				return(FALSE);

			area_len = __min(area_len, other_area.area_len-dp);
		}

		// An intersection is present.
		assert(area_len > 0);
		return(TRUE);
	}

	bool CheckIntersectionAllowBorderTouch(TSourceAreaInfo &other_area)
	{
		assert(pars_id == other_area.pars_id);
		assert(file_id == other_area.file_id);

		long dp = area_beg - other_area.area_beg;
		if (dp < 0)
		{
			if (area_len < -dp)
				return(FALSE);
		}
		else
		{
			if (other_area.area_len < dp)
				return(FALSE);
		}

		return(TRUE);
	}

	inline const wchar_t *PrepareFileId(wchar_t *buffer, long buff_len)
	{
		return(TFileInfo::PrepareFileId(pars_id, file_id, buffer, buff_len));
	}

	inline const wchar_t *PrepareFileIdShort(wchar_t *buffer, long buff_len)
	{
		return(TFileInfo::PrepareFileIdShort(pars_id, file_id, buffer, buff_len));
	}

	inline void ExtendEndingTo(const TSourceAreaInfo &area)
	{
		assert(doc_id == area.doc_id && AreaBeg() <= area.AreaEnd());
		area_len = area.AreaEnd()-area_beg;
	}
};

//
//  This is an area of the linear space that defines the source that is being parsed. Linear area may span
//  over several files and macro calls. This area may describe objects that consist of several lexemas.
//
struct TLinearArea
{
	long			linear_beg;				// The first byte of the parsed source gets an offset equal to one. Because of that
										// the structure with the linear_beg equal to zero is not pointing anywhere.
	long			linear_len;				// The length of the area. It can be zero.

public:

	inline void	Clear() { linear_beg = linear_len = 0; }
	inline bool	IsNull() const { return(linear_beg == 0); }

	inline long	AreaBeg() const { return(linear_beg); }
	inline long	AreaEnd() const { return(linear_beg+linear_len); }

	inline bool	IsEqualTo(TLinearArea &inst) const { return(linear_beg == inst.linear_beg && linear_len == inst.linear_len); }
	inline bool	Contains(long offs) const { return(offs >= linear_beg && offs <= linear_beg+linear_len); }

	inline void	ExtendEndingTo(const TLinearArea &area) { assert(AreaBeg() <= area.AreaEnd()); linear_len = area.AreaEnd()-linear_beg; }
};

struct PrEventHeader;

struct TSourceLocation
{
	ID					parsing_id;
	TLinearArea			lin_area;

	ID					source_ref;
	TSourceAreaInfo		src_area;

	bool					partial_src_area;
							// When this field is TRUE, this means that linear area spans across several different source areas.
							// In this case the src_area field shows only the first segment of the linear area. When the source
							// area contains the lex source calls, this situation does not qualify to be a "partial_src_area".

	bool					src_ext_goes_up;
							// This field makes sense only when the source area is partial. When the value is FALSE, this means
							// that linear area ended inside the lex src call. Otherwise this means that the end of the lex src was
							// reached and the linear area turned out to be longer.

	inline void	Clear() { memset(this, 0, sizeof(TSourceLocation)); }
	inline bool	IsNull__() const { return(parsing_id == 0 && lin_area.linear_beg == 0); }

	inline long	LinearBeg() const	{ return(lin_area.AreaBeg()); }
	inline long	LinearEnd() const	{ return(lin_area.AreaEnd()); }
	inline long	SrcBeg() const		{ return(src_area.AreaBeg()); }
	inline long	SrcEnd() const		{ return(src_area.AreaEnd()); }

	inline void	InitFromPrHeader(ID pars_id, PrEventHeader &hdr, bool ext_goes_up = FALSE);
					// This method expects that the second param is fully inited.

	inline bool IsEqualTo(TSourceLocation &inst)
	{
		return(parsing_id == inst.parsing_id && lin_area.IsEqualTo(inst.lin_area) == TRUE && source_ref == inst.source_ref && src_area.IsEqualTo(inst.src_area) == TRUE);
	}
};

// - - - - - -  Section 3. Objects in the C/Cpp language database  - - - - - -

enum TCppItemType
{
	cxx_none,

	cxx_cond_pars_param				= 15,		// This object can be present only in the special database that was created using
													// the conditional parsing analysis engine.
	cxx_built_in_type					= 16,
	cxx_bit_field_type					= 17,		// Bit sized data fields can be present only inside the structs/unions/classes.
	cxx_cv_ptr_ref_type					= 18,
	cxx_array_type						= 19,
	cxx_data_type_alias					= 20,

	cxx_enum_type						= 21,
	cxx_enum_member					= 22,
	cxx_struct_type						= 23,		// This object is one of: struct/union/class.
	cxx_base_class_spec				= 24,
	cxx_field_align_spec					= 25,		// This is inctruction for specific alignment of the next stuct/class data field (rarely used).
	cxx_friend_spec						= 26,
	cxx_data_field						= 27,		// This is the first out of the two most important types of the objects.
	cxx_data_field_initializer				= 28,

	cxx_function_type					= 29,
	cxx_function_type_alias				= 30,
	cxx_code_entry_point				= 31,		// This is the second out of the two most important types of the objects.
	cxx_possible_exception_spec			= 32,		// This is specification of exception that can be thrown by a function (rarely used).

	cxx_namespace						= 33,
	cxx_global_namespace				= 34,		// The global namespace cannot be instantiated. It can exist only inside the cpp database.
	cxx_namespace_alias				= 35,
	cxx_using_declaration				= 36,
	cxx_using_directive					= 37,
	cxx_asm_insert						= 38,
	cxx_linkage_spec					= 39,

	cxx_class_template					= 42,
	cxx_class_template_spec			= 43,
	cxx_class_template_inst				= 44,
	cxx_func_template					= 45,
	cxx_func_template_spec				= 46,
	cxx_func_template_inst				= 47,

	cxx_template_type_param			= 48,
	cxx_template_template_param		= 49,		// This is a formal parameter of the template that is a class template itself.
	cxx_template_type_argument		= 50,
	cxx_template_template_argument	= 51,		// This is a template instantiation argument that is a class template itself.

	cxx_unresolved_name_info			= 52,		// Element of the sequence of unresolved names.
	cxx_unresolved_data_type			= 53,
	cxx_unresolved_struct_type			= 54,
	cxx_unresolved_data_field			= 55,
	cxx_unresolved_code_entry_point	= 56,
	cxx_unresolved_class_template		= 57,
	cxx_unresolved_func_template		= 58,

	cxx_empty_stmt						= 61,
	cxx_if_stmt							= 62,
	cxx_switch_stmt						= 63,
	cxx_for_stmt						= 64,
	cxx_while_stmt						= 65,
	cxx_do_while_stmt					= 66,

	cxx_goto_stmt						= 67,
	cxx_break_stmt						= 68,
	cxx_continue_stmt					= 69,
	cxx_return_stmt						= 70,

	cxx_block							= 71,
	cxx_try_block						= 72,
	cxx_catch_block						= 73,
	cxx_light_block						= 74,
	cxx_label							= 75,		// This is named label for using it in GOTO statements.
	cxx_case_label						= 76,

	cxx_operand_expr					= 77,
	cxx_unary_expr						= 78,
	cxx_binary_expr						= 79,
	cxx_multiop_expr					= 80,
	cxx_assignment_expr				= 81,

	cxx_throw_expr						= 82,
	cxx_conditional_expr				= 83,
	cxx_field_access_expr				= 84,
	cxx_field_access_ptr_expr			= 85,
	cxx_array_index_expr				= 86,
	cxx_function_call_expr				= 87,

	cxx_cast_expr						= 88,		// This can be one of: simple_cast, const_cast, dyna_cast, rein_cast, static_cast.
	cxx_sizeof_expr						= 89,
	cxx_new_expr						= 90,
	cxx_delete_expr						= 91,
	cxx_typeid_expr						= 92,

	cxx_num_types,
};

enum TCppSourceAreaType
{
	cxa_none,

	cxa_bright_keyword,
	cxa_namespace,

	cxa_enum_fwd_decl,
	cxa_struct_fwd_decl,
	cxa_union_fwd_decl,
	cxa_class_fwd_decl,

	cxa_enum_defn,
	cxa_struct_defn,
	cxa_union_defn,
	cxa_class_defn,

	cxa_enum_member,				// Enum member definition.
	cxa_data_field_xtrn,
	cxa_data_field_defn,
	cxa_function_proto,
	cxa_function_header,

	cxa_nameless_bft,
	cxa_nameless_cvpr,
	cxa_nameless_array,
	cxa_nameless_fntp,

	cxa_num_types,
};

struct TCppSourceArea
{
	TCppSourceAreaType			m_cpp_atype;
	TLinearArea					m_cpp_lin_area;

};

class TCppItemHelper
{
public:

	static const wchar_t	*GetItemTypeDisplayName(TCppItemType item_type);
	static const wchar_t	*GetItemTypeEnumName(TCppItemType item_type);
	static const wchar_t	*GetSrcAreaTypeEnumName(TCppSourceAreaType val);
};

// - - - - - -  Section 4. Event headers  - - - - - - - -

//
//  This structure is used by the scanners to report place in the source file where something was found.
//  Raw scanners setup just the source area. Other fields are set up later by the MidLev scanner. Lexema
//  cannot span across several lex src enclosures or cond parsing areas.
//
//  High level language parsers can modify or combine several event headers that were received from
//  the mid lev scanner.
//
//  Event may correspond to some place in the source code and may not. In the first case all fields of
//  the structure should be set up. In the second case all fields except for the cn should contain zeroes.
//
struct PrEventHeader
{
	ID					cn;						// The change number is assigned by the code that generates event.
												// Events can be submitted for processing in any order of their cns.
	ID					source_ref;				// This is an id of the lex source enter event that describes the beginning
												// of the linear and source areas of the event.
	ID					cond_ref;				// Header may span across several cond parsing areas. This field shows
												// the state of the beginning of the area.
	TLinearArea			lin_area;				// Linear length can be longer than the length of the corresponding source
												// area if the source area contains lex source calls.
	TSourceArea			src_area;				// Source area of the beginning of the area, described with this header.
												// Header may span across several lex sources.
	bool					partial_src_area;
	bool					mixed_cond_pars;

public:

	inline void Clear() { memset(this, 0, sizeof(PrEventHeader)); }

	inline void Setup(PrEventHeader *proto, ID new_cn)
	{
		if (proto != NULL)
			memcpy(this, proto, sizeof(PrEventHeader));
		else memset(this, 0, sizeof(PrEventHeader));

		cn = new_cn;
	}

	inline void PrepareLinearArea(long src_file_linear_beg)
	{
		lin_area.linear_beg = src_file_linear_beg+src_area.area_beg;
		lin_area.linear_len = src_area.area_len;
	}

	inline void PrepareLinAreaFromLinOffs(long linear_beg)
	{
		lin_area.linear_beg = linear_beg;
		lin_area.linear_len = src_area.area_len;
	}

	void ExtendPrHeader(PrEventHeader &ext)
	{
		// Ensure that both objects are set up.
		assert(source_ref != 0 && lin_area.linear_beg > 0);
		assert(ext.source_ref != 0 && ext.lin_area.linear_beg > 0);

		long lin_end2 = ext.lin_area.AreaEnd();
		if (lin_end2 > lin_area.AreaEnd())
		{
			// Passed header extends the current header.
			lin_area.linear_len = lin_end2-lin_area.linear_beg;

			if (source_ref == ext.source_ref)
			{
				// Process the source area.
				long src_end2 = ext.src_area.AreaEnd();
				if (src_end2 > src_area.AreaEnd())
					src_area.area_len = src_end2-src_area.area_beg;

				// The area is not partial. Although it is possible that it was partial before.
				partial_src_area = FALSE;
			}
			else
			{
				// New end of the area belongs to some other lex src enclosure.
				partial_src_area = TRUE;
			}

			if (cond_ref != ext.cond_ref)
				mixed_cond_pars = TRUE;
		}
	}

	inline void AdjustToAreaBeg()
	{
		lin_area.linear_len = 0;
		src_area.area_len = 0;

		// The area became empty. It cannot belong to different lex src enclosures or cond pars areas.
		partial_src_area = FALSE;
		mixed_cond_pars = FALSE;
	}

	inline void AdjustToAreaEnd()
	{
		// This method may put the linear and the source areas out of sync. This action can be done
		// safely only if the the source area is not partial.
		assert(partial_src_area == FALSE);

		lin_area.linear_beg += lin_area.linear_len;
		lin_area.linear_len = 0;
		src_area.area_beg += src_area.area_len;
		src_area.area_len = 0;

		if (mixed_cond_pars == TRUE)
		{
			// Several cond parsing areas are involved. Kill the cond parsing info.
			cond_ref = 0xFFFFFFFF;
		}

		mixed_cond_pars = FALSE;
	}
};

//
// Encodings for the CtvEventHeader.event_type.
//
enum CtvEventType
{
	pset_none = 0,

	pset_register_drive_subst		=  1,
	pset_register_temp_file			=  2,
	pset_paste_buffering_change		=  3,
	pset_lex_source_enter			=  4,
	pset_lex_source_exit			=  5,
	pset_macro_library_change		=  6,
	pset_cond_parsing_change		=  7,
	pset_misc_preproc_stmt			=  8,
	pset_new_include_path			=  9,
	pset_error_warning_event		= 10,
	pset_cond_param_use			= 11,

	pset_grammar_section			= 12,		// Grammar definition parsing only.
	pset_grammar_object			= 13,		// Grammar definition parsing only.
												// Objects can be: terminal, non terminal and grammar rule.
	pset_cpp_item_ref				= 14,		// Link between the place in the source code and
												// the C/Cpp database object.
	pset_num_types,
};

// Ctv event types as a bit mask.
typedef DWORD PSET_TYPES_MASK;
#define PSET_TYPE_BIT(x)  (1 << (x))
#define PSET_ALL_TYPES    (PSET_TYPES_MASK)(-1)

//
//  Header of the mid lev event as it appears in the parsing events database.
//
//  Note that the MidLev database does not garantee that all events with every event id are really existing.
//  It is also important that linear offset is not garanteed to increase with increase of the event id value.
//
struct CtvEventHeader
{
	ID					parsing_id;
	ID					event_id;
	CtvEventType			event_type;

	TLinearArea			area;			// Note that the linear area can be missing (linear_beg == 0). This means that there
										// is no direct relation between the event and any place in the source code.

	inline void Clear() { memset(this, 0, sizeof(CtvEventHeader)); }

	inline void Init(ID pars_id, CtvEventType et, PrEventHeader &hdr)
	{
		parsing_id = pars_id;
		event_id = hdr.cn;
		event_type = et;
		area = hdr.lin_area;
	}

	inline void Init(ID pars_id, TLinearArea &lin_area)
	{
		parsing_id = pars_id;
		event_id = 0;
		event_type = pset_none;
		area = lin_area;
	}

	inline bool Equal(const CtvEventHeader &inst) const
	{
		return(parsing_id == inst.parsing_id && event_id == inst.event_id);
	}

	inline bool EqualExact(const CtvEventHeader &inst) const
	{
		if (parsing_id != inst.parsing_id || event_id != inst.event_id || event_type != inst.event_type)
			return(FALSE);
		return(TRUE);
	}

	inline static int CompareHeaders(const CtvEventHeader &hdr1, const CtvEventHeader &hdr2)
	{
		if (hdr1.parsing_id > hdr2.parsing_id)
			return(1);
		else if (hdr1.parsing_id < hdr2.parsing_id)
			return(-1);

		if (hdr1.event_id > hdr2.event_id)
			return(1);
		else if (hdr1.event_id < hdr2.event_id)
			return(-1);

		// The passed headers describe the same event.
		return(0);
	}

	const wchar_t			*GetEventTypeName();
	const wchar_t			*GetEventTypeNameForListBox();
							// These methods return the name of the event type.

	static const wchar_t	*GetEventTypeName(CtvEventType val);
	static const wchar_t	*GetEventTypeEnumName(CtvEventType val);
	static const wchar_t	*GetEventTypeNameForXml(CtvEventType val);
};

inline void TSourceLocation::InitFromPrHeader(ID pars_id, PrEventHeader &hdr, bool ext_goes_up)
{
	// This method cannot be implemented inside the TSourceLocation because it uses fields of the PrEventHeader.
	parsing_id = pars_id;
	lin_area = hdr.lin_area;
	source_ref = hdr.source_ref;
	src_area.Init(hdr.src_area);
	partial_src_area = hdr.partial_src_area;
	src_ext_goes_up = ext_goes_up;
}

// - - - - - -  Section 5. Raw scanned files indexing  - - - - - -

// File indexing key type.
enum TKeyType
{
	kt_name,
	kt_number,
	kt_token,
	kt_num_types,
};

// Types of the raw indexing session contexts.
enum TRawContextType : unsigned char
{
	rctx_normal,				// Indexing key stays in the ordinary area.
	rctx_string,				// Indexing key stays inside the string.
	rctx_comment,			// Indexing key stays inside the comment.
	rctx_num_types,
};

// File indexing key definition.
struct TKeyInfo
{
	ID					key_id;				// Key id is an unique indentifier of the key in the whole mid lev database.
											// It is assigned by the LookupOrRegisterIndexingKey() method.
	TKeyType			key_type;
	wchar_t				*key_name;			// The name of the key is a plain text for identifiers and it is a "stnd text representation"
											// for all other types of the indexing keys.
	ULONG				key_file_use_cnt;
	ULONG				key_file_refs_len;
	ULONG				key_pars_use_cnt;
	ULONG				key_pars_refs_len;

	inline void Clear() { memset(this, 0, sizeof(TKeyInfo)); }

	static const wchar_t	*GetKeyTypeName(TKeyType ktp, wchar_t *buff_40_chars);
	static const wchar_t	*GetRawContextTypeName(TRawContextType ctxt, wchar_t *buff_40_chars);

	static const ID IndexingKeyIdsBase = 32;
					// The lower range of the indexing key ids is reserved for the well known ids.
};

// Info on the indexing key use while iterating the raw indexing data.
struct CtvRawIndexingKeyUseInfo
{
	ID					key_id;
	TRawContextType		ctx_type;
	TSourceAreaInfo		src_area;
};

// Use of the indexing key in the source file.
struct TKeyUseInfo
{
	TKeyInfo				key;
	TRawContextType		raw_ctx_type;
	TSourceLocation		origin;

	inline void Clear() { memset(this, 0, sizeof(TKeyUseInfo)); }
};

// - - - - - -  Section 6. High level language databases  - - - - - -

enum THighLevelLangType
{
	lang_none,					// Raw indexing session.
	lang_unknown,
	lang_grammar_def,
	lang_clang,
	lang_cpp,
	lang_c_like,					// Custom language that uses the C/C++ compatible scanner.
	lang_csh,
	lang_num_types,
};

//
//  The high level database is intended for storing objects of the high level languages. Operations on such
//  databases are language specific. Most likely this database will have a hierarchical structure.
//
class THighLevelDatabase : public TListItem
{
public:

			THighLevelDatabase(ID parsing_id = 0) { m_pars_sess_id = parsing_id; m_data_store_owned = FALSE; }
	virtual	~THighLevelDatabase() { assert(m_data_store_owned == FALSE); }

	virtual THighLevelLangType	GetDatabaseLangType() const { return(lang_unknown); }

	virtual void		ClearDatabase() = 0;

	ID				GetParsingSessionId() const { return(m_pars_sess_id); }
	bool				IsProjectDatabase() const { return(m_pars_sess_id == TProjectParsSessId); }

public:

	static const wchar_t		*GetLangTypeName(THighLevelLangType val);
	static const wchar_t		*GetLangTypeEnumName(THighLevelLangType val);
	static THighLevelLangType	GetLangTypeFromString(const wchar_t *name);

protected:

	ID				m_pars_sess_id;

	bool				m_data_store_owned;			// When the value of this field is TRUE, this means that the object is owned by
													// the THighLevelDataStorage or its derivative. Most likely this object was created
													// with the AllocateParsSessDatabase() method.
	friend class THighLevelDataStorage;
	friend class TCondParsAnalysisDataStorage;
	friend class TCppDataStorage;
};

class THighLevelDataStorage : public TListItem
{
public:
			THighLevelDataStorage() { m_latest_database = NULL; }
	virtual	~THighLevelDataStorage() { ResetStorage(); }

	virtual void		ResetStorage();

	virtual THighLevelDatabase	*AllocateParsSessDatabase(ID parsing_id) = 0;
								// This method should be called only with a non zero parsing session ID.

	virtual THighLevelDatabase	*FindParsSessDatabase(ID parsing_id);
								// It is possible to get info about the latest compilation by specifying the parsing_id equal to 0.

	virtual void		DeleteParsSessDatabase(ID parsing_id);

protected:

	enum { NUM_DATABASE_BUCKETS = 32 };

	TList						m_database_buckets[NUM_DATABASE_BUCKETS];
								// The trailing bits of the pars sess id are used as the hash value of the parsing session data.

	THighLevelDatabase		*m_latest_database;

};

#endif	// TextScan_CoreTypes_H


