# Roteiro do Hackintosh no Dell G15 5530

**Alvo:** macOS Tahoe para uso diário, com boot pelo NVMe dedicado, Windows no
seu próprio SSD, tela interna acelerada a 120 Hz, rede, áudio, teclado,
touchpad, USB e repouso validados no Dell. A RTX 3050 e o HDMI têm uma etapa
própria de ativação e teste.

**Estado em 21/11/2026:** o pendrive já abriu a Recuperação gráfica, o Wi-Fi
funcionou e o instalador preparou o NVMe para a etapa seguinte. Falta chegar
ao Assistente de Configuração e ao desktop. O perfil de sistema também
precisa de uma validação própria antes de ir para a EFI do SSD.

## As três máquinas

| | Nosso Dell | Dell de Rodrigo Rangel | ASUS da live |
| --- | --- | --- | --- |
| CPU | i7-13650HX, 6P + 8E, 20 threads | i5-13450HX, 6P + 4E | Core 5 210H |
| Vídeo | UHD `8086:A78B` com tela de 120 Hz em Hybrid/Optimus. RTX 3050 6 GB Laptop `10DE:25EC` | UHD A78B e painel FHD 120 Hz. RTX 4050 Laptop para Metal e HDMI | RTX 3050 6 GB Laptop ligada ao painel pelo MUX em Ultimate. NullMoth após a instalação |
| ACPI | BIOS 1.37.0, DSDT nativa e ajuste RP25 para o instalador | BIOS 1.0, DSDT modificada para brilho e repouso | SSDTs geradas a partir do dump do próprio ASUS |
| Disco | Windows no Micron 2550. Tahoe no segundo NVMe de 512 GB | Samsung 970 EVO para macOS, outro NVMe para Fedora | Redimensionou a partição do Windows e instalou o Tahoe em uma partição do mesmo NVMe |
| Perfil Mac | `MacBookPro16,1` | `MacBookPro16,1`, Tahoe 26.7.3 (25G342) | `MacBookPro16,2`, Tahoe 26.7.3 ao final |

CPU, PCI IDs das GPUs e BIOS foram conferidos no Windows. O identificador A78B
é o mesmo que Rodrigo Rangel usou no Dell. O relato de outro i7-13650HX sem Metal
no WhateverXe envolvia um HP OMEN com iGPU A788, outro painel e outro par de
versões do driver. O primeiro teste da UHD no nosso Dell ainda é decisivo.
[Hardware e gráficos de Rodrigo Rangel](../README.md#what-works) ·
[Relato do HP OMEN](https://github.com/b00t0x/WhateverXe/issues/51)

## Etapas e critérios de conclusão

### 1. Firmware, inventário e pendrive

- BIOS **1.37.0**, UEFI, **AHCI/NVMe sem RAID/VMD** e **Hybrid/Optimus** são a
  base do Dell. Para inicializar a EFI atual, usar Secure Boot UEFI desligado. O Windows
  segue disponível diretamente pelo Windows Boot Manager no F12. A ausência
  de BitLocker já foi conferida neste Windows. O ajuste de Hyper-Threading
  feito no ASUS pertence ao teste do Core 5 da live.
- O [registro ACPI](../acpi-native/README.md) reúne os dumps das duas BIOS e os
  manifests com hashes. Nos dois dumps da 1.37.0, só o endereço da imagem de
  boot na BGRT e seu checksum variaram. O SSDTTime local foi conferido com
  `iasl.exe` e `acpidump.exe` 20251212. O APIC tem 20 CPUs lógicas habilitadas,
  com IDs 0 a 19 cobertos pela PLUG-ALT.
- Pendrive `TAHOE` em GPT/FAT32, com OpenCore **DEBUG 1.0.8** e o DMG de
  Recuperação Tahoe 26.6.2 (25G83) e sua chunklist da Apple em
  `com.apple.recovery.boot`. O instalador usa `OpenHfsPlus.efi` para ler o
  HFS+ interno do DMG. O disco de destino usa APFS.
- No repositório, `EFI/OC/config-install.plist` é a base do instalador e
  `EFI/OC/config.plist` é o perfil de sistema. No pendrive o OpenCore lê
  `EFI/OC/config.plist`, hoje com o perfil do instalador. A cópia chamada
  `config-system.plist` guarda o perfil de sistema para a troca posterior.
  Cada cópia local usa o mesmo SMBIOS gerado pelo
  [GenSMBIOS](../tools/GenSMBIOS/README.md), com os quatro identificadores
  privados fora do Git.
- **Em paralelo no Windows:** mapear USB 2 e USB 3 em cada porta externa
  usada, incluindo USB-C, e identificar Bluetooth e webcam internos. O
  instalador já chegou à interface com `USBToolBox + UTBDefault` na porta
  lógica `XHCI.RHUB.SS03`. O mapa próprio `UTBMap.kext` deve substituir
  `UTBDefault.kext` após teste, mantendo `USBToolBox.kext`.
  [Fluxo do USBToolBox](https://github.com/USBToolBox/kext#usage)

**Concluída para instalação:** F12 abre o OpenCore, o volume
`TAHOE (external) (dmg)` abre a interface, o NVMe é visível e existe
conexão com a Apple. A interface e o Wi-Fi já funcionaram uma vez. Mapa
USB definitivo e repetibilidade seguem pendentes.

### 2. Instalação até o primeiro desktop

- O NVMe dedicado já está em GPT com uma ESP FAT32 de 200 MiB e duas partições
  APFS de cerca de 256 GB. A primeira contém a instalação preparada. Durante
  os reinícios, entrar pelo mesmo OpenCore USB e escolher
  **`macOS Installer`** enquanto a entrada estiver presente. Em seguida,
  escolher o volume do sistema. O perfil de instalação usa WhateverGreen,
  `-wegnoegpu` e I2C em polling. Essa etapa funciona com vídeo básico.
- O primeiro reinício já ocorreu. O log confirmou a carga de
  `com.apple.installer/boot.efi` no NVMe, e a tela parou na configuração PCI.
  Mais tarde, a Recuperação USB também parou durante a inicialização. A
  opção `EnableJumpstart=false` no pendrive é o teste em aberto para
  a Recuperação USB. Para o OpenCore carregar novamente a etapa APFS do
  NVMe, restaurar `EnableJumpstart=true`. O último log disponível,
  `opencore-2026-11-21-123517.txt`, é anterior a esse teste. O relógio variou
  entre os boots. Usar também o conteúdo do log para ordenar as tentativas.
- O [registro ACPI](../acpi-native/README.md#patches-do-ssdttime-avaliados-na-bios-1370)
  descreve FakeEC, USBX, PluginType, RTCAWAC, PNLF, XOSI, HPET, DMAR, SMBus e
  ALS0. FixHPET foi gerado e validado em simulação com RP25.
  `SetupVirtualMap` está em `false` no Dell e é outro candidato que a live
  motivou para teste isolado. No ASUS ele resolveu uma falha anterior ao
  ponto mostrado no Dell. `ResizeAppleGpuBars` está em `-1` no Dell. O
  valor `0` da live foi escolhido para o ReBAR do ASUS.
  [Manual do OpenCore 1.0.8](https://github.com/acidanthera/OpenCorePkg/blob/1.0.8/Docs/Configuration.tex#L1675-L1733)

**Concluída quando:** o Assistente de Configuração termina, surge o desktop e
o volume instalado volta a iniciar pelo USB. A porcentagem e os minutos
estimados são indicadores de progresso. O instalador pode reiniciar várias
vezes. [Fluxo de instalação da Apple](https://support.apple.com/pt-br/102655)

### 3. Perfil de sistema e boot independente pelo SSD

1. Levar para o perfil de sistema os ajustes comprovados no Dell. O
   `config.plist` do repositório e o `config-system.plist` do pendrive
   ainda carecem do patch RP25. Em uma cópia temporária com RP25, o
   `ocvalidate 1.0.8` passou e o AcpiExec 20160930 carregou 24 tabelas e
   resolveu, entre outros, RP25, PNLF e os métodos GOFF e GONN.
2. Validar o perfil de sistema pelo USB com o mesmo SMBIOS e o sistema
   instalado. Conferir primeiro tela interna, entrada e boot repetível.
   Os ajustes de repouso S0, GPIO da tampa/touchpad e corte de energia da
   RTX têm testes próprios antes de entrar no uso diário.
3. Identificar a ESP de 200 MiB do NVMe do macOS e copiar a EFI de sistema
   validada. Iniciar pelo F12 sem o pendrive. Testar também o Windows
   diretamente no Windows Boot Manager. Guardar a EFI USB que funcionou
   como meio de recuperação.

**Concluída quando:** o Dell inicia Tahoe pelo SSD com o pendrive removido
e inicia Windows pelo SSD dele. O USB ainda permite recuperar o Tahoe.

### 4. Aceleração, periféricos e energia

| Função | Sequência e teste no Dell |
| --- | --- |
| **UHD e tela interna** | Validar o fork WhateverXe de Rodrigo Rangel junto com o pacote Tahoe WhateverXe Graphics **0.4.4**. O perfil de sistema carrega WhateverXe e o de instalação carrega WhateverGreen. Após instalar o pacote e reiniciar, conferir Metal, 120 Hz, brilho, reprodução de vídeo e retorno da tela após apagar. O painel interno é a primeira prioridade gráfica. [Pacote 0.4.4](https://github.com/b00t0x/WhateverXe-Graphics/releases/tag/v0.4.4) |
| **Patches pós-instalação** | Conferir a versão/build com `sw_vers` e o relatório `install-macos.sh check`. O script de Rodrigo Rangel trata os patches de vídeo VA e o shim Metal stencil, instala XeSpeakers e G15.app e configura `pmset`. Conferir o resultado de cada etapa, pois o script continua em alguns casos de falha. Os patches VA fazem checagens de bytes e guardam cópias em `/Library/G15-uhd`. Revalidar VA e stencil após reinstalar o pacote gráfico. [Pós-instalação de Rodrigo Rangel](../README.md#post-install-in-macos) |
| **Áudio e entrada** | Testar alto-falantes, microfone e headset com AudShim + XeSpeakers. Validar teclado e teclas Fn com VoodooPS2 e G15.app. Para o touchpad, partir do VoodooI2C/I2CHID de Rodrigo Rangel, conferir o dispositivo real e testar gestos, polling e, em seguida, interrupções GPIO e wake. |
| **USB e rede** | Validar o mapa próprio em todas as portas testadas, inclusive após repouso. Testar Ethernet Realtek, Wi-Fi Intel, Bluetooth Intel e reconexão. A Recuperação já conectou por Wi-Fi. |
| **RTX 3050 e HDMI** | Com a UHD estável, testar NullMoth separadamente. Rodrigo Rangel usa **3.0** e a RTX 4050 junto à UHD. A live demonstrou a RTX 3050 em um ASUS com o MUX na dedicada e registrou instabilidade ao trocar versões. No Dell, confirmar Metal na RTX, compatibilidade com o painel interno via UHD, HDMI com cabo ligado no boot e após hotplug e estabilidade em uso real. [Projeto NullMoth](https://github.com/nullmoth/nvidia-macos-driver) · [HDMI no Dell de Rodrigo Rangel](../README.md#what-partly-works) |
| **Repouso e bateria** | Só depois de testar as duas GPUs, validar S0/Deep Idle, tampa, retorno do painel, retomada de áudio/rede, G15Fan, G15PM, política de E-cores e consumo na bateria. Testar o corte de energia da RTX e os ajustes de idle separadamente. Rodrigo Rangel usou GPIO `GPP_E3` para wake do touchpad. O caminho físico precisa ser conferido no nosso Dell. |

O perfil de instalação usa `csr-active-config=130A0000`. O perfil de
sistema mantém `430A0000` e o relaxamento da AMFI previsto por Rodrigo Rangel
para os componentes gráficos. Vincular qualquer ajuste adicional à
versão dos drivers que estamos testando e manter o mesmo SMBIOS.

**Pronto para uso diário quando:** Tahoe e Windows iniciam separadamente,
a UHD tem Metal a 120 Hz, os periféricos usados funcionam e o Dell passa
por ciclos de repouso e retomada em bateria e na tomada. RTX, HDMI e
recursos opcionais têm resultados próprios. FileVault, Sidecar, AirDrop,
Handoff e áudio via HDMI também estão sem validação no upstream.

## Sequência demonstrada na live

Os horários abaixo são do transcript **Dell_G15_5530_Hackintosh_Live_transcript.txt**.
O equipamento demonstrado é o ASUS TUF Gaming F16.

| Trecho | O que foi feito |
| --- | --- |
| 00:23–00:28 | Conferiu Secure Boot, testou a topologia do Core 5 com Hyper-Threading desligado e desativou o VMD para o NVMe aparecer. |
| 00:44–00:46 | Mudou o MUX pelo Armoury Crate para Ultimate, ligando o painel à RTX. |
| 01:11–01:21 | Conferiu iASL 20251212, fez o dump, gerou as SSDTs e a DMAR ajustada. Copiou as dez tabelas e mesclou as entradas `ACPI > Add`, `Patch` e `Delete` com PatchMerge. |
| 01:23–01:51 | Gerou o SMBIOS 16,2, mapeou USB no Windows, criou `UTBMap.kext`, ajustou `ResizeAppleGpuBars=0` conforme o ReBAR do ASUS, ajustou SIP/boot-args para a RTX e atualizou o snapshot do config. |
| 01:53–02:09 | Preparou FAT32 e Recuperação online, separou espaço no NVMe interno, fez Reset NVRAM e corrigiu o primeiro boot com `SetupVirtualMap=true`. |
| 02:09–02:46 | Formatou a partição para APFS, instalou com internet e passou pelos reinícios e pela configuração inicial com vídeo básico. |
| 02:49–03:36 | Preparou os ajustes de EFI do NullMoth, instalou o driver, experimentou 2.9 e 3.0 e mostrou a RTX com aceleração e HDMI. A troca de versão apresentou instabilidade. |
| 03:25–03:50 | Voltou a áudio e touchpad, ajustou os kexts Voodoo e o som de boot e mostrou as entradas separadas do OpenCore e do Windows Boot Manager. |

A [sequência original de Rodrigo Rangel](../README.md#install-from-scratch)
continua como referência para esse chassi, especialmente a transição para
WhateverXe, a validação da UHD com o painel interno e as rotinas de S0.
