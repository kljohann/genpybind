// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --dump-graph=visibility --verbose %s -- %INCLUDES% -xc 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

// NOTE: This file used to be parsed as C code, so records weren't CXXRecordDecls.

// CHECK:      Adjusting command for file {{.*}}record-types-crash.h to
// CHECK-NEXT: -xc++

typedef union {
} Union;
typedef struct {
} Struct;

// CHECK:      Declaration context graph (unpruned) with visibility of all nodes:
// CHECK-NEXT: |-CXXRecord 'Union': hidden
// CHECK-NEXT: `-CXXRecord 'Struct': hidden
