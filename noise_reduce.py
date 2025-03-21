import librosa
import soundfile as sf
import noisereduce as nr

# 1. readin the audio 
noisy_audio, sr = librosa.load('mar_13_5_words_head.wav', sr=None)

# 2.  reduce the noise 
reduced_noise = nr.reduce_noise(y=noisy_audio, sr=sr)

amplify_factor = 3  
amplified_audio = noisy_audio * amplify_factor

# 3. save the audio 
sf.write('output_denoised.wav', reduced_noise, sr)
sf.write('output_deniosed_amplitude.wav', amplified_audio, sr)

print("save to output_denoised.wav")
