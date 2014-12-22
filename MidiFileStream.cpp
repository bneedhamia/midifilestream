/*
 * MIDI File reader.
 * 
 * Copyright (c) 2014 Bradford Needham
 * (@bneedhamia, https://www.needhamia.com)
 * Licensed under the LGPL version 2.1
 * a version of which should be supplied with this file.
 */

#include <MidiFileStream.h>
//#define MIDIFILESTREAM_DEBUG 1
//#define MIDIFILESTREAM_VERBOSE 1

/*
 * Returns the Midi file format:
 * 0 = single track
 * 1 = simultaneously-played tracks
 *   (or more often, 1 header track and 1 real track)
 * 2 = sequentially-played tracks.
 */
int MidiFileStream::getFormat() {
  return _format;
}

/*
 * Returns the number of tracks in the file,
 * as reported by the file header.
 */
int MidiFileStream::getNumTracks() {
  return _numTracks;
}

/*
 * Returns the ticks per beat, from the Midi header.
 */
int MidiFileStream::getTicksPerBeat() {
  return _ticksPerBeat;
}

/*
 * Returns the number of bytes remaining to be read
 * from the current chunk
 */
long MidiFileStream::getChunkBytesLeft() {
  return _bytesLeft;
}

/*
 * Returns the number of ticks between the previous event
 * and the current event
 */
long MidiFileStream::getEventDeltaTicks() {
  return _eventDeltaTicks;
}

/*
 * Returns the type of the current event.
 * See ET_*.
 */
event_t MidiFileStream::getEventType() {
  return _eventType;
}

/*
 * Returns a pointer to the data of the current event.
 * To Use:
 *  Suppose the event type is ET_KEY_SIGN.
 *  The corresponding _eventData field is keySign.
 *  So, to access the numSharps field of the event:
 *    int nSharps = midiFile.getEventDataP()->keySign.numSharps;
 */
union eventData *MidiFileStream::getEventDataP() {
  return &_eventData;
}

/*
 * Initialize based on the given, open Midi file stream,
 * and read the Midi file header chunk from that stream.
 * Returns true if successful; false if not successful.
 */
boolean MidiFileStream::begin(Stream& stream) { 
  _pStream = &stream;
  _bytesLeft = -1;
  _format = -1;
  _numTracks = -1;
  _ticksPerBeat = 0;
  _runningStatus = 0;
  _eventType = ET_UNK;
  _eventDeltaTicks = -1;

  chunk_t chunkType;
 
  // Open the header chunk
  chunkType = openChunk();
  if (chunkType != CT_MTHD) {
#ifdef MIDIFILESTREAM_DEBUG
    Serial.print("Expected chunk type CT_MTHD, instead read type ");
    Serial.println((int) chunkType);
#endif
    return false;
  }
  if (_bytesLeft != 6) {
#ifdef MIDIFILESTREAM_DEBUG
    Serial.print("Expected chunk length of 6, instead read ");
    Serial.println(_bytesLeft);
#endif
    return false;
  }

  _format = (int) readFixedLong(2);
  if (_format < 0) {
#ifdef MIDIFILESTREAM_DEBUG
    Serial.println("Error reading header format");
#endif
    return false;
  }
#ifdef MIDIFILESTREAM_VERBOSE
  Serial.print("Format type: ");
  Serial.println(_format);
#endif

  _numTracks = (int) readFixedLong(2);
  if (_numTracks < 0) {
#ifdef MIDIFILESTREAM_DEBUG
    Serial.println("Error reading header number of tracks");
#endif
    return false;
  }
#ifdef MIDIFILESTREAM_VERBOSE
  Serial.print("Number of tracks: ");
  Serial.println(_numTracks);
#endif

  _ticksPerBeat = (int) readFixedLong(2);
  if (_ticksPerBeat < 0) {
#ifdef MIDIFILESTREAM_DEBUG
    Serial.println("Unsupported SMPTE frames-per-second format.");
#endif
    return false;
  }
#ifdef MIDIFILESTREAM_VERBOSE
  Serial.print("Ticks per Beat = ");
  Serial.println(_ticksPerBeat);
#endif
  
  if (_bytesLeft > 0) {
#ifdef MIDIFILESTREAM_DEBUG
    Serial.print("INTERNAL ERROR: header has ");
    Serial.print(_bytesLeft);
    Serial.println(" too many bytes.");
#endif
    return false;
  }

  return true;
}

/*
 * Closes the object.
 * Note: the caller is responsible for closing
 * the corresponding Midi Stream.
 */
void MidiFileStream::end() {
  _pStream = 0;
  _bytesLeft = -1;
  _format = -1;
  _numTracks = -1;
  _ticksPerBeat = 0;
  _runningStatus = 0;
  _eventType = ET_UNK;
  _eventDeltaTicks = -1;
 
}

/*
 * Reads the chunk type and (if the chunk exists) its length.
 * sets the length of the chunk, _bytesLeft.
 * Returns the chunk type. See CT_*.
 */
chunk_t MidiFileStream::openChunk() {
  const char mthd[] = { 'M', 'T', 'h', 'd' };
  const char mtrk[] = { 'M', 'T', 'r', 'k' };
  char sig[4]; // chunk signature
  int bint;
  int i;

  _bytesLeft = -1;
  _runningStatus = 0;
  _eventType = ET_UNK;
  _eventDeltaTicks = -1;
  
  // Read the chunk signature
  for (i = 0; i < 4; ++i) {
    bint = _pStream->read();
    sig[i] = (char) bint;
    if (bint < 0) {
      if (i == 0) {
        return CT_END;
      } else {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("End of file reading a chunk header.");
#endif
        return CT_UNK;
      }
    }
  }
  
  // Read chunk length
  _bytesLeft = 4;  // hack so readFixedLong() can work.
  _bytesLeft = readFixedLong(4);
  if (_bytesLeft < 0) {
#ifdef MIDIFILESTREAM_DEBUG
    Serial.println("Error reading chunk length.");
#endif
    return CT_UNK;
  }

  // See if it's a header chunk
  for (i = 0; i < 4; ++i) {
    if (sig[i] != mthd[i]) {
      break;
    }
  }
  if (i == 4) {
    return CT_MTHD;
  }
  
  // See if it's a track chunk
  for (i = 0; i < 4; ++i) {
    if (sig[i] != mtrk[i]) {
      break;
    }
  }
  if (i == 4) {
    return CT_MTRK;
  }
  
  // Unknown chunk type.
#ifdef MIDIFILESTREAM_DEBUG
  Serial.print("Unknown chunk signature:");
  for (i = 0; i < 4; ++i) {
    Serial.print(" ");
    Serial.print(((int) sig[i]) & 0xFF, HEX);
  }
  Serial.println();
#endif
  return CT_UNK;
  
}

/*
 * Read the next event in the track (if any).
 * Sets _eventType, _eventDeltaTicks, and _eventData.
 * Returns ET_UNK for an error,
 * ET_END for the end of track data (not an End of Track event),
 * or an ET_* value.
 */
event_t MidiFileStream::readEvent() {
  int bint;
  int metaType;  // Midi byte that identifies the meta event type
  long length;
  long l;
  
  _eventType = ET_UNK;

  _eventDeltaTicks = readVariableLong();
  if (_eventDeltaTicks < 0) {
    _eventType = ET_END;
    return _eventType;  // normal end of track reached (or an error).
  }

#ifdef MIDIFILESTREAM_VERBOSE  
  Serial.print(_eventDeltaTicks);
  Serial.print("T ");
#endif
  
  // The first byte tells what event type it is.
  bint = readChunkByte();
  if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
    Serial.println("Error reading event type byte.");
#endif
    _eventType = ET_UNK;
    return _eventType;
  }
  
  if ((bint == 0xF0) || (bint == 0xF7)) {
    // A Sysex event of one type or another.
    // That clears running status.
    _runningStatus = 0;
    
    length = readVariableLong();
    if (length < 0) {
#ifdef MIDIFILESTREAM_DEBUG
      Serial.println("Error reading Sysex F0 or Sysex Esc data length");
#endif
      _eventType = ET_UNK;
      return _eventType;
    }

    if (bint == 0xF0) {
      _eventType = ET_SYSEX_F0;

      _eventData.sysexF0.length = readVariableBytes(length, &_eventData.sysexF0.bytes[0]);
      if (_eventData.sysexF0.length < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Sysex F0 data");
#endif
        _eventType = ET_UNK;
        return _eventType;
      }
      
#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.print("Sysex F0, length: ");
      Serial.println(_eventData.sysexF0.length);
#endif
      
    } else if (bint == 0xF7) {
      _eventType = ET_SYSEX_ESC;

      _eventData.sysexEsc.length = readVariableBytes(length, &_eventData.sysexEsc.bytes[0]);
      if (_eventData.sysexEsc.length < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Sysex Esc data");
#endif
        _eventType = ET_UNK;
        return _eventType;
      }

#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.print("Sysex F7, length: ");
      Serial.println(_eventData.sysexF0.length);
#endif
    }
    
    return _eventType;
    
  }
  
  if (bint == 0xFF) {
    // A Meta event.
    // That clears running status;
    _runningStatus = 0;

    metaType = readChunkByte();
    if (metaType < 0) {
#ifdef MIDIFILESTREAM_DEBUG
      Serial.println("End of track reading meta event type.");
#endif
       _eventType = ET_UNK;
      return _eventType;
     }
             
    length = readVariableLong();
    if (length < 0) {
#ifdef MIDIFILESTREAM_DEBUG
      Serial.println("Error reading Meta Event data length");
#endif
      _eventType = ET_UNK;
      return _eventType;
    }
    
    switch ((char) metaType) {
      
    case (char) 0x00:  // Sequence Number
      _eventType = ET_SEQ_NUM;
      
      if (length != 2) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.print("Garbled Meta SeqNum length: ");
        Serial.println(length);
#endif
        _eventType = ET_UNK;
        return _eventType;
      }
      
      l = readFixedLong(2);
      if (l < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Meta Sequence Number");
#endif
        _eventType = ET_UNK;
        return _eventType;
      }
      _eventData.seqNum.number = (int) l;

#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.print("Meta SeqNum ");
      Serial.println(_eventData.seqNum.number);
#endif
      break;
    
    case (char) 0x01: // Text
      _eventType = ET_TEXT;
    
      _eventData.text.length = readVariableBytes(length, &_eventData.text.bytes[0]);
      if (_eventData.text.length < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Meta Text data");
#endif
        _eventType = ET_UNK;
        return _eventType;
      }

#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.print("Text: ");
      Serial.println(_eventData.text.bytes);
#endif
      break;
    
    case (char) 0x02: // Copyright
      _eventType = ET_COPYRIGHT;
    
      _eventData.copyright.length = readVariableBytes(length, &_eventData.copyright.bytes[0]);
      if (_eventData.copyright.length < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Meta Copyright data");
#endif
        _eventType = ET_UNK;
        return _eventType;
      }

#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.print("Copyright: ");
      Serial.println(_eventData.copyright.bytes);
#endif
      break;
      
    case (char) 0x03: // Sequence/Track name
      _eventType = ET_NAME;
    
      _eventData.name.length = readVariableBytes(length, &_eventData.name.bytes[0]);
      if (_eventData.name.length < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Meta Seq/Trk data");
#endif
        _eventType = ET_UNK;
        return _eventType;
      }

#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.print("Seq/Track Name: ");
      Serial.println(_eventData.name.bytes);
#endif
      break;
    
    case (char) 0x04: // Instrument name
      _eventType = ET_INSTRUMENT;

      _eventData.instrument.length = readVariableBytes(length, &_eventData.instrument.bytes[0]);
      if (_eventData.instrument.length < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Meta Instrument Name data");
#endif
        _eventType = ET_UNK;
        return _eventType;
      }

#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.print("Instrument: ");
      Serial.println(_eventData.instrument.bytes);
#endif
      break;
 
    case (char) 0x05: // Lyric
      _eventType = ET_LYRIC;

      _eventData.lyric.length = readVariableBytes(length, &_eventData.lyric.bytes[0]);
      if (_eventData.lyric.length < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Meta Lyric data");
#endif
        _eventType = ET_UNK;
        return _eventType;
      }

#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.print("Lyric: ");
      Serial.println(_eventData.lyric.bytes);
#endif
      break;
 
    case (char) 0x06: // Marker
      _eventType = ET_MARKER;
    
      _eventData.marker.length = readVariableBytes(length, &_eventData.marker.bytes[0]);
      if (_eventData.marker.length < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Meta Marker data");
#endif
        _eventType = ET_UNK;
        return _eventType;
      }

#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.print("Marker: ");
      Serial.println(_eventData.marker.bytes);
#endif
      break;
 
    case (char) 0x07: // Cue Point
      _eventType = ET_CUE;

      _eventData.cue.length = readVariableBytes(length, &_eventData.cue.bytes[0]);
      if (_eventData.cue.length < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Meta Cue data");
#endif
        _eventType = ET_UNK;
        return _eventType;
      }

#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.print("Cue: ");
      Serial.println(_eventData.cue.bytes);
#endif
      break;
    
    case (char) 0x20: // Midi Channel Prefix
      _eventType = ET_CHAN_PREFIX;
    
      if (length != 1) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.print("Garbled Meta Chan Prefix length: ");
        Serial.println(length);
#endif
        _eventType = ET_UNK;
        return _eventType;
      }
      
      bint = readChunkByte();
      if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Meta Chan Prefix");
#endif
        _eventType = ET_UNK;
        return _eventType;
      }
      _eventData.chanPrefix.chan = (int) bint;

#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.print("Channel Prefix: ");
      Serial.println(_eventData.chanPrefix.chan);
#endif
      break;
      
    case (char) 0x2F: // End of Track
      _eventType = ET_END_TRACK;

      if (length != 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.print("Garbled Meta End of Track length: ");
        Serial.println(length);
#endif
        _eventType = ET_UNK;
        return _eventType;
      }
      // There's no data to read.

#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.println("End of Track event");
#endif
      break;
    
    case (char) 0x51: // Set Tempo
      _eventType = ET_TEMPO;

      if (length != 3) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.print("Garbled Meta Tempo length: ");
        Serial.println(length);
#endif
        _eventType = ET_UNK;
        return _eventType;
      }
      
      _eventData.tempo.uSecPerBeat = readFixedLong(3);
      if (_eventData.tempo.uSecPerBeat < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Meta Tempo data");
#endif
        _eventType = ET_UNK;
        return _eventType;
      }
      
#ifdef MIDIFILESTREAM_VERBOSE
      Serial.print("Tempo. uSecPerBeat: ");
      Serial.println(_eventData.tempo.uSecPerBeat);
#endif
      break;
      
    case (char) 0x54: // SMPTE Offset
      _eventType = ET_SMPTE_OFFSET;

      if (length != 5) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.print("Garbled Meta SMPTE Offset length: ");
        Serial.println(length);
#endif
        _eventType = ET_UNK;
        return _eventType;
      }
      
      bint = readChunkByte();
      if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error SMPTE Offset");
#endif
        _eventType = ET_UNK;
      }
      _eventData.smpteOffset.hours = (int) bint;
      
      bint = readChunkByte();
      if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error SMPTE Offset");
#endif
        _eventType = ET_UNK;
      }
      _eventData.smpteOffset.minutes = (int) bint;
      
      bint = readChunkByte();
      if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error SMPTE Offset");
#endif
        _eventType = ET_UNK;
      }
      _eventData.smpteOffset.seconds = (int) bint;
      
      bint = readChunkByte();
      if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error SMPTE Offset");
#endif
        _eventType = ET_UNK;
      }
      _eventData.smpteOffset.frames = (int) bint;
      
      bint = readChunkByte();
      if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error SMPTE Offset");
#endif
        _eventType = ET_UNK;
      }
      _eventData.smpteOffset.f100ths = (int) bint;
      
#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.println("SMPTE Offset");
#endif
      break;
      
    case (char) 0x58: // Time Signature
      _eventType = ET_TIME_SIGN;

      if (length != 4) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.print("Garbled Meta Time Signature length: ");
        Serial.println(length);
#endif
        _eventType = ET_UNK;
        return _eventType;
      }
      
      bint = readChunkByte();
      if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Time Signature");
#endif
        _eventType = ET_UNK;
      }
      _eventData.timeSign.numer = (int) bint;
      
      // Denominator is expressed as a power of 2.
      bint = readChunkByte();
      if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Time Signature");
#endif
        _eventType = ET_UNK;
      }
      _eventData.timeSign.denom = 1 << bint;
      
      bint = readChunkByte();
      if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Time Signature");
#endif
        _eventType = ET_UNK;
      }
      _eventData.timeSign.metro = (int) bint;
      
      bint = readChunkByte();
      if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Time Signature");
#endif
        _eventType = ET_UNK;
      }
      _eventData.timeSign.m32nds = (int) bint;

#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.print("Time Sign: ");
      Serial.print(_eventData.timeSign.numer);
      Serial.print(", ");
      Serial.print(_eventData.timeSign.denom);
      Serial.print(", ");
      Serial.print(_eventData.timeSign.metro);
      Serial.print(", ");
      Serial.println(_eventData.timeSign.m32nds);
#endif
      break;
      
    case (char) 0x59: // Key Signature
      _eventType = ET_KEY_SIGN;

      if (length != 2) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.print("Garbled Meta Key Signature length: ");
        Serial.println(length);
#endif
        _eventType = ET_UNK;
        return _eventType;
      }
      
      bint = readChunkByte();
      if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Key Signature");
#endif
        _eventType = ET_UNK;
      }
      // Sign-extend the byte
      _eventData.keySign.numSharps = (int) bint;
      if ((_eventData.keySign.numSharps & 0x80) != 0) {
        _eventData.keySign.numSharps |= -0x80;
      }
            
      bint = readChunkByte();
      if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
        Serial.println("Error reading Key Signature");
#endif
        _eventType = ET_UNK;
      }
      _eventData.keySign.isMinor = (int) bint;

#ifdef MIDIFILESTREAM_VERBOSE
      Serial.print("Key Sign: ");
      Serial.print(_eventData.keySign.numSharps);
      Serial.print(" sharps, ");
      Serial.println(_eventData.keySign.isMinor ? "minor" : "major");
#endif
      break;
      
    default: // Unknown Meta event.
      _eventType = ET_NO_OP; // no event.
      
      long bytesLeft;
      for (bytesLeft = length; bytesLeft > 0; --bytesLeft) {
        bint = readChunkByte();
        if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
          Serial.println("Error reading unknown Meta event data.");
#endif
          _eventType = ET_UNK;
          return _eventType;
        }
      }

#ifdef MIDIFILESTREAM_VERBOSE 
      Serial.print("Unknown Meta, length: ");
      Serial.println(length);
#endif
    };
    
    return _eventType;  // returns the Meta event

  }
  
  /*
   * A Channel event.
   * If the first byte top bit is not set,
   * the event uses _runningStatus as the first byte.
   */
  _eventType = ET_CHANNEL;

  int runningBint = -1;
  if ((bint & 0x80) == 0) {
    runningBint = bint;  // sort of a pushdown stack.
    bint = _runningStatus;
    if ((bint & 0x80) == 0) {
#ifdef MIDIFILESTREAM_DEBUG
      Serial.println("Running status used, but not active.");
#endif
      _eventType = ET_UNK;
      return _eventType;
    }
  }
  _eventData.channel.code = (char) (bint >> 4);
  _eventData.channel.chan = bint & 0x0F;
 
  _runningStatus = bint;
  
  if (runningBint != -1) {
    _eventData.channel.param1 = runningBint;
  } else {
    bint = readChunkByte();
    if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
      Serial.println("End of track reading channel event parameter 1.");
#endif
      _eventType = ET_UNK;
      return _eventType;
    }
    _eventData.channel.param1 = bint;
  }

  // Some channel events don't have a second parameter
  if (_eventData.channel.code == 0xC || _eventData.channel.code == 0xD) {
    _eventData.channel.param2 = 0;
  } else {
    bint = readChunkByte();
    if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
      Serial.println("End of track reading channel event parameter 2.");
#endif
      _eventType = ET_UNK;
      return _eventType;
    }
    _eventData.channel.param2 = bint;
  }

#ifdef MIDIFILESTREAM_VERBOSE
  Serial.print("Chan code 0x");
  Serial.print(_eventData.channel.code, HEX);
  Serial.print(" [");
  Serial.print(_eventData.channel.chan);
  Serial.print("] ");
  Serial.print(_eventData.channel.param1);
  Serial.print(", ");
  Serial.println(_eventData.channel.param2);
#endif
  
  return _eventType;
  
}

/*
 * Read a fixed-length integer from the current chunk in the midi file.
 * returns the number, or -1 for failure.
 * Also maintains the chunk bytes left.
 */
long MidiFileStream::readFixedLong(int numBytes) {
  long result;
  int bint;
  int i;
 
  result = 0;
  for (i = 0; i < numBytes; ++i) {
    
    bint = readChunkByte();
    if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
      Serial.println("End of chunk reading a fixed-length number. Expected ");
      Serial.print(numBytes);
      Serial.println(" bytes.");
#endif
      return -1;
    }
    
    result <<= 8;
    result |= bint;
  }

  return result;
}

/*
 * Reads the data for an event that has a variable number of bytes
 * of data.
 *   length = the number of data bytes in the event.
 *   pBuffer = pointer to the buffer to store the data into.
 *     We assume this buffer is [EV_BUFFER_SIZE] long.
 * Truncates the length and data to EV_BUFFER_SIZE - 1 bytes.
 * Adds a null terminator to the end of the data.
 * Returns the truncated length if successful;
 * -1 if an error occurs.
 */
long MidiFileStream::readVariableBytes(long length, char *pBuffer) {
 long truncLength; // truncated length; the stored number of data bytes.
  long bytesRead; // number of data bytes read so far.
  char *cp;
  int bint;
  
  pBuffer[0] = '\0';
  
  truncLength = length;
  if (length > EV_BUFFER_SIZE - 1) {
    truncLength = EV_BUFFER_SIZE - 1;
  }
  
  // Read the truncated data into the buffer.
  cp = pBuffer;
  for (bytesRead = 0; bytesRead < truncLength; ++bytesRead) {
    bint = readChunkByte();
    if (bint < 0) {
#ifdef MIDIFILESTREAM_DEBUG
      Serial.print("Error reading variable byte[");
      Serial.print(bytesRead);
      Serial.println("]");
#endif
      return -1;
    }
    *cp++ = (char) bint;
  }
  *cp++ = '\0';
  
  // Skip the remaining data.
  for ( ; bytesRead < length; ++bytesRead) {
    if (readChunkByte() < 0) {
#ifdef MIDIFILESTREAM_DEBUG
      Serial.print("Error skipping variable byte[");
      Serial.print(bytesRead);
      Serial.println("]");
#endif
      return -1;
    }
  }
  
  return truncLength;
  
}
 
/*
 * Reads a variable-length, non-negative number.
 * Returns the number on success, or -1 on failure.
 */
long MidiFileStream::readVariableLong() {
  int bint;
  long result;
  int anotherByte; // if true, there's at least one more byte to read.
  int numBytes;    // number of bytes in the variable number.
  
 result = 0;
  numBytes = 0;
  anotherByte = true;
  while (anotherByte) {
    if (numBytes > 4) {
#ifdef MIDIFILESTREAM_DEBUG
      Serial.println("Corrupted file: variable number not terminated after 4 bytes.");
#endif
      return -1;
    }
    bint = readChunkByte();
    if (bint < 0) {
      // If we're reading a delta tick number, end of chunk could be normal.
      return -1;
    }
    ++numBytes;
    anotherByte = ((bint & 0x80) == 0x80);
    result <<= 7;
    result |= (bint & 0x7F);
  }
 
  return result;
}

/*
 * Reads one byte from the current chunk,
 * returning the byte as an integer
 * or -1 if at end of chunk.
 */
int MidiFileStream::readChunkByte() {
  if (_bytesLeft <= 0) {
    return -1;
  }
  
  --_bytesLeft;
  return _pStream->read();
}
