/*
  Future Composer & Hippel audio decoder plug-in
  for the DeaDBeeF music player

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Local plugins directory:
  cp .libs/fcdec.so ~/.local/lib64/deadbeef
*/

#include <string.h>
#include <stdlib.h>
#include <deadbeef/deadbeef.h>
#include <fc14audiodecoder.h>

extern DB_decoder_t fcdec_plugin;
DB_functions_t *deadbeef;

const char *fcdec_exts[] = { "fc", "fc13", "fc14", "fc3", "fc4", "smod", "hip", "hipc", "hip7", "mcmd", NULL };

static const char settings_dlg[] =
    "property \"Sample rate [Hz]\" select[3] fcdec.samplerate 1 48000 44100 22050;\n"
    "property \"Panning\" spinbtn[0,100,1] fcdec.panning 75;\n"
    "property \"Min.duration [sec]\" entry fcdec.minduration 10;\n"
;

typedef struct {
    DB_fileinfo_t info;
    DB_FILE *file;
    void* decoder;
    int subsong;
    int duration;
} fcdec_info_t;

DB_fileinfo_t * fcdec_open (uint32_t hints) {
    fcdec_info_t *info = (fcdec_info_t *)malloc (sizeof (fcdec_info_t));
    DB_fileinfo_t *_info = (DB_fileinfo_t *)info;
    memset (info, 0, sizeof (fcdec_info_t));
    return _info;
}


int fcdec_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    /* return -1 on failure */
    fcdec_info_t *info = (fcdec_info_t*)_info;

    info->decoder = fc14dec_new();
    if (!info->decoder) {
        return -1;
    }

    deadbeef->pl_lock();
    char *uri = strdup( deadbeef->pl_find_meta(it, ":URI") );
    deadbeef->pl_unlock();
    info->file = deadbeef->fopen(uri);
    free(uri);
    if (!info->file) {
        return -1;
    }

    int samplerates[3] = { 48000, 44100, 22050 };
    int samplerate = samplerates[deadbeef->conf_get_int ("fcdec.samplerate", 1)];
    int bits = 16;
    int channels = 2;
    int panning = deadbeef->conf_get_int ("fcdec.panning", 50+25);

    /* We ignore short tracks when populating the playlist, but
       the preferences can be changed, so we toggle "End Shorts" mode
       here properly. */
    int mindur = deadbeef->conf_get_int ("fcdec.minduration", 10);
    if (mindur != 0) {
        fc14dec_end_shorts(info->decoder,true,mindur);
    }
    else {
        fc14dec_end_shorts(info->decoder,false,mindur);
    }
    
    info->subsong = deadbeef->pl_find_meta_int (it, ":TRACKNUM", 1);
    info->duration = deadbeef->pl_get_item_duration (it);
    
    int64_t len = deadbeef->fgetlength(info->file);
    char* buf = malloc(len);
    if (!buf) {
        fc14dec_delete(info->decoder);
        return -1;
    }
    size_t read = deadbeef->fread(buf,len,1,info->file);
    deadbeef->fclose(info->file);
    int haveModule = fc14dec_init(info->decoder,buf,len,info->subsong);
    free(buf);

    fc14dec_mixer_init(info->decoder,samplerate,bits,channels,0,panning);
    
    _info->plugin = &fcdec_plugin;
    _info->fmt.bps = bits;
    _info->fmt.channels = channels;
    _info->fmt.samplerate = samplerate;
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);
    _info->readpos = 0;

    return 0;
}

void fcdec_free (DB_fileinfo_t *_info) {
    fcdec_info_t *info = (fcdec_info_t *)_info;

    fc14dec_delete(info->decoder);
}

int fcdec_read (DB_fileinfo_t *_info, char *bytes, int size) {
    fcdec_info_t *info = (fcdec_info_t *)_info;

    fc14dec_buffer_fill(info->decoder,bytes,size);
    if (fc14dec_song_end(info->decoder)) {
        return 0;
    }

    int samplesize = (_info->fmt.bps>>3) * _info->fmt.channels;
    _info->readpos += size / samplesize / (float)_info->fmt.samplerate;
    return size;
}

int fcdec_seek (DB_fileinfo_t *_info, float time) {
    fcdec_info_t *info = (fcdec_info_t *)_info;
    fc14dec_seek(info->decoder,time*1000);  /* seconds */
    
    _info->readpos = time;
    return 0;
}

DB_playItem_t* fcdec_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    DB_FILE *file = deadbeef->fopen(fname);
    if ( !file ) {
        return after;
    }
    int64_t len = deadbeef->fgetlength(file);
    char* buf = malloc(len);
    if (!buf) {
        deadbeef->fclose(file);
        return after;
    }
    size_t read = deadbeef->fread(buf,len,1,file);
    deadbeef->fclose(file);
    
    void *decoder = nullptr;
    decoder = fc14dec_new();
    if (!decoder) {
        free(buf);
        return after;
    }
    int haveModule = fc14dec_init(decoder,buf,len,0);
    free(buf);

    if (haveModule) {
        int songs = fc14dec_songs(decoder);
        for (int s=0; s<songs; s++) {
            DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, fcdec_plugin.plugin.id);
            deadbeef->pl_set_meta_int (it, ":TRACKNUM", s);  /* not +1 */
            char trk[10];
            snprintf (trk, 10, "%d", s+1);
            deadbeef->pl_add_meta (it, "track", trk);

            int good = fc14dec_reinit(decoder,s);
            if (good) {
                uint32_t dur = fc14dec_duration(decoder)/1000;
                deadbeef->plt_set_item_duration (plt, it, dur);
                deadbeef->pl_add_meta (it, ":FILETYPE", fc14dec_format_id(decoder));
                /* ignore short songs as configured */
                int mindur = deadbeef->conf_get_int ("fcdec.minduration", 10);
                if (dur >= mindur) {
                    after = deadbeef->plt_insert_item (plt, after, it);
                }
            }
            deadbeef->pl_item_unref (it);
        }
    }
    fc14dec_delete(decoder);
    return after;
}

int fcdec_start (void) {
    return 0;
}

int fcdec_stop (void) {
    return 0;
}

DB_plugin_t* fcdec_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&fcdec_plugin);
}


DB_decoder_t fcdec_plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "fcdec",
    .plugin.name = "FC & Hippel player",
    .plugin.descr = "Future Composer (AMIGA) player\n"
    "TFMX/Hippel (AMIGA) player\n\n"
    "File name extensions:\n"
    ".fc, .fc13, .fc14, .fc3, .fc4, .smod\n"
    ".hip, .hipc, .hip7, .mcmd\n",
    .plugin.copyright = "Created by Michael Schwendt\n\n"
    "License: GPLv2 or later\n",
    .plugin.website = "https://github.com/mschwendt/deadbeef-plugins-fc",
    .plugin.start = fcdec_start,
    .plugin.stop = fcdec_stop,
    .plugin.configdialog = settings_dlg,
    .open = fcdec_open,
    .init = fcdec_init,
    .free = fcdec_free,
    .read = fcdec_read,
    .seek = fcdec_seek,
    .seek_sample = NULL,
    .insert = fcdec_insert,
    .exts = fcdec_exts,
};
