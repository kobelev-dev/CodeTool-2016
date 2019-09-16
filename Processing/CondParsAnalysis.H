//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Conditional parsing structure analysis.
//

#ifndef	Processing_CondParsAnalysis_H
#define	Processing_CondParsAnalysis_H

#ifndef   Xml_HtmlGenCore_H
#include  "Xml/HtmlGenCore.H"
#endif
#ifndef   TextScan_RamBasedStore_H
#include  "TextScan/RamBasedStore.H"
#endif
#ifndef   TextScan_CondParsDatabase_H
#include  "TextScan/CondParsDatabase.H"
#endif
#ifndef   Processing_HtmlReportHelper_H
#include  "Processing/HtmlReportHelper.H"
#endif

class TCpanHelperBase : public THtmlReportHelper
{
public:
			TCpanHelperBase(TGenericConsoleEx &cons);
			~TCpanHelperBase() { assert(m_curr_reader == NULL); }

	bool		PrepareHelper(TMidLevRamLogger &mldb);
	void		CleanupHelper(TMidLevRamLogger &mldb);

	void		DumpSourceFiles(const wchar_t *reports_directory, bool delete_old_source_files, bool delete_processed_pars_sessions_from_database);
				// This method generates sources for all files that are marked for processing.

	void		DumpSourceFileName(THtmlDocument &rprt);
	void		DumpSourceFileBody(THtmlDocument &rprt);
				// These methods process the current file of the object. Its id is stored in the m_curr_fid.

	void		GenSingleFileNameCell(THtmlDocument &rprt, const wchar_t *show_src_file_script_name, TCpanNameUseInfosArray &locations,
										int &iloc, int psess_inx, THtmlCrLfMode crlf_mode = crlm_before_and_after_object);
	void		GenLinesRangeAndGap(THtmlDocument &rprt, THtmlSourceCodeFile &src_file, long &prev_dumped_line, long curr_gap_beg, long curr_gap_len);
	void		GenSkippedLinesRange(THtmlDocument &rprt, THtmlSourceCodeFile &src_file, long skipped_beg, long skipped_end);

protected:

	TMidLevStoreReader				*m_curr_reader;

	struct TFileProcInfo
	{
		ID			m_pars_sessions[2];
						// For now the indexes in this array are fixed. The index 0 is occupied with the session that should
						// be present all the times. All events from this session are painted. The element with an index 1 is
						// occupied with an optional session. Only the #define/#undef events from this session are painted.

		TStructsArray<long, 16, 256>	m_line_numbers;
										// This is an array of important guarded line numbers. Lines with these numbers
										// should be preserved when generating the file listing.

		inline void FixObjectMove() { m_line_numbers.FixObjectMove(); }
		inline void ReleaseObject() { m_line_numbers.ReleaseObject(); }
		inline void WipeObject() { m_line_numbers.WipeObject(); }
	};

	ID											m_curr_fid;
	TStructsArrayEx2<TFileProcInfo, 4096, 8192>		m_src_proc_info;
													// This is array of the file infos. Index into this array is an id of the disk file.

	bool				m_enable_src_files_legal_censoring;
	bool				m_enable_src_files_bandwidth_saving;

	int				m_num_framing_lines;
	int				m_min_lines_to_remove;
						// These data fields are props of the bandwidth saving files preparation procedure.
};

class TCpanDefnListsIntersectionHelper : public TCpanHelperBase
{
public:
			TCpanDefnListsIntersectionHelper(TGenericConsoleEx &cons, TCondParsAnalysisDataStorage &ds1, TCondParsAnalysisDataStorage &ds2)
				: TCpanHelperBase(cons), m_ds1(ds1), m_ds2(ds2) { }
protected:

	void		WriteHtmlReportBody(THtmlDocument &rprt);

	void		WriteTableHeaderRow(THtmlDocument &rprt);
	void		WriteDefnInfoRows(THtmlDocument &rprt, int cnt_defns, const wchar_t *defn_name, TCpanNameUseInfosArray &defn_locs1, TCpanNameUseInfosArray &defn_locs2);

protected:

	TCondParsAnalysisDataStorage		&m_ds1;
	TCondParsAnalysisDataStorage		&m_ds2;
};

class TCpanCondParamsClassificationHelper : public TCpanHelperBase
{
public:
			TCpanCondParamsClassificationHelper(TGenericConsoleEx &cons, TCondParsAnalysisDataStorage &ds)
				: TCpanHelperBase(cons), m_ds(ds) { m_cnt_free_params = m_cnt_dbl_def_preventers = m_cnt_other_params = m_cnt_literal_consts = 0; m_show_src_scipt_name[0] = 0; }
protected:

	void		WriteHtmlReportBody(THtmlDocument &rprt);
	void		WriteSummaryReportBody(THtmlDocument &rprt);

	void		DumpFreeCondParsParams(THtmlDocument &rprt);
	void		DumpDoubleDefPreventerParams(THtmlDocument &rprt);
	void		DumpOtherNonLiteralConstParams(THtmlDocument &rprt);
	void		DumpLiteralConstParameters(THtmlDocument &rprt);

	void		WriteCategoryTextHeader(THtmlDocument &rprt, const wchar_t *href_to_table, const wchar_t *category_name);
	void		WriteParamsTableHeader(THtmlDocument &rprt, bool lit_consts_table);
	void		CheckWriteHeaderRow(THtmlDocument &rprt, int cnt_objects, bool lit_consts_table);
	void		WriteOrdinaryParamInfo(THtmlDocument &rprt, int cnt_objects, TCpanMacroNameObject &mno);
	void		WriteLiteralConstParamInfo(THtmlDocument &rprt, int cnt_objects, TCpanMacroNameObject &mno);

	static const wchar_t *NumUsesText(int cnt_uses, wchar_t *buffer) { if (cnt_uses <= 0) return(L"-"); else return(_itow(cnt_uses, buffer, 10)); }
	static const wchar_t *NumUsesStyle(int cnt_uses) { return((cnt_uses <= 0) ? L"arp_num_uses_dim" : L"arp_num_uses"); }

	void		WritePadddingAndControlCells(THtmlDocument &rprt, const wchar_t *locs_name);
	void		WriteUseLocationsInfo(THtmlDocument &rprt, const wchar_t *locs_name, TCpanMacroNameObject &mno);
	void		WriteUseLocations(THtmlDocument &rprt, TCpanNameUseInfosArray &locations, int psess_inx);

	void		WriteSummaryReportRow(THtmlDocument &rprt, const wchar_t *href_to_table, const wchar_t *category_name, int num_params, const wchar_t *extra_style = NULL);

protected:

	TCondParsAnalysisDataStorage		&m_ds;

	int		m_cnt_free_params;
	int		m_cnt_dbl_def_preventers;
	int		m_cnt_other_params;
	int		m_cnt_literal_consts;

	wchar_t		m_show_src_scipt_name[80];

	friend class TCpanProcessor;
};

class TCpanProcessor
{
public:
			TCpanProcessor(TGenericConsoleEx &cons) : m_cpan_proc_console(cons) { }

	void		CompareDefinitionSets(const wchar_t *root_dir1, const wchar_t *root_dir2, const wchar_t **list_of_fname_exts);
	void		ClassifyCondParsParams(const wchar_t *report_fnames_core, const wchar_t *root_directory, const wchar_t **list_of_fname_exts);

	static int		DumpAllCondParsParams(TCondParsAnalysisDataStorage &ds);
	static void	DumpSingleCondParsParam(int cnt, TCpanMacroNameObject &mno);

protected:

	bool		CollectMacroNames(TCpanMidLevScanCbkHandler::TScanPurpose scan_type, TMidLevRamLogger &logger, TCondParsAnalysisDataStorage &ds, const wchar_t *root_dir, const wchar_t **list_of_fname_exts);
	bool		CollectCondParams(TMidLevRamLogger &logger, TCondParsAnalysisDataStorage &ds, const wchar_t *root_dir, const wchar_t **list_of_fname_exts, const wchar_t *expandable_macros_file_name = NULL);

	bool		RunTheSession(TCpanMidLevScanCbkHandler::TScanPurpose scan_type,
							TMidLevScannerOptions &scan_opts, TMidLevRamLogger &logger, TCondParsAnalysisDataStorage &andb,
							const wchar_t *root_dir, const wchar_t **list_of_fname_exts, const wchar_t *session_name);
protected:

	TGenericConsoleEx		&m_cpan_proc_console;
};

#endif	// Processing_CondParsAnalysis_H


