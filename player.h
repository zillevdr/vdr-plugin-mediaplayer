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

class cMediaPlayer : public cPlayer, cThread {
friend class cMediaPlayerMenu;
friend class cMediaPlayerControl;
private:
  struct MediaFile {
    string Path;
    string File;
    string Folder;
    string SubFolder;
  };
  struct MediaFile MediaFiles[5000];
  int CurrentFile;
  int LastFile;
  int StopFile;
  int RandomPlay;
  int ThisFile;
  int PausePlay;
  int StopPlay;
  int Jump;
  char *Playlist;
  void Previous(void);
  void Next(void);
  void Play(int index);
  void ReadPL(void);
  void PlayFile(const char *path);
  void DelFromPL(int title);
  static cMediaPlayer *pPlayer;
protected:
  virtual void Activate(bool On);
  virtual void Action(void);
public:
  static volatile int Running;
  cMediaPlayer(const char *Playlist);
  virtual ~cMediaPlayer();
  static cMediaPlayer *Player() { return pPlayer; }
  char *GetPlaylist() { return Playlist; }
};
