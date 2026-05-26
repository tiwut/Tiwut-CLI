# Tiwut-CLI: Cyberpunk Terminal TUI Desktop Dashboard

Tiwut-CLI is an advanced, lightweight, single-window terminal desktop environment and interactive widget dashboard written in native C++ using Ncurses. Designed for speed, minimal resource footprints, and complete user autonomy, Tiwut-CLI turns standard console spaces into a multi-pane navigable cyberpunk workstation. 

The application implements a high-performance single-buffer rendering pipeline, an integrated developer terminal, a live system monitor, a dynamic application launcher, a remote GitHub repository manager with an interactive file browser, and a secure decentralized App Store featuring strict SHA-256 verification.

---

## Key Features

### 1. Multi-Pane Visual Grid and Desktop Architecture
* Single-Window Curses Pipeline: Inspired by the nano editor, Tiwut-CLI completely avoids multi-window buffering overhead. It uses derived viewports (derwin on stdscr) that write directly to the parent screen buffer, minimizing memory allocation and eliminating terminal flickers or cursor stutters.
* Persistent Widget Layout: Tiled panels automatically calculate grid positions upon terminal resizing.
* Navigable Drop-Down Menus: Instant accessibility via function keys F1 (File Actions), F2 (Visual Settings), F3 (Help Links), and F4 (Developer Utilities).
* Dock Taskbar: Active bottom status dock highlighting the currently selected pane in real time.
* Cyberpunk Styling: Curated, high-contrast xterm 256-color schemes featuring live theme toggles (using the C key or Settings menu).

### 2. Live System Stats Widget
* Real-Time Monitoring: Computes and displays CPU utilization, RAM usage, and active storage metrics.
* Container Safety: Features division-by-zero guards for virtual containers, namespace filesystem blocks, and clamps progress bar ranges securely to prevent length errors.

### 3. Integrated Developer Console Shell
* Real-Time Output Streams: Native terminal shell shell environment executing tools and utilities through popen.
* Interactive Navigation: Complete with UP/DOWN recall history, interactive BACKSPACE edits, and Page Up/Page Down terminal history scrolling.
* Synchronized Directories: Includes a built-in cd command handler that automatically synchronizes paths with the active File Explorer widget panel.

### 4. Interactive File Explorer and Notepad
* File Manager: Navigate local directory structures, preview document contents inside pop-up dialog overlays, and track paths.
* Scratchpad Notepad: Edit and maintain real-time drafts and scratch notes in a dedicated interactive pane.

### 5. GitHub Repository Manager and Remote Explorer
* Multi-Tab Navigation: Instantly filter and switch repository arrays for specified GitHub users (such as tiwut and nexus-titan) using the LEFT/RIGHT arrow keys or TAB key.
* Interactive Remote Directory Browser: Explore folders and documents remotely using the GitHub Contents API without cloning. Directories are highlighted in green, and files in standard text.
* Ascent Navigation: Traverse nested folder trees and ascend back to parent pathways using the BACKSPACE key or by selecting the parent directory indicator (../).
* Markdown README Viewer: Scrollable full-screen reader modal that parses and renders README documentation on the fly. Bold pink accents denote headers, cyan highlights lists, and green denotes blockquotes.
* Remote File Previews: Retrieves raw contents from remote CDN paths, allowing users to view files in a scrollable viewer before initiating standard Git clone procedures.

### 6. Decentralized App Store and Security Engine
* Remote Manifest Loader: Automatically retrieves live store packages from the raw catalog endpoint, with a robust fallback to local structures when offline.
* Platform Compatibility Check: Validates host OS restrictions (supporting Linux and macOS configurations) and system dependencies (such as bash, curl, and git) using live status check badges.
* Secure SHA-256 Verification: Calculates the checksum of downloaded installer assets using native shasum or sha256sum utilities. If the computed hash deviates from the manifest expectation, the installation is aborted immediately and files are purged.
* Version Tracking Registry: Maintains installed application states inside a local registry file (installed_apps.txt) to report package upgrade availabilities.
* Automated Launcher Integration: Successfully installed packages generate corresponding desktop shortcut mappings and append details to user-level launcher indexes automatically.

---

## Keyboard Controls and Hotkeys

Tiwut-CLI maps specific keys to ensure seamless, mouse-free workflow actions:

### Desktop Panels and Navigation
* TAB: Cycle keyboard focus sequentially across the 4 workspace panes (Stats, Notepad, App Links, File Explorer).
* M / m: Toggle focused panel between Standard Grid and Full-Screen Maximized viewports.
* C / c: Cycle through visual color theme profiles on the fly.
* ESC: Close active overlays, modal panels, or drop-down menus.
* Q / q: Safely exit the dashboard (when no modal overlays are active).

### Navigation & Actions
* Arrow Keys: Highlight files, directories, apps, or items inside active widgets.
* Page Up / Page Down: Scroll through extensive lists in the GitHub repositories menu, remote directory explorer, README viewer, and file modals.
* ENTER: Execute shortcut links, switch directories, open files, or trigger App Store installations.
* BACKSPACE: Ascend to parent directories in both local and remote file browsers.

### Drop-down Category Function Keys
* F1: File Menu (Preview Document, System Environment Specs, Reset Scratchpad, Shutdown).
* F2: Settings Menu (Toggle Full Screen, Cycle Color Theme, Reset UI Workspace Layout).
* F3: Help Menu (Open Official Website, Open GitHub Profile, Core Credits).
* F4: Developer Menu (Interactive Shell, Fetch GitHub Repos, Decentralized App Store).

---

## Directory Structure

* CMakeLists.txt: Main compiler build target directive.
* install.sh: Universal multi-OS dependency resolver and native builder.
* uninstall.sh: Cache cleanup and global command link removal script.
* desktop/: Config index and default desktop app launcher shortcuts.
* Remote/: Decentralized App Store manifest and installer script templates.
* src/: Main dashboard logic, visual menus, and screen drawing scripts.
* src/widgets/: Modular panels (system stats, notepad scratch, file explorer, git console, App Store).

---

## Installation and Compiling

Tiwut-CLI provides an intelligent, automated build script that handles platform discovery, checks compiler integrity, configures prefix linkages, and deploys global search pathways.

### Prerequisite Dependencies
Building from source requires standard C++17 build tools, CMake, and Ncurses wide-character headers (ncursesw).

* Debian/Ubuntu: `sudo apt install build-essential cmake libncurses-dev`
* Arch Linux: `sudo pacman -S base-devel cmake ncurses`
* Fedora: `sudo dnf groupinstall "Development Tools" && sudo dnf install cmake ncurses-devel`
* macOS: Requires Xcode Command Line Tools and Homebrew: `brew install cmake ncurses`

### 1. Build and Deploy
Execute the universal multi-OS installer from the root of your local clone. The script will automatically verify dependencies, configure Homebrew prefixes for Atomic platforms (like Bazzite) or macOS, and link binaries to user-level search folders:

```bash
chmod +x install.sh
./install.sh
```

### 2. Launching the Dashboard
Once the installation completes, reload your shell configuration (or execute `source ~/.bashrc` / `source ~/.zshrc`), and launch:

```bash
Tiwut-CLI
```

### 3. Cleanup and Removal
To remove build targets, program caches, user preferences, and global search linkages, run:

```bash
chmod +x uninstall.sh
./uninstall.sh
```

---

## Security Mandate: App Store Verifications

The Tiwut App Store is designed with a decentralized, zero-trust security paradigm:

1. Download Stage: Installation packages or scripts are retrieved securely into a local staging cache (~/.local/share/tiwut-cli/tmp/).
2. Hash Extraction: Native utilities compute the exact SHA-256 checksum of the local script.
3. Cryptographic Matching: The client compares the computed hash with the cryptographically secure signature defined in the raw repository manifest.
4. Abortion Protocol: If the hashes fail to match perfectly, the installation terminates immediately, a security warning is logged, and the file is permanently deleted from the hard drive before execution can take place.
