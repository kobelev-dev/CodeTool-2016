//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	Processing_HtmlReportHelper_H
#define	Processing_HtmlReportHelper_H

#ifndef   Common_GenericConsole_H
#include  "Common/GenericConsole.H"
#endif
#ifndef   Xml_HtmlGenCore_H
#include  "Xml/HtmlGenCore.H"
#endif

class THtmlReportHelper
{
public:
			THtmlReportHelper(TGenericConsoleEx &cons) : m_helper_console(cons) { }

	typedef void (THtmlReportHelper::*HtmlBodyWriterMethod)(THtmlDocument &rprt);

	bool		WriteHtmlReport(const wchar_t *report_dir, const wchar_t *report_fname, bool want_doc_fragment, HtmlBodyWriterMethod body_writer = NULL);
	void		WriteEmptyTableMessage(THtmlDocument &rprt, const wchar_t *colspan, const wchar_t *message, bool want_bold_msg = FALSE);

protected:

	virtual void	WriteHtmlReportBody(THtmlDocument &rprt) { }
					// This is a default writer method. It is used when the WriteHtmlReport() is called without the last parameter.

	void		GeneratePaddingCells(THtmlDocument &rprt, int current_layer, int max_padding_columns, const wchar_t *padding_cell_css_style);
				// The layers counting starts from zero.

	void		OpenAlignedCell(THtmlDocument &rprt, int current_layer, int max_padding_columns, const wchar_t *cell_css_style);
	void		CloseAlignedCell(THtmlDocument &rprt);

	TGenericConsoleEx		&m_helper_console;
};

#endif	// Processing_HtmlReportHelper_H


