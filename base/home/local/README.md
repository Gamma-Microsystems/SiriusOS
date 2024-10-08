# Welcome to SiriusOS!

SuruysOS provides a familiar Unix-like environment, but please be
aware that the shell is incomplete and does not implement all Unix
shell features. For help with the shell's syntax and built-in
functions, run `help`. For a list of available commands, press Tab
twice. Tab completion is available for both commands and file names.

To install packages, use the `sirpkg` tool. You can install a GCC/binutils
toolchain with:

    permit sirpkg install build-essential

Or you can install some games with:

    permit sirpkg install doom quake

The password for the default user (`local`) is `local`.

SiriusOS's compositing window server includes many common keybindings:
- Hold Alt to drag windows.
- Super (Win) combined with the arrow keys will "grid" windows to the
  sides or top and bottom of the screen. Combine with Ctrl and Shift
  for quarter-sized gridding.
- Alt-F10 maximized and unmaximizes windows.
- Alt-F4 closes windows.
