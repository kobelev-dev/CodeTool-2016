//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	Processing_RefactoringEngine_H
#define	Processing_RefactoringEngine_H

#ifndef   Common_HashTable_H
#include  "Common/HashTable.H"
#endif
#ifndef   Common_GenericConsole_H
#include  "Common/GenericConsole.H"
#endif
#ifndef   TextScan_CoreTypes_H
#include  "TextScan/CoreTypes.H"
#endif

// --------------  Processing request  -----------------------

class TRefactoringWorkItem : public TListItem
{
public:

	TRefactoringWorkItem(TSourceArea &remove_area, const wchar_t *replace_data = NULL)
				{ m_remove_area = remove_area; m_new_data = (wchar_t*)replace_data; }

	TSourceArea			m_remove_area;		// Area to remove may have zero length. In any case
												// it should contain a valid offset of the place to change.
	wchar_t				*m_new_data;			// The object is not owning this data.

	static int __cdecl CompareWorkItems(void *ctx, const TListItem **item1, const TListItem **item2)
	{
		TRefactoringWorkItem *i1 = *((TRefactoringWorkItem**)item1);
		TRefactoringWorkItem *i2 = *((TRefactoringWorkItem**)item2);
		assert(i1->m_remove_area.file_info == i2->m_remove_area.file_info);

		if (i1->m_remove_area.area_beg > i2->m_remove_area.area_beg)
			return(1);
		if (i1->m_remove_area.area_beg < i2->m_remove_area.area_beg)
			return(-1);

		// The situation is bogus.
		return(0);
	}
};

enum TRefactoringBackupRestoreMode
{
	bkrm_none,
	bkrm_file_copy_immediate,		// The files are copied to the backup directory before applying the work items.
	bkrm_file_copy_with_cmd,		// The files are copied to the backup directory using the generated cmd file.
	bkrm_version_control,			// The source code version control system is used (Perforce commands).
};

class TBasicRefactoringEngine
{
public:		//
			//  The refactoring engine serves 2 relatively independent functions:
			//
			//	   1.	It holds info about the whole subtree that is being processed. It generates backup copies, names
			//		of the destination files, etc.
			//
			//	   2.	It keeps the list of work items that should be applied to the SINGLE FILE. After finishing creating the list
			//		of work items for each file the changes should be applied and the list of changes should be reset.
			//

	TBasicRefactoringEngine(TGenericConsole &cons, bool apply_changes_phase,
								const wchar_t *source_root, const wchar_t *destination_root = NULL,
								TRefactoringBackupRestoreMode bkrs_mode = bkrm_none, const wchar_t *backup_root = NULL);
			// When the destination root is NULL, this means that it is identical to the source root.
			// The backup root is checked only when the backup mode is one of bkrm_file_copy_*.

	~TBasicRefactoringEngine() { m_work_to_do_list.DeleteAll(); }

	bool		IsPrepareChangesPhase() const { return(m_apply_changes_phase == FALSE); }
	bool		IsApplyChangesPhase() const { return(m_apply_changes_phase == TRUE); }

	int		GetLocalErrorsCount() const { return(m_local_errors_count); }
				// There is no way to reset the count of errors. The object should be reinstantiated.

	void		ClearWorkItemsList() { m_work_to_do_list.DeleteAll(); }
	bool		AddWorkItem(TSourceArea &remove_area, const wchar_t *replace_data = NULL);
				// Current implementation expects that all wok items describe changes in the same file.

	bool		WorkItemsPresent() const { return(m_work_to_do_list.IsEmpty() == FALSE); }

	bool		ApplyWorkItemsAndSaveTheFile();
				// This method picks up the name of the file from the first available work item and expects
				// that all work items are targeted to the same file because this point is explicitly checked in
				// the AddWorkItem() method.

	bool		FinalizeMakingChanges();
	void		ReportRefactoringError(const wchar_t *format, ...);

protected:

	void		PrintFinalMessage(const wchar_t *main_msg, WORD screen_highlight_attr);

	bool		PrepareSrcFileName(TFileNameBuffer &buffer, const wchar_t **psrc_fname);
	bool		ExtractSrcSubtreePath(TFileNameBuffer &buffer, const wchar_t *src_fname);
	bool		PrepareBackupFileName(TFileNameBuffer &buffer, const wchar_t *src_fname);
	bool		PrepareDestinationFileName(TFileNameBuffer &buffer, const wchar_t *src_fname);

	int		SearchForBackslash(const wchar_t *directory_name, int inx_start);
	bool		CheckCreateDirectoryForFile(const wchar_t *src_fname);

protected:

	TGenericConsole					&m_engine_console;

	bool								m_apply_changes_phase;
	int								m_local_errors_count;

	TFileNameBuffer					m_source_root;
	TFileNameBuffer					m_destination_root;

	TRefactoringBackupRestoreMode		m_bkrs_mode;
	TFileNameBuffer					m_backup_root;
	TStringsDict						m_backup_dirs_dict;

	TDestinationFile					m_files_open_cmd;					// Cmd file that either checks alll affected files out of the version
																		// control system or copies them to the backup directory.
	TDestinationFile					m_files_revert_cmd;					// Cmd file that either reverts all affected files back to their original state
																		// in the version control system or restores them from the backup directory.

	TList								m_work_to_do_list;					// List of TRefactoringWorkItem objects.
	TStringsDict						m_replace_data_dict;
};

#endif	// Processing_RefactoringEngine_H


