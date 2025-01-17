mlpack_hmm_train --input_file healthy_training_data.csv --output_model_file healthy_hmm.xml --type gaussian --states 1
mlpack_hmm_train --input_file unhealthy_training_data.csv --output_model_file unhealthy_hmm.xml --type gaussian --states 1
