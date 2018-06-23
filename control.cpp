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
using std::string;

#include <vdr/player.h>
#include <vdr/interface.h>

#include "player.h"
#include "control.h"
#include "menu.h"

volatile int cMediaPlayerControl::MenuOpen = 0;
cMediaPlayerControl *cMediaPlayerControl::pControl = NULL;

cMediaPlayerControl::cMediaPlayerControl(const char *Playlist)
:cControl(pPlayer = new cMediaPlayer(Playlist))
{
  pControl = this;
  Close = 0;
}

cMediaPlayerControl::~cMediaPlayerControl()
{
  if (pPlayer) {
    pPlayer->StopPlay = 1;
    delete pPlayer;
  }
}

void cMediaPlayerControl::Hide(void)
{
//  fprintf(stderr, "[cMediaPlayerControl] Hide\n");
}

eOSState cMediaPlayerControl::ProcessKey(eKeys Key)
{

  switch (Key) {
    case kNone:
      if (Close)
        return osStopReplay;
      break;

    case kPlay:
      if (pPlayer->PausePlay == 1)
        pPlayer->PausePlay = 0;
      break;

    case kBlue:
      return osEnd;

    case kPause:
      pPlayer->PausePlay ^= 1;
      break;

    case kNext:
      pPlayer->Next();
      break;

    case kPrev:
      pPlayer->Previous();
      break;
    default:
      break;
  }
  return osContinue;
}
