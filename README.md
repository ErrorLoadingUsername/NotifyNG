# NotifyNG

SKSE plugin for Skyrim SE / AE (including 1.7.104) that keeps every HUD notification - the short
messages in the corner of the screen - and lists them on a page in the SKSE Menu Framework.
Optionally they are also written to `NotifyNG-Notifications.log`.

- Catches notifications from Papyrus scripts (`Debug.Notification`), from the game itself and from
  other SKSE plugins - everything the HUD shows as a notification.
- Each entry: real time, in-game date and time, location, text. The same message several times in a
  row is counted instead of repeated.
- Search, newest first, adjustable list size, German / English.
- Settings: `Documents\My Games\Skyrim Special Edition\SKSE\NotifyNG.ini`.

## Requirements

- SKSE64 for your game version, Address Library for SKSE Plugins
- SKSE Menu Framework (for the in-game page)

## Build

```
git clone --recursive https://github.com/ErrorLoadingUsername/NotifyNG
```

Put `SKSEMenuFramework.h` into `include/` (see [include/README.md](include/README.md)), then open
the folder in Visual Studio 2022 and build the `release` preset (vcpkg).

## License

[GPL-3.0-or-later](LICENSE) - built with CommonLibSSE NG (alandtse), which is GPL-3.0-or-later.
