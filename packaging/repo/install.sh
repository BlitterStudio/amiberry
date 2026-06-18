#!/bin/sh
# Amiberry Package Repository Setup Script
# Usage: curl -fsSL https://packages.amiberry.com/install.sh | sudo sh

set -e

# Must run as root
if [ "$(id -u)" != "0" ]; then
    echo "Error: This script must be run as root (use sudo)"
    exit 1
fi

# Detect OS
if [ ! -f /etc/os-release ]; then
    echo "Error: Cannot detect Linux distribution (/etc/os-release not found)"
    exit 1
fi

. /etc/os-release

REPO_BASE="https://packages.amiberry.com"
GPG_KEY_URL="${REPO_BASE}/gpg.key"
GPG_ASC_URL="${REPO_BASE}/gpg.asc"
GPG_KEYRING="/usr/share/keyrings/amiberry-archive-keyring.gpg"

APT_FAMILY=""
APT_SUITE=""
APT_VERSION_ID="${VERSION_ID:-}"

set_ubuntu_suite() {
    APT_FAMILY="ubuntu"
    APT_SUITE="$1"
    case "$APT_SUITE" in
        jammy)    APT_VERSION_ID="22.04" ;;
        noble)    APT_VERSION_ID="24.04" ;;
        questing) APT_VERSION_ID="25.10" ;;
        resolute) APT_VERSION_ID="26.04" ;;
    esac
}

set_debian_suite() {
    APT_FAMILY="debian"
    APT_SUITE="$1"
    case "$APT_SUITE" in
        bookworm) APT_VERSION_ID="12" ;;
        trixie)   APT_VERSION_ID="13" ;;
    esac
}

case " ${NAME:-} ${PRETTY_NAME:-} " in
    *"Parrot"*)
        case "${VERSION_ID:-}:${VERSION_CODENAME:-}" in
            6*:*)  set_debian_suite "bookworm" ;;
            7*:*)  set_debian_suite "trixie" ;;
            *:lory) set_debian_suite "bookworm" ;;
        esac
        ;;
esac

if [ -z "$APT_SUITE" ]; then
    case "$ID" in
        ubuntu)
            set_ubuntu_suite "${VERSION_CODENAME:-}"
            APT_VERSION_ID="${VERSION_ID:-$APT_VERSION_ID}"
            ;;
        debian|raspbian)
            set_debian_suite "${VERSION_CODENAME:-}"
            APT_VERSION_ID="${VERSION_ID:-$APT_VERSION_ID}"
            ;;
        devuan)
            # Devuan is a Debian fork; map its codenames/version to the Debian
            # equivalents so we can reuse the Debian repository suites.
            case "${VERSION_CODENAME:-}" in
                excalibur) set_debian_suite "trixie" ;;
                daedalus)  set_debian_suite "bookworm" ;;
                chimaera)  set_debian_suite "bullseye" ;;
                *)         set_debian_suite "${VERSION_CODENAME:-}" ;;
            esac
            ;;
        linuxmint)
            if [ -n "${UBUNTU_CODENAME:-}" ]; then
                set_ubuntu_suite "$UBUNTU_CODENAME"
            else
                case "${VERSION_ID:-}" in
                    6*) set_debian_suite "bookworm" ;;
                    7*) set_debian_suite "trixie" ;;
                esac
            fi
            ;;
        elementary|pop|zorin)
            if [ -n "${UBUNTU_CODENAME:-}" ]; then
                set_ubuntu_suite "$UBUNTU_CODENAME"
            else
                case "$ID:${VERSION_ID:-}" in
                    elementary:7*|pop:22.04*|zorin:17*) set_ubuntu_suite "jammy" ;;
                    elementary:8*|pop:24.04*|zorin:18*) set_ubuntu_suite "noble" ;;
                esac
            fi
            ;;
        parrot)
            case "${VERSION_ID:-}" in
                6*) set_debian_suite "bookworm" ;;
                7*) set_debian_suite "trixie" ;;
            esac
            ;;
        sparky)
            case "${VERSION_ID:-}" in
                7*) set_debian_suite "bookworm" ;;
                8*) set_debian_suite "trixie" ;;
            esac
            ;;
    esac
fi

if [ -z "$APT_SUITE" ] && [ -n "${UBUNTU_CODENAME:-}" ]; then
    set_ubuntu_suite "$UBUNTU_CODENAME"
fi

if [ -z "$APT_SUITE" ]; then
    case "${VERSION_CODENAME:-}" in
        jammy|noble|questing|resolute)
            case " ${ID_LIKE:-} " in
                *" ubuntu "*|*" debian "*) set_ubuntu_suite "$VERSION_CODENAME" ;;
            esac
            ;;
        bookworm|trixie)
            case " ${ID_LIKE:-} " in
                *" debian "*|*" ubuntu "*) set_debian_suite "$VERSION_CODENAME" ;;
            esac
            ;;
    esac
fi

if [ -n "$APT_SUITE" ]; then
    echo "Detected ${PRETTY_NAME}"
    echo "Using ${APT_SUITE} repository suite"

    # Check if codename is supported
    case "$APT_SUITE" in
        jammy|noble|questing|resolute|bookworm|trixie)
            ;;
        *)
            echo "Warning: ${APT_SUITE} is not officially supported."
            echo "You can still try to use the closest supported release."
            ;;
    esac
        
    # Install curl if not available
    if ! command -v curl >/dev/null 2>&1; then
        apt-get update -q && apt-get install -y -q curl
    fi
        
    # Download and install GPG key
    echo "Installing GPG signing key..."
    curl -fsSL "$GPG_KEY_URL" -o "$GPG_KEYRING"
    chmod 644 "$GPG_KEYRING"
        
    # Get architecture
    ARCH=$(dpkg --print-architecture)
        
    # Determine which sources format to use
    # DEB822 (.sources) for Ubuntu 24.04+ and Debian 13+ (trixie), legacy (.list) for older
    USE_DEB822=0
    case "$APT_FAMILY" in
        ubuntu)
            MAJOR=$(echo "${APT_VERSION_ID:-0}" | cut -d. -f1)
            [ "${MAJOR:-0}" -ge 24 ] && USE_DEB822=1
            ;;
        debian)
            MAJOR=$(echo "${APT_VERSION_ID:-0}" | cut -d. -f1)
            [ "${MAJOR:-0}" -ge 13 ] && USE_DEB822=1
            ;;
    esac
        
    if [ "$USE_DEB822" = "1" ]; then
        # Modern DEB822 format
        cat > /etc/apt/sources.list.d/amiberry.sources <<EOF
Types: deb
URIs: ${REPO_BASE}/apt
Suites: ${APT_SUITE}
Components: main
Architectures: ${ARCH}
Signed-By: ${GPG_KEYRING}
EOF
        echo "Created /etc/apt/sources.list.d/amiberry.sources"
    else
        # Legacy one-line format
        echo "deb [arch=${ARCH} signed-by=${GPG_KEYRING}] ${REPO_BASE}/apt ${APT_SUITE} main" \
            > /etc/apt/sources.list.d/amiberry.list
        echo "Created /etc/apt/sources.list.d/amiberry.list"
    fi
        
    # Update and install
    echo "Updating package lists..."
    apt-get update
    echo ""
    echo "Repository configured! Install Amiberry with:"
    echo "  sudo apt install amiberry"
else
    case "$ID" in
    fedora|rhel|centos|rocky|almalinux)
        echo "Detected ${PRETTY_NAME}"
        
        # Install curl if not available
        if ! command -v curl >/dev/null 2>&1; then
            dnf install -y curl
        fi
        
        # Download .repo file
        echo "Installing Amiberry repository..."
        curl -fsSL "${REPO_BASE}/rpm/amiberry.repo" -o /etc/yum.repos.d/amiberry.repo
        
        # Import GPG key
        echo "Importing GPG signing key..."
        rpm --import "$GPG_ASC_URL"
        
        echo ""
        echo "Repository configured! Install Amiberry with:"
        echo "  sudo dnf install amiberry"
        ;;
    
    *)
        echo "Error: Unsupported distribution: $ID"
        echo "Supported: Ubuntu (22.04+), Debian (12+), Devuan (5+), Fedora"
        echo "Also supported through compatible suites: Linux Mint, LMDE,"
        echo "Elementary OS, Pop!_OS, Zorin OS, Parrot OS, and SparkyLinux"
        echo ""
        echo "For manual installation, visit: https://packages.amiberry.com"
        exit 1
        ;;
    esac
fi

echo ""
echo "GPG key fingerprint: D74D52525340C442A4F8B657A12B57C04E1FE282"
echo "Repository: ${REPO_BASE}"
