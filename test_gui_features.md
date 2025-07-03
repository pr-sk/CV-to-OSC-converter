# Test Guide for New GUI Features

## 🎯 Features to Test

### 1. Show All Windows Feature
- **Menu Access**: View → Show All Windows (Ctrl+Shift+A)
- **Expected Result**: All main windows should open and be arranged on screen
- **Windows to Check**:
  - Main Window (CV to OSC Converter)
  - Channel Configuration
  - OSC Configuration
  - Audio Configuration
  - Performance Monitor

### 2. Welcome Dialog
- **First Launch**: Should appear automatically on first run
- **Manual Access**: View → Show Welcome Dialog
- **Features to Test**:
  - Dialog centers on screen
  - Contains helpful information about available windows
  - Shows keyboard shortcuts tips
  - "Don't show again" checkbox works
  - "Get Started!" button closes dialog

### 3. Hot Keys Integration
- **Ctrl+Shift+A**: Show all windows
- **Ctrl+1-5**: Toggle individual windows
- **Ctrl+Shift+H**: Open Hot Key Editor to see all shortcuts
- **Space**: Start/Stop conversion
- **F1-F8**: Toggle individual channels

### 4. Menu Bar Enhancements
- **View Menu**: Check that all menu items show correct shortcuts
- **Window Arrangement**: All windows should have proper positioning

## 🔍 Testing Checklist

### Basic Functionality
- [ ] Application starts without errors
- [ ] Welcome dialog appears on first launch
- [ ] All main windows can be opened
- [ ] Window arrangement works properly
- [ ] Hot keys respond correctly

### Menu System
- [ ] View menu shows "Show All Windows" option
- [ ] Keyboard shortcuts are displayed in menus
- [ ] "Show Welcome Dialog" option works
- [ ] All window toggle options work

### Window Management
- [ ] Windows are positioned correctly when arranged
- [ ] Windows can be moved and resized manually
- [ ] Window visibility states are maintained
- [ ] Performance is smooth with all windows open

### Hot Key System
- [ ] Ctrl+Shift+A opens all windows
- [ ] Individual window toggles work (Ctrl+1-5)
- [ ] Hot Key Editor opens with Ctrl+Shift+H
- [ ] All shortcuts are properly documented in editor

## 🚀 Quick Test Sequence

1. **Start Application**
   ```bash
   ./build/cv_to_osc_converter_gui
   ```

2. **Check Welcome Dialog**
   - Should appear automatically
   - Test "Don't show again" option
   - Close with "Get Started!"

3. **Test Show All Windows**
   - Press Ctrl+Shift+A
   - Verify all windows open and arrange properly

4. **Test Individual Toggles**
   - Press Ctrl+1 (Main Window)
   - Press Ctrl+2 (Channel Configuration)
   - Press Ctrl+3 (OSC Configuration)
   - Press Ctrl+4 (Audio Configuration)
   - Press Ctrl+5 (Performance Monitor)

5. **Test Hot Key Editor**
   - Press Ctrl+Shift+H
   - Verify all shortcuts are listed
   - Check categories are organized properly

6. **Test Menu Access**
   - Use View menu to access features
   - Verify shortcuts match actual behavior

## 🎨 Visual Verification

### Window Layout
- Main window should be largest, positioned top-left
- Configuration windows arranged in organized grid
- No overlapping windows on first arrangement
- Proper spacing between windows

### Welcome Dialog
- Centered on screen
- Professional appearance with emoji icons
- Clear, helpful text
- Proper button functionality

### Menu Integration
- Shortcuts displayed consistently
- Menu items work as expected
- Status indicators show correct information

## 📋 Success Criteria

✅ **All features work as designed**
✅ **No crashes or errors**
✅ **Smooth user experience**
✅ **Professional appearance**
✅ **Helpful for new users**
