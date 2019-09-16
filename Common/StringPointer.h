//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	Common_StringPointer_H
#define	Common_StringPointer_H

struct TStrPtrInfo;

//
//  Pointer to a non NULL terminated string. The body of the string is not owned by the object.
//
class TStringPtr
{
public:

	inline TStringPtr(const wchar_t *str = L"", int len = -1) { if (str == NULL) Clear(); else { m_body = (wchar_t*)str; m_len = (len >= 0) ? len : (int)wcslen(str); } }
	inline TStringPtr(const TStringPtr &ptrStr) { assert(ptrStr.m_body != NULL); m_body = ptrStr.m_body; m_len = ptrStr.m_len; }
	inline TStringPtr(const TStrPtrInfo &ptrStrInfo);

	inline bool		IsEmpty() const { return(m_len == 0); }

	inline TStringPtr	&operator = (const wchar_t *str) { if (str == NULL) Clear(); else { m_body = (wchar_t*)str; m_len = (int)wcslen(str); } return(*this); }
	inline TStringPtr	&operator = (const TStringPtr &ptrStr) { assert(ptrStr.m_body != NULL); m_body = ptrStr.m_body; m_len = ptrStr.m_len; return(*this); }

	inline void		Clear() { m_body = L""; m_len = 0; }
	inline void		SetData(const wchar_t *str, int len = -1) { if (str == NULL) Clear(); else { m_body = (wchar_t*)str; m_len = (len >= 0) ? len : (int)wcslen(str); } }

	inline bool		operator == (const wchar_t *str) const { return(m_len == wcslen(str) && wcsncmp(m_body, str, m_len) == 0); }
	inline bool		operator != (const wchar_t *str) const { return(m_len != wcslen(str) || wcsncmp(m_body, str, m_len) != 0); }

	inline bool		operator == (const TStringPtr &ptrStr) const { return(m_len == ptrStr.m_len && wcsncmp(m_body, ptrStr.m_body, m_len) == 0); }
	inline bool		operator != (const TStringPtr &ptrStr) const { return(m_len != ptrStr.m_len || wcsncmp(m_body, ptrStr.m_body, m_len) != 0); }

	inline bool		StartsWith(const wchar_t *str) const { int len = wcslen(str); return(len <= m_len && wcsncmp(m_body, str, len) == 0); }

	static int			Compare(const TStringPtr &ptrStr1, const TStringPtr &ptrStr2);

	inline wchar_t		*GetBodyPtr() const { return(m_body); }
	inline int			GetLength() const { return(m_len); }

	inline wchar_t		GetChar(int inx) const { assert(inx >= 0 && inx < m_len); return(m_body[inx]); }
	inline void		SetChar(int inx, wchar_t val) { assert(inx >= 0 && inx < m_len); m_body[inx] = val; }

	inline void		AdjustToStringEnd() { if (m_len > 0) { m_body += m_len; m_len = 0; } }

	wchar_t	*CopyWithTruncationTo(wchar_t *buff, int buffLen);
	wchar_t	*CopyToVerifiedBuffer(wchar_t *buffer_with_long_enough_length);

protected:

	wchar_t		*m_body;		// This pointer points to the non NULL terminated string. In fact, this will be the typical case.
								// In more rare cases the string may also contain zero characters inside it or at the end.
								// In all cases this memory is not owned by the object.
	int			m_len;			// Length includes the NULL terminators if by occasion they are present in the string.
								// The length is expressed in symbols, not in bytes.
private:

	// Disallow these stuffs.
	void    *operator new(size_t, void* mem) { }
	void    *operator new(size_t) { }
	void     operator delete(void*) { }
};

//
//  This is variant of the class above that is intended to be used in unions that do not allow ctros in their fields.
//
struct TStrPtrInfo
{
	wchar_t		*m_body;		// This pointer points to non NULL terminated string. The memory is not owned by the object.
	int			m_len;			// The length is expressed in symbols, not in bytes.

public:

	inline bool			IsEmpty() const { return(m_len == 0); }

	inline void			Clear() { m_body = L""; m_len = 0; }
	inline void			SetData(const wchar_t *str, int len = -1) { if (str == NULL) Clear(); else { m_body = (wchar_t*)str; m_len = (len >= 0) ? len : (int)wcslen(str); } }

	inline TStrPtrInfo		&operator = (const wchar_t *str) { if (str == NULL) Clear(); else { m_body = (wchar_t*)str; m_len = (int)wcslen(str); } return(*this); }
	inline TStrPtrInfo		&operator = (const TStringPtr &ptrStr) { assert(ptrStr.GetBodyPtr() != NULL); m_body = ptrStr.GetBodyPtr(); m_len = ptrStr.GetLength(); return(*this); }

	inline bool			operator == (const wchar_t *str) const { return(m_len == wcslen(str) && wcsncmp(m_body, str, m_len) == 0); }
	inline bool			operator != (const wchar_t *str) const { return(m_len != wcslen(str) || wcsncmp(m_body, str, m_len) != 0); }

	inline bool			operator == (const TStrPtrInfo &ptrStr) const { return(m_len == ptrStr.m_len && wcsncmp(m_body, ptrStr.m_body, m_len) == 0); }
	inline bool			operator != (const TStrPtrInfo &ptrStr) const { return(m_len != ptrStr.m_len || wcsncmp(m_body, ptrStr.m_body, m_len) != 0); }

	inline bool			StartsWith(const wchar_t *str) const { int len = wcslen(str); return(len <= m_len && wcsncmp(m_body, str, len) == 0); }

	wchar_t		*CopyWithTruncationTo(wchar_t *buff, int buffLen);
	wchar_t		*CopyToVerifiedBuffer(wchar_t *buffer_with_long_enough_length);
};

// This ctor cannot be defined in the body of its class because it uses definition of the TStrPtrInfo.
inline TStringPtr::TStringPtr(const TStrPtrInfo &ptrStrInfo)
{
	if (ptrStrInfo.m_body == NULL)
	{
		m_body = L"";
		m_len = 0;
	}
	else
	{
		m_body = ptrStrInfo.m_body;
		m_len = (ptrStrInfo.m_len >= 0) ? ptrStrInfo.m_len : (int)wcslen(ptrStrInfo.m_body);
	}
}

#endif	// Common_StringPointer_H


