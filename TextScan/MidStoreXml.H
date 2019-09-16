//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	TextScan_MidStoreXml_H
#define	TextScan_MidStoreXml_H

#ifndef   Common_XmlWriter_H
#include  "Xml/XmlWriter.H"
#endif
#ifndef   TextScan_MidStore_H
#include  "TextScan/MidStore.H"
#endif

enum TMidLevXmlTags
{
	mxlt_none,

	//
	// Section 1. Low Level Structures.
	//

	// <IdStruct>
	mxlt_id_struct,
	mxlt_id_struct_pars_id,					// Optional.
	mxlt_id_struct_file_id,

	// <SourceAreaInfo>
	mxlt_src_area,
	mxlt_src_area_info_pars,				// Optional.
	mxlt_src_area_info_fid,
	mxlt_src_area_info_beg,
	mxlt_src_area_info_len,

	// <LinearArea>
	mxlt_lin_area,
	mxlt_lin_area_info_beg,
	mxlt_lin_area_info_len,

	// <SourceLocation>
	mxlt_src_loc,
	mxlt_src_loc_pars_id,
	mxlt_src_loc_lin_area,
	mxlt_src_loc_source_ref,
	mxlt_src_loc_src_area,
	mxlt_src_loc_part_sarea,				// Optional.
	mxlt_src_loc_sarea_up,					// Optional.
	mxlt_src_loc_sarea_down,				// Optional.

	// <FileInfo>
	mxlt_file_info,
	mxlt_file_info_pars_id,					// Optional.
	mxlt_file_info_file_id,					// The file id should not be zero.
	mxlt_file_info_file_name,
	mxlt_file_info_file_body,					// This field is never emitted.
	mxlt_file_info_file_len,
	mxlt_file_info_file_date,					// DateTime in dec from as string
	mxlt_file_info_file_dtx,					// DateTime as text. This field should not be used while reading the XML doc.
	mxlt_file_info_file_crc,					// DWORD in the hex form as string.
	mxlt_file_info_strg_type,					// Enum.
	mxlt_file_info_rscn_type,				// Enum.

	mxlt_file_info_mj_src_sess_id,			// Optional.
	mxlt_file_info_raw_sess_id,				// Optional.
	mxlt_file_info_indexing_date,			// Optional.
	mxlt_file_info_indexing_dtx,				// Optional. When the indexing date field is present and it is not zero,
											// this means that the whole raw indexing group is present.
	mxlt_file_info_pars_refs_len,

	// <IndexingKeyInfo>
	mxlt_indexing_key,
	mxlt_indexing_key_id,
	mxlt_indexing_key_type,					// Enum.
	mxlt_indexing_key_name,
	mxlt_indexing_key_fuse_cnt,
	mxlt_indexing_key_frefs_len,
	mxlt_indexing_key_puse_cnt,
	mxlt_indexing_key_prefs_len,

	// <IndexingKeyUseInfo>
	mxlt_indexing_key_use,
	mxlt_indexing_key_use_key,
	mxlt_indexing_key_use_ctype,
	mxlt_indexing_key_use_origin,

	// <CondExprSmplMode>
	mxlt_expr_smpl_simplif,
	mxlt_expr_smpl_unkn_names,			// Enum.
	mxlt_expr_smpl_constant,

	// <MidLevScannerOptions>
	mxlt_scan_opts,
	mxlt_scan_opts_want_ant_env,
	mxlt_scan_opts_raw_scan_mode,
	mxlt_scan_opts_built_in_defs,
	mxlt_scan_opts_disab_define,
	mxlt_scan_opts_disab_include,
	mxlt_scan_opts_disab_cpars,
	mxlt_scan_opts_disab_misc,
	mxlt_scan_opts_disab_mcalls,
	mxlt_scan_opts_xpr_smpl_mode,			// Container.
	mxlt_scan_opts_ignore_err_st,
	mxlt_scan_opts_rprt_redef_sm,
	mxlt_scan_opts_rprt_redef_df,
	mxlt_scan_opts_rprt_inc_sm_dir,
	mxlt_scan_opts_rprt_inc_dupls,
	mxlt_scan_opts_rprt_def_unkn,

	// <MidLevPrepOptions>
	mxlt_prep_opts_disab_incpath,
	mxlt_prep_opts_disab_define,
	mxlt_prep_opts_disab_srcinc,
	mxlt_prep_opts_immed_mjsrc,
	mxlt_prep_opts_allow_src_lex,
	mxlt_prep_opts_scan_opts,				// Container.

	// <RawIndexingSessSummary>
	mxlt_raw_sumry_files_indexed,
	mxlt_raw_sumry_npars_msgs,
	mxlt_raw_sumry_npars_warnings,
	mxlt_raw_sumry_npars_errors,

	// <MidLevParsingSummary>
	mxlt_summary_cmdl_key_params,
	mxlt_summary_cmdl_pos_params,
	mxlt_summary_file_src_lexemas,
	mxlt_summary_macro_src_lexemas,
	mxlt_summary_pbuff_src_lexemas,
	mxlt_summary_mid_lev_lexemas,
	mxlt_summary_drv_substs_num,
	mxlt_summary_inc_paths_num,
	mxlt_summary_built_in_conds,
	mxlt_summary_predef_conds,
	mxlt_summary_file_cache_size,
	mxlt_summary_temp_files,
	mxlt_summary_interm_files,
	mxlt_summary_file_src_loads,
	mxlt_summary_diff_file_srcs,
	mxlt_summary_diff_files_len,
	mxlt_summary_macro_dict_size,
	mxlt_summary_diff_macro_used,
	mxlt_summary_macro_calls,
	mxlt_summary_mparam_calls,
	mxlt_summary_pbuff_areas,
	mxlt_summary_pbuff_sources,
	mxlt_summary_cond_pars_areas,
	mxlt_summary_cond_params_num,
	mxlt_summary_npars_msgs,
	mxlt_summary_npars_warnings,
	mxlt_summary_npars_errors,
	mxlt_summary_mid_lev_events,
	mxlt_summary_final_lin_offs,
	mxlt_summary_final_cn,

	// <CtvCompilationInfo>
	mxlt_compil_info,
	mxlt_compil_info_batch_id,
	mxlt_compil_info_parsing_id,
	mxlt_compil_info_lang_type,				// Enum.
	mxlt_compil_info_grm_vers,
	mxlt_compil_info_cmdl_origin,			// Optional.
	mxlt_compil_info_cmdl_docid,			// Optional.
	mxlt_compil_info_cmdl_fname,			// Optional.

	mxlt_compil_info_mjsrc_param,
	mxlt_compil_info_mjsrc_docid,
	mxlt_compil_info_mjsrc_fname,
	mxlt_compil_info_mjsrc_scevt,
	mxlt_compil_info_mjsrc_enevt,
	mxlt_compil_info_mjsrc_pp_id,

	mxlt_compil_info_obj_docid,				// Optional.
	mxlt_compil_info_obj_fname,				// Optional.
	mxlt_compil_info_pars_date,				// DateTime as dec number.
	mxlt_compil_info_pars_dtx,				// DateTime as text. This is optional field.
											// It should not be used when reading the xml doc.
	mxlt_compil_info_prep_opts,				// Container.
	mxlt_compil_info_scan_opts,				// Container.
	mxlt_compil_info_base_dir,
	mxlt_compil_info_allow_interm,
	mxlt_compil_info_allow_unsubst,
	mxlt_compil_info_raw_summary,			// Container.
	mxlt_compil_info_summary,				// Container.
	mxlt_compil_info_pars_status,

	// <CtvEventHeader>
	mxlt_event_hdr,
	mxlt_event_hdr_parsing_id,
	mxlt_event_hdr_event_id,
	mxlt_event_hdr_event_type,				// Enum.
	mxlt_event_hdr_linear_area,				// Container.

	//
	// Section 2. Parsing events.
	//

	// <CtvDrvSubstRegEvent>
	mxlt_drv_subst_hdr,
	mxlt_drv_subst_intern,
	mxlt_drv_subst_extern,

	// <CtvTempFileRegEvent>
	mxlt_temp_file_hdr,
	mxlt_temp_file_info,

	// <CtvPasteBufferingChgEvent>
	mxlt_paste_buff_hdr,
	mxlt_paste_buff_parent_ref,				// Optional. It is better to show the root record without this field.
	mxlt_paste_buff_enter_cn,
	mxlt_paste_buff_new_level,
	mxlt_paste_buff_buff_type,				// Enum (TPreprocToken).
	mxlt_paste_buff_buffer_fid,
	mxlt_paste_buff_exit_linear,
	mxlt_paste_buff_real_lin_end,

	// <CtvLexSourceEnterEvent>
	mxlt_src_enter_hdr,
	mxlt_src_enter_source_ref,
	mxlt_src_enter_cond_ref,
	mxlt_src_enter_pbuff_ref,

	mxlt_src_enter_src_type,
	mxlt_src_enter_on_doubt,
	mxlt_src_enter_inc_fn_mode,

	mxlt_src_enter_new_src_level,
	mxlt_src_enter_lin_after_enter,

	mxlt_src_enter_exit_cn,
	mxlt_src_enter_exit_cond_ref,
	mxlt_src_enter_exit_pbuff_ref,

	mxlt_src_enter_exit_linear_pos,
	mxlt_src_enter_linear_after_ret,
	mxlt_src_enter_src_after_ret,

	mxlt_src_enter_inc_path_id,
	mxlt_src_enter_inc_file_info,
	mxlt_src_enter_inc_token_area,
	mxlt_src_enter_inc_fname_area,
	mxlt_src_enter_inc_pp_cnt_beg,
	mxlt_src_enter_inc_pp_cnt_end,
	mxlt_src_enter_scgm_area,

	mxlt_src_enter_mc_def_id,
	mxlt_src_enter_mc_def_name,
	mxlt_src_enter_mc_def_area,
	mxlt_src_enter_mc_xpnd_macro,
	mxlt_src_enter_mc_paste_pref,
	mxlt_src_enter_mc_paste_suff,
	mxlt_src_enter_mc_param_inx,
	mxlt_src_enter_mc_prm_decor,
	mxlt_src_enter_mc_name_area,
	mxlt_src_enter_append_tail,
	mxlt_src_enter_pbuff_fid,
	mxlt_src_enter_pbuff_len,

	// <CtvLexSourceExitEvent>
	mxlt_src_exit_hdr,
	mxlt_src_exit_source_ref,
	mxlt_src_exit_cond_ref,
	mxlt_src_exit_pbuff_ref,
	mxlt_src_exit_src_type,
	mxlt_src_exit_linear_pos,

	// <CtvMacroLibraryChangeEvent>
	mxlt_mdef_chg_hdr,
	mxlt_mdef_chg_prev_def_id,				// Optional.
	mxlt_mdef_chg_mdef_status,
	mxlt_mdef_chg_mdef_nature,
	mxlt_mdef_chg_mdef_key_id,
	mxlt_mdef_chg_mdef_name,
	mxlt_mdef_chg_num_params,			// Optional.
	mxlt_mdef_chg_va_args_tail,				// Optional.
	mxlt_mdef_chg_token_area,
	mxlt_mdef_chg_name_area,
	mxlt_mdef_chg_body_area,

	// <CtvCondParsingChangeEvent>
	mxlt_cond_pars_hdr,
	mxlt_cond_pars_source_ref,
	mxlt_cond_pars_cond_ref,
	mxlt_cond_pars_prev_stmt_id,
	mxlt_cond_pars_next_stmt_id,
	mxlt_cond_pars_stmt_type,
	mxlt_cond_pars_expr_smpl_res,
	mxlt_cond_pars_expr_result,
	mxlt_cond_pars_stmt_code_stt,
	mxlt_cond_pars_new_code_stt,
	mxlt_cond_pars_stmt_code_lev,
	mxlt_cond_pars_new_code_lev,
	mxlt_cond_pars_scan_expr_err,			// Optional.
	mxlt_cond_pars_scan_expr_dts,
	mxlt_cond_pars_num_expr_nods,
	mxlt_cond_pars_token_area,
	mxlt_cond_pars_cexpr_area,

	// <CtvMiscPreprocStmtEvent>
	mxlt_misc_stmt_hdr,
	mxlt_misc_stmt_stmt_type,
	mxlt_misc_stmt_token_area,
	mxlt_misc_stmt_code_stt,
	mxlt_misc_stmt_evt_details,
	mxlt_misc_stmt_evt_str_data,
	mxlt_misc_stmt_line_num,
	mxlt_misc_stmt_line_area,
	mxlt_misc_stmt_fid_struct,				// Container.
	mxlt_misc_stmt_file_name,
	mxlt_misc_stmt_fname_area,

	// <CtvNewIncPathEvent>
	mxlt_inc_path_hdr,
	mxlt_inc_path_inc_path,
	mxlt_inc_path_inc_dir,

	// <CtvErrorWarningEvent>
	mxlt_err_warn_hdr,
	mxlt_err_warn_parent_event,			// Optional.
	mxlt_err_warn_err_class,
	mxlt_err_warn_err_code,
	mxlt_err_warn_err_message,
	mxlt_err_warn_ext_context,				// Optional.

	// <CtvCondParamUseEvent>
	mxlt_cond_param_hdr,
	mxlt_cond_param_cond_stmt_id,
	mxlt_cond_param_prm_id,
	mxlt_cond_param_prm_type,
	mxlt_cond_param_prm_name,
	mxlt_cond_param_decor_def_id,			// Optional.
	mxlt_cond_param_decor_def_stts,		// Optional.

	// <CtvGrammarDefnSection>
	mxlt_grammar_sect_hdr,
	mxlt_grammar_sect_type,				// Enum.

	// <CtvGrammarDefnObject>
	mxlt_grammar_object_hdr,
	mxlt_grammar_object_symbol,
	mxlt_grammar_object_appid,				// Optional.

	// <CtvGrammarDefnSection>
	mxlt_cpp_item_ref_hdr,
	mxlt_cpp_item_ref_item_type,			// Enum.
	mxlt_cpp_item_ref_item_nesting,
	mxlt_cpp_item_ref_pars_sess_id,
	mxlt_cpp_item_ref_prj_level_id,
	mxlt_cpp_item_ref_ext_atype,			// Enum.
	mxlt_cpp_item_ref_ext_abeg,
	mxlt_cpp_item_ref_ext_alen,
	mxlt_cpp_item_ref_bin_file_id,

	//
	// Section 3. Top level containers.
	//

	// <ParsingEventsList>
	mxlt_event_header,
	mxlt_drive_subst_event,
	mxlt_temp_file_event,
	mxlt_paste_buff_event,
	mxlt_src_enter_event,
	mxlt_src_exit_event,
	mxlt_mdef_chg_event,
	mxlt_cond_pars_event,
	mxlt_misc_stmt_event,
	mxlt_inc_path_event,
	mxlt_err_warn_event,
	mxlt_cond_param_event,
	mxlt_grm_defn_section,
	mxlt_grm_defn_object,
	mxlt_cpp_ref_event,

	// <ParsingSessionInfo>
	mxlt_parsing_sess_info,
	mxlt_parsing_header,
	mxlt_parsing_events,

	//
	//  <BatchIdsList>
	//  <DiskFilesList>
	//  <IndexingKeysList>
	//  <ParsingSessionsList>
	//
	//	These are Ids of the items inside the lists. All items in the list have the same tag_id.
	//
	mxlt_l2list_batch_id,
	mxlt_l2list_disk_file_info,
	mxlt_l2list_indexing_key_info,
	mxlt_l2list_pars_session_info,

	//
	//  <MidLevStore>
	//
	//	These are ids of the lists inside the MidLevStore.
	//
	mxlt_store_store,
	mxlt_store_batch_ids_list,
	mxlt_store_disk_files_list,
	mxlt_store_indexing_keys_list,
	mxlt_store_pars_sessions_list,
};

class TMidLevXmlHelper
{
public:

	//
	//  All functions of this class are static.
	//  There are 2 variants of calls that differ by the tag_id value:
	//
	//       tag_id
	//   ----------------------------------
	//		0		This type of call should be used when the structure is saved as a separate document,
	//				i.e. when it is not part of a bigger document. The method will use a default container name.
	//       value		This type of the call can be used ONLY when the structure is saved into the already opened
	//				XML nesting. Opened nesting means that the tags dict is set and this dict can be searched for
	//				an entry with the passed id. The name of the container will be taken from the dict entry.
	//				This allows saving structures using the context specific names.
	//

	//
	// Group 1. Low level structures.
	//

	static void	DumpIdStruct								(TXmlWriter &doc, WORD tag_id, TLongId &data);
	static void	DumpSourceAreaInfo						(TXmlWriter &doc, WORD tag_id, TSourceAreaInfo &data);
	static void	DumpLinearArea							(TXmlWriter &doc, WORD tag_id, TLinearArea &data);
	static void	DumpSourceLocation					(TXmlWriter &doc, WORD tag_id, TSourceLocation &data);

	static void	DumpFileInfo								(TXmlWriter &doc, WORD tag_id, TFileInfo &data, bool file_info_mode = TRUE);
					// Last param allows to dump the structure either in thr directory info mode (FALSE) or in the file info mode (TRUE).

	static void	DumpIndexingKeyInfo					(TXmlWriter &doc, WORD tag_id, TKeyInfo &data);
	static void	DumpIndexingKeyUseInfo				(TXmlWriter &doc, WORD tag_id, TKeyUseInfo &data);
	static void	DumpMidLevScannerOptions				(TXmlWriter &doc, WORD tag_id, TMidLevScannerOptions &data);
	static void	DumpCtvCompilationInfo					(TXmlWriter &doc, WORD tag_id, CtvCompilationInfo &data);
	static void	DumpCtvEventHeader					(TXmlWriter &doc, WORD tag_id, CtvEventHeader &data);

	//
	// Group 2. Parsing events.
	//

	static void	DumpCtvDriveSubstRegEvent			(TXmlWriter &doc, WORD tag_id, CtvDriveSubstRegEvent &data);
	static void	DumpCtvTempFileRegEvent				(TXmlWriter &doc, WORD tag_id, CtvTempFileRegEvent &data);
	static void	DumpCtvPasteBufferingChgEvent		(TXmlWriter &doc, WORD tag_id, CtvPasteBufferingChgEvent &data);
	static void	DumpCtvLexSourceEnterEvent			(TXmlWriter &doc, WORD tag_id, CtvLexSourceEnterEvent &data);
	static void	DumpCtvLexSourceExitEvent			(TXmlWriter &doc, WORD tag_id, CtvLexSourceExitEvent &data);
	static void	DumpCtvMacroLibraryChangeEvent	(TXmlWriter &doc, WORD tag_id, CtvMacroLibraryChangeEvent &data);
	static void	DumpCtvCondParsingChangeEvent	(TXmlWriter &doc, WORD tag_id, CtvCondParsingChangeEvent &data);
	static void	DumpCtvMiscPreprocStmtEvent		(TXmlWriter &doc, WORD tag_id, CtvMiscPreprocStmtEvent &data);
	static void	DumpCtvNewIncPathEvent				(TXmlWriter &doc, WORD tag_id, CtvNewIncPathEvent &data);
	static void	DumpCtvErrorWarningEvent			(TXmlWriter &doc, WORD tag_id, CtvErrorWarningEvent &data);
	static void	DumpCtvCondParamUseEvent			(TXmlWriter &doc, WORD tag_id, CtvCondParamUseEvent &data);

	static void	DumpCtvGrammarDefnSection			(TXmlWriter &doc, WORD tag_id, CtvGrammarDefnSection &data);
	static void	DumpCtvGrammarDefnObject			(TXmlWriter &doc, WORD tag_id, CtvGrammarDefnObject &data);
	static void	DumpCtvCppItemRefEvent				(TXmlWriter &doc, WORD tag_id, CtvCppItemRefEvent &data);

	static void	DumpParsingEvent						(TXmlWriter &doc, WORD tag_id, CtvParsingEventInfo &data);

	//
	// Group 3. Dumping data from the TMidLevStore.
	//

	static void	DumpParsingSession					(TXmlWriter &doc, WORD tag_id, TMidLevStoreReader &store,
																ID parsing_id, PSET_TYPES_MASK events_mask = PSET_ALL_TYPES);

	static void	DumpBatchIdsList						(TXmlWriter &doc, WORD tag_id, TMidLevStoreReader &store);
	static void	DumpDiskFilesList						(TXmlWriter &doc, WORD tag_id, TMidLevStoreReader &store);
	static void	DumpIndexingKeysList					(TXmlWriter &doc, WORD tag_id, TMidLevStoreReader &store);
	static void	DumpParsingSessionsList				(TXmlWriter &doc, WORD tag_id, TMidLevStoreReader &store,
																ID batch_id = 0, ID parsing_id = 0, PSET_TYPES_MASK events_mask = PSET_ALL_TYPES);

	static void	DumpDataStore							(TXmlWriter &doc, WORD tag_id, TMidLevStoreReader &store,
																ID batch_id = 0, ID parsing_id = 0, PSET_TYPES_MASK events_mask = PSET_ALL_TYPES,
																bool emit_batch_ids_list = FALSE,
																bool emit_disk_files_list = TRUE,
																bool emit_indexing_keys_list = TRUE,
																bool emit_parsing_sessions_list = TRUE);
					//
					//     batch_id  parsing_id
					//   --------------------------------------------------------
					//         0             0		Dump all parsing headers and all parsing sessions.
					//         0           !=0		Dump only one parsing header and one parsing session.
					//       !=0           x		Dump headers and parsing sessions that belong to
					//						the passed batch_id.
					//
};

#endif	// TextScan_MidStoreXml_H


