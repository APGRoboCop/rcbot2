# RCBot2 Documentation

Welcome to the comprehensive documentation for RCBot2!

## Table of Contents

### Getting Started
- **[Installation Guide](#installation)** - Installing RCBot2 on your server
- **[Quick Start](#quick-start)** - Get up and running in minutes
- **[Command Reference](commands.md)** - Complete list of console commands
- **[Configuration Guide](configuration.md)** - Configure bots and server settings

### For Server Operators
- **[Waypoint Guide](waypoints.md)** - Download, install, and create waypoints
- **[Troubleshooting](troubleshooting.md)** - Common issues and solutions
- **[Performance Tuning](performance.md)** - Optimize bot performance
- **[Game-Specific Guides](game-guides/)** - Guides for TF2, DOD:S, CS:S, etc.

### For Developers
- **[Development Guide](../claude.md)** - Architecture, coding guidelines, workflow
- **[Building Guide](building.md)** - Compile RCBot2 from source
- **[API Reference](api.md)** - SourceMod natives and integration
- **[Contributing Guide](contributing.md)** - How to contribute to RCBot2

### Project Information
- **[Project Roadmap](../roadmap.md)** - Future plans and development priorities
- **[Changelog](CHANGELOG.md)** - Version history and changes
- **[FAQ](faq.md)** - Frequently asked questions
- **[License](../LICENSE)** - GNU AGPL-3.0 license information

## Installation

### Requirements

- Source Engine dedicated server (TF2, DOD:S, HL2:DM, CS:S, etc.)
- MetaMod:Source 1.10+ installed
- SourceMod (optional, for extended functionality)

### Quick Install

1. [Install MetaMod:Source](https://wiki.alliedmods.net/Installing_Metamod:Source) on your server
2. Download the [latest RCBot2 release](https://github.com/ethanbissbort/rcbot2/releases)
3. Extract into your game directory (e.g., `tf/` or `cstrike/`)
4. Download waypoints for your maps from the [waypoint repository](http://rcbot.bots-united.com/waypoints.php)
5. Extract waypoints to `rcbot2/waypoints/{game}/`
6. Restart your server
7. Type `rcbotd` in console to verify installation

See the full installation guide for detailed instructions.

## Quick Start

### Adding Bots

```
rcbot_create               # Create one bot
rcbot_quota 8              # Set bot count to 8
rcbot_create_class scout   # Create a specific class (TF2)
```

### Managing Bots

```
rcbot_removeall            # Remove all bots
rcbot_debug 2              # Enable debug output
rcbot_change_classes 1     # Allow bots to change classes
```

### Waypoint Commands

```
rcbot_autowaypoint on      # Enable automatic waypointing
rcbot_wpt_save             # Save waypoints
rcbot_wpt_load             # Load waypoints
```

For complete command reference, see [commands.md](commands.md).

## Support and Community

### Getting Help

- **GitHub Issues**: [Report bugs or request features](https://github.com/ethanbissbort/rcbot2/issues)
- **Discord**: [Bots United Discord](https://discord.gg/5v5YvKG4Hr)
- **Forums**: [Official RCBot2 Forums](http://rcbot.bots-united.com/forums/)

### Resources

- **Official Website**: http://rcbot.bots-united.com/
- **Waypoint Downloads**: http://rcbot.bots-united.com/waypoints.php
- **AlliedModders Wiki**: https://wiki.alliedmods.net/

## Contributing

We welcome contributions! See the [Contributing Guide](contributing.md) for details on:

- Reporting bugs
- Submitting code
- Creating waypoints
- Improving documentation
- Testing features

## License

RCBot2 is released under the **GNU Affero General Public License v3.0 (AGPL-3.0)**.

This means any modifications must have sources available to players on your server.

See [LICENSE](../LICENSE) for full license text.

---

**Last Updated**: 2025-11-21
**Version**: 1.7+
**Maintainer**: RCBot2 Community
