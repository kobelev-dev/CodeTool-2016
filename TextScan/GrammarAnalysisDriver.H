//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	TextScan_GrammarAnalysisDriver_H
#define	TextScan_GrammarAnalysisDriver_H

#ifndef   TextScan_GrammarAnalysis_H
#include  "TextScan/GrammarAnalysis.H"
#endif
#ifndef   TextScan_GrammarAnalysisXml_H
#include  "TextScan/GrammarAnalysisXml.H"
#endif
#ifndef   Common_XmlReader_H
#include  "Xml/XmlReader.H"
#endif
#ifndef   Common_XmlWriter_H
#include  "Xml/XmlWriter.H"
#endif

// -------------------------------------------------------------------------

class TGrammarConflictAnalysisDriver : public TBasicGrammarAnalysisHelper
{
public:

			TGrammarConflictAnalysisDriver(TGenericConsoleEx &console, DWORD tracing_mask, TGrammar *root_grammar);
					//
					//  The passed grammar should be not NULL and it should be in the full success state. This class is not checking this,
					//  but the TGrammarConflictAnalysisHelper class will call HardAssert if this is not so.
					//
					//  It is ok to pass grammars that do not have conflicts. In this case the core analysis helper will do nothing and it will
					//  exit with success.
					//
					//  Passed grammar may already contain some anaysis results. If user will select to start new processing, then
					//  processing will start from the data, that is already present in the grammar. If user will continue existing processing,
					//  then situation will depend on the contents of the analysis database. Records from the database will overwrite
					//  data in the grammar. If there will be no database record for some particular igrammar, iconflict, iaction, then
					//  processing will continue from whatever is present in the grammar.
					//

	void		SetAnalysisDirectory(const wchar_t *analysis_database_directory);
					//
					//  It is ok to pass relative file name as a param of this method. It will be converted into the absolute path using
					//  GetFullPathNameW() function. This directory should be already existing. Otherwise DoConflictsProcessing() will
					//  issue a warning message and will do nothing.
					//

	bool		DeleteAnalysisFiles();
	void		DoConflictsProcessing(int cnt_tasks = 0);

	virtual	bool		PrepareAnalysisTaskAndProps(TGrammarConflictAnalysisTask &task, TGrammarConflictAnalysisProps &props, bool prepare_props) { task.SetDefaultValues(); if (prepare_props == TRUE) props.SetDefaultValues(); return(TRUE); }
						// This method is called when the new processing is started. This allows derived class to set the options as needed.
						// Later on these props are stored in the XML file and processing is picking up these props for continuation.

	static	wchar_t	g_AnalysisDatabaseFileName[];
						// Short name of the file without backslashes in front of it.

protected:

	bool		DoConflictsProcessingInternal(int cnt_tasks);

	void		PrepareDatabaseFileName(TFileNameBuffer &fname_buff);
	void		PrepareSavedAnalysisStateFileName(TFileNameBuffer &fname_buff, int igrammar, int iconflict, int iaction,
													TConflictingActionAnalysisResults::TGenResult gres, TFileNameBuffer &fname_buff_alt);

	HANDLE	LoadDatabase(int igrammar_to_preserve, int iconflict_to_preserve, int iaction_to_preserve);

	bool		SaveDatabase(HANDLE hOpenedFile);
				// This method always closes the passed opened file handle regardless if saving succeeds or not.

	bool		LoadSavedAnalysisState(TGrammarConflictAnalysisHelper &data, int igrammar, int iconflict, int iaction, __int64 num_steps);
	bool		SaveCurrentAnalysisState(TGrammarConflictAnalysisHelper &data);

	bool		StoreCurrentGrammar();
				// The grammar tables are stored into the analysis database directory.

	enum TSelectActionRes
	{
		sar_no_action,
		sar_error_selecting,
		sar_error_marking,
		sar_start_as_new,
		sar_continue,
	};

	struct TAnalysisDatabaseAndTaskStatus
	{
		int			m_num_scheduled_actions;
		int			m_num_started_actions;				// The number of actions that have saved a processing state or that are currentrly running.
		int			m_num_started_no_paths;			// This field contains a total number of running and finished actions that have not discovered paths.
		int			m_num_complete_actions;			// The mumber of conflicting actions where processing terminated either with success or with an error.

		int			m_cnt_running;
		int			m_cnt_in_problem;
		__int64		m_cnt_too_deep_truncs;
		__int64		m_cnt_partial_iter_aborts;
						// These counters reflect the state of the actions that are scheduled for processing,
						// not the state of all conflicting actions in the hierarchy of grammars.

		void Clear() { memset(this, 0, sizeof(TAnalysisDatabaseAndTaskStatus)); }
	};

	TSelectActionRes	FindBestConflictingAction(int &igrammar, int &iconflict, int &iaction, __int64 &num_steps_in_selected_action, TAnalysisDatabaseAndTaskStatus &summary);
						//
						//  Methos finds and returns either a non started action with the lowest igrammar/iconflict/iaction indexes or,
						//  if all actions are already started, action with the smallest number of carried out iteration steps.
						//

	bool		FindBestConflictingActionInGrammar(int igrm_prm, int &igrammar, int &iconflict, int &iaction,
													__int64 &min_steps, int &num_paths_at_min_steps, TAnalysisDatabaseAndTaskStatus &summary);
	bool		FindBestConflictingActionInConflict(TGrammar *grm, int igrm_prm, int icon_prm, int iact_prm, int &igrammar, int &iconflict, int &iaction,
													__int64 &min_steps, int &num_paths_at_min_steps, TAnalysisDatabaseAndTaskStatus &summary);
	void		EvaluateConflictingActionProps(TGrammarConflict &cfct, int igrm_prm, int icon_prm, int iact_prm, int &igrammar, int &iconflict, int &iaction,
													__int64 &min_steps, int &num_paths_at_min_steps, TAnalysisDatabaseAndTaskStatus &summary);

	bool		MarkConflictingAction(int igrammar, int iconflict, int iaction, TConflictingActionAnalysisResults::TGenResult gres);

protected:

	GUID							m_database_guid;
	TGrammarConflictAnalysisProps		m_analysis_props;

	bool								m_save_grammar_tables;
										// When the value of this flag is TRUE, the grammar tables are saved after each step of analysis.

	TGrammarConflictAnalysisTask		m_analysis_task;
										// The analysis task is not stored in the database. It is picked up using the virtual method every
										// time the analysis driver starts.

	TGrammar						*m_grammar;
	TFileNameBuffer					m_database_directory;

	friend class TGrammarConflictsAnalysisStartupDialog;
	friend class TGrammarAnalysisConsole;
};

#endif	// TextScan_GrammarAnalysisDriver_H


