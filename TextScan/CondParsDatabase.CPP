//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Conditional parsing structure analysis.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "TextScan/CondParsDatabase.H"

//--------------------------------------------------------------------------
//  =================  TCpanMacroNameObject  =====================
//--------------------------------------------------------------------------

TCpanMacroNameObject::TCpanMacroNameObject(ID key_id, TKeyType key_type, const wchar_t *persistent_key_name)
{
	m_key_info.Clear();
	m_key_info.key_id = key_id;
	m_key_info.key_type = key_type;
	m_key_info.key_name = (wchar_t*)persistent_key_name;

	m_cnt_simple_defs = 0;
	m_cnt_complex_defs = 0;
	m_cnt_undef_stmts = 0;

	m_cnt_literal_const_uses = 0;
	m_cnt_arithm_operand_uses = 0;
	m_cnt_def_operation_uses = 0;

	m_category_assigned = FALSE;
	m_next_same_hash_object = NULL;
}

//--------------------------------------------------------------------------------
//  =================  TCondParsAnalysisDataStorage  =====================
//--------------------------------------------------------------------------------

TCpanMacroNameObject *TCondParsAnalysisDataStorage::FindNameObject(ID key_id)
{
	assert(key_id != 0);

	// Pick up the head of the appropriate single linked list.
	TCpanMacroNameObject *name_info = m_hash_chain_entries[key_id & (NUM_MNO_BUCKETS-1)];

	// Search this list for an identical item.
	while (name_info != NULL)
	{
		if (name_info->m_key_info.key_id == key_id)
			return(name_info);
		name_info = name_info->m_next_same_hash_object;
	}

	// The requested item is not available.
	return(NULL);
}

TCpanMacroNameObject *TCondParsAnalysisDataStorage::FindOrCreateNameObject(TKeyInfo &key_info)
{
	assert(key_info.key_id != 0 && key_info.key_name != NULL);
	TCpanMacroNameObject *info = FindNameObject(key_info.key_id);
	if (info != NULL)
	{
		// An object is already existing. Return it.
		assert(info->m_key_info.key_type == key_info.key_type);
		return(info);
	}

	// Register the name of the key.
	const wchar_t *local_key_name = m_key_names_dict.RegisterStr(key_info.key_name);
	if (local_key_name == NULL)
		return(NULL);

	// Allocate a new record.
	TCpanMacroNameObject *new_info = new TCpanMacroNameObject(key_info.key_id, key_info.key_type, local_key_name);
	if (new_info == NULL)
		return(NULL);

	// Add record to the hashed storage.
	m_macro_names.AppendItem(new_info);
	int inx = key_info.key_id & (NUM_MNO_BUCKETS-1);
	new_info->m_next_same_hash_object = m_hash_chain_entries[inx];
	m_hash_chain_entries[inx] = new_info;

	// Success.
	return(new_info);
}

//--------------------------------------------------------------------------------
//  ===================  TCpanCondParsStatement  ======================
//--------------------------------------------------------------------------------

TCpanCondParsStatement::TCpanCondParsStatement(PrCondParsingSttChange *info)
{
	m_stmt_type = info->stmt_type;
	m_stmt_area = info->hdr.src_area;
	m_expr_area = info->expr_area.src_area;

	m_scan_expr_error = info->scan_expr_error;
	m_scan_expr_details = info->scan_expr_details;

	// Accept the code tree and clear the pointer in the param to indicate that the the code is taken.
	m_expr_code = info->expr_code;
	info->expr_code = NULL;
}

//-------------------------------------------------------------------------------
//  =================  TCpanMidLevScanCbkHandler  =====================
//-------------------------------------------------------------------------------

bool TCpanMidLevScanCbkHandler::StartParsing(PrInitParsingInfo1 &init_info)
{
	assert(m_curr_task != scp_none);

	// For now just call be base class.
	return(TMidLevScanCbkHandlerProxy::StartParsing(init_info));
}

void TCpanMidLevScanCbkHandler::FinalizeParsing(TMidLevPrepOptions *prep_opts, TMidLevScannerOptions *scan_opts, TMidLevParsingSummary &summary, int final_pars_status)
{
	TMidLevScanCbkHandlerProxy::FinalizeParsing(prep_opts, scan_opts, summary, final_pars_status);

	// Kill the file caches to prevent their considerable growth.
	TList disk_files, temp_files;
	m_scanner->ExtractFilesCache(disk_files, temp_files);
	disk_files.DeleteAll();
	temp_files.DeleteAll();
}

void TCpanMidLevScanCbkHandler::MacroLibraryChangeNotification(PrMacroLibraryChange *info)
{
	assert(info->hdr.cn == info->macro->hdr.cn);

	if (m_curr_task == scp_macro_defns_list)
	{
		// This task expects that the analysis data storage is set.
		assert(m_andb_storage != NULL);

		// Create an object for the current name if it is not existing yet.
		TCpanMacroNameObject *name_info = m_andb_storage->FindOrCreateNameObject(info->macro->key_info->data);
		if (name_info != NULL)
		{
			// Update the record with the back ref to the current event.
			TCpanNameUseInfo evt_ident = { GetParsingId(), info->macro->hdr.cn, 0 };
			if (name_info->m_definitions.AppendItem(evt_ident) == FALSE)
				ShowProcessingError(L"TCpanMidLevScanCbkHandler::MacroLibraryChangeNotification: Low on memory (1).");
		}
		else
		{
			ShowProcessingError(L"TCpanMidLevScanCbkHandler::MacroLibraryChangeNotification: Low on memory (2).");
		}
	}
	else if (m_curr_task == scp_cond_pars_params)
	{
		// This task expects that the analysis data storage is set.
		assert(m_andb_storage != NULL);

		// Do not create the object if it is not yet existing.
		TCpanMacroNameObject *name_info = m_andb_storage->FindNameObject(info->macro->key_info->data.key_id);
		if (name_info != NULL)
		{
			TCpanNameUseInfo evt_ident = { GetParsingId(), info->macro->hdr.cn, 0 };

			// Classify the type of definition.
			if (GET_DEFINED_STT(info->macro->def_status) == FALSE)
			{
				name_info->m_cnt_undef_stmts++;
				evt_ident.use_type = TCpanMacroNameObject::cputi_undef_stmt;
			}
			else if (info->macro->scst_body_lex.IsLex() == TRUE)
			{
				name_info->m_cnt_simple_defs++;
				evt_ident.use_type = TCpanMacroNameObject::cputi_simple_def;
			}
			else
			{
				// Once this is definition and it is  not a simple one, this is a complex definition.
				name_info->m_cnt_complex_defs++;
				evt_ident.use_type = TCpanMacroNameObject::cputi_complex_def;
			}

			// Update the record with the back ref to the current event.
			if (name_info->m_definitions.AppendItem(evt_ident) == FALSE)
				ShowProcessingError(L"TCpanMidLevScanCbkHandler::MacroLibraryChangeNotification: Low on memory (3).");

		}
		else
		{
			// Do not execute the default action. This significantly reduces the memory footprint.
			return;
		}
	}

	// Call the default action.
	TMidLevScanCbkHandlerProxy::MacroLibraryChangeNotification(info);
}

void TCpanMidLevScanCbkHandler::CondParsSttChangeNotification(PrCondParsingSttChange *info)
{
	// Call the default action first because processing on the curr layer may modify the info.
	TMidLevScanCbkHandlerProxy::CondParsSttChangeNotification(info);

	if (m_curr_task == scp_cond_pars_simplif)
	{
		assert(m_cpsimp_storage != NULL);

		TCpanCondParsStatement *stmt = new TCpanCondParsStatement(info);
		if (stmt != NULL)
		{
			// Save information about this statement.
			m_cpsimp_storage->m_cond_pars_statements.AppendItem(stmt);
		}
		else
		{
			ShowProcessingError(L"TCpanMidLevScanCbkHandler::CondParsSttChangeNotification: Low on memory (1).");
		}
	}
}

void TCpanMidLevScanCbkHandler::CondParamUseNotification(PrCondParamUseEvent *info)
{
	if (m_curr_task == scp_cond_pars_params)
	{
		// This method expects that the analysis data storage is set.
		assert(m_andb_storage != NULL);

		// This notification is sent for the following types of objects: literal constants (numbers and charconsts),
		// operands of the arithmetic operations (these can be only names, both known and unknown) and operands
		// of the "defined" operation. In all these cases the passed info should contain valid code item and this code
		// item should contain a valid key info field. Ensure this.
		assert(info->param != NULL && info->param->key_info != NULL && info->param->key_info->data.key_name != NULL);

		TCpanMacroNameObject *name_info = m_andb_storage->FindOrCreateNameObject(info->param->key_info->data);
		if (name_info != NULL)
		{
			TCpanNameUseInfo evt_ident = { GetParsingId(), info->hdr.cn, 0 };

			TCondExprTreeItemType param_type = info->param->item_type;
			if (param_type == cecit_constant)
			{
				name_info->m_cnt_literal_const_uses++;
				evt_ident.use_type = TCpanMacroNameObject::cputi_litconst_use;
			}
			else if (param_type == cecit_decorated || param_type == cecit_variable)
			{
				name_info->m_cnt_arithm_operand_uses++;
				evt_ident.use_type = TCpanMacroNameObject::cputi_arithm_use;
			}
			else if (param_type == cecit_name_def || param_type == cecit_variab_def)
			{
				name_info->m_cnt_def_operation_uses++;
				evt_ident.use_type = TCpanMacroNameObject::cputi_defop_use;
			}
			else
			{
				// Other types of param uses should not come here.
				assert(FALSE);
			}

			// Update the record with the back ref to the current event.
			if (name_info->m_cond_param_uses.AppendItem(evt_ident) == FALSE)
				ShowProcessingError(L"TCpanMidLevScanCbkHandler::CondParamUseNotification: Low on memory (1).");
		}
		else
		{
			ShowProcessingError(L"TCpanMidLevScanCbkHandler::CondParamUseNotification: Low on memory (2).");
		}
	}

	// Call the base class.
	TMidLevScanCbkHandlerProxy::CondParamUseNotification(info);
}


