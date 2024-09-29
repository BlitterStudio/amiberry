-- This code was adapted to serve the application needs.
--
-- Original code from: https://gitlab.kitware.com/cmake/cmake/-/blob/d3812437036e95329fbee0773282b88e8b013fbe/Packaging/CMakeDMGSetup.scpt
-- Original license text:
--
-- CMake - Cross Platform Makefile Generator
-- Copyright 2000-2016 Kitware, Inc.
-- Copyright 2000-2011 Insight Software Consortium
-- All rights reserved.
--
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions
-- are met:
--
-- * Redistributions of source code must retain the above copyright
--   notice, this list of conditions and the following disclaimer.
--
-- * Redistributions in binary form must reproduce the above copyright
--   notice, this list of conditions and the following disclaimer in the
--   documentation and/or other materials provided with the distribution.
--
-- * Neither the names of Kitware, Inc., the Insight Software Consortium,
--   nor the names of their contributors may be used to endorse or promote
--   products derived from this software without specific prior written
--   permission.
--
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
-- "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
-- LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
-- A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
-- HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
-- SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
-- LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
-- DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
-- THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
-- (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
-- OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

on run argv
  set image_name to item 1 of argv

  tell application "Finder"
    tell disk image_name

    -- Wait for the image to finish mounting.
      set open_attempts to 0
      repeat while open_attempts < 4
        try
          open
          delay 1
          set open_attempts to 5
          close
        on error errStr number errorNumber
          set open_attempts to open_attempts + 1
          delay 10
        end try
      end repeat
      delay 5

      -- Open the image and save a .DS_Store with background and icon setup.
      open
      set current view of container window to icon view
      set theViewOptions to the icon view options of container window
      set background picture of theViewOptions to file ".background:background.tiff"
      set arrangement of theViewOptions to not arranged
      set icon size of theViewOptions to 128
      delay 5
      close

      -- Setup the position of the app and Applications symlink
      -- and hide all the window decoration.
      open
      tell container window
        set sidebar width to 0
        set statusbar visible to false
        set toolbar visible to false
        -- Those bounds are defined as:
        -- x-start, y-start, x-end, y-end (aka. x, z, width, height)
        -- Making this 540 x 400 in size + some random 8 pixel to actually fit the content ¯\_(ツ)_/¯
        set the bounds to {400, 100, 940, 528}
        set position of item "SmallSDL2App.app" to {140, 200}
        set position of item "Applications" to {405, 200}
      end tell
      delay 5
      close

      -- Open and close for visual verification.
      open
      delay 5
      close

    end tell
    delay 1
  end tell
end run
