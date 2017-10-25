#include <nn.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

static inline float nn_sigmoid(float input)
{
	if(input < -45.0){
		return 0;
	}else if(input > 45.0){
		return 1;
	}

	return 1.0 / (1 + exp(-input));
}

static inline float nn_fast_sigmoid(float input)
{
	return input / (1 + fabs(input)); 
}

static inline float nn_relu(float input)
{
	if(input < 0){
		return 0.0;
	}

	return input;
}

static inline float nn_rand(float start, float end)
{
	assert(start < end);

	float range = end - start;

	return (float)rand() / (float)(RAND_MAX / range) + start;
}

static inline float nn_activate(enum nn_activation activation, float input)
{
	switch(activation){
		case NN_ACTIVATION_PASSTHROUGH:
			return input;
		case NN_ACTIVATION_SIGMOID:
			return nn_sigmoid(input);
		case NN_ACTIVATION_FAST_SIGMOID:
			return nn_fast_sigmoid(input);
		case NN_ACTIVATION_RELU:
			return nn_relu(input);
		default:
			assert(false);
	}

	return 0.0f;
}

static void nn_ffnet_set_pointers(struct nn_ffnet *net)
{
	assert(net);

	net->weight = (float*)((char*)net + sizeof(struct nn_ffnet));
	net->output = net->weight + net->nweights;
	net->activation = (char*)(net->output + net->nneurons);
}

static float *nn_ffnet_weight_at_hidden_layer(struct nn_ffnet *net,
					      size_t layer)
{
	assert(net);
	assert(layer < net->nhidden_layers);

	size_t input_offset = (net->ninputs + 1) * net->nhiddens;

	return net->weight + input_offset + (net->nhiddens + 1) * layer;
}

static size_t nn_ffnet_hidden_weights(size_t input_count,
				      size_t hidden_count,
				      size_t output_count,
				      size_t hidden_layer_count)
{
	if(hidden_layer_count == 0){
		return 0;
	}

	size_t input_weights = (input_count + 1) * hidden_count;
	size_t hidden_internal_weights = (hidden_layer_count - 1) *
		(hidden_count + 1) *
		hidden_count;

	return input_weights + hidden_internal_weights;
}

static size_t nn_ffnet_output_weights(size_t input_count,
				      size_t hidden_count,
				      size_t output_count,
				      size_t hidden_layer_count)
{
	size_t output_weights;
	if(hidden_layer_count > 0){
		output_weights = hidden_count + 1;
	}else{
		output_weights = input_count + 1;
	}
	output_weights *= output_count;

	return output_weights;
}

static size_t nn_ffnet_total_weights(size_t input_count,
				     size_t hidden_count,
				     size_t output_count,
				     size_t hidden_layer_count)
{
	size_t hidden_weights = nn_ffnet_hidden_weights(input_count,
							hidden_count,
							output_count,
							hidden_layer_count);

	size_t output_weights = nn_ffnet_output_weights(input_count,
							hidden_count,
							output_count,
							hidden_layer_count);

	return hidden_weights + output_weights;
}

static size_t nn_ffnet_total_neurons(size_t input_count,
				     size_t hidden_count,
				     size_t output_count,
				     size_t hidden_layer_count)
{
	return input_count + hidden_count * hidden_layer_count + output_count;
}

static size_t nn_ffnet_total_activations(size_t hidden_count,
					 size_t output_count,
					 size_t hidden_layer_count)
{
	return hidden_count * hidden_layer_count + output_count;
}

struct nn_ffnet *nn_ffnet_create(size_t input_count,
				 size_t hidden_count,
				 size_t output_count,
				 size_t hidden_layer_count)
{
	assert(input_count > 0);
	assert(output_count > 0);
	assert(hidden_count > 0);

	size_t total_weights = nn_ffnet_total_weights(input_count,
						      hidden_count,
						      output_count,
						      hidden_layer_count);

	size_t total_neurons = nn_ffnet_total_neurons(input_count,
						      hidden_count,
						      output_count,
						      hidden_layer_count);

	size_t total_activs = nn_ffnet_total_activations(hidden_count,
							 output_count,
							 hidden_layer_count);

	/* Allocate the struct with extra bytes behind it for the data */
	size_t items_bytes = sizeof(float) * (total_weights + total_neurons);
	/* Allocate the activations */
	items_bytes += sizeof(char) * total_activs;
	assert(items_bytes > 0);
	struct nn_ffnet *net = calloc(items_bytes + sizeof(struct nn_ffnet), 1);
	assert(net);

	net->ninputs = input_count;
	net->nhiddens = hidden_count;
	net->noutputs = output_count;
	net->nhidden_layers = hidden_layer_count;

	net->nweights = total_weights;
	net->nneurons = total_neurons;
	net->nactivations = total_activs;

	/* Default values */
	net->bias = -1.0;

	nn_ffnet_set_pointers(net);

	return net;
}

struct nn_ffnet *nn_ffnet_copy(struct nn_ffnet *net)
{
	assert(net);

	size_t extra = sizeof(float) * (net->nweights + net->nneurons);
	extra += sizeof(char) * net->nactivations;
	size_t bytes = sizeof(struct nn_ffnet) + extra;
	assert(bytes > sizeof(struct nn_ffnet));

	struct nn_ffnet *new = malloc(bytes);
	assert(new);

	memcpy(new, net, bytes);

	nn_ffnet_set_pointers(new);

	return new;
}

void nn_ffnet_destroy(struct nn_ffnet *net)
{
	assert(net);

	free(net);
}

struct nn_ffnet *nn_ffnet_add_hidden_layer(struct nn_ffnet *net, float weight)
{
	assert(net);
	assert(net->nhiddens > 0);

	/* Creating a new network is the easiest solution since all the pointers
	 * will be put in the right position for us by default
	 */
	struct nn_ffnet *new = nn_ffnet_create(net->ninputs,
					       net->nhiddens,
					       net->noutputs,
					       net->nhidden_layers + 1);
	assert(new);

	/* ACTIVATIONS */
	/* Copy the hidden activations */
	memcpy(new->activation,
	       net->activation,
	       net->nhiddens * net->nhidden_layers);

	/* Copy the output activations */
	memcpy(new->activation + new->nactivations - new->noutputs,
	       net->activation + net->nactivations - net->noutputs,
	       net->noutputs);

	/* WEIGHTS */
	/* Copy the hidden and the input weights */
	size_t noutput_weights = nn_ffnet_output_weights(new->ninputs,
							 new->nhiddens,
							 new->noutputs,
							 new->nhidden_layers);
	memcpy(new->weight,
	       net->weight,
	       sizeof(float) * (net->nweights - noutput_weights));

	/* Copy the output weights */
	memcpy(new->weight + new->nweights - noutput_weights,
	       net->weight + net->nweights - noutput_weights,
	       sizeof(float) * noutput_weights);

	/* Find where the location and amount of the new empty weights */
	size_t nnew_weights = new->nweights - net->nweights;

	/* Destroy the old one */
	nn_ffnet_destroy(net);

	size_t nweights_per_layer = new->nhiddens + 1;
	size_t new_layer = new->nhidden_layers - 1;
	float *new_weight;
	if(new_layer == 0){
		new_weight = new->weight;
	}else{
		new_weight = nn_ffnet_weight_at_hidden_layer(new,
							     new_layer - 1);
	}

	/* Skip the bias */
	new_weight++;

	float *new_weight_finish = new_weight + nnew_weights;
	/* Set each weight directly connected to the node before it */
	do{
		*new_weight = weight;
	}while((new_weight += nweights_per_layer + 1) < new_weight_finish);

	return new;
}

void nn_ffnet_set_activations(struct nn_ffnet *net,
			      enum nn_activation hidden,
			      enum nn_activation output)
{
	assert(net);

	char *activation = net->activation;
	for(size_t i = 0; i < net->nactivations - net->noutputs; i++){
		*activation++ = (char)hidden;
	}

	for(size_t i = 0; i < net->noutputs; i++){
		*activation++ = (char)output;
	}
}

void nn_ffnet_set_bias(struct nn_ffnet *net, float bias)
{
	assert(net);

	net->bias = bias;
}

void nn_ffnet_set_weights(struct nn_ffnet *net, float weight)
{
	assert(net);

	for(int i = 0; i < net->nweights; i++){
		net->weight[i] = weight;
	}
}

void nn_ffnet_randomize(struct nn_ffnet *net)
{
	assert(net);

	for(int i = 0; i < net->nweights; i++){
		net->weight[i] = nn_rand(-0.5, 0.5);
	}
}

float *nn_ffnet_run(struct nn_ffnet *net, const float *inputs)
{
	assert(net);

	/* Copy the inputs to the extra output memory space so we don't have
	 * to make a special case for the input layer, it will look like this:
	 * [ **struct**, weight.. , input.., output.., **delta** ]
	 */
	float *input = net->output;
	memcpy(input, inputs, sizeof(float) * net->ninputs);

	/* Calculate hidden layers */
	float *weight = net->weight;
	float *output = net->output + net->ninputs;
	char *activation = net->activation;
	for(size_t i = 0; i < net->nhidden_layers; i++){
		/* First get all the inputs, then get all the hidden layers */
		size_t nweights = net->nhiddens;
		if(i == 0){
			nweights = net->ninputs;
		}

		for(size_t j = 0; j < net->nhiddens; j++){
			/* Start with the bias */
			float sum = *weight++ * net->bias;

			/* Sum the rest of the weights */
			for(size_t k = 0; k < nweights; k++){
				sum += *weight++ * input[k];
			}

			*output++ = nn_activate(*activation++, sum);
		}

		input += nweights;
	}

	/* The return value must be saved because the output pointer is going
	 * to be changed by the output layer calculation */
	float *ret = output;

	size_t nweights = net->nhiddens;
	/* Get the input layer if there are no hidden layers */
	if(net->nhidden_layers == 0){
		nweights = net->ninputs;
	}

	/* Calculate output layer */
	for(size_t i = 0; i < net->noutputs; i++){
		/* Start with the bias */
		float sum = *weight++ * net->bias;

		/* Sum the rest of the weights */
		for(size_t j = 0; j < nweights; j++){
			sum += *weight++ * input[j];
		}

		*output++ = nn_activate(*activation++, sum);
	}

	assert(weight - net->weight == net->nweights);
	assert(output - net->output == net->nneurons);

	return ret;
}

bool nn_ffnet_neuron_is_connected(struct nn_ffnet *net, size_t neuron_id)
{
	assert(net);
	assert(neuron_id < net->nneurons);

	/* The input layer is always connected */
	if(neuron_id < net->ninputs){
		return true;
	}

	size_t start_index = nn_ffnet_get_weight_to_neuron(net, neuron_id);

	size_t nweights = net->nhiddens;
	if(neuron_id < net->ninputs + net->nhiddens){
		/* Check the first hidden layer connected to the inputs */
		nweights = net->ninputs;
	}

	/* + 1 for ignoring the bias */
	float *weight = net->weight + start_index;
	int is_connected = 0;
	for(size_t i = 0; i < nweights; i++){
		is_connected += *weight++ != 0.0f;
	}

	return is_connected != 0;
}

size_t nn_ffnet_get_weight_to_neuron(struct nn_ffnet *net, size_t neuron_id)
{
	assert(net);
	assert(neuron_id < net->nneurons);
	assert(neuron_id >= net->ninputs);

	neuron_id -= net->ninputs;
	/* The input -> first hidden layer */
	if(neuron_id < net->nhiddens){
		return neuron_id * (net->ninputs + 1) + 1;
	}

	neuron_id -= net->nhiddens;

	/* The other layers */
	size_t first_layer_offset = (net->ninputs + 1) * net->nhiddens;

	return first_layer_offset + neuron_id * (net->nhiddens + 1) + 1;
}
