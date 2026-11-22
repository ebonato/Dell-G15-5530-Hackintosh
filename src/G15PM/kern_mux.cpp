//  G15PM: mux de tela PS8461E para o driver da NVIDIA (17/11/2026, docs/GMUX-G15.md, fase 3).
//
//  O RM da NVIDIA chama nv_acpi_mux_method(nv, &inout, muxAcpiId, "MXDS"|"MXDM"|"MXID"|"LRST") para o mux dinamico
//  (DDS / Advanced Optimus). No NullMoth essa funcao e um stub (nv_xnu_stubs.cpp: NV_ERR_NOT_SUPPORTED). Aqui ela faz o
//  que o driver Linux da NVIDIA faz (nv-acpi.c): acha, sob a RTX (\_SB.PC00.PEG1.PEGP), o dispositivo ACPI cujo _ADR e
//  muxAcpiId (no G15, LCD0 = 0x8000a450) e chama nele o metodo com o argumento inout, devolvendo o resultado em inout.
//
//  So com o boot-arg -g15muxrm. Leituras (MXID; MXDS com arg&0xF == 0; MXDM/LRST com arg&7 == 0) passam; pedidos de
//  troca do proprio driver sao RECUSADOS e so registrados, a menos que -g15muxrm-sw (o boot nao troca a tela sozinho).
//  Registro G15MuxRM = {chamadas, leituras, trocas recusadas, trocas feitas, erros, gancho}; G15MuxRMLast = ultimas.
#include <Headers/kern_api.hpp>
#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include "g15_filelog.hpp"
#include <kern/thread_call.h>
#include <IOKit/IOLib.h>

typedef uint32_t NvU32;
typedef uint32_t NV_STATUS;
static constexpr NV_STATUS NV_OK = 0, NV_ERR_GENERIC = 0x0000FFFF, NV_ERR_INVALID_ARGUMENT = 0x0000001F,
	NV_ERR_NOT_SUPPORTED = 0x00000056, NV_ERR_OBJECT_NOT_FOUND = 0x00000057;

static mach_vm_address_t gMuxOrg = 0;
static uint32_t gMx[6] {};
static bool gMuxSwitchAllowed = false;
static char gMxLast[6][48] {};
static unsigned gMxLastN = 0;
static IOLock *gMxLock = nullptr;

static void mxPublish() {
	IORegistryEntry::getRegistryRoot()->setProperty("G15MuxRM", gMx, sizeof(gMx));
	char b[6 * 48 + 8]; int n = 0;
	for (unsigned i = 0; i < 6 && i < gMxLastN; i++) {
		const char *s = gMxLast[(gMxLastN - 1 - i) % 6];
		n += snprintf(b + n, sizeof(b) - n, "%s%s", i ? "; " : "", s);
	}
	b[n] = 0;
	IORegistryEntry::getRegistryRoot()->setProperty("G15MuxRMLast", gMxLastN ? b : "nenhuma");
}

static bool mxIsRead(const char *m, NvU32 arg) {
	if (!strcmp(m, "MXID")) return true;
	if (!strcmp(m, "MXDS")) return (arg & 0xF) == 0;
	if (!strcmp(m, "MXDM") || !strcmp(m, "LRST")) return (arg & 0x7) == 0;
	return false;
}

// o dispositivo ACPI filho da RTX com _ADR = id (a localizacao no IOACPIPlane e o _ADR em hexa)
static IOACPIPlatformDevice *mxFind(NvU32 id) {
	auto pegp = IORegistryEntry::fromPath("IOACPIPlane:/_SB/PC00/PEG1@10000/PEGP@0");
	if (!pegp) return nullptr;
	char want[16]; snprintf(want, sizeof(want), "%x", id);
	IOACPIPlatformDevice *found = nullptr;
	auto it = pegp->getChildIterator(IORegistryEntry::getPlane("IOACPIPlane"));
	while (it && !found) {
		auto e = OSDynamicCast(IOACPIPlatformDevice, it->getNextObject());
		if (!e) break;
		const char *l = e->getLocation();
		if (l && !strcasecmp(l, want)) { found = e; found->retain(); }
	}
	OSSafeReleaseNULL(it);
	pegp->release();
	return found;
}


// (17/11) -g15muxrm-cache: as leituras sao respondidas da MEMORIA. MXDS(0)/MXDM(0)/MXID(0)/MXID(1)/LRST(0) do LCD0 lidos
// uma vez no carregamento do NVRM (antes do RM iniciar); dentro da chamada do RM nada de ACPI, registro ou arquivo
// (suspeitos da trava com -g15muxrtx + nvrmacpibl=1 + -g15muxrm). So anota, IOLog (aparece no -v) e agenda a publicacao.
static bool gMxCache = false;
// -g15muxrm-okw (com o -cache): pedidos de ESCRITA do RM (MXDS set, MXDM set, LRST set) respondem 1 (o "ok" do ACPI)
// SEM executar nada: o mux ja esta na RTX pelo -g15muxrtx e o MXDM chamaria o SMI do BIOS. Testa se a recusa prendia o RM.
static bool gMxOkWrites = false;
// -g15muxrm-linux (17/11, implica -okw): como o nv_acpi_mux_method do driver Linux da NVIDIA (kernel-open/nvidia/nv-acpi.c,
// tag 620.70.06), que so aceita MXDS e MXDM: MXID e LRST respondem NOT_SUPPORTED. O nosso gancho respondia os quatro; o RM
// guarda MXDM/MXID/MXDS do _DOD no boot (acpi_common.c) e manda ao GSP. Hipotese: o MXID ok (0x10003) leva o GSP a outro caminho.
static bool gMxLinux = false;
static NvU32 gMxCId = 0x8000A450, gMxC[5] {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};   // MXDS MXDM MXID0 MXID1 LRST
static thread_call_t gMxPub = nullptr;
static void mxPubFn(thread_call_param_t, thread_call_param_t) { mxPublish(); }
static void mxCacheFill() {
	auto dev = mxFind(gMxCId);
	if (!dev) return;
	const char *ms[5] = {"MXDS", "MXDM", "MXID", "MXID", "LRST"}; const NvU32 as[5] = {0, 0, 0, 1, 0};
	for (int i = 0; i < 5; i++) {
		OSObject *res = nullptr; auto a = OSNumber::withNumber(as[i], 32); OSObject *params[1] = {a};
		if (a && dev->evaluateObject(ms[i], &res, params, 1) == kIOReturnSuccess)
			if (auto n = OSDynamicCast(OSNumber, res)) gMxC[i] = n->unsigned32BitValue();
		OSSafeReleaseNULL(res); OSSafeReleaseNULL(a);
	}
	dev->release();
	g15fl::logf("G15PM: mux cache MXDS %u MXDM %u MXID %u/%u LRST %u", gMxC[0], gMxC[1], gMxC[2], gMxC[3], gMxC[4]);
}
static NV_STATUS muxMethodCached(const char *m, NvU32 *inout, NvU32 id) {
	const NvU32 arg = *inout;
	NV_STATUS st = NV_ERR_NOT_SUPPORTED; NvU32 out = 0xFFFFFFFF;
	if (gMxLinux && (!strcmp(m, "MXID") || !strcmp(m, "LRST"))) { /* NOT_SUPPORTED, como no Linux */ }
	else if (id == gMxCId && !mxIsRead(m, arg) && gMxOkWrites) { out = 1; *inout = out; st = NV_OK; }
	else if (id == gMxCId && mxIsRead(m, arg)) {
		int i = !strcmp(m, "MXDS") ? 0 : !strcmp(m, "MXDM") ? 1 : !strcmp(m, "MXID") ? (arg ? 3 : 2) : !strcmp(m, "LRST") ? 4 : -1;
		if (i >= 0 && gMxC[i] != 0xFFFFFFFF) { out = gMxC[i]; *inout = out; st = NV_OK; }
	}
	OSIncrementAtomic(reinterpret_cast<volatile SInt32 *>(&gMx[0]));
	OSIncrementAtomic(reinterpret_cast<volatile SInt32 *>(&gMx[st == NV_OK ? 1 : 2]));
	const unsigned k = (unsigned)OSIncrementAtomic(reinterpret_cast<volatile SInt32 *>(&gMxLastN)) % 6;
	snprintf(gMxLast[k], sizeof(gMxLast[0]), "%s(0x%x)@%x=%s0x%x", m, arg, id, st != NV_OK ? "RECUSADO/" : (mxIsRead(m, arg) ? "" : "OKFALSO/"), st == NV_OK ? out : st);
	IOLog("G15PM: mux RM %s(0x%x) id 0x%x -> st 0x%x out 0x%x (cache)\n", m, arg, id, st, out);
	if (gMxPub) thread_call_enter(gMxPub);
	return st;
}

static NV_STATUS muxMethod(void *nv, NvU32 *inout, NvU32 id, const char *name) {
	if (!inout || !name) return NV_ERR_INVALID_ARGUMENT;
	char m[5] {}; strlcpy(m, name, sizeof(m));
	if (strcmp(m, "MXDS") && strcmp(m, "MXDM") && strcmp(m, "MXID") && strcmp(m, "LRST")) return NV_ERR_INVALID_ARGUMENT;
	if (gMxCache) return muxMethodCached(m, inout, id);
	const NvU32 arg = *inout;
	const bool rd = mxIsRead(m, arg);
	NV_STATUS st = NV_OK; NvU32 out = 0xFFFFFFFF;
	if (gMxLock) IOLockLock(gMxLock);
	gMx[0]++;
	if (!rd && !gMuxSwitchAllowed) { gMx[2]++; st = NV_ERR_NOT_SUPPORTED; }
	else {
		auto dev = mxFind(id);
		if (!dev) { gMx[4]++; st = NV_ERR_OBJECT_NOT_FOUND; }
		else {
			OSObject *res = nullptr; auto a = OSNumber::withNumber(arg, 32); OSObject *params[1] = {a};
			IOReturn r = a ? dev->evaluateObject(m, &res, params, 1) : kIOReturnNoMemory;
			auto num = OSDynamicCast(OSNumber, res);
			if (r != kIOReturnSuccess || !num) { gMx[4]++; st = NV_ERR_GENERIC; }
			else { out = num->unsigned32BitValue(); *inout = out; if (rd) gMx[1]++; else gMx[3]++; }
			OSSafeReleaseNULL(res); OSSafeReleaseNULL(a);
			dev->release();
		}
	}
	snprintf(gMxLast[gMxLastN % 6], sizeof(gMxLast[0]), "%s(0x%x)@%x=%s0x%x", m, arg, id,
	         st == NV_OK ? "" : (st == NV_ERR_NOT_SUPPORTED ? "RECUSADO/" : "ERRO/"), st == NV_OK ? out : st);
	gMxLastN++;
	mxPublish();
	if (gMxLock) IOLockUnlock(gMxLock);
	g15fl::logf("G15PM: mux RM %s(0x%x) id 0x%x -> st 0x%x out 0x%x", m, arg, id, st, out);
	return st;
}

// (17/11) -g15muxdod: nv_acpi_dod_method so com o _DOD da RTX (\_SB.PC00.PEG1.PEGP._DOD), sem o _DSM NBCI que o
// nvrmacpibl=1 do NullMoth libera junto (o nvrmacpibl=1 trava o boot no Hybrid). Sem os ids do _DOD o RM nao monta a
// lista de saidas com mux e nunca chama o MXDS. Registro G15MuxDOD = {chamadas, ids devolvidos, erros, gancho} e
// G15MuxDODIds = os ids.
// -g15muxdod-sempainel (17/11): o mesmo, mas tira o painel (0x8000A450, LCD0) da lista. O _DOD com o painel trava o
// boot no Hybrid; sem ele, se subir, o gatilho e o painel na lista (o RM tentando o eDP com o mux do lado da UHD).
static mach_vm_address_t gDodOrg = 0;
static bool gDodNoPanel = false;
static uint32_t gDd[4] {};
static void ddPublish(const char *ids) {
	IORegistryEntry::getRegistryRoot()->setProperty("G15MuxDOD", gDd, sizeof(gDd));
	if (ids) IORegistryEntry::getRegistryRoot()->setProperty("G15MuxDODIds", ids);
}
static NV_STATUS dodMethod(void *nv, NvU32 *out, NvU32 *size) {
	gDd[0]++;
	if (!out || !size) { gDd[2]++; ddPublish(nullptr); return NV_ERR_INVALID_ARGUMENT; }
	auto pegp = OSDynamicCast(IOACPIPlatformDevice, IORegistryEntry::fromPath("IOACPIPlane:/_SB/PC00/PEG1@10000/PEGP@0"));
	if (!pegp) { gDd[2]++; ddPublish(nullptr); return NV_ERR_OBJECT_NOT_FOUND; }
	const NvU32 cap = *size / sizeof(NvU32);
	OSObject *o = nullptr;
	NV_STATUS st = NV_ERR_GENERIC;
	char ids[160] = ""; int n = 0;
	if (pegp->evaluateObject("_DOD", &o) == kIOReturnSuccess) {
		auto a = OSDynamicCast(OSArray, o);
		if (a && a->getCount() <= cap) {
			st = NV_OK; *size = 0;
			unsigned k = 0;
			for (unsigned i = 0; i < a->getCount(); i++) {
				auto num = OSDynamicCast(OSNumber, a->getObject(i));
				if (!num) { st = NV_ERR_GENERIC; break; }
				const NvU32 id = num->unsigned32BitValue();
				if (gDodNoPanel && id == 0x8000A450) { n += snprintf(ids + n, sizeof(ids) - n, "%s(-0x%x)", k ? " " : "", id); continue; }
				out[k] = id; *size += sizeof(NvU32);
				n += snprintf(ids + n, sizeof(ids) - n, "%s0x%x", k ? " " : "", id);
				k++;
			}
		}
	}
	OSSafeReleaseNULL(o);
	pegp->release();
	if (st == NV_OK) gDd[1] = *size / sizeof(NvU32); else gDd[2]++;
	ddPublish(st == NV_OK ? ids : "erro");
	g15fl::logf("G15PM: _DOD da RTX para o RM: st 0x%x ids %s (cap %u)", st, ids, cap);
	return st;
}

// (17/11) -g15kapilog: SO LEITURA. Gancho no GetDynamicDisplayInfo do NVKMS (tabela KAPI, o que o NVRMFB chama na busca
// de telas). Chama o original e anota por saida: perguntas e a ultima resposta "connected" (byte logo depois do handle
// em NvKmsKapiDynamicDisplayParams). Responde se, com o painel no lado da RTX (mux), o NVKMS ve o 0x200 conectado.
// Registro G15KapiDpy = "0xHANDLE:cN/perguntas ...".
static mach_vm_address_t gKdOrg = 0;
static struct { uint32_t h, n; uint8_t c; } gKd[8];
static void kdPublish() {
	char b[200]; int n = 0;
	for (unsigned i = 0; i < 8 && gKd[i].h; i++) n += snprintf(b + n, sizeof(b) - n, "%s0x%x:c%u/%u", i ? " " : "", gKd[i].h, gKd[i].c, gKd[i].n);
	b[n] = 0;
	IORegistryEntry::getRegistryRoot()->setProperty("G15KapiDpy", n ? b : "nenhuma");
}
static uint8_t kdHook(void *dev, void *params) {
	uint8_t r = FunctionCast(kdHook, gKdOrg)(dev, params);
	if (params) {
		const uint32_t h = *static_cast<uint32_t *>(params);
		const uint8_t c = static_cast<uint8_t *>(params)[4];
		unsigned i = 0;
		while (i < 8 && gKd[i].h && gKd[i].h != h) i++;
		if (i < 8) {
			const bool novo = !gKd[i].h || gKd[i].c != c;
			gKd[i].h = h; gKd[i].n++; gKd[i].c = r ? c : 0xFF;
			if (novo) g15fl::logf("G15PM: KAPI dpy 0x%x connected %u (r %u)", h, c, r);
			kdPublish();
		}
	}
	return r;
}

static const char *gMxNvPath[] {"/Library/Extensions/NVRM.kext/Contents/MacOS/NVRM"};
static KernelPatcher::KextInfo gMxNv {"com.nullmoth.NVRM", gMxNvPath, arrsize(gMxNvPath), {true}, {}, KernelPatcher::KextInfo::Unloaded};

void muxRmInstall() {
	gDodNoPanel = checkKernelArgument("-g15muxdod-sempainel");
	const bool dod = checkKernelArgument("-g15muxdod") || gDodNoPanel;
	const bool kapi = checkKernelArgument("-g15kapilog");
	if (dod) { gDd[3] = 9; ddPublish(nullptr); }
	gMxCache = checkKernelArgument("-g15muxrm-cache");
	gMxOkWrites = checkKernelArgument("-g15muxrm-okw");
	gMxLinux = checkKernelArgument("-g15muxrm-linux");
	if (gMxLinux) gMxOkWrites = true;
	if (gMxOkWrites) gMxCache = true;
	if (!checkKernelArgument("-g15muxrm") && !gMxCache && !dod && !kapi) return;
	if (gMxCache) gMxPub = thread_call_allocate(mxPubFn, nullptr);
	gMuxSwitchAllowed = checkKernelArgument("-g15muxrm-sw");
	gMxLock = IOLockAlloc();
	gMx[5] = 9; mxPublish();   // 9 = esperando o NVRM
	lilu.onKextLoadForce(&gMxNv, 1, [](void *, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		if (!gMxNv.loadIndex || index != gMxNv.loadIndex) return;
		if (checkKernelArgument("-g15kapilog")) {
			mach_vm_address_t k = patcher.solveSymbol(index, "_GetDynamicDisplayInfo", address, size);
			if (k) gKdOrg = patcher.routeFunction(k, reinterpret_cast<mach_vm_address_t>(kdHook), true);
			if (!k || patcher.getError() != KernelPatcher::Error::NoError) { gKdOrg = 0; patcher.clearError(); }
			IORegistryEntry::getRegistryRoot()->setProperty("G15KapiDpy", gKdOrg ? "gancho instalado" : "gancho FALHOU");
			g15fl::logf("G15PM: gancho do GetDynamicDisplayInfo %s", gKdOrg ? "instalado" : "FALHOU");
		}
		if (gDd[3] == 9) {
			mach_vm_address_t d = patcher.solveSymbol(index, "_nv_acpi_dod_method", address, size);
			if (!d) { gDd[3] = 2; patcher.clearError(); }
			else {
				gDodOrg = patcher.routeFunction(d, reinterpret_cast<mach_vm_address_t>(dodMethod), true);
				if (patcher.getError() != KernelPatcher::Error::NoError) { gDodOrg = 0; gDd[3] = 3; patcher.clearError(); }
				else gDd[3] = 1;
			}
			ddPublish(nullptr);
			g15fl::logf("G15PM: gancho do nv_acpi_dod_method %s", gDd[3] == 1 ? "instalado" : "FALHOU");
		}
		if (!checkKernelArgument("-g15muxrm") && !gMxCache) return;
		if (gMxCache) mxCacheFill();
		mach_vm_address_t fn = patcher.solveSymbol(index, "_nv_acpi_mux_method", address, size);
		if (!fn) { gMx[5] = 2; patcher.clearError(); }
		else {
			gMuxOrg = patcher.routeFunction(fn, reinterpret_cast<mach_vm_address_t>(muxMethod), true);
			if (patcher.getError() != KernelPatcher::Error::NoError) { gMuxOrg = 0; gMx[5] = 3; patcher.clearError(); }
			else gMx[5] = 1;
		}
		mxPublish();
		g15fl::logf("G15PM: gancho do nv_acpi_mux_method %s (trocas %s)", gMx[5] == 1 ? "instalado" : "FALHOU",
		            gMuxSwitchAllowed ? "PERMITIDAS" : "recusadas");
	});
}
