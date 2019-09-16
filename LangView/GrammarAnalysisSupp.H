//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_GrammarAnalysisSupp_H
#define	LangView_GrammarAnalysisSupp_H

#ifndef   WinUi_DialogBasedConsole_H
#include  "WinUi/DialogBasedConsole.H"
#endif
#ifndef   TextScan_GrammarAnalysis_H
#include  "TextScan/GrammarAnalysis.H"
#endif

struct TLangViewProcessGrammarProps
{
	TGrammarConflictAnalysisTask		m_antask;
	TGrammarConflictAnalysisProps		m_anprops;

	void SetDefaultValues()
	{
		m_antask.SetDefaultValues();
		m_anprops.SetDefaultValues();
	}

	void TakeDataFrom(TLangViewProcessGrammarProps &other_inst)
	{
		m_antask.TakeDataFrom(other_inst.m_antask);
		m_anprops = other_inst.m_anprops;
	}
};

//
//  This is a MODAL dialog. Possible return values are:
//
//		TRUE	--	Process conflicts.
//		FALSE	--	The cancel button was pressed.
//		  -1		--	Dialog failed to open.
//
//  Template: IDD_LANGV_ANPROC_EDIT_ANPROPS
//
class TLangViewEditConflictAnalysisProps : public TBasicDialog
{
public:

		TLangViewEditConflictAnalysisProps(TGrammar &root_grm, TLangViewProcessGrammarProps &props, bool allow_cfct_indexes_sets, bool allow_iter_props_changes);

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	bool			EditAnalysisProps(HWND hParent);
					// This is the major API. It will open the dialog from inside.

	static bool	GetIntValue(TBasicDialog *inst, int rcId, int &value, const wchar_t *prefix_chars_to_ignore = NULL);
					// This method is also used in the grammar analysis console.

protected:

	void			LoadActionSelectionControls();

	void			SetupGrammarTypeSelection(bool bOneGrammar);
	void			SetupConflictTypeSelection(bool set_focus_to_grm_inx_entry_field);
	void			SetupActionTypeSelection(bool bOneAction);

	bool			CheckSaveData();
	bool			AssembleListOfConflictIndexes(TGrammar *curr_grammar, TSymbolsArray &res_array);

protected:

	TGrammar						&m_grammar;
	TLangViewProcessGrammarProps		&m_props;

	bool								m_allow_cfct_indexes_sets;
	bool								m_allow_iter_props_changes;
};

//
//  Template: IDD_LANGV_ANPROC_CONSOLE
//
class TGrammarAnalysisConsole : public TDialogBasedConsole
{
public:

			TGrammarAnalysisConsole();

	virtual LRESULT	ProcessInitDialog(HWND hDefFocusControl);
	virtual  void		ProcessWmCommand(short controlId, short notificationCode, HWND hWndControl);
	virtual  void		ProcessWmTimer(int timerId);
	virtual  void		ProcessWmDestroy();

	void		SetConsoleLabel(int labelId, const wchar_t *msg = NULL);

protected:

	enum { ConsoleTimerIdent = 50 };

	void			EnabDisabManageIterationControls(bool bEnable);
					// The controls are disabled while the partial abort request is being processed.
};

#endif	// LangView_GrammarAnalysisSupp_H


