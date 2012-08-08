/*
Fast Artificial Neural Network Library (fann)
Copyright (C) 2003-2012 Steffen Nissen (sn@leenissen.dk)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//#include "fann.h"
#include "doublefann.h"


int main(int argc, char **argv)
{
	if(argc < 3)
	{
		printf("Usage: train_net <input.train> <output.net>\n");
		exit(-1);
		
	}
	const unsigned int num_input = 2;
	const unsigned int num_output = 1;
	const unsigned int num_layers = 3;
	const unsigned int num_neurons_hidden = 8;
	const float desired_error = (const float) 0.000042;
	const unsigned int max_epochs = 500000;
	const unsigned int epochs_between_reports = 1000;

	struct fann *ann = fann_create_standard(num_layers, num_input, num_neurons_hidden, num_output);

	fann_set_activation_function_hidden(ann, FANN_SIGMOID_SYMMETRIC);
	fann_set_activation_function_output(ann, FANN_SIGMOID_SYMMETRIC);

// 	fann_set_activation_steepness_hidden(ann, 1);
// 	fann_set_activation_steepness_output(ann, 1);

// 	fann_set_activation_function_hidden(ann, FANN_SIGMOID_SYMMETRIC);
// 	fann_set_activation_function_output(ann, FANN_SIGMOID_SYMMETRIC);

// 	fann_set_train_stop_function(ann, FANN_STOPFUNC_BIT);
 	fann_set_bit_fail_limit(ann, 0.01f);

	fann_set_training_algorithm(ann, FANN_TRAIN_RPROP);


	
	//fann_train_on_file(ann, argv[1], max_epochs, epochs_between_reports, desired_error);

	struct fann_train_data *data;
	data = fann_read_train_from_file(argv[1]);
	fann_init_weights(ann, data);

	printf("Training network on data from %s.\n", argv[1]);
	fann_train_on_data(ann, data, max_epochs, epochs_between_reports, desired_error);

	printf("Testing network. %f\n", fann_test_data(ann, data));

	double error, errorSum = 0;
	unsigned int i = 0, size = fann_length_train_data(data);
	fann_type *calc_out;
	for(i = 0; i < size; i++)
	{
		calc_out = fann_run(ann, data->input[i]);
		error = fann_abs(calc_out[0] - data->output[i][0]) * 1000;
		printf("Distance test (%d dBm,%f%%) -> %f meters, should be %f meters, difference=%f meters\n",
			   (int)(data->input[i][0] * 150 - 150), data->input[i][1], calc_out[0] * 1000, data->output[i][0] * 1000,
			   error);
		errorSum += error;
	}

	printf("Average Error: %f\n", errorSum / size);
	fann_save(ann, argv[2]);

	fann_destroy(ann);

	return 0;
}
