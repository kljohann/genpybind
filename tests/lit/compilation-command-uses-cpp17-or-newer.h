// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --verbose %s -- %INCLUDES% -std=gnu++14 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

// CHECK:      Adjusting command for file {{.*}}cpp17-or-newer.h to
// CHECK-NOT: gnu++14
// CHECK-NEXT: -xc++{{.*}}cpp17-or-newer.h{{.*}}-std=gnu++17
