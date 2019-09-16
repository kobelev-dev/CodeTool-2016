//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Scanning the command line.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "TextScan/CmdlScan.H"

//---------------------------------------------------------------------
//  =================  TCmdlLexema  ========================
//---------------------------------------------------------------------

static const wchar_t *g_CmdlLexTypeNames1[cmdl_num_lex_types] =
{
	L"Empty lexema",			// cmdl_empty,
	L"Comment",				// cmdl_comment,
	L"Key parameter",			// cmdl_key_param
	L"Positional parameter",		// cmdl_pos_param
	L"EOF",					// cmdl_eof,
	L"Error",					// cmdl_error,
};

static const wchar_t *g_CmdlLexTypeNames2[cmdl_num_lex_types] =
{
	L"empty lexema",			// cmdl_empty,
	L"comment",				// cmdl_comment,
	L"key parameter",			// cmdl_key_param
	L"positional parameter",		// cmdl_pos_param
	L"EOF",					// cmdl_eof,
	L"error",					// cmdl_error,
};

bool TCmdlLexema::IsEqualTo(TCmdlLexema &lex) const
{
	// Compare the types of lexemas.
	if (type != lex.type)
		return(FALSE);

	// Check other props if needed.
	switch (type)
	{
		case cmdl_key_param:
				return(num_value == lex.num_value && str_value == lex.str_value);

		case cmdl_pos_param:
				return(str_value == lex.str_value);

		case cmdl_error:
				return(num_value == lex.num_value);
	}

	// The lexemas are identical.
	return(TRUE);
}

wchar_t *TCmdlLexema::SprintfLexema(wchar_t *buffer, int buff_len)
{
	// The doc states that the length of the buffer should be at least 120 chars. Ensure this.
	assert(buff_len >= 120);

	wchar_t locBuff[100];
	switch (type)
	{
		case cmdl_empty:
				{
					wcscpy(buffer, L"Empty");
				}
				break;

		case cmdl_comment:
				{
					wcscpy(buffer, L"Command line comment");
				}
				break;

		case cmdl_key_param:
				{
					swprintf(buffer, buff_len-1, L"KeyParam, keyId=%d, val=\"%s\"", num_value, str_value.CopyWithTruncationTo(locBuff, 100));
				}
				break;

		case cmdl_pos_param:
				{
					swprintf(buffer, buff_len-1, L"PosParam, prmNum=%d, val=\"%s\"",  num_value, str_value.CopyWithTruncationTo(locBuff, 100));
				}
				break;

		case cmdl_eof:
				{
					wcscpy(buffer, L"==EOF==");
				}
				break;

		case cmdl_error:
				{
					swprintf(buffer, buff_len-1, L"***Error, errCode=%d, errMsg=\"%s\"", num_value, str_value.CopyWithTruncationTo(locBuff, 100));
				}
				break;

		default:
			{
				swprintf(buffer, buff_len-1, L"UnknownCmdlLexType=%d", type);
			}
			break;
	}

	return(buffer);
}

const wchar_t *TCmdlLexema::GetLexTypeName(TCmdlLexType tp, bool want_first_cap)
{
	if (tp < 0 || tp >= cmdl_num_lex_types)
		return(L"Bogus cmdl lex type value");

	return((want_first_cap == TRUE) ? g_CmdlLexTypeNames1[tp] : g_CmdlLexTypeNames2[tp]);
}

//------------------------------------------------------------------------------
//  ====================  TCmdlLexemaInfo  =========================
//------------------------------------------------------------------------------

bool TCmdlLexemaInfo::Init(ID pars_id, TCmdlLexema &lex, TStringsDict *stringsDict)
{
	type = lex.type;
	num_value = lex.num_value;

	bool retValue = TRUE;
	str_value = NULL;
	if (type == cmdl_key_param || type == cmdl_pos_param || type == cmdl_error)
	{
		// Passed lexema contains the string value.
		if (stringsDict != NULL)
		{
			str_value = stringsDict->RegisterStr(lex.str_value.GetBodyPtr(), lex.str_value.GetLength());
			if (str_value == NULL)
				retValue = FALSE;
		}
	}

	key_name_origin.InitFromPrHeader(pars_id, lex.key_name_origin);
	value_origin.InitFromPrHeader(pars_id, lex.value_origin);
	return(retValue);
}

//-----------------------------------------------------------------------------
//  ====================  TCmdlRawScanner  ========================
//-----------------------------------------------------------------------------

TCmdlRawScanner::TCmdlRawScanner(const wchar_t *key_start_characters, TCmdlKeysDictEntry *dict, TFileInfo *file, TStringsDict *strings_dict)
{
	// Verify the error code values from the local error codes enum.
	assert(cscn_err_base == perb_cmdl_scan_base);
	assert(cscn_err_max <= perb_cmdl_scan_max);

	key_start_chars = (wchar_t*)key_start_characters;
	keys_dict = dict;
	Setup(file, strings_dict);
}

bool TCmdlRawScanner::Seek(long new_scan_pos, long len_to_scan)
{
	assert(src != NULL);

	// Check that the new scanning area belongs to the file.
	if (new_scan_pos < 0 || new_scan_pos > src->info.file_len)
		return(FALSE);
	if (len_to_scan < 0)
		len_to_scan = src->info.file_len-new_scan_pos;
	if (new_scan_pos+len_to_scan > src->info.file_len)
		return(FALSE);

	// Accept the parameters.
	buff_length = new_scan_pos+len_to_scan;
	offs = new_scan_pos;
	num_scanned = 0;
	return(TRUE);
}

TCmdlLexType TCmdlRawScanner::GetLex(TCmdlLexema &lex)
{
	// Get rid of the previous lexema contents if any.
	lex.Clear();

	// Scan empty space before any lexema.
	long len_comment = SkipSpaces();
	if (offs >= buff_length)
	{
		// Nothing is present before the end of the file. This is the only one place where the EOF lexema
		// can be returned. Note that source area is present but it has zero length.
		lex.SetValueSrcInfo(src, offs, offs);
		return(lex.SetEndOfFileLex());
	}

	// Some non empty character is still present in the file. In this or that way it will be converted
	// into the non empty lexema.
	num_scanned++;

	if (len_comment > 0)
	{
		// Comment lexema.
		lex.SetValueSrcInfo(src, offs, offs+len_comment);
		offs += len_comment;
		return(lex.SetComment());
	}

	// Check, if this is a key parameter or a positional parameter.
	wchar_t ch = buff_ptr[offs];
	if (key_start_chars == NULL || wcschr(key_start_chars, ch) == NULL)
	{
		// Keys start chars are missing or current char does not belong to this set.
		// This is a positional parameter. No doublt.
		if (ScanValue(lex) == FALSE)
			return(SetRawScanningError(lex, cscn_err_eof_in_string));

		// Str value and its origin are already set.
		return(lex.SetPosParam());
	}

	// This is a key parameter because it starts with one of the "key start" characters.
	long key_lex_beg = offs;

	// Search for the description of this key param.
	TCmdlKeysDictEntry *key_entry = CheckKeysDict();
	if (key_entry == NULL)
	{
		//
		//  The key is not present in the keys dictionary. Props of this key are not known. Consider
		//  the key start character as "key part" of this lexema and all following characters as
		//  "value part" of the lexema.
		//

		lex.SetKeyParam(cmdk_unexpected_key);
		lex.SetKeyNameSrcInfo(src, key_lex_beg, ++offs);

		if (GetCmdlSpaceLen(offs) == 0)
		{
			// Something right after the key start character is present.
			if (ScanValue(lex) == FALSE)
				return(SetRawScanningError(lex, cscn_err_eof_in_string));
		}
	}
	else
	{
		//
		//  The dict entry is available. The key is known and its props are available.
		//

		lex.SetKeyParam(key_entry->key_ident);
		offs += key_entry->key_name_len;
		lex.SetKeyNameSrcInfo(src, key_lex_beg, offs);

		// Prepeare the string ptr for the name of the key.
		TStringPtr key_name_ptr(buff_ptr+key_lex_beg, offs-key_lex_beg);

		// Check what stays right after the key name.
		long spc_len = GetCmdlSpaceLen(offs);

		// Check the props of the key.
		if (key_entry->value_prop == cmkp_none)
		{
			// When the key is defined as having no value, the delimiter prop is not important. The key matching guarantees
			// that after the key there is either a space or an EOF. The string value and the value origin are already empty.
			assert(spc_len != 0);
		}
		else
		{
			// Check what stays after the key name first.
			if (spc_len < 0)
			{
				// There is nothing after the key name. For optional key value the situation is already fine.
				// For the mandatory key value an error should be reported.
				if (key_entry->value_prop == cmkp_req)
				{
					lex.SetValueSrcInfo(src, offs, offs);
					return(SetRawScanningError(lex, cscn_err_keyval_missing, &key_name_ptr));
				}
			}
			else if (spc_len == 0)
			{
				// Value without delimiter is available.
				if (ScanValue(lex) == FALSE)
					return(SetRawScanningError(lex, cscn_err_eof_in_keyval, &key_name_ptr));
			}
			else
			{
				// There is a delimiter after the key name. Check the props of the key.
				if (key_entry->delim_prop == cmkp_none)
				{
					// For the optional key value the situation is already fine. For the mandatory key value an error should be reported.
					if (key_entry->value_prop == cmkp_req)
					{
						lex.SetValueSrcInfo(src, offs, offs);
						return(SetRawScanningError(lex, cscn_err_keyval_missing, &key_name_ptr));
					}
				}
				else
				{
					// Delimiter is either optional or mandatory. Skip this delimiter.
					len_comment = SkipSpaces();
					ch = buff_ptr[offs];
					if (len_comment == 0 && offs < buff_length && (key_start_chars == NULL || wcschr(key_start_chars, ch) == NULL))
					{
						// This is something that is neither a comment nor key.
						if (ScanValue(lex) == FALSE)
							return(SetRawScanningError(lex, cscn_err_eof_in_keyval, &key_name_ptr));
					}
					else
					{
						// For the optional key value the situation is already fine. For the mandatory key value an error should be reported.
						if (key_entry->value_prop == cmkp_req)
						{
							lex.SetValueSrcInfo(src, offs, offs);
							return(SetRawScanningError(lex, cscn_err_keyval_missing, &key_name_ptr));
						}
					}
				}
			}
		}
	}

	// Key param preparation success.
	return(cmdl_key_param);
}

const wchar_t *TCmdlRawScanner::GetErrorText(int err_code)
{
	switch (err_code)
	{
		case cscn_err_eof_in_string:		return(L"End of the command line file inside the quoted parameter.");
		case cscn_err_keyval_missing:		return(L"The required value for the key parameter \"%s\" is missing.");
		case cscn_err_eof_in_keyval:		return(L"End of the command line file inside the quoted value of the key parameter \"%s\".");
		case cscn_err_oom_on_errgen:		return(L"Out of memory while generating an error message.");
	}

	return(L"Unknown CmdlScan error code.");
}

long TCmdlRawScanner::GetCmdlSpaceLen(long start_offs)
{
	if (start_offs >= buff_length)
	{
		return(-1);
	}

	wchar_t ch = buff_ptr[start_offs];
	if (ch == L' ' || ch == L'\t' || ch == L'\f' || ch == L'\n' || ch == L'\r' || ch == 0)
	{
		return(1);
	}

	if (ch == L'\\' && start_offs+1 < buff_length)
	{
		// Check, if this backslash stays before the end of the line or not.
		if (buff_ptr[start_offs+1] == L'\n')
		{
			return(2);
		}

		// Check for other type of the EOL.
		if (buff_ptr[start_offs+1] == L'\r' && start_offs+2 < buff_length && buff_ptr[start_offs+2] == L'\n')
		{
			return(3);
		}
	}

	if (ch == L'/' && start_offs+1 < buff_length && buff_ptr[start_offs+1] == L'/')
	{
		// This is a C++ style comment.
		long loc_offs = start_offs+2;
		while (loc_offs < buff_length)
		{
			if (buff_ptr[loc_offs++] == L'\n')
				break;
		}

		return(loc_offs-start_offs);
	}

	// Some symbol is present and it is not a cmdl space.
	return(0);
}

long TCmdlRawScanner::SkipSpaces()
{
	for(;;)
	{
		long spc_len = GetCmdlSpaceLen(offs);
		if (spc_len <= 0)
		{
			// This is either an end of file or a non space character.
			break;
		}

		if (spc_len >= 2 && buff_ptr[offs] == L'/' && buff_ptr[offs+1] == L'/')
		{
			// Return the length of the comment.
			return(spc_len);
		}

		offs += spc_len;
	}

	return(0);
}

TCmdlKeysDictEntry *TCmdlRawScanner::CheckKeysDict()
{
	if (keys_dict == NULL)
		return(NULL);

	// The dictionary of key params is present. Iterate this dict.
	wchar_t ch = buff_ptr[offs];
	wchar_t *next_ch = buff_ptr+offs+1;
	TCmdlKeysDictEntry *dict = keys_dict;
	while (dict->key_name != NULL)
	{
		// Check the name of the key.
		assert(dict->key_ident != cmdk_unexpected_key);
		assert(dict->key_name_len > 0);

		int len_tail = dict->key_name_len-1;
		if (dict->key_name[0] == ch && (len_tail == 0 || offs+len_tail <= buff_length && wcsncmp(next_ch, dict->key_name+1, len_tail) == 0))
		{
			// The key in the source file matches the name of the current dict entry.
			if (dict->value_prop == cmkp_none || dict->delim_prop == cmkp_req)
			{
				// These key props require that after the key name there should be either EOF or the space character.
				// Check what is going on after the prefix.
				if (GetCmdlSpaceLen(offs+len_tail+1) == 0)
				{
					// There is some non empty character right after the key name. Consider this situation as "no match".
					dict++;
					continue;
				}
			}

			// The key entry is found.
			return(dict);
		}

		// Shift to the next dict entry.
		dict++;
	}

	// There is no match with any dict entry.
	return(NULL);
}

bool TCmdlRawScanner::ScanValue(TCmdlLexema &lex)
{
	// By definiton of this function the current character in the buffer should be present and it should not be a space character.
	assert(GetCmdlSpaceLen(offs) == 0);

	// Current offset is the beginning of the param source area. Allocate offsets for the param value.
	// They will be updates by the code below.
	lex.SetValueSrcInfo(src, offs, offs);
	long beg_inx = offs;				// Index of the beginning of the param value.
	long end_inx = offs;				// Index of the character that stays right after the param value.

	bool paramOk = TRUE;
	if (buff_ptr[offs] == L'\"')
	{
		// This is a quoted parameter. Consume the opening quote.
		beg_inx++;
		offs++;

		// Look for the closing quote.
		paramOk = ScanQuotedString(end_inx);
	}
	else
	{
		// The parameter starts with a regular character, not with a quote. But quotes may be present inside the param.
		// Note that this loop allows more than one quoted string inside the param and it does not require param to
		// end on the quote once the quote is present somewhere inside the param. And this quote will be included into
		// the value.
		while (GetCmdlSpaceLen(offs) == 0)
		{
			if (buff_ptr[offs] == L'\\' && offs+1 < buff_length && (buff_ptr[offs+1] == L'\"' || buff_ptr[offs+1] == L'\'' || buff_ptr[offs+1] == L'\\'))
			{
				// This is an escaped apostrophe, quote or backslash.
				offs += 2;
				continue;
			}

			if (buff_ptr[offs++] == L'\"' && ScanQuotedString(end_inx) == FALSE)
			{
				paramOk = FALSE;
				break;
			}
		}

		end_inx = offs;
	}

	// Setup the data field and finalize the value area.
	lex.str_value.SetData(buff_ptr+beg_inx, end_inx-beg_inx);
	lex.SetValueSrcLen(offs);
	return(paramOk);
}

bool TCmdlRawScanner::ScanQuotedString(long &end_inx)
{
	// An opening quote is already taken out. Look for the closing quote.
	for(;;)
	{
		if (offs >= buff_length)
		{
			// Quoted value is not properly terminated.
			return(FALSE);
		}

		if (buff_ptr[offs] == L'\\' && offs+1 < buff_length && (buff_ptr[offs+1] == L'\"' || buff_ptr[offs+1] == L'\'' || buff_ptr[offs+1] == L'\\'))
		{
			// This is an escaped apostrophe, quote or backslash.
			offs += 2;
			continue;
		}

		if (buff_ptr[offs++] == L'\"')
		{
			end_inx = offs-1;
			break;
		}
	}

	return(TRUE);
}

TCmdlLexType TCmdlRawScanner::SetRawScanningError(TCmdlLexema &lex, int err_code, TStringPtr *extra_ctx_data)
{
	// Ensure that the value origin is not empty.
	assert(lex.value_origin.src_area.file_info != NULL);

	wchar_t *err_msg = (wchar_t*)GetErrorText(err_code);
	assert(err_msg != NULL);

	if (extra_ctx_data != NULL && generated_strings != NULL)
	{
		// This message requires formatting.
		wchar_t err_msg_buff[256], extra_ctx_buff[80];
		swprintf(err_msg_buff, 256, err_msg, extra_ctx_data->CopyWithTruncationTo(extra_ctx_buff, 80));
		wchar_t *err_msg_dict = generated_strings->RegisterStr(err_msg_buff);
		if (err_msg_dict != NULL)
		{
			// Formatting of the error message and string duplication went fine.
			err_msg = err_msg_dict;
		}
		else
		{
			// The Duplication of the string failed. Change the error message.
			err_msg = (wchar_t*)GetErrorText(cscn_err_oom_on_errgen);
			assert(err_msg != NULL);
		}
	}

	lex.SetValueSrcLen(offs);
	return(lex.SetError(err_code, err_msg));
}


