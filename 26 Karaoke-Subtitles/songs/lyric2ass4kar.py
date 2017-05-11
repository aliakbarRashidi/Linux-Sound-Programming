#!/usr/bin/python


import fileinput
import string
import math

TEXT_STR = "Dialogue: 0,%s,%s,Default,,0000,0000,0000,,"

textStr = TEXT_STR
startTime = -1
endTime = -1

def printPreface():
    print '[Script Info]\r\n\
; Script generated by Aegisub 2.1.9\r\n\
; http://www.aegisub.org/\r\n\
Title: Default Aegisub file\r\n\
ScriptType: v4.00+\r\n\
WrapStyle: 0\r\n\
PlayResX: 640\r\n\
PlayResY: 480\r\n\
ScaledBorderAndShadow: yes\r\n\
Video Aspect Ratio: 0\r\n\
Video Zoom: 6\r\n\
Video Position: 0\r\n\
\r\n\
[V4+ Styles]\r\n\
Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\r\n\
Style: Default,Arial,36,&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,2,2,2,10,10,10,1\r\n\
\r\n\
[Events]\r\n\
Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text'

def timeFormat(s):
    global microSecondsPerTick

    tf = float(s)
    tf /= 62.6  #ticks per sec

    # This should be right , but is too slow
    #tf = (tf * microSecondsPerTick) / 1000000

    t = int(math.floor(tf))
    hundredths = round((tf-t)*100)
    secs = t % 60
    t /= 60
    mins = t % 60
    t /= 60
    hrs = t
    return "%01d:%02d:%02d.%02d" % (hrs, mins, secs, hundredths)

def doLyric(words):
    global textStr
    global startTime
    global endTime
    global TEXT_STR

    if words[1] == "0:":
        #print "skipping"
        return

    time = string.rstrip(words[1], ':')
    if startTime == -1:
        startTime = time
    #print words[1],
    if len(words) == 5:
        if words[4][0] == '\\' or words[4][0] == '/':
            #print "My name is %s and weight is %d kg!" % ('Zara', 21)
            #print startTime, endTime
            if len(words[4]) == 1:
                print textStr % (timeFormat(startTime), timeFormat(endTime)) + "\r\n",
            textStr = TEXT_STR + words[4][1:]
            startTime = -1
        else:
            textStr += words[4]
    else:
        textStr += ' '

    endTime = time

printPreface()

for line in fileinput.input():
    words = line.split()

    if len(words)  >= 2:
        if words[0] == "Resolution:":
            ticksPerBeat = words[1]
        elif words[0] == "Length:":
            numTicks = int(words[1])
        elif words[0] == "Duration:":
            duration = int(words[1])
            microSecondsPerTick = duration/numTicks
            # print "Duration %d numTicks %d microSecondsPerTick %d" % (duration, numTicks, microSecondsPerTick)

    if len(words) >= 3 and words[2] == "Text":
        doLyric(words)

