NexPad
======

Controller-first desktop navigation for Windows.

NexPad is a lightweight Windows utility that maps controller input to mouse and keyboard actions for desktop navigation, media control, and couch-first PC use. It is designed for living-room PCs, handhelds, and setups where a gamepad is more practical than a keyboard and mouse.

Highlights
======

* Controller-driven cursor movement and scrolling
* Mouse click and keyboard mapping support
* Live settings and preset management in the native Windows UI
* Xbox XInput support plus PlayStation HID fallback already present in the codebase
* Small native Windows footprint with no runtime-heavy frontend stack

Compatible Controllers
======
### Xbox 360
* [Official Wired Xbox 360 Controller for Windows](https://www.amazon.com/Microsoft-Wired-Controller-Windows-Console/dp/B004QRKWLA/)
* [Official Wireless Xbox 360 Controller](https://www.amazon.com/Xbox-360-Wireless-Controller-Glossy-microsoft/dp/B003ZSP0WW/)
* [A compatible wireless adapter](https://www.amazon.com/Microsoft-Xbox-Wireless-Receiver-Windows/dp/B000HZFCT2/) is required to get a standard controller to connect to a Windows PC)

### Xbox One
* [Offical Xbox One Wireless Controller](https://www.amazon.com/Xbox-Wireless-Controller-Black-one/dp/B01LPZM7VI/)
* [A bluetooth adapter/antenna](https://www.amazon.com/Bluetooth-Wireless-Headphone-Controller-Keyboard/dp/B0774NZNGX/) is needed for XBone controller Bluetooth mode. Many laptops and tablets will *already* have this. Read the manual!
* [A wireless adapter](https://www.amazon.com/Microsoft-Xbox-Wireless-Adapter-Windows-one/dp/B00ZB7W4QU/) is needed if you want wireless mode.
* [A decently long MicroUSB cable](https://www.amazon.com/Charger-Durable-Charging-Smartphones-Motorola/dp/B06XZTK2JL/) will be needed for comfortable direct wired mode usage. This is the cheapest option if you already have the controller and no Bluetooth antenna!

### DualShock
DualShock controllers are great, but you NEED to emulate Xinput for NexPad to see and understand them. Fortunately, Xinput emulation is a very popular thing, as there are just as many people with DualShock controllers as there are Xbox controllers. Listings coming soon.

### Third party
SOME third party controllers will most likely work as well. I haven't seen one not work, but I'd imagine some wouldn't. Research before buying, NexPad expects native Xinput devices, so the controller should as well. I won't be listing any for now until I know what ones will work. If I can find one that does the job and saves you from Microsoft's extreme profit margins, I'll list it.

Table of contents
=================

  * [About](#about-nexpad)
  * [Features](#features)
  * [Default Controls](#default-controls)
  * [Requirements](#requirements)
  * [DualShock Controllers](#using-dualshock-controllers)
  * [Re-binding Controls](#config-file-instructions)
  * [Building](#build-instructions)
  * [Build Output Layout](#build-output-layout)
  * [License](#license)



About NexPad
======

NexPad turns a controller into a practical Windows input device. Analog sticks handle cursor movement and scrolling, buttons trigger clicks and keys, and the app stays small, fast, and configurable.

NexPad separates itself from the competition by being efficient, small, portable, free, and fully open. If you have something you'd like to see improved, added, or changed, please fill out the survey.

Features
======

* Desktop cursor control from a controller
* Scroll wheel emulation
* Mouse click mapping
* Keyboard mapping for controller buttons and triggers
* Enable and disable toggle shortcuts
* Preset save, load, import, and export support
* Native Windows configuration UI

Requirements
======
NexPad is incredibly great at being a standalone program, but with one major exception: it absolutely needs Visual C++ 2015 Runtimes to be installed. If you have run Windows Updates at least once in the lifetime of your computer, this really won't be an issue.

Using DualShock Controllers
======
DualShock controllers don't use typical xinput libraries like the X360 and Xbone controllers do, so you'll need something like InputMapper, SCP, DS4Windows, or DS3Tool to "emulate" an xinput device in order to get xinput-using applications like NexPad to understand it. NexPad DOES NOT automatically offer these emulation layers ( yet ;) ), so you'll need to use something to emulate it before NexPad can understand it.

Video Demonstration
======

https://vine.co/v/MYadBgWXuWY

[![NexPad Video 1](http://img.youtube.com/vi/UWYUodeontM/0.jpg)](https://www.youtube.com/watch?v=UWYUodeontM)

[![NexPad Video 2](http://img.youtube.com/vi/8APmA1ohPdM/0.jpg)](https://www.youtube.com/watch?v=8APmA1ohPdM)


Download Instructions
======

Use the repository's Releases page for packaged builds when releases are published.

I recommend that you copy it somewhere outside of the ZIP and make a shortcut to it. Adding it to your startup folder in your HTPC can make bootups a lot more convenient!

Default Controls
======
NexPad automatically generates a config file, which will contain documentation information on all input types and key bindings.

**A**: Left Mouse-Click.

**X**: Right Mouse-click.

**Y**: Hide terminal.

**B**: Enter.

**D-pad**: Arrow keys.

**Right Analog**: Scroll up/down.

**Right Analog Click**: F2.

**Left Analog**: Mouse.

**Left Analog Click**: Middle mouse click.

**Back**: Browser refresh

**Start**: Left Windows Key

**Start + Back**: Toggle. Useful for when you launch emulators or open Steam Big Picture mode. Press again to re-enable.

**Start + DPad Up**: Toggle NexPad vibration setting.

**LBumper**: Browser previous

**RBumper**: Browser next

**LBumber + RBummper**: Cycle speed (x3)

**LTrigger**: Space

**RTrigger**: Backspace

Config file instructions
======
There is a configuration file (config.ini) that can be reconfigured for simple keybindings.

If you delete or break the active config file, NexPad can generate a fresh one on the next run.

You can set which controller buttons will activate the configuration events based on the official microsoft keys hexadecimal values.

Virtual Windows Keys:
https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731

XInput Controller Buttons:
https://msdn.microsoft.com/en-us/library/windows/desktop/microsoft.directx_sdk.reference.xinput_gamepad%28v=vs.85%29.aspx

More instruction in the configuration file.


```diff
+ If you make a config file you feel could benefit people with the same use scenario as you, feel free to make a pull request for it in the public configs directory.
```


Build Instructions
======
Building is pretty straightforward, but you may get a "missing win32 include" error due to the solution targetting. Simply follow the instructions the error provides (Project -> Retarget solution) to ensure your project has a working link to the libraries it needs.

The Windows project now targets the Visual Studio 2022 `v143` toolset. You can build from Visual Studio with `Windows/NexPad.sln` or from the command line with MSBuild:

```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe' .\Windows\NexPad.sln /p:Configuration=Release /p:Platform=Win32
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe' .\Windows\NexPad.sln /p:Configuration=Release /p:Platform=x64
```

Build Output Layout
======
Release and Debug builds mirror the runtime layout from the repository root instead of writing final binaries into `Windows/Release` and `Windows/x64/Release`.

Release outputs:

```text
NexPad.exe
config.ini
presets/
x64/
  NexPad.exe
  config.ini
  presets/
release/
  NexPad.exe
  config.ini
  presets/
  README.md
  LICENSE
  x64/
    NexPad.exe
    config.ini
    presets/
```

Debug outputs:

```text
Debug/
  NexPad.exe
  config.ini
  presets/
x64/
  Debug/
    NexPad.exe
    config.ini
    presets/
```

Intermediate objects, PDBs, libs, and tlogs are written under `Windows/.build/`.

License
======
NexPad is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see http://www.gnu.org/licenses/.




![Controller GIF](https://thumbs.gfycat.com/ElasticUnrulyBighorn-max-1mb.gif)
