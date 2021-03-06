#include <gtest/gtest.h>

#include <nnpack.h>

#include <testers/convolution.h>

/*
 * Test that implementation works for a single tile of transformation
 */

TEST(FT8x8_RECOMPUTE, single_tile) {
	ConvolutionTester()
		.inputSize(8, 8)
		.iterations(100)
		.errorLimit(1.0e-5)
		.testInference(nnp_convolution_algorithm_ft8x8, nnp_convolution_kernel_transform_strategy_recompute);
}

TEST(FT8x8_REUSE, single_tile) {
	ConvolutionTester()
		.inputSize(8, 8)
		.iterations(100)
		.errorLimit(1.0e-5)
		.testInference(nnp_convolution_algorithm_ft8x8, nnp_convolution_kernel_transform_strategy_reuse);
}

TEST(FT16x16_RECOMPUTE, single_tile) {
	ConvolutionTester()
		.inputSize(16, 16)
		.iterations(100)
		.errorLimit(1.0e-5)
		.testInference(nnp_convolution_algorithm_ft16x16, nnp_convolution_kernel_transform_strategy_recompute);
}

TEST(FT16x16_REUSE, single_tile) {
	ConvolutionTester()
		.inputSize(16, 16)
		.iterations(100)
		.errorLimit(1.0e-5)
		.testInference(nnp_convolution_algorithm_ft16x16, nnp_convolution_kernel_transform_strategy_reuse);
}

TEST(WT8x8_RECOMPUTE, single_tile) {
	ConvolutionTester()
		.inputSize(8, 8)
		.iterations(100)
		.errorLimit(1.0e-3)
		.testInference(nnp_convolution_algorithm_wt8x8, nnp_convolution_kernel_transform_strategy_recompute);
}

TEST(WT8x8_REUSE, single_tile) {
	ConvolutionTester()
		.inputSize(8, 8)
		.iterations(100)
		.errorLimit(1.0e-3)
		.testInference(nnp_convolution_algorithm_wt8x8, nnp_convolution_kernel_transform_strategy_reuse);
}

/*
 * Test that the implementation handles extraction of input subtile
 */

TEST(FT8x8_RECOMPUTE, input_subtile) {
	ConvolutionTester()
		.inputSize(4, 4)
		.errorLimit(1.0e-5)
		.testInference(nnp_convolution_algorithm_ft8x8, nnp_convolution_kernel_transform_strategy_recompute);
}

TEST(FT8x8_REUSE, input_subtile) {
	ConvolutionTester()
		.inputSize(4, 4)
		.errorLimit(1.0e-5)
		.testInference(nnp_convolution_algorithm_ft8x8, nnp_convolution_kernel_transform_strategy_reuse);
}

TEST(FT16x16_RECOMPUTE, input_subtile) {
	ConvolutionTester()
		.inputSize(4, 4)
		.errorLimit(1.0e-5)
		.testInference(nnp_convolution_algorithm_ft16x16, nnp_convolution_kernel_transform_strategy_recompute);
}

TEST(FT16x16_REUSE, input_subtile) {
	ConvolutionTester()
		.inputSize(4, 4)
		.errorLimit(1.0e-5)
		.testInference(nnp_convolution_algorithm_ft16x16, nnp_convolution_kernel_transform_strategy_reuse);
}

TEST(WT8x8_RECOMPUTE, input_subtile) {
	ConvolutionTester()
		.inputSize(4, 4)
		.errorLimit(1.0e-4)
		.testInference(nnp_convolution_algorithm_wt8x8, nnp_convolution_kernel_transform_strategy_recompute);
}

TEST(WT8x8_REUSE, input_subtile) {
	ConvolutionTester()
		.inputSize(4, 4)
		.errorLimit(1.0e-4)
		.testInference(nnp_convolution_algorithm_wt8x8, nnp_convolution_kernel_transform_strategy_reuse);
}

/*
 * Test that the implementation handles multi-tile inputs
 */

TEST(FT8x8_RECOMPUTE, multi_tile) {
	ConvolutionTester()
		.inputSize(13, 13)
		.errorLimit(1.0e-5)
		.testInference(nnp_convolution_algorithm_ft8x8, nnp_convolution_kernel_transform_strategy_recompute);
}

TEST(FT8x8_REUSE, multi_tile) {
	ConvolutionTester()
		.inputSize(13, 13)
		.errorLimit(1.0e-5)
		.testInference(nnp_convolution_algorithm_ft8x8, nnp_convolution_kernel_transform_strategy_reuse);
}

TEST(FT16x16_RECOMPUTE, multi_tile) {
	ConvolutionTester()
		.inputSize(29, 29)
		.errorLimit(1.0e-5)
		.testInference(nnp_convolution_algorithm_ft16x16, nnp_convolution_kernel_transform_strategy_recompute);
}

TEST(FT16x16_REUSE, multi_tile) {
	ConvolutionTester()
		.inputSize(29, 29)
		.errorLimit(1.0e-5)
		.testInference(nnp_convolution_algorithm_ft16x16, nnp_convolution_kernel_transform_strategy_reuse);
}

TEST(WT8x8_RECOMPUTE, multi_tile) {
	ConvolutionTester()
		.inputSize(13, 13)
		.errorLimit(1.0e-3)
		.testInference(nnp_convolution_algorithm_wt8x8, nnp_convolution_kernel_transform_strategy_recompute);
}

TEST(WT8x8_REUSE, multi_tile) {
	ConvolutionTester()
		.inputSize(13, 13)
		.errorLimit(1.0e-3)
		.testInference(nnp_convolution_algorithm_wt8x8, nnp_convolution_kernel_transform_strategy_reuse);
}

/*
 * Test that the implementation handles implicit padding of input
 */

TEST(FT8x8_RECOMPUTE, implicit_padding) {
	ConvolutionTester tester;
	tester.inputSize(8, 8)
		.kernelSize(5, 5)
		.errorLimit(5.0e-2);
	for (size_t paddingTop = 0; paddingTop < tester.kernelHeight(); paddingTop++) {
		for (size_t paddingRight = 0; paddingRight < tester.kernelWidth(); paddingRight++) {
			for (size_t paddingLeft = 0; paddingLeft < tester.kernelWidth(); paddingLeft++) {
				for (size_t paddingBottom = 0; paddingBottom < tester.kernelHeight(); paddingBottom++) {
					tester.inputPadding(paddingTop, paddingRight, paddingLeft, paddingBottom)
						.testInference(nnp_convolution_algorithm_ft8x8, nnp_convolution_kernel_transform_strategy_recompute);
				}
			}
		}
	}
}

TEST(FT8x8_REUSE, implicit_padding) {
	ConvolutionTester tester;
	tester.inputSize(8, 8)
		.kernelSize(5, 5)
		.errorLimit(5.0e-2);
	for (size_t paddingTop = 0; paddingTop < tester.kernelHeight(); paddingTop++) {
		for (size_t paddingRight = 0; paddingRight < tester.kernelWidth(); paddingRight++) {
			for (size_t paddingLeft = 0; paddingLeft < tester.kernelWidth(); paddingLeft++) {
				for (size_t paddingBottom = 0; paddingBottom < tester.kernelHeight(); paddingBottom++) {
					tester.inputPadding(paddingTop, paddingRight, paddingLeft, paddingBottom)
						.testInference(nnp_convolution_algorithm_ft8x8, nnp_convolution_kernel_transform_strategy_reuse);
				}
			}
		}
	}
}

TEST(FT16x16_RECOMPUTE, implicit_padding) {
	ConvolutionTester tester;
	tester.inputSize(16, 16)
		.kernelSize(5, 5)
		.errorLimit(5.0e-2);
	for (size_t paddingTop = 0; paddingTop < tester.kernelHeight(); paddingTop++) {
		for (size_t paddingRight = 0; paddingRight < tester.kernelWidth(); paddingRight++) {
			for (size_t paddingLeft = 0; paddingLeft < tester.kernelWidth(); paddingLeft++) {
				for (size_t paddingBottom = 0; paddingBottom < tester.kernelHeight(); paddingBottom++) {
					tester.inputPadding(paddingTop, paddingRight, paddingLeft, paddingBottom)
						.testInference(nnp_convolution_algorithm_ft16x16, nnp_convolution_kernel_transform_strategy_recompute);
				}
			}
		}
	}
}

TEST(FT16x16_REUSE, implicit_padding) {
	ConvolutionTester tester;
	tester.inputSize(16, 16)
		.kernelSize(5, 5)
		.errorLimit(5.0e-2);
	for (size_t paddingTop = 0; paddingTop < tester.kernelHeight(); paddingTop++) {
		for (size_t paddingRight = 0; paddingRight < tester.kernelWidth(); paddingRight++) {
			for (size_t paddingLeft = 0; paddingLeft < tester.kernelWidth(); paddingLeft++) {
				for (size_t paddingBottom = 0; paddingBottom < tester.kernelHeight(); paddingBottom++) {
					tester.inputPadding(paddingTop, paddingRight, paddingLeft, paddingBottom)
						.testInference(nnp_convolution_algorithm_ft16x16, nnp_convolution_kernel_transform_strategy_reuse);
				}
			}
		}
	}
}

TEST(WT8x8_RECOMPUTE, implicit_padding) {
	ConvolutionTester tester;
	tester.inputSize(8, 8)
		.kernelSize(3, 3)
		.errorLimit(1.0e-1);
	for (size_t paddingTop = 0; paddingTop < tester.kernelHeight(); paddingTop++) {
		for (size_t paddingRight = 0; paddingRight < tester.kernelWidth(); paddingRight++) {
			for (size_t paddingLeft = 0; paddingLeft < tester.kernelWidth(); paddingLeft++) {
				for (size_t paddingBottom = 0; paddingBottom < tester.kernelHeight(); paddingBottom++) {
					tester.inputPadding(paddingTop, paddingRight, paddingLeft, paddingBottom)
						.testInference(nnp_convolution_algorithm_wt8x8, nnp_convolution_kernel_transform_strategy_recompute);
				}
			}
		}
	}
}

TEST(WT8x8_REUSE, implicit_padding) {
	ConvolutionTester tester;
	tester.inputSize(8, 8)
		.kernelSize(3, 3)
		.errorLimit(1.0e-1);
	for (size_t paddingTop = 0; paddingTop < tester.kernelHeight(); paddingTop++) {
		for (size_t paddingRight = 0; paddingRight < tester.kernelWidth(); paddingRight++) {
			for (size_t paddingLeft = 0; paddingLeft < tester.kernelWidth(); paddingLeft++) {
				for (size_t paddingBottom = 0; paddingBottom < tester.kernelHeight(); paddingBottom++) {
					tester.inputPadding(paddingTop, paddingRight, paddingLeft, paddingBottom)
						.testInference(nnp_convolution_algorithm_wt8x8, nnp_convolution_kernel_transform_strategy_reuse);
				}
			}
		}
	}
}

/*
 * Test that the implementation can handle small non-unit number of input channels
 */

TEST(FT8x8_RECOMPUTE, few_input_channels) {
	ConvolutionTester tester;
	tester.inputSize(8, 8)
		.errorLimit(1.0e-5);
	for (size_t inputChannels = 2; inputChannels <= 5; inputChannels++) {
		tester.inputChannels(inputChannels)
			.testInference(nnp_convolution_algorithm_ft8x8, nnp_convolution_kernel_transform_strategy_recompute);
	}
}

TEST(FT8x8_REUSE, few_input_channels) {
	ConvolutionTester tester;
	tester.inputSize(8, 8)
		.errorLimit(1.0e-5);
	for (size_t inputChannels = 2; inputChannels <= 5; inputChannels++) {
		tester.inputChannels(inputChannels)
			.testInference(nnp_convolution_algorithm_ft8x8, nnp_convolution_kernel_transform_strategy_reuse);
	}
}

TEST(FT16x16_RECOMPUTE, few_input_channels) {
	ConvolutionTester tester;
	tester.inputSize(16, 16)
		.errorLimit(1.0e-5);
	for (size_t inputChannels = 2; inputChannels <= 5; inputChannels++) {
		tester.inputChannels(inputChannels)
			.testInference(nnp_convolution_algorithm_ft16x16, nnp_convolution_kernel_transform_strategy_recompute);
	}
}

TEST(FT16x16_REUSE, few_input_channels) {
	ConvolutionTester tester;
	tester.inputSize(16, 16)
		.errorLimit(1.0e-5);
	for (size_t inputChannels = 2; inputChannels <= 5; inputChannels++) {
		tester.inputChannels(inputChannels)
			.testInference(nnp_convolution_algorithm_ft16x16, nnp_convolution_kernel_transform_strategy_reuse);
	}
}

TEST(WT8x8_RECOMPUTE, few_input_channels) {
	ConvolutionTester tester;
	tester.inputSize(8, 8)
		.errorLimit(1.0e-3);
	for (size_t inputChannels = 2; inputChannels <= 5; inputChannels++) {
		tester.inputChannels(inputChannels)
			.testInference(nnp_convolution_algorithm_wt8x8, nnp_convolution_kernel_transform_strategy_recompute);
	}
}

TEST(WT8x8_REUSE, few_input_channels) {
	ConvolutionTester tester;
	tester.inputSize(8, 8)
		.errorLimit(1.0e-3);
	for (size_t inputChannels = 2; inputChannels <= 5; inputChannels++) {
		tester.inputChannels(inputChannels)
			.testInference(nnp_convolution_algorithm_wt8x8, nnp_convolution_kernel_transform_strategy_reuse);
	}
}

/*
 * Test that the implementation can handle small non-unit number of output channels
 */

TEST(FT8x8_RECOMPUTE, few_output_channels) {
	ConvolutionTester tester;
	tester.inputSize(8, 8)
		.errorLimit(1.0e-5);
	for (size_t outputChannels = 2; outputChannels <= 5; outputChannels++) {
		tester.outputChannels(outputChannels)
			.testInference(nnp_convolution_algorithm_ft8x8, nnp_convolution_kernel_transform_strategy_recompute);
	}
}

TEST(FT8x8_REUSE, few_output_channels) {
	ConvolutionTester tester;
	tester.inputSize(8, 8)
		.errorLimit(1.0e-5);
	for (size_t outputChannels = 2; outputChannels <= 5; outputChannels++) {
		tester.outputChannels(outputChannels)
			.testInference(nnp_convolution_algorithm_ft8x8, nnp_convolution_kernel_transform_strategy_reuse);
	}
}

TEST(FT16x16_RECOMPUTE, few_output_channels) {
	ConvolutionTester tester;
	tester.inputSize(16, 16)
		.errorLimit(1.0e-5);
	for (size_t outputChannels = 2; outputChannels <= 5; outputChannels++) {
		tester.outputChannels(outputChannels)
			.testInference(nnp_convolution_algorithm_ft16x16, nnp_convolution_kernel_transform_strategy_recompute);
	}
}

TEST(FT16x16_REUSE, few_output_channels) {
	ConvolutionTester tester;
	tester.inputSize(16, 16)
		.errorLimit(1.0e-5);
	for (size_t outputChannels = 2; outputChannels <= 5; outputChannels++) {
		tester.outputChannels(outputChannels)
			.testInference(nnp_convolution_algorithm_ft16x16, nnp_convolution_kernel_transform_strategy_reuse);
	}
}

TEST(WT8x8_RECOMPUTE, few_output_channels) {
	ConvolutionTester tester;
	tester.inputSize(8, 8)
		.errorLimit(1.0e-3);
	for (size_t outputChannels = 2; outputChannels <= 5; outputChannels++) {
		tester.outputChannels(outputChannels)
			.testInference(nnp_convolution_algorithm_wt8x8, nnp_convolution_kernel_transform_strategy_recompute);
	}
}

TEST(WT8x8_REUSE, few_output_channels) {
	ConvolutionTester tester;
	tester.inputSize(8, 8)
		.errorLimit(1.0e-3);
	for (size_t outputChannels = 2; outputChannels <= 5; outputChannels++) {
		tester.outputChannels(outputChannels)
			.testInference(nnp_convolution_algorithm_wt8x8, nnp_convolution_kernel_transform_strategy_reuse);
	}
}

int main(int argc, char* argv[]) {
	const enum nnp_status init_status = nnp_initialize();
	assert(init_status == nnp_status_success);
	setenv("TERM", "xterm-256color", 0);
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
