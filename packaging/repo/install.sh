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

case "$ID" in
    ubuntu|debian|raspbian|devuan)
        echo "Detected ${PRETTY_NAME}"

        # Devuan is a Debian fork; map its codenames/version to the Debian equivalents
        # so we can reuse the Debian repository suites.
        if [ "$ID" = "devuan" ]; then
            case "$VERSION_CODENAME" in
                excalibur) VERSION_CODENAME="trixie";  VERSION_ID="13" ;;
                daedalus)  VERSION_CODENAME="bookworm"; VERSION_ID="12" ;;
                chimaera)  VERSION_CODENAME="bullseye"; VERSION_ID="11" ;;
            esac
        fi

        # Check if codename is supported
        case "$VERSION_CODENAME" in
            jammy|noble|plucky|bookworm|trixie)
                ;;
            *)
                echo "Warning: ${VERSION_CODENAME} is not officially supported."
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
        case "$ID" in
            ubuntu)
                MAJOR=$(echo "${VERSION_ID:-0}" | cut -d. -f1)
                [ "${MAJOR:-0}" -ge 24 ] && USE_DEB822=1
                ;;
            debian|raspbian|devuan)
                MAJOR=$(echo "${VERSION_ID:-0}" | cut -d. -f1)
                [ "${MAJOR:-0}" -ge 13 ] && USE_DEB822=1
                ;;
        esac
        
        if [ "$USE_DEB822" = "1" ]; then
            # Modern DEB822 format
            cat > /etc/apt/sources.list.d/amiberry.sources <<EOF
Types: deb
URIs: ${REPO_BASE}/apt
Suites: ${VERSION_CODENAME}
Components: main
Architectures: ${ARCH}
Signed-By: ${GPG_KEYRING}
EOF
            echo "Created /etc/apt/sources.list.d/amiberry.sources"
        else
            # Legacy one-line format
            echo "deb [arch=${ARCH} signed-by=${GPG_KEYRING}] ${REPO_BASE}/apt ${VERSION_CODENAME} main" \
                > /etc/apt/sources.list.d/amiberry.list
            echo "Created /etc/apt/sources.list.d/amiberry.list"
        fi
        
        # Update and install
        echo "Updating package lists..."
        apt-get update
        echo ""
        echo "Repository configured! Install Amiberry with:"
        echo "  sudo apt install amiberry"
        ;;
    
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
        echo ""
        echo "For manual installation, visit: https://packages.amiberry.com"
        exit 1
        ;;
esac

echo ""
echo "GPG key fingerprint: D74D52525340C442A4F8B657A12B57C04E1FE282"
echo "Repository: ${REPO_BASE}"
