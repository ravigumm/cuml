/*
 * Copyright (c) 2018, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "ml_utils.h"
#include <stats/mean.h>
#include <stats/mean_center.h>
#include <stats/stddev.h>
#include <linalg/norm.h>
#include <matrix/math.h>
#include <matrix/matrix.h>

namespace ML {
namespace GLM {

using namespace MLCommon;

template<typename math_t>
void preProcessData(math_t *input, int n_rows, int n_cols, math_t *labels,
		math_t *intercept, math_t *mu_input, math_t *mu_labels, math_t *norm2_input,
		bool fit_intercept, bool normalize, cublasHandle_t cublas_handle,
		cusolverDnHandle_t cusolver_handle) {

	ASSERT(n_cols > 1,
			"Parameter n_cols: number of columns cannot be less than two");
	ASSERT(n_rows > 1,
			"Parameter n_rows: number of rows cannot be less than two");

	if (fit_intercept) {
		Stats::mean(mu_input, input, n_cols, n_rows, false, false);
		Stats::meanCenter(input, mu_input, n_cols, n_rows, false);

		Stats::mean(mu_labels, labels, 1, n_rows, false, false);
		Stats::meanCenter(labels, mu_labels, 1, n_rows, false);

		if (normalize) {
			LinAlg::norm2(norm2_input, input, n_cols, n_rows, false);
			Matrix::matrixVectorBinaryDivSkipZero(input, norm2_input, n_rows, n_cols, false, true);
		}
	}

}

template<typename math_t>
void postProcessData(math_t *input, int n_rows, int n_cols, math_t *labels, math_t *coef,
		math_t *intercept, math_t *mu_input, math_t *mu_labels, math_t *norm2_input,
		bool fit_intercept, bool normalize, cublasHandle_t cublas_handle,
		cusolverDnHandle_t cusolver_handle) {

	ASSERT(n_cols > 1,
			"Parameter n_cols: number of columns cannot be less than two");
	ASSERT(n_rows > 1,
			"Parameter n_rows: number of rows cannot be less than two");

	math_t *d_intercept;
	allocate(d_intercept, 1);

	if (normalize) {
		Matrix::matrixVectorBinaryMult(input, norm2_input, n_rows, n_cols, false);
		Matrix::matrixVectorBinaryDivSkipZero(coef, norm2_input, 1, n_cols, false, true);
	}

	math_t alpha = math_t(1);
	math_t beta = math_t(0);
	LinAlg::gemm(mu_input, 1, n_cols, coef, d_intercept, 1, 1,
		    false, false, alpha, beta, cublas_handle);

	LinAlg::subtract(d_intercept, mu_labels, d_intercept, 1);
	updateHost(intercept, d_intercept, 1);
	if (d_intercept != NULL)
		cudaFree(d_intercept);

	Stats::meanAdd(input, mu_input, n_cols, n_rows, false);
	Stats::meanAdd(labels, mu_labels, 1, n_rows, false);

}

/** @} */
}
;
}
;
// end namespace ML
