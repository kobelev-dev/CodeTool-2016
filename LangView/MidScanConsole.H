//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_MidScanConsole_H
#define	LangView_MidScanConsole_H

#ifndef   WinUi_DialogBasedConsole_H
#include  "WinUi/DialogBasedConsole.H"
#endif

typedef  TSavedConsoleControlState<1024>  TSavedConsoleControlState1024;

//
//  Template:	IDD_LANGV_CONS_SINGLE_FILE_SCAN,
//			IDD_LANGV_CONS_BATCH_PROC_SCAN
//
class TMidLevScannerConsole : public TDialogBasedConsole
{
public:

			TMidLevScannerConsole(HINSTANCE hInst = NULL, bool batch_mode = FALSE);

	void		SetBatchMode(bool value);

	virtual LRESULT	ProcessInitDialog(HWND hDefFocusControl);
	virtual void		ProcessWmCommand(short controlId, short notificationCode, HWND hWndControl);

	virtual void		ResetErrorsCount() { int old_num_errors = GetErrorsCount(); TDialogBasedConsole::ResetErrorsCount(); UpdateErrorsCountLabel(old_num_errors); }
	virtual void		BumpErrorsCount() { int old_num_errors = GetErrorsCount(); TDialogBasedConsole::BumpErrorsCount(); UpdateErrorsCountLabel(old_num_errors); }

	virtual void		SetConsoleLabel(int labelId, const wchar_t *msg = NULL);

protected:

	void		UpdateErrorsCountLabel(int old_num_errors);

	static short GetTemplateId(bool batch_mode);
				// The implementation of this method is not inline to avoid bringing identifiers of resorces into this header.

protected:

	bool									m_curr_batch_mode;

	TSavedConsoleControlState1024			m_major_file_name_stt;
	TSavedConsoleControlState40			m_mid_lev_lexemas_stt;
	TSavedConsoleControlState40			m_mid_lev_events_stt;
	TSavedConsoleControlState40			m_num_errors_stt;
	TSavedConsoleControlState40			m_num_warns_stt;
	TSavedConsoleControlState80			m_elapsed_time_stt;

	TSavedConsoleControlState40			m_run_time_errs_stt;

	TSavedConsoleControlState40			m_batch_files_processed_stt;
	TSavedConsoleControlState40			m_batch_dirs_processed_stt;
	TSavedConsoleControlState40			m_batch_num_errors_stt;
	TSavedConsoleControlState40			m_batch_num_warns_stt;
	TSavedConsoleControlState80			m_batch_elapsed_time_stt;
};

//
//  This is MODAL console. It is not expected to be directly opened/closed by the app layer.
//
//  This console is opened automatically from the DoSession() method. It is closed from of processing
//  the WM_TIMER message in the opened modal dialog.
//
class TLangViewBasicAsyncConsole : public TDialogBasedConsole
{
public:

			TLangViewBasicAsyncConsole(const wchar_t *console_title);

	virtual LRESULT		ProcessInitDialog(HWND hDefFocusCtrl);
	virtual void			ProcessWmCommand(short controlId, short notificationCode, HWND hWndControl);
	virtual void			ProcessWmTimer(int timer_id);

	virtual void			OpenConsole() { assert(FALSE); }
	virtual void			CloseConsole() { assert(FALSE); }
	virtual int				OpenModalConsole() { assert(FALSE); return(-1); }

	enum SessionResults
	{
		sres_not_started		= -3,			// Session was not started.
		sres_console_error		= -2,			// Failure opening the console dialog.
		sres_thread_error		= -1,			// Error starting the worker thread.
		sres_app_result			=  0,			// Retun value from the worker thread.
												// Currently the worker thread all the time returns 0.
		sres_still_active = STILL_ACTIVE,			// Session is still running (0x103).
	};

	//
	//  Major API of the class.
	//
	SessionResults	DoSession(TBasicDialog *proto_and_owner);
						// Once started, the session cannot be aborted programmaticaly. The result of the session is stored
						// in the m_dwThreadExitCode field and it is also returned as the return value of this method.
protected:

	virtual void		SessionWorkerProcedure() = 0;
						// This method is called on the worker thread. This class has no idea of what this method will do.
						// Derived class should provide a meaningful implementation.

	enum { ConsoleTimerIdent = 50 };

	class TWorkerThreadConsoleEx : public TGenericConsoleEx
	{
	public:		//
				//	This console is passed to the parsing environment that works on the worker thread.
				//

		TWorkerThreadConsoleEx() { ::InitializeCriticalSection(&m_crits); m_major_msg_buff[0] = 0; m_minor_msg_buff[0] = 0; }
		~TWorkerThreadConsoleEx() { ::DeleteCriticalSection(&m_crits); }

		virtual void	SetMajorStatus(const wchar_t *msg = NULL) { ::EnterCriticalSection(&m_crits); wcsncpy(m_major_msg_buff, msg, 120); m_major_msg_buff[119] = 0; ::LeaveCriticalSection(&m_crits); }
		virtual void	SetMinorStatus(const wchar_t *msg = NULL) { ::EnterCriticalSection(&m_crits); wcsncpy(m_minor_msg_buff, msg, 120); m_minor_msg_buff[119] = 0; ::LeaveCriticalSection(&m_crits); }
						// These methods are expected to be called from the worker thread only.

		CRITICAL_SECTION		m_crits;
		wchar_t					m_major_msg_buff[120];
		wchar_t					m_minor_msg_buff[120];
	};

	static DWORD WINAPI WorkerThreadProc(void *context);

protected:

	TWorkerThreadConsoleEx			m_worker_thread_console;

	HANDLE							m_hThread;							// When hThread is not NULL, this means that parsing/conversion process is active.
	DWORD							m_dwThreadId;						// This data field is added for debugging purposes only.
	DWORD							m_dwThreadExitCode;				// Possible values of this field are defined in SessionResults enum.
};

#endif	// LangView_MidScanConsole_H


