#include "cddb.h"

//------------------- CDDB / FREEDB Section. ----------------------
//These functions are involved in reading tracklists from CDD servers
//and writing the results to a local cache

//taken from the example on freedb.org
int cddb_sum(int n)
{
	int ret;

	/* For backward compatibility this algorithm must not change */

	ret = 0;

	while (n > 0) {
		ret = ret + (n % 10);
		n = n / 10;
	}

	return (ret);
}


unsigned long calc_cddb_discid(ListNode *Tracks)
{
ListNode *Curr;
int trackcount=0, val1=0, val2=0;
TTrack *Track, *FirstTrack;
char *ptr;

Curr=ListGetNext(Tracks);
if (Curr) FirstTrack=(TTrack *) Curr->Item;
while (Curr)
{
  Track=(TTrack *) Curr->Item;
  if (Curr->Next !=NULL)
  {
	  trackcount++;
	  val1=val1 + cddb_sum((Track->Mins *60) + Track->Secs);
  }

  Curr=ListGetNext(Curr);
}

val2=((Track->Mins * 60) + Track->Secs) - ((FirstTrack->Mins * 60) + FirstTrack->Secs);

return((val1 % 0xff) << 24 | val2 << 8 | trackcount);
}

char *FreeDBGenerateIDString(char *RetStr, ListNode *Tracks)
{
char *CDDB_ID=NULL;
int trackcount;
unsigned long DiskIDVal;
ListNode *Curr;
TTrack *Track;
int total_len;
char *Tempstr=NULL;

CDDB_ID=RetStr;
//Calculate the string that identifies a track
trackcount=ListSize(Tracks)-1;
DiskIDVal=calc_cddb_discid(Tracks);
CDDB_ID=FormatStr(CDDB_ID,"%x %d ",DiskIDVal, trackcount);

Curr=ListGetNext(Tracks);
while (Curr)
{
	Track=(TTrack *) Curr->Item;

	total_len+=(Track->Mins * CD_SECS) + Track->Secs;

	if (Curr->Next)
	{
		Tempstr=FormatStr(Tempstr,"%d ",Track->FrameOffset);
		CDDB_ID=CatStr(CDDB_ID,Tempstr);
	}
Curr=ListGetNext(Curr);
}

//Get total seconds from Frame offset of last track (last track is a blank track)
//there are 75 frames to a second
Tempstr=FormatStr(Tempstr,"%d",Track->FrameOffset / CD_FRAMES);
CDDB_ID=CatStr(CDDB_ID,Tempstr);

DestroyString(Tempstr);

return(CDDB_ID);
}


void FreeDBCacheReadTrackDetails(STREAM *S, ListNode *Tracks)
{
char *Tempstr=NULL, *Artist=NULL, *Album=NULL, *Token=NULL, *ptr;
ListNode *Curr;
int val;
TTrack *Track;

Tempstr=STREAMReadLine(Tempstr,S);
while (Tempstr)
{
StripTrailingWhitespace(Tempstr);

ptr=GetToken(Tempstr,":",&Token,0);
StripLeadingWhitespace(ptr);

if (strcmp(Token,"}")==0) break;
else if (strcmp(Token,"Artist")==0) Artist=CopyStr(Artist,ptr);
else if (strcmp(Token,"Album")==0) Album=CopyStr(Album,ptr);
else if (isdigit(*Token))
{
	val=atoi(Token);
	Curr=ListGetNth(Tracks,val-1);
	if (Curr)
	{
		Track=(TTrack *) Curr->Item;
		Track->Artist=CopyStr(Track->Artist,Artist);
		Track->Album=CopyStr(Track->Album,Album);
		Track->Title=CopyStr(Track->Title,ptr);
	}
}

Tempstr=STREAMReadLine(Tempstr,S);
}

DestroyString(Tempstr);
DestroyString(Artist);
DestroyString(Album);
DestroyString(Token);
}


int FreeDBCacheRead(int Bucket, char *ID, ListNode *Tracks)
{
int result=FALSE;
char *Tempstr=NULL;
STREAM *S;

Tempstr=FormatStr(Tempstr,"/var/cache/cddb/%x.dat",Bucket);
S=STREAMOpenFile(Tempstr, O_RDONLY);

if (! S)
{
Tempstr=FormatStr(Tempstr,"%s/.cd-util/%x.dat",GetCurrUserHomeDir(),Bucket);
S=STREAMOpenFile(Tempstr, O_RDONLY);
}

if (S)
{
Tempstr=STREAMReadLine(Tempstr,S);
while (Tempstr)
{
	StripLeadingWhitespace(Tempstr);
	StripTrailingWhitespace(Tempstr);
	if (strcmp(Tempstr,ID)==0)
	{
		FreeDBCacheReadTrackDetails(S,Tracks);
		result=TRUE;
		break;
	}
Tempstr=STREAMReadLine(Tempstr,S);
}

STREAMClose(S);
}
DestroyString(Tempstr);

return(result);
}

void FreeDBCacheWrite(ListNode *Tracks)
{
char *Tempstr=NULL;
STREAM *S;
ListNode *Curr;
TTrack *Track;
int trackcount;
char *CDDB_ID=NULL;
unsigned long DiskIDVal, Bucket;

//Calculate the string that identifies a track
trackcount=ListSize(Tracks)-1;
DiskIDVal=calc_cddb_discid(Tracks);
Bucket=DiskIDVal % NO_OF_CACHE_BUCKETS, 
CDDB_ID=FreeDBGenerateIDString(CDDB_ID,Tracks);

Tempstr=FormatStr(Tempstr,"/var/cache/cddb/%x.dat",Bucket);
S=STREAMOpenFile(Tempstr,O_CREAT | O_WRONLY);
if (! S)
{
	Tempstr=FormatStr(Tempstr,"%s/.cd-util/",GetCurrUserHomeDir());
	mkdir(Tempstr,0700);
	Tempstr=FormatStr(Tempstr,"%s/.cd-util/%x.dat",GetCurrUserHomeDir(),Bucket);
	S=STREAMOpenFile(Tempstr,O_CREAT | O_WRONLY);
}

if (S)
{
	Curr=ListGetNext(Tracks);
	if (Curr)
	{
		Track=(TTrack *) Curr->Item;
		Tempstr=FormatStr(Tempstr,"%s\n{\nAlbum: %s\nArtist: %s\n",CDDB_ID,Track->Album,Track->Artist);

		STREAMWriteLine(Tempstr,S);

		while (Curr)
		{
		Track=(TTrack *) Curr->Item;
		if (Track->TrackNo==0xAA) break;
		Tempstr=FormatStr(Tempstr,"%d: %s\n",Track->TrackNo,Track->Title);
		STREAMWriteLine(Tempstr,S);
		Curr=ListGetNext(Curr);
		}
		STREAMWriteLine("}\n\n",S);
	}

STREAMClose(S);
}


DestroyString(Tempstr);
DestroyString(CDDB_ID);
}


STREAM *FreeDBOpenCommand(char *Server, char *Command)
{
STREAM *Con=NULL;
int CDDBPort=8880;
char *Tempstr=NULL, *Token=NULL, *ptr;

printf("Querying CDDB server at %s\n",Server);
Con=STREAMCreate();
ptr=Server;
if (strncmp(ptr,"http:",5)==0) ptr+=5;
if (strncmp(ptr,"cddb:",5)==0) ptr+=5;
ptr=GetToken(ptr,":",&Tempstr,0);
if (StrLen(ptr)) CDDBPort=atoi(ptr);
if (STREAMConnectToHost(Con,Tempstr,CDDBPort,0))
{
//read banner
Tempstr=STREAMReadLine(Tempstr,Con);

Token=SetStrLen(Token,HOST_NAME_MAX);
gethostname(Token,HOST_NAME_MAX);
Tempstr=MCopyStr(Tempstr,"cddb hello user ",Token," ",PROG_NAME, " ",PROG_VERSION,"\r\n",NULL);
STREAMWriteLine(Tempstr,Con);
STREAMFlush(Con);

Tempstr=STREAMReadLine(Tempstr,Con);

Tempstr=MCopyStr(Tempstr,"cddb ",Command,"\r\n",NULL);
STREAMWriteLine(Tempstr,Con);
STREAMFlush(Con);
}

DestroyString(Tempstr);
DestroyString(Token);

return(Con);
}


/* FREEDB DATA LOOKS LIKE:
210 newage b30c340c CD database entry follows (until terminating `.')
# xmcd
#
# Track frame offsets:
#        150
#        20960
#        40285
#        50642
#        69452
#        93747
#        107746
#        131372
#        152700
#        168724
#        191753
#        210746
#
# Disc length: 3126 seconds
#
# Revision: 7
# Processed by: cddbd v1.5.2PL0 Copyright (c) Steve Scherf et al.
# Submitted via: ExactAudioCopy v0.99pb5
#
DISCID=b30c340c
DTITLE=Kasabian / West Ryder Pauper Lunatic Asylum
TTITLE0=Underdog
TTITLE1=Where Did All The Love Go ?
TTITLE2=Swarfiga
TTITLE3=Fast Fuse
TTITLE4=Take Aim
TTITLE5=Thick As Thieves
TTITLE6=West Ryder Silver Bullet
TTITLE7=Vlad the Impaler
TTITLE8=Ladies And Gentlemen, Roll The Dice
TTITLE9=Secret Alphabets
TTITLE10=Fire
TTITLE11=Happiness
EXTD= YEAR: 2009
EXTT0=
EXTT1=
EXTT2=
EXTT3=
EXTT4=
EXTT5=
EXTT6=
EXTT7=
EXTT8=
EXTT9=
EXTT10=
EXTT11=
PLAYORDER=
*/


void FreeDBSendTrackOffsets(STREAM *Con, ListNode *Tracks)
{
ListNode *Curr;
TTrack *Track;
char *Tempstr=NULL;

STREAMWriteLine("# Track frame offsets:\r\n",Con);

Curr=ListGetNext(Tracks);
while (Curr)
{
Track=(TTrack *) Curr->Item;
Tempstr=FormatStr(Tempstr,"# %d\r\n",Track->FrameOffset);
STREAMWriteLine(Tempstr,Con);
Curr=ListGetNext(Curr);
}
STREAMWriteLine("#\r\n",Con);

Tempstr=FormatStr(Tempstr,"# Disc length: %d seconds\r\n",Track->FrameOffset / CD_FRAMES);
STREAMWriteLine(Tempstr,Con);
STREAMWriteLine("#\r\n",Con);

DestroyString(Tempstr);
}

void FreeDBSendTrackTitles(STREAM *Con, ListNode *Tracks)
{
ListNode *Curr;
TTrack *Track;
char *Tempstr=NULL;
int count=0;

Curr=ListGetNext(Tracks);
Track=(TTrack *) Curr->Item;
Tempstr=FormatStr(Tempstr,"# DTITLE=%s / %s\r\n",Track->Artist,Track->Album);
STREAMWriteLine(Tempstr,Con);
while (Curr)
{
	Track=(TTrack *) Curr->Item;
	Tempstr=FormatStr(Tempstr,"# TTITLE%d=%s\r\n",count,Track->Title);
	STREAMWriteLine(Tempstr,Con);
	count++;
	Curr=ListGetNext(Curr);
}

//All the following is apparently required, but can be blank
count=0;
STREAMWriteLine("EXTD=\r\n",Con);
Curr=ListGetNext(Tracks);
while (Curr)
{
	Track=(TTrack *) Curr->Item;
	Tempstr=FormatStr(Tempstr,"# EXTT%d=\r\n",count);
	STREAMWriteLine(Tempstr,Con);
	count++;
	Curr=ListGetNext(Curr);
}
STREAMWriteLine("PLAYORDER=\r\n",Con);

DestroyString(Tempstr);
}


int FreeDBSend(char *Server, ListNode *Tracks)
{
STREAM *Con;
char *Token=NULL, *Tempstr=NULL, *Line=NULL;
char *ptr;
ListNode *Curr;
TTrack *Track;
unsigned long DiskIDVal;
int trackcount;

//Calculate the string that identifies a track
trackcount=ListSize(Tracks)-1;
DiskIDVal=calc_cddb_discid(Tracks);

Tempstr=FormatStr(Tempstr,"write newage %x",DiskIDVal);
Con=FreeDBOpenCommand(Server, Tempstr);

if (Con)
{
Line=STREAMReadLine(Line,Con);
StripTrailingWhitespace(Line);
ptr=GetToken(Line,"\\S",&Tempstr,0);
if (! strcmp(Tempstr,"200")==0)
{
	printf("1 Failed to send disc info to CDDB server: %s\n",Line);	
}
else
{
	STREAMWriteLine("# xmcd\r\n",Con);
	STREAMWriteLine("#\r\n",Con);
	FreeDBSendTrackOffsets(Con,Tracks);

	Tempstr=MCopyStr(Tempstr,"# Submitted via: ",PROG_NAME,PROG_VERSION,"\r\n",NULL);
	STREAMWriteLine(Tempstr,Con);
	STREAMWriteLine("#\r\n",Con);

	Tempstr=FormatStr(Tempstr,"DISCID=%x\r\n",DiskIDVal);
	STREAMWriteLine(Tempstr,Con);
	FreeDBSendTrackTitles(Con,Tracks);


	Line=STREAMReadLine(Line,Con);
	StripTrailingWhitespace(Line);
	ptr=GetToken(Line,"\\S",&Tempstr,0);
	if (! strcmp(Tempstr,"200")==0)
	{
		printf("Failed to send disc info to CDDB server: %s\n",Line);	
	}
	else printf("CDDB Server updated: %s\n",Line);	
}
}
else printf("Not connected to CDDB server!\n");

STREAMClose(Con);

DestroyString(Token);
DestroyString(Tempstr);
DestroyString(Line);
}




int FreeDBQuery(char *Server, int DiskIDVal, char *CDDB_ID, ListNode *Tracks)
{
STREAM *Con;
int CDDBPort=8880;
char *Token=NULL, *Tempstr=NULL, *DiskID=NULL, *Line=NULL, *ArtistTitle=NULL;
char *ptr;
int trackno, RetVal=FREEDB_NOT_CONNECTED;
ListNode *Curr;
TTrack *Track;

Tempstr=MCopyStr(Tempstr,"query ",CDDB_ID,NULL);
Con=FreeDBOpenCommand(Server, Tempstr);

if (Con)
{
Line=STREAMReadLine(Line,Con);
ptr=GetToken(Line,"\\S",&Tempstr,0);
if (strcmp(Tempstr,"200")==0)
{
  ptr=GetToken(ptr,"\\S",&Tempstr,0);
  ptr=GetToken(ptr,"\\S",&DiskID,0);
  ArtistTitle=CopyStr(ArtistTitle,ptr);
  StripTrailingWhitespace(ArtistTitle);
	RetVal=FREEDB_MATCH;

	//If we've been passed a list to populate, then get the tracks
	if (Tracks)
	{
	  Line=FormatStr(Line,"cddb read %s %08x\r\n",Tempstr,DiskIDVal);
	  STREAMWriteLine(Line,Con);

		Line=STREAMReadLine(Line,Con);
		StripTrailingWhitespace(Line);
		while (StrLen(Line) && (strcmp(Line,".") !=0) )
		{
			ptr=GetToken(Line,"=",&Tempstr,0);
			if (strncmp(Tempstr,"TTITLE",6)==0)
			{
		  trackno=atoi(Tempstr+6);
 			Curr=ListGetNth(Tracks,trackno);
		  if (Curr)
		  {
			  Track=(TTrack *) Curr->Item;
			  Track->Title=CopyStr(Track->Title,ptr);
			  ptr=GetToken(ArtistTitle,"/",&Token,0);
			  Track->Artist=CopyStr(Track->Artist,Token);
			  Track->Album=CopyStr(Track->Album,ptr);
 		 	}
			}
		Line=STREAMReadLine(Line,Con);
		StripTrailingWhitespace(Line);
		}
	}


// If we got a match, write to Cache
FreeDBCacheWrite(Tracks);
}
else RetVal=FREEDB_NOT_MATCH;

STREAMClose(Con);
}
else 
{
	printf("Not connected to freedb: \n");
	RetVal=FREEDB_NOT_CONNECTED;
}

DestroyString(ArtistTitle);
DestroyString(Token);
DestroyString(Tempstr);
DestroyString(DiskID);
DestroyString(Line);

return(RetVal);
}


//Create an ID string that identifies the disk, and first look for it in
//The cache, and if that fails call 'FreeDBQuery'
int FreeDBLookup(char *Server, ListNode *Tracks, int GetTracks)
{
ListNode *Curr;
TTrack *Track;
int total_len=0, trackcount=0, RetVal;
char *Tempstr=NULL, *CDDB_ID=NULL;
unsigned long DiskIDVal;

Curr=ListGetNext(Tracks);
if (! Curr) return(FREEDB_NO_TRACKS);

DiskIDVal=calc_cddb_discid(Tracks);
CDDB_ID=FreeDBGenerateIDString(CDDB_ID,Tracks);
//Now we have the ID string, check in the Cache for it
if (! FreeDBCacheRead(DiskIDVal % NO_OF_CACHE_BUCKETS, CDDB_ID, Tracks)) 
{
	if (GetTracks) RetVal=FreeDBQuery(Server, DiskIDVal, CDDB_ID, Tracks);
	else RetVal=FreeDBQuery(Server, DiskIDVal, CDDB_ID, NULL);
}
else RetVal=FREEDB_MATCH;

DestroyString(CDDB_ID);
DestroyString(Tempstr);
}

