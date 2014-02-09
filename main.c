#include "common.h"
#include "cddb.h"


typedef struct
{
int Flags;
char *CdDevice;
char *Tracks;
char *OutputFormat;
char *OutputPath;
char *RipCommand;
char *EncodeCommand;
char *CDDBServer;
char *InfoFile;
int Speed;
} TArgs;

typedef struct
{
int Type;
char *MediaType;
char *CommandLine;
} THelper;

typedef struct
{
char TAG[3];
char Title[30];
char Artist[30];
char Album[30];
char Year[4];
char Comment[30];
char Genre;
}TID3_TAG;


#define PT_ENCODER 1
#define PT_RIPPER 2

#define TRACK_CURR -1
#define TRACK_NEXT -2
#define TRACK_PREV -3
#define TRACK_RW -4
#define TRACK_FF -5
#define TRACK_LOOP -6
#define TRACK_QUIT -7
#define TRACK_EJECT -10
#define PRINT_USAGE -11
#define PRINT_TRACKS -12

#define ERR_OPEN_INPUT -999
#define ERR_OPEN_OUTPUT -998


#define FLAG_OVERLAP_PROGRAMS 1
#define FLAG_LEAVEWAVE 2
#define FLAG_EJECT 4
#define FLAG_TEST 8
#define FLAG_HELP 16
#define FLAG_VERSION 32
#define FLAG_PLAY 64
#define FLAG_LOOP 128
#define FLAG_SLAVE 256
#define FLAG_NOCDDB 512
#define FLAG_BACKGROUND 1024
#define FLAG_CONTINUING 2048


STREAM *StdIn=NULL;
char *CmdLine;
int EncodePid=0, OutBuffSize=CD_FRAMESIZE_RAW * 60;
ListNode *Tracks, *Vars, *Helpers;
char *ProgName=NULL;

//ReadInput callback is used to handle input. Depending on how the 
//program is being used, it might read keystrokes or strings
typedef int (*READ_INPUT_FUNC) (STREAM *InputS);
READ_INPUT_FUNC ReadInput=NULL;



void PrintUsage()
{
printf("\n%s: version %s\n",PROG_NAME,PROG_VERSION);
printf("Author: Colum Paget\n");
printf("Email: colums.projects@gmail.com\n");
printf("Blogs: \n");
printf("  tech: http://idratherhack.blogspot.com \n");
printf("  rants: http://thesingularitysucks.blogspot.com \n");
printf("\n\ncd-record [options] [cd-device-path] [trackno] [trackno] ...\n\n");
printf("cd-device-path is of the form /dev/scd0 and is the path to the input device. 'trackno' is any track number, if none are given all tracks are played/ripped.\nOptions are:\n");
printf("	'-r'		rip tracks rather than playing\n");
printf("	'-b'		fork into background\n");
printf("	'-e'		eject cdrom when finished (combine with -t to eject without playing)\n");
printf("	'-t'		test mode, don't rip\n");
printf("	'-x'		Overlap encoding and ripping next track\n");
printf("	'-cddb <server> CDDB Server (default freedb.freedb.org:8880)\n");
printf("			'none' to disable.\n");
printf("	'-i <path>'	Track information file\n");
printf("	'-f <format>'	Output format for ripping (wav, mp3, ogg, flac, etc).\n");
printf("			Dependant on available encoders\n");
printf("	'-o <path>'	Output path for ripping.\n");
printf("	'-?'		print help\n");
printf("	'-help'		print help\n");
printf("	'--help'	print help\n");
printf("	'-v'		print version\n");
printf("	'-version'	print version\n");
printf("	'--version'	print version\n");
printf("\nKeys: Typing '?' while program is running will give a list of keypress operations\n");
printf("\nTrack Info:\nNormally track info is found from cddb (freedb.org) but using the -i flag an information file can be provided that contains the tracklisting. Format of this file is:\n");
printf("  1st Line: Artist\n");
printf("  2nd Line: Album\n");
printf("  3rd Line: 1st Song Name\n");
printf("  4th Line: 2nd Song Name\n");
printf("  etc.. etc.. etc.\n");
printf("\nOutput Path:\nThe output path can either be a device for playing or a filename for ripping. Devices have 'dev:' prepended to the front, for example:\n\n");
printf("	cd-command -o dev:/dev/dsp\n");
printf("	cd-command -o dev:/dev/audio\n");
printf("\nFor ripping the output path can contain values like $(Album) $(Artist) $(TrackNo) and $(Track), for example: \n\n");
printf("	cd-command -r -o /home/MyMusic/$(Artist)-($Album)/$(TrackNo)-$(Track)\n");
printf("\ncd-command will append '.wav' or '.mp3' automatically to the final output path, using the value given by the -f flag.\n");
printf("\nOutput Format:\nThis matches to helper programs that encode the output. cd-command has a default set of helpers (lame, gogo, oggenc) but others can be setup in the config file. Examples:\n\n");
printf("	cd-command -r -f mp3 /tmp/$(Track)\n");
printf("	cd-command -r -f ogg /tmp/$(TrackNo)-$(Track)\n");
printf("\nConfigFile:\ncd-command reads settings from /etc/cd-command.conf or from ~/.cd-command.conf (where ~ is the user's home directory). Example file format is:\n");
printf("\nEncoder=mp3,gogo -m j '$(Output)' '$(EncPath)'\n");
printf("Encoder=ogg,oggenc '$(Output)' -o '$(EncPath)'\n");
printf("Ripper=cdparanoia -d $(CDDev) $(TrackNo) -w '$(Output)'\n");
printf("EjectWhenDone=Y\n");
printf("Overlap=Y\n");
printf("CDDBServer=freedb.freedb.org\n");
printf("CDDevice=/dev/hcd0\n\n");
printf("Where:\n");
printf("'Encoder' sets programs to be used for encoding into audio formats. Note that the first argument of the 'Encoder' value is the 'Output Format' as set with the -f flag. The variable $(Output) can be used to pass the ripped .wav file that is to be encoded. $(EncPath) is the path of the file AFTER encoding. If no encoder is given or -f wav is used, then cd-command will just rip to .wav files.\n");
printf("'Ripper' sets programs to be used for ripping from CD (cd-command can do this itself, but will use a ripper program like cdparanoia is available).\n");
printf("'EjectWhenDone' eject cd when finished playing/ripping\n");
printf("'Overlap' overlap ripping/encoding\n");
printf("'CDDBServer' set cddb server to use. Setting this blank disables lookups.\n");
printf("'CDDevice' path to cd device file\n");
printf("\nCDDB Caching:\ncd-command will cache data from CDDB lookups, allowing it to 'remember' the identity of a CD even when CDDB servers cannot be reached. The data is stored in a directory in the user's home directory called '.cd-util'. However, if a directory called '/var/cache/cddb/' is available, the data will instead be cached in that, allowing a cache to be shared between all users on a system.\n");
}



void PrintKeys()
{
printf("\rKeys:                     \n");
printf("  TAB:		Toggle CD loop\n");
printf("  SPACE:	Pause\n");
printf("  ENTER:	Pause\n");
printf("  i:		Tracks Info\n");
printf("  ?:		Keys Help\n");
printf("  1-(n):		Goto track\n");
printf("  +:		Prev track\n");
printf("  -:		Next track\n");
printf("  =:		Next track\n");
printf("  , or <:	Prev track\n");
printf("  . or >:	Next track\n");
printf("  Cursor Up:	Prev track\n");
printf("  Cursor Down:	Next track\n");
printf("  Cursor Left:	Rewind\n");
printf("  Cursor Right:	Fast forward\n");
printf("  q: Quit\n");
printf("  e: Quit and Eject\n");
printf("  BACKSPACE: Quit and Eject\n");
printf("  Cntrl-C: Quit\n");
}





void PrintTracks(ListNode *Tracks)
{
ListNode *Curr;
TTrack *Track;
int val, Mins, Secs, Frames;

Curr=ListGetNext(Tracks);
if (Curr)
{
	Track=(TTrack *) Curr->Item;
	printf("\rArtist: %s                  \n",Track->Artist);
	printf("Album:  %s\n",Track->Album);

	//last track is a 'dummy' track
	while (Curr && Curr->Next)
	{
		Track=(TTrack *) Curr->Item;

		val=Track->NoOfFrames;
		Mins=val / (CD_FRAMES * 60);
		val-=Mins * 60 * CD_FRAMES;
		Secs=val / (CD_FRAMES);
		val-=Secs * CD_FRAMES;


		printf("  % 3d %02d:%02d:%02d %s\n",Track->TrackNo,Mins,Secs,val,Track->Title);
		Curr=ListGetNext(Curr);
	}
	printf(".\n");
}
}



int WriteID3Tag(char *Path, TTrack *Track)
{
TID3_TAG *Tag;
int fd;
char *ptr;

Tag=(TID3_TAG *) calloc(1,sizeof(TID3_TAG));

strncpy(Tag->TAG,"TAG",3);
if (StrLen(Track->Artist)) strncpy(Tag->Artist,Track->Artist,TAGFIELD_LEN);
if (StrLen(Track->Album)) strncpy(Tag->Album,Track->Album,TAGFIELD_LEN);
if (StrLen(Track->Title)) strncpy(Tag->Title,Track->Title,TAGFIELD_LEN);
strcpy(Tag->Comment,"");

strncpy(Tag->Year,"????",4);
Tag->Genre=12;

fd=open(Path,O_APPEND | O_WRONLY);
if (fd > -1)
{
	write(fd,Tag,sizeof(TID3_TAG));
	close(fd);
}
free(Tag);
}


//Replaces any characters in a string that would cause a problem with 
//file paths
char *ReplaceBadChars(char *Str)
{
int i, len;

len=StrLen(Str);
for (i=0; i < len; i++)
{
   if (Str[i]=='!') Str[i]='_';
   if (Str[i]=='"') Str[i]='_';
   if (Str[i]=='\'') Str[i]='_';
   if (Str[i]=='/') Str[i]='+';
   if (Str[i]=='(') Str[i]='_';
   if (Str[i]==')') Str[i]='_';
}

return(Str);
}



//Opens either a wav file or a /dev/dsp style device
STREAM *OpenOutput(ListNode *Vars)
{
	TAudioInfo *AI;
	int fd;
	STREAM *S=NULL;
	char *ptr;

ptr=GetVar(Vars,"Output");

AI=AudioInfoCreate(AFMT_S16_LE, 2, 44100, 2, 0);

if (strncmp(ptr,"dev:",4)==0)
{
	if (strcmp(ptr+4,"default")==0) fd=SoundOpenOutput("", AI);
	else fd=SoundOpenOutput(ptr+4, AI);
	S=STREAMFromFD(fd);
	STREAMResizeBuffer(S,OutBuffSize+(CD_FRAMESIZE_RAW * 40));
	STREAMSetFlushType(S,FLUSH_FULL,0);
}
else
{
 S=STREAMOpenFile(ptr,O_WRONLY|O_CREAT|O_TRUNC);
 if (S) SoundWriteWAVHeader(S,AI);
 else printf("FAILED TO OPEN %s\n",ptr);
}

free(AI);

return(S);
}


void CloseOutput(ListNode *Vars, STREAM *OutS)
{
TAudioInfo *AI;
struct stat StatInfo;
char *ptr;

ptr=GetVar(Vars,"Output");
if (strncmp(ptr,"dev:",4)!=0)
{
STREAMFlush(OutS);
fstat(OutS->out_fd,&StatInfo);
STREAMSeek(OutS,0,SEEK_SET);
AI=AudioInfoCreate(AFMT_S16_LE, 2, 44100, 2, 0);
AI->DataSize=StatInfo.st_size-44;
SoundWriteWAVHeader(OutS,AI);
free(AI);
}

STREAMClose(OutS);
}


int ReadKey(STREAM *InputS)
{
char *Tempstr=NULL, *ptr;
int RetVal=TRACK_CURR, result, i;

		Tempstr=SetStrLen(Tempstr,100);
		result=STREAMReadBytes(InputS,Tempstr,100);	

		switch (*Tempstr)
		{
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '0':
			usleep(500000);
			result=STREAMReadBytes(InputS,Tempstr+1,5);	
			RetVal=atoi(Tempstr);
			break;


			case '<':
			case ',':
			case '-':
			RetVal=TRACK_PREV;
			break;


			case '>':
			case '.':
			case '+':
			case '=':
			RetVal=TRACK_NEXT;
			break;
	


		case '\33':
			ptr=Tempstr+1;
			if (*ptr == '[')
			{
			ptr++;

			//printf("KEY: %c\n",*ptr);
			switch (*ptr)
			{
			//Arrow Keys
			case 'D': RetVal=TRACK_RW; break;
			case 'C': RetVal=TRACK_FF; break;
			case 'A': RetVal=TRACK_PREV; break;
			case 'B': RetVal=TRACK_NEXT; break;

			//Function Keys
			case '1': RetVal=1; break;
			case '2': RetVal=2; break;
			case '3': RetVal=3; break;
			case '4': RetVal=4; break;
			case '5': RetVal=5; break;
			case '6': RetVal=6; break;
			case '7': RetVal=7; break;
			case '8': RetVal=8; break;
			case '9': RetVal=9; break;
			case '0': RetVal=10; break;
			} //End of innner switch
		}
		//Keypad
		else if (*ptr=='O') 
		{
			ptr++;
			switch (*ptr)
			{
			case 'q': RetVal=1; break;
			case 'r': RetVal=2; break;
			case 's': RetVal=3; break;
			case 't': RetVal=4; break;
			case 'u': RetVal=5; break;
			case 'v': RetVal=6; break;
			case 'w': RetVal=7; break;
			case 'x': RetVal=8; break;
			case 'y': RetVal=9; break;
			case 'p': RetVal=10; break;
			case 'k': RetVal=TRACK_NEXT; break;
			case 'm': RetVal=TRACK_PREV; break;
			case 'n': RetVal=TRACK_LOOP; break;
			}			
		}
		break;

		case '\t': RetVal=TRACK_LOOP; break;


		//Pause
		case '\r':
		case '\n':
		case ' ':
			RetVal=TRACK_CURR;
			while (1)
			{
				usleep(500000);
				result=STREAMReadBytes(InputS,Tempstr,100);	
				if (result > 0) break;
			}
		break;

		//Backspace, stop, eject and exit
		case '\b':
		case 'e':
			RetVal=TRACK_EJECT;
		break;



		case '?':
			RetVal=PRINT_USAGE;
		break;

		case 'i':
		case 'I':
			RetVal=PRINT_TRACKS;
		break;

		case 3: //Ascii 3 is 'End of Text' and 'Control-C'
		case 'q': RetVal=TRACK_QUIT; break;
	}

DestroyString(Tempstr);
return(RetVal);
}


int CDSetSpeed(int cd, char *Speed)
{
int	val=0, target=0, result;

	if (strcmp(Speed,"lowest")==0)
	{
		for (val=1; val < 99; val++) 
		{
			result=ioctl(cd,CDROM_SELECT_SPEED,&val);
			printf("SPD: %d %d\n",val,result);
			if (result ==0) break;
		}
	}
	else
	{
		if (strcmp(Speed,"highest")==0) target=99;
		else target=atoi(Speed);

		for (val=target; val > -1; val--) 
		{
			result=ioctl(cd,CDROM_SELECT_SPEED,&val);
			if (result ==0) break;
		}
	}
}


//This is the main function that rips/plays a track
//By calling the CDROMREADRAW function and writing the data
//to file/device
int InternalRipper(char *CDRomDev, ListNode *Vars, int *Flags,  TTrack *Track)
{
 struct cdrom_msf msf;         
 char Frame[CD_FRAMESIZE_RAW]; 
 char *Tempstr=NULL;
 STREAM *S;
 int cd, val;
 int result;
 int FramesRead;
 int RetVal=TRACK_NEXT;
 

 cd = open(CDRomDev, O_RDONLY);
 if (cd==-1) return(ERR_OPEN_INPUT);

 Tempstr=CopyStr(Tempstr,GetVar(Vars,"CDSpeed"));

	printf("Rip %s %d\n",Tempstr,*Flags & FLAG_PLAY);
 if (StrLen(Tempstr)) CDSetSpeed(cd,Tempstr);
 else if (*Flags & FLAG_PLAY) CDSetSpeed(cd,"lowest");

 S=OpenOutput(Vars);
 if (! S) return(ERR_OPEN_OUTPUT);

 for (FramesRead=FramesRead=Track->FrameOffset; FramesRead < (Track->FrameOffset + Track->NoOfFrames); FramesRead++)
 {

 msf.cdmsf_min0=FramesRead / CD_FRAMES / CD_SECS;
 msf.cdmsf_sec0=(FramesRead / CD_FRAMES) % CD_SECS;
 msf.cdmsf_frame0=FramesRead  % CD_FRAMES;

	memcpy(Frame,(char *) &msf,sizeof(struct cdrom_msf));
  result=ioctl(cd, CDROMREADRAW, Frame);
	STREAMWriteBytes(S,(char *) Frame, CD_FRAMESIZE_RAW);
	if ((*Flags & FLAG_CONTINUING) || (FramesRead > OutBuffSize)) 
	{
		STREAMFlush(S);
		//FlAG_CONTINUING means that we're not seeking or starting a fresh read from the CD
		//instead we are smoothly flowing from one track to another
		*Flags |= FLAG_CONTINUING;
	}

	if ((*Flags & FLAG_PLAY) && (FramesRead % 20))
	{
 		if (StdIn && STREAMCheckForBytes(StdIn)) 
		{
			if (ReadInput) RetVal=ReadInput(StdIn);
			if (RetVal==TRACK_CURR); //do nothing
			else if (RetVal==TRACK_RW) FramesRead-=CD_FRAMES * 5;
			else if (RetVal==TRACK_FF) FramesRead+=CD_FRAMES * 5;
			else if (RetVal==TRACK_LOOP) 
			{
				if (*Flags & FLAG_LOOP) *Flags &= ~FLAG_LOOP;
				else *Flags |= FLAG_LOOP;

				if (*Flags & FLAG_LOOP) printf("\rLOOP ON                          \r\n");
				else printf("\rloop off                         \r\n");
			}
			else if (RetVal==PRINT_USAGE) PrintKeys();
			else if (RetVal==PRINT_TRACKS) PrintTracks(Tracks);
			else
			{
				//All other posibilities are various forms of 'next/prev' track
				//or stop.

				//We'll be doing a fresh seek on the next play, so unset this
				*Flags &= ~FLAG_CONTINUING;
				break;
			}

			if (FramesRead < 0) FramesRead=0;
		}
		
		printf("\r% 3d%%  %02d:%02d:%02d          ",(FramesRead - Track->FrameOffset) * 100 / Track->NoOfFrames, msf.cdmsf_min0,msf.cdmsf_sec0,msf.cdmsf_frame0);
	}

 }   

DestroyString(Tempstr);
CloseOutput(Vars,S);

close(cd);

return(RetVal);
}


//The following functions all relate to examining the tracks on a disc
TTrack *AudioCDReadTrack(int drive, int trackno)
{
   struct cdrom_tocentry tocentry;
   TTrack *Track;

		Track=(TTrack *) calloc(1,sizeof(TTrack));
	  Track->TrackNo=trackno;	
		Track->Artist=CopyStr(Track->Artist,"????");
		Track->Album=CopyStr(Track->Album,"????");
   	Track->Title=FormatStr(Track->Title,"track %d",trackno);
        tocentry.cdte_track = trackno;
        tocentry.cdte_format = CDROM_MSF;
        ioctl(drive, CDROMREADTOCENTRY, &tocentry);
	
				Track->Mins=tocentry.cdte_addr.msf.minute;
				Track->Secs=tocentry.cdte_addr.msf.second;
        Track->Frames = tocentry.cdte_addr.msf.frame;
        Track->FrameOffset = tocentry.cdte_addr.msf.frame;
        Track->FrameOffset += Track->Mins * CD_SECS * CD_FRAMES;
        Track->FrameOffset += Track->Secs * CD_FRAMES;

	return(Track);
}


int AudioCDReadContents(char *CDRomDev, ListNode *Tracks)
{
    int drive, result;
    int i, val;
    char *Tempstr=NULL;
		TTrack *Track;
   struct cdrom_tochdr tochdr;

    drive = open(CDRomDev, O_RDONLY | O_NONBLOCK);
    if (drive==-1) return(-1);
    result=ioctl(drive, CDROMREADTOCHDR, &tochdr);

    for (i = tochdr.cdth_trk0; i <= tochdr.cdth_trk1; i++) 
    {
			Track=AudioCDReadTrack(drive,  i);
			ListAddItem(Tracks,Track);
    }

		Track=AudioCDReadTrack(drive, 0xAA);
		ListAddItem(Tracks,Track);

//		ioctl(drive,CDROM_LOCKDOOR,0);
		close(drive);

 DestroyString(Tempstr);
 return(tochdr.cdth_trk1);
}


int CDStatus(char *CDRomDev)
{
    int drive;
    struct cdrom_subchnl subchnl;
    struct cdrom_ti track_index;

    drive = open(CDRomDev, O_RDONLY | O_NONBLOCK);
    memset(&subchnl,0,sizeof(struct cdrom_subchnl));
    subchnl.cdsc_format=CDROM_MSF;
    ioctl(drive, CDROMSUBCHNL, &subchnl);
    close(drive);

if (subchnl.cdsc_audiostatus==CDROM_AUDIO_PLAY) return(TRUE);
return(FALSE);
}


int CDPlayTrack(char *CDRomDev, int TrackNo)
{
    int drive;
    int trackpos;
    struct cdrom_tochdr tochdr;
    struct cdrom_ti track_index;

    drive = open(CDRomDev, O_RDONLY | O_NONBLOCK);
    ioctl(drive, CDROMREADTOCHDR, &tochdr);
    trackpos=tochdr.cdth_trk0 + TrackNo;
    
   track_index.cdti_trk0=trackpos;
   track_index.cdti_ind0=0;
   track_index.cdti_trk1=trackpos;
   track_index.cdti_ind1=255;
    
   ioctl(drive,CDROMPLAYTRKIND,(void *) &track_index);
   close(drive);

return(TRUE);
}


int CDPause(char *CDRomDev)
{
    int drive;

    drive = open(CDRomDev, O_RDONLY | O_NONBLOCK);
    ioctl(drive, CDROMPAUSE, 0);
    close(drive);

return(TRUE);
}


int CDStop(char *CDRomDev)
{
    int drive;

    drive = open(CDRomDev, O_RDONLY | O_NONBLOCK);
    ioctl(drive, CDROMSTOP, 0);
    close(drive);

return(TRUE);
}


int CDResume(char *CDRomDev)
{
    int drive;

    drive = open(CDRomDev, O_RDONLY | O_NONBLOCK);
    ioctl(drive, CDROMRESUME, 0);
    close(drive);

return(TRUE);
}

int CDEject(char *CDRomDev)
{
    int drive;

    drive = open(CDRomDev, O_RDONLY | O_NONBLOCK);
    ioctl(drive, CDROMEJECT, 0);
    close(drive);

return(TRUE);
}




THelper *AddHelperProgram(ListNode *Helpers, int Type, char *MediaType, char *CommandLine)
{
THelper *ProgInfo;

ProgInfo=(THelper *) calloc(1,sizeof(THelper));
ProgInfo->Type=Type;
ProgInfo->MediaType=CopyStr(NULL,MediaType);
ProgInfo->CommandLine=CopyStr(NULL,CommandLine);

ListInsertItem(Helpers,ProgInfo);

return(ProgInfo);
}

int SelectAvailableHelpers(TArgs *Args,ListNode *Helpers)
{
ListNode *Curr;
THelper *ProgInfo;
char *Path=NULL, *Cmd=NULL, *SearchPath=NULL, *SelectedMediaType=NULL, *SelectedCommand=NULL, *ptr;
int result=FALSE;

SearchPath=CopyStr(SearchPath,getenv("PATH"));

Curr=ListGetNext(Helpers);
while (Curr)
{
	ProgInfo=(THelper *) Curr->Item;

	if (
				(StrLen(Args->OutputFormat)==0) || 
				(strcmp(ProgInfo->MediaType,Args->OutputFormat)==0)
		)
		{
			ptr=GetToken(ProgInfo->CommandLine," ",&Cmd,0);
			Path=FindFileInPath(Path,Cmd,SearchPath);	
			if (StrLen(Path))
			{
				if (ProgInfo->Type==PT_ENCODER) 
				{
					Args->EncodeCommand=MCopyStr(Args->EncodeCommand,Path," ",ptr,NULL);
					SelectedMediaType=CopyStr(SelectedMediaType,ProgInfo->MediaType);
					result=TRUE;
				}
				printf("Found Helper Program: %s\n",Path);
			}
		}

	Curr=ListGetNext(Curr);
}


if (! StrLen(Args->OutputFormat))
{
	if (StrLen(SelectedMediaType)) 
	{
		Args->OutputFormat=CopyStr(Args->OutputFormat,SelectedMediaType);
	}
	else Args->OutputFormat=CopyStr(Args->OutputFormat,"enc");
}


DestroyString(Cmd);
DestroyString(Path);
DestroyString(SearchPath);
DestroyString(SelectedMediaType);
}


void SetupHelperPrograms(TArgs *Args)
{
Helpers=ListCreate();
AddHelperProgram(Helpers,PT_RIPPER,"","cdparanoia -d $(CDDev) $(TrackNo) -w '$(Output)'");

/*'$(Output)' here is the output of ripping the cd, which is the input */
/* to the encoder */
AddHelperProgram(Helpers,PT_ENCODER,"mp3","gogo -m j '$(Output)' '$(EncPath)'");
AddHelperProgram(Helpers,PT_ENCODER,"mp3","lame '$(Output)' '$(EncPath)'");
AddHelperProgram(Helpers,PT_ENCODER,"mp3","ffmpeg -vn -i '$(Output)' -f mp3 '$(EncPath)'");
AddHelperProgram(Helpers,PT_ENCODER,"flac","ffmpeg -vn -i '$(Output)' -f flac '$(EncPath)'");
AddHelperProgram(Helpers,PT_ENCODER,"ogg","oggenc '$(Output)' -o '$(EncPath)'");
AddHelperProgram(Helpers,PT_ENCODER,"flac","flac '$(Output)' -o '$(EncPath)'");

//Find Suitable Programs
SelectAvailableHelpers(Args,Helpers);
}





TArgs *ParseArgs(int argc, char *argv[])
{
TArgs *Args;
int i;
char *Tempstr=NULL;

Args=(TArgs *) calloc(1,sizeof(TArgs));
Args->Flags |= FLAG_PLAY;
Args->CDDBServer=CopyStr(Args->CDDBServer,"freedb.freedb.org");
Args->OutputPath=CopyStr(Args->OutputPath,"dev:default");

if (! isatty(0)) Args->Flags |= FLAG_SLAVE;

for (i=1; i < argc; i++)
{
if (strcmp(argv[i],"-f")==0) Args->OutputFormat=CopyStr(Args->OutputFormat,argv[++i]);
else if (strcmp(argv[i],"-o")==0) Args->OutputPath=CopyStr(Args->OutputPath,argv[++i]);
else if (strcmp(argv[i],"-b")==0) Args->Flags |= FLAG_BACKGROUND;
else if (strcmp(argv[i],"-c")==0) Args->Flags &= ~FLAG_SLAVE;
else if (strcmp(argv[i],"-e")==0) Args->Flags |= FLAG_EJECT;
else if (strcmp(argv[i],"-s")==0) Args->Speed=atoi(argv[++i]);
else if (strcmp(argv[i],"-r")==0) 
{
Args->Flags &=~FLAG_PLAY;
Args->OutputPath=CopyStr(Args->OutputPath,"$(TrackNo)-$(Title)");
}
else if (strcmp(argv[i],"-cddb")==0) 
{
Args->CDDBServer=CopyStr(Args->CDDBServer,argv[++i]);
if ((StrLen(Args->CDDBServer)==0) || (strcasecmp(Args->CDDBServer,"none")==0)) Args->Flags |= FLAG_NOCDDB;
}
else if (strcmp(argv[i],"-eject")==0) Args->Flags |= FLAG_EJECT;
else if (strcmp(argv[i],"-s")==0) Args->Flags |= FLAG_SLAVE;
else if (strcmp(argv[i],"-t")==0) Args->Flags |= FLAG_TEST;
else if (strcmp(argv[i],"-test")==0) Args->Flags |= FLAG_TEST;
else if (strcmp(argv[i],"-x")==0) Args->Flags |= FLAG_OVERLAP_PROGRAMS;
else if (strcmp(argv[i],"-?")==0) Args->Flags |= FLAG_HELP;
else if (strcmp(argv[i],"-help")==0) Args->Flags |= FLAG_HELP;
else if (strcmp(argv[i],"--help")==0) Args->Flags |= FLAG_HELP;
else if (strcmp(argv[i],"-v")==0) Args->Flags |= FLAG_VERSION;
else if (strcmp(argv[i],"-version")==0) Args->Flags |= FLAG_VERSION;
else if (strcmp(argv[i],"--version")==0) Args->Flags |= FLAG_VERSION;
else if (strcmp(argv[i],"-i")==0) Args->InfoFile=CopyStr(Args->InfoFile,argv[++i]);
else if (isdigit(argv[i][0])) Args->Tracks=MCatStr(Args->Tracks,argv[i]," ",NULL);
else Args->CdDevice=CopyStr(Args->CdDevice,argv[i]);
}


DestroyString(Tempstr);
return(Args);
}



char *FindCDDevice(char *In)
{
char *CdDevs[]={"cdrom","scd0","hcd0","sr0",NULL};
char *CdDevice=NULL, *Tempstr=NULL;
int i;

CdDevice=CopyStr(In,"");
	printf("Searching for CD device:\n");
	for (i=0; CdDevs[i] !=NULL; i++)
	{
		Tempstr=MCopyStr(Tempstr,"/dev/",CdDevs[i],NULL);
	
		printf("	checking '%s' ...",Tempstr);
		if (access(Tempstr,F_OK)==0)
		{
			CdDevice=CopyStr(CdDevice,Tempstr);
			printf("YES!\n");
			break;
		}
		printf("no.\n");
	}

DestroyString(Tempstr);
return(CdDevice);
}

void LoadTracksFromInfoFile(char *Path, ListNode *Tracks)
{
STREAM *S;
char *Tempstr=NULL, *Artist=NULL, *Album=NULL;
int count=0;
ListNode *Curr;
TTrack *Track;

S=STREAMOpenFile(Path,O_RDONLY);
if (S)
{
	Tempstr=STREAMReadLine(Tempstr,S);

	while (Tempstr)
	{
		StripTrailingWhitespace(Tempstr);

		if (StrLen(Tempstr)) 
		{
			if (count==0) Artist=CopyStr(Artist,Tempstr);
			else if (count==1) Album=CopyStr(Album,Tempstr);
			else
			{
				Curr=ListGetNth(Tracks,count-2);
				Track=(TTrack *) Curr->Item;
				Track->Artist=CopyStr(Track->Artist,Artist);
				Track->Album=CopyStr(Track->Album,Album);
				Track->Title=CopyStr(Track->Title,Tempstr);
			}
			
			count++;
		}		
		Tempstr=STREAMReadLine(Tempstr,S);
	}

STREAMClose(S);
}

DestroyString(Tempstr);
DestroyString(Artist);
DestroyString(Album);
}

void AnalyzeTracks(TArgs *Args, ListNode *Tracks)
{
ListNode *Curr;
TTrack *Track, *PrevTrack=NULL;

	if (StrLen(Args->InfoFile)) 
	{
		LoadTracksFromInfoFile(Args->InfoFile,Tracks);
		if (! (Args->Flags & FLAG_NOCDDB))
		{
    if (! FreeDBLookup(Args->CDDBServer,Tracks,FALSE))
		{
			printf("This disk is not in freedb. Looking it up\n");
			FreeDBSend(Args->CDDBServer, Tracks);
		}
		}
		//update cache with InfoFile information
		FreeDBCacheWrite(Tracks);
	}
	else
	{
		//Do FreeDB Lookup
		if (! (Args->Flags & FLAG_NOCDDB)) FreeDBLookup(Args->CDDBServer,Tracks,TRUE);
	}


//Calculate Track length in frames
	Curr=ListGetNext(Tracks);
	while (Curr)
	{
		Track=(TTrack *) Curr->Item;
		if (PrevTrack)
		{
			PrevTrack->NoOfFrames=Track->FrameOffset-PrevTrack->FrameOffset;
		}
		PrevTrack=Track;
	 	Curr=ListGetNext(Curr);
	}

}


ListNode *GetNextTrack(ListNode *CurrTrack, char *Tracks)
{
static char *ptr=NULL;
char *Token=NULL;
ListNode *Curr;

if (StrLen(Tracks))
{
	if (! ptr) ptr=Tracks;
	ptr=GetToken(ptr," ",&Token,0);

	if (! ptr) CurrTrack=NULL;
	Curr=ListGetHead(CurrTrack);
	Curr=ListGetNth(Curr,atoi(Token)-1);
}
else
{
	Curr=ListGetNext(CurrTrack);
}

DestroyString(Token);

return(Curr);
}



void PrintAlbumDetails(ListNode *Tracks)
{
ListNode *Curr;
TTrack *Track;
char *Tempstr=NULL;

	Curr=ListGetNext(Tracks);
	if (Curr)
	{
		 Track=(TTrack *) Curr->Item;
		 Tempstr=FormatStr(Tempstr,"%s-%s",Track->Artist,Track->Album);
		 mkdir(Tempstr,0700);
		 chdir(Tempstr);
	}

DestroyString(Tempstr);
}


void SetupVars(TArgs *Args, TTrack *Track)
{
char *Tempstr=NULL;

		Tempstr=FormatStr(Tempstr,"%02d",Track->TrackNo);
		SetVar(Vars,"TrackNo",Tempstr);
		SetVar(Vars,"Artist",ReplaceBadChars(Track->Artist));
		SetVar(Vars,"Album",ReplaceBadChars(Track->Album));
		SetVar(Vars,"Title",ReplaceBadChars(Track->Title));

		if (strncmp(Args->OutputPath,"dev:",4)==0) Tempstr=CopyStr(Tempstr,Args->OutputPath);
		else
		{
		  Tempstr=SubstituteVarsInString(Tempstr,Args->OutputPath,Vars,0);
			Tempstr=CatStr(Tempstr,".wav");
		}

		SetVar(Vars,"Output",Tempstr);

		SetVar(Vars,"CDDev",Args->CdDevice);

		Tempstr=SubstituteVarsInString(Tempstr,Args->OutputPath,Vars,0);
		Tempstr=MCatStr(Tempstr,".",Args->OutputFormat,NULL);
		SetVar(Vars,"EncPath",Tempstr);

DestroyString(Tempstr);
}



int RunRipCommand(TArgs *Args, ListNode *Vars, TTrack *Track)
{
int result, RetVal;
char *Tempstr=NULL;

	if (StrLen(Args->RipCommand))
	{
		Tempstr=SubstituteVarsInString(Tempstr,Args->RipCommand,Vars,0);
		if (StrLen(Tempstr)) 
		{
			printf("Launch: %s\n",Tempstr);
			result=SpawnWithIO(Tempstr,-1,1,2);
			waitpid(result,NULL,0);
		}
		RetVal=TRACK_NEXT;
	}
	else 
	{
		RetVal=InternalRipper(Args->CdDevice,Vars,&Args->Flags,Track);
	}

DestroyString(Tempstr);
return(RetVal);
}


void EncodeRippedWavFile(TArgs *Args, TTrack *Track)
{
char *Tempstr=NULL;
pid_t pid;

		if (! (Args->Flags & (FLAG_TEST | FLAG_PLAY)))
		{
			Tempstr=SubstituteVarsInString(Tempstr,Args->EncodeCommand,Vars,0);
			printf("Launch: [%s]\n",Tempstr);
			if (Args->Flags & FLAG_OVERLAP_PROGRAMS) 
			{
				if (EncodePid > 0) waitpid(EncodePid,NULL,0);
				EncodePid=SpawnWithIO(Tempstr,-1,-1,2);
			}
			else 
			{
				pid=SpawnWithIO(Tempstr,-1,1,2);
				waitpid(pid,NULL,0);
			}

			SetVar(Vars,"EncPath",Tempstr);
 			WriteID3Tag(GetVar(Vars,"EncPath"),Track);
		}

DestroyString(Tempstr);
}


void RipOrPlayTrack(TArgs *Args, ListNode *Vars, int TrackNo)
{
ListNode *Curr;
TTrack *Track;

		if (TrackNo < 1) TrackNo=1;
		Curr=ListGetNth(Tracks,TrackNo-1);
		if (Curr)
		{
			Track=(TTrack *) Curr->Item;
			SetupVars(Args, Track);
			RunRipCommand(Args, Vars, Track);
			if (! (Args->Flags & FLAG_PLAY)) EncodeRippedWavFile(Args, Track);
		}
}



void DoRipOrPlay(TArgs *Args, ListNode *Vars)
{
ListNode *Curr;
TTrack *Track;
char *Tempstr=NULL;
int i, NoOfTracks=0, result;

	NoOfTracks=ListSize(Tracks);
	Curr=GetNextTrack(Tracks, Args->Tracks);

	//Check for Curr->Next because CD's always have an extra blank track at the end 
	 while (Curr && Curr->Next)
	 {
		Track=(TTrack *) Curr->Item;
		printf("\r%d %s-%s/%s\n",Track->TrackNo,Track->Artist,Track->Album,Track->Title);
		sprintf(CmdLine,"%s: track %d of %d\0",ProgName,Track->TrackNo,NoOfTracks);

		SetupVars(Args, Track);

		if (! (Args->Flags & FLAG_TEST))
		{
			result=RunRipCommand(Args,Vars,Track);
			if (result > -1)
			{
					Curr=ListGetNth(Tracks, result);
					Track=(TTrack *) Curr->Item;
					//this should fall through to 'result==TRACK_PREV'
					result=TRACK_PREV;
			}

			//no else here, previous 'if' statement falls through!
			switch (result)
			{
					case ERR_OPEN_INPUT:
						printf("Failed to open input %s\n",Args->CdDevice);
						Curr=NULL;
					break;
					
					case ERR_OPEN_OUTPUT:
						 printf("Failed to open output %s\n",GetVar(Vars,"Output"));
					break;

					case TRACK_PREV:
						if (Curr->Prev) Curr=Curr->Prev;
						if (Curr->Prev) Curr=Curr->Prev;
						else Curr=Tracks;
					break;

					case TRACK_QUIT:
						Curr=NULL;
					break;
	
					case TRACK_EJECT:
						Args->Flags |= FLAG_EJECT;
						Curr=NULL;
					break;
				}
		}

		EncodeRippedWavFile(Args,Track);
		ListClear(Vars,DestroyString);
		Curr=GetNextTrack(Curr, Args->Tracks);
	 }


if (Args->Flags & FLAG_PLAY) 
{
ResetTTY(0);
}

DestroyString(Tempstr);
}


void HandleCommandSet(TArgs *Args, char *Data)
{
char *Token=NULL, *ptr;
		
		ptr=GetToken(Data," ",&Token,0);

		if (strcmp(Token,"OutputFormat")==0) 
		{
			Args->OutputFormat=CopyStr(Args->OutputFormat,ptr);
			SelectAvailableHelpers(Args,Helpers);
		}

DestroyString(Token);
}


int ReadSlaveModeString(STREAM *InputS, TArgs *Args)
{
char *Tempstr=NULL, *Token=NULL, *ptr;
int RetVal=TRACK_CURR, result;
char *Cmds[]={"INFO","PLAY","RW","FF","LOOP","PAUSE","EJECT","RIP","SET","QUIT",NULL};
typedef enum {CMD_INFO, CMD_PLAY, CMD_RW, CMD_FF, CMD_LOOP, CMD_PAUSE, CMD_EJECT, CMD_RIP, CMD_SET, CMD_QUIT};

		Tempstr=STREAMReadLine(Tempstr,InputS);

		StripTrailingWhitespace(Tempstr);
		ptr=GetToken(Tempstr,"\\S",&Token,0);
		result=MatchTokenFromList(Token,Cmds,0);
		switch (result)
		{
			case CMD_PLAY:
			Args->Flags |=FLAG_PLAY;
			if (strcmp(ptr,"NEXT")==0) RetVal=TRACK_NEXT;
			else if (strcmp(ptr,"PREV")==0) RetVal=TRACK_PREV;
			else RetVal=atoi(ptr);
			Args->OutputPath=CopyStr(Args->OutputPath,GetVar(Vars,"PlayOutputPath"));
			break;

			case CMD_RIP:
			Args->Flags &=~FLAG_PLAY;
			if (strcmp(ptr,"NEXT")==0) RetVal=TRACK_NEXT;
			else if (strcmp(ptr,"PREV")==0) RetVal=TRACK_PREV;
			else RetVal=atoi(ptr);
			Args->OutputPath=CopyStr(Args->OutputPath,GetVar(Vars,"RipOutputPath"));
			break;

			case CMD_RW:
				RetVal=TRACK_RW;
			break;

			case CMD_LOOP:
				if (strcasecmp(ptr,"on")==0) RetVal=TRACK_LOOP;
			break;


		//Pause
		case CMD_PAUSE:
			RetVal=TRACK_CURR;
			while (1)
			{
				usleep(500000);
				result=STREAMReadBytes(InputS,Tempstr,100);	
				if (result > 0) break;
			}
		break;

		case CMD_EJECT:
			RetVal=TRACK_EJECT;
		break;

		case CMD_INFO:
			RetVal=PRINT_TRACKS;
		break;

		case CMD_SET:
			HandleCommandSet(Args, ptr);
		break;

		case CMD_QUIT: RetVal=TRACK_QUIT; break;
	}

DestroyString(Tempstr);
DestroyString(Token);

return(RetVal);
}


void SlaveMode(TArgs *Args, ListNode *Vars)
{
char *Tempstr=NULL, *Token=NULL, *ptr;
int result, TrackNo=1;
ListNode *Curr;
TTrack *Track;


result=ReadSlaveModeString(StdIn, Args);
while (result != TRACK_QUIT)
{
	switch (result)
	{
		case TRACK_PREV:
			TrackNo--;
		break;

		case TRACK_NEXT:
			TrackNo++;
		break;

		case PRINT_TRACKS:
			PrintTracks(Tracks);
		break;

		case TRACK_EJECT:
			Args->Flags |= FLAG_EJECT;
			result=TRACK_QUIT;
		break;

		default:
			if (result > 0)
			{
			TrackNo=result;
			}
		break;
	}

	switch (result)
	{	
		case PRINT_TRACKS:
		case TRACK_EJECT:
		break;

		default:
		RipOrPlayTrack(Args, Vars, TrackNo);
		printf("DONE\n");
		break;
	}

	if (result != TRACK_QUIT) result=ReadSlaveModeString(StdIn,Args);
}


DestroyString(Tempstr);
DestroyString(Token);
}

int ReadConfigFile(char *Path, TArgs *Args)
{
STREAM *S;
char *Tempstr=NULL, *Token=NULL, *ptr;

S=STREAMOpenFile(Path,O_CREAT | O_RDONLY);

if (S)
{
	Tempstr=STREAMReadLine(Tempstr,S);
	while (Tempstr)
	{
		ptr=GetToken(Tempstr,"=",&Token,0);

		if (strcasecmp(Token,"Encoder")==0)
		{
			ptr=GetToken(ptr,",",&Token,0);
			AddHelperProgram(Helpers,PT_ENCODER,Token,ptr);
		}
		else if (strcasecmp(Token,"Ripper")==0)
		{
			AddHelperProgram(Helpers,PT_RIPPER,"",ptr);
		}
		else if (strcasecmp(Token,"EjectWhenDone")==0)
		{	
			if (strcasecmp(ptr,"Y")==0) Args->Flags |= FLAG_EJECT;
		}
		else if (strcasecmp(Token,"Overlap")==0)
		{	
			if (strcasecmp(ptr,"Y")==0) Args->Flags |= FLAG_OVERLAP_PROGRAMS;
		}
		else if (strcasecmp(Token,"CDDBServer")==0)
		{
			Args->CDDBServer=CopyStr(Args->CDDBServer,ptr);
		}
		else if (strcasecmp(Token,"CDDevice")==0)
		{
			Args->CdDevice=CopyStr(Args->CdDevice,ptr);
		}

		Tempstr=STREAMReadLine(Tempstr,S);
	}
STREAMClose(S);
}

DestroyString(Tempstr);
DestroyString(Token);
}


main(int argc, char *argv[])
{
ListNode *Curr;
char *Tempstr=NULL, *OutPath=NULL;
TArgs *Args;

	Tracks=ListCreate();
	Vars=ListCreate();

	ProgName=CopyStr(ProgName,argv[0]);

	Args=ParseArgs(argc,argv);
	if (Args->Flags & FLAG_HELP) 
	{
		PrintUsage();
		exit(0);
	}

	if (Args->Flags & FLAG_VERSION) 
	{
		printf("cd-command (CD Command) %s\n",PROG_VERSION);
		exit(0);
	}


	ReadConfigFile("/etc/cd-command.conf",Args);

	Tempstr=MCopyStr(Tempstr,GetCurrUserHomeDir(),".cd-command.conf",NULL);

	if (argc > 1) argv[1]=NULL;
	CmdLine=argv[0];

	if (Args->Speed > 0) 
	{
		Tempstr=FormatStr(Tempstr,"%d",Args->Speed);
		SetVar(Vars,"CDSpeed",Tempstr);
	}

	if (StrLen(Args->CdDevice)==0) Args->CdDevice=FindCDDevice(Args->CdDevice);

	if (! StrLen(Args->CdDevice))
	{
		printf("Cannot find a cd device! Specify one on the command line.\n");
		exit(1);
	}

	SetupHelperPrograms(Args);

  AudioCDReadContents(Args->CdDevice,Tracks);
	if (ListSize(Tracks) ==0)
	{
		printf("Failed to read from %s\n",Args->CdDevice);
	}
	else
	{
	if (Args->Flags & FLAG_BACKGROUND) demonize();
	else
	{
		if (Args->Flags & FLAG_SLAVE)	
		{
			StdIn=STREAMFromFD(0);
			Tempstr=CopyStr(Tempstr,"$(TrackNo)-$(Title)");
			SetVar(Vars,"RipOutputPath",Tempstr);
		}
		else if (Args->Flags & FLAG_PLAY) 
		{
			InitTTY(0, 38400,TTYFLAG_LFCR);
			StdIn=STREAMFromFD(0);
			STREAMSetNonBlock(StdIn,TRUE);
			SetVar(Vars,"PlayOutputPath",Args->CdDevice);
			ReadInput=ReadKey;
			printf("Press '?' to see keys\n");
		}
	}
	AnalyzeTracks(Args,Tracks);
 
	if (! (Args->Flags & (FLAG_TEST | FLAG_PLAY))) PrintAlbumDetails(Tracks);
	
	if (Args->Flags & FLAG_SLAVE) SlaveMode(Args, Vars);
	else DoRipOrPlay(Args,Vars);
	}

	if (Args->Flags & FLAG_EJECT) CDEject(Args->CdDevice);
}
