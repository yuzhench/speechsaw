# import whisper

# model = whisper.load_model("turbo")
# result = model.transcribe("mar_14_this_is_a_test_amplifier_reduce_noise.wav")
# print("Transcribed Text:", result["text"])

import whisper
from pydub import AudioSegment
import numpy as np
import os

# Load the audio file via pydub
audio_path = "condition2-trial1-vpu-noisereduce2.wav"
audio = AudioSegment.from_file(audio_path, format="wav")

# Let's define chunk duration (in ms), e.g., 1000 ms = 1 second
chunk_length_ms = 1000

# Threshold for deciding if a chunk is "loud enough" to be considered the wearer's speech
# This threshold depends on your audio environment. You can adjust it as needed.
# pydub's raw dBFS is negative. For example, -30 dBFS is quieter than -20 dBFS.
loudness_threshold_dbfs = -30.0

chunks = []
loud_chunks = AudioSegment.silent(duration=0)  # Start with an empty audio segment

# Split the audio in fixed-size chunks
for start_ms in range(0, len(audio), chunk_length_ms):
    end_ms = start_ms + chunk_length_ms
    chunk = audio[start_ms:end_ms]

    # chunk.dBFS gives the average loudness of this chunk in dBFS (negative number).
    # The higher the value (e.g., -20 dBFS vs -40 dBFS), the "louder" the chunk.
    if chunk.dBFS > loudness_threshold_dbfs:
        # If chunk is louder than threshold, we keep it
        loud_chunks += chunk  # Append to our "loud" segments

# Now we have an AudioSegment 'loud_chunks' that (hopefully) mostly contains the louder speaker.
# Export to a temporary WAV file if needed (because whisper.transcribe typically takes a file path).
filtered_path = "filtered.wav"
loud_chunks.export(filtered_path, format="wav")

# Load Whisper model (replace "base" with "tiny", "medium", "large", or custom if desired)
model = whisper.load_model("base")

# Transcribe the filtered audio
result = model.transcribe(filtered_path)
print("Transcribed text:", result["text"])

# Optional: Cleanup temporary file
os.remove(filtered_path)

