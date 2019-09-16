//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   In memory storage for C/C++ objects.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/FormatNumber.H"
#include  "TextScan/CppDatabaseXml.H"

static TXmlCnrWritingProps xpr_CppDbCnrProps =
{
	FALSE,				// Use short tag close form.
	FALSE, FALSE,			// Tag header attrs are all on one line.
	TRUE, TRUE,			// Start every field tag on a new line.
	TRUE, TRUE,			// Start every container on a new line.
	FALSE,				// No crlf before the tag hdr close.
	TRUE,				// Add CrLf before the container close.
	FALSE,				// No crlf after tag attributes.
	TRUE,				// Add CrLf after field tags.
	TRUE,				// Add CrLf after subcontainers.
};

extern TXmlDictRecord xtags_CppDatabaseItems[];

static TXmlDictRecord xtags_CppItemCore[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"ItemId"),					cxlt_item_core_id,						},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ParentId"),					cxlt_item_core_parent_id,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"Type"),						cxlt_item_core_item_type,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumRelatedEvents"),			cxlt_item_core_num_pars_evts,			},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppDefinition[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"DefnProblem"),				cxlt_defn_info_problem,					},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"Name"),						cxlt_defn_info_name,					},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"KeyId"),						cxlt_defn_info_key_id,					},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ElabType"),					cxlt_defn_info_elab_type,				},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppExpression[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"Operation"),					cxlt_expr_info_operation,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"LValue"),					cxlt_expr_info_lvalue_expr,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"ConstExpr"),					cxlt_expr_info_const_expr,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ResultTypeId"),				cxlt_expr_info_result_type_id,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ResultTypeName"),			cxlt_expr_info_result_type_name,			},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppCvPtrRefSpec[] =
{
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"Const"),						cxlt_cvpr_spec_const,					},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"Volatile"),					cxlt_cvpr_spec_volatile,					},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"Pointer"),					cxlt_cvpr_spec_pointer,					},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"Reference"),					cxlt_cvpr_spec_reference,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"StructId"),					cxlt_cvpr_spec_struct_id,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"StructName"),				cxlt_cvpr_spec_struct_name,				},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppCvPtrRefSpecsArray[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"LayerInfo"),					cxlt_cvpr_specs_array_element,			xtef_none,	xtags_CppCvPtrRefSpec },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppArrayDimensionSpec[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumElements"),				cxlt_array_dim_num_elements,			},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"ConstExpr"),					cxlt_array_dim_num_elems_expr_const,	},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ExprRef"),					cxlt_array_dim_num_elems_expr_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppArrayDimensionSpecsArray[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"DimensionInfo"),				cxlt_array_dims_array_element,			xtef_none,	xtags_CppArrayDimensionSpec },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppNamespaceStatistics[] =
{
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"EnumTypes"),				cxlt_namespace_stts_enum_types,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"StructTypes"),				cxlt_namespace_stts_struct_types,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"UnionTypes"),				cxlt_namespace_stts_union_types,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ClassTypes"),				cxlt_namespace_stts_class_types,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"StructuredTypes"),			cxlt_namespace_stts_structured_types,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"DataFields"),					cxlt_namespace_stts_data_fields,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"CodeEntryPoints"),			cxlt_namespace_stts_code_entry_points,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OtherObjects"),				cxlt_namespace_stts_other_objects,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"TotalObjects"),				cxlt_namespace_stts_total_objects,		},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppStructTypeStatistics[] =
{
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"InstanceDataFields"),			cxlt_struct_type_stts_inst_data_fields,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"StaticDataFields"),				cxlt_struct_type_stts_static_data_fields,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"CodeEntryPoints"),			cxlt_struct_type_stts_code_entry_points,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OtherMemberObjects"),		cxlt_struct_type_stts_other_objects,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"TotalMemberObjects"),			cxlt_struct_type_stts_total_objects,		},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFunctionTypeParam[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"Ref"),						cxlt_func_param_ref,					},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_string,		DefTag(L"Name"),						cxlt_func_param_name,					},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"TypeRef"),					cxlt_func_param_type_ref,				},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_string,		DefTag(L"TypeName"),					cxlt_func_param_type_name,			},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFunctionTypeParamsArray[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"ParamInfo"),					cxlt_func_params_array_element,		xtef_none,	xtags_CppFunctionTypeParam },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFunctionTypeThrow[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"Ref"),						cxlt_func_throws_ref,					},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"TypeRef"),					cxlt_func_throws_type_ref,				},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_string,		DefTag(L"TypeName"),					cxlt_func_throws_type_name,			},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFunctionTypeThrowsArray[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"ThrowInfo"),					cxlt_func_throws_array_element,			xtef_none,	xtags_CppFunctionTypeThrow },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFunctionTypeProps[] =
{
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"SpecificParsSess"),			cxlt_func_type_props_visib_pars_sess,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"LinkageSpecRef"),				cxlt_func_type_props_linkage_spec_ref,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"LinkageSpecName"),			cxlt_func_type_props_linkage_spec_name,	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"AccessLevel"),				cxlt_func_type_props_access_level,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"FuncAttrs"),					cxlt_func_type_props_func_attrs,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"FuncAttrsText"),				cxlt_func_type_props_func_attrs_as_text,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"SpecType"),					cxlt_func_type_props_spec_type,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"OperatorType"),				cxlt_func_type_props_operator_type,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ReturnValueTypeRef"),			cxlt_func_type_props_ret_val_type_ref,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ReturnValueTypeName"),		cxlt_func_type_props_ret_val_type_name,	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumParams"),				cxlt_func_type_props_num_params,		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Params"),					cxlt_func_type_props_params,			xtef_none,	xtags_CppFunctionTypeParamsArray },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumThrows"),				cxlt_func_type_props_num_throws,		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Throws"),					cxlt_func_type_props_throws,			xtef_none,	xtags_CppFunctionTypeThrowsArray },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppTemplateParamInfo[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"Ref"),						cxlt_tplt_param_info_ref,				},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_string,		DefTag(L"Name"),						cxlt_tplt_param_info_name,				},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_string,		DefTag(L"ParamType"),				cxlt_tplt_param_info_type,				},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_number,	DefTag(L"DataTypeRef"),				cxlt_tplt_param_info_data_type_ref,		},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_string,		DefTag(L"DataTypeName"),				cxlt_tplt_param_info_data_type_name,	},
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumParams"),				cxlt_tplt_param_info_tplt_num_params,	},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppTemplateParamInfosArray[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"TemplateParamInfo"),			cxlt_tplt_param_infos_array_element,		xtef_none,	xtags_CppTemplateParamInfo },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppCondParsParam[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_eof }
};

//
//	------------------------ Definitions-1 ---------------------------
//

static TXmlDictRecord xtags_CppBuiltInTypeDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"RealType"),					cxlt_built_in_type_real_type,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"Unsigned"),					cxlt_built_in_type_unsigned,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ByteLength"),				cxlt_built_in_type_byte_len,				},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppBitFieldTypeDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BaseTypeId"),				cxlt_bit_field_type_base_type_id,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"BaseTypeName"),				cxlt_bit_field_type_base_type_name,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"ConstBitSizeExpr"),			cxlt_bit_field_type_const_expr,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BitSize"),					cxlt_bit_field_type_bit_size,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"BitSizeExprRef"),				cxlt_bit_field_type_expr_ref,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppCvPtrRefTypeDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BaseTypeId"),				cxlt_cvpr_type_base_type_id,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"BaseTypeName"),				cxlt_cvpr_type_base_type_name,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumLayers"),				cxlt_cvpr_type_num_layers,				},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Layers"),					cxlt_cvpr_type_layers,					xtef_none,	xtags_CppCvPtrRefSpecsArray },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppArrayTypeDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BaseTypeId"),				cxlt_array_type_base_type_id,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"BaseTypeName"),				cxlt_array_type_base_type_name,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumDimensions"),			cxlt_array_type_num_dimensions,		},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Dimensions"),				cxlt_array_type_dimensions,				xtef_none,	xtags_CppArrayDimensionSpecsArray },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppDataTypeAliasDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BaseTypeId"),				cxlt_data_type_alias_base_type_id,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"BaseTypeName"),				cxlt_data_type_alias_base_type_name,	},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppEnumTypeDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"ForwardDeclaration"),			cxlt_enum_type_forward_decl,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"BaseTypeRef"),				cxlt_enum_type_base_type_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"BaseTypeName"),				cxlt_enum_type_base_type_name,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumEnumMembers"),			cxlt_enum_type_num_members,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppEnumMemberDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"ExplicitValue"),				cxlt_enum_member_explicit_value,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"ConstValueExpr"),				cxlt_enum_member_const_value,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"Value"),						cxlt_enum_member_value,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ValueSubtype"),				cxlt_enum_member_value_subt,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ValueExprRef"),				cxlt_enum_member_value_expr_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppStructTypeDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"ForwardDeclaration"),			cxlt_struct_type_forward_decl,			},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"MembersInfo"),				cxlt_struct_type_member_statistics,		xtef_none,	xtags_CppStructTypeStatistics },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppBaseClassSpec[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"Virtual"),					cxlt_base_class_spec_virtual,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"AccessLevel"),				cxlt_base_class_spec_access_level,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BaseClassRef"),				cxlt_base_class_spec_base_class_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"BaseClassName"),			cxlt_base_class_spec_base_class_name,	},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFieldAlignSpec[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"ConstAlignValueExpr"),			cxlt_field_align_spec_const_expr,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"AlignValue"),					cxlt_field_align_spec_align_value,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"AlignValueExprRef"),			cxlt_field_align_spec_expr_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFriendSpec[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FriendSpecRef"),				cxlt_friend_spec_friend_ref,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"FriendSpecName"),			cxlt_friend_spec_friend_name,			},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppDataFieldDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"SpecificParsSess"),			cxlt_data_field_visib_pars_sess,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"LinkageSpecRef"),				cxlt_data_field_linkage_spec_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"LinkageSpecName"),			cxlt_data_field_linkage_spec_name,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"AccessLevel"),				cxlt_data_field_access_level,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"StorageClass"),				cxlt_data_field_storage_class,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ParamIndex"),				cxlt_data_field_param_index,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"IsMutable"),					cxlt_data_field_mutable,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"DataTypeRef"),				cxlt_data_field_data_type_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"DataTypeName"),				cxlt_data_field_data_type_name,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"InitializerRef"),				cxlt_data_field_init_ref,					},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppDataFieldInitializer[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"InitializerType"),				cxlt_data_field_initializer_init_type,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

//
//	------------------------ Definitions-2 ---------------------------
//

static TXmlDictRecord xtags_CppFunctionTypeDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"FunctionTypeProps"),			cxlt_function_type_func_props,			xtef_none,	xtags_CppFunctionTypeProps },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFunctionTypeAliasDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BaseFunctionTypeRef"),		cxlt_function_type_alias_base_type_ref,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"BaseFunctionTypeName"),		cxlt_function_type_alias_base_type_name,	},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppCodeEntryPointDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"FunctionTypeProps"),			cxlt_code_entry_point_func_props,		xtef_none,	xtags_CppFunctionTypeProps },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FunctionBodyRef"),			cxlt_code_entry_point_func_body_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppPossibleExceptionSpec[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ExceptionTypeRef"),			cxlt_exception_spec_data_type_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ExceptionTypeName"),			cxlt_exception_spec_data_type_name,		},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppNamespaceDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"InnerObjectsInfo"),			cxlt_namespace_elements_statistics,		xtef_none,	xtags_CppNamespaceStatistics },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppGlobalNamespaceDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"InnerObjectsInfo"),			cxlt_global_namespace_elements_stts,		xtef_none,	xtags_CppNamespaceStatistics },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppNamespaceAliasDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BaseNamespaceRef"),			cxlt_namespace_alias_namespace_ref,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"BaseNamespaceName"),		cxlt_namespace_alias_namespace_name,	},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppUsingDeclarationDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"DefnToUseRef"),				cxlt_using_declaration_defn_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"DefnToUseName"),			cxlt_using_declaration_defn_name,		},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppUsingDirectiveDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NamespaceToUseRef"),		cxlt_using_directive_namespace_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"NamespaceToUseName"),		cxlt_using_directive_namespace_name,	},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppAsmInsertDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"StringLength"),				cxlt_asm_insert_length,					},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"StringValue"),				cxlt_asm_insert_value,					},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppLinkageSpec[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"SpecName"),					cxlt_linkage_spec_name,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"SpecKeyId"),					cxlt_linkage_spec_key_id,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"SpecCntUse"),				cxlt_linkage_spec_cnt_use,				},
	{ xdrt_eof }
};

//
//	------------------------ Templates ---------------------------
//

static TXmlDictRecord xtags_CppClassTemplateDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"ForwardDeclaration"),			cxlt_class_template_forward_decl,		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"MembersInfo"),				cxlt_class_template_member_stts,		xtef_none,	xtags_CppStructTypeStatistics },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumTemplateParams"),		cxlt_class_template_num_params,		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"TemplateParams"),			cxlt_class_template_params,				xtef_none,	xtags_CppTemplateParamInfosArray },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppClassTemplateSpecialization[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BaseTemplateRef"),			cxlt_class_template_spec_template_ref,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"BaseTemplateName"),			cxlt_class_template_spec_template_name,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"ForwardDeclaration"),			cxlt_class_template_spec_forward_decl,	},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"MembersInfo"),				cxlt_class_template_spec_member_stts,	xtef_none,	xtags_CppStructTypeStatistics },
	{ xdrt_field,	xrdm_opt_mi,	  xtbt_number,	DefTag(L"SpecParamRef"),				cxlt_class_template_spec_param_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppClassTemplateInstantiation[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"OriginalTemplateRef"),			cxlt_class_template_inst_template_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"OriginalTemplateName"),		cxlt_class_template_inst_template_name,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"ForwardDeclaration"),			cxlt_class_template_inst_forward_decl,	},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"MembersInfo"),				cxlt_class_template_inst_member_stts,	xtef_none,	xtags_CppStructTypeStatistics },
	{ xdrt_field,	xrdm_opt_mi,	  xtbt_number,	DefTag(L"InstArgumentRef"),			cxlt_class_template_inst_argument_ref,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFunctionTemplateDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumTemplateParams"),		cxlt_func_template_num_params,			},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"TemplateParams"),			cxlt_func_template_params,				xtef_none,	xtags_CppTemplateParamInfosArray },
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"FunctionTypeProps"),			cxlt_func_template_func_props,			xtef_none,	xtags_CppFunctionTypeProps },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFunctionTemplateSpecialization[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BaseTemplateRef"),			cxlt_func_template_spec_template_ref,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"BaseTemplateName"),			cxlt_func_template_spec_template_name,	},
	{ xdrt_field,	xrdm_opt_mi,	  xtbt_number,	DefTag(L"SpecParamRef"),				cxlt_func_template_spec_param_ref,		},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"FunctionTypeProps"),			cxlt_func_template_spec_func_props,		xtef_none,	xtags_CppFunctionTypeProps },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFunctionTemplateInstantiation[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"OriginalTemplateRef"),			cxlt_func_template_inst_template_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"OriginalTemplateName"),		cxlt_func_template_inst_template_name,	},
	{ xdrt_field,	xrdm_opt_mi,	  xtbt_number,	DefTag(L"InstArgumentRef"),			cxlt_func_template_inst_argument_ref,	},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"FunctionTypeProps"),			cxlt_func_template_inst_func_props,		xtef_none,	xtags_CppFunctionTypeProps },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppTemplateTypeParamDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParamIndex"),				cxlt_type_param_index,					},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"DefaultValueId"),				cxlt_type_param_default_value_id,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"DefaultValueName"),			cxlt_type_param_default_value_name,		},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppTemplateTemplateParamDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParamIndex"),				cxlt_template_param_index,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumTemplateParams"),		cxlt_template_param_num_params,		},
	{ xdrt_field,	xrdm_opt_mi,	  xtbt_number,	DefTag(L"TemplateParamRef"),			cxlt_template_param_param_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"DefaultValueId"),				cxlt_template_param_default_value_id,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"DefaultValueName"),			cxlt_template_param_default_value_name,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppTemplateTypeArgument[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"TypeId"),					cxlt_type_argument_type_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"TypeName"),					cxlt_type_argument_type_name,			},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppTemplateTemplateArgument[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"ArgumentType"),				cxlt_template_argument_type,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"TemplateId"),				cxlt_template_argument_template_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"TemplateName"),				cxlt_template_argument_template_name,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"TemplateParamRef"),			cxlt_template_argument_param_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"TemplateParamName"),		cxlt_template_argument_param_name,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"TemplateParamIndex"),		cxlt_template_argument_param_index,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"SearchResBucketName"),		cxlt_template_argument_sres_name,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"SearchResBucketKeyId"),		cxlt_template_argument_sres_key_id,		},
	{ xdrt_eof }
};

//
//	------------------------ Unresolved objects ---------------------------
//

static TXmlDictRecord xtags_CppUnresolvedNameInfo[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"NodeType"),					cxlt_unresolved_name_node_type,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"OverloadedOperator"),			cxlt_unresolved_name_oper_token,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OvldConvDestTypeId"),		cxlt_unresolved_name_dest_type_id,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"OvldConvDestTypeName"),		cxlt_unresolved_name_dest_type_name,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumTemplateArguments"),		cxlt_unresolved_name_num_templ_args,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppUnresolvedDataType[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumUnresolvedNames"),		cxlt_unres_data_type_num_names,		},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppUnresolvedStructType[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumUnresolvedNames"),		cxlt_unres_struct_type_num_names,		},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppUnresolvedDataField[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumUnresolvedNames"),		cxlt_unres_data_field_num_names,		},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppUnresolvedCodeEntryPoint[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumUnresolvedNames"),		cxlt_unres_code_entry_point_num_names,	},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppUnresolvedClassTemplate[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumUnresolvedNames"),		cxlt_unres_class_template_num_names,	},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppUnresolvedFunctionTemplate[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumUnresolvedNames"),		cxlt_unres_func_template_num_names,	},
	{ xdrt_eof }
};

//
//	------------------------ Statements ---------------------------
//

static TXmlDictRecord xtags_CppEmptyStatement[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppIfStatement[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ConditionStmtId"),			cxlt_if_stmt_cond_stmt_ref,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"CondStmtTypeName"),			cxlt_if_stmt_cond_stmt_type,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ThenStatementId"),			cxlt_if_stmt_then_stmt_ref,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ThenStmtTypeName"),			cxlt_if_stmt_then_stmt_type,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ElseStatementId"),			cxlt_if_stmt_else_stmt_ref,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ElseStmtTypeName"),			cxlt_if_stmt_else_stmt_type,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppSwitchStatement[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ConditionStmtId"),			cxlt_switch_stmt_cond_stmt_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"CondStmtTypeName"),			cxlt_switch_stmt_cond_stmt_type,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"SwitchBodyStmtId"),			cxlt_switch_stmt_body_stmt_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"SwitchBodyStmtTypeName"),	cxlt_switch_stmt_body_stmt_type,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumCaseLabels"),				cxlt_switch_stmt_num_case_labs,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppForStatement[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"InitStmtId"),					cxlt_for_stmt_init_stmt_ref,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"InitStmtTypeName"),			cxlt_for_stmt_init_stmt_type,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"CheckStmtId"),				cxlt_for_stmt_check_stmt_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"CheckStmtTypeName"),		cxlt_for_stmt_check_stmt_type,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ReinitExprId"),				cxlt_for_stmt_reinit_expr_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ReinitExprTypeName"),			cxlt_for_stmt_reinit_expr_type,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BodyStmtId"),				cxlt_for_stmt_body_stmt_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"BodyStmtTypeName"),			cxlt_for_stmt_body_stmt_type,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppWhileStatement[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ConditionStmtId"),			cxlt_while_stmt_cond_stmt_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"CondStmtTypeName"),			cxlt_while_stmt_cond_stmt_type,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BodyStmtId"),				cxlt_while_stmt_body_stmt_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"BodyStmtTypeName"),			cxlt_while_stmt_body_stmt_type,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppDoWhileStatement[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BodyStmtId"),				cxlt_do_while_stmt_body_stmt_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"BodyStmtTypeName"),			cxlt_do_while_stmt_body_stmt_type,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ConditionExprId"),			cxlt_do_while_stmt_cond_expr_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"CondExprTypeName"),			cxlt_do_while_stmt_cond_expr_type,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppGotoStatement[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"LabelId"),					cxlt_goto_stmt_label_id,					},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"LabelName"),					cxlt_goto_stmt_label_name,				},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppBreakStatement[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppContinueStatement[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppReturnStatement[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"RetValueExprId"),				cxlt_return_stmt_expression_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"RetValueExprObjectType"),		cxlt_return_stmt_expression_type,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppBlockDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppTryBlockDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppCatchBlockDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ExceptionParamId"),			cxlt_catch_block_exception_param_ref,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ExceptionParamName"),		cxlt_catch_block_exception_param_name,	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ExceptionTypeId"),			cxlt_catch_block_exception_type_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ExceptionTypeName"),			cxlt_catch_block_exception_type_name,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppLightBlockDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),					cxlt_item_core,						xtef_none,	xtags_CppItemCore },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppLabelDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"LabelDefined"),				cxlt_label_defined,						},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppCaseLabelDefn[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppDefnInfo"),				cxlt_defn_info,						xtef_none,	xtags_CppDefinition },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"DefaultLabel"),				cxlt_case_label_default,					},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"ValidConstValue"),			cxlt_case_label_const_value,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumericValue"),				cxlt_case_label_value,					},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumericValueExprRef"),		cxlt_case_label_value_expr_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

//
//	------------------------ Expressions ---------------------------
//

// Operand is the only one type of expression that does not have children.
static TXmlDictRecord xtags_CppOperandExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"OperandType"),				cxlt_oprnd_expr_type,					},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"BoolValue"),					cxlt_oprnd_expr_bool_value,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"NumSubtype"),				cxlt_oprnd_expr_num_subt,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumValue"),					cxlt_oprnd_expr_num_value,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"CharconstSubtype"),			cxlt_oprnd_expr_charconst_subt,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"CharconstValue"),				cxlt_oprnd_expr_charconst_value,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"StringSubtype"),				cxlt_oprnd_expr_string_subt,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"StringLength"),				cxlt_oprnd_expr_string_length,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"StringValue"),				cxlt_oprnd_expr_string_value,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"EnumMemberRef"),			cxlt_oprnd_expr_enum_member_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"EnumMemberName"),			cxlt_oprnd_expr_enum_member_name,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"EnumMemberValue"),			cxlt_oprnd_expr_enum_member_value,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"DataFieldRef"),				cxlt_oprnd_expr_data_field_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"DataFieldName"),				cxlt_oprnd_expr_data_field_name,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"CodeEntryPointRef"),			cxlt_oprnd_expr_code_entry_point_ref,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"CodeEntryPointName"),		cxlt_oprnd_expr_code_entry_point_name,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"TemplateNameRef"),			cxlt_oprnd_expr_template_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"TemplateNameName"),			cxlt_oprnd_expr_template_name,			},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppUnaryExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OperandRef"),				cxlt_unary_expr_operand_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppBinaryExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"Operand1Ref"),				cxlt_binary_expr_operand1_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"Operand2Ref"),				cxlt_binary_expr_operand2_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppMultiOpExprOperand[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"Ref"),						cxlt_multiop_expr_oprnd_ref,			},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppMultiOpExprOperandsArray[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"OperandInfo"),				cxlt_multiop_expr_oprnds_array_element,	xtef_none,	xtags_CppMultiOpExprOperand },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppMultiOpExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumOperands"),				cxlt_multiop_expr_num_operands,		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Operands"),					cxlt_multiop_expr_operands,				xtef_none,	xtags_CppMultiOpExprOperandsArray },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppAssignmentExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"Operand1Ref"),				cxlt_assignment_expr_operand1_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"Operand2Ref"),				cxlt_assignment_expr_operand2_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppThrowExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OperandRef"),				cxlt_throw_expr_operand_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppConditionalExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"Operand1Ref"),				cxlt_conditional_expr_operand1_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"Operand2Ref"),				cxlt_conditional_expr_operand2_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"Operand3Ref"),				cxlt_conditional_expr_operand3_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFieldAccessExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"StructOperandRef"),			cxlt_field_access_expr_struct_ref,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"StructMemberRef"),			cxlt_field_access_expr_member_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"StructMemberName"),			cxlt_field_access_expr_member_name,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFieldAccessPtrExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"StructOperandRef"),			cxlt_field_access_ptr_expr_struct_ref,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"StructMemberPtrRef"),			cxlt_field_access_ptr_expr_member_ref,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppArrayIndexExprIndexExpr[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"Ref"),						cxlt_array_index_ref,					},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppArrayIndexExprIndexExprsArray[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"IndexExprInfo"),				cxlt_array_indexes_array_element,		xtef_none,	xtags_CppArrayIndexExprIndexExpr },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppArrayIndexExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ArrayObjectRef"),				cxlt_array_index_expr_array_object_ref,	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumIndexExprs"),				cxlt_array_index_expr_num_indexes,		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"IndexExprs"),				cxlt_array_index_expr_indexes,			xtef_none,	xtags_CppArrayIndexExprIndexExprsArray },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFunctionCallExprParamExpr[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"ExprRef"),					cxlt_func_call_param_ref,		},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFunctionCallExprParamsArray[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"ParamInfo"),					cxlt_func_call_params_array_element,		xtef_none,	xtags_CppFunctionCallExprParamExpr },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppFunctionCallExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FunctionObjectRef"),			cxlt_function_call_expr_function_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"FunctionName"),				cxlt_function_call_expr_function_name,	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumCallParams"),				cxlt_function_call_expr_num_params,		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"CallParams"),				cxlt_function_call_expr_params,			xtef_none,	xtags_CppFunctionCallExprParamsArray },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppCastExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"DestTypeId"),				cxlt_cast_expr_dest_type_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"DestTypeName"),				cxlt_cast_expr_dest_type_name,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OperandRef"),				cxlt_cast_expr_operand_ref,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppSizeofExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"SizeofOfType"),				cxlt_sizeof_expr_data_operand,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"TypeId"),					cxlt_sizeof_expr_source_type_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"TypeName"),					cxlt_sizeof_expr_source_type_name,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OperandRef"),				cxlt_sizeof_expr_operand_ref,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ResultSize"),					cxlt_sizeof_expr_sizeof_value,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppNewExprPlacementExpr[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"Ref"),						cxlt_new_expr_plc_expr_ref,				},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppNewExprPlacementExprsArray[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"PlacementExprInfo"),			cxlt_new_expr_plc_exprs_array_element,	xtef_none,	xtags_CppNewExprPlacementExpr },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppNewExprInitParam[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"ExprRef"),					cxlt_new_expr_init_prm_ref,				},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppNewExprInitParamsArray[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"InitParamInfo"),				cxlt_new_expr_init_prms_array_element,	xtef_none,	xtags_CppNewExprInitParam },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppNewExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"GlobalNew"),					cxlt_new_expr_global_new,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumPlacementExprs"),			cxlt_new_expr_placement_num_exprs,	},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"PlacementExprs"),				cxlt_new_expr_placement_exprs,			xtef_none,	xtags_CppNewExprPlacementExprsArray },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NewTypeId"),				cxlt_new_expr_new_type_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"NewTypeName"),				cxlt_new_expr_new_type_name,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumInitParams"),				cxlt_new_expr_init_num_params,			},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"InitParams"),					cxlt_new_expr_init_params,				xtef_none,	xtags_CppNewExprInitParamsArray },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"CtorCallRef"),				cxlt_new_expr_ctor_call_ref,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"CtorEntryPointId"),			cxlt_new_expr_ctor_func_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"CtorEntryPointName"),			cxlt_new_expr_ctor_func_name,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"CtorCallNumParams"),			cxlt_new_expr_ctor_num_params,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppDeleteExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"GlobalDelete"),				cxlt_delete_expr_global_delete,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"ArrayDelete"),				cxlt_delete_expr_array_delete,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OperandRef"),				cxlt_delete_expr_operand_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CppTypeidExpr[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CppExprInfo"),				cxlt_expr_info,						xtef_none,	xtags_CppExpression },
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"TypeidOfType"),				cxlt_typeid_expr_data_operand,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"SourceTypeId"),				cxlt_typeid_expr_source_type_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"SourceTypeName"),			cxlt_typeid_expr_source_type_name,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OperandRef"),				cxlt_typeid_expr_operand_ref,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"StaticResult"),				cxlt_typeid_expr_resolution_code,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ResultTypeId"),				cxlt_typeid_expr_result_type_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ResultTypeName"),			cxlt_typeid_expr_result_type_name,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ResolutionCodeRef"),			cxlt_typeid_expr_res_code_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumChildren"),				cxlt_item_num_children,				},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Children"),					cxlt_item_children,						xtef_none,	xtags_CppDatabaseItems },
	{ xdrt_eof }
};

//
//	------------------------ Other ---------------------------
//

static TXmlDictRecord xtags_CppDatabaseItems[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppItemHeader"),						cxlt_item_core,				xtef_none,	xtags_CppItemCore },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppDefnHeader"),						cxlt_defn_info,				xtef_none,	xtags_CppDefinition },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppExprHeader"),						cxlt_expr_info,				xtef_none,	xtags_CppExpression },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppCondParsParam"),					cxlt_cond_pars_param,			xtef_none,	xtags_CppCondParsParam },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppBuiltInTypeDefn"),					cxlt_built_in_type,				xtef_none,	xtags_CppBuiltInTypeDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppBitFieldTypeDefn"),					cxlt_bit_field_type,				xtef_none,	xtags_CppBitFieldTypeDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppCvPtrRefTypeDefn"),				cxlt_cvpr_type,				xtef_none,	xtags_CppCvPtrRefTypeDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppArrayTypeDefn"),					cxlt_array_type,				xtef_none,	xtags_CppArrayTypeDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppDataTypeAliasDefn"),				cxlt_data_type_alias,			xtef_none,	xtags_CppDataTypeAliasDefn },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppEnumTypeDefn"),					cxlt_enum_type,				xtef_none,	xtags_CppEnumTypeDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppEnumMemberDefn"),				cxlt_enum_member,			xtef_none,	xtags_CppEnumMemberDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppStructTypeDefn"),					cxlt_struct_type,				xtef_none,	xtags_CppStructTypeDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppBaseClassSpec"),					cxlt_base_class_spec,			xtef_none,	xtags_CppBaseClassSpec },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppFieldAlignSpec"),					cxlt_field_align_spec,			xtef_none,	xtags_CppFieldAlignSpec },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppFriendSpec"),						cxlt_friend_spec,				xtef_none,	xtags_CppFriendSpec },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppDataFieldDefn"),					cxlt_data_field,				xtef_none,	xtags_CppDataFieldDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppDataFieldInitializer"),				cxlt_data_field_initializer,		xtef_none,	xtags_CppDataFieldInitializer },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppFunctionTypeDefn"),				cxlt_function_type,				xtef_none,	xtags_CppFunctionTypeDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppFunctionTypeAliasDefn"),			cxlt_function_type_alias,		xtef_none,	xtags_CppFunctionTypeAliasDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppCodeEntryPointDefn"),				cxlt_code_entry_point,			xtef_none,	xtags_CppCodeEntryPointDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppPossibleExceptionSpec"),			cxlt_exception_spec,			xtef_none,	xtags_CppPossibleExceptionSpec },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppNamespaceDefn"),					cxlt_namespace,				xtef_none,	xtags_CppNamespaceDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppGlobalNamespaceDefn"),			cxlt_global_namespace,			xtef_none,	xtags_CppGlobalNamespaceDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppNamespaceAliasDefn"),				cxlt_namespace_alias,			xtef_none,	xtags_CppNamespaceAliasDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppUsingDeclarationDefn"),				cxlt_using_declaration,			xtef_none,	xtags_CppUsingDeclarationDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppUsingDirectiveDefn"),				cxlt_using_directive,			xtef_none,	xtags_CppUsingDirectiveDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppAsmInsertDefn"),					cxlt_asm_insert,				xtef_none,	xtags_CppAsmInsertDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppLinkageSpec"),					cxlt_linkage_spec,				xtef_none,	xtags_CppLinkageSpec },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppClassTemplateDefn"),				cxlt_class_template,			xtef_none,	xtags_CppClassTemplateDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppClassTemplateSpecialization"),		cxlt_class_template_spec,		xtef_none,	xtags_CppClassTemplateSpecialization },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppClassTemplateInstantiation"),		cxlt_class_template_inst,		xtef_none,	xtags_CppClassTemplateInstantiation },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppFunctionTemplateDefn"),			cxlt_func_template,			xtef_none,	xtags_CppFunctionTemplateDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppFunctionTemplateSpecialization"),		cxlt_func_template_spec,		xtef_none,	xtags_CppFunctionTemplateSpecialization },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppFunctionTemplateInstantiation"),		cxlt_func_template_inst,			xtef_none,	xtags_CppFunctionTemplateInstantiation },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppTemplateTypeParamDefn"),			cxlt_type_param,				xtef_none,	xtags_CppTemplateTypeParamDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppTemplateTemplateParamDefn"),		cxlt_template_param,			xtef_none,	xtags_CppTemplateTemplateParamDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppTemplateTypeArgument"),			cxlt_type_argument,			xtef_none,	xtags_CppTemplateTypeArgument },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppTemplateTemplateArgument"),		cxlt_template_argument,		xtef_none,	xtags_CppTemplateTemplateArgument },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppUnresolvedNameInfo"),				cxlt_unresolved_name,			xtef_none,	xtags_CppUnresolvedNameInfo },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppUnresolvedDataType"),				cxlt_unres_data_type,			xtef_none,	xtags_CppUnresolvedDataType },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppUnresolvedStructType"),			cxlt_unres_struct_type,			xtef_none,	xtags_CppUnresolvedStructType },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppUnresolvedDataField"),				cxlt_unres_data_field,			xtef_none,	xtags_CppUnresolvedDataField },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppUnresolvedCodeEntryPoint"),			cxlt_unres_code_entry_point,	xtef_none,	xtags_CppUnresolvedCodeEntryPoint },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppUnresolvedClassTemplate"),			cxlt_unres_class_template,		xtef_none,	xtags_CppUnresolvedClassTemplate },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppUnresolvedFunctionTemplate"),		cxlt_unres_func_template,		xtef_none,	xtags_CppUnresolvedFunctionTemplate },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppEmptyStatement"),					cxlt_empty_stmt,				xtef_none,	xtags_CppEmptyStatement },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppIfStatement"),						cxlt_if_stmt,					xtef_none,	xtags_CppIfStatement },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppSwitchStatement"),					cxlt_switch_stmt,				xtef_none,	xtags_CppSwitchStatement },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppForStatement"),					cxlt_for_stmt,					xtef_none,	xtags_CppForStatement },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppWhileStatement"),					cxlt_while_stmt,				xtef_none,	xtags_CppWhileStatement },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppDoWhileStatement"),				cxlt_do_while_stmt,			xtef_none,	xtags_CppDoWhileStatement },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppGotoStatement"),					cxlt_goto_stmt,				xtef_none,	xtags_CppGotoStatement },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppBreakStatement"),					cxlt_break_stmt,				xtef_none,	xtags_CppBreakStatement },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppContinueStatement"),				cxlt_continue_stmt,				xtef_none,	xtags_CppContinueStatement },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppReturnStatement"),				cxlt_return_stmt,				xtef_none,	xtags_CppReturnStatement },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppBlockDefn"),						cxlt_block,					xtef_none,	xtags_CppBlockDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppTryBlockDefn"),					cxlt_try_block,				xtef_none,	xtags_CppTryBlockDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppCatchBlockDefn"),					cxlt_catch_block,				xtef_none,	xtags_CppCatchBlockDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppLightBlockDefn"),					cxlt_light_block,				xtef_none,	xtags_CppLightBlockDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppLabelDefn"),						cxlt_label,					xtef_none,	xtags_CppLabelDefn },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppCaseLabelDefn"),					cxlt_case_label,				xtef_none,	xtags_CppCaseLabelDefn },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppOperandExpr"),					cxlt_oprnd_expr,				xtef_none,	xtags_CppOperandExpr },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppUnaryExpr"),						cxlt_unary_expr,				xtef_none,	xtags_CppUnaryExpr },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppBinaryExpr"),						cxlt_binary_expr,				xtef_none,	xtags_CppBinaryExpr },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppMultiOpExpr"),						cxlt_multiop_expr,				xtef_none,	xtags_CppMultiOpExpr },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppAssignmentExpr"),					cxlt_assignment_expr,			xtef_none,	xtags_CppAssignmentExpr },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppThrowExpr"),						cxlt_throw_expr,				xtef_none,	xtags_CppThrowExpr },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppConditionalExpr"),					cxlt_conditional_expr,			xtef_none,	xtags_CppConditionalExpr },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppFieldAccessExpr"),					cxlt_field_access_expr,			xtef_none,	xtags_CppFieldAccessExpr },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppFieldAccessPtrExpr"),				cxlt_field_access_ptr_expr,		xtef_none,	xtags_CppFieldAccessPtrExpr },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppArrayIndexExpr"),					cxlt_array_index_expr,			xtef_none,	xtags_CppArrayIndexExpr },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppFunctionCallExpr"),					cxlt_function_call_expr,			xtef_none,	xtags_CppFunctionCallExpr },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppCastExpr"),						cxlt_cast_expr,				xtef_none,	xtags_CppCastExpr },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppSizeofExpr"),						cxlt_sizeof_expr,				xtef_none,	xtags_CppSizeofExpr },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppNewExpr"),						cxlt_new_expr,				xtef_none,	xtags_CppNewExpr },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppDeleteExpr"),						cxlt_delete_expr,				xtef_none,	xtags_CppDeleteExpr },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppTypeidExpr"),						cxlt_typeid_expr,				xtef_none,	xtags_CppTypeidExpr },

	{ xdrt_eof }
};

//-----------------------------------------------------------------------------------
//  =====================  TCppDatabaseXmlHelper  ========================
//-----------------------------------------------------------------------------------

void TCppDatabaseXmlHelper::DumpCppItemCore(TXmlWriter &doc, WORD tag_id, TCppItemBase &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_item_core, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	doc.WriteNumTag(cxlt_item_core_id, data.m_cib_id);

	if (data.m_cib_parent != NULL)
		doc.WriteNumTag(cxlt_item_core_parent_id, data.m_cib_parent->ItemId());

	doc.WriteStringTag(cxlt_item_core_item_type, data.ItemTypeEnumName());
	doc.WriteNumTag(cxlt_item_core_num_pars_evts, data.m_cib_ctv_refs.NumItems());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpCppDefnHeader(TXmlWriter &doc, WORD tag_id, TCppDefnBase &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_defn_info, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);
	DumpCppItemCore(doc, cxlt_item_core, data);

	if (data.DefnProblem() != cdpr_none)
		doc.WriteStringTag(cxlt_defn_info_problem, TCppDefnBase::GetDefnProblemEnumName(data.DefnProblem()));

	doc.WriteStringTag(cxlt_defn_info_name, data.DefnName());
	doc.WriteNumTag(cxlt_defn_info_key_id, data.DefnKeyId());

	if (data.DefnElabType() != elab_none)
		doc.WriteStringTag(cxlt_defn_info_elab_type, data.GetElabTypeName());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpCppExprHeader(TXmlWriter &doc, WORD tag_id, TCppExpressionBase &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_expr_info, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);
	DumpCppItemCore(doc, cxlt_item_core, data);

	if (data.m_operation == 0 || data.m_operation == opr_none)
		doc.WriteStringTag(cxlt_expr_info_operation, L"opr_none");
	else doc.WriteStringTag(cxlt_expr_info_operation, TLexema::GetKeywordIdEnumName(data.m_operation));

	if (data.m_lvalue_expr == TRUE)
		doc.WriteBoolTag(cxlt_expr_info_lvalue_expr, data.m_lvalue_expr);
	if (data.m_const_expr == TRUE)
		doc.WriteBoolTag(cxlt_expr_info_const_expr, data.m_const_expr);

	doc.WriteNumTag(cxlt_expr_info_result_type_id, (data.m_result_type != NULL) ? data.m_result_type->ItemId() : 0);
	if (data.m_result_type != NULL)
		doc.WriteStringTag(cxlt_expr_info_result_type_name, data.m_result_type->DefnName());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpNamespaceStatistics(TXmlWriter &doc, WORD tag_id, TCppDefnBase &data)
{
	assert(data.IsNamespace() == TRUE);

	// This container cannot be used as a root container. Use simple open container method.
	doc.OpenContainer(tag_id);

	TList *inner_elements_list = NULL;
	if (data.ItemType() == cxx_namespace)
		inner_elements_list = &(((TCppNamespaceDefn&)data).m_members);
	else if (data.ItemType() == cxx_global_namespace)
		inner_elements_list = &(((TCppGlobalNamespaceDefn&)data).m_members);

	if (inner_elements_list != NULL)
	{
		TList &items_list = *inner_elements_list;
		int num_direct_children = data.NumDirectChildren();
		assert(num_direct_children >= 0);

		int num_enum_types = data.GetNumEnumTypes();
		int num_struct_types = data.GetNumStructTypes(elab_struct);
		int num_union_types = data.GetNumStructTypes(elab_union);
		int num_class_types = data.GetNumStructTypes(elab_class);
		int num_structured_types = num_struct_types+num_union_types+num_class_types;

		int num_data_fields = data.GetNumDataFields();
		int num_code_entry_points = data.GetNumCodeEntryPoints();
		int num_other_objects = num_direct_children-num_enum_types-num_structured_types-num_data_fields-num_code_entry_points;
		assert(num_other_objects >= 0);

		if (num_enum_types > 0)
			doc.WriteNumTag(cxlt_namespace_stts_enum_types, num_enum_types);
		if (num_struct_types > 0)
			doc.WriteNumTag(cxlt_namespace_stts_struct_types, num_struct_types);
		if (num_union_types > 0)
			doc.WriteNumTag(cxlt_namespace_stts_union_types, num_union_types);
		if (num_class_types > 0)
			doc.WriteNumTag(cxlt_namespace_stts_class_types, num_class_types);
		if (num_structured_types > 0)
			doc.WriteNumTag(cxlt_namespace_stts_structured_types, num_structured_types);

		if (num_data_fields > 0)
			doc.WriteNumTag(cxlt_namespace_stts_data_fields, num_data_fields);
		if (num_code_entry_points > 0)
			doc.WriteNumTag(cxlt_namespace_stts_code_entry_points, num_code_entry_points);
		if (num_other_objects > 0)
			doc.WriteNumTag(cxlt_namespace_stts_other_objects, num_other_objects);

		doc.WriteNumTag(cxlt_namespace_stts_total_objects, num_direct_children);
	}

	doc.CloseContainer();
}

void TCppDatabaseXmlHelper::DumpStructMemberStatistics(TXmlWriter &doc, WORD tag_id, TCppStructTypeDefn &data)
{
	// This container cannot be used as a root container. Use simple open container method.
	doc.OpenContainer(tag_id);

	int num_direct_children = data.NumDirectChildren();
	assert(num_direct_children >= 0);

	int num_inst_data_fields = data.GetNumDataFields(strgc_none);
	int num_static_data_fields = data.GetNumDataFields(strgc_static);
	int num_code_entry_points = data.GetNumCodeEntryPoints();
	int num_other_objects = num_direct_children-num_inst_data_fields-num_static_data_fields-num_code_entry_points;
	assert(num_other_objects >= 0);

	if (num_inst_data_fields > 0)
		doc.WriteNumTag(cxlt_struct_type_stts_inst_data_fields, num_inst_data_fields);
	if (num_static_data_fields > 0)
		doc.WriteNumTag(cxlt_struct_type_stts_static_data_fields, num_static_data_fields);
	if (num_code_entry_points > 0)
		doc.WriteNumTag(cxlt_struct_type_stts_code_entry_points, num_code_entry_points);
	if (num_other_objects > 0)
		doc.WriteNumTag(cxlt_struct_type_stts_other_objects, num_other_objects);

	doc.WriteNumTag(cxlt_struct_type_stts_total_objects, num_direct_children);

	doc.CloseContainer();
}

void TCppDatabaseXmlHelper::DumpFunctionTypeProps(TXmlWriter &doc, WORD tag_id, TCppFunctionTypeDefn &data)
{
	// This container cannot be used as a root container. Use simple open container method.
	doc.OpenContainer(tag_id);

	if (data.m_visibility_pars_sess != 0)
		doc.WriteNumTag(cxlt_func_type_props_visib_pars_sess, data.m_visibility_pars_sess);

	if (data.m_linkage_spec != NULL)
	{
		doc.WriteNumTag(cxlt_func_type_props_linkage_spec_ref, data.m_linkage_spec->ItemId());
		doc.WriteStringTag(cxlt_func_type_props_linkage_spec_name, data.m_linkage_spec->m_spec_name);
	}

	doc.WriteStringTag(cxlt_func_type_props_access_level, TCppDataFieldDefn::GetAccessLevelFriendlyName(data.m_accs_level));

	if (data.m_func_attrs != fcta_none)
	{
		// Non empty function attrs are present.  Emit them as a bimary mask.
		doc.WriteNumTag(cxlt_func_type_props_func_attrs, data.m_func_attrs);

		// Emit function attrs as a list of keywords.
		wchar_t func_attrs_buffer[256];
		doc.WriteStringTag(cxlt_func_type_props_func_attrs_as_text, TCppDatabase::GetFuncAttrsMaskFriendlyName(func_attrs_buffer, data.m_func_attrs));
	}

	if (data.m_spec_type != spcft_none)
		doc.WriteStringTag(cxlt_func_type_props_spec_type, TCppFunctionTypeDefn::GetSpecFuncTypeEnumName(data.m_spec_type));

	if (data.m_spec_type == spcft_operator)
	{
		if (data.m_operator_token == 0 || data.m_operator_token == opr_none)
			doc.WriteStringTag(cxlt_func_type_props_operator_type, L"opr_none");
		else doc.WriteStringTag(cxlt_func_type_props_operator_type, TLexema::GetKeywordIdEnumName(data.m_operator_token));
	}

	// Emit information about the type of the return value.
	doc.WriteNumTag(cxlt_func_type_props_ret_val_type_ref, (data.m_return_value_type != NULL) ? data.m_return_value_type->ItemId() : 0);
	if (data.m_return_value_type != NULL)
		doc.WriteStringTag(cxlt_func_type_props_ret_val_type_name, data.m_return_value_type->DefnName());

	// Emit information about the parameters of the function.
	doc.WriteNumTag(cxlt_func_type_props_num_params, data.GetNumParams());
	TCppDataFieldDefn *param = data.GetFirstParam();

	if (param != NULL)
	{
		// Params are available.
		doc.OpenContainer(cxlt_func_type_props_params);

		while (param != NULL)
		{
			doc.OpenContainer(cxlt_func_params_array_element);

			doc.WriteNumTag(cxlt_func_param_ref, param->ItemId());
			doc.WriteStringTag(cxlt_func_param_name, param->DefnName());
			doc.WriteNumTag(cxlt_func_param_type_ref, (param->m_field_type != NULL) ? param->m_field_type->ItemId() : 0);
			if (param->m_field_type != NULL)
				doc.WriteStringTag(cxlt_func_param_type_name, param->m_field_type->DefnName());

			doc.CloseContainer();

			param = data.GetNextParam(param);
		}

		doc.CloseContainer();
	}

	// Emit information about the possible exception specs.
	TCppPossibleExceptionSpec *throw_spec = data.GetFirstThrowSpec();
	if (throw_spec != NULL)
	{
		// Throw specs are available.
		doc.WriteNumTag(cxlt_func_type_props_num_throws, data.GetNumThrowSpecs());
		doc.OpenContainer(cxlt_func_type_props_throws);

		while (throw_spec != NULL)
		{
			doc.OpenContainer(cxlt_func_throws_array_element);

			doc.WriteNumTag(cxlt_func_throws_ref, throw_spec->ItemId());
			doc.WriteNumTag(cxlt_func_throws_type_ref, (throw_spec->m_exception_type != NULL) ? throw_spec->m_exception_type->ItemId() : 0);
			if (throw_spec->m_exception_type != NULL)
				doc.WriteStringTag(cxlt_func_throws_type_name, throw_spec->m_exception_type->DefnName());

			doc.CloseContainer();

			throw_spec = data.GetNextThrowSpec(throw_spec);
		}

		doc.CloseContainer();
	}

	doc.CloseContainer();
}

void TCppDatabaseXmlHelper::DumpTemplateParamInfos(TXmlWriter &doc, WORD tag_id, TList &data)
{
	// This container cannot be used as a root container. Use simple open container method.
	doc.OpenContainer(tag_id);

	TCppDefnBase *param = (TCppDefnBase*)data.GetFirst();
	while (param != NULL)
	{
		if (TCppItemBase::IsTemplateParam(param) == TRUE)
		{
			doc.OpenContainer(cxlt_tplt_param_infos_array_element);

			doc.WriteNumTag(cxlt_tplt_param_info_ref, param->ItemId());
			doc.WriteStringTag(cxlt_tplt_param_info_name, param->DefnName());
			doc.WriteStringTag(cxlt_tplt_param_info_type, param->ItemTypeEnumName());

			if (param->ItemType() == cxx_data_field)
			{
				TCppDataFieldDefn *data_param = (TCppDataFieldDefn*)param;
				doc.WriteNumTag(cxlt_tplt_param_info_data_type_ref, (data_param->m_field_type != NULL) ? data_param->m_field_type->ItemId() : 0);
				if (data_param->m_field_type != NULL)
					doc.WriteStringTag(cxlt_tplt_param_info_data_type_name, data_param->m_field_type->DefnName());
			}
			else if (param->ItemType() == cxx_template_template_param)
			{
				TCppTemplateTemplateParamDefn *tplt_tplt_param = (TCppTemplateTemplateParamDefn*)param;
				doc.WriteNumTag(cxlt_tplt_param_info_tplt_num_params, tplt_tplt_param->m_template_params.NumItems());
			}

			doc.CloseContainer();
		}

		param = (TCppDefnBase*)data.GetNext(param);
	}

	doc.CloseContainer();
}

void TCppDatabaseXmlHelper::DumpCondParsParam(TXmlWriter &doc, WORD tag_id, TCppCondParsParam &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_cond_pars_param);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_cond_pars_param, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);
	DumpCppDefnHeader(doc, cxlt_defn_info, data);
	doc.CloseContainerEx(rcnr_dct);
}

//
// ----------------------------  Definitions-1  -----------------------------------
//

void TCppDatabaseXmlHelper::DumpBuiltInTypeDefn(TXmlWriter &doc, WORD tag_id, TCppBuiltInTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_built_in_type);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_built_in_type, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteStringTag(cxlt_built_in_type_real_type, TCppBuiltInTypeDefn::GetBuiltInTypeTypeEnumName(data.m_type));
	doc.WriteBoolTag(cxlt_built_in_type_unsigned, data.m_unsigned);
	doc.WriteNumTag(cxlt_built_in_type_byte_len, data.m_byte_length);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpBitFieldTypeDefn(TXmlWriter &doc, WORD tag_id, TCppBitFieldTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_bit_field_type);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_bit_field_type, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_bit_field_type_base_type_id, (data.m_base_type != NULL) ? data.m_base_type->ItemId() : 0);
	if (data.m_base_type != NULL)
		doc.WriteStringTag(cxlt_bit_field_type_base_type_name, data.m_base_type->DefnName());

	if (data.m_bit_length < 0)
		doc.WriteBoolTag(cxlt_bit_field_type_const_expr, FALSE);

	doc.WriteNumTag(cxlt_bit_field_type_bit_size, data.m_bit_length);
	doc.WriteNumTag(cxlt_bit_field_type_expr_ref, (data.m_bit_length_expr != NULL) ? data.m_bit_length_expr->ItemId() : 0);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpCvPtrRefTypeDefn(TXmlWriter &doc, WORD tag_id, TCppCvPtrRefTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_cv_ptr_ref_type);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_cvpr_type, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_cvpr_type_base_type_id, (data.m_base_type != NULL) ? data.m_base_type->ItemId() : 0);
	if (data.m_base_type != NULL)
		doc.WriteStringTag(cxlt_cvpr_type_base_type_name, data.m_base_type->DefnName());

	doc.WriteNumTag(cxlt_cvpr_type_num_layers, data.m_layers.NumItems());
	doc.OpenContainer(cxlt_cvpr_type_layers);

	// Iterate available indirection layers.
	for (int ilayer=0; ilayer<data.m_layers.NumItems(); ++ilayer)
	{
		TCppCvPtrRefSpec &spec = data.m_layers[ilayer];
		doc.OpenContainer(cxlt_cvpr_specs_array_element);

		// Dump pointer and reference fields first to make result clearer.
		if (spec.cvpr_pointer == TRUE)
			doc.WriteBoolTag(cxlt_cvpr_spec_pointer, spec.cvpr_pointer);
		if (spec.cvpr_reference == TRUE)
			doc.WriteBoolTag(cxlt_cvpr_spec_reference, spec.cvpr_reference);

		if (spec.cvpr_const == TRUE)
			doc.WriteBoolTag(cxlt_cvpr_spec_const, spec.cvpr_const);
		if (spec.cvpr_volatile == TRUE)
			doc.WriteBoolTag(cxlt_cvpr_spec_volatile, spec.cvpr_volatile);

		if (spec.cvpr_struct != NULL)
		{
			doc.WriteNumTag(cxlt_cvpr_spec_struct_id, spec.cvpr_struct->ItemId());
			doc.WriteStringTag(cxlt_cvpr_spec_struct_name, spec.cvpr_struct->DefnName());
		}

		doc.CloseContainer();
	}

	doc.CloseContainer();
	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpArrayTypeDefn(TXmlWriter &doc, WORD tag_id, TCppArrayTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_array_type);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_array_type, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_array_type_base_type_id, (data.m_base_type != NULL) ? data.m_base_type->ItemId() : 0);
	if (data.m_base_type != NULL)
		doc.WriteStringTag(cxlt_array_type_base_type_name, data.m_base_type->DefnName());

	doc.WriteNumTag(cxlt_array_type_num_dimensions, data.m_dimensions.NumItems());
	doc.OpenContainer(cxlt_array_type_dimensions);

	// Iterate available dimensions.
	for (int idim=0; idim<data.m_dimensions.NumItems(); ++idim)
	{
		TCppArrayDimensionSpec &spec = data.m_dimensions[idim];
		doc.OpenContainer(cxlt_array_dims_array_element);

		// The number of elements should be dumped even if it is wrong/bogus.
		doc.WriteNumTag(cxlt_array_dim_num_elements, spec.m_num_elements);

		if (spec.m_num_elements < 0)
			doc.WriteBoolTag(cxlt_array_dim_num_elems_expr_const, FALSE);

		doc.WriteNumTag(cxlt_array_dim_num_elems_expr_ref, (spec.m_num_elements_expr != NULL) ? spec.m_num_elements_expr->ItemId() : 0);

		// This structure may have children when the size of array is defined using a non const expression
		// in the definition of the class or function template.
		if (spec.m_num_elements_expr != NULL && cxdcm_mode != cxdcm_none)
		{
			if (cxdcm_mode == cxdcm_num_children)
			{
				doc.WriteNumTag(cxlt_item_num_children, 1);
			}
			else if (cxdcm_mode == cxdcm_dump_one_layer)
			{
				doc.OpenContainer(cxlt_item_children);
				DumpCppItem(doc, 0, *spec.m_num_elements_expr, cxdcm_none);
				doc.CloseContainer();
			}
			else if (cxdcm_mode == cxdcm_dump_subtree)
			{
				doc.OpenContainer(cxlt_item_children);
				DumpCppItem(doc, 0, *spec.m_num_elements_expr, cxdcm_dump_subtree);
				doc.CloseContainer();
			}
		}

		doc.CloseContainer();
	}

	doc.CloseContainer();
	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpDataTypeAliasDefn(TXmlWriter &doc, WORD tag_id, TCppDataTypeAliasDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_data_type_alias);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_data_type_alias, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_data_type_alias_base_type_id, (data.m_base_type != NULL) ? data.m_base_type->ItemId() : 0);
	if (data.m_base_type != NULL)
		doc.WriteStringTag(cxlt_data_type_alias_base_type_name, data.m_base_type->DefnName());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpEnumTypeDefn(TXmlWriter &doc, WORD tag_id, TCppEnumTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_enum_type);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_enum_type, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	if (data.m_defined == FALSE)
		doc.WriteBoolTag(cxlt_enum_type_forward_decl, TRUE);

	if (data.m_base_type != NULL)
	{
		doc.WriteNumTag(cxlt_enum_type_base_type_ref, data.m_base_type->ItemId());
		doc.WriteStringTag(cxlt_enum_type_base_type_name, data.m_base_type->DefnName());
	}

	doc.WriteNumTag(cxlt_enum_type_num_members, data.GetNumEnumMembers());

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpEnumMemberDefn(TXmlWriter &doc, WORD tag_id, TCppEnumMemberDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_enum_member);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_enum_member, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteBoolTag(cxlt_enum_member_explicit_value, (data.m_value_expr != NULL) ? TRUE : FALSE);

	// It makes sense to write if the value is const or not only if the expression is present.
	// Otherwise, once the value is implicit, it is always const.
	if (data.m_value_expr != NULL)
		doc.WriteBoolTag(cxlt_enum_member_const_value, data.m_const_value);

	if (data.m_const_value == TRUE)
	{
		doc.WriteNumTag(cxlt_enum_member_value, data.m_value);
		doc.WriteStringTag(cxlt_enum_member_value_subt, TLexema::GetLexNumberTypeEnumName(data.m_value_num_subt));
	}

	if (data.m_value_expr != NULL)
		doc.WriteNumTag(cxlt_enum_member_value_expr_ref, data.m_value_expr->ItemId());

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpStructTypeDefn(TXmlWriter &doc, WORD tag_id, TCppStructTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_struct_type);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_struct_type, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	if (data.m_defined == FALSE)
		doc.WriteBoolTag(cxlt_struct_type_forward_decl, TRUE);
	if (data.m_members.IsEmpty() == FALSE)
		DumpStructMemberStatistics(doc, cxlt_struct_type_member_statistics, data);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpBaseClassSpec(TXmlWriter &doc, WORD tag_id, TCppBaseClassSpec &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_base_class_spec);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_base_class_spec, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	if (data.m_virtual == TRUE)
		doc.WriteBoolTag(cxlt_base_class_spec_virtual, data.m_virtual);

	doc.WriteStringTag(cxlt_base_class_spec_access_level, TCppDataFieldDefn::GetAccessLevelFriendlyName(data.m_accs_level));

	doc.WriteNumTag(cxlt_base_class_spec_base_class_ref, (data.m_base_class != NULL) ? data.m_base_class->ItemId() : 0);
	if (data.m_base_class != NULL)
		doc.WriteStringTag(cxlt_base_class_spec_base_class_name, data.m_base_class->DefnName());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpFieldAlignSpec(TXmlWriter &doc, WORD tag_id, TCppFieldAlignSpec &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_field_align_spec);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_field_align_spec, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	if (data.m_align_value < 0)
		doc.WriteBoolTag(cxlt_field_align_spec_const_expr, FALSE);

	doc.WriteNumTag(cxlt_field_align_spec_align_value, data.m_align_value);
	doc.WriteNumTag(cxlt_field_align_spec_expr_ref, (data.m_align_value_expr != NULL) ? data.m_align_value_expr->ItemId() : 0);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpFriendSpec(TXmlWriter &doc, WORD tag_id, TCppFriendSpec &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_friend_spec);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_friend_spec, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.WriteNumTag(cxlt_friend_spec_friend_ref, (data.m_friendly_object != NULL) ? data.m_friendly_object->ItemId() : 0);
	if (data.m_friendly_object != NULL)
		doc.WriteStringTag(cxlt_friend_spec_friend_name, data.m_friendly_object->DefnName());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpDataFieldDefn(TXmlWriter &doc, WORD tag_id, TCppDataFieldDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_data_field);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_data_field, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	if (data.m_visibility_pars_sess != 0)
		doc.WriteNumTag(cxlt_data_field_visib_pars_sess, data.m_visibility_pars_sess);

	if (data.m_linkage_spec != NULL)
	{
		doc.WriteNumTag(cxlt_data_field_linkage_spec_ref, data.m_linkage_spec->ItemId());
		doc.WriteStringTag(cxlt_data_field_linkage_spec_name, data.m_linkage_spec->m_spec_name);
	}

	if (data.m_strg_class != strgc_param && data.m_strg_class != strgc_template_param && data.m_strg_class != strgc_catch_block_param)
		doc.WriteStringTag(cxlt_data_field_access_level, TCppDataFieldDefn::GetAccessLevelFriendlyName(data.m_accs_level));

	doc.WriteStringTag(cxlt_data_field_storage_class, TCppDataFieldDefn::GetStorageClassEnumName(data.m_strg_class));

	if (data.m_strg_class == strgc_param || data.m_strg_class == strgc_template_param && data.m_strg_class != strgc_catch_block_param)
		doc.WriteNumTag(cxlt_data_field_param_index, data.m_param_index);
	if (data.m_mutable == TRUE)
		doc.WriteBoolTag(cxlt_data_field_mutable, data.m_mutable);

	doc.WriteNumTag(cxlt_data_field_data_type_ref, (data.m_field_type != NULL) ? data.m_field_type->ItemId() : 0);
	if (data.m_field_type != NULL)
		doc.WriteStringTag(cxlt_data_field_data_type_name, data.m_field_type->DefnName());

	if (data.m_field_initializer != NULL)
	{
		doc.WriteNumTag(cxlt_data_field_init_ref, data.m_field_initializer->ItemId());
		DumpCppItemChildren(doc, data, cxdcm_mode);
	}

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpDataFieldInitializer(TXmlWriter &doc, WORD tag_id, TCppDataFieldInitializer &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_data_field_initializer);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_data_field_initializer, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.WriteStringTag(cxlt_data_field_initializer_init_type, TCppDataFieldInitializer::GetInitializerTypeEnumName(data.m_init_type));

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

//
// ----------------------------  Definitions-2  -----------------------------------
//

void TCppDatabaseXmlHelper::DumpFunctionTypeDefn(TXmlWriter &doc, WORD tag_id, TCppFunctionTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_function_type);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_function_type, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);
	DumpFunctionTypeProps(doc, cxlt_function_type_func_props, data);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpFunctionTypeAliasDefn(TXmlWriter &doc, WORD tag_id, TCppFunctionTypeAliasDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_function_type_alias);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_function_type_alias, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_function_type_alias_base_type_ref, (data.m_base_type != NULL) ? data.m_base_type->ItemId() : 0);
	if (data.m_base_type != NULL)
		doc.WriteStringTag(cxlt_function_type_alias_base_type_name, data.m_base_type->DefnName());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpCodeEntryPointDefn(TXmlWriter &doc, WORD tag_id, TCppCodeEntryPointDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_code_entry_point);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_code_entry_point, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);
	DumpFunctionTypeProps(doc, cxlt_code_entry_point_func_props, data);

	doc.WriteNumTag(cxlt_code_entry_point_func_body_ref, (data.m_function_body != NULL) ? data.m_function_body->ItemId() : 0);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpPossibleExceptionSpec(TXmlWriter &doc, WORD tag_id, TCppPossibleExceptionSpec &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_possible_exception_spec);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_exception_spec, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.WriteNumTag(cxlt_exception_spec_data_type_ref, (data.m_exception_type != NULL) ? data.m_exception_type->ItemId() : 0);
	if (data.m_exception_type != NULL)
		doc.WriteStringTag(cxlt_exception_spec_data_type_name, data.m_exception_type->DefnName());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpNamespaceDefn(TXmlWriter &doc, WORD tag_id, TCppNamespaceDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_namespace);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_namespace, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);
	DumpNamespaceStatistics(doc, cxlt_namespace_elements_statistics, data);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpGlobalNamespaceDefn(TXmlWriter &doc, WORD tag_id, TCppGlobalNamespaceDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_global_namespace);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_global_namespace, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);
	DumpNamespaceStatistics(doc, cxlt_global_namespace_elements_stts, data);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpNamespaceAliasDefn(TXmlWriter &doc, WORD tag_id, TCppNamespaceAliasDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_namespace_alias);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_namespace_alias, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_namespace_alias_namespace_ref, (data.m_base_namespace != NULL) ? data.m_base_namespace->ItemId() : 0);
	if (data.m_base_namespace != NULL)
		doc.WriteStringTag(cxlt_namespace_alias_namespace_name, data.m_base_namespace->DefnName());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpUsingDeclarationDefn(TXmlWriter &doc, WORD tag_id, TCppUsingDeclarationDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_using_declaration);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_using_declaration, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_using_declaration_defn_ref, (data.m_defn_to_use != NULL) ? data.m_defn_to_use->ItemId() : 0);
	if (data.m_defn_to_use != NULL)
		doc.WriteStringTag(cxlt_using_declaration_defn_name, data.m_defn_to_use->DefnName());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpUsingDirectiveDefn(TXmlWriter &doc, WORD tag_id, TCppUsingDirectiveDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_using_directive);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_using_directive, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_using_directive_namespace_ref, (data.m_namespace_to_use != NULL) ? data.m_namespace_to_use->ItemId() : 0);
	if (data.m_namespace_to_use != NULL)
		doc.WriteStringTag(cxlt_using_directive_namespace_name, data.m_namespace_to_use->DefnName());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpAsmInsertDefn(TXmlWriter &doc, WORD tag_id, TCppAsmInsertDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_asm_insert);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_asm_insert, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_asm_insert_length, data.m_asm_insert.m_len);

	wchar_t buffer[256];
	TStringPtr str_ptr(data.m_asm_insert);
	doc.WriteStringTag(cxlt_asm_insert_value, str_ptr.CopyWithTruncationTo(buffer, 256));

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpLinkageSpec(TXmlWriter &doc, WORD tag_id, TCppLinkageSpec &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_linkage_spec);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_linkage_spec, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.WriteStringTag(cxlt_linkage_spec_name, data.m_spec_name);
	doc.WriteNumTag(cxlt_linkage_spec_key_id, data.m_spec_key_id);
	doc.WriteNumTag(cxlt_linkage_spec_cnt_use, data.m_spec_cnt_use);

	doc.CloseContainerEx(rcnr_dct);
}

//
// ----------------------------  Templates  -----------------------------------
//

void TCppDatabaseXmlHelper::DumpClassTemplateDefn(TXmlWriter &doc, WORD tag_id, TCppClassTemplateDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_class_template);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_class_template, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	if (data.m_defined == FALSE)
		doc.WriteBoolTag(cxlt_class_template_forward_decl, TRUE);
	if (data.m_members.IsEmpty() == FALSE)
		DumpStructMemberStatistics(doc, cxlt_class_template_member_stts, data);

	int num_template_params = data.GetNumTemplateParams();
	doc.WriteNumTag(cxlt_class_template_num_params, num_template_params);
	if (num_template_params > 0)
		DumpTemplateParamInfos(doc, cxlt_class_template_params, data.m_members);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpClassTemplateSpecialization(TXmlWriter &doc, WORD tag_id, TCppClassTemplateSpecialization &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_class_template_spec);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_class_template_spec, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_class_template_spec_template_ref, (data.m_base_template != NULL) ? data.m_base_template->ItemId() : 0);
	if (data.m_base_template != NULL)
		doc.WriteStringTag(cxlt_class_template_spec_template_name, data.m_base_template->DefnName());

	if (data.m_defined == FALSE)
		doc.WriteBoolTag(cxlt_class_template_spec_forward_decl, TRUE);
	if (data.m_members.IsEmpty() == FALSE)
		DumpStructMemberStatistics(doc, cxlt_class_template_spec_member_stts, data);

	for (TListIter<TCppItemBase> iter(data.m_specialization_params); iter; ++iter)
		doc.WriteNumTag(cxlt_class_template_spec_param_ref, iter.CurrItem().ItemId());

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpClassTemplateInstantiation(TXmlWriter &doc, WORD tag_id, TCppClassTemplateInstantiation &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_class_template_inst);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_class_template_inst, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_class_template_inst_template_ref, (data.m_original_template != NULL) ? data.m_original_template->ItemId() : 0);
	if (data.m_original_template != NULL)
		doc.WriteStringTag(cxlt_class_template_inst_template_name, data.m_original_template->DefnName());

	if (data.m_defined == FALSE)
		doc.WriteBoolTag(cxlt_class_template_inst_forward_decl, TRUE);
	if (data.m_members.IsEmpty() == FALSE)
		DumpStructMemberStatistics(doc, cxlt_class_template_inst_member_stts, data);

	for (TListIter<TCppItemBase> iter(data.m_instantiation_arguments); iter; ++iter)
		doc.WriteNumTag(cxlt_class_template_inst_argument_ref, iter.CurrItem().ItemId());

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpFunctionTemplateDefn	(TXmlWriter &doc, WORD tag_id, TCppFunctionTemplateDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_func_template);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_func_template, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	int num_template_params = data.GetNumTemplateParams();
	doc.WriteNumTag(cxlt_func_template_num_params, num_template_params);
	if (num_template_params > 0)
		DumpTemplateParamInfos(doc, cxlt_func_template_params, data.m_members);

	DumpFunctionTypeProps(doc, cxlt_func_template_func_props, data);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpFunctionTemplateSpecialization(TXmlWriter &doc, WORD tag_id, TCppFunctionTemplateSpecialization &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_func_template_spec);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_func_template_spec, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_func_template_spec_template_ref, (data.m_base_template != NULL) ? data.m_base_template->ItemId() : 0);
	if (data.m_base_template != NULL)
		doc.WriteStringTag(cxlt_func_template_spec_template_name, data.m_base_template->DefnName());

	for (TListIter<TCppItemBase> iter(data.m_specialization_params); iter; ++iter)
		doc.WriteNumTag(cxlt_func_template_spec_param_ref, iter.CurrItem().ItemId());

	DumpFunctionTypeProps(doc, cxlt_func_template_spec_func_props, data);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpFunctionTemplateInstantiation(TXmlWriter &doc, WORD tag_id, TCppFunctionTemplateInstantiation &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_func_template_inst);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_func_template_inst, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_func_template_inst_template_ref, (data.m_original_template != NULL) ? data.m_original_template->ItemId() : 0);
	if (data.m_original_template != NULL)
		doc.WriteStringTag(cxlt_func_template_inst_template_name, data.m_original_template->DefnName());

	for (TListIter<TCppItemBase> iter(data.m_instantiation_arguments); iter; ++iter)
		doc.WriteNumTag(cxlt_func_template_inst_argument_ref, iter.CurrItem().ItemId());

	DumpFunctionTypeProps(doc, cxlt_func_template_inst_func_props, data);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpTemplateTypeParamDefn(TXmlWriter &doc, WORD tag_id, TCppTemplateTypeParamDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_template_type_param);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_type_param, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_type_param_index, data.m_param_index);

	if (data.m_default_value != NULL)
	{
		doc.WriteNumTag(cxlt_type_param_default_value_id, data.m_default_value->ItemId());
		doc.WriteStringTag(cxlt_type_param_default_value_name, data.m_default_value->DefnName());
	}

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpTemplateTemplateParamDefn(TXmlWriter &doc, WORD tag_id, TCppTemplateTemplateParamDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_template_template_param);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_template_param, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteNumTag(cxlt_template_param_index, data.m_param_index);
	doc.WriteNumTag(cxlt_template_param_num_params, data.m_template_params.NumItems());

	for (TListIter<TCppItemBase> iter(data.m_template_params); iter; ++iter)
		doc.WriteNumTag(cxlt_template_param_param_ref, iter.CurrItem().ItemId());

	if (data.m_default_value != NULL)
	{
		doc.WriteNumTag(cxlt_template_param_default_value_id, data.m_default_value->ItemId());
		doc.WriteStringTag(cxlt_template_param_default_value_name, data.m_default_value->DefnName());
	}

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpTemplateTypeArgument(TXmlWriter &doc, WORD tag_id, TCppTemplateTypeArgument &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_template_type_argument);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_type_argument, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.WriteNumTag(cxlt_type_argument_type_ref, (data.m_argument_value != NULL) ? data.m_argument_value->ItemId() : 0);
	if (data.m_argument_value != NULL)
		doc.WriteStringTag(cxlt_type_argument_type_name, data.m_argument_value->DefnName());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpTemplateTemplateArgument(TXmlWriter &doc, WORD tag_id, TCppTemplateTemplateArgument &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_template_template_argument);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_template_argument, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.WriteStringTag(cxlt_template_argument_type, TCppTemplateTemplateArgument::GetArgTypeEnumName(data.m_arg_type));
	switch (data.m_arg_type)
	{
		case TCppTemplateTemplateArgument::ctarg_template_defn:
				{
					doc.WriteNumTag(cxlt_template_argument_template_ref, (data.m_argument_value != NULL) ? data.m_argument_value->ItemId() : 0);
					if (data.m_argument_value != NULL)
						doc.WriteStringTag(cxlt_template_argument_template_name, data.m_argument_value->DefnName());
				}
				break;

		case TCppTemplateTemplateArgument::ctarg_template_template_param:
				{
					doc.WriteNumTag(cxlt_template_argument_param_ref, (data.m_template_param != NULL) ? data.m_template_param->ItemId() : 0);
					if (data.m_template_param != NULL)
					{
						doc.WriteStringTag(cxlt_template_argument_param_name, data.m_template_param->DefnName());
						doc.WriteNumTag(cxlt_template_argument_param_index, data.m_template_param->m_param_index);
					}
				}
				break;

		case TCppTemplateTemplateArgument::ctarg_class_templates_bucket:
				{
					if (data.m_search_res_single_defn != NULL)
					{
						// The data is present.
						doc.WriteStringTag(cxlt_template_argument_sres_name, data.m_search_res_single_defn->DefnName());
						doc.WriteNumTag(cxlt_template_argument_sres_key_id, data.m_search_res_single_defn->DefnKeyId());
					}
					else
					{
						// The situation is bogus. At least show the names of expected tags.
						doc.WriteStringTag(cxlt_template_argument_sres_name, NULL);
						doc.WriteNumTag(cxlt_template_argument_sres_key_id, 0);
					}
				}
				break;
	}

	doc.CloseContainerEx(rcnr_dct);
}

//
// ----------------------------  Unresolved objects  -----------------------------------
//

void TCppDatabaseXmlHelper::DumpUnresolvedNameInfo(TXmlWriter &doc, WORD tag_id, TCppUnresolvedNameInfo &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_unresolved_name_info);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_unresolved_name, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	doc.WriteStringTag(cxlt_unresolved_name_node_type, TCppUnresolvedNameInfo::GetNameInfoTypeEnumName(data.m_node_type));

	if (data.m_node_type == cnit_ovld_operator || data.m_node_type == cnit_ovld_op_template)
	{
		if (data.m_operator_token == 0 || data.m_operator_token == opr_none)
			doc.WriteStringTag(cxlt_unresolved_name_oper_token, L"opr_none");
		else doc.WriteStringTag(cxlt_unresolved_name_oper_token, TLexema::GetKeywordIdEnumName(data.m_operator_token));
	}
	else if (data.m_node_type == cnit_ovld_conversion || data.m_node_type == cnit_ovld_conv_template)
	{
		doc.WriteNumTag(cxlt_unresolved_name_dest_type_id, (data.m_conversion_dest_type != NULL) ? data.m_conversion_dest_type->ItemId() : 0);
		if (data.m_conversion_dest_type != NULL)
			doc.WriteStringTag(cxlt_unresolved_name_dest_type_name, data.m_conversion_dest_type->DefnName());
	}

	if (data.m_template_id_args.IsEmpty() == FALSE)
	{
		doc.WriteNumTag(cxlt_unresolved_name_num_templ_args, data.m_template_id_args.NumItems());
	}

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpUnresolvedDataType(TXmlWriter &doc, WORD tag_id, TCppUnresolvedDataType &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_unresolved_data_type);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_unres_data_type, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);
	doc.WriteNumTag(cxlt_unres_data_type_num_names, data.m_name_elements.NumItems());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpUnresolvedStructType(TXmlWriter &doc, WORD tag_id, TCppUnresolvedStructType &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_unresolved_struct_type);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_unres_struct_type, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);
	doc.WriteNumTag(cxlt_unres_struct_type_num_names, data.m_name_elements.NumItems());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpUnresolvedDataField(TXmlWriter &doc, WORD tag_id, TCppUnresolvedDataField &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_unresolved_data_field);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_unres_data_field, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);
	doc.WriteNumTag(cxlt_unres_data_field_num_names, data.m_name_elements.NumItems());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpUnresolvedCodeEntryPoint(TXmlWriter &doc, WORD tag_id, TCppUnresolvedCodeEntryPoint &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_unresolved_code_entry_point);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_unres_code_entry_point, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);
	doc.WriteNumTag(cxlt_unres_code_entry_point_num_names, data.m_name_elements.NumItems());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpUnresolvedClassTemplate(TXmlWriter &doc, WORD tag_id, TCppUnresolvedClassTemplate &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_unresolved_class_template);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_unres_class_template, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);
	doc.WriteNumTag(cxlt_unres_class_template_num_names, data.m_name_elements.NumItems());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpUnresolvedFunctionTemplate(TXmlWriter &doc, WORD tag_id, TCppUnresolvedFunctionTemplate &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_unresolved_func_template);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_unres_func_template, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);
	doc.WriteNumTag(cxlt_unres_func_template_num_names, data.m_name_elements.NumItems());

	doc.CloseContainerEx(rcnr_dct);
}

//
// ----------------------------  Statements  -----------------------------------
//

void TCppDatabaseXmlHelper::DumpEmptyStatement(TXmlWriter &doc, WORD tag_id, TCppEmptyStatement &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_empty_stmt);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_empty_stmt, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	// This item type has no data fields and it cannot have children.
	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpIfStatement(TXmlWriter &doc, WORD tag_id, TCppIfStatement &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_if_stmt);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_if_stmt, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.WriteNumTag(cxlt_if_stmt_cond_stmt_ref, (data.m_if_condition != NULL) ? data.m_if_condition->ItemId() : 0);
	if (data.m_if_condition != NULL)
		doc.WriteStringTag(cxlt_if_stmt_cond_stmt_type, data.m_if_condition->ItemTypeEnumName());

	doc.WriteNumTag(cxlt_if_stmt_then_stmt_ref, (data.m_then_statement != NULL) ? data.m_then_statement->ItemId() : 0);
	if (data.m_then_statement != NULL)
		doc.WriteStringTag(cxlt_if_stmt_then_stmt_type, data.m_then_statement->ItemTypeEnumName());

	doc.WriteNumTag(cxlt_if_stmt_else_stmt_ref, (data.m_else_statement != NULL) ? data.m_else_statement->ItemId() : 0);
	if (data.m_else_statement != NULL)
		doc.WriteStringTag(cxlt_if_stmt_else_stmt_type, data.m_else_statement->ItemTypeEnumName());

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpSwitchStatement(TXmlWriter &doc, WORD tag_id, TCppSwitchStatement &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_switch_stmt);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_switch_stmt, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.WriteNumTag(cxlt_switch_stmt_cond_stmt_ref, (data.m_condition != NULL) ? data.m_condition->ItemId() : 0);
	if (data.m_condition != NULL)
		doc.WriteStringTag(cxlt_switch_stmt_cond_stmt_type, data.m_condition->ItemTypeEnumName());

	doc.WriteNumTag(cxlt_switch_stmt_body_stmt_ref, (data.m_statement != NULL) ? data.m_statement->ItemId() : 0);
	if (data.m_statement != NULL)
		doc.WriteStringTag(cxlt_switch_stmt_body_stmt_type, data.m_statement->ItemTypeEnumName());

	doc.WriteNumTag(cxlt_switch_stmt_num_case_labs, data.NumCaseLabels());

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpForStatement(TXmlWriter &doc, WORD tag_id, TCppForStatement &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_for_stmt);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_for_stmt, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.WriteNumTag(cxlt_for_stmt_init_stmt_ref, (data.m_init_statement != NULL) ? data.m_init_statement->ItemId() : 0);
	if (data.m_init_statement != NULL)
		doc.WriteStringTag(cxlt_for_stmt_init_stmt_type, data.m_init_statement->ItemTypeEnumName());

	doc.WriteNumTag(cxlt_for_stmt_check_stmt_ref, (data.m_check_continue != NULL) ? data.m_check_continue->ItemId() : 0);
	if (data.m_check_continue != NULL)
		doc.WriteStringTag(cxlt_for_stmt_check_stmt_type, data.m_check_continue->ItemTypeEnumName());

	doc.WriteNumTag(cxlt_for_stmt_reinit_expr_ref, (data.m_reinit_expression != NULL) ? data.m_reinit_expression->ItemId() : 0);
	if (data.m_reinit_expression != NULL)
		doc.WriteStringTag(cxlt_for_stmt_reinit_expr_type, data.m_reinit_expression->ItemTypeEnumName());

	doc.WriteNumTag(cxlt_for_stmt_body_stmt_ref, (data.m_body_statement != NULL) ? data.m_body_statement->ItemId() : 0);
	if (data.m_body_statement != NULL)
		doc.WriteStringTag(cxlt_for_stmt_body_stmt_type, data.m_body_statement->ItemTypeEnumName());

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpWhileStatement(TXmlWriter &doc, WORD tag_id, TCppWhileStatement &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_while_stmt);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_while_stmt, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.WriteNumTag(cxlt_while_stmt_cond_stmt_ref, (data.m_while_condition != NULL) ? data.m_while_condition->ItemId() : 0);
	if (data.m_while_condition != NULL)
		doc.WriteStringTag(cxlt_while_stmt_cond_stmt_type, data.m_while_condition->ItemTypeEnumName());

	doc.WriteNumTag(cxlt_while_stmt_body_stmt_ref, (data.m_body_statement != NULL) ? data.m_body_statement->ItemId() : 0);
	if (data.m_body_statement != NULL)
		doc.WriteStringTag(cxlt_while_stmt_body_stmt_type, data.m_body_statement->ItemTypeEnumName());

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpDoWhileStatement(TXmlWriter &doc, WORD tag_id, TCppDoWhileStatement &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_do_while_stmt);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_do_while_stmt, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.WriteNumTag(cxlt_do_while_stmt_body_stmt_ref, (data.m_body_statement != NULL) ? data.m_body_statement->ItemId() : 0);
	if (data.m_body_statement != NULL)
		doc.WriteStringTag(cxlt_do_while_stmt_body_stmt_type, data.m_body_statement->ItemTypeEnumName());

	doc.WriteNumTag(cxlt_do_while_stmt_cond_expr_ref, (data.m_do_while_expression != NULL) ? data.m_do_while_expression->ItemId() : 0);
	if (data.m_do_while_expression != NULL)
		doc.WriteStringTag(cxlt_do_while_stmt_cond_expr_type, data.m_do_while_expression->ItemTypeEnumName());

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpGotoStatement(TXmlWriter &doc, WORD tag_id, TCppGotoStatement &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_goto_stmt);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_goto_stmt, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.WriteNumTag(cxlt_goto_stmt_label_id, (data.m_label != NULL) ? data.m_label->ItemId() : 0);
	if (data.m_label != NULL)
		doc.WriteStringTag(cxlt_goto_stmt_label_name, data.m_label->DefnName());

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpBreakStatement(TXmlWriter &doc, WORD tag_id, TCppBreakStatement &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_break_stmt);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_break_stmt, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	// This item type has no data fields and it cannot have children.
	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpContinueStatement(TXmlWriter &doc, WORD tag_id, TCppContinueStatement &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_continue_stmt);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_continue_stmt, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	// This item type has no data fields and it cannot have children.
	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpReturnStatement(TXmlWriter &doc, WORD tag_id, TCppReturnStatement &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_return_stmt);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_return_stmt, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	if (data.m_return_value != NULL)
	{
		doc.WriteNumTag(cxlt_return_stmt_expression_ref, data.m_return_value->ItemId());
		doc.WriteStringTag(cxlt_return_stmt_expression_type, data.m_return_value->ItemTypeEnumName());
	}

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpBlockDefn(TXmlWriter &doc, WORD tag_id, TCppBlockDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_block);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_block, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	// Dump the header of the object and its children.
	DumpCppItemCore(doc, cxlt_item_core, data);
	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpTryBlockDefn(TXmlWriter &doc, WORD tag_id, TCppTryBlockDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_try_block);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_try_block, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	// Dump the header of the object and its children.
	DumpCppItemCore(doc, cxlt_item_core, data);
	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpCatchBlockDefn(TXmlWriter &doc, WORD tag_id, TCppCatchBlockDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_catch_block);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_catch_block, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppItemCore(doc, cxlt_item_core, data);

	doc.WriteNumTag(cxlt_catch_block_exception_param_ref, (data.m_exception != NULL) ? data.m_exception->ItemId() : 0);
	if (data.m_exception != NULL)
	{
		doc.WriteStringTag(cxlt_catch_block_exception_param_name, data.m_exception->DefnName());
		doc.WriteNumTag(cxlt_catch_block_exception_type_ref, (data.m_exception->m_field_type != NULL) ? data.m_exception->m_field_type->ItemId() : 0);
		if (data.m_exception->m_field_type != NULL)
			doc.WriteStringTag(cxlt_catch_block_exception_type_name, data.m_exception->m_field_type->DefnName());
	}

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpLightBlockDefn(TXmlWriter &doc, WORD tag_id, TCppLightBlockDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_light_block);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_light_block, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	// Dump the header of the object and its children.
	DumpCppItemCore(doc, cxlt_item_core, data);
	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpLabelDefn(TXmlWriter &doc, WORD tag_id, TCppLabelDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_label);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_label, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	if (data.m_defined == FALSE)
		doc.WriteBoolTag(cxlt_label_defined, data.m_defined);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpCaseLabelDefn(TXmlWriter &doc, WORD tag_id, TCppCaseLabelDefn &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_case_label);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_case_label, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppDefnHeader(doc, cxlt_defn_info, data);

	if (data.m_default == TRUE)
	{
		doc.WriteBoolTag(cxlt_case_label_default, data.m_default);
	}
	else
	{
		doc.WriteBoolTag(cxlt_case_label_const_value, data.m_const_value);
		doc.WriteNumTag(cxlt_case_label_value, data.m_value);
		doc.WriteNumTag(cxlt_case_label_value_expr_ref, (data.m_value_expr != NULL) ? data.m_value_expr->ItemId() : 0);
	}

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

//
// ----------------------------  Expressions  -----------------------------------
//

void TCppDatabaseXmlHelper::DumpOperandExpr(TXmlWriter &doc, WORD tag_id, TCppOperandExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_operand_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_oprnd_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);
	doc.WriteStringTag(cxlt_oprnd_expr_type, TCppOperandExpr::GetOperandTypeEnumName(data.m_oprnd_type));

	// Only some of the operand types require dumping their specific props.
	switch (data.m_oprnd_type)
	{
		case eopr_bool_literal:
				{
					doc.WriteBoolTag(cxlt_oprnd_expr_bool_value, data.m_bool_value);
				}
				break;

		case eopr_num_literal:
				{
					doc.WriteStringTag(cxlt_oprnd_expr_num_subt, TLexema::GetLexNumberTypeEnumName(data.m_num_subt));
					doc.WriteNumTag(cxlt_oprnd_expr_num_value, data.m_num_value);
				}
				break;

		case eopr_charconst_literal:
				{
					doc.WriteStringTag(cxlt_oprnd_expr_charconst_subt, TLexema::GetLexCharConstTypeEnumName(data.m_charconst_subt));
					doc.WriteNumTag(cxlt_oprnd_expr_charconst_value, data.m_charconst_value);
				}
				break;

		case eopr_string_literal:
				{
					doc.WriteStringTag(cxlt_oprnd_expr_string_subt, TLexema::GetLexStringTypeEnumName(data.m_string_subt));
					doc.WriteNumTag(cxlt_oprnd_expr_string_length, data.m_string_value.m_len);

					wchar_t buffer[256];
					TStringPtr str_ptr(data.m_string_value);
					doc.WriteStringTag(cxlt_oprnd_expr_string_value, str_ptr.CopyWithTruncationTo(buffer, 256));
				}
				break;

		case eopr_enum_literal:
				{
					doc.WriteNumTag(cxlt_oprnd_expr_enum_member_ref, (data.m_enum_member_ref != NULL) ? data.m_enum_member_ref->ItemId() : 0);
					if (data.m_enum_member_ref != NULL)
					{
						doc.WriteStringTag(cxlt_oprnd_expr_enum_member_name, data.m_enum_member_ref->DefnName());
						doc.WriteNumTag(cxlt_oprnd_expr_enum_member_value, data.m_enum_member_ref->m_value);
					}
				}
				break;

		case eopr_data_field:
				{
					doc.WriteNumTag(cxlt_oprnd_expr_data_field_ref, (data.m_data_field_ref != NULL) ? data.m_data_field_ref->ItemId() : 0);
					if (data.m_data_field_ref != NULL)
						doc.WriteStringTag(cxlt_oprnd_expr_data_field_name, data.m_data_field_ref->DefnName());
				}
				break;

		case eopr_code_entry_point:
				{
					doc.WriteNumTag(cxlt_oprnd_expr_code_entry_point_ref, (data.m_code_entry_point_ref != NULL) ? data.m_code_entry_point_ref->ItemId() : 0);
					if (data.m_code_entry_point_ref != NULL)
						doc.WriteStringTag(cxlt_oprnd_expr_code_entry_point_name, data.m_code_entry_point_ref->DefnName());
				}
				break;
	}

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpUnaryExpr(TXmlWriter &doc, WORD tag_id, TCppUnaryExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_unary_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_unary_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	doc.WriteNumTag(cxlt_unary_expr_operand_ref, (data.m_operand != NULL) ? data.m_operand->ItemId() : 0);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpBinaryExpr(TXmlWriter &doc, WORD tag_id, TCppBinaryExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_binary_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_binary_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	doc.WriteNumTag(cxlt_binary_expr_operand1_ref, (data.m_operand1 != NULL) ? data.m_operand1->ItemId() : 0);
	doc.WriteNumTag(cxlt_binary_expr_operand2_ref, (data.m_operand2 != NULL) ? data.m_operand2->ItemId() : 0);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpMultiOpExpr(TXmlWriter &doc, WORD tag_id, TCppMultiOpExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_multiop_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_multiop_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	// Dump brief information about each operand in addition to dumping info about the children if any.
	doc.WriteNumTag(cxlt_multiop_expr_num_operands, data.m_operands.NumItems());
	doc.OpenContainer(cxlt_multiop_expr_operands);

	for (TListIter<TCppItemBase> iter(data.m_operands); iter; ++iter)
	{
		doc.OpenContainer(cxlt_multiop_expr_oprnds_array_element);
		doc.WriteNumTag(cxlt_multiop_expr_oprnd_ref, iter.CurrItem().ItemId());
		doc.CloseContainer();
	}

	doc.CloseContainer();

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpAssignmentExpr(TXmlWriter &doc, WORD tag_id, TCppAssignmentExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_assignment_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_assignment_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	doc.WriteNumTag(cxlt_assignment_expr_operand1_ref, (data.m_operand1 != NULL) ? data.m_operand1->ItemId() : 0);
	doc.WriteNumTag(cxlt_assignment_expr_operand2_ref, (data.m_operand2 != NULL) ? data.m_operand2->ItemId() : 0);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpThrowExpr(TXmlWriter &doc, WORD tag_id, TCppThrowExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_throw_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_throw_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	doc.WriteNumTag(cxlt_throw_expr_operand_ref, (data.m_operand != NULL) ? data.m_operand->ItemId() : 0);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpConditionalExpr(TXmlWriter &doc, WORD tag_id, TCppConditionalExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_conditional_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_conditional_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	doc.WriteNumTag(cxlt_conditional_expr_operand1_ref, (data.m_operand1 != NULL) ? data.m_operand1->ItemId() : 0);
	doc.WriteNumTag(cxlt_conditional_expr_operand2_ref, (data.m_operand2 != NULL) ? data.m_operand2->ItemId() : 0);
	doc.WriteNumTag(cxlt_conditional_expr_operand2_ref, (data.m_operand3 != NULL) ? data.m_operand3->ItemId() : 0);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpFieldAccessExpr(TXmlWriter &doc, WORD tag_id, TCppFieldAccessExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_field_access_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_field_access_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	doc.WriteNumTag(cxlt_field_access_expr_struct_ref, (data.m_struct_operand != NULL) ? data.m_struct_operand->ItemId() : 0);

	doc.WriteNumTag(cxlt_field_access_expr_member_ref, (data.m_struct_member_defn != NULL) ? data.m_struct_member_defn->ItemId() : 0);
	if (data.m_struct_member_defn != NULL)
		doc.WriteStringTag(cxlt_field_access_expr_member_name, data.m_struct_member_defn->DefnName());

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpFieldAccessPtrExpr(TXmlWriter &doc, WORD tag_id, TCppFieldAccessPtrExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_field_access_ptr_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_field_access_ptr_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	doc.WriteNumTag(cxlt_field_access_ptr_expr_struct_ref, (data.m_struct_operand != NULL) ? data.m_struct_operand->ItemId() : 0);
	doc.WriteNumTag(cxlt_field_access_ptr_expr_member_ref, (data.m_struct_member_ptr != NULL) ? data.m_struct_member_ptr->ItemId() : 0);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpArrayIndexExpr(TXmlWriter &doc, WORD tag_id, TCppArrayIndexExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_array_index_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_array_index_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	doc.WriteNumTag(cxlt_array_index_expr_array_object_ref, (data.m_array_operand != NULL) ? data.m_array_operand->ItemId() : 0);
	doc.WriteNumTag(cxlt_array_index_expr_num_indexes, data.m_index_expressions.NumItems());

	if (data.m_index_expressions.IsEmpty() == FALSE)
	{
		doc.OpenContainer(cxlt_array_index_expr_indexes);

		for (TListIter<TCppItemBase> iter(data.m_index_expressions); iter; ++iter)
		{
			doc.OpenContainer(cxlt_array_indexes_array_element);
			doc.WriteNumTag(cxlt_array_index_ref, iter.CurrItem().ItemId());
			doc.CloseContainer();
		}

		doc.CloseContainer();
	}

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpFunctionCallExpr(TXmlWriter &doc, WORD tag_id, TCppFunctionCallExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_function_call_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_function_call_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	doc.WriteNumTag(cxlt_function_call_expr_function_ref, (data.m_function != NULL) ? data.m_function->ItemId() : 0);
	if (data.m_function != NULL && data.m_function->ItemType() == cxx_operand_expr)
	{
		TCppOperandExpr *oprnd_expr = (TCppOperandExpr*)(data.m_function);
		if (oprnd_expr->m_oprnd_type == eopr_code_entry_point && oprnd_expr->m_code_entry_point_ref != NULL)
			doc.WriteStringTag(cxlt_function_call_expr_function_name, oprnd_expr->m_code_entry_point_ref->DefnName());
	}

	doc.WriteNumTag(cxlt_function_call_expr_num_params, data.m_parameters.NumItems());

	if (data.m_parameters.IsEmpty() == FALSE)
	{
		doc.OpenContainer(cxlt_function_call_expr_params);

		for (TListIter<TCppItemBase> iter(data.m_parameters); iter; ++iter)
		{
			doc.OpenContainer(cxlt_func_call_params_array_element);
			doc.WriteNumTag(cxlt_func_call_param_ref, iter.CurrItem().ItemId());
			doc.CloseContainer();
		}

		doc.CloseContainer();
	}

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpCastExpr(TXmlWriter &doc, WORD tag_id, TCppCastExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_cast_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_cast_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	doc.WriteNumTag(cxlt_cast_expr_dest_type_ref, (data.m_dest_type != NULL) ? data.m_dest_type->ItemId() : 0);
	if (data.m_dest_type != NULL)
		doc.WriteStringTag(cxlt_cast_expr_dest_type_name, data.m_dest_type->DefnName());

	doc.WriteNumTag(cxlt_cast_expr_operand_ref, (data.m_operand != NULL) ? data.m_operand->ItemId() : 0);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpSizeofExpr(TXmlWriter &doc, WORD tag_id, TCppSizeofExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_sizeof_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_sizeof_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	doc.WriteBoolTag(cxlt_sizeof_expr_data_operand, (data.m_expr_operand == FALSE) ? TRUE : FALSE);

	if (data.m_expr_operand == FALSE)
	{
		// Sizeof operation is applied to the name of the type.
		doc.WriteNumTag(cxlt_sizeof_expr_source_type_ref, (data.m_data_type != NULL) ? data.m_data_type->ItemId() : 0);
		if (data.m_data_type != NULL)
			doc.WriteStringTag(cxlt_sizeof_expr_source_type_name, data.m_data_type->DefnName());
	}
	else
	{
		// Sizeof operation is applied to some expression.
		doc.WriteNumTag(cxlt_sizeof_expr_operand_ref, (data.m_operand != NULL) ? data.m_operand->ItemId() : 0);
	}

	doc.WriteNumTag(cxlt_sizeof_expr_sizeof_value, data.m_sizeof_value);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpNewExpr(TXmlWriter &doc, WORD tag_id, TCppNewExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_new_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_new_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	if (data.m_global_new == TRUE)
		doc.WriteBoolTag(cxlt_new_expr_global_new, data.m_global_new);

	if (data.m_new_placement_exprs.IsEmpty() == FALSE)
	{
		doc.WriteNumTag(cxlt_new_expr_placement_num_exprs, data.m_new_placement_exprs.NumItems());
		doc.OpenContainer(cxlt_new_expr_placement_exprs);

		for (TListIter<TCppItemBase> iter(data.m_new_placement_exprs); iter; ++iter)
		{
			doc.OpenContainer(cxlt_new_expr_plc_exprs_array_element);
			doc.WriteNumTag(cxlt_new_expr_plc_expr_ref, iter.CurrItem().ItemId());
			doc.CloseContainer();
		}

		doc.CloseContainer();
	}

	doc.WriteNumTag(cxlt_new_expr_new_type_ref, (data.m_new_type != NULL) ? data.m_new_type->ItemId() : 0);
	if (data.m_new_type != NULL)
		doc.WriteStringTag(cxlt_new_expr_new_type_name, data.m_new_type->DefnName());

	if (data.m_init_param_exprs.IsEmpty() == FALSE)
	{
		doc.WriteNumTag(cxlt_new_expr_init_num_params, data.m_init_param_exprs.NumItems());
		doc.OpenContainer(cxlt_new_expr_init_params);

		for (TListIter<TCppItemBase> iter(data.m_init_param_exprs); iter; ++iter)
		{
			doc.OpenContainer(cxlt_new_expr_init_prms_array_element);
			doc.WriteNumTag(cxlt_new_expr_init_prm_ref, iter.CurrItem().ItemId());
			doc.CloseContainer();
		}

		doc.CloseContainer();
	}

	if (data.m_ctor_call_expr != NULL)
	{
		doc.WriteNumTag(cxlt_new_expr_ctor_call_ref, data.m_ctor_call_expr->ItemId());
		doc.WriteNumTag(cxlt_new_expr_ctor_func_ref, (data.m_ctor_call_expr->m_function != NULL) ? data.m_ctor_call_expr->m_function->ItemId() : 0);

		if (data.m_ctor_call_expr->m_function != NULL && data.m_ctor_call_expr->m_function->ItemType() == cxx_operand_expr)
		{
			TCppOperandExpr *func_oprnd_expr = (TCppOperandExpr*)(data.m_ctor_call_expr->m_function);
			if (func_oprnd_expr->m_oprnd_type == eopr_code_entry_point && func_oprnd_expr->m_code_entry_point_ref != NULL)
				doc.WriteStringTag(cxlt_new_expr_ctor_func_name, func_oprnd_expr->m_code_entry_point_ref->DefnName());
		}

		doc.WriteNumTag(cxlt_new_expr_ctor_num_params, data.m_ctor_call_expr->m_parameters.NumItems());
	}

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpDeleteExpr(TXmlWriter &doc, WORD tag_id, TCppDeleteExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_delete_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_delete_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	if (data.m_global_delete == TRUE)
		doc.WriteBoolTag(cxlt_delete_expr_global_delete, data.m_global_delete);
	if (data.m_array_delete == TRUE)
		doc.WriteBoolTag(cxlt_delete_expr_array_delete, data.m_array_delete);

	doc.WriteNumTag(cxlt_delete_expr_operand_ref, (data.m_operand != NULL) ? data.m_operand->ItemId() : 0);

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

void TCppDatabaseXmlHelper::DumpTypeidExpr(TXmlWriter &doc, WORD tag_id, TCppTypeidExpr &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	assert(data.ItemType() == cxx_typeid_expr);
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, cxlt_typeid_expr, xtags_CppDatabaseItems, &xpr_CppDbCnrProps, &xpr_CppDbCnrProps);

	DumpCppExprHeader(doc, cxlt_expr_info, data);

	doc.WriteBoolTag(cxlt_typeid_expr_data_operand, (data.m_expr_operand == FALSE) ? TRUE : FALSE);
	if (data.m_expr_operand == FALSE)
	{
		// Typeid operation is applied to the data type.
		doc.WriteNumTag(cxlt_typeid_expr_source_type_ref, (data.m_data_type != NULL) ? data.m_data_type->ItemId() : 0);
		if (data.m_data_type != NULL)
			doc.WriteStringTag(cxlt_typeid_expr_source_type_name, data.m_data_type->DefnName());
	}
	else
	{
		// Typeid operation is applied to an expression.
		doc.WriteNumTag(cxlt_typeid_expr_operand_ref, (data.m_operand != NULL) ? data.m_operand->ItemId() : 0);
	}

	doc.WriteBoolTag(cxlt_typeid_expr_resolution_code, (data.m_dynamic_result == FALSE) ? TRUE : FALSE);
	if (data.m_dynamic_result == FALSE)
	{
		// Typeid operation contains static result type.
		doc.WriteNumTag(cxlt_typeid_expr_result_type_ref, (data.m_static_type_struct != NULL) ? data.m_static_type_struct->ItemId() : 0);
		if (data.m_static_type_struct != NULL)
			doc.WriteStringTag(cxlt_typeid_expr_result_type_name, data.m_static_type_struct->DefnName());
	}
	else
	{
		// Typeid operation contains type resolution code.
		doc.WriteNumTag(cxlt_typeid_expr_res_code_ref, (data.m_type_resolution_code != NULL) ? data.m_type_resolution_code->ItemId() : 0);
	}

	DumpCppItemChildren(doc, data, cxdcm_mode);

	doc.CloseContainerEx(rcnr_dct);
}

//
// ----------------------------  Other  -----------------------------------
//

void	TCppDatabaseXmlHelper::DumpCppItem(TXmlWriter &doc, WORD tag_id, TCppItemBase &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	switch (data.ItemType())
	{
		case cxx_none:					DumpCppItemCore(doc, tag_id, data);													break;
		case cxx_cond_pars_param:		DumpCondParsParam(doc, tag_id, (TCppCondParsParam&)data, cxdcm_mode);				break;

		case cxx_built_in_type:				DumpBuiltInTypeDefn(doc, tag_id, (TCppBuiltInTypeDefn&)data, cxdcm_mode);				break;
		case cxx_bit_field_type:			DumpBitFieldTypeDefn(doc, tag_id, (TCppBitFieldTypeDefn&)data, cxdcm_mode);				break;
		case cxx_cv_ptr_ref_type:			DumpCvPtrRefTypeDefn(doc, tag_id, (TCppCvPtrRefTypeDefn&)data, cxdcm_mode);			break;
		case cxx_array_type:				DumpArrayTypeDefn(doc, tag_id, (TCppArrayTypeDefn&)data, cxdcm_mode);				break;
		case cxx_data_type_alias:			DumpDataTypeAliasDefn(doc, tag_id, (TCppDataTypeAliasDefn&)data, cxdcm_mode);			break;

		case cxx_enum_type:				DumpEnumTypeDefn(doc, tag_id, (TCppEnumTypeDefn&)data, cxdcm_mode);				break;
		case cxx_enum_member:			DumpEnumMemberDefn(doc, tag_id, (TCppEnumMemberDefn&)data, cxdcm_mode);			break;
		case cxx_struct_type:				DumpStructTypeDefn(doc, tag_id, (TCppStructTypeDefn&)data, cxdcm_mode);				break;
		case cxx_base_class_spec:			DumpBaseClassSpec(doc, tag_id, (TCppBaseClassSpec&)data, cxdcm_mode);				break;
		case cxx_field_align_spec:			DumpFieldAlignSpec(doc, tag_id, (TCppFieldAlignSpec&)data, cxdcm_mode);					break;
		case cxx_friend_spec:				DumpFriendSpec(doc, tag_id, (TCppFriendSpec&)data, cxdcm_mode);						break;
		case cxx_data_field:				DumpDataFieldDefn(doc, tag_id, (TCppDataFieldDefn&)data, cxdcm_mode);					break;
		case cxx_data_field_initializer:		DumpDataFieldInitializer(doc, tag_id, (TCppDataFieldInitializer&)data, cxdcm_mode);			break;

		case cxx_function_type:			DumpFunctionTypeDefn(doc, tag_id, (TCppFunctionTypeDefn&)data, cxdcm_mode);			break;
		case cxx_function_type_alias:		DumpFunctionTypeAliasDefn(doc, tag_id, (TCppFunctionTypeAliasDefn&)data, cxdcm_mode);	break;
		case cxx_code_entry_point:			DumpCodeEntryPointDefn(doc, tag_id, (TCppCodeEntryPointDefn&)data, cxdcm_mode);		break;
		case cxx_possible_exception_spec:	DumpPossibleExceptionSpec(doc, tag_id, (TCppPossibleExceptionSpec&)data, cxdcm_mode);	break;

		case cxx_namespace:				DumpNamespaceDefn(doc, tag_id, (TCppNamespaceDefn&)data, cxdcm_mode);				break;
		case cxx_global_namespace:		DumpGlobalNamespaceDefn(doc, tag_id, (TCppGlobalNamespaceDefn&)data, cxdcm_mode);	break;
		case cxx_namespace_alias:			DumpNamespaceAliasDefn(doc, tag_id, (TCppNamespaceAliasDefn&)data, cxdcm_mode);		break;
		case cxx_using_declaration:			DumpUsingDeclarationDefn(doc, tag_id, (TCppUsingDeclarationDefn&)data, cxdcm_mode);		break;
		case cxx_using_directive:			DumpUsingDirectiveDefn(doc, tag_id, (TCppUsingDirectiveDefn&)data, cxdcm_mode);			break;
		case cxx_asm_insert:				DumpAsmInsertDefn(doc, tag_id, (TCppAsmInsertDefn&)data, cxdcm_mode);					break;
		case cxx_linkage_spec:			DumpLinkageSpec(doc, tag_id, (TCppLinkageSpec&)data, cxdcm_mode);						break;

		case cxx_class_template:				DumpClassTemplateDefn(doc, tag_id, (TCppClassTemplateDefn&)data, cxdcm_mode);						break;
		case cxx_class_template_spec:			DumpClassTemplateSpecialization(doc, tag_id, (TCppClassTemplateSpecialization&)data, cxdcm_mode);		break;
		case cxx_class_template_inst:			DumpClassTemplateInstantiation(doc, tag_id, (TCppClassTemplateInstantiation&)data, cxdcm_mode);			break;
		case cxx_func_template:				DumpFunctionTemplateDefn(doc, tag_id, (TCppFunctionTemplateDefn&)data, cxdcm_mode);					break;
		case cxx_func_template_spec:			DumpFunctionTemplateSpecialization(doc, tag_id, (TCppFunctionTemplateSpecialization&)data, cxdcm_mode);	break;
		case cxx_func_template_inst:			DumpFunctionTemplateInstantiation(doc, tag_id, (TCppFunctionTemplateInstantiation&)data, cxdcm_mode);	break;

		case cxx_template_type_param:			DumpTemplateTypeParamDefn(doc, tag_id, (TCppTemplateTypeParamDefn&)data, cxdcm_mode);			break;
		case cxx_template_template_param:		DumpTemplateTemplateParamDefn(doc, tag_id, (TCppTemplateTemplateParamDefn&)data, cxdcm_mode);		break;
		case cxx_template_type_argument:		DumpTemplateTypeArgument(doc, tag_id, (TCppTemplateTypeArgument&)data, cxdcm_mode);				break;
		case cxx_template_template_argument:	DumpTemplateTemplateArgument(doc, tag_id, (TCppTemplateTemplateArgument&)data, cxdcm_mode);		break;

		case cxx_unresolved_name_info:		DumpUnresolvedNameInfo(doc, tag_id, (TCppUnresolvedNameInfo&)data, cxdcm_mode);					break;
		case cxx_unresolved_data_type:			DumpUnresolvedDataType(doc, tag_id, (TCppUnresolvedDataType&)data, cxdcm_mode);					break;
		case cxx_unresolved_struct_type:		DumpUnresolvedStructType(doc, tag_id, (TCppUnresolvedStructType&)data, cxdcm_mode);					break;
		case cxx_unresolved_data_field:			DumpUnresolvedDataField(doc, tag_id, (TCppUnresolvedDataField&)data, cxdcm_mode);					break;
		case cxx_unresolved_code_entry_point:	DumpUnresolvedCodeEntryPoint(doc, tag_id, (TCppUnresolvedCodeEntryPoint&)data, cxdcm_mode);			break;
		case cxx_unresolved_class_template:		DumpUnresolvedClassTemplate(doc, tag_id, (TCppUnresolvedClassTemplate&)data, cxdcm_mode);			break;
		case cxx_unresolved_func_template:		DumpUnresolvedFunctionTemplate(doc, tag_id, (TCppUnresolvedFunctionTemplate&)data, cxdcm_mode);		break;

		case cxx_empty_stmt:				DumpEmptyStatement(doc, tag_id, (TCppEmptyStatement&)data, cxdcm_mode);				break;
		case cxx_if_stmt:					DumpIfStatement(doc, tag_id, (TCppIfStatement&)data, cxdcm_mode);						break;
		case cxx_switch_stmt:				DumpSwitchStatement(doc, tag_id, (TCppSwitchStatement&)data, cxdcm_mode);				break;
		case cxx_for_stmt:				DumpForStatement(doc, tag_id, (TCppForStatement&)data, cxdcm_mode);					break;
		case cxx_while_stmt:				DumpWhileStatement(doc, tag_id, (TCppWhileStatement&)data, cxdcm_mode);				break;
		case cxx_do_while_stmt:			DumpDoWhileStatement(doc, tag_id, (TCppDoWhileStatement&)data, cxdcm_mode);			break;

		case cxx_goto_stmt:				DumpGotoStatement(doc, tag_id, (TCppGotoStatement&)data, cxdcm_mode);					break;
		case cxx_break_stmt:				DumpBreakStatement(doc, tag_id, (TCppBreakStatement&)data, cxdcm_mode);				break;
		case cxx_continue_stmt:			DumpContinueStatement(doc, tag_id, (TCppContinueStatement&)data, cxdcm_mode);			break;
		case cxx_return_stmt:				DumpReturnStatement(doc, tag_id, (TCppReturnStatement&)data, cxdcm_mode);				break;

		case cxx_block:					DumpBlockDefn(doc, tag_id, (TCppBlockDefn&)data, cxdcm_mode);							break;
		case cxx_try_block:				DumpTryBlockDefn(doc, tag_id, (TCppTryBlockDefn&)data, cxdcm_mode);					break;
		case cxx_catch_block:				DumpCatchBlockDefn(doc, tag_id, (TCppCatchBlockDefn&)data, cxdcm_mode);				break;
		case cxx_light_block:				DumpLightBlockDefn(doc, tag_id, (TCppLightBlockDefn&)data, cxdcm_mode);					break;
		case cxx_label:					DumpLabelDefn(doc, tag_id, (TCppLabelDefn&)data, cxdcm_mode);							break;
		case cxx_case_label:				DumpCaseLabelDefn(doc, tag_id, (TCppCaseLabelDefn&)data, cxdcm_mode);					break;

		case cxx_operand_expr:			DumpOperandExpr(doc, tag_id, (TCppOperandExpr&)data, cxdcm_mode);					break;
		case cxx_unary_expr:				DumpUnaryExpr(doc, tag_id, (TCppUnaryExpr&)data, cxdcm_mode);						break;
		case cxx_binary_expr:				DumpBinaryExpr(doc, tag_id, (TCppBinaryExpr&)data, cxdcm_mode);						break;
		case cxx_multiop_expr:			DumpMultiOpExpr(doc, tag_id, (TCppMultiOpExpr&)data, cxdcm_mode);						break;
		case cxx_assignment_expr:			DumpAssignmentExpr(doc, tag_id, (TCppAssignmentExpr&)data, cxdcm_mode);				break;

		case cxx_throw_expr:				DumpThrowExpr(doc, tag_id, (TCppThrowExpr&)data, cxdcm_mode);						break;
		case cxx_conditional_expr:			DumpConditionalExpr(doc, tag_id, (TCppConditionalExpr&)data, cxdcm_mode);				break;
		case cxx_field_access_expr:		DumpFieldAccessExpr(doc, tag_id, (TCppFieldAccessExpr&)data, cxdcm_mode);				break;
		case cxx_field_access_ptr_expr:		DumpFieldAccessPtrExpr(doc, tag_id, (TCppFieldAccessPtrExpr&)data, cxdcm_mode);			break;
		case cxx_array_index_expr:		DumpArrayIndexExpr(doc, tag_id, (TCppArrayIndexExpr&)data, cxdcm_mode);				break;
		case cxx_function_call_expr:		DumpFunctionCallExpr(doc, tag_id, (TCppFunctionCallExpr&)data, cxdcm_mode);				break;

		case cxx_cast_expr:				DumpCastExpr(doc, tag_id, (TCppCastExpr&)data, cxdcm_mode);							break;
		case cxx_sizeof_expr:				DumpSizeofExpr(doc, tag_id, (TCppSizeofExpr&)data, cxdcm_mode);						break;
		case cxx_new_expr:				DumpNewExpr(doc, tag_id, (TCppNewExpr&)data, cxdcm_mode);							break;
		case cxx_delete_expr:				DumpDeleteExpr(doc, tag_id, (TCppDeleteExpr&)data, cxdcm_mode);						break;
		case cxx_typeid_expr:				DumpTypeidExpr(doc, tag_id, (TCppTypeidExpr&)data, cxdcm_mode);						break;

		default:
			assert(FALSE);
			break;
	}
}

void TCppDatabaseXmlHelper::DumpCppItemChildren(TXmlWriter &doc, TCppItemBase &data, TCppXmlDumpChildrenMode cxdcm_mode)
{
	if (cxdcm_mode == cxdcm_num_children)
	{
		doc.WriteNumTag(cxlt_item_num_children, data.NumDirectChildren());
	}
	else if (cxdcm_mode == cxdcm_dump_one_layer)
	{
		doc.OpenContainer(cxlt_item_children);

		for (TCppDirectChildrenIter iter(data); iter; ++iter)
			DumpCppItem(doc, 0, iter.CurrItem(), cxdcm_none);

		doc.CloseContainer();
	}
	else if (cxdcm_mode == cxdcm_dump_subtree)
	{
		doc.OpenContainer(cxlt_item_children);

		for (TCppDirectChildrenIter iter(data); iter; ++iter)
			DumpCppItem(doc, 0, iter.CurrItem(), cxdcm_dump_subtree);

		doc.CloseContainer();
	}
}


