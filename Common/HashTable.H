//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Classic hash table with fixed number of buckets. Generic dictionary of strings.
//

#ifndef	Common_HashTable_H
#define	Common_HashTable_H

#ifndef   Common_DoubleLinkedList_H
#include  "Common/DoubleLinkedList.H"
#endif

//
//  Hash table item is declared as a template to allow defining the type of parameter
//  of the IsEqualTo() method as derived class, not as the base class.
//
template <class hashTableItem>
class THashTableItem : public TListItem
{
public:

	virtual  DWORD	GetHashValue() = 0;
	virtual  bool		IsEqualTo(const hashTableItem &otherInst) = 0;

	// Utility function for getting the hash value of the null terminated string.
	static unsigned short CalcStrHash(const wchar_t *str)
	{
		unsigned short hashVal = 0;

		while (*str != 0)
		{
			hashVal = hashVal + (unsigned short)(*str++);
			hashVal = hashVal << 1;
			if ((hashVal & 0x8000) != 0)
				hashVal++;
		}

		return(hashVal);
	}

	// Utility function for getting the hash value of the NON null terminated string.
	static unsigned short CalcStrHash(const wchar_t *str, int len)
	{
		unsigned short hashVal = 0;

		while (len-- > 0)
		{
			hashVal = hashVal + (unsigned short)(*str++);
			hashVal = hashVal << 1;
			if ((hashVal & 0x8000)  != 0)
				hashVal++;
		}

		return(hashVal);
	}
};

//
//  Classic hash table with the fixed number of slots. The number of slots in this hash table
//  should be a strict power of 2.
//
template <class hashTableClass, class hashTableItem, int hashTableSlots>
class THashTable
{
public:

	THashTable() { assert((hashTableSlots & (hashTableSlots-1)) == 0); m_iterated_slot_index = hashTableSlots; m_curr_iterated_item = NULL; }

	int		NumItems();
	void		DeleteAll();

	hashTableItem  *FindRecord(hashTableItem *item);
						// Return value is either an identical record of NULL.

	hashTableItem  *AddRecord(hashTableItem *item);
						// When the table already contains an identical record, then the passed record is not inserted
						// and the pointer to an already existing record is returned to the caller.

	void		RemoveRecord(hashTableItem *item);

	void		StartIteration();
	void		StepIteration();
	void		CloseIteration() { m_iterated_slot_index = hashTableSlots; m_curr_iterated_item = NULL; }

	bool		CheckIteration() const { return(m_iterated_slot_index < hashTableSlots); }

	hashTableItem *CurrIteratedItem() const { return(m_curr_iterated_item); }


	void		IterateWithCallback(void *context, void (*callbackFunction)(void *ctx, hashTableItem *item));

	void		ConvertToList(TList &destList);
				// Method kills prev contents of the destination list and moves contents of the hash table into this list.

	void		TakeContentsFrom(THashTable<hashTableClass, hashTableItem, hashTableSlots> &other_inst);

private:

	static hashTableItem *FindItemInTheSlot(TList &slot, hashTableItem *item);

	TList					m_slots[hashTableSlots];

	int					m_iterated_slot_index;
	hashTableItem		*m_curr_iterated_item;
};

// =====================================================
//    ---------------------  THashTable - methods -----------------------
// =====================================================

template <class hashTableClass, class hashTableItem, int hashTableSlots>
int THashTable<hashTableClass, hashTableItem, hashTableSlots>::NumItems()
{
	int numItems = 0;

	TList *slot = m_slots;
	for (int inx = 0; inx < hashTableSlots; ++inx, ++slot)
		numItems += slot->NumItems();

	return(numItems);
}

template <class hashTableClass, class hashTableItem, int hashTableSlots>
void THashTable<hashTableClass, hashTableItem, hashTableSlots>::DeleteAll()
{
	TList *slot = m_slots;
	for (int inx = 0; inx < hashTableSlots; ++inx, ++slot)
		slot->DeleteAll();

	// Clear the iteration related vars.
	CloseIteration();
}

template <class hashTableClass, class hashTableItem, int hashTableSlots>
hashTableItem *THashTable<hashTableClass, hashTableItem, hashTableSlots>::FindRecord(hashTableItem *item)
{
	assert(item != NULL);

	int inx = (int)(item->GetHashValue() & (hashTableSlots-1));
	return(FindItemInTheSlot(m_slots[inx], item));
}

template <class hashTableClass, class hashTableItem, int hashTableSlots>
hashTableItem *THashTable<hashTableClass, hashTableItem, hashTableSlots>::AddRecord(hashTableItem *item)
{
	assert(item != NULL);

	int inx = (int)(item->GetHashValue() & (hashTableSlots-1));
	TList &slotRef = m_slots[inx];

	hashTableItem *otherItem = FindItemInTheSlot(slotRef, item);
	if (otherItem != NULL)
	{
		// Identical record is present. Do not add the passed record.
		// Return pointer to an existing record.
		return(otherItem);
	}

	// There is no identical record in the table.
	slotRef.AppendItem(item);
	return(item);
}

template <class hashTableClass, class hashTableItem, int hashTableSlots>
void THashTable<hashTableClass, hashTableItem, hashTableSlots>::RemoveRecord(hashTableItem *item)
{
	assert(item != NULL);
	assert(item->IsInList() == TRUE);

	int inx = (int)(item->GetHashValue() & (hashTableSlots-1));
	m_slots[inx].RemoveItem(item);
}

template <class hashTableClass, class hashTableItem, int hashTableSlots>
void THashTable<hashTableClass, hashTableItem, hashTableSlots>::StartIteration()
{
	// Reset current iteration if any and make generic iteration step.
	// This will either find some non NULL record or finish the iteration.
	m_iterated_slot_index = 0;
	m_curr_iterated_item = NULL;
	StepIteration();
}

template <class hashTableClass, class hashTableItem, int hashTableSlots>
void THashTable<hashTableClass, hashTableItem, hashTableSlots>::StepIteration()
{
	while (m_iterated_slot_index < hashTableSlots)
	{
		// Iteration is still active.
		TList &currSlot = m_slots[m_iterated_slot_index];
		m_curr_iterated_item = (hashTableItem*)((m_curr_iterated_item == NULL) ? currSlot.GetFirst() : currSlot.GetNext(m_curr_iterated_item));
		if (m_curr_iterated_item != NULL)
		{
			break;
		}

		// Shift to the next slot.
		m_iterated_slot_index++;
	}
}

template <class hashTableClass, class hashTableItem, int hashTableSlots>
void THashTable<hashTableClass, hashTableItem, hashTableSlots>::IterateWithCallback(void *context, void (*callback_function)(void *ctx, hashTableItem *item))
{
	assert(callback_function != NULL);

	TList *slot = m_slots;
	for (int inx = 0; inx < hashTableSlots; ++inx, ++slot)
	{
		for (TListIter<hashTableItem> iter(slot); iter; ++iter)
		{
			// Call the callback.
			callback_function(context, &(iter.CurrItem()));
		}
	}
}

template <class hashTableClass, class hashTableItem, int hashTableSlots>
void THashTable<hashTableClass, hashTableItem, hashTableSlots>::ConvertToList(TList &destList)
{
	destList.DeleteAll();

	TList *slot = m_slots;
	for (int inx = 0; inx < hashTableSlots; ++inx, ++slot)
		destList.AppendList(*slot);
}

template <class hashTableClass, class hashTableItem, int hashTableSlots>
void THashTable<hashTableClass, hashTableItem, hashTableSlots>::TakeContentsFrom(THashTable<hashTableClass, hashTableItem, hashTableSlots> &other_inst)
{
	assert(CheckIteration() == FALSE);
	assert(other_inst.CheckIteration() == FALSE);

	TList *slot = m_slots;
	TList *other_slot = other_inst.m_slots;
	for (int inx = 0; inx < hashTableSlots; ++inx, ++slot, ++other_slot)
		slot->TakeDataFrom(*other_slot);
}

template <class hashTableClass, class hashTableItem, int hashTableSlots>
hashTableItem *THashTable<hashTableClass, hashTableItem, hashTableSlots>::FindItemInTheSlot(TList &slot, hashTableItem *item)
{
	for (TListIter<hashTableItem> iter(slot); iter; ++iter)
	{
		hashTableItem *currItem = &(iter.CurrItem());
		if (currItem->IsEqualTo(*item) == TRUE)
			return(currItem);
	}

	return(NULL);
}

// -----------  Infrastructure  for the text strings cache  ------------------

class TStringsDictItem : public THashTableItem<TStringsDictItem>
{
protected:

	TStringsDictItem(const wchar_t *str) { assert(str != NULL); m_contents = (wchar_t*)str; m_conts_len = (int)wcslen(str); }
	TStringsDictItem(const wchar_t *str, int len) { assert(str != NULL); m_contents = (wchar_t*)str; m_conts_len = len; }
	~TStringsDictItem() { if (m_contents != NULL) free(m_contents); }

public:

	virtual DWORD	GetHashValue() { return(CalcStrHash(m_contents, m_conts_len)); }
	virtual bool		IsEqualTo(const TStringsDictItem &otherInst) { return(m_conts_len == otherInst.m_conts_len && memcmp(m_contents, otherInst.m_contents, m_conts_len*sizeof(wchar_t)) == 0); }
						// Note that memcmp should be used here because string may contain NULL chars in the middle.

	const wchar_t		*Contents() const { return(m_contents); }

protected:

	wchar_t			*m_contents;		// This is a NULL terminated string that is owned by the class in all regular cases.
										// In rare special cases, like temp dummy record for lookup operation, the string
										// can be a non NULL terminated string that is not owned by the class.
	int				m_conts_len;		// Length of the string does not include the NULL character.

	friend class TStringsDict;
};

//
//	There is no way to remove the string from the dictionary.
//
class TStringsDict : public THashTable<TStringsDict, TStringsDictItem, 0x200>
{
public:

	~TStringsDict() { DeleteAll(); }

	void		ResetDict() { DeleteAll(); }

	const wchar_t		*FindString(const wchar_t *str, int strLen = -1);
						// The return value is NULL when the string is unknown.

	wchar_t	*RegisterStr(const wchar_t *str);
	wchar_t	*RegisterStr(const wchar_t *str, int strLen);
				// These methods either return pointer to an already existing instance of the identical string or allocate
				// new string record in the heap and return pointer to the copy. Methods may return NULL if the system
				// is low on memory. App layer should check for this. Second method will ALWAYS add NULL character
				// to the end of the passed string regardless if NULL is already present there or not.
private:

	wchar_t	*AddDictItem(const wchar_t *str, int len);
};

#endif	// Common_HashTable_H


