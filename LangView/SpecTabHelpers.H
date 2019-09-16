//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_SpecTabHelpers_H
#define	LangView_SpecTabHelpers_H

#ifndef   Common_Buffers_H
#include  "Common/Buffers.H"
#endif
#ifndef   TextScan_MidScan_H
#include  "TextScan/MidScan.H"
#endif
#ifndef   Xml_HtmlGenDecors_H
#include  "Xml/HtmlGenDecors.H"
#endif

struct TSpecTabPosInfo
{
	enum { MAX_TAB_VAL = 12 };

	TLexemaType		m_lext;
	TLexSubtype		m_lex_subt;

	long				m_src_offs;

	long		m_char_offs[MAX_TAB_VAL+1];
				// Element [0] in this array is not used. Other elements describe the offset from the beg
				// of the line for this lexema according to different possible values of the tab expansion.
};

typedef TStructsArray<TSpecTabPosInfo, 40, 160> TSpecTabPosInfosArray;

struct TSpecTabLineLayout
{
	TSimpleLineInfo				m_line_info;
	TSpecTabPosInfosArray			m_spec_tabs;

public:

	bool		IsEmpty() const { return(m_spec_tabs.NumItems() == 0); }
	void		Clear() { m_line_info.line_offs = m_line_info.line_len = 0; m_spec_tabs.Clear(); }

	bool AddSpecTabPosInfo(TLexema &lex)
	{
		TSpecTabPosInfo info = { lex.type, lex.subtype, lex.origin.src_area.area_beg };
		return(m_spec_tabs.AppendItem(info));
	}

	bool AddSpecTabPosInfo(TLexEndOfLineType subt, PrEventHeader &origin)
	{
		TSpecTabPosInfo info = { ltx_eol, subt, origin.src_area.area_beg };
		return(m_spec_tabs.AppendItem(info));
	}

	void		FixObjectMove() { m_spec_tabs.FixObjectMove(); }
	void		ReleaseObject() { m_spec_tabs.ReleaseObject(); }
	void		WipeObject() { m_spec_tabs.WipeObject(); }
};

typedef TStructsArrayEx2<TSpecTabLineLayout, 512, 2048> TSpecTabLineLayoutsArray;

struct TSpecTabLinesRangeLayout
{
	TSpecTabLinesRangeLayout() { m_lines_range_beg = -1; }

	long							m_lines_range_beg;
									// This structure describes layout of several consequitive lines. The field above defines
									// the beginning of this range of lines. The length of the lines range ie equal to the length
									// of the array below.

	TSpecTabLineLayoutsArray		m_lines_layout;

public:

	bool		IsEmpty() const { return(m_lines_layout.NumItems() == 0); }
	void		Clear() { m_lines_range_beg = -1; m_lines_layout.Clear(); }

	bool		AddLineInfo(long line_number, TSpecTabLineLayout &line_info);

	void		ReleaseLineInfosFromTop(int num_lines);
	void		ReleaseLineInfosFromBottom(int num_lines);

	int		ApplyLayout(THtmlDecoratedFile &dest);
				// Return value is the count of errors that happened while applying decorations.
};

class TSpecTabsBasicProcessor
{
public:
			TSpecTabsBasicProcessor(THtmlDecoratedFile &rprt, int &cnt_apply_decor_errors, int &cnt_temp_struct_alloc_errors);
			~TSpecTabsBasicProcessor() { assert(m_curr_line_layout.IsEmpty() == TRUE && m_accumulated_layout.IsEmpty() == TRUE); }

	bool		IsLayoutPresent() { return(m_accumulated_layout.m_lines_layout.NumItems() > 0); }

	void		ProcessLexema(TLexema &lex);
				// Application is responsible for calling this method for EVERY lexema in the source file
				// including the EOF lexema.

	virtual	void		ProcessNonEmptyLexema(bool front_ext, TLexema &lex, bool back_ext) { }
						// Application is expected to modify the layout of the current line.

	virtual	void		ProcessEndOfLine(TLexEndOfLineType lex_eol_subt, PrEventHeader &lex_eol_origin) { }
						// All values of the enum are possible. In particular the leolt_num_eol_types value means that the line
						// ended inside the multiline lexema. Application is expected either to add the layout of the current line
						// to the accumulated layout and/or to modify/apply it. It may also discard the layout.

	virtual	void		ApplyAccumulatedLayout() = 0;
						// Application should dump the layout and reset it. Resetting the layout may require app level actions.

protected:

	void AddLexemaToCurrLineLayout(TLexema &lex)
	{
		if (m_curr_line_layout.AddSpecTabPosInfo(lex) == FALSE)
			m_temp_struct_alloc_errors_count++;
	}

	void AddEolToCurrLineLayout(TLexEndOfLineType lex_eol_subt, PrEventHeader &lex_eol_origin)
	{
		if (m_curr_line_layout.AddSpecTabPosInfo(lex_eol_subt, lex_eol_origin) == FALSE)
			m_temp_struct_alloc_errors_count++;
	}

	bool SaveCurrentLineLayout()
	{
		if (m_accumulated_layout.AddLineInfo(m_curr_line_num, m_curr_line_layout) == FALSE)
		{
			ApplyAccumulatedLayout();
			m_temp_struct_alloc_errors_count++;
			return(FALSE);
		}

		// Success.
		return(TRUE);
	}

protected:

	THtmlDecoratedFile				&m_dest_file;
	int							&m_apply_decor_errors_count;
	int							&m_temp_struct_alloc_errors_count;

	//
	// Information about the current line.
	//

	int							m_curr_line_num;
	TSpecTabLineLayout			m_curr_line_layout;

	//
	// Information about the upper lines.
	//

	TSpecTabLinesRangeLayout		m_accumulated_layout;
};

class TSpecTabDefinitionRangesProcessor : public TSpecTabsBasicProcessor
{
public:
			TSpecTabDefinitionRangesProcessor(THtmlDecoratedFile &rprt, int &cnt_apply_decor_errors, int &cnt_temp_struct_alloc_errors);

	void		ProcessNonEmptyLexema(bool front_ext, TLexema &lex, bool back_ext);
	void		ProcessEndOfLine(TLexEndOfLineType lex_eol_subt, PrEventHeader &lex_eol_origin);
	void		ApplyAccumulatedLayout();

protected:

	enum TDefsRangeLineType
	{
		dplt_none,
		dplt_defn_line,
		dplt_defn_cont,
		dplt_breaker,
	};

	// Info about the current line.
	TDefsRangeLineType			m_line_type;
	int							m_cnt_line_lexemas;
	bool							m_prev_spec_eol;			// Previous line ended with a backslash before the EOL
															// or the end of the line was inside a multiline lexema.
	// Info about the lines above.
	int							m_latest_defns;
	int							m_latest_nbrks;
};

class TSpecTabGrammarLanguageProcessor : public TSpecTabsBasicProcessor
{
public:
			TSpecTabGrammarLanguageProcessor(THtmlDecoratedFile &rprt, int &cnt_apply_decor_errors, int &cnt_temp_struct_alloc_errors);

	TGrammarSectionType	CurrSection() const { return((m_sect_header_line == FALSE) ? m_curr_section : grsc_num_types); }

	void		ProcessNonEmptyLexema(bool front_ext, TLexema &lex, bool back_ext);
	void		ProcessEndOfLine(TLexEndOfLineType lex_eol_subt, PrEventHeader &lex_eol_origin);
	void		ApplyAccumulatedLayout();

protected:

	void		FinalizeAndApplyRulesLayout();

protected:

	bool							m_sect_hdr_beg_flag;
	TGrammarSectionType			m_sect_hdr_section;
	bool							m_sect_header_line;

	bool							m_xpct_name_noticed;
	int							m_xpct_conflicts_nesting;

	TGrammarSectionType			m_curr_section;
	int							m_cnt_line_lexemas;
};

#endif	// LangView_SpecTabHelpers_H


