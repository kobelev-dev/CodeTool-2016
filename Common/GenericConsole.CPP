//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/DebugLogger.H"
#include  "Common/GenericConsole.H"

//--------------------------------------------------------------------------
//  ====================  TGenericConsole  =======================
//--------------------------------------------------------------------------

void	(*TGenericConsole::m_proc_pending_msgs_cbk)(int minProcessingTimeInMilliseconds) = NULL;

int TGenericConsole::ShowRunTimeError(const wchar_t *message, int buttons, bool inc_errors_count)
{
	// Send this string to the console and to the debug log file.
	HandleLowLevelMessage(message, ErrorHighlightMode);

	if (inc_errors_count == TRUE)
		m_num_errors++;

	return(CONS_BTN_OK);
}

const wchar_t *TGenericConsole::GetLogFileName() const
{
	// This implementation of console is using the DebugLogger module.
	return(LogGetLogFileName());
}

void TGenericConsole::ProcessWindowsMessages()
{
	if (m_proc_pending_msgs_cbk != NULL)
	{
		// Use the callback that will take care of opened non modal dialogs.
		m_proc_pending_msgs_cbk(0);
		return;
	}

	// This is a simplified version of processing the Windows messages that expects that TBasicDialog
	// and other non modal dialogs are not used in the current process.
	MSG msg;
	while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
}

void TGenericConsole::PrepareConsolePrefix(wchar_t *buff_40_chars, const wchar_t *console_msg_prefix)
{
	if (console_msg_prefix == NULL || console_msg_prefix[0] == 0)
	{
		// The passed prefix prototype is empty.
		buff_40_chars[0] = 0;
	}
	else
	{
		// Duplicate the prefix, maybe with truncation, and append a space to its end.
		wcsncpy(buff_40_chars, console_msg_prefix, 40);
		buff_40_chars[38] = 0;
		wcscat(buff_40_chars, L" ");
	}
}

void TGenericConsole::HandleLowLevelMessage(const wchar_t *message, WORD highlight_mode)
{
	if (message != NULL && message[0] != 0)
	{
		HANDLE hCons = NULL;
		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

		if (highlight_mode != DefHighlightMode)
		{
			// Pick up the current color of the Windows console.
			hCons = ::GetStdHandle(STD_OUTPUT_HANDLE);
			assert(hCons != NULL && hCons != INVALID_HANDLE_VALUE);

			BOOL res1 = ::GetConsoleScreenBufferInfo(hCons, &consoleInfo);
			assert(res1 != 0);

			// Set the modified color.
			::SetConsoleTextAttribute(hCons, (highlight_mode & 0xFF));
		}

		// Send message to the console.
		const wchar_t *ptr_line_beg = message;
		while (ptr_line_beg[0] != 0)
		{
			const wchar_t *ptr = ptr_line_beg;
			while (ptr[0] != 0 && ptr[0] != L'\r' && ptr[0] != L'\n')
				ptr++;

			if (ptr != ptr_line_beg)
				wprintf(L"%.*s\r\n", ptr-ptr_line_beg, ptr_line_beg);

			if (ptr[0] == L'\r' && ptr[1] == L'\n')
				ptr += 2;
			else if (ptr[0] == L'\r' || ptr[0] == L'\n')
				ptr++;

			ptr_line_beg = ptr;
		}

		if (hCons != NULL)
		{
			// Revert the current color back.
			::SetConsoleTextAttribute(hCons, consoleInfo.wAttributes);
		}

		// Write this message to the log file also.
		LogUnicodeMessage(message);
	}
}


