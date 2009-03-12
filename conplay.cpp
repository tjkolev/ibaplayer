/***************************************************************************
 *   Copyright (C) 2004 by TJ Kolev                                        *
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
    if(NULL == player) return;
    
    player->getLibMngr().getLibrary().load();
    MusicLibBrowser& br = player->getLibMngr().getBrowser();
    br.init();
    
    AlsaPlayerCntr ap = player->getPlayer();
    ap.init(player->getConfig(), player->getLog());
    
    cout << "+:Next, -:Previous, u:levelUp, d:levelDown\ng:Genre, r:Artist, a:Album, t:Track\ns:Select\n";
    cout << "Selection: " << br.getLvlTag() << '\n';
    
    bool isBrowsing = true;
    
    //player->getLog().log("Starting console play.", IBALogger::LOGS_INFO);
    
    char ch = ' ';
    while(ch != 'q')
    {
        cout << "Command: ";
        cin >> ch;

        if(isBrowsing)
        {
            switch(ch)
            {
                case 'x': cout << "Play mode"; isBrowsing = false; break;
                
                case '+': cout << br.next(); break;
                case '-': cout << br.prev(); break;
                case 'u': cout << br.lvlUp(); break;
                case 'd': cout << br.lvlDn(); break;
                
                case 'g': cout << br.setStartLevel(br.BR_LEVEL_GENRE); break;
                case 'r': cout << br.setStartLevel(br.BR_LEVEL_ARTIST); break;
                case 'a': cout << br.setStartLevel(br.BR_LEVEL_ALBUM); break;
                case 't': cout << br.setStartLevel(br.BR_LEVEL_TRACK); break;
                
                case 's':
                case 'p':
                {
                    const playlist_t playlist = br.select();
                    if(ch == 's')
                    {    
                        ap.add(playlist);
                    }
                    else //if(ch == 'p')
                    {
                        ap.clear();
                        ap.add(playlist);
                        ap.setLoop(true);
                        ap.next();
                    }
                    
                    break;
                } 
                
                case 'f':
                    const playlist_t playlist = br.getDefaultPlaylist();
                    ap.clear();
                    ap.add(playlist);
                    ap.next();
                
            }
        }
        else
        {
            switch(ch)
            {
                case 'x': cout << "Browse mode"; isBrowsing = true; break;
                
                case 'p': ap.play(); break;
                case 's': ap.stop(); break;
                case '+': ap.next(); break;
                case '-': ap.previous(); break;
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
        //selected(br);
    }
}
