/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * miscservices under the License is miscservices on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MISCSERVICES_REPORTER_H
#define MISCSERVICES_REPORTER_H

#include "fault_reporter.h"
#include "statistic_reporter.h"

namespace OHOS {
namespace MiscServices {
class Reporter {
public:
    static Reporter &GetInstance();
    // fault
    FaultReporter &Fault();
    // statistic
    StatisticReporter &UsageTimeStatistic();
};
} // namespace MiscServices
} // namespace OHOS
#endif // MISCSERVICES_REPORTER_H
