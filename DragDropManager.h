#pragma once

#include <vector>
#include <functional>
#include <string>
#include "imgui.h"

struct DragDropItem {
    int id;
    std::string name;
    std::string type;
    void* userData;
    
    DragDropItem(int itemId, const std::string& itemName, const std::string& itemType, void* data = nullptr)
        : id(itemId), name(itemName), type(itemType), userData(data) {}
};

class DragDropManager {
public:
    using DropCallback = std::function<void(const DragDropItem& source, const DragDropItem& target)>;
    using ReorderCallback = std::function<void(int fromIndex, int toIndex)>;
    
    DragDropManager();
    ~DragDropManager();
    
    // Register callbacks
    void setDropCallback(DropCallback callback) { dropCallback_ = callback; }
    void setReorderCallback(ReorderCallback callback) { reorderCallback_ = callback; }
    
    // Drag & Drop operations
    bool beginDragDropSource(const DragDropItem& item);
    bool beginDragDropTarget(const DragDropItem& targetItem);
    void endDragDropSource();
    void endDragDropTarget();
    
    // Channel reordering helpers
    bool beginChannelReorder(int channelIndex, const std::string& channelName);
    bool handleChannelDropTarget(int targetChannelIndex);
    
    // Configuration payload types
    static const char* PAYLOAD_TYPE_CHANNEL;
    static const char* PAYLOAD_TYPE_OSC_ADDRESS;
    static const char* PAYLOAD_TYPE_PRESET;
    
    // Visual feedback
    void drawDropZone(const ImVec2& size, const char* text = "Drop Here");
    void drawDragPreview(const DragDropItem& item);
    
    // Utility functions
    bool isItemBeingDragged(int itemId) const;
    const DragDropItem* getCurrentDragItem() const;
    
private:
    DropCallback dropCallback_;
    ReorderCallback reorderCallback_;
    DragDropItem* currentDragItem_;
    bool isDragging_;
    
    // Internal helpers
    void handleDrop(const DragDropItem& source, const DragDropItem& target);
    void renderDragPreview(const DragDropItem& item);
};
