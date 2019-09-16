//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>
#include  <conio.h>

#include  "TextScan/GrammarDefinitionParser.H"
#include  "Processing/IdentifiersHelper.H"
#include  "Processing/FixGrammarSources.H"

class TGrammarNonTerminalNamesFixer : public TLexemaDetailsChecker
{
public:

	TGrammarNonTerminalNamesFixer(TSrcDetailsIterationHandler &handler, TBasicRefactoringEngine &changes_engine)
			: TLexemaDetailsChecker(handler), m_changes_engine(changes_engine) { m_cnt_modified = 0; }

	virtual int				NumDetailTypes() { return(1); }
	virtual const wchar_t	*GetDetailName(int idetail)	   { return(L"Identifiers modified"); }
	virtual int				GetNumDetails(int idetail)	   { return(m_cnt_modified); }

	virtual void ProcessLexema(TLexema &prev_lex, TLexema &lex)
	{
		if (lex.type != ltx_name)
			return;

		// The names of the sections should not be modified.
		if (GetGrammarDefnSectionType(lex.str_value) != grsc_none)
			return;

		// The names of the lexema types should not be modified.
		if (lex.str_value.StartsWith(L"ltx_") == TRUE || lex.str_value.StartsWith(L"spr_") == TRUE || lex.str_value.StartsWith(L"opr_") == TRUE)
			return;
		if (lex.str_value.StartsWith(L"clg_") == TRUE || lex.str_value.StartsWith(L"cpp_") == TRUE)
			return;

		// Preserve some of the terminal symbol names.
		if (lex.IsName(L"number") == TRUE || lex.IsName(L"charconst") == TRUE)
			return;
		if (lex.IsName(L"string") == TRUE || lex.IsName(L"identifier") == TRUE)
			return;

		TNameBuffer updated_name_buffer;
		if (TIdentifiersHelper::ConvertUnderscoredLowsToCaps(updated_name_buffer, lex.str_value) == FALSE)
		{
			// This should be a rare case.
			m_changes_engine.ReportRefactoringError(L"Unable to generate modified identifier.");
		}
		else if (lex.str_value != updated_name_buffer)
		{
			// Conversion has generated something different.
			if (m_changes_engine.AddWorkItem(lex.origin.src_area, updated_name_buffer) == TRUE)
			{
				// Something is modified.
				m_iteration_handler.ReportSilentDetail();
				m_cnt_modified++;
			}
		}
	}

	virtual void FinalizeFileProcessing()
	{
		if (m_changes_engine.WorkItemsPresent() == TRUE && m_changes_engine.GetLocalErrorsCount() == 0)
		{
			// Current file contains scheduled changes and there are no errors detected so far.
			m_changes_engine.ApplyWorkItemsAndSaveTheFile();
		}

		// Prepare for processing the next file.
		m_changes_engine.ClearWorkItemsList();
	}
protected:

	TBasicRefactoringEngine		&m_changes_engine;

	int							m_cnt_modified;
};

// -----------------------------------------------------
//   =============  Functions  ==================
// -----------------------------------------------------

void DoGrammarSourcesConversion(const wchar_t *grammars_root_dir, const wchar_t *grm_to_process_sub_dir, const wchar_t *dest_subdir)
{
	// Prepare the exact source directory name.
	wchar_t source_dir_buffer[2*MAX_PATH];
	swprintf(source_dir_buffer, 2*MAX_PATH, L"%s\\%s", grammars_root_dir, grm_to_process_sub_dir);

	// Allocate the backup vars.
	TRefactoringBackupRestoreMode bk_mode = bkrm_none;
	wchar_t backup_dir_buffer[2*MAX_PATH];
	wchar_t *backup_dir_ptr = NULL;

	// Prepare backup and dest dir manes.
	wchar_t destination_dir_buffer[2*MAX_PATH];
	if (dest_subdir != NULL)
	{
		// Do not make any backup. Simply place results into the destination directory.
		swprintf(destination_dir_buffer, 2*MAX_PATH, L"%s\\%s", grammars_root_dir, dest_subdir);
		dest_subdir = destination_dir_buffer;
	}
	else
	{
		// The data should be backed up to the backup directory.
		TRefactoringBackupRestoreMode bk_mode = bkrm_file_copy_immediate;
		swprintf(backup_dir_buffer, 2*MAX_PATH, L"%s\\Backup\\%s", grammars_root_dir, grm_to_process_sub_dir);
		backup_dir_ptr = backup_dir_buffer;
		dest_subdir = source_dir_buffer;
	}

	// Allocate the changes engine.
	TGenericConsoleEx cons;
	TBasicRefactoringEngine changes_engine(cons, TRUE, source_dir_buffer, dest_subdir, bk_mode, backup_dir_ptr);

	// Allocate the checker infrastructure objects.
	TSrcDetailsIterationHandler handler;
	TGrammarNonTerminalNamesFixer names_fixer(handler, changes_engine);

	// File name extensions to process (all file types should have rscn_c_cpp raw scanning type).
	const wchar_t *list_of_fname_extensions[] =
	{
		L".GRA",
		NULL,
	};

	// Do the job.
	TBatchParsingSummary sumry;
	handler.ProcessSubtree(cons, source_dir_buffer, FALSE, list_of_fname_extensions, FALSE, &sumry);
	handler.PrintSummary(cons);

	// Close the backup/restore cmd files if needed and trace the final messages.
	changes_engine.FinalizeMakingChanges();
}


