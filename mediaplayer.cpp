/*
 * Mediaplayer plugin for VDR
 *
 * (C) 2018 by zille.  All Rights Reserved.
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

#include <getopt.h>
#include <string>

using std::string;

#include <vdr/player.h>
#include <vdr/plugin.h>

#include "mediaplayer.h"
#include "player.h"
#include "control.h"
#include "menu.h"
#include "setupmenu.h"

//////////////////////////////////////////////////////////////////////////////
//	cPluginMediaplayer
//////////////////////////////////////////////////////////////////////////////

// Default Settings
string cPluginMediaplayer::StartPath = "/";
volatile int cPluginMediaplayer::BufferOffset = 10;

cPluginMediaplayer::cPluginMediaplayer(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginMediaplayer::~cPluginMediaplayer()
{
  // Clean up after yourself!
}

const char *cPluginMediaplayer::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return
  "  --startpath=/path/to/files\tpath to start search mediafiles (default /)\n"
  ;
}

bool cPluginMediaplayer::ProcessArgs(int argc, char *argv[])
{
  static struct option long_options[] =
  {
    { "startpath", required_argument, NULL, 'p' },
    { NULL, no_argument, NULL, '\0' }
  };

  int c, option_index = 0;

  while ((c = getopt_long(argc, argv, "p", long_options, &option_index)) != -1) {
    switch (c) {
      case 'p':
        StartPath.assign(optarg);
        break;
      default:
        esyslog("MediaPlayer unknown option %c", c);
        return false;
    }
  }
  return true;
}

bool cPluginMediaplayer::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool cPluginMediaplayer::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginMediaplayer::Stop(void)
{
  // Stop any background activities the plugin is performing.
}

void cPluginMediaplayer::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginMediaplayer::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginMediaplayer::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

time_t cPluginMediaplayer::WakeupTime(void)
{
  // Return custom wakeup time for shutdown script
  return 0;
}

cOsdObject *cPluginMediaplayer::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return new cMediaPlayerMenu("Mediaplayer");
}

cMenuSetupPage *cPluginMediaplayer::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cMenuSetupMediaplayer();
}

bool cPluginMediaplayer::SetupParse(const char *Name, const char *Value)
{
  if (!strcasecmp(Name, "BufferOffset")) {
    BufferOffset = atoi(Value);
    return true;
  }
  return false;
}

bool cPluginMediaplayer::Service(__attribute__ ((unused))const char *Id,
  __attribute__ ((unused))void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginMediaplayer::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginMediaplayer::SVDRPCommand(__attribute__ ((unused))const char *Command,
  __attribute__ ((unused))const char *Option, __attribute__ ((unused))int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}



VDRPLUGINCREATOR(cPluginMediaplayer); // Don't touch this!
