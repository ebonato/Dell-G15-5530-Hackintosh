//  G15PM: Bluetooth USB que dorme quando ocioso (14/11/2026, WhateverXe #87).
//
//  O BT do G15 (Intel AX201, 8087:0026) e um aparelho USB Full Speed cujo dono exclusivo e o bluetoothd. O IOUSBHostFamily
//  so suspende um aparelho ocioso se o driver ligar uma politica de ocio (IOUSBHostFamily.h, "Idle suspend" e "Pausing IO":
//  IOUSBHostInterface::setIdlePolicy + IOUSBHostPipe::setIdlePolicy nos IN interrupt/bulk, que sempre tem leitura
//  pendente). O bluetoothd nao liga (nos Macs o BT e UART/PCIe), entao com o BT ligado a porta nunca suspende e o pacote
//  fica em PC2 (+5 W). Aqui o IOUSBHostInterface::copyPipe e trocado (Lilu): quando o bluetoothd pega os pipes IN 0x81
//  (eventos HCI) e 0x82 (ACL) da interface 0 de um BT Intel, liga a politica nos dois e na interface. O aparelho suspende
//  e acorda sozinho (remote wake) quando tem dado. Como no Linux (btusb autosuspend), o risco e o firmware nao acordar.
//  sysctl debug.g15btidle = ms da politica (padrao 2000; 0 = nao liga). Vale na proxima vez que o BT ligar.
//  (22/11) Padrao 50 -> 2000 ms (o do autosuspend do Linux): com um mouse BLE (G603) e 50 ms o controlador suspendia a
//  cada pausa (centenas de ciclos por minuto) e depois de ~1000 o firmware do AX201 parou de pedir remote wake: a porta
//  ficou suspensa e o mouse "conectado" sem funcionar. Com 2000 ms: 25 ciclos em 20 min de uso, sem travar
//  (logs/macos/bt-g603-btidle2000-*.txt); parado, o BT continua suspenso a maior parte do tempo.
//  Registro "G15BtIdle" = {gancho (1 ok, 2 simbolo, 3 rota), pipes ligados, interfaces ligadas, ultimo IOReturn do pipe,
//  ultimo IOReturn da interface, ms}. -g15btidle-off desliga.
#include <Headers/kern_api.hpp>
#include <IOKit/IOService.h>
#include <sys/sysctl.h>
#include <kern/thread_call.h>
#include <sys/proc.h>
#include <pexpert/pexpert.h>
#include "g15_filelog.hpp"

static mach_vm_address_t gCpOrg = 0, gPipeIdle = 0, gIfIdle = 0;
static void *cpHook(IOService *that, uint8_t address);
static uint32_t gBi[7] {};   // [6] = IntelBluetoothFirmware tirados da arvore de energia (automatico)
static int gBiMs = 2000;
static void biPublish() { gBi[5] = (uint32_t)gBiMs; IORegistryEntry::getRegistryRoot()->setProperty("G15BtIdle", gBi, sizeof(gBi)); }

static uint32_t biNum(IORegistryEntry *e, const char *k) {
	auto n = e ? OSDynamicCast(OSNumber, e->getProperty(k)) : nullptr;
	return n ? n->unsigned32BitValue() : 0xFFFFFFFFu;
}

// (14/11, 2a etapa) Mesmo com a politica a porta nao suspendia: o IntelBluetoothFirmware (carrega o firmware no boot e
// fica parado) segue como filho de energia do IOUSBHostDevice num estado sem kIOPMLowPower, e o IOUSBHostFamily.h diz que
// isso desliga o idle suspend. Com PMstop nele (sysctl debug.g15btidle_fw=1) a porta passou a ficar 63% suspensa.
// 3a etapa: feito sozinho, numa thread_call, na 1a vez que o bluetoothd pega um pipe de cada aparelho BT (cada
// reenumeracao cria aparelho e IntelBluetoothFirmware novos). debug.g15btidle_fw: 1 = PMstop, 2 = terminate (a mao);
// a leitura devolve quantos foram tratados na ultima vez.
static int gBiFw = 0;
static int fwDetach(int v) {
	int n = 0;
	// o IntelBluetoothFirmware nao e registrado (getMatchingServices nao o ve): procura nos filhos do BT Intel
	auto m = IOService::serviceMatching("IOUSBHostDevice");
	auto it = m ? IOService::getMatchingServices(m) : nullptr;
	OSSafeReleaseNULL(m);
	while (auto dev = it ? OSDynamicCast(IOService, it->getNextObject()) : nullptr) {
		if (biNum(dev, "idVendor") != 0x8087 || biNum(dev, "bDeviceClass") != 0xE0) continue;
		auto ci = dev->getChildIterator(gIOServicePlane);
		while (auto c = ci ? OSDynamicCast(IOService, ci->getNextObject()) : nullptr) {
			if (!c->metaCast("IntelBluetoothFirmware")) continue;
			if (v == 1) { if (!c->getProperty("G15PMStop")) { c->PMstop(); c->setProperty("G15PMStop", true); n++; } }
			else { c->terminate(); n++; }
			g15fl::logf("G15PM: IntelBluetoothFirmware: %s", v == 1 ? "PMstop" : "terminate");
		}
		OSSafeReleaseNULL(ci);
	}
	OSSafeReleaseNULL(it);
	gBiFw = n;
	return n;
}
static int biFwSysctl SYSCTL_HANDLER_ARGS {
	int v = gBiFw; const int e = sysctl_handle_int(oidp, &v, 0, req);
	if (e || !req->newptr || (v != 1 && v != 2)) return e;
	fwDetach(v);
	return 0;
}
static thread_call_t gFwCall = nullptr;
static uint64_t gFwDevId = 0;
static void fwCallFn(thread_call_param_t, thread_call_param_t) { gBi[6] += (uint32_t)fwDetach(1); biPublish(); }

// (22/11/2026) Receptor USB de mouse/teclado sem fio da Logitech (046d, "USB Receiver"; aqui o c539 Lightspeed): 3
// interfaces HID (teclado, mouse, HID++), cada uma com um IN de interrupcao sondado a cada 1 ms (bInterval 1), e o
// driver HID da Apple (AppleUserHIDDrivers, DriverKit) nao liga politica de ocio: o xHCI busca descritores na memoria a
// cada ms e o pacote fica em PC2 (+2 W na bateria com a RTX em GC6, docs/RTX-GC6.md 22/11 00:36-00:43:
// PKGC_BREAK_CAUSE_PCIE_FRM_NONZERO ~460/s). O receptor anuncia remote wakeup (bmAttributes 0xa0): com a politica o
// aparelho suspende depois de N ms sem relatorio e acorda sozinho quando o mouse/teclado manda algo (o 1o movimento
// depois do ocio espera a retomada do USB, ~20-40 ms). Liga em dois caminhos: no gancho do copyPipe (se o driver em
// DriverKit passar por ele) e numa varredura (10 s depois de o receptor aparecer e a cada escrita da sysctl) que pede
// os pipes IN 0x81-0x87 das interfaces HID com o copyPipe original e liga a politica neles e na interface.
// sysctl debug.g15usbidle = ms (padrao 2000; 0 = desliga, vale na hora). -g15usbidle-off nao instala nada.
// (22/11, 2a etapa) So a politica nao basta: os 3 AppleUserHIDDevice (o lado do kernel do driver HID em DriverKit) sao
// filhos de energia do IOUSBHostDevice num estado sem kIOPMLowPower, e o IOUSBHostFamily.h diz que isso desliga o idle
// suspend (a porta ficou 100 % ligada; ChildrenPowerState 2). Mesmo caso do IntelBluetoothFirmware no BtIdle. Com
// debug.g15usbidle_pm=1 (ou boot-arg g15usbidle_pm=1) eles saem da arvore de energia (PMstop) e o aparelho pode ir ao
// estado 1 (kIOPMLowPower = suspenso). Padrao 0: experimental (o driver HID deixa de receber os avisos de energia).
// Registro "G15UsbIdle" = {receptores, interfaces ok, pipes ok, IOReturn interface, IOReturn pipe, ms, pelo gancho,
// varreduras, AppleUserHIDDevice tirados da arvore de energia}.
static int gUiMs = 2000, gUiPm = 0;
static uint32_t gUi[9] {};
static bool gUiOn = false;
static thread_call_t gUiCall = nullptr;
static void uiPublish() { gUi[5] = (uint32_t)gUiMs; IORegistryEntry::getRegistryRoot()->setProperty("G15UsbIdle", gUi, sizeof(gUi)); }
static bool uiIsReceiver(IORegistryEntry *dev) {
	if (!dev || biNum(dev, "idVendor") != 0x046d) return false;
	auto nm = OSDynamicCast(OSString, dev->getProperty("USB Product Name"));
	return nm && strstr(nm->getCStringNoCopy(), "Receiver");
}
static void uiSet(IOService *iface, void *pipe) {
	auto setPipe = reinterpret_cast<IOReturn (*)(void *, uint32_t)>(gPipeIdle);
	auto setIf = reinterpret_cast<IOReturn (*)(void *, uint32_t)>(gIfIdle);
	if (iface && setIf) { IOReturn r = setIf(iface, (uint32_t)gUiMs); gUi[1] += r == kIOReturnSuccess; gUi[3] = (uint32_t)r; }
	if (pipe && setPipe) { IOReturn r = setPipe(pipe, (uint32_t)gUiMs); gUi[2] += r == kIOReturnSuccess; gUi[4] = (uint32_t)r; }
}
static void uiScan(thread_call_param_t, thread_call_param_t) {
	gUi[7]++;
	uint32_t found = 0;
	auto m = IOService::serviceMatching("IOUSBHostDevice");
	auto it = m ? IOService::getMatchingServices(m) : nullptr;
	OSSafeReleaseNULL(m);
	while (auto dev = it ? OSDynamicCast(IOService, it->getNextObject()) : nullptr) {
		if (!uiIsReceiver(dev)) continue;
		found++;
		auto ci = dev->getChildIterator(gIOServicePlane);
		while (auto c = ci ? OSDynamicCast(IOService, ci->getNextObject()) : nullptr) {
			if (!c->metaCast("IOUSBHostInterface") || biNum(c, "bInterfaceClass") != 3) continue;
			uiSet(c, nullptr);
			if (gUiPm == 1 && gUiMs > 0) {   // 2a etapa: o driver HID fora da arvore de energia do aparelho
				auto hi = c->getChildIterator(gIOServicePlane);
				while (auto h = hi ? OSDynamicCast(IOService, hi->getNextObject()) : nullptr) {
					if (strcmp(h->getMetaClass()->getClassName(), "AppleUserHIDDevice") || h->getProperty("G15PMStop")) continue;
					h->PMstop(); h->setProperty("G15PMStop", true); gUi[8]++;
					g15fl::logf("G15PM: receptor USB: AppleUserHIDDevice da interface %u fora da arvore de energia", biNum(c, "bInterfaceNumber"));
				}
				OSSafeReleaseNULL(hi);
			}
			if (!gCpOrg) continue;
			for (uint8_t a = 0x81; a <= 0x87; a++) {
				auto pipe = reinterpret_cast<OSObject *>(FunctionCast(cpHook, gCpOrg)(c, a));
				if (!pipe) continue;
				uiSet(nullptr, pipe);
				pipe->release();
			}
		}
		OSSafeReleaseNULL(ci);
	}
	OSSafeReleaseNULL(it);
	gUi[0] = found;
	uiPublish();
	if (gUi[7] <= 6) g15fl::logf("G15PM: receptor USB: %u achado(s), politica de ocio %d ms (interfaces ok %u, pipes ok %u, 0x%x/0x%x)",
	                             found, gUiMs, gUi[1], gUi[2], gUi[3], gUi[4]);
}
static bool uiMatched(void *, void *, IOService *dev, IONotifier *) {
	if (gUiCall && uiIsReceiver(dev)) {
		uint64_t dl; clock_interval_to_deadline(10, kSecondScale, &dl);
		thread_call_enter_delayed(gUiCall, dl);
	}
	return true;
}

static void *cpHook(IOService *that, uint8_t address) {
	void *pipe = FunctionCast(cpHook, gCpOrg)(that, address);
	if (!pipe) return pipe;
	IOService *dev = that->getProvider();
	if (gUiOn && gUiMs > 0 && (address & 0x80) && uiIsReceiver(dev) && biNum(that, "bInterfaceClass") == 3) {
		uiSet(that, pipe); gUi[6]++; uiPublish();
		return pipe;
	}
	if (gBiMs <= 0) return pipe;
	if (biNum(dev, "idVendor") != 0x8087 || biNum(dev, "bDeviceClass") != 0xE0) return pipe;
	// so quando quem abre e o bluetoothd: a 1a abertura (~2 s do boot) e do proprio IntelBluetoothFirmware no start()
	if (gFwCall && dev->getRegistryEntryID() != gFwDevId) {
		char nm[24] {}; proc_selfname(nm, sizeof(nm));
		if (!strcmp(nm, "bluetoothd")) { gFwDevId = dev->getRegistryEntryID(); thread_call_enter(gFwCall); }
	}
	auto setPipe = reinterpret_cast<IOReturn (*)(void *, uint32_t)>(gPipeIdle);
	auto setIf = reinterpret_cast<IOReturn (*)(void *, uint32_t)>(gIfIdle);
	IOReturn ri = setIf ? setIf(that, (uint32_t)gBiMs) : kIOReturnUnsupported;   // toda interface do BT (a 1 e a do audio SCO)
	gBi[2] += ri == kIOReturnSuccess; gBi[4] = (uint32_t)ri;
	if ((address != 0x81 && address != 0x82) || biNum(that, "bInterfaceNumber") != 0) { biPublish(); return pipe; }
	IOReturn rp = setPipe ? setPipe(pipe, (uint32_t)gBiMs) : kIOReturnUnsupported;
	gBi[1] += rp == kIOReturnSuccess; gBi[3] = (uint32_t)rp;
	biPublish();
	if (gBi[1] <= 4) g15fl::logf("G15PM: BT pipe 0x%02x: politica de ocio %d ms (pipe 0x%x, interface 0x%x)", address, gBiMs, rp, ri);
	return pipe;
}

SYSCTL_PROC(_debug, OID_AUTO, g15btidle_fw, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_LOCKED, nullptr, 0, biFwSysctl, "I", "");

static int biSysctl SYSCTL_HANDLER_ARGS {
	int v = gBiMs; const int e = sysctl_handle_int(oidp, &v, 0, req);
	if (!e && req->newptr) { gBiMs = v < 0 ? 0 : v > 10000 ? 10000 : v; biPublish(); }
	return e;
}
SYSCTL_PROC(_debug, OID_AUTO, g15btidle, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_LOCKED, nullptr, 0, biSysctl, "I", "");

static int uiSysctl SYSCTL_HANDLER_ARGS {
	int v = gUiMs; const int e = sysctl_handle_int(oidp, &v, 0, req);
	if (!e && req->newptr) { gUiMs = v < 0 ? 0 : v > 60000 ? 60000 : v; if (gUiCall) thread_call_enter(gUiCall); }   // 0 = desliga
	return e;
}
SYSCTL_PROC(_debug, OID_AUTO, g15usbidle, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, nullptr, 0, uiSysctl, "I", "");
static int uiPmSysctl SYSCTL_HANDLER_ARGS {   // 1 = PMstop nos AppleUserHIDDevice do receptor (na hora; volta so reiniciando)
	int v = gUiPm; const int e = sysctl_handle_int(oidp, &v, 0, req);
	if (!e && req->newptr) { gUiPm = v == 1 ? 1 : 0; if (gUiPm && gUiCall) thread_call_enter(gUiCall); }
	return e;
}
SYSCTL_PROC(_debug, OID_AUTO, g15usbidle_pm, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, nullptr, 0, uiPmSysctl, "I", "");

static const char *gUsbPath[] {"/System/Library/Extensions/IOUSBHostFamily.kext/Contents/MacOS/IOUSBHostFamily"};
static KernelPatcher::KextInfo gUsb {"com.apple.iokit.IOUSBHostFamily", gUsbPath, arrsize(gUsbPath), {true}, {}, KernelPatcher::KextInfo::Unloaded};

void btIdleInstall() {
	if (checkKernelArgument("-g15btidle-off")) return;
	sysctl_register_oid(&sysctl__debug_g15btidle); sysctl_register_oid(&sysctl__debug_g15btidle_fw);
	gFwCall = thread_call_allocate(fwCallFn, nullptr);
	gBi[0] = 9; biPublish();   // 9 = esperando o IOUSBHostFamily
	if (!checkKernelArgument("-g15usbidle-off")) {
		PE_parse_boot_argn("g15usbidle", &gUiMs, sizeof(gUiMs));
		PE_parse_boot_argn("g15usbidle_pm", &gUiPm, sizeof(gUiPm));
		sysctl_register_oid(&sysctl__debug_g15usbidle); sysctl_register_oid(&sysctl__debug_g15usbidle_pm);
		gUiCall = thread_call_allocate(uiScan, nullptr);
		gUiOn = gUiCall != nullptr;
		uiPublish();
	}
	lilu.onKextLoadForce(&gUsb, 1, [](void *, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		if (!gUsb.loadIndex || index != gUsb.loadIndex) return;
		gPipeIdle = patcher.solveSymbol(index, "__ZN13IOUSBHostPipe13setIdlePolicyEj", address, size); patcher.clearError();
		gIfIdle = patcher.solveSymbol(index, "__ZN18IOUSBHostInterface13setIdlePolicyEj", address, size); patcher.clearError();
		mach_vm_address_t fn = patcher.solveSymbol(index, "__ZN18IOUSBHostInterface8copyPipeEh", address, size);
		if (!fn || !gPipeIdle || !gIfIdle) { gBi[0] = 2; patcher.clearError(); biPublish(); g15fl::logf("G15PM: simbolos do IOUSBHost nao achados"); return; }
		gCpOrg = patcher.routeFunction(fn, reinterpret_cast<mach_vm_address_t>(cpHook), true);
		if (patcher.getError() != KernelPatcher::Error::NoError) { gCpOrg = 0; gBi[0] = 3; patcher.clearError(); }
		else gBi[0] = 1;
		biPublish();
		if (gUiOn && gBi[0] == 1) {   // receptor ja presente no boot ou ligado depois: varredura 10 s depois de aparecer
			auto m = IOService::serviceMatching("IOUSBHostDevice");
			if (m) { IOService::addMatchingNotification(gIOMatchedNotification, m, uiMatched, nullptr); m->release(); }
		}
		g15fl::logf("G15PM: gancho do IOUSBHostInterface::copyPipe %s", gBi[0] == 1 ? "instalado" : "FALHOU");
	});
}
