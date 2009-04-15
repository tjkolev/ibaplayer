/***************************************************************************
 *   Copyright (C) 2009 by TJ Kolev                                        *
 *   tjkolev@yahoo.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "IBAPlayer.h"

void consolePlay(IBAPlayer* player)
{
    if(NULL == player)
		return;

    AlsaPlayerCntr& ap = player->getPlayer();
	if(!ap.isApRunning())
	{
		cout << "Alsa player does not seem to be running.\n";
		return;
	}

    MusicLibBrowser br;
    br.Init(ap);
    br.LoadPlayQueue();

    cout << "+:Next, -:Previous, u:PageUp, d:PageDown\nm:Menu, s:Select, p:Play, a:Add\n";

    bool isBrowsing = true;

    Log("Starting console play.", IBALogger::LOGS_INFO);

	ap.setLoop(true);

    char ch = ' ';
    while(ch != 'q')
    {
        cout << "Command: ";
        cin >> ch;

        if(isBrowsing)
        {
            switch(ch)
            {
                case '+': cout << br.Next(); break;
                case '-': cout << br.Prev(); break;
				case 'u': cout << br.PgUp(); break;
				case 'd': cout << br.PgDn(); break;

                case 'm': cout << br.Menu(); break;
                case 's': cout << br.Select(); break;
                case 'p': cout << br.Play(); PrintList(br.PlayQueue()); break;
                case 'a': cout << br.Add(); break;
                case '~': br.Shuffle(); PrintList(br.PlayQueue()); break;
                case '@': br.RandomPick(); PrintList(br.PlayQueue()); break;

                case '>': ap.next(); break;
                case '<': ap.previous(); break;

                case 'i': cout << ap.getInfo(); break;
                case 't': cout << ap.getTimeInfo(); break;
                case 'r': cout << ap.getMiscInfo(); break;
                    /*
                    char str_type[32];
                    ap_get_stream_type(0, str_type);
                    cout << "Stream type: " << str_type << endl;
                    //ap_get_status(0, str_type);
                    //cout << "Status: " << str_type << endl;
                    int num;
                    //ap_get_tracks(0, &num);
                    //cout << "Tracks: " << num << endl;
                    ap_get_playlist_length(0, &num);
                    cout << "Playlist length: " << num << endl;
                    ap_get_playlist_position(0, &num);
                    cout << "Playlist position: " << num << endl;
                    // MP3 44KHz 160kbit 11:23
                    // 0004/5006 160k 11:23

                    break;*/
            }
        }

        cout << '\n';

        if(ch == 'q')
        {
            ap.stop();
            return;
        }
    }
}
