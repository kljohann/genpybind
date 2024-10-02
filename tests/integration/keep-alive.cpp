// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "keep-alive.h"

Resource::Resource() { ++created, ++alive; }
Resource::~Resource() { ++destroyed, --alive; }

int Resource::created = 0;
int Resource::destroyed = 0;
int Resource::alive = 0;

Container::Container() { ++created, ++alive; }
Container::~Container() { ++destroyed, --alive; }
Container::Container(Resource *) : Container() {}
void Container::unannotated_sink(Resource *) {}
void Container::keep_alive_sink(Resource *) {}
Resource *Container::unannotated_source() { return new Resource(); }
Resource *Container::keep_alive_source() { return new Resource(); }
Resource *Container::reverse_keep_alive_source() { return new Resource(); }

int Container::created = 0;
int Container::destroyed = 0;
int Container::alive = 0;

void link(Container *, Resource *) {}
