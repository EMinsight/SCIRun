/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2020 Scientific Computing and Imaging Institute,
   University of Utah.

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <Core/Algorithms/Base/AlgorithmPreconditions.h>
#include <Core/Algorithms/Base/AlgorithmVariableNames.h>
#include <Core/Algorithms/Math/SelectSubMatrix.h>
#include <Core/Datatypes/DenseMatrix.h>
#include <Core/Datatypes/MatrixTypeConversions.h>
#include <Core/Datatypes/SparseRowMatrix.h>
#include <Core/Datatypes/SparseRowMatrixFromMap.h>

using namespace SCIRun::Core::Algorithms;
using namespace SCIRun::Core::Algorithms::Math;
using namespace SCIRun::Core::Datatypes;

SelectSubMatrixAlgorithm::SelectSubMatrixAlgorithm()
{
  addParameter(Parameters::rowCheckBox, false);
  addParameter(Parameters::columnCheckBox, false);
  addParameter(Parameters::rowStartSpinBox, 0);
  addParameter(Parameters::columnStartSpinBox, 0);
  addParameter(Parameters::columnEndSpinBox, 0);
  addParameter(Parameters::rowEndSpinBox, 0);
}

MatrixHandle SelectSubMatrixAlgorithm::get_sub_matrix(
    MatrixHandle input_matrix, DenseMatrixHandle rows, DenseMatrixHandle cols) const
{
  std::vector<index_type> sel_rows;
  std::vector<index_type> sel_cols;

  size_type count = 0;
  if (rows)
  {
    if (rows->nrows() > 0 && rows->ncols() > 0)
    {
      index_type nr = rows->nrows(), nc = rows->ncols();
      if (nr == 0) nr = 1;
      if (nc == 0) nc = 1;
      sel_rows.resize(nr * nc);

      for (index_type i = 0; i < nr; i++)
        for (index_type j = 0; j < nc; j++)
        {
          sel_rows[count++] = static_cast<index_type>((*rows)(i, j));
        }
    }
  }
  count = 0;

  if (cols)
  {
    if (cols->nrows() > 0 && cols->ncols() > 0)
    {
      index_type nr = cols->nrows(), nc = cols->ncols();
      if (nr == 0) nr = 1;
      if (nc == 0) nc = 1;
      sel_cols.resize(nr * nc);
      for (index_type i = 0; i < nr; i++)
        for (index_type j = 0; j < nc; j++)
          sel_cols[count++] = static_cast<index_type>((*cols)(i, j));
    }
  }
  return run(input_matrix, sel_rows, sel_cols);
}

MatrixHandle SelectSubMatrixAlgorithm::run(
    MatrixHandle input_matrix, DenseMatrixHandle row_indices, DenseMatrixHandle col_indices) const
{
  if (!input_matrix || input_matrix->nrows() == 0 || input_matrix->ncols() == 0)
  {
    remark(" No valid inputs: input matrix or row,column matrix contain null pointer ");
    return MatrixHandle();
  }

  MatrixHandle sub_matrix;
  const bool row_select = get(Parameters::rowCheckBox).toBool();
  const bool col_select = get(Parameters::columnCheckBox).toBool();
  index_type row_start = get(Parameters::rowStartSpinBox).toInt();
  index_type row_end = get(Parameters::rowEndSpinBox).toInt();
  index_type col_start = get(Parameters::columnStartSpinBox).toInt();
  index_type col_end = get(Parameters::columnEndSpinBox).toInt();

  if (!row_select && !col_select)  /// pipe input through
    if (!row_indices && !col_indices)
    {
      if (!input_matrix) return input_matrix;
    }

  if (row_indices || col_indices)
  {
    if (row_select || col_select)
    {
      remark("Index matrices detected on inputs (indexing starts from 0), ignoring UI settings");
    }
  }

  if (!(row_indices || col_indices))
  {
    if (row_select && col_select)
    {
      row_end = row_end - row_start + 1;
      col_end = col_end - col_start + 1;
    }
    else if (row_select && !col_select)
    {
      col_start = 0;
      row_end = row_end - row_start + 1;
      col_end = input_matrix->ncols();
    }
    else if (!row_select && col_select)
    {
      row_start = 0;
      row_end = input_matrix->nrows();
      col_end = col_end - col_start + 1;
    }
    else
    {  /// pass it through
      row_start = 0;
      col_start = 0;
      row_end = input_matrix->nrows();
      col_end = input_matrix->ncols();
    }
  }
  if (row_start < 0 || col_start < 0 || row_end > input_matrix->nrows() ||
      col_end > input_matrix->ncols())
  {
    remark(" Specified matrix indices from UI settings exceed matrix dimensions ");
    return nullptr;
  }

  if (row_indices || col_indices)
  {
    sub_matrix = get_sub_matrix(input_matrix, row_indices, col_indices);
    if (!sub_matrix) 
      return nullptr;
  }
  else  /// GUI input only
  {
    if (matrixIs::sparse(input_matrix))
    {
      SparseRowMatrixHandle mat(new SparseRowMatrix(
          castMatrix::toSparse(input_matrix)->block(row_start, col_start, row_end, col_end)));
      return mat;
    }
    if (matrixIs::dense(input_matrix))
    {
      DenseMatrixHandle mat(new DenseMatrix(
          castMatrix::toDense(input_matrix)->block(row_start, col_start, row_end, col_end)));
      return mat;
    }
    if (matrixIs::column(input_matrix))
    {
      if (input_matrix->ncols() != 1)
      {
        THROW_ALGORITHM_INPUT_ERROR("Input matrix is apparently not a column matrix! ");
      }
      if (input_matrix->nrows() == 0)
      {
        THROW_ALGORITHM_INPUT_ERROR("Input matrix (column) does not contain any rows! ");
      }
      if (row_start < 0 || row_end >= input_matrix->nrows())
      {
        THROW_ALGORITHM_INPUT_ERROR("Gui row indeces exceed input matrix dimensions! ");
      }
      if (col_start < 0 || col_end >= input_matrix->nrows())
      {
        THROW_ALGORITHM_INPUT_ERROR("Gui row indeces exceed input matrix dimensions! ");
      }

      const auto first_column = castMatrix::toColumn(input_matrix);

      if (!first_column || first_column->nrows() <= 0 || first_column->ncols() != 1)
      {
        THROW_ALGORITHM_INPUT_ERROR("Conversion to column matrix did not work! ");
      }

      auto output(makeShared<DenseMatrix>(row_end - row_start + 1, 1));

      for (index_type i = 0; i < row_end - row_start + 1; i++)
      {
        if (i >= 0 && i < output->nrows())
        {
          (*output)(i, 0) = first_column->coeff(row_start + i);
        }
        else
        {
          THROW_ALGORITHM_INPUT_ERROR("Index in column index matrix out of range! ");
        }
      }
      return output;
    }

    remark("This module needs row indices, or column indices or both from UI or input matrices");
    remark("Copying input matrix to output");
    sub_matrix = input_matrix;
  }

  return sub_matrix;
}

MatrixHandle SelectSubMatrixAlgorithm::run(MatrixHandle input_matrix,
    const std::vector<SCIRun::index_type>& rows, const std::vector<SCIRun::index_type>& cols) const
{
  if (!input_matrix) { THROW_ALGORITHM_INPUT_ERROR("No input matrix"); }

  if ((rows.empty()) && (cols.empty()))
  {
    THROW_ALGORITHM_INPUT_ERROR("No row and column indices given");
  }

  size_type m = input_matrix->nrows();
  size_type n = input_matrix->ncols();

  for (size_t r = 0; r < rows.size(); r++)
  {
    if (rows[r] >= static_cast<index_type>(m) || rows[r] < 0 || !IsFinite(rows[r]))
    {
      THROW_ALGORITHM_INPUT_ERROR("Selected row exceeds matrix dimensions");
    }
  }

  for (size_t r = 0; r < cols.size(); r++)
  {
    if (cols[r] >= static_cast<index_type>(n) || cols[r] < 0 || !IsFinite(cols[r]))
    {
      THROW_ALGORITHM_INPUT_ERROR("Selected column exceeds matrix dimensions");
    }
  }

  auto sparse_matrix = castMatrix::toSparse(input_matrix);

  if (sparse_matrix)
  {
    SparseRowMatrixFromMap::Values additionalData;
    if (!rows.empty() && !cols.empty())  /// get only the indices intersection
    {
      m = rows.size();
      n = cols.size();
      for (auto row : rows)
      {
        for (auto col : cols)
        {
          auto tmp = sparse_matrix->coeff(row, col);
          if (tmp) 
            additionalData[row][col] = tmp;
        }
      }
    }
    if (!rows.empty() && cols.empty())
    {
      m = rows.size();
      for (size_t i = 0; i < rows.size(); i++)
      {
        Eigen::SparseVector<double> matrix_row = sparse_matrix->row(rows[i]);
        for (Eigen::SparseVector<double>::InnerIterator it(matrix_row); it; ++it)
        {
          additionalData[rows[i]][it.index()] = it.value();
        }
      }
    }
    if (rows.empty() && !cols.empty())
    {
      n = cols.size();
      for (size_t j = 0; j < cols.size(); j++)
      {
        Eigen::SparseVector<double> matrix_col = sparse_matrix->col(cols[j]);
        for (Eigen::SparseVector<double>::InnerIterator it(matrix_col); it; ++it)
        {
          additionalData[it.index()][cols[j]] = it.value();
        }
      }
    }
    auto output_matrix = SparseRowMatrixFromMap::make(m, n, additionalData);
  }
  else
  {
    DenseColumnMatrixHandle col_dense_input_matrix;

    if (matrixIs::dense(input_matrix))
    {
      auto dense_input_matrix = castMatrix::toDense(input_matrix);

      if (!rows.empty() && !cols.empty())
      {
        auto output(makeShared<DenseMatrix>(rows.size(), cols.size()));

        for (index_type i = 0; i < rows.size(); i++)
          for (index_type j = 0; j < cols.size(); j++)
            (*output)(i, j) = dense_input_matrix->coeff(rows[i], cols[j]);

        return output;
      }

      if (!rows.empty() && cols.empty())
      {
        auto output(makeShared<DenseMatrix>(rows.size(), n));
        for (index_type i = 0; i < rows.size(); i++)
          output->row(i) = dense_input_matrix->row(rows[i]);
        return output;
      }

      if (rows.empty() && !cols.empty())
      {
        auto output(makeShared<DenseMatrix>(m, cols.size()));

        for (index_type i = 0; i < cols.size(); i++)
        {
          output->col(i) = dense_input_matrix->col(cols[i]);
        }
        return output;
      }
    }
    else if (matrixIs::column(input_matrix))  ///@Spencer this stuff needs to be tested in UnitTest
    {
      if (input_matrix->ncols() != 1)
      {
        THROW_ALGORITHM_INPUT_ERROR("Input matrix is apparently not a column matrix! ");
      }
      if (input_matrix->nrows() == 0)
      {
        THROW_ALGORITHM_INPUT_ERROR("Input matrix (column) does not contain any rows! ");
      }
      if (!cols.empty())
      {
        for (auto col : cols)
        {
          if (col != 0)
          {
            THROW_ALGORITHM_INPUT_ERROR("Column input matrix does contain bad indeces! ");
          }
        }
      }
      if (rows.empty())
      {
        THROW_ALGORITHM_INPUT_ERROR("Rows input matrix does not contain any rows! ");
      }
      const auto first_column = castMatrix::toColumn(input_matrix);

      if (!first_column || first_column->nrows() <= 0 || first_column->ncols() != 1)
      {
        THROW_ALGORITHM_INPUT_ERROR("Conversion to column matrix did not work! ");
      }
      auto output(makeShared<DenseMatrix>(rows.size(), 1));
      for (index_type i = 0; i < rows.size(); i++)
      {
        if (IsFinite(rows[i]) && rows[i] > 0 && rows[i] < input_matrix->nrows())
        {
          (*output)(i, 0) = (*first_column).coeff(rows[i]);
        }
        else
        {
          THROW_ALGORITHM_INPUT_ERROR("Index in column index matrix out of range! ");
        }
      }
      return output;
    }
    else
    {
      THROW_ALGORITHM_INPUT_ERROR("Unknown input matrix type");
    }
  }
  remark("Could not deal with input data. Copying input matrix to output");
  return input_matrix;
}

const AlgorithmInputName SelectSubMatrixAlgorithm::RowIndicies("RowIndicies");
const AlgorithmInputName SelectSubMatrixAlgorithm::ColumnIndicies("ColumnIndicies");
ALGORITHM_PARAMETER_DEF(Math, rowCheckBox);
ALGORITHM_PARAMETER_DEF(Math, columnCheckBox);
ALGORITHM_PARAMETER_DEF(Math, rowStartSpinBox);
ALGORITHM_PARAMETER_DEF(Math, columnStartSpinBox);
ALGORITHM_PARAMETER_DEF(Math, columnEndSpinBox);
ALGORITHM_PARAMETER_DEF(Math, rowEndSpinBox);

AlgorithmOutput SelectSubMatrixAlgorithm::run(const AlgorithmInput& input) const
{
  auto input_matrix = input.get<Matrix>(Variables::InputMatrix);
  auto rowindicies = input.get<DenseMatrix>(RowIndicies);
  auto columnindicies = input.get<DenseMatrix>(ColumnIndicies);

  auto output_matrix = run(input_matrix, rowindicies, columnindicies);

  AlgorithmOutput output;
  output[Variables::ResultMatrix] = output_matrix;
  return output;
}
