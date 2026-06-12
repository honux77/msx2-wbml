# WBML Sound Effect Debug Notes

Date: 2026-06-11

## Symptom

- Boot/start jingle should be a short melodic "pyororong" sound, but the port currently plays a stretched/simple tone.
- Coin insert SFX should be a very short "tiriring" sound, but currently becomes long and dragged out.
- User-provided MAME path: `D:\Games\mame0277b`

## Current Local Change State

Current working change is mainly in `src/win-ref/machine.c`.

Important local changes made during this investigation:

- PPI sound latch handling was changed from unconditional NMI pulse to a more MAME-like active-low OBF handshake model.
- `sound_control_w` now treats PPI port C bit 7 as active-low sound CPU NMI.
- Main CPU `OUT 0x14` writes the sound latch and lowers PPI port C bits 7/6.
- Sound CPU read from `E000` acknowledges the latch by restoring port C bits 7/6 high.
- `machine_init` initializes `ppi_portc` to `0xC0`.
- Diagnostic logging was added around command writes and SN76489 writes.
- `g_frame` was added as a diagnostic frame counter.

No intended Z80 core fix has been committed/applied yet.

## Key MAME Facts Confirmed

MAME source references checked in `refs/system1.cpp`:

- `MASTER_CLOCK = 20 MHz`
- Main Z80 clock for sys1ppi: `MASTER_CLOCK / 5 = 4 MHz`
- Sound Z80 clock: `SOUND_CLOCK / 2 = 4 MHz`
- Sound IRQ timer: scanline 32, period 64, effectively 4 interrupts per frame.
- System 2 / WBML uses `sys1ppis`, with PPI port C callback `.exor(0x01)` for system mute.
- MAME PPI sound path:
  - Main CPU writes PPI port A through `soundport_w`.
  - `soundport_w` writes `m_soundlatch` and gives scheduler `perfect_quantum(100us)`.
  - Sound CPU reads latch at `E000`.
  - `sound_data_r` toggles PPI port C bit 6 low/high as ACK.

## MAME Instrumentation Used

Lua scripts were created under `build/`:

- `build/mame_probe.lua`
- `build/mame_introspect.lua`
- `build/mame_soundtap.lua`
- `build/mame_slot_tap.lua`

These scripts are diagnostic only and can be removed later.

Useful MAME command pattern:

```powershell
& 'D:\Games\mame0277b\mame.exe' wbml `
  -rompath 'D:\Games\mame0277b\roms;C:\Users\k5020\hobby-projects\msx2-wbml\roms' `
  -skip_gameinfo -nothrottle -video none -sound none -seconds_to_run 5 `
  -autoboot_script 'C:\Users\k5020\hobby-projects\msx2-wbml\build\mame_soundtap.lua'
```

Useful MAME device tags found:

- Main CPU: `:maincpu`
- Sound CPU: `:soundcpu`
- PPI: `:ppi8255`
- SN chips: `:sn1`, `:sn2`

## Most Important Comparison Result

MAME boot sound tap result:

```text
cmd frame=81 time=1.000000000 offset=0014 data=17 mask=255
sn frame=81 time=1.000000000 addr=C000 data=82
sn frame=81 time=1.000000000 addr=C000 data=0B
sn frame=81 time=1.000000000 addr=C000 data=90
sn frame=81 time=1.000000000 addr=C000 data=A2
sn frame=81 time=1.000000000 addr=C000 data=0B
sn frame=81 time=1.000000000 addr=C000 data=B0
```

Local emulator after PPI handshake fix:

```text
[cmd] f=76 ... val=17
[sn2] f=76 ... 80
[sn2] f=76 ... 00
[sn2] f=76 ... 80
[sn2] f=76 ... A0
[sn2] f=76 ... 00
[sn2] f=76 ... A0
...
[sn2] f=128 ... 82
[sn2] f=128 ... 0B
[sn2] f=128 ... 90
[sn2] f=128 ... A2
[sn2] f=128 ... 0B
[sn2] f=128 ... B0
```

Interpretation:

- MAME sends command `0x17` and immediately writes the expected first audible SN2 sequence.
- Local emulator receives command `0x17`, but first writes `80 00 80 A0 00 A0`, then does not emit the MAME-like `82 0B 90 A2 0B B0` sequence until about 52 frames later.
- Therefore, the main CPU command timing is probably not the primary issue. The problem is likely in sound CPU execution, sound driver state, or Z80 opcode behavior around the sound driver.

## Sound ROM Details

Sound ROM: `roms\wbml\epr-11037.126`

Important decoded behavior:

- NMI handler at `0x0066` reads latch from `E000`.
- If `8012 != 0`, it stores the command in `801D`.
- Otherwise it queues the command into ring buffer at `8020`.
- Main sound loop:
  - processes queue at `00E0`
  - calls `03AF`
  - calls `021E`
  - calls `0290`
  - waits for IRQ flag bit 0 at `8000`

Command table:

- Command `0x17` points to data at `0x43EA`.
- Data begins:

```text
02 02 0D F3 43 0C F5 43 FF F2 02 E0 06 E6 F8 C4 ...
```

Command initialization for `0x17` activates slot index `0x0D`, base `0x82F0`.

Observed local slot initialization around command:

```text
82F0=90
82F1=00
82F5=10
82FE=00
82F6=17
82F7=02
82F8=01
82F9=00
82FA=01
82FB=00
8300=AC
8301=08
8315=00
82FC=F3
82FD=43
```

MAME RAM tap showed very similar slot initialization for the same command.

## Suspicious Area

The local emulator appears to wait for a global/tempo counter around `8072` to count down from roughly `D1` before producing the expected first note.

Relevant sound-driver routine around `0x021E`:

- Loops over 16 slots from `IX=8080`.
- Tests slot active bit and mode bits using `DD CB` indexed bit instructions.
- At `0256`, decrements `(IX+14)`.
- If zero, reads next event from pointer `(IX+12)/(IX+13)`.
- Stores transformed delay/value back into `(IX+14)`.

Potential root causes to investigate next:

- Z80 `DD CB` / `FD CB` indexed bit behavior, especially flags and memory/register writeback.
- Z80 interrupt timing around EI/NMI/RETN/IRQ. NMI currently does not copy `IFF1` to `IFF2` before clearing `IFF1`; this may matter.
- Whether `0x03AF` or `0x021E` sees different flags/registers locally vs MAME immediately after command `0x17`.
- Whether MAME's `perfect_quantum(100us)` after latch write lets sound CPU process command in the same scheduler slice in a way our frame loop does not.
- Whether PPI port C bit 0 xor/system mute should be modeled for sys2/WBML. This likely affects mute state, but the delayed SN write sequence points more strongly at sound-driver execution/state.

## Z80 Core Notes

Files inspected:

- `src/win-ref/z80/z80.c`
- `src/win-ref/z80/z80.h`

Observations:

- `exec_opcode_ddfd` has support for indexed loads/stores/arithmetic and `DD CB/FDCB`.
- `exec_opcode_dcb` writes back memory for non-BIT operations and updates target registers for undocumented forms when `z_ != 6`.
- `DDCB` BIT sets X/Y flags from high byte of indexed address.
- `process_interrupts` handles NMI by clearing `IFF1`, but does not save old `IFF1` into `IFF2` first. Real Z80 NMI preserves old `IFF1` in `IFF2`, and `RETN` restores it.

This NMI/IFF behavior is worth trying because the sound CPU uses NMI for latch and `RETN` in the handler.

## Useful Local Commands

Build debug executable:

```powershell
gcc -O2 -std=c11 -Wall -Wextra -Isrc/win-ref -Isrc/win-ref/z80 `
  src/win-ref/dbg.c src/win-ref/machine.c src/win-ref/mc8123.c `
  src/win-ref/video.c src/win-ref/sn76489.c src/win-ref/z80/z80.c `
  -o build/dbg.exe
```

Run local SN log:

```powershell
$env:SN_LOG='1'
.\build\dbg.exe roms\wbml 160 0 play *> build\our_boot_snlog_current.txt
Remove-Item Env:\SN_LOG
```

Build SDL executable:

```powershell
gcc -O2 -std=c11 -Wall -Wextra -Ithird_party/SDL2/include/SDL2 -Isrc/win-ref/z80 `
  src/win-ref/main.c src/win-ref/machine.c src/win-ref/mc8123.c `
  src/win-ref/video.c src/win-ref/sn76489.c src/win-ref/z80/z80.c `
  -o build/wbml-ref.exe -Lthird_party/SDL2/lib -lmingw32 -lSDL2main -lSDL2
```

Generate MAME WAV for audio comparison:

```powershell
& 'D:\Games\mame0277b\mame.exe' wbml `
  -rompath 'D:\Games\mame0277b\roms;C:\Users\k5020\hobby-projects\msx2-wbml\roms' `
  -skip_gameinfo -nothrottle -video none -sound xaudio2 -seconds_to_run 5 `
  -wavwrite 'C:\Users\k5020\hobby-projects\msx2-wbml\build\mame_boot.wav'
```

## Recommended Next Session Plan

1. Keep the current PPI handshake fix as a base.
2. Add a temporary local trace for sound CPU state at:
   - command `0x17` NMI handler exit
   - first entry to `00E0`
   - entry/exit of `03AF`
   - entry/exit of `021E`
   - first SN2 write after command
3. Mirror the same state points in MAME using Lua memory taps and possibly CPU state reads.
4. First quick patch to try:
   - On NMI service, set `iff2 = iff1` before clearing `iff1`.
5. If that does not fix it, focus on indexed `DDCB` handling and flags around the sound-driver slot tests.
6. After fix, compare:
   - MAME `0x17` command immediate SN2 writes
   - local `0x17` command immediate SN2 writes
   - boot WAV active frame envelope
   - coin insert SFX length

