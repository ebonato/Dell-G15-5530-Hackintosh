/*
 * Bluetooth preso em reset pelo G15Fan.kext (Dell G15 5530, WhateverXe #87). Com o BT desligado no macOS o aparelho
 * USB do BT (Intel AX201, 8087:0026) continua enumerado e o rádio/CNVi ainda quebra o pacote (NOT_L1). O \_SB.BTRK do
 * DSDT da Dell move o GPIO de reset do BT (GBTK): Zero = em reset (sai do USB), One = solto.
 * Os nomes são simples e ficam no \_SB.AMWW porque o evaluateObject da Apple não executa caminho com pontos.
 * O .aml é gerado por tools/gen-ssdt-btrk.py (o Mac não tem iasl); este arquivo é a fonte equivalente.
 */
DefinitionBlock ("", "SSDT", 2, "G15", "BtReset", 0x00001000)
{
    External (\_SB.AMWW, DeviceObj)
    External (\_SB.BTRK, MethodObj)
    External (\_SB.GBTR, MethodObj)

    Scope (\_SB.AMWW)
    {
        Method (BTOF, 0, Serialized) { \_SB.BTRK (Zero) }
        Method (BTON, 0, Serialized) { \_SB.BTRK (One) }
        Method (BTST, 0, NotSerialized) { Return (\_SB.GBTR ()) }
    }
}
