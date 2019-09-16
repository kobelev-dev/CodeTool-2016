//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Classic double linked list. One more reincarnation of the same thing.
//

#define    STRICT
#include  <stdlib.h>
#include  <assert.h>

#include  "Common/DoubleLinkedList.H"

bool TList::QuickSort(void *ctx, int (__cdecl *compareFunction)(void *ctx, const TListItem **item1, const TListItem **item2))
{
	// Count the number of items in the list.
	int numItems = NumItems();
	if (numItems < 2)
	{
		// Empty or single item lists cannot be sorted.
		return(true);
	}

	// Allocate temporary storage for pointers to all items.
	TListItem **ptrsList = (TListItem**)malloc(numItems*sizeof(TListItem*));
	if (ptrsList == NULL)
		return(false);

	// Convert list into the array of pointers.
	TListItem **ptr = ptrsList;
	for (int i=0; i<numItems; ++i, ++ptr)
	{
		TListItem *ptrFirst = GetFirst();
		assert(ptrFirst != NULL);
		RemoveItem(ptrFirst);
		*ptr = ptrFirst;
	}

	// Sort the array of pointers. This may take a long time.
	qsort_s(ptrsList, numItems, sizeof(TListItem*), (int (__cdecl*)(void*, const void*, const void*))compareFunction, ctx);

	// Assemble the list back.
	ptr = ptrsList;
	for (int i=0; i<numItems; ++i, ++ptr)
		AppendItem(**ptr);

	// Finaly release the temp storage.
	free(ptrsList);
	return(true);
}

bool TList::IsItemInList(TListItem *item) const
{
	assert(item != NULL);
	if (item->IsInList() == false)
		return(false);

	// The passed item belongs to some list. Iterate the list members.
	for (TRawListIterator iter((TList*)this); iter; ++iter)
	{
		if (&iter.CurrBaseItem() == item)
			return(true);
	}

	// The match is not found.
	return(false);
}


