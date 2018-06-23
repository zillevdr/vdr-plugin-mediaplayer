This is a "plugin" for the Video Disk Recorder (VDR).

Copyright (c) 2018 by zillevdr.  All Rights Reserved.

Written by:                  zille <zillevdr@gmx.de>

Project's homepage:          https://github.com/zillevdr/

Latest version available at: https://github.com/zillevdr/

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
See the file COPYING for more information.

Description:

vdr-mediaplayer play actually mp3 files using the output device for decoding and output.

Requirements:

- Output device that decode mediafiles. Testet with softhddevice-drm.
- mediafiles

Install:

- git clone https://github.com/zillevdr/vdr-mediaplayer.git
- cd vdr-mediaplayer
- make install
- change vdr commandline ... -P "mediaplayer --startpath=/path/to/files" ...

Commandline option for the plugin:

  --startpath=/path/to/files	path to start search mediafiles (default /)

Todo:

- jump in file for and backward
- other media files (depending from output device)
- ID3Tag
