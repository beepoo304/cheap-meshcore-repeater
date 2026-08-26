[CmdletBinding()]
param(
    [string]$PublicKey,
    [string]$PrivateKey
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Read-HexKey {
    param(
        [string]$Value,
        [int]$ExpectedLength,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        $Value = Read-Host "$Label ($ExpectedLength hexadecimal characters)"
    }

    $Value = $Value.Trim().ToUpperInvariant()
    if ($Value.Length -ne $ExpectedLength -or $Value -notmatch '^[0-9A-F]+$') {
        throw "$Label must contain exactly $ExpectedLength hexadecimal characters."
    }

    return $Value
}

function Format-HexArray {
    param(
        [string]$Hex,
        [int]$ValuesPerLine = 16
    )

    $values = for ($index = 0; $index -lt $Hex.Length; $index += 2) {
        '0x' + $Hex.Substring($index, 2)
    }

    $lines = for ($index = 0; $index -lt $values.Count; $index += $ValuesPerLine) {
        '    ' + (($values[$index..([Math]::Min($index + $ValuesPerLine - 1, $values.Count - 1))]) -join ', ')
    }

    return $lines -join ",`r`n"
}

$public = Read-HexKey -Value $PublicKey -ExpectedLength 64 -Label 'Public key'
$private = Read-HexKey -Value $PrivateKey -ExpectedLength 128 -Label 'Private key'

$identityPath = Join-Path $PSScriptRoot 'identity.cpp'
$source = @"
#include "identity.h"

const uint8_t GHOST_PUBLIC_KEY[32] = {
$(Format-HexArray -Hex $public)
};

const uint8_t GHOST_PRIVATE_KEY[64] = {
$(Format-HexArray -Hex $private)
};
"@

[System.IO.File]::WriteAllText($identityPath, $source, [System.Text.UTF8Encoding]::new($false))
Write-Host "identity.cpp created locally. It is ignored by Git and will not be committed."
