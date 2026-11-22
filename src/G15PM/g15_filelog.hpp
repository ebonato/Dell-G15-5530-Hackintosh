// G15: log em arquivo para kexts injetados pelo OpenCore (o IOLog deles nao chega ao log persistente; roteiro 11/11 23:50).
// Cada linha vai ao IOLog e a um buffer de texto; um thread_call grava o buffer inteiro em
// /private/var/log/g15-<tag>-<epoch>.txt alguns segundos depois (nunca no caminho de quem chamou: pode ser falha de
// pagina, aviso de energia ou interrupcao). Antes da raiz montar o vnode_open falha e a gravacao e retentada.
// Uso: '#define G15FL_IMPL' num unico .cpp; g15fl::init("g15pm") uma vez no start; g15fl::logf("G15PM: ...") sem '\n' no fim.
#pragma once
#include <IOKit/IOLib.h>
#include <kern/clock.h>
#include <kern/thread_call.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/kauth.h>
#include <sys/errno.h>
#include <stdarg.h>

namespace g15fl {
// Variaveis definidas UMA vez (o linker de kext nao une variaveis 'inline': cada .cpp ficava com a sua copia, 12/11).
// Exatamente um .cpp do kext faz '#define G15FL_IMPL' antes do include.
#ifdef G15FL_IMPL
#define G15FL_VAR(decl, init) decl init
#else
#define G15FL_VAR(decl, init) extern decl
#endif
G15FL_VAR(char buf[48 * 1024], );
G15FL_VAR(volatile uint32_t len, = 0);
G15FL_VAR(volatile uint32_t dropped, = 0);
G15FL_VAR(volatile uint32_t pending, = 0);
G15FL_VAR(volatile uint32_t retries, = 0);
G15FL_VAR(volatile uint32_t writes, = 0);
G15FL_VAR(volatile int lastRc, = -999);
G15FL_VAR(char path[96], );
G15FL_VAR(const char *tag, = "g15");
G15FL_VAR(thread_call_t call, = nullptr);
#undef G15FL_VAR

inline int flushNow() {
	uint32_t n = len; if (n > sizeof(buf)) n = sizeof(buf); if (!n) return 0;
	if (!path[0]) {
		clock_sec_t s = 0; clock_usec_t u = 0; clock_get_calendar_microtime(&s, &u);
		snprintf(path, sizeof(path), "/private/var/log/g15-%s-%lu.txt", tag, static_cast<unsigned long>(s));
	}
	vfs_context_t ctx = vfs_context_create(nullptr); if (!ctx) return ENOMEM;
	vnode_t vp = NULLVP;
	int e = vnode_open(path, O_CREAT | O_TRUNC | FWRITE | O_NOFOLLOW, 0644, 0, &vp, ctx);
	if (!e) {
		int resid = 0;
		e = vn_rdwr(UIO_WRITE, vp, buf, static_cast<int>(n), 0, UIO_SYSSPACE, IO_NOCACHE | IO_NODELOCKED | IO_UNIT,
		            vfs_context_ucred(ctx), &resid, vfs_context_proc(ctx));
		if (!e && resid) e = EIO;
		vnode_close(vp, FWRITE, ctx);
	}
	vfs_context_rele(ctx);
	return e;
}

inline void schedule(uint32_t ms) {
	if (!call || !__sync_bool_compare_and_swap(&pending, 0, 1)) return;
	uint64_t dl = 0; clock_interval_to_deadline(ms, kMillisecondScale, &dl);
	thread_call_enter_delayed(call, dl);
}

inline void fire(thread_call_param_t, thread_call_param_t) {
	pending = 0;
	const uint32_t before = len;
	lastRc = flushNow();
	if (lastRc) { if (retries < 60) { retries++; schedule(20000); } }   // raiz ainda nao montada: tenta por ~20 min
	else { writes++; if (len != before) schedule(5000); }               // chegou linha durante a gravacao
}

inline void logf(const char *fmt, ...) __printflike(1, 2);
inline void logf(const char *fmt, ...) {
	char line[300];
	uint64_t ns = 0; absolutetime_to_nanoseconds(mach_absolute_time(), &ns);
	int h = snprintf(line, sizeof(line), "%llu.%03llu ", ns / 1000000000ULL, (ns / 1000000ULL) % 1000ULL);
	if (h < 0) h = 0;
	va_list ap; va_start(ap, fmt); int n = vsnprintf(line + h, sizeof(line) - h - 1, fmt, ap); va_end(ap);
	if (n < 0) return;
	uint32_t t = static_cast<uint32_t>(h + n); if (t > sizeof(line) - 2) t = sizeof(line) - 2;
	line[t++] = '\n'; line[t] = 0;
	IOLog("%s", line + h);
	const uint32_t off = __sync_fetch_and_add(&len, t);
	if (off < sizeof(buf) && sizeof(buf) - off >= t) memcpy(buf + off, line, t);
	else { __sync_fetch_and_sub(&len, t); __sync_fetch_and_add(&dropped, 1); return; }
	schedule(5000);
}

inline void init(const char *t) {
	tag = t;
	if (!call) call = thread_call_allocate(fire, nullptr);
	clock_sec_t s = 0; clock_usec_t u = 0; clock_get_calendar_microtime(&s, &u);
	logf("%s: log em arquivo ligado (calendario %lu)", t, static_cast<unsigned long>(s));
}
}  // namespace g15fl
