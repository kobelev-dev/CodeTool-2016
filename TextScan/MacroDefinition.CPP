//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "TextScan/MidScan.H"

//----------------------------------------------------------------------------
//  ===================  TMacroDefinition  ==========================
//----------------------------------------------------------------------------

static wchar_t *g_MacroDefStatusNames[2*mds_num_types] =
{
	L"UndefSure",		L"Undefined",				// mds_hard_undef
	L"DefSure",		L"Defined",				// mds_hard_def
	L"UndefDoubt",	L"Undefined with doubt",	// mds_soft_undef
	L"DefDoubt",		L"Defined with doubt",		// mds_soft_defS
};

static wchar_t *g_MacroDefOriginNames[2*mdo_num_types] =
{
	L"Ordinary",		L"Ordinary macro",			// mdo_definition,
	L"Predefined",		L"Predefined macro",		// mdo_predefined
	L"BuiltIn",		L"Built in macro",			// mdo_builtin,
};

TMacroDefinition::TMacroDefinition(TStrPtrInfo &macro_name, TMacroDefStatus macro_status, TMacroDefOrigin macro_origin_type)
{
	hdr.Clear();
	key_info = NULL;
	def_name = macro_name;

	def_status = macro_status;
	def_origin_type = macro_origin_type;
	def_blt_in_type = mbit_simple;

	num_params = 0;
	var_args_macro = FALSE;
	macro_body.Clear();

	def_use_count = 0;
	macro_call_flag = FALSE;
}

bool TMacroDefinition::AddMacroParam(TStrPtrInfo &param_name)
{
	if (num_params >= MAX_MACRO_PARAMS_NUM)
		return(FALSE);

	def_param_name[num_params++] = param_name;
	return(TRUE);
}

int TMacroDefinition::IsMacroParamName(TStrPtrInfo &prm_str)
{
	for (int ii=0; ii<num_params; ++ii)
	{
		if (prm_str == def_param_name[ii])
			return(ii);
	}

	return(-1);
}

bool TMacroDefinition::CompareDefinitions(TMacroDefinition &macro, TStringsDict *dict)
{
	// Check the name and the number of parameters first.
	if (def_name != macro.def_name || num_params != macro.num_params || var_args_macro != macro.var_args_macro)
		return(FALSE);

	// Now it is necessary to compare the bodies of the macros. Instantiate two raw scanners for that.
	TRawScanner scanner1(macro_body.src_area.file_info, dict);
	scanner1.Seek(macro_body.src_area.area_beg, macro_body.src_area.area_len, FALSE, FALSE);
	TRawScanner scanner2(macro.macro_body.src_area.file_info, dict);
	scanner2.Seek(macro.macro_body.src_area.area_beg, macro.macro_body.src_area.area_len, FALSE, FALSE);

	// Loop over the lexemas in the bodies.
	for(;;)
	{
		// Skip comment and eol lexemas on both sides if any.
		TLexema lex1, lex2;
		do { scanner1.GetLex(lex1); }
		while (lex1.type == ltx_comment || lex1.type == ltx_eol);
		do { scanner2.GetLex(lex2); }
		while (lex2.type == ltx_comment || lex2.type == ltx_eol);

		// Check for the end of the macros.
		if (lex1.type == ltx_eof && lex2.type == ltx_eof)
		{
			// Bodies have ended at the same time.
			break;
		}

		//
		// There are non empty lexemas in both bodies.
		//

		// Check whether these lexemas are param references or not.
		int prm_inx1 = -1;
		if (lex1.type == ltx_name)
			prm_inx1 = IsMacroParamName(lex1.str_value);
		int prm_inx2 = -1;
		if (lex2.type == ltx_name)
			prm_inx2 = macro.IsMacroParamName(lex2.str_value);

		if (prm_inx1 >= 0 || prm_inx2 >= 0)
		{
			// Both lexemas are parameters. Compare their indexes.
			if (prm_inx1 != prm_inx2)
				return(FALSE);

			continue;
		}

		// Current lexemas are not params of the macro. This means that they should be identical.
		if (lex1.IsEqualTo(lex2) == FALSE)
			return(FALSE);
	}

	// Everything turned out to be identical.
	return(TRUE);
}

bool TMacroDefinition::AddCallParam(PrEventHeader &param_area, PrEventHeader &delim_area)
{
	if (num_passed_params > MAX_MACRO_PARAMS_NUM)
	{
		// There is no space for this param.
		return(FALSE);
	}

	if (num_passed_params < num_params || var_args_macro == FALSE)
	{
		// Accept this param as a named param. The delim area is discarded.
		prm_val[num_passed_params++] = param_area;
	}
	else
	{
		// Accept this param as a var_args param.
		prm_val[num_passed_params++] = param_area;
		prm_val[num_passed_params++] = delim_area;
	}

	// Success.
	return(TRUE);
}

bool TMacroDefinition::AddVargsCallParam(TMacroDefinition *src_ctx, PrEventHeader &final_delim_area)
{
	// Here is funny point that when the var_args are not present in the passed context, then
	// nothing is added and the passed final delimiter goes nowhere.
	if (src_ctx->num_passed_params > src_ctx->num_params)
	{
		// Var_args params are available in the passed ctx. Iterate them.
		int avail_vargs = src_ctx->num_passed_params;
		int ifinal = avail_vargs-1;
		for (int ip = src_ctx->num_params; ip<avail_vargs; ip += 2)
		{
			PrEventHeader &delim = (ip < ifinal) ? src_ctx->prm_val[ip+1] : final_delim_area;
			if (AddCallParam(src_ctx->prm_val[ip], delim) == FALSE)
				return(FALSE);
		}
	}

	// Adding params succeeded if any.
	return(TRUE);
}

wchar_t *TMacroDefinition::GetStatusName(TMacroDefStatus val, bool full_form)
{
	if (val < 0 || val >= mds_num_types)
		return(L"BogusMacroDefStatus");

	return(g_MacroDefStatusNames[2*val+((full_form == TRUE) ? 1 : 0)]);
}

wchar_t *TMacroDefinition::GetOriginName(TMacroDefOrigin val, bool full_form)
{
	if (val < 0 || val >= mdo_num_types)
		return(L"BogusMacroDefOrigin");

	return(g_MacroDefOriginNames[2*val+((full_form == TRUE) ? 1 : 0)]);
}

//--------------------------------------------------------------------------------------
//  =====================  TMacroDefinitionsList  =============================
//--------------------------------------------------------------------------------------

static TStrPtrInfo g_MacroDefnsList_LookupInstName = { L"", 0 };
static TMacroDefinition g_MacroDefnsList_LookupInst(g_MacroDefnsList_LookupInstName, mds_hard_def, mdo_definition);

TMacroDefinition *TMacroDefinitionsList::LookupMacro(TStrPtrInfo &macro_name)
{
	// Use single threaded approach for now to avoid creating temp macro definition on the stack.
	g_MacroDefnsList_LookupInst.def_name = macro_name;
	TMacroDefinition *mdef = FindRecord(&g_MacroDefnsList_LookupInst);
	return(mdef);
}

int TMacroDefinitionsList::GetNumMacroCalls()
{
	int num_calls = 0;
	for (StartIteration(); CheckIteration(); StepIteration())
	{
		TMacroDefinition *macro = CurrIteratedItem();
		num_calls += macro->def_use_count;
	}

	return(num_calls);
}

int TMacroDefinitionsList::GetNumCalledMacros()
{
	int num_called = 0;
	for (StartIteration(); CheckIteration(); StepIteration())
	{
		TMacroDefinition *macro = CurrIteratedItem();
		if (macro->def_use_count != 0)
			num_called++;
	}

	return(num_called);
}


