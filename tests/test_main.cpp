#include <gtest/gtest.h>
#include <hpx/init_runtime_local/init_runtime_local.hpp>

namespace {
int runTests() {
	int const result = RUN_ALL_TESTS();
	hpx::local::finalize();
	return result;
}
} // namespace
int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	hpx::local::init_params parameters;
	parameters.cfg = {"hpx.os_threads=1", "hpx.stacks.small_size=0x400000",
					  "hpx.commandline.allow_unknown=0"};
	return hpx::local::init(&runTests, argc, argv, parameters);
}
