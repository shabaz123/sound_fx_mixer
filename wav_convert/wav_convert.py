# wav_convert.py
# rev 1 - shabaz - Jan 2024
# This code scans the wav_files folder for all .wav files, and converts them to C arrays
# The generated C arrays are in the output folder

import os
import wave
import struct

# function to find all .wav files in the current folder
def find_wav_files():
    wav_files = []
    for file in os.listdir("./wav_files"):
        if file.endswith(".wav"):
            wav_files.append(file)
    return wav_files


def maincode():
    summary = []
    files = find_wav_files()
    for file in files:
        print("Processing file: " + file)
        wav_file = wave.open("./wav_files/"+file, 'r')
        nframes = wav_file.getnframes()  # get the number of frames
        # read the frames and convert to a list of integers
        frames = wav_file.readframes(nframes)
        # convert the list of integers to a list of 16-bit integers
        samples = struct.unpack_from("%dh" % nframes, frames)
        # now create the output file!
        file = file.rsplit('.', 1)[0]  # remove the .wav extension
        new_file = open("./output/"+file + ".c", "w")  # open the output file
        new_file.write("#include <stdint.h>\n")
        new_file.write("const int16_t " + file + "[" + str(nframes) + "] = {\n")
        for sample in samples:
            new_file.write(str(sample) + ",\n")
        new_file.write("};\n")  # terminate the array
        new_file.close()
        summary.append("extern const int16_t " + file + "[" + str(nframes) + "];")
    # print the summary
    print("Done! Generated C arrays:")
    for line in summary:
        print(line)


if __name__ == '__main__':
    maincode()
