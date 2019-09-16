//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Simple look aside table.
//

#ifndef	Common_LookAsideTable_H
#define	Common_LookAsideTable_H

//
//  This table allows handling only those classes that have the default constructor.
//
template <class lookAsideTableItem, int lookAsideTableSize>
class TLookAsideTable
{
public:

	TLookAsideTable() { m_num_items = 0; }
	~TLookAsideTable() { ClearTable(); }

	void ClearTable()
	{
		// Release all cached items if any.
		while (m_num_items > 0)
			delete m_items[--m_num_items];
	}

	lookAsideTableItem *GetItem()
	{
		if (m_num_items > 0)
		{
			// Recycled items are available.
			return(m_items[--m_num_items]);
		}
		else
		{
			// Allocate new item.
			return(new lookAsideTableItem());
		}
	}

	lookAsideTableItem *GetItemNoAlloc()
	{
		if (m_num_items > 0)
		{
			// Recycled items are available.
			return(m_items[--m_num_items]);
		}

		// The table is empty.
		return(NULL);
	}

	void RecycleItem(lookAsideTableItem *item)
	{
		if (m_num_items < lookAsideTableSize)
		{
			// Store the passed item for future use.
			m_items[m_num_items++] = item;
		}
		else
		{
			// Get rid of the item.
			delete item;
		}
	}

protected:

	lookAsideTableItem		*m_items[lookAsideTableSize];
	int						m_num_items;
};

#endif	// Common_LookAsideTable_H


