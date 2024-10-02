# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

from dataclasses import dataclass

import keep_alive as m
import pytest


@dataclass
class Snapshot:
    created: int
    destroyed: int
    alive: int

    @classmethod
    def create(cls, src):
        return cls(created=src.created, destroyed=src.destroyed, alive=src.alive)


@pytest.mark.parametrize("thing", [m.Resource, m.Container])
def test_thing_counts_number_of_instances(thing):
    before = Snapshot.create(thing)
    thing()
    now = Snapshot.create(thing)
    assert before.alive == 0
    assert now.alive == 0
    assert now.created == before.created + 1
    assert now.destroyed == before.destroyed + 1


def test_constructor_can_keep_resource_alive():
    assert m.Container.alive == 0

    before = Snapshot.create(m.Resource)
    container = m.Container(m.Resource())
    now = Snapshot.create(m.Resource)

    assert m.Container.alive == 1
    assert before.alive == 0
    assert now.alive == 1
    assert now.created == before.created + 1
    assert now.destroyed == before.destroyed

    del container
    now = Snapshot.create(m.Resource)

    assert m.Container.alive == 0
    assert before.alive == 0
    assert now.alive == 0
    assert now.created == before.created + 1
    assert now.destroyed == before.destroyed + 1


def test_unannotated_sink_method_has_no_effect_on_lifetime():
    before = Snapshot.create(m.Resource)
    m.Resource()
    now = Snapshot.create(m.Resource)

    assert before.alive == 0
    assert now.alive == 0
    assert now.created == before.created + 1
    assert now.destroyed == before.destroyed + 1

    container = m.Container()

    before = Snapshot.create(m.Resource)
    container.unannotated_sink(m.Resource())
    now = Snapshot.create(m.Resource)

    assert before.alive == 0
    assert now.alive == 0
    assert now.created == before.created + 1
    assert now.destroyed == before.destroyed + 1


def test_sink_method_can_keep_resource_alive():
    assert m.Container.alive == 0
    container = m.Container()
    assert m.Container.alive == 1

    before = Snapshot.create(m.Resource)
    container.keep_alive_sink(m.Resource())
    now = Snapshot.create(m.Resource)

    assert before.alive == 0
    assert now.alive == 1
    assert now.created == before.created + 1
    assert now.destroyed == before.destroyed

    del container
    now = Snapshot.create(m.Resource)

    assert m.Container.alive == 0
    assert before.alive == 0
    assert now.alive == 0
    assert now.created == before.created + 1
    assert now.destroyed == before.destroyed + 1


def test_unannotated_source_method_has_no_effect_on_lifetime():
    container = m.Container()

    before = Snapshot.create(m.Resource)
    container.unannotated_source()
    now = Snapshot.create(m.Resource)

    assert before.alive == 0
    assert now.alive == 0
    assert now.created == before.created + 1
    assert now.destroyed == before.destroyed + 1


def test_source_method_can_keep_resource_alive():
    assert m.Container.alive == 0
    container = m.Container()
    assert m.Container.alive == 1

    before = Snapshot.create(m.Resource)
    container.keep_alive_source()
    now = Snapshot.create(m.Resource)

    assert before.alive == 0
    assert now.alive == 1
    assert now.created == before.created + 1
    assert now.destroyed == before.destroyed

    del container
    now = Snapshot.create(m.Resource)

    assert m.Container.alive == 0
    assert before.alive == 0
    assert now.alive == 0
    assert now.created == before.created + 1
    assert now.destroyed == before.destroyed + 1


def test_source_method_can_keep_container_alive():
    container_before = Snapshot.create(m.Container)
    container = m.Container()
    container_alive = Snapshot.create(m.Container)
    assert container_alive != container_before
    assert container_before.alive == 0
    assert container_alive.alive == 1

    assert m.Resource.alive == 0
    resource = container.reverse_keep_alive_source()
    assert m.Resource.alive == 1

    del container
    container_now = Snapshot.create(m.Container)

    assert container_now == container_alive

    del resource
    container_now = Snapshot.create(m.Container)

    assert m.Resource.alive == 0
    assert container_now.alive == 0
    assert container_now.created == container_before.created + 1
    assert container_now.destroyed == container_before.destroyed + 1


def test_free_function_can_keep_resource_alive():
    assert m.Container.alive == 0
    container = m.Container()
    assert m.Container.alive == 1

    before = Snapshot.create(m.Resource)
    m.link(container, m.Resource())
    now = Snapshot.create(m.Resource)

    assert before.alive == 0
    assert now.alive == 1
    assert now.created == before.created + 1
    assert now.destroyed == before.destroyed

    del container
    now = Snapshot.create(m.Resource)

    assert m.Container.alive == 0
    assert before.alive == 0
    assert now.alive == 0
    assert now.created == before.created + 1
    assert now.destroyed == before.destroyed + 1
