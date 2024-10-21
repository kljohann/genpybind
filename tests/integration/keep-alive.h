// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

struct GENPYBIND(visible) Resource final {
  Resource();
  ~Resource();

  GENPYBIND(readonly)
  static int created;
  GENPYBIND(readonly)
  static int destroyed;
  GENPYBIND(readonly)
  static int alive;
};

struct GENPYBIND(visible) Container final {
  Container();
  ~Container();

  GENPYBIND(keep_alive(this, resource))
  Container(Resource *resource);

  void unannotated_sink(Resource *resource);

  GENPYBIND(keep_alive(this, resource))
  void keep_alive_sink(Resource *resource);

  Resource *unannotated_source();

  GENPYBIND(keep_alive(this, "return"))
  Resource *keep_alive_source();

  GENPYBIND(keep_alive(return, this))
  Resource *reverse_keep_alive_source();

  GENPYBIND(readonly)
  static int created;
  GENPYBIND(readonly)
  static int destroyed;
  GENPYBIND(readonly)
  static int alive;
};

void link(Container *container, Resource *resource)
    GENPYBIND(keep_alive(container, resource));
