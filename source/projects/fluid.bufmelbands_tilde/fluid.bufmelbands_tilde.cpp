#include <clients/rt/MelBandsClient.hpp>
#include <FluidMaxWrapper.hpp>

void ext_main(void*)
{
  using namespace fluid::client;
  makeMaxWrapper<NRTMelBandsClient>("fluid.bufmelbands~");
}