//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	TextScan_CppParsingResults_H
#define	TextScan_CppParsingResults_H

enum TProjectType
{
	prjt_none,

	prjt_cond_params,				// Analysis of the cond parsing parameters use.
	prjt_cpp_codebase,				// Set of C and/or C++ files.
};

struct TCppParsingTask
{

	TList					m_project_elements;
	TList					m_fname_extension_rules;
	TList					m_command_lines;

	TStringsDict			m_prj_dict;
};

struct TCppParsingResults
{
	TCppParsingTask			m_parsing_task;
	TMidLevRamLogger			m_mid_lev_database;
	TCppDataStorage			m_cpp_data_storage;
};

#endif	// TextScan_CppParsingResults_H


