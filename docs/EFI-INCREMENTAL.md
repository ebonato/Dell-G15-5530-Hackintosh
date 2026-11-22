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

A diferença funcional do plist se limita ao `Kernel > Add` e a `boot-args`.
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
