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

#include  "Common/FormatNumber.H"
#include  "Processing/HtmlReportHelper.H"

// -----------------------------------------------------------------------------
//	====================  THtmlReportHelper  =======================
// -----------------------------------------------------------------------------

bool THtmlReportHelper::WriteHtmlReport(const wchar_t *report_dir, const wchar_t *report_fname, bool want_doc_fragment, HtmlBodyWriterMethod body_writer)
{
	wchar_t msg_buffer[360];
	THtmlDocument rprt(NULL, want_doc_fragment);

	// Prepare the file name and open the report file for writing.
	const wchar_t *rprt_ext = (want_doc_fragment == TRUE) ? L".HTML_DIV" : L".HTML";
	if (TPathHelper::PrepareDestFile(msg_buffer, 360, rprt, report_dir, NULL, report_fname, rprt_ext, L"analysis report") == FALSE)
	{
		m_helper_console.HandleTrace(msg_buffer, TGenericConsole::ErrorHighlightMode);
		return(FALSE);
	}

	if (want_doc_fragment == FALSE)
	{
		// Emit the top part of the HTML page.
		rprt.OpenDocumentHead();
		rprt.AddCssFileRef(NULL, L"Wbs_AnalysisReports.CSS");
		rprt.OpenDocumentBody();
	}

	// Do the job.
	if (body_writer == NULL)
		WriteHtmlReportBody(rprt);
	else (this->*body_writer)(rprt);

	if (want_doc_fragment == FALSE)
	{
		// Emit the bottom part of the HTML page.
		rprt.CloseDocument();
	}

	// Close the report. This method will also check for buffering/writing errors.
	if (TPathHelper::CloseDestFile(msg_buffer, 360, rprt) == FALSE)
	{
		m_helper_console.HandleTrace(msg_buffer, TGenericConsole::ErrorHighlightMode);
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

void THtmlReportHelper::WriteEmptyTableMessage(THtmlDocument &rprt, const wchar_t *colspan, const wchar_t *message, bool want_bold_msg)
{
	// This message with be displayed using the inherited style.
	TArrayBasedHtmlContainer msg_row(L"tr"), msg_cell2(L"td");
	TSimpleHtmlElement msg_cell1(L"td"), msg_br(L"br", NULL, 0, NULL, crlm_none);
	TSimpleHtmlElement msg_body((want_bold_msg == TRUE) ? L"b" : NULL, message, -1, NULL, crlm_none);
	msg_cell2.SetFirstAppAttr(L"colspan", colspan);
	msg_cell2.AppendElements(&msg_br, &msg_body);
	msg_row.AppendElements(&msg_cell1, &msg_cell2);
	rprt.EmitElement(msg_row);
}

void THtmlReportHelper::GeneratePaddingCells(THtmlDocument &rprt, int current_layer, int max_padding_columns, const wchar_t *padding_cell_css_style)
{
	if (current_layer > max_padding_columns)
		current_layer = max_padding_columns;

	TSimpleHtmlElement indent_cell(L"td", NULL, 0, padding_cell_css_style, crlm_none);
	for (int ic=0; ic<current_layer; ++ic)
		rprt.EmitElement(indent_cell);
}

void THtmlReportHelper::OpenAlignedCell(THtmlDocument &rprt, int current_layer, int max_padding_columns, const wchar_t *cell_css_style)
{
	TAdvancedHtmlElement body_cell(L"td", NULL, 0, cell_css_style, crlm_before_whole_obj);

	wchar_t colspan_buff[40];
	if (current_layer < max_padding_columns)
		body_cell.SetFirstAppAttr(L"colspan", _itow(max_padding_columns+1-current_layer, colspan_buff, 10));

	rprt.EmitElementBeg(body_cell);
}

void THtmlReportHelper::CloseAlignedCell(THtmlDocument &rprt)
{
	TSimpleHtmlElement body_cell(L"td", NULL, 0, NULL, crlm_after_whole_obj);
	rprt.EmitElementEnd(body_cell);
}


