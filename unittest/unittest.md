Unit Testing and Code Coverage Guide
====================================

This document outlines the procedure for executing unit tests and generating code coverage reports for the **SW GenLock library**, using the Unity C test framework in combination with `gcov` and `lcov`.

Setup Instructions
------------------

**1. Clone Unity Test Framework**

Clone the Unity test framework into the `unittest` directory (from project root folder):

```console
   $ git clone https://github.com/ThrowTheSwitch/Unity.git unittest/unity  --branch v2.6.1 --depth 1
```

This places `unity.c` and `unity.h` under `unittest/unity/src`.

Build the Library with Coverage Support
---------------------------------------

Navigate to the library directory and build the library with coverage instrumentation enabled:

```console
   $ cd lib
   $ make coverage
```

Both static and shared libraries are built with the necessary `gcov` flags (`-fprofile-arcs -ftest-coverage`). `.gcno` files should be generated in the `lib/obj/` directory corresponding to the compiled sources.

Build the Unit Tests
--------------------

To build the unit test binary:

```console
   $ cd ../unittest
   $ make
```
The resulting binary is typically named `swgenlock_tests` and is linked against the instrumented version of the library.

PHY Coverage Considerations
---------------------------

The SW GenLock library supports four PHY types:

- **Combo**
- **DKL**
- **C10**
- **C20**

Since a single system may not include support for all PHY types, complete code coverage requires executing the unit tests on multiple platforms. Each platform should support one or more of the target PHY types.

Run Tests and Capture Coverage
------------------------------

Clean any existing .gcda files from lib/obj folder

```console
   $ rm ../lib/obj/*.gcda
```

**On systems with Combo and DKL PHYs:**

1. Execute the test binary:

```console
   $ ./swgenlock_tests
```

   For m_n based test execution

```console
   $ ./swgenlock_tests --run-mn-test
```

2. Capture the code coverage data:

From within the unittest directory

```console
   $ lcov --capture --directory ../lib/obj --output-file combo_dkl.info
```

**On systems with other PHYs such as C10 or C20:**

Repeat the same earlier test execution procedure, capturing `.info` files separately as needed:

```console
   $ lcov --capture --directory ../lib/obj --output-file c10.info
```

```console
   $ lcov --capture --directory ../lib/obj --output-file c20.info
```

If the target system is connected to both C10 and C20 monitors, coverage data for both PHY types will be included in a single .info file. This file should be renamed to c10_c20.info for clarity.

Combine Coverage Files
----------------------

Once all `.info` files have been collected from the relevant systems, merge them into a single combined report:

```console
   $ lcov --add-tracefile combo_dkl.info --add-tracefile c10.info --add-tracefile c20.info --output-file final.info
```

This step may be adapted based on which `.info` files are available.


Optional: Remove coverage from system or external libraries:
------------------------------------------------------------

```console
   $ lcov --remove final.info '/usr/*' --ignore-errors unused --output-file final.info
```

Generate HTML Report
--------------------

To produce an HTML report for inspection:

```console
   $ genhtml final.info --output-directory coverage-report
```

The file `coverage-report/index.html` provides a browsable overview of code coverage across the SW GenLock library.

Notes
-----

- Only the **library source code** is included in the final coverage report. Reference applications are excluded.
- Ensure `.gcda` files are not unintentionally overwritten by multiple simultaneous test runs.
- Clean object directories and re-run tests to refresh coverage data where necessary.
