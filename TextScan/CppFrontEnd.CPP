//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   C/C++ Parser and Linker.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "TextScan/CppFrontEnd.H"

//--------------------------------------------------------------------------
//  =====================  TCppNameInfo  =======================
//--------------------------------------------------------------------------

const bool TCppNameInfo::g_TemplateIdNameForm[cnit_num_types] =
{
	FALSE,			// cnit_none
	FALSE, TRUE,		// cnit_identifier
	FALSE, TRUE,		// cnit_constructor
	FALSE,			// cnit_dector
	FALSE, TRUE,		// cnit_ovld_operator
	FALSE, TRUE,		// cnit_ovld_conversion
};

static const wchar_t *g_CppDefnProblemNamePrefix[cdpr_num_types] =
{
	L"____Cdpr_None",
	L"Ambig",	L"Unapp",	L"Unres",
	L"ErrorLoc",	L"Dup",		L"Conflict",
};

// ---------  Functions  ---------

static const wchar_t *g_CppFuncObjectTypeName[4] =
{
	L"prototype of the function",		L"function",
	L"prototypes of the functions",	L"functions",
};

static const wchar_t *g_CppFuncTemplateObjectTypeName[4] =
{
	L"prototype of the function template",		L"function template",
	L"prototypes of the function templates",	L"function templates",
};

static const wchar_t *g_CppFuncTemplateExplicitInstantiationTypeName[4] =
{
	L"explicit instantiation of the function template",		L"explicit instantiation BOGUS DEFINITION of the function template",
	L"explicit instantiations of the function templates",	L"explicit instantiation BOGUS DEFINITIONS of the function templates",
};

static const wchar_t *g_CppFuncTemplateSpecializationTypeName[4] =
{
	L"prototype of the function template specialization",		L"function template specialization",
	L"prototypes of the function template specializations",		L"function template specializations",
};

// ---------  Methods  ---------

static const wchar_t *g_CppMethodObjectTypeName[4] =
{
	L"prototype of the %s method",		L"method of the %s",
	L"prototypes of the %s methods",		L"methods of the %s",
};

static const wchar_t *g_CppMethodTemplateObjectTypeName[4] =
{
	L"prototype of the %s method template",		L"%s method template",
	L"prototypes of the %s method templates",		L"%s method templates",
};

static const wchar_t *g_CppMethodTemplateExplicitInstantiationTypeName[4] =
{
	L"explicit instantiation of the %s method template",		L"explicit instantiation BOGUS DEFINITION of the %s method template",
	L"explicit instantiations of the %s method templates",		L"explicit instantiation BOGUS DEFINITIONS of the %s method templates",
};

static const wchar_t *g_CppMethodTemplateSpecializationTypeName[4] =
{
	L"prototype of the %s method template specialization",		L"definition of the %s method template specialization",
	L"prototypes of the %s method template specializations",		L"definitions of the %s method template specializations",
};

// ---------  Constructors  ---------

static const wchar_t *g_CppClassCtorObjectTypeName[4] =
{
	L"prototype of the constructor",		L"constructor of the %s",
	L"prototypes of the constructors",		L"constructors of the %s",
};

static const wchar_t *g_CppClassCtorTemplateObjectTypeName[4] =
{
	L"prototype of the constructor template",		L"constructor template",
	L"prototypes of the constructor templates",		L"constructor templates",
};

static const wchar_t *g_CppClassCtorTemplateExplicitInstantiationTypeName[4] =
{
	L"explicit instantiation of the %s constructor template",		L"explicit instantiation BOGUS DEFINITION of the %s constructor template",
	L"explicit instantiations of the %s constructor templates",		L"explicit instantiation BOGUS DEFINITIONS of the %s constructor templates",
};

static const wchar_t *g_CppClassCtorTemplateSpecializationTypeName[4] =
{
	L"prototype of the constructor template specialization",		L"definition of the constructor template specialization",
	L"prototypes of the constructor template specializations",		L"definitions of the constructor template specializations",
};

// ---------  Destructors  ---------

static const wchar_t *g_CppClassDectorObjectTypeName[4] =
{
	L"prototype of the destructor",		L"destructor of the %s",
	L"prototypes of the destructors",		L"destructors of the %s",
};

static const wchar_t *g_CppClassDectorTemplateObjectTypeName[4] =
{
	L"prototype of the BOGUS destructor template",		L"definition of the BOGUS destructor template",
	L"prototypes of the BOGUS destructor templates",	L"definitions of the BOGUS destructor templates",
};

// ---------  Overloaded Operators  ---------

static const wchar_t *g_CppOvldOperatorTypeName[4] =
{
	L"prototype of the overloaded operator",		L"overloaded operator",
	L"prototypes of the overloaded operators",		L"overloaded operators",
};

static const wchar_t *g_CppOvldOperatorTemplateTypeName[4] =
{
	L"prototype of the overloaded operator template",		L"overloaded operator template",
	L"prototypes of the overloaded operator templates",		L"overloaded operator templates",
};

static const wchar_t *g_CppOvldOperatorTemplateExplicitInstantiationName[4] =
{
	L"explicit instantiation of the overloaded operator template",		L"explicit instantiation BOGUS DEFINITION of the overloaded operator template",
	L"explicit instantiations of the overloaded operator templates",		L"explicit instantiation BOGUS DEFINITIONS of the overloaded operator templates",
};

static const wchar_t *g_CppOvldOperatorTemplateSpecializationTypeName[4] =
{
	L"prototype of the overloaded operator template specialization",		L"definition of the overloaded operator template specialization",
	L"prototypes of the overloaded operator template specializations",		L"definitions of the overloaded operator template specializations",
};

// ---------  Overloaded Conversions  ---------

static const wchar_t *g_CppOvldConversionTypeName[4] =
{
	L"prototype of the overloaded conversion",		L"definition of the overloaded conversion",
	L"prototypes of the overloaded conversions",	L"definitions of the overloaded conversions",
};

static const wchar_t *g_CppOvldConversionTemplateTypeName[4] =
{
	L"prototype of the overloaded conversion template",		L"definition of the overloaded conversion template",
	L"prototypes of the overloaded conversion templates",	L"definitions of the overloaded conversion templates",
};

static const wchar_t *g_CppOvldConversionTemplateExplicitInstantiationName[4] =
{
	L"explicit instantiation of the overloaded conversion template",		L"explicit instantiation BOGUS DEFINITION of the overloaded conversion template",
	L"explicit instantiations of the overloaded conversion templates",	L"explicit instantiation BOGUS DEFINITIONS of the overloaded conversion templates",
};

static const wchar_t *g_CppOvldConversionTemplateSpecializationTypeName[4] =
{
	L"prototype of the overloaded conversion template specialization",		L"definition of the overloaded conversion template specialization",
	L"prototypes of the overloaded conversion template specializations",	L"definitions of the overloaded conversion template specializations",
};

void TCppNameInfo::SetupName(TCppNameInfoType name_subt, ID key_id)
{
	Clear();
	m_core.m_info_type = name_subt;
	m_core.m_key_id = key_id;
}

void TCppNameInfo::SetupOverloadedOperator(TCppNameInfoType ovld_op_type, TOperatorToken ovld_operator)
{
	Clear();
	m_core.m_info_type = ovld_op_type;
	m_core.m_ovld_operator_token = ovld_operator;
}

void TCppNameInfo::SetupOverloadedConversion(TCppNameInfoType ovld_conv_type, TCppDataTypeBase *ovld_conv_dest)
{
	Clear();
	m_core.m_info_type = ovld_conv_type;
	m_core.m_ovld_conversion_dest = ovld_conv_dest;
}

void TCppNameInfo::FinalizeShortNameInfo(TCppParser *inst, TSymbolStateItem &new_non_term, wchar_t *new_cached_name, TList *template_args_list)
{
	assert(IsQualified() == FALSE);
	m_cached_name = new_cached_name;
	m_template_args_list = template_args_list;

	// Current name info describes the short name. Place identical info into the full and short headers.
	m_qual_name_context = new_non_term.GetOriginHeader();
	m_short_name_context = m_qual_name_context;
}

void TCppNameInfo::SetupLayerResolutionProblem(TList *initial_unres_names_list, TCppDefnProblem layer_problem)
{
	assert(initial_unres_names_list != NULL && layer_problem != cdpr_none);
	assert(m_unresolved_names_list == NULL && m_name_problem == cdpr_none);

	m_name_problem = layer_problem;
	m_unresolved_names_list = initial_unres_names_list;
}

bool TCppNameInfo::FinalizeLayerResProblemName(TCppParser *inst)
{
	// This method expects that object contains unresolved defn layers and that the name problem is already set.
	assert(m_name_problem != cdpr_none && m_preferred_object_id == 0 && m_unresolved_names_list != NULL);

	m_core.ClearSearchKeys();
	m_preferred_object_id = inst->m_cpp_database.GetNextItemId();

	const wchar_t *name_prefix = GetDefnProblemNamePrefix(m_name_problem);
	m_cached_name = inst->m_cpp_database.CreateUnresolvedObjectName(name_prefix, m_unresolved_names_list, m_cached_name, m_preferred_object_id);
	if (m_cached_name == NULL)
	{
		inst->RaiseCriticalCppError(ccerr_oom_generating_object_name, L"an unresolved layer");
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

void TCppNameInfo::DowngradeToUnqualifiedNameInfo(TCppParser *inst)
{
	assert(m_preferred_object_id == 0 && m_unproc_template_header == NULL);

	m_name_problem = cdpr_none;
	m_resolved_defns_layer = NULL;

	if (m_unresolved_names_list != NULL)
	{
		inst->RecycleGeneralPurposeList(m_unresolved_names_list);
		m_unresolved_names_list = NULL;
	}
}

void TCppNameInfo::DowngradeTemplateIdToIdentifier(TCppParser *inst)
{
	// Current name should a TemplateId. Ensure this.
	assert(IsTempalteId() == TRUE);

	// Update the structure.
	m_core.m_info_type = cnit_identifier;
	m_cached_name = GetCashedNonTemplateIdName(inst);
	ReleaseTemplateArgsList(inst);
}

bool TCppNameInfo::DowngradeExpressionIdToIdentifier(TCppParser *inst, const wchar_t *subst_name_prefix)
{
	// This method converts any form of the name into the identifier.
	assert(IsIdentifier() == FALSE);

	if (IsTempalteId() == TRUE)
	{
		// Just remove the template arguments without resetting the key id and setting the name problem.
		DowngradeTemplateIdToIdentifier(inst);
	}
	else
	{
		// Situation is worse. The name does not contain an identifier.
		m_core.m_info_type = cnit_identifier;
		m_core.ClearSearchKeys();

		m_name_problem = cdpr_unapprop;
		m_preferred_object_id = inst->m_cpp_database.GetNextItemId();

		m_cached_name = inst->m_cpp_database.CreateAlternativeName(subst_name_prefix, m_preferred_object_id);
		if (m_cached_name == NULL)
		{
			inst->RaiseCriticalCppError(ccerr_oom_setting_alt_name);
			return(FALSE);
		}

		ReleaseTemplateArgsList(inst);
	}

	// Success.
	return(TRUE);
}

bool TCppNameInfo::SetupShortNameDefnProblem(TCppParser *inst, TCppDefnProblem name_problem)
{
	assert(name_problem != cdpr_none && m_name_problem == cdpr_none);
	assert(m_preferred_object_id == 0 && m_unresolved_names_list == NULL);

	m_name_problem = name_problem;
	m_preferred_object_id = inst->m_cpp_database.GetNextItemId();
	const wchar_t *name_prefix = GetDefnProblemNamePrefix(name_problem);

	if (name_prefix == NULL)
		m_cached_name = inst->m_cpp_database.CreateAlternativeName(m_cached_name, m_preferred_object_id);
	else m_cached_name = inst->m_cpp_database.CreateAlternativeNameEx(name_prefix, m_cached_name, m_preferred_object_id);

	if (m_cached_name == NULL)
	{
		inst->RaiseCriticalCppError(ccerr_oom_setting_alt_name);
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

void TCppNameInfo::CheckRemoveTemplateDefnTemplateArgs(TCppParser *inst)
{
	if (m_unproc_template_header != NULL && TemplateArgsPresent() == TRUE)
	{
		// Template header is present and the subtype of the name allows presence of the template arguments.
		if (TCppItemBase::IsTemplateDefinitionArgsList(TemplateHeaderParams(), m_template_args_list) == TRUE)
		{
			// Process this case as a simple template definition.
			m_core.m_info_type = (TCppNameInfoType)(m_core.m_info_type-1);
			m_cached_name = GetCashedNonTemplateIdName(inst);
			ReleaseTemplateArgsList(inst);
		}
	}
}

void TCppNameInfo::SetNameToObject(TCppDefnBase *defn) const
{
	assert(defn->IsDefinition() == TRUE && defn->IsFunctionName() == FALSE);
	assert(m_preferred_object_id == 0 || defn->ItemId() == m_preferred_object_id);

	defn->m_defn_problem = m_name_problem;
	defn->m_defn_name = m_cached_name;
	defn->m_defn_key_id = m_core.m_key_id;
}

void TCppNameInfo::SetNameToFunctionObject(TCppFunctionTypeDefn *func_defn) const
{
	assert(func_defn->IsDefinition() == TRUE && func_defn->IsFunctionName() == TRUE);
	assert(m_preferred_object_id == 0 || func_defn->ItemId() == m_preferred_object_id);

	func_defn->m_defn_problem = m_name_problem;
	func_defn->m_defn_name = m_cached_name;

	func_defn->m_defn_key_id = m_core.m_key_id;
	func_defn->m_operator_token = m_core.m_ovld_operator_token;
	func_defn->m_return_value_type = m_core.m_ovld_conversion_dest;
}

const wchar_t *TCppNameInfo::GetDefnProblemNamePrefix(TCppDefnProblem cdpr)
{
	if (cdpr < 0 || cdpr >= cdpr_num_types)
		return(L"BogusCdpr");

	return(g_CppDefnProblemNamePrefix[cdpr]);
}

wchar_t *TCppNameInfo::GetCashedNonTemplateIdName(TCppParser *inst) const
{
	// Downgrade current cashed name to the non template form. In fact, the persistent string of the old,
	// non template form of the cashed name is lost. It is difficult to pick it up from the indexing keys table
	// of the scanner because this hash table is indexed by the value if the string and it does not have any
	// other index. Find the temlate args opening angle bracket in the current cached name and use this
	// name to repick the persistent name from the strings table of the database. This procedure should
	// not fail because the non template form of the current name should be cased earlier.
	wchar_t *ptr_bracket = wcschr(m_cached_name, L'<');
	if (ptr_bracket == NULL)
		return(m_cached_name);

	int len_name = (int)(ptr_bracket-m_cached_name);
	wchar_t *new_cached_name = inst->m_cpp_database.m_global_strings_dict.RegisterStr(m_cached_name, len_name);
	assert(new_cached_name != NULL);
	return(new_cached_name);
}

const wchar_t *TCppNameInfo::GetFuncNameLs(wchar_t *buff96, TCppDefnBase *dest_layer, bool func_hdr_ctx) const
{
	return(GetFuncObjectTypeName(buff96, FALSE, FALSE, dest_layer->DefnElabType(), func_hdr_ctx));
}

const wchar_t *TCppNameInfo::GetFuncNameLp(wchar_t *buff96, TCppDefnBase *dest_layer, bool func_hdr_ctx) const
{
	return(GetFuncObjectTypeName(buff96, FALSE, TRUE, dest_layer->DefnElabType(), func_hdr_ctx));
}

const wchar_t *TCppNameInfo::GetFuncNameCs(wchar_t *buff96, TCppDefnBase *dest_layer, bool func_hdr_ctx) const
{
	return(GetFuncObjectTypeName(buff96, TRUE, FALSE, dest_layer->DefnElabType(), func_hdr_ctx));
}

const wchar_t *TCppNameInfo::GetFuncNameCp(wchar_t *buff96, TCppDefnBase *dest_layer, bool func_hdr_ctx) const
{
	return(GetFuncObjectTypeName(buff96, TRUE, TRUE, dest_layer->DefnElabType(), func_hdr_ctx));
}

const wchar_t *TCppNameInfo::GetFuncObjectTypeName(wchar_t *buff96, bool want_caps, bool want_plurals, TCppElabType elab_type, bool func_hdr_ctx) const
{
	int inx = ((want_plurals == TRUE) ? 2 : 0) + ((func_hdr_ctx == TRUE) ? 1 : 0);

	TList *header_params_list = NULL;
	if (m_unproc_template_header != NULL)
		header_params_list = m_unproc_template_header->m_template_info.m_parameters_list;

	switch (m_core.m_info_type)
	{
		case cnit_identifier:
				{
					if (elab_type == elab_none)
					{
						if (m_unproc_template_header == NULL)
						{
							// This is name of the function.
							wcscpy(buff96, g_CppFuncObjectTypeName[inx]);
						}
						else
						{
							// This is name of the function template.
							wcscpy(buff96, g_CppFuncTemplateObjectTypeName[inx]);
						}
					}
					else
					{
						if (m_unproc_template_header == NULL)
						{
							// This is name of the method.
							swprintf(buff96, 96, g_CppMethodObjectTypeName[inx], TCppDefnBase::GetElabTypeName(elab_type, FALSE, (want_plurals == TRUE && func_hdr_ctx == TRUE)));
						}
						else
						{
							// This is name of the method template.
							swprintf(buff96, 96, g_CppMethodTemplateObjectTypeName[inx], TCppDefnBase::GetElabTypeName(elab_type));
						}
					}
				}
				break;

		case cnit_template_id:
				{
					if (elab_type == elab_none)
					{
						if (header_params_list == NULL && func_hdr_ctx == FALSE)
						{
							// This is name of the explicit instantiation of the function template.
							wcscpy(buff96, g_CppFuncTemplateExplicitInstantiationTypeName[inx]);
						}
						else
						{
							// This is name of the function template specialization.
							wcscpy(buff96, g_CppFuncTemplateSpecializationTypeName[inx]);
						}
					}
					else
					{
						if (header_params_list == NULL && func_hdr_ctx == FALSE)
						{
							// This is name of the explicit instantiation of the method template.
							swprintf(buff96, 96, g_CppMethodTemplateExplicitInstantiationTypeName[inx], TCppDefnBase::GetElabTypeName(elab_type));
						}
						else
						{
							// This is name of the method template specialization.
							swprintf(buff96, 96, g_CppMethodTemplateSpecializationTypeName[inx], TCppDefnBase::GetElabTypeName(elab_type));
						}
					}
				}
				break;

		case cnit_constructor:
				{
					if (m_unproc_template_header == NULL)
					{
						// This is name of the constructor.
						swprintf(buff96, 96, g_CppClassCtorObjectTypeName[inx], TCppDefnBase::GetElabTypeName(elab_type, FALSE, (want_plurals == TRUE && func_hdr_ctx == TRUE)));
					}
					else
					{
						// This is name of the constructor template.
						wcscpy(buff96, g_CppClassCtorTemplateObjectTypeName[inx]);
					}
				}
				break;

		case cnit_ctor_template:
				{
					if (header_params_list == NULL && func_hdr_ctx == FALSE)
					{
						// This is name of the explicit instantiation of the constructor template.
						swprintf(buff96, 96, g_CppClassCtorTemplateExplicitInstantiationTypeName[inx], TCppDefnBase::GetElabTypeName(elab_type));
					}
					else
					{
						// This is name of the constructor template specialization.
						wcscpy(buff96, g_CppClassCtorTemplateSpecializationTypeName[inx]);
					}
				}
				break;

		case cnit_dector:
				{
					if (m_unproc_template_header == NULL)
					{
						// This is name of the destructor.
						swprintf(buff96, 96, g_CppClassDectorObjectTypeName[inx], TCppDefnBase::GetElabTypeName(elab_type, FALSE, (want_plurals == TRUE && func_hdr_ctx == TRUE)));
					}
					else
					{
						// This is name of the destructor template.
						wcscpy(buff96, g_CppClassDectorTemplateObjectTypeName[inx]);
					}
				}
				break;

		case cnit_ovld_operator:
				{
					if (m_unproc_template_header == NULL)
					{
						// This is name of the overloaded operator.
						wcscpy(buff96, g_CppOvldOperatorTypeName[inx]);
					}
					else
					{
						// This is name of the overloaded operator template.
						wcscpy(buff96, g_CppOvldOperatorTemplateTypeName[inx]);
					}
				}
				break;

		case cnit_ovld_op_template:
				{
					if (header_params_list == NULL && func_hdr_ctx == FALSE)
					{
						// This is name of the explicit instantiation of the overloaded operator template.
						wcscpy(buff96, g_CppOvldOperatorTemplateExplicitInstantiationName[inx]);
					}
					else
					{
						// This is name of the overloaded operator template specialization.
						wcscpy(buff96, g_CppOvldOperatorTemplateSpecializationTypeName[inx]);
					}
				}
				break;

		case cnit_ovld_conversion:
				{
					if (m_unproc_template_header == NULL)
					{
						// This is name of the overloaded conversion.
						wcscpy(buff96, g_CppOvldConversionTypeName[inx]);
					}
					else
					{
						// This is name of the overloaded conversion template.
						wcscpy(buff96, g_CppOvldConversionTemplateTypeName[inx]);
					}
				}
				break;

		case cnit_ovld_conv_template:
				{
					if (header_params_list == NULL && func_hdr_ctx == FALSE)
					{
						// This is name of the explicit instantiation of the overloaded conversion template.
						wcscpy(buff96, g_CppOvldConversionTemplateExplicitInstantiationName[inx]);
					}
					else
					{
						// This is name of the overloaded conversion template specialization.
						wcscpy(buff96, g_CppOvldConversionTemplateSpecializationTypeName[inx]);
					}
				}
				break;

		default:
			{
				// Bogus value of the name subtype.
				return(L"UNSUPP-NAME-SUBTYPE");
			}
	}

	if (want_caps == TRUE)
	{
		// Catipalize the first character in the buffer.
		if (buff96[0] == L'p')
			buff96[0] = L'P';
		else if (buff96[0] == L'd')
			buff96[0] = L'D';
		else if (buff96[0] == L'e')
			buff96[0] = L'E';
		else if (buff96[0] == L'c')
			buff96[0] = L'C';
		else if (buff96[0] == L's')
			buff96[0] = L'S';
		else if (buff96[0] == L'u')
			buff96[0] = L'U';
		else if (buff96[0] == L'f')
			buff96[0] = L'F';
		else if (buff96[0] == L'm')
			buff96[0] = L'M';
		else if (buff96[0] == L'o')
			buff96[0] = L'O';
	}

	// Success.
	return(buff96);
}

void TCppNameInfo::ReleaseResources(TCppParser *inst)
{
	// Both resource fields are optional. Note that this method should be called only before complete
	// releasing of the structure because it is not clearing the resource fields.
	if (m_template_args_list != NULL)
		inst->RecycleGeneralPurposeList(m_template_args_list);
	if (m_unresolved_names_list != NULL)
		inst->RecycleGeneralPurposeList(m_unresolved_names_list);
}

void TCppNameInfo::ReleaseTemplateArgsList(TCppParser *inst)
{
	if (m_template_args_list != NULL)
	{
		inst->RecycleGeneralPurposeList(m_template_args_list);
		m_template_args_list = NULL;
	}
}

//--------------------------------------------------------------------------
//  ===================  TCppBuiltInTypeInfo  ======================
//--------------------------------------------------------------------------

TCppErrorCode TCppBuiltInTypeInfo::AddBuiltInTypeSpec(WORD terminal_symbol_app_id)
{
	switch (terminal_symbol_app_id)
	{
		case sym_signed:
				{
					if (m_signed_spec_present == TRUE || m_unsigned_spec_present == TRUE)
						return(cerr_bltin_sign_spec_twice);

					if (m_pri_type_present == TRUE)
					{
						if (m_primary_type == bltin_type_void || m_primary_type == bltin_type_bool || m_primary_type == bltin_type_float)
							return(cerr_bltin_sign_spec_with_bad_type);
					}

					// The specification"signed" is compatible with the current set of specs.
					m_signed_spec_present = TRUE;
				}
				break;

		case sym_unsigned:
				{
					if (m_signed_spec_present == TRUE || m_unsigned_spec_present == TRUE)
						return(cerr_bltin_sign_spec_twice);

					if (m_pri_type_present == TRUE)
					{
						if (m_primary_type == bltin_type_void || m_primary_type == bltin_type_bool || m_primary_type == bltin_type_float)
							return(cerr_bltin_sign_spec_with_bad_type);
					}

					// The specification "unsigned" is compatible with the current set of specs.
					m_unsigned_spec_present = TRUE;
				}
				break;

		case sym_short:
				{
					if (m_short_spec_present == TRUE || m_long_spec_present == TRUE)
						return(cerr_bltin_size_spec_twice);

					if (m_pri_type_present == TRUE)
					{
						if (m_primary_type != bltin_type_int)
							return(cerr_bltin_size_spec_with_bad_type);
					}

					// The specification "short" is compatible with the current set of specs.
					m_short_spec_present = TRUE;
				}
				break;

		case sym_long:
				{
					if (m_short_spec_present == TRUE || m_long_spec_present == TRUE)
						return(cerr_bltin_size_spec_twice);

					if (m_pri_type_present == TRUE)
					{
						if (m_primary_type == bltin_type_void || m_primary_type == bltin_type_bool || m_primary_type == bltin_type_char || (m_primary_type == bltin_type_float && m_pri_type_long == FALSE))
							return(cerr_bltin_size_spec_with_bad_type);
					}

					// The specification "long" is compatible with the current set of specs.
					m_long_spec_present = TRUE;
				}
				break;

		case sym_void:
				{
					if (m_signed_spec_present == TRUE || m_unsigned_spec_present == TRUE)
						return(cerr_bltin_bad_type_for_sign_spec);
					else if (m_short_spec_present == TRUE || m_long_spec_present == TRUE)
						return(cerr_bltin_bad_type_for_size_spec);
					else if (m_pri_type_present == TRUE)
						return(cerr_bltin_type_already_present);

					// The primary type "void" is compatible with the current set of specs.
					m_primary_type = bltin_type_void;
					m_pri_type_present = TRUE;
				}
				break;

		case sym_bool:
				{
					if (m_signed_spec_present == TRUE || m_unsigned_spec_present == TRUE)
						return(cerr_bltin_bad_type_for_sign_spec);
					else if (m_short_spec_present == TRUE || m_long_spec_present == TRUE)
						return(cerr_bltin_bad_type_for_size_spec);
					else if (m_pri_type_present == TRUE)
						return(cerr_bltin_type_already_present);

					// The primary type "bool" is compatible with the current set of specs.
					m_primary_type = bltin_type_bool;
					m_pri_type_present = TRUE;
				}
				break;

		case sym_char:
				{
					if (m_short_spec_present == TRUE || m_long_spec_present == TRUE)
						return(cerr_bltin_bad_type_for_size_spec);
					else if (m_pri_type_present == TRUE)
						return(cerr_bltin_type_already_present);

					// The primary type "char" is compatible with the current set of specs.
					m_primary_type = bltin_type_char;
					m_pri_type_present = TRUE;
				}
				break;

		case sym_wchar:
				{
					if (m_short_spec_present == TRUE || m_long_spec_present == TRUE)
						return(cerr_bltin_bad_type_for_size_spec);
					else if (m_pri_type_present == TRUE)
						return(cerr_bltin_type_already_present);

					// The primary type "wchar_t" is compatible with the current set of specs.
					m_primary_type = bltin_type_char;
					m_pri_type_long = TRUE;
					m_pri_type_present = TRUE;
				}
				break;

		case sym_int:
				{
					if (m_pri_type_present == TRUE)
						return(cerr_bltin_type_already_present);

					// The primary type "int" is compatible with the current set of specs.
					m_primary_type = bltin_type_int;
					m_pri_type_present = TRUE;
				}
				break;

		case sym_float:
				{
					if (m_signed_spec_present == TRUE || m_unsigned_spec_present == TRUE)
						return(cerr_bltin_bad_type_for_sign_spec);
					else if (m_short_spec_present == TRUE || m_long_spec_present == TRUE)
						return(cerr_bltin_bad_type_for_size_spec);
					else if (m_pri_type_present == TRUE)
						return(cerr_bltin_type_already_present);

					// The primary type "float" is compatible with the current set of specs.
					m_primary_type = bltin_type_float;
					m_pri_type_present = TRUE;
				}
				break;

		case sym_double:
				{
					if (m_signed_spec_present == TRUE || m_unsigned_spec_present == TRUE)
						return(cerr_bltin_bad_type_for_sign_spec);
					else if (m_short_spec_present == TRUE)
						return(cerr_bltin_bad_type_for_size_spec);
					else if (m_pri_type_present == TRUE)
						return(cerr_bltin_type_already_present);

					// The primary type "double" is compatible with the current set of specs.
					m_primary_type = bltin_type_float;
					m_pri_type_long = TRUE;
					m_pri_type_present = TRUE;
				}
				break;

		default:
			{
				// Passed app id of the symbol is bogus.
				return(cerr_bltin_unknown_symbol_app_id);
			}
	}

	// Success.
	return(cerr_none);
}

TCppBuiltInTypeIdent TCppBuiltInTypeInfo::GetBuiltInTypeIdent() const
{
	if (m_pri_type_present == TRUE)
	{
		switch (m_primary_type)
		{
			case bltin_type_void:
					{
						// Modifiers are not applicable to this type.
						return(bltin_id_void);
					}

			case bltin_type_bool:
					{
						// Modifiers are not applicable to this type.
						return(bltin_id_bool);
					}

			case bltin_type_char:
					{
						if (m_signed_spec_present == TRUE)
						{
							// The signed property of the character type was stated explicitly.
							return((m_pri_type_long == FALSE) ? bltin_id_schar8 : bltin_id_schar16);
						}
						else if (m_unsigned_spec_present == TRUE)
						{
							// The unsigned property of the character type was stated explicitly.
							return((m_pri_type_long == FALSE) ? bltin_id_uchar8 : bltin_id_uchar16);
						}
						else
						{
							// The signed/unsigned property of the char is implicit.
							return((m_pri_type_long == FALSE) ? bltin_id_schar8 : bltin_id_uchar16);
						}
					}

			case bltin_type_int:
					{
						if (m_unsigned_spec_present == TRUE)
						{
							// Check presence of the short modifier only. Int and long int types have the same size.
							return((m_short_spec_present == TRUE) ? bltin_id_uint16 : bltin_id_uint32);
						}
						else
						{
							// Check presence of the short modifier only. Int and long int types have the same size.
							return((m_short_spec_present == TRUE) ? bltin_id_sint16 : bltin_id_sint32);
						}
					}

			case bltin_type_float:
					{
						if (m_pri_type_long == FALSE)
						{
							// The float type cannot have modifiers.
							return(bltin_id_float4);
						}
						else
						{
							// Check for the long modifier.
							return((m_long_spec_present == FALSE) ? bltin_id_float8 : bltin_id_float16);
						}
					}

			default:
				{
					// The value of the primary type is bogus.
					assert(FALSE);
					return(bltin_id_void);
				}
		}
	}
	else if (m_signed_spec_present == TRUE || m_unsigned_spec_present == TRUE || m_short_spec_present == TRUE || m_long_spec_present == TRUE)
	{
		// The primary type is missing, but the modifiers are present. Result will be some flavor of int.
		if (m_unsigned_spec_present == TRUE)
		{
			// Check presence of the short modifier only. Int and long int types have the same size.
			return((m_short_spec_present == TRUE) ? bltin_id_uint16 : bltin_id_uint32);
		}
		else
		{
			// Check presence of the short modifier only. Int and long int types have the same size.
			return((m_short_spec_present == TRUE) ? bltin_id_sint16 : bltin_id_sint32);
		}
	}
	else
	{
		// The structure is completely empty. This should never happen.
		assert(FALSE);
		return(bltin_id_void);
	}
}

//--------------------------------------------------------------------------
//  ==================  TCppSymbolStateItem  =====================
//--------------------------------------------------------------------------

bool TCppSymbolStateItem::GetAppLayerDescription(TDestinationFile &doc)
{
	//
	// Note that meaning of the symbol item is taken here NOT from the m_meaning field. This is done so
	// because the primary meaning field can be destroyed when other non terminal is being constructed.
	// While the m_sym_meaning_ex field contains a stable copy of the original meaning. In a few cases
	// the code needs to change the meaning of the symbol that is not about to be deleted. In this case
	// it should modify both fields.
	//

	if (m_symbol < NonTerminalsBase && m_sym_meaning_ex == 0)
	{
		doc.Write(L"<TerminalSymbol>");
		return(TRUE);
	}

	switch (m_sym_meaning_ex)
	{
		case csm_none:					doc.Write(L"csm_none");					break;
		case csm_simp_or_qual:			doc.Write(L"csm_simp_or_qual");				break;

		case csm_declaration:				doc.Write(L"csm_declaration");				break;
		case csm_declarations_list:			doc.Write(L"csm_declarations_list");			break;
		case csm_decl_type_spec:			doc.Write(L"csm_decl_type_spec");			break;
		case csm_decl_prefix:				doc.Write(L"csm_decl_prefix");				break;
		case csm_function_attr:			doc.Write(L"csm_function_attr");				break;
		case csm_storage_class:			doc.Write(L"csm_storage_class");			break;
		case csm_built_in_type_spec:		doc.Write(L"csm_built_in_type_spec");		break;
		case csm_linkage_spec_header:		doc.Write(L"csm_linkage_spec_header");		break;

		case csm_declarator:				doc.Write(L"csm_declarator");				break;
		case csm_ptr_operator:			doc.Write(L"csm_ptr_operator");				break;
		case csm_cv_qualifier:				doc.Write(L"csm_cv_qualifier");				break;
		case csm_func_params_list:			doc.Write(L"csm_func_params_list");			break;
		case csm_func_parameter:			doc.Write(L"csm_func_parameter");			break;
		case csm_exceptions_list:			doc.Write(L"csm_exceptions_list");			break;
		case csm_data_type:				doc.Write(L"csm_data_type");				break;
		case csm_initializer:				doc.Write(L"csm_initializer");				break;

		case csm_namesp_header:			doc.Write(L"csm_namesp_header");			break;
		case csm_enum_header:			doc.Write(L"csm_enum_header");			break;

		case csm_class_header:			doc.Write(L"csm_class_header");				break;
		case csm_class_key:				doc.Write(L"csm_class_key");				break;
		case csm_access_spec:			doc.Write(L"csm_access_spec");				break;
		case csm_base_classes_list:			doc.Write(L"csm_base_classes_list");			break;
		case csm_base_class_spec:			doc.Write(L"csm_base_class_spec");			break;

		case csm_template_header:			doc.Write(L"csm_template_header");			break;
		case csm_template_params_list:		doc.Write(L"csm_template_params_list");		break;

		case csm_function_header:			doc.Write(L"csm_function_header");			break;
		case csm_overloadable_operator:	doc.Write(L"csm_overloadable_operator");		break;

		case csm_statement:				doc.Write(L"csm_statement");				break;
		case csm_block_header:			doc.Write(L"csm_block_header");				break;
		case csm_do_statement_body:		doc.Write(L"csm_do_statement_body");		break;
		case csm_catch_blocks_seq:		doc.Write(L"csm_catch_blocks_seq");			break;
		case csm_catch_block:				doc.Write(L"csm_catch_block");				break;
		case csm_exception_param:		doc.Write(L"csm_exception_param");			break;

		case csm_expression:				doc.Write(L"csm_expression");				break;
		case csm_assignment_operator:		doc.Write(L"csm_assignment_operator");		break;
		case csm_unary_operator:			doc.Write(L"csm_unary_operator");			break;
		case csm_strings_seq:				doc.Write(L"csm_strings_seq");				break;

		default:
			assert(FALSE);
			return(FALSE);
	}

	return(TRUE);
}

//-------------------------------------------------------------------------------
//  ==================  TCppParserCtvRefEventInfo  ======================
//-------------------------------------------------------------------------------

void TCppParserCtvRefEventInfo::RecycleEventInfo()
{
	if (m_owner_parser != NULL)
	{
		m_owner_parser->RecycleUnsubmittedCtvEventInfo(this);
	}
}

//-------------------------------------------------------------------------------
//  =======================  TCppParser  ===========================
//-------------------------------------------------------------------------------

static TCppNamespaceDefn	g_CppDummyNamespace;
		// Address of this data field is used as a well known pointer when search for the namespace
		// by name fails and it is necessary to indicate that function has not failed with a critical error.

TCppParser::TCppParser(TCppDatabase &cpp_database, TCppObjectsFactory &cpp_factory, TGrammar &cpp_grammar)
		: TGrammarBasedParser(&cpp_grammar), m_cpp_database(cpp_database), m_objects_factory(cpp_factory)
{
	// Verify error codes from the local enum.
	assert(cerr_base == perb_cpp_parser_base);
	assert(ccerr_max <= perb_cpp_parser_max);

	ResetScalarFields();
}

TCppParser::~TCppParser()
{
	// Publish all undefined labels right on the global namespace. Placement of all these labels will be
	// definitely wrong. But this is better than to leave them unpublished because otherwise objects,
	// that are already published in the database, i.e. the GOTO stmt objects, will have bad pointers.
	while (m_unplaced_labels.IsEmpty() == FALSE)
	{
		TCppItemBase *label = (TCppItemBase*)m_unplaced_labels.GetFirst();
		m_unplaced_labels.RemoveItem(label);
		m_cpp_database.GetGlobalNamespace().AddChildItem(label);
	}

	// Call the reset parser method explictly here. In fact, this method will be called again from the dector
	// of the base class once again when the virual methods of the current class will be already demoted.
	// This is why it is important to call it here.
	ResetParser();
}

void TCppParser::ResetScalarFields()
{
	// Reset the built in defns. They are not owned.
	for (int inx=0; inx<bltin_ids_max_plus1-bltin_ids_base; ++inx)
		m_built_in_defns[inx] = NULL;

	m_size_t_defn = NULL;
	m_type_info_defn = NULL;

	// There is no parent error at the beginning.
	m_parent_syntax_error = 0;

	// Reset misc pointers.
	m_curr_decl_type_spec = NULL;
	m_curr_enum_header = NULL;
	m_curr_template_header = NULL;
	m_curr_params_list = NULL;

	// Set the curr defns layer to the global namespace.
	m_defns_layer_owner = &m_cpp_database.GetGlobalNamespace();
	m_defns_layer_symbol = NULL;

	// Reset the current linkage specifier.
	m_curr_linkage_spec = NULL;
}

// Inherited virtual method.
void TCppParser::ResetParser()
{
	// Reset the base class. This is an important action because the cpp parser plays passive
	// role and acts as a callback handler of its base class, the grammar based parser.
	TGrammarBasedParser::ResetParser();

	// Reset tables and scalar fields.
	m_unplaced_labels.DeleteAll();
	ResetScalarFields();

	// Recycle the local look aside lists.
	m_dcltr_mdfs_look_aside.ClearTable();
	m_gen_purp_lists_look_aside.ClearTable();
	m_unsubm_ctv_infos_look_aside.ClearTable();
}

// Inherited virtual method.
bool TCppParser::Parse()
{
	m_cpp_database.ClearDatabase();
	return(TGrammarBasedParser::Parse());
}

// Inherited virtual method.
bool TCppParser::CheckGrammar() const
{
	if (m_root_grammar == NULL || m_root_grammar->grm_props.processing_result != grpr_full_success)
		return(FALSE);
	else if (m_root_grammar->grm_props.lang_type != lang_cpp)
		return(FALSE);
	else if (wcscmp(m_root_grammar->grm_props.cbk_handler_name, L"TCppParser") != 0)
		return(FALSE);

	// Success.
	return(TRUE);
}

// Inherited virtual method.
void TCppParser::ReviewInputSymbol(TSymbolStateItem *item)
{
	// The following objects can be passed to this method: terminal symbols, ignore symbols and error symbols.
	if (item->m_symbol < m_grammar->NumTerminals())
	{
		// Fill in the AppId value for this terminal symbol. This id is checked in several rule handlers.
		item->m_app_id = m_grammar->symbols[item->m_symbol].app_id;
	}
}

// Inherited virtual method.
void TCppParser::ReviewParsingAction(WORD &action)
{
	if (action >= actb_conflict && action < actb_nested_result)
	{
		// Check for conflicts that require attention from the callback handler.
		TGrammarConflict &cfct = m_grammar->conflicts[action-actb_conflict];
		if (cfct.expected_conflict_inx >= 0)
		{
			// Current grammar conflict belongs to some expected conflict.
			TExpectedGrammarConflict &curr_xpct = m_grammar->xpct_conflicts[cfct.expected_conflict_inx];
			switch (curr_xpct.xpct_conflict_app_id)
			{
				case xpct_ExpressionId_OR_NamedTypeSpecifier:		// X0
						{
							// -BUGBUG-
							// if (m_curr_decl_type_spec != NULL && m_curr_decl_type_spec->m_decl_type_spec_info.IsBaseTypeSpecPresent() == TRUE)
							{
								// The decl type spec is available and the base type is type specifier is present there.
								// Select the first conflicting action that will promote current name into the expression id.
								action = cfct.conflicting_actions[0];
							}
						}
						break;

				case xpct_SimpleTypeSpec_OR_TypeSpecifier:			// X1
						{
							action = cfct.conflicting_actions[0];
						}
						break;

				case xpct_DirectDeclarator_OR_PrimaryExpression:		// X2
						{
							action = cfct.conflicting_actions[1];
						}
						break;

				case xpct_Qual_OR_MoreQual:						// X5
						{
							action = cfct.conflicting_actions[0];
						}
						break;

				case xpct_Declarator_OR_ParameterDeclaration:		// X6
						{
							action = cfct.conflicting_actions[0];
						}
						break;

				case xpct_ExtendedId_OR_TemplateParams:			// X13
						{
							action = cfct.conflicting_actions[1];
						}
						break;

				case xpct_ClassKey_OR_TypeParameter:				// X17
						{
							if (IsTemplateParamsListContext() == TRUE)
							{
								// Consider this symbol as beginning of the type parameter. Use the shift action.
								action = cfct.conflicting_actions[0];
							}
							else
							{
								// Consider this symbol as class key in front of something. Use the reduce action.
								action = cfct.conflicting_actions[1];
							}
						}
						break;

				case xpct_RelationalExpr_OR_TemplateIdDataArg:		// X18
						{
							if (IsTemplateParamsListContext() == TRUE)
							{
								// Current position stays inside the list of template params or instantiation arguments.
								// Preset action value with reduce action as if this is the closing angle bracket.
								action = cfct.conflicting_actions[1];

								// Look for opening parenthesis that may hide the current closing angle bracket.
								TCppSymbolStateItem *sym = LastStackSymPtr();
								while (sym != NULL && sym != m_curr_params_list)
								{
									if (sym->m_app_id == sym_lpar)
									{
										// Consider this as comparison operation symbol. Use the shift action.
										action = cfct.conflicting_actions[0];
										break;
									}

									sym = PrevRuleSymPtr(sym);
								}
							}
							else
							{
								// This is simple case. Use the shift action.
								action = cfct.conflicting_actions[0];
							}
						}
						break;

				case xpct_PtrOperator_OR_NewTypeId:				// X22
				case xpct_PtrOperator_OR_NewDeclarator:			// X23
						{
							// Select action SHIFT.
							action = cfct.conflicting_actions[0];
						}
						break;

				case xpct_BigElse_OR_SmallElse:					// X28
						{
							// Standard asks for a fixed resolution. This is small if statement.
							action = cfct.conflicting_actions[0];
						}
						break;
			}
		}
	}
	else if (action >= actb_nested_result && action < actb_bogus_val)
	{
		// This is result of the nested parsing. It should be replaced with the regular parsing action.
		assert(FALSE);
	}
}

// Inherited virtual method.
TSymbolStateItem *TCppParser::AllocateSymbolStateItem(WORD irule)
{
	// Retrieve structure from the lookaside list or create the new one.
	TCppSymbolStateItem *item = (TCppSymbolStateItem*)PickUpRecycledSymbolStateItem();
	if (item == NULL)
	{
		// The look aside table is empty. Create new item with the app type.
		item = new TCppSymbolStateItem();
		if (item == NULL)
		{
			// Allocation has failed. This is critical error. It will be checked and reported by the grammar
			// based parser. This method is a callback. It should simply return the NULL value.
			return(NULL);
		}
	}

	// Ensure that symbol state object is in the empty state.
	assert(item->m_meaning == csm_none);

	// State structure for the new symbol is created.
	if (irule == 0xFFFF)
	{
		// This is creation of the state item for lexema. No additional processing is needed in this case.
		return(item);
	}

	// This is creation of the non terminal. Fill in the AppId value.
	TGrammarRule &rule = m_grammar->rules[irule];
	item->m_app_id = m_grammar->non_terminals[rule.non_term-NonTerminalsBase].app_id;

	//  Fill in the source area.
	if (rule.Length() > 0)
	{
		// Colect origins of the rule symbols into the origin of the new non terminal.
		PrepareNonTerminalArea(item->m_non_term_origin, GetStackSlotPtr(rule.Length()));
	}
	else
	{
		// The non terminal is created out of nothing. Use an empty area at the beginning of the first lexema
		// in the input queue as an origin area for this non terminal.
		PrEventHeader *curr_loc = GetCurrentLocation();
		assert(curr_loc != NULL);

		item->m_non_term_origin = *curr_loc;
		item->m_non_term_origin.AdjustToAreaBeg();
		item->m_non_term_origin.cn = 0;
	}

	// Do the high level processing.
	ProcessCppRule(*item, irule, rule);
	item->m_sym_meaning_ex = item->m_meaning;

	// Processing has succeeded in this or that way.
	return(item);
}

void TCppParser::RecycleSymbolStateItem(TSymbolStateItem *item)
{
	// Typecast the param pointer to the application type that is used on the current layer.
	TCppSymbolStateItem *cpp_item = (TCppSymbolStateItem*)item;

	// Do the app layer cleanup. Note that only some of the meanings own resources that should be released.
	switch (cpp_item->m_meaning)
	{
		//
		// Section 1.
		//

		case csm_simp_or_qual:
				{
					// Relase optional resource fields.
					cpp_item->m_simp_or_qual.ReleaseResources(this);
				}
				break;

		case csm_declaration:
				{
					// Declaration object should be not NULL.
					assert(cpp_item->m_declaration != NULL);
					delete cpp_item->m_declaration;
				}
				break;

		case csm_declarations_list:
				{
					// List of declarations should be not NULL. It is ok simply to delete this object because
					// it is a Cpp database item. Destructor will take care of its children if any.
					assert(cpp_item->m_declarations_list != NULL);
					delete cpp_item->m_declarations_list;
				}
				break;

		case csm_decl_type_spec:
				{
					// The decl type spec may contain resources in its name info field. Release them.
					cpp_item->m_decl_type_spec_info.m_dclt_unres_name_info.ReleaseResources(this);
				}
				break;

		case csm_linkage_spec_header:
				{
					// These symbols do not have resources but they can be nested. Procedure that pops the stack
					// of the nested linkage specs resets the meaning of this symbol. This means that symbols with
					// this meaning should not come to the recycle procedure except for exit with the crititcal error.
					assert(CheckForCriticalError() == TRUE && cpp_item == m_curr_linkage_spec);
					m_curr_linkage_spec = cpp_item->m_linkage_spec.m_parent_spec_layer;
				}
				break;

		//
		// Section 2.
		//

		case csm_declarator:
				{
					cpp_item->m_declarator_info.m_name_info.ReleaseResources(this);
					TCppDeclaratorModifiersArray *mdfs = cpp_item->m_declarator_info.m_modifiers;
					if (mdfs != NULL)
					{
						// Array of modifiers is present. Release resources in these modifiers if any.
						for (int imd=0; imd<mdfs->NumItems(); ++imd)
							mdfs->ItemRef(imd).ReleaseResources();

						// Push this array of modifiers into the look aside list.
						mdfs->SetNumItems(0);
						RecycleDeclaratorModifiersArray(mdfs);
					}
				}
				break;

		case csm_func_params_list:
				{
					// This symbol should be recycled only in case of the critical error.
					assert(CheckForCriticalError() == TRUE);
					if (cpp_item->m_func_params_info.m_params_list != NULL)
						RecycleGeneralPurposeList(cpp_item->m_func_params_info.m_params_list);

					// The symbol ParameterDeclarationClause has a csm_func_params_list meaning but its params list
					// is already removed from the stack. This means that the stack should be popped only sometimes.
					if (cpp_item == m_curr_params_list)
						m_curr_params_list = cpp_item->m_func_params_info.m_parent_params_symbol;
				}
				break;

		case csm_func_parameter:
				{
					// Parameter object should not be NULL.
					assert(cpp_item->m_func_parameter != NULL);
					delete cpp_item->m_func_parameter;
				}
				break;

		case csm_exceptions_list:
				{
					// The list should go the look aside list.
					assert(cpp_item->m_exceptions_list != NULL);
					RecycleGeneralPurposeList(cpp_item->m_exceptions_list);
				}
				break;

		case csm_initializer:
				{
					// Inititalizer object should not be NULL.
					assert(cpp_item->m_initializer_data != NULL);
					delete cpp_item->m_initializer_data;
				}
				break;

		//
		// Section 3.
		//

		case csm_namesp_header:
		case csm_class_header:
				{
					// These symbols should be recycled only in case of the critical error.
					assert(CheckForCriticalError() == TRUE);
					ExitCurrentDefnsLayer(cpp_item);
				}
				break;

		case csm_base_classes_list:
				{
					// List of classes should not be NULL.
					assert(cpp_item->m_base_classes_list != NULL);
					RecycleGeneralPurposeList(cpp_item->m_base_classes_list);
				}
				break;

		case csm_base_class_spec:
				{
					// Base class specification cannot be NULL.
					assert(cpp_item->m_base_class_spec != NULL);
					delete cpp_item->m_base_class_spec;
				}
				break;

		case csm_template_header:
				{
					// This type of symbols is always explicitly removed from the stack of symbols and states.
					// This means that it should be automatically recycled only in case of the critical error.
					assert(CheckForCriticalError() == TRUE);

					if (cpp_item->m_template_info.m_parameters_list != NULL)
						RecycleGeneralPurposeList(cpp_item->m_template_info.m_parameters_list);

					if (cpp_item->m_template_info.m_parent_template_header != NULL)
					{
						// Clear the child link field in the parent template header info.
						cpp_item->m_template_info.m_parent_template_header->m_template_info.m_child_template_header = NULL;
					}

					assert(cpp_item == m_curr_template_header);
					m_curr_template_header = cpp_item->m_template_info.m_parent_template_header;
				}
				break;

		case csm_template_params_list:
				{
					// This symbol should be recycled only in the case of the critical error.
					assert(CheckForCriticalError() == TRUE);

					if (cpp_item->m_template_params_info.m_params_list != NULL)
						RecycleGeneralPurposeList(cpp_item->m_template_params_info.m_params_list);

					assert(cpp_item == m_curr_params_list);
					m_curr_params_list = cpp_item->m_template_params_info.m_parent_params_symbol;
				}
				break;

		//
		// Section 4.
		//

		case csm_statement:
				{
					// The statement object should not be NULL.
					assert(cpp_item->m_statement != NULL);
					delete cpp_item->m_statement;
				}
				break;

		case csm_block_header:
				{
					// Delete the block only it is not yet published in the database.
					assert(cpp_item->m_block_info.m_block_header != NULL);
					if (cpp_item->m_block_info.m_block_object_published == FALSE)
						delete cpp_item->m_block_info.m_block_header;

					ExitCurrentDefnsLayer(cpp_item);
				}
				break;

		case csm_do_statement_body:
				{
					// The implicit block header pointer can be NULL. The loop body pointer cannot be NULL.
					if (cpp_item->m_do_stmt_info.m_do_stmt_block_header != NULL)
					{
						delete cpp_item->m_do_stmt_info.m_do_stmt_block_header;
						ExitCurrentDefnsLayer(cpp_item);
					}

					assert(cpp_item->m_do_stmt_info.m_do_stmt_loop_body != NULL);
					delete cpp_item->m_do_stmt_info.m_do_stmt_loop_body;
				}
				break;

		case csm_catch_blocks_seq:
				{
					// List of the catch blocks should be existing and it should be not empty.
					assert(cpp_item->m_catch_blocks_seq != NULL);
					delete cpp_item->m_catch_blocks_seq;
				}
				break;

		case csm_catch_block:
				{
					// As usual, the owned object should be not NULL.
					assert(cpp_item->m_catch_block != NULL);
					delete cpp_item->m_catch_block;
				}
				break;

		case csm_exception_param:
				{
					// The param should be not NULL.
					assert(cpp_item->m_exception_param != NULL);
					delete cpp_item->m_exception_param;
				}
				break;

		case csm_expression:
				{
					 // Expression object should be not NULL.
					assert(cpp_item->m_expression != NULL);
					delete cpp_item->m_expression;
				}
				break;
	}

	// Put a mark stating that Cpp layer resources are released.
	cpp_item->m_meaning = csm_none;
	cpp_item->m_app_id = 0;
	cpp_item->m_sym_meaning_ex = 0;

	// Base class will do the rest of the work.
	TGrammarBasedParser::RecycleSymbolStateItem(cpp_item);
}

const wchar_t *TCppParser::GetErrorText(int err_code)
{
	switch (err_code)
	{
		case cerr_error_adding_event_link:			return(L"Out of memory while adding parsing event link to the Cpp database object.");
		case cerr_ambig_defns_array_item:			return(L"   Ambiguous definition %d: \"%s\" (%s).");

		case cerr_dts_decl_prefix_dupe:				return(L"Type specifier can have only one prefix. The prefix \"%s\" is already present. Second prefix is ignored.");
		case cerr_dts_func_attr_dupe:				return(L"Function attribute \"%s\" is already present in the type specifier.");
		case cerr_dts_strg_class_dupe:				return(L"Storage class \"%s\" is already present in the type specifier. Second storage class specifier is ignored.");
		case cerr_dts_const_spec_dupe:				return(L"Const specifier is already present in the type specifier. Second const specifier is ignored.");
		case cerr_dts_volatile_spec_dupe:			return(L"Volatile specifier is already present in the type specifier. Second volatile specifier is ignored.");
		case cerr_dts_bad_built_in_spec:				return(L"Data type is already present in the type specifier. Built in type specifier is ignored.");
		case cerr_dts_bad_named_type:				return(L"Data type is already present in the type specifier. The name of the type is ignored.");
		case cerr_dts_bad_elab_type_spec:			return(L"Data type is already present in the type specifier. Elaborated type specifier is ignored.");
		case cerr_dts_bad_enum_or_class:			return(L"Data type is already present in the type specifier. %s definition is ignored.");
		case cerr_dts_throws_type_duplication:		return(L"Possible exception type \"%s\" is already present in the list of the possible exception types.");

		case cerr_sdecl_bogus_typedef_prefix:		return(L"Typedef specifier is not applicable in this context.");
		case cerr_sdecl_bogus_func_attr:			return(L"Function attribute \"%s\" is not applicable in this context.");
		case cerr_sdecl_bogus_strg_class:			return(L"Storage class specifier \"%s\" is not applicable in this context.");
		case cerr_sdecl_bogus_cv_spec:				return(L"Const and volatile specifiers are not applicable in this context.");
		case cerr_sdecl_bogus_friend_spec:			return(L"Friend specifier is not applicable %s.");
		case cerr_sdecl_bogus_built_in_type:			return(L"Built in type cannot be used in the stand alone type specifier.");
		case cerr_sdecl_bogus_named_type:			return(L"Named type cannot be used in the stand alone type specifier.");
		case cerr_sdecl_bogus_exinst_hdr_missing:	return(L"An explicit template instantiation header is missing.");
		case cerr_sdecl_bogus_type_missing:			return(L"Information about the base type is missing.");

		case cerr_sdecl_bogus_template_spec:		return(L"Template specialization is not a data type and it cannot be used in the type specifier.");

		case cerr_context_extern_with_initializer:		return(L"Initializer can not be used with the extern variable.");
		case cerr_context_bad_initializer:			return(L"Initializer is not applicable in the current context.");
		case cerr_context_bad_friend:				return(L"Friend specifier is not applicable outside of the class or class template context.");
		case cerr_context_name_occupied:			return(L"The name \"%s\" is already defined on the current layer as %s.");

		case cerr_bltin_sign_spec_twice:				return(L"The \"signed\" or \"unsigned\" specifier is already pesent.");
		case cerr_bltin_size_spec_twice:				return(L"The \"short\" or \"long\" specifier is already pesent.");
		case cerr_bltin_sign_spec_with_bad_type:		return(L"The \"signed\"/\"unsigned\" specifier is not compatible with selected primary type.");
		case cerr_bltin_size_spec_with_bad_type:		return(L"The \"short\"/\"long\" specifier is not compatible with selected primary type.");
		case cerr_bltin_bad_type_for_sign_spec:		return(L"Primary type is not compatible with existing \"signed\"/\"unsigned\" specifier.");
		case cerr_bltin_bad_type_for_size_spec:		return(L"Primary type is not compatible with existing \"short\"/\"long\" specifier.");
		case cerr_bltin_type_already_present:		return(L"Primary type specifier is already present.");
		case cerr_bltin_unknown_symbol_app_id:		return(L"Unknown built in type specifier. The specifier is ignored.");

		case cerr_enum_bad_template_args:			return(L"Template arguments are not allowed in the enum definition. Template arguments are ignored.");
		case cerr_enum_base_type_non_numeric:		return(L"Base type of the enum cannot be converted to the numeric type.");
		case cerr_enum_base_type_enum_type:		return(L"Enum type cannot be used as the base type for other enum.");
		case cerr_enum_base_type_func_type:		return(L"Function type cannot be used as the base type for enum.");
		case cerr_enum_member_expr_not_const:		return(L"Expression in the defintion of the enum member is not constant.");
		case cerr_enum_member_expr_not_numeric:	return(L"Expression in the defintion of the enum member cannot be converted into the numeric value.");

		case cerr_struct_member_init_not_applic:		return(L"Constant initializer is not applicable to this type of the %s member.");
		case cerr_struct_bft_no_decl_type_spec:		return(L"Named bit field member requires the type specifier.");
		case cerr_struct_bft_bad_decl_prefix:			return(L"Type specifier prefix \"%s\" is not applicable to the bit field member.");
		case cerr_struct_bft_bad_func_attr:			return(L"Function attribute \"%s\" %s not applicable to the bit field member.");
		case cerr_struct_bft_bad_strg_class:			return(L"Storage class \"%s\" is not applicable to the bit field member. Storage class is ignored.");
		case cerr_struct_bft_base_type_not_numeric:	return(L"Bit field member requires integral or enum type.");
		case cerr_struct_bft_base_type_func_type:	return(L"Function type is not allowed as a type for the bit field members.");
		case cerr_struct_base_class_duplication:		return(L"Base class \"%s\" is already present in the list of base classes.");

		case cerr_special_func_not_in_class:			return(L"Special function outside of the class context.");

		case cerr_label_is_undefined:				return(L"Label \"%s\" is not defined. Label is placed at the end of the function body.");
		case cerr_label_already_defined:				return(L"Label with the name \"%s\" is already existing. Label with an altered name was added.");
		case cerr_case_label_not_const:				return(L"Case label does not contain a constant expression.");
		case cerr_case_label_not_numeric:			return(L"Constant expression in the case label cannot be converted into the numeric value.");

		case cerr_condition_bad_qual_path:			return(L"Qualification path is not allowed in the definition of condition. Qualification path is ignored.");
		case cerr_condition_not_simple_name:		return(L"Condition should be a simple name. Condition with a substed name is created.");
		case cerr_condition_defines_array:			return(L"Current context does not allow array variables because arrays cannot be inited with expressions.");

		case cerr_expr_lvalue_missing:				return(L"It is not possible to assign the value to the first operand of the assignment expression.");
		case cerr_expr_const_missing:				return(L"Expression is not constant.");
		case cerr_expr_this_outside_of_func:			return(L"Operand \"this\" cannot be used outside of the function.");
		case cerr_expr_this_in_namesp_func:			return(L"Operand \"this\" cannot be used in functions on the namespace layer.");
		case cerr_expr_this_in_static_method:		return(L"Operand \"this\" cannot be used in static methods of %s.");
		case cerr_expr_base_in_static_method:		return(L"Names of the base classes cannot be used in the static methods.");

		case cerr_expr_typeid_type_info_missing:		return(L"Error processing the typeid operation. The \"std::type_info\" structure is not defined.");
		case cerr_expr_typeid_type_info_problem:		return(L"Error processing the typeid operation. Check the \"std::type_info\" definition. It should be an unambiguous struct object.");

		case cerr_strings_seq_subt_mismatch:		return(L"String subtype mismatch in the sequence of literal strings. Second string is discarded.");

		case cerr_object_already_defined:			return(L"Object with the name \"%s\" is already existing. %s with an altered name \"%s\" was created.");

		case cerr_elab_defn_in_template_inst:		return(L"%s definitions are not allowed inside the class template instantiations.");
		case cerr_elab_defn_on_upper_layer:			return(L"%s definitions are not allowed on the upper layers of the hierarchy.");
		case cerr_elab_defn_names_conflict:			return(L"Object with the name \"%s\" is already existing. %s definition with an altered name was created.");
		case cerr_elab_defn_already_existing:		return(L"%s definition \"%s\"%s is already existing. Definition with an altered name was created.");
		case cerr_elab_defn_on_lower_layer:			return(L"%s definitions are not allowed inside the inner layers of the hierarchy.");
		case cerr_elab_defn_on_unrelated_layer:		return(L"%s definitions are not allowed on the unrelated layers of the hierarchy.");

		case cerr_data_field_in_template_inst:		return(L"Data field definitions are not allowed inside the class template instantiations.");
		case cerr_data_field_on_upper_layer:			return(L"Data field definitions are not allowed on the upper layers of the hierarchy.");
		case cerr_data_field_names_conflict:			return(L"Object with the name \"%s\" is already existing. Data field with an altered name was created.");
		case cerr_data_field_name_duplication:		return(L"Data field with the name \"%s\" is already existing. Data field with an altered name was created.");
		case cerr_data_field_lo_layer_extern:			return(L"Extern data fields are not allowed inside the nested namespaces.");
		case cerr_data_field_lo_layer_mutable:		return(L"Mutable data fields are not allowed inside the nested %s.");
		case cerr_data_field_on_lower_layer:			return(L"Data field definitions are not allowed on the lower layers of the hierarchy.");
		case cerr_data_field_lo_namesp_dupe:		return(L"Data field \"%s\" is already existing in the nested namespace. Data field with an altered name was created.");
		case cerr_data_field_lo_class_inst_dupe:		return(L"Data field \"%s\" is defined as an instance data field in the nested %s. Data field with an altered name was created.");
		case cerr_data_field_lo_class_sttc_dupe:		return(L"Data field \"%s\" is defined as a static data field in the nested %s. Data field with an altered name was created.");
		case cerr_data_field_on_unrelated_layer:		return(L"Data field definitions are not allowed on the unrelated layers of the hierarchy.");

		case cerr_func_obj_friend_proto_ambig:		return(L"Name of the prototype is ambiguous between %d objects.");
		case cerr_func_obj_friend_proto_unappr:		return(L"Object with the name \"%s\" is existing but it is not a name of the %s.");
		case cerr_func_obj_in_template_inst:			return(L"%s are not allowed inside the class template instantiations.");
		case cerr_func_obj_on_upper_layer:			return(L"%s are not allowed on the upper layers of the hierarchy.");
		case cerr_func_obj_names_conflict:			return(L"Object with the name \"%s\" is already existing. %s with an altered name was created.");
		case cerr_func_obj_name_duplication:		return(L"%s with the name \"%s\" is already existing. %s with an altered name was created.");
		case cerr_func_obj_on_lower_layer:			return(L"%s are not allowed inside the inner layers of the hierarchy.");
		case cerr_func_obj_on_unrelated_layer:		return(L"%s are not allowed on the unrelated layers of the hierarchy.");

		case cerr_defns_search_name_ambig:		return(L"The name \"%s\" is ambiguous between %d definitions or buckets with overloaded definitions.");
		case cerr_defns_search_not_namespace:		return(L"The name \"%s\" does not represent the name of the namespace in the current context.");
		case cerr_defns_search_not_named_type:		return(L"The name \"%s\" cannot be used as a data or function type.");
		case cerr_defns_search_cttp_template_args:	return(L"Template arguments should not be used used with default value of the template template param. Template arguments are ignored.");
		case cerr_defns_search_cttp_no_overload:		return(L"None of the available class templates with the name \"%s\" match the current set of template parameters.");
		case cerr_defns_search_cttp_not_template:	return(L"The name \"%s\" is not the name of the class template.");
		case cerr_defns_search_not_class_member:	return(L"The name \"%s\" is not a field in the \"%s\".");
		case cerr_defns_search_not_operand:		return(L"Object with the name \"%s\" cannot be used as an operand in the expression.");

		case cerr_ptr_operator_not_struct:			return(L"Name \"%s\" is not a name of the structure type.");
		case cerr_context_not_class_name:			return(L"Name \"%s\" is not a name of the class type.");

		case cerr_defns_search_bad_template_args:	return(L"Template arguments are used with a non template object. Template arguments are ignored.");
		case cerr_defns_search_unknown_name:		return(L"The name \"%s\" is not defined in the current context.");

		case cerr_template_hdr_not_allowed:			return(L"Template header is not allowed in front of %s. Template header is ignored.");
		case cerr_template_hdr_unexpected:			return(L"Unexpected template header. Template header is ignored.");
		case cerr_template_hdr_with_empty_defn1:	return(L"Template header is not followed by template definition.");
		case cerr_template_hdr_with_empty_defn2:	return(L"Explicit template instantiation header is not followed by template instantiation.");

		case cerr_decltor_array_dim_not_numeric:	return(L"Arithmetic expression does not have a numeric type.");
		case cerr_decltor_array_dim_not_constant:	return(L"Arithmetic expression is not constant.");
		case cerr_decltor_array_negative_dim:		return(L"Size of the array dimension cannot be negative (%I64d).");
		case cerr_decltor_array_zero_first_dim:		return(L"Zero size of the first array dimension cannot be used with this type of declarator.");
		case cerr_decltor_array_zero_sec_dim:		return(L"Size of the secondary array dimension cannot be zero.");
		case cerr_decltor_array_elem_func:			return(L"Element of the array cannot be a function.");

		case cerr_decltor_base_func_type_cv:		return(L"Function type cannot have const/volatile specifiers in front of the declarator.");
		case cerr_decltor_func_retval_func:			return(L"Function cannot return a function.");
		case cerr_decltor_result_func_type:			return(L"The data type is required. Function type is not applicable in this context.");

		case cerr_case_label_dupl_default:			return(L"Duplication of the default case label.");
		case cerr_case_label_dupl_value:			return(L"Duplication of the case label \"%I64d:\".");
		case cerr_case_label_outside_switch:			return(L"Case label stays outside of the switch statement.");

		case cerr_param_name_not_ident:			return(L"Name of the parameter is not an identifier.");
		case cerr_param_name_bad_qual_path:		return(L"Qualification path is not allowed in the name of the parameter. Qualification path is ignored.");
		case cerr_param_name_duplication:			return(L"Parameter name duplication.");

		case cerr_base_type_missing:				return(L"The type specifier is missing.");
		case cerr_struct_template_params_missing:	return(L"Template parameters are missing in the definition of the %s template.");

		case cerr_templ_spec_no_templ_header:		return(L"Template header is missing in front of the %s template specialization.");
		case cerr_templ_spec_expl_inst_header:		return(L"Explicit template instantiation header is not allowed in front of the %s template specialization.");
		case cerr_templ_spec_base_classes:			return(L"Base classes are not allowed in the %s template specialization. Base classes are ignored.");

		case cerr_elab_fwd_enum_templid:			return(L"Template arguments are not allowed in the forward declaration of the enum. Template arguments are ignored.");
		case cerr_elab_fwd_templ_params_missing:	return(L"Template parameters are missing in the forward declaration of the %s template.");
		case cerr_elab_fwd_find_name_ambig:		return(L"Name in the forward declaration is ambiguous between %d definitions or buckets of overloaded definitions.");
		case cerr_elab_fwd_names_conflict:			return(L"Object with the name \"%s\" is existing but it is not a name of the %s.");

		case cerr_elab_fwd_in_template_inst:			return(L"%s forward declarations are not allowed inside the class template instantiations.");
		case cerr_elab_fwd_on_upper_class_layer:	return(L"%s forward declarations are not allowed on the upper class layers of the definitions hierarchy.");
		case cerr_elab_fwd_on_lower_layer:			return(L"%s forward declarations are not allowed inside the inner layers of the definitions hierarchy.");
		case cerr_elab_fwd_on_unrelated_layer:		return(L"%s forward declarations are not allowed on the unrelated layers of the definitions hierarchy.");

		case cerr_friend_rec_obj_on_same_layer:		return(L"Friendly object with the name \"%s\" resides on the current layer.");

		case cerr_bft_non_const_expr:				return(L"Expression in the %s width is not constant.");
		case cerr_bft_non_numeric_expr:			return(L"Constant expression in the %s width cannot be converted into the numeric value.");
		case cerr_bft_negative_const_expr:			return(L"Width of the %s cannot be negative (%I64d).");
		case cerr_bft_zero_const_expr:				return(L"Bit field member type should not have zero width.");
		case cerr_bft_too_big_const_expr:			return(L"Value of the %s width is too big (%I64d).");

		case cerr_data_field_bad_friend:				return(L"Friend specifier is not applicable to the definition of the data field.");
		case cerr_data_field_bad_func_attr:			return(L"Function attribute \"%s\" is not applicable to the definition of the data field.");
		case cerr_data_field_bad_strg_class:			return(L"Storage class specifier \"%s\" is not applicable to the data field %s. Storage class specifier is ignored.");
		case cerr_data_field_base_type_missing:		return(L"Type specifier is missing in the definition of the data field.");
		case cerr_data_field_bad_template_args:		return(L"Template arguments are not allowed in the definition of the data field. Template arguments are ignored.");
		case cerr_data_field_diff_linkage_specs1:		return(L"Already existing data field with the name \"%s\" does not have linkage spec.");
		case cerr_data_field_diff_linkage_specs2:		return(L"Already existing data field with the name \"%s\" has different linkage spec: \"%s\".");
		case cerr_data_field_diff_data_types:			return(L"Already existing data field with the name \"%s\" has different data type.");

		case cerr_typedef_bad_func_attr:			return(L"Function attribute \"%s\" is not applicable to the typedef definition.");
		case cerr_typedef_bad_strg_class:			return(L"Storage class specifier \"%s\" is not applicable to the typedef definition.");
		case cerr_typedef_bad_qual_path:			return(L"Qualification path is not allowed in the typedef definition. Qualification path is ignored.");
		case cerr_typedef_bad_template_args:		return(L"Template arguments are not allowed in the typedef definition. Template arguments are ignored.");

		case cerr_func_dts_bad_typedef:				return(L"Type specifier prefix \"typedef\" is not applicable to the %s.");
		case cerr_func_dts_bad_friend_context:		return(L"Type specifier prefix \"friend\" cannot be used outside of the class or class template context.");
		case cerr_func_dts_bad_virtual_context:		return(L"Function attribute \"virtual\" is applicable only to methods of classes and class templates.");
		case cerr_func_dts_bad_virtual_ctor:			return(L"Constructor of the %s cannot be defined virtual. The function attrubute \"virtual\" is ignored.");
		case cerr_func_dts_bad_no_ctor_explicit:		return(L"Function attribute \"explicit\" can be used only with constructors.");
		case cerr_func_dts_bad_static_ctor:			return(L"Constructor cannot be defined static. The specifier is ignored.");
		case cerr_func_dts_bad_static_dector:		return(L"Destructor cannot be defined static. The specifier is ignored.");
		case cerr_func_dts_bad_static_ovld_conv:		return(L"Overloaded conversion cannot be defined static. The specifier is ignored.");
		case cerr_func_dts_extern_in_class:			return(L"Storage class specifier \"extern\" cannot be used in the context of %s.");
		case cerr_func_dts_extern_with_method:		return(L"Storage class specifier \"extern\" cannot be used with the method of %s.");
		case cerr_func_dts_bad_strg_class:			return(L"Storage class specifier \"%s\" is not applicable to the %s.");

		case cerr_func_props_diff_linkage_specs1:		return(L"Already existing %s with the name \"%s\" does not have a linkage spec.");
		case cerr_func_props_diff_linkage_specs2:		return(L"Already existing %s with the name \"%s\" has different linkage spec: \"%s\".");
		case cerr_func_props_diff_access_levels:		return(L"Already existing %s with the name \"%s\" has different access level: \"%s\".");
		case cerr_func_props_unexpected_func_attr:	return(L"Function attribute \"%s\" is not present in the existing %s. Object with an altered name was created.");
		case cerr_func_props_unexpected_this_spec:	return(L"Specifier \"%s\" is not present in the existing %s. Object with an altered name was created.");
		case cerr_func_props_missing_this_spec:		return(L"Parameter \"this\" of the already existing %s is marked with the specifier \"%s\". Object with an altered name was created.");

		case cerr_func_decl_base_type_missing:		return(L"Return value type is missing in the %s.");
		case cerr_func_decl_func_params_missing:	return(L"Parameters of the %s are missing.");
		case cerr_func_decl_namesp_func_this_spec:	return(L"Specifier \"%s\" is not allowed on the namespace layer. The specifier is ignored.");
		case cerr_func_decl_static_method_this_spec:	return(L"Specifier \"%s\" is not allowed with static members of %s. The specifier is ignored.");

		case cerr_func_decl_templ_params_missing:	return(L"Template parameters are missing in the %s.");
		case cerr_func_decl_function_unxpct_static:	return(L"%s \"%s\" is not defined as static. Object with an altered name was created.");
		case cerr_func_decl_method_unxpct_static:	return(L"%s \"%s\" is defined as an instant method the %s. Object with an altered name was created.");
		case cerr_func_decl_ret_value_mismatch:		return(L"Return value type of the current definition does not match the return value type of the prototype. Object with an altered name was created.");

		case cerr_typeless_func_bad_const_spec:		return(L"Const specifier is not allowed in front of the %s. Const specifier is ignored.");
		case cerr_typeless_func_bad_volatile_spec:	return(L"Volatile specifier is not allowed in front of the %s. Volatile specifier is ignored.");
		case cerr_typeless_func_bad_base_type:		return(L"Type specifier is not allowed with the %s. Type specifier is ignored.");

		case cerr_typeless_func_ctor_this_spec:		return(L"Specifier \"%s\" is not allowed after the params of the constructor. The specifier is ignored.");
		case cerr_typeless_func_dector_this_spec:		return(L"Specifier \"%s\" is not allowed after empty params spec of the destructor. The specifier is ignored.");
		case cerr_typeless_func_dector_bad_params:	return(L"List of parameters of the destructor is not empty. Parameters are ignored.");
		case cerr_typeless_func_oconv_bad_params:	return(L"List of parameters of the overloaded conversion is not empty. Parameters are ignored.");
		case cerr_typeless_func_no_templ_params:	return(L"Template parameters are missing in the %s.");

		case cerr_typeless_func_dector_in_block:		return(L"Destructor prototypes are not allowed inside the body of the function. Error object was created to the namespace layer.");
		case cerr_typeless_func_dector_on_namesp:	return(L"Destructors are not allowed in namespaces.");
		case cerr_typeless_func_dector_bad_name:	return(L"Name of the destructor \"%s\" does not match the name of the %s.");
		case cerr_typeless_func_oconv_in_block:		return(L"Overloaded conversion prototypes are not allowed inside the body of the function. Error object was created to the namespace layer.");
		case cerr_typeless_func_oconv_on_namesp:	return(L"Overloaded conversion functions are not allowed in namespaces.");

		case cerr_defns_layer_name_ambig:			return(L"Name of the definitions layer is ambiguous between %d definitions or buckets of overloaded definitions.");
		case cerr_defns_layer_bad_template_args:	return(L"Template arguments cannot be used with name of the %s. Template arguments are ignored.");
		case cerr_defns_layer_bad_template1:		return(L"Name of the class template cannot be used as a name of the layer without template header and without template arguments.");
		case cerr_defns_layer_bad_template2:		return(L"Unable to resolve the class template overload using template parameters from the template header.");
		case cerr_defns_layer_name_unuseable:		return(L"Object with the name \"%s\" cannot be used as a name of the definitions layer.");
		case cerr_defns_layer_name_unknown:		return(L"Unknown definition layer.");

		case cerr_typeless_decl_no_params_ctor:		return(L"Constructor must have list of parameters.");
		case cerr_typeless_decl_no_params_dector:	return(L"Destructor must have empty list of parameters.");
		case cerr_typeless_decl_no_params_conv:		return(L"Overloaded conversion must have empty list of parameters.");
		case cerr_typeless_decl_bad_mdfs_ctor:		return(L"Unexpected modifier%s in the constructor declarator. Constructor should have only the list of parameters.");
		case cerr_typeless_decl_bad_mdfs_dector:	return(L"Unexpected modifier%s in the destructor declarator. Destructor must have an empty list of parameters.");
		case cerr_typeless_decl_bad_mdfs_conv:		return(L"Unexpected modifier%s in the overloaded conversion declarator. Overloaded converstion must have an empty list of parameters.");

		case cerr_oom_decltor_cvpr_modifier:		return(L"Out of memory while allocating storage for const/volatile/prt/ref type modifier.");
		case cerr_oom_decltor_array_modifier:		return(L"Out of memory while allocating storage for array type modifier.");
		case cerr_oom_decltor_func_modifier:		return(L"Out of memory while allocating storage for function type modifier.");
		case cerr_oom_decltor_modifiers_layer:		return(L"Out of memory while allocating tempopary storage for declarator modifiers.");

		//
		// Critical errors.
		//

		case ccerr_misc_error_in_grammar:			return(L"Cpp grammar error. Callback handler is not compatible with the grammar.");
		case ccerr_misc_not_implemented:			return(L"The feature is not implemented yet: %s.");
		case ccerr_missing_non_term_app_id:		return(L"App id is missing for non terminal \"%s\" (sym=%hd).");
		case ccerr_unexpected_non_term_app_id:		return(L"Unexpected non terminal app id %hd for \"%s\" (sym=%hd).");
		case ccerr_unexpected_gram_rule_app_id:	return(L"Rule R%hd for non term \"%s\" (sym=%hd) has unexpected rule app_id: %hd.");
		case ccerr_unexpected_rule_sym_app_id:		return(L"Rule R%hd for non term \"%s\" (sym=%hd) has symbol with unexpected app_id: %hd.");
		case ccerr_unexpected_rule_sym_meaning:	return(L"Rule R%hd for non term \"%s\" (sym=%hd) has symbol (isym=%d) with unexpected meaning.");

		case ccerr_unhandled_template_header:		return(L"Template header was not properly handled.");

		case ccerr_key_id_picking_problem:			return(L"Unable to retrieve indexing key id for \"%.*s\".");
		case ccerr_oom_storing_name_string:		return(L"Unable to store the name string. Length of the string in chars: %d.");
		case ccerr_oom_generating_object_name:		return(L"Out of memory while generating %s name.");
		case ccerr_oom_setting_alt_name:			return(L"Out of memory while creating alternative object name.");

		case ccerr_oom_decltor_ptr_to_func:			return(L"Out of memory while allocating storage for implicit pointer to the function indirection layer.");
		case ccerr_oom_general_purpose_list:		return(L"Out of memory while allocating tempopary general purpose list.");
		case ccerr_oom_unsubm_ctv_event_info:		return(L"Out of memory while allocating temp storage for unsubmitted parsing event.");
		case ccerr_oom_combining_literal_strings:		return(L"Out of memory while allocating space for the sequence of literal strings.");
		case ccerr_oom_storing_literal_string:		return(L"Unable to store the literal string. Length of the string in chars: %d.");
		case ccerr_oom_creating_cpp_object:			return(L"Out of memory while creating cpp database object %s.");
		case ccerr_oom_creating_unres_object:		return(L"Out of memory while creating unresolved name object.");
	}

	// Unknown error code.
	return(L"TCppParser: Unknown error/warning code.");
}

TCppSymbolStateItem *TCppParser::GetFirstTemplateHeaderSymbol()
{
	// Look for the topmost template header that still belongs to the current defns layer.
	TCppSymbolStateItem *unproc_header = NULL;
	TCppSymbolStateItem *hdr_ptr = m_curr_template_header;
	while (hdr_ptr != NULL && hdr_ptr->m_template_info.m_header_layer_owner == m_defns_layer_owner)
	{
		unproc_header = hdr_ptr;
		hdr_ptr = hdr_ptr->m_template_info.m_parent_template_header;

		// Ensure correctness of the child template link.
		assert(hdr_ptr == NULL || hdr_ptr->m_template_info.m_child_template_header == unproc_header);
	}

	return(unproc_header);
}

TCppCodeEntryPointDefn *TCppParser::GetOwningFunctionForCodeArea()
{
	assert(m_defns_layer_owner != NULL);
	if (m_defns_layer_owner->IsBlock() == FALSE)
	{
		// Current context is not a function context.
		return(NULL);
	}

	// Every block area should be a child of the function object.
	TCppItemBase *layer_owner = m_defns_layer_owner;
	while (layer_owner != NULL)
	{
		if (layer_owner->IsFunctionFamily() == TRUE)
		{
			// The function is available.
			return((TCppCodeEntryPointDefn*)layer_owner);
		}
		else if (layer_owner->IsBlock() == FALSE)
		{
			break;
		}

		// Shift to the upper layer.
		layer_owner = layer_owner->ItemParent();
	}

	// This is a structural problem in the code tree.
	assert(FALSE);
	return(NULL);
}

ID TCppParser::ReportSyntaxError(TSymbolStateItem *err_ctx_sym, TCppErrorCode err_code, ...)
{
	// Pick up the current location and the error message format.
	PrEventHeader *ctx = (err_ctx_sym != NULL) ? &(err_ctx_sym->GetOriginHeader()) : GetCurrentLocation();
	const wchar_t *err_msg_fmt = GetErrorText(err_code);

	// Format the error message.
	va_list vargs;
	va_start(vargs, err_code);
	wchar_t err_msg_buff[2048+80];
	vswprintf(err_msg_buff, 2048, err_msg_fmt, vargs);
	va_end(vargs);

	// Call the worker method in the mid lev scanner.
	return(m_scanner->AddErrorWarning(ctx, m_parent_syntax_error, lerrc_syntax_err, err_code, err_msg_buff));
}

ID TCppParser::ReportSyntaxError(TSymbolStateItem *err_ctx_sym, TCppItemBase *related_object, TCppErrorCode err_code, ...)
{
	// Pick up the current location and the error message format.
	PrEventHeader *ctx = (err_ctx_sym != NULL) ? &(err_ctx_sym->GetOriginHeader()) : GetCurrentLocation();
	const wchar_t *err_msg_fmt = GetErrorText(err_code);

	// Format the error message.
	va_list vargs;
	va_start(vargs, err_code);
	wchar_t err_msg_buff[2048+80];
	vswprintf(err_msg_buff, 2048, err_msg_fmt, vargs);
	va_end(vargs);

	// Call the worker method in the mid lev scanner to store the error.
	ID error_event_id = m_scanner->AddErrorWarning(ctx, m_parent_syntax_error, lerrc_syntax_err, err_code, err_msg_buff);

	// Add info about the error to the cpp item if needed.
	if (related_object != NULL)
		AddCtvEventRef(ctx, related_object, clink_error, error_event_id);

	return(error_event_id);
}

ID TCppParser::ReportSyntaxError(PrEventHeader &err_ctx, TCppItemBase *related_object, TCppErrorCode err_code, ...)
{
	// Pick up the error message format.
	const wchar_t *err_msg_fmt = GetErrorText(err_code);

	// Format the error message.
	va_list vargs;
	va_start(vargs, err_code);
	wchar_t err_msg_buff[2048+80];
	vswprintf(err_msg_buff, 2048, err_msg_fmt, vargs);
	va_end(vargs);

	// Call the worker method in the mid lev scanner to store the error.
	ID error_event_id = m_scanner->AddErrorWarning(&err_ctx, m_parent_syntax_error, lerrc_syntax_err, err_code, err_msg_buff);

	// Add info about the error to the cpp item if needed.
	if (related_object != NULL)
		AddCtvEventRef(&err_ctx, related_object, clink_error, error_event_id);

	return(error_event_id);
}

void TCppParser::RaiseCriticalCppError(TCppCriticalErrorCode critical_error_code, ...)
{
	// Pick up the current location and the error message format.
	PrEventHeader *ctx = GetCurrentLocation();
	const wchar_t *err_msg_fmt = GetErrorText(critical_error_code);

	// Format the error message.
	va_list vargs;
	va_start(vargs, critical_error_code);
	wchar_t err_msg_buff[2048+80];
	vswprintf(err_msg_buff, 2048, err_msg_fmt, vargs);
	va_end(vargs);

	// Call the worker method in the base class.
	TGenericParser::RaiseCriticalError(ctx, critical_error_code, L"C/Cpp critical error.", err_msg_buff);

	// Fill in the buffer that will be displayed on the parsing console.
	wcsncpy(m_critical_error_details, err_msg_buff, CRITICAL_ERROR_DTLS_LEN);
	m_critical_error_details[CRITICAL_ERROR_DTLS_LEN-1] = 0;
}

void TCppParser::RaiseNotImplementedError(const wchar_t *details_msg)
{
	RaiseCriticalCppError(ccerr_misc_not_implemented, details_msg);
}

void TCppParser::RaiseUnexpectedGrammarRuleAppIdError(WORD irule)
{
	TGrammarRule &rule = m_grammar->rules[irule];
	TNonTerminalSymbol &non_term = m_grammar->non_terminals[rule.non_term-NonTerminalsBase];
	RaiseCriticalCppError(ccerr_unexpected_gram_rule_app_id, irule, non_term.symbol_name, rule.non_term, rule.rule_app_id);
}

void TCppParser::RaiseUnexpectedRuleSymbolAppIdError(WORD irule, TSymbolStateItem *sym)
{
	TGrammarRule &rule = m_grammar->rules[irule];
	TNonTerminalSymbol &non_term = m_grammar->non_terminals[rule.non_term-NonTerminalsBase];
	RaiseCriticalCppError(ccerr_unexpected_rule_sym_app_id, irule, non_term.symbol_name, rule.non_term, sym->m_app_id);
}

void TCppParser::RaiseUnexpectedRuleSymbolMeaningError(WORD irule, int isym)
{
	TGrammarRule &rule = m_grammar->rules[irule];
	TNonTerminalSymbol &non_term = m_grammar->non_terminals[rule.non_term-NonTerminalsBase];
	RaiseCriticalCppError(ccerr_unexpected_rule_sym_meaning, irule, non_term.symbol_name, rule.non_term, isym);
}

void TCppParser::ReportUnexpectedTemplateHeaders(const wchar_t *non_templateable_obj_type, TCppSymbolStateItem *leftmost_unexpected_header)
{
	// Report errors on all template headers of the current layer and mark them as processed.
	TCppSymbolStateItem *sym = (leftmost_unexpected_header != NULL) ? leftmost_unexpected_header : GetFirstTemplateHeaderSymbol();
	while (sym != NULL)
	{
		ReportSyntaxError(sym, cerr_template_hdr_not_allowed, non_templateable_obj_type);
		sym->m_template_info.m_header_processed = TRUE;
		sym = sym->m_template_info.m_child_template_header;
	}
}

void TCppParser::ReportUnprocessedTemplateHeaders(TCppSymbolStateItem *leftmost_unprocessed_header_symbol)
{
	TCppSymbolStateItem *sym = leftmost_unprocessed_header_symbol;
	while (sym != NULL)
	{
		ReportSyntaxError(sym, cerr_template_hdr_unexpected);
		sym->m_template_info.m_header_processed = TRUE;
		sym = sym->m_template_info.m_child_template_header;
	}
}

void TCppParser::ReportEmptyDefinitionTemplateHeaders()
{
	// Report errors on all template headers of the current layer and mark them as processed.
	TCppSymbolStateItem *sym = GetFirstTemplateHeaderSymbol();
	while (sym != NULL)
	{
		if (sym->m_template_info.m_parameters_list != NULL)
			ReportSyntaxError(sym, cerr_template_hdr_with_empty_defn1);
		else ReportSyntaxError(sym, cerr_template_hdr_with_empty_defn2);

		sym->m_template_info.m_header_processed = TRUE;
		sym = sym->m_template_info.m_child_template_header;
	}
}

void TCppParser::ReportAmbiguousDefnsArray(TSymbolStateItem *err_ctx_sym, ID main_error_msg_id, TCppDefnBasePtrsArray &ambig_defn_ptrs_array)
{
	assert(ambig_defn_ptrs_array.NumItems() >= 2);
	ID parent_syntax_error_copy = m_parent_syntax_error;
	m_parent_syntax_error = main_error_msg_id;

	// Dump info about the passed ambiguous definitions.
	TTextBuffer256 name_buffer;
	wchar_t object_type_descr_buff[80];
	for (int idefn=0; idefn<ambig_defn_ptrs_array.NumItems(); ++idefn)
	{
		TCppDefnBase *defn = ambig_defn_ptrs_array[idefn];
		ReportSyntaxError(err_ctx_sym, cerr_ambig_defns_array_item, idefn+1, TCppDatabase::GenerateQualifiedName(name_buffer, defn),
						defn->GetObjectTypeFriendlyName(object_type_descr_buff));
	}

	// Restore an old value of the error parent (most likely zero).
	m_parent_syntax_error = parent_syntax_error_copy;
}

void TCppParser::ReportUnappropFunctionAttrs(TSymbolStateItem *err_ctx_sym, TCppItemBase *related_object, TCppErrorCode err_code, TCppFunctionAttr attrs_mask)
{
	assert(attrs_mask != fcta_none);

	// Emit separate error message on each flag in the mask.
	for (int mask=1; mask != 0x10000; mask <<= 1)
	{
		if ((attrs_mask & mask) != 0)
		{
			ReportSyntaxError(err_ctx_sym, related_object, err_code, TCppDatabase::GetFunctionAttributeFriendlyName((TCppFunctionAttr)mask));
		}
	}
}

void TCppParser::AddCtvEventRef(PrEventHeader *ctx, TCppItemBase *item, TCppItemSrcLinkType ref_type, ID mldb_event_id)
{
	assert(item != NULL && ref_type != clink_none && mldb_event_id != 0);

	// Create a link between the MidLev database event and the Cpp database object.
	TCppItemCtvEventRef ctv_event_ref = { ref_type, GetParsingId(), mldb_event_id };
	if (item->AppendCtvEventRef(ctv_event_ref) == FALSE)
	{
		// The situation is bad. The chance that the error below will be recorded into the parsing log
		// is relatively small. Although the count of errors will be increased.
		m_scanner->AddErrorWarning(ctx, 0, lerrc_syntax_err, cerr_error_adding_event_link, GetErrorText(cerr_error_adding_event_link));
	}
}

ID TCppParser::WriteObjectIntroEvent(TSymbolStateItem *whole_area_sym, TCppItemSrcLinkType ref_type, TCppItemBase *item, TCppSourceAreaType atype, PrEventHeader *area_header)
{
	assert(whole_area_sym != NULL && item != NULL);
	PrEventHeader *ctx = &whole_area_sym->GetOriginHeader();

	if (item->ItemId() == 0)
		m_cpp_database.SetupItemId(item);

	// Prepare the cpp item ref event.
	ID event_cn = m_scanner->GetNextCN();
	PrCppItemReferenceEvent info;
	info.hdr.Setup(ctx, event_cn);
	info.cpp_item_type = item->ItemType();
	info.cpp_item_nesting = item->GetItemNesting();
	info.cpp_item_id = item->ItemId();

	if (atype != cxa_none)
	{
		assert(atype < cxa_num_types);
		if (area_header == NULL)
			area_header = &whole_area_sym->GetOriginHeader();

		TCppSourceAreaHeader src_area_header = { atype, *area_header };
		info.event_related_areas.AppendItem(src_area_header);
	}

	// Send notification to the mid lev callback handler for processing.
	m_scanner->CbkHandler().CppItemNotification(&info);
	m_scanner->PostProsessMidLevCbkCall(info.hdr);

	// Add info about this event to the cpp item.
	AddCtvEventRef(ctx, item, ref_type, event_cn);
	return(event_cn);
}

void TCppParser::WriteObjectIntroEventUpdate(TSymbolStateItem *whole_area_sym, TCppItemBase *item, ID existing_mldb_event_id)
{
	assert(whole_area_sym != NULL && item != NULL && item->ItemId() != 0 && item->m_cib_ctv_refs.NumItems() > 0);

	// Prepare cpp item ref event with the passed id and without any extra areas.
	PrCppItemReferenceEvent info;
	info.hdr.Setup(&whole_area_sym->GetOriginHeader(), existing_mldb_event_id);
	info.cpp_item_type = item->ItemType();
	info.cpp_item_nesting = item->GetItemNesting();
	info.cpp_item_id = item->ItemId();

	// Send notification to the mid lev callback handler for processing.
	m_scanner->CbkHandler().CppItemNotification(&info);
	m_scanner->PostProsessMidLevCbkCallEx(info.hdr);
}

// -------------------------------------------------------------
// - - - - - - - - - - - -  App methods group  - - - - - - - - - - - - - - -
// -------------------------------------------------------------

void TCppParser::ProcessCppRule(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	// Do the high level switch based on the type of the non terminal that is being created.
	switch (new_non_term.m_app_id)
	{
		//
		// Part 01. Main.
		//

		case sym_TranslationUnit:				break;	// No processing is needed for this symbol.

		//
		// Part 02. Names.
		//

		case sym_SimpleOrQualifiedId:			ProcessSimpleOrQualifiedId(new_non_term, irule, rule);			break;
		case sym_ExtendedId:					ProcessExtendedId(new_non_term, irule, rule);					break;
		case sym_ExpressionId	:				ProcessExpressionId(new_non_term, irule, rule);					break;
		case sym_SpecialUnqualifiedId:			ProcessSpecialUnqualifiedId(new_non_term, irule, rule);			break;

		//
		// Part 03. Declarations.
		//

		case sym_DeclarationsSeq:				break;	// No processing is needed for this symbol.
		case sym_Declaration:					ProcessDeclaration(new_non_term, irule, rule);					break;
		case sym_BlockDeclaration:				ProcessBlockDeclaration(new_non_term, irule, rule);				break;
		case sym_SimpleDeclaration:			ProcessSimpleDeclaration(new_non_term, irule, rule);				break;

		case sym_DeclTypeSpecifier:			ProcessDeclTypeSpecifier(new_non_term, irule, rule);				break;
		case sym_DeclPrefix:					ProcessDeclPrefix(new_non_term, irule, rule);					break;
		case sym_FunctionSpecifier:				ProcessFunctionSpecifier(new_non_term, irule, rule);				break;
		case sym_StorageClassSpecifier:		ProcessStorageClassSpecifier(new_non_term, irule, rule);			break;

		case sym_TypeSpecifier:				ProcessTypeSpecifier(new_non_term, irule, rule);					break;
		case sym_SimpleTypeSpecifier:			ProcessSimpleTypeSpecifier(new_non_term, irule, rule);			break;
		case sym_BuiltInTypeSpecifier:			ProcessBuiltInTypeSpecifier(new_non_term, irule, rule);			break;
		case sym_NamedTypeSpecifier:			ProcessNamedTypeSpecifier(new_non_term, irule, rule);			break;
		case sym_ElaboratedTypeSpecifier:		ProcessElaboratedTypeSpecifier(new_non_term, irule, rule);		break;

		case sym_UsingDeclaration:				ProcessUsingDeclaration(new_non_term, irule, rule);				break;
		case sym_UsingDirective:				ProcessUsingDirective(new_non_term, irule, rule);				break;
		case sym_AsmDefinition:				ProcessAsmDefinition(new_non_term, irule, rule);					break;
		case sym_LinkageSpecification:			ProcessLinkageSpecification(new_non_term, irule, rule);			break;
		case sym_LinkageSpecHeader:			ProcessLinkageSpecHeader(new_non_term, irule, rule);			break;

		//
		// Part 04. Declarators.
		//

		case sym_InitDeclaratorsList:			ProcessInitDeclaratorsList(new_non_term, irule, rule);				break;
		case sym_InitDeclarator:				ProcessInitDeclarator(new_non_term, irule, rule);					break;
		case sym_UninitedDeclarator:			ProcessUninitedDeclarator(new_non_term, irule, rule);				break;
		case sym_Declarator:					ProcessDeclarator(new_non_term, irule, rule);					break;
		case sym_DirectDeclarator:				ProcessDirectDeclarator(new_non_term, irule, rule);				break;
		case sym_AbstractDeclarator:			ProcessAbstractDeclarator(new_non_term, irule, rule);				break;
		case sym_DirectAbstractDeclarator:		ProcessDirectAbstractDeclarator(new_non_term, irule, rule);		break;

		case sym_PtrOperator:					ProcessPtrOperator(new_non_term, irule, rule);					break;
		case sym_CvQualifier:					ProcessCvQualifier(new_non_term, irule, rule);					break;
		case sym_ParameterDeclarationClause:	ProcessParameterDeclarationClause(new_non_term, irule, rule);	break;
		case sym_ParameterDeclarationHeader:	ProcessParameterDeclarationHeader(new_non_term, irule, rule);	break;
		case sym_ParameterDeclarationsList:		break;	// No processing is needed for this symbol.
		case sym_ParameterDeclaration:			ProcessParameterDeclaration(new_non_term, irule, rule);			break;
		case sym_UninitedParameter:			ProcessUninitedParameter(new_non_term, irule, rule);				break;
		case sym_ExceptionsSpecification:		ProcessExceptionsSpecification(new_non_term, irule, rule);			break;
		case sym_ExceptionTypeIdsList:			ProcessExceptionTypeIdsList(new_non_term, irule, rule);			break;

		case sym_TypeId:						ProcessTypeId(new_non_term, irule, rule);						break;
		case sym_Initializer:					ProcessInitializer(new_non_term, irule, rule);					break;
		case sym_InitializerClause:				ProcessInitializerClause(new_non_term, irule, rule);				break;
		case sym_InitializersList:				ProcessInitializersList(new_non_term, irule, rule);					break;

		//
		// Part 05. Namespaces.
		//

		case sym_NamespaceDefinition:			ProcessNamespaceDefinition(new_non_term, irule, rule);			break;
		case sym_NamespaceHeader:			ProcessNamespaceHeader(new_non_term, irule, rule);				break;
		case sym_NamespaceAliasDefinition:		ProcessNamespaceAliasDefinition(new_non_term, irule, rule);		break;

		//
		// Part 06. Enums.
		//

		case sym_EnumSpecifier:				ProcessEnumSpecifier(new_non_term, irule, rule);				break;
		case sym_EnumeratorHeader:			ProcessEnumeratorHeader(new_non_term, irule, rule);			break;
		case sym_EnumeratorsList:				break;	// No processing is needed for this symbol.
		case sym_EnumeratorDefinition:			ProcessEnumeratorDefinition(new_non_term, irule, rule);			break;

		//
		// Part 07. Classes.
		//

		case sym_ClassSpecifier:				ProcessClassSpecifier(new_non_term, irule, rule);				break;
		case sym_ClassHeader:				ProcessClassHeader(new_non_term, irule, rule);					break;
		case sym_ClassKey:					ProcessClassKey(new_non_term, irule, rule);					break;

		case sym_MemberDeclarationsSeq:		break;	// No processing is needed for this symbol.
		case sym_MemberDeclaration:			ProcessMemberDeclaration(new_non_term, irule, rule);			break;
		case sym_AccessSpecifier:				ProcessAccessSpecifier(new_non_term, irule, rule);				break;
		case sym_SimpleMemberDeclaration:		ProcessSimpleMemberDeclaration(new_non_term, irule, rule);		break;
		case sym_MemberDeclaratorsList:		break;	// No processing is needed for this symbol.
		case sym_MemberDeclarator:			ProcessMemberDeclarator(new_non_term, irule, rule);				break;
		case sym_ConstantInitializer:			ProcessConstantInitializer(new_non_term, irule, rule);				break;

		case sym_BaseClassSpecifiersList:		ProcessBaseClassSpecifiersList(new_non_term, irule, rule);			break;
		case sym_BaseClassSpecifier:			ProcessBaseClassSpecifier(new_non_term, irule, rule);			break;

		//
		// Part 08. Templates.
		//

		case sym_TemplateDeclaration:			ProcessGenericTemplateDeclaration(new_non_term, irule, rule);		break;
		case sym_MemberTemplateDeclaration:	ProcessGenericTemplateDeclaration(new_non_term, irule, rule);		break;
		case sym_TemplateDeclarationHeader:	ProcessTemplateDeclarationHeader(new_non_term, irule, rule);		break;

		case sym_TemplateParamsListHeader:	ProcessTemplateParamsListHeader(new_non_term, irule, rule);		break;
		case sym_TemplateParametersList:		break;	// No processing is needed for this symbol.
		case sym_TemplateParameter:			break;	// No processing is needed for this symbol.
		case sym_TemplateTypeParameter:		ProcessTemplateTypeParameter(new_non_term, irule, rule);		break;
		case sym_TemplateTemplateParameter:	ProcessTemplateTemplateParameter(new_non_term, irule, rule);	break;

		case sym_ExplicitInstantiation:			ProcessGenericTemplateDeclaration(new_non_term, irule, rule);		break;
		case sym_MemberExplicitInstantiation:	ProcessGenericTemplateDeclaration(new_non_term, irule, rule);		break;
		case sym_ExplicitInstantiationHeader:		ProcessExplicitInstantiationHeader(new_non_term, irule, rule);		break;

		case sym_TemplateId:					ProcessTemplateId(new_non_term, irule, rule);					break;
		case sym_TemplateArgumentsList:		break;	// No processing is needed for this symbol.
		case sym_TemplateArgument:			ProcessTemplateArgument(new_non_term, irule, rule);			break;

		//
		// Part 09. Functions.
		//

		case sym_FunctionDefinition:			ProcessFunctionDefinition(new_non_term, irule, rule);				break;
		case sym_FunctionHeader:				ProcessFunctionHeader(new_non_term, irule, rule);				break;
		case sym_MemberInitsList:				break;	// No processing is needed for this symbol.
		case sym_MemberInitializer:			ProcessMemberInitializer(new_non_term, irule, rule);				break;

		case sym_OperatorFunctionId:			ProcessOperatorFunctionId(new_non_term, irule, rule);			break;
		case sym_OverloadableOperator:		ProcessOverloadableOperator(new_non_term, irule, rule);			break;
		case sym_ConversionFunctionId:			ProcessConversionFunctionId(new_non_term, irule, rule);			break;
		case sym_ConversionTypeId:			ProcessConversionTypeId(new_non_term, irule, rule);				break;
		case sym_ConversionDeclarator:			ProcessConversionDeclarator(new_non_term, irule, rule);			break;

		//
		// Part 10. Statements.
		//

		case sym_Statement:					ProcessStatement(new_non_term, irule, rule);					break;
		case sym_CompoundStatement:			ProcessCompoundStatement(new_non_term, irule, rule);			break;
		case sym_CompoundStatementHeader:	ProcessCompoundStatementHeader(new_non_term, irule, rule);		break;
		case sym_StatementsSeq:				ProcessStatementsSeq(new_non_term, irule, rule);				break;
		case sym_LabeledStatement:			ProcessLabeledStatement(new_non_term, irule, rule);				break;
		case sym_ExpressionStatement:			ProcessExpressionStatement(new_non_term, irule, rule);			break;
		case sym_SelectionStatement:			ProcessSelectionStatement(new_non_term, irule, rule);			break;
		case sym_IterationStatement:			ProcessIterationStatement(new_non_term, irule, rule);			break;
		case sym_ForInitStatement:				ProcessForInitStatement(new_non_term, irule, rule);				break;
		case sym_Condition:					ProcessCondition(new_non_term, irule, rule);					break;
		case sym_UninitedCondition:			ProcessUninitedCondition(new_non_term, irule, rule);				break;
		case sym_DoStatementBody:			ProcessDoStatementBody(new_non_term, irule, rule);				break;
		case sym_JumpStatement:				ProcessJumpStatement(new_non_term, irule, rule);				break;
		case sym_TryBlock:					ProcessTryBlock(new_non_term, irule, rule);						break;
		case sym_ExceptionHandlersSeq:		ProcessExceptionHandlersSeq(new_non_term, irule, rule);			break;
		case sym_ExceptionHandler:			ProcessExceptionHandler(new_non_term, irule, rule);				break;
		case sym_ExceptionHandlerHeader:		ProcessExceptionHandlerHeader(new_non_term, irule, rule);		break;

		//
		// Part 11. Expressions.
		//

		case sym_ExpressionsList:				ProcessMultiOpExpression(new_non_term, irule, rule);				break;
		case sym_Expression:					ProcessMultiOpExpression(new_non_term, irule, rule);				break;
		case sym_AssignmentExpression:		ProcessAssignmentExpression(new_non_term, irule, rule);			break;
		case sym_AssignmentOperator:			ProcessAssignmentOperator(new_non_term, irule, rule);			break;

		case sym_ThrowExpression:				ProcessThrowExpression(new_non_term, irule, rule);				break;
		case sym_ConstantExpressionsList:		ProcessMultiOpExpression(new_non_term, irule, rule);				break;
		case sym_ConstantExpression:			ProcessConstantExpression(new_non_term, irule, rule);			break;
		case sym_ConditionalExpression:		ProcessConditionalExpression(new_non_term, irule, rule);			break;
		case sym_LogicalOrExpression:			ProcessMultiOpExpression(new_non_term, irule, rule);				break;
		case sym_LogicalAndExpression:			ProcessMultiOpExpression(new_non_term, irule, rule);				break;
		case sym_BitOrExpression:				ProcessMultiOpExpression(new_non_term, irule, rule);				break;
		case sym_BitXorExpression:				ProcessMultiOpExpression(new_non_term, irule, rule);				break;
		case sym_BitAndExpression:			ProcessMultiOpExpression(new_non_term, irule, rule);				break;

		case sym_EqualityExpression:			ProcessBinaryExpression(new_non_term, irule, rule);				break;
		case sym_RelationalExpression:			ProcessBinaryExpression(new_non_term, irule, rule);				break;
		case sym_ShiftExpression:				ProcessBinaryExpression(new_non_term, irule, rule);				break;
		case sym_AdditiveExpression:			ProcessAddMulExpression(new_non_term, irule, rule);				break;
		case sym_MultiplicativeExpression:		ProcessAddMulExpression(new_non_term, irule, rule);				break;
		case sym_PmExpression:				ProcessPmExpression(new_non_term, irule, rule);				break;

		case sym_CastExpression:				ProcessCastExpression(new_non_term, irule, rule);				break;
		case sym_UnaryExpression:				ProcessUnaryExpression(new_non_term, irule, rule);				break;
		case sym_UnaryOperator:				ProcessUnaryOperator(new_non_term, irule, rule);				break;

		case sym_NewExpression:				ProcessNewExpression(new_non_term, irule, rule);				break;
		case sym_NewPlacement:				ProcessNewPlacement(new_non_term, irule, rule);				break;
		case sym_NewTypeId:					ProcessNewTypeId(new_non_term, irule, rule);					break;
		case sym_NewDeclarator:				ProcessNewDeclarator(new_non_term, irule, rule);				break;
		case sym_DirectNewDeclarator:			ProcessDirectNewDeclarator(new_non_term, irule, rule);			break;
		case sym_NewInitializer:				ProcessNewInitializer(new_non_term, irule, rule);					break;
		case sym_DeleteExpression:			ProcessDeleteExpression(new_non_term, irule, rule);				break;

		case sym_PostfixExpression:			ProcessPostfixExpression(new_non_term, irule, rule);				break;
		case sym_PrimaryExpression:			ProcessPrimaryExpression(new_non_term, irule, rule);				break;
		case sym_Literal:						ProcessLiteral(new_non_term, irule, rule);						break;
		case sym_StringsSeq:					ProcessStringsSeq(new_non_term, irule, rule);					break;

		// Consistency check 1.
		case 0:
			{
				// Destination non terminal does not have any app id.
				TNonTerminalSymbol &non_term = m_grammar->non_terminals[rule.non_term-NonTerminalsBase];
				RaiseCriticalCppError(ccerr_missing_non_term_app_id, non_term.symbol_name, rule.non_term);
				break;
			}

		// Consistency check 2.
		default:
			{
				// Unexpected app id of the dest non terminal.
				TNonTerminalSymbol &non_term = m_grammar->non_terminals[rule.non_term-NonTerminalsBase];
				RaiseCriticalCppError(ccerr_unexpected_non_term_app_id, non_term.app_id, non_term.symbol_name, rule.non_term);
				break;
			}
	}
}

// ----------------------------------------------------------------------------------------------
// ===================  Part 2. Names  ==========================================
// ----------------------------------------------------------------------------------------------

enum P02_Names { P02_member };						// This enum is used for src code navigation purposes only.

void TCppParser::ProcessSimpleOrQualifiedId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//		ExtendedId
	//	 '::'	ExtendedId
	//
	//	SimpleOrQualifiedId   '::'			ExtendedId
	//	SimpleOrQualifiedId   '::'   TEMPLATE	ExtendedId
	//
	int rlen = rule.Length();
	assert(rlen >= 1 && rlen <= 4);
	HandleSimpOrQualNameSymbols(new_non_term, FirstRuleSym(rlen), rlen);
}

void TCppParser::ProcessExtendedId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	identifier
	//	TemplateId
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (s1.m_app_id == sym_identifier)
	{
		// Create simp_or_qual_name non terminal with an identifier in the name info subtype.
		// Note that definitions for this name are not looked up at this point.
		new_non_term.m_meaning = csm_simp_or_qual;
		if (PrepareSimpleNameInfo(s1, new_non_term.m_simp_or_qual) == FALSE)
			return;
	}
	else
	{
		// Rule symbol contains a template id object. Promote it up.
		assert(s1.m_meaning == csm_simp_or_qual && s1.m_simp_or_qual.IsTempalteId() == TRUE);
		new_non_term.m_meaning = csm_simp_or_qual;
		new_non_term.m_simp_or_qual = s1.m_simp_or_qual;
		s1.ResetMeaning();
	}
}

void TCppParser::ProcessExpressionId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	SimpleOrQualifiedId
	//	SpecialUnqualifiedId
	//
	//	'::' OperatorFunctionId
	//	'::' ConversionFunctionId
	//
	//	SimpleOrQualifiedId   '::'			SpecialUnqualifiedId
	//	SimpleOrQualifiedId   '::'   TEMPLATE	SpecialUnqualifiedId
	//
	int rlen = rule.Length();
	assert(rlen >= 1 && rlen <= 4);
	HandleSimpOrQualNameSymbols(new_non_term, FirstRuleSym(rlen), rlen);
}

void TCppParser::ProcessSpecialUnqualifiedId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	'~' identifier
	//	OperatorFunctionId
	//	ConversionFunctionId
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 2)
	{
		// This is creation of a destructor. The second symbol should be an identifier.
		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		assert(s2.m_app_id == sym_identifier);

		// Create the identifier descriptor first. Besides other things this procedure will setup the key id and
		// the unprocessed template header pointer. These fields are needed and they will not be modified below.
		new_non_term.m_meaning = csm_simp_or_qual;
		if (PrepareSimpleNameInfo(s2, new_non_term.m_simp_or_qual) == FALSE)
			return;

		// Update the subtype and change the cached name. Note that the key_id is not reset.
		new_non_term.m_simp_or_qual.m_core.m_info_type = cnit_dector;
		new_non_term.m_simp_or_qual.m_cached_name = m_cpp_database.CreateDestructorName(new_non_term.m_simp_or_qual.m_cached_name);
		if (new_non_term.m_simp_or_qual.m_cached_name == NULL)
		{
			RaiseCriticalCppError(ccerr_oom_generating_object_name, L"a destructor");
			return;
		}

		// Setup the source area of the destructor name to the current non terminal area.
		new_non_term.m_simp_or_qual.m_qual_name_context = new_non_term.GetOriginHeader();
		new_non_term.m_simp_or_qual.m_short_name_context = new_non_term.m_simp_or_qual.m_qual_name_context;
	}
	else
	{
		// Promote existing meaning of the symbol up.
		assert(s1.m_meaning == csm_simp_or_qual);
		new_non_term.m_meaning = csm_simp_or_qual;
		new_non_term.m_simp_or_qual = s1.m_simp_or_qual;
		s1.ResetMeaning();
	}
}

// ----------------------------------------------------------------------------------------------
// ===================  Part 3. Declarations  ======================================
// ----------------------------------------------------------------------------------------------

enum P03_Declarations { P03_member };				// This enum is used for src code navigation purposes only.

void TCppParser::ProcessDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	NamespaceDefinition
	//	BlockDeclaration
	//	FunctionDefinition
	//	TemplateDeclaration
	//	ExplicitInstantiation
	//	LinkageSpecification
	//
	// This non terminal needs processing only when the rule symbol is BlockDeclaration. Note that meaning
	// of the resulting non terminal is csm_none in all cases.
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (s1.m_app_id == sym_BlockDeclaration)
	{
		// This is block declaration. This type of declaration can be present either in the namespace/class context
		// or in the block context. In the block context declaration can stay deep inside the block. At the point of definition
		// such declarations are immediately added to the defns space but their ownersip remains assigned to the non teminal
		// symbol in the stack of the parser. Full publishing happens only here.
		if (s1.m_meaning == csm_declaration)
		{
			// Publish single declaration if it is not an empty statement.
			assert(s1.m_declaration != NULL);
			if (s1.m_declaration->ItemType() != cxx_empty_stmt)
			{
				if (s1.m_declaration->IsPublished() == FALSE)
					PublishOnTheCurrentLayer(s1.m_declaration);
			}
			else
			{
				// Discard this empty statement without publishing it.
				delete s1.m_declaration;
			}
		}
		else
		{
			// This should be a list of declarations.
			assert(s1.m_meaning == csm_declarations_list && s1.m_declarations_list != NULL);
			assert(s1.m_declarations_list->ItemType() == cxx_light_block);

			// Publish members of the declqarations list one by one except for the empty statements.
			while (s1.m_declarations_list->m_members.IsEmpty() == FALSE)
			{
				TCppItemBase *item = (TCppItemBase*)s1.m_declarations_list->m_members.GetFirst();
				s1.m_declarations_list->m_members.RemoveItem(item);
				if (item->ItemType() != cxx_empty_stmt)
				{
					if (item->IsPublished() == FALSE)
						PublishOnTheCurrentLayer(item);
				}
				else
				{
					// Discard this empty statement without publishing it.
					delete item;
				}
			}

			// Discard the light block container.
			delete s1.m_declarations_list;
		}

		s1.ResetMeaning();
	}
	else
	{
		// All symbols that are not BlockDeclarations, should have empty meaning of the symbol.
		assert(s1.m_meaning == csm_none);
	}
}

void TCppParser::ProcessBlockDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	SimpleDeclaration
	//	NamespaceAliasDefinition
	//	UsingDeclaration
	//	UsingDirective
	//	AsmDefinition
	//
	int rlen = rule.Length();
	assert(rlen == 1);

	// Promote meaning of the single symbol of the rule up. No conversion is needed here. The object cannot be
	// published here because this non terminal can be promoted either to the DeclarationsSeq or to the Statement.
	// These non teminals publish their declarations differently.
	PromoteExistingDeclaration(new_non_term, FirstRuleSym(rlen));
}

void TCppParser::ProcessSimpleDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//									';'
	//					InitDeclaratorsList	';'				<--   All declarators in the list should be the overloaded conversions.
	//	DeclTypeSpecifier					';'				<--   DeclTypeSpecifier should be either enum or class.
	//	DeclTypeSpecifier	InitDeclaratorsList	';'
	//
	int rlen = rule.Length();
	assert(rlen >= 1 && rlen <= 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		if (IsTemplateHeaderContext() == TRUE)
		{
			// Template header is not followed by any definition.
			ReportEmptyDefinitionTemplateHeaders();
		}

		// This is an empty statement. Create exactly this type of object.
		TCppEmptyStatement *empty_stmt = (TCppEmptyStatement*)AllocateNewCppItem(cxx_empty_stmt);
		if (empty_stmt == NULL)
			return;

		// Prepare non term with the non NULL pointer in its meaning.
		new_non_term.m_meaning = csm_declaration;
		new_non_term.m_declaration = empty_stmt;
	}
	else if (rlen == 2)
	{
		if (s1.m_app_id == sym_InitDeclaratorsList)
		{
			// This this is typeless declaration on the namespace or block layer. This can be only prototype of
			// the overloaded conversion or several overloaded conversion prototypes. The meaningful processing
			// for this type of objects was taking place on the layer of the single UninitedDeclarator.
			assert(m_curr_decl_type_spec == NULL);
			PromoteExistingDeclaration(new_non_term, s1);
		}
		else
		{
			// This is special form of the rule where only definitions and forward declaration of enums and classes
			// are allowed. Call the helper finction that will create forward declarations and/or report syntax errors.
			assert(s1.m_meaning == csm_decl_type_spec);
			if (DoHandleStandAloneDeclTypeSpec(s1) == FALSE)
				return;

			// Generate empty statement as the meaning of the non terminal of this rule.
			TCppEmptyStatement *empty_stmt = (TCppEmptyStatement*)AllocateNewCppItem(cxx_empty_stmt);
			if (empty_stmt == NULL)
				return;

			new_non_term.m_meaning = csm_declaration;
			new_non_term.m_declaration = empty_stmt;

			// Current published decl type spec should be equal to the first symbol of the rule. Cancel this publication.
			assert(m_curr_decl_type_spec != NULL && m_curr_decl_type_spec == &s1);
			m_curr_decl_type_spec = NULL;
		}
	}
	else
	{
		// This is regular most often used form of the simple declaration that contains the decl type spec and declarator.
		// Note that meaningful processing was taking place on the layer of the single UninitedDeclarator.
		assert(s1.m_meaning == csm_decl_type_spec);
		TCppSymbolStateItem &s2 = NextRuleSym(s1);

		// Promote the meaning that was accumulated in the second symbol of the rule.
		PromoteExistingDeclaration(new_non_term, s2);

		// Current published decl type spec should be equal to the first symbol of the rule. Cancel this publication.
		assert(m_curr_decl_type_spec != NULL && m_curr_decl_type_spec == &s1);
		m_curr_decl_type_spec = NULL;
	}
}

void TCppParser::ProcessDeclTypeSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	DeclPrefix
	//	FunctionSpecifier
	//	StorageClassSpecifier
	//
	//	CONST
	//	VOLATILE
	//	BuiltInTypeSpecifier
	//	NamedTypeSpecifier
	//
	//	ElaboratedTypeSpecifier
	//	EnumSpecifier
	//	ClassSpecifier
	//
	//	DeclTypeSpecifier	 DeclPrefix
	//	DeclTypeSpecifier	 FunctionSpecifier
	//	DeclTypeSpecifier	 StorageClassSpecifier
	//
	//	DeclTypeSpecifier	 CONST
	//	DeclTypeSpecifier	 VOLATILE
	//	DeclTypeSpecifier	 BuiltInTypeSpecifier
	//	DeclTypeSpecifier	 NamedTypeSpecifier
	//
	//	DeclTypeSpecifier	 ElaboratedTypeSpecifier
	//	DeclTypeSpecifier	 EnumSpecifier
	//	DeclTypeSpecifier	 ClassSpecifier
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	HandleTypeSpecifierSymbols(new_non_term, irule, rlen);

	// Publish this non termimal on the data field of the class. This is needed for processing declarators.
	// This data field allows easy access  to the decl type specifier for each declarator in the sequence
	// of declarators regardless of the position of the declarator in the list.
	if (rlen == 1)
	{
		assert(m_curr_decl_type_spec == NULL);
		m_curr_decl_type_spec = &new_non_term;
	}
	else
	{
		assert(m_curr_decl_type_spec == FirstRuleSymPtr(rlen));
		m_curr_decl_type_spec = &new_non_term;
	}
}

void TCppParser::ProcessDeclPrefix(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	TYPEDEF | FRIEND
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	switch (s1.m_app_id)
	{
		case sym_typedef:	new_non_term.m_meaning = csm_decl_prefix;
							new_non_term.m_decl_prefix = declpr_typedef;
							break;
		case sym_friend:		new_non_term.m_meaning = csm_decl_prefix;
							new_non_term.m_decl_prefix = declpr_friend;
							break;
		default:
			RaiseUnexpectedRuleSymbolAppIdError(irule, &s1);
			break;
	}
}

void TCppParser::ProcessFunctionSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	INLINE | VIRTUAL | EXPLICIT
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	switch (s1.m_app_id)
	{
		case sym_inline:		new_non_term.m_meaning = csm_function_attr;
							new_non_term.m_function_attr = fcta_inline;
							break;
		case sym_virtual:		new_non_term.m_meaning = csm_function_attr;
							new_non_term.m_function_attr = fcta_virtual;
							break;
		case sym_explicit:		new_non_term.m_meaning = csm_function_attr;
							new_non_term.m_function_attr = fcta_explicit;
							break;
		default:
			RaiseUnexpectedRuleSymbolAppIdError(irule, &s1);
			break;
	}
}

void TCppParser::ProcessStorageClassSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	REGISTER | AUTO | STATIC | EXTERN | MUTABLE
	//
	// Note that MUTABLE is not a storage class. This is an optional attribute that is applicable to the non static
	// data member of the class. It allows changing this data member from member functions, that are declared
	// as const.
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	switch (s1.m_app_id)
	{
		case sym_register:	new_non_term.m_meaning = csm_storage_class;
							new_non_term.m_storage_class = strgc_register;
							break;
		case sym_auto:		new_non_term.m_meaning = csm_storage_class;
							new_non_term.m_storage_class = strgc_auto;
							break;
		case sym_static:		new_non_term.m_meaning = csm_storage_class;
							new_non_term.m_storage_class = strgc_static;
							break;
		case sym_extern:		new_non_term.m_meaning = csm_storage_class;
							new_non_term.m_storage_class = strgc_extern;
							break;
		case sym_mutable:	new_non_term.m_meaning = csm_storage_class;
							new_non_term.m_storage_class = strgc_mutable;
							break;
		default:
			RaiseUnexpectedRuleSymbolAppIdError(irule, &s1);
			break;
	}
}

void TCppParser::ProcessTypeSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	CONST
	//	VOLATILE
	//	BuiltInTypeSpecifier
	//	NamedTypeSpecifier
	//
	//	ElaboratedTypeSpecifier
	//	EnumSpecifier
	//	ClassSpecifier
	//
	//	TypeSpecifier	 CONST
	//	TypeSpecifier	 VOLATILE
	//	TypeSpecifier	 BuiltInTypeSpecifier
	//	TypeSpecifier	 NamedTypeSpecifier
	//
	//	TypeSpecifier	 ElaboratedTypeSpecifier
	//	TypeSpecifier	 EnumSpecifier
	//	TypeSpecifier	 ClassSpecifier
	//
	// Note that this non terminal is not published on the data members of the class like the DeclTypeSpecifier.
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	HandleTypeSpecifierSymbols(new_non_term, irule, rlen);
}

void TCppParser::ProcessSimpleTypeSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	BuiltInTypeSpecifier
	//	NamedTypeSpecifier
	//
	//	SimpleTypeSpecifier	BuiltInTypeSpecifier
	//	SimpleTypeSpecifier	NamedTypeSpecifier
	//
	// Note that this non terminal is not published on the data members of the class like DeclTypeSpecifier.
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	HandleTypeSpecifierSymbols(new_non_term, irule, rlen);
}

void TCppParser::ProcessBuiltInTypeSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	VOID | BOOL | CHAR | WCHAR | SHORT | INT | LONG | SIGNED | UNSIGNED | FLOAT | DOUBLE
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(s1.m_symbol < NonTerminalsBase);

	// Meaning of the rule non terminal is the app id of the terminal symbol in its right hand side.
	// Validity of this app id will be verified later, during assembling the type specifier.
	new_non_term.m_meaning = csm_built_in_type_spec;
	new_non_term.m_bltin_spec_app_id = s1.m_app_id;
}

void TCppParser::ProcessNamedTypeSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//				SimpleOrQualifiedId
	//	TYPENAME	SimpleOrQualifiedId
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);

	// Pick up the symbol with the name info.
	TCppSymbolStateItem *sym_type_name = LastStackSymPtr();
	assert(sym_type_name->m_meaning == csm_simp_or_qual);

	// Prepare the non term. Resolving the name is postponed because in some cases, like using this name
	// in the stand alone decl type specifier, there will be no resolving at all.
	new_non_term.m_meaning = csm_decl_type_spec;
	new_non_term.m_decl_type_spec_info.Clear();
	new_non_term.m_decl_type_spec_info.m_dclt_unres_name_info = sym_type_name->m_simp_or_qual;
	sym_type_name->ResetMeaning();
}

void TCppParser::ProcessElaboratedTypeSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	ENUM	   SimpleOrQualifiedId
	//	ClassKey	   SimpleOrQualifiedId
	//
	int rlen = rule.Length();
	assert(rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);
	assert(s2.m_meaning == csm_simp_or_qual);

	// Convert first symbol of the rule into the elaboration type.
	TCppElabType elab_type = elab_enum;
	if (s1.m_app_id != sym_enum)
	{
		assert(s1.m_meaning == csm_class_key);
		elab_type = s1.m_class_key;
	}

	// Prepare the non term. Note that resolving the name in the rule is postponed because at this point it
	// is not clear if this type specifier will be used with the FRIEND keyword or not. Processing is different
	// depending on presence or absence of this keyword.
	new_non_term.m_meaning = csm_decl_type_spec;
	new_non_term.m_decl_type_spec_info.Clear();
	new_non_term.m_decl_type_spec_info.m_dclt_unres_name_elab = elab_type;
	new_non_term.m_decl_type_spec_info.m_dclt_unres_name_info = s2.m_simp_or_qual;
	s2.ResetMeaning();
}

void TCppParser::ProcessUsingDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	USING			 ExpressionId   ';'
	//	USING  TYPENAME	 ExpressionId   ';'
	//
	int rlen = rule.Length();
	assert(rlen == 3 || rlen == 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
	assert(s1.m_app_id == sym_using);

	if (IsTemplateHeaderContext() == TRUE)
		ReportUnexpectedTemplateHeaders(L"using declaration");

	// Pick up the info about the definition to use.
	TCppNameInfo *name_info = NULL;
	if (rlen == 3)
	{
		assert(s2.m_meaning == csm_simp_or_qual);
		name_info = &s2.m_simp_or_qual;
	}
	else
	{
		// This is template context.
		assert(s3.m_meaning == csm_simp_or_qual);
		name_info = &s3.m_simp_or_qual;
	}

	// Create new Cpp database object for this directive.
	TCppUsingDeclarationDefn *new_declaration = (TCppUsingDeclarationDefn*)AllocateNewCppItem(cxx_using_declaration);
	if (new_declaration == NULL)
		return;

	// Setup  the props and assign the dummy name for this object.
	// -BUGBUG- Do the name resolution.
	if (m_cpp_database.PrepareAndSetParserGeneratedName(new_declaration, L"Using-Declaration") == FALSE)
	{
		RaiseCriticalCppError(ccerr_oom_setting_alt_name);
		delete new_declaration;
		return;
	}

	// This is block declaration. Publish it and return it at the same time.
	PublishBlockDeclaration(new_declaration, FALSE);
	new_non_term.m_meaning = csm_declaration;
	new_non_term.m_declaration = new_declaration;
}

void TCppParser::ProcessUsingDirective(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	USING   NAMESPACE   SimpleOrQualifiedId   ';'
	//
	int rlen = rule.Length();
	assert(rlen == 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
	assert(s1.m_app_id == sym_using && s3.m_meaning == csm_simp_or_qual);

	if (IsTemplateHeaderContext() == TRUE)
		ReportUnexpectedTemplateHeaders(L"a using directive");

	// Check situation for the name in the third symbol.
	TCppDefnBase *base_object = NULL;
	if (CheckLookupExistingDefinition(&s3, elab_none, s3.m_simp_or_qual, sym_UsingDirective, base_object) == FALSE)
		return;

	// Create new using directive object.
	TCppUsingDirectiveDefn *new_directive = (TCppUsingDirectiveDefn*)AllocateNewCppItem(cxx_using_directive);
	if (new_directive == NULL)
		return;

	if (base_object != NULL)
	{
		// Name in the third symbol corresponds to some existing namespace.
		assert(base_object->ItemType() == cxx_namespace);
		new_directive->m_namespace_to_use = (TCppNamespaceDefn*)base_object;
	}

	// Assign a dummy name to this object.
	if (m_cpp_database.PrepareAndSetParserGeneratedName(new_directive, L"Using-Namespace") == FALSE)
	{
		RaiseCriticalCppError(ccerr_oom_setting_alt_name);
		delete new_directive;
		return;
	}

	// This is block declaration. Publish it and return it at the same time.
	PublishBlockDeclaration(new_directive, FALSE);
	m_defns_layer_owner->GetAssocDefnsSpaceRefsList()->AppendItem(new_directive->m_assoc_ref);

	new_non_term.m_meaning = csm_declaration;
	new_non_term.m_declaration = new_directive;
}

void TCppParser::ProcessAsmDefinition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	ASM '(' StringsSeq ')' ';'
	//
	int rlen = rule.Length();
	assert(rlen == 5);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
	assert(s3.m_meaning == csm_strings_seq);

	if (IsTemplateHeaderContext() == TRUE)
		ReportUnexpectedTemplateHeaders(L"an asm insert");

	// Move the body of the string from the storage of the scanner to the storage of the Cpp database.
	wchar_t *persistent_str_ptr = CacheLiteralString(s3.m_strs_seq_value.m_string_value);
	if (persistent_str_ptr == NULL)
		return;

	// Allocate a database object for new ASM insert.
	TCppAsmInsertDefn *new_asm_insert = (TCppAsmInsertDefn*)AllocateNewCppItem(cxx_asm_insert);
	if (new_asm_insert == NULL)
		return;

	// Setup the props.
	new_asm_insert->SetNameInfo(persistent_str_ptr, 0, cdpr_none);
	new_asm_insert->m_asm_insert.m_body = persistent_str_ptr;
	new_asm_insert->m_asm_insert.m_len = s3.m_strs_seq_value.m_string_value.m_len;

	// Publish the object and return it.
	PublishBlockDeclaration(new_asm_insert, FALSE);
	new_non_term.m_meaning = csm_declaration;
	new_non_term.m_declaration = new_asm_insert;
}

void TCppParser::ProcessLinkageSpecification(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	LinkageSpecHeader  '{'					'}'
	//	LinkageSpecHeader  '{'   DeclarationsSeq	'}'
	//
	//	LinkageSpecHeader  Declaration
	//
	// Note that Declaration and DeclarationsSeq symbols have both empty meaning. This means that
	// they do not require processing.
	//
	int rlen = rule.Length();
	assert(rlen >= 2 && rlen <= 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(s1.m_meaning == csm_linkage_spec_header);

	// Pop one layer of the linkage spec headers stack.
	assert(m_curr_linkage_spec != NULL && m_curr_linkage_spec == &s1);
	m_curr_linkage_spec = s1.m_linkage_spec.m_parent_spec_layer;
	s1.ResetMeaning();
}

void TCppParser::ProcessLinkageSpecHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	EXTERN  StringsSeq
	//
	int rlen = rule.Length();
	assert(rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);
	assert(s2.m_meaning == csm_strings_seq);

	if (IsTemplateHeaderContext() == TRUE)
		ReportUnexpectedTemplateHeaders(L"linkage specification");

	// Procedure for picking up the indexing key below might look strange. This is rarely used API.
	ID err_ident = 0;
	bool malloc_problem = FALSE;
	TStringPtr lnk_type_str_ptr(s2.m_strs_seq_value.m_string_value.m_body, s2.m_strs_seq_value.m_string_value.m_len);
	TKeyInfoItem *key_info = m_scanner->GetIndexingKey(GetCurrentLocation(), kt_name, lnk_type_str_ptr, err_ident, malloc_problem);
	if (key_info == NULL)
	{
		// Low level error was already reported. Duplicate this with the critical error.
		RaiseCriticalCppError(ccerr_key_id_picking_problem, lnk_type_str_ptr.GetLength(), lnk_type_str_ptr.GetBodyPtr());
		return;
	}

	// Copy body of the string from the storage of the scanner into the storage of the Cpp database.
	wchar_t *persistent_str_ptr = CacheLiteralString(s2.m_strs_seq_value.m_string_value);
	if (persistent_str_ptr == NULL)
		return;

	// Allocate the database object for this linkage spec.
	TCppLinkageSpec *new_linkage_spec = (TCppLinkageSpec*)AllocateNewCppItem(cxx_linkage_spec);
	if (new_linkage_spec == NULL)
		return;

	// Fill in the props.
	new_linkage_spec->m_spec_name = persistent_str_ptr;
	new_linkage_spec->m_spec_key_id = key_info->data.key_id;
	new_linkage_spec->m_spec_cnt_use = 0;

	// This object can be present only on the namespace layer. Ensure this and publish the object.
	assert(m_defns_layer_owner->IsNamespace() == TRUE);
	PublishOnTheCurrentLayer(new_linkage_spec);

	// Push this object into the stack.
	new_non_term.m_meaning = csm_linkage_spec_header;
	new_non_term.m_linkage_spec.m_link_spec = new_linkage_spec;
	new_non_term.m_linkage_spec.m_parent_spec_layer = m_curr_linkage_spec;
	m_curr_linkage_spec = &new_non_term;
}

// ----------------------------------------------------------------------------------------------
// ===================  Part 4. Declarators  =======================================
// ----------------------------------------------------------------------------------------------

enum P04_Declarators { P04_member };				// This enum is used for src code navigation purposes only.

void TCppParser::ProcessInitDeclaratorsList(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	InitDeclarator
	//	InitDeclaratorsList  ','  InitDeclarator
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Do not apply any changes now.
		PromoteExistingDeclaration(new_non_term, s1);
	}
	else
	{
		// The third symbol of the rule should be the declaration.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
		assert(s3.m_meaning == csm_declaration && s3.m_declaration != NULL);

		TCppLightBlockDefn *light_block = NULL;
		if (s1.m_meaning == csm_declaration)
		{
			assert(s1.m_declaration != NULL);

			// New light block is needed to create a list of declarations.
			light_block = (TCppLightBlockDefn*)AllocateNewCppItem(cxx_light_block);
			if (light_block == NULL)
				return;

			// Add both declarations into the block.
			light_block->AddChildItem(s1.m_declaration);
			light_block->AddChildItem(s3.m_declaration);
		}
		else
		{
			// Append one more declaration to the contents of the already existing light block.
			assert(s1.m_meaning == csm_declarations_list && s1.m_declarations_list != NULL);
			light_block = s1.m_declarations_list;
			light_block->AddChildItem(s3.m_declaration);
		}

		s1.ResetMeaning();
		s3.ResetMeaning();

		// Finalize the non terminal.
		new_non_term.m_meaning = csm_declarations_list;
		new_non_term.m_declarations_list = light_block;
	}
}

void TCppParser::ProcessInitDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	UninitedDeclarator
	//	UninitedDeclarator  Initializer
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(s1.m_meaning == csm_declaration && s1.m_declaration != NULL);

	if (rlen == 2)
	{
		// The initializer is present.
		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		assert(s2.m_meaning == csm_initializer && s2.m_initializer_data != NULL);

		if (s1.m_declaration->ItemType() == cxx_data_field)
		{
			// Data field can have the initilizer. Check the storage class.
			TCppDataFieldDefn *data_field = (TCppDataFieldDefn*)s1.m_declaration;
			if (data_field->m_strg_class == strgc_extern)
			{
				ReportSyntaxError(&s2, s1.m_declaration, cerr_context_extern_with_initializer);
			}
			else
			{
				// Accept the initializer. The data type will be checked later.
				TCppDataFieldDefn *data_field = (TCppDataFieldDefn*)s1.m_declaration;
				data_field->AddChildItem(s2.m_initializer_data, cdefn_chdest_dtfld_init);
				s2.ResetMeaning();
			}
		}
		else
		{
			// This is some other type of object: typedef, function prototype, etc.
			ReportSyntaxError(&s1, s1.m_declaration, cerr_context_bad_initializer);
		}
	}

	// Promote the meaning of the first symbol.
	new_non_term.m_meaning = csm_declaration;
	new_non_term.m_declaration = s1.m_declaration;
	s1.ResetMeaning();
}

void TCppParser::ProcessUninitedDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	Declarator
	//
	// This non terminal was introduced to assemble, resolve and publish the name before processing the initializer
	// of the declarator if any. This may sound like something not important, but standard explicitly requires this and
	// gives examples where this aspect is significant.
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(s1.m_meaning == csm_declarator);

	// Call the worker function. It can return various types of objects. Note that the value of the current published decl
	// type specifier can be NULL. The worker function will take care of this.
	TCppDefnBase *definition = DoHandleGenericDeclarator(s1, FALSE);
	if (definition == NULL)
		return;

	// Processing may return either an already existing object or create a new one. Check for this.
	if (definition->IsPublished() == FALSE)
	{
		// Publish constucted object as a block layer declaration.
		PublishBlockDeclaration(definition, FALSE);
	}

	// Promote the generated declaration up.
	new_non_term.m_meaning = csm_declaration;
	new_non_term.m_declaration = definition;
}

void TCppParser::ProcessDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	DirectDeclarator
	//	PtrOperator  Declarator
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Promote existing declarator without changing it.
		PromoteExistingDeclarator(new_non_term, s1);
	}
	else
	{
		// Add one more layer of Ptr or Ref indirection.
		HandlePtrOperatorDeclaratorSymbols(new_non_term, s1);
	}
}

void TCppParser::ProcessDirectDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	ExpressionId
	//
	//	DirectDeclarator   '[' ']'
	//	DirectDeclarator   '[' ConstantExpressionsList ']'
	//
	//	DirectDeclarator   ParameterDeclarationClause
	//	DirectDeclarator   ParameterDeclarationClause				ExceptionsSpecification
	//	DirectDeclarator   ParameterDeclarationClause   CvQualifier
	//	DirectDeclarator   ParameterDeclarationClause   CvQualifier	ExceptionsSpecification
	//
	//	'(' Declarator ')'
	//
	int rlen = rule.Length();
	assert(rlen >= 1 && rlen <= 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// This is simple form of the rule. Start building the new declarator.
		assert(s1.m_meaning == csm_simp_or_qual);
		new_non_term.m_meaning = csm_declarator;
		new_non_term.m_declarator_info.Clear();
		new_non_term.m_declarator_info.m_name_info = s1.m_simp_or_qual;
		s1.ResetMeaning();
	}
	else if (s1.m_meaning == csm_declarator)
	{
		// Move the declarator from the first symbol to the non terminal.
		PromoteExistingDeclarator(new_non_term, s1);

		// This is adding modifier to an already existing declarator.
		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		if (s2.m_app_id == sym_lbracket)
		{
			// This is an array modifier.
			if (rlen == 3)
			{
				// This is array with either an empty or an implicit size.
				ApplyMultiDimArrayModifier(NextRuleSymPtr(&s2), new_non_term.m_declarator_info, NULL);
			}
			else
			{
				// The size of array is given explicitly.
				assert(rlen == 4);
				TCppSymbolStateItem &s3 = NextRuleSym(s2), &s4 = NextRuleSym(s3);
				assert(s3.m_meaning == csm_expression && s3.m_expression != NULL);

				// Apply modifier and release the expression.
				ApplyMultiDimArrayModifier(&s4, new_non_term.m_declarator_info, s3.m_expression);
				delete s3.m_expression;
				s3.ResetMeaning();
			}
		}
		else
		{
			// This is function modifier. The worker function will do the job.
			assert(s2.m_meaning == csm_func_params_list);
			HandleFuncParamsDeclaratorSymbols(new_non_term, irule, &s2);
		}
	}
	else
	{
		// Promote meaning of the existing declarator up.
		assert(rlen == 3);
		PromoteExistingDeclarator(new_non_term, NextRuleSym(s1));
	}
}

void TCppParser::ProcessAbstractDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	DirectAbstractDeclarator
	//	PtrOperator
	//	PtrOperator  AbstractDeclarator
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// This is simple form of the rule.
		if (s1.m_meaning == csm_declarator)
		{
			// Promote existing declarator without changing it.
			PromoteExistingDeclarator(new_non_term, s1);
		}
		else
		{
			// Start building the new declarator.
			new_non_term.m_meaning = csm_declarator;
			new_non_term.m_declarator_info.Clear();

			// Apply the modifier.
			assert(s1.m_meaning == csm_ptr_operator);
			ApplyCvPtrRefModifier(&s1, new_non_term.m_declarator_info, s1.m_ptr_operator);
		}
	}
	else
	{
		// Add one more layer of pointer or reference indirection.
		HandlePtrOperatorDeclaratorSymbols(new_non_term, s1);
	}
}

void TCppParser::ProcessDirectAbstractDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	'(' AbstractDeclarator ')'
	//							'[' ']'
	//							'[' ConstantExpressionsList ']'
	//	DirectAbstractDeclarator	'[' ']'
	//	DirectAbstractDeclarator	'[' ConstantExpressionsList ']'
	//
	//							ParameterDeclarationClause
	//							ParameterDeclarationClause				  ExceptionsSpecification
	//							ParameterDeclarationClause    CvQualifier
	//							ParameterDeclarationClause    CvQualifier	  ExceptionsSpecification
	//	DirectAbstractDeclarator	ParameterDeclarationClause
	//	DirectAbstractDeclarator	ParameterDeclarationClause				  ExceptionsSpecification
	//	DirectAbstractDeclarator	ParameterDeclarationClause    CvQualifier
	//	DirectAbstractDeclarator	ParameterDeclarationClause    CvQualifier	  ExceptionsSpecification
	//
	int rlen = rule.Length();
	assert(rlen >= 1 && rlen <= 4);
	TCppSymbolStateItem *sym = FirstRuleSymPtr(rlen);

	if (rlen == 3 && sym->m_app_id == sym_lpar)
	{
		// Promote meaning of the existing declarator up.
		PromoteExistingDeclarator(new_non_term, NextRuleSym(*sym));
	}
	else
	{
		if (sym->m_meaning == csm_declarator)
		{
			// Move declarator from the first symbol to the non terminal.
			PromoteExistingDeclarator(new_non_term, *sym);

			// Shift to the next symbol.
			sym = NextRuleSymPtr(sym);
			assert(sym != NULL);
		}
		else
		{
			// Start building the new abstract declarator.
			new_non_term.m_meaning = csm_declarator;
			new_non_term.m_declarator_info.Clear();
		}

		if (sym->m_app_id == sym_lbracket)
		{
			// This is an array modifier. Shift to the next symbol.
			sym = NextRuleSymPtr(sym);
			assert(sym != NULL);

			if (sym->m_meaning != csm_expression)
			{
				// Apply array modifier with empty expression.
				assert(sym->m_app_id == sym_rbracket);
				ApplyMultiDimArrayModifier(sym, new_non_term.m_declarator_info, NULL);
			}
			else
			{
				// Apply array modifier with a meaningful expression.
				assert(sym->m_expression != NULL);
				ApplyMultiDimArrayModifier(NextRuleSymPtr(sym), new_non_term.m_declarator_info, sym->m_expression);
			}
		}
		else
		{
			// This is function modifier. The worker function will take care of the rest.
			assert(sym->m_meaning == csm_func_params_list);
			HandleFuncParamsDeclaratorSymbols(new_non_term, irule, sym);
		}
	}
}

void TCppParser::ProcessPtrOperator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//								'*'
	//								'*'   CvQualifier
	//								'&'
	//			  SimpleOrQualifiedId	'::'  '*'
	//			  SimpleOrQualifiedId	'::'  '*'  CvQualifier
	//	ClassKey	  SimpleOrQualifiedId	'::'  '*'
	//	ClassKey	  SimpleOrQualifiedId	'::'  '*'  CvQualifier
	//
	int rlen = rule.Length();
	assert(rlen >= 1 && rlen <= 5);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	// Start building the non terminal.
	new_non_term.m_meaning = csm_ptr_operator;
	new_non_term.m_ptr_operator.Clear();

	if (s1.m_app_id == sym_mul)
	{
		// This is a simple pointer or a pointer with a const/volatile specifier.
		new_non_term.m_ptr_operator.cvpr_pointer = TRUE;
		if (rlen == 2)
		{
			TCppSymbolStateItem &s2 = NextRuleSym(s1);
			assert(s2.m_meaning == csm_cv_qualifier);
			new_non_term.m_ptr_operator.cvpr_const = s2.m_cv_qualifier.cvpr_const;
			new_non_term.m_ptr_operator.cvpr_volatile = s2.m_cv_qualifier.cvpr_volatile;
		}
	}
	else if (s1.m_app_id == sym_bitand)
	{
		// This is a reference modifier. It should not have any other attributes.
		assert(rlen == 1);
		new_non_term.m_ptr_operator.cvpr_reference = TRUE;
	}
	else
	{
		// This should be a pointer to the member of the class.
		assert(rlen >= 3);
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);

		TCppElabType elab_type = elab_none;
		TCppSymbolStateItem *ptr_class_name_sym = NULL;
		TCppSymbolStateItem *ptr_cv_qual = NULL;
		if (s1.m_meaning == csm_class_key)
		{
			assert(s2.m_meaning == csm_simp_or_qual);
			elab_type = s1.m_class_key;
			ptr_class_name_sym = &s2;
			ptr_cv_qual = NextRuleSymPtr(NextRuleSymPtr(&s3));
		}
		else
		{
			assert(s1.m_meaning == csm_simp_or_qual);
			ptr_class_name_sym = &s1;
			ptr_cv_qual = NextRuleSymPtr(&s3);
		}

		// Pick up the structure type object if any.
		TCppDefnBase *base_data_type = NULL;
		if (CheckLookupExistingDefinition(ptr_class_name_sym, elab_type, ptr_class_name_sym->m_simp_or_qual, sym_PtrOperator, base_data_type) == FALSE)
			return;

		// Lookup should always return non NULL pointer, this will be at least an unresolved object.
		assert(base_data_type != NULL && base_data_type->IsStructType() == TRUE);
		new_non_term.m_ptr_operator.cvpr_struct = (TCppStructTypeDefn*)base_data_type;

		// Finish building the structure.
		new_non_term.m_ptr_operator.cvpr_pointer = TRUE;
		if (ptr_cv_qual != NULL)
		{
			assert(ptr_cv_qual->m_meaning == csm_cv_qualifier);
			new_non_term.m_ptr_operator.cvpr_const = ptr_cv_qual->m_cv_qualifier.cvpr_const;
			new_non_term.m_ptr_operator.cvpr_volatile = ptr_cv_qual->m_cv_qualifier.cvpr_volatile;
		}
	}
}

void TCppParser::ProcessCvQualifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	CONST | VOLATILE | CONST VOLATILE | VOLATILE CONST
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);

	new_non_term.m_meaning = csm_cv_qualifier;
	new_non_term.m_cv_qualifier.Clear();

	if (rlen == 1)
	{
		// Check the symbol in the rule.
		TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
		if (s1.m_app_id == sym_const)
			new_non_term.m_cv_qualifier.cvpr_const = TRUE;
		else new_non_term.m_cv_qualifier.cvpr_volatile = TRUE;
	}
	else
	{
		// Set both flags ON.
		new_non_term.m_cv_qualifier.cvpr_const = TRUE;
		new_non_term.m_cv_qualifier.cvpr_volatile = TRUE;
	}
}

void TCppParser::ProcessParameterDeclarationClause(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	'('		 ')'
	//	'('	'...'	 ')'
	//
	//	'('  ParameterDeclarationsList		')'
	//	'('  ParameterDeclarationsList	 '...'	')'
	//	'('  ParameterDeclarationsList  ','	 '...'	')'
	//
	// Note that the list of params is stored in the first symbol (ParamsListHeader) and not in the params list symbol itself.
	//
	int rlen = rule.Length();
	assert(rlen >= 2 && rlen <= 5);
	TCppSymbolStateItem *sym = FirstRuleSymPtr(rlen);

	// The first symbol should be the params list header.
	assert(sym->m_meaning == csm_func_params_list);
	TList *prms_list = sym->m_func_params_info.m_params_list;

	// Pop the stack of declared param lists.
	assert(sym == m_curr_params_list);
	m_curr_params_list = sym->m_func_params_info.m_parent_params_symbol;
	sym->ResetMeaning();

	sym = NextRuleSymPtr(sym);
	if (sym->m_app_id == sym_ParameterDeclarationsList)
	{
		// Shift the rule symbol pointer.
		sym = NextRuleSymPtr(sym);
		assert(sym != NULL);
	}

	if (sym->m_app_id == sym_comma)
	{
		// This symbol does not have any meaning here. Shift the rule sym pointer.
		sym = NextRuleSymPtr(sym);
		assert(sym != NULL);
	}

	if (sym->m_app_id == sym_ellipsis)
	{
		// Create nameless parameter with a special built in type.
		TCppBuiltInTypeDefn *ellipsis_type = GetBuiltInTypeDefn(bltin_id_ellips);
		if (ellipsis_type == NULL)
		{
			if (prms_list != NULL)
				RecycleGeneralPurposeList(prms_list);
			return;
		}

		// Create the data field object for this param.
		TCppDataFieldDefn *ellipsis_param = (TCppDataFieldDefn*)AllocateNewCppItem(cxx_data_field);
		if (ellipsis_param == NULL)
		{
			if (prms_list != NULL)
				RecycleGeneralPurposeList(prms_list);
			return;
		}

		if (prms_list == NULL)
		{
			// The list is not allocated yet. Create a new one.
			prms_list = AllocateGeneralPurposeList();
			if (prms_list == NULL)
			{
				delete ellipsis_param;
				return;
			}
		}

		// Fill in the props. Param index will be set up later.
		ellipsis_param->SetNameInfo(L"...", 0, cdpr_none);
		ellipsis_param->m_strg_class = strgc_param;
		ellipsis_param->m_field_type = ellipsis_type;

		// Finally add this param to the list.
		prms_list->AppendItem(ellipsis_param);
	}

	// Setup the non terminal. Note that the list, that is being set, can be NULL.
	new_non_term.m_meaning = csm_func_params_list;
	new_non_term.m_func_params_info.m_params_list = prms_list;
	new_non_term.m_func_params_info.m_parent_params_symbol = NULL;
}

void TCppParser::ProcessParameterDeclarationHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	'('
	//

	// Setup the non terminal with the NULL params list and push the stack.
	new_non_term.m_meaning = csm_func_params_list;
	new_non_term.m_func_params_info.m_params_list = NULL;
	new_non_term.m_func_params_info.m_parent_params_symbol = m_curr_params_list;
	m_curr_params_list = &new_non_term;
}

void TCppParser::ProcessParameterDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	UninitedParameter
	//	UninitedParameter  '='  AssignmentExpression
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(s1.m_meaning == csm_func_parameter && s1.m_func_parameter != NULL);

	if (rlen == 3)
	{
		// Take care of the default value. Expression can be used as a default value directly.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
		assert(s3.m_meaning == csm_expression && s3.m_expression != NULL);
		s1.m_func_parameter->AddChildItem(s3.m_expression, cdefn_chdest_dtfld_init);
		s3.ResetMeaning();
	}

	// Give out the object.
	if (PublishParameterObject(s1.m_func_parameter) == TRUE)
	{
		// The param object was consumed.
		s1.ResetMeaning();
	}
}

void TCppParser::ProcessUninitedParameter(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	TypeSpecifier
	//	TypeSpecifier  Declarator
	//	TypeSpecifier  AbstractDeclarator
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	// Create the data field object for this param.
	TCppDataFieldDefn *new_param = CreateDataParamObject(s1, (rlen == 1) ? FALSE : TRUE);
	if (new_param == NULL)
		return;

	// Setup the non terminal.
	new_non_term.m_meaning = csm_func_parameter;
	new_non_term.m_func_parameter = new_param;
}

void TCppParser::ProcessExceptionsSpecification(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	THROW  '('  ')'
	//	THROW  '('  ExceptionTypeIdsList  ')'
	//
	int rlen = rule.Length();
	assert(rlen == 3 || rlen == 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 3)
	{
		// The list of exceptions is empty. This situation is handled in the following way. The list is created and
		// it contains single item with a NULL pointer to the data type. This allows to differentiate this case from
		// the situation when there is no THROW specification at all.
		if (CreatePossibleExceptiosList(new_non_term, NULL) == FALSE)
			return;
	}
	else
	{
		// The rule contains non empty list of exceptions that the function can throw.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
		assert(s3.m_meaning == csm_exceptions_list && s3.m_exceptions_list != NULL);

		// Setup the non terminal.
		new_non_term.m_meaning = csm_exceptions_list;
		new_non_term.m_exceptions_list = s3.m_exceptions_list;
		s3.ResetMeaning();
	}
}

void TCppParser::ProcessExceptionTypeIdsList(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	TypeId
	//	ExceptionTypeIdsList  ','  TypeId
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Start building the list of possible exceptions.
		assert(s1.m_meaning == csm_data_type && s1.m_data_type != NULL);
		if (CreatePossibleExceptiosList(new_non_term, s1.m_data_type) == FALSE)
			return;
	}
	else
	{
		// Append one more exception to the already existing list of possible exceptions.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
		assert(s1.m_meaning == csm_exceptions_list && s1.m_exceptions_list != NULL);
		assert(s3.m_meaning == csm_data_type && s3.m_data_type != NULL);

		// Check, if the new data type is already present in the existing list or not.
		bool unique_type = TRUE;
		for (TListIter<TCppPossibleExceptionSpec> iter(s1.m_exceptions_list); iter; ++iter)
		{
			if (TCppItemBase::CompareSameDatabaseDataTypes(iter.CurrItem().m_exception_type, s3.m_data_type) == TRUE)
			{
				ReportSyntaxError(&s3, cerr_dts_throws_type_duplication, s3.m_data_type->DefnName());
				unique_type = FALSE;
				break;
			}
		}

		if (unique_type == TRUE)
		{
			// The exception type is fine.
			TCppPossibleExceptionSpec *spec = (TCppPossibleExceptionSpec*)AllocateNewCppItem(cxx_possible_exception_spec);
			if (spec == NULL)
				return;

			spec->m_exception_type = s3.m_data_type;
			s1.m_exceptions_list->AppendItem(spec);
		}

		// Finalize the data structure.
		new_non_term.m_meaning = csm_exceptions_list;
		new_non_term.m_exceptions_list = s1.m_exceptions_list;
		s1.ResetMeaning();
	}
}

void TCppParser::ProcessTypeId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	TypeSpecifier
	//	TypeSpecifier  AbstractDeclarator
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	HandleGenericTypeIdSymbols(new_non_term, FirstRuleSym(rlen), rlen);
}

void TCppParser::ProcessInitializer(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	'=' InitializerClause
	//	'(' ExpressionsList ')'
	//
	int rlen = rule.Length();
	assert(rlen == 2 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);

	if (rlen == 2)
	{
		// Second symbol should be the initializer. Promote it up.
		assert(s2.m_meaning == csm_initializer && s2.m_initializer_data != NULL);
		new_non_term.m_meaning = csm_initializer;
		new_non_term.m_initializer_data = s2.m_initializer_data;
		s2.ResetMeaning();
	}
	else
	{
		// Second symbol should be the expression. Create function call object and use this expression as params.
		assert(s2.m_meaning == csm_expression && s2.m_expression != NULL);

		TCppFunctionCallExpr *fun_call_node = (TCppFunctionCallExpr*)AllocateNewCppItem(cxx_function_call_expr);
		if (fun_call_node == NULL)
			return;

		// Prepare the data. The pointer to the function will be NULL for now.
		fun_call_node->AddCallParameters(s2.m_expression);
		s2.ResetMeaning();

		new_non_term.m_meaning = csm_initializer;
		new_non_term.m_initializer_data = fun_call_node;
	}
}

void TCppParser::ProcessInitializerClause(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	AssignmentExpression
	//
	//	'{'   '}'
	//	'{'   InitializersList		'}'
	//	'{'   InitializersList    ','	'}'
	//
	int rlen = rule.Length();
	assert(rlen >= 1 && rlen <= 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Convert expression into the initializer.
		assert(s1.m_meaning == csm_expression && s1.m_expression != NULL);
		new_non_term.m_meaning = csm_initializer;
		new_non_term.m_initializer_data = s1.m_expression;
		s1.ResetMeaning();
	}
	else if (rlen == 2)
	{
		// Create an empty initializer.
		TCppDataFieldInitializer *init_node = (TCppDataFieldInitializer*)AllocateNewCppItem(cxx_data_field_initializer);
		if (init_node == NULL)
			return;

		// Set it into the non terminal.
		new_non_term.m_meaning = csm_initializer;
		new_non_term.m_initializer_data = init_node;
	}
	else
	{
		// Promote existing initializer from the second symbol.
		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		assert(s2.m_meaning == csm_initializer && s2.m_initializer_data != NULL);
		new_non_term.m_meaning = csm_initializer;
		new_non_term.m_initializer_data = s2.m_initializer_data;
		s2.ResetMeaning();
	}
}

void TCppParser::ProcessInitializersList(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	InitializerClause
	//	InitializersList  ','  InitializerClause
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Start building the initializer.
		assert(s1.m_meaning == csm_initializer && s1.m_initializer_data != NULL);

		// List of initializers, even if it will contains only one item, will be enclosed in the curved brackets.
		// This is unavoidable. This means that it is ok to create the initializer node right now.
		TCppDataFieldInitializer *init_node = (TCppDataFieldInitializer*)AllocateNewCppItem(cxx_data_field_initializer);
		if (init_node == NULL)
			return;

		// Prepare the node.
		init_node->AddChildItem(s1.m_initializer_data);
		s1.ResetMeaning();

		// Setup the non terminal.
		new_non_term.m_meaning = csm_initializer;
		new_non_term.m_initializer_data = init_node;
	}
	else
	{
		// Ensure that symbols of the rule have expected meaning.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
		assert(s1.m_meaning == csm_initializer && s1.m_initializer_data != NULL);
		assert(s3.m_meaning == csm_initializer && s3.m_initializer_data != NULL);

		// Extend the existing initializer.
		assert(s1.m_initializer_data->ItemType() == cxx_data_field_initializer);
		TCppDataFieldInitializer *init_node = (TCppDataFieldInitializer*)s1.m_initializer_data;
		init_node->AddChildItem(s3.m_initializer_data);
		s3.ResetMeaning();

		// Setup the non terminal.
		new_non_term.m_meaning = csm_initializer;
		new_non_term.m_initializer_data = s1.m_initializer_data;
		s1.ResetMeaning();
	}
}

// ----------------------------------------------------------------------------------------------
// ===================  Part 05. Namespaces  ====================================
// ----------------------------------------------------------------------------------------------

enum P05_Namespaces { P05_member };				// This enum is used for src code navigation purposes only.

void TCppParser::ProcessNamespaceDefinition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	NamespaceHeader					'}'
	//	NamespaceHeader   DeclarationsSeq	'}'
	//
	int rlen = rule.Length();
	assert(rlen == 2 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(s1.m_meaning == csm_namesp_header && s1.m_namesp_defn != NULL);

	// The processing is simple. Just exit the layer and reset the meaning of the symbol with namesp header.
	ExitCurrentDefnsLayer(&s1);
	s1.ResetMeaning();
}

void TCppParser::ProcessNamespaceHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	NAMESPACE			'{'
	//	NAMESPACE  identifier	'{'
	//
	int rlen = rule.Length();
	assert(rlen == 2 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);

	if (IsTemplateHeaderContext() == TRUE)
		ReportUnexpectedTemplateHeaders(L"namespace definition");

	TCppNamespaceDefn *namesp_object = NULL;
	if (rlen == 3)
	{
		// The name for the namespace is present. Check the situation for this name.
		assert(s2.m_app_id == sym_identifier);
		TCppNameInfo name_info;
		TCppDefnBase *existing_namesp_object = NULL;
		if (CheckLookupSimpleIdentifierSymbol(s2, name_info, cxx_namespace, &existing_namesp_object) == FALSE)
			return;

		// Check results of the search.
		if (existing_namesp_object == NULL)
		{
			// Create new namespace.
			namesp_object = (TCppNamespaceDefn*)AllocateNewCppItem(cxx_namespace, name_info.m_preferred_object_id);
			if (namesp_object == NULL)
				return;

			// Setup the props.
			name_info.SetNameToObject(namesp_object);
		}
		else
		{
			// Use an already existing one.
			assert(existing_namesp_object->ItemType() == cxx_namespace);
			namesp_object = (TCppNamespaceDefn*)existing_namesp_object;
		}
	}
	else
	{
		// This is nameless namespace. This may sound strange. Nevertheless Cpp standard allows such objects.
		namesp_object = (TCppNamespaceDefn*)AllocateNewCppItem(cxx_namespace);
		if (namesp_object == NULL)
			return;

		// The name for the namespace is still needed. Setup a non empty unique name.
		if (m_cpp_database.PrepareAndSetParserGeneratedName(namesp_object, L"") == FALSE)
		{
			RaiseCriticalCppError(ccerr_oom_generating_object_name, L"a nameless namespace");
			delete namesp_object;
			return;
		}
	}

	// Publish this namespace on the current layer if it was just created.
	assert(m_defns_layer_owner->IsNamespace() == TRUE);
	if (namesp_object->IsPublished() == FALSE)
		PublishOnTheCurrentLayer(namesp_object);

	// Setup the non terminal.
	new_non_term.m_meaning = csm_namesp_header;
	new_non_term.m_namesp_defn = namesp_object;
	EnterDefinitionsLayer(namesp_object, new_non_term);
}

void TCppParser::ProcessNamespaceAliasDefinition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	NAMESPACE  identifier  '='  SimpleOrQualifiedId  ';'
	//
	int rlen = rule.Length();
	assert(rlen == 5);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2), &s4 = NextRuleSym(s3);

	if (IsTemplateHeaderContext() == TRUE)
		ReportUnexpectedTemplateHeaders(L"namespace alias definition");

	// Pick up the key_id for the name in the second symbol and look for defns with this name on the current layer.
	TCppNameInfo name_info;
	if (CheckLookupSimpleIdentifierSymbol(s2, name_info, cxx_namespace_alias) == FALSE)
		return;

	// Resolve the mentioned namespace. Note that this namespace should be already existing.
	TCppDefnBase *base_object = NULL;
	if (CheckLookupExistingDefinition(&s4, elab_none, s4.m_simp_or_qual, sym_NamespaceAliasDefinition, base_object) == FALSE)
		return;

	// Create new namespace alias object.
	TCppNamespaceAliasDefn *namesp_alias = (TCppNamespaceAliasDefn*)AllocateNewCppItem(cxx_namespace_alias, name_info.m_preferred_object_id);
	if (namesp_alias == NULL)
		return;

	// Setup the props.
	name_info.SetNameToObject(namesp_alias);
	if (base_object != NULL)
	{
		// Name in the forth symbol corresponds to an existing namespace.
		assert(base_object->ItemType() == cxx_namespace);
		namesp_alias->m_base_namespace = (TCppNamespaceDefn*)base_object;
	}

	// This is a block level declaration. Publish it and return it at the same time.
	PublishBlockDeclaration(namesp_alias, FALSE);
	new_non_term.m_meaning = csm_declaration;
	new_non_term.m_declaration = namesp_alias;
}

// ----------------------------------------------------------------------------------------------
// ===================  Part 06. Enums  =========================================
// ----------------------------------------------------------------------------------------------

enum P06_Enums { P06_member };						// This enum is used for src code navigation purposes only.

void TCppParser::ProcessEnumSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	EnumeratorHeader						  '}'
	//	EnumeratorHeader     EnumeratorsList		  '}'
	//	EnumeratorHeader     EnumeratorsList    ','	  '}'
	//
	int rlen = rule.Length();
	assert(rlen >= 2 && rlen <= 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(s1.m_meaning == csm_enum_header && m_curr_enum_header == &s1);

	// Processing is pretty simple.
	new_non_term.m_meaning = csm_data_type;
	new_non_term.m_data_type = s1.m_enum_info.m_enum_defn;
	m_curr_enum_header = NULL;
}

void TCppParser::ProcessEnumeratorHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	ENUM											'{'
	//	ENUM					   ':'	  SimpleTypeSpecifier	'{'
	//	ENUM    SimpleOrQualifiedId							'{'
	//	ENUM    SimpleOrQualifiedId	   ':'	  SimpleTypeSpecifier	'{'
	//
	// Note that template headers in front of this definition, if they are present, are ignored here. They will be
	// checked/used either while processing the stand alone decl type spec or while processing the declarator.
	//
	int rlen = rule.Length();
	assert(rlen >= 2 && rlen <= 5);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);
	assert(m_curr_enum_header == NULL);

	TCppDefnBase *defns_layer = NULL;
	TCppEnumTypeDefn *enum_object = NULL;
	TCppSymbolStateItem *base_type_symbol = NULL;
	ID intro_event_id = 0;

	if (s2.m_meaning == csm_simp_or_qual)
	{
		// The name of the enum is present in the rule. Check various possible cases.
		TCppNameInfo &name_info = s2.m_simp_or_qual;
		defns_layer = name_info.GetDefnsLayer(m_defns_layer_owner);

		if (name_info.IsTempalteId() == TRUE)
		{
			// Name of the enum contains template arguments. This is not allowed in the enum header.
			ReportSyntaxError(&s2, cerr_enum_bad_template_args);
			name_info.DowngradeTemplateIdToIdentifier(this);
		}

		assert(name_info.IsIdentifier() == TRUE && name_info.KeyId() != 0);
		TCppDefnBase *existing_forward_decl_object = NULL;
		if (CheckLookupElabTypeDefnName(elab_enum, &s2, cxx_enum_type, existing_forward_decl_object) == FALSE)
			return;

		// Check the results of the search.
		if (existing_forward_decl_object == NULL)
		{
			// Appropriate forward declaration is not available. New object should be created.
			enum_object = (TCppEnumTypeDefn*)AllocateNewCppItem(cxx_enum_type, name_info.m_preferred_object_id);
			if (enum_object == NULL)
				return;

			// Use the prepared cached name from the name info. This name is either an original name that was
			// retrieved from the source code or this is a modified name in case if some error was detected.
			name_info.SetNameToObject(enum_object);
		}
		else
		{
			// Accept the discovered forward declaration.
			assert(existing_forward_decl_object->ItemType() == cxx_enum_type);
			enum_object = (TCppEnumTypeDefn*)existing_forward_decl_object;
		}

		// Link created of discovered object to the current place in the code.
		intro_event_id = WriteObjectIntroEvent(&s2, clink_definition, enum_object, cxa_enum_defn);

		// Check for the base type symbol presence.
		TCppSymbolStateItem *sym_after_enum_name = NextRuleSymPtr(&s2);
		assert(sym_after_enum_name != NULL);
		if (sym_after_enum_name->m_app_id == sym_colon)
		{
			base_type_symbol = NextRuleSymPtr(sym_after_enum_name);
			assert(base_type_symbol != NULL);
		}

	}
	else
	{
		// This is a nameless enum.
		defns_layer = m_defns_layer_owner;
		enum_object = (TCppEnumTypeDefn*)AllocateNewCppItem(cxx_enum_type);
		if (enum_object == NULL)
			return;

		// Setup dummy unique name with the key id equal to zero.
		if (m_cpp_database.PrepareAndSetParserGeneratedName(enum_object, L"") == FALSE)
		{
			RaiseCriticalCppError(ccerr_oom_generating_object_name, L"a nameless enum");
			delete enum_object;
			return;
		}

		// Link this nameless object to the current place in the code.
		intro_event_id = WriteObjectIntroEvent(&s1, clink_definition, enum_object, cxa_bright_keyword);

		// Check for the base type symbol presence.
		if (s2.m_app_id == sym_colon)
		{
			base_type_symbol = NextRuleSymPtr(&s2);
			assert(base_type_symbol != NULL);
		}
	}

	// Convert this enum object into the full definition.
	assert(enum_object != NULL);
	enum_object->m_defined = TRUE;

	if (base_type_symbol != NULL)
	{
		// Base type symbol is present in the rule. Note that this is simple type specifier. This means that
		// storage class, cv_spec, etc cannot be present there. The function below will take care of the base
		// class. There is no need to do any additional checks here.
		assert(base_type_symbol->m_meaning == csm_decl_type_spec);
		if (CheckPrepareTypeSpecifier(*base_type_symbol) == FALSE)
			return;

		TCppDefnBase *dclt_base_type = base_type_symbol->m_decl_type_spec_info.m_dclt_base_type;
		assert(dclt_base_type != NULL);

		if (dclt_base_type->IsDataType() == TRUE)
		{
			// Type specifier contains the data type. Check this type.
			TCppDataTypeBase *underlying_type = TCppDataTypeBase::GetUnderlyingNumericType((TCppDataTypeBase*)dclt_base_type, IsTemplateBodyContext());
			if (underlying_type == NULL)
			{
				if (dclt_base_type->IsUnresolvedObject() == FALSE)
				{
					// The specified base type is bogus.
					ReportSyntaxError(base_type_symbol, cerr_enum_base_type_non_numeric);
				}
			}
			else if (underlying_type->ItemType() == cxx_enum_type)
			{
				// Enum cannot be used as the base type for other enums. Isue an error and leave the base type
				// field NULL. This is the same value as if the base type was not specified at all.
				ReportSyntaxError(base_type_symbol, cerr_enum_base_type_enum_type);
			}
			else
			{
				// Accept the base type. Note that it can be template param or unresoved type in template context.
				enum_object->m_base_type = (TCppDataTypeBase*)dclt_base_type;
			}
		}
		else
		{
			// This should be the function type. It is not acceptable here.
			assert(dclt_base_type->IsFunctionType() == TRUE);
			ReportSyntaxError(base_type_symbol, cerr_enum_base_type_func_type);
		}
	}

	if (enum_object->IsPublished() == FALSE)
	{
		if (defns_layer == m_defns_layer_owner)
		{
			// This is special case when the full publishing is needed. This situation may also happen on the block layer.
			PublishBlockDeclaration(enum_object, TRUE);
		}
		else
		{
			// This enum should show up on some other layer.
			PublishOnTheSpecificLayer(defns_layer, enum_object);
		}
	}

	// Setup the enum header into the non terminal.
	new_non_term.m_meaning = csm_enum_header;
	new_non_term.m_enum_info.m_enum_defn = enum_object;
	new_non_term.m_enum_info.m_next_value = 0;
	new_non_term.m_enum_info.m_object_intro_ctv_event = intro_event_id;
	m_curr_enum_header = &new_non_term;
}

void TCppParser::ProcessEnumeratorDefinition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	identifier
	//	identifier  '='  ConstantExpression
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(m_curr_enum_header != NULL);

	// Pick up the defns layer for enum members that might be different from the current layer.
	TCppDefnBase *defns_layer = (TCppDefnBase*)m_curr_enum_header->m_enum_info.m_enum_defn->ItemParent();
	assert(defns_layer != NULL);

	// Pick up the info about the identifier.
	TCppNameInfo name_info;
	if (CheckLookupSimpleIdentifierSymbol(s1, name_info, cxx_enum_member, NULL, defns_layer) == FALSE)
		return;

	// Create new enum member object.
	TCppEnumMemberDefn *enum_member = (TCppEnumMemberDefn*)AllocateNewCppItem(cxx_enum_member, name_info.m_preferred_object_id);
	if (enum_member == NULL)
		return;

	// Fill in the name props.
	name_info.SetNameToObject(enum_member);

	// Link this brand new object to the current place in the code.
	WriteObjectIntroEvent(&s1, clink_definition, enum_member, cxa_enum_member);

	// Assign the numeric value.
	if (rlen == 1)
	{
		// Setup the value based on the value of the counter in the enum header.
		enum_member->m_const_value = TRUE;
		enum_member->m_value = m_curr_enum_header->m_enum_info.m_next_value++;
		enum_member->m_value_num_subt = TCppExpressionBase::GetNumericSubtypeFromValue(enum_member->m_value);
	}
	else
	{
		// The rule contains an expression.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
		assert(s3.m_meaning == csm_expression && s3.m_expression != NULL);

		// Try to compute this expression.
		TCppOperandExpr enum_member_value;
		if (s3.m_expression->GetConstOperandValue(enum_member_value) == FALSE)
		{
			// The expression is not constant.
			if (IsTemplateBodyContext() == FALSE)
				ReportSyntaxError(&s3, enum_member, cerr_enum_member_expr_not_const);
		}
		else if (enum_member_value.ConvertToNumericLiteral() == FALSE)
		{
			// The expr is constant but the constant value is not numeric.
			if (IsTemplateBodyContext() == FALSE)
				ReportSyntaxError(&s3, enum_member, cerr_enum_member_expr_not_numeric);
		}
		else
		{
			// Conversion to the numeric value went fine. Store the value.
			enum_member->m_const_value = TRUE;
			enum_member->m_value = enum_member_value.m_num_value;
			enum_member->m_value_num_subt = enum_member_value.m_num_subt;
			m_curr_enum_header->m_enum_info.m_next_value = enum_member_value.m_num_value+1;
		}

		// Store expression in the enum object. Note that expression is stored regardless if it was
		// converted to the const numeric value or not.
		enum_member->AddChildItem(s3.m_expression);
		s3.ResetMeaning();
	}

	if (defns_layer == m_defns_layer_owner)
	{
		// Publish this defintion on the current layer without releasing the ownership.
		if (PublishBlockDeclaration(enum_member, FALSE) == FALSE)
			return;
	}
	else
	{
		// Add enum member to the defns space of the remote layer. This code has an unobvious aspect when
		// enum header contains a qualifying path that leads to a place where enum defns are not allowed.
		// The enum object itself will be created with prefix, suffix and key_id equal to zero. This will make it
		// clear that it is not a legimate object. But members of this enum will show up as legitimate objects.
		defns_layer->AddToNestedDefnsSpace(enum_member);
	}

	// Assign the ownership to the enum type object. The non terminal should have an empty meaning.
	m_curr_enum_header->m_enum_info.m_enum_defn->AddChildItem(enum_member);
}

// ----------------------------------------------------------------------------------------------
// ===================  Part 07. Classes  =========================================
// ----------------------------------------------------------------------------------------------

enum P07_Classes { P07_member };						// This enum is used for src code navigation purposes only.

void TCppParser::ProcessClassSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	ClassHeader							  '}'
	//	ClassHeader	  MemberSpecificationsSeq	  '}'
	//
	int rlen = rule.Length();
	assert(rlen == 2 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(s1.m_meaning == csm_class_header && s1.m_struct_info.m_struct_defn != NULL);
	ExitCurrentDefnsLayer(&s1);

	// Setup non terminal as the data type reference.
	new_non_term.m_meaning = csm_data_type;
	new_non_term.m_data_type = s1.m_struct_info.m_struct_defn;
	s1.ResetMeaning();
}

void TCppParser::ProcessClassHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	ClassKey												  '{'
	//	ClassKey						':'	BaseClassSpecifiersList	  '{'
	//	ClassKey   SimpleOrQualifiedId							  '{'
	//	ClassKey   SimpleOrQualifiedId	':'	BaseClassSpecifiersList	  '{'
	//
	// This handler checks/processes themplate headers. At the same time at this point it is not clear yet
	// if this specifier will be used as stand alone decl type spec or with declarator. This means that other
	// elements of the decl type spec will be checked later even if they are already scanned.
	//
	int rlen = rule.Length();
	assert(rlen >= 2 && rlen <= 5);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);
	assert(s1.m_meaning == csm_class_key);

	// Pick up optional symbols that can be present in the header.
	TCppSymbolStateItem *sym_class_name = NULL;
	TCppSymbolStateItem *sym_base_classes = NULL;
	if (s2.m_meaning == csm_simp_or_qual)
	{
		// The name of the class is present. Store the pointer to the symbol.
		sym_class_name = &s2;
		TCppSymbolStateItem *sym = NextRuleSymPtr(&s2);
		assert(sym != NULL);

		if (sym->m_app_id == sym_colon)
		{
			// The colon is present. Pick up the list of the base classes that should follow the colon.
			sym_base_classes = NextRuleSymPtr(sym);
			assert(sym_base_classes != NULL && sym_base_classes->m_meaning == csm_base_classes_list);
		}
	}
	else if (s2.m_app_id == sym_colon)
	{
		// The class key is followed by the colon. The list of the base classes should follow the colon.
		sym_base_classes = NextRuleSymPtr(&s2);
		assert(sym_base_classes != NULL && sym_base_classes->m_meaning == csm_base_classes_list);
	}

	// Create one of the forms of the struct object.
	TCppDefnBase *defns_layer = NULL;
	TCppStructTypeDefn *struct_object = NULL;
	ID intro_event_id = 0;

	if (sym_class_name != NULL)
	{
		// The name is present. Several different cases are possible.
		TCppNameInfo &name_info = sym_class_name->m_simp_or_qual;
		defns_layer = name_info.GetDefnsLayer(m_defns_layer_owner);

		// Check for special case where template args are straight repetition of params from the template header.
		name_info.CheckRemoveTemplateDefnTemplateArgs(this);

		// Call an appropriate worker method. They will create target object in all cases. Besides that they will report errors
		// about absense/presence of template headers, bogus qual path in the name if any, wrong type of the name, etc.
		if (name_info.IsIdentifier() == TRUE)
		{
			if (name_info.m_unproc_template_header == NULL)
			{
				// This is creation of the struct type.
				struct_object = FindOrCreateStructTypeObject(s1.m_class_key, sym_class_name, sym_base_classes);
			}
			else
			{
				// This is creation of the class template.
				struct_object = FindOrCreateClassTemplateObject(s1.m_class_key, sym_class_name, sym_base_classes);
			}
		}
		else
		{
			// This should be creation of the class template specialization.
			struct_object = FindOrCreateClassTemplateSpecObject(s1.m_class_key, sym_class_name, sym_base_classes);
		}

		// Check for the critical error.
		if (struct_object == NULL)
			return;

		// Link created of discovered object to the current place in the code.
		intro_event_id = WriteObjectIntroEvent(&s2, clink_definition, struct_object, (TCppSourceAreaType)(cxa_struct_defn+struct_object->DefnElabType()-elab_struct));
	}
	else
	{
		// This is a nameless structure.
		defns_layer = m_defns_layer_owner;

		// Nameless structure cannot create any form of the class template. Class templates are always named.
		if (IsTemplateHeaderContext() == TRUE)
		{
			// Issue the same error for all types of template headers and ignore these headers if any.
			wchar_t object_type_name_buff[80];
			swprintf(object_type_name_buff, 80, L"definition of the nameless %s", TCppDefnBase::GetElabTypeName(s1.m_class_key));
			ReportUnexpectedTemplateHeaders(object_type_name_buff);
		}

		// Create the struct object.
		struct_object = (TCppStructTypeDefn*)AllocateNewCppItem(cxx_struct_type);
		if (struct_object == NULL)
			return;

		// Setup a non empty unique name.
		if (m_cpp_database.PrepareAndSetParserGeneratedName(struct_object, L"") == FALSE)
		{
			wchar_t object_type_name_buff[80];
			swprintf(object_type_name_buff, 80, L"a nameless %s", TCppDefnBase::GetElabTypeName(s1.m_class_key));
			RaiseCriticalCppError(ccerr_oom_generating_object_name, object_type_name_buff);
			delete struct_object;
			return;
		}

		// Fill in the struct type and props.
		struct_object->SetupStructType(s1.m_class_key);
		struct_object->m_defined = TRUE;

		// Link this nameless object to the current place in the code.
		intro_event_id = WriteObjectIntroEvent(&s1, clink_definition, struct_object, cxa_bright_keyword);

		if (sym_base_classes != NULL)
		{
			// Nameless structures can have base classes. This seems strange but this is allowed.
			struct_object->AddBaseClassesList(sym_base_classes->m_base_classes_list);
		}

		// Look for the named parent. In typical case such parent will be available. Once this is not so,
		// the nameless structure will use its own defns space for storing the member lookup info.
		TCppSymbolStateItem *layer_symbol = m_defns_layer_symbol;
		while (layer_symbol != NULL && layer_symbol->m_meaning == csm_class_header)
		{
			if (layer_symbol->m_struct_info.m_struct_defn->IsNamelessStruct() == FALSE)
			{
				// Named structure is present in the stack of the entered layers.
				struct_object->SetupNamedParent(layer_symbol->m_struct_info.m_struct_defn);
				break;
			}

			layer_symbol = layer_symbol->m_parent_layer_symbol;
		}
	}

	// Creation of the object suceeded.
	assert(struct_object != NULL);
	if (struct_object->IsPublished() == FALSE)
	{
		if (defns_layer == m_defns_layer_owner)
		{
			// This is special case when the full publishing is needed. Pass the second param as TRUE.
			PublishBlockDeclaration(struct_object, TRUE);
		}
		else
		{
			// This struct should show up on some other layer.
			PublishOnTheSpecificLayer(defns_layer, struct_object);
		}
	}

	// Setup the non terminal.
	new_non_term.m_meaning = csm_class_header;
	new_non_term.m_struct_info.m_struct_defn = struct_object;
	new_non_term.m_struct_info.m_curr_accs_level = (struct_object->DefnElabType() == elab_class) ? accs_private : accs_public;
	new_non_term.m_struct_info.m_object_intro_ctv_event = intro_event_id;
	EnterDefinitionsLayer(struct_object, new_non_term);
}

void TCppParser::ProcessClassKey(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	STRUCT | UNION | CLASS
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	switch (s1.m_app_id)
	{
		case sym_struct:		new_non_term.m_meaning = csm_class_key;
							new_non_term.m_class_key = elab_struct;
							break;
		case sym_union:		new_non_term.m_meaning = csm_class_key;
							new_non_term.m_class_key = elab_union;
							break;
		case sym_class:		new_non_term.m_meaning = csm_class_key;
							new_non_term.m_class_key = elab_class;
							break;
		default:
			RaiseUnexpectedRuleSymbolAppIdError(irule, &s1);
			break;
	}
}

void TCppParser::ProcessMemberDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	AccessSpecifier ':'
	//	SimpleMemberDeclaration
	//	FunctionDefinition
	//	MemberTemplateDeclaration
	//	MemberExplicitInstantiation
	//	UsingDeclaration
	//
	int rlen = rule.Length();
	assert(rlen >= 1 && rlen <= 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(m_defns_layer_symbol->m_meaning  == csm_class_header && m_defns_layer_owner->IsStructType() == TRUE);

	if (s1.m_meaning == csm_access_spec)
	{
		// This access level specifier.
		if (IsTemplateHeaderContext() == TRUE)
			ReportUnexpectedTemplateHeaders(L"member access level specifier");

		//  Change the current access layer.
		m_defns_layer_symbol->m_struct_info.m_curr_accs_level = s1.m_access_spec;
	}
	else if (s1.m_meaning == csm_declaration)
	{
		// This should be a using declaration.
		assert(s1.m_declaration != NULL && s1.m_declaration->ItemType() == cxx_using_declaration);
		if (s1.m_declaration->IsPublished() == FALSE)
			PublishOnTheCurrentLayer(s1.m_declaration);
		s1.ResetMeaning();
	}
	else
	{
		// All other symbols should have empty meaning.
		assert(s1.m_meaning == csm_none);
	}
}

void TCppParser::ProcessAccessSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	PUBLIC | PROTECTED | PRIVATE
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	switch (s1.m_app_id)
	{
		case sym_public:		new_non_term.m_meaning = csm_access_spec;
							new_non_term.m_access_spec = accs_public;
							break;
		case sym_protected:	new_non_term.m_meaning = csm_access_spec;
							new_non_term.m_access_spec = accs_protected;
							break;
		case sym_private:		new_non_term.m_meaning = csm_access_spec;
							new_non_term.m_access_spec = accs_private;
							break;
		default:
			RaiseUnexpectedRuleSymbolAppIdError(irule, &s1);
			break;
	}
}

void TCppParser::ProcessSimpleMemberDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//										 ';'
	//					MemberDeclaratorsList	 ';'			<--   All declarators should be: ctors, dectors or overloaded conversions.
	//	DeclTypeSpecifier						 ';'			<--   DeclTypeSpecifier should only be either enum or class.
	//	DeclTypeSpecifier	MemberDeclaratorsList	 ';'
	//
	// Note that this non terminal has an empty meaning.
	//
	int rlen = rule.Length();
	assert(rlen >= 1 && rlen <= 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(m_defns_layer_symbol->m_meaning  == csm_class_header && m_defns_layer_owner->IsStructType() == TRUE);

	if (rlen == 1)
	{
		if (IsTemplateHeaderContext() == TRUE)
		{
			// Template header is not followed by any definition.
			ReportEmptyDefinitionTemplateHeaders();
		}
	}
	else if (rlen == 2)
	{
		if (s1.m_meaning == csm_decl_type_spec)
		{
			// This is special form of the rule where only definitions and forward declaration of enums and classes
			// are allowed. Call the helper finction that will create forward declarations and/or report syntax errors.
			if (DoHandleStandAloneDeclTypeSpec(s1) == FALSE)
				return;

			// Current decl type spec should be equal to the first symbol of the rule. Remove the published spec.
			assert(m_curr_decl_type_spec != NULL && m_curr_decl_type_spec == &s1);
			m_curr_decl_type_spec = NULL;
		}
		else
		{
			// Meaning of the list of declarators inside the class should be empty.
			assert(m_curr_decl_type_spec == NULL && s1.m_meaning == csm_none);
		}
	}
	else
	{
		// This is some regular declaration. Everything what was needed is already done.
		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		assert(s1.m_meaning == csm_decl_type_spec && s2.m_meaning == csm_none);

		// Current decl type spec should be equal to the first symbol of the rule. Remove the published spec.
		assert(m_curr_decl_type_spec != NULL && m_curr_decl_type_spec == &s1);
		m_curr_decl_type_spec = NULL;
	}
}

void TCppParser::ProcessMemberDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	Declarator
	//	Declarator	  ConstantInitializer
	//
	//	identifier	   ':'	  ConstantExpression
	//			   ':'	  ConstantExpression
	//
	// Note that bit fields cannot have initializers. Maybe this is so because they cannot be static (it is not fully clear why).
	//
	int rlen = rule.Length();
	assert(rlen >= 1 && rlen <= 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	TCppItemBase *item = NULL;
	if (s1.m_meaning == csm_declarator)
	{
		// This one of the most often used rules. Call the worker function. It can return various types of objects.
		// Note that the value of the current decl type specifier can be NULL. The worker function will take care of this.
		item = DoHandleGenericDeclarator(s1, FALSE);
		if (item == NULL)
			return;

		if (rlen > 1)
		{
			// The initializer is present. Check its compatibility with the created member.
			TCppSymbolStateItem &s2 = NextRuleSym(s1);
			assert(s2.m_meaning == csm_expression && s2.m_expression != NULL);

			if (item->ItemType() == cxx_data_field)
			{
			}
			else if (item->ItemType() == cxx_code_entry_point)
			{
			}
			else
			{
				// All other objects cannot have initializers.
				ReportSyntaxError(&s2, item, cerr_struct_member_init_not_applic, m_defns_layer_owner->GetElabTypeName());
			}
		}
	}
	else if (s1.m_app_id == sym_identifier)
	{
		// This is bit field member.
		if (IsTemplateHeaderContext() == TRUE)
			ReportUnexpectedTemplateHeaders(L"the bit field member");

		TCppDataTypeBase *base_bf_type = NULL;
		bool mutable_bit_field = FALSE;

		if (m_curr_decl_type_spec == NULL)
		{
			// The decl type specifier is missing.
			ReportSyntaxError(&s1, cerr_struct_bft_no_decl_type_spec);
		}
		else
		{
			// The decl type spec is available. Check for unwanted fields there.
			TCppDeclTypeSpecInfo &dts = m_curr_decl_type_spec->m_decl_type_spec_info;
			if (dts.m_dclt_prefix != declpr_none)
				ReportSyntaxError(&s1, cerr_struct_bft_bad_decl_prefix, TCppDatabase::GetDeclTypeSpecPrefixFriendlyName(dts.m_dclt_prefix));
			if (dts.m_dclt_func_attrs != fcta_none)
				ReportUnappropFunctionAttrs(&s1, NULL, cerr_struct_bft_bad_func_attr, dts.m_dclt_func_attrs);
			if (dts.m_dclt_strg_class != strgc_none && dts.m_dclt_strg_class != strgc_mutable)
				ReportSyntaxError(&s1, cerr_struct_bft_bad_strg_class, TCppDataFieldDefn::GetStorageClassFriendlyName(dts.m_dclt_strg_class));

			if (dts.m_dclt_strg_class == strgc_mutable)
				mutable_bit_field = TRUE;

			// Resolve the base type.
			if (CheckPrepareTypeSpecifier(*m_curr_decl_type_spec) == FALSE)
				return;

			// Check the base type. It should be present in the decl type specifier.
			assert(dts.m_dclt_base_type != NULL);
			if (dts.m_dclt_base_type->IsDataType() == TRUE)
			{
				TCppDataTypeBase *underlying_type = TCppDataTypeBase::GetUnderlyingNumericType((TCppDataTypeBase*)dts.m_dclt_base_type, IsTemplateBodyContext());
				if (underlying_type == NULL)
				{
					// The specified base type is bogus.
					ReportSyntaxError(&s1, cerr_struct_bft_base_type_not_numeric);
				}
				else
				{
					// Accept the base type. Note that in the template context it can be also a template param
					// or one of the unresoved types flavor.
					base_bf_type = (TCppDataTypeBase*)dts.m_dclt_base_type;

					// Check for the CV spec.
					if (dts.m_dclt_cv_spec.IsEmpty() == FALSE)
					{
						// Create a const/volatile modifier.
						TCppDeclaratorModifier extra_cv_spec;
						extra_cv_spec.SetCvprSpec(dts.m_dclt_cv_spec);
						base_bf_type = CreateCvPtrRefModification(&s1, base_bf_type, &extra_cv_spec, 1, TRUE, TRUE);
						if (base_bf_type == NULL)
							return;
					}
				}
			}
			else
			{
				// This should be a function type. Such type is not good for bit fields.
				assert(dts.m_dclt_base_type->IsFunctionType() == TRUE);
				ReportSyntaxError(&s1, cerr_struct_bft_base_type_func_type);
			}
		}

		if (base_bf_type == NULL)
		{
			// The base type is missing or the existing base type is bogus. Setup the substitution.
			base_bf_type = GetBuiltInTypeDefn(bltin_id_void);
			if (base_bf_type == NULL)
				return;
		}

		// The base type was resolved or substed. Create a bit field type on this base.
		TCppBitFieldTypeDefn *bf_type = CreateBitFieldType(&s1, base_bf_type, NextRuleSym(NextRuleSym(s1)));
		if (bf_type == NULL)
			return;

		// Note that grammar allows using only identifier as the name of the bit field. Attempt to use declarator
		// modifiers or template arguments will result in the declarator symbol or tempalte id. Most likely a syntax
		// error will be generated. In any case the control will not come here.
		TCppNameInfo name_info;
		if (CheckLookupSimpleIdentifierSymbol(s1, name_info, cxx_bit_field_type) == FALSE)
			return;

		// Create new data field object.
		TCppDataFieldDefn *data_field = (TCppDataFieldDefn*)AllocateNewCppItem(cxx_data_field, name_info.m_preferred_object_id);
		if (data_field == NULL)
			return;

		// Fill in the props and give the object out.
		name_info.SetNameToObject(data_field);
		data_field->m_accs_level = GetCurrentAccessLevel();
		data_field->m_mutable = mutable_bit_field;
		data_field->m_field_type = bf_type;
		item = data_field;
	}
	else
	{
		// This is data field alignment record.
		if (IsTemplateHeaderContext() == TRUE)
			ReportUnexpectedTemplateHeaders(L"the data alignment record");

		// Create a new object for this record.
		assert(s1.m_app_id == sym_colon);
		TCppFieldAlignSpec *align_spec = (TCppFieldAlignSpec *)AllocateNewCppItem(cxx_field_align_spec);
		if (align_spec == NULL)
			return;

		// Pick up and evaluate the expression.
		TCppSymbolStateItem &expr_sym = NextRuleSym(s1);
		short bit_size_value = ComputeBitWidthExpression(expr_sym, FALSE);

		// Fill in the props and give the object out.
		align_spec->m_align_value = bit_size_value;
		align_spec->AddChildItem(expr_sym.m_expression);
		expr_sym.ResetMeaning();
		item = align_spec;
	}

	// Publish the object if needed. Objects, that are created out of declarators in the class scope should
	// be published immediately. There is no need to propagate them up as this is needed in blocks.
	if (item->IsPublished() == FALSE)
	{
		// Publish the constucted object.
		PublishOnTheCurrentLayer(item);
	}
}

void TCppParser::ProcessConstantInitializer(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	'='  ConstantExpression
	//
	int rlen = rule.Length();
	assert(rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);
	assert(s2.m_meaning == csm_expression && s2.m_expression != NULL);

	// Promote expression from the second rule symbol up.
	new_non_term.m_meaning = csm_expression;
	new_non_term.m_expression = s2.m_expression;
	s2.ResetMeaning();
}

void TCppParser::ProcessBaseClassSpecifiersList(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	BaseClassSpecifier
	//	BaseClassSpecifiersList   ','   BaseClassSpecifier
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	TList *classes_list = NULL;
	if (rlen == 1)
	{
		// Start building the list.
		classes_list = AllocateGeneralPurposeList();
		if (classes_list == NULL)
			return;

		// Append the first base class.
		assert(s1.m_meaning == csm_base_class_spec);
		classes_list->AppendItem(s1.m_base_class_spec);
		s1.ResetMeaning();
	}
	else
	{
		// Pick up the existing list.
		assert(s1.m_meaning == csm_base_classes_list && s1.m_base_classes_list != NULL);
		classes_list = s1.m_base_classes_list;
		s1.ResetMeaning();

		// Pick up the next base class.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
		assert(s3.m_meaning == csm_base_class_spec);

		// Look for this class in the already existing list.
		bool unique_class = TRUE;
		for (TListIter<TCppBaseClassSpec> iter(classes_list); iter; ++iter)
		{
			if (TCppItemBase::CompareSameDatabaseDataTypes(iter.CurrItem().m_base_class, s3.m_base_class_spec->m_base_class) == TRUE)
			{
				// Use full name of the class in the error message.
				TTextBuffer256 name_buffer;
				ReportSyntaxError(&s3, cerr_struct_base_class_duplication, TCppDatabase::GenerateQualifiedName(name_buffer, s3.m_base_class_spec));
				unique_class = FALSE;
				break;
			}
		}

		if (unique_class == TRUE)
		{
			// The class is fine.
			classes_list->AppendItem(s3.m_base_class_spec);
			s3.ResetMeaning();
		}
	}

	// Setup the non terminal.
	new_non_term.m_meaning = csm_base_classes_list;
	new_non_term.m_base_classes_list = classes_list;
}

void TCppParser::ProcessBaseClassSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//							SimpleOrQualifiedId
	//	VIRTUAL					SimpleOrQualifiedId
	//	AccessSpecifier			SimpleOrQualifiedId
	//	VIRTUAL   AccessSpecifier	SimpleOrQualifiedId
	//	AccessSpecifier   VIRTUAL	SimpleOrQualifiedId
	//
	int rlen = rule.Length();
	assert(rlen >= 1 && rlen <= 3);
	TCppSymbolStateItem *sym = FirstRuleSymPtr(rlen);

	TCppBaseClassSpec *base_class_proxy = (TCppBaseClassSpec*)AllocateNewCppItem(cxx_base_class_spec);
	if (base_class_proxy == NULL)
		return;

	// Pick up the props from the optional symbols that can stay in front of the name.
	while (sym->m_app_id == sym_virtual || sym->m_meaning == csm_access_spec)
	{
		if (sym->m_app_id == sym_virtual)
			base_class_proxy->m_virtual = TRUE;
		else if (sym->m_meaning == csm_access_spec)
			base_class_proxy->m_accs_level = sym->m_access_spec;

		sym = NextRuleSymPtr(sym);
		assert(sym != NULL);
	}

	// Ensure that the loop above came to the name symbol.
	assert(sym->m_meaning == csm_simp_or_qual && sym->m_simp_or_qual.m_core.m_info_type <= cnit_template_id);

	// Setup name to the proxy before resolving the name itself.
	sym->m_simp_or_qual.SetNameToObject(base_class_proxy);

	// Pick up the object that is referenced by this name.
	TCppDefnBase *base_class = NULL;
	if (CheckLookupExistingDefinition(sym, elab_none, sym->m_simp_or_qual, sym_BaseClassSpecifier, base_class) == FALSE)
		return;

	// The name was resolved into the struct type, template instantiation or into an unresolved struct type.
	// Note that in the template context base class can also be a class template.
	assert(base_class != NULL && base_class->IsStructType() == TRUE);
	base_class_proxy->m_base_class = (TCppStructTypeDefn*)base_class;

	// Setup the non terminal.
	new_non_term.m_meaning = csm_base_class_spec;
	new_non_term.m_base_class_spec = base_class_proxy;
}

// ----------------------------------------------------------------------------------------------
// ===================  Part 08. Templates  =======================================
// ----------------------------------------------------------------------------------------------

enum P08_Templates { P08_member };					// This enum is used for src code navigation purposes only.

void TCppParser::ProcessGenericTemplateDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	TemplateDeclarationHeader  Declaration
	//	TemplateDeclarationHeader  MemberDeclaration
	//
	//	ExplicitInstantiationHeader  Declaration
	//	ExplicitInstantiationHeader  MemberDeclaration
	//
	// This function contains the word "generic" in its name because it handles rules for 4 different non terminals.
	//
	int rlen = rule.Length();
	assert(rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(s1.m_meaning == csm_template_header);

	if (s1.m_template_info.m_header_processed == FALSE)
	{
		// The template object was not created and an error about that was not reported.
		RaiseCriticalCppError(ccerr_unhandled_template_header);
		return;
	}

	if (s1.m_template_info.m_parameters_list != NULL)
		RecycleGeneralPurposeList(s1.m_template_info.m_parameters_list);

	// Pop the stack of template headers.
	assert(m_curr_template_header == &s1);
	m_curr_template_header = s1.m_template_info.m_parent_template_header;
	s1.ResetMeaning();
}

void TCppParser::ProcessTemplateDeclarationHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//			TEMPLATE   '<'  '>'
	//	EXPORT	TEMPLATE   '<'  '>'
	//			TEMPLATE   '<'  TemplateParametersList  '>'
	//	EXPORT	TEMPLATE   '<'  TemplateParametersList  '>'
	//
	// Note that the list of template params is stored in the second (third) symbol (ParamsListHeader) and not in the list symbol itself.
	//
	int rlen = rule.Length();
	assert(rlen >= 3 && rlen <= 5);
	TCppSymbolStateItem *sym = FirstRuleSymPtr(rlen);

	// Setup empty template header at the beginning.
	new_non_term.m_meaning = csm_template_header;
	new_non_term.m_template_info.Clear();

	if (sym->m_app_id == sym_export)
	{
		// Set the flag and skip the symbol.
		new_non_term.m_template_info.m_export = TRUE;
		sym = NextRuleSymPtr(sym);
		assert(sym != NULL);
	}

	// Skip rule symbol that contains the TEMPLATE keyword.
	sym = NextRuleSymPtr(sym);
	assert(sym != NULL && sym->m_meaning == csm_template_params_list);

	if (sym->m_template_params_info.m_params_list != NULL)
	{
		// Pick up the list from the rule symbol.
		new_non_term.m_template_info.m_parameters_list = sym->m_template_params_info.m_params_list;
	}
	else
	{
		// The header does not contain the list of params. Nevertheless it is still necessary to create the list object.
		// This is needed to differentiate curr case from the explicit instantiation header, that places NULL in this field.
		new_non_term.m_template_info.m_parameters_list = AllocateGeneralPurposeList();
		if (new_non_term.m_template_info.m_parameters_list == NULL)
			return;
	}

	// Pop the stack of declared param lists.
	assert(sym == m_curr_params_list);
	m_curr_params_list = sym->m_template_params_info.m_parent_params_symbol;
	sym->ResetMeaning();

	if (m_curr_template_header != NULL)
		m_curr_template_header->m_template_info.m_child_template_header = &new_non_term;

	// Setup the new template header context.
	new_non_term.m_template_info.m_header_layer_owner = m_defns_layer_owner;
	new_non_term.m_template_info.m_parent_template_header = m_curr_template_header;
	m_curr_template_header = &new_non_term;
}

void TCppParser::ProcessTemplateParamsListHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//		'<'
	//

	// Setup the non terminal as the params list header and push the stack of the nested param lists.
	new_non_term.m_meaning = csm_template_params_list;
	new_non_term.m_template_params_info.m_params_list = NULL;
	new_non_term.m_template_params_info.m_parent_params_symbol = m_curr_params_list;
	m_curr_params_list = &new_non_term;
}

void TCppParser::ProcessTemplateTypeParameter(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	CLASS
	//	CLASS			'='   TypeId
	//	CLASS  identifier
	//	CLASS  identifier	'='   TypeId
	//
	//	TYPENAME
	//	TYPENAME			'='   TypeId
	//	TYPENAME   identifier
	//	TYPENAME   identifier	'='   TypeId
	//
	int rlen = rule.Length();
	assert(rlen >= 1 && rlen <= 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	TCppNameInfo name_info;
	TCppSymbolStateItem *default_value_symbol = NULL;
	if (rlen == 1)
	{
		// None of the optional symbols are available.
		name_info.Clear();
	}
	else
	{
		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		if (s2.m_app_id == sym_identifier)
		{
			// The name of the param is present.
			assert(s2.m_symbol < NonTerminalsBase);
			if (PrepareSimpleNameInfo(s2, name_info) == FALSE)
				return;

			// Check for param names duplication.
			if (CheckUpdateParameterName(&s2, name_info) == FALSE)
				return;

			if (rlen > 2)
			{
				// Default value is present.
				default_value_symbol = NextRuleSymPtr(NextRuleSymPtr(&s2));
				assert(default_value_symbol != NULL);
			}
		}
		else
		{
			// Name of the parameter is missing. But the default value should be present.
			assert(s2.m_app_id == sym_eq && rlen == 3);

			name_info.Clear();
			default_value_symbol = NextRuleSymPtr(&s2);
			assert(default_value_symbol != NULL);
		}
	}

	// The props are verified. Allocate the param object.
	TCppTemplateTypeParamDefn *param_defn = (TCppTemplateTypeParamDefn*)AllocateNewCppItem(cxx_template_type_param, name_info.m_preferred_object_id);
	if (param_defn == NULL)
		return;

	// Allocation went fine. Assign the props.
	name_info.SetNameToObject(param_defn);
	if (default_value_symbol != NULL)
	{
		// Processing default value is simple because the type should be already prepared and resolved.
		assert(default_value_symbol->m_meaning == csm_data_type && default_value_symbol->m_data_type != NULL);
		param_defn->m_default_value = default_value_symbol->m_data_type;
	}

	// Finally give out the object.
	if (PublishParameterObject(param_defn) == FALSE)
	{
		// The error is already reported.
		delete param_defn;
	}
}

void TCppParser::ProcessTemplateTemplateParameter(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	TEMPLATE   '<' TemplateParametersList '>'    ClassKey
	//	TEMPLATE   '<' TemplateParametersList '>'    ClassKey			'='   SimpleOrQualifiedId
	//	TEMPLATE   '<' TemplateParametersList '>'    ClassKey    identifier
	//	TEMPLATE   '<' TemplateParametersList '>'    ClassKey    identifier	'='   SimpleOrQualifiedId
	//
	// Note that the list of template params is stored in the second symbol (ParamsListHeader) and not in the params list symbol itself.
	//
	int rlen = rule.Length();
	assert(rlen >= 5 && rlen <= 8);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2), &s4 = NextRuleSym(s3), &s5 = NextRuleSym(s4);
	assert(s2.m_meaning == csm_template_params_list && s2.m_template_params_info.m_params_list != NULL);

	assert(s5.m_meaning == csm_class_key);
	TCppElabType param_elab = s5.m_class_key;

	TCppNameInfo name_info;
	TCppSymbolStateItem *default_value_symbol = NULL;
	if (rlen == 5)
	{
		// Only the template params are available. The name of the param and the default value are missing.
		name_info.Clear();
	}
	else
	{
		TCppSymbolStateItem &s6 = NextRuleSym(s5);
		if (s6.m_app_id == sym_identifier)
		{
			// The name of the param is present.
			assert(s6.m_symbol < NonTerminalsBase);
			if (PrepareSimpleNameInfo(s6, name_info) == FALSE)
				return;

			// Check for param names duplication.
			if (CheckUpdateParameterName(&s6, name_info) == FALSE)
				return;

			if (rlen > 6)
			{
				// Default value is present.
				default_value_symbol = NextRuleSymPtr(NextRuleSymPtr(&s6));
				assert(default_value_symbol != NULL);
			}
		}
		else
		{
			// Name of the parameter is missing. The default value should be present.
			assert(s6.m_app_id == sym_eq && rlen == 7);

			name_info.Clear();
			default_value_symbol = NextRuleSymPtr(&s6);
			assert(default_value_symbol != NULL);
		}
	}

	TCppClassTemplateDefn *default_value = NULL;
	if (default_value_symbol != NULL)
	{
		// Ensure that default value symbol contains the name and pick up the definition for this name.
		assert(default_value_symbol->m_meaning == csm_simp_or_qual);

		// Place current list of template params into the name info to make the template overload resolition
		// inside the lookup method. This is sort of trick and this is part of the interface to the lookup method.
		// Resolving the overload on this layer is logically better but it leads to code duplication because it will
		// be necessary to issue an error and create subsitution here in case of resoluition problems.
		default_value_symbol->m_simp_or_qual.m_unproc_template_header = &s2;

		TCppDefnBase *base_object = NULL;
		if (CheckLookupExistingDefinition(default_value_symbol, elab_none, default_value_symbol->m_simp_or_qual, sym_TemplateTemplateParameter, base_object) == FALSE)
			return;

		// Returned object should be a class template (or at least an unresolved template).
		default_value_symbol->m_simp_or_qual.m_unproc_template_header = NULL;
		assert(base_object != NULL && (base_object->ItemType() == cxx_class_template || base_object->ItemType() == cxx_unresolved_class_template));
		default_value = (TCppClassTemplateDefn*)base_object;
	}

	TCppTemplateTemplateParamDefn *param_defn = (TCppTemplateTemplateParamDefn*)AllocateNewCppItem(cxx_template_template_param, name_info.m_preferred_object_id);
	if (param_defn == NULL)
		return;

	// Allocation went fine. Assign the props.
	param_defn->m_defn_elab_type = param_elab;
	name_info.SetNameToObject(param_defn);
	param_defn->AddListOfChildren(*s2.m_template_params_info.m_params_list);
	param_defn->m_default_value = default_value;
	RecycleGeneralPurposeList(s2.m_template_params_info.m_params_list);
	s2.m_template_params_info.m_params_list = NULL;

	// Pop the stack of declared param lists.
	assert(&s2 == m_curr_params_list);
	m_curr_params_list = s2.m_template_params_info.m_parent_params_symbol;
	s2.ResetMeaning();

	// Finally give out the object.
	if (PublishParameterObject(param_defn) == FALSE)
	{
		// The error is already reported.
		delete param_defn;
	}
}

void TCppParser::ProcessExplicitInstantiationHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	TEMPLATE
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(s1.m_app_id == sym_template);

	if (m_curr_template_header != NULL)
		m_curr_template_header->m_template_info.m_child_template_header = &new_non_term;

	// New non terminal should contain an explicit instantiation header.
	new_non_term.m_meaning = csm_template_header;
	new_non_term.m_template_info.Clear();
	new_non_term.m_template_info.m_header_layer_owner = m_defns_layer_owner;
	new_non_term.m_template_info.m_parent_template_header = m_curr_template_header;
	m_curr_template_header = &new_non_term;
}

void TCppParser::ProcessTemplateId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	identifier  '<' '>'
	//	identifier  '<' TemplateArgumentsList '>'
	//
	// Note that the list of template params if any is stored in the second symbol that is ParamsListHeader and
	// not in the arguments list symbol itself that does not have any meaning.
	//
	int rlen = rule.Length();
	assert(rlen == 3 || rlen == 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);

	// Set up the non terminal as a simple identifier. A few lines later its subtype will be changed.
	// Besides other things this procedure will setup the key id and the unprocessed template header
	// pointer. These fields are needed and they will not be modified below.
	new_non_term.m_meaning = csm_simp_or_qual;
	if (PrepareSimpleNameInfo(s1, new_non_term.m_simp_or_qual) == FALSE)
		return;

	// Pick up the list of template arguments. Note that it can be NULL.
	assert(s2.m_meaning == csm_template_params_list);
	TList *args_list = s2.m_template_params_info.m_params_list;

	// Pop the stack of opened param lists.
	assert(&s2 == m_curr_params_list);
	m_curr_params_list = s2.m_template_params_info.m_parent_params_symbol;
	s2.ResetMeaning();

	// Create a persistent name for this object. Note that this name will never be used for comparing
	// different template instantiations or other similar things.
	wchar_t *cached_name = m_cpp_database.CreateTemplateIdName(new_non_term.m_simp_or_qual.m_cached_name, args_list);
	if (cached_name == NULL)
	{
		RaiseCriticalCppError(ccerr_oom_generating_object_name, L"a template id");
		if (args_list != NULL)
			RecycleGeneralPurposeList(args_list);
		return;
	}

	// Finalize the non term.
	new_non_term.m_simp_or_qual.m_core.m_info_type = cnit_template_id;
	new_non_term.m_simp_or_qual.FinalizeShortNameInfo(this, new_non_term, cached_name, args_list);
}

void TCppParser::ProcessTemplateArgument(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	AssignmentExpression
	//	TypeId
	//	SimpleOrQualifiedId				<-- This should be the name of the class template.
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	TCppItemBase *arg_object = NULL;
	if (s1.m_meaning == csm_expression)
	{
		// Pick up the expression. The expression object itself will be used as an argument.
		assert(s1.m_expression != NULL);
		arg_object = s1.m_expression;
		s1.ResetMeaning();
	}
	else if (s1.m_meaning == csm_data_type)
	{
		// Create a template type argument wrapper.
		TCppTemplateTypeArgument *new_type_arg = (TCppTemplateTypeArgument*)AllocateNewCppItem(cxx_template_type_argument);
		if (new_type_arg == NULL)
			return;

		assert(s1.m_data_type != NULL);
		new_type_arg->m_argument_value = s1.m_data_type;
		arg_object = new_type_arg;
	}
	else
	{
		// Create a template template argument wrapper.
		assert(s1.m_meaning == csm_simp_or_qual);
		TCppTemplateTemplateArgument *new_templ_arg = (TCppTemplateTemplateArgument*)AllocateNewCppItem(cxx_template_template_argument);
		if (new_templ_arg == NULL)
			return;

		TCppDefnBase *base_object = NULL;
		if (CheckLookupExistingDefinition(&s1, elab_none, s1.m_simp_or_qual, sym_TemplateArgument, base_object) == FALSE)
		{
			delete new_templ_arg;
			return;
		}

		// Returned object should be a class template (or at least an unresolved template).
		assert(base_object != NULL && (base_object->ItemType() == cxx_class_template || base_object->ItemType() == cxx_unresolved_class_template));

		// -BUGBUG- Assemble the structure as an existing template for now.
		new_templ_arg->m_arg_type = TCppTemplateTemplateArgument::ctarg_template_defn;
		new_templ_arg->m_argument_value = (TCppClassTemplateDefn*)base_object;
		arg_object = new_templ_arg;
	}

	// Give out the object.
	if (PublishParameterObject(arg_object) == FALSE)
	{
		// The error is already reported.
		delete arg_object;
	}
}

// ----------------------------------------------------------------------------------------------
// ===================  Part 09. Functions  =======================================
// ----------------------------------------------------------------------------------------------

enum P09_Functions { P09_member };					// This enum is used for src code navigation purposes only.

void TCppParser::ProcessFunctionDefinition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	FunctionHeader							CompoundStatement
	//	FunctionHeader		':'	MemberInitsList	CompoundStatement
	//	FunctionHeader   TRY						CompoundStatement	 ExceptionHandlersSeq
	//	FunctionHeader   TRY	':'	MemberInitsList	CompoundStatement	 ExceptionHandlersSeq
	//
	int rlen = rule.Length();
	assert(rlen == 2 || rlen == 4 || rlen == 6);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);
	assert(s1.m_meaning == csm_function_header && s1.m_function_header != NULL);

	TCppCodeEntryPointDefn *func_object = s1.m_function_header;
	if (rlen == 2)
	{
		// Everything is already published. Just reset the meanings.
		assert(s2.m_meaning == csm_statement && s2.m_statement != NULL);
		s1.ResetMeaning();
		s2.ResetMeaning();
	}
	else
	{
		// Reset meanings in other symbols.
		TCppSymbolStateItem &s3 = NextRuleSym(s2), &s4 = NextRuleSym(s3);
		if (s2.m_app_id != sym_try)
		{
			assert(s4.m_meaning == csm_statement && s4.m_statement != NULL);
			s1.ResetMeaning();
			s4.ResetMeaning();
		}
		else
		{
			// Current function is using the function TRY block.
			TCppTryBlockDefn *try_block = NULL;
			TCppLightBlockDefn *light_block = NULL;
			if (rlen == 4)
			{
				assert(s3.m_meaning == csm_statement && s3.m_statement != NULL && s3.m_statement->ItemType() == cxx_try_block);
				try_block = (TCppTryBlockDefn*)s3.m_statement;
				light_block = s4.m_catch_blocks_seq;
				s3.ResetMeaning();
				s4.ResetMeaning();
			}
			else
			{
				TCppSymbolStateItem &s5 = NextRuleSym(s4), &s6 = NextRuleSym(s5);
				assert(s5.m_meaning == csm_statement && s5.m_statement != NULL && s5.m_statement->ItemType() == cxx_try_block);
				try_block = (TCppTryBlockDefn*)s5.m_statement;
				light_block = s6.m_catch_blocks_seq;
				s5.ResetMeaning();
				s6.ResetMeaning();
			}

			// The TRY symbol should contain the body of the function.
			assert(s2.m_meaning == csm_block_header && s2.m_block_info.m_block_header != NULL);

			// Assemble the function body. Note that body is already attached to the code entry point.
			s2.m_block_info.m_block_header->AddChildItem(try_block);
			s2.m_block_info.m_block_header->AddListOfChildren(light_block->m_members);
			delete light_block;

			// Exit the function body.
			ExitCurrentDefnsLayer(&s2);
			s1.ResetMeaning();
			s2.ResetMeaning();
		}
	}

	// Check for goto statements that point to undefined labels.
	while (m_unplaced_labels.IsEmpty() == FALSE)
	{
		TCppLabelDefn *unplaced_label = (TCppLabelDefn*)m_unplaced_labels.GetFirst();
		ReportSyntaxError(LastStackSymPtr(), cerr_label_is_undefined, unplaced_label->DefnName());

		m_unplaced_labels.RemoveItem(unplaced_label);
		func_object->m_function_body->AddChildItem(unplaced_label);
	}
}

void TCppParser::ProcessFunctionHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//					Declarator			<--   The declarator should be: ctor, dector or overloaded conversion.
	//	DeclTypeSpecifier	Declarator			<--   This should be either a regular function or an overlodaed operator.
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	TCppCodeEntryPointDefn *new_function = NULL;
	if (rlen == 1)
	{
		// This is special function.
		assert(s1.m_meaning == csm_declarator);

		assert(m_curr_decl_type_spec == NULL);
		new_function = (TCppCodeEntryPointDefn*)DoHandleGenericDeclarator(s1, TRUE);
		if (new_function == NULL)
			return;
	}
	else
	{
		// Process regular defintion of the function.
		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		assert(s2.m_meaning == csm_declarator);

		assert(m_curr_decl_type_spec != NULL && m_curr_decl_type_spec == &s1);
		new_function = (TCppCodeEntryPointDefn*)DoHandleGenericDeclarator(s2, TRUE);
		m_curr_decl_type_spec = NULL;

		if (new_function == NULL)
			return;
	}

	// Processing may result in an already existing function prototype. If object is just created, it should be published.
	assert(new_function->IsFunctionFamily() == TRUE && new_function->m_function_body == NULL);
	if (new_function->IsPublished() == FALSE)
	{
		// Publish this new function.
		PublishOnTheCurrentLayer(new_function);
	}

	// Setup the non terminal.
	assert(new_function->m_function_body == NULL);
	new_non_term.m_meaning = csm_function_header;
	new_non_term.m_function_header = new_function;
}

void TCppParser::ProcessMemberInitializer(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	SimpleOrQualifiedId  '('  ')'
	//	SimpleOrQualifiedId  '('  ExpressionsList  ')'
	//
	// Processing significantly depends on symbols that stay in front of the symbols of this rule.
	//
	// The following combinations of symbols are possible:
	//
	//	FunctionHeader		':'	MemberInitializer			<--   Create the function body Block. Publish initializer inside this block.
	//	FunctionHeader   TRY	':'	MemberInitializer			<--   Create the function body Block and the TryBlock. Publish initializer inside them.
	//
	//	MemberInitsList   ','   MemberInitializer					<--   Appropriate blocks are already created. Just publish the initializer on the current layer.
	//
	int rlen = rule.Length();
	assert(rlen == 3 || rlen == 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
	assert(s1.m_meaning == csm_simp_or_qual);

	// Do the tricky part. It is necessary to check symbols that stay right in front of the symbols of this rule.
	TCppSymbolStateItem *prev_sym = PrevRuleSymPtr(&s1);
	assert(prev_sym != NULL);

	// First discovered symbol should be either a colon or a comma.
	if (prev_sym->m_app_id == sym_colon)
	{
		// This is beginning of the member initializers.
		TCppSymbolStateItem *colon_symbol = prev_sym;

		// Check what stands in front of the colon.
		prev_sym = PrevRuleSymPtr(prev_sym);
		assert(prev_sym != NULL);

		TCppSymbolStateItem *try_symbol = NULL;
		TCppTryBlockDefn *function_try_block = NULL;
		if (prev_sym->m_app_id == sym_try)
		{
			// It turns out that the source program contains the function try block.
			try_symbol = prev_sym;
			function_try_block = (TCppTryBlockDefn*)AllocateNewCppItem(cxx_try_block);
			if (function_try_block == NULL)
				return;

			// Shift to the symbol that stays in front of the TRY keyword.
			prev_sym = PrevRuleSymPtr(prev_sym);
			assert(prev_sym != NULL);
		}

		// Discovered symbol should be the function header. Ensure this.
		assert(prev_sym->m_meaning == csm_function_header && prev_sym->m_function_header != NULL);

		// Create a body for this ctor function.
		TCppBlockDefn *function_body = CreateAndPrepareFunctionBody(prev_sym->m_function_header);
		if (function_body == NULL)
		{
			if (function_try_block != NULL)
				delete function_try_block;
			return;
		}

		if (try_symbol != NULL)
		{
			// Setup special meaning to the TRY symbol. Note that this meaning will become a simple block, not the TRY block.
			try_symbol->m_meaning = csm_block_header;
			try_symbol->m_sym_meaning_ex = try_symbol->m_meaning;
			try_symbol->m_block_info.m_block_header = function_body;
			try_symbol->m_block_info.m_block_object_published = TRUE;
			EnterDefinitionsLayer(function_body, *try_symbol);

			// Setup special meaning to the colon symbol using the TRY block.
			colon_symbol->m_meaning = csm_block_header;
			colon_symbol->m_sym_meaning_ex = colon_symbol->m_meaning;
			colon_symbol->m_block_info.m_block_header = function_try_block;
			colon_symbol->m_block_info.m_block_object_published = FALSE;
			EnterDefinitionsLayer(function_try_block, *colon_symbol);
		}
		else
		{
			// Setup special meaning to the colon symbol.
			colon_symbol->m_meaning = csm_block_header;
			colon_symbol->m_sym_meaning_ex = colon_symbol->m_meaning;
			colon_symbol->m_block_info.m_block_header = function_body;
			colon_symbol->m_block_info.m_block_object_published = TRUE;
			EnterDefinitionsLayer(function_body, *try_symbol);
		}
	}
	else
	{
		// This should be the comma, that should stay after the previous initializer. Appropriate blocks are already created.
		assert(prev_sym->m_app_id == sym_comma);
	}

	// Assemble the initializer. Create the function call expression object for this.
	TCppFunctionCallExpr *member_init_fun_call = (TCppFunctionCallExpr*)AllocateNewCppItem(cxx_function_call_expr);
	if (member_init_fun_call == NULL)
		return;

	// Create the operand to reference the member that is being initialized.
	TCppOperandExpr *member_item = (TCppOperandExpr*)AllocateNewCppItem(cxx_operand_expr);
	if (member_item == NULL)
	{
		delete member_init_fun_call;
		return;
	}

	/* -BUGBUG-: Resolve the simp or qual id.
	// For now the oprand object will point to nowhere.
	member_item->SetDataField((TCppDataFieldDefn*)s1.m_simp_or_qual.m_primary_defn);
	*/

	// Assemble the data structure.
	member_init_fun_call->AddChildItem(member_item, cexpr_chdest_fcall_func);

	if (rlen == 4)
	{
		// Non empty params are present. Add them to the expresssion.
		member_init_fun_call->AddCallParameters(s3.m_expression);
		s3.ResetMeaning();
	}

	// Release the initializer.
	PublishOnTheCurrentLayer(member_init_fun_call);
}

void TCppParser::ProcessOperatorFunctionId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	OPERATOR  OverloadableOperator
	//	OPERATOR  OverloadableOperator  '<'  '>'
	//	OPERATOR  OverloadableOperator  '<'  TemplateArgumentsList  '>'
	//
	// Note that the list of template params is stored in the third symbol (ParamsListHeader) and not in the params list symbol itself.
	//
	int rlen = rule.Length();
	assert(rlen == 2 || rlen == 4 || rlen == 5);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);
	assert(s2.m_meaning = csm_overloadable_operator);

	// Allocate local vars and assign values that correspond to the non template form of the overloadable operator.
	TCppNameInfoType ovld_op_type = cnit_ovld_operator;
	bool ovld_op_template_form = FALSE;
	TList *args_list = NULL;

	// Check for optional symbols.
	if (rlen > 2)
	{
		// This is template form of the overloadable operator.
		ovld_op_type = cnit_ovld_op_template;
		ovld_op_template_form = TRUE;

		// Pick up the list of params list if any.
		TCppSymbolStateItem &s3 = NextRuleSym(s2);
		assert(s3.m_meaning == csm_template_params_list);
		args_list = s3.m_template_params_info.m_params_list;

		// Pop the stack of declared param lists.
		assert(&s3 == m_curr_params_list);
		m_curr_params_list = s3.m_template_params_info.m_parent_params_symbol;
		s3.ResetMeaning();
	}

	// Create persistent name for this object.
	wchar_t *cached_name = m_cpp_database.CreateOverloadedOperatorName(s2.m_overloadable_operator, ovld_op_template_form, args_list);
	if (cached_name == NULL)
	{
		RaiseCriticalCppError(ccerr_oom_generating_object_name, L"an overloaded operator");
		if (args_list != NULL)
			RecycleGeneralPurposeList(args_list);
		return;
	}

	// Setup the non terminal fields.
	new_non_term.m_meaning = csm_simp_or_qual;
	new_non_term.m_simp_or_qual.SetupOverloadedOperator(ovld_op_type, s2.m_overloadable_operator);
	new_non_term.m_simp_or_qual.m_unproc_template_header = GetFirstTemplateHeaderSymbol();
	new_non_term.m_simp_or_qual.FinalizeShortNameInfo(this, new_non_term, cached_name, args_list);
}

void TCppParser::ProcessOverloadableOperator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	 NEW	|  NEW '[' ']'
	//	 DELETE	|  DELETE '[' ']'
	//
	//	 '+'		|  '-'	| '*'		|  '/'	|  '%'	|  '^'	|  '&'	|  '|'	|  '~'
	//	 '!'		|  '='	| '<'		|  '>'	|  '+='	|  '-='	|  '*='	|  '/='	|  '%='
	//	 '^='	|  '&='	| '|='	|  '<<'	|  '>>'	|  '<<='	|  '>>='	|  '=='	|  '!='
	//	 '<='	|  '>='	| '&&'	|  '||'	|  '++'	|  '--'	|  ','		|  '->*'	|  '->'
	//
	//	 '(' ')'	|  '[' ']'
	//
	int rlen = rule.Length();
	assert(rlen >= 1 && rlen <= 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Misc operations.
		switch (s1.m_app_id)
		{
			case sym_new:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_new;
									break;
			case sym_delete:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_delete;
									break;

			case sym_plus:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_plus;
									break;
			case sym_minus:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_minus;
									break;
			case sym_mul:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_mul;
									break;
			case sym_div:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_div;
									break;
			case sym_rmnd:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_rmnd;
									break;
			case sym_bitxor:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_bitxor;
									break;
			case sym_bitand:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_bitand;
									break;
			case sym_bitor:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_bitor;
									break;
			case sym_tilda:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_tilda;
									break;

			case sym_bang:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_bang;
									break;
			case sym_assign:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_assign;
									break;
			case sym_lt:				new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_lt;
									break;
			case sym_gt:				new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_gt;
									break;
			case sym_plus_asgn:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_plus_asgn;
									break;
			case sym_minus_asgn:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_minus_asgn;
									break;
			case sym_mul_asgn:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_mul_asgn;
									break;
			case sym_div_asgn:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_div_asgn;
									break;
			case sym_rmnd_asgn:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_rmnd_asgn;
									break;

			case sym_btxor_asgn:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_btxor_asgn;
									break;
			case sym_btand_asgn:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_btand_asgn;
									break;
			case sym_btor_asgn:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_btor_asgn;
									break;
			case sym_bitlsh:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_bitlsh;
									break;
			case sym_bitrsh:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_bitrsh;
									break;
			case sym_lsh_asgn:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_lsh_asgn;
									break;
			case sym_rsh_asgn:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_rsh_asgn;
									break;
			case sym_eq:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_eq;
									break;
			case sym_ne:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_ne;
									break;

			case sym_le:				new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_le;
									break;
			case sym_ge:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_ge;
									break;
			case sym_logand:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_logand;
									break;
			case sym_logor:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_logor;
									break;
			case sym_plusplus:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_plusplus;
									break;
			case sym_minusminus:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_minusminus;
									break;
			case sym_comma:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_comma;
									break;
			case sym_arrowstar:		new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_arrowstar;
									break;
			case sym_arrow:			new_non_term.m_meaning = csm_overloadable_operator;
									new_non_term.m_overloadable_operator = opr_arrow;
									break;

			default:
				RaiseUnexpectedRuleSymbolAppIdError(irule, &s1);
				break;
		}
	}
	else if (rlen == 2)
	{
		// This is a function call operation or an array index operation.
		switch (s1.m_app_id)
		{
			case sym_lpar:		new_non_term.m_meaning = csm_overloadable_operator;
								new_non_term.m_overloadable_operator = opr_funcall;
								break;
			case sym_lbracket:	new_non_term.m_meaning = csm_overloadable_operator;
								new_non_term.m_overloadable_operator = opr_arr_index;
								break;
			default:
				RaiseUnexpectedRuleSymbolAppIdError(irule, &s1);
				break;
		}
	}
	else
	{
		// New array operation and delete array operation.
		switch (s1.m_app_id)
		{
			case sym_new:		new_non_term.m_meaning = csm_overloadable_operator;
								new_non_term.m_overloadable_operator = opr_new_array;
								break;
			case sym_delete:		new_non_term.m_meaning = csm_overloadable_operator;
								new_non_term.m_overloadable_operator = opr_delete_array;
								break;
			default:
				RaiseUnexpectedRuleSymbolAppIdError(irule, &s1);
				break;
		}
	}
}

void TCppParser::ProcessConversionFunctionId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	OPERATOR  ConversionTypeId
	//	OPERATOR  ConversionTypeId   '<'  '>'
	//	OPERATOR  ConversionTypeId   '<'  TemplateArgumentsList  '>'
	//
	// Note that the list of template params is stored in the third symbol (ParamsListHeader) and not in the params list symbol itself.
	//
	int rlen = rule.Length();
	assert(rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);
	assert(s2.m_meaning == csm_data_type && s2.m_data_type != NULL);

	// Allocate local vars and assign values that correspond to the non template form of the overloaded conversion.
	TCppNameInfoType ovld_conv_type = cnit_ovld_conversion;
	bool ovld_conv_template_form = FALSE;
	TList *args_list = NULL;

	// Check for optional symbols.
	if (rlen > 2)
	{
		// This is template form of the overloaded conversion.
		ovld_conv_type = cnit_ovld_conv_template;
		ovld_conv_template_form = TRUE;

		// Pick up the list of params if any.
		TCppSymbolStateItem &s3 = NextRuleSym(s2);
		assert(s3.m_meaning == csm_template_params_list);
		args_list = s3.m_template_params_info.m_params_list;

		// Pop the stack of declared param lists.
		assert(&s3 == m_curr_params_list);
		m_curr_params_list = s3.m_template_params_info.m_parent_params_symbol;
		s3.ResetMeaning();
	}

	// Create persistent name for this object.
	wchar_t *cached_name = m_cpp_database.CreateOverloadedConversionName(s2.m_data_type, ovld_conv_template_form, args_list);
	if (cached_name == NULL)
	{
		RaiseCriticalCppError(ccerr_oom_generating_object_name, L"an overloaded conversion");
		if (args_list != NULL)
			RecycleGeneralPurposeList(args_list);
		return;
	}

	// Setup the simp_or_qual meaning without doing any lookup procedure.
	new_non_term.m_meaning = csm_simp_or_qual;
	new_non_term.m_simp_or_qual.SetupOverloadedConversion(ovld_conv_type, s2.m_data_type);
	new_non_term.m_simp_or_qual.m_unproc_template_header = GetFirstTemplateHeaderSymbol();
	new_non_term.m_simp_or_qual.FinalizeShortNameInfo(this, new_non_term, cached_name, args_list);
}

void TCppParser::ProcessConversionTypeId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	TypeSpecifier
	//	TypeSpecifier  ConversionDeclarator
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	HandleGenericTypeIdSymbols(new_non_term, FirstRuleSym(rlen), rlen);
}

void TCppParser::ProcessConversionDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	PtrOperator
	//	ConversionDeclarator  PtrOperator
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	new_non_term.m_meaning = csm_declarator;
	TCppSymbolStateItem *ptr_opr_symbol = NULL;
	if (rlen == 1)
	{
		// This is short form of the rule. Start building declarator from scratch.
		new_non_term.m_declarator_info.Clear();

		assert(s1.m_meaning == csm_ptr_operator);
		ptr_opr_symbol = &s1;
	}
	else
	{
		// This is long form of the rule. Inherit already existing declarator.
		assert(s1.m_meaning == csm_declarator);
		new_non_term.m_declarator_info = s1.m_declarator_info;
		s1.ResetMeaning();

		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		assert(s2.m_meaning == csm_ptr_operator);
		ptr_opr_symbol = &s2;
	}

	// Apply the modifier.
	ApplyCvPtrRefModifier(ptr_opr_symbol, new_non_term.m_declarator_info, ptr_opr_symbol->m_ptr_operator);
	ptr_opr_symbol->ResetMeaning();
}

// ----------------------------------------------------------------------------------------------
// ===================  Part 10. Statements  ======================================
// ----------------------------------------------------------------------------------------------

enum P10_Statements { P10_member };					// This enum is used for src code navigation purposes only.

void TCppParser::ProcessStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	CompoundStatement
	//	LabeledStatement					<--   Meaning of the symbol always contains the LightBlock object.
	//	BlockDeclaration					<--   Meaning of this symbol is csm_declaration.
	//	ExpressionStatement
	//	SelectionStatement
	//	IterationStatement
	//	JumpStatement
	//	TryBlock							<--   Meaning of the symbol always contains the LightBlock object.
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	// Ensure that the current enclosure is block.
	assert(m_defns_layer_owner->IsBlock() == TRUE);

	if (s1.m_meaning == csm_statement)
	{
		assert(s1.m_statement != NULL);

		// Simply promote the existing meaning up.
		new_non_term.m_meaning = csm_statement;
		new_non_term.m_statement = s1.m_statement;
	}
	else
	{
		// This should be the block level declaration. Convert declaration into the statement.
		if (s1.m_meaning == csm_declaration)
		{
			assert(s1.m_declaration != NULL);
			new_non_term.m_meaning = csm_statement;
			new_non_term.m_statement = s1.m_declaration;
		}
		else
		{
			assert(s1.m_meaning == csm_declarations_list && s1.m_declarations_list != NULL);
			new_non_term.m_meaning = csm_statement;
			new_non_term.m_declarations_list = s1.m_declarations_list;
		}

	}

	// Data from the symbol of the rule is already removed.
	s1.ResetMeaning();
}

void TCppParser::ProcessCompoundStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	CompoundStatementHeader					'}'
	//	CompoundStatementHeader    StatementsSeq	'}'
	//
	// Processing of this rule depends on symbols that stay in front of symbols of this rule.The following combinations
	// are possible:
	//
	//		':' MemberInitsList
	//		"Something-else"
	//
	int rlen = rule.Length();
	assert(rlen == 2 || rlen == 3);
	TCppSymbolStateItem *sym = FirstRuleSymPtr(rlen);
	assert(sym->m_meaning == csm_none || sym->m_meaning == csm_block_header);

	TCppSymbolStateItem *layer_symbol = NULL;
	if (sym->m_meaning == csm_block_header)
	{
		// This is simple case.
		layer_symbol = sym;
	}
	else
	{
		// Pick up the block pointer from second symbol in front.
		sym = PrevRuleSymPtr(sym);
		assert(sym != NULL && sym->m_app_id == sym_MemberInitsList);
		sym = PrevRuleSymPtr(sym);
		assert(sym != NULL && sym->m_app_id == sym_colon && sym->m_meaning == csm_block_header);
		layer_symbol = sym;
	}

	// Pop up one layer of definitions.
	assert(layer_symbol->m_block_info.m_block_header != NULL);
	ExitCurrentDefnsLayer(layer_symbol);

	// Setup the non terminal.
	new_non_term.m_meaning = csm_statement;
	new_non_term.m_statement = layer_symbol->m_block_info.m_block_header;
	layer_symbol->ResetMeaning();
}

void TCppParser::ProcessCompoundStatementHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//		'{'
	//
	// This rule controls the block entrance. It looks simple. In reality its processing depends on the symbols
	// that stay in front of the curved bracket. The following combinations are possible:
	//
	//	FunctionHeader							'{'		<--   Create simple Block and enter it.
	//	FunctionHeader		':'	MemberInitsList	'{'		<--   Do not do anything. Block was created when MemberInitsList was processed.
	//	FunctionHeader   TRY						'{'		<--   Create pair of simple Block and TryBlock. Enther both blocks.
	//	FunctionHeader   TRY	':'	MemberInitsList	'{'		<--   Do not do anything. Both blocks were created when MemberInitsList was processed.
	//
	//	TRY						'{'		<--   Create TryBlock and enter it.
	//	ExceptionHandlerHeader	'{'		<--   Create CatchBlock and enter it.
	//	"Nothing-From-Above"		'{'		<--   Create simple code Block and enter it.
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem *prev_sym = FirstRuleSymPtr(rlen+1);
	assert(prev_sym != NULL);

	TCppBlockDefn *new_block = NULL;
	bool new_block_published = FALSE;
	if (prev_sym->m_meaning == csm_function_header)
	{
		// Create the body of the function. Code after this if statement will do the rest of the work.
		assert(prev_sym->m_function_header != NULL);
		new_block = CreateAndPrepareFunctionBody(prev_sym->m_function_header);
		new_block_published = TRUE;
	}
	else if (prev_sym->m_app_id == sym_MemberInitsList)
	{
		// This one of two forms of ctor that include member initializers. One block or two blocks are already
		// created and entered. There is no need to do anything else here.
		return;
	}
	else if (prev_sym->m_app_id == sym_try)
	{
		// Check one more symbol in front.
		TCppSymbolStateItem *prev_prev_sym = PrevRuleSymPtr(prev_sym);
		if (prev_prev_sym != NULL && prev_prev_sym->m_meaning == csm_function_header)
		{
			// This is function try block without member initializers. Two blocks are needed.
			assert(prev_prev_sym->m_function_header != NULL);
			TCppBlockDefn *function_body_block = CreateAndPrepareFunctionBody(prev_prev_sym->m_function_header);
			if (function_body_block == NULL)
				return;

			// Create TRY block that will be inserted inside the function body.
			new_block = (TCppTryBlockDefn*)AllocateNewCppItem(cxx_try_block);
			if (new_block == NULL)
			{
				// Do not delete the function body because it is already owned by the published code entry point.
				return;
			}

			// Setup the TRY symbol that is part of the stack. Note this will change the meaning of this symbol.
			prev_sym->m_meaning = csm_block_header;
			prev_sym->m_sym_meaning_ex = prev_sym->m_meaning;
			prev_sym->m_block_info.m_block_header = function_body_block;
			prev_sym->m_block_info.m_block_object_published = TRUE;
			EnterDefinitionsLayer(function_body_block, *prev_sym);
		}
		else
		{
			// This is TRY block inside the body of the function.
			new_block = (TCppTryBlockDefn*)AllocateNewCppItem(cxx_try_block);
		}
	}
	else if (prev_sym->m_meaning == csm_exception_param)
	{
		// Create the catch block and add exception defention to its members.
		TCppCatchBlockDefn *catch_block = (TCppCatchBlockDefn*)AllocateNewCppItem(cxx_catch_block);
		if (catch_block == NULL)
			return;

		assert(prev_sym->m_exception_param != NULL);
		catch_block->AddChildItem(prev_sym->m_exception_param);
		catch_block->m_exception = prev_sym->m_exception_param;
		prev_sym->ResetMeaning();

		// Give the block out.
		new_block = catch_block;
	}
	else
	{
		// This is nested block inside of the function body.
		new_block = (TCppBlockDefn*)AllocateNewCppItem(cxx_block);
	}

	if (new_block == NULL)
	{
		// Critical error happened. Do not do anything.
		return;
	}

	// Setup the non terminal.
	new_non_term.m_meaning = csm_block_header;
	new_non_term.m_block_info.m_block_header = new_block;
	new_non_term.m_block_info.m_block_object_published = new_block_published;
	EnterDefinitionsLayer(new_block, new_non_term);
}

void TCppParser::ProcessStatementsSeq(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	Statement
	//	StatementsSeq  Statement
	//
	// Sequence of statements can be present only directly inside the block. Publish the statement
	// in the current block. The new non terminal does not have any meaning.
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	// Pick up the statement.
	assert(m_defns_layer_owner->IsBlock() == TRUE);
	TCppStatementBase *stmt = NULL;
	if (rlen == 1)
	{
		assert(s1.m_meaning == csm_statement && s1.m_statement != NULL);
		stmt = s1.m_statement;
		s1.ResetMeaning();
	}
	else
	{
		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		assert(s2.m_meaning == csm_statement && s2.m_statement != NULL);
		stmt = s2.m_statement;
		s2.ResetMeaning();
	}

	// Publish the statement if it is not empty.
	if (stmt->ItemType() != cxx_empty_stmt)
	{
		if (stmt->IsPublished() == FALSE)
			PublishOnTheCurrentLayer(stmt);
	}
	else
	{
		// This statement is not needed.
		delete stmt;
	}
}

void TCppParser::ProcessLabeledStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	identifier ':'				 Statement
	//	CASE ConstantExpression ':'	 Statement
	//	DEFAULT ':'				 Statement
	//
	int rlen = rule.Length();
	assert(rlen == 3 || rlen == 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);

	// Prepare the two parts of the final statement.
	TCppDefnBase *base_label = NULL;
	TCppStatementBase *base_stmt = NULL;

	if (s1.m_app_id == sym_identifier)
	{
		// This is definition of the named label. Pick up the key_id for this identifier.
		TCppNameInfo name_info;
		if (PrepareSimpleNameInfo(s1, name_info) == FALSE)
			return;

		// Worker fuction will return either an existing label or create a new one.
		TCppLabelDefn *named_label = FindOrCreateNamedLabel(name_info, FALSE);
		if (named_label == NULL)
			return;

		if (named_label->IsPublished() == TRUE)
		{
			// Retruned object is already published. This means that label with this name is already present
			// in the code of the function.
			ReportSyntaxError(&s1, cerr_label_already_defined, named_label->DefnName());

			// The label is still needed. Prepare the modified name.
			if (name_info.SetupShortNameDefnProblem(this, cdpr_dupe) == FALSE)
				return;

			// Create the new label object.
			named_label = (TCppLabelDefn*)AllocateNewCppItem(cxx_label, name_info.m_preferred_object_id);
			if (named_label == NULL)
				return;

			// Set up the alt name there.
			name_info.SetNameToObject(named_label);
			named_label->m_defined = TRUE;
		}

		// Give the label object out.
		base_label = named_label;

		// Pick up the stmt from the third symbol of the rule.
		assert(rlen == 3 && s3.m_meaning == csm_statement);
		base_stmt = s3.m_statement;
		s3.ResetMeaning();
	}
	else
	{
		// This is a case label. Create a new case label object.
		TCppCaseLabelDefn *case_label = (TCppCaseLabelDefn*)AllocateNewCppItem(cxx_case_label);
		if (case_label == NULL)
			return;

		if (rlen == 4)
		{
			// Second symbol of the rule should contain a const expression. Ensure this.
			assert(s2.m_meaning == csm_expression && s2.m_expression != NULL);

			// Try to compute this expression.
			TCppOperandExpr case_label_operand;
			if (s2.m_expression->GetConstOperandValue(case_label_operand) == FALSE)
			{
				// The expression is not constant.
				if (IsTemplateBodyContext() == FALSE)
					ReportSyntaxError(&s2, cerr_case_label_not_const);
			}
			else if (case_label_operand.ConvertToNumericLiteral() == FALSE)
			{
				// The expr is constant but the constant value is not numeric.
				if (IsTemplateBodyContext() == FALSE)
					ReportSyntaxError(&s2, cerr_case_label_not_numeric);
			}
			else
			{
				// Conversion to the numeric value went fine. Setup the label.
				case_label->m_const_value = TRUE;
				case_label->m_value = case_label_operand.m_num_value;
				CheckReportCaseLabelProblems(&s2, case_label);
			}

			// Move arithm expression into the case label object regardless if it is const or not.
			case_label->AddChildItem(s2.m_expression);
			s2.ResetMeaning();

			// The statement should be in the forth symbol of the rule.
			TCppSymbolStateItem &s4 = NextRuleSym(s3);
			assert(s4.m_meaning == csm_statement);

			base_stmt = s4.m_statement;
			s4.ResetMeaning();
		}
		else
		{
			// Situation is more simple. This is default case label.
			case_label->m_default = TRUE;
			CheckReportCaseLabelProblems(&s1, case_label);

			// Statement should be in the third stymbol.
			assert(rlen == 3 && s3.m_meaning == csm_statement);
			base_stmt = s3.m_statement;
			s3.ResetMeaning();
		}

		// Props of the label are set. Show them in the name of the label.
		if (m_cpp_database.PrepareAndSetCaseLabelName(case_label) == FALSE)
		{
			RaiseCriticalCppError(ccerr_oom_generating_object_name, L"a case label");
			delete case_label;
			return;
		}

		// Give the label object out.
		base_label = case_label;
	}

	// Check the type of the base statement.
	assert(base_label->ItemParent() == NULL);
	if (base_stmt->ItemType() != cxx_light_block)
	{
		// A new light block is needed.
		TCppLightBlockDefn *light_block = (TCppLightBlockDefn*)AllocateNewCppItem(cxx_light_block);
		if (light_block == NULL)
		{
			delete base_label;
			delete base_stmt;
			return;
		}

		// Add label and statement into the newly created block.
		light_block->AddChildItem(base_label);
		light_block->AddChildItem(base_stmt);
		base_stmt = light_block;
	}
	else
	{
		// Simply prepend this label to the contents of the already existing light block.
		TCppLightBlockDefn *light_block = (TCppLightBlockDefn*)base_stmt;
		light_block->AddChildItem(base_label, cstmt_chdest_ltblk_prepend);
	}

	// Assemble the non term.
	new_non_term.m_meaning = csm_statement;
	new_non_term.m_statement = base_stmt;
}

void TCppParser::ProcessExpressionStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	ExpressionsList  ';'
	//
	int rlen = rule.Length();
	assert(rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);
	assert(s1.m_meaning == csm_expression && s1.m_expression != NULL);

	// Promote existing expression up as statement.
	new_non_term.m_meaning = csm_statement;
	new_non_term.m_statement = s1.m_expression;
	s1.ResetMeaning();
}

void TCppParser::ProcessSelectionStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	IF  '(' Condition ')'  Statement
	//	IF  '(' Condition ')'  Statement  ELSE  Statement
	//
	//	SWITCH  '(' Condition ')'  Statement
	//
	int rlen = rule.Length();
	assert(rlen == 5 || rlen == 7);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2), &s4 = NextRuleSym(s3), &s5 = NextRuleSym(s4);

	if (s1.m_app_id == sym_if)
	{
		// Ensure right meaning of the third and fifth symbols.
		assert(s3.m_meaning == csm_statement && s3.m_statement != NULL && s5.m_meaning == csm_statement && s5.m_statement != NULL);

		// Allocate new database object for the stmt.
		TCppIfStatement *cond_stmt = (TCppIfStatement*)AllocateNewCppItem(cxx_if_stmt);
		if (cond_stmt == NULL)
			return;

		// Take care of the optional ELSE part.
		TCppStatementBase *else_part = NULL;
		if (rlen == 5)
		{
			// Empty ELSE is needed.
			else_part = (TCppStatementBase*)AllocateNewCppItem(cxx_empty_stmt);
			if (else_part == NULL)
			{
				delete cond_stmt;
				return;
			}
		}
		else
		{
			// Check the seventh symbol and pick up its meaning.
			TCppSymbolStateItem &s6 = NextRuleSym(s5), &s7 = NextRuleSym(s6);
			assert(s7.m_meaning == csm_statement && s7.m_statement != NULL);
			else_part = s7.m_statement;
			s7.ResetMeaning();
		}

		// Assemble the data structure.
		cond_stmt->AddChildItem(s3.m_statement, cstmt_chdest_if_cond);
		cond_stmt->AddChildItem(s5.m_statement, cstmt_chdest_if_then);
		cond_stmt->AddChildItem(else_part, cstmt_chdest_if_else);
		s3.ResetMeaning();
		s5.ResetMeaning();

		// Setup the non term.
		new_non_term.m_meaning = csm_statement;
		new_non_term.m_statement = cond_stmt;
	}
	else
	{
		// Ensure app id of the first symbol and right meaning of the third and fifth symbols.
		assert(s1.m_app_id == sym_switch);
		assert(s3.m_meaning == csm_statement && s3.m_statement != NULL && s5.m_meaning == csm_statement && s5.m_statement != NULL);

		// Allocate new database object for the stmt.
		TCppSwitchStatement *switch_stmt = (TCppSwitchStatement*)AllocateNewCppItem(cxx_switch_stmt);
		if (switch_stmt == NULL)
			return;

		// Assemble the data structure.
		switch_stmt->AddChildItem(s3.m_statement, cstmt_chdest_sw_cond);
		switch_stmt->AddChildItem(s5.m_statement, cstmt_chdest_sw_stmt);
		s3.ResetMeaning();
		s5.ResetMeaning();

		// Setup the non term.
		new_non_term.m_meaning = csm_statement;
		new_non_term.m_statement = switch_stmt;
	}

	// Check for implicit block around the statement.
	if (s1.m_meaning == csm_block_header)
	{
		// The block is existing. Add prepared statement to the block and replace the non term meaning.
		s1.m_block_info.m_block_header->AddChildItem(new_non_term.m_statement);
		new_non_term.m_statement = s1.m_block_info.m_block_header;

		// Exit this block.
		ExitCurrentDefnsLayer(&s1);
		s1.ResetMeaning();
	}
}

void TCppParser::ProcessIterationStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	FOR  '('  ForInitStatement			';'			   ')'	  Statement
	//	FOR  '('  ForInitStatement			';'   Expression   ')'	  Statement
	//	FOR  '('  ForInitStatement   Condition	';'			   ')'	  Statement
	//	FOR  '('  ForInitStatement   Condition	';'   Expression   ')'	  Statement
	//
	//	WHILE '(' Condition ')' Statement
	//	DoStatementBody WHILE '(' Expression ')'	';'
	//
	int rlen = rule.Length();
	assert(rlen >= 5 && rlen <= 8);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2), &s4 = NextRuleSym(s3), &s5 = NextRuleSym(s4);
	assert(s1.m_app_id == sym_for || s1.m_app_id == sym_while || s1.m_app_id == sym_DoStatementBody);

	TCppStatementBase *new_stmt_base = NULL;
	if (s1.m_app_id == sym_for)
	{
		// Create the loop object.
		assert(s3.m_meaning == csm_statement && s3.m_statement != NULL);
		TCppForStatement *for_stmt = (TCppForStatement*)AllocateNewCppItem(cxx_for_stmt);
		if (for_stmt == NULL)
			return;

		TCppStatementBase *check_cont_stmt = NULL;
		TCppSymbolStateItem *reinint_loop_expr_symbol = NULL;
		if (s4.m_app_id == sym_Condition)
		{
			// Rule contains the loop continuation condition.
			assert(s4.m_meaning == csm_statement && s4.m_statement != NULL);
			check_cont_stmt = s4.m_statement;
			s4.ResetMeaning();

			// Setup pointer that should point to the reinit expression symbol if it is available.
			reinint_loop_expr_symbol = NextRuleSymPtr(&s5);
			assert(reinint_loop_expr_symbol != NULL);
		}
		else
		{
			// In this case the forth symbol should be the semicolon.
			assert(s4.m_app_id == sym_semicol);

			// Create empty continuation condition.
			check_cont_stmt = (TCppStatementBase*)AllocateNewCppItem(cxx_empty_stmt);
			if (check_cont_stmt == NULL)
			{
				delete for_stmt;
				return;
			}

			// Setup pointer that should point to the reinit expression symbol if it is available.
			reinint_loop_expr_symbol = &s5;
		}

		TCppStatementBase *reinint_loop_expr = NULL;
		TCppSymbolStateItem *loop_body_symbol = NULL;
		if (reinint_loop_expr_symbol->m_meaning == csm_expression)
		{
			// Rule contains the loop reinit expression.
			assert(reinint_loop_expr_symbol->m_expression != NULL);
			reinint_loop_expr = reinint_loop_expr_symbol->m_expression;
			reinint_loop_expr_symbol->ResetMeaning();

			// Setup pointer that should point to the loop body symbol.
			loop_body_symbol = NextRuleSymPtr(NextRuleSymPtr(reinint_loop_expr_symbol));
			assert(loop_body_symbol != NULL);
		}
		else
		{
			// In this case there should be right parenthesis instead of expression.
			assert(reinint_loop_expr_symbol->m_app_id == sym_rpar);

			// Create empty reinit expression.
			reinint_loop_expr = (TCppStatementBase*)AllocateNewCppItem(cxx_empty_stmt);
			if (reinint_loop_expr == NULL)
			{
				delete check_cont_stmt;
				delete for_stmt;
				return;
			}

			// Setup pointer that should point to the loop body symbol.
			loop_body_symbol = NextRuleSymPtr(reinint_loop_expr_symbol);
			assert(loop_body_symbol != NULL);
		}

		// Ensure that body of the FOR loop is available.
		assert(loop_body_symbol->m_meaning == csm_statement && loop_body_symbol->m_statement != NULL);

		// Assemble the parts of the statement.
		for_stmt->AddChildItem(s3.m_statement, cstmt_chdest_for_expr1);
		for_stmt->AddChildItem(check_cont_stmt, cstmt_chdest_for_expr2);
		for_stmt->AddChildItem(reinint_loop_expr, cstmt_chdest_for_expr3);
		for_stmt->AddChildItem(loop_body_symbol->m_statement, cstmt_chdest_for_body);
		s3.ResetMeaning();
		loop_body_symbol->ResetMeaning();

		// Give the stmt out.
		new_stmt_base = for_stmt;
	}
	else if (s1.m_app_id == sym_while)
	{
		// The WHILE loop contains two meaningful parts.
		assert(s3.m_meaning == csm_statement && s3.m_statement != NULL && s5.m_meaning == csm_statement && s5.m_statement != NULL);

		// Create the loop object.
		TCppWhileStatement *while_stmt = (TCppWhileStatement*)AllocateNewCppItem(cxx_while_stmt);
		if (while_stmt == NULL)
			return;

		// Allocation succeeded. Fill in the props.
		while_stmt->AddChildItem(s3.m_statement, cstmt_chdest_wh_cond);
		while_stmt->AddChildItem(s5.m_statement, cstmt_chdest_wh_stmt);
		s3.ResetMeaning();
		s5.ResetMeaning();

		// Give the stmt out.
		new_stmt_base = while_stmt;
	}
	else if (s1.m_app_id == sym_DoStatementBody)
	{
		// The DO-WHILE loop contains the body and check-continue expression.
		assert(s1.m_meaning == csm_do_statement_body && s1.m_do_stmt_info.m_do_stmt_loop_body != NULL && s4.m_meaning == csm_expression && s4.m_expression != NULL);

		// Create the loop object.
		TCppDoWhileStatement *do_while_stmt = (TCppDoWhileStatement*)AllocateNewCppItem(cxx_do_while_stmt);
		if (do_while_stmt == NULL)
			return;

		// Allocation succeeded. Fill in the props.
		do_while_stmt->AddChildItem(s1.m_do_stmt_info.m_do_stmt_loop_body, cstmt_chdest_dw_stmt);
		do_while_stmt->AddChildItem(s4.m_statement, cstmt_chdest_dw_cond);
		s4.ResetMeaning();

		if (s1.m_do_stmt_info.m_do_stmt_block_header != NULL)
		{
			// Implicit block is present. Insert statement into the block and give it out.
			s1.m_do_stmt_info.m_do_stmt_block_header->AddChildItem(do_while_stmt);
			new_stmt_base = s1.m_do_stmt_info.m_do_stmt_block_header;

			// Exit this implicit block.
			ExitCurrentDefnsLayer(&s1);
		}
		else
		{
			// Give the stmt out.
			new_stmt_base = do_while_stmt;
		}

		s1.ResetMeaning();
	}

	// Setup the non terminal.
	assert(new_stmt_base != NULL);
	new_non_term.m_meaning = csm_statement;
	new_non_term.m_statement = new_stmt_base;

	// Check for the implicit block around this statement.
	if (s1.m_meaning == csm_block_header)
	{
		// -BUGBUG- Remove the pseudo child link.
		s1.m_block_info.m_block_header->m_cib_parent = NULL;

		// Block is existing. Add prepared statement to the block and replace the non term meaning.
		s1.m_block_info.m_block_header->AddChildItem(new_non_term.m_statement);
		new_non_term.m_statement = s1.m_block_info.m_block_header;

		// Exit this block.
		ExitCurrentDefnsLayer(&s1);
		s1.ResetMeaning();
	}
}

void TCppParser::ProcessForInitStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	SimpleDeclaration
	//	ExpressionStatement
	//
	// Note that SimpleDeclaration allows to define several variables that belong to the same type. Tricky processing
	// that may replace the IF/SWITCH/FOR/WHILE/DO-WHILE terminal symbols and DoStatementBody non terminal
	// with the block header take place when the definition is being published. Processing for this non term is relatively
	// simple.
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(s1.m_meaning == csm_declaration || s1.m_meaning == csm_declarations_list || s1.m_meaning == csm_expression);

	// New non terminal will be always a statement.
	new_non_term.m_meaning = csm_statement;
	if (s1.m_meaning == csm_declaration)
	{
		assert(s1.m_declaration != NULL);
		new_non_term.m_statement = s1.m_declaration;
	}
	if (s1.m_meaning == csm_declarations_list)
	{
		assert(s1.m_declarations_list != NULL);
		new_non_term.m_statement = s1.m_declarations_list;
	}
	else
	{
		assert(s1.m_expression != NULL);
		new_non_term.m_statement = s1.m_expression;
	}

	s1.ResetMeaning();
}

void TCppParser::ProcessCondition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	Expression
	//	UninitedCondition  '='  AssignmentExpression
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Simply promote expression meaning into the statement meaning.
		assert(s1.m_meaning == csm_expression && s1.m_expression != NULL);
		new_non_term.m_meaning = csm_statement;
		new_non_term.m_statement = s1.m_expression;
		s1.ResetMeaning();
	}
	else
	{
		// This form or the rule contains an initializer.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
		assert(s1.m_meaning == csm_declaration && s3.m_meaning == csm_expression && s3.m_expression != NULL);
		TCppDataFieldDefn *dfd = (TCppDataFieldDefn*)s1.m_declaration;
		dfd->AddChildItem(s3.m_expression, cdefn_chdest_dtfld_init);
		s3.ResetMeaning();

		// Prepare the non term.
		new_non_term.m_meaning = csm_statement;
		new_non_term.m_statement = s1.m_declaration;
		s1.ResetMeaning();
	}
}

void TCppParser::ProcessUninitedCondition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	TypeSpecifier  Declarator
	//
	// This non terminal was introduced to publish the variable in the scope of the current block
	// before assembling the initializer expression.
	//
	int rlen = rule.Length();
	assert(rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);
	assert(s1.m_meaning == csm_decl_type_spec && s2.m_meaning == csm_declarator);

	// Finalize building the type specifier. Note that since grammar contains the type specifier here,
	// this means that function attrs, strorage classes, etc are not possible here.
	if (CheckPrepareTypeSpecifier(s1) == FALSE)
		return;

	if (s2.m_declarator_info.m_name_info.IsQualified() == TRUE)
	{
		// Condition allows only local definitions.
		ReportSyntaxError(&s2, cerr_condition_bad_qual_path);
		s2.m_declarator_info.m_name_info.DowngradeToUnqualifiedNameInfo(this);
	}

	if (s2.m_declarator_info.m_name_info.IsIdentifier() == FALSE)
	{
		// Condition should be a simple name. Convert it into identifier.
		ReportSyntaxError(&s2, cerr_condition_not_simple_name);
		if (s2.m_declarator_info.m_name_info.DowngradeExpressionIdToIdentifier(this, L"Condition") == FALSE)
			return;
	}

	// Check situation for this name on the current layer.
	TCppNameInfo name_info;
	if (CheckLookupSimpleIdentifierSymbol(s2, name_info, cxx_data_field) == FALSE)
		return;

	// Assemble the type of the variable using the type spec and declarator modifiers.
	TCppDeclTypeSpecInfo &dts = s1.m_decl_type_spec_info;
	TCppDataTypeBase *data_type = (TCppDataTypeBase*)AssembleDeclaratorType(&s1, FALSE, dts.m_dclt_cv_spec, dts.m_dclt_base_type, s2);
	assert(data_type == NULL || data_type->IsDataType() == TRUE);
	if (data_type == NULL)
		return;

	// Check the constructed data type to be sure that it is not an array.
	TCppDataTypeBase *data_type_to_check = data_type;
	for(;;)
	{
		if (data_type_to_check->ItemType() == cxx_data_type_alias)
		{
			// Strip this alias layer.
			data_type_to_check = ((TCppDataTypeAliasDefn*)data_type_to_check)->m_base_type;
		}
		else if (data_type_to_check->ItemType() == cxx_array_type)
		{
			// The type is an array. Report this as an error.
			ReportSyntaxError(&s2, cerr_condition_defines_array);

			// Change the type of the variable to the type of the item of this array. This type will be
			// a wrong type, but at least this type will not be an array.
			data_type = ((TCppArrayTypeDefn*)data_type_to_check)->m_base_type;
			data_type_to_check = data_type;
		}
		else
		{
			break;
		}
	}

	// Create the data field object.
	TCppDataFieldDefn *new_var_object = (TCppDataFieldDefn*)AllocateNewCppItem(cxx_data_field, name_info.m_preferred_object_id);
	if (new_var_object == NULL)
		return;

	// Fill in the props.
	name_info.SetNameToObject(new_var_object);
	new_var_object->m_strg_class = strgc_auto;
	new_var_object->m_field_type = data_type;

	// Publish the definition. This may create implicit block object.
	if (PublishBlockDeclaration(new_var_object, FALSE) == FALSE)
	{
		// Critical error has happened.
		delete new_var_object;
		return;
	}

	// Prepare the non term.
	new_non_term.m_meaning = csm_declaration;
	new_non_term.m_declaration = new_var_object;
}

void TCppParser::ProcessDoStatementBody(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	DO  Statement
	//
	// This non terminal is needed for correct processing of the WHILE keyword. It is necessary to distinguish
	// situations where WHILE is part of the DO-WHILE loop and when it describes its own WHILE loop.
	//
	int rlen = rule.Length();
	assert(rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);
	assert(s2.m_meaning == csm_statement && s2.m_statement != NULL);

	if (s1.m_meaning == csm_none)
	{
		// Prepare non term for a more simple case. Implicit block was not entered while processing the body
		// of the DO-WHILE loop. Prepare simple do body meaning without a block.
		new_non_term.m_meaning = csm_do_statement_body;
		new_non_term.m_do_stmt_info.m_do_stmt_block_header = NULL;
		new_non_term.m_do_stmt_info.m_do_stmt_loop_body = s2.m_statement;
		new_non_term.m_parent_layer_owner = NULL;
		new_non_term.m_parent_layer_symbol = NULL;
	}
	else
	{
		// The DO symbol contains header of the implicit block. Exit this implicit block and enter it again.
		assert(s1.m_meaning == csm_block_header);
		TCppBlockDefn *implicit_block = s1.m_block_info.m_block_header;
		ExitCurrentDefnsLayer(&s1);
		s1.ResetMeaning();

		// Prepare complex meaning with block header and enter it.
		new_non_term.m_meaning = csm_do_statement_body;
		new_non_term.m_do_stmt_info.m_do_stmt_block_header = implicit_block;
		new_non_term.m_do_stmt_info.m_do_stmt_loop_body = s2.m_statement;
		EnterDefinitionsLayer(implicit_block, new_non_term);
	}

	s2.ResetMeaning();
}

void TCppParser::ProcessJumpStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	GOTO identifier ';'
	//	BREAK ';'
	//	CONTINUE ';'
	//	RETURN ';'
	//	RETURN Expression ';'
	//
	int rlen = rule.Length();
	assert(rlen == 2 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);
	assert(s1.m_app_id == sym_goto || s1.m_app_id == sym_break || s1.m_app_id == sym_continue || s1.m_app_id == sym_return);

	TCppStatementBase *new_stmt_base = NULL;
	if (s1.m_app_id == sym_goto)
	{
		// Pick up the key_id for the name in the second symbol.
		TCppNameInfo name_info;
		if (PrepareSimpleNameInfo(s2, name_info) == FALSE)
			return;

		// The worker function will handle all three possible cases: name of the label is unknown, label is known and
		// it is already defined somewhere the code, label is known but it is not yet defined in the code.
		TCppLabelDefn *label_object = FindOrCreateNamedLabel(name_info, TRUE);
		if (label_object == NULL)
			return;

		// Create the jump object. In the case of failure do not delete the label.
		TCppGotoStatement *goto_stmt = (TCppGotoStatement*)AllocateNewCppItem(cxx_goto_stmt);
		if (goto_stmt == NULL)
			return;

		// Finalize the statement and give it out. Note that label is not owned by the GOTO statement.
		goto_stmt->m_label = label_object;
		new_stmt_base = goto_stmt;
	}
	else if (s1.m_app_id == sym_break)
	{
		// Create the BREAK stmt object. This statement does not need any additional setup.
		new_stmt_base = (TCppBreakStatement*)AllocateNewCppItem(cxx_break_stmt);
		if (new_stmt_base == NULL)
			return;
	}
	else if (s1.m_app_id == sym_continue)
	{
		// Create the CONTINUE stmt object. This statement does not need any additional setup.
		new_stmt_base = (TCppContinueStatement*)AllocateNewCppItem(cxx_continue_stmt);
		if (new_stmt_base == NULL)
			return;
	}
	else if (s1.m_app_id == sym_return)
	{
		// Create the RETURN stmt object.
		TCppReturnStatement *return_stmt = (TCppReturnStatement*)AllocateNewCppItem(cxx_return_stmt);
		if (return_stmt == NULL)
			return;

		if (rlen == 3)
		{
			// The rule contains expression.
			assert(s2.m_meaning == csm_expression && s2.m_expression != NULL);
			return_stmt->AddChildItem(s2.m_statement, cstmt_chdest_ret_expr);
			s2.ResetMeaning();
		}

		// Give the statement out.
		new_stmt_base = return_stmt;
	}

	// Setup the non terminal.
	assert(new_stmt_base != NULL);
	new_non_term.m_meaning = csm_statement;
	new_non_term.m_statement = new_stmt_base;
}

void TCppParser::ProcessTryBlock(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	TRY  CompoundStatement  ExceptionHandlersSeq
	//
	int rlen = rule.Length();
	assert(rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);

	// Second symbol should contain the TRY block and the third symbol the seq of handlers.
	assert(s2.m_meaning == csm_statement && s2.m_statement != NULL && s2.m_statement->ItemType() == cxx_try_block);
	assert(s3.m_meaning == csm_catch_blocks_seq && s3.m_catch_blocks_seq != NULL);

	// Assemble these parts.
	s3.m_catch_blocks_seq->AddChildItem(s2.m_statement, cstmt_chdest_ltblk_prepend);
	s2.ResetMeaning();

	// Place result into the new non terminal.
	new_non_term.m_meaning = csm_statement;
	new_non_term.m_statement = s3.m_catch_blocks_seq;
	s3.ResetMeaning();
}

void TCppParser::ProcessExceptionHandlersSeq(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	ExceptionHandler
	//	ExceptionHandlersSeq  ExceptionHandler
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	TCppLightBlockDefn *light_block = NULL;
	if (rlen == 1)
	{
		// First symbol should be the catch block. Create the list right now. This makes sense even
		// if there will be only one handler in the sequence of exception handlers.
		assert(s1.m_meaning == csm_catch_block && s1.m_catch_block != NULL);
		light_block = (TCppLightBlockDefn*)AllocateNewCppItem(cxx_light_block);
		if (light_block == NULL)
			return;

		light_block->AddChildItem(s1.m_catch_block);
		s1.ResetMeaning();
	}
	else
	{
		// The list should be already existing. Add one more handler there.
		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		assert(s1.m_meaning == csm_catch_blocks_seq && s1.m_catch_blocks_seq != NULL);
		assert(s2.m_meaning == csm_catch_block && s2.m_catch_block != NULL);

		light_block = s1.m_catch_blocks_seq;
		light_block->AddChildItem(s2.m_catch_block);
		s1.ResetMeaning();
		s2.ResetMeaning();
	}

	// Put the list of handlers into the non terminal.
	new_non_term.m_meaning = csm_catch_blocks_seq;
	new_non_term.m_catch_blocks_seq = light_block;
}

void TCppParser::ProcessExceptionHandler(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	ExceptionHandlerHeader  CompoundStatement
	//
	int rlen = rule.Length();
	assert(rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);

	// Second symbol should be the CatchBlock. Ensure this. Note that exception param was placed into the catch block
	// when the beginning of the compound statement was processed. Exception params is now owned by this block.
	assert(s2.m_meaning == csm_statement && s2.m_statement != NULL && s2.m_statement->ItemType() == cxx_catch_block);

	// Promote the catch block up.
	new_non_term.m_meaning = csm_catch_block;
	new_non_term.m_catch_block = (TCppCatchBlockDefn*)s2.m_statement;
	s2.ResetMeaning();
}

void TCppParser::ProcessExceptionHandlerHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	CATCH  '(' TypeSpecifier ')'
	//	CATCH  '(' TypeSpecifier Declarator ')'
	//	CATCH  '(' TypeSpecifier AbstractDeclarator ')'
	//	CATCH  '(' '...' ')'
	//
	int rlen = rule.Length();
	assert(rlen == 4 || rlen == 5);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2), &s4 = NextRuleSym(s3);

	TCppDataFieldDefn *new_param = NULL;
	if (s3.m_meaning == csm_decl_type_spec)
	{
		// Use utility function to create the data field object for exception that is being handled.
		new_param = CreateDataParamObject(s3, (rlen == 4) ? FALSE : TRUE);
		if (new_param == NULL)
			return;
	}
	else
	{
		// This is the "catch all" rule.
		assert(s3.m_app_id == sym_ellipsis);

		// Create pseudo type for this param.
		TCppBuiltInTypeDefn *prm_type = GetBuiltInTypeDefn(bltin_id_ellips);
		if (prm_type == NULL)
			return;

		// Create the data field object for this param directly.
		new_param = (TCppDataFieldDefn*)AllocateNewCppItem(cxx_data_field);
		if (new_param == NULL)
			return;

		// Fill in the props. Param index will be set up later.
		new_param->SetNameInfo(L"...", 0, cdpr_none);
		new_param->m_strg_class = strgc_param;
		new_param->m_field_type = prm_type;
	}

	// Put this data field into the meaning of the new non term.
	new_non_term.m_meaning = csm_exception_param;
	new_non_term.m_exception_param = new_param;
}

// ----------------------------------------------------------------------------------------------
// ===================  Part 11. Expressions  ======================================
// ----------------------------------------------------------------------------------------------

enum P11_Expressions { P11_member };				// This enum is used for src code navigation purposes only.

void TCppParser::ProcessBinaryExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	LowerLevelExpression
	//	ThisLevelExpression  <operator>  LowerLevelExpression
	//
	// Operator can be here (11 cases): "==", "!=", "<", ">", "<=", ">=", "<<", ">>", "-", "/", "%".
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Symbol of the rule should be expression.
		assert(s1.m_meaning == csm_expression && s1.m_expression != NULL);

		// Promote expression of the rule symbol up.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = s1.m_expression;
		s1.ResetMeaning();
	}
	else
	{
		// Symbols of the rule should be: expression, terminal_symbol, expression.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
		assert(s1.m_meaning == csm_expression && s2.m_symbol < NonTerminalsBase && s3.m_meaning == csm_expression);

		// Pick up the operation code.
		TOperatorToken operation = opr_none;
		switch (s2.m_app_id)
		{
			case sym_eq:		operation = opr_eq;
								break;
			case sym_ne:		operation = opr_ne;
								break;
			case sym_lt:			operation = opr_lt;
								break;
			case sym_gt:			operation = opr_gt;
								break;
			case sym_le:			operation = opr_le;
								break;
			case sym_ge:		operation = opr_ge;
								break;
			case sym_bitlsh:		operation = opr_bitlsh;
								break;
			case sym_bitrsh:		operation = opr_bitrsh;
								break;
			case sym_minus:		operation = opr_minus;
								break;
			case sym_div:		operation = opr_div;
								break;
			case sym_rmnd:		operation = opr_rmnd;
								break;
			default:
				// Unexpected operation. Report critical error and exit.
				RaiseUnexpectedRuleSymbolAppIdError(irule, &s2);
				return;
		}

		// The operation is available. Create new expression object.
		TCppBinaryExpr *new_expr_node = (TCppBinaryExpr*)AllocateNewCppItem(cxx_binary_expr);
		if (new_expr_node == NULL)
			return;

		// New expr does not have L-value property by default. Result is const if both operands are const.
		new_expr_node->m_const_expr = s1.m_expression->m_const_expr && s3.m_expression->m_const_expr;

		// Fill in the rest of the props of the expression.
		new_expr_node->SetOperation(operation);
		new_expr_node->AddChildItem(s1.m_expression, cexpr_chdest_binary_oprnd1);
		new_expr_node->AddChildItem(s3.m_expression, cexpr_chdest_binary_oprnd2);

		// Set prepared expression into the non terminal of the rule.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = new_expr_node;

		// Data from both expression symbols is already taken.
		s1.ResetMeaning();
		s3.ResetMeaning();
	}
}

void TCppParser::ProcessMultiOpExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	LowerLevelExpression
	//	ThisLevelExpression  <operator>  LowerLevelExpression
	//
	// Operator can be here (8 cases): comma, "&&", "||", "&", "|", "^", "+", "*".
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Symbol of the rule should be expression.
		assert(s1.m_meaning == csm_expression && s1.m_expression != NULL);

		// Promote expression of the rule symbol up.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = s1.m_expression;
		s1.ResetMeaning();
	}
	else
	{
		// Symbols of the rule should be: expression, terminal_symbol, expression.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
		assert(s1.m_meaning == csm_expression && s2.m_symbol < NonTerminalsBase && s3.m_meaning == csm_expression);

		// Pick up the operation code.
		TOperatorToken operation = opr_none;
		switch (s2.m_app_id)
		{
			case sym_comma:	operation = opr_comma;
								break;
			case sym_logand:		operation = opr_logand;
								break;
			case sym_logor:		operation = opr_logor;
								break;
			case sym_bitand:		operation = opr_bitand;
								break;
			case sym_bitor:		operation = opr_bitor;
								break;
			case sym_bitxor:		operation = opr_bitxor;
								break;
			case sym_plus:		operation = opr_plus;
								break;
			case sym_mul:		operation = opr_mul;
								break;
			default:
				// Unexpected operation. Report critical error and exit.
				RaiseUnexpectedRuleSymbolAppIdError(irule, &s2);
				return;
		}

		// Check for possibilities of merging participating expressions one into another.
		if (s1.m_expression->m_operation == operation && s3.m_expression->m_operation == operation)
		{
			// Use expression of the first operand as a base.
			assert(s1.m_expression->ItemType() == cxx_multiop_expr && s3.m_expression->ItemType() == cxx_multiop_expr);
			new_non_term.m_meaning = csm_expression;
			new_non_term.m_expression = s1.m_expression;

			TCppMultiOpExpr &s3_multiop_expr = (TCppMultiOpExpr&)(*s3.m_expression);
			if (s3_multiop_expr.m_operands.IsEmpty() == FALSE)
			{
				// Operands in the second expr are available. This is typical case.
				if (operation == opr_comma)
				{
					TCppExpressionBase *last_operand = (TCppExpressionBase*)s3_multiop_expr.m_operands.GetLast();
					new_non_term.m_expression->m_lvalue_expr = last_operand->m_lvalue_expr;
					new_non_term.m_expression->m_const_expr = last_operand->m_const_expr;
				}
				else
				{
					new_non_term.m_expression->m_lvalue_expr = FALSE;
					new_non_term.m_expression->m_const_expr &= s3_multiop_expr.m_const_expr;
				}

				// Add operands from the second expr to the list of operands of the first expression.
				TCppMultiOpExpr &s1_multiop_expr = (TCppMultiOpExpr&)(*(new_non_term.m_expression));
				s1_multiop_expr.AddListOfChildren(s3_multiop_expr.m_operands);
			}

			// Get rid of expression of the second operand that is empty now.
			delete s3.m_expression;
		}
		else if (s1.m_expression->m_operation == operation)
		{
			// Add second symbol as operand to the expression of the first symbol.
			new_non_term.m_meaning = csm_expression;
			new_non_term.m_expression = s1.m_expression;

			if (operation == opr_comma)
			{
				new_non_term.m_expression->m_lvalue_expr = s3.m_expression->m_lvalue_expr;
				new_non_term.m_expression->m_const_expr = s3.m_expression->m_const_expr;
			}
			else
			{
				new_non_term.m_expression->m_lvalue_expr = FALSE;
				new_non_term.m_expression->m_const_expr &= s3.m_expression->m_const_expr;
			}

			// Add operand to the end of the list.
			new_non_term.m_expression->AddChildItem(s3.m_expression);
		}
		else if (s3.m_expression->m_operation == operation)
		{
			// Add first symbol as operand to the expression of the second symbol.
			new_non_term.m_meaning = csm_expression;
			new_non_term.m_expression = s3.m_expression;

			if (operation != opr_comma)
			{
				new_non_term.m_expression->m_lvalue_expr = FALSE;
				new_non_term.m_expression->m_const_expr &= s1.m_expression->m_const_expr;
			}

			// Add operand to the beginning of the list.
			new_non_term.m_expression->AddChildItem(s1.m_expression, cexpr_chdest_multiop_prepend);
		}
		else
		{
			// It is necessary to create new expression node for this operation.
			TCppMultiOpExpr *new_expr_node = (TCppMultiOpExpr*)AllocateNewCppItem(cxx_multiop_expr);
			if (new_expr_node == NULL)
				return;

			if (operation == opr_comma)
			{
				// L-value and const properties should be taken from the second operand.
				new_expr_node->m_lvalue_expr = s3.m_expression->m_lvalue_expr;
				new_expr_node->m_const_expr = s3.m_expression->m_const_expr;
			}
			else
			{
				// New expr does not have L-value property by default. Result is const if both operands are const.
				new_expr_node->m_const_expr = s1.m_expression->m_const_expr && s3.m_expression->m_const_expr;
			}

			// Fill in the rest of the props of the expression.
			new_expr_node->SetOperation(operation);
			new_expr_node->AddChildItem(s1.m_expression);
			new_expr_node->AddChildItem(s3.m_expression);

			// Set prepared expression into the non terminal of the rule.
			new_non_term.m_meaning = csm_expression;
			new_non_term.m_expression = new_expr_node;
		}

		// Data from both expression symbols is already taken.
		s1.ResetMeaning();
		s3.ResetMeaning();
	}
}

void TCppParser::ProcessAddMulExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	LowerLevelExpression
	//	ThisLevelExpression  <operator>  LowerLevelExpression
	//
	// Operator can be here (5 cases): "+", "-", "*", "/", "%".
	//
	// This method was introduced for simplification of the grammar code. Non terminals AdditiveExpression
	// and MultiplicativeExpression each have operations that should be implemented using different expression
	// objects. This method is doing exaactly this switch.
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Symbol of the rule should be expression.
		assert(s1.m_meaning == csm_expression && s1.m_expression != NULL);

		// Promote expression of the rule symbol up.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = s1.m_expression;
		s1.ResetMeaning();
	}
	else
	{
		// Second symbol of the rule should be a terminal_symbol.
		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		assert(s2.m_symbol < NonTerminalsBase);

		// Check the operation code.
		switch (s2.m_app_id)
		{
			case sym_plus:
			case sym_mul:		ProcessMultiOpExpression(new_non_term, irule, rule);
								break;
			case sym_minus:
			case sym_div:
			case sym_rmnd:		ProcessBinaryExpression(new_non_term, irule, rule);
								break;
			default:
				// Unexpected operation. Report critical error and exit.
				RaiseUnexpectedRuleSymbolAppIdError(irule, &s2);
				return;
		}
	}
}

void TCppParser::ProcessAssignmentExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	ConditionalExpression
	//	ThrowExpression
	//
	//	LogicalOrExpression  AssignmentOperator  AssignmentExpression
	//
	// In this case ConditionalExpression and ThrowExpression are allowed forms of lower level expressions.
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Symbol of the rule should be expression.
		assert(s1.m_meaning == csm_expression && s1.m_expression != NULL);

		// Promote expression of the rule symbol up.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = s1.m_expression;
		s1.ResetMeaning();
	}
	else
	{
		// Symbols of the rule should be: expression, AssignmentOperator, expression.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
		assert(s1.m_meaning == csm_expression && s2.m_meaning == csm_assignment_operator && s3.m_meaning == csm_expression);

		if (s1.m_expression->m_lvalue_expr == FALSE)
		{
			// Report the error, but continue building the expression.
			ReportSyntaxError(&s1, cerr_expr_lvalue_missing);
		}

		// Create the expression object.
		TCppAssignmentExpr *new_expr_node = (TCppAssignmentExpr*)AllocateNewCppItem(cxx_assignment_expr);
		if (new_expr_node == NULL)
			return;

		// New expr does not have L-value and const properties by default. Fill in the rest of the props.
		new_expr_node->SetOperation(s2.m_assgn_operator);
		new_expr_node->AddChildItem(s1.m_expression, cexpr_chdest_assgn_oprnd1);
		new_expr_node->AddChildItem(s3.m_expression, cexpr_chdest_assgn_oprnd2);

		// Set prepared expression into the non terminal of the rule.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = new_expr_node;

		// Data from both expression symbols is already taken.
		s1.ResetMeaning();
		s3.ResetMeaning();
	}
}

void TCppParser::ProcessAssignmentOperator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms (11 cases):
	//
	//	'=' | '+=' | '-=' | '*=' | '/=' | '%=' | '<<=' | '>>=' | '&=' | '|=' | '^='
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	switch (s1.m_app_id)
	{
		case sym_assign:			new_non_term.m_meaning = csm_assignment_operator;
								new_non_term.m_assgn_operator = opr_assign;
								break;
		case sym_plus_asgn:		new_non_term.m_meaning = csm_assignment_operator;
								new_non_term.m_assgn_operator = opr_plus_asgn;
								break;
		case sym_minus_asgn:		new_non_term.m_meaning = csm_assignment_operator;
								new_non_term.m_assgn_operator = opr_minus_asgn;
								break;
		case sym_mul_asgn:		new_non_term.m_meaning = csm_assignment_operator;
								new_non_term.m_assgn_operator = opr_mul_asgn;
								break;
		case sym_div_asgn:		new_non_term.m_meaning = csm_assignment_operator;
								new_non_term.m_assgn_operator = opr_div_asgn;
								break;
		case sym_rmnd_asgn:		new_non_term.m_meaning = csm_assignment_operator;
								new_non_term.m_assgn_operator = opr_rmnd_asgn;
								break;
		case sym_lsh_asgn:		new_non_term.m_meaning = csm_assignment_operator;
								new_non_term.m_assgn_operator = opr_lsh_asgn;
								break;
		case sym_rsh_asgn:		new_non_term.m_meaning = csm_assignment_operator;
								new_non_term.m_assgn_operator = opr_rsh_asgn;
								break;
		case sym_btand_asgn:		new_non_term.m_meaning = csm_assignment_operator;
								new_non_term.m_assgn_operator = opr_btand_asgn;
								break;
		case sym_btor_asgn:		new_non_term.m_meaning = csm_assignment_operator;
								new_non_term.m_assgn_operator = opr_btor_asgn;
								break;
		case sym_btxor_asgn:		new_non_term.m_meaning = csm_assignment_operator;
								new_non_term.m_assgn_operator = opr_btxor_asgn;
								break;
		default:
			RaiseUnexpectedRuleSymbolAppIdError(irule, &s1);
			break;
	}
}

void TCppParser::ProcessThrowExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	THROW
	//	THROW  AssignmentExpression
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);

	// New expression object is needed in any case.
	TCppThrowExpr *new_expr_node = (TCppThrowExpr*)AllocateNewCppItem(cxx_throw_expr);
	if (new_expr_node == NULL)
		return;

	if (rlen == 2)
	{
		// Operand expression is present.
		TCppSymbolStateItem &s2 = NextRuleSym(FirstRuleSym(rlen));
		assert(s2.m_meaning == csm_expression);
		new_expr_node->AddChildItem(s2.m_expression, cexpr_chdest_throw_oprnd);
		new_expr_node->m_result_type = s2.m_expression->m_result_type;
		s2.ResetMeaning();
	}

	// Set expression into the non terminal of the rule.
	new_non_term.m_meaning = csm_expression;
	new_non_term.m_expression = new_expr_node;
}

void TCppParser::ProcessConstantExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	ConditionalExpression
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);
	assert(s1.m_meaning == csm_expression);

	/* -BUGBUG- Comment this out for now.
	if (s1.m_expression->m_const_expr == FALSE)
	{
		// Issue an error and continue building the expression.
		ReportSyntaxError(&s1, cerr_expr_const_missing);
	}
	*/

	// Promote the existing expression up.
	new_non_term.m_meaning = csm_expression;
	new_non_term.m_expression = s1.m_expression;
	s1.ResetMeaning();
}

void TCppParser::ProcessConditionalExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	LogicalOrExpression
	//	LogicalOrExpression  '?'  Expression  ':'  AssignmentExpression
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 5);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Symbol of the rule should be expression.
		assert(s1.m_meaning == csm_expression && s1.m_expression != NULL);

		// Promote expression of the rule symbol up.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = s1.m_expression;
		s1.ResetMeaning();
	}
	else
	{
		// Symbols of the rule should be: expression, terminal, expression, terminal, expression.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2), &s4 = NextRuleSym(s3), &s5 = NextRuleSym(s4);
		assert(s1.m_meaning == csm_expression && s3.m_meaning == csm_expression && s5.m_meaning == csm_expression);

		// New expression object is needed.
		TCppConditionalExpr *new_expr_node = (TCppConditionalExpr*)AllocateNewCppItem(cxx_conditional_expr);
		if (new_expr_node == NULL)
			return;

		// New expr does not have L-value property by default, the result is const if all operands are const.
		new_expr_node->m_const_expr = s1.m_expression->m_const_expr && s3.m_expression->m_const_expr && s5.m_expression->m_const_expr;

		// Fill in the operands.
		new_expr_node->AddChildItem(s1.m_expression, cexpr_chdest_cond_oprnd1);
		new_expr_node->AddChildItem(s3.m_expression, cexpr_chdest_cond_oprnd2);
		new_expr_node->AddChildItem(s5.m_expression, cexpr_chdest_cond_oprnd3);

		// Set expression into the non terminal of the rule.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = new_expr_node;

		// Data from both expression symbols is already taken.
		s1.ResetMeaning();
		s3.ResetMeaning();
		s5.ResetMeaning();
	}
}

void TCppParser::ProcessPmExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	CastExpression
	//
	//	PmExpression     '.*'	CastExpression
	//	PmExpression    '->*'	CastExpression
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Symbol of the rule should be expression.
		assert(s1.m_meaning == csm_expression && s1.m_expression != NULL);

		// Promote expression of the rule symbol up.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = s1.m_expression;
		s1.ResetMeaning();
	}
	else
	{
		// Symbols of the rule should be: expression, terminal_symbol, expression.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);
		assert(s1.m_meaning == csm_expression && s2.m_symbol < NonTerminalsBase && s3.m_meaning == csm_expression);

		// Pick up the operation code.
		TOperatorToken operation = opr_none;
		switch (s2.m_app_id)
		{
			case sym_dotstar:		operation = opr_dotstar;
								break;
			case sym_arrowstar:	operation = opr_arrowstar;
								break;
			default:
				// Unexpected operation. Report critical error and exit.
				RaiseUnexpectedRuleSymbolAppIdError(irule, &s2);
				return;
		}

		// The operation is available. Create new expression object.
		TCppFieldAccessPtrExpr *new_expr_node = (TCppFieldAccessPtrExpr*)AllocateNewCppItem(cxx_field_access_ptr_expr);
		if (new_expr_node == NULL)
			return;

		// Result has L-value property and it is not const.
		new_expr_node->m_lvalue_expr = TRUE;

		// Fill in the rest of the props of the expression.
		new_expr_node->SetOperation(operation);
		new_expr_node->AddChildItem(s1.m_expression, cexpr_chdest_fp_acc_oprnd1);
		new_expr_node->AddChildItem(s3.m_expression, cexpr_chdest_fp_acc_oprnd2);

		// Set expression into the non terminal of the rule.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = new_expr_node;

		// Data from both expression symbols is already taken.
		s1.ResetMeaning();
		s3.ResetMeaning();
	}
}

void TCppParser::ProcessCastExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	UnaryExpression
	//	'(' TypeId ')'  CastExpression
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Symbol of the rule should be expression.
		assert(s1.m_meaning == csm_expression && s1.m_expression != NULL);

		// Promote expression of the rule symbol up.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = s1.m_expression;
		s1.ResetMeaning();
	}
	else
	{
		// Symbols of the rule should be: terminal, data type, terminal, expression.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2), &s4 = NextRuleSym(s3);
		assert(s2.m_meaning == csm_data_type && s4.m_meaning == csm_expression);

		// New expression object is needed.
		TCppCastExpr *new_expr_node = (TCppCastExpr*)AllocateNewCppItem(cxx_cast_expr);
		if (new_expr_node == NULL)
			return;

		// New expr does not have the L-value property by default, the result is const if the operand is const.
		new_expr_node->m_const_expr = s4.m_expression->m_const_expr;

		// Fill in the operands.
		new_expr_node->SetOperation(opr_cast);
		new_expr_node->m_dest_type = s2.m_data_type;
		new_expr_node->AddChildItem(s4.m_expression, cexpr_chdest_cast_oprnd);

		// Set expression into the non terminal of the rule.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = new_expr_node;

		// Data from the operand symbols is already taken.
		s2.ResetMeaning();
		s4.ResetMeaning();
	}
}

void TCppParser::ProcessUnaryExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	PostfixExpression | NewExpression | DeleteExpression
	//
	//	'++'	 CastExpression
	//	'--'	 CastExpression
	//
	//	UnaryOperator  CastExpression
	//
	//	SIZEOF  UnaryExpression
	//	SIZEOF  '(' TypeId ')'
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2 || rlen == 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Rule symbol should be expression.
		assert(s1.m_meaning == csm_expression);

		// Promote expression of the rule symbol up.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = s1.m_expression;
		s1.ResetMeaning();
	}
	else if (rlen == 2)
	{
		// Second symbol should be expression.
		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		assert(s2.m_meaning == csm_expression);

		// Retrieve operation code from the first symbol.
		TOperatorToken operation = opr_none;
		if (s1.m_meaning == csm_unary_operator)
		{
			// Unary operator was prepared by separate rule.
			operation = s1.m_unary_operator;
		}
		else
		{
			// Operator should be expressed with terminal symbol.
			switch (s1.m_app_id)
			{
				case sym_plusplus:		operation = opr_plusplus;
										break;
				case sym_minusminus:		operation = opr_minusminus;
										break;
				case sym_sizeof:			operation = opr_sizeof;
										break;
				default:
					// Unexpected operation. Report critical error and exit.
					RaiseUnexpectedRuleSymbolAppIdError(irule, &s1);
					return;
			}
		}

		TCppExpressionBase *new_expr = NULL;
		if (operation == opr_unplus)
		{
			// Discard this operation. Promote existing expression of the rule symbol up.
			new_expr = s2.m_expression;
		}
		else if (operation == opr_sizeof)
		{
			// This is picking up the size of some data operand.
			TCppSizeofExpr *new_expr_node = (TCppSizeofExpr*)AllocateNewCppItem(cxx_sizeof_expr);
			if (new_expr_node == NULL)
				return;

			// Fill in the props of the new node.
			new_expr_node->AddChildItem(s2.m_expression, cexpr_chdest_sizeof_oprnd);
			new_expr = new_expr_node;
		}
		else
		{
			// Generate regular unary operation.
			TCppUnaryExpr *new_expr_node = (TCppUnaryExpr*)AllocateNewCppItem(cxx_unary_expr);
			if (new_expr_node == NULL)
				return;

			// Fill in the props of the new node.
			new_expr_node->SetOperation(operation);
			new_expr_node->AddChildItem(s2.m_expression, cexpr_chdest_unary_oprnd);
			new_expr = new_expr_node;

			if (operation == opr_deref_ptr)
				new_expr_node->m_lvalue_expr = TRUE;
		}

		// Setup non terminal with prepared expression.
		assert(new_expr != NULL);
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = new_expr;
		s2.ResetMeaning();
	}
	else
	{
		// Symbols of the rule should be: terminal, terminal, data type, terminal.
		TCppSymbolStateItem &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2), &s4 = NextRuleSym(s3);
		assert(s3.m_meaning == csm_data_type);

		// New expression object is needed.
		TCppSizeofExpr *new_expr_node = (TCppSizeofExpr*)AllocateNewCppItem(cxx_sizeof_expr);
		if (new_expr_node == NULL)
			return;

		// Fill in the props and setup the rule non terminal.
		new_expr_node->SetDataTypeOperand(s3.m_data_type);
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = new_expr_node;
		s3.ResetMeaning();
	}
}

void TCppParser::ProcessUnaryOperator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms (6 cases):
	//
	//	'*' | '&' | '+' | '-' | '!' | '~'
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	switch (s1.m_app_id)
	{
		case sym_mul:		new_non_term.m_meaning = csm_unary_operator;
							new_non_term.m_unary_operator = opr_deref_ptr;
							break;
		case sym_bitand:		new_non_term.m_meaning = csm_unary_operator;
							new_non_term.m_unary_operator = opr_get_addr;
							break;
		case sym_plus:		new_non_term.m_meaning = csm_unary_operator;
							new_non_term.m_unary_operator = opr_unplus;
							break;
		case sym_minus:		new_non_term.m_meaning = csm_unary_operator;
							new_non_term.m_unary_operator = opr_unminus;
							break;
		case sym_bang:		new_non_term.m_meaning = csm_unary_operator;
							new_non_term.m_unary_operator = opr_bang;
							break;
		case sym_tilda:		new_non_term.m_meaning = csm_unary_operator;
							new_non_term.m_unary_operator = opr_tilda;
							break;
		default:
			RaiseUnexpectedRuleSymbolAppIdError(irule, &s1);
			break;
	}
}

void TCppParser::ProcessNewExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//		NEW					 NewTypeId
	//		NEW					 NewTypeId	NewInitializer
	//		NEW    NewPlacement	 NewTypeId
	//		NEW    NewPlacement	 NewTypeId	NewInitializer
	//	'::'	NEW					 NewTypeId
	//	'::'	NEW					 NewTypeId	NewInitializer
	//	'::'	NEW    NewPlacement	 NewTypeId
	//	'::'	NEW    NewPlacement	 NewTypeId	NewInitializer
	//
	//		NEW					'(' TypeId ')'
	//		NEW					'(' TypeId ')'	NewInitializer
	//		NEW    NewPlacement	'(' TypeId ')'
	//		NEW    NewPlacement	'(' TypeId ')'	NewInitializer
	//	'::'	NEW					'(' TypeId ')'
	//	'::'	NEW					'(' TypeId ')'	NewInitializer
	//	'::'	NEW    NewPlacement	'(' TypeId ')'
	//	'::'	NEW    NewPlacement	'(' TypeId ')'	NewInitializer
	//
	int rlen = rule.Length();
	assert(rlen >= 2 && rlen <= 7);
	TCppSymbolStateItem *sym = FirstRuleSymPtr(rlen);

	// New expression object is needed.
	TCppNewExpr *new_expr_node = (TCppNewExpr*)AllocateNewCppItem(cxx_new_expr);
	if (new_expr_node == NULL)
		return;

	if (sym->m_app_id == sym_dblcolon)
	{
		// Double colon is present in the rule.
		new_expr_node->m_global_new = TRUE;
		sym = NextRuleSymPtr(sym);
	}

	// Skip the NEW keyword.
	assert(sym->m_app_id == sym_new);
	sym = NextRuleSymPtr(sym);

	if (sym->m_app_id == sym_NewPlacement)
	{
		// Placement params are present.
		assert(sym->m_meaning == csm_expression && sym->m_expression != NULL);

		if (sym->m_expression->m_operation == opr_comma)
		{
			// Expression in the symbol is the list of expressions. Do not reset the meaning because
			// only the operands are moved. The expr itself will be released by recycle procedure.
			TCppMultiOpExpr &mop_expr = (TCppMultiOpExpr&)(*(sym->m_expression));
			new_expr_node->AddListOfChildren(mop_expr.m_operands, cexpr_chdest_new_plc_params);
		}
		else
		{
			// Symbol contains some other type of expression. Add it as one placement param.
			new_expr_node->AddChildItem(sym->m_expression, cexpr_chdest_new_plc_params);
			sym->ResetMeaning();
		}

		// Shift to the next symbol.
		sym = NextRuleSymPtr(sym);
	}

	// Pick up the data type as it was constructed by the grammar.
	if (sym->m_app_id == sym_NewTypeId)
	{
		// The type is stored right in the current symbol.
		assert(sym->m_meaning == csm_data_type && sym->m_data_type != NULL);
		new_expr_node->m_new_type = sym->m_data_type;

		// Skip the NewTypeId symbol.
		sym = NextRuleSymPtr(sym);
	}
	else
	{
		// Skip the first parenthesis.
		sym = NextRuleSymPtr(sym);

		// Pick up the type from the current symbol.
		assert(sym->m_meaning == csm_data_type && sym->m_data_type != NULL);
		new_expr_node->m_new_type = sym->m_data_type;

		// Skip the TypeId symbol and the second parenthesis.
		sym = NextRuleSymPtr(sym);
		sym = NextRuleSymPtr(sym);
	}

	// Expression should return data type with one more layer of indirection because the data type
	// that is specified in the expression is the type of the object in the heap.
	TCppDeclaratorModifier extra_ptr_spec;
	extra_ptr_spec.SetPointerSpec();
	TCppItemType ntt = new_expr_node->m_new_type->ItemType();
	bool named_base_type = (ntt != cxx_cv_ptr_ref_type && ntt != cxx_array_type) ? TRUE : FALSE;
	new_expr_node->m_result_type = CreateCvPtrRefModification(LastStackSymPtr(), new_expr_node->m_new_type, &extra_ptr_spec, 1, TRUE, named_base_type);
	if (new_expr_node->m_result_type == NULL)
	{
		delete new_expr_node;
		return;
	}

	if (sym != NULL)
	{
		// Initializer params are present.
		assert(sym->m_meaning == csm_expression && sym->m_expression != NULL);

		if (sym->m_expression->m_operation == opr_comma)
		{
			// Expression is the list of expressions, maybe empty. Do not reset the meaning because
			// only the operands are moved. The expr itself will be released by recycle procedure.
			TCppMultiOpExpr &mop_expr = (TCppMultiOpExpr&)(*(sym->m_expression));
			new_expr_node->AddListOfChildren(mop_expr.m_operands, cexpr_chdest_new_init_params);
		}
		else
		{
			// Symbol contains some other type of expression. Add it as one init param.
			new_expr_node->AddChildItem(sym->m_expression, cexpr_chdest_new_init_params);
			sym->ResetMeaning();
		}
	}

	// Set prepared expression into the non terminal of the rule.
	new_non_term.m_meaning = csm_expression;
	new_non_term.m_expression = new_expr_node;
}

void TCppParser::ProcessNewPlacement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	'('  ExpressionsList  ')'
	//
	int rlen = rule.Length();
	assert(rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);

	// Pick up expression from the second symbol and promote it up.
	assert(s2.m_meaning == csm_expression && s2.m_expression != NULL);
	new_non_term.m_meaning = csm_expression;
	new_non_term.m_expression = s2.m_expression;
	s2.ResetMeaning();
}
void TCppParser::ProcessNewTypeId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	TypeSpecifier
	//	TypeSpecifier  NewDeclarator
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	HandleGenericTypeIdSymbols(new_non_term, FirstRuleSym(rlen), rlen);
}

void TCppParser::ProcessNewDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	DirectNewDeclarator
	//
	//	PtrOperator
	//	PtrOperator  NewDeclarator
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	new_non_term.m_meaning = csm_declarator;
	if (s1.m_app_id == sym_PtrOperator)
	{
		if (rlen == 1)
		{
			// Short rule. Start building new declarator from scratch.
			new_non_term.m_declarator_info.Clear();
			new_non_term.m_declarator_info.m_new_expr_decl = TRUE;
			ApplyCvPtrRefModifier(&s1, new_non_term.m_declarator_info, s1.m_ptr_operator);
			s1.ResetMeaning();
		}
		else
		{
			// Long rule. Inherit already existing declarator.
			TCppSymbolStateItem &s2 = NextRuleSym(s1);
			assert(s2.m_meaning == csm_declarator);
			ApplyCvPtrRefModifier(&s2, s2.m_declarator_info, s1.m_ptr_operator);
			new_non_term.m_declarator_info = s2.m_declarator_info;
			s1.ResetMeaning();
			s2.ResetMeaning();
		}
	}
	else
	{
		// Promote meaning of the rule symbol up.
		assert(s1.m_meaning == csm_declarator);
		new_non_term.m_declarator_info = s1.m_declarator_info;
		s1.ResetMeaning();
	}
}

void TCppParser::ProcessDirectNewDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	'[' ExpressionsList ']'
	//	DirectNewDeclarator  '[' ExpressionsList ']'
	//
	int rlen = rule.Length();
	assert(rlen == 3 || rlen == 4);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1), &s3 = NextRuleSym(s2);

	new_non_term.m_meaning = csm_declarator;
	TCppSymbolStateItem *dims_list_symbol = NULL;
	if (rlen == 3)
	{
		// Short rule. Start building new declarator from scratch.
		new_non_term.m_declarator_info.Clear();
		new_non_term.m_declarator_info.m_new_expr_decl = TRUE;

		assert(s2.m_meaning == csm_expression);
		dims_list_symbol = &s2;
	}
	else
	{
		// Long rule. Inherit already existing declarator.
		new_non_term.m_declarator_info = s1.m_declarator_info;
		s1.ResetMeaning();

		assert(s3.m_meaning == csm_expression);
		dims_list_symbol = &s3;
	}

	// Apply modifiers and release the expression.
	ApplyMultiDimArrayModifier(dims_list_symbol, new_non_term.m_declarator_info, dims_list_symbol->m_expression);
	delete dims_list_symbol->m_expression;
	dims_list_symbol->ResetMeaning();
}

void TCppParser::ProcessNewInitializer(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	'('				 ')'
	//	'('  ExpressionsList	 ')'
	//
	int rlen = rule.Length();
	assert(rlen == 2 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen), &s2 = NextRuleSym(s1);

	if (rlen == 2)
	{
		// The expression in the source file is missing. On the other hand the non terminal that
		// is being created, has the expression meaning. Create new empty expression for this.
		TCppMultiOpExpr *new_expr_node = (TCppMultiOpExpr*)AllocateNewCppItem(cxx_multiop_expr);
		if (new_expr_node == NULL)
			return;

		// Set the comma operation there and set it into the non terminal.
		new_expr_node->SetOperation(opr_comma);
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = new_expr_node;
	}
	else
	{
		// Pick up expression from the second symbol and promote it up.
		assert(s2.m_meaning == csm_expression && s2.m_expression != NULL);
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = s2.m_expression;
		s2.ResetMeaning();
	}
}

void TCppParser::ProcessDeleteExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//		DELETE		CastExpression
	//		DELETE  '[' ']'	CastExpression
	//	'::'	DELETE		CastExpression
	//	'::'	DELETE  '[' ']'	CastExpression
	//
	int rlen = rule.Length();
	assert(rlen >= 2 && rlen <= 5);
	TCppSymbolStateItem *sym = FirstRuleSymPtr(rlen);

	// Delete expression object is needed.
	TCppDeleteExpr *new_expr_node = (TCppDeleteExpr*)AllocateNewCppItem(cxx_delete_expr);
	if (new_expr_node == NULL)
		return;

	if (sym->m_app_id == sym_dblcolon)
	{
		// Double colon is present in the rule.
		new_expr_node->m_global_delete = TRUE;
		sym = NextRuleSymPtr(sym);
	}

	// Skip the DELETE keyword.
	assert(sym->m_app_id == sym_delete);
	sym = NextRuleSymPtr(sym);

	if (sym->m_app_id == sym_lbracket)
	{
		// Set the flag and skip two symbols.
		new_expr_node->m_array_delete = TRUE;
		sym = NextRuleSymPtr(sym);

		assert(sym->m_app_id == sym_rbracket);
		sym = NextRuleSymPtr(sym);
	}

	// The local variable sym should point to the object that should be deleted.
	assert(sym->m_app_id == sym_CastExpression && sym->m_meaning == csm_expression);
	new_expr_node->AddChildItem(sym->m_expression, cexpr_chdest_delete_oprnd);
	sym->ResetMeaning();

	// Set prepared expression into the non terminal of the rule.
	new_non_term.m_meaning = csm_expression;
	new_non_term.m_expression = new_expr_node;
}

void TCppParser::ProcessPostfixExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	PrimaryExpression
	//
	//	PostfixExpression  '++'									__id(rcpp_pfix_plpl)
	//	PostfixExpression  '--'									__id(rcpp_pfix_mnmn)
	//
	//	PostfixExpression   '.'	ExpressionId						__id(rcpp_pfix_dot)
	//	PostfixExpression   '.'	TEMPLATE  ExpressionId			__id(rcpp_pfix_dot_tmpl)
	//	PostfixExpression  '->'	ExpressionId						__id(rcpp_pfix_arrow)
	//	PostfixExpression  '->'	TEMPLATE  ExpressionId			__id(rcpp_pfix_arrow_tmpl)
	//
	//	PostfixExpression  '[' ExpressionsList ']'					__id(rcpp_pfix_array)
	//
	//	PostfixExpression  '(' ')'									__id(rcpp_pfix_fcall_simp)
	//	PostfixExpression  '(' ExpressionsList ')'					__id(rcpp_pfix_fcall_prms)
	//
	//	SimpleTypeSpecifier  '(' ')'								__id(rcpp_pfix_ctor1)
	//	SimpleTypeSpecifier  '(' ExpressionsList ')'					__id(rcpp_pfix_ctor2)
	//	TYPENAME  SimpleOrQualifiedId  '(' ')'						__id(rcpp_pfix_ctor3)
	//	TYPENAME  SimpleOrQualifiedId  '(' ExpressionsList ')'		__id(rcpp_pfix_ctor4)
	//
	//	CONST_CAST	 '<' TypeId '>'  '(' Expression ')'				__id(rcpp_pfix_cs_cast)
	//	STATIC_CAST	 '<' TypeId '>'  '(' Expression ')'				__id(rcpp_pfix_st_cast)
	//	DYNA_CAST	 '<' TypeId '>'  '(' Expression ')'				__id(rcpp_pfix_dy_cast)
	//	REIN_CAST	 '<' TypeId '>'  '(' Expression ')'				__id(rcpp_pfix_re_cast)
	//
	//	TYPEID  '(' Expression ')'								__id(rcpp_pfix_tpid_expr)
	//	TYPEID  '(' TypeId ')'									__id(rcpp_pfix_tpid_type)
	//
	int rlen = rule.Length();
	assert(rlen >= 1 || rlen <= 5);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Symbol of the rule should be an expression.
		assert(s1.m_meaning == csm_expression && s1.m_expression != NULL);

		// Promote expression of the rule symbol up.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = s1.m_expression;
		s1.ResetMeaning();
	}
	else
	{
		// The expression is non trivial. Call an appropriate handler.
		TCppExpressionBase *new_expr = NULL;
		switch (rule.rule_app_id)
		{
			case rcpp_pfix_plpl:			new_expr = ExprHandlePostfixUnaryOperation(s1, opr_plpl_postfix);
										break;
			case rcpp_pfix_mnmn:			new_expr = ExprHandlePostfixUnaryOperation(s1, opr_mnmn_postfix);
										break;

			case rcpp_pfix_dot:
			case rcpp_pfix_dot_tmpl:		new_expr = ExprHandleDotArrowOperation(s1, opr_dot);
										break;
			case rcpp_pfix_arrow:
			case rcpp_pfix_arrow_tmpl:		new_expr = ExprHandleDotArrowOperation(s1, opr_arrow);
										break;

			case rcpp_pfix_array:			new_expr = ExprHandleArrayIndexOperation(s1, rlen);
										break;

			case rcpp_pfix_fcall_simp:
			case rcpp_pfix_fcall_prms:		new_expr = ExprHandleFunctionCallOperation(s1, rlen);
										break;

			case rcpp_pfix_ctor1:
			case rcpp_pfix_ctor2:
			case rcpp_pfix_ctor3:
			case rcpp_pfix_ctor4:			new_expr = ExprHandleCtorCallOperation(s1, rlen);
										break;

			case rcpp_pfix_cs_cast:		new_expr = ExprHandleTypeCastOperation(s1, opr_const_cast);
										break;
			case rcpp_pfix_st_cast:			new_expr = ExprHandleTypeCastOperation(s1, opr_static_cast);
										break;
			case rcpp_pfix_dy_cast:		new_expr = ExprHandleTypeCastOperation(s1, opr_dyna_cast);
										break;
			case rcpp_pfix_re_cast:		new_expr = ExprHandleTypeCastOperation(s1, opr_rein_cast);
										break;

			case rcpp_pfix_tpid_expr:
			case rcpp_pfix_tpid_type:		new_expr = ExprHandleTypeIdOperation(s1, rule.rule_app_id);
										break;

			default:
				RaiseUnexpectedGrammarRuleAppIdError(irule);
				return;
		}

		// Set prepared expression (or NULL in case of the critical error) into the non terminal of the rule.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = new_expr;
	}
}

void TCppParser::ProcessPrimaryExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	Literal
	//	THIS
	//	ExpressionId
	//	'(' Expression ')'
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 3);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		if (s1.m_app_id == sym_Literal)
		{
			// This is literal operand. Literal operand is an expression.
			assert(s1.m_meaning == csm_expression && s1.m_expression->ItemType() == cxx_operand_expr);

			// Promote this expression up.
			new_non_term.m_meaning = csm_expression;
			new_non_term.m_expression = s1.m_expression;
			s1.ResetMeaning();
		}
		else if (s1.m_app_id == sym_this)
		{
			// This is ''this" operand. Call the worker function.
			new_non_term.m_meaning = csm_expression;
			new_non_term.m_expression = ExprHandlePrimaryThis(s1);
		}
		else
		{
			// This is named operand. Call the worker function.
			new_non_term.m_meaning = csm_expression;
			new_non_term.m_expression = ExprHandlePrimaryExpessionId(s1);
		}
	}
	else
	{
		// This is expression in parenthesis.
		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		assert(s2.m_meaning == csm_expression && s2.m_expression != NULL);

		// Promote this expression up.
		new_non_term.m_meaning = csm_expression;
		new_non_term.m_expression = s2.m_expression;
		s2.ResetMeaning();
	}
}

void TCppParser::ProcessLiteral(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	FALSE | TRUE | number | charconst | StringsSeq
	//
	int rlen = rule.Length();
	assert(rlen == 1);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	// This non terminal needs operand object as the meaning of its rule symbol.
	TCppOperandExpr *oprnd_item = (TCppOperandExpr*)AllocateNewCppItem(cxx_operand_expr);
	if (oprnd_item == NULL)
		return;

	new_non_term.m_meaning = csm_expression;
	new_non_term.m_expression = oprnd_item;

	// All literals are constants. The l-value property is already FALSE.
	oprnd_item->m_const_expr = TRUE;

	// The result of picking up the built in type is not checked here because this function is exited
	// right after the switch. In the case of failure the function should be aslo exited.
	switch (s1.m_app_id)
	{
		case sym_false:
				{
					oprnd_item->SetBoolLiteral(FALSE);
					oprnd_item->m_result_type = GetBuiltInTypeDefn(bltin_id_bool);
				}
				break;

		case sym_true:
				{
					oprnd_item->SetBoolLiteral(TRUE);
					oprnd_item->m_result_type = GetBuiltInTypeDefn(bltin_id_bool);
				}
				break;

		case sym_number:
				{
					oprnd_item->SetNumericLiteral(s1.m_lex.number_type, s1.m_lex.num_value);
					oprnd_item->m_result_type = GetBuiltInTypeDefn(TCppExpressionBase::GetBuiltInTypeIdFromNumberSubtype(s1.m_lex.number_type));
				}
				break;

		case sym_charconst:
				{
					oprnd_item->SetCharConstLiteral(s1.m_lex.charconst_type, s1.m_lex.num_value);
					oprnd_item->m_result_type = GetBuiltInTypeDefn(TCppExpressionBase::GetBuiltInTypeIdFromCharconstSubtype(s1.m_lex.charconst_type));
				}
				break;

		case sym_StringsSeq:
				{
					assert(s1.m_meaning == csm_strings_seq);
					oprnd_item->SetStringLiteral(s1.m_strs_seq_value.m_string_subt, s1.m_strs_seq_value.m_string_value);
					oprnd_item->m_result_type = GetBuiltInTypeDefn(TCppExpressionBase::GetBuiltInTypeIdFromStringSubtype(oprnd_item->m_string_subt));
				}
				break;

		default:
			{
				// This is a grammar problem.
				RaiseUnexpectedRuleSymbolAppIdError(irule, &s1);
			}
			break;
	}
}

void TCppParser::ProcessStringsSeq(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule)
{
	//
	// Possible rule forms:
	//
	//	string
	//	StringsSeq  string
	//
	int rlen = rule.Length();
	assert(rlen == 1 || rlen == 2);
	TCppSymbolStateItem &s1 = FirstRuleSym(rlen);

	if (rlen == 1)
	{
		// Symbol of the rule should be the string lexema.
		assert(s1.m_app_id == sym_string && s1.m_lex.type == ltx_string);

		// Start building the symbol from scratch. Pointers from the lexema can be simply copied because
		// they are persistent at least till the end of compilation. Later on, when this string will be set into
		// some Cpp database object, this string will be moved to the strings dict of the Cpp database.
		new_non_term.m_meaning = csm_strings_seq;
		new_non_term.m_strs_seq_value.m_string_subt = s1.m_lex.string_type;
		new_non_term.m_strs_seq_value.m_string_value = s1.m_lex.str_value;
	}
	else
	{
		// Symbols of the rule should be a StringsSeq non terminal and a string lexema.
		TCppSymbolStateItem &s2 = NextRuleSym(s1);
		assert(s1.m_meaning == csm_strings_seq && s2.m_app_id == sym_string && s2.m_lex.type == ltx_string);

		// Move meaning from the first symbol of the rule to the new symbol.
		new_non_term.m_meaning = csm_strings_seq;
		new_non_term.m_strs_seq_value = s1.m_strs_seq_value;

		if (new_non_term.m_strs_seq_value.m_string_subt != s2.m_lex.string_type)
		{
			// Report an error and discard the second part of the string. Keep just the first part.
			ReportSyntaxError(&s2, cerr_strings_seq_subt_mismatch);
			return;
		}

		// Types of the strings are identical. Allocate local buffer for the merged string.
		TMessageBuffer merged_str_buffer;
		int first_str_body_len = new_non_term.m_strs_seq_value.m_string_value.m_len;
		int second_str_body_len = s2.m_lex.str_value.m_len;
		if (merged_str_buffer.ReserveSpace(first_str_body_len+second_str_body_len) == FALSE)
		{
			RaiseCriticalCppError(ccerr_oom_combining_literal_strings);
			return;
		}

		// Copy in the data.
		merged_str_buffer.Append(new_non_term.m_strs_seq_value.m_string_value.m_body, first_str_body_len);
		merged_str_buffer.Append(s2.m_lex.str_value.m_body, second_str_body_len);

		// Temp strore this string in the strings dict of the mid lev scanner.
		TStringPtr local_buff_ptr(merged_str_buffer.DataPtr(), first_str_body_len+second_str_body_len);
		wchar_t *new_str_data = m_scanner->StroreIntermediateString(local_buff_ptr);
		if (new_str_data == NULL)
		{
			RaiseCriticalCppError(ccerr_oom_combining_literal_strings);
			return;
		}

		// Caching suceeded. Set up the props of the non term.
		new_non_term.m_strs_seq_value.m_string_value.m_body = new_str_data;
		new_non_term.m_strs_seq_value.m_string_value.m_len = first_str_body_len+second_str_body_len;
	}
}

// ----------------------------------------------------------------------------
// - - - - - - - - - - - - - - -  Environment group  - - - - - - - - - - - - - - - - - - -
// ----------------------------------------------------------------------------

enum P12_EnvironmentGroup { EnvGrp_member };					// This enum is used for src code navigation purposes only.

bool TCppParser::PrepareSimpleNameInfo(TCppSymbolStateItem &identifier_symbol, TCppNameInfo &dest_info_buffer)
{
	// Ensure that passed symbol contains a name lexema.
	assert(identifier_symbol.m_app_id == sym_identifier);
	assert(identifier_symbol.m_lex.type == ltx_name);
	TStrPtrInfo &name_str = identifier_symbol.m_lex.str_value;

	// First step. Pick up or assign the KeyId to the passed name. Note that ids of the indexing keys
	// are assigned by the mid lev callback handler. At the same time the mid lev scanner maintains
	// the hash table of all indexing keys that were used in the current parsing session.
	TKeyInfoItem *key_info = m_scanner->GetIndexingKeyFromLex(identifier_symbol.m_lex);
	if (key_info == NULL)
	{
		// The low level error was already reported. Duplicate this error with the critical error.
		RaiseCriticalCppError(ccerr_key_id_picking_problem, name_str.m_len, name_str.m_body);
		return(FALSE);
	}

	// Clear the passed name info buffer and fill it with the discovered key id value.
	dest_info_buffer.SetupName(cnit_identifier, key_info->data.key_id);

	// Second step. Create or pick up the local copy of the name in the strings dict of the Cpp database.
	// This pointer most likely will be set into the Cpp database object that may live longer than the parsing
	// infrastructure. This is why this duplication is necessary.
	dest_info_buffer.m_cached_name = m_cpp_database.m_global_strings_dict.RegisterStr(name_str.m_body, name_str.m_len);
	if (dest_info_buffer.m_cached_name == NULL)
	{
		RaiseCriticalCppError(ccerr_oom_storing_name_string, name_str.m_len);
		return(NULL);
	}

	// Third step. Pick up the leftmost template header if any that is applicable to this name.
	dest_info_buffer.m_unproc_template_header = GetFirstTemplateHeaderSymbol();

	// Forth step. Fill in the source area of this name.
	dest_info_buffer.m_qual_name_context = identifier_symbol.GetOriginHeader();
	dest_info_buffer.m_short_name_context = dest_info_buffer.m_qual_name_context;

	// Success.
	return(TRUE);
}

bool TCppParser::DoHandleStandAloneDeclTypeSpec(TCppSymbolStateItem &decl_type_spec_sym)
{
	// Check the passed symbol.
	assert(decl_type_spec_sym.m_meaning == csm_decl_type_spec);
	TCppDeclTypeSpecInfo &dts = decl_type_spec_sym.m_decl_type_spec_info;

	// (1) Check the template headers if any. Current situation is special because decl type spec
	// may not contain any name info at all. This is why the template header context is checked.
	if (IsTemplateHeaderContext() == TRUE)
	{
		// The template headers are present. Check the primary type.
		if (dts.m_dclt_blt_in_info.IsEmpty() == FALSE)
		{
			// Built in type cannot be used with template headers.
			ReportUnexpectedTemplateHeaders(L"the stand alone built in type");
		}
		else if (dts.m_dclt_unres_name_info.IsEmpty() == FALSE)
		{
			if (dts.m_dclt_unres_name_info.m_unproc_template_header != NULL)
			{
				// Write template headers errors for named type spec and elaborated enum type spec right now.
				// Validity of template headers for the elaborated struct/union/class type specifier will be checked
				// later when this elaborated type specifier will be resolved.
				if (dts.m_dclt_unres_name_elab == elab_none)
					ReportUnexpectedTemplateHeaders(L"the stand alone type name", dts.m_dclt_unres_name_info.m_unproc_template_header);
				else if (dts.m_dclt_unres_name_elab == elab_enum)
					ReportUnexpectedTemplateHeaders(L"the forward declaration of the enum type", dts.m_dclt_unres_name_info.m_unproc_template_header);
			}
		}
		else if (dts.m_dclt_base_type != NULL)
		{
			assert(dts.m_dclt_local_enum_class == TRUE);
			if (dts.m_dclt_base_type->DefnElabType() == elab_enum)
			{
				// This is enum definition with template header.
				ReportUnexpectedTemplateHeaders(L"the enum definition");
			}
		}
		else
		{
			// There is no base type at all. Write some neutral message.
			ReportUnexpectedTemplateHeaders(L"the stand alone type specifier");
		}
	}

	// (2) Check the decl type spec prefix if any.
	bool want_friend_proxy = FALSE;
	if (dts.m_dclt_prefix == declpr_typedef)
	{
		// Typedef is not allowed in the stand alone decl type spec.
		ReportSyntaxError(&decl_type_spec_sym, cerr_sdecl_bogus_typedef_prefix);
	}
	else if (dts.m_dclt_prefix == declpr_friend)
	{
		if (m_defns_layer_owner->IsStructType() == TRUE)
		{
			// Accept the FRIEND specifier. For now this means only its presence and the right context.
			want_friend_proxy = TRUE;
		}
		else
		{
			// Friend specifier stays outside of the class context.
			ReportSyntaxError(&decl_type_spec_sym, cerr_context_bad_friend);
		}
	}

	// (3) Check the function attrs, storage class and the cv spec fields if any.
	if (dts.m_dclt_func_attrs != fcta_none)
		ReportUnappropFunctionAttrs(&decl_type_spec_sym, NULL, cerr_sdecl_bogus_func_attr, dts.m_dclt_func_attrs);
	if (dts.m_dclt_strg_class != strgc_none)
		ReportSyntaxError(&decl_type_spec_sym, cerr_sdecl_bogus_strg_class, TCppDataFieldDefn::GetStorageClassFriendlyName(dts.m_dclt_strg_class));
	if (dts.m_dclt_cv_spec.IsEmpty() == FALSE)
		ReportSyntaxError(&decl_type_spec_sym, cerr_sdecl_bogus_cv_spec);

	// (4) Check the base type.
	if (dts.m_dclt_blt_in_info.IsEmpty() == FALSE)
	{
		// Built in type does not make sence in the stand alone decl type specifier.
		if (dts.m_dclt_prefix == declpr_friend)
			ReportSyntaxError(&decl_type_spec_sym, cerr_sdecl_bogus_friend_spec, L"to the stand alone built in type");
		ReportSyntaxError(&decl_type_spec_sym, cerr_sdecl_bogus_built_in_type);
	}
	else if (dts.m_dclt_unres_name_info.IsEmpty() == FALSE)
	{
		TCppNameInfo &name_info = dts.m_dclt_unres_name_info;
		assert(name_info.IsIdentifier() == TRUE || name_info.IsTempalteId() == TRUE);

		// This is an unprocessed named type spec or an elaborated type specifier.
		if (dts.m_dclt_unres_name_elab == elab_none)
		{
			// Named type spec cannot be used in the stand alone decl type specifier.
			if (dts.m_dclt_prefix == declpr_friend)
				ReportSyntaxError(&decl_type_spec_sym, cerr_sdecl_bogus_friend_spec, L"to the stand alone type name");
			ReportSyntaxError(&decl_type_spec_sym, cerr_sdecl_bogus_named_type);
		}
		else
		{
			if (dts.m_dclt_unres_name_elab != elab_enum)
			{
				// This is struct/union/class. Additional checks are needed.
				if (name_info.IsTempalteId() == TRUE && name_info.m_unproc_template_header == NULL)
				{
					// This is request for explicit template instantiation without an appropriate template header.
					ReportSyntaxError(&decl_type_spec_sym, cerr_sdecl_bogus_exinst_hdr_missing);
				}

				if (name_info.m_unproc_template_header != NULL && name_info.m_unproc_template_header->m_template_info.m_child_template_header != NULL)
				{
					// Current template header is not the final header in the sequence of template headers.
					ReportUnprocessedTemplateHeaders(name_info.m_unproc_template_header->m_template_info.m_child_template_header);
				}
			}

			// Call the special procedure that will either retrieve an existing type object or create a new one.
			dts.m_dclt_base_type = FindOrCreateElaboratedTypeObject(&decl_type_spec_sym, dts.m_dclt_unres_name_elab, name_info, TRUE, want_friend_proxy);
			if (dts.m_dclt_base_type == NULL)
				return(FALSE);

			assert(dts.m_dclt_base_type->IsUnresolvedObject() || dts.m_dclt_base_type->DefnElabType() == dts.m_dclt_unres_name_elab);
			if (dts.m_dclt_prefix == declpr_friend)
			{
				if (dts.m_dclt_base_type->DefnElabType() == elab_enum)
					ReportSyntaxError(&decl_type_spec_sym, cerr_sdecl_bogus_friend_spec, L"to the forward declaration of the enum type");
				else if (want_friend_proxy == TRUE)
					CreateAndPublishFriendRecord(&decl_type_spec_sym, dts.m_dclt_base_type);
			}

			// Link created of discovered object to the current place in the code.
			WriteObjectIntroEvent(&decl_type_spec_sym, clink_fwd_decl, dts.m_dclt_base_type,
							(TCppSourceAreaType)(cxa_enum_fwd_decl+dts.m_dclt_base_type->DefnElabType()-elab_enum), &name_info.m_short_name_context);
		}
	}
	else if (dts.m_dclt_base_type != NULL)
	{
		assert(dts.m_dclt_local_enum_class == TRUE);
		if (dts.m_dclt_prefix == declpr_friend)
		{
			if (dts.m_dclt_base_type->DefnElabType() == elab_enum)
			{
				// Friend spec stays in front of the local enum definition.
				ReportSyntaxError(&decl_type_spec_sym, cerr_sdecl_bogus_friend_spec, L"to the enum definition");
			}
			else
			{
				// Friend spec in front of the local definition of the class.
				assert(dts.m_dclt_base_type->DefnElabType() != elab_none);
				if (want_friend_proxy == TRUE)
					CreateAndPublishFriendRecord(&decl_type_spec_sym, dts.m_dclt_base_type);
			}
		}
	}
	else
	{
		// The base type info is missing.
		if (dts.m_dclt_prefix == declpr_friend)
			ReportSyntaxError(&decl_type_spec_sym, cerr_sdecl_bogus_friend_spec, L"in this context");
		ReportSyntaxError(&decl_type_spec_sym, cerr_sdecl_bogus_type_missing);
	}

	// Success.
	return(TRUE);
}

TCppDefnBase *TCppParser::DoHandleGenericDeclarator(TCppSymbolStateItem &declarator_sym, bool func_header_ctx)
{
	// The passed declarator symbol should be meaningful. Ensure this.
	assert(declarator_sym.m_meaning == csm_declarator);
	TCppNameInfo &name_info = declarator_sym.m_declarator_info.m_name_info;

	// Compute the value of the friend flag without checking if it is applicable to the object that is currently being
	// defined or not. Error messages will be generated later. It turns out that this version of the code is simpler.
	bool friend_flag = FALSE;
	if (m_curr_decl_type_spec != NULL && m_curr_decl_type_spec->m_decl_type_spec_info.m_dclt_prefix == declpr_friend)
	{
		// Friend flag is valid only in the context of the class.
		friend_flag = (m_defns_layer_owner->IsStructType() == TRUE);
	}

	// Pick up the layer where the function object is going to be defined. Note that the friend flag is used to determine
	// the layer. Data fields and typedefs may be defined on a different layer.
	TCppDefnBase *func_defns_layer;
	if (name_info.m_resolved_defns_layer != NULL)
	{
		// Name info contains an explicit layer. Use this layer.
		func_defns_layer = name_info.m_resolved_defns_layer;
	}
	else if (m_defns_layer_owner->IsBlock() == TRUE || friend_flag == TRUE)
	{
		// The name in the name info is short. This was verified a few lines above. Adjust passed current layer
		// to the closest upper namespace layer. Note that friend flag can be TRUE only when FIEND spec stays
		// on the struct layer. This was laso verified a few lines above.
		func_defns_layer = m_defns_layer_owner->GetNamespaceParent();
		assert(func_defns_layer != NULL);
	}
	else
	{
		// It is ok to use the current layer.
		func_defns_layer = m_defns_layer_owner;
	}

	// Do the first switch based on the subtype of the name.
	TCppNameInfoType cnit = name_info.m_core.m_info_type;
	if (cnit == cnit_identifier || cnit == cnit_template_id)
	{
		// The name is an identifier. Check the name.
		if (func_defns_layer->IsStructType() == TRUE && name_info.KeyId() == func_defns_layer->DefnKeyId())
		{
			// The target layer is a struct/union/class and the name in the declarator matches the name of the layer.
			// Process this declarator as constructor. Here is a caviat because structures/templates that were created
			// using alt names have zero int the indexing key id field. This means that they cannot have ctors.
			name_info.m_core.m_info_type = (TCppNameInfoType)(name_info.m_core.m_info_type+2);
			return(DoHandleTypelessFuncDecl(friend_flag, func_defns_layer, declarator_sym, func_header_ctx));
		}
	}
	else if (cnit == cnit_ovld_operator || cnit == cnit_ovld_op_template)
	{
		// Declarator contains name of an overloaded operator.
		return(DoHandleFunctionDeclarator(friend_flag, func_defns_layer, declarator_sym, func_header_ctx));
	}
	else
	{
		// Declarator contains name of a destructor or a name of an overloaded conversion.
		assert(cnit == cnit_dector || cnit == cnit_ovld_conversion || cnit == cnit_ovld_conv_template);
		return(DoHandleTypelessFuncDecl(friend_flag, func_defns_layer, declarator_sym, func_header_ctx));
	}

	// Remaining cases are data field, typedef and simple fucntion.
	assert(name_info.IsIdentifierOrTempalteId() == TRUE);
	if (func_header_ctx == TRUE)
	{
		// Declarator is followed by the beginning of the fucntion body. Process this declarator as
		// a function header even if does not have a list of params after the name.
		return(DoHandleFunctionDeclarator(friend_flag, func_defns_layer, declarator_sym, TRUE));
	}
	else
	{
		if (m_curr_decl_type_spec != NULL && m_curr_decl_type_spec->m_decl_type_spec_info.m_dclt_prefix == declpr_typedef)
		{
			// The decl type spec is present and it contains the TYPEDEF keyword. This is definition of the type.
			return(DoHandleTypedefDeclarator(declarator_sym));
		}
		else if (declarator_sym.m_declarator_info.IsCodeEntryPointStyle() == FALSE)
		{
			// There is no TYPEDEF keyword and there is no list of params. This is data field.
			return(DoHandleDataFieldDeclarator(declarator_sym));
		}
		else
		{
			// The list of params is available. This is prototype of the function.
			return(DoHandleFunctionDeclarator(friend_flag, func_defns_layer, declarator_sym, FALSE));
		}
	}
}

TCppDefnBase *TCppParser::DoHandleDataFieldDeclarator(TCppSymbolStateItem &declarator_sym)
{
	assert(declarator_sym.m_meaning == csm_declarator);
	TCppNameInfo &name_info = declarator_sym.m_declarator_info.m_name_info;
	TCppDefnBase *defns_layer = name_info.GetDefnsLayer(m_defns_layer_owner);

	// The rule that describes the data field contains the non terminal ExpressionId in it. It may seem that the name
	// in the declarator may have any form (overloaded operator, etc) here. This is not so, because the caller code
	// when it decides if it is data field, typedef or function, passes only identifiers and TemplateIds here.
	assert(name_info.IsIdentifierOrTempalteId() == TRUE);

	// Check for the template headers.
	if (name_info.m_unproc_template_header != NULL)
	{
		// Data field can stay inside the template but it cannot be a template itself.
		ReportUnprocessedTemplateHeaders(name_info.m_unproc_template_header);
	}

	// Pick up info from the decl type spec.
	TCppStorageClass strg_class;
	TCppCvPtrRefSpec cv_spec;
	TCppDefnBase *base_type;

	if (m_curr_decl_type_spec != NULL)
	{
		TCppDeclTypeSpecInfo &dts = m_curr_decl_type_spec->m_decl_type_spec_info;
		assert(dts.m_dclt_prefix != declpr_typedef);

		if (dts.m_dclt_prefix == declpr_friend)
		{
			// Friend specifier cannot be used with the data field.
			TCppErrorReportingCtxSymbolStateItem error_context(name_info);
			ReportSyntaxError(&error_context, cerr_data_field_bad_friend);
		}

		if (dts.m_dclt_func_attrs != fcta_none)
		{
			// Function attrs cannot be used with the data field.
			TCppErrorReportingCtxSymbolStateItem error_context(name_info);
			ReportUnappropFunctionAttrs(&error_context, NULL, cerr_data_field_bad_func_attr, dts.m_dclt_func_attrs);
		}

		// Check the storage class against the type of the destination layer.
		strg_class = dts.m_dclt_strg_class;
		if (defns_layer->IsNamespace() == TRUE)
		{
			if (strg_class == strgc_register || strg_class == strgc_auto || strg_class == strgc_mutable)
			{
				// These storage classes are not allowed on the namespace layer.
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&error_context, cerr_data_field_bad_strg_class, TCppDataFieldDefn::GetStorageClassFriendlyName(strg_class), L"on the namespace layer");
				strg_class = strgc_none;
			}
		}
		else if (defns_layer->IsStructType() == TRUE)
		{
			if (strg_class == strgc_register || strg_class == strgc_auto || strg_class == strgc_extern)
			{
				// These storage classes are not allowed on the struct/union/class layer.
				wchar_t layer_type_name_buff[80], context_name_buff[120];
				swprintf(context_name_buff, 120, L"inside the %s", defns_layer->GetObjectTypeFriendlyName(layer_type_name_buff));
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&error_context, cerr_data_field_bad_strg_class, TCppDataFieldDefn::GetStorageClassFriendlyName(strg_class), context_name_buff);
				strg_class = strgc_none;
			}
		}
		else
		{
			// Destination layer can be a block layer only when the name in the declarator is a short because
			// blocks do not have layer names. They cannot be specified using any qualifying path.
			assert(defns_layer->IsBlock() == TRUE && name_info.m_resolved_defns_layer == NULL);

			if (strg_class == strgc_extern)
			{
				// This is special case. Adjust the destination layer.
				defns_layer->GetNamespaceParent();
				assert(defns_layer != NULL);
			}
			else if (strg_class == strgc_mutable)
			{
				// Storage class MUTABLE is an attribute that is applicable only to instance data fields inside classes.
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&error_context, cerr_data_field_bad_strg_class, TCppDataFieldDefn::GetStorageClassFriendlyName(strg_class), L"in the context of the function body");
				strg_class = strgc_none;
			}
		}

		if (CheckPrepareTypeSpecifier(*m_curr_decl_type_spec) == FALSE)
			return(NULL);

		// Pick up the cv spec and the base type.
		assert(dts.m_dclt_base_type != NULL);
		cv_spec = dts.m_dclt_cv_spec;
		base_type = dts.m_dclt_base_type;
	}
	else
	{
		// The decl type spec is missing.
		ReportSyntaxError(&declarator_sym, cerr_data_field_base_type_missing);

		// Setup the default values.
		strg_class = strgc_none;
		cv_spec.Clear();
		base_type = GetBuiltInTypeDefn(bltin_id_void);
		if (base_type == NULL)
			return(NULL);
	}

	// Check the type of the name in the declarator.
	if (name_info.IsTempalteId() == TRUE)
	{
		// Template arguments are not allowed in the data field.
		TCppErrorReportingCtxSymbolStateItem error_context(name_info);
		ReportSyntaxError(&error_context, cerr_data_field_bad_template_args);
		name_info.DowngradeTemplateIdToIdentifier(this);
	}

	// Assemble the type.
	TCppDataTypeBase *data_type = (TCppDataTypeBase*)AssembleDeclaratorType(m_curr_decl_type_spec, FALSE, cv_spec, base_type, declarator_sym);
	assert(data_type == NULL || data_type->IsDataType() == TRUE);
	if (data_type == NULL)
		return(NULL);

	// Veryfy the destination layer and lookup this name on that layer.
	TCppDataFieldDefn *existing_data_field = NULL;
	if (CheckLookupDataFieldObject(defns_layer, strg_class, name_info, existing_data_field) == FALSE)
		return(NULL);

	if (existing_data_field != NULL)
	{
		// Verify props of the returned object for compatibility with props of the current definition.
		bool props_compatible = TRUE;

		if (m_curr_linkage_spec != NULL)
		{
			// Standard explicitly states that linkage spec can be omitted in subsequent declarations/definitions.
			// The spec is currently present. This means it should match the linkage spec in the discovered object.
			if (TCppLinkageSpec::CompareLinkageSpecs(existing_data_field->m_linkage_spec, m_curr_linkage_spec->m_linkage_spec.m_link_spec) == FALSE)
			{
				TTextBuffer256 name_buffer;
				const wchar_t *full_name = TCppDatabase::GenerateQualifiedName(name_buffer, existing_data_field);

				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				if (existing_data_field->m_linkage_spec == NULL)
					ReportSyntaxError(&error_context, cerr_data_field_diff_linkage_specs1, full_name);
				else ReportSyntaxError(&error_context, cerr_data_field_diff_linkage_specs2, full_name, existing_data_field->m_linkage_spec->m_spec_name);

				props_compatible = FALSE;
			}
		}

		if (TCppItemBase::CompareSameDatabaseDataTypes(existing_data_field->m_field_type, data_type) == FALSE)
		{
			TTextBuffer256 name_buffer;
			TCppErrorReportingCtxSymbolStateItem error_context(name_info);
			ReportSyntaxError(&error_context, cerr_data_field_diff_data_types, TCppDatabase::GenerateQualifiedName(name_buffer, existing_data_field));
			props_compatible = FALSE;
		}

		if (props_compatible == TRUE)
		{
			if (strg_class == strgc_extern)
			{
				// Add reference event to the existing object.
				assert(defns_layer->IsNamespace() == TRUE);
				WriteObjectIntroEvent(&declarator_sym, clink_fwd_decl, existing_data_field, cxa_data_field_xtrn, &name_info.m_short_name_context);
			}
			else
			{
				// Lookup procedure discovered data field definition with the storage class EXTERN.
				// Change the strorage class in this definition.
				assert(existing_data_field->m_strg_class == strgc_extern);

				if (defns_layer->IsNamespace() == TRUE)
				{
					// Change the storage class either to "none" or "static".
					existing_data_field->m_strg_class = strg_class;
					if (strg_class == strgc_static)
						existing_data_field->m_visibility_pars_sess = GetParsingId();
				}
				else
				{
					// This is confirmation of defn of the static data field of the class from outside of the class.
					// Without such confirmation linkers comlain that the field is missing.
					assert(defns_layer->IsStructType() == TRUE);
					existing_data_field->m_strg_class = strgc_static;
				}

				// Add reference event to the existing object.
				WriteObjectIntroEvent(&declarator_sym, clink_definition, existing_data_field, cxa_data_field_defn, &name_info.m_short_name_context);
			}

			// Return already existing data field to the caller.
			return(existing_data_field);
		}

		// Discovered object is not compatible. Create new object with an alt name.
		if (name_info.SetupShortNameDefnProblem(this, cdpr_dupe) == FALSE)
			return(NULL);
	}

	// The new data field object should be created. Note that this can be either a "correct" object with the name
	// from the source code or a "problematic" object with a generated name.
	TCppDataFieldDefn *new_data_field = CreateDataFieldObject(defns_layer, name_info, strg_class, data_type);
	if (new_data_field == NULL)
		return(NULL);

	if (strg_class == strgc_extern)
		WriteObjectIntroEvent(&declarator_sym, clink_fwd_decl, new_data_field, cxa_data_field_xtrn, &name_info.m_short_name_context);
	else WriteObjectIntroEvent(&declarator_sym, clink_definition, new_data_field, cxa_data_field_defn, &name_info.m_short_name_context);

	return(new_data_field);
}

TCppDefnBase *TCppParser::DoHandleTypedefDeclarator(TCppSymbolStateItem &declarator_sym)
{
	assert(declarator_sym.m_meaning == csm_declarator);
	TCppNameInfo &decl_name_info = declarator_sym.m_declarator_info.m_name_info;

	// The rule that describes the typedef contains the non terminal ExpressionId in it. It may seem that the name
	// in the declarator may have any form (overloaded operator, etc) here. This is not so, because the upper code
	// when it decides if it is data field, typedef or function, passes only identifiers and TemplateIds here.
	assert(decl_name_info.IsIdentifierOrTempalteId() == TRUE);

	// Check for the template headers.
	if (decl_name_info.m_unproc_template_header != NULL)
	{
		// Definition of the type can stay inside the template but it cannot be a template itself.
		ReportUnprocessedTemplateHeaders(decl_name_info.m_unproc_template_header);
		decl_name_info.m_unproc_template_header = NULL;
	}

	// Pick up info from the decl type spec. It should be always present.
	assert(m_curr_decl_type_spec != NULL);
	TCppDeclTypeSpecInfo &dts = m_curr_decl_type_spec->m_decl_type_spec_info;
	assert(dts.m_dclt_prefix == declpr_typedef);

	if (dts.m_dclt_func_attrs != fcta_none)
	{
		// Function attrs cannot be used with the typedef.
		TCppErrorReportingCtxSymbolStateItem error_context(decl_name_info);
		ReportUnappropFunctionAttrs(&error_context, NULL, cerr_typedef_bad_func_attr, dts.m_dclt_func_attrs);
	}

	if (dts.m_dclt_strg_class != strgc_none)
	{
		// Type definition cannot have any specific storage class.
		TCppErrorReportingCtxSymbolStateItem error_context(decl_name_info);
		ReportSyntaxError(&error_context, cerr_typedef_bad_strg_class);
	}

	if (CheckPrepareTypeSpecifier(*m_curr_decl_type_spec) == FALSE)
		return(NULL);

	if (decl_name_info.IsQualified() == TRUE)
	{
		// Typedef allows only local definitions.
		TCppErrorReportingCtxSymbolStateItem error_context(decl_name_info);
		ReportSyntaxError(&error_context, cerr_typedef_bad_qual_path);
		decl_name_info.DowngradeToUnqualifiedNameInfo(this);
	}

	if (decl_name_info.IsTempalteId() == TRUE)
	{
		// Template arguments are not allowed in the TYPEDEF declaration.
		TCppErrorReportingCtxSymbolStateItem error_context(decl_name_info);
		ReportSyntaxError(&error_context, cerr_typedef_bad_template_args);
		decl_name_info.DowngradeTemplateIdToIdentifier(this);
	}

	// Assemble the type. This will also create an appropriate type object.
	assert(decl_name_info.IsSimpleIdentifier() == TRUE);
	TCppDefnBase *definition = AssembleDeclaratorType(m_curr_decl_type_spec, TRUE, dts.m_dclt_cv_spec, dts.m_dclt_base_type, declarator_sym);
	if (definition == NULL)
		return(NULL);

	// Check situation for this name on the current layer.
	TCppNameInfo name_info;
	TCppDefnBase *existing_typedef_object = definition;
	if (CheckLookupSimpleIdentifierSymbol(declarator_sym, name_info, cxx_cv_ptr_ref_type, &existing_typedef_object) == FALSE)
		return(NULL);

	if (existing_typedef_object == NULL)
	{
		// Compatible typedef object was not found. Give out the typedef object, that was constructed
		// while the declarator type was assembled. Although this typedef may show up under an alt name.
		assert(definition->ItemId() == 0);
		if (name_info.m_preferred_object_id != 0)
		{
			// The names collision was detected. Setup the preferred id.
			definition->m_cib_id = name_info.m_preferred_object_id;
		}

		// Fill in the name.
		if (definition->IsFunctionName() == TRUE)
			name_info.SetNameToFunctionObject((TCppFunctionTypeDefn*)definition);
		else name_info.SetNameToObject(definition);
	}
	else
	{
		// Discard the constructed definition and use the discovered object.
		delete definition;
		definition = existing_typedef_object;
	}

	// Success.
	return(definition);
}

TCppDefnBase *TCppParser::DoHandleFunctionDeclarator(bool want_friend_proxy, TCppDefnBase *dest_defns_layer, TCppSymbolStateItem &declarator_sym, bool func_header_ctx)
{
	assert(declarator_sym.m_meaning == csm_declarator);
	TCppNameInfo &name_info = declarator_sym.m_declarator_info.m_name_info;
	wchar_t buff96[96];

	// Pick up info from the decl type spec.
	TCppFunctionAttr func_attrs;
	TCppCvPtrRefSpec cv_spec;
	TCppDefnBase *base_type;
	PrEventHeader base_type_context;

	if (m_curr_decl_type_spec != NULL)
	{
		// The decl type specifier is present. The local var below is needed primarely for debugging.
		TCppDeclTypeSpecInfo &dts = m_curr_decl_type_spec->m_decl_type_spec_info;

		// Check the decl type spec prefix, func attrs and the storage class if any.
		func_attrs = CheckFunctionDeclTypeSpec(dest_defns_layer, declarator_sym, func_header_ctx);

		// Check/resolve the base type.
		if (CheckPrepareTypeSpecifier(*m_curr_decl_type_spec) == FALSE)
			return(NULL);

		// Pick up the cv spec and the base type.
		assert(dts.m_dclt_base_type != NULL);
		cv_spec = dts.m_dclt_cv_spec;
		base_type = dts.m_dclt_base_type;
		base_type_context = m_curr_decl_type_spec->GetOriginHeader();
	}
	else
	{
		// The decl type spec is missing.
		ReportSyntaxError(&declarator_sym, cerr_func_decl_base_type_missing, name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));

		// Setup the default values.
		func_attrs = fcta_none;
		cv_spec.Clear();
		base_type_context = declarator_sym.GetOriginHeader();

		// Pick up the return value type subst.
		base_type = GetBuiltInTypeDefn(bltin_id_void);
		if (base_type == NULL)
			return(NULL);
	}

	// Assemble the type of the return value.
	TCppDeclaratorModifier *unapplied_func_modifier = NULL;
	TCppDataTypeBase *data_type = (TCppDataTypeBase*)AssembleDeclaratorType(m_curr_decl_type_spec, FALSE, cv_spec, base_type, declarator_sym, &unapplied_func_modifier);
	assert(data_type == NULL || data_type->IsDataType() == TRUE);
	if (data_type == NULL)
		return(NULL);

	// The type was asssembled. Check the presence and validity of the function parameters.
	if (unapplied_func_modifier == NULL)
	{
		TCppErrorReportingCtxSymbolStateItem error_context(name_info);
		ReportSyntaxError(&error_context, cerr_func_decl_func_params_missing, name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
	}
	else
	{
		if (dest_defns_layer->IsNamespace() == TRUE)
		{
			// Object belongs to the namespace.
			if (unapplied_func_modifier->m_cvpr_spec.cvpr_const == TRUE)
			{
				TCppErrorReportingCtxSymbolStateItem error_context(unapplied_func_modifier->m_func_spec.m_this_cv_spec_context);
				ReportSyntaxError(&error_context, cerr_func_decl_namesp_func_this_spec, L"const");
				unapplied_func_modifier->m_cvpr_spec.cvpr_const = FALSE;
			}

			if (unapplied_func_modifier->m_cvpr_spec.cvpr_volatile == TRUE)
			{
				TCppErrorReportingCtxSymbolStateItem error_context(unapplied_func_modifier->m_func_spec.m_this_cv_spec_context);
				ReportSyntaxError(&error_context, cerr_func_decl_namesp_func_this_spec, L"volatile");
				unapplied_func_modifier->m_cvpr_spec.cvpr_volatile = FALSE;
			}
		}
		else  if (dest_defns_layer->IsStructType() == TRUE && (func_attrs & fcta_static) != 0)
		{
			// Object is a static member of the class.
			if (unapplied_func_modifier->m_cvpr_spec.cvpr_const == TRUE)
			{
				TCppErrorReportingCtxSymbolStateItem error_context(unapplied_func_modifier->m_func_spec.m_this_cv_spec_context);
				ReportSyntaxError(&error_context, cerr_func_decl_static_method_this_spec, L"const", dest_defns_layer->GetElabTypeName(FALSE, TRUE));
				unapplied_func_modifier->m_cvpr_spec.cvpr_const = FALSE;
			}

			if (unapplied_func_modifier->m_cvpr_spec.cvpr_volatile == TRUE)
			{
				TCppErrorReportingCtxSymbolStateItem error_context(unapplied_func_modifier->m_func_spec.m_this_cv_spec_context);
				ReportSyntaxError(&error_context, cerr_func_decl_static_method_this_spec, L"volatile", dest_defns_layer->GetElabTypeName(FALSE, TRUE));
				unapplied_func_modifier->m_cvpr_spec.cvpr_volatile = FALSE;
			}
		}
	}

	// Check for special case where template args are a straight repetition of params from the template header.
	name_info.CheckRemoveTemplateDefnTemplateArgs(this);

	// For now process only functions and func templates.
	if (name_info.TemplateArgsPresent() == TRUE)
	{
		if (func_header_ctx == FALSE && name_info.TemplateHeaderParams() == NULL)
		{
			RaiseNotImplementedError(L"FunctionTemplateExplicitInstantiation");
			return(NULL);
		}
		else
		{
			RaiseNotImplementedError(L"FunctionTemplateSpecialization");
			return(NULL);
		}
	}

	if (name_info.m_unproc_template_header != NULL)
	{
		// The function template is being defined.
		name_info.m_unproc_template_header->m_template_info.m_header_processed = TRUE;

		if (name_info.m_unproc_template_header->m_template_info.m_child_template_header != NULL)
		{
			// Current template header is not the final header in the sequence of template headers.
			ReportUnprocessedTemplateHeaders(name_info.m_unproc_template_header->m_template_info.m_child_template_header);
		}

		if (name_info.TemplateHeaderParams() == NULL || name_info.TemplateHeaderParams()->IsEmpty() == TRUE)
		{
			// The template keyword is present while the template params are missing.
			TCppErrorReportingCtxSymbolStateItem error_context(name_info);
			ReportSyntaxError(&error_context, cerr_func_decl_templ_params_missing, name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
		}
	}

	if (name_info.IsUnresolvedLayer() == TRUE)
	{
		// Lookup is not needed. Just prepare name for creating the function object.
		if (name_info.FinalizeLayerResProblemName(this) == FALSE)
			return(NULL);
	}
	else
	{
		// Veryfy destination layer and search for existing objects on that layer.
		TCppCodeEntryPointDefn *compatible_func_object = NULL;
		if (CheckLookupFucntionObject(want_friend_proxy, dest_defns_layer, name_info, unapplied_func_modifier, compatible_func_object, func_header_ctx) == FALSE)
			return(NULL);

		if (compatible_func_object != NULL)
		{
			// Only the function prototype and the func template prototype should be returned. Ensure this.
			assert(compatible_func_object->ItemType() == cxx_code_entry_point || compatible_func_object->ItemType() == cxx_func_template);
			assert(compatible_func_object->m_function_body == NULL);

			// Compare generic additional props first.
			bool extra_props_matched = CheckExistingFunctionProps(dest_defns_layer, func_attrs, name_info, unapplied_func_modifier, compatible_func_object, func_header_ctx);

			if ((func_attrs & fcta_static) != 0 && (compatible_func_object->m_func_attrs & fcta_static) == 0)
			{
				// Unexpected STATIC spec.
				assert(m_curr_decl_type_spec != NULL);
				TTextBuffer256 name_buffer;
				const wchar_t *func_obj_type_name = name_info.GetFuncNameCs(buff96, dest_defns_layer, func_header_ctx);
				const wchar_t *full_func_name = TCppDatabase::GenerateQualifiedName(name_buffer, compatible_func_object);

				if (dest_defns_layer->IsNamespace() == TRUE)
					ReportSyntaxError(m_curr_decl_type_spec, cerr_func_decl_function_unxpct_static, func_obj_type_name, full_func_name);
				else ReportSyntaxError(m_curr_decl_type_spec, cerr_func_decl_method_unxpct_static, func_obj_type_name, full_func_name, dest_defns_layer->GetElabTypeName());

				extra_props_matched = FALSE;
			}

			// This method handles only typed functions. This means that the type should be checked.
			if (TCppItemBase::CompareSameDatabaseDataTypes(compatible_func_object->m_return_value_type, data_type) == FALSE)
			{
				// Return value types are different.
				TCppErrorReportingCtxSymbolStateItem error_context(base_type_context);
				ReportSyntaxError(&error_context, cerr_func_decl_ret_value_mismatch);
				extra_props_matched = FALSE;
			}

			if (extra_props_matched == TRUE)
			{
				if (want_friend_proxy == TRUE)
				{
					TCppErrorReportingCtxSymbolStateItem error_context(name_info);
					CreateAndPublishFriendRecord(&error_context, compatible_func_object);
				}

				if (func_header_ctx == FALSE)
					WriteObjectIntroEvent(&declarator_sym, clink_fwd_decl, compatible_func_object, cxa_function_proto, &name_info.m_short_name_context);
				else WriteObjectIntroEvent(&declarator_sym, clink_definition, compatible_func_object, cxa_function_header, &name_info.m_short_name_context);

				// Return already existing data field to the caller.
				return(compatible_func_object);
			}

			// Discovered object is not compatible. Create new object with an alt name.
			if (name_info.SetupShortNameDefnProblem(this, cdpr_conflict) == FALSE)
				return(NULL);
		}
	}

	// The new  code entry point or the func template object should be created. Note that this can be either
	// a "correct" object with the name from the source code or a "problematic" object with a generated name.
	TCppCodeEntryPointDefn *new_function = CreateFunctionPrototype(dest_defns_layer, func_attrs, name_info, data_type, unapplied_func_modifier);
	if (new_function == NULL)
		return(NULL);

	if (dest_defns_layer != m_defns_layer_owner)
		PublishOnTheSpecificLayer(dest_defns_layer, new_function);

	if (want_friend_proxy == TRUE)
	{
		TCppErrorReportingCtxSymbolStateItem error_context(name_info);
		CreateAndPublishFriendRecord(&error_context, new_function);
	}

	if (func_header_ctx == FALSE)
		WriteObjectIntroEvent(&declarator_sym, clink_fwd_decl, new_function, cxa_function_proto, &name_info.m_short_name_context);
	else WriteObjectIntroEvent(&declarator_sym, clink_definition, new_function, cxa_function_header, &name_info.m_short_name_context);

	// Success.
	return(new_function);
}

TCppDefnBase *TCppParser::DoHandleTypelessFuncDecl(bool want_friend_proxy, TCppDefnBase *dest_defns_layer, TCppSymbolStateItem &declarator_sym, bool func_header_ctx)
{
	assert(declarator_sym.m_meaning == csm_declarator);
	TCppNameInfo &name_info = declarator_sym.m_declarator_info.m_name_info;
	TCppNameInfoType cnit = name_info.m_core.m_info_type;
	wchar_t buff96[96];

	// Pick up the function attrs from the decl type spec if it is present.
	TCppFunctionAttr func_attrs = fcta_none;
	if (m_curr_decl_type_spec != NULL)
	{
		// The decl type specifier is present.
		TCppDeclTypeSpecInfo &dts = m_curr_decl_type_spec->m_decl_type_spec_info;

		// Check the decl type spec prefix, func attrs and the storage class if any.
		func_attrs = CheckFunctionDeclTypeSpec(dest_defns_layer, declarator_sym, func_header_ctx);

		if (dts.m_dclt_cv_spec.cvpr_const == TRUE)
		{
			// Bogus const specifier is present.
			ReportSyntaxError(m_curr_decl_type_spec, cerr_typeless_func_bad_const_spec, name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
		}

		if (dts.m_dclt_cv_spec.cvpr_volatile == TRUE)
		{
			// Bogus volatile specifier is present.
			ReportSyntaxError(m_curr_decl_type_spec, cerr_typeless_func_bad_volatile_spec, name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
		}

		if (dts.IsBaseTypeSpecPresent() == TRUE)
		{
			// Bogus base type is present.
			ReportSyntaxError(m_curr_decl_type_spec, cerr_typeless_func_bad_base_type, name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
		}
	}

	// Pick up the func params from the declarator.
	TCppDeclaratorModifier *func_params_mdf = NULL;
	AssembleTypelessDeclarator(declarator_sym, func_params_mdf);

	if (func_params_mdf != NULL)
	{
		// The list of function params is present.
		if (cnit == cnit_constructor || cnit == cnit_ctor_template)
		{
			if (func_params_mdf->m_cvpr_spec.cvpr_const == TRUE)
			{
				TCppErrorReportingCtxSymbolStateItem error_context(func_params_mdf->m_func_spec.m_this_cv_spec_context);
				ReportSyntaxError(&error_context, cerr_typeless_func_ctor_this_spec, L"const");
				func_params_mdf->m_cvpr_spec.cvpr_const = FALSE;
			}

			if (func_params_mdf->m_cvpr_spec.cvpr_volatile == TRUE)
			{
				TCppErrorReportingCtxSymbolStateItem error_context(func_params_mdf->m_func_spec.m_this_cv_spec_context);
				ReportSyntaxError(&error_context, cerr_typeless_func_ctor_this_spec, L"volatile");
				func_params_mdf->m_cvpr_spec.cvpr_volatile = FALSE;
			}
		}
		else if (cnit == cnit_dector)
		{
			if (func_params_mdf->m_cvpr_spec.cvpr_const == TRUE)
			{
				TCppErrorReportingCtxSymbolStateItem error_context(func_params_mdf->m_func_spec.m_this_cv_spec_context);
				ReportSyntaxError(&error_context, cerr_typeless_func_dector_this_spec, L"const");
				func_params_mdf->m_cvpr_spec.cvpr_const = FALSE;
			}

			if (func_params_mdf->m_cvpr_spec.cvpr_volatile == TRUE)
			{
				TCppErrorReportingCtxSymbolStateItem error_context(func_params_mdf->m_func_spec.m_this_cv_spec_context);
				ReportSyntaxError(&error_context, cerr_typeless_func_dector_this_spec, L"volatile");
				func_params_mdf->m_cvpr_spec.cvpr_volatile = FALSE;
			}

			if (func_params_mdf->m_func_spec.GetNumParams() > 0)
			{
				TCppErrorReportingCtxSymbolStateItem error_context(func_params_mdf->m_modifier_context);
				ReportSyntaxError(&error_context, cerr_typeless_func_dector_bad_params);
				func_params_mdf = NULL;
			}
		}
		else
		{
			assert(cnit == cnit_ovld_conversion || cnit == cnit_ovld_conv_template);
			if (func_params_mdf->m_func_spec.GetNumParams() > 0)
			{
				TCppErrorReportingCtxSymbolStateItem error_context(func_params_mdf->m_modifier_context);
				ReportSyntaxError(&error_context, cerr_typeless_func_oconv_bad_params);
				func_params_mdf = NULL;
			}
		}
	}

	// Check for special case where template args are straight repetition of params from the template header.
	name_info.CheckRemoveTemplateDefnTemplateArgs(this);

	// For now process only functions and function templates.
	if (name_info.TemplateArgsPresent() == TRUE)
	{
		if (func_header_ctx == FALSE && name_info.TemplateHeaderParams() == NULL)
		{
			RaiseNotImplementedError(L"TypelessFunctionTemplateExplicitInstantiation");
			return(NULL);
		}
		else
		{
			RaiseNotImplementedError(L"TypelessFunctionTemplateSpecialization");
			return(NULL);
		}
	}

	if (name_info.m_unproc_template_header != NULL && cnit == cnit_dector)
	{
		// Templates of destructors are not allowed.
		ReportUnexpectedTemplateHeaders(L"the destructor", name_info.m_unproc_template_header);
		name_info.m_unproc_template_header = NULL;
	}

	if (name_info.m_unproc_template_header != NULL)
	{
		// The special function template is being defined.
		name_info.m_unproc_template_header->m_template_info.m_header_processed = TRUE;

		if (name_info.m_unproc_template_header->m_template_info.m_child_template_header != NULL)
		{
			// Current template header is not the final header in the sequence of template headers.
			ReportUnprocessedTemplateHeaders(name_info.m_unproc_template_header->m_template_info.m_child_template_header);
		}

		if (name_info.TemplateHeaderParams() == NULL || name_info.TemplateHeaderParams()->IsEmpty() == TRUE)
		{
			// The template keyword is present while the template parameters are missing.
			TCppErrorReportingCtxSymbolStateItem error_context(name_info);
			ReportSyntaxError(&error_context, cerr_typeless_func_no_templ_params, name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
		}
	}

	if (name_info.IsUnresolvedLayer() == TRUE)
	{
		// Lookup is not needed. Just prepare name for creating the function object.
		if (name_info.FinalizeLayerResProblemName(this) == FALSE)
			return(NULL);
	}
	else
	{
		// Verify location of the definition and destination layer against the type of the name. Note that names
		// of the ctors are always fine becase type of the name is changed from identifier to constructor only when
		// the dest layer is struct and when name of the function matches the name of the layer.
		TCppDefnProblem name_or_loc_problem = cdpr_none;
		if (cnit == cnit_dector)
		{
			// Name in the declarator contains a tilda in front of it. Check the type of the target layer.
			// Check for the block location first, becase passed dest layer cannot have a block type.
			if (name_info.GetDefnsLayer(m_defns_layer_owner)->IsBlock() == TRUE)
			{
				// Note that condition above checks the simple dest layer and not the passed dest layer.
				assert(name_info.IsQualified() == FALSE && func_header_ctx == FALSE);
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&declarator_sym, cerr_typeless_func_dector_in_block);
				name_or_loc_problem = cdpr_errloc;
			}
			else if (dest_defns_layer->IsNamespace() == TRUE)
			{
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&declarator_sym, cerr_typeless_func_dector_on_namesp);
				name_or_loc_problem = cdpr_errloc;
			}
			else if (dest_defns_layer->DefnKeyId() != name_info.KeyId())
			{
				// The name in the declarator does not match the name of the layer.
				assert(dest_defns_layer->IsStructType() == TRUE);
				wchar_t loc_buff_80[80];
				ReportSyntaxError(&declarator_sym, cerr_typeless_func_dector_bad_name, name_info.m_cached_name, dest_defns_layer->GetObjectTypeFriendlyName(loc_buff_80));
				name_or_loc_problem = cdpr_unapprop;
			}
		}
		else if (cnit == cnit_ovld_conversion || cnit == cnit_ovld_conv_template)
		{
			// This is overloaded conversion. These objects are allowed only in classes.
			// Check for the block location first, becase passed dest layer cannot have a block type.
			if (name_info.GetDefnsLayer(m_defns_layer_owner)->IsBlock() == TRUE)
			{
				// Note that condition above checks the simple dest layer and not the passed dest layer.
				assert(name_info.IsQualified() == FALSE && func_header_ctx == FALSE);
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&declarator_sym, cerr_typeless_func_oconv_in_block);
				name_or_loc_problem = cdpr_errloc;
			}
			if (dest_defns_layer->IsNamespace() == TRUE)
			{
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&declarator_sym, cerr_typeless_func_oconv_on_namesp);
				name_or_loc_problem = cdpr_errloc;
			}
		}

		if (name_or_loc_problem != cdpr_none)
		{
			// Integrate discovered problem into the name info.
			if (name_info.SetupShortNameDefnProblem(this, name_or_loc_problem) == FALSE)
				return(NULL);
		}
	}

	if (name_info.m_name_problem == cdpr_none)
	{
		// Veryfy destination layer and search for existing objects on that layer.
		TCppCodeEntryPointDefn *compatible_spec_func_object = NULL;
		if (CheckLookupFucntionObject(want_friend_proxy, dest_defns_layer, name_info, func_params_mdf, compatible_spec_func_object, func_header_ctx) == FALSE)
			return(NULL);

		if (compatible_spec_func_object != NULL)
		{
			// Compare additional props.
			if (CheckExistingFunctionProps(dest_defns_layer, func_attrs, name_info, func_params_mdf, compatible_spec_func_object, func_header_ctx) == TRUE)
			{
				if (want_friend_proxy == TRUE)
				{
					TCppErrorReportingCtxSymbolStateItem error_context(name_info);
					CreateAndPublishFriendRecord(&error_context, compatible_spec_func_object);
				}

				if (func_header_ctx == FALSE)
					WriteObjectIntroEvent(&declarator_sym, clink_fwd_decl, compatible_spec_func_object, cxa_function_proto, &name_info.m_short_name_context);
				else WriteObjectIntroEvent(&declarator_sym, clink_definition, compatible_spec_func_object, cxa_function_header, &name_info.m_short_name_context);

				// Return already existing data field to the caller.
				return(compatible_spec_func_object);
			}

			// Discovered object is not compatible. Create new object with an alt name.
			if (name_info.SetupShortNameDefnProblem(this, cdpr_conflict) == FALSE)
				return(NULL);
		}
	}

	// The new  code entry point or the func template object should be created. Note that this can be either
	// a "correct" object with the name from the source code or a "problematic" object with a generated name.
	TCppCodeEntryPointDefn *new_spec_func = CreateFunctionPrototype(dest_defns_layer, func_attrs, name_info, NULL, func_params_mdf);
	if (new_spec_func == NULL)
		return(NULL);

	if (want_friend_proxy == TRUE)
	{
		TCppErrorReportingCtxSymbolStateItem error_context(name_info);
		CreateAndPublishFriendRecord(&error_context, new_spec_func);
	}

	if (dest_defns_layer != m_defns_layer_owner)
		PublishOnTheSpecificLayer(dest_defns_layer, new_spec_func);

	if (func_header_ctx == FALSE)
		WriteObjectIntroEvent(&declarator_sym, clink_fwd_decl, new_spec_func, cxa_function_proto, &name_info.m_short_name_context);
	else WriteObjectIntroEvent(&declarator_sym, clink_definition, new_spec_func, cxa_function_header, &name_info.m_short_name_context);

	// Success.
	return(new_spec_func);
}

void TCppParser::HandleSimpOrQualNameSymbols(TCppSymbolStateItem &new_non_term, TCppSymbolStateItem &first_rule_sym, int rule_len)
{
	//
	// Possible rule forms:
	//
	//		AllowedFormOfTheFinalName
	//	 '::'	AllowedFormOfTheFinalName
	//
	//	SimpleOrQualifiedId   '::'			AllowedFormOfTheFinalName
	//	SimpleOrQualifiedId   '::'   TEMPLATE	AllowedFormOfTheFinalName
	//

	if (rule_len == 1)
	{
		// Promote existing meaning of the first symbol up.
		assert(first_rule_sym.m_meaning == csm_simp_or_qual);
		new_non_term.m_meaning = csm_simp_or_qual;
		new_non_term.m_simp_or_qual = first_rule_sym.m_simp_or_qual;
		first_rule_sym.ResetMeaning();
	}
	else
	{
		TCppSymbolStateItem &second_sym = NextRuleSym(first_rule_sym);
		if (rule_len == 2)
		{
			// This is adding the global namespace prefix to one of the final forms of the name object.
			assert(second_sym.m_meaning == csm_simp_or_qual && second_sym.m_simp_or_qual.IsQualified() == FALSE);

			// Move existing meaning of the second symbol to the non terminal and set the global namespace as
			// the resolved defns layer. This action cannot fail. The global namespace is always available.
			new_non_term.m_meaning = csm_simp_or_qual;
			new_non_term.m_simp_or_qual = second_sym.m_simp_or_qual;
			new_non_term.m_simp_or_qual.m_resolved_defns_layer = &m_cpp_database.GetGlobalNamespace();
			second_sym.ResetMeaning();
		}
		else
		{
			// It is necessary to convert contents of the frist rule symbol into the description of the nesting layer.
			// First symbol can be either a simple identifier or a template definition/specialization/instantiation.
			assert(first_rule_sym.m_meaning == csm_simp_or_qual);

			TCppNameInfo &layer_name_info = first_rule_sym.m_simp_or_qual;
			assert(layer_name_info.IsIdentifierOrTempalteId() == TRUE);

			if (layer_name_info.IsUnresolvedLayer() == TRUE)
			{
				// Layer resolution has already failed. There is no need to do the name lookup.
				assert(layer_name_info.m_name_problem != cdpr_none);
				TCppUnresolvedNameInfo *unres_name_info = CreateUnresNameInfo(layer_name_info);
				if (unres_name_info == NULL)
					return;

				// Add name info to the existing list. Other props should simply keep their values.
				layer_name_info.AddUnresolvedLayerInfo(unres_name_info);
			}
			else
			{
				// The name of the layer is short or the current layer is fully resolved.
				assert(layer_name_info.m_name_problem == cdpr_none);
				TCppDefnBase *new_resolved_layer = NULL;
				TCppDefnProblem layer_resolution_problem = cdpr_none;

				// Make the lookup.
				TCppExistingDefnsSearchHelper hlpr(layer_name_info.GetDefnsLayer(m_defns_layer_owner));
				if (hlpr.DoSearch(elab_none, layer_name_info.m_core, layer_name_info.GetSearchHierFlag()) == TRUE)
				{
					// Something is present.
					if (hlpr.IsAmbigResult() == TRUE)
					{
						// Existing name is ambiguous.
						TCppErrorReportingCtxSymbolStateItem err_ctx(layer_name_info);
						ID main_error_id = ReportSyntaxError(&err_ctx, cerr_defns_layer_name_ambig, hlpr.m_ambig_defns.NumItems());
						ReportAmbiguousDefnsArray(&err_ctx, main_error_id, hlpr.m_ambig_defns);
						layer_resolution_problem = cdpr_ambig;
					}
					else if (hlpr.m_single_defn->ItemType() == cxx_namespace || hlpr.m_single_defn->ItemType() == cxx_struct_type)
					{
						// Pick up the discovered layer definition object.
						new_resolved_layer = hlpr.m_single_defn;
						if (layer_name_info.IsTempalteId() == TRUE)
						{
							// Report an error on the bogus template arguments and ignore them.
							TCppErrorReportingCtxSymbolStateItem err_ctx(layer_name_info);
							const wchar_t *error_param = (hlpr.m_single_defn->IsNamespace() == TRUE) ? L"namespace" : hlpr.m_single_defn->GetElabTypeName();
							ReportSyntaxError(&err_ctx, cerr_defns_layer_bad_template_args, error_param);
						}
					}
					else if (hlpr.m_single_defn->IsClassTemplateFamily() == TRUE)
					{
						// Check for special case where template args are straight repetition of params of the curr template header.
						layer_name_info.CheckRemoveTemplateDefnTemplateArgs(this);

						if (layer_name_info.IsIdentifier() == TRUE)
						{
							if (layer_name_info.m_unproc_template_header == NULL)
							{
								// The name of the class template is used without a template header in front of it. At the same
								// time the template args are missing. It is not possible to resolve the class template overload.
								TCppErrorReportingCtxSymbolStateItem err_ctx(layer_name_info);
								ReportSyntaxError(&err_ctx, cerr_defns_layer_bad_template1);
								layer_resolution_problem = cdpr_unapprop;
							}
							else
							{
								// The template header is available. Look for an appropriate class template overload.
								TCppClassTemplateDefn *class_template =  hlpr.FindClassTemplateOverload(layer_name_info.KeyId(), layer_name_info.TemplateHeaderParams());
								if (class_template != NULL)
								{
									// Use the resolved class template as a new definitions layer.
									new_resolved_layer = class_template;
								}
								else
								{
									// Requested overload is not available. This is an unresolved layer.
									TCppErrorReportingCtxSymbolStateItem err_ctx(layer_name_info);
									ReportSyntaxError(&err_ctx, cerr_defns_layer_bad_template2);
									layer_resolution_problem = cdpr_unres;
								}

								// Consume the current template header.
								layer_name_info.m_unproc_template_header->m_template_info.m_header_processed = TRUE;
								layer_name_info.m_unproc_template_header = layer_name_info.m_unproc_template_header->m_template_info.m_child_template_header;
							}
						}
						else
						{
							// This is request for the template instantiation. Note that template headers pointer is not shifted.
							RaiseNotImplementedError(L"Class template instantiation");
							return;
						}
					}
					else
					{
						// Discovered object cannot be used as definitions layer.
						TCppErrorReportingCtxSymbolStateItem err_ctx(layer_name_info);
						ReportSyntaxError(&err_ctx, cerr_defns_layer_name_unuseable, layer_name_info.m_cached_name);
						layer_resolution_problem = cdpr_unapprop;
					}
				}
				else
				{
					// The requested name is not known at all.
					TCppErrorReportingCtxSymbolStateItem err_ctx(layer_name_info);
					ReportSyntaxError(&err_ctx, cerr_defns_layer_name_unknown);
					layer_resolution_problem = cdpr_unres;
				}

				// Check results of the layer resolution.
				if (new_resolved_layer != NULL)
				{
					// The layer was resolved. Set this layer into the layer info struct.
					assert(layer_resolution_problem == cdpr_none);
					layer_name_info.m_resolved_defns_layer = new_resolved_layer;
				}
				else
				{
					// The name of the layer was not resolved. Start building list of unresolved name infos.
					assert(layer_resolution_problem != cdpr_none);

					TList *unresolved_names_list = AllocateGeneralPurposeList();
					if (unresolved_names_list == NULL)
						return;

					TCppUnresolvedNameInfo *unres_name_info = CreateUnresNameInfo(layer_name_info);
					if (unres_name_info == NULL)
					{
						delete unresolved_names_list;
						return;
					}

					// Asssemble the data structure.
					unresolved_names_list->AppendItem(unres_name_info);
					layer_name_info.SetupLayerResolutionProblem(unresolved_names_list, layer_resolution_problem);
				}
			}

			// Pick up the the tip object symbol.
			TCppSymbolStateItem *sym_unqual = NextRuleSymPtr(&second_sym);
			if (sym_unqual->m_app_id == sym_template)
			{
				// Shift to the next symbol.
				sym_unqual = NextRuleSymPtr(sym_unqual);
				assert(sym_unqual != NULL);
			}

			// Ensure that discovered tip symbol contains the name info and that it is unqualified.
			assert(sym_unqual->m_meaning == csm_simp_or_qual && sym_unqual->m_simp_or_qual.IsQualified() == FALSE);

			// Use the tip name info as the base for preparing the non terminal.
			new_non_term.m_meaning = csm_simp_or_qual;
			new_non_term.m_simp_or_qual = sym_unqual->m_simp_or_qual;

			// Fill in the updated layer info.
			new_non_term.m_simp_or_qual.m_name_problem = layer_name_info.m_name_problem;
			new_non_term.m_simp_or_qual.m_unproc_template_header = layer_name_info.m_unproc_template_header;
			new_non_term.m_simp_or_qual.m_resolved_defns_layer = layer_name_info.m_resolved_defns_layer;
			new_non_term.m_simp_or_qual.m_unresolved_names_list = layer_name_info.m_unresolved_names_list;

			layer_name_info.m_unresolved_names_list = NULL;
			layer_name_info.ReleaseResources(this);
			first_rule_sym.ResetMeaning();
			sym_unqual->ResetMeaning();
		}

		// Pick up the area that covers all symbols of the current rule. Short area in the new non term
		// is already fine. It was copied from the tip name info.
		new_non_term.m_simp_or_qual.m_qual_name_context = new_non_term.GetOriginHeader();
	}
}

void TCppParser::HandleTypeSpecifierSymbols(TCppSymbolStateItem &new_non_term, WORD irule, int rule_len)
{
	new_non_term.m_meaning = csm_decl_type_spec;
	TCppSymbolStateItem *sym = FirstRuleSymPtr(rule_len);
	int isym = 0;

	if (rule_len == 1)
	{
		// Start building the new decl type specifier symbol.
		new_non_term.m_decl_type_spec_info.Clear();
	}
	else
	{
		// Inherit contents of the already existing decl type spec symbol.
		assert(sym->m_meaning = csm_decl_type_spec);
		new_non_term.m_decl_type_spec_info = sym->m_decl_type_spec_info;
		sym->ResetMeaning();

		// Shift to the next symbol.
		sym = NextRuleSymPtr(sym);
		assert(sym != NULL);
		isym++;
	}

	// Apply new specifier to an already existing structure.
	TCppDeclTypeSpecInfo &dti = new_non_term.m_decl_type_spec_info;
	switch (sym->m_meaning)
	{
		case csm_decl_prefix:
				{
					if (dti.m_dclt_prefix != declpr_none)
						ReportSyntaxError(sym, cerr_dts_decl_prefix_dupe, TCppDatabase::GetDeclTypeSpecPrefixFriendlyName(dti.m_dclt_prefix));
					else dti.m_dclt_prefix = sym->m_decl_prefix;
				}
				break;

		case csm_function_attr:
				{
					if ((dti.m_dclt_func_attrs & sym->m_function_attr) != 0)
						ReportSyntaxError(sym, cerr_dts_func_attr_dupe, TCppDatabase::GetFunctionAttributeFriendlyName(sym->m_function_attr));
					else dti.m_dclt_func_attrs = (TCppFunctionAttr)(dti.m_dclt_func_attrs | sym->m_function_attr);
				}
				break;

		case csm_storage_class:
				{
					if (dti.m_dclt_strg_class != strgc_none)
						ReportSyntaxError(sym, cerr_dts_strg_class_dupe, TCppDataFieldDefn::GetStorageClassFriendlyName(dti.m_dclt_strg_class));
					else dti.m_dclt_strg_class = sym->m_storage_class;
				}
				break;

		case csm_none:
				{
					// This should be either CONST or VOLATILE terminal symbols.
					switch (sym->m_app_id)
					{
						case sym_const:
								{
									if (dti.m_dclt_cv_spec.cvpr_const == TRUE)
										ReportSyntaxError(sym, cerr_dts_const_spec_dupe);
									else dti.m_dclt_cv_spec.cvpr_const = TRUE;
								}
								break;

						case sym_volatile:
								{
									if (dti.m_dclt_cv_spec.cvpr_volatile == TRUE)
										ReportSyntaxError(sym, cerr_dts_volatile_spec_dupe);
									else dti.m_dclt_cv_spec.cvpr_volatile = TRUE;
								}
								break;

						default:
							// Issue an assert and ignore this symbol.
							assert(FALSE);
							break;
					}
				}
				break;

		case csm_built_in_type_spec:
				{
					// Built in type specifiers should be accepted only when there is no other base type info.
					if (dti.m_dclt_unres_name_info.IsEmpty() == FALSE || dti.m_dclt_base_type != NULL)
					{
						ReportSyntaxError(sym, cerr_dts_bad_built_in_spec);
					}
					else
					{
						// Apply the new built in type spec.
						TCppErrorCode error_code = dti.m_dclt_blt_in_info.AddBuiltInTypeSpec(sym->m_bltin_spec_app_id);
						if (error_code != 0)
						{
							// Report this error but continue building the type. Any non empty sequence of built in
							// type specs is mapped into some valid built in type.
							ReportSyntaxError(sym, error_code);
						}
					}
				}
				break;

		case csm_decl_type_spec:
				{
					// Ensure that there is no other base type info.
					if (dti.IsBaseTypeSpecPresent() == TRUE)
					{
						if (sym->m_decl_type_spec_info.m_dclt_unres_name_elab == elab_none)
							ReportSyntaxError(sym, cerr_dts_bad_named_type);
						else ReportSyntaxError(sym, cerr_dts_bad_elab_type_spec);
					}
					else
					{
						// Accept the type info. Note that  the accepted name info is not resolved yet.
						dti.m_dclt_unres_name_elab = sym->m_decl_type_spec_info.m_dclt_unres_name_elab;
						dti.m_dclt_unres_name_info = sym->m_decl_type_spec_info.m_dclt_unres_name_info;
						sym->ResetMeaning();
					}
				}
				break;

		case csm_data_type:
				{
					// This is definition of either an enum or a class. Ensure that there is no other base type info.
					assert(sym->m_data_type != NULL);
					if (dti.IsBaseTypeSpecPresent() == TRUE)
					{
						wchar_t buff80[80];
						ReportSyntaxError(sym, cerr_dts_bad_enum_or_class, sym->m_data_type->GetObjectTypeFriendlyName(buff80, TRUE));
					}
					else
					{
						// Accept the type info. Set the mark that states that enum or class (class template/ class template spec) was defined locally.
						dti.m_dclt_base_type = sym->m_data_type;
						dti.m_dclt_local_enum_class = TRUE;
					}
				}
				break;

		default:
			// This is some problem in the grammar.
			RaiseUnexpectedRuleSymbolMeaningError(irule, isym);
			break;
	}
}

void TCppParser::HandlePtrOperatorDeclaratorSymbols(TCppSymbolStateItem &new_non_term, TCppSymbolStateItem &first_rule_sym)
{
	// Add one more layer of Ptr or Ref indirection. Rule should contain 2 symbols.
	TCppSymbolStateItem &second_rule_sym = NextRuleSym(first_rule_sym);
	assert(first_rule_sym.m_meaning == csm_ptr_operator && second_rule_sym.m_meaning == csm_declarator);

	// Move existing declarator from the second symbol to the non terminal.
	new_non_term.m_meaning = csm_declarator;
	new_non_term.m_declarator_info = second_rule_sym.m_declarator_info;
	second_rule_sym.ResetMeaning();

	// Apply the modifier. This operation may fail. In this case the function will simply do notihng.
	ApplyCvPtrRefModifier(&second_rule_sym, new_non_term.m_declarator_info, first_rule_sym.m_ptr_operator);
	first_rule_sym.ResetMeaning();
}

void TCppParser::HandleFuncParamsDeclaratorSymbols(TCppSymbolStateItem &new_non_term, WORD irule, TCppSymbolStateItem *sym_params_decl_clause)
{
	//
	// Possible rule forms:
	//
	//	ParameterDeclarationClause
	//	ParameterDeclarationClause				  ExceptionsSpecification
	//	ParameterDeclarationClause    CvQualifier
	//	ParameterDeclarationClause    CvQualifier	  ExceptionsSpecification
	//

	// The passed symbol should have a fixed meaning. Pick up its data.
	assert(sym_params_decl_clause != NULL && sym_params_decl_clause->m_meaning == csm_func_params_list);
	TList *params_list = sym_params_decl_clause->m_func_params_info.m_params_list;
	sym_params_decl_clause->ResetMeaning();

	// Check/pick up the optional symbols.
	TCppSymbolStateItem *cv_qual_sym = NULL;
	TList *exceptions_spec = NULL;
	TCppSymbolStateItem *sym = NextRuleSymPtr(sym_params_decl_clause);
	int isym = 0;
	while (sym != NULL)
	{
		switch (sym->m_meaning)
		{
			case csm_cv_qualifier:		assert(cv_qual_sym == NULL);
									cv_qual_sym = sym;
									break;
			case csm_exceptions_list:	assert(exceptions_spec == NULL);
									exceptions_spec = sym->m_exceptions_list;
									sym->ResetMeaning();
									break;
			default:
				// This is some problem in the grammar.
				RaiseUnexpectedRuleSymbolMeaningError(irule, isym);
				break;
		}

		sym = NextRuleSymPtr(sym);
		isym++;
	}

	// Apply the modifier. This operation may fail. In this case the function will simply do notihng.
	assert(new_non_term.m_meaning == csm_declarator);
	ApplyFuncParamsModifier(sym_params_decl_clause, new_non_term.m_declarator_info, params_list, cv_qual_sym, exceptions_spec);

	// Release the params lists and the list of exceptions if any.
	if (params_list != NULL)
		RecycleGeneralPurposeList(params_list);
	if (exceptions_spec != NULL)
		RecycleGeneralPurposeList(exceptions_spec);
}

void TCppParser::HandleGenericTypeIdSymbols(TCppSymbolStateItem &new_non_term, TCppSymbolStateItem &first_rule_sym, int rule_len)
{
	// This function expects that passed symbol is the TypeSpecifier, not the DeclTypeSpecifier.
	assert(first_rule_sym.m_meaning == csm_decl_type_spec);
	assert(first_rule_sym.m_decl_type_spec_info.m_dclt_prefix == declpr_none);

	TCppDataTypeBase *data_type;
	if (rule_len == 1)
	{
		// Only the type specifier is available.
		data_type = CheckPrepareDataTypeFromTypeSpecifier(first_rule_sym);
	}
	else
	{
		// Finalize building the type specifier.
		if (CheckPrepareTypeSpecifier(first_rule_sym) == FALSE)
			return;

		// Combine the type specifier and the declarator.
		TCppDeclTypeSpecInfo &dts = first_rule_sym.m_decl_type_spec_info;
		data_type = (TCppDataTypeBase*)AssembleDeclaratorType(&first_rule_sym, FALSE, dts.m_dclt_cv_spec, dts.m_dclt_base_type, NextRuleSym(first_rule_sym));
	}

	assert(data_type == NULL || data_type->IsDataType() == TRUE);
	if (data_type == NULL)
		return;

	// Prepare the non terminal.
	new_non_term.m_meaning = csm_data_type;
	new_non_term.m_data_type = data_type;
}

TCppExpressionBase *TCppParser::ExprHandlePostfixUnaryOperation(TCppSymbolStateItem &first_rule_sym, TOperatorToken operation)
{
	//
	// Possible rule forms:
	//
	//	PostfixExpression  '++'
	//	PostfixExpression  '--'
	//
	assert(first_rule_sym.m_meaning == csm_expression && first_rule_sym.m_expression != NULL);
	assert(operation == opr_plpl_postfix || operation == opr_mnmn_postfix);

	// Generate an unary operation.
	TCppUnaryExpr *new_expr_node = (TCppUnaryExpr*)AllocateNewCppItem(cxx_unary_expr);
	if (new_expr_node == NULL)
		return(NULL);

	// Fill in the props of the new node.
	new_expr_node->SetOperation(operation);
	new_expr_node->AddChildItem(first_rule_sym.m_expression, cexpr_chdest_unary_oprnd);
	first_rule_sym.ResetMeaning();

	// Give the prepared expression out.
	return(new_expr_node);
}

TCppExpressionBase *TCppParser::ExprHandleDotArrowOperation(TCppSymbolStateItem &first_rule_sym, TOperatorToken operation)
{
	//
	// Possible rule forms:
	//
	//	PostfixExpression	'.' ExpressionId
	//	PostfixExpression	'.' TEMPLATE  ExpressionId
	//	PostfixExpression  '->' ExpressionId
	//	PostfixExpression  '->' TEMPLATE  ExpressionId
	//
	assert(first_rule_sym.m_meaning == csm_expression && first_rule_sym.m_expression != NULL);
	assert(operation == opr_dot || operation == opr_arrow);

	// Generate the field access operation.
	TCppFieldAccessExpr *new_expr_node = (TCppFieldAccessExpr*)AllocateNewCppItem(cxx_field_access_expr);
	if (new_expr_node == NULL)
		return(NULL);

	// Fill in the first operand.
	assert(first_rule_sym.m_meaning == csm_expression);
	new_expr_node->SetOperation(operation);
	new_expr_node->AddChildItem(first_rule_sym.m_expression, cexpr_chdest_fld_acc_oprnd);
	first_rule_sym.ResetMeaning();

	// Pick up the the structured type of the first operand if possible.
	TCppDefnBase *struct_object = NULL;
	if (new_expr_node->m_struct_operand->m_result_type == NULL)
	{
		// The type of the first operand is missing.
	}
	else if (operation == opr_dot)
	{
		// The type of the operand should be the struct type or a ref to struct.
		if (new_expr_node->m_struct_operand->m_result_type->IsStructType() == TRUE)
		{
			// Accept the type.
			struct_object = new_expr_node->m_struct_operand->m_result_type;
		}
		else if (new_expr_node->m_struct_operand->m_result_type->ItemType() == cxx_cv_ptr_ref_type)
		{
			TCppCvPtrRefTypeDefn *cvpr_type = (TCppCvPtrRefTypeDefn*)new_expr_node->m_struct_operand->m_result_type;
			if (cvpr_type->m_layers.NumItems() == 1 && cvpr_type->m_layers[0].cvpr_reference == TRUE)
			{
				// The type of the first operand is a reference. Check its base type.
				if (cvpr_type->m_base_type->IsStructType() == TRUE)
				{
					// Accept the base type.
					struct_object = cvpr_type->m_base_type;
				}
			}
		}

		if (struct_object == NULL)
		{
			// The type is not a struct and not a reference to struct. This is something different.
		}
	}
	else
	{
		// The type should be a pointer to struct.
		assert(operation == opr_arrow);
		if (new_expr_node->m_struct_operand->m_result_type->ItemType() == cxx_cv_ptr_ref_type)
		{
			TCppCvPtrRefTypeDefn *cvpr_type = (TCppCvPtrRefTypeDefn*)new_expr_node->m_struct_operand->m_result_type;
			if (cvpr_type->m_layers.NumItems() == 1 && cvpr_type->m_layers[0].cvpr_pointer == TRUE)
			{
				// The type of the first operand is a pointer. Check its base type.
				if (cvpr_type->m_base_type->IsStructType() == TRUE)
				{
					// Accept the base type.
					struct_object = cvpr_type->m_base_type;
				}
			}
		}

		if (struct_object == NULL)
		{
			// The type is not a pointer to struct. This is something different.
		}
	}

	// Ignore the TEMPLATE keyword.
	TCppSymbolStateItem *exprid_sym = NextRuleSymPtr(NextRuleSymPtr(&first_rule_sym));
	if (exprid_sym->m_app_id == sym_template)
	{
		// Shift to the next symbol.
		exprid_sym = NextRuleSymPtr(exprid_sym);
		assert(exprid_sym != NULL);
	}

	// Check the ExpressionId symbol.
	assert(exprid_sym != NULL && exprid_sym->m_meaning == csm_simp_or_qual);
	TCppNameInfo &struct_field_name_info = exprid_sym->m_simp_or_qual;
	if (struct_field_name_info.TemplateArgsPresent() == TRUE)
	{
	}

	// Try to resolve the field of the structure.
	if (struct_object != NULL)
	{
		// The search space is available. Check the data field symbol.
		if (struct_field_name_info.IsUnresolvedLayer() == FALSE)
		{
			if (struct_field_name_info.m_resolved_defns_layer != NULL && struct_field_name_info.m_resolved_defns_layer != struct_object)
			{
			}
			else
			{
				// Setup the search space directly. This will also block the hierarchy search.
				struct_field_name_info.m_resolved_defns_layer = struct_object;

				// Do the search itself.
				TCppDefnBase *operand_defn = NULL;
				TCppDefnsBucket *operand_defns_bucket = NULL;
				if (CheckLookupExistingDefinition(exprid_sym, elab_none, struct_field_name_info, sym_PostfixExpression, operand_defn, &operand_defns_bucket) == FALSE)
					return(NULL);

				// Check results of the search.
				assert(operand_defn != NULL);
				TCppItemType obj_type = operand_defn->ItemType();
				if (obj_type == cxx_enum_member)
				{
					TCppEnumMemberDefn *enum_member  = (TCppEnumMemberDefn*)operand_defn;
					TCppEnumTypeDefn *enum_type  = (TCppEnumTypeDefn*)(operand_defn->ItemParent());
					assert(enum_type->ItemType() == cxx_enum_type);

					// This object is read only.
					new_expr_node->m_const_expr = TRUE;

					new_expr_node->m_struct_member_defn = enum_member;
					new_expr_node->m_result_type = enum_type;
				}
				else if (obj_type == cxx_base_class_spec)
				{
					// This case is treated as typecasted "this" operand.
					TCppBaseClassSpec *base_class_spec = (TCppBaseClassSpec*)operand_defn;

					new_expr_node->m_struct_member_defn = base_class_spec;
					new_expr_node->m_result_type = base_class_spec->m_base_class;
				}
				else if (obj_type == cxx_data_field || obj_type == cxx_unresolved_data_field)
				{
					// Both objects are data fields.
					TCppDataFieldDefn *data_field = (TCppDataFieldDefn*)operand_defn;

					// Set the L-value flag ON.
					new_expr_node->m_lvalue_expr = TRUE;

					new_expr_node->m_struct_member_defn = data_field;
					new_expr_node->m_result_type = data_field->m_field_type;
				}
				else if (operand_defn->IsFunctionFamily() == TRUE)
				{
					// The whole bucket should be returned.
					assert(operand_defns_bucket != NULL);

					int num_overloads = TCppNewDefnsSearchHelper::GetNumberOfOverloads(struct_field_name_info.m_core, operand_defn, operand_defns_bucket);
					if (obj_type == cxx_code_entry_point && num_overloads == 1)
					{
						// -BUGBUG- There is only one function with this name. Use it.
						new_expr_node->m_struct_member_defn = operand_defn;
						new_expr_node->m_result_type = ((TCppCodeEntryPointDefn*)operand_defn)->m_return_value_type;
					}
				}
				else
				{
					// Unexpeccted object type. Issue an assert and return an unprepared operand.
					assert(FALSE);
				}
			}
		}
	}

	// Give the prepared expression out.
	return(new_expr_node);
}

TCppExpressionBase *TCppParser::ExprHandleArrayIndexOperation(TCppSymbolStateItem &first_rule_sym, int rule_len)
{
	//
	// Possible rule forms:
	//
	//	PostfixExpression '[' ExpressionsList ']'
	//

	// Symbols of the rule should be: expression, terminal, expression, terminal.
	TCppSymbolStateItem &s2 = NextRuleSym(first_rule_sym), &s3 = NextRuleSym(s2);
	assert(first_rule_sym.m_meaning == csm_expression && s3.m_meaning == csm_expression);

	// New expression object is needed.
	TCppArrayIndexExpr *new_expr_node = (TCppArrayIndexExpr*)AllocateNewCppItem(cxx_array_index_expr);
	if (new_expr_node == NULL)
		return(NULL);

	// Fill in the operands.
	new_expr_node->AddChildItem(first_rule_sym.m_expression, cexpr_chdest_index_array);
	first_rule_sym.ResetMeaning();

	// Assign the type of result.
	if (new_expr_node->m_array_operand->m_result_type != NULL)
	{
		if (new_expr_node->m_array_operand->m_result_type->ItemType() == cxx_cv_ptr_ref_type)
		{
			TCppCvPtrRefTypeDefn *cvpr_type = (TCppCvPtrRefTypeDefn*)new_expr_node->m_array_operand->m_result_type;
			if (cvpr_type->m_layers.NumItems() > 0 && cvpr_type->m_layers[cvpr_type->m_layers.NumItems()-1].IsSimplePointer() == TRUE)
			{
				// Final layer is a pointer layer. Pick up or create a data type that does not contain this layer.
				TCppDataTypeBase *res_type = CreateDereferencedType(&first_rule_sym, cvpr_type, cvpr_type->m_layers.NumItems()-1);
				if (res_type == NULL)
					return(NULL);

				// Lookup or creation succeded.
				new_expr_node->m_result_type = res_type;
			}
		}
		else if (new_expr_node->m_array_operand->m_result_type->ItemType() == cxx_array_type)
		{
			// Assume for now that all indexes are dereferenced.
			TCppArrayTypeDefn *array_type = (TCppArrayTypeDefn*)new_expr_node->m_array_operand->m_result_type;
			new_expr_node->m_result_type = array_type->m_base_type;
		}
	}

	if (s3.m_expression->m_operation == opr_comma)
	{
		// Expression in the symbol is the list of expressions. Do not reset the meaning because
		// only the operands are moved. The expr itself will be released by the recycle procedure.
		TCppMultiOpExpr &mop_expr = (TCppMultiOpExpr&)(*(s3.m_expression));
		new_expr_node->AddListOfChildren(mop_expr.m_operands);
	}
	else
	{
		// Symbol contains some other type of expression. Add it as one parameter.
		new_expr_node->AddChildItem(s3.m_expression, cexpr_chdest_index_params);
		s3.ResetMeaning();
	}

	// Give the prepared expression out.
	new_expr_node->m_lvalue_expr = TRUE;
	return(new_expr_node);
}

TCppExpressionBase *TCppParser::ExprHandleFunctionCallOperation(TCppSymbolStateItem &first_rule_sym, int rule_len)
{
	//
	// Possible rule forms:
	//
	//	PostfixExpression  '(' ')'
	//	PostfixExpression  '(' ExpressionsList ')'
	//

	// New function call object is needed.
	TCppFunctionCallExpr *new_expr_node = (TCppFunctionCallExpr*)AllocateNewCppItem(cxx_function_call_expr);
	if (new_expr_node == NULL)
		return(NULL);

	// The first symbol of the rule is the function to call.
	assert(first_rule_sym.m_meaning == csm_expression);
	new_expr_node->AddChildItem(first_rule_sym.m_expression, cexpr_chdest_fcall_func);
	first_rule_sym.ResetMeaning();

	if (rule_len > 3)
	{
		// Symbols of the rule should be: terminal "(", expressions list, terminal ")".
		TCppSymbolStateItem &s2 = NextRuleSym(first_rule_sym), &s3 = NextRuleSym(s2);
		new_expr_node->AddCallParameters(s3.m_expression);
		s3.ResetMeaning();
	}

	// Give the prepared expression out.
	return(new_expr_node);
}

TCppExpressionBase *TCppParser::ExprHandleCtorCallOperation(TCppSymbolStateItem &first_rule_sym, int rule_len)
{
	//
	// Possible rule forms:
	//
	//	SimpleTypeSpecifier  '(' ')'
	//	SimpleTypeSpecifier  '(' ExpressionsList ')'
	//	TYPENAME  SimpleOrQualifiedId  '(' ')'
	//	TYPENAME  SimpleOrQualifiedId  '(' ExpressionsList ')'
	//

	RaiseNotImplementedError(L"Call-Of-Constructor");
	return(NULL);
}

TCppExpressionBase *TCppParser::ExprHandleTypeCastOperation(TCppSymbolStateItem &first_rule_sym, TOperatorToken operation)
{
	//
	// Possible rule forms:
	//
	//	CONST_CAST	 '<' TypeId '>'  '(' Expression ')'
	//	STATIC_CAST  '<' TypeId '>'  '(' Expression ')'
	//	DYNA_CAST	 '<' TypeId '>'  '(' Expression ')'
	//	REIN_CAST	 '<' TypeId '>'  '(' Expression ')'
	//

	// Symbols of the rule should be: terminal, terminal, data type, terminal, expression, terminal.
	TCppSymbolStateItem &s2 = NextRuleSym(first_rule_sym), &s3 = NextRuleSym(s2), &s4 = NextRuleSym(s3), &s5 = NextRuleSym(s4), &s6 = NextRuleSym(s5);
	assert(s3.m_meaning == csm_data_type && s6.m_meaning == csm_expression);

	// New expression object is needed.
	TCppCastExpr *new_expr_node = (TCppCastExpr*)AllocateNewCppItem(cxx_cast_expr);
	if (new_expr_node == NULL)
		return(NULL);

	// Propagate the const property of the operand up.
	new_expr_node->m_const_expr = s6.m_expression->m_const_expr;

	// Fill in the operands.
	new_expr_node->SetOperation(operation);
	new_expr_node->m_dest_type = s3.m_data_type;
	new_expr_node->AddChildItem(s6.m_expression, cexpr_chdest_cast_oprnd);

	// Data from the operand symbols is already taken.
	s3.ResetMeaning();
	s6.ResetMeaning();

	// Give the prepared expression out.
	return(new_expr_node);
}

TCppExpressionBase *TCppParser::ExprHandleTypeIdOperation(TCppSymbolStateItem &first_rule_sym, WORD rule_app_id)
{
	//
	// Possible rule forms:
	//
	//	TYPEID	'('  Expression	 ')'
	//	TYPEID	'('    TypeId	 ')'
	//

	// Pick up the rule symbols.
	TCppSymbolStateItem &s2 = NextRuleSym(first_rule_sym), &s3 = NextRuleSym(s2);

	// Pick up the "std::type_info" class. The standard tells that once this class is not defined at the point of using
	// the TYPEID operator, the program is ill formed. MS compiler is not making any silent substitution. It writes
	// syntax error. Do this search only once at the point of the first use of this operation.
	if (m_type_info_defn == NULL)
	{
		TCppDefnProblem std_type_info_problem = cdpr_none;
		const wchar_t *std_type_info_name[] = { L"std", L"type_info", NULL };
		m_type_info_defn = FindUnambigNonOverloadableDefnByName(std_type_info_name, std_type_info_problem);

		if (m_type_info_defn == NULL)
		{
			// Report error.
			assert(std_type_info_problem != cdpr_none);
			if (std_type_info_problem == cdpr_unres)
				ReportSyntaxError(&first_rule_sym, cerr_expr_typeid_type_info_missing);
			else ReportSyntaxError(&first_rule_sym, cerr_expr_typeid_type_info_problem);

			// Create substitution.
			TCppNameInfo unres_name_info;
			unres_name_info.SetupName(cnit_identifier, 0);
			unres_name_info.m_cached_name = L"std::type_info";
			m_type_info_defn = CreateUnresolvedObject(unres_name_info, std_type_info_problem, cxx_unresolved_data_type);
			if (m_type_info_defn == NULL)
				return(NULL);
		}
	}

	// New expression object is needed.
	TCppTypeidExpr *new_expr_node = (TCppTypeidExpr*)AllocateNewCppItem(cxx_typeid_expr);
	if (new_expr_node == NULL)
		return(NULL);

	// Fill in the props.
	new_expr_node->m_result_type = m_type_info_defn;
	if (rule_app_id == rcpp_pfix_tpid_expr)
	{
		assert(s3.m_meaning == csm_expression);
		new_expr_node->AddChildItem(s3.m_expression, cexpr_chdest_typeid_oprnd);
		s3.ResetMeaning();
	}
	else
	{
		assert(s3.m_meaning == csm_data_type);
		new_expr_node->SetDataTypeOperand(s3.m_data_type);
		s3.ResetMeaning();
	}

	// Call the callback handler for either filling the static result or adding dynamic type resolution code.
	m_objects_factory.FinalizeTypeidOperationObject(new_expr_node);

	// Give out the prepared expression.
	return(new_expr_node);
}

TCppExpressionBase *TCppParser::ExprHandlePrimaryThis(TCppSymbolStateItem &this_symbol)
{
	//
	// Possible rule forms:
	//
	//	THIS
	//

	// This is ''this" operand. Allocate the operand structure.
	TCppOperandExpr *oprnd_item = (TCppOperandExpr*)AllocateNewCppItem(cxx_operand_expr);
	if (oprnd_item == NULL)
		return(NULL);

	// Check the layer that owns current piece of code.
	TCppDefnBase *this_type = NULL;
	bool this_const = FALSE;
	TCppCodeEntryPointDefn *curr_func_object = GetOwningFunctionForCodeArea();
	if (curr_func_object == NULL)
	{
		// Operand this is used on the namespace layer or inside the class.
		ReportSyntaxError(&this_symbol, cerr_expr_this_outside_of_func);
	}
	else
	{
		// The owning function is available. This is is code context.
		TCppItemBase *curr_func_layer = curr_func_object->ItemParent();
		assert(curr_func_layer != NULL);

		if (curr_func_layer->IsNamespace() == TRUE)
		{
			// Current function resides on the namespace layer.
			ReportSyntaxError(&this_symbol, cerr_expr_this_in_namesp_func);
		}
		else
		{
			assert(curr_func_layer->IsStructType() == TRUE);
			if ((curr_func_object->m_func_attrs & fcta_static) != 0)
			{
				// Operand "this" is used in the static method.
				ReportSyntaxError(&this_symbol, cerr_expr_this_in_static_method, ((TCppDefnBase*)curr_func_layer)->GetElabTypeName(FALSE, TRUE));
			}
			else
			{
				// Accept the defns layer of the function as the type of the "this" keyword.
				this_type = (TCppDefnBase*)curr_func_layer;
				this_const = ((curr_func_object->m_func_attrs & fcta_this_const) != 0) ? TRUE : FALSE;
			}
		}
	}

	if (this_type == NULL)
	{
		// A subst for the missing type is needed.
		TCppNameInfo subst_type_info;
		subst_type_info.SetupName(cnit_identifier, 0);
		subst_type_info.m_cached_name = L"This";
		this_type = CreateUnresolvedObject(subst_type_info, cdpr_unres, cxx_unresolved_struct_type);
		if (this_type == NULL)
			return(NULL);
	}

	// Create pointer to the structured type.
	TCppDeclaratorModifier extra_ptr_spec;
	extra_ptr_spec.SetPointerSpec(this_const);
	TCppDefnBase *ptr_to_this_type = CreateCvPtrRefModification(&this_symbol, this_type, &extra_ptr_spec, 1, TRUE, TRUE);
	if (ptr_to_this_type == NULL)
		return(NULL);

	// Fill in the data fields.
	oprnd_item->m_const_expr = this_const;
	oprnd_item->m_oprnd_type = eopr_this;
	oprnd_item->m_result_type = ptr_to_this_type;

	// Success.
	return(oprnd_item);
}

TCppExpressionBase *TCppParser::ExprHandlePrimaryExpessionId(TCppSymbolStateItem &expr_id_sym)
{
	//
	// Possible rule forms:
	//
	//	ExpressionId
	//

	// This is named operand. Most likely it is either a data field or a name of the function.
	assert(expr_id_sym.m_app_id == sym_ExpressionId && expr_id_sym.m_meaning == csm_simp_or_qual);
	TCppNameInfo &name_info = expr_id_sym.m_simp_or_qual;

	// Allocate an operand structure for wrapping this name.
	TCppOperandExpr *oprnd_item = (TCppOperandExpr*)AllocateNewCppItem(cxx_operand_expr);
	if (oprnd_item == NULL)
		return(NULL);

	// Check situation on the defns space for this name.
	TCppDefnBase *operand_defn = NULL;
	TCppDefnsBucket *operand_defns_bucket = NULL;
	if (CheckLookupExistingDefinition(&expr_id_sym, elab_none, name_info, sym_PrimaryExpression, operand_defn, &operand_defns_bucket) == FALSE)
		return(NULL);

	// Check results of the search. Something should be returned, at least an unresolved object.
	assert(operand_defn != NULL);
	TCppItemType obj_type = operand_defn->ItemType();

	if (obj_type == cxx_enum_member)
	{
		// Discovered object is a name of the enum member.
		TCppEnumMemberDefn *enum_member  = (TCppEnumMemberDefn*)operand_defn;
		TCppEnumTypeDefn *enum_type  = (TCppEnumTypeDefn*)(operand_defn->ItemParent());
		assert(enum_type->ItemType() == cxx_enum_type);

		// This is a constant object.
		oprnd_item->m_const_expr = TRUE;

		oprnd_item->SetEnumLiteral(enum_member);
		oprnd_item->m_result_type = enum_type;
	}
	else if (obj_type == cxx_data_field || obj_type == cxx_unresolved_data_field)
	{
		// Both objects are data fields.
		TCppDataFieldDefn *data_field = (TCppDataFieldDefn*)operand_defn;

		// Set the L-value flag ON.
		if (obj_type == cxx_data_field)
			oprnd_item->m_lvalue_expr = TRUE;

		// Check if the type of the data field is const or not.
		if (data_field->m_field_type != NULL && data_field->m_field_type->ItemType() == cxx_cv_ptr_ref_type)
		{
			TCppCvPtrRefTypeDefn *cvpr_type = (TCppCvPtrRefTypeDefn*)data_field->m_field_type;
			if (cvpr_type->m_layers.NumItems() > 0 && cvpr_type->m_layers[cvpr_type->m_layers.NumItems()-1].cvpr_const == TRUE)
			{
				oprnd_item->m_lvalue_expr = FALSE;
				oprnd_item->m_const_expr = TRUE;
			}
		}

		oprnd_item->SetDataField(data_field);
		oprnd_item->m_result_type = data_field->m_field_type;
	}
	else if (operand_defn->IsFunctionFamily() == TRUE)
	{
		// The whole bucket of functions is returned. Note that the result type will be set later.
		assert(operand_defns_bucket != NULL);

		int num_overloads = TCppNewDefnsSearchHelper::GetNumberOfOverloads(name_info.m_core, operand_defn, operand_defns_bucket);
		if (obj_type == cxx_code_entry_point && num_overloads == 1)
		{
			// -BUGBUG- There is only one function with this name. Use it.
			oprnd_item->SetCodeEntryPoint((TCppCodeEntryPointDefn*)operand_defn);
		}
		else
		{
			oprnd_item->m_oprnd_type = eopr_ovld_defns_bucket;
			oprnd_item->m_ovld_defn_sample = operand_defn;
			oprnd_item->m_ovld_defns_bucket = operand_defns_bucket;
		}
	}
	else
	{
		// Unexpeccted object type. Issue an assert and return an unprepared operand.
		assert(FALSE);
	}

	// Sussess.
	return(oprnd_item);
}

void TCppParser::PromoteExistingDeclaration(TCppSymbolStateItem &new_non_term, TCppSymbolStateItem &sym)
{
	if (sym.m_meaning == csm_declaration)
	{
		assert(sym.m_declaration != NULL);
		new_non_term.m_meaning = csm_declaration;
		new_non_term.m_declaration = sym.m_declaration;
	}
	else
	{
		assert(sym.m_meaning == csm_declarations_list && sym.m_declarations_list != NULL);
		new_non_term.m_meaning = csm_declarations_list;
		new_non_term.m_declarations_list = sym.m_declarations_list;
	}

	// Data from the source symbol is already removed.
	sym.ResetMeaning();
}

void TCppParser::PromoteExistingDeclarator(TCppSymbolStateItem &new_non_term, TCppSymbolStateItem &sym)
{
	assert(sym.m_meaning == csm_declarator);
	new_non_term.m_meaning = csm_declarator;
	new_non_term.m_declarator_info = sym.m_declarator_info;
	sym.ResetMeaning();
}

bool TCppParser::CheckLookupSimpleIdentifierSymbol(TCppSymbolStateItem &symbol, TCppNameInfo &name_info, TCppItemType new_object_type,
														TCppDefnBase **in_typedef_type_out_definition, TCppDefnBase *specific_defns_layer)
{
	// Prepare the name info struct that is passed by the upper layer uninited.
	if (symbol.m_meaning == csm_declarator)
	{
		// Copy name info from the declarator and clear the name info there.
		name_info = symbol.m_declarator_info.m_name_info;
		symbol.m_declarator_info.m_name_info.Clear();
		if (name_info.m_name_problem != cdpr_none)
		{
			// The name is already modified. In any case it is not possible to do the search.
			return(TRUE);
		}
	}
	else
	{
		// Passed symbol should be a simple name. Worker function will ensure this.
		if (PrepareSimpleNameInfo(symbol, name_info) == FALSE)
			return(FALSE);
	}

	// Ensure that name info contains a simple identifier and that it is fully prepared.
	assert(name_info.IsSimpleIdentifier() == TRUE && name_info.KeyId() != 0 && name_info.m_cached_name != NULL);

	// Pick up the existing defn or bucket of defns for this name if any.
	TCppNewDefnsSearchHelper hlpr((specific_defns_layer != NULL) ? specific_defns_layer : m_defns_layer_owner);
	if (hlpr.DoSearch(elab_none, name_info.m_core) == TRUE)
	{
		// Some definition for the passed name is present.
		TCppItemType dft = hlpr.m_single_defn->ItemType();
		TCppDefnProblem defn_problem = cdpr_none;

		if (new_object_type == cxx_namespace)
		{
			// This is request for creating the namespace.
			assert(in_typedef_type_out_definition != NULL && *in_typedef_type_out_definition == NULL);
			if (dft == cxx_namespace)
			{
				// The object is a namespace. Give this namespace out.
				*in_typedef_type_out_definition = hlpr.m_single_defn;
			}
			else
			{
				// This is names collision.
				defn_problem = cdpr_conflict;
			}
		}
		else if (new_object_type == cxx_cv_ptr_ref_type)
		{
			// This is request for creating the typedef definition.
			assert(in_typedef_type_out_definition != NULL && *in_typedef_type_out_definition != NULL);
			TCppDefnBase *new_typedef = *in_typedef_type_out_definition;
			*in_typedef_type_out_definition = NULL;

			// Check the type of discovered object.
			if ((dft == cxx_cv_ptr_ref_type || dft == cxx_array_type || dft == cxx_data_type_alias) && new_typedef->IsDataType() == TRUE)
			{
				if (TCppItemBase::CompareSameDatabaseDataTypes((TCppDataTypeBase*)hlpr.m_single_defn, (TCppDataTypeBase*)new_typedef) == TRUE)
					*in_typedef_type_out_definition = hlpr.m_single_defn;
			}
			else if ((dft == cxx_function_type || dft == cxx_function_type_alias) && new_typedef->IsFunctionType() == TRUE)
			{
				if (TCppItemBase::CompareSameDatabaseFunctionTypes((TCppFunctionTypeBase*)hlpr.m_single_defn, (TCppFunctionTypeBase*)new_typedef, FALSE) == TRUE)
					*in_typedef_type_out_definition = hlpr.m_single_defn;
			}

			if (*in_typedef_type_out_definition == NULL)
			{
				// Existing object has some other cpp item type or the new typedef plans to define different type.
				defn_problem = cdpr_conflict;
			}
		}
		else if (new_object_type == cxx_bit_field_type && dft == cxx_data_field)
		{
			// This is special case of name duplication. The bit field is basically a data field and it is represented
			// with a data field object in the structured code. The cxx_bit_field_type value is passed to differentiate
			// this call from the call from the uninited condition context.
			defn_problem = cdpr_dupe;
		}
		else if (new_object_type == dft)
		{
			// Discovered object matches the expected type. This is name duplication.
			defn_problem = cdpr_dupe;
		}
		else
		{
			// This cpp item types are different. This is names collision.
			defn_problem = cdpr_conflict;
		}

		if (defn_problem != cdpr_none)
		{
			// The problem is detected. Reset the key id and setup an alternative name.
			const wchar_t *original_name = name_info.m_cached_name;
			if (name_info.SetupShortNameDefnProblem(this, defn_problem) == FALSE)
				return(FALSE);

			// Note that different values of the new_object_type param are passed all calls to this method.
			// This is needed to be able to construct the error message properly.
			const wchar_t *new_object_type_name = L"UNEXPECTED-OBJECT-TYPE";
			switch (new_object_type)
			{
				case cxx_namespace:		  new_object_type_name = L"Namespace";			break;
				case cxx_namespace_alias:	  new_object_type_name = L"Namespace alias";		break;
				case cxx_enum_member:	  new_object_type_name = L"Enum member";		break;
				case cxx_bit_field_type:	  new_object_type_name = L"Bit field member";		break;
				case cxx_data_field:		  new_object_type_name = L"Condition";			break;
				case cxx_cv_ptr_ref_type:	  new_object_type_name = L"Typedef";			break;
			}

			// Report the names collision error.
			ReportSyntaxError(&symbol, cerr_object_already_defined, original_name, new_object_type_name, name_info.m_cached_name);
		}
	}
	else if (in_typedef_type_out_definition != NULL)
	{
		// Clear the output field to indicate that nothing was discovered.
		*in_typedef_type_out_definition = NULL;
	}

	// Success. No critical error has happened.
	return(TRUE);
}

bool TCppParser::CheckLookupElabTypeSpecName(TCppDefnBase *dest_defns_layer, TCppElabType elab_type, TCppNameInfo &name_info, TCppItemType dest_item_type,
													TCppDataTypeBase *&compatible_elaborated_type_object)
{
	assert(dest_defns_layer != NULL && elab_type != elab_none);
	assert(compatible_elaborated_type_object == NULL);
	wchar_t buff80[80];

	// Check the type of the destination layer.
	TCppDefnProblem new_fwd_decl_problem = cdpr_none;
	if (dest_defns_layer->IsTemplateInstantiation() == TRUE)
	{
		// The target layer is the body of the class template instantiation.
		TCppErrorReportingCtxSymbolStateItem error_context(name_info);
		ReportSyntaxError(&error_context, cerr_elab_fwd_in_template_inst, TCppDefnBase::GetObjectTypeFriendlyName(buff80, dest_item_type, elab_type, TRUE));
		new_fwd_decl_problem = cdpr_errloc;
	}
	else if (dest_defns_layer->IsParentOf(m_defns_layer_owner) == TRUE)
	{
		// The target layer is an upper layer.
		if (dest_defns_layer->IsStructType() == TRUE)
		{
			TCppErrorReportingCtxSymbolStateItem error_context(name_info);
			ReportSyntaxError(&error_context, cerr_elab_fwd_on_upper_class_layer, TCppDefnBase::GetObjectTypeFriendlyName(buff80, dest_item_type, elab_type, TRUE));
			new_fwd_decl_problem = cdpr_errloc;
		}
	}
	else if (dest_defns_layer->IsChildOf(m_defns_layer_owner) == TRUE)
	{
		// This is a lower layer. Modify the name.
		TCppErrorReportingCtxSymbolStateItem error_context(name_info);
		ReportSyntaxError(&error_context, cerr_elab_fwd_on_lower_layer, TCppDefnBase::GetObjectTypeFriendlyName(buff80, dest_item_type, elab_type, TRUE));
		new_fwd_decl_problem = cdpr_errloc;
	}
	else if (dest_defns_layer != m_defns_layer_owner)
	{
		// This is an unrealated layer.
		TCppErrorReportingCtxSymbolStateItem error_context(name_info);
		ReportSyntaxError(&error_context, cerr_elab_fwd_on_unrelated_layer, TCppDefnBase::GetObjectTypeFriendlyName(buff80, dest_item_type, elab_type, TRUE));
		new_fwd_decl_problem = cdpr_errloc;
	}

	// Check contents of the destination layer.
	if (new_fwd_decl_problem == cdpr_none)
	{
		// Passed destination layer allows creating the forward declaration.
		TCppNewDefnsSearchHelper hlpr(dest_defns_layer);
		if (hlpr.DoSearch(elab_type, name_info.m_core) == TRUE)
		{
			// Something with this name is present.
			if (dest_item_type == cxx_enum_type && hlpr.m_single_defn->ItemType() == cxx_enum_type)
			{
				// Pick up the discovered object.
				compatible_elaborated_type_object = (TCppDataTypeBase*)hlpr.m_single_defn;
				return(TRUE);
			}
			else if (dest_item_type == cxx_struct_type && hlpr.m_single_defn->ItemType() == cxx_struct_type)
			{
				// Pick up the discovered object.
				compatible_elaborated_type_object = (TCppDataTypeBase*)hlpr.m_single_defn;
				return(TRUE);
			}
			else if (dest_item_type == cxx_class_template &&  hlpr.m_single_defn->IsClassTemplateFamily() == TRUE)
			{
				TCppClassTemplateDefn *class_template_overload =  hlpr.FindClassTemplateOverload(name_info.KeyId(), name_info.TemplateHeaderParams());
				if (class_template_overload != NULL)
				{
					// Pick up the discovered class template.
					compatible_elaborated_type_object = class_template_overload;
					return(TRUE);
				}
			}
			else
			{
				// This is a names conflict.
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&error_context, cerr_elab_fwd_names_conflict, name_info.m_cached_name, TCppDefnBase::GetObjectTypeFriendlyName(buff80, dest_item_type, elab_type, FALSE));
				new_fwd_decl_problem = cdpr_conflict;
			}
		}
	}

	// Check for the discovered problem.
	if (new_fwd_decl_problem != cdpr_none)
	{
		// Modify the short name.
		if (name_info.SetupShortNameDefnProblem(this, new_fwd_decl_problem) == FALSE)
			return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TCppParser::CheckLookupElabTypeDefnName(TCppElabType elab_type, TCppSymbolStateItem *name_symbol, TCppItemType dest_item_type,
													TCppDefnBase *&compatible_forward_declaration)
{
	// This method expects to receive valid name symbol.
	assert(elab_type != elab_none);
	assert(name_symbol != NULL && name_symbol->m_meaning == csm_simp_or_qual);
	assert(compatible_forward_declaration == NULL);
	wchar_t buff80[80];

	// The key id should be not equal to zero because it is needed to do the search.
	TCppNameInfo &name_info = name_symbol->m_simp_or_qual;
	assert(name_info.IsIdentifier() == TRUE && name_info.KeyId() != 0);

	if (name_info.IsUnresolvedLayer() == TRUE)
	{
		// An uresolved path is already present. Error on the first element of this path is already reported.
		// Create name for the new object that includes all unresolved elements.
		return(name_info.FinalizeLayerResProblemName(this));
	}

	TCppDefnProblem defn_problem = cdpr_none;
	TCppDefnBase *defns_layer = name_info.GetDefnsLayer(m_defns_layer_owner);

	if (defns_layer->IsTemplateInstantiation() == TRUE)
	{
		// Qualification path is present in the name and this path leads into the class template instantiation.
		// New objects are not allowed there. All objects inside the template instantiation should be created
		// by the class template instantiation procedure. Report an error but nevertheless the caller will
		// create an object there with an "ErrorLoc" prefix in the name.
		assert(name_info.m_resolved_defns_layer != NULL);
		ReportSyntaxError(name_symbol, cerr_elab_defn_in_template_inst, TCppDefnBase::GetObjectTypeFriendlyName(buff80, dest_item_type, elab_type, TRUE));
		defn_problem = cdpr_errloc;
	}
	else
	{
		// Name belongs to known layer that was not generated automatically. Check the type of this layer.
		if (defns_layer->IsParentOf(m_defns_layer_owner) == TRUE)
		{
			// Qualification path points to an upper layer.
			ReportSyntaxError(name_symbol, cerr_elab_defn_on_upper_layer, TCppDefnBase::GetObjectTypeFriendlyName(buff80, dest_item_type, elab_type, TRUE));
			defn_problem = cdpr_errloc;
		}
		else if (defns_layer == m_defns_layer_owner || defns_layer->IsChildOf(m_defns_layer_owner) == TRUE)
		{
			// Name symbol contains short name or the current layer was specified explicitly or this is an inner layer.
			// All these types of layers require checking their contents.
			bool something_discovered = FALSE;
			TCppNewDefnsSearchHelper hlpr(defns_layer);
			if (hlpr.DoSearch(elab_type, name_info.m_core) == TRUE)
			{
				// Something with this name is present.
				something_discovered = TRUE;

				if (dest_item_type == cxx_enum_type && hlpr.m_single_defn->ItemType() == cxx_enum_type)
				{
					TCppEnumTypeDefn *enum_object_proto = (TCppEnumTypeDefn*)hlpr.m_single_defn;
					if (enum_object_proto->m_defined == FALSE)
					{
						// Caller will expand this forward declaration into the full definition.
						compatible_forward_declaration = enum_object_proto;
					}
				}
				else if (dest_item_type == cxx_struct_type && hlpr.m_single_defn->ItemType() == cxx_struct_type)
				{
					TCppStructTypeDefn *struct_object_proto = (TCppStructTypeDefn*)hlpr.m_single_defn;
					if (struct_object_proto->m_defined == FALSE)
					{
						// Caller will expand this forward declaration into the full definition.
						compatible_forward_declaration = struct_object_proto;
					}
				}
				else if (dest_item_type == cxx_class_template &&  hlpr.m_single_defn->IsClassTemplateFamily() == TRUE)
				{
					// Class template is a special case because several class templates with the same name can exist
					// on the same layer of defninitions. All these templates should differ by their sets of params.
					// Compare params from the unprocessed template header with params in existing class templates.
					assert(name_info.m_unproc_template_header != NULL);
					TCppClassTemplateDefn *class_template_proto =  hlpr.FindClassTemplateOverload(name_info.KeyId(), name_info.TemplateHeaderParams());
					if (class_template_proto == NULL)
					{
						// All existing templates have different sets of params. Treat this case as if nothing was discovered.
						something_discovered = FALSE;
					}
					else if (class_template_proto->m_defined == FALSE)
					{
						// Compatible class template is available and this template is a forward declaration.
						// Caller will expand this forward declaration into the full definition.
						compatible_forward_declaration = class_template_proto;
					}
				}
				else
				{
					// Existing non overloadable object has some other type or the search procedure discovered a group
					// of overloadable objects that have some other nature.
					ReportSyntaxError(name_symbol, cerr_elab_defn_names_conflict, name_info.m_cached_name, TCppDefnBase::GetObjectTypeFriendlyName(buff80, dest_item_type, elab_type, TRUE));
					defn_problem = cdpr_conflict;
				}
			}

			// Check results of the search.
			if (something_discovered == TRUE)
			{
				if (defn_problem == cdpr_none && compatible_forward_declaration == NULL)
				{
					// Discovered object is not has not generated names conflict. This means that its object type is compatible.
					// At the same time it is not a compatible forward declaration. This means that this is object duplication.
					const wchar_t *params_explanation = L"";
					if (dest_item_type == cxx_class_template)
						params_explanation = L" with identical set of params";

					ReportSyntaxError(name_symbol, cerr_elab_defn_already_existing, TCppDefnBase::GetObjectTypeFriendlyName(buff80, dest_item_type, elab_type, TRUE), name_info.m_cached_name, params_explanation);
					defn_problem = cdpr_dupe;
				}
			}
			else if (defns_layer->IsChildOf(m_defns_layer_owner) == TRUE)
			{
				// This is a new definition on the lower layer. Modify the name.
				ReportSyntaxError(name_symbol, cerr_elab_defn_on_lower_layer, TCppDefnBase::GetObjectTypeFriendlyName(buff80, dest_item_type, elab_type, TRUE));
				defn_problem = cdpr_errloc;
			}
		}
		else
		{
			// Qualification path points to some existing unrelated layer.
			ReportSyntaxError(name_symbol, cerr_elab_defn_on_unrelated_layer, TCppDefnBase::GetObjectTypeFriendlyName(buff80, dest_item_type, elab_type, TRUE));
			defn_problem = cdpr_errloc;
		}
	}

	if (defn_problem != cdpr_none)
	{
		// Request for the name modification is pending.
		if (name_info.SetupShortNameDefnProblem(this, defn_problem) == FALSE)
			return(FALSE);
	}

	// Success. No critical error has happened.
	return(TRUE);
}

bool TCppParser::CheckLookupDataFieldObject(TCppDefnBase *dest_defns_layer, TCppStorageClass strg_class, TCppNameInfo &name_info, TCppDataFieldDefn *&compatible_data_field)
{
	assert(name_info.IsIdentifier() == TRUE && name_info.KeyId() != 0);

	if (name_info.IsUnresolvedLayer() == TRUE)
	{
		// (1) An uresolved path is already present. Error on the first element of this path is already reported.
		// Create name for the new data field using all already unresolved elements and the final name.
		return(name_info.FinalizeLayerResProblemName(this));
	}

	TCppDefnProblem defn_problem = cdpr_none;
	if (dest_defns_layer->IsTemplateInstantiation() == TRUE)
	{
		// (2) Source code wants to a create data field inside the template instantiation.
		TCppErrorReportingCtxSymbolStateItem error_context(name_info);
		ReportSyntaxError(&error_context, cerr_data_field_in_template_inst);
		defn_problem = cdpr_errloc;
	}
	else
	{
		// Target layer is a real layer. Run the lookup procedure on this layer.
		TCppItemType existing_obj_type = cxx_none;
		TCppStorageClass existing_obj_strg_class = strgc_num_types;

		TCppNewDefnsSearchHelper hlpr(dest_defns_layer);
		if (hlpr.DoSearch(elab_none, name_info.m_core) == TRUE)
		{
			existing_obj_type = hlpr.m_single_defn->ItemType();
			if (existing_obj_type == cxx_data_field)
				existing_obj_strg_class = ((TCppDataFieldDefn*)hlpr.m_single_defn)->m_strg_class;
		}

		if (dest_defns_layer->IsParentOf(m_defns_layer_owner) == TRUE)
		{
			// Target layer is an upper layer.
			if (dest_defns_layer->IsStructType() == TRUE || strg_class != strgc_extern)
			{
				// (3.1) Source code either wants to a create data field inside the upper class or a non extern data field
				// on the upper namespace layer. Results of the name lookup are not important in this case.
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&error_context, cerr_data_field_on_upper_layer);
				defn_problem = cdpr_errloc;
			}
			else if (existing_obj_type == cxx_none)
			{
				// (3.2) Source code wants to a create extern data field on the namespace layer and this name is not
				// occupied on this namespace with anything. Allow this creation using the original name.
				return(TRUE);
			}
			else if (existing_obj_type == cxx_data_field)
			{
				// (3.3) Source code wants to create an extern data field on the namespace layer while the data field with
				// this name is already existing. Return this data field for further processing.
				compatible_data_field = (TCppDataFieldDefn*)hlpr.m_single_defn;
				return(TRUE);
			}
			else
			{
				// (3.4) This is names conflict with some other non data field object.
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&error_context, cerr_data_field_names_conflict, name_info.m_cached_name);
				defn_problem = cdpr_conflict;
			}
		}
		else if (dest_defns_layer == m_defns_layer_owner)
		{
			// Target layer is the current layer.
			if (existing_obj_type == cxx_none)
			{
				// (4.1) Source code wants to a create data field on the current layer and on this layer this name
				// is not occupied with anything. Allow creation using the original name.
				return(TRUE);
			}
			else if (existing_obj_type == cxx_data_field)
			{
				if (dest_defns_layer->IsNamespace() == TRUE)
				{
					if (strg_class == strgc_extern)
					{
						// (4.2) Source code wants to a create extern data field on the current layer that is a namespace layer
						// while the data field with this name is already existing.
						compatible_data_field = (TCppDataFieldDefn*)hlpr.m_single_defn;
						return(TRUE);
					}
					else if (existing_obj_strg_class != strgc_extern)
					{
						// (4.3) This is data field duplication.
						TCppErrorReportingCtxSymbolStateItem error_context(name_info);
						ReportSyntaxError(&error_context, cerr_data_field_name_duplication, name_info.m_cached_name);
						defn_problem = cdpr_dupe;
					}
					else
					{
						// (4.4) Source code wants to convert existing extern data field on the current layer that is a namespace
						// layer into the full object. Return this data field for further processing.
						compatible_data_field = (TCppDataFieldDefn*)hlpr.m_single_defn;
						return(TRUE);
					}
				}
				else
				{
					// (4.5) This is data field duplication.
					TCppErrorReportingCtxSymbolStateItem error_context(name_info);
					ReportSyntaxError(&error_context, cerr_data_field_name_duplication, name_info.m_cached_name);
					defn_problem = cdpr_dupe;
				}
			}
			else
			{
				// (4.6) This is names conflict with some other non data field object.
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&error_context, cerr_data_field_names_conflict, name_info.m_cached_name);
				defn_problem = cdpr_conflict;
			}
		}
		else if (dest_defns_layer->IsChildOf(m_defns_layer_owner) == TRUE)
		{
			// Target layer is an lower layer.
			if (strg_class == strgc_extern)
			{
				// (5.1) Source code wants to a create extern data field on the lower namespace.
				assert(dest_defns_layer->IsNamespace() == TRUE);
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&error_context, cerr_data_field_lo_layer_extern);
				defn_problem = cdpr_errloc;
			}
			else if (strg_class == strgc_mutable)
			{
				// (5.2) Source code wants to a create mutable data field on the lower class layer.
				assert(dest_defns_layer->IsStructType() == TRUE);
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&error_context, cerr_data_field_lo_layer_mutable, TCppDefnBase::GetElabTypeName(dest_defns_layer->DefnElabType(), FALSE, TRUE));
				defn_problem = cdpr_errloc;
			}
			else if (existing_obj_type != cxx_data_field)
			{
				// (5.3) New definitions are not allowed on the lower layers. Report names conflict as general
				// problem of the definition location.
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&error_context, cerr_data_field_on_lower_layer);
				defn_problem = cdpr_errloc;
			}
			else
			{
				// The data field with this name is existing.
				if (dest_defns_layer->IsNamespace() == TRUE)
				{
					// The data field with this name is existing.
					if (existing_obj_strg_class == strgc_extern)
					{
						// (5.4) Source code wants to convert extern data field on the lower namespace layer into the full object.
						compatible_data_field = (TCppDataFieldDefn*)hlpr.m_single_defn;
						return(TRUE);
					}
					else
					{
						// (5.5) Source code wants to a create non extern data field on the namespace layer while
						// non extern data field with this name is already existing.
						TCppErrorReportingCtxSymbolStateItem error_context(name_info);
						ReportSyntaxError(&error_context, cerr_data_field_lo_namesp_dupe, name_info.m_cached_name);
						defn_problem = cdpr_dupe;
					}
				}
				else
				{
					assert(dest_defns_layer->IsStructType() == TRUE);
					if (existing_obj_strg_class == strgc_extern)
					{
						// (5.6) Source code wants to convert prototype of the static data field of the class into the full object.
						compatible_data_field = (TCppDataFieldDefn*)hlpr.m_single_defn;
						return(TRUE);
					}
					else if (existing_obj_strg_class != strgc_static)
					{
						// (5.7) This is data field duplication.
						TCppErrorReportingCtxSymbolStateItem error_context(name_info);
						ReportSyntaxError(&error_context, cerr_data_field_lo_class_inst_dupe, name_info.m_cached_name, dest_defns_layer->GetElabTypeName());
						defn_problem = cdpr_dupe;
					}
					else
					{
						// (5.8) Source code wants to define static data field inside the lower class while data field with this name
						// is existing and this is an instance data field.
						TCppErrorReportingCtxSymbolStateItem error_context(name_info);
						ReportSyntaxError(&error_context, cerr_data_field_lo_class_sttc_dupe, name_info.m_cached_name, dest_defns_layer->GetElabTypeName());
						defn_problem = cdpr_errloc;
					}
				}
			}
		}
		else
		{
			// (6) Target layer is an unrelated layer.
			TCppErrorReportingCtxSymbolStateItem error_context(name_info);
			ReportSyntaxError(&error_context, cerr_data_field_on_unrelated_layer);
			defn_problem = cdpr_errloc;
		}
	}

	// Once the control has came here,  this means that some name problem was detected.
	assert(defn_problem != cdpr_none);
	return(name_info.SetupShortNameDefnProblem(this, defn_problem));
}

bool TCppParser::CheckLookupFucntionObject(bool want_friend_proxy, TCppDefnBase *dest_defns_layer, TCppNameInfo &name_info, TCppDeclaratorModifier *func_modifier, TCppCodeEntryPointDefn *&compatible_func_object, bool func_header_ctx)
{
	// This method should be used only for finding functions and func templates. Function template specs
	// and function template instantiations should be looked up using other methods.
	assert(name_info.TemplateArgsPresent() == FALSE && name_info.m_name_problem == cdpr_none);
	assert(name_info.IsUnresolvedLayer() == FALSE && compatible_func_object == NULL);
	wchar_t buff96[96];

	if (want_friend_proxy == TRUE && func_header_ctx == FALSE)
	{
		// Source code wants to create a friend proxy out of the function prototype. Search for an existing object.
		TCppExistingDefnsSearchHelper hlpr(name_info.GetDefnsLayer(m_defns_layer_owner));
		if (hlpr.DoSearch(elab_none, name_info.m_core, name_info.GetSearchHierFlag()) == TRUE)
		{
			// Something is present.
			TCppDefnProblem existing_func_search_problem = cdpr_none;
			if (hlpr.IsAmbigResult() == TRUE)
			{
				// Passed name is ambiguous.
				if (hlpr.m_curr_search_layer->IsChildOf(dest_defns_layer) == TRUE)
				{
					TCppErrorReportingCtxSymbolStateItem error_context(name_info);
					ID main_error_id = ReportSyntaxError(&error_context, cerr_func_obj_friend_proto_ambig, hlpr.m_ambig_defns.NumItems());
					ReportAmbiguousDefnsArray(&error_context, main_error_id, hlpr.m_ambig_defns);
					existing_func_search_problem = cdpr_ambig;
				}
			}
			else if (hlpr.m_single_defn->IsFunctionFamily() == TRUE)
			{
				// Discovered object is a function or a func template. Resolve the params overloading.
				TCppCodeEntryPointDefn *existing_func_object;
				if (name_info.m_unproc_template_header == NULL)
					existing_func_object = hlpr.FindCodeEntryPointOverload(name_info.m_core, func_modifier);
				else existing_func_object = hlpr.FindFunctionTemplateOverload(name_info.TemplateHeaderParams(), name_info.m_core, func_modifier);

				if (existing_func_object != NULL)
				{
					// Object with compatible set of params is existing. Return it for further processing.
					compatible_func_object = existing_func_object;
					return(TRUE);
				}
			}
			else
			{
				// Passed name matches some object that is not a func and not a func template.
				if (hlpr.m_curr_search_layer->IsChildOf(dest_defns_layer) == TRUE)
				{
					TCppErrorReportingCtxSymbolStateItem error_context(name_info);
					ReportSyntaxError(&error_context, cerr_func_obj_friend_proto_unappr, name_info.m_cached_name, name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
					existing_func_search_problem = cdpr_unapprop;
				}
			}

			if (existing_func_search_problem != cdpr_none)
			{
				// The search problem it present. Fill in the layer, where the problem was discovered, into the name
				// info as an explicit layer and integrate discovered problem into the name info.
				name_info.m_resolved_defns_layer = hlpr.m_curr_search_layer;
				return(name_info.SetupShortNameDefnProblem(this, existing_func_search_problem));
			}
		}
	}

	TCppDefnProblem func_defn_problem = cdpr_none;
	if (dest_defns_layer->IsTemplateInstantiation() == TRUE)
	{
		// Source code wants to a create function object inside the template instantiation.
		TCppErrorReportingCtxSymbolStateItem error_context(name_info);
		ReportSyntaxError(&error_context, cerr_func_obj_in_template_inst, name_info.GetFuncNameCp(buff96, dest_defns_layer, func_header_ctx));
		func_defn_problem = cdpr_errloc;
	}
	else
	{
		// Target layer is a real layer. Run the lookup procedure on this layer.
		TCppNewDefnsSearchHelper hlpr(dest_defns_layer);
		bool something_discovered = hlpr.DoSearch(elab_none, name_info.m_core);
		TCppCodeEntryPointDefn *discovered_func_object = NULL;

		if (something_discovered == TRUE && hlpr.m_single_defn->IsFunctionFamily() == TRUE)
		{
			// Discovered object is a function or a func template. Resolve the params overloading.
			if (name_info.m_unproc_template_header == NULL)
				discovered_func_object = hlpr.FindCodeEntryPointOverload(name_info.m_core, func_modifier);
			else discovered_func_object = hlpr.FindFunctionTemplateOverload(name_info.TemplateHeaderParams(), name_info.m_core, func_modifier);

			if (discovered_func_object == NULL)
			{
				// Passed set of parans is unknown. Consider this case as if nothing was discovered.
				something_discovered = FALSE;
			}
		}

		if (dest_defns_layer->IsParentOf(m_defns_layer_owner) == TRUE)
		{
			// Target layer is an upper layer of the current layer.
			if (dest_defns_layer->IsNamespace() == TRUE)
			{
				if (func_header_ctx == FALSE)
				{
					// Source code wants to create prototype on the upper namespace layer.
					if (something_discovered == FALSE)
					{
						// There is no object with this name on the dest layer. Allow this creation using the original name.
						return(TRUE);
					}
					else if (discovered_func_object != NULL)
					{
						// Object with this name is already existing. Return discovered object for further processing.
						compatible_func_object = discovered_func_object;
						return(TRUE);
					}
					else
					{
						// This is conflict with an object of some other type.
						TCppErrorReportingCtxSymbolStateItem error_context(name_info);
						const wchar_t *func_obj_type_name = name_info.GetFuncNameCs(buff96, dest_defns_layer, func_header_ctx);
						ReportSyntaxError(&error_context, cerr_func_obj_names_conflict, name_info.m_cached_name, func_obj_type_name);
						func_defn_problem = cdpr_conflict;
					}
				}
				else if (want_friend_proxy == FALSE)
				{
					// Source code wants to create full object on the upper namespace layer.
					TCppErrorReportingCtxSymbolStateItem error_context(name_info);
					ReportSyntaxError(&error_context, cerr_func_obj_on_upper_layer, name_info.GetFuncNameCp(buff96, dest_defns_layer, func_header_ctx));
					func_defn_problem = cdpr_errloc;
				}
				else
				{
					// This is special case that is explicitly mentioned in the standard. Source code wants to create
					// full friendly object on the namespace layer while the definition itself stays inside the class.
					assert(m_defns_layer_owner->IsStructType() == TRUE);
					if (something_discovered == FALSE)
					{
						// There is no object with this name on the dest layer. Allow this creation using the original name.
						return(TRUE);
					}
					else if (discovered_func_object != NULL && discovered_func_object->m_function_body == NULL)
					{
						// Prototype with this name is already existing. Return discovered prototype for further processing.
						compatible_func_object = discovered_func_object;
						return(TRUE);
					}
					else if (discovered_func_object != NULL && discovered_func_object->m_function_body != NULL)
					{
						// Full object with this name is already existing. This is duplication.
						TCppErrorReportingCtxSymbolStateItem error_context(name_info);
						const wchar_t *func_obj_type_name = name_info.GetFuncNameCs(buff96, dest_defns_layer, func_header_ctx);
						ReportSyntaxError(&error_context, cerr_func_obj_name_duplication, func_obj_type_name, name_info.m_cached_name, func_obj_type_name);
						func_defn_problem = cdpr_dupe;
					}
					else
					{
						// This is conflict with an object of some other type.
						TCppErrorReportingCtxSymbolStateItem error_context(name_info);
						const wchar_t *func_obj_type_name = name_info.GetFuncNameCs(buff96, dest_defns_layer, func_header_ctx);
						ReportSyntaxError(&error_context, cerr_func_obj_names_conflict, name_info.m_cached_name, func_obj_type_name);
						func_defn_problem = cdpr_conflict;
					}
				}
			}
			else
			{
				// Source code wants to create something on the upper class layer.
				assert(dest_defns_layer->IsStructType() == TRUE);
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&error_context, cerr_func_obj_on_upper_layer, name_info.GetFuncNameCp(buff96, dest_defns_layer, func_header_ctx));
				func_defn_problem = cdpr_errloc;
			}
		}
		else if (dest_defns_layer == m_defns_layer_owner)
		{
			// Target layer is the current layer. This can be either a short name or a full name of the current layer.
			if (something_discovered == FALSE)
			{
				// There is no object with this name on the dest layer. Allow this creation using the original name.
				return(TRUE);
			}
			else if (discovered_func_object != NULL)
			{
				if (func_header_ctx == FALSE || discovered_func_object->m_function_body == NULL)
				{
					// Source code wants to a create prototype on the current layer or it wants to expand existing prototype,
					// that resides on the current layer, info the full object. Return discovered object for further processing.
					compatible_func_object = discovered_func_object;
					return(TRUE);
				}
				else
				{
					// This is function name duplication.
					TCppErrorReportingCtxSymbolStateItem error_context(name_info);
					const wchar_t *func_obj_type_name = name_info.GetFuncNameCs(buff96, dest_defns_layer, func_header_ctx);
					ReportSyntaxError(&error_context, cerr_func_obj_name_duplication, func_obj_type_name, name_info.m_cached_name, func_obj_type_name);
					func_defn_problem = cdpr_dupe;
				}
			}
			else
			{
				// This is conflict with an object of some other type.
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				const wchar_t *func_obj_type_name = name_info.GetFuncNameCs(buff96, dest_defns_layer, func_header_ctx);
				ReportSyntaxError(&error_context, cerr_func_obj_names_conflict, name_info.m_cached_name, func_obj_type_name);
				func_defn_problem = cdpr_conflict;
			}
		}
		else if (dest_defns_layer->IsChildOf(m_defns_layer_owner) == TRUE)
		{
			// Target layer is a lower layer of the current layer.
			if (func_header_ctx == FALSE || something_discovered == FALSE)
			{
				// Source code wants to a create a prototype or a full func object while there is no prototype with
				// this name on that lower layer.
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				ReportSyntaxError(&error_context, cerr_func_obj_on_lower_layer, name_info.GetFuncNameCp(buff96, dest_defns_layer, func_header_ctx));
				func_defn_problem = cdpr_errloc;
			}
			else if (discovered_func_object && discovered_func_object->m_function_body == NULL)
			{
				// Source code wants to a create function object with the body on the lower layer and a prototype
				// object with this name is already existing. Return this func object for further processing.
				compatible_func_object = discovered_func_object;
				return(TRUE);
			}
			else if (discovered_func_object && discovered_func_object->m_function_body != NULL)
			{
				// This is function name duplication.
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				const wchar_t *func_obj_type_name = name_info.GetFuncNameCs(buff96, dest_defns_layer, func_header_ctx);
				ReportSyntaxError(&error_context, cerr_func_obj_name_duplication, func_obj_type_name, name_info.m_cached_name, func_obj_type_name);
				func_defn_problem = cdpr_dupe;
			}
			else
			{
				// This is conflict with an object of some other type.
				TCppErrorReportingCtxSymbolStateItem error_context(name_info);
				const wchar_t *func_obj_type_name = name_info.GetFuncNameCs(buff96, dest_defns_layer, func_header_ctx);
				ReportSyntaxError(&error_context, cerr_func_obj_names_conflict, name_info.m_cached_name, func_obj_type_name);
				func_defn_problem = cdpr_conflict;
			}
		}
		else
		{
			// Target layer is an unrelated layer.
			TCppErrorReportingCtxSymbolStateItem error_context(name_info);
			ReportSyntaxError(&error_context, cerr_func_obj_on_unrelated_layer, name_info.GetFuncNameCp(buff96, dest_defns_layer, func_header_ctx));
			func_defn_problem = cdpr_errloc;
		}
	}

	// Once the control has came here,  this means that some name problem was detected.
	assert(func_defn_problem != cdpr_none);
	return(name_info.SetupShortNameDefnProblem(this, func_defn_problem));
}

bool TCppParser::CheckLookupExistingDefinition(TSymbolStateItem *err_ctx_sym, TCppElabType elab_type, TCppNameInfo &name_info, WORD dest_symbol_app_id,
													TCppDefnBase *&compatible_object, TCppDefnsBucket **compatible_defns_bucket)
{
	assert(name_info.m_preferred_object_id == 0);
	assert(compatible_object == NULL);

	// Step 1. Resolve the name info if possible.
	TCppDefnProblem defn_resolution_problem = cdpr_none;
	if (name_info.IsUnresolvedLayer() == FALSE)
	{
		// Search for existing definitions.
		TCppExistingDefnsSearchHelper hlpr(name_info.GetDefnsLayer(m_defns_layer_owner));
		if (hlpr.DoSearch(elab_type, name_info.m_core, name_info.GetSearchHierFlag()) == TRUE)
		{
			// Something is present.
			if (hlpr.IsAmbigResult() == TRUE)
			{
				// An existing name is ambiguous.
				TCppErrorReportingCtxSymbolStateItem err_ctx(name_info);
				ID main_error_id = ReportSyntaxError(&err_ctx, cerr_defns_search_name_ambig, name_info.m_cached_name, hlpr.m_ambig_defns.NumItems());
				ReportAmbiguousDefnsArray(&err_ctx, main_error_id, hlpr.m_ambig_defns);
				defn_resolution_problem = cdpr_ambig;
			}
			else
			{
				TCppDefnBase *pri_defn = hlpr.m_single_defn;
				assert(pri_defn != NULL);

				// Preset error variable with the most common value.
				defn_resolution_problem = cdpr_unapprop;
				switch (dest_symbol_app_id)
				{
					case sym_UsingDirective:
					case sym_NamespaceAliasDefinition:
							{
								// Caller wants a namespace object.
								if (pri_defn->ItemType() == cxx_namespace)
								{
									// Return the discovered namespace.
									compatible_object = pri_defn;
									return(TRUE);
								}
								else if (pri_defn->ItemType() == cxx_namespace_alias)
								{
									// Return the namespace and not the alias object.
									TCppNamespaceAliasDefn *alias_object = (TCppNamespaceAliasDefn*)pri_defn;
									compatible_object = alias_object->m_base_namespace;
									return(TRUE);
								}

								// The discovered object is not good.
								ReportSyntaxError(err_ctx_sym, cerr_defns_search_not_namespace, name_info.m_cached_name);
								break;
							}

					case sym_NamedTypeSpecifier:
							{
								// Data types and function types are good for this destination.
								if (pri_defn->IsClassTemplateFamily() == TRUE)
								{
									if (name_info.IsTempalteId() == TRUE)
									{
										// This is a request for implicit template instantiation.
									}
								}
								else if (pri_defn->IsDataType() == TRUE || pri_defn->IsFunctionType() == TRUE)
								{
									if (name_info.IsTempalteId() == TRUE)
									{
										// Bogus template params.
										ReportSyntaxError(err_ctx_sym, cerr_defns_search_bad_template_args);
									}

									// Return the discovered type.
									compatible_object = pri_defn;
									return(TRUE);
								}
								else if (pri_defn->ItemType() == cxx_base_class_spec)
								{
									// Replace the base class proxy with the real class and return it.
									TCppBaseClassSpec *base_class_proxy = (TCppBaseClassSpec*)pri_defn;
									compatible_object = base_class_proxy->m_base_class;
									return(TRUE);
								}

								// The discovered object is not good.
								ReportSyntaxError(err_ctx_sym, cerr_defns_search_not_named_type, name_info.m_cached_name);
								break;
							}

					case sym_PtrOperator:
							{
								if (pri_defn->ItemType() == cxx_struct_type)
								{
									// Return the discovered object.
									compatible_object = pri_defn;
									return(TRUE);
								}

								ReportSyntaxError(err_ctx_sym, cerr_ptr_operator_not_struct, name_info.m_cached_name);
								break;
							}

					case sym_BaseClassSpecifier:
							{
								if (pri_defn->ItemType() == cxx_base_class_spec)
								{
									// Replace the base class proxy with the real class.
									TCppBaseClassSpec *base_class_proxy = (TCppBaseClassSpec*)pri_defn;
									pri_defn = base_class_proxy->m_base_class;
								}

								if (pri_defn->ItemType() == cxx_struct_type)
								{
									// Return the discovered object.
									compatible_object = pri_defn;
									return(TRUE);
								}

								ReportSyntaxError(err_ctx_sym, cerr_context_not_class_name, name_info.m_cached_name);
								break;
							}

					case sym_TemplateTemplateParameter:
							{
								// This is picking up the default value of the template parameter that is a class template itself.
								if (name_info.IsTempalteId() == TRUE)
								{
									// Bogus template params. Ignore them.
									ReportSyntaxError(err_ctx_sym, cerr_defns_search_cttp_template_args);
								}

								if (pri_defn->IsClassTemplateFamily() == TRUE)
								{
									// Search has discovered a bucket of class templates. Make the overload resolution.
									// Note that procedure of picking up the list of template params is special here.
									assert(name_info.m_unproc_template_header != NULL && name_info.m_unproc_template_header->m_meaning == csm_template_params_list);
									TList *template_template_param_params = name_info.m_unproc_template_header->m_template_params_info.m_params_list;
									compatible_object = hlpr.FindClassTemplateOverload(name_info.KeyId(), template_template_param_params);
									if (compatible_object != NULL)
										return(TRUE);

									// Template params do not match.
									ReportSyntaxError(err_ctx_sym, cerr_defns_search_cttp_no_overload, name_info.m_cached_name);
									defn_resolution_problem = cdpr_unres;
								}
								else
								{
									// The name is not a class template at all.
									ReportSyntaxError(err_ctx_sym, cerr_defns_search_cttp_not_template, name_info.m_cached_name);
								}

								break;
							}

					case sym_PostfixExpression:
					case sym_PrimaryExpression:
							{
								// This is picking up an operand object or a field of the structured type.
								assert(compatible_defns_bucket != NULL && *compatible_defns_bucket == NULL);

								TCppItemType obj_type = pri_defn->ItemType();
								if (obj_type == cxx_enum_member || obj_type == cxx_base_class_spec || obj_type == cxx_data_field)
								{
									// These objects are fine for being operands.
									compatible_object = pri_defn;
									return(TRUE);
								}
								else if (pri_defn->IsFunctionFamily() == TRUE)
								{
									// This looks like a function call. If this a true, the params of the call are not even scanned yet.
									// Give out the whole bucket with overloads. The caller will resolve the potential overload.
									compatible_object = pri_defn;
									*compatible_defns_bucket = hlpr.m_defns_bucket;
									return(TRUE);
								}

								TTextBuffer256 name_buffer;
								if (dest_symbol_app_id == sym_PostfixExpression)
									ReportSyntaxError(err_ctx_sym, cerr_defns_search_not_class_member, name_info.m_cached_name, TCppDatabase::GenerateQualifiedName(name_buffer, name_info.m_resolved_defns_layer));
								else ReportSyntaxError(err_ctx_sym, cerr_defns_search_not_operand, TCppDatabase::GenerateQualifiedName(name_buffer, pri_defn));
								break;
							}

					default:
						{
							// Unexpected destination symbol.
							wchar_t loc_buff[80];
							swprintf(loc_buff, 80, L"LookupExistingDefinition for symbol app id: %d.", dest_symbol_app_id);
							RaiseNotImplementedError(loc_buff);
							return(FALSE);
						}
				}
			}
		}
		else
		{
			// Requested name is not present in any form.
			TCppErrorReportingCtxSymbolStateItem err_ctx(name_info);
			ReportSyntaxError(&err_ctx, cerr_defns_search_unknown_name, name_info.m_cached_name);
			defn_resolution_problem = cdpr_unres;
		}
	}

	// Step 2. Generate substitution for the failed resolution if needed.
	assert(compatible_object == NULL);
	if (dest_symbol_app_id != sym_UsingDirective && dest_symbol_app_id != sym_NamespaceAliasDefinition)
	{
		// An unresolved name object is needed.
		TCppItemType unres_object_type = cxx_none;
		switch (dest_symbol_app_id)
		{
			case sym_NamedTypeSpecifier:
					unres_object_type = cxx_unresolved_data_type;
					break;

			case sym_PtrOperator:
			case sym_BaseClassSpecifier:
					unres_object_type = cxx_unresolved_struct_type;
					break;

			case sym_TemplateTemplateParameter:
					unres_object_type = cxx_unresolved_class_template;
					break;

			case sym_PostfixExpression:
			case sym_PrimaryExpression:
					unres_object_type = cxx_unresolved_data_field;
					break;
		}

		assert(unres_object_type != cxx_none);
		compatible_object = CreateUnresolvedObject(name_info, defn_resolution_problem, unres_object_type);
		if (compatible_object == NULL)
		{
			RaiseCriticalCppError(ccerr_oom_creating_unres_object);
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

bool TCppParser::CheckUpdateParameterName(TSymbolStateItem *err_ctx_sym, TCppNameInfo &name_info)
{
	assert(err_ctx_sym != NULL);

	if (name_info.IsQualified() == TRUE)
	{
		// Name of the parameter contains a qualifying prefix.
		TCppErrorReportingCtxSymbolStateItem error_context(name_info, TRUE);
		ReportSyntaxError(&error_context, cerr_param_name_bad_qual_path);
		name_info.DowngradeToUnqualifiedNameInfo(this);
	}

	if (name_info.IsIdentifier() == FALSE)
	{
		// This is bogus form of the name.
		TCppErrorReportingCtxSymbolStateItem error_context(name_info, TRUE);
		ReportSyntaxError(&error_context, cerr_param_name_not_ident);
		if (name_info.DowngradeExpressionIdToIdentifier(this, L"Param") == FALSE)
			return(FALSE);
	}

	if (name_info.KeyId() != 0)
	{
		// The name was not killed by the code above. Check the uniqueness of the name.
		assert(m_curr_params_list != NULL);
		TList *prms_list = (m_curr_params_list->m_meaning == csm_func_params_list) ? m_curr_params_list->m_func_params_info.m_params_list : m_curr_params_list->m_template_params_info.m_params_list;
		if (prms_list != NULL)
		{
			for (TListIter<TCppDefnBase> iter(prms_list); iter; ++iter)
			{
				if (iter.CurrItem().DefnKeyId() == name_info.KeyId())
				{
					// This is names duplication.
					TCppErrorReportingCtxSymbolStateItem error_context(name_info, TRUE);
					ReportSyntaxError(&error_context, cerr_param_name_duplication);
					if (name_info.SetupShortNameDefnProblem(this, cdpr_dupe) == FALSE)
						return(FALSE);
				}
			}
		}
	}

	// Success.
	return(TRUE);
}

bool TCppParser::CheckPrepareTypeSpecifier(TCppSymbolStateItem &decl_type_spec_sym)
{
	assert(decl_type_spec_sym.m_meaning == csm_decl_type_spec);
	TCppDeclTypeSpecInfo &dts = decl_type_spec_sym.m_decl_type_spec_info;

	if (dts.m_dclt_base_type != NULL)
	{
		// The type specifier is already preprared.
		return(TRUE);
	}

	if (dts.m_dclt_blt_in_info.IsEmpty() == FALSE)
	{
		// Pick up the type id that was assembled out of the built in type specs so far.
		TCppBuiltInTypeIdent ident = dts.m_dclt_blt_in_info.GetBuiltInTypeIdent();

		// Pick up and assign the the type structure for this built in type. This will also publish this type
		// on the global namespace if this particular built in type was not used before.
		dts.m_dclt_base_type = GetBuiltInTypeDefn(ident);
	}
	else if (dts.m_dclt_unres_name_info.IsEmpty() == FALSE)
	{
		// It is necessary to resolve the name in the name info.
		TCppNameInfo &name_info = dts.m_dclt_unres_name_info;
		assert(name_info.IsIdentifier() == TRUE || name_info.IsTempalteId() == TRUE);

		if (dts.m_dclt_unres_name_elab == elab_none)
		{
			// This is named type specifier. Do generic lookup for this name.
			assert(dts.m_dclt_base_type == NULL);
			if (CheckLookupExistingDefinition(&decl_type_spec_sym, elab_none, name_info, sym_NamedTypeSpecifier, dts.m_dclt_base_type) == FALSE)
				return(FALSE);
		}
		else
		{
			// This is an unprocessed elaborated type specifier.
			dts.m_dclt_base_type = FindOrCreateElaboratedTypeObject(&decl_type_spec_sym, dts.m_dclt_unres_name_elab, name_info, FALSE, FALSE);

			if (dts.m_dclt_base_type != NULL)
			{
				// Link created of discovered object to the current place in the code.
				WriteObjectIntroEvent(&decl_type_spec_sym, clink_fwd_decl, dts.m_dclt_base_type,
									(TCppSourceAreaType)(cxa_enum_fwd_decl+dts.m_dclt_base_type->DefnElabType()-elab_enum), &name_info.m_short_name_context);
			}
		}
	}
	else if (dts.m_dclt_base_type == NULL)
	{
		// The base type is missing.
		ReportSyntaxError(&decl_type_spec_sym, cerr_base_type_missing);

		// Setup the type substitute.
		dts.m_dclt_base_type = GetBuiltInTypeDefn(bltin_id_void);
	}

	// Check results of the type resolution.
	if (dts.m_dclt_base_type == NULL)
	{
		// This is critical error. The error is already reported.
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

TCppDataTypeBase *TCppParser::CheckPrepareDataTypeFromTypeSpecifier(TCppSymbolStateItem &type_spec_sym)
{
	if (CheckPrepareTypeSpecifier(type_spec_sym) == FALSE)
		return(NULL);

	TCppDataTypeBase *base_data_type;
	TCppDeclTypeSpecInfo &dts = type_spec_sym.m_decl_type_spec_info;
	if (dts.m_dclt_base_type->IsFunctionType() == TRUE)
	{
		// Silently convert this function type into the pointer to this type. This will make it a data type.
		TCppDeclaratorModifier extra_ptr_spec1;
		extra_ptr_spec1.SetPointerSpec();
		base_data_type = CreateCvPtrRefModification(&type_spec_sym, dts.m_dclt_base_type, &extra_ptr_spec1, 1, TRUE, TRUE);
		if (base_data_type == NULL)
			return(NULL);
	}
	else
	{
		// The base type is already a data type.
		assert(dts.m_dclt_base_type->IsDataType() == TRUE);
		base_data_type = (TCppDataTypeBase*)dts.m_dclt_base_type;
	}

	// Check for the CV spec.
	if (dts.m_dclt_cv_spec.IsEmpty() == FALSE)
	{
		// Create a const/volatile modifier.
		TCppDeclaratorModifier extra_cv_spec2;
		extra_cv_spec2.SetCvprSpec(dts.m_dclt_cv_spec);
		base_data_type = CreateCvPtrRefModification(&type_spec_sym, base_data_type, &extra_cv_spec2, 1, TRUE, TRUE);
		if (base_data_type == NULL)
			return(NULL);
	}

	// Success.
	return(base_data_type);
}

void TCppParser::ApplyCvPtrRefModifier(TSymbolStateItem *err_ctx_sym, TCppDeclaratorInfo &decl_info, TCppCvPtrRefSpec &cvpr_info)
{
	// Ensure that layer, that is going to be created, will be not empty.
	assert(cvpr_info.IsEmpty() == FALSE);

	int mdf_inx = AllocateDeclaratorModifierLayer(err_ctx_sym, decl_info);
	if (mdf_inx < 0)
	{
		// Allocation failed. Non critical error is already reported.
		return;
	}

	// Fill in the layer. This type of modifer does not have any restrictions to check.
	TCppDeclaratorModifier &mdf_info = decl_info.m_modifiers->ItemRef(mdf_inx);
	mdf_info.m_dmt = dclmdf_cvpr;
	mdf_info.m_cvpr_spec = cvpr_info;
}

void TCppParser::ApplyMultiDimArrayModifier(TSymbolStateItem *err_ctx_sym, TCppDeclaratorInfo &decl_info, TCppExpressionBase *dims_list)
{
	if (dims_list != NULL && dims_list->m_operation == opr_comma)
	{
		// This is a list of several layers. Iterate these layers one by one.
		assert(dims_list->ItemType() == cxx_multiop_expr);
		TCppMultiOpExpr *exprs_list = (TCppMultiOpExpr*)dims_list;
		for (TListIter<TCppExpressionBase> iter(exprs_list->m_operands); iter; ++iter)
			ApplySingleDimArrayModifier(err_ctx_sym, decl_info, iter.CurrItemPtr());
	}
	else
	{
		// Simple case. Caller has passed just one layer of indexes.
		ApplySingleDimArrayModifier(err_ctx_sym, decl_info, dims_list);
	}
}

void TCppParser::ApplySingleDimArrayModifier(TSymbolStateItem *err_ctx_sym, TCppDeclaratorInfo &decl_info, TCppExpressionBase *dim_expr)
{
	// It is necessary to evaluate the passed expression. Note that error messages are written based of the type
	// of the expr (numeric or not numeric) and on if it is const expresssion or not. On the contrary the data model
	// in cpp database is more simple. Cpp database object stores only info the fact of if it was possible to get non
	// negative const numeric value or not. Reasons on why this constant is not available are not present there.
	bool numeric_type = FALSE;
	bool const_value = FALSE;
	__int64 dim_value = -1;

	TCppOperandExpr dim_operand;
	if (dim_expr == NULL)
	{
		// Consider this case as if zero value was given explicitly. NULL value is passed instead of expression when
		// array declaration contians empty square brackets.
		numeric_type = TRUE;
		const_value = TRUE;
		dim_value = 0;
	}
	else if (dim_expr->GetConstOperandValue(dim_operand) == FALSE)
	{
		// Passed expression is not constant. Check the type of this expression.
		TCppDefnBase *res_type = dim_expr->m_result_type;
		if (res_type != NULL && res_type->IsFunctionType() == FALSE && TCppDataTypeBase::GetUnderlyingNumericType((TCppDataTypeBase*)res_type, IsTemplateBodyContext()) != NULL)
		{
			// Expression has the numeric type or its type can be numeric in the context of template instantiation.
			numeric_type = TRUE;
		}
	}
	else
	{
		// This is constant expression.
		const_value = TRUE;

		// Check the value.
		 if (dim_operand.ConvertToNumericLiteral() == TRUE)
		{
			// Conversion to the numeric value went fine.
			numeric_type = TRUE;
			dim_value = dim_operand.m_num_value;
		}
	}

	// Allocate the modification description layer. The error message depends on the props of the modification layer.
	int mdf_inx = AllocateDeclaratorModifierLayer(err_ctx_sym, decl_info);
	if (mdf_inx < 0)
	{
		// Allocation failed. A non critical error is already reported.
		return;
	}

	// Check the results of expression evaluation. Note that positive numeric constant is always fine.
	if (numeric_type == FALSE)
	{
		// Type of expression should be numeric in all cases.
		ReportSyntaxError(err_ctx_sym, cerr_decltor_array_dim_not_numeric);
	}
	else if (const_value == FALSE)
	{
		// The value of the expr is not constant. In some of the cases this is still fine.
		if (IsTemplateContext() == FALSE)
		{
			// Current context is not template. Variable size is allowed only in the first dimension of the new declarator.
			if (mdf_inx > 0 || decl_info.m_new_expr_decl == FALSE)
				ReportSyntaxError(err_ctx_sym, cerr_decltor_array_dim_not_constant);
		}
	}
	else if (dim_value < 0)
	{
		// Explicit negative value cannot be accepted.
		ReportSyntaxError(err_ctx_sym, cerr_decltor_array_negative_dim, dim_value);
		if (dim_value < -1)
			dim_value = -1;
	}
	else if (dim_value == 0)
	{
		// Zero value of the size can be used is some cases.
		if (mdf_inx == 0)
		{
			// On the first layer of modifiers the zero value can be used only with parameters.
			if (IsParamsListContext() == FALSE)
				ReportSyntaxError(err_ctx_sym, cerr_decltor_array_zero_first_dim);
		}
		else if (decl_info.m_modifiers->ItemRef(mdf_inx-1).m_dmt == dclmdf_array)
		{
			// This is secondary layer of array modifiers.
			ReportSyntaxError(err_ctx_sym, cerr_decltor_array_zero_sec_dim);
		}
	}

	// Fill in the layer. Note that the layer is applied even if errors were reported.
	// -BUGBUG- Procedure for storing expression in the array dim defn is not written yet.
	decl_info.m_modifiers->ItemRef(mdf_inx).SetArrayDimSpec(dim_value);
}

void TCppParser::ApplyFuncParamsModifier(TSymbolStateItem *err_ctx_sym, TCppDeclaratorInfo &decl_info, TList *params_list, TCppSymbolStateItem *cv_qual_sym, TList *exceptions_list)
{
	// Check the type of the latest modifier if any.
	if (decl_info.m_modifiers != NULL && decl_info.m_modifiers->NumItems() > 0)
	{
		// Modifier is present. It is not allowed to create arrays of functions and function should not
		// return function (note that pointer to function is ok). Ensure this.
		TCppDeclaratorModifierType &latest_mdft = decl_info.m_modifiers->ItemRef(decl_info.m_modifiers->NumItems()-1).m_dmt;
		if (latest_mdft == dclmdf_array)
		{
			// Report the error and do not add the modifier.
			ReportSyntaxError(err_ctx_sym, cerr_decltor_array_elem_func);
			return;
		}
		else if (latest_mdft == dclmdf_function)
		{
			// Report the error and do not add the modifier.
			ReportSyntaxError(err_ctx_sym, cerr_decltor_func_retval_func);
			return;
		}
	}

	int mdf_inx = AllocateDeclaratorModifierLayer(err_ctx_sym, decl_info);
	if (mdf_inx < 0)
	{
		// Allocation failed. This is not a critical error.
		return;
	}

	// The modifier can be applied.
	TList local_specs_list;
	TCppDeclaratorModifier &mdf_info = decl_info.m_modifiers->ItemRef(mdf_inx);

	// Fill in the basic fields.
	mdf_info.m_dmt = dclmdf_function;
	if (params_list != NULL)
	{
		// Move param defns to the local list.
		local_specs_list.AppendList(*params_list);
	}

	if (cv_qual_sym != NULL)
	{
		// Qualifiers of the parameter "this" are passed. They should be not empty.
		assert(cv_qual_sym->m_ptr_operator.IsEmpty() == FALSE);
		assert(cv_qual_sym->m_ptr_operator.cvpr_pointer == FALSE && cv_qual_sym->m_ptr_operator.cvpr_reference == FALSE);

		mdf_info.m_func_spec.m_this_cv_spec = cv_qual_sym->m_ptr_operator;
		mdf_info.m_func_spec.m_this_cv_spec_context = cv_qual_sym->GetOriginHeader();
	}

	if (exceptions_list != NULL)
	{
		// Move exception specs to the local list.
		local_specs_list.AppendList(*exceptions_list);
	}

	if (local_specs_list.IsEmpty() == FALSE)
	{
		// Move contents of the local list into the modifier record. This will also transfer the ownership on this data.
		mdf_info.m_func_spec.m_params_beg = (TCppItemBase*)local_specs_list.GetFirst();
		mdf_info.m_func_spec.m_params_end = (TCppItemBase*)local_specs_list.GetLast();
		local_specs_list.Clear();
	}
}

TCppStructTypeDefn *TCppParser::FindOrCreateTemplateInstantiation(TSymbolStateItem *err_ctx_sym, TCppClassTemplateDefn *class_template, TList *&instantiation_arguments)
{
	// The template itself cannot be NULL, while the list of arguments can be NULL.
	assert(class_template != NULL);

	TCppDefnBase *class_template_container = (TCppDefnBase*)class_template->ItemParent();
	assert(class_template_container->IsNamespace() == TRUE || class_template_container->IsStructType() == TRUE);

	// Pick up the storage bucket where the template itself and all its instantiatios are stored.
	TCppDefnsSpace *space = class_template_container->GetNestedDefnsSpace();
	TCppDefnsBucket *overloads_bucket = space->GetNamedBucket(class_template->DefnKeyId());

	// -BUGBUG- Look for existing instantiation.

	// Appropriate instantiation was not found. Create a new one.
	TCppClassTemplateInstantiation *inst = (TCppClassTemplateInstantiation*)AllocateNewCppItem(cxx_class_template_inst);
	if (inst == NULL)
		return(NULL);

	// Fill in the props.
	inst->SetupStructType(class_template->DefnElabType());
	inst->SetNameInfo(class_template->DefnName(), class_template->DefnKeyId(), class_template->DefnProblem());
	inst->m_original_template = class_template;

	if (instantiation_arguments != NULL)
	{
		// Arguments are present. Move them into the instantiation object.
		while (instantiation_arguments->IsEmpty() == FALSE)
		{
			TCppItemBase *item = (TCppItemBase*)(instantiation_arguments->GetFirst());
			instantiation_arguments->RemoveItem(item);
			inst->AddChildItem(item, cdefn_chdest_cltplt_iprm);
		}

		// Release the arguments list itself.
		RecycleGeneralPurposeList(instantiation_arguments);
		instantiation_arguments = NULL;
	}

	// Publish this instantiation. This is special case because all instantiations of the template are published
	// in the same container where the template itself is defined.
	class_template_container->AddChildItem(inst);

	// Success.
	return(inst);
}

TCppLabelDefn *TCppParser::FindOrCreateNamedLabel(TCppNameInfo &name_info, bool goto_stmt_context)
{
	// This function should be called only from the context of the block.
	assert(m_defns_layer_owner->IsBlock() == TRUE);

	// Iterate all symbols of the stack starting from its top up to the border of the current function body.
	// Check symbols that may own cpp database items that are not yet published in the body of the function.
	TCppSymbolStateItem *sym = LastStackSymPtr();
	TCppSymbolStateItem *layer_symbol = m_defns_layer_symbol;
	while (sym != NULL)
	{
		TCppItemBase *pri_container = NULL, *sec_container = NULL;
		GetLabelContainersFromSymbol(sym, pri_container, sec_container);

		if (pri_container != NULL)
		{
			TCppLabelDefn *existing_label = pri_container->FindNamedLabelHier(name_info.KeyId());
			if (existing_label != NULL)
				return(existing_label);
		}

		if (sec_container != NULL)
		{
			TCppLabelDefn *existing_label = sec_container->FindNamedLabelHier(name_info.KeyId());
			if (existing_label != NULL)
				return(existing_label);
		}

		if (sym == layer_symbol)
		{
			// Iteration has just processed the header of the current definitions layer. Check the type
			// of the upper layer. If the upper layer is not a block, the iteration should be stopped.
			if (layer_symbol->m_parent_layer_owner->IsBlock() == FALSE)
				break;

			// Setup the new iteration limit.
			layer_symbol = layer_symbol->m_parent_layer_symbol;
			if (layer_symbol == NULL)
			{
				// The situation is bad.
				assert(FALSE);
				break;
			}
		}

		// Shift to the next symbol up the stack of the parser.
		sym = PrevRuleSymPtr(sym);
		assert(sym != NULL);
	}

	// Check the list of the undefined labels.
	for (TListIter<TCppLabelDefn> iter(m_unplaced_labels); iter; ++iter)
	{
		if (iter.CurrItem().DefnKeyId() == name_info.KeyId())
		{
			// Label with the requested name is available.
			TCppLabelDefn *existing_label = iter.CurrItemPtr();
			if (goto_stmt_context == FALSE)
			{
				m_unplaced_labels.RemoveItem(existing_label);
				existing_label->m_defined = TRUE;
			}

			return(existing_label);
		}
	}

	// There is no label with this name. Create new one.
	TCppLabelDefn *new_label = (TCppLabelDefn*)AllocateNewCppItem(cxx_label);
	if (new_label == NULL)
		return(NULL);

	name_info.SetNameToObject(new_label);
	if (goto_stmt_context == TRUE)
	{
		// This label will be referenced in the goto statement but it does not have any location in the code yet.
		m_unplaced_labels.AppendItem(new_label);
	}
	else
	{
		// The label was created while processing the labeled statement.
		new_label->m_defined = TRUE;
	}

	// Success.
	return(new_label);
}

void TCppParser::CheckReportCaseLabelProblems(TSymbolStateItem *err_ctx_sym, TCppCaseLabelDefn *case_label)
{
	// Ensure that current context is context of the function body.
	assert(m_defns_layer_owner->IsBlock() == TRUE);

	// Iterate all symbols of the stack starting from its top (the right most symbol) up to the SWITCH
	// keyword or to the border of the current function body.
	TCppSymbolStateItem *sym = LastStackSymPtr();
	TCppSymbolStateItem *layer_symbol = m_defns_layer_symbol;
	while (sym != NULL)
	{
		TCppItemBase *pri_container = NULL, *sec_container = NULL;
		GetLabelContainersFromSymbol(sym, pri_container, sec_container);

		if (pri_container != NULL)
			CheckReportCaseLabelDuplication(err_ctx_sym, pri_container, case_label);
		if (sec_container != NULL)
			CheckReportCaseLabelDuplication(err_ctx_sym, sec_container, case_label);

		if (sym->m_app_id == sym_switch)
		{
			// The switch symbol is present. This means that this case label stays in the right context.
			break;
		}

		if (sym == layer_symbol)
		{
			// Iteration has just processed the header of the current definitions layer. Check the type
			// of the upper layer. If the upper layer is not a block, the iteration should be stopped.
			if (layer_symbol->m_parent_layer_owner->IsBlock() == FALSE)
			{
				// There is no SWITCH symbol up the stack.
				ReportSyntaxError(err_ctx_sym, cerr_case_label_outside_switch);
				break;
			}

			// Setup the new iteration limit.
			layer_symbol = layer_symbol->m_parent_layer_symbol;
			if (layer_symbol == NULL)
			{
				// The situation is bad.
				assert(FALSE);
				break;
			}
		}

		// Shift to the next symbol up the stack of the parser.
		sym = PrevRuleSymPtr(sym);
		assert(sym != NULL);
	}
}

void TCppParser::GetLabelContainersFromSymbol(TCppSymbolStateItem *sym, TCppItemBase *&pri_container, TCppItemBase *&sec_container)
{
	pri_container = NULL;
	sec_container = NULL;

	// Only some of the symbol meanings can contain database item subtrees that can contain labels.
	switch (sym->m_meaning)
	{
		case csm_statement:				pri_container = sym->m_statement;
										break;
		case csm_catch_blocks_seq:		pri_container = sym->m_catch_blocks_seq;
										break;
		case csm_catch_block:				pri_container = sym->m_catch_block;
										break;
		case csm_block_header:
				{
					if (sym->m_block_info.m_block_object_published == FALSE || sym->m_parent_layer_owner->IsBlock() == FALSE)
					{
						// This block is not published or it is is the body of the function.
						pri_container = sym->m_block_info.m_block_header;
					}
				}
				break;

		case csm_do_statement_body:
				{
					// This symbol can have two containers at the same time. REturn them both.
					pri_container = sym->m_do_stmt_info.m_do_stmt_loop_body;
					sec_container = sym->m_do_stmt_info.m_do_stmt_block_header;
				}
				break;
	}
}

void TCppParser::CheckReportCaseLabelDuplication(TSymbolStateItem *err_ctx_sym, TCppItemBase *container, TCppCaseLabelDefn *case_label)
{
	assert(container != NULL);

	TCppCaseLabelDefn *other_label = container->FindCaseLabelHier(case_label);
	if (other_label != NULL)
	{
		// Label withe the same props is existing. Report this as an error.
		if (case_label->m_default == TRUE)
			ReportSyntaxError(err_ctx_sym, cerr_case_label_dupl_default);
		else ReportSyntaxError(err_ctx_sym, cerr_case_label_dupl_value, case_label->m_value);
	}
}

TCppUnresolvedNameInfo *TCppParser::CreateUnresNameInfo(TCppNameInfo &info)
{
	TCppUnresolvedNameInfo *unres_name_info = (TCppUnresolvedNameInfo*)AllocateNewCppItem(cxx_unresolved_name_info);
	if (unres_name_info == NULL)
		return(NULL);

	// Allocation succeeded. Setup the name.
	info.SetNameToObject(unres_name_info);

	// Fill in the props. These props describe only the final part of the passed name info.
	// The resolved/unresolved layer info is not included into the returned object.
	unres_name_info->m_node_type = info.m_core.m_info_type;
	unres_name_info->m_operator_token = info.m_core.m_ovld_operator_token;
	unres_name_info->m_conversion_dest_type = info.m_core.m_ovld_conversion_dest;

	if (info.m_template_args_list != NULL)
	{
		unres_name_info->AddListOfChildren(*info.m_template_args_list);
		RecycleGeneralPurposeList(info.m_template_args_list);
		info.m_template_args_list = NULL;
	}

	// Success.
	return(unres_name_info);
}

TCppDefnBase *TCppParser::CreateUnresolvedObject(TCppNameInfo &name_info, TCppDefnProblem unintegrated_problem, TCppItemType item_type)
{
	// Create name info object for the final part of the name.
	assert(name_info.m_preferred_object_id == 0);
	TCppUnresolvedNameInfo *final_name_info = CreateUnresNameInfo(name_info);
	if (final_name_info == NULL)
		return(NULL);

	// Procedures for resolved and unresoolved layers are very different.
	TCppDefnBase *unres_object;
	TCppDefnBase *defns_layer = name_info.GetDefnsLayer(m_defns_layer_owner);
	if (name_info.IsUnresolvedLayer() == TRUE)
	{
		// Unresolved names are already available. Add final name info to this list.
		assert(unintegrated_problem == cdpr_none);
		name_info.m_unresolved_names_list->AppendItem(final_name_info);
		final_name_info = NULL;

		// Check for existing unresolved object using a multiname procedure.
		TCppDefnBase *existing_object = defns_layer->GetNestedDefnsSpace()->LookupUnresolvedObject(item_type, name_info.m_unresolved_names_list, name_info.m_name_problem);
		if (existing_object != NULL)
			return(existing_object);

		// Object with the required props is not existing.
		if (name_info.FinalizeLayerResProblemName(this) == FALSE)
			return(NULL);

		unres_object = (TCppDefnBase*)AllocateNewCppItem(item_type, name_info.m_preferred_object_id);
		if (unres_object == NULL)
			return(NULL);

		// Add the complete seq of available unresolved name infos.
		unres_object->AddListOfChildren(*name_info.m_unresolved_names_list);
		RecycleGeneralPurposeList(name_info.m_unresolved_names_list);
		name_info.m_unresolved_names_list = NULL;
	}
	else
	{
		// Check for identical existing object using a single name procedure.
		assert(name_info.m_name_problem == cdpr_none && unintegrated_problem != cdpr_none);
		TCppDefnBase *existing_object = defns_layer->GetNestedDefnsSpace()->LookupUnresolvedObject(item_type, final_name_info, unintegrated_problem);
		if (existing_object != NULL)
		{
			delete final_name_info;
			return(existing_object);
		}

		// Object is not existing. Integrate passed problem into the name info.
		if (name_info.SetupShortNameDefnProblem(this, unintegrated_problem) == FALSE)
		{
			delete final_name_info;
			return(NULL);
		}

		unres_object = (TCppDefnBase*)AllocateNewCppItem(item_type, name_info.m_preferred_object_id);
		if (unres_object == NULL)
		{
			delete final_name_info;
			return(NULL);
		}

		// Add just one unres name info.
		unres_object->AddChildItem(final_name_info);
		final_name_info = NULL;
	}

	// Setup the name and add this object to the names space.
	name_info.SetNameToObject(unres_object);
	PublishOnTheSpecificLayer(defns_layer, unres_object);

	// Success.
	return(unres_object);
}

TCppStructTypeDefn *TCppParser::FindOrCreateStructTypeObject(TCppElabType elab_type, TCppSymbolStateItem *sym_class_name, TCppSymbolStateItem *sym_base_classes)
{
	assert(elab_type != elab_none && sym_class_name != NULL);

	// This method expects to receive a valid name symbol with an identifier in it (maybe qualified).
	TCppNameInfo &name_info = sym_class_name->m_simp_or_qual;
	assert(name_info.IsIdentifier() == TRUE && name_info.KeyId() != 0);

	// There should be no template headers. Otherwise other method should be called.
	assert(name_info.m_unproc_template_header == NULL);

	TCppDefnBase *existing_forward_decl_object = NULL;
	if (CheckLookupElabTypeDefnName(elab_type, sym_class_name, cxx_struct_type, existing_forward_decl_object) == FALSE)
		return(NULL);

	// Check the results of the search.
	TCppStructTypeDefn *struct_object = NULL;
	if (existing_forward_decl_object == NULL)
	{
		// Appropriate forward declaration is not available. Create new struct type object.
		struct_object = (TCppStructTypeDefn*)AllocateNewCppItem(cxx_struct_type, name_info.m_preferred_object_id);
		if (struct_object == NULL)
			return(NULL);

		// Setup the name and other props.
		name_info.SetNameToObject(struct_object);
		struct_object->SetupStructType(elab_type);
		struct_object->m_defined = TRUE;
	}
	else
	{
		// Accept the available struct/union/class forward declaration.
		assert(existing_forward_decl_object->ItemType() == cxx_struct_type);
		struct_object = (TCppStructTypeDefn*)existing_forward_decl_object;
		struct_object->m_defined = TRUE;
	}

	// The struct type object is created. Add the base classes if any.
	if (sym_base_classes != NULL)
		struct_object->AddBaseClassesList(sym_base_classes->m_base_classes_list);

	// Success.
	return(struct_object);
}

TCppStructTypeDefn *TCppParser::FindOrCreateClassTemplateObject(TCppElabType elab_type, TCppSymbolStateItem *sym_class_name, TCppSymbolStateItem *sym_base_classes)
{
	assert(elab_type != elab_none && sym_class_name != NULL);

	// This method expects to receive a valid name symbol with an identifier in it (maybe qualified).
	TCppNameInfo &name_info = sym_class_name->m_simp_or_qual;
	assert(name_info.IsIdentifier() == TRUE && name_info.KeyId() != 0);

	// There should be an unprocessed template header. Otherwise other method should be called.
	assert(name_info.m_unproc_template_header != NULL);
	name_info.m_unproc_template_header->m_template_info.m_header_processed = TRUE;

	TList *header_params_list = name_info.TemplateHeaderParams();
	if (header_params_list == NULL || header_params_list->NumItems() == 0)
	{
		// The template keyword is present while the template params are missing.
		ReportSyntaxError(sym_class_name, cerr_struct_template_params_missing, TCppDefnBase::GetElabTypeName(elab_type, FALSE));
	}

	if (name_info.m_unproc_template_header->m_template_info.m_child_template_header != NULL)
		ReportUnprocessedTemplateHeaders(name_info.m_unproc_template_header->m_template_info.m_child_template_header);

	TCppDefnBase *existing_forward_decl_object = NULL;
	if (CheckLookupElabTypeDefnName(elab_type, sym_class_name, cxx_class_template, existing_forward_decl_object) == FALSE)
		return(NULL);

	// Check the results of the search.
	TCppClassTemplateDefn *class_template = NULL;
	if (existing_forward_decl_object == NULL)
	{
		// Appropriate forward declaration is not available. Create new class template object.
		class_template = (TCppClassTemplateDefn*)AllocateNewCppItem(cxx_class_template, name_info.m_preferred_object_id);
		if (class_template == NULL)
			return(NULL);

		// Setup in the name that might be modified in case of the names collision or other error and other props.
		name_info.SetNameToObject(class_template);
		class_template->SetupStructType(elab_type);
		class_template->m_defined = TRUE;
	}
	else
	{
		// Accept discovered forward declaration of the class template.
		assert(existing_forward_decl_object->ItemType() == cxx_class_template);
		class_template = (TCppClassTemplateDefn*)existing_forward_decl_object;
		class_template->m_defined = TRUE;
	}

	if (class_template->IsPublished() == FALSE)
	{
		// Check and process the mandatory template params.
		if (header_params_list != NULL && header_params_list->IsEmpty() == FALSE)
		{
			// Move template params into the template definition.
			class_template->AddListOfChildren(*header_params_list);
		}
	}

	// Add the base classes if any.
	if (sym_base_classes != NULL)
		class_template->AddBaseClassesList(sym_base_classes->m_base_classes_list);

	// Success.
	return(class_template);
}

TCppStructTypeDefn *TCppParser::FindOrCreateClassTemplateSpecObject(TCppElabType elab_type, TCppSymbolStateItem *sym_class_name, TCppSymbolStateItem *sym_base_classes)
{
	assert(elab_type != elab_none && sym_class_name != NULL);
	TCppNameInfo &name_info = sym_class_name->m_simp_or_qual;
	assert(name_info.IsTempalteId() == TRUE && name_info.KeyId() != 0);

	// Check the template header. All types of template headers are possible here.
	const  wchar_t *elab_type_name = TCppDefnBase::GetElabTypeName(elab_type);
	if (name_info.m_unproc_template_header == NULL)
	{
		ReportSyntaxError(sym_class_name, cerr_templ_spec_no_templ_header, elab_type_name);
	}
	else
	{
		if (name_info.TemplateHeaderParams() == NULL)
			ReportSyntaxError(sym_class_name, cerr_templ_spec_expl_inst_header, elab_type_name);
		if (name_info.m_unproc_template_header->m_template_info.m_child_template_header != NULL)
			ReportUnprocessedTemplateHeaders(name_info.m_unproc_template_header->m_template_info.m_child_template_header);
	}

	// Look for the base template.
	/* -BUGBUG-
	TCppDefnBase *class_template = NULL;
	if (CheckLookupExistingDefinition(sym_class_name, elab_type, sym_class_name->m_simp_or_qual, sym_ClassHeader, class_template) == FALSE)
		return(NULL);
	*/

	if (sym_base_classes != NULL)
	{
		// Class template specialization should not have any base classes.
		ReportSyntaxError(sym_class_name, cerr_templ_spec_base_classes, elab_type_name);
	}

	// Return failure for now.
	RaiseNotImplementedError(L"Class template specialization");
	return(NULL);
}

TCppDataTypeBase *TCppParser::FindOrCreateElaboratedTypeObject(TSymbolStateItem *err_ctx_sym, TCppElabType elab_type, TCppNameInfo &name_info, bool stand_alone_dts_context, bool friend_flag)
{
	assert(elab_type != elab_none);
	assert(name_info.IsIdentifierOrTempalteId() == TRUE);

	// Step 1. Figure out the type of object that should be picked up or created and check the params.
	TCppItemType dest_item_type = cxx_none;
	if (elab_type == elab_enum)
	{
		// This is forward declaration of the enum type.
		dest_item_type = cxx_enum_type;
		if (name_info.IsTempalteId() == TRUE)
		{
			// Name of the forward declaration of the enum type contains the template arguments.
			ReportSyntaxError(err_ctx_sym, cerr_elab_fwd_enum_templid);
			name_info.DowngradeTemplateIdToIdentifier(this);
		}
	}
	else
	{
		// This is forward declaration of the struct/union/class/template/tempate_spec/template_inst.
		TList *header_params_list = name_info.TemplateHeaderParams();

		// Check for special case where template args are straight repetition of params from the template header.
		name_info.CheckRemoveTemplateDefnTemplateArgs(this);

		if (name_info.IsIdentifier() == TRUE)
		{
			if (name_info.m_unproc_template_header == NULL)
			{
				 // This is a simple struct type.
				 dest_item_type = cxx_struct_type;
			}
			else
			{
				// This is request to process the class template definition.
				dest_item_type = cxx_class_template;
				if (header_params_list == NULL || header_params_list->IsEmpty() == TRUE)
				{
					// The template keyword is present while the template params are missing.
					ReportSyntaxError(err_ctx_sym, cerr_elab_fwd_templ_params_missing, TCppDefnBase::GetElabTypeName(elab_type, FALSE));
				}
			}
		}
		else
		{
			if (header_params_list == NULL)
			{
				// This is request to instantiate a class template.
				dest_item_type = cxx_class_template_inst;
				RaiseNotImplementedError(L"Explicit template instantiation");
				return(NULL);
			}
			else
			{
				// This is request to process a class template spec.
				dest_item_type = cxx_class_template_spec;
				RaiseNotImplementedError(L"Forward declaration of the class template specialization");
				return(NULL);
			}
		}
	}

	assert(dest_item_type != cxx_none);
	assert(name_info.IsIdentifier() == TRUE);
	if (stand_alone_dts_context == TRUE && name_info.m_unproc_template_header != NULL)
	{
		// Purpose and validity of this template header is not verified yet. To keep the code simple
		// mark this header as processed right now instead of doing basically the same thing in front
		// of every return statement below.
		name_info.m_unproc_template_header->m_template_info.m_header_processed = TRUE;
	}

	// Step 2. Handle the layer resolution problem. In this case there is no need to do any name lookup.
	if (name_info.IsUnresolvedLayer() == TRUE)
	{
		// In the moment the cashed name in the name info contains the final name of the qual seq.
		// Generate full unresolved name and place it into the cashed name of the name info.
		if (name_info.FinalizeLayerResProblemName(this) == FALSE)
			return(NULL);

		// Generate forward declaration using the generated name.
		return(CreateAndPublishElabFwdDecl(name_info.GetDefnsLayer(m_defns_layer_owner), elab_type, name_info, dest_item_type));
	}

	// Step 3. Select the defns layer for creating the new forward declaration.
	TCppDefnBase *new_fwd_decl_layer;
	if (name_info.m_resolved_defns_layer != NULL)
	{
		// Name info contains an explicit layer. Use this layer.
		new_fwd_decl_layer = name_info.m_resolved_defns_layer;
	}
	else if (friend_flag == TRUE)
	{
		// The name in the name info is short. This was verified a few lines above. Adjust passed current layer
		// to the closest upper namespace layer. Note that caller is responsible for passing active friend flag only
		// when the FIEND spec stays on the struct layer.
		new_fwd_decl_layer = m_defns_layer_owner->GetNamespaceParent();
		assert(new_fwd_decl_layer != NULL);
	}
	else
	{
		// It is ok to use the current layer.
		new_fwd_decl_layer = m_defns_layer_owner;
	}

	// Step 4. Passed name belongs to some existing layer. Do the existing name search if the context of
	// the caller expects this. Look for more details on this in the comments to the prototype of this method.
	if (stand_alone_dts_context == FALSE || friend_flag == TRUE)
	{
		TCppDefnProblem existing_defns_search_problem = cdpr_none;
		TCppDefnBase *existing_defns_search_layer = name_info.GetDefnsLayer(m_defns_layer_owner);

		TCppExistingDefnsSearchHelper hlpr(existing_defns_search_layer);
		if (hlpr.DoSearch(elab_type, name_info.m_core, name_info.GetSearchHierFlag()) == TRUE)
		{
			// Something is present.
			if (hlpr.IsAmbigResult() == TRUE)
			{
				// Passed name is ambiguous.
				if (hlpr.m_curr_search_layer->IsChildOf(new_fwd_decl_layer) == TRUE)
				{
					ID main_error_id = ReportSyntaxError(err_ctx_sym, cerr_elab_fwd_find_name_ambig, hlpr.m_ambig_defns.NumItems());
					ReportAmbiguousDefnsArray(err_ctx_sym, main_error_id, hlpr.m_ambig_defns);
					existing_defns_search_problem = cdpr_ambig;
				}
			}
			else if (hlpr.m_single_defn->ItemType() == cxx_enum_type && dest_item_type == cxx_enum_type)
			{
				// Pick up the discovered object.
				return((TCppDataTypeBase*)hlpr.m_single_defn);
			}
			else if (hlpr.m_single_defn->ItemType() == cxx_struct_type && dest_item_type == cxx_struct_type)
			{
				// Pick up the discovered object.
				return((TCppDataTypeBase*)hlpr.m_single_defn);
			}
			else if (hlpr.m_single_defn->IsClassTemplateFamily() == TRUE && dest_item_type == cxx_class_template)
			{
				// Passed name corresponds to some class template. Resolve the overload.
				TCppClassTemplateDefn *class_template_overload =  hlpr.FindClassTemplateOverload(name_info.KeyId(), name_info.TemplateHeaderParams());
				if (class_template_overload != NULL)
				{
					// Pick up the discovered class template.
					return(class_template_overload);
				}
			}
			else
			{
				// Passed name describes some incompatible object.
				if (hlpr.m_curr_search_layer->IsChildOf(new_fwd_decl_layer) == TRUE)
				{
					wchar_t local_buff80[80];
					ReportSyntaxError(err_ctx_sym, cerr_elab_fwd_names_conflict, name_info.m_cached_name,
									TCppDefnBase::GetObjectTypeFriendlyName(local_buff80, dest_item_type, elab_type, FALSE));
					existing_defns_search_problem = cdpr_conflict;
				}
			}
		}

		if (existing_defns_search_problem != cdpr_none)
		{
			// Integrate discovered problem into the name.
			if (name_info.SetupShortNameDefnProblem(this, existing_defns_search_problem) == FALSE)
				return(NULL);

			// Generate forward decl using the generated name.
			return(CreateAndPublishElabFwdDecl(hlpr.m_curr_search_layer, elab_type, name_info, dest_item_type));
		}
	}

	// Step 5.  Search for existing defns was not needed or it has not discovered anything. Check the type
	// of selected defns layer and its contents.
	TCppDataTypeBase *compatible_elaborated_type_object = NULL;
	if (CheckLookupElabTypeSpecName(new_fwd_decl_layer, elab_type, name_info, dest_item_type, compatible_elaborated_type_object) == FALSE)
		return(NULL);

	if (compatible_elaborated_type_object != NULL)
	{
		// Required elab type object is already existing.
		return(compatible_elaborated_type_object);
	}

	// Create new forward declaration.
	return(CreateAndPublishElabFwdDecl(new_fwd_decl_layer, elab_type, name_info, dest_item_type));
}

TCppDefnBase *TCppParser::FindUnambigNonOverloadableDefnByName(const wchar_t **names_seq, TCppDefnProblem &discovered_problem)
{
	assert(names_seq != NULL && names_seq[0] != NULL && discovered_problem == cdpr_none);

	// The search always starts from the global namespace.
	TCppDefnBase *curr_layer = &m_cpp_database.GetGlobalNamespace();
	TCppDefnBase *unambig_object = NULL;
	for (;;)
	{
		// Pick up the key id of the current name.
		TStringPtr name_info(names_seq[0]);
		TKeyInfoItem *key_info = m_scanner->LookupIndexingKey(name_info);
		if (key_info == NULL)
		{
			// Current name element does not have a key id.
			discovered_problem = cdpr_unres;
			return(NULL);
		}

		// The name is known. Pick up the bucket for this name and check its contents.
		TCppDefnsBucket *slot = curr_layer->GetNestedDefnsSpace()->GetNamedBucket(key_info->data.key_id);

		unambig_object = NULL;
		for (TCppDefnsSpaceBucketKeyIdIter<TCppDefnBase> iter(slot, key_info->data.key_id); iter; ++iter)
		{
			TCppDefnBase &item = iter.Item();
			if (item.IsFunctionName() == TRUE || item.ItemType() == cxx_base_class_spec || item.IsClassTemplateFamily() == TRUE)
			{
				// Current object can be overloadable. This is not what is needed.
				discovered_problem = cdpr_unapprop;
				return(NULL);
			}
			else if (unambig_object != NULL)
			{
				// This is ambiguity between the different types of elaboration.
				assert(item.DefnElabType() != unambig_object->DefnElabType());
				discovered_problem = cdpr_ambig;
				return(NULL);
			}

			// Accept the discovered object.
			unambig_object = &item;
		}

		if (unambig_object == NULL)
		{
			discovered_problem = cdpr_unres;
			return(NULL);
		}

		// Check the condition of the outer loop.
		names_seq++;
		if (names_seq[0] == NULL)
			break;

		// Curr name is not the final name. Only the names of the layers are good.
		if (unambig_object == NULL || (unambig_object->ItemType() != cxx_namespace && unambig_object->ItemType() != cxx_struct_type))
		{
			discovered_problem = cdpr_unapprop;
			return(NULL);
		}

		// Accept the discovered object as the new layer object and continue iteration.
		curr_layer = unambig_object;
	}

	// Success. The result of the search is non empty.
	return(unambig_object);
}

TCppDataFieldDefn *TCppParser::CreateDataParamObject(TCppSymbolStateItem &type_spec_sym, bool declarator_present)
{
	ID param_name_key_id = 0;
	TCppDefnProblem param_name_problem = cdpr_none;
	ID param_object_preferred_id = 0;
	wchar_t *cached_param_name = NULL;

	TCppDataTypeBase *data_type = NULL;
	TCppSymbolStateItem *err_ctx_sym = NULL;

	if (declarator_present == FALSE)
	{
		// Only the type specifier is present in the rule.
		data_type = CheckPrepareDataTypeFromTypeSpecifier(type_spec_sym);
		err_ctx_sym = &type_spec_sym;
	}
	else
	{
		// Finalize the type specifier.
		if (CheckPrepareTypeSpecifier(type_spec_sym) == FALSE)
			return(NULL);

		// Pick up declarator from the second symbol of the rule.
		TCppSymbolStateItem &sym_declarator = NextRuleSym(type_spec_sym);
		assert(sym_declarator.m_meaning == csm_declarator);
		TCppNameInfo &decl_name_info = sym_declarator.m_declarator_info.m_name_info;

		if (sym_declarator.m_declarator_info.IsAbstractDeclarator() == FALSE)
		{
			// Check/fix validity and possible duplication of the parameter name.
			if (CheckUpdateParameterName(&sym_declarator, decl_name_info) == FALSE)
				return(NULL);
		}

		// Assemble the type of the parameter.
		TCppDeclTypeSpecInfo &dts = type_spec_sym.m_decl_type_spec_info;
		data_type = (TCppDataTypeBase*)AssembleDeclaratorType(&type_spec_sym, FALSE, dts.m_dclt_cv_spec, dts.m_dclt_base_type, sym_declarator);
		err_ctx_sym = &sym_declarator;

		// Name related fields are fine both for ordinary (named) and abstract (nameless) declarators.
		param_name_key_id = decl_name_info.KeyId();
		param_name_problem = decl_name_info.m_name_problem;
		param_object_preferred_id = decl_name_info.m_preferred_object_id;
		cached_param_name = decl_name_info.m_cached_name;
	}

	// Check for critical error.
	assert(data_type == NULL || data_type->IsDataType() == TRUE);
	if (data_type == NULL)
		return(NULL);

	// Check for the array type. Once it is present, it should be converted into the pointer.
	if (data_type->ItemType() == cxx_array_type)
	{
		TCppDeclaratorModifier extra_ptr_spec;
		extra_ptr_spec.SetPointerSpec();

		bool named_data_type = (data_type == type_spec_sym.m_decl_type_spec_info.m_dclt_base_type) ? TRUE : FALSE;
		data_type = CreateCvPtrRefModification(err_ctx_sym, data_type, &extra_ptr_spec, 1, TRUE, named_data_type);
		if (data_type == NULL)
			return(NULL);
	}

	// Create the data field object for this param.
	TCppDataFieldDefn *new_param = (TCppDataFieldDefn*)AllocateNewCppItem(cxx_data_field, param_object_preferred_id);
	if (new_param == NULL)
		return(NULL);

	// Fill in the props. Param index will be set up later. Note that in case of the abstract declarator
	// the name of the parameter is set as NULL and its key id as 0.
	new_param->SetNameInfo(cached_param_name, param_name_key_id, param_name_problem);
	new_param->m_strg_class = strgc_param;
	new_param->m_field_type = data_type;

	// Success.
	return(new_param);
}

TCppDataFieldDefn *TCppParser::CreateDataFieldObject(TCppDefnBase *dest_defns_layer, TCppNameInfo &name_info, TCppStorageClass strg_class, TCppDataTypeBase *data_type)
{
	assert(dest_defns_layer != NULL && data_type != NULL);

	// Create the data field object.
	TCppDataFieldDefn *new_data_field = (TCppDataFieldDefn*)AllocateNewCppItem(cxx_data_field, name_info.m_preferred_object_id);
	if (new_data_field == NULL)
		return(NULL);

	// Assign the prepared name.
	name_info.SetNameToObject(new_data_field);

	if (name_info.m_name_problem == cdpr_none)
	{
		// Fill in the linkage props.
		if (m_curr_linkage_spec != NULL)
			new_data_field->m_linkage_spec = m_curr_linkage_spec->m_linkage_spec.m_link_spec;

		// Setup the access level.
		new_data_field->m_accs_level = GetCurrentAccessLevel();
	}

	// Check/update the storage class.
	if (strg_class == strgc_static)
	{
		if (dest_defns_layer->IsNamespace() == TRUE)
			new_data_field->m_visibility_pars_sess = GetParsingId();
		if (dest_defns_layer->IsStructType() == TRUE)
			strg_class = strgc_extern;
	}
	else if (strg_class == strgc_mutable)
	{
		// This function expects that validity of passing of this storage class is already verified.
		new_data_field->m_mutable = TRUE;
		strg_class = strgc_none;
	}

	// Fill in the storage class and type.
	new_data_field->m_strg_class = strg_class;
	new_data_field->m_field_type = data_type;

	// Publish the object if needed.
	if (dest_defns_layer != m_defns_layer_owner)
		PublishOnTheSpecificLayer(dest_defns_layer, new_data_field);

	// Success.
	return(new_data_field);
}

TCppCodeEntryPointDefn *TCppParser::CreateFunctionPrototype(TCppDefnBase *dest_defns_layer, TCppFunctionAttr func_attrs, TCppNameInfo &name_info, TCppDataTypeBase *ret_value_type, TCppDeclaratorModifier *unapplied_func_modifier)
{
	assert(name_info.TemplateArgsPresent() == FALSE);

	// Create either a code entry point object or a function template object.
	TCppItemType func_obj_type = (name_info.m_unproc_template_header == NULL) ? cxx_code_entry_point : cxx_func_template;
	TCppCodeEntryPointDefn *func_object = (TCppCodeEntryPointDefn*)AllocateNewCppItem(func_obj_type, name_info.m_preferred_object_id);
	if (func_object == NULL)
		return(NULL);

	// Assign the prepared name.
	name_info.SetNameToFunctionObject(func_object);
	if (name_info.m_core.m_info_type != cnit_identifier)
	{
		// This is special function. Setup its spec type.
		if (name_info.m_core.m_info_type == cnit_constructor)
			func_object->m_spec_type = spcft_ctor;
		else if (name_info.m_core.m_info_type == cnit_dector)
			func_object->m_spec_type = spcft_dector;
		else if (name_info.m_core.m_info_type == cnit_ovld_operator)
			func_object->m_spec_type = spcft_operator;
		else if (name_info.m_core.m_info_type == cnit_ovld_conversion)
			func_object->m_spec_type = spcft_typecast;

		assert(func_object->m_spec_type != spcft_none);
	}

	if (dest_defns_layer->IsNamespace() == TRUE && (func_attrs & fcta_static) != 0)
	{
		// This function should be visible only in the current parsing session.
		func_object->m_visibility_pars_sess = GetParsingId();
	}

	if (name_info.m_name_problem == cdpr_none)
	{
		// Fill in the linkage props.
		if (m_curr_linkage_spec != NULL)
			func_object->m_linkage_spec = m_curr_linkage_spec->m_linkage_spec.m_link_spec;

		// Setup the access level.
		if (dest_defns_layer == m_defns_layer_owner)
			func_object->m_accs_level = GetCurrentAccessLevel();
	}

	// Setup the func attrs and the return value type.
	func_object->m_func_attrs = func_attrs;
	if (func_object->m_spec_type != spcft_typecast)
		func_object->m_return_value_type = ret_value_type;

	// Check for the presence of the final params modifer.
	if (unapplied_func_modifier != NULL)
	{
		// Function params modifier is present. Move data from the modifier to the function object.
		// Note that this call can add "fcta_this_const" and/or "fcta_this_volatile" to the func attrs.
		unapplied_func_modifier->m_func_spec.SetupFunctionTypeObject(func_object);
	}

	if (dest_defns_layer->IsStructType() == TRUE && (func_attrs & fcta_static) == 0)
	{
		// This function is an instance method. Check for other methods that it might override.
		CheckCollectOverrideProtos(func_object, dest_defns_layer, TRUE);
	}

	// Success.
	return(func_object);
}

TCppBlockDefn *TCppParser::CreateAndPrepareFunctionBody(TCppCodeEntryPointDefn *func_object)
{
	assert(func_object != NULL && func_object->m_function_body == NULL);

	// Body of the function is always a block. This is so regardless if function has a simple body
	// or a function try block.
	TCppBlockDefn *new_block = (TCppBlockDefn*)AllocateNewCppItem(cxx_block);
	if (new_block == NULL)
		return(NULL);

	// Set the function body into the function object right now in spite of the fact that the body is still empty.
	func_object->AddChildItem(new_block);
	func_object->m_function_body = new_block;

	// Add parameters of the function into the names table of the newly created block.
	for (TCppDirectChildrenIter iter(func_object); iter; ++iter)
	{
		TCppDataFieldDefn *data_field = iter.CurrItemAsDataField();
		if (data_field != NULL && data_field->m_strg_class == strgc_param)
		{
			// Current item is a param of the function.
			new_block->AddToNestedDefnsSpace(data_field);
		}
	}

	// Success.
	return(new_block);
}

TCppDataTypeBase *TCppParser::CreateAndPublishElabFwdDecl(TCppDefnBase *fwd_decl_layer, TCppElabType elab_type, TCppNameInfo &name_info, TCppItemType item_type)
{
	// This method expects that name of the object is fully prepared.
	assert(elab_type != elab_none && name_info.IsIdentifier() == TRUE);

	TCppDataTypeBase *fwd_decl = (TCppDataTypeBase*)AllocateNewCppItem(item_type);
	if (fwd_decl == NULL)
		return(NULL);

	if (elab_type != elab_enum)
		((TCppStructTypeDefn*)fwd_decl)->SetupStructType(elab_type);

	name_info.SetNameToObject(fwd_decl);

	if (item_type == cxx_class_template)
	{
		TList *template_header_params = name_info.TemplateHeaderParams();
		if (template_header_params != NULL)
			fwd_decl->AddListOfChildren(*template_header_params);
	}

	// Success.
	PublishOnTheSpecificLayer(fwd_decl_layer, fwd_decl);
	return(fwd_decl);
}

TCppFriendSpec *TCppParser::CreateAndPublishFriendRecord(TSymbolStateItem *err_ctx_sym, TCppDefnBase *friendly_object)
{
	assert(friendly_object != NULL);
	if (friendly_object->ItemParent() == m_defns_layer_owner)
	{
		// Friendly object resides on the current layer.
		TTextBuffer256 name_buffer;
		ReportSyntaxError(err_ctx_sym, cerr_friend_rec_obj_on_same_layer, TCppDatabase::GenerateQualifiedName(name_buffer, friendly_object));
	}

	TCppFriendSpec *friend_spec = (TCppFriendSpec*)AllocateNewCppItem(cxx_friend_spec);
	if (friend_spec == NULL)
		return(NULL);

	// Setup the props and publish this spec on the current layer.
	friend_spec->m_friendly_object = friendly_object;
	PublishOnTheCurrentLayer(friend_spec);

	// Success.
	return(friend_spec);
}

short TCppParser::ComputeBitWidthExpression(TCppSymbolStateItem &width_expr_sym, bool bit_field_width_expr)
{
	// Veryfy the exression symbol.
	assert(width_expr_sym.m_meaning == csm_expression && width_expr_sym.m_expression != NULL);
	const wchar_t *target_object_name = (bit_field_width_expr == TRUE) ? L"bit field member" : L"data alignment record";

	// Compute the expression.
	TCppOperandExpr bit_size_value_operand;
	if (width_expr_sym.m_expression->GetConstOperandValue(bit_size_value_operand) == FALSE)
	{
		// The expression is not constant.
		if (IsTemplateBodyContext() == FALSE)
			ReportSyntaxError(&width_expr_sym, cerr_bft_non_const_expr, target_object_name);
	}
	else if (bit_size_value_operand.ConvertToNumericLiteral() == FALSE)
	{
		// The expression is constant but the constant value is not numeric.
		if (IsTemplateBodyContext() == FALSE)
			ReportSyntaxError(&width_expr_sym, cerr_bft_non_numeric_expr, target_object_name);
	}
	else
	{
		// Conversion to the numeric value went fine. Check the value.
		if (bit_size_value_operand.m_num_value < 0)
		{
			ReportSyntaxError(&width_expr_sym, cerr_bft_negative_const_expr, target_object_name, bit_size_value_operand.m_num_value);
		}
		else if (bit_size_value_operand.m_num_value == 0 && bit_field_width_expr == TRUE)
		{
			// Here is a caveat. Bit field type does not allow zero width while data field alignment record does.
			ReportSyntaxError(&width_expr_sym, cerr_bft_zero_const_expr);
		}
		else if (bit_size_value_operand.m_num_value > 0x7FFF)
		{
			ReportSyntaxError(&width_expr_sym, cerr_bft_too_big_const_expr, target_object_name, bit_size_value_operand.m_num_value);
		}
		else
		{
			// The value of the width expr looks fine. Store the value.
			return((short)bit_size_value_operand.m_num_value);
		}
	}

	// Procedure has not succeeded.
	return(-1);
}

bool TCppParser::CreatePossibleExceptiosList(TCppSymbolStateItem &new_non_term, TCppDataTypeBase *exception_type)
{
	TList *plist = AllocateGeneralPurposeList();
	if (plist == NULL)
		return(FALSE);

	TCppPossibleExceptionSpec *spec = (TCppPossibleExceptionSpec*)AllocateNewCppItem(cxx_possible_exception_spec);
	if (spec == NULL)
	{
		RecycleGeneralPurposeList(plist);
		return(FALSE);
	}

	// Both allocations succceded. Build the data structure.
	spec->m_exception_type = exception_type;
	plist->AppendItem(spec);

	// Setup the non terminal.
	new_non_term.m_meaning = csm_exceptions_list;
	new_non_term.m_exceptions_list = plist;
	return(TRUE);
}

TCppFunctionAttr TCppParser::CheckFunctionDeclTypeSpec(TCppDefnBase *dest_defns_layer, TCppSymbolStateItem &declarator_sym, bool func_header_ctx)
{
	// This method should be called only when the decl type spec is present.
	assert(m_curr_decl_type_spec != NULL);
	TCppDeclTypeSpecInfo &dts = m_curr_decl_type_spec->m_decl_type_spec_info;

	// Pick up the name info from the declarator.
	assert(declarator_sym.m_meaning == csm_declarator);
	TCppNameInfo &name_info = declarator_sym.m_declarator_info.m_name_info;
	wchar_t buff96[96];

	// Check the decl type spec prefix.
	if (dts.m_dclt_prefix == declpr_typedef)
	{
		// Typedef specifier cannot be used with the function.
		ReportSyntaxError(m_curr_decl_type_spec, cerr_func_dts_bad_typedef, name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
	}
	else if (dts.m_dclt_prefix == declpr_friend && m_defns_layer_owner->IsStructType() == FALSE)
	{
		// Friend specifier stays outside of the class. Issue an error.
		ReportSyntaxError(m_curr_decl_type_spec, cerr_func_dts_bad_friend_context);
	}

	// Check process the function attrs if any.
	TCppFunctionAttr func_attrs = dts.m_dclt_func_attrs;

	if ((func_attrs & fcta_virtual) != 0)
	{
		if (dest_defns_layer->IsStructType() == FALSE)
		{
			// Func attr VIRTUAL is applicable only to the methods of the classes.
			ReportSyntaxError(m_curr_decl_type_spec, cerr_func_dts_bad_virtual_context);
			func_attrs = (TCppFunctionAttr)(func_attrs & ~fcta_virtual);
		}
		else if (name_info.m_core.m_info_type == cnit_constructor || name_info.m_core.m_info_type == cnit_ctor_template)
		{
			// Constructors cannot be virtual.
			ReportSyntaxError(m_curr_decl_type_spec, cerr_func_dts_bad_virtual_ctor, dest_defns_layer->GetElabTypeName());
			func_attrs = (TCppFunctionAttr)(func_attrs & ~fcta_virtual);
		}
	}

	if ((func_attrs & fcta_explicit) != 0 && name_info.m_core.m_info_type != cnit_constructor && name_info.m_core.m_info_type != cnit_ctor_template)
	{
		// Function attr EXPLICIT can be used only with constructors.
		ReportSyntaxError(m_curr_decl_type_spec, cerr_func_dts_bad_no_ctor_explicit);
		func_attrs = (TCppFunctionAttr)(func_attrs & ~fcta_explicit);
	}

	// Check process the storage class specifier if any.
	if (dts.m_dclt_strg_class == strgc_static)
	{
		// Static functions are allowed on all types of defn layers but not to all types of objects.
		if (name_info.m_core.m_info_type == cnit_constructor || name_info.m_core.m_info_type == cnit_ctor_template)
		{
			ReportSyntaxError(m_curr_decl_type_spec, cerr_func_dts_bad_static_ctor);
		}
		else if (name_info.m_core.m_info_type == cnit_dector)
		{
			ReportSyntaxError(m_curr_decl_type_spec, cerr_func_dts_bad_static_dector);
		}
		else if (name_info.m_core.m_info_type == cnit_ovld_conversion || name_info.m_core.m_info_type == cnit_ovld_conv_template)
		{
			ReportSyntaxError(m_curr_decl_type_spec, cerr_func_dts_bad_static_ovld_conv);
		}
		else
		{
			// Accept STATIC keyword as the function attribute.
			func_attrs = (TCppFunctionAttr)(func_attrs | fcta_static);
		}
	}
	else if (dts.m_dclt_strg_class == strgc_extern)
	{
		 // On the namespace layers this storage class should be ignored.
		if (m_defns_layer_owner->IsStructType() == TRUE)
		{
			// This storage class is not applicable to the class context.
			ReportSyntaxError(m_curr_decl_type_spec, cerr_func_dts_extern_in_class, dest_defns_layer->GetElabTypeName());
		}
		else if (dest_defns_layer->IsStructType() == TRUE)
		{
			// This storage class is not applicable to the members of classes.
			ReportSyntaxError(m_curr_decl_type_spec, cerr_func_dts_extern_with_method, dest_defns_layer->GetElabTypeName());
		}
	}
	else if (dts.m_dclt_strg_class != strgc_none)
	{
		// Some bogus storage class is present.
		ReportSyntaxError(m_curr_decl_type_spec, cerr_func_dts_bad_strg_class, TCppDataFieldDefn::GetStorageClassFriendlyName(dts.m_dclt_strg_class),
						name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
	}

	// It is expected that only INLINE, VIRTUAL and EXPLICIT attrs can come from the source code.
	assert((func_attrs & ~(fcta_inline | fcta_virtual | fcta_explicit | fcta_static)) == 0);
	return(func_attrs);
}

bool TCppParser::CheckExistingFunctionProps(TCppDefnBase *dest_defns_layer, TCppFunctionAttr func_attrs, TCppNameInfo &name_info, TCppDeclaratorModifier *unapplied_func_modifier, TCppCodeEntryPointDefn *existing_func_object, bool func_header_ctx)
{
	bool func_props_compatible = TRUE;
	wchar_t buff96[96];

	if (m_curr_linkage_spec != NULL)
	{
		// Standard explicitly states that linkage spec can be omitted in subsequent prototypes/definitions.
		// The spec is currently present. This means it should match the linkage spec in the discovered object.
		if (TCppLinkageSpec::CompareLinkageSpecs(existing_func_object->m_linkage_spec, m_curr_linkage_spec->m_linkage_spec.m_link_spec) == FALSE)
		{
			wchar_t buff96[96];
			TTextBuffer256 name_buffer;
			const wchar_t *func_type_name = name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx);
			const wchar_t *full_func_name = TCppDatabase::GenerateQualifiedName(name_buffer, existing_func_object);

			TCppErrorReportingCtxSymbolStateItem error_context(name_info);
			if (existing_func_object->m_linkage_spec == NULL)
				ReportSyntaxError(&error_context, cerr_func_props_diff_linkage_specs1, func_type_name, full_func_name);
			else ReportSyntaxError(&error_context, cerr_func_props_diff_linkage_specs2, func_type_name, full_func_name, existing_func_object->m_linkage_spec->m_spec_name);

			func_props_compatible = FALSE;
		}
	}

	if (dest_defns_layer == m_defns_layer_owner && dest_defns_layer->IsStructType() == TRUE)
	{
		// Compare access levels.
		if (existing_func_object->m_accs_level != GetCurrentAccessLevel())
		{
			wchar_t buff96[96];
			TTextBuffer256 name_buffer;
			const wchar_t *func_type_name = name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx);
			const wchar_t *full_func_name = TCppDatabase::GenerateQualifiedName(name_buffer, existing_func_object);

			TCppErrorReportingCtxSymbolStateItem error_context(name_info);
			ReportSyntaxError(&error_context, cerr_func_props_diff_access_levels, func_type_name, full_func_name, existing_func_object->m_accs_level);
			func_props_compatible = FALSE;
		}
	}

	if ((func_attrs & fcta_inline) != 0 && (existing_func_object->m_func_attrs & fcta_inline) == 0)
	{
		// Func attr INLINE is missing.
		assert(m_curr_decl_type_spec != NULL);
		ReportSyntaxError(m_curr_decl_type_spec, cerr_func_props_unexpected_func_attr, L"inline", name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
		func_props_compatible = FALSE;
	}

	if ((func_attrs & fcta_virtual) != 0 && (existing_func_object->m_func_attrs & fcta_virtual) == 0)
	{
		// Func attr VIRTUAL is missing.
		assert(m_curr_decl_type_spec != NULL && dest_defns_layer->IsStructType() == TRUE);
		ReportSyntaxError(m_curr_decl_type_spec, cerr_func_props_unexpected_func_attr, L"virtual", name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
		func_props_compatible = FALSE;
	}

	if ((func_attrs & fcta_explicit) != 0 && (existing_func_object->m_func_attrs & fcta_explicit) == 0)
	{
		assert(m_curr_decl_type_spec != NULL && (name_info.m_core.m_info_type == cnit_constructor || name_info.m_core.m_info_type == cnit_ctor_template));
		ReportSyntaxError(m_curr_decl_type_spec, cerr_func_props_unexpected_func_attr, L"explicit", name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
		func_props_compatible = FALSE;
	}

	if (unapplied_func_modifier != NULL)
	{
		// Compare cv qualifiers of the paramter "this".
		assert(unapplied_func_modifier->m_dmt == dclmdf_function);
		TCppCvPtrRefSpec &defn_cv_spec = unapplied_func_modifier->m_func_spec.m_this_cv_spec;
		TCppFunctionAttr existing_func_attrs = existing_func_object->m_func_attrs;

		if (defn_cv_spec.cvpr_const == TRUE && (existing_func_attrs & fcta_this_const) == 0)
		{
			TCppErrorReportingCtxSymbolStateItem error_context(unapplied_func_modifier->m_func_spec.m_this_cv_spec_context);
			ReportSyntaxError(&error_context, cerr_func_props_unexpected_this_spec, L"const", name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
			func_props_compatible = FALSE;
		}
		else if (defn_cv_spec.cvpr_const == FALSE && (existing_func_attrs & fcta_this_const) != 0)
		{
			TCppErrorReportingCtxSymbolStateItem error_context(unapplied_func_modifier->m_modifier_context);
			ReportSyntaxError(&error_context, cerr_func_props_missing_this_spec, name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx), L"const");
			func_props_compatible = FALSE;
		}

		if (defn_cv_spec.cvpr_volatile == TRUE && (existing_func_attrs & fcta_this_volatile) == 0)
		{
			TCppErrorReportingCtxSymbolStateItem error_context(unapplied_func_modifier->m_func_spec.m_this_cv_spec_context);
			ReportSyntaxError(&error_context, cerr_func_props_unexpected_this_spec, L"volatile", name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx));
			func_props_compatible = FALSE;
		}
		else if (defn_cv_spec.cvpr_volatile == FALSE && (existing_func_attrs & fcta_this_volatile) != 0)
		{
			TCppErrorReportingCtxSymbolStateItem error_context(unapplied_func_modifier->m_modifier_context);
			ReportSyntaxError(&error_context, cerr_func_props_missing_this_spec, name_info.GetFuncNameLs(buff96, dest_defns_layer, func_header_ctx), L"volatile");
			func_props_compatible = FALSE;
		}
	}

	return(func_props_compatible);
}

void TCppParser::CheckCollectOverrideProtos(TCppCodeEntryPointDefn *func_object, TCppDefnBase *defns_container, bool dest_layer)
{
	if (dest_layer == FALSE)
	{
		// Check instance methods of the passed definitions container.
		for (TCppDirectChildrenIter iter1(defns_container); iter1; ++iter1)
		{
			TCppCodeEntryPointDefn *instance_method = iter1.CurrItemAsCodeEntryPoint(FALSE);
			if (instance_method == NULL)
				continue;

			if (TCppFunctionTypeDefn::CompareFunctionTypes(func_object, instance_method, TRUE) == TRUE)
			{
				// Add this discovered method to the list of the override protos.
				if (func_object->m_override_protos.AppendItem(instance_method) == FALSE)
				{
				}

				// There is no need to check the base classes of the passed container.
				return;
			}
		}
	}

	// Check base classes of the passed container if any.
	for (TCppDirectChildrenIter iter2(defns_container); iter2; ++iter2)
	{
		if (iter2.CurrItem().ItemType() == cxx_base_class_spec)
		{
			TCppBaseClassSpec &base_class_proxy = (TCppBaseClassSpec&)iter2.CurrItem();
			CheckCollectOverrideProtos(func_object, base_class_proxy.m_base_class, FALSE);
		}
	}
}

void TCppParser::AssembleTypelessDeclarator(TCppSymbolStateItem &declarator_sym, TCppDeclaratorModifier *&first_mdfs_layer)
{
	assert(declarator_sym.m_meaning == csm_declarator && first_mdfs_layer == NULL);
	TCppNameInfoType cnit = declarator_sym.m_declarator_info.m_name_info.m_core.m_info_type;
	TCppDeclaratorModifiersArray *modifiers = declarator_sym.m_declarator_info.m_modifiers;

	// The modifer should be present and it should contain one layer of func params.
	if (modifiers == NULL || modifiers->NumItems() == 0)
	{
		// The list of function parameters is missing.
		if (cnit == cnit_constructor || cnit == cnit_ctor_template)
		{
			ReportSyntaxError(&declarator_sym, cerr_typeless_decl_no_params_ctor);
		}
		else if (cnit == cnit_dector)
		{
			ReportSyntaxError(&declarator_sym, cerr_typeless_decl_no_params_dector);
		}
		else
		{
			assert(cnit == cnit_ovld_conversion || cnit == cnit_ovld_conv_template);
			ReportSyntaxError(&declarator_sym, cerr_typeless_decl_no_params_conv);
		}
	}
	else if (modifiers->NumItems() != 1 || modifiers->ItemRef(0).m_dmt != dclmdf_function)
	{
		// Even if the second layer is func params, both can be considered as bad because func params should go fist.
		const wchar_t *num_bad_mdfs_suff = (modifiers->NumItems() >= 3 || modifiers->ItemRef(0).m_dmt != dclmdf_function) ? L"s" : L"";
		if (cnit == cnit_constructor || cnit == cnit_ctor_template)
		{
			ReportSyntaxError(&declarator_sym, cerr_typeless_decl_bad_mdfs_ctor, num_bad_mdfs_suff);
		}
		else if (cnit == cnit_dector)
		{
			ReportSyntaxError(&declarator_sym, cerr_typeless_decl_bad_mdfs_dector, num_bad_mdfs_suff);
		}
		else
		{
			assert(cnit == cnit_ovld_conversion || cnit == cnit_ovld_conv_template);
			ReportSyntaxError(&declarator_sym, cerr_typeless_decl_bad_mdfs_conv, num_bad_mdfs_suff);
		}
	}
	else
	{
		// Only one layer of func params is present. Give it out.
		first_mdfs_layer = modifiers->ItemPtr(0);
	}
}

TCppDefnBase *TCppParser::AssembleDeclaratorType(TSymbolStateItem *dts_ctx_sym, bool typedef_mode, TCppCvPtrRefSpec decl_cv_spec, TCppDefnBase *decl_base_type, TCppSymbolStateItem &declarator_sym, TCppDeclaratorModifier **unapplied_func_modifier)
{
	// The decl type spec symbol can be missing.
	if (dts_ctx_sym == NULL)
	{
		// Use the source area of the declarator symbol for reporting errors.
		dts_ctx_sym = &declarator_sym;
	}

	// The base type should be passed and the passed symbol should be declarator.
	assert(decl_base_type != NULL && declarator_sym.m_meaning == csm_declarator);
	TCppDeclaratorInfo &declarator_info = declarator_sym.m_declarator_info;

	// Process the const/volatile modifier that can be present in the type specifier.
	if (decl_cv_spec.IsEmpty() == FALSE)
	{
		// This type of modifiers is compatible only with the data type in the base type.
		if (decl_base_type->IsDataType() == TRUE)
		{
			// Convert modifiers from the decl type spec into the new layer of modifiers. This action may fail and generate
			// syntax error. In this case the modifier will be simply lost.
			ApplyCvPtrRefModifier(dts_ctx_sym, declarator_info, decl_cv_spec);
		}
		else
		{
			assert(decl_base_type->IsFunctionType() == TRUE);

			// Note that passed decl_cv_spec variable cannot contain ptr or ref flags that are applicable to the function type.
			// It can contains only const/volatile flags that are not good. Issue an error and remove this bogus modifier.
			ReportSyntaxError(dts_ctx_sym, cerr_decltor_base_func_type_cv);
		}
	}

	// Check the compatibility between the base type and the last modifier of the declarator.
	if (declarator_info.m_modifiers != NULL)
	{
		if (decl_base_type->IsFunctionType() == TRUE)
		{
			// The base type is the function type. Certain restrictions apply.
			while (declarator_info.m_modifiers->NumItems() > 0)
			{
				// Pick up the last modifier. It will be applied directly to the passed base type.
				TCppDeclaratorModifier &final_mdf = declarator_info.m_modifiers->ItemRef(declarator_info.m_modifiers->NumItems()-1);
				if (final_mdf.m_dmt == dclmdf_array)
				{
					// Report an error and remove this modifier. This may create strange result type.
					ReportSyntaxError(dts_ctx_sym, cerr_decltor_array_elem_func);
					final_mdf.ReleaseResources();
					declarator_info.m_modifiers->ReleaseLastItem();
				}
				else if (final_mdf.m_dmt == dclmdf_function)
				{
					// Report an error and remove this modifier. This may create strange result type.
					ReportSyntaxError(dts_ctx_sym, cerr_decltor_func_retval_func);
					final_mdf.ReleaseResources();
					declarator_info.m_modifiers->ReleaseLastItem();
				}
				else
				{
					// This modifier is fine.
					assert(final_mdf.m_dmt == dclmdf_cvpr);
					break;
				}
			}
		}

		// The list of modifiers may become empty or it may be non NULL and empty from the beginning.
		if (declarator_info.m_modifiers->NumItems() <= 0)
		{
			RecycleDeclaratorModifiersArray(declarator_info.m_modifiers);
			declarator_info.m_modifiers = NULL;
		}
	}

	if (declarator_info.m_modifiers == NULL)
	{
		// This is special case when there are no modifiers in the declarator.
		if (decl_base_type->IsDataType() == TRUE)
		{
			// The passed base type is the data type.
			if (typedef_mode == TRUE)
			{
				// Create data type alias wrapper.
				TCppDataTypeAliasDefn *data_type_alias = (TCppDataTypeAliasDefn*)AllocateNewCppItem(cxx_data_type_alias);
				if (data_type_alias == NULL)
					return(NULL);

				// Finalize and give out this data type alias.
				data_type_alias->m_base_type = (TCppDataTypeBase*)decl_base_type;
				return(data_type_alias);
			}

			//  Return the original type.
			return(decl_base_type);
		}
		else
		{
			// The base type should be the function type. Ensure this.
			assert(decl_base_type->IsFunctionType() == TRUE);
			if (typedef_mode == TRUE)
			{
				// Create function type alias wrapper.
				TCppFunctionTypeAliasDefn *func_type_alias = (TCppFunctionTypeAliasDefn*)AllocateNewCppItem(cxx_function_type_alias);
				if (func_type_alias == NULL)
					return(NULL);

				// Finalize and give out this func type alias.
				func_type_alias->m_base_type = (TCppFunctionTypeBase*)decl_base_type;
				return(func_type_alias);
			}

			// The source code is trying to create something out of the function type without modifiers. In this situation
			// MS compiler silently converts function type into the pointer to this function type. Note that resulting type
			// will be a pointer and pointer is the data type. Do the same thing.
			TCppCvPtrRefSpec local_cvpr;
			local_cvpr.SetPointer();
			ApplyCvPtrRefModifier(NextRuleSymPtr(dts_ctx_sym), declarator_info, local_cvpr);

			// In fact, failure to add the modifier above is a critical error because otherwise the function will have
			// to return pointer to the function type, which is much worse.
			if (declarator_info.m_modifiers == NULL)
			{
				RaiseCriticalCppError(ccerr_oom_decltor_ptr_to_func);
				return(NULL);
			}
		}
	}

	// At this point the modifiers should be present. Ensure this.
	assert(declarator_info.m_modifiers != NULL && declarator_info.m_modifiers->NumItems() > 0);
	TCppDeclaratorModifiersArray &mdfs = *(declarator_info.m_modifiers);

	// Modifiers are scanned starting from the variable in the direction to the base type. During the process of scanning
	// discovered modifiers are added to the end of the array. Purpose of this method is to create the type for the variable.
	// This is done by applying modifers the base type. This means that modifiers should be processed starting from bigger
	// indexes in the direction to lower indexes. Look for ranges of similar modifiers.
	int range_beg = mdfs.NumItems()-1;
	bool first_modifier = TRUE;
	while (range_beg >= 0)
	{
		TCppErrorReportingCtxSymbolStateItem last_mdf_context(mdfs[range_beg].m_modifier_context);
		TCppDeclaratorModifierType range_type = mdfs[range_beg].m_dmt;

		int range_end = range_beg-1;
		if (range_type != dclmdf_function)
		{
			// Function modifiers do not constitute ranges. They are always processed one by one.
			// Other types of modifiers do have ranges. Look for the end of the current range.
			while (range_end >= 0 && mdfs[range_end].m_dmt == range_type)
				range_end--;
		}

		int start_index = range_end+1;
		TCppDeclaratorModifier *first_mdf_ptr = mdfs.ItemPtr(start_index);
		int len_range = range_beg-range_end;

		bool publish_this_layer = TRUE;
		if (start_index == 0 && typedef_mode == TRUE)
		{
			// Final modification in the typedef definition should not be published.
			publish_this_layer = FALSE;
		}

		if (range_type == dclmdf_cvpr)
		{
			// Create const/volatile/ptr/ref modifier.
			decl_base_type = CreateCvPtrRefModification(&last_mdf_context, decl_base_type, first_mdf_ptr, len_range, publish_this_layer, first_modifier);
			if (decl_base_type == NULL)
				return(NULL);
		}
		else if (range_type == dclmdf_array)
		{
			// Create an array modifier.
			assert(decl_base_type->IsDataType() == TRUE);
			if (first_modifier == TRUE && decl_base_type->ItemType() == cxx_array_type)
			{
				// Do a trick by combining layers from the base type with the layers from the declarator.
				TCppDeclaratorInfo combined_layers_info;
				combined_layers_info.Clear();

				// First add layers from the base type.
				TCppArrayTypeDefn *array_base_type = (TCppArrayTypeDefn*)decl_base_type;
				for (int d1=0; d1<array_base_type->m_dimensions.NumItems(); ++d1)
				{
					int inx = AllocateDeclaratorModifierLayer(dts_ctx_sym, combined_layers_info);
					if (inx < 0)
						break;

					combined_layers_info.m_modifiers->ItemRef(inx).SetArrayDimSpec(array_base_type->m_dimensions[d1].m_num_elements);
				}

				// Append layers from the declarator.
				for (int d2=0; d2<len_range; ++d2)
				{
					int inx = AllocateDeclaratorModifierLayer(&last_mdf_context, combined_layers_info);
					if (inx < 0)
						break;

					combined_layers_info.m_modifiers->ItemRef(inx).SetArrayDimSpec(mdfs.ItemRef(start_index+d2).m_array_spec.m_num_elements);
				}

				if (combined_layers_info.m_modifiers == NULL)
				{
					// This should be exteremely rare case. Error is reported above. Consider this case as critical error.
					return(NULL);
				}

				// Finally create the array modifier.
				int num_modifiers = combined_layers_info.m_modifiers->NumItems();
				assert(num_modifiers > 0);
				decl_base_type = CreateArrayModification(&last_mdf_context, array_base_type->m_base_type, combined_layers_info.m_modifiers->DataPtr(), num_modifiers, publish_this_layer, first_modifier);
				RecycleDeclaratorModifiersArray(combined_layers_info.m_modifiers);
				if (decl_base_type == NULL)
					return(NULL);
			}
			else
			{
				// Process simple case.
				decl_base_type = CreateArrayModification(&last_mdf_context, (TCppDataTypeBase*)decl_base_type, first_mdf_ptr, len_range, publish_this_layer, first_modifier);
				if (decl_base_type == NULL)
					return(NULL);
			}
		}
		else
		{
			// This should be the function modifier.
			assert(range_type == dclmdf_function);
			if (start_index != 0 || typedef_mode == TRUE)
			{
				// The type that is currently being created is not the final type or the destination is the typedef. Function type
				// can be present as an intermediate type in any declarator. Typedef allows function type as the result type.
				assert(decl_base_type->IsDataType() == TRUE);
				decl_base_type = CreateFunctionModification(&last_mdf_context, (TCppDataTypeBase*)decl_base_type, first_mdf_ptr, publish_this_layer, first_modifier);
				if (decl_base_type == NULL)
					return(NULL);
			}
			else if (unapplied_func_modifier != NULL)
			{
				// Caller wants to create either a function or a function prototype.
				*unapplied_func_modifier = first_mdf_ptr;
			}
			else
			{
				// Context of the caller expects only the data type.
				ReportSyntaxError(&last_mdf_context, cerr_decltor_result_func_type);
			}
		}

		// Shift to the next range of compatible modifiers.
		range_beg = range_end	;
		first_modifier = FALSE;
	}

	// Retrun the resulting data type.
	return(decl_base_type);
}

TCppDataTypeBase *TCppParser::CreateCvPtrRefModification(TSymbolStateItem *err_ctx_sym, TCppDefnBase *base_type, TCppDeclaratorModifier *cvprs, int num_cvprs, bool publish_modifier, bool named_base_type)
{
	// Do simple params check.
	assert(base_type != NULL && cvprs != NULL && num_cvprs >= 0);

	TCppDefnBase *base_type_owner = NULL;
	if (publish_modifier == TRUE)
	{
		// Look for the cached modifier of this type.
		base_type_owner = (TCppDefnBase*)base_type->ItemParent();
		assert(base_type_owner != NULL && base_type_owner->IsDefinition() == TRUE);

		TCppDefnsSpace *defns_space = base_type_owner->GetNestedDefnsSpace();
		assert(defns_space != NULL);

		TCppCvPtrRefTypeDefn *cached_cvpr_type = defns_space->LookupUnnamedCvPtrRefType(base_type->ItemId(), cvprs, num_cvprs);
		if (cached_cvpr_type != NULL)
		{
			// Requested modifier was already used somewhere.
			return(cached_cvpr_type);
		}
	}

	// New data type object is needed.
	TCppCvPtrRefTypeDefn *data_type = (TCppCvPtrRefTypeDefn*)AllocateNewCppItem(cxx_cv_ptr_ref_type);
	if (data_type == NULL)
		return(NULL);

	// Set the base type.
	data_type->m_base_type = base_type;

	// Reserve space for all requested indirection layers at once.
	if (data_type->m_layers.ReserveSpace(num_cvprs) == FALSE)
	{
		ReportSyntaxError(err_ctx_sym, cerr_oom_decltor_cvpr_modifier);
		delete data_type;
		return(NULL);
	}

	// Add layers from the array. Orignal array of modifiers is ordered "from the variable to the base type".
	// Reverse the order of these modifiers to "from base type to final type".
	for (int inx = num_cvprs-1; inx >= 0; --inx)
		data_type->m_layers.AppendItem(cvprs[inx].m_cvpr_spec);

	if (publish_modifier == TRUE)
	{
		// Prepare the name for this new object. Key id in this name will be zero.
		if (m_cpp_database.PrepareAndSetCvPtrRefTypeName(data_type, named_base_type) == FALSE)
		{
			ReportSyntaxError(err_ctx_sym, cerr_oom_decltor_cvpr_modifier);
			delete data_type;
			return(NULL);
		}

		// Release the definition.
		PublishDerivedDefinition(base_type_owner, data_type);
	}

	// Success.
	return(data_type);
}

TCppDataTypeBase *TCppParser::CreateArrayModification(TSymbolStateItem *err_ctx_sym, TCppDataTypeBase *base_type, TCppDeclaratorModifier *layers, int num_layers, bool publish_modifier, bool named_base_type)
{
	// Do simple params check.
	assert(base_type != NULL && layers != NULL && num_layers > 0);

	TCppDefnBase *base_type_owner = NULL;
	if (publish_modifier == TRUE)
	{
		// Look for the cached modifier of this type first.
		base_type_owner = (TCppDefnBase*)base_type->ItemParent();
		assert(base_type_owner != NULL && base_type_owner->IsDefinition() == TRUE);

		TCppDefnsSpace *defns_space = base_type_owner->GetNestedDefnsSpace();
		assert(defns_space != NULL);

		TCppArrayTypeDefn *cached_array_type = defns_space->LookupUnnamedArrayType(base_type->ItemId(), layers, num_layers);
		if (cached_array_type != NULL)
		{
			// Requested modifier was already used somewhere.
			return(cached_array_type);
		}
	}

	// New data type descriptor is needed.
	TCppArrayTypeDefn *data_type = (TCppArrayTypeDefn*)AllocateNewCppItem(cxx_array_type);
	if (data_type == NULL)
		return(NULL);

	// Set the base type.
	data_type->m_base_type = base_type;

	// Reserve space for all requested dimensions at once.
	if (data_type->m_dimensions.ReserveSpace(num_layers) == FALSE)
	{
		ReportSyntaxError(err_ctx_sym, cerr_oom_decltor_array_modifier);
		delete data_type;
		return(NULL);
	}

	// Add passed dimension sizes. Do this using two separate loops.
	for (int inx1=0; inx1<num_layers; ++inx1)
	{
		TCppArrayDimensionSpec spec1 = { layers[inx1].m_array_spec.m_num_elements, NULL };
		data_type->m_dimensions.AppendItem(spec1);
	}

	// Run second loop on passing the expression objects.
	for (int inx2=0; inx2<num_layers; ++inx2)
	{
		TCppArrayDimensionSpec &spec2 = layers[inx2].m_array_spec;
		if (spec2.m_num_elements_expr != NULL)
		{
			data_type->AddChildItem(spec2.m_num_elements_expr, inx2);
			spec2.m_num_elements_expr = NULL;
		}
	}

	if (publish_modifier == TRUE)
	{
		// Prepare the name for this new object. Key id in this name will be zero.
		if (m_cpp_database.PrepareAndSetArrayTypeName(data_type, named_base_type) == FALSE)
		{
			ReportSyntaxError(err_ctx_sym, cerr_oom_decltor_array_modifier);
			delete data_type;
			return(NULL);
		}

		// Release the definition.
		PublishDerivedDefinition(base_type_owner, data_type);
	}

	// Success.
	return(data_type);
}

TCppFunctionTypeDefn *TCppParser::CreateFunctionModification(TSymbolStateItem *err_ctx_sym, TCppDataTypeBase *base_type, TCppDeclaratorModifier *mdf_data, bool publish_modifier, bool named_base_type)
{
	// Do a simple params check.
	assert(base_type != NULL && mdf_data != NULL && mdf_data->m_dmt == dclmdf_function);

	TCppDefnBase *base_type_owner = NULL;
	if (publish_modifier == TRUE)
	{
		// Look for the cached modifier of this type.
		base_type_owner = (TCppDefnBase*)base_type->ItemParent();
		assert(base_type_owner != NULL && base_type_owner->IsDefinition() == TRUE);

		TCppDefnsSpace *defns_space = base_type_owner->GetNestedDefnsSpace();
		assert(defns_space != NULL);

		TCppFunctionTypeDefn *cached_func_type = defns_space->LookupUnnamedFunctionType(base_type->ItemId(), mdf_data);
		if (cached_func_type != NULL)
			return(cached_func_type);
	}

	// New function type object is needed.
	TCppFunctionTypeDefn *func_type = (TCppFunctionTypeDefn*)AllocateNewCppItem(cxx_function_type);
	if (func_type == NULL)
		return(NULL);

	// Setup the props.
	func_type->m_return_value_type = base_type;
	mdf_data->m_func_spec.SetupFunctionTypeObject(func_type);

	if (publish_modifier == TRUE)
	{
		// Prepare the name for this new object. The key id in this name will be zero.
		if (m_cpp_database.PrepareAndSetUnnamedFuncTypeName(func_type, named_base_type) == FALSE)
		{
			ReportSyntaxError(err_ctx_sym, cerr_oom_decltor_func_modifier);
			delete func_type;
			return(NULL);
		}

		// Release the definition.
		PublishDerivedDefinition(base_type_owner, func_type);
	}

	// Success.
	return(func_type);
}

TCppBitFieldTypeDefn *TCppParser::CreateBitFieldType(TSymbolStateItem *err_ctx_sym, TCppDataTypeBase *base_type, TCppSymbolStateItem &bit_size_expr_sym)
{
	assert(base_type != NULL);
	TCppDefnBase *base_type_owner = (TCppDefnBase*)base_type->ItemParent();
	assert(base_type_owner != NULL && base_type_owner->IsDefinition() == TRUE);

	// Evaluate the passed expression.
	short bit_size_value = ComputeBitWidthExpression(bit_size_expr_sym, TRUE);

	if (bit_size_value > 0)
	{
		// This type has fixed bit width. Look for the cached type with these props.
		TCppBitFieldTypeDefn *cached_bit_field_type = base_type_owner->GetNestedDefnsSpace()->LookupBitFieldType(base_type->ItemId(), bit_size_value);
		if (cached_bit_field_type != NULL)
			return(cached_bit_field_type);
	}

	// The type is unknown or it has variable width.
	TCppBitFieldTypeDefn *bit_field_type = (TCppBitFieldTypeDefn*)AllocateNewCppItem(cxx_bit_field_type);
	if (bit_field_type == NULL)
		return(NULL);

	// Prepare the name. The key id in the name will be zero.
	if (m_cpp_database.PrepareAndSetBitFieldTypeName(bit_field_type, TRUE) == FALSE)
	{
		RaiseCriticalCppError(ccerr_oom_generating_object_name, L"a bit field type");
		delete bit_field_type;
		return(NULL);
	}

	// Setup the props.
	bit_field_type->m_base_type = base_type;
	bit_field_type->m_bit_length = bit_size_value;
	bit_field_type->AddChildItem(bit_size_expr_sym.m_expression);
	bit_size_expr_sym.ResetMeaning();

	// Add this special type to the cache for future use or just publish it.
	PublishDerivedDefinition(base_type_owner, bit_field_type);

	// Success.
	return(bit_field_type);
}

TCppDataTypeBase *TCppParser::CreateDereferencedType(TSymbolStateItem *err_ctx_sym, TCppCvPtrRefTypeDefn *source_type, int new_num_cvprs)
{
	assert(new_num_cvprs >= 0);
	if (new_num_cvprs <= 0)
	{
		// Caller asks to remove all modifiers.
		return(/*-BUGBUG-*/(TCppDataTypeBase*)source_type->m_base_type);
	}

	TCppDeclaratorModifier mdf;
	mdf.Clear();

	// Convert array of cvpr specs into the array of declarator modifiers.
	assert(new_num_cvprs < source_type->m_layers.NumItems());
	TCppDeclaratorModifiersArray cvprs_array;
	for (int inx=0; inx<new_num_cvprs; ++inx)
	{
		mdf.SetCvprSpec(source_type->m_layers[inx]);
		if (cvprs_array.AppendItem(mdf) == FALSE)
		{
			// -BUGBUG-
			return(NULL);
		}
	}

	// Call the worker function.
	return(CreateCvPtrRefModification(err_ctx_sym, source_type->m_base_type, cvprs_array.DataPtr(), new_num_cvprs, TRUE, FALSE));
}

void TCppParser::PublishOnTheCurrentLayer(TCppItemBase *item)
{
	// Implementation of this method is simple. Purpose of the method is to simplify tracking these actions.
	m_defns_layer_owner->AddChildItem(item);
}

void TCppParser::PublishOnTheSpecificLayer(TCppDefnBase *defns_layer, TCppItemBase *item)
{
	// Implementation of this method is simple. Purpose of the method is to simplify tracking these actions.
	defns_layer->AddChildItem(item);
}

bool TCppParser::PublishBlockDeclaration(TCppDefnBase *defn_object_to_publish, bool want_full_procedure)
{
	// This method expects only definitions, not any type of cpp object.
	assert(defn_object_to_publish != NULL && defn_object_to_publish->IsDefinition() == TRUE);

	if (m_defns_layer_owner->IsBlock() == TRUE)
	{
		// Check if is it necessary to create an implicit block for this definition or not.
		TCppBlockDefn *implicit_block = NULL;

		// Iterate symbols on the stack starting from the bottom of the stack up to the header of the current block.
		TCppSymbolStateItem *sym = LastStackSymPtr();
		while (sym != NULL && sym != m_defns_layer_symbol)
		{
			if (sym->m_app_id == sym_while)
			{
				// The WHILE symbol should be ignored if it stays right after the do stmt body.
				TCppSymbolStateItem *prev_sym = PrevRuleSymPtr(sym);
				if (prev_sym != NULL && prev_sym->m_app_id == sym_DoStatementBody)
					sym = prev_sym;
			}

			WORD app_id = sym->m_app_id;
			if (app_id == sym_if || app_id == sym_switch || app_id == sym_for || app_id == sym_while || app_id == sym_do)
			{
				// Create an implicit block.
				assert(sym->m_meaning == csm_none);
				implicit_block =  (TCppBlockDefn*)AllocateNewCppItem(cxx_block);
				if (implicit_block == NULL)
					return(FALSE);

				// Associate this block with the current symbol. Note this will change the meaning of the symbol.
				sym->m_meaning = csm_block_header;
				sym->m_sym_meaning_ex = sym->m_meaning;
				sym->m_block_info.m_block_header = implicit_block;
				sym->m_block_info.m_block_object_published = FALSE;
				break;
			}
			else if (app_id == sym_DoStatementBody)
			{
				// Create an implicit block.
				assert(sym->m_meaning == csm_do_statement_body && sym->m_do_stmt_info.m_do_stmt_block_header == NULL);
				implicit_block =  (TCppBlockDefn*)AllocateNewCppItem(cxx_block);
				if (implicit_block == NULL)
					return(FALSE);

				// Associate this block with the current symbol. The meaning of the symbol is already fine.
				sym->m_do_stmt_info.m_do_stmt_block_header = implicit_block;
				break;
			}

			// Shift to the next symbol up the stack of the parser.
			sym = PrevRuleSymPtr(sym);
			assert(sym != NULL);
		}

		if (implicit_block != NULL)
		{
			// -BUGBUG- Make this block a pseudo child of the current layer.
			implicit_block->m_cib_parent = m_defns_layer_owner;

			// Enter this block.
			EnterDefinitionsLayer(implicit_block, *sym);
		}
	}

	// Publish defintion on the current layer. Note that the layer may have changed above.
	if (want_full_procedure == FALSE)
	{
		// This function publishes block definitions that are always named. Pass TRUE as the second parameter
		// to state this fact explicitly. This will place this defn into named buckets of the defns space.
		m_defns_layer_owner->AddToNestedDefnsSpace(defn_object_to_publish);
	}
	else
	{
		if (defn_object_to_publish->ItemId() == 0)
		{
			// Explicitly setup the id of the item because it is possible that current layer owner is not published
			// in the database itself. This function should garantee presence of the id after an exit from here.
			m_cpp_database.SetupItemId(defn_object_to_publish);
		}

		m_defns_layer_owner->AddChildItem(defn_object_to_publish);
	}

	// Success.
	return(TRUE);
}

bool TCppParser::PublishParameterObject(TCppItemBase *param_or_arg_to_publish)
{
	assert(param_or_arg_to_publish != NULL && m_curr_params_list != NULL);

	if (m_curr_params_list->m_meaning == csm_template_params_list && param_or_arg_to_publish->ItemType() == cxx_data_field)
	{
		// This is a data param (integral constant of address/reference of a static data field) of the template.
		// It is necessary to change the storage class of the data field object.
		TCppDataFieldDefn *data_field = (TCppDataFieldDefn*)param_or_arg_to_publish;
		data_field->m_strg_class = strgc_template_param;
	}

	TCppParamsListInfo &prms_list_info = (m_curr_params_list->m_meaning == csm_func_params_list) ? m_curr_params_list->m_func_params_info : m_curr_params_list->m_template_params_info;
	if (prms_list_info.m_params_list == NULL)
	{
		// Start building the list of params/arguments from scratch.
		prms_list_info.m_params_list = AllocateGeneralPurposeList();
		if (prms_list_info.m_params_list == NULL)
			return(FALSE);
	}

	// Setup the param index field.
	if (param_or_arg_to_publish->ItemType() == cxx_data_field)
		((TCppDataFieldDefn*)param_or_arg_to_publish)->m_param_index = prms_list_info.m_params_list->NumItems();
	else if (param_or_arg_to_publish->ItemType() == cxx_template_type_param)
		((TCppTemplateTypeParamDefn*)param_or_arg_to_publish)->m_param_index = prms_list_info.m_params_list->NumItems();
	else if (param_or_arg_to_publish->ItemType() == cxx_template_template_param)
		((TCppTemplateTemplateParamDefn*)param_or_arg_to_publish)->m_param_index = prms_list_info.m_params_list->NumItems();

	// The params/arguments list is available.
	prms_list_info.m_params_list->AppendItem(param_or_arg_to_publish);
	return(TRUE);
}

void TCppParser::PublishDerivedDefinition(TCppDefnBase *base_type_owner, TCppDefnBase *defn_item)
{
	// Publish this object on the same level of definitions where the base type is located. Do an important trick
	// by publishing this object in 2 separate steps. The first call will add defn into special buckets of the defns
	// space. The second call will only add this object into the list of children.
	base_type_owner->AddToNestedDefnsSpaceSpecial(defn_item);
	base_type_owner->AddChildItem(defn_item);
}

void TCppParser::EnterDefinitionsLayer(TCppDefnBase *layer_owner, TCppSymbolStateItem &layer_symbol)
{
	// Store info about the current layer in the symbol.
	layer_symbol.m_parent_layer_owner = m_defns_layer_owner;
	layer_symbol.m_parent_layer_symbol = m_defns_layer_symbol;

	// Nest the new layer of definitions.
	m_defns_layer_owner = layer_owner;
	m_defns_layer_symbol = &layer_symbol;
}

void TCppParser::ExitCurrentDefnsLayer(TCppSymbolStateItem *curr_layer_symbol)
{
	// Ensure that caller has correct understanding of what the current layer is.
	assert(m_defns_layer_symbol == curr_layer_symbol);

	// Exit the layer.
	m_defns_layer_owner = curr_layer_symbol->m_parent_layer_owner;
	m_defns_layer_symbol = curr_layer_symbol->m_parent_layer_symbol;
}

int TCppParser::AllocateDeclaratorModifierLayer(TSymbolStateItem *err_ctx_sym, TCppDeclaratorInfo &decl_info)
{
	if (decl_info.m_modifiers == NULL)
	{
		// Array of modifiers is not allocated yet.
		decl_info.m_modifiers = m_dcltr_mdfs_look_aside.GetItem();
		if (decl_info.m_modifiers == NULL)
		{
			// Report this as non critical error. As the result the declarator will be not accurate.
			ReportSyntaxError(err_ctx_sym, cerr_oom_decltor_modifiers_layer);
			return(-1);
		}

		// Clear the instance.
		decl_info.m_modifiers->SetNumItems(0);
	}

	if (decl_info.m_modifiers->HasFreeSpace() == FALSE)
	{
		// More space is needed in the array of modifiers.
		if (decl_info.m_modifiers->ReserveSpace(1) == FALSE)
		{
			// Report this as non critical error. As the result the declarator will be not accurate.
			ReportSyntaxError(err_ctx_sym, cerr_oom_decltor_modifiers_layer);
			return(-1);
		}
	}

	// Pick up the index of the layer and return it.
	int mdf_inx = decl_info.m_modifiers->NumItems();
	TCppDeclaratorModifier &mdf_info = decl_info.m_modifiers->ItemRef(mdf_inx);
	mdf_info.Clear();
	mdf_info.m_modifier_context = err_ctx_sym->GetOriginHeader();

	decl_info.m_modifiers->IncNumItems();
	return(mdf_inx);
}

TList *TCppParser::AllocateGeneralPurposeList()
{
	TList *plist = m_gen_purp_lists_look_aside.GetItem();
	if (plist == NULL)
	{
		// Report this to the user. For simplicity this is a critical error.
		RaiseCriticalCppError(ccerr_oom_general_purpose_list);
	}

	// Return the result, maybe NULL.
	return(plist);
}

TCppParserCtvRefEventInfo *TCppParser::AllocateUnsubmittedCtvEventInfo()
{
	TCppParserCtvRefEventInfo *pinfo = m_unsubm_ctv_infos_look_aside.GetItem();
	if (pinfo != NULL)
	{
		// Setup the back link to allow recycling this object.
		pinfo->m_owner_parser = this;
	}
	else
	{
		// Report this to the user. For simplicity this is a critical error.
		RaiseCriticalCppError(ccerr_oom_unsubm_ctv_event_info);
	}

	// Return the result, maybe NULL.
	return(pinfo);
}

wchar_t *TCppParser::CacheLiteralString(TStrPtrInfo &str_ptr_info)
{
	// Resgister the passed string or pick up the existing persistent pointer.
	wchar_t *persistent_str = m_cpp_database.m_global_strings_dict.RegisterStr(str_ptr_info.m_body, str_ptr_info.m_len);
	if (persistent_str == NULL)
	{
		RaiseCriticalCppError(ccerr_oom_storing_literal_string, str_ptr_info.m_len);
		return(NULL);
	}

	// Allocation suceeded.
	return(persistent_str);
}

TCppBuiltInTypeDefn *TCppParser::GetBuiltInTypeDefn(TCppBuiltInTypeIdent bltin_type_id)
{
	// Ensure that passed id belongs to the range of the built in type ids.
	assert(bltin_type_id > bltin_ids_base && bltin_type_id < bltin_ids_max_plus1);
	int bltin_inx = bltin_type_id-bltin_ids_base;

	// Check if this type was already used or not.
	if (m_built_in_defns[bltin_inx] != NULL)
		return(m_built_in_defns[bltin_inx]);

	// Create new definition for this built in type.
	TCppBuiltInTypeDefn *defn = (TCppBuiltInTypeDefn*)AllocateNewCppItem(cxx_built_in_type, bltin_type_id);
	if (defn == NULL)
	{
		// Critical error is already raised.
		return(NULL);
	}

	// Publish this definition on the global namespace.
	m_cpp_database.GetGlobalNamespace().AddChildItem(defn);

	// Cache this definition for future use.
	m_built_in_defns[bltin_inx] = defn;
	return(defn);
}

TCppItemBase *TCppParser::AllocateNewCppItem(TCppItemType defn_type, ID predef_item_id)
{
	// Create object of the appropriate class depending on the requested type.
	TCppItemBase *item = m_objects_factory.AllocateCppObject(defn_type, predef_item_id);

	// Check the creation result.
	if (item == NULL)
	{
		// This is critical error.
		RaiseCriticalCppError(ccerr_oom_creating_cpp_object, TCppItemBase::GetItemStructName(defn_type));
		return(NULL);
	}

	// Return the allocated structure.
	return(item);
}


