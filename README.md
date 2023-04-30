# Libarch: An ARM64 Disassembly Framework.

![Libarch](https://github.com/h3adshotzz/libarch/actions/workflows/main.yml/badge.svg?branch=working)

See the [Releases](https://github.com/h3adshotzz/libarch/releases) page for downloads. The current `master` branch is `1.0.0`. Libarch will move to the GPL-3.0 license as-of version 1.1. 

## Description

Libarch is a disassembly framework that is designed specifically for the ARM64/AArch64 architecture. It is a lightweight and easy-to-use library that provides the tools you need to reverse engineer binaries that have been compiled for 64-bit ARM processors.

The development of Libarch was primarily driven by the need for a reliable and accurate disassembly tool for the BSc Computer Science project HTool. HTool used Libarch to provide disassembly of Mach-O binaries and iOS/macOS firmware files, and the library was instrumental in the success of the project.

![Example of Libarch disassembly output](example.png)

## Installation

To use Libarch in your project, you can add it as a Git Submodule and include it via CMake. This allows you to easily integrate Libarch into your build system and start using its disassembly functionality.

While there is currently no support for installing Libarch via a package manager, the Git Submodule approach provides a reliable and straightforward way to use the library in your project.

In addition to using Libarch as a library, you can also develop your own disassembly tools as part of the Libarch codebase under the tools/ directory. This can be a convenient way to build on top of the existing functionality provided by Libarch and create custom disassembly tools that meet your specific needs.


## Contributing

Libarch is an open-source project that welcomes contributions from the community. If you're interested in contributing to the project, the process is straightforward: simply fork the repository, make your changes, and create a Pull Request.

I will do my best to review and merge any contributions as quickly as possible, and appreciate any contributions that can help improve the project.

While there currently is no formal code style or guidelines document, I plan to develop one soon. In the meantime, I encourage contributors to follow best practices and common coding conventions to keep the codebase clean and maintainable.


## Documentation

Documentation will be available on the Libarch tab of the Projects page on h3adsh0tzz.com. This will provide a central location for users to find the latest documentation, as well as other resources related to Libarch.