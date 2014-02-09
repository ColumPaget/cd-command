
#ifndef CD_COMMAND_H
#define CD_COMMAND_H

#include "libUseful-2.0/libUseful.h"

//some values from these are needed in multiple places
//so best they're included in 'common.h'
#include <linux/cdrom.h>
#include <linux/soundcard.h>


#define PROG_NAME "CdCommand"
#define PROG_VERSION "0.0.2"

#define TAGFIELD_LEN 30
#define NO_OF_CACHE_BUCKETS 4096



typedef struct
{
unsigned int TrackNo;
char *Title;
char *Artist;
char *Album;

//These all relate to the start position
unsigned int Mins;
unsigned int Secs;
unsigned int Frames;
unsigned int FrameOffset; //Mins/Seconds/Frames converted into just frames

//Length of track in frames
unsigned int NoOfFrames;
} TTrack;


#endif
