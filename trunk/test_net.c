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

#include <stdio.h>

#include "doublefann.h"

//#include "fann.h"
//#include "3rdparty/FANN-2.2.0-Source/src/doublefann.c"

int main(int argc, char **argv)
{
	if(argc < 3)
	{
		printf("Usage: test_net <input.train> <output.net>\n");
		exit(-1);

	}
	
	fann_type *calc_out;
	unsigned int i;
	int ret = 0;

	struct fann *ann;
	struct fann_train_data *data;

	//printf("Creating network.\n");

	ann = fann_create_from_file(argv[2]);

	if(!ann)
	{
		printf("Error creating ann --- ABORTING.\n");
		return -1;
	}

// 	fann_print_connections(ann);
// 	fann_print_parameters(ann);

	printf("Loading data...\n");

	data = fann_read_train_from_file(argv[1]);
	printf("[debug] main: First pair of inputs:  %f, %f\n", data->input[0][0], data->input[0][1]);

//	for(i = 0; i < fann_length_train_data(data); i++)
	for(i = 0; i < 3; i++)
	{
		fann_reset_MSE(ann);
		calc_out = fann_test(ann, data->input[i], data->output[i]);
/*#ifdef FIXEDFANN
		printf("XOR test (%d, %d) -> %d, should be %d, difference=%f\n",
			   data->input[i][0], data->input[i][1], calc_out[0], data->output[i][0],
			   (float) fann_abs(calc_out[0] - data->output[i][0]) / fann_get_multiplier(ann));

		if((float) fann_abs(calc_out[0] - data->output[i][0]) / fann_get_multiplier(ann) > 0.2)
		{
			printf("Test failed\n");
			ret = -1;
		}
#else*/


		printf("Distance test (%d dBm,%f%%) -> %f meters, should be %f meters, difference=%f meters\n",
			   (int)(data->input[i][0] * 150 - 150), data->input[i][1], calc_out[0] * 1000, data->output[i][0] * 1000,
			   fann_abs(calc_out[0] - data->output[i][0]) * 1000);
			   
			   
// 		printf("XOR test (%f, %f) -> %f, should be %f, difference=%f\n",
// 			   data->input[i][0], data->input[i][1], calc_out[0], data->output[i][0],
// 			   (float) fann_abs(calc_out[0] - data->output[i][0]));

// 		printf("Data set #%d:\n", i);
// 		printf("\t Inputs:  %f, %f\n", data->input[i][0], data->input[i][1]);
// 		//calc_out[0], 
// 		printf("\t Outputs: %f\n",     data->output[i][0]);
		//	   (float) fann_abs(calc_out[0] - data->output[i][0]
//#endif
	}

	//printf("Cleaning up.\n");
	fann_destroy_train(data);
	fann_destroy(ann);

	return ret;
}
