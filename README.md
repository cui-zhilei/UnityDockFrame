## Languages
- [English](README.md) (Default)
- [简体中文](README.zh.md)

# UnityDockFrame

## Project Overview

UnityDockFrame is a Qt-based dockable window system designed to implement Unity Editor-style interface layout functionality. The Unity Editor is renowned for its flexible and powerful window docking system, which allows users to freely adjust the interface layout according to their personal workflow, improving work efficiency.

## Unity Editor Docking Features

The Unity Editor's docking system has the following core characteristics:

1. **Flexible Window Layout**
   - Support dragging any window to different positions in the editor (top, bottom, left, right, or central area)
   - Automatically display docking previews, intuitively indicating where the window will be docked
   - Support nested docking between windows to create complex multi-window layouts

2. **Tabbed Window Management**
   - Multiple windows in the same area are automatically organized as tabs
   - Quick switching between different function windows via tabs
   - Support dragging windows from tabs to other positions

3. **Floating Window Support**
   - Windows can be detached from the main editor to become independent floating windows
   - Floating windows can be dragged back to the main editor area for docking
   - Support window layouts across multiple monitors

4. **Layout Save and Restore**
   - Support saving custom window layout configurations
   - Quickly switch between different work layouts (such as 2D editing layout, 3D editing layout, animation editing layout, etc.)
   - Automatically restore the last used layout after restarting the editor

## UnityDockFrame Implementation

The UnityDockFrame project draws on the design concepts of the Unity Editor's docking system and implements a fully functional dockable window system based on the Qt framework. Its main features include:

- Support window dragging, docking, floating, and maximizing operations
- Provide tabbed window layout management
- Implement flexible split layouts based on custom Splitter
- Support layout saving and loading functions
- Provide a window factory mechanism for easy extension of custom window types
- Support fixed layouts to prevent accidental drag modification

This project is suitable for Qt application development that requires flexible interface layouts, such as IDEs, image editors, data analysis tools, etc.

## Core Features

- **Flexible Window Management**
  - Support dragging windows to dock at different positions of the container (top, bottom, left, right)
  - Support window floating and maximizing operations
  - Support tabbed window layout, allowing switching between multiple windows in the same container
  - Provide window context menu, supporting custom operations

- **Powerful Layout System**
  - Flexible split layout based on custom Splitter
  - Support saving layouts as JSON format files
  - Support loading layouts from JSON files
  - Support fixed layout to prevent drag modification

## UnityDockFrame-Demo Example Introduction

UnityDockFrame-Demo is an example application using the UnityDockFrame library, demonstrating how to create and use a dockable window system.

### Function Description

- **Menu System**
  - Provide "Menu 1" and "Window" menus
  - "Window" menu lists all registered window types
  - "Layout" menu provides layout management functions (Layout 1, Layout 2, Save Layout, Open Layout, Default Layout, Fixed Layout)

- **Example Windows**
  - Implements multiple color windows (BlackWindow, BlueWindow, GreenWindow, RedWindow, CyanWindow)
  - Each window is a subclass of `DockableWindow`
  - Demonstrates how to register and use dockable windows

- **Layout Management**
  - Automatically load the last saved layout at startup
  - Automatically save the current layout when exiting
  - Support switching between multiple predefined layouts
  - Support fixed layout to prevent drag modification

### How to Run

1. Open `UnityDockFrame-Demo.pro` file with Qt Creator
2. Compile and run the project
3. Select different window types in the "Window" menu to create windows
4. Drag windows for docking, floating, etc.
5. Use the "Layout" menu to manage window layouts

## Dock Library API Usage

### 1. Creating Dockable Windows

To create a dockable window, you need to inherit from the `DockableWindow` class:

```cpp
#include "DockableWindow.h"

class MyWindow : public dock::DockableWindow
{
    Q_OBJECT
public:
    MyWindow(QWidget *parent = nullptr);
    ~MyWindow();
    
    // Optional: override these methods to implement custom functionality
    virtual void onFloating() override;        // Called when window is floating
    virtual void onDocking() override;        // Called when window is docking
    virtual void onContextMenu(QMenu* menu) override;  // Context menu
    virtual bool canClose() override;         // Whether the window can be closed
    virtual bool load(const QJsonObject &jsonObj) override;  // Load window state
    virtual void saveObject(QJsonObject &jsonObj) override;   // Save window state
    virtual QString getTitle() override;      // Get window title
};
```

### 2. Registering Windows

There are two ways to register windows: static registration and dynamic registration.

#### Static Registration

Use the `STATIC_REGISTER_WINDOW` macro to register windows in the header file:

```cpp
// At the end of MyWindow.h file
STATIC_REGISTER_WINDOW(MyWindow, "My Window", true)  // Parameters: class name, window title, whether it's a unique instance
```

#### Dynamic Registration

First use the `DEC_WINDOW_FACTORY` macro to declare the window factory, then call the `REGISTER_WINDOW` macro in the code to register:

```cpp
// In MyWindow.h file
DEC_WINDOW_FACTORY(MyWindow, "My Window", true)

// In main.cpp or other appropriate location
REGISTER_WINDOW(MyWindow)
```

### 3. Using DockContainer

`DockContainer` is the core container of the docking system, used to manage all dockable windows:

```cpp
#include "DockContainer.h"

// Create main window
QMainWindow *mainWindow = new QMainWindow();
mainWindow->setCentralWidget(new QWidget());

// Create DockContainer
DockContainer *container = new DockContainer(mainWindow->centralWidget());

// Create and display window
uint windowTypeId = qHash(QString("MyWindow"));
container->floatView(windowTypeId);  // Create floating window
// or
container->activeView(windowTypeId);  // Create or activate existing window
```

### 4. Layout Management

```cpp
// Save layout to JSON file
QFile file("layout.json");
if (file.open(QIODevice::WriteOnly)) {
    QJsonObject jsonObj;
    container->saveLayoutToJson(jsonObj);
    QJsonDocument doc(jsonObj);
    file.write(doc.toJson());
    file.close();
}

// Load layout from JSON file
QFile file("layout.json");
if (file.open(QIODevice::ReadOnly)) {
    QByteArray data = file.readAll();
    QJsonDocument doc(QJsonDocument::fromJson(data));
    QJsonObject jsonObj = doc.object();
    container->createLayoutFromJson(jsonObj);
    file.close();
}

// Initialize default layout
container->initLayout();

// Enable/disable drag functionality
container->enableDrag(true);  // Enable drag
container->enableDrag(false); // Disable drag (fixed layout)
```

### 5. Window Factory Management

```cpp
// Get window factory manager instance
WindowFactoryManager *manager = WindowFactoryManager::getInstance();

// Get all registered window factories
const std::map<uint, WindowFactory *> &factories = manager->getAllFactorys();

// Get window factory by window type ID
WindowFactory *factory = manager->getFactory(windowTypeId);

// Create window instance
DockableWindow *window = factory->create(parentWidget);
```

## Compilation and Running

### Environment Requirements

- Qt 5.0 or higher
- C++11 or higher compiler

### Compilation Steps

1. Open `DockFrame.pro` file with Qt Creator
2. Select build configuration (Debug or Release)
3. Compile the entire project

### Running the Example

After compilation is complete, run the UnityDockFrame-Demo project directly to see the example application.

## License

This project is licensed under the [MIT License](LICENSE), which allows free use, modification, and distribution.

## Author

Cui Zhilei

## Version

Current version: 1.0.0

Release date: April 2017- [English](README.md) (default)
- [简体中文](README.zh.md)# UnityDockFrame

## Project Overview

UnityDockFrame is a Qt-based dockable window system designed to implement Unity Editor-style interface layout functionality. The Unity Editor is renowned for its flexible and powerful window docking system, which allows users to freely adjust the interface layout according to their personal workflow, improving work efficiency.

## Unity Editor Docking Features

The Unity Editor's docking system has the following core characteristics:

1. **Flexible Window Layout**
   - Support dragging any window to different positions in the editor (top, bottom, left, right, or central area)
   - Automatically display docking previews, intuitively indicating where the window will be docked
   - Support nested docking between windows to create complex multi-window layouts

2. **Tabbed Window Management**
   - Multiple windows in the same area are automatically organized as tabs
   - Quick switching between different function windows via tabs
   - Support dragging windows from tabs to other positions

3. **Floating Window Support**
   - Windows can be detached from the main editor to become independent floating windows
   - Floating windows can be dragged back to the main editor area for docking
   - Support window layouts across multiple monitors

4. **Layout Save and Restore**
   - Support saving custom window layout configurations
   - Quickly switch between different work layouts (such as 2D editing layout, 3D editing layout, animation editing layout, etc.)
   - Automatically restore the last used layout after restarting the editor

## UnityDockFrame Implementation

The UnityDockFrame project draws on the design concepts of the Unity Editor's docking system and implements a fully functional dockable window system based on the Qt framework. Its main features include:

- Support window dragging, docking, floating, and maximizing operations
- Provide tabbed window layout management
- Implement flexible split layouts based on custom Splitter
- Support layout saving and loading functions
- Provide a window factory mechanism for easy extension of custom window types
- Support fixed layouts to prevent accidental drag modification

This project is suitable for Qt application development that requires flexible interface layouts, such as IDEs, image editors, data analysis tools, etc.

## Core Features

- **Flexible Window Management**
  - Support dragging windows to dock at different positions of the container (top, bottom, left, right)
  - Support window floating and maximizing operations
  - Support tabbed window layout, allowing switching between multiple windows in the same container
  - Provide window context menu, supporting custom operations

- **Powerful Layout System**
  - Flexible split layout based on custom Splitter
  - Support saving layouts as JSON format files
  - Support loading layouts from JSON files
  - Support fixed layout to prevent drag modification

- **Convenient Window Factory Mechanism**
  - Provide factory pattern for window registration and creation
  - Support both static and dynamic window registration
  - Support unique instance windows (singleton windows), ensuring only one instance exists for the same window type

- **Good Extensibility**
  - Provide base class `DockableWindow` for users to customize windows
  - Support window type ID identification for easy management and search
  - Support window state saving and restoration

## Directory Structure
