//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_LexItemsXml_H
#define	LangView_LexItemsXml_H

#ifndef   LangView_LexItems_H
#include  "LangView/LexItems.H"
#endif
#ifndef   Common_XmlReader_H
#include  "Xml/XmlReader.H"
#endif
#ifndef   Common_XmlWriter_H
#include  "Xml/XmlWriter.H"
#endif

enum TLexItemXmlTag
{
	lixt_none = 0,

	//
	// <LexItemStyleInfo>
	//

	lixt_sym_index,

	lixt_cnct_icon,
	lixt_pcb_icon,

	lixt_main_bkgr_style,
	lixt_pcb_lower_style,
	lixt_pcb_lo_trasp_nterm,
	lixt_pcb_lo_opaq_nterm,

	lixt_main_label_style,
	lixt_symbol_label_style,

	lixt_cnct_circle_diam,
	lixt_cnct_main_lab_shx,
	lixt_cnct_main_lab_shy,
	lixt_cnct_sym_lab_shx,
	lixt_cnct_sym_lab_shy,

	lixt_pcb_max_rectw,
	lixt_pcb_mlab_margins,
	lixt_pcb_mlab_down_sh,
	lixt_pcb_mlab_lo_space,
	lixt_pcb_symlab_lmarg,
	lixt_pcb_symlab_rmarg,
	lixt_pcb_lo_rect_height,

	//
	// <LexItemStyles>
	//

	lixt_low_level_styles,
	lixt_lexema_style,
	lixt_keyword_style,
	lixt_mid_lev_obj_style,
	lixt_non_term_style,
	lixt_spec_item_style,
	lixt_vpars_item_style,
	lixt_cpp_item_style,
};

class TLangViewLexItemStylePropsXmlHandler;

class TLangViewLexItemStyleInfoXmlHandler : public TXmlContainerHandler
{
public:

			TLangViewLexItemStyleInfoXmlHandler();
	void		SetupHandler(TLangViewLexItemStylePropsXmlHandler *owner, TLexItemXmlTag list_type);

	virtual void		BeginNodeProcessing(WORD tagId);
	virtual void		FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems);

	virtual void		ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName);
	virtual void		ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName);

protected:

	void				ResolveSymbolName(TStringPtr &strValue, TStringPtr &tagName);
	void				ResolveBasicStyleName(TBasicStyle *&style, TStringPtr &strValue, TStringPtr &tagName);
	void				ResolveTextStyleName(TTextStyle *&style, TStringPtr &strValue, TStringPtr &tagName);

	TLexItemStyleInfo		*GetStyleInfoArrayPtr(TLexItemXmlTag list_type);
	int					GetStyleInfoArrayLen(TLexItemXmlTag list_type);

	void					ApplyExplicitFields(TLexItemStyleInfo *dest);

protected:

	TLangViewLexItemStylePropsXmlHandler		*m_owner;
	TLexItemXmlTag							m_list_type;

	int										m_sym_value;			// Value of the field is non negative when symbol name is successfully resolved.
	bool										m_any_symbol;			// Value of the field is TRUE when symbol name is "Any" or "All".

	TLexItemStyleInfo							m_buffer;
	TLexItemStyleInfo							m_buffer_flags;

	TBasicStyle								*m_transp_nterm_lo_bkgr;
	TBasicStyle								*m_opaque_nterm_lo_bkgr;
};

class TLangViewLexItemStylePropsXmlHandler : public TXmlContainerHandler
{
public:
			TLangViewLexItemStylePropsXmlHandler(TLangViewLexItemStyle *dest, int dim_grade, bool highlight_non_terms_transp);

	bool		LoadFromExeDirectory(const wchar_t *short_fname, TGenericConsole *cons = NULL, TDateTime *latest_scanned_file_date = NULL);
				// It is ok to pass the NULL console when error messages are not needed. When pointer to the latest scanned file date is NULL,
				// scanning errors are sent to the console all the time. Otherwise errors are sent only when date of the file differs from the passed
				// date of the last scan.

	bool		ResolveFixedBasicStyleName(TBasicStyle *&style, const wchar_t *style_name);
	bool		ResolveFixedTextStyleName(TTextStyle *&style, const wchar_t *style_name);
	bool		ResolveFixedLineStyleName(TLineStyle *&style, const wchar_t *style_name);
				// If requested style is missing or in case of the allocation error these methods are not changing the value
				// of the passed style pointer. Return value is FALSE only in case of the allocation error.
protected:

	virtual TXmlContainerHandler	*OpenSubContainer(WORD tagId, TStringPtr &tagName);

	virtual void	FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems);

protected:

	TLangViewLexItemStyle						*m_dest_style;

	int										m_dim_grade;
	bool										m_highlight_non_terms_transp;

	TScreenItemStylePropsCollection				m_low_level_styles;
	TLangViewLexItemStyleInfoXmlHandler		m_style_info_handler;

	friend class TLangViewLexItemStyleInfoXmlHandler;
};

#endif	// LangView_LexItemsXml_H


