#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <cpuid.h>

#include <perf_counter.h>

#include <nnpack.h>

extern unsigned long long median(unsigned long long array[], size_t length);
extern struct nnp_profile median_profile(struct nnp_profile array[], size_t length);
extern void read_memory(const void* memory, size_t length);

enum mode {
	mode_output,
	mode_input_gradient,
	mode_kernel_gradient,
	mode_inference,
};

struct nnp_profile benchmark_convolution(
	enum mode mode,
	const void* memory, size_t cache_size,
	enum nnp_convolution_algorithm algorithm,
	enum nnp_convolution_kernel_transform_strategy kernel_transform_strategy,
	size_t batch_size,
	size_t input_channels,
	size_t output_channels,
	struct nnp_size input_size,
	struct nnp_padding input_padding,
	struct nnp_size kernel_size,
	float input[],
	float kernel[],
	const float bias[],
	float output[],
	pthreadpool_t threadpool,
	size_t max_iterations)
{
	struct nnp_profile computation_profile[max_iterations];
	for (size_t iteration = 0; iteration < max_iterations; iteration++) {
		read_memory(memory, cache_size);
		switch (mode) {
			case mode_output:
				nnp_convolution_output(
					algorithm,
					batch_size,
					input_channels,
					output_channels,
					input_size,
					input_padding,
					kernel_size,
					input,
					kernel,
					bias,
					output,
					threadpool,
					&computation_profile[iteration]);
				break;
			case mode_input_gradient:
				nnp_convolution_input_gradient(
					algorithm,
					batch_size,
					input_channels,
					output_channels,
					input_size,
					input_padding,
					kernel_size,
					output,
					kernel,
					input,
					threadpool,
					&computation_profile[iteration]);
				break;
			case mode_kernel_gradient:
				nnp_convolution_kernel_gradient(
					algorithm,
					batch_size,
					input_channels,
					output_channels,
					input_size,
					input_padding,
					kernel_size,
					input,
					output,
					kernel,
					threadpool,
					&computation_profile[iteration]);
				break;
			case mode_inference:
				nnp_convolution_inference(
					algorithm,
					kernel_transform_strategy,
					input_channels,
					output_channels,
					input_size,
					input_padding,
					kernel_size,
					input,
					kernel,
					bias,
					output,
					threadpool,
					&computation_profile[iteration]);
				break;
		}
	}
	return median_profile(computation_profile, max_iterations);
}

unsigned long long profile_convolution_output(
	enum nnp_convolution_algorithm algorithm,
	size_t batch_size,
	size_t input_channels,
	size_t output_channels,
	struct nnp_size input_size,
	struct nnp_padding input_padding,
	struct nnp_size kernel_size,
	const float input[],
	const float kernel[],
	const float bias[],
	float output[],
	int perf_counter_file_descriptor, size_t max_iterations)
{
	unsigned long long overhead_count[max_iterations];
	size_t overhead_samples = 0;
	for (size_t iteration = 0; iteration < max_iterations; iteration++) {
		unsigned long long start_count = 0, end_count = 0;
		if (!read_perf_counter(perf_counter_file_descriptor, &start_count))
			continue;

		unsigned int eax, ebx, ecx, edx;
		__cpuid(0, eax, ebx, ecx, edx);
		__cpuid(0, eax, ebx, ecx, edx);

		if (!read_perf_counter(perf_counter_file_descriptor, &end_count))
			continue;

		overhead_count[overhead_samples++] = end_count - start_count;
	}

	/* Performance counters aren't working */
	if (overhead_samples == 0)
		return ULLONG_MAX;

	unsigned long long computation_count[max_iterations];
	size_t computation_samples = 0;
	for (size_t iteration = 0; iteration < max_iterations; iteration++) {
		unsigned long long start_count = 0, end_count = 0;
		if (!read_perf_counter(perf_counter_file_descriptor, &start_count))
			continue;

		unsigned int eax, ebx, ecx, edx;
		__cpuid(0, eax, ebx, ecx, edx);

		nnp_convolution_output(
			algorithm,
			batch_size,
			input_channels,
			output_channels,
			input_size,
			input_padding,
			kernel_size,
			input,
			kernel,
			bias,
			output,
			NULL,
			NULL);

		__cpuid(0, eax, ebx, ecx, edx);

		if (!read_perf_counter(perf_counter_file_descriptor, &end_count))
			continue;

		computation_count[computation_samples++] = end_count - start_count;
	}

	const unsigned long long median_overhead_count = median(overhead_count, overhead_samples);
	const unsigned long long median_computation_count = median(computation_count, computation_samples);

	if (median_computation_count > median_overhead_count)
		return median_computation_count - median_overhead_count;
	else
		return 0;
}

struct options {
	enum mode mode;
	size_t batch_size;
	size_t input_channels;
	size_t output_channels;
	struct nnp_size input_size;
	size_t input_padding;
	struct nnp_size kernel_size;
	enum nnp_convolution_algorithm algorithm;
	enum nnp_convolution_kernel_transform_strategy kernel_transform_strategy;
	size_t threads;
	size_t iterations;
	bool hardware_events;
	bool threadpool;
};

static void print_options_help(const char* program_name) {
	printf(
"%s parameters...\n"
"Required parameters:\n"
"  -ic  --input-channels     The number of input channels\n"
"  -oc  --output-channels    The number of output channels\n"
"  -is  --input-size         Input height and width\n"
"  -os  --kernel-size        Kernel height and width\n"
"Optional parameters:\n"
"  -m   --mode               The convolution mode (output, inference)\n"
"  -a   --algorithm          The algorithm (auto, ft8x8, ft16x16, or wt8x8) for computing convolution (default: auto)\n"
"  -kt  --kernel-transform   The kernel transform strategy (recompute, reuse, precompute) in inference mode (default: recompute)\n"
"  -b   --batch              The size of a minibatch (default: 1)\n"
"  -p   --padding            Implicit input padding (default: 0)\n"
"  -t   --threads            The number of threads (default: all; 0 to disable threadpool)\n"
"  -i   --iterations         # iterations (default: 3)\n"
"  -e   --hardware-events    Collect hardware events for the kernel\n",
		program_name);
}

static struct options parse_options(int argc, char** argv) {
	struct options options = {
		.mode = mode_output,
		.batch_size = 1,
		.input_channels = 0,
		.output_channels = 0,
		.input_size = { 0, 0 },
		.input_padding = 0,
		.kernel_size = { 0, 0 },
		.algorithm = nnp_convolution_algorithm_auto,
		.kernel_transform_strategy = nnp_convolution_kernel_transform_strategy_recompute,
		.threads = 0,
		.iterations = 3,
		.hardware_events = false,
		.threadpool = true,
	};
	for (int argi = 1; argi < argc; argi += 1) {
		if ((strcmp(argv[argi], "--batch") == 0) || (strcmp(argv[argi], "-b") == 0)) {
			if (argi + 1 == argc) {
				fprintf(stderr, "Error: expected batch value\n");
				exit(EXIT_FAILURE);
			}
			if (sscanf(argv[argi + 1], "%zu", &options.batch_size) != 1) {
				fprintf(stderr, "Error: can not parse %s as an unsigned integer\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			if (options.batch_size == 0) {
				fprintf(stderr, "Error: invalid value %s for the batch size: positive value expected\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			argi += 1;
		} else if ((strcmp(argv[argi], "--input-channels") == 0) || (strcmp(argv[argi], "-ic") == 0)) {
			if (argi + 1 == argc) {
				fprintf(stderr, "Error: expected input channels value\n");
				exit(EXIT_FAILURE);
			}
			if (sscanf(argv[argi + 1], "%zu", &options.input_channels) != 1) {
				fprintf(stderr, "Error: can not parse %s as an unsigned integer\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			if (options.input_channels == 0) {
				fprintf(stderr, "Error: invalid value %s for the number of input channels: positive value expected\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			argi += 1;
		} else if ((strcmp(argv[argi], "--output-channels") == 0) || (strcmp(argv[argi], "-oc") == 0)) {
			if (argi + 1 == argc) {
				fprintf(stderr, "Error: expected output channels value\n");
				exit(EXIT_FAILURE);
			}
			if (sscanf(argv[argi + 1], "%zu", &options.output_channels) != 1) {
				fprintf(stderr, "Error: can not parse %s as an unsigned integer\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			if (options.output_channels == 0) {
				fprintf(stderr, "Error: invalid value %s for the number of output channels: positive value expected\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			argi += 1;
		} else if ((strcmp(argv[argi], "--input-size") == 0) || (strcmp(argv[argi], "-is") == 0)) {
			if (argc - argi < 2) {
				fprintf(stderr, "Error: expected two input size values\n");
				exit(EXIT_FAILURE);
			}
			if (sscanf(argv[argi + 1], "%zu", &options.input_size.height) != 1) {
				fprintf(stderr, "Error: can not parse %s as an unsigned integer\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			if (options.input_size.height == 0) {
				fprintf(stderr, "Error: invalid value %s for the input height: positive value expected\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			if (sscanf(argv[argi + 2], "%zu", &options.input_size.width) != 1) {
				fprintf(stderr, "Error: can not parse %s as an unsigned integer\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			if (options.input_size.width == 0) {
				fprintf(stderr, "Error: invalid value %s for the input width: positive value expected\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			argi += 2;
		} else if ((strcmp(argv[argi], "--kernel-size") == 0) || (strcmp(argv[argi], "-ks") == 0)) {
			if (argc - argi < 2) {
				fprintf(stderr, "Error: expected two kernel size values\n");
				exit(EXIT_FAILURE);
			}
			if (sscanf(argv[argi + 1], "%zu", &options.kernel_size.height) != 1) {
				fprintf(stderr, "Error: can not parse %s as an unsigned integer\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			if (options.kernel_size.height == 0) {
				fprintf(stderr, "Error: invalid value %s for the kernel height: positive value expected\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			if (sscanf(argv[argi + 2], "%zu", &options.kernel_size.width) != 1) {
				fprintf(stderr, "Error: can not parse %s as an unsigned integer\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			if (options.kernel_size.width == 0) {
				fprintf(stderr, "Error: invalid value %s for the kernel width: positive value expected\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			argi += 2;
		} else if ((strcmp(argv[argi], "--input-padding") == 0) || (strcmp(argv[argi], "-ip") == 0)) {
			if (argi + 1 == argc) {
				fprintf(stderr, "Error: expected padding value\n");
				exit(EXIT_FAILURE);
			}
			if (sscanf(argv[argi + 1], "%zu", &options.input_padding) != 1) {
				fprintf(stderr, "Error: can not parse %s as an unsigned integer\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			argi += 1;
		} else if ((strcmp(argv[argi], "--algorithm") == 0) || (strcmp(argv[argi], "-a") == 0)) {
			if (argi + 1 == argc) {
				fprintf(stderr, "Error: expected convolution algorithm name\n");
				exit(EXIT_FAILURE);
			}
			if (strcmp(argv[argi + 1], "auto") == 0) {
				options.algorithm = nnp_convolution_algorithm_auto;
			} else if (strcmp(argv[argi + 1], "ft8x8") == 0) {
				options.algorithm = nnp_convolution_algorithm_ft8x8;
			} else if (strcmp(argv[argi + 1], "ft16x16") == 0) {
				options.algorithm = nnp_convolution_algorithm_ft16x16;
			} else if (strcmp(argv[argi + 1], "wt8x8") == 0) {
				options.algorithm = nnp_convolution_algorithm_wt8x8;
			} else {
				fprintf(stderr, "Error: invalid convolution algorithm name %s\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			argi += 1;
		} else if ((strcmp(argv[argi], "--kernel-transform") == 0) || (strcmp(argv[argi], "-kt") == 0)) {
			if (argi + 1 == argc) {
				fprintf(stderr, "Error: expected kernel transform strategy\n");
				exit(EXIT_FAILURE);
			}
			if (strcmp(argv[argi + 1], "recompute") == 0) {
				options.kernel_transform_strategy = nnp_convolution_kernel_transform_strategy_recompute;
			} else if (strcmp(argv[argi + 1], "reuse") == 0) {
				options.kernel_transform_strategy = nnp_convolution_kernel_transform_strategy_reuse;
			} else if (strcmp(argv[argi + 1], "precomputed") == 0) {
				options.kernel_transform_strategy = nnp_convolution_kernel_transform_strategy_precomputed;
			} else {
				fprintf(stderr, "Error: invalid kernel transform strategy %s\n", argv[argi]);
				exit(EXIT_FAILURE);
			}
			argi += 1;
		} else if ((strcmp(argv[argi], "--mode") == 0) || (strcmp(argv[argi], "-m") == 0)) {
			if (argi + 1 == argc) {
				fprintf(stderr, "Error: expected convolution mode name\n");
				exit(EXIT_FAILURE);
			}
			if (strcmp(argv[argi + 1], "output") == 0) {
				options.mode = mode_output;
			} else if (strcmp(argv[argi + 1], "input-gradient") == 0) {
				options.mode = mode_input_gradient;
			} else if (strcmp(argv[argi + 1], "kernel-gradient") == 0) {
				options.mode = mode_kernel_gradient;
			} else if (strcmp(argv[argi + 1], "inference") == 0) {
				options.mode = mode_inference;
			} else {
				fprintf(stderr, "Error: invalid value %s for the convolution mode\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			argi += 1;
		} else if ((strcmp(argv[argi], "--threads") == 0) || (strcmp(argv[argi], "-t") == 0)) {
			if (argi + 1 == argc) {
				fprintf(stderr, "Error: expected number of threads value\n");
				exit(EXIT_FAILURE);
			}
			if (sscanf(argv[argi + 1], "%zu", &options.threads) != 1) {
				fprintf(stderr, "Error: can not parse %s as an unsigned integer\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			if (options.threads == 0) {
				options.threadpool = false;
			}
			argi += 1;
		} else if ((strcmp(argv[argi], "--iterations") == 0) || (strcmp(argv[argi], "-i") == 0)) {
			if (argi + 1 == argc) {
				fprintf(stderr, "Error: expected iterations value\n");
				exit(EXIT_FAILURE);
			}
			if (sscanf(argv[argi + 1], "%zu", &options.iterations) != 1) {
				fprintf(stderr, "Error: can not parse %s as an unsigned integer\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			if (options.iterations == 0) {
				fprintf(stderr, "Error: invalid value %s for the number of iterations: positive value expected\n", argv[argi + 1]);
				exit(EXIT_FAILURE);
			}
			argi += 1;
		} else if ((strcmp(argv[argi], "--hardware-events") == 0) || (strcmp(argv[argi], "-e") == 0)) {
			options.hardware_events = true;
		} else if ((strcmp(argv[argi], "--help") == 0) || (strcmp(argv[argi], "-h") == 0)) {
			print_options_help(argv[0]);
			exit(EXIT_SUCCESS);
		} else {
			fprintf(stderr, "Error: unknown argument '%s'\n", argv[argi]);
			print_options_help(argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	if ((options.mode == mode_inference) && (options.batch_size != 1)) {
		fprintf(stderr, "Error: inference requires unit batch size\n");
		exit(EXIT_FAILURE);
	}
	if (options.input_channels == 0) {
		fprintf(stderr, "Error: the number of input channels is not specified\n");
		print_options_help(argv[0]);
		exit(EXIT_FAILURE);
	}
	if (options.output_channels == 0) {
		fprintf(stderr, "Error: the number of output channels is not specified\n");
		print_options_help(argv[0]);
		exit(EXIT_FAILURE);
	}
	if (options.input_size.width == 0) {
		fprintf(stderr, "Error: the input size is not specified\n");
		print_options_help(argv[0]);
		exit(EXIT_FAILURE);
	}
	if (options.kernel_size.width == 0) {
		fprintf(stderr, "Error: the kernel size is not specified\n");
		print_options_help(argv[0]);
		exit(EXIT_FAILURE);
	}
	return options;
}

int main(int argc, char** argv) {
	enum nnp_status init_status = nnp_initialize();
	if (init_status != nnp_status_success) {
		fprintf(stderr, "NNPACK initialization failed: error code %d\n", init_status);
		exit(EXIT_FAILURE);
	}

	const struct options options = parse_options(argc, argv);

	const size_t batch_size = options.batch_size;
	const size_t input_channels = options.input_channels;
	const size_t output_channels = options.output_channels;
	const struct nnp_padding input_padding = { options.input_padding, options.input_padding, options.input_padding, options.input_padding };
	const struct nnp_size input_size = options.input_size;
	const struct nnp_size kernel_size = options.kernel_size;
	const struct nnp_size output_size = {
		.width = input_padding.left + input_size.width + input_padding.right - kernel_size.width + 1,
		.height = input_padding.top + input_size.height + input_padding.bottom - kernel_size.height + 1
	};
	struct nnp_size tile_size;
	double flops_per_element;

	printf("Batch size: %zu\n", batch_size);
	printf("Input channels: %zu\n", input_channels);
	printf("Output channels: %zu\n", output_channels);
	printf("Input: %zux%zu with implicit padding %zu\n", input_size.height, input_size.width, options.input_padding);
	printf("Kernel: %zux%zu\n", kernel_size.height, kernel_size.width);
	switch (options.algorithm) {
		case nnp_convolution_algorithm_auto:
			/* To avoid failure in the next phases */
			tile_size = kernel_size;
			printf("Algorithm: auto\n");
			break;
		case nnp_convolution_algorithm_ft8x8:
			tile_size = (struct nnp_size) { 8, 8 };
			flops_per_element = 4.0;
			printf("Algorithm: FT8x8\n");
			break;
		case nnp_convolution_algorithm_ft16x16:
			tile_size = (struct nnp_size) { 16, 16 };
			flops_per_element = 4.0;
			printf("Algorithm: FT16x16\n");
			break;
		case nnp_convolution_algorithm_wt8x8:
			tile_size = (struct nnp_size) { 8, 8 };
			flops_per_element = 2.0;
			printf("Algorithm: WT8x8\n");
			break;
	}
	const struct nnp_size output_tile_size = {
		.height = tile_size.height - kernel_size.height + 1,
		.width = tile_size.width - kernel_size.width + 1
	};
	const size_t tile_count =
		(output_size.height / output_tile_size.height + !!(output_size.height % output_tile_size.height)) *
		(output_size.width / output_tile_size.width + !!(output_size.width % output_tile_size.width));

	const size_t cache_size = 128 * 1024 * 1024;
	void* memory = NULL;
	if (posix_memalign(&memory, 64, cache_size) != 0) {
		fprintf(stderr, "Error: failed to allocate memory for cache flushing buffer\n");
		exit(EXIT_FAILURE);
	}

	void* input = malloc(batch_size * input_channels * input_size.width * input_size.height * sizeof(float));
	void* kernel = malloc(input_channels * output_channels * kernel_size.width * kernel_size.height * sizeof(float));
	void* output = malloc(batch_size * output_channels * output_size.width * output_size.height * sizeof(float));
	void* bias = malloc(output_channels * sizeof(float));

	memset(input, 0, batch_size * input_channels * input_size.width * input_size.height * sizeof(float));
	memset(kernel, 0, input_channels * output_channels * kernel_size.width * kernel_size.height * sizeof(float));
	memset(output, 0, batch_size * output_channels * output_size.width * output_size.height * sizeof(float));
	memset(bias, 0, output_channels * sizeof(float));

	{
		pthreadpool_t threadpool = NULL;
		if (options.threadpool) {
			threadpool = pthreadpool_create(options.threads);
			printf("Threads: %zu\n", pthreadpool_get_threads_count(threadpool));
		}
		printf("Iterations: %zu\n", options.iterations);

		const struct nnp_profile convolution_profile =
			benchmark_convolution(
				options.mode,
				memory, cache_size,
				options.algorithm,
				options.kernel_transform_strategy,
				batch_size, input_channels, output_channels,
				input_size, input_padding, kernel_size,
				input, kernel, bias, output,
				threadpool, options.iterations);
		const double convolution_time = convolution_profile.total;

		const size_t input_transform_footprint = sizeof(float) * batch_size * input_channels *
			(input_size.height * input_size.width + tile_count * tile_size.height * tile_size.width);
		const size_t kernel_transform_footprint = sizeof(float) * output_channels * input_channels *
			(kernel_size.height * kernel_size.width + tile_size.height * tile_size.width);
		const size_t output_transform_footprint = sizeof(float)* batch_size * output_channels *
			(output_size.height * output_size.width + tile_count * tile_size.height * tile_size.width);

		printf("Time: %5.3f ms\n", convolution_time * 1.0e+3);
		printf("Input transform: %5.3f ms (%.1f%%) [%.1f GB/s]\n",
			convolution_profile.input_transform * 1.0e+3,
			(convolution_profile.input_transform / convolution_time) * 100.0,
			((double) input_transform_footprint) * 1.0e-9 / convolution_profile.input_transform);
		printf("Kernel transform: %5.3f ms (%.1f%%) [%.1f GB/s]\n",
			convolution_profile.kernel_transform * 1.0e+3,
			(convolution_profile.kernel_transform / convolution_time) * 100.0,
			((double) kernel_transform_footprint) * 1.0e-9 / convolution_profile.kernel_transform);
		printf("Output transform: %5.3f ms (%.1f%%) [%.1f GB/s]\n",
			convolution_profile.output_transform * 1.0e+3,
			(convolution_profile.output_transform / convolution_time) * 100.0,
			((double) output_transform_footprint) * 1.0e-9 / convolution_profile.output_transform);
		if (convolution_profile.block_multiplication != 0.0) {
			if (options.algorithm == nnp_convolution_algorithm_auto) {
				/* We don't know which algorithm is actually used, and thus can't compute FLOPS */
				printf("Block multiplication: %5.3f ms (%.1f%%)\n",
					convolution_profile.block_multiplication * 1.0e+3,
					(convolution_profile.block_multiplication / convolution_time) * 100.0);
			} else {
				printf("Block multiplication: %5.3f ms (%.1f%%) [%.1f GFLOPS]\n",
					convolution_profile.block_multiplication * 1.0e+3,
					(convolution_profile.block_multiplication / convolution_time) * 100.0,
					(flops_per_element * tile_size.height * tile_size.width * tile_count * batch_size * output_channels * input_channels * 1.0e-9) /
						convolution_profile.block_multiplication);
			}
		}
		const double overhead_time = convolution_profile.total -
			(convolution_profile.input_transform +
				convolution_profile.kernel_transform +
				convolution_profile.output_transform +
				convolution_profile.block_multiplication);
		printf("Overhead: %5.3f ms (%.1f%%)\n",
			overhead_time * 1.0e+3, (overhead_time / convolution_time) * 100.0);

		if (threadpool) {
			pthreadpool_destroy(threadpool);
		}
	}

	int failures = 0;
	if (options.hardware_events) {
		size_t performance_counters_count = 0;
		const struct performance_counter* performance_counters =
			init_performance_counters(&performance_counters_count);

		for (size_t i = 0; i < performance_counters_count; i++) {
			if (!enable_perf_counter(performance_counters[i].file_descriptor)) {
				failures++;
				continue;
			}

			unsigned long long count = profile_convolution_output(
				options.algorithm,
				batch_size,
				input_channels,
				output_channels,
				input_size,
				input_padding,
				kernel_size,
				input,
				kernel,
				bias,
				output,
				performance_counters[i].file_descriptor,
				options.iterations);
			if (count == ULLONG_MAX) {
				failures++;
				continue;
			}

			if (!disable_perf_counter(performance_counters[i].file_descriptor)) {
				failures++;
				continue;
			}

			printf("%s: %llu\n", performance_counters[i].name, count);
		}
	}
	return failures;
}
