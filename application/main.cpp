/**
 *	Libraries.
 */

/* Inclusion of mbed.h */
#include "mbed.h"

/* Inclusion of model.h */
#include "model.h"

/* Inclusion of FXOS8700.h. */
#include "FXOS8700.h"

/* Inclusion of FXAS21002.h. */
#include "FXAS21002.h"

/* Inclusion of ctime.h. */
#include <ctime>

/* Inclusion of all_ops_resolver.h. */
#include "all_ops_resolver.h"

/* Inclusion of micro_error_reporter.h. */
#include "micro_error_reporter.h"

/* Inclusion of micro_interpreter.h. */
#include "micro_interpreter.h"

/* Inclusion of schema/schema_generated.h. */
#include "schema_generated.h"

/* Inclusion of version.h. */
#include "version.h"

/* Inclusion of kernel_util.h. */
#include "kernel_util.h"

/* Use tflite namespace. */
using namespace tflite;


/**
 *	Symbolic constants.
 */
 
/* Number of classes. */
#define N_CLASSES 6


/**
 *	Global variables.
 */
 
/* Activities array. */
const char labels[N_CLASSES][32] = {"Sitting", "Lying", "Standing", "Jumping", "Walking", "Running"};

/* Serial object that allows serial communication. */
Serial pc(USBTX, USBRX, 115200);

/* MicroMutableOpResolver object used in order to make use of the different types of layers. */
MicroMutableOpResolver<8> micro_op_resolver;

/* Pointer to a ErrorReporter object used to print errors. */
ErrorReporter* error_reporter = nullptr;

/* Pointer to a Model object. */
const Model* model = nullptr;

/* Pointer to a MicroInterpreter object used to run inferences. */
MicroInterpreter* interpreter = nullptr;

/* Pointer to the TfLiteTensor tensor that stores the input values to the neural network. */
TfLiteTensor* input = nullptr;

/* Pointer to the TfLiteTensor tensor that stores the output values of the neural network. */
TfLiteTensor* output = nullptr;

/* Fixed number of bytes used to store working tensors. */
const int k_tensor_arena_size = 32768;

/* Working tensors' dedicated memory. */
alignas(16) uint8_t tensor_arena[k_tensor_arena_size];

/* FXOS8700 object that represents the accelerometer. */
FXOS8700 accel(PTC11, PTC10);

/* FXAS21002 object that represents the gyroscope. */
FXAS21002 gyro(PTC11, PTC10);

/* Thread used to sample from accelerometer and gyroscope at 50Hz. */
Thread sampling_thread(osPriorityNormal, 512);

/* Variable used to store the index of the current sample. */
volatile int curr_sample;


/**
 *	Functions declaration.
 */

/* Function that initializes all the variables and objects. */
void setup();

/* Function that runs the application. */
void loop();

/* Function used to sample and store new accelerometer and gyroscope values. */
void sample();


/**
 *	Functions definition.
 */

/* main function. */
int main() 
{	
	/* Initialize variables and objects. */
	setup();
	
	/* Run the application. */
	loop();
	
	/* Return zero. */
	return 0;
}

/* setup function used to initializes variables and objects. */
void setup()
{
	/* Add the Quantize operator. */
	micro_op_resolver.AddQuantize();
	
	/* Add the ReLU operator. */
	micro_op_resolver.AddRelu();
	
	/* Add the Reshape operator. */
	micro_op_resolver.AddReshape();
	
	/* Add the Conv2D operator. Reshape and Conv2D implements the Conv1D operation. */
	micro_op_resolver.AddConv2D();
	
	/* Add the MaxPool2D operator. Reshape and MaxPool2D implements the MaxPool1D operation. */
	micro_op_resolver.AddMaxPool2D();
	
	/* Add the FullyConnected operator. */
	micro_op_resolver.AddFullyConnected();
	
	/* Add the Softmax operator. */
	micro_op_resolver.AddSoftmax();
	
	/* Add the Dequantize operator. */
	micro_op_resolver.AddDequantize();
	
	/* New MicroErrorReporter object. */
	static MicroErrorReporter micro_error_reporter;
	
	/* error_reporter initialization. */
	error_reporter = &micro_error_reporter;
	
	/* model initialization. */
	model = tflite::GetModel(g_model);

	/* New MicroInterpreter object. */
	static MicroInterpreter static_interpreter(model, micro_op_resolver, tensor_arena, k_tensor_arena_size, error_reporter);
	
	/* interpreter initialization. */
	interpreter = &static_interpreter;
	
	/* Memory allocation for all the necessary tensors. */
	interpreter->AllocateTensors();

	/* input initialization. */
	input = interpreter->input(0);
	
	/* output initialization. */
	output = interpreter->output(0);
	
	/* accel initialization. */
	accel.accel_config();
	
	/* gyo initialization. */
	gyro.gyro_config();
	
	/* curr_sample initialization. */
	curr_sample = 0;
}

/* loop function used to run the application. */
void loop()
{
	/* Start to sample and store values. */
	sampling_thread.start(sample);
}

/* sample function used to sample and store new accelerometer and gyroscope values. */
void sample()
{
	/* Local variable used to store the initial time of the current sampling step. */
	std::clock_t t_0;
	
	/* Local variable used to store the duration of the current sampling step. */
	float duration;
	
	/* Infinite loop that allows infinite sampling. */
	while(true)
	{
		/* Store the initial time. */
		t_0 = std::clock();
		
		/* Sample from accelerometer. */
		accel.acquire_accel_data_g(accel_data[curr_sample]);
		
		/* Sample from gyroscope. */
		gyro.acquire_gyro_data_dps(gyro_data[curr_sample]);
			
		/* Update of the current sample. */
		curr_sample++;
		
		/* Eventually run the inference. */
			
		/* Get the duration of the current sampling step. */
	    duration = (std::clock() - t_0) / (float)CLOCKS_PER_SEC;
	    
		/* Check if duration if less than 20ms. */
		if(duration < 0.02)
			
			/* Wait for (20ms - duration)ms. */
			wait(0.02 - duration);
	}
}