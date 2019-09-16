//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/FormatNumber.H"
#include  "Processing/RefactoringEngine.H"

// -----------------------------------------------------------------------------
//	===================  TBasicRefactoringEngine  =====================
// -----------------------------------------------------------------------------

TBasicRefactoringEngine::TBasicRefactoringEngine(TGenericConsole &cons, bool apply_changes_phase, const wchar_t *source_root, const wchar_t *destination_root,
												TRefactoringBackupRestoreMode bkrs_mode, const wchar_t *backup_root)
		: m_engine_console(cons)
{
	m_apply_changes_phase = apply_changes_phase;
	m_local_errors_count = 0;

	if (m_source_root.Append(source_root) == FALSE || TPathHelper::EnsureTrailingSlash(m_source_root) == FALSE)
		ReportRefactoringError(L"Unable to init the sources root variable.");
	if (destination_root != NULL && (m_destination_root.Append(destination_root) == FALSE || TPathHelper::EnsureTrailingSlash(m_destination_root) == FALSE))
		ReportRefactoringError(L"Unable to init the destination root variable.");

	TPathHelper::ConvertToBackSlashes(m_source_root);
	TPathHelper::ConvertToBackSlashes(m_destination_root);

	m_bkrs_mode = bkrs_mode;
	if (bkrs_mode == bkrm_file_copy_immediate || bkrs_mode == bkrm_file_copy_with_cmd)
	{
		assert(TPathHelper::IsEmptyPath(backup_root) == FALSE);
		if (m_backup_root.Append(backup_root) == FALSE || TPathHelper::EnsureTrailingSlash(m_backup_root) == FALSE)
			ReportRefactoringError(L"Unable to init the backup root variable.");

		TPathHelper::ConvertToBackSlashes(m_backup_root);
	}

	// Prepare the cmd files only if the source directory is existing. If the source directory
	// is not existing, processing will fail in any case.
	if (TPathHelper::IsPathADir(m_source_root) == TRUE)
	{
		// Check open the cmd files.
		int src_root_name_len = m_source_root.NumItems();

		bool files_open_needed = FALSE;
		if (apply_changes_phase == FALSE && (bkrs_mode == bkrm_file_copy_with_cmd || bkrs_mode == bkrm_version_control))
		{
			// The backup preparation script is needed.
			files_open_needed = TRUE;
			if (m_source_root.Append(L"files_open.cmd") == FALSE)
			{
				ReportRefactoringError(L"Unable to prepare the \"files_open.cmd\" file name.");
			}
			else
			{
				// The name of the file is prepared.
				m_files_open_cmd.SetFileName(m_source_root);

				if (m_files_open_cmd.PrepareDiskFile() == FALSE)
					ReportRefactoringError(L"Unable to open the \"%s\" for writitng.", m_source_root.DataPtr());
				else m_files_open_cmd.WriteLine(L"echo off");
			}

			m_source_root.TruncateTo(src_root_name_len);
		}

		// Note that when the backup mode is bkrm_file_copy_immediate, the processing phase sould be ignored.
		if (files_open_needed == TRUE || bkrs_mode == bkrm_file_copy_immediate)
		{
			// The revert changes script is needed.
			if (m_source_root.Append(L"files_revert.cmd") == FALSE)
			{
				ReportRefactoringError(L"Unable to prepare the \"files_revert.cmd\" file name.");
			}
			else
			{
				// The name of the file is prepared.
				m_files_revert_cmd.SetFileName(m_source_root);

				if (m_files_revert_cmd.PrepareDiskFile() == FALSE)
					ReportRefactoringError(L"Unable to open the \"%s\" for writitng.", m_source_root.DataPtr());
				else m_files_revert_cmd.WriteLine(L"echo off");
			}

			m_source_root.TruncateTo(src_root_name_len);
		}
	}
}

bool TBasicRefactoringEngine::AddWorkItem(TSourceArea &remove_area, const wchar_t *replace_data)
{
	// Check the file info in the passed param.
	if (remove_area.file_info == NULL || remove_area.file_info->info.file_name == NULL)
	{
		ReportRefactoringError(L"Info about the file to process is missing.");
		return(FALSE);
	}
	else if (m_work_to_do_list.IsEmpty() == FALSE)
	{
		// This is not the first work item. Ensure that the current work item updates the same file.
		TRefactoringWorkItem *first_item = (TRefactoringWorkItem*)m_work_to_do_list.GetFirst();
		if (remove_area.file_info != first_item->m_remove_area.file_info)
		{
			ReportRefactoringError(L"Work items for different files are not supported.");
			return(FALSE);
		}
	}

	// Check if the passed work item is empty or not.
	if (remove_area.area_len <= 0 && (replace_data == NULL || *replace_data == 0))
	{
		ReportRefactoringError(L"The passed work item is empty.");
		return(TRUE);
	}

	// Update the cmd files if needed.
	if (m_work_to_do_list.IsEmpty() == TRUE && (m_apply_changes_phase == FALSE || m_bkrs_mode == bkrm_file_copy_immediate))
	{
		// This is the first work item for the current file.
		TFileNameBuffer src_fn_buffer;
		const wchar_t *curr_file_name = remove_area.file_info->info.file_name;
		if (PrepareSrcFileName(src_fn_buffer, &curr_file_name) == FALSE)
			return(FALSE);

		switch (m_bkrs_mode)
		{
			case bkrm_file_copy_immediate:
			case bkrm_file_copy_with_cmd:
					{
						// Selected backup procedure expects simple files copying.
						TFileNameBuffer backup_fname;
						if (PrepareBackupFileName(backup_fname, curr_file_name) == FALSE)
						{
							// An error is already reported.
							break;
						}

						if (m_bkrs_mode == bkrm_file_copy_with_cmd)
						{
							// Check if copying into this directory was already issued or not yet.
							TFileNameBuffer curr_file_backup_dir;
							if (TPathHelper::GetDirectoryName(curr_file_backup_dir, backup_fname) == FALSE)
							{
								ReportRefactoringError(L"Error preparing the backup direcrtory name for \"%s\".", curr_file_name);
								break;
							}

							if (m_backup_dirs_dict.FindString(curr_file_backup_dir) == NULL)
							{
								// Use a simplified approach where the create directory command can be issued several times.
								m_files_open_cmd.WriteLine();
								m_files_open_cmd.WriteFmtLine(L"md %s", curr_file_backup_dir.DataPtr());
								m_backup_dirs_dict.RegisterStr(curr_file_backup_dir);
							}

							// Add the copy command to the script.
							m_files_open_cmd.WriteFmtLine(L"copy %s %s", curr_file_name, backup_fname.DataPtr());
						}

						// Both file copy modes need the restore command.
						m_files_revert_cmd.WriteFmtLine(L"copy %s %s", backup_fname.DataPtr(), curr_file_name);
					}
					break;

			case bkrm_version_control:
					{
						// Backup procedure uses the source code version control system.
						m_files_open_cmd.WriteFmtLine(L"call p4 edit %s", curr_file_name);
						m_files_revert_cmd.WriteFmtLine(L"call p4 revert %s", curr_file_name);
					}
					break;
		}
	}

	const wchar_t *new_data = NULL;
	if (replace_data != NULL && *replace_data != 0)
	{
		new_data = m_replace_data_dict.RegisterStr(replace_data);
		if (new_data == NULL)
		{
			ReportRefactoringError(L"Unable to store the replace data for the work item.");
			return(FALSE);
		}
	}

	TRefactoringWorkItem *wi = new TRefactoringWorkItem(remove_area, new_data);
	if (wi == NULL)
	{
		ReportRefactoringError(L"Unable to create the work item.");
		return(FALSE);
	}

	// Allocation succeeded. Add item to the list to do.
	m_work_to_do_list.AppendItem(wi);
	return(TRUE);
}

bool TBasicRefactoringEngine::ApplyWorkItemsAndSaveTheFile()
{
	assert(m_apply_changes_phase == TRUE || m_bkrs_mode == bkrm_file_copy_immediate);
	if (WorkItemsPresent() == FALSE)
	{
		// There is nothing to do.
		return(TRUE);
	}
	else if (m_local_errors_count > 0)
	{
		// Do not apply changes in the case of error.
		return(TRUE);
	}

	// Pick up the info about the current file.
	TRefactoringWorkItem *first_work_item = (TRefactoringWorkItem*)m_work_to_do_list.GetFirst();
	const wchar_t *curr_file_name = first_work_item->m_remove_area.file_info->info.file_name;
	long curr_file_len = first_work_item->m_remove_area.file_info->info.file_len;

	// Fix the forward slashes in the name of the file if any.
	TFileNameBuffer src_fn_buffer;
	if (PrepareSrcFileName(src_fn_buffer, &curr_file_name) == FALSE)
		return(FALSE);

	if (m_bkrs_mode == bkrm_file_copy_immediate)
	{
		// Execute the backup procedure.
		TFileNameBuffer backup_fname;
		if (PrepareBackupFileName(backup_fname, curr_file_name) == FALSE || CheckCreateDirectoryForFile(backup_fname) == FALSE)
		{
			// An error is already reported.
			return(FALSE);
		}

		// Call the file copy function.
		if (::CopyFileW(curr_file_name, backup_fname, FALSE) == FALSE)
		{
			DWORD err_code = ::GetLastError();
			ReportRefactoringError(L"Error backing up (Win32Error=%d) the  file \"%s\".", err_code, curr_file_name);
			return(FALSE);
		}
	}

	// Prepare the destination file name where to write the file.
	TFileNameBuffer dest_fname;
	if (PrepareDestinationFileName(dest_fname, curr_file_name) == FALSE || CheckCreateDirectoryForFile(dest_fname) == FALSE)
	{
		// Error is already reported.
		return(FALSE);
	}

	// Ensure that the destination file is either not existing or if it is present, it is not write protected.
	if (TPathHelper::IsPathAFile(dest_fname) == TRUE)
	{
		// The file is existing.
		DWORD attrs = ::GetFileAttributesW(dest_fname);
		assert(attrs != INVALID_FILE_ATTRIBUTES);
		if ((attrs & FILE_ATTRIBUTE_READONLY) != 0)
		{
			ReportRefactoringError(L"The destination file \"%s\" is READ_ONLY.", dest_fname.DataPtr());
			return(FALSE);
		}
	}

	// Sort the work items.
	if (m_work_to_do_list.QuickSort(NULL, TRefactoringWorkItem::CompareWorkItems) == FALSE)
	{
		ReportRefactoringError(L"Error sorting the work items (NumItems=%d) for the file \"%s\".", m_work_to_do_list.NumItems(), curr_file_name);
		return(FALSE);
	}

	// Ensure that the source areas do not overlap.
	long prev_area_end = -1;
	for (TListIter<TRefactoringWorkItem> iter1(m_work_to_do_list); iter1; ++iter1)
	{
		TSourceArea &rm_area = iter1.CurrItem().m_remove_area;
		if (rm_area.AreaBeg() < prev_area_end)
		{
			ReportRefactoringError(L"Areas to process overlap in the file \"%s\".", curr_file_name);
			return(FALSE);
		}
		else if (rm_area.AreaEnd() > curr_file_len)
		{
			ReportRefactoringError(L"Area to process (Offs=%ld, Len=%ld) sticks out in the file \"%s\".", rm_area.area_beg, rm_area.area_len, curr_file_name);
			return(FALSE);
		}

		prev_area_end = rm_area.AreaEnd();
	}

	// Copy the body of the file into the local buffer. The result of allocation will be checked later.
	TTextBuffer<256> body_buff;
	body_buff.Append(first_work_item->m_remove_area.file_info->info.file_body, curr_file_len);

	// Process the work items.
	TListIter<TRefactoringWorkItem> iter2(m_work_to_do_list);
	for (iter2.SeekToLast(); iter2; --iter2)
	{
		TSourceArea &rm_area = iter2.CurrItem().m_remove_area;
		if (rm_area.area_len > 0)
		{
			// Some chars should be removed from the file.
			body_buff.ReleaseItems(rm_area.area_beg, rm_area.area_len);
		}

		// Check if something should be inserted instead.
		wchar_t *new_data = iter2.CurrItem().m_new_data;
		if (new_data != NULL)
		{
			// Insert the string.
			body_buff.Insert(rm_area.area_beg, new_data);
		}
	}

	// Check the result of applying the changes.
	if (body_buff.GetXpndError() == TRUE)
	{
		ReportRefactoringError(L"Low on memory while processing the file \"%s\".", curr_file_name);
		return(FALSE);
	}
	else
	{
		// Save the result to disk. This will destroy contents of the file body buffer.
		if (TFileBodyHelper::SaveAsciiFileBody(dest_fname, body_buff.DataPtr(), body_buff.NumItems()) != ldres_success)
		{
			ReportRefactoringError(L"Error saving the file \"%s\".", dest_fname);
			return(FALSE);
		}
	}

	// Full success.
	return(FALSE);
}

bool TBasicRefactoringEngine::FinalizeMakingChanges()
{
	// Finalize check for errors in the first cmd.
	if (m_files_open_cmd.IsWritingState() == TRUE && m_files_open_cmd.Close() == FALSE)
	{
		ReportRefactoringError(L"Error preparing the file \"%s\".", m_files_open_cmd.FileName());
	}

	// Finalize check for errors in the second cmd.
	if (m_files_revert_cmd.IsWritingState() == TRUE && m_files_revert_cmd.Close() == FALSE)
	{
		ReportRefactoringError(L"Error preparing the file \"%s\".", m_files_revert_cmd.FileName());
	}

	if (m_local_errors_count > 0)
	{
		// The overall session failed.
		wchar_t buff[80];
		swprintf(buff, 80, L"*  Refactoring procedure failed with %d error%s.", m_local_errors_count, PluralSuffix(m_local_errors_count));

		// Do the final trace.
		WORD RedAttr = (FOREGROUND_RED | FOREGROUND_INTENSITY);
		PrintFinalMessage(buff, RedAttr);
		return(FALSE);
	}

	// Success.
	WORD YellowAttr =  (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	PrintFinalMessage(L"*  Refactoring succeeded.", YellowAttr);
	return(TRUE);
}

void TBasicRefactoringEngine::ReportRefactoringError(const wchar_t *format, ...)
{
	if (m_local_errors_count == 0)
		m_engine_console.HandleTrace(L" ");

	// Prepare the message.
	wchar_t buff[2*MAX_PATH+80];
	va_list vargs;
	va_start(vargs, format);
	vswprintf(buff, 2*MAX_PATH, format, vargs);
	va_end(vargs);

	// Send this message to the console and bump the errors count.
	m_engine_console.HandleTrace(buff, TGenericConsole::ErrorHighlightMode);
	m_local_errors_count++;
}

void TBasicRefactoringEngine::PrintFinalMessage(const wchar_t *main_msg, WORD screen_highlight_attr)
{
	m_engine_console.HandleTrace(L"************************************************************************************", screen_highlight_attr);
	m_engine_console.HandleTrace(L"*", screen_highlight_attr);
	m_engine_console.HandleTrace(main_msg, screen_highlight_attr);
	m_engine_console.HandleTrace(L"*", screen_highlight_attr);
	m_engine_console.HandleTrace(L"************************************************************************************", screen_highlight_attr);
	m_engine_console.HandleTrace(L" ");
}

bool TBasicRefactoringEngine::PrepareSrcFileName(TFileNameBuffer &buffer, const wchar_t **psrc_fname)
{
	if (wcschr(*psrc_fname, L'/') != NULL)
	{
		// Passed path contains forward slashes. Fix them.
		buffer.ClearBuffer();
		if (buffer.Append(*psrc_fname) == FALSE)
		{
			ReportRefactoringError(L"Error preparing src path from \"%s\".", *psrc_fname);
			return(FALSE);
		}

		TPathHelper::ConvertToBackSlashes(buffer);
		*psrc_fname = buffer.DataPtr();
	}

	// Success.
	return(TRUE);
}

bool TBasicRefactoringEngine::ExtractSrcSubtreePath(TFileNameBuffer &buffer, const wchar_t *src_fname)
{
	assert(TPathHelper::IsEmptyPath(m_source_root) == FALSE);

	// Ensure that the source file name belongs to the src tree.
	int src_fname_len = wcslen(src_fname);
	if (src_fname_len <= m_source_root.NumItems() || wcsncmp(m_source_root, src_fname, m_source_root.NumItems()) != 0)
	{
		ReportRefactoringError(L"The source file \"%s\" does not belong to the source tree \"%s\".", src_fname, m_source_root.DataPtr());
		return(FALSE);
	}

	buffer.ClearBuffer();
	if (buffer.Append(src_fname+m_source_root.NumItems()) == FALSE)
	{
		ReportRefactoringError(L"Error extracting the subtree path from \"%s\".", src_fname);
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TBasicRefactoringEngine::PrepareBackupFileName(TFileNameBuffer &buffer, const wchar_t *src_fname)
{
	// Once this method is called, the backup root should be not empty.
	assert(TPathHelper::IsEmptyPath(m_backup_root) == FALSE);

	TFileNameBuffer src_subtree_path;
	if (ExtractSrcSubtreePath(src_subtree_path, src_fname) == FALSE)
		return(FALSE);

	// Construct the file name out of two parts.
	buffer.ClearBuffer();
	if (buffer.Append(m_backup_root) == FALSE || buffer.Append(src_subtree_path) == FALSE)
	{
		ReportRefactoringError(L"Error preparing backup file name for \"%s\".", src_fname);
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TBasicRefactoringEngine::PrepareDestinationFileName(TFileNameBuffer &buffer, const wchar_t *src_fname)
{
	buffer.ClearBuffer();
	if (m_destination_root.NumItems() == 0)
	{
		// The destination file name is identical to the source file name.
		buffer.Append(src_fname);
	}
	else
	{
		TFileNameBuffer src_subtree_path;
		if (ExtractSrcSubtreePath(src_subtree_path, src_fname) == FALSE)
			return(FALSE);

		// Construct the file name out of two parts.
		buffer.Append(m_destination_root);
		buffer.Append(src_subtree_path);
	}

	if (buffer.GetXpndError() == TRUE)
	{
		ReportRefactoringError(L"Error preparing the destination file name for \"%s\".", src_fname);
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

int TBasicRefactoringEngine::SearchForBackslash(const wchar_t *directory_name, int inx_start)
{
	int inx = inx_start;
	while (directory_name[inx] != 0)
	{
		if (directory_name[inx] == L'\\')
			return(inx);

		inx++;
	}

	// The back slash is missing. Return the length of the path.
	return(inx);
}

bool TBasicRefactoringEngine::CheckCreateDirectoryForFile(const wchar_t *src_fname)
{
	TFileNameBuffer file_directory_buffer;
	if (TPathHelper::GetDirectoryName(file_directory_buffer, src_fname) == FALSE)
	{
		ReportRefactoringError(L"Error extracting direcrtory name from the file \"%s\".", src_fname);
		return(FALSE);
	}
	else if (TPathHelper::IsPathADir(file_directory_buffer) == TRUE)
	{
		// The directory is existing. The situation is fine.
		return(TRUE);
	}

	// Some part of the path is not existing.
	TPathHelper::ConvertToBackSlashes(m_source_root);
	int full_dir_name_len = file_directory_buffer.NumItems();

	int first_bksl_inx = SearchForBackslash(file_directory_buffer, 0);
	if (first_bksl_inx >= full_dir_name_len)
	{
		ReportRefactoringError(L"Error extracting the drive name from the file \"%s\".", src_fname);
		return(FALSE);
	}

	int fragm_length = SearchForBackslash(file_directory_buffer, first_bksl_inx+1);
	if (fragm_length >= full_dir_name_len)
	{
		ReportRefactoringError(L"Error extracting the root direcrtory name from the file \"%s\".", src_fname);
		return(FALSE);
	}

	// Testing shows that ::CreateDirectoryW() can create only one subdirectory layer at a time.
	// At the same time this function accepts mixture of forward and back slashes.
	for(;;)
	{
		file_directory_buffer[fragm_length] = 0;
		if (TPathHelper::IsPathADir(file_directory_buffer) == FALSE && ::CreateDirectoryW(file_directory_buffer, NULL) == FALSE)
		{
			DWORD err_code = ::GetLastError();
			ReportRefactoringError(L"Error creating directory. Win32 error: %d.", err_code);
			return(FALSE);
		}

		// This layer of subdirs was already existing or its creation succeeded.
		if (fragm_length >= full_dir_name_len)
			break;

		file_directory_buffer[fragm_length] = L'\\';
		fragm_length = SearchForBackslash(file_directory_buffer, fragm_length+1);
	}

	// Success.
	assert(TPathHelper::IsPathADir(file_directory_buffer) == TRUE);
	return(TRUE);
}


