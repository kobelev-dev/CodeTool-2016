//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   In memory storage for C/C++ objects.
//

#ifndef	TextScan_CppDatabase_H
#define	TextScan_CppDatabase_H

#ifndef   Common_DoubleLinkedList_H
#include  "Common/DoubleLinkedList.H"
#endif
#ifndef   TextScan_RawScan_H
#include  "TextScan/RawScan.H"
#endif

class	TCppItemBase;
class	TCppDirectChildrenIter;
class	TCppDefnBase;
class	TCppDataTypeBase;
class	TCppFunctionTypeBase;

struct	TCppCvPtrRefSpec;
class	TCppDefnsSpace;
enum	TCppStorageClass;
enum	TCppBuiltInTypeIdent;
enum	TCppExprOperandType;

struct	TCppBuiltInTypeDefn;
struct	TCppBitFieldTypeDefn;
struct	TCppCvPtrRefTypeDefn;
struct	TCppArrayTypeDefn;
struct	TCppStructTypeDefn;
struct	TCppDataFieldDefn;
struct	TCppDataFieldInitializer;
struct	TCppFunctionTypeDefn;
struct	TCppCodeEntryPointDefn;
struct	TCppPossibleExceptionSpec;
struct	TCppLinkageSpec;

struct	TCppClassTemplateDefn;
struct	TCppFunctionTemplateDefn;
struct	TCppBlockDefn;
struct	TCppLabelDefn;
struct	TCppCaseLabelDefn;
struct	TCppOperandExpr;

class	TCppDatabase;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//   ====== Part 1. ==========  Cpp objects infrastructure  =============
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//
// Every Cpp database object may contain information about places in the source code where this object
// was defined and/or used. Information about these places is described using the TCppItemCtvEventRef
// structure. This struct contians either a <parsing_id, event_id> pair or information about an unsubmited
// event. Sometimes it is not possible to submit the event immediately because it is not clear if particular
// Cpp object will be published in the Cpp database or destroyed. In the meanwhile this info needs to be
// stored somewhere. The structure below is a prototype. Cpp parser uses a derived structure.
//
struct TCppCtvRefEventInfoBase
{
	virtual void RecycleEventInfo() = 0;
};

enum TCppItemSrcLinkType
{
	clink_none,

	clink_location,					// This type of reference is used for objects that that do not have names, like statements
									// and expressions. They cannot be referenced from anywhere else in the code.
	clink_use_defn,					// This type of reference is used for objects that do not require explicit definition, like built
									// in types, nameless types, namespaces, etc.
	clink_fwd_decl,					// This is a forward declaration of the object. Concept of the forward declaration is applicable
									// not only to elaborated types. This can also be an extern definition of  the data field or
									// a fucntion prototype.
	clink_definition,					// Primary definition of the object. C and C++ follow the one definition rule. Every object
									// can have only one definition in each translation unit
	clink_using,						// This is use of the object that was defined somewhere else. For example this can be using
									// the named label in the GOTO statement.

	clink_warning,					// A warning message that is related to the cpp object.
	clink_error,						// Syntax error that is related to the cpp object.

	clink_num_types,

	clink_unsubm_flag = 0x40,		// This flag indicates what option of the anonymous union in the TCppItemCtvEventRef
									// struct should be used.
};

struct TCppItemCtvEventRef
{
	TCppItemSrcLinkType		ctv_ref_type;

	union
	{
		struct
		{
			ID						ctv_pars_id;
			ID						ctv_event_id;
		};

		TCppCtvRefEventInfoBase		*ctv_unsubm_info;
	};
};

typedef TStructsArray<TCppItemCtvEventRef, 1, 4> TCtvEventRefsArray;

//
//  Base class of all objects in the C/C++ database.
//
class TCppItemBase : public TListItem
{
public:

		TCppItemBase(TCppItemType cxt, ID id = 0) { m_cib_id = id; m_cib_type = cxt; m_cib_parent = NULL; }
		~TCppItemBase();
			// Destructor of the derived class is responsible for freeing the owned children if any.

	inline  ID					ItemId() const { return(m_cib_id); }
	inline  TCppItemType		ItemType() const { return(m_cib_type); }

	inline  TCppItemBase		*ItemParent() const { return(m_cib_parent); }
	inline  const wchar_t		*ItemTypeEnumName() const { return(TCppItemHelper::GetItemTypeEnumName(m_cib_type)); }

	inline  bool				IsPublished() const;
								// The item is considered to be published when it is a direct or an indirect child of the global namespace.

	inline  bool				IsParentOf(TCppItemBase *child_item) const;
	inline  bool				IsChildOf(TCppItemBase *parent_item) const;

	virtual const wchar_t		*BasicItemName() const { return(TCppItemHelper::GetItemTypeEnumName(m_cib_type)); }
								// TCppDefnBase overwrites this method and returns the name of the object if it has one.

	static int	__cdecl CompareBasicItemNames(const TCppItemBase **ppi1, const TCppItemBase **ppi2)
	{
		return(wcscmp((*ppi1)->BasicItemName(), (*ppi2)->BasicItemName()));
	}

public:

	virtual bool				IsStatementBase() const { return(FALSE); }
	virtual bool				IsDefinition() const { return(FALSE); }

	virtual bool				IsDataType(TCppDefnBase *context = NULL) const { return(FALSE); }
	virtual bool				IsFunctionType(TCppDefnBase *context = NULL) const { return(FALSE); }
								// There is a caviat with these methods because the name of the template inside this particular
								// template and in all its nested classes/member templates/methods, etc is a data/function type.
								// In all other contexts it is not a data/function type.

	virtual bool				IsExpression() const { return(FALSE); }

	virtual bool				IsUnresolvedObject() const { return(FALSE); }
								// Unresolved object is always a definition. These objects are created when parsing code references
								// something that was not defined or when definition with particular name, overloaded operator or
								// overloaded conversion with the given destination cannot be created in the target names space.

	virtual bool				IsClassTemplateFamily() const { return(FALSE); }
								// Class templates family consists of ClassTemplate, ClassTemplateSpec and ClassTemplateInst.
								// This is the first group of overloadable objects. Note that simple class is NOT a part of this group.

	virtual bool				IsFunctionFamily() const { return(FALSE); }
								// Functions family consists of CodeEntryPoint, FuncTemplate, FuncTemplateSpec and FuncTemplateInst.
								// This is a second group of overloadable objects. Note that simple function IS a part of this group.

	virtual bool				IsFunctionName() const { return(FALSE); }
								// This method returns TRUE for all objects that can be named not only with an identifier, but also with
								// a token of an overloaded operator, dest type of an overloaded conversion, or with a destructor form
								// of the name.

	virtual bool				IsNamespace() const { return(FALSE); }
	virtual bool				IsStructType() const { return(FALSE); }
	virtual bool				IsBlock() const { return(FALSE); }

	inline  bool				IsTemplateInstantiation() const;
								// Return value is TRUE if the object is either a template instantiation itself or if it is a direct or indirect child
								// of a template instantiation. Note that all objects inside the template instantiation are created only during
								// the template instantiation process. They cannot be added there later.
public:

	virtual bool				CanHaveChildren() const { return(FALSE); }

	bool						AppendCtvEventRef(TCppItemCtvEventRef &evt_ref) { return(m_cib_ctv_refs.AppendItem(evt_ref)); }
								// Array of realated events is considered to be unordered.

	virtual void				AddChildItem(TCppItemBase *item, int item_destination = -1) { assert(FALSE); }
	virtual void				NewIndirectChildNotification(TCppItemBase *items_subtree_root) { assert(FALSE); }
								// These methods are overwritten only by those classes that support children. Owned children
								// should be added ONLY using either the AddChildItem() or AddListOfChildren() methods.

	void						AddListOfChildren(TList &items_to_add, int items_destination = -1);
								// Method moves elements of the list one by one. It is simple wrapper over the AddChildItem().
public:

	inline  int					NumDirectChildren() const;
								// Method returns the number of owned items.

	inline  int					NumSubtreeItems() const;
								// This method returns number of objects in the subtree including the object itself.

	inline  int					GetItemNesting() const;
								// This method should be called for published objects only. It returns the distance from this object
								// to the global namespace. Members of the global namespace have distance equal to 1.

	TCppLabelDefn			*FindNamedLabelHier(ID key_id);
	TCppCaseLabelDefn		*FindCaseLabelHier(TCppCaseLabelDefn *case_label);
								// The named labels searcher is searching the whole subtree. The case label searcher ignores
								// case labels that stay inside the nested SWITCH statements.

	inline  TCppDefnBase		*GetNamespaceParent() const;

	static bool				IsTemplateParam(TCppItemBase *item);
	static bool				IsTemplateDefinitionArgsList(TList *params_list, TList *args_list);
	static bool				CompareTemplateParams(TCppItemBase *param1, TCppItemBase *param2);
	static bool				CompareTemplateParamLists(TList *params_list1, TList *params_list2);
								// Comparison of template params is placed into the core object because class templates and function
								// templates reside in different branches of the classes inheritance tree.

	static bool				CompareSameDatabaseDataTypes(TCppDataTypeBase *type1, TCppDataTypeBase *type2);
	static bool				CompareSameDatabaseFunctionTypes(TCppFunctionTypeBase *type1, TCppFunctionTypeBase *type2, bool do_full_compare = TRUE);
								// These methods assume that enums, structs (and derived types), type params, etc are identical
								// once they have the same value of the item Id.

	static const wchar_t		*GetItemStructName(TCppItemType item_type);

protected:

	static bool				CompareTypesInternal(TCppDefnBase *type1, TCppCvPtrRefSpec *unapplied_type1_layers, int num_unapplied_type1_layers,
													TCppDefnBase *type2, TCppCvPtrRefSpec *unapplied_type2_layers, int num_unapplied_type2_layers);

	virtual void				PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void				PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void				ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
								// Default implementation corresponds to the case when derived object cannot have children.

	static TCppDefnBase		*GetFirstTemplateParameter(const TList *members_list);
	static TCppDefnBase		*GetNextTemplateParameter(const TList *members_list, TCppDefnBase *param);

protected:

	ID							m_cib_id;					// This is an identifier of the object in the Cpp database. This id has nothing
															// to do with ids of the objects in the mid scan layer storage.
	TCppItemType				m_cib_type;					// Type of the object. This field may be converted into virtual method in future.

	TCppItemBase				*m_cib_parent;				// All objects, registered in the Cpp database except for the global namespace
															// object itself have non NULL parent object.
	TCtvEventRefsArray			m_cib_ctv_refs;				// List of the MidLev database parsing events that are related to the object.
															// In the typical case this will be a definition event and the related errors.
private:

	friend class TCppDirectChildrenIter;
	friend class TCppChildrenSubtreeIter;
	friend class TCppDatabaseXmlHelper;

	friend class TCppDatabase;
	friend class TCppParser;

	//
	//  All classes and structures below are public derivatives of the current class. Nevertheless it is still
	//  necessary to declare them as friends here because they modify data fields of other TCppItemBase
	//  derivatives that are passed to their methods as parameters.
	//

	friend class TCppDefnBase;

	friend struct TCppBitFieldTypeDefn;
	friend struct TCppArrayTypeDefn;
	friend struct TCppEnumTypeDefn;
	friend struct TCppEnumMemberDefn;
	friend struct TCppStructTypeDefn;
	friend struct TCppFieldAlignSpec;
	friend struct TCppDataFieldDefn;
	friend struct TCppDataFieldInitializer;
	friend struct TCppFunctionTypeDefn;
	friend struct TCppNamespaceDefn;
	friend struct TCppGlobalNamespaceDefn;

	friend struct TCppClassTemplateSpecialization;
	friend struct TCppClassTemplateInstantiation;
	friend struct TCppFunctionTemplateSpecialization;
	friend struct TCppFunctionTemplateInstantiation;
	friend struct TCppTemplateTemplateParamDefn;

	friend struct TCppUnresolvedNameInfo;
	friend struct TCppUnresolvedDataType;
	friend struct TCppUnresolvedStructType;
	friend struct TCppUnresolvedDataField;
	friend struct TCppUnresolvedCodeEntryPoint;
	friend struct TCppUnresolvedClassTemplate;
	friend struct TCppUnresolvedFunctionTemplate;

	friend struct TCppIfStatement;
	friend struct TCppSwitchStatement;
	friend struct TCppForStatement;
	friend struct TCppWhileStatement;
	friend struct TCppDoWhileStatement;
	friend struct TCppReturnStatement;
	friend struct TCppLightBlockDefn;
	friend struct TCppCaseLabelDefn;

	friend struct TCppUnaryExpr;
	friend struct TCppBinaryExpr;
	friend struct TCppMultiOpExpr;
	friend struct TCppAssignmentExpr;

	friend struct TCppThrowExpr;
	friend struct TCppConditionalExpr;
	friend struct TCppFieldAccessExpr;
	friend struct TCppFieldAccessPtrExpr;
	friend struct TCppArrayIndexExpr;
	friend struct TCppFunctionCallExpr;
	friend struct TCppCastExpr;
	friend struct TCppSizeofExpr;
	friend struct TCppNewExpr;
	friend struct TCppDeleteExpr;
	friend struct TCppTypeidExpr;
};

typedef THeapBasedArray<TCppItemBase*, 0x800> TCppItemPtrsArray;

//
//  This iterator iterates one layer of children of any Cpp database object. Typical use of this iterator in the app code:
//
//		for (TCppDirectChildrenIter iter(some_item); iter; ++iter)
//		{
//			TCppItemBase &item = iter.CurrItem();
//			item.SomeMethod();
//		}
//
//	This iterator works fine when "some_item" does not have children and also when it cannot have children.
//
class TCppDirectChildrenIter
{
public:

	inline	TCppDirectChildrenIter() { m_item = NULL; m_special_count = 0; m_curr_child = NULL; }
	inline	TCppDirectChildrenIter(TCppItemBase &item) { m_item = &item; item.PrepareChildrenIter(*this); }
	inline	TCppDirectChildrenIter(TCppItemBase *item) { m_item = item; item->PrepareChildrenIter(*this); }

	inline	TCppDirectChildrenIter(const TCppDirectChildrenIter &iter) { m_item = iter.m_item; m_special_count = iter.m_special_count; m_curr_child = iter.m_curr_child; }
	inline	TCppDirectChildrenIter &operator = (const TCppDirectChildrenIter &iter) { m_item = iter.m_item; m_special_count = iter.m_special_count; m_curr_child = iter.m_curr_child; return(*this); }

	inline	operator		bool() const			{ return(m_curr_child != NULL); }
	inline	bool				IsComplete() const	{ return(m_curr_child == NULL); }

	inline	TCppItemBase	&CurrItem() const		{ return(*m_curr_child); }
	inline	TCppItemBase	*CurrItemPtr() const	{ return(m_curr_child); }
								// These methods return valid reference/pointer only when iterator is not in the "end-of-iteration" state.

	inline	TCppDataFieldDefn			*CurrItemAsDataField() const { if (m_curr_child->ItemType() == cxx_data_field) return ((TCppDataFieldDefn*)m_curr_child); else return(NULL); }
	inline	TCppDataFieldDefn			*CurrItemAsDataField(TCppStorageClass strgc) const;
	inline	TCppCodeEntryPointDefn	*CurrItemAsCodeEntryPoint() const { if (m_curr_child->ItemType() == cxx_code_entry_point) return ((TCppCodeEntryPointDefn*)m_curr_child); else return(NULL); }
	inline	TCppCodeEntryPointDefn	*CurrItemAsCodeEntryPoint(bool static_method) const;
	inline	TCppOperandExpr			*CurrItemAsOperand() const { if (m_curr_child->ItemType() == cxx_operand_expr) return ((TCppOperandExpr*)m_curr_child); else return(NULL); }
	inline	TCppOperandExpr			*CurrItemAsOperand(TCppExprOperandType oprndt) const;
										// Methods above return a non NULL pointer if the current item has the expected type.
										// Otherwise they return NULL.

	inline	void		operator ++()		{ m_item->ShiftChildrenIter(*this); }
	inline	void		ShiftDown()		{ m_item->ShiftChildrenIter(*this); }

	inline	void		SeekToFirst()						{ m_item->PrepareChildrenIter(*this); }
	inline	void		SeekToItem(TCppItemBase *item)	{ m_item->PrepareChildrenIterEx(*this, item); }
						// Last method should be used with care because it assumes that the passed item is a valid direct child.
protected:

	TCppItemBase		*m_item;

	int					m_special_count;			// Meaning of the count if any depends on the actual type of the m_item field.
	TCppItemBase		*m_curr_child;

	friend class TCppItemBase;
	friend class TCppChildrenSubtreeIter;

	friend struct TCppBitFieldTypeDefn;
	friend struct TCppArrayTypeDefn;
	friend struct TCppEnumTypeDefn;
	friend struct TCppEnumMemberDefn;
	friend struct TCppStructTypeDefn;
	friend struct TCppFieldAlignSpec;
	friend struct TCppDataFieldDefn;
	friend struct TCppDataFieldInitializer;
	friend struct TCppFunctionTypeDefn;
	friend struct TCppNamespaceDefn;
	friend struct TCppGlobalNamespaceDefn;

	friend struct TCppClassTemplateSpecialization;
	friend struct TCppClassTemplateInstantiation;
	friend struct TCppFunctionTemplateSpecialization;
	friend struct TCppFunctionTemplateInstantiation;
	friend struct TCppTemplateTemplateParamDefn;

	friend struct TCppUnresolvedNameInfo;
	friend struct TCppUnresolvedDataType;
	friend struct TCppUnresolvedStructType;
	friend struct TCppUnresolvedDataField;
	friend struct TCppUnresolvedCodeEntryPoint;
	friend struct TCppUnresolvedClassTemplate;
	friend struct TCppUnresolvedFunctionTemplate;

	friend struct TCppIfStatement;
	friend struct TCppSwitchStatement;
	friend struct TCppForStatement;
	friend struct TCppWhileStatement;
	friend struct TCppDoWhileStatement;
	friend struct TCppReturnStatement;
	friend struct TCppBlockDefn;
	friend struct TCppLightBlockDefn;
	friend struct TCppCaseLabelDefn;

	friend struct TCppUnaryExpr;
	friend struct TCppBinaryExpr;
	friend struct TCppMultiOpExpr;
	friend struct TCppAssignmentExpr;

	friend struct TCppThrowExpr;
	friend struct TCppConditionalExpr;
	friend struct TCppFieldAccessExpr;
	friend struct TCppFieldAccessPtrExpr;
	friend struct TCppArrayIndexExpr;
	friend struct TCppFunctionCallExpr;
	friend struct TCppCastExpr;
	friend struct TCppSizeofExpr;
	friend struct TCppNewExpr;
	friend struct TCppDeleteExpr;
	friend struct TCppTypeidExpr;
};

//
//  This iterator iterates all direct and indirect children of any Cpp database object. The root of the subtree
//  is NOT included into the iteration. Typical use of this iterator in the app code looks like:
//
//		for (TCppChildrenSubtreeIter iter(some_item); iter; ++iter)
//		{
//			TCppItemBase item = iter.CurrItem();
//			item.SomeMethod();
//		}
//
//	This iterator works fine when "some_item" does not have children and also when it cannot have children.
//
class TCppChildrenSubtreeIter
{
public:

	inline	TCppChildrenSubtreeIter(TCppItemBase &subtree_root_item) { m_subtree_root = &subtree_root_item; m_curr_layer_iter.m_item = &subtree_root_item; subtree_root_item.PrepareChildrenIter(m_curr_layer_iter); m_block_children = FALSE; }
	inline	TCppChildrenSubtreeIter(TCppItemBase *subtree_root_item) { m_subtree_root = subtree_root_item; m_curr_layer_iter.m_item = subtree_root_item; subtree_root_item->PrepareChildrenIter(m_curr_layer_iter); m_block_children = FALSE; }

	inline	operator		bool() const			{ return(m_curr_layer_iter.m_curr_child != NULL || m_curr_layer_iter.m_item != m_subtree_root); }
	inline	bool				IsComplete() const	{ return(m_curr_layer_iter.m_curr_child == NULL && m_curr_layer_iter.m_item == m_subtree_root); }

	inline	TCppItemBase	&CurrItem() const		{ return(m_curr_layer_iter.CurrItem()); }
	inline	TCppItemBase	*CurrItemPtr() const	{ return(m_curr_layer_iter.CurrItemPtr()); }
								// These methods return valid reference/pointer only when iterator is not in the "end-of-iteration" state.

	inline	TCppDataFieldDefn		*CurrItemAsDataField() const { return (m_curr_layer_iter.CurrItemAsDataField()); }
	inline	TCppDataFieldDefn		*CurrItemAsDataField(TCppStorageClass strgc) const { return (m_curr_layer_iter.CurrItemAsDataField(strgc)); }
	inline	TCppOperandExpr		*CurrItemAsOperand() const  { return (m_curr_layer_iter.CurrItemAsOperand()); }
	inline	TCppOperandExpr		*CurrItemAsOperand(TCppExprOperandType oprndt) const { return (m_curr_layer_iter.CurrItemAsOperand(oprndt)); }
									// Methods above return a non NULL pointer if the current item has the expected type. Otherwise
									// they return NULL.

	inline	void		operator ++();
	inline	void		ShiftDown()	{ this->operator ++(); }
	inline	void		SeekToFirst()	{ m_curr_layer_iter.m_item = m_subtree_root; m_subtree_root->PrepareChildrenIter(m_curr_layer_iter); m_block_children = FALSE; }
	inline	void		BlockCurrItemChildrenIteration() { m_block_children = TRUE; }

protected:

	TCppItemBase			*m_subtree_root;

	TCppDirectChildrenIter		m_curr_layer_iter;

	bool						m_block_children;			// Once this flag is set, it blocks iterating the children of the current item if any.
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline bool TCppItemBase::IsPublished() const
{
	// Look for the global namespace among the parents of the instance.
	TCppItemBase *item = m_cib_parent;
	while (item != NULL)
	{
		if (item->m_cib_type == cxx_global_namespace)
			return(TRUE);
		item = item->m_cib_parent;
	}
	return(FALSE);
}

inline bool TCppItemBase::IsParentOf(TCppItemBase *child_item) const
{
	if (child_item == NULL)
		return(FALSE);

	// Start iteration from the parent of the passed child to avoid comparison with the passed child item.
	// This is necessary when "this" is passed as the parameter. The object is not a parent of itself.
	TCppItemBase *item = child_item->m_cib_parent;
	while (item != NULL)
	{
		if (item == this)
			return(TRUE);
		item = item->m_cib_parent;
	}

	return(FALSE);
}

inline bool TCppItemBase::IsChildOf(TCppItemBase *parent_item) const
{
	if (parent_item == NULL)
		return(FALSE);

	// Start iteration from the parent item to avoid false detection of the "this" object as the child of itself.
	TCppItemBase *item = m_cib_parent;
	while (item != NULL)
	{
		if (item == parent_item)
			return(TRUE);
		item = item->m_cib_parent;
	}

	return(FALSE);
}

inline bool TCppItemBase::IsTemplateInstantiation() const
{
	TCppItemBase *item = (TCppItemBase*)this;
	while (item != NULL)
	{
		if (item->m_cib_type == cxx_class_template_inst || item->m_cib_type == cxx_func_template_inst)
			return(TRUE);
		item = item->m_cib_parent;
	}
	return(FALSE);
}

inline int TCppItemBase::NumDirectChildren() const
{
	int cnt = 0;
	for (TCppDirectChildrenIter iter((TCppItemBase*)this); iter; ++iter)
		cnt ++;
	return(cnt);
}

inline int TCppItemBase::NumSubtreeItems() const
{
	int cnt = 1;
	for (TCppDirectChildrenIter iter((TCppItemBase*)this); iter; ++iter)
		cnt += iter.CurrItem().NumSubtreeItems();
	return(cnt);
}

inline int TCppItemBase::GetItemNesting() const
{
	int cnt = 0;
	const TCppItemBase *item = this;
	while (item != NULL)
	{
		if (item->m_cib_type == cxx_global_namespace)
			break;
		cnt++;
		item = item->m_cib_parent;
	}
	return(cnt);
}

inline TCppDefnBase *TCppItemBase::GetNamespaceParent() const
{
	TCppItemBase *candidate = m_cib_parent;
	while (candidate != NULL)
	{
		if (candidate->IsNamespace() == TRUE)
			return((TCppDefnBase*)candidate);
		candidate = candidate->m_cib_parent;
	}

	// The object is not published in the database or this is a data corruption.
	return(NULL);
}

inline void TCppItemBase::PrepareChildrenIter(TCppDirectChildrenIter &iter) const
{
	assert(iter.m_item == this);

	// Do inititialization for classes that cannot have children.
	iter.m_special_count = 0;
	iter.m_curr_child = NULL;
}

inline void TCppItemBase::PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const
{
	assert(iter.m_item == this);

	// Do inititialization for classes that cannot have children.
	iter.m_special_count = 0;
	iter.m_curr_child = NULL;
}

inline void TCppItemBase::ShiftChildrenIter(TCppDirectChildrenIter &iter) const
{
	assert(iter.m_item == this);

	// Do shift for classes that cannot have children.
	iter.m_special_count = 0;
	iter.m_curr_child = NULL;
}

//
//  Statement object can be part of the executable code.
//
class TCppStatementBase : public TCppItemBase
{
public:

	TCppStatementBase(TCppItemType cxt, ID id = 0) : TCppItemBase(cxt, id) { }

	virtual bool		IsStatementBase() const { return(TRUE); }
};

//
//  Defn proxy is an internal class that is used for building hash tables of names. It should not be used directly
//  in any analysis code. This class should be used only for the first data member of the TCppDefnBase class.
//
class TCppDefnProxy : public TListItem
{
public:

	inline TCppDefnBase	*GetDefn() { return((TCppDefnBase*)(((TCppItemBase*)this)-1)); }
};

enum TCppDefnProblem
{
	cdpr_none,

	// Existing definitions lookup.
	cdpr_ambig,
	cdpr_unapprop,
	cdpr_unres,

	// New definition name clearance.
	cdpr_errloc,						// It is not allowed to create defns of this type on the destination layer.
	cdpr_dupe,						// Identical definition is already existing.
	cdpr_conflict,					// Object of a different type with this name is already existing.

	cdpr_num_types,
};

//
//  This enum suports specific C/C++ feature that allows defining "enum Blah" and "union Blah" in the same
//  context. Such definitions are not creating the names collision. When it is necessary to define variables of these
//  types it is necessary to add specifier "enum" or specifier "union" to avoid the ambiguity.
//
enum TCppElabType
{
	elab_none,
	elab_enum,
	elab_struct,
	elab_union,
	elab_class,
	elab_num_types,
};

//
//  This enumerator has two slightly different meanings. One meaning is straightforward. It describes the storage
//  class specifier when it is retrieved from the source code. Effectively these are the first 6 members of this enum.
//  Other meaning appears when the m_strg_class data field of the TCppDataFieldDefn is interpreted. In this case
//  the meaning of the field depends on the type of the owning container (namespace/class/block). Look for details
//  in the comments inside the TCppDataFieldDefn class.
//
enum TCppStorageClass
{
	strgc_none,						// This storage class is used for non static (instance) data members in classes.
	strgc_register,
	strgc_auto,
	strgc_static,
	strgc_extern,
	strgc_mutable,					// This is not a storage class but rather an optional attr that is applicable only to the non static
									// data members of the classes. This attribute allows changing this data member from the class
									// member functions, that are declared with trailing const.
	strgc_param,					// Param of the function. This value has no corresponding keyword in the C/C++ grammar.
	strgc_template_param,			// Non-type template parameter. This value has no corresp keyword in the C/C++ grammar.
	strgc_catch_block_param,		// Param from the catch block header. There is no corresp keyword in the C/C++ grammar.
	strgc_num_types,
};

enum TCppSpecFuncType
{
	spcft_none,						// Ordinary function that has name, type of return value, etc.
	spcft_ctor,						// Constructor.
	spcft_dector,					// Destructor.
	spcft_operator,					// Overloaded operator.
	spcft_typecast,					// Overloaded conversion.
	spcft_num_types,
};

//
//  Base class for C/C++ language definitions.
//
//  This class is based on the statement class because definitions, that are located inside the blocks,
//  are statements. They can be labeled and executed. On the contrary, definitions in the namespaces
//  and classes cannot have labels.
//
class TCppDefnBase : public TCppStatementBase
{
public:

		TCppDefnBase(TCppItemType cxt, ID id = 0)
				: TCppStatementBase(cxt, id) { m_defn_problem = cdpr_none; m_defn_elab_type = elab_none; m_defn_name = NULL; m_defn_key_id = 0; m_defn_cnt_use = 0; }

	virtual	bool				IsDefinition() const { return(TRUE); }

	inline	TCppDefnProblem		DefnProblem() const { return(m_defn_problem); }
	inline	TCppElabType			DefnElabType() const { return(m_defn_elab_type); }

	inline	const wchar_t			*DefnName() const { return(m_defn_name); }				// Defn name can be NULL.
	inline	ID					DefnKeyId() const { return(m_defn_key_id); }				// Defn key id can be zero.

	virtual	const wchar_t			*BasicItemName() const { return((m_defn_name != NULL && m_defn_name[0] != 0) ? m_defn_name : TCppItemBase::BasicItemName()); }
									// Return the real name of the definition if possible.

	void						SetNameInfo(const wchar_t *name, ID key_id, TCppDefnProblem cdpr) { m_defn_name = (wchar_t*)name; m_defn_key_id = key_id; m_defn_problem = cdpr; }
								// Note that NULL name and key_id equal to zero are legal values that are used in the code.
								// Ownership on the name string is not passed.

	const	wchar_t			*GetElabTypeName(bool want_first_cap = FALSE, bool plural_case = FALSE) const { return(GetElabTypeName(m_defn_elab_type, want_first_cap, plural_case)); }
	const	wchar_t			*GetObjectTypeFriendlyName(wchar_t *buff80, bool want_first_cap = FALSE) const { return(GetObjectTypeFriendlyName(buff80, m_cib_type, m_defn_elab_type, want_first_cap)); }

	static	const  wchar_t		*GetElabTypeName(TCppElabType elab, bool want_first_cap = FALSE, bool plural_case = FALSE);
								// Return value will be one of: "none", "enum", "struct", "union", "class", possibly with the first capital letter.

	static	const wchar_t		*GetObjectTypeFriendlyName(wchar_t *buff80, TCppItemType item_type, TCppElabType elab, bool want_first_cap = FALSE);
								// Method generates human readable names of the type for enums, classes and class templates that are good
								// for using them in error messages, like "struct", "class template", "union template instantiation", etc.

	static	const  wchar_t		*GetDefnProblemEnumName(TCppDefnProblem cdpr);

	virtual	TList				*GetNestedMembersList() const { return(NULL); }
	virtual	TList				*GetUnresolvedNamesList() const { return(NULL); }
								// Note that there is no assert inside these methods. This means that they can be called on any object.

protected:

	virtual	TCppDefnsSpace	*GetNestedDefnsSpace() { assert(FALSE); return(NULL); }
								// This method should be used only with those derived classes, that support nested defns spaces.
								// Retrieveing the nested space is needed for calling various query methods.

	virtual	TList				*GetAssocDefnsSpaceRefsList() { assert(FALSE); return(NULL); }
								// Associated definition spaces are spaces of definitions that are visible inside the current container
								// besides the local definitions. Examples are base classes and "USE NAMESPACE" directives.

	virtual	void				AddToNestedDefnsSpace(TCppDefnBase *defn_item) { assert(FALSE); }
	virtual void				AddToNestedDefnsSpaceSpecial(TCppDefnBase *defn_item) { assert(FALSE); }
								// These methods should be used when it is necessary to add object to the local defns space of the definition
								// that is not a child of the current object (typically the child of the child) or when the object will be added
								// as a child later. Second param does not tell anything about the nature of the name of the passed
								// definition. The value of this param can be FALSE only for CvPtrRefType, ArrayType and FunctionType
								// objects. It instructs to put these objects into special buckets once it is specified as FALSE.

	void		AddToMembersListAndDefnsSpace(TCppItemBase *item, TList &members_list, TCppDefnsSpace &defns_space);
				// This is a protected worker method that is expected to be used only in the namespace, struct type
				// and block classes. Similar functionality for the global namespace class is implemented directly in
				// the TCppGlobalNamespaceDefn::AddChildItem() because this method cannot be used in the global
				// namespace class. This method adds defns only to the named buckets of the defns space.

	inline bool CheckContextSpecial(TCppDefnBase *context) const
	{
		TCppItemBase *item = context;
		while (item != NULL && item->IsNamespace() == FALSE)
		{
			if (item == this)
				return(TRUE);
			item = item->m_cib_parent;
		}

		// Current item does not belong to the non namespace part of the passed context.
		return(FALSE);
	}

public:

	int		GetNumEnumTypes(TCppDataTypeBase *base_type = NULL) const;
				// Passing non NULL base type will result in the number of enums with equivalent (not identical)
				// base type.

	int		GetNumStructTypes(TCppElabType elab_type_only = elab_none) const;
				// Passing elab_none will result in getting the number of all stuct types.

	int		GetNumDataFields(TCppStorageClass strg_class_only = strgc_num_types) const;
				// Passing the value strgc_none will result in the number of data fields with this storage type.
				// The storage type "none" is used for the instance data fields of structs/unions/classes.
				// For getting total number of all data fields, it is necessary to pass strgc_num_types.

	int		GetNumCodeEntryPoints(TCppSpecFuncType spcft_only = spcft_num_types) const;
				// Passing default value will result in getting the total number of all types of functions.

	TCppDefnBase	*FindNamedDirectChildDefn(const wchar_t *object_name);
						// For simplicity this method allows to search only for unambiguous, unoverloaded, etc objects.

private:

	TCppDefnProxy				m_defn_proxy;					// This proxy is used to link definitions into the defns space.
																// IMPORTANT: This field should be the first data field in this class.
protected:

	TCppDefnProblem				m_defn_problem;				// This field can be not empty only when there was some problem with this name
																// during parsing like an unresolved layer, names conflict, etc.
	TCppElabType					m_defn_elab_type;				// Elaboration type is tightly linked to the type of the object. For example elab type
																// enum can be present only in the TCppEnumTypeDefn instances.

	wchar_t						*m_defn_name;					// Name of the properly inited object cannot be NULL.

	ID							m_defn_key_id;					// When the name is empty (NULL), the key id field (key id of the name) should be
																// zero. Key id is not zero for all names that were taken directly from the source
																// code. In several other cases like names collision, nameless definitions, etc.,
																// the value of the key id is zero.
	int							m_defn_cnt_use;

	friend class TCppDefnsSpace;
	friend class TCppNewDefnsSearchHelper;
	friend class TCppExistingDefnsSearchHelper;
	friend struct TCppGlobalNamespaceDefn;
	friend class TCppDatabase;
	friend struct TCppNameInfo;
	friend class TCppParser;
	friend class TLangViewCppItemListBoxItem;
};

typedef TStructsArray<TCppDefnBase*, 8, 64> TCppDefnBasePtrsArray;

class TCppDataTypeBase : public TCppDefnBase
{
public:

	TCppDataTypeBase(TCppItemType cxt, ID id = 0) : TCppDefnBase(cxt, id) { }

	virtual bool		IsDataType(TCppDefnBase *context = NULL) const { return(TRUE); }

	static TCppDataTypeBase	*GetUnderlyingNumericType(TCppDataTypeBase *data_type, bool template_conext);
								//
								//	The return value is NULL when the passed data type cannot be converted into the numeric type.
								//	Otherwise the return value can be:
								//
								//		 --	BuiltInType (bltin_type_bool, bltin_type_char, bltin_type_int  subtypes only).
								//		 --	EnumType (only with NULL base type).
								//		 --	TemplateTypeParam, UnresolvedDataType, UnresolvedStructType (in the tempate context only).
								//
};

class TCppFunctionTypeBase : public TCppDefnBase
{
public:

	TCppFunctionTypeBase(TCppItemType cxt, ID id = 0) : TCppDefnBase(cxt, id) { }

	virtual bool		IsFunctionType(TCppDefnBase *context = NULL) const { return(TRUE); }
};

class TCppExpressionBase : public TCppStatementBase
{
public:

	TCppExpressionBase(TCppItemType cxt, ID id = 0) : TCppStatementBase(cxt, id) { m_operation = opr_none; m_lvalue_expr = m_const_expr = FALSE; m_result_type = NULL; }

	virtual bool		IsExpression() const { return(TRUE); }
	inline	bool		IsOperandExpr() const { return(m_cib_type == cxx_operand_expr); }

	const wchar_t		*OperationEnumName() const;

	virtual bool		CanHaveChildren() const { return(TRUE); }
						// Out of 16 types of expresssions 15 types can have children. Only the operand expression cannot
						// have children. This is why this method is overwritten in the TCppOperandExpr class one again.

	virtual bool		GetConstOperandValue(TCppOperandExpr &expr_value) const { return(FALSE); }
						// Return value of thhis method is FALSE when expression is not const or, in other words, when some
						// operation of the expression tree does not allow static computation, like the call of the function.
						// Note that this method is slightly wider than the m_const_expr flag. It allows to retrieve const oprnd
						// values in some cases when this flag is not set.

	static TCppBuiltInTypeIdent		GetBuiltInTypeIdFromNumberSubtype(TLexNumberType subt);
	static TCppBuiltInTypeIdent		GetBuiltInTypeIdFromCharconstSubtype(TLexCharConstType subt);
	static TCppBuiltInTypeIdent		GetBuiltInTypeIdFromStringSubtype(TLexStringType subt);

	static TLexNumberType			GetNumericOperationResultType(TLexNumberType subt1, TLexNumberType subt2);
	static TLexNumberType			GetNumericSubtypeFromValue(__int64 value);

public:

	TOperatorToken				m_operation;
									// Majort part of the values of TOperatorToken enum can be present in this field. Nevertheless
									// there are some exceptions. When the value of the field is opr_none, this means that object
									// represents an operand.

	bool							m_lvalue_expr;
									// The value of the field is TRUE when current expression can be used in the LEFT hand side of
									// the assignment expression.

	bool							m_const_expr;
									// The value of the field is TRUE when expression can be computed at the time of compilation.

	TCppDefnBase				*m_result_type;
									// The type of the expression result is always known because the code is comuted statically.
									// In typical case expression is a data manipilation expression. In this situation this field contains
									// the data type of the expression result. Expression can also be the code entry point expression.
	friend class  TCppParser;
};

struct TCppCvPtrRefSpec
{
	union
	{
		struct
		{
			bool			cvpr_const;
			bool			cvpr_volatile;

			bool			cvpr_pointer;
			bool			cvpr_reference;
		};									// Note that when the pointer and const flags are set at the same time, this means "int * const var",
											// not the "int const * var". Second case should be expressed using two separate indirection layers.

		DWORD			cvpr_flags;
	};

	TCppStructTypeDefn	*cvpr_struct;		// Pointer in this field is not NULL only when the structure describes a pointer/reference to
											// the member of the class/struct/union. Ordinary pointers and refs have NULL in this field.
		//
		//	Notes on the const/volatile and ptr/ref fields:
		//
		//		const int *p1;		  p1++;		<-- Ok.
		//						*p1 = 2;		<-- Error.
		//
		//	Const keyword on the side of the pointed data makes the data const. The pointer itself can be changed freely.
		//
		//		int *const p2;		  p2++;		<-- Error.
		//						*p2 = 4;		<-- Ok.
		//
		//	Const keyword on the side of the pointer makes the pointer const. The pointed data can be changed freely.
		//

	inline void		Clear() { memset(this, 0, sizeof(TCppCvPtrRefSpec)); }
	inline bool		IsEmpty() const { return(cvpr_flags == 0); }
	inline bool		IsSimplePointer() const { return(cvpr_const == FALSE && cvpr_volatile == FALSE && cvpr_pointer == TRUE && cvpr_struct == NULL); }
	inline bool		Compare(TCppCvPtrRefSpec &inst);

	inline void		SetPointer(TCppStructTypeDefn *ptr_strust = NULL) { cvpr_flags = 0; cvpr_pointer = TRUE; cvpr_struct = ptr_strust; }
	inline void		SetReference(TCppStructTypeDefn *ref_strust = NULL) { cvpr_flags = 0; cvpr_reference = TRUE; cvpr_struct = ref_strust; }
};

struct TCppArrayDimensionSpec
{
	__int64					m_num_elements;				// In the normal situation the number of elements should be either positive or zero.
															// When the value of this field is -1, this means that expression that was defining
															// the size of array was not const or it was not numeric or its value was negative.
															// MS compiler does not allow allocating variable sized arrays as local variables.
															//
															// The following situations are possible:
															//
															//		> 0		--	The size was given using a valid const exprsession
															//					with a positive numeric value.
															//		== 0	--	Const expr with zero value or the length was
															//					specified using an empty square brackets.
															//		 -1		--	Size was specified using a non const expression
															//					that has a type that is castable to numeric.
															//		 -2		--	Expression contains syntax error, it has bogus
															//					type or its numeric value is negative.

	TCppExpressionBase		*m_num_elements_expr;			// Expression that was used to specify the size. When an array definition is used in
															// the template context and the expression is not constant the error mesasge is not
															// generated. Note that this pointer describes an owned child.
};

typedef TStructsArray<TCppCvPtrRefSpec, 4, 8> TCppCvPtrRefSpecsArray;
typedef TStructsArray<TCppArrayDimensionSpec, 4, 8> TCppArrayDimensionsArray;

struct TCppFunctionModifierSpec
{
	TCppCvPtrRefSpec			m_this_cv_spec;					// Only const and volatile flags are allowed in this field. These flags can
															// be present only in the non static member functions. Essentially they are
															// modifiers of the implicit first parameter "this".
	PrEventHeader			m_this_cv_spec_context;

	TCppItemBase			*m_params_beg;				// Pointer to the first item of the params list or NULL.
	TCppItemBase			*m_params_end;				// Pointer to the last item of the params list or NULL.
								//
								// When the list of params is empty, both pointers are NULLs. This list can contain the function
								// parameters and the possible exception types.
								//
								// The data structure with these two pointers is rather specific. It is very similar to the regular
								// double linked list. This pair of pointers allows moving the structure in memory that the regular
								// list does not allow. On the other hand, it is not heeded to add/remove items to the list one by
								// one. This list is always created outside and it is given out as a whole. That is it.
								//

	int						GetNumParams() const;

	TCppDataFieldDefn			*GetFirstParam() const;
	TCppDataFieldDefn			*GetNextParam(TCppDataFieldDefn *param) const;

	void		SetupFunctionTypeObject(TCppFunctionTypeDefn *func_type_defn);
				// Method moves resources of the struct into the passed Cpp item.
};

enum TCppDeclaratorModifierType
{
	dclmdf_cvpr,
	dclmdf_array,
	dclmdf_function,
};

//
//  C/C++ language allows three types of declarator modifiers:
//
//	  --	Pointer/reference to something. This modifier can also have a const/volatile property;
//	  --	Array of something. This can be either a singe dimensional or a multi dimensional array;
//	  --	Function returning something. Function has a list of parameters and can also have a list of
//		exception specs and/or const/volatile specifier for 'this' param (instance methods only).
//
//  This structure describes one of these 3 possible modifiers. For certain extent this structure is more part
//  of the Cpp parser rather a cpp classes library. Nevertheless defining this struct here simplifies the code.
//
struct TCppDeclaratorModifier
{
	TCppDeclaratorModifierType			m_dmt;

	union
	{
		TCppCvPtrRefSpec				m_cvpr_spec;					// This field contains all props of the non terminal PtrOperator.
		TCppArrayDimensionSpec		m_array_spec;
		TCppFunctionModifierSpec		m_func_spec;
	};

	PrEventHeader					m_modifier_context;
										// This context is needed only for issuing syntax errors while assembling the declarator.

	inline void	Clear() { memset(this, 0, sizeof(TCppDeclaratorModifier)); }
	inline void	SetCvprSpec(TCppCvPtrRefSpec &spec) { m_dmt = dclmdf_cvpr; m_cvpr_spec = spec; m_modifier_context.Clear(); }
	inline void	SetPointerSpec(bool set_const_ptr = FALSE) { m_dmt = dclmdf_cvpr; m_cvpr_spec.SetPointer(); m_cvpr_spec.cvpr_const = set_const_ptr; m_modifier_context.Clear(); }
	inline void	SetArrayDimSpec(__int64 dim_size, TCppExpressionBase *expr = NULL) { m_dmt = dclmdf_array; m_array_spec.m_num_elements = dim_size; m_array_spec.m_num_elements_expr = expr; m_modifier_context.Clear(); }
	inline void	SetEmpltyFuncSpec() { m_dmt = dclmdf_function; m_func_spec.m_this_cv_spec.Clear(); m_func_spec.m_params_beg = m_func_spec.m_params_end = NULL; m_modifier_context.Clear(); }

	void		ReleaseResources();
};

typedef TStructsArray<TCppDeclaratorModifier, 16, 32> TCppDeclaratorModifiersArray;
			// Elements in this array are ordered in the order of modifiers discovery when the declarator
			// is being scanned. In other words they are ordered "from the variable to the base type".
			// Assembling the declarator type is going from high indexes to the low indexes.

//
// This struct has very limited use. It should be used only as a data member in the base class spec and
// in the using namespace directive.
//
struct TCppAssocDefnsSpaceRef : public TListItem
{
	TCppDefnBase					*m_assoc_space_ref_object;
										// This poniter points to an owning object where this object resides as a data member.
										// For now this can be only either a BaseClassSpec or a UsingDirectiveDefn.
};

//
// Defns bucket class is used both for named and special buckets in the spaces of definitions.
// Members of this list can be only instances of TCppDefnProxy class.
//
class TCppDefnsBucket : public TList
{
public:

	inline int	GetNumObjects(TCppItemType item_type, TCppElabType elab_type, ID key_id) const;
};

//
//  Defns space is an interface to the hash table of various definitions. This hash table stores only refrences
//  to named and unnamed (special) definitions. Named definitions are always searched using the key_id
//  of their names. It is not possible to search for name using its string representation. Unnamed definitions
//  have various different search keys depending on its nature.
//
//  Named definitions have 5 elaboration buckets (these buckets are virtual buckets, not real buckets):
//
//	    Unelaborated bucket, ENUM bucket, STRUCT bucket, UNION bucket, CLASS bucket.
//
//  Each elaboration bucket can contain zero, one definition or group of overloaded definitions that use
//  the same name.
//
//  Named overloading can happen only inside the following groups:
//
//	    Grp1: BaseClassSpec	This is rare case when base classes have the same short name and
//							they come from different classes/namespaces.
//	    Grp2: ClassTemplate, ClassTemplateSpecialization, ClassTemplateInstantiation.
//							Class templates allow overloading.
//	    Grp3: CodeEntryPoint, FuncTemplate, FuncTemplateSpecialization, FuncTemplateInstantiation.
//							These are simple named functions or constructors.
//
//  Unnamed overloading may happen only inside the following groups:
//
//	    Grp1: FunctionType		Rare case of unnamed function type. Primarily this happens when pointer
//							to function is defined inside the list of function parameters.
//	    Grp2: CodeEntryPoint, FuncTemplate, FuncTemplateSpecialization, FuncTemplateInstantiation.
//							These are destructors, overloaded operators and overloaded conversions.
//	    Grp3: UnresolvedDataType, UnresolvedStructType, UnresolvedDataField, UnresolvedCodeEntryPoint,
//							UnresolvedClassTemplate, UnresFuncTemplate.
//
//  This class is using overloading terminology for BaseClassSpecs and UnresolvedNames. Although from
//  the high level stand point this is more an ambiguity rather than overloading. Nevertheless for now the code
//  is written this way.
//
class TCppDefnsSpace
{
public:

	void						Clear();
								//
								//   Remove all definitions from the space.
								//

	void						AddRegularDefinition(TCppDefnBase *defn);
								//
								//   This method adds passed definition to the defns space. Note that upper layer is responsible for
								//   maintaining the inambiguity of the space. The following different cases are possible:
								//
								//		--	Regular named definition;
								//		--	Special function like destructor, ovld operator and ovld conversion;
								//		--	Nameless object (nameless namespace, enum, class, param, etc);
								//		--	Definition that is created from the name collision.
								//
								//   In the last 2 cases the definition has its key_id equal to zero and it is not added to the space.
								//

	void						AddSpecialDefinition(TCppDefnBase *defn);
								//
								//   Passed definition is added only to the special buckets. Definition can belong only to the following
								//   groups:
								//				--	Derived data type (BitFieldType, CvPtrRefType, ArrayType);
								//				--	Unnamed function type;
								//				--	Unresolved object.
								//
								//   Passed definition should have its key_id equal to zero and these definitions can be only searched
								//   usion one of the lookup methos of this class.
								//

	TCppBitFieldTypeDefn		*LookupBitFieldType(ID base_type_id, short bit_length);
	TCppCvPtrRefTypeDefn		*LookupUnnamedCvPtrRefType(ID base_type_id, TCppDeclaratorModifier *layers, int num_layers);
	TCppArrayTypeDefn		*LookupUnnamedArrayType(ID base_type_id, TCppDeclaratorModifier *dimensions, int num_dimensions);
								//
								// Data types group. The BitFieldType can be only unnamed.
								//

	TCppFunctionTypeDefn		*LookupUnnamedFunctionType(ID ret_value_type_id, TCppDeclaratorModifier *func_mdf);
								//
								// Unnamed function type is used for decsribing constructs like:
								//
								//	void f(int (*pfunc)());
								//
								// Here pfunc is the name of the parameter. The type of this parameter does not have any name at all.
								//

	TCppDefnBase			*LookupUnresolvedObject(TCppItemType item_type, TCppUnresolvedNameInfo *unres_name_info, TCppDefnProblem item_problem);
	TCppDefnBase			*LookupUnresolvedObject(TCppItemType item_type, TList *unres_name_infos_seq, TCppDefnProblem item_problem);
								//
								// Misc group. The first method should be used for unqualified unresolved names and the second method
								// for a list of unresolved name infos.
								//
protected:

	void						AddToNamedBuckets(TCppDefnBase *defn);
	void						AddToSpecialBuckets(TCppDefnBase *defn);

	virtual int					NumNamedBuckets() const = 0;
	virtual int					NumSpecialBuckets() const = 0;

	virtual TCppDefnsBucket	*GetNamedBucket(int bucket_index) = 0;
	virtual TCppDefnsBucket	*GetSpecialBucket(int bucket_index) = 0;

	friend class TCppNewDefnsSearchHelper;
	friend class TCppExistingDefnsSearchHelper;
	friend class TCppParser;
};

template<int _NumNamedSlots, int _NumSpecialSlots>		// Number of slots should be an even power of 2.
class TCppDefnsTable : public TCppDefnsSpace
{
public:		//
			// This hash table is not owning its items.
			//

	TCppDefnsTable() { assert((_NumNamedSlots & (_NumNamedSlots-1)) == 0 && (_NumSpecialSlots & (_NumSpecialSlots-1)) == 0); }

protected:

	int					NumNamedBuckets() const			{ return(_NumNamedSlots); }
	int					NumSpecialBuckets() const			{ return(_NumSpecialSlots); }
	TCppDefnsBucket		*GetNamedBucket(int bucket_index)	{ return(&(m_named_slots[bucket_index & (_NumNamedSlots-1)])); }
	TCppDefnsBucket		*GetSpecialBucket(int bucket_index)	{ return(&(m_special_slots[bucket_index & (_NumSpecialSlots-1)])); }

	TCppDefnsBucket			m_named_slots[_NumNamedSlots];		// Hash buckets.
	TCppDefnsBucket			m_special_slots[_NumSpecialSlots];		// Hash buckets.
};

//
//  This iterator iterates all elements of the bucket without filtering. Note that bucket can contain olny derivatives
//  of the TCppDefnBase because this is a bucket of definitions, not an arbitrary list of cpp items.
//
class TCppDefnsSpaceBucketIterator : protected TRawListIterator
{
public:

	TCppDefnsSpaceBucketIterator(TCppDefnsBucket *bucket)
					: TRawListIterator(*bucket) { }
	TCppDefnsSpaceBucketIterator(TCppDefnsBucket *bucket, TCppDefnBase *start_from_defn)
					: TRawListIterator(*bucket, start_from_defn) { }

	inline TCppDefnBase		&Item() const	{ return(*((TCppDefnProxy&)CurrBaseItem()).GetDefn()); }
	inline TCppDefnBase		*ItemPtr() const	{ return(((TCppDefnProxy&)CurrBaseItem()).GetDefn()); }
	inline		operator	bool() const		{ return(IsComplete() == FALSE); }
	inline void	operator	++ ()			{ ShiftDown(); }
};

//
//  This is defns space bucket iterator WITH key_id FILTERING. Note that overoladed definitions are stored
//  in the bucket as separate definitions and they have the same key_id.
//
//  Note that this iterator is NOT DOING the object type filtering.
//
template <class derived_class = TCppDefnBase>
class TCppDefnsSpaceBucketKeyIdIter : protected TRawListIterator
{
public:

	TCppDefnsSpaceBucketKeyIdIter(TCppDefnsBucket *bucket, ID key_id_restr)
					: TRawListIterator(*bucket) { m_key_id_filter = key_id_restr; FilterCurrItem(); }
	TCppDefnsSpaceBucketKeyIdIter(TCppDefnsBucket *bucket, TCppDefnBase *start_from_defn, ID key_id_restr)
					: TRawListIterator(*bucket, start_from_defn) { m_key_id_filter = key_id_restr; FilterCurrItem(); }

	inline derived_class		&Item() const	{ return(*((derived_class*)(((TCppDefnProxy&)CurrBaseItem()).GetDefn()))); }
	inline derived_class		*ItemPtr() const	{ return((derived_class*)(((TCppDefnProxy&)CurrBaseItem()).GetDefn())); }
	inline		operator	bool() const		{ return(IsComplete() == FALSE); }
	inline void	operator	++ ()			{ ShiftDown(); FilterCurrItem(); }

protected:

	void		FilterCurrItem() { while (IsComplete() == FALSE && (((TCppDefnProxy&)CurrBaseItem()).GetDefn())->DefnKeyId() != m_key_id_filter) ShiftDown(); }

	ID					m_key_id_filter;
};

//
//  This is defns space bucket iterator WITH defn_type FILTERING. This iterator can be used both
//  for named and special defn buckets.
//
template <class derived_class = TCppDefnBase>
class TCppDefnsSpaceBucketDefnTypeIter : protected TRawListIterator
{
public:

	TCppDefnsSpaceBucketDefnTypeIter(TCppDefnsBucket *bucket, TCppItemType item_type_restr)
					: TRawListIterator(*bucket) { m_item_type_filter = item_type_restr; FilterCurrItem(); }
	TCppDefnsSpaceBucketDefnTypeIter(TCppDefnsBucket *bucket, TCppDefnBase *start_from_defn, TCppItemType item_type_restr)
					: TRawListIterator(*bucket, start_from_defn) { m_item_type_filter = item_type_restr; FilterCurrItem(); }

	inline derived_class		&Item() const	{ return(*((derived_class*)(((TCppDefnProxy&)CurrBaseItem()).GetDefn()))); }
	inline derived_class		*ItemPtr() const	{ return((derived_class*)(((TCppDefnProxy&)CurrBaseItem()).GetDefn())); }
	inline		operator	bool() const		{ return(IsComplete() == FALSE); }
	inline void	operator	++ ()			{ ShiftDown(); FilterCurrItem(); }

protected:

	void		FilterCurrItem() { while (IsComplete() == FALSE && (((TCppDefnProxy&)CurrBaseItem()).GetDefn())->ItemType() != m_item_type_filter) ShiftDown(); }

	TCppItemType		m_item_type_filter;
};

inline int TCppDefnsBucket::GetNumObjects(TCppItemType item_type, TCppElabType elab_type, ID key_id) const
{
	int cnt_objects = 0;
	for (TCppDefnsSpaceBucketDefnTypeIter<TCppDefnBase> iter((TCppDefnsBucket*)this, item_type); iter; ++iter)
	{
		if ((elab_type == elab_num_types || iter.Item().DefnElabType() == elab_type) && iter.Item().DefnKeyId() == key_id)
			cnt_objects++;
	}

	return(cnt_objects);
}

enum TCppNameInfoType
{
	cnit_none,

	cnit_identifier,						// Simple identifier.
	cnit_template_id,					// Identifier with template arguments.

	cnit_constructor,						// Simple constructor.
	cnit_ctor_template,					// Name of the constructor with template arguments.
	cnit_dector,							// Destructor. Destructors do not have template form.

	cnit_ovld_operator,					// Overloaded operator.
	cnit_ovld_op_template,				// Overloaded operator with template arguments.
	cnit_ovld_conversion,				// Overloaded conversion.
	cnit_ovld_conv_template,			// Overloaded conversion with template arguments.

	cnit_num_types,
};

//
// This structure is not used in the Cpp database itself. This is presentation of the name as the Cpp parser
// sees it. At the same time the TCppNewDefnsSearchHelper and TCppExistingDefnsSearchHelper classes
// can be used in any application. This is why this structure is included in this header.
//
struct TCppNameInfoCore
{
	TCppNameInfoType				m_info_type;

	ID								m_key_id;
	TOperatorToken					m_ovld_operator_token;
	TCppDataTypeBase				*m_ovld_conversion_dest;				// The pointer is NOT owned.

	void ClearSearchKeys() { m_key_id = 0; m_ovld_operator_token = opr_none; m_ovld_conversion_dest = NULL; }
};

//
//  This helper should be used before defining a new object to figure out if something with a particular name
//  is already existing or not. Desired name of the new definition is always searched only in one defns space,
//  where this definition will be placed. Contents of all other defn spaces are not important at this point.
//
//  This procedure guarantees that each defns space is not ambiguous.
//
class TCppNewDefnsSearchHelper
{
public:

		TCppNewDefnsSearchHelper(TCppDefnBase *layer_to_search) { assert(layer_to_search != NULL); m_search_container = layer_to_search; }

	bool		DoSearch(TCppElabType key_elab, TCppNameInfoCore &key_info_core);
				//
				//	 This method checks contents of only one elaboration bucket. Elaboration type elab_none is treated
				//	 in the same way as any other elaboration type.
				//
				//  Possible results of the search:
				//
				//		RetVal	  Defn	 Bucket
				//	  -------------------------------------------------------------------------------
				//		false		  NULL	  NULL		There is no information about the passed key.
				//		TRUE	   Val	  NULL		Single non overloadable definition.
				//		TRUE	   Val	   Val		Bucket with overloadable defns plus the first discovered object that belongs to the bucket.
				//									Note that discovered bucket may contain only one definition.
				//
				//  The following types of definitions can be overloaded:
				//
				//		-- Base classes;
				//		-- Class templates (plus class template specs and class template insts);
				//		-- Functions (plus func templates, func template specs and func template insts);
				//
				//  Note that the ambiguity concept is not applicable to this searcher.
				//

	bool		IsSmthDiscovered() const { return(m_single_defn != NULL); }
	bool		IsSingleDefnResult() const { return(m_single_defn != NULL && m_defns_bucket == NULL); }
	bool		IsBucketOfDefnsResult() const { return(m_single_defn != NULL && m_defns_bucket != NULL); }

	TCppClassTemplateDefn	*FindClassTemplateOverload(ID key_id, TList *template_params_list);
	TCppCodeEntryPointDefn	*FindCodeEntryPointOverload(TCppNameInfoCore &key_info_core, TCppDeclaratorModifier *func_mdf);
	TCppFunctionTemplateDefn	*FindFunctionTemplateOverload(TList *template_params_list, TCppNameInfoCore &key_info_core, TCppDeclaratorModifier *func_mdf);
								// These methods expect that the DoSearch() method is already called and that it found appropriate
								// bucket of definitions. These methods only resolve overloading. They are not doing anything else.

	static int		GetNumberOfOverloads(TCppNameInfoCore &key_info_core, TCppDefnBase *single_defn, TCppDefnsBucket *defns_bucket);
					// This method should be called only after the seach. Method is defined as static and it has params for
					// simplicity of the code and for perf reasons. It is possible to redesign the class and get rid of params.
					// This method also works fine when the search discovers a non overloadable object.
public:

	TCppDefnBase		*m_single_defn;
	TCppDefnsBucket		*m_defns_bucket;
							// These data fields contain results of the search and they are accessed by the caller directly.
protected:

	TCppDefnBase		*m_search_container;
};

class TCppExistingDefnsSearchHelper : public TCppNewDefnsSearchHelper
{
public:

		TCppExistingDefnsSearchHelper(TCppDefnBase *layer_to_search) : TCppNewDefnsSearchHelper(layer_to_search) { m_search_key_info = NULL; m_curr_search_layer = NULL; }

	bool		DoSearch(TCppElabType key_elab, TCppNameInfoCore &key_info, bool search_hier);
				//
				//	Possible results of the search:
				//
				//		RetVal	  Defn	 Bucket	  Ambig
				//	  -------------------------------------------------------------------------------
				//		false		  NULL	  NULL	     no		There is no information about the passed key.
				//		TRUE	   Val	  NULL	     no		Single non overloadable definition.
				//		TRUE	   Val	   Val	     no		Bucket with overloadable defns plus the first discovered object that belongs to the bucket.
				//		TRUE	  NULL	  NULL	    YES		The passed search key is ambiguous. The m_ambig_defns array contains pointers to two or
				//											more definitions that are ambiguous to each other. All these definitions belong to different
				//											overload buckets in case if type of definitions allows overloading.
				//
				//	Note that base classes with identical short names are considered as ambiguous by this searcher.
				//	They do not form an overload group like it is done in the new defn searcher. There are two types
				//	of buckets with overloads here:
				//
				//		-- Class templates (plus class template specs and class template insts);
				//		-- Functions (plus func templates, func template specs and func template insts);
				//
				//	Unelaborated search:
				//
				//		If an unelaborated and an elaborated objects are both available at the same time, the unelaborated
				//		object is taking precedence. For example, consider the defns space:
				//
				//			struct s1 { ... };
				//			int s1;
				//
				//		Lookup for the name "s1" will result in the second definition and there will be no ambiguity. In order
				//		to access the first definition it is necessary to search for "struct s1" by passing appropriate param.
				//		Other example:
				//
				//			enum e2 { ... };
				//			class e2 { ... };
				//
				//		Lookup for the name "e2" is ambiguous because objects with different elaborations are present.
				//
				//	Elaborated search:
				//
				//		Objects of only one elaboration buckets are checked. Objects of all other elaborations are simply ignored.
				//

	bool		IsAmbigResult() const { return(m_ambig_defns.NumItems() > 0); }

public:

	TCppDefnBasePtrsArray		m_ambig_defns;
	bool						m_ambig_defns_buffering_error;
								// Error on expanding the list of ambiguous defns should be extremely rare. Nevertheless this still may
								// happen. Wording of the error message in this case should include something like "and other defns".

	TCppDefnBase			*m_curr_search_layer;
								// After non empty search this variable contains the latest search layer. Primary use of this variable is
								// picking up the layer where the ambiguity was discovered.
private:

	// This context is needed to avoid infinite recursions in situations when namespaces contain "USE NAMESPACE"
	// directives that point to each other (maybe indirectly), bogus class inheritance hierarchies with loops, etc.
	struct TCppSearchHelperContext
	{
		TCppDefnBase				*m_defns_container;
		TCppSearchHelperContext		*m_upper_context;

		bool	SearchContext(TCppDefnBase *defns_container)
		{
			TCppSearchHelperContext *ctx = this;
			while (ctx != NULL)
			{
				if (ctx->m_defns_container == defns_container)
					return(TRUE);
				ctx = ctx->m_upper_context;
			}

			// Passed container is not present in the current list of processed defn containers.
			return(FALSE);
		}
	};

	TCppElabType			m_search_key_elab;
	TCppNameInfoCore	*m_search_key_info;

	void		SearchDefnsLayer(TCppSearchHelperContext &context, TCppDefnsSpace *defns_space, TList *assoc_spaces_list);
	void		SearchSingleDefnsSpace(TCppDefnsSpace *defns_space);
	void		SummarizeDiscoveredObject(TCppDefnBase *single_defn, TCppDefnsBucket *defns_bucket);
	void		SummarizeDiscoveredAmbiguity(TCppDefnBasePtrsArray &ambig_defns, bool ambig_defns_buffering_error);
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//   ====== Part 2. ==========  App level defns  ================
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum TCppDeclTypeSpecPrefix
{
	declpr_none,
	declpr_typedef,
	declpr_friend,
};

enum TCppAccessLevel
{
	accs_public,
	accs_protected,
	accs_private,
	accs_num_types,
};

enum TCppFunctionAttr
{
	fcta_none			=   0,

	fcta_inline			=   1,
	fcta_virtual			=   2,
	fcta_explicit			=   4,		// This specifier can be used only with constructors.
									// It denies using such constructor as an implicit type converter.

	fcta_static			=   8,		// This flag is applicable only to the class member functions. Note that
									// function types do not have the storage class attribute like the data fields.

	fcta_this_const		=  32,		// This is const specifier, that stays at the end of the function prototype.
	fcta_this_volatile	=  64,		// This is volatile spec, that stays at the end of the function prototype.
			//
			// Const and volatile flags in the function prototype and function header are applicable
			// only to the non static class member functions. They specify propertiess of the first
			// implicit parameter "this".
			//

	fcta_pure			= 128,		// This flag has no corresponding keyword in the C++ grammar.
};

enum TCppBuiltInTypeType
{
	bltin_type_void,
	bltin_type_bool,
	bltin_type_char,
	bltin_type_int,
	bltin_type_float,
	bltin_type_ellips,

	bltin_num_types,
};

//
//  Built in types have fixed ids that do not change from one compilation to another.
//  All other objects in the Cpp database have dynamically assigned ids.
//
enum TCppBuiltInTypeIdent
{
	bltin_ids_base = 0x1220000,

	bltin_id_void,		bltin_id_bool,

	bltin_id_schar8,		bltin_id_uchar8,
	bltin_id_schar16,	bltin_id_uchar16,

	bltin_id_sint8,		bltin_id_uint8,
	bltin_id_sint16,		bltin_id_uint16,
	bltin_id_sint32,		bltin_id_uint32,
	bltin_id_sint64,		bltin_id_uint64,

	bltin_id_float4,		bltin_id_float8,
	bltin_id_float16,

	bltin_id_ellips,

	bltin_ids_max_plus1,
};

enum TCppInitalizerType
{
	cinit_none,

	cinit_single_expr,
	cinit_exprs_list,
	cinit_aggregate,

	cinit_num_types,
};

// ---------------------------------------------------------------------------
//  - - (CondParsAnalysis) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ---------------------------------------------------------------------------

struct TCppCondParsParam : public TCppDefnBase
{
	TCppCondParsParam(ID id = 0) : TCppDefnBase(cxx_cond_pars_param, id) { }
};

// ---------------------------------------------------------------------------
//  - - (Defns1) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ---------------------------------------------------------------------------

enum TCppDefnChildDest
{
	cdefn_chdest_none,

	cdefn_chdest_cltplt_iprm,
	cdefn_chdest_dtfld_init,
	cdefn_chdest_fntplt_iprm,

	cdefn_chdest_default = -1,
};


struct TCppBuiltInTypeDefn : public TCppDataTypeBase
{
	TCppBuiltInTypeDefn(ID id = 0) : TCppDataTypeBase(cxx_built_in_type, id)
			{ if (id != 0) SetBuiltInTypeProps((TCppBuiltInTypeIdent)id); else { m_type = bltin_type_void; m_unsigned = FALSE; m_byte_length = 0; } }

	TCppBuiltInTypeType				m_type;
	bool								m_unsigned;
	short							m_byte_length;
										// These data fields are not expected to be set up directly. Use the SetBuiltInTypeProps()
										// method to set them up.
public:

	void		SetBuiltInTypeProps(TCppBuiltInTypeIdent ident);

	static const wchar_t	*GetBuiltInTypeFriendlyName(TCppBuiltInTypeIdent ident);
	static const wchar_t	*GetBuiltInTypeTypeEnumName(TCppBuiltInTypeType val);
};

struct TCppBitFieldTypeDefn : public TCppDataTypeBase
{
	TCppBitFieldTypeDefn(ID id = 0) : TCppDataTypeBase(cxx_bit_field_type, id) { m_base_type = NULL; m_bit_length = 0; m_bit_length_expr = NULL; }
	~TCppBitFieldTypeDefn() { if (m_bit_length_expr != NULL) delete m_bit_length_expr; }
			// These objects are created with generated names only. When this object is inserted into the defns
			// space its base type should be a valid pointer to some integral type. Otherwise JPF will happen.

			//
			//  Note that bit field members can be specified as CONST/VOLATILE. These flags are stored in the base
			//  type of the bit field type. To get these props of the data field the base type of the current object should
			//  be checked.
			//

	TCppDataTypeBase				*m_base_type;					// Base type can be only either integer or enum. The pointer is not owned.

	short							m_bit_length;					// Bit length is a positive value. It cannot be zero. The value can be bigger than
																	// the bit size of the base type. In this case extra bits if any are used as padding
																	// and they do not increase the max possible value of the data field of this data
																	// type. When the value of the field is -1, this means that expression was not
																	// const or it was not numeric or its value was out of the reasonable range.
	TCppExpressionBase				*m_bit_length_expr;				// Expression that was used to specify the bit length. When bit feild is used in
																	// the template code and expression is not constant no error is generated.
																	// Note that this pointer describes an owned child.
public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);				//	Possible destination: cdefn_chdest_default.
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = m_bit_length_expr; }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_curr_child = NULL; }
};

struct TCppCvPtrRefTypeDefn : public TCppDataTypeBase
{
	TCppCvPtrRefTypeDefn(ID id = 0) : TCppDataTypeBase(cxx_cv_ptr_ref_type, id) { m_base_type = NULL; }
			// These objects can be have either a user defiend or a generated names. The base type should
			// always be some valid non NULL pointer.

	TCppDefnBase					*m_base_type;					//
																	// The pointer is not owned. It can be one of:
																	//
																	//	  TCppDataTypeBase			or its derivatives;
																	//	  TCppFunctionTypeBase		or its derivatives;
																	//
	TCppCvPtrRefSpecsArray			m_layers;						// The order of layers is the same to the order of stars in the source code.
																	// In other words, the layer[0] should be applied to the base type first.
																	// This order can also be called "from the base type to type of the variable".
};																	//

struct TCppArrayTypeDefn : public TCppDataTypeBase
{
	TCppArrayTypeDefn(ID id = 0) : TCppDataTypeBase(cxx_array_type, id) { m_base_type = NULL; }
			// These objects can be created using user defiend and generated names. Base type should always be some
			// valid non NULL pointer.

	~TCppArrayTypeDefn() { for (int idim=0; idim<m_dimensions.NumItems(); ++idim) if (m_dimensions[idim].m_num_elements_expr != NULL) delete m_dimensions[idim].m_num_elements_expr; }

	TCppDataTypeBase				*m_base_type;					// This pointer is not owned. Note that the base type is always a data type.
																	// It cannot be a function type.
	TCppArrayDimensionsArray			m_dimensions;					// The order of array elements in this data field is the same to the order of indexes
																	// in the source code where the array is being defined. When this type is constructed,
																	// dimensions should be added with the NULL expression subtree. The expr objects
																	// should be added later using the AddChildItem() method.
public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination_layer_index = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;

private:

	void			FindNextChildInternal(TCppDirectChildrenIter &iter) const;
};

struct TCppDataTypeAliasDefn : public TCppDataTypeBase
{
	TCppDataTypeAliasDefn(ID id = 0) : TCppDataTypeBase(cxx_data_type_alias, id) { m_base_type = NULL; }
			// When this object is added to the defns space the name and its non zero key_id should be set.

	TCppDataTypeBase				*m_base_type;					// The pointer is not owned.
};

// ---------------------------------------------------------------------------
//  - - (Defns2) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ---------------------------------------------------------------------------

struct TCppEnumTypeDefn : public TCppDataTypeBase
{
	TCppEnumTypeDefn(ID id = 0) : TCppDataTypeBase(cxx_enum_type, id) { m_defn_elab_type = elab_enum; m_defined = FALSE; m_base_type = NULL; }
	~TCppEnumTypeDefn() { m_members.DeleteAll(); }

		//
		//  When the m_defn_key_id field is zero this means that either this is nameless enum or that
		//  there was a name collision and as a consequence the enum object got an alternative name.
		//

	bool								m_defined;						// This field shows if enum definition is already processed or only one or
																	// several possible forward declarations were noticed.
	TCppDataTypeBase				*m_base_type;					// For now the base type of enum can be only an integral type. This can be
																	// a built in type, CvPtrRef (with const and/or volatile) or the data type alias.
	TList								m_members;					// Emun does not have its own hash table for its members because they are
																	// placed into the hash table of the containing named structure.
public:

	int			GetNumEnumMembers() const;

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);				//  Possible destination: cdefn_chdest_default.
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_members.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_members.GetNext(iter.m_curr_child); }
};

struct TCppEnumMemberDefn : public TCppDefnBase
{
	TCppEnumMemberDefn(ID id = 0) : TCppDefnBase(cxx_enum_member, id) { m_const_value = FALSE; m_value = 0; m_value_num_subt = lnt_u128bit; m_value_expr = NULL; }
	~TCppEnumMemberDefn() { if (m_value_expr != NULL) delete m_value_expr; }

	bool								m_const_value;					// The value was expressed using const and numeric expression or the value
																	// was implicit. When the value of this field is TRUE, this also means that
																	// the m_value field is meaningful.
	__int64							m_value;
	TLexNumberType					m_value_num_subt;

	TCppExpressionBase				*m_value_expr;					// Expression that was used to specify the value. Value of this field is NULL when
																	// the value of the enum member is implicit.
public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);				//  Possible destination: cdefn_chdest_default.
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = m_value_expr; }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_curr_child = NULL; }
};

//
//  This is an important structure. It describes classes, structures and unions. Note that structs/unions/classes
//  can be named and nameless.
//
struct TCppStructTypeDefn : public TCppDataTypeBase
{
	TCppStructTypeDefn(ID id = 0) : TCppDataTypeBase(cxx_struct_type, id) { m_defined = FALSE; m_real_defns_table = &m_defns_table; }
			// Note that this ctor sets an incorrect elaboration type. Correct elab type should be set up later.

	~TCppStructTypeDefn() { m_members.DeleteAll(); }
			// Defns table does not need destruction.

	void		SetupStructType(TCppElabType elab_type) { assert(elab_type != elab_none && elab_type != elab_enum); m_defn_elab_type = elab_type; }
	void		SetupNamedParent(TCppStructTypeDefn *named_parent) { m_real_defns_table = &(named_parent->m_defns_table); }
	void		AddBaseClassesList(TList *base_classes_list);

	bool								m_defined;						// This field shows if the struct/union/class definition is already processed
																	// or just some number of forward declarations were noticed.
	TList								m_members;

	TCppDefnsTable<64, 8>			m_defns_table;
	TCppDefnsTable<64, 8>			*m_real_defns_table;
										// Ordinary named structures are using their own names table and this pointer points to
										// the local definitions table. On the contrary the nameless structures are using names
										// tables of their nearest named parent and their local names tables are not used.

	TList								m_assoc_defn_spaces;			// This list contains instances of TCppAssocDefnsSpaceRef and
																	// it is not owning its members.
public:

	virtual bool		IsStructType() const { return(TRUE); }
	bool				IsNamelessStruct() const { return(m_defn_key_id == 0); }
	virtual bool		CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1) { AddToMembersListAndDefnsSpace(item, m_members, *m_real_defns_table); }
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

	virtual TList	*GetNestedMembersList() const { return((TList*)&m_members); }

	int				GetNumTemplateParams() const;
	TCppDefnBase	*GetFirstTemplateParam() const { return(GetFirstTemplateParameter(&m_members)); }
	TCppDefnBase	*GetNextTemplateParam(TCppDefnBase *param) const { return(GetNextTemplateParameter(&m_members, param)); }
						// Methods for iterating template parameters are defined in this class because ClassTemplateSpecialization
						// class and ClassTemplateInstantiation are derived directly from the struct type class.
protected:

	virtual TCppDefnsSpace		*GetNestedDefnsSpace() { return(m_real_defns_table); }
	virtual TList				*GetAssocDefnsSpaceRefsList() { return(&m_assoc_defn_spaces); }
	virtual void				AddToNestedDefnsSpace(TCppDefnBase *defn_item) { m_real_defns_table->AddRegularDefinition(defn_item); }
	virtual void				AddToNestedDefnsSpaceSpecial(TCppDefnBase *defn_item) { m_real_defns_table->AddSpecialDefinition(defn_item); }

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_members.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_members.GetNext(iter.m_curr_child); }

	friend class TCppParser;
};

struct TCppBaseClassSpec : public TCppDefnBase
{
	TCppBaseClassSpec(ID id = 0) : TCppDefnBase(cxx_base_class_spec, id) { m_virtual = FALSE; m_accs_level = accs_private; m_base_class = NULL; m_assoc_ref.m_assoc_space_ref_object = this; }
			// The name of the base class is expected to be duplicated into the m_defn_name of the instance.
			// Important: Elaboration of this object is always elab_none regardless of the nature (struct/union/class)
			// of the base class itself. This is needed for proper work of the defns space search classes.

	bool								m_virtual;
	TCppAccessLevel					m_accs_level;
	TCppStructTypeDefn				*m_base_class;					// The pointer is not owned. In non template contexts this pointer can point to TCppStructTypeDefn,
																	// TCppClassTemplateInstantiation and TCppUnresolvedStructType. In the context of a template definition
																	// this can also be a TCppClassTemplateDefn.
	TCppAssocDefnsSpaceRef			m_assoc_ref;
};

struct TCppFieldAlignSpec : public TCppItemBase
{
	TCppFieldAlignSpec(ID id = 0) : TCppItemBase(cxx_field_align_spec, id) { m_align_value = 0; m_align_value_expr = NULL; }
	~TCppFieldAlignSpec() { if (m_align_value_expr != NULL) delete m_align_value_expr; }

	short							m_align_value;					// Field alignment is a positive value or zero. When the value of this field is -1,
																	// this means that expression was not const or it was not numeric or its value
																	// was negative or the value was out of positive reasonable range.
	TCppExpressionBase				*m_align_value_expr;			// Expression that was used to specify the align value. When align spec is used
																	// in the template code and expression is not constant no error is generated.
																	// Note that this pointer describes an owned child.
public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);				//	Possible destination: cdefn_chdest_default.
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = m_align_value_expr; }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_curr_child = NULL; }
};

struct TCppFriendSpec : public TCppItemBase
{
	TCppFriendSpec(ID id = 0) : TCppItemBase(cxx_friend_spec, id) { m_friendly_object = NULL; }

	TCppDefnBase					*m_friendly_object;				// The pointer is not owned. This pointer always points to the definition of the real object,
																	// that can be searched by its name, used for creating data fields if this is appropriate, etc.
																	// The friend can be one of: structure, functuion, class template, template specialization,
																	// template instantiation and 3 similar object types for function templates.
};

//
//  Data field describes location in the static memory, stack of the running thread or in the heap memory.
//  It can be also a non-type tempate parameter or a catch block parameter.
//
struct TCppDataFieldDefn : public TCppDefnBase
{
	TCppDataFieldDefn(ID id = 0) : TCppDefnBase(cxx_data_field, id)
			{ m_visibility_pars_sess = 0; m_linkage_spec = NULL; m_accs_level = accs_public; m_strg_class = strgc_none; m_param_index = -1; m_mutable = FALSE; m_field_type = NULL; m_field_initializer = NULL; }

	~TCppDataFieldDefn() { if (m_field_initializer != NULL) delete m_field_initializer; }

	ID								m_visibility_pars_sess;			// This data field has meaning only for data fields on the namespace layers. If the value of
																	// the field is not zero, this means that this data field is visible only inside this specific parsing
																	// session. In databases that describe only one parsing session, different data fields with zero
																	// and non zero values for a given name are not allowed. In multisession databases only one
																	// object for a given layer/name pair can have zero in this data field.
	TCppLinkageSpec					*m_linkage_spec;

	TCppAccessLevel					m_accs_level;

	TCppStorageClass					m_strg_class;
											//
											// Possible values for the storage class of the regular data fields on the different types of layers:
											//
											//	Namespace:			none, static, extern.
											//	Struct/Union/Class:	none, static, extern.
											//	Block:				none, register, auto, static;
											//
											// Extern data field on the class layer is a static data field of the class that is not yet defined
											// on the namespace level. Note that blocks can have static data fields. It would be correct to
											// understand them as objects on the namespace layer, only their names are not visible outside
											// of their blocks.
											//

	short							m_param_index;					// This field has meaning only when the strg_class field is equal either to strgc_param or to
																	// strgc_template_param. This is zero based index.
	bool								m_mutable;						// This flag has meaning only for non static data members of the classes. It allows changing
																	// this data field from the member functions, that are declared with trailing const.
	TCppDataTypeBase				*m_field_type;

	TCppItemBase					*m_field_initializer;				// Initializer is an owned child. For function/template parameters this is a default value.
																	// Value of this field can only be either a TCppDataFieldInitializer or a derivative ftom
																	// the TCppExpressionBase.
public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

	static const wchar_t	*GetAccessLevelFriendlyName(TCppAccessLevel accs);
	static const wchar_t	*GetStorageClassFriendlyName(TCppStorageClass strgc);
	static const wchar_t	*GetStorageClassEnumName(TCppStorageClass strgc);

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = m_field_initializer; }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_curr_child = NULL; }
};

struct TCppDataFieldInitializer : public TCppItemBase
{
	TCppDataFieldInitializer(ID id = 0) : TCppItemBase(cxx_data_field_initializer, id) { m_init_type = cinit_none; }
	~TCppDataFieldInitializer() { m_init_elements.DeleteAll(); }
			// By its nature the initializer is very close to the list of expressions. Nevertheless it is not derived from
			// expression because it cannot be part of any expression. Plus deriving it from expression will require
			// defining new type of operation since all expressions have something in their m_operation field.

	TCppInitalizerType			m_init_type;
	TList						m_init_elements;						// This list contains mixture of TCppDataFieldInitializer instances and
																	// derivatives form TCppExpressionBase.
public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

	static const wchar_t	*GetInitializerTypeEnumName(TCppInitalizerType init_type);

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_init_elements.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_init_elements.GetNext(iter.m_curr_child); }
};

// ---------------------------------------------------------------------------
//  - - (Defns3) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ---------------------------------------------------------------------------

struct TCppFunctionTypeDefn : public TCppFunctionTypeBase
{
	TCppFunctionTypeDefn(ID id = 0) : TCppFunctionTypeBase(cxx_function_type, id)
			{ m_visibility_pars_sess = 0; m_linkage_spec = NULL; m_accs_level = accs_public; m_func_attrs = fcta_none; m_spec_type = spcft_none; m_operator_token = opr_none; m_return_value_type = NULL; }

	~TCppFunctionTypeDefn() { m_members.DeleteAll(); }
			// Members of the list (parameters) are mapped into the names space of the function body.

	ID								m_visibility_pars_sess;			// This data field has meaning only for functions of the namespace level. If the value
																	// of the field is not zero, this means that this function is visible only in the specified
																	// parsing session. In the source code such functions appear as static functions.
	TCppLinkageSpec					*m_linkage_spec;

	TCppAccessLevel					m_accs_level;
	TCppFunctionAttr					m_func_attrs;					// This field is especially important for class member functions. It shows whether the function
																	// is a static method or an instance method.

	TCppSpecFuncType				m_spec_type;
	TOperatorToken					m_operator_token;				// This field has meaning only if m_spec_type == spcft_operator.
	TCppDataTypeBase				*m_return_value_type;			// The pointer is not owned. This field has meaning for all functions and methods that can
																	// return value and also when m_spec_type == spcft_typecast.

	TList								m_members;					// Members of this list can be: parameters and possible exception specs. Note that derived
																	// classes may store other objects in this list like the body of the function and/or parameters
																	// of the function template.
	virtual bool		IsFunctionName() const { return(TRUE); }
	virtual bool		CanHaveChildren() const { return(TRUE); }

	bool				IsInstanceMethod() const { return((m_func_attrs & fcta_static) == 0); }
	bool				IsStaticMethod() const { return((m_func_attrs & fcta_static) != 0); }
						// Note that both methods are not checking if current definition is a member of the class or not.
public:

	static bool			IsParamItem(TCppItemBase *item);

	int					GetNumParams() const;
	TCppDataFieldDefn		*GetFirstParam() const;
	TCppDataFieldDefn		*GetNextParam(TCppDataFieldDefn *param) const;

	int							GetNumThrowSpecs() const;
	TCppPossibleExceptionSpec		*GetFirstThrowSpec() const;
	TCppPossibleExceptionSpec		*GetNextThrowSpec(TCppPossibleExceptionSpec *curr_throw_spec) const;

	bool			CompareFunctionNames(TCppNameInfoCore &name_info) const;
	bool			CompareParameterLists(TCppDeclaratorModifier *func_mdf_layer) const;

	static bool	CompareFunctionTypes(TCppFunctionTypeDefn *func_type1, TCppFunctionTypeDefn *func_type2, bool compare_param_lists);

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

	static const wchar_t	*GetSpecFuncTypeEnumName(TCppSpecFuncType spcft);

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_members.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_members.GetNext(iter.m_curr_child); }
};

struct TCppFunctionTypeAliasDefn : public TCppFunctionTypeBase
{
	TCppFunctionTypeAliasDefn(ID id = 0) : TCppFunctionTypeBase(cxx_function_type_alias, id) { m_base_type = NULL; }

	TCppFunctionTypeBase				*m_base_type;					// The pointer is not owned.
};

typedef THeapBasedArray<TCppCodeEntryPointDefn*, 4>	TCppCodeEntryPointDefnPtrsArray;
														// This definiton should stay before the code entrypoint itself.

//
//  Code entry point describes some function or method of the class.
//
struct TCppCodeEntryPointDefn : public TCppFunctionTypeDefn
{
	TCppCodeEntryPointDefn(ID id = 0) : TCppFunctionTypeDefn(id) { m_cib_type = cxx_code_entry_point; m_function_body = NULL; }

	TCppBlockDefn					*m_function_body;				// This pointer points at one of the elements of list of members that is defined
																	// in the TCppFunctionTypeDefn class. Note that this field can point either to
																	// a simple block or to a try block. When objects represents a function prototype,
																	// the value of this field is NULL.
	TCppCodeEntryPointDefnPtrsArray	m_override_protos;				// This is a list of identical methods in the base classes. Please, do not confuse
																	// method overloading with method overriding.

	virtual bool		IsFunctionFamily() const { return(TRUE); }
	bool				IsFunctionPrototype() const { return(m_function_body == NULL); }

public:

	int				GetNumTemplateParams() const;
	TCppDefnBase	*GetFirstTemplateParam() const { return(GetFirstTemplateParameter(&m_members)); }
	TCppDefnBase	*GetNextTemplateParam(TCppDefnBase *param) const { return(GetNextTemplateParameter(&m_members, param)); }
						// Methods for iterating template parameters are defined in this class because FucntionTemplateSpecialization
						// and FucntionTemplateInstantiation are derived directly from the code entry point class.
};

struct TCppPossibleExceptionSpec : public TCppItemBase
{
	TCppPossibleExceptionSpec(ID id = 0) : TCppItemBase(cxx_possible_exception_spec, id) { m_exception_type = NULL; }

	TCppDataTypeBase				*m_exception_type;				// The pointer is not owned. In fact, this pointer can be NULL. This case represents
																	// situation when the list of possible exceptions is present and it is empty. Such empty
																	// list is described by an exceptions spec object with a NULL in its data field.
};

// ---------------------------------------------------------------------------
//  - - (Defns4) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ---------------------------------------------------------------------------

struct TCppNamespaceDefn : public TCppDefnBase
{
	TCppNamespaceDefn(ID id = 0) : TCppDefnBase(cxx_namespace, id) { }
			// Note that namespaces can be named and nameless.

	~TCppNamespaceDefn() { m_members.DeleteAll(); }
			// Defns table does not need destruction.

	TList								m_members;
	TCppDefnsTable<256, 16>			m_defns_table;

	TList								m_assoc_defn_spaces;			// This list contains instances of TCppAssocDefnsSpaceRef and
																	// it is not owning its members.
public:

	virtual bool	IsNamespace() const { return(TRUE); }
	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1) { AddToMembersListAndDefnsSpace(item, m_members, m_defns_table); }
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

	virtual TList	*GetNestedMembersList() const { return((TList*)&m_members); }

protected:

	virtual TCppDefnsSpace		*GetNestedDefnsSpace() { return(&m_defns_table); }
	virtual TList				*GetAssocDefnsSpaceRefsList() { return(&m_assoc_defn_spaces); }
	virtual void				AddToNestedDefnsSpace(TCppDefnBase *defn_item) { m_defns_table.AddRegularDefinition(defn_item); }
	virtual void				AddToNestedDefnsSpaceSpecial(TCppDefnBase *defn_item) { m_defns_table.AddSpecialDefinition(defn_item); }

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_members.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_members.GetNext(iter.m_curr_child); }
};

struct TCppGlobalNamespaceDefn : public TCppDefnBase
{
	TCppGlobalNamespaceDefn(ID id = 0) : TCppDefnBase(cxx_global_namespace, id) { m_owner_database = NULL; }
			// Global namespace does not have any name. In typical scenario it is never created
			// using the cpp items factory. It is data field of the cpp database object.

	~TCppGlobalNamespaceDefn() { m_members.DeleteAll(); }
			// Defns table does not need destruction.

	TList								m_members;
	TCppDefnsTable<4096, 256>		m_defns_table;

	TList								m_assoc_defn_spaces;			// This list contains instances of TCppAssocDefnsSpaceRef and
																	// it is not owning its members.
	TCppDatabase					*m_owner_database;			// Primary purpose of this pointer is assigning item ids to
																	// the new objects that are added to the global namespace.
public:

	virtual bool	IsNamespace() const { return(TRUE); }
	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root);

	virtual TList	*GetNestedMembersList() const { return((TList*)&m_members); }

protected:

	void			Clear();

	virtual TCppDefnsSpace		*GetNestedDefnsSpace() { return(&m_defns_table); }
	virtual TList				*GetAssocDefnsSpaceRefsList() { return(&m_assoc_defn_spaces); }
	virtual void				AddToNestedDefnsSpace(TCppDefnBase *defn_item) { m_defns_table.AddRegularDefinition(defn_item); }
	virtual void				AddToNestedDefnsSpaceSpecial(TCppDefnBase *defn_item) { m_defns_table.AddSpecialDefinition(defn_item); }

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = -1; iter.m_curr_child = (TCppItemBase*)m_members.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_members.GetNext(iter.m_curr_child); }

	friend class TCppDatabase;
};

struct TCppNamespaceAliasDefn : public TCppDefnBase
{
	TCppNamespaceAliasDefn(ID id = 0) : TCppDefnBase(cxx_namespace_alias, id) { m_base_namespace = NULL; }

	TCppNamespaceDefn				*m_base_namespace;			// The pointer is not owned. The value of the field is NULL when
																	// the name of the namespace was not resolved.
};

struct TCppUsingDeclarationDefn : public TCppDefnBase
{
	TCppUsingDeclarationDefn(ID id = 0) : TCppDefnBase(cxx_using_declaration, id) { m_defn_to_use = NULL; }

	TCppDefnBase					*m_defn_to_use;				// The pointer is not owned.
};

struct TCppUsingDirectiveDefn : public TCppDefnBase
{
	TCppUsingDirectiveDefn(ID id = 0) : TCppDefnBase(cxx_using_directive, id) { m_namespace_to_use = NULL; m_assoc_ref.m_assoc_space_ref_object = this; }

	TCppNamespaceDefn				*m_namespace_to_use;			// The pointer is not owned. The value of the field is NULL when "use
																	// namespace" directive is not pointing to an existing namespace.
	TCppAssocDefnsSpaceRef			m_assoc_ref;
};

struct TCppAsmInsertDefn : public TCppDefnBase
{
	TCppAsmInsertDefn(ID id = 0) : TCppDefnBase(cxx_asm_insert, id) { m_asm_insert.m_body = NULL; m_asm_insert.m_len = 0; }

	TStrPtrInfo						m_asm_insert;					// The string is not owned by the object. The field has TStrPtrInfo type
																	// because the string may contain NULL characters inside it.
};

struct TCppLinkageSpec : public TCppItemBase
{
	TCppLinkageSpec(ID id = 0) : TCppItemBase(cxx_linkage_spec, id) { m_spec_name = NULL; m_spec_key_id = 0; m_spec_cnt_use = 0; }

	wchar_t							*m_spec_name;					// Note that the string is not owned.
	ID								m_spec_key_id;
	int								m_spec_cnt_use;

public:

	static bool	CompareLinkageSpecs(const TCppLinkageSpec *spec1, const TCppLinkageSpec *spec2);
					// This method allows passing NULL specs. Two NULL specs are compatible.
};

// ---------------------------------------------------------------------------
//  - - (Templates) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ---------------------------------------------------------------------------

enum TCppTemplateChildDest
{
	ctplt_chdest_none,
	ctplt_chdest_default = -1,
};

struct TCppClassTemplateDefn : public TCppStructTypeDefn
{
	TCppClassTemplateDefn(ID id = 0) : TCppStructTypeDefn(id) { m_cib_type = cxx_class_template; }
				// Template parameters are added into the list of the struct members that is located in the base class.
				// They can be distinguished from other members of this list by their object types for type params and
				// template params and by their storage class (strgc_template_param) for data params.

	//
	//  Important: Elaboration type of this object (also the class template spec and the class template inst)
	//  should always be non empty. This is needed for proper work of the defns space search classes.
	//

	virtual bool	IsDataType(TCppDefnBase *context = NULL) const { return(CheckContextSpecial(context)); }
	virtual bool	IsClassTemplateFamily() const { return(TRUE); }

	//
	//  Note that struct type contains methods for iterating template parameters like GetFirstTemplateParam(),
	//  GetNextTemplateParam(), etc. These methods are not defined here because template specialization and
	//  template instantiation are derived directly from the struct type.
	//
};

struct TCppClassTemplateSpecialization : public TCppStructTypeDefn
{
	TCppClassTemplateSpecialization(ID id = 0) : TCppStructTypeDefn(id) { m_cib_type = cxx_class_template_spec; m_base_template = NULL; }
	~TCppClassTemplateSpecialization() { m_specialization_params.DeleteAll(); }

	TCppClassTemplateDefn			*m_base_template;					// The pointer is not owned.
	TList								m_specialization_params;

public:

	virtual bool	IsDataType(TCppDefnBase *context = NULL) const { return(CheckContextSpecial(context)); }
	virtual bool	IsClassTemplateFamily() const { return(TRUE); }
	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppClassTemplateInstantiation : public TCppStructTypeDefn
{
	TCppClassTemplateInstantiation(ID id = 0) : TCppStructTypeDefn(id) { m_cib_type = cxx_class_template_inst; m_defined = TRUE; m_original_template = NULL; }
	~TCppClassTemplateInstantiation() { m_instantiation_arguments.DeleteAll(); }
			// Note that template instantiation cannot be a forward declaration.

	TCppClassTemplateDefn			*m_original_template;				// The pointer is not owned.
	TList								m_instantiation_arguments;

public:

	virtual bool	IsClassTemplateFamily() const { return(TRUE); }
	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);				// Possible destinations: cdefn_chdest_cltplt_iprm, cdefn_chdest_default.
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppFunctionTemplateDefn : public TCppCodeEntryPointDefn
{
	TCppFunctionTemplateDefn(ID id = 0) : TCppCodeEntryPointDefn(id) { m_cib_type = cxx_func_template; }
				// Template parameters are added into the list of the function type members. They can be distinguished
				// from other members of this list by their object types for type params and template params and by
				// their storage class (strgc_template_param) for data params.

	virtual bool	IsFunctionType(TCppDefnBase *context = NULL) const { return(CheckContextSpecial(context)); }

	//
	//  Note that code entry point class contains methods for template params iteration like GetFirstTemplateParam(),
	//  GetNextTemplateParam(), etc. These methods are not defined in this class because func template specialization
	//  and function template instantiation classes are derived directly from the code entry point class.
	//
};

struct TCppFunctionTemplateSpecialization : public TCppCodeEntryPointDefn
{
	TCppFunctionTemplateSpecialization(ID id = 0) : TCppCodeEntryPointDefn(id) { m_cib_type = cxx_func_template_spec; m_base_template = NULL; }
	~TCppFunctionTemplateSpecialization() { m_specialization_params.DeleteAll(); }

	TCppFunctionTemplateDefn			*m_base_template;					// The pointer is not owned.
	TList								m_specialization_params;

public:

	virtual bool	IsFunctionType(TCppDefnBase *context = NULL) const { return(CheckContextSpecial(context)); }
	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppFunctionTemplateInstantiation : public TCppCodeEntryPointDefn
{
	TCppFunctionTemplateInstantiation(ID id = 0) : TCppCodeEntryPointDefn(id) { m_cib_type = cxx_func_template_inst; m_original_template = NULL; }
	~TCppFunctionTemplateInstantiation() { m_instantiation_arguments.DeleteAll(); }

	TCppFunctionTemplateDefn			*m_original_template;				// The pointer is not owned.
	TList								m_instantiation_arguments;

public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppTemplateTypeParamDefn : public TCppDataTypeBase
{
	TCppTemplateTypeParamDefn(ID id = 0) : TCppDataTypeBase(cxx_template_type_param, id) { m_param_index = -1; m_default_value = NULL; }
			// Note that the type parameter itself does not have any properties.

	short							m_param_index;
	TCppDataTypeBase				*m_default_value;				// The pointer can be NULL and it is not owned.
};

struct TCppTemplateTemplateParamDefn : public TCppDefnBase
{
	TCppTemplateTemplateParamDefn(ID id = 0) : TCppDefnBase(cxx_template_template_param, id) { m_param_index = -1; m_default_value = NULL; }
	~TCppTemplateTemplateParamDefn() { m_template_params.DeleteAll(); }

	short							m_param_index;
	TList								m_template_params;
	TCppClassTemplateDefn			*m_default_value;				// The pointer can be NULL and it is not owned.

public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_template_params.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_template_params.GetNext(iter.m_curr_child); }
};

struct TCppTemplateTypeArgument : public TCppItemBase
{
	TCppTemplateTypeArgument(ID id = 0) : TCppItemBase(cxx_template_type_argument, id) { m_argument_value = NULL; }

	TCppDataTypeBase				*m_argument_value;				// The pointer is not owned.
};

struct TCppTemplateTemplateArgument : public TCppItemBase
{
	TCppTemplateTemplateArgument(ID id = 0) : TCppItemBase(cxx_template_template_argument, id) { m_arg_type = ctarg_none; }

	enum TCppTemplTemplArgType
	{
		ctarg_none,
		ctarg_template_defn,
		ctarg_template_template_param,
		ctarg_class_templates_bucket,
		ctarg_num_types,
	};

	TCppTemplTemplArgType			m_arg_type;

	union
	{
		TCppClassTemplateDefn			*m_argument_value;			// Option for "ctarg_template_defn". This is either a resolved class template definition or
																	// an unresolved class template object. The pointer is not owned.
		TCppTemplateTemplateParamDefn	*m_template_param;		// Option for "ctarg_template_template_param". This is pointer to the object in the template
																	// header. This value cannot be present in the in the args list of the full template instantiation.
																	// It is used while scanning template ids. The pointer is not owned.
		struct
		{
			TCppDefnBase				*m_search_res_single_defn;
			TCppDefnsBucket				*m_search_res_defns_bucket;
											// Option for "ctarg_class_templates_bucket". These fields are intermediate values that are used
											// when parsing detects that name describes a class template but overload resolution has not taken
		};									// place yet. Both pointers are not owned.
	};

	const wchar_t			*GetReferencedDefnName();
	static const wchar_t	*GetArgTypeEnumName(TCppTemplTemplArgType ctarg);
};

// ---------------------------------------------------------------------------
//  - - (Unresolved) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ---------------------------------------------------------------------------

enum TCppUnresolvedChildDest
{
	cunr_chdest_none,
	cunr_chdest_default = -1,
};

struct TCppUnresolvedNameInfo : public TCppDefnBase
{
	TCppUnresolvedNameInfo(ID id = 0) : TCppDefnBase(cxx_unresolved_name_info, id) { m_node_type = cnit_none; m_operator_token = opr_none; m_conversion_dest_type = NULL; }
	~TCppUnresolvedNameInfo() { m_template_id_args.DeleteAll(); }

	TCppNameInfoType				m_node_type;					// Note that the key_id for all named forms of the name info (name, template id and destructor)
																	// is stored in m_defn_key_id data field of the base class.
	TOperatorToken					m_operator_token;				// Token of the overloaded operator.
	TCppDataTypeBase				*m_conversion_dest_type;		// Type of the overloaded conversion destination. The pointer cannot be NULL and it is not
																	// owned.
	TList								m_template_id_args;			// This list can contain objects of the following types: TCppTemplateTypeArgument,
																	// TCppTemplateTemplateArgument and derivatives of the TCppExpressionBase.
public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

	static const wchar_t	*GetNameInfoTypeEnumName(TCppNameInfoType nitp);

protected:

	static bool	CompareNameInfoSequencies(TList *list1, TList *list2);
	static bool	CompareNameInfoInstances(TCppUnresolvedNameInfo *obj1, TCppUnresolvedNameInfo *obj2);
	static bool	CompareNameInfoTemplateArgLists(TList &list1, TList &list2);

	DWORD			GetHashValue();
	static DWORD		GetHashValueForNamesSeq(TList *unres_names_list);

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_template_id_args.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_template_id_args.GetNext(iter.m_curr_child); }

	friend class TCppItemBase;
	friend class TCppDefnsSpace;
};

struct TCppUnresolvedDataType : public TCppDataTypeBase
{
	TCppUnresolvedDataType(ID id = 0) : TCppDataTypeBase(cxx_unresolved_data_type, id) { }
	~TCppUnresolvedDataType() { m_name_elements.DeleteAll(); }

	TList								m_name_elements;
										// List of TCppUnresolvedNameInfo objects. In a regular case this list should be not empty.
public:

	virtual bool	IsUnresolvedObject() const { return(TRUE); }
	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual TList	*GetUnresolvedNamesList() const { return((TList*)&m_name_elements); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);				//	Possible destination: cdefn_chdest_default.
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_name_elements.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_name_elements.GetNext(iter.m_curr_child); }
};

//
// Typical elaboration for this type is elab_none.
//
struct TCppUnresolvedStructType : public TCppStructTypeDefn
{
	TCppUnresolvedStructType(ID id = 0) : TCppStructTypeDefn(id) { m_cib_type = cxx_unresolved_struct_type; }
	~TCppUnresolvedStructType() { m_name_elements.DeleteAll(); }

	TList								m_name_elements;
										// List of TCppUnresolvedNameInfo objects. In a regular case this list should be not empty.
public:

	virtual bool	IsUnresolvedObject() const { return(TRUE); }
	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual TList	*GetUnresolvedNamesList() const { return((TList*)&m_name_elements); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);				//	Possible destination: cdefn_chdest_default.
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_name_elements.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_name_elements.GetNext(iter.m_curr_child); }
};

struct TCppUnresolvedDataField : public TCppDataFieldDefn
{
	TCppUnresolvedDataField(ID id = 0) : TCppDataFieldDefn(id)  { m_cib_type = cxx_unresolved_data_field; }
	~TCppUnresolvedDataField() { m_name_elements.DeleteAll(); }

	TList								m_name_elements;
										// List of TCppUnresolvedNameInfo objects. In a regular case this list should be not empty.
public:

	virtual bool	IsUnresolvedObject() const { return(TRUE); }
	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual TList	*GetUnresolvedNamesList() const { return((TList*)&m_name_elements); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);				//	Possible destination: cdefn_chdest_default.
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_name_elements.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_name_elements.GetNext(iter.m_curr_child); }
};

struct TCppUnresolvedCodeEntryPoint : public TCppCodeEntryPointDefn
{
	TCppUnresolvedCodeEntryPoint(ID id = 0) : TCppCodeEntryPointDefn(id) { m_cib_type = cxx_unresolved_code_entry_point; }
	~TCppUnresolvedCodeEntryPoint() { m_name_elements.DeleteAll(); }

	TList								m_name_elements;
										// List of TCppUnresolvedNameInfo objects. In a regular case this list should be not empty.
public:

	virtual bool	IsUnresolvedObject() const { return(TRUE); }
	virtual bool	IsFunctionFamily() const { return(FALSE); }
	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual TList	*GetUnresolvedNamesList() const { return((TList*)&m_name_elements); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);				//	Possible destination: cdefn_chdest_default.
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_name_elements.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_name_elements.GetNext(iter.m_curr_child); }
};

struct TCppUnresolvedClassTemplate : public TCppClassTemplateDefn
{
	TCppUnresolvedClassTemplate(ID id = 0) : TCppClassTemplateDefn(id) { m_cib_type = cxx_unresolved_class_template; }
	~TCppUnresolvedClassTemplate() { m_name_elements.DeleteAll(); }

	TList								m_name_elements;
										// List of TCppUnresolvedNameInfo objects. In a regular case this list should be not empty.
public:

	virtual bool	IsUnresolvedObject() const { return(TRUE); }
	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual TList	*GetUnresolvedNamesList() const { return((TList*)&m_name_elements); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);				//	Possible destination: cdefn_chdest_default.
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_name_elements.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_name_elements.GetNext(iter.m_curr_child); }
};

struct TCppUnresolvedFunctionTemplate : public TCppFunctionTemplateDefn
{
	TCppUnresolvedFunctionTemplate(ID id = 0) : TCppFunctionTemplateDefn(id) { m_cib_type = cxx_unresolved_func_template; }
	~TCppUnresolvedFunctionTemplate() { m_name_elements.DeleteAll(); }

	TList								m_name_elements;
										// List of TCppUnresolvedNameInfo objects. In a regular case this list should be not empty.
public:

	virtual bool	IsUnresolvedObject() const { return(TRUE); }
	virtual bool	IsFunctionFamily() const { return(FALSE); }
	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual TList	*GetUnresolvedNamesList() const { return((TList*)&m_name_elements); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);				//	Possible destination: cdefn_chdest_default.
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_name_elements.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_name_elements.GetNext(iter.m_curr_child); }
};

// ---------------------------------------------------------------------------
//  - - (Stmts1) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ---------------------------------------------------------------------------

enum TCppStmtChildDest
{
	cstmt_chdest_none,

	cstmt_chdest_if_cond,
	cstmt_chdest_if_then,
	cstmt_chdest_if_else,
	cstmt_chdest_sw_cond,
	cstmt_chdest_sw_stmt,

	cstmt_chdest_for_expr1,
	cstmt_chdest_for_expr2,
	cstmt_chdest_for_expr3,
	cstmt_chdest_for_body,

	cstmt_chdest_wh_cond,
	cstmt_chdest_wh_stmt,
	cstmt_chdest_dw_stmt,
	cstmt_chdest_dw_cond,
	cstmt_chdest_ret_expr,

	cstmt_chdest_ltblk_prepend,
	cstmt_chdest_default = -1,
};

struct TCppEmptyStatement : public TCppStatementBase
{
	TCppEmptyStatement(ID id = 0) : TCppStatementBase(cxx_empty_stmt, id) { }
};

struct TCppIfStatement : public TCppStatementBase
{
	TCppIfStatement(ID id = 0) : TCppStatementBase(cxx_if_stmt, id) { m_if_condition = m_then_statement = m_else_statement = NULL; }
	~TCppIfStatement() { if (m_if_condition != NULL) delete m_if_condition; if (m_then_statement != NULL) delete m_then_statement; if (m_else_statement != NULL) delete m_else_statement; }

	TCppStatementBase				*m_if_condition;					// This can be either definition of ONE variable (with initializer) or an expression.
																	// When condition introduces a variable, the whole statement is wrapped inyo a block.
	TCppStatementBase				*m_then_statement;
	TCppStatementBase				*m_else_statement;

	inline bool	HasThenPart() const { return(m_then_statement != NULL && m_then_statement->ItemType() != cxx_empty_stmt); }
	inline bool	HasElsePart() const { return(m_else_statement != NULL && m_else_statement->ItemType() != cxx_empty_stmt); }

public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
					// Possible destinations: cstmt_chdest_if_cond, cstmt_chdest_if_then, cstmt_chdest_if_else.

	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppSwitchStatement : public TCppStatementBase
{
	TCppSwitchStatement(ID id = 0) : TCppStatementBase(cxx_switch_stmt, id) { m_condition = m_statement = NULL; }
	~TCppSwitchStatement() { if (m_condition != NULL) delete m_condition; if (m_statement != NULL) delete m_statement; }

	TCppStatementBase				*m_condition;					// This can be either definition of ONE variable (with initializer) or an expression.
																	// When condition introduces a variable, the whole statement is wrapped into a block.
	TCppStatementBase				*m_statement;					// In typical case this will be the block statement.

public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

	int			NumCaseLabels() const;

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppForStatement : public TCppStatementBase
{
	TCppForStatement(ID id = 0) : TCppStatementBase(cxx_for_stmt, id) { m_init_statement = NULL; m_check_continue = NULL; m_reinit_expression = NULL; m_body_statement = NULL; }
	~TCppForStatement() { if (m_init_statement != NULL) delete m_init_statement; if (m_check_continue != NULL) delete m_check_continue; if (m_reinit_expression != NULL) delete m_reinit_expression;  if (m_body_statement != NULL) delete m_body_statement; }

	TCppStatementBase				*m_init_statement;				// This can be definition of ONE variable, defenition of SEVERAL vars (all these variables
																	// should have the same base type) or simply an expression.
	TCppStatementBase				*m_check_continue;				// Continue check may contain definition of ONE variable. This is extremely rare but
																	// standard and compilers allow this. This is why the type of this field is statement,
																	// not an expression.
	TCppStatementBase				*m_reinit_expression;			// This is either an empty statement or a non empty expression. Since this class library
																	// does not have empty expression class (only an empty statement is available), the type
																	// of this field is statement, not an expression.
	TCppStatementBase				*m_body_statement;				// Body of the loop. This is real or empty statement.

public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppWhileStatement : public TCppStatementBase
{
	TCppWhileStatement(ID id = 0) : TCppStatementBase(cxx_while_stmt, id) { m_while_condition = m_body_statement = NULL; }
	~TCppWhileStatement() { if (m_while_condition != NULL) delete m_while_condition; if (m_body_statement != NULL) delete m_body_statement; }

	TCppStatementBase				*m_while_condition;				// Note that condition may contain definition of ONE variable. This is why the type
																	// of this field is statement, not an expression.
	TCppStatementBase				*m_body_statement;				// Body of the loop.

public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppDoWhileStatement : public TCppStatementBase
{
	TCppDoWhileStatement(ID id = 0) : TCppStatementBase(cxx_do_while_stmt, id) { m_body_statement = NULL; m_do_while_expression = NULL; }
	~TCppDoWhileStatement() { if (m_body_statement != NULL) delete m_body_statement; if (m_do_while_expression != NULL) delete m_do_while_expression; }

	TCppStatementBase				*m_body_statement;
	TCppExpressionBase				*m_do_while_expression;

public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppGotoStatement : public TCppStatementBase
{
	TCppGotoStatement(ID id = 0) : TCppStatementBase(cxx_goto_stmt, id) { m_label = NULL; }

	TCppLabelDefn					*m_label;						// The pointer to the label is not owned. Each label always has its own definition.
																	// When the GOTO statement jumps to an undefined label, the compiler places this
																	// label at the end of the function and issues an error message.
};

struct TCppBreakStatement : public TCppStatementBase
{
	TCppBreakStatement(ID id = 0) : TCppStatementBase(cxx_break_stmt, id) { }
};

struct TCppContinueStatement : public TCppStatementBase
{
	TCppContinueStatement(ID id = 0) : TCppStatementBase(cxx_continue_stmt, id) { }
};

struct TCppReturnStatement : public TCppStatementBase
{
	TCppReturnStatement(ID id = 0) : TCppStatementBase(cxx_return_stmt, id) { m_return_value = NULL; }
	~TCppReturnStatement() { if (m_return_value != NULL) delete m_return_value; }

	TCppExpressionBase				*m_return_value;				// This is an expression that computes a return value.

public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = m_return_value; }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_curr_child = NULL; }
};

// ---------------------------------------------------------------------------
//  - - (Stmts2) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ---------------------------------------------------------------------------

struct TCppBlockDefn : public TCppDefnBase
{
	TCppBlockDefn(ID id = 0) : TCppDefnBase(cxx_block, id) { }
	~TCppBlockDefn() { m_members.DeleteAll(); }

	TList								m_members;
	TCppDefnsTable<16, 8>			m_defns_table;

	TList								m_assoc_defn_spaces;			// This list contains instances of TCppAssocDefnsSpaceRef and
																	// it is not owning its members.
public:

	virtual bool	IsBlock() const { return(TRUE); }
	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1) { AddToMembersListAndDefnsSpace(item, m_members, m_defns_table); }
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

	virtual TList	*GetNestedMembersList() const { return((TList*)&m_members); }

protected:

	virtual TCppDefnsSpace		*GetNestedDefnsSpace() { return(&m_defns_table); }
	virtual TList				*GetAssocDefnsSpaceRefsList() { return(&m_assoc_defn_spaces); }
	virtual void				AddToNestedDefnsSpace(TCppDefnBase *defn_item) { m_defns_table.AddRegularDefinition(defn_item); }
	virtual void				AddToNestedDefnsSpaceSpecial(TCppDefnBase *defn_item) { m_defns_table.AddSpecialDefinition(defn_item); }

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_members.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_members.GetNext(iter.m_curr_child); }

	friend class TCppParser;
};

struct TCppTryBlockDefn : public TCppBlockDefn
{
	TCppTryBlockDefn(ID id = 0) : TCppBlockDefn(id) { m_cib_type = cxx_try_block; }
			// The only difference between the simple block and the TRY block is the type of the object.
};

struct TCppCatchBlockDefn : public TCppBlockDefn
{
	TCppCatchBlockDefn(ID id = 0) : TCppBlockDefn(id) { m_cib_type = cxx_catch_block; m_exception = NULL; }

	TCppDataFieldDefn					*m_exception;					// This pointer points into the list of members of the TCppBlockDefn class.
																	// The object under this pointer is not owned.
};

struct TCppLightBlockDefn : public TCppStatementBase
{
	TCppLightBlockDefn(ID id = 0) : TCppStatementBase(cxx_light_block, id) { }
	~TCppLightBlockDefn() { m_members.DeleteAll(); }

	TList								m_members;

public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
					// Note that child item can be either appended or prepended depending on the value of the destination param.

	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_members.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_members.GetNext(iter.m_curr_child); }
};

struct TCppLabelDefn : public TCppDefnBase
{
	TCppLabelDefn(ID id = 0) : TCppDefnBase(cxx_label, id) { m_defined = FALSE; }
			// Note that names of the labels constitute their own space of names inside each function body. Cpp standard
			// explicitly states that it is ok to create variables and labels with the same names in the same context.
			// Parser guarantees that all labels in the function have different names. In the case of the names collision
			// a label with an alternative name is created.

	bool								m_defined;
};

struct TCppCaseLabelDefn : public TCppDefnBase
{
	TCppCaseLabelDefn(ID id = 0) : TCppDefnBase(cxx_case_label, id) { m_default = FALSE; m_const_value = FALSE; m_value = 0; m_value_expr = NULL; }
	~TCppCaseLabelDefn() { if (m_value_expr != NULL) delete m_value_expr; }
			// Parser does not guarantee that case labels in the structured code will have different values. In case of
			// the case lable values duplication or when case label stays outside of the SWITCH statement, the parser
			// generates syntax error and places the case label whereever it is with its value as specified.

	bool								m_default;

	bool								m_const_value;					// When the value of this field is TRUE, this means that the value of the label
																	// was expressed using constant and numeric expression. This also means that
																	// the m_value field is meaningful.
	__int64							m_value;

	TCppExpressionBase				*m_value_expr;					// Expression that was used to specify the value. When case label is used in
																	// the template code and expression is not constant no error is generated.
public:

	virtual bool	CanHaveChildren() const { return(TRUE); }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);				//	Possible destination: cstmt_chdest_default.
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = m_value_expr; }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_curr_child = NULL; }
};

// ---------------------------------------------------------------------------
//  - - (Exprs1) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ---------------------------------------------------------------------------

enum TCppExprChildDest
{
	cexpr_chdest_none,

	cexpr_chdest_unary_oprnd,
	cexpr_chdest_binary_oprnd1,
	cexpr_chdest_binary_oprnd2,
	cexpr_chdest_multiop_prepend,
	cexpr_chdest_multiop_append,
	cexpr_chdest_assgn_oprnd1,
	cexpr_chdest_assgn_oprnd2,

	cexpr_chdest_throw_oprnd,
	cexpr_chdest_cond_oprnd1,
	cexpr_chdest_cond_oprnd2,
	cexpr_chdest_cond_oprnd3,

	cexpr_chdest_fld_acc_oprnd,
	cexpr_chdest_fp_acc_oprnd1,
	cexpr_chdest_fp_acc_oprnd2,

	cexpr_chdest_index_array,
	cexpr_chdest_index_params,
	cexpr_chdest_fcall_func,
	cexpr_chdest_fcall_params,

	cexpr_chdest_cast_oprnd,
	cexpr_chdest_sizeof_oprnd,
	cexpr_chdest_new_plc_params,
	cexpr_chdest_new_init_params,
	cexpr_chdest_new_ctor_call,
	cexpr_chdest_delete_oprnd,
	cexpr_chdest_typeid_oprnd,
	cexpr_chdest_typeid_dyn_code,

	cexpr_chdest_default = -1,
};

enum TCppExprOperandType
{
	eopr_none,

	eopr_bool_literal,
	eopr_num_literal,
	eopr_charconst_literal,
	eopr_string_literal,

	eopr_enum_literal,
	eopr_this,

	eopr_data_field,					// Data field that can range from a simple integer to a complex class.
	eopr_code_entry_point,			// Function can range from a simple function to a user defined typecast.

	eopr_ovld_defns_bucket,			// Bucket of overloaded definitions. This is a transient operand type that
									// can exist only during the parsing process.
	eopr_num_types,
};

struct TCppOperandExpr : public TCppExpressionBase
{
	TCppOperandExpr(ID id = 0) : TCppExpressionBase(cxx_operand_expr, id) { m_oprnd_type = eopr_none; }
			// Operand expression cannot have children and none of its pointers is owned.

	void			Clear() { m_oprnd_type = eopr_none; }

	void			SetBoolLiteral(bool bool_value) { m_oprnd_type = eopr_bool_literal; m_bool_value = bool_value; }
	void			SetNumericLiteral(TLexNumberType subt, __int64 value) { m_oprnd_type = eopr_num_literal; m_num_subt = subt; m_num_value = value; }
	void			SetCharConstLiteral(TLexCharConstType subt, __int64 value) { m_oprnd_type = eopr_charconst_literal; m_charconst_subt = subt; m_charconst_value = value; }
	void			SetStringLiteral(TLexStringType subt, const TStrPtrInfo &value) { m_oprnd_type = eopr_string_literal; m_string_subt = subt; m_string_value = value; }
	void			SetEnumLiteral(TCppEnumMemberDefn *enum_member) { m_oprnd_type = eopr_enum_literal; m_enum_member_ref = enum_member; }

	void			SetDataField(TCppDataFieldDefn *data_field) { m_oprnd_type = eopr_data_field; m_data_field_ref = data_field; if (data_field != NULL) m_result_type = data_field->m_field_type; }
	void			SetCodeEntryPoint(TCppCodeEntryPointDefn *code_entry_point) { m_oprnd_type = eopr_code_entry_point; m_code_entry_point_ref = code_entry_point; if (code_entry_point != NULL) m_result_type = code_entry_point->m_return_value_type; }

	bool			GetConstBoolValue(bool &bool_value) const;

	bool			ConvertToNumericLiteral();
					// In case of successful conversion the subtype of the object is always eopr_num_literal.

	virtual bool	GetConstOperandValue(TCppOperandExpr &expr_value) const;

	virtual bool	CanHaveChildren() const { return(FALSE); }

	const wchar_t			*GetTextRepresentation(wchar_t *buffer, int buffer_len, bool want_ref_num);
	static const wchar_t	*GetOperandTypeEnumName(TCppExprOperandType oprnd_type);

public:

	TCppExprOperandType		m_oprnd_type;

	union
	{
		struct
		{
			bool					m_bool_value;
		};

		struct
		{
			TLexNumberType		m_num_subt;
			__int64				m_num_value;
		};

		struct
		{
			TLexCharConstType	m_charconst_subt;
			__int64				m_charconst_value;
		};

		struct
		{
			TLexStringType		m_string_subt;
			TStrPtrInfo			m_string_value;						// The string is not owned by the object. Pointer in this field always points
																	// to the non NULL terminated unicode string. Length of the string should be
																	// determined using the length field. There is no other way.
		};

		TCppEnumMemberDefn		*m_enum_member_ref;				// The pointer is not owned.
		TCppDataFieldDefn			*m_data_field_ref;					// The pointer is not owned.
		TCppCodeEntryPointDefn	*m_code_entry_point_ref;			// The pointer is not owned.

		struct
		{
			TCppDefnBase		*m_ovld_defn_sample;				// The pointer is not owned.
			TCppDefnsBucket		*m_ovld_defns_bucket;				// The pointer is not owned.
		};
	};
};

//
//  Unary expression can be (10 possible operation codes): ++op, --op, *op, &op, +op, -op, !op, ~op, op++, op--.
//
struct TCppUnaryExpr : public TCppExpressionBase
{
	TCppUnaryExpr(ID id = 0) : TCppExpressionBase(cxx_unary_expr, id) { m_operand = NULL; }
	~TCppUnaryExpr() { if (m_operand != NULL) delete m_operand; }

	TCppExpressionBase			*m_operand;

public:

	void			SetOperation(TOperatorToken opr);

	virtual bool	GetConstOperandValue(TCppOperandExpr &expr_value) const;

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = m_operand; }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_curr_child = NULL; }
};

//
//  Binary expression can be (11 possible operation codes): ==, !=, <, >, <=, >=, <<, >>, -, /, %.
//
struct TCppBinaryExpr : public TCppExpressionBase
{
	TCppBinaryExpr(ID id = 0) : TCppExpressionBase(cxx_binary_expr, id) { m_operand1 = m_operand2 = NULL; }
	~TCppBinaryExpr() { if (m_operand1 != NULL) delete m_operand1; if (m_operand2 != NULL) delete m_operand2; }

	TCppExpressionBase			*m_operand1;
	TCppExpressionBase			*m_operand2;

public:

	void			SetOperation(TOperatorToken opr);

	virtual bool	GetConstOperandValue(TCppOperandExpr &expr_value) const;

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

//
//  Multi operation expression can be (8 possible  operation codes): ',' (comma), &&, ||, &, |, ^, +, *.
//
//  Note that in rare cases expression with comma operation may have empty list of operands and this situation is legal.
//
struct TCppMultiOpExpr : public TCppExpressionBase
{
	TCppMultiOpExpr(ID id = 0) : TCppExpressionBase(cxx_multiop_expr, id) { }
	~TCppMultiOpExpr() { m_operands.DeleteAll(); }

	TList							m_operands;

public:

	void			SetOperation(TOperatorToken opr);

	virtual bool	GetConstOperandValue(TCppOperandExpr &expr_value) const;

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = (TCppItemBase*)m_operands.GetFirst(); }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { if (iter.m_curr_child != NULL) iter.m_curr_child = (TCppItemBase*)m_operands.GetNext(iter.m_curr_child); }
};

//
//  Assignment expression can be (11 possible operation codes): =, +=, -=, *=, /=, %=, <<=, >>=, &=, |=, ^=.
//
struct TCppAssignmentExpr : public TCppExpressionBase
{
	TCppAssignmentExpr(ID id = 0) : TCppExpressionBase(cxx_assignment_expr, id) { m_operand1 = m_operand2 = NULL; }
	~TCppAssignmentExpr() { if (m_operand1 != NULL) delete m_operand1; if (m_operand2 != NULL) delete m_operand2; }

	TCppExpressionBase			*m_operand1;
	TCppExpressionBase			*m_operand2;

public:

	void			SetOperation(TOperatorToken opr);

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

// ---------------------------------------------------------------------------
//  - - (Exprs2) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ---------------------------------------------------------------------------

struct TCppThrowExpr : public TCppExpressionBase
{
	TCppThrowExpr(ID id = 0) : TCppExpressionBase(cxx_throw_expr, id) { m_operation = opr_throw; m_operand = NULL; }
	~TCppThrowExpr() { if (m_operand != NULL) delete m_operand; }

	TCppExpressionBase			*m_operand;						// Note that operand can be NULL. This is legal case, but such expression
																	// may occur only in the context of the catch block.
public:

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = m_operand; }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_curr_child = NULL; }
};

struct TCppConditionalExpr : public TCppExpressionBase
{
	TCppConditionalExpr(ID id = 0) : TCppExpressionBase(cxx_conditional_expr, id) { m_operation = opr_question; m_operand1 = m_operand2 = m_operand3 = NULL; }
	~TCppConditionalExpr() { if (m_operand1 != NULL) delete m_operand1; if (m_operand2 != NULL) delete m_operand2; if (m_operand3 != NULL) delete m_operand3; }

	TCppExpressionBase			*m_operand1;
	TCppExpressionBase			*m_operand2;
	TCppExpressionBase			*m_operand3;

public:

	virtual bool	GetConstOperandValue(TCppOperandExpr &expr_value) const;

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppFieldAccessExpr : public TCppExpressionBase
{
	TCppFieldAccessExpr(ID id = 0) : TCppExpressionBase(cxx_field_access_expr, id) { m_struct_operand = NULL; m_struct_member_defn = NULL; }
	~TCppFieldAccessExpr() { if (m_struct_operand != NULL) delete m_struct_operand; }
			// This class supports "dot" and "arrow" operations.

	TCppExpressionBase			*m_struct_operand;
	TCppDefnBase				*m_struct_member_defn;			// The pointer is not owned.

public:

	void			SetOperation(TOperatorToken opr) { assert(opr == 0 || opr == opr_none || opr == opr_dot || opr == opr_arrow); m_operation = opr; }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = m_struct_operand; }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_curr_child = NULL; }
};

struct TCppFieldAccessPtrExpr : public TCppExpressionBase
{
	TCppFieldAccessPtrExpr(ID id = 0) : TCppExpressionBase(cxx_field_access_ptr_expr, id) { m_struct_operand = NULL; m_struct_member_ptr = NULL; }
	~TCppFieldAccessPtrExpr() { if (m_struct_operand != NULL) delete m_struct_operand; if (m_struct_member_ptr != NULL) delete m_struct_member_ptr; }
			// This class supports "dot-star" and "arrow-star" operations.

	TCppExpressionBase			*m_struct_operand;
	TCppExpressionBase			*m_struct_member_ptr;				// The pointer is an owned expression.
																	// Both data fields are children of the current expr.
public:

	void			SetOperation(TOperatorToken opr) { assert(opr == 0 || opr == opr_none || opr == opr_dotstar || opr == opr_arrowstar); m_operation = opr; }

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppArrayIndexExpr : public TCppExpressionBase
{
	TCppArrayIndexExpr(ID id = 0) : TCppExpressionBase(cxx_array_index_expr, id) { m_operation = opr_arr_index; m_array_operand = NULL; }
	~TCppArrayIndexExpr() { if (m_array_operand != NULL) delete m_array_operand; m_index_expressions.DeleteAll(); }

	TCppExpressionBase			*m_array_operand;
	TList							m_index_expressions;

public:

	virtual bool	GetConstOperandValue(TCppOperandExpr &expr_value) const;

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppFunctionCallExpr : public TCppExpressionBase
{
	TCppFunctionCallExpr(ID id = 0) : TCppExpressionBase(cxx_function_call_expr, id) { m_operation = opr_funcall; m_function = NULL; }
	~TCppFunctionCallExpr() { if (m_function != NULL) delete m_function; m_parameters.DeleteAll(); }

	TCppExpressionBase			*m_function;						// When a simple function is called, the function being called is expressed using
																	// the TCppOperandExpr object. In other cases this can be a more complex expresssion.
																	// Note that figuring out the value of the "this" param in the call of the member function
																	// may be tricky. This requires studying of the "m_function" data field that should contain
																	// either a FieldAccess or a FieldAccessPtr node somewhere in the subtree.
	TList							m_parameters;						// This is the list of the parameters of the call. This list can be empty.

public:

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);			// Possible dests: cexpr_chdest_fcall_func or cexpr_chdest_fcall_params.
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

	void			AddCallParameters(TCppExpressionBase *params_expr);
					// When the passed expression is a comma expr, its subexpressions are added as separate
					// parameters. Otherwise the whole passed expr is added as one parameter.
protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

//
//  Cast expression can have the following variants: (TypeId)operand, const_cast, dyna_cast, rein_cast, static_cast.
//
//  In the moment parser implements the cast operation only partially. It assumes that any type can be casted to any
//  data type. Parser is not generating syntax errors unless the destination type is data type and not a function type.
//  It is up the code generator/interpreter to decide if error messages are needed or not and how exactly the casting
//  should be implemented.
//
//  The result type is always statically known.
//
struct TCppCastExpr : public TCppExpressionBase
{
	TCppCastExpr(ID id = 0) : TCppExpressionBase(cxx_cast_expr, id) { m_dest_type = NULL; m_operand = NULL; }
	~TCppCastExpr() { if (m_operand != NULL) delete m_operand; }

	TCppDataTypeBase			*m_dest_type;						// This type pointer is just referenced. It is not owned.
	TCppExpressionBase			*m_operand;

public:

	void			SetOperation(TOperatorToken opr) { assert(opr == 0 || opr == opr_none || opr == opr_cast || opr == opr_const_cast || opr == opr_dyna_cast || opr == opr_rein_cast || opr == opr_static_cast); m_operation = opr; }

	virtual bool	GetConstOperandValue(TCppOperandExpr &expr_value) const;

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = m_operand; }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_curr_child = NULL; }
};

struct TCppSizeofExpr : public TCppExpressionBase
{
	TCppSizeofExpr(ID id = 0) : TCppExpressionBase(cxx_sizeof_expr, id) { m_operation = opr_sizeof; m_expr_operand = FALSE; m_data_type = NULL; m_sizeof_value = 0; }
	~TCppSizeofExpr() { if (m_expr_operand == TRUE && m_operand != NULL) delete m_operand; }
		//
		// When the SIZEOF operation is processed, parser searches for the "size_t" type. Parser expects that
		// this type should be defined before the first use of the SIZEOF operator. This type should be one of
		// the integral types. Definition of this type is the ony one way to tell to the parser what type should be
		// used for handling sizes and as underlying type for pointers. This type is used as the result type of
		// this operation.
		//

	bool							m_expr_operand;

	union
	{
		TCppDataTypeBase		*m_data_type;						// Pointer is not owned.
		TCppExpressionBase		*m_operand;						// This is an owned child.
	};

	__int64						m_sizeof_value;						// Note that size of the object can be zero.

public:

	virtual bool	GetConstOperandValue(TCppOperandExpr &expr_value) const;

	void			SetDataTypeOperand(TCppItemBase *item);
					// This method should be called only when the operand of the sizeof operator is DATA TYPE.

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
					// This method should be called ONLY when the operand is an expression.

	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppNewExpr : public TCppExpressionBase
{
	TCppNewExpr(ID id = 0) : TCppExpressionBase(cxx_new_expr, id) { m_operation = opr_new; m_global_new = FALSE; m_new_type = NULL; m_ctor_call_expr = NULL; }
	~TCppNewExpr() { m_new_placement_exprs.DeleteAll(); m_init_param_exprs.DeleteAll(); if (m_ctor_call_expr != NULL) delete m_ctor_call_expr; }

	bool							m_global_new;
	TList							m_new_placement_exprs;			// The list can be empty.
	TCppDataTypeBase			*m_new_type;						// The pointer is not owned.
	TList							m_init_param_exprs;				// The list can be empty.
	TCppFunctionCallExpr			*m_ctor_call_expr;					// This is a child expression.

public:

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

struct TCppDeleteExpr : public TCppExpressionBase
{
	TCppDeleteExpr(ID id = 0) : TCppExpressionBase(cxx_delete_expr, id) { m_operation = opr_delete; m_global_delete = m_array_delete = FALSE; m_operand = NULL; }
	~TCppDeleteExpr() { if (m_operand != NULL) delete m_operand; }

	bool							m_global_delete;
	bool							m_array_delete;
	TCppExpressionBase			*m_operand;

public:

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_special_count = 0; iter.m_curr_child = m_operand; }
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const { iter.m_special_count = 0; iter.m_curr_child = curr_chld; }
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const { iter.m_curr_child = NULL; }
};

struct TCppTypeidExpr : public TCppExpressionBase
{
	TCppTypeidExpr(ID id = 0) : TCppExpressionBase(cxx_typeid_expr, id) { m_operation = opr_typeid; m_expr_operand = FALSE; m_dynamic_result = FALSE; m_data_type = NULL; m_static_type_struct = NULL; }
	~TCppTypeidExpr() { if (m_expr_operand == TRUE && m_operand != NULL) delete m_operand; if (m_dynamic_result == TRUE && m_type_resolution_code != NULL) delete m_type_resolution_code; }
		//
		// When the TYPEID operation is processed, parser searches for std::type_info. Parser expects that
		// this structure should be defined before the first use of the TYPEID operator. This structure is used
		// as the type of the result of the expression.
		//

	bool							m_expr_operand;
	bool							m_dynamic_result;

	union
	{
		TCppDataTypeBase		*m_data_type;						// The pointer is not owned.
		TCppExpressionBase		*m_operand;						// This is an owned child.
	};

	union
	{
		TCppDataFieldDefn			*m_static_type_struct;				// The pointer is not owned.
		TCppItemBase			*m_type_resolution_code;			// This is an owned child.
										//
										// Core parser is placing NULLs in these fields. The callback handler from the TCppObjectsFactory
										// may assign appropriate non NULL values. This design was selected because C++ standard is not
										// defining constuctors and/or data fields in the std::type_info structure
	};									//

public:

	void			SetDataTypeOperand(TCppItemBase *item);
					// This method should be called only when the operand of the typeid operator is DATA TYPE.

	void			SetStaticTypeStruct(TCppItemBase *item);
					// Parameter of this method is not becoming an owned child ob the current object.

	virtual void	AddChildItem(TCppItemBase *item, int item_destination = -1);
					// This method should be called ONLY when the operand is an expression.

	virtual void	NewIndirectChildNotification(TCppItemBase *items_subtree_root) { if (m_cib_parent != NULL) m_cib_parent->NewIndirectChildNotification(items_subtree_root); }

protected:

	virtual void	PrepareChildrenIter(TCppDirectChildrenIter &iter) const;
	virtual void	PrepareChildrenIterEx(TCppDirectChildrenIter &iter, TCppItemBase *curr_chld) const;
	virtual void	ShiftChildrenIter(TCppDirectChildrenIter &iter) const;
};

//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline bool TCppItemBase::CompareSameDatabaseDataTypes(TCppDataTypeBase *type1, TCppDataTypeBase *type2)
{
	return(CompareTypesInternal(type1, NULL, 0, type2, NULL, 0));
}

inline bool TCppCvPtrRefSpec::Compare(TCppCvPtrRefSpec &inst)
{
	// Compare the flags first.
	if (cvpr_flags != inst.cvpr_flags)
		return(FALSE);

	if (cvpr_struct == NULL && inst.cvpr_struct == NULL)
	{
		// Both struct pointers are NULL. Specs are identical.
		return(TRUE);
	}

	if (cvpr_struct == NULL || inst.cvpr_struct == NULL)
	{
		// The check above showed that at least one struct pointer is not NULL. Current check
		// shows that one of the pointers is NULL. This means that the specs are different.
		return(FALSE);
	}

	// Both pointers are not NULL. Compare the struct definitions.
	return(TCppItemBase::CompareSameDatabaseDataTypes(cvpr_struct, inst.cvpr_struct) == TRUE);
}

inline TCppDataFieldDefn *TCppDirectChildrenIter::CurrItemAsDataField(TCppStorageClass strgc) const
{
	if (m_curr_child->ItemType() != cxx_data_field)
		return(NULL);

	TCppDataFieldDefn *data_field = (TCppDataFieldDefn*)m_curr_child;
	return ((data_field->m_strg_class == strgc) ? data_field : NULL);
}

inline TCppCodeEntryPointDefn *TCppDirectChildrenIter::CurrItemAsCodeEntryPoint(bool static_method) const
{
	if (m_curr_child->ItemType() != cxx_code_entry_point)
		return(NULL);

	TCppCodeEntryPointDefn *entry_point = (TCppCodeEntryPointDefn*)m_curr_child;
	return ((entry_point->IsStaticMethod() == static_method) ? entry_point : NULL);
}

inline TCppOperandExpr *TCppDirectChildrenIter::CurrItemAsOperand(TCppExprOperandType oprndt) const
{
	if (m_curr_child->ItemType() != cxx_operand_expr)
		return(NULL);

	TCppOperandExpr *oprnd_expr = (TCppOperandExpr*)m_curr_child;
	return ((oprnd_expr->m_oprnd_type == oprndt) ? oprnd_expr : NULL);
}

inline void TCppChildrenSubtreeIter::operator++()
{
	if (IsComplete() == TRUE)
		return;

	if (m_block_children == TRUE)
	{
		// For some reason application does not want to iterate children of the current item.
		m_block_children = FALSE;
	}
	else
	{
		// Step 1. Check if current item can have children and really has them.
		if (m_curr_layer_iter.CurrItem().CanHaveChildren() == TRUE)
		{
			// Class of the current item allows having children.
			TCppDirectChildrenIter curr_layer_iter_copy = m_curr_layer_iter;
			m_curr_layer_iter.m_item = &m_curr_layer_iter.CurrItem();
			m_curr_layer_iter.SeekToFirst();
			if (m_curr_layer_iter == TRUE)
				return;

			// Current item does not have children to iterate.
			m_curr_layer_iter = curr_layer_iter_copy;
		}
	}

	// Loop on right shifts and steps up.
	for (;;)
	{
		// Step 2. Check if it is possible to shift iterator on the current layer.
		m_curr_layer_iter.ShiftDown();
		if (m_curr_layer_iter == TRUE)
			return;

		// Step 3. Check the current iteration layer.
		if (m_curr_layer_iter.m_item == m_subtree_root)
			return;

		// Step 4. Step up the hierarchy.
		TCppItemBase *item = m_curr_layer_iter.m_item;
		assert(item != NULL && item->ItemParent() != NULL);

		m_curr_layer_iter.m_item = item->ItemParent();
		m_curr_layer_iter.SeekToItem(item);
	}
}

class TCppObjectsFactory
{
public:

	virtual	TCppItemBase	*AllocateCppObject(TCppItemType cxt, ID id = 0);
								// This method should simply allocate object of the required class. Returned objects should be deletable
								// using simple operator delete. Global namespace object should not be created using this method.

	virtual	void		FinalizeTypeidOperationObject(TCppTypeidExpr *expr_node) { }
						// This method is called by the parser at the end of processing the TYPEID operator. It allows application
						// either to create a static object that describes the type of the operand or to add code that determines
						// the type of the operand dynamically.

	virtual	void		CppNamespaceNotification(TCppNamespaceDefn *object, bool scanning_complete) { }
	virtual	void		CppClassNotification(TCppStructTypeDefn *object, bool scanning_complete) { }
	virtual	void		CppFunctionNotification(TCppCodeEntryPointDefn *object, bool scanning_complete) { }
	virtual	void		CppBlockNotification(TCppCodeEntryPointDefn *object, bool scanning_complete) { }
						// These methods notify about entering and exiting the definition nesting layers.

	virtual	void		CppEnumNotification(TCppEnumTypeDefn *object) { }
	virtual	void		CppDataTypedefNotification(TCppDataTypeBase *object) { }
	virtual	void		CppFunctionTypedefNotification(TCppFunctionTypeBase *object) { }
	virtual	void		CppDataFieldNotification(TCppDataFieldDefn *object) { }
	virtual	void		CppFunctionPrototypeNotification(TCppCodeEntryPointDefn *object) { }
						// These methods notify about completeness of the major types of objects.

	virtual	void		CppObjectReadingComplete(TCppItemBase *object, BYTE *app_data = NULL, int app_data_len = 0) { }
						// This method should eb called by the CPP database reader when all system fields of the passed object
						// are already set. Implementation can finish buiding its application level fields of the passed object.
};

typedef TDataBuffersArray<128, 128> TCppDatabaseBuffersArray;

class TCppDatabase : public THighLevelDatabase
{
public:

		TCppDatabase(ID parsing_id = 0);
			// This class does not need a destructor because all its data members have their own destructors.

	virtual THighLevelLangType	GetDatabaseLangType() const { return(lang_cpp); }

	TCppGlobalNamespaceDefn	&GetGlobalNamespace() { return(m_global_namesp); }

	void		ClearDatabase();

	int		NumDatabaseItems() const { return(m_global_namesp.NumSubtreeItems()-1); }
	int		NumGlobalNamespaceItems() const { return(m_global_namesp.NumDirectChildren()); }
				// Note that the global namespace itself is not included into both counts of objects.

	TCppItemBase *GetCppObjectPtr(ID cpp_object_id)
	{
		if (cpp_object_id >= bltin_ids_base)
		{
			// The passed id belongs to the range of the special item ids.
			int offs = cpp_object_id-bltin_ids_base;
			if (offs < (bltin_ids_max_plus1-bltin_ids_base))
				return(m_built_in_cpp_object_ptrs[offs]);
		}
		else if (cpp_object_id >= CPP_OBJECT_IDS_BASE)
		{
			// This is id of an ordinary item.
			int offs = cpp_object_id-CPP_OBJECT_IDS_BASE;
			if (offs < m_cpp_object_ptrs.NumItems())
				return(m_cpp_object_ptrs[cpp_object_id-CPP_OBJECT_IDS_BASE]);
		}

		// Passed id does not belong to any range of the valid id values.
		assert(FALSE);
		return(NULL);
	}

public:

	enum { CPP_OBJECT_IDS_BASE = 100 };

	inline ID		GetNextItemId() { return(m_next_object_ident++); }
	inline void	SetupItemId(TCppItemBase *item) { assert(item->m_cib_id == 0); item->m_cib_id = m_next_object_ident++; }

	bool			PrepareAndSetBitFieldTypeName(TCppBitFieldTypeDefn *bft_object, bool named_base_type);
	bool			PrepareAndSetCvPtrRefTypeName(TCppCvPtrRefTypeDefn *cvpr_object, bool named_base_type);
	bool			PrepareAndSetArrayTypeName(TCppArrayTypeDefn *array_object, bool named_base_type);
	bool			PrepareAndSetUnnamedFuncTypeName(TCppFunctionTypeDefn *func_type_object, bool named_base_type);
	bool			PrepareAndSetCaseLabelName(TCppCaseLabelDefn *case_lab_object);
	bool			PrepareAndSetParserGeneratedName(TCppDefnBase *defn_object, const wchar_t *name_proto);
					// These methods expect that all necessary properties in the passed objects are already set.
					// Based on these props these methods generate an set an appropriate name for the object.

	wchar_t		*CreateDestructorName(const wchar_t *class_name);
	wchar_t		*CreateTemplateIdName(const wchar_t *template_name, TList *template_arguments = NULL);
	wchar_t		*CreateOverloadedOperatorName(TOperatorToken operation, bool op_template_form = FALSE, TList *template_arguments = NULL);
	wchar_t		*CreateOverloadedConversionName(TCppDataTypeBase *dest_type, bool conv_template_form = FALSE, TList *template_arguments = NULL);

	wchar_t		*CreateAlternativeName(const wchar_t *name_proto, ID name_suffix);
	wchar_t		*CreateAlternativeNameEx(const wchar_t *name_prefix, const wchar_t *name_proto, ID name_suffix);
	wchar_t		*CreateUnresolvedObjectName(const wchar_t *name_prefix, TList *unres_name_infos, const wchar_t *name_proto, ID name_suffix);

	static const wchar_t	*GenerateQualifiedName(TTextBuffer256 &name_buffer, TCppDefnBase *defn, bool want_buffer_overflow_subst = TRUE);
	static const wchar_t	*GetBaseTypeName(const TCppDefnBase *base_type_defn, wchar_t *buff40, bool named_base_type);
	static const wchar_t	*GetStructTypeName(const TCppDataTypeBase *struct_type_defn);
	static const wchar_t	*GetFuncParamTypeName(const TCppDataTypeBase *param_type_defn);
	static const wchar_t	*GetTemplateTypeArgumentName(const TCppDataTypeBase *template_type_argument);
	static const wchar_t	*GetTemplateTemplateArgumentName(const wchar_t *argument_name);

	static const wchar_t	*GetConstVolatileFlagsTextRepr(const TCppCvPtrRefSpec &spec);
	static const wchar_t	*GetDeclTypeSpecPrefixFriendlyName(TCppDeclTypeSpecPrefix pfx);
	static const wchar_t	*GetFunctionAttributeFriendlyName(TCppFunctionAttr attr);
	static const wchar_t	*GetFuncAttrsMaskFriendlyName(wchar_t *buffer256, TCppFunctionAttr set_of_attrs);

protected:

	void			ProcessNewChildNotification(TCppItemBase *items_subtree_root);

	void			AppendTemplateArguments(TTextBuffer256 &name_buffer, TList *args_list);
	void			ChangeSpacesToUnderscores(TTextBuffer256 &name_buffer);
	bool			EnlargeCppNameBuffer(TCppDefnBase *defn, TTextBuffer256 &buffer, int extra_wchars_needed);
	bool			SetupPersistentName(TCppDefnBase *defn, const wchar_t *non_persistent_name);

protected:

	TCppGlobalNamespaceDefn				m_global_namesp;
											// Database always owns all its objects. Note that this object is created without using the cpp objects
											// factory and the ID of this object is zero.

	TStringsDict							m_global_strings_dict;
											// Storage for various strings, primarily for the names of the variables.

	ID									m_next_object_ident;
											// This counter is used for assigning ids for new objects during the parsing process. When the database
											// is retrieved from the persistent storage, this counter remains zero.

	TCppItemPtrsArray						m_cpp_object_ptrs;
											// Array of pointers to objects in the database. All objects of the database except for the built in
											// types are present in this index. Checking this index is the valid way to figure out if object belongs
											// to the database or not. Note that some slots in this array can be NULL. Arrays is not owning its
											// pointers. Objects belong to their parents.

	TCppItemBase						*m_built_in_cpp_object_ptrs[bltin_ids_max_plus1-bltin_ids_base];
											// Array of pointers to the built in types. Note that some slots in this array can be NULL. This array
											// is not owning its pointers. Built in types always belong to the global namespace.

	bool									m_cpp_object_ptrs_expansion_error;
	bool									m_cpp_object_ptrs_double_pub_error;
											// Updating the list of pointers is done inside the void callback. Allocation errors may still happen there.
											// These fields keep the count of such errors.

	friend class TCppDirectChildrenIter;
	friend struct TCppGlobalNamespaceDefn;
	friend struct TCppNameInfo;
	friend class TCppParser;
};

class TCppDataStorage : public THighLevelDataStorage
{
public:
			TCppDataStorage(bool single_sess_storage = FALSE)
					: m_project_database(TProjectParsSessId) { m_single_pars_sess_storage = single_sess_storage; }

	TCppDatabase		&GetProjectDatabase() { return(m_project_database); }

	virtual THighLevelDatabase	*AllocateParsSessDatabase(ID parsing_id);

protected:

	TCppDatabase		m_project_database;
	bool					m_single_pars_sess_storage;
};

#endif	// TextScan_CppDatabase_H


