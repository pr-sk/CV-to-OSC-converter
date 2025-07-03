#include "DragDropManager.h"
#include <iostream>

// Static payload type constants
const char* DragDropManager::PAYLOAD_TYPE_CHANNEL = "CHANNEL";
const char* DragDropManager::PAYLOAD_TYPE_OSC_ADDRESS = "OSC_ADDRESS";
const char* DragDropManager::PAYLOAD_TYPE_PRESET = "PRESET";

DragDropManager::DragDropManager() 
    : currentDragItem_(nullptr), isDragging_(false) {
}

DragDropManager::~DragDropManager() {
    delete currentDragItem_;
}

bool DragDropManager::beginDragDropSource(const DragDropItem& item) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        // Store drag item data
        delete currentDragItem_;
        currentDragItem_ = new DragDropItem(item);
        isDragging_ = true;
        
        // Set payload based on item type
        if (item.type == PAYLOAD_TYPE_CHANNEL) {
            ImGui::SetDragDropPayload(PAYLOAD_TYPE_CHANNEL, &item.id, sizeof(int));
        } else if (item.type == PAYLOAD_TYPE_OSC_ADDRESS) {
            ImGui::SetDragDropPayload(PAYLOAD_TYPE_OSC_ADDRESS, item.name.c_str(), item.name.size() + 1);
        } else if (item.type == PAYLOAD_TYPE_PRESET) {
            ImGui::SetDragDropPayload(PAYLOAD_TYPE_PRESET, &item, sizeof(DragDropItem));
        }
        
        // Show drag preview
        renderDragPreview(item);
        
        return true;
    }
    return false;
}

bool DragDropManager::beginDragDropTarget(const DragDropItem& targetItem) {
    if (ImGui::BeginDragDropTarget()) {
        // Handle different payload types
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PAYLOAD_TYPE_CHANNEL)) {
            int sourceChannelId = *(const int*)payload->Data;
            if (currentDragItem_ && currentDragItem_->id != targetItem.id) {
                handleDrop(*currentDragItem_, targetItem);
            }
            endDragDropTarget();
            return true;
        }
        
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PAYLOAD_TYPE_OSC_ADDRESS)) {
            std::string oscAddress((const char*)payload->Data);
            if (currentDragItem_) {
                DragDropItem oscItem(-1, oscAddress, PAYLOAD_TYPE_OSC_ADDRESS);
                handleDrop(oscItem, targetItem);
            }
            endDragDropTarget();
            return true;
        }
        
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PAYLOAD_TYPE_PRESET)) {
            const DragDropItem* presetItem = (const DragDropItem*)payload->Data;
            if (presetItem) {
                handleDrop(*presetItem, targetItem);
            }
            endDragDropTarget();
            return true;
        }
        
        return false;
    }
    return false;
}

void DragDropManager::endDragDropSource() {
    ImGui::EndDragDropSource();
    if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        delete currentDragItem_;
        currentDragItem_ = nullptr;
        isDragging_ = false;
    }
}

void DragDropManager::endDragDropTarget() {
    ImGui::EndDragDropTarget();
}

bool DragDropManager::beginChannelReorder(int channelIndex, const std::string& channelName) {
    DragDropItem channelItem(channelIndex, channelName, PAYLOAD_TYPE_CHANNEL);
    return beginDragDropSource(channelItem);
}

bool DragDropManager::handleChannelDropTarget(int targetChannelIndex) {
    DragDropItem targetItem(targetChannelIndex, "Target Channel", PAYLOAD_TYPE_CHANNEL);
    
    if (beginDragDropTarget(targetItem)) {
        return true;
    }
    
    return false;
}

void DragDropManager::drawDropZone(const ImVec2& size, const char* text) {
    ImVec2 cursor_pos = ImGui::GetCursorPos();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    
    // Draw drop zone background
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImU32 col_bg = ImGui::GetColorU32(ImGuiCol_FrameBg);
    ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
    
    // Check if we're hovering over a valid drop target
    bool is_hovered = false;
    if (isDragging_ && ImGui::IsMouseHoveringRect(canvas_pos, ImVec2(canvas_pos.x + size.x, canvas_pos.y + size.y))) {
        col_bg = ImGui::GetColorU32(ImGuiCol_FrameBgHovered);
        col_border = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
        is_hovered = true;
    }
    
    // Draw background and border
    draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + size.x, canvas_pos.y + size.y), col_bg);
    draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + size.x, canvas_pos.y + size.y), col_border, 4.0f, 0, 2.0f);
    
    // Draw text
    ImVec2 text_size = ImGui::CalcTextSize(text);
    ImVec2 text_pos = ImVec2(
        canvas_pos.x + (size.x - text_size.x) * 0.5f,
        canvas_pos.y + (size.y - text_size.y) * 0.5f
    );
    
    ImU32 text_col = ImGui::GetColorU32(is_hovered ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    draw_list->AddText(text_pos, text_col, text);
    
    // Advance cursor
    ImGui::SetCursorPos(ImVec2(cursor_pos.x, cursor_pos.y + size.y));
}

void DragDropManager::drawDragPreview(const DragDropItem& item) {
    renderDragPreview(item);
}

bool DragDropManager::isItemBeingDragged(int itemId) const {
    return isDragging_ && currentDragItem_ && currentDragItem_->id == itemId;
}

const DragDropItem* DragDropManager::getCurrentDragItem() const {
    return currentDragItem_;
}

void DragDropManager::handleDrop(const DragDropItem& source, const DragDropItem& target) {
    if (source.type == PAYLOAD_TYPE_CHANNEL && target.type == PAYLOAD_TYPE_CHANNEL) {
        // Handle channel reordering
        if (reorderCallback_) {
            reorderCallback_(source.id, target.id);
        }
    } else {
        // Handle general drop operations
        if (dropCallback_) {
            dropCallback_(source, target);
        }
    }
    
    std::cout << "Drag & Drop: " << source.name << " -> " << target.name << std::endl;
}

void DragDropManager::renderDragPreview(const DragDropItem& item) {
    // Create a simple preview showing the item being dragged
    ImGui::BeginTooltip();
    
    if (item.type == PAYLOAD_TYPE_CHANNEL) {
        ImGui::Text("Channel: %s", item.name.c_str());
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Drop to reorder");
    } else if (item.type == PAYLOAD_TYPE_OSC_ADDRESS) {
        ImGui::Text("OSC: %s", item.name.c_str());
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Drop to assign");
    } else if (item.type == PAYLOAD_TYPE_PRESET) {
        ImGui::Text("Preset: %s", item.name.c_str());
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Drop to apply");
    } else {
        ImGui::Text("Item: %s", item.name.c_str());
    }
    
    ImGui::EndTooltip();
}
