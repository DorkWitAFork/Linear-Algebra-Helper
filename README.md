# Linear Algebra Helper

![image alt](https://github.com/DorkWitAFork/Linear-Algebra-Helper/blob/main/LA%20Helper%20Photo.png?raw=true)
LA Helper is a Windows desktop symbolic matrix calculator and linear-algebra tutor. It is a C++17/Qt 6 rewrite of `LA Helper.py`, using SymEngine for exact arithmetic and symbolic expressions.

## Features

- Exact integers, fractions, constants, and symbolic variables
- Saved matrices and safe matrix-expression evaluation
- System solving and `Ax = b`
- Parameter consistency conditions
- Transpose, inverse, multiplication steps, and LU factorization
- Echelon form and RREF with elementary row-operation steps
- Determinant, rank, nullity, nullspace, and column space
- Characteristic polynomials, supported eigenvalues, and eigenspaces
- UTF-8 text export of the latest result

Matrix expressions include examples such as:

```text
A + B*3
A*B
A**-1
det(A)
inv(A)
rref(A)
A.T
A[0,1]
diag(1, x, 3)
```

Matrix indexing is zero-based, matching the Python application.

## Windows Build

Install these prerequisites:

- Visual Studio 2022 with the Desktop development with C++ workload
- CMake 3.24 or newer
- Git
- vcpkg

Set `VCPKG_ROOT` to the vcpkg directory, then run from a Developer PowerShell:

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
powershell -ExecutionPolicy Bypass -File scripts/package-windows.ps1
```

The first configure installs Qt and SymEngine through the included `vcpkg.json` manifest and can take some time. The portable application is written to `dist/LAHelper`. Distribute the entire `LAHelper` folder, not only `LAHelper.exe`, because Qt and SymEngine are dynamically linked.

Build files and manifest dependencies are stored under `%LOCALAPPDATA%/LAHelper/build/windows-release` so vcpkg dependencies that do not support spaces can build even when the source directory contains spaces.

## Symbolic Behavior

The matrix algorithms are implemented by LA Helper over SymEngine expressions. A pivot is treated as nonzero unless SymEngine can simplify it exactly to zero. Consequently, parameter values that make a symbolic pivot vanish can form special cases and should be checked separately.

SymEngine solves many polynomial eigenvalue and single-parameter consistency equations, but it does not have full SymPy solver parity. When it cannot produce a finite explicit solution set, LA Helper preserves and displays the exact characteristic or consistency equation instead of silently replacing it with an approximate answer.

General polynomial roots are subject to the Abel-Ruffini limitation; arbitrary degree-five and higher polynomials do not necessarily have radical solutions.

## Project Layout

- `src/symbolic.*`: SymEngine scalar adapter
- `src/matrix.*`: application-owned symbolic matrix type
- `src/algorithms.*`: linear algebra and educational reports
- `src/expression.*`: restricted matrix-expression parser
- `src/mainwindow.*`: Qt desktop interface and session state
- `tests/core_tests.cpp`: core regression tests
- `scripts/package-windows.ps1`: portable-folder deployment

The original Python file remains in the project as a behavior reference and is not needed by the compiled application.
