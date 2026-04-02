# smartmet-library-macgyver

Part of [SmartMet Server](https://github.com/fmidev/smartmet-library-spine). See the [SmartMet Server documentation](https://github.com/fmidev/smartmet-library-spine) for an overview of the ecosystem.

## Overview

The macgyver library is the general-purpose utility library of SmartMet Server. It provides a wide range of common functionality used across all server components.

## Features

- **Astronomy** — solar and lunar position calculations, lunar phase
- **Caching** — LRU cache with configurable size and statistics
- **Date and time** — flexible datetime parsing and formatting, timezone support
- **Filesystem** — directory monitoring, file utilities
- **String utilities** — charset conversion, Base64 encoding/decoding, CSV reading
- **Async tasks** — task groups with cancellation support
- **Atomic shared pointers** — lock-free shared pointer implementation
- **Exception handling** — structured exceptions with stack traces

## Usage

Macgyver is a foundational dependency of most SmartMet Server libraries and components, including [smartmet-library-spine](https://github.com/fmidev/smartmet-library-spine).

## License

MIT — see [LICENSE](LICENSE)

## Contributing

Bug reports and pull requests are welcome on [GitHub](../../issues).
