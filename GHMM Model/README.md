## Overview <br>
MLpack is a fast and flexible machine learning library written in C++. It is designed to provide easy-to-use machine learning algorithms, and it aims to be efficient for both small-scale and large-scale machine learning tasks. MLpack supports a wide range of algorithms, such as classification, regression, clustering, and dimensionality reduction, among others.
<br>
<br>
### What is `mlpack_bin`? <br>
`mlpack_bin` is the directory where the mlpack executables, such as mlpack_hmm_train and mlpack_hmm_loglik, are stored after building mlpack from source. <br>
These executables are command-line utilities provided by the mlpack library to perform various machine learning tasks, including training and using Hidden Markov Models (HMMs).
<br><br>
### What is `mlpack_hmm`?
`mlpack_hmm` is a set of tools provided by mlpack for:<br>
- Training HMMs (mlpack_hmm_train).<br>
- Evaluating HMM log-likelihoods (mlpack_hmm_loglik).<br>
- Performing sequence predictions with trained HMMs (mlpack_hmm_generate).<br><br>

#### Train an HMM Using mlpack_hmm_train <br>
`mlpack_hmm_train --input_file healthy_training_data.csv --output_model_file healthy_hmm.xml --type gaussian --states 1`
<br><br>

#### Train an HMM Using mlpack_hmm_train<br>
`mlpack_hmm_train --input_file unhealthy_training_data.csv --output_model_file unhealthy_hmm.xml --type gaussian --states 1`

