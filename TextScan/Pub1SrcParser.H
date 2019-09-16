//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Callback handler for the small fragment of the C/C++ grammar.
//

#ifndef	TextScan_Pub1SrcParser_H
#define	TextScan_Pub1SrcParser_H

#ifndef   TextScan_GrammarBasedParser_H
#include  "TextScan/GrammarBasedParser.H"
#endif

class TPub1SrcParser : public TGrammarBasedParser
{
public:

	TPub1SrcParser(TGrammar *grammar) : TGrammarBasedParser(grammar) { }

	THighLevelLangType GetParserLangType() const { return(lang_c_like); }
	const wchar_t *GetParserDescription(wchar_t *buff_80_chars) const { return(L"TPub1SrcParser"); }

	bool CheckGrammar() const
	{
		// Grammar should be present and it should contain the fixed name of the callback handler.
		return(m_root_grammar != NULL && wcscmp(m_root_grammar->grm_props.cbk_handler_name, L"TPub1SrcParser") == 0);
	}

	// Contents of the app layer field in the symbol state object.
	enum  NameType
	{
		symt_none,			// This value is stored in m_app_data_int field of terminal symbols and all non terminals
							// that are not the "QualifiedName" and not the "SpecialQualName".
		symt_namesp,		// Subtype of the simple or qualified name.
		symt_type,			// Subtype of the simple or qualified name.
		symt_variab,		// Subtype of the simple or qualified name.
	};

	NameType GetNameType(TSymbolStateItem *item)
	{
		// This method should be called only for lexema items that contain name lexema inside them.
		assert(item->m_symbol < IgnoreLexTypeBase);
		assert(item->m_lex.type == ltx_name);

		if (item->m_lex.str_value.m_body[0] == L'N')
		{
			return(symt_namesp);
		}
		else if (item->m_lex.str_value.m_body[0] == L'T')
		{
			return(symt_type);
		}
		else
		{
			return(symt_variab);
		}
	}

	void					ReviewParsingAction(WORD &action);
	TSymbolStateItem		*AllocateSymbolStateItem(WORD irule = 0xFFFF);

protected:

	TList			m_names_table;
};

#endif	// TextScan_Pub1SrcParser_H


