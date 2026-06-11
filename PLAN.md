# Wonder Boy: Monster Land → MSX2+ 포팅 계획

## 목표

MAME의 `wbml` (Wonder Boy: Monster Land, Sega System 2, 1987) 을 MSX2+ 에서
돌아가는 게임으로 만든다. 2단계 전략으로 진행한다.

1. **Phase A — Windows 레퍼런스 에뮬레이터** (현재 진행)
   복호화한 실제 ROM을 그대로 실행하는, wbml 전용 미니 에뮬레이터를 Windows에
   만든다. 게임 로직을 100% 정확히 재현하는 "골든 레퍼런스"이자, 그래픽/사운드
   변환 파이프라인의 개발·검증대.

2. **Phase B — MSX2+ 리메이크** (이후)
   Phase A에서 동작을 관찰·문서화한 게임 로직을 MSX2+ (V9958) 에 맞게 C(SDCC)로
   재구현한다. 변환된 에셋을 사용하고, Phase A를 1:1 비교 기준으로 삼는다.

## 원본 하드웨어 (Sega System 2, wbml)

| 구성 | 내용 |
|---|---|
| 메인 CPU | Z80 @ 4MHz (20MHz/5), **NEC MC-8123 암호화** (317-0043 키) |
| 사운드 CPU | Z80 @ 4MHz (8MHz/2) |
| 사운드 칩 | SN76489A × 2 (clock 2MHz, 4MHz) |
| I/O | 8255 PPI (포트 A=사운드 래치, B=videomode/뱅킹, C=사운드 컨트롤) |
| 비디오 | 타일맵 8페이지(고정 fg=page0, 스크롤 bg=4페이지 선택) + 32 스프라이트 |
| 화면 | 512×224, ~60.06Hz, 256색 팔레트 (팔레트 PROM 3개 RGBx4bit) |
| 보호 | MC-8123 암호화뿐. **8751 MCU 없음** (wbml은 sys2x = PPI 기반) |
| ROM | 메인 128KB(0x20000) / 타일 96KB / 스프라이트 128KB / PROM |

### 메모리 맵 (메인 CPU)
```
0000-7fff  bankr bank0d   (복호화 opcode) / bank0  (data)   ※ MC-8123: 페치/데이터 분리
8000-bfff  bankr bank1    (뱅크 ROM, 0x10000~ 영역에서 4뱅크 선택)
c000-cfff  RAM
d000-d7ff  spriteram
d800-dfff  paletteram (write 시 팔레트 갱신)
e000-efff  videoram (뱅크 선택, 8페이지)
f000-f3ff  mixer collision  r/w
f400-f7ff  mixer collision reset
f800-fbff  sprite collision r/w
fc00-ffff  sprite collision reset
```
뱅킹: `bank0c_custom_w` — 뱅크 비트는 data의 bit3,2 → bank1 (그리고 bank1d) 4뱅크.

### I/O 맵 (PPI, global_mask 0x1f)
```
00 P1   04 P2   08 SYSTEM   0c SWA(DIP2)   0d/10 SWB(DIP1)   14-17 8255
```

### 인터럽트
- 메인: VBLANK → IRQ (HOLD_LINE), IM1 (RST 38)
- 사운드: 스캔라인 타이머(32V,96V…)로 IRQ + PPI 포트C bit7로 NMI 제어

## MC-8123 복호화 (완료)

`tools/mc8123.py` — MAME `mc8123.cpp` (Salmoria 알고리즘 / Widel 테이블) 의 충실한
포트. opcode 페치와 data 읽기가 서로 다른 키 테이블 절반(0x0000 / 0x1000)을 쓴다.
주소 12비트(0xFD57 마스크)로 테이블을 선택.

출력: `build/main_opcodes.bin`, `build/main_data.bin` (각 0x20000).
검증: 부팅 시퀀스 `di / ld sp,C800 / jp 034F`, 폰트 타일·"PUSH START BUTTON"
문자열 정상 디코드 확인.

## 에셋 추출 (완료, 변환은 Phase B)

`tools/extract_gfx.py`:
- `build/tiles.png` — 4096개 8×8 3bpp 타일 (3개 plane ROM)
- `build/sprites_raw.png` — 스프라이트 4bpp 니블 스트림 raw 뷰
- `build/palette_proms.png` — 팔레트 PROM 색역 (234색)

## Phase A 구현 (Windows 레퍼런스)

- **언어/빌드**: C, MinGW gcc, `make`. SDL2는 `third_party/SDL2`에 벤더링.
- **Z80 코어**: superzazu/z80 (MIT). MC-8123용 `fetch_opcode` 훅 추가 완료
  (M1 페치 = 메인 opcode + CB/ED/DD/FD 프리픽스 후 opcode; DDCB 변위/opcode는 데이터).
- 구성 파일:
  - `src/win-ref/z80/` — Z80 코어 (opcode 페치 훅 패치됨)
  - `src/win-ref/machine.*` — 메모리맵, 뱅킹, PPI, I/O, 두 Z80 연결
  - `src/win-ref/video.*` — System 2 렌더러 (system1_v.cpp 포트)
  - `src/win-ref/sn76489.*` — SN76489A PSG (MAME 충실 포팅, LFSR feedback 0x10000/탭 비트2,3)
  - `src/win-ref/main.c` — SDL2 윈도우/입력/오디오/60Hz 루프
- **오디오 파이프라인**: 프레임당 8슬라이스로 main+sound Z80 인터리브, 슬라이스마다 SN 샘플 생성 →
  44.1kHz 모노 → SDL_QueueAudio. SN1=2MHz(gain0.40), SN2=4MHz(gain0.60).

## Phase B 구현 (MSX2+ 리메이크) — 추후 상세화

- 툴체인: SDCC + libmsx/fusion-c, 테스트 openMSX
- 그래픽: 타일/스프라이트를 V9958 스크린 모드에 맞게 변환 (색 수·스프라이트 제약 대응)
- 사운드: SN76489 BGM → PSG(AY-3-8910) 편곡/변환
- 로직: Phase A에서 디스어셈블·관찰한 맵/적/충돌/점수 규칙을 재구현
- 검증: Phase A와 동일 입력 시 동일 거동인지 비교

## 진행 현황

- [x] ROM 추출, MC-8123 복호화 도구 + 검증
- [x] 그래픽/팔레트 추출 도구
- [x] Z80 코어 확보 + opcode/data 분리 페치 패치
- [x] SDL2 벤더링
- [x] Phase A: machine (메모리맵/뱅킹/PPI/I/O) + video (System 2 렌더러) + host (SDL2)
- [x] Phase A 플레이 가능 검증 — 타이틀 화면 + 어트랙트 데모 정상 (build/dbg_300.png, f1100.png)
- [x] Phase A 사운드: 사운드 Z80 + SN76489A ×2 — MAME sn76496.cpp 충실 포팅, 두 CPU 인터리브
      실행으로 래치 핸드셰이크/음정 타이밍 정합. 타이틀 음악 E옥타브 정상 (build/dbg.wav로 검증)
- [ ] Phase B 착수

### 디버깅 도구
- `tools/disasm.py <addr_hex> <count>` — MC-8123 디코드 ROM 디스어셈블 (opcode=op뷰, operand=data뷰)
- `src/win-ref/dbg.c` → `build/dbg.exe <romdir> <frames> [steps] [pc_filter]` — 헤드리스 실행 +
  CPU/비디오 상태 덤프 + 명령 트레이스 + `build/dbg.ppm` 프레임 저장

### 핵심 교훈 (해결한 버그)
- 8255 PPI 포트 B/C는 출력 포트지만 **읽으면 출력 래치를 반환**해야 함. 0xFF를 반환하면
  뱅크 선택 루틴(0x10A3)의 read-modify-write가 blank/flip 비트를 오염시켜 화면이 켜지지 않음.
