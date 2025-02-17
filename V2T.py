import torch
import torchaudio
from transformers import Wav2Vec2ForCTC, Wav2Vec2Processor

def transcribe_audio(file_path: str, device: str = "cpu") -> str:
    """
    Transcribes an English WAV audio file into text using a pretrained Wav2Vec2 model.
    
    Args:
        file_path (str): Path to the WAV file.
        device (str): 'cpu' or 'cuda' (if GPU is available).

    Returns:
        str: Transcribed text.
    """
    # Load pretrained model and tokenizer
    tokenizer = Wav2Vec2Processor.from_pretrained("facebook/wav2vec2-base-960h")
    model = Wav2Vec2ForCTC.from_pretrained("facebook/wav2vec2-base-960h")
    model.to(device)

    # 1) Load audio file using torchaudio
    speech_array, sampling_rate = torchaudio.load(file_path)

    # 2) Convert to mono if needed (in case the file has multiple channels)
    #    torchaudio.load already returns a [channels, time] tensor,
    #    so if more than one channel, average them:
    if speech_array.shape[0] > 1:
        speech_array = torch.mean(speech_array, dim=0, keepdim=True)
    
    # 3) Resample to 16kHz if not already
    target_rate = 16000
    if sampling_rate != target_rate:
        resampler = torchaudio.transforms.Resample(orig_freq=sampling_rate, new_freq=target_rate)
        speech_array = resampler(speech_array)
        sampling_rate = target_rate
    
    # 4) Normalize/scale the audio to float32
    speech_array = speech_array.squeeze()  # shape: [time]
    speech_array = speech_array.numpy()

    # 5) Tokenize input
    input_values = tokenizer(speech_array, return_tensors="pt", sampling_rate=target_rate).input_values
    input_values = input_values.to(device)

    # 6) Perform inference
    with torch.no_grad():
        logits = model(input_values).logits
    
    # 7) Decode the output
    predicted_ids = torch.argmax(logits, dim=-1)
    transcription = tokenizer.decode(predicted_ids[0])

    return transcription

if __name__ == "__main__":
    cuda_available = torch.cuda.is_available()
    device = torch.device("cuda" if cuda_available else "cpu")
    print("device is: ", device)
    path_to_wav = "1.wav"   
    text_output = transcribe_audio(path_to_wav, device=device)  
    print("Transcribed Text:", text_output)
