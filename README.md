# TAComparator

TAComparator is a Qt/C++ desktop application for comparing IPSA transient analysis output files (`.itf`). It allows users to load multiple transient result files, select object types/components/signals, and compare time-series results visually across files.

The current focus is to provide a practical transient-analysis comparison tool with a simple workflow:

1. Load one or more `.itf` files.
2. Select which files should be included in a plot.
3. Choose a study/object type, component, signal, and plot type.
4. View the selected signal across the loaded files.
5. Export plots as PNG or PDF reports.

---

## Features

### File loading

- Load multiple IPSA transient `.itf` files.
- Maintain a common file-selection panel for all plot windows.
- Enable/disable individual loaded files for plotting.
- Configure per-file line colour and line thickness.
- Clear all loaded files safely.
- Warn when loaded files appear to come from different networks.

### ITF parsing

The parser uses a two-pass design:

1. **Discovery pass**
   - Reads `Define Transient ...` blocks.
   - Discovers object groups and field/signal schemas.
   - Reads `Compound SystemMap` records.
   - Creates components such as busbars, branches, generators, induction machines, governors, etc.

2. **Population pass**
   - Reads each `Compound Step` block.
   - Stores time values from `RunStep`.
   - Stores output signal values against the appropriate discovered components.

Currently handled output records include:

- `RunStep`
- `BusbarOutput`
- `MonBranchOutput`
- `GeneratorOutput`
- `AVROutput`
- `GovernorOutput`
- `IndMachOutput`

Unsupported or non-plottable records are safely skipped.

### Plot windows

- Supports up to 4 plot windows.
- Each plot window has independent selections:
  - Study type
  - Component
  - Signal
  - Plot type
- Supported plot types:
  - Line plot
  - Bar plot
- Plot windows are dockable, movable, floatable, and closable.
- At least one plot window is always kept open.
- Plot layout can be reset from the menu.

### Plot interaction

- Hover over plotted data to compare values across loaded files.
- Mouse wheel zooms the X-axis.
- Drag after zooming to pan along the X-axis.
- Double-click resets zoom/pan.
- Plot selection panel can be hidden to give more room to the graph.

### Display modes

- Raw ITF names can be shown directly.
- IPSA-friendly display names can be used for clearer labels.

Examples:

- `BusbarOutput` -> `Busbars`
- `MonBranchOutput` -> `Branches`
- `GeneratorOutput` -> `Synchronous machines`
- `IndMachOutput` -> `Induction machines`

### Export

- Export the selected plot as a PNG file.
- Export all plots as PNG files.
- Export all plots as a PDF report.

---


## Requirements

- Qt 6.x
- Qt Widgets module
- CMake 3.19 or newer
- C++17-compatible compiler

Tested development environment:

- Qt Creator 20.0.0
- Qt 6.11.1
- MinGW 64-bit kit
- Windows 10/11

---

## Build instructions

### Using Qt Creator

1. Open `CMakeLists.txt` in Qt Creator.
2. Select a Qt 6 kit, for example MinGW 64-bit or MSVC 2022.
3. Configure the project.
4. Build the project.
5. Set the run working directory to the project source directory if you want to load files from `samples/` using relative paths.
6. Run the application.

---

## Usage

### Load ITF files

Use:

```text
File -> Add ITF File(s)
```

Select one or more `.itf` files.

Loaded files appear in the Files dock. Each loaded file can be enabled/disabled for plotting.

### Add plot windows

Use:

```text
Plot -> Add Plot Window
```

A maximum of 4 plot windows is supported.

### Configure a plot

In each plot window:

1. Choose the study type.
2. Choose the component.
3. Choose the signal.
4. Choose Line or Bar plot.

The plot updates automatically when the selection changes.

### Reset plot layout

Use:

```text
Plot -> Reset Plot Window Layout
```

This restores the docked plot arrangement.

### Export plots

Use:

```text
Export -> Export Selected Plot as PNG...
Export -> Export All Plots as PNG Files...
Export -> Export All Plots as PDF Report...
```

---


## Known limitations

- Only selected transient output records are currently populated for plotting.
- Not every discovered ITF record is guaranteed to be plottable.
- Some ITF files may contain placeholder rows such as component ID `0`; these rows are skipped if the component is not present in the SystemMap.
- The application currently supports a maximum of 4 plot windows.
- Plot types are currently limited to line and bar plots.
- Computed/derived signals are not yet implemented.

---


## Development status

Current working areas:

- Improve support for more ITF output record types.


---
