import whisper

model = whisper.load_model("turbo")
result = model.transcribe("mar_14_this_is_a_test_amplifier_reduce_noise.wav")
print("Transcribed Text:", result["text"])