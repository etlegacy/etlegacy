SDL3 Upgrade Notes
==================

Summary
-------
- Switched the project from SDL2 to SDL3 3.2.x (bundled or system).
- Removed SDL gamma ramp usage; gamma support is disabled.
- Ported window, input, and audio code paths to SDL3 APIs.
- Updated Android loader strings and build references to SDL3.

Build and CMake Changes
-----------------------
- Added ``cmake/FindSDL3.cmake`` and removed ``cmake/FindSDL2.cmake``.
- ``cmake/ETLSetupFeatures.cmake`` now calls ``find_package(SDL3 3.2 REQUIRED)``
  and defines ``SDL_ENABLE_OLD_NAMES`` for SDL2 compatibility macros.
- ``libs/CMakeLists.txt`` now downloads SDL3 3.2.0 and links SDL3 libraries
  when ``BUNDLED_SDL`` is enabled.
- Updated bundled SDL paths/names from ``SDL2`` to ``SDL3``.
- ``cmake/ETLPlatform.cmake`` comments updated to remove SDL2 wording.

Core SDL Header Updates
-----------------------
- ``src/sdl/sdl_defs.h`` includes SDL3 headers and enables
  ``SDL_ENABLE_OLD_NAMES``.
- ``src/sdl/sdl_glimp.c`` and ``src/sys/sys_win32.c`` include SDL3 headers.
- ``vendor/tinygettext/tinygettext/iconv.hpp`` now includes ``SDL3/SDL.h``.

Audio (SDL3)
------------
File: ``src/sdl/sdl_snd.c``

- Migrated to SDL3's audio device stream model:
  - Uses ``SDL_OpenAudioDeviceStream`` with a stream callback.
  - Locks/unlocks via ``SDL_LockAudioStream``/``SDL_UnlockAudioStream``.
  - Starts playback with ``SDL_ResumeAudioStreamDevice``.
- Device enumeration switched to ``SDL_GetAudioPlaybackDevices`` and
  ``SDL_GetAudioDeviceName`` (device id based).
- Audio format reporting updated for SDL3 format constants.
- ``SDL_AudioSpec`` usage updated (no ``samples``, ``silence``, ``size`` fields).

Input (SDL3)
------------
File: ``src/sdl/sdl_input.c``

- Keyboard input now uses ``SDL_KeyboardEvent`` directly instead of ``SDL_Keysym``.
- Event handling updated to SDL3 event types:
  - Window events map to ``SDL_EVENT_WINDOW_*``.
  - Gamepad axis uses ``e.gaxis`` instead of ``e.caxis``.
- Joystick/gamepad handling updated:
  - Enumeration via ``SDL_GetJoysticks`` and IDs.
  - Gamepads via ``SDL_IsGamepad`` and ``SDL_OpenGamepad``.
  - Event toggles use ``SDL_SetJoystickEventsEnabled`` and
    ``SDL_SetGamepadEventsEnabled``.
- Mouse grab/relative mode updated:
  - ``SDL_SetWindowRelativeMouseMode`` and ``SDL_SetWindowMouseGrab``.
  - Cursor toggles use ``SDL_ShowCursor``/``SDL_HideCursor``.
- Text input uses ``SDL_StartTextInput(mainScreen)`` and
  ``SDL_StopTextInput(mainScreen)``.
- Drop event payload moved to ``e.drop.data``.
- Display index lookup uses SDL3 display IDs.

Windowing/Video (SDL3)
----------------------
File: ``src/sdl/sdl_glimp.c``

- Display enumeration uses SDL3 display IDs:
  - ``SDL_GetDisplays`` / ``SDL_GetDisplayForWindow``.
  - ``SDL_GetFullscreenDisplayModes`` / ``SDL_GetDesktopDisplayMode``.
- Window creation uses SDL3 signature:
  - ``SDL_CreateWindow(title, w, h, flags)`` then ``SDL_SetWindowPosition``.
- Fullscreen handling:
  - Uses ``SDL_GetClosestFullscreenDisplayMode`` for exclusive modes.
  - Falls back to borderless fullscreen via
    ``SDL_SetWindowFullscreenMode(window, NULL)``.
  - Applies fullscreen only after window and GL context creation.
- Window icon creation uses ``SDL_CreateSurfaceFrom`` with ``SDL_PIXELFORMAT_RGBA32``.
- Environment variable set via ``SDL_SetEnvironmentVariable``.
- SDL version logging uses ``SDL_VERSION`` and ``SDL_GetVersion``.
- Removed SDL gamma ramp usage:
  - ``deviceSupportsGamma`` forced false.
  - ``GLimp_SetGamma`` is a no-op.

System/UI/Strings
-----------------
- ``src/sys/sys_main.c`` message box field renamed to ``buttonID`` and
  ``SDL_ShowMessageBox`` return handling updated.
- Sound backend strings updated to ``SDL3``:
  - ``src/client/snd_main.c``
  - ``etmain/ui/options_system.menu``
- ``src/ui/keycodes.h`` comment updated to mention SDL3 gamepad ordering.
- Docker comment updated to SDL3 wording in ``misc/docker/build.Dockerfile``.

Android
-------
- ``app/jni/src/Android.mk`` uses ``SDL3`` library name.
- ``app/src/main/java/org/libsdl/app/SDLActivity.java`` loads ``SDL3``.

Gamma Ramp Removal
------------------
- The engine no longer attempts gamma ramp or brightness probing via SDL.
- ``glConfig->deviceSupportsGamma`` is always false on SDL3.

Build/Run Validation
--------------------
- Build: ``./build.sh`` succeeds after SDL3 port.
