param(
    [string]$BuildRoot = (Join-Path $PSScriptRoot '..\build'),
    [string]$Version = '1.0.0',
    [string]$OutputRoot = 'P:\_tmp'
)

$ErrorActionPreference = 'Stop'

$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$resolvedBuildRoot = (Resolve-Path -LiteralPath $BuildRoot).Path
$resolvedOutputRoot = (Resolve-Path -LiteralPath $OutputRoot).Path

if (-not $resolvedOutputRoot.StartsWith('P:\_tmp', [System.StringComparison]::OrdinalIgnoreCase)) {
    throw 'Release staging must remain under P:\_tmp.'
}

$artifactRoot = Join-Path $resolvedBuildRoot 'VRCScriabinNoteLink_artefacts\Release'
$vstSource = Join-Path $artifactRoot 'VST3\VRC Scriabin Note Link.vst3'
$standaloneSource = Join-Path $artifactRoot 'Standalone\VRC Scriabin Note Link.exe'

foreach ($required in @($vstSource, $standaloneSource)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required Release artifact was not found: $required"
    }
}

$releaseName = "VRC-Scriabin-Note-Link-v$Version-Windows-x64"
$stage = Join-Path $resolvedOutputRoot $releaseName
$zipPath = Join-Path $resolvedOutputRoot "$releaseName.zip"
$zipHashPath = "$zipPath.sha256"

foreach ($target in @($stage, $zipPath, $zipHashPath)) {
    if (Test-Path -LiteralPath $target) {
        $resolvedTarget = (Resolve-Path -LiteralPath $target).Path
        if (-not $resolvedTarget.StartsWith('P:\_tmp', [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to replace a target outside P:\_tmp: $resolvedTarget"
        }
        Remove-Item -LiteralPath $resolvedTarget -Recurse -Force
    }
}

New-Item -ItemType Directory -Path $stage | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stage 'VST3') | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stage 'Standalone') | Out-Null

Copy-Item -LiteralPath $vstSource -Destination (Join-Path $stage 'VST3') -Recurse
Copy-Item -LiteralPath $standaloneSource -Destination (Join-Path $stage 'Standalone')
foreach ($document in @('README.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md', 'SOURCE_OFFER.txt')) {
    Copy-Item -LiteralPath (Join-Path $projectRoot $document) -Destination $stage
}
Copy-Item -LiteralPath (Join-Path $projectRoot 'THIRD_PARTY_LICENSES') -Destination $stage -Recurse

$payloadFiles = Get-ChildItem -LiteralPath $stage -File -Recurse | Sort-Object FullName
$checksumLines = foreach ($file in $payloadFiles) {
    $relative = $file.FullName.Substring($stage.Length + 1).Replace('\', '/')
    $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $relative"
}
$checksumLines | Set-Content -LiteralPath (Join-Path $stage 'SHA256SUMS.txt') -Encoding utf8NoBOM

Compress-Archive -LiteralPath $stage -DestinationPath $zipPath -CompressionLevel Optimal
$zipHash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
"$zipHash  $([System.IO.Path]::GetFileName($zipPath))" | Set-Content -LiteralPath $zipHashPath -Encoding ascii

[pscustomobject]@{
    Stage = $stage
    Zip = $zipPath
    ZipSha256 = $zipHash
    PayloadFiles = (Get-ChildItem -LiteralPath $stage -File -Recurse).Count
}
