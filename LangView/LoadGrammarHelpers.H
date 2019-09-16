//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_LoadGrammarHelpers_H
#define	LangView_LoadGrammarHelpers_H

#ifndef   Xml_HtmlGenDecors_H
#include  "Xml/HtmlGenDecors.H"
#endif
#ifndef   TextScan_GrammarDefinitionParser_H
#include  "TextScan/GrammarDefinitionParser.H"
#endif
#ifndef   LangView_MidScanConsole_H
#include  "LangView/MidScanConsole.H"
#endif

//
//  This class is derived from the TLangViewBasicAsyncConsole class that provides functionality for running the app layer
//  procedure on the worker thread while the main UI thread is displaying the modal console. Besides implementing
//  the app level procedure that scans the grammar definition and makes various reports, this class provides the FileOpen()
//  functionality that allows getting the name of the file from the user. Name of the selected file is stored in the data fields
//  of this class when the FileOpen() is closed and scanning is not started yet.
//
class TLangViewLoadGrammarHelper : public TLangViewBasicAsyncConsole
{
public:

			TLangViewLoadGrammarHelper(DWORD parsing_trace_flags = (sctr_err_warn | gdtr_steps | gdtr_reports), bool build_parsing_table = TRUE,
												const wchar_t *grammar_source_emitting_console_trace_prefix = NULL);
						// Trace flags can include both the "sctr_*" flags from the TMidLevScannerTraceFlags and the "gdtr_*" flags
						// from the TGrammarDefinitionParserTraceFlags.

	bool				GetGrammarFileName(TBasicDialog *proto_and_owner);
						// This method prompts user with the Windows FileOpen() dialog.

	SessionResults	LoadGrammarDefinition(TBasicDialog *proto_and_owner, TGrammar *grammar, TMidLevScanCbkHandler *logger = NULL);
						// This method clears the passed grammar right at the beginning for simplicity. Note that name of the file
						// to process is not given explicitly. It is expected that this name is a result of the FileOpen() dialog.

	void				ShowSessionResults(TBasicDialog *dlg);
						// Method shows stored results in the message boxes. Either one or two message boxes
						// can be shown. In the case of a full success this method will not show anything.
protected:

	virtual void		SessionWorkerProcedure();

	void				EmitGrammarSourcesAsCppData(TList &disk_files, TGrammarDefinitionParser &grdef_parser, bool place_all_files_in_mjsrc_subdir);
						// The generated files are placed either into the "Obj" subdir of each orginal source file or all generated files
						// are placed into the "Obj" subdir of the directory of the primary grammar source file.

	void				EmitGrammarSourcesAsHtmlTables(TList &disk_files, TGrammarDefinitionParser &grdef_parser, bool want_single_page_fragm_with_all_src_files);
						// This metod generates either a separate HTML page for each grammar source file or a page fragment that
						// contains sequence of <table> elements. Each table element describes its own grammar source file.

	bool				EmitSingleFileAsHtmlPage(TList &disk_files, TGrammarDefinitionParser &grdef_parser, THtmlDocument &rprt, TFileInfo &file_info, int igrammar, bool want_html_hyper_links);
						// Note that this method expects the report to be already opened. It emits the page and closes the report.

	static int __cdecl	CompareDiskFileInfoItems(void *ctx, const TListItem **item1, const TListItem **item2);

protected:

	TFileOpenHelper					m_ofn_grammar;

	DWORD							m_parsing_trace_flags;
	bool								m_build_parsing_table;

	wchar_t							*m_src_emitting_trace_prefix;		// The trace prefix is not owned and it is expected to be static.
	wchar_t							*m_css_files_directory;				// Directory name should contain forward slashes and should
																		// not have trailing slash at the end.
	TGrammar						*m_grammar;
	TMidLevScanCbkHandler			*m_parsing_logger;

	SessionResults					m_sres;
	int								m_grammar_syntax_errors;
	bool								m_grammar_src_emitting_error;

	wchar_t							m_console_prefix[40];				// This prefix is prepared when the session worker procedure starts and
																		// it should be used only in the context of this procedure.
};

struct THtmlGrammarSourceFileGenProps
{
	bool			m_show_app_ids;

	bool			m_add_hyper_links;
	bool			m_add_dest_loc_info;
};

class THtmlGrammarSourceFile : public THtmlDecoratedFile
{
public:			//
				//	This object uses a fixed set of CSS styles for highlighting various objects.
				//	Note that an id of this object has always one of the following formats:
				//
				//		"G%dF%lu"   or   "G%dT%lu.%lu"
				//
				//	depending on whether the grammar source file is a disk file or a temp file.
				//

			THtmlGrammarSourceFile(const wchar_t *css_class = NULL, const wchar_t *css_style = NULL, THtmlCrLfMode crlf_mode = crlm_outline_object_frames);
			~THtmlGrammarSourceFile() { ResetGrammarSourceFile(); }

	void		ResetGrammarSourceFile();
				// After resetting the object can be reused to display some other file.

	bool		SetupGrammarSourceFile(const wchar_t *console_msgs_prefix, TGenericConsole &cons, TList &disk_files, TGrammarDefinitionParser &grdef_parser,
										TFileInfo &file, bool bypass_file_info_data, int igrammar, THtmlGrammarSourceFileGenProps &gen_props);
				// This method creates an image of the whole file. There is no way to prepare a part of the file. The console messages
				// prefix should be either empty or it should be a short string (not more than 39 chars) with a space at its end.
protected:

	enum TGrammmarSourceZorder
	{
		grszo_navigation_decors,
		grszo_normal,
		grszo_elevated,
	};

	void		ApplyRawDecors(const wchar_t *console_msgs_prefix, TGenericConsole &cons, TList &disk_files, TGrammarDefinitionParser &grdef_parser,
								TGrammar *grm, int igrammar, THtmlGrammarSourceFileGenProps &gen_props);

	void		ApplyGrammarDecors(TGrammarDefinitionParser &grdef_parser, TGrammar *grm, int igrammar);
	void		ApplySpecTabDecors(const wchar_t *console_msgs_prefix, TGenericConsole &cons);

	bool		FindGrammarSourceFile(const wchar_t *console_msgs_prefix, TGenericConsole &cons, TList &disk_files, TStrPtrInfo &name, TFileInfo *&result);
	void		AddDestinationInfoDecor(THtmlGrammarSourceFileGenProps &gen_props, int igrammar, TFileInfo *dest_file, long dest_area_beg, TSourceArea &link_area);
	void		AddGrammarSymbolLabel(WORD sym_value, TSourceArea &sym_area);

	void		ApplyFrgrDecor(const wchar_t *decor_class, TGrammmarSourceZorder zorder, long area_beg, long area_len);
	void		ApplyHyperLinkDecor(const wchar_t *link_href, long area_beg, long area_len);
	void		ApplyDestLocDecor(const wchar_t *section_ref, const wchar_t *element_ref, long area_beg, long area_len);
	void		AddLineMark(const wchar_t *decor_class, const wchar_t *decor_text, long line_number);

	static void	PrepareGrmFileId(wchar_t *buffer, int buff_len, int igrammar, TFileInfo &file);
	static void	PrepareXgrFileName(wchar_t *buffer, int buff_len, int igrammar, const wchar_t *file_name);

	static void	ShowSetupError(const wchar_t *msg_prefix, TGenericConsole &cons, const wchar_t *format, ...);

protected:

	TFileInfo								m_grm_file;
	bool									m_grm_file_data_owned;

	THtmlGenDecorOwnersContext			m_grm_decor_owners;

	int									m_grm_decor_errors;
	int									m_grm_decor_owner_errors;
	int									m_grm_temp_data_alloc_errors;
											// When these types of errors happen, the message is not generated. Only at the end
											// of the setup process a summary message with the count of errors is emitted if needed.

	int									m_grm_other_reported_errors;
											// These errors are reported when they happen and there is no summary message at the end.
											// All types of errors form the overall boolean result of the method Setup().

	friend class TLangViewLoadGrammarHelper;
};

#endif	// LangView_LoadGrammarHelpers_H


