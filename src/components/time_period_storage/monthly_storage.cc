/* Copyright 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/time_period_storage/monthly_storage.h"

namespace {
constexpr size_t kAverageDaysInMonth = 30;
}

MonthlyStorage::MonthlyStorage(PrefService* prefs, const char* pref_name)
    : TimePeriodStorage(prefs, pref_name, kAverageDaysInMonth) {}

uint64_t MonthlyStorage::GetMonthlySum() const {
  return GetPeriodSum();
}

uint64_t MonthlyStorage::GetHighestValueInMonth() const {
  return GetHighestValueInPeriod();
}
