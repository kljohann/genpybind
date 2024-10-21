// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

#include <memory>

struct GENPYBIND(visible, holder_type("std::shared_ptr<Shared>")) Shared
    : public std::enable_shared_from_this<Shared> {
  std::shared_ptr<Shared> clone();
};

class GENPYBIND(visible) Holder {
public:
  Holder();
  std::shared_ptr<Shared> getShared();
  long uses() const;

private:
  std::shared_ptr<Shared> shared;
};
