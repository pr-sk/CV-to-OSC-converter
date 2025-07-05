#include "CommandSystem.h"
#include "GuiApplication.h"

// ChangeChannelNameCommand
ChangeChannelNameCommand::ChangeChannelNameCommand(CVChannelData* channel, const std::string& newName)
    : channel_(channel), newName_(newName) {
    if (channel_) {
        oldName_ = channel_->name;
    }
}

void ChangeChannelNameCommand::execute() {
    if (channel_) {
        channel_->name = newName_;
    }
}

void ChangeChannelNameCommand::undo() {
    if (channel_) {
        channel_->name = oldName_;
    }
}

std::string ChangeChannelNameCommand::getDescription() const {
    return "Change channel name to '" + newName_ + "'";
}

// ChangeChannelRangeCommand
ChangeChannelRangeCommand::ChangeChannelRangeCommand(CVChannelData* channel, float newMin, float newMax)
    : channel_(channel), newMin_(newMin), newMax_(newMax) {
    if (channel_) {
        oldMin_ = channel_->minRange;
        oldMax_ = channel_->maxRange;
    }
}

void ChangeChannelRangeCommand::execute() {
    if (channel_) {
        channel_->minRange = newMin_;
        channel_->maxRange = newMax_;
    }
}

void ChangeChannelRangeCommand::undo() {
    if (channel_) {
        channel_->minRange = oldMin_;
        channel_->maxRange = oldMax_;
    }
}

std::string ChangeChannelRangeCommand::getDescription() const {
    return "Change channel range to [" + std::to_string(newMin_) + ", " + std::to_string(newMax_) + "]";
}

// ToggleChannelCommand
ToggleChannelCommand::ToggleChannelCommand(CVChannelData* channel)
    : channel_(channel) {
    if (channel_) {
        oldState_ = channel_->enabled;
    }
}

void ToggleChannelCommand::execute() {
    if (channel_) {
        channel_->enabled = !channel_->enabled;
    }
}

void ToggleChannelCommand::undo() {
    if (channel_) {
        channel_->enabled = oldState_;
    }
}

std::string ToggleChannelCommand::getDescription() const {
    return oldState_ ? "Disable channel" : "Enable channel";
}

// ChangeOSCAddressCommand
ChangeOSCAddressCommand::ChangeOSCAddressCommand(CVChannelData* channel, const std::string& newAddress)
    : channel_(channel), newAddress_(newAddress) {
    if (channel_) {
        oldAddress_ = channel_->oscAddress;
    }
}

void ChangeOSCAddressCommand::execute() {
    if (channel_) {
        channel_->oscAddress = newAddress_;
    }
}

void ChangeOSCAddressCommand::undo() {
    if (channel_) {
        channel_->oscAddress = oldAddress_;
    }
}

std::string ChangeOSCAddressCommand::getDescription() const {
    return "Change OSC address to '" + newAddress_ + "'";
}
