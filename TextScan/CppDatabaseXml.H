//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   In memory storage for C/C++ objects.
//

#ifndef	TextScan_CppDatabaseXml_H
#define	TextScan_CppDatabaseXml_H

#ifndef   Common_XmlWriter_H
#include  "Xml/XmlWriter.H"
#endif
#ifndef   TextScan_CppDatabase_H
#include  "TextScan/CppDatabase.H"
#endif

enum TCppItemXmlTags
{
	cxlt_none,
	cxlt_item_num_children,
	cxlt_item_children,

	// TCppItemBase
	cxlt_item_core,
	cxlt_item_core_id,
	cxlt_item_core_parent_id,
	cxlt_item_core_item_type,
	cxlt_item_core_num_pars_evts,

	// TCppDefnBase
	cxlt_defn_info,
	cxlt_defn_info_problem,					// Optional.
	cxlt_defn_info_name,
	cxlt_defn_info_key_id,
	cxlt_defn_info_elab_type,				// Optional.

	// TCppExpressionBase
	cxlt_expr_info,
	cxlt_expr_info_operation,
	cxlt_expr_info_lvalue_expr,
	cxlt_expr_info_const_expr,
	cxlt_expr_info_result_type_id,
	cxlt_expr_info_result_type_name,

	// TCppCvPtrRefSpec
	cxlt_cvpr_spec_const,
	cxlt_cvpr_spec_volatile,
	cxlt_cvpr_spec_pointer,
	cxlt_cvpr_spec_reference,
	cxlt_cvpr_spec_struct_id,				// Optional.
	cxlt_cvpr_spec_struct_name,				// Optional.

	// Array of TCppCvPtrRefSpec's.
	cxlt_cvpr_specs_array_element,

	// TCppArrayDimensionSpec
	cxlt_array_dim_num_elements,
	cxlt_array_dim_num_elems_expr_const,
	cxlt_array_dim_num_elems_expr_ref,

	// Array of TCppArrayDimensionSpec's.
	cxlt_array_dims_array_element,

	// Namespace inner elements statistics info.
	cxlt_namespace_stts_enum_types,
	cxlt_namespace_stts_struct_types,
	cxlt_namespace_stts_union_types,
	cxlt_namespace_stts_class_types,
	cxlt_namespace_stts_structured_types,
	cxlt_namespace_stts_data_fields,
	cxlt_namespace_stts_code_entry_points,
	cxlt_namespace_stts_other_objects,
	cxlt_namespace_stts_total_objects,

	// Struct members statistics info.
	cxlt_struct_type_stts_inst_data_fields,
	cxlt_struct_type_stts_static_data_fields,
	cxlt_struct_type_stts_code_entry_points,
	cxlt_struct_type_stts_other_objects,
	cxlt_struct_type_stts_total_objects,

	// Brief function parameter info.
	cxlt_func_param_ref,
	cxlt_func_param_name,
	cxlt_func_param_type_ref,
	cxlt_func_param_type_name,

	// Array of brief function parameter infos.
	cxlt_func_params_array_element,

	// Brief throw spec info.
	cxlt_func_throws_ref,
	cxlt_func_throws_type_ref,
	cxlt_func_throws_type_name,

	// Array of brief throw spec infos.
	cxlt_func_throws_array_element,

	// Function type props (There is no explicit structure for this XML container.)
	cxlt_func_type_props_visib_pars_sess,
	cxlt_func_type_props_linkage_spec_ref,
	cxlt_func_type_props_linkage_spec_name,
	cxlt_func_type_props_access_level,
	cxlt_func_type_props_func_attrs,
	cxlt_func_type_props_func_attrs_as_text,
	cxlt_func_type_props_spec_type,
	cxlt_func_type_props_operator_type,
	cxlt_func_type_props_ret_val_type_ref,
	cxlt_func_type_props_ret_val_type_name,
	cxlt_func_type_props_num_params,
	cxlt_func_type_props_params,
	cxlt_func_type_props_num_throws,
	cxlt_func_type_props_throws,

	// Brief template param info.
	cxlt_tplt_param_info_ref,
	cxlt_tplt_param_info_name,
	cxlt_tplt_param_info_type,
	cxlt_tplt_param_info_data_type_ref,
	cxlt_tplt_param_info_data_type_name,
	cxlt_tplt_param_info_tplt_num_params,

	// Array of brief template param infos.
	cxlt_tplt_param_infos_array_element,

	//
	//  ------ Misc Analysis ------
	//

	// TCppCondParsParam
	cxlt_cond_pars_param,

	//
	//  ------ Definitions-1 ------
	//

	cxlt_built_in_type,
	cxlt_built_in_type_real_type,
	cxlt_built_in_type_unsigned,
	cxlt_built_in_type_byte_len,

	cxlt_bit_field_type,
	cxlt_bit_field_type_base_type_id,
	cxlt_bit_field_type_base_type_name,
	cxlt_bit_field_type_const_expr,
	cxlt_bit_field_type_bit_size,
	cxlt_bit_field_type_expr_ref,

	cxlt_cvpr_type,
	cxlt_cvpr_type_base_type_id,
	cxlt_cvpr_type_base_type_name,
	cxlt_cvpr_type_num_layers,
	cxlt_cvpr_type_layers,

	cxlt_array_type,
	cxlt_array_type_base_type_id,
	cxlt_array_type_base_type_name,
	cxlt_array_type_num_dimensions,
	cxlt_array_type_dimensions,

	cxlt_data_type_alias,
	cxlt_data_type_alias_base_type_id,
	cxlt_data_type_alias_base_type_name,

	cxlt_enum_type,
	cxlt_enum_type_forward_decl,
	cxlt_enum_type_base_type_ref,
	cxlt_enum_type_base_type_name,
	cxlt_enum_type_num_members,

	cxlt_enum_member,
	cxlt_enum_member_explicit_value,
	cxlt_enum_member_const_value,
	cxlt_enum_member_value,
	cxlt_enum_member_value_subt,
	cxlt_enum_member_value_expr_ref,

	cxlt_struct_type,
	cxlt_struct_type_forward_decl,
	cxlt_struct_type_member_statistics,

	cxlt_base_class_spec,
	cxlt_base_class_spec_virtual,
	cxlt_base_class_spec_access_level,
	cxlt_base_class_spec_base_class_ref,
	cxlt_base_class_spec_base_class_name,

	cxlt_field_align_spec,
	cxlt_field_align_spec_const_expr,
	cxlt_field_align_spec_align_value,
	cxlt_field_align_spec_expr_ref,

	cxlt_friend_spec,
	cxlt_friend_spec_friend_ref,
	cxlt_friend_spec_friend_name,

	cxlt_data_field,
	cxlt_data_field_visib_pars_sess,
	cxlt_data_field_linkage_spec_ref,
	cxlt_data_field_linkage_spec_name,
	cxlt_data_field_access_level,
	cxlt_data_field_storage_class,
	cxlt_data_field_param_index,
	cxlt_data_field_mutable,
	cxlt_data_field_data_type_ref,
	cxlt_data_field_data_type_name,
	cxlt_data_field_init_ref,

	cxlt_data_field_initializer,
	cxlt_data_field_initializer_init_type,

	//
	//  ------ Definitions-2 ------
	//

	cxlt_function_type,
	cxlt_function_type_func_props,

	cxlt_function_type_alias,
	cxlt_function_type_alias_base_type_ref,
	cxlt_function_type_alias_base_type_name,

	cxlt_code_entry_point,
	cxlt_code_entry_point_func_props,
	cxlt_code_entry_point_func_body_ref,

	cxlt_exception_spec,
	cxlt_exception_spec_data_type_ref,
	cxlt_exception_spec_data_type_name,

	cxlt_namespace,
	cxlt_namespace_elements_statistics,

	cxlt_global_namespace,
	cxlt_global_namespace_elements_stts,

	cxlt_namespace_alias,
	cxlt_namespace_alias_namespace_ref,
	cxlt_namespace_alias_namespace_name,

	cxlt_using_declaration,
	cxlt_using_declaration_defn_ref,
	cxlt_using_declaration_defn_name,

	cxlt_using_directive,
	cxlt_using_directive_namespace_ref,
	cxlt_using_directive_namespace_name,

	cxlt_asm_insert,
	cxlt_asm_insert_length,
	cxlt_asm_insert_value,

	cxlt_linkage_spec,
	cxlt_linkage_spec_name,
	cxlt_linkage_spec_key_id,
	cxlt_linkage_spec_cnt_use,

	//
	//  ------ Templates ------
	//

	cxlt_class_template,
	cxlt_class_template_forward_decl,
	cxlt_class_template_member_stts,
	cxlt_class_template_num_params,
	cxlt_class_template_params,

	cxlt_class_template_spec,
	cxlt_class_template_spec_template_ref,
	cxlt_class_template_spec_template_name,
	cxlt_class_template_spec_forward_decl,
	cxlt_class_template_spec_member_stts,
	cxlt_class_template_spec_param_ref,

	cxlt_class_template_inst,
	cxlt_class_template_inst_template_ref,
	cxlt_class_template_inst_template_name,
	cxlt_class_template_inst_forward_decl,
	cxlt_class_template_inst_member_stts,
	cxlt_class_template_inst_argument_ref,

	cxlt_func_template,
	cxlt_func_template_num_params,
	cxlt_func_template_params,
	cxlt_func_template_func_props,

	cxlt_func_template_spec,
	cxlt_func_template_spec_template_ref,
	cxlt_func_template_spec_template_name,
	cxlt_func_template_spec_param_ref,
	cxlt_func_template_spec_func_props,

	cxlt_func_template_inst,
	cxlt_func_template_inst_template_ref,
	cxlt_func_template_inst_template_name,
	cxlt_func_template_inst_argument_ref,
	cxlt_func_template_inst_func_props,

	cxlt_type_param,
	cxlt_type_param_index,
	cxlt_type_param_default_value_id,
	cxlt_type_param_default_value_name,

	cxlt_template_param,
	cxlt_template_param_index,
	cxlt_template_param_num_params,
	cxlt_template_param_param_ref,
	cxlt_template_param_default_value_id,
	cxlt_template_param_default_value_name,

	cxlt_type_argument,
	cxlt_type_argument_type_ref,
	cxlt_type_argument_type_name,

	cxlt_template_argument,
	cxlt_template_argument_type,
	cxlt_template_argument_template_ref,
	cxlt_template_argument_template_name,
	cxlt_template_argument_param_ref,
	cxlt_template_argument_param_name,
	cxlt_template_argument_param_index,
	cxlt_template_argument_sres_name,
	cxlt_template_argument_sres_key_id,

	//
	//  ------ Unresolved objects ------
	//

	cxlt_unresolved_name,
	cxlt_unresolved_name_node_type,
	cxlt_unresolved_name_oper_token,
	cxlt_unresolved_name_dest_type_id,
	cxlt_unresolved_name_dest_type_name,
	cxlt_unresolved_name_num_templ_args,
	cxlt_unresolved_name_templ_args,

	cxlt_unres_data_type,
	cxlt_unres_data_type_num_names,

	cxlt_unres_struct_type,
	cxlt_unres_struct_type_num_names,

	cxlt_unres_data_field,
	cxlt_unres_data_field_num_names,

	cxlt_unres_code_entry_point,
	cxlt_unres_code_entry_point_num_names,

	cxlt_unres_class_template,
	cxlt_unres_class_template_num_names,

	cxlt_unres_func_template,
	cxlt_unres_func_template_num_names,

	//
	//  ------ Statements ------
	//

	cxlt_empty_stmt,

	cxlt_if_stmt,
	cxlt_if_stmt_cond_stmt_ref,
	cxlt_if_stmt_cond_stmt_type,
	cxlt_if_stmt_then_stmt_ref,
	cxlt_if_stmt_then_stmt_type,
	cxlt_if_stmt_else_stmt_ref,
	cxlt_if_stmt_else_stmt_type,

	cxlt_switch_stmt,
	cxlt_switch_stmt_cond_stmt_ref,
	cxlt_switch_stmt_cond_stmt_type,
	cxlt_switch_stmt_body_stmt_ref,
	cxlt_switch_stmt_body_stmt_type,
	cxlt_switch_stmt_num_case_labs,

	cxlt_for_stmt,
	cxlt_for_stmt_init_stmt_ref,
	cxlt_for_stmt_init_stmt_type,
	cxlt_for_stmt_check_stmt_ref,
	cxlt_for_stmt_check_stmt_type,
	cxlt_for_stmt_reinit_expr_ref,
	cxlt_for_stmt_reinit_expr_type,
	cxlt_for_stmt_body_stmt_ref,
	cxlt_for_stmt_body_stmt_type,

	cxlt_while_stmt,
	cxlt_while_stmt_cond_stmt_ref,
	cxlt_while_stmt_cond_stmt_type,
	cxlt_while_stmt_body_stmt_ref,
	cxlt_while_stmt_body_stmt_type,

	cxlt_do_while_stmt,
	cxlt_do_while_stmt_body_stmt_ref,
	cxlt_do_while_stmt_body_stmt_type,
	cxlt_do_while_stmt_cond_expr_ref,
	cxlt_do_while_stmt_cond_expr_type,

	cxlt_goto_stmt,
	cxlt_goto_stmt_label_id,
	cxlt_goto_stmt_label_name,

	cxlt_break_stmt,
	cxlt_continue_stmt,

	cxlt_return_stmt,
	cxlt_return_stmt_expression_ref,
	cxlt_return_stmt_expression_type,

	cxlt_block,
	cxlt_try_block,

	cxlt_catch_block,
	cxlt_catch_block_exception_param_ref,
	cxlt_catch_block_exception_param_name,
	cxlt_catch_block_exception_type_ref,
	cxlt_catch_block_exception_type_name,

	cxlt_light_block,

	cxlt_label,
	cxlt_label_defined,

	cxlt_case_label,
	cxlt_case_label_default,
	cxlt_case_label_const_value,
	cxlt_case_label_value,
	cxlt_case_label_value_expr_ref,


	//
	//  ------ Expressions ------
	//

	cxlt_oprnd_expr,
	cxlt_oprnd_expr_type,
	cxlt_oprnd_expr_bool_value,
	cxlt_oprnd_expr_num_subt,
	cxlt_oprnd_expr_num_value,
	cxlt_oprnd_expr_charconst_subt,
	cxlt_oprnd_expr_charconst_value,
	cxlt_oprnd_expr_string_subt,
	cxlt_oprnd_expr_string_length,
	cxlt_oprnd_expr_string_value,
	cxlt_oprnd_expr_enum_member_ref,
	cxlt_oprnd_expr_enum_member_name,
	cxlt_oprnd_expr_enum_member_value,
	cxlt_oprnd_expr_data_field_ref,
	cxlt_oprnd_expr_data_field_name,
	cxlt_oprnd_expr_code_entry_point_ref,
	cxlt_oprnd_expr_code_entry_point_name,
	cxlt_oprnd_expr_template_ref,
	cxlt_oprnd_expr_template_name,

	cxlt_unary_expr,
	cxlt_unary_expr_operand_ref,

	cxlt_binary_expr,
	cxlt_binary_expr_operand1_ref,
	cxlt_binary_expr_operand2_ref,

	cxlt_multiop_expr_oprnd_ref,
	cxlt_multiop_expr_oprnds_array_element,

	cxlt_multiop_expr,
	cxlt_multiop_expr_num_operands,
	cxlt_multiop_expr_operands,

	cxlt_assignment_expr,
	cxlt_assignment_expr_operand1_ref,
	cxlt_assignment_expr_operand2_ref,

	cxlt_throw_expr,
	cxlt_throw_expr_operand_ref,

	cxlt_conditional_expr,
	cxlt_conditional_expr_operand1_ref,
	cxlt_conditional_expr_operand2_ref,
	cxlt_conditional_expr_operand3_ref,

	cxlt_field_access_expr,
	cxlt_field_access_expr_struct_ref,
	cxlt_field_access_expr_member_ref,
	cxlt_field_access_expr_member_name,

	cxlt_field_access_ptr_expr,
	cxlt_field_access_ptr_expr_struct_ref,
	cxlt_field_access_ptr_expr_member_ref,

	cxlt_array_index_ref,
	cxlt_array_indexes_array_element,

	cxlt_array_index_expr,
	cxlt_array_index_expr_array_object_ref,
	cxlt_array_index_expr_num_indexes,
	cxlt_array_index_expr_indexes,

	cxlt_func_call_param_ref,
	cxlt_func_call_params_array_element,

	cxlt_function_call_expr,
	cxlt_function_call_expr_function_ref,
	cxlt_function_call_expr_function_name,
	cxlt_function_call_expr_num_params,
	cxlt_function_call_expr_params,

	cxlt_cast_expr,
	cxlt_cast_expr_dest_type_ref,
	cxlt_cast_expr_dest_type_name,
	cxlt_cast_expr_operand_ref,

	cxlt_sizeof_expr,
	cxlt_sizeof_expr_data_operand,
	cxlt_sizeof_expr_source_type_ref,
	cxlt_sizeof_expr_source_type_name,
	cxlt_sizeof_expr_operand_ref,
	cxlt_sizeof_expr_sizeof_value,

	cxlt_new_expr_plc_expr_ref,
	cxlt_new_expr_plc_exprs_array_element,
	cxlt_new_expr_init_prm_ref,
	cxlt_new_expr_init_prms_array_element,

	cxlt_new_expr,
	cxlt_new_expr_global_new,
	cxlt_new_expr_placement_num_exprs,
	cxlt_new_expr_placement_exprs,
	cxlt_new_expr_new_type_ref,
	cxlt_new_expr_new_type_name,
	cxlt_new_expr_init_num_params,
	cxlt_new_expr_init_params,
	cxlt_new_expr_ctor_call_ref,
	cxlt_new_expr_ctor_func_ref,
	cxlt_new_expr_ctor_func_name,
	cxlt_new_expr_ctor_num_params,

	cxlt_delete_expr,
	cxlt_delete_expr_global_delete,
	cxlt_delete_expr_array_delete,
	cxlt_delete_expr_operand_ref,

	cxlt_typeid_expr,
	cxlt_typeid_expr_data_operand,
	cxlt_typeid_expr_source_type_ref,
	cxlt_typeid_expr_source_type_name,
	cxlt_typeid_expr_operand_ref,
	cxlt_typeid_expr_resolution_code,
	cxlt_typeid_expr_result_type_ref,
	cxlt_typeid_expr_result_type_name,
	cxlt_typeid_expr_res_code_ref,
};

enum TCppXmlDumpChildrenMode
{
	cxdcm_none,						// Do not dump anything at all.
	cxdcm_num_children,
	cxdcm_dump_one_layer,
	cxdcm_dump_subtree,
};

class TCppDatabaseXmlHelper
{
public:

	//
	//  All functions of this class are static.
	//  There are 2 variants of calls that differ by the tag_id value:
	//
	//       tag_id
	//   ----------------------------------
	//		0		This type of call should be used when the structure is saved as a separate document,
	//				i.e. when it is not part of a bigger document. Method will use the default container name.
	//       value		This call can be used ONLY when the structure is saved into the already opened XML nesting.
	//				Opened nesting means that the tags dict is existing and this dict should be searched for entry
	//				with the passed id. Container name will be taken from this entry. This allows saving cpp items
	//				using the context specific names.
	//

	static void	DumpCppItemCore				(TXmlWriter &doc, WORD tag_id, TCppItemBase &data);
	static void	DumpCppDefnHeader			(TXmlWriter &doc, WORD tag_id, TCppDefnBase &data);
	static void	DumpCppExprHeader			(TXmlWriter &doc, WORD tag_id, TCppExpressionBase &data);

	static void	DumpNamespaceStatistics		(TXmlWriter &doc, WORD tag_id, TCppDefnBase &data);
	static void	DumpStructMemberStatistics	(TXmlWriter &doc, WORD tag_id, TCppStructTypeDefn &data);
	static void	DumpFunctionTypeProps		(TXmlWriter &doc, WORD tag_id, TCppFunctionTypeDefn &data);
	static void	DumpTemplateParamInfos		(TXmlWriter &doc, WORD tag_id, TList &data);

	static void	DumpCondParsParam			(TXmlWriter &doc, WORD tag_id, TCppCondParsParam &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpBuiltInTypeDefn			(TXmlWriter &doc, WORD tag_id, TCppBuiltInTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpBitFieldTypeDefn			(TXmlWriter &doc, WORD tag_id, TCppBitFieldTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpCvPtrRefTypeDefn			(TXmlWriter &doc, WORD tag_id, TCppCvPtrRefTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpArrayTypeDefn				(TXmlWriter &doc, WORD tag_id, TCppArrayTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpDataTypeAliasDefn			(TXmlWriter &doc, WORD tag_id, TCppDataTypeAliasDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpEnumTypeDefn				(TXmlWriter &doc, WORD tag_id, TCppEnumTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpEnumMemberDefn			(TXmlWriter &doc, WORD tag_id, TCppEnumMemberDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpStructTypeDefn			(TXmlWriter &doc, WORD tag_id, TCppStructTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpBaseClassSpec				(TXmlWriter &doc, WORD tag_id, TCppBaseClassSpec &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpFieldAlignSpec				(TXmlWriter &doc, WORD tag_id, TCppFieldAlignSpec &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpFriendSpec					(TXmlWriter &doc, WORD tag_id, TCppFriendSpec &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpDataFieldDefn				(TXmlWriter &doc, WORD tag_id, TCppDataFieldDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpDataFieldInitializer			(TXmlWriter &doc, WORD tag_id, TCppDataFieldInitializer &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpFunctionTypeDefn			(TXmlWriter &doc, WORD tag_id, TCppFunctionTypeDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpFunctionTypeAliasDefn	(TXmlWriter &doc, WORD tag_id, TCppFunctionTypeAliasDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpCodeEntryPointDefn		(TXmlWriter &doc, WORD tag_id, TCppCodeEntryPointDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpPossibleExceptionSpec	(TXmlWriter &doc, WORD tag_id, TCppPossibleExceptionSpec &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpNamespaceDefn			(TXmlWriter &doc, WORD tag_id, TCppNamespaceDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpGlobalNamespaceDefn		(TXmlWriter &doc, WORD tag_id, TCppGlobalNamespaceDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpNamespaceAliasDefn		(TXmlWriter &doc, WORD tag_id, TCppNamespaceAliasDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpUsingDeclarationDefn		(TXmlWriter &doc, WORD tag_id, TCppUsingDeclarationDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpUsingDirectiveDefn		(TXmlWriter &doc, WORD tag_id, TCppUsingDirectiveDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpAsmInsertDefn				(TXmlWriter &doc, WORD tag_id, TCppAsmInsertDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpLinkageSpec				(TXmlWriter &doc, WORD tag_id, TCppLinkageSpec &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpClassTemplateDefn				(TXmlWriter &doc, WORD tag_id, TCppClassTemplateDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpClassTemplateSpecialization		(TXmlWriter &doc, WORD tag_id, TCppClassTemplateSpecialization &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpClassTemplateInstantiation		(TXmlWriter &doc, WORD tag_id, TCppClassTemplateInstantiation &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpFunctionTemplateDefn				(TXmlWriter &doc, WORD tag_id, TCppFunctionTemplateDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpFunctionTemplateSpecialization	(TXmlWriter &doc, WORD tag_id, TCppFunctionTemplateSpecialization &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpFunctionTemplateInstantiation	(TXmlWriter &doc, WORD tag_id, TCppFunctionTemplateInstantiation &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpTemplateTypeParamDefn			(TXmlWriter &doc, WORD tag_id, TCppTemplateTypeParamDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpTemplateTemplateParamDefn		(TXmlWriter &doc, WORD tag_id, TCppTemplateTemplateParamDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpTemplateTypeArgument			(TXmlWriter &doc, WORD tag_id, TCppTemplateTypeArgument &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpTemplateTemplateArgument		(TXmlWriter &doc, WORD tag_id, TCppTemplateTemplateArgument &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpUnresolvedNameInfo				(TXmlWriter &doc, WORD tag_id, TCppUnresolvedNameInfo &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpUnresolvedDataType				(TXmlWriter &doc, WORD tag_id, TCppUnresolvedDataType &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpUnresolvedStructType				(TXmlWriter &doc, WORD tag_id, TCppUnresolvedStructType &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpUnresolvedDataField				(TXmlWriter &doc, WORD tag_id, TCppUnresolvedDataField &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpUnresolvedCodeEntryPoint		(TXmlWriter &doc, WORD tag_id, TCppUnresolvedCodeEntryPoint &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpUnresolvedClassTemplate			(TXmlWriter &doc, WORD tag_id, TCppUnresolvedClassTemplate &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpUnresolvedFunctionTemplate		(TXmlWriter &doc, WORD tag_id, TCppUnresolvedFunctionTemplate &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpEmptyStatement			(TXmlWriter &doc, WORD tag_id, TCppEmptyStatement &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpIfStatement					(TXmlWriter &doc, WORD tag_id, TCppIfStatement &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpSwitchStatement			(TXmlWriter &doc, WORD tag_id, TCppSwitchStatement &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpForStatement				(TXmlWriter &doc, WORD tag_id, TCppForStatement &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpWhileStatement			(TXmlWriter &doc, WORD tag_id, TCppWhileStatement &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpDoWhileStatement			(TXmlWriter &doc, WORD tag_id, TCppDoWhileStatement &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpGotoStatement				(TXmlWriter &doc, WORD tag_id, TCppGotoStatement &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpBreakStatement			(TXmlWriter &doc, WORD tag_id, TCppBreakStatement &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpContinueStatement		(TXmlWriter &doc, WORD tag_id, TCppContinueStatement &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpReturnStatement			(TXmlWriter &doc, WORD tag_id, TCppReturnStatement &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpBlockDefn					(TXmlWriter &doc, WORD tag_id, TCppBlockDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpTryBlockDefn				(TXmlWriter &doc, WORD tag_id, TCppTryBlockDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpCatchBlockDefn			(TXmlWriter &doc, WORD tag_id, TCppCatchBlockDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpLightBlockDefn				(TXmlWriter &doc, WORD tag_id, TCppLightBlockDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpLabelDefn					(TXmlWriter &doc, WORD tag_id, TCppLabelDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpCaseLabelDefn				(TXmlWriter &doc, WORD tag_id, TCppCaseLabelDefn &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpOperandExpr				(TXmlWriter &doc, WORD tag_id, TCppOperandExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpUnaryExpr					(TXmlWriter &doc, WORD tag_id, TCppUnaryExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpBinaryExpr					(TXmlWriter &doc, WORD tag_id, TCppBinaryExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpMultiOpExpr				(TXmlWriter &doc, WORD tag_id, TCppMultiOpExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpAssignmentExpr			(TXmlWriter &doc, WORD tag_id, TCppAssignmentExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpThrowExpr					(TXmlWriter &doc, WORD tag_id, TCppThrowExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpConditionalExpr			(TXmlWriter &doc, WORD tag_id, TCppConditionalExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpFieldAccessExpr			(TXmlWriter &doc, WORD tag_id, TCppFieldAccessExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpFieldAccessPtrExpr		(TXmlWriter &doc, WORD tag_id, TCppFieldAccessPtrExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpArrayIndexExpr			(TXmlWriter &doc, WORD tag_id, TCppArrayIndexExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpFunctionCallExpr			(TXmlWriter &doc, WORD tag_id, TCppFunctionCallExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpCastExpr					(TXmlWriter &doc, WORD tag_id, TCppCastExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpSizeofExpr					(TXmlWriter &doc, WORD tag_id, TCppSizeofExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpNewExpr					(TXmlWriter &doc, WORD tag_id, TCppNewExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpDeleteExpr					(TXmlWriter &doc, WORD tag_id, TCppDeleteExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);
	static void	DumpTypeidExpr					(TXmlWriter &doc, WORD tag_id, TCppTypeidExpr &data, TCppXmlDumpChildrenMode cxdcm_mode);

	static void	DumpCppItem						(TXmlWriter &doc, WORD tag_id, TCppItemBase &data, TCppXmlDumpChildrenMode cxdcm_mode);

protected:

	static void	DumpCppItemChildren			(TXmlWriter &doc, TCppItemBase &data, TCppXmlDumpChildrenMode cxdcm_mode);

};

#endif	// TextScan_CppDatabaseXml_H


