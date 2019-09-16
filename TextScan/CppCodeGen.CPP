//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "TextScan/CppCodeGen.H"

//--------------------------------------------------------------------------------------
//  ========================  TCppFunctionCode  ===========================
//--------------------------------------------------------------------------------------

TCppCodeStepInfo *TCppFunctionCode::GetCodeChunk(TCppCodeChunkId chunk_id, int *len_chunk)
{
	if (chunk_id == FuncEntryChunkId)
	{
		if (m_num_code_chunks <= 0)
			return(NULL);

		// The first code chunk describes the entry point into the function.
		if (len_chunk != NULL)
			*len_chunk = m_code_chunks[0].m_num_steps;
		return(m_code_chunks[0].m_chunk_steps);
	}

	if (chunk_id >= (TCppCodeChunkId)CodeChunkIdIndexOffs && (int)(chunk_id-CodeChunkIdIndexOffs) < m_num_code_chunks)
	{
		// Code chunk with the requested id is available.
		TCppCodeChunkInfo &info = m_code_chunks[chunk_id-CodeChunkIdIndexOffs];

		if (len_chunk != NULL)
			*len_chunk = info.m_num_steps;
		return(info.m_chunk_steps);
	}

	// Failure. The passed chunk id is bogus.
	return(NULL);
}

bool TCppFunctionCode::GenerateBasicReport(TDestinationFile &rprt)
{
	if (m_function_defn == NULL)
	{
		rprt.WriteLine(L"The structure is not inited.");
		return(TRUE);
	}

	// Emit information about the function.
	assert(m_function_defn->ItemType() == cxx_code_entry_point || m_function_defn->ItemType() == cxx_func_template_inst);
	rprt.WriteLine();
	rprt.WriteLine(L"  Cpp function code");
	rprt.WriteLine(L"---------------------------------------------");
	rprt.WriteFmtLine(L"  FunctionId: \t\t%ld", m_function_defn->ItemId());
	rprt.WriteFmtLine(L"  FunctionName: \t%s", m_function_defn->DefnName());

	if (m_function_defn->m_return_value_type != NULL)
	{
		rprt.WriteFmtLine(L"  RetValueTypeId: \t%ld", m_function_defn->m_return_value_type->ItemId());
		rprt.WriteFmtLine(L"  RetValueTypeName: \t%s", m_function_defn->m_return_value_type->DefnName());
	}
	else
	{
		rprt.WriteLine(L"  ReturnValueTypeInfoIsMissing");
	}

	rprt.WriteFmtLine(L"  NumParams: \t\t%d", m_function_defn->GetNumParams());
	rprt.WriteFmtLine(L"  NumCodeChunks: \t%d", m_num_code_chunks);
	rprt.WriteLine();

	// Emit info about the code.
	if (m_num_code_chunks > 0)
	{
		// Dump code chunks one by one.
		for (int ichunk=0; ichunk<m_num_code_chunks; ++ichunk)
			DumpCodeChunk(rprt, ChunkIdFromChunkIndex(ichunk), m_code_chunks[ichunk]);
	}

	// Success.
	return(TRUE);
}

void TCppFunctionCode::ReleaseData()
{
	m_function_defn = NULL;
	m_code_chunks = NULL;
	m_num_code_chunks = 0;
}

void TCppFunctionCode::DumpCodeChunk(TDestinationFile &rprt, TCppCodeChunkId chunk_id, TCppCodeChunkInfo &chunk_info)
{
	rprt.WriteFmtLine(L"  Code chunk %lu.", chunk_id);
	rprt.WriteLine(L"---------------------------------------------");

	if (chunk_info.m_num_steps <= 0)
	{
		rprt.WriteLine(L"  The code chunk is empty.");
	}
	else
	{
		// Iterate steps of the code chunk.
		for (int istep=0; istep<chunk_info.m_num_steps; ++istep)
		{
			TCppCodeStepInfo &step = chunk_info.m_chunk_steps[istep];
			switch (step.m_code_step_type)
			{
				case ccst_expression:
						{
							TCppItemBase *cpp_item = step.m_code_step_cpp_item;
							assert(cpp_item != NULL);

							wchar_t op_code_buffer[80];
							if (cpp_item->IsExpression() == TRUE)
							{
								if (cpp_item->ItemType() == cxx_operand_expr)
								{
									// Provide better details about the operand.
									TCppOperandExpr *oprnd_expr = (TCppOperandExpr*)cpp_item;
									switch (oprnd_expr->m_oprnd_type)
									{
										case eopr_num_literal:
												swprintf(op_code_buffer, 80, L"Operand, NumericLiteral=%I64d", oprnd_expr->m_num_value);
												break;

										case eopr_data_field:
												swprintf(op_code_buffer, 80, L"Operand, DataField=%s", oprnd_expr->m_data_field_ref->DefnName());
												break;

										default:
											wcscpy(op_code_buffer, L"BOGUS-OPERAND-TYPE");
											break;
									}
								}
								else
								{
									// Just show the op code.
									swprintf(op_code_buffer, 80, L"Operation=%s", ((TCppExpressionBase*)cpp_item)->OperationEnumName());
								}
							}
							else
							{
								// Write beginning in capitals to make this sutiation more visible.
								wcscpy(op_code_buffer, L"NON-EXPR-CPP-ITEM");
							}

							rprt.WriteFmtLine(L"%3d: %s, CodeItemId=%lu, CodeItemType=%s.", istep+1, op_code_buffer, cpp_item->ItemId(), cpp_item->ItemTypeEnumName());
						}
						break;

				case ccst_pop_stack:
						rprt.WriteFmtLine(L"%3d: PopStack.", istep+1);
						break;

				case ccst_if:
						rprt.WriteFmtLine(L"%3d: If-statement, Then=%lu, Else=%lu.", istep+1, step.m_if_then_chunk, step.m_if_else_chunk);
						break;

				case ccst_switch:
						rprt.WriteFmtLine(L"%3d: Switch-statement, NumLabels=%d.", istep+1, step.m_switch_num_labels);
						break;

				case ccst_goto:
						rprt.WriteFmtLine(L"%3d: Goto %lu.", istep+1, step.m_goto_chunk);
						break;

				case ccst_return:
						rprt.WriteFmtLine(L"%3d: Return.", istep+1);
						break;

				default:
					rprt.WriteFmtLine(L"%3d: Bogus value of the code step type (%d).", istep+1, step.m_code_step_type);
					break;
			}
		}
	}

	rprt.WriteLine();
}

//---------------------------------------------------------------------------------------
//  =====================  TCppFunctionCodeGenerator  ========================
//---------------------------------------------------------------------------------------

TCppFunctionCodeGenerator::TCppFunctionCodeGenerator(TCppDatabase &cpp_database)
		: m_cpp_database(cpp_database)
{
	m_error_buffer = NULL;
	m_data_buffers = NULL;
	m_stack_len = 0;
}

bool TCppFunctionCodeGenerator::GenerateFunctionCode(TMessageBuffer &error_buffer, TCppFunctionCode &code_buffer, TCppCodeEntryPointDefn *func_to_process)
{
	assert(func_to_process != NULL && (func_to_process->ItemType() == cxx_code_entry_point || func_to_process->ItemType() == cxx_func_template_inst));
	error_buffer.ClearBuffer();
	code_buffer.ReleaseData();

	m_error_buffer = &error_buffer;
	m_data_buffers = &(code_buffer.m_code_data_buffers);

	m_curr_chunk.Clear();
	m_stack_len = 0;
	m_chunks_info.Clear();

	code_buffer.m_function_defn = func_to_process;

	if (func_to_process->m_function_body != NULL)
	{
		if (ProcessCppItem(func_to_process->m_function_body) == FALSE)
			return(FALSE);
	}

	// Save the final code chunk if any.
	if (m_curr_chunk.NumItems() > 0 && SaveCurrentChunk() == FALSE)
		return(FALSE);

	// Process directory of the code chunks.
	if (m_chunks_info.NumItems() > 0)
	{
		// Move info about the code chunks into the data bufffers.
		TCppCodeChunkInfo *chunks_data = (TCppCodeChunkInfo*)code_buffer.m_code_data_buffers.AddStructAlignedData(m_chunks_info.DataPtr(), m_chunks_info.NumItems()*sizeof(TCppCodeChunkInfo));
		if (chunks_data == NULL)
		{
			PrepareErrorMessage(L"Error saving the chunks directory.");
			return(FALSE);
		}

		// Space in the data buffers was allocated.
		code_buffer.m_code_chunks = chunks_data;
		code_buffer.m_num_code_chunks = m_chunks_info.NumItems();
	}

	m_error_buffer = NULL;
	m_data_buffers = NULL;
	return(TRUE);
}

bool TCppFunctionCodeGenerator::ProcessCppItem(TCppItemBase *cpp_item)
{
	assert(cpp_item != NULL);

	switch (cpp_item->ItemType())
	{
		case cxx_enum_type:
		case cxx_struct_type:
				// These items cannot be executed. Simply ignore them.
				break;

		case cxx_if_stmt:
				{
					TCppIfStatement *if_stmt = (TCppIfStatement*)cpp_item;
					if (ProcessCppItem(if_stmt->m_if_condition) == FALSE)
						return(FALSE);

					TCppCodeChunkId next_chunk_id = (m_chunks_info.NumItems())+TCppFunctionCode::CodeChunkIdIndexOffs+1;
					if (AddIfStatementStepAndSaveChunk(next_chunk_id, next_chunk_id+1, cpp_item) == FALSE)
						return(FALSE);

					if (ProcessCppItem(if_stmt->m_then_statement) == FALSE)
						return(FALSE);

					/*
					TCppCodeChunkId then_chunk = next_chunk_id, else_chunk = next_chunk_id, cont_chunk = next_chunk_id;
					if (if_stmt->IsThenPart() == TRUE)
						else_chunk++;
					*/
				}
				break;

		case cxx_block:
				{
					TCppBlockDefn *block = (TCppBlockDefn*)cpp_item;
					for (TCppDirectChildrenIter iter(cpp_item); iter; ++iter)
					{
						if (ProcessCppItem(&(iter.CurrItem())) == FALSE)
							return(FALSE);

						while (m_stack_len > 0)
						{
							if (AddPopStackStep() == FALSE)
								return(FALSE);
							m_stack_len--;
						}
					}
				}
				break;

		case cxx_return_stmt:
				{
					TCppReturnStatement *ret_stmt = (TCppReturnStatement*)cpp_item;
					if (ret_stmt->m_return_value != NULL)
					{
						// Expression is present in the statement. Process it.
						if (ProcessCppItem(ret_stmt->m_return_value) == FALSE)
							return(FALSE);
					}

					// Finalize curr chunk with the return step and mark this step with the stmt from the source code.
					if (AddReturnStatementStepAndSaveChunk(cpp_item) == FALSE)
						return(FALSE);
				}
				break;

		case cxx_operand_expr:
				{
					if (AddExpressionStep(cpp_item) == FALSE)
						return(FALSE);

					m_stack_len++;
				}
				break;

		case cxx_unary_expr:
				{
					TCppUnaryExpr *unary_expr = (TCppUnaryExpr*)cpp_item;
					if (ProcessCppItem(unary_expr->m_operand) == FALSE)
						return(FALSE);
					if (AddExpressionStep(cpp_item) == FALSE)
						return(FALSE);
				}
				break;

		case cxx_binary_expr:
				{
					TCppBinaryExpr *binary_expr = (TCppBinaryExpr*)cpp_item;
					if (ProcessCppItem(binary_expr->m_operand1) == FALSE)
						return(FALSE);
					if (ProcessCppItem(binary_expr->m_operand2) == FALSE)
						return(FALSE);
					if (AddExpressionStep(cpp_item) == FALSE)
						return(FALSE);

					m_stack_len--;
				}
				break;

		case cxx_assignment_expr:
				{
					TCppAssignmentExpr *assgn_expr = (TCppAssignmentExpr*)cpp_item;

					if (ProcessCppItem(assgn_expr->m_operand1) == FALSE)
						return(FALSE);
					if (ProcessCppItem(assgn_expr->m_operand2) == FALSE)
						return(FALSE);
					if (AddExpressionStep(cpp_item) == FALSE)
						return(FALSE);

					m_stack_len--;
				}
				break;

		default:
			// All other types of cpp items should not appear in this method.
			PrepareErrorMessage(L"Inexpected CppItemType=%s.", cpp_item->ItemTypeEnumName());
			return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TCppFunctionCodeGenerator::AddExpressionStep(TCppItemBase *step_cpp_item)
{
	assert(step_cpp_item != NULL);

	TCppCodeStepInfo step_info;
	step_info.Clear();
	step_info.m_code_step_type = ccst_expression;
	step_info.m_code_step_cpp_item = step_cpp_item;

	if (m_curr_chunk.AppendItem(step_info) == FALSE)
	{
		PrepareErrorMessage(L"Error saving the expression step. CppItemId=%lu.", step_cpp_item->ItemId());
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TCppFunctionCodeGenerator::AddPopStackStep()
{
	TCppCodeStepInfo step_info;
	step_info.Clear();
	step_info.m_code_step_type = ccst_pop_stack;

	if (m_curr_chunk.AppendItem(step_info) == FALSE)
	{
		PrepareErrorMessage(L"Error saving the pop stack step.");
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TCppFunctionCodeGenerator::AddIfStatementStepAndSaveChunk(TCppCodeChunkId then_chunk, TCppCodeChunkId else_chunk, TCppItemBase *step_cpp_item)
{
	TCppCodeStepInfo step_info;
	step_info.Clear();
	step_info.m_code_step_type = ccst_if;
	step_info.m_code_step_cpp_item = step_cpp_item;
	step_info.m_if_then_chunk = then_chunk;
	step_info.m_if_else_chunk = else_chunk;

	if (m_curr_chunk.AppendItem(step_info) == FALSE)
	{
		PrepareErrorMessage(L"Error saving the if-statement step.");
		return(FALSE);
	}
	else if (SaveCurrentChunk() == FALSE)
	{
		// Error is already reported.
		return(FALSE);
	}

	// Success.
	m_stack_len = 0;
	return(TRUE);
}

bool TCppFunctionCodeGenerator::AddGotoStatementStepAndSaveChunk(TCppCodeChunkId goto_chunk, TCppItemBase *step_cpp_item)
{
	TCppCodeStepInfo step_info;
	step_info.Clear();
	step_info.m_code_step_type = ccst_goto;
	step_info.m_code_step_cpp_item = step_cpp_item;
	step_info.m_goto_chunk = goto_chunk;

	if (m_curr_chunk.AppendItem(step_info) == FALSE)
	{
		PrepareErrorMessage(L"Error saving the goto-statement step.");
		return(FALSE);
	}
	else if (SaveCurrentChunk() == FALSE)
	{
		// Error is already reported.
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TCppFunctionCodeGenerator::AddReturnStatementStepAndSaveChunk(TCppItemBase *step_cpp_item)
{
	TCppCodeStepInfo step_info;
	step_info.Clear();
	step_info.m_code_step_type = ccst_return;
	step_info.m_code_step_cpp_item = step_cpp_item;

	if (m_curr_chunk.AppendItem(step_info) == FALSE)
	{
		PrepareErrorMessage(L"Error saving the return-statement step.");
		return(FALSE);
	}
	else if (SaveCurrentChunk() == FALSE)
	{
		// Error is already reported.
		return(FALSE);
	}

	// Success.
	m_stack_len = 0;
	return(TRUE);
}

bool TCppFunctionCodeGenerator::SaveCurrentChunk()
{
	// Check the current code chunk.
	if (m_curr_chunk.NumItems() <= 0)
	{
		PrepareErrorMessage(L"Current code chunk is empty.");
		return(FALSE);
	}

	TCppCodeStepType last_step_type = m_curr_chunk[m_curr_chunk.NumItems()-1].m_code_step_type;
	if (last_step_type < ccst_if || last_step_type > ccst_return)
	{
		PrepareErrorMessage(L"Current code chunk is not terminated properly. LastStepType=%d.", last_step_type);
		return(FALSE);
	}

	// Move steps of the current chunk into the data bufffers.
	long steps_data_len = m_curr_chunk.NumItems()*sizeof(TCppCodeStepInfo);
	TCppCodeStepInfo *steps_data = (TCppCodeStepInfo*)m_data_buffers->AddStructAlignedData(m_curr_chunk.DataPtr(), steps_data_len);
	if (steps_data == NULL)
	{
		PrepareErrorMessage(L"Error saving steps of the current code chunk.");
		return(FALSE);
	}

	// Prepare and save the chunk header.
	TCppCodeChunkInfo info = { steps_data, m_curr_chunk.NumItems(), -1 };
	if (m_chunks_info.AppendItem(info) == FALSE)
	{
		PrepareErrorMessage(L"Error saving header of the current code chunk.");
		return(FALSE);
	}

	// Clear the current chunk.
	m_curr_chunk.Clear();
	return(TRUE);
}

void TCppFunctionCodeGenerator::PrepareErrorMessage(const wchar_t *fmt, ...)
{
	wchar_t err_msg_buff[2048];

	// Format the primary error message.
	va_list vargs;
	va_start(vargs, fmt);
	vswprintf(err_msg_buff, 2048, fmt, vargs);
	va_end(vargs);

	m_error_buffer->Append(err_msg_buff);

	int all_chunks_steps = 0;
	for (int ich=0; ich<m_chunks_info.NumItems(); ++ich)
		all_chunks_steps += m_chunks_info[ich].m_num_steps;

	// Prepare some details of the current state.
	swprintf(err_msg_buff, 2048, L"\r\n\r\nNumChunks=%d, NumCurrChunkSteps=%d, TotalNumberOfSteps=%d.",
			m_chunks_info.NumItems(), m_curr_chunk.NumItems(), all_chunks_steps);

	m_error_buffer->Append(err_msg_buff);
}


