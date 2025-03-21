def calculate_wer(ref, hyp):
    """
    Calculate the Word Error Rate (WER) between the reference text (ref) and
    the recognized text (hyp).

    Parameters:
    ref (str or list): The ground truth text, either as a string or a list of words.
    hyp (str or list): The recognized text, either as a string or a list of words.

    Returns:
    float: The WER value (0.0 means an identical match; higher numbers indicate more errors).
    """
    import numpy as np

    # If 'ref' or 'hyp' is a string, split it into a list of words.
    if isinstance(ref, str):
        ref_words = ref.split()
    else:
        ref_words = ref
    
    if isinstance(hyp, str):
        hyp_words = hyp.split()
    else:
        hyp_words = hyp

    # Length of reference and hypothesis word lists
    n = len(ref_words)
    m = len(hyp_words)

    # Create a DP matrix (n+1) x (m+1),
    # dp[i][j] will hold the minimum edit distance between
    # ref_words[:i] and hyp_words[:j].
    dp = np.zeros((n + 1, m + 1), dtype=int)

    # Initialize the first row and first column.
    # dp[i][0] means transforming the first i words of ref to an empty sequence (deletions).
    # dp[0][j] means transforming an empty sequence to the first j words of hyp (insertions).
    for i in range(1, n + 1):
        dp[i][0] = i
    for j in range(1, m + 1):
        dp[0][j] = j

    # Fill in the DP matrix
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            if ref_words[i - 1] == hyp_words[j - 1]:
                # If the current words match, no additional edit distance
                dp[i][j] = dp[i - 1][j - 1]
            else:
                # Calculate costs for substitution, insertion, and deletion
                substitution = dp[i - 1][j - 1] + 1
                insertion = dp[i][j - 1] + 1
                deletion = dp[i - 1][j] + 1
                dp[i][j] = min(substitution, insertion, deletion)

    # The minimum edit distance for the full sequence is in dp[n][m]
    edit_distance = dp[n][m]

    # WER = (edit_distance) / (number of words in reference)
    wer = edit_distance / float(n) if n > 0 else 0.0
    return wer

# ================= EXAMPLE USAGE =================
if __name__ == "__main__":
    ground_truth = "I really like cats"
    predicted_text = "I really like cats"
    
    wer_value = calculate_wer(ground_truth, predicted_text)
    print(f"WER: {wer_value:.2%}")  # e.g., 25.00% means 0.25 WER
