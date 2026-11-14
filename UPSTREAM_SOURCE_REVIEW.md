# Upstream Source Review for BreadOS

Updated: 2026-11-13

This document records an initial architecture and licensing review of the requested upstream sources. It is not a claim that the repositories have been cloned, merged, or fully ported. The upstream repositories are maintained by their respective owners; the authenticated GitHub connection has read access but does not have push permission to those upstream repositories. Changes intended for BreadOS must be committed to JAPStudios/BreadOS.

## Integration rule

Do not copy entire organisations or repositories into BreadOS. Their repositories contain different operating systems, kernels, language runtimes, build systems, hardware assumptions, third-party dependencies, and licences. Import only identifiable components that solve a BreadOS requirement, after checking the exact source revision, licence, notices, dependencies, architecture, build compatibility, security history, and test coverage. Preserve copyright and licence notices. Record the source commit and local modifications.

BreadOS is a freestanding 32-bit x86 operating system with a native kernel and desktop. It does not adopt Linux, Arch, systemd, pacman, Wayland, Hyprland, Quickshell, D-Bus, or the Linux userspace ABI. A repository's usefulness as a visual or feature reference does not make its implementation code portable.

## GrapheneOS

Requested source: https://github.com/orgs/GrapheneOS/repositories

The organisation contains Android platform components, Android applications, kernel/device trees, build infrastructure, and standalone security-related projects. Most Android platform repositories are not drop-in components for a freestanding 32-bit x86 kernel.

### Candidate: hardened_malloc

Source: https://github.com/GrapheneOS/hardened_malloc

The upstream README describes a hardened general-purpose allocator with heap-corruption mitigations. Its stated supported environments are Bionic, musl, and glibc; the project explicitly states that its design targets 64-bit systems and fundamentally does not support 32-bit. It therefore cannot be copied into the current 32-bit BreadOS kernel as a working allocator.

The repository's LICENSE is MIT. Any future reuse still requires preserving the copyright and permission notice. A future 64-bit BreadOS userspace allocator could evaluate the design and port only after a compatible memory-management and C-library environment exists. The existing BreadOS kernel heap is a separate implementation and should not be described as hardened_malloc.

Source revision inspected: README blob 4163475972d8a7748ca4514ff0f5d55849d76de5; LICENSE blob 297b203c6b4a30d743d9acbd2e6fee4a80ecd471.

### Other GrapheneOS repositories

Vanadium, Auditor, AppStore, Seedvault, Gallery, Settings, platform framework, platform build, Bionic, device trees, and Android system components are potential references for future application or platform requirements, not direct kernel imports. Their Android framework, runtime, and device assumptions must be evaluated individually. No wholesale import is approved by this review.

## omacom

Requested source: https://github.com/orgs/omacom/repositories

The omacom/omarchy repository is explicitly a Linux distribution. Its README documents Linux applications and workflows, including Hyprland-related desktop components, terminal and shell tools, package management, browser and GUI applications, snapshots, and system configuration. Its current role for BreadOS remains a feature and interaction reference, not a source of Linux dependencies or a Linux-based architecture.

The Omarchy repository's LICENSE is MIT. BreadOS already contains a set of native desktop features and 23 colour palettes ported from Omarchy theme data. Further reuse must identify the exact file and source revision and preserve the required notice. Do not import Linux shell scripts, package recipes, Hyprland configuration, systemd services, or Linux-specific desktop services into the freestanding kernel.

Inspected source revision: README blob 5834780f5b789190faf215880094f0ad291ffe8a; LICENSE blob f12cfa766c55541c9d8e008e310c95a721e70a97.

## RetroPie

Requested source: https://github.com/orgs/RetroPie/repositories

RetroPie contains emulator front ends, emulator projects, setup scripts, themes, shaders, and Raspberry Pi/Linux-specific integration. RetroPie-Setup explicitly expects Raspberry Pi OS, Ubuntu, or related Linux systems and relies on APT and Linux package installation. It cannot be integrated as an operating-system installer or native BreadOS subsystem.

RetroPie/EmulationStation is a potentially useful reference for a future native game-library and controller-oriented launcher. Its README lists dependencies including SDL2, FreeImage, FreeType, LibVLC, cURL, and RapidJSON. Those dependencies and its graphics/audio/runtime assumptions are not present in the current BreadOS kernel. A port would require a supported userspace, C++ runtime, graphics/input APIs, and dependency ports. The repository's root LICENSE path was not present in the inspected source; determine the actual licensing files and notices before copying any code.

RetroPie-Setup is a Linux setup script, not portable BreadOS source. Its root LICENSE path was not present in the inspected source; determine the licensing terms of each selected component independently.

Inspected README revisions: EmulationStation blob 0f16f47a237d80d3fac49aac70774d545583befe; RetroPie-Setup blob 14a453115ca6ba8799a1c833bda3c6af1d422132.

## pewdiepie-archdaemon/dionysus

Requested source: https://github.com/pewdiepie-archdaemon/dionysus

The GitHub contents API returned 404 for the requested repository README. The repository's existence, visibility, source tree, current revision, and licence therefore could not be verified through the connected GitHub access. No code can responsibly be imported until the repository is accessible and its source and licence can be inspected.

## Next technical integration priorities

1. Continue native BreadOS development rather than importing an incompatible operating system.
2. Review GrapheneOS security designs for process isolation, memory safety, permission enforcement, verified updates, and application trust; port only components whose architecture and licence fit.
3. Evaluate selected Omarchy theme assets and desktop interaction patterns as source-level references while preserving BreadOS's native implementation.
4. Defer EmulationStation integration until BreadOS has a general userspace ABI, graphics/audio/input APIs, C++ runtime, and a dependency strategy.
5. Revisit hardened_malloc only for a future 64-bit userspace environment; its current upstream implementation is not a 32-bit kernel allocator.
6. Resolve the Dionysus repository access and licensing question before considering it.

## Access and commit status

The upstream repositories inspected here are not writable through the connected GitHub account. This review can be committed to JAPStudios/BreadOS. It does not claim that upstream repositories were cloned in full or that their contents were merged into BreadOS.
