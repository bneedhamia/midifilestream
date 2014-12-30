# MidiFileStream

## Introduction

MidiFileStream is an Arduino library that reads MIDI events from a MIDI file, in sequence.

I wrote the library as part of my Robotic Glockenspiel project, at https://github.com/bneedhamia/glockenspiel  I needed a library to read events from a MIDI file, with a control flow that would allow my sketch to play notes, including simultaneous notes, with the correct timing.

## Installation

To install on Windows:

1. Run git bash (or whatever git client you like)
1. cd Documents/Arduino/libraries
1. git clone https://github.com/bneedhamia/midifilestream.git MidiFileStream
1. restart your Arduino IDE

To install on Linux:

1. cd sketchbook/libraries
1. git clone https://github.com/bneedhamia/midifilestream.git MidiFileStream
1. restart your Arduino IDE

## To use

See **examples/MidiDump** for a simple example Sketch that reads and prints the events in a MIDI file.

See https://github.com/bneedhamia/glockenspiel for a more complex sketch, that plays a MIDI file on a set of chimes.

The basic flow of reading a MIDI file is:

    MidiFileStream midiFile;
    File file;
    
    SD.begin(pinSelectSD);
    file = SD.open(fName, FILE_READ);
    midiFile.begin(file);
    
    midiFile.openChunk();

    ...in loop()...
    event_t eventType = midiFile.readEvent();
    if the eventType is End of Track, open the next track (if there is one).
    if the eventType is an event you want to handle, handle it.
    when you reach end of file, call midiFile.end() and file.close().

# Timing

MidiFileStream delivers events in order and immediately.  If your sketch needs to play events at the correct times, it must keep track of time and the delays associated with each event. Doing so is a complex act, involving keeping track of tempo changes, multiple simultaneous events, and the total delay from the start of the track to the current event. See https://github.com/bneedhamia/glockenspiel for a complete example of timing.
