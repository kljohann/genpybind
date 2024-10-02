# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import return_value_policy as m


def test_readwrite_field_can_be_changed():
    inst = m.Example()
    assert inst.value == 0
    inst.number.value = 5
    assert inst.value == 5


def test_unannotated_references_returned_by_copy():
    inst = m.Example()
    assert inst.value == 0

    inst.reference().value = 5
    assert inst.value == 0

    inst.constReference().value = 5
    assert inst.value == 0


def test_return_value_policy_can_be_set_to_copy_explicitly():
    inst = m.Example()
    assert inst.value == 0

    inst.referenceAsCopy().value = 5
    assert inst.value == 0

    inst.constReferenceAsCopy().value = 5
    assert inst.value == 0


def test_internal_references_can_be_returned():
    inst = m.Example()
    assert inst.value == 0

    inst.referenceAsInternalReference().value = 5
    assert inst.value == 5

    inst.constReferenceAsInternalReference().value = 15
    assert inst.value == 15
