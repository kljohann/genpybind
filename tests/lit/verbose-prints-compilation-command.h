// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --verbose %s -- %INCLUDES% 2>&1 \
// RUN: -DEXAMPLE_MARKER \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

// NOTE: The trailing `--` above should prevent genpybind to infer the command from
// compile_commands.json.  This is tested using `{{$}}` below.

// CHECK:      Analyzing file {{.*}}verbose-prints-compilation-command.h {{.*}} using command{{ *$}}
// CHECK-NEXT: -DEXAMPLE_MARKER

// CHECK:      Adjusting command for file {{.*}}verbose-prints-compilation-command.h to
// CHECK-NEXT: -DEXAMPLE_MARKER{{.*}}-fsyntax-only
