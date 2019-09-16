//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "Common/DateTimeHelper.H"
#include  "TextScan/MidStoreXml.H"

//------------------------------------------------------------------------------
//  ====================  TMidLevXmlHelper  =========================
//------------------------------------------------------------------------------

extern TXmlDictRecord xtags_MidLevStoreObjects[];

static const wchar_t *g_NullRepr = L"=NULL=";

static TXmlCnrWritingProps xpr_MidLevCnrProps =
{
	FALSE,				// Use the short tag close form.
	FALSE, FALSE,			// Tag header attrs are all on one line.
	TRUE, TRUE,			// Start every field tag on a new line.
	TRUE, TRUE,			// Start every container on a new line.
	FALSE,				// No crlf before the tag hdr close.
	TRUE,				// Add CrLf before the container close.
	FALSE,				// No crlf after the tag attrs.
	TRUE,				// Add CrLf after the field tags.
	TRUE,				// Add CrLf after the subcontainers.
};

//------------------------------------------------------------------------
//  ===============  MidLev Low Level Objects  =====================
//------------------------------------------------------------------------

// ----- <IdStruct> -----
static TXmlDictRecord xtags_IdStruct[] =
{
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ParsId"),					mxlt_id_struct_pars_id,				},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"FileId"),						mxlt_id_struct_file_id,				},
	{ xdrt_eof }
};

// ----- <SourceAreaInfo> -----
static TXmlDictRecord xtags_SourceAreaInfo[] =
{
	{ xdrt_attr,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ParsId"),					mxlt_src_area_info_pars,			},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"FileId"),						mxlt_src_area_info_fid,				},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"Beg"),						mxlt_src_area_info_beg,			},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"Len"),						mxlt_src_area_info_len,				},
	{ xdrt_eof }
};

// ----- <LinearArea> -----
static TXmlDictRecord xtags_LinearArea[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"Beg"),						mxlt_lin_area_info_beg,				},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"Len"),						mxlt_lin_area_info_len,				},
	{ xdrt_eof }
};

// ----- <SourceLocation> -----
static TXmlDictRecord xtags_SourceLocation[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParsId"),					mxlt_src_loc_pars_id,				},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"LinearArea"),					mxlt_src_loc_lin_area,				xtef_none,	xtags_LinearArea },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"SourceRef"),					mxlt_src_loc_source_ref,			},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"SrcArea"),					mxlt_src_loc_src_area,				xtef_none,	xtags_SourceAreaInfo },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"PartialSrcArea"),				mxlt_src_loc_part_sarea,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"SrcAreaExtGoesUp"),			mxlt_src_loc_sarea_up,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"SrcAreaExtGoesDown"),		mxlt_src_loc_sarea_down,			},
	{ xdrt_eof }
};

// ----- <FileInfo> -----
static TXmlDictRecord xtags_FileInfo[] =
{
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ParsId"),					mxlt_file_info_pars_id,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FileId"),						mxlt_file_info_file_id,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"Name"),						mxlt_file_info_file_name,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"Body"),						mxlt_file_info_file_body,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"Length"),					mxlt_file_info_file_len,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"Date"),						mxlt_file_info_file_date,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"DateAsText"),				mxlt_file_info_file_dtx,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"Crc32"),						mxlt_file_info_file_crc,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"StrgType"),					mxlt_file_info_strg_type,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"RscnType"),					mxlt_file_info_rscn_type,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"LatestMjSrcSessId"),			mxlt_file_info_mj_src_sess_id,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"IndexingSessId"),				mxlt_file_info_raw_sess_id,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"IndexingDate"),				mxlt_file_info_indexing_date,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"IndexingDateAsText"),			mxlt_file_info_indexing_dtx,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ParsRefsLen"),				mxlt_file_info_pars_refs_len,			},
	{ xdrt_eof }
};

// ----- <IndexingKeyInfo> -----
static TXmlDictRecord xtags_IndexingKeyInfo[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"KeyId"),						mxlt_indexing_key_id,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"Type"),						mxlt_indexing_key_type,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"Name"),						mxlt_indexing_key_name,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"FileUseCnt"),					mxlt_indexing_key_fuse_cnt,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"FileRefsLen"),				mxlt_indexing_key_frefs_len,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ParsUseCnt"),				mxlt_indexing_key_puse_cnt,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ParsRefsLen"),				mxlt_indexing_key_prefs_len,		},
	{ xdrt_eof }
};

// ----- <IndexingKeyUseInfo> -----
static TXmlDictRecord xtags_IndexingKeyUseInfo[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"KeyInfo"),					mxlt_indexing_key_use_key,			xtef_none,	xtags_IndexingKeyInfo },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"ContextType"),				mxlt_indexing_key_use_ctype,		},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"SourceLocation"),				mxlt_indexing_key_use_origin,		xtef_none,	xtags_SourceLocation },
	{ xdrt_eof }
};

// ----- <CondExprSmplMode> -----
static TXmlDictRecord xtags_CondExprSmplMode[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"SimplifySimplifConds"),		mxlt_expr_smpl_simplif,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"UnknNamesSimplifMode"),		mxlt_expr_smpl_unkn_names,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"SimplifyConstants"),			mxlt_expr_smpl_constant,			},
	{ xdrt_eof }
};

// ----- <MidLevScannerOptions> -----
static TXmlDictRecord xtags_MidLevScannerOptions[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"WantAnalysisToolProfile"),		mxlt_scan_opts_want_ant_env,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"RawScanningMode"),			mxlt_scan_opts_raw_scan_mode,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BuiltInCondsFid"),				mxlt_scan_opts_built_in_defs,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"DisabDefineStmts"),			mxlt_scan_opts_disab_define,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"DisabIncludeStmts"),			mxlt_scan_opts_disab_include,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"DisabCondParsStmts"),		mxlt_scan_opts_disab_cpars,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"DisabMiscPreprStmts"),		mxlt_scan_opts_disab_misc,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"DisabMacroCalls"),			mxlt_scan_opts_disab_mcalls,		},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CondExprSmplMode"),			mxlt_scan_opts_xpr_smpl_mode,		xtef_none,	xtags_CondExprSmplMode },
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"IgnoreErrorStmts"),			mxlt_scan_opts_ignore_err_st,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"ReportRedefToSameValue"),	mxlt_scan_opts_rprt_redef_sm,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"ReportRedefToDiffValue"),		mxlt_scan_opts_rprt_redef_df,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"ReportInclsFromSameDir"),		mxlt_scan_opts_rprt_inc_sm_dir,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"ReportIncludeDups"),			mxlt_scan_opts_rprt_inc_dupls,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"ReportDefWithUnknNames"),	mxlt_scan_opts_rprt_def_unkn,		},
	{ xdrt_eof }
};

// ----- <MidLevPrepOptions> -----
static TXmlDictRecord xtags_MidLevPrepOptions[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"DisabIncPathKeys"),			mxlt_prep_opts_disab_incpath,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"DisabDefineStmts"),			mxlt_prep_opts_disab_define,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"DisabIncludeStmts"),			mxlt_prep_opts_disab_srcinc,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"ImmedMjSrcLoad"),			mxlt_prep_opts_immed_mjsrc,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"AllowMidLevLexemas"),		mxlt_prep_opts_allow_src_lex,		},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"ScanOpts"),					mxlt_prep_opts_scan_opts,			xtef_none,	xtags_MidLevScannerOptions },
	{ xdrt_eof }
};

// ----- <RawIndexingSessSummary> -----
static TXmlDictRecord xtags_RawIndexingSessSummary[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FilesIndexed"),				mxlt_raw_sumry_files_indexed,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParsingMessages"),			mxlt_raw_sumry_npars_msgs,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParsingWarnings"),			mxlt_raw_sumry_npars_warnings,	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParsingErrors"),				mxlt_raw_sumry_npars_errors,		},
	{ xdrt_eof }
};

// ----- <MidLevParsingSummary> -----
static TXmlDictRecord xtags_MidLevParsingSummary[] =
{
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"CmdlKeyParams"),			mxlt_summary_cmdl_key_params,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"CmdlPosParams"),			mxlt_summary_cmdl_pos_params,	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FileSrcLexemas"),				mxlt_summary_file_src_lexemas,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"MacroSrcLexemas"),			mxlt_summary_macro_src_lexemas,	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"PasteBuffSrcLexemas"),		mxlt_summary_pbuff_src_lexemas,	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"MidLevLexemas"),				mxlt_summary_mid_lev_lexemas,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"DriveSubsts"),				mxlt_summary_drv_substs_num,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"IncludePaths"),				mxlt_summary_inc_paths_num,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BuiltInConds"),				mxlt_summary_built_in_conds,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"PredefConds"),				mxlt_summary_predef_conds,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FilesCacheSize"),				mxlt_summary_file_cache_size,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"TempFilesNum"),				mxlt_summary_temp_files,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"IntermFilesNum"),				mxlt_summary_interm_files,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FileSourceLoads"),			mxlt_summary_file_src_loads,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"DiffFileSources"),				mxlt_summary_diff_file_srcs,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"DiffFileSrcsLen"),				mxlt_summary_diff_files_len,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"MacroDictSize"),				mxlt_summary_macro_dict_size,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"DiffMacroUsed"),				mxlt_summary_diff_macro_used,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"MacroCallsNum"),				mxlt_summary_macro_calls,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"MacroParamCalls"),			mxlt_summary_mparam_calls,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"PasteBuffAreas"),				mxlt_summary_pbuff_areas,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"PasteBuffSources"),			mxlt_summary_pbuff_sources,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"CondParsingsAreas"),			mxlt_summary_cond_pars_areas,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"CondParsingsParams"),		mxlt_summary_cond_params_num,	},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParsingMessages"),			mxlt_summary_npars_msgs,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParsingWarnings"),			mxlt_summary_npars_warnings,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParsingErrors"),				mxlt_summary_npars_errors,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"MidLevEvents"),				mxlt_summary_mid_lev_events,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FinalLinearOffs"),				mxlt_summary_final_lin_offs,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FinalCn"),					mxlt_summary_final_cn,			},
	{ xdrt_eof }
};

// ----- <CtvCompilationInfo> -----
static TXmlDictRecord xtags_CtvCompilationInfo[] =
{
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"BatchId"),					mxlt_compil_info_batch_id,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParsingId"),					mxlt_compil_info_parsing_id,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"LangType"),					mxlt_compil_info_lang_type,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"GrammarVersion"),			mxlt_compil_info_grm_vers,			},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"CmdlOrigin"),				mxlt_compil_info_cmdl_origin,		xtef_none,	xtags_SourceAreaInfo },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"CmdlDocId"),				mxlt_compil_info_cmdl_docid,		xtef_none,	xtags_IdStruct },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"CmdlFileName"),				mxlt_compil_info_cmdl_fname,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"MajorSrcPosParam"),			mxlt_compil_info_mjsrc_param,		},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"MajorSrcDocId"),				mxlt_compil_info_mjsrc_docid,		xtef_none,	xtags_IdStruct },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"MajorSrcFileName"),			mxlt_compil_info_mjsrc_fname,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"MajorSrcScanEvt"),			mxlt_compil_info_mjsrc_scevt,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"MajorSrcEnterEvt"),			mxlt_compil_info_mjsrc_enevt,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"PrevMajorSrcParsSess"),		mxlt_compil_info_mjsrc_pp_id,		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"ObjDocId"),					mxlt_compil_info_obj_docid,			xtef_none,	xtags_IdStruct },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ObjFileName"),				mxlt_compil_info_obj_fname,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParsingDate"),				mxlt_compil_info_pars_date,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ParsingDateAsText"),			mxlt_compil_info_pars_dtx,			},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"PrepOptions"),				mxlt_compil_info_prep_opts,			xtef_none,	xtags_MidLevPrepOptions },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"ScanOptions"),				mxlt_compil_info_scan_opts,			xtef_none,	xtags_MidLevScannerOptions },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"BaseDir"),					mxlt_compil_info_base_dir,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"AllowIntermPaths"),			mxlt_compil_info_allow_interm,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_bool,		DefTag(L"AllowUnsubstedPaths"),		mxlt_compil_info_allow_unsubst,		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"RawSessSummary"),			mxlt_compil_info_raw_summary,		xtef_none,	xtags_RawIndexingSessSummary },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"Summary"),					mxlt_compil_info_summary,			xtef_none,	xtags_MidLevParsingSummary },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FinalParsingStatus"),			mxlt_compil_info_pars_status,		},
	{ xdrt_eof }
};

// ----- <CtvEventHeader> -----
static TXmlDictRecord xtags_CtvEventHeader[] =
{
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParsingId"),					mxlt_event_hdr_parsing_id,			},
	{ xdrt_attr,	xrdm_req_si,	  xtbt_number,	DefTag(L"EventId"),					mxlt_event_hdr_event_id,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"Type"),						mxlt_event_hdr_event_type,			},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"LinearArea"),					mxlt_event_hdr_linear_area,			xtef_none,	xtags_LinearArea },
	{ xdrt_eof }
};

// ----------------------  Functions  ---------------------------

void TMidLevXmlHelper::DumpIdStruct(TXmlWriter &doc, WORD tag_id, TLongId &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_id_struct, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	TIdStruct &strct_data = (TIdStruct&)data;
	if (strct_data.pars_id != 0)
		doc.WriteNumTag(mxlt_id_struct_pars_id, strct_data.pars_id);

	doc.WriteNumTag(mxlt_id_struct_file_id, strct_data.file_id);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpSourceAreaInfo(TXmlWriter &doc, WORD tag_id, TSourceAreaInfo &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_src_area, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	if (data.pars_id != 0)
		doc.WriteNumTag(mxlt_src_area_info_pars, data.pars_id);

	doc.WriteNumTag(mxlt_src_area_info_fid, data.file_id);
	doc.WriteNumTag(mxlt_src_area_info_beg, data.area_beg);
	doc.WriteNumTag(mxlt_src_area_info_len, data.area_len);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpLinearArea(TXmlWriter &doc, WORD tag_id, TLinearArea &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_lin_area, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	doc.WriteNumTag(mxlt_lin_area_info_beg, data.linear_beg);
	doc.WriteNumTag(mxlt_lin_area_info_len, data.linear_len);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpSourceLocation(TXmlWriter &doc, WORD tag_id, TSourceLocation &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_src_loc, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	doc.WriteNumTag(mxlt_src_loc_pars_id, data.parsing_id);
	DumpLinearArea(doc, mxlt_src_loc_lin_area, data.lin_area);

	doc.WriteNumTag(mxlt_src_loc_source_ref, data.source_ref);
	DumpSourceAreaInfo(doc, mxlt_src_loc_src_area, data.src_area);

	if (data.partial_src_area != FALSE)
	{
		doc.WriteBoolTag(mxlt_src_loc_part_sarea, data.partial_src_area);

		if (data.src_ext_goes_up == TRUE)
			doc.WriteBoolTag(mxlt_src_loc_sarea_up, TRUE);
		else doc.WriteBoolTag(mxlt_src_loc_sarea_down, TRUE);
	}

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpFileInfo(TXmlWriter &doc, WORD tag_id, TFileInfo &data, bool file_info_mode)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_file_info, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	// General info about the file.
	if (data.pars_id != 0)
		doc.WriteNumTag(mxlt_file_info_pars_id, data.pars_id);

	doc.WriteNumTag(mxlt_file_info_file_id, data.file_id);
	doc.WriteStringTag(mxlt_file_info_file_name, (data.info.file_name != NULL) ? data.info.file_name : g_NullRepr);

	if (file_info_mode == TRUE)
		doc.WriteNumTag(mxlt_file_info_file_len, data.info.file_len);

	doc.WriteNumTag(mxlt_file_info_file_date, data.info.file_date);
	if (data.info.file_date != 0)
	{
		wchar_t date_buff_text[80];
		doc.WriteStringTag(mxlt_file_info_file_dtx, FormatDateTime(data.info.file_date, date_buff_text, 80, FALSE));
	}

	if (file_info_mode == TRUE)
	{
		// This is dumping info about the file, not about the subdirectory.
		wchar_t crc_buff[20];
		swprintf(crc_buff, 20,	L"0x%08X", data.info.file_crc);

		doc.WriteStringTag(mxlt_file_info_file_crc, crc_buff);
		doc.WriteStringTag(mxlt_file_info_strg_type, TFileInfo::GetStrgTypeEnumName(data.strg_type));
		doc.WriteStringTag(mxlt_file_info_rscn_type, TFileInfo::GetRscnTypeEnumName(data.rscn_type));
	}

	if (data.major_src_latest_pars_id != 0)
		doc.WriteNumTag(mxlt_file_info_mj_src_sess_id, data.major_src_latest_pars_id);

	if (data.raw_indexing_date != 0)
	{
		// Raw indexing session info.
		doc.WriteNumTag(mxlt_file_info_raw_sess_id, data.raw_sess_id);
		doc.WriteNumTag(mxlt_file_info_indexing_date, data.raw_indexing_date);

		wchar_t rinx_date_buff_text[80];
		doc.WriteStringTag(mxlt_file_info_indexing_dtx, FormatDateTime(data.raw_indexing_date, rinx_date_buff_text, 80, FALSE));
	}

	// The length of the parsing session references array.
	if (data.pars_sess_refs_len != 0)
		doc.WriteNumTag(mxlt_file_info_pars_refs_len, data.pars_sess_refs_len);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpIndexingKeyInfo(TXmlWriter &doc, WORD tag_id, TKeyInfo &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_indexing_key, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	wchar_t type_name_buff[60];
	doc.WriteNumTag(mxlt_indexing_key_id, data.key_id);
	doc.WriteStringTag(mxlt_indexing_key_type, TKeyInfo::GetKeyTypeName(data.key_type, type_name_buff));
	doc.WriteStringTag(mxlt_indexing_key_name, (data.key_name != NULL) ? data.key_name : g_NullRepr);
	doc.WriteNumTag(mxlt_indexing_key_fuse_cnt, data.key_file_use_cnt);
	doc.WriteNumTag(mxlt_indexing_key_frefs_len, data.key_file_refs_len);
	doc.WriteNumTag(mxlt_indexing_key_puse_cnt, data.key_pars_use_cnt);
	doc.WriteNumTag(mxlt_indexing_key_prefs_len, data.key_pars_refs_len);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpIndexingKeyUseInfo(TXmlWriter &doc, WORD tag_id, TKeyUseInfo &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_indexing_key_use, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	wchar_t misc_buff[40];
	DumpIndexingKeyInfo(doc, mxlt_indexing_key_use_key, data.key);
	doc.WriteStringTag(mxlt_indexing_key_use_ctype, TKeyInfo::GetRawContextTypeName(data.raw_ctx_type, misc_buff));
	DumpSourceLocation(doc, mxlt_indexing_key_use_origin, data.origin);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpMidLevScannerOptions(TXmlWriter &doc, WORD tag_id, TMidLevScannerOptions &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_scan_opts, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	doc.WriteBoolTag(mxlt_scan_opts_want_ant_env, data.want_analysis_env);
	doc.WriteBoolTag(mxlt_scan_opts_raw_scan_mode, data.raw_scanning_mode);
	doc.WriteNumTag(mxlt_scan_opts_built_in_defs, data.built_in_macros_fid);
	doc.WriteBoolTag(mxlt_scan_opts_disab_define, data.disab_define_stmts);
	doc.WriteBoolTag(mxlt_scan_opts_disab_include, data.disab_include_stmts);
	doc.WriteBoolTag(mxlt_scan_opts_disab_cpars, data.disab_cond_pars_stmts);
	doc.WriteBoolTag(mxlt_scan_opts_disab_misc, data.disab_misc_prepr_stmts);
	doc.WriteBoolTag(mxlt_scan_opts_disab_mcalls, data.disab_macro_calls);

	{
		//  Dump the mode of the conditional expressions simplification.
		doc.OpenContainer(mxlt_scan_opts_xpr_smpl_mode);
		doc.WriteBoolTag(mxlt_expr_smpl_simplif, data.expr_mode.smpl_simplif);

		wchar_t unpm_buff[60];
		doc.WriteStringTag(mxlt_expr_smpl_unkn_names, TCondExprHelper::GetUnknNamesProcModeName(unpm_buff, data.expr_mode.unkn_names));

		doc.WriteBoolTag(mxlt_expr_smpl_constant, data.expr_mode.smpl_constants);
		doc.CloseContainer();
	}

	doc.WriteBoolTag(mxlt_scan_opts_ignore_err_st, data.ignore_error_stmt);
	doc.WriteBoolTag(mxlt_scan_opts_rprt_redef_sm, data.report_macro_redef_to_same_val);
	doc.WriteBoolTag(mxlt_scan_opts_rprt_redef_df, data.report_macro_redef_to_diff_val);
	doc.WriteBoolTag(mxlt_scan_opts_rprt_inc_sm_dir, data.report_includes_from_same_dir);
	doc.WriteBoolTag(mxlt_scan_opts_rprt_inc_dupls, data.report_include_file_duplications);
	doc.WriteBoolTag(mxlt_scan_opts_rprt_def_unkn, data.report_def_opers_with_unkn_names);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvCompilationInfo(TXmlWriter &doc, WORD tag_id, CtvCompilationInfo &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_compil_info, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	// General information.
	if (data.batch_id != 0)
		doc.WriteNumTag(mxlt_compil_info_batch_id, data.batch_id);
	doc.WriteNumTag(mxlt_compil_info_parsing_id, data.parsing_id);
	doc.WriteStringTag(mxlt_compil_info_lang_type, THighLevelDatabase::GetLangTypeName(data.lang_type));
	doc.WriteNumTag(mxlt_compil_info_grm_vers, data.grammar_vers_code);

	// Info about the command line.
	if (data.cmd_line_origin.file_id != 0)
		DumpSourceAreaInfo(doc, mxlt_compil_info_cmdl_origin, data.cmd_line_origin);
	if (data.cmd_line_doc_id != 0)
		DumpIdStruct(doc, mxlt_compil_info_cmdl_docid, data.cmd_line_doc_id);
	if (data.cmd_line_file_name != NULL)
		doc.WriteStringTag(mxlt_compil_info_cmdl_fname, data.cmd_line_file_name);

	// Info about the major source file.
	doc.WriteNumTag(mxlt_compil_info_mjsrc_param, data.major_src_pos_param);
	DumpIdStruct(doc, mxlt_compil_info_mjsrc_docid, data.major_src_doc_id.doc_id);

	doc.WriteStringTag(mxlt_compil_info_mjsrc_fname, (data.major_src_file_name != NULL) ? data.major_src_file_name : g_NullRepr);
	if (data.major_src_scan_evt != 0)
		doc.WriteNumTag(mxlt_compil_info_mjsrc_scevt, data.major_src_scan_evt);
	if (data.major_src_enter_evt != 0)
		doc.WriteNumTag(mxlt_compil_info_mjsrc_enevt, data.major_src_enter_evt);
	if (data.major_src_prev_pars_id != 0)
		doc.WriteNumTag(mxlt_compil_info_mjsrc_pp_id, data.major_src_prev_pars_id);

	// Obj file info.
	if (data.obj_file_doc_id != 0)
		DumpIdStruct(doc, mxlt_compil_info_obj_docid, data.obj_file_doc_id);
	if (data.obj_file_name != NULL)
		doc.WriteStringTag(mxlt_compil_info_obj_fname, data.obj_file_name);

	// Parsing date.
	doc.WriteNumTag(mxlt_compil_info_pars_date, data.parsing_date);
	if (data.parsing_date != 0)
	{
		wchar_t date_buff_text[80];
		doc.WriteStringTag(mxlt_compil_info_pars_dtx, FormatDateTime(data.parsing_date, date_buff_text, 80, FALSE));
	}

	if (data.lang_type != lang_none)
	{
		//
		// This is a language level session.
		//

		if (data.cmd_line_doc_id != 0)
		{
			// The prep phase was taking place. Dump the preparation options.
			doc.OpenContainer(mxlt_compil_info_prep_opts);
			doc.WriteBoolTag(mxlt_prep_opts_disab_incpath, data.prep_opts.disab_inc_path_keys);
			doc.WriteBoolTag(mxlt_prep_opts_disab_define, data.prep_opts.disab_define_keys);
			doc.WriteBoolTag(mxlt_prep_opts_disab_srcinc, data.prep_opts.disab_src_inc_keys);
			doc.WriteBoolTag(mxlt_prep_opts_immed_mjsrc, data.prep_opts.immed_mj_src_load);
			doc.WriteBoolTag(mxlt_prep_opts_allow_src_lex, data.prep_opts.allow_mid_lev_lexemas);
			DumpMidLevScannerOptions(doc, mxlt_prep_opts_scan_opts, data.prep_opts.scan_opts);
			doc.CloseContainer();
		}

		// Dump the scanning options.
		DumpMidLevScannerOptions(doc, mxlt_compil_info_scan_opts, data.scan_opts);
	}

	// Dump the base directory and final info about the drive susbsts.
	doc.WriteStringTag(mxlt_compil_info_base_dir, (data.base_dir != NULL) ? data.base_dir : g_NullRepr);
	doc.WriteBoolTag(mxlt_compil_info_allow_interm, data.allow_intermediate_paths);
	doc.WriteBoolTag(mxlt_compil_info_allow_unsubst, data.allow_unsubsted_paths);

	// Check the type of the parsing session.
	if (data.lang_type == lang_none)
	{
		// Write the raw indexing session summary.
		doc.OpenContainer(mxlt_compil_info_raw_summary);
		doc.WriteNumTag(mxlt_raw_sumry_files_indexed, data.raw_summary.files_indexed);
		doc.WriteNumTag(mxlt_raw_sumry_npars_msgs, data.raw_summary.num_parsing_msgs);
		doc.WriteNumTag(mxlt_raw_sumry_npars_warnings, data.raw_summary.num_warnings);
		doc.WriteNumTag(mxlt_raw_sumry_npars_errors, data.raw_summary.num_errors);
		doc.CloseContainer();
	}
	else
	{
		// Write the parsing summary. All data fields are ULONGs here.
		doc.OpenContainer(mxlt_compil_info_summary);

		if (data.summary.cmdl_key_params != 0)
			doc.WriteNumTag(mxlt_summary_cmdl_key_params, data.summary.cmdl_key_params);
		if (data.summary.cmdl_pos_params != 0)
			doc.WriteNumTag(mxlt_summary_cmdl_pos_params, data.summary.cmdl_pos_params);

		doc.WriteNumTag(mxlt_summary_file_src_lexemas, data.summary.file_src_lexemas);
		doc.WriteNumTag(mxlt_summary_macro_src_lexemas, data.summary.macro_src_lexemas);
		doc.WriteNumTag(mxlt_summary_pbuff_src_lexemas, data.summary.pbuff_src_lexemas);
		doc.WriteNumTag(mxlt_summary_mid_lev_lexemas, data.summary.mid_lev_lexemas);

		if (data.summary.drive_substs_num != 0)
			doc.WriteNumTag(mxlt_summary_drv_substs_num, data.summary.drive_substs_num);

		doc.WriteNumTag(mxlt_summary_inc_paths_num, data.summary.inc_paths_num);
		doc.WriteNumTag(mxlt_summary_built_in_conds, data.summary.built_in_conds);
		doc.WriteNumTag(mxlt_summary_predef_conds, data.summary.predefined_conds);
		doc.WriteNumTag(mxlt_summary_file_cache_size, data.summary.files_cache_size);
		doc.WriteNumTag(mxlt_summary_temp_files, data.summary.temp_files_num);
		doc.WriteNumTag(mxlt_summary_interm_files, data.summary.interm_files_num);
		doc.WriteNumTag(mxlt_summary_file_src_loads, data.summary.file_source_loads);
		doc.WriteNumTag(mxlt_summary_diff_file_srcs, data.summary.different_file_srcs);
		doc.WriteNumTag(mxlt_summary_diff_files_len, data.summary.diff_file_srcs_len);
		doc.WriteNumTag(mxlt_summary_macro_dict_size, data.summary.macro_dict_size);
		doc.WriteNumTag(mxlt_summary_diff_macro_used, data.summary.different_macro_used);
		doc.WriteNumTag(mxlt_summary_macro_calls, data.summary.macro_calls_num);
		doc.WriteNumTag(mxlt_summary_mparam_calls, data.summary.mparam_calls_num);
		doc.WriteNumTag(mxlt_summary_pbuff_areas, data.summary.paste_buff_areas);
		doc.WriteNumTag(mxlt_summary_pbuff_sources, data.summary.paste_buff_sources);
		doc.WriteNumTag(mxlt_summary_cond_pars_areas, data.summary.cond_parsing_areas);
		doc.WriteNumTag(mxlt_summary_cond_params_num, data.summary.cond_params_num);
		doc.WriteNumTag(mxlt_summary_npars_msgs, data.summary.num_parsing_msgs);
		doc.WriteNumTag(mxlt_summary_npars_warnings, data.summary.num_warnings);
		doc.WriteNumTag(mxlt_summary_npars_errors, data.summary.num_errors);
		doc.WriteNumTag(mxlt_summary_mid_lev_events, data.summary.mid_lev_events);
		doc.WriteNumTag(mxlt_summary_final_lin_offs, data.summary.final_linear_offs);
		doc.WriteNumTag(mxlt_summary_final_cn, data.summary.final_cn);

		doc.CloseContainer();
	}

	// Final integer tag.
	doc.WriteNumTag(mxlt_compil_info_pars_status, data.parsing_status);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvEventHeader(TXmlWriter &doc, WORD tag_id, CtvEventHeader &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_event_hdr, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	doc.WriteNumTag(mxlt_event_hdr_parsing_id, data.parsing_id);
	doc.WriteNumTag(mxlt_event_hdr_event_id, data.event_id);
	doc.WriteStringTag(mxlt_event_hdr_event_type, CtvEventHeader::GetEventTypeNameForXml(data.event_type));
	DumpLinearArea(doc, mxlt_event_hdr_linear_area, data.area);

	doc.CloseContainerEx(rcnr_dct);
}

//-----------------------------------------------------------------------------
//  ==================  MidLev Parsing Events  ========================
//-----------------------------------------------------------------------------

static TXmlDictRecord xtags_CtvDriveSubstRegEvent[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_drv_subst_hdr,				xtef_none,	xtags_CtvEventHeader },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"InternalPrefix"),			mxlt_drv_subst_intern,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"ExternalSubst"),			mxlt_drv_subst_extern,				},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CtvTempFileRegEvent[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_temp_file_hdr,				xtef_none,	xtags_CtvEventHeader },
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"FileInfo"),				mxlt_temp_file_info,				xtef_none,	xtags_FileInfo },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CtvPasteBufferingChgEvent[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_paste_buff_hdr,				xtef_none,	xtags_CtvEventHeader },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ParentBuffCn"),			mxlt_paste_buff_parent_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"EvtEnterCn"),			mxlt_paste_buff_enter_cn,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NewBuffLevel"),			mxlt_paste_buff_new_level,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"BuffType"),				mxlt_paste_buff_buff_type,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"BufferFid"),				mxlt_paste_buff_buffer_fid,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"EvtExitLinearOffs"),		mxlt_paste_buff_exit_linear,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"RealLinearEnd"),			mxlt_paste_buff_real_lin_end,		},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CtvLexSourceEnterEvent[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_src_enter_hdr,				xtef_none,	xtags_CtvEventHeader },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"SourceRef"),				mxlt_src_enter_source_ref,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"CondRef"),				mxlt_src_enter_cond_ref,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"PasteBuffRef"),			mxlt_src_enter_pbuff_ref,			},

	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"SrcType"),				mxlt_src_enter_src_type,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"OnDoubt"),				mxlt_src_enter_on_doubt,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"IncFileNameMode"),		mxlt_src_enter_inc_fn_mode,		},

	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NewSrcLevel"),			mxlt_src_enter_new_src_level,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"LinPosAfterEntering"),		mxlt_src_enter_lin_after_enter,		},

	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"EvtExitCn"),				mxlt_src_enter_exit_cn,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"EvtExitCondRef"),			mxlt_src_enter_exit_cond_ref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"EvtExitPasteBuffRef"),		mxlt_src_enter_exit_pbuff_ref,		},

	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"EvtExitLinearPos"),		mxlt_src_enter_exit_linear_pos,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"EvtExitLinPosAfterRet"),	mxlt_src_enter_linear_after_ret,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"SrcOffsAfterRet"),			mxlt_src_enter_src_after_ret,		},

	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"IncPathId"),				mxlt_src_enter_inc_path_id,			},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"IncFileInfo"),				mxlt_src_enter_inc_file_info,			xtef_none,	xtags_FileInfo },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"IncTokenArea"),			mxlt_src_enter_inc_token_area,		xtef_none,	xtags_LinearArea },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"IncFileNameArea"),		mxlt_src_enter_inc_fname_area,		xtef_none,	xtags_LinearArea },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"PosParamsCount"),		mxlt_src_enter_inc_pp_cnt_beg,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"PosPrmsCntAfterRet"),		mxlt_src_enter_inc_pp_cnt_end,		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"SelfChangingMacroArea"),	mxlt_src_enter_scgm_area,			xtef_none,	xtags_SourceAreaInfo },

	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"mCallDefId"),			mxlt_src_enter_mc_def_id,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"mCallDefName"),			mxlt_src_enter_mc_def_name,		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"mCallDefArea"),			mxlt_src_enter_mc_def_area,		xtef_none,	xtags_SourceAreaInfo },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"mCallXpndMacros"),		mxlt_src_enter_mc_xpnd_macro,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"mCallPastePrefix"),		mxlt_src_enter_mc_paste_pref,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"mCallPasteSuffix"),		mxlt_src_enter_mc_paste_suff,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"mCallParamInx"),			mxlt_src_enter_mc_param_inx,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"mCallParamDecor"),		mxlt_src_enter_mc_prm_decor,		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"mCallNameArea"),		mxlt_src_enter_mc_name_area,		xtef_none,	xtags_LinearArea },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"AppendTail"),				mxlt_src_enter_append_tail,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"PasteBuffFid"),			mxlt_src_enter_pbuff_fid,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"PasteBuffLen"),			mxlt_src_enter_pbuff_len,			},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CtvLexSourceExitEvent[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_src_exit_hdr,					xtef_none,	xtags_CtvEventHeader },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"SourceRef"),				mxlt_src_exit_source_ref,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"CondRef"),				mxlt_src_exit_cond_ref,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"PasteBuffRef"),			mxlt_src_exit_pbuff_ref,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"SrcType"),				mxlt_src_exit_src_type,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"LinPosAfterRet"),			mxlt_src_exit_linear_pos,			},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CtvMacroLibraryChangeEvent[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_mdef_chg_hdr,				xtef_none,	xtags_CtvEventHeader },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"PrevDefId"),				mxlt_mdef_chg_prev_def_id,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"mDefStatus"),			mxlt_mdef_chg_mdef_status,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"mDefNature"),			mxlt_mdef_chg_mdef_nature,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"mDefKeyId"),			mxlt_mdef_chg_mdef_key_id,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"mDefName"),			mxlt_mdef_chg_mdef_name,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"NumParams"),			mxlt_mdef_chg_num_params,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"VaArgs"),				mxlt_mdef_chg_va_args_tail,		},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"TokenArea"),				mxlt_mdef_chg_token_area,			xtef_none,	xtags_LinearArea },
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"NameArea"),				mxlt_mdef_chg_name_area,			xtef_none,	xtags_LinearArea },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"BodyArea"),				mxlt_mdef_chg_body_area,			xtef_none,	xtags_LinearArea },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CtvCondParsingChangeEvent[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_cond_pars_hdr,				xtef_none,	xtags_CtvEventHeader },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"SourceRef"),				mxlt_cond_pars_source_ref,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"CondRef"),				mxlt_cond_pars_cond_ref,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"PrevStmtCn"),			mxlt_cond_pars_prev_stmt_id,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NextStmtCn"),			mxlt_cond_pars_next_stmt_id,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"StmtType"),				mxlt_cond_pars_stmt_type,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"ExprSmplRes"),			mxlt_cond_pars_expr_smpl_res,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"ExprResult"),				mxlt_cond_pars_expr_result,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"StmtCodeStt"),			mxlt_cond_pars_stmt_code_stt,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"NewCodeStt"),			mxlt_cond_pars_new_code_stt,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"StmtCodeLevel"),			mxlt_cond_pars_stmt_code_lev,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NewCodeLevel"),			mxlt_cond_pars_new_code_lev,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ScanExprErrorCn"),		mxlt_cond_pars_scan_expr_err,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"ScanExprDetails"),		mxlt_cond_pars_scan_expr_dts,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"NumExprNodes"),			mxlt_cond_pars_num_expr_nods,		},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"StmtTokenArea"),			mxlt_cond_pars_token_area,			xtef_none,	xtags_LinearArea },
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"CondExprArea"),			mxlt_cond_pars_cexpr_area,			xtef_none,	xtags_LinearArea },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CtvMiscPreprocStmtEvent[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_misc_stmt_hdr,				xtef_none,	xtags_CtvEventHeader },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"StmtType"),				mxlt_misc_stmt_stmt_type,			},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"StmtTokenArea"),			mxlt_misc_stmt_token_area,			xtef_none,	xtags_LinearArea },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"StmtCodeStt"),			mxlt_misc_stmt_code_stt,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"EventDetails"),			mxlt_misc_stmt_evt_details,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"EventStrData"),			mxlt_misc_stmt_evt_str_data,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"LineNum"),				mxlt_misc_stmt_line_num,			},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"LineArea"),				mxlt_misc_stmt_line_area,			xtef_none,	xtags_LinearArea },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"FileIdStruct"),			mxlt_misc_stmt_fid_struct,			xtef_none,	xtags_IdStruct },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"FileName"),				mxlt_misc_stmt_file_name,			},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"FileNameArea"),			mxlt_misc_stmt_fname_area,		xtef_none,	xtags_LinearArea },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CtvNewIncPathEvent[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_inc_path_hdr,					xtef_none,	xtags_CtvEventHeader },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"IncPath"),				mxlt_inc_path_inc_path,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"IncDirectory"),			mxlt_inc_path_inc_dir,				},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CtvErrorWarningEvent[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_err_warn_hdr,				xtef_none,	xtags_CtvEventHeader },
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ParentEvent"),			mxlt_err_warn_parent_event,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"ErrWarnClass"),			mxlt_err_warn_err_class,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ErrorCode"),				mxlt_err_warn_err_code,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"Message"),				mxlt_err_warn_err_message,		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"ExtContext"),				mxlt_err_warn_ext_context,			xtef_none,	xtags_LinearArea },
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CtvCondParamUseEvent[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_cond_param_hdr,				xtef_none,	xtags_CtvEventHeader },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"CondStmtId"),			mxlt_cond_param_cond_stmt_id,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"ParamId"),				mxlt_cond_param_prm_id,			},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"ParamType"),			mxlt_cond_param_prm_type,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"ParamName"),			mxlt_cond_param_prm_name,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"DecorDefId"),			mxlt_cond_param_decor_def_id,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"DecorDefStts"),			mxlt_cond_param_decor_def_stts,	},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CtvGrammarDefnSection[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_grammar_sect_hdr,			xtef_none,	xtags_CtvEventHeader },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"SectionType"),			mxlt_grammar_sect_type,			},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CtvGrammarDefnObject[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_grammar_object_hdr,			xtef_none,	xtags_CtvEventHeader },
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"Symbol"),				mxlt_grammar_object_symbol,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"AppId"),					mxlt_grammar_object_appid,		},
	{ xdrt_eof }
};

static TXmlDictRecord xtags_CtvCppItemRefEvent[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"Header"),				mxlt_cpp_item_ref_hdr,				xtef_none,	xtags_CtvEventHeader },
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"CppItemType"),			mxlt_cpp_item_ref_item_type,		},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"CppItemNesting"),		mxlt_cpp_item_ref_item_nesting,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ParsSessCppItemId"),		mxlt_cpp_item_ref_pars_sess_id,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ProjectLevelCppItemId"),	mxlt_cpp_item_ref_prj_level_id,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"ExtSrcAreaType"),		mxlt_cpp_item_ref_ext_atype,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ExtSrcAreaBeg"),			mxlt_cpp_item_ref_ext_abeg,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ExtSrcAreaLen"),			mxlt_cpp_item_ref_ext_alen,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ExtraSrcAreasFileId"),		mxlt_cpp_item_ref_bin_file_id,		},
	{ xdrt_eof }
};

// ----------------------  Functions  ---------------------------

void TMidLevXmlHelper::DumpCtvDriveSubstRegEvent(TXmlWriter &doc, WORD tag_id, CtvDriveSubstRegEvent &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_drive_subst_event, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	DumpCtvEventHeader(doc, mxlt_drv_subst_hdr, data.hdr);
	doc.WriteStringTag(mxlt_drv_subst_intern, data.subst_info.internal_prefix);
	doc.WriteStringTag(mxlt_drv_subst_extern, data.subst_info.external_subst);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvTempFileRegEvent(TXmlWriter &doc, WORD tag_id, CtvTempFileRegEvent &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_temp_file_event, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	DumpCtvEventHeader(doc, mxlt_temp_file_hdr, data.hdr);
	DumpFileInfo(doc, mxlt_temp_file_info, data.temp_file_info);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvPasteBufferingChgEvent(TXmlWriter &doc, WORD tag_id, CtvPasteBufferingChgEvent &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_paste_buff_event, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	DumpCtvEventHeader(doc, mxlt_paste_buff_hdr, data.hdr);

	if (data.parent_buff_ref != 0)
		doc.WriteNumTag(mxlt_paste_buff_parent_ref, data.parent_buff_ref);
	if (data.evt_enter_cn != 0)
		doc.WriteNumTag(mxlt_paste_buff_enter_cn, data.evt_enter_cn);

	doc.WriteNumTag(mxlt_paste_buff_new_level, data.new_buff_level);
	doc.WriteStringTag(mxlt_paste_buff_buff_type, TLexema::GetKeywordIdEnumName(data.buff_type));
	doc.WriteNumTag(mxlt_paste_buff_buffer_fid, data.buffer_fid);

	if (data.evt_exit_linear != 0)
		doc.WriteNumTag(mxlt_paste_buff_exit_linear, data.evt_exit_linear);

	if (data.real_linear_end != 0)
		doc.WriteNumTag(mxlt_paste_buff_real_lin_end, data.real_linear_end);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvLexSourceEnterEvent(TXmlWriter &doc, WORD tag_id, CtvLexSourceEnterEvent &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_src_enter_event, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	// Common part.
	DumpCtvEventHeader(doc, mxlt_src_enter_hdr, data.hdr);

	doc.WriteNumTag(mxlt_src_enter_source_ref, data.source_ref);
	doc.WriteNumTag(mxlt_src_enter_cond_ref, data.cond_ref);
	if (data.paste_buffering_ref != 0 || data.evt_exit_pbuff_ref != 0)
		doc.WriteNumTag(mxlt_src_enter_pbuff_ref, data.paste_buffering_ref);

	doc.WriteStringTag(mxlt_src_enter_src_type, TLexemasSource::GetLexSourceTypeEnumName(data.src_type));

	if (data.on_doubt == TRUE)
		doc.WriteBoolTag(mxlt_src_enter_on_doubt, data.on_doubt);
	if (data.inc_fn_mode == TRUE)
		doc.WriteBoolTag(mxlt_src_enter_inc_fn_mode, data.inc_fn_mode);

	doc.WriteNumTag(mxlt_src_enter_new_src_level, data.new_src_level);
	doc.WriteNumTag(mxlt_src_enter_lin_after_enter, data.LinearPosAfterEntering());

	doc.WriteNumTag(mxlt_src_enter_exit_cn, data.evt_exit_id);
	if (data.evt_exit_cond_ref != 0)
		doc.WriteNumTag(mxlt_src_enter_exit_cond_ref, data.evt_exit_cond_ref);
	if (data.paste_buffering_ref != 0 || data.evt_exit_pbuff_ref != 0)
		doc.WriteNumTag(mxlt_src_enter_exit_pbuff_ref, data.evt_exit_pbuff_ref);

	if (data.evt_exit_linear_pos != 0)
		doc.WriteNumTag(mxlt_src_enter_exit_linear_pos, data.evt_exit_linear_pos);
	if (data.evt_exit_linear_after_ret != 0)
		doc.WriteNumTag(mxlt_src_enter_linear_after_ret, data.evt_exit_linear_after_ret);
	if (data.src_offs_after_exit != 0)
		doc.WriteNumTag(mxlt_src_enter_src_after_ret, data.src_offs_after_exit);

	// Select the right union group depending on the src_type.
	switch (data.src_type)
	{
		case srct_none:
				break;

		case srct_cmdl:
		case srct_ant_env:
		case srct_file:
		case srct_bltin_defs:
		case srct_mj_src:
				{
					// File enter group of fields.
					doc.WriteNumTag(mxlt_src_enter_inc_path_id, data.inc_path_id);
					DumpFileInfo(doc, mxlt_src_enter_inc_file_info, data.inc_file_info);
					DumpLinearArea(doc, mxlt_src_enter_inc_token_area, data.inc_token_area);
					DumpLinearArea(doc, mxlt_src_enter_inc_fname_area, data.inc_fname_area);
					doc.WriteNumTag(mxlt_src_enter_inc_pp_cnt_beg, data.pos_params_cnt);
					if (data.evt_exit_id != 0)
						doc.WriteNumTag(mxlt_src_enter_inc_pp_cnt_end, data.evt_exit_pos_prms_cnt);
				}
				break;

		case srct_scgm:
				{
					// Body of the self changing macros like __FILE__, __LINE__, etc.
					DumpSourceAreaInfo(doc, mxlt_src_enter_scgm_area, data.scgm_scan_area);
				}
				break;

		case srct_macro:
		case srct_mparam:
				{
					// Macro definition and macro param call.
					if (data.src_type == srct_macro)
						doc.WriteNumTag(mxlt_src_enter_mc_def_id, data.mcall_def_id);

					doc.WriteStringTag(mxlt_src_enter_mc_def_name, data.mcall_def_name);
					DumpSourceAreaInfo(doc, mxlt_src_enter_mc_def_area, data.mcall_def_area);

					doc.WriteBoolTag(mxlt_src_enter_mc_xpnd_macro, data.mcall_xpnd_macros);

					if (data.mcall_paste_prefix == TRUE)
						doc.WriteBoolTag(mxlt_src_enter_mc_paste_pref, data.mcall_paste_prefix);
					if (data.mcall_paste_suffix == TRUE)
						doc.WriteBoolTag(mxlt_src_enter_mc_paste_suff, data.mcall_paste_suffix);

					if (data.src_type == srct_mparam)
					{
						doc.WriteNumTag(mxlt_src_enter_mc_param_inx, data.mcall_param_inx);
						if (data.mcall_param_decor != 0)
							doc.WriteStringTag(mxlt_src_enter_mc_prm_decor, TLexema::GetKeywordIdEnumName(data.mcall_param_decor));
					}

					if (data.src_type == srct_macro)
					{
						DumpLinearArea(doc, mxlt_src_enter_mc_name_area, data.mcall_name_area);
					}
				}
				break;

		case srct_pbuff_macro:
		case srct_pbuff_mparam:
				{
					// The paste buff group of fields.
					doc.WriteBoolTag(mxlt_src_enter_append_tail, data.append_tail);
					doc.WriteNumTag(mxlt_src_enter_pbuff_fid, data.paste_buff_fid);
					doc.WriteNumTag(mxlt_src_enter_pbuff_len, data.paste_buff_len);
				}
				break;

		default:
			{
				wchar_t buffer[120];
				swprintf(buffer, 120, L"TMidLevXmlHelper::DumpCtvLexSourceEnterEvent: Bogus source type %d. parsId=%lu, eventId=%lu.",
						data.src_type, data.hdr.parsing_id, data.hdr.event_id);
				doc.ReportError(xwre_app_failure, buffer);
			}
			break;
	}

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvLexSourceExitEvent(TXmlWriter &doc, WORD tag_id, CtvLexSourceExitEvent &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_src_exit_event, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	DumpCtvEventHeader(doc, mxlt_src_exit_hdr, data.hdr);
	doc.WriteNumTag(mxlt_src_exit_source_ref, data.source_ref);
	doc.WriteNumTag(mxlt_src_exit_cond_ref, data.cond_ref);

	if (data.paste_buff_ref != 0)
		doc.WriteNumTag(mxlt_src_exit_pbuff_ref, data.paste_buff_ref);

	doc.WriteStringTag(mxlt_src_exit_src_type, TLexemasSource::GetLexSourceTypeEnumName(data.src_type));
	doc.WriteNumTag(mxlt_src_exit_linear_pos, data.linear_pos_after_return);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvMacroLibraryChangeEvent(TXmlWriter &doc, WORD tag_id, CtvMacroLibraryChangeEvent &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_mdef_chg_event, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	DumpCtvEventHeader(doc, mxlt_mdef_chg_hdr, data.hdr);
	if (data.prev_def_id != 0)
		doc.WriteNumTag(mxlt_mdef_chg_prev_def_id, data.prev_def_id);

	doc.WriteStringTag(mxlt_mdef_chg_mdef_status, TMacroDefinition::GetStatusName(data.mdef_status, FALSE));
	doc.WriteStringTag(mxlt_mdef_chg_mdef_nature, TMacroDefinition::GetOriginName(data.mdef_origin_type, FALSE));
	doc.WriteNumTag(mxlt_mdef_chg_mdef_key_id, data.mdef_key_id);
	doc.WriteStringTag(mxlt_mdef_chg_mdef_name, data.mdef_name);

	if (data.num_params != 0)
		doc.WriteNumTag(mxlt_mdef_chg_num_params, data.num_params);
	if (data.va_args_tail == TRUE)
		doc.WriteBoolTag(mxlt_mdef_chg_va_args_tail, data.va_args_tail);

	DumpLinearArea(doc, mxlt_mdef_chg_token_area, data.token_area);
	DumpLinearArea(doc, mxlt_mdef_chg_name_area, data.name_area);
	DumpLinearArea(doc, mxlt_mdef_chg_body_area, data.body_area);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvCondParsingChangeEvent(TXmlWriter &doc, WORD tag_id, CtvCondParsingChangeEvent &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_cond_pars_event, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	wchar_t smlp_buff[60], cer1[60], cer2[60], cer3[60], scan_expr_dtls_buff[60];
	swprintf(scan_expr_dtls_buff, 60, L"0x%X", data.scan_expr_details);

	DumpCtvEventHeader(doc, mxlt_cond_pars_hdr, data.hdr);
	doc.WriteNumTag(mxlt_cond_pars_source_ref, data.source_ref);
	doc.WriteNumTag(mxlt_cond_pars_cond_ref, data.cond_ref);
	doc.WriteNumTag(mxlt_cond_pars_prev_stmt_id, data.prev_stmt_id);
	doc.WriteNumTag(mxlt_cond_pars_next_stmt_id, data.next_stmt_id);
	doc.WriteStringTag(mxlt_cond_pars_stmt_type, TLexema::GetKeywordIdEnumName(data.stmt_type));
	doc.WriteStringTag(mxlt_cond_pars_expr_smpl_res, TCondExprHelper::GetSmplResultName(smlp_buff, data.expr_smpl_res, FALSE));
	doc.WriteStringTag(mxlt_cond_pars_expr_result, TCondExprHelper::GetCondStatusEnumName(cer1, data.expr_result));
	doc.WriteStringTag(mxlt_cond_pars_stmt_code_stt, TCondExprHelper::GetCondStatusEnumName(cer2, data.stmt_code_stt));
	doc.WriteStringTag(mxlt_cond_pars_new_code_stt, TCondExprHelper::GetCondStatusEnumName(cer3, data.new_code_stt));
	doc.WriteNumTag(mxlt_cond_pars_stmt_code_lev, data.stmt_code_level);
	doc.WriteNumTag(mxlt_cond_pars_new_code_lev, data.new_code_level);

	if (data.scan_expr_error_id != 0)
		doc.WriteNumTag(mxlt_cond_pars_scan_expr_err, data.scan_expr_error_id);

	doc.WriteStringTag(mxlt_cond_pars_scan_expr_dts, scan_expr_dtls_buff);
	doc.WriteNumTag(mxlt_cond_pars_num_expr_nods, data.num_expr_nodes);
	DumpLinearArea(doc, mxlt_cond_pars_token_area, data.stmt_token_area);
	DumpLinearArea(doc, mxlt_cond_pars_cexpr_area, data.cond_expr_area);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvMiscPreprocStmtEvent(TXmlWriter &doc, WORD tag_id, CtvMiscPreprocStmtEvent &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_misc_stmt_event, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	wchar_t code_stt_buff[60];
	TMiscEventEx mpr = data.mis_event_type;

	DumpCtvEventHeader(doc, mxlt_misc_stmt_hdr, data.hdr);
	doc.WriteStringTag(mxlt_misc_stmt_stmt_type, PrMiscPreprocStmtEvent::GetMiscEventExEnumName(mpr));
	DumpLinearArea(doc, mxlt_misc_stmt_token_area, data.token_area);
	doc.WriteStringTag(mxlt_misc_stmt_code_stt, TCondExprHelper::GetCondStatusEnumName(code_stt_buff, data.code_stt));

	doc.WriteNumTag(mxlt_misc_stmt_evt_details, data.evt_details);
	if (data.evt_str_data != NULL)
	{
		doc.WriteStringTag(mxlt_misc_stmt_evt_str_data, data.evt_str_data);
	}

	if (mpr == exmis_line)
	{
		doc.WriteNumTag(mxlt_misc_stmt_line_num, data.line_num);
		DumpLinearArea(doc, mxlt_misc_stmt_line_area, data.line_num_area);
	}

	if (mpr == exmis_mjsrc_area || mpr == exmis_failed_include || mpr == exmis_import || mpr == exmis_line || mpr == exmis_using)
	{
		DumpIdStruct(doc, mxlt_misc_stmt_fid_struct, data.doc_ident);
		doc.WriteStringTag(mxlt_misc_stmt_file_name, data.file_name);
		DumpLinearArea(doc, mxlt_misc_stmt_fname_area, data.fname_area);
	}

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvNewIncPathEvent(TXmlWriter &doc, WORD tag_id, CtvNewIncPathEvent &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_inc_path_event, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	DumpCtvEventHeader(doc, mxlt_inc_path_hdr, data.hdr);
	doc.WriteStringTag(mxlt_inc_path_inc_path, data.inc_path);
	doc.WriteStringTag(mxlt_inc_path_inc_dir, data.inc_directory);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvErrorWarningEvent(TXmlWriter &doc, WORD tag_id, CtvErrorWarningEvent &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_err_warn_event, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	DumpCtvEventHeader(doc, mxlt_err_warn_hdr, data.hdr);

	if (data.parent_event != 0)
		doc.WriteNumTag(mxlt_err_warn_parent_event, data.parent_event);

	doc.WriteStringTag(mxlt_err_warn_err_class, TLexema::GetLexErrorClassEnumName(data.err_class));
	doc.WriteNumTag(mxlt_err_warn_err_code, data.err_code);
	doc.WriteStringTag(mxlt_err_warn_err_message, data.err_msg);

	if (data.ext_context.linear_beg != 0)
		DumpLinearArea(doc, mxlt_err_warn_ext_context, data.ext_context);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvCondParamUseEvent(TXmlWriter &doc, WORD tag_id, CtvCondParamUseEvent &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_cond_param_event, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	wchar_t param_type_name_buff[80];

	DumpCtvEventHeader(doc, mxlt_cond_param_hdr, data.hdr);
	doc.WriteNumTag(mxlt_cond_param_cond_stmt_id, data.cond_stmt_id);
	doc.WriteNumTag(mxlt_cond_param_prm_id, data.cond_param_id);
	doc.WriteStringTag(mxlt_cond_param_prm_type, TCondExprHelper::GetCondExprTreeItemTypeName(param_type_name_buff, data.cond_param_type, FALSE));
	doc.WriteStringTag(mxlt_cond_param_prm_name, data.cond_param_name);

	doc.WriteNumTag(mxlt_cond_param_decor_def_id, data.decor_mdef_id);
	if (data.decor_mdef_id != 0)
	{
		doc.WriteStringTag(mxlt_cond_param_decor_def_stts, TMacroDefinition::GetStatusName(data.decor_mdef_status, FALSE));
	}

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvGrammarDefnSection(TXmlWriter &doc, WORD tag_id, CtvGrammarDefnSection &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_grm_defn_section, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	DumpCtvEventHeader(doc, mxlt_grammar_sect_hdr, data.hdr);
	doc.WriteStringTag(mxlt_grammar_sect_type, GetGrammarDefnSectionTypeName(data.section_type));

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvGrammarDefnObject(TXmlWriter &doc, WORD tag_id, CtvGrammarDefnObject &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_grm_defn_object, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	DumpCtvEventHeader(doc, mxlt_grammar_object_hdr, data.hdr);
	doc.WriteNumTag(mxlt_grammar_object_symbol, data.object_sym);
	doc.WriteNumTag(mxlt_grammar_object_appid, data.object_app_id);

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpCtvCppItemRefEvent(TXmlWriter &doc, WORD tag_id, CtvCppItemRefEvent &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_cpp_ref_event, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	DumpCtvEventHeader(doc, mxlt_cpp_item_ref_hdr, data.hdr);
	doc.WriteStringTag(mxlt_cpp_item_ref_item_type, TCppItemHelper::GetItemTypeEnumName(data.cpp_item_type));
	doc.WriteNumTag(mxlt_cpp_item_ref_item_nesting, data.cpp_item_nesting);

	if (data.pars_sess_cpp_item_id != 0 || data.project_level_cpp_item_id != 0)
	{
		// At least one of the ids is not zero.
		if (data.pars_sess_cpp_item_id != 0)
			doc.WriteNumTag(mxlt_cpp_item_ref_pars_sess_id, data.pars_sess_cpp_item_id);
		if (data.project_level_cpp_item_id != 0)
			doc.WriteNumTag(mxlt_cpp_item_ref_prj_level_id, data.project_level_cpp_item_id);
	}
	else
	{
		// Both ids are zero. Dump them both to highlight this fact.
		doc.WriteNumTag(mxlt_cpp_item_ref_pars_sess_id, data.pars_sess_cpp_item_id);
		doc.WriteNumTag(mxlt_cpp_item_ref_prj_level_id, data.project_level_cpp_item_id);
	}

	if (data.ext_src_area.m_cpp_atype != cxa_none)
	{
		// Additional source area is not empty.
		doc.WriteStringTag(mxlt_cpp_item_ref_ext_atype, TCppItemHelper::GetSrcAreaTypeEnumName(data.ext_src_area.m_cpp_atype));
		doc.WriteNumTag(mxlt_cpp_item_ref_ext_abeg, data.ext_src_area.m_cpp_lin_area.linear_beg);
		doc.WriteNumTag(mxlt_cpp_item_ref_ext_alen, data.ext_src_area.m_cpp_lin_area.linear_len);
	}

	if (data.extra_areas_temp_file_id != 0)
	{
		// This is an Id of the file that contains an array of additional areas.
		doc.WriteNumTag(mxlt_cpp_item_ref_bin_file_id, data.extra_areas_temp_file_id);
	}

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpParsingEvent(TXmlWriter &doc, WORD tag_id, CtvParsingEventInfo &data)
{
	// The id of the tag should be decided on the upper layer.
	switch (data.hdr.event_type)
	{
		case pset_none:
				DumpCtvEventHeader(doc, tag_id, data.hdr);
				break;

		case pset_register_drive_subst:
				DumpCtvDriveSubstRegEvent(doc, tag_id, data.drv_subst);
				break;

		case pset_register_temp_file:
				DumpCtvTempFileRegEvent(doc, tag_id, data.temp_file);
				break;

		case pset_paste_buffering_change:
				DumpCtvPasteBufferingChgEvent(doc, tag_id, data.pbuff_chg);
				break;

		case pset_lex_source_enter:
				DumpCtvLexSourceEnterEvent(doc, tag_id, data.src_enter);
				break;

		case pset_lex_source_exit:
				DumpCtvLexSourceExitEvent(doc, tag_id, data.src_exit);
				break;

		case pset_macro_library_change:
				DumpCtvMacroLibraryChangeEvent(doc, tag_id, data.mdef_chg);
				break;

		case pset_cond_parsing_change:
				DumpCtvCondParsingChangeEvent(doc, tag_id, data.cond_pars);
				break;

		case pset_misc_preproc_stmt:
				DumpCtvMiscPreprocStmtEvent(doc, tag_id, data.misc_stmt);
				break;

		case pset_new_include_path:
				DumpCtvNewIncPathEvent(doc, tag_id, data.inc_path);
				break;

		case pset_error_warning_event:
				DumpCtvErrorWarningEvent(doc, tag_id, data.err_warn);
				break;

		case pset_cond_param_use:
				DumpCtvCondParamUseEvent(doc, tag_id, data.cond_param);
				break;

		case pset_grammar_section:
				DumpCtvGrammarDefnSection(doc, tag_id, data.grm_sect);
				break;

		case pset_grammar_object:
				DumpCtvGrammarDefnObject(doc, tag_id, data.grm_object);
				break;

		case pset_cpp_item_ref:
				DumpCtvCppItemRefEvent(doc, tag_id, data.cpp_item);
				break;

		default:
			{
				// Bogus or unsupported event_type value.
				wchar_t buffer[120];
				swprintf(buffer, 120, L"TMidLevXmlHelper::DumpParsingEvent: Bogus event type %d. parsId=%lu, eventId=%lu.", data.hdr.event_type, data.hdr.parsing_id, data.hdr.event_id);
				doc.ReportError(xwre_app_failure, buffer);
			}
			break;
	}
}

//----------------------------------------------------------------------------------
// ===================  TMidLevStoreReader contents  ======================
//----------------------------------------------------------------------------------

// ----- <ParsingEventsList> -----
static TXmlDictRecord xtags_ParsingEventsList[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"EventHeader"),				mxlt_event_header,			xtef_none,	xtags_CtvEventHeader },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"DriveSubstRegEvent"),			mxlt_drive_subst_event,		xtef_none,	xtags_CtvDriveSubstRegEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"TempFileRegEvent"),			mxlt_temp_file_event,			xtef_none,	xtags_CtvTempFileRegEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"PasteBufferingChgEvent"),		mxlt_paste_buff_event,			xtef_none,	xtags_CtvPasteBufferingChgEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"LexSourceEnterEvent"),		mxlt_src_enter_event,			xtef_none,	xtags_CtvLexSourceEnterEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"LexSourceExitEvent"),			mxlt_src_exit_event,			xtef_none,	xtags_CtvLexSourceExitEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"MacroLibraryChangeEvent"),	mxlt_mdef_chg_event,			xtef_none,	xtags_CtvMacroLibraryChangeEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CondParsingChangeEvent"),	mxlt_cond_pars_event,			xtef_none,	xtags_CtvCondParsingChangeEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"MiscPreprocStmtEvent"),		mxlt_misc_stmt_event,			xtef_none,	xtags_CtvMiscPreprocStmtEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"NewIncPathEvent"),			mxlt_inc_path_event,			xtef_none,	xtags_CtvNewIncPathEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"ErrorWarningEvent"),			mxlt_err_warn_event,			xtef_none,	xtags_CtvErrorWarningEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CondParamUseEvent"),		mxlt_cond_param_event,		xtef_none,	xtags_CtvCondParamUseEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"GrammarDefnSection"),		mxlt_grm_defn_section,			xtef_none,	xtags_CtvGrammarDefnSection },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"GrammarDefnObject"),			mxlt_grm_defn_object,			xtef_none,	xtags_CtvGrammarDefnObject },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppItemRefEvent"),			mxlt_cpp_ref_event,			xtef_none,	xtags_CtvCppItemRefEvent },
	{ xdrt_eof }
};

// ----- <ParsingSessionInfo> -----
static TXmlDictRecord xtags_ParsingSessionInfo[] =
{
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"ParsingHeader"),				mxlt_parsing_header,			xtef_none,	xtags_CtvCompilationInfo },
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"ParsingEventsList"),			mxlt_parsing_events,			xtef_none,	xtags_ParsingEventsList },
	{ xdrt_eof }
};

// ----- <BatchIdsList> -----
static TXmlDictRecord xtags_BatchIdsList[] =
{
	{ xdrt_field,	xrdm_opt_mi,	  xtbt_number,	DefTag(L"BatchId"),					mxlt_l2list_batch_id,			},
	{ xdrt_eof }
};

// ----- <DiskFilesList> -----
static TXmlDictRecord xtags_DiskFilesList[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"DiskFileInfo"),				mxlt_l2list_disk_file_info,		xtef_none,	xtags_FileInfo },
	{ xdrt_eof }
};

// ----- <IndexingKeysList> -----
static TXmlDictRecord xtags_IndexingKeysList[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"IndexingKeyInfo"),			mxlt_l2list_indexing_key_info,	xtef_none,	xtags_IndexingKeyInfo },
	{ xdrt_eof }
};

// ----- <ParsingSessionsList> -----
static TXmlDictRecord xtags_ParsingSessionsList[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"ParsingSessionInfo"),			mxlt_l2list_pars_session_info,	xtef_none,	xtags_ParsingSessionInfo },
	{ xdrt_eof }
};

// ----- <MidLevStore> -----
static TXmlDictRecord xtags_MidLevStore[] =
{
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"BatchIdsList"),				mxlt_store_batch_ids_list,		xtef_none,	xtags_BatchIdsList },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"DiskFilesList"),				mxlt_store_disk_files_list,		xtef_none,	xtags_DiskFilesList },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"IndexingKeysList"),			mxlt_store_indexing_keys_list,	xtef_none,	xtags_IndexingKeysList },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"ParsingSessionsList"),			mxlt_store_pars_sessions_list,	xtef_none,	xtags_ParsingSessionsList },
	{ xdrt_eof }
};

// -------------   Root Containers Directory   -------------------------

static TXmlDictRecord xtags_MidLevStoreObjects[] =
{
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"IdStruct"),					mxlt_id_struct,				xtef_none,	xtags_IdStruct },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"SourceArea"),				mxlt_src_area,				xtef_none,	xtags_SourceAreaInfo },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"LinearArea"),					mxlt_lin_area,					xtef_none,	xtags_LinearArea },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"SourceLocation"),				mxlt_src_loc,					xtef_none,	xtags_SourceLocation },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"FileInfo"),					mxlt_file_info,					xtef_none,	xtags_FileInfo },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"IndexingKeyInfo"),			mxlt_indexing_key,			xtef_none,	xtags_IndexingKeyInfo },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"IndexingKeyUseInfo"),			mxlt_indexing_key_use,			xtef_none,	xtags_IndexingKeyUseInfo },
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"ScanOptions"),				mxlt_scan_opts,				xtef_none,	xtags_MidLevScannerOptions },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"CtvCompilationInfo"),			mxlt_compil_info,				xtef_none,	xtags_CtvCompilationInfo },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"CtvEventHeader"),			mxlt_event_hdr,				xtef_none,	xtags_CtvEventHeader },

	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"DriveSubstRegEvent"),			mxlt_drive_subst_event,		xtef_none,	xtags_CtvDriveSubstRegEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"TempFileRegEvent"),			mxlt_temp_file_event,			xtef_none,	xtags_CtvTempFileRegEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"PasteBufferingChgEvent"),		mxlt_paste_buff_event,			xtef_none,	xtags_CtvPasteBufferingChgEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"LexSourceEnterEvent"),		mxlt_src_enter_event,			xtef_none,	xtags_CtvLexSourceEnterEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"LexSourceExitEvent"),			mxlt_src_exit_event,			xtef_none,	xtags_CtvLexSourceExitEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"MacroLibraryChangeEvent"),	mxlt_mdef_chg_event,			xtef_none,	xtags_CtvMacroLibraryChangeEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CondParsingChangeEvent"),	mxlt_cond_pars_event,			xtef_none,	xtags_CtvCondParsingChangeEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"MiscPreprocStmtEvent"),		mxlt_misc_stmt_event,			xtef_none,	xtags_CtvMiscPreprocStmtEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"NewIncPathEvent"),			mxlt_inc_path_event,			xtef_none,	xtags_CtvNewIncPathEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"ErrorWarningEvent"),			mxlt_err_warn_event,			xtef_none,	xtags_CtvErrorWarningEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CondParamUseEvent"),		mxlt_cond_param_event,		xtef_none,	xtags_CtvCondParamUseEvent },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"GrammarDefnSection"),		mxlt_grm_defn_section,			xtef_none,	xtags_CtvGrammarDefnSection },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"GrammarDefnObject"),			mxlt_grm_defn_object,			xtef_none,	xtags_CtvGrammarDefnObject },
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppItemRefEvent"),			mxlt_cpp_ref_event,			xtef_none,	xtags_CtvCppItemRefEvent },

	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"ParsingSessionInfo"),			mxlt_parsing_sess_info,			xtef_none,	xtags_ParsingSessionInfo },

	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"BatchIdsList"),				mxlt_store_batch_ids_list,		xtef_none,	xtags_BatchIdsList },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"DiskFilesList"),				mxlt_store_disk_files_list,		xtef_none,	xtags_DiskFilesList },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"IndexingKeysList"),			mxlt_store_indexing_keys_list,	xtef_none,	xtags_IndexingKeysList },
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"ParsingSessionsList"),			mxlt_store_pars_sessions_list,	xtef_none,	xtags_ParsingSessionsList },

	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"MidLevStore"),				mxlt_store_store,				xtef_none,	xtags_MidLevStore },

	{ xdrt_eof }
};

// ----------------------  Functions  ---------------------------

void TMidLevXmlHelper::DumpParsingSession(TXmlWriter &doc, WORD tag_id, TMidLevStoreReader &store, ID parsing_id, PSET_TYPES_MASK events_mask)
{
	// Open a nesting layer for the whole parsing session.
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_parsing_sess_info, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	// Dump the parsing header.
	if (store.GetCompilationInfo(parsing_id) == TRUE)
	{
		DumpCtvCompilationInfo(doc, mxlt_parsing_header, store.CompilationData());

		if (events_mask != 0)
		{
			// Dump parsing events of the requested types. Open a nesting leayer for this.
			doc.OpenContainer(mxlt_parsing_events);

			//
			// Depending on the nature of the store, it will either return events in the order of event_ids
			// or it will return them category by category.
			//
			if (store.StartParsingEventsIteration(parsing_id, events_mask) == TRUE)
			{
				while (store.NextParsingEvent() == TRUE)
				{
					CtvEventType evt = store.EventData().hdr.event_type;
					if ((events_mask & PSET_TYPE_BIT(evt)) != 0)
					{
						DumpParsingEvent(doc, mxlt_event_header+evt, store.EventData());
					}
				}

				store.CloseParsingEventsIteration();
			}

			// Close the events list nesting layer.
			doc.CloseContainer();
		}
	}
	else
	{
		doc.ReportError(xwre_app_failure, L"Error getting the compilation info (mxlt_parsing_header).");
	}

	// Close the parsing session nesting layer.
	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpBatchIdsList(TXmlWriter &doc, WORD tag_id, TMidLevStoreReader &store)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_store_batch_ids_list, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	// Iterate all batches in the store.
	if (store.StartBatchesIteration() == TRUE)
	{
		ID batch_id;
		while (store.NextBatchId(batch_id) == TRUE)
			doc.WriteNumTag(mxlt_l2list_batch_id, batch_id);

		store.CloseBatchesIteration();
	}

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpDiskFilesList(TXmlWriter &doc, WORD tag_id, TMidLevStoreReader &store)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_store_disk_files_list, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	// Iterate disk files only.
	if (store.StartFilesIteration(TRUE, FALSE) == TRUE)
	{
		while (store.NextFileRecord() == TRUE)
			DumpFileInfo(doc, mxlt_l2list_disk_file_info, store.FileData());

		store.CloseFilesIteration();
	}

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpIndexingKeysList(TXmlWriter &doc, WORD tag_id, TMidLevStoreReader &store)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_store_indexing_keys_list, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	// Iterate the indexing keys.
	if (store.StartIndexingKeysIteration() == TRUE)
	{
		while (store.NextIndexingKeyRecord() == TRUE)
			DumpIndexingKeyInfo(doc, mxlt_l2list_indexing_key_info, store.IndexingKeyData());

		store.CloseIndexingKeysIteration();
	}

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpParsingSessionsList(TXmlWriter &doc, WORD tag_id, TMidLevStoreReader &store, ID batch_id, ID parsing_id, PSET_TYPES_MASK events_mask)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_store_pars_sessions_list, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	if (batch_id == 0 && parsing_id != 0)
	{
		// This is a special procedure for dumping only one parsing session.
		DumpParsingSession(doc, mxlt_l2list_pars_session_info, store, parsing_id, events_mask);
	}
	else
	{
		// Create secondary query processor. Note that this action may fail.
		TMidLevStoreReader *ext_qproc = store.CreateSecondaryStore();
		if (ext_qproc != NULL)
		{
			// Iterate all or some of the parsing sessions in the store. This loop is similar to dumping
			// the parsing session headers. But the target function inside the loop is different.
			if (store.StartCompilationsIteration(batch_id) == TRUE)
			{
				while (store.NextCompilationRecord() == TRUE)
					DumpParsingSession(doc, mxlt_l2list_pars_session_info, *ext_qproc, store.CompilationData().parsing_id, events_mask);

				store.CloseCompilationsIteration();
			}
			else
			{
				if (batch_id != 0)
					doc.ReportError(xwre_app_failure, L"Error starting the compilations iteration (mxlt_l2list_pars_session_info).");
			}

			// Release the secondary query processor.
			store.ReleaseSecondaryStore(ext_qproc);
		}
		else
		{
			doc.ReportError(xwre_app_failure, L"Error creating the query processor (mxlt_l2list_pars_session_info).");
		}
	}

	doc.CloseContainerEx(rcnr_dct);
}

void TMidLevXmlHelper::DumpDataStore(TXmlWriter &doc, WORD tag_id, TMidLevStoreReader &store,
										ID batch_id, ID parsing_id, PSET_TYPES_MASK events_mask,
										bool emit_batch_ids_list, bool emit_disk_files_list,
										bool emit_indexing_keys_list, bool emit_parsing_sessions_list)
{
	// Open the outer layer for the whole store.
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, mxlt_store_store, xtags_MidLevStoreObjects, &xpr_MidLevCnrProps, &xpr_MidLevCnrProps);

	// Emit major lists.
	if (emit_batch_ids_list == TRUE)
		DumpBatchIdsList(doc, mxlt_store_batch_ids_list, store);
	if (emit_disk_files_list == TRUE)
		DumpDiskFilesList(doc, mxlt_store_disk_files_list, store);
	if (emit_indexing_keys_list == TRUE)
		DumpIndexingKeysList(doc, mxlt_store_indexing_keys_list, store);
	if (emit_parsing_sessions_list == TRUE)
		DumpParsingSessionsList(doc, mxlt_store_pars_sessions_list, store, batch_id, parsing_id, events_mask);

	// Close the layer of the whole data store.
	doc.CloseContainerEx(rcnr_dct);
}


