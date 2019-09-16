//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	Common_GenericConsole_H
#define	Common_GenericConsole_H

//
//  The generic console class is definition of the console interface plus an instantiatable
//  implementation that is doing a basic service.
//
class TGenericConsole
{
public:

	TGenericConsole() { m_num_errors = 0; m_abort_signalled = m_hide_abort_flag = FALSE; }

	enum { DefHighlightMode = 0x100, ErrorHighlightMode = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY };
				//
				//  Default highlight mode is not changing the current console attributes if this is applicable to the console implementation.
				//
				//  Other possible attr values (from WinCon.h):
				//
				//		FOREGROUND_BLUE, FOREGROUND_GREEN, FOREGROUND_RED, FOREGROUND_INTENSITY,
				//		BACKGROUND_BLUE, BACKGROUND_GREEN, BACKGROUND_RED, BACKGROUND_INTENSITY.
				//

	enum { CONS_BTN_OK = 0 };
				//
				// This definition is consistent with definitions from the Windows headers.
				//

		//
		//	Virtual methods. Implementation may overwrite only some of them.
		//

	virtual void	HandleTrace(const wchar_t *message, WORD highlight_mode = DefHighlightMode) { HandleLowLevelMessage(message, highlight_mode); }
	virtual int		ShowRunTimeError(const wchar_t *message, int buttons = CONS_BTN_OK, bool inc_errors_count = TRUE);

	virtual void	ResetErrorsCount() { m_num_errors = 0; }
	virtual void	BumpErrorsCount() { m_num_errors++; }

	virtual void	SetAbortFlag(bool new_val) { m_abort_signalled = new_val; }
	virtual void	SetupAbortFlagHiding(bool new_val) { m_hide_abort_flag = new_val; }

	virtual int		GetErrorsCount() const { return(m_num_errors); }
	virtual bool	GetAbortFlag() const { return((m_hide_abort_flag == FALSE) ? m_abort_signalled : FALSE); }

	virtual bool	IsUserResponseSupported() const { return(FALSE); }
					// The derived class should return TRUE if its implementation of the ShowRunTimeError() method
					// can return a non default value.

	virtual const wchar_t	*GetLogFileName() const;
							// This function returns either a static address of a full file name or a NULL pointer
							// when the logging is not active.

	static void	ProcessWindowsMessages();
					// This method processes messages that may be stuck in the Windows message queue.

	static void	PrepareConsolePrefix(wchar_t *buff_40_chars, const wchar_t *console_msg_prefix);
					// When the passed prefix prototype is empty, the result is also empty. Otherwise method copies
					// the prefix prototype into the buffer with possible truncation and adds a space after it.
protected:

	int			m_num_errors;
	bool		m_abort_signalled;
	bool		m_hide_abort_flag;			// Hiding the abort flag makes sense when during the abort processing the code is doing
											// termination actions or some sort of cleanup. In this situation it is good to hide the state
											// of the abort flag for a while.

	static void	(*m_proc_pending_msgs_cbk)(int minProcessingTimeInMilliseconds);
					// This callback is set to a not NULL value when application is using the TBasicDialog class.
					// Basic dialog class maintains static list of all its opened non modal dialogs. This list is needed
					// for proper processing calls to the IsDialogMessage() function.
private:

	static void	HandleLowLevelMessage(const wchar_t *message, WORD highlight_mode);
					// This method sends the passed message to the console window (i.e. calls the wprintf()) and
					// writes it to the debug log file if the debug logging is currently enabled.

	friend class TBasicDialog;
};

//
//  The extended console provides interface for an updateable window.
//
class TGenericConsoleEx : public TGenericConsole
{
public:

	TGenericConsoleEx() { m_posted_param_present = FALSE; }

	//
	//  Virtual methods. Implementation may overwrite only some of them.
	//

	virtual void	OpenConsole() { }
	virtual void	CloseConsole() { }
	virtual bool	IsOpened() const { return(FALSE); }
						// Application can open and close the console window.

	virtual int		OpenModalConsole() { return(FALSE); }
						// This method returns control when the console is already closed. There is no API to close the modal
						// console programmatically. The return value is the parameter of the EndDialog() call that has closed
						// the console.

	virtual void	SetMajorStatus(const wchar_t *msg = NULL) { }
	virtual void	SetMinorStatus(const wchar_t *msg = NULL) { }
						// By default console should have 2 updateable labels.

	virtual void	SetConsoleLabel(int labelId, const wchar_t *msg = NULL) { }
						// The state of the label might not be preserved when the console is closed.

	virtual void	SetControlButtonState(const wchar_t *label, bool enabled_state = TRUE) { }
						// By default console should have one control button.

	virtual bool	PostParamValue(int paramId, DWORD paramValue) { if (m_posted_param_present == TRUE) return(FALSE); m_posted_param_value = paramValue; m_posted_param_present = TRUE; return(TRUE); }
						// The queue of params is just one param long and the paramId is not used in this class.

	virtual bool	GetPostedParamValue(int paramId, DWORD &paramValue, bool removeParamValue) { if (m_posted_param_present == FALSE) return(FALSE); paramValue = m_posted_param_value; if (removeParamValue == TRUE) m_posted_param_present = FALSE; return(TRUE);  }
						// This method returns the posted param if it is available. The paramId is not used in this class.

protected:

	bool		m_posted_param_present;
	DWORD		m_posted_param_value;
};

#endif	// Common_GenericConsole_H


