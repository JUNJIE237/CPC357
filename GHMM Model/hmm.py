import subprocess
import re
def get_log_likelihood(model_file, test_file):
    # Run mlpack_hmm_loglik with correct arguments
    result = subprocess.run(
        [
            "mlpack_hmm_loglik",
            "--input_file", test_file,
            "--input_model_file", model_file,
            #"--verbose", "false"
        ],
        capture_output=True,
        text=True
    )
    
    # Parse and return the log-likelihood
    try:
        print(result)
        match = re.search(r"log_likelihood:\s*(-?\d+\.?\d*)", result.stdout)
        if match:
            # Convert the matched value to a float and return it
            return float(match.group(1))
        #log_likelihood = float(result.stdout.strip())
        #return log_likelihood
    except ValueError:
        raise ValueError(f"Unexpected output: {result.stdout.strip()}")

# Example usage
healthy_ll = get_log_likelihood("healthy_hmm.xml", "sample_unhealthy.csv")
unhealthy_ll = get_log_likelihood("unhealthy_hmm.xml","sample_unhealthy.csv")

# Compare likelihoods
if healthy_ll > unhealthy_ll:
    prediction="Healthy"
else:
    prediction="Unhealthy"

# Example usage
print(f"Predicted class: {prediction}")

