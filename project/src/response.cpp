#include <string>
#include <memory>
#include <vector>
#include <print>

#include "detection.h"
#include "entity_model.h"
#include "event.h"
#include "os_handle.h"
#include "response.h"
#include "text.h"
#include "fields.h"

namespace nano_edr {

// --- ResponsePlanner -------------------------------------------------------

ResponsePlanner::ResponsePlanner(const EntityModel* model,
                                 std::string quarantine_dir)
    : model_(model), quarantine_dir_(quarantine_dir) {}

ResponsePlan ResponsePlanner::Plan(const Detection& detection) const {
    ResponsePlan plan;
    const std::vector<std::string> tree = model_->GetProcessTree(detection.pid);
    for (const std::string& name : detection.actions) {
        if (name == "kill_process") {
            for (std::size_t i = tree.size(); i > 0; --i) {
            const std::string& pid = tree[i - 1];
            Action action;
            action.kind = ActionKind::kKillProcess;
            action.pid = pid;
            action.expected_start = model_->FindProcess(pid)->start;
            action.reason = detection.rule;
            if (pid != detection.pid) {
                action.reason += ", потомок " + detection.pid;
            }
            plan.Add(std::move(action));
            }
        } else if (name == "quarantine_source") {
            if (model_) {
                const FileRecord* source = model_->FindSpawnSource(detection.pid);
                if (source != nullptr) {
                    Action action;
                    action.kind = ActionKind::kQuarantineFile;
                    action.path = source->path;
                    action.to = QuarantinePath(source->path, detection.pid);
                    if (action.to.empty() || StartIsLowerCase("C:\\Windows\\", action.path) || StartIsLowerCase("C:\\Program Files", action.path)) {
                        continue;
                    }
                    action.reason = detection.rule + ", источник запуска";
                    plan.Add(std::move(action));
                }
            }
        } else if (name == "quarantine_image") {
            if (model_) {
                const ProcessRecord* image = model_->FindProcess(detection.pid);
                if (image != nullptr) {
                    Action action;
                    action.kind = ActionKind::kQuarantineFile;
                    action.path = image->image;
                    action.to = QuarantinePath(image->image, detection.pid);
                    if (action.to.empty() || StartIsLowerCase("C:\\Windows\\", action.path) || StartIsLowerCase("C:\\Program Files", action.path)) {
                        continue;
                    }
                    action.reason = detection.rule + ", образ процесса";
                    plan.Add(std::move(action));
                }
            }
        } else if (name == "delete_dropped") {
            for (const std::string& pid : tree) {
                const std::vector<std::string> files = model_->GetFilesCreatedBy(pid, true);
                for (const std::string& path : files) {
                    if (StartIsLowerCase("C:\\Windows\\", path) || StartIsLowerCase("C:\\Program Files", path)) {
                        continue;
                    }
                    Action action;
                    action.kind = ActionKind::kDeleteFile;
                    action.path = path;
                    action.reason = detection.rule + ", принесён " + pid;
                    plan.Add(std::move(action));
                }
            }
        } else if (name == "delete_evidence") {
            for (const std::unique_ptr<Event>& event : detection.evidence) {
                if (event == nullptr) {
                    continue;
                }
                const std::string* path =
                    event->event_type() == EventType::kFileMove ? FindField(*event, "to") : FindField(*event, "path");
                if (path == nullptr || StartIsLowerCase("C:\\Windows\\", *path) || StartIsLowerCase("C:\\Program Files", *path)) {
                    continue;
                }
                Action action;
                action.kind = ActionKind::kDeleteFile;
                action.path = *path;
                action.reason = detection.rule + ", файл из детекта";
                plan.Add(std::move(action));
            }
        }
    }
    return plan;
}
std::string ResponsePlanner::QuarantinePath(const std::string& path,
                                            const std::string& pid) const {
    if (quarantine_dir_.empty()) {
        return "";
    }
    std::size_t slash_pos = path.find_last_of('\\');
    if (slash_pos == std::string::npos) {
        slash_pos = 0;
    } else {
        ++slash_pos;
    }
    return quarantine_dir_ + '\\' + pid + '-' + path.substr(slash_pos);
}

// --- Responder -------------------------------------------------------------

Responder::Responder(OsHandle* os, bool dry_run) : os_(os), dry_run_(dry_run) {}

void Responder::Execute(const ResponsePlan& plan) {
    for (const Action& action : plan.actions()) {
        ++stats_.planned;

        // цель, по которой уже отработали: на одном процессе срабатывает
        // несколько правил
        bool repeat = false;
        for (const std::string& target : done_) {
            if (target == action.target()) {
                repeat = true;
                break;
            }
        }
        if (repeat) {
            ++stats_.skipped_repeat;
            continue;
        }
        done_.push_back(action.target());

        if (dry_run_ || os_ == nullptr) {
            std::print("[PLAN] {}\n", ToString(action));
            continue;
        }
        switch (action.kind) {
            case ActionKind::kKillProcess:
                Kill(action);
                break;
            case ActionKind::kQuarantineFile:
                MoveToQuarantine(action);
                break;
            case ActionKind::kDeleteFile:
                Delete(action);
                break;
        }
    }
}

}  // namespace nano_edr
