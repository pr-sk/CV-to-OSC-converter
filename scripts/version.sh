#!/bin/bash

# Version Management Script for CV to OSC Converter
# Usage:
#   ./scripts/version.sh bump patch    # Increment patch version (1.0.0 -> 1.0.1)
#   ./scripts/version.sh bump minor    # Increment minor version (1.0.0 -> 1.1.0)
#   ./scripts/version.sh bump major    # Increment major version (1.0.0 -> 2.0.0)
#   ./scripts/version.sh set 1.2.3     # Set specific version
#   ./scripts/version.sh get           # Get current version
#   ./scripts/version.sh tag           # Create git tag for current version

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Paths
VERSION_FILE="Version.h"
CHANGELOG_FILE="CHANGELOG.md"

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Function to get current version from Version.h
get_current_version() {
    local major=$(grep "#define CV_TO_OSC_VERSION_MAJOR" $VERSION_FILE | awk '{print $3}')
    local minor=$(grep "#define CV_TO_OSC_VERSION_MINOR" $VERSION_FILE | awk '{print $3}')
    local patch=$(grep "#define CV_TO_OSC_VERSION_PATCH" $VERSION_FILE | awk '{print $3}')
    echo "${major}.${minor}.${patch}"
}

# Function to update version in Version.h
update_version_file() {
    local new_major=$1
    local new_minor=$2
    local new_patch=$3
    local build_type=${4:-"release"}
    
    # Create backup
    cp $VERSION_FILE "${VERSION_FILE}.bak"
    
    # Update version numbers
    sed -i.tmp "s/#define CV_TO_OSC_VERSION_MAJOR.*/#define CV_TO_OSC_VERSION_MAJOR $new_major/" $VERSION_FILE
    sed -i.tmp "s/#define CV_TO_OSC_VERSION_MINOR.*/#define CV_TO_OSC_VERSION_MINOR $new_minor/" $VERSION_FILE
    sed -i.tmp "s/#define CV_TO_OSC_VERSION_PATCH.*/#define CV_TO_OSC_VERSION_PATCH $new_patch/" $VERSION_FILE
    sed -i.tmp "s/#define CV_TO_OSC_VERSION_BUILD.*/#define CV_TO_OSC_VERSION_BUILD \"$build_type\"/" $VERSION_FILE
    
    # Clean up temporary files
    rm -f "${VERSION_FILE}.tmp"
    
    print_success "Updated version to ${new_major}.${new_minor}.${new_patch}"
}

# Function to create or update CHANGELOG.md
update_changelog() {
    local version=$1
    local date=$(date +"%Y-%m-%d")
    
    if [ ! -f "$CHANGELOG_FILE" ]; then
        cat > "$CHANGELOG_FILE" << EOF
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [$version] - $date

### Added
- Initial release of CV to OSC Converter
- Real-time CV to OSC conversion with sub-millisecond latency
- Configurable CV input ranges per channel
- Auto-calibration system for precise voltage measurement
- Advanced signal filtering (low-pass, high-pass, median, etc.)
- Performance monitoring with real-time metrics
- Hot configuration reloading without restart
- Configuration profiles for different setups
- Comprehensive testing suite with 46+ automated tests
- Cross-platform support (macOS, Linux, Windows)

### Changed
- N/A

### Deprecated
- N/A

### Removed
- N/A

### Fixed
- N/A

### Security
- N/A

EOF
    else
        # Add new version entry to existing changelog
        local temp_file=$(mktemp)
        
        # Extract header
        head -n 6 "$CHANGELOG_FILE" > "$temp_file"
        
        # Add new version entry
        echo "" >> "$temp_file"
        echo "## [$version] - $date" >> "$temp_file"
        echo "" >> "$temp_file"
        echo "### Added" >> "$temp_file"
        echo "- Version $version release" >> "$temp_file"
        echo "" >> "$temp_file"
        echo "### Changed" >> "$temp_file"
        echo "- Performance improvements and bug fixes" >> "$temp_file"
        echo "" >> "$temp_file"
        
        # Add rest of existing changelog
        tail -n +7 "$CHANGELOG_FILE" >> "$temp_file"
        
        mv "$temp_file" "$CHANGELOG_FILE"
    fi
    
    print_success "Updated CHANGELOG.md"
}

# Function to create git tag
create_git_tag() {
    local version=$1
    local tag_name="v$version"
    
    # Check if tag already exists
    if git tag -l | grep -q "^$tag_name$"; then
        print_error "Tag $tag_name already exists"
        return 1
    fi
    
    # Create annotated tag
    print_info "Creating git tag: $tag_name"
    git tag -a "$tag_name" -m "Release version $version"
    
    print_success "Created git tag: $tag_name"
    print_info "Push with: git push origin $tag_name"
}

# Function to bump version
bump_version() {
    local bump_type=$1
    local current_version=$(get_current_version)
    
    # Parse current version
    IFS='.' read -ra VERSION_PARTS <<< "$current_version"
    local major=${VERSION_PARTS[0]}
    local minor=${VERSION_PARTS[1]}
    local patch=${VERSION_PARTS[2]}
    
    # Bump version based on type
    case $bump_type in
        "major")
            major=$((major + 1))
            minor=0
            patch=0
            ;;
        "minor")
            minor=$((minor + 1))
            patch=0
            ;;
        "patch")
            patch=$((patch + 1))
            ;;
        *)
            print_error "Invalid bump type: $bump_type. Use 'major', 'minor', or 'patch'"
            exit 1
            ;;
    esac
    
    local new_version="${major}.${minor}.${patch}"
    
    print_info "Bumping version from $current_version to $new_version"
    
    # Update files
    update_version_file $major $minor $patch "release"
    update_changelog $new_version
    
    print_success "Version bumped to $new_version"
    
    # Ask if user wants to commit changes
    read -p "Commit changes? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        git add $VERSION_FILE $CHANGELOG_FILE
        git commit -m "chore: bump version to $new_version"
        print_success "Changes committed"
        
        # Ask if user wants to create tag
        read -p "Create git tag? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            create_git_tag $new_version
        fi
    fi
}

# Function to set specific version
set_version() {
    local new_version=$1
    
    # Validate version format
    if [[ ! $new_version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        print_error "Invalid version format: $new_version. Use semantic versioning (e.g., 1.2.3)"
        exit 1
    fi
    
    # Parse version
    IFS='.' read -ra VERSION_PARTS <<< "$new_version"
    local major=${VERSION_PARTS[0]}
    local minor=${VERSION_PARTS[1]}
    local patch=${VERSION_PARTS[2]}
    
    print_info "Setting version to $new_version"
    
    # Update files
    update_version_file $major $minor $patch "release"
    update_changelog $new_version
    
    print_success "Version set to $new_version"
}

# Function to show current version
show_version() {
    local current_version=$(get_current_version)
    echo "Current version: $current_version"
    
    # Show git information if available
    if command -v git &> /dev/null && git rev-parse --git-dir > /dev/null 2>&1; then
        local git_hash=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
        local git_branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
        echo "Git commit: $git_hash"
        echo "Git branch: $git_branch"
        
        # Check if working directory is clean
        if ! git diff-index --quiet HEAD -- 2>/dev/null; then
            print_warning "Working directory has uncommitted changes"
        fi
    fi
}

# Function to prepare for development
set_development() {
    local current_version=$(get_current_version)
    
    # Parse current version
    IFS='.' read -ra VERSION_PARTS <<< "$current_version"
    local major=${VERSION_PARTS[0]}
    local minor=${VERSION_PARTS[1]}
    local patch=${VERSION_PARTS[2]}
    
    print_info "Setting development build"
    update_version_file $major $minor $patch "dev"
    print_success "Set to development build"
}

# Main script logic
case "${1:-}" in
    "bump")
        if [ -z "${2:-}" ]; then
            print_error "Bump type required. Use: major, minor, or patch"
            exit 1
        fi
        bump_version "$2"
        ;;
    "set")
        if [ -z "${2:-}" ]; then
            print_error "Version required. Use semantic versioning (e.g., 1.2.3)"
            exit 1
        fi
        set_version "$2"
        ;;
    "get"|"show")
        show_version
        ;;
    "tag")
        local current_version=$(get_current_version)
        create_git_tag "$current_version"
        ;;
    "dev"|"development")
        set_development
        ;;
    "help"|"-h"|"--help")
        echo "Version Management Script"
        echo ""
        echo "Usage:"
        echo "  $0 bump patch          # Increment patch version (1.0.0 -> 1.0.1)"
        echo "  $0 bump minor          # Increment minor version (1.0.0 -> 1.1.0)"
        echo "  $0 bump major          # Increment major version (1.0.0 -> 2.0.0)"
        echo "  $0 set 1.2.3           # Set specific version"
        echo "  $0 get                 # Show current version"
        echo "  $0 tag                 # Create git tag for current version"
        echo "  $0 dev                 # Set development build"
        echo "  $0 help                # Show this help"
        echo ""
        echo "Examples:"
        echo "  $0 bump patch          # For bug fixes"
        echo "  $0 bump minor          # For new features"
        echo "  $0 bump major          # For breaking changes"
        ;;
    *)
        print_error "Unknown command: ${1:-}"
        echo "Use '$0 help' for usage information"
        exit 1
        ;;
esac
