//  G15PM: buffer grande do standby da RTX em write-combining (17/11/2026, docs/RTX-TRAVAS.md).
//
//  Ao parar a RTX (RtxPm, mesmo caminho do repouso), o RM do NullMoth salva a VRAM em uso na memoria do sistema
//  (FBSR_TYPE_DMA: memmgrSavePowerMgmtState -> _memmgrRunFbsrOperation -> memdescAlloc -> nv_alloc_pages). O RM pede esse
//  buffer SEM cache (NV_MEMORY_UNCACHED, "never touched by the CPU") e zerado; o nv_alloc_pages do NullMoth zera com
//  `rep stosb` pelo mapeamento sem cache: centenas de MB em escritas de 1 byte nao combinadas. Spindump de 17/11 17:11:
//  508 de 551 amostras da thread do standby nesse rep stosb (nv_alloc_pages+401), ~5 s, e a UHD inteira parada junto
//  (o controlador de memoria e dividido). Aqui o nv_alloc_pages e trocado (Lilu): pedidos SEM cache de >= 4 MB vao como
//  NV_MEMORY_WRITECOMBINED (so muda o mapeamento da CPU; a RTX le/escreve por DMA igual) e o zeramento vira rajadas.
//    -g15fbsrwc   troca e mede.   -g15fbsrlog   so mede (sem trocar), para comparar.
//  So na thread do NVRM::gpuSuspend (tambem trocado): no boot o RM grava o firmware da GSP em buffers sem cache e la o
//  write-combining exigiria esvaziar antes da placa ler. Registro G15Fbsr = {gancho (1 ok, 9 esperando, 2/3/4 falhou),
//  pedidos grandes sem cache no standby, trocados, ultimo MB, ultimo ms, pior ms, total MB}.
#include <Headers/kern_api.hpp>
#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <kern/thread_call.h>
#include <kern/clock.h>

typedef uint32_t NvU32; typedef uint64_t NvU64; typedef int32_t NvS32; typedef uint8_t NvBool; typedef uint32_t NV_STATUS;
using AllocFn = NV_STATUS (*)(void *, NvU32, NvU64, NvBool, NvU32, NvBool, NvBool, NvS32, NvU64 *, void **);
static constexpr NvU32 kUncached = 1, kWriteCombined = 2;
static constexpr uint64_t kMinBytes = 4ull << 20;

static mach_vm_address_t gAllocOrg = 0, gSuspOrg = 0;
static thread_t gSuspThread = nullptr;   // so os pedidos da thread do NVRM::gpuSuspend (standby) sao trocados
static bool gWc = false;
static uint32_t gFb[7] {};
static thread_call_t gFbPub = nullptr;
static uint32_t gL1Pub[3] {};
static void fbPublish(thread_call_param_t, thread_call_param_t);

static NV_STATUS allocHook(void *nv, NvU32 count, NvU64 psz, NvBool contig, NvU32 cache, NvBool zeroed, NvBool unenc,
                           NvS32 node, NvU64 *pte, void **priv) {
	const uint64_t bytes = (uint64_t)count * PAGE_SIZE;
	if (cache != kUncached || bytes < kMinBytes || current_thread() != gSuspThread)
		return FunctionCast(allocHook, gAllocOrg)(nv, count, psz, contig, cache, zeroed, unenc, node, pte, priv);
	const NvU32 use = gWc ? kWriteCombined : cache;
	uint64_t t0 = mach_absolute_time();
	NV_STATUS st = FunctionCast(allocHook, gAllocOrg)(nv, count, psz, contig, use, zeroed, unenc, node, pte, priv);
	uint64_t ns = 0; absolutetime_to_nanoseconds(mach_absolute_time() - t0, &ns);
	const uint32_t ms = (uint32_t)(ns / 1000000);
	gFb[1]++; if (use != cache) gFb[2]++;
	gFb[3] = (uint32_t)(bytes >> 20); gFb[4] = ms; if (ms > gFb[5]) gFb[5] = ms; gFb[6] += (uint32_t)(bytes >> 20);
	IOLog("G15PM: nv_alloc_pages %llu MB %s (contig %u, zeroed %u) -> 0x%x em %u ms\n", bytes >> 20,
	      use != cache ? "SEM cache -> WRITECOMBINED" : "sem cache", contig, zeroed, st, ms);
	if (gFbPub) thread_call_enter(gFbPub);
	return st;
}

static void suspHook(void *that) {   // void NVRM::gpuSuspend()
	gSuspThread = current_thread();
	FunctionCast(suspHook, gSuspOrg)(that);
	gSuspThread = nullptr;
}

// (17/11, docs/RTX-TRAVAS.md, ASPM) -g15nvl1: o RM pergunta a porta de cima se ela anuncia L1
// (clIsL1SupportedForUpstreamPort: LNKCAP da PEG1) e manda a resposta ao GSP (PDB_PROP_CL_ASPM_L1_UPSTREAM_PORT_SUPPORTED,
// CL_SYNC_PDB). O BIOS da Dell grava "nenhum ASPM" na PEG1 (o hardware tem L1 e L1SS completos; o driver do Windows liga o
// L1 por conta propria com Optimus + bateria/silencioso). Aqui a resposta vira "sim". Registro G15NvL1 = {gancho, chamadas,
// resposta original da ultima}.
static mach_vm_address_t gL1Org = 0;
static uint32_t gL1[3] {};
static uint8_t l1Hook(void *gpu, void *cl) {
	uint8_t r = FunctionCast(l1Hook, gL1Org)(gpu, cl);
	gL1[1]++; gL1[2] = r;
	if (gFbPub) thread_call_enter(gFbPub);
	return 1;
}

static const char *gFbNvPath[] {"/Library/Extensions/NVRM.kext/Contents/MacOS/NVRM"};
static KernelPatcher::KextInfo gFbNv {"com.nullmoth.NVRM", gFbNvPath, arrsize(gFbNvPath), {true}, {}, KernelPatcher::KextInfo::Unloaded};

static void fbPublish(thread_call_param_t, thread_call_param_t) {
	IORegistryEntry::getRegistryRoot()->setProperty("G15Fbsr", gFb, sizeof(gFb));
	if (gL1[0]) { memcpy(gL1Pub, gL1, sizeof(gL1)); IORegistryEntry::getRegistryRoot()->setProperty("G15NvL1", gL1Pub, sizeof(gL1Pub)); }
}

void fbsrInstall() {
	gWc = checkKernelArgument("-g15fbsrwc");
	const bool l1 = checkKernelArgument("-g15nvl1");
	if (l1) gL1[0] = 9;
	if (!gWc && !l1 && !checkKernelArgument("-g15fbsrlog")) return;
	gFbPub = thread_call_allocate(fbPublish, nullptr);
	gFb[0] = 9; fbPublish(nullptr, nullptr);   // 9 = esperando o NVRM
	lilu.onKextLoadForce(&gFbNv, 1, [](void *, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		if (!gFbNv.loadIndex || index != gFbNv.loadIndex) return;
		if (gL1[0] == 9) {
			mach_vm_address_t f = patcher.solveSymbol(index, "_clIsL1SupportedForUpstreamPort_IMPL", address, size);
			if (f) gL1Org = patcher.routeFunction(f, reinterpret_cast<mach_vm_address_t>(l1Hook), true);
			gL1[0] = (f && patcher.getError() == KernelPatcher::Error::NoError) ? 1 : 2;
			if (gL1[0] != 1) { gL1Org = 0; patcher.clearError(); }
			IOLog("G15PM: gancho do clIsL1SupportedForUpstreamPort %s\n", gL1[0] == 1 ? "instalado" : "FALHOU");
			fbPublish(nullptr, nullptr);
		}
		mach_vm_address_t sp = patcher.solveSymbol(index, "__ZN4NVRM10gpuSuspendEv", address, size);
		if (sp) gSuspOrg = patcher.routeFunction(sp, reinterpret_cast<mach_vm_address_t>(suspHook), true);
		if (!sp || patcher.getError() != KernelPatcher::Error::NoError) { gSuspOrg = 0; gFb[0] = 4; patcher.clearError(); fbPublish(nullptr, nullptr); return; }
		mach_vm_address_t fn = patcher.solveSymbol(index, "_nv_alloc_pages", address, size);
		if (!fn) { gFb[0] = 2; patcher.clearError(); }
		else {
			gAllocOrg = patcher.routeFunction(fn, reinterpret_cast<mach_vm_address_t>(allocHook), true);
			if (patcher.getError() != KernelPatcher::Error::NoError) { gAllocOrg = 0; gFb[0] = 3; patcher.clearError(); }
			else gFb[0] = 1;
		}
		fbPublish(nullptr, nullptr);
		IOLog("G15PM: gancho do nv_alloc_pages %s (%s)\n", gFb[0] == 1 ? "instalado" : "FALHOU", gWc ? "write-combining" : "so mede");
	});
}
