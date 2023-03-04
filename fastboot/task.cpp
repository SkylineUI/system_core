//
// Copyright (C) 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "task.h"
#include "fastboot.h"
#include "util.h"

#include "fastboot.h"
#include "util.h"

FlashTask::FlashTask(const std::string& _slot, const std::string& _pname)
    : pname_(_pname), fname_(find_item(_pname)), slot_(_slot) {
    if (fname_.empty()) die("cannot determine image filename for '%s'", pname_.c_str());
}
FlashTask::FlashTask(const std::string& _slot, const std::string& _pname, const std::string& _fname)
    : pname_(_pname), fname_(_fname), slot_(_slot) {}

void FlashTask::Run() {
    auto flash = [&](const std::string& partition) {
        if (should_flash_in_userspace(partition) && !is_userspace_fastboot()) {
            die("The partition you are trying to flash is dynamic, and "
                "should be flashed via fastbootd. Please run:\n"
                "\n"
                "    fastboot reboot fastboot\n"
                "\n"
                "And try again. If you are intentionally trying to "
                "overwrite a fixed partition, use --force.");
        }
        do_flash(partition.c_str(), fname_.c_str());
    };
    do_for_partitions(pname_, slot_, flash, true);
}

RebootTask::RebootTask(FlashingPlan* _fp) : fp_(_fp){};
RebootTask::RebootTask(FlashingPlan* _fp, const std::string& _reboot_target)
    : reboot_target_(_reboot_target), fp_(_fp){};

void RebootTask::Run() {
    if ((reboot_target_ == "userspace" || reboot_target_ == "fastboot")) {
        if (!is_userspace_fastboot()) {
            reboot_to_userspace_fastboot();
            fp_->fb->WaitForDisconnect();
        }
    } else if (reboot_target_ == "recovery") {
        fp_->fb->RebootTo("recovery");
        fp_->fb->WaitForDisconnect();
    } else if (reboot_target_ == "bootloader") {
        fp_->fb->RebootTo("bootloader");
        fp_->fb->WaitForDisconnect();
    } else if (reboot_target_ == "") {
        fp_->fb->Reboot();
        fp_->fb->WaitForDisconnect();
    } else {
        syntax_error("unknown reboot target %s", reboot_target_.c_str());
    }
}
