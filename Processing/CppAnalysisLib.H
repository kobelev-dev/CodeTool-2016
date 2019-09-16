//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Conditional parsing structure analysis.
//

#ifndef	Processing_CppAnalysisLib_H
#define	Processing_CppAnalysisLib_H

#ifndef   LangView_LexItems_H
#include  "LangView/LexItems.H"
#endif
#ifndef   Processing_HtmlReportHelper_H
#include  "Processing/HtmlReportHelper.H"
#endif

class TCppDetectInheritanceHelper
{
public:

	TCppDetectInheritanceHelper(TGenericConsole &cons) : m_can_lib_console(cons) { m_visible_root = NULL; }

	bool		BuildInheritanceTree(TLangViewCppItemTreeNode &visible_objects_root, TCppItemBase &cpp_items_subtree);
				// This method destryes the passed lex items subtree if any.
protected:

	bool		ProcessStructuredType(TCppStructTypeDefn *struct_type);
	bool		AddClassObject(TLangViewCppItemTreeNode *parent_node, TCppStructTypeDefn *struct_type);

	TGenericConsole				&m_can_lib_console;

	TLangViewCppItemTreeNode		*m_visible_root;

	int			m_cnt_unproc_bases;
	int			m_cnt_new_derivatives;
					// Both variables describe state and ressults of one pass.
};

class TOverwrittienMethodsReport : public THtmlReportHelper
{
public:

	TOverwrittienMethodsReport(TGenericConsoleEx &cons) : THtmlReportHelper(cons) { m_classes_hier_root = NULL; }

	bool		GenerateReport(TLangViewCppItemTreeNode *root_node, TCppDatabase &cpp_database, const wchar_t *short_report_name);

	virtual void	WriteHtmlReportBody(THtmlDocument &rprt);

protected:

	bool		ProcessMethodOverrideParents(TCppCodeEntryPointDefnPtrsArray &data);
	bool		ProcessMethodOverrideRoot(TCppCodeEntryPointDefn *entry_point);

	void		GenerateTableHeaderRows(THtmlDocument &rprt);
	void		GenerateReportRowForClass(THtmlDocument &rprt, TCppItemBase *cpp_item, int zero_based_layer);

	typedef TCppCodeEntryPointDefn *TCppCodeEntryPointDefnPtr;
	static int	__cdecl CompareCodeEntryPointPtrs(const TCppCodeEntryPointDefnPtr *ppi1, const TCppCodeEntryPointDefnPtr *ppi2)
	{
		return(TCppItemBase::CompareBasicItemNames((const TCppItemBase**)ppi1, (const TCppItemBase**)ppi2));
	}

	enum { NUM_CLASS_HIER_COLUMNS = 20 };

	TLangViewCppItemTreeNode				*m_classes_hier_root;

	TCppCodeEntryPointDefnPtrsArray		m_method_override_roots;
};

#endif	// Processing_CppAnalysisLib_H


