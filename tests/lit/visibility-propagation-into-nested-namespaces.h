// RUN: genpybind-tool -dump-graph=visibility %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

#define GENPYBIND_VISIBLE GENPYBIND(visible)
#define GENPYBIND_HIDDEN GENPYBIND(hidden)

// CHECK:      Declaration context graph after visibility propagation:

// CHECK-NEXT: |-CXXRecord 'Unannotated':
class Unannotated {};
// CHECK-NEXT: |-CXXRecord 'Visible': visible
class GENPYBIND(visible) Visible {};
// CHECK-NEXT: |-CXXRecord 'Default': visible(default)
class GENPYBIND(visible(default)) Default {};
// CHECK-NEXT: |-CXXRecord 'Hidden': hidden
class GENPYBIND(hidden) Hidden {};

// CHECK-NEXT: |-Namespace 'in_unannotated_namespace':
namespace in_unannotated_namespace {
// CHECK-NEXT: | |-CXXRecord 'in_unannotated_namespace::Unannotated':
class Unannotated {};
// CHECK-NEXT: | |-CXXRecord 'in_unannotated_namespace::Visible': visible
class GENPYBIND(visible) Visible {};
// CHECK-NEXT: | |-CXXRecord 'in_unannotated_namespace::Default': visible(default)
class GENPYBIND(visible(default)) Default {};
// CHECK-NEXT: | |-CXXRecord 'in_unannotated_namespace::Hidden': hidden
class GENPYBIND(hidden) Hidden {};

// CHECK-NEXT: | `-LinkageSpec:
extern "C" {
// CHECK-NEXT: |   `-CXXRecord 'in_unannotated_namespace::WithLinkage':
class WithLinkage {};
}
} // namespace in_unannotated_namespace

// CHECK-NEXT: |-Namespace 'in_visible_namespace': visible
namespace in_visible_namespace GENPYBIND_VISIBLE {
// CHECK-NEXT: | |-CXXRecord 'in_visible_namespace::Unannotated':
class Unannotated {};
// CHECK-NEXT: | |-CXXRecord 'in_visible_namespace::Visible': visible
class GENPYBIND(visible) Visible {};
// CHECK-NEXT: | |-CXXRecord 'in_visible_namespace::Default': visible(default)
class GENPYBIND(visible(default)) Default {};
// CHECK-NEXT: | |-CXXRecord 'in_visible_namespace::Hidden': hidden
class GENPYBIND(hidden) Hidden {};

// CHECK-NEXT: | |-LinkageSpec:
extern "C" {
// CHECK-NEXT: | | `-CXXRecord 'in_visible_namespace::WithLinkage':
class WithLinkage {};
}

// CHECK-NEXT: | |-Namespace 'in_visible_namespace::in_nested_hidden_namespace': hidden
namespace in_nested_hidden_namespace GENPYBIND_HIDDEN {
// CHECK-NEXT: | | |-CXXRecord 'in_visible_namespace::in_nested_hidden_namespace::Unannotated':
class Unannotated {};
// CHECK-NEXT: | | |-CXXRecord 'in_visible_namespace::in_nested_hidden_namespace::Visible': visible
class GENPYBIND(visible) Visible {};
// CHECK-NEXT: | | |-CXXRecord 'in_visible_namespace::in_nested_hidden_namespace::Default': visible(default)
class GENPYBIND(visible(default)) Default {};
// CHECK-NEXT: | | |-CXXRecord 'in_visible_namespace::in_nested_hidden_namespace::Hidden': hidden
class GENPYBIND(hidden) Hidden {};

// CHECK-NEXT: | | `-LinkageSpec:
extern "C" {
// CHECK-NEXT: | |   `-CXXRecord 'in_visible_namespace::in_nested_hidden_namespace::WithLinkage':
class WithLinkage {};
}
} // namespace GENPYBIND_HIDDEN

// CHECK-NEXT: | `-Namespace 'in_visible_namespace::in_nested_unannotated_namespace':
namespace in_nested_unannotated_namespace {
// CHECK-NEXT: |   |-CXXRecord 'in_visible_namespace::in_nested_unannotated_namespace::Unannotated':
class Unannotated {};
// CHECK-NEXT: |   |-CXXRecord 'in_visible_namespace::in_nested_unannotated_namespace::Visible': visible
class GENPYBIND(visible) Visible {};
// CHECK-NEXT: |   |-CXXRecord 'in_visible_namespace::in_nested_unannotated_namespace::Default': visible(default)
class GENPYBIND(visible(default)) Default {};
// CHECK-NEXT: |   |-CXXRecord 'in_visible_namespace::in_nested_unannotated_namespace::Hidden': hidden
class GENPYBIND(hidden) Hidden {};

// CHECK-NEXT: |   `-LinkageSpec:
extern "C" {
// CHECK-NEXT: |     `-CXXRecord 'in_visible_namespace::in_nested_unannotated_namespace::WithLinkage':
class WithLinkage {};
}
} // namespace in_nested_unannotated_namespace
} // namespace GENPYBIND_VISIBLE

// CHECK-NEXT: `-Namespace 'in_hidden_namespace': hidden
namespace in_hidden_namespace GENPYBIND_HIDDEN {
// CHECK-NEXT:   |-CXXRecord 'in_hidden_namespace::Unannotated':
class Unannotated {};
// CHECK-NEXT:   |-CXXRecord 'in_hidden_namespace::Visible': visible
class GENPYBIND(visible) Visible {};
// CHECK-NEXT:   |-CXXRecord 'in_hidden_namespace::Default': visible(default)
class GENPYBIND(visible(default)) Default {};
// CHECK-NEXT:   |-CXXRecord 'in_hidden_namespace::Hidden': hidden
class GENPYBIND(hidden) Hidden {};

// CHECK-NEXT:   `-LinkageSpec:
extern "C" {
// CHECK-NEXT:     `-CXXRecord 'in_hidden_namespace::WithLinkage':
class WithLinkage {};
}
} // namespace GENPYBIND_HIDDEN
