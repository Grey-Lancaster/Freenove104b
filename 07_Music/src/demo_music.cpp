#include "demo_music.h"
#include "olive_mp3.h"

char * title_list[99];

char * songs[99] ;
bool read_flag = false;
int song_num = 0;
static bool using_spiffs_fallback = false;

// Writes the embedded Olive.mp3 to SPIFFS on first boot (or after SPIFFS gets
// reformatted) so it can be played the same way as any SD-card file, via
// audio.connecttoFS(). Skips the write if it's already there.
static bool ensure_fallback_on_spiffs(void)
{
    if (!SPIFFS.begin(true)) // true = format on mount failure
    {
        Serial.println("SPIFFS mount failed");
        return false;
    }
    if (SPIFFS.exists("/olive.mp3"))
    {
        return true;
    }
    File f = SPIFFS.open("/olive.mp3", FILE_WRITE);
    if (!f)
    {
        Serial.println("Failed to open /olive.mp3 on SPIFFS for writing");
        return false;
    }
    size_t written = f.write(olive_mp3, olive_mp3_len);
    f.close();
    if (written != olive_mp3_len)
    {
        Serial.println("Failed to fully write embedded fallback track to SPIFFS");
        SPIFFS.remove("/olive.mp3");
        return false;
    }
    Serial.println("Wrote embedded fallback track to SPIFFS: /olive.mp3");
    return true;
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    int i=0;
    while(file)
    {
        if(file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels)
            {
                listDir(fs, file.path(), levels -1);
            }
        } 
        else 
        {
            char *filepath = (char *)file.path();  // Use file.path() instead of file.name()
            Serial.println(filepath);
            int8_t len = strlen(filepath);
            if (strstr(strlwr(filepath + (len - 4)), ".mp3"))
            {
                songs[i] = new char[len + 1]; // Add 1 for null terminator
                read_flag = true;
                strcpy(songs[i], filepath);
                Serial.print("out:");
                Serial.println(filepath);
                Serial.print("songs:");
                Serial.println(songs[i]);
                i+=1;
            }
        }
        file = root.openNextFile();
    }
    song_num = i;
}

bool demo_music(void)
{
    listDir(SD_MMC,"/",1);
    if (read_flag)
    {
        memcpy(title_list,songs,sizeof(songs));
        return true;
    } 
    else
    {
        Serial.println("not find MP3 file");
        if (ensure_fallback_on_spiffs())
        {
            Serial.println("Falling back to embedded track (SPIFFS:/olive.mp3)");
            songs[0] = new char[11]; // "/olive.mp3" (10 chars) + null terminator
            strcpy(songs[0], "/olive.mp3");
            song_num = 1;
            using_spiffs_fallback = true;
            memcpy(title_list, songs, sizeof(songs));
            return true;
        }
        return false;
    }
}

const char * demo_music_get_title(uint32_t track_id)
{
    if(track_id >= sizeof(title_list) / sizeof(title_list[0])) 
    {
        return NULL;
    }
    return title_list[track_id];
}

void demo_music_play(uint32_t track_id) 
{
    char chbuf[100];
    const char *filename = demo_music_get_title(track_id);
    Serial.println(String(filename));
    sprintf(chbuf, "%s", filename);
    audio.pauseResume();
    delay(100);
    if (using_spiffs_fallback)
    {
        audio.connecttoFS(SPIFFS, chbuf);
    }
    else
    {
        audio.connecttoFS(SD_MMC, chbuf);
    }
}

void demo_music_pause(void)
{
    audio.pauseResume();
}
