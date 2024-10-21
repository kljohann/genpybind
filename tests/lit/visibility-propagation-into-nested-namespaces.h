// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --dump-graph=visibility %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

#define GENPYBIND_VISIBLE GENPYBIND(visible)
#define GENPYBIND_HIDDEN GENPYBIND(hidden)

// CHECK:      Declaration context graph (unpruned) with visibility of all nodes:

// CHECK-NEXT: |-CXXRecord 'Unannotated': hidden
class Unannotated {};
// CHECK-NEXT: |-CXXRecord 'Visible': visible
class GENPYBIND(visible) Visible {};
// CHECK-NEXT: |-CXXRecord 'Default': hidden
class GENPYBIND(visible(default)) Default {};
// CHECK-NEXT: |-CXXRecord 'Hidden': hidden
class GENPYBIND(hidden) Hidden {};

void unannotated();
void visible() GENPYBIND(visible);
void defaulted() GENPYBIND(visible(default));
void hidden() GENPYBIND(hidden);

// CHECK-NOT: LinkageSpec
extern "C" {
void with_linkage();
}

// CHECK-NEXT: |-Namespace 'in_unannotated_namespace': hidden
namespace in_unannotated_namespace {
// CHECK-NEXT: | |-CXXRecord 'in_unannotated_namespace::UnannotatedInNs': hidden
class UnannotatedInNs {};
// CHECK-NEXT: | |-CXXRecord 'in_unannotated_namespace::VisibleInNs': visible
class GENPYBIND(visible) VisibleInNs {};
// CHECK-NEXT: | |-CXXRecord 'in_unannotated_namespace::DefaultInNs': hidden
class GENPYBIND(visible(default)) DefaultInNs {};
// CHECK-NEXT: | |-CXXRecord 'in_unannotated_namespace::HiddenInNs': hidden
class GENPYBIND(hidden) HiddenInNs {};

void unannotated_in_ns();
void visible_in_ns() GENPYBIND(visible);
void defaulted_in_ns() GENPYBIND(visible(default));
void hidden_in_ns() GENPYBIND(hidden);

// CHECK-NOT: LinkageSpec
extern "C" {
// CHECK-NEXT: | `-CXXRecord 'in_unannotated_namespace::WithLinkageInNs': hidden
class WithLinkageInNs {};
void with_linkage_in_ns();
}
} // namespace in_unannotated_namespace

// CHECK-NEXT: |-Namespace 'in_visible_namespace': visible
namespace in_visible_namespace GENPYBIND_VISIBLE {
// CHECK-NEXT: | |-CXXRecord 'in_visible_namespace::UnannotatedInVisible': visible
class UnannotatedInVisible {};
// CHECK-NEXT: | |-CXXRecord 'in_visible_namespace::VisibleInVisible': visible
class GENPYBIND(visible) VisibleInVisible {};
// CHECK-NEXT: | |-CXXRecord 'in_visible_namespace::DefaultInVisible': visible
class GENPYBIND(visible(default)) DefaultInVisible {};
// CHECK-NEXT: | |-CXXRecord 'in_visible_namespace::HiddenInVisible': hidden
class GENPYBIND(hidden) HiddenInVisible {};

void unannotated_in_visible();
void visible_in_visible() GENPYBIND(visible);
void defaulted_in_visible() GENPYBIND(visible(default));
void hidden_in_visible() GENPYBIND(hidden);

// CHECK-NOT: LinkageSpec
extern "C" {
// CHECK-NEXT: | |-CXXRecord 'in_visible_namespace::WithLinkageInVisible': visible
class WithLinkageInVisible {};
void with_linkage_in_visible();
}

// CHECK-NEXT: | |-Namespace 'in_visible_namespace::in_nested_hidden_namespace': hidden
namespace in_nested_hidden_namespace GENPYBIND_HIDDEN {
// CHECK-NEXT: | | |-CXXRecord 'in_visible_namespace::in_nested_hidden_namespace::UnannotatedInHiddenInVisible': hidden
class UnannotatedInHiddenInVisible {};
// CHECK-NEXT: | | |-CXXRecord 'in_visible_namespace::in_nested_hidden_namespace::VisibleInHiddenInVisible': visible
class GENPYBIND(visible) VisibleInHiddenInVisible {};
// CHECK-NEXT: | | |-CXXRecord 'in_visible_namespace::in_nested_hidden_namespace::DefaultInHiddenInVisible': hidden
class GENPYBIND(visible(default)) DefaultInHiddenInVisible {};
// CHECK-NEXT: | | |-CXXRecord 'in_visible_namespace::in_nested_hidden_namespace::HiddenInHiddenInVisible': hidden
class GENPYBIND(hidden) HiddenInHiddenInVisible {};

void unannotated_in_hidden_in_visible();
void visible_in_hidden_in_visible() GENPYBIND(visible);
void defaulted_in_hidden_in_visible() GENPYBIND(visible(default));
void hidden_in_hidden_in_visible() GENPYBIND(hidden);

// CHECK-NOT: LinkageSpec
extern "C" {
// CHECK-NEXT: | | `-CXXRecord 'in_visible_namespace::in_nested_hidden_namespace::WithLinkageInHiddenInVisible': hidden
class WithLinkageInHiddenInVisible {};
void with_linkage_in_hidden_in_visible();
}
} // namespace GENPYBIND_HIDDEN

// CHECK-NEXT: | `-Namespace 'in_visible_namespace::in_nested_unannotated_namespace': visible
namespace in_nested_unannotated_namespace {
// CHECK-NEXT: |   |-CXXRecord 'in_visible_namespace::in_nested_unannotated_namespace::UnannotatedInNsInVisible': visible
class UnannotatedInNsInVisible {};
// CHECK-NEXT: |   |-CXXRecord 'in_visible_namespace::in_nested_unannotated_namespace::VisibleInNsInVisible': visible
class GENPYBIND(visible) VisibleInNsInVisible {};
// CHECK-NEXT: |   |-CXXRecord 'in_visible_namespace::in_nested_unannotated_namespace::DefaultInNsInVisible': visible
class GENPYBIND(visible(default)) DefaultInNsInVisible {};
// CHECK-NEXT: |   |-CXXRecord 'in_visible_namespace::in_nested_unannotated_namespace::HiddenInNsInVisible': hidden
class GENPYBIND(hidden) HiddenInNsInVisible {};

void unannotated_in_ns_in_visible();
void visible_in_ns_in_visible() GENPYBIND(visible);
void defaulted_in_ns_in_visible() GENPYBIND(visible(default));
void hidden_in_ns_in_visible() GENPYBIND(hidden);

// CHECK-NOT: LinkageSpec
extern "C" {
// CHECK-NEXT: |   `-CXXRecord 'in_visible_namespace::in_nested_unannotated_namespace::WithLinkageInNsInVisible': visible
class WithLinkageInNsInVisible {};
void with_linkage_in_ns_in_visible();
}
} // namespace in_nested_unannotated_namespace
} // namespace GENPYBIND_VISIBLE

// CHECK-NEXT: `-Namespace 'in_hidden_namespace': hidden
namespace in_hidden_namespace GENPYBIND_HIDDEN {
// CHECK-NEXT:   |-CXXRecord 'in_hidden_namespace::UnannotatedInHidden': hidden
class UnannotatedInHidden {};
// CHECK-NEXT:   |-CXXRecord 'in_hidden_namespace::VisibleInHidden': visible
class GENPYBIND(visible) VisibleInHidden {};
// CHECK-NEXT:   |-CXXRecord 'in_hidden_namespace::DefaultInHidden': hidden
class GENPYBIND(visible(default)) DefaultInHidden {};
// CHECK-NEXT:   |-CXXRecord 'in_hidden_namespace::HiddenInHidden': hidden
class GENPYBIND(hidden) HiddenInHidden {};

void unannotated_in_hidden();
void visible_in_hidden() GENPYBIND(visible);
void defaulted_in_hidden() GENPYBIND(visible(default));
void hidden_in_hidden() GENPYBIND(hidden);

// CHECK-NOT: LinkageSpec
extern "C" {
// CHECK-NEXT:   `-CXXRecord 'in_hidden_namespace::WithLinkageInHidden': hidden
class WithLinkageInHidden {};
void with_linkage_in_hidden();
}
} // namespace GENPYBIND_HIDDEN
