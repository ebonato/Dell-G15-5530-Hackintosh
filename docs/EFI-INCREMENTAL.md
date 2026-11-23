# EFI-Incremental

## Teste 01 — Intel UHD A78B com WhateverXe oficial

A base é a EFI-Live-Release que iniciou o Tahoe 26.7.3 no volume G15.
O commit `2fd3bb0` criou `EFI-Incremental` a partir dessa base, com o
primeiro perfil de teste descrito abaixo.

### Versão em teste

A tag oficial 0.8.3 inclui correções experimentais de eDP na inicialização
e após o repouso da tela, incluindo mudanças no handoff e na retomada do
Display13. A branch `g15-deep-idle` de Rodrigo Rangel incorporou essa tag
no commit `bf35ea5`. O documento de Deep Idle ainda descreve os testes que
ele fez com a base 0.8.0. Entre as mudanças próprias do fork estão a
realocação dos ponteiros de estado da GPU ao acordar, o retorno da luz do
painel por PWM e a retomada do canal AUX no Dell. Os relatos públicos
sobre o painel e o repouso estão nos issues #31 e #80. A tag oficial
foi publicada pelo PR #160. [Release 0.8.3](https://github.com/b00t0x/WhateverXe/releases/tag/0.8.3) ·
[Merge de 0.8.3 no fork](https://github.com/rodgod-rr/WhateverXe/commit/bf35ea5b) ·
[Deep Idle no Dell](https://github.com/rodgod-rr/WhateverXe/blob/g15-deep-idle/docs/g15-deep-idle/README.md) ·
[Issue #31](https://github.com/b00t0x/WhateverXe/issues/31) ·
[Issue #80](https://github.com/b00t0x/WhateverXe/issues/80) ·
[PR #160](https://github.com/b00t0x/WhateverXe/pull/160)

A primeira rodada usa o WhateverXe oficial 0.8.3 Release com o pacote
Graphics 0.4.4 para separar o teste de Metal e do painel dos ajustes de
repouso e dos demais patches específicos do fork.

### Mudanças em relação à Live Release

| Onde | Mudança | Motivo |
| --- | --- | --- |
| `OC/Kexts` e `Kernel > Add` | Adicionados `AMFIAnchor.kext` 0.1.2 Release, `AMFIAnchorInjector.kext` da mesma tag e `WhateverXe.kext` 0.8.3 Release, nessa ordem após Lilu. As três entradas são x86_64 e limitadas ao Darwin 25.x do Tahoe. | Lilu carrega os complementos. O injector fornece a âncora do certificado usado pelo projeto Graphics. O WhateverXe adapta o driver à UHD A78B. |
| `Kernel > Add` | `AMFIPass.kext` passou para desabilitado. | A configuração oficial do WhateverXe 0.8.3 usa AMFIAnchor e seu injector, com AMFI ativa. |
| `Kernel > Add` | `AirportRTW.kext` passou para desabilitado. | É o driver Wi-Fi de placas Realtek RTW88/89 e sua documentação usa AMFIPass. O Dell tem a Intel `8086:7A70`. Para essa placa, o perfil usa `AirportItlwm.kext`. |
| `NVRAM > Add > boot-args` | Acrescentados `-allow3d` e `-disablegfxfirmware`. | O primeiro habilita o caminho de renderização 3D. O segundo evita o caminho de carregamento de firmware gráfico do driver Apple, associado ao GuC, e seleciona o caminho suportado pelo WhateverXe. Aplica-se na inicialização do macOS. |
| `NVRAM > Add > boot-args` | Removidos `amfi_get_out_of_my_way=0x1`, `amfi=0x80` e `ipc_control_port_options=0`. | Retira os relaxamentos usados pelo perfil anterior. A assinatura dos componentes Graphics é tratada pela nova cadeia de confiança. |
| Cabeçalho do plist | Identificação textual trocada para `Dell G15 5530 - Intel UHD Graphics test`. | Facilita reconhecer o perfil ao abrir o arquivo. |

No primeiro perfil, a diferença funcional do plist se limita ao `Kernel > Add` e a `boot-args`.
O teste usa o SIP `03080000`, a DSDT da BIOS 1.37.0, o mapa USB e o
SMBIOS da Release. O `ocvalidate` 1.0.8 aceitou o perfil, com 20 kexts
habilitados. [Instalação do WEX 0.8.3](https://github.com/b00t0x/WhateverXe/tree/0.8.3#installation) ·
[AMFIAnchor 0.1.2](https://github.com/b00t0x/AMFIAnchor/tree/0.1.2#install-with-opencore) ·
[AirportRTW 2.3.3](https://github.com/souissiak1-maker/AirportRTW/blob/V2.3.3/README.md#how-to-use)

### Como testar

No F12, selecione a entrada normal do pendrive TAHOE e inicie `G15`.
No desktop, abra `G15-Intel/WhateverXe-Graphics-0.4.4.pkg` no pendrive.
Depois de concluída a instalação, reinicie pela mesma entrada. Em
`Informações do Sistema > Gráficos/Monitores`, confira se a Intel UHD
aparece com Metal. Teste também se o painel interno segue aceso. A saída
USB-C traseira era dirigida pela Intel na sessão Windows observada.
[Pacote Graphics 0.4.4](https://github.com/b00t0x/WhateverXe-Graphics/releases/tag/v0.4.4)

Para voltar à configuração de vídeo básico pelo Windows, a cópia do
plist da Live Release está em `G15-Intel/config-live-release.plist` no
pendrive. Ela corresponde ao perfil `EFI-Live-Release/OC/config.plist` do
repositório. O carregador do pendrive lê `EFI/OC/config.plist`.

### Erro de leitura do plist no primeiro reinício

Após instalar o Graphics 0.4.4 e reiniciar, o OpenCore parou ao ler
`EFI/OC/config.plist`. A cópia ativa do pendrive tinha 11 bytes alterados,
incluindo três bytes NUL. A leitura do XML falhou na linha 631.

O plist ativo foi restaurado a partir deste perfil incremental. O
`ocvalidate` 1.0.8 aceitou o arquivo no pendrive, e os hashes dos 176
arquivos da EFI coincidiram com os do repositório. A causa da corrupção
segue indeterminada. Os scripts do pacote para o Tahoe foram conferidos.
Eles operam no macOS, incluindo a reconstrução da AuxKC em `/Library`.
[Repositório do Graphics](https://github.com/b00t0x/WhateverXe-Graphics)

### Segundo teste com o ajuste de DVMT

O primeiro boot até o desktop após a restauração do plist continuou com
vídeo básico. O pacote Graphics 0.4.4 foi reinstalado. No Windows, o
plist do TAHOE ainda correspondia ao incremental por SHA-256. Os logs
mais recentes disponíveis no pendrive continham apenas zeros.

A etapa `Preboot` citada pelo Instalador se refere à inicialização APFS
do macOS. O script do pacote para Tahoe gera a coleção auxiliar de kexts
em `/Library/KernelCollections` e registra uma preferência do CoreMedia
em `/Library/Preferences`. Ele não escreve no plist do OpenCore nem na
EFI do Windows. [Código do Graphics 0.4.4](https://github.com/b00t0x/WhateverXe-Graphics/tree/v0.4.4)

O novo perfil acrescenta `-igfxdvmt` a `boot-args` e as propriedades Data
`framebuffer-patch-enable=01000000` e `framebuffer-stolenmem=0000C003` na
Intel em `PciRoot(0x0)/Pci(0x2,0x0)`. O WhateverXe documenta esses valores
para 60 MB de DVMT pré-alocado. Rodrigo Rangel relatou essa configuração
no G15 5530 dele, com a mesma UHD A78B. A reserva de firmware deste Dell
ainda precisa ser medida. [Configuração DVMT do WhateverXe 0.8.3](https://github.com/b00t0x/WhateverXe/tree/0.8.3#dvmt) ·
[Relato de Rodrigo Rangel](https://github.com/b00t0x/WhateverXe/issues/51)

O `ocvalidate` 1.0.8 aceitou o plist no repositório e no pendrive. Os 176
arquivos da EFI ativa do TAHOE conferiram por SHA-256. A cópia do perfil
anterior continua em `G15-Intel/EFI/OC/config.plist` no pendrive.

Para testar, desligue pelo menu do sistema, ligue o Dell e use F12 no
teclado integrado. Para o Tahoe, selecione o TAHOE e depois `G15`. Para
o Windows, selecione `Windows Boot Manager` diretamente no F12. O
desligamento completo contornou o problema observado no reinício. As
fotos dos boots que travaram mostram que o kernel chegou ao PCI e ao
AppleNVMe, com o aviso de RP25 já visto antes do teste gráfico. O motivo
do travamento e da entrada na recuperação do Windows segue em aberto.
[Menu F12 do G15 5530](https://www.dell.com/support/manuals/pt-br/g-series-15-5530-laptop/dell-g15-5530-owners-manual/f12-one-time-boot-menu?guid=guid-71da763a-3f2f-4021-8ef7-3e50da009c74&lang=en-us)

Se o desktop continuar com vídeo básico, os comandos abaixo conferem o
perfil carregado, o recibo do pacote, a âncora de assinatura, os drivers
em execução e o suporte a Metal na mesma sessão do macOS.

```sh
sysctl -n kern.bootargs
pkgutil --pkg-info org.b00t0x.WhateverXe.Tahoe
ioreg -r -c AMFIAnchor -l
kmutil showloaded | grep -Ei 'WhateverXe|AMFIAnchor|AppleIntelTGL'
system_profiler SPDisplaysDataType
```

Em `AMFIAnchor`, conferir `InstallStatus = 1` e `LoadedAnchors` com
`org.b00t0x.AMFIAnchorInjector`. A presença de um kext na lista confirma
que foi carregado. O resultado de Metal precisa ser conferido em
`SPDisplaysDataType`. [Validação da âncora](https://github.com/b00t0x/AMFIAnchor/tree/0.1.2#install-with-opencore)
