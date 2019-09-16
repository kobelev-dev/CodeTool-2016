//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <windows.h>
#include  <stdio.h>
#include  <assert.h>

#include  "Common/FormatNumber.H"
#include  "TextScan/MidScan.H"
#include  "LangView/LangViewResources.H"
#include  "LangView/LoadGrammarHelpers.H"
#include  "LangView/SpecTabHelpers.H"

// -------------------------------------------------------------------------
//   ===============  TLangViewLoadGrammarHelper  ==================
// -------------------------------------------------------------------------

static const wchar_t *g_GrammarFilterSpec = L"Grammar Files (*.gra)\0*.gra\0All Files (*.*)\0*.*\0";
static const wchar_t *g_GrammarOpenTitle = L" Open Grammar File ";

TLangViewLoadGrammarHelper::TLangViewLoadGrammarHelper(DWORD parsing_trace_flags, bool build_parsing_table, const wchar_t *src_emt_trace_prefix)
					: TLangViewBasicAsyncConsole(L" Load Grammar "),
					m_ofn_grammar(NULL, g_GrammarFilterSpec, L"GRM", g_GrammarOpenTitle)
{
	m_parsing_trace_flags = parsing_trace_flags;
	m_build_parsing_table = build_parsing_table;

	m_src_emitting_trace_prefix = (src_emt_trace_prefix != NULL) ? (wchar_t*)src_emt_trace_prefix : L"";
	m_css_files_directory = L"C:/Ballerup/Calvados/Grammars/CSS";
	// m_css_files_directory = L"http://www.CodeStructureAnalysis.com/TestInfra/Proto1/CSS";

	m_grammar = NULL;
	m_parsing_logger = NULL;
}

bool TLangViewLoadGrammarHelper::GetGrammarFileName(TBasicDialog *proto_and_owner)
{
	// Prompt the user with the file open dialog.
	SetDialogParams(proto_and_owner);
	SetParentWindow(proto_and_owner->GetWnd());
	return(m_ofn_grammar.GetOpenFileName(proto_and_owner->GetWnd()));
}

TLangViewBasicAsyncConsole::SessionResults TLangViewLoadGrammarHelper::LoadGrammarDefinition(TBasicDialog *proto_and_owner, TGrammar *grammar, TMidLevScanCbkHandler *logger)
{
	assert(grammar != NULL);

	// Clear the grammar right now. This grammar will be cleared again later in the TGrammarDefinitionParser::Parse().
	// But this will happen only after passing several potential points of failure that may leave the grammar uncleared.
	grammar->Clear();
	m_grammar = grammar;

	TMidLevScanCbkHandler dummy_logger;
	m_parsing_logger = (logger != NULL) ? logger : &dummy_logger;

	m_sres = sres_not_started;
	m_grammar_syntax_errors = 0;
	m_grammar_src_emitting_error = FALSE;

	// Call a worker method in the base class that will open the console and start an async thread. This thread will
	// run the scanning, build the analysis matrix and generate reports. DoSession will return control when worker
	// thread will terminate. Meanwhile the main thread will prosess the messages from the message queue.
	m_sres = DoSession(proto_and_owner);
	return(m_sres);
}

void TLangViewLoadGrammarHelper::ShowSessionResults(TBasicDialog *dlg)
{
	// Ensure that a loading session was run before.
	assert(m_grammar != NULL);

	// Process the session result code.
	switch (m_sres)
	{
		case sres_not_started:
				{
					dlg->MsgBox(L"The session is not started.");
				}
				break;

		case sres_console_error:
				{
					dlg->MsgBox(L"Error opening the parsing session console.");
				}
				break;

		case sres_thread_error:
				{
					dlg->MsgBox(L"Cannot start the worker thread. The action is aborted.");
				}
				break;

		case sres_app_result:
				{
					if (m_grammar->grm_props.processing_result != grpr_full_success)
					{
						// Note that the step 2 is done regardless of results in the step 1.
						const wchar_t *emitting_err_msg = L"";
						if (m_grammar_src_emitting_error == TRUE)
							emitting_err_msg = L"\r\n\r\n        Error emitting the grammar tables.";

						// Step 1 experienced problems. Show results of the grammar parsing.
						switch (m_grammar->grm_props.processing_result)
						{
							case grpr_none:
									dlg->MsgBoxFmt(MB_OK, L"The resulting grammar is empty.%s", emitting_err_msg);
									break;

							case grpr_syntax_errors:
									dlg->MsgBoxFmt(MB_OK, L"Parsed grammar contains %d syntax error%s.%s",
													m_grammar_syntax_errors, PluralSuffix(m_grammar_syntax_errors), emitting_err_msg);
									break;

							case grpr_conversion_errors:
									dlg->MsgBoxFmt(MB_OK, L"There were grammar conversion errors or the grammar conversion was aborted.%s", emitting_err_msg);
									break;
						}
					}
					else if (m_grammar_src_emitting_error == TRUE)
					{
						dlg->MsgBox(L"Error emitting the grammar tables.");
					}
				}
				break;

		case STILL_ACTIVE:
				{
					dlg->MsgBox(L"The session is still running.");
					assert(FALSE);
				}
				break;

		default:
			assert(FALSE);
			break;
	}
}

void TLangViewLoadGrammarHelper::SessionWorkerProcedure()
{
	assert(m_parsing_logger != NULL);

	// Prepare the tracing delimiter.
	wchar_t delim_msg_buffer[240];
	TGenericConsole::PrepareConsolePrefix(m_console_prefix, m_src_emitting_trace_prefix);
	swprintf(delim_msg_buffer, 240, L"%s--------------------------------------------------------------------------------------------", m_console_prefix);

	// Load and convert the grammar.
	m_parsing_logger->SetupConsole(&m_worker_thread_console);
	TGrammarDefinitionParserHelper helper(*m_parsing_logger, *m_grammar, NULL, m_build_parsing_table);
	helper.ParseGrammarDefinitionFile(m_ofn_grammar.GetCurrFileName(), m_parsing_trace_flags, &m_grammar_syntax_errors);
	m_parsing_logger->SetupConsole(NULL);

	// Emit resulting grammar as a source code. This should be done regardless of the state
	// of the abort signal, presence/absence of the syntax/conversion errors, etc.
	m_worker_thread_console.HandleTrace(delim_msg_buffer);
	bool res = m_grammar->EmitTablesEx(&m_worker_thread_console, m_src_emitting_trace_prefix, TGrammar::DEF_GRM_SRC_SUBDIR, L".cxx", m_grammar->grm_props.preferred_emitting_prefix);

	// Extract lists of files that were used during the parsing.
	TList disk_files, temp_files;
	helper.Scanner().ExtractFilesCache(disk_files, temp_files);

	// Sort disk files in the order of their file ids.
	disk_files.QuickSort(NULL, CompareDiskFileInfoItems);

	// Emit grammar source files as the C++ static data.
	EmitGrammarSourcesAsCppData(disk_files, helper.Parser(), TRUE);

	// Emit files as the HTML tables.
	EmitGrammarSourcesAsHtmlTables(disk_files, helper.Parser(), TRUE);

	// Release contents of the file lists if any.
	disk_files.DeleteAll();
	temp_files.DeleteAll();

	// Store the results of emitting for the main thread.
	m_worker_thread_console.HandleTrace(delim_msg_buffer);
	m_grammar_src_emitting_error = (res == TRUE) ? FALSE : TRUE;
}

void TLangViewLoadGrammarHelper::EmitGrammarSourcesAsCppData(TList &disk_files, TGrammarDefinitionParser &grdef_parser, bool place_all_files_in_mjsrc_subdir)
{
	wchar_t msg_buffer[360];
	int len_prefix = wcslen(m_console_prefix);
	bool disk_files_present = FALSE;

	TFileNameBuffer dest_directory_name(TGrammar::DEF_GRM_SRC_SUBDIR);
	if (place_all_files_in_mjsrc_subdir == TRUE)
	{
		// Caller wants to place all generated files into one directory. Prepare the name of this directory.
		const wchar_t *mjsrc_fname = grdef_parser.DestGrammar().grm_props.grms_file_name;
		if (TPathHelper::PrepareDestDirectory(msg_buffer+len_prefix, 360-len_prefix, dest_directory_name, TGrammar::DEF_GRM_SRC_SUBDIR, mjsrc_fname, L"grammar source file") == FALSE)
		{
			wcsncpy(msg_buffer, m_console_prefix, len_prefix);
			m_worker_thread_console.HandleTrace(msg_buffer, ErrorHighlightMode);
			return;
		}
	}

	for (TListIter<TFileInfoItem> iter(disk_files); iter; ++iter)
	{
		// Pick up the basic descriptor of the file.
		TBasicFileInfo &basic_info = iter.CurrItem().data.info;
		if (basic_info.file_name[0] == L'@')
		{
			// This is a built in file. It does not need dumping.
			continue;
		}

		disk_files_present = TRUE;

		// Emit current file as a C++ static data.
		wchar_t rprt_fname_buff[TDestinationFile::FNAME_BUFFER_LEN];
		if (TFileDumpHelper::DumpAsSourceFile(msg_buffer+len_prefix, 360-len_prefix, basic_info, dest_directory_name, basic_info.file_name, L"grammar source file", L"gsrc_", L"_source_file.cxx",
										L"grammar source file C++ representation", L"g_GrmSrc_", rprt_fname_buff, TDestinationFile::FNAME_BUFFER_LEN) == TRUE)
		{
			// Write a success message.
			wchar_t file_id_buff[40];
			swprintf(msg_buffer, 360, L"%sGrammar source %s is successfully emitted to: \"%s\".", m_console_prefix, iter.CurrItem().data.PrepareFileIdShort(file_id_buff, 40), rprt_fname_buff);
			m_worker_thread_console.HandleTrace(msg_buffer);
		}
		else
		{
			// Emitting failed. Show an error message.
			wcsncpy(msg_buffer, m_console_prefix, len_prefix);
			m_worker_thread_console.HandleTrace(msg_buffer, ErrorHighlightMode);
		}
	}

	if (disk_files_present == FALSE)
	{
		wchar_t console_msg[240];
		swprintf(console_msg, 240, L"%sUnable to emit C++ represenation for the grammar source files. The list of the source files is empty.", m_console_prefix);
		m_worker_thread_console.HandleTrace(console_msg, ErrorHighlightMode);
	}
}

void TLangViewLoadGrammarHelper::EmitGrammarSourcesAsHtmlTables(TList &disk_files, TGrammarDefinitionParser &grdef_parser, bool want_single_page_fragm_with_all_src_files)
{
	int len_prefix = wcslen(m_console_prefix);
	wchar_t msg_buffer[360];

	// Prepare the name of the destination directory. It will be either a short dir name or a subdir of the major source file.
	TFileNameBuffer dest_directory_name;
	const wchar_t *mjsrc_fname = grdef_parser.DestGrammar().grm_props.grms_file_name;
	if (TPathHelper::PrepareDestDirectory(msg_buffer+len_prefix, 360-len_prefix, dest_directory_name, TGrammarDefinitionParser::DEF_REPORTS_SUBDIR, mjsrc_fname, L"grammar source file") == FALSE)
	{
		wcsncpy(msg_buffer, m_console_prefix, len_prefix);
		m_worker_thread_console.HandleTrace(msg_buffer, ErrorHighlightMode);
		return;
	}

	int num_grammars = grdef_parser.DestGrammar().GetNumGrammars();
	for (int igrm=0; igrm<num_grammars; ++igrm)
	{
		wchar_t fname_prefix[20];
		swprintf(fname_prefix, 20, L"xgr_%02d_", igrm);

		bool combined_sources_high_lev_success = TRUE;
		THtmlDocument src_names_rprt(NULL, TRUE), src_bodies_rprt(NULL, TRUE);
		if (want_single_page_fragm_with_all_src_files == TRUE)
		{
			// Open two report files. The first one will contain items of the listbox with the names of the files.
			// The second will contain the bodies of the source files as the sequence of HTML tables.
			if (TPathHelper::PrepareDestFile(msg_buffer+len_prefix, 360-len_prefix, src_names_rprt, dest_directory_name, fname_prefix, L"grammar_sources_names", L".HTML_FRAG", L"grammar source file names list") == FALSE)
			{
				// Preparing the file name or opening the disk file for writing failed.
				wcsncpy(msg_buffer, m_console_prefix, len_prefix);
				m_worker_thread_console.HandleTrace(msg_buffer, ErrorHighlightMode);
				continue;
			}
			else if (TPathHelper::PrepareDestFile(msg_buffer+len_prefix, 360-len_prefix, src_bodies_rprt, dest_directory_name, fname_prefix, L"grammar_sources_bodies", L".HTML_FRAG", L"grammar source files HTML page fragment") == FALSE)
			{
				// Preparing the file name or opening the disk file for writing failed.
				wcsncpy(msg_buffer, m_console_prefix, len_prefix);
				m_worker_thread_console.HandleTrace(msg_buffer, ErrorHighlightMode);
				continue;
			}
		}

		for (TListIter<TFileInfoItem> iter(disk_files); iter; ++iter)
		{
			// Pick up the basic descriptor of the file.
			TFileInfo &file_info = iter.CurrItem().data;
			if (file_info.info.file_name[0] == L'@')
			{
				// This is a built in file. It does not need dumping.
				continue;
			}
			else if (file_info.BuildLinesInfo() == FALSE)
			{
				// Generation of the HTML data is not possible without the lines info. Skip this file.
				swprintf(msg_buffer, 360, L"%sError building lines info for: \"%s\".", m_console_prefix, file_info.info.file_name);
				m_worker_thread_console.HandleTrace(msg_buffer, ErrorHighlightMode);
				continue;
			}

			if (want_single_page_fragm_with_all_src_files == FALSE)
			{
				// Generate complete HTML page.
				THtmlDocument full_page_rprt(NULL);
				if (TPathHelper::PrepareDestFile(msg_buffer+len_prefix, 360-len_prefix, full_page_rprt, dest_directory_name, fname_prefix, file_info.info.file_name, L".HTML", L"grammar source file as HTML page") == FALSE)
				{
					// Preparing the file name or opening the file failed.
					wcsncpy(msg_buffer, m_console_prefix, len_prefix);
					m_worker_thread_console.HandleTrace(msg_buffer, ErrorHighlightMode);
				}
				else if (EmitSingleFileAsHtmlPage(disk_files, grdef_parser, full_page_rprt, file_info, igrm, TRUE) == FALSE)
				{
					swprintf(msg_buffer, 360, L"%sError generating grammar source \"%s\" as a HTML page.", m_console_prefix, file_info.info.file_name);
					m_worker_thread_console.HandleTrace(msg_buffer, ErrorHighlightMode);
				}
				else if (full_page_rprt.GetErrorFlag() == TRUE)
				{
					swprintf(msg_buffer, 360, L"%sError emitting grammar source as a HTML page to: \"%s\".", m_console_prefix, full_page_rprt.FileName());
					m_worker_thread_console.HandleTrace(msg_buffer, ErrorHighlightMode);
				}
				else
				{
					// All generation steps succeeded.
					swprintf(msg_buffer, 360, L"%sGrammar source is successfully emitted as a HTML page to: \"%s\".", m_console_prefix, full_page_rprt.FileName());
					m_worker_thread_console.HandleTrace(msg_buffer);
				}
			}
			else
			{
				// Instantiate the grammar source file object and dump it. Note that the "style" attr on this object is set for
				// not displaying the object. This attr will be modified later dynamically using the JavaScript code.
				THtmlGrammarSourceFileGenProps gen_props = { TRUE, FALSE, TRUE };
				THtmlGrammarSourceFile grammar_source_file(NULL, L"display:none");
				if (grammar_source_file.SetupGrammarSourceFile(m_console_prefix, m_worker_thread_console, disk_files, grdef_parser, file_info, FALSE, igrm, gen_props) == TRUE)
				{
					// The setup has succeeded. Format the id of the current file.
					wchar_t small_buffer[60];
					THtmlGrammarSourceFile::PrepareGrmFileId(small_buffer, 60, igrm, file_info);

					// Pick up the short name of the current file. It will be displayed in the listbox.
					TFileNameBuffer short_name;
					if (TPathHelper::ExtractShortName(short_name, file_info.info.file_name, TRUE) == FALSE)
					{
						// This situation is extremely unlikely to happen. Nevertheless.
						short_name.Set(L"File-Name-Error-For-");
						short_name.Append(small_buffer);
					}

					// Update the list of the file names.
					TArrayBasedHtmlContainer list_item(L"div", L"sec_menu_item", crlm_before_and_after_object);
					list_item.SetFirstAppAttr(L"DestSect", small_buffer);
					TImageHtmlElement empty_bullet(L"/Images/MainFrame/EmptyBullet.Gif");
					TSimpleHtmlElement list_item_body(NULL, short_name);
					list_item.AppendElements(&empty_bullet, &list_item_body);
					src_names_rprt.EmitElement(list_item);

					// Emit the grammar source file element.
					src_bodies_rprt.EmitElement(grammar_source_file);
					src_bodies_rprt.WriteLine();
				}
				else
				{
					// Setup failure.
					swprintf(msg_buffer, 360, L"%sError generating grammar source fragment for \"%s\".", m_console_prefix, file_info.info.file_name);
					m_worker_thread_console.HandleTrace(msg_buffer, ErrorHighlightMode);
					combined_sources_high_lev_success = FALSE;
				}
			}
		}

		if (want_single_page_fragm_with_all_src_files == TRUE)
		{
			// Close the first report and check its result.
			src_names_rprt.Close();
			if (src_names_rprt.GetErrorFlag() == TRUE)
			{
				swprintf(msg_buffer, 360, L"%sError emitting grammar source file names list to: \"%s\".", m_console_prefix, src_names_rprt.FileName());
				m_worker_thread_console.HandleTrace(msg_buffer, ErrorHighlightMode);
			}
			else if (combined_sources_high_lev_success == TRUE)
			{
				// All generation steps for this report succeeded.
				swprintf(msg_buffer, 360, L"%sGrammar source files were successfully emitted as HTML page fragment to: \"%s\".", m_console_prefix, src_names_rprt.FileName());
				m_worker_thread_console.HandleTrace(msg_buffer);
			}

			// Close the second report and check its result.
			src_bodies_rprt.Close();
			if (src_bodies_rprt.GetErrorFlag() == TRUE)
			{
				swprintf(msg_buffer, 360, L"%sError emitting grammar source files HTML page fragment to: \"%s\".", m_console_prefix, src_bodies_rprt.FileName());
				m_worker_thread_console.HandleTrace(msg_buffer, ErrorHighlightMode);
			}
			else if (combined_sources_high_lev_success == TRUE)
			{
				// All generation steps for this report succeeded.
				swprintf(msg_buffer, 360, L"%sGrammar source files were successfully emitted as HTML page fragment to: \"%s\".", m_console_prefix, src_bodies_rprt.FileName());
				m_worker_thread_console.HandleTrace(msg_buffer);
			}
		}
	}
}

bool TLangViewLoadGrammarHelper::EmitSingleFileAsHtmlPage(TList &disk_files, TGrammarDefinitionParser &grdef_parser, THtmlDocument &rprt, TFileInfo &file_info, int igrammar, bool want_html_hyper_links)
{
	assert(rprt.IsDocFragmentMode() == FALSE);

	// Emit the beg of the HTML page.
	rprt.OpenDocumentHead();
	rprt.AddCssFileRef(m_css_files_directory, L"Wbs_RawSyntaxHighlight.CSS");
	rprt.AddCssFileRef(m_css_files_directory, L"Wbs_GrammarDefinition.CSS");

	if (want_html_hyper_links == TRUE)
	{
		// Add insturctions to block displaying the underlines under the hyper links. The order of pseudo classses
		// below IS IMPORTANT. Docs explictly state this. Otherwise browsers do not pick them up properly.
		rprt.AddInlineCssStyle(L"a:link { text-decoration:none; } a:visited { text-decoration:none; }");
		rprt.AddInlineCssStyle(L"a:hover { text-decoration:underline; } a:active { text-decoration:none; }");
	}

	rprt.OpenDocumentBody();
	rprt.SetupIndentDelta(-1);
	rprt.WriteLine();

	if (want_html_hyper_links == TRUE && wcscmp(file_info.info.file_name, grdef_parser.DestGrammar().grm_props.grms_file_name) != 0)
	{
		// Current file is not the major source file of the grammar. Emit a hyper link that will point to the main file.
		TFileNameBuffer short_mjsrc_name;
		if (TPathHelper::ExtractShortName(short_mjsrc_name, grdef_parser.DestGrammar().grm_props.grms_file_name) == FALSE)
			short_mjsrc_name.Set(L"Error-Getting-MjSrc-Short-Name");

		// The body of the link should contian the short name of the mjsrc with its original extension.
		wchar_t link_body_buffer[MAX_PATH+80];
		swprintf(link_body_buffer, MAX_PATH+80, L"Back to %s ...", short_mjsrc_name.DataPtr());

		// The "href" attr should contain the name of the generated HTML file.
		wchar_t link_href_buffer[MAX_PATH+80];
		THtmlGrammarSourceFile::PrepareXgrFileName(link_href_buffer, MAX_PATH+80, igrammar, grdef_parser.DestGrammar().grm_props.grms_file_name);

		TArrayBasedHtmlContainer link_div(L"div", L"grm_back_to_main_file_div");
		THyperLinkHtmlElement link_element(link_href_buffer, link_body_buffer, -1, L"grm_back_to_main_file_text");
		link_div.AppendElement(&link_element);

		rprt.EmitElement(link_div);
		rprt.WriteLine();
	}

	// Show the grammar app ids and generate the hyper links only if the passed param asks for this.
	THtmlGrammarSourceFileGenProps gen_props = { TRUE, want_html_hyper_links, FALSE };

	// Instantiate the grammar source file and emit it.
	THtmlGrammarSourceFile grm_source_file;
	bool setup_res = grm_source_file.SetupGrammarSourceFile(m_console_prefix, m_worker_thread_console, disk_files, grdef_parser, file_info, FALSE, igrammar, gen_props);
	rprt.EmitElement(grm_source_file);
	rprt.WriteLine();

	// Emit an end of the HTML page.
	rprt.SetupIndentDelta(0);
	rprt.CloseDocument();
	rprt.Close();

	// The return value should reflect the results of setup.
	return(setup_res);
}

int __cdecl TLangViewLoadGrammarHelper::CompareDiskFileInfoItems(void *ctx, const TListItem **item1, const TListItem **item2)
{
	TFileInfoItem *fi1 = (TFileInfoItem*)(*item1);
	TFileInfoItem *fi2 = (TFileInfoItem*)(*item2);
	assert(fi1->data.pars_id == 0 && fi2->data.pars_id == 0);
	return(((int)fi1->data.file_id)-((int)fi2->data.file_id));
}

// ---------------------------------------------------------------------------
//   =================  THtmlGrammarSourceFile  =====================
// ---------------------------------------------------------------------------

THtmlGrammarSourceFile::THtmlGrammarSourceFile(const wchar_t *css_class, const wchar_t *css_style, THtmlCrLfMode crlf_mode)
		: THtmlDecoratedFile(css_class, css_style, crlf_mode), m_grm_decor_owners(m_grm_decor_owner_errors)
{
	m_grm_file.Clear();
	m_grm_file_data_owned = FALSE;

	m_grm_decor_errors = 0;
	m_grm_decor_owner_errors = 0;
	m_grm_temp_data_alloc_errors = 0;
	m_grm_other_reported_errors = 0;
}

void THtmlGrammarSourceFile::ResetGrammarSourceFile()
{
	// Reset the base class.
	ResetDecoratedFile();

	if (m_grm_file_data_owned == TRUE)
	{
		// The file data is owned. This should be a relatively rare case.
		m_grm_file.ReleaseBasicFileInfo();
		m_grm_file.ReleaseLinesInfo();
	}

	m_grm_file.Clear();
	m_grm_file_data_owned = FALSE;

	m_grm_decor_owners.ResetContext();

	m_grm_decor_errors = 0;
	m_grm_decor_owner_errors = 0;
	m_grm_temp_data_alloc_errors = 0;
	m_grm_other_reported_errors = 0;
}

bool THtmlGrammarSourceFile::SetupGrammarSourceFile(const wchar_t *console_msgs_prefix, TGenericConsole &cons, TList &disk_files, TGrammarDefinitionParser &grdef_parser,
														TFileInfo &file, bool bypass_file_info_data, int igrammar, THtmlGrammarSourceFileGenProps &gen_props)
{
	// Delete previous contents of the object if any.
	ResetGrammarSourceFile();

	// Do simple checks on the passed file.
	if (file.info.file_name == NULL)
	{
		ShowSetupError(console_msgs_prefix, cons, L"THtmlGrammarSourceFile: The passed file does not have a file name.");
		return(FALSE);
	}
	else if (file.info.file_body == NULL)
	{
		ShowSetupError(console_msgs_prefix, cons, L"THtmlGrammarSourceFile: Passed file \"%s\" does not have a file body.", file.info.file_name);
		return(FALSE);
	}
	else if (file.BuildLinesInfo() == FALSE)
	{
		ShowSetupError(console_msgs_prefix, cons, L"THtmlGrammarSourceFile: Unable to build lines info for \"%s\".", file.info.file_name);
		return(FALSE);
	}

	// Setup the length of the file body if it is missing.
	if (file.info.file_len < 0)
		file.info.file_len = (long)wcslen(file.info.file_body);

	// Do not show the last line of the file if this line is empty.
	int lines_to_emit = 0;
	if (file.lines_info_len >= 2)
	{
		lines_to_emit = file.lines_info_len;
		if (file.lines_info[file.lines_info_len-1].line_len == 0 && file.lines_info[file.lines_info_len-2].line_len == 0)
			lines_to_emit--;
	}

	wchar_t element_id_buff[60];
	PrepareGrmFileId(element_id_buff, 60, igrammar, file);

	// Prepare the decorated file props. Note that names of all styles are fixed.
	THtmlDecoratedFileProps dfl_props = { L"grm_src_line_num", L"grm_src_marks_area", L"grm_src_line_body", 0, 0, rdem_escape_tab_space };

	// Do the base class setup. Note that the ownership on the file data is not bypassed.
	if (Setup(dfl_props, element_id_buff, file.info.file_body, file.info.file_len, 0, 1, file.lines_info, file.lines_info_len, 1, lines_to_emit) == FALSE)
	{
		ShowSetupError(console_msgs_prefix, cons, L"THtmlGrammarSourceFile: Error setting up the file \"%s\" into the base class.", file.info.file_name);
		return(FALSE);
	}

	// Prepare and set the HTML id of the whole object and set the CSS class of the tables of the object.
	SetupElementId(element_id_buff);
	SetupCssClassForTable(L"grm_source_file_table");

	// Copy in the data fields.
	m_grm_file = file;
	m_grm_file_data_owned = bypass_file_info_data;

	// Pick up the requested grammar.
	TGrammar *requested_grammar = grdef_parser.DestGrammar().GetGrammarByIndex(igrammar);
	if (requested_grammar == NULL)
	{
		ShowSetupError(console_msgs_prefix, cons, L"THtmlGrammarSourceFile: Requested grammar G%d is missing in the passed tree of grammars.", igrammar);
		return(FALSE);
	}

	// Apply decorations.
	ApplyRawDecors(console_msgs_prefix, cons, disk_files, grdef_parser, requested_grammar, igrammar, gen_props);
	ApplyGrammarDecors(grdef_parser, requested_grammar, igrammar);
	ApplySpecTabDecors(console_msgs_prefix, cons);

	// Show info about errors, encountered during the decoration process.
	if (m_grm_temp_data_alloc_errors > 0)
		ShowSetupError(console_msgs_prefix, cons, L"THtmlGrammarSourceFile: Allocating temp data structures encountered %d problem%s.", m_grm_temp_data_alloc_errors, PluralSuffix(m_grm_temp_data_alloc_errors));
	if (m_grm_decor_owner_errors > 0)
		ShowSetupError(console_msgs_prefix, cons, L"THtmlGrammarSourceFile: Decoration owner allocation encountered %d problem%s.", m_grm_decor_owner_errors, PluralSuffix(m_grm_decor_owner_errors));
	if (m_grm_decor_errors > 0)
		ShowSetupError(console_msgs_prefix, cons, L"THtmlGrammarSourceFile: Creating decorations encountered %d problem%s.", m_grm_decor_errors, PluralSuffix(m_grm_decor_errors));

	// Evaluate and return the overall success.
	return(m_grm_decor_errors == 0 && m_grm_decor_owner_errors == 0 && m_grm_temp_data_alloc_errors == 0 && m_grm_other_reported_errors == 0);
}

void THtmlGrammarSourceFile::ApplyRawDecors(const wchar_t *console_msgs_prefix, TGenericConsole &cons, TList &disk_files, TGrammarDefinitionParser &grdef_parser,
												TGrammar *grm, int igrammar, THtmlGrammarSourceFileGenProps &gen_props)
{
	// Instantiate the raw scanner and run the loop over lexemas in the file.
	TStringsDict dict;
	TRawScanner scanner(&m_grm_file, &dict);

	TLexema lex_sect_hdr_beg;
	TGrammarSectionType curr_section = grsc_none;
	int xpct_defns_nesting = 0;
	bool prev_lex_undef = FALSE;

	TLexema lex_app_id_beg;
	TLexema lex_loc_obj_beg;
	for(;;)
	{
		TLexema lex;
		scanner.GetLex(lex);
		if (lex.type == ltx_eof)
			break;

		// Process lexemas that switch special raw scanning modes ON.
		if (lex.IsKeyword(mpr_include) == TRUE || lex.IsKeyword(mpr_line) == TRUE)
			scanner.SetSpecialFileNameMode(TRUE);
		if (lex_sect_hdr_beg.IsLex() == TRUE && lex.type == ltx_name)
			curr_section = GetGrammarDefnSectionType(lex.str_value);

		if (lex.IsKeyword(spr_lcurvbr) == TRUE)
		{
			xpct_defns_nesting++;
		}
		else if (lex.IsKeyword(spr_rcurvbr) == TRUE)
		{
			xpct_defns_nesting--;
			curr_section = grsc_none;
		}

		// Procedure 1. Pick up the CSS class for the picked lexema.
		const wchar_t *decor_class = lex.GetCssStyleName();

		// Procedure 2. Decorate the file names in the #include statements.
		if (lex.type == ltx_string && scanner.GetSpecialFileNameMode() == TRUE)
		{
			// String after the "#include" keyword should have a different decoration.
			decor_class = TLexema::GetIncFnStringCssStyleName();

			if (gen_props.m_add_hyper_links == TRUE || gen_props.m_add_dest_loc_info == TRUE)
			{
				// The link info is needed. Note that this is a link to the file in general, not to any specific line there.
				TFileInfo *dest_file_info = NULL;
				if (FindGrammarSourceFile(console_msgs_prefix, cons, disk_files, lex.str_value, dest_file_info) == TRUE)
					AddDestinationInfoDecor(gen_props, igrammar, dest_file_info, -1, lex.origin.src_area);
			}
		}

		if (prev_lex_undef == TRUE && lex.type == ltx_name)
		{
			// Show this name in a grey color.
			decor_class = L"grm_spec_cond_param";
		}

		// Procedure 3. Decorate the grammar section names. Include the angle brackets into the decoration.
		if (lex.IsKeyword(opr_lt) == TRUE)
		{
			// Store lexema that starts the area in the variable that is defined outside of the loop.
			lex_sect_hdr_beg = lex;
		}
		else if (lex.IsKeyword(opr_gt) == TRUE || lex.IsKeyword(spr_semicol) == TRUE)
		{
			if (lex_sect_hdr_beg.IsLex() == TRUE)
			{
				// This is an end of the grammar section header. Highlight the name of the section.
				const wchar_t *sect_name_decor_class = (xpct_defns_nesting == 0) ? L"grm_section_header" : L"grm_section_header_nested";
				long sect_name_beg = lex_sect_hdr_beg.origin.src_area.area_beg;
				ApplyFrgrDecor(sect_name_decor_class, grszo_elevated, sect_name_beg, lex.origin.src_area.AreaEnd()-sect_name_beg);
			}

			lex_sect_hdr_beg.Clear();
		}

		// Procedure 4. Highlight additional grammar defn language keywords.
		if (lex.IsName(L"warnings") == TRUE || lex.IsName(L"__warnings") == TRUE || lex.IsName(L"__public") == TRUE)
		{
			// These lexemas are keywords of the grammar defn language.
			TLexSubtype subt = { cpp_public };
			decor_class = TLexema::GetCssStyleName(ltx_keyword, subt);
		}

		// Procedure 5. Application Ids of the grammar objects.
		if (lex.IsName(L"__id") == TRUE)
		{
			// Store lexema that starts the area in the variable that is defined outside of the loop.
			lex_app_id_beg = lex;
		}
		else if (lex.IsKeyword(opr_rpar) == TRUE)
		{
			if (lex_app_id_beg.IsLex() == TRUE)
			{
				// Highlight or hide the app id. Note that hiding the app id is carried out by applying a NULL decor owner.
				const wchar_t *ident_obj_decor_class = (gen_props.m_show_app_ids == TRUE) ? L"grm_object_app_id" : NULL;
				long ident_obj_area_beg = lex_app_id_beg.origin.src_area.AreaBeg();
				ApplyFrgrDecor(ident_obj_decor_class, grszo_elevated, ident_obj_area_beg, lex.origin.src_area.AreaEnd()-ident_obj_area_beg);
			}

			lex_app_id_beg.Clear();
		}

		// Procedure 6. Non terminal attributes.
		if (lex.IsName(L"__seq") == TRUE || lex.IsName(L"__list") == TRUE)
		{
			// Replace the decoration class.
			decor_class = L"grm_non_term_restr";
		}

		// Procedure 7. Conflict location markers.
		if (lex.IsKeyword(opr_lbracket) == TRUE)
		{
			// Store lexema that starts the area in the variable that is defined outside of the loop.
			lex_loc_obj_beg = lex;
		}
		else if (lex.IsKeyword(opr_rbracket) == TRUE)
		{
			if (lex_loc_obj_beg.IsLex() == TRUE)
			{
				// Highlight the conflict location.
				long lex_loc_area_beg = lex_loc_obj_beg.origin.src_area.AreaBeg();
				ApplyFrgrDecor(L"grm_conflict_loc_marker", grszo_elevated, lex_loc_area_beg, lex.origin.src_area.AreaEnd()-lex_loc_area_beg);
			}

			lex_loc_obj_beg.Clear();
		}

		// Procedure 8. Symbols of the grammar.
		WORD sym = grm->FindSymbol(lex);
		if (sym != 0)
		{
			// Current lexema is describing some grammar symbol.
			if (sym < IgnoreLexTypeBase)
				decor_class = L"grm_terminal_symbol";
			else decor_class = L"grm_non_terminal";

			if (gen_props.m_add_hyper_links == TRUE || gen_props.m_add_dest_loc_info == TRUE)
			{
				// The link info is needed. Find the place where this symbol was defined.
				PrEventHeader *org_info = NULL;
				if (grdef_parser.GetPreparedGrammarSymbolOrigin(org_info, igrammar, sym) == TRUE)
					AddDestinationInfoDecor(gen_props, igrammar, org_info->src_area.file_info, org_info->src_area.area_beg, lex.origin.src_area);
			}
		}

		// Procedure 9. Names of the expected conflicts and conflict location names in the xpct conflict definitions.
		if (lex.type == ltx_name && lex.str_value.m_len <= TGrammar::MAX_NAME_LENGTH)
		{
			wchar_t name_buffer[TGrammar::MAX_NAME_LENGTH+1];
			lex.str_value.CopyToVerifiedBuffer(name_buffer);

			int ixpct = grm->FindExpectedConflict(name_buffer);
			if (ixpct >= 0)
			{
				// This is the name of an xpct conflict. Highlight this name.
				decor_class = (xpct_defns_nesting == 0) ? L"grm_xpct_name" :  L"grm_xpct_name_nested";

				// Show an index of the xpct conflict if possible.
				int line_num = m_grm_file.GetLineNumber(lex.origin.src_area.AreaBeg());
				if (line_num > 0 && IsMarksAeaEmpty(line_num) == TRUE)
				{
					wchar_t xpct_label[20];
					swprintf(xpct_label, 20, L"X%d", ixpct);
					AddLineMark(L"grm_xpct_mark", xpct_label, line_num);
				}
			}

			int iloc = grm->FindConflictLocationObject(name_buffer);
			if (iloc >= 0 && curr_section == grsc_locations)
			{
				// This is the name of a conflict location object inside the xpct conflict definition.
				decor_class = L"grm_conflict_loc_marker";
			}
		}

		// Procedure 10. Comments inside the xpct conflict definitions.
		if (xpct_defns_nesting > 0 && lex.IsComment(lct_cpp_wholeline))
		{
			// Show this comment in a smaller font.
			TLexSubtype subt = { lct_cpp_endofline };
			decor_class = TLexema::GetCssStyleName(ltx_comment, subt);
		}

		//
		//  Apply the resulting decoration if any.
		//
		if (decor_class != NULL)
		{
			ApplyFrgrDecor(decor_class, grszo_normal, lex.origin.src_area.area_beg, lex.origin.src_area.area_len);
		}

		// Process lexemas that switch special raw scanning modes OFF.
		if (lex.IsEol() == TRUE)
			scanner.SetSpecialFileNameMode(FALSE);

		// This flag is needed to show the name after the #undef in a dim color.
		prev_lex_undef = lex.IsKeyword(mpr_undef);
	}
}

void THtmlGrammarSourceFile::ApplyGrammarDecors(TGrammarDefinitionParser &parser, TGrammar *grm, int igrammar)
{
	// Procedure 1. Iterate terminal symbols. Do not include EOF symbol into the iteration.
	for (int isym1=1; isym1<grm->NumTerminals(); ++isym1)
	{
		PrEventHeader *org_info = NULL;
		WORD sym = TerminalSymbolsBase+isym1;
		if (parser.GetPreparedGrammarSymbolOrigin(org_info, igrammar, sym) == TRUE)
		{
			AddGrammarSymbolLabel(sym, org_info->src_area);
		}

		TSourceArea &defn_area = org_info->src_area;
		if (defn_area.file_info->doc_id == m_grm_file.doc_id)
		{
			// This symbol was defined in the current file. Highlight it with a special CSS class with a bigger
			// font. This will also to prevent showing a finger cursor when the mouse is hovered over it.
			ApplyFrgrDecor(L"grm_terminal_symbol_defn", grszo_elevated, defn_area.area_beg, defn_area.area_len);
		}
	}

	// Procedure 2. Iterate non terminals. All non terms should be iterated.
	for (int isym2=0; isym2<grm->NumNonTerminals(); ++isym2)
	{
		PrEventHeader *org_info = NULL;
		WORD sym = NonTerminalsBase+isym2;
		if (parser.GetPreparedGrammarSymbolOrigin(org_info, igrammar, sym) == TRUE)
		{
			AddGrammarSymbolLabel(sym, org_info->src_area);
		}
	}

	// Procedure 3. Iterate grammar rules.
	for (int irule=0; irule<grm->NumRules(); ++irule)
	{
		TGrammarRuleOrigin *org_info = NULL;
		if (parser.GetPreparedGrammarRuleOrigin(org_info, igrammar, irule) == TRUE)
		{
			TSourceArea &rule_intro_area = org_info->rule_origin.src_area;
			if (rule_intro_area.file_info->doc_id == m_grm_file.doc_id)
			{
				// This rule was introduced in the current file.
				int line_num = m_grm_file.GetLineNumber(rule_intro_area.AreaBeg());
				if (line_num > 0 && IsMarksAeaEmpty(line_num) == TRUE)
				{
					// This is the first rule for this line.
					wchar_t rule_label[20];
					swprintf(rule_label, 20, L"R%d", irule);
					AddLineMark(L"grm_rule_mark", rule_label, line_num);
				}
			}

			TSourceArea &non_term_area = org_info->non_term_origin.src_area;
			if (non_term_area.file_info->doc_id == m_grm_file.doc_id)
			{
				// Rule non terminal also belongs to the current file. Highlight it with the special decoration.
				// Note that the rule non terminal and the rule intro symbol are the same for conflict resolution
				// rules. Rule non terminals should be highlighted only in the ordinary rules. Check for this.
				if (non_term_area.AreaBeg() != rule_intro_area.AreaBeg())
				{
					// Rule non terminal differs from the rule intro symbol.
					ApplyFrgrDecor(L"grm_rule_non_term", grszo_elevated, non_term_area.area_beg, non_term_area.area_len);
				}
			}
		}
	}
}

void THtmlGrammarSourceFile::ApplySpecTabDecors(const wchar_t *console_msgs_prefix, TGenericConsole &cons)
{
	// Instantiate two different processors.
	TSpecTabDefinitionRangesProcessor defn_ranges_proc(*this, m_grm_decor_errors, m_grm_temp_data_alloc_errors);
	TSpecTabGrammarLanguageProcessor grm_lang_proc(*this, m_grm_decor_errors, m_grm_temp_data_alloc_errors);

	// Iterate lexemas in the file.
	TStringsDict dict;
	TRawScanner scanner(&m_grm_file, &dict);

	TLexema lex;
	while (lex.IsEof() == FALSE)
	{
		scanner.GetLex(lex);
		grm_lang_proc.ProcessLexema(lex);

		// The defn ranges processor should be active only when the grammar section is not defined.
		if (grm_lang_proc.CurrSection() == grsc_none)
			defn_ranges_proc.ProcessLexema(lex);
		else if (defn_ranges_proc.IsLayoutPresent() == TRUE)
			defn_ranges_proc.ApplyAccumulatedLayout();
	}
}

bool THtmlGrammarSourceFile::FindGrammarSourceFile(const wchar_t *console_msgs_prefix, TGenericConsole &cons, TList &disk_files, TStrPtrInfo &name, TFileInfo *&result)
{
	assert(result == NULL);

	// The number of files in the grammar project should be not big. Iterate all files.
	int cnt_matches = 0;
	for (TListIter<TFileInfoItem> iter(disk_files); iter; ++iter)
	{
		TFileInfo &file_info = iter.CurrItem().data;
		int file_name_len = wcslen(file_info.info.file_name);

		if (file_name_len == name.m_len)
		{
			// The lengths of the names are the same.
			if (_wcsnicmp(file_info.info.file_name, name.m_body, file_name_len) == 0)
			{
				result = &file_info;
				cnt_matches++;
			}
		}
		else if (file_name_len > name.m_len)
		{
			int len_diff = file_name_len-name.m_len;
			wchar_t chfr = file_info.info.file_name[len_diff-1];
			if ((chfr == L':' || chfr == L'\\' || chfr == L'/') && _wcsnicmp(file_info.info.file_name+len_diff, name.m_body, name.m_len) == 0)
			{
				result = &file_info;
				cnt_matches++;
			}
		}
	}

	if (cnt_matches < 1)
	{
		ShowSetupError(console_msgs_prefix, cons, L"THtmlGrammarSourceFile: The name of the include file \"%.*s\" is not present in the list of files being dumped.", name.m_len, name.m_body);
		m_grm_other_reported_errors++;
	}
	else if (cnt_matches > 1)
	{
		ShowSetupError(console_msgs_prefix, cons, L"THtmlGrammarSourceFile: The name of the include file \"%.*s\" is ambiguous in the list of files being dumped.", name.m_len, name.m_body);
		m_grm_other_reported_errors++;
	}

	// Return success only if there is only one match.
	return(cnt_matches == 1);
}

void THtmlGrammarSourceFile::AddDestinationInfoDecor(THtmlGrammarSourceFileGenProps &gen_props, int igrammar, TFileInfo *dest_file, long dest_area_beg, TSourceArea &link_area)
{
	// Ensure that information on the dest file is complete.
	assert(dest_file != NULL && dest_file->info.file_name != NULL);
	assert(dest_file->lines_info != NULL && dest_file->lines_info_len > 0);

	// Pick up the line of the link area and ensure that it belongs to the source file.
	long source_line = link_area.file_info->GetLineNumber(link_area.AreaBeg());
	assert(source_line > 0 && source_line <= link_area.file_info->lines_info_len);

	// The dest area offset can be negative. It is ok to pass such offset into the function below.
	dest_file->BuildLinesInfo();
	long dest_line = dest_file->GetLineNumber(dest_area_beg);

	wchar_t dest_sect_name[60];
	PrepareGrmFileId(dest_sect_name, 60, igrammar, *dest_file);

	if (gen_props.m_add_hyper_links == TRUE)
	{
		// Prepare the name of the stand alone HTML page that descibes the destination file. Set the line number
		// to navigate to with a certain offset to provide some context above the line.
		wchar_t dest_xgr_fname_buffer[MAX_PATH+80], dest_line_buffer[60];
		PrepareXgrFileName(dest_xgr_fname_buffer, MAX_PATH+80, igrammar, dest_file->info.file_name);
		swprintf(dest_line_buffer, 60, L"%sL%d", dest_sect_name, (dest_line > 3) ? dest_line-3 : dest_line);

		if (link_area.file_info->doc_id != dest_file->doc_id)
		{
			// Dest file differs from the current file.
			if (dest_line > 0)
			{
				// Use the full link.
				wchar_t link_buff[MAX_PATH+160];
				swprintf(link_buff, 80, L"%s#%s", dest_xgr_fname_buffer, dest_line_buffer);
				ApplyHyperLinkDecor(link_buff, link_area.area_beg, link_area.area_len);
			}
			else
			{
				// Use a short form of the link.
				ApplyHyperLinkDecor(dest_xgr_fname_buffer, link_area.area_beg, link_area.area_len);
			}
		}
		else if (source_line != dest_line)
		{
			// The file is the same, but the line number is different. Use a short form of the link.
			wchar_t link_buff[80];
			swprintf(link_buff, 80, L"#%s", dest_line_buffer);
			ApplyHyperLinkDecor(link_buff, link_area.area_beg, link_area.area_len);
		}
	}

	if (gen_props.m_add_dest_loc_info == TRUE)
	{
		wchar_t dest_element_name[60];
		swprintf(dest_element_name, 60, L"%sL%d", dest_sect_name, dest_line);

		if (link_area.file_info->doc_id != dest_file->doc_id)
		{
			// Dest file differs from the current file.
			if (dest_line > 0)
			{
				// Use the full loc info.
				ApplyDestLocDecor(dest_sect_name, dest_element_name, link_area.area_beg, link_area.area_len);
			}
			else
			{
				// Use a short form of the loc info.
				ApplyDestLocDecor(dest_sect_name, NULL, link_area.area_beg, link_area.area_len);
			}
		}
		else if (source_line != dest_line)
		{
			// The file is the same, but the line number is different.  Use a short form of the loc info.
			ApplyDestLocDecor(NULL, dest_element_name, link_area.area_beg, link_area.area_len);
		}
	}
}

void THtmlGrammarSourceFile::AddGrammarSymbolLabel(WORD sym_value, TSourceArea &sym_area)
{
	if (sym_area.file_info->doc_id == m_grm_file.doc_id)
	{
		// This symbol was introduced in the current file.
		int line_num = m_grm_file.GetLineNumber(sym_area.AreaBeg());
		if (line_num > 0 && IsMarksAeaEmpty(line_num) == TRUE)
		{
			// The line number is present and the marks area is not occupied on this line.
			wchar_t symbol_value_label[20];
			swprintf(symbol_value_label, 20, L"%hd", sym_value);
			AddLineMark(L"grm_symbol_mark", symbol_value_label, line_num);
		}
	}
}

void THtmlGrammarSourceFile::ApplyFrgrDecor(const wchar_t *decor_class, TGrammmarSourceZorder zorder, long area_beg, long area_len)
{
	// Note that the passed decor class can be NULL. This special sace is used when it is necessary to exclude
	// some part of the source file from rendering. When a frgr decoration is set with a NULL owner and this
	// decoration turns out to be on top, the decorated area is not placed into the resulting HTML.
	TRawHtmlElement *frgr_owner = NULL;
	if (decor_class != NULL)
	{
		frgr_owner = m_grm_decor_owners.FindOrCreateSimpleOwner(decor_class);
		if (frgr_owner == NULL)
			return;
	}

	if (SetFrgrDecoration(frgr_owner, zorder, area_beg, area_len) == FALSE)
		m_grm_decor_errors++;
}

void THtmlGrammarSourceFile::ApplyHyperLinkDecor(const wchar_t *link_href, long area_beg, long area_len)
{
	TRawHtmlElement *hyper_link_owner = m_grm_decor_owners.CreateHyperLinkOwner(link_href);
	if (hyper_link_owner == NULL)
		return;

	if (SetBkgrDecoration(hyper_link_owner, grszo_navigation_decors, area_beg, area_len) == FALSE)
		m_grm_decor_errors++;
}

void THtmlGrammarSourceFile::ApplyDestLocDecor(const wchar_t *section_ref, const wchar_t *element_ref, long area_beg, long area_len)
{
	const wchar_t *attr1_name = (section_ref != NULL) ? L"Sect" : NULL;
	const wchar_t *attr2_name = (element_ref != NULL) ? L"Elem" : NULL;

	TRawHtmlElement *dest_loc_owner = m_grm_decor_owners.CreateAppAttrsOwner(attr1_name, section_ref, attr2_name, element_ref);
	if (dest_loc_owner == NULL)
		return;

	if (SetBkgrDecoration(dest_loc_owner, grszo_navigation_decors, area_beg, area_len) == FALSE)
		m_grm_decor_errors++;
}

void THtmlGrammarSourceFile::AddLineMark(const wchar_t *decor_class, const wchar_t *decor_text, long line_number)
{
	TRawHtmlElement *mark_object = m_grm_decor_owners.CreateMarksAreaLabel(decor_text, decor_class);
	if (mark_object == NULL)
		return;

	if (SetMarksAreaDecoration(line_number, grszo_normal, mark_object) == FALSE)
		m_grm_decor_errors++;
}

void THtmlGrammarSourceFile::PrepareGrmFileId(wchar_t *buffer, int buff_len, int igrammar, TFileInfo &file)
{
	swprintf(buffer, buff_len, L"G%d", igrammar);
	int grm_num_len = wcslen(buffer);
	file.PrepareFileIdShort(buffer+grm_num_len, buff_len-grm_num_len);
}

void THtmlGrammarSourceFile::PrepareXgrFileName(wchar_t *buffer, int buff_len, int igrammar, const wchar_t *file_name)
{
	// Extract the short name without extension.
	TFileNameBuffer short_fname;
	if (TPathHelper::ExtractShortName(short_fname, file_name, TRUE) == FALSE)
		short_fname.Set(L"Error-Getting-Short-Name");

	// Format resulting name in the buffer.
	swprintf(buffer, buff_len, L"xgr_%02d_%s.HTML", igrammar, short_fname.DataPtr());
}

void THtmlGrammarSourceFile::ShowSetupError(const wchar_t *msgs_prefix, TGenericConsole &cons, const wchar_t *format, ...)
{
	// Setup the prefix in the beginning of the buffer.
	wchar_t err_buffer[40+2048], *ptr_err_buff = err_buffer;
	if (msgs_prefix != NULL && msgs_prefix[0] != 0)
	{
		int prefix_len = wcslen(msgs_prefix);
		assert(prefix_len < 40);

		wcscpy(ptr_err_buff, msgs_prefix);
		ptr_err_buff += prefix_len;
	}

	// Format the error message.
	va_list vargs;
	va_start(vargs, format);
	vswprintf(ptr_err_buff, 2048, format, vargs);
	va_end(vargs);

	// Show the message.
	cons.HandleTrace(err_buffer, TGenericConsole::ErrorHighlightMode);
}


