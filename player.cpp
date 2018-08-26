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

#include <fstream>
#include <string>
#include <cstdlib>

using std::ifstream;
using std::string;

#include <vdr/player.h>
#include <vdr/plugin.h>

#include "player.h"
#include "control.h"
#include "menu.h"
#include "setupmenu.h"
#include "mediaplayer.h"

extern "C"
{
#include <libavformat/avformat.h>
}

volatile int cMediaPlayer::Running = 0;
cMediaPlayer *cMediaPlayer::pPlayer = NULL;

cMediaPlayer::cMediaPlayer(const char *PL)
:cPlayer (pmAudioOnly)
{
  pPlayer= this;
  Playlist = (char *) malloc(1 + strlen(PL));
  strcpy(Playlist, PL);
  DeviceClear();
  ReadPL();
  StopPlay = 0;
  PausePlay = 0;
  ThisFile = 0;
  RandomPlay = 0;
}

cMediaPlayer::~cMediaPlayer(void)
{
  Running = 0;
  StopFile = 1;
  StopPlay = 1;
  free(Playlist);
}

void cMediaPlayer::Activate(bool On)
{
  if (On)
    Start();
}

void cMediaPlayer::ReadPL(void)
{
  ifstream f;
  int i = 0;

  f.open(Playlist);
  if (!f.good())
    esyslog("Mediaplayer: open PL %s failed\n", Playlist);

  while (!f.eof() && i < 5000) {
    string s;
    getline(f, s);
    if (s.size() && s.compare(1, 1, "/")) {
      MediaFiles[i].Path = s;

      MediaFiles[i].File = MediaFiles[i].Path.substr(MediaFiles[i].Path.find_last_of("/")+1,
        string::npos);

      string SubString = MediaFiles[i].Path.substr(0, MediaFiles[i].Path.find_last_of("/"));
      MediaFiles[i].SubFolder = SubString.substr(SubString.find_last_of("/")+1, string::npos);

      string FolderString = MediaFiles[i].Path.substr(0, SubString.find_last_of("/"));
      MediaFiles[i].Folder = FolderString.substr(FolderString.find_last_of("/")+1, string::npos);

      i++;
    }
  }

  LastFile = i - 1;
  CurrentFile = 0;
  f.close();
}

void cMediaPlayer::DelFromPL(int title)
{
  LastFile--;
  while (LastFile >= title) {
    MediaFiles[title] = MediaFiles[title + 1];
    title++;
  }

  if (cMediaPlayerControl::Control()->MenuOpen)
    cMediaPlayerMenu::Menu()->PlayMenu();

  // Write new PL
  FILE *playlist = fopen(Playlist, "w");
  for (int i = 0; i <= LastFile; i++) {
    fprintf(playlist, "%s\n", MediaFiles[i].Path.c_str());
  }
  fclose (playlist);
}

void cMediaPlayer::PlayFile(const char *path)
{
  struct buf {
    unsigned char *buff;
    size_t size;
  };
  int max_buffers = 500;
  int next_buffer = 0;
  struct buf bufs[max_buffers];
  int err = 0;
  cPoller oPoller;

  static const unsigned char header[] = {
    0x00, // byte 0: PES header
    0x00, // byte 1
    0x01, // byte 2
    0xC0, // byte 3: Stream ID: one MPEG1 or MPEG2 audio stream
    0x00, // byte 4: PES packet length
    0x00, // byte 5
    0x87, // byte 6: mpeg2, aligned, copyright, original
    0x00, // byte 7: no pts/dts
    0x00, // byte 8: PES header length after this field, 0 can be only if video stream
    0x00, // byte 9 
    0xFF, // byte 10
    0x00, // byte 11
    0x00, // byte 12
    0x00, // byte 13
    0x01, // byte 14
    0x80  // byte 15
  };

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58,18,100)
  av_register_all();
#endif
  // get format from audio file
  AVFormatContext* format = avformat_alloc_context();
  if (avformat_open_input(&format, path, NULL, NULL) != 0) {
    esyslog("Mediaplayer: Could not open file '%s'\n", path);
    return;
  }

  if (avformat_find_stream_info(format, NULL) < 0) {
    esyslog("Mediaplayer: Could not retrieve stream info from file '%s'\n", path);
    return;
  }

  AVPacket packet;
  av_init_packet(&packet);

  int secs  = format->duration / AV_TIME_BASE;
  int us    = format->duration % AV_TIME_BASE;
  int mins  = secs / 60;
  secs %= 60;
  int hours = mins / 60;
  mins %= 60;
  esyslog("Mediaplayer: Play %s codec: %s duration %02d:%02d:%02d.%02d\n",
    format->url, format->iformat->name, hours, mins, secs,
                      (100 * us) / AV_TIME_BASE);

  while (next_buffer != max_buffers + 1) {
    bufs[next_buffer].buff = NULL;
    next_buffer++;
  }
  next_buffer = 0;

  // iterate through frames
  while (err == 0 && !StopFile) {
    err = av_read_frame(format, &packet);
    if (err == 0) {
      // make header + data
      if (!bufs[next_buffer].buff) {
        bufs[next_buffer].buff = (unsigned char *) malloc(sizeof(header) + packet.size);
      }

      memcpy(bufs[next_buffer].buff, header, sizeof(header));
      memcpy(bufs[next_buffer].buff + sizeof(header), packet.data, packet.size);
      bufs[next_buffer].size = packet.size + 16;
      bufs[next_buffer].buff[4] = ((bufs[next_buffer].size - 6) >> 8) & 0xFF;
      bufs[next_buffer].buff[5] = (bufs[next_buffer].size - 6) & 0xFF;
      bufs[next_buffer].buff[8] = 7; // pes_header_data_length

repeat:
      DevicePoll(oPoller, 100);
      if (PlayPes(bufs[next_buffer].buff, bufs[next_buffer].size, false) <= 0) {
        // vorher sollte ein sleep von einem Frame Dauer eingebaut werden!!!!
        usleep(packet.duration * AV_TIME_BASE * format->streams[0]->time_base.num 
          / format->streams[0]->time_base.den);
        goto repeat;
      }

      if (next_buffer == max_buffers)
        next_buffer = 0;
      else next_buffer++;

      if (PausePlay) {
        DeviceClear();
        Jump = cPluginMediaplayer::BufferOffset;
        while (PausePlay) {
          sleep(1);
        }
      }

      if (Jump) {
        av_seek_frame(format, format->streams[0]->index,
          packet.pts + (int64_t)((Jump - cPluginMediaplayer::BufferOffset) *
          format->streams[0]->time_base.den / format->streams[0]->time_base.num),
          AVSEEK_FLAG_ANY);
        DeviceClear();
        Jump = 0;
      }
    }
  }

  if (StopFile)
    DeviceClear();

  next_buffer = max_buffers;
  while (next_buffer > -1) {
    free(bufs[next_buffer].buff);
    next_buffer--;
  }
  next_buffer = 0;

  av_packet_unref(&packet);
  avformat_close_input(&format);
  avformat_free_context(format);
}

void cMediaPlayer::Play(int index)
{
  CurrentFile = index;
  ThisFile = 1;
  StopFile = 1;
}

void cMediaPlayer::Next(void)
{
  StopFile = 1;
}

void cMediaPlayer::Previous(void)
{
  CurrentFile--;
  StopFile = 1;
}

void cMediaPlayer::Action(void)
{
  Running = 1;
  DevicePlay();

  while (CurrentFile <= LastFile && !StopPlay) {
    StopFile = 0;

    if (cMediaPlayerControl::Control()->MenuOpen)
      cMediaPlayerMenu::Menu()->PlayMenu();

    PlayFile(MediaFiles[CurrentFile].Path.c_str());

    if (ThisFile) {
      ThisFile = 0;
      continue;
    }
    if (RandomPlay) {
      srand (time (NULL));
      CurrentFile = std::rand() % (LastFile + 1);
    } else CurrentFile++;
  }
  sleep (10);
  cMediaPlayerControl::Control()->Close = 1;
}
