//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	ScreenItems_XmlFileView_H
#define	ScreenItems_XmlFileView_H

#ifndef   ScreenItems_DecoratedFileView_H
#include  "ScreenItems/DecoratedFileView.H"
#endif
#ifndef   Common_XmlReader_H
#include  "Xml/XmlReader.H"
#endif

extern const TXmlItemPresentationProps g_XmlView_ShowLeafTagAsContainer;

//
//  Ownership on all returned style pointers still belongs to the style object.
//
class TXmlFileViewStyle : public TScreenItemStyle
{
public:
			TXmlFileViewStyle() { m_xml_stls_prepared = FALSE; m_app_xml_area_styles = NULL; m_app_xml_area_styles_len = 0; }
			~TXmlFileViewStyle() { if (m_app_xml_area_styles != NULL) delete[] m_app_xml_area_styles; }

	enum { type_ID = 0x02010 };

	short	StyleTypeId() const { return(type_ID); }

	bool		SetupDefaultStyles();
				// This method sets up the standard styles. Note that current implementation does not support non ASCII
				// substs in the XML files. There is no way to pass the non ASCII substs info.

	bool		SetupAppTagStyles(const TTextStyleProps *props_array, int props_array_len);
				// This method is setting up an array of the user defined foreground text styles. These styles are used
				// indirectly by passing directory of known tags to the SetupXmlFile() method.

	bool		SetupForDeviceContext(HDC hDC);

	enum   { num_app_styles = 16 };

	TDecoratedFileViewStyle	*GetWholeFileStyle() { return(&m_file_view_style); }
	TTextStyle				*GetStndFrgrAreaStyle(TXmlFileAreaType stnd_style_index);
	TTextStyle				*GetAppFrgrAreaStyle(BYTE zero_based_app_style_index);

	bool		CheckXmlStylePrepared() { return(m_xml_stls_prepared == TRUE); }

protected:

	bool							m_xml_stls_prepared;

	TDecoratedFileViewStyle		m_file_view_style;
	TTextStyle					m_line_nums_style;
	TTextStyle					m_default_text_style;
	TTextStyle					m_line_overflow_style;

	TTextStyle					m_stnd_xml_area_styles[xfat_num_area_types];

	TTextStyle					*m_app_xml_area_styles;
	int							m_app_xml_area_styles_len;
};

class TXmlFileViewScreenItem : public TDecoratedFileViewScreenItem
{
public:

	TXmlFileViewScreenItem();

	enum  { type_ID = 0x051 };

	short	GetItemTypeId() const { return(type_ID); }

	void		SetStyle(TXmlFileViewStyle *new_style);
				// The style should be set up before setting up the source file. If the style changes, it is necessary
				// to resetup the source file again.

	void		ResetSourceFile();

	bool		SetupXmlFile(TScreenItemsViewObject *view, TXmlItemPresentationRecord *special_tags_dict,
							const wchar_t *fdata, long fdata_len = -1, TDataBypassMode fdbp_mode = dbm_duplicate,
							int line_from = 1, int num_lines = 0);

	bool		GetXmlData(TDestinationFile &rprt) const;
	int		GetNumDecorCreationErrors() { return(m_xml_decor_creation_errors); }

protected:

	TXmlFileViewStyle		*XmlViewStyle() const { return((TXmlFileViewStyle*)m_style); }

	void		ApplySyntaxHighlightDecoration(TXmlFileAreaInfo &xml_area_info);

	static void SyntaxHighlightCallbackHandler(XmlSyntaxHighlightCbkContext ctx, TXmlFileAreaInfo &xml_area_info)
	{
		TXmlFileViewScreenItem *inst = (TXmlFileViewScreenItem*)ctx;
		inst->ApplySyntaxHighlightDecoration(xml_area_info);
	}

protected:

	enum TXmlDecorsZorder { xdz_normal = 1, xdz_comment = 2, xdz_error = 3, };

	int				m_xml_decor_creation_errors;
};

#endif	// ScreenItems_XmlFileView_H


