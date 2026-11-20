# Start — adaptação do Dell G15 5530

Estado em 20/11/2026: preparação antes da atualização de BIOS. A versão confirmada
é **1.35.0**; o pacote 1.37.0 foi informado, mas sua instalação ainda não foi confirmada.

| Item | Alvo deste fork |
| --- | --- |
| CPU | i7-13650HX, 6P + 8E, 14 núcleos / 20 threads |
| iGPU | Intel UHD `8086:A78B` |
| dGPU | NVIDIA RTX 3050 6 GB Laptop `10DE:25EC` |
| Tela | 120 Hz; não foram criados timings ou patches de EDID |
| Gráficos | Hybrid/Optimus, conforme cenário informado |
| Sistema | macOS Tahoe em NVMe dedicado, separado do Windows |

CPU, IDs das GPUs e BIOS 1.35.0 foram conferidos no Windows. Tela, modo híbrido e
destinação do SSD seguem as informações do usuário. O comportamento no macOS ainda
não foi testado. Não foram alterados firmware, modo de armazenamento, discos ou boot UEFI.

## Os seis pontos resolvidos

1. **Perfis do OpenCore.** `config-install.plist` e `config.plist` foram restaurados
   exatamente ao commit-base `4cb55f4` deste checkout. O primeiro continua sendo o
   perfil de instalação do autor; o segundo, o perfil pós-instalação com WhateverXe
   e os componentes originais. Foram desfeitas a troca gráfica, a remoção de kexts,
   as mudanças de boot-args, a alteração do nome de CPU e as demais mudanças anteriores.
   O OpenCore normalmente lê `config.plist`; o nome `config-install.plist` não cria
   uma seleção automática por fase. O procedimento upstream coloca o perfil de
   instalação sob o nome `config.plist` na mídia usada para instalar.

2. **OpenHfsPlus removido.** O arquivo acrescentado e sua documentação de licença
   foram removidos. Não há entradas dele nos perfis restaurados. Não foi observada
   falha que justificasse adicioná-lo. O destino APFS, sozinho, não determina o
   formato de todos os componentes de um instalador; isso será verificado com a
   mídia efetivamente usada, se surgir uma necessidade concreta.

3. **EC/USBX restaurado, fonte e binário.** A condição `_STA` que acrescentei ao
   USBX restringia a apresentação desse dispositivo a Darwin. Não corrigia nenhum
   erro documentado ou reproduzido nesta máquina; foi retirada. O `.dsl` em
   `acpi-src` é o fonte legível e o `.aml` em `EFI/OC/ACPI` é o binário que o
   OpenCore pode carregar. Ambos voltaram ao original, sem nova compilação ou patch.
   Esse SSDT complementar não é a DSDT nativa coletada da BIOS.

4. **Coletas disponíveis ao Git.** `acpi-native` não está ignorado. A remoção da
   regra de ignore já estava presente no início desta revisão e foi preservada.
   Mantidas as duas coletas 1.35.0; removidas somente duas cópias duplicadas da
   extração anterior, após verificar igualdade de hash. Não houve commit automático.

5. **Dump SSDTTime comparado.** Os 32 arquivos fornecidos foram preservados sem
   alteração. As 17 tabelas comuns à coleta anterior são idênticas byte a byte;
   as outras 15 ampliam o inventário. Nomes diferentes não representam conteúdos
   diferentes. A correspondência, os hashes e a comparação inicial com a DSDT do
   autor estão no [registro ACPI](../acpi-native/README.md).

6. **BIOS antes da adaptação final.** A coleta 1.35.0 fica como referência anterior
   à atualização. Nenhum ajuste específico da 1.37.0 foi presumido. Após confirmar
   a versão instalada e obter seu dump, comparar com a 1.35.0 e com a tabela do
   autor antes de modificar novamente os perfis e os SSDTs.

## SIP e ponto de partida

Os valores originais do projeto foram restaurados, sem imposição de SIP integral:

| Perfil | `csr-active-config` (bytes hex) | Gráficos originais |
| --- | --- | --- |
| Instalação | `130A0000` | WhateverGreen, `-wegnoegpu`, I2C em polling |
| Pós-instalação | `430A0000` | WhateverXe e componentes do autor |

As permissões futuras serão ajustadas em função da cadeia de drivers e pacotes
efetivamente escolhida. Não foi aplicada uma política genérica de segurança ao
custo de remover o funcionamento pretendido pelo projeto.

**Start não significa EFI pronta para boot neste i7.** Os perfis restaurados ainda
carregam a DSDT modificada do autor e recursos do i5/RTX 4050. Mantê-los como base
comparável evita misturar ajustes não testados com o processo de atualização da
BIOS. O próximo trabalho será adaptar essa base ao firmware final e ao hardware
confirmado, preservando a função distinta de cada perfil.

## Verificações desta revisão

- Os dois perfis e o par fonte/AML EC/USBX conferidos contra os blobs do commit-base.
- Ambos os perfis validados com ocvalidate **1.0.8**, sem problemas de configuração.
- Caminhos habilitados de ACPI, kexts, executáveis, Info.plist e drivers conferidos.
- Todos os 49 arquivos das duas coletas conferidos por tamanho declarado e checksum;
  correspondências verificadas por SHA-256.
- Nenhuma instalação ou inicialização do macOS foi realizada. ocvalidate e checksums
  não validam a compatibilidade de hardware, os métodos ACPI ou a experiência de instalação.
