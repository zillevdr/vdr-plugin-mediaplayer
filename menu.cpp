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

#include <string>
#include <fstream>

using std::ifstream;
using std::string;

#include <vdr/interface.h>
#include <vdr/player.h>
#include <vdr/plugin.h>

#include "mediaplayer.h"
#include "player.h"
#include "control.h"
#include "menu.h"

cMediaPlayerMenu *cMediaPlayerMenu::pMenu = NULL;

cMediaPlayerMenu::cMediaPlayerMenu(const char *title, int c0, int c1, int c2, int c3,
    int c4)
:cOsdMenu(title, c0, c1, c2, c3, c4)
{
  pMenu = this;
  MenuSelPL = 0;
  if (!cMediaPlayer::Running) {
    Path = cPluginMediaplayer::GetStartPath();
    Playlist = cPlugin::ConfigDirectory("mediaplayer");
    Playlist.append("/default.m3u");
    FindFile(Path, true, NULL);
  } else {
    pPlayer = cMediaPlayer::pPlayer;
    PlayMenu();
    Playlist.assign(pPlayer->GetPlaylist());
  }
}

cMediaPlayerMenu::~cMediaPlayerMenu()
{
  pControl->MenuOpen = 0;
  pMenu = NULL;
}

void cMediaPlayerMenu::SelectPL(void)
{
  struct dirent **DirList;
  int n, i;

  if ((n = scandir(cPlugin::ConfigDirectory("mediaplayer"), &DirList, NULL, alphasort)) == -1) {
    esyslog("Mediaplayer: searching PL %s failed (%d): %m\n",
      cPlugin::ConfigDirectory("mediaplayer"), errno);
  } else {
    Clear();
    for (i = 0; i < n; i++) {
      if (DirList[i]->d_name[0] != '.' && (strcasestr(DirList[i]->d_name, ".M3U"))) {
        Add(new cOsdItem(tr(DirList[i]->d_name)));
      }
    }
    SetHelp("Select PL", NULL, NULL, NULL);
    Display();
    MenuSelPL = 1;
  }
}

void cMediaPlayerMenu::FindFile(string SearchPath, bool Menu, FILE *playlist)
{
  struct dirent **DirList;
  int n, i;
  const char * sp;

  if (!SearchPath.size())
    sp = "/";
  else sp = SearchPath.c_str();

  if (Menu) {
    Clear();
    if (SearchPath.size())
      Add(new cOsdItem(tr("[..]")));
  }

  if ((n = scandir(sp, &DirList, NULL, alphasort)) == -1) {
    esyslog("Mediaplayer: scanning directory %s failed (%d): %m\n", sp, errno);
  } else {
    for (i = 0; i < n; i++) {
      if (DirList[i]->d_type == DT_DIR && DirList[i]->d_name[0] != '.') {
        if (Menu)
          Add(new cOsdItem(tr(DirList[i]->d_name)));
        if (playlist) {
          string str = SearchPath + "/" + DirList[i]->d_name;
          FindFile(str.c_str(), false, playlist);
        }
      }
    }
    for (i = 0; i < n; i++) {
      if (DirList[i]->d_type == DT_REG && (strcasestr(DirList[i]->d_name, ".MP3"))) {
        if (Menu)
          Add(new cOsdItem(tr(DirList[i]->d_name)));
        if (playlist)
          fprintf(playlist, "%s/%s\n", SearchPath.c_str(), DirList[i]->d_name);
      }
    }
  }

  if (Menu) {
    SetHelp(cMediaPlayer::Running ? NULL : "Set new PL",
            "Add to PL",
            cMediaPlayer::Running ? NULL : "Play",
            cMediaPlayer::Running ? "Play Menu" : "Select PL");
    Display();
  }
  pControl->MenuOpen = 0;
}

void cMediaPlayerMenu::MakePlayList(const char * Target, const char * mode)
{
  FILE *playlist = fopen(Playlist.c_str(), mode);
  if (playlist != NULL) {
    if (strcasestr(Target, ".MP3")) {
      fprintf(playlist, "%s/%s\n", Path.c_str(), Target);
    } else {
      string str = Path + "/" + Target;
      FindFile(str.c_str(), false, playlist);
    }
  fclose (playlist);
  }
}

void cMediaPlayerMenu::PlayMenu(void)
{
  Clear();

  for (int i = 0; i <= (pPlayer->LastFile); i++) {
    string NewString = pPlayer->MediaFiles[i].Folder
      + " - " + pPlayer->MediaFiles[i].File;
    Add(new cOsdItem(NewString.c_str()), (i == pPlayer->CurrentFile));
  }

  if (pControl->MenuOpen < 2)
    SetHelp(pPlayer->RandomPlay ? "Random Play" : " No Random Play",
            "Jump -30s",
            "Jump +30s",
            ">>");
  else SetHelp("<<", "Add to PL", "Del from PL", "Close");

  Display();
  // das sollte in ProcessKey gesetzt werden !!!
  if (!pControl->MenuOpen)
    pControl->MenuOpen = 1;
}

eOSState cMediaPlayerMenu::ProcessKey(eKeys Key)
{
  eOSState state = osContinue;
  cOsdItem *item;

  item = (cOsdItem *) Get(Current());
  state = cOsdMenu::ProcessKey(Key);

  switch (Key) {
    case kOk:
      if (MenuSelPL) {
        Playlist.assign(cPlugin::ConfigDirectory("mediaplayer"));
        Playlist.append("/");
        Playlist.append(item->Text());
        MenuSelPL = 0;
        FindFile(Path, true, NULL);
      } else if (pControl->MenuOpen) {
        pPlayer->Play(Current());
        Interface->Confirm(tr("Play new file"), 1, true);
      } else {
        if (strcasestr(item->Text(), "[..]")) {
          string NewPathString = Path.substr(0,Path.find_last_of("/"));
          Path = NewPathString;
          FindFile(Path.c_str(), true, NULL);
        } else {
          if (strcasestr(item->Text(), ".MP3")) {
            if (!cMediaPlayer::Running) {
              MakePlayList(item->Text(), "w");
              cControl::Launch(pControl = new cMediaPlayerControl(Playlist.c_str()));
              pPlayer = cMediaPlayer::Player();
            } else {
              MakePlayList(item->Text(), "a");
              pPlayer->ReadPL();
            }
            PlayMenu();
          } else { // Fixme! Control this is a folder!!!
            string NewPathString = Path + "/" + item->Text();
            Path = NewPathString;
            FindFile(NewPathString.c_str(), true, NULL);
          }
        }
      }
      break;

    case kRed:
      if (MenuSelPL) {
        Playlist.assign(cPlugin::ConfigDirectory("mediaplayer"));
        Playlist.append("/");
        Playlist.append(item->Text());
        MenuSelPL = 0;
        FindFile(Path, true, NULL);
      } else if (pControl->MenuOpen == 2) {
        pControl->MenuOpen = 1;
        PlayMenu();
      } else if (pControl->MenuOpen == 1) {
        pPlayer->RandomPlay ^= 1;
        PlayMenu();
      } else if (!cMediaPlayer::Running) {
        MakePlayList(item->Text(), "w");
        Interface->Confirm(tr("Added to new Playlist"), 1, true);
      }
      break;

    case kGreen:
      if (MenuSelPL)
        break;
      if (pControl->MenuOpen == 2) {
        Path = cPluginMediaplayer::GetStartPath();
        FindFile(Path, true, NULL);
      } else if (pControl->MenuOpen == 1) {
        pPlayer->Jump = -30;
      } else {
        MakePlayList(item->Text(), "a");
        Interface->Confirm(tr("Added to Playlist"), 1, true);
        if (cMediaPlayer::Running)
          pPlayer->ReadPL();
      }
      break;

    case kYellow:
      if (MenuSelPL)
        break;
      if (pControl->MenuOpen == 2) {
        pPlayer->DelFromPL(Current());
        break;
      } else if (pControl->MenuOpen == 1) {
        pPlayer->Jump = 30;
        break;
      }
    case kPlay:
      if (MenuSelPL) {
        Playlist.assign(cPlugin::ConfigDirectory("mediaplayer"));
        Playlist.append("/");
        Playlist.append(item->Text());
        MenuSelPL = 0;
      }
      if(!ifstream(Playlist.c_str())) {
        Interface->Confirm(tr("Thers is no Playlist! Please open or make one!"), 1, true);
      } else if (!cMediaPlayer::Running) { // test is there a playlist with files!!!
        cControl::Launch(pControl = new cMediaPlayerControl(Playlist.c_str()));
        pPlayer = cMediaPlayer::Player();
        PlayMenu();
      }
      break;

    case kBlue:
      if (MenuSelPL)
        break;
      if (pControl->MenuOpen == 2) {
        state = osStopReplay;
      } else if (pControl->MenuOpen == 1) {
        pControl->MenuOpen = 2;
        PlayMenu();
      } else if (cMediaPlayer::Running) {
        PlayMenu();
      } else if (!cMediaPlayer::Running) {
        SelectPL();
      }
      break;
    default:
      break;
  }
  return state;
}
