// RUN: genpybind-tool --dump-graph=visibility %s -- -xc -D__GENPYBIND__ 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

// NOTE: This file is parsed as C code, records aren't CXXRecordDecls.

typedef union {} Union;
typedef struct {} Struct;

// CHECK:      Declaration context graph (unpruned) with visibility of all nodes:
// CHECK-NEXT: |-Record 'Union': hidden
// CHECK-NEXT: `-Record 'Struct': hidden
