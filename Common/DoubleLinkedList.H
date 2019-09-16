//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Classic double linked list. This is one more reincarnation of the same thing.
//
//   Note that destuctor of the list item is virtual and empty. The list itself has
//   no destructor. The lists should be explictly destroyed by the application.
//

#ifndef	Common_DoubleLinkedList_H
#define	Common_DoubleLinkedList_H

class TListItem
{
public:
				TListItem() { m_next = m_prev = this; }
	virtual		~TListItem() { }

	void		Clear() { m_next = m_prev = this; }
	bool		IsInList() const { return(m_next != this || m_prev != this); }

	bool		CheckItemLinks() const { return(m_next->m_prev == this && m_prev->m_next == this); }
				// When the list item is not corrupted, this method returns TRUE both when the item is not inserted
				// into any list and when it is part of some list.
protected:

	void		SetNextItem(TListItem &item)	{ item.m_next = m_next; item.m_prev = this; m_next->m_prev = &item; m_next = &item; }
	void		SetPrevItem(TListItem &item)	{ item.m_prev = m_prev; item.m_next = this; m_prev->m_next = &item; m_prev = &item; }
	void		RemoveFromTheList()		{ m_prev->m_next = m_next; m_next->m_prev = m_prev; m_next = m_prev = this; }

	TListItem		*m_next;
	TListItem		*m_prev;

	friend class TList;
	friend class TRawListIterator;
};

class TList : protected TListItem
{
public:

	TList() { }
	TList(TListItem *ptr_first, TListItem *ptr_last) { m_next = ptr_first; ptr_first->m_prev = this; m_prev = ptr_last; ptr_last->m_next = this; }

	bool		IsEmpty() const		{ return(m_next == (const TListItem*)this); }
	int		NumItems() const	{ int num = 0; TListItem *item = GetFirst(); while (item != NULL) { item = GetNext(item); num++; } return(num); }

	void		Clear()				{ TListItem::Clear(); }
	void		DeleteAll()			{ while (m_next != this) { TListItem *item = m_next; item->RemoveFromTheList(); delete item; } }

	void		PrependItem(TListItem &item)		{ SetNextItem(item); }
	void		AppendItem(TListItem &item)		{ SetPrevItem(item); }

	void		PrependItem(TListItem *item)		{ SetNextItem(*item); }
	void		AppendItem(TListItem *item)		{ SetPrevItem(*item); }

	void		InsertBefore(TListItem &item, TListItem &insBefore) { insBefore.SetPrevItem(item); }
	void		InsertAfter(TListItem &item, TListItem &insAfter) { insAfter.SetNextItem(item); }
	void		RemoveItem(TListItem &item)		{ item.RemoveFromTheList(); }

	void		InsertBefore(TListItem *item, TListItem *insBefore) { insBefore->SetPrevItem(*item); }
	void		InsertAfter(TListItem *item, TListItem *insAfter) { insAfter->SetNextItem(*item); }
	void		RemoveItem(TListItem *item)		{ item->RemoveFromTheList(); }

	void		PrependList(TList &lst)				{ if (lst.IsEmpty() == false) { m_next->m_prev = lst.m_prev; lst.m_prev->m_next = m_next; lst.m_next->m_prev = this; m_next = lst.m_next; lst.Clear(); } }
	void		AppendList(TList &lst)				{ if (lst.IsEmpty() == false) { m_prev->m_next = lst.m_next; lst.m_next->m_prev = m_prev; lst.m_prev->m_next = this; m_prev = lst.m_prev; lst.Clear(); } }
	void		TakeDataFrom(TList &lst)			{ DeleteAll(); if (lst.IsEmpty() == false) { m_next = lst.m_next; m_prev = lst.m_prev; m_next->m_prev = this; m_prev->m_next = this; lst.Clear(); } }

	TListItem		*GetFirst() const				{ return((m_next == (const TListItem*)this) ? NULL : m_next); }
	TListItem		*GetLast() const				{ return((m_prev == (const TListItem*)this) ? NULL : m_prev); }
	TListItem		*GetNext(TListItem *item) const	{ return((m_prev == item) ? NULL : item->m_next); }
	TListItem		*GetPrev(TListItem *item) const	{ return((m_next == item) ? NULL : item->m_prev); }

	bool		QuickSort(void *ctx, int (__cdecl *compareFunction)(void *ctx, const TListItem **item1, const TListItem **item2));
				// Method allocates temp storage for storing pointers to the list items. Actual sorting is done using the qsort.
				// Return value shows if it was possible to allocate the storage and sort the list or not. Note that in the case
				// of failure the list is not destroyed. It will simply remain unsorted.

	bool		IsItemInList(TListItem *item) const;
				// This method iterates the whole list to find out if the passed item is present in the list or not.
				// When the list is big, this may take considerable time.
private:

	// Disallow the binary copying.
	TList(TList&) { }					// TList object1(object2);
	void operator = (TList&) { }		// object1 = object2;

	friend class TRawListIterator;
};

class TRawListIterator
{
public:

	inline	TRawListIterator(TList &lst) : m_list(lst) { m_item = lst.m_next; }
	inline	TRawListIterator(TList *plst) : m_list(*plst) { m_item = plst->m_next; }
	inline	TRawListIterator(TList &lst, TListItem *item) : m_list(lst) { m_item = item; }
	inline	TRawListIterator(TList *plst, TListItem *item) : m_list(*plst) { m_item = item; }

	inline	operator	bool() const					{ return(m_item != &m_list); }
	inline	bool			IsComplete() const			{ return(m_item == &m_list); }

	inline	TListItem		&CurrBaseItem() const		{ return(*m_item); }

	inline	void		operator ++ ()					{ m_item = m_item->m_next; }
	inline	void		operator -- ()					{ m_item = m_item->m_prev; }
	inline	void		ShiftDown()					{ m_item = m_item->m_next; }
	inline	void		ShiftUp()						{ m_item = m_item->m_prev; }

	inline	void		SeekToFirst()					{ m_item = m_list.m_next; }
	inline	void		SeekToLast()					{ m_item = m_list.m_prev; }
	inline	void		SeekToItem(TListItem *item)		{ if (item != NULL) m_item = item; else m_item = &m_list; }

protected:

	TList			&m_list;
	TListItem		*m_item;
};

//
// This template expects that "derived_class" is derived from the TListItem. Template cannot check and/or enforce this.
//
template <class derivedClass>
class TListIter : public TRawListIterator
{
public:

	inline	TListIter(TList &lst) : TRawListIterator(lst) { }
	inline	TListIter(TList *plst) : TRawListIterator(plst) { }
	inline	TListIter(TList &lst, derivedClass *inst) : TRawListIterator(lst, inst) { }
	inline	TListIter(TList *plst, derivedClass *inst) : TRawListIterator(plst, inst) { }

	inline derivedClass	&CurrItem() const { return(derivedClass&)(*m_item); }
	inline derivedClass	*CurrItemPtr() const { return(derivedClass*)m_item; }
};

#endif	// Common_DoubleLinkedList_H


