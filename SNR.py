import numpy as np
import matplotlib.pyplot as plt

def calculate_snr(signal, noise):
    """
    Calculate the Signal-to-Noise Ratio (SNR) in both linear and dB scales.

    Parameters:
    -----------
    signal : numpy.ndarray
        Array containing the signal samples.
    noise : numpy.ndarray
        Array containing the noise samples.

    Returns:
    --------
    snr_linear : float
        The SNR (signal power / noise power) as a linear ratio.
    snr_db : float
        The SNR in decibels (dB).
    """
    # Calculate power of signal (mean of square of samples)
    power_signal = np.mean(signal**2)
    print("the =12")
    
    # Calculate power of noise (mean of square of samples)
    power_noise = np.mean(noise**2)
    
    # Linear SNR
    snr_linear = power_signal / power_noise
    
    # SNR in dB
    snr_db = 10 * np.log10(snr_linear)
    
    return snr_linear, snr_db

# ------------------
# Example usage
# ------------------

# 1. Generate a synthetic signal
#    Let's create a 5 kHz sine wave sampled at 50 kHz for 0.01s
fs = 50_000  # sampling frequency --> number of semples per second
duration = 0.01  # seconds
t = np.linspace(0, duration, int(fs*duration), endpoint=False)
freq = 5_000  # 5 kHz

# Sine wave of amplitude 1 (peak)
clean_signal = np.sin(2 * np.pi * freq * t)



# 2. Generate noise and create a noisy signal
noise_amplitude = 0.2  # standard deviation of the noise
noise = noise_amplitude * np.random.randn(len(t))
noisy_signal = clean_signal + noise

# 3. Calculate SNR
#    Here, we treat the "clean_signal" as the true signal
#    and the "noise" as the noise component.
snr_linear, snr_db = calculate_snr(clean_signal, noise)

print(f"SNR (linear)  : {snr_linear:.3f}")
print(f"SNR (dB)      : {snr_db:.3f} dB")

# 4. (Optional) Visualize the results
plt.figure(figsize=(10,5))
plt.plot(t, noisy_signal, label='Noisy Signal')
plt.plot(t, clean_signal, label='Clean Signal', alpha=0.8)
plt.xlabel('Time [s]')
plt.ylabel('Amplitude')
plt.title(f'Synthetic Sine Wave + Noise\nSNR = {snr_db:.2f} dB')
plt.legend()
plt.tight_layout()
plt.show()
