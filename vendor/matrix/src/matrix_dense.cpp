#include <cassert>

#include "matrix_dense.h"

extern "C" int dgetrf_(const int* m, const int* n, double* a, const int* lda, int* ipiv, int* info);
extern "C" int dgetrs_(
        const char* trans,
        const int* n,
        const int* nrhs,
        double* a,
        const int* lda,
        int* ipiv,
        double* b,
        const int* ldb,
        int* info);

Matrix_Dense::Matrix_Dense(int n) : Matrix(n)
{
    assert(n > 0);
    ipiv = std::make_unique<int[]>(n);
    a = std::make_unique<double[]>(n * n);
    for (int i(0); i < n * n; ++i) {
        a[i] = 0;
    }
}

void Matrix_Dense::set_element(int i, int j, double aij)
{
    a[i * n + j] = aij;
}

double Matrix_Dense::get_element(int i, int j) const
{
    assert(i < n);
    assert(j < n);
    return a[i * n + j];
}

int Matrix_Dense::factorize_method()
{
    int info;
    dgetrf_(&n, &n, a.get(), &n, ipiv.get(), &info);
    return info;
}

int Matrix_Dense::solve_inplace_method(const char transpose, double* b, int nrows, int ncols) const
{
    int info;

    dgetrs_(&transpose, &n, &ncols, a.get(), &n, ipiv.get(), b, &nrows, &info);
    return info;
}