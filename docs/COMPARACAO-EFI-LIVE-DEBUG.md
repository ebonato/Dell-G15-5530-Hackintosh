# Comparação das EFIs do Dell G15 5530

A EFI que Rodrigo Rangel publicou para instalar e iniciar o Tahoe contém o **OpenCore 1.0.8 Debug**. O carregador UEFI, o OpenCore e o OpenRuntime dele coincidem byte a byte com a distribuição Debug da Acidanthera. Já no perfil de instalação, os **23 kexts habilitados que contêm código são Release**. Na nossa Live Debug, há **14 kexts Debug e 4 Release** habilitados. `UTBMap` fornece o mapa USB por meio de um plist.

## Versões e perfis comparados

Comparação de 22/11/2026. As árvores locais `EFI` e `EFI-Live-Debug` correspondem ao commit `501b682` do fork. Para identificar o que Rodrigo Rangel (GitHub `rodgod-rr`, Discord `rush05765`) distribuiu, a referência é a árvore `EFI` de `upstream/main` em `db70d93`. O equipamento dele tem i5-13450HX e RTX 4050. O nosso tem i7-13650HX, RTX 3050 6 GB e BIOS 1.37.0. Os dois têm iGPU Intel UHD A78B e painel de 120 Hz. [EFI de Rodrigo Rangel](https://github.com/rodgod-rr/Dell-G15-5530-Hackintosh/tree/db70d9337096bfbba3ae7c4ca563f3edf1910550/EFI) · [Hardware e histórico do Dell](HARDWARE-LOCAL.md)

| Perfil | Plist comparado | Tabelas em `ACPI > Add` | Regras em `ACPI > Patch` | Entradas em `Kernel > Add` | Drivers UEFI |
| --- | --- | ---: | ---: | ---: | ---: |
| Rodrigo · instalação | `OC/config-install.plist` | 6 | 0 | 25 | 2 |
| Rodrigo · sistema | `OC/config.plist` | 11 | 0 | 31 | 2 |
| Fork · instalação | `OC/config-install.plist` | 6 | 6 | 25 | 3 |
| Fork · sistema | `OC/config.plist` | 9 | 1 | 31 | 2 |
| Live Debug | `OC/config.plist` | 11 | 0 | 19 | 5 |

Os números contam entradas habilitadas. Nos perfis de instalação e sistema de Rodrigo e do fork, `AirportItlwm` se aplica ao Darwin 25.x usado pelo Tahoe. A Live usa um único plist para o teste de Recuperação. [Instalação do fork](../EFI/OC/config-install.plist) · [Sistema do fork](../EFI/OC/config.plist) · [Live Debug](../EFI-Live-Debug/OC/config.plist)

Desde o ancestral comum `4cb55f4`, o fork acrescentou cinco AML (EC, HPET, RTCAWAC, SBUS-MCHC e USBX), `OpenHfsPlus.efi` e `UTBMap.kext` à pasta `EFI` e editou os dois plists. Do lado de Rodrigo, os executáveis de `G15Fan` e `G15PM` receberam atualizações posteriores. Isso explica por que uma correspondência entre as pastas locais pode ter origem no nosso próprio trabalho.

## Arquivos em comum

| Comparação | Arquivos em cada árvore | Mesmo caminho relativo | Conteúdo idêntico no mesmo caminho | Apenas na primeira / apenas na Live |
| --- | --- | ---: | ---: | ---: |
| `EFI` × `EFI-Live-Debug`, arquivos locais | 157 / 119 | 33 | 6 | 124 / 86 |
| `EFI` × `EFI-Live-Debug`, objetos Git do fork | 157 / 119 | 33 | 10 | 124 / 86 |
| EFI original de Rodrigo × Live, objetos Git | 150 / 119 | 29 | 7 | 121 / 90 |

A contagem abrange arquivos ocultos, recursos do seletor, fontes, assinaturas e plug-ins. A diferença entre seis correspondências no checkout e dez no Git vem de quatro arquivos de texto com finais de linha CRLF/LF. O anexo de arquivos lista os 33 caminhos compartilhados e seus hashes. Fora dos caminhos de mesmo nome, há também dois AML idênticos de RTC e SMBus.

A `EFI` de Rodrigo e a do fork contêm 58 arquivos de fontes, imagens e rótulos em `OC/Resources`. A Live mantém quatro marcadores nessas pastas e usa o seletor `Builtin`, também selecionado nos quatro perfis derivados de Rodrigo. O Lilu da Live inclui cabeçalhos de desenvolvimento e o VirtualSMC inclui cabeçalhos do SDK. Isso também influencia a contagem de arquivos por bundle.

## OpenCore, drivers e ferramenta UEFI

Os prefixos abaixo são os primeiros 12 dígitos do SHA-256 do executável. As compilações da Acidanthera e do Dortania têm o mesmo número de versão. Os sete executáveis Debug da Live coincidem com o pacote do Dortania `6fe4d15` e com a base Debug local de Gabriel Luchina. [Pacotes e hashes da Acidanthera 1.0.8](https://github.com/acidanthera/OpenCorePkg/releases/expanded_assets/1.0.8) · [Build do Dortania 6fe4d15](https://github.com/dortania/build-repo/releases/tag/OpenCorePkg-6fe4d15)

| Arquivo | Rodrigo Rangel / `EFI` do fork | `EFI-Live-Debug` | Função ou perfil |
| --- | --- | --- | --- |
| `BOOT/BOOTx64.efi` | Acidanthera Debug `7b26e33bfd84` | Dortania Debug `c4c8e264ad0f` | Entrada de boot x64 pela mídia. |
| `OC/OpenCore.efi` | Acidanthera Debug `11ba0b763d76` | Dortania Debug `4894c595476b` | Seletor e carregamento do sistema. |
| `OC/Drivers/OpenRuntime.efi` | Acidanthera Debug `cf2df818f751` | Dortania Debug `12b9c3048bf4` | Serviços de runtime UEFI em todos os perfis. |
| `OC/Drivers/ResetNvramEntry.efi` | Dortania Release `21d85c60f9fb` | Dortania Debug `5217eff38159` | Entrada de reset da NVRAM nos cinco perfis. |
| `OC/Drivers/OpenHfsPlus.efi` | Acidanthera Release `ebc5d139ecfa` (só fork) | — | Leitura de HFS+ no instalador do fork. |
| `OC/Drivers/HfsPlus.efi` | — | Variante indeterminada `a55b5fff3657` | Leitura de HFS+ na Live. Mesmo arquivo na base Debug e Release de Luchina. |
| `OC/Drivers/AudioDxe.efi` | — | Dortania Debug `8369c0cd572a` | Áudio do ambiente UEFI na Live. |
| `OC/Drivers/FirmwareSettingsEntry.efi` | — | Dortania Debug `93cd9a015c4b` | Entrada para os ajustes do firmware na Live. |
| `OC/Tools/ControlMsrE2.efi` | — | Dortania Debug `d5b918933cee` | Ferramenta auxiliar selecionável no menu da Live. |

`Misc > Debug > Target` vale `65` (`0x41`) nos perfis derivados de Rodrigo e `67` (`0x43`) na Live. Ambos habilitam log em arquivo. A Live habilita também o console do OpenCore. `AppleDebug` e `ApplePanic` estão ligados no instalador do fork e na Live, e desligados nos dois perfis originais e no sistema do fork. O nível `DisplayLevel` é `0x80000042` nos perfis derivados de Rodrigo e `0x80000002` na Live. `-v` e `debug=0x100` são argumentos passados ao macOS. Em todos os perfis, `OpenRuntime` e `ResetNvramEntry` são habilitados com `LoadEarly=false` e sem argumentos. [Manual do OpenCore 1.0.8, opções de diagnóstico](https://github.com/acidanthera/OpenCorePkg/blob/1.0.8/Docs/Configuration.tex#L3707-L3721)

## Kexts e variantes de compilação

| Conjunto | Debug | Release | Variante indeterminada | Só dados | Total |
| --- | ---: | ---: | ---: | ---: | ---: |
| Pasta original de Rodrigo, incluindo plug-ins | 1 | 27 | 2 | 3 | 33 |
| Pasta `EFI` do fork, incluindo plug-ins | 1 | 27 | 2 | 4 | 34 |
| Instalação, tanto Rodrigo quanto fork | 0 | 23 | 0 | 2 | 25 |
| Sistema, tanto Rodrigo quanto fork | 1 | 25 | 2 | 3 | 31 |
| Live Debug, inventário e perfil habilitado | 14 | 4 | 0 | 1 | 19 |

Cada linha conta uma vez cada bundle, incluindo plug-ins com `Info.plist` próprio. As linhas de perfil usam `Kernel > Add`. O inventário completo inclui uma segunda cópia de `VoodooInput` dentro do bundle PS/2. Os plists de Rodrigo e do fork selecionam a cópia do VoodooI2C. `G15Fan` e `AudShim` são os dois executáveis de variante indeterminada. Os scripts de `G15Fan` usam `-O2` e o de `AudShim` combina `-O2` e `-g`. As opções de compilador, por si só, não identificam uma configuração Xcode Debug ou Release. [Build de G15Fan](../src/G15Fan/build-mac.sh) · [Build de AudShim](../src/AudShim/build-kext.sh)

A identificação usou o executável dos pacotes publicados quando disponível, as marcas `DBG-` e `REL-` gravadas no Mach-O e os caminhos de objetos do próprio alvo Xcode, como `.build/Release/`. Conferi os dois pacotes de USBToolBox, os cinco executáveis do pacote VoodooPS2 e o AirportRTW para Tahoe. Os arquivos compactados foram conferidos contra os hashes publicados. [USBToolBox 1.2.0](https://github.com/USBToolBox/kext/releases/expanded_assets/1.2.0) · [VoodooPS2 2.3.8, build 7eab4a3](https://github.com/dortania/build-repo/releases/tag/VoodooPS2-7eab4a3) · [AirportRTW V2.3.3](https://github.com/souissiak1-maker/AirportRTW/releases/tag/V2.3.3)

### Componentes compartilhados

Os cinco perfis habilitam os primeiros nove bundles da tabela. Os números de versão vêm de `CFBundleVersion`.

| Bundle ou função | Rodrigo Rangel / `EFI` do fork | Live Debug | Correspondência |
| --- | --- | --- | --- |
| `AirportItlwm` | 2.4.0 Release | 2.4.0 Release | Wi-Fi Intel. Bundle idêntico nos objetos do Git. |
| `RealtekRTL8111` | 3.0.4 Release | 3.0.4 Release | Ethernet. Bundle idêntico no Git. A versão curta no plist é 3.0.0. |
| `Lilu` | 1.7.3 Release | 1.7.2 Debug | Mesma dependência dos plug-ins. Versão e compilação próprias de cada conjunto. |
| `NVMeFix` | 1.1.4 Release | 1.1.3 Debug | Mesmo suporte aos NVMe. |
| `RestrictEvents` | 1.1.7 Release | 1.1.6 Debug | Mesmos itens `sbvmm` e `cpuname` em `revpatch`. |
| `VirtualSMC` | 1.3.9 Release | 1.3.8 Debug | Emulação do SMC nos cinco perfis. |
| `SMCBatteryManager` | 1.3.9 Release | 1.3.8 Debug | Bateria. O recurso `SSDT-BATC.dsl` do bundle é idêntico no Git. |
| `SMCProcessor` | 1.3.9 Release | 1.3.8 Debug | Sensores da CPU. |
| `USBToolBox` | 1.2.0 Release | 1.2.0 Debug | Mesmo identificador e versão. Executáveis distintos dos pacotes publicados. |
| `UTBMap` | 1.1 · só dados (só fork) | 1.1 · só dados | Ativo no instalador do fork e na Live. Os dez índices aparecem abaixo. |
| Família `AMFIPass` | 1.4.1 Release | 1.4.1 Release | Ativo no sistema de Rodrigo/fork como `AMFIPass-Mod` e na Live como `AMFIPass`. |

O instalador do fork e a Live têm **dez caminhos de kext ativos em comum**, contando `UTBMap`. O instalador original tem nove. Entre esses dez, só `AirportItlwm` e `NVMeFix` têm filtros diferentes em `Kernel > Add`. Rodrigo e o fork restringem `AirportItlwm` a `x86_64` e Darwin `25.0.0–25.99.99` e definem Darwin `18.0.0` como mínimo do `NVMeFix`. A Live usa `Arch=Any` e limites vazios nas 19 entradas. Os filtros, os caminhos internos e a habilitação dos outros oito coincidem.

O `AMFIPass-Mod` de Rodrigo e o `AMFIPass` da Live declaram `com.dhinakg.AMFIPass`. Na arquitetura x86_64, a seção de instruções `__TEXT,__text` é idêntica. As seções `__TEXT,__const` e `__DATA,__data` diferem. Nos objetos do Git, o `Info.plist` dos dois tem os mesmos bytes, apesar dos nomes de pasta diferentes.

### Componentes específicos de cada conjunto

| Recurso | Rodrigo Rangel e `EFI` do fork | Live Debug |
| --- | --- | --- |
| Vídeo do instalador | `WhateverGreen` 1.7.2 Release com `-wegnoegpu`. | Inicialização com os argumentos de vídeo listados abaixo. Os dois perfis de instalação derivados de Rodrigo são os únicos que habilitam WhateverGreen. |
| Vídeo e energia no sistema | `WhateverXe` 0.8.2 com marca `DBG-082-2026-11-17`, `G15PM` Release, `G15Fan` e `G15AGPMIgpu` (só dados). | O conjunto de drivers de aceleração e energia de Rodrigo fica restrito aos perfis de sistema comparados. |
| CPU híbrida e TSC | `IntelHybridCores` 0.1.0 Release no sistema. | `CpuTopologyRebuild` 2.0.2 Debug e `CpuTscSync` 1.1.2 Debug. |
| Áudio | `AudShim` 0.9.10 no sistema. | `AppleALC` 1.9.8 Debug. `AudioDxe` atende à fase UEFI. |
| Wi-Fi e Bluetooth | Wi-Fi Intel com `AirportItlwm`. Bluetooth com `IntelBluetoothFirmware` 2.5.3, `IntelBTPatcher` 2.5.3 e `BlueToolFixup` 2.7.3, todos identificados como Release. | Mesmo executável Intel de Wi-Fi e também `AirportRTW` 2.3.3 Release para Wi-Fi Realtek. O bundle Realtek está habilitado na configuração. A associação de um driver depende do dispositivo PCI presente. |
| Entrada e brilho | `BrightnessKeys` 1.0.4, `VoodooI2C` 2.9.1, `VoodooI2CHID`, `VoodooGPIO`, `VoodooI2CServices`, `VoodooInput` e `VoodooPS2Controller` 2.3.8 com teclado, mouse e trackpad. Executáveis Release. | As tabelas de brilho PNLF e XOSI fazem parte da Live. A pilha Voodoo está presente apenas nas árvores derivadas de Rodrigo. |
| USB | `USBToolBox`, `XHCI-unsupported` (só dados) e `UTBDefault` nos perfis originais e no sistema do fork. O instalador do fork usa `UTBMap`. | `USBToolBox` e `UTBMap`. |
| Sensores e recursos do macOS | `VirtualSMC`, `SMCBatteryManager` e `SMCProcessor` nos dois perfis. | Aos três em comum se somam `SMCDellSensors`, `SMCLightSensor` e `SMCSuperIO` 1.3.8 Debug, além de `FeatureUnlock` 1.1.8 Debug. |

Na ordem de `Kernel > Add`, Lilu vem primeiro nos quatro perfis derivados de Rodrigo. A Live coloca `AirportItlwm` e `AirportRTW` antes de Lilu e declara `AMFIPass` em décimo. A documentação do AirportRTW 2.3.3 indica a sequência Lilu → AMFIPass → AirportRTW para o driver Wi-Fi Realtek. [Instruções do AirportRTW 2.3.3](https://github.com/souissiak1-maker/AirportRTW/blob/V2.3.3/README.md#how-to-use)

## ACPI e origem dos AML

| Função | Rodrigo Rangel | `EFI` do fork | Live Debug |
| --- | --- | --- | --- |
| DSDT principal | `DSDT.aml` da BIOS 1.0, habilitada na instalação e no sistema. Redireciona chamadas para XOSI e remove a declaração `_S3`. | A mesma cópia de 520.568 bytes está na pasta. Os dois perfis usam a DSDT do firmware e aplicam a regra de RP25 em `ACPI > Patch`. | Carrega `DSDT-G15-1.37.0.aml`, 653.360 bytes. Incorpora RP25, as cinco regras de HPET/IRQ e os renomes PNLF, OSID e _OSI. A descompilação conserva a declaração condicional `_S3`. |
| RTC/AWAC | `SSDT-AWAC` seleciona o RTC por `STAS`. | `SSDT-RTCAWAC` na instalação e `SSDT-AWAC` no sistema. | `SSDT-AWAC` é **idêntica** à `SSDT-RTCAWAC` do fork. O corpo AML também coincide com o da AWAC original após o cabeçalho ACPI de 36 bytes. |
| Gerenciamento de CPU | `SSDT-PLUG-ALT` nos dois perfis. | Mesma tabela de Rodrigo. | Mesmo corpo AML de PLUG-ALT, com cabeçalho diferente. |
| Controlador embarcado e energia USB | `SSDT-EC-USBX` reúne `EC` e `USBX`. Corrente `3000` e alimentação `9600`. | Na instalação, `SSDT-EC` e `SSDT-USBX` separadas. O sistema habilita a tabela combinada original. | `SSDT-EC` e `SSDT-USBX` são **idênticas** às do instalador do fork. O USBX usa `2100` e `5100`, respectivamente. |
| HPET e IRQs | A lista de tabelas adicionadas na EFI original não inclui um SSDT de HPET. | Instalador com `SSDT-HPET` e cinco regras na DSDT para HPET, IPIC, RTC e TIMR. | **Mesmo AML de HPET**. As cinco correções complementares estão incorporadas na DSDT da Live. |
| SMBus | Sem SSDT dedicado. | `SSDT-SBUS-MCHC` na instalação. | `SSDT-SBUS` é **idêntica** à `SSDT-SBUS-MCHC` do fork, inclusive no cabeçalho. |
| Brilho e identificação do SO | `SSDT-XOSI` nos dois perfis e `SSDT-PNLF` no sistema. O PNLF fica em `\_SB.PC00.GFX0.PNLF`. | XOSI desabilitado nos dois perfis. O sistema habilita o PNLF de Rodrigo. | XOSI e PNLF habilitados. As duas implementações de XOSI respondem a versões do Windows no Darwin com listas diferentes. O PNLF da Live fica em `\PNLF`. Ambos os PNLF usam `_UID=0x13`. |
| Repouso, tampa, teclado PS/2, RTX e Bluetooth | `SSDT-PS2K-RMCF` nos dois perfis. `SSDT-DeepIdle`, `SSDT-LIDWAKE`, `SSDT-RTXKEXT` e `SSDT-BTRK` no sistema. | O sistema habilita essas cinco tabelas. | Essas cinco tabelas são exclusivas da família de EFIs de Rodrigo. |
| Tabelas removidas e sensor de luz | Os dois perfis removem o SSDT OEM `xh_Dell_`. | Mesma remoção. | Mesma remoção. Também substitui a tabela DMAR por `SSDT-DMAR.aml` (assinatura real `DMAR`) e adiciona `SSDT-ALS0`. |

O instalador do fork e a Live compartilham as **mesmas cinco tabelas AML** para EC, USBX, RTC/AWAC, HPET e SMBus. Em duas delas o nome do arquivo mudou. Na comparação com a EFI original, AWAC e PLUG-ALT têm o mesmo corpo AML que os equivalentes da Live. As DSDTs de Rodrigo e da Live, por sua vez, têm o mesmo ID OEM `Dell Inc` e hashes diferentes. As revisões da BIOS, os pontos de montagem de PNLF e as regras de repouso importam ao comparar o comportamento. [Fontes do fork](../acpi-src/) · [Fontes da Live](../acpi-src-live/) · [Origem da DSDT 1.37.0](../acpi-native/README.md#dsdt-dos-perfis-live)

## Mapa USB do nosso Dell

Os dois `UTBMap.kext` declaram o mesmo controlador `XHCI`, os mesmos nomes de porta e os mesmos índices. `port-count` vale `0x17` nas duas cópias, correspondente ao maior índice, e há dez portas descritas. Os quatro índices SuperSpeed e os dois dispositivos internos têm o mesmo `UsbConnector`. As quatro diferenças estão no lado USB 2 dos conectores externos. [Mapa do instalador do fork](../EFI/OC/Kexts/UTBMap.kext/Contents/Info.plist) · [Mapa da Live](../EFI-Live-Debug/OC/Kexts/UTBMap.kext/Contents/Info.plist)

| Conector observado no Windows | Nome e índice USB 2 / USB 3 | `UsbConnector` no fork | `UsbConnector` na Live |
| --- | --- | --- | --- |
| Webcam interna | `HS01 (3)` | 255 | 255 |
| USB-A traseira | `HS02 (8)` / `SS01 (17)` | 3 / 3 | 0 / 3 |
| USB-A direita, posição do pendrive de Recuperação | `HS03 (9)` / `SS02 (19)` | 3 / 3 | 0 / 3 |
| USB-A direita, ao lado do pendrive | `HS04 (10)` / `SS03 (20)` | 3 / 3 | 0 / 3 |
| USB-C traseira, hub com teclado | `HS05 (11)` / `SS04 (23)` | 9 / 9 | 8 / 9 |
| Bluetooth Intel interno | `HS06 (14)` | 255 | 255 |

A EFI original usa o injetor `UTBDefault`. A correspondência desses dez índices é entre as duas configurações preparadas para o nosso Dell.

## Configuração do OpenCore

Dos 67 campos em `ACPI/Booter/Kernel/UEFI > Quirks`, **60 têm o mesmo valor em todos os cinco perfis**. O anexo de configuração reúne também as coincidências nas outras seções do plist.

| Seção | Valores em comum | Valores diferentes nos perfis derivados de Rodrigo e na Live |
| --- | --- | --- |
| `ACPI > Quirks` | 6/6. `ResetLogoStatus=true` e `RebaseRegions=false`. | Todos coincidem. |
| `Booter > Quirks` | 19/22, incluindo `AvoidRuntimeDefrag`, `DevirtualiseMmio`, `EnableSafeModeSlide`, `ProtectUefiServices`, `ProvideCustomSlide`, `RebuildAppleMemoryMap` e `SyncRuntimePermissions` ligados. | Rodrigo/fork e Live, respectivamente, usam `SetupVirtualMap=false/true`, `ResizeAppleGpuBars=-1/0` e `FixupAppleEfiImages=true/false`. A lista MMIO de Rodrigo/fork habilita `0xC0000000`, `0xF9000000` e `0xFF000000`. A lista da Live é vazia. |
| `Kernel > Quirks` | 21/23. `ProvideCurrentCpuInfo`, `DisableIoMapper`, `DisableLinkeditJettison` e `PanicNoKextDump` ligados. `XhciPortLimit=false` e `SetApfsTrimTimeout=-1`. | Rodrigo/fork habilitam `AppleXcpmCfgLock` e `CustomSMBIOSGuid`. Na Live ambos estão desligados. |
| `UEFI > Quirks` | 14/16. `RequestBootVarRouting=true`, `ResizeGpuBars=-1` e `TscSyncTimeout=0` em todos. | Rodrigo/fork e Live usam `ReleaseUsbOwnership=true/false` e `EnableVectorAcceleration=false/true`. |
| CPU e SMBIOS | Todos emulam a assinatura CPUID EAX `0x000A0655`, usam `ProvideCurrentCpuInfo`, `ProcessorType=3841` e `PlatformInfo > Automatic=true`. RestrictEvents recebe `revcpu=1` e os itens `cpuname` e `sbvmm` em `revpatch`. | Rodrigo/fork também mascaram bits de EBX (`0x00FF0000`), selecionam `MacBookPro16,1`, modo `Custom` e mantêm o nome i5-13450HX em `revcpuname`. A Live mascara apenas EAX, seleciona `MacBookPro16,2`, modo `Create` e nomeia o i7-13650HX. |
| Seletor e segurança | `ShowPicker=true`, `PickerMode=Builtin`, `ScanPolicy=0`, `DmgLoading=Signed`, `SecureBootModel=Disabled` e `Vault=Optional`. | `Timeout` é 5 s em Rodrigo e no sistema do fork, 0 no instalador do fork e 15 s na Live. `HideAuxiliary=false` no instalador do fork e na Live. |
| UEFI e volumes | `ConnectDrivers=true`, `APFS > EnableJumpstart=true`, `KeySupport=true` e `ProvideConsoleGop=true`. As seções `AppleInput`, `Input`, `Output` e `ProtocolOverrides` coincidem por inteiro. | `APFS > MinDate/MinVersion` usa `-1/-1` em Rodrigo/fork e `0/0` na Live. `AudioSupport` fica ligado somente na Live. |
| Vídeo e `DeviceProperties` | A Live e os dois perfis de instalação têm `DeviceProperties > Add` vazio. | Os dois perfis de sistema injetam propriedades da iGPU em `PciRoot(0x0)/Pci(0x2,0x0)`, inclusive `framebuffer-stolenmem`. |
| NVRAM e argumentos de boot | `WriteFlash=true`. Todos pedem `keepsyms=1 debug=0x100`. A Live e os instaladores têm `-v`. Os perfis de sistema e a Live compartilham `nvfb=1 nvaccel=1 nvfbheads=4`. | Os instaladores de Rodrigo/fork acrescentam `-wegnoegpu -vi2c-force-polling`. A Live usa, entre outros, `-nvkmsnosmooth`. `csr-active-config` em ordem de bytes do plist é `130a0000` nos dois instaladores, `430a0000` nos dois sistemas e `03080000` na Live. |
| Patches e bloqueios de kernel | Os três patches de HT da família de Rodrigo estão desabilitados. Os cinco perfis têm `Kernel > Force` vazio. | Os dois perfis de sistema bloqueiam `com.apple.iokit.IONDRVSupport`. A Live tem essa entrada desabilitada e habilita o patch do APFS identificado como `Force FileVault off` para Darwin 21 a 25. |

O `ocvalidate` 1.0.8 aceitou os cinco plists. Também foram conferidos os caminhos de cada driver UEFI, ferramenta, tabela AML, `Info.plist` e executável de kext habilitado. A análise identifica o que o OpenCore está configurado para oferecer ao macOS. A inicialização da Recuperação e a associação dos drivers ao hardware são verificações de boot no notebook.

<details>
<summary>Inventário dos 33 caminhos comuns entre as duas pastas locais</summary>

Hashes dos arquivos no checkout Windows. A igualdade foi verificada com o conteúdo completo. `CRLF/LF` indica que a normalização dos finais de linha torna os conteúdos idênticos.

| Caminho relativo à EFI | SHA-256 do fork (prefixo) | SHA-256 da Live (prefixo) | Resultado |
| --- | --- | --- | --- |
| `BOOT/BOOTx64.efi` | `7b26e33bfd84` | `c4c8e264ad0f` | Difere |
| `OC/ACPI/SSDT-AWAC.aml` | `120eee1da010` | `665933431fb4` | Mesmo corpo AML |
| `OC/ACPI/SSDT-EC.aml` | `46b8ead0b185` | `46b8ead0b185` | Idêntico |
| `OC/ACPI/SSDT-HPET.aml` | `04097b2e26f9` | `04097b2e26f9` | Idêntico |
| `OC/ACPI/SSDT-PLUG-ALT.aml` | `3370fec9a571` | `64331ae5c240` | Mesmo corpo AML |
| `OC/ACPI/SSDT-PNLF.aml` | `5af53cd0a50c` | `d33b45ac83c2` | Difere |
| `OC/ACPI/SSDT-USBX.aml` | `5ac27370e742` | `5ac27370e742` | Idêntico |
| `OC/ACPI/SSDT-XOSI.aml` | `96bc6e517805` | `fdd2117f961d` | Difere |
| `OC/config.plist` | `31f93888110c` | `f795c316c14b` | Difere |
| `OC/Drivers/OpenRuntime.efi` | `cf2df818f751` | `12b9c3048bf4` | Difere |
| `OC/Drivers/ResetNvramEntry.efi` | `21d85c60f9fb` | `5217eff38159` | Difere |
| `OC/Kexts/AirportItlwm.kext/Contents/Info.plist` | `87bad8512c80` | `87bad8512c80` | Idêntico |
| `OC/Kexts/AirportItlwm.kext/Contents/MacOS/AirportItlwm` | `259bac9432bf` | `259bac9432bf` | Idêntico |
| `OC/Kexts/Lilu.kext/Contents/Info.plist` | `7b5ee421ad1c` | `b7798755a2dd` | Difere |
| `OC/Kexts/Lilu.kext/Contents/MacOS/Lilu` | `e3f00df5aca9` | `438b912da905` | Difere |
| `OC/Kexts/NVMeFix.kext/Contents/Info.plist` | `7055cad05cc1` | `37c3f3927190` | Difere |
| `OC/Kexts/NVMeFix.kext/Contents/MacOS/NVMeFix` | `d32c74322001` | `8d44110df6b2` | Difere |
| `OC/Kexts/RealtekRTL8111.kext/Contents/_CodeSignature/CodeResources` | `a006159ca0dd` | `6686de10a28a` | CRLF/LF |
| `OC/Kexts/RealtekRTL8111.kext/Contents/Info.plist` | `1c043f21855a` | `73df871ead48` | CRLF/LF |
| `OC/Kexts/RealtekRTL8111.kext/Contents/MacOS/RealtekRTL8111` | `a9a99d105518` | `a9a99d105518` | Idêntico |
| `OC/Kexts/RestrictEvents.kext/Contents/Info.plist` | `c5424fe26266` | `1170129a0011` | Difere |
| `OC/Kexts/RestrictEvents.kext/Contents/MacOS/RestrictEvents` | `8a244c696234` | `6613749e0f8c` | Difere |
| `OC/Kexts/SMCBatteryManager.kext/Contents/Info.plist` | `b64867ece5c2` | `99f711358e27` | Difere |
| `OC/Kexts/SMCBatteryManager.kext/Contents/MacOS/SMCBatteryManager` | `791d81018a53` | `988e7917c24e` | Difere |
| `OC/Kexts/SMCBatteryManager.kext/Contents/Resources/SSDT-BATC.dsl` | `bc61dfb383ca` | `e66bbd57e5c6` | CRLF/LF |
| `OC/Kexts/SMCProcessor.kext/Contents/Info.plist` | `d8a6d7074d93` | `797bae2a2cd4` | Difere |
| `OC/Kexts/SMCProcessor.kext/Contents/MacOS/SMCProcessor` | `dcf738671431` | `2f36b24dd014` | Difere |
| `OC/Kexts/USBToolBox.kext/Contents/Info.plist` | `17c7e6f98d52` | `f6095d4d3b02` | CRLF/LF |
| `OC/Kexts/USBToolBox.kext/Contents/MacOS/USBToolBox` | `c5c8dc205a07` | `cfc9847c0a52` | Difere |
| `OC/Kexts/UTBMap.kext/Contents/Info.plist` | `a2e11b06cece` | `517ba9a6e6a0` | Difere |
| `OC/Kexts/VirtualSMC.kext/Contents/Info.plist` | `7466b24490f2` | `2edc12cf9569` | Difere |
| `OC/Kexts/VirtualSMC.kext/Contents/MacOS/VirtualSMC` | `bf91e9542b86` | `2f6ced578785` | Difere |
| `OC/OpenCore.efi` | `11ba0b763d76` | `4894c595476b` | Difere |

</details>

<details>
<summary>Executáveis dos kexts e evidência de compilação, incluindo plug-ins</summary>

Os hashes se referem aos arquivos em `Contents/MacOS`. A coluna do fork usa `=` quando os bytes coincidem com os de Rodrigo. Uma marca `REL-`/`DBG-` ou o caminho Xcode identifica a compilação do próprio alvo. `Pacote` indica que o executável coincidiu byte a byte com o arquivo distribuído na variante indicada. `AMFIPass-Mod` conserva os caminhos de compilação da base Release.

| Bundle ou plug-in | SHA-256 Rodrigo | SHA-256 fork | SHA-256 Live | Evidência em Rodrigo / na Live |
| --- | --- | --- | --- | --- |
| `AirportItlwm` | `259bac9432bf` | = | `259bac9432bf` | Xcode Release / Xcode Release |
| `AirportRTW` | — | — | `c63eee3bfd4a` | — / Pacote Release |
| `AMFIPass-Mod / AMFIPass` | `2d36aeca1740` | = | `4c35bc196d35` | Xcode Release / Xcode Release |
| `AppleALC` | — | — | `e8fe6a1edef1` | — / `DBG-198-2026-09-27` |
| `AudShim` | `0b6fa690252a` | = | — | Indeterminada / — |
| `BlueToolFixup` | `e11fc0299c23` | = | — | `REL-273-2026-03-20` / — |
| `BrightnessKeys` | `f47210181030` | = | — | `REL-104-2024-06-25` / — |
| `CpuTopologyRebuild` | — | — | `c3b6d70148db` | — / `DBG-202-2025-02-11` |
| `CpuTscSync` | — | — | `b099bdd94a94` | — / `DBG-112-2025-07-07` |
| `FeatureUnlock` | — | — | `853d190e5028` | — / `DBG-118-2025-07-07` |
| `G15Fan` | `450043c71117` | `f5bbf9cc9ce6` | — | Indeterminada / — |
| `G15PM` | `b0925fa89047` | `230037c6d79c` | — | `REL-100-2026-11-22` (fork `REL-100-2026-11-17`) / — |
| `IntelBluetoothFirmware` | `397376f72345` | = | — | Xcode Release / — |
| `IntelBTPatcher` | `ddef8d044500` | = | — | `REL-253-2026-11-03` / — |
| `IntelHybridCores` | `e323024dfefe` | = | — | `REL-010-2026-11-15` / — |
| `Lilu` | `e3f00df5aca9` | = | `438b912da905` | `REL-173-2026-03-20` / `DBG-172-2026-03-20` |
| `NVMeFix` | `d32c74322001` | = | `8d44110df6b2` | `REL-114-2025-07-13` / `DBG-113-2025-07-07` |
| `RealtekRTL8111` | `a9a99d105518` | = | `a9a99d105518` | Xcode Release / Xcode Release |
| `RestrictEvents` | `8a244c696234` | = | `6613749e0f8c` | `REL-117-2025-07-13` / `DBG-116-2025-07-07` |
| `SMCBatteryManager` | `791d81018a53` | = | `988e7917c24e` | `REL-139-2026-09-27` / `DBG-138-2026-09-27` |
| `SMCDellSensors` | — | — | `bbf12fa7903d` | — / `DBG-138-2026-09-27` |
| `SMCLightSensor` | — | — | `8ff8b90225f7` | — / `DBG-138-2026-09-27` |
| `SMCProcessor` | `dcf738671431` | = | `2f36b24dd014` | `REL-139-2026-09-27` / `DBG-138-2026-04-09` |
| `SMCSuperIO` | — | — | `c324bdfdea98` | — / `DBG-138-2026-04-09` |
| `USBToolBox` | `c5c8dc205a07` | = | `cfc9847c0a52` | Pacote Release / Pacote Debug |
| `VirtualSMC` | `bf91e9542b86` | = | `2f6ced578785` | `REL-139-2026-09-27` / `DBG-138-2026-04-09` |
| `VoodooI2C` | `d3362d39e5ff` | = | — | Xcode Release / — |
| `VoodooI2C / VoodooGPIO` | `55e9ef293540` | = | — | Xcode Release / — |
| `VoodooI2C / VoodooI2CServices` | `007a10cc4ab8` | = | — | Xcode Release / — |
| `VoodooI2C / VoodooInput` | `113b7876eb13` | = | — | Xcode Release / — |
| `VoodooI2CHID` | `6eeb108e59ca` | = | — | Xcode Release / — |
| `VoodooPS2Controller` | `be6f3aa9dc88` | = | — | Pacote Release / — |
| `VoodooPS2Controller / VoodooInput` | `960a2b41616a` | = | — | Pacote Release / — |
| `VoodooPS2Controller / VoodooPS2Keyboard` | `a1e4903278b2` | = | — | Pacote Release / — |
| `VoodooPS2Controller / VoodooPS2Mouse` | `075be1006783` | = | — | Pacote Release / — |
| `VoodooPS2Controller / VoodooPS2Trackpad` | `e42cd2f68ee4` | = | — | Pacote Release / — |
| `WhateverGreen` | `059967ba94fb` | = | — | `REL-172-2026-09-27` / — |
| `WhateverXe` | `5592eb40675e` | = | — | `DBG-082-2026-11-17` / — |

</details>

<details>
<summary>Campos de configuração com o mesmo valor na Live e em pelo menos um dos outros quatro perfis</summary>

Cada linha agrupa campos da mesma seção e indica em quais perfis seu valor coincide com a Live. `Todos` inclui os dois perfis de Rodrigo e os dois do fork. Listas de componentes foram comparadas por nome e estado nas tabelas acima. Identificadores individuais do SMBIOS e credenciais foram excluídos.

| Seção | Mesmos valores da Live | Perfis correspondentes |
| --- | --- | --- |
| `ACPI` | `Patch=[]` | Rodrigo, ambos |
| `ACPI > Quirks` | `false` em `FadtEnableReset`, `NormalizeHeaders`, `RebaseRegions`, `ResetHwSig`, `SyncTableIds` · `ResetLogoStatus=true` | Todos |
| `Booter > Quirks` | `false` em `AllowRelocationBlock`, `ClearTaskSwitchBit`, `DisableSingleUser`, `DisableVariableWrite`, `DiscardHibernateMap`, `EnableWriteUnprotector`, `ForceBooterSignature`, `ForceExitBootServices`, `ProtectMemoryRegions`, `ProtectSecureBoot`, `SignalAppleOS` · `true` em `AvoidRuntimeDefrag`, `DevirtualiseMmio`, `EnableSafeModeSlide`, `ProtectUefiServices`, `ProvideCustomSlide`, `RebuildAppleMemoryMap`, `SyncRuntimePermissions` · `ProvideMaxSlide=0` | Todos |
| `DeviceProperties` | `Add={}` | Ambos os instaladores |
| `DeviceProperties` | `Delete={}` | Todos |
| `Kernel > Emulate` | `DummyPowerManagement=false` · `""` em `MaxKernel`, `MinKernel` | Todos |
| `Kernel` | `Force=[]` | Todos |
| `Kernel > Quirks` | `false` em `AppleCpuPmCfgLock`, `AppleXcpmExtraMsrs`, `AppleXcpmForceBoost`, `CustomPciSerialDevice`, `DisableIoMapperMapping`, `DisableRtcChecksum`, `ExtendBTFeatureFlags`, `ExternalDiskIcons`, `ForceAquantiaEthernet`, `ForceSecureBootScheme`, `IncreasePciBarSize`, `LapicKernelPanic`, `LegacyCommpage`, `ThirdPartyDrives`, `XhciPortLimit` · `true` em `DisableIoMapper`, `DisableLinkeditJettison`, `PanicNoKextDump`, `PowerTimeoutKernelPanic`, `ProvideCurrentCpuInfo` · `SetApfsTrimTimeout=-1` | Todos |
| `Kernel > Scheme` | `CustomKernel=false` · `FuzzyMatch=true` · `KernelCache=Auto` | Todos |
| `Misc` | `[]` em `BlessOverride`, `Entries` | Todos |
| `Misc > Boot` | `0` em `ConsoleAttributes`, `TakeoffDelay` · `HibernateMode=None` · `false` em `HibernateSkipsPicker`, `PickerAudioAssist` · `InstanceIdentifier=""` · `LauncherOption=Disabled` · `LauncherPath=Default` · `PickerAttributes=17` · `PickerMode=Builtin` · `PickerVariant=Auto` · `true` em `PollAppleHotKeys`, `ShowPicker` | Todos |
| `Misc > Boot` | `HideAuxiliary=false` | Fork · instalação |
| `Misc > Debug` | `true` em `AppleDebug`, `ApplePanic` | Fork · instalação |
| `Misc > Debug` | `DisableWatchDog=true` · `DisplayDelay=0` · `LogModules=*` · `SysReport=false` | Todos |
| `Misc > Security` | `true` em `AllowSetDefault`, `BlacklistAppleUpdate` · `0` em `ApECID`, `ScanPolicy` · `false` em `AuthRestart`, `EnablePassword` · `DmgLoading=Signed` · `HaltLevel=2147483648` · `SecureBootModel=Disabled` · `Vault=Optional` | Todos |
| `Misc > Serial` | `false` em `Init`, `Override` | Todos |
| `NVRAM > Add > 4D1FDA02-38C7-4A6A-9CC6-4BCCA8B30102` | `revcpu=1` · `rtc-blacklist=dados vazios` | Todos |
| `NVRAM > Add > 7C436110-AB2A-4BBB-A880-FE41995C9F82` | `ForceDisplayRotationInEFI=0` · `SystemAudioVolume=0x46` · `run-efi-updater=No` | Todos |
| `NVRAM > Delete` | `4D1EDE05-38C7-4A6A-9CC6-4BCCA8B38C14=DefaultBackgroundColor` | Todos |
| `NVRAM` | `LegacyOverwrite=false` · `WriteFlash=true` | Todos |
| `NVRAM > LegacySchema` | `8BE4DF61-93CA-11D2-AA0D-00E098032B8C=Boot0080, Boot0081, Boot0082, BootNext, BootOrder` | Todos |
| `PlatformInfo` | `true` em `Automatic`, `UpdateDataHub`, `UpdateNVRAM`, `UpdateSMBIOS` · `false` em `CustomMemory`, `UseRawUuidEncoding` | Todos |
| `PlatformInfo > Generic` | `false` em `AdviseFeatures`, `MaxBIOSVersion` · `ProcessorType=3841` · `SpoofVendor=true` · `SystemMemoryStatus=Auto` | Todos |
| `UEFI > APFS` | `true` em `EnableJumpstart`, `HideVerbose` · `false` em `GlobalConnect`, `JumpstartHotPlug` | Todos |
| `UEFI > AppleInput` | `AppleEvent=Builtin` · `CustomDelays=false` · `GraphicsInputMirroring=true` · `KeyInitialDelay=50` · `KeySubsequentDelay=5` · `0` em `PointerDwellClickTimeout`, `PointerDwellDoubleClickTimeout`, `PointerDwellRadius` · `PointerPollMask=-1` · `PointerPollMax=80` · `PointerPollMin=10` · `1` em `PointerSpeedDiv`, `PointerSpeedMul` | Todos |
| `UEFI > Audio` | `0` em `AudioCodec`, `SetupDelay` · `AudioDevice=PciRoot(0x0)/Pci(0x1b,0x0)` · `false` em `DisconnectHda`, `ResetTrafficClass` · `MaximumGain=-15` · `MinimumAssistGain=-30` · `MinimumAudibleGain=-55` · `PlayChime=Auto` | Todos |
| `UEFI` | `ConnectDrivers=true` · `[]` em `ReservedMemory`, `Unload` | Todos |
| `UEFI > Input` | `false` em `KeyFiltering`, `KeySwap`, `PointerSupport` · `KeyForgetThreshold=5` · `KeySupport=true` · `KeySupportMode=Auto` · `PointerSupportMode=ASUS` · `TimerResolution=50000` | Todos |
| `UEFI > Output` | `false` em `ClearScreenOnModeSwitch`, `DirectGopRendering`, `ForceResolution`, `GopBurstMode`, `IgnoreTextInGraphics`, `ReconnectGraphicsOnConnect`, `ReconnectOnResChange`, `ReplaceTabWithSpace`, `SanitiseClearScreen`, `UgaPassThrough` · `""` em `ConsoleFont`, `ConsoleMode` · `GopPassThrough=Disabled` · `InitialMode=Auto` · `ProvideConsoleGop=true` · `Resolution=Max` · `TextRenderer=BuiltinGraphics` · `UIScale=0` | Todos |
| `UEFI > ProtocolOverrides` | `false` em `AppleAudio`, `AppleBootPolicy`, `AppleDebugLog`, `AppleEg2Info`, `AppleFramebufferInfo`, `AppleImageConversion`, `AppleImg4Verification`, `AppleKeyMap`, `AppleRtcRam`, `AppleSecureBoot`, `AppleSmcIo`, `AppleUserInterfaceTheme`, `DataHub`, `DeviceProperties`, `HashServices`, `OSInfo`, `PciIo`, `UnicodeCollation` · `FirmwareVolume=true` | Todos |
| `UEFI > Quirks` | `false` em `ActivateHpetSupport`, `DisableSecurityPolicy`, `EnableVmx`, `ForceOcWriteFlash`, `ForgeUefiSupport`, `IgnoreInvalidFlexRatio`, `ReloadOptionRoms`, `ResizeUsePciRbIo`, `ShimRetainProtocol`, `UnblockFsConnect` · `0` em `ExitBootServicesDelay`, `TscSyncTimeout` · `RequestBootVarRouting=true` · `ResizeGpuBars=-1` | Todos |

Nas listas `NVRAM > Delete`, os perfis compartilham `DefaultBackgroundColor` no GUID de interface, `rtc-blacklist`, `revcpu`, `revcpuname` e `revpatch` no GUID do OpenCore e `boot-args`, `ForceDisplayRotationInEFI` e `csr-active-config` no GUID da Apple. As 14 variáveis do GUID Apple em `LegacySchema` da Live também constam em Rodrigo e no fork, que incluem ainda `backlight-level` e `BootCampHD`.

</details>
