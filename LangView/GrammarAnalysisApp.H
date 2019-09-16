//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_GrammarAnalysisApp_H
#define	LangView_GrammarAnalysisApp_H

#ifndef   WinUi_DialogBasedConsole_H
#include  "WinUi/DialogBasedConsole.H"
#endif
#ifndef   TextScan_GrammarAnalysisDriver_H
#include  "TextScan/GrammarAnalysisDriver.H"
#endif

class TGrammarConflictAnalysisDriverApp;

//
//  This is a MODAL dialog. Possible return values are:
//
//	     TRUE	   --  Process conflicts.
//	    FALSE	   --  The cancel button was pressed.
//		-1	   --  Dialog failed to open.
//
//  Template: IDD_LANGV_ANPROC_STARTUP_DIALOG
//
class TGrammarConflictsAnalysisStartupDialog : public TBasicDialog
{
public:

		TGrammarConflictsAnalysisStartupDialog(TGrammarConflictAnalysisDriverApp *owner);

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	int			ShowDialog(HWND hParent, const wchar_t *proposed_database_directory = NULL);
						// After displaying the dialog the app level shoud pick up the directory name because this dialog will
						// set it up if it was not passed plus it may change it if it was passed. Once the return value is positive,
						// an application can be sure that the analysis directory is existing.

	const wchar_t		*GetDirectoryName() const { return(m_ofn_andir.GetCurrFileName()); }

protected:

	void			UpdateDirectoryStatus();
	bool			CheckGrammar();
	bool			CheckCreateDirectory(TBasicDialog *dlg);

protected:

	TGrammarConflictAnalysisDriverApp			*m_owner;
	TFileOpenHelper							m_ofn_andir;

	bool										m_database_file_present;

	friend class TGrammarConflictsAnalysisEnterDirName;
};

//
//  This is a MODAL dialog. Possible return values are:
//
//         TRUE/FALSE/-1
//
//  Template: IDD_LANGV_ANPROC_ENTER_DIR_NAME
//
class TGrammarConflictsAnalysisEnterDirName : public TBasicDialog
{
public:

		TGrammarConflictsAnalysisEnterDirName(TGrammarConflictsAnalysisStartupDialog *owner);

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

protected:

	TGrammarConflictsAnalysisStartupDialog		*m_owner;

};

//
//  The analysis driver app is not a dialog class. Although it is opening dialogs, that are defined above.
//
class TGrammarConflictAnalysisDriverApp : public TGrammarConflictAnalysisDriver
{
public:

		TGrammarConflictAnalysisDriverApp(HWND hParent, TDialogBasedConsole &console, DWORD tracing_mask, TGrammar *root_grammar)
				: TGrammarConflictAnalysisDriver(console, tracing_mask, root_grammar) { m_hParent = hParent; }

	virtual bool	PrepareAnalysisTaskAndProps(TGrammarConflictAnalysisTask &prm_task, TGrammarConflictAnalysisProps &prm_props, bool prepare_props);

	void			DoConflictsProcessingApp(const wchar_t *proposed_database_directory = NULL);
	void			SaveAnalysisTask(TGrammarConflictAnalysisTask &prm_task);

protected:

	HWND		m_hParent;
};

#endif	// LangView_GrammarAnalysisApp_H


