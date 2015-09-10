import pymedia.audio.acodec as acodec
import pymedia.muxer as muxer
import os, wave, eyed3

tags = ['album', 'album_artist', 'album_type', 'artist',
        'best_release_date', 'bpm', 'comments', 'disc_num', 'genre',
        'images', 'lyrics', 'objects', 'publisher', 'recording_date',
        'release_date', 'title', 'track_num', 'version']

def getMP3tags(name): # STILL WORKING ON THIS, NOT DONE
    song = eyed3.load(name)
    tag_dict = {}
    for tag in tags:
        exec('tag_dict[tag] = song.tag.' + tag)
    print tag_dict

    img = open(name[:-3]+'jpg', 'wb')
    img.write(song.tag.images[0].image_data)
    img.close()
    
def MP3toWAV(name):
    if name[-3:] != 'mp3':
        return
    
    dm = muxer.Demuxer('mp3')
    
    f = open(name, 'rb')
    s = 1
    fw = None
    dec = None
    while s:
        s = f.read(20000)
        if s:
            frames = dm.parse(s)
            for frame in frames:
                if not dec:
                    dec = acodec.Decoder(dm.streams[0])

                r = dec.decode(frame[1])
                if r and r.data:
                    if not fw:
                        fw = wave.open(name[:-3] + 'wav', 'wb')
                        fw.setparams((r.channels, 2, r.sample_rate, 0, 'NONE',''))
                    fw.writeframes(r.data)


folder = 'D:/music/Metro Zu/'
songs = os.listdir(folder)

for song in songs[0:1]:
    MP3toWAV(folder+song)
    
