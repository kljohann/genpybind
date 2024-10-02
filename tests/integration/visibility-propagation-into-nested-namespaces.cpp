// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "visibility-propagation-into-nested-namespaces.h"

void unannotated() {}
void visible() {}
void defaulted() {}
void hidden() {}
void with_linkage() {}

void in_unannotated_namespace::unannotated_in_ns() {}
void in_unannotated_namespace::visible_in_ns() {}
void in_unannotated_namespace::defaulted_in_ns() {}
void in_unannotated_namespace::hidden_in_ns() {}
void in_unannotated_namespace::with_linkage_in_ns() {}

void in_visible_namespace::unannotated_in_visible() {}
void in_visible_namespace::visible_in_visible() {}
void in_visible_namespace::defaulted_in_visible() {}
void in_visible_namespace::hidden_in_visible() {}
void in_visible_namespace::with_linkage_in_visible() {}

void in_visible_namespace::in_nested_hidden_namespace::
    unannotated_in_hidden_in_visible() {}
void in_visible_namespace::in_nested_hidden_namespace::
    visible_in_hidden_in_visible() {}
void in_visible_namespace::in_nested_hidden_namespace::
    defaulted_in_hidden_in_visible() {}
void in_visible_namespace::in_nested_hidden_namespace::
    hidden_in_hidden_in_visible() {}
void in_visible_namespace::in_nested_hidden_namespace::
    with_linkage_in_hidden_in_visible() {}

void in_visible_namespace::in_nested_unannotated_namespace::
    unannotated_in_ns_in_visible() {}
void in_visible_namespace::in_nested_unannotated_namespace::
    visible_in_ns_in_visible() {}
void in_visible_namespace::in_nested_unannotated_namespace::
    defaulted_in_ns_in_visible() {}
void in_visible_namespace::in_nested_unannotated_namespace::
    hidden_in_ns_in_visible() {}
void in_visible_namespace::in_nested_unannotated_namespace::
    with_linkage_in_ns_in_visible() {}

void in_hidden_namespace::unannotated_in_hidden() {}
void in_hidden_namespace::visible_in_hidden() {}
void in_hidden_namespace::defaulted_in_hidden() {}
void in_hidden_namespace::hidden_in_hidden() {}
void in_hidden_namespace::with_linkage_in_hidden() {}
