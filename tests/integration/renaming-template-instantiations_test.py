# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import pytest


@pytest.mark.xfail(reason="known defect", raises=ImportError, strict=True)
def test_expected_instantiations_are_present():
    with pytest.raises(ImportError, match='"_Renamed".*is already defined') as err:
        import renaming_template_instantiations  # noqa: F401
    raise err.value
