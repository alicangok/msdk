###############################################################################
 #
 # Copyright (C) 2022-2023 Maxim Integrated Products, Inc. All Rights Reserved.
 # (now owned by Analog Devices, Inc.),
 # Copyright (C) 2023 Analog Devices, Inc. All Rights Reserved. This software
 # is proprietary to Analog Devices, Inc. and its licensors.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #     http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 #
 ##############################################################################
"""
Voice recorder
"""
import wave
import argparse
import pyaudio


SR = 16000  # sample per sec
CHANNEL = 1  # number of input channel
FORMAT = pyaudio.paInt16  # data format
CHUNK = 1024
LENGTH = 4


def audio_recorder(filename='output.wav', recordlength=LENGTH, samplerate=SR):
    """
    Record audio to a file.
    """
    audio = pyaudio.PyAudio()

    print("Recording started for", recordlength, " sec")
    stream = audio.open(format=FORMAT,
                        channels=CHANNEL,
                        rate=samplerate,
                        input=True,
                        frames_per_buffer=CHUNK)

    frame = []
    for i in range(0, int(samplerate / CHUNK * recordlength)):
        data = stream.read(CHUNK)
        frame.append(data)
        print(i)

    print("Recording finished!")
    stream.stop_stream()
    stream.close()
    audio.terminate()

    # Store in file
    wf = wave.open(filename, 'wb')
    wf.setnchannels(CHANNEL)
    wf.setsampwidth(audio.get_sample_size(FORMAT))
    wf.setframerate(samplerate)
    wf.writeframes(b''.join(frame))
    wf.close()


def command_parser():
    """
    Return the argument parser
    """
    parser = argparse.ArgumentParser(description='Audio recorder command parser')
    parser.add_argument('-d', '--duration', type=int, default=LENGTH,
                        help='audio recording duration (default:' + LENGTH.__str__() + ')')
    parser.add_argument('-sr', '--samplerate', type=int, default=SR,
                        help='recording samplerate (default:' + SR.__str__() + ')')
    parser.add_argument('-o', '--output', type=str, default='voice.wav',
                        help='output wavefile name')
    return parser.parse_args()


if __name__ == "__main__":
    command = command_parser()
    print("Output Name = ", command.output)
    print("Sample Rate = ", command.samplerate)
    print("Duration = ", command.duration)
    audio_recorder(command.output, command.duration, command.samplerate)
