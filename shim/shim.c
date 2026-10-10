/*
** The CG50 side of an fx-9860 add-in: the game's own code runs unchanged at 0x00300200 (where
** the fx-9860 OS also loads it) and calls in here for everything that differs between the two
** calculators: OS syscalls (fx_dispatch), the monochrome LCD (shim_lcd_push) and a few routines
** that touch fx-9860 hardware.
*/

#include <stdint.h>
#include <stddef.h>

typedef uint16_t u16;
typedef uint32_t u32;

/* CG50 OS (cg.S) */
void *cg_GetVRAMAddress(void);
void cg_PutDisp_DD(void);
void cg_PutDisp_DD_stripe(int y1, int y2);
void cg_AllClr_VRAM(void);
void cg_EnableStatusArea(int opt);
int cg_RTC_GetTicks(void);
int cg_Timer_Install(int id, void (*handler)(void), int ms);
int cg_Timer_Deinstall(int id);
int cg_Timer_Start(int id);
int cg_Timer_Stop(int id);
int cg_GetKey(int *key);
void cg_WaitMs(int ms);
int cg_Bfile_Open(const u16 *name, int mode, int zero);
int cg_Bfile_Close(int h);
int cg_Bfile_Size(int h);
int cg_Bfile_Seek(int h, int pos);
int cg_Bfile_Read(int h, void *buf, int size, int readpos);
int cg_Bfile_Create(const u16 *name, int mode, int *size);
int cg_Bfile_Write(int h, const void *buf, int size);
int cg_Bfile_Delete(const u16 *name);

/* The game: its entry point, as the fx-9860 OS would call it */
#define GAME_ENTRY 0x00300200

/* Screen: the 128x64 game screen scaled 3x is 384x192, the VRAM's full width */
#define VRAM_W 384
#define VRAM_H 216
#define SCALE 3
#define GAME_Y0 ((VRAM_H - 64 * SCALE) / 2)
#define COLOR_ON 0x0000
#define COLOR_OFF 0xFFFF
#define COLOR_BORDER 0x0000

/* Linker symbols (shim.ld) */
extern u32 sdata, edata, ldata, sbss, ebss; /* _sdata... in shim.ld */

u32 shim_saved_sp;
static u16 *vram;

/* A record of the syscalls the game made, for the emulator probe and for bring-up:
   shim_trace is a ring of (number, r4, r5, return address); shim_unknown counts the numbers
   fx_dispatch has no translation for. */
#define TRACE_N 128
struct trace { u32 n, a, b, pr; };
struct trace shim_trace[TRACE_N];
u32 shim_trace_pos;
#define UNKNOWN_N 32
struct unknown { u32 n, count; };
struct unknown shim_unknown[UNKNOWN_N];
u32 shim_frames;

static void note_unknown(u32 n)
{
	for (int i = 0; i < UNKNOWN_N; i++) {
		if (shim_unknown[i].n == n || shim_unknown[i].count == 0) {
			shim_unknown[i].n = n;
			shim_unknown[i].count++;
			return;
		}
	}
}

static void *mem_set(void *dst, int c, size_t n)
{
	uint8_t *d = dst;
	while (n--)
		*d++ = c;
	return dst;
}

/* ---- display ---- */

/* The game has no frame limiter: it runs as fast as the CPU allows, menus and play alike, and
   the CG50 is several times faster than the fx-9860 models it was made for. The shim slows it
   down uniformly: after each frame it waits (speed - 1) times as long as the game worked since
   the previous one, so speed 2 is half the CG50's pace (about an fx-9860GIII), speed 1 is full
   speed. The + and - keys (which the game does not use) change it while playing; the bottom
   border shows it as small blocks. Times are RTC ticks (1/128 s): their sum is exact even when
   a frame is shorter than a tick. */
#define SPEED_MIN 1
#define SPEED_MAX 8
static int pace_speed = 2;
static int pace_last;
static int pace_debt; /* ms x 128 */
static int pace_keys;

/* KEYSC: the key matrix row 2 is byte 0xA44B0003; '+' is bit 3, '-' bit 2 (the game's own
   key table has 137 '+' and 153 '-' there) */
#define KEYSC_ROW2 ((volatile uint8_t *)0xA44B0003)

static void speed_show(void)
{
	u16 *v = vram + (VRAM_H - 8) * VRAM_W;
	for (int y = 0; y < 4; y++)
		for (int b = 0; b < SPEED_MAX; b++)
			for (int x = 0; x < 4; x++)
				v[y * VRAM_W + VRAM_W - 6 - (SPEED_MAX - b) * 6 + x] =
					b < pace_speed ? 0xFFFF : COLOR_BORDER;
}

static void frame_pace(void)
{
	int keys = *KEYSC_ROW2 & 0x0C;
	int pressed = keys & ~pace_keys;
	pace_keys = keys;
	if ((pressed & 0x08) && pace_speed > SPEED_MIN)
		pace_speed--;
	if ((pressed & 0x04) && pace_speed < SPEED_MAX)
		pace_speed++;
	if (pressed)
		speed_show();

	int now = cg_RTC_GetTicks();
	int work = now - pace_last;
	if (work < 0 || work > 128)
		work = 0;
	pace_debt += work * 1000 * (pace_speed - 1);
	int ms = pace_debt / 128;
	if (ms > 500)
		ms = 500, pace_debt = 0;
	if (ms > 0) {
		cg_WaitMs(ms);
		pace_debt -= ms * 128;
	}
	pace_last = cg_RTC_GetTicks();
}

/* the buffers the game has pushed (bring-up) */
u32 shim_push_src[8];

void shim_lcd_push(const uint8_t *src)
{
	for (int i = 0; i < 8; i++) {
		if (shim_push_src[i] == (u32)src)
			break;
		if (!shim_push_src[i]) {
			shim_push_src[i] = (u32)src;
			break;
		}
	}
	u16 *row = vram + GAME_Y0 * VRAM_W;
	for (int y = 0; y < 64; y++) {
		u16 *p = row;
		for (int x = 0; x < 16; x++) {
			unsigned b = *src++;
			for (int k = 7; k >= 0; k--) {
				u16 c = (b >> k) & 1 ? COLOR_ON : COLOR_OFF;
				p[0] = c;
				p[1] = c;
				p[2] = c;
				p += 3;
			}
		}
		u32 *s = (u32 *)row;
		u32 *d1 = (u32 *)(row + VRAM_W);
		u32 *d2 = (u32 *)(row + 2 * VRAM_W);
		for (int i = 0; i < VRAM_W / 2; i++)
			d1[i] = d2[i] = s[i];
		row += SCALE * VRAM_W;
	}
	cg_PutDisp_DD();
	shim_frames++;
	frame_pace();
}

void shim_lcd_contrast(int level)
{
	(void)level;
}

static void screen_init(void)
{
	vram = cg_GetVRAMAddress();
	cg_EnableStatusArea(3);
	u32 *v = (u32 *)vram;
	u32 border = COLOR_BORDER << 16 | COLOR_BORDER;
	for (int i = 0; i < VRAM_W * VRAM_H / 2; i++)
		v[i] = border;
	speed_show();
	cg_PutDisp_DD();
	pace_last = cg_RTC_GetTicks();
}

/* ---- hardware routines of the game ---- */

/* The game reads the seconds of the SH3 RTC (0xFFFFFEC0), probably as a random seed */
int shim_rtc_seconds(void)
{
	return (cg_RTC_GetTicks() / 128) % 60;
}

void shim_exit(void)
{
}

/* ---- fx-9860 syscalls ---- */

/* fx-9860 user timers are 6-10; CG50 timers are allocated by the OS */
static int timer_ids[16];

/* fx Bfile open modes (1 read, 2 write, 3 read/write, 0x80 share) to the CG50's (0 read,
   1 read-share, 2 write, 3 read/write, 4 read/write-share) */
static int bfile_mode(int fx)
{
	switch (fx) {
	case 0x01: return 0;
	case 0x80: return 1;
	case 0x81: return 1;
	case 0x02: return 2;
	case 0x03: return 3;
	case 0x83: return 4;
	default: return fx;
	}
}

int fx_dispatch(int a, int b, int c, int d, u32 n, int *stack, u32 pr)
{
	(void)stack;
	struct trace *t = &shim_trace[shim_trace_pos++ % TRACE_N];
	t->n = n;
	t->a = a;
	t->b = b;
	t->pr = pr;

	switch (n) {
	/* startup and quit (SDK crt0 and its quit handler) */
	case 0x0013: /* GlibGetAddinLibInf: the result is not used */
	case 0x0494: /* SetQuitHandler: the OS's quit request is not forwarded */
	case 0x01E7: /* quit handler cleanup */
	case 0x0218:
	case 0x0244:
		return 0;
	case 0x03FA: /* crt0 copies 108 bytes of fx-9860 OS data (0x8801E000) to 0x08102000 */
		mem_set((void *)a, 0, c);
		return 0;

	/* OS version (the key table lookup): zeros select the game's built-in key table */
	case 0x0014:
		*(int *)a = 0;
		*(int *)b = 0;
		*(int *)c = 0;
		return 0;
	case 0x0015:
		*(int *)a = 0;
		*(int *)b = 0;
		*(int *)c = 0;
		*(int *)d = 0;
		return 0;

	/* timers */
	case 0x0118: { /* Timer_Install(id, handler, ms) */
		int id = cg_Timer_Install(0, (void (*)(void))b, c);
		if (id < 0 || a < 0 || a >= 16)
			return -1;
		timer_ids[a] = id + 1;
		return a;
	}
	case 0x0119: /* Timer_Deinstall(id) */
		if (a >= 0 && a < 16 && timer_ids[a]) {
			cg_Timer_Deinstall(timer_ids[a] - 1);
			timer_ids[a] = 0;
		}
		return 0;
	case 0x011A: /* Timer_Start(id) */
		return a >= 0 && a < 16 && timer_ids[a] ? cg_Timer_Start(timer_ids[a] - 1) : -1;
	case 0x011B: /* Timer_Stop(id) */
		return a >= 0 && a < 16 && timer_ids[a] ? cg_Timer_Stop(timer_ids[a] - 1) : -1;
	case 0x0420: /* Sleep(ms) */
		cg_WaitMs(a);
		return 0;

	/* keyboard: the key codes (KEY_CTRL_*, ASCII) are the same on both */
	case 0x090F: { /* GetKey(unsigned int *key) */
		int key = 0;
		int r = cg_GetKey(&key);
		*(u32 *)a = key;
		return r;
	}

	/* storage memory files: the same calls with other numbers */
	case 0x042C: return cg_Bfile_Open((const u16 *)a, bfile_mode(b), 0);
	case 0x042D: return cg_Bfile_Close(a);
	case 0x042E: /* Bfile_GetMediaFree(device, int *free) */
		*(int *)b = 1 << 20;
		return 0;
	case 0x042F: return cg_Bfile_Size(a);
	case 0x0431: return cg_Bfile_Seek(a, b);
	case 0x0432: return cg_Bfile_Read(a, (void *)b, c, d);
	case 0x0434: return cg_Bfile_Create((const u16 *)a, b, (int *)c);
	case 0x0435: return cg_Bfile_Write(a, (const void *)b, c);
	case 0x0439: return cg_Bfile_Delete((const u16 *)a);

	default:
		note_unknown(n);
		return 0;
	}
}

/* ---- start ---- */

void shim_init(void)
{
	u32 *s = &ldata, *dst = &sdata;
	while (dst < &edata)
		*dst++ = *s++;
	for (dst = &sbss; dst < &ebss; dst++)
		*dst = 0;

	screen_init();
}

int shim_main(int isAppli, int option)
{
	int (*game)(int, int) = (int (*)(int, int))GAME_ENTRY;
	return game(isAppli, option);
}
