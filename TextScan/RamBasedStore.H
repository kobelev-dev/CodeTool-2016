//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   In memory storage for the parsing events.
//

#ifndef	TextScan_RamBasedStore_H
#define	TextScan_RamBasedStore_H

#ifndef   TextScan_MidStore_H
#include  "TextScan/MidStore.H"
#endif

//  ----------- Database writer object -----------------

//
//  Info about the TEMP file that is stored in the memory based parsing events database.
//  Note that this record is derived from the regular temp file reg event.
//
struct TMidLevRamLogTempFileRecord : public CtvTempFileRegEvent
{
	BYTE			*indexing_data;				// This data field is expected to follow special compressed
												// format and it should have the proper termination element.
};

struct TMidLevRamLogLexSourceEnterEvent : public CtvLexSourceEnterEvent
{
	ID				min_event_id;
	ID				max_event_id;
						// These are min and max event ids of events whose linear area has non empty
						// intersection with the linear area of the lex src enclosure.

	bool				upper_lin_area_events;		// The value of this flag is TRUE if there is an event with an id that belongs to
												// the range of ids between the lex enter and lex exit events and that is located
												// (at least partly) on the linear area outside of this lex source enclosure.

	inline void Clear() { memset(this, 0, sizeof(TMidLevRamLogLexSourceEnterEvent)); }
};

// Array of 128kb buffers with initial capacity of 128 pointers.
typedef TDataBuffersArray<128, 128> TMidLevRamLogBuffsArray;

class TMidLevRamStoreReader;

//
//  The parsing log stores information about one parsing session.
//
//  NB1:	String pointers in all event structures point to strings in the external dictionary of strings.
//		This strings dict is shared between all parsing sessions in the ram based data store.
//
//  NB2:	Parsing log DOES contain an event if a pointer to its body is present in the m_event_ptrs array.
//		This is an official and the only one way to figure this out.
//
class TMidLevRamBasedParsingLog : public TListItem
{
public:
			TMidLevRamBasedParsingLog(TStringsDict *strs_dict);

	bool		SetupHeader(PrInitParsingInfo1 &data, ID pars_id);
	bool		SetupPrepHeaderInfo(PrInitParsingInfo2 &data);
	bool		SetupMainHeaderInfo(PrInitParsingInfo3 &data);
	void		FinalizeHeaderInfo(TMidLevPrepOptions *prep_options, TMidLevScannerOptions *scan_options, TMidLevParsingSummary &data, int final_pars_status);
	void		FinalizeHeaderInfoRawSess(TRawIndexingSessSummary &sumry, TFileInfoItem *mj_src_file = 0);

	bool		AddEvent(CtvParsingEventInfo *pev, long event_length);
				// Each event carries an event_id. In fact, the parser can submit events out of the event ids order.
				// Parsing log cannot guarantee that parser will submit all events with every event_id ever assigned.
				// Plus the submission itself may fail. This means that some events can be missing.

	bool		SetTempFileRawIndexingData(ID raw_sess_id, PrRawIndexingFileInfo &hdr_info, BYTE *inx_data, long inx_data_len);
				// Method expects that all passed data is correct.

	wchar_t	*StrDupe(const wchar_t *str, long len = -1);
				// This method saves string in m_buffers without any hashing.

	wchar_t	*RegisterStr(const wchar_t *str);
	wchar_t	*RegisterStr(TStringPtr &str_ptr);
	wchar_t	*RegisterStr(TStrPtrInfo &str_ptr_info);
				// These methods register string in the external dictionary but increment local count of errors in case of failure.

	inline int	GetErrCnt() const { return(m_cnt_errors); }
	inline ID	NumEventPtrs() const { return((ID)m_event_ptrs.NumItems()); }

	ID		GetSourceRefFromLinearOffs(long lin_offs);
	ID		GetCondRefFromLinearOffs(long lin_offs);

	bool		GetSrcLocInfo(TMidLevRamStoreReader *db, TSourceLocation &buffer, TLinearArea &lin_area, long max_linear_offs);

protected:

	int		LexSrcIndexIndexFromLinearOffs(long lin_offs);
	int		CondParsIndexIndexFromLinearOffs(long lin_offs);
				// Both methods return lo_good index or -1.

	inline long	LexSrcLinearOffs(int inx)		{ CtvParsingEventInfo *pev = m_lex_src_index[inx]; return((pev->hdr.event_type == pset_lex_source_enter) ? pev->src_enter.LinearPosAfterEntering() : pev->src_exit.linear_pos_after_return); }
	inline long	CondParsLinearOffs(int inx)	{ return(m_cond_pars_index[inx]->cond_pars.hdr.area.linear_beg); }

protected:

	CtvCompilationInfo					m_session_header;

	CtvParsingEventPtrsArray			m_event_ptrs;
	int								m_cnt_errors;
										// This array stores events based on the event_id to allow getting events using the event_id
										// as an index into the array. Nevertheless some event slots can be empty. Once an even slot
										// is not empty, event should have all its fields properly set. For perf reasons array can be
										// longer than the actual number of events in the parsing session.

	CtvParsingEventPtrsArray			m_lex_src_index;
										// Array of pointers to all LexSrcEnter and LexSrcExit events in the order of event IDs.

	CtvParsingEventPtrsArray			m_cond_pars_index;
										// Array of pointers to all CondParsingChange events in the order of event IDs.

	TMidLevRamLogBuffsArray			m_buffers;
	TStringsDict						*m_pStrDict;

	friend class TMidLevRamLogger;
	friend class TMidLevRamBasedDataStorage;
	friend class TMidLevRamStoreReader;
	friend class MidScanTestMidStoreHelper;
};

//
//  This structure contains info about the disk file that is stored in the memory based parsing events
//  database. This structure has nested data fields with ctors. Although these fields allow initializing them
//  with zeroes and moving them from one place in the memory to another.
//
struct TMidLevRamBasedDiskFileInfo : public THashedStructsArrayItem
{
public:

	TFileInfo				data;
							// The  name of the file is not owned by the structure. The file body and the lines
							// info are always NULL.

	BYTE				*indexing_data;
							// This data is expected to follow the special packed format and have proper
							// termination element. Indexinf data is owned by this structure.

	TParsingIdsArray		pars_refs;
							// List of parsing session ids that use this file. Note that this is a heap based array.
							// It can be simply clobbered with zeroes.

	// This is replacement for the default ctor. All data fields of the struct should conform to this method.
	void Clear() { memset(this, 0, sizeof(TMidLevRamBasedDiskFileInfo)); }

	// Second ctor.
	void Init(TFileInfo &proto, wchar_t *fn_copy)
	{
		// This call will clear all fields of the class including the lines info fields.
		Clear();

		data.info = proto.info;
		data.info.file_body = NULL;
		data.info.file_name = fn_copy;
		data.strg_type = proto.strg_type;
		data.rscn_type = proto.rscn_type;
	}

	// This method is a replacement for the desctructor.
	void ReleaseData()
	{
		if (indexing_data != NULL)
			free(indexing_data);

		pars_refs.ReleaseData();
		Clear();
	}

	bool		SetRawIndexingData(ID raw_sess_id, PrRawIndexingFileInfo &hdr_info, BYTE *inx_data, long inx_data_len);

	bool		AddParsingId(ID pars_id);				// The return val is FALSE in case of the mem failure.
	bool		RemoveParsingId(ID pars_id);			// The return value is FALSE if the pars_id is unknown.
	bool		CheckParsingId(ID pars_id);

protected:

	int		FindParsIdInx(ID pars_id);
				// Method returns a lo_good index or -1.
};

// Define this number to allow its consistent use in asserts later.
#define	MID_LEV_RAM_STORE_INITIAL_DISK_FILES_NUM	200

class TMidLevRamBasedDiskFilesArray : public THashedStructsArray<TMidLevRamBasedDiskFileInfo, MID_LEV_RAM_STORE_INITIAL_DISK_FILES_NUM, 1000, 128>
{
public:

	TMidLevRamBasedDiskFilesArray() { InitDiskFilesArray(); }

	void		InitDiskFilesArray();
				// Disk files array needs special initialization. The range of built in file ids (1...TFileInfo::DiskFileIdsBase-1) should be
				// filled with empty records. These records should have file_id == 0 and they should not be present in the hash lists.
				// Presence or absence of the built in file is determined on the value of the file_id field (zero or not zero).

	DWORD CalcHashValue(TMidLevRamBasedDiskFileInfo &item)
	{
		return(CalcStrHash(item.data.info.file_name));
	}

	int CompareRecords(TMidLevRamBasedDiskFileInfo &i1, TMidLevRamBasedDiskFileInfo &i2)
	{
		TBasicFileInfo &fi1 = i1.data.info;
		TBasicFileInfo &fi2 = i2.data.info;
		if (fi1.file_crc != fi2.file_crc || ___abs(fi1.file_date-fi2.file_date) > 100000 || fi1.file_len != fi2.file_len && wcscmp(fi1.file_name, fi2.file_name) != 0)
		{
			// The files are different.
			return(1);
		}

		// Basic parameters of the file are the same. Ensure that other props are identical also.
		assert(i1.data.strg_type == i2.data.strg_type);
		assert(i1.data.rscn_type == i2.data.rscn_type);
		return(0);
	}
};

//
//  Info about the cond parameter that is stored in the memory based parsing events database.
//
struct TMidLevRamBasedIndexingKeyInfo : public THashedStructsArrayItem
{
public:

	TKeyInfo					data;

	CtvKeyFileRefsArray		file_refs;
	CtvKeyParsRefsArray		pars_refs;
								// References are sorted by doc_id/pars_id;

	// This is replacement of the default ctor. Data fields should conform to this method.
	void Clear()
	{
		memset(this, 0, sizeof(TMidLevRamBasedIndexingKeyInfo));
	}

	// Second ctor.
	void Init(TKeyType type, const wchar_t *name_copy)
	{
		Clear();
		data.key_type = type;
		data.key_name = (wchar_t*)name_copy;
	}

	// This is replacement of desctructor.
	void ReleaseData()
	{
		file_refs.ReleaseData();
		pars_refs.ReleaseData();
		Clear();
	}

	bool		FindFileRefInfo(ID pars, ID fid, CtvKeyFileRefInfo &buffer);
	bool		FindParsRefInfo(ID pars, CtvKeyParsRefInfo &buffer);

	bool		UpdateFileRef(TLongId doc_id, int new_use_cnt, DWORD new_use_flags);
	bool		UpdateParsRef(ID pars, int new_use_cnt, DWORD new_use_flags);
				// These methods can add amd remove file/pars from ref lists depending on the value
				// of the new_use_cnt param: val > 0 or val == 0.
protected:

	int		GetFileRefIndex(TLongId id);
	int		GetParsRefIndex(ID pars);
				// Methods return a lo_good index or -1.
};

class TMidLevRamBasedIndexingKeysArray : public THashedStructsArray<TMidLevRamBasedIndexingKeyInfo, 500, 1000, 256>
{
	DWORD CalcHashValue(TMidLevRamBasedIndexingKeyInfo &item)
	{
		return(CalcStrHash(item.data.key_name));
	}

	int CompareRecords(TMidLevRamBasedIndexingKeyInfo &item1, TMidLevRamBasedIndexingKeyInfo &item2)
	{
		if (item1.data.key_type != item2.data.key_type || wcscmp(item1.data.key_name, item2.data.key_name) != 0)
		{
			// These are different keys.
			return(1);
		}

		// The keys are identical.
		return(0);
	}
};

class TMidLevRamBasedDataStorage
{
public:
			TMidLevRamBasedDataStorage();
			~TMidLevRamBasedDataStorage();
protected:

	TMidLevRamBasedParsingLog *CreateParsingLog(PrInitParsingInfo1 &init_info);

	void		DeleteAllParsingLogs() { m_parsings.DeleteAll(); }
	void		DeleteParsingLog(TMidLevRamBasedParsingLog *log) { m_parsings.RemoveItem(log); delete log; }
	void		ResetStorage() { m_parsings.DeleteAll(); m_disk_files.InitDiskFilesArray(); m_indexing_keys.Clear(); }

	ID		LookupOrRegisterDiskFile(TFileInfo &info, ID bk_ref_pars_id, ID &raw_indexing_sess_id);
				// This method should be called only for disk, built_in and proto files.
				// Method is also adding the parsing session back reference.

	ID		LookupOrRegisterIndexingKey(TKeyInfoItem &info);
				// The indexing key is identified by combination of its name and type.
				// Lookup operation is not creating any back refs.

	TMidLevRamBasedParsingLog *FindParsingLog(ID pars_id, ID *min_pid = NULL, ID *max_pid = NULL);
				// Method returns pointer to the parsing log or NULL. Min pid and max pid values are set only when
				// the requested log is not available.

	void		TakeContentsFrom(TMidLevRamBasedDataStorage &other_inst);
				// Method picks up the contents using the move semantics.

protected:

	TList									m_parsings;				// This list contains the TMidLevRamBasedParsingLog instances.

	ID									m_batch_id_cnt;
	ID									m_pars_id_cnt;

	TMidLevRamBasedDiskFilesArray			m_disk_files;
											// This array contains info on all disk, built_in and proto files. These storage types have
											// parsing independent FIDs. This table stores file headers without the file bodies and lines info.
											// Files can be picked up either by uisng the FID as an index to the array or by using the file
											// name as the key for the hashing mechanism.

	TMidLevRamBasedIndexingKeysArray		m_indexing_keys;

		//
		// The disk file and indexing key tables cannot have allocation errors becase when an allocation
		// attempt fails, the access methods return FALSE and this allows the caller to handle this error.
		// This technique keeps these tables always in a sort of "correct" state.
		//

	TStringsDict							m_strs_dict;
											// The strings dictionary is shared between all parsing logs.

	friend class TMidLevRamLogger;
	friend class TMidLevRamStoreReader;
};

class TMidLevRawIndexingDataReader
{
public:

	TMidLevRawIndexingDataReader() { m_data = NULL; m_key_id_restr = 0; }

	void		Setup(BYTE *data, ID key_id_restr) { m_data = data; m_key_id_restr = key_id_restr; }
	bool		GetNextKeyUseInfo(CtvRawIndexingKeyUseInfo &buffer) { return(FALSE); }

protected:

	BYTE		*m_data;
	ID			m_key_id_restr;
};

class TMidLevRamLogger : public TMidLevScanCbkHandler
{
public:

	TMidLevRamLogger(TGenericConsoleEx *console = NULL, bool single_pars_logger = FALSE)
					: TMidLevScanCbkHandler(console) { m_cp = NULL; m_single_parsing_logger = single_pars_logger; }
		//
		// When the second param of the ctor is TRUE this means that the storage is allowed to keep
		// only one parsing session. This param does not mean more than what was mentioned above.
		//

	TMidLevRamBasedDataStorage	*GetDataStore() { return(&m_data); }

	//
	//  All methods below are inherited from the parent class.
	//

	bool		AllocateBatchId(ID &batch_id) { batch_id = m_data.m_batch_id_cnt++; return(TRUE); }

	bool		LookupOrRegisterDiskFileInfo(TFileInfo &info);
	bool		LookupOrRegisterIndexingKey(TKeyInfoItem &info);

	bool		StartRawIndexingSession(PrInitParsingInfo1 &init_info);
	void		FinishRawIndexingSession(TRawIndexingSessSummary &sumry, TFileInfoItem *mj_src_file = NULL);

	bool		StartParsing(PrInitParsingInfo1 &init_info);			// This method allocates a new parsing ID for the parsing session.
	bool		StartPrepPhase(PrInitParsingInfo2 &init_info);
	bool		StartMainPhase(PrInitParsingInfo3 &init_info);
	void		FinalizeParsing(TMidLevPrepOptions *prep_options, TMidLevScannerOptions *scan_options, TMidLevParsingSummary &summary, int final_pars_status);

	void		RegisterDriveSubstData(PrDriveSubstRegistration *info);
	void		RegisterTempFileData(PrTempFileRegistration *info);
	void		PasteBuffChangeNotification(PrPasteBufferingChange *info);
	void		LexSourceEnterNotification(PrLexSourceEnter *info);
	void		LexSourceExitNotification(PrLexSourceExit *info);
	void		MacroLibraryChangeNotification(PrMacroLibraryChange *info);
	void		CondParsSttChangeNotification(PrCondParsingSttChange *info);
	void		MiscPreprocEventNotification(PrMiscPreprocStmtEvent *info);
	void		NewIncludePathNotification(PrNewIncPathEvent *info);
	void		ParsingErrorNotification(PrErrorWarningEvent *info);
	void		CondParamUseNotification(PrCondParamUseEvent *info);

	void		GrammarSectionNotification(PrGrammarDefnSection *info);
	void		GrammarObjectNotification(PrGrammarDefnObject *info);
	void		CppItemNotification(PrCppItemReferenceEvent *info);

	bool		SetParsSessIndexingKeyUseStatus(TKeyInfoItem *info);
	bool		SetRawSessIndexingKeyUseStatus(TLongId doc_id, TKeyInfoItem *info);
	bool		SetRawIndexingData(PrRawIndexingFileInfo &hdr_info, BYTE *inx_data, long inx_data_len);

	TMidLevStoreReader	*AllocateReaderObject(wchar_t *buffer_4_max_path = NULL);
	void					DisposeReaderObject(TMidLevStoreReader *reader);

public:

	//
	//  New methods.
	//

	void		AttachReader(TMidLevRamStoreReader *reader);
				// This method sets the data source into the reader. There is no need to detach the reader from
				// the logger, only the reader should not be used after disposing the logger.

	void		ResetStorage() { m_data.ResetStorage(); }
	void		TakeContentsFrom(TMidLevRamLogger &other_inst);
				// This method implements limited copying that transfers only info about the finished parsings.
				// After taking contents from outside the logger should be used only for reading data from it.
protected:

	inline ID	ParsingId() { return(m_cp->m_session_header.parsing_id); }
					// This is inline version of the public virtual method. This method should be kept separate
					// because virtual functions cannot have inline implementation.

	void		ShowEventContentsError(PrEventHeader &hdr, CtvEventType etp) { ShowEventError(hdr, etp, L"Event Contents Error"); }
	void		ShowEventContentsErrorEx(PrEventHeader &hdr, CtvEventType etp, const wchar_t *hdr_msg) { ShowEventError(hdr, etp, hdr_msg); }
	void		ShowEventPreparationError(PrEventHeader &hdr, CtvEventType etp) { ShowEventError(hdr, etp, L"Event Preparation Error"); }
	void		ShowEventProcessingError(PrEventHeader &hdr, CtvEventType etp)  { ShowEventError(hdr, etp, L"Event Processing Error");  }
	void		ShowEventPreparationError(CtvEventHeader &hdr) { ShowEventError(hdr, L"Event Preparation Error"); }
	void		ShowEventProcessingError(CtvEventHeader &hdr)  { ShowEventError(hdr, L"Event Processing Error");  }
	void		ShowEventError(PrEventHeader &hdr, CtvEventType etp, const wchar_t *message);
	void		ShowEventError(CtvEventHeader &hdr, const wchar_t *message);

protected:

	TMidLevRamBasedDataStorage				m_data;
	TMidLevRamBasedParsingLog				*m_cp;
	bool										m_single_parsing_logger;

	TStructsArray<TCppSourceArea, 16, 32>		m_cpp_src_areas_buffer;
												// This buffer is used for temp storying the cpp source areas while converting
												// the PrCppItemReferenceEvent struct into the CtvCppItemRefEvent record.
};

//
//  ------------- Database reader object ------------------
//
class TMidLevRamStoreReader : public TMidLevStoreReader
{
public:

	TMidLevRamStoreReader(bool allow_other_revisions = TRUE, bool recheck_cached_revisions = TRUE);

	void			SetupDataSource(TMidLevRamBasedDataStorage *data);
					// The mid lev data store class is essentially a database reader object. This method
					// setups the data, that is retrieved using this reader.

	//
	//  Part 1a. Picking up the top level objects.
	//

	bool		GetCompilationInfo				(ID parsing_id, bool silent_mode = FALSE);
	bool		GetLatestCompilationInfo		(bool silent_mode = FALSE);
	bool		GetFileInformation				(ID pars_id, ID file_id, bool want_body_and_linfo = FALSE, bool *file_version_mismatch_flag = NULL, bool silent_mode = FALSE);
	bool		CheckFileInfoPresence			(ID pars_id, ID file_id);
	int		GetNumDiskFiles					(bool want_files_array_len = FALSE) const;
	bool		GetIndexingKeyInfo				(ID key_id);

	bool		DeleteParsingSession			(ID parsing_id);

	//
	//  Part 1b. Iterating the top level objects.
	//

	bool  StartBatchesIteration			() { StartTopLevelIteration(icat_batch); return(TRUE); }
	bool  StartCompilationsIteration		(ID batch_id = 0) { StartTopLevelIteration(icat_pars_sess);  m_batch_restr = batch_id; return(TRUE); }
	bool  StartFilesIteration				(bool show_dfiles, bool show_tfiles, ID pars_id_restr = 0, bool want_body_and_linfo = FALSE, bool want_indexing_info = FALSE);
	bool  StartIndexingKeysIteration		() { StartTopLevelIteration(icat_keys); return(TRUE); }

	bool  NextBatchId						(ID &id);
	bool  NextCompilationRecord			();
	bool  NextFileRecord					();
	bool  NextIndexingKeyRecord			();

	bool  CloseBatchesIteration			() { return(CloseIteration(icat_batch)); }
	bool  CloseCompilationsIteration		() { return(CloseIteration(icat_pars_sess)); }
	bool  CloseFilesIteration				() { return(CloseIteration(icat_files)); }
	bool  CloseIndexingKeysIteration		() { return(CloseIteration(icat_keys)); }

	//
	//  Part 2a. Picking up one parsing event.
	//
	//	   Note:	Picking up info about the temp file does not allow to retrieve persistent pointer to the info on this file.
	//			The same applies to parsing events iteration when temp file registration event is part of the iteration.
	//			This can be achieved only using the GetFileInformation()/StartFilesIteration() API.
	//

	bool  CheckEventPresence			(ID parsing_id, ID event_id) { return(GetParsingEventPointer(parsing_id, event_id, pset_none, FALSE, TRUE) != NULL); }
	bool  GetParsingEvent					(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_none)); }

	bool  GetDriveSubstEvent				(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_register_drive_subst)); }
	bool  GetTempFileInfo					(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_register_temp_file)); }
	bool  GetPasteBufferingChgEvent	(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_paste_buffering_change)); }
	bool  GetLexSourceEnterEvent		(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_lex_source_enter)); }
	bool  GetLexSourceExitEvent			(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_lex_source_exit)); }
	bool  GetMacroLibraryChangeEvent	(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_macro_library_change)); }
	bool  GetCondParsingChangeEvent	(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_cond_parsing_change)); }
	bool  GetMiscPreprocStmtEvent		(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_misc_preproc_stmt)); }
	bool  GetIncludePathRegEvent		(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_new_include_path)); }
	bool  GetErrorWarningEvent			(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_error_warning_event)); }
	bool  GetCondParamUseEvent		(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_cond_param_use)); }
	bool  GetGrammarSectionEvent		(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_grammar_section)); }
	bool  GetGrammarObjectEvent		(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_grammar_object)); }
	bool  GetCppItemRefEvent			(ID parsing_id, ID event_id) { return(GetParsingEventInternal(parsing_id, event_id, pset_cpp_item_ref)); }

	//
	//  Part 2b. Looking for the parent events.
	//

	bool  GetSourceRefEvent				(ID parsing_id, long linear_offs);
	bool  GetCondRefEvent				(ID parsing_id, long linear_offs);

	//
	//  Part 2c. Misc events.
	//

	bool  GetParsingEventsArray			(CtvParsingEventPtrsArray &buffer, ID parsing_id, PSET_TYPES_MASK evt_types_mask, ID min_evid = 0, ID max_evid = 0);

	//
	//  Part 3a. Parsing events iteration.
	//
	//  Note: Once a non zero "max_event_id" is passed, then object with this id will be included into the iteration.
	//

	bool  StartParsingEventsIteration				(ID parsing_id, PSET_TYPES_MASK evts_mask, ID src_ref = 0, ID min_evid = 0, ID max_evid = 0) { return(StartEventsIteration(parsing_id, evts_mask, src_ref, min_evid, max_evid, pset_none)); }

	bool  StartDriveSubstsIteration				(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_register_drive_subst)); }
	bool  StartTempFileRegsIteration				(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_register_temp_file)); }
	bool  StartPasteBufferingChgsIteration		(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_paste_buffering_change )); }
	bool  StartLexSourceEntersIteration			(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_lex_source_enter)); }
	bool  StartLexSourceExitsIteration			(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_lex_source_exit)); }

	bool  StartMacroLibraryChangesIteration	(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_macro_library_change)); }
	bool  StartCondParsingChangesIteration		(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_cond_parsing_change)); }
	bool  StartMiscPreprocStmtsIteration		(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_misc_preproc_stmt)); }
	bool  StartIncludePathRegsIteration			(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_new_include_path)); }
	bool  StartErrorWarningEventsIteration		(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_error_warning_event)); }
	bool  StartCondParamsUseIteration			(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_cond_param_use)); }

	bool  StartGrammarSectionsIteration			(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_grammar_section)); }
	bool  StartGrammarObjectsIteration			(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_grammar_object)); }
	bool  StartCppItemRefEventsIteration		(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) { return(StartEventsIteration(parsing_id, 0, source_ref, min_event_id, max_event_id, pset_cpp_item_ref)); }

			//
			// When the source_ref param is zero, this means that there is no source_ref restriction.
			//

	bool  StartCondParsChgCondRefsIteration	(ID parsing_id, ID cond_ref, ID min_event_id = 0, ID max_event_id = 0, bool ascending_iter_order = TRUE);

	bool  NextParsingEvent						() { return(NextParsingEventInternal1(pset_none)); }
	bool  NextDriveSubstEvent					() { return(NextParsingEventInternal1(pset_register_drive_subst)); }
	bool  NextTempFileRegsEvent					() { return(NextParsingEventInternal1(pset_register_temp_file)); }
	bool  NextPasteBufferingChgsEvent			() { return(NextParsingEventInternal1(pset_paste_buffering_change)); }
	bool  NextLexSourceEntersEvent				() { return(NextParsingEventInternal1(pset_lex_source_enter)); }
	bool  NextLexSourceExitsEvent				() { return(NextParsingEventInternal1(pset_lex_source_exit)); }
	bool  NextMacroLibraryChangesEvent		() { return(NextParsingEventInternal1(pset_macro_library_change)); }
	bool  NextCondParsingChangesEvent		() { return(NextParsingEventInternal1(pset_cond_parsing_change)); }
	bool  NextMiscPreprocStmtsEvent			() { return(NextParsingEventInternal1(pset_misc_preproc_stmt)); }
	bool  NextIncludePathRegEvent				() { return(NextParsingEventInternal1(pset_new_include_path)); }
	bool  NextErrorWarningEventsEvent			() { return(NextParsingEventInternal1(pset_error_warning_event)); }
	bool  NextCondParamsUseEvent				() { return(NextParsingEventInternal1(pset_cond_param_use)); }
	bool  NextGrammarSectionEvent				() { return(NextParsingEventInternal1(pset_grammar_section)); }
	bool  NextGrammarObjectEvent				() { return(NextParsingEventInternal1(pset_grammar_object)); }
	bool  NextCppItemRefEvent					() { return(NextParsingEventInternal1(pset_cpp_item_ref)); }

	bool  NextCondParsChgCondRefEvent		() { return(NextParsingEventInternal2(pset_cond_parsing_change)); }

	bool  CloseParsingEventsIteration			() { return(CloseIteration(EvtIterCat(pset_none))); }
	bool  CloseDriveSubstsIteration				() { return(CloseIteration(EvtIterCat(pset_register_drive_subst))); }
	bool  CloseTempFileRegsIteration				() { return(CloseIteration(EvtIterCat(pset_register_temp_file))); }
	bool  ClosePasteBufferingChgsIteration		() { return(CloseIteration(EvtIterCat(pset_paste_buffering_change))); }
	bool  CloseLexSourceEntersIteration			() { return(CloseIteration(EvtIterCat(pset_lex_source_enter))); }
	bool  CloseLexSourceExitsIteration			() { return(CloseIteration(EvtIterCat(pset_lex_source_exit))); }
	bool  CloseMacroLibraryChangesIteration	() { return(CloseIteration(EvtIterCat(pset_macro_library_change))); }
	bool  CloseCondParsingChangesIteration	() { return(CloseIteration(EvtIterCat(pset_cond_parsing_change))); }
	bool  CloseMiscPreprocStmtsIteration		() { return(CloseIteration(EvtIterCat(pset_misc_preproc_stmt))); }
	bool  CloseIncludePathRegsIteration			() { return(CloseIteration(EvtIterCat(pset_new_include_path))); }
	bool  CloseErrorWarningEventsIteration		() { return(CloseIteration(EvtIterCat(pset_error_warning_event))); }
	bool  CloseCondParamsUseIteration			() { return(CloseIteration(EvtIterCat(pset_cond_param_use))); }
	bool  CloseGrammarSectionsIteration		() { return(CloseIteration(EvtIterCat(pset_grammar_section))); }
	bool  CloseGrammarObjectsIteration			() { return(CloseIteration(EvtIterCat(pset_grammar_object))); }
	bool  CloseCppItemRefEventsIteration		() { return(CloseIteration(EvtIterCat(pset_cpp_item_ref))); }

	bool  CloseCondParsChgCondRefsIteration	() { return(CloseIteration(EvtIterCat(pset_cond_parsing_change))); }

	//
	//  Part 3b. Iterating file/pars bkrefs and raw indexing key uses.
	//

	bool  StartFileParsRefsIteration				(ID pars_id, ID file_id);
	bool  StartFileIndexingSessionsIteration		(ID pars_id, ID file_id, ID key_id = 0);
	bool  StartKeyFileRefsIteration				(ID key_id) { return(StartKeyRefsIteration(key_id, icat_key_file)); }
	bool  StartKeyParsRefsIteration				(ID key_id) { return(StartKeyRefsIteration(key_id, icat_key_pars)); }

	bool  NextFileParsRefsParsingId				(ID &id);
	bool  NextFileIndexingSessionRecord		(CtvRawIndexingKeyUseInfo &info) { assert(IterationActive(icat_file_key) == TRUE); return(m_raw_data_reader.GetNextKeyUseInfo(info)); }
	bool  NextKeyFileRefRecord					();
	bool  NextKeyParsRefRecord					();

	bool  CloseFileParsRefsIteration				() { return(CloseIteration(icat_file_pars)); }
	bool  CloseFileIndexingSessionsIteration		() { return(CloseIteration(icat_file_key));  }
	bool  CloseKeyFileRefsIteration				() { return(CloseIteration(icat_key_file));  }
	bool  CloseKeyParsRefsIteration				() { return(CloseIteration(icat_key_pars));  }

	//
	//  Part 4. Misc and support methods.
	//

	TMidLevStoreReader	*AllocateNewSecondaryStore();

protected:

	bool  GetSourceLocationInfoInternal(TSourceLocation &buffer, ID parsing_id, TLinearArea &lin_area, long max_linear_offs);

	// High level types of iterations.
	enum TIterCategory
	{
		icat_none,		// Iteration is not active.
		icat_batch,		icat_pars_sess,		icat_files,		icat_keys,
		icat_file_pars,	icat_file_key,		icat_key_file,	icat_key_pars,
		icat_events,		// This is beginning of the values range.
	};

	inline static TIterCategory	EvtIterCat(CtvEventType evt_type) { return((TIterCategory)(icat_events+evt_type)); }

	inline bool	DataStorePresent() { return((m_data != NULL) ? TRUE : FALSE); }
	inline bool	IterationInactive() { return((m_data == NULL || m_iter_type == icat_none) ? TRUE : FALSE); }
	inline bool	IterationActive(TIterCategory ctgr) { return((m_data != NULL && m_iter_type == ctgr) ? TRUE : FALSE); }
					// These function are primarily expected to be used in assert statements.

	TMidLevRamBasedDiskFileInfo	*GetDiskFilePtr(ID file_id, bool silent_mode);
	TMidLevRamLogTempFileRecord	*GetTempFilePtr(ID pars_id, ID file_id, bool silent_mode);

	bool					PrepareDiskFileInfo(TFileInfo &info, bool want_body_and_linfo, bool *file_version_mismatch_flag);
	CtvParsingEventInfo	*GetParsingEventPointer(ID parsing_id, ID event_id, CtvEventType expected_event_type, bool want_full_check, bool silent_mode);
	bool					GetParsingEventInternal(ID parsing_id, ID event_id, CtvEventType expected_event_type);

	void		StartTopLevelIteration(TIterCategory icat);
	bool		StartEventsIteration(ID pid, PSET_TYPES_MASK evt_types_mask, ID src_ref, ID min_evid, ID max_evid, CtvEventType events_category);
	bool		StartKeyRefsIteration(ID key_id, TIterCategory iter_type);

	bool		NextCompilationRecodInternal();
	bool		NextFileRecordInternal(TFileInfo **ppInfo);
	bool		NextParsingEventInternal1(CtvEventType event_category);
	bool		NextParsingEventInternal2(CtvEventType event_category);

	bool		CloseIteration(TIterCategory category = icat_none);
				// When this method is called with category == ic_none, it silently resets any iteration if it is active.

protected:

	TMidLevRamBasedDataStorage			*m_data;

	TIterCategory							m_iter_type;

	TMidLevRamBasedParsingLog			*m_curr_log;
	bool									m_parsing_logs_eof;				// Pointer to the log that was found by the latest step of the parsings
																		// iteration or current log that is used by an active events iteration.
	int									m_curr_inx;						// Index of the top level object or event that was not considered yet.

	// Group 1b. Top level objects iteration.
	ID									m_batch_restr;
	bool									m_show_diskf_restr;
	bool									m_show_tempf_restr;
	ID									m_pars_id_restr;
	bool									m_wbody_instr;					// Want body of the file.
	bool									m_wrinx_instr;					// Want raw indexing session info.

	// Group 3a. Events of one parsing session.
	PSET_TYPES_MASK					m_evt_types_mask;
	ID									m_src_ref_restr;
	ID									m_cond_ref_restr;
	ID									m_evid_restr;					// This is a max_evid for the ascending iterations (an object with this id can
																		// be returned) and this is min_evid if the order or iteration is descending.
	bool									m_evts_ascending_order;

	// Group 3b. Back refs and raw indexing iteration.
	TParsingIdsArray						*m_citer_pars_bkrefs;
	TParsingIdsArray						m_dummy_pars_bkref;
	TMidLevRawIndexingDataReader			m_raw_data_reader;
	TMidLevRamBasedIndexingKeyInfo		*m_citer_indexing_key;

	friend class TMidLevRamBasedParsingLog;
	friend class MidScanTestMidStoreHelper;
};

#endif	// TextScan_RamBasedStore_H


