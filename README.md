# MicroDexed for ESP32, beta 1.0, by nyh.

## Thanks to Holger W. for the MicroDexed code: https://github.com/dcoredump/MicroDexed

----------

The sample program plays a short tune (Holst's Jupiter) using MicroDexed engine using the I2S module (PCM5102).

Also, thanks to Len Shustek for the Miditones code: https://github.com/LenShustek/miditones 

Note: This only works with 8 notes - more than that the whole thing will stutter.

Suggestions are welcome to improve this with more than 8 notes. In the meantime I'm still finding ways and means 
to improve the FM synth algorithm.

A slight modification is done on dexed.cpp -> the SSAT instruction is replaced by CLAMPS because ESP32 doesn't 
have SSAT. Fundamentally, CLAMPS works the same as SSAT.

As this is a beta, the code can be very messy. Again, I'm working to reorganize the code as much as possible.

