//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	Common_StacksAndQueues_H
#define	Common_StacksAndQueues_H

//
//   Circular stack is used for storing entities that can be silently discarded like the history of browsing something.
//
//   The element of the stack is expected to be a simple POD.
//
template <class stackItem, int numStackItems>
class TCircularStack
{
public:

	TCircularStack() { m_num_items = 0; }

	int GetNumItems() const { return(m_num_items); }

	void PushItem(stackItem &item)
	{
		if (m_num_items >= numStackItems)
		{
			// Abandon the oldest item.
			m_num_items--;
			for (int inx=0; inx<m_num_items; ++inx)
				m_items[inx] = m_items[inx+1];
		}

		// Store the new item.
		m_items[m_num_items++] = item;
	}

	bool PopItem(stackItem &item)
	{
		if (m_num_items > 0)
		{
			// Give out the item.
			item = m_items[--m_num_items];
			return(TRUE);
		}

		// The stack is empty.
		return(FALSE);
	}

	bool CompareWithTipItem(stackItem &item)
	{
		return(m_num_items > 0 && item.CompareTo(m_items[m_num_items-1]) == TRUE);
	}

protected:

	stackItem			m_items[numStackItems];
	int					m_num_items;
};

#endif	// Common_StacksAndQueues_H


