#include <clients/rt/LoudnessClient.hpp>
#include <FluidMaxWrapper.hpp>

void ext_main(void*)
{
  using namespace fluid::client;
  makeMaxWrapper<LoudnessClient>("fluid.loudness~");
}