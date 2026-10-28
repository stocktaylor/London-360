// london - mid-asm hooks referenced from london_manifest.toml ([[midasm_hook]]).
//
// Codegen emits `extern void <name>(...);` and calls the hook at the given
// guest address, so these must stay at global scope with these exact names.

#include <cstdint>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>

REXCVAR_DEFINE_BOOL(london_60fps, true, "London",
                    "Present every vblank instead of every second one (60 fps at 60 Hz; the "
                    "360 version is capped at 30, the PC version runs at 60)");

namespace {
// Input manager (the object at this+8256 in sub_82288BD0). +280 is "Kinect
// ready": its update (sub_82289BA0) then moves from mode 0 to mode 3, which
// the scripts read as GestureRecogniser:IsTracking().
constexpr uint32_t kInputManagerOffset = 8256;
constexpr uint32_t kKinectReadyOffset = 280;
}  // namespace

// Kinect start-up (sub_82288BD0, hook at 0x82288BE0, before NuiInitialize).
//
// Every event's start screen (olympics/hud/hudinput.lua HUDInput:WAIT) waits
// in `while not GestureRecogniser:IsTracking() do SKIP() end` before it
// creates its A/Y button patterns, so the controller does nothing there until
// the input manager's Kinect-ready flag is set. Only this thread sets it, after
// NuiInitialize and skeleton tracking start. That needs XAM's Kinect service,
// which neither rexglue nor Xenia emulates: with the SDK's XamContentOpenFile
// and LDI* fixes NuiInitialize gets as far as XAM's NUI messages (0x2A005,
// 0x2C009, ...) and then waits forever on the boot logo; without them it fails
// and the event screens ignore input (Xenia game-compatibility #783).
//
// Skip the Kinect start-up, set the flag the way success would, and leave
// through the function's own failure exit (0x82288C04, which restores the
// stack). The per-frame mode-3 update only touches Kinect when its frame
// source (*0x82ECFB50) exists, and that is only created further down this
// function, so it stays idle.
void LondonSkipKinectInit(PPCRegister& r26) {
  auto* memory = rex::system::kernel_memory();
  uint32_t input_manager = rex::memory::load_and_swap<uint32_t>(
      memory->TranslateVirtual<uint8_t*>(r26.u32 + kInputManagerOffset));
  if (!input_manager) {
    REXLOG_WARN("LondonSkipKinectInit: no input manager");
    return;
  }
  *memory->TranslateVirtual<uint8_t*>(input_manager + kKinectReadyOffset) = 1;
  REXLOG_INFO("Kinect start-up skipped; input manager {:08X} marked Kinect-ready",
              input_manager);
}

// D3D swap scheduling (sub_825BDD40, hook at 0x825BDD50 once the swap token is
// in r30). The GPU's swap interrupt hands D3D a token
//   frontbuffer address (bits 12-31) | swap interval << 8 | present-immediate
//   threshold (bits 0-7),
// and the flip is queued for (previous flip's vblank + interval); the vblank
// handler (sub_825BDC10) performs it once the vblank count gets there. London
// presents with interval 2, i.e. every second vblank: 30 fps at 60 Hz. Its game
// update already uses the measured frame time, so presenting every vblank just
// doubles the frame rate (the PC version runs at 60).
void LondonSwapInterval(PPCRegister& r30) {
  if (!REXCVAR_GET(london_60fps)) {
    return;
  }
  uint32_t token = r30.u32;
  if (((token >> 8) & 0xF) == 2) {
    r30.u64 = (token & ~0xF00u) | (1u << 8);
  }
}
