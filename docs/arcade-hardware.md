# Wonder Boy: Monster Land (wbml) — 아케이드 하드웨어 분석

세가 System 2 기판. 출처: MAME `src/mame/sega/system1.cpp`, `system1_v.cpp`,
`src/devices/cpu/z80/mc8123.cpp` (refs/ 에 사본, BSD-3-Clause).

## CPU 구성

| 구성 | 아케이드 | MSX2+ 대응 |
|---|---|---|
| 메인 CPU | Z80 @ 4MHz (MC-8123 암호화, 키 317-0043) | Z80 @ 3.58MHz (복호화 후 코드 재사용) |
| 사운드 CPU | Z80 @ 4MHz, 별도 주소공간 | 없음 — 별도 전략 필요 |
| 사운드 칩 | SN76489A ×2 | PSG(AY-3-8910) / MSX-MUSIC |

### MC-8123 암호화
- M1(opcode) 페치와 데이터 페치가 **서로 다른 키**로 복호화됨
- 키 인덱스: 주소 비트 0xFD57 (12비트) + opcode/data 플래그 → 8KB 키
- `tools/mc8123.py` 가 `build/main_opcodes.bin`(M1 뷰), `build/main_data.bin`(데이터 뷰) 생성
- **리스팅을 만들 때 명령어 첫 바이트는 opcode 뷰, 피연산자는 data 뷰에서 읽을 것**
- 검증됨: 부트 `di / ld sp,C800 / jp 034F`, 데이터 뷰에서 "PUSH START BUTTON" 등 평문 확인

## 메인 CPU 메모리 맵

| 주소 | 내용 | 비고 |
|---|---|---|
| 0000-7FFF | 고정 ROM (epr-11031a.90) | |
| 8000-BFFF | 뱅크 ROM (16KB ×4) | epr-11032.91 + epr-11033.92, 뱅크 선택은 videomode 비트 2-3 (bank0c) |
| C000-CFFF | 워크 RAM 4KB | SP 초기값 C800 |
| D000-D7FF | 스프라이트 RAM | 32 엔트리 × 16바이트 = 512B (나머지는 여분 RAM) |
| D800-DFFF | 팔레트 RAM | 3×512 엔트리 (스프라이트/FG/BG), 쓰기 시 즉시 반영 |
| E000-EFFF | VRAM 윈도우 4KB | 뱅크드: 전체 8페이지×2KB=16KB, 페이지쌍 선택은 PPI 포트C 상위비트 |
| F000-F3FF | 믹서 충돌 R/W | 읽기: D0=충돌비트, D7=요약 |
| F400-F7FF | 믹서 충돌 요약 리셋 (W) | |
| F800-FBFF | 스프라이트 충돌 R/W | 32×32 스프라이트 쌍 |
| FC00-FFFF | 스프라이트 충돌 요약 리셋 (W) | |

ROM 리