# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import subprocess
import sys
from pathlib import Path


def tool():
    path = Path(__file__).parent.parent / "bin/genpybind-tool"
    sys.exit(subprocess.call([path] + sys.argv[1:]))
