//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Helper classes for handling variable sized arrays.
//

#ifndef	Common_Buffers_H
#define	Common_Buffers_H

// =====================================================================
//
//   This template expects that the element of the buffer is a plain old data structure (POD). This means
//   that item can be copied from one place to another without destroyting it and that no cleaup is need.
//
//	The item record does not need to have any specific methods.
//
// =====================================================================

template <class item_struct, int initial_num_items, int items_realloc_extra>
class TStructsArray
{
public:

	TStructsArray();
			// Writeable ctor. After creating object with this ctor the array is empty.

	TStructsArray(item_struct *data, int len_data);
			// This is a ReadOnly ctor. Passed array becomes the contents of the array while it is still not owned by the array.

	~TStructsArray() { ReleaseObject(); }
			// It is important that destructor is calling only the ReleaseObject() method.
			// Destructor and ReleaseObject() can be used instead one another.

	void SetupReadWriteMode();
			// This method resets prev contents of the array if any and sets the read/write mode.

	void SetupReadOnlyData(item_struct *data, int len_data);
			// This method resets prev contents of the array if any, changes the access mode
			// to read only and sets the data.

	void TakeContentsFrom(TStructsArray<item_struct, initial_num_items, items_realloc_extra> &other_inst);
			//
			// Result of the call depends on the mode of the 'other_inst' parameter:
			//
			//	ReadOnly   -	data from 'other_inst' is shared with 'other_inst'
			//				in readOnly mode.
			//	ReadWrite  -	data is taken from 'other_inst' and the 'other_inst' param
			//				becomes empty.
			//

	inline  void	Clear() { SetupReadWriteMode(); }
					// Method switches instance into the read/write mode and releases the extended buffer if it is present.

	inline  void	FixObjectMove()		{ if (read_only == TRUE) return; if (num_alloc <= initial_num_items) extended_data = (item_struct*)initial_data; }
	inline  void	ReleaseObject()		{ if (read_only == TRUE) return; num_items = 0; num_alloc = initial_num_items; if (extended_data != (item_struct*)initial_data) { free(extended_data); extended_data = (item_struct*)initial_data; } }
	inline  void	WipeObject()			{ if (read_only == TRUE) return; num_items = 0; num_alloc = initial_num_items; extended_data = (item_struct*)initial_data; }

	inline  bool	IsReadOnly() const		{ return(read_only); }
	inline  bool	HasFreeSpace() const	{ return(num_items != num_alloc); }

	inline  int		NumItems() const		{ return(num_items); }
	inline  int		NumAllocedItems()	{ return(num_alloc); }

	inline  int		NumInitialItems() const			{ return(initial_num_items); }
	inline  int		NumItemsReallocExtra() const	{ return(items_realloc_extra); }

	inline  item_struct	&ItemRef(int inx) const		{ return(extended_data[inx]); }
	inline  item_struct	*ItemPtr(int inx) const			{ return(extended_data+inx); }
	inline  item_struct	&operator[] (int inx) const		{ return(extended_data[inx]); }
	inline  item_struct	*DataPtr() const				{ return(extended_data); }

	inline  void	SetNumItems(int new_num)			{ if (read_only == TRUE) return; assert(new_num <= num_alloc); num_items = new_num; }
	inline  void	IncNumItems(int delta = 1)		{ if (read_only == TRUE) return; int new_num_items = num_items+delta; assert(new_num_items <= num_alloc); num_items = new_num_items; }
					// Note: Caller of these methods takes the full responsibilty on setting/removing data to/from the curr array buffer.

	inline  bool	ReserveSpace(int num_extra_items = 1) { return(ReserveTotalBufferSpace(num_items+num_extra_items)); }
	bool			ReserveTotalBufferSpace(int total_num_items);

	bool  AppendItem(item_struct &item);
	bool  AppendItems(item_struct *items_array, int array_len);

	bool  InsertItem(int insert_before_inx, item_struct &item);
	bool  InsertItems(int insert_before_inx, const item_struct *items_array, int array_len);
			// Both methods allow passing the "insert_before_inx" param equal to the current length of the array.
			// In this case one item or sevaral items will be appended to the end of the array

	void  ReleaseItem(int inx);
	void  ReleaseItems(int inx, int num_items_to_release);

	void  ReleaseLastItem() { if (read_only == TRUE) return; if (num_items > 0) ReleaseItem(num_items-1); }

	bool  GetXpndError() const { return(xpnd_error); }
	bool  ResetXpndError() { xpnd_error = FALSE; }

	void  QuickSort(int (__cdecl *compareFunction)(const item_struct *item1, const item_struct *item2));
	void  QuickSortWithCtx(void *ctx, int (__cdecl *compareFunction)(void *ctx, const item_struct *item1, const item_struct *item2));

protected:

	int				num_items;
	int				num_alloc;

	bool			read_only;
	bool			xpnd_error;				// This flag has limited use. It is used only in the TTextBuffer template
											// where it indicates the buffering failure. This flag is defined in the core
											// template because other flag is already present.

	item_struct		*extended_data;
	BYTE			initial_data[sizeof(item_struct)*initial_num_items];
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Writeable ctor.
template <class item_struct, int initial_num_items, int items_realloc_extra>
TStructsArray<item_struct, initial_num_items, items_realloc_extra>::TStructsArray()
{
	num_items = 0;
	num_alloc = initial_num_items;
	read_only = xpnd_error = FALSE;
	extended_data = (item_struct*)initial_data;
}

// ReadOnly ctor.
template <class item_struct, int initial_num_items, int items_realloc_extra>
TStructsArray<item_struct, initial_num_items, items_realloc_extra>::TStructsArray(item_struct *data, int len_data)
{
	num_items = len_data;
	num_alloc = len_data;
	read_only = TRUE;
	xpnd_error = FALSE;
	extended_data = data;
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArray<item_struct, initial_num_items, items_realloc_extra>::SetupReadWriteMode()
{
	if (read_only == FALSE && extended_data != (item_struct*)initial_data)
		free(extended_data);

	num_items = 0;
	num_alloc = initial_num_items;
	read_only = FALSE;
	extended_data = (item_struct*)initial_data;
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArray<item_struct, initial_num_items, items_realloc_extra>::SetupReadOnlyData(item_struct *data, int len_data)
{
	if (read_only == FALSE && extended_data != (item_struct*)initial_data)
		free(extended_data);

	num_items = len_data;
	num_alloc = len_data;
	read_only = TRUE;
	extended_data = data;
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArray<item_struct, initial_num_items, items_realloc_extra>::TakeContentsFrom(TStructsArray<item_struct, initial_num_items, items_realloc_extra> &other_inst)
{
	if (read_only == FALSE && extended_data != (item_struct*)initial_data)
		free(extended_data);

	// Copy all scalar data members from the other inst.
	num_items = other_inst.num_items;
	num_alloc = other_inst.num_alloc;
	read_only = other_inst.read_only;
	extended_data = other_inst.extended_data;

	if (other_inst.read_only == FALSE)
	{
		// Other instance owns its data.
		if (other_inst.extended_data == (item_struct*)other_inst.initial_data)
		{
			// Local array of the other instance is not empty.
			// Copy its contents into local array of 'this' instantce.
			if (num_items > 0)
				memcpy(initial_data, other_inst.initial_data, num_items*sizeof(item_struct));
			extended_data = (item_struct*)initial_data;
		}

		// Kill data in the passed instance.
		other_inst.num_items = 0;
		other_inst.num_alloc = initial_num_items;
		other_inst.extended_data = (item_struct*)other_inst.initial_data;
	}
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArray<item_struct, initial_num_items, items_realloc_extra>::ReserveTotalBufferSpace(int total_num_items)
{
	if (read_only == TRUE)
		return(FALSE);

	if (total_num_items <= num_alloc)
	{
		// The buffer already has enough space.
		return(TRUE);
	}

	int num_wanted = total_num_items;
	item_struct *buffer;
	if (extended_data == (item_struct*)initial_data)
	{
		// This is the first movement to the heap. Allocate exactly what the user is asking for.
		buffer = (item_struct*)malloc(num_wanted*sizeof(item_struct));
		if (buffer != NULL)
			memcpy(buffer, initial_data, initial_num_items*sizeof(item_struct));
	}
	else
	{
		// Allocate more than the user wants.
		num_wanted += items_realloc_extra;
		buffer = (item_struct*)realloc(extended_data, num_wanted*sizeof(item_struct));
	}

	if (buffer == NULL)
		return(FALSE);

	// Allocation/reallocation went fine.
	num_alloc = num_wanted;
	extended_data = buffer;
	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArray<item_struct, initial_num_items, items_realloc_extra>::AppendItem(item_struct &item)
{
	if (read_only == TRUE)
		return(FALSE);

	if (ReserveSpace() == FALSE)
		return(FALSE);

	memcpy(ItemPtr(num_items++), &item, sizeof(item_struct));
	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArray<item_struct, initial_num_items, items_realloc_extra>::AppendItems(item_struct *items_array, int array_len)
{
	if (read_only == TRUE)
		return(FALSE);

	if (array_len > 0)
	{
		if (ReserveSpace(array_len) == FALSE)
			return(FALSE);

		memcpy(ItemPtr(num_items), items_array, array_len*sizeof(item_struct));
		IncNumItems(array_len);
	}
	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArray<item_struct, initial_num_items, items_realloc_extra>::InsertItem(int ib_inx, item_struct &item)
{
	if (read_only == TRUE)
		return(FALSE);

	if (ib_inx < 0 || ib_inx > num_items)
		return(FALSE);

	if (ReserveSpace() == FALSE)
		return(FALSE);

	// The index is fine and there is enough space in the buffer.
	item_struct *new_rec = DataPtr()+ib_inx;
	if (ib_inx < num_items)
	{
		// Shift all trailer structures.
		item_struct *dest = DataPtr()+num_items;
		item_struct *rec = dest-1;
		while (dest > new_rec)
			*dest-- = *rec--;
	}

	// Copy in the item.
	memcpy(ItemPtr(ib_inx), &item, sizeof(item_struct));

	// Success.
	num_items++;
	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArray<item_struct, initial_num_items, items_realloc_extra>::InsertItems(int ib_inx, const item_struct *items_array, int array_len)
{
	if (read_only == TRUE)
		return(FALSE);

	if (ib_inx < 0 || ib_inx > num_items)
		return(FALSE);

	if (ReserveSpace(array_len) == FALSE)
		return(FALSE);

	// Index is fine and there is enough space in the current buffer.
	item_struct *new_rec = DataPtr()+ib_inx;
	if (ib_inx < num_items)
	{
		// Shift all trailer structures.
		item_struct *rec = DataPtr()+num_items-1;
		item_struct *dest = rec+array_len;
		while (rec >= new_rec)
			*dest-- = *rec--;
	}

	// Copy in the data items.
	for (int cnt=0; cnt<array_len; ++cnt)
		*new_rec++ = *items_array++;

	// Success.
	num_items += array_len;
	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArray<item_struct, initial_num_items, items_realloc_extra>::ReleaseItem(int inx)
{
	if (read_only == TRUE)
		return;

	if (inx < 0 || inx >= num_items)
		return;

	num_items--;
	if (inx < num_items)
		memmove(ItemPtr(inx), ItemPtr(inx+1), (num_items-inx)*sizeof(item_struct));
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArray<item_struct, initial_num_items, items_realloc_extra>::ReleaseItems(int inx, int num_items_to_release)
{
	if (read_only == TRUE)
		return;

	if (inx < 0 || inx+num_items_to_release >= num_items)
		return;

	num_items -= num_items_to_release;
	if (inx < num_items)
		memmove(ItemPtr(inx), ItemPtr(inx+num_items_to_release), (num_items-inx)*sizeof(item_struct));
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArray<item_struct, initial_num_items, items_realloc_extra>::QuickSort(int (__cdecl *compareFunction)(const item_struct *item1, const item_struct *item2))
{
	qsort(DataPtr(), NumItems(), sizeof(item_struct), (int (__cdecl*)(const void*, const void*))compareFunction);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArray<item_struct, initial_num_items, items_realloc_extra>::QuickSortWithCtx(void *ctx, int (__cdecl *compareFunction)(void *ctx, const item_struct *item1, const item_struct *item2))
{
	qsort_s(DataPtr(), NumItems(), sizeof(item_struct), (int (__cdecl*)(void*, const void*, const void*))compareFunction, ctx);
}

// ====================================================================
//
//   This template expects that item record needs the cleanup. This means that the item struct should
//	implement the following methods:
//
//		void  ReleaseObject();		// Release resources if they are owned by the item.
//		void	 WipeObject();		// Item can contain some old state or garbage. Clean the item
//								// without releasing anything.
//
// ====================================================================

template <class item_struct, int initial_num_items, int items_realloc_extra>
class TStructsArrayEx : public TStructsArray<item_struct, initial_num_items, items_realloc_extra>
{
public:

	TStructsArrayEx() : TStructsArray<item_struct, initial_num_items, items_realloc_extra>() { }
	TStructsArrayEx(item_struct *data, int len_data) : TStructsArray<item_struct, initial_num_items, items_realloc_extra>(data, len_data) { }
	~TStructsArrayEx() { ReleaseObject(); }

	void		SetupReadWriteMode();
	void		SetupReadOnlyData(item_struct *data, int len_data);
	void		TakeContentsFrom(TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra> &other_inst);

	inline void Clear() { SetupReadWriteMode(); }
				// This method should be overwritten because SetupReadWriteMode() is overwritten and it is not virtual.

	void ReleaseObject()
	{
		if (read_only == TRUE)
			return;

		// Release all child items.
		for (int i1=0; i1<num_items; ++i1)
			ItemRef(i1).ReleaseObject();

		// Fix the data fields.
		num_items = 0;
		num_alloc = initial_num_items;
		if (extended_data != (item_struct*)initial_data)
		{
			free(extended_data);
			extended_data = (item_struct*)initial_data;
		}
	}

	bool		AppendItem(item_struct &item);
	bool		AppendItems(item_struct *items_array, int array_len);

	bool		InsertItem(int insert_before_inx, item_struct &item);
	bool		InsertItems(int insert_before_inx, const item_struct *items_array, int array_len);
				// Both methods allow passing "insert_before_inx" param equal to the current length of the array.
				// In this case the item or items will be appended to the end of the array

	void		ReleaseItem(int inx);
	void		ReleaseItems(int inx, int num_items_to_release);
	void		ReleaseLastItem() { if (read_only == TRUE) return; if (num_items > 0) ReleaseItem(num_items-1); }
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra>::SetupReadWriteMode()
{
	if (read_only == FALSE)
	{
		// Release all child items.
		for (int i1=0; i1<num_items; ++i1)
			ItemRef(i1).ReleaseObject();

		if (extended_data != (item_struct*)initial_data)
			free(extended_data);
	}

	num_items = 0;
	num_alloc = initial_num_items;
	read_only = FALSE;
	extended_data = (item_struct*)initial_data;
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra>::SetupReadOnlyData(item_struct *data, int len_data)
{
	if (read_only == FALSE)
	{
		// Release all child items.
		for (int i1=0; i1<num_items; ++i1)
			ItemRef(i1).ReleaseObject();

		if (extended_data != (item_struct*)initial_data)
			free(extended_data);
	}

	num_items = len_data;
	num_alloc = len_data;
	read_only = TRUE;
	extended_data = data;
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra>::TakeContentsFrom(TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra> &other_inst)
{
	if (read_only == FALSE)
	{
		// Release all local child items.
		for (int i1=0; i1<num_items; ++i1)
			ItemRef(i1).ReleaseObject();

		if (extended_data != (item_struct*)initial_data)
			free(extended_data);
	}

	// Copy all scalar data members from other inst.
	num_items = other_inst.num_items;
	num_alloc = other_inst.num_alloc;
	read_only = other_inst.read_only;
	extended_data = other_inst.extended_data;

	if (other_inst.read_only == FALSE)
	{
		// Other instance owns its data.
		if (other_inst.extended_data == (item_struct*)other_inst.initial_data)
		{
			// Local array of the other instance is not empty.
			// Copy its contents into the local array of  'this' instantce.
			if (num_items > 0)
				memcpy(initial_data, other_inst.initial_data, num_items*sizeof(item_struct));
			extended_data = (item_struct*)initial_data;
		}

		// Kill data in other instance.
		other_inst.num_items = 0;
		other_inst.num_alloc = initial_num_items;
		other_inst.extended_data = (item_struct*)other_inst.initial_data;
	}
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra>::AppendItem(item_struct &item)
{
	if (read_only == TRUE)
		return(FALSE);

	if (ReserveSpace() == FALSE)
		return(FALSE);

	memcpy(ItemPtr(num_items++), &item, sizeof(item_struct));
	item.WipeObject();

	// Success.
	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra>::AppendItems(item_struct *items_array, int array_len)
{
	if (read_only == TRUE)
		return(FALSE);

	if (array_len > 0)
	{
		if (ReserveSpace(array_len) == FALSE)
			return(FALSE);

		memcpy(ItemPtr(num_items), items_array, array_len*sizeof(item_struct));
		IncNumItems(array_len);

		// Wipe out all passed items.
		for (int cnt=0; cnt<array_len; ++cnt)
		{
			items_array->WipeObject();
			items_array++;
		}
	}

	// Success.
	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra>::InsertItem(int ib_inx, item_struct &item)
{
	if (read_only == TRUE)
		return(FALSE);

	if (ib_inx < 0 || ib_inx > num_items)
		return(FALSE);

	if (ReserveSpace() == FALSE)
		return(FALSE);

	// The index is fine and there is enough space in the buffer.
	item_struct *new_rec = DataPtr()+ib_inx;
	if (ib_inx < num_items)
	{
		// Shift all trailer structures.
		item_struct *dest = DataPtr()+num_items;
		item_struct *rec = dest-1;
		while (dest > new_rec)
			*dest-- = *rec--;
	}

	// Copy in the item.
	memcpy(ItemPtr(ib_inx), &item, sizeof(item_struct));
	item.WipeObject();

	// Success.
	num_items++;
	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra>::InsertItems(int ib_inx, const item_struct *items_array, int array_len)
{
	if (read_only == TRUE)
		return(FALSE);

	if (ib_inx < 0 || ib_inx > num_items)
		return(FALSE);

	if (ReserveSpace(array_len) == FALSE)
		return(FALSE);

	// Index is fine and there is enough space in the current buffer.
	item_struct *new_rec = DataPtr()+ib_inx;
	if (ib_inx < num_items)
	{
		// Shift all trailer structures.
		item_struct *rec = DataPtr()+num_items-1;
		item_struct *dest = rec+array_len;
		while (rec >= new_rec)
			*dest-- = *rec--;
	}

	// Copy in and wipe out the data items.
	for (int cnt=0; cnt<array_len; ++cnt)
	{
		*new_rec++ = *items_array;
		items_array->WipeObject();
		items_array++;
	}

	// Success.
	num_items += array_len;
	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra>::ReleaseItem(int inx)
{
	if (read_only == TRUE)
		return;

	if (inx < 0 || inx >= num_items)
		return;

	// Release item.
	ItemRef(inx).ReleaseObject();

	num_items--;
	if (inx < num_items)
		memmove(ItemPtr(inx), ItemPtr(inx+1), (num_items-inx)*sizeof(item_struct));
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra>::ReleaseItems(int inx, int num_items_to_release)
{
	if (read_only == TRUE)
		return;

	if (inx < 0 || inx+num_items_to_release >= num_items)
		return;

	// Release items.
	int last_item_inx = inx+num_items_to_release;
	for (int i1=inx; i1 <= last_item_inx; ++i1)
		ItemRef(i1).ReleaseObject();

	num_items -= num_items_to_release;
	if (inx < num_items)
		memmove(ItemPtr(inx), ItemPtr(inx+num_items_to_release), (num_items-inx)*sizeof(item_struct));
}

// =================================================================
//
//   This template expects that item record needs cleanup plus item needs fixing after it has been
//   moved in the memory. This means that item record should implement the following methods:
//
//		void	 FixObjectMove();
//		void  ReleaseObject();
//		void	 WipeObject();
//
// =================================================================

template <class item_struct, int initial_num_items, int items_realloc_extra>
class TStructsArrayEx2 : public TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra>
{
public:

	TStructsArrayEx2() : TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra>() { }
	TStructsArrayEx2(item_struct *data, int len_data) : TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra>(data, len_data) { }

	TStructsArrayEx2(item_struct **initial_data_directory, int len_data_total);
		// This ctor creates a read/write array. It turned out that MS compiler has annoying limit on the size of the static
		// array that it is able to init with ctor calls. Problems start with arrays that containseveral thousands elements.
		// One reasonable way to go out of this sututation is to organize data in several chunks and then assemble these
		// chunks together. Data directory contains pointers to arrays. All arrays in this directory except for the last one
		// have length of items_realloc_extra. Second param is the number of items in all arrays in the passed directory
		// of arrays.

	~TStructsArrayEx2() { ReleaseObject(); }

	void TakeContentsFrom(TStructsArrayEx2<item_struct, initial_num_items, items_realloc_extra> &other_inst);

	void FixObjectMove()
	{
		assert(read_only == FALSE);

		if (num_alloc <= initial_num_items)
		{
			// Fix the core part of the object.
			extended_data = (item_struct*)initial_data;

			// Fix all available elements of the array.
			for (int i=0; i<num_items; ++i)
				ItemRef(i).FixObjectMove();
		}
	}

	inline bool	ReserveSpace(int num_extra_items = 1) { return(ReserveTotalBufferSpace(num_items+num_extra_items)); }
	bool			ReserveTotalBufferSpace(int total_num_items);

	bool  AppendItem(item_struct &item);
	bool  AppendItems(item_struct *items_array, int array_len);

	bool  InsertItem(int insert_before_inx, item_struct &item);
	bool  InsertItems(int insert_before_inx, const item_struct *items_array, int array_len);
			// Both methods allow passing "insert_before_inx" equal to the current length of the array.
			// In this case item or items will be appended to the end.

	void  ReleaseItem(int inx);
	void  ReleaseItems(int inx, int num_items_to_release);

	void  ReleaseLastItem() { if (read_only == TRUE) return; if (num_items > 0) ReleaseItem(num_items-1); }

	void  QuickSort(int (__cdecl *compareFunction)(const item_struct *item1, const item_struct *item2));
	void  QuickSortWithCtx(void *ctx, int (__cdecl *compareFunction)(void *ctx, const item_struct *item1, const item_struct *item2));
			// Note that the passed function should call FixObjectMove() on its arguments at the beginning.
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


template <class item_struct, int initial_num_items, int items_realloc_extra>
TStructsArrayEx2<item_struct, initial_num_items, items_realloc_extra>::TStructsArrayEx2(item_struct **initial_data_directory, int len_data_total)
		: TStructsArrayEx<item_struct, initial_num_items, items_realloc_extra>()
{
	// This ctor is tricky because it can fail. The result will be an empty array.
	if (ReserveSpace(len_data_total) == FALSE)
	{
		xpnd_error = TRUE;
		return;
	}

	while (len_data_total > 0)
	{
		// Process single chunk. Note that AppendItems() cannot fail because space was reserved
		// for items of all chunks in one attempt.
		int len_curr_chunk = (len_data_total > items_realloc_extra) ? items_realloc_extra : len_data_total;
		AppendItems(*initial_data_directory, len_curr_chunk);

		// Shift to the next directory element.
		initial_data_directory++;
		len_data_total -= len_curr_chunk;
	}
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArrayEx2<item_struct, initial_num_items, items_realloc_extra>::TakeContentsFrom(TStructsArrayEx2<item_struct, initial_num_items, items_realloc_extra> &other_inst)
{
	if (read_only == FALSE)
	{
		// Release all child items.
		for (int i1=0; i1<num_items; ++i1)
			ItemRef(i1).ReleaseObject();

		if (extended_data != (item_struct*)initial_data)
			free(extended_data);
	}

	// Copy all scalar data members from other inst.
	num_items = other_inst.num_items;
	num_alloc = other_inst.num_alloc;
	read_only = other_inst.read_only;
	extended_data = other_inst.extended_data;

	if (other_inst.read_only == FALSE)
	{
		// Other instance owns its data.
		if (other_inst.extended_data == (item_struct*)other_inst.initial_data)
		{
			// Local array of the other instance is not empty.
			// Copy its contents into the local array of 'this' instantce.
			if (num_items > 0)
				memcpy(initial_data, other_inst.initial_data, num_items*sizeof(item_struct));
			extended_data = (item_struct*)initial_data;

			// Fix all items after moving.
			for (int i2=0; i2<num_items; ++i2)
				ItemRef(i2).FixObjectMove();
		}

		// Kill data in another instance.
		other_inst.num_items = 0;
		other_inst.num_alloc = initial_num_items;
		other_inst.extended_data = (item_struct*)other_inst.initial_data;
	}
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArrayEx2<item_struct, initial_num_items, items_realloc_extra>::ReserveTotalBufferSpace(int total_num_items)
{
	if (read_only == TRUE)
		return(FALSE);

	if (total_num_items <= num_alloc)
	{
		// The buffer has enough space.
		return(TRUE);
	}

	int len_wanted = (total_num_items+items_realloc_extra)*sizeof(item_struct);
	item_struct *buffer;
	if (extended_data == (item_struct*)initial_data)
	{
		buffer = (item_struct*)malloc(len_wanted);
		if (buffer != NULL)
			memcpy(buffer, initial_data, initial_num_items*sizeof(item_struct));
	}
	else
	{
		buffer = (item_struct*)realloc(extended_data, len_wanted);
	}

	if (buffer == NULL)
		return(FALSE);

	// Allocation/reallocation went fine.
	num_alloc = total_num_items+items_realloc_extra;
	extended_data = buffer;

	// Fix all items after moving.
	for (int i1=0; i1<num_items; ++i1)
		ItemRef(i1).FixObjectMove();

	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArrayEx2<item_struct, initial_num_items, items_realloc_extra>::AppendItem(item_struct &item)
{
	if (read_only == TRUE)
		return(FALSE);

	if (ReserveSpace() == FALSE)
		return(FALSE);

	memcpy(ItemPtr(num_items), &item, sizeof(item_struct));
	ItemRef(num_items++).FixObjectMove();
	item.WipeObject();

	// Success.
	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArrayEx2<item_struct, initial_num_items, items_realloc_extra>::AppendItems(item_struct *items_array, int array_len)
{
	if (read_only == TRUE)
		return(FALSE);

	if (array_len > 0)
	{
		if (ReserveSpace(array_len) == FALSE)
			return(FALSE);

		item_struct *new_rec = ItemPtr(num_items);
		memcpy(new_rec, items_array, array_len*sizeof(item_struct));
		IncNumItems(array_len);

		// Fix taken items and wipe out the old places.
		for (int cnt=0; cnt<array_len; ++cnt)
		{
			new_rec->FixObjectMove();
			new_rec++;
			items_array->WipeObject();
			items_array++;
		}
	}

	// Success.
	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArrayEx2<item_struct, initial_num_items, items_realloc_extra>::InsertItem(int ib_inx, item_struct &item)
{
	if (read_only == TRUE)
		return(FALSE);

	if (ib_inx < 0 || ib_inx > num_items)
		return(FALSE);

	if (ReserveSpace() == FALSE)
		return(FALSE);

	// The index is fine and there is enough space in the array.
	item_struct *new_rec = DataPtr()+ib_inx;
	if (ib_inx < num_items)
	{
		// Shift all trailer structures.
		item_struct *dest = DataPtr()+num_items;
		item_struct *rec = dest-1;
		while (dest > new_rec)
		{
			*dest = *rec--;
			dest->FixObjectMove();
			dest--;
		}
	}

	// Copy in the item.
	memcpy(ItemPtr(ib_inx), &item, sizeof(item_struct));
	ItemRef(ib_inx).FixObjectMove();
	item.WipeObject();

	// Success.
	num_items++;
	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
bool TStructsArrayEx2<item_struct, initial_num_items, items_realloc_extra>::InsertItems(int ib_inx, const item_struct *items_array, int array_len)
{
	if (read_only == TRUE)
		return(FALSE);

	if (ib_inx < 0 || ib_inx > num_items)
		return(FALSE);

	if (ReserveSpace(array_len) == FALSE)
		return(FALSE);

	// The index is fine and there is enough space in the current buffer.
	item_struct *new_rec = DataPtr()+ib_inx;
	if (ib_inx < num_items)
	{
		// Shift all trailer structures.
		item_struct *rec = DataPtr()+num_items-1;
		item_struct *dest = rec+array_len;
		while (rec >= new_rec)
		{
			*dest = *rec--;
			dest->FixObjectMove();
			dest--;
		}
	}

	// Copy in and wipe out the data items.
	for (int cnt=0; cnt<array_len; ++cnt)
	{
		*new_rec = *items_array;
		new_rec->FixObjectMove();
		new_rec++;
		items_array->WipeObject();
		items_array++;
	}

	// Success.
	num_items += array_len;
	return(TRUE);
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArrayEx2<item_struct, initial_num_items, items_realloc_extra>::ReleaseItem(int inx)
{
	if (read_only == TRUE)
		return;

	if (inx < 0 || inx >= num_items)
		return;

	// Release item.
	ItemRef(inx).ReleaseObject();

	num_items--;
	if (inx < num_items)
	{
		// Bulk move trailing items.
		memmove(ItemPtr(inx), ItemPtr(inx+1), (num_items-inx)*sizeof(item_struct));

		// Fix trailing items after moving.
		for (int i2=inx; i2<num_items; ++i2)
			ItemRef(i2).FixObjectMove();
	}
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArrayEx2<item_struct, initial_num_items, items_realloc_extra>::ReleaseItems(int inx, int num_items_to_release)
{
	if (read_only == TRUE)
		return;

	if (inx < 0 || inx+num_items_to_release >= num_items)
		return;

	// Release items.
	int last_item_inx = inx+num_items_to_release;
	for (int i1=inx; i1 <= last_item_inx; ++i1)
		ItemRef(i1).ReleaseObject();

	num_items -= num_items_to_release;
	if (inx < num_items)
	{
		// Bulk move all trailing items.
		memmove(ItemPtr(inx), ItemPtr(inx+num_items_to_release), (num_items-inx)*sizeof(item_struct));

		// Fix trailing items after moving them.
		for (int i2=inx; i2<num_items; ++i2)
			ItemRef(i2).FixObjectMove();
	}
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArrayEx2<item_struct, initial_num_items, items_realloc_extra>::QuickSort(int (__cdecl *compareFunction)(const item_struct *item1, const item_struct *item2))
{
	qsort(DataPtr(), NumItems(), sizeof(item_struct), (int (__cdecl*)(const void*, const void*))compareFunction);

	// Fix all items after moving them around.
	for (int inx=0; inx<num_items; ++inx)
		ItemRef(inx).FixObjectMove();
}

template <class item_struct, int initial_num_items, int items_realloc_extra>
void TStructsArrayEx2<item_struct, initial_num_items, items_realloc_extra>::QuickSortWithCtx(void *ctx, int (__cdecl *compareFunction)(void *ctx, const item_struct *item1, const item_struct *item2))
{
	// Call the sort function that will do the work.
	qsort_s(DataPtr(), NumItems(), sizeof(item_struct), (int (__cdecl*)(void*, const void*, const void*))compareFunction, ctx);

	// Fix all items after moving them around.
	for (int inx=0; inx<num_items; ++inx)
		ItemRef(inx).FixObjectMove();
}


// ===================================================================
//
//	Array of wide characters.
//
//	Note that the terminating NULL is always present, but it is not included into the count of array items.
//
// ===================================================================

template <int initial_num_items, int items_realloc_extra = 2048>
class TTextBuffer : public TStructsArray<wchar_t, initial_num_items, items_realloc_extra>
{
public:

	TTextBuffer() { ItemRef(0) = 0; }
	TTextBuffer(const wchar_t *initial_data) { ItemRef(0) = 0; Append(initial_data); }

	inline operator const wchar_t*() const { return(DataPtr()); }

	inline bool	operator == (const wchar_t *str) const { return(wcscmp(DataPtr(), str) == 0); }
	inline bool	operator != (const wchar_t *str) const { return(wcscmp(DataPtr(), str) != 0); }

	inline bool	IsEmpty() const { return(NumItems() == 0); }

	void TakeContentsFrom(TTextBuffer &other_inst)
	{
		TakeContentsFromSpecial(other_inst);
		xpnd_error = other_inst.xpnd_error;

		if (other_inst.read_only == FALSE)
			other_inst.xpnd_error = FALSE;
	}

	// Prohibit using this method for a while.
	inline void Clear() { assert(FALSE); }

	void ClearBuffer() { SetNumItems(0); ItemRef(0) = 0; xpnd_error = FALSE; }

	void SetupReadOnlyString(const wchar_t *string)
	{
		int len_data = (int)(wcslen(string)+1);
		SetupReadOnlyData((wchar_t*)string, len_data);
		IncNumItems(-1);
	}

	bool Set(const wchar_t *string, int len = -1)
	{
		ClearBuffer();
		return(Append(string, len));
	}

	bool Append(const wchar_t ch)
	{
		return(Append(&ch, 1));
	}

	bool Append(const wchar_t *string, int len = -1)
	{
		int ext_len = (len < 0) ? (int)wcslen(string) : len;
		if (ReserveSpace(ext_len+1) == FALSE)
		{
			xpnd_error = TRUE;
			return(FALSE);
		}

		memcpy(ItemPtr(num_items), string, ext_len*sizeof(wchar_t));
		IncNumItems(ext_len);
		ItemRef(num_items) = 0;
		return(TRUE);
	}

	bool AppendQuoted(const wchar_t *string, int len = -1, bool front_space = TRUE)
	{
		int ext_len = (front_space == TRUE) ? 1 : 0;
		ext_len += (len < 0) ? (int)wcslen(string) : len;
		ext_len += 2;

		if (ReserveSpace(ext_len+1) == FALSE)
		{
			xpnd_error = TRUE;
			return(FALSE);
		}

		// Reserving of the space succeeded. Appending items cannot fail now.
		if (front_space == TRUE)
			Append(L" ");
		Append(L"\"");
		Append(string, len);
		Append(L"\"");
		return(TRUE);
	}

	bool Insert(int inx_before, const wchar_t *new_data, int len = -1)
	{
		int ext_len = (len < 0) ? (int)wcslen(new_data) : len;
		if (InsertItems(inx_before, (wchar_t*)new_data, ext_len) == FALSE)
		{
			xpnd_error = TRUE;
			return(FALSE);
		}

		ItemRef(num_items) = 0;
		return(TRUE);
	}

	void  ReleaseItem(int inx) { TStructsArray::ReleaseItem(inx); ItemRef(num_items) = 0; }
	void  ReleaseItems(int inx, int num_items_to_release) { TStructsArray::ReleaseItems(inx, num_items_to_release); ItemRef(num_items) = 0; }
	void  ReleaseLastItem() { if (read_only == TRUE) return; if (num_items > 0) ReleaseItem(num_items-1); }

	void  TruncateTo(int len) { if (len >= 0 && len < num_items) { num_items = len; extended_data[num_items] = 0; } }

protected:

	void TakeContentsFromSpecial(TTextBuffer &other_inst)
	{
		if (read_only == FALSE && extended_data != (wchar_t*)initial_data)
			free(extended_data);

		// Copy all scalar data members from the other inst.
		num_items = other_inst.num_items;
		num_alloc = other_inst.num_alloc;
		read_only = other_inst.read_only;
		extended_data = other_inst.extended_data;

		// Ensure that current buffer has space for the trailing NULL.
		assert(num_items < num_alloc);

		if (other_inst.read_only == FALSE)
		{
			// Other instance owns its data.
			if (other_inst.extended_data == (wchar_t*)other_inst.initial_data)
			{
				// Local array of the other instance is not empty.
				// Copy its contents into the local array of 'this' instantce.
				if (num_items > 0)
					memcpy(initial_data, other_inst.initial_data, (num_items+1)*sizeof(wchar_t));
				extended_data = (wchar_t*)initial_data;
			}

			// Kill data in the passed instance.
			other_inst.num_items = 0;
			other_inst.num_alloc = initial_num_items;
			other_inst.extended_data = (wchar_t*)other_inst.initial_data;
		}
	}
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef TTextBuffer<40, 160> TTextBuffer40;
typedef TTextBuffer<80, 320> TTextBuffer80;
typedef TTextBuffer<256, 1024> TTextBuffer256;

typedef TTextBuffer<40, 120> TNameBuffer;
typedef TTextBuffer<2*MAX_PATH, 4*MAX_PATH> TFileNameBuffer;
typedef TTextBuffer<2048, 4096> TMessageBuffer;

// =================================================================
//
//  This array expects that items can be only added. The items cannot be removed.
//
//  Element of the array with the index zero is not used.
//
//  NB: The number of the hash buckets should be a strict power of 2.
//
// =================================================================

struct THashedStructsArrayItem
{
	int		next_same_hash_inx;
};

template <class item_struct, int initial_num_items, int items_realloc_extra, int num_hash_buckets>
class THashedStructsArray : public TStructsArray<item_struct, initial_num_items, items_realloc_extra>
{
public:

	THashedStructsArray()
	{
		// Ensure restriction on the number of the hash buckets.
		assert((num_hash_buckets & (num_hash_buckets-1)) == 0);
		memset(hash_buckets, 0, sizeof(hash_buckets));

		// Item with the index 0 is not used.
		memset(initial_data, 0, sizeof(item_struct));
		SetNumItems(1);
	}

	//
	// Overwrite this non virtual method.
	//
	void Clear()
	{
		TStructsArray<item_struct, initial_num_items, items_realloc_extra>::Clear();
		memset(hash_buckets, 0, sizeof(hash_buckets));
		memset(initial_data, 0, sizeof(item_struct));
		SetNumItems(1);
	}

	void TakeContentsFrom(THashedStructsArray<item_struct, initial_num_items, items_realloc_extra, num_hash_buckets> &other_inst)
	{
		TStructsArray<item_struct, initial_num_items, items_realloc_extra>::TakeContentsFrom(other_inst);
		for (int ibkt=0; ibkt<num_hash_buckets; ++ibkt)
			hash_buckets[ibkt] = other_inst.hash_buckets[ibkt];

		// Finalize cleaning the other instance.
		memset(other_inst.hash_buckets, 0, sizeof(other_inst.hash_buckets));
		other_inst.SetNumItems(1);
	}

	int NumRealItems() const { return(NumItems()-1); }

	//
	// Items cannot be removed.
	//
	void  ReleaseItem(int inx) { assert(FALSE); }
	void  ReleaseItems(int inx, int num_items_to_release) { assert(FALSE); }
	void  ReleaseLastItem() { assert(FALSE); }

	//
	// Return value is an index of the new/identical item. The return value is 0 in case of failure to add
	// the new item to the table.
	//
	int FindOrAppend(item_struct &item)
	{
		// Find bucket for this item.
		int hash_inx = CalcHashValue(item) & (num_hash_buckets-1);

		// Check for similar items.
		int inx = hash_buckets[hash_inx];
		while (inx > 0)
		{
			item_struct &curr = extended_data[inx];
			if (CompareRecords(item, curr) == 0)
			{
				// Identical record was found.
				item_added = FALSE;
				return(inx);
			}

			inx = curr.next_same_hash_inx;
		}

		// Add new record.
		item.next_same_hash_inx = hash_buckets[hash_inx];
		if (AppendItem(item) == FALSE)
		{
			// Failure to add the new item.
			item_added = FALSE;
			return(0);
		}

		// Update data fields.
		item_added = TRUE;
		int new_item_inx = num_items-1;
		hash_buckets[hash_inx] = new_item_inx;
		return(new_item_inx);
	}

	void ReleaseLatestItem()
	{
		if (num_items <= 1)
			return;

		// The record is available. Release it.
		item_struct &item = extended_data[num_items-1];
		int hash_inx = CalcHashValue(item) & (num_hash_buckets-1);
		hash_buckets[hash_inx] = item.next_same_hash_inx;
		IncNumItems(-1);
	}

	//
	// This method has exteremely rare use. Item with the passed index should already belong
	// to the array and at the same time it should not be present in the hash lists.
	//
	void AddToHashIndex(int item_inx)
	{
		// Find bucket for this item.
		int hash_inx = CalcHashValue(ItemRef(item_inx)) & (num_hash_buckets-1);

		// Update the list.
		ItemRef(item_inx).next_same_hash_inx = hash_buckets[hash_inx];
		hash_buckets[hash_inx] = item_inx;
	}

	bool		item_added;
				// This flag exposes additional result of FindOrAppend(...) method.

protected:

	virtual DWORD	CalcHashValue(item_struct &item) = 0;
	virtual int			CompareRecords(item_struct &item1, item_struct &item2) = 0;
						// Ret value: 0 - identical, !0 - different);

	int		hash_buckets[num_hash_buckets];
				// Item 0 of the array is not used. Hash bucket contains index of the first item with the given hash or zero
				// if table does not contain items with this hash value.

	static unsigned short CalcStrHash(const wchar_t *str)
					{ unsigned short val = 0; while (*str != 0) { val = val + (short)(*str++); val = val << 1; if ((val & 0x8000) != 0) val++; } return(val); }
};

// =================================================================
//
//    Helper class for handling variable sized arrays.
//
//    This array does NOT have preallocated records.
//
//  Current design expects that:
//
//    1. The item struct is POD.
//    2. The object itself is never moved.
//
// =================================================================

template <class item_struct, int items_realloc_extra>
class THeapBasedArray
{
public:

	THeapBasedArray() { Clear(); }
	~THeapBasedArray() { if (array_data != NULL) free(array_data); }

	inline void	Clear() { num_items = num_alloc = 0; array_data = NULL; }
	inline void	ReleaseData() { if (array_data != NULL) free(array_data); Clear();  }

	void TakeContentsFrom(THeapBasedArray<item_struct, items_realloc_extra> &other_inst)
	{
		if (array_data != NULL)
			free(array_data);

		num_items = other_inst.num_items; num_alloc = other_inst.num_alloc;
		array_data = other_inst.array_data; other_inst.array_data = NULL;

		other_inst.ReleaseData();
	}

	inline item_struct		&ItemRef(int inx) const		{ return(array_data[inx]); }
	inline item_struct		*ItemPtr(int inx) const		{ return(array_data+inx); }
	inline item_struct		&operator[] (int inx) const	{ return(array_data[inx]); }
	inline item_struct		*DataPtr() const				{ return(array_data); }

	inline bool	ReserveSpace(int num_extra_items = 1) { return(ReserveTotalBufferSpace(num_items+num_extra_items)); }
	bool			ReserveTotalBufferSpace(int total_num_items);

	void		SetNumItems(int new_num)	{ assert(new_num <= num_alloc); num_items = new_num;  }
	void		IncNumItems(int delta = 1)		{ num_items += delta; assert(num_items <= num_alloc); }

	inline int		NumItems() const			{ return(num_items);  }
	inline int		NumAllocedItems() const	{ return(num_alloc);  }

	bool		AppendItem(item_struct &item);
	bool		AppendItems(const item_struct *items_array, int array_len);

	bool		InsertItem(int insert_before_inx, item_struct &item);
				// It is ok to call this method with insert before index equal to the current length of the array
				// for appending item to the end of array.

	void		ReleaseItem(int inx);

	void  QuickSort(int (__cdecl *compareFunction)(const item_struct *item1, const item_struct *item2));

protected:

	int				num_items;
	int				num_alloc;
	item_struct		*array_data;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <class item_struct, int items_realloc_extra>
bool THeapBasedArray<item_struct, items_realloc_extra>::ReserveTotalBufferSpace(int total_num_items)
{
	if (total_num_items <= num_alloc)
	{
		// The buffer has enough space.
		return(TRUE);
	}

	int len_wanted = (total_num_items+items_realloc_extra)*sizeof(item_struct);
	item_struct *buffer;
	if (array_data == NULL)
	{
		buffer = (item_struct*)malloc(len_wanted);
	}
	else
	{
		buffer = (item_struct*)realloc(array_data, len_wanted);
	}

	if (buffer == NULL)
		return(FALSE);

	// Allocation/reallocation went fine.
	num_alloc = total_num_items+items_realloc_extra;
	array_data = buffer;
	return(TRUE);
}

template <class item_struct, int items_realloc_extra>
bool THeapBasedArray<item_struct, items_realloc_extra>::AppendItem(item_struct &item)
{
	if (ReserveSpace() == FALSE)
		return(FALSE);

	memcpy(&ItemRef(num_items++), &item, sizeof(item_struct));
	return(TRUE);
}

template <class item_struct, int items_realloc_extra>
bool THeapBasedArray<item_struct, items_realloc_extra>::AppendItems(const item_struct *items_array, int array_len)
{
	if (ReserveSpace(array_len) == FALSE)
		return(FALSE);

	memcpy(&ItemRef(num_items), items_array, array_len*sizeof(item_struct));
	IncNumItems(array_len);
	return(TRUE);
}

template <class item_struct, int items_realloc_extra>
bool THeapBasedArray<item_struct, items_realloc_extra>::InsertItem(int ib_inx, item_struct &item)
{
	if (ib_inx < 0 || ib_inx > num_items)
		return(FALSE);

	if (ReserveSpace() == FALSE)
		return(FALSE);

	// Index is fine and there is enough space in the array.
	if (ib_inx < num_items)
	{
		// Shift all trailer structures.
		item_struct *rec = DataPtr()+num_items;
		item_struct *new_rec = DataPtr()+ib_inx;
		while (rec > new_rec)
		{
			*rec = *(rec-1);
			rec--;
		}
	}

	memcpy(&ItemRef(ib_inx), &item, sizeof(item_struct));
	num_items++;
	return(TRUE);
}

template <class item_struct, int items_realloc_extra>
void THeapBasedArray<item_struct, items_realloc_extra>::ReleaseItem(int inx)
{
	if (inx < 0 || inx >= num_items)
		return;

	num_items--;
	if (inx < num_items)
		memmove(&ItemRef(inx), &ItemRef(inx+1), (num_items-inx)*sizeof(item_struct));
}

template <class item_struct, int items_realloc_extra>
void THeapBasedArray<item_struct, items_realloc_extra>::QuickSort(int (__cdecl *compareFunction)(const item_struct *item1, const item_struct *item2))
{
	qsort(DataPtr(), NumItems(), sizeof(item_struct), (int (__cdecl*)(const void*, const void*))compareFunction);
}

// ======================================================================
//
//  This template is a storage for arbitrary binary data. It is good for storing big amounts of data that consist
//  of many small pieces. This storage does not allow discarding these small pieces one by one. All these pieces
//  should be discarded only all together. Such design allows to save on the allocation headers and on the perf
//  overhead of calling more complex allocation functions if comparing this structure with storing every piece of
//  data in the individual heap allocations.
//
//  Current design expects that the object itself is never moved.
//
// ======================================================================

template <int initial_num_buff_ptr_slots, int buff_ptr_slots_realloc_extra, int min_data_buff_size = 0x40000>
class TDataBuffersArray : public TStructsArray<BYTE*, initial_num_buff_ptr_slots, buff_ptr_slots_realloc_extra>
{
public:

	TDataBuffersArray() { ClearVars(); }
	~TDataBuffersArray() { ReleaseData(); }

	void  Clear() { ReleaseData(); ClearVars(); SetupReadWriteMode(); }

	BYTE	*AddByteAlignedData(void *data, long data_len_in_bytes)	{ return(AddData(data, data_len_in_bytes, sizeof(BYTE))); }
	BYTE	*AddWordAlignedData(void *data, long data_len_in_bytes)	{ return(AddData(data, data_len_in_bytes, sizeof(WORD))); }
	BYTE	*AddDwordAlignedData(void *data, long data_len_in_bytes)	{ return(AddData(data, data_len_in_bytes, sizeof(DWORD))); }
	BYTE	*AddQwordAlignedData(void *data, long data_len_in_bytes)	{ return(AddData(data, data_len_in_bytes, 2*sizeof(DWORD))); }
	BYTE	*AddStructAlignedData(void *data, long data_len_in_bytes)	{ return(AddData(data, data_len_in_bytes, sizeof(DWORD))); }
				// Param data can be NULL. In this case methods allocate uninited space.

	BYTE	*AllocStuctAlignedSpace(long data_len_in_bytes) { return(AddData(NULL, data_len_in_bytes, sizeof(DWORD))); }
				// This method returns pointer to an uninited space.

	BYTE	*GetDataBuffPtr(int buff_inx, long &data_len);
				// Each big data buffer contains its length in its first DWORD.
protected:

	void		ClearVars() { m_curr_buff = NULL; m_curr_buff_offs = m_curr_buff_len = 0; }
	void		ReleaseData() { for (int i=0; i<NumItems(); ++i) free(ItemRef(i)); }
	BYTE	*AddData(void *data, long num_bytes, int alignment_val);

	BYTE		*m_curr_buff;
					// Current buffer belongs to the main array of pointers.

	long			m_curr_buff_offs;
	long			m_curr_buff_len;
					// Length of some particular data buffer is not fixed and it can be bigger than the default size.
					// This happens when application asks to store data fragment that is bigger than the default
					// size of the buffer.
};

#define DBA_ROUND_UP(val, align_val) (((val)+align_val-1) & ~(align_val-1))

template <int initial_num_buff_ptr_slots, int buff_ptr_slots_realloc_extra, int min_data_buff_size>
BYTE *TDataBuffersArray<initial_num_buff_ptr_slots, buff_ptr_slots_realloc_extra, min_data_buff_size>::AddData(void *data, long num_bytes_needed, int align_val)
{
	assert(num_bytes_needed > 0);

	// Buffer pointer and the buff len are expected to be 8 bytes aligned.
	long aligned_offs = DBA_ROUND_UP(m_curr_buff_offs, align_val);

	if (m_curr_buff == NULL || (m_curr_buff_len-aligned_offs) < num_bytes_needed)
	{
		// Put actual length of data into the buffer.
		if (m_curr_buff != NULL)
			((long*)m_curr_buff)[0] = m_curr_buff_offs-2*sizeof(long);

		// Allocate new buffer and add it to the array of buffers.
		long new_buff_len = DBA_ROUND_UP(__max(num_bytes_needed, min_data_buff_size)+2*sizeof(long), 8);
		BYTE *new_buff = (BYTE*)malloc(new_buff_len);
		if (new_buff == NULL)
			return(NULL);

		if (AppendItem(new_buff) == FALSE)
		{
			free(new_buff);
			return(NULL);
		}

		// Allocation went fine. The buffer can be used.
		m_curr_buff = new_buff;
		m_curr_buff_offs = 2*sizeof(long);
		m_curr_buff_len = new_buff_len;

		// Realign the offset. Application may ask for the QWORD alignment.
		aligned_offs = DBA_ROUND_UP(m_curr_buff_offs, align_val);
	}

	// Current buffer has enough space for storing the data.
	if (data != NULL)
		memcpy(m_curr_buff+aligned_offs, data, num_bytes_needed);
	m_curr_buff_offs = aligned_offs+num_bytes_needed;

	return(m_curr_buff+aligned_offs);
}

template <int initial_num_buff_ptr_slots, int buff_ptr_slots_realloc_extra, int min_data_buff_size>
BYTE *TDataBuffersArray<initial_num_buff_ptr_slots, buff_ptr_slots_realloc_extra, min_data_buff_size>::GetDataBuffPtr(int buff_inx, long &data_len)
{
	if (buff_inx < 0 || buff_inx >= NumItems())
		return(NULL);

	if (buff_inx == NumItems()-1)
		data_len = m_curr_buff_offs-2*sizeof(long);
	else data_len = *(long*)ItemRef(buff_inx);

	return(ItemRef(buff_inx)+2*sizeof(long));
}

#endif	// Common_Buffers_H


