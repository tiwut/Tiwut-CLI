#!/usr/bin/env bash
set -e


RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'


clear || true

echo -e "${RED}${BOLD}"
echo "  _   _                 _   _ _____ _____ "
echo " | \ | |               | | | /  ___/  ___|"
echo " |  \| | ___  _   _ ___| | | \ \`--.\ \`--. "
echo " | . \` |/ _ \| | | / __| | | |\`--. \\\`--. \\"
echo " | |\  |  __/| |_| \__ \ |_| /\__/ /\__/ /"
echo " \_| \_/\___| \__,_|___/\___/\____/\____/ "
echo -e "                                          ${RESET}"
echo -e "${CYAN}${BOLD}       -- TIWUT CLI DESKTOP UNINSTALLER --${RESET}\n"

echo -e "${BLUE}[1/2]${RESET} Wiping CMake build caches and local directory trees..."


if [ -d build ]; then
    echo -e "  ${CYAN}* Removing CMake build cache folder: build/${RESET}"
    rm -rf build
fi


LOCAL_BIN="$HOME/.local/bin/Tiwut-CLI"
LOCAL_SHARE="$HOME/.local/share/tiwut-cli"


GLOBAL_BIN="/usr/local/bin/Tiwut-CLI"
GLOBAL_SHARE="/usr/local/share/tiwut-cli"

REMOVED_ANY=false


if [ -f "$LOCAL_BIN" ]; then
    echo -e "  ${CYAN}* Removing local binary: ${LOCAL_BIN}${RESET}"
    rm -f "$LOCAL_BIN"
    REMOVED_ANY=true
fi


if [ -d "$LOCAL_SHARE" ]; then
    echo -e "  ${CYAN}* Deleting local asset folder: ${LOCAL_SHARE}${RESET}"
    rm -rf "$LOCAL_SHARE"
    REMOVED_ANY=true
fi


if [ -f "$GLOBAL_BIN" ]; then
    if [ "$EUID" -eq 0 ]; then
        echo -e "  ${CYAN}* Removing global binary: ${GLOBAL_BIN}${RESET}"
        rm -f "$GLOBAL_BIN"
        REMOVED_ANY=true
    else
        echo -e "  ${YELLOW}! Warning: Global binary found at '${GLOBAL_BIN}' but you are not running as root.${RESET}"
        echo -e "    Please run with sudo to delete global binaries: sudo ./uninstall.sh"
    fi
fi


if [ -d "$GLOBAL_SHARE" ]; then
    if [ "$EUID" -eq 0 ]; then
        echo -e "  ${CYAN}* Deleting global asset folder: ${GLOBAL_SHARE}${RESET}"
        rm -rf "$GLOBAL_SHARE"
        REMOVED_ANY=true
    else
        echo -e "  ${YELLOW}! Warning: Global assets found at '${GLOBAL_SHARE}' but you are not running as root.${RESET}"
        echo -e "    Please run with sudo to delete global assets: sudo ./uninstall.sh"
    fi
fi

echo -e "\n${BLUE}[2/2]${RESET} Finalizing file system cleanup..."

if [ "$REMOVED_ANY" = true ]; then
    echo -e "\n${GREEN}${BOLD}====================================================${RESET}"
    echo -e "${GREEN}${BOLD}✔ Tiwut-CLI successfully uninstalled!${RESET}"
    echo -e "${GREEN}${BOLD}====================================================${RESET}\n"
    echo -e "All compiled executable binaries, logo assets, cache configurations, and notes databases have been removed."
else
    echo -e "  ${YELLOW}! No installed components of Tiwut-CLI were detected in standard directories.${RESET}"
fi
echo -e "Thanks for using Tiwut CLI!\n"
