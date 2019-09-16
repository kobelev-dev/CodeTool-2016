//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Analysis of the conditional parsing structure.
//

#ifndef	TextScan_CondParsDatabase_H
#define	TextScan_CondParsDatabase_H

#ifndef   TextScan_MidScan_H
#include  "TextScan/MidScan.H"
#endif

class TCondParsAnalysisDataStorage;

struct TCpanNameUseInfo
{
	TIdStruct		evt_info;
	DWORD		use_type;
};

typedef TStructsArray<TCpanNameUseInfo, 6, 512> TCpanNameUseInfosArray;

class TCpanMacroNameObject : public TListItem
{
public:

	TCpanMacroNameObject(ID key_id, TKeyType key_type, const wchar_t *persistent_key_name);
		// The destructor is not needed here because all necessary fields have their own destructors.

	const wchar_t		*Name() const { return(m_key_info.key_name); }

protected:

	TKeyInfo						m_key_info;			// Only the key_id, key_type and key_name subfields are relevant here.

	enum TCondParamUseTypeIndex
	{
		cputi_simple_def = 1,
		cputi_complex_def = 2,
		cputi_undef_stmt = 3,

		cputi_arithm_use = 4,
		cputi_defop_use = 5,
		cputi_litconst_use = 6,
	};

	TCpanNameUseInfosArray		m_definitions;
	TCpanNameUseInfosArray		m_cond_param_uses;

	// Classification of the #define/#undef statements.
	int		m_cnt_simple_defs;						// Macrodefinition without params and its body consists of a single number or a charconst.
	int		m_cnt_complex_defs;					// All other types of macrodefinitions.
	int		m_cnt_undef_stmts;						// The number of #undef statements.

	// Classification of the cond pars param uses.
	int		m_cnt_literal_const_uses;				// Param was used as a literal const (only for numbers and char consts).
	int		m_cnt_arithm_operand_uses;			// The name was used in arithmetic operation.
	int		m_cnt_def_operation_uses;				// Status of the name definition was checked (defined/undefined).

	bool		m_category_assigned;

protected:

	// This comparison function is needed for sorting the list of the macro name objects.
	static int __cdecl CompareNames(void *ctx, const TListItem **pp1, const TListItem **pp2)
	{
		TCpanMacroNameObject &p1 = **((TCpanMacroNameObject**)pp1);
		TCpanMacroNameObject &p2 = **((TCpanMacroNameObject**)pp2);

		return(wcscmp(p1.m_key_info.key_name, p2.m_key_info.key_name));
	}

	// This comparison function is needed for sorting the list of the macro name objects.
	static int __cdecl CompareNumRedefs(void *ctx, const TListItem **pp1, const TListItem **pp2)
	{
		TCpanMacroNameObject &p1 = **((TCpanMacroNameObject**)pp1);
		TCpanMacroNameObject &p2 = **((TCpanMacroNameObject**)pp2);

		if (p1.m_definitions.NumItems() > p2.m_definitions.NumItems())
			return(1);
		if (p1.m_definitions.NumItems() < p2.m_definitions.NumItems())
			return(-1);


		// The count of definition/undefinition stmts is the same. Compare the names.
		return(wcscmp(p1.m_key_info.key_name, p2.m_key_info.key_name));
	}

private:

	TCpanMacroNameObject	*m_next_same_hash_object;

	friend class TCondParsAnalysisDataStorage;
	friend class TCpanMidLevScanCbkHandler;
	friend class TCpanDefnListsIntersectionHelper;
	friend class TCpanCondParamsClassificationHelper;
	friend class TCpanProcessor;
};

class TCondParsAnalysisDataStorage : public THighLevelDataStorage
{
public:

	TCondParsAnalysisDataStorage() { memset(m_hash_chain_entries, 0, sizeof(m_hash_chain_entries)); }
	~TCondParsAnalysisDataStorage() { m_macro_names.DeleteAll(); }

	virtual void	ResetStorage() { m_macro_names.DeleteAll(); memset(m_hash_chain_entries, 0, sizeof(m_hash_chain_entries)); }
	virtual THighLevelDatabase	*AllocateParsSessDatabase(ID parsing_id) { assert(FALSE); return(NULL); }

	TCpanMacroNameObject	*FindNameObject(ID key_id);
	TCpanMacroNameObject	*FindOrCreateNameObject(TKeyInfo &key_info);

	int GetNumMacroNames() const
	{
		return(m_macro_names.NumItems());
	}

	int GetNumObjectsWithDefns() const
	{
		int num = 0;
		TCpanMacroNameObject *name = (TCpanMacroNameObject*)m_macro_names.GetFirst();
		while (name != NULL)
		{
			if (name->m_definitions.NumItems() > 0)
				num++;
			name = (TCpanMacroNameObject*)m_macro_names.GetNext(name);
		}
		return(num);
	}

	bool		SortNamesAlphabetically() { return(m_macro_names.QuickSort(this, TCpanMacroNameObject::CompareNames)); }
	bool		SortNamesByNumRedefs() { return(m_macro_names.QuickSort(this, TCpanMacroNameObject::CompareNumRedefs)); }

protected:

	enum { NUM_MNO_BUCKETS = 128 };

	TList							m_macro_names;								// Instances of TCpanMacroNameObject.
	TCpanMacroNameObject		*m_hash_chain_entries[NUM_MNO_BUCKETS];

	TStringsDict					m_key_names_dict;

	friend class TCpanProcessor;
	friend class TCpanDefnListsIntersectionHelper;
	friend class TCpanCondParamsClassificationHelper;
};

class TCpanCondParsStatement : public TListItem
{
public:
		TCpanCondParsStatement(PrCondParsingSttChange *info);

protected:

	TPreprocToken				m_stmt_type;

	TSourceArea					m_stmt_area;
	TSourceArea					m_expr_area;

	ID							m_scan_expr_error;
	TCondExprScanningDetails		m_scan_expr_details;

	TCondExprTreeItem			*m_expr_code;						// This is an unsimplified expression.
};

class TCondParsInfoDatabase : public THighLevelDatabase
{
public:
		~TCondParsInfoDatabase() { ClearDatabase(); }

	virtual void	ClearDatabase() { m_cond_pars_statements.DeleteAll(); }

protected:

	TList				m_cond_pars_statements;

	friend class TCpanMidLevScanCbkHandler;
};

class TCpanMidLevScanCbkHandler : public TMidLevScanCbkHandlerProxy
{
public:
		TCpanMidLevScanCbkHandler(TMidLevScanCbkHandler *cbk_handler = NULL, TGenericConsoleEx *console = NULL)
				: TMidLevScanCbkHandlerProxy(cbk_handler, console) { ResetAnalysisTask(); }

	enum TScanPurpose
	{
		scp_none,
		scp_macro_defns_list,
		scp_cond_pars_params,
		scp_cond_pars_simplif,
	};

	void		ResetAnalysisTask() { m_curr_task = scp_none; m_andb_storage = NULL; m_cpsimp_storage = NULL; }
	void		SetupAnalysisTask(TScanPurpose new_task, TCondParsAnalysisDataStorage *andb) { m_curr_task = new_task; m_andb_storage = andb; m_cpsimp_storage = NULL; }
	void		SetupAnalysisTask(TScanPurpose new_task, TCondParsInfoDatabase *cpdb) { m_curr_task = new_task; m_andb_storage = NULL; m_cpsimp_storage = cpdb; }

public:

	bool		StartParsing(PrInitParsingInfo1 &init_info);
	void		FinalizeParsing(TMidLevPrepOptions *prep_opts, TMidLevScannerOptions *scan_opts, TMidLevParsingSummary &summary, int final_parsing_status);

	void		MacroLibraryChangeNotification(PrMacroLibraryChange *info);
	void		CondParsSttChangeNotification(PrCondParsingSttChange *info);
	void		ParsingErrorNotification(PrErrorWarningEvent *info) { }
	void		CondParamUseNotification(PrCondParamUseEvent *info);

protected:

	TScanPurpose						m_curr_task;

	TCondParsAnalysisDataStorage		*m_andb_storage;
	TCondParsInfoDatabase				*m_cpsimp_storage;
										// These storages are external properties and they are not owned by this object.
};

#endif	// TextScan_CondParsDatabase_H


