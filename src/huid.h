// Copyright (C) 2026  Alexey Kutepov <reximkut@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, see <https://www.gnu.org/licenses/>.
#ifndef HUID_H_
#define HUID_H_

#define HUID_REGEXP_FOR_USER_REPORT_PURPOSES "/[0-9]{8}-[0-9]{6}(-[a-zA-Z0-9\\-]*)?/"

bool chop_huid(String_View *content, String_View *huid);
bool is_valid_huid(const char *id);
char *temp_new_huid(const char *suffix);

#endif // HUID_H_
