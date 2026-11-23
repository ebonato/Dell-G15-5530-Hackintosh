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

### Terceiro teste com o WhateverXe do Rodrigo Rangel

O pendrive TAHOE e a EFI-Incremental receberam o `WhateverXe.kext` de
`EFI/OC/Kexts` do upstream `3da8474`, que corresponde à revisão
`bf35ea5` de `g15-deep-idle` e carrega a identificação
`DBG-083-2026-11-22`. A entrada do OpenCore continua a mesma, inclusive
a ordem após Lilu e os limites Darwin 25.x. O executável tem SHA-256
`c95b5a1af29c02fe48f260171dd2daacdc6d88b2cc5a9095d6094153a3a3234e`.
[Revisão no upstream](https://github.com/rodgod-rr/Dell-G15-5530-Hackintosh/commit/3da847445e3a0d9befc9857b2517615b4db18b4b) ·
[Código do kext](https://github.com/rodgod-rr/WhateverXe/tree/bf35ea5b8dfc594d8cd1297b3fc6c76811dccfb9)

O perfil mantém Lilu 1.7.2, AMFIAnchor 0.1.2, seu injector, AMFI ativa e
SIP `03080000`. WhateverGreen e AMFIPass estão fora do carregamento. Esse
é o caminho de assinatura indicado pelo b00t0x e pelo README da revisão
`bf35ea5` para o Graphics não modificado. O Graphics oficial 0.4.4 no
pendrive coincide com o SHA-256 publicado. A branch `g15` do Rodrigo, na
revisão `f1d112a`, usa os mesmos payloads e contém patches
pós-instalação com assinatura ad hoc. A confirmação do recibo, da âncora
e dos kexts carregados depende de uma sessão no Tahoe.
[Instalação do WhateverXe](https://github.com/b00t0x/WhateverXe#installation) ·
[Graphics 0.4.4](https://github.com/b00t0x/WhateverXe-Graphics/releases/tag/v0.4.4) ·
[Camada pós-instalação do Rodrigo](https://github.com/rodgod-rr/WhateverXe-Graphics/blob/f1d112a0b87ba34ea5caa63a8ffb16a88d3aaa8a/g15/README.md)

#### Argumentos por etapa

| Momento | Argumentos do upstream | Aplicação |
|---|---|---|
| Já presentes antes da troca | `keepsyms=1`, `debug=0x100`, `-allow3d`, `-disablegfxfirmware`, `-igfxdvmt`, `nvfb=1`, `nvaccel=1`, `nvfbheads=4` | Os dois primeiros ajudam a diagnosticar panics. Os três seguintes configuram a iGPU e o caminho de DVMT. Os três `nv*` são herdados da Live e se aplicam à etapa do driver da RTX. |
| Neste teste | `-wex-g15-noccs`, `-wexdbg` | Desabilita o uso do motor de compute CCS em favor do RCS, conforme a correção do Rodrigo para travamentos da UHD. O segundo ativa mensagens da build Debug. |
| Inicialização do painel, se necessário | `gfxrst=1` | Força o modo de reset do framebuffer. Testar sozinho se o driver carregar e o painel falhar no handoff. |
| Brilho, depois de Metal | `wexblmin=4900`, `-wex-blmin=4900` | `wexblmin` define o piso do PWM e já tem 4900 como padrão nessa revisão. A grafia com hífens não aparece no código inspecionado. |
| Painel e estabilidade, depois de Metal | `wexg15psr=3`, `wexg15unstick=31`, `-wex-g15-gsync`, `-wex-g15-ccsreset-off` | PSR1, tentativas de recuperar motores travados e ajustes do reset da Intel. Testar conforme o sintoma e em rodadas separadas. |
| Diagnóstico sob demanda | `msgbuf=4194304`, `-g15diag`, `IGLogLevel=3`, `acpi=0xC` | Buffer maior, diagnósticos do G15, verbosidade de gráficos Intel e eventos ACPI. Usar conforme a investigação. |
| Energia, CPU e RTX | `-g15pm`, `hcmode=4`, `-g15egpu`, `-g15fan-rtxpm-corte` | Dependem de G15PM, IntelHybridCores ou G15Fan. Validar CPU e energia em separado. As opções da RTX vêm depois do NullMoth e da validação da RTX 3050 e do HDMI. |
| NVMe | `-nvmefaspm` | Força ASPM L1 globalmente. NVMeFix já está habilitado no plist. Teste separado após estabilizar o boot. [NVMeFix](https://github.com/acidanthera/NVMeFix#configuration) |
| Patches pós-instalação com assinatura ad hoc | `amfi_get_out_of_my_way=1` | A variante do Rodrigo pede essa opção quando aplica sua camada ad hoc. O teste atual carrega o Graphics oficial pela cadeia do AMFIAnchor. |

O perfil também conserva `-v` e `-nvkmsnosmooth`, vindos da Live.
[Argumentos do Rodrigo](../README.md#boot-args-of-the-system-config)

#### ACPI para as próximas etapas

| Tabela | Situação neste Dell |
|---|---|
| `DSDT.aml`, `XOSI`, `PNLF` | A DSDT da BIOS 1.0 de Rodrigo difere da 1.37.0 local. A incremental usa a DSDT própria com RP25, HPET e redirecionamentos de OSI, mais as SSDTs locais XOSI e PNLF. Ambas as PNLF têm UID 19. Comparar a PNLF de Rodrigo sob GFX0 e a integração com BrightnessKeys se o brilho falhar depois de Metal. |
| `AWAC`, `EC-USBX`, `PLUG-ALT` | Já cobertos por AWAC, EC, USBX e PLUG-ALT locais. A tabela combinada de Rodrigo duplicaria os dispositivos EC e USBX. |
| `DeepIdle` e `LIDWAKE` | Candidatas para repouso S0. As tabelas adicionam LPS0/LXEN e PRW0 na tampa. A tampa existe na DSDT local. Conferir o GPE do EC e a definição condicional de _S3 antes de aplicar ao perfil de repouso. |
| `PS2K-RMCF` | O caminho PS2K existe na BIOS 1.37.0. A tabela mapeia o scancode de Print Screen para F13 no VoodooPS2, para uso do G15.app. |
| `BTRK` | BTRK e GBTR existem na DSDT local. O dispositivo AMWW vem da SSDT1 nativa. Candidata para reset do Bluetooth sob controle do G15Fan, após validar Bluetooth e wake. |
| `RTXKEXT` | AMWW, PEG1.PG01 e PEG1.PEGP.GSTA estão no conjunto 1.37.0. Validar o comportamento das transições com o binário da RTX 3050, seu HDMI e a versão do G15Fan antes de adicionar GOFF/GONN. |

A comparação foi feita com o dump SSDTTime 1.37.0 e o iASL 20251212. O
teste do WhateverXe roda com o ACPI da incremental. [Evidências ACPI](../acpi-native/README.md) ·
[Fontes das SSDTs do Rodrigo](../acpi-src)

#### Alcance da issue 81

A discussão começou com travamentos no pós-processamento de vídeo em
Tiger Lake. Rodrigo reproduziu o caso no Dell i5-13450HX com A78B, Tahoe
26.7.2, kext 0.8.0 modificado e Graphics 0.4.0 modificado. Ao retirar
dois formatos da lista de saída do processamento de vídeo, as prévias
da App Store, o TV e o protetor de tela passaram a funcionar no teste.
H.264 e HEVC por hardware continuaram disponíveis. O relato confirma
que a A78B já estava acelerada naquela combinação e descreve uma
correção de reprodução de vídeo, posterior à inicialização de Metal.
O Graphics 0.4.4 traz correções próprias de conversão e vazamento. Os
patches VA e stencil da camada G15 têm objetivos adicionais e dependem
dos sintomas observados depois da aceleração.
[Relato na issue 81](https://github.com/b00t0x/WhateverXe/issues/81) ·
[Notas do Graphics 0.4.4](https://github.com/b00t0x/WhateverXe-Graphics/releases/tag/v0.4.4)

O `ocvalidate` 1.0.8 passou nos dois plists e os 176 arquivos da EFI
ativa coincidiram por SHA-256. No próximo boot a frio, use
`F12 > TAHOE > G15`, com o Graphics 0.4.4 já instalado. Confira Metal e
o painel interno. Se ainda aparecer vídeo básico, use as verificações
da seção anterior e confira `WhateverXePhysicalGGC` no IORegistry para
investigar o DVMT.
