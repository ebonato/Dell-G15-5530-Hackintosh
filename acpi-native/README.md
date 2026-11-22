# Coletas ACPI nativas — BIOS 1.35.0 e 1.37.0

Estas tabelas são evidência para comparação, não arquivos para copiar para a EFI.
As duas coletas ficam disponíveis ao Git; não há regra ignorando `acpi-native`.

- `SSDTTime-1.35.0/`: dump fornecido pelo usuário, 32 arquivos, preservados sem alterações.
- `bios-1.35.0/`: leitura anterior do registro do Windows, 17 arquivos distintos.
  Removidas apenas duas cópias duplicadas geradas pela coleta anterior.
- `manifest-1.35.0.json`: SHA-256, identificação, tamanho e checksum de todos os 49 arquivos.

## Resultado da comparação

**As 17 tabelas comuns são idênticas byte a byte.** A diferença entre as pastas é
de nomenclatura e abrangência, não uma divergência no conteúdo das tabelas comuns.
O SSDTTime enumera os SSDTs como `SSDT`, `SSDT1` etc.; a coleta do registro usou o
OEM Table ID nos nomes. Não é correto comparar apenas os nomes ou o número de arquivos.

| Coleta do registro | Arquivo SSDTTime | Resultado SHA-256 |
| --- | --- | --- |
| `registry-DSDT-DellInc.aml` | `DSDT.aml` | idêntico |
| `registry-FACP-DellInc.aml` | `FACP.aml` | idêntico |
| `registry-SSDT-ADebTabl.aml` | `SSDT14.aml` | idêntico |
| `registry-SSDT-AWCCTABL.aml` | `SSDT1.aml` | idêntico |
| `registry-SSDT-CpuSsdt.aml` | `SSDT2.aml` | idêntico |
| `registry-SSDT-DellRtd3.aml` | `SSDT4.aml` | idêntico |
| `registry-SSDT-DptfTabl.aml` | `SSDT3.aml` | idêntico |
| `registry-SSDT-IgfxSsdt.aml` | `SSDT5.aml` | idêntico |
| `registry-SSDT-NvdTable.aml` | `SSDT10.aml` | idêntico |
| `registry-SSDT-Pmax_Dev.aml` | `SSDT.aml` | idêntico |
| `registry-SSDT-PtidDevc.aml` | `SSDT7.aml` | idêntico |
| `registry-SSDT-SocCmn.aml` | `SSDT13.aml` | idêntico |
| `registry-SSDT-SocGpe.aml` | `SSDT12.aml` | idêntico |
| `registry-SSDT-TbtTypeC.aml` | `SSDT8.aml` | idêntico |
| `registry-SSDT-Tpm2Tabl.aml` | `SSDT9.aml` | idêntico |
| `registry-SSDT-UsbCTabl.aml` | `SSDT6.aml` | idêntico |
| `registry-SSDT-xh_Dell_.aml` | `SSDT11.aml` | idêntico |

O SSDTTime contém mais 15 tabelas: APIC, BGRT, BOOT, DBG2, DBGP, DMAR, FPDT,
HPET, LPIT, MCFG, NHLT, PHAT, TPM2, WSMT e XSDT. Elas não estavam na pasta da
coleta pelo registro; ausência nessa pasta não significa ausência no firmware.
Todos os 49 arquivos passaram na conferência de tamanho declarado e checksum ACPI.

O [manifesto](manifest-1.35.0.json) registra o mesmo mapeamento em cada entrada.
`equivalent` aponta para a `collection` e o `file` correspondentes na outra
coleta, identificados por SHA-256. O vínculo é recíproco. O valor `null`
indica que a outra coleta não contém um arquivo com o mesmo hash.

DSDT nativa de ambas as coletas: 653.360 bytes, SHA-256
`71A1652BE0EDE98C96887FA1ED79A2B0E8C0450D2F5221133F2F1ADC5A12DC22`.

## Comparação inicial com o material de Rodrigo Rangel

A DSDT distribuída em `EFI/OC/ACPI/DSDT.aml` é diferente da nativa:
520.568 bytes, SHA-256
`7019D13829DEB2E47CDD191340AFE1A6B262DE669ADE1EE43809B7DA239E445A`.

Na descompilação isolada com iASL 20250807, a tabela de Rodrigo Rangel contém
19 chamadas a `XOSI` e nenhuma a `_OSI`; a nativa tem 19 chamadas a `_OSI` e nenhuma a `XOSI`.
A nativa contém `Name (_S3, ...)`; essa definição não está na tabela de Rodrigo Rangel.
Isso comprova modificações relevantes além da diferença de tamanho e hardware.

Em ambas, o trecho examinado de `\_SB.PC00.LPCB.ECDV._STA` retorna `0x0F`.
O comentário no fonte EC/USBX de Rodrigo Rangel sobre esconder esse EC não descreve esse
trecho do AML distribuído. Não deve ser usado isoladamente para justificar patches.

Limite: isso é uma comparação inicial, não uma portabilidade concluída. A análise
conjunta das 16 tabelas DSDT/SSDT do SSDTTime, sem as cópias duplicadas da coleta
anterior, ainda encontra `AE_ALREADY_EXISTS`, incluindo `\_SB.TPM`.
As referências externas e os caminhos condicionais precisam de avaliação própria;
esse resultado do descompilador não prova defeito em execução nem justifica, por si
só, excluir uma tabela ou criar um patch. Nenhum AML foi gerado dessa análise para a EFI.

## O que a EFI de Rodrigo Rangel carrega

A listagem abaixo vem das entradas habilitadas em `ACPI > Add` nos dois plist do
commit-base `4cb55f4`. O OpenCore substitui a DSDT do firmware ao carregar uma
tabela com assinatura `DSDT`. Tabelas `SSDT` são adicionadas e podem referenciar
objetos definidos na DSDT ou em outros SSDTs. Consulte a seção *ACPI / Add* da
[documentação do OpenCore 1.0.8](https://github.com/acidanthera/OpenCorePkg/blob/1.0.8/Docs/Configuration.pdf).

| Tabela ativa | Instalador | Sistema | Função e o que conferir após a atualização |
| --- | :---: | :---: | --- |
| `DSDT.aml` | Sim | Sim | DSDT completa e modificada de Rodrigo Rangel. Definir se os ajustes necessários podem ser aplicados à tabela nativa durante o boot. Se houver motivo para uma substituição completa, usar a DSDT da BIOS instalada como base. |
| `SSDT-AWAC.aml` | Sim | Sim | Ajusta `STAS` para usar o RTC legado. Conferir `STAS`, a lógica do AWAC e o RTC do firmware. |
| `SSDT-EC-USBX.aml` | Sim | Sim | Injeta um dispositivo `EC` e propriedades de alimentação USB. Conferir o caminho de `LPCB`, o EC real e colisões com `EC` ou `USBX`. |
| `SSDT-PLUG-ALT.aml` | Sim | Sim | Adiciona objetos `Processor` e `plugin-type`. Conferir os IDs e os objetos de CPU com a topologia do i7 e as tabelas da BIOS instalada. |
| `SSDT-XOSI.aml` | Sim | Sim | Define `XOSI`, usado pelas chamadas renomeadas na DSDT de Rodrigo Rangel. Avaliar o caminho das teclas de brilho junto com o eventual redirecionamento das chamadas necessárias. |
| `SSDT-PS2K-RMCF.aml` | Sim | Sim | Configura o VoodooPS2 para mapear `PrtSc` a F13. Conferir `LPCB.PS2K` e um eventual `RMCF` já existente. |
| `SSDT-DeepIdle.aml` | — | Sim | Adiciona `LPS0` e `LXEN` para repouso S0. Conferir se os nomes já existem e testar suspensão em conjunto com os drivers de Rodrigo Rangel. |
| `SSDT-RTXKEXT.aml` | — | Sim | Expõe métodos usados pelo G15Fan para controlar energia da GPU. `PG01` aparece no SSDT OEM `NvdTable` da 1.35.0, em um bloco condicional. Conferir esse caminho, `AMWW`, os métodos de energia e o comportamento real da RTX 3050. |
| `SSDT-PNLF.aml` | — | Sim | Cria o dispositivo de brilho sob `\_SB.PC00.GFX0`. Conferir o caminho do vídeo integrado e o brilho do painel com o driver gráfico escolhido. |
| `SSDT-LIDWAKE.aml` | — | Sim | Adiciona `PRW0` em `\_SB.LID0` com GPE `0x6E`. Conferir tampa, roteamento do evento e despertar real. |
| `SSDT-BTRK.aml` | — | Sim | Expõe comandos para o G15Fan controlar o reset do Bluetooth. A DSDT da 1.35.0 contém `BTRK` e `GBTR`; conferir também `AMWW` e a ligação do GPIO na BIOS instalada. |

Os dois perfis usam também `ACPI > Delete` para remover a primeira tabela com
assinatura `SSDT` e OEM Table ID `xh_Dell_`. Ela aparece como `SSDT11.aml` na
coleta SSDTTime 1.35.0. A necessidade dessa exclusão ainda precisa ser avaliada
com a BIOS instalada e o comportamento do USB. `ACPI > Patch` está vazio nos
dois perfis. As alterações descritas no passo 4 vêm no `DSDT.aml` já compilado.

## Correspondência entre fonte e AML

Os nove `.dsl` de `acpi-src` correspondem a SSDTs da EFI. Todos compilaram com
iASL 20250807 em uma pasta de análise. Sete reproduziram o corpo AML entregue
byte a byte, descontando o cabeçalho ACPI de 36 bytes. São `AWAC`, `DeepIdle`,
`EC-USBX`, `PLUG-ALT`, `PS2K-RMCF`, `RTXKEXT` e `XOSI`.

Em `BTRK` e `LIDWAKE`, o corpo AML entregue é idêntico ao trecho final do corpo
recompilado. A recompilação acrescentou declarações `External` antes desse
trecho. As descompilações mostraram os mesmos métodos de reset e a mesma
propriedade `PRW0`, respectivamente. Para ler `BTRK`, o iASL recebeu as
assinaturas de `BTRK` e `GBTR` observadas na DSDT 1.35.0. A comparação confirma
que os nove fontes correspondem ao código entregue. O funcionamento de cada
SSDT ainda depende dos objetos que a BIOS publica durante o boot.

`DSDT.aml` e `SSDT-PNLF.aml` estão na EFI em formato compilado. O checkout não
traz os `.dsl` correspondentes em `acpi-src`.

## Como decidir o que portar

O dump nativo já traz a DSDT em AML. O iASL consegue descompilá-la para DSL, com
as demais tabelas como apoio para resolver referências. Esse DSL reconstruído
serve para comparar caminhos e métodos mesmo sem os fontes originais da Dell.
Antes de usá-lo para gerar um AML de boot, seria preciso resolver as referências
e conferir a recompilação. A comparação com a DSDT modificada de Rodrigo Rangel confirma
as diferenças em `_OSI`/`XOSI` e na definição de `_S3`. Para atribuir todas as
outras diferenças a Rodrigo Rangel, seria necessário também um dump original da BIOS 1.0.

Na 1.35.0, `_S3` está dentro de `If (SS3)`, `SS3` é inicializado com `Zero` no
DSL e a FADT anuncia baixo consumo em S0 (`LOW_POWER_S0_IDLE_CAPABLE`). Isso pede
verificar o estado efetivo na BIOS final antes de decidir pela remoção de `_S3`.
O par `_OSI`/`XOSI` também tem uma finalidade específica, as teclas de brilho.
Se a adaptação usar a DSDT nativa, carregar só `SSDT-XOSI` não redireciona as
chamadas. O redirecionamento deve ser avaliado com as referências da BIOS final.

Para o primeiro boot de instalação, a proposta é usar a DSDT da BIOS instalada
e revisar as cinco adições do perfil de instalação, além da exclusão de
`xh_Dell_`. Cada SSDT pode ser reaproveitado se as dependências forem compatíveis.
Se o nome ou o comportamento de um objeto de firmware mudou, portar o ajuste
específico. O perfil do sistema acrescenta brilho do painel e ajustes de repouso,
energia da RTX e Bluetooth que podem ser validados depois de estabelecer o boot.
Se houver evidência de que um ajuste é necessário já no instalador, ele entra
no perfil de instalação. Essa ordem preserva a finalidade dos dois perfis.

## BIOS 1.37.0 confirmada no Windows

A coleta `SSDTTime-1.37.0/` tem os mesmos 32 nomes da coleta SSDTTime anterior.
Os 32 arquivos passam nas verificações de tamanho e checksum ACPI. Por SHA-256,
30 arquivos são idênticos, incluindo DSDT, todos os 15 SSDTs, FACP, APIC e DMAR.
O [manifesto da 1.37.0](manifest-1.37.0.json) liga cada arquivo à tabela anterior
e registra se o conteúdo mudou.

| Tabela | Diferença entre as coletas 1.35.0 e 1.37.0 |
| --- | --- |
| `BGRT.aml` | `Image Address` foi `0x575BC018` na 1.35.0, `0x575BB018` na primeira coleta 1.37.0 e `0x56AF9018` na nova coleta 1.37.0. Os checksums foram `0x3E`, `0x4E` e `0x1B`, respectivamente. Tipo, status, orientação e coordenadas permaneceram iguais. |
| `PHAT.aml` | O campo de versão do registro com GUID `03285589-1E37-4B72-A5F9-70D1ADAE5D34`, produtor `INTC`, passou de `0x0000000117000000` para `0x0000000119000000`. O checksum mudou de `0xB6` para `0xB4`. O GUID não identifica por si só o componente por nome. |

A nova coleta na mesma BIOS 1.37.0 usou `acpidump` 20251212. O iASL 20251212
confirmou que só mudaram o endereço físico do bitmap de boot e o checksum da
BGRT. A especificação define a BGRT como uma tabela dinâmica. Os outros 31
arquivos, incluindo DSDT, todos os SSDTs e PHAT, têm o mesmo SHA-256 nas duas
coletas da 1.37.0. O manifesto registra o hash da captura mais recente.

Essas duas alterações estão em tabelas de dados. Não apareceu mudança nos
métodos AML coletados. Isso permite reaproveitar a análise das dependências da
1.35.0 para preparar a EFI da 1.37.0. A comparação cobre o conteúdo do dump e
não testa a execução dos métodos nem o comportamento do firmware.
[Especificação ACPI 6.6](https://uefi.org/specs/ACPI/6.6/05_ACPI_Software_Programming_Model.html)

## Passo 4 aplicado à EFI local

Os dois perfis usam a DSDT nativa da BIOS 1.37.0. Em `ACPI > Add`, `DSDT.aml`
e `SSDT-XOSI.aml` ficam desativados. O OpenCore só substitui a DSDT do firmware
quando carrega uma tabela com assinatura `DSDT`. O `XOSI` de Rodrigo Rangel atendia às
chamadas renomeadas na DSDT que ele distribuía. As teclas Fn de brilho ficam
para teste no sistema. A definição condicional de `_S3` permanece a do firmware.
Consulte a seção ACPI da [referência do OpenCore 1.0.8](https://github.com/acidanthera/OpenCorePkg/blob/1.0.8/Docs/Configuration.pdf).

- `SSDT-AWAC`, `SSDT-EC-USBX`, `SSDT-PLUG-ALT` e `SSDT-PS2K-RMCF` seguem
  ativos no instalador e no sistema. A conferência estática localizou `STAS`
  e o RTC legado, `LPCB.ECDV`, `LPCB.PS2K` e os 20 IDs de CPU ativos (0 a 19).
- A exclusão de `xh_Dell_` em `ACPI > Delete` segue a base do instalador de
  Rodrigo Rangel. O dump atual identifica essa tabela como `SSDT11.aml`. O efeito
  sobre as portas USB deve ser conferido no primeiro boot com a DSDT nativa. A saída
  da descompilação conjunta não demonstra, por si só, um conflito em execução.
- `PNLF`, `DeepIdle`, `LIDWAKE`, `RTXKEXT` e `BTRK` seguem exclusivos do
  perfil pós-instalação. A revisão encontrou `GFX0`, `LID0`, `AMWW`, `BTRK` e
  `GBTR` na 1.37.0. O recurso `PG01` aparece num bloco condicional de `NvdTable`
  (`SSDT10.aml`). A presença desses nomes não comprova os efeitos no macOS.
  Brilho, repouso, Bluetooth e corte de energia da RTX 3050 exigem testes
  específicos antes de usar o perfil pós-instalação no dia a dia.

O `ocvalidate` 1.0.8 validou os dois plist no repositório e no pendrive. A
cópia aplicou as duas entradas ACPI e preservou o SMBIOS e os nomes dos perfis
dos passos 2 e 3. O primeiro boot ainda precisa validar a execução no Dell.

## Boot da Recuperação com a DSDT nativa

O primeiro boot pela entrada `TAHOE (External) (dmg)` abriu a Recuperação e
chegou à inicialização ACPI do macOS. A foto mostra `AE_NOT_FOUND` para campos
presentes no dump 1.37.0, incluindo `STAS` e `PXDC` na região `GNVS` e
`BMID` na região `DSMP`. O log do OpenCore confirma a DSDT nativa e a
exclusão de `xh_Dell_` naquele teste.

No AcpiExec 20250807, a DSDT nativa isolada acusa `AE_NOT_FOUND` ao
resolver `\_SB.PC00.RP25`, mas a carga continua e publica `STAS`. O
dispositivo `RP25` está sob uma condição na DSDT nativa e há um escopo
posterior que o referencia. A DSDT de Rodrigo Rangel carrega `RP25` no mesmo teste e
usa outros endereços para `GNVS`, `PNVA` e `DSMP`. A primeira falha no
interpretador do macOS ainda precisa ser capturada. O perfil de instalação
habilita `AppleDebug` e `ApplePanic` para o próximo teste.

### Compatibilidade com o interpretador ACPI do macOS

Na tentativa de 21/11/2026, o log registrou `EXITBS:START` e a tela avançou
até o mesmo ponto da inicialização ACPI, onde ficou por cinco minutos.
O AcpiExec 20160930, versão indicada na tela, reproduziu a rejeição da
DSDT nativa por `AE_NOT_FOUND` em `\_SB.PC00.RP25`. O Windows detectou
esse mesmo caminho ACPI como a porta PCI `8086:7A48`, em `00:1a.0`. A
DSDT publicada no registro do Windows tem o mesmo SHA-256 do dump 1.37.0.

O perfil de instalação ganhou um patch em `ACPI > Patch`. Ele move só a
declaração de `\_SB.PC00.RP25` para fora de `If (PCHS == 0x04)` e
ajusta o tamanho desse bloco. As outras declarações continuam condicionais.
A regra exige a assinatura `DSDT`, o OEM Table ID `Dell Inc`, o tamanho
de 653.360 bytes e a sequência exata do AML da 1.37.0. Ela é aplicada à
tabela publicada pelo firmware durante o boot. O perfil de sistema no
repositório também inclui a mesma regra para a DSDT 1.37.0.

O [diff de RP25 em ASL](../acpi-src-live/DSDT-RP25.diff) mostra a
mudança na declaração de RP25. Nos perfis em `EFI/OC`, a regra em
`ACPI > Patch` substitui 1.646 bytes a partir do offset `0x1D95A`.
A sequência foi encontrada uma vez na DSDT coletada.

Na simulação com a DSDT corrigida e as mesmas SSDTs da tentativa, o
AcpiExec 20160930 carregou as 19 tabelas AML e localizou `RP25`, `STAS`,
`PXDC` e `BMID`, tanto no modo padrão quanto com `-em`. O iASL
descompilou o AML e confirmou a nova posição de `RP25`. Com esse patch,
um boot do pendrive chegou à interface da Recuperação no Dell.

### DSDT dos perfis Live

Os perfis Live Debug e Release carregam a mesma `DSDT-G15-1.37.0.aml`
em `ACPI > Add`. Os nove patches antes listados em `ACPI > Patch`
no commit `286fd67` foram aplicados em ordem ao AML coletado neste Dell.
As cinco regras de HPET e as alterações de PNLF, OSID e _OSI estão na
mesma tabela que a correção de RP25. No dump, nenhuma dessas regras
atingia outra tabela carregada. A DSDT mantém o tamanho e o cabeçalho
originais, com o checksum recalculado.

O [diff de RP25](../acpi-src-live/DSDT-RP25.diff) e o
[diff das demais correções](../acpi-src-live/DSDT-Live-Extras.diff)
registram as mudanças em ASL. Os dois diffs, aplicados nessa ordem,
reproduzem o ASL descompilado com iASL 20251212 a partir de
`DefinitionBlock`, com finais de linha LF e sem espaços ao fim das
linhas. O AML foi gerado por substituição de bytes na tabela nativa.
A recompilação direta do ASL descompilado com iASL apresentou erros
nas referências do firmware ao controlador embarcado.

O AcpiExec 20160930 carregou as 24 tabelas AML dos perfis Live no modo
padrão e com `-em`. A DSDT de origem coincidiu com a tabela publicada
pelo Windows na BIOS 1.37.0. Após alterar a BIOS ou suas configurações,
compare um novo dump antes de reutilizar esta DSDT, pois a tabela
contém endereços de memória informados pelo firmware.

## Patches do SSDTTime avaliados na BIOS 1.37.0

O perfil de instalação já cobre FakeEC Laptop e USBX com `SSDT-EC-USBX`,
PluginType com `SSDT-PLUG-ALT` e RTCAWAC com `SSDT-AWAC`. Ao gerar
RTCAWAC para este dump, o SSDTTime também selecionou `STAS` e preservou
os recursos do RTC. FakeEC Laptop gerou o mesmo dispositivo `EC` de
HID `ACID0001` sob `\_SB.PC00.LPCB`, ativo em Darwin. O PluginType
regenerado tem o mesmo corpo AML do arquivo da EFI, com diferença no
cabeçalho ACPI. O USBX gerado usa valores padrão de 2100 para limite de
corrente e 5100 para alimentação. A EFI usa 3000 e 9600, respectivamente.
O mapeamento de portas USB e as propriedades de alimentação do USBX são
ajustes independentes.

Os ajustes dependentes do hardware devem partir da DSDT e dos SSDTs do
nosso Dell. Para brilho e repouso, conferir os caminhos e condições de
`GFX0`, `OSID`, `SMEE`, `LID0` e os GPEs antes de portar mudanças.
Para a RTX 3050 e Bluetooth, conferir os recursos de energia e o GPIO e
testar no Dell. A carga das tabelas em simulação confirma o namespace
da combinação, enquanto o efeito físico requer teste próprio.

O `SSDT-PNLF` fica no perfil pós-instalação.
`SSDT-XOSI` precisa do redirecionamento de chamadas `_OSI` para ter efeito
e fica desativado com a DSDT nativa.

O FixHPET gerou `SSDT-HPET.aml` e cinco patches para a DSDT. São dois
renames do HPET (`_STA` e `_CRS`) e três ajustes de IRQ (IPIC 2, RTC 8,
TIMR 0). Cada busca encontrou uma ocorrência após aplicar o patch RP25.
Em simulação, o AcpiExec 20160930 carregou as 20 tabelas no modo padrão
e com `-em` e resolveu RP25, HPET, STAS, PXDC e BMID. O efeito sobre os
IRQs no boot real segue em aberto.

O Fix DMAR detectou uma região reservada para a iGPU (`00:02.0`). O
instalador usa `DisableIoMapper=true`, que desativa o IOMapper no macOS.
SMBus gerou uma extensão para o `SBUS` nativo em `00:1f.4`, e ALS0 gerou
um sensor de luz virtual.

### Perfil de instalação reconstruído com o dump local

O [perfil de instalação](../EFI/OC/config-install.plist) carrega FakeEC
Laptop, USBX, RTCAWAC, HPET e SMBus gerados pelo SSDTTime com iASL 20251212
a partir da BIOS 1.37.0 deste Dell. Os cinco pares DSL/AML ficam em
`acpi-src` e `EFI/OC/ACPI`. A geração local de PluginType produziu o
mesmo corpo AML da PLUG-ALT existente. O USBX local usa os valores padrão
do SSDTTime. A personalização PS/2 do atalho Fn+F10 fica no perfil de
sistema.

A simulação com AcpiExec 20160930 carregou as 21 tabelas no modo padrão
e com `-em`. As seis regras da DSDT, incluindo RP25 e as cinco do HPET,
encontraram uma ocorrência cada, com assinatura, OEM Table ID e tamanho
do dump local. O `ocvalidate` 1.0.8 aceitou o perfil reconstruído.

O SSDTTime também gerou candidatos locais PNLF, XOSI, DMAR sem a região
reservada e ALS0. PNLF, XOSI e ALS0 tratam de funções que podem ser
avaliadas após a instalação. Para DMAR, o perfil continua usando
`DisableIoMapper=true`.
