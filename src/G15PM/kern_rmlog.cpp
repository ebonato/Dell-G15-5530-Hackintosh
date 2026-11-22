//  G15PM: log do RM do NullMoth em arquivo, sincrono (projeto GC6, docs/RTX-GC6.md, etapa 2; 17/11/2026).
//
//  O log unificado do macOS chega em lotes pelo logd: num congelamento as ultimas linhas antes da trava se perdem
//  (gc6-20261117-2114: 110 linhas, todas no mesmo ms). Aqui as funcoes de log do NVRM (out_string = NV_PRINTF do RM,
//  nv_printf, nv_dev_vprintf) sao trocadas (Lilu): cada linha vai, sem trava, para um buffer de 1 MB; um thread_call grava
//  SO O NOVO a cada 20 ms (a 1a vez so apos 5 s: no comeco do boot o VFS nao existe e o vnode_open travou o boot,
//  17/11 21:30) no fim de /private/var/log/g15-gc6-rm.txt com IO_SYNC. So com -g15rmlog. Tambem recebe as
//  linhas do G15PM via g15RmLog() (ganchos de _DSM).
#include <Headers/kern_api.hpp>
#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <kern/thread_call.h>
#include <kern/clock.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <stdarg.h>

typedef uint32_t NvU32;
static constexpr uint32_t kCap = 1u << 20;
static char *gBuf = nullptr;
static volatile uint32_t gLen = 0, gDone = 0, gDropped = 0, gPending = 0;
static thread_call_t gFlush = nullptr;
static bool gOn = false, gFirst = true;   // 1a gravacao do boot zera o arquivo
static volatile bool gRootOk = false;     // so depois de uma gravacao ok o ritmo vira 20 ms (antes: 5 s, como o g15fl)
static const char *kPath = "/private/var/log/g15-gc6-rm.txt";

static void put(const char *s, uint32_t n) {
	if (!gBuf || !n) return;
	const uint32_t off = __sync_fetch_and_add(&gLen, n);
	if (off >= kCap || kCap - off < n) { __sync_fetch_and_sub(&gLen, n); __sync_fetch_and_add(&gDropped, 1); return; }
	memcpy(gBuf + off, s, n);
	if (gFlush && __sync_bool_compare_and_swap(&gPending, 0, 1)) {
		uint64_t dl = 0; clock_interval_to_deadline(gRootOk ? 20 : 5000, kMillisecondScale, &dl);
		thread_call_enter_delayed(gFlush, dl);
	}
}
static void line(const char *s) {
	if (!s) return;
	char b[600]; uint64_t ns = 0; absolutetime_to_nanoseconds(mach_absolute_time(), &ns);
	int h = snprintf(b, sizeof(b), "%llu.%03llu ", ns / 1000000000ULL, (ns / 1000000ULL) % 1000ULL);
	size_t l = strnlen(s, sizeof(b) - h - 2);
	memcpy(b + h, s, l); uint32_t t = (uint32_t)(h + l);
	if (!t || b[t - 1] != '\n') b[t++] = '\n';
	put(b, t);
}
void g15RmLog(const char *fmt, ...) {
	if (!gOn) return;
	char b[400]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof(b), fmt, ap); va_end(ap);
	line(b);
}

static void flushFn(thread_call_param_t, thread_call_param_t) {
	gPending = 0;
	uint32_t end = gLen; if (end > kCap) end = kCap;
	if (end <= gDone) return;
	vfs_context_t ctx = vfs_context_create(nullptr); if (!ctx) return;
	vnode_t vp = NULLVP;
	int e = vnode_open(kPath, O_CREAT | FWRITE | O_NOFOLLOW | (gFirst ? O_TRUNC : 0), 0644, 0, &vp, ctx);
	if (!e) gFirst = false;
	if (!e) {
		int resid = 0;
		e = vn_rdwr(UIO_WRITE, vp, gBuf + gDone, (int)(end - gDone), (off_t)gDone, UIO_SYSSPACE, IO_SYNC | IO_NOCACHE | IO_NODELOCKED | IO_UNIT,
		            vfs_context_ucred(ctx), &resid, vfs_context_proc(ctx));
		if (!e && !resid) { gDone = end; gRootOk = true; }
		vnode_close(vp, FWRITE, ctx);
	}
	vfs_context_rele(ctx);
	if (gLen > gDone && __sync_bool_compare_and_swap(&gPending, 0, 1)) {   // chegou linha (ou a raiz nao montou ainda)
		uint64_t dl = 0; clock_interval_to_deadline(e ? 5000 : 20, kMillisecondScale, &dl);
		thread_call_enter_delayed(gFlush, dl);
	}
}

static mach_vm_address_t gOutOrg = 0, gPrOrg = 0, gDevOrg = 0;
static void outHook(const char *s) { line(s); FunctionCast(outHook, gOutOrg)(s); }
static int prHook(NvU32 lvl, const char *fmt, ...) {
	char b[512]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof(b), fmt, ap); va_end(ap);
	line(b);
	return reinterpret_cast<int (*)(NvU32, const char *, ...)>(gPrOrg)(lvl, "%s", b);
}
static int devHook(void *nv, int lvl, const char *fmt, va_list ap) {
	char b[512]; va_list c; va_copy(c, ap); vsnprintf(b, sizeof(b), fmt, c); va_end(c);
	line(b);
	return FunctionCast(devHook, gDevOrg)(nv, lvl, fmt, ap);
}

static const char *gRlPath[] {"/Library/Extensions/NVRM.kext/Contents/MacOS/NVRM"};
static KernelPatcher::KextInfo gRlNv {"com.nullmoth.NVRM", gRlPath, arrsize(gRlPath), {true}, {}, KernelPatcher::KextInfo::Unloaded};

void rmLogInstall() {
	if (!checkKernelArgument("-g15rmlog")) return;
	gBuf = static_cast<char *>(IOMalloc(kCap));
	if (!gBuf) return;
	gFlush = thread_call_allocate(flushFn, nullptr);
	gOn = true;
	clock_sec_t s = 0; clock_usec_t u = 0; clock_get_calendar_microtime(&s, &u);
	g15RmLog("==== G15PM -g15rmlog: boot (calendario %lu) ====", (unsigned long)s);
	lilu.onKextLoadForce(&gRlNv, 1, [](void *, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		if (!gRlNv.loadIndex || index != gRlNv.loadIndex) return;
		struct { const char *n; mach_vm_address_t h; mach_vm_address_t *o; } t[] = {
			{"_out_string", reinterpret_cast<mach_vm_address_t>(outHook), &gOutOrg},
			{"_nv_printf", reinterpret_cast<mach_vm_address_t>(prHook), &gPrOrg},
			{"_nv_dev_vprintf", reinterpret_cast<mach_vm_address_t>(devHook), &gDevOrg}};
		for (auto &x : t) {
			mach_vm_address_t f = patcher.solveSymbol(index, x.n, address, size);
			if (f) *x.o = patcher.routeFunction(f, x.h, true);
			if (!f || patcher.getError() != KernelPatcher::Error::NoError) { *x.o = 0; patcher.clearError(); }
			g15RmLog("G15PM: gancho de log %s %s", x.n, *x.o ? "instalado" : "FALHOU");
		}
	});
}
