//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Classic hash table with fixed number of buckets. Generic dictionary of strings.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/HashTable.H"

//---------------------------------------------------------------------
//  ===================  TStringsDict  =======================
//---------------------------------------------------------------------

const wchar_t *TStringsDict::FindString(const wchar_t *str, int strLen)
{
	if (strLen < 0)
		strLen = wcslen(str);

	// Check, maybe this string is already known.
	TStringsDictItem lookupItem(str, strLen);
	TStringsDictItem *res = FindRecord(&lookupItem);
	lookupItem.m_contents = NULL;
	return((res != NULL) ? res->m_contents : NULL);
}

wchar_t *TStringsDict::RegisterStr(const wchar_t *str)
{
	// Check, maybe this string is already known.
	TStringsDictItem lookupItem(str);
	TStringsDictItem *res = FindRecord(&lookupItem);
	lookupItem.m_contents = NULL;

	if (res != NULL)
		return(res->m_contents);

	// The string is new.
	return(AddDictItem(str, lookupItem.m_conts_len));
}

wchar_t *TStringsDict::RegisterStr(const wchar_t *str, int strLen)
{
	// Check, maybe this string is already known.
	TStringsDictItem lookupItem(str, strLen);
	TStringsDictItem *res = FindRecord(&lookupItem);
	lookupItem.m_contents = NULL;

	if (res != NULL)
		return(res->m_contents);

	// The string is new.
	return(AddDictItem(str, strLen));
}

wchar_t *TStringsDict::AddDictItem(const wchar_t *str, int len)
{
	// Allocate memory for the copy of the passed string.
	wchar_t *strDup = (wchar_t*)malloc((len+1)*sizeof(wchar_t));
	if (strDup == NULL)
		return(NULL);

	// Copy the data in. Use the memcpy function because string may contain NULL chars in the middle.
	memcpy(strDup, str, len*sizeof(wchar_t));
	strDup[len] = 0;

	// Allocate the item itself.
	TStringsDictItem *dictItem = new TStringsDictItem(strDup, len);
	if (dictItem == NULL)
	{
		free(strDup);
		return(NULL);
	}

	// Add new item to the table.
	TStringsDictItem *addedItem = AddRecord(dictItem);
	assert(addedItem == dictItem);

	// Full success.
	return(strDup);
}


