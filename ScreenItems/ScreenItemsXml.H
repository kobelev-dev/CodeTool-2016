//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	ScreenItems_ScreenItemsXml_H
#define	ScreenItems_ScreenItemsXml_H

#ifndef   ScreenItems_ScreenItems_H
#include  "ScreenItems/ScreenItems.H"
#endif
#ifndef   Common_XmlReader_H
#include  "Xml/XmlReader.H"
#endif
#ifndef   Common_XmlWriter_H
#include  "Xml/XmlWriter.H"
#endif

enum TScreenItemPropsXmlTags
{
	scxt_none,

	//
	//  BasicStyleProps.
	//
	scxt_basic_css_style_name,				// The CSS style name is only dumped.
	scxt_basic_background_color,
	scxt_basic_shadow_up_color,
	scxt_basic_shadow_up_width,
	scxt_basic_shadow_down_color,
	scxt_basic_shadow_down_width,
	scxt_basic_frame_color,
	scxt_basic_frame_width,
	scxt_basic_frame_offset,
	scxt_basic_frame_style,
	scxt_basic_frame_shadow_up_color,
	scxt_basic_frame_shadow_up_width,
	scxt_basic_frame_shadow_down_color,
	scxt_basic_frame_shadow_down_width,
	scxt_basic_frame_in_marg_horz,
	scxt_basic_frame_in_marg_left,
	scxt_basic_frame_in_marg_right,
	scxt_basic_frame_in_marg_vert,
	scxt_basic_frame_in_marg_top,
	scxt_basic_frame_in_marg_bottom,
	scxt_basic_frame_out_pddn_horz,
	scxt_basic_frame_out_pddn_left,
	scxt_basic_frame_out_pddn_right,
	scxt_basic_frame_out_pddn_vert,
	scxt_basic_frame_out_pddn_top,
	scxt_basic_frame_out_pddn_bottom,

	//
	//  LineStyleProps
	//
	scxt_line_foreground_color,
	scxt_line_line_width,

	//
	//  TextStyleProps.
	//
	scxt_text_css_style_name,				// The CSS style name is only dumped.
	scxt_text_font_name,
	scxt_text_font_height,
	scxt_text_is_bold,
	scxt_text_foreground_color,
	scxt_text_background_color,
	scxt_text_frame_color,
	scxt_text_frame_corner_style,			// enum TFrameCornerStyle.
	scxt_text_shadow_down_color,
	scxt_text_shadow_down_width,
	scxt_text_up_side_adjust,
	scxt_text_baseline_adjust,
	scxt_text_down_side_adjust,
	scxt_text_front_side_ext,
	scxt_text_back_side_ext,
	scxt_text_outer_front_padding,
	scxt_text_outer_back_padding,

	//
	//  NamedStylePropsItem
	//
	scxt_cln_style_name,
	scxt_cln_style_props,
	scxt_cln_num_value,

	//
	//  StylePropsCollection
	//
	scxt_cln_basic_style,
	scxt_cln_line_style,
	scxt_cln_text_style,
	scxt_cln_num_param,

	//
	//	Root containers.
	//
	scxt_basic_props,
	scxt_line_props,
	scxt_text_props,
	scxt_props_collection,
};

extern const TXmlDictRecord xtags_StylePropsCollection[];
extern const TXmlDictRecord xtags_ScreenItemStyleRootContainersDirectory[];

class TScreenItemsStyleXmlHelper
{
public:

	static void	DumpBasicStyleProps	(TXmlWriter &doc, WORD tag_id, TBasicStyleProps &data);
	static void	DumpLineStyleProps		(TXmlWriter &doc, WORD tag_id, TLineStyleProps &data);
	static void	DumpTextStyleProps		(TXmlWriter &doc, WORD tag_id, TTextStyleProps &data);
};

class TBasicScreenItemStyleXmlContainerHandler : public TXmlContainerHandler
{
public:

	TBasicScreenItemStyleXmlContainerHandler() { m_pdata = NULL; }

	void			SetDestination(TBasicStyleProps *pdata) { m_pdata = pdata; }

	virtual void	BeginNodeProcessing(WORD tagId) { assert(m_pdata != NULL); m_pdata->Clear(); }

	virtual void	ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName);
	virtual void	ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName);
	virtual void	ProcessColorField(WORD tagId, TColor tagValue, TStringPtr &tagName);

protected:

	TBasicStyleProps		*m_pdata;
};

class TLineScreenItemStyleXmlContainerHandler : public TXmlContainerHandler
{
public:

	TLineScreenItemStyleXmlContainerHandler() { m_pdata = NULL; }

	void			SetDestination(TLineStyleProps *pdata) { m_pdata = pdata; }

	virtual void	BeginNodeProcessing(WORD tagId) { assert(m_pdata != NULL); m_pdata->Clear(); }

	virtual void	ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName);
	virtual void	ProcessColorField(WORD tagId, TColor tagValue, TStringPtr &tagName);

protected:

	TLineStyleProps		*m_pdata;
};

class TTextScreenItemStyleXmlContainerHandler : public TXmlContainerHandler
{
public:

	TTextScreenItemStyleXmlContainerHandler() { m_pdata = NULL; }

	void			SetDestination(TTextStyleProps *pdata) { m_pdata = pdata; }

	virtual void	BeginNodeProcessing(WORD tagId) { assert(m_pdata != NULL); m_pdata->Clear(); m_pdata->background_color = cs_transparent; }

	virtual void	ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName);
	virtual void	ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName);
	virtual void	ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName);
	virtual void	ProcessColorField(WORD tagId, TColor tagValue, TStringPtr &tagName);

protected:

	TTextStyleProps		*m_pdata;
};

class TScreenItemStylePropsCollection : public TXmlContainerHandler
{
public:
			TScreenItemStylePropsCollection() { m_current_style = NULL; }
			~TScreenItemStylePropsCollection() { ClearCollection(); }

	void		ClearCollection() { if (m_current_style != NULL) { delete m_current_style; m_current_style = NULL; } m_data.DeleteAll(); }

	TNamedScreenItemStyleProps	*FindPropsItem(const wchar_t *props_name, short props_type, TGenericConsole *cons = NULL, const wchar_t *cons_error_prefix = NULL);

	bool		PrepareNumParam(__int64 &param_value, const wchar_t *param_name, TGenericConsole *cons = NULL, const wchar_t *cons_error_prefix = NULL);
	bool		PrepareBasicProps(TBasicStyleProps &style_props, const wchar_t *props_name, TGenericConsole *cons = NULL, const wchar_t *cons_error_prefix = NULL);
	bool		PrepareLineProps(TLineStyleProps &style_props, const wchar_t *props_name, TGenericConsole *cons = NULL, const wchar_t *cons_error_prefix = NULL);
	bool		PrepareTextProps(TTextStyleProps &style_props, const wchar_t *props_name, TGenericConsole *cons = NULL, const wchar_t *cons_error_prefix = NULL);

	void		ReportUnusedStyles(TGenericConsole *cons, const wchar_t *cons_error_prefix = NULL);

	void		DumpPropsCollection(TXmlWriter &doc, WORD tag_id);

	bool		LoadFromExeDirectory(const wchar_t *short_fname, TGenericConsole *cons = NULL, TDateTime *latest_scanned_file_date = NULL, bool *new_file_detected_flag = NULL);
				// It is ok to pass the NULL console when error messages are not needed.

protected:

	virtual TXmlContainerHandler	*OpenSubContainer(WORD tagId, TStringPtr &tagName);

	virtual void		BeginNodeProcessing(WORD tagId) { if (m_current_style == NULL) ClearCollection(); }
	virtual void		FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems);

	virtual void		ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName);
	virtual void		ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName);

protected:

	TNamedScreenItemStyleProps					*m_current_style;
	TBasicScreenItemStyleXmlContainerHandler		m_basic_handler;
	TLineScreenItemStyleXmlContainerHandler			m_line_handler;
	TTextScreenItemStyleXmlContainerHandler			m_text_handler;
													// These fields should be used only while reading the XML document.

	TList											m_data;
													// This is list of TNamedScreenItemStyleProps structures. Names of all props
													// in this list should be unique.
};

#endif	// ScreenItems_ScreenItemsXml_H


