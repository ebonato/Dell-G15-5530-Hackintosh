# Roteiro do Hackintosh no Dell G15 5530

**Alvo:** macOS Tahoe para uso diário, com boot pelo NVMe dedicado, Windows no
seu próprio SSD, tela interna acelerada a 120 Hz, rede, áudio, teclado,
touchpad, USB e repouso validados no Dell. A RTX 3050 e o HDMI têm uma etapa
própria de ativação e teste.

**Teste de 22/11/2026.** A `EFI-Live-Debug` chegou à interface da
Recuperação no Dell. O log `opencore-2026-11-22-102151.txt` registra o
OpenCore `DBG-108-2026-09-23`, a substituição da DSDT de 653.360 bytes e
`EXITBS:START`. A chegada à interface foi confirmada na tela.

O pendrive `TAHOE` está com a `EFI-Live-Release` para repetir esse boot.
As duas variantes usam o mesmo SMBIOS, as mesmas 11 tabelas ACPI e o
mesmo mapa USB. A Release usa o menu gráfico OpenCanopy e mantém `-v`.
O teste termina ao abrir a Recuperação do pendrive, identificada no menu
de texto como `TAHOE (external) (dmg)`. [Perfil Release](../EFI-Live-Release/OC/config.plist)

## As três máquinas

| | Nosso Dell | Dell de Rodrigo Rangel | ASUS da live |
| --- | --- | --- | --- |
| CPU | i7-13650HX, 6P + 8E, 20 threads | i5-13450HX, 6P + 4E | Core 5 210H |
| Vídeo | UHD `8086:A78B` com tela de 120 Hz em Hybrid/Optimus. RTX 3050 6 GB Laptop `10DE:25EC` | UHD A78B e painel FHD 120 Hz. RTX 4050 Laptop para Metal e HDMI | RTX 3050 6 GB Laptop ligada ao painel pelo MUX em Ultimate. NullMoth após a instalação |
| ACPI | BIOS 1.37.0, DSDT local com RP25 incorporado nos dois perfis Live | BIOS 1.0, DSDT modificada para brilho e repouso | SSDTs geradas a partir do dump do próprio ASUS |
| Disco | Windows no Micron 2550. Tahoe no segundo NVMe de 512 GB | Samsung 970 EVO para macOS, outro NVMe para Fedora | Redimensionou a partição do Windows e instalou o Tahoe em uma partição do mesmo NVMe |
| Perfil Mac | `MacBookPro16,2` nas EFIs Live | `MacBookPro16,1`, Tahoe 26.7.3 (25G342) | `MacBookPro16,2`, Tahoe 26.7.3 ao final |

CPU, PCI IDs das GPUs e BIOS foram conferidos no Windows. O identificador A78B
é o mesmo que Rodrigo Rangel usou no Dell. O relato de outro i7-13650HX sem Metal
no WhateverXe envolvia um HP OMEN com iGPU A788, outro painel e outro par de
versões do driver. O primeiro teste da UHD no nosso Dell ainda é decisivo.
[Hardware e gráficos de Rodrigo Rangel](../README.md#what-works) ·
[Relato do HP OMEN](https://github.com/b00t0x/WhateverXe/issues/51)

### Escolha do SMBIOS

A Apple lista Tahoe 26 para os modelos `MacBookPro16,1` (16 polegadas, 2019)
e `MacBookPro16,2` (13 polegadas, 2020, quatro portas Thunderbolt 3). O
`16,1` já abriu a Recuperação no nosso Dell e é a chave de máquina usada pelo
perfil de energia da UHD no [G15AGPMIgpu](../EFI/OC/Kexts/G15AGPMIgpu.kext/Contents/Info.plist).
O `16,2` abriu a Recuperação com a Live Debug e também identifica a Live
Release. O efeito desse perfil sobre Metal, brilho e consumo será avaliado
depois da instalação.
[Modelos de MacBook Pro](https://support.apple.com/en-us/108052)

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
- Pendrive `TAHOE` em GPT/FAT32, com OpenCore Release 1.0.8 e os arquivos
  da Recuperação Tahoe 26.6.2 (25G83) em `com.apple.recovery.boot`. A
  EFI Live usa `HfsPlus.efi` para ler o HFS+ da imagem. O destino do
  macOS no NVMe dedicado usa APFS.
- O `EFI/OC/config.plist` ativo no pendrive é a cópia de
  `EFI-Live-Release/OC/config.plist`. Os perfis derivados de Rodrigo
  ficam em `EFI/OC` no repositório. [Comparação dos perfis](COMPARACAO-EFI-LIVE-DEBUG.md)
- O USBToolBox 0.2 e o inventário PnP do Windows confirmaram um único
  controlador PCI, Intel xHCI `8086:7A60` em `00:14.0`. O mapa de
  instalação foi gerado com dez portas observadas. As três USB-A foram
  confirmadas com o mouse em USB 2.0 e com armazenamento em USB 3.0.

  | Conector | Índice USB 2.0 | Índice USB 3.0 |
  | --- | ---: | ---: |
  | USB-A traseira | 8 | 17 |
  | USB-A direita, usada pelo pendrive de Recuperação | 9 | 19 |
  | USB-A direita, ao lado do pendrive | 10 | 20 |
  | USB-C traseiro, hub com teclado | 11 | 23 |
  | Webcam interna | 3 | — |
  | Bluetooth Intel interno | 14 | — |

  O perfil de instalação carrega `USBToolBox.kext` com
  `UTBMap.kext`. No USB-C traseiro, foram observados os dois lados do hub
  na orientação atual e o teclado Corsair no lado USB 2.0. O firmware
  declara USB-C tipo 9. A outra orientação segue pendente.
  [Fluxo do USBToolBox](https://github.com/USBToolBox/kext#usage)

**Concluída para instalação:** F12 abre o OpenCore, o volume
`TAHOE (external) (dmg)` abre a interface, o NVMe é visível e existe
conexão com a Apple. A interface e o Wi-Fi já funcionaram uma vez. A
repetibilidade do boot e o teste do hub no macOS seguem pendentes.

### Roteamento de vídeo observado no Windows

| Tela | Saída vista pelo Windows | GPU que dirige a tela |
| --- | --- | --- |
| Painel interno | eDP | NVIDIA RTX 3050 `10DE:25EC` na sessão observada |
| Samsung Odyssey G5 | HDMI direto do notebook | NVIDIA RTX 3050 `10DE:25EC` |
| 27G2E1 | DisplayPort pelo USB-C traseiro, convertido para HDMI pelo hub | Intel UHD `8086:A78B` |

Os caminhos são da sessão do Windows. Conferir a seleção de Hybrid/Optimus
e observar também o monitor pelo USB-C no teste de Recuperação. O hub USB
transporta o teclado separadamente do vídeo DisplayPort.
[Tipos de saída do Windows](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ne-wingdi-displayconfig_video_output_technology)

### 2. Instalação até o primeiro desktop

- Na tentativa anterior, o NVMe de 512 GB tinha uma ESP de 200 MiB e duas
  partições APFS de cerca de 256 GB. O disco foi limpo depois daquele
  teste. Para a próxima instalação, identificar o NVMe dedicado na
  Recuperação. Nos reinícios, usar o mesmo OpenCore USB e selecionar
  **`macOS Installer`** enquanto essa entrada estiver presente. Depois,
  selecionar o volume do sistema.

**Histórico do perfil `EFI/OC/config-install.plist`**

- No primeiro reinício daquela instalação, o log confirmou a carga de
  `com.apple.installer/boot.efi` no NVMe, e a tela parou na configuração PCI.
  Mais tarde, a Recuperação USB também parou durante a inicialização. A
  tentativa com `EnableJumpstart=false` usou a Recuperação USB e registrou
  `EXITBS:START` no log `opencore-2026-11-21-164308.txt`. A foto mostra
  o kernel parando durante a listagem das tabelas ACPI mesmo sem carregar
  o driver APFS do SSD. O log `195200` corresponde ao Windows. O perfil de instalação daquela
  tentativa voltou a usar `EnableJumpstart=true` para poder
  continuar pela etapa APFS após o primeiro reinício. O `ocvalidate 1.0.8`
  aceitou o perfil ativo e o DMG e a chunklist mantêm os hashes originais.
- O [registro ACPI](../acpi-native/README.md#patches-do-ssdttime-avaliados-na-bios-1370)
  descreve FakeEC, USBX, PluginType, RTCAWAC, PNLF, XOSI, HPET, DMAR, SMBus e
  ALS0. O instalador reconstruído usa FakeEC Laptop, USBX, RTCAWAC,
  HPET e SMBus gerados a partir do dump local, além de PluginType e RP25
  verificados contra a BIOS 1.37.0. A simulação carregou as 21 tabelas.
  Nesse perfil, `SetupVirtualMap=false` e `ResizeAppleGpuBars=-1`. As
  EFIs Live que estamos testando usam `true` e `0`, respectivamente.
  [Manual do OpenCore 1.0.8](https://github.com/acidanthera/OpenCorePkg/blob/1.0.8/Docs/Configuration.tex#L1675-L1733)

**Concluída quando:** o Assistente de Configuração termina, surge o desktop e
o volume instalado volta a iniciar pelo USB. A porcentagem e os minutos
estimados são indicadores de progresso. O instalador pode reiniciar várias
vezes. Antes de instalar, conferir entrada, USB, rede e a identificação
do NVMe dedicado na Recuperação. O Windows está no Micron 2550, com EFI própria.
[Fluxo de instalação da Apple](https://support.apple.com/pt-br/102655)

### 3. Perfil de sistema e boot independente pelo SSD

1. Usar como referência a EFI Live que chegar ao login do sistema
   instalado. Confirmar um novo boot pelo pendrive com o mesmo SMBIOS.
2. Identificar a partição EFI do NVMe dedicado ao macOS e copiar a EFI
   validada. Iniciar pelo F12 sem o pendrive e testar o Windows pelo
   Windows Boot Manager. A EFI do pendrive serve para recuperação.

**Concluída quando:** o Dell inicia Tahoe pelo SSD com o pendrive removido
e inicia Windows pelo SSD dele. O USB ainda permite recuperar o Tahoe.

### 4. Aceleração, periféricos e energia

Depois do primeiro login, aproveitar as otimizações de Rodrigo Rangel uma
por vez sobre a EFI Live validada. Testar cada função com as dependências
que ela exige. Para vídeo, brilho e repouso, conferir a DSDT da BIOS
1.37.0 e a RTX 3050 do nosso Dell antes de portar ajustes do equipamento dele.

| Função | Sequência e teste no Dell |
| --- | --- |
| **UHD e tela interna** | Validar o fork WhateverXe de Rodrigo Rangel junto com o pacote Tahoe WhateverXe Graphics **0.4.4**. Na EFI de Rodrigo, o perfil de sistema carrega WhateverXe e o de instalação carrega WhateverGreen. Após instalar o pacote e reiniciar, conferir Metal, 120 Hz, brilho, reprodução de vídeo e retorno da tela após apagar. O painel interno é a primeira prioridade gráfica. [Pacote 0.4.4](https://github.com/b00t0x/WhateverXe-Graphics/releases/tag/v0.4.4) |
| **Patches pós-instalação** | Conferir a versão/build com `sw_vers` e o relatório `install-macos.sh check`. O script de Rodrigo Rangel trata os patches de vídeo VA e o shim Metal stencil, instala XeSpeakers e G15.app e configura `pmset`. Conferir o resultado de cada etapa, pois o script continua em alguns casos de falha. Os patches VA fazem checagens de bytes e guardam cópias em `/Library/G15-uhd`. Revalidar VA e stencil após reinstalar o pacote gráfico. [Pós-instalação de Rodrigo Rangel](../README.md#post-install-in-macos) |
| **Áudio e entrada** | Testar alto-falantes, microfone e headset com AudShim + XeSpeakers. Validar teclado e teclas Fn com VoodooPS2 e G15.app. Para o touchpad, partir do VoodooI2C/I2CHID de Rodrigo Rangel, conferir o dispositivo real e testar gestos, polling e, em seguida, interrupções GPIO e wake. |
| **USB e rede** | Validar o mapa próprio em todas as portas testadas, inclusive após repouso. Testar Ethernet Realtek, Wi-Fi Intel, Bluetooth Intel e reconexão. A Recuperação já conectou por Wi-Fi. |
| **RTX 3050 e HDMI** | Com a UHD estável, testar NullMoth separadamente. Rodrigo Rangel usa **3.0** e a RTX 4050 junto à UHD. A live demonstrou a RTX 3050 em um ASUS com o MUX na dedicada e registrou instabilidade ao trocar versões. No Dell, confirmar Metal na RTX, compatibilidade com o painel interno via UHD, HDMI com cabo ligado no boot e após hotplug e estabilidade em uso real. [Projeto NullMoth](https://github.com/nullmoth/nvidia-macos-driver) · [HDMI no Dell de Rodrigo Rangel](../README.md#what-partly-works) |
| **Repouso e bateria** | Só depois de testar as duas GPUs, validar S0/Deep Idle, tampa, retorno do painel, retomada de áudio/rede, G15Fan, G15PM, política de E-cores e consumo na bateria. Testar o corte de energia da RTX e os ajustes de idle separadamente. Rodrigo Rangel usou GPIO `GPP_E3` para wake do touchpad. O caminho físico precisa ser conferido no nosso Dell. |

Os perfis derivados de Rodrigo em `EFI/OC` usam `csr-active-config=130A0000`
na instalação e `430A0000` no sistema. Os dois perfis Live usam `03080000`. Vincular qualquer ajuste adicional à
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
