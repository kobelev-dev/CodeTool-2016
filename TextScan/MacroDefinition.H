//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	TextScan_MacroDefinition_H
#define	TextScan_MacroDefinition_H

#ifndef   TextScan_MidScan_H
#error    "This file should not be included directly. Use MidScan.h."
#endif

// -------------------------------------------------------------------
//   ================== Macro Definition =====================
// -------------------------------------------------------------------

enum TMacroDefStatus
{
	mds_hard_undef,
	mds_hard_def,
	mds_soft_undef,
	mds_soft_def,
	mds_num_types,
};

#define	GET_DEFINED_STT(x)	((bool)((((x) & 1) != 0) ? TRUE : FALSE))
#define	GET_DOUBT_STT(x)		((bool)((((x) & 2) != 0) ? TRUE : FALSE))

enum TMacroDefOrigin
{
	mdo_definition,			// The macro was introduced during parsing of the primary src or one of its include files.
	mdo_predefined,			// The macro was introduced before entering the primary source file.
	mdo_builtin,				// The macro came from the special built in file. This file is includeded when the first
							// lex source is entered.
	mdo_num_types,
};

enum TBuiltInMacroType
{
	mbit_simple,			// Simple macro that is not changing its body on each call.

	mbit_file,				//  __FILE__
	mbit_line,				//  __LINE__
	mbit_function,			//  __FUNCTION__
	mbit_counter,			//  __COUNTER__

	mbit_num_types,
};

class TKeyInfoItem;

//
// This implementation is not allowing recursive macro calls.
//
class TMacroDefinition : public THashTableItem<TMacroDefinition>
{
public:
			TMacroDefinition(TStrPtrInfo &macro_name, TMacroDefStatus macro_status, TMacroDefOrigin macro_origin_type);
			~TMacroDefinition() { redefinitions_history.DeleteAll(); }

	bool	AddMacroParam(TStrPtrInfo &param_name);
	void	SetupDefinitionArea(PrEventHeader &hdr_proto) { hdr = hdr_proto; }
	void	SetupBodyArea(PrEventHeader &body) { macro_body = body; }

	int	IsMacroParamName(TStrPtrInfo &prm_str);
			// The return val is -1 if the passed string is not a macro param. Otherwise method returns
			// a zero based macro param index.

	bool	CompareDefinitions(TMacroDefinition &other_macro, TStringsDict *dict);
			// This method also compares the bodies of the macro.

	inline void StartMacroCallParamsPassing()
	{
		num_passed_params = next_prm_index = 0;
	}

	inline int FinishMacroCallParamsPassing()
	{
		// When the var_args params are added, the delimiter after the last param is also added to the prm_val array.
		// The code is doing this because it does not know if there will be more params or not. This method is truncating
		// the last delimiter.
		if (num_passed_params > num_params && var_args_macro == TRUE)
		{
			// The var_args params are really present.
			num_passed_params--;
			assert(num_passed_params > num_params);
		}

		return(num_passed_params);
	}

	bool   AddCallParam(PrEventHeader &param_area, PrEventHeader &delim_area);
			// The delim area for normal named params (non var_args) is ignored.

	bool   AddVargsCallParam(TMacroDefinition *src_ctx, PrEventHeader &delim_area);
			// The src_ctx should be a var_args macro. It may have the var_args arguments.

	static wchar_t *GetStatusName(TMacroDefStatus val, bool full_form);
	static wchar_t *GetOriginName(TMacroDefOrigin val, bool full_form);

public:

	//
	//  Definition part.
	//

	PrEventHeader			hdr;						// This header contains the definition change number. Header area completely
													// covers the whole definition.
	TKeyInfoItem				*key_info;				// This data field is not maintaned by the macro definition class itself. It is set
													// by the mid lev scanner.
	TStrPtrInfo				def_name;				// This field always contains a non NULL terminated name of the macro.

	TMacroDefStatus			def_status;				// Current definition status of the name. This field shows if the macro name is
													// currently defined or undefined.
	TMacroDefOrigin			def_origin_type;
	TBuiltInMacroType			def_blt_in_type;			// All ordinary macros have scmt_simple value in this field.

	int						num_params;			// The number of named params.
	bool						var_args_macro;
	TStrPtrInfo				def_param_name[MAX_MACRO_PARAMS_NUM];

	PrEventHeader			macro_body;
	TLexema					scst_body_lex;			// This lexema is not empty only for macros without params that have only one
													// lexema in their body and this lexems is either a number or a charconst. This field
													// is used for conditional expressions scanning and for the grammar defn parser.
	//
	//  Statistics and history.
	//

	long						def_use_count;
	TList						redefinitions_history;

	//
	//  These functions are required for the hash table to work. Note that these functions are not used for
	//  comparing the already existing macros. They are used for placing new macrodefs into the table and
	//  looking for mdef once some name is picked up from the source. Because of that the number of params
	//  cannot be used in the hash function.
	//
	virtual DWORD	GetHashValue() { return(CalcStrHash(def_name.m_body, def_name.m_len)); }
	virtual bool		IsEqualTo(const TMacroDefinition &otherInst) { return(def_name == otherInst.def_name); }

protected:

	//
	//  Data fields for macro invocation support. The C/C++ macroprocessor does not allow recursion.
	//  So, for the simplicity and speed these macro call related fields are placed directly here.
	//

	PrEventHeader			prm_val[MAX_MACRO_PARAMS_NUM+1];
								// Macros with the variable number of parameters treat commas that are
								// part of __VA_ARGS__ as separate params. Commas occupy slots in
								// the prm_val array.

	int						num_passed_params;
	int						next_prm_index;			// This field is used for the varg params iteration.
	bool						real_paste_suff;				// This field is used for the varg params iteration.
	bool						macro_call_flag;				// This flag is set to TRUE when the macro is called.

	friend class TMidLevScanner;
};

class TMacroDefinitionsList : public THashTable<TMacroDefinitionsList, TMacroDefinition, 0x800>
{
public:

	TMacroDefinition	*LookupMacro(TStrPtrInfo &macro_name);

	int				GetNumMacroCalls();
	int				GetNumCalledMacros();
};

#endif	// TextScan_MacroDefinition_H


