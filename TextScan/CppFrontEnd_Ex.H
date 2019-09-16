//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   C/C++ Parser and Linker.
//

#ifndef	TextScan_CppFrontEnd_H
#define	TextScan_CppFrontEnd_H

#ifndef   Common_LookAsideTable_H
#include  "Common/LookAsideTable.H"
#endif
#ifndef   TextScan_GrammarBasedParser_H
#include  "TextScan/GrammarBasedParser.H"
#endif
#ifndef   TextScan_CppDatabase_H
#include  "TextScan/CppDatabase.H"
#endif
#ifndef   Grammars_CppAppIds_H
#include  "Grammars/Gra_Cpp/CppGrammarAppIds.H"
#endif

enum TCppErrorCode;

class TCppSymbolStateItem;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//   ====== Part 3. ==========  Symbol State Item  ==============
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum TCppSymbolMeaningType
{
	csm_none = 0,

	// Names.
	csm_simp_or_qual,					// SimpleOrQualifiedId, ExtendedId, ExpressionId, SpecialUnqualifiedId, TemplateId, OperatorFunctionId, ConversionFunctionId.
										// This meaning includes all forms of the name including all possible ExpressionId forms.
	// Declarations.
	csm_declaration,					// BlockDeclaration, SimpleDeclaration, InitDeclaratorsList, InitDeclarator, UninitedDeclarator, UsingDeclaration, UsingDirective, AsmDefinition, NamespaceAliasDefinition, UninitedCondition.
	csm_declarations_list,				// BlockDeclaration, SimpleDeclaration, InitDeclaratorsList.
	csm_decl_type_spec,				// DeclTypeSpecifier, TypeSpecifier, SimpleTypeSpecifier, NamedTypeSpecifier, ElaboratedTypeSpecifier.
	csm_decl_prefix,					// DeclPrefix.
	csm_function_attr,					// FunctionSpecifier.
	csm_storage_class,					// StorageClassSpecifier.
	csm_built_in_type_spec,				// BuiltInTypeSpecifier.
	csm_linkage_spec_header,			// LinkageSpecHeader.

	// Declarators.
	csm_declarator,						// Declarator, DirectDeclarator, AbstractDeclarator, DirectAbstractDeclarator, ConversionDeclarator, NewDeclarator, DirectNewDeclarator.
	csm_ptr_operator,					// PtrOperator.
	csm_cv_qualifier,					// CvQualifier.
	csm_func_params_list,				// ParameterDeclarationHeader, ParameterDeclarationClause.
	csm_func_parameter,				// UninitedParameter.
	csm_exceptions_list,					// ExceptionsSpecification, ExceptionTypeIdsList.
	csm_data_type,						// TypeId, EnumSpecifier, ClassSpecifier, ConversionTypeId, NewTypeId.
	csm_initializer,						// Initializer, InitializerClause, InitializersList.

	// Namespaces and enums.
	csm_namesp_header,				// NamespaceHeader.
	csm_enum_header,					// EnumeratorHeader.

	// Classes.
	csm_class_header,					// ClassHeader.
	csm_class_key,						// ClassKey.
	csm_access_spec,					// AccessSpecifier.
	csm_base_classes_list,				// BaseClassSpecifiersList.
	csm_base_class_spec,				// BaseClassSpecifier.

	// Templates.
	csm_template_header,				// TemplateDeclarationHeader, ExplicitInstantiationHeader.
	csm_template_params_list,			// TemplateParamsListHeader.

	// Functions.
	csm_function_header,				// FunctionHeader.
	csm_overloadable_operator,			// OverloadableOperator.

	// Statements.
	csm_statement,						// Almost all statements have this meaning.
	csm_block_header,					// CompoundStatementHeader, IF, SWITCH, FOR, WHILE, DO, TRY (function try block only), ":" (member inits list).
	csm_do_statement_body,			// DoStatementBody.
	csm_catch_blocks_seq,				// ExceptionHandlersSeq.
	csm_catch_block,					// ExceptionHandler.
	csm_exception_param,				// ExceptionHandlerHeader.

	// Expressions.
	csm_expression,						// Almost all expressions have this meaning.
	csm_assignment_operator,			// AssignmentOperator.
	csm_unary_operator,				// UnaryOperator.
	csm_strings_seq,					// StringsSeq.
};

//
//  Name info structure describes a simple name or the state of assembling the qualified name.
//  Note that this structure does not contain any info about existing definitions if any with this name.
//
struct TCppNameInfo
{
	TCppNameInfoCore		m_core;

	TList						*m_template_args_list;						// The pointer IS owned. The value of the pointer is NULL for all non TemplateId froms of the name.
																		// When the name belongs to one of the TemplateId forms and the list of template args in the source
																		// code is empty, the value of the field is still NULL. Once the list is not NULL, it should be not empty.
																		// Empty lists are not allowed here.
	wchar_t					*m_cached_short_name;						// For identifier and non template ctor this is a copy of the name that is stored in the srtings dictionary
																		// of the cpp database. For all other types of names this is a generated description of the current
																		// contents of the structure.

	TCppSymbolStateItem		*m_unproc_template_header;				// This field stores the state of processing the template headers. The value of this field is NULL if
																		// the current context does not contain template headers or if all available template headers are already
																		// consumed while assembling the qualified name.

	TCppDefnBase			*m_resolved_defns_layer;					// Pointer to a published object.

	TList						*m_unresolved_names_list;					// The pointer IS owned. Once the pointer is not NULL, the list should be not empty.

	TCppDefnProblem			m_qual_name_res_problem;

	ID						m_preferred_object_id;						// In the normal situation the value of this field is zero. When processing detects some error
																		// like an unappropriate type of the name for the current context or a names collision, it places
																		// zero into the m_key_id field, places generated unique name into the m_cached_name field
																		// and sets the non zero value into the preferred id field. This identifier should be assigned to
																		// the new object if parsing will decide to create an object out of this name info.

	PrEventHeader			m_short_name_context;						// Area of the final part of the name.
	PrEventHeader			m_qual_name_context;						// The whole area of the name includes the qual/unres part or the name if any and the final part.
																		// When the name info describes a simp or qual symbol, this field duplicates the area, stored in
																		// the base class of the Cpp symbol state item. Nevertheless this field is still needed becuase this
																		// structure is used in other places also.
								//
								// Both context headers are needed only for issuing syntax errors and creating source code links.
								//

	inline void	Clear() { memset(this, 0, sizeof(TCppNameInfo)); }

	inline ID		KeyId() const { return(m_core.m_key_id); }
	inline bool	IsEmpty() const { return(m_core.m_info_type == cnit_none); }
	inline bool	IsIdentifier() const { return(m_core.m_info_type == cnit_identifier); }
	inline bool	IsTempalteId() const { return(m_core.m_info_type == cnit_template_id); }
	inline bool	IsIdentifierOrTempalteId() const { return(m_core.m_info_type == cnit_identifier || m_core.m_info_type == cnit_template_id); }
	inline bool	IsSimpleIdentifier() const { return(m_core.m_info_type == cnit_identifier && m_resolved_defns_layer == NULL && m_unresolved_names_list == NULL); }
	inline bool	IsQualified() const { return(m_resolved_defns_layer != NULL || m_unresolved_names_list != NULL); }
	inline bool	IsUnresolvedLayer() const { return(m_unresolved_names_list != NULL); }
	inline bool	TemplateArgsListPresent() const { return(g_TemplateIdNameForm[m_core.m_info_type]); }

	inline TList			*TemplateHeaderParams() const;
	inline TCppDefnBase	*GetDefnsLayer(TCppDefnBase *curr_defns_layer) const { return((m_resolved_defns_layer != NULL) ? m_resolved_defns_layer : curr_defns_layer); }
	inline bool			GetSearchHierFlag() const { return(m_resolved_defns_layer == NULL); }

	void		SetupName(TCppNameInfoType ident_based_name_type, ID key_id);
	void		SetupOverloadedOperator(TCppNameInfoType ovld_op_type, TOperatorToken ovld_operator);
	void		SetupOverloadedConversion(TCppNameInfoType ovld_conv_type, TCppDataTypeBase *ovld_conv_dest);
	void		FinalizeShortNameInfo(TSymbolStateItem &new_non_term, wchar_t *new_cached_name, TList *template_args_list = NULL);

	void		SetupLayerResolutionProblem(TList *initial_unres_names_list, TCppDefnProblem layer_problem);
	void		AddUnresolvedLayerInfo(TCppUnresolvedNameInfo *unres_name_info) { assert(m_unresolved_names_list != NULL); m_unresolved_names_list->AppendItem(unres_name_info); }
	bool		FinalizeLayerResProblemName(TCppParser *inst);

	void		DowngradeToUnqualifiedNameInfo(TCppParser *inst);
	void		DowngradeTemplateIdToIdentifier(TCppParser *inst);
	bool		DowngradeExpressionIdToIdentifier(TCppParser *inst, const wchar_t *subst_name_prefix);
	bool		SetupShortNameDefnProblem(TCppParser *inst, TCppDefnProblem name_problem);

	void		CheckRemoveTemplateDefnTemplateArgs(TCppParser *inst);
				// This method checks if current template args are literal repetion of templare params from the current
				// unprocessed template header or not. If this is so, method cancels these template arguments.

	void		SetNameToObject(TCppDefnBase *defn) const;
	void		SetNameToFunctionObject(TCppFunctionTypeDefn *func_defn) const;

	static const wchar_t	*GetDefnProblemNamePrefix(TCppDefnProblem cdpr);
	wchar_t				*GetCashedNonTemplateIdName(TCppParser *inst) const;

	const wchar_t			*GetFuncNameLs(wchar_t *buff96, TCppDefnBase *dest_layer, bool func_hdr_ctx) const;
	const wchar_t			*GetFuncNameLp(wchar_t *buff96, TCppDefnBase *dest_layer, bool func_hdr_ctx) const;
	const wchar_t			*GetFuncNameCs(wchar_t *buff96, TCppDefnBase *dest_layer, bool func_hdr_ctx) const;
	const wchar_t			*GetFuncNameCp(wchar_t *buff96, TCppDefnBase *dest_layer, bool func_hdr_ctx) const;
	const wchar_t			*GetFuncObjectTypeName(wchar_t *buff96, bool want_caps, bool want_plurals, TCppElabType elab_type, bool func_hdr_ctx) const;

	static const bool		g_TemplateIdNameForm[cnit_num_types];

	void		ReleaseResources(TCppParser *inst);
	void		ReleaseTemplateArgsList(TCppParser *inst);
};

struct TCppBuiltInTypeInfo
{
	bool						m_signed_spec_present;
	bool						m_unsigned_spec_present;
	bool						m_short_spec_present;
	bool						m_long_spec_present;
	bool						m_long_long_present;

	TCppBuiltInTypeType		m_primary_type;
	bool						m_pri_type_long;							// This special flag is used for marking wchar_t and double that
																		// are mapped to the built in primary types "char" and "float".
	bool						m_pri_type_present;

public:

	void						Clear() { memset(this, 0, sizeof(TCppBuiltInTypeInfo)); }
	TCppErrorCode			AddBuiltInTypeSpec(WORD terminal_symbol_app_id);
								// Adding any valid specifier makes the structure non empty.

	bool						IsEmpty() const { return(m_pri_type_present == FALSE && m_signed_spec_present == FALSE && m_unsigned_spec_present == FALSE && m_short_spec_present == FALSE && m_long_spec_present == FALSE); }
								// First check the flag that is most likely to be set.

	TCppBuiltInTypeIdent		GetBuiltInTypeIdent() const;
								// The structure is always in a consistent state because it is expected to be built using
								// only the methods above. This means that the resulting type is always valid. An empty
								// struct corresponds to the void type.
private:

	TCppBuiltInTypeIdent		AssembleIntegerTypeFromModifiers() const;
};

//
//  DeclTypeSpecifier describes the left part of the definition, like:
//
//		static const int		x1, *x2, x3[10];
//
//  In this example "static const int" is the left part. The right part contains a sequence of declarators.
//
//  The following situations with the base type are possible:
//
//	  --	 There is no base type info at all.
//	  --	 Built in type. The m_dclt_blt_in_info struct is not empty.
//	  --	 Named type specifier. The m_dclt_unres_name_info is not empty, the elab type is "elab_none".
//	  --	 Elaborated type specifier. The m_dclt_unres_name_info is not empty, the elab type is also non empty.
//	  --	 Locally defined enum or class (class template, class template spec). The m_dclt_local_enum_class is TRUE.
//
struct TCppDeclTypeSpecInfo
{
	TCppDeclTypeSpecPrefix			m_dclt_prefix;
	TCppFunctionAttr					m_dclt_func_attrs;
	TCppStorageClass					m_dclt_strg_class;					// Value of the strg class keyword as it was specified in the source code.

	TCppCvPtrRefSpec					m_dclt_cv_spec;						// This field can contain only const or volatile flags or both. Pointer and reference
																		// flags are not possible here. Logically these flags are more a part of the declarator.
	TCppBuiltInTypeInfo				m_dclt_blt_in_info;					// This is state of assembling the built in type specifiers if any.

	TCppElabType						m_dclt_unres_name_elab;			// Elaboration of the name in the m_dclt_unres_name_info field.
	TCppNameInfo					m_dclt_unres_name_info;			// This field stores either a name of the named type spec or a name of the elaborated
																		// type specifier. To check if this field contains anything or not the metod IsEmpty()
																		// should be used.
	TCppDefnBase					*m_dclt_base_type;					// This field should point either to an object derived from the TCppDataTypeBase or
																		// to an object derived from the TCppFunctionTypeBase. In rare cases this field can
																		// also point to a class template definition or a class template specialization.
	bool								m_dclt_local_enum_class;			// When the value of this field is TRUE, this means that either enum or class were defined
																		// during constuction of the non terminal.

	inline void Clear() { memset(this, 0, sizeof(TCppDeclTypeSpecInfo)); }
	inline bool IsBaseTypeSpecPresent() const { return(m_dclt_blt_in_info.IsEmpty() == FALSE || m_dclt_unres_name_info.IsEmpty() == FALSE || m_dclt_base_type != NULL); }
};

//
//  Current state of assembling the declarator. This state contains the name of the declarator if it is not
//  abstract and an array of modifiers that are already applied to this name. Note that an abstract declarator
//  cannot have empty array of modifiers because when the TypeId or one of its flavors is created directly
//  from the TypeSpecifier the data type is created directly from the TCppDeclTypeSpecInfo structure
//  without creating an intermediate declarator.
//
struct TCppDeclaratorInfo
{
	bool								m_new_expr_decl;					// This flag allows specifying arrays with non constant value in the first
																		// dimension in the first sequence of array modifiers.
	TCppNameInfo					m_name_info;

	TCppDeclaratorModifiersArray		*m_modifiers;						// This field is a pointer because the whole structure is used as a field of the union
																		// that stays inside the big cpp symbol state structure. This means that all fields of
																		// this struct should not have ctors. This struct owns resoures in this pointer.
	inline void Clear() { memset(this, 0, sizeof(TCppDeclaratorInfo)); }

	inline bool IsAbstractDeclarator() const { return(m_name_info.m_core.m_info_type == cnit_none); }
	inline bool IsCodeEntryPointStyle() const { return(m_modifiers != NULL && m_modifiers->ItemRef(0).m_dmt == dclmdf_function); }
};

//
// This struct describes current state of parsing the enum definition.
//
struct TCppEnumHeaderInfo
{
	TCppEnumTypeDefn				*m_enum_defn;						// Enum object that is currently being defined.
																		// The pointer is not owned.
	__int64							m_next_value;

	ID								m_object_intro_ctv_event;
};

//
// This struct describes curr state of the struct/union/class definition parsing.
//
struct TCppStructHeaderInfo
{
	TCppStructTypeDefn				*m_struct_defn;						// The struct type that is currently being defined. The pointer is not owned.
																		// Note that this can also be a class template or a class template specialization.
	TCppAccessLevel					m_curr_accs_level;

	ID								m_object_intro_ctv_event;
};

struct TCppTemplateHeaderInfo
{
	bool								m_export;							// Keyword EXPORT stays in front of the template header.

	TList								*m_parameters_list;					// The pointer in this field is NULL when template header describes an explicit instantiation.
																		// Pointer can point to an empty list when the partial specialization specializes all parameters
																		// of the template. All three cases are possible here: NULL, empty list, non empty list.

	bool								m_header_processed;				// This flag is set when parser decides that template header is applicable to the current context
																		// or after reporting a syntax error, like detecting a template hdr in front of the typedef or enum.

	TCppDefnBase					*m_header_layer_owner;			// This is the definitions owner at the point where the template header was scanned.
																		// This field allows to distinguish between headers and nested class headers.

	TCppSymbolStateItem				*m_parent_template_header;
	TCppSymbolStateItem				*m_child_template_header;
										// These two fields form a sort of a double linked list. The operations with this list are limited.
										// New template header is always added to the end of the list and the whole list is destroyed
										// when headers are removed from the stack of the parser. There are no other operations.

	inline void Clear() { memset(this, 0, sizeof(TCppTemplateHeaderInfo)); }
};

//
// This struct is used for handling function param lists, template header param lists and template id argument
// lists. The type of the list can be determined from the m_meaning field of the symbol state structure.
//
struct TCppParamsListInfo
{
	TList								*m_params_list;						// The list pointer is NULL when function or class tempate or func template or
																		// template instantiation has no params. Once the list is not NULL, it should be
																		// not empty. Empty list is not possible here.
	TCppSymbolStateItem				*m_parent_params_symbol;			// Opened parameter lists can nest into each other. This data field creates
																		// a single linked list of currently opened param lists.
};

struct TCppBLockInfo
{
	TCppBlockDefn					*m_block_header;
	bool								m_block_object_published;
};

//
// When the body of the DO-WHILE loop is processed, it may be necessary to create an implicit containing block
// that encloses the body of the loop. At the same time when the whole structure will be assembled and published,
// the body of the block will become a child of the TCppDoWhileStatement object that is not yet created
// when the body of the loop is processed.
//
struct TCppDoStatementInfo
{
	TCppBlockDefn					*m_do_stmt_block_header;
	TCppStatementBase				*m_do_stmt_loop_body;
};

struct TCppLinkageSpecInfo
{
	TCppLinkageSpec					*m_link_spec;						// This pointer is always not NULL and it is not owned.
	TCppSymbolStateItem				*m_parent_spec_layer;				// Pointer to the parent can be NULL.
};

struct TCppStringsSeqInfo
{
	TLexStringType					m_string_subt;
	TStrPtrInfo						m_string_value;						// Pointer in this field points to a non NULL terminated unicode string.
																		// Length of the string should be determined using the m_len field.
};

class TCppSymbolStateItem : public TSymbolStateItem
{
	bool		GetAppLayerDescription(TDestinationFile &doc);

protected:

	TCppSymbolStateItem() { m_meaning = csm_none; }
	~TCppSymbolStateItem() { assert(m_meaning == csm_none); }

	inline void	ResetMeaning() { m_meaning = csm_none; }
					// This method should be called from TCppParser rule handling methods when all owned resources are
					// already removed from the object. For perf reasons the the app fields themselves are not cleared.

protected:

	TCppSymbolMeaningType				m_meaning;
											//
											// This data field is used as a key to determine what field from the big union below should be
											// checked and/or set. Object cleanup is done from inside the TCppParser::RecycleSymbolStateItem().
											// The code is written this way primarily for perf reasons.
											//
	union
	{
		// Names.
		TCppNameInfo					m_simp_or_qual;				// csm_simp_or_qual

		// Declarations.
		TCppStatementBase				*m_declaration;					// csm_declaration
		TCppLightBlockDefn				*m_declarations_list;			// csm_declarations_list
		TCppDeclTypeSpecInfo				m_decl_type_spec_info;			// csm_decl_type_spec
		TCppDeclTypeSpecPrefix			m_decl_prefix;					// csm_decl_prefix
		TCppFunctionAttr					m_function_attr;					// csm_function_attr
		TCppStorageClass					m_storage_class;				// csm_storage_class					// Value of the storage class keyword.
		WORD							m_bltin_spec_app_id;			// csm_built_in_type_spec
		TCppLinkageSpecInfo				m_linkage_spec;					// csm_linkage_spec_header

		// Declarators.
		TCppDeclaratorInfo				m_declarator_info;				// csm_declarator
		TCppCvPtrRefSpec					m_ptr_operator;					// csm_ptr_operator
		TCppCvPtrRefSpec					m_cv_qualifier;					// csm_cv_qualifier
		TCppParamsListInfo				m_func_params_info;			// csm_func_params_list
		TCppDataFieldDefn					*m_func_parameter;				// csm_func_parameter					// Func param before checking for presence of the default value.
		TList								*m_exceptions_list;				// csm_exceptions_list					// The pointer cannot be NULL.
		TCppDataTypeBase				*m_data_type;					// csm_data_type
		TCppItemBase					*m_initializer_data;				// csm_initializer

		// Enums.
		TCppEnumHeaderInfo				m_enum_info;					// csm_enum_header

		// Classes.
		TCppElabType						m_class_key;					// csm_class_key
		TCppAccessLevel					m_access_spec;					// csm_access_spec
		TList								*m_base_classes_list;			// csm_base_classes_list
		TCppBaseClassSpec				*m_base_class_spec;			// csm_base_class_spec

		// Templates.
		TCppTemplateHeaderInfo			m_template_info;				// csm_template_header
		TCppParamsListInfo				m_template_params_info;		// csm_template_params_list			// This filed describes either params of a template header
																											// or arguments of a template specialization/instantiation.
		// Functions.
		TCppCodeEntryPointDefn			*m_function_header;				// csm_function_header
		TOperatorToken					m_overloadable_operator;		// csm_overloadable_operator

		// Statements.
		TCppStatementBase				*m_statement;					// csm_statement
		TCppLightBlockDefn				*m_catch_blocks_seq;			// csm_catch_blocks_seq
		TCppCatchBlockDefn				*m_catch_block;					// csm_catch_block
		TCppDataFieldDefn					*m_exception_param;			// csm_exception_param

		// Expressions.
		TCppExpressionBase				*m_expression;					// csm_expression
		TOperatorToken					m_assgn_operator;				// csm_assignment_operator
		TOperatorToken					m_unary_operator;				// csm_unary_operator
		TCppStringsSeqInfo				m_strs_seq_value;				// csm_strings_seq

		// ---------------------------------------------------

		// Symbol meanings that can own a definition layer.
		struct
		{
			union
			{
				TCppNamespaceDefn		*m_namesp_defn;				// csm_namesp_header
				TCppStructHeaderInfo		m_struct_info;					// csm_class_header
				TCppBLockInfo			m_block_info;					// csm_block_header
				TCppDoStatementInfo		m_do_stmt_info;					// csm_do_statement_body
			};

			TCppDefnBase				*m_parent_layer_owner;			// This field cannot be NULL because every level of definitons has
																		// an owner, even the global namespace.
			TCppSymbolStateItem			*m_parent_layer_symbol;		// This field is NULL when parent layer is the global namespace.
		};

	};

	friend struct TCppNameInfo;
	friend class TCppParser;
};

struct TCppErrorReportingCtxSymbolStateItem : public TSymbolStateItem
{
	TCppErrorReportingCtxSymbolStateItem(PrEventHeader &context)
	{
		m_symbol = NonTerminalsBase;
		m_non_term_origin = context;
	}

	TCppErrorReportingCtxSymbolStateItem(TCppNameInfo &context_name_info, bool want_full_context = FALSE)
	{
		m_symbol = NonTerminalsBase;
		m_non_term_origin = (want_full_context == TRUE) ? context_name_info.m_qual_name_context : context_name_info.m_short_name_context;
	}
};

struct TCppParserCtvRefEventInfo : protected TCppCtvRefEventInfoBase
{
		inline TCppParserCtvRefEventInfo() { m_owner_parser = NULL; }
		virtual void RecycleEventInfo();
public:

	PrEventHeader				m_main_header;
	TCppSourceAreaHeadersArray	m_ext_headers;

	TCppParser					*m_owner_parser;
};

inline TList *TCppNameInfo::TemplateHeaderParams() const
{
	if (m_unproc_template_header == NULL)
		return(NULL);

	return(m_unproc_template_header->m_template_info.m_parameters_list);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//   ====== Part 4. ============  Cpp Parser  ================
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Ranges of the error code values are specified in TParsingErrorCodeBases enum.
enum TCppErrorCode
{
	cerr_none = 0,
	cerr_base = 1500,

	// Common error reporting.
	cerr_error_adding_event_link				= 1508,
	cerr_ambig_defns_array_item				= 1510,			// %d, %s, %s.

	// Assembling the decl type specifier.
	cerr_dts_decl_prefix_dupe					= 1518,			// %s.
	cerr_dts_func_attr_dupe						= 1519,			// %s.
	cerr_dts_strg_class_dupe					= 1520,			// %s.
	cerr_dts_const_spec_dupe					= 1521,
	cerr_dts_volatile_spec_dupe					= 1522,
	cerr_dts_bad_built_in_spec					= 1523,
	cerr_dts_bad_named_type					= 1524,
	cerr_dts_bad_elab_type_spec				= 1525,
	cerr_dts_bad_enum_or_class					= 1526,			// %s.
	cerr_dts_throws_type_duplication			= 1527,			// %s.

	// Special forms of the simple declaration and other misc.
	cerr_sdecl_bogus_typedef_prefix				= 1530,
	cerr_sdecl_bogus_func_attr					= 1531,			// %s.
	cerr_sdecl_bogus_strg_class					= 1532,			// %s.
	cerr_sdecl_bogus_cv_spec					= 1533,
	cerr_sdecl_bogus_friend_spec				= 1534,			// %s.
	cerr_sdecl_bogus_built_in_type				= 1535,
	cerr_sdecl_bogus_named_type				= 1536,
	cerr_sdecl_bogus_exinst_hdr_missing		= 1537,
	cerr_sdecl_bogus_type_missing				= 1538,

	cerr_sdecl_bogus_template_spec,

	// Picking up name info from the current context.
	cerr_context_extern_with_initializer			= 1540,
	cerr_context_bad_initializer					= 1541,
	cerr_context_bad_friend						= 1542,
	cerr_context_name_occupied					= 1543,			// %s, %s.

	// Assembling the built in type name.
	cerr_bltin_sign_spec_twice					= 1550,
	cerr_bltin_size_spec_twice					= 1551,
	cerr_bltin_sign_spec_with_bad_type			= 1552,
	cerr_bltin_size_spec_with_bad_type			= 1553,
	cerr_bltin_bad_type_for_sign_spec			= 1554,
	cerr_bltin_bad_type_for_size_spec			= 1555,
	cerr_bltin_double_with_long_long				= 1556,
	cerr_bltin_triple_long_spec					= 1557,
	cerr_bltin_type_already_present				= 1558,
	cerr_bltin_unknown_symbol_app_id			= 1559,

	// Enums.
	cerr_enum_bad_template_args				= 1560,
	cerr_enum_base_type_non_numeric			= 1561,
	cerr_enum_base_type_enum_type			= 1562,
	cerr_enum_base_type_func_type				= 1563,
	cerr_enum_member_expr_not_const			= 1564,
	cerr_enum_member_expr_not_numeric		= 1565,

	// Classes.
	cerr_struct_member_init_not_applic			= 1570,			// %s.
	cerr_struct_bft_no_decl_type_spec			= 1571,
	cerr_struct_bft_bad_decl_prefix				= 1572,			// %s.
	cerr_struct_bft_bad_func_attr				= 1573,			// %s.
	cerr_struct_bft_bad_strg_class				= 1574,			// %s.
	cerr_struct_bft_base_type_not_numeric		= 1575,
	cerr_struct_bft_base_type_func_type			= 1576,
	cerr_struct_base_class_duplication			= 1577,			// %s.

	// Errors in templates and functions.
	cerr_special_func_not_in_class				= 1584,

	// Errors in statements.
	cerr_label_is_undefined,						// %s.
	cerr_label_already_defined,					// %s.
	cerr_case_label_not_const,
	cerr_case_label_not_numeric,

	cerr_condition_bad_qual_path,
	cerr_condition_not_simple_name,
	cerr_condition_defines_array,

	// Errors in expressions.
	cerr_expr_lvalue_missing					= 1600,
	cerr_expr_const_missing						= 1601,
	cerr_expr_this_outside_of_func				= 1602,
	cerr_expr_this_in_namesp_func				= 1603,
	cerr_expr_this_in_static_method				= 1604,			// %s.
	cerr_expr_base_in_static_method			= 1605,

	cerr_expr_typeid_type_info_missing			= 1606,
	cerr_expr_typeid_type_info_problem			= 1607,

	// Assembling sequence of literal strings.
	cerr_strings_seq_subt_mismatch				= 1630,

	//
	//  Environment methods.
	//

	cerr_object_already_defined					= 1653,			// %s, %s, %s.

	cerr_elab_defn_in_template_inst				= 1654,			// %s.
	cerr_elab_defn_on_upper_layer				= 1655,			// %s.
	cerr_elab_defn_names_conflict				= 1656,			// %s, %s.
	cerr_elab_defn_already_existing				= 1657,			// %s, %s, %s.
	cerr_elab_defn_on_lower_layer				= 1658,			// %s.
	cerr_elab_defn_on_unrelated_layer			= 1659,			// %s.

	cerr_data_field_in_template_inst,
	cerr_data_field_on_upper_layer,
	cerr_data_field_names_conflict,				// %s.
	cerr_data_field_name_duplication,			// %s.
	cerr_data_field_lo_layer_extern,
	cerr_data_field_lo_layer_mutable,			// %s.
	cerr_data_field_on_lower_layer,
	cerr_data_field_lo_namesp_dupe,			// %s.
	cerr_data_field_lo_class_inst_dupe,			// %s, %s.
	cerr_data_field_lo_class_sttc_dupe,			// %s, %s.
	cerr_data_field_on_unrelated_layer,

	cerr_func_obj_friend_proto_ambig,			// %d.
	cerr_func_obj_friend_proto_unappr,			// %s, %s.
	cerr_func_obj_in_template_inst,				// %s.
	cerr_func_obj_on_upper_layer,				// %s.
	cerr_func_obj_names_conflict,				// %s, %s.
	cerr_func_obj_name_duplication,				// %s, %s %s.
	cerr_func_obj_on_lower_layer,				// %s.
	cerr_func_obj_on_unrelated_layer,			// %s.

	cerr_defns_search_name_ambig				= 1710,			// %s, %d.
	cerr_defns_search_not_namespace			= 1711,			// %s.
	cerr_defns_search_not_named_type			= 1712,			// %s.
	cerr_defns_search_cttp_template_args		= 1713,
	cerr_defns_search_cttp_no_overload			= 1714,			// %s.
	cerr_defns_search_cttp_not_template			= 1715,			// %s.
	cerr_defns_search_not_class_member		= 1716,			// %s, %s.
	cerr_defns_search_not_operand				= 1717,			// %s.

	cerr_ptr_operator_not_struct,				// %s.
	cerr_context_not_class_name,				// %s.

	cerr_defns_search_bad_template_args		= 1720,
	cerr_defns_search_unknown_name,			// %s.

	cerr_template_hdr_not_allowed				= 1730,			// %s.
	cerr_template_hdr_unexpected				= 1731,
	cerr_template_hdr_with_empty_defn1		= 1732,
	cerr_template_hdr_with_empty_defn2		= 1733,

	cerr_decltor_array_dim_not_numeric			= 1740,
	cerr_decltor_array_dim_not_constant			= 1741,
	cerr_decltor_array_negative_dim				= 1742,			// %I64d.
	cerr_decltor_array_zero_first_dim			= 1743,
	cerr_decltor_array_zero_sec_dim			= 1744,
	cerr_decltor_array_elem_func				= 1745,

	cerr_decltor_base_func_type_cv				= 1747,
	cerr_decltor_func_retval_func				= 1748,
	cerr_decltor_result_func_type				= 1749,

	cerr_case_label_dupl_default,
	cerr_case_label_dupl_value,					// %I64d.
	cerr_case_label_outside_switch,

	cerr_param_name_not_ident					= 1760,
	cerr_param_name_bad_qual_path			= 1761,
	cerr_param_name_duplication				= 1762,

	cerr_base_type_missing						= 1767,
	cerr_struct_template_params_missing		= 1772,			// %s.

	cerr_templ_spec_no_templ_header			= 1776,			// %s.
	cerr_templ_spec_expl_inst_header			= 1777,			// %s.
	cerr_templ_spec_base_classes				= 1778,			// %s.

	cerr_elab_fwd_enum_templid					= 1780,
	cerr_elab_fwd_templ_params_missing		= 1781,			// %s.
	cerr_elab_fwd_find_name_ambig				= 1782,			// %d.
	cerr_elab_fwd_names_conflict				= 1783,			// %s, %s.

	cerr_elab_fwd_in_template_inst				= 1784,			// %s.
	cerr_elab_fwd_on_upper_class_layer			= 1785,			// %s.
	cerr_elab_fwd_on_lower_layer				= 1786,			// %s.
	cerr_elab_fwd_on_unrelated_layer			= 1787,			// %s.

	cerr_friend_rec_obj_on_same_layer			= 1788,			// %s.

	cerr_bft_non_const_expr						= 1790,			// %s.
	cerr_bft_non_numeric_expr					= 1791,			// %s.
	cerr_bft_negative_const_expr				= 1792,			// %s, %I64d.
	cerr_bft_zero_const_expr					= 1793,
	cerr_bft_too_big_const_expr					= 1794,			// %s, %I64d.

	cerr_data_field_bad_friend,
	cerr_data_field_bad_func_attr,				// %s.
	cerr_data_field_bad_strg_class,				// %s, %s.
	cerr_data_field_base_type_missing,
	cerr_data_field_bad_template_args,
	cerr_data_field_diff_linkage_specs1,			// %s.
	cerr_data_field_diff_linkage_specs2,			// %s, %s.
	cerr_data_field_diff_data_types,				// %s.

	cerr_typedef_bad_func_attr,					// %s.
	cerr_typedef_bad_strg_class,				// %s.
	cerr_typedef_bad_qual_path,
	cerr_typedef_bad_template_args,

	cerr_func_dts_bad_typedef					= 1820,			// %s.
	cerr_func_dts_bad_friend_context			= 1821,
	cerr_func_dts_bad_virtual_context			= 1822,
	cerr_func_dts_bad_virtual_ctor				= 1823,			// %s.
	cerr_func_dts_bad_no_ctor_explicit			= 1824,
	cerr_func_dts_bad_static_ctor				= 1825,
	cerr_func_dts_bad_static_dector				= 1826,
	cerr_func_dts_bad_static_ovld_conv			= 1827,
	cerr_func_dts_extern_in_class				= 1828,			// %s.
	cerr_func_dts_extern_with_method			= 1829,			// %s.
	cerr_func_dts_bad_strg_class				= 1830,			// %s, %s.

	cerr_func_props_diff_linkage_specs1,			// %s, %s.
	cerr_func_props_diff_linkage_specs2,			// %s, %s, %s.
	cerr_func_props_diff_access_levels,			// %s, %s, %s.
	cerr_func_props_unexpected_func_attr,		// %s, %s.
	cerr_func_props_unexpected_this_spec,		// %s, %s.
	cerr_func_props_missing_this_spec,			// %s, %s.

	cerr_func_decl_base_type_missing			= 1840,			// %s.
	cerr_func_decl_func_params_missing			= 1841,			// %s.
	cerr_func_decl_namesp_func_this_spec		= 1842,			// %s.
	cerr_func_decl_static_method_this_spec		= 1843,			// %s, %s.

	cerr_func_decl_templ_params_missing		= 1844,			// %s.
	cerr_func_decl_function_unxpct_static		= 1846,			// %s, %s.
	cerr_func_decl_method_unxpct_static			= 1847,			// %s, %s, %s.
	cerr_func_decl_ret_value_mismatch			= 1848,

	cerr_typeless_func_bad_const_spec			= 1850,			// %s.
	cerr_typeless_func_bad_volatile_spec		= 1851,			// %s.
	cerr_typeless_func_bad_base_type			= 1852,			// %s.

	cerr_typeless_func_ctor_this_spec			= 1853,			// %s.
	cerr_typeless_func_dector_this_spec			= 1854,			// %s.
	cerr_typeless_func_dector_bad_params		= 1855,
	cerr_typeless_func_oconv_bad_params		= 1856,
	cerr_typeless_func_no_templ_params		= 1857,			// %s.

	cerr_typeless_func_dector_in_block			= 1860,
	cerr_typeless_func_dector_on_namesp		= 1861,
	cerr_typeless_func_dector_bad_name		= 1862,			// %s, %s.
	cerr_typeless_func_oconv_in_block			= 1863,
	cerr_typeless_func_oconv_on_namesp		= 1864,

	cerr_defns_layer_name_ambig				= 1880,			// %d.
	cerr_defns_layer_bad_template_args			= 1881,
	cerr_defns_layer_bad_template1				= 1882,
	cerr_defns_layer_bad_template2				= 1883,
	cerr_defns_layer_name_unuseable			= 1884,			// %s.
	cerr_defns_layer_name_unknown				= 1885,

	cerr_typeless_decl_no_params_ctor			= 1940,
	cerr_typeless_decl_no_params_dector		= 1941,
	cerr_typeless_decl_no_params_conv			= 1942,
	cerr_typeless_decl_bad_mdfs_ctor			= 1943,			// %s.
	cerr_typeless_decl_bad_mdfs_dector			= 1944,			// %s.
	cerr_typeless_decl_bad_mdfs_conv			= 1945,			// %s.

	cerr_oom_decltor_cvpr_modifier				= 1961,
	cerr_oom_decltor_array_modifier			= 1962,
	cerr_oom_decltor_func_modifier				= 1963,
	cerr_oom_decltor_modifiers_layer			= 1964,
};

// Ranges of the error code values are specified in TParsingErrorCodeBases enum.
enum TCppCriticalErrorCode
{
	// Misc internal compiler problems.
	ccerr_misc_error_in_grammar				= 2201,
	ccerr_misc_not_implemented					= 2202,			// %s.
	ccerr_missing_non_term_app_id				= 2203,			// %s, %hd.
	ccerr_unexpected_non_term_app_id			= 2204,			// %hd, %s, %hd.
	ccerr_unexpected_gram_rule_app_id			= 2205,			// %hd, %s, %hd, %hd.
	ccerr_unexpected_rule_sym_app_id			= 2206,			// %hd, %s, %hd, %hd.
	ccerr_unexpected_rule_sym_meaning			= 2207,			// %hd, %s, %hd, %d.

	// Structural problems.
	ccerr_unhandled_template_header			= 2210,

	// Processing names.
	ccerr_key_id_picking_problem				= 2214,			// %.*s.
	ccerr_oom_storing_name_string				= 2215,			// %d.
	ccerr_oom_generating_object_name			= 2216,			// %s.
	ccerr_oom_setting_alt_name					= 2217,			//

	// Misc allocations.
	ccerr_oom_decltor_ptr_to_func				= 2223,
	ccerr_oom_general_purpose_list				= 2224,
	ccerr_oom_unsubm_ctv_event_info			= 2225,
	ccerr_oom_combining_literal_strings			= 2226,
	ccerr_oom_storing_literal_string				= 2227,			// %d.
	ccerr_oom_creating_cpp_object				= 2228,			// %s.
	ccerr_oom_creating_unres_object			= 2229,

	ccerr_max									= 2299,
};

//
//    Major parser class
// -------------------------
//
class TCppParser : public TGrammarBasedParser
{
public:
			TCppParser(TCppDatabase &cpp_database, TCppObjectsFactory &cpp_factory, TGrammar &cpp_grammar);
			~TCppParser();

	void		ResetScalarFields();

	//
	// Inherited virtual methods.
	//

	TParserNamesTableViewerType	GetNamesTableViewerCode() const { return(pntvc_cpp); }

	void							ResetParser();
	bool							Parse();

	THighLevelLangType			GetParserLangType() const { return(lang_cpp); }
	const wchar_t					*GetParserDescription(wchar_t *buff_80_chars) const { return(L"TCppParser (Dec 2011)"); }
	bool							CheckGrammar() const;

	void							ReviewInputSymbol(TSymbolStateItem *item);
	void							ReviewParsingAction(WORD &action);
									//
									// These are important callback methods. They implement syntax error recovery. Second method
									// is responsible for resolving grammar conflicts that should be resolved by the callback handler.
									//

	TSymbolStateItem				*AllocateSymbolStateItem(WORD irule = 0xFFFF);
									//
									// This is the main method of the parser. It actually implements the app specific handling of all rules.
									//

	void							RecycleSymbolStateItem(TSymbolStateItem *item);
									//
									// Method should do the Cpp layer cleanup and pass the structure to the base class.
									//
	//
	// New methods.
	//

	TCppDatabase		&GetDatabase() { return(m_cpp_database); }

	static const wchar_t	*GetErrorText(int err_code);

protected:

	inline TCppSymbolStateItem		*FirstRuleSymPtr(int rule_length) { return((TCppSymbolStateItem*)GetStackSlotPtr(rule_length)); }
	inline TCppSymbolStateItem		*PrevRuleSymPtr(TSymbolStateItem *item) { return((TCppSymbolStateItem*)m_stack.GetPrev(item)); }
	inline TCppSymbolStateItem		*NextRuleSymPtr(TSymbolStateItem *item) { return((TCppSymbolStateItem*)m_stack.GetNext(item)); }
	inline TCppSymbolStateItem		*LastStackSymPtr() { return((TCppSymbolStateItem*)m_stack.GetLast()); }

	inline TCppSymbolStateItem		&FirstRuleSym(int rule_length) { return((TCppSymbolStateItem&)*GetStackSlotPtr(rule_length)); }
	inline TCppSymbolStateItem		&NextRuleSym(TCppSymbolStateItem &item) { return((TCppSymbolStateItem&)*m_stack.GetNext(&item)); }

	TCppSymbolStateItem			*GetFirstTemplateHeaderSymbol();
	TCppCodeEntryPointDefn		*GetOwningFunctionForCodeArea();

	ID		ReportSyntaxError(TSymbolStateItem *err_ctx_sym, TCppErrorCode err_code, ...);
	ID		ReportSyntaxError(TSymbolStateItem *err_ctx_sym, TCppItemBase *related_object, TCppErrorCode err_code, ...);
	ID		ReportSyntaxError(PrEventHeader &err_ctx, TCppItemBase *related_object, TCppErrorCode err_code, ...);
				// Report ing syntax errors from inside the C/C++ parser.

	void		RaiseCriticalCppError(TCppCriticalErrorCode critical_error_code, ...);
	void		RaiseNotImplementedError(const wchar_t *details_msg);
				// Methods raise critical errors from inside the C/C++ parser.

	void		RaiseUnexpectedGrammarRuleAppIdError(WORD irule);
	void		RaiseUnexpectedRuleSymbolAppIdError(WORD irule, TSymbolStateItem *sym);
	void		RaiseUnexpectedRuleSymbolMeaningError(WORD irule, int isym);
				// These methods also raise critical errors.

	void		ReportUnexpectedTemplateHeaders(const wchar_t *non_templateable_obj_type, TCppSymbolStateItem *leftmost_unexpected_header = NULL);
	void		ReportUnprocessedTemplateHeaders(TCppSymbolStateItem *leftmost_unprocessed_header_symbol);
	void		ReportEmptyDefinitionTemplateHeaders();

	void		ReportAmbiguousDefnsArray(TSymbolStateItem *err_ctx_sym, ID main_error_msg_id, TCppDefnBasePtrsArray &ambig_defn_ptrs_array);
	void		ReportUnappropFunctionAttrs(TSymbolStateItem *err_ctx_sym, TCppItemBase *related_object, TCppErrorCode err_code, TCppFunctionAttr attrs_mask);

	void		AddCtvEventRef(PrEventHeader *ctx, TCppItemBase *item, TCppItemSrcLinkType ref_type, ID mldb_event_id);
	ID		WriteObjectIntroEvent(TSymbolStateItem *whole_area_sym, TCppItemSrcLinkType ref_type, TCppItemBase *item, TCppSourceAreaType atype = cxa_none, PrEventHeader *area_header = NULL);
	void		WriteObjectIntroEventUpdate(TSymbolStateItem *whole_area_sym, TCppItemBase *item, ID existing_mldb_event_id);

	//
	//  App methods.
	//

	void		ProcessCppRule(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
				// This is generic method for processing all rules in the grammar.

	//
	// Part 02. Names.
	//

	void		ProcessSimpleOrQualifiedId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessExtendedId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessExpressionId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessSpecialUnqualifiedId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	//
	// Part 03. Declarations.
	//

	void		ProcessDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessBlockDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessSimpleDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	void		ProcessDeclTypeSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessDeclPrefix(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessFunctionSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessStorageClassSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	void		ProcessTypeSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessSimpleTypeSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessBuiltInTypeSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessNamedTypeSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessElaboratedTypeSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	void		ProcessUsingDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessUsingDirective(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessAsmDefinition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessLinkageSpecification(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessLinkageSpecHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	//
	// Part 04. Declarators.
	//

	void		ProcessInitDeclaratorsList(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessInitDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessUninitedDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessDirectDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessAbstractDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessDirectAbstractDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	void		ProcessPtrOperator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessCvQualifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessParameterDeclarationClause(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessParameterDeclarationHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessParameterDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessUninitedParameter(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessExceptionsSpecification(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessExceptionTypeIdsList(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	void		ProcessTypeId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessInitializer(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessInitializerClause(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessInitializersList(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	//
	// Part 05. Namespaces.
	//

	void		ProcessNamespaceDefinition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessNamespaceHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessNamespaceAliasDefinition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	//
	// Part 06. Enums.
	//

	void		ProcessEnumSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessEnumeratorHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessEnumeratorDefinition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	//
	// Part 07. Classes.
	//

	void		ProcessClassSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessClassHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessClassKey(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessMemberDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessAccessSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessSimpleMemberDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessMemberDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessConstantInitializer(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessBaseClassSpecifiersList(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessBaseClassSpecifier(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	//
	// Part 08. Templates.
	//

	void		ProcessGenericTemplateDeclaration(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessTemplateDeclarationHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessTemplateParamsListHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessTemplateTypeParameter(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessTemplateTemplateParameter(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessExplicitInstantiationHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessTemplateId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessTemplateArgument(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	//
	// Part 09. Functions.
	//

	void		ProcessFunctionDefinition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessFunctionHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessMemberInitializer(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessOperatorFunctionId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessOverloadableOperator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessConversionFunctionId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessConversionTypeId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessConversionDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	//
	// Part 10. Statements.
	//

	void		ProcessStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessCompoundStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessCompoundStatementHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessStatementsSeq(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessLabeledStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	void		ProcessExpressionStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessSelectionStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessIterationStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessForInitStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessCondition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessUninitedCondition(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessDoStatementBody(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	void		ProcessJumpStatement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessTryBlock(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessExceptionHandlersSeq(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessExceptionHandler(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessExceptionHandlerHeader(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	//
	//  Part 11. Expressions.
	//

	void		ProcessBinaryExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessMultiOpExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessAddMulExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessAssignmentExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessAssignmentOperator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	void		ProcessThrowExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessConstantExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessConditionalExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessPmExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessCastExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessUnaryExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessUnaryOperator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	void		ProcessNewExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessNewPlacement(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessNewTypeId(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessNewDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessDirectNewDeclarator(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessNewInitializer(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessDeleteExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	void		ProcessPostfixExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessPrimaryExpression(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessLiteral(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);
	void		ProcessStringsSeq(TCppSymbolStateItem &new_non_term, WORD irule, TGrammarRule &rule);

	//
	//  Environment methods.
	//

	bool						PrepareSimpleNameInfo(TCppSymbolStateItem &identifier_symbol, TCppNameInfo &dest_info_buffer);
								// When return value from this method is FALSE, this means that critical error has happened and
								// that it is already reported.

	bool						DoHandleStandAloneDeclTypeSpec(TCppSymbolStateItem &decl_type_spec_sym);

	TCppDefnBase			*DoHandleGenericDeclarator(TCppSymbolStateItem &declarator_sym, bool func_header_context);
	TCppDefnBase			*DoHandleTypedefDeclarator(TCppSymbolStateItem &declarator_sym);
	TCppDefnBase			*DoHandleDataFieldDeclarator(TCppSymbolStateItem &declarator_sym);
	TCppDefnBase			*DoHandleFunctionDeclarator(bool want_friend_proxy, TCppDefnBase *dest_defns_layer, TCppSymbolStateItem &declarator_sym, bool func_header_context);
	TCppDefnBase			*DoHandleTypelessFuncDecl(bool want_friend_proxy, TCppDefnBase *dest_defns_layer, TCppSymbolStateItem &declarator_sym, bool func_header_context);

	void						HandleSimpOrQualNameSymbols(TCppSymbolStateItem &new_non_term, TCppSymbolStateItem &first_rule_sym, int rule_len);
	void						HandleTypeSpecifierSymbols(TCppSymbolStateItem &new_non_term, WORD irule, int rule_len);
	void						HandlePtrOperatorDeclaratorSymbols(TCppSymbolStateItem &new_non_term, TCppSymbolStateItem &first_rule_sym);
	void						HandleFuncParamsDeclaratorSymbols(TCppSymbolStateItem &new_non_term, WORD irule, TCppSymbolStateItem *sym_params_decl_clause);
	void						HandleGenericTypeIdSymbols(TCppSymbolStateItem &new_non_term, TCppSymbolStateItem &first_rule_sym, int rule_len);
								// All methods above may report errors and/or meet the oom situation. In any case they will maintain
								// correct state of the declarator info or non terminal that are passed in as parameters.

	TCppExpressionBase		*ExprHandlePostfixUnaryOperation(TCppSymbolStateItem &first_rule_sym, TOperatorToken operation);
	TCppExpressionBase		*ExprHandleDotArrowOperation(TCppSymbolStateItem &first_rule_sym, TOperatorToken operation);
	TCppExpressionBase		*ExprHandleArrayIndexOperation(TCppSymbolStateItem &first_rule_sym, int rule_len);
	TCppExpressionBase		*ExprHandleFunctionCallOperation(TCppSymbolStateItem &first_rule_sym, int rule_len);
	TCppExpressionBase		*ExprHandleCtorCallOperation(TCppSymbolStateItem &first_rule_sym, int rule_len);
	TCppExpressionBase		*ExprHandleTypeCastOperation(TCppSymbolStateItem &first_rule_sym, TOperatorToken operation);
	TCppExpressionBase		*ExprHandleTypeIdOperation(TCppSymbolStateItem &first_rule_sym, WORD rule_app_id);
	TCppExpressionBase		*ExprHandlePrimaryThisOperand(TCppSymbolStateItem &this_symbol);
	TCppExpressionBase		*ExprHandlePrimaryExprIdOperand(TCppSymbolStateItem &expr_id_symbol);

	void						PromoteExistingDeclaration(TCppSymbolStateItem &new_non_term, TCppSymbolStateItem &sym);
	void						PromoteExistingDeclarator(TCppSymbolStateItem &new_non_term, TCppSymbolStateItem &sym);

	// ---------------------------------------------------------------------------------------------

	bool						CheckLookupSimpleIdentifierSymbol(TCppSymbolStateItem &symbol, TCppNameInfo &name_info, TCppItemType new_object_type,
																	TCppDefnBase **in_typedef_type_out_compatible_definition = NULL, TCppDefnBase *specific_defns_layer = NULL);
								//
								//	Passed symbol should be either a simple identifier that was just puicked up from the source code or an assembled
								//	declarator that contains unqualified identifier in it.
								//
								//	Method checks if it is ok to create new definition on the current layer or on passed specific layer (for enum members)
								//	with the passed name or not. This method supports exceptions for names of namespaces because they can be defined
								//	in several chunks and names of the types because they allow several identical definitions. When the passed name is
								//	occupied, method prepares an alt name and returns the unique id that was used in that name.
								//
								//	Return value is FALSE when critical error happens inside this method.
								//

	bool						CheckLookupElabTypeSpecName(TCppDefnBase *dest_defns_layer, TCppElabType elab_type, TCppNameInfo &name_info,
																	TCppItemType dest_item_type, TCppDataTypeBase *&compatible_elaborated_type_object);

	bool						CheckLookupElabTypeDefnName(TCppElabType elab_type, TCppSymbolStateItem *name_symbol, TCppItemType dest_item_type,
																	TCppDefnBase *&compatible_elaborated_forward_declaration);
								//
								//	This method checks contents of only one layer to verify if is it or to create elaborated type object with the passed
								//	name or not. Note that caller is expected to plan creation of the full definition, not the forward declaration.
								//
								//	In case of success this method returns TRUE plus either an already existing forward declaration or a fully prepared
								//	name and corresponding preferred id that can be used for creating the new object. In case of the critical error this
								//	method returns FALSE.
								//

	bool						CheckLookupDataFieldObject(TCppDefnBase *dest_defns_layer, TCppStorageClass new_data_field_strgc, TCppNameInfo &name_info,
															TCppDataFieldDefn *&compatible_data_field);

	bool						CheckLookupFucntionObject(bool want_friend_proxy, TCppDefnBase *dest_defns_layer, TCppNameInfo &name_info, TCppDeclaratorModifier *unapplied_func_modifier,
															TCppCodeEntryPointDefn *&compatible_function_or_func_template, bool func_header_ctx);

	bool						CheckLookupExistingDefinition(TSymbolStateItem *err_ctx_sym, TCppElabType elab_type, TCppNameInfo &name_info, WORD dest_symbol_app_id,
															TCppDefnBase *&compatible_object, TCppDefnsBucket **compatible_defns_bucket = NULL);
								//
								//	This method expects that its name info param contains a prepared name info that is either an identifier or a template id.
								//
								//									   Dummy	Struct	OtherData	Function
								//										 Ptr		 Type	    Type		 Type	    Namesp	    Template
								//	----------------------------------------------------------------------------------------------------
								//		  SimpleOrQualifiedId				-		   +			-		   -			 +		   (instantiate)
								//		  NamedTypeSpecifier				-		   +			+		   +			 -		   (instantiate)
								//		  UsingDirective					+		   -			-		   -			 +			  -
								//		  PtrOperator						+		   +			-		   -			 -		   (instantiate)
								//		  NamespaceAlias					+		   -			-		   -			 +			  -
								//		  ClassHeader	(template spec)		+		   -			-		   -			 -			  +
								//		  BaseClassSpecifier				+		   +			-		   -			 -		   (instantiate)
								//		  TemplateTemplateParam			+		   -			-		   -			 -			  +
								//		  PostfixExpression					-		   +			+		   -			 -		   (instantiate)
								//
								//	When this function is called in the template context and the passed name does not correspond to anything,
								//	then an unresolved name object is returned.
								//

	bool						CheckUpdateParameterName(TSymbolStateItem *err_ctx_sym, TCppNameInfo &param_name_info);

	bool						CheckPrepareTypeSpecifier(TCppSymbolStateItem &decl_type_spec_sym);
								//
								//	This method ignores the decl type spec prefix, function attrs, storage class and cv spec fields in the passed
								//	decl type specifier. It is the responsibility of the caller to verifiy if their presence/absense is appropriate in
								//	the current context or not.
								//
								//	This method checks the base type, in particular:
								//
								//		 --	Resolves the built in type if the built in type struct is not empty.
								//		 --	Resolves the named type specifier. Un case of resolution failure, the unresolved data type
								//			object is returned.
								//		 --	Resolves the elaborated type spec. In case, if resolution is not successful and existing qual path
								//			does not allow creating the forward declaration, it creates unresolved data type with elaborated
								//			unresolved name info inside.
								//		 --	If the base type is missing, this method issues an error and sets the base type to void.
								//
								//	The return value shows if critical error has happened or not.
								//

	TCppDataTypeBase		*CheckPrepareDataTypeFromTypeSpecifier(TCppSymbolStateItem &type_spec_sym);
								//
								//	This method calls the CheckPrepareTypeSpecifier() first. In the case of success, it checks the base type again.
								//	If the base type is a function type, it generates an indirection layer. If the cv spec field in the type spec is not
								//	empty, method adds an appropriate layer of indirection.
								//

	void						ApplyCvPtrRefModifier(TSymbolStateItem *err_ctx_sym, TCppDeclaratorInfo &decl_info, TCppCvPtrRefSpec &cvpr_info);
	void						ApplyMultiDimArrayModifier(TSymbolStateItem *err_ctx_sym, TCppDeclaratorInfo &decl_info, TCppExpressionBase *dims_list);
	void						ApplySingleDimArrayModifier(TSymbolStateItem *err_ctx_sym, TCppDeclaratorInfo &decl_info, TCppExpressionBase *dim_expr);
	void						ApplyFuncParamsModifier(TSymbolStateItem *err_ctx_sym, TCppDeclaratorInfo &decl_info, TList *params_list = NULL, TCppSymbolStateItem *cv_qual_sym = NULL, TList *exceptions_list = NULL);

	inline TCppAccessLevel		GetCurrentAccessLevel() const { return((m_defns_layer_symbol != NULL && m_defns_layer_symbol->m_meaning == csm_class_header) ? m_defns_layer_symbol->m_struct_info.m_curr_accs_level : accs_public); }

	inline bool				IsTemplateContext() const { return(m_curr_template_header != NULL); }
	inline bool				IsTemplateHeaderContext() const { return(m_curr_template_header != NULL && m_curr_template_header->m_template_info.m_header_layer_owner == m_defns_layer_owner); }
	inline bool				IsTemplateBodyContext() const { return(m_curr_template_header != NULL && m_curr_template_header->m_template_info.m_header_layer_owner != m_defns_layer_owner); }

	inline bool				IsParamsListContext() const { return(m_curr_params_list != NULL); }
	inline bool				IsTemplateParamsListContext() const { return(m_curr_params_list != NULL && m_curr_params_list->m_meaning == csm_template_params_list); }

	TCppStructTypeDefn		*FindOrCreateTemplateInstantiation(TSymbolStateItem *err_ctx_sym, TCppClassTemplateDefn *class_template, TList *&instantiation_arguments);
								// In case of success this method returns cached or just created template instantiation.

	TCppLabelDefn			*FindOrCreateNamedLabel(TCppNameInfo &name_info, bool goto_stmt_context);
								// This is special type of the name search. Experiments with MS compiler show that GOTO statements
								// can jump to any label in the function. Goto can enter blocks, etc. Do the same thing.

	void						CheckReportCaseLabelProblems(TSymbolStateItem *err_ctx_sym, TCppCaseLabelDefn *case_label);
								// This function issues syntax errors when passed label is not unique in the current context or when the passed
								// case label stays outside of the switch statement. This function is not modifying the passed label or any other
								// data structure.

	void						GetLabelContainersFromSymbol(TCppSymbolStateItem *sym, TCppItemBase *&pri_container, TCppItemBase *&sec_container);
	void						CheckReportCaseLabelDuplication(TSymbolStateItem *err_ctx_sym, TCppItemBase *container, TCppCaseLabelDefn *case_label);

	// ----------------------------------------------------------------------------------------------

	TCppUnresolvedNameInfo	*CreateUnresNameInfo(TCppNameInfo &info);
	TCppDefnBase			*CreateUnresolvedObject(TCppNameInfo &info, TCppDefnProblem unintegrated_problem, TCppItemType item_type);

	TCppStructTypeDefn		*FindOrCreateStructTypeObject(TCppElabType elab_type, TCppSymbolStateItem *sym_class_name, TCppSymbolStateItem *sym_base_classes = NULL);
	TCppStructTypeDefn		*FindOrCreateClassTemplateObject(TCppElabType elab_type, TCppSymbolStateItem *sym_class_name, TCppSymbolStateItem *sym_base_classes = NULL);
	TCppStructTypeDefn		*FindOrCreateClassTemplateSpecObject(TCppElabType elab_type, TCppSymbolStateItem *sym_class_name, TCppSymbolStateItem *sym_base_classes = NULL);
								// All three methods above expect to receive correct elaboration type and non NULL prepared class name
								// symbol with an identifier or with a template id.

	TCppDataTypeBase		*FindOrCreateElaboratedTypeObject(TSymbolStateItem *err_ctx_sym, TCppElabType elab_type, TCppNameInfo &name_info, bool stand_alone_dts_context, bool friend_flag);
								//
								//  This method is called when the elaborated type specifier is processed. It serves two significantly different types of calls:
								//
								//	Case 1:	The forward declaration appears in the type specifier or it is used with the friend keyword. This means
								//			that caller plans to use the returned object either as a data type or for creating a friend stub around it.
								//			In this situation the qualified name can point to any layer of definitions and full search for existing
								//			definitions should be carried out.
								//
								//	Case 2:	Forward declaration appears as the forward declaration itself, meaning that source code wants to pre
								//			declare the object or confirm that the desired object is already existing. In this situation a much more
								//			limited search should be carried out plus only one layer should be checked and forward declaration
								//			should be created only if it is allowed to create forward declarations there.
								//

	TCppDefnBase			*FindUnambigNonOverloadableDefnByName(const wchar_t **names_seq, TCppDefnProblem &discovered_problem);
								//
								//	This method searches only for non overloadable objects. This means that it cannot find templates or anyting
								//	that is located inside templates. Note that since the required elaboration type is not in the list of parameters,
								//	the returned object may have any elaboration. The search starts from the global namespace and goes down
								//	into the tree of defns. Ambiguity may take place only between objects with different type of elaboration.
								//

	TCppDataFieldDefn			*CreateDataParamObject(TCppSymbolStateItem &type_spec_sym, bool declarator_present);
	TCppDataFieldDefn			*CreateDataFieldObject(TCppDefnBase *dest_defns_layer, TCppNameInfo &name_info, TCppStorageClass strg_class, TCppDataTypeBase *data_type);

	TCppCodeEntryPointDefn	*CreateFunctionPrototype(TCppDefnBase *dest_defns_layer, TCppFunctionAttr func_attrs, TCppNameInfo &name_info, TCppDataTypeBase *ret_value_type, TCppDeclaratorModifier *unapplied_func_modifier);
	TCppBlockDefn			*CreateAndPrepareFunctionBody(TCppCodeEntryPointDefn *func_object);

	TCppDataTypeBase		*CreateAndPublishElabFwdDecl(TCppDefnBase *fwd_decl_layer, TCppElabType elab_type, TCppNameInfo &name_info, TCppItemType item_type);
	TCppFriendSpec			*CreateAndPublishFriendRecord(TSymbolStateItem *err_ctx_sym, TCppDefnBase *friendly_object);

	short					ComputeBitWidthExpression(TCppSymbolStateItem &width_expr_sym, bool bit_field_width_expr);
	bool						CreatePossibleExceptiosList(TCppSymbolStateItem &new_non_term, TCppDataTypeBase *exception_type);

	TCppFunctionAttr			CheckFunctionDeclTypeSpec(TCppDefnBase *dest_defns_layer, TCppSymbolStateItem &declarator_sym, bool func_header_context);
	bool						CheckExistingFunctionProps(TCppDefnBase *dest_defns_layer, TCppFunctionAttr func_attrs, TCppNameInfo &name_info, TCppDeclaratorModifier *unapplied_func_modifier, TCppCodeEntryPointDefn *existing_func_object, bool func_header_context);
	void						CheckCollectOverrideProtos(TCppCodeEntryPointDefn *func_object, TCppDefnBase *defns_container, bool dest_layer);

	void						AssembleTypelessDeclarator(TCppSymbolStateItem &declarator_sym, TCppDeclaratorModifier *&first_mdfs_layer);

	TCppDefnBase			*AssembleDeclaratorType(TSymbolStateItem *dts_ctx_sym, bool typedef_mode, TCppCvPtrRefSpec decl_cv_spec, TCppDefnBase *decl_base_type, TCppSymbolStateItem &declarator_sym, TCppDeclaratorModifier **unapplied_func_modifier = NULL);
									//
									//  In the declarator parameter
									//  only the modifiers if any are important. Other fields of the declarator like the name
									//  are not checked.
									//

	TCppDataTypeBase		*CreateCvPtrRefModification(TSymbolStateItem *err_ctx_sym, TCppDefnBase *base_type, TCppDeclaratorModifier *cvprs, int num_cvprs, bool publish_modifier, bool named_base_type);
	TCppDataTypeBase		*CreateArrayModification(TSymbolStateItem *err_ctx_sym, TCppDataTypeBase *base_type, TCppDeclaratorModifier *layers, int num_layers, bool publish_modifier, bool named_base_type);
	TCppFunctionTypeDefn		*CreateFunctionModification(TSymbolStateItem *err_ctx_sym, TCppDataTypeBase *base_type, TCppDeclaratorModifier *mdf_data, bool publish_modifier, bool named_base_type);

	TCppBitFieldTypeDefn		*CreateBitFieldType(TSymbolStateItem *err_ctx_sym, TCppDataTypeBase *base_type, TCppSymbolStateItem &bit_size_expr_sym);
	TCppDataTypeBase		*CreateDereferencedType(TSymbolStateItem *err_ctx_sym, TCppCvPtrRefTypeDefn *source_type, int new_num_cvprs);

	// ----------------------------------------------------------------------------------------------

	void						PublishOnTheCurrentLayer(TCppItemBase *item);
	void						PublishOnTheSpecificLayer(TCppDefnBase *defns_layer, TCppItemBase *item);
								// These methods process only the named objects.

	bool						PublishBlockDeclaration(TCppDefnBase *defn_object_to_publish, bool want_full_publishing_procedure);
								// This function should be used for publishing only named objects, in other words only those objects
								// that take their name directly from the source code that is being processed.

	bool						PublishParameterObject(TCppItemBase *param_or_arg_to_publish);
								// This method publishes function parameters, template header parameters and template instantiation
								// arguments in the params list of the stack symbol that owns the current params layer. If the param
								// object is passed, method expects that the name of the param is already verified.

	void						PublishDerivedDefinition(TCppDefnBase *base_type_owner, TCppDefnBase *defn_item);
								// This is special method for publishing nameless definitions. After return from this method
								// the passed object is fully registered with the database and it is owned by the database.

	// ----------------------------------------------------------------------------------------------

	void						EnterDefinitionsLayer(TCppDefnBase *layer_owner, TCppSymbolStateItem &layer_symbol);
	void						ExitCurrentDefnsLayer(TCppSymbolStateItem *curr_layer_symbol);

	int						AllocateDeclaratorModifierLayer(TSymbolStateItem *err_ctx_sym, TCppDeclaratorInfo &decl_info);
	inline void				RecycleDeclaratorModifiersArray(TCppDeclaratorModifiersArray *info) { m_dcltr_mdfs_look_aside.RecycleItem(info); }

	TList						*AllocateGeneralPurposeList();
	inline void				RecycleGeneralPurposeList(TList *info) { info->DeleteAll(); m_gen_purp_lists_look_aside.RecycleItem(info); }

	TCppParserCtvRefEventInfo		*AllocateUnsubmittedCtvEventInfo();
	inline void					RecycleUnsubmittedCtvEventInfo(TCppParserCtvRefEventInfo *info) { info->m_owner_parser = NULL; m_unsubm_ctv_infos_look_aside.RecycleItem(info); }

	wchar_t					*CacheLiteralString(TStrPtrInfo &str_ptr_info);
	TCppBuiltInTypeDefn		*GetBuiltInTypeDefn(TCppBuiltInTypeIdent type_id);
	TCppItemBase			*AllocateNewCppItem(TCppItemType defn_type, ID predef_item_id = 0);
								// Methods of the group either return a valid non NULL pointer or return NULL and raise a critical error.

protected:

	TCppDatabase						&m_cpp_database;
											// This is database of the language level objects of the current translation unit.

	TCppObjectsFactory					&m_objects_factory;
											// Objects factory allows application layer to create objects of its own derived classes.

	TCppBuiltInTypeDefn					*m_built_in_defns[bltin_ids_max_plus1-bltin_ids_base];
											// This array is not owning its items. All these items belong to the global namespace.

	TCppDefnBase						*m_size_t_defn;
	TCppDefnBase						*m_type_info_defn;
											// Both pointers are not owned. These are cashed pointers to the user defined objects.
											// These objects can be either defns or unresolved objects if the appropriate user defined
											// objects are missing or conflicting.
protected:

	ID									m_parent_syntax_error;				// In the normal situation the value of this field is zero. This field should be set
																			// before emitting a dependent syntax error and it should be immediately reset
																			// after that.
	TList									m_unplaced_labels;					// This list contains labels that are already used in the GOTO statements but
																			// they are not yet defined in the code of the current function.
	TList									m_undefined_class_members;

	TCppSymbolStateItem					*m_curr_decl_type_spec;
	TCppSymbolStateItem					*m_curr_enum_header;

	TCppSymbolStateItem					*m_curr_template_header;
	TCppSymbolStateItem					*m_curr_params_list;

	TCppDefnBase						*m_defns_layer_owner;				// The value of this field is never NULL.
	TCppSymbolStateItem					*m_defns_layer_symbol;				// Value of this field is NULL when the current layer is the global namespace.

	TCppSymbolStateItem					*m_curr_linkage_spec;

private:

	typedef  TLookAsideTable<TCppDeclaratorModifiersArray, 64>  TDeclaratorModifiersLookAsideTable;
	typedef  TLookAsideTable<TList, 96>  TGeneralPurposeListsLookAsideTable;
	typedef  TLookAsideTable<TCppParserCtvRefEventInfo, 48> TCppParserCtvEventInfosLookAsideTable;

	TDeclaratorModifiersLookAsideTable			m_dcltr_mdfs_look_aside;
	TGeneralPurposeListsLookAsideTable			m_gen_purp_lists_look_aside;
	TCppParserCtvEventInfosLookAsideTable		m_unsubm_ctv_infos_look_aside;

	//
	// Note:	Symbol state objects (TCppSymbolStateItem instances) are stored in the look aside list
	//		of the base class, TGrammarBasedParser.
	//

	friend struct TCppNameInfo;
	friend struct TCppParserCtvRefEventInfo;
};

#endif	// TextScan_CppFrontEnd_H


