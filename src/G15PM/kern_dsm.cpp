//  G15PM: responde ao RM os _DSM JT e NVOP da RTX (so leitura) (17/11/2026, docs/RTX-TRAVAS.md, ASPM/LTR).
//
//  No Linux (driver NVIDIA, Fedora deste notebook) a RTX anuncia LTR+ na DEVCAP2; no macOS anuncia LTR-. A diferenca de
//  plataforma que sobra: o nv_acpi_dsm_method do NullMoth so responde NBCI/GPS/NVPCF; NVOP (Optimus) e JT ficam "nao
//  suportado", entao o GSP nao sabe que esta num notebook Optimus/JT (no ACPI do G15: JTEN=1, MHYB=1, RPCL=1, GC6V=2,
//  JTRV=0x200). Aqui o nv_acpi_dsm_method e trocado (Lilu): JT funcoes 0 (SUPPORT) e 1 (CAPS) e NVOP 0 (SUPPORT) e 0x1A
//  (OPTIMUSCAPS, so com entrada zero = leitura) sao avaliados no _DSM real do \_SB.PC00.PEG1.PEGP; o resto do JT/NVOP
//  (JT 3 = POWERCONTROL etc.) e recusado, para nao brigar com o RtxPm. Outros GUIDs vao ao original. So com -g15nvjt.
//  Registro G15NvDsm = {gancho, chamadas JT/NVOP, respondidas, recusadas}; G15NvDsmLast = ultimas.
#include <Headers/kern_api.hpp>
#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <kern/thread_call.h>
#include <kern/clock.h>

typedef uint32_t NvU32; typedef uint16_t NvU16; typedef uint8_t NvU8; typedef uint8_t NvBool; typedef uint32_t NV_STATUS;
static constexpr NV_STATUS NV_OK = 0, NV_ERR_NOT_SUPPORTED = 0x56, NV_ERR_OPERATING_SYSTEM = 0x00000041;
static const NvU8 kJt[16]   {0x51, 0xA3, 0xEC, 0xCB, 0x7B, 0x06, 0x24, 0x49, 0x9C, 0xBD, 0xB4, 0x6B, 0x00, 0xB8, 0x6F, 0x34};
static const NvU8 kNvop[16] {0xF8, 0xD8, 0x86, 0xA4, 0xDA, 0x0B, 0x1B, 0x47, 0xA7, 0x2B, 0x60, 0x42, 0xA6, 0xB5, 0xBE, 0xE0};

void g15RmLog(const char *fmt, ...);
static mach_vm_address_t gDsmOrg = 0;
static uint32_t gDs[4] {};
static char gDsLast[6][64] {};
static unsigned gDsN = 0;
static thread_call_t gDsPub = nullptr;
static void dsPublish(thread_call_param_t, thread_call_param_t) {
	IORegistryEntry::getRegistryRoot()->setProperty("G15NvDsm", gDs, sizeof(gDs));
	char b[6 * 64 + 8]; int n = 0;
	for (unsigned i = 0; i < 6 && i < gDsN; i++) n += snprintf(b + n, sizeof(b) - n, "%s%s", i ? "; " : "", gDsLast[(gDsN - 1 - i) % 6]);
	IORegistryEntry::getRegistryRoot()->setProperty("G15NvDsmLast", b);
}

// (17/11 21:40) As respostas sao lidas UMA vez no carregamento do NVRM (contexto que pode dormir) e guardadas. Dentro do
// RM o _DSM chega com trava presa, onde o AML nao pode dormir: chamar o evaluateObject ali CONGELOU o sistema (2 vezes;
// o proprio NullMoth recusa _DSM fora do NBCI nesse caso, nv_xnu_acpi_may_sleep). Agora o gancho so copia da memoria.
struct DsmAns { bool jt; NvU32 fn; NvU32 in; NvU8 data[64]; unsigned len; bool ok; };
// (22:20) Respostas FIXAS, medidas pelo DsmProbe (G15Fan) neste G15 com a RTX inicializada: avaliar o _DSM antes do RM
// iniciar a placa trava (1a leitura do ID da RTX pelo CKNG/NDID; boot travou com a leitura no carregamento do NVRM).
static DsmAns gAns[4] {{true, 0, 0, {0x1B, 0x00, 0x00, 0x00}, 4, true}, {true, 1, 0, {0x03, 0x60, 0x01, 0x20}, 4, true},
                       {false, 0, 0, {0x01, 0x00, 0x00, 0x0C}, 4, true}, {false, 0x1A, 0, {0x59, 0x00, 0x00, 0x19}, 4, true}};
static volatile bool gAnsReady = true;   // respostas fixas
static void dsmCache(thread_call_param_t, thread_call_param_t) {
	auto acpi = OSDynamicCast(IOACPIPlatformDevice, IORegistryEntry::fromPath("IOACPIPlane:/_SB/PC00/PEG1@10000/PEGP@0"));
	if (!acpi) { g15RmLog("G15PM: _DSM cache: sem PEGP no IOACPIPlane"); return; }
	for (auto &a : gAns) {
		g15RmLog("G15PM: _DSM cache: avaliando %s 0x%x ...", a.jt ? "JT" : "NVOP", a.fn);
		const NvU8 *g = a.jt ? kJt : kNvop;
		const NvU32 rev = a.jt ? 0x200 : 0x100;
		OSObject *args[4] = {OSData::withBytes(g, 16), OSNumber::withNumber(rev, 32), OSNumber::withNumber(a.fn, 32), OSData::withBytes(&a.in, 4)};
		OSObject *o = nullptr;
		IOReturn r = (args[0] && args[1] && args[2] && args[3]) ? acpi->evaluateObject("_DSM", &o, args, 4) : kIOReturnNoMemory;
		for (OSObject *x : args) OSSafeReleaseNULL(x);
		if (r == kIOReturnSuccess) {
			if (auto d = OSDynamicCast(OSData, o)) { a.len = d->getLength(); if (a.len > sizeof(a.data)) a.len = sizeof(a.data); memcpy(a.data, d->getBytesNoCopy(), a.len); a.ok = true; }
			else if (auto n = OSDynamicCast(OSNumber, o)) { uint32_t v = n->unsigned32BitValue(); a.len = 4; memcpy(a.data, &v, 4); a.ok = true; }
		}
		OSSafeReleaseNULL(o);
		uint32_t w = 0; if (a.len >= 4) memcpy(&w, a.data, 4);
		g15RmLog("G15PM: _DSM cache %s 0x%x -> %s 0x%x (%u B)", a.jt ? "JT" : "NVOP", a.fn, a.ok ? "ok" : "FALHOU", w, a.len);
	}
	acpi->release();
	gAnsReady = true;
	g15RmLog("G15PM: _DSM cache pronto");
}

static NV_STATUS dsmHook(void *nv, NvU8 *guid, NvU32 rev, NvBool pcf, NvU32 fn, void *in, NvU16 inSize, NvU32 *outStatus,
                         void *out, NvU16 *pSize) {
	const bool jt = guid && !memcmp(guid, kJt, 16), nvop = guid && !memcmp(guid, kNvop, 16);
	if (!jt && !nvop) return FunctionCast(dsmHook, gDsmOrg)(nv, guid, rev, pcf, fn, in, inSize, outStatus, out, pSize);
	gDs[1]++;
	uint32_t in0 = 0; if (in && inSize >= 4) memcpy(&in0, in, 4);
	g15RmLog("G15PM: _DSM %s 0x%x rev 0x%x in 0x%x (pedido)", jt ? "JT" : "NVOP", fn, rev, in0);
	NV_STATUS st = NV_ERR_NOT_SUPPORTED; uint32_t w = 0; unsigned len = 0;
	for (auto &a : gAns)
		if (gAnsReady && a.ok && a.jt == jt && a.fn == fn && (fn != 0x1A || in0 == 0) && out && pSize) {
			len = a.len; if (len > *pSize) len = *pSize;
			memcpy(out, a.data, len); *pSize = (NvU16)len; st = NV_OK; if (len >= 4) memcpy(&w, a.data, 4);
			break;
		}
	gDs[st == NV_OK ? 2 : 3]++;
	snprintf(gDsLast[gDsN % 6], sizeof(gDsLast[0]), "%s 0x%x(in 0x%x)=%s0x%x", jt ? "JT" : "NVOP", fn, in0,
	         st == NV_OK ? "" : "RECUSADO/", st == NV_OK ? w : st);
	gDsN++;
	g15RmLog("G15PM: _DSM %s 0x%x -> st 0x%x out 0x%x (%u B)", jt ? "JT" : "NVOP", fn, st, w, len);
	if (gDsPub) thread_call_enter(gDsPub);
	return st;
}

static const char *gDsNvPath[] {"/Library/Extensions/NVRM.kext/Contents/MacOS/NVRM"};
static KernelPatcher::KextInfo gDsNv {"com.nullmoth.NVRM", gDsNvPath, arrsize(gDsNvPath), {true}, {}, KernelPatcher::KextInfo::Unloaded};

void nvDsmInstall() {
	if (!checkKernelArgument("-g15nvjt")) return;
	gDsPub = thread_call_allocate(dsPublish, nullptr);
	gDs[0] = 9; dsPublish(nullptr, nullptr);
	lilu.onKextLoadForce(&gDsNv, 1, [](void *, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		if (!gDsNv.loadIndex || index != gDsNv.loadIndex) return;
		// (22:10) Leitura numa thread LOGO no carregamento do NVRM (o callback do Lilu trava o boot se avaliar ACPI; a
		// thread 15 s depois pegou a RTX sem driver/cortada e travou na 1a leitura do ID dela pelo CKNG/NDID). Agora: a
		// RTX ainda esta ligada pelo BIOS e o auto-go do NullMoth so inicia o RM segundos depois; o CKNG guarda o ID (NGEN)
		// e as proximas chamadas nao tocam mais a placa. DsmProbe (G15Fan) provou as 4 funcoes com a RTX ligada.
		// sem leitura no boot: as respostas sao fixas (o dsmCache fica para conferencia manual)
		mach_vm_address_t f = patcher.solveSymbol(index, "_nv_acpi_dsm_method", address, size);
		if (f) gDsmOrg = patcher.routeFunction(f, reinterpret_cast<mach_vm_address_t>(dsmHook), true);
		gDs[0] = (f && patcher.getError() == KernelPatcher::Error::NoError) ? 1 : 2;
		if (gDs[0] != 1) { gDsmOrg = 0; patcher.clearError(); }
		dsPublish(nullptr, nullptr);
		IOLog("G15PM: gancho do nv_acpi_dsm_method %s (JT/NVOP so leitura)\n", gDs[0] == 1 ? "instalado" : "FALHOU");
	});
}
