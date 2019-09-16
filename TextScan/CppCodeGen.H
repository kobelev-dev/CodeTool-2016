//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	TextScan_CppCodeGen_H
#define	TextScan_CppCodeGen_H

#ifndef   TextScan_CppDatabase_H
#include  "TextScan/CppDatabase.H"
#endif

//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef ID TCppCodeChunkId;
				// Code chunk IDs are unique only inside the function. Zero value of the chunk ID is
				// an invalid value. Properly assigned chunk ID is always a positive value.

enum TCppCodeStepType
{
	ccst_expression,						// Expression code steps can originate from expression stmts, loop conditions, etc.
	ccst_pop_stack,						// This code step is used when it is necessary to discard the expression value. This
										// code step type does not have additional details.

	// Code steps of the types below can appear only at the end of the code chunk. Each properly prepared
	// code chunk ends with one of these steps that tell what should be done after executing the code chunk.
	// Code chunk can consist of only one termination step.
	ccst_if,
	ccst_switch,
	ccst_goto,
	ccst_return,							// Return code step does not have any details. Return value of the non void function
										// is located in the stack. Stack of the void function should be empty when the return
										// statement is executed.
};

struct TCppCodeCaseLabelInfo
{
	TCppCaseLabelDefn					*m_case_label_item;				// Case label for comparing with result of the switch expression.
	TCppCodeChunkId						m_case_label_chunk;				// Id of the code chunk to execute in case of the case label match.
};

struct TCppCodeStepInfo
{
	TCppCodeStepType					m_code_step_type;

	TCppItemBase						*m_code_step_cpp_item;			// For expression code steps the value of this field is always not NULL.
																		// For pop stack code steps the value is always NULL. For all other types
																		// of code steps the value may be NULL and may be not NULL.
	union
	{
		// Details for "ccst_if" code step. Note that BOTH chunk ids are always not zero.
		struct
		{
			TCppCodeChunkId				m_if_then_chunk;
			TCppCodeChunkId				m_if_else_chunk;
		};

		// Details for "ccst_switch" code step. List of the case labels can be empty only in case of the syntax errors.
		struct
		{
			TCppCodeCaseLabelInfo		*m_switch_labels_info;			// Array of case labels of the switch statement.
			int							m_switch_num_labels;			// Length of the array of the case labels.
		};

		// Details for "ccst_goto" code step. Chunk id cannot be zero.
		struct
		{
			TCppCodeChunkId				m_goto_chunk;
		};
	};

	void Clear() { memset(this, 0, sizeof(TCppCodeStepInfo)); }
};

struct TCppCodeChunkInfo
{
	TCppCodeStepInfo						*m_chunk_steps;
	int									m_num_steps;

	TCppCodeChunkId						m_next_chunk;					// Id of the chunk that was generated out of the source code that stays
																		// below the source code of the current chunk. Last chunk in the function
																		// has zero value in this field.
};

// Note that this structure can describe only real code. Template code cannot be described using this structure.
struct TCppFunctionCode
{
			TCppFunctionCode() { m_function_defn = NULL; m_code_chunks = NULL; m_num_code_chunks = 0; }

	TCppCodeEntryPointDefn		*GetSrcFunction() { return (m_function_defn); }

	enum { FuncEntryChunkId = 1 };

	int					NumCodeChunks() const { return (m_num_code_chunks); }
	TCppCodeChunkId		ChunkIdFromChunkIndex(int chunk_index) const { return(chunk_index+CodeChunkIdIndexOffs); }

	TCppCodeStepInfo		*GetCodeChunk(TCppCodeChunkId chunk_id, int *len_chunk = NULL);
								// Ownership on the returned pointer still belongs to the structure. Application does not need
								// to release it in any way.

	bool					GenerateBasicReport(TDestinationFile &report_buffer);

protected:

	void					ReleaseData();
	void					DumpCodeChunk(TDestinationFile &report_buffer, TCppCodeChunkId chunk_id, TCppCodeChunkInfo &chunk_info);

protected:

	enum { CodeChunkIdIndexOffs = 40 };

	TCppCodeEntryPointDefn			*m_function_defn;

	TCppCodeChunkInfo				*m_code_chunks;
	int								m_num_code_chunks;			// The number of code chunks is zero only when the structure describes
																	// the prototype of the function.

	TCppDatabaseBuffersArray			m_code_data_buffers;

	friend class TCppFunctionCodeGenerator;
};

class TCppFunctionCodeGenerator
{
public:

		TCppFunctionCodeGenerator(TCppDatabase &cpp_database);

		bool		GenerateFunctionCode(TMessageBuffer &error_buffer, TCppFunctionCode &dest_code_buffer, TCppCodeEntryPointDefn *func_to_process);

protected:

		bool		ProcessCppItem(TCppItemBase *cpp_item);

		bool		AddExpressionStep(TCppItemBase *step_cpp_item);
		bool		AddPopStackStep();

		bool		AddIfStatementStepAndSaveChunk(TCppCodeChunkId then_chunk, TCppCodeChunkId else_chunk, TCppItemBase *step_cpp_item = NULL);
		bool		AddGotoStatementStepAndSaveChunk(TCppCodeChunkId goto_chunk, TCppItemBase *step_cpp_item = NULL);
		bool		AddReturnStatementStepAndSaveChunk(TCppItemBase *step_cpp_item = NULL);

		bool		SaveCurrentChunk();

		void		PrepareErrorMessage(const wchar_t *fmt, ...);

protected:

		typedef TStructsArray<TCppCodeStepInfo, 256, 1024> TCppCodeStepInfosArray;
		typedef TStructsArray<TCppCodeChunkInfo, 256, 1024> TCppCodeChunkInfosArray;

		TCppDatabase					&m_cpp_database;
		TMessageBuffer					*m_error_buffer;
		TCppDatabaseBuffersArray			*m_data_buffers;

		TCppCodeStepInfosArray			m_curr_chunk;
		int								m_stack_len;

		TCppCodeChunkInfosArray			m_chunks_info;
};

#endif	// TextScan_CppCodeGen_H


