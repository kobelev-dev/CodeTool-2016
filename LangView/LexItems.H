//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_LexItems_H
#define	LangView_LexItems_H

#ifndef   ScreenItems_BasicSymbolItem_H
#include  "ScreenItems/BasicSymbolItem.H"
#endif
#ifndef   ScreenItems_ScreenItemsTree_H
#include  "ScreenItems/ScreenItemsTree.H"
#endif
#ifndef   ScreenItems_ScreenItemsXml_H
#include  "ScreenItems/ScreenItemsXml.H"
#endif
#ifndef   TextScan_MidStore_H
#include  "TextScan/MidStore.H"
#endif
#ifndef   TextScan_GrammarStructure_H
#include  "TextScan/GrammarStructure.H"
#endif
#ifndef   TextScan_GrammarBasedParser_H
#include  "TextScan/GrammarBasedParser.H"
#endif
#ifndef   TextScan_CppDatabase_H
#include  "TextScan/CppDatabase.H"
#endif
#ifndef   LangView_LangView_H
#include  "LangView/LangView.H"
#endif

enum TLexItemType
{
	litp_lexema,						// Lexema as element of the grammar or as the scanned lexema.
	litp_mid_lev_object,				// High level object of the mid lev database or parsing event like #ifdef statement, macro call, etc.
	litp_non_terminal,				// Non terminal of the grammar or non term as intermediare result of the grammar based parsing.
	litp_special_lex_item,			// Various support objects like grammar rule labels, parsing states, etc.
	litp_visual_parser_item,
	litp_cpp_database_item,
};

enum TMidLevObjectLexItemType
{
	mlit_compilation_info,
	mlit_file_info,
	mlit_key_info,
	mlit_parsing_event,
	mlit_num_types,
};

enum TSpecialLexItemType
{
	//
	// Grammar object categories.
	//
	slit_grammar_rules,				// All rules of the single grammar as a category header.
	slit_parsing_states,				// All parsing stattes of the single grammar as a category header.
	slit_xpct_conflicts,				// All expected conflicts as a category header.
	slit_all_grammar_conflicts,		// All grammar conflicts as a category header.
	slit_ambig_grm_conflicts,		// Grammar conflicts that belong to more than one expected conflict.
	slit_assigned_grm_conflicts,		// Grammar conflicts that belong to one expected conflict.
	slit_unexp_grm_conflicts,		// Grammar conflicts that do not belong to any expected conflict.

	//
	// Grammar object instances.
	//
	slit_grammar,					// Grammar object. This object is used for displaying grammar hierarchies.
	slit_rule_label,					// Small grammar rule label. It has only index. Nothing else.
	slit_rule_position,				// Rule position consists of the rule index and index or the symbol in the rule.
	slit_xpct_conflict,				// Expected grammar conflict.
	slit_grammar_conflict,			// Grammar conflict. All subtypes: ambiguous, single xpct, unexpected.
	slit_conflicting_action,			// Conflicting action of the grammar conflict.
	slit_derivation_path,				// Root of the derivation path when it is displayed as AST fragment.

	//
	// Misc objects.
	//
	slit_center_button,				// Button that is used as the root of the tree.
	slit_empty_symbol,				// This object is close to the terminal symbol. It can be only part of an AST.
	slit_missing_link,				// This object shows that required field in the structure is NULL.
	slit_code_location,				// This is "foreign object" for the mid lev events tree.

	slit_num_types,
};

enum TVisualParserLexItemType
{
	vplit_program,					// Root of the fully parsed program. This item can be used only as the root.
	vplit_parser_stk_slot,			// Item shows stk slot inx and pars stt. This item can be only in the root.
	vplit_parser_inp_sym,			// Item shows input symbol inx only. This item can be used only as the root.
	vplit_null_item,					// Item denotes missing children. This is rate type of item for marking OOM cases.
	vplit_num_types,
};

//
//	Part 1. Low level style definitions.
// --------------------------------------------------
//

// All possible LexItem colors.
enum TLexItemBkgrStyleInx
{
	// Lexemas, mid lev objects and non terminals group.
	gti_bkgr_lexema,
	gti_bkgr_lexema_chlds,
	gti_bkgr_mid_lev,
	gti_bkgr_mid_lev_chlds,
	gti_bkgr_non_terminal,
	gti_bkgr_non_term_chlds,
	gti_bkgr_transp_non_term_chlds,
	gti_bkgr_opaq_non_term_chlds,

	gti_bkgr_mlev_aux_call,
	gti_bkgr_mlev_aux_call_chlds,
	gti_bkgr_mlev_file_call,
	gti_bkgr_mlev_file_call_chlds,
	gti_bkgr_mlev_mj_src_call,
	gti_bkgr_mlev_mj_src_call_chlds,
	gti_bkgr_mlev_error_stmt,
	gti_bkgr_mlev_passive_stmt,
	gti_bkgr_mlev_error_event,

	// Special lex items group.
	gti_bkgr_spec_lex_filler,
	gti_bkgr_ambig_conflicts,
	gti_bkgr_grammar_inst,
	gti_bkgr_rule_label,
	gti_bkgr_rule_pos,						// Rule position or the full rule position.
	gti_bkgr_xpct_inst,
	gti_bkgr_conflict_inst,
	gti_bkgr_cfcting_action,
	gti_bkgr_derivation_path,
	gti_bkgr_center_button,
	gti_bkgr_missing_link,
	gti_bkgr_code_location,

	// Visual parser related items.
	gti_bkgr_program,
	gti_bkgr_program_chlds,
	gti_bkgr_parser_obj,						// Parser stack slot or input queue item.
	gti_bkgr_parser_obj_chlds,				// Parser stack slot or input queue item.
	gti_bkgr_null_item,
	gti_bkgr_null_item_chlds,

	// Cpp database related items.
	gti_bkgr_cpp_misc,
	gti_bkgr_cpp_defns,
	gti_bkgr_cpp_unresolved,
	gti_bkgr_cpp_stmts,
	gti_bkgr_cpp_labels,
	gti_bkgr_cpp_exprs,
	gti_bkgr_cpp_oprnds,
	gti_bkgr_cpp_temarg_chlds,
	gti_bkgr_cpp_dim_obj,

	// Misc group.
	gti_bkgr_slct_light_roze,					// This style is used to paint circ/rect selection in symbol use reports.
	gti_bkgr_slct_bold_blue,
	gti_bkgr_slct_foreign_nterm,

	gti_bkgr_num_styles,
};

// All possible LexItem fonts.
enum TLexItemTextStyleInx
{
	// Lexemas and non terminals group.
	gti_text_lexema,
	gti_text_token_operator,
	gti_text_token_keyword,
	gti_text_lexema_symbol,
	gti_text_non_terminal,
	gti_text_non_terminal_symbol,
	gti_text_mlev_small_mlab,

	// Rule action symbols and elements of symbol sequencies as text labels.
	gti_text_act_sym_normal,
	gti_text_act_sym_operator,
	gti_text_act_sym_eof,

	// Grammar conflict input symbols (dim terminal symbols) as text labels.
	gti_text_cinp_sym_normal,
	gti_text_cinp_sym_operator,
	gti_text_cinp_sym_eof,

	// Non terminals as text labels.
	gti_text_non_term_label,

	// Parsing state, rule position and conflicts group.
	gti_text_message,
	gti_text_details,
	gti_text_small_title,
	gti_text_grammar_object,
	gti_text_rule_mlab,
	gti_text_rule_symbol,

	// Visual parser related special items.
	gti_text_program,
	gti_text_parser_obj_index,
	gti_text_null_item,

	// Cpp database related items.
	gti_text_cpp_misc,
	gti_text_cpp_defn,
	gti_text_cpp_dim_obj,
	gti_text_cpp_item_symbol,					// Symbol labels for all types of the cpp items.

	gti_text_num_styles,
};

// All possible types of links between the LexItems.
enum TLexItemLineStyleInx
{
	gti_line_upper_link_default,
	gti_line_upper_link_selected,
	gti_line_right_link_default,
	gti_line_right_link_selected,

	gti_line_num_styles,
};

//
//	Part 2. High level style definitions.
// -----------------------------------------------------------
//

enum { lex_props_any_index = 0xFFFE, lex_props_end_mark = 0xFFFF };

//
//  Props for different types of lex items. Each lex item type has one or more array of styles.
//  Different types use different indexes into these arrays:
//
//		Non keyword lexemas		-	lex_type;
//		Keywords and operators	-	keyword_id;
//		Mid lev objects			-	complex computed index;
//		Non terminals				-	symbol_value;
//
//		Special lex items			-	item_type;
//		Visual parser items			-	item_type;
//		Cpp database items		-	complex computed index;
//
//  These records are used to statically init the style props.
//

struct TLexItemStyleInfoProps
{
	WORD		sym_index;						// Index of the object or one of the special codes:
												// lex_props_any_index, lex_props_end_mark.

	WORD		circ_icon, pcb_icon;				// These are static icon ids that are displayed on every object that is described by the current
												// set of the view props. However, TLangViewLexItem class contains GetAppIconId() method
												// that allows displaying icons based on individual props of the objects.

	BYTE		main_bkgr_style_inx;			// Bkgr style inx. Value 0xFF means that the style in the dest style info stuct should be NULL.
	BYTE		pcb_lower_bkgr_style_inx;		// Bkgr style inx. Value 0xFF means that the style in the dest style info stuct should be NULL.
												// Lower bkgr style can be specified as 0xF8. In this case style from the m_bkgr_styles_dark_lo_rect
												// array is used. These styles are automaticaly derived from all built in bkgr style props.

	BYTE		main_label_style_inx;			// Text style inx.
	BYTE		symbol_label_style_inx;			// Text style inx.

	BYTE		circle_diameter;
	char			main_label_shift_x;
	char			main_label_shift_y;
	char			symbol_label_shift_x;
	char			symbol_label_shift_y;
};

struct TLexItemMainLabelProps
{
	WORD		sym_index;						// Index of the object that record describes or one of
												// the special codes: li_props_any_index, li_props_end_mark.
	wchar_t		*main_label;
};

//
//  TLangViewLexItem instances are painted in different size/fonts/colors depending on their subtype and props.
//  Class TLangViewLexItemStyle is a collection of possible styles for all different TLangViewLexItem subtypes/props.
//
//  TLexItemStyleInfo contains styles for painting one specific subtype/props of TLangViewLexItem.
//
struct TLexItemStyleInfo
{
	TBasicSymbolStyleInfo		basic_props;

	wchar_t					*preferred_main_label;
								// This field is used only for lexemas and non terminals. In many cases this field is NULL.
								// In this case either the lexema value is shown (rather complex procedure) or the name
								// of the non terminal from the grammar.

	void Clear() { memset(this, 0, sizeof(TLexItemStyleInfo)); }
};

enum TLangViewLexItemSelectionType
{
	list_light_roze,
	list_bold_blue,
	list_frin_nterm,			// Dotted frame that tells that non terminal belongs to other section of rules.
	list_num_types,
};

//
//  This class is primarily a set of the TLexItemStyleInfo structures for painting different types of TLangViewLexItem objects.
//
class TLangViewLexItemStyle : public TScreenItemStyle
{
public:

	TLangViewLexItemStyle();
	~TLangViewLexItemStyle() { Reset(); }

	enum { type_ID = 0x01060 };

	short StyleTypeId() const { return(type_ID); }

	enum { MIN_DIM_GRADE = 0, MAX_DIM_GRADE = 3, NUM_DIM_GRADES = MAX_DIM_GRADE+1 };
			// Dim grade 0 is not changing the color.

	void  Reset();
	bool  SetupLexItemsProps(TGrammar *grammar = NULL, int dim_grade = 0, bool show_terminals_grm_names = FALSE, bool highlight_non_terms_transp = FALSE, const wchar_t *short_fname = NULL, TGenericConsole *cons = NULL);
			// When non NULL short profile name is passed, this means that it is necessary to load the style props from this XML profile.

	bool  SetupForDeviceContext(HDC hDC);

	TGrammar		*GetGrammar() const { return(m_grammar); }

	wchar_t			*GetLexemaTypeLabel(TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt, wchar_t *buff_40_chars);
	TTextStyle		*GetLexemaTypeStyle(TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt, bool bright_symbol);
	TTextStyle		*GetNonTermLabelStyle(WORD symbol_value) { return(m_non_term_label_style); }
						// Primary purpose of these methods is getting styles for painting rule action symbols in the grammar view.
						// Althouh these methods can be used everywhere where the grammar symbol should be displayed as single
						// text screen item.

	TLexItemStyleInfo		&GetLexemaStyleInfo(TLexemaType ltx, short keyword_id = 0);
	TLexItemStyleInfo		&GetMidLevObjectStyleInfo(TMidLevObjectLexItemType mlev_item_subt);
	TLexItemStyleInfo		&GetMidLevEventStyleInfo(CtvParsingEventInfo &evt_info);
	TLexItemStyleInfo		&GetNonTerminalStyleInfo(WORD symbol_value);
	TLexItemStyleInfo		&GetSpecialItemStyleInfo(TSpecialLexItemType spec_item_type);
	TLexItemStyleInfo		&GetVisualParserItemStyleInfo(TVisualParserLexItemType vpars_item_type);
	TLexItemStyleInfo		&GetCppDatabaseItemStyleInfo(TCppItemBase *item);
	TLexItemStyleInfo		&GetCppDatabaseItemDimStyleInfo() { return(m_cpp_item_styles[cpp_item_style_dim_object]); }

	TLineStyle		*GetUpperLinkStyle(bool slct_link)		{ return(m_upper_link_styles[(slct_link == TRUE) ? 1 : 0]); }
	TLineStyle		*GetRightLinkStyle(bool slct_link)		{ return(m_right_link_styles[(slct_link == TRUE) ? 1 : 0]); }

	TBitmapStyle		*GetAppIconsStyle()										  { return(&m_app_icons_style); }
	TBasicStyle		*GetBkgrSelectionStyle(TLangViewLexItemSelectionType selt)	  { return(m_bkgr_selection_style[selt]); }

	bool				GetShowLexemasGrammarNameMode() const { return(m_show_terminals_grammar_names); }

	static void	ApplyDimGrade(TColor &color, int dim_grade);
	static void	ApplyDimGrade(TBasicStyleProps &props, int dim_grade);
	static void	ApplyDimGrade(TTextStyleProps &props, int dim_grade);
	static void	ApplyDimGrade(TLineStyleProps &props, int dim_grade);
					// Dim grade MIN_DIM_GRADE is not changing the colors.

	static  wchar_t			*g_DefaultProfileName;
	static  TDateTime		g_LatestScannedProfileDate;

	static  TConcentricModePlacingProps	g_DefaultConcentricPlacingProps;
	static  TRectLinesModePlacingProps	g_DefaultRectLinesPlacingProps;

protected:

	int			ResolveLexemaTypeName(const wchar_t *type_name);
	int			ResolveKeywordName(const wchar_t *type_name);
	int			ResolveMidLevEventTypeName(const wchar_t *type_name);
	int			ResolveNonTerminalName(const wchar_t *type_name);
	int			ResolveSpecItemName(const wchar_t *type_name);
	int			ResolveVisualParserItemName(const wchar_t *type_name);
	int			ResolveCppDatabaseItemName(const wchar_t *type_name);

	bool			ResolveBuiltInBasicStyleName(TBasicStyle *&style, const wchar_t *style_name);
	bool			ResolveBuiltInTextStyleName(TTextStyle *&style, const wchar_t *style_name);

	bool			FindOrAllocateDynamicBasicStyle(TBasicStyle *&style, TNamedScreenItemStyleProps *sp_item, int dim_grade);
	bool			FindOrAllocateDynamicTextStyle(TTextStyle *&style, TNamedScreenItemStyleProps *sp_item, int dim_grade);
	bool			FindOrAllocateDynamicLineStyle(TLineStyle *&style, TNamedScreenItemStyleProps *sp_item, int dim_grade);

	void			PrepareStyleInfosArray(TLexItemType itp, TLexItemStyleInfo *slots, int num_slots, TLexItemStyleInfoProps *props_script);
	void			PreparePreferredLabels(TLexItemStyleInfo *slots, int num_slots, TLexItemMainLabelProps *props_script);
	void			PrepareStyleInfo(TLexItemType itp, TLexItemStyleInfo &slot, TBitmapStyle *app_icons_style, TLexItemStyleInfoProps &props);

protected:

	enum { glxi_label_normal, glxi_label_operator, glxi_label_eof, glxi_num_types, };
				// Text style indexes for displaying lexemas as small text labels.

	enum { mid_lev_object_num_styles = mlit_num_types + pset_num_types + srct_num_types + exmis_num_types + pinc_num_types + lerrc_num_errorclass_types };
				// Number of possible styles for the mid lev lex item objects.

public:

	enum
	{
		cpp_item_style_default,
		cpp_item_style_defn,
		cpp_item_style_unresolved,
		cpp_item_style_stmt,
		cpp_item_style_label,
		cpp_item_style_expr,
		cpp_item_style_operand,
		cpp_item_style_template_arg,			// Tempate type argument and template template argument.
		cpp_item_style_dim_object,
		cpp_item_num_styles,
	};

protected:

	TGrammar				*m_grammar;

	TTextStyle				*m_lexema_label_styles[glxi_num_types];
	TTextStyle				*m_cfct_inp_sym_label_styles[glxi_num_types];
	TTextStyle				*m_non_term_label_style;
								// Styles for lexemas and non terminals as simple text labels.

	TLexItemStyleInfo			m_lexema_type_styles[ltx_num_lexema_types];
	TLexItemStyleInfo			m_keyword_styles[ltkn_num_keyword_vals];
								// These arrays have fixed lengths because every grammar always has a finite number
								// of the terminal symbols that the raw scanner is able to return.

	TLexItemStyleInfo			m_mid_lev_event_styles[mid_lev_object_num_styles];

	TLexItemStyleInfo			m_def_non_terminal_style;			// This style is used when no grammar is set up.
	TLexItemStyleInfo			*m_non_terminal_styles;
	int						m_non_terminal_styles_len;
								// Array of the non terminal styles. The length of this array varies and it is equal to the number
								// of the non terminals in the grammar.

	TLexItemStyleInfo			m_spec_item_styles[slit_num_types];
	TLexItemStyleInfo			m_vpars_item_styles[vplit_num_types];
	TLexItemStyleInfo			m_cpp_item_styles[cpp_item_num_styles];

	TLineStyle				*m_upper_link_styles[2];
	TLineStyle				*m_right_link_styles[2];
	TBasicStyle				*m_bkgr_selection_style[list_num_types];

	bool						m_show_terminals_grammar_names;
								// When avalue of this flag is TRUE, main label of the lexema lex items and lexema text labels
								// should show name of the symbol in the grammar instead of lexema type that this terminal
								// is mapped to.
private:

	TBitmapStyle				m_app_icons_style;

	TBasicStyle				m_bkgr_styles[gti_bkgr_num_styles];
	TBasicStyle				m_bkgr_styles_dark_lo_rect[gti_bkgr_num_styles];
	TTextStyle				m_text_styles[gti_text_num_styles];
	TLineStyle				m_line_styles[gti_line_num_styles];

	TList						m_dynamic_bkgr_styles;					// List of TBasicStyle stuctures.
	TList						m_dynamic_text_styles;					// List of TTextStyle stuctures.
	TList						m_dynamic_line_styles;					// List of TLineStyle stuctures.

	friend class TLangViewNonTerminalLexItem;
					// Non terminal object needs access to the grammar to get the name of the non terminal.

	friend class TLangViewLexItemStyleInfoXmlHandler;
	friend class TLangViewLexItemStylePropsXmlHandler;
					// Xml readers are a real friends of this class.
};

//
//	Part 3. Screen items.
// -----------------------------------------------------------
//

//
// This enum describes the origin of the parsing action that was used to create the lex item. Note that
// the Lexema lex items are always created using the parsing action "shift" and the NonTerminals are
// always created using the parsing action "resolve".
//
enum TLangViewSymbolSpecialOrigin
{
	sporg_none,						// LexItem was created using the action that was taken from the parsing table.
	sporg_cooked,					// App callback has changed the parsing action or fixed the generic syntax error.
	sporg_resolved_conflict,			// The grammar conflict was resolved by the callback handler.
	sporg_nested_parsing,			// The grammar conflict was resolved by the nested parsing without involving the app callback.
	sporg_resolved_nres,			// Nested parsing generated a nested proc result that was resolved by the callback handler.
	sporg_num_types,
};

class TLangViewLexItem : public TBasicSymbolItem
{
public:

	TLangViewLexItem(TLexItemType item_type, TLangViewLexItemStyle *style = NULL);

	virtual void		SetStyle(TScreenItemStyle *stl) { if (stl != NULL) assert(stl->StyleTypeId() == TLangViewLexItemStyle::type_ID); m_style = stl; }

	TLexItemType		GetItemType() const { return(m_item_type); }
	virtual WORD		GetSymbolValue() const { return(0xFFFF); }

	virtual void		PrepareForConcentricMode(HDC hDC, long pos_x, long pos_y);
	virtual void		PrepareForRectLinesMode(HDC hDC, long pos_x, long pos_y, int num_children = 0, bool root_down_placing = FALSE);

	virtual bool		PrepareClickEvent(TLangViewClickEvent &info) { return(FALSE); }
						// Return value FALSE means that the object is not clickable.

	TScreenItem		*CreateSelectionScreenItem(TLangViewLexItemSelectionType selt);

	TLangViewLexItemStyle		*Style() const { return((TLangViewLexItemStyle*)m_style); }
	virtual TLexItemStyleInfo	&GetStyleInfo() = 0;

	virtual short		GetAppIconId(TTreeItemsPlacingMode mode) { return(0); }
						// Lex item may have static application icon that is defined in the style info structure. This method allows
						// overwrtiting this setting and provide special app icon based on the individual props of the object.

	virtual wchar_t	*GetMainLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, TTreeItemsPlacingMode plc_mode) = 0;
	virtual wchar_t	*GetSymbolLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, short *extra_vert_offs, TTreeItemsPlacingMode plc_mode) = 0;

	enum { RCLN_UPPER_LINK_OFFSET = 6, RCLN_CHLD_LINKS_OFFSET = 6, RCLN_CHLD_LINKS_STEP = 4, };

	virtual int			GetConcentricModeParam(HDC hDC, TConcentricPlacingModeParamIndex inx);
	virtual int			GetRectLinesModeParam(HDC hDC, TRectLinesPlacingModeParamIndex inx, TRectLinesModePlacingProps &props, int num_children);
	long				GetMinWidthForPlacingChldLinks(HDC hDC, TLexItemStyleInfo &style_info, int num_children, wchar_t *symbol_label = NULL);

	static wchar_t		*FormatSymbolValue(WORD symbol, wchar_t *buff_40_chars, short *extra_vert_offs = NULL);

	static TLangViewSymbolSpecialOrigin GetSpecOriginFromSpecOriginInfo(WORD final_action, TParsingActionOriginInfo &info);

protected:

	TLexItemType					m_item_type;

	friend class TLangViewTreeNode;
	friend class TLangViewLexemaTreeNode;
	friend class TLangViewNonTerminalTreeNode;
	friend class TLangViewSpecialTreeNode;
};

//
//  --------------- Top Level Screen Items ----------------------
//

class TLangViewLexemaLexItem : public TLangViewLexItem
{
public:

	TLangViewLexemaLexItem(WORD symbol, TLexemaType lex_type, bool check_subt, DWORD lex_subt);
		// Ctor for creating terminal symbols as part of the grammar definition.

	TLangViewLexemaLexItem(WORD symbol, TLexemaInfo &lex_info, WORD dest_state, TParsingActionOriginInfo spec_origin);
		// Ctor for creating terminal symbols during the parsing process.

	TLexemaType		GetLexemaType() const { return(m_lex_info.type); }
	WORD			GetSymbolValue() const { return(m_symbol); }

protected:

	virtual TLexItemStyleInfo	&GetStyleInfo();

	virtual short		GetAppIconId(TTreeItemsPlacingMode mode);

	virtual wchar_t	*GetMainLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, TTreeItemsPlacingMode plc_mode);
	virtual wchar_t	*GetSymbolLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, short *extra_vert_offs, TTreeItemsPlacingMode plc_mode)
																{ return(FormatSymbolValue(m_symbol, buff_40_chars, extra_vert_offs)); }
protected:

	WORD							m_symbol;
	bool								m_grammar_object;

	TLexemaInfo						m_lex_info;
	bool								m_check_subt;				// Grammar object only.

	WORD							m_shift_dest_state;			// Source file parsing only.
	TParsingActionOriginInfo			m_spec_origin;				// This field is relevant only for lexemas that were created during the source file
																// parsing. Grammar based parser can store lexema either in the input queue or
																// in the preview queue. In this case the value of the dw_action_origin field is always
																// 0xFFFFFFFF. When lexema is part of the parser stack, it is always shifted in there.
	friend class TLangViewLexemaTreeNode;
	friend class TLangViewAbstractSyntaxTreeViewer;
};

class TLangViewMidLevObjectLexItem : public TLangViewLexItem
{
public:

	TLangViewMidLevObjectLexItem(CtvCompilationInfo &pars_info);
	TLangViewMidLevObjectLexItem(TFileInfo &file_info);
	TLangViewMidLevObjectLexItem(TKeyInfo &key_info);
	TLangViewMidLevObjectLexItem(CtvParsingEventInfo &event_info);
		// This object cannot appear in the grammar context. It always represents some object of the mid lev database.
		// Data in the passed params is not bypassed. This data should stay valid throughout the whole life of the object.

	virtual bool		PrepareClickEvent(TLangViewClickEvent &info);

protected:

	virtual TLexItemStyleInfo	&GetStyleInfo();

	virtual wchar_t	*GetMainLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, TTreeItemsPlacingMode plc_mode);
	virtual wchar_t	*GetSymbolLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, short *extra_vert_offs, TTreeItemsPlacingMode plc_mode);

protected:

	TMidLevObjectLexItemType		m_mlev_obj_type;

	// This object never owns any data in its fields.
	union
	{
		CtvCompilationInfo			m_pars_info;
		TFileInfo					m_file_info;
		TKeyInfo					m_key_info;
		CtvParsingEventInfo		m_event;
	};

	friend class TLangViewMidLevObjectTreeNode;
};

class TLangViewNonTerminalLexItem : public TLangViewLexItem
{
public:

	TLangViewNonTerminalLexItem(WORD symbol);
		// Ctor for creating non terminals as part of the grammar definition.

	TLangViewNonTerminalLexItem(WORD symbol, WORD rule_index, TSourceLocation &origin, TParsingActionOriginInfo spec_origin);
		// Ctor for creating non terminals during the parsing process.

	WORD			GetSymbolValue() const { return(m_symbol); }

protected:

	virtual TLexItemStyleInfo	&GetStyleInfo() { return(Style()->GetNonTerminalStyleInfo(m_symbol)); }

	virtual short		GetAppIconId(TTreeItemsPlacingMode mode);

	virtual wchar_t	*GetMainLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, TTreeItemsPlacingMode plc_mode);
	virtual wchar_t	*GetSymbolLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, short *extra_vert_offs, TTreeItemsPlacingMode plc_mode)
																{ return(FormatSymbolValue(m_symbol, buff_40_chars, extra_vert_offs)); }
protected:

	WORD							m_symbol;
	bool								m_grammar_object;

	WORD							m_rule_index;				// Source file parsing mode only.
	TSourceLocation					m_origin;					// Source file parsing mode only.
	TParsingActionOriginInfo			m_spec_origin;				// This field is relevant for the source file parsing only. It describes
																// origin of the parsing action that was used to create this lex item.

	friend class TLangViewNonTerminalTreeNode;
	friend class TLangViewAbstractSyntaxTreeViewer;
};

class TLangViewSpecialLexItem : public TLangViewLexItem
{
public:

	TLangViewSpecialLexItem(TSpecialLexItemType type = slit_num_types);
		// Right after creation the object is functional only for simple subtypes. Is is necessary to call SetXyzProps() to make the object meaningful.

	void		SetGrammarCategoryProps(TSpecialLexItemType category_type, WORD igrammar, int num_instances);

	void		SetGrammarProps(WORD igrammar);
	void		SetRuleLabelProps(WORD igrammar, WORD irule);
	void		SetRulePositionProps(WORD igrammar, WORD irule, WORD isym, WORD rule_action_sym = 0xFFFF);
	void		SetXpctConflictProps(WORD igrammar, WORD ixpct);
	void		SetGrammarConflictProps(WORD igrammar, WORD iconflict);
	void		SetConflictingActionProps(WORD igrammar, WORD iconflict, WORD iaction, WORD parsing_action);
	void		SetDerivationPathProps(WORD igrammar, WORD iconflict, WORD iaction, int ipath);

	void		SetSimpleObjectProps(TSpecialLexItemType simple_object_type);
	void		SetEmptySymbolProps(TLexemaInfo &empty_lex);
	void		SetMissingLinkProps(const wchar_t *link_name);
	void		SetCodeLocationProps(TSourceLocation &loc_info, TFileInfo &file_info);

	TSpecialLexItemType	GetSpecType() const { return(m_special_item_type); }

	WORD	GetGrammarIndex() const { assert(m_special_item_type < slit_num_types); return(m_igrammar); }
	WORD	GetRuleIndex() const { assert(m_special_item_type == slit_rule_label || m_special_item_type == slit_rule_position); return(m_irule); }
	WORD	GetRuleSymIndex() const { assert(m_special_item_type == slit_rule_position); return(m_isym); }

	WORD	GetXpctConflictIndex() const { assert(m_special_item_type == slit_xpct_conflict); return(m_ixpct); }
	WORD	GetGrammarConflictIndex() const { assert(m_special_item_type >= slit_grammar_conflict && m_special_item_type <= slit_derivation_path); return(m_iconflict); }
	WORD	GetActionIndex() const { assert(m_special_item_type == slit_conflicting_action || m_special_item_type == slit_derivation_path); return(m_iaction); }
	WORD	GetPathIndex() const { assert(m_special_item_type == slit_derivation_path); return(m_ipath); }

protected:

	virtual TLexItemStyleInfo	&GetStyleInfo() { return(Style()->GetSpecialItemStyleInfo(m_special_item_type)); }

	virtual wchar_t	*GetMainLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, TTreeItemsPlacingMode plc_mode);
	virtual wchar_t	*GetSymbolLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, short *extra_vert_offs, TTreeItemsPlacingMode plc_mode);

	void		ClearAppLevelFields();
	void		CloneAppLevelFields(TLangViewSpecialLexItem &cloned_fields_recipient);

protected:

	TSpecialLexItemType			m_special_item_type;

	WORD						m_igrammar;
	WORD						m_irule;
	WORD						m_isym;

	WORD						m_ixpct;
	WORD						m_iconflict;
	WORD						m_iaction;
	int							m_ipath;

	int							m_num_instances;
	WORD						m_action_value;						// Parsing action of the conflicting action or the rule action symbol.

	TLexemaInfo					m_empty_sym_empty_lex;

	wchar_t						*m_missing_link_name;				// Only static strings are expected in this field.

	TSourceLocation				m_cloc_pos_info;
	TFileInfo						m_cloc_file_info;

	friend class TLangViewSpecialTreeNode;
};

class TLangViewVisualParserLexItem : public TLangViewLexItem
{
public:

	TLangViewVisualParserLexItem() : TLangViewLexItem(litp_visual_parser_item) { m_parser_item_type = vplit_num_types; m_slot_index = 0xFFFF; }

	void		SetProps(TVisualParserLexItemType itype, WORD slot_inx) { m_parser_item_type = itype; m_slot_index = slot_inx; }

protected:

	virtual TLexItemStyleInfo	&GetStyleInfo() { return(Style()->GetVisualParserItemStyleInfo(m_parser_item_type)); }

	virtual wchar_t	*GetMainLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, TTreeItemsPlacingMode plc_mode);
	virtual wchar_t	*GetSymbolLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, short *extra_vert_offs, TTreeItemsPlacingMode plc_mode) { return(NULL); }

protected:

	TVisualParserLexItemType		m_parser_item_type;
	WORD						m_slot_index;			// This field is applicable only to vplit_parser_stk_slot and vplit_parser_inp_sym items.

	friend class TLangViewVisualParserTreeNode;
};

class TLangViewCppItemLexItem : public TLangViewLexItem
{
public:

	TLangViewCppItemLexItem(TCppItemBase *item = NULL) : TLangViewLexItem(litp_cpp_database_item) { m_cpp_item_ptr = item; m_paint_in_dim_style = FALSE; }

	virtual bool		PrepareClickEvent(TLangViewClickEvent &info);

protected:

	virtual TLexItemStyleInfo	&GetStyleInfo() { return((m_paint_in_dim_style == TRUE) ? Style()->GetCppDatabaseItemDimStyleInfo() : Style()->GetCppDatabaseItemStyleInfo(m_cpp_item_ptr)); }

	virtual wchar_t	*GetMainLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, TTreeItemsPlacingMode plc_mode);
	virtual wchar_t	*GetSymbolLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, short *extra_vert_offs, TTreeItemsPlacingMode plc_mode);

protected:

	TCppItemBase				*m_cpp_item_ptr;
	bool							m_paint_in_dim_style;

	friend class TLangViewCppItemTreeNode;
};

//
//	Part 4. Tree nodes.
// -----------------------------------------------------------
//

class TLangViewTreeNode : public TGenericTreeNode
{
public:

	TLangViewTreeNode(TGenericTreeNode *owner = NULL, BYTE poly_inx = 0);

	TLangViewTreeNode	*GetOwner() { return((TLangViewTreeNode*)m_owner); }

	void			SetClickOwner(void *owner, DWORD ext_data = 0) { LangViewScreenItem().SetOwner(owner, ext_data); }
	void			SetLexItemClickOwner(DWORD ext_data = 0) { LangViewScreenItem().SetOwner(&LangViewScreenItem(), ext_data); }
	void			SetOwnerToScreenItem(TScreenItem *item, DWORD ext_data = 0) { item->SetOwner(&LangViewScreenItem(), ext_data); }

	void			ResetScreenItems(TList &screen_items_data_set,  bool reset_links = TRUE);

	virtual TLexItemType	GetItemType() const = 0;
	virtual WORD			GetSymbolValue() { return(LangViewScreenItem().GetSymbolValue()); }

	virtual void			SetSelectedStateEx(bool upper_link_visib, bool upper_link_slct, bool right_link_visib, bool right_link_slct);
	virtual void			SetSelectedStateEx2(int upper_link_visib, int upper_link_slct, int right_link_visib, int right_link_slct);

	virtual void			SetStyle(TLangViewLexItemStyle *stl) = 0;

	virtual TLangViewTreeNode	*Clone(bool clone_children) = 0;
									// Note that cloned item still needs to be prepared for displaying in the same way
									// as it should be prepared after instantiation.

	virtual TScreenItem	*CreateSelectionScreenItem(TLangViewLexItemSelectionType selt) = 0;

	inline DWORD			GetUniqueNodeId() const { return(m_unique_node_id); }
	inline void			SetUniqueNodeId(DWORD id) { m_unique_node_id = id; }
	inline long			GetDiameter() { return(LangViewScreenItem().m_def_circle.GetDiameter()); }

	TScreenItem			*GetPrimaryBkgrItem() { if (LangViewScreenItem().m_def_circle.GetDiameter() > 0) return(&LangViewScreenItem().m_def_circle); else return(&LangViewScreenItem().m_main_rect); }
	TLangViewTreeNode	*GetLowerLeftChild(bool show_all = FALSE) { return((TLangViewTreeNode*)GetFirstChild(show_all)); }

	inline bool			GetUpperLinkSlct() { return(m_upper_link_slct); }
	inline bool			GetRightLinkSlct() { return(m_right_link_slct); }

	TLinesChainItem		*GetUpperLinkItem() { return(&m_tree_link_item); }
	TLinesChainItem		*GetRightLinkItem() { return(&m_special_link); }

protected:

	virtual TLangViewLexItem	&LangViewScreenItem() = 0;

	void			CloneGenericFields(TLangViewTreeNode *cloned_fields_recipient);
	void			CloneChildren(TLangViewTreeNode *cloned_children_recipient);

	void			CheckPrepareConcentricRightLink(TLangViewLexItem &item);
	void			CheckPrepareRectLinesRightLink(TLangViewLexItem &item);
	void			ShowPreparedRightLink(TList &screen_items, TLangViewLexItem &item) { ShowPreparedItem(screen_items, m_special_link); }

protected:

	DWORD				m_unique_node_id;			// This id is assigned by the visual parser with the help of its internal
													// sequence number counter.
	bool					m_missing_chld;				// This field is TRUE if some children were not created in the visual parser
													// or cloned bacause of the memory problems.

	bool					m_upper_link_visib;
	bool					m_upper_link_slct;
	bool					m_right_link_visib;
	bool					m_right_link_slct;

	TLinesChainItem1		m_special_link;

	friend class TLangViewVisualParserConsoleObject;
};

template <class derived_tree_node, class lang_view_screen_item>
class TLangViewTreeNodeTemplate : public TLangViewTreeNode
{
public:

	//
	//  Derived classes should not have data fields. All data fields should be either in TLangViewTreeNode (base class)
	//  or in the data member lex item. This brings some structure and simplifies the overall design.
	//

	// Ctors for lexema lex items.
	TLangViewTreeNodeTemplate(TGenericTreeNode *owner, BYTE poly_inx, WORD symbol, TLexemaType lex_type, bool check_subt, DWORD lex_subt)
						: TLangViewTreeNode(owner, poly_inx), m_lang_view_screen_item(symbol, lex_type, check_subt, lex_subt) { }
	TLangViewTreeNodeTemplate(TGenericTreeNode *owner, BYTE poly_inx, WORD symbol, TLexemaInfo &lex_info, WORD dest_state, TParsingActionOriginInfo spec_origin)
						: TLangViewTreeNode(owner, poly_inx), m_lang_view_screen_item(symbol, lex_info, dest_state, spec_origin) { }

	// Ctors for mid lev objects lex items.
	TLangViewTreeNodeTemplate(TGenericTreeNode *owner, BYTE poly_inx, CtvCompilationInfo &info)
						: TLangViewTreeNode(owner, poly_inx), m_lang_view_screen_item(info) { }
	TLangViewTreeNodeTemplate(TGenericTreeNode *owner, BYTE poly_inx, TFileInfo &info)
						: TLangViewTreeNode(owner, poly_inx), m_lang_view_screen_item(info) { }
	TLangViewTreeNodeTemplate(TGenericTreeNode *owner, BYTE poly_inx, TKeyInfo &info)
						: TLangViewTreeNode(owner, poly_inx), m_lang_view_screen_item(info) { }
	TLangViewTreeNodeTemplate(TGenericTreeNode *owner, BYTE poly_inx, CtvParsingEventInfo &event)
						: TLangViewTreeNode(owner, poly_inx), m_lang_view_screen_item(event) { }

	// Ctors for non terminal lex items.
	TLangViewTreeNodeTemplate(TGenericTreeNode *owner, BYTE poly_inx, WORD symbol)
						: TLangViewTreeNode(owner, poly_inx), m_lang_view_screen_item(symbol) { }
	TLangViewTreeNodeTemplate(TGenericTreeNode *owner, BYTE poly_inx, WORD symbol, WORD rule_index, TSourceLocation &origin, TParsingActionOriginInfo spec_origin)
						: TLangViewTreeNode(owner, poly_inx), m_lang_view_screen_item(symbol, rule_index, origin, spec_origin) { }

	// Ctor for special lex items.
	TLangViewTreeNodeTemplate(TGenericTreeNode *owner, BYTE poly_inx, TSpecialLexItemType spec_type)
						: TLangViewTreeNode(owner, poly_inx), m_lang_view_screen_item(spec_type) { }

	// Ctor for visual parser related lex items.
	TLangViewTreeNodeTemplate(TGenericTreeNode *owner = NULL, BYTE poly_inx = 0)
						: TLangViewTreeNode(owner, poly_inx) { }

	// Ctor for cpp database item based lex items.
	TLangViewTreeNodeTemplate(TGenericTreeNode *owner, BYTE poly_inx, TCppItemBase *item)
						: TLangViewTreeNode(owner, poly_inx), m_lang_view_screen_item(item) { }

	long			GetWidth()	{ return(m_lang_view_screen_item.GetWidth()); }
	long			GetHeight()	{ return(m_lang_view_screen_item.GetHeight()); }

	//
	//  Implementaion of the TGenericTreeNode methods.
	//
	//  Instantiations of this template expect that passed context is TLangViewLexItemStyle*.
	//

	virtual void	PrepareForPlacing(HDC hDC, TTreeItemsPlacingMode mode, bool root_object_flag, void *context);
	virtual int		GetConcentricModePlacingParam(HDC hDC, TConcentricPlacingModeParamIndex inx, void *context);
	virtual int		GetRectLinesModePlacingParam(HDC hDC, TRectLinesPlacingModeParamIndex inx, TRectLinesModePlacingProps &props, void *context);

	virtual void	PrepareConcentricModeScreenItems(HDC hDC, void *context);
	virtual void	PrepareRectLinesModeScreenItems(HDC hDC, TRectLinesModePlacingProps &props, void *context);

	virtual void	ShowLinkScreenItems(TTreeItemsPlacingMode mode, TList &screen_items);
	virtual void	ShowBkgrScreenItems(TTreeItemsPlacingMode mode, TList &screen_items);
	virtual void	ShowFrgrScreenItems(TTreeItemsPlacingMode mode, TList &screen_items);

	//
	//  Implementaion of the TLangViewTreeNode methods.
	//

	virtual TLexItemType	GetItemType() const { return(m_lang_view_screen_item.GetItemType()); }
	virtual void			SetStyle(TLangViewLexItemStyle *stl) { m_lang_view_screen_item.SetStyle(stl); }

	virtual TScreenItem	*CreateSelectionScreenItem(TLangViewLexItemSelectionType selt) { return(m_lang_view_screen_item.CreateSelectionScreenItem(selt)); }

protected:

	virtual TLangViewLexItem	&LangViewScreenItem() { return(m_lang_view_screen_item); }

	lang_view_screen_item		m_lang_view_screen_item;
									//
									// This should be the only one data field in the template.
									// Derived classes should not have their own data fields.
									//
};

template <class derived_tree_node, class lang_view_screen_item>
void TLangViewTreeNodeTemplate<derived_tree_node, lang_view_screen_item>::PrepareForPlacing(HDC hDC, TTreeItemsPlacingMode mode, bool root_object_flag, void *context)
{
	SetStyle((TLangViewLexItemStyle*)context);
}

template <class derived_tree_node, class lang_view_screen_item>
int TLangViewTreeNodeTemplate<derived_tree_node, lang_view_screen_item>::GetConcentricModePlacingParam(HDC hDC, TConcentricPlacingModeParamIndex inx, void *context)
{
	return(m_lang_view_screen_item.GetConcentricModeParam(hDC, inx));
}

template <class derived_tree_node, class lang_view_screen_item>
int TLangViewTreeNodeTemplate<derived_tree_node, lang_view_screen_item>::GetRectLinesModePlacingParam(HDC hDC, TRectLinesPlacingModeParamIndex inx, TRectLinesModePlacingProps &props, void *context)
{
	return(m_lang_view_screen_item.GetRectLinesModeParam(hDC, inx, props, NumDirectChildren(FALSE)));
}

template <class derived_tree_node, class lang_view_screen_item>
void TLangViewTreeNodeTemplate<derived_tree_node, lang_view_screen_item>::PrepareConcentricModeScreenItems(HDC hDC, void *context)
{
	TLineStyle *lineStyle = NULL;
	if (m_upper_link_visib == TRUE)
		lineStyle = m_lang_view_screen_item.Style()->GetUpperLinkStyle(m_upper_link_slct);

	PrepareConcentricModeUpperLinkItem(hDC, lineStyle, context);
	m_lang_view_screen_item.PrepareForConcentricMode(hDC, m_pos_x, m_pos_y);
	CheckPrepareConcentricRightLink(m_lang_view_screen_item);
}

template <class derived_tree_node, class lang_view_screen_item>
void TLangViewTreeNodeTemplate<derived_tree_node, lang_view_screen_item>::PrepareRectLinesModeScreenItems(HDC hDC, TRectLinesModePlacingProps &props, void *context)
{
	TLineStyle *lineStyle = NULL;
	if (m_upper_link_visib == TRUE)
		lineStyle = m_lang_view_screen_item.Style()->GetUpperLinkStyle(m_upper_link_slct);

	PrepareRectLinesModeUpperLinkItem(hDC, lineStyle, props, context);
	m_lang_view_screen_item.PrepareForRectLinesMode(hDC, m_pos_x, m_pos_y, NumDirectChildren(FALSE), props.up_side_down_placing);
	CheckPrepareRectLinesRightLink(m_lang_view_screen_item);
}

template <class derived_tree_node, class lang_view_screen_item>
void TLangViewTreeNodeTemplate<derived_tree_node, lang_view_screen_item>::ShowLinkScreenItems(TTreeItemsPlacingMode mode, TList &screen_items)
{
	// Placing mode is not important for this method.
	ShowPreparedItem(screen_items, m_tree_link_item);
	ShowPreparedRightLink(screen_items, m_lang_view_screen_item);
}

template <class derived_tree_node, class lang_view_screen_item>
void TLangViewTreeNodeTemplate<derived_tree_node, lang_view_screen_item>::ShowBkgrScreenItems(TTreeItemsPlacingMode mode, TList &screen_items)
{
	if (mode == plc_mode_concentric)
	{
		ShowPreparedItem(screen_items, m_lang_view_screen_item.m_def_circle);
	}
	else if (mode == plc_mode_rect_lines)
	{
		ShowPreparedItem(screen_items, m_lang_view_screen_item.m_main_rect);
		ShowPreparedItem(screen_items, m_lang_view_screen_item.m_lower_rect);
	}
}

template <class derived_tree_node, class lang_view_screen_item>
void TLangViewTreeNodeTemplate<derived_tree_node, lang_view_screen_item>::ShowFrgrScreenItems(TTreeItemsPlacingMode mode, TList &screen_items)
{
	// Placing mode is not important for this method.
	ShowPreparedItem(screen_items, m_lang_view_screen_item.m_main_label);
	ShowPreparedItem(screen_items, m_lang_view_screen_item.m_symbol_label);
	ShowPreparedItem(screen_items, m_lang_view_screen_item.m_app_icon);
}

//
//  --------------- Top Level Tree Nodes ----------------------
//

class TLangViewLexemaTreeNode : public TLangViewTreeNodeTemplate<TLangViewLexemaTreeNode, TLangViewLexemaLexItem>
{
public:

	TLangViewLexemaTreeNode(TGenericTreeNode *owner, BYTE poly_inx, WORD symbol, TLexemaType lex_type, bool check_subt, DWORD lex_subt)
						: TLangViewTreeNodeTemplate<TLangViewLexemaTreeNode, TLangViewLexemaLexItem>(owner, poly_inx, symbol, lex_type, check_subt, lex_subt) { }
	TLangViewLexemaTreeNode(TGenericTreeNode *owner, BYTE poly_inx, WORD symbol, TLexemaInfo &lex_info, WORD dest_state, TParsingActionOriginInfo spec_origin)
						: TLangViewTreeNodeTemplate<TLangViewLexemaTreeNode, TLangViewLexemaLexItem>(owner, poly_inx, symbol, lex_info, dest_state, spec_origin) { }

	virtual TLangViewTreeNode	*Clone(bool clone_children);

	WORD			GetShiftDestState() const { return(m_lang_view_screen_item.m_shift_dest_state); }
	DWORD			GetSpecOriginDword() const { return(m_lang_view_screen_item.m_spec_origin.dw_action_origin); }

	void		SetShiftDestState(WORD value) { m_lang_view_screen_item.m_shift_dest_state = value; }
	void		SetSpecOriginDword(DWORD value) { m_lang_view_screen_item.m_spec_origin.dw_action_origin = value; }
};

class TLangViewMidLevObjectTreeNode : public TLangViewTreeNodeTemplate<TLangViewMidLevObjectTreeNode, TLangViewMidLevObjectLexItem>
{
public:

	TLangViewMidLevObjectTreeNode(TGenericTreeNode *owner, BYTE poly_inx, CtvCompilationInfo &info)
								: TLangViewTreeNodeTemplate<TLangViewMidLevObjectTreeNode, TLangViewMidLevObjectLexItem>(owner, poly_inx, info) { }
	TLangViewMidLevObjectTreeNode(TGenericTreeNode *owner, BYTE poly_inx, TFileInfo &info)
								: TLangViewTreeNodeTemplate<TLangViewMidLevObjectTreeNode, TLangViewMidLevObjectLexItem>(owner, poly_inx, info) { }
	TLangViewMidLevObjectTreeNode(TGenericTreeNode *owner, BYTE poly_inx, TKeyInfo &info)
								: TLangViewTreeNodeTemplate<TLangViewMidLevObjectTreeNode, TLangViewMidLevObjectLexItem>(owner, poly_inx, info) { }
	TLangViewMidLevObjectTreeNode(TGenericTreeNode *owner, BYTE poly_inx, CtvParsingEventInfo &event)
								: TLangViewTreeNodeTemplate<TLangViewMidLevObjectTreeNode, TLangViewMidLevObjectLexItem>(owner, poly_inx, event) { }

	virtual TLangViewTreeNode	*Clone(bool clone_children);

	TMidLevObjectLexItemType	ObjectType() const { return(m_lang_view_screen_item.m_mlev_obj_type); }

	const CtvCompilationInfo	&ParsInfo() const { assert(m_lang_view_screen_item.m_mlev_obj_type == mlit_compilation_info); return(m_lang_view_screen_item.m_pars_info); }
	const TFileInfo				&FileInfo() const { assert(m_lang_view_screen_item.m_mlev_obj_type == mlit_file_info); return(m_lang_view_screen_item.m_file_info); }
	const TKeyInfo			&KeyInfo() const { assert(m_lang_view_screen_item.m_mlev_obj_type == mlit_key_info); return(m_lang_view_screen_item.m_key_info); }
	const CtvParsingEventInfo	&Event() const { assert(m_lang_view_screen_item.m_mlev_obj_type == mlit_parsing_event); return(m_lang_view_screen_item.m_event); }
};

class TLangViewNonTerminalTreeNode : public TLangViewTreeNodeTemplate<TLangViewNonTerminalTreeNode, TLangViewNonTerminalLexItem>
{
public:

	TLangViewNonTerminalTreeNode(TGenericTreeNode *owner, BYTE poly_inx, WORD symbol)
						: TLangViewTreeNodeTemplate<TLangViewNonTerminalTreeNode, TLangViewNonTerminalLexItem>(owner, poly_inx, symbol) { }
	TLangViewNonTerminalTreeNode(TGenericTreeNode *owner, BYTE poly_inx, WORD symbol, WORD rule_index, TSourceLocation &origin, TParsingActionOriginInfo spec_origin)
						: TLangViewTreeNodeTemplate<TLangViewNonTerminalTreeNode, TLangViewNonTerminalLexItem>(owner, poly_inx, symbol, rule_index, origin, spec_origin) { }

	virtual TLangViewTreeNode	*Clone(bool clone_children);
};

class TLangViewSpecialTreeNode : public TLangViewTreeNodeTemplate<TLangViewSpecialTreeNode, TLangViewSpecialLexItem>
{
public:

	TLangViewSpecialTreeNode(TGenericTreeNode *owner = NULL, BYTE poly_inx = 0, TSpecialLexItemType spec_type = slit_num_types)
								: TLangViewTreeNodeTemplate<TLangViewSpecialTreeNode, TLangViewSpecialLexItem>(owner, poly_inx, spec_type) { }

	virtual TLangViewTreeNode	*Clone(bool clone_children);

	TLangViewSpecialLexItem	&Item() { return(m_lang_view_screen_item); }
};

class TLangViewVisualParserTreeNode : public TLangViewTreeNodeTemplate<TLangViewVisualParserTreeNode, TLangViewVisualParserLexItem>
{
public:

	TLangViewVisualParserTreeNode(TGenericTreeNode *owner = NULL, BYTE poly_inx = 0)
								: TLangViewTreeNodeTemplate<TLangViewVisualParserTreeNode, TLangViewVisualParserLexItem>(owner, poly_inx) { }

	virtual TLangViewTreeNode	*Clone(bool clone_children);

	TVisualParserLexItemType	GetParserNodeType() const { return(m_lang_view_screen_item.m_parser_item_type); }
	WORD					GetParserSlotIndex() const { return(m_lang_view_screen_item.m_slot_index); }

	void		SetParserNodeType(TVisualParserLexItemType node_type) { m_lang_view_screen_item.m_parser_item_type = node_type; }
	void		SetParserSlotIndex(WORD inx) { m_lang_view_screen_item.m_slot_index = inx; }
};

class TLangViewCppItemTreeNode : public TLangViewTreeNodeTemplate<TLangViewCppItemTreeNode, TLangViewCppItemLexItem>
{
public:

	TLangViewCppItemTreeNode(TGenericTreeNode *owner = NULL, BYTE poly_inx = 0, TCppItemBase *item = NULL)
								: TLangViewTreeNodeTemplate<TLangViewCppItemTreeNode, TLangViewCppItemLexItem>(owner, poly_inx, item) { }

	TCppItemBase	*GetCppItem() { return(m_lang_view_screen_item.m_cpp_item_ptr); }

	void				SetCppItemPtr(TCppItemBase *item) { m_lang_view_screen_item.m_cpp_item_ptr = item; }
	void				SetDimStyleMode(bool new_value) { m_lang_view_screen_item.m_paint_in_dim_style = new_value; }

	TLangViewCppItemTreeNode	*FindNodeByIdHier(ID cpp_item_id);

	virtual TLangViewTreeNode	*Clone(bool clone_children);
};

#endif // LangView_LexItems_H


