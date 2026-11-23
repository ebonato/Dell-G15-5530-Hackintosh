#!/bin/bash
set -u
export PATH=/usr/bin:/bin:/usr/sbin:/sbin

if [ "$(uname -s)" != Darwin ]; then
    printf '%s\n' 'Execute este diagnóstico no macOS.' >&2
    exit 1
fi

directory=$(cd "$(dirname "$0")" && pwd -P) || exit 1
report="$directory/diagnostico-tahoe-video.txt"
temporary=$(mktemp "$directory/.diagnostico-tahoe-video.XXXXXX") || exit 1
trap 'rm -f "$temporary"' EXIT

printf '%s\n' 'Coletando o diagnóstico de vídeo. Aguarde a confirmação no Terminal.'

{
    printf '\n=== macOS, perfil de boot e pacote Graphics ===\n'
    sw_vers
    sysctl -n kern.bootargs
    pkgutil --pkg-info org.b00t0x.WhateverXe.Tahoe

    printf '\n=== Histórico de atualizações e snapshots do sistema ===\n'
    softwareupdate --history
    diskutil apfs listSnapshots /
    if [ -r /var/log/install.log ]; then
        printf '\n=== Mensagens recentes do instalador do macOS ===\n'
        grep -Ei 'MobileSoftwareUpdate|OSInstaller|rollback|revert(ing)?.*snapshot' /var/log/install.log | tail -n 60
    fi

    printf '\n=== Âncora de assinatura ===\n'
    ioreg -r -c AMFIAnchor -l -w 0

    printf '\n=== Kexts de vídeo carregados ===\n'
    kmutil showloaded | grep -Ei 'WhateverXe|AMFIAnchor|AppleIntelTGL|NVDA|NVRM'

    printf '\n=== Gráficos e monitores ===\n'
    system_profiler SPDisplaysDataType

    printf '\n=== GPUs no IORegistry ===\n'
    for name in GFX0 IGPU PEGP; do
        printf '\n--- %s ---\n' "$name"
        ioreg -p IOService -r -n "$name" -d 2 -l -w 0
    done

    printf '\n=== Framebuffers e acelerador Intel ===\n'
    for class in IONDRVFramebuffer AppleIntelFramebufferController AppleIntelFramebuffer IntelAccelerator; do
        printf '\n--- %s ---\n' "$class"
        ioreg -p IOService -r -c "$class" -d 1 -l -w 0
    done

    printf '\n=== Propriedades de inicialização do WhateverXe ===\n'
    ioreg -p IOService -l -w 0 | grep -E '"(WhateverXe|WEX|G15Mux|G15ADLSSetMode|G15ADLSCDClockFix|G15NoCCS|TGLEDP)[^"]*"'

    printf '\n=== Buffer de mensagens gráficas do kernel ===\n'
    if kernel_messages=$(dmesg 2>&1); then
        printf '%s\n' "$kernel_messages" | grep -Ei 'WhateverXe|IntelAccelerator|AppleIntelTGL|link.training|modeset|G15ADLS' | tail -n 240
    else
        printf '%s\n' "$kernel_messages"
    fi


    printf '\n=== Mensagens gráficas do boot atual ===\n'
    log show --last boot --style compact --info --debug \
        --predicate 'sender == "WhateverXe" OR (process == "kernel" AND (eventMessage CONTAINS[c] "WhateverXe" OR eventMessage CONTAINS[c] "IntelAccelerator" OR eventMessage CONTAINS[c] "AppleIntelTGL" OR eventMessage CONTAINS[c] "link training" OR eventMessage CONTAINS[c] "modeset"))' 2>&1 | tail -n 240
} > "$temporary" 2>&1

mv -f "$temporary" "$report" || exit 1
printf 'Diagnóstico salvo em %s\n' "$report"
