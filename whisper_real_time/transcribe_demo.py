#! python3.7

import argparse
import os
import numpy as np
import speech_recognition as sr
import whisper
import torch

from datetime import datetime, timedelta
from queue import Queue
from time import sleep
from sys import platform

def check_available_microphone():
    print("Available microphone devices are: ")
    for index, name in enumerate(sr.Microphone.list_microphone_names()):
        print(f"Microphone with name \"{name}\" found")


def main():
    #define the configuration parameters 
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", default="medium", help="Model to use", choices=["tiny", "base", "small", "medium", "large"])
 
    parser.add_argument("--mic_energy_threshold", default=1000, help="Energy level for mic to detect.", type=int)

    parser.add_argument("--aduio_chunck", default=2, help="record how many second as a chunk and send to queue", type=float)
    parser.add_argument("--phrase_timeout", default=3, help="How many second empty should we consider the current sentence is done", type=float)

    parser.add_argument("--computer_syetem", default='linux', help="type in your computer")
    parser.add_argument("--sampling_rate", default=16000, type=int)

    args = parser.parse_args()

    # The last time a recording was retrieved from the queue.
    phrase_time = None
    
    """datastructure which used to store the audio chunk"""
    data_queue = Queue()


    """create a recorder in the back thread"""
    recorder = sr.Recognizer()
    recorder.energy_threshold = args.energy_threshold
    recorder.dynamic_energy_threshold = False

    
    """set up the microphone source"""
    if 'linux' in platform:
        # mic_name = args.default_microphone
        mic_name = "MCHStreamer PDM16"
        if not mic_name or mic_name == 'list':
            check_available_microphone()
            return
        else:
            for index, name in enumerate(sr.Microphone.list_microphone_names()):
                if mic_name in name:
                    source = sr.Microphone(sample_rate=args.sampling_rate, device_index=index)
                    break
    else:
        source = sr.Microphone(sample_rate=16000)

    """load the whisper model"""
    model = args.model
    audio_model = whisper.load_model(model)

    aduio_chunck = args.aduio_chunck
    phrase_timeout = args.phrase_timeout

    transcription = ['']


    """record the noise source and used it to reduce the noise in the future"""
    with source:
        recorder.adjust_for_ambient_noise(source)

    def record_callback(_, audio:sr.AudioData) -> None:
        """
        Threaded callback function to receive audio data when recordings finish.
        audio: An AudioData containing the recorded bytes.
        """
        # Grab the raw bytes and push it into the thread safe queue.
        data = audio.get_raw_data()
        data_queue.put(data)
    
    #cut the recording chunk into aduio_chunk second 
    recorder.listen_in_background(source, record_callback, phrase_time_limit=aduio_chunck)

    # Cue the user that we're ready to go.
    print("the recording start! ")

    while True:
        try:
            now = datetime.utcnow()
            """if the queue is not empty, grab the data from the queue"""
            if not data_queue.empty():
                phrase_complete = False
                #check if the waiting time is longer enought for a new sentence 
                if phrase_time and now - phrase_time > timedelta(seconds=phrase_timeout):
                    phrase_complete = True
                
                #update the new sentence time counter
                phrase_time = now
                
                # Combine audio data from queue
                audio_data = b''.join(data_queue.queue)
                data_queue.queue.clear()
                
                # Convert in-ram buffer to something the model can use directly without needing a temp file.
                # Convert data from 16 bit wide integers to floating point with a width of 32 bits.
                # Clamp the audio stream frequency to a PCM wavelength compatible default of 32768hz max.
                audio_np = np.frombuffer(audio_data, dtype=np.int16).astype(np.float32) / 32768.0

                # Read the transcription.
                result = audio_model.transcribe(audio_np, fp16=torch.cuda.is_available())
                text = result['text'].strip()

                # If we detected a pause between recordings, add a new item to our transcription.
                # Otherwise edit the existing one.
                if phrase_complete:
                    transcription.append(text)
                else:
                    transcription[-1] = text

                # Clear the console to reprint the updated transcription.
                os.system('cls' if os.name=='nt' else 'clear') 
                for line in transcription:
                    print(line)
                # Flush stdout.
                print('', end='', flush=True)
            else:
                # Infinite loops are bad for processors, must sleep.
                sleep(0.25)
        except KeyboardInterrupt:
            break

    print("\n\nTranscription:")
    for line in transcription:
        print(line)


if __name__ == "__main__":
    main()
