# RCBot2 Documentation

Welcome to the RCBot2 documentation!

---

## Documentation Files

| Document | Description |
|----------|-------------|
| **[USAGE.md](USAGE.md)** | Installation, commands, configuration, troubleshooting, SourceMod API |
| **[waypoints.md](waypoints.md)** | Creating, editing, auto-testing, and refining waypoints |
| **[BUILDING.md](BUILDING.md)** | Compiling RCBot2 from source |
| **[ML.md](ML.md)** | Machine learning features (not yet implemented) |

---

## Quick Start

### Installation

1. Install [MetaMod:Source](https://wiki.alliedmods.net/Installing_Metamod:Source) on your server
2. Download [RCBot2](https://github.com/ethanbissbort/rcbot2/releases)
3. Extract to your game directory
4. Download [waypoints](http://rcbot.bots-united.com/waypoints.php) for your maps
5. Restart server
6. Verify: `rcbotd`

### Adding Bots

```
rcbot addbot               // Add one bot
rcbot_quota 8              // Maintain 8 bots
rcbot addbot soldier       // Add specific class (TF2)
```

### Managing Bots

```
rcbot kickbot              // Remove one bot
rcbot_removeall            // Remove all bots
rcbot debug 2              // Enable debug
```

### Waypoints

```
rcbot wpt on               // Show waypoints
rcbot wpt load             // Load waypoints
rcbot wpt save             // Save waypoints
```

For complete command reference, see [USAGE.md](USAGE.md).

---

## Supported Games

- Team Fortress 2
- Day of Defeat: Source
- Half-Life 2: Deathmatch
- Counter-Strike: Source

---

## Resources

- **Website**: http://rcbot.bots-united.com/
- **Waypoints**: http://rcbot.bots-united.com/waypoints.php
- **GitHub**: https://github.com/ethanbissbort/rcbot2
- **Discord**: [Bots United](https://discord.gg/5v5YvKG4Hr)
- **Forums**: http://rcbot.bots-united.com/forums/

---

## License

RCBot2 is released under the **GNU Affero General Public License v3.0**.

---

**Last Updated**: 2025-12-27
