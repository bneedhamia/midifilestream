#ifndef MidiFileStream_h
#define MidiFileStream_h

#include <Arduino.h>

#define MidiFileStream_Version 0x00010000L
#define MidiFileStream_sVersion "1.0.0"

/*
 * Stream-based MIDI File reading library.
 * Version 1.0
 * December 10, 2014
 *
 * Copyright (c) 2014 Bradford Needham
 * (@bneedhamia, https://www.needhamia.com)
 * Licensed under the LGPL version 2.1
 * a version of which should be supplied with this file.
 *
 * The library supports two #defines:
 *  #define MIDIFILESTREAM_DEBUG 1 to print file format error messages.
 *  #define MIDIFILESTREAM_VERBOSE 1 to print each event as it is read.
 *
 * If you're looking for a more callback-oriented library,
 * you may be interested in:
 * The Standard MIDI Files (SMF) Processing Library,
 * by codeplex user 8136821
 * http://arduinocode.codeplex.com/releases/view/115256
 *
 * References:
 * http://cs.fit.edu/~ryan/cse4051/projects/midi/midi.html
 * http://www.sonicspot.com/guide/midifiles.html
 * http://www.cs.cmu.edu/~music/cmsip/readings/MIDI%20tutorial%20for%20programmers.html
 */

/*
 * File chunk types:
 * CT_UNK = Unknown/unset chunk type.
 * CT_END = end of file.  That is, no chunk follows.
 * CT_MTHD = the file header.
 * CT_MTRK = a track of events.
 *
 * A note on memory used: I did a simple test of enum vs. const
 * and found sizeof(chunkType) == 1 and sizeof(enum of a few values) = 2.
 * So I won't be using enums any time soon, for space reasons.
 */
typedef byte chunk_t;
const chunk_t CT_UNK = (chunk_t) 0;
const chunk_t CT_END = (chunk_t) 1;
const chunk_t CT_MTHD = (chunk_t) 100;
const chunk_t CT_MTRK = (chunk_t) 101;

/*
 * Midi Event types.
 * Note: these are internal type values; their values do not match those of midi event codes.
 */
typedef byte event_t;
const event_t ET_UNK = (event_t) 0;          // Unknown/Error event
const event_t ET_NO_OP = (event_t) 1;        // No operation - an event to skip.
const event_t ET_END = (event_t) 2;          // End of the current track
const event_t ET_SYSEX_F0 = (event_t) 3;     // Sysex F0 (arbitrary data) message
const event_t ET_SYSEX_ESC = (event_t) 4;    // Sysex F7 (Escaped data) message
const event_t ET_SEQ_NUM = (event_t) 5;      // Meta sequence number event.
const event_t ET_TEXT = (event_t) 6;         // Meta Text event (e.g., "From the back of the house")
const event_t ET_COPYRIGHT = (event_t) 7;    // Meta Copyright event
const event_t ET_NAME = (event_t) 8;         // Meta Sequence or Track Name event
const event_t ET_INSTRUMENT = (event_t) 9;   // Meta Instrument Name event
const event_t ET_LYRIC = (event_t) 10;       // Meta Lyric event
const event_t ET_MARKER = (event_t) 11;      // Meta Marker event (e.g., "rehearsal A")
const event_t ET_CUE = (event_t) 12;         // Meta Cue Point event (e.g., "H")
const event_t ET_CHAN_PREFIX = (event_t) 13; // Meta Channel Prefix event
const event_t ET_END_TRACK = (event_t) 14;   // Meta End of Track event (there is no data in this event)
const event_t ET_TEMPO = (event_t) 15;       // Meta Tempo event
const event_t ET_SMPTE_OFFSET = (event_t) 16;// Meta SMPTE offset event
const event_t ET_TIME_SIGN = (event_t) 17;   // Meta Time Signature event
const event_t ET_KEY_SIGN = (event_t) 18;    // Meta Key Signature event
const event_t ET_CHANNEL = (event_t) 19;     // A channel message.  See CH_* below.

/*
 * Values for dataChannel.code.
 * These values do match the corresponding MIDI codes.
 */
const char CH_NOTE_OFF = (char) 0x8;        // Channel Note Off code
const char CH_NOTE_ON = (char) 0x09;        // Channel Note On code
const char CH_NOTE_AFTERTOUCH = (char) 0xA; // Channel Note Aftertouch code
const char CH_CONTROLLER = (char) 0xB;      // Channel Controller code
const char CH_PROG_CHANGE = (char) 0xC;     // Channel Program Change code
const char CH_CHAN_AFTERTOUCH = (char) 0xD; // Channel Channel Aftertouch code
const char CH_PITCH_BEND = (char) 0xE;      // Channel Pitch Bend code

/*
 * Size (bytes) of all of the character buffers in ET_* value structures.
 * +1 to account for an always-there null terminator.
 * The maximum number of bytes that can be put in such a buffer,
 * not counting the terminating null, is (EV_BUFFER_SIZE - 1).
 * Fields that are larger than this in the file are truncated to this length.
 */
const int EV_BUFFER_SIZE = (140 + 1);

/*
 * Data from an ET_SYSEX_F0 event.
 *  length = the number of bytes in the bytes[],
 *   not counting the terminating null.
 *  bytes[] = the data bytes.  Can contain arbirary values, such as null.
 */
struct dataSysexF0 {
  int length;
  char bytes[EV_BUFFER_SIZE];
};

/*
 * Data from an ET_SYSEX_ESC event.
 *  length = the number of bytes in the bytes[], not counting the terminating null.
 *  bytes[] = the data bytes.  Can contain arbirary values, such as null.
 */
struct dataSysexEsc {
  int length;
  char bytes[EV_BUFFER_SIZE];
};

/*
 * Data from an ET_SEQ_NUM event.
 *  number = sequence number of this track.
 */
struct dataSeqNum {
  int number;
};

/*
 * Data from an ET_TEXT event.
 *  length = the number of bytes in the bytes[], not counting the terminating null.
 *  bytes[] = the data bytes.  Can contain arbirary values, such as null.
 */
struct dataText {
  int length;
  char bytes[EV_BUFFER_SIZE];
};

/*
 * Data from an ET_COPYRIGHT event.
 *  length = the number of bytes in the bytes[], not counting the terminating null.
 *  bytes[] = the data bytes.  Can contain arbirary values, such as null.
 */
struct dataCopyright {
  int length;
  char bytes[EV_BUFFER_SIZE];
};

/*
 * Data from an ET_NAME event.
 *  length = the number of bytes in the bytes[], not counting the terminating null.
 *  bytes[] = the data bytes.  Can contain arbirary values, such as null.
 */
struct dataName {
  int length;
  char bytes[EV_BUFFER_SIZE];
};

/*
 * Data from an ET_INSTRUMENT event.
 *  length = the number of bytes in the bytes[], not counting the terminating null.
 *  bytes[] = the data bytes.  Can contain arbirary values, such as null.
 */
struct dataInstrument {
  int length;
  char bytes[EV_BUFFER_SIZE];
};

/*
 * Data from an ET_LYRIC event.
 *  length = the number of bytes in the bytes[], not counting the terminating null.
 *  bytes[] = the data bytes.  Can contain arbirary values, such as null.
 */
struct dataLyric {
  int length;
  char bytes[EV_BUFFER_SIZE];
};

/*
 * Data from an ET_MARKER event.
 *  length = the number of bytes in the bytes[], not counting the terminating null.
 *  bytes[] = the data bytes.  Can contain arbirary values, such as null.
 */
struct dataMarker {
  int length;
  char bytes[EV_BUFFER_SIZE];
};

/*
 * Data from an ET_CUE event.
 *  length = the number of bytes in the bytes[], not counting the terminating null.
 *  bytes[] = the data bytes.  Can contain arbirary values, such as null.
 */
struct dataCue {
  int length;
  char bytes[EV_BUFFER_SIZE];
};

/*
 * Data from an ET_CHAN_PREFIX event.
 *  chan = the channel
 */
struct dataChanPrefix {
  int chan;
};

/*
 * Data from an ET_TEMPO event.
 *  uSecPerBeat = the new tempo, in microseconds per beat.
 */
struct dataTempo {
  long uSecPerBeat;
};

/*
 * Data from an ET_SMPTE_OFFSET event.
 *  hours, minutes, seconds, frames, and 100ths of a frame
 *  SMPTE time when the track is to start.
 */
struct dataSmpteOffset {
  int hours;
  int minutes;
  int seconds;
  int frames;
  int f100ths;
};

/*
 * Data from an ET_TIME_SIGN event.
 *  numer = numerator. That is, musical beats per measure.
 *  denom = denominator. That is, musical duration that gets the beat
 *  metro = Midi clocks per metronome sounding.
 *  m32nds = number of 32nd notes per 24 Midi clocks.
 */
struct dataTimeSign {
  int numer;
  int denom;
  int metro;
  int m32nds;
};

/*
 * Data from an ET_KEY_SIGN event.
 *  numSharps = number of sharps;
 *    negative numbers are the number of flats;
 *    0 = no sharps or flats. That is, the key of C major or A minor.
 *  isMinor = if non-zero, Minor key; if 0, Major key
 */
struct dataKeySign {
  int numSharps;
  int isMinor;
};

/*
 * Data from an ET_CHANNEL event.
 *  code = the specific channel code. See CH_* above.
 *  chan = the Midi channel affected (0..15)
 *  param1 = the first parameter
 *  param2 = the second parameter, or 0  if this event has no 2nd parameter
 */
struct dataChannel {
  char code;
  int chan;
  int param1;
  int param2;
};

/*
 * The data of one event.
 * the field name corresponds to the event type.
 * E.g., ET_CHANNEL's data is eventData.channel
 */
union eventData {
  struct dataSysexF0 sysexF0;
  struct dataSysexEsc sysexEsc;
  struct dataSeqNum seqNum;
  struct dataText text;
  struct dataCopyright copyright;
  struct dataName name;
  struct dataInstrument instrument;
  struct dataLyric lyric;
  struct dataMarker marker;
  struct dataCue cue;
  struct dataChanPrefix chanPrefix;
  struct dataTempo tempo;
  struct dataSmpteOffset smpteOffset;
  struct dataTimeSign timeSign;
  struct dataKeySign keySign;
  struct dataChannel channel;
};

class MidiFileStream {
  private:
    Stream *_pStream;  // the underlying Midi file stream
    
    long _bytesLeft;   // bytes remaining to be read in the current chunk.
    
    int _format;        // file format from header: 0, 1, or 2, from the header chunk.
    int _numTracks;     // number of tracks, from the header chunk.
    int _ticksPerBeat;  // tempo, from the header chunk.
    
    int _runningStatus; // if non-zero, the status byte (event byte) of the previous event.
    
    event_t _eventType; // type of the current event, or ET_UNK if none. See ET_* above.
    long _eventDeltaTicks; // number of ticks delay between the previous event and this one.
    union eventData _eventData; // data for the current event
    
  public:
    boolean begin(Stream& stream);
    void end();
    chunk_t openChunk();
    event_t readEvent();
    long getChunkBytesLeft();
    
    int getFormat();
    int getNumTracks();
    int getTicksPerBeat();
    
    event_t getEventType();
    long getEventDeltaTicks();
    union eventData *getEventDataP();
    
    long readVariableBytes(long, char *pBuffer);
    long readFixedLong(int numBytes);
    long readVariableLong();
    int readChunkByte();
    
};
#endif
