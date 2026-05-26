#!/usr/bin/env bash
set -e


RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'


clear || true

echo -e "${MAGENTA}${BOLD}"
echo "____________________________________________________________"
echo "|                         Tiwut-CLI             {#} {-} {x} |"
echo "|-----------------------------------------------------------|"
echo "|         #######  #  #         #  #     #  #######         |"
echo "|            #     #   #   #   #   #     #     #            |"
echo "|            #     #    # # # #    #     #     #            |"
echo "|            #     #     #   #      #####      #            |"
echo "|___________________________________________________________|"
echo -e "                                          ${RESET}"
echo -e "${CYAN}${BOLD}_____________________________________________________________${RESET}\n"
echo -e "${CYAN}${BOLD}|                 <-- MULTI-OS INSTALLER -->                |${RESET}\n"
echo -e "${CYAN}${BOLD}|___________________________________________________________|${RESET}\n"


echo -e "${BLUE}[1/5]${RESET} Detecting operating system environment..."

OS_TYPE="Unknown"
PKG_MGR="Unknown"
IS_MAC=false

if [ "$(uname -s)" = "Darwin" ]; then
    OS_TYPE="macOS"
    IS_MAC=true
    PKG_MGR="brew"
    echo -e "  ${GREEN}✔ Detected Apple macOS.${RESET}"
elif [ -f /etc/os-release ]; then
    . /etc/os-release
    OS_TYPE=$NAME
    if [ -f /usr/bin/apt-get ] || [ -f /usr/bin/apt ]; then
        PKG_MGR="apt"
    elif [ -f /usr/bin/pacman ]; then
        PKG_MGR="pacman"
    elif [ -f /usr/bin/dnf ] || [ -f /usr/bin/yum ]; then
        PKG_MGR="dnf"
    elif [ -f /sbin/apk ]; then
        PKG_MGR="apk"
    fi
    echo -e "  ${GREEN}✔ Detected Linux Distribution: ${OS_TYPE} (${PKG_MGR})${RESET}"
else
    echo -e "  ${YELLOW}! Unknown OS. Assuming standard POSIX environment.${RESET}"
fi


INSTALL_GLOBAL=false
if [ "$EUID" -eq 0 ]; then
    echo -e "  ${YELLOW}! Running as root/sudo. Global installation targeting /usr/local/bin.${RESET}"
    INSTALL_GLOBAL=true
    BIN_DIR="/usr/local/bin"
    SHARE_DIR="/usr/local/share/tiwut-cli"
else
    echo -e "  ${GREEN}✔ Running as local user. Local installation targeting user directories.${RESET}"
    BIN_DIR="$HOME/.local/bin"
    SHARE_DIR="$HOME/.local/share/tiwut-cli"
fi


echo -e "\n${BLUE}[2/5]${RESET} Validating and installing dependencies..."


USE_BREW=false
BREW_PREFIX=""
if command -v brew &> /dev/null; then
    USE_BREW=true
    BREW_PREFIX=$(brew --prefix)
    echo -e "  ${GREEN}✔ Detected Homebrew environment. Prefix: ${BREW_PREFIX}${RESET}"
fi

install_dependencies() {
    echo -e "  ${CYAN}* Automatically installing build dependencies...${RESET}"
    case $PKG_MGR in
        apt)
            apt-get update
            apt-get install -y build-essential cmake libncurses-dev
            ;;
        pacman)
            pacman -Sy --noconfirm base-devel cmake ncurses
            ;;
        dnf)
            dnf groupinstall -y "Development Tools"
            dnf install -y cmake ncurses-devel
            ;;
        apk)
            apk update
            apk add --no-cache build-base cmake ncurses-dev bash
            ;;
        brew)
            brew install cmake ncurses
            ;;
        *)
            echo -e "  ${RED}✘ Unsupported package manager. Please install 'cmake', 'g++', and 'ncurses dev' manually.${RESET}"
            exit 1
            ;;
    esac
}


MISSING_DEPS=false
if ! command -v cmake &> /dev/null; then
    MISSING_DEPS=true
    echo -e "  ${YELLOW}! 'cmake' builder tool is missing.${RESET}"
fi

if ! command -v g++ &> /dev/null && [ "$IS_MAC" = false ]; then
    MISSING_DEPS=true
    echo -e "  ${YELLOW}! 'g++' compiler is missing.${RESET}"
fi


HAS_CURSES_H=false
if [ "$IS_MAC" = true ]; then
    HAS_CURSES_H=true
elif [ "$USE_BREW" = true ] && [ -f "${BREW_PREFIX}/opt/ncurses/include/curses.h" ]; then
    HAS_CURSES_H=true
elif command -v g++ &> /dev/null; then
    if echo "#include <curses.h>" | g++ -x c++ -E - &>/dev/null; then
        HAS_CURSES_H=true
    fi
fi

if [ "$HAS_CURSES_H" = false ]; then
    MISSING_DEPS=true
    echo -e "  ${YELLOW}! Curses header files (curses.h) are missing.${RESET}"
fi

if [ "$MISSING_DEPS" = true ]; then
    if [ "$USE_BREW" = true ]; then
        echo -e "  ${CYAN}* Installing missing dependencies using Homebrew...${RESET}"
        brew install cmake ncurses

        BREW_PREFIX=$(brew --prefix)
    elif [ "$EUID" -eq 0 ] || [ "$PKG_MGR" = "brew" ]; then
        install_dependencies
    else
        echo -e "  ${YELLOW}! Missing compiler, CMake, or Ncurses header files.${RESET}"
        echo -e "  ${YELLOW}! Run this installer as root/sudo to install dependencies automatically, or run:${RESET}"
        case $PKG_MGR in
            apt) echo -e "    ${CYAN}sudo apt install build-essential cmake libncurses-dev${RESET}" ;;
            pacman) echo -e "    ${CYAN}sudo pacman -S base-devel cmake ncurses${RESET}" ;;
            dnf) echo -e "    ${CYAN}sudo dnf groupinstall \"Development Tools\" && sudo dnf install cmake ncurses-devel${RESET}" ;;
            apk) echo -e "    ${CYAN}sudo apk add build-base cmake ncurses-dev${RESET}" ;;
            *) echo -e "    ${CYAN}Install Homebrew (https://brew.sh) and the installer will resolve everything automatically!${RESET}" ;;
        esac
        exit 1
    fi
else
    echo -e "  ${GREEN}✔ Build utilities, compilers, and Curses headers are resolved.${RESET}"
fi


echo -e "\n${BLUE}[3/5]${RESET} Generating CMake configuration and compiling binary..."


rm -rf build
mkdir -p build
cd build

echo -e "  ${CYAN}* Running CMake...${RESET}"
if [ ! -z "${BREW_PREFIX}" ]; then
    echo -e "  ${CYAN}* Configuring CMake with Homebrew prefix: ${BREW_PREFIX}${RESET}"
    cmake -DCMAKE_PREFIX_PATH="${BREW_PREFIX}" ..
else
    cmake ..
fi


CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)
echo -e "  ${CYAN}* Compiling with ${CORES} CPU core threads...${RESET}"
make -j"${CORES}"

echo -e "  ${GREEN}✔ Program successfully compiled natively!${RESET}"
cd ..


echo -e "\n${BLUE}[4/5]${RESET} Deploying built binaries and assets..."

echo -e "  ${CYAN}* Creating folders: ${SHARE_DIR}${RESET}"
mkdir -p "${SHARE_DIR}"

if [ -f logo.txt ]; then
    echo -e "  ${CYAN}* Deploying backup logo config: logo.txt -> ${SHARE_DIR}/logo.txt${RESET}"
    cp logo.txt "${SHARE_DIR}/logo.txt"
fi

if [ -d desktop ]; then
    echo -e "  ${CYAN}* Creating desktop icons folder: ${SHARE_DIR}/desktop${RESET}"
    mkdir -p "${SHARE_DIR}/desktop"
    echo -e "  ${CYAN}* Deploying default app shortcuts to ${SHARE_DIR}/desktop/...${RESET}"
    cp desktop/*.json "${SHARE_DIR}/desktop/"
fi


echo -e "  ${CYAN}* Creating target binary directory: ${BIN_DIR}${RESET}"
mkdir -p "${BIN_DIR}"

echo -e "  ${CYAN}* Deploying binary: build/Tiwut-CLI -> ${BIN_DIR}/Tiwut-CLI${RESET}"
rm -f "${BIN_DIR}/Tiwut-CLI"
cp build/Tiwut-CLI "${BIN_DIR}/Tiwut-CLI"
chmod +x "${BIN_DIR}/Tiwut-CLI"



echo -e "\n${BLUE}[5/5]${RESET} Verifying shell binary command execution pathways..."

PATH_MODIFIED=false
if [ "$INSTALL_GLOBAL" = false ]; then
    if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
        echo -e "  ${YELLOW}! '${BIN_DIR}' is not in your current PATH environment.${RESET}"
        

        SHELL_FILES=("$HOME/.bashrc" "$HOME/.zshrc")
        if [ "$IS_MAC" = true ]; then
            SHELL_FILES+=("$HOME/.bash_profile" "$HOME/.zprofile" "$HOME/.profile")
        fi
        
        for shell_rc in "${SHELL_FILES[@]}"; do
            if [ -f "$shell_rc" ]; then
                echo -e "  ${CYAN}* Appending PATH setup to ${shell_rc}${RESET}"
                echo -e "\n# Tiwut-CLI global command path" >> "$shell_rc"
                echo "export PATH=\"${BIN_DIR}:\$PATH\"" >> "$shell_rc"
                PATH_MODIFIED=true
            fi
        done
    fi
fi

echo -e "\n${GREEN}${BOLD}====================================================${RESET}"
echo -e "${GREEN}${BOLD}✔ Tiwut-CLI successfully built and installed!${RESET}"
echo -e "${GREEN}${BOLD}====================================================${RESET}\n"

if [ "$PATH_MODIFIED" = true ]; then
    echo -e "${YELLOW}${BOLD}ℹ PATH Environment Updated:${RESET} Please reload your shell or run:"
    echo -e "  ${CYAN}${BOLD}source ~/.bashrc${RESET} (or ${CYAN}${BOLD}source ~/.zshrc${RESET} / ${CYAN}${BOLD}source ~/.zprofile${RESET})\n"
fi

echo -e "Launch your upgraded terminal TUI desktop dashboard by typing:"
echo -e "  ${CYAN}${BOLD}Tiwut-CLI${RESET}\n"
