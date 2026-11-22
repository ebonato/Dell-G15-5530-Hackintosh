//  G15PM: religa a RTX quando um programa a pede (10/11/2026).
//
//  O G15.app (com o G15Fan, RtxPm) para a RTX ociosa: NVRM e audio HDMI no estado 0 e, com -g15fan-rtxpm-corte, energia
//  cortada (GOFF). O NVAccel continua registrado e o Metal ainda lista a placa para os programas da nvmtl-allow.txt; um
//  trabalho mandado com ela parada travaria. Aqui o NVAccel::newUserClient (todo programa que abre a RTX passa por ele) e
//  trocado (Lilu): antes do original chama G15Fan::callPlatformFunction("G15RtxWake"), que religa (RtxPm 1: GONN, IOPM
//  acorda o NVRM) e so volta com o NVRM no estado 1. Registro G15RtxWake = {aberturas, religadas, ultima espera ms,
//  falhas, gancho}. -g15rtxwake-off desliga.
#include <Headers/kern_api.hpp>
#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <sys/proc.h>
#include "g15_filelog.hpp"

static mach_vm_address_t gNaOrg = 0;
static uint32_t gWk[7] {};   // [5] = aberturas recusadas (coletores com a RTX parada), [6] = deixadas sem religar (RtxLazy)
static UInt32 gWkType = 0;
static void wkPublish() { IORegistryEntry::getRegistryRoot()->setProperty("G15RtxWake", gWk, sizeof(gWk)); }

// Coletores periodicos (systemstats abre o NVAccel tipo 33 a cada ~10 min): com a RTX parada, recusa em vez de religar.
static bool wkIsCollector() {
	char nm[24] {}; proc_selfname(nm, sizeof(nm));
	return !strcmp(nm, "systemstats");
}

// (11/11) Perfil Bateria: o G15.app liga "RtxDeny" no G15PMControl e, com a RTX parada, toda abertura e recusada (o Metal
// abre cada placa so para lista-las; quem nao esta na nvmtl-allow.txt usa a Intel de qualquer jeito).
static bool gWkDenyAll = false;
void rtxWakeSetDeny(bool d) { gWkDenyAll = d; }
bool rtxWakeGetDeny() { return gWkDenyAll; }

// (11/11) "Religar no uso de verdade" (RtxLazy, G15.app liga junto com o revoke): todo programa abre o NVAccel com
// tipo baixo (5...) so para listar as placas do Metal; isso nao religa mais. Quem vai usar a RTX abre um cliente do
// RM (tipo 0x4E56xxxx, NVK criando o dispositivo) ou chama o driver (NVRMUserClient::externalMethod, gancho do
// revoke): ai religa. "G15RtxLazyWho" = ultimos que abriram sem religar.
static bool gWkLazy = false;
void rtxWakeSetLazy(bool l) { gWkLazy = l; }
bool rtxWakeGetLazy() { return gWkLazy; }

// "nome pid tipo" nos ultimos 6 de uma lista do registro, o mais novo primeiro
struct WkWho { char w[6][40]; unsigned n; };
static void wkRecord(WkWho &l, const char *key) {
	for (unsigned k = 5; k > 0; k--) memcpy(l.w[k], l.w[k - 1], sizeof(l.w[0]));
	char nm[24] {}; proc_selfname(nm, sizeof(nm));
	snprintf(l.w[0], sizeof(l.w[0]), "%s %d t%u", nm, proc_selfpid(), gWkType);
	if (l.n < 6) l.n++;
	auto arr = OSArray::withCapacity(l.n);
	for (unsigned k = 0; arr && k < l.n; k++) { auto st = OSString::withCString(l.w[k]); if (st) { arr->setObject(st); st->release(); } }
	if (arr) { IORegistryEntry::getRegistryRoot()->setProperty(key, arr); arr->release(); }
}
static WkWho gWho {}, gDenyWho {}, gLazyWho {};

// true = recusar a abertura
static bool wkEnsure() {
	auto m = IOService::serviceMatching("G15Fan");
	auto it = m ? IOService::getMatchingServices(m) : nullptr;
	OSSafeReleaseNULL(m);
	auto fan = it ? OSDynamicCast(IOService, it->getNextObject()) : nullptr;
	if (fan) fan->retain();
	OSSafeReleaseNULL(it);
	if (!fan) return false;
	bool deny = false;
	auto parked = OSDynamicCast(OSBoolean, fan->getProperty("RtxParked"));
	// parada, ou acordando (NVRM ainda no estado 0): pede ao G15Fan e espera
	bool need = parked && parked->isTrue();
	if (!need) {
		auto mn = IOService::serviceMatching("NVRM");
		auto nv = mn ? OSDynamicCast(IOService, IOService::copyMatchingService(mn)) : nullptr;
		OSSafeReleaseNULL(mn);
		need = nv && nv->getPowerState() < 1;
		OSSafeReleaseNULL(nv);
	}
	if (need && wkIsCollector() && !checkKernelArgument("-g15rtxwake-all")) { gWk[5]++; deny = true; need = false; }
	if (need && gWkDenyAll) {                       // perfil Bateria: ninguem religa; "G15RtxDenyWho" = ultimos recusados
		gWk[5]++; deny = true; need = false;
		wkRecord(gDenyWho, "G15RtxDenyWho");
		char nm[24] {}; proc_selfname(nm, sizeof(nm));
		g15fl::logf("G15PM: RTX parada, abertura de %s recusada (perfil Bateria)", nm);
	}
	if (need && gWkLazy && (gWkType & 0xFFFF0000u) != 0x4E560000u) {   // so listando as placas: nao religa
		gWk[6]++; need = false;
		wkRecord(gLazyWho, "G15RtxLazyWho");
	}
	if (need) {
		uint32_t ms = 0;
		auto sym = OSSymbol::withCString("G15RtxWake");
		IOReturn e = sym ? fan->callPlatformFunction(sym, true, &ms, nullptr, nullptr, nullptr) : kIOReturnNoMemory;
		OSSafeReleaseNULL(sym);
		gWk[1]++; gWk[2] = ms;
		wkRecord(gWho, "G15RtxWakeWho");             // quem religou a RTX
		if (e != kIOReturnSuccess || ms == 0xFFFFFFFF) gWk[3]++;
		g15fl::logf("G15PM: RTX pedida por um programa: religada (0x%x, %u ms)", e, ms);
	}
	fan->release();
	return deny;
}

static IOReturn naNewUC(IOService *that, task_t owningTask, void *securityID, UInt32 type, IOUserClient **handler) {
	gWk[0]++; gWkType = type;
	bool deny = wkEnsure();
	wkPublish();
	if (deny) return kIOReturnNotReady;
	return FunctionCast(naNewUC, gNaOrg)(that, owningTask, securityID, type, handler);
}

// (16/11) RTX como eGPU (docs/RTX-EGPU.md), so com -g15egpu.
// v1 (IOPCITunnelled no NVAccel) funcionou no Metal mas o repouso caiu no S3 (0x1F): com o bit 13 do config o
// IOGraphicsAccelerator2 roda o caminho de eGPU no start/systemWillSleep/systemDidWake. v2: o kernel fica como antes; so
// o que vai para a user space muda: IOAccelDevice2::get_config devolve o bit 13 (Metal: isRemovable, location=external)
// quando o dono e o NVAccel, e o NVAccel ganha Removable=Yes (libGFXShared: "Preferir GPU externa"/preferRemovable).
// Registro G15Egpu = {gancho start, starts, gancho get_config, configs marcados}.
static mach_vm_address_t gNaStartOrg = 0, gGcOrg = 0;
static IOService *gNvAccel = nullptr;
static uint32_t gEg[4] {};
static void egPublish() { IORegistryEntry::getRegistryRoot()->setProperty("G15Egpu", gEg, sizeof(gEg)); }
static bool naStart(IOService *that, IOService *provider) {
	bool r = FunctionCast(naStart, gNaStartOrg)(that, provider);
	if (r) { gNvAccel = that; that->setProperty("Removable", kOSBooleanTrue); }
	gEg[1]++; egPublish();
	g15fl::logf("G15PM: NVAccel::start -> %d, Removable (eGPU)", r);
	return r;
}
static IOReturn gcHook(IOService *that, void *out) {
	IOReturn r = FunctionCast(gcHook, gGcOrg)(that, out);
	if (r == kIOReturnSuccess && out && gNvAccel && that->getProvider() == gNvAccel) {
		*static_cast<uint32_t *>(out) |= 0x2000;
		gEg[3]++; egPublish();
	}
	return r;
}
static const char *gAfPath[] {"/System/Library/Extensions/IOAcceleratorFamily2.kext/Contents/MacOS/IOAcceleratorFamily2"};
static KernelPatcher::KextInfo gAf {"com.apple.iokit.IOAcceleratorFamily2", gAfPath, arrsize(gAfPath), {true}, {}, KernelPatcher::KextInfo::Unloaded};
static void egpuGcInstall() {
	lilu.onKextLoadForce(&gAf, 1, [](void *, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		if (!gAf.loadIndex || index != gAf.loadIndex) return;
		mach_vm_address_t fn = patcher.solveSymbol(index, "__ZN14IOAccelDevice210get_configEP23IOAccelDeviceConfigData", address, size);
		if (!fn) { gEg[2] = 2; patcher.clearError(); }
		else {
			gGcOrg = patcher.routeFunction(fn, reinterpret_cast<mach_vm_address_t>(gcHook), true);
			if (patcher.getError() != KernelPatcher::Error::NoError) { gGcOrg = 0; gEg[2] = 3; patcher.clearError(); }
			else gEg[2] = 1;
		}
		egPublish();
		g15fl::logf("G15PM: gancho do IOAccelDevice2::get_config (eGPU) %s", gEg[2] == 1 ? "instalado" : "FALHOU");
	});
}

static const char *gNaPath[] {"/Library/Extensions/NVAccel.kext/Contents/MacOS/NVAccel"};
static KernelPatcher::KextInfo gNa {"com.nullmoth.NVAccel", gNaPath, arrsize(gNaPath), {true}, {}, KernelPatcher::KextInfo::Unloaded};

void rtxWakeInstall() {
	static bool wake, egpu;
	wake = !checkKernelArgument("-g15rtxwake-off");
	egpu = checkKernelArgument("-g15egpu");
	if (!wake && !egpu) return;
	if (egpu) egpuGcInstall();
	if (wake) { gWk[4] = 9; wkPublish(); }   // 9 = esperando o NVAccel carregar
	lilu.onKextLoadForce(&gNa, 1, [](void *, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		if (!gNa.loadIndex || index != gNa.loadIndex) return;
		if (egpu) {
			mach_vm_address_t st = patcher.solveSymbol(index, "__ZN7NVAccel5startEP9IOService", address, size);
			if (!st) { gEg[0] = 2; patcher.clearError(); }
			else {
				gNaStartOrg = patcher.routeFunction(st, reinterpret_cast<mach_vm_address_t>(naStart), true);
				if (patcher.getError() != KernelPatcher::Error::NoError) { gNaStartOrg = 0; gEg[0] = 3; patcher.clearError(); }
				else gEg[0] = 1;
			}
			egPublish();
			g15fl::logf("G15PM: gancho do NVAccel::start (eGPU) %s", gEg[0] == 1 ? "instalado" : "FALHOU");
		}
		if (!wake) return;
		mach_vm_address_t fn = patcher.solveSymbol(index, "__ZN7NVAccel13newUserClientEP4taskPvjPP12IOUserClient", address, size);
		if (!fn) { gWk[4] = 2; patcher.clearError(); wkPublish(); g15fl::logf("G15PM: NVAccel::newUserClient nao achado"); return; }
		gNaOrg = patcher.routeFunction(fn, reinterpret_cast<mach_vm_address_t>(naNewUC), true);
		if (patcher.getError() != KernelPatcher::Error::NoError) { gNaOrg = 0; gWk[4] = 3; patcher.clearError(); }
		else gWk[4] = 1;
		wkPublish();
		g15fl::logf("G15PM: gancho do NVAccel::newUserClient %s", gWk[4] == 1 ? "instalado" : "FALHOU");
	});
}
