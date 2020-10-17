import os

import manual as m


def test_manual_method():
    inst = m.Example()
    assert inst.manual_method()
    assert not inst[0]
    assert not inst[1]
    inst[1] = True
    assert inst[1]
    inst[0] = True
    assert inst[0]

    m.Hidden()


def test_order_of_execution_matches_context_graph_hierarchy():
    order_of_execution = os.environ.get("order_of_execution", "")
    assert order_of_execution == "preamble,namespace,class,postamble"
