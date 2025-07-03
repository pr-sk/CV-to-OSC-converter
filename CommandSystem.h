#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>

// Базовый класс для команд
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getDescription() const = 0;
    virtual bool canUndo() const { return true; }
};

// Система управления командами с поддержкой undo/redo
class CommandManager {
private:
    std::vector<std::unique_ptr<Command>> undoStack_;
    std::vector<std::unique_ptr<Command>> redoStack_;
    static const size_t MAX_HISTORY_SIZE = 100;

public:
    void executeCommand(std::unique_ptr<Command> command) {
        if (command) {
            command->execute();
            
            // Добавляем в стек отмены только если команда поддерживает отмену
            if (command->canUndo()) {
                undoStack_.push_back(std::move(command));
                
                // Ограничиваем размер истории
                if (undoStack_.size() > MAX_HISTORY_SIZE) {
                    undoStack_.erase(undoStack_.begin());
                }
                
                // Очищаем стек повтора при выполнении новой команды
                redoStack_.clear();
            }
        }
    }
    
    bool canUndo() const {
        return !undoStack_.empty();
    }
    
    bool canRedo() const {
        return !redoStack_.empty();
    }
    
    void undo() {
        if (canUndo()) {
            auto command = std::move(undoStack_.back());
            undoStack_.pop_back();
            
            command->undo();
            redoStack_.push_back(std::move(command));
        }
    }
    
    void redo() {
        if (canRedo()) {
            auto command = std::move(redoStack_.back());
            redoStack_.pop_back();
            
            command->execute();
            undoStack_.push_back(std::move(command));
        }
    }
    
    std::string getLastCommandDescription() const {
        if (!undoStack_.empty()) {
            return undoStack_.back()->getDescription();
        }
        return "";
    }
    
    std::string getNextRedoDescription() const {
        if (!redoStack_.empty()) {
            return redoStack_.back()->getDescription();
        }
        return "";
    }
    
    void clear() {
        undoStack_.clear();
        redoStack_.clear();
    }
    
    size_t getUndoCount() const { return undoStack_.size(); }
    size_t getRedoCount() const { return redoStack_.size(); }
};

// Макро для удобного создания простых команд
#define MAKE_SIMPLE_COMMAND(name, executeCode, undoCode, description) \
class name##Command : public Command { \
private: \
    std::function<void()> executeFunc_; \
    std::function<void()> undoFunc_; \
public: \
    name##Command(std::function<void()> execFunc, std::function<void()> undoFunc) \
        : executeFunc_(execFunc), undoFunc_(undoFunc) {} \
    void execute() override { executeFunc_(); } \
    void undo() override { undoFunc_(); } \
    std::string getDescription() const override { return description; } \
};

// Конкретные команды для CV to OSC приложения
struct CVChannelData; // Forward declaration

class ChangeChannelNameCommand : public Command {
private:
    CVChannelData* channel_;
    std::string oldName_;
    std::string newName_;
    
public:
    ChangeChannelNameCommand(CVChannelData* channel, const std::string& newName);
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
};

class ChangeChannelRangeCommand : public Command {
private:
    CVChannelData* channel_;
    float oldMin_, oldMax_;
    float newMin_, newMax_;
    
public:
    ChangeChannelRangeCommand(CVChannelData* channel, float newMin, float newMax);
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
};

class ToggleChannelCommand : public Command {
private:
    CVChannelData* channel_;
    bool oldState_;
    
public:
    ToggleChannelCommand(CVChannelData* channel);
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
};

class ChangeOSCAddressCommand : public Command {
private:
    CVChannelData* channel_;
    std::string oldAddress_;
    std::string newAddress_;
    
public:
    ChangeOSCAddressCommand(CVChannelData* channel, const std::string& newAddress);
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
};

class BatchChannelCommand : public Command {
private:
    std::vector<std::unique_ptr<Command>> commands_;
    std::string description_;
    
public:
    BatchChannelCommand(const std::string& description) : description_(description) {}
    
    void addCommand(std::unique_ptr<Command> command) {
        commands_.push_back(std::move(command));
    }
    
    void execute() override {
        for (auto& cmd : commands_) {
            cmd->execute();
        }
    }
    
    void undo() override {
        // Отменяем в обратном порядке
        for (auto it = commands_.rbegin(); it != commands_.rend(); ++it) {
            (*it)->undo();
        }
    }
    
    std::string getDescription() const override {
        return description_;
    }
    
    bool isEmpty() const { return commands_.empty(); }
};
