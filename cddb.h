#ifndef CD_COMMAND_CDDB_H
#define CD_COMMAND_CDDB_H

#include "common.h"

#define FREEDB_MATCH 0
#define FREEDB_NOT_MATCH 1
#define FREEDB_NOT_CONNECTED 2
#define FREEDB_NO_TRACKS 3



char *FreeDBGenerateIDString(char *RetStr, ListNode *Tracks);
void FreeDBCacheReadTrackDetails(STREAM *S, ListNode *Tracks);
int FreeDBCacheRead(int Bucket, char *ID, ListNode *Tracks);
void FreeDBCacheWrite(ListNode *Tracks);
STREAM *FreeDBOpenCommand(char *Server, char *Command);
void FreeDBSendTrackOffsets(STREAM *Con, ListNode *Tracks);
void FreeDBSendTrackTitles(STREAM *Con, ListNode *Tracks);
int FreeDBSend(char *Server, ListNode *Tracks);
int FreeDBQuery(char *Server, int DiskIDVal, char *CDDB_ID, ListNode *Tracks);
int FreeDBLookup(char *Server, ListNode *Tracks, int GetTracks);

#endif
